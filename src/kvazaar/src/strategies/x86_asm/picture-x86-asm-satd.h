#ifndef _PICTURE_X86_ASM_SATD_H_
#define _PICTURE_X86_ASM_SATD_H_
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
 * Optimizations for AVX, utilizing ASM implementations.
 */

#include "global.h" // IWYU pragma: keep


unsigned kvz_satd_4x4_avx(const kvz_pixel *org, const kvz_pixel *cur);
unsigned kvz_satd_8x8_avx(const kvz_pixel *org, const kvz_pixel *cur);
unsigned kvz_satd_16x16_avx(const kvz_pixel *org, const kvz_pixel *cur);
unsigned kvz_satd_32x32_avx(const kvz_pixel *org, const kvz_pixel *cur);
unsigned kvz_satd_64x64_avx(const kvz_pixel *org, const kvz_pixel *cur);

#endif
