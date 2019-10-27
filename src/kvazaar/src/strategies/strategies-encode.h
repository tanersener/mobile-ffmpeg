#ifndef STRATEGIES_ENCODE_H_
#define STRATEGIES_ENCODE_H_
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

/**
 * \ingroup Optimization
 * \file
 * Interface for quantization functions.
 */

#include "cu.h"
#include "encoderstate.h"
#include "global.h" // IWYU pragma: keep
#include "kvazaar.h"
#include "tables.h"


// Declare function pointers.
typedef unsigned (encode_coeff_nxn_func)(encoder_state_t * const state,
                                         cabac_data_t * const cabac,
                                         const coeff_t *coeff,
                                         uint8_t width,
                                         uint8_t type,
                                         int8_t scan_mode,
                                         int8_t tr_skip);

// Declare function pointers.
extern encode_coeff_nxn_func *kvz_encode_coeff_nxn;

int kvz_strategy_register_encode(void* opaque, uint8_t bitdepth);


#define STRATEGIES_ENCODE_EXPORTS \
  {"encode_coeff_nxn", (void**) &kvz_encode_coeff_nxn}, \



#endif //STRATEGIES_ENCODE_H_
