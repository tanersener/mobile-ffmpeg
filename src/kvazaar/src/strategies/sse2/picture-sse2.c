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

#include "strategies/sse2/picture-sse2.h"

#if COMPILE_INTEL_SSE2
#include <immintrin.h>
#include <stdlib.h>

#include "kvazaar.h"
#include "strategyselector.h"


static unsigned reg_sad_sse2(const kvz_pixel * const data1, const kvz_pixel * const data2,
                        const int width, const int height, const unsigned stride1, const unsigned stride2)
{
  int y, x;
  unsigned sad = 0;
  __m128i sse_inc = _mm_setzero_si128 ();
  long long int sse_inc_array[2];
  
  for (y = 0; y < height; ++y) {
    for (x = 0; x <= width-16; x+=16) {
      const __m128i a = _mm_loadu_si128((__m128i const*) &data1[y * stride1 + x]);
      const __m128i b = _mm_loadu_si128((__m128i const*) &data2[y * stride2 + x]);
      sse_inc = _mm_add_epi32(sse_inc, _mm_sad_epu8(a,b));
    }

    for (; x < width; ++x) {
      sad += abs(data1[y * stride1 + x] - data2[y * stride2 + x]);
    }
  }
  _mm_storeu_si128((__m128i*) sse_inc_array, sse_inc);
  sad += sse_inc_array[0] + sse_inc_array[1];

  return sad;
}

static unsigned sad_8bit_4x4_sse2(const kvz_pixel *buf1, const kvz_pixel *buf2)
{
  const __m128i *const mbuf1 = (const __m128i *)buf1;
  const __m128i *const mbuf2 = (const __m128i *)buf2;

  __m128i sum = _mm_sad_epu8(_mm_load_si128(mbuf1), _mm_load_si128(mbuf2));

  uint32_t result[4];
  _mm_storeu_si128((__m128i*)result, sum);
  return result[0] + result[2];
}

#endif //COMPILE_INTEL_SSE2

int kvz_strategy_register_picture_sse2(void* opaque, uint8_t bitdepth) {
  bool success = true;
#if COMPILE_INTEL_SSE2
  if (bitdepth == 8){
    success &= kvz_strategyselector_register(opaque, "reg_sad", "sse2", 10, &reg_sad_sse2);
    success &= kvz_strategyselector_register(opaque, "sad_4x4", "sse2", 10, &sad_8bit_4x4_sse2);
  }
#endif
  return success;
}


