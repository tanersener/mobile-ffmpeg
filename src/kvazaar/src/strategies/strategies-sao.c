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

#include "strategies/strategies-sao.h"
#include "strategies/avx2/sao-avx2.h"
#include "strategies/generic/sao-generic.h"
#include "strategyselector.h"


// Define function pointers.
sao_edge_ddistortion_func * kvz_sao_edge_ddistortion;
calc_sao_edge_dir_func * kvz_calc_sao_edge_dir;
sao_reconstruct_color_func * kvz_sao_reconstruct_color;
sao_band_ddistortion_func * kvz_sao_band_ddistortion;


int kvz_strategy_register_sao(void* opaque, uint8_t bitdepth) {
  bool success = true;

  success &= kvz_strategy_register_sao_generic(opaque, bitdepth);

  if (kvz_g_hardware_flags.intel_flags.avx2) {
    success &= kvz_strategy_register_sao_avx2(opaque, bitdepth);
  }

  return success;
}