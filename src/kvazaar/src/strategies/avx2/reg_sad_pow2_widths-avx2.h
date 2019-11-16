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

#ifndef REG_SAD_POW2_WIDTHS_AVX2_H_
#define REG_SAD_POW2_WIDTHS_AVX2_H_

#include "strategies/sse41/reg_sad_pow2_widths-sse41.h"
#include "kvazaar.h"

static INLINE uint32_t reg_sad_w32(const kvz_pixel * const data1, const kvz_pixel * const data2,
                            const int32_t height, const uint32_t stride1,
                            const uint32_t stride2)
{
  __m256i avx_inc = _mm256_setzero_si256();
  int32_t y;

  const int32_t height_fourline_groups = height & ~3;
  const int32_t height_residual_lines  = height &  3;

  for (y = 0; y < height_fourline_groups; y += 4) {
    __m256i a = _mm256_loadu_si256((const __m256i *)(data1 + (y + 0) * stride1));
    __m256i b = _mm256_loadu_si256((const __m256i *)(data2 + (y + 0) * stride2));
    __m256i c = _mm256_loadu_si256((const __m256i *)(data1 + (y + 1) * stride1));
    __m256i d = _mm256_loadu_si256((const __m256i *)(data2 + (y + 1) * stride2));
    __m256i e = _mm256_loadu_si256((const __m256i *)(data1 + (y + 2) * stride1));
    __m256i f = _mm256_loadu_si256((const __m256i *)(data2 + (y + 2) * stride2));
    __m256i g = _mm256_loadu_si256((const __m256i *)(data1 + (y + 3) * stride1));
    __m256i h = _mm256_loadu_si256((const __m256i *)(data2 + (y + 3) * stride2));

    __m256i curr_sads_ab = _mm256_sad_epu8(a, b);
    __m256i curr_sads_cd = _mm256_sad_epu8(c, d);
    __m256i curr_sads_ef = _mm256_sad_epu8(e, f);
    __m256i curr_sads_gh = _mm256_sad_epu8(g, h);

    avx_inc = _mm256_add_epi64(avx_inc, curr_sads_ab);
    avx_inc = _mm256_add_epi64(avx_inc, curr_sads_cd);
    avx_inc = _mm256_add_epi64(avx_inc, curr_sads_ef);
    avx_inc = _mm256_add_epi64(avx_inc, curr_sads_gh);
  }
  if (height_residual_lines) {
    for (; y < height; y++) {
      __m256i a = _mm256_loadu_si256((const __m256i *)(data1 + (y + 0) * stride1));
      __m256i b = _mm256_loadu_si256((const __m256i *)(data2 + (y + 0) * stride2));

      __m256i curr_sads = _mm256_sad_epu8(a, b);
      avx_inc = _mm256_add_epi64(avx_inc, curr_sads);
    }
  }

  __m128i inchi = _mm256_extracti128_si256(avx_inc, 1);
  __m128i inclo = _mm256_castsi256_si128  (avx_inc);

  __m128i sum_1 = _mm_add_epi64    (inclo, inchi);
  __m128i sum_2 = _mm_shuffle_epi32(sum_1, _MM_SHUFFLE(1, 0, 3, 2));
  __m128i sad   = _mm_add_epi64    (sum_1, sum_2);

  return _mm_cvtsi128_si32(sad);
}

static INLINE uint32_t reg_sad_w64(const kvz_pixel * const data1, const kvz_pixel * const data2,
                            const int32_t height, const uint32_t stride1,
                            const uint32_t stride2)
{
  __m256i avx_inc = _mm256_setzero_si256();
  int32_t y;

  const int32_t height_twoline_groups = height & ~1;
  const int32_t height_residual_lines = height &  1;

  for (y = 0; y < height_twoline_groups; y += 2) {
    __m256i a = _mm256_loadu_si256((const __m256i *)(data1 + (y + 0) * stride1));
    __m256i b = _mm256_loadu_si256((const __m256i *)(data2 + (y + 0) * stride2));
    __m256i c = _mm256_loadu_si256((const __m256i *)(data1 + (y + 0) * stride1 + 32));
    __m256i d = _mm256_loadu_si256((const __m256i *)(data2 + (y + 0) * stride2 + 32));

    __m256i e = _mm256_loadu_si256((const __m256i *)(data1 + (y + 1) * stride1));
    __m256i f = _mm256_loadu_si256((const __m256i *)(data2 + (y + 1) * stride2));
    __m256i g = _mm256_loadu_si256((const __m256i *)(data1 + (y + 1) * stride1 + 32));
    __m256i h = _mm256_loadu_si256((const __m256i *)(data2 + (y + 1) * stride2 + 32));

    __m256i curr_sads_ab = _mm256_sad_epu8(a, b);
    __m256i curr_sads_cd = _mm256_sad_epu8(c, d);
    __m256i curr_sads_ef = _mm256_sad_epu8(e, f);
    __m256i curr_sads_gh = _mm256_sad_epu8(g, h);

    avx_inc = _mm256_add_epi64(avx_inc, curr_sads_ab);
    avx_inc = _mm256_add_epi64(avx_inc, curr_sads_cd);
    avx_inc = _mm256_add_epi64(avx_inc, curr_sads_ef);
    avx_inc = _mm256_add_epi64(avx_inc, curr_sads_gh);
  }
  if (height_residual_lines) {
    for (; y < height; y++) {
      __m256i a = _mm256_loadu_si256((const __m256i *)(data1 + (y + 0) * stride1));
      __m256i b = _mm256_loadu_si256((const __m256i *)(data2 + (y + 0) * stride2));
      __m256i c = _mm256_loadu_si256((const __m256i *)(data1 + (y + 0) * stride1 + 32));
      __m256i d = _mm256_loadu_si256((const __m256i *)(data2 + (y + 0) * stride2 + 32));

      __m256i curr_sads_ab = _mm256_sad_epu8(a, b);
      __m256i curr_sads_cd = _mm256_sad_epu8(c, d);
      avx_inc = _mm256_add_epi64(avx_inc, curr_sads_ab);
      avx_inc = _mm256_add_epi64(avx_inc, curr_sads_cd);
    }
  }

  __m128i inchi = _mm256_extracti128_si256(avx_inc, 1);
  __m128i inclo = _mm256_castsi256_si128  (avx_inc);

  __m128i sum_1 = _mm_add_epi64    (inclo, inchi);
  __m128i sum_2 = _mm_shuffle_epi32(sum_1, _MM_SHUFFLE(1, 0, 3, 2));
  __m128i sad   = _mm_add_epi64    (sum_1, sum_2);

  return _mm_cvtsi128_si32(sad);
}

static uint32_t hor_sad_avx2_w32(const kvz_pixel *pic_data, const kvz_pixel *ref_data,
                                 int32_t height, uint32_t pic_stride, uint32_t ref_stride,
                                 const uint32_t left, const uint32_t right)
{
  __m256i avx_inc = _mm256_setzero_si256();

  const size_t block_width      = 32;
  const size_t block_width_log2 = 5;
  const size_t lane_width       = 16;

  const int32_t left_eq_wid     = left  >> block_width_log2;
  const int32_t left_clamped    = left  -  left_eq_wid;
  const int32_t right_eq_wid    = right >> block_width_log2;
  const int32_t right_clamped   = right -  right_eq_wid;

  const __m256i zero        = _mm256_setzero_si256();
  const __m256i lane_widths = _mm256_set1_epi8((uint8_t)lane_width);
  const __m256i lefts       = _mm256_set1_epi8((uint8_t)left_clamped);
  const __m256i rights      = _mm256_set1_epi8((uint8_t)right_clamped);
  const __m256i unsign_mask = _mm256_set1_epi8(0x7f);
  const __m256i ns          = _mm256_setr_epi8(0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15,
                                               16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31);

  const __m256i rightmost_good_idx = _mm256_set1_epi8((uint8_t)(block_width - right - 1));

  const __m256i shufmask1_l    = _mm256_sub_epi8  (ns,          lefts);
  const __m256i shufmask1_r    = _mm256_add_epi8  (shufmask1_l, rights);
  const __m256i shufmask1      = _mm256_and_si256 (shufmask1_r, unsign_mask);

  const __m256i epol_mask_r    = _mm256_min_epi8  (ns,    rightmost_good_idx);
  const __m256i epol_mask      = _mm256_max_epi8  (lefts, epol_mask_r);

  const __m256i mlo2hi_mask_l  = _mm256_cmpgt_epi8(lefts, ns);
  const __m256i mlo2hi_imask_r = _mm256_cmpgt_epi8(lane_widths, shufmask1);
  const __m256i mlo2hi_mask_r  = _mm256_cmpeq_epi8(mlo2hi_imask_r, zero);

  // For left != 0,  use low lane of mlo2hi_mask_l as blend mask for high lane.
  // For right != 0, use low lane of mlo2hi_mask_r as blend mask for low lane.
  const __m256i xchg_mask1     = _mm256_permute2x128_si256(mlo2hi_mask_l, mlo2hi_mask_r, 0x02);

  // If left != 0 (ie. right == 0), the xchg should only affect high lane,
  // if right != 0 (ie. left == 0), the low lane. Set bits on the lane that
  // the xchg should affect. left == right == 0 should never happen, this'll
  // break if it does.
  const __m256i lanes_llo_rhi  = _mm256_blend_epi32(lefts, rights, 0xf0);
  const __m256i xchg_lane_mask = _mm256_cmpeq_epi32(lanes_llo_rhi, zero);

  const __m256i xchg_data_mask = _mm256_and_si256(xchg_mask1, xchg_lane_mask);

  // If we're straddling the left border, start from the left border instead,
  // and if right border, end on the border
  const int32_t ld_offset = left - right;

  int32_t y;
  for (y = 0; y < height; y++) {
    __m256i a = _mm256_loadu_si256((__m256i *)(pic_data + (y + 0) * pic_stride + 0));
    __m256i b = _mm256_loadu_si256((__m256i *)(ref_data + (y + 0) * ref_stride + 0  + ld_offset));

    __m256i b_shifted            = _mm256_shuffle_epi8     (b, shufmask1);
    __m256i b_lanes_reversed     = _mm256_permute4x64_epi64(b_shifted,   _MM_SHUFFLE(1, 0, 3, 2));
    __m256i b_data_transfered    = _mm256_blendv_epi8      (b_shifted, b_lanes_reversed, xchg_data_mask);
    __m256i b_epoled             = _mm256_shuffle_epi8     (b_data_transfered, epol_mask);

    __m256i curr_sads_ab         = _mm256_sad_epu8(a, b_epoled);

    avx_inc = _mm256_add_epi64(avx_inc, curr_sads_ab);
  }
  __m128i inchi = _mm256_extracti128_si256(avx_inc, 1);
  __m128i inclo = _mm256_castsi256_si128  (avx_inc);

  __m128i sum_1 = _mm_add_epi64    (inclo, inchi);
  __m128i sum_2 = _mm_shuffle_epi32(sum_1, _MM_SHUFFLE(1, 0, 3, 2));
  __m128i sad   = _mm_add_epi64    (sum_1, sum_2);

  return _mm_cvtsi128_si32(sad);
}

#endif
