/* ecc-gost256cpa.c

   Compile time constant (but machine dependent) tables.

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

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include <gnutls_int.h>

#include <nettle/ecc.h>
#include "ecc-internal.h"
#include "ecc-gost-curve.h"

#define USE_REDC (ECC_REDC_SIZE != 0)

#if GMP_NUMB_BITS == 32
#include "ecc-gost256cpa-32.h"
#elif GMP_NUMB_BITS == 64
#include "ecc-gost256cpa-64.h"
#else
#error unsupported configuration
#endif

#if ECC_REDC_SIZE > 0
#  define ecc_256_redc ecc_pp1_redc
#elif ECC_REDC_SIZE == 0
#  define ecc_256_redc NULL
#else
# error Configuration error
#endif

#define ecc_256_modp ecc_mod
#define ecc_256_modq ecc_mod

static const struct ecc_curve _gnutls_gost_256cpa =
{
  {
    256,
    ECC_LIMB_SIZE,
    ECC_BMODP_SIZE,
    ECC_REDC_SIZE,
    ECC_MOD_INV_ITCH (ECC_LIMB_SIZE),
    0,

    ecc_p,
    ecc_Bmodp,
    ecc_Bmodp_shifted,
    ecc_redc_ppm1,

    ecc_pp1h,
    ecc_256_modp,
    USE_REDC ? ecc_256_redc : ecc_256_modp,
    ecc_mod_inv,
    NULL,
  },
  {
    256,
    ECC_LIMB_SIZE,
    ECC_BMODQ_SIZE,
    0,
    ECC_MOD_INV_ITCH (ECC_LIMB_SIZE),
    0,

    ecc_q,
    ecc_Bmodq,
    ecc_Bmodq_shifted,
    NULL,
    ecc_qp1h,

    ecc_256_modq,
    ecc_256_modq,
    ecc_mod_inv,
    NULL,
  },

  USE_REDC,
  ECC_PIPPENGER_K,
  ECC_PIPPENGER_C,

  ECC_ADD_JJJ_ITCH (ECC_LIMB_SIZE),
  ECC_MUL_A_ITCH (ECC_LIMB_SIZE),
  ECC_MUL_G_ITCH (ECC_LIMB_SIZE),
  ECC_J_TO_A_ITCH (ECC_LIMB_SIZE),

  ecc_add_jjj,
  ecc_mul_a,
  ecc_mul_g,
  ecc_j_to_a,

  ecc_b,
  ecc_g,
  NULL,
  ecc_unit,
  ecc_table
};

const struct ecc_curve *nettle_get_gost_256cpa(void)
{
  return &_gnutls_gost_256cpa;
}
