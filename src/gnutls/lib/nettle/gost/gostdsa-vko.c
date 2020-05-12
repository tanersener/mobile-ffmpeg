/* gostdsa-vko.c

   Copyright (C) 2016 Dmitry Eremin-Solenikov

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
   not, see http://www.gnu.org/licenses/.
*/

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include <gnutls_int.h>

#include <stdlib.h>

#include "ecc-internal.h"
#include "gostdsa2.h"

int
gostdsa_vko(const struct ecc_scalar *key,
	    const struct ecc_point *pub,
	    size_t ukm_length, const uint8_t *ukm,
	    size_t out_length, uint8_t *out)
{
  const struct ecc_curve *ecc = key->ecc;
  unsigned bsize = (ecc_bit_size(ecc) + 7) / 8;
  mp_size_t size = ecc->p.size;
  mp_size_t itch = 4*size + ecc->mul_itch;
  mp_limb_t *scratch;

  if (itch < 5*size + ecc->h_to_a_itch)
      itch = 5*size + ecc->h_to_a_itch;

  if (pub->ecc != ecc)
      return 0;

  if (out_length < 2 * bsize) {
      return 0;
  }

  scratch = gmp_alloc_limbs (itch);

  mpn_set_base256_le (scratch, size, ukm, ukm_length);
  if (mpn_zero_p (scratch, size))
    mpn_add_1 (scratch, scratch, size, 1);
  ecc_modq_mul (ecc, scratch + 3*size, key->p, scratch);
  ecc->mul (ecc, scratch, scratch + 3*size, pub->p, scratch + 4*size);
  ecc->h_to_a (ecc, 0, scratch + 3*size, scratch, scratch + 5*size);
  mpn_get_base256_le (out, bsize, scratch + 3*size, size);
  mpn_get_base256_le (out+bsize, bsize, scratch + 4*size, size);
  gmp_free_limbs (scratch, itch);

  return 2 * bsize;
}
