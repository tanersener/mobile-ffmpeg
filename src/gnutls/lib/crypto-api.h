/*
 * Copyright (C) 2000-2016 Free Software Foundation, Inc.
 * Copyright (C) 2016-2017 Red Hat, Inc.
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

#ifndef GNUTLS_LIB_CRYPTO_API_H
#define GNUTLS_LIB_CRYPTO_API_H

#include <gnutls_int.h>

inline static
int _gnutls_aead_cipher_init(gnutls_aead_cipher_hd_t handle,
			     gnutls_cipher_algorithm_t cipher,
			     const gnutls_datum_t * key)
{
	const cipher_entry_st* e;

	e = cipher_to_entry(cipher);
	if (e == NULL || e->type != CIPHER_AEAD)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	return
	    _gnutls_cipher_init(&handle->ctx_enc, e, key,
				NULL, 1);
}

inline static
void _gnutls_aead_cipher_deinit(gnutls_aead_cipher_hd_t handle)
{
	api_aead_cipher_hd_st *h = handle;

	_gnutls_cipher_deinit(&h->ctx_enc);
}

#endif /* GNUTLS_LIB_CRYPTO_API_H */
