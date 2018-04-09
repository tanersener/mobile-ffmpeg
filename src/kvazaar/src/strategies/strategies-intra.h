#ifndef STRATEGIES_INTRA_H_
#define STRATEGIES_INTRA_H_
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
 * Interface for intra prediction functions.
 */

#include "global.h" // IWYU pragma: keep
#include "kvazaar.h"


typedef void (angular_pred_func)(
  const int_fast8_t log2_width,
  const int_fast8_t intra_mode,
  const kvz_pixel *const in_ref_above,
  const kvz_pixel *const in_ref_left,
  kvz_pixel *const dst);

typedef void (intra_pred_planar_func)(
  const int_fast8_t log2_width,
  const kvz_pixel *const ref_top,
  const kvz_pixel *const ref_left,
  kvz_pixel *const dst);

// Declare function pointers.
extern angular_pred_func * kvz_angular_pred;
extern intra_pred_planar_func * kvz_intra_pred_planar;

int kvz_strategy_register_intra(void* opaque, uint8_t bitdepth);


#define STRATEGIES_INTRA_EXPORTS \
  {"angular_pred", (void**) &kvz_angular_pred}, \
  {"intra_pred_planar", (void**) &kvz_intra_pred_planar}, \



#endif //STRATEGIES_INTRA_H_
