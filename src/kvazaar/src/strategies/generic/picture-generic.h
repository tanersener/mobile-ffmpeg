#ifndef STRATEGIES_PICTURE_GENERIC_H_
#define STRATEGIES_PICTURE_GENERIC_H_
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
 * Generic C implementations of optimized functions.
 */

#include "global.h" // IWYU pragma: keep
#include "kvazaar.h"

int kvz_strategy_register_picture_generic(void* opaque, uint8_t bitdepth);

// Function to clip int16_t to pixel. (0-255 or 0-1023)
// Assumes PIXEL_MAX to be 2^n-1
kvz_pixel kvz_fast_clip_16bit_to_pixel(int16_t value);

// Function to clip int32_t to pixel. (0-255 or 0-1023)
// Assumes PIXEL_MAX to be 2^n-1
kvz_pixel kvz_fast_clip_32bit_to_pixel(int32_t value);

unsigned kvz_satd_4x4_subblock_generic(const kvz_pixel * buf1,
                                       const int32_t     stride1,
                                       const kvz_pixel * buf2,
                                       const int32_t     stride2);

void kvz_satd_4x4_subblock_quad_generic(const kvz_pixel *preds[4],
                                        const int strides[4],
                                        const kvz_pixel *orig,
                                        const int orig_stride,
                                        unsigned costs[4]);

#endif //STRATEGIES_PICTURE_GENERIC_H_
