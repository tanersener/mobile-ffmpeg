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

#include "strategies/x86_asm/picture-x86-asm.h"

#if defined(KVZ_COMPILE_ASM)
#include <stdlib.h>

#include "kvazaar.h"
#include "strategies/x86_asm/picture-x86-asm-sad.h"
#include "strategies/x86_asm/picture-x86-asm-satd.h"
#include "strategies/sse41/picture-sse41.h"
#include "strategyselector.h"


static unsigned kvz_sad_32x32_avx(const kvz_pixel *data1, const kvz_pixel *data2)
{
  unsigned sad = 0;
  sad += kvz_sad_16x16_avx(data1, data2);
  sad += kvz_sad_16x16_avx(data1 + 8 * 32, data2 + 8 * 32);
  sad += kvz_sad_16x16_avx(data1 + 16 * 32, data2 + 16 * 32);
  sad += kvz_sad_16x16_avx(data1 + 24 * 32, data2 + 24 * 32);
  return sad;
}

static unsigned kvz_sad_64x64_avx(const kvz_pixel *data1, const kvz_pixel *data2)
{
  unsigned sad = 0;
  sad += kvz_sad_32x32_avx(data1, data2);
  sad += kvz_sad_32x32_avx(data1 + 16 * 64, data2 + 16 * 64);
  sad += kvz_sad_32x32_avx(data1 + 32 * 64, data2 + 32 * 64);
  sad += kvz_sad_32x32_avx(data1 + 48 * 64, data2 + 48 * 64);
  return sad;
}

static unsigned kvz_sad_other_avx(const kvz_pixel *data1, const kvz_pixel *data2,
                                  int width, int height,
                                  unsigned stride)
{
  unsigned sad = 0;

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      sad += abs(data1[y * stride + x] - data2[y * stride + x]);
    }
  }

  return sad;
}

static unsigned reg_sad_x86_asm(const kvz_pixel *data1, const kvz_pixel * data2,
                                const int width, const int height,
                                const unsigned stride1, const unsigned stride2)
{
  if (width == height) {
    if (width == 8) {
      return kvz_sad_8x8_stride_avx(data1, data2, stride1);
    } else if (width == 16) {
      return kvz_sad_16x16_stride_avx(data1, data2, stride1);
    } else if (width == 32) {
      return kvz_sad_32x32_stride_avx(data1, data2, stride1);
    } else if (width == 64) {
      return kvz_sad_64x64_stride_avx(data1, data2, stride1);
    }
  }

  if (width * height >= 16) {
    // Call the vectorized general SAD SSE41 function when the block
    // is big enough to make it worth it.
    return kvz_reg_sad_sse41(data1, data2, width, height, stride1, stride2);
  } else {
    return kvz_sad_other_avx(data1, data2, width, height, stride1);
  }
}

#endif //defined(KVZ_COMPILE_ASM)

int kvz_strategy_register_picture_x86_asm_avx(void* opaque, uint8_t bitdepth)
{
  bool success = true;
#if defined(KVZ_COMPILE_ASM)
  if (bitdepth == 8){
    success &= kvz_strategyselector_register(opaque, "reg_sad", "x86_asm_avx", 30, &reg_sad_x86_asm);

    success &= kvz_strategyselector_register(opaque, "sad_4x4", "x86_asm_avx", 30, &kvz_sad_4x4_avx);
    success &= kvz_strategyselector_register(opaque, "sad_8x8", "x86_asm_avx", 30, &kvz_sad_8x8_avx);
    success &= kvz_strategyselector_register(opaque, "sad_16x16", "x86_asm_avx", 30, &kvz_sad_16x16_avx);
    success &= kvz_strategyselector_register(opaque, "sad_32x32", "x86_asm_avx", 30, &kvz_sad_32x32_avx);
    success &= kvz_strategyselector_register(opaque, "sad_64x64", "x86_asm_avx", 30, &kvz_sad_64x64_avx);

    success &= kvz_strategyselector_register(opaque, "satd_4x4", "x86_asm_avx", 30, &kvz_satd_4x4_avx);
    success &= kvz_strategyselector_register(opaque, "satd_8x8", "x86_asm_avx", 30, &kvz_satd_8x8_avx);
    success &= kvz_strategyselector_register(opaque, "satd_16x16", "x86_asm_avx", 30, &kvz_satd_16x16_avx);
    success &= kvz_strategyselector_register(opaque, "satd_32x32", "x86_asm_avx", 30, &kvz_satd_32x32_avx);
    success &= kvz_strategyselector_register(opaque, "satd_64x64", "x86_asm_avx", 30, &kvz_satd_64x64_avx);
  }
#endif //!defined(KVZ_COMPILE_ASM)
  return success;
}
