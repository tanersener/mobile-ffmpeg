/*
 * Copyright (C) 2001-2015 Free Software Foundation, Inc.
 * Copyright (C) 2015 Nikos Mavrogiannopoulos
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
#include <extras/randomart.h>

/**
 * gnutls_random_art:
 * @type: The type of the random art (for now only %GNUTLS_RANDOM_ART_OPENSSH is supported)
 * @key_type: The type of the key (RSA, DSA etc.)
 * @key_size: The size of the key in bits
 * @fpr: The fingerprint of the key
 * @fpr_size: The size of the fingerprint
 * @art: The returned random art
 *
 * This function will convert a given fingerprint to an "artistic"
 * image. The returned image is allocated using gnutls_malloc(), is
 * null-terminated but art->size will not account the terminating null.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise
 *   an error code is returned.
 *
 **/
int gnutls_random_art(gnutls_random_art_t type,
		      const char *key_type, unsigned int key_size,
		      void *fpr, size_t fpr_size, gnutls_datum_t * art)
{
	if (type != GNUTLS_RANDOM_ART_OPENSSH)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	art->data =
	    (void *) _gnutls_key_fingerprint_randomart(fpr, fpr_size,
						       key_type, key_size,
						       NULL);
	if (art->data == NULL)
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

	art->size = strlen((char *) art->data);

	return 0;
}

