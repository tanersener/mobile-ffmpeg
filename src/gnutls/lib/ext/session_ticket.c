/*
 * Copyright (C) 2009-2018 Free Software Foundation, Inc.
 *
 * Author: Daiki Ueno, Ander Juaristi
 *
 * This file is part of GnuTLS.
 *
 * The GnuTLS is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>
 *
 */

#include "gnutls_int.h"
#include "errors.h"
#include <datum.h>
#include <algorithms.h>
#include <handshake.h>
#include <num.h>
#include <constate.h>
#include <session_pack.h>
#include <random.h>
#include <ext/session_ticket.h>
#include <mbuffers.h>
#include <hello_ext.h>
#include <constate.h>
#include <dtls.h>
#include "stek.h"
#include "db.h"

static int session_ticket_recv_params(gnutls_session_t session,
				      const uint8_t * data,
				      size_t data_size);
static int session_ticket_send_params(gnutls_session_t session,
				      gnutls_buffer_st * extdata);
static int session_ticket_unpack(gnutls_buffer_st * ps,
				 gnutls_ext_priv_data_t * _priv);
static int session_ticket_pack(gnutls_ext_priv_data_t _priv,
			       gnutls_buffer_st * ps);
static void session_ticket_deinit_data(gnutls_ext_priv_data_t priv);

const hello_ext_entry_st ext_mod_session_ticket = {
	.name = "Session Ticket",
	.tls_id = 35,
	.gid = GNUTLS_EXTENSION_SESSION_TICKET,
	.validity = GNUTLS_EXT_FLAG_TLS | GNUTLS_EXT_FLAG_DTLS | GNUTLS_EXT_FLAG_CLIENT_HELLO |
		    GNUTLS_EXT_FLAG_TLS12_SERVER_HELLO,
	.parse_type = GNUTLS_EXT_TLS,
	.recv_func = session_ticket_recv_params,
	.send_func = session_ticket_send_params,
	.pack_func = session_ticket_pack,
	.unpack_func = session_ticket_unpack,
	.deinit_func = session_ticket_deinit_data,
	.cannot_be_overriden = 1
};

typedef struct {
	uint8_t *session_ticket;
	int session_ticket_len;
} session_ticket_ext_st;

static void
deinit_ticket(struct ticket_st *ticket)
{
	free(ticket->encrypted_state);
}

static int
unpack_ticket(const gnutls_datum_t *ticket_data, struct ticket_st *ticket)
{
	const uint8_t * data = ticket_data->data;
	ssize_t data_size = ticket_data->size;
	const uint8_t *encrypted_state;

	/* Format:
	 *  Key name
	 *  IV
	 *  data length
	 *  encrypted data
	 *  MAC
	 */
	DECR_LEN(data_size, TICKET_KEY_NAME_SIZE);
	memcpy(ticket->key_name, data, TICKET_KEY_NAME_SIZE);
	data += TICKET_KEY_NAME_SIZE;

	DECR_LEN(data_size, TICKET_IV_SIZE);
	memcpy(ticket->IV, data, TICKET_IV_SIZE);
	data += TICKET_IV_SIZE;

	DECR_LEN(data_size, 2);
	ticket->encrypted_state_len = _gnutls_read_uint16(data);
	data += 2;

	encrypted_state = data;

	DECR_LEN(data_size, ticket->encrypted_state_len);
	data += ticket->encrypted_state_len;

	DECR_LEN(data_size, TICKET_MAC_SIZE);
	memcpy(ticket->mac, data, TICKET_MAC_SIZE);

	ticket->encrypted_state =
		gnutls_malloc(ticket->encrypted_state_len);
	if (!ticket->encrypted_state) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}
	memcpy(ticket->encrypted_state, encrypted_state,
	       ticket->encrypted_state_len);

	return 0;
}

static void
pack_ticket(const struct ticket_st *ticket, gnutls_datum_t *ticket_data)
{
	uint8_t *p;

	p = ticket_data->data;

	memcpy(p, ticket->key_name, TICKET_KEY_NAME_SIZE);
	p += TICKET_KEY_NAME_SIZE;

	memcpy(p, ticket->IV, TICKET_IV_SIZE);
	p += TICKET_IV_SIZE;

	_gnutls_write_uint16(ticket->encrypted_state_len, p);
	p += 2;

	memcpy(p, ticket->encrypted_state, ticket->encrypted_state_len);
	p += ticket->encrypted_state_len;

	memcpy(p, ticket->mac, TICKET_MAC_SIZE);
}

static
int digest_ticket(const gnutls_datum_t * key, struct ticket_st *ticket,
	      uint8_t * digest)
{
	mac_hd_st digest_hd;
	uint16_t length16;
	int ret;

	ret = _gnutls_mac_init(&digest_hd, mac_to_entry(TICKET_MAC_ALGO),
			      key->data, key->size);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	_gnutls_mac(&digest_hd, ticket->key_name, TICKET_KEY_NAME_SIZE);
	_gnutls_mac(&digest_hd, ticket->IV, TICKET_IV_SIZE);
	length16 = _gnutls_conv_uint16(ticket->encrypted_state_len);
	_gnutls_mac(&digest_hd, &length16, 2);
	_gnutls_mac(&digest_hd, ticket->encrypted_state,
		   ticket->encrypted_state_len);
	_gnutls_mac_deinit(&digest_hd, digest);

	return 0;
}

int
_gnutls_decrypt_session_ticket(gnutls_session_t session,
			       const gnutls_datum_t *ticket_data,
			       gnutls_datum_t *state)
{
	cipher_hd_st cipher_hd;
	gnutls_datum_t IV;
	gnutls_datum_t stek_key_name, stek_cipher_key, stek_mac_key;
	uint8_t cmac[TICKET_MAC_SIZE];
	struct ticket_st ticket;
	int ret;

	/* callers must have that checked */
	assert(!(session->internals.flags & GNUTLS_NO_TICKETS));

	/* Retrieve ticket decryption keys */
	if (_gnutls_get_session_ticket_decryption_key(session,
						      ticket_data,
						      &stek_key_name,
						      &stek_mac_key,
						      &stek_cipher_key) < 0)
		return gnutls_assert_val(GNUTLS_E_DECRYPTION_FAILED);

	ret = unpack_ticket(ticket_data, &ticket);
	if (ret < 0)
		return ret;

	/* If the key name of the ticket does not match the one that is currently active,
	   issue a new ticket. */
	if (memcmp
	    (ticket.key_name, stek_key_name.data,
	     stek_key_name.size)) {
		ret = GNUTLS_E_DECRYPTION_FAILED;
		goto cleanup;
	}

	/* Check the integrity of ticket */
	ret = digest_ticket(&stek_mac_key, &ticket, cmac);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	if (memcmp(ticket.mac, cmac, TICKET_MAC_SIZE)) {
		ret = gnutls_assert_val(GNUTLS_E_DECRYPTION_FAILED);
		goto cleanup;
	}

	if (ticket.encrypted_state_len % TICKET_BLOCK_SIZE != 0) {
		ret = gnutls_assert_val(GNUTLS_E_DECRYPTION_FAILED);
		goto cleanup;
	}

	/* Decrypt encrypted_state */
	IV.data = ticket.IV;
	IV.size = TICKET_IV_SIZE;
	ret =
	    _gnutls_cipher_init(&cipher_hd,
				cipher_to_entry(TICKET_CIPHER),
				&stek_cipher_key, &IV, 0);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = _gnutls_cipher_decrypt(&cipher_hd, ticket.encrypted_state,
				     ticket.encrypted_state_len);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup2;
	}

	state->data = ticket.encrypted_state;
	state->size = ticket.encrypted_state_len;

	ticket.encrypted_state = NULL;

	ret = 0;

cleanup2:
	_gnutls_cipher_deinit(&cipher_hd);

cleanup:
	deinit_ticket(&ticket);

	return ret;

}

int
_gnutls_encrypt_session_ticket(gnutls_session_t session,
			       const gnutls_datum_t *state,
			       gnutls_datum_t *ticket_data)
{
	cipher_hd_st cipher_hd;
	gnutls_datum_t IV;
	gnutls_datum_t encrypted_state = {NULL,0};
	uint8_t iv[TICKET_IV_SIZE];
	gnutls_datum_t stek_cipher_key, stek_mac_key, stek_key_name;
	struct ticket_st ticket;
	int ret;

	encrypted_state.size = ((state->size + TICKET_BLOCK_SIZE - 1) / TICKET_BLOCK_SIZE) * TICKET_BLOCK_SIZE;
	ticket_data->size = TICKET_KEY_NAME_SIZE + TICKET_IV_SIZE + 2 +
	    encrypted_state.size + TICKET_MAC_SIZE;
	ticket_data->data = gnutls_calloc(1, ticket_data->size);
	if (!ticket_data->data) {
		gnutls_assert();
		ret = GNUTLS_E_MEMORY_ERROR;
		goto cleanup;
	}
	encrypted_state.data = ticket_data->data + TICKET_KEY_NAME_SIZE + TICKET_IV_SIZE + 2;
	memcpy(encrypted_state.data, state->data, state->size);

	/* Retrieve ticket encryption keys */
	if (_gnutls_get_session_ticket_encryption_key(session,
						      &stek_key_name,
						      &stek_mac_key,
						      &stek_cipher_key) < 0) {
		ret = GNUTLS_E_ENCRYPTION_FAILED;
		goto cleanup;
	}

	/* Encrypt state */
	IV.data = iv;
	IV.size = TICKET_IV_SIZE;

	ret = gnutls_rnd(GNUTLS_RND_NONCE, iv, TICKET_IV_SIZE);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret =
	    _gnutls_cipher_init(&cipher_hd,
				cipher_to_entry(TICKET_CIPHER),
				&stek_cipher_key, &IV, 1);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = _gnutls_cipher_encrypt(&cipher_hd, encrypted_state.data,
				     encrypted_state.size);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup2;
	}


	/* Fill the ticket structure to compute MAC. */
	memcpy(ticket.key_name, stek_key_name.data, stek_key_name.size);
	memcpy(ticket.IV, IV.data, IV.size);
	ticket.encrypted_state_len = encrypted_state.size;
	ticket.encrypted_state = encrypted_state.data;

	ret = digest_ticket(&stek_mac_key, &ticket, ticket.mac);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup2;
	}

	encrypted_state.data = NULL;

	pack_ticket(&ticket, ticket_data);

	ret = 0;

cleanup2:
	_gnutls_cipher_deinit(&cipher_hd);

cleanup:
	_gnutls_free_datum(&encrypted_state);

	return ret;
}

static int
unpack_session(gnutls_session_t session, const gnutls_datum_t *state)
{
	int ret;

	if (unlikely(!state))
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

	ret = _gnutls_session_unpack(session, state);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = _gnutls_check_resumed_params(session);
	if (ret < 0)
		return gnutls_assert_val(ret);

	session->internals.resumed = RESUME_TRUE;
	return 0;
}

static int
session_ticket_recv_params(gnutls_session_t session,
			   const uint8_t * data, size_t _data_size)
{
	gnutls_datum_t ticket_data;
	gnutls_datum_t state;
	ssize_t data_size = _data_size;
	int ret;

	if (session->internals.flags & GNUTLS_NO_TICKETS)
		return 0;

	if (session->security_parameters.entity == GNUTLS_SERVER) {
		/* The client requested a new session ticket. */
		if (data_size == 0) {
			session->internals.session_ticket_renew = 1;
			return 0;
		}

		ticket_data.data = (void *)data;
		ticket_data.size = data_size;
		if ((ret = _gnutls_decrypt_session_ticket(session, &ticket_data, &state)) == 0) {
			ret = unpack_session(session, &state);

			_gnutls_free_datum(&state);
		}

		if (ret < 0) {
			session->internals.session_ticket_renew = 1;
			return 0;
		}
	} else {		/* Client */

		if (data_size == 0) {
			session->internals.session_ticket_renew = 1;
			return 0;
		}
	}

	return 0;
}

/* returns a positive number if we send the extension data, (0) if we
   do not want to send it, and a negative number on failure.
 */
static int
session_ticket_send_params(gnutls_session_t session,
			   gnutls_buffer_st * extdata)
{
	session_ticket_ext_st *priv = NULL;
	gnutls_ext_priv_data_t epriv;
	int ret;

	if (session->internals.flags & GNUTLS_NO_TICKETS)
		return 0;

	if (session->security_parameters.entity == GNUTLS_SERVER) {
		if (session->internals.session_ticket_renew) {
			return GNUTLS_E_INT_RET_0;
		}
	} else {
		ret =
		    _gnutls_hello_ext_get_resumed_priv(session,
							 GNUTLS_EXTENSION_SESSION_TICKET,
							 &epriv);
		if (ret >= 0)
			priv = epriv;

		/* no previous data. Just advertize it */
		if (ret < 0)
			return GNUTLS_E_INT_RET_0;

		/* previous data had session tickets disabled. Don't advertize. Ignore. */
		if (session->internals.flags & GNUTLS_NO_TICKETS)
			return 0;

		if (priv->session_ticket_len > 0) {
			ret =
			    _gnutls_buffer_append_data(extdata,
						       priv->
						       session_ticket,
						       priv->
						       session_ticket_len);
			if (ret < 0)
				return gnutls_assert_val(ret);

			return priv->session_ticket_len;
		}
	}
	return 0;
}


static void session_ticket_deinit_data(gnutls_ext_priv_data_t epriv)
{
	session_ticket_ext_st *priv = epriv;

	gnutls_free(priv->session_ticket);
	gnutls_free(priv);
}

static int
session_ticket_pack(gnutls_ext_priv_data_t epriv, gnutls_buffer_st * ps)
{
	session_ticket_ext_st *priv = epriv;
	int ret;

	BUFFER_APPEND_PFX4(ps, priv->session_ticket,
			   priv->session_ticket_len);

	return 0;
}

static int
session_ticket_unpack(gnutls_buffer_st * ps, gnutls_ext_priv_data_t * _priv)
{
	session_ticket_ext_st *priv = NULL;
	int ret;
	gnutls_ext_priv_data_t epriv;
	gnutls_datum_t ticket;

	priv = gnutls_calloc(1, sizeof(*priv));
	if (priv == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	BUFFER_POP_DATUM(ps, &ticket);
	priv->session_ticket = ticket.data;
	priv->session_ticket_len = ticket.size;

	epriv = priv;
	*_priv = epriv;

	return 0;

      error:
	gnutls_free(priv);
	return ret;
}



/**
 * gnutls_session_ticket_key_generate:
 * @key: is a pointer to a #gnutls_datum_t which will contain a newly
 * created key.
 *
 * Generate a random key to encrypt security parameters within
 * SessionTicket.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, or an
 * error code.
 *
 * Since: 2.10.0
 **/
int gnutls_session_ticket_key_generate(gnutls_datum_t * key)
{
	if (_gnutls_fips_mode_enabled()) {
		int ret;
		/* in FIPS140-2 mode gnutls_key_generate imposes
		 * some limits on allowed key size, thus it is not
		 * used. These limits do not affect this function as
		 * it does not generate a "key" but rather key material
		 * that includes nonces and other stuff. */
		key->data = gnutls_malloc(TICKET_MASTER_KEY_SIZE);
		if (key->data == NULL)
			return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

		key->size = TICKET_MASTER_KEY_SIZE;
		ret = gnutls_rnd(GNUTLS_RND_RANDOM, key->data, key->size);
		if (ret < 0) {
			gnutls_free(key->data);
			return ret;
		}
		return 0;
	} else {
		return gnutls_key_generate(key, TICKET_MASTER_KEY_SIZE);
	}
}

/**
 * gnutls_session_ticket_enable_client:
 * @session: is a #gnutls_session_t type.
 *
 * Request that the client should attempt session resumption using
 * SessionTicket. This call is typically unnecessary as session
 * tickets are enabled by default.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, or an
 * error code.
 *
 * Since: 2.10.0
 **/
int gnutls_session_ticket_enable_client(gnutls_session_t session)
{
	if (!session) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	session->internals.flags &= ~GNUTLS_NO_TICKETS;

	return 0;
}

/**
 * gnutls_session_ticket_enable_server:
 * @session: is a #gnutls_session_t type.
 * @key: key to encrypt session parameters.
 *
 * Request that the server should attempt session resumption using
 * session tickets, i.e., by delegating storage to the client.
 * @key must be initialized using gnutls_session_ticket_key_generate().
 * To avoid leaking that key, use gnutls_memset() prior to
 * releasing it.
 *
 * The default ticket expiration time can be overridden using
 * gnutls_db_set_cache_expiration().
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, or an
 * error code.
 *
 * Since: 2.10.0
 **/
int
gnutls_session_ticket_enable_server(gnutls_session_t session,
				    const gnutls_datum_t * key)
{
	int ret;

	if (!session || !key || key->size != TICKET_MASTER_KEY_SIZE || !key->data) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	ret = _gnutls_initialize_session_ticket_key_rotation(session, key);
	if (ret < 0)
		return gnutls_assert_val(ret);

	session->internals.flags &= ~GNUTLS_NO_TICKETS;

	return 0;
}

/*
 * Return zero if session tickets haven't been enabled.
 */
int _gnutls_send_new_session_ticket(gnutls_session_t session, int again)
{
	mbuffer_st *bufel = NULL;
	uint8_t *data = NULL, *p;
	int data_size = 0;
	int ret;
	gnutls_datum_t state = { NULL, 0 };
	uint16_t epoch_saved = session->security_parameters.epoch_write;
	gnutls_datum_t ticket_data;

	if (again == 0) {
		if (session->internals.flags & GNUTLS_NO_TICKETS)
			return 0;
		if (!session->internals.session_ticket_renew)
			return 0;

		_gnutls_handshake_log
		    ("HSK[%p]: sending session ticket\n", session);

		/* XXX: Temporarily set write algorithms to be used.
		   _gnutls_write_connection_state_init() does this job, but it also
		   triggers encryption, while NewSessionTicket should not be
		   encrypted in the record layer. */
		ret =
		    _gnutls_epoch_set_keys(session,
					   session->security_parameters.
					   epoch_next, 0);
		if (ret < 0) {
			gnutls_assert();
			return ret;
		}

		/* Under TLS1.2 with session tickets, the session ID is used for different
		 * purposes than the TLS1.0 session ID. Ensure that there is an internally
		 * set value which the server will see on the original and resumed sessions */
		if (session->internals.resumed != RESUME_TRUE) {
			ret = _gnutls_generate_session_id(session->security_parameters.
							  session_id,
							  &session->security_parameters.
							  session_id_size);
			if (ret < 0) {
				gnutls_assert();
				return ret;
			}
		}

		session->security_parameters.epoch_write =
		    session->security_parameters.epoch_next;

		/* Pack security parameters. */
		ret = _gnutls_session_pack(session, &state);
		if (ret < 0) {
			gnutls_assert();
			return ret;
		}

		/* Generate an encrypted ticket */
		ret = _gnutls_encrypt_session_ticket(session, &state, &ticket_data);
		session->security_parameters.epoch_write = epoch_saved;
		_gnutls_free_datum(&state);
		if (ret < 0) {
			gnutls_assert();
			return ret;
		}

		bufel =
		    _gnutls_handshake_alloc(session, 
					    4 + 2 + ticket_data.size);
		if (!bufel) {
			gnutls_assert();
			_gnutls_free_datum(&ticket_data);
			return GNUTLS_E_MEMORY_ERROR;
		}

		data = _mbuffer_get_udata_ptr(bufel);
		p = data;

		_gnutls_write_uint32(session->internals.expire_time, p);
		p += 4;

		_gnutls_write_uint16(ticket_data.size, p);
		p += 2;

		memcpy(p, ticket_data.data, ticket_data.size);
		p += ticket_data.size;

		_gnutls_free_datum(&ticket_data);

		data_size = p - data;

		session->internals.hsk_flags |= HSK_TLS12_TICKET_SENT;
	}
	return _gnutls_send_handshake(session, data_size ? bufel : NULL,
				      GNUTLS_HANDSHAKE_NEW_SESSION_TICKET);
}

/*
 * Return zero if session tickets haven't been enabled.
 */
int _gnutls_recv_new_session_ticket(gnutls_session_t session)
{
	uint8_t *p;
	int data_size;
	gnutls_buffer_st buf;
	uint16_t ticket_len;
	int ret;
	session_ticket_ext_st *priv = NULL;
	gnutls_ext_priv_data_t epriv;

	if (session->internals.flags & GNUTLS_NO_TICKETS)
		return 0;
	if (!session->internals.session_ticket_renew)
		return 0;

	/* This is the last flight and peer cannot be sure
	 * we have received it unless we notify him. So we
	 * wait for a message and retransmit if needed. */
	if (IS_DTLS(session) && !_dtls_is_async(session)) {
		unsigned have;
		mbuffer_st *bufel = NULL;

		have = gnutls_record_check_pending(session) +
		       record_check_unprocessed(session);

		if (have != 0) {
			bufel = _mbuffer_head_get_first(&session->internals.record_buffer, NULL);
		}

		if (have == 0 || (bufel && bufel->type != GNUTLS_HANDSHAKE)) {
			ret = _dtls_wait_and_retransmit(session);
			if (ret < 0)
				return gnutls_assert_val(ret);
		}
	}

	ret = _gnutls_recv_handshake(session,
				     GNUTLS_HANDSHAKE_NEW_SESSION_TICKET,
				     0, &buf);
	if (ret < 0)
		return gnutls_assert_val_fatal(ret);

	p = buf.data;
	data_size = buf.length;

	DECR_LENGTH_COM(data_size, 4, ret =
			GNUTLS_E_UNEXPECTED_PACKET_LENGTH;
			goto error);
	/* skip over lifetime hint */
	p += 4;

	DECR_LENGTH_COM(data_size, 2, ret =
			GNUTLS_E_UNEXPECTED_PACKET_LENGTH;
			goto error);
	ticket_len = _gnutls_read_uint16(p);
	p += 2;

	DECR_LENGTH_COM(data_size, ticket_len, ret =
			GNUTLS_E_UNEXPECTED_PACKET_LENGTH;
			goto error);

	priv = gnutls_calloc(1, sizeof(*priv));
	if (!priv) {
		gnutls_assert();
		ret = GNUTLS_E_MEMORY_ERROR;
		goto error;
	}
	priv->session_ticket =
	    gnutls_realloc_fast(priv->session_ticket, ticket_len);
	if (!priv->session_ticket) {
		gnutls_free(priv);
		gnutls_assert();
		ret = GNUTLS_E_MEMORY_ERROR;
		goto error;
	}
	memcpy(priv->session_ticket, p, ticket_len);
	priv->session_ticket_len = ticket_len;
	epriv = priv;

	/* Discard the current session ID.  (RFC5077 3.4) */
	ret =
	    _gnutls_generate_session_id(session->security_parameters.
					session_id,
					&session->security_parameters.
					session_id_size);
	if (ret < 0) {
		gnutls_assert();
		session_ticket_deinit_data(epriv);
		ret = GNUTLS_E_INTERNAL_ERROR;
		goto error;
	}
	ret = 0;

	_gnutls_handshake_log
		    ("HSK[%p]: received session ticket\n", session);
	session->internals.hsk_flags |= HSK_TICKET_RECEIVED;

	_gnutls_hello_ext_set_priv(session,
			GNUTLS_EXTENSION_SESSION_TICKET,
			epriv);

      error:
	_gnutls_buffer_clear(&buf);

	return ret;
}
