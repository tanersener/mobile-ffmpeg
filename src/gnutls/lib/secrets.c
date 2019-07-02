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

/* TLS 1.3 secret key derivation handling.
 */

#include <config.h>
#include "gnutls_int.h"
#include <nettle/hkdf.h>
#include <nettle/hmac.h>
#include "secrets.h"

/* HKDF-Extract(0,0) or HKDF-Extract(0, PSK) */
int _tls13_init_secret(gnutls_session_t session, const uint8_t *psk, size_t psk_size)
{
	session->key.proto.tls13.temp_secret_size = session->security_parameters.prf->output_size;

	return _tls13_init_secret2(session->security_parameters.prf,
				   psk, psk_size,
				   session->key.proto.tls13.temp_secret);
}

int _tls13_init_secret2(const mac_entry_st *prf,
			const uint8_t *psk, size_t psk_size,
			void *out)
{
	char buf[128];

	if (unlikely(prf == NULL))
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

	/* when no PSK, use the zero-value */
	if (psk == NULL) {
		psk_size = prf->output_size;
		if (unlikely(psk_size >= sizeof(buf)))
			return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

		memset(buf, 0, psk_size);
		psk = (uint8_t*) buf;
	}

	return gnutls_hmac_fast(prf->id,
				"", 0,
				psk, psk_size,
				out);
}

/* HKDF-Extract(Prev-Secret, key) */
int _tls13_update_secret(gnutls_session_t session, const uint8_t *key, size_t key_size)
{
	return gnutls_hmac_fast(session->security_parameters.prf->id,
				session->key.proto.tls13.temp_secret, session->key.proto.tls13.temp_secret_size,
				key, key_size,
				session->key.proto.tls13.temp_secret);
}

/* Derive-Secret(Secret, Label, Messages) */
int _tls13_derive_secret2(const mac_entry_st *prf,
			 const char *label, unsigned label_size,
			 const uint8_t *tbh, size_t tbh_size,
			 const uint8_t secret[MAX_HASH_SIZE],
			 void *out)
{
	uint8_t digest[MAX_HASH_SIZE];
	int ret;
	unsigned digest_size;

	if (unlikely(prf == NULL))
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);
	if (unlikely(label_size >= sizeof(digest)))
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	digest_size = prf->output_size;
	ret = gnutls_hash_fast((gnutls_digest_algorithm_t) prf->id,
				tbh, tbh_size, digest);
	if (ret < 0)
		return gnutls_assert_val(ret);

	return _tls13_expand_secret2(prf, label, label_size, digest, digest_size, secret, digest_size, out);
}

/* Derive-Secret(Secret, Label, Messages) */
int _tls13_derive_secret(gnutls_session_t session,
			 const char *label, unsigned label_size,
			 const uint8_t *tbh, size_t tbh_size,
			 const uint8_t secret[MAX_HASH_SIZE],
			 void *out)
{
	if (unlikely(session->security_parameters.prf == NULL))
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

	return _tls13_derive_secret2(session->security_parameters.prf, label, label_size, tbh, tbh_size,
				         secret,
				         out);
}

/* HKDF-Expand-Label(Secret, Label, HashValue, Length) */
int _tls13_expand_secret2(const mac_entry_st *prf,
			 const char *label, unsigned label_size,
			 const uint8_t *msg, size_t msg_size,
			 const uint8_t secret[MAX_HASH_SIZE],
			 unsigned out_size,
			 void *out)
{
	uint8_t tmp[256] = "tls13 ";
	gnutls_buffer_st str;
	int ret;

	if (unlikely(label_size >= sizeof(tmp)-6))
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	_gnutls_buffer_init(&str);

	ret = _gnutls_buffer_append_prefix(&str, 16, out_size);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	memcpy(&tmp[6], label, label_size);
	ret = _gnutls_buffer_append_data_prefix(&str, 8, tmp, label_size+6);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = _gnutls_buffer_append_data_prefix(&str, 8, msg, msg_size);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	switch (prf->id) {
	case GNUTLS_MAC_SHA256:{
		struct hmac_sha256_ctx ctx;

		hmac_sha256_set_key(&ctx, SHA256_DIGEST_SIZE, secret);
		hkdf_expand(&ctx, (nettle_hash_update_func*)hmac_sha256_update,
			(nettle_hash_digest_func*)hmac_sha256_digest, SHA256_DIGEST_SIZE,
			str.length, str.data, out_size, out);
		break;
	}
	case GNUTLS_MAC_SHA384:{
		struct hmac_sha384_ctx ctx;

		hmac_sha384_set_key(&ctx, SHA384_DIGEST_SIZE, secret);
		hkdf_expand(&ctx, (nettle_hash_update_func*)hmac_sha384_update,
			(nettle_hash_digest_func*)hmac_sha384_digest, SHA384_DIGEST_SIZE,
			str.length, str.data, out_size, out);
		break;
	}
	default:
		gnutls_assert();
		ret = GNUTLS_E_INTERNAL_ERROR;
		goto cleanup;
	}

#if 0
        _gnutls_hard_log("INT: hkdf label: %d,%s\n",
                         out_size,
                         _gnutls_bin2hex(str.data, str.length,
                                         (char*)tmp, sizeof(tmp), NULL));
        _gnutls_hard_log("INT: secret expanded for '%.*s': %d,%s\n", 
                         (int)label_size, label, out_size,
                         _gnutls_bin2hex(out, out_size,
                                         (char*)tmp, sizeof(tmp), NULL));
#endif

	ret = 0;
 cleanup:
	_gnutls_buffer_clear(&str);
	return ret;
}

int _tls13_expand_secret(gnutls_session_t session,
		const char *label, unsigned label_size,
		const uint8_t *msg, size_t msg_size,
		const uint8_t secret[MAX_CIPHER_KEY_SIZE],
		unsigned out_size,
		void *out)
{
	if (unlikely(session->security_parameters.prf == NULL))
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

	return _tls13_expand_secret2(session->security_parameters.prf,
			label, label_size,
			msg, msg_size, secret,
			out_size, out);
}

