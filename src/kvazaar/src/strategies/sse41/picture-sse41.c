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

#include "global.h"

#if COMPILE_INTEL_SSE41
#include "strategies/sse41/picture-sse41.h"
#include "strategies/sse41/reg_sad_pow2_widths-sse41.h"

#include <immintrin.h>
#include <stdlib.h>

#include "kvazaar.h"
#include "strategyselector.h"

uint32_t kvz_reg_sad_sse41(const kvz_pixel * const data1, const kvz_pixel * const data2,
                           const int32_t width, const int32_t height, const uint32_t stride1,
                           const uint32_t stride2)
{
  if (width == 0)
    return 0;
  if (width == 4)
    return reg_sad_w4(data1, data2, height, stride1, stride2);
  if (width == 8)
    return reg_sad_w8(data1, data2, height, stride1, stride2);
  if (width == 12)
    return reg_sad_w12(data1, data2, height, stride1, stride2);
  if (width == 16)
    return reg_sad_w16(data1, data2, height, stride1, stride2);
  if (width == 24)
    return reg_sad_w24(data1, data2, height, stride1, stride2);
  else
    return reg_sad_arbitrary(data1, data2, width, height, stride1, stride2);
}

static optimized_sad_func_ptr_t get_optimized_sad_sse41(int32_t width)
{
  if (width == 0)
    return reg_sad_w0;
  if (width == 4)
    return reg_sad_w4;
  if (width == 8)
    return reg_sad_w8;
  if (width == 12)
    return reg_sad_w12;
  if (width == 16)
    return reg_sad_w16;
  if (width == 24)
    return reg_sad_w24;
  else
    return NULL;
}

static uint32_t ver_sad_sse41(const kvz_pixel *pic_data, const kvz_pixel *ref_data,
                              int32_t width, int32_t height, uint32_t stride)
{
  if (width == 0)
    return 0;
  if (width == 4)
    return ver_sad_w4(pic_data, ref_data, height, stride);
  if (width == 8)
    return ver_sad_w8(pic_data, ref_data, height, stride);
  if (width == 12)
    return ver_sad_w12(pic_data, ref_data, height, stride);
  if (width == 16)
    return ver_sad_w16(pic_data, ref_data, height, stride);
  else
    return ver_sad_arbitrary(pic_data, ref_data, width, height, stride);
}

static uint32_t hor_sad_sse41_w32(const kvz_pixel *pic_data, const kvz_pixel *ref_data,
                                  int32_t height, uint32_t pic_stride, uint32_t ref_stride,
                                  uint32_t left, uint32_t right)
{
  const size_t vec_width       = 16;
  const uint32_t blkwidth_log2 = 5;
  const uint32_t left_eq_wid   = left  >> blkwidth_log2;
  const uint32_t right_eq_wid  = right >> blkwidth_log2;
  const int32_t  left_clamped  = left  - left_eq_wid;
  const int32_t  right_clamped = right - right_eq_wid;

  const int32_t height_twoline_groups = height & ~1;
  const int32_t height_residual_lines = height &  1;

  const __m128i zero       = _mm_setzero_si128();
  const __m128i vec_widths = _mm_set1_epi8((uint8_t)vec_width);
  const __m128i lefts      = _mm_set1_epi8((uint8_t)left_clamped);
  const __m128i rights     = _mm_set1_epi8((uint8_t)right_clamped);
  const __m128i nslo       = _mm_setr_epi8(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
  const __m128i nshi       = _mm_add_epi8 (nslo, vec_widths);

  const __m128i rightmost_good_idx = _mm_set1_epi8((uint8_t)((vec_width << 1) - right - 1));

  const __m128i epol_mask_right_lo = _mm_min_epi8  (nslo,            rightmost_good_idx);
  const __m128i epol_mask_right_hi = _mm_min_epi8  (nshi,            rightmost_good_idx);
  const __m128i epol_mask_lo       = _mm_max_epi8  (lefts,           epol_mask_right_lo);
  const __m128i epol_mask_hi       = _mm_max_epi8  (lefts,           epol_mask_right_hi);

  const __m128i is_left            = _mm_cmpeq_epi8(rights,          zero);
  const __m128i vecwid_for_left    = _mm_and_si128 (is_left,         vec_widths);
  const __m128i ns_for_shufmask    = _mm_or_si128  (nslo,            vecwid_for_left);

  const __m128i shufmask1_right    = _mm_add_epi8  (ns_for_shufmask, rights);
  const __m128i shufmask1          = _mm_sub_epi8  (shufmask1_right, lefts);

  const __m128i md2bimask          = _mm_cmpgt_epi8(vec_widths,      shufmask1);
  const __m128i move_d_to_b_imask  = _mm_or_si128  (is_left,         md2bimask);
  const __m128i move_b_to_d_mask   = _mm_cmpgt_epi8(lefts,           nslo);

  // If we're straddling the left border, start from the left border instead,
  // and if right border, end on the border
  const int32_t ld_offset = left - right;

  int32_t y;
  __m128i sse_inc = _mm_setzero_si128();
  for (y = 0; y < height_twoline_groups; y += 2) {
    __m128i a = _mm_loadu_si128((__m128i *)(pic_data + (y + 0) * pic_stride + 0));
    __m128i b = _mm_loadu_si128((__m128i *)(ref_data + (y + 0) * ref_stride + 0  + ld_offset));
    __m128i c = _mm_loadu_si128((__m128i *)(pic_data + (y + 0) * pic_stride + 16));
    __m128i d = _mm_loadu_si128((__m128i *)(ref_data + (y + 0) * ref_stride + 16 + ld_offset));
    __m128i e = _mm_loadu_si128((__m128i *)(pic_data + (y + 1) * pic_stride + 0));
    __m128i f = _mm_loadu_si128((__m128i *)(ref_data + (y + 1) * ref_stride + 0  + ld_offset));
    __m128i g = _mm_loadu_si128((__m128i *)(pic_data + (y + 1) * pic_stride + 16));
    __m128i h = _mm_loadu_si128((__m128i *)(ref_data + (y + 1) * ref_stride + 16 + ld_offset));

    __m128i b_shifted         = _mm_shuffle_epi8(b, shufmask1);
    __m128i d_shifted         = _mm_shuffle_epi8(d, shufmask1);
    __m128i f_shifted         = _mm_shuffle_epi8(f, shufmask1);
    __m128i h_shifted         = _mm_shuffle_epi8(h, shufmask1);

    // TODO: could these be optimized for two-operand efficiency? Only one of
    // these ever does useful work, the other should leave the vector untouched,
    // so could the first result be used in the second calculation or something?
    __m128i b_with_d_data     = _mm_blendv_epi8(d_shifted, b_shifted, move_d_to_b_imask);
    __m128i d_with_b_data     = _mm_blendv_epi8(d_shifted, b_shifted, move_b_to_d_mask);
    __m128i f_with_h_data     = _mm_blendv_epi8(h_shifted, f_shifted, move_d_to_b_imask);
    __m128i h_with_f_data     = _mm_blendv_epi8(h_shifted, f_shifted, move_b_to_d_mask);

    __m128i b_final           = _mm_shuffle_epi8(b_with_d_data, epol_mask_lo);
    __m128i d_final           = _mm_shuffle_epi8(d_with_b_data, epol_mask_hi);
    __m128i f_final           = _mm_shuffle_epi8(f_with_h_data, epol_mask_lo);
    __m128i h_final           = _mm_shuffle_epi8(h_with_f_data, epol_mask_hi);

    __m128i curr_sads_ab      = _mm_sad_epu8    (a, b_final);
    __m128i curr_sads_cd      = _mm_sad_epu8    (c, d_final);
    __m128i curr_sads_ef      = _mm_sad_epu8    (e, f_final);
    __m128i curr_sads_gh      = _mm_sad_epu8    (g, h_final);

    sse_inc = _mm_add_epi64(sse_inc, curr_sads_ab);
    sse_inc = _mm_add_epi64(sse_inc, curr_sads_cd);
    sse_inc = _mm_add_epi64(sse_inc, curr_sads_ef);
    sse_inc = _mm_add_epi64(sse_inc, curr_sads_gh);
  }
  if (height_residual_lines) {
    __m128i a = _mm_loadu_si128((__m128i *)(pic_data + (y + 0) * pic_stride + 0));
    __m128i b = _mm_loadu_si128((__m128i *)(ref_data + (y + 0) * ref_stride + 0  + ld_offset));
    __m128i c = _mm_loadu_si128((__m128i *)(pic_data + (y + 0) * pic_stride + 16));
    __m128i d = _mm_loadu_si128((__m128i *)(ref_data + (y + 0) * ref_stride + 16 + ld_offset));

    __m128i b_shifted         = _mm_shuffle_epi8(b, shufmask1);
    __m128i d_shifted         = _mm_shuffle_epi8(d, shufmask1);

    __m128i b_with_d_data     = _mm_blendv_epi8(d_shifted, b_shifted, move_d_to_b_imask);
    __m128i d_with_b_data     = _mm_blendv_epi8(d_shifted, b_shifted, move_b_to_d_mask);

    __m128i b_final           = _mm_shuffle_epi8(b_with_d_data, epol_mask_lo);
    __m128i d_final           = _mm_shuffle_epi8(d_with_b_data, epol_mask_hi);

    __m128i curr_sads_ab      = _mm_sad_epu8    (a, b_final);
    __m128i curr_sads_cd      = _mm_sad_epu8    (c, d_final);

    sse_inc = _mm_add_epi64(sse_inc, curr_sads_ab);
    sse_inc = _mm_add_epi64(sse_inc, curr_sads_cd);
  }
  __m128i sse_inc_2 = _mm_shuffle_epi32(sse_inc, _MM_SHUFFLE(1, 0, 3, 2));
  __m128i sad       = _mm_add_epi64    (sse_inc, sse_inc_2);
  return _mm_cvtsi128_si32(sad);
}

static uint32_t hor_sad_sse41(const kvz_pixel *pic_data, const kvz_pixel *ref_data,
                              int32_t width, int32_t height, uint32_t pic_stride,
                              uint32_t ref_stride, uint32_t left, uint32_t right)
{
  if (width == 4)
    return hor_sad_sse41_w4(pic_data, ref_data, height,
                            pic_stride, ref_stride, left, right);
  if (width == 8)
    return hor_sad_sse41_w8(pic_data, ref_data, height,
                            pic_stride, ref_stride, left, right);
  if (width == 16)
    return hor_sad_sse41_w16(pic_data, ref_data, height,
                             pic_stride, ref_stride, left, right);
  if (width == 32)
    return hor_sad_sse41_w32(pic_data, ref_data, height,
                             pic_stride, ref_stride, left, right);
  else
    return hor_sad_sse41_arbitrary(pic_data, ref_data, width, height,
                                   pic_stride, ref_stride, left, right);
}

#endif //COMPILE_INTEL_SSE41


int kvz_strategy_register_picture_sse41(void* opaque, uint8_t bitdepth) {
  bool success = true;
#if COMPILE_INTEL_SSE41
  if (bitdepth == 8){
    success &= kvz_strategyselector_register(opaque, "reg_sad", "sse41", 20, &kvz_reg_sad_sse41);
    success &= kvz_strategyselector_register(opaque, "get_optimized_sad", "sse41", 20, &get_optimized_sad_sse41);
    success &= kvz_strategyselector_register(opaque, "ver_sad", "sse41", 20, &ver_sad_sse41);
    success &= kvz_strategyselector_register(opaque, "hor_sad", "sse41", 20, &hor_sad_sse41);
  }
#endif
  return success;
}
