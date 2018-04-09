#ifndef STRATEGIES_DCT_GENERIC_H_
#define STRATEGIES_DCT_GENERIC_H_
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

#include  "global.h" // IWYU pragma: keep

extern const int16_t kvz_g_dst_4[4][4];
extern const int16_t kvz_g_dct_4[4][4];
extern const int16_t kvz_g_dct_8[8][8];
extern const int16_t kvz_g_dct_16[16][16];
extern const int16_t kvz_g_dct_32[32][32];

extern const int16_t kvz_g_dst_4_t[4][4];
extern const int16_t kvz_g_dct_4_t[4][4];
extern const int16_t kvz_g_dct_8_t[8][8];
extern const int16_t kvz_g_dct_16_t[16][16];
extern const int16_t kvz_g_dct_32_t[32][32];

int kvz_strategy_register_dct_generic(void* opaque, uint8_t bitdepth);

#endif //STRATEGIES_DCT_GENERIC_H_
