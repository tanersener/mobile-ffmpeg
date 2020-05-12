/*
 * Copyright (C) 2017-2018 Free Software Foundation, Inc.
 * Copyright (C) 2018 Red Hat, Inc.
 *
 * Author: Ander Juaristi, Nikos Mavrogiannopoulos
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
#include "auth/psk.h"
#include "handshake.h"
#include "kx.h"
#include "secrets.h"
#include "tls13/anti_replay.h"
#include "tls13/psk_ext_parser.h"
#include "tls13/finished.h"
#include "tls13/session_ticket.h"
#include "auth/psk_passwd.h"
#include <ext/session_ticket.h>
#include <ext/pre_shared_key.h>
#include <assert.h>

static int
compute_psk_from_ticket(const tls13_ticket_st *ticket, gnutls_datum_t *key)
{
	int ret;

	if (unlikely(ticket->prf == NULL || ticket->prf->output_size == 0))
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

	key->data = gnutls_malloc(ticket->prf->output_size);
	if (!key->data) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}
	key->size = ticket->prf->output_size;

	ret = _tls13_expand_secret2(ticket->prf,
				    RESUMPTION_LABEL, sizeof(RESUMPTION_LABEL)-1,
				    ticket->nonce, ticket->nonce_size,
				    ticket->resumption_master_secret,
				    key->size,
				    key->data);
	if (ret < 0)
		gnutls_assert();

	return ret;
}

static int
compute_binder_key(const mac_entry_st *prf,
		   const uint8_t *key, size_t keylen,
		   bool resuming,
		   void *out)
{
	int ret;
	const char ext_label[] = EXT_BINDER_LABEL;
	const size_t ext_label_len = sizeof(ext_label) - 1;
	const char res_label[] = RES_BINDER_LABEL;
	const size_t res_label_len = sizeof(res_label) - 1;
	const char *label = resuming ? res_label : ext_label;
	size_t label_len = resuming ? res_label_len : ext_label_len;
	uint8_t tmp_key[MAX_HASH_SIZE];

	/* Compute HKDF-Extract(0, psk) */
	ret = _tls13_init_secret2(prf, key, keylen, tmp_key);
	if (ret < 0)
		return ret;

	/* Compute Derive-Secret(secret, label, transcript_hash) */
	ret = _tls13_derive_secret2(prf, label, label_len,
				    NULL, 0, tmp_key, out);
	if (ret < 0)
		return ret;

	return 0;
}

static int
compute_psk_binder(gnutls_session_t session,
		   const mac_entry_st *prf, unsigned binders_length,
		   int exts_length, int ext_offset,
		   const gnutls_datum_t *psk, const gnutls_datum_t *client_hello,
		   bool resuming, void *out)
{
	int ret;
	unsigned client_hello_pos, extensions_len_pos;
	gnutls_buffer_st handshake_buf;
	uint8_t binder_key[MAX_HASH_SIZE];

	_gnutls_buffer_init(&handshake_buf);

	if (session->security_parameters.entity == GNUTLS_CLIENT) {
		if (session->internals.hsk_flags & HSK_HRR_RECEIVED) {
			ret = gnutls_buffer_append_data(&handshake_buf,
							(const void *) session->internals.handshake_hash_buffer.data,
							session->internals.handshake_hash_buffer.length);
			if (ret < 0) {
				gnutls_assert();
				goto error;
			}
		}

		client_hello_pos = handshake_buf.length;
		ret = gnutls_buffer_append_data(&handshake_buf, client_hello->data,
						client_hello->size);
		if (ret < 0) {
			gnutls_assert();
			goto error;
		}

		/* This is a ClientHello message */
		handshake_buf.data[client_hello_pos] = GNUTLS_HANDSHAKE_CLIENT_HELLO;

		/* At this point we have not yet added the binders to the ClientHello,
		 * but we have to overwrite the size field, pretending as if binders
		 * of the correct length were present.
		 */
		_gnutls_write_uint24(handshake_buf.length - client_hello_pos + binders_length - 2, &handshake_buf.data[client_hello_pos + 1]);
		_gnutls_write_uint16(handshake_buf.length - client_hello_pos + binders_length - ext_offset,
				     &handshake_buf.data[client_hello_pos + ext_offset]);
		extensions_len_pos = handshake_buf.length - client_hello_pos - exts_length - 2;
		_gnutls_write_uint16(exts_length + binders_length + 2,
				     &handshake_buf.data[client_hello_pos + extensions_len_pos]);
	} else {
		if (session->internals.hsk_flags & HSK_HRR_SENT) {
			if (unlikely(session->internals.handshake_hash_buffer.length <= client_hello->size)) {
				ret = gnutls_assert_val(GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER);
				goto error;
			}

			ret = gnutls_buffer_append_data(&handshake_buf,
							session->internals.handshake_hash_buffer.data,
							session->internals.handshake_hash_buffer.length - client_hello->size);
			if (ret < 0) {
				gnutls_assert();
				goto error;
			}
		}

		if (unlikely(client_hello->size <= binders_length)) {
			ret = gnutls_assert_val(GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER);
			goto error;
		}

		ret = gnutls_buffer_append_data(&handshake_buf,
						(const void *) client_hello->data,
						client_hello->size - binders_length);
		if (ret < 0) {
			gnutls_assert();
			goto error;
		}
	}

	ret = compute_binder_key(prf,
				 psk->data, psk->size, resuming,
				 binder_key);
	if (ret < 0) {
		gnutls_assert();
		goto error;
	}

	ret = _gnutls13_compute_finished(prf, binder_key,
					 &handshake_buf,
					 out);
	if (ret < 0) {
		gnutls_assert();
		goto error;
	}

	ret = 0;
error:
	_gnutls_buffer_clear(&handshake_buf);
	return ret;
}

static int
generate_early_secrets(gnutls_session_t session,
		       const mac_entry_st *prf)
{
	int ret;

	ret = _tls13_derive_secret2(prf, EARLY_TRAFFIC_LABEL, sizeof(EARLY_TRAFFIC_LABEL)-1,
				    session->internals.handshake_hash_buffer.data,
				    session->internals.handshake_hash_buffer_client_hello_len,
				    session->key.proto.tls13.temp_secret,
				    session->key.proto.tls13.e_ckey);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = _gnutls_call_keylog_func(session, "CLIENT_EARLY_TRAFFIC_SECRET",
				       session->key.proto.tls13.e_ckey,
				       prf->output_size);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = _tls13_derive_secret2(prf, EARLY_EXPORTER_MASTER_LABEL, sizeof(EARLY_EXPORTER_MASTER_LABEL)-1,
				    session->internals.handshake_hash_buffer.data,
				    session->internals.handshake_hash_buffer_client_hello_len,
				    session->key.proto.tls13.temp_secret,
				    session->key.proto.tls13.ap_expkey);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = _gnutls_call_keylog_func(session, "EARLY_EXPORTER_SECRET",
				       session->key.proto.tls13.ap_expkey,
				       prf->output_size);
	if (ret < 0)
		return gnutls_assert_val(ret);

	return 0;
}

/* Calculate TLS 1.3 Early Secret and the derived secrets from the
 * selected PSK. */
int
_gnutls_generate_early_secrets_for_psk(gnutls_session_t session)
{
	const uint8_t *psk;
	size_t psk_size;
	const mac_entry_st *prf;
	int ret;

	psk = session->key.binders[0].psk.data;
	psk_size = session->key.binders[0].psk.size;
	prf = session->key.binders[0].prf;

	if (unlikely(psk_size == 0))
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	ret = _tls13_init_secret2(prf, psk, psk_size,
				  session->key.proto.tls13.temp_secret);
	if (ret < 0)
		return gnutls_assert_val(ret);

	session->key.proto.tls13.temp_secret_size = prf->output_size;

	ret = generate_early_secrets(session, session->key.binders[0].prf);
	if (ret < 0)
		return gnutls_assert_val(ret);

	return 0;
}

static int
client_send_params(gnutls_session_t session,
		   gnutls_buffer_t extdata,
		   const gnutls_psk_client_credentials_t cred)
{
	int ret, ext_offset = 0;
	uint8_t binder_value[MAX_HASH_SIZE];
	size_t spos;
	gnutls_datum_t username = {NULL, 0};
	gnutls_datum_t user_key = {NULL, 0}, rkey = {NULL, 0};
	gnutls_datum_t client_hello;
	unsigned next_idx;
	const mac_entry_st *prf_res = NULL;
	const mac_entry_st *prf_psk = NULL;
	struct timespec cur_time;
	uint32_t ticket_age, ob_ticket_age;
	int free_username = 0;
	psk_auth_info_t info = NULL;
	unsigned psk_id_len = 0;
	unsigned binders_len, binders_pos;

	if (((session->internals.flags & GNUTLS_NO_TICKETS) ||
	    session->internals.tls13_ticket.ticket.data == NULL) &&
	    (!cred || !_gnutls_have_psk_credentials(cred, session))) {

		return 0;
	}

	binders_len = 0;

	/* placeholder to be filled later */
	spos = extdata->length;
	ret = _gnutls_buffer_append_prefix(extdata, 16, 0);
	if (ret < 0)
		return gnutls_assert_val(ret);

	/* First, let's see if we have a session ticket to send */
	if (!(session->internals.flags & GNUTLS_NO_TICKETS) &&
	    session->internals.tls13_ticket.ticket.data != NULL) {
		/* We found a session ticket */
		if (unlikely(session->internals.tls13_ticket.prf == NULL)) {
			_gnutls13_session_ticket_unset(session);
			ret = gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);
			goto cleanup;
		}

		prf_res = session->internals.tls13_ticket.prf;

		gnutls_gettime(&cur_time);
		if (unlikely(_gnutls_timespec_cmp(&cur_time,
						  &session->internals.
						  tls13_ticket.
						  arrival_time) < 0)) {
			gnutls_assert();
			_gnutls13_session_ticket_unset(session);
			goto ignore_ticket;
		}

		/* Check whether the ticket is stale */
		ticket_age = timespec_sub_ms(&cur_time,
					     &session->internals.tls13_ticket.
					     arrival_time);
		if (ticket_age / 1000 > session->internals.tls13_ticket.lifetime) {
			_gnutls13_session_ticket_unset(session);
			goto ignore_ticket;
		}

		ret = compute_psk_from_ticket(&session->internals.tls13_ticket, &rkey);
		if (ret < 0) {
			_gnutls13_session_ticket_unset(session);
			goto ignore_ticket;
		}

		/* Calculate obfuscated ticket age, in milliseconds, mod 2^32 */
		ob_ticket_age = ticket_age + session->internals.tls13_ticket.age_add;

		if ((ret = _gnutls_buffer_append_data_prefix(extdata, 16,
							     session->internals.tls13_ticket.ticket.data,
							     session->internals.tls13_ticket.ticket.size)) < 0) {
			gnutls_assert();
			goto cleanup;
		}

		/* Now append the obfuscated ticket age */
		if ((ret = _gnutls_buffer_append_prefix(extdata, 32, ob_ticket_age)) < 0) {
			gnutls_assert();
			goto cleanup;
		}

		psk_id_len += 6 + session->internals.tls13_ticket.ticket.size;
		binders_len += 1 + _gnutls_mac_get_algo_len(prf_res);
	}

 ignore_ticket:
	if (cred && _gnutls_have_psk_credentials(cred, session)) {
		gnutls_datum_t tkey;

		if (cred->binder_algo == NULL) {
			gnutls_assert();
			ret = gnutls_assert_val(GNUTLS_E_INSUFFICIENT_CREDENTIALS);
			goto cleanup;
		}

		prf_psk = cred->binder_algo;

		ret = _gnutls_find_psk_key(session, cred, &username, &tkey, &free_username);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		if (username.size == 0 || username.size > UINT16_MAX) {
			ret = gnutls_assert_val(GNUTLS_E_INVALID_PASSWORD);
			goto cleanup;
		}

		if (!free_username) {
			/* we need to copy the key */
			ret = _gnutls_set_datum(&user_key, tkey.data, tkey.size);
			if (ret < 0) {
				gnutls_assert();
				goto cleanup;
			}
		} else {
			user_key.data = tkey.data;
			user_key.size = tkey.size;
		}

		ret = _gnutls_auth_info_init(session, GNUTLS_CRD_PSK, sizeof(psk_auth_info_st), 1);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		info = _gnutls_get_auth_info(session, GNUTLS_CRD_PSK);
		assert(info != NULL);

		_gnutls_copy_psk_username(info, &username);

		if ((ret = _gnutls_buffer_append_data_prefix(extdata, 16,
							     username.data,
							     username.size)) < 0) {
			gnutls_assert();
			goto cleanup;
		}

		/* Now append the obfuscated ticket age */
		if ((ret = _gnutls_buffer_append_prefix(extdata, 32, 0)) < 0) {
			gnutls_assert();
			goto cleanup;
		}

		psk_id_len += 6 + username.size;
		binders_len += 1 + _gnutls_mac_get_algo_len(prf_psk);
	}

	/* if no tickets or identities to be sent */
	if (psk_id_len == 0) {
		/* reset extensions buffer */
		extdata->length = spos;
		return 0;
	}

	_gnutls_write_uint16(psk_id_len, &extdata->data[spos]);

	binders_pos = extdata->length-spos;
	ext_offset = _gnutls_ext_get_extensions_offset(session);

	/* Compute the binders. extdata->data points to the start
	 * of this client hello. */
	assert(extdata->length >= sizeof(mbuffer_st));
	assert(ext_offset >= (ssize_t)sizeof(mbuffer_st));
	ext_offset -= sizeof(mbuffer_st);
	client_hello.data = extdata->data+sizeof(mbuffer_st);
	client_hello.size = extdata->length-sizeof(mbuffer_st);

	next_idx = 0;

	ret = _gnutls_buffer_append_prefix(extdata, 16, binders_len);
	if (ret < 0) {
		gnutls_assert_val(ret);
		goto cleanup;
	}

	if (prf_res && rkey.size > 0) {
		ret = compute_psk_binder(session, prf_res,
					 binders_len, binders_pos,
					 ext_offset, &rkey, &client_hello, 1,
					 binder_value);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		/* Associate the selected pre-shared key with the session */
		gnutls_free(session->key.binders[next_idx].psk.data);
		session->key.binders[next_idx].psk.data = rkey.data;
		session->key.binders[next_idx].psk.size = rkey.size;
		rkey.data = NULL;

		session->key.binders[next_idx].prf = prf_res;
		session->key.binders[next_idx].resumption = 1;
		session->key.binders[next_idx].idx = next_idx;

		_gnutls_handshake_log("EXT[%p]: sent PSK resumption identity (%d)\n", session, next_idx);

		next_idx++;

		/* Add the binder */
		ret = _gnutls_buffer_append_data_prefix(extdata, 8, binder_value, prf_res->output_size);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		session->internals.hsk_flags |= HSK_TLS13_TICKET_SENT;
	}

	if (prf_psk && user_key.size > 0 && info) {
		ret = compute_psk_binder(session, prf_psk,
					 binders_len, binders_pos,
					 ext_offset, &user_key, &client_hello, 0,
					 binder_value);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		/* Associate the selected pre-shared key with the session */
		gnutls_free(session->key.binders[next_idx].psk.data);
		session->key.binders[next_idx].psk.data = user_key.data;
		session->key.binders[next_idx].psk.size = user_key.size;
		user_key.data = NULL;

		session->key.binders[next_idx].prf = prf_psk;
		session->key.binders[next_idx].resumption = 0;
		session->key.binders[next_idx].idx = next_idx;

		_gnutls_handshake_log("EXT[%p]: sent PSK identity '%s' (%d)\n", session, info->username, next_idx);

		next_idx++;

		/* Add the binder */
		ret = _gnutls_buffer_append_data_prefix(extdata, 8, binder_value, prf_psk->output_size);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}
	}

	ret = 0;

cleanup:
	if (free_username)
		_gnutls_free_datum(&username);

	_gnutls_free_temp_key_datum(&user_key);
	_gnutls_free_temp_key_datum(&rkey);

	return ret;
}

static int
server_send_params(gnutls_session_t session, gnutls_buffer_t extdata)
{
	int ret;

	if (!(session->internals.hsk_flags & HSK_PSK_SELECTED))
		return 0;

	ret = _gnutls_buffer_append_prefix(extdata, 16,
					   session->key.binders[0].idx);
	if (ret < 0)
		return gnutls_assert_val(ret);

	return 2;
}

static int server_recv_params(gnutls_session_t session,
			      const unsigned char *data, size_t len,
			      const gnutls_psk_server_credentials_t pskcred)
{
	int ret;
	const mac_entry_st *prf;
	gnutls_datum_t full_client_hello;
	uint8_t binder_value[MAX_HASH_SIZE];
	uint16_t psk_index, i;
	gnutls_datum_t binder_recvd = { NULL, 0 };
	gnutls_datum_t key = {NULL, 0};
	psk_ext_parser_st psk_parser;
	psk_ext_iter_st psk_iter;
	struct psk_st psk;
	psk_auth_info_t info;
	tls13_ticket_st ticket_data;
	/* These values should be set properly when session ticket is accepted. */
	uint32_t ticket_age = UINT32_MAX;
	struct timespec ticket_creation_time = { 0, 0 };
	bool resuming;

	ret = _gnutls13_psk_ext_parser_init(&psk_parser, data, len);
	if (ret < 0) {
		/* No PSKs advertised by client */
		if (ret == GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE)
			return 0;
		return gnutls_assert_val(ret);
	}

	_gnutls13_psk_ext_iter_init(&psk_iter, &psk_parser);
	for (psk_index = 0; ; psk_index++) {
		ret = _gnutls13_psk_ext_iter_next_identity(&psk_iter, &psk);
		if (ret < 0) {
			/* We couldn't find any usable PSK */
			if (ret == GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE)
				return 0;
			return gnutls_assert_val(ret);
		}

		/* This will unpack the session ticket if it is well
		 * formed and has the expected name */
		if (!(session->internals.flags & GNUTLS_NO_TICKETS) &&
		    (ret = _gnutls13_unpack_session_ticket(session, &psk.identity, &ticket_data)) == 0) {
			prf = ticket_data.prf;

			session->internals.resumption_requested = 1;

			/* Check whether ticket is stale or not */
			ticket_age = psk.ob_ticket_age - ticket_data.age_add;
			if (ticket_age / 1000 > ticket_data.lifetime) {
				gnutls_assert();
				tls13_ticket_deinit(&ticket_data);
				continue;
			}

			ret = compute_psk_from_ticket(&ticket_data, &key);
			if (ret < 0) {
				gnutls_assert();
				tls13_ticket_deinit(&ticket_data);
				continue;
			}

			memcpy(&ticket_creation_time,
			       &ticket_data.creation_time,
			       sizeof(struct timespec));

			tls13_ticket_deinit(&ticket_data);

			resuming = 1;
			break;
		} else if (pskcred &&
			   psk.ob_ticket_age == 0 &&
			   psk.identity.size > 0 && psk.identity.size <= MAX_USERNAME_SIZE) {
			prf = pskcred->binder_algo;

			/* this fails only on configuration errors; as such we always
			 * return its error code in that case */
			ret = _gnutls_psk_pwd_find_entry(session, (char *) psk.identity.data, psk.identity.size, &key);
			if (ret < 0)
				return gnutls_assert_val(ret);

			resuming = 0;
			break;
		}
	}

	_gnutls13_psk_ext_iter_init(&psk_iter, &psk_parser);
	for (i = 0; i <= psk_index; i++) {
		ret = _gnutls13_psk_ext_iter_next_binder(&psk_iter, &binder_recvd);
		if (ret < 0) {
			gnutls_assert();
			/* We couldn't extract binder */
			if (ret == GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE)
				ret = GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER;
			goto fail;
		}
	}

	/* Get full ClientHello */
	if (!_gnutls_ext_get_full_client_hello(session, &full_client_hello)) {
		ret = GNUTLS_E_INTERNAL_ERROR;
		gnutls_assert();
		goto fail;
	}

	/* Compute the binder value for this PSK */
	ret = compute_psk_binder(session, prf, psk_parser.binders_len+2, 0, 0,
				 &key, &full_client_hello, resuming,
				 binder_value);
	if (ret < 0) {
		gnutls_assert();
		goto fail;
	}

	if (_gnutls_mac_get_algo_len(prf) != binder_recvd.size ||
	    safe_memcmp(binder_value, binder_recvd.data, binder_recvd.size)) {
		gnutls_assert();
		ret = GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER;
		goto fail;
	}

	if (session->internals.hsk_flags & HSK_PSK_KE_MODE_DHE_PSK)
		_gnutls_handshake_log("EXT[%p]: selected DHE-PSK mode\n", session);
	else {
		reset_cand_groups(session);
		_gnutls_handshake_log("EXT[%p]: selected PSK mode\n", session);
	}

	/* save the username in psk_auth_info to make it available
	 * using gnutls_psk_server_get_username() */
	if (!resuming) {
		assert(psk.identity.size < sizeof(info->username));

		ret = _gnutls_auth_info_init(session, GNUTLS_CRD_PSK, sizeof(psk_auth_info_st), 1);
		if (ret < 0) {
			gnutls_assert();
			goto fail;
		}

		info = _gnutls_get_auth_info(session, GNUTLS_CRD_PSK);
		assert(info != NULL);

		_gnutls_copy_psk_username(info, &psk.identity);
		_gnutls_handshake_log("EXT[%p]: selected PSK identity: %s (%d)\n", session, info->username, psk_index);
	} else {
		if (session->internals.hsk_flags & HSK_EARLY_DATA_ACCEPTED) {
			if (session->internals.anti_replay) {
				ret = _gnutls_anti_replay_check(session->internals.anti_replay,
								ticket_age,
								&ticket_creation_time,
								&binder_recvd);
				if (ret < 0) {
					session->internals.hsk_flags &= ~HSK_EARLY_DATA_ACCEPTED;
					_gnutls_handshake_log("EXT[%p]: replay detected; rejecting early data\n",
						      session);
				}
			} else {
				_gnutls_handshake_log("EXT[%p]: anti-replay is not enabled; rejecting early data\n",
						      session);
				session->internals.hsk_flags &= ~HSK_EARLY_DATA_ACCEPTED;
			}
		}

		session->internals.resumed = RESUME_TRUE;
		_gnutls_handshake_log("EXT[%p]: selected resumption PSK identity (%d)\n", session, psk_index);
	}

	session->internals.hsk_flags |= HSK_PSK_SELECTED;

	/* Reference the selected pre-shared key */
	session->key.binders[0].psk.data = key.data;
	session->key.binders[0].psk.size = key.size;

	session->key.binders[0].idx = psk_index;
	session->key.binders[0].prf = prf;
	session->key.binders[0].resumption = resuming;

	ret = _gnutls_generate_early_secrets_for_psk(session);
	if (ret < 0) {
		gnutls_assert();
		goto fail;
	}

	return 0;

 fail:
	gnutls_free(key.data);
	return ret;
}

/*
 * Return values for this function:
 *  -  0 : Not applicable.
 *  - >0 : Ok. Return size of extension data.
 *  - GNUTLS_E_INT_RET_0 : Size of extension data is zero.
 *  - <0 : There's been an error.
 *
 * In the client, generates the PskIdentity and PskBinderEntry messages.
 *
 *      PskIdentity identities<7..2^16-1>;
 *      PskBinderEntry binders<33..2^16-1>;
 *
 *      struct {
 *          opaque identity<1..2^16-1>;
 *          uint32 obfuscated_ticket_age;
 *      } PskIdentity;
 *
 *      opaque PskBinderEntry<32..255>;
 *
 * The server sends the selected identity, which is a zero-based index
 * of the PSKs offered by the client:
 *
 *      struct {
 *          uint16 selected_identity;
 *      } PreSharedKeyExtension;
 */
static int _gnutls_psk_send_params(gnutls_session_t session,
				   gnutls_buffer_t extdata)
{
	gnutls_psk_client_credentials_t cred = NULL;
	const version_entry_st *vers;

	if (session->security_parameters.entity == GNUTLS_CLIENT) {
		vers = _gnutls_version_max(session);

		if (!vers || !vers->tls13_sem)
			return 0;

		if (session->internals.hsk_flags & HSK_PSK_KE_MODES_SENT) {
			cred = (gnutls_psk_client_credentials_t)
					_gnutls_get_cred(session, GNUTLS_CRD_PSK);
		}

		if ((session->internals.flags & GNUTLS_NO_TICKETS) && !session->internals.priorities->have_psk)
			return 0;

		return client_send_params(session, extdata, cred);
	} else {
		vers = get_version(session);

		if (!vers || !vers->tls13_sem)
			return 0;

		if ((session->internals.flags & GNUTLS_NO_TICKETS) && !session->internals.priorities->have_psk)
			return 0;

		if (session->internals.hsk_flags & HSK_PSK_KE_MODES_RECEIVED)
			return server_send_params(session, extdata);
		else
			return 0;
	}
}

static void swap_binders(gnutls_session_t session)
{
	struct binder_data_st tmp;

	memcpy(&tmp, &session->key.binders[0], sizeof(struct binder_data_st));
	memcpy(&session->key.binders[0], &session->key.binders[1], sizeof(struct binder_data_st));
	memcpy(&session->key.binders[1], &tmp, sizeof(struct binder_data_st));
}

/*
 * Return values for this function:
 *  -  0 : Not applicable.
 *  - >0 : Ok. Return size of extension data.
 *  - <0 : There's been an error.
 */
static int _gnutls_psk_recv_params(gnutls_session_t session,
				   const unsigned char *data, size_t len)
{
	unsigned i;
	gnutls_psk_server_credentials_t pskcred;
	const version_entry_st *vers = get_version(session);
	int ret;

	if (!vers || !vers->tls13_sem)
		return 0;

	if (session->security_parameters.entity == GNUTLS_CLIENT) {
		if (session->internals.hsk_flags & HSK_PSK_KE_MODES_SENT) {
			uint16_t selected_identity = _gnutls_read_uint16(data);

			for (i=0;i<sizeof(session->key.binders)/sizeof(session->key.binders[0]);i++) {
				if (session->key.binders[i].prf != NULL && session->key.binders[i].idx == selected_identity) {
					if (session->key.binders[i].resumption) {
						session->internals.resumed = RESUME_TRUE;
						_gnutls_handshake_log("EXT[%p]: selected PSK-resumption mode\n", session);
					} else {
						_gnutls_handshake_log("EXT[%p]: selected PSK mode\n", session);
					}

					/* different PSK is selected, than the one we calculated early secrets */
					if (i != 0) {
						/* ensure that selected binder is set on (our) index zero */
						swap_binders(session);

						ret = _gnutls_generate_early_secrets_for_psk(session);
						if (ret < 0)
							return gnutls_assert_val(ret);
					}
					session->internals.hsk_flags |= HSK_PSK_SELECTED;
				}
			}

			return 0;
		} else {
			return gnutls_assert_val(GNUTLS_E_RECEIVED_ILLEGAL_EXTENSION);
		}
	} else {
		if (session->internals.hsk_flags & HSK_PSK_KE_MODES_RECEIVED) {
			if (session->internals.hsk_flags & HSK_PSK_KE_MODE_INVALID) {
				/* We received a "psk_ke_modes" extension, but with a value we don't support */
				return 0;
			}

			pskcred = (gnutls_psk_server_credentials_t)
					_gnutls_get_cred(session, GNUTLS_CRD_PSK);

			/* If there are no PSK credentials, this extension is not applicable,
			 * so we return zero. */
			if (pskcred == NULL && (session->internals.flags & GNUTLS_NO_TICKETS))
				return 0;

			return server_recv_params(session, data, len, pskcred);
		} else {
			return gnutls_assert_val(GNUTLS_E_RECEIVED_ILLEGAL_EXTENSION);
		}
	}
}

const hello_ext_entry_st ext_mod_pre_shared_key = {
	.name = "Pre Shared Key",
	.tls_id = PRE_SHARED_KEY_TLS_ID,
	.gid = GNUTLS_EXTENSION_PRE_SHARED_KEY,
	.client_parse_point = GNUTLS_EXT_TLS,
	.server_parse_point = GNUTLS_EXT_TLS,
	.validity = GNUTLS_EXT_FLAG_TLS | GNUTLS_EXT_FLAG_CLIENT_HELLO | GNUTLS_EXT_FLAG_TLS13_SERVER_HELLO,
	.send_func = _gnutls_psk_send_params,
	.recv_func = _gnutls_psk_recv_params
};
