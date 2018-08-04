/*
 * Copyright (c) 2018, Alliance for Open Media. All rights reserved
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */

#include <smmintrin.h>  // SSE4.1
#include <immintrin.h>  // AVX2

#include <assert.h>

#include "aom/aom_integer.h"
#include "aom_ports/mem.h"
#include "aom_dsp/aom_dsp_common.h"

#include "aom_dsp/x86/synonyms.h"
#include "aom_dsp/x86/synonyms_avx2.h"
#include "aom_dsp/x86/blend_mask_sse4.h"

#include "config/aom_dsp_rtcd.h"

static INLINE void blend_a64_d16_mask_w16_avx2(
    uint8_t *dst, const CONV_BUF_TYPE *src0, const CONV_BUF_TYPE *src1,
    const __m256i *m0, const __m256i *v_round_offset, const __m256i *v_maxval,
    int shift) {
  const __m256i max_minus_m0 = _mm256_sub_epi16(*v_maxval, *m0);
  const __m256i s0_0 = yy_loadu_256(src0);
  const __m256i s1_0 = yy_loadu_256(src1);
  __m256i res0_lo = _mm256_madd_epi16(_mm256_unpacklo_epi16(s0_0, s1_0),
                                      _mm256_unpacklo_epi16(*m0, max_minus_m0));
  __m256i res0_hi = _mm256_madd_epi16(_mm256_unpackhi_epi16(s0_0, s1_0),
                                      _mm256_unpackhi_epi16(*m0, max_minus_m0));
  res0_lo =
      _mm256_srai_epi32(_mm256_sub_epi32(res0_lo, *v_round_offset), shift);
  res0_hi =
      _mm256_srai_epi32(_mm256_sub_epi32(res0_hi, *v_round_offset), shift);
  const __m256i res0 = _mm256_packs_epi32(res0_lo, res0_hi);
  __m256i res = _mm256_packus_epi16(res0, res0);
  res = _mm256_permute4x64_epi64(res, 0xd8);
  _mm_storeu_si128((__m128i *)(dst), _mm256_castsi256_si128(res));
}

static INLINE void blend_a64_d16_mask_w32_avx2(
    uint8_t *dst, const CONV_BUF_TYPE *src0, const CONV_BUF_TYPE *src1,
    const __m256i *m0, const __m256i *m1, const __m256i *v_round_offset,
    const __m256i *v_maxval, int shift) {
  const __m256i max_minus_m0 = _mm256_sub_epi16(*v_maxval, *m0);
  const __m256i max_minus_m1 = _mm256_sub_epi16(*v_maxval, *m1);
  const __m256i s0_0 = yy_loadu_256(src0);
  const __m256i s0_1 = yy_loadu_256(src0 + 16);
  const __m256i s1_0 = yy_loadu_256(src1);
  const __m256i s1_1 = yy_loadu_256(src1 + 16);
  __m256i res0_lo = _mm256_madd_epi16(_mm256_unpacklo_epi16(s0_0, s1_0),
                                      _mm256_unpacklo_epi16(*m0, max_minus_m0));
  __m256i res0_hi = _mm256_madd_epi16(_mm256_unpackhi_epi16(s0_0, s1_0),
                                      _mm256_unpackhi_epi16(*m0, max_minus_m0));
  __m256i res1_lo = _mm256_madd_epi16(_mm256_unpacklo_epi16(s0_1, s1_1),
                                      _mm256_unpacklo_epi16(*m1, max_minus_m1));
  __m256i res1_hi = _mm256_madd_epi16(_mm256_unpackhi_epi16(s0_1, s1_1),
                                      _mm256_unpackhi_epi16(*m1, max_minus_m1));
  res0_lo =
      _mm256_srai_epi32(_mm256_sub_epi32(res0_lo, *v_round_offset), shift);
  res0_hi =
      _mm256_srai_epi32(_mm256_sub_epi32(res0_hi, *v_round_offset), shift);
  res1_lo =
      _mm256_srai_epi32(_mm256_sub_epi32(res1_lo, *v_round_offset), shift);
  res1_hi =
      _mm256_srai_epi32(_mm256_sub_epi32(res1_hi, *v_round_offset), shift);
  const __m256i res0 = _mm256_packs_epi32(res0_lo, res0_hi);
  const __m256i res1 = _mm256_packs_epi32(res1_lo, res1_hi);
  __m256i res = _mm256_packus_epi16(res0, res1);
  res = _mm256_permute4x64_epi64(res, 0xd8);
  _mm256_storeu_si256((__m256i *)(dst), res);
}

static INLINE void lowbd_blend_a64_d16_mask_subw0_subh0_w16_avx2(
    uint8_t *dst, uint32_t dst_stride, const CONV_BUF_TYPE *src0,
    uint32_t src0_stride, const CONV_BUF_TYPE *src1, uint32_t src1_stride,
    const uint8_t *mask, uint32_t mask_stride, int h,
    const __m256i *round_offset, int shift) {
  const __m256i v_maxval = _mm256_set1_epi16(AOM_BLEND_A64_MAX_ALPHA);
  for (int i = 0; i < h; ++i) {
    const __m128i m = xx_loadu_128(mask);
    const __m256i m0 = _mm256_cvtepu8_epi16(m);

    blend_a64_d16_mask_w16_avx2(dst, src0, src1, &m0, round_offset, &v_maxval,
                                shift);
    mask += mask_stride;
    dst += dst_stride;
    src0 += src0_stride;
    src1 += src1_stride;
  }
}

static INLINE void lowbd_blend_a64_d16_mask_subw0_subh0_w32_avx2(
    uint8_t *dst, uint32_t dst_stride, const CONV_BUF_TYPE *src0,
    uint32_t src0_stride, const CONV_BUF_TYPE *src1, uint32_t src1_stride,
    const uint8_t *mask, uint32_t mask_stride, int h, int w,
    const __m256i *round_offset, int shift) {
  const __m256i v_maxval = _mm256_set1_epi16(AOM_BLEND_A64_MAX_ALPHA);
  for (int i = 0; i < h; ++i) {
    for (int j = 0; j < w; j += 32) {
      const __m256i m = yy_loadu_256(mask + j);
      const __m256i m0 = _mm256_cvtepu8_epi16(_mm256_castsi256_si128(m));
      const __m256i m1 = _mm256_cvtepu8_epi16(_mm256_extracti128_si256(m, 1));

      blend_a64_d16_mask_w32_avx2(dst + j, src0 + j, src1 + j, &m0, &m1,
                                  round_offset, &v_maxval, shift);
    }
    mask += mask_stride;
    dst += dst_stride;
    src0 += src0_stride;
    src1 += src1_stride;
  }
}

static INLINE void lowbd_blend_a64_d16_mask_subw1_subh1_w16_avx2(
    uint8_t *dst, uint32_t dst_stride, const CONV_BUF_TYPE *src0,
    uint32_t src0_stride, const CONV_BUF_TYPE *src1, uint32_t src1_stride,
    const uint8_t *mask, uint32_t mask_stride, int h,
    const __m256i *round_offset, int shift) {
  const __m256i v_maxval = _mm256_set1_epi16(AOM_BLEND_A64_MAX_ALPHA);
  const __m256i one_b = _mm256_set1_epi8(1);
  const __m256i two_w = _mm256_set1_epi16(2);
  for (int i = 0; i < h; ++i) {
    const __m256i m_i00 = yy_loadu_256(mask);
    const __m256i m_i10 = yy_loadu_256(mask + mask_stride);

    const __m256i m0_ac = _mm256_adds_epu8(m_i00, m_i10);
    const __m256i m0_acbd = _mm256_maddubs_epi16(m0_ac, one_b);
    const __m256i m0 = _mm256_srli_epi16(_mm256_add_epi16(m0_acbd, two_w), 2);

    blend_a64_d16_mask_w16_avx2(dst, src0, src1, &m0, round_offset, &v_maxval,
                                shift);
    mask += mask_stride << 1;
    dst += dst_stride;
    src0 += src0_stride;
    src1 += src1_stride;
  }
}

static INLINE void lowbd_blend_a64_d16_mask_subw1_subh1_w32_avx2(
    uint8_t *dst, uint32_t dst_stride, const CONV_BUF_TYPE *src0,
    uint32_t src0_stride, const CONV_BUF_TYPE *src1, uint32_t src1_stride,
    const uint8_t *mask, uint32_t mask_stride, int h, int w,
    const __m256i *round_offset, int shift) {
  const __m256i v_maxval = _mm256_set1_epi16(AOM_BLEND_A64_MAX_ALPHA);
  const __m256i one_b = _mm256_set1_epi8(1);
  const __m256i two_w = _mm256_set1_epi16(2);
  for (int i = 0; i < h; ++i) {
    for (int j = 0; j < w; j += 32) {
      const __m256i m_i00 = yy_loadu_256(mask + 2 * j);
      const __m256i m_i01 = yy_loadu_256(mask + 2 * j + 32);
      const __m256i m_i10 = yy_loadu_256(mask + mask_stride + 2 * j);
      const __m256i m_i11 = yy_loadu_256(mask + mask_stride + 2 * j + 32);

      const __m256i m0_ac = _mm256_adds_epu8(m_i00, m_i10);
      const __m256i m1_ac = _mm256_adds_epu8(m_i01, m_i11);
      const __m256i m0_acbd = _mm256_maddubs_epi16(m0_ac, one_b);
      const __m256i m1_acbd = _mm256_maddubs_epi16(m1_ac, one_b);
      const __m256i m0 = _mm256_srli_epi16(_mm256_add_epi16(m0_acbd, two_w), 2);
      const __m256i m1 = _mm256_srli_epi16(_mm256_add_epi16(m1_acbd, two_w), 2);

      blend_a64_d16_mask_w32_avx2(dst + j, src0 + j, src1 + j, &m0, &m1,
                                  round_offset, &v_maxval, shift);
    }
    mask += mask_stride << 1;
    dst += dst_stride;
    src0 += src0_stride;
    src1 += src1_stride;
  }
}

static INLINE void lowbd_blend_a64_d16_mask_subw1_subh0_w16_avx2(
    uint8_t *dst, uint32_t dst_stride, const CONV_BUF_TYPE *src0,
    uint32_t src0_stride, const CONV_BUF_TYPE *src1, uint32_t src1_stride,
    const uint8_t *mask, uint32_t mask_stride, int h, int w,
    const __m256i *round_offset, int shift) {
  const __m256i v_maxval = _mm256_set1_epi16(AOM_BLEND_A64_MAX_ALPHA);
  const __m256i one_b = _mm256_set1_epi8(1);
  const __m256i zeros = _mm256_setzero_si256();
  for (int i = 0; i < h; ++i) {
    for (int j = 0; j < w; j += 16) {
      const __m256i m_i00 = yy_loadu_256(mask + 2 * j);
      const __m256i m0_ac = _mm256_maddubs_epi16(m_i00, one_b);
      const __m256i m0 = _mm256_avg_epu16(m0_ac, zeros);

      blend_a64_d16_mask_w16_avx2(dst + j, src0 + j, src1 + j, &m0,
                                  round_offset, &v_maxval, shift);
    }
    mask += mask_stride;
    dst += dst_stride;
    src0 += src0_stride;
    src1 += src1_stride;
  }
}

static INLINE void lowbd_blend_a64_d16_mask_subw1_subh0_w32_avx2(
    uint8_t *dst, uint32_t dst_stride, const CONV_BUF_TYPE *src0,
    uint32_t src0_stride, const CONV_BUF_TYPE *src1, uint32_t src1_stride,
    const uint8_t *mask, uint32_t mask_stride, int h, int w,
    const __m256i *round_offset, int shift) {
  const __m256i v_maxval = _mm256_set1_epi16(AOM_BLEND_A64_MAX_ALPHA);
  const __m256i one_b = _mm256_set1_epi8(1);
  const __m256i zeros = _mm256_setzero_si256();
  for (int i = 0; i < h; ++i) {
    for (int j = 0; j < w; j += 32) {
      const __m256i m_i00 = yy_loadu_256(mask + 2 * j);
      const __m256i m_i01 = yy_loadu_256(mask + 2 * j + 32);
      const __m256i m0_ac = _mm256_maddubs_epi16(m_i00, one_b);
      const __m256i m1_ac = _mm256_maddubs_epi16(m_i01, one_b);
      const __m256i m0 = _mm256_avg_epu16(m0_ac, zeros);
      const __m256i m1 = _mm256_avg_epu16(m1_ac, zeros);

      blend_a64_d16_mask_w32_avx2(dst + j, src0 + j, src1 + j, &m0, &m1,
                                  round_offset, &v_maxval, shift);
    }
    mask += mask_stride;
    dst += dst_stride;
    src0 += src0_stride;
    src1 += src1_stride;
  }
}

static INLINE void lowbd_blend_a64_d16_mask_subw0_subh1_w16_avx2(
    uint8_t *dst, uint32_t dst_stride, const CONV_BUF_TYPE *src0,
    uint32_t src0_stride, const CONV_BUF_TYPE *src1, uint32_t src1_stride,
    const uint8_t *mask, uint32_t mask_stride, int h, int w,
    const __m256i *round_offset, int shift) {
  const __m256i v_maxval = _mm256_set1_epi16(AOM_BLEND_A64_MAX_ALPHA);
  const __m128i zeros = _mm_setzero_si128();
  for (int i = 0; i < h; ++i) {
    for (int j = 0; j < w; j += 16) {
      const __m128i m_i00 = xx_loadu_128(mask + j);
      const __m128i m_i10 = xx_loadu_128(mask + mask_stride + j);

      const __m128i m_ac = _mm_avg_epu8(_mm_adds_epu8(m_i00, m_i10), zeros);
      const __m256i m0 = _mm256_cvtepu8_epi16(m_ac);

      blend_a64_d16_mask_w16_avx2(dst + j, src0 + j, src1 + j, &m0,
                                  round_offset, &v_maxval, shift);
    }
    mask += mask_stride << 1;
    dst += dst_stride;
    src0 += src0_stride;
    src1 += src1_stride;
  }
}

static INLINE void lowbd_blend_a64_d16_mask_subw0_subh1_w32_avx2(
    uint8_t *dst, uint32_t dst_stride, const CONV_BUF_TYPE *src0,
    uint32_t src0_stride, const CONV_BUF_TYPE *src1, uint32_t src1_stride,
    const uint8_t *mask, uint32_t mask_stride, int h, int w,
    const __m256i *round_offset, int shift) {
  const __m256i v_maxval = _mm256_set1_epi16(AOM_BLEND_A64_MAX_ALPHA);
  const __m256i zeros = _mm256_setzero_si256();
  for (int i = 0; i < h; ++i) {
    for (int j = 0; j < w; j += 32) {
      const __m256i m_i00 = yy_loadu_256(mask + j);
      const __m256i m_i10 = yy_loadu_256(mask + mask_stride + j);

      const __m256i m_ac =
          _mm256_avg_epu8(_mm256_adds_epu8(m_i00, m_i10), zeros);
      const __m256i m0 = _mm256_cvtepu8_epi16(_mm256_castsi256_si128(m_ac));
      const __m256i m1 =
          _mm256_cvtepu8_epi16(_mm256_extracti128_si256(m_ac, 1));

      blend_a64_d16_mask_w32_avx2(dst + j, src0 + j, src1 + j, &m0, &m1,
                                  round_offset, &v_maxval, shift);
    }
    mask += mask_stride << 1;
    dst += dst_stride;
    src0 += src0_stride;
    src1 += src1_stride;
  }
}

void aom_lowbd_blend_a64_d16_mask_avx2(
    uint8_t *dst, uint32_t dst_stride, const CONV_BUF_TYPE *src0,
    uint32_t src0_stride, const CONV_BUF_TYPE *src1, uint32_t src1_stride,
    const uint8_t *mask, uint32_t mask_stride, int w, int h, int subw, int subh,
    ConvolveParams *conv_params) {
  const int bd = 8;
  const int round_bits =
      2 * FILTER_BITS - conv_params->round_0 - conv_params->round_1;

  const int round_offset =
      ((1 << (round_bits + bd)) + (1 << (round_bits + bd - 1)) -
       (1 << (round_bits - 1)))
      << AOM_BLEND_A64_ROUND_BITS;

  const int shift = round_bits + AOM_BLEND_A64_ROUND_BITS;
  assert(IMPLIES((void *)src0 == dst, src0_stride == dst_stride));
  assert(IMPLIES((void *)src1 == dst, src1_stride == dst_stride));

  assert(h >= 4);
  assert(w >= 4);
  assert(IS_POWER_OF_TWO(h));
  assert(IS_POWER_OF_TWO(w));
  const __m128i v_round_offset = _mm_set1_epi32(round_offset);
  const __m256i y_round_offset = _mm256_set1_epi32(round_offset);

  if (subw == 0 && subh == 0) {
    switch (w) {
      case 4:
        aom_lowbd_blend_a64_d16_mask_subw0_subh0_w4_sse4_1(
            dst, dst_stride, src0, src0_stride, src1, src1_stride, mask,
            mask_stride, h, &v_round_offset, shift);
        break;
      case 8:
        aom_lowbd_blend_a64_d16_mask_subw0_subh0_w8_sse4_1(
            dst, dst_stride, src0, src0_stride, src1, src1_stride, mask,
            mask_stride, h, &v_round_offset, shift);
        break;
      case 16:
        lowbd_blend_a64_d16_mask_subw0_subh0_w16_avx2(
            dst, dst_stride, src0, src0_stride, src1, src1_stride, mask,
            mask_stride, h, &y_round_offset, shift);
        break;
      default:
        lowbd_blend_a64_d16_mask_subw0_subh0_w32_avx2(
            dst, dst_stride, src0, src0_stride, src1, src1_stride, mask,
            mask_stride, h, w, &y_round_offset, shift);
        break;
    }
  } else if (subw == 1 && subh == 1) {
    switch (w) {
      case 4:
        aom_lowbd_blend_a64_d16_mask_subw1_subh1_w4_sse4_1(
            dst, dst_stride, src0, src0_stride, src1, src1_stride, mask,
            mask_stride, h, &v_round_offset, shift);
        break;
      case 8:
        aom_lowbd_blend_a64_d16_mask_subw1_subh1_w8_sse4_1(
            dst, dst_stride, src0, src0_stride, src1, src1_stride, mask,
            mask_stride, h, &v_round_offset, shift);
        break;
      case 16:
        lowbd_blend_a64_d16_mask_subw1_subh1_w16_avx2(
            dst, dst_stride, src0, src0_stride, src1, src1_stride, mask,
            mask_stride, h, &y_round_offset, shift);
        break;
      default:
        lowbd_blend_a64_d16_mask_subw1_subh1_w32_avx2(
            dst, dst_stride, src0, src0_stride, src1, src1_stride, mask,
            mask_stride, h, w, &y_round_offset, shift);
        break;
    }
  } else if (subw == 1 && subh == 0) {
    switch (w) {
      case 4:
        aom_lowbd_blend_a64_d16_mask_subw1_subh0_w4_sse4_1(
            dst, dst_stride, src0, src0_stride, src1, src1_stride, mask,
            mask_stride, h, &v_round_offset, shift);
        break;
      case 8:
        aom_lowbd_blend_a64_d16_mask_subw1_subh0_w8_sse4_1(
            dst, dst_stride, src0, src0_stride, src1, src1_stride, mask,
            mask_stride, h, &v_round_offset, shift);
        break;
      case 16:
        lowbd_blend_a64_d16_mask_subw1_subh0_w16_avx2(
            dst, dst_stride, src0, src0_stride, src1, src1_stride, mask,
            mask_stride, h, w, &y_round_offset, shift);
        break;
      default:
        lowbd_blend_a64_d16_mask_subw1_subh0_w32_avx2(
            dst, dst_stride, src0, src0_stride, src1, src1_stride, mask,
            mask_stride, h, w, &y_round_offset, shift);
        break;
    }
  } else {
    switch (w) {
      case 4:
        aom_lowbd_blend_a64_d16_mask_subw0_subh1_w4_sse4_1(
            dst, dst_stride, src0, src0_stride, src1, src1_stride, mask,
            mask_stride, h, &v_round_offset, shift);
        break;
      case 8:
        aom_lowbd_blend_a64_d16_mask_subw0_subh1_w8_sse4_1(
            dst, dst_stride, src0, src0_stride, src1, src1_stride, mask,
            mask_stride, h, &v_round_offset, shift);
        break;
      case 16:
        lowbd_blend_a64_d16_mask_subw0_subh1_w16_avx2(
            dst, dst_stride, src0, src0_stride, src1, src1_stride, mask,
            mask_stride, h, w, &y_round_offset, shift);
        break;
      default:
        lowbd_blend_a64_d16_mask_subw0_subh1_w32_avx2(
            dst, dst_stride, src0, src0_stride, src1, src1_stride, mask,
            mask_stride, h, w, &y_round_offset, shift);
        break;
    }
  }
}
