/* bignum.h

   Bignum operations that are missing from gmp.

   Copyright (C) 2001 Niels Möller

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
 
#ifndef GNUTLS_LIB_NETTLE_GOST_BIGNUM_LE_H
#define GNUTLS_LIB_NETTLE_GOST_BIGNUM_LE_H

#include <nettle/nettle-meta.h>

#include <nettle/nettle-types.h>

#include <nettle/bignum.h>

#ifdef __cplusplus
extern "C" {
#endif

#define nettle_mpz_sizeinbase_256_u_le nettle_mpz_sizeinbase_256_u

#define nettle_mpz_get_str_256_u_le _gnutls_mpz_get_str_256_u_le
#define nettle_mpz_set_str_256_u_le _gnutls_mpz_set_str_256_u_le
#define nettle_mpz_init_set_str_256_u_le _gnutls_mpz_init_set_str_256_u_le

/* Writes an integer as length octets, using big endian byte order,
 * and unsigned number format. */
void
nettle_mpz_get_str_256_u_le(size_t length, uint8_t *s, const mpz_t x);


void
nettle_mpz_set_str_256_u_le(mpz_t x,
			    size_t length, const uint8_t *s);

void
nettle_mpz_init_set_str_256_u_le(mpz_t x,
				 size_t length, const uint8_t *s);

#ifdef __cplusplus
}
#endif

#endif /* GNUTLS_LIB_NETTLE_GOST_BIGNUM_LE_H */
