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
#include "hello_ext.h"
#include "handshake.h"
#include "tls13/hello_retry.h"
#include "auth/cert.h"
#include "mbuffers.h"
#include "state.h"

int _gnutls13_send_hello_retry_request(gnutls_session_t session, unsigned again)
{
	int ret;
	mbuffer_st *bufel = NULL;
	gnutls_buffer_st buf;
	const version_entry_st *ver;
	const uint8_t vbuf[2] = {0x03, 0x03};

	if (again == 0) {
		ver = get_version(session);
		if (unlikely(ver == NULL || session->security_parameters.cs == NULL))
			return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

		ret = _gnutls_buffer_init_handshake_mbuffer(&buf);
		if (ret < 0)
			return gnutls_assert_val(ret);

		ret = _gnutls_buffer_append_data(&buf, vbuf, 2);
		if (ret < 0)
			return gnutls_assert_val(ret);

		ret = _gnutls_buffer_append_data(&buf,
						 HRR_RANDOM,
						 GNUTLS_RANDOM_SIZE);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		ret = _gnutls_buffer_append_data_prefix(&buf, 8,
							session->security_parameters.session_id,
							session->security_parameters.session_id_size);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		ret = _gnutls_buffer_append_data(&buf, session->security_parameters.cs->id, 2);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		/* compression */
		ret = _gnutls_buffer_append_prefix(&buf, 8, 0);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		ret = _gnutls_gen_hello_extensions(session, &buf,
						   GNUTLS_EXT_FLAG_HRR,
						   GNUTLS_EXT_ANY);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		/* reset extensions sent by this session to allow re-sending them */
		session->internals.used_exts = 0;

		reset_binders(session);

		bufel = _gnutls_buffer_to_mbuffer(&buf);
	}

	return _gnutls_send_handshake(session, bufel, GNUTLS_HANDSHAKE_HELLO_RETRY_REQUEST);

 cleanup:
	_gnutls_buffer_clear(&buf);
	return ret;
}

int
_gnutls13_recv_hello_retry_request(gnutls_session_t session,
				   gnutls_buffer_st *buf)
{
	int ret;
	uint8_t tmp[2];
	const gnutls_cipher_suite_entry_st *cs;
	const mac_entry_st *prf;
	gnutls_datum_t session_id;
	uint8_t random[GNUTLS_RANDOM_SIZE];

	/* only under TLS 1.3 */
	if (IS_DTLS(session))
		return gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET);

	if (session->internals.hsk_flags & HSK_HRR_RECEIVED)
		return gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET);

	session->internals.hsk_flags |= HSK_HRR_RECEIVED;

	/* version */
	ret = _gnutls_buffer_pop_data(buf, tmp, 2);
	if (ret < 0)
		return gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET_LENGTH);

	if (unlikely(tmp[0] != 0x03 || tmp[1] != 0x03))
		return gnutls_assert_val(GNUTLS_E_UNSUPPORTED_VERSION_PACKET);

	ret = _gnutls_buffer_pop_data(buf, random, GNUTLS_RANDOM_SIZE);
	if (ret < 0)
		return gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET_LENGTH);

	if (memcmp(random, HRR_RANDOM, GNUTLS_RANDOM_SIZE) != 0) {
		return gnutls_assert_val(GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER);
	}

	ret = _gnutls_buffer_pop_datum_prefix8(buf, &session_id);
	if (ret < 0)
		return gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET_LENGTH);

	/* read ciphersuites */
	ret = _gnutls_buffer_pop_data(buf, tmp, 2);
	if (ret < 0)
		return gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET_LENGTH);

	cs = ciphersuite_to_entry(tmp);
	if (unlikely(cs == NULL))
		return gnutls_assert_val(GNUTLS_E_UNKNOWN_CIPHER_SUITE);

	_gnutls_handshake_log("EXT[%p]: Hello Retry Request with %s\n", session, cs->name);
	memcpy(session->internals.hrr_cs, cs->id, 2);

	prf = mac_to_entry(cs->prf);
	if (unlikely(prf == NULL))
		return gnutls_assert_val(GNUTLS_E_UNKNOWN_CIPHER_SUITE);

	/* compression */
	ret = _gnutls_buffer_pop_data(buf, tmp, 1);
	if (ret < 0)
		return gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET_LENGTH);

	if (unlikely(tmp[0] != 0))
		return gnutls_assert_val(GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER);

	ret = _gnutls13_handshake_hash_buffers_synth(session, prf, 1);
	if (ret < 0)
		return gnutls_assert_val(ret);

	if (buf->length <= 2) {
		/* no extensions present */
		return gnutls_assert_val(GNUTLS_E_UNEXPECTED_EXTENSIONS_LENGTH);
	}

	/* figure version first */
	ret =
	    _gnutls_parse_hello_extensions(session, GNUTLS_EXT_FLAG_HRR,
					   GNUTLS_EXT_VERSION_NEG,
					   buf->data, buf->length);
	if (ret < 0)
		return gnutls_assert_val(ret);

	/* parse the rest of extensions */
	ret = _gnutls_parse_hello_extensions(session, GNUTLS_EXT_FLAG_HRR, GNUTLS_EXT_ANY,
					     buf->data, buf->length);
	if (ret < 0)
		return gnutls_assert_val(ret);

	session->internals.used_exts = 0;

	return 0;
}
