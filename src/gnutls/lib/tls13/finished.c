/*
 * Copyright (C) 2017 Red Hat, Inc.
 *
 * Author: Nikos Mavrogiannopoulos
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
#include "handshake.h"
#include "tls13/finished.h"
#include "mem.h"
#include "mbuffers.h"
#include "secrets.h"

int _gnutls13_compute_finished(const mac_entry_st *prf,
		const uint8_t *base_key,
		gnutls_buffer_st *handshake_hash_buffer,
		void *out)
{
	int ret;
	uint8_t fkey[MAX_HASH_SIZE];
	uint8_t ts_hash[MAX_HASH_SIZE];

	ret = _tls13_expand_secret2(prf,
			"finished", 8,
			NULL, 0,
			base_key,
			prf->output_size, fkey);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = gnutls_hash_fast(prf->id,
			       handshake_hash_buffer->data,
			       handshake_hash_buffer->length,
			       ts_hash);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = gnutls_hmac_fast(prf->id,
			       fkey, prf->output_size,
			       ts_hash, prf->output_size,
			       out);
	if (ret < 0)
		return gnutls_assert_val(ret);

	return 0;
}

int _gnutls13_recv_finished(gnutls_session_t session)
{
	int ret;
	gnutls_buffer_st buf;
	uint8_t verifier[MAX_HASH_SIZE];
	const uint8_t *base_key;
	unsigned hash_size;

	if (unlikely(session->security_parameters.prf == NULL))
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

	hash_size = session->security_parameters.prf->output_size;

	if (!session->internals.initial_negotiation_completed) {
		if (session->security_parameters.entity == GNUTLS_CLIENT)
			base_key = session->key.proto.tls13.hs_skey;
		else
			base_key = session->key.proto.tls13.hs_ckey;
	} else {
		if (session->security_parameters.entity == GNUTLS_CLIENT)
			base_key = session->key.proto.tls13.ap_skey;
		else
			base_key = session->key.proto.tls13.ap_ckey;
	}

	ret = _gnutls13_compute_finished(session->security_parameters.prf,
			base_key,
			&session->internals.handshake_hash_buffer,
			verifier);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = _gnutls_recv_handshake(session, GNUTLS_HANDSHAKE_FINISHED, 0, &buf);
	if (ret < 0)
		return gnutls_assert_val(ret);

	_gnutls_handshake_log("HSK[%p]: parsing finished\n", session);

	if (buf.length != hash_size) {
		gnutls_assert();
		ret = GNUTLS_E_UNEXPECTED_PACKET_LENGTH;
		goto cleanup;
	}


#if defined(FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION)
# warning This is unsafe for production builds
#else
	if (safe_memcmp(verifier, buf.data, buf.length) != 0) {
		gnutls_assert();
		ret = GNUTLS_E_ERROR_IN_FINISHED_PACKET;
		goto cleanup;
	}
#endif

	ret = 0;
cleanup:
	
	_gnutls_buffer_clear(&buf);
	return ret;
}

int _gnutls13_send_finished(gnutls_session_t session, unsigned again)
{
	int ret;
	uint8_t verifier[MAX_HASH_SIZE];
	mbuffer_st *bufel = NULL;
	const uint8_t *base_key;
	unsigned hash_size;

	if (again == 0) {
		if (unlikely(session->security_parameters.prf == NULL))
			return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

		hash_size = session->security_parameters.prf->output_size;

		if (!session->internals.initial_negotiation_completed) {
			if (session->security_parameters.entity == GNUTLS_CLIENT)
				base_key = session->key.proto.tls13.hs_ckey;
			else
				base_key = session->key.proto.tls13.hs_skey;
		} else {
			if (session->security_parameters.entity == GNUTLS_CLIENT)
				base_key = session->key.proto.tls13.ap_ckey;
			else
				base_key = session->key.proto.tls13.ap_skey;
		}

		ret = _gnutls13_compute_finished(session->security_parameters.prf,
				base_key,
				&session->internals.handshake_hash_buffer,
				verifier);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		_gnutls_handshake_log("HSK[%p]: sending finished\n", session);

		bufel = _gnutls_handshake_alloc(session, hash_size);
		if (bufel == NULL)
			return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

		_mbuffer_set_udata_size(bufel, 0);
		ret = _mbuffer_append_data(bufel, verifier, hash_size);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}
	}

	return _gnutls_send_handshake(session, bufel, GNUTLS_HANDSHAKE_FINISHED);

cleanup:
	_mbuffer_xfree(&bufel);
	return ret;
}
