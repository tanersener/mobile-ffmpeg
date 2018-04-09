#ifndef STRATEGIES_PICTURE_SSE41_H_
#define STRATEGIES_PICTURE_SSE41_H_
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
 * Optimizations for SSE4.1.
 */

#include "global.h" // IWYU pragma: keep
#include "kvazaar.h"

unsigned kvz_reg_sad_sse41(const kvz_pixel * const data1, const kvz_pixel * const data2,
                           const int width, const int height, const unsigned stride1, const unsigned stride2);

int kvz_strategy_register_picture_sse41(void* opaque, uint8_t bitdepth);

#endif //STRATEGIES_PICTURE_SSE41_H_
