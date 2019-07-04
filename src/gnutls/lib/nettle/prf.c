/*
 * Copyright (C) 2017 Red Hat, Inc.
 *
 * Author: Nikos Mavrogiannopoulos
 *
 * This file is part of GNUTLS.
 *
 * The GNUTLS library is free software; you can redistribute it and/or
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

#include <gnutls_int.h>
#include "int/tls1-prf.h"
#include <nettle/hmac.h>

/*-
 * _gnutls_prf_raw:
 * @mac: the MAC algorithm to use, set to %GNUTLS_MAC_MD5_SHA1 for the TLS1.0 mac
 * @master_size: length of the @master variable.
 * @master: the master secret used in PRF computation
 * @label_size: length of the @label variable.
 * @label: label used in PRF computation, typically a short string.
 * @seed_size: length of the @seed variable.
 * @seed: optional extra data to seed the PRF with.
 * @outsize: size of pre-allocated output buffer to hold the output.
 * @out: pre-allocated buffer to hold the generated data.
 *
 * Apply the TLS Pseudo-Random-Function (PRF) on the master secret
 * and the provided data.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, or an error code.
 -*/
int
_gnutls_prf_raw(gnutls_mac_algorithm_t mac,
		size_t master_size, const void *master,
		size_t label_size, const char *label,
		size_t seed_size, const uint8_t *seed, size_t outsize, char *out)
{
	int ret;

	switch (mac) {
	case GNUTLS_MAC_MD5_SHA1:
		tls10_prf(master_size, (uint8_t*)master, label_size, label,
			  seed_size, seed, outsize, (uint8_t*)out);
		return 0;
	case GNUTLS_MAC_SHA256:{
		struct hmac_sha256_ctx ctx;
		hmac_sha256_set_key(&ctx, master_size, (uint8_t*)master);

		ret = tls12_prf(&ctx,
			  (nettle_hash_update_func *)
			  hmac_sha256_update,
			  (nettle_hash_digest_func *)
			  hmac_sha256_digest, SHA256_DIGEST_SIZE,
			  label_size, label, seed_size,
			  seed, outsize,
			  (uint8_t*)out);

		if (unlikely(ret != 1))
			return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);
		break;
	}
	case GNUTLS_MAC_SHA384:{
		struct hmac_sha384_ctx ctx;
		hmac_sha384_set_key(&ctx, master_size, master);

		ret = tls12_prf(&ctx,
			  (nettle_hash_update_func *)
			  hmac_sha384_update,
			  (nettle_hash_digest_func *)
			  hmac_sha384_digest, SHA384_DIGEST_SIZE,
			  label_size, label, seed_size,
			  seed, outsize,
			  (uint8_t*)out);

		if (unlikely(ret != 1))
			return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);
		break;
	}
	default:
		gnutls_assert();
		_gnutls_debug_log("unhandled PRF %s\n",
				  gnutls_mac_get_name(mac));
		return GNUTLS_E_INVALID_REQUEST;

	}

	return 0;
}
