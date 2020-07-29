/* ecc-dup-th.c

   Copyright (C) 2014, 2019 Niels Möller

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

#include "ecc.h"
#include "ecc-internal.h"

/* Double a point on a twisted Edwards curve, in homogeneous coordinates */
void
ecc_dup_th (const struct ecc_curve *ecc,
	    mp_limb_t *r, const mp_limb_t *p,
	    mp_limb_t *scratch)
{
  /* Formulas (from djb,
     http://www.hyperelliptic.org/EFD/g1p/auto-twisted-projective.html#doubling-dbl-2008-bbjlp):

     B = (X1+Y1)^2
     C = X1^2
     D = Y1^2
     (E = a*C = -C)
     F = E+D
     H = Z1^2
     J = F-2*H
     X3 = (B-C-D)*J
     Y3 = F*(E-D)
     Z3 = F*J         (-C+D)*(-C+D - 2Z1^2)

     In the formula for Y3, we have E - D = -(C+D). To avoid explicit
     negation, negate all of X3, Y3, Z3, and use

     Computation	Operation	Live variables

     B = (X1+Y1)^2	sqr		B
     C = X1^2		sqr		B, C
     D = Y1^2		sqr		B, C, D
     F = -C+D				B, C, D, F
     H = Z1^2		sqr		B, C, D, F, H
     J = 2*H - F			B, C, D, F, J
     X3 = (B-C-D)*J	mul		C, F, J  (Replace C <-- C+D)
     Y3 = F*(C+D)	mul		F, J
     Z3 = F*J		mul

     3M+4S
  */
  /* FIXME: Could reduce scratch need by reusing D storage. */
#define B scratch
#define C (scratch  + ecc->p.size)
#define D (scratch  + 2*ecc->p.size)
#define F (scratch  + 3*ecc->p.size)
#define J (scratch  + 4*ecc->p.size)

  /* B */
  ecc_mod_add (&ecc->p, F, p, p + ecc->p.size);
  ecc_mod_sqr (&ecc->p, B, F);

  /* C */
  ecc_mod_sqr (&ecc->p, C, p);
  /* D */
  ecc_mod_sqr (&ecc->p, D, p + ecc->p.size);
  /* Can use r as scratch, even for in-place operation. */
  ecc_mod_sqr (&ecc->p, r, p + 2*ecc->p.size);
  /* F, */
  ecc_mod_sub (&ecc->p, F, D, C);
  /* B - C - D */
  ecc_mod_add (&ecc->p, C, C, D);
  ecc_mod_sub (&ecc->p, B, B, C);
  /* J */
  ecc_mod_add (&ecc->p, r, r, r);
  ecc_mod_sub (&ecc->p, J, r, F);

  /* x' */
  ecc_mod_mul (&ecc->p, r, B, J);
  /* y' */
  ecc_mod_mul (&ecc->p, r + ecc->p.size, F, C);
  /* z' */
  ecc_mod_mul (&ecc->p, B, F, J);
  mpn_copyi (r + 2*ecc->p.size, B, ecc->p.size);
}
