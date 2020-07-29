/* ecc-mul-m.c

   Point multiplication using Montgomery curve representation.

   Copyright (C) 2014 Niels Möller

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

#include <assert.h>

#include "ecc.h"
#include "ecc-internal.h"

void
ecc_mul_m (const struct ecc_modulo *m,
	   mp_limb_t a24,
	   unsigned bit_low, unsigned bit_high,
	   mp_limb_t *qx, const uint8_t *n, const mp_limb_t *px,
	   mp_limb_t *scratch)
{
  unsigned i;
  mp_limb_t cy;

  /* FIXME: Could save some more scratch space, e.g., by letting BB
     overlap C, D, and CB overlap A, D. And possibly reusing some of
     x2, z2, x3, z3. */
#define x2 (scratch)
#define z2 (scratch + m->size)
#define x3 (scratch + 2*m->size)
#define z3 (scratch + 3*m->size)

#define A  (scratch + 4*m->size)
#define B  (scratch + 5*m->size)
#define C  (scratch + 6*m->size)
#define D  (scratch + 7*m->size)
#define AA  (scratch + 8*m->size)
#define BB  (scratch +9*m->size)
#define E  (scratch + 9*m->size) /* Overlap BB */
#define DA  (scratch + 8*m->size) /* Overlap AA */
#define CB  (scratch + 9*m->size) /* Overlap BB */

  /* Initialize, x2 = px, z2 = 1 */
  mpn_copyi (x2, px, m->size);
  z2[0] = 1;
  mpn_zero (z2+1, m->size - 1);

  /* Get x3, z3 from doubling. Since most significant bit is forced to 1. */
  ecc_mod_add (m, A, x2, z2);
  ecc_mod_sub (m, B, x2, z2);
  ecc_mod_sqr (m, AA, A);
  ecc_mod_sqr (m, BB, B);
  ecc_mod_mul (m, x3, AA, BB);
  ecc_mod_sub (m, E, AA, BB);
  ecc_mod_addmul_1 (m, AA, E, a24);
  ecc_mod_mul (m, z3, E, AA);

  for (i = bit_high; i >= bit_low; i--)
    {
      int bit = (n[i/8] >> (i & 7)) & 1;

      cnd_swap (bit, x2, x3, 2*m->size);

      /* Formulas from RFC 7748. We compute new coordinates in
	 memory-address order, since mul and sqr clobbers higher
	 limbs. */
      ecc_mod_add (m, A, x2, z2);
      ecc_mod_sub (m, B, x2, z2);
      ecc_mod_sqr (m, AA, A);
      ecc_mod_sqr (m, BB, B);
      ecc_mod_mul (m, x2, AA, BB); /* Last use of BB */
      ecc_mod_sub (m, E, AA, BB);
      ecc_mod_addmul_1 (m, AA, E, a24);
      ecc_mod_add (m, C, x3, z3);
      ecc_mod_sub (m, D, x3, z3);
      ecc_mod_mul (m, z2, E, AA); /* Last use of E and AA */
      ecc_mod_mul (m, DA, D, A);  /* Last use of D, A. FIXME: could
				     let CB overlap. */
      ecc_mod_mul (m, CB, C, B);

      ecc_mod_add (m, C, DA, CB);
      ecc_mod_sqr (m, x3, C);
      ecc_mod_sub (m, C, DA, CB);
      ecc_mod_sqr (m, DA, C);
      ecc_mod_mul (m, z3, DA, px);

      /* FIXME: Could be combined with the loop's initial cnd_swap. */
      cnd_swap (bit, x2, x3, 2*m->size);
    }
  /* Do the low zero bits, just duplicating x2 */
  for (i = 0; i < bit_low; i++)
    {
      ecc_mod_add (m, A, x2, z2);
      ecc_mod_sub (m, B, x2, z2);
      ecc_mod_sqr (m, AA, A);
      ecc_mod_sqr (m, BB, B);
      ecc_mod_mul (m, x2, AA, BB);
      ecc_mod_sub (m, E, AA, BB);
      ecc_mod_addmul_1 (m, AA, E, a24);
      ecc_mod_mul (m, z2, E, AA);
    }
  assert (m->invert_itch <= 7 * m->size);
  m->invert (m, x3, z2, z3 + m->size);
  ecc_mod_mul (m, z3, x2, x3);
  cy = mpn_sub_n (qx, z3, m->m, m->size);
  cnd_copy (cy, qx, z3, m->size);
}
