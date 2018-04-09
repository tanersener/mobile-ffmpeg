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

#include "strategies/strategies-ipol.h"

#include "strategies/avx2/ipol-avx2.h"
#include "strategies/generic/ipol-generic.h"
#include "strategyselector.h"


// Define function pointers.
ipol_func *kvz_filter_inter_quarterpel_luma;
ipol_func *kvz_filter_inter_halfpel_chroma;
ipol_func *kvz_filter_inter_octpel_chroma;
ipol_frac_blocks_func *kvz_filter_frac_blocks_luma;
epol_func *kvz_get_extended_block;
kvz_sample_quarterpel_luma_func * kvz_sample_quarterpel_luma;
kvz_sample_octpel_chroma_func * kvz_sample_octpel_chroma;
kvz_sample_14bit_quarterpel_luma_func * kvz_sample_14bit_quarterpel_luma;
kvz_sample_14bit_octpel_chroma_func * kvz_sample_14bit_octpel_chroma;


int kvz_strategy_register_ipol(void* opaque, uint8_t bitdepth) {
  bool success = true;

  success &= kvz_strategy_register_ipol_generic(opaque, bitdepth);

  if (kvz_g_hardware_flags.intel_flags.avx2) {
    success &= kvz_strategy_register_ipol_avx2(opaque, bitdepth);
  }
  return success;
}
