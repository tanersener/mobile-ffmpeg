/*
 * Copyright (c) 2019, Alliance for Open Media. All rights reserved
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */

#include <assert.h>
#include <emmintrin.h>

#include "config/av1_rtcd.h"
#include "av1/encoder/encoder.h"
#include "av1/encoder/temporal_filter.h"

// For the squared error buffer, keep a padding for 4 samples
#define SSE_STRIDE (BW + 4)

#if EXPERIMENT_TEMPORAL_FILTER

DECLARE_ALIGNED(32, static const uint32_t, sse_bytemask_2x4[4][2][4]) = {
  { { 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF }, { 0xFFFF, 0x0000, 0x0000, 0x0000 } },
  { { 0x0000, 0xFFFF, 0xFFFF, 0xFFFF }, { 0xFFFF, 0xFFFF, 0x0000, 0x0000 } },
  { { 0x0000, 0x0000, 0xFFFF, 0xFFFF }, { 0xFFFF, 0xFFFF, 0xFFFF, 0x0000 } },
  { { 0x0000, 0x0000, 0x0000, 0xFFFF }, { 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF } }
};

static void get_squared_error(uint8_t *frame1, unsigned int stride,
                              uint8_t *frame2, unsigned int stride2,
                              int block_width, int block_height,
                              uint16_t *frame_sse, unsigned int dst_stride) {
  uint8_t *src1 = frame1;
  uint8_t *src2 = frame2;
  uint16_t *dst = frame_sse;

  for (int i = 0; i < block_height; i++) {
    for (int j = 0; j < block_width; j += 16) {
      // Set zero to unitialized memory to avoid uninitialized loads later
      *(uint32_t *)(dst) = _mm_cvtsi128_si32(_mm_setzero_si128());

      __m128i vsrc1 = _mm_loadu_si128((__m128i *)(src1 + j));
      __m128i vsrc2 = _mm_loadu_si128((__m128i *)(src2 + j));

      __m128i vmax = _mm_max_epu8(vsrc1, vsrc2);
      __m128i vmin = _mm_min_epu8(vsrc1, vsrc2);
      __m128i vdiff = _mm_subs_epu8(vmax, vmin);

      __m128i vzero = _mm_setzero_si128();
      __m128i vdiff1 = _mm_unpacklo_epi8(vdiff, vzero);
      __m128i vdiff2 = _mm_unpackhi_epi8(vdiff, vzero);

      __m128i vres1 = _mm_mullo_epi16(vdiff1, vdiff1);
      __m128i vres2 = _mm_mullo_epi16(vdiff2, vdiff2);

      _mm_storeu_si128((__m128i *)(dst + j + 2), vres1);
      _mm_storeu_si128((__m128i *)(dst + j + 10), vres2);
    }

    // Set zero to unitialized memory to avoid uninitialized loads later
    *(uint32_t *)(dst + block_width + 2) =
        _mm_cvtsi128_si32(_mm_setzero_si128());

    src1 += stride;
    src2 += stride2;
    dst += dst_stride;
  }
}

static void xx_load_and_pad(uint16_t *src, __m128i *dstvec, int col,
                            int block_width) {
  __m128i vtmp = _mm_loadu_si128((__m128i *)src);
  __m128i vzero = _mm_setzero_si128();
  __m128i vtmp1 = _mm_unpacklo_epi16(vtmp, vzero);
  __m128i vtmp2 = _mm_unpackhi_epi16(vtmp, vzero);
  // For the first column, replicate the first element twice to the left
  dstvec[0] = (col) ? vtmp1 : _mm_shuffle_epi32(vtmp1, 0xEA);
  // For the last column, replicate the last element twice to the right
  dstvec[1] = (col < block_width - 4) ? vtmp2 : _mm_shuffle_epi32(vtmp2, 0x54);
}

static int32_t xx_mask_and_hadd(__m128i vsum1, __m128i vsum2, int i) {
  __m128i veca, vecb;
  // Mask and obtain the required 5 values inside the vector
  veca = _mm_and_si128(vsum1, *(__m128i *)sse_bytemask_2x4[i][0]);
  vecb = _mm_and_si128(vsum2, *(__m128i *)sse_bytemask_2x4[i][1]);
  // A = [A0+B0, A1+B1, A2+B2, A3+B3]
  veca = _mm_add_epi32(veca, vecb);
  // B = [A2+B2, A3+B3, 0, 0]
  vecb = _mm_srli_si128(veca, 8);
  // A = [A0+B0+A2+B2, A1+B1+A3+B3, X, X]
  veca = _mm_add_epi32(veca, vecb);
  // B = [A1+B1+A3+B3, 0, 0, 0]
  vecb = _mm_srli_si128(veca, 4);
  // A = [A0+B0+A2+B2+A1+B1+A3+B3, X, X, X]
  veca = _mm_add_epi32(veca, vecb);
  return _mm_cvtsi128_si32(veca);
}

void av1_temporal_filter_plane_sse2(uint8_t *frame1, unsigned int stride,
                                    uint8_t *frame2, unsigned int stride2,
                                    int block_width, int block_height,
                                    int strength, double sigma,
                                    int decay_control, const int *blk_fw,
                                    int use_32x32, unsigned int *accumulator,
                                    uint16_t *count) {
  (void)strength;
  (void)blk_fw;
  (void)use_32x32;
  const double h = decay_control * (0.7 + log(sigma + 0.5));
  const double beta = 1.0;

  uint16_t frame_sse[SSE_STRIDE * BH];
  uint32_t acc_5x5_sse[BH][BW];

  assert(WINDOW_LENGTH == 2);
  assert(((block_width == 32) && (block_height == 32)) ||
         ((block_width == 16) && (block_height == 16)));

  get_squared_error(frame1, stride, frame2, stride2, block_width, block_height,
                    frame_sse, SSE_STRIDE);

  __m128i vsrc[5][2];

  // Traverse 4 columns at a time
  // First and last columns will require padding
  for (int col = 0; col < block_width; col += 4) {
    uint16_t *src = frame_sse + col;

    // Load and pad(for first and last col) 3 rows from the top
    for (int i = 2; i < 5; i++) {
      xx_load_and_pad(src, vsrc[i], col, block_width);
      src += SSE_STRIDE;
    }

    // Padding for top 2 rows
    vsrc[0][0] = vsrc[2][0];
    vsrc[0][1] = vsrc[2][1];
    vsrc[1][0] = vsrc[2][0];
    vsrc[1][1] = vsrc[2][1];

    for (int row = 0; row < block_height; row++) {
      __m128i vsum1 = _mm_setzero_si128();
      __m128i vsum2 = _mm_setzero_si128();

      // Add 5 consecutive rows
      for (int i = 0; i < 5; i++) {
        vsum1 = _mm_add_epi32(vsrc[i][0], vsum1);
        vsum2 = _mm_add_epi32(vsrc[i][1], vsum2);
      }

      // Push all elements by one element to the top
      for (int i = 0; i < 4; i++) {
        vsrc[i][0] = vsrc[i + 1][0];
        vsrc[i][1] = vsrc[i + 1][1];
      }

      if (row <= block_height - 4) {
        // Load next row
        xx_load_and_pad(src, vsrc[4], col, block_width);
        src += SSE_STRIDE;
      } else {
        // Padding for bottom 2 rows
        vsrc[4][0] = vsrc[3][0];
        vsrc[4][1] = vsrc[3][1];
      }

      // Accumulate the sum horizontally
      for (int i = 0; i < 4; i++) {
        acc_5x5_sse[row][col + i] = xx_mask_and_hadd(vsum1, vsum2, i);
      }
    }
  }

  for (int i = 0, k = 0; i < block_height; i++) {
    for (int j = 0; j < block_width; j++, k++) {
      const int pixel_value = frame2[i * stride2 + j];

      int diff_sse = acc_5x5_sse[i][j];
      diff_sse /= WINDOW_SIZE;

      double scaled_diff = -diff_sse / (2 * beta * h * h);
      // clamp the value to avoid underflow in exp()
      if (scaled_diff < -15) scaled_diff = -15;
      double w = exp(scaled_diff);
      const int weight = (int)(w * SCALE);

      count[k] += weight;
      accumulator[k] += weight * pixel_value;
    }
  }
}
#endif
