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

/*
* \file
*/

#include "strategies/avx2/ipol-avx2.h"

#if COMPILE_INTEL_AVX2
#include <immintrin.h>
#include <stdio.h>
#include <string.h>

#include "encoder.h"
#include "kvazaar.h"
#include "search_inter.h"
#include "strategies/generic/picture-generic.h"
#include "strategies/strategies-ipol.h"
#include "strategyselector.h"
#include "strategies/generic/ipol-generic.h"


extern int8_t kvz_g_luma_filter[4][8];
extern int8_t kvz_g_chroma_filter[8][4];

static int32_t kvz_eight_tap_filter_hor_avx2(int8_t *filter, kvz_pixel *data)
{
  __m128i fir = _mm_loadl_epi64((__m128i*)filter);
  __m128i row = _mm_loadl_epi64((__m128i*)data);
  __m128i acc;
  acc = _mm_maddubs_epi16(row, fir);
  __m128i temp = _mm_srli_si128(acc, 4);
  acc = _mm_add_epi16(acc, temp);
  temp = _mm_srli_si128(acc, 2);
  acc = _mm_add_epi16(acc, temp);
  int32_t filtered = _mm_cvtsi128_si32(acc);

  return filtered;
}

static void kvz_init_shuffle_masks(__m256i *shuf_01_23, __m256i *shuf_45_67) {
  // Shuffle pairs
  *shuf_01_23 = _mm256_setr_epi8(0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8,
                                 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10);
  *shuf_45_67 = _mm256_setr_epi8(4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12,
                                 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14);
}

static void kvz_init_shuffle_masks_chroma(__m256i *shuf_01, __m256i *shuf_23) {
  // Shuffle pairs
  *shuf_01 = _mm256_setr_epi8(0, 1, 1, 2, 2, 3, 3, 4, 8, 9, 9, 10, 10, 11, 11, 12,
                              0, 1, 1, 2, 2, 3, 3, 4, 8, 9, 9, 10, 10, 11, 11, 12);
  *shuf_23 = _mm256_setr_epi8(2, 3, 3, 4, 4, 5, 5, 6, 10, 11, 11, 12, 12, 13, 13, 14,
                              2, 3, 3, 4, 4, 5, 5, 6, 10, 11, 11, 12, 12, 13, 13, 14);
}

static void kvz_init_filter_taps(int8_t *filter,
  __m256i *taps_01_23, __m256i *taps_45_67) {
  // Filter weights
  __m256i all_taps = _mm256_castsi128_si256(_mm_loadl_epi64((__m128i*)filter));
  __m256i perm_01 = _mm256_setr_epi32(0, 0, 0, 0, 1, 1, 1, 1);
  __m256i perm_23 = _mm256_setr_epi32(2, 2, 2, 2, 3, 3, 3, 3);
  all_taps = _mm256_unpacklo_epi16(all_taps, all_taps);
  *taps_01_23 = _mm256_permutevar8x32_epi32(all_taps, perm_01);
  *taps_45_67 = _mm256_permutevar8x32_epi32(all_taps, perm_23);
}

static void kvz_init_filter_taps_chroma(int8_t *filter,
  __m256i *taps_01, __m256i *taps_23) {
  // Filter weights
  __m256i all_taps = _mm256_set1_epi32(*(int32_t*)filter);
  all_taps = _mm256_unpacklo_epi16(all_taps, all_taps);
  *taps_01 = _mm256_shuffle_epi32(all_taps, _MM_SHUFFLE(0, 0, 0, 0));
  *taps_23 = _mm256_shuffle_epi32(all_taps, _MM_SHUFFLE(1, 1, 1, 1));
}

static void kvz_init_ver_filter_taps(int8_t *filter, __m256i *filters) {
  for (int i = 0; i < 4; ++i) filters[i] = _mm256_cvtepi8_epi16(_mm_set1_epi16(*(int16_t*)&filter[2 * i]));
  filters[0] = _mm256_inserti128_si256(filters[0], _mm256_castsi256_si128(filters[3]), 1); // Pairs 01 67
  filters[1] = _mm256_inserti128_si256(filters[1], _mm256_castsi256_si128(filters[0]), 1); // Pairs 23 01
  filters[2] = _mm256_inserti128_si256(filters[2], _mm256_castsi256_si128(filters[1]), 1); // Pairs 45 23
  filters[3] = _mm256_inserti128_si256(filters[3], _mm256_castsi256_si128(filters[2]), 1); // Pairs 67 45
}

static void kvz_eight_tap_filter_hor_8x1_avx2(kvz_pixel *data, int16_t * out,
  __m256i *shuf_01_23, __m256i *shuf_45_67,
  __m256i *taps_01_23, __m256i *taps_45_67) {

  __m256i row = _mm256_broadcastsi128_si256(_mm_loadu_si128((__m128i*)data));

  __m256i pairs_01_23 = _mm256_shuffle_epi8(row, *shuf_01_23);
  __m256i pairs_45_67 = _mm256_shuffle_epi8(row, *shuf_45_67);
  
  __m256i temp0 = _mm256_maddubs_epi16(pairs_01_23, *taps_01_23);
  __m256i temp1 = _mm256_maddubs_epi16(pairs_45_67, *taps_45_67);

  __m256i sum = _mm256_add_epi16(temp0, temp1);
  __m128i filtered = _mm_add_epi16(_mm256_castsi256_si128(sum), _mm256_extracti128_si256(sum, 1));
  _mm_storeu_si128((__m128i*)out, filtered);
}

static void kvz_four_tap_filter_hor_4x4_avx2(kvz_pixel *data, int stride, int16_t * out, int out_stride,
  __m256i *shuf_01, __m256i *shuf_23,
  __m256i *taps_01, __m256i *taps_23) {

  __m256i four_rows = _mm256_setr_epi64x(
    *(int64_t*)&data[0 * stride],
    *(int64_t*)&data[1 * stride],
    *(int64_t*)&data[2 * stride],
    *(int64_t*)&data[3 * stride]);

  __m256i pairs_l = _mm256_shuffle_epi8(four_rows, *shuf_01);
  __m256i pairs_r = _mm256_shuffle_epi8(four_rows, *shuf_23);

  __m256i temp_l = _mm256_maddubs_epi16(pairs_l, *taps_01);
  __m256i temp_r = _mm256_maddubs_epi16(pairs_r, *taps_23);

  __m256i sum = _mm256_add_epi16(temp_l, temp_r);

  __m128i lower = _mm256_castsi256_si128(sum);
  __m128i upper = _mm256_extracti128_si256(sum, 1);
  _mm_storel_epi64((__m128i*)(out + 0 * out_stride), lower);
  _mm_storeh_pd((double*)(out + 1 * out_stride), _mm_castsi128_pd(lower));
  _mm_storel_epi64((__m128i*)(out + 2 * out_stride), upper);
  _mm_storeh_pd((double*)(out + 3 * out_stride), _mm_castsi128_pd(upper));
}

static void kvz_four_tap_filter_hor_4xN_avx2(kvz_pixel *data, int stride, int16_t * out, int out_stride,
  __m256i *shuf_01_23, __m256i *taps_01_23,
  int rows) {

  for (int i = 0; i < rows; ++i) {
    __m256i row = _mm256_set1_epi64x(*(int64_t*)&data[i * stride]);

    __m256i pairs_l_r = _mm256_shuffle_epi8(row, *shuf_01_23);
    __m256i temp_l_r = _mm256_maddubs_epi16(pairs_l_r, *taps_01_23);

    __m128i temp_l = _mm256_castsi256_si128(temp_l_r);
    __m128i temp_r = _mm256_extracti128_si256(temp_l_r, 1);
    __m128i sum = _mm_add_epi16(temp_l, temp_r);

    _mm_storel_epi64((__m128i*)(out + i * out_stride), sum);
  }
}

static int32_t kvz_eight_tap_filter_hor_16bit_avx2(int8_t *filter, int16_t *data)
{
  __m128i fir = _mm_loadl_epi64((__m128i*)filter);
  fir = _mm_cvtepi8_epi16(fir);
  __m128i row = _mm_loadu_si128((__m128i*)data);
  __m128i acc;
  acc = _mm_madd_epi16(fir, row);
  __m128i temp = _mm_srli_si128(acc, 8);
  acc = _mm_add_epi32(acc, temp);
  temp = _mm_srli_si128(acc, 4);
  acc = _mm_add_epi32(acc, temp);
  int32_t filtered = _mm_cvtsi128_si32(acc);

  return filtered;
}

static void kvz_eight_tap_filter_ver_16bit_1x8_avx2(int8_t *filter, int16_t *data, int16_t stride, kvz_pixel *out)
{
  // Interpolation filter shifts
  int32_t shift2 = 6;

  // Weighted prediction offset and shift
  int32_t wp_shift1 = 14 - KVZ_BIT_DEPTH;
  int32_t wp_offset1 = 1 << (wp_shift1 - 1);

  // Filter weights
  __m256i all_taps = _mm256_castsi128_si256(_mm_cvtepi8_epi16(_mm_loadl_epi64((__m128i*)filter)));
  __m256i taps_01_23 = _mm256_shuffle_epi32(all_taps, _MM_SHUFFLE(0, 0, 0, 0));
  __m128i taps_23 = _mm_shuffle_epi32(_mm256_castsi256_si128(all_taps), _MM_SHUFFLE(1, 1, 1, 1));
  __m256i taps_45_67 = _mm256_shuffle_epi32(all_taps, _MM_SHUFFLE(2, 2, 2, 2));
  __m128i taps_67 = _mm_shuffle_epi32(_mm256_castsi256_si128(all_taps), _MM_SHUFFLE(3, 3, 3, 3));

  taps_01_23 = _mm256_inserti128_si256(taps_01_23, taps_23, 1);
  taps_45_67 = _mm256_inserti128_si256(taps_45_67, taps_67, 1);

  __m256i rows02 = _mm256_castsi128_si256(_mm_loadu_si128((__m128i*)&data[0 * stride]));
  __m128i row2 = _mm_loadu_si128((__m128i*)&data[2 * stride]);
  rows02 = _mm256_inserti128_si256(rows02, row2, 1);

  __m256i rows13 = _mm256_castsi128_si256(_mm_loadu_si128((__m128i*)&data[1 * stride]));
  __m128i row3 = _mm_loadu_si128((__m128i*)&data[3 * stride]);
  rows13 = _mm256_inserti128_si256(rows13, row3, 1);

  __m256i pairs_01_23_lo = _mm256_unpacklo_epi16(rows02, rows13);
  __m256i pairs_01_23_hi = _mm256_unpackhi_epi16(rows02, rows13);
  __m256i temp_01_23_lo = _mm256_madd_epi16(pairs_01_23_lo, taps_01_23);
  __m256i temp_01_23_hi = _mm256_madd_epi16(pairs_01_23_hi, taps_01_23);

  __m256i rows46 = _mm256_castsi128_si256(_mm_loadu_si128((__m128i*)&data[4 * stride]));
  __m128i row6 = _mm_loadu_si128((__m128i*)&data[6 * stride]);
  rows46 = _mm256_inserti128_si256(rows46, row6, 1);

  __m256i rows57 = _mm256_castsi128_si256(_mm_loadu_si128((__m128i*)&data[5 * stride]));
  __m128i row7 = _mm_loadu_si128((__m128i*)&data[7 * stride]);
  rows57 = _mm256_inserti128_si256(rows57, row7, 1);

  __m256i pairs_45_67_lo = _mm256_unpacklo_epi16(rows46, rows57);
  __m256i pairs_45_67_hi = _mm256_unpackhi_epi16(rows46, rows57);
  __m256i temp_45_67_lo = _mm256_madd_epi16(pairs_45_67_lo, taps_45_67);
  __m256i temp_45_67_hi = _mm256_madd_epi16(pairs_45_67_hi, taps_45_67);

  __m256i sum_lo_half = _mm256_add_epi32(temp_01_23_lo, temp_45_67_lo);
  __m256i sum_hi_half = _mm256_add_epi32(temp_01_23_hi, temp_45_67_hi);

  __m128i sum_lo = _mm_add_epi32(_mm256_castsi256_si128(sum_lo_half), _mm256_extracti128_si256(sum_lo_half, 1));
  __m128i sum_hi = _mm_add_epi32(_mm256_castsi256_si128(sum_hi_half), _mm256_extracti128_si256(sum_hi_half, 1));

  sum_lo = _mm_srai_epi32(sum_lo, shift2);
  sum_hi = _mm_srai_epi32(sum_hi, shift2);

  __m128i offset = _mm_set1_epi32(wp_offset1);
  sum_lo = _mm_add_epi32(sum_lo, offset);
  sum_lo = _mm_srai_epi32(sum_lo, wp_shift1);
  sum_hi = _mm_add_epi32(sum_hi, offset);
  sum_hi = _mm_srai_epi32(sum_hi, wp_shift1);
  __m128i filtered = _mm_packus_epi32(sum_lo, sum_hi);
  filtered = _mm_packus_epi16(filtered, filtered);


  _mm_storel_epi64((__m128i*)out, filtered);
}

static void kvz_four_tap_filter_ver_16bit_4x4_avx2(int8_t *filter, int16_t *data, int16_t stride, kvz_pixel *out, int16_t out_stride)
{
  // Interpolation filter shifts
  int32_t shift2 = 6;

  // Weighted prediction offset and shift
  int32_t wp_shift1 = 14 - KVZ_BIT_DEPTH;
  int32_t wp_offset1 = 1 << (wp_shift1 - 1);

  // Filter weights
  __m128i all_taps = _mm_cvtepi8_epi16(_mm_cvtsi32_si128(*(int32_t*)filter));
  __m128i taps_01 = _mm_shuffle_epi32(all_taps, _MM_SHUFFLE(0, 0, 0, 0));
  __m128i taps_23 = _mm_shuffle_epi32(all_taps, _MM_SHUFFLE(1, 1, 1, 1));

  __m128i row0 = _mm_loadl_epi64((__m128i*)&data[0 * stride]);
  __m128i row1 = _mm_loadl_epi64((__m128i*)&data[1 * stride]);
  __m128i row2 = _mm_loadl_epi64((__m128i*)&data[2 * stride]);
  __m128i row3 = _mm_loadl_epi64((__m128i*)&data[3 * stride]);
  __m128i row4 = _mm_loadl_epi64((__m128i*)&data[4 * stride]);
  __m128i row5 = _mm_loadl_epi64((__m128i*)&data[5 * stride]);
  __m128i row6 = _mm_loadl_epi64((__m128i*)&data[6 * stride]);

  __m128i pairs01 = _mm_unpacklo_epi16(row0, row1);
  __m128i pairs23 = _mm_unpacklo_epi16(row2, row3);
  __m128i temp01 = _mm_madd_epi16(pairs01, taps_01);
  __m128i temp23 = _mm_madd_epi16(pairs23, taps_23);
  __m128i sum0123 = _mm_add_epi32(temp01, temp23);

  __m128i pairs12 = _mm_unpacklo_epi16(row1, row2);
  __m128i pairs34 = _mm_unpacklo_epi16(row3, row4);
  __m128i temp12 = _mm_madd_epi16(pairs12, taps_01);
  __m128i temp34 = _mm_madd_epi16(pairs34, taps_23);
  __m128i sum1234 = _mm_add_epi32(temp12, temp34);

  __m128i pairs45 = _mm_unpacklo_epi16(row4, row5);
  __m128i temp23_2 = _mm_madd_epi16(pairs23, taps_01);
  __m128i temp45 = _mm_madd_epi16(pairs45, taps_23);
  __m128i sum2345 = _mm_add_epi32(temp23_2, temp45);

  __m128i pairs56 = _mm_unpacklo_epi16(row5, row6);
  __m128i temp34_2 = _mm_madd_epi16(pairs34, taps_01);
  __m128i temp56 = _mm_madd_epi16(pairs56, taps_23);
  __m128i sum3456 = _mm_add_epi32(temp34_2, temp56);

  sum0123 = _mm_srai_epi32(sum0123, shift2);
  sum1234 = _mm_srai_epi32(sum1234, shift2);
  sum2345 = _mm_srai_epi32(sum2345, shift2);
  sum3456 = _mm_srai_epi32(sum3456, shift2);

  __m128i offset = _mm_set1_epi32(wp_offset1);
  sum0123 = _mm_add_epi32(sum0123, offset);
  sum1234 = _mm_add_epi32(sum1234, offset);
  sum2345 = _mm_add_epi32(sum2345, offset);
  sum3456 = _mm_add_epi32(sum3456, offset);

  sum0123 = _mm_srai_epi32(sum0123, wp_shift1);
  sum1234 = _mm_srai_epi32(sum1234, wp_shift1);
  sum2345 = _mm_srai_epi32(sum2345, wp_shift1);
  sum3456 = _mm_srai_epi32(sum3456, wp_shift1);

  __m128i filtered01 = _mm_packs_epi32(sum0123, sum1234);
  __m128i filtered23 = _mm_packs_epi32(sum2345, sum3456);
  __m128i filtered = _mm_packus_epi16(filtered01, filtered23);

  *(int32_t*)&out[0 * out_stride] = _mm_cvtsi128_si32(filtered);
  *(int32_t*)&out[1 * out_stride] = _mm_extract_epi32(filtered, 1);
  *(int32_t*)&out[2 * out_stride] = _mm_extract_epi32(filtered, 2);
  *(int32_t*)&out[3 * out_stride] = _mm_extract_epi32(filtered, 3);
}

static void kvz_four_tap_filter_ver_16bit_4x4_no_round_avx2(int8_t *filter, int16_t *data, int16_t stride, int16_t *out, int16_t out_stride)
{
  int32_t shift2 = 6;

  // Filter weights
  __m128i all_taps = _mm_cvtepi8_epi16(_mm_cvtsi32_si128(*(int32_t*)filter));
  __m128i taps_01 = _mm_shuffle_epi32(all_taps, _MM_SHUFFLE(0, 0, 0, 0));
  __m128i taps_23 = _mm_shuffle_epi32(all_taps, _MM_SHUFFLE(1, 1, 1, 1));

  __m128i row0 = _mm_loadl_epi64((__m128i*)&data[0 * stride]);
  __m128i row1 = _mm_loadl_epi64((__m128i*)&data[1 * stride]);
  __m128i row2 = _mm_loadl_epi64((__m128i*)&data[2 * stride]);
  __m128i row3 = _mm_loadl_epi64((__m128i*)&data[3 * stride]);
  __m128i row4 = _mm_loadl_epi64((__m128i*)&data[4 * stride]);
  __m128i row5 = _mm_loadl_epi64((__m128i*)&data[5 * stride]);
  __m128i row6 = _mm_loadl_epi64((__m128i*)&data[6 * stride]);

  __m128i pairs01 = _mm_unpacklo_epi16(row0, row1);
  __m128i pairs23 = _mm_unpacklo_epi16(row2, row3);
  __m128i temp01 = _mm_madd_epi16(pairs01, taps_01);
  __m128i temp23 = _mm_madd_epi16(pairs23, taps_23);
  __m128i sum0123 = _mm_add_epi32(temp01, temp23);

  __m128i pairs12 = _mm_unpacklo_epi16(row1, row2);
  __m128i pairs34 = _mm_unpacklo_epi16(row3, row4);
  __m128i temp12 = _mm_madd_epi16(pairs12, taps_01);
  __m128i temp34 = _mm_madd_epi16(pairs34, taps_23);
  __m128i sum1234 = _mm_add_epi32(temp12, temp34);

  __m128i pairs45 = _mm_unpacklo_epi16(row4, row5);
  __m128i temp23_2 = _mm_madd_epi16(pairs23, taps_01);
  __m128i temp45 = _mm_madd_epi16(pairs45, taps_23);
  __m128i sum2345 = _mm_add_epi32(temp23_2, temp45);

  __m128i pairs56 = _mm_unpacklo_epi16(row5, row6);
  __m128i temp34_2 = _mm_madd_epi16(pairs34, taps_01);
  __m128i temp56 = _mm_madd_epi16(pairs56, taps_23);
  __m128i sum3456 = _mm_add_epi32(temp34_2, temp56);

  sum0123 = _mm_srai_epi32(sum0123, shift2);
  sum1234 = _mm_srai_epi32(sum1234, shift2);
  sum2345 = _mm_srai_epi32(sum2345, shift2);
  sum3456 = _mm_srai_epi32(sum3456, shift2);

  __m128i filtered01 = _mm_packs_epi32(sum0123, sum1234);
  __m128i filtered23 = _mm_packs_epi32(sum2345, sum3456);

  _mm_storel_pi((__m64*)&out[0 * out_stride], _mm_castsi128_ps(filtered01));
  _mm_storeh_pi((__m64*)&out[1 * out_stride], _mm_castsi128_ps(filtered01));
  _mm_storel_pi((__m64*)&out[2 * out_stride], _mm_castsi128_ps(filtered23));
  _mm_storeh_pi((__m64*)&out[3 * out_stride], _mm_castsi128_ps(filtered23));
}

INLINE static void filter_row_ver_16b_8x1_avx2(int16_t *data, int64_t stride, __m256i* taps, kvz_pixel * out, int64_t out_stride)
{
  // Interpolation filter shifts
  int32_t shift2 = 6;

  // Weighted prediction offset and shift
  int32_t wp_shift1 = 14 - KVZ_BIT_DEPTH;
  int32_t wp_offset1 = 1 << (wp_shift1 - 1);

  __m256i pairs_lo, pairs_hi;

  // Filter 01 later with 67
  __m256i br0 = _mm256_broadcastsi128_si256(_mm_loadu_si128((__m128i*)(data + 0 * stride)));
  __m256i br1 = _mm256_broadcastsi128_si256(_mm_loadu_si128((__m128i*)(data + 1 * stride)));

  __m256i br2 = _mm256_broadcastsi128_si256(_mm_loadu_si128((__m128i*)(data + 2 * stride)));
  __m256i br3 = _mm256_broadcastsi128_si256(_mm_loadu_si128((__m128i*)(data + 3 * stride)));
  pairs_lo = _mm256_unpacklo_epi16(br2, br3);
  pairs_hi = _mm256_unpackhi_epi16(br2, br3);
  __m256i rows02_23_01_lo = _mm256_madd_epi16(pairs_lo, taps[1]); // Firs 23 01
  __m256i rows02_23_01_hi = _mm256_madd_epi16(pairs_hi, taps[1]); // Firs 23 01

  __m256i br4 = _mm256_broadcastsi128_si256(_mm_loadu_si128((__m128i*)(data + 4 * stride)));
  __m256i br5 = _mm256_broadcastsi128_si256(_mm_loadu_si128((__m128i*)(data + 5 * stride)));
  pairs_lo = _mm256_unpacklo_epi16(br4, br5);
  pairs_hi = _mm256_unpackhi_epi16(br4, br5);
  __m256i rows02_45_23_lo = _mm256_madd_epi16(pairs_lo, taps[2]); // Firs 45 23
  __m256i rows02_45_23_hi = _mm256_madd_epi16(pairs_hi, taps[2]); // Firs 45 23

  __m256i br6 = _mm256_broadcastsi128_si256(_mm_loadu_si128((__m128i*)(data + 6 * stride)));
  __m256i br7 = _mm256_broadcastsi128_si256(_mm_loadu_si128((__m128i*)(data + 7 * stride)));
  pairs_lo = _mm256_unpacklo_epi16(br6, br7);
  pairs_hi = _mm256_unpackhi_epi16(br6, br7);
  __m256i rows02_67_45_lo = _mm256_madd_epi16(pairs_lo, taps[3]); // Firs 67 45
  __m256i rows02_67_45_hi = _mm256_madd_epi16(pairs_hi, taps[3]); // Firs 67 45
  __m256i rows46_23_01_lo = _mm256_madd_epi16(pairs_lo, taps[1]); // Firs 23 01
  __m256i rows46_23_01_hi = _mm256_madd_epi16(pairs_hi, taps[1]); // Firs 23 01

  __m256i br8 = _mm256_broadcastsi128_si256(_mm_loadu_si128((__m128i*)(data + 8 * stride)));
  __m256i br9 = _mm256_broadcastsi128_si256(_mm_loadu_si128((__m128i*)(data + 9 * stride)));
  pairs_lo = _mm256_unpacklo_epi16(br8, br9);
  pairs_hi = _mm256_unpackhi_epi16(br8, br9);
  // Filter rows02 later
  __m256i rows46_45_23_lo = _mm256_madd_epi16(pairs_lo, taps[2]); // Firs 45 23
  __m256i rows46_45_23_hi = _mm256_madd_epi16(pairs_hi, taps[2]); // Firs 45 23

  __m256i br10 = _mm256_broadcastsi128_si256(_mm_loadu_si128((__m128i*)(data + 10 * stride)));
  __m256i br11 = _mm256_broadcastsi128_si256(_mm_loadu_si128((__m128i*)(data + 11 * stride)));
  pairs_lo = _mm256_unpacklo_epi16(br10, br11);
  pairs_hi = _mm256_unpackhi_epi16(br10, br11);
  __m256i rows46_67_45_lo = _mm256_madd_epi16(pairs_lo, taps[3]); // Firs 67 45
  __m256i rows46_67_45_hi = _mm256_madd_epi16(pairs_hi, taps[3]); // Firs 67 45

  // Deferred
  __m256i r08 = _mm256_permute2x128_si256(br0, br8, _MM_SHUFFLE(0, 2, 0, 0));
  __m256i r19 = _mm256_permute2x128_si256(br1, br9, _MM_SHUFFLE(0, 2, 0, 0));
  pairs_lo = _mm256_unpacklo_epi16(r08, r19);
  pairs_hi = _mm256_unpackhi_epi16(r08, r19);
  __m256i rows02_01_67_lo = _mm256_madd_epi16(pairs_lo, taps[0]); // Firs 01 67
  __m256i rows02_01_67_hi = _mm256_madd_epi16(pairs_hi, taps[0]); // Firs 01 67

  __m256i br12 = _mm256_broadcastsi128_si256(_mm_loadu_si128((__m128i*)(data + 12 * stride)));
  __m256i br13 = _mm256_broadcastsi128_si256(_mm_loadu_si128((__m128i*)(data + 13 * stride)));

  __m256i r412 = _mm256_permute2x128_si256(br4, br12, _MM_SHUFFLE(0, 2, 0, 0));
  __m256i r513 = _mm256_permute2x128_si256(br5, br13, _MM_SHUFFLE(0, 2, 0, 0));
  pairs_lo = _mm256_unpacklo_epi16(r412, r513);
  pairs_hi = _mm256_unpackhi_epi16(r412, r513);
  __m256i rows46_01_67_lo = _mm256_madd_epi16(pairs_lo, taps[0]); // Firs 01 67
  __m256i rows46_01_67_hi = _mm256_madd_epi16(pairs_hi, taps[0]); // Firs 01 67

  __m256i accu02_lo, accu02_hi;
  __m256i accu46_lo, accu46_hi;

  accu02_lo = _mm256_add_epi32(rows02_23_01_lo, rows02_45_23_lo);
  accu02_lo = _mm256_add_epi32(accu02_lo, rows02_67_45_lo);
  accu02_lo = _mm256_add_epi32(accu02_lo, rows02_01_67_lo);

  accu02_hi = _mm256_add_epi32(rows02_23_01_hi, rows02_45_23_hi);
  accu02_hi = _mm256_add_epi32(accu02_hi, rows02_67_45_hi);
  accu02_hi = _mm256_add_epi32(accu02_hi, rows02_01_67_hi);

  accu46_lo = _mm256_add_epi32(rows46_23_01_lo, rows46_45_23_lo);
  accu46_lo = _mm256_add_epi32(accu46_lo, rows46_67_45_lo);
  accu46_lo = _mm256_add_epi32(accu46_lo, rows46_01_67_lo);

  accu46_hi = _mm256_add_epi32(rows46_23_01_hi, rows46_45_23_hi);
  accu46_hi = _mm256_add_epi32(accu46_hi, rows46_67_45_hi);
  accu46_hi = _mm256_add_epi32(accu46_hi, rows46_01_67_hi);

  accu02_lo = _mm256_srai_epi32(accu02_lo, shift2);
  accu02_hi = _mm256_srai_epi32(accu02_hi, shift2);
  accu46_lo = _mm256_srai_epi32(accu46_lo, shift2);
  accu46_hi = _mm256_srai_epi32(accu46_hi, shift2);

  __m256i offset = _mm256_set1_epi32(wp_offset1);
  accu02_lo = _mm256_add_epi32(accu02_lo, offset);
  accu02_hi = _mm256_add_epi32(accu02_hi, offset);
  accu46_lo = _mm256_add_epi32(accu46_lo, offset);
  accu46_hi = _mm256_add_epi32(accu46_hi, offset);

  accu02_lo = _mm256_srai_epi32(accu02_lo, wp_shift1);
  accu02_hi = _mm256_srai_epi32(accu02_hi, wp_shift1);
  accu46_lo = _mm256_srai_epi32(accu46_lo, wp_shift1);
  accu46_hi = _mm256_srai_epi32(accu46_hi, wp_shift1);

  __m256i rows02 = _mm256_packs_epi32(accu02_lo, accu02_hi);
  __m256i rows46 = _mm256_packs_epi32(accu46_lo, accu46_hi);

  __m256i filtered04_26 = _mm256_packus_epi16(rows02, rows46);
  __m128i filtered04 = _mm256_castsi256_si128(filtered04_26);
  __m128i filtered26 = _mm256_extracti128_si256(filtered04_26, 1);

  _mm_storel_pi((__m64*)&out[0 * out_stride], _mm_castsi128_ps(filtered04));
  _mm_storel_pi((__m64*)&out[2 * out_stride], _mm_castsi128_ps(filtered26));
  _mm_storeh_pi((__m64*)&out[4 * out_stride], _mm_castsi128_ps(filtered04));
  _mm_storeh_pi((__m64*)&out[6 * out_stride], _mm_castsi128_ps(filtered26));
}

INLINE static void filter_row_ver_16b_8x1_no_round_avx2(int16_t *data, int64_t stride, __m256i *taps, int16_t *out, int64_t out_stride) {

  int32_t shift2 = 6;

  __m256i pairs_lo, pairs_hi;

  // Filter 01 later with 67
  __m256i br0 = _mm256_broadcastsi128_si256(_mm_loadu_si128((__m128i*)(data + 0 * stride)));
  __m256i br1 = _mm256_broadcastsi128_si256(_mm_loadu_si128((__m128i*)(data + 1 * stride)));

  __m256i br2 = _mm256_broadcastsi128_si256(_mm_loadu_si128((__m128i*)(data + 2 * stride)));
  __m256i br3 = _mm256_broadcastsi128_si256(_mm_loadu_si128((__m128i*)(data + 3 * stride)));
  pairs_lo = _mm256_unpacklo_epi16(br2, br3);
  pairs_hi = _mm256_unpackhi_epi16(br2, br3);
  __m256i rows02_23_01_lo = _mm256_madd_epi16(pairs_lo, taps[1]); // Firs 23 01
  __m256i rows02_23_01_hi = _mm256_madd_epi16(pairs_hi, taps[1]); // Firs 23 01

  __m256i br4 = _mm256_broadcastsi128_si256(_mm_loadu_si128((__m128i*)(data + 4 * stride)));
  __m256i br5 = _mm256_broadcastsi128_si256(_mm_loadu_si128((__m128i*)(data + 5 * stride)));
  pairs_lo = _mm256_unpacklo_epi16(br4, br5);
  pairs_hi = _mm256_unpackhi_epi16(br4, br5);
  __m256i rows02_45_23_lo = _mm256_madd_epi16(pairs_lo, taps[2]); // Firs 45 23
  __m256i rows02_45_23_hi = _mm256_madd_epi16(pairs_hi, taps[2]); // Firs 45 23

  __m256i br6 = _mm256_broadcastsi128_si256(_mm_loadu_si128((__m128i*)(data + 6 * stride)));
  __m256i br7 = _mm256_broadcastsi128_si256(_mm_loadu_si128((__m128i*)(data + 7 * stride)));
  pairs_lo = _mm256_unpacklo_epi16(br6, br7);
  pairs_hi = _mm256_unpackhi_epi16(br6, br7);
  __m256i rows02_67_45_lo = _mm256_madd_epi16(pairs_lo, taps[3]); // Firs 67 45
  __m256i rows02_67_45_hi = _mm256_madd_epi16(pairs_hi, taps[3]); // Firs 67 45
  __m256i rows46_23_01_lo = _mm256_madd_epi16(pairs_lo, taps[1]); // Firs 23 01
  __m256i rows46_23_01_hi = _mm256_madd_epi16(pairs_hi, taps[1]); // Firs 23 01

  __m256i br8 = _mm256_broadcastsi128_si256(_mm_loadu_si128((__m128i*)(data + 8 * stride)));
  __m256i br9 = _mm256_broadcastsi128_si256(_mm_loadu_si128((__m128i*)(data + 9 * stride)));
  pairs_lo = _mm256_unpacklo_epi16(br8, br9);
  pairs_hi = _mm256_unpackhi_epi16(br8, br9);
  // Filter rows02 later
  __m256i rows46_45_23_lo = _mm256_madd_epi16(pairs_lo, taps[2]); // Firs 45 23
  __m256i rows46_45_23_hi = _mm256_madd_epi16(pairs_hi, taps[2]); // Firs 45 23

  __m256i br10 = _mm256_broadcastsi128_si256(_mm_loadu_si128((__m128i*)(data + 10 * stride)));
  __m256i br11 = _mm256_broadcastsi128_si256(_mm_loadu_si128((__m128i*)(data + 11 * stride)));
  pairs_lo = _mm256_unpacklo_epi16(br10, br11);
  pairs_hi = _mm256_unpackhi_epi16(br10, br11);
  __m256i rows46_67_45_lo = _mm256_madd_epi16(pairs_lo, taps[3]); // Firs 67 45
  __m256i rows46_67_45_hi = _mm256_madd_epi16(pairs_hi, taps[3]); // Firs 67 45

                                                                  // Deferred
  __m256i r08 = _mm256_permute2x128_si256(br0, br8, _MM_SHUFFLE(0, 2, 0, 0));
  __m256i r19 = _mm256_permute2x128_si256(br1, br9, _MM_SHUFFLE(0, 2, 0, 0));
  pairs_lo = _mm256_unpacklo_epi16(r08, r19);
  pairs_hi = _mm256_unpackhi_epi16(r08, r19);
  __m256i rows02_01_67_lo = _mm256_madd_epi16(pairs_lo, taps[0]); // Firs 01 67
  __m256i rows02_01_67_hi = _mm256_madd_epi16(pairs_hi, taps[0]); // Firs 01 67

  __m256i br12 = _mm256_broadcastsi128_si256(_mm_loadu_si128((__m128i*)(data + 12 * stride)));
  __m256i br13 = _mm256_broadcastsi128_si256(_mm_loadu_si128((__m128i*)(data + 13 * stride)));

  __m256i r412 = _mm256_permute2x128_si256(br4, br12, _MM_SHUFFLE(0, 2, 0, 0));
  __m256i r513 = _mm256_permute2x128_si256(br5, br13, _MM_SHUFFLE(0, 2, 0, 0));
  pairs_lo = _mm256_unpacklo_epi16(r412, r513);
  pairs_hi = _mm256_unpackhi_epi16(r412, r513);
  __m256i rows46_01_67_lo = _mm256_madd_epi16(pairs_lo, taps[0]); // Firs 01 67
  __m256i rows46_01_67_hi = _mm256_madd_epi16(pairs_hi, taps[0]); // Firs 01 67

  __m256i accu02_lo, accu02_hi;
  __m256i accu46_lo, accu46_hi;

  accu02_lo = _mm256_add_epi32(rows02_23_01_lo, rows02_45_23_lo);
  accu02_lo = _mm256_add_epi32(accu02_lo, rows02_67_45_lo);
  accu02_lo = _mm256_add_epi32(accu02_lo, rows02_01_67_lo);

  accu02_hi = _mm256_add_epi32(rows02_23_01_hi, rows02_45_23_hi);
  accu02_hi = _mm256_add_epi32(accu02_hi, rows02_67_45_hi);
  accu02_hi = _mm256_add_epi32(accu02_hi, rows02_01_67_hi);

  accu46_lo = _mm256_add_epi32(rows46_23_01_lo, rows46_45_23_lo);
  accu46_lo = _mm256_add_epi32(accu46_lo, rows46_67_45_lo);
  accu46_lo = _mm256_add_epi32(accu46_lo, rows46_01_67_lo);

  accu46_hi = _mm256_add_epi32(rows46_23_01_hi, rows46_45_23_hi);
  accu46_hi = _mm256_add_epi32(accu46_hi, rows46_67_45_hi);
  accu46_hi = _mm256_add_epi32(accu46_hi, rows46_01_67_hi);

  accu02_lo = _mm256_srai_epi32(accu02_lo, shift2);
  accu02_hi = _mm256_srai_epi32(accu02_hi, shift2);
  accu46_lo = _mm256_srai_epi32(accu46_lo, shift2);
  accu46_hi = _mm256_srai_epi32(accu46_hi, shift2);

  __m256i rows02 = _mm256_packs_epi32(accu02_lo, accu02_hi);
  __m256i rows46 = _mm256_packs_epi32(accu46_lo, accu46_hi);

  __m128i filtered0 = _mm256_castsi256_si128(rows02);
  __m128i filtered2 = _mm256_extracti128_si256(rows02, 1);
  __m128i filtered4 = _mm256_castsi256_si128(rows46);
  __m128i filtered6 = _mm256_extracti128_si256(rows46, 1);

  _mm_storeu_si128((__m128i*)(out + 0 * out_stride), filtered0);
  _mm_storeu_si128((__m128i*)(out + 2 * out_stride), filtered2);
  _mm_storeu_si128((__m128i*)(out + 4 * out_stride), filtered4);
  _mm_storeu_si128((__m128i*)(out + 6 * out_stride), filtered6);
}

INLINE static void kvz_eight_tap_filter_ver_16bit_8x8_avx2(__m256i *filters, int16_t *data, int16_t stride, kvz_pixel *out, int out_stride)
{
  // Filter even rows
  filter_row_ver_16b_8x1_avx2(data, stride, filters, out, out_stride); // 0 2 4 6

  // Filter odd rows
  filter_row_ver_16b_8x1_avx2(data + stride, stride, filters, out + out_stride, out_stride); // 1 3 5 7
 
}

INLINE static void kvz_eight_tap_filter_ver_16bit_8x8_no_round_avx2(__m256i *filters, int16_t *data, int16_t stride, int16_t *out, int out_stride)
{
  // Filter even rows
  filter_row_ver_16b_8x1_no_round_avx2(data, stride, filters, out, out_stride); // 0 2 4 6

  // Filter odd rows
  filter_row_ver_16b_8x1_no_round_avx2(data + stride, stride, filters, out + out_stride, out_stride); // 1 3 5 7

}

static void kvz_filter_hpel_blocks_hor_ver_luma_avx2(const encoder_control_t * encoder,
  kvz_pixel *src,
  int16_t src_stride,
  int width,
  int height,
  kvz_pixel filtered[4][LCU_WIDTH * LCU_WIDTH],
  int16_t hor_intermediate[5][(KVZ_EXT_BLOCK_W_LUMA + 1) * LCU_WIDTH],
  int8_t fme_level,
  int16_t hor_first_cols[5][KVZ_EXT_BLOCK_W_LUMA + 1],
  int8_t hpel_off_x, int8_t hpel_off_y)
{
  int x, y, first_y;

  // Interpolation filter shifts
  int16_t shift1 = KVZ_BIT_DEPTH - 8;
  int32_t shift2 = 6;

  // Weighted prediction offset and shift
  int32_t wp_shift1 = 14 - KVZ_BIT_DEPTH;
  int32_t wp_offset1 = 1 << (wp_shift1 - 1);

  int8_t *fir0 = kvz_g_luma_filter[0];
  int8_t *fir2 = kvz_g_luma_filter[2];

  int16_t dst_stride = LCU_WIDTH;
  int16_t hor_stride = LCU_WIDTH;

  int16_t *hor_pos0 = hor_intermediate[0];
  int16_t *hor_pos2 = hor_intermediate[1];
  int16_t *col_pos0 = hor_first_cols[0];
  int16_t *col_pos2 = hor_first_cols[2];

  // Horizontally filtered samples from the top row are
  // not needed unless samples for diagonal positions are filtered later.
  first_y = fme_level > 1 ? 0 : 1;

  // HORIZONTAL STEP
  // Integer pixels
  __m256i shuf_01_23, shuf_45_67;
  __m256i taps_01_23, taps_45_67;

  for (y = 0; y < height + KVZ_EXT_PADDING_LUMA + 1; ++y) {
    
    for (x = 0; x + 7 < width; x += 8) {
      int ypos = y - KVZ_LUMA_FILTER_OFFSET;
      int xpos = x + 1;
      __m128i* out = (__m128i*)&hor_pos0[y * hor_stride + x];
      __m128i chunk = _mm_loadl_epi64((__m128i*)&src[src_stride*ypos + xpos]);
      chunk = _mm_cvtepu8_epi16(chunk);
      chunk = _mm_slli_epi16(chunk, 6); // Multiply by 64
      _mm_storeu_si128(out, chunk); //TODO: >> shift1
    }
  }

  // Write the first column in contiguous memory
  x = 0;
  for (y = 0; y < height + KVZ_EXT_PADDING_LUMA + 1; ++y) {
    int ypos = y - KVZ_LUMA_FILTER_OFFSET;
    int32_t first_sample = src[src_stride*ypos + x] << 6 >> shift1;
    col_pos0[y] = first_sample;
  }

  // Half pixels
  kvz_init_shuffle_masks(&shuf_01_23, &shuf_45_67);
  kvz_init_filter_taps(fir2, &taps_01_23, &taps_45_67);

  for (y = first_y; y < height + KVZ_EXT_PADDING_LUMA + 1; ++y) {

    for (x = 0; x + 7 < width; x += 8) {
      int ypos = y - KVZ_LUMA_FILTER_OFFSET;
      int xpos = x - KVZ_LUMA_FILTER_OFFSET + 1;
      kvz_eight_tap_filter_hor_8x1_avx2(&src[src_stride*ypos + xpos], &hor_pos2[y * hor_stride + x],
                                            &shuf_01_23, &shuf_45_67,
                                            &taps_01_23, &taps_45_67); //TODO: >> shift1
    }
  }

  // Write the first column in contiguous memory
  x = 0;
  for (y = first_y; y < height + KVZ_EXT_PADDING_LUMA + 1; ++y) {
    int ypos = y - KVZ_LUMA_FILTER_OFFSET;
    int xpos = x - KVZ_LUMA_FILTER_OFFSET;
    col_pos2[y] = kvz_eight_tap_filter_hor_avx2(fir2, &src[src_stride*ypos + xpos]) >> shift1;
  }

  // VERTICAL STEP
  kvz_pixel *out_l = filtered[0];
  kvz_pixel *out_r = filtered[1];
  kvz_pixel *out_t = filtered[2];
  kvz_pixel *out_b = filtered[3];

  __m256i taps[4];
  kvz_init_ver_filter_taps(fir0, taps);

  // Right
  for (y = 0; y + 7 < height; y+=8) {

    for (x = 0; x + 7 < width ; x+=8) {
      kvz_eight_tap_filter_ver_16bit_8x8_avx2(taps, &hor_pos2[(y + 1) * hor_stride + x], hor_stride, &out_r[y * dst_stride + x], dst_stride);
    }
  }

  // Left
  // Copy from the right filtered block and filter the extra column
  for (y = 0; y < height; ++y) {
    x = 0;
    *(uint64_t*)&out_l[y * dst_stride + x] = *(uint64_t*)&out_r[y * dst_stride + x] << 8;
    for (x = 8; x < width; x += 8) *(int64_t*)&out_l[y * dst_stride + x] = *(int64_t*)&out_r[y * dst_stride + x - 1];
    x = 0;
    int16_t sample = 64 * col_pos2[y + 1 + KVZ_LUMA_FILTER_OFFSET] >> shift2;
    sample = kvz_fast_clip_16bit_to_pixel((sample + wp_offset1) >> wp_shift1);
    out_l[y * dst_stride + x] = sample;
  }

  kvz_init_ver_filter_taps(fir2, taps);
  // Top
  for (y = 0; y + 7 < height; y+=8) {
    for (x = 0; x + 7 < width; x+=8) {
      kvz_eight_tap_filter_ver_16bit_8x8_avx2(taps, &hor_pos0[y * hor_stride + x], hor_stride, &out_t[y * dst_stride + x], dst_stride);
    }
  }

  // Bottom
  // Copy what can be copied from the top filtered values.
  // Then filter the last row from horizontal intermediate buffer.
  for (y = 0; y < height - 1; ++y) {
    for (x = 0; x + 7 < width; x += 8) {
      *(int64_t*)&out_b[(y + 0) * dst_stride + x] = *(int64_t*)&out_t[(y + 1) * dst_stride + x];
    }
  }

  for (x = 0; x + 7 < width; x += 8) {
    kvz_eight_tap_filter_ver_16bit_1x8_avx2(fir2, &hor_pos0[(y + 1) * hor_stride + x], hor_stride, &out_b[y * dst_stride + x]);
  }
}

static void kvz_filter_hpel_blocks_diag_luma_avx2(const encoder_control_t * encoder,
  kvz_pixel *src,
  int16_t src_stride,
  int width,
  int height,
  kvz_pixel filtered[4][LCU_WIDTH * LCU_WIDTH],
  int16_t hor_intermediate[5][(KVZ_EXT_BLOCK_W_LUMA + 1) * LCU_WIDTH],
  int8_t fme_level,
  int16_t hor_first_cols[5][KVZ_EXT_BLOCK_W_LUMA + 1],
  int8_t hpel_off_x, int8_t hpel_off_y)
{
  int x, y;

  // Interpolation filter shifts
  int32_t shift2 = 6;

  // Weighted prediction offset and shift
  int32_t wp_shift1 = 14 - KVZ_BIT_DEPTH;
  int32_t wp_offset1 = 1 << (wp_shift1 - 1);

  int8_t *fir2 = kvz_g_luma_filter[2];

  int16_t dst_stride = LCU_WIDTH;
  int16_t hor_stride = LCU_WIDTH;

  int16_t *hor_pos2 = hor_intermediate[1];
  int16_t *col_pos2 = hor_first_cols[2];

  // VERTICAL STEP
  kvz_pixel *out_tl = filtered[0];
  kvz_pixel *out_tr = filtered[1];
  kvz_pixel *out_bl = filtered[2];
  kvz_pixel *out_br = filtered[3];

 __m256i taps[4];
  kvz_init_ver_filter_taps(fir2, taps);
  // Top-Right
  for (y = 0; y + 7 < height; y += 8) {
    for (x = 0; x + 7 < width; x += 8) {
      kvz_eight_tap_filter_ver_16bit_8x8_avx2(taps, &hor_pos2[y * hor_stride + x], hor_stride, &out_tr[y * dst_stride + x], dst_stride);
    }
  }

  // Top-left
  // Copy from the top-right filtered block and filter the extra column
  for (y = 0; y < height; ++y) {
    x = 0;
    int16_t sample = kvz_eight_tap_filter_hor_16bit_avx2(fir2, &col_pos2[y]) >> shift2;
    sample = kvz_fast_clip_16bit_to_pixel((sample + wp_offset1) >> wp_shift1);
    out_tl[y * dst_stride + x] = sample;

    for (x = 1; x < width; ++x) out_tl[y * dst_stride + x] = out_tr[y * dst_stride + x - 1];
  }

  // Bottom-right
  // Copy what can be copied from top-right filtered values. Filter the last row.
  for (y = 0; y < height - 1; ++y) {
    for (x = 0; x + 7 < width; x += 8) {
      memcpy(&out_br[y * dst_stride + x], &out_tr[(y + 1) * dst_stride + x], 8);
    }
  }

  for (x = 0; x + 7 < width; x += 8) {
    kvz_eight_tap_filter_ver_16bit_1x8_avx2(fir2, &hor_pos2[(y + 1) * hor_stride + x], hor_stride, &out_br[y * dst_stride + x]);
  }

  // Bottom-left
  // Copy what can be copied from the top-left filtered values.
  // Copy what can be copied from the bottom-right filtered values.
  // Finally filter the last pixel from the column array.
  for (y = 0; y < height - 1; ++y) {
    for (x = 0; x + 7 < width; x += 8) {
      memcpy(&out_bl[y * dst_stride + x], &out_tl[(y + 1) * dst_stride + x], 8);
    }
  }

  for (x = 1; x < width; ++x) out_bl[y * dst_stride + x] = out_br[y * dst_stride + x - 1];
  x = 0;
  int16_t sample = kvz_eight_tap_filter_hor_16bit_avx2(fir2, &col_pos2[(y + 1)]) >> shift2;
  sample = kvz_fast_clip_16bit_to_pixel((sample + wp_offset1) >> wp_shift1);
  out_bl[y * dst_stride + x] = sample;
}

static void kvz_filter_qpel_blocks_hor_ver_luma_avx2(const encoder_control_t * encoder,
  kvz_pixel *src,
  int16_t src_stride,
  int width,
  int height,
  kvz_pixel filtered[4][LCU_WIDTH * LCU_WIDTH],
  int16_t hor_intermediate[5][(KVZ_EXT_BLOCK_W_LUMA + 1) * LCU_WIDTH],
  int8_t fme_level,
  int16_t hor_first_cols[5][KVZ_EXT_BLOCK_W_LUMA + 1],
  int8_t hpel_off_x, int8_t hpel_off_y)
{
  int x, y;

  // Interpolation filter shifts
  int16_t shift1 = KVZ_BIT_DEPTH - 8;
  int32_t shift2 = 6;

  // Weighted prediction offset and shift
  int32_t wp_shift1 = 14 - KVZ_BIT_DEPTH;
  int32_t wp_offset1 = 1 << (wp_shift1 - 1);

  int8_t *fir0 = kvz_g_luma_filter[0];
  int8_t *fir2 = kvz_g_luma_filter[2];
  int8_t *fir1 = kvz_g_luma_filter[1];
  int8_t *fir3 = kvz_g_luma_filter[3];

  // Horiziontal positions. Positions 0 and 2 have already been calculated in filtered.
  int16_t *hor_pos0 = hor_intermediate[0];
  int16_t *hor_pos2 = hor_intermediate[1];
  int16_t *hor_pos_l = hor_intermediate[3];
  int16_t *hor_pos_r = hor_intermediate[4];
  int8_t *hor_fir_l = hpel_off_x != 0 ? fir1 : fir3;
  int8_t *hor_fir_r = hpel_off_x != 0 ? fir3 : fir1;
  int16_t *col_pos_l = hor_first_cols[1];
  int16_t *col_pos_r = hor_first_cols[3];

  int16_t dst_stride = LCU_WIDTH;
  int16_t hor_stride = LCU_WIDTH;

  int16_t *hor_hpel_pos = hpel_off_x != 0 ? hor_pos2 : hor_pos0;
  int16_t *col_pos_hor = hpel_off_x != 0 ? hor_first_cols[2] : hor_first_cols[0];

  // Specify if integer pixels are filtered from left or/and top integer samples
  int off_x_fir_l = hpel_off_x < 1 ? 0 : 1;
  int off_x_fir_r = hpel_off_x < 0 ? 0 : 1;
  int off_y_fir_t = hpel_off_y < 1 ? 0 : 1;
  int off_y_fir_b = hpel_off_y < 0 ? 0 : 1;

  // HORIZONTAL STEP
  __m256i shuf_01_23, shuf_45_67;
  __m256i taps_01_23, taps_45_67;

  // Left QPEL
  kvz_init_shuffle_masks(&shuf_01_23, &shuf_45_67);
  kvz_init_filter_taps(hor_fir_l, &taps_01_23, &taps_45_67);
  
  int sample_off_y = hpel_off_y < 0 ? 0 : 1;

  for (y = 0; y < height + KVZ_EXT_PADDING_LUMA + 1; ++y) {

    for (x = 0; x + 7 < width; x += 8) {
      int ypos = y - KVZ_LUMA_FILTER_OFFSET;
      int xpos = x - KVZ_LUMA_FILTER_OFFSET + 1;
      kvz_eight_tap_filter_hor_8x1_avx2(&src[src_stride*ypos + xpos], &hor_pos_l[y * hor_stride + x],
        &shuf_01_23, &shuf_45_67,
        &taps_01_23, &taps_45_67); //TODO: >> shift1
    }
  }

  // Write the first column in contiguous memory
  x = 0;
  for (y = 0; y < height + KVZ_EXT_PADDING_LUMA + 1; ++y) {
    int ypos = y - KVZ_LUMA_FILTER_OFFSET;
    int xpos = x - KVZ_LUMA_FILTER_OFFSET;
    col_pos_l[y] = kvz_eight_tap_filter_hor_avx2(hor_fir_l, &src[src_stride*ypos + xpos]) >> shift1;
  }

  // Right QPEL
  kvz_init_filter_taps(hor_fir_r, &taps_01_23, &taps_45_67);

  for (y = 0; y < height + KVZ_EXT_PADDING_LUMA + 1; ++y) {

    for (x = 0; x + 7 < width; x += 8) {
      int ypos = y - KVZ_LUMA_FILTER_OFFSET;
      int xpos = x - KVZ_LUMA_FILTER_OFFSET + 1;
      kvz_eight_tap_filter_hor_8x1_avx2(&src[src_stride*ypos + xpos], &hor_pos_r[y * hor_stride + x],
        &shuf_01_23, &shuf_45_67,
        &taps_01_23, &taps_45_67); //TODO: >> shift1
    }
  }

  // Write the first column in contiguous memory
  x = 0;
  for (y = 0; y < height + KVZ_EXT_PADDING_LUMA + 1; ++y) {
    int ypos = y - KVZ_LUMA_FILTER_OFFSET;
    int xpos = x - KVZ_LUMA_FILTER_OFFSET;
    col_pos_r[y] = kvz_eight_tap_filter_hor_avx2(hor_fir_r, &src[src_stride*ypos + xpos]) >> shift1;
  }

  // VERTICAL STEP
  kvz_pixel *out_l = filtered[0];
  kvz_pixel *out_r = filtered[1];
  kvz_pixel *out_t = filtered[2];
  kvz_pixel *out_b = filtered[3];

  int8_t *ver_fir_l = hpel_off_y != 0 ? fir2 : fir0;
  int8_t *ver_fir_r = hpel_off_y != 0 ? fir2 : fir0;
  int8_t *ver_fir_t = hpel_off_y != 0 ? fir1 : fir3;
  int8_t *ver_fir_b = hpel_off_y != 0 ? fir3 : fir1;

  __m256i taps[4];

  // Left QPEL (1/4 or 3/4 x positions) 
  // Filter block and then filter column and align if neccessary
  kvz_init_ver_filter_taps(ver_fir_l, taps);

  for (y = 0; y + 7 < height; y += 8) {
    for (x = 0; x + 7 < width; x += 8) {
      int ypos = y + sample_off_y;
      kvz_eight_tap_filter_ver_16bit_8x8_avx2(taps, &hor_pos_l[ypos * hor_stride + x], hor_stride, &out_l[y * dst_stride + x], dst_stride);
    }
  }

  if (!off_x_fir_l) {
    for (y = 0; y < height; ++y) {
      for (x = width - 8; x >= 8; x -= 8) {
        uint64_t chunk = *(uint64_t*)&out_l[y * dst_stride + x - 1];
        *(uint64_t*)&out_l[y * dst_stride + x] = chunk;
      }

      x = 0;
      int16_t sample = kvz_eight_tap_filter_hor_16bit_avx2(ver_fir_l, &col_pos_l[y + sample_off_y]) >> shift2;
      sample = kvz_fast_clip_16bit_to_pixel((sample + wp_offset1) >> wp_shift1);
      uint64_t first = sample;
      uint64_t rest = *(uint64_t*)&out_l[y * dst_stride + x];
      uint64_t chunk = (rest << 8) | first;
      *(uint64_t*)&out_l[y * dst_stride + x] = chunk;
    }
  }

  // Right QPEL (3/4 or 1/4 x positions)
  // Filter block and then filter column and align if neccessary
  kvz_init_ver_filter_taps(ver_fir_r, taps);

  for (y = 0; y + 7 < height; y += 8) {
    for (x = 0; x + 7 < width; x += 8) {
      int ypos = y + sample_off_y;
      kvz_eight_tap_filter_ver_16bit_8x8_avx2(taps, &hor_pos_r[ypos * hor_stride + x], hor_stride, &out_r[y * dst_stride + x], dst_stride);
    }
  }

  if (!off_x_fir_r) {
    for (y = 0; y < height; ++y) {
      for (x = width - 8; x >= 8; x -= 8) {
        uint64_t chunk = *(uint64_t*)&out_r[y * dst_stride + x - 1];
        *(uint64_t*)&out_r[y * dst_stride + x] = chunk;
      }

      x = 0;
      int16_t sample = kvz_eight_tap_filter_hor_16bit_avx2(ver_fir_r, &col_pos_r[y + sample_off_y]) >> shift2;
      sample = kvz_fast_clip_16bit_to_pixel((sample + wp_offset1) >> wp_shift1);
      uint64_t first = sample;
      uint64_t rest = *(uint64_t*)&out_r[y * dst_stride + x];
      uint64_t chunk = (rest << 8) | first;
      *(uint64_t*)&out_r[y * dst_stride + x] = chunk;
    }
  }

  
  // Top QPEL (1/4 or 3/4 y positions)
  // Filter block and then filter column and align if neccessary
  int sample_off_x = (hpel_off_x > -1 ? 1 : 0);
  kvz_init_ver_filter_taps(ver_fir_t, taps);

  for (y = 0; y + 7 < height; y += 8) {
    for (x = 0; x + 7 < width; x += 8) {
      int ypos = y + off_y_fir_t;
      kvz_eight_tap_filter_ver_16bit_8x8_avx2(taps, &hor_hpel_pos[ypos * hor_stride + x], hor_stride, &out_t[y * dst_stride + x], dst_stride);
    }
  }

  if (!sample_off_x) {
    for (y = 0; y < height; ++y) {
      for (x = width - 8; x >= 8; x -= 8) {
        uint64_t chunk = *(uint64_t*)&out_t[y * dst_stride + x - 1];
        *(uint64_t*)&out_t[y * dst_stride + x] = chunk;
      }

      x = 0;
      int16_t sample = kvz_eight_tap_filter_hor_16bit_avx2(ver_fir_t, &col_pos_hor[y + off_y_fir_t]) >> shift2;
      sample = kvz_fast_clip_16bit_to_pixel((sample + wp_offset1) >> wp_shift1);
      uint64_t first = sample;
      uint64_t rest = *(uint64_t*)&out_t[y * dst_stride + x];
      uint64_t chunk = (rest << 8) | first;
      *(uint64_t*)&out_t[y * dst_stride + x] = chunk;
    }
  }

  // Bottom QPEL (3/4 or 1/4 y positions)
  // Filter block and then filter column and align if neccessary
  kvz_init_ver_filter_taps(ver_fir_b, taps);

  for (y = 0; y + 7 < height; y += 8) {
    for (x = 0; x + 7 < width; x += 8) {
      int ypos = y + off_y_fir_b;
      kvz_eight_tap_filter_ver_16bit_8x8_avx2(taps, &hor_hpel_pos[ypos * hor_stride + x], hor_stride, &out_b[y * dst_stride + x], dst_stride);
    }
  }

  if (!sample_off_x) {
    for (y = 0; y < height; ++y) {
      for (x = width - 8; x >= 8; x -= 8) {
        uint64_t chunk = *(uint64_t*)&out_b[y * dst_stride + x - 1];
        *(uint64_t*)&out_b[y * dst_stride + x] = chunk;
      }

      x = 0;
      int16_t sample = kvz_eight_tap_filter_hor_16bit_avx2(ver_fir_b, &col_pos_hor[y + off_y_fir_b]) >> shift2;
      sample = kvz_fast_clip_16bit_to_pixel((sample + wp_offset1) >> wp_shift1);
      uint64_t first = sample;
      uint64_t rest = *(uint64_t*)&out_b[y * dst_stride + x];
      uint64_t chunk = (rest << 8) | first;
      *(uint64_t*)&out_b[y * dst_stride + x] = chunk;
    }
  }
}

static void kvz_filter_qpel_blocks_diag_luma_avx2(const encoder_control_t * encoder,
  kvz_pixel *src,
  int16_t src_stride,
  int width,
  int height,
  kvz_pixel filtered[4][LCU_WIDTH * LCU_WIDTH],
  int16_t hor_intermediate[5][(KVZ_EXT_BLOCK_W_LUMA + 1) * LCU_WIDTH],
  int8_t fme_level,
  int16_t hor_first_cols[5][KVZ_EXT_BLOCK_W_LUMA + 1],
  int8_t hpel_off_x, int8_t hpel_off_y)
{
  int x, y;

  // Interpolation filter shifts
  int32_t shift2 = 6;

  // Weighted prediction offset and shift
  int32_t wp_shift1 = 14 - KVZ_BIT_DEPTH;
  int32_t wp_offset1 = 1 << (wp_shift1 - 1);

  int8_t *fir1 = kvz_g_luma_filter[1];
  int8_t *fir3 = kvz_g_luma_filter[3];

  int16_t *hor_pos_l = hor_intermediate[3];
  int16_t *hor_pos_r = hor_intermediate[4];

  int16_t *col_pos_l = hor_first_cols[1];
  int16_t *col_pos_r = hor_first_cols[3];

  int16_t dst_stride = LCU_WIDTH;
  int16_t hor_stride = LCU_WIDTH;

  // VERTICAL STEP
  kvz_pixel *out_tl = filtered[0];
  kvz_pixel *out_tr = filtered[1];
  kvz_pixel *out_bl = filtered[2];
  kvz_pixel *out_br = filtered[3];

  int8_t *ver_fir_t = hpel_off_y != 0 ? fir1 : fir3;
  int8_t *ver_fir_b = hpel_off_y != 0 ? fir3 : fir1;

  // Specify if integer pixels are filtered from left or/and top integer samples
  int off_x_fir_l = hpel_off_x < 1 ? 0 : 1;
  int off_x_fir_r = hpel_off_x < 0 ? 0 : 1;
  int off_y_fir_t = hpel_off_y < 1 ? 0 : 1;
  int off_y_fir_b = hpel_off_y < 0 ? 0 : 1;

  __m256i taps[4];
  // Top-left QPEL
  // Filter block and then filter column and align if neccessary
  kvz_init_ver_filter_taps(ver_fir_t, taps);

  for (y = 0; y + 7 < height; y += 8) {
    for (x = 0; x + 7 < width; x += 8) {
      int ypos = y + off_y_fir_t;
      kvz_eight_tap_filter_ver_16bit_8x8_avx2(taps, &hor_pos_l[ypos * hor_stride + x], hor_stride, &out_tl[y * dst_stride + x], dst_stride);
    }
  }

  if (!off_x_fir_l) {
    for (y = 0; y < height; ++y) {
      for (x = width - 8; x >= 8; x -= 8) {
        uint64_t chunk = *(uint64_t*)&out_tl[y * dst_stride + x - 1];
        *(uint64_t*)&out_tl[y * dst_stride + x] = chunk;
      }

      x = 0;
      int16_t sample = kvz_eight_tap_filter_hor_16bit_avx2(ver_fir_t, &col_pos_l[y + off_y_fir_t]) >> shift2;
      sample = kvz_fast_clip_16bit_to_pixel((sample + wp_offset1) >> wp_shift1);
      uint64_t first = sample;
      uint64_t rest = *(uint64_t*)&out_tl[y * dst_stride + x];
      uint64_t chunk = (rest << 8) | first;
      *(uint64_t*)&out_tl[y * dst_stride + x] = chunk;
    }
  }

  // Top-right QPEL
  // Filter block and then filter column and align if neccessary

  for (y = 0; y + 7 < height; y += 8) {
    for (x = 0; x + 7 < width; x += 8) {
      int ypos = y + off_y_fir_t;
      kvz_eight_tap_filter_ver_16bit_8x8_avx2(taps, &hor_pos_r[ypos * hor_stride + x], hor_stride, &out_tr[y * dst_stride + x], dst_stride);
    }
  }

  if (!off_x_fir_r) {
    for (y = 0; y < height; ++y) {
      for (x = width - 8; x >= 8; x -= 8) {
        uint64_t chunk = *(uint64_t*)&out_tr[y * dst_stride + x - 1];
        *(uint64_t*)&out_tr[y * dst_stride + x] = chunk;
      }

      x = 0;
      int16_t sample = kvz_eight_tap_filter_hor_16bit_avx2(ver_fir_t, &col_pos_r[y + off_y_fir_t]) >> shift2;
      sample = kvz_fast_clip_16bit_to_pixel((sample + wp_offset1) >> wp_shift1);
      uint64_t first = sample;
      uint64_t rest = *(uint64_t*)&out_tr[y * dst_stride + x];
      uint64_t chunk = (rest << 8) | first;
      *(uint64_t*)&out_tr[y * dst_stride + x] = chunk;
    }
  }

  // Bottom-left QPEL
  // Filter block and then filter column and align if neccessary
  kvz_init_ver_filter_taps(ver_fir_b, taps);

  for (y = 0; y + 7 < height; y += 8) {
    for (x = 0; x + 7 < width; x += 8) {
      int ypos = y + off_y_fir_b;
      kvz_eight_tap_filter_ver_16bit_8x8_avx2(taps, &hor_pos_l[ypos * hor_stride + x], hor_stride, &out_bl[y * dst_stride + x], dst_stride);
    }
  }

  if (!off_x_fir_l) {
    for (y = 0; y < height; ++y) {
      for (x = width - 8; x >= 8; x -= 8) {
        uint64_t chunk = *(uint64_t*)&out_bl[y * dst_stride + x - 1];
        *(uint64_t*)&out_bl[y * dst_stride + x] = chunk;
      }

      x = 0;
      int16_t sample = kvz_eight_tap_filter_hor_16bit_avx2(ver_fir_b, &col_pos_l[y + off_y_fir_b]) >> shift2;
      sample = kvz_fast_clip_16bit_to_pixel((sample + wp_offset1) >> wp_shift1);
      uint64_t first = sample;
      uint64_t rest = *(uint64_t*)&out_bl[y * dst_stride + x];
      uint64_t chunk = (rest << 8) | first;
      *(uint64_t*)&out_bl[y * dst_stride + x] = chunk;
    }
  }

  // Bottom-right QPEL
  // Filter block and then filter column and align if neccessary
  for (y = 0; y + 7 < height; y += 8) {
    for (x = 0; x + 7 < width; x += 8) {
      int ypos = y + off_y_fir_b;
      kvz_eight_tap_filter_ver_16bit_8x8_avx2(taps, &hor_pos_r[ypos * hor_stride + x], hor_stride, &out_br[y * dst_stride + x], dst_stride);
    }
  }

  if (!off_x_fir_r) {
    for (y = 0; y < height; ++y) {
      for (x = width - 8; x >= 8; x -= 8) {
        uint64_t chunk = *(uint64_t*)&out_br[y * dst_stride + x - 1];
        *(uint64_t*)&out_br[y * dst_stride + x] = chunk;
      }

      x = 0;
      int16_t sample = kvz_eight_tap_filter_hor_16bit_avx2(ver_fir_b, &col_pos_r[y + off_y_fir_b]) >> shift2;
      sample = kvz_fast_clip_16bit_to_pixel((sample + wp_offset1) >> wp_shift1);
      uint64_t first = sample;
      uint64_t rest = *(uint64_t*)&out_br[y * dst_stride + x];
      uint64_t chunk = (rest << 8) | first;
      *(uint64_t*)&out_br[y * dst_stride + x] = chunk;
    }
  }
}

static void kvz_sample_quarterpel_luma_avx2(const encoder_control_t * const encoder,
  kvz_pixel *src, 
  int16_t src_stride, 
  int width, 
  int height, 
  kvz_pixel *dst, 
  int16_t dst_stride, 
  int8_t hor_flag, 
  int8_t ver_flag, 
  const int16_t mv[2])
{
  // TODO: Optimize SMP and AMP
  if (width != height) {
    kvz_sample_quarterpel_luma_generic(encoder, src, src_stride, width, height, dst, dst_stride, hor_flag, ver_flag, mv);
    return;
  }

  int x, y;

  int8_t *hor_fir = kvz_g_luma_filter[mv[0] & 3];
  int8_t *ver_fir = kvz_g_luma_filter[mv[1] & 3];

  int16_t hor_stride = LCU_WIDTH;
  int16_t hor_intermediate[KVZ_EXT_BLOCK_W_LUMA * LCU_WIDTH];

  // HORIZONTAL STEP
  __m256i shuf_01_23, shuf_45_67;
  __m256i taps_01_23, taps_45_67;

  kvz_init_shuffle_masks(&shuf_01_23, &shuf_45_67);
  kvz_init_filter_taps(hor_fir, &taps_01_23, &taps_45_67);

  for (y = 0; y < height + KVZ_EXT_PADDING_LUMA; ++y) {

    for (x = 0; x + 7 < width; x += 8) {
      int ypos = y - KVZ_LUMA_FILTER_OFFSET;
      int xpos = x - KVZ_LUMA_FILTER_OFFSET;
      kvz_eight_tap_filter_hor_8x1_avx2(&src[src_stride*ypos + xpos], &hor_intermediate[y * hor_stride + x],
        &shuf_01_23, &shuf_45_67,
        &taps_01_23, &taps_45_67); //TODO: >> shift1
    }
  }

  // VERTICAL STEP
  __m256i taps[4];
  kvz_init_ver_filter_taps(ver_fir, taps);

  for (y = 0; y + 7 < height; y += 8) {
    for (x = 0; x + 7 < width; x += 8) {
      kvz_eight_tap_filter_ver_16bit_8x8_avx2(taps, &hor_intermediate[y * hor_stride + x], hor_stride, &dst[y * dst_stride + x], dst_stride);
    }
  }
}

static void kvz_sample_14bit_quarterpel_luma_avx2(const encoder_control_t * const encoder,
  kvz_pixel *src, 
  int16_t src_stride, 
  int width, 
  int height, 
  int16_t *dst, 
  int16_t dst_stride, 
  int8_t hor_flag, 
  int8_t ver_flag, 
  const int16_t mv[2])
{
  // TODO: Optimize SMP and AMP
  if (width != height) {
    kvz_sample_14bit_quarterpel_luma_generic(encoder, src, src_stride, width, height, dst, dst_stride, hor_flag, ver_flag, mv);
    return;
  }
  // TODO: horizontal and vertical only filtering
  int x, y;

  int8_t *hor_fir = kvz_g_luma_filter[mv[0] & 3];
  int8_t *ver_fir = kvz_g_luma_filter[mv[1] & 3];

  int16_t hor_stride = LCU_WIDTH;
  int16_t hor_intermediate[KVZ_EXT_BLOCK_W_LUMA * LCU_WIDTH];

  // HORIZONTAL STEP
  __m256i shuf_01_23, shuf_45_67;
  __m256i taps_01_23, taps_45_67;

  kvz_init_shuffle_masks(&shuf_01_23, &shuf_45_67);
  kvz_init_filter_taps(hor_fir, &taps_01_23, &taps_45_67);

  for (y = 0; y < height + KVZ_EXT_PADDING_LUMA; ++y) {

    for (x = 0; x + 7 < width; x += 8) {
      int ypos = y - KVZ_LUMA_FILTER_OFFSET;
      int xpos = x - KVZ_LUMA_FILTER_OFFSET;
      kvz_eight_tap_filter_hor_8x1_avx2(&src[src_stride*ypos + xpos], &hor_intermediate[y * hor_stride + x],
        &shuf_01_23, &shuf_45_67,
        &taps_01_23, &taps_45_67); //TODO: >> shift1
    }
  }

  // VERTICAL STEP
  __m256i taps[4];
  kvz_init_ver_filter_taps(ver_fir, taps);

  for (y = 0; y + 7 < height; y += 8) {
    for (x = 0; x + 7 < width; x += 8) {
      kvz_eight_tap_filter_ver_16bit_8x8_no_round_avx2(taps, &hor_intermediate[y * hor_stride + x], hor_stride, &dst[y * dst_stride + x], dst_stride);
    }
  }
}


static void kvz_sample_octpel_chroma_avx2(const encoder_control_t * const encoder,
  kvz_pixel *src,
  int16_t src_stride,
  int width,
  int height,
  kvz_pixel *dst,
  int16_t dst_stride,
  int8_t hor_flag,
  int8_t ver_flag,
  const int16_t mv[2])
{
  // TODO: Optimize SMP and AMP
  if (width != height) {
    kvz_sample_octpel_chroma_generic(encoder, src, src_stride, width, height, dst, dst_stride, hor_flag, ver_flag, mv);
    return;
  }
  int x, y;

  int8_t *hor_fir = kvz_g_chroma_filter[mv[0] & 7];
  int8_t *ver_fir = kvz_g_chroma_filter[mv[1] & 7];

  int16_t hor_stride = LCU_WIDTH_C;
  int16_t hor_intermediate[KVZ_EXT_BLOCK_W_CHROMA * LCU_WIDTH_C];

  // HORIZONTAL STEP
  __m256i shuf_01, shuf_23;
  __m256i taps_01, taps_23;

  kvz_init_shuffle_masks_chroma(&shuf_01, &shuf_23);
  kvz_init_filter_taps_chroma(hor_fir, &taps_01, &taps_23);

  for (y = 0; y + 3 < height + KVZ_EXT_PADDING_CHROMA; y += 4) {

    for (x = 0; x + 3 < width; x += 4) {
      int ypos = y - KVZ_CHROMA_FILTER_OFFSET;
      int xpos = x - KVZ_CHROMA_FILTER_OFFSET;
      kvz_four_tap_filter_hor_4x4_avx2(&src[src_stride*ypos + xpos], src_stride, &hor_intermediate[y * hor_stride + x], hor_stride,
        &shuf_01, &shuf_23,
        &taps_01, &taps_23); //TODO: >> shift1
    }
  }

  __m256i shuf_01_23 = _mm256_permute2x128_si256(shuf_01, shuf_23, _MM_SHUFFLE(0, 2, 0, 0));
  __m256i taps_01_23 = _mm256_permute2x128_si256(taps_01, taps_23, _MM_SHUFFLE(0, 2, 0, 0));
  
  int rows = 3;
  for (x = 0; x + 3 < width; x += 4) {
    int ypos = y - KVZ_CHROMA_FILTER_OFFSET;
    int xpos = x - KVZ_CHROMA_FILTER_OFFSET;
    kvz_four_tap_filter_hor_4xN_avx2(&src[src_stride*ypos + xpos], src_stride, &hor_intermediate[y * hor_stride + x], hor_stride,
      &shuf_01_23, &taps_01_23,
      rows); //TODO: >> shift1
  }

  // VERTICAL STEP
  for (y = 0; y + 3 < height; y += 4) {
    for (x = 0; x + 3 < width; x += 4) {
      kvz_four_tap_filter_ver_16bit_4x4_avx2(ver_fir, &hor_intermediate[y * hor_stride + x], hor_stride, &dst[y * dst_stride + x], dst_stride);
    }
  }
}

static void kvz_sample_14bit_octpel_chroma_avx2(const encoder_control_t * const encoder,
  kvz_pixel *src, 
  int16_t src_stride, 
  int width, 
  int height, 
  int16_t *dst, 
  int16_t dst_stride, 
  int8_t hor_flag, 
  int8_t ver_flag, 
  const int16_t mv[2])
{
  // TODO: Optimize SMP and AMP
  if (width != height) {
    kvz_sample_14bit_octpel_chroma_generic(encoder, src, src_stride, width, height, dst, dst_stride, hor_flag, ver_flag, mv);
    return;
  }
  // TODO: horizontal and vertical only filtering
  int x, y;

  int8_t *hor_fir = kvz_g_chroma_filter[mv[0] & 7];
  int8_t *ver_fir = kvz_g_chroma_filter[mv[1] & 7];

  int16_t hor_stride = LCU_WIDTH_C;
  int16_t hor_intermediate[KVZ_EXT_BLOCK_W_CHROMA * LCU_WIDTH_C];

  // HORIZONTAL STEP
  __m256i shuf_01, shuf_23;
  __m256i taps_01, taps_23;

  kvz_init_shuffle_masks_chroma(&shuf_01, &shuf_23);
  kvz_init_filter_taps_chroma(hor_fir, &taps_01, &taps_23);

  for (y = 0; y + 3 < height + KVZ_EXT_PADDING_CHROMA; y += 4) {

    for (x = 0; x + 3 < width; x += 4) {
      int ypos = y - KVZ_CHROMA_FILTER_OFFSET;
      int xpos = x - KVZ_CHROMA_FILTER_OFFSET;
      kvz_four_tap_filter_hor_4x4_avx2(&src[src_stride*ypos + xpos], src_stride, &hor_intermediate[y * hor_stride + x], hor_stride,
        &shuf_01, &shuf_23,
        &taps_01, &taps_23); //TODO: >> shift1
    }
  }

  __m256i shuf_01_23 = _mm256_permute2x128_si256(shuf_01, shuf_23, _MM_SHUFFLE(0, 2, 0, 0));
  __m256i taps_01_23 = _mm256_permute2x128_si256(taps_01, taps_23, _MM_SHUFFLE(0, 2, 0, 0));
  
  int rows = 3;
  for (x = 0; x + 3 < width; x += 4) {
    int ypos = y - KVZ_CHROMA_FILTER_OFFSET;
    int xpos = x - KVZ_CHROMA_FILTER_OFFSET;
    kvz_four_tap_filter_hor_4xN_avx2(&src[src_stride*ypos + xpos], src_stride, &hor_intermediate[y * hor_stride + x], hor_stride,
      &shuf_01_23, &taps_01_23,
      rows); //TODO: >> shift1
  }

  // VERTICAL STEP
  for (y = 0; y + 3 < height; y += 4) {
    for (x = 0; x + 3 < width; x += 4) {
      kvz_four_tap_filter_ver_16bit_4x4_no_round_avx2(ver_fir, &hor_intermediate[y * hor_stride + x], hor_stride, &dst[y * dst_stride + x], dst_stride);
    }
  }
}

void kvz_get_extended_block_avx2(int xpos, int ypos, int mv_x, int mv_y, int off_x, int off_y, kvz_pixel *ref, int ref_width, int ref_height,
  int filter_size, int width, int height, kvz_extended_block *out) {

  int half_filter_size = filter_size >> 1;

  out->buffer = ref + (ypos - half_filter_size + off_y + mv_y) * ref_width + (xpos - half_filter_size + off_x + mv_x);
  out->stride = ref_width;
  out->orig_topleft = out->buffer + out->stride * half_filter_size + half_filter_size;
  out->malloc_used = 0;

  int min_y = ypos - half_filter_size + off_y + mv_y;
  int max_y = min_y + height + filter_size;
  int out_of_bounds_y = (min_y < 0) || (max_y >= ref_height);

  int min_x = xpos - half_filter_size + off_x + mv_x;
  int max_x = min_x + width + filter_size;
  int out_of_bounds_x = (min_x < 0) || (max_x >= ref_width);

  int sample_out_of_bounds = out_of_bounds_y || out_of_bounds_x;

  if (sample_out_of_bounds){
    // Alloc 5 pixels more than we actually use because AVX2 filter
    // functions read up to 5 pixels past the last pixel.
    out->buffer = MALLOC(kvz_pixel, (width + filter_size) * (height + filter_size) + 5);
    if (!out->buffer){
      fprintf(stderr, "Memory allocation failed!\n");
      assert(0);
    }
    out->stride = width + filter_size;
    out->orig_topleft = out->buffer + out->stride * half_filter_size + half_filter_size;
    out->malloc_used = 1;

    int dst_y; int y; int dst_x; int x; int coord_x; int coord_y;

    for (dst_y = 0, y = ypos - half_filter_size; y < ((ypos + height)) + half_filter_size; dst_y++, y++) {

      // calculate y-pixel offset
      coord_y = y + off_y + mv_y;
      coord_y = CLIP(0, (ref_height)-1, coord_y);
      coord_y *= ref_width;

      if (!out_of_bounds_x){
        memcpy(&out->buffer[dst_y * out->stride + 0], &ref[coord_y + min_x], out->stride * sizeof(kvz_pixel));
      } else {
        for (dst_x = 0, x = (xpos)-half_filter_size; x < ((xpos + width)) + half_filter_size; dst_x++, x++) {

          coord_x = x + off_x + mv_x;
          coord_x = CLIP(0, (ref_width)-1, coord_x);

          // Store source block data (with extended borders)
          out->buffer[dst_y * out->stride + dst_x] = ref[coord_y + coord_x];
        }
      }
    }
  }
}

#endif //COMPILE_INTEL_AVX2

int kvz_strategy_register_ipol_avx2(void* opaque, uint8_t bitdepth)
{
  bool success = true;
#if COMPILE_INTEL_AVX2
  if (bitdepth == 8){
    success &= kvz_strategyselector_register(opaque, "filter_hpel_blocks_hor_ver_luma", "avx2", 40, &kvz_filter_hpel_blocks_hor_ver_luma_avx2);
    success &= kvz_strategyselector_register(opaque, "filter_hpel_blocks_diag_luma", "avx2", 40, &kvz_filter_hpel_blocks_diag_luma_avx2);
    success &= kvz_strategyselector_register(opaque, "filter_qpel_blocks_hor_ver_luma", "avx2", 40, &kvz_filter_qpel_blocks_hor_ver_luma_avx2);
    success &= kvz_strategyselector_register(opaque, "filter_qpel_blocks_diag_luma", "avx2", 40, &kvz_filter_qpel_blocks_diag_luma_avx2);
    success &= kvz_strategyselector_register(opaque, "sample_quarterpel_luma", "avx2", 40, &kvz_sample_quarterpel_luma_avx2);
    success &= kvz_strategyselector_register(opaque, "sample_octpel_chroma", "avx2", 40, &kvz_sample_octpel_chroma_avx2);
    success &= kvz_strategyselector_register(opaque, "sample_14bit_quarterpel_luma", "avx2", 40, &kvz_sample_14bit_quarterpel_luma_avx2);
    success &= kvz_strategyselector_register(opaque, "sample_14bit_octpel_chroma", "avx2", 40, &kvz_sample_14bit_octpel_chroma_avx2);
  }
  success &= kvz_strategyselector_register(opaque, "get_extended_block", "avx2", 40, &kvz_get_extended_block_avx2);
#endif //COMPILE_INTEL_AVX2
  return success;
}
