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
#include <immintrin.h>

#include "config/av1_rtcd.h"
#include "av1/encoder/encoder.h"
#include "av1/encoder/temporal_filter.h"

#define SSE_STRIDE (BW + 2)

#if EXPERIMENT_TEMPORAL_FILTER
DECLARE_ALIGNED(32, static const uint32_t, sse_bytemask[4][8]) = {
  { 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0x0000, 0x0000 },
  { 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0x0000 },
  { 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000 },
  { 0x0000, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF }
};

DECLARE_ALIGNED(32, static const uint8_t, shufflemask_16b[2][16]) = {
  { 0, 1, 0, 1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 },
  { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 10, 11, 10, 11 }
};

static AOM_FORCE_INLINE void get_squared_error_16x16_avx2(
    uint8_t *frame1, unsigned int stride, uint8_t *frame2, unsigned int stride2,
    int block_width, int block_height, uint16_t *frame_sse,
    unsigned int sse_stride) {
  (void)block_width;
  uint8_t *src1 = frame1;
  uint8_t *src2 = frame2;
  uint16_t *dst = frame_sse;
  for (int i = 0; i < block_height; i++) {
    __m128i vf1_128, vf2_128;
    __m256i vf1, vf2, vdiff1, vsqdiff1;

    vf1_128 = _mm_loadu_si128((__m128i *)(src1));
    vf2_128 = _mm_loadu_si128((__m128i *)(src2));
    vf1 = _mm256_cvtepu8_epi16(vf1_128);
    vf2 = _mm256_cvtepu8_epi16(vf2_128);
    vdiff1 = _mm256_sub_epi16(vf1, vf2);
    vsqdiff1 = _mm256_mullo_epi16(vdiff1, vdiff1);

    _mm256_storeu_si256((__m256i *)(dst), vsqdiff1);
    // Set zero to unitialized memory to avoid uninitialized loads later
    *(uint32_t *)(dst + 16) = _mm_cvtsi128_si32(_mm_setzero_si128());

    src1 += stride, src2 += stride2;
    dst += sse_stride;
  }
}

static AOM_FORCE_INLINE void get_squared_error_32x32_avx2(
    uint8_t *frame1, unsigned int stride, uint8_t *frame2, unsigned int stride2,
    int block_width, int block_height, uint16_t *frame_sse,
    unsigned int sse_stride) {
  (void)block_width;
  uint8_t *src1 = frame1;
  uint8_t *src2 = frame2;
  uint16_t *dst = frame_sse;
  for (int i = 0; i < block_height; i++) {
    __m256i vsrc1, vsrc2, vmin, vmax, vdiff, vdiff1, vdiff2, vres1, vres2;

    vsrc1 = _mm256_loadu_si256((__m256i *)src1);
    vsrc2 = _mm256_loadu_si256((__m256i *)src2);
    vmax = _mm256_max_epu8(vsrc1, vsrc2);
    vmin = _mm256_min_epu8(vsrc1, vsrc2);
    vdiff = _mm256_subs_epu8(vmax, vmin);

    __m128i vtmp1 = _mm256_castsi256_si128(vdiff);
    __m128i vtmp2 = _mm256_extracti128_si256(vdiff, 1);
    vdiff1 = _mm256_cvtepu8_epi16(vtmp1);
    vdiff2 = _mm256_cvtepu8_epi16(vtmp2);

    vres1 = _mm256_mullo_epi16(vdiff1, vdiff1);
    vres2 = _mm256_mullo_epi16(vdiff2, vdiff2);
    _mm256_storeu_si256((__m256i *)(dst), vres1);
    _mm256_storeu_si256((__m256i *)(dst + 16), vres2);
    // Set zero to unitialized memory to avoid uninitialized loads later
    *(uint32_t *)(dst + 32) = _mm_cvtsi128_si32(_mm_setzero_si128());

    src1 += stride;
    src2 += stride2;
    dst += sse_stride;
  }
}

static AOM_FORCE_INLINE __m256i xx_load_and_pad(uint16_t *src, int col,
                                                int block_width) {
  __m128i v128tmp = _mm_loadu_si128((__m128i *)(src));
  if (col == 0) {
    // For the first column, replicate the first element twice to the left
    v128tmp = _mm_shuffle_epi8(v128tmp, *(__m128i *)shufflemask_16b[0]);
  }
  if (col == block_width - 4) {
    // For the last column, replicate the last element twice to the right
    v128tmp = _mm_shuffle_epi8(v128tmp, *(__m128i *)shufflemask_16b[1]);
  }
  return _mm256_cvtepi16_epi32(v128tmp);
}

static AOM_FORCE_INLINE int32_t xx_mask_and_hadd(__m256i vsum, int i) {
  // Mask the required 5 values inside the vector
  __m256i vtmp = _mm256_and_si256(vsum, *(__m256i *)sse_bytemask[i]);
  __m128i v128a, v128b;
  // Extract 256b as two 128b registers A and B
  v128a = _mm256_castsi256_si128(vtmp);
  v128b = _mm256_extracti128_si256(vtmp, 1);
  // A = [A0+B0, A1+B1, A2+B2, A3+B3]
  v128a = _mm_add_epi32(v128a, v128b);
  // B = [A2+B2, A3+B3, 0, 0]
  v128b = _mm_srli_si128(v128a, 8);
  // A = [A0+B0+A2+B2, A1+B1+A3+B3, X, X]
  v128a = _mm_add_epi32(v128a, v128b);
  // B = [A1+B1+A3+B3, 0, 0, 0]
  v128b = _mm_srli_si128(v128a, 4);
  // A = [A0+B0+A2+B2+A1+B1+A3+B3, X, X, X]
  v128a = _mm_add_epi32(v128a, v128b);
  return _mm_extract_epi32(v128a, 0);
}

void av1_temporal_filter_plane_avx2(uint8_t *frame1, unsigned int stride,
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

  assert(((block_width == 32) && (block_height == 32)) ||
         ((block_width == 16) && (block_height == 16)));

  if (block_width == 32) {
    get_squared_error_32x32_avx2(frame1, stride, frame2, stride2, block_width,
                                 block_height, frame_sse, SSE_STRIDE);
  } else {
    get_squared_error_16x16_avx2(frame1, stride, frame2, stride2, block_width,
                                 block_height, frame_sse, SSE_STRIDE);
  }

  __m256i vsrc[5];

  // Traverse 4 columns at a time
  // First and last columns will require padding
  for (int col = 0; col < block_width; col += 4) {
    uint16_t *src = (col) ? frame_sse + col - 2 : frame_sse;

    // Load and pad(for first and last col) 3 rows from the top
    for (int i = 2; i < 5; i++) {
      vsrc[i] = xx_load_and_pad(src, col, block_width);
      src += SSE_STRIDE;
    }

    // Copy first row to first 2 vectors
    vsrc[0] = vsrc[2];
    vsrc[1] = vsrc[2];

    for (int row = 0; row < block_height; row++) {
      __m256i vsum = _mm256_setzero_si256();

      // Add 5 consecutive rows
      for (int i = 0; i < 5; i++) {
        vsum = _mm256_add_epi32(vsum, vsrc[i]);
      }

      // Push all elements by one element to the top
      for (int i = 0; i < 4; i++) {
        vsrc[i] = vsrc[i + 1];
      }

      // Load next row to the last element
      if (row <= block_width - 4) {
        vsrc[4] = xx_load_and_pad(src, col, block_width);
        src += SSE_STRIDE;
      } else {
        vsrc[4] = vsrc[3];
      }

      // Accumulate the sum horizontally
      for (int i = 0; i < 4; i++) {
        acc_5x5_sse[row][col + i] = xx_mask_and_hadd(vsum, i);
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
