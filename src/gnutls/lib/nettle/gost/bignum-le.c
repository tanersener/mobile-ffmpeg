/* bignum.c

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

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include <gnutls_int.h>

#include <string.h>

#include <nettle/bignum.h>
#include "bignum-le.h"

void
nettle_mpz_get_str_256_u_le(size_t length, uint8_t *s, const mpz_t x)
{
  if (!length)
    {
      /* x must be zero */
      assert(!mpz_sgn(x));
      return;
    }

  size_t count;

  assert(nettle_mpz_sizeinbase_256_u(x) <= length);
  mpz_export(s, &count, -1, 1, 0, 0, x);
  memset(s + count, 0, length - count);
}

#define nettle_mpz_from_octets_le(x, length, s) \
   mpz_import((x), (length), -1, 1, 0, 0, (s))

void
nettle_mpz_set_str_256_u_le(mpz_t x,
			    size_t length, const uint8_t *s)
{
  nettle_mpz_from_octets_le(x, length, s);
}

void
nettle_mpz_init_set_str_256_u_le(mpz_t x,
				 size_t length, const uint8_t *s)
{
  mpz_init(x);
  nettle_mpz_from_octets_le(x, length, s);
}
