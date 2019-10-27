#ifndef STRATEGIES_DCT_H_
#define STRATEGIES_DCT_H_
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
 * Interface for transform functions.
 */

#include "global.h" // IWYU pragma: keep
#include "cu.h"

typedef unsigned (dct_func)(int8_t bitdepth, const int16_t *input, int16_t *output);


// Declare function pointers.
extern dct_func * kvz_fast_forward_dst_4x4;

extern dct_func * kvz_dct_4x4;
extern dct_func * kvz_dct_8x8;
extern dct_func * kvz_dct_16x16;
extern dct_func * kvz_dct_32x32;

extern dct_func * kvz_fast_inverse_dst_4x4;

extern dct_func * kvz_idct_4x4;
extern dct_func * kvz_idct_8x8;
extern dct_func * kvz_idct_16x16;
extern dct_func * kvz_idct_32x32;


int kvz_strategy_register_dct(void* opaque, uint8_t bitdepth);
dct_func * kvz_get_dct_func(int8_t width, color_t color, cu_type_t type);
dct_func * kvz_get_idct_func(int8_t width, color_t color, cu_type_t type);



#define STRATEGIES_DCT_EXPORTS \
  {"fast_forward_dst_4x4", (void**) &kvz_fast_forward_dst_4x4}, \
  \
  {"dct_4x4", (void**) &kvz_dct_4x4}, \
  {"dct_8x8", (void**) &kvz_dct_8x8}, \
  {"dct_16x16", (void**) &kvz_dct_16x16}, \
  {"dct_32x32", (void**) &kvz_dct_32x32}, \
  \
  {"fast_inverse_dst_4x4", (void**) &kvz_fast_inverse_dst_4x4}, \
  \
  {"idct_4x4", (void**)&kvz_idct_4x4}, \
  {"idct_8x8", (void**)&kvz_idct_8x8}, \
  {"idct_16x16", (void**)&kvz_idct_16x16}, \
  {"idct_32x32", (void**)&kvz_idct_32x32}, \



#endif //STRATEGIES_DCT_H_
