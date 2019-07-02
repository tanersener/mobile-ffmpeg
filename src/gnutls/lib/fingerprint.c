/*
 * Copyright (C) 2001-2012 Free Software Foundation, Inc.
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
#include <auth/srp_kx.h>
#include <auth/anon.h>
#include <auth/cert.h>
#include <auth/psk.h>
#include "errors.h"
#include <auth.h>
#include <state.h>
#include <datum.h>
#include <algorithms.h>

/**
 * gnutls_fingerprint:
 * @algo: is a digest algorithm
 * @data: is the data
 * @result: is the place where the result will be copied (may be null).
 * @result_size: should hold the size of the result. The actual size
 * of the returned result will also be copied there.
 *
 * This function will calculate a fingerprint (actually a hash), of
 * the given data.  The result is not printable data.  You should
 * convert it to hex, or to something else printable.
 *
 * This is the usual way to calculate a fingerprint of an X.509 DER
 * encoded certificate.  Note however that the fingerprint of an
 * OpenPGP certificate is not just a hash and cannot be calculated with this
 * function.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise
 *   an error code is returned.
 **/
int
gnutls_fingerprint(gnutls_digest_algorithm_t algo,
		   const gnutls_datum_t * data, void *result,
		   size_t * result_size)
{
	int ret;
	int hash_len = _gnutls_hash_get_algo_len(hash_to_entry(algo));

	if (hash_len < 0 || (unsigned) hash_len > *result_size
	    || result == NULL) {
		*result_size = hash_len;
		return GNUTLS_E_SHORT_MEMORY_BUFFER;
	}
	*result_size = hash_len;

	if (result) {
		ret =
		    _gnutls_hash_fast(algo, data->data, data->size,
				      result);
		if (ret < 0)
			return gnutls_assert_val(ret);
	}

	return 0;
}


