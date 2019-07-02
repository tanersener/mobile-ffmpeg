/* gmp-glue.h

   Copyright (C) 2013 Niels Möller
   Copyright (C) 2013 Red Hat

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

#ifndef GNUTLS_LIB_NETTLE_GOST_GMP_GLUE_H
#define GNUTLS_LIB_NETTLE_GOST_GMP_GLUE_H

#include <nettle/bignum.h>

#define mpz_limbs_copy _nettle_mpz_limbs_copy
#define mpn_set_base256_le _nettle_mpn_set_base256_le
#define mpn_get_base256_le _nettle_mpn_get_base256_le
#define gmp_alloc_limbs _nettle_gmp_alloc_limbs
#define gmp_free_limbs _nettle_gmp_free_limbs

void
mpn_set_base256_le (mp_limb_t *rp, mp_size_t rn,
		    const uint8_t *xp, size_t xn);

void
mpn_get_base256_le (uint8_t *rp, size_t rn,
		    const mp_limb_t *xp, mp_size_t xn);

#if !defined(mpn_zero_p) && !defined(__MINI_GMP_H__)
static inline int
mpn_zero_p (const mp_limb_t *xp, mp_size_t n)
{
  while (n > 0)
    if (xp[--n] != 0)
      return 0;
  return 1;
}
#endif

/* Copy limbs, with zero-padding. */
/* FIXME: Reorder arguments, on the theory that the first argument of
   an _mpz_* function should be an mpz_t? Or rename to _mpz_get_limbs,
   with argument order consistent with mpz_get_*. */
void
mpz_limbs_copy (mp_limb_t *xp, mpz_srcptr x, mp_size_t n);

mp_limb_t *
gmp_alloc_limbs (mp_size_t n);

void
gmp_free_limbs (mp_limb_t *p, mp_size_t n);

#endif /* GNUTLS_LIB_NETTLE_GOST_GMP_GLUE_H */
