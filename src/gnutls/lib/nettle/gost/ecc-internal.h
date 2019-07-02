/* ecc-internal.h

   Copyright (C) 2013, 2014 Niels Möller

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

#ifndef GNUTLS_LIB_NETTLE_GOST_ECC_INTERNAL_H
#define GNUTLS_LIB_NETTLE_GOST_ECC_INTERNAL_H

#include <nettle/nettle-types.h>
#include <nettle/bignum.h>
#include <nettle/ecc-curve.h>
#include "gmp-glue.h"

#define ecc_mod_add _nettle_ecc_mod_add
#define ecc_mod_mul _nettle_ecc_mod_mul
#define ecc_mod_random _nettle_ecc_mod_random
#define ecc_mod _nettle_ecc_mod
#define ecc_mod_inv _nettle_ecc_mod_inv
#define gost_hash _gnutls_gost_hash
#define ecc_j_to_a _nettle_ecc_j_to_a
#define ecc_add_jjj _nettle_ecc_add_jjj
#define ecc_mul_g _nettle_ecc_mul_g
#define ecc_mul_a _nettle_ecc_mul_a
#define cnd_copy _nettle_cnd_copy

/* Window size for ecc_mul_a. Using 4 bits seems like a good choice,
   for both Intel x86_64 and ARM Cortex A9. For the larger curves, of
   384 and 521 bits, we could improve speed by a few percent if we go
   up to 5 bits, but I don't think that's worth doubling the
   storage. */
#define ECC_MUL_A_WBITS 4

struct ecc_modulo;

/* Reduces from 2*ecc->size to ecc->size. */
/* Required to return a result < 2q. This property is inherited by
   mod_mul and mod_sqr. */
typedef void ecc_mod_func (const struct ecc_modulo *m, mp_limb_t *rp);

typedef void ecc_mod_inv_func (const struct ecc_modulo *m,
			       mp_limb_t *vp, const mp_limb_t *ap,
			       mp_limb_t *scratch);

/* Computes the square root of (u/v) (mod p) */
typedef int ecc_mod_sqrt_func (const struct ecc_modulo *m,
			       mp_limb_t *rp,
			       const mp_limb_t *up, const mp_limb_t *vp,
			       mp_limb_t *scratch);

typedef void ecc_add_func (const struct ecc_curve *ecc,
			   mp_limb_t *r,
			   const mp_limb_t *p, const mp_limb_t *q,
			   mp_limb_t *scratch);

typedef void ecc_mul_g_func (const struct ecc_curve *ecc, mp_limb_t *r,
			     const mp_limb_t *np, mp_limb_t *scratch);

typedef void ecc_mul_func (const struct ecc_curve *ecc,
			   mp_limb_t *r,
			   const mp_limb_t *np, const mp_limb_t *p,
			   mp_limb_t *scratch);

typedef void ecc_h_to_a_func (const struct ecc_curve *ecc,
			      int flags,
			      mp_limb_t *r, const mp_limb_t *p,
			      mp_limb_t *scratch);

struct ecc_modulo
{
  unsigned short bit_size;
  unsigned short size;
  unsigned short B_size;
  unsigned short redc_size;
  unsigned short invert_itch;
  unsigned short sqrt_itch;

  const mp_limb_t *m;
  /* B^size mod m. Expected to have at least 32 leading zeros
     (equality for secp_256r1). */
  const mp_limb_t *B;
  /* 2^{bit_size} - p, same value as above, but shifted. */
  const mp_limb_t *B_shifted;
  /* m +/- 1, for redc, excluding redc_size low limbs. */
  const mp_limb_t *redc_mpm1;
  /* (m+1)/2 */
  const mp_limb_t *mp1h;

  ecc_mod_func *mod;
  ecc_mod_func *reduce;
  ecc_mod_inv_func *invert;
  ecc_mod_sqrt_func *sqrt;
};

/* Represents an elliptic curve of the form

     y^2 = x^3 - 3x + b (mod p)
*/
struct ecc_curve
{
  /* The prime p. */
  struct ecc_modulo p;
  /* Group order. FIXME: Currently, many functions rely on q.size ==
     p.size. This has to change for radix-51 implementation of
     curve25519 mod p arithmetic. */
  struct ecc_modulo q;

  unsigned short use_redc;
  unsigned short pippenger_k;
  unsigned short pippenger_c;

  unsigned short add_hhh_itch;
  unsigned short mul_itch;
  unsigned short mul_g_itch;
  unsigned short h_to_a_itch;

  ecc_add_func *add_hhh;
  ecc_mul_func *mul;
  ecc_mul_g_func *mul_g;
  ecc_h_to_a_func *h_to_a;

  /* Curve constant */
  const mp_limb_t *b;
  /* Generator, x coordinate followed by y (affine coordinates).
     Currently used only by the test suite. */
  const mp_limb_t *g;
  /* If non-NULL, the constant needed for transformation to the
     equivalent Edwards curve. */
  const mp_limb_t *edwards_root;

  /* For redc, same as B mod p, otherwise 1. */
  const mp_limb_t *unit;

  /* Tables for multiplying by the generator, size determined by k and
     c. The first 2^c entries are defined by

       T[  j_0 +   j_1 2 +     ... + j_{c-1} 2^{c-1} ]
         = j_0 g + j_1 2^k g + ... + j_{c-1} 2^{k(c-1)} g

     The following entries differ by powers of 2^{kc},

       T[i] = 2^{kc} T[i-2^c]
  */  
  const mp_limb_t *pippenger_table;
};

/* In-place reduction. */
ecc_mod_func ecc_mod;

ecc_mod_inv_func ecc_mod_inv;

void
ecc_mod_add (const struct ecc_modulo *m, mp_limb_t *rp,
	     const mp_limb_t *ap, const mp_limb_t *bp);

/* NOTE: mul and sqr needs 2*ecc->size limbs at rp */
void
ecc_mod_mul (const struct ecc_modulo *m, mp_limb_t *rp,
	     const mp_limb_t *ap, const mp_limb_t *bp);

#define ecc_modq_add(ecc, r, a, b) \
  ecc_mod_add (&(ecc)->q, (r), (a), (b))
#define ecc_modq_mul(ecc, r, a, b) \
  ecc_mod_mul (&(ecc)->q, (r), (a), (b))

/* mod q operations. */
void
ecc_mod_random (const struct ecc_modulo *m, mp_limb_t *xp,
		void *ctx, nettle_random_func *random, mp_limb_t *scratch);

void
gost_hash (const struct ecc_modulo *m,
	  mp_limb_t *hp,
	  size_t length, const uint8_t *digest);

/* Converts a point P in jacobian coordinates into a point R in affine
   coordinates. If op == 1, produce x coordinate only. If op == 2,
   produce the x coordinate only, and in also it modulo q. FIXME: For
   the public interface, have separate for the three cases, and use
   this flag argument only for the internal ecc->h_to_a function. */
void
ecc_j_to_a (const struct ecc_curve *ecc,
	    int op,
	    mp_limb_t *r, const mp_limb_t *p,
	    mp_limb_t *scratch);

/* Point addition with Jacobian input and output. */
void
ecc_add_jjj (const struct ecc_curve *ecc,
	     mp_limb_t *r, const mp_limb_t *p, const mp_limb_t *q,
	     mp_limb_t *scratch);

/* Computes N * the group generator. N is an array of ecc_size()
   limbs. It must be in the range 0 < N < group order, then R != 0,
   and the algorithm can work without any intermediate values getting
   to zero. */ 
void
ecc_mul_g (const struct ecc_curve *ecc, mp_limb_t *r,
	   const mp_limb_t *np, mp_limb_t *scratch);

/* Computes N * P. The scalar N is the same as for ecc_mul_g. P is a
   non-zero point on the curve, in affine coordinates. Output R is a
   non-zero point, in Jacobian coordinates. */
void
ecc_mul_a (const struct ecc_curve *ecc,
	   mp_limb_t *r,
	   const mp_limb_t *np, const mp_limb_t *p,
	   mp_limb_t *scratch);

void
cnd_copy (int cnd, mp_limb_t *rp, const mp_limb_t *ap, mp_size_t n);

/* Current scratch needs: */
#define ECC_MOD_INV_ITCH(size) (2*(size))
#define ECC_J_TO_A_ITCH(size) (5*(size))
#define ECC_ADD_JJJ_ITCH(size) (8*(size))
#define ECC_MUL_G_ITCH(size) (9*(size))
#if ECC_MUL_A_WBITS == 0
#define ECC_MUL_A_ITCH(size) (12*(size))
#else
#define ECC_MUL_A_ITCH(size) \
  (((3 << ECC_MUL_A_WBITS) + 11) * (size))
#endif
#define ECC_GOSTDSA_SIGN_ITCH(size) (12*(size))

#endif /* GNUTLS_LIB_NETTLE_GOST_ECC_INTERNAL_H */
