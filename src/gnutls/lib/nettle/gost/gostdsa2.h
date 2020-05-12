/* gostdsa.h

   Copyright (C) 2015 Dmity Eremin-Solenikov
   Copyright (C) 2013 Niels Möller

   This file is part of GNU Nettle.

   GNU Nettle is free software: you can redistribute it and/or
   modify it under the terms of either:

     * the GNU Lesser General Public License as published by the Free
       Software Foundation; either version 3 of the License, or (at your
       option) any later version.

   or

     * the GNU General Public License as published by the Free
       Software Foundation; either version 2 of the License, or (at your
       option) any later version.

   or both in parallel, as here.

   GNU Nettle is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received copies of the GNU General Public License and
   the GNU Lesser General Public License along with this program.  If
   not, see https://www.gnu.org/licenses/.
*/

/* Development of Nettle's ECC support was funded by the .SE Internet Fund. */

#ifndef GNUTLS_LIB_NETTLE_GOST_GOSTDSA2_H
#define GNUTLS_LIB_NETTLE_GOST_GOSTDSA2_H

#include <nettle/ecc.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Name mangling */
#define gostdsa_unmask_key _gnutls_gostdsa_unmask_key
#define gostdsa_vko _gnutls_gostdsa_vko

int
gostdsa_unmask_key (const struct ecc_curve *ecc,
		    mpz_t key);

int
gostdsa_vko(const struct ecc_scalar *key,
	    const struct ecc_point *pub,
	    size_t ukm_length, const uint8_t *ukm,
	    size_t out_length, uint8_t *out);

#ifdef __cplusplus
}
#endif

#endif /* GNUTLS_LIB_NETTLE_GOST_GOSTDSA2_H */
