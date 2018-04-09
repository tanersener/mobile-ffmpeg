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
#include "strategies/generic/picture-generic.h"
#include "strategies/strategies-ipol.h"
#include "strategyselector.h"
#include "strategies/generic/ipol-generic.h"


#define FILTER_OFFSET 3
#define FILTER_SIZE 8

#define MAX_HEIGHT (4 * (LCU_WIDTH + 1) + FILTER_SIZE)
#define MAX_WIDTH ((LCU_WIDTH + 1) + FILTER_SIZE)

extern int8_t kvz_g_luma_filter[4][8];
extern int8_t kvz_g_chroma_filter[8][4];

void kvz_eight_tap_filter_x8_and_flip(__m128i *data01, __m128i *data23, __m128i *data45, __m128i *data67, __m128i *filter, __m128i *dst)
{
  __m128i a, b, c, d;
  __m128i fir = _mm_broadcastq_epi64(_mm_loadl_epi64(filter));

  a = _mm_maddubs_epi16(*data01, fir);
  b = _mm_maddubs_epi16(*data23, fir);
  a = _mm_hadd_epi16(a, b);

  c = _mm_maddubs_epi16(*data45, fir);
  d = _mm_maddubs_epi16(*data67, fir);
  c = _mm_hadd_epi16(c, d);

  a = _mm_hadd_epi16(a, c);

  _mm_storeu_si128(dst, a);
}

static __m128i kvz_eight_tap_filter_flip_x8_16bit_avx2(__m128i *row, int8_t *filter, int32_t offset23, int32_t shift23)
{
  __m128i temp[8];
  __m128i temp_lo;
  __m128i temp_hi;
  __m128i fir = _mm_cvtepi8_epi16(_mm_loadl_epi64((__m128i*)filter));

  temp[0] = _mm_madd_epi16(row[0], fir);
  temp[1] = _mm_madd_epi16(row[1], fir);
  temp_lo = _mm_unpacklo_epi32(temp[0], temp[1]);
  temp_hi = _mm_unpackhi_epi32(temp[0], temp[1]);
  temp[0] = _mm_add_epi32(temp_lo, temp_hi);

  temp[2] = _mm_madd_epi16(row[2], fir);
  temp[3] = _mm_madd_epi16(row[3], fir);
  temp_lo = _mm_unpacklo_epi32(temp[2], temp[3]);
  temp_hi = _mm_unpackhi_epi32(temp[2], temp[3]);
  temp[2] = _mm_add_epi32(temp_lo, temp_hi);

  temp[4] = _mm_madd_epi16(row[4], fir);
  temp[5] = _mm_madd_epi16(row[5], fir);
  temp_lo = _mm_unpacklo_epi32(temp[4], temp[5]);
  temp_hi = _mm_unpackhi_epi32(temp[4], temp[5]);
  temp[4] = _mm_add_epi32(temp_lo, temp_hi);

  temp[6] = _mm_madd_epi16(row[6], fir);
  temp[7] = _mm_madd_epi16(row[7], fir);
  temp_lo = _mm_unpacklo_epi32(temp[6], temp[7]);
  temp_hi = _mm_unpackhi_epi32(temp[6], temp[7]);
  temp[6] = _mm_add_epi32(temp_lo, temp_hi);

  temp_lo = _mm_unpacklo_epi32(temp[0], temp[2]);
  temp_hi = _mm_unpackhi_epi32(temp[0], temp[2]);
  temp[0] = _mm_add_epi32(temp_lo, temp_hi);
  temp[0] = _mm_shuffle_epi32(temp[0], _MM_SHUFFLE(3, 1, 2, 0));

  temp_lo = _mm_unpacklo_epi32(temp[4], temp[6]);
  temp_hi = _mm_unpackhi_epi32(temp[4], temp[6]);
  temp[4] = _mm_add_epi32(temp_lo, temp_hi);
  temp[4] = _mm_shuffle_epi32(temp[4], _MM_SHUFFLE(3, 1, 2, 0));

  __m128i add = _mm_set1_epi32(offset23);
  temp[0] = _mm_add_epi32(temp[0], add);
  temp[4] = _mm_add_epi32(temp[4], add);
  temp[0] = _mm_srai_epi32(temp[0], shift23);
  temp[4] = _mm_srai_epi32(temp[4], shift23);

  temp[0] = _mm_packus_epi32(temp[0], temp[4]);
  temp[0] = _mm_packus_epi16(temp[0], temp[0]);

  return temp[0];
}

static __m256i kvz_eight_tap_filter_flip_x8_16bit_dual_avx2(__m256i *row, int8_t *filter[2], int32_t offset23, int32_t shift23)
{
  __m256i temp[8];
  __m256i temp_lo;
  __m256i temp_hi;
  __m256i fir = _mm256_cvtepi8_epi16(_mm_unpacklo_epi64(_mm_loadl_epi64((__m128i*)filter[0]), _mm_loadl_epi64((__m128i*)filter[1])));

  temp[0] = _mm256_madd_epi16(row[0], fir);
  temp[1] = _mm256_madd_epi16(row[1], fir);
  temp_lo = _mm256_unpacklo_epi32(temp[0], temp[1]);
  temp_hi = _mm256_unpackhi_epi32(temp[0], temp[1]);
  temp[0] = _mm256_add_epi32(temp_lo, temp_hi);

  temp[2] = _mm256_madd_epi16(row[2], fir);
  temp[3] = _mm256_madd_epi16(row[3], fir);
  temp_lo = _mm256_unpacklo_epi32(temp[2], temp[3]);
  temp_hi = _mm256_unpackhi_epi32(temp[2], temp[3]);
  temp[2] = _mm256_add_epi32(temp_lo, temp_hi);

  temp[4] = _mm256_madd_epi16(row[4], fir);
  temp[5] = _mm256_madd_epi16(row[5], fir);
  temp_lo = _mm256_unpacklo_epi32(temp[4], temp[5]);
  temp_hi = _mm256_unpackhi_epi32(temp[4], temp[5]);
  temp[4] = _mm256_add_epi32(temp_lo, temp_hi);

  temp[6] = _mm256_madd_epi16(row[6], fir);
  temp[7] = _mm256_madd_epi16(row[7], fir);
  temp_lo = _mm256_unpacklo_epi32(temp[6], temp[7]);
  temp_hi = _mm256_unpackhi_epi32(temp[6], temp[7]);
  temp[6] = _mm256_add_epi32(temp_lo, temp_hi);

  temp_lo = _mm256_unpacklo_epi32(temp[0], temp[2]);
  temp_hi = _mm256_unpackhi_epi32(temp[0], temp[2]);
  temp[0] = _mm256_add_epi32(temp_lo, temp_hi);
  temp[0] = _mm256_shuffle_epi32(temp[0], _MM_SHUFFLE(3, 1, 2, 0));

  temp_lo = _mm256_unpacklo_epi32(temp[4], temp[6]);
  temp_hi = _mm256_unpackhi_epi32(temp[4], temp[6]);
  temp[4] = _mm256_add_epi32(temp_lo, temp_hi);
  temp[4] = _mm256_shuffle_epi32(temp[4], _MM_SHUFFLE(3, 1, 2, 0));

  __m256i add = _mm256_set1_epi32(offset23);
  temp[0] = _mm256_add_epi32(temp[0], add);
  temp[4] = _mm256_add_epi32(temp[4], add);
  temp[0] = _mm256_srai_epi32(temp[0], shift23);
  temp[4] = _mm256_srai_epi32(temp[4], shift23);

  temp[0] = _mm256_packus_epi32(temp[0], temp[4]);
  temp[0] = _mm256_packus_epi16(temp[0], temp[0]);

  return temp[0];
}

/*
static __m128i kvz_eight_tap_filter_flip_x8_avx2(__m128i *row, int8_t *filter,  int32_t shift1)
{
  __m128i temp[4];
  __m128i fir = _mm_broadcastq_epi64(_mm_loadl_epi64((__m128i*)filter));
  
  temp[0] = _mm_unpacklo_epi64(row[0], row[1]);
  temp[0] = _mm_maddubs_epi16(temp[0], fir);

  temp[1] = _mm_unpacklo_epi64(row[2], row[3]);
  temp[1] = _mm_maddubs_epi16(temp[1], fir);

  temp[0] = _mm_hadd_epi16(temp[0], temp[1]);

  temp[2] = _mm_unpacklo_epi64(row[4], row[5]);
  temp[2] = _mm_maddubs_epi16(temp[2], fir);

  temp[3] = _mm_unpacklo_epi64(row[6], row[7]);
  temp[3] = _mm_maddubs_epi16(temp[3], fir);
  
  temp[2] = _mm_hadd_epi16(temp[2], temp[3]);

  temp[0] = _mm_hadd_epi16(temp[0], temp[2]);

  temp[0] = _mm_srai_epi16(temp[0], shift1);

  return temp[0];
}
*/

static __m256i kvz_eight_tap_filter_flip_x8_dual_avx2(__m256i *row, int8_t *filter[2],  int32_t shift1)
{
  __m256i temp[4];
  __m256i fir = _mm256_inserti128_si256(_mm256_castsi128_si256(_mm_loadl_epi64((__m128i*)filter[0])), _mm_loadl_epi64((__m128i*)filter[1]), 1);
  fir = _mm256_shuffle_epi32(fir, _MM_SHUFFLE(1, 0, 1, 0));
  
  temp[0] = _mm256_unpacklo_epi64(row[0], row[1]);
  temp[0] = _mm256_maddubs_epi16(temp[0], fir);

  temp[1] = _mm256_unpacklo_epi64(row[2], row[3]);
  temp[1] = _mm256_maddubs_epi16(temp[1], fir);

  temp[0] = _mm256_hadd_epi16(temp[0], temp[1]);

  temp[2] = _mm256_unpacklo_epi64(row[4], row[5]);
  temp[2] = _mm256_maddubs_epi16(temp[2], fir);

  temp[3] = _mm256_unpacklo_epi64(row[6], row[7]);
  temp[3] = _mm256_maddubs_epi16(temp[3], fir);
  
  temp[2] = _mm256_hadd_epi16(temp[2], temp[3]);

  temp[0] = _mm256_hadd_epi16(temp[0], temp[2]);

  temp[0] = _mm256_srai_epi16(temp[0], shift1);

  return temp[0];
}

/*
static INLINE void kvz_filter_flip_shift_x8_avx2(kvz_pixel *src, int16_t src_stride, int8_t *filter, int32_t shift1, int16_t *dst){

  __m128i rows[8];
  rows[0] = _mm_loadl_epi64((__m128i*)(src + 0 * src_stride));
  rows[1] = _mm_loadl_epi64((__m128i*)(src + 1 * src_stride));
  rows[2] = _mm_loadl_epi64((__m128i*)(src + 2 * src_stride));
  rows[3] = _mm_loadl_epi64((__m128i*)(src + 3 * src_stride));
  rows[4] = _mm_loadl_epi64((__m128i*)(src + 4 * src_stride));
  rows[5] = _mm_loadl_epi64((__m128i*)(src + 5 * src_stride));
  rows[6] = _mm_loadl_epi64((__m128i*)(src + 6 * src_stride));
  rows[7] = _mm_loadl_epi64((__m128i*)(src + 7 * src_stride));
  __m128i out = kvz_eight_tap_filter_flip_x8_avx2(rows, filter, shift1);
  _mm_storeu_si128((__m128i*)dst,  out);
}
*/

static INLINE void kvz_filter_flip_shift_x8_dual_avx2(kvz_pixel *src, int16_t src_stride, int8_t *firs[2], int32_t shift1, int16_t *dst[2]){

  __m256i rows[8];
  rows[0] = _mm256_broadcastsi128_si256(_mm_loadl_epi64((__m128i*)(src + 0 * src_stride)));
  rows[1] = _mm256_broadcastsi128_si256(_mm_loadl_epi64((__m128i*)(src + 1 * src_stride)));
  rows[2] = _mm256_broadcastsi128_si256(_mm_loadl_epi64((__m128i*)(src + 2 * src_stride)));
  rows[3] = _mm256_broadcastsi128_si256(_mm_loadl_epi64((__m128i*)(src + 3 * src_stride)));
  rows[4] = _mm256_broadcastsi128_si256(_mm_loadl_epi64((__m128i*)(src + 4 * src_stride)));
  rows[5] = _mm256_broadcastsi128_si256(_mm_loadl_epi64((__m128i*)(src + 5 * src_stride)));
  rows[6] = _mm256_broadcastsi128_si256(_mm_loadl_epi64((__m128i*)(src + 6 * src_stride)));
  rows[7] = _mm256_broadcastsi128_si256(_mm_loadl_epi64((__m128i*)(src + 7 * src_stride)));
  __m256i out = kvz_eight_tap_filter_flip_x8_dual_avx2(rows, firs, shift1);
  _mm_storeu_si128((__m128i*)dst[0],  _mm256_castsi256_si128(out));
  _mm_storeu_si128((__m128i*)dst[1],  _mm256_extracti128_si256(out, 1));
}

static INLINE void kvz_filter_flip_round_clip_x8_16bit_avx2(int16_t *flipped_filtered, int16_t src_stride, int8_t *filter, int32_t offset23, int32_t shift23, kvz_pixel *dst){

  __m128i rows[8];
  rows[0] = _mm_loadu_si128((__m128i*)(flipped_filtered + 0 * src_stride));
  rows[1] = _mm_loadu_si128((__m128i*)(flipped_filtered + 1 * src_stride));
  rows[2] = _mm_loadu_si128((__m128i*)(flipped_filtered + 2 * src_stride));
  rows[3] = _mm_loadu_si128((__m128i*)(flipped_filtered + 3 * src_stride));
  rows[4] = _mm_loadu_si128((__m128i*)(flipped_filtered + 4 * src_stride));
  rows[5] = _mm_loadu_si128((__m128i*)(flipped_filtered + 5 * src_stride));
  rows[6] = _mm_loadu_si128((__m128i*)(flipped_filtered + 6 * src_stride));
  rows[7] = _mm_loadu_si128((__m128i*)(flipped_filtered + 7 * src_stride));
  _mm_storel_epi64((__m128i*)dst, kvz_eight_tap_filter_flip_x8_16bit_avx2(rows, filter, offset23, shift23) );
}

static INLINE void kvz_filter_flip_round_clip_x8_16bit_dual_avx2(int16_t *flipped_filtered[2], int16_t src_stride, int8_t *firs[2], int32_t offset23, int32_t shift23, kvz_pixel *dst[2]){

  __m256i rows[8];
  rows[0] = _mm256_inserti128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)(flipped_filtered[0] + 0 * src_stride))), _mm_loadu_si128((__m128i*)(flipped_filtered[1] + 0 * src_stride)), 1);
  rows[1] = _mm256_inserti128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)(flipped_filtered[0] + 1 * src_stride))), _mm_loadu_si128((__m128i*)(flipped_filtered[1] + 1 * src_stride)), 1);
  rows[2] = _mm256_inserti128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)(flipped_filtered[0] + 2 * src_stride))), _mm_loadu_si128((__m128i*)(flipped_filtered[1] + 2 * src_stride)), 1);
  rows[3] = _mm256_inserti128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)(flipped_filtered[0] + 3 * src_stride))), _mm_loadu_si128((__m128i*)(flipped_filtered[1] + 3 * src_stride)), 1);
  rows[4] = _mm256_inserti128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)(flipped_filtered[0] + 4 * src_stride))), _mm_loadu_si128((__m128i*)(flipped_filtered[1] + 4 * src_stride)), 1);
  rows[5] = _mm256_inserti128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)(flipped_filtered[0] + 5 * src_stride))), _mm_loadu_si128((__m128i*)(flipped_filtered[1] + 5 * src_stride)), 1);
  rows[6] = _mm256_inserti128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)(flipped_filtered[0] + 6 * src_stride))), _mm_loadu_si128((__m128i*)(flipped_filtered[1] + 6 * src_stride)), 1);
  rows[7] = _mm256_inserti128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)(flipped_filtered[0] + 7 * src_stride))), _mm_loadu_si128((__m128i*)(flipped_filtered[1] + 7 * src_stride)), 1);
  __m256i out = kvz_eight_tap_filter_flip_x8_16bit_dual_avx2(rows, firs, offset23, shift23);
  _mm_storel_epi64((__m128i*)dst[0],  _mm256_castsi256_si128(out));
  _mm_storel_epi64((__m128i*)dst[1],  _mm256_extracti128_si256(out, 1));

}

__m128i kvz_eight_tap_filter_x4_and_flip_16bit(__m128i *data0, __m128i *data1, __m128i *data2, __m128i *data3, __m128i *filter)
{
  __m128i a, b, c, d;
  __m128i fir = _mm_cvtepi8_epi16(_mm_loadu_si128((__m128i*)(filter)));

  a = _mm_madd_epi16(*data0, fir);
  b = _mm_madd_epi16(*data1, fir);
  a = _mm_hadd_epi32(a, b);

  c = _mm_madd_epi16(*data2, fir);
  d = _mm_madd_epi16(*data3, fir);
  c = _mm_hadd_epi32(c, d);

  a = _mm_hadd_epi32(a, c);

  return a;
}

void kvz_eight_tap_filter_and_flip_avx2(int8_t filter[4][8], kvz_pixel *src, int16_t src_stride, int16_t* __restrict dst)
{

  //Load 2 rows per xmm register
  __m128i rows01 = _mm_loadl_epi64((__m128i*)(src + 0 * src_stride));
  rows01 = _mm_castpd_si128(_mm_loadh_pd(_mm_castsi128_pd(rows01), (double*)(src + 1 * src_stride)));

  __m128i rows23 = _mm_loadl_epi64((__m128i*)(src + 2 * src_stride));
  rows23 = _mm_castpd_si128(_mm_loadh_pd(_mm_castsi128_pd(rows23), (double*)(src + 3 * src_stride)));

  __m128i rows45 = _mm_loadl_epi64((__m128i*)(src + 4 * src_stride));
  rows45 = _mm_castpd_si128(_mm_loadh_pd(_mm_castsi128_pd(rows45), (double*)(src + 5 * src_stride)));

  __m128i rows67 = _mm_loadl_epi64((__m128i*)(src + 6 * src_stride));
  rows67 = _mm_castpd_si128(_mm_loadh_pd(_mm_castsi128_pd(rows67), (double*)(src + 7 * src_stride)));

  //Filter rows
  const int dst_stride = MAX_WIDTH;
  kvz_eight_tap_filter_x8_and_flip(&rows01, &rows23, &rows45, &rows67, (__m128i*)(&filter[0]), (__m128i*)(dst + 0));
  kvz_eight_tap_filter_x8_and_flip(&rows01, &rows23, &rows45, &rows67, (__m128i*)(&filter[1]), (__m128i*)(dst + 1 * dst_stride));
  kvz_eight_tap_filter_x8_and_flip(&rows01, &rows23, &rows45, &rows67, (__m128i*)(&filter[2]), (__m128i*)(dst + 2 * dst_stride));
  kvz_eight_tap_filter_x8_and_flip(&rows01, &rows23, &rows45, &rows67, (__m128i*)(&filter[3]), (__m128i*)(dst + 3 * dst_stride));
}

static INLINE void eight_tap_filter_and_flip_16bit_avx2(int8_t filter[4][8], int16_t *src, int16_t src_stride, int offset, int combined_shift, kvz_pixel* __restrict dst, int16_t dst_stride)
{

  //Load a row per xmm register
  __m128i row0 = _mm_loadu_si128((__m128i*)(src + 0 * src_stride));
  __m128i row1 = _mm_loadu_si128((__m128i*)(src + 1 * src_stride));
  __m128i row2 = _mm_loadu_si128((__m128i*)(src + 2 * src_stride));
  __m128i row3 = _mm_loadu_si128((__m128i*)(src + 3 * src_stride));

  //Filter rows
  union {
    __m128i vector;
    int32_t array[4];
  } temp[4];

  temp[0].vector = kvz_eight_tap_filter_x4_and_flip_16bit(&row0, &row1, &row2, &row3, (__m128i*)(&filter[0]));
  temp[1].vector = kvz_eight_tap_filter_x4_and_flip_16bit(&row0, &row1, &row2, &row3, (__m128i*)(&filter[1]));
  temp[2].vector = kvz_eight_tap_filter_x4_and_flip_16bit(&row0, &row1, &row2, &row3, (__m128i*)(&filter[2]));
  temp[3].vector = kvz_eight_tap_filter_x4_and_flip_16bit(&row0, &row1, &row2, &row3, (__m128i*)(&filter[3]));

  __m128i packed_offset = _mm_set1_epi32(offset);

  temp[0].vector = _mm_add_epi32(temp[0].vector, packed_offset);
  temp[0].vector = _mm_srai_epi32(temp[0].vector, combined_shift);
  temp[1].vector = _mm_add_epi32(temp[1].vector, packed_offset);
  temp[1].vector = _mm_srai_epi32(temp[1].vector, combined_shift);

  temp[0].vector = _mm_packus_epi32(temp[0].vector, temp[1].vector);

  temp[2].vector = _mm_add_epi32(temp[2].vector, packed_offset);
  temp[2].vector = _mm_srai_epi32(temp[2].vector, combined_shift);
  temp[3].vector = _mm_add_epi32(temp[3].vector, packed_offset);
  temp[3].vector = _mm_srai_epi32(temp[3].vector, combined_shift);

  temp[2].vector = _mm_packus_epi32(temp[2].vector, temp[3].vector);

  temp[0].vector = _mm_packus_epi16(temp[0].vector, temp[2].vector);

  int32_t* four_pixels = (int32_t*)&(dst[0 * dst_stride]);
  *four_pixels = temp[0].array[0];

  four_pixels = (int32_t*)&(dst[1 * dst_stride]);
  *four_pixels = _mm_extract_epi32(temp[0].vector, 1);

  four_pixels = (int32_t*)&(dst[2 * dst_stride]);
  *four_pixels = _mm_extract_epi32(temp[0].vector, 2);

  four_pixels = (int32_t*)&(dst[3 * dst_stride]);
  *four_pixels = _mm_extract_epi32(temp[0].vector, 3);


}

int16_t kvz_eight_tap_filter_hor_avx2(int8_t *filter, kvz_pixel *data)
{

  __m128i sample;

  __m128i packed_data = _mm_loadl_epi64((__m128i*)data);
  __m128i packed_filter = _mm_loadl_epi64((__m128i*)filter);

  sample = _mm_maddubs_epi16(packed_data, packed_filter);
  sample = _mm_add_epi16(sample, _mm_shuffle_epi32(sample, _MM_SHUFFLE(0, 1, 0, 1)));
  sample = _mm_add_epi16(sample, _mm_shufflelo_epi16(sample, _MM_SHUFFLE(0, 1, 0, 1)));

  return (int16_t)_mm_cvtsi128_si32(sample);
}


int32_t kvz_eight_tap_filter_hor_16bit_avx2(int8_t *filter, int16_t *data)
{
  __m128i sample;

  __m128i packed_data = _mm_loadu_si128((__m128i*)data);
  __m128i packed_filter = _mm_cvtepi8_epi16(_mm_loadl_epi64((__m128i*)filter));

  sample = _mm_madd_epi16(packed_data, packed_filter);
  sample = _mm_add_epi32(sample, _mm_shuffle_epi32(sample, _MM_SHUFFLE(0, 1, 3, 2)));
  sample = _mm_add_epi32(sample, _mm_shuffle_epi32(sample, _MM_SHUFFLE(0, 1, 0, 1)));

  return _mm_extract_epi32(sample, 0);
}

int16_t kvz_eight_tap_filter_ver_avx2(int8_t *filter, kvz_pixel *data, int16_t stride)
{
  int16_t temp = 0;
  for (int i = 0; i < 8; ++i)
  {
    temp += filter[i] * data[stride * i];
  }

  return temp;
}

int32_t kvz_eight_tap_filter_ver_16bit_avx2(int8_t *filter, int16_t *data, int16_t stride)
{
  int32_t temp = 0;
  for (int i = 0; i < 8; ++i)
  {
    temp += filter[i] * data[stride * i];
  }

  return temp;
}

int16_t kvz_four_tap_filter_hor_avx2(int8_t *filter, kvz_pixel *data)
{
  __m128i packed_data = _mm_cvtsi32_si128(*(int32_t*)data);
  __m128i packed_filter = _mm_cvtsi32_si128(*(int32_t*)filter);

  __m128i temp = _mm_maddubs_epi16(packed_data, packed_filter);
  temp = _mm_hadd_epi16(temp, temp);

  return _mm_extract_epi16(temp, 0);
}

int32_t kvz_four_tap_filter_hor_16bit_avx2(int8_t *filter, int16_t *data)
{
  __m128i packed_data = _mm_loadl_epi64((__m128i*)data);
  __m128i packed_filter = _mm_cvtepi8_epi16(_mm_cvtsi32_si128(*(int32_t*)filter) );

  __m128i temp = _mm_madd_epi16(packed_data, packed_filter);
  temp = _mm_hadd_epi32(temp, temp);

  return _mm_cvtsi128_si32(temp);
}

int16_t kvz_four_tap_filter_ver_avx2(int8_t *filter, kvz_pixel *data, int16_t stride)
{
  int16_t temp = 0;
  for (int i = 0; i < 4; ++i)
  {
    temp += filter[i] * data[stride * i];
  }

  return temp;
}

int32_t kvz_four_tap_filter_ver_16bit_avx2(int8_t *filter, int16_t *data, int16_t stride)
{
  int32_t temp = 0;
  for (int i = 0; i < 4; ++i)
  {
    temp += filter[i] * data[stride * i];
  }

  return temp;
}

void kvz_eight_tap_filter_x4_hor_avx2(int8_t *filter, kvz_pixel *data, int shift, int16_t* dst)
{
  __m256i packed_data = _mm256_inserti128_si256(_mm256_castsi128_si256(_mm_loadl_epi64((__m128i*)data)), _mm_loadl_epi64((__m128i*)(data + 2)), 1);
  __m256i packed_filter = _mm256_broadcastq_epi64(_mm_loadl_epi64((__m128i*)filter));
  __m256i idx_lookup = _mm256_broadcastsi128_si256(_mm_setr_epi8(0, 1, 2, 3, 4, 5, 6, 7, 1, 2, 3, 4, 5, 6, 7, 8));

  __m256i temp = _mm256_shuffle_epi8(packed_data, idx_lookup);

  temp = _mm256_maddubs_epi16(temp, packed_filter);
  __m128i temp_128 = _mm_hadd_epi16(_mm256_extracti128_si256(temp, 0), _mm256_extracti128_si256(temp, 1));
  temp_128 = _mm_hadd_epi16(temp_128, temp_128);
  temp_128 = _mm_srai_epi16(temp_128, shift);

  _mm_storel_epi64((__m128i*)dst, temp_128);
}

void kvz_four_tap_filter_x4_hor_avx2(int8_t *filter, kvz_pixel *data, int shift, int16_t* dst)
{
  __m128i packed_data = _mm_loadl_epi64((__m128i*)data);
  __m128i packed_filter = _mm_set1_epi32(*(int32_t*)filter);
  __m128i idx_lookup = _mm_setr_epi8(0, 1, 2, 3, 1, 2, 3, 4, 2, 3, 4, 5, 3, 4, 5, 6);

  __m128i temp = _mm_shuffle_epi8(packed_data, idx_lookup);

  temp = _mm_maddubs_epi16(temp, packed_filter);
  temp = _mm_hadd_epi16(temp, temp);
  temp = _mm_srai_epi16(temp, shift);

  _mm_storel_epi64((__m128i*)dst, temp);
}

void kvz_eight_tap_filter_x8_hor_avx2(int8_t *filter, kvz_pixel *data, int shift, int16_t* dst)
{
  __m256i packed_data = _mm256_inserti128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)data)), _mm_loadu_si128((__m128i*)(data + 4)), 1);
  __m256i packed_filter = _mm256_broadcastq_epi64(_mm_loadl_epi64((__m128i*)filter));
  __m256i idx_lookup0 = _mm256_broadcastsi128_si256(_mm_setr_epi8(0, 1, 2, 3, 4, 5, 6, 7, 1, 2, 3, 4, 5, 6, 7, 8));
  __m256i idx_lookup1 = _mm256_broadcastsi128_si256(_mm_setr_epi8(2, 3, 4, 5, 6, 7, 8, 9, 3, 4, 5, 6, 7, 8, 9, 10));

  __m256i temp0 = _mm256_shuffle_epi8(packed_data, idx_lookup0);
  __m256i temp1 = _mm256_shuffle_epi8(packed_data, idx_lookup1);

  temp0 = _mm256_maddubs_epi16(temp0, packed_filter);
  temp1 = _mm256_maddubs_epi16(temp1, packed_filter);
  temp0 = _mm256_hadd_epi16(temp0, temp1);
  temp0 = _mm256_hadd_epi16(temp0, temp0);

  temp0 = _mm256_srai_epi16(temp0, shift);

  temp0 = _mm256_permute4x64_epi64(temp0, _MM_SHUFFLE(3, 1, 2, 0));

  _mm_storeu_si128((__m128i*)dst, _mm256_castsi256_si128(temp0));
}

void kvz_four_tap_filter_x8_hor_avx2(int8_t *filter, kvz_pixel *data, int shift, int16_t* dst)
{
  __m256i packed_data = _mm256_inserti128_si256(_mm256_castsi128_si256(_mm_loadl_epi64((__m128i*)data)), _mm_loadl_epi64((__m128i*)(data + 4)), 1);
  __m256i packed_filter = _mm256_set1_epi32(*(int32_t*)filter);
  __m256i idx_lookup = _mm256_broadcastsi128_si256(_mm_setr_epi8(0, 1, 2, 3, 1, 2, 3, 4, 2, 3, 4, 5, 3, 4, 5, 6));

  __m256i temp = _mm256_shuffle_epi8(packed_data, idx_lookup);

  temp = _mm256_maddubs_epi16(temp, packed_filter);
  temp = _mm256_hadd_epi16(temp, temp);
  temp = _mm256_srai_epi16(temp, shift);

  _mm_storel_epi64((__m128i*)dst, _mm256_castsi256_si128(temp));
  _mm_storel_epi64((__m128i*)(dst + 4), _mm256_extracti128_si256(temp, 1));
}

int32_t kvz_eight_tap_filter_x4_ver_16bit_avx2(int8_t *filter, int16_t *data, int16_t stride, int offset, int shift2, int shift3)
{

  __m128i v_filter = _mm_cvtepi8_epi16(_mm_set1_epi16(*(int16_t*)&(filter[0])));
  __m128i v_data0 = _mm_loadl_epi64((__m128i*)(data + stride * 0));
  __m128i v_data1 = _mm_loadl_epi64((__m128i*)(data + stride * 1));
  __m128i v_data = _mm_unpacklo_epi16(v_data0, v_data1);
  __m128i temp =  _mm_madd_epi16(v_filter, v_data);

  v_filter = _mm_cvtepi8_epi16(_mm_set1_epi16(*(int16_t*)&(filter[2])));
  __m128i v_data2 = _mm_loadl_epi64((__m128i*)(data + stride * 2));
  __m128i v_data3 = _mm_loadl_epi64((__m128i*)(data + stride * 3));
  v_data = _mm_unpacklo_epi16(v_data2, v_data3);
  temp = _mm_add_epi32(temp, _mm_madd_epi16(v_filter, v_data) );

  temp = _mm_add_epi32(temp, _mm_set1_epi32(offset));
  temp = _mm_srai_epi32(temp, shift2 + shift3);

  temp = _mm_packus_epi32(temp, temp);
  temp = _mm_packus_epi16(temp, temp);

  return _mm_cvtsi128_si32(temp);
}

int32_t kvz_four_tap_filter_x4_ver_16bit_avx2(int8_t *filter, int16_t *data, int16_t stride, int offset, int shift2, int shift3)
{

  __m128i v_filter = _mm_cvtepi8_epi16(_mm_set1_epi16(*(int16_t*)&(filter[0])));
  __m128i v_data0 = _mm_loadl_epi64((__m128i*)(data + stride * 0));
  __m128i v_data1 = _mm_loadl_epi64((__m128i*)(data + stride * 1));
  __m128i v_data = _mm_unpacklo_epi16(v_data0, v_data1);
  __m128i temp =  _mm_madd_epi16(v_filter, v_data);

  v_filter = _mm_cvtepi8_epi16(_mm_set1_epi16(*(int16_t*)&(filter[2])));
  __m128i v_data2 = _mm_loadl_epi64((__m128i*)(data + stride * 2));
  __m128i v_data3 = _mm_loadl_epi64((__m128i*)(data + stride * 3));
  v_data = _mm_unpacklo_epi16(v_data2, v_data3);
  temp = _mm_add_epi32(temp, _mm_madd_epi16(v_filter, v_data) );

  temp = _mm_add_epi32(temp, _mm_set1_epi32(offset));
  temp = _mm_srai_epi32(temp, shift2 + shift3);

  temp = _mm_packus_epi32(temp, temp);
  temp = _mm_packus_epi16(temp, temp);

  return _mm_cvtsi128_si32(temp);
}

void kvz_eight_tap_filter_x8_ver_16bit_avx2(int8_t *filter, int16_t *data, int16_t stride, int offset, int shift2, int shift3, kvz_pixel* dst)
{

  __m256i v_filter = _mm256_cvtepi8_epi16(_mm_set1_epi16(*(int16_t*)&(filter[0])));
  __m256i v_data0 = _mm256_cvtepu16_epi32(_mm_loadu_si128((__m128i*)(data + stride * 0)));
  __m256i v_data1 = _mm256_cvtepu16_epi32(_mm_loadu_si128((__m128i*)(data + stride * 1)));
  __m256i v_data = _mm256_or_si256(v_data0, _mm256_slli_epi32(v_data1, 16));
  __m256i temp =  _mm256_madd_epi16(v_filter, v_data);

  v_filter = _mm256_cvtepi8_epi16(_mm_set1_epi16(*(int16_t*)&(filter[2])));
  __m256i v_data2 = _mm256_cvtepu16_epi32(_mm_loadu_si128((__m128i*)(data + stride * 2)));
  __m256i v_data3 = _mm256_cvtepu16_epi32(_mm_loadu_si128((__m128i*)(data + stride * 3)));
  v_data = _mm256_or_si256(v_data2, _mm256_slli_epi32(v_data3, 16));
  temp = _mm256_add_epi32(temp, _mm256_madd_epi16(v_filter, v_data) );

  v_filter = _mm256_cvtepi8_epi16(_mm_set1_epi16(*(int16_t*)&(filter[4])));
  __m256i v_data4 = _mm256_cvtepu16_epi32(_mm_loadu_si128((__m128i*)(data + stride * 4)));
  __m256i v_data5 = _mm256_cvtepu16_epi32(_mm_loadu_si128((__m128i*)(data + stride * 5)));
  v_data = _mm256_or_si256(v_data4, _mm256_slli_epi32(v_data5, 16));
  temp = _mm256_add_epi32(temp, _mm256_madd_epi16(v_filter, v_data) );

  v_filter = _mm256_cvtepi8_epi16(_mm_set1_epi16(*(int16_t*)&(filter[6])));
  __m256i v_data6 = _mm256_cvtepu16_epi32(_mm_loadu_si128((__m128i*)(data + stride * 6)));
  __m256i v_data7 = _mm256_cvtepu16_epi32(_mm_loadu_si128((__m128i*)(data + stride * 7)));
  v_data = _mm256_or_si256(v_data6, _mm256_slli_epi32(v_data7, 16));
  temp = _mm256_add_epi32(temp, _mm256_madd_epi16(v_filter, v_data) );

  temp = _mm256_add_epi32(temp, _mm256_set1_epi32(offset));
  temp = _mm256_srai_epi32(temp, shift2 + shift3);

  temp = _mm256_packus_epi32(temp, temp);
  temp = _mm256_packus_epi16(temp, temp);

  *(int32_t*)dst = _mm_cvtsi128_si32(_mm256_castsi256_si128(temp));
  *(int32_t*)(dst + 4) = _mm_cvtsi128_si32(_mm256_extracti128_si256(temp, 1));
}

void kvz_four_tap_filter_x8_ver_16bit_avx2(int8_t *filter, int16_t *data, int16_t stride, int offset, int shift2, int shift3, kvz_pixel* dst)
{

  __m256i v_filter = _mm256_cvtepi8_epi16(_mm_set1_epi16(*(int16_t*)&(filter[0])));
  __m256i v_data0 = _mm256_cvtepu16_epi32(_mm_loadu_si128((__m128i*)(data + stride * 0)));
  __m256i v_data1 = _mm256_cvtepu16_epi32(_mm_loadu_si128((__m128i*)(data + stride * 1)));
  __m256i v_data = _mm256_or_si256(v_data0, _mm256_slli_epi32(v_data1, 16));
  __m256i temp =  _mm256_madd_epi16(v_filter, v_data);

  v_filter = _mm256_cvtepi8_epi16(_mm_set1_epi16(*(int16_t*)&(filter[2])));
  __m256i v_data2 = _mm256_cvtepu16_epi32(_mm_loadu_si128((__m128i*)(data + stride * 2)));
  __m256i v_data3 = _mm256_cvtepu16_epi32(_mm_loadu_si128((__m128i*)(data + stride * 3)));
  v_data = _mm256_or_si256(v_data2, _mm256_slli_epi32(v_data3, 16));
  temp = _mm256_add_epi32(temp, _mm256_madd_epi16(v_filter, v_data) );

  temp = _mm256_add_epi32(temp, _mm256_set1_epi32(offset));
  temp = _mm256_srai_epi32(temp, shift2 + shift3);

  temp = _mm256_packus_epi32(temp, temp);
  temp = _mm256_packus_epi16(temp, temp);

  *(int32_t*)dst = _mm_cvtsi128_si32(_mm256_castsi256_si128(temp));
  *(int32_t*)(dst + 4) = _mm_cvtsi128_si32(_mm256_extracti128_si256(temp, 1));
}

void kvz_filter_inter_quarterpel_luma_avx2(const encoder_control_t * const encoder, kvz_pixel *src, int16_t src_stride, int width, int height, kvz_pixel *dst, int16_t dst_stride, int8_t hor_flag, int8_t ver_flag)
{

  int32_t x, y;
  int16_t shift1 = KVZ_BIT_DEPTH - 8;
  int32_t shift2 = 6;
  int32_t shift3 = 14 - KVZ_BIT_DEPTH;
  int32_t offset23 = 1 << (shift2 + shift3 - 1);

  //coefficients for 1/4, 2/4 and 3/4 positions
  int8_t *c0, *c1, *c2, *c3;

  c0 = kvz_g_luma_filter[0];
  c1 = kvz_g_luma_filter[1];
  c2 = kvz_g_luma_filter[2];
  c3 = kvz_g_luma_filter[3];

  int16_t flipped_hor_filtered[MAX_HEIGHT][MAX_WIDTH];

  // Filter horizontally and flip x and y
  for (x = 0; x < width; ++x) {
    for (y = 0; y < height; y += 8) {
      int ypos = y - FILTER_OFFSET;
      int xpos = x - FILTER_OFFSET;

      kvz_eight_tap_filter_and_flip_avx2(kvz_g_luma_filter, &src[src_stride*ypos + xpos], src_stride, (int16_t*)&(flipped_hor_filtered[4 * x + 0][y]));
    
    }

    for (; y < height + FILTER_SIZE - 1; ++y) {
      int ypos = y - FILTER_OFFSET;
      int xpos = x - FILTER_OFFSET;
      flipped_hor_filtered[4 * x + 0][y] = kvz_eight_tap_filter_hor_avx2(c0, &src[src_stride*ypos + xpos]) << shift1;
      flipped_hor_filtered[4 * x + 1][y] = kvz_eight_tap_filter_hor_avx2(c1, &src[src_stride*ypos + xpos]) << shift1;
      flipped_hor_filtered[4 * x + 2][y] = kvz_eight_tap_filter_hor_avx2(c2, &src[src_stride*ypos + xpos]) << shift1;
      flipped_hor_filtered[4 * x + 3][y] = kvz_eight_tap_filter_hor_avx2(c3, &src[src_stride*ypos + xpos]) << shift1;
    }
  }

  // Filter vertically and flip x and y
  for (y = 0; y < height; ++y) {
    for (x = 0; x < 4 * width - 3; x += 4) {

     eight_tap_filter_and_flip_16bit_avx2(kvz_g_luma_filter, &flipped_hor_filtered[x][y], MAX_WIDTH, offset23, shift2 + shift3, &(dst[(4 * y + 0)*dst_stride + x]), dst_stride);

    }

  }

}

/**
* \brief Interpolation for chroma half-pixel
* \param src source image in integer pels (-2..width+3, -2..height+3)
* \param src_stride stride of source image
* \param width width of source image block
* \param height height of source image block
* \param dst destination image in half-pixel resolution
* \param dst_stride stride of destination image
*
*/
void kvz_filter_inter_halfpel_chroma_avx2(const encoder_control_t * const encoder, kvz_pixel *src, int16_t src_stride, int width, int height, kvz_pixel *dst, int16_t dst_stride, int8_t hor_flag, int8_t ver_flag)
{
  /* ____________
  * | B0,0|ae0,0|
  * |ea0,0|ee0,0|
  *
  * ae0,0 = (-4*B-1,0  + 36*B0,0  + 36*B1,0  - 4*B2,0)  >> shift1
  * ea0,0 = (-4*B0,-1  + 36*B0,0  + 36*B0,1  - 4*B0,2)  >> shift1
  * ee0,0 = (-4*ae0,-1 + 36*ae0,0 + 36*ae0,1 - 4*ae0,2) >> shift2
  */
  int32_t x, y;
  int32_t shift1 = KVZ_BIT_DEPTH - 8;
  int32_t shift2 = 6;
  int32_t shift3 = 14 - KVZ_BIT_DEPTH;
  int32_t offset3 = 1 << (shift3 - 1);
  int32_t offset23 = 1 << (shift2 + shift3 - 1);

  int8_t* c = kvz_g_chroma_filter[4];
  int16_t temp[4] = {0,0,0,0};

  // Loop source pixels and generate four filtered half-pel pixels on each round
  for (y = 0; y < height; y++) {
    int dst_pos_y = (y << 1)*dst_stride;
    int src_pos_y = y*src_stride;
    for (x = 0; x < width; x++) {
      // Calculate current dst and src pixel positions
      int dst_pos = dst_pos_y + (x << 1);
      int src_pos = src_pos_y + x;

      // Original pixel (not really needed)
      dst[dst_pos] = src[src_pos]; //B0,0

      // ae0,0 - We need this only when hor_flag and for ee0,0
      if (hor_flag) {
        temp[1] = kvz_four_tap_filter_hor_avx2(c, &src[src_pos - 1]) >> shift1; // ae0,0
      }
      // ea0,0 - needed only when ver_flag
      if (ver_flag) {
        dst[dst_pos + 1 * dst_stride] = kvz_fast_clip_32bit_to_pixel(((kvz_four_tap_filter_ver_avx2(c, &src[src_pos - src_stride], src_stride) >> shift1) + (1 << (shift3 - 1))) >> shift3); // ea0,0
      }

      // When both flags, we use _only_ this pixel (but still need ae0,0 for it)
      if (hor_flag && ver_flag) {
        // Calculate temporary values..
        src_pos -= src_stride;  //0,-1
        temp[0] = (kvz_four_tap_filter_hor_avx2(c, &src[src_pos - 1]) >> shift1); // ae0,-1
        src_pos += 2 * src_stride;  //0,1
        temp[2] = (kvz_four_tap_filter_hor_avx2(c, &src[src_pos - 1]) >> shift1); // ae0,1
        src_pos += src_stride;  //0,2
        temp[3] = (kvz_four_tap_filter_hor_avx2(c, &src[src_pos - 1]) >> shift1); // ae0,2

        dst[dst_pos + 1 * dst_stride + 1] = kvz_fast_clip_32bit_to_pixel(((kvz_four_tap_filter_hor_16bit_avx2(c, temp) + offset23) >> shift2) >> shift3); // ee0,0
      }

      if (hor_flag) {
        dst[dst_pos + 1] = kvz_fast_clip_32bit_to_pixel((temp[1] + offset3) >> shift3);
      }
    }
  }
}

void kvz_filter_inter_octpel_chroma_avx2(const encoder_control_t * const encoder, kvz_pixel *src, int16_t src_stride, int width, int height, kvz_pixel *dst, int16_t dst_stride, int8_t hor_flag, int8_t ver_flag)
{

  int32_t x, y;
  int32_t shift1 = KVZ_BIT_DEPTH - 8;
  int32_t shift2 = 6;
  int32_t shift3 = 14 - KVZ_BIT_DEPTH;
  int32_t offset3 = 1 << (shift3 - 1);
  int32_t offset23 = 1 << (shift2 + shift3 - 1);

  //coefficients for 1/8, 2/8, 3/8, 4/8, 5/8, 6/8 and 7/8 positions
  int8_t *c1, *c2, *c3, *c4, *c5, *c6, *c7;

  int i;
  c1 = kvz_g_chroma_filter[1];
  c2 = kvz_g_chroma_filter[2];
  c3 = kvz_g_chroma_filter[3];
  c4 = kvz_g_chroma_filter[4];
  c5 = kvz_g_chroma_filter[5];
  c6 = kvz_g_chroma_filter[6];
  c7 = kvz_g_chroma_filter[7];

  int16_t temp[7][4]; // Temporary horizontal values calculated from integer pixels


  // Loop source pixels and generate 64 filtered 1/8-pel pixels on each round
  for (y = 0; y < height; y++) {
    int dst_pos_y = (y << 3)*dst_stride;
    int src_pos_y = y*src_stride;
    for (x = 0; x < width; x++) {
      // Calculate current dst and src pixel positions
      int dst_pos = dst_pos_y + (x << 3);
      int src_pos = src_pos_y + x;

      // Original pixel
      dst[dst_pos] = src[src_pos];

      // Horizontal 1/8-values
      if (hor_flag && !ver_flag) {

        temp[0][1] = (kvz_four_tap_filter_hor_avx2(c1, &src[src_pos - 1]) >> shift1); // ae0,0 h0
        temp[1][1] = (kvz_four_tap_filter_hor_avx2(c2, &src[src_pos - 1]) >> shift1);
        temp[2][1] = (kvz_four_tap_filter_hor_avx2(c3, &src[src_pos - 1]) >> shift1);
        temp[3][1] = (kvz_four_tap_filter_hor_avx2(c4, &src[src_pos - 1]) >> shift1);
        temp[4][1] = (kvz_four_tap_filter_hor_avx2(c5, &src[src_pos - 1]) >> shift1);
        temp[5][1] = (kvz_four_tap_filter_hor_avx2(c6, &src[src_pos - 1]) >> shift1);
        temp[6][1] = (kvz_four_tap_filter_hor_avx2(c7, &src[src_pos - 1]) >> shift1);

      }

      // Vertical 1/8-values
      if (ver_flag) {
        dst[dst_pos + 1 * dst_stride] = kvz_fast_clip_32bit_to_pixel(((kvz_four_tap_filter_ver_avx2(c1, &src[src_pos - 1 * src_stride], src_stride) >> shift1) + (1 << (shift3 - 1))) >> shift3); //
        dst[dst_pos + 2 * dst_stride] = kvz_fast_clip_32bit_to_pixel(((kvz_four_tap_filter_ver_avx2(c2, &src[src_pos - 1 * src_stride], src_stride) >> shift1) + (1 << (shift3 - 1))) >> shift3);
        dst[dst_pos + 3 * dst_stride] = kvz_fast_clip_32bit_to_pixel(((kvz_four_tap_filter_ver_avx2(c3, &src[src_pos - 1 * src_stride], src_stride) >> shift1) + (1 << (shift3 - 1))) >> shift3);
        dst[dst_pos + 4 * dst_stride] = kvz_fast_clip_32bit_to_pixel(((kvz_four_tap_filter_ver_avx2(c4, &src[src_pos - 1 * src_stride], src_stride) >> shift1) + (1 << (shift3 - 1))) >> shift3);
        dst[dst_pos + 5 * dst_stride] = kvz_fast_clip_32bit_to_pixel(((kvz_four_tap_filter_ver_avx2(c5, &src[src_pos - 1 * src_stride], src_stride) >> shift1) + (1 << (shift3 - 1))) >> shift3);
        dst[dst_pos + 6 * dst_stride] = kvz_fast_clip_32bit_to_pixel(((kvz_four_tap_filter_ver_avx2(c6, &src[src_pos - 1 * src_stride], src_stride) >> shift1) + (1 << (shift3 - 1))) >> shift3);
        dst[dst_pos + 7 * dst_stride] = kvz_fast_clip_32bit_to_pixel(((kvz_four_tap_filter_ver_avx2(c7, &src[src_pos - 1 * src_stride], src_stride) >> shift1) + (1 << (shift3 - 1))) >> shift3);
      }

      // When both flags, interpolate values from temporary horizontal values
      if (hor_flag && ver_flag) {

        // Calculate temporary values
        src_pos -= 1 * src_stride;  //0,-3
        for (i = 0; i < 4; ++i) {

          temp[0][i] = (kvz_four_tap_filter_hor_avx2(c1, &src[src_pos + i * src_stride - 1]) >> shift1);
          temp[1][i] = (kvz_four_tap_filter_hor_avx2(c2, &src[src_pos + i * src_stride - 1]) >> shift1);
          temp[2][i] = (kvz_four_tap_filter_hor_avx2(c3, &src[src_pos + i * src_stride - 1]) >> shift1);
          temp[3][i] = (kvz_four_tap_filter_hor_avx2(c4, &src[src_pos + i * src_stride - 1]) >> shift1);
          temp[4][i] = (kvz_four_tap_filter_hor_avx2(c5, &src[src_pos + i * src_stride - 1]) >> shift1);
          temp[5][i] = (kvz_four_tap_filter_hor_avx2(c6, &src[src_pos + i * src_stride - 1]) >> shift1);
          temp[6][i] = (kvz_four_tap_filter_hor_avx2(c7, &src[src_pos + i * src_stride - 1]) >> shift1);

        }


        //Calculate values from temporary horizontal 1/8-values
        for (i = 0; i<7; ++i){
          dst[dst_pos + 1 * dst_stride + i + 1] = kvz_fast_clip_32bit_to_pixel(((kvz_four_tap_filter_hor_16bit_avx2(c1, &temp[i][0]) + offset23) >> shift2) >> shift3); // ee0,0
          dst[dst_pos + 2 * dst_stride + i + 1] = kvz_fast_clip_32bit_to_pixel(((kvz_four_tap_filter_hor_16bit_avx2(c2, &temp[i][0]) + offset23) >> shift2) >> shift3);
          dst[dst_pos + 3 * dst_stride + i + 1] = kvz_fast_clip_32bit_to_pixel(((kvz_four_tap_filter_hor_16bit_avx2(c3, &temp[i][0]) + offset23) >> shift2) >> shift3);
          dst[dst_pos + 4 * dst_stride + i + 1] = kvz_fast_clip_32bit_to_pixel(((kvz_four_tap_filter_hor_16bit_avx2(c4, &temp[i][0]) + offset23) >> shift2) >> shift3);
          dst[dst_pos + 5 * dst_stride + i + 1] = kvz_fast_clip_32bit_to_pixel(((kvz_four_tap_filter_hor_16bit_avx2(c5, &temp[i][0]) + offset23) >> shift2) >> shift3);
          dst[dst_pos + 6 * dst_stride + i + 1] = kvz_fast_clip_32bit_to_pixel(((kvz_four_tap_filter_hor_16bit_avx2(c6, &temp[i][0]) + offset23) >> shift2) >> shift3);
          dst[dst_pos + 7 * dst_stride + i + 1] = kvz_fast_clip_32bit_to_pixel(((kvz_four_tap_filter_hor_16bit_avx2(c7, &temp[i][0]) + offset23) >> shift2) >> shift3);

        }

      }

      if (hor_flag) {
        dst[dst_pos + 1] = kvz_fast_clip_32bit_to_pixel((temp[0][1] + offset3) >> shift3);
        dst[dst_pos + 2] = kvz_fast_clip_32bit_to_pixel((temp[1][1] + offset3) >> shift3);
        dst[dst_pos + 3] = kvz_fast_clip_32bit_to_pixel((temp[2][1] + offset3) >> shift3);
        dst[dst_pos + 4] = kvz_fast_clip_32bit_to_pixel((temp[3][1] + offset3) >> shift3);
        dst[dst_pos + 5] = kvz_fast_clip_32bit_to_pixel((temp[4][1] + offset3) >> shift3);
        dst[dst_pos + 6] = kvz_fast_clip_32bit_to_pixel((temp[5][1] + offset3) >> shift3);
        dst[dst_pos + 7] = kvz_fast_clip_32bit_to_pixel((temp[6][1] + offset3) >> shift3);
      }


    }
  }
}

void kvz_filter_hpel_blocks_hor_ver_luma_avx2(const encoder_control_t * const encoder, kvz_pixel *src, int16_t src_stride, int width, int height, frac_search_block *filtered)
{
  int x, y;
  int16_t shift1 = KVZ_BIT_DEPTH - 8;
  int32_t shift2 = 6;
  int32_t shift3 = 14 - KVZ_BIT_DEPTH;
  int32_t offset23 = 1 << (shift2 + shift3 - 1);

  int8_t *fir0 = kvz_g_luma_filter[0];
  int8_t *fir2 = kvz_g_luma_filter[2];

  int16_t flipped0[(LCU_WIDTH + 1) * (KVZ_EXT_BLOCK_W + 1)];
  int16_t flipped2[(LCU_WIDTH + 1) * (KVZ_EXT_BLOCK_W + 1)];

  int16_t temp_stride = height + KVZ_EXT_PADDING + 1;
  int16_t dst_stride = (LCU_WIDTH + 1);

  // Horizontal positions
  for (x = 0; x < width + 1; ++x) {
    for (y = 0; y + 8 < height + KVZ_EXT_PADDING + 1; y += 8) {
      int ypos = y - FILTER_OFFSET;
      int xpos = x - FILTER_OFFSET;
      int8_t *firs[2] = { fir0, fir2 };
      int16_t *dsts[2] = { &flipped0[x * temp_stride + y], &flipped2[x * temp_stride + y] };
      kvz_filter_flip_shift_x8_dual_avx2(&src[src_stride*ypos + xpos], src_stride, &firs[0], shift1, &dsts[0]);
    }

    for (; y < height + KVZ_EXT_PADDING + 1; ++y) {
      int ypos = y - FILTER_OFFSET;
      int xpos = x - FILTER_OFFSET;
      flipped0[x * temp_stride + y] = kvz_eight_tap_filter_hor_avx2(fir0, &src[src_stride*ypos + xpos]) >> shift1;
      flipped2[x * temp_stride + y] = kvz_eight_tap_filter_hor_avx2(fir2, &src[src_stride*ypos + xpos]) >> shift1;
    }
  }

  // Filter vertically and flip x and y
  for (x = 0; x + 8 < width + 1; x += 8) {
    for (y = 0; y < height + 1; ++y) {  
      int8_t *firs[2] = { fir0, fir2 };
      kvz_pixel *dsts[2] = { &filtered[HPEL_POS_HOR][y * dst_stride + x], &filtered[HPEL_POS_VER][y * dst_stride + x]};
      int16_t *srcs[2] = {flipped2 + x * temp_stride + y, flipped0 + x * temp_stride + y };
      kvz_filter_flip_round_clip_x8_16bit_dual_avx2(srcs, temp_stride, firs, offset23, shift2 + shift3, dsts);
    }
  }

  // The remaining pixels
  for (; x < width + 1; ++x) {
    for (y = 0; y < height + 1; ++y) {
      filtered[HPEL_POS_HOR][y * dst_stride + x]  = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_hor_16bit_avx2(fir0, &flipped2[x * temp_stride + y]) + offset23) >> shift2) >> shift3);
      filtered[HPEL_POS_VER][y * dst_stride + x]  = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_hor_16bit_avx2(fir2, &flipped0[x * temp_stride + y]) + offset23) >> shift2) >> shift3);
    }
  }
}

void kvz_filter_hpel_blocks_full_luma_avx2(const encoder_control_t * const encoder, kvz_pixel *src, int16_t src_stride, int width, int height, frac_search_block *filtered)
{
  int x, y;
  int16_t shift1 = KVZ_BIT_DEPTH - 8;
  int32_t shift2 = 6;
  int32_t shift3 = 14 - KVZ_BIT_DEPTH;
  int32_t offset23 = 1 << (shift2 + shift3 - 1);

  int8_t *fir0 = kvz_g_luma_filter[0];
  int8_t *fir2 = kvz_g_luma_filter[2];

  int16_t flipped0[(LCU_WIDTH + 1) * (KVZ_EXT_BLOCK_W + 1)];
  int16_t flipped2[(LCU_WIDTH + 1) * (KVZ_EXT_BLOCK_W + 1)];

  int16_t temp_stride = height + KVZ_EXT_PADDING + 1;
  int16_t dst_stride = (LCU_WIDTH + 1);

  // Horizontal positions
  for (x = 0; x < width + 1; ++x) {
    for (y = 0; y + 8 < height + KVZ_EXT_PADDING + 1; y += 8) {
      int ypos = y - FILTER_OFFSET;
      int xpos = x - FILTER_OFFSET;
      int8_t *firs[2] = { fir0, fir2 };
      int16_t *dsts[2] = { &flipped0[x * temp_stride + y], &flipped2[x * temp_stride + y] };
      kvz_filter_flip_shift_x8_dual_avx2(&src[src_stride*ypos + xpos], src_stride, &firs[0], shift1, &dsts[0]);

    }

    for (; y < height + KVZ_EXT_PADDING + 1; ++y) {
      int ypos = y - FILTER_OFFSET;
      int xpos = x - FILTER_OFFSET;
      flipped0[x * temp_stride + y] = kvz_eight_tap_filter_hor_avx2(fir0, &src[src_stride*ypos + xpos]) >> shift1;
      flipped2[x * temp_stride + y] = kvz_eight_tap_filter_hor_avx2(fir2, &src[src_stride*ypos + xpos]) >> shift1;
    }
  }

  // Filter vertically and flip x and y
  for (x = 0; x + 8 < width + 1; x += 8) {
    for (y = 0; y < height + 1; ++y) {
      int8_t *firs[2] = { fir0, fir2 };
      kvz_pixel *dsts[2] = { &filtered[HPEL_POS_HOR][y * dst_stride + x], &filtered[HPEL_POS_VER][y * dst_stride + x]};
      int16_t *srcs[2] = {flipped2 + x * temp_stride + y, flipped0 + x * temp_stride + y };
      kvz_filter_flip_round_clip_x8_16bit_dual_avx2(srcs, temp_stride, firs, offset23, shift2 + shift3, dsts);
      kvz_filter_flip_round_clip_x8_16bit_avx2(flipped2 + x * temp_stride + y, temp_stride, fir2, offset23, shift2 + shift3, &filtered[HPEL_POS_DIA][y * dst_stride + x]);

    }
  }

  // The remaining pixels
  for (; x < width + 1; ++x) {
    for (y = 0; y < height + 1; ++y) {
      filtered[HPEL_POS_HOR][y * dst_stride + x]  = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_hor_16bit_avx2(fir0, &flipped2[x * temp_stride + y]) + offset23) >> shift2) >> shift3);
      filtered[HPEL_POS_VER][y * dst_stride + x]  = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_hor_16bit_avx2(fir2, &flipped0[x * temp_stride + y]) + offset23) >> shift2) >> shift3);
      filtered[HPEL_POS_DIA][y * dst_stride + x]  = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_hor_16bit_avx2(fir2, &flipped2[x * temp_stride + y]) + offset23) >> shift2) >> shift3);
    }
  }
}

void kvz_filter_qpel_blocks_hor_ver_luma_avx2(const encoder_control_t * const encoder, kvz_pixel *src, int16_t src_stride, int width, int height, frac_search_block *filtered)
{
  int x, y;
  int16_t shift1 = KVZ_BIT_DEPTH - 8;
  int32_t shift2 = 6;
  int32_t shift3 = 14 - KVZ_BIT_DEPTH;
  int32_t offset23 = 1 << (shift2 + shift3 - 1);

  int8_t *fir0 = kvz_g_luma_filter[0];
  int8_t *fir2 = kvz_g_luma_filter[2];
  int8_t *fir1 = kvz_g_luma_filter[1];
  int8_t *fir3 = kvz_g_luma_filter[3];

  int16_t flipped0[(LCU_WIDTH + 1) * (KVZ_EXT_BLOCK_W + 1)];
  int16_t flipped2[(LCU_WIDTH + 1) * (KVZ_EXT_BLOCK_W + 1)];
  int16_t flipped1[(LCU_WIDTH + 1) * (KVZ_EXT_BLOCK_W + 1)];
  int16_t flipped3[(LCU_WIDTH + 1) * (KVZ_EXT_BLOCK_W + 1)];

  int16_t temp_stride = height + KVZ_EXT_PADDING + 1;
  int16_t dst_stride = (LCU_WIDTH + 1);
  
  // Horizontal positions
  for (x = 0; x < width + 1; ++x) {
    for (y = 0; y + 8 < height + KVZ_EXT_PADDING + 1; y += 8) {
      int ypos = y - FILTER_OFFSET;
      int xpos = x - FILTER_OFFSET;
      int8_t *firs[4] = { fir0, fir2, fir1, fir3 };
      int16_t *dsts[4] = { &flipped0[x * temp_stride + y], &flipped2[x * temp_stride + y], &flipped1[x * temp_stride + y], &flipped3[x * temp_stride + y]};
      kvz_filter_flip_shift_x8_dual_avx2(&src[src_stride*ypos + xpos], src_stride, &firs[0], shift1, &dsts[0]);
      kvz_filter_flip_shift_x8_dual_avx2(&src[src_stride*ypos + xpos], src_stride, &firs[2], shift1, &dsts[2]);
    }

    for (y = 0; y < height + KVZ_EXT_PADDING + 1; ++y) {
      int ypos = y - FILTER_OFFSET;
      int xpos = x - FILTER_OFFSET;
      flipped0[x * temp_stride + y] = kvz_eight_tap_filter_hor_avx2(fir0, &src[src_stride*ypos + xpos]) >> shift1;
      flipped2[x * temp_stride + y] = kvz_eight_tap_filter_hor_avx2(fir2, &src[src_stride*ypos + xpos]) >> shift1;
      flipped1[x * temp_stride + y] = kvz_eight_tap_filter_hor_avx2(fir1, &src[src_stride*ypos + xpos]) >> shift1;
      flipped3[x * temp_stride + y] = kvz_eight_tap_filter_hor_avx2(fir3, &src[src_stride*ypos + xpos]) >> shift1;
    }
  }

  // Filter vertically and flip x and y
  for (x = 0; x + 8 < width + 1; x += 8) {
    for (y = 0; y < height + 1; ++y) {
      
      // HPEL
      int8_t *firs0[2] = { fir0, fir2 };
      kvz_pixel *dsts0[2] = { &filtered[0][y * dst_stride + x], &filtered[1][y * dst_stride + x]};
      int16_t *srcs0[4] = { flipped2 + x * temp_stride + y, flipped0 + x * temp_stride + y};
      kvz_filter_flip_round_clip_x8_16bit_dual_avx2(srcs0, temp_stride, firs0, offset23, shift2 + shift3, dsts0);
      kvz_filter_flip_round_clip_x8_16bit_avx2(flipped2 + x * temp_stride + y, temp_stride, fir2, offset23, shift2 + shift3, &filtered[2][y * dst_stride + x]);
     
      // QPEL
      // Horizontal
      int8_t *firs1[4] = { fir0, fir0, fir2, fir2 };
      kvz_pixel *dsts1[4] = { &filtered[3][y * dst_stride + x], &filtered[4][y * dst_stride + x], 
                              &filtered[5][y * dst_stride + x], &filtered[6][y * dst_stride + x] };
      int16_t *srcs1[4] = { flipped1 + x * temp_stride + y, flipped3 + x * temp_stride + y, 
                            flipped1 + x * temp_stride + y, flipped3 + x * temp_stride + y, };
      kvz_filter_flip_round_clip_x8_16bit_dual_avx2(&srcs1[0], temp_stride, &firs1[0], offset23, shift2 + shift3, &dsts1[0]);
      kvz_filter_flip_round_clip_x8_16bit_dual_avx2(&srcs1[2], temp_stride, &firs1[2], offset23, shift2 + shift3, &dsts1[2]);

      // Vertical
      int8_t *firs2[4] = { fir1, fir1, fir3, fir3 };
      kvz_pixel *dsts2[4] = { &filtered[7][y * dst_stride + x], &filtered[8][y * dst_stride + x], 
                              &filtered[9][y * dst_stride + x], &filtered[10][y * dst_stride + x] };
      int16_t *srcs2[4] = { flipped0 + x * temp_stride + y, flipped2 + x * temp_stride + y, 
                            flipped0 + x * temp_stride + y, flipped2 + x * temp_stride + y, };
      kvz_filter_flip_round_clip_x8_16bit_dual_avx2(&srcs2[0], temp_stride, &firs2[0], offset23, shift2 + shift3, &dsts2[0]);
      kvz_filter_flip_round_clip_x8_16bit_dual_avx2(&srcs2[2], temp_stride, &firs2[2], offset23, shift2 + shift3, &dsts2[2]);
    }
  }

  // The remaining pixels
  for (; x < width + 1; ++x) {
    for (y = 0; y < height + 1; ++y) {

      // HPEL
      filtered[0][y * dst_stride + x] = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_hor_16bit_avx2(fir0, &flipped2[x * temp_stride + y]) + offset23) >> shift2) >> shift3);
      filtered[1][y * dst_stride + x] = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_hor_16bit_avx2(fir2, &flipped0[x * temp_stride + y]) + offset23) >> shift2) >> shift3);
      filtered[2][y * dst_stride + x] = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_hor_16bit_avx2(fir2, &flipped2[x * temp_stride + y]) + offset23) >> shift2) >> shift3);

      // QPEL
      // Horizontal
      filtered[3][y * dst_stride + x] = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_hor_16bit_avx2(fir0, &flipped1[x * temp_stride + y]) + offset23) >> shift2) >> shift3);
      filtered[4][y * dst_stride + x] = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_hor_16bit_avx2(fir0, &flipped3[x * temp_stride + y]) + offset23) >> shift2) >> shift3);
      filtered[5][y * dst_stride + x] = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_hor_16bit_avx2(fir2, &flipped1[x * temp_stride + y]) + offset23) >> shift2) >> shift3);
      filtered[6][y * dst_stride + x] = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_hor_16bit_avx2(fir2, &flipped3[x * temp_stride + y]) + offset23) >> shift2) >> shift3);

      // Vertical
      filtered[7][y * dst_stride + x] = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_hor_16bit_avx2(fir1, &flipped0[x * temp_stride + y]) + offset23) >> shift2) >> shift3);
      filtered[8][y * dst_stride + x] = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_hor_16bit_avx2(fir1, &flipped2[x * temp_stride + y]) + offset23) >> shift2) >> shift3);
      filtered[9][y * dst_stride + x] = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_hor_16bit_avx2(fir3, &flipped0[x * temp_stride + y]) + offset23) >> shift2) >> shift3);
      filtered[10][y * dst_stride + x] = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_hor_16bit_avx2(fir3, &flipped2[x * temp_stride + y]) + offset23) >> shift2) >> shift3);
    }
  }
}

void kvz_filter_qpel_blocks_full_luma_avx2(const encoder_control_t * const encoder, kvz_pixel *src, int16_t src_stride, int width, int height, frac_search_block *filtered)
{
  int x, y;
  int16_t shift1 = KVZ_BIT_DEPTH - 8;
  int32_t shift2 = 6;
  int32_t shift3 = 14 - KVZ_BIT_DEPTH;
  int32_t offset23 = 1 << (shift2 + shift3 - 1);

  int8_t *fir0 = kvz_g_luma_filter[0];
  int8_t *fir2 = kvz_g_luma_filter[2];
  int8_t *fir1 = kvz_g_luma_filter[1];
  int8_t *fir3 = kvz_g_luma_filter[3];

  int16_t flipped0[(LCU_WIDTH + 1) * (KVZ_EXT_BLOCK_W + 1)];
  int16_t flipped2[(LCU_WIDTH + 1) * (KVZ_EXT_BLOCK_W + 1)];
  int16_t flipped1[(LCU_WIDTH + 1) * (KVZ_EXT_BLOCK_W + 1)];
  int16_t flipped3[(LCU_WIDTH + 1) * (KVZ_EXT_BLOCK_W + 1)];

  int16_t temp_stride = height + KVZ_EXT_PADDING + 1;
  int16_t dst_stride = (LCU_WIDTH + 1);
  
  // Horizontal positions
  for (x = 0; x < (width + 1); ++x) {
    for (y = 0; y + 8 < height + KVZ_EXT_PADDING + 1; y += 8) {
      int ypos = y - FILTER_OFFSET;
      int xpos = x - FILTER_OFFSET; 
      int8_t *firs[4] = { fir0, fir2, fir1, fir3 };
      int16_t *dsts[4] = { &flipped0[x * temp_stride + y], &flipped2[x * temp_stride + y], &flipped1[x * temp_stride + y], &flipped3[x * temp_stride + y]};
      kvz_filter_flip_shift_x8_dual_avx2(&src[src_stride*ypos + xpos], src_stride, &firs[0], shift1, &dsts[0]);
      kvz_filter_flip_shift_x8_dual_avx2(&src[src_stride*ypos + xpos], src_stride, &firs[2], shift1, &dsts[2]);
    }

    for (; y < height + KVZ_EXT_PADDING + 1; ++y) {
      int ypos = y - FILTER_OFFSET;
      int xpos = x - FILTER_OFFSET;
      flipped0[x * temp_stride + y] = kvz_eight_tap_filter_hor_avx2(fir0, &src[src_stride*ypos + xpos]) >> shift1;
      flipped2[x * temp_stride + y] = kvz_eight_tap_filter_hor_avx2(fir2, &src[src_stride*ypos + xpos]) >> shift1;
      flipped1[x * temp_stride + y] = kvz_eight_tap_filter_hor_avx2(fir1, &src[src_stride*ypos + xpos]) >> shift1;
      flipped3[x * temp_stride + y] = kvz_eight_tap_filter_hor_avx2(fir3, &src[src_stride*ypos + xpos]) >> shift1;
    }
  }

  // Filter vertically and flip x and y
  for (x = 0; x + 8 < width + 1; x += 8) {
    for (y = 0; y < height + 1; ++y) {
      
      // HPEL
      int8_t *firs0[2] = { fir0, fir2 };
      kvz_pixel *dsts0[2] = { &filtered[0][y * dst_stride + x], &filtered[1][y * dst_stride + x]};
      int16_t *srcs0[4] = { flipped2 + x * temp_stride + y, flipped0 + x * temp_stride + y};
      kvz_filter_flip_round_clip_x8_16bit_dual_avx2(srcs0, temp_stride, firs0, offset23, shift2 + shift3, dsts0);
      kvz_filter_flip_round_clip_x8_16bit_avx2(flipped2 + x * temp_stride + y, temp_stride, fir2, offset23, shift2 + shift3, &filtered[2][y * dst_stride + x]);
     
      // QPEL
      // Horizontal
      int8_t *firs1[4] = { fir0, fir0, fir2, fir2 };
      kvz_pixel *dsts1[4] = { &filtered[3][y * dst_stride + x], &filtered[4][y * dst_stride + x], 
                              &filtered[5][y * dst_stride + x], &filtered[6][y * dst_stride + x] };
      int16_t *srcs1[4] = { flipped1 + x * temp_stride + y, flipped3 + x * temp_stride + y, 
                            flipped1 + x * temp_stride + y, flipped3 + x * temp_stride + y, };
      kvz_filter_flip_round_clip_x8_16bit_dual_avx2(&srcs1[0], temp_stride, &firs1[0], offset23, shift2 + shift3, &dsts1[0]);
      kvz_filter_flip_round_clip_x8_16bit_dual_avx2(&srcs1[2], temp_stride, &firs1[2], offset23, shift2 + shift3, &dsts1[2]);

      // Vertical
      int8_t *firs2[4] = { fir1, fir1, fir3, fir3 };
      kvz_pixel *dsts2[4] = { &filtered[7][y * dst_stride + x], &filtered[8][y * dst_stride + x], 
                              &filtered[9][y * dst_stride + x], &filtered[10][y * dst_stride + x] };
      int16_t *srcs2[4] = { flipped0 + x * temp_stride + y, flipped2 + x * temp_stride + y, 
                            flipped0 + x * temp_stride + y, flipped2 + x * temp_stride + y, };
      kvz_filter_flip_round_clip_x8_16bit_dual_avx2(&srcs2[0], temp_stride, &firs2[0], offset23, shift2 + shift3, &dsts2[0]);
      kvz_filter_flip_round_clip_x8_16bit_dual_avx2(&srcs2[2], temp_stride, &firs2[2], offset23, shift2 + shift3, &dsts2[2]);

      // Diagonal
      int8_t *firs3[4] = { fir1, fir1, fir3, fir3 };
      kvz_pixel *dsts3[4] = { &filtered[11][y * dst_stride + x], &filtered[12][y * dst_stride + x], 
                              &filtered[13][y * dst_stride + x], &filtered[14][y * dst_stride + x] };
      int16_t *srcs3[4] = { flipped1 + x * temp_stride + y, flipped3 + x * temp_stride + y, 
                            flipped1 + x * temp_stride + y, flipped3 + x * temp_stride + y, };
      kvz_filter_flip_round_clip_x8_16bit_dual_avx2(&srcs3[0], temp_stride, &firs3[0], offset23, shift2 + shift3, &dsts3[0]);
      kvz_filter_flip_round_clip_x8_16bit_dual_avx2(&srcs3[2], temp_stride, &firs3[2], offset23, shift2 + shift3, &dsts3[2]);
    }
  }

  // The remaining pixels
  for (; x < width + 1; ++x) {
    for (y = 0; y < height + 1; ++y) {

      // HPEL
      filtered[0][y * dst_stride + x] = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_hor_16bit_avx2(fir0, &flipped2[x * temp_stride + y]) + offset23) >> shift2) >> shift3);
      filtered[1][y * dst_stride + x] = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_hor_16bit_avx2(fir2, &flipped0[x * temp_stride + y]) + offset23) >> shift2) >> shift3);
      filtered[2][y * dst_stride + x] = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_hor_16bit_avx2(fir2, &flipped2[x * temp_stride + y]) + offset23) >> shift2) >> shift3);

      // QPEL
      // Horizontal
      filtered[3][y * dst_stride + x] = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_hor_16bit_avx2(fir0, &flipped1[x * temp_stride + y]) + offset23) >> shift2) >> shift3);
      filtered[4][y * dst_stride + x] = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_hor_16bit_avx2(fir0, &flipped3[x * temp_stride + y]) + offset23) >> shift2) >> shift3);
      filtered[5][y * dst_stride + x] = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_hor_16bit_avx2(fir2, &flipped1[x * temp_stride + y]) + offset23) >> shift2) >> shift3);
      filtered[6][y * dst_stride + x] = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_hor_16bit_avx2(fir2, &flipped3[x * temp_stride + y]) + offset23) >> shift2) >> shift3);

      // Vertical
      filtered[7][y * dst_stride + x] = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_hor_16bit_avx2(fir1, &flipped0[x * temp_stride + y]) + offset23) >> shift2) >> shift3);
      filtered[8][y * dst_stride + x] = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_hor_16bit_avx2(fir1, &flipped2[x * temp_stride + y]) + offset23) >> shift2) >> shift3);
      filtered[9][y * dst_stride + x] = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_hor_16bit_avx2(fir3, &flipped0[x * temp_stride + y]) + offset23) >> shift2) >> shift3);
      filtered[10][y * dst_stride + x] = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_hor_16bit_avx2(fir3, &flipped2[x * temp_stride + y]) + offset23) >> shift2) >> shift3);

      // Diagonal
      filtered[11][y * dst_stride + x] = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_hor_16bit_avx2(fir1, &flipped1[x * temp_stride + y]) + offset23) >> shift2) >> shift3);
      filtered[12][y * dst_stride + x] = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_hor_16bit_avx2(fir1, &flipped3[x * temp_stride + y]) + offset23) >> shift2) >> shift3);
      filtered[13][y * dst_stride + x] = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_hor_16bit_avx2(fir3, &flipped1[x * temp_stride + y]) + offset23) >> shift2) >> shift3);
      filtered[14][y * dst_stride + x] = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_hor_16bit_avx2(fir3, &flipped3[x * temp_stride + y]) + offset23) >> shift2) >> shift3);
    }
  }
}

void kvz_filter_frac_blocks_luma_avx2(const encoder_control_t * const encoder, kvz_pixel *src, int16_t src_stride, int width, int height, frac_search_block filtered[15], int8_t fme_level)
{
  switch (fme_level) {
    case 1:
      kvz_filter_hpel_blocks_hor_ver_luma_avx2(encoder, src, src_stride, width, height, filtered);
      break;
    case 2:
      kvz_filter_hpel_blocks_full_luma_avx2(encoder, src, src_stride, width, height, filtered);
      break;
    case 3:
      kvz_filter_qpel_blocks_hor_ver_luma_avx2(encoder, src, src_stride, width, height, filtered);
      break;
    default:
      kvz_filter_qpel_blocks_full_luma_avx2(encoder, src, src_stride, width, height, filtered);
      break;
  }
}

void kvz_sample_quarterpel_luma_avx2(const encoder_control_t * const encoder, kvz_pixel *src, int16_t src_stride, int width, int height,kvz_pixel *dst, int16_t dst_stride, int8_t hor_flag, int8_t ver_flag, const int16_t mv[2])
{
  //Check for amp
  if (width != height) {
    kvz_sample_quarterpel_luma_generic(encoder, src, src_stride, width, height, dst, dst_stride, hor_flag, ver_flag, mv);
    return;
  }
  //TODO: horizontal and vertical only filtering
  int32_t x, y;
  int16_t shift1 = KVZ_BIT_DEPTH - 8;
  int32_t shift2 = 6;
  int32_t shift3 = 14 - KVZ_BIT_DEPTH;
  int32_t offset23 = 1 << (shift2 + shift3 - 1);

  int8_t *hor_filter = kvz_g_luma_filter[mv[0] & 3];
  int8_t *ver_filter = kvz_g_luma_filter[mv[1] & 3];

  int16_t hor_filtered[(LCU_WIDTH + 1) + FILTER_SIZE][(LCU_WIDTH + 1) + FILTER_SIZE];

  if (width == 4) {
    // Filter horizontally and flip x and y
    for (y = 0; y < height + FILTER_SIZE - 1; ++y) {
      for (x = 0; x < width; x += 4) {
        int ypos = y - FILTER_OFFSET;
        int xpos = x - FILTER_OFFSET;
        int16_t *out = &(hor_filtered[y][x]);
        kvz_eight_tap_filter_x4_hor_avx2(hor_filter, &src[src_stride*ypos + xpos], shift1, out);
      }
    }

    // Filter vertically and flip x and y
    for (y = 0; y < height; ++y) {
      for (x = 0; x < width; x+=4) {
        int ypos = y;
        int xpos = x;
        *(int32_t*)&(dst[y*dst_stride + x]) = kvz_eight_tap_filter_x4_ver_16bit_avx2(ver_filter, &hor_filtered[ypos][xpos], sizeof(hor_filtered[0])/sizeof(int16_t), offset23, shift2, shift3);
      }
    }

  } else {
    // Filter horizontally and flip x and y
    for (y = 0; y < height + FILTER_SIZE - 1; ++y) {
      for (x = 0; x < width; x+=8) {
        int ypos = y - FILTER_OFFSET;
        int xpos = x - FILTER_OFFSET;
        int16_t *dst = &(hor_filtered[y][x]);
        kvz_eight_tap_filter_x8_hor_avx2(hor_filter, &src[src_stride*ypos + xpos], shift1, dst);
      }
    }

    // Filter vertically and flip x and y
    for (y = 0; y < height; ++y) {
      for (x = 0; x < width; x+=8) {
        int ypos = y;
        int xpos = x;
        kvz_pixel *out = &(dst[y*dst_stride + x]);
        kvz_eight_tap_filter_x8_ver_16bit_avx2(ver_filter, &hor_filtered[ypos][xpos], sizeof(hor_filtered[0])/sizeof(int16_t), offset23, shift2, shift3, out);
      }
    }
  }
}

void kvz_sample_octpel_chroma_avx2(const encoder_control_t * const encoder, kvz_pixel *src, int16_t src_stride, int width, int height,kvz_pixel *dst, int16_t dst_stride, int8_t hor_flag, int8_t ver_flag, const int16_t mv[2])
{
  //Check for amp
  if (width != height) {
    kvz_sample_octpel_chroma_generic(encoder, src, src_stride, width, height, dst, dst_stride, hor_flag, ver_flag, mv);
    return;
  }
  //TODO: horizontal and vertical only filtering
  int32_t x, y;
  int16_t shift1 = KVZ_BIT_DEPTH - 8;
  int32_t shift2 = 6;
  int32_t shift3 = 14 - KVZ_BIT_DEPTH;
  int32_t offset23 = 1 << (shift2 + shift3 - 1);

  int8_t *hor_filter = kvz_g_chroma_filter[mv[0] & 7];
  int8_t *ver_filter = kvz_g_chroma_filter[mv[1] & 7];

#define FILTER_SIZE_C (FILTER_SIZE / 2)
#define FILTER_OFFSET_C (FILTER_OFFSET / 2)
  int16_t hor_filtered[(LCU_WIDTH_C + 1) + FILTER_SIZE_C][(LCU_WIDTH_C + 1) + FILTER_SIZE_C];

  if (width == 4) {
    // Filter horizontally and flip x and y
    for (y = 0; y < height + FILTER_SIZE_C - 1; ++y) {
      for (x = 0; x < width; x += 4) {
        int ypos = y - FILTER_OFFSET_C;
        int xpos = x - FILTER_OFFSET_C;
        int16_t *out = &(hor_filtered[y][x]);
        kvz_four_tap_filter_x4_hor_avx2(hor_filter, &src[src_stride*ypos + xpos], shift1, out);
      }
    }

    // Filter vertically and flip x and y
    for (y = 0; y < height; ++y) {
      for (x = 0; x < width; x+=4) {
        int ypos = y;
        int xpos = x;
        *(int32_t*)&(dst[y*dst_stride + x]) = kvz_four_tap_filter_x4_ver_16bit_avx2(ver_filter, &hor_filtered[ypos][xpos], sizeof(hor_filtered[0])/sizeof(int16_t), offset23, shift2, shift3);
      }
    }

  } else {
    // Filter horizontally and flip x and y
    for (y = 0; y < height + FILTER_SIZE_C - 1; ++y) {
      for (x = 0; x < width; x += 8) {
        int ypos = y - FILTER_OFFSET_C;
        int xpos = x - FILTER_OFFSET_C;
        int16_t *dst = &(hor_filtered[y][x]);
        kvz_four_tap_filter_x8_hor_avx2(hor_filter, &src[src_stride*ypos + xpos], shift1, dst);
      }
    }

    // Filter vertically and flip x and y
    for (y = 0; y < height; ++y) {
      for (x = 0; x < width; x+=8) {
        int ypos = y;
        int xpos = x;
        kvz_pixel *out = &(dst[y*dst_stride + x]);
        kvz_four_tap_filter_x8_ver_16bit_avx2(ver_filter, &hor_filtered[ypos][xpos], sizeof(hor_filtered[0])/sizeof(int16_t), offset23, shift2, shift3, out);
      }
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
    success &= kvz_strategyselector_register(opaque, "filter_inter_quarterpel_luma", "avx2", 40, &kvz_filter_inter_quarterpel_luma_avx2);
    success &= kvz_strategyselector_register(opaque, "filter_inter_halfpel_chroma", "avx2", 40, &kvz_filter_inter_halfpel_chroma_avx2);
    success &= kvz_strategyselector_register(opaque, "filter_inter_octpel_chroma", "avx2", 40, &kvz_filter_inter_octpel_chroma_avx2);
    success &= kvz_strategyselector_register(opaque, "filter_frac_blocks_luma", "avx2", 40, &kvz_filter_frac_blocks_luma_avx2);
    success &= kvz_strategyselector_register(opaque, "sample_quarterpel_luma", "avx2", 40, &kvz_sample_quarterpel_luma_avx2);
    success &= kvz_strategyselector_register(opaque, "sample_octpel_chroma", "avx2", 40, &kvz_sample_octpel_chroma_avx2);
  }
  success &= kvz_strategyselector_register(opaque, "get_extended_block", "avx2", 40, &kvz_get_extended_block_avx2);
#endif //COMPILE_INTEL_AVX2
  return success;
}
