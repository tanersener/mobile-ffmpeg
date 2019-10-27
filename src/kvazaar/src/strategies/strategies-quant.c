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

#include "strategies/strategies-quant.h"

#include "strategies/avx2/quant-avx2.h"
#include "strategies/generic/quant-generic.h"
#include "strategyselector.h"


// Define function pointers.
quant_func *kvz_quant;
quant_residual_func *kvz_quantize_residual;
dequant_func *kvz_dequant;
coeff_abs_sum_func *kvz_coeff_abs_sum;
fast_coeff_cost_func *kvz_fast_coeff_cost;


int kvz_strategy_register_quant(void* opaque, uint8_t bitdepth) {
  bool success = true;

  success &= kvz_strategy_register_quant_generic(opaque, bitdepth);

  if (kvz_g_hardware_flags.intel_flags.avx2) {
    success &= kvz_strategy_register_quant_avx2(opaque, bitdepth);
  }
  return success;
}
