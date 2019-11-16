/*
 * Copyright (C) 2017-2018 Red Hat, Inc.
 *
 * Author: Nikos Mavrogiannopoulos, Ander Juaristi
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
#include "extv.h"
#include "handshake.h"
#include "mbuffers.h"
#include "ext/pre_shared_key.h"
#include "ext/session_ticket.h"
#include "ext/early_data.h"
#include "auth/cert.h"
#include "tls13/session_ticket.h"
#include "session_pack.h"
#include "db.h"

static int
pack_ticket(gnutls_session_t session, tls13_ticket_st *ticket, gnutls_datum_t *packed)
{
	uint8_t *p;
	gnutls_datum_t state;
	int ret;

	ret = _gnutls_session_pack(session, &state);
	if (ret < 0)
		return gnutls_assert_val(ret);

	packed->size = 2 + 4 + 4 +
		1 + ticket->prf->output_size +
		1 + ticket->nonce_size + 2 + state.size + 12;

	packed->data = gnutls_malloc(packed->size);
	if (!packed->data) {
		gnutls_assert();
		ret = GNUTLS_E_MEMORY_ERROR;
		goto cleanup;
	}

	p = packed->data;

	_gnutls_write_uint16(ticket->prf->id, p);
	p += 2;
	_gnutls_write_uint32(ticket->age_add, p);
	p += 4;
	_gnutls_write_uint32(ticket->lifetime, p);
	p += 4;
	*p = ticket->prf->output_size;
	p += 1;
	memcpy(p, ticket->resumption_master_secret, ticket->prf->output_size);
	p += ticket->prf->output_size;
	*p = ticket->nonce_size;

	p += 1;
	memcpy(p, ticket->nonce, ticket->nonce_size);
	p += ticket->nonce_size;

	_gnutls_write_uint16(state.size, p);
	p += 2;

	memcpy(p, state.data, state.size);
	p += state.size;

	_gnutls_write_uint32((uint64_t) ticket->creation_time.tv_sec >> 32, p);
	p += 4;
	_gnutls_write_uint32(ticket->creation_time.tv_sec & 0xFFFFFFFF, p);
	p += 4;
	_gnutls_write_uint32(ticket->creation_time.tv_nsec, p);

	ret = 0;

 cleanup:
	gnutls_free(state.data);
	return ret;
}

static int
unpack_ticket(gnutls_session_t session, gnutls_datum_t *packed, tls13_ticket_st *data)
{
	uint32_t age_add, lifetime;
	struct timespec creation_time;
	uint8_t resumption_master_secret[MAX_HASH_SIZE];
	size_t resumption_master_secret_size;
	uint8_t nonce[UINT8_MAX];
	size_t nonce_size;
	gnutls_datum_t state;
	gnutls_mac_algorithm_t kdf;
	const mac_entry_st *prf;
	uint8_t *p;
	size_t len;
	uint64_t v;
	int ret;

	if (unlikely(packed == NULL || data == NULL))
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

	memset(data, 0, sizeof(*data));

	p = packed->data;
	len = packed->size;

	DECR_LEN(len, 2);
	kdf = _gnutls_read_uint16(p);
	p += 2;

	/* Check if the MAC ID we got is valid */
	prf = _gnutls_mac_to_entry(kdf);
	if (prf == NULL)
		return gnutls_assert_val(GNUTLS_E_ILLEGAL_PARAMETER);

	/* Read the ticket age add and the ticket lifetime */
	DECR_LEN(len, 4);
	age_add = _gnutls_read_uint32(p);
	p += 4;

	DECR_LEN(len, 4);
	lifetime = _gnutls_read_uint32(p);
	p += 4;

	/*
	 * Check if the whole ticket is large enough,
	 * and read the resumption master secret
	 */
	DECR_LEN(len, 1);
	resumption_master_secret_size = *p;
	p += 1;

	/* Check if the size of resumption_master_secret matches the PRF */
	if (resumption_master_secret_size != prf->output_size)
		return gnutls_assert_val(GNUTLS_E_ILLEGAL_PARAMETER);

	DECR_LEN(len, resumption_master_secret_size);
	memcpy(resumption_master_secret, p, resumption_master_secret_size);
	p += resumption_master_secret_size;

	/* Read the ticket nonce */
	DECR_LEN(len, 1);
	nonce_size = *p;
	p += 1;

	DECR_LEN(len, nonce_size);
	memcpy(nonce, p, nonce_size);
	p += nonce_size;

	DECR_LEN(len, 2);
	state.size = _gnutls_read_uint16(p);
	p += 2;

	DECR_LEN(len, state.size);
	state.data = p;
	p += state.size;

	DECR_LEN(len, 12);
	v = _gnutls_read_uint32(p);
	p += 4;
	creation_time.tv_sec = (v << 32) | _gnutls_read_uint32(p);
	p += 4;
	creation_time.tv_nsec = _gnutls_read_uint32(p);

	ret = _gnutls_session_unpack(session, &state);
	if (ret < 0)
		return gnutls_assert_val(ret);

	/* No errors - Now return all the data to the caller */
	data->prf = prf;
	memcpy(data->resumption_master_secret, resumption_master_secret,
	       resumption_master_secret_size);
	memcpy(data->nonce, nonce, nonce_size);
	data->nonce_size = nonce_size;
	data->age_add = age_add;
	data->lifetime = lifetime;
	memcpy(&data->creation_time, &creation_time, sizeof(struct timespec));

	return 0;
}

static int
generate_session_ticket(gnutls_session_t session, tls13_ticket_st *ticket)
{
	int ret;
	gnutls_datum_t packed = { NULL, 0 };
	struct timespec now;
	tls13_ticket_st ticket_data;

	gnutls_gettime(&now);
	if (session->internals.resumed != RESUME_FALSE) {
		/* If we are resuming ensure that we don't extend the lifetime
		 * of the ticket past the original session expiration time */
		if (now.tv_sec >= session->security_parameters.timestamp + session->internals.expire_time)
			return GNUTLS_E_INT_RET_0; /* don't send ticket */
		else
			ticket->lifetime = session->security_parameters.timestamp +
					   session->internals.expire_time - now.tv_sec;
	} else {
		/* Set ticket lifetime to the default expiration time */
		ticket->lifetime = session->internals.expire_time;
	}

	/* Generate a random 32-bit ticket nonce */
	ticket->nonce_size = 4;

	if ((ret = gnutls_rnd(GNUTLS_RND_NONCE,
			ticket->nonce, ticket->nonce_size)) < 0)
		return gnutls_assert_val(ret);

	if ((ret = gnutls_rnd(GNUTLS_RND_NONCE, &ticket->age_add, sizeof(uint32_t))) < 0)
		return gnutls_assert_val(ret);
	/* This is merely to produce the same binder value on
	 * different endian architectures. */
#ifdef WORDS_BIGENDIAN
	ticket->age_add = bswap_32(ticket->age_add);
#endif

	ticket->prf = session->security_parameters.prf;

	/* Encrypt the ticket and place the result in ticket->ticket */
	ticket_data.lifetime = ticket->lifetime;
	ticket_data.age_add = ticket->age_add;
	memcpy(&ticket_data.creation_time, &now, sizeof(struct timespec));
	memcpy(ticket_data.nonce, ticket->nonce, ticket->nonce_size);
	ticket_data.nonce_size = ticket->nonce_size;
	ticket_data.prf = ticket->prf;
	memcpy(&ticket_data.resumption_master_secret,
	       session->key.proto.tls13.ap_rms,
	       ticket->prf->output_size);

	ret = pack_ticket(session, &ticket_data, &packed);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = _gnutls_encrypt_session_ticket(session, &packed, &ticket->ticket);
	_gnutls_free_datum(&packed);
	if (ret < 0)
		return gnutls_assert_val(ret);

	return 0;
}

static int append_nst_extension(void *ctx, gnutls_buffer_st *buf)
{
	gnutls_session_t session = ctx;
	int ret;

	if (!(session->internals.flags & GNUTLS_ENABLE_EARLY_DATA))
		return 0;

	ret = _gnutls_buffer_append_prefix(buf, 32,
					   session->security_parameters.
					   max_early_data_size);
	if (ret < 0)
		gnutls_assert();

	return ret;
}

int _gnutls13_send_session_ticket(gnutls_session_t session, unsigned nr, unsigned again)
{
	int ret = 0;
	mbuffer_st *bufel = NULL;
	gnutls_buffer_st buf;
	tls13_ticket_st ticket;
	unsigned i;

	/* Client does not send a NewSessionTicket */
	if (unlikely(session->security_parameters.entity == GNUTLS_CLIENT))
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

	/* Session resumption has not been enabled */
	if (session->internals.flags & GNUTLS_NO_TICKETS)
		return gnutls_assert_val(0);

	/* If we received the psk_key_exchange_modes extension which
	 * does not have overlap with the server configuration, don't
	 * send a session ticket */
	if (session->internals.hsk_flags & HSK_PSK_KE_MODE_INVALID)
		return gnutls_assert_val(0);

	if (again == 0) {
		for (i=0;i<nr;i++) {
			unsigned init_pos;

			memset(&ticket, 0, sizeof(tls13_ticket_st));
			bufel = NULL;

			ret = _gnutls_buffer_init_handshake_mbuffer(&buf);
			if (ret < 0)
				return gnutls_assert_val(ret);

			ret = generate_session_ticket(session, &ticket);
			if (ret < 0) {
				if (ret == GNUTLS_E_INT_RET_0) {
					ret = gnutls_assert_val(0);
					goto cleanup;
				}
				gnutls_assert();
				goto cleanup;
			}

			ret = _gnutls_buffer_append_prefix(&buf, 32, ticket.lifetime);
			if (ret < 0) {
				gnutls_assert();
				goto cleanup;
			}

			ret = _gnutls_buffer_append_prefix(&buf, 32, ticket.age_add);
			if (ret < 0) {
				gnutls_assert();
				goto cleanup;
			}

			/* append ticket_nonce */
			ret = _gnutls_buffer_append_data_prefix(&buf, 8, ticket.nonce, ticket.nonce_size);
			if (ret < 0) {
				gnutls_assert();
				goto cleanup;
			}

			/* append ticket */
			ret = _gnutls_buffer_append_data_prefix(&buf, 16, ticket.ticket.data, ticket.ticket.size);
			if (ret < 0) {
				gnutls_assert();
				goto cleanup;
			}

			_gnutls_free_datum(&ticket.ticket);

			/* append extensions */
			ret = _gnutls_extv_append_init(&buf);
			if (ret < 0) {
				gnutls_assert();
				goto cleanup;
			}
			init_pos = ret;

			ret = _gnutls_extv_append(&buf, ext_mod_early_data.tls_id, session,
						  (extv_append_func)append_nst_extension);
			if (ret < 0) {
				gnutls_assert();
				goto cleanup;
			}

			ret = _gnutls_extv_append_final(&buf, init_pos, 0);
			if (ret < 0) {
				gnutls_assert();
				goto cleanup;
			}

			bufel = _gnutls_buffer_to_mbuffer(&buf);

			ret = _gnutls_send_handshake2(session, bufel,
						      GNUTLS_HANDSHAKE_NEW_SESSION_TICKET, 1);
			if (ret < 0) {
				gnutls_assert();
				goto cleanup;
			}

			session->internals.hsk_flags |= HSK_TLS13_TICKET_SENT;
		}
	}

	ret = _gnutls_handshake_io_write_flush(session);

	return ret;

cleanup:
	_gnutls_free_datum(&ticket.ticket);
	_mbuffer_xfree(&bufel);
	_gnutls_buffer_clear(&buf);

	return ret;
}

static int parse_nst_extension(void *ctx, unsigned tls_id, const unsigned char *data, unsigned data_size)
{
	gnutls_session_t session = ctx;
	if (tls_id == ext_mod_early_data.tls_id) {
		if (data_size < 4)
			return gnutls_assert_val(GNUTLS_E_TLS_PACKET_DECODING_ERROR);
		session->security_parameters.max_early_data_size =
			_gnutls_read_uint32(data);
	}
	return 0;
}

int _gnutls13_recv_session_ticket(gnutls_session_t session, gnutls_buffer_st *buf)
{
	int ret;
	uint8_t value;
	tls13_ticket_st *ticket = &session->internals.tls13_ticket;
	gnutls_datum_t t;
	size_t val;

	if (unlikely(buf == NULL))
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

	_gnutls_free_datum(&ticket->ticket);
	memset(ticket, 0, sizeof(tls13_ticket_st));

	_gnutls_handshake_log("HSK[%p]: parsing session ticket message\n", session);

	/* ticket_lifetime */
	ret = _gnutls_buffer_pop_prefix32(buf, &val, 0);
	if (ret < 0)
		return gnutls_assert_val(ret);
	ticket->lifetime = val;

	/* ticket_age_add */
	ret = _gnutls_buffer_pop_prefix32(buf, &val, 0);
	if (ret < 0)
		return gnutls_assert_val(ret);
	ticket->age_add = val;

	/* ticket_nonce */
	ret = _gnutls_buffer_pop_prefix8(buf, &value, 0);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ticket->nonce_size = value;
	ret = _gnutls_buffer_pop_data(buf, ticket->nonce, ticket->nonce_size);
	if (ret < 0)
		return gnutls_assert_val(ret);

	/* ticket */
	ret = _gnutls_buffer_pop_datum_prefix16(buf, &t);
	if (ret < 0)
		return gnutls_assert_val(ret);

	gnutls_free(ticket->ticket.data);
	ret = _gnutls_set_datum(&ticket->ticket, t.data, t.size);
	if (ret < 0)
		return gnutls_assert_val(ret);

	/* Extensions */
	ret = _gnutls_extv_parse(session, parse_nst_extension, buf->data, buf->length);
	if (ret < 0)
		return gnutls_assert_val(ret);

	/* Record the ticket arrival time */
	gnutls_gettime(&ticket->arrival_time);

	return 0;
}

/*
 * Parse the ticket in 'data' and return the resumption master secret
 * and the KDF ID associated to it.
 */
int _gnutls13_unpack_session_ticket(gnutls_session_t session,
		gnutls_datum_t *data,
		tls13_ticket_st *ticket_data)
{
	int ret;
	gnutls_datum_t decrypted = { NULL, 0 };

	if (unlikely(data == NULL || ticket_data == NULL))
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

	/* Check MAC and decrypt ticket */
	ret = _gnutls_decrypt_session_ticket(session, data, &decrypted);
	if (ret < 0)
		return gnutls_assert_val(ret);

	/* Return ticket parameters */
	ret = unpack_ticket(session, &decrypted, ticket_data);
	_gnutls_free_datum(&decrypted);
	if (ret < 0)
		return ret;

	ret = _gnutls_check_resumed_params(session);
	if (ret < 0)
		return gnutls_assert_val(ret);

	return 0;
}
