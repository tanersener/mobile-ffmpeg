/*****************************************************************************
 * This file is part of Kvazaar HEVC encoder.
 *
 * Copyright (C) 2013-2015 Tampere University of Technology and others (see
 * COPYING file).
 *
 * Kvazaar is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * Kvazaar is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with Kvazaar.  If not, see <http://www.gnu.org/licenses/>.
 ****************************************************************************/

#include "strategies/strategies-dct.h"

#include "avx2/dct-avx2.h"
#include "generic/dct-generic.h"
#include "strategyselector.h"


// Define function pointers.
dct_func * kvz_fast_forward_dst_4x4 = 0;

dct_func * kvz_dct_4x4 = 0;
dct_func * kvz_dct_8x8 = 0;
dct_func * kvz_dct_16x16 = 0;
dct_func * kvz_dct_32x32 = 0;

dct_func * kvz_fast_inverse_dst_4x4 = 0;

dct_func * kvz_idct_4x4 = 0;
dct_func * kvz_idct_8x8= 0;
dct_func * kvz_idct_16x16 = 0;
dct_func * kvz_idct_32x32 = 0;


int kvz_strategy_register_dct(void* opaque, uint8_t bitdepth) {
  bool success = true;

  success &= kvz_strategy_register_dct_generic(opaque, bitdepth);

  if (kvz_g_hardware_flags.intel_flags.avx2) {
    success &= kvz_strategy_register_dct_avx2(opaque, bitdepth);
  }

  return success;
}


/**
* \brief  Get a function that calculates SAD for NxN block.
*
* \param n  Width of the region for which SAD is calculated.
*
* \returns  Pointer to cost_16bit_nxn_func.
*/
dct_func * kvz_get_dct_func(int8_t width, int32_t mode)
{
  switch (width) {
  case 4:
    switch (mode){
    case 65535:
      return kvz_dct_4x4;
    default:
      return kvz_fast_forward_dst_4x4;
  }
  case 8:
    return kvz_dct_8x8;
  case 16:
    return kvz_dct_16x16;
  case 32:
    return kvz_dct_32x32;
  default:
    return NULL;
  }
}

/**
* \brief  Get a function that calculates SAD for NxN block.
*
* \param n  Width of the region for which SAD is calculated.
*
* \returns  Pointer to cost_16bit_nxn_func.
*/
dct_func * kvz_get_idct_func(int8_t width, int32_t mode)
{
  switch (width) {
  case 4:
    switch (mode){
    case 65535:
      return kvz_idct_4x4;
    default:
      return kvz_fast_inverse_dst_4x4;
    }
  case 8:
    return kvz_idct_8x8;
  case 16:
    return kvz_idct_16x16;
  case 32:
    return kvz_idct_32x32;
  default:
    return NULL;
  }
}
