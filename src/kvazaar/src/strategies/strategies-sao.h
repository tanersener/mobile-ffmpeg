#ifndef STRATEGIES_SAO_H_
#define STRATEGIES_SAO_H_
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
 * Interface for sao functions.
 */

#include "encoder.h"
#include "encoderstate.h"
#include "global.h" // IWYU pragma: keep
#include "kvazaar.h"
#include "sao.h"


// Declare function pointers.
typedef int (sao_edge_ddistortion_func)(const kvz_pixel *orig_data, const kvz_pixel *rec_data,
  int block_width, int block_height,
  int eo_class, int offsets[NUM_SAO_EDGE_CATEGORIES]);

typedef void (calc_sao_edge_dir_func)(const kvz_pixel *orig_data, const kvz_pixel *rec_data,
  int eo_class, int block_width, int block_height,
  int cat_sum_cnt[2][NUM_SAO_EDGE_CATEGORIES]);

typedef void (sao_reconstruct_color_func)(const encoder_control_t * const encoder,
  const kvz_pixel *rec_data, kvz_pixel *new_rec_data,
  const sao_info_t *sao,
  int stride, int new_stride,
  int block_width, int block_height,
  color_t color_i);

typedef int (sao_band_ddistortion_func)(const encoder_state_t * const state, const kvz_pixel *orig_data, const kvz_pixel *rec_data,
  int block_width, int block_height,
  int band_pos, const int sao_bands[4]);

// Declare function pointers.
extern sao_edge_ddistortion_func * kvz_sao_edge_ddistortion;
extern calc_sao_edge_dir_func * kvz_calc_sao_edge_dir;
extern sao_reconstruct_color_func * kvz_sao_reconstruct_color;
extern sao_band_ddistortion_func * kvz_sao_band_ddistortion;

int kvz_strategy_register_sao(void* opaque, uint8_t bitdepth);


#define STRATEGIES_SAO_EXPORTS \
  {"sao_edge_ddistortion", (void**) &kvz_sao_edge_ddistortion}, \
  {"calc_sao_edge_dir", (void**) &kvz_calc_sao_edge_dir}, \
  {"sao_reconstruct_color", (void**) &kvz_sao_reconstruct_color}, \
  {"sao_band_ddistortion", (void**) &kvz_sao_band_ddistortion}, \



#endif //STRATEGIES_SAO_H_
