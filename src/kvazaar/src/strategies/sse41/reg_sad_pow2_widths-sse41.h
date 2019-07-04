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

#ifndef REG_SAD_POW2_WIDTHS_SSE41_H_
#define REG_SAD_POW2_WIDTHS_SSE41_H_

#include "kvazaar.h"
#include "strategies/missing-intel-intrinsics.h"
#include <immintrin.h>

static INLINE uint32_t reg_sad_w0(const kvz_pixel * const data1, const kvz_pixel * const data2,
                           const int32_t height, const uint32_t stride1,
                           const uint32_t stride2)
{
  return 0;
}

static INLINE uint32_t reg_sad_w4(const kvz_pixel * const data1, const kvz_pixel * const data2,
                           const int32_t height, const uint32_t stride1,
                           const uint32_t stride2)
{
  __m128i sse_inc = _mm_setzero_si128();
  int32_t y;

  const int32_t height_fourline_groups = height & ~3;
  const int32_t height_residual_lines  = height &  3;

  for (y = 0; y < height_fourline_groups; y += 4) {
    __m128i a = _mm_cvtsi32_si128(*(uint32_t *)(data1 + y * stride1));
    __m128i b = _mm_cvtsi32_si128(*(uint32_t *)(data2 + y * stride2));

    a = _mm_insert_epi32(a, *(const uint32_t *)(data1 + (y + 1) * stride1), 1);
    b = _mm_insert_epi32(b, *(const uint32_t *)(data2 + (y + 1) * stride2), 1);
    a = _mm_insert_epi32(a, *(const uint32_t *)(data1 + (y + 2) * stride1), 2);
    b = _mm_insert_epi32(b, *(const uint32_t *)(data2 + (y + 2) * stride2), 2);
    a = _mm_insert_epi32(a, *(const uint32_t *)(data1 + (y + 3) * stride1), 3);
    b = _mm_insert_epi32(b, *(const uint32_t *)(data2 + (y + 3) * stride2), 3);

    __m128i curr_sads = _mm_sad_epu8(a, b);
    sse_inc = _mm_add_epi64(sse_inc, curr_sads);
  }
  if (height_residual_lines) {
    for (; y < height; y++) {
      __m128i a = _mm_cvtsi32_si128(*(const uint32_t *)(data1 + y * stride1));
      __m128i b = _mm_cvtsi32_si128(*(const uint32_t *)(data2 + y * stride2));

      __m128i curr_sads = _mm_sad_epu8(a, b);
      sse_inc = _mm_add_epi64(sse_inc, curr_sads);
    }
  }
  __m128i sse_inc_2 = _mm_shuffle_epi32(sse_inc, _MM_SHUFFLE(1, 0, 3, 2));
  __m128i sad       = _mm_add_epi64    (sse_inc, sse_inc_2);

  return _mm_cvtsi128_si32(sad);
}

static INLINE uint32_t reg_sad_w8(const kvz_pixel * const data1, const kvz_pixel * const data2,
                           const int32_t height, const uint32_t stride1,
                           const uint32_t stride2)
{
  __m128i sse_inc = _mm_setzero_si128();
  int32_t y;

  const int32_t height_fourline_groups = height & ~3;
  const int32_t height_residual_lines  = height &  3;

  for (y = 0; y < height_fourline_groups; y += 4) {
    __m128d a_d = _mm_setzero_pd();
    __m128d b_d = _mm_setzero_pd();
    __m128d c_d = _mm_setzero_pd();
    __m128d d_d = _mm_setzero_pd();

    a_d = _mm_loadl_pd(a_d, (const double *)(data1 + (y + 0) * stride1));
    b_d = _mm_loadl_pd(b_d, (const double *)(data2 + (y + 0) * stride2));
    a_d = _mm_loadh_pd(a_d, (const double *)(data1 + (y + 1) * stride1));
    b_d = _mm_loadh_pd(b_d, (const double *)(data2 + (y + 1) * stride2));

    c_d = _mm_loadl_pd(c_d, (const double *)(data1 + (y + 2) * stride1));
    d_d = _mm_loadl_pd(d_d, (const double *)(data2 + (y + 2) * stride2));
    c_d = _mm_loadh_pd(c_d, (const double *)(data1 + (y + 3) * stride1));
    d_d = _mm_loadh_pd(d_d, (const double *)(data2 + (y + 3) * stride2));

    __m128i a = _mm_castpd_si128(a_d);
    __m128i b = _mm_castpd_si128(b_d);
    __m128i c = _mm_castpd_si128(c_d);
    __m128i d = _mm_castpd_si128(d_d);

    __m128i curr_sads_ab = _mm_sad_epu8(a, b);
    __m128i curr_sads_cd = _mm_sad_epu8(c, d);
    sse_inc = _mm_add_epi64(sse_inc, curr_sads_ab);
    sse_inc = _mm_add_epi64(sse_inc, curr_sads_cd);
  }
  if (height_residual_lines) {
    for (; y < height; y++) {
      __m128i a = _mm_loadl_epi64((__m128i *)(data1 + y * stride1));
      __m128i b = _mm_loadl_epi64((__m128i *)(data2 + y * stride2));

      __m128i curr_sads_ab = _mm_sad_epu8(a, b);
      sse_inc = _mm_add_epi64(sse_inc, curr_sads_ab);
    }
  }
  __m128i sse_inc_2 = _mm_shuffle_epi32(sse_inc, _MM_SHUFFLE(1, 0, 3, 2));
  __m128i sad       = _mm_add_epi64    (sse_inc, sse_inc_2);

  return _mm_cvtsi128_si32(sad);
}

static INLINE uint32_t reg_sad_w12(const kvz_pixel * const data1, const kvz_pixel * const data2,
                            const int32_t height, const uint32_t stride1,
                            const uint32_t stride2)
{
  __m128i sse_inc = _mm_setzero_si128();
  int32_t y;
  for (y = 0; y < height; y++) {
    __m128i a = _mm_loadu_si128((const __m128i *)(data1 + y * stride1));
    __m128i b = _mm_loadu_si128((const __m128i *)(data2 + y * stride2));

    __m128i b_masked  = _mm_blend_epi16(a, b, 0x3f);
    __m128i curr_sads = _mm_sad_epu8   (a, b_masked);
    sse_inc = _mm_add_epi64(sse_inc, curr_sads);
  }
  __m128i sse_inc_2 = _mm_shuffle_epi32(sse_inc, _MM_SHUFFLE(1, 0, 3, 2));
  __m128i sad       = _mm_add_epi64    (sse_inc, sse_inc_2);
  return _mm_cvtsi128_si32(sad);
}

static INLINE uint32_t reg_sad_w16(const kvz_pixel * const data1, const kvz_pixel * const data2,
                            const int32_t height, const uint32_t stride1,
                            const uint32_t stride2)
{
  __m128i sse_inc = _mm_setzero_si128();
  int32_t y;

  const int32_t height_fourline_groups = height & ~3;
  const int32_t height_residual_lines  = height &  3;

  for (y = 0; y < height_fourline_groups; y += 4) {
    __m128i a = _mm_loadu_si128((const __m128i *)(data1 + (y + 0) * stride1));
    __m128i b = _mm_loadu_si128((const __m128i *)(data2 + (y + 0) * stride2));
    __m128i c = _mm_loadu_si128((const __m128i *)(data1 + (y + 1) * stride1));
    __m128i d = _mm_loadu_si128((const __m128i *)(data2 + (y + 1) * stride2));
    __m128i e = _mm_loadu_si128((const __m128i *)(data1 + (y + 2) * stride1));
    __m128i f = _mm_loadu_si128((const __m128i *)(data2 + (y + 2) * stride2));
    __m128i g = _mm_loadu_si128((const __m128i *)(data1 + (y + 3) * stride1));
    __m128i h = _mm_loadu_si128((const __m128i *)(data2 + (y + 3) * stride2));

    __m128i curr_sads_ab = _mm_sad_epu8(a, b);
    __m128i curr_sads_cd = _mm_sad_epu8(c, d);
    __m128i curr_sads_ef = _mm_sad_epu8(e, f);
    __m128i curr_sads_gh = _mm_sad_epu8(g, h);

    sse_inc = _mm_add_epi64(sse_inc, curr_sads_ab);
    sse_inc = _mm_add_epi64(sse_inc, curr_sads_cd);
    sse_inc = _mm_add_epi64(sse_inc, curr_sads_ef);
    sse_inc = _mm_add_epi64(sse_inc, curr_sads_gh);
  }
  if (height_residual_lines) {
    for (; y < height; y++) {
      __m128i a = _mm_loadu_si128((const __m128i *)(data1 + (y + 0) * stride1));
      __m128i b = _mm_loadu_si128((const __m128i *)(data2 + (y + 0) * stride2));

      __m128i curr_sads = _mm_sad_epu8(a, b);
      sse_inc = _mm_add_epi64(sse_inc, curr_sads);
    }
  }

  __m128i sse_inc_2 = _mm_shuffle_epi32(sse_inc, _MM_SHUFFLE(1, 0, 3, 2));
  __m128i sad       = _mm_add_epi64    (sse_inc, sse_inc_2);
  return _mm_cvtsi128_si32(sad);
}

static INLINE uint32_t reg_sad_w24(const kvz_pixel * const data1, const kvz_pixel * const data2,
                            const int32_t height, const uint32_t stride1,
                            const uint32_t stride2)
{
  __m128i sse_inc = _mm_setzero_si128();
  int32_t y;

  const int32_t height_doublelines = height & ~1;
  const int32_t height_parity      = height &  1;

  for (y = 0; y < height_doublelines; y += 2) {
    __m128i a = _mm_loadu_si128((const __m128i *)(data1 + (y + 0) * stride1));
    __m128i b = _mm_loadu_si128((const __m128i *)(data2 + (y + 0) * stride2));
    __m128i c = _mm_loadu_si128((const __m128i *)(data1 + (y + 1) * stride1));
    __m128i d = _mm_loadu_si128((const __m128i *)(data2 + (y + 1) * stride2));

    __m128d e_d = _mm_setzero_pd();
    __m128d f_d = _mm_setzero_pd();

    e_d = _mm_loadl_pd(e_d, (const double *)(data1 + (y + 0) * stride1 + 16));
    f_d = _mm_loadl_pd(f_d, (const double *)(data2 + (y + 0) * stride2 + 16));
    e_d = _mm_loadh_pd(e_d, (const double *)(data1 + (y + 1) * stride1 + 16));
    f_d = _mm_loadh_pd(f_d, (const double *)(data2 + (y + 1) * stride2 + 16));

    __m128i e = _mm_castpd_si128(e_d);
    __m128i f = _mm_castpd_si128(f_d);

    __m128i curr_sads_1 = _mm_sad_epu8(a, b);
    __m128i curr_sads_2 = _mm_sad_epu8(c, d);
    __m128i curr_sads_3 = _mm_sad_epu8(e, f);

    sse_inc = _mm_add_epi64(sse_inc, curr_sads_1);
    sse_inc = _mm_add_epi64(sse_inc, curr_sads_2);
    sse_inc = _mm_add_epi64(sse_inc, curr_sads_3);
  }
  if (height_parity) {
    __m128i a = _mm_loadu_si128   ((const __m128i *)(data1 + y * stride1));
    __m128i b = _mm_loadu_si128   ((const __m128i *)(data2 + y * stride2));
    __m128i c = _mm_loadl_epi64   ((const __m128i *)(data1 + y * stride1 + 16));
    __m128i d = _mm_loadl_epi64   ((const __m128i *)(data2 + y * stride2 + 16));

    __m128i curr_sads_1 = _mm_sad_epu8(a, b);
    __m128i curr_sads_2 = _mm_sad_epu8(c, d);

    sse_inc = _mm_add_epi64(sse_inc, curr_sads_1);
    sse_inc = _mm_add_epi64(sse_inc, curr_sads_2);
  }
  __m128i sse_inc_2 = _mm_shuffle_epi32(sse_inc, _MM_SHUFFLE(1, 0, 3, 2));
  __m128i sad       = _mm_add_epi64    (sse_inc, sse_inc_2);
  return _mm_cvtsi128_si32(sad);
}

static INLINE uint32_t reg_sad_arbitrary(const kvz_pixel * const data1, const kvz_pixel * const data2,
                                  const int32_t width, const int32_t height, const uint32_t stride1,
                                  const uint32_t stride2)
{
  int32_t y, x;
  __m128i sse_inc = _mm_setzero_si128();
  
  // Bytes in block in 128-bit blocks per each scanline, and remainder
  const int32_t width_xmms             = width  & ~15;
  const int32_t width_residual_pixels  = width  &  15;

  const int32_t height_fourline_groups = height & ~3;
  const int32_t height_residual_lines  = height &  3;

  const __m128i rds    = _mm_set1_epi8 (width_residual_pixels);
  const __m128i ns     = _mm_setr_epi8 (0,  1,  2,  3,  4,  5,  6,  7,
                                        8,  9,  10, 11, 12, 13, 14, 15);
  const __m128i rdmask = _mm_cmpgt_epi8(rds, ns);

  for (x = 0; x < width_xmms; x += 16) {
    for (y = 0; y < height_fourline_groups; y += 4) {
      __m128i a = _mm_loadu_si128((const __m128i *)(data1 + (y + 0) * stride1 + x));
      __m128i b = _mm_loadu_si128((const __m128i *)(data2 + (y + 0) * stride2 + x));
      __m128i c = _mm_loadu_si128((const __m128i *)(data1 + (y + 1) * stride1 + x));
      __m128i d = _mm_loadu_si128((const __m128i *)(data2 + (y + 1) * stride2 + x));
      __m128i e = _mm_loadu_si128((const __m128i *)(data1 + (y + 2) * stride1 + x));
      __m128i f = _mm_loadu_si128((const __m128i *)(data2 + (y + 2) * stride2 + x));
      __m128i g = _mm_loadu_si128((const __m128i *)(data1 + (y + 3) * stride1 + x));
      __m128i h = _mm_loadu_si128((const __m128i *)(data2 + (y + 3) * stride2 + x));

      __m128i curr_sads_ab = _mm_sad_epu8(a, b);
      __m128i curr_sads_cd = _mm_sad_epu8(c, d);
      __m128i curr_sads_ef = _mm_sad_epu8(e, f);
      __m128i curr_sads_gh = _mm_sad_epu8(g, h);

      sse_inc = _mm_add_epi64(sse_inc, curr_sads_ab);
      sse_inc = _mm_add_epi64(sse_inc, curr_sads_cd);
      sse_inc = _mm_add_epi64(sse_inc, curr_sads_ef);
      sse_inc = _mm_add_epi64(sse_inc, curr_sads_gh);
    }
    if (height_residual_lines) {
      for (; y < height; y++) {
        __m128i a = _mm_loadu_si128((const __m128i *)(data1 + y * stride1 + x));
        __m128i b = _mm_loadu_si128((const __m128i *)(data2 + y * stride2 + x));

        __m128i curr_sads = _mm_sad_epu8(a, b);

        sse_inc = _mm_add_epi64(sse_inc, curr_sads);
      }
    }
  }

  if (width_residual_pixels) {
    for (y = 0; y < height_fourline_groups; y += 4) {
      __m128i a = _mm_loadu_si128((const __m128i *)(data1 + (y + 0) * stride1 + x));
      __m128i b = _mm_loadu_si128((const __m128i *)(data2 + (y + 0) * stride2 + x));
      __m128i c = _mm_loadu_si128((const __m128i *)(data1 + (y + 1) * stride1 + x));
      __m128i d = _mm_loadu_si128((const __m128i *)(data2 + (y + 1) * stride2 + x));
      __m128i e = _mm_loadu_si128((const __m128i *)(data1 + (y + 2) * stride1 + x));
      __m128i f = _mm_loadu_si128((const __m128i *)(data2 + (y + 2) * stride2 + x));
      __m128i g = _mm_loadu_si128((const __m128i *)(data1 + (y + 3) * stride1 + x));
      __m128i h = _mm_loadu_si128((const __m128i *)(data2 + (y + 3) * stride2 + x));

      __m128i b_masked     = _mm_blendv_epi8(a, b, rdmask);
      __m128i d_masked     = _mm_blendv_epi8(c, d, rdmask);
      __m128i f_masked     = _mm_blendv_epi8(e, f, rdmask);
      __m128i h_masked     = _mm_blendv_epi8(g, h, rdmask);

      __m128i curr_sads_ab = _mm_sad_epu8   (a, b_masked);
      __m128i curr_sads_cd = _mm_sad_epu8   (c, d_masked);
      __m128i curr_sads_ef = _mm_sad_epu8   (e, f_masked);
      __m128i curr_sads_gh = _mm_sad_epu8   (g, h_masked);

      sse_inc = _mm_add_epi64(sse_inc, curr_sads_ab);
      sse_inc = _mm_add_epi64(sse_inc, curr_sads_cd);
      sse_inc = _mm_add_epi64(sse_inc, curr_sads_ef);
      sse_inc = _mm_add_epi64(sse_inc, curr_sads_gh);
    }
    if (height_residual_lines) {
      for (; y < height; y++) {
        __m128i a = _mm_loadu_si128((const __m128i *)(data1 + y * stride1 + x));
        __m128i b = _mm_loadu_si128((const __m128i *)(data2 + y * stride2 + x));

        __m128i b_masked  = _mm_blendv_epi8(a, b, rdmask);
        __m128i curr_sads = _mm_sad_epu8   (a, b_masked);

        sse_inc = _mm_add_epi64(sse_inc, curr_sads);
      }
    }
  }
  __m128i sse_inc_2 = _mm_shuffle_epi32(sse_inc, _MM_SHUFFLE(1, 0, 3, 2));
  __m128i sad       = _mm_add_epi64    (sse_inc, sse_inc_2);

  return _mm_cvtsi128_si32(sad);
}

static uint32_t ver_sad_w4(const kvz_pixel *pic_data, const kvz_pixel *ref_data,
                           int32_t height, uint32_t stride)
{
  __m128i ref_row = _mm_set1_epi32(*(const uint32_t *)ref_data);
  __m128i sse_inc = _mm_setzero_si128();
  int32_t y;

  const int32_t height_fourline_groups = height & ~3;
  const int32_t height_residual_lines  = height &  3;

  for (y = 0; y < height_fourline_groups; y += 4) {
    __m128i a = _mm_cvtsi32_si128(*(uint32_t *)(pic_data + y * stride));

    a = _mm_insert_epi32(a, *(const uint32_t *)(pic_data + (y + 1) * stride), 1);
    a = _mm_insert_epi32(a, *(const uint32_t *)(pic_data + (y + 2) * stride), 2);
    a = _mm_insert_epi32(a, *(const uint32_t *)(pic_data + (y + 3) * stride), 3);

    __m128i curr_sads = _mm_sad_epu8(a, ref_row);
    sse_inc = _mm_add_epi64(sse_inc, curr_sads);
  }
  if (height_residual_lines) {
    // Only pick the last dword, because we're comparing single dwords (lines)
    ref_row = _mm_bsrli_si128(ref_row, 12);

    for (; y < height; y++) {
      __m128i a = _mm_cvtsi32_si128(*(const uint32_t *)(pic_data + y * stride));

      __m128i curr_sads = _mm_sad_epu8(a, ref_row);
      sse_inc = _mm_add_epi64(sse_inc, curr_sads);
    }
  }
  __m128i sse_inc_2 = _mm_shuffle_epi32(sse_inc, _MM_SHUFFLE(1, 0, 3, 2));
  __m128i sad       = _mm_add_epi64    (sse_inc, sse_inc_2);

  return _mm_cvtsi128_si32(sad);
}

static uint32_t ver_sad_w8(const kvz_pixel *pic_data, const kvz_pixel *ref_data,
                           int32_t height, uint32_t stride)
{
  const __m128i ref_row = _mm_set1_epi64x(*(const uint64_t *)ref_data);
  __m128i sse_inc = _mm_setzero_si128();
  int32_t y;

  const int32_t height_fourline_groups = height & ~3;
  const int32_t height_residual_lines  = height &  3;

  for (y = 0; y < height_fourline_groups; y += 4) {
    __m128d a_d = _mm_setzero_pd();
    __m128d c_d = _mm_setzero_pd();

    a_d = _mm_loadl_pd(a_d, (const double *)(pic_data + (y + 0) * stride));
    a_d = _mm_loadh_pd(a_d, (const double *)(pic_data + (y + 1) * stride));

    c_d = _mm_loadl_pd(c_d, (const double *)(pic_data + (y + 2) * stride));
    c_d = _mm_loadh_pd(c_d, (const double *)(pic_data + (y + 3) * stride));

    __m128i a = _mm_castpd_si128(a_d);
    __m128i c = _mm_castpd_si128(c_d);

    __m128i curr_sads_ab = _mm_sad_epu8(a, ref_row);
    __m128i curr_sads_cd = _mm_sad_epu8(c, ref_row);
    sse_inc = _mm_add_epi64(sse_inc, curr_sads_ab);
    sse_inc = _mm_add_epi64(sse_inc, curr_sads_cd);
  }
  if (height_residual_lines) {
    __m128i b = _mm_move_epi64(ref_row);

    for (; y < height; y++) {
      __m128i a = _mm_loadl_epi64((__m128i *)(pic_data + y * stride));

      __m128i curr_sads_ab = _mm_sad_epu8(a, b);
      sse_inc = _mm_add_epi64(sse_inc, curr_sads_ab);
    }
  }
  __m128i sse_inc_2 = _mm_shuffle_epi32(sse_inc, _MM_SHUFFLE(1, 0, 3, 2));
  __m128i sad       = _mm_add_epi64    (sse_inc, sse_inc_2);

  return _mm_cvtsi128_si32(sad);
}

static uint32_t ver_sad_w12(const kvz_pixel *pic_data, const kvz_pixel *ref_data,
                            int32_t height, uint32_t stride)
{
  const __m128i ref_row = _mm_loadu_si128((__m128i *)ref_data);
  __m128i sse_inc = _mm_setzero_si128();
  int32_t y;

  for (y = 0; y < height; y++) {
    __m128i a = _mm_loadu_si128((const __m128i *)(pic_data + y * stride));

    __m128i a_masked  = _mm_blend_epi16(ref_row, a, 0x3f);
    __m128i curr_sads = _mm_sad_epu8   (ref_row, a_masked);
    sse_inc = _mm_add_epi64(sse_inc, curr_sads);
  }
  __m128i sse_inc_2 = _mm_shuffle_epi32(sse_inc, _MM_SHUFFLE(1, 0, 3, 2));
  __m128i sad       = _mm_add_epi64    (sse_inc, sse_inc_2);
  return _mm_cvtsi128_si32(sad);
}

static uint32_t ver_sad_w16(const kvz_pixel *pic_data, const kvz_pixel *ref_data,
                            int32_t height, uint32_t stride)
{
  const __m128i ref_row = _mm_loadu_si128((__m128i *)ref_data);
  __m128i sse_inc       = _mm_setzero_si128();
  int32_t y;

  const int32_t height_fourline_groups = height & ~3;
  const int32_t height_residual_lines  = height &  3;

  for (y = 0; y < height_fourline_groups; y += 4) {
    __m128i pic_row_1   = _mm_loadu_si128((__m128i *)(pic_data + (y + 0) * stride));
    __m128i pic_row_2   = _mm_loadu_si128((__m128i *)(pic_data + (y + 1) * stride));
    __m128i pic_row_3   = _mm_loadu_si128((__m128i *)(pic_data + (y + 2) * stride));
    __m128i pic_row_4   = _mm_loadu_si128((__m128i *)(pic_data + (y + 3) * stride));

    __m128i curr_sads_1 = _mm_sad_epu8   (pic_row_1, ref_row);
    __m128i curr_sads_2 = _mm_sad_epu8   (pic_row_2, ref_row);
    __m128i curr_sads_3 = _mm_sad_epu8   (pic_row_3, ref_row);
    __m128i curr_sads_4 = _mm_sad_epu8   (pic_row_4, ref_row);

    sse_inc = _mm_add_epi64(sse_inc, curr_sads_1);
    sse_inc = _mm_add_epi64(sse_inc, curr_sads_2);
    sse_inc = _mm_add_epi64(sse_inc, curr_sads_3);
    sse_inc = _mm_add_epi64(sse_inc, curr_sads_4);
  }
  if (height_residual_lines) {
    for (; y < height; y++) {
      __m128i pic_row   = _mm_loadu_si128((__m128i *)(pic_data + (y + 0) * stride));
      __m128i curr_sads = _mm_sad_epu8   (pic_row, ref_row);

      sse_inc = _mm_add_epi64(sse_inc, curr_sads);
    }
  }
  __m128i sse_inc_2 = _mm_shuffle_epi32(sse_inc, _MM_SHUFFLE(1, 0, 3, 2));
  __m128i sad       = _mm_add_epi64    (sse_inc, sse_inc_2);

  return _mm_cvtsi128_si32(sad);
}

static uint32_t ver_sad_arbitrary(const kvz_pixel *pic_data, const kvz_pixel *ref_data,
                                  int32_t width, int32_t height, uint32_t stride)
{
  int32_t y, x;
  __m128i sse_inc = _mm_setzero_si128();

  // Bytes in block in 128-bit blocks per each scanline, and remainder
  const int32_t width_xmms             = width  & ~15;
  const int32_t width_residual_pixels  = width  &  15;

  const int32_t height_fourline_groups = height & ~3;
  const int32_t height_residual_lines  = height &  3;

  const __m128i rds    = _mm_set1_epi8 (width_residual_pixels);
  const __m128i ns     = _mm_setr_epi8 (0,  1,  2,  3,  4,  5,  6,  7,
                                        8,  9,  10, 11, 12, 13, 14, 15);
  const __m128i rdmask = _mm_cmpgt_epi8(rds, ns);

  for (x = 0; x < width_xmms; x += 16) {
    const __m128i ref_row = _mm_loadu_si128((__m128i *)(ref_data + x));
    for (y = 0; y < height_fourline_groups; y += 4) {
      __m128i a = _mm_loadu_si128((const __m128i *)(pic_data + (y + 0) * stride + x));
      __m128i c = _mm_loadu_si128((const __m128i *)(pic_data + (y + 1) * stride + x));
      __m128i e = _mm_loadu_si128((const __m128i *)(pic_data + (y + 2) * stride + x));
      __m128i g = _mm_loadu_si128((const __m128i *)(pic_data + (y + 3) * stride + x));

      __m128i curr_sads_ab = _mm_sad_epu8(ref_row, a);
      __m128i curr_sads_cd = _mm_sad_epu8(ref_row, c);
      __m128i curr_sads_ef = _mm_sad_epu8(ref_row, e);
      __m128i curr_sads_gh = _mm_sad_epu8(ref_row, g);

      sse_inc = _mm_add_epi64(sse_inc, curr_sads_ab);
      sse_inc = _mm_add_epi64(sse_inc, curr_sads_cd);
      sse_inc = _mm_add_epi64(sse_inc, curr_sads_ef);
      sse_inc = _mm_add_epi64(sse_inc, curr_sads_gh);
    }
    if (height_residual_lines) {
      for (; y < height; y++) {
        __m128i a = _mm_loadu_si128((const __m128i *)(pic_data + y * stride + x));

        __m128i curr_sads = _mm_sad_epu8(a, ref_row);

        sse_inc = _mm_add_epi64(sse_inc, curr_sads);
      }
    }
  }

  if (width_residual_pixels) {
    const __m128i ref_row = _mm_loadu_si128((__m128i *)(ref_data + x));
    for (y = 0; y < height_fourline_groups; y += 4) {
      __m128i a = _mm_loadu_si128((const __m128i *)(pic_data + (y + 0) * stride + x));
      __m128i c = _mm_loadu_si128((const __m128i *)(pic_data + (y + 1) * stride + x));
      __m128i e = _mm_loadu_si128((const __m128i *)(pic_data + (y + 2) * stride + x));
      __m128i g = _mm_loadu_si128((const __m128i *)(pic_data + (y + 3) * stride + x));

      __m128i a_masked     = _mm_blendv_epi8(ref_row, a, rdmask);
      __m128i c_masked     = _mm_blendv_epi8(ref_row, c, rdmask);
      __m128i e_masked     = _mm_blendv_epi8(ref_row, e, rdmask);
      __m128i g_masked     = _mm_blendv_epi8(ref_row, g, rdmask);

      __m128i curr_sads_ab = _mm_sad_epu8   (ref_row, a_masked);
      __m128i curr_sads_cd = _mm_sad_epu8   (ref_row, c_masked);
      __m128i curr_sads_ef = _mm_sad_epu8   (ref_row, e_masked);
      __m128i curr_sads_gh = _mm_sad_epu8   (ref_row, g_masked);

      sse_inc = _mm_add_epi64(sse_inc, curr_sads_ab);
      sse_inc = _mm_add_epi64(sse_inc, curr_sads_cd);
      sse_inc = _mm_add_epi64(sse_inc, curr_sads_ef);
      sse_inc = _mm_add_epi64(sse_inc, curr_sads_gh);
    }
    if (height_residual_lines) {
      for (; y < height; y++) {
        __m128i a = _mm_loadu_si128((const __m128i *)(pic_data + y * stride + x));

        __m128i a_masked  = _mm_blendv_epi8(ref_row, a, rdmask);
        __m128i curr_sads = _mm_sad_epu8   (ref_row, a_masked);

        sse_inc = _mm_add_epi64(sse_inc, curr_sads);
      }
    }
  }
  __m128i sse_inc_2 = _mm_shuffle_epi32(sse_inc, _MM_SHUFFLE(1, 0, 3, 2));
  __m128i sad       = _mm_add_epi64    (sse_inc, sse_inc_2);

  return _mm_cvtsi128_si32(sad);
}

static uint32_t hor_sad_sse41_w4(const kvz_pixel *pic_data, const kvz_pixel *ref_data,
                                 int32_t height, uint32_t pic_stride, uint32_t ref_stride,
                                 uint32_t left, uint32_t right)
{
  const int32_t right_border_idx = 3 - right;
  const int32_t border_idx       = left ? left : right_border_idx;

  const __m128i ns               = _mm_setr_epi8(0,  1,  2,  3,  4,  5,  6,  7,
                                                 8,  9,  10, 11, 12, 13, 14, 15);

  const int32_t border_idx_negative = border_idx >> 31;
  const int32_t leftoff             = border_idx_negative | left;

  // Dualword (ie. line) base indexes, ie. the edges the lines read will be
  // clamped towards
  const __m128i dwbaseids   = _mm_setr_epi8(0, 0, 0, 0, 4, 4, 4, 4,
                                            8, 8, 8, 8, 12, 12, 12, 12);

  __m128i right_border_idxs = _mm_set1_epi8((int8_t)right_border_idx);
  __m128i left_128          = _mm_set1_epi8((int8_t)left);

  right_border_idxs         = _mm_add_epi8 (right_border_idxs, dwbaseids);

  __m128i mask_right        = _mm_min_epi8 (ns,         right_border_idxs);
  __m128i mask1             = _mm_sub_epi8 (mask_right, left_128);

  const __m128i epol_mask   = _mm_max_epi8(mask1, dwbaseids);

  const int32_t height_fourline_groups = height & ~3;
  const int32_t height_residual_lines  = height &  3;

  __m128i sse_inc = _mm_setzero_si128();
  int32_t y;
  for (y = 0; y < height_fourline_groups; y += 4) {
    __m128i a = _mm_cvtsi32_si128(*(const uint32_t *)(pic_data + y * pic_stride));
    __m128i b = _mm_cvtsi32_si128(*(const uint32_t *)(ref_data + y * ref_stride + leftoff));

    a = _mm_insert_epi32(a, *(const uint32_t *)(pic_data + (y + 1) * pic_stride),           1);
    b = _mm_insert_epi32(b, *(const uint32_t *)(ref_data + (y + 1) * ref_stride + leftoff), 1);
    a = _mm_insert_epi32(a, *(const uint32_t *)(pic_data + (y + 2) * pic_stride),           2);
    b = _mm_insert_epi32(b, *(const uint32_t *)(ref_data + (y + 2) * ref_stride + leftoff), 2);
    a = _mm_insert_epi32(a, *(const uint32_t *)(pic_data + (y + 3) * pic_stride),           3);
    b = _mm_insert_epi32(b, *(const uint32_t *)(ref_data + (y + 3) * ref_stride + leftoff), 3);

    __m128i b_epol    = _mm_shuffle_epi8(b,       epol_mask);
    __m128i curr_sads = _mm_sad_epu8    (a,       b_epol);
            sse_inc   = _mm_add_epi64   (sse_inc, curr_sads);
  }
  if (height_residual_lines) {
    for (; y < height; y++) {
      __m128i a = _mm_cvtsi32_si128(*(const uint32_t *)(pic_data + y * pic_stride));
      __m128i b = _mm_cvtsi32_si128(*(const uint32_t *)(ref_data + y * ref_stride + leftoff));

      __m128i b_epol = _mm_shuffle_epi8(b, epol_mask);
      __m128i curr_sads = _mm_sad_epu8 (a, b_epol);
      sse_inc = _mm_add_epi64(sse_inc, curr_sads);
    }
  }
  __m128i sse_inc_2 = _mm_shuffle_epi32(sse_inc, _MM_SHUFFLE(1, 0, 3, 2));
  __m128i sad       = _mm_add_epi64    (sse_inc, sse_inc_2);

  return _mm_cvtsi128_si32(sad);
}

static uint32_t hor_sad_sse41_w8(const kvz_pixel *pic_data, const kvz_pixel *ref_data,
                                 int32_t height, uint32_t pic_stride, uint32_t ref_stride,
                                 uint32_t left, uint32_t right)
{
  // right is the number of overhanging pixels in the vector, so it has to be
  // handled this way to produce the index of last valid (border) pixel
  const int32_t right_border_idx = 7 - right;
  const int32_t border_idx       = left ? left : right_border_idx;

  const __m128i ns               = _mm_setr_epi8(0,  1,  2,  3,  4,  5,  6,  7,
                                                 8,  9,  10, 11, 12, 13, 14, 15);

  // Quadword (ie. line) base indexes, ie. the edges the lines read will be
  // clamped towards; higher qword (lower line) bytes tend towards 8 and lower
  // qword (higher line) bytes towards 0
  const __m128i qwbaseids   = _mm_setr_epi8(0, 0, 0, 0, 0, 0, 0, 0,
                                            8, 8, 8, 8, 8, 8, 8, 8);

  // Dirty hack alert! If right == block_width (ie. the entire vector is
  // outside the frame), move the block offset one pixel to the left (so
  // that the leftmost pixel in vector is actually the valid border pixel
  // from which we want to extrapolate), and use an epol mask that will
  // simply stretch the pixel all over the vector.
  //
  // To avoid a branch here:
  // The mask will be -1 (0xffffffff) for border_idx -1 and 0 for >= 0
  const int32_t border_idx_negative = border_idx >> 31;
  const int32_t leftoff             = border_idx_negative | left;

  __m128i right_border_idxs = _mm_set1_epi8((int8_t)right_border_idx);
  __m128i left_128          = _mm_set1_epi8((int8_t)left);

  right_border_idxs         = _mm_add_epi8 (right_border_idxs, qwbaseids);

  // If we're straddling the left border, right_border_idx is 7 and the first
  // operation does nothing. If right border, left is 0 and the second
  // operation does nothing.
  __m128i mask_right        = _mm_min_epi8 (ns,         right_border_idxs);
  __m128i mask1             = _mm_sub_epi8 (mask_right, left_128);

  // If right == 8 (we're completely outside the frame), right_border_idx is
  // -1 and so is mask1. Clamp negative values to qwbaseid and as discussed
  // earlier, adjust the load offset instead to load the "-1'st" pixels and
  // using qwbaseids as the shuffle mask, broadcast it all over the rows.
  const __m128i epol_mask = _mm_max_epi8(mask1, qwbaseids);

  const int32_t height_fourline_groups = height & ~3;
  const int32_t height_residual_lines  = height &  3;

  __m128i sse_inc = _mm_setzero_si128();
  int32_t y;
  for (y = 0; y < height_fourline_groups; y += 4) {
    __m128d a_d = _mm_setzero_pd();
    __m128d b_d = _mm_setzero_pd();
    __m128d c_d = _mm_setzero_pd();
    __m128d d_d = _mm_setzero_pd();

    a_d = _mm_loadl_pd(a_d, (const double *)(pic_data + (y + 0) * pic_stride));
    b_d = _mm_loadl_pd(b_d, (const double *)(ref_data + (y + 0) * ref_stride + leftoff));
    a_d = _mm_loadh_pd(a_d, (const double *)(pic_data + (y + 1) * pic_stride));
    b_d = _mm_loadh_pd(b_d, (const double *)(ref_data + (y + 1) * ref_stride + leftoff));

    c_d = _mm_loadl_pd(c_d, (const double *)(pic_data + (y + 2) * pic_stride));
    d_d = _mm_loadl_pd(d_d, (const double *)(ref_data + (y + 2) * ref_stride + leftoff));
    c_d = _mm_loadh_pd(c_d, (const double *)(pic_data + (y + 3) * pic_stride));
    d_d = _mm_loadh_pd(d_d, (const double *)(ref_data + (y + 3) * ref_stride + leftoff));

    __m128i a = _mm_castpd_si128(a_d);
    __m128i b = _mm_castpd_si128(b_d);
    __m128i c = _mm_castpd_si128(c_d);
    __m128i d = _mm_castpd_si128(d_d);

    __m128i b_epol = _mm_shuffle_epi8(b, epol_mask);
    __m128i d_epol = _mm_shuffle_epi8(d, epol_mask);

    __m128i curr_sads_ab = _mm_sad_epu8(a, b_epol);
    __m128i curr_sads_cd = _mm_sad_epu8(c, d_epol);
    sse_inc = _mm_add_epi64(sse_inc, curr_sads_ab);
    sse_inc = _mm_add_epi64(sse_inc, curr_sads_cd);
  }
  if (height_residual_lines) {
    for (; y < height; y++) {
      __m128i a = _mm_loadl_epi64((__m128i *)(pic_data + y * pic_stride));
      __m128i b = _mm_loadl_epi64((__m128i *)(ref_data + y * ref_stride + leftoff));

      __m128i b_epol = _mm_shuffle_epi8(b, epol_mask);

      __m128i curr_sads_ab = _mm_sad_epu8(a, b_epol);
      sse_inc = _mm_add_epi64(sse_inc, curr_sads_ab);
    }
  }
  __m128i sse_inc_2 = _mm_shuffle_epi32(sse_inc, _MM_SHUFFLE(1, 0, 3, 2));
  __m128i sad       = _mm_add_epi64    (sse_inc, sse_inc_2);
  return _mm_cvtsi128_si32(sad);
}

/*
 * left and right measure how many pixels of one horizontal scanline will be
 * outside either the left or the right screen border. For blocks straddling
 * the left border, read the scanlines starting from the left border instead,
 * and use the extrapolation mask to essentially move the pixels right while
 * copying the left border pixel to the vector positions that logically point
 * outside of the buffer.
 *
 * For blocks straddling the right border, just read over the right border,
 * and extrapolate all pixels beyond the border idx to copy the value of the
 * border pixel. An exception is right == width (leftmost reference pixel is
 * one place right from the right border, it's ugly because the pixel to
 * extrapolate from is located at relative X offset -1), abuse the left border
 * aligning functionality instead to actually read starting from the valid
 * border pixel, and use a suitable mask to fill all the other pixels with
 * that value.
 */
static uint32_t hor_sad_sse41_w16(const kvz_pixel *pic_data, const kvz_pixel *ref_data,
                                  int32_t height, uint32_t pic_stride, uint32_t ref_stride,
                                  const uint32_t left, const uint32_t right)
{
  // right is the number of overhanging pixels in the vector, so it has to be
  // handled this way to produce the index of last valid (border) pixel
  const int32_t right_border_idx = 15 - right;
  const int32_t border_idx       = left ? left : right_border_idx;

  const __m128i ns               = _mm_setr_epi8(0,  1,  2,  3,  4,  5,  6,  7,
                                                 8,  9,  10, 11, 12, 13, 14, 15);
  const __m128i zero             = _mm_setzero_si128();

  // Dirty hack alert! If right == block_width (ie. the entire vector is
  // outside the frame), move the block offset one pixel to the left (so
  // that the leftmost pixel in vector is actually the valid border pixel
  // from which we want to extrapolate), and use an epol mask that will
  // simply stretch the pixel all over the vector.
  //
  // To avoid a branch here:
  // The mask will be -1 (0xffffffff) for border_idx -1 and 0 for >= 0
  const int32_t border_idx_negative = border_idx >> 31;
  const int32_t leftoff             = border_idx_negative | left;

  __m128i right_border_idxs = _mm_set1_epi8((int8_t)right_border_idx);
  __m128i left_128          = _mm_set1_epi8((int8_t)left);

  // If we're straddling the left border, right_border_idx is 15 and the first
  // operation does nothing. If right border, left is 0 and the second
  // operation does nothing.
  __m128i mask_right        = _mm_min_epi8 (ns,         right_border_idxs);
  __m128i mask1             = _mm_sub_epi8 (mask_right, left_128);

  // If right == 16 (we're completely outside the frame), right_border_idx is
  // -1 and so is mask1. Clamp negative values to zero and as discussed
  // earlier, adjust the load offset instead to load the "-1'st" pixel and
  // using an all-zero shuffle mask, broadcast it all over the vector.
  const __m128i epol_mask = _mm_max_epi8(mask1, zero);

  const int32_t height_fourline_groups = height & ~3;
  const int32_t height_residual_lines  = height &  3;

  __m128i sse_inc = _mm_setzero_si128();
  int32_t y;
  for (y = 0; y < height_fourline_groups; y += 4) {
    __m128i a = _mm_loadu_si128((__m128i *)(pic_data + (y + 0) * pic_stride));
    __m128i b = _mm_loadu_si128((__m128i *)(ref_data + (y + 0) * ref_stride + leftoff));
    __m128i c = _mm_loadu_si128((__m128i *)(pic_data + (y + 1) * pic_stride));
    __m128i d = _mm_loadu_si128((__m128i *)(ref_data + (y + 1) * ref_stride + leftoff));
    __m128i e = _mm_loadu_si128((__m128i *)(pic_data + (y + 2) * pic_stride));
    __m128i f = _mm_loadu_si128((__m128i *)(ref_data + (y + 2) * ref_stride + leftoff));
    __m128i g = _mm_loadu_si128((__m128i *)(pic_data + (y + 3) * pic_stride));
    __m128i h = _mm_loadu_si128((__m128i *)(ref_data + (y + 3) * ref_stride + leftoff));

    __m128i b_epol = _mm_shuffle_epi8(b, epol_mask);
    __m128i d_epol = _mm_shuffle_epi8(d, epol_mask);
    __m128i f_epol = _mm_shuffle_epi8(f, epol_mask);
    __m128i h_epol = _mm_shuffle_epi8(h, epol_mask);

    __m128i curr_sads_ab = _mm_sad_epu8(a, b_epol);
    __m128i curr_sads_cd = _mm_sad_epu8(c, d_epol);
    __m128i curr_sads_ef = _mm_sad_epu8(e, f_epol);
    __m128i curr_sads_gh = _mm_sad_epu8(g, h_epol);

    sse_inc = _mm_add_epi64(sse_inc, curr_sads_ab);
    sse_inc = _mm_add_epi64(sse_inc, curr_sads_cd);
    sse_inc = _mm_add_epi64(sse_inc, curr_sads_ef);
    sse_inc = _mm_add_epi64(sse_inc, curr_sads_gh);
  }
  if (height_residual_lines) {
    for (; y < height; y++) {
      __m128i a = _mm_loadu_si128((__m128i *)(pic_data + (y + 0) * pic_stride));
      __m128i b = _mm_loadu_si128((__m128i *)(ref_data + (y + 0) * ref_stride + leftoff));
      __m128i b_epol = _mm_shuffle_epi8(b, epol_mask);
      __m128i curr_sads = _mm_sad_epu8(a, b_epol);
      sse_inc = _mm_add_epi64(sse_inc, curr_sads);
    }
  }
  __m128i sse_inc_2 = _mm_shuffle_epi32(sse_inc, _MM_SHUFFLE(1, 0, 3, 2));
  __m128i sad       = _mm_add_epi64    (sse_inc, sse_inc_2);
  return _mm_cvtsi128_si32(sad);
}

static INLINE uint32_t hor_sad_sse41_arbitrary(const kvz_pixel *pic_data, const kvz_pixel *ref_data,
                                               int32_t width, int32_t height, uint32_t pic_stride,
                                               uint32_t ref_stride, uint32_t left, uint32_t right)
{
  __m128i sse_inc = _mm_setzero_si128();

  const size_t vec_width = 16;
  const size_t vecwid_bitmask = 15;
  const size_t vec_width_log2 = 4;

  const int32_t height_fourline_groups = height & ~3;
  const int32_t height_residual_lines  = height &  3;

  const __m128i rights     = _mm_set1_epi8((uint8_t)right);
  const __m128i blk_widths = _mm_set1_epi8((uint8_t)width);
  const __m128i vec_widths = _mm_set1_epi8((uint8_t)vec_width);
  const __m128i nslo       = _mm_setr_epi8(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);

  uint32_t outside_vecs,  inside_vecs,  left_offset, is_left_bm;
  int32_t  outside_width, inside_width, border_off,  invec_lstart,
           invec_lend,    invec_linc;
  if (left) {
    outside_vecs  =    left                              >> vec_width_log2;
    inside_vecs   = (( width           + vecwid_bitmask) >> vec_width_log2) - outside_vecs;
    outside_width =    outside_vecs * vec_width;
    inside_width  =    inside_vecs  * vec_width;
    left_offset   =    left;
    border_off    =    left;
    invec_lstart  =    0;
    invec_lend    =    inside_vecs;
    invec_linc    =    1;
    is_left_bm    =    -1;
  } else {
    inside_vecs   =  ((width - right) + vecwid_bitmask)  >> vec_width_log2;
    outside_vecs  = (( width          + vecwid_bitmask)  >> vec_width_log2) - inside_vecs;
    outside_width =    outside_vecs * vec_width;
    inside_width  =    inside_vecs  * vec_width;
    left_offset   =    right - width;
    border_off    =    width - 1 - right;
    invec_lstart  =    inside_vecs - 1;
    invec_lend    =    -1;
    invec_linc    =    -1;
    is_left_bm    =    0;
  }
  left_offset &= vecwid_bitmask;

  const __m128i left_offsets = _mm_set1_epi8 ((uint8_t)left_offset);
  const __m128i is_left      = _mm_cmpeq_epi8(rights, _mm_setzero_si128());
  const __m128i vw_for_left  = _mm_and_si128 (is_left, vec_widths);

  // -x == (x ^ 0xff) + 1 = (x ^ 0xff) - 0xff. Also x == (x ^ 0x00) - 0x00.
  // in other words, calculate inverse of left_offsets if is_left is true.
  const __m128i offs_neg            = _mm_xor_si128 (left_offsets, is_left);
  const __m128i offs_for_sm1        = _mm_sub_epi8  (offs_neg,     is_left);

  const __m128i ns_for_sm1          = _mm_or_si128  (vw_for_left,  nslo);
  const __m128i shufmask1           = _mm_add_epi8  (ns_for_sm1,   offs_for_sm1);

  const __m128i mo2bmask_l          = _mm_cmpgt_epi8(left_offsets, nslo);
  const __m128i mo2bimask_l         = _mm_cmpeq_epi8(mo2bmask_l,   _mm_setzero_si128());
  const __m128i mo2bimask_r         = _mm_cmpgt_epi8(vec_widths,   shufmask1);
  const __m128i move_old_to_b_imask = _mm_blendv_epi8(mo2bimask_r, mo2bimask_l, is_left);

  const int32_t outvec_offset = (~is_left_bm) & inside_width;
  int32_t x, y;
  for (y = 0; y < height_fourline_groups; y += 4) {
    __m128i borderpx_vec_b = _mm_set1_epi8(ref_data[(int32_t)((y + 0) * ref_stride + border_off)]);
    __m128i borderpx_vec_d = _mm_set1_epi8(ref_data[(int32_t)((y + 1) * ref_stride + border_off)]);
    __m128i borderpx_vec_f = _mm_set1_epi8(ref_data[(int32_t)((y + 2) * ref_stride + border_off)]);
    __m128i borderpx_vec_h = _mm_set1_epi8(ref_data[(int32_t)((y + 3) * ref_stride + border_off)]);

    for (x = 0; x < outside_vecs; x++) {
      __m128i a = _mm_loadu_si128((__m128i *)(pic_data + x * vec_width + (y + 0) * pic_stride + outvec_offset));
      __m128i c = _mm_loadu_si128((__m128i *)(pic_data + x * vec_width + (y + 1) * pic_stride + outvec_offset));
      __m128i e = _mm_loadu_si128((__m128i *)(pic_data + x * vec_width + (y + 2) * pic_stride + outvec_offset));
      __m128i g = _mm_loadu_si128((__m128i *)(pic_data + x * vec_width + (y + 3) * pic_stride + outvec_offset));

      __m128i startoffs  = _mm_set1_epi8  ((x + inside_vecs) << vec_width_log2);
      __m128i ns         = _mm_add_epi8   (startoffs, nslo);

      // Unread imask is (is_left NOR unrd_imask_for_right), do the maths etc
      __m128i unrd_imask = _mm_cmpgt_epi8 (blk_widths, ns);
              unrd_imask = _mm_or_si128   (unrd_imask, is_left);
      __m128i unrd_mask  = _mm_cmpeq_epi8 (unrd_imask, _mm_setzero_si128());

      __m128i b_unread   = _mm_blendv_epi8(borderpx_vec_b, a, unrd_mask);
      __m128i d_unread   = _mm_blendv_epi8(borderpx_vec_d, c, unrd_mask);
      __m128i f_unread   = _mm_blendv_epi8(borderpx_vec_f, e, unrd_mask);
      __m128i h_unread   = _mm_blendv_epi8(borderpx_vec_h, g, unrd_mask);

      __m128i sad_ab     = _mm_sad_epu8   (a, b_unread);
      __m128i sad_cd     = _mm_sad_epu8   (c, d_unread);
      __m128i sad_ef     = _mm_sad_epu8   (e, f_unread);
      __m128i sad_gh     = _mm_sad_epu8   (g, h_unread);

      sse_inc = _mm_add_epi64(sse_inc, sad_ab);
      sse_inc = _mm_add_epi64(sse_inc, sad_cd);
      sse_inc = _mm_add_epi64(sse_inc, sad_ef);
      sse_inc = _mm_add_epi64(sse_inc, sad_gh);
    }
    int32_t a_off = outside_width & is_left_bm;
    int32_t leftoff_with_sign_neg = (left_offset ^ is_left_bm) - is_left_bm;

    __m128i old_b = borderpx_vec_b;
    __m128i old_d = borderpx_vec_d;
    __m128i old_f = borderpx_vec_f;
    __m128i old_h = borderpx_vec_h;

    for (x = invec_lstart; x != invec_lend; x += invec_linc) {
      __m128i a = _mm_loadu_si128((__m128i *)(pic_data + x * vec_width + (y + 0) * pic_stride + a_off));
      __m128i c = _mm_loadu_si128((__m128i *)(pic_data + x * vec_width + (y + 1) * pic_stride + a_off));
      __m128i e = _mm_loadu_si128((__m128i *)(pic_data + x * vec_width + (y + 2) * pic_stride + a_off));
      __m128i g = _mm_loadu_si128((__m128i *)(pic_data + x * vec_width + (y + 3) * pic_stride + a_off));
      __m128i b = _mm_loadu_si128((__m128i *)(ref_data + x * vec_width + (y + 0) * ref_stride + a_off - leftoff_with_sign_neg));
      __m128i d = _mm_loadu_si128((__m128i *)(ref_data + x * vec_width + (y + 1) * ref_stride + a_off - leftoff_with_sign_neg));
      __m128i f = _mm_loadu_si128((__m128i *)(ref_data + x * vec_width + (y + 2) * ref_stride + a_off - leftoff_with_sign_neg));
      __m128i h = _mm_loadu_si128((__m128i *)(ref_data + x * vec_width + (y + 3) * ref_stride + a_off - leftoff_with_sign_neg));

      __m128i b_shifted    = _mm_shuffle_epi8(b,     shufmask1);
      __m128i d_shifted    = _mm_shuffle_epi8(d,     shufmask1);
      __m128i f_shifted    = _mm_shuffle_epi8(f,     shufmask1);
      __m128i h_shifted    = _mm_shuffle_epi8(h,     shufmask1);

      __m128i b_with_old   = _mm_blendv_epi8 (old_b, b_shifted, move_old_to_b_imask);
      __m128i d_with_old   = _mm_blendv_epi8 (old_d, d_shifted, move_old_to_b_imask);
      __m128i f_with_old   = _mm_blendv_epi8 (old_f, f_shifted, move_old_to_b_imask);
      __m128i h_with_old   = _mm_blendv_epi8 (old_h, h_shifted, move_old_to_b_imask);

      uint8_t startoff     = (x << vec_width_log2) + a_off;
      __m128i startoffs    = _mm_set1_epi8   (startoff);
      __m128i curr_ns      = _mm_add_epi8    (startoffs,    nslo);
      __m128i unrd_imask   = _mm_cmpgt_epi8  (blk_widths,   curr_ns);
      __m128i unrd_mask    = _mm_cmpeq_epi8  (unrd_imask,   _mm_setzero_si128());

      __m128i b_unread     = _mm_blendv_epi8 (b_with_old,   a, unrd_mask);
      __m128i d_unread     = _mm_blendv_epi8 (d_with_old,   c, unrd_mask);
      __m128i f_unread     = _mm_blendv_epi8 (f_with_old,   e, unrd_mask);
      __m128i h_unread     = _mm_blendv_epi8 (h_with_old,   g, unrd_mask);

      old_b = b_shifted;
      old_d = d_shifted;
      old_f = f_shifted;
      old_h = h_shifted;

      __m128i sad_ab     = _mm_sad_epu8(a, b_unread);
      __m128i sad_cd     = _mm_sad_epu8(c, d_unread);
      __m128i sad_ef     = _mm_sad_epu8(e, f_unread);
      __m128i sad_gh     = _mm_sad_epu8(g, h_unread);

      sse_inc = _mm_add_epi64(sse_inc, sad_ab);
      sse_inc = _mm_add_epi64(sse_inc, sad_cd);
      sse_inc = _mm_add_epi64(sse_inc, sad_ef);
      sse_inc = _mm_add_epi64(sse_inc, sad_gh);
    }
  }
  if (height_residual_lines) {
    for (; y < height; y++) {
      __m128i borderpx_vec = _mm_set1_epi8(ref_data[(int32_t)((y + 0) * ref_stride + border_off)]);
      for (x = 0; x < outside_vecs; x++) {
        __m128i a = _mm_loadu_si128((__m128i *)(pic_data + x * vec_width + (y + 0) * pic_stride + outvec_offset));

        __m128i startoffs  = _mm_set1_epi8  ((x + inside_vecs) << vec_width_log2);
        __m128i ns         = _mm_add_epi8   (startoffs, nslo);

        // Unread imask is (is_left NOR unrd_imask_for_right), do the maths etc
        __m128i unrd_imask = _mm_cmpgt_epi8 (blk_widths, ns);
                unrd_imask = _mm_or_si128   (unrd_imask, is_left);
        __m128i unrd_mask  = _mm_cmpeq_epi8 (unrd_imask, _mm_setzero_si128());
        __m128i b_unread   = _mm_blendv_epi8(borderpx_vec, a, unrd_mask);

        __m128i sad_ab     = _mm_sad_epu8   (a, b_unread);
        sse_inc = _mm_add_epi64(sse_inc, sad_ab);
      }
      int32_t a_off = outside_width & is_left_bm;
      int32_t leftoff_with_sign_neg = (left_offset ^ is_left_bm) - is_left_bm;

      __m128i old_b = borderpx_vec;
      for (x = invec_lstart; x != invec_lend; x += invec_linc) {
        __m128i a = _mm_loadu_si128((__m128i *)(pic_data + x * vec_width + (y + 0) * pic_stride + a_off));
        __m128i b = _mm_loadu_si128((__m128i *)(ref_data + x * vec_width + (y + 0) * ref_stride + a_off - leftoff_with_sign_neg));

        __m128i b_shifted    = _mm_shuffle_epi8(b,     shufmask1);
        __m128i b_with_old   = _mm_blendv_epi8 (old_b, b_shifted, move_old_to_b_imask);

        uint8_t startoff     = (x << vec_width_log2) + a_off;
        __m128i startoffs    = _mm_set1_epi8   (startoff);
        __m128i curr_ns      = _mm_add_epi8    (startoffs,    nslo);
        __m128i unrd_imask   = _mm_cmpgt_epi8  (blk_widths,   curr_ns);
        __m128i unrd_mask    = _mm_cmpeq_epi8  (unrd_imask,   _mm_setzero_si128());
        __m128i b_unread     = _mm_blendv_epi8 (b_with_old,   a, unrd_mask);

        old_b = b_shifted;

        __m128i sad_ab     = _mm_sad_epu8(a, b_unread);
        sse_inc = _mm_add_epi64(sse_inc, sad_ab);
      }
    }
  }
  __m128i sse_inc_2 = _mm_shuffle_epi32(sse_inc, _MM_SHUFFLE(1, 0, 3, 2));
  __m128i sad       = _mm_add_epi64    (sse_inc, sse_inc_2);
  return _mm_cvtsi128_si32(sad);
}

#endif
