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
#include "strategies/avx2/picture-avx2.h"

#if COMPILE_INTEL_AVX2
#include <immintrin.h>
#include <string.h>

#include "kvazaar.h"
#include "strategies/strategies-picture.h"
#include "strategyselector.h"
#include "strategies/generic/picture-generic.h"


/**
* \brief Calculate SAD for 8x8 bytes in continuous memory.
*/
static INLINE __m256i inline_8bit_sad_8x8_avx2(const __m256i *const a, const __m256i *const b)
{
  __m256i sum0, sum1;
  sum0 = _mm256_sad_epu8(_mm256_load_si256(a + 0), _mm256_load_si256(b + 0));
  sum1 = _mm256_sad_epu8(_mm256_load_si256(a + 1), _mm256_load_si256(b + 1));

  return _mm256_add_epi32(sum0, sum1);
}


/**
* \brief Calculate SAD for 16x16 bytes in continuous memory.
*/
static INLINE __m256i inline_8bit_sad_16x16_avx2(const __m256i *const a, const __m256i *const b)
{
  const unsigned size_of_8x8 = 8 * 8 / sizeof(__m256i);

  // Calculate in 4 chunks of 16x4.
  __m256i sum0, sum1, sum2, sum3;
  sum0 = inline_8bit_sad_8x8_avx2(a + 0 * size_of_8x8, b + 0 * size_of_8x8);
  sum1 = inline_8bit_sad_8x8_avx2(a + 1 * size_of_8x8, b + 1 * size_of_8x8);
  sum2 = inline_8bit_sad_8x8_avx2(a + 2 * size_of_8x8, b + 2 * size_of_8x8);
  sum3 = inline_8bit_sad_8x8_avx2(a + 3 * size_of_8x8, b + 3 * size_of_8x8);

  sum0 = _mm256_add_epi32(sum0, sum1);
  sum2 = _mm256_add_epi32(sum2, sum3);

  return _mm256_add_epi32(sum0, sum2);
}


/**
* \brief Get sum of the low 32 bits of four 64 bit numbers from __m256i as uint32_t.
*/
static INLINE uint32_t m256i_horizontal_sum(const __m256i sum)
{
  // Add the high 128 bits to low 128 bits.
  __m128i mm128_result = _mm_add_epi32(_mm256_castsi256_si128(sum), _mm256_extractf128_si256(sum, 1));
  // Add the high 64 bits  to low 64 bits.
  uint32_t result[4];
  _mm_storeu_si128((__m128i*)result, mm128_result);
  return result[0] + result[2];
}


static unsigned sad_8bit_8x8_avx2(const kvz_pixel *buf1, const kvz_pixel *buf2)
{
  const __m256i *const a = (const __m256i *)buf1;
  const __m256i *const b = (const __m256i *)buf2;
  __m256i sum = inline_8bit_sad_8x8_avx2(a, b);

  return m256i_horizontal_sum(sum);
}


static unsigned sad_8bit_16x16_avx2(const kvz_pixel *buf1, const kvz_pixel *buf2)
{
  const __m256i *const a = (const __m256i *)buf1;
  const __m256i *const b = (const __m256i *)buf2;
  __m256i sum = inline_8bit_sad_16x16_avx2(a, b);

  return m256i_horizontal_sum(sum);
}


static unsigned sad_8bit_32x32_avx2(const kvz_pixel *buf1, const kvz_pixel *buf2)
{
  const __m256i *const a = (const __m256i *)buf1;
  const __m256i *const b = (const __m256i *)buf2;

  const unsigned size_of_8x8 = 8 * 8 / sizeof(__m256i);
  const unsigned size_of_32x32 = 32 * 32 / sizeof(__m256i);

  // Looping 512 bytes at a time seems faster than letting VC figure it out
  // through inlining, like inline_8bit_sad_16x16_avx2 does.
  __m256i sum0 = inline_8bit_sad_8x8_avx2(a, b);
  for (unsigned i = size_of_8x8; i < size_of_32x32; i += size_of_8x8) {
    __m256i sum1 = inline_8bit_sad_8x8_avx2(a + i, b + i);
    sum0 = _mm256_add_epi32(sum0, sum1);
  }

  return m256i_horizontal_sum(sum0);
}


static unsigned sad_8bit_64x64_avx2(const kvz_pixel * buf1, const kvz_pixel * buf2)
{
  const __m256i *const a = (const __m256i *)buf1;
  const __m256i *const b = (const __m256i *)buf2;

  const unsigned size_of_8x8 = 8 * 8 / sizeof(__m256i);
  const unsigned size_of_64x64 = 64 * 64 / sizeof(__m256i);

  // Looping 512 bytes at a time seems faster than letting VC figure it out
  // through inlining, like inline_8bit_sad_16x16_avx2 does.
  __m256i sum0 = inline_8bit_sad_8x8_avx2(a, b);
  for (unsigned i = size_of_8x8; i < size_of_64x64; i += size_of_8x8) {
    __m256i sum1 = inline_8bit_sad_8x8_avx2(a + i, b + i);
    sum0 = _mm256_add_epi32(sum0, sum1);
  }

  return m256i_horizontal_sum(sum0);
}

static unsigned satd_4x4_8bit_avx2(const kvz_pixel *org, const kvz_pixel *cur)
{

  __m128i original = _mm_cvtepu8_epi16(_mm_loadl_epi64((__m128i*)org));
  __m128i current = _mm_cvtepu8_epi16(_mm_loadl_epi64((__m128i*)cur));

  __m128i diff_lo = _mm_sub_epi16(current, original);

  original = _mm_cvtepu8_epi16(_mm_loadl_epi64((__m128i*)(org + 8)));
  current = _mm_cvtepu8_epi16(_mm_loadl_epi64((__m128i*)(cur + 8)));

  __m128i diff_hi = _mm_sub_epi16(current, original);


  //Hor
  __m128i row0 = _mm_hadd_epi16(diff_lo, diff_hi);
  __m128i row1 = _mm_hsub_epi16(diff_lo, diff_hi);

  __m128i row2 = _mm_hadd_epi16(row0, row1);
  __m128i row3 = _mm_hsub_epi16(row0, row1);

  //Ver
  row0 = _mm_hadd_epi16(row2, row3);
  row1 = _mm_hsub_epi16(row2, row3);

  row2 = _mm_hadd_epi16(row0, row1);
  row3 = _mm_hsub_epi16(row0, row1);

  //Abs and sum
  row2 = _mm_abs_epi16(row2);
  row3 = _mm_abs_epi16(row3);

  row3 = _mm_add_epi16(row2, row3);

  row3 = _mm_add_epi16(row3, _mm_shuffle_epi32(row3, _MM_SHUFFLE(1, 0, 3, 2) ));
  row3 = _mm_add_epi16(row3, _mm_shuffle_epi32(row3, _MM_SHUFFLE(0, 1, 0, 1) ));
  row3 = _mm_add_epi16(row3, _mm_shufflelo_epi16(row3, _MM_SHUFFLE(0, 1, 0, 1) ));

  unsigned sum = _mm_extract_epi16(row3, 0);
  unsigned satd = (sum + 1) >> 1;

  return satd;
}


static void satd_8bit_4x4_dual_avx2(
  const pred_buffer preds, const kvz_pixel * const orig, unsigned num_modes, unsigned *satds_out) 
{

  __m256i original = _mm256_broadcastsi128_si256(_mm_cvtepu8_epi16(_mm_loadl_epi64((__m128i*)orig)));
  __m256i pred = _mm256_cvtepu8_epi16(_mm_loadl_epi64((__m128i*)preds[0]));
  pred = _mm256_inserti128_si256(pred, _mm_cvtepu8_epi16(_mm_loadl_epi64((__m128i*)preds[1])), 1);

  __m256i diff_lo = _mm256_sub_epi16(pred, original);

  original = _mm256_broadcastsi128_si256(_mm_cvtepu8_epi16(_mm_loadl_epi64((__m128i*)(orig + 8))));
  pred = _mm256_cvtepu8_epi16(_mm_loadl_epi64((__m128i*)(preds[0] + 8)));
  pred = _mm256_inserti128_si256(pred, _mm_cvtepu8_epi16(_mm_loadl_epi64((__m128i*)(preds[1] + 8))), 1);

  __m256i diff_hi = _mm256_sub_epi16(pred, original);

  //Hor
  __m256i row0 = _mm256_hadd_epi16(diff_lo, diff_hi);
  __m256i row1 = _mm256_hsub_epi16(diff_lo, diff_hi);

  __m256i row2 = _mm256_hadd_epi16(row0, row1);
  __m256i row3 = _mm256_hsub_epi16(row0, row1);

  //Ver
  row0 = _mm256_hadd_epi16(row2, row3);
  row1 = _mm256_hsub_epi16(row2, row3);

  row2 = _mm256_hadd_epi16(row0, row1);
  row3 = _mm256_hsub_epi16(row0, row1);

  //Abs and sum
  row2 = _mm256_abs_epi16(row2);
  row3 = _mm256_abs_epi16(row3);

  row3 = _mm256_add_epi16(row2, row3);

  row3 = _mm256_add_epi16(row3, _mm256_shuffle_epi32(row3, _MM_SHUFFLE(1, 0, 3, 2) ));
  row3 = _mm256_add_epi16(row3, _mm256_shuffle_epi32(row3, _MM_SHUFFLE(0, 1, 0, 1) ));
  row3 = _mm256_add_epi16(row3, _mm256_shufflelo_epi16(row3, _MM_SHUFFLE(0, 1, 0, 1) ));

  unsigned sum1 = _mm_extract_epi16(_mm256_castsi256_si128(row3), 0);
  sum1 = (sum1 + 1) >> 1;

  unsigned sum2 = _mm_extract_epi16(_mm256_extracti128_si256(row3, 1), 0);
  sum2 = (sum2 + 1) >> 1;

  satds_out[0] = sum1;
  satds_out[1] = sum2;
}

static INLINE void hor_transform_row_avx2(__m128i* row){
  
  __m128i mask_pos = _mm_set1_epi16(1);
  __m128i mask_neg = _mm_set1_epi16(-1);
  __m128i sign_mask = _mm_unpacklo_epi64(mask_pos, mask_neg);
  __m128i temp = _mm_shuffle_epi32(*row, _MM_SHUFFLE(1, 0, 3, 2));
  *row = _mm_sign_epi16(*row, sign_mask);
  *row = _mm_add_epi16(*row, temp);

  sign_mask = _mm_unpacklo_epi32(mask_pos, mask_neg);
  temp = _mm_shuffle_epi32(*row, _MM_SHUFFLE(2, 3, 0, 1));
  *row = _mm_sign_epi16(*row, sign_mask);
  *row = _mm_add_epi16(*row, temp);

  sign_mask = _mm_unpacklo_epi16(mask_pos, mask_neg);
  temp = _mm_shufflelo_epi16(*row, _MM_SHUFFLE(2,3,0,1));
  temp = _mm_shufflehi_epi16(temp, _MM_SHUFFLE(2,3,0,1));
  *row = _mm_sign_epi16(*row, sign_mask);
  *row = _mm_add_epi16(*row, temp);
}

static INLINE void hor_transform_row_dual_avx2(__m256i* row){
  
  __m256i mask_pos = _mm256_set1_epi16(1);
  __m256i mask_neg = _mm256_set1_epi16(-1);
  __m256i sign_mask = _mm256_unpacklo_epi64(mask_pos, mask_neg);
  __m256i temp = _mm256_shuffle_epi32(*row, _MM_SHUFFLE(1, 0, 3, 2));
  *row = _mm256_sign_epi16(*row, sign_mask);
  *row = _mm256_add_epi16(*row, temp);

  sign_mask = _mm256_unpacklo_epi32(mask_pos, mask_neg);
  temp = _mm256_shuffle_epi32(*row, _MM_SHUFFLE(2, 3, 0, 1));
  *row = _mm256_sign_epi16(*row, sign_mask);
  *row = _mm256_add_epi16(*row, temp);

  sign_mask = _mm256_unpacklo_epi16(mask_pos, mask_neg);
  temp = _mm256_shufflelo_epi16(*row, _MM_SHUFFLE(2,3,0,1));
  temp = _mm256_shufflehi_epi16(temp, _MM_SHUFFLE(2,3,0,1));
  *row = _mm256_sign_epi16(*row, sign_mask);
  *row = _mm256_add_epi16(*row, temp);
}

static INLINE void add_sub_avx2(__m128i *out, __m128i *in, unsigned out_idx0, unsigned out_idx1, unsigned in_idx0, unsigned in_idx1)
{
  out[out_idx0] = _mm_add_epi16(in[in_idx0], in[in_idx1]);
  out[out_idx1] = _mm_sub_epi16(in[in_idx0], in[in_idx1]);
}

static INLINE void ver_transform_block_avx2(__m128i (*rows)[8]){

  __m128i temp0[8];
  add_sub_avx2(temp0, (*rows), 0, 1, 0, 1);
  add_sub_avx2(temp0, (*rows), 2, 3, 2, 3);
  add_sub_avx2(temp0, (*rows), 4, 5, 4, 5);
  add_sub_avx2(temp0, (*rows), 6, 7, 6, 7);

  __m128i temp1[8];
  add_sub_avx2(temp1, temp0, 0, 1, 0, 2);
  add_sub_avx2(temp1, temp0, 2, 3, 1, 3);
  add_sub_avx2(temp1, temp0, 4, 5, 4, 6);
  add_sub_avx2(temp1, temp0, 6, 7, 5, 7);

  add_sub_avx2((*rows), temp1, 0, 1, 0, 4);
  add_sub_avx2((*rows), temp1, 2, 3, 1, 5);
  add_sub_avx2((*rows), temp1, 4, 5, 2, 6);
  add_sub_avx2((*rows), temp1, 6, 7, 3, 7);
  
}

static INLINE void add_sub_dual_avx2(__m256i *out, __m256i *in, unsigned out_idx0, unsigned out_idx1, unsigned in_idx0, unsigned in_idx1)
{
  out[out_idx0] = _mm256_add_epi16(in[in_idx0], in[in_idx1]);
  out[out_idx1] = _mm256_sub_epi16(in[in_idx0], in[in_idx1]);
}


static INLINE void ver_transform_block_dual_avx2(__m256i (*rows)[8]){

  __m256i temp0[8];
  add_sub_dual_avx2(temp0, (*rows), 0, 1, 0, 1);
  add_sub_dual_avx2(temp0, (*rows), 2, 3, 2, 3);
  add_sub_dual_avx2(temp0, (*rows), 4, 5, 4, 5);
  add_sub_dual_avx2(temp0, (*rows), 6, 7, 6, 7);

  __m256i temp1[8];
  add_sub_dual_avx2(temp1, temp0, 0, 1, 0, 2);
  add_sub_dual_avx2(temp1, temp0, 2, 3, 1, 3);
  add_sub_dual_avx2(temp1, temp0, 4, 5, 4, 6);
  add_sub_dual_avx2(temp1, temp0, 6, 7, 5, 7);

  add_sub_dual_avx2((*rows), temp1, 0, 1, 0, 4);
  add_sub_dual_avx2((*rows), temp1, 2, 3, 1, 5);
  add_sub_dual_avx2((*rows), temp1, 4, 5, 2, 6);
  add_sub_dual_avx2((*rows), temp1, 6, 7, 3, 7);
  
}

INLINE static void haddwd_accumulate_avx2(__m128i *accumulate, __m128i *ver_row)
{
  __m128i abs_value = _mm_abs_epi16(*ver_row);
  *accumulate = _mm_add_epi32(*accumulate, _mm_madd_epi16(abs_value, _mm_set1_epi16(1)));
}

INLINE static void haddwd_accumulate_dual_avx2(__m256i *accumulate, __m256i *ver_row)
{
  __m256i abs_value = _mm256_abs_epi16(*ver_row);
  *accumulate = _mm256_add_epi32(*accumulate, _mm256_madd_epi16(abs_value, _mm256_set1_epi16(1)));
}

INLINE static unsigned sum_block_avx2(__m128i *ver_row)
{
  __m128i sad = _mm_setzero_si128();
  haddwd_accumulate_avx2(&sad, ver_row + 0);
  haddwd_accumulate_avx2(&sad, ver_row + 1);
  haddwd_accumulate_avx2(&sad, ver_row + 2);
  haddwd_accumulate_avx2(&sad, ver_row + 3); 
  haddwd_accumulate_avx2(&sad, ver_row + 4);
  haddwd_accumulate_avx2(&sad, ver_row + 5);
  haddwd_accumulate_avx2(&sad, ver_row + 6);
  haddwd_accumulate_avx2(&sad, ver_row + 7);

  sad = _mm_add_epi32(sad, _mm_shuffle_epi32(sad, _MM_SHUFFLE(1, 0, 3, 2)));
  sad = _mm_add_epi32(sad, _mm_shuffle_epi32(sad, _MM_SHUFFLE(0, 1, 0, 1)));

  return _mm_cvtsi128_si32(sad);
}

INLINE static void sum_block_dual_avx2(__m256i *ver_row, unsigned *sum0, unsigned *sum1)
{
  __m256i sad = _mm256_setzero_si256();
  haddwd_accumulate_dual_avx2(&sad, ver_row + 0);
  haddwd_accumulate_dual_avx2(&sad, ver_row + 1);
  haddwd_accumulate_dual_avx2(&sad, ver_row + 2);
  haddwd_accumulate_dual_avx2(&sad, ver_row + 3); 
  haddwd_accumulate_dual_avx2(&sad, ver_row + 4);
  haddwd_accumulate_dual_avx2(&sad, ver_row + 5);
  haddwd_accumulate_dual_avx2(&sad, ver_row + 6);
  haddwd_accumulate_dual_avx2(&sad, ver_row + 7);

  sad = _mm256_add_epi32(sad, _mm256_shuffle_epi32(sad, _MM_SHUFFLE(1, 0, 3, 2)));
  sad = _mm256_add_epi32(sad, _mm256_shuffle_epi32(sad, _MM_SHUFFLE(0, 1, 0, 1)));

  *sum0 = _mm_cvtsi128_si32(_mm256_extracti128_si256(sad, 0));
  *sum1 = _mm_cvtsi128_si32(_mm256_extracti128_si256(sad, 1));
}

INLINE static __m128i diff_row_avx2(const kvz_pixel *buf1, const kvz_pixel *buf2)
{
  __m128i buf1_row = _mm_cvtepu8_epi16(_mm_loadl_epi64((__m128i*)buf1));
  __m128i buf2_row = _mm_cvtepu8_epi16(_mm_loadl_epi64((__m128i*)buf2));
  return _mm_sub_epi16(buf1_row, buf2_row);
}

INLINE static __m256i diff_row_dual_avx2(const kvz_pixel *buf1, const kvz_pixel *buf2, const kvz_pixel *orig)
{
  __m128i temp1 = _mm_loadl_epi64((__m128i*)buf1);
  __m128i temp2 = _mm_loadl_epi64((__m128i*)buf2);
  __m128i temp3 = _mm_loadl_epi64((__m128i*)orig);
  __m256i buf1_row = _mm256_cvtepu8_epi16(_mm_unpacklo_epi64(temp1, temp2));
  __m256i buf2_row = _mm256_cvtepu8_epi16(_mm_broadcastq_epi64(temp3));

  return _mm256_sub_epi16(buf1_row, buf2_row);
}

INLINE static void diff_blocks_avx2(__m128i (*row_diff)[8],
                                                           const kvz_pixel * buf1, unsigned stride1,
                                                           const kvz_pixel * orig, unsigned stride_orig)
{
  (*row_diff)[0] = diff_row_avx2(buf1 + 0 * stride1, orig + 0 * stride_orig);
  (*row_diff)[1] = diff_row_avx2(buf1 + 1 * stride1, orig + 1 * stride_orig);
  (*row_diff)[2] = diff_row_avx2(buf1 + 2 * stride1, orig + 2 * stride_orig);
  (*row_diff)[3] = diff_row_avx2(buf1 + 3 * stride1, orig + 3 * stride_orig);
  (*row_diff)[4] = diff_row_avx2(buf1 + 4 * stride1, orig + 4 * stride_orig);
  (*row_diff)[5] = diff_row_avx2(buf1 + 5 * stride1, orig + 5 * stride_orig);
  (*row_diff)[6] = diff_row_avx2(buf1 + 6 * stride1, orig + 6 * stride_orig);
  (*row_diff)[7] = diff_row_avx2(buf1 + 7 * stride1, orig + 7 * stride_orig);

}

INLINE static void diff_blocks_dual_avx2(__m256i (*row_diff)[8],
                                                           const kvz_pixel * buf1, unsigned stride1,
                                                           const kvz_pixel * buf2, unsigned stride2,
                                                           const kvz_pixel * orig, unsigned stride_orig)
{
  (*row_diff)[0] = diff_row_dual_avx2(buf1 + 0 * stride1, buf2 + 0 * stride2, orig + 0 * stride_orig);
  (*row_diff)[1] = diff_row_dual_avx2(buf1 + 1 * stride1, buf2 + 1 * stride2, orig + 1 * stride_orig);
  (*row_diff)[2] = diff_row_dual_avx2(buf1 + 2 * stride1, buf2 + 2 * stride2, orig + 2 * stride_orig);
  (*row_diff)[3] = diff_row_dual_avx2(buf1 + 3 * stride1, buf2 + 3 * stride2, orig + 3 * stride_orig);
  (*row_diff)[4] = diff_row_dual_avx2(buf1 + 4 * stride1, buf2 + 4 * stride2, orig + 4 * stride_orig);
  (*row_diff)[5] = diff_row_dual_avx2(buf1 + 5 * stride1, buf2 + 5 * stride2, orig + 5 * stride_orig);
  (*row_diff)[6] = diff_row_dual_avx2(buf1 + 6 * stride1, buf2 + 6 * stride2, orig + 6 * stride_orig);
  (*row_diff)[7] = diff_row_dual_avx2(buf1 + 7 * stride1, buf2 + 7 * stride2, orig + 7 * stride_orig);

}

INLINE static void hor_transform_block_avx2(__m128i (*row_diff)[8])
{
  hor_transform_row_avx2((*row_diff) + 0);
  hor_transform_row_avx2((*row_diff) + 1);
  hor_transform_row_avx2((*row_diff) + 2);
  hor_transform_row_avx2((*row_diff) + 3);
  hor_transform_row_avx2((*row_diff) + 4);
  hor_transform_row_avx2((*row_diff) + 5);
  hor_transform_row_avx2((*row_diff) + 6);
  hor_transform_row_avx2((*row_diff) + 7);
}

INLINE static void hor_transform_block_dual_avx2(__m256i (*row_diff)[8])
{
  hor_transform_row_dual_avx2((*row_diff) + 0);
  hor_transform_row_dual_avx2((*row_diff) + 1);
  hor_transform_row_dual_avx2((*row_diff) + 2);
  hor_transform_row_dual_avx2((*row_diff) + 3);
  hor_transform_row_dual_avx2((*row_diff) + 4);
  hor_transform_row_dual_avx2((*row_diff) + 5);
  hor_transform_row_dual_avx2((*row_diff) + 6);
  hor_transform_row_dual_avx2((*row_diff) + 7);
}

static void kvz_satd_8bit_8x8_general_dual_avx2(const kvz_pixel * buf1, unsigned stride1,
                                                const kvz_pixel * buf2, unsigned stride2,
                                                const kvz_pixel * orig, unsigned stride_orig,
                                                unsigned *sum0, unsigned *sum1)
{
  __m256i temp[8];

  diff_blocks_dual_avx2(&temp, buf1, stride1, buf2, stride2, orig, stride_orig);
  hor_transform_block_dual_avx2(&temp);
  ver_transform_block_dual_avx2(&temp);
  
  sum_block_dual_avx2(temp, sum0, sum1);

  *sum0 = (*sum0 + 2) >> 2;
  *sum1 = (*sum1 + 2) >> 2;
}

/**
* \brief  Calculate SATD between two 4x4 blocks inside bigger arrays.
*/
static unsigned kvz_satd_4x4_subblock_8bit_avx2(const kvz_pixel * buf1,
                                                const int32_t     stride1,
                                                const kvz_pixel * buf2,
                                                const int32_t     stride2)
{
  // TODO: AVX2 implementation
  return kvz_satd_4x4_subblock_generic(buf1, stride1, buf2, stride2);
}

static void kvz_satd_4x4_subblock_quad_avx2(const kvz_pixel *preds[4],
                                       const int strides[4],
                                       const kvz_pixel *orig,
                                       const int orig_stride,
                                       unsigned costs[4])
{
  // TODO: AVX2 implementation
  kvz_satd_4x4_subblock_quad_generic(preds, strides, orig, orig_stride, costs);
}

static unsigned satd_8x8_subblock_8bit_avx2(const kvz_pixel * buf1, unsigned stride1, const kvz_pixel * buf2, unsigned stride2)
{
  __m128i temp[8];

  diff_blocks_avx2(&temp, buf1, stride1, buf2, stride2);
  hor_transform_block_avx2(&temp);
  ver_transform_block_avx2(&temp);
  
  unsigned sad = sum_block_avx2(temp);

  unsigned result = (sad + 2) >> 2;
  return result;
}

static void satd_8x8_subblock_quad_avx2(const kvz_pixel **preds,
  const int *strides,
  const kvz_pixel *orig,
  const int orig_stride,
  unsigned *costs)
{
  kvz_satd_8bit_8x8_general_dual_avx2(preds[0], strides[0], preds[1], strides[1], orig, orig_stride, &costs[0], &costs[1]);
  kvz_satd_8bit_8x8_general_dual_avx2(preds[2], strides[2], preds[3], strides[3], orig, orig_stride, &costs[2], &costs[3]);
}

SATD_NxN(8bit_avx2,  8)
SATD_NxN(8bit_avx2, 16)
SATD_NxN(8bit_avx2, 32)
SATD_NxN(8bit_avx2, 64)
SATD_ANY_SIZE(8bit_avx2)

// Function macro for defining hadamard calculating functions
// for fixed size blocks. They calculate hadamard for integer
// multiples of 8x8 with the 8x8 hadamard function.
#define SATD_NXN_DUAL_AVX2(n) \
static void satd_8bit_ ## n ## x ## n ## _dual_avx2( \
  const pred_buffer preds, const kvz_pixel * const orig, unsigned num_modes, unsigned *satds_out)  \
{ \
  unsigned x, y; \
  satds_out[0] = 0; \
  satds_out[1] = 0; \
  unsigned sum1 = 0; \
  unsigned sum2 = 0; \
  for (y = 0; y < (n); y += 8) { \
  unsigned row = y * (n); \
  for (x = 0; x < (n); x += 8) { \
  kvz_satd_8bit_8x8_general_dual_avx2(&preds[0][row + x], (n), &preds[1][row + x], (n), &orig[row + x], (n), &sum1, &sum2); \
  satds_out[0] += sum1; \
  satds_out[1] += sum2; \
    } \
    } \
  satds_out[0] >>= (KVZ_BIT_DEPTH-8); \
  satds_out[1] >>= (KVZ_BIT_DEPTH-8); \
}

static void satd_8bit_8x8_dual_avx2(
  const pred_buffer preds, const kvz_pixel * const orig, unsigned num_modes, unsigned *satds_out) 
{ 
  unsigned x, y; 
  satds_out[0] = 0;
  satds_out[1] = 0;
  unsigned sum1 = 0;
  unsigned sum2 = 0;
  for (y = 0; y < (8); y += 8) { 
  unsigned row = y * (8); 
  for (x = 0; x < (8); x += 8) { 
  kvz_satd_8bit_8x8_general_dual_avx2(&preds[0][row + x], (8), &preds[1][row + x], (8), &orig[row + x], (8), &sum1, &sum2); 
  satds_out[0] += sum1;
  satds_out[1] += sum2;
      } 
      } 
  satds_out[0] >>= (KVZ_BIT_DEPTH-8);
  satds_out[1] >>= (KVZ_BIT_DEPTH-8);
}

//SATD_NXN_DUAL_AVX2(8) //Use the non-macro version
SATD_NXN_DUAL_AVX2(16)
SATD_NXN_DUAL_AVX2(32)
SATD_NXN_DUAL_AVX2(64)

#define SATD_ANY_SIZE_MULTI_AVX2(suffix, num_parallel_blocks) \
  static cost_pixel_any_size_multi_func satd_any_size_## suffix; \
  static void satd_any_size_ ## suffix ( \
      int width, int height, \
      const kvz_pixel **preds, \
      const int *strides, \
      const kvz_pixel *orig, \
      const int orig_stride, \
      unsigned num_modes, \
      unsigned *costs_out, \
      int8_t *valid) \
  { \
    unsigned sums[num_parallel_blocks] = { 0 }; \
    const kvz_pixel *pred_ptrs[4] = { preds[0], preds[1], preds[2], preds[3] };\
    const kvz_pixel *orig_ptr = orig; \
    costs_out[0] = 0; costs_out[1] = 0; costs_out[2] = 0; costs_out[3] = 0; \
    if (width % 8 != 0) { \
      /* Process the first column using 4x4 blocks. */ \
      for (int y = 0; y < height; y += 4) { \
        kvz_satd_4x4_subblock_ ## suffix(preds, strides, orig, orig_stride, sums); \
            } \
      orig_ptr += 4; \
      for(int blk = 0; blk < num_parallel_blocks; ++blk){\
        pred_ptrs[blk] += 4; \
            }\
      width -= 4; \
            } \
    if (height % 8 != 0) { \
      /* Process the first row using 4x4 blocks. */ \
      for (int x = 0; x < width; x += 4 ) { \
        kvz_satd_4x4_subblock_ ## suffix(pred_ptrs, strides, orig_ptr, orig_stride, sums); \
            } \
      orig_ptr += 4 * orig_stride; \
      for(int blk = 0; blk < num_parallel_blocks; ++blk){\
        pred_ptrs[blk] += 4 * strides[blk]; \
            }\
      height -= 4; \
        } \
    /* The rest can now be processed with 8x8 blocks. */ \
    for (int y = 0; y < height; y += 8) { \
      orig_ptr = &orig[y * orig_stride]; \
      pred_ptrs[0] = &preds[0][y * strides[0]]; \
      pred_ptrs[1] = &preds[1][y * strides[1]]; \
      pred_ptrs[2] = &preds[2][y * strides[2]]; \
      pred_ptrs[3] = &preds[3][y * strides[3]]; \
      for (int x = 0; x < width; x += 8) { \
        satd_8x8_subblock_ ## suffix(pred_ptrs, strides, orig_ptr, orig_stride, sums); \
        orig_ptr += 8; \
        pred_ptrs[0] += 8; \
        pred_ptrs[1] += 8; \
        pred_ptrs[2] += 8; \
        pred_ptrs[3] += 8; \
        costs_out[0] += sums[0]; \
        costs_out[1] += sums[1]; \
        costs_out[2] += sums[2]; \
        costs_out[3] += sums[3]; \
      } \
    } \
    for(int i = 0; i < num_parallel_blocks; ++i){\
      costs_out[i] = costs_out[i] >> (KVZ_BIT_DEPTH - 8);\
    } \
    return; \
  }

SATD_ANY_SIZE_MULTI_AVX2(quad_avx2, 4)


static unsigned pixels_calc_ssd_avx2(const kvz_pixel *const ref, const kvz_pixel *const rec,
                 const int ref_stride, const int rec_stride,
                 const int width)
{
  __m256i ssd_part;
  __m256i diff = _mm256_setzero_si256();
  __m128i sum;

  __m256i ref_epi16;
  __m256i rec_epi16;

  __m128i ref_row0, ref_row1, ref_row2, ref_row3;
  __m128i rec_row0, rec_row1, rec_row2, rec_row3;

  int ssd;

  switch (width) {

  case 4:

    ref_row0 = _mm_cvtsi32_si128(*(int32_t*)&(ref[0 * ref_stride]));
    ref_row1 = _mm_cvtsi32_si128(*(int32_t*)&(ref[1 * ref_stride]));
    ref_row2 = _mm_cvtsi32_si128(*(int32_t*)&(ref[2 * ref_stride]));
    ref_row3 = _mm_cvtsi32_si128(*(int32_t*)&(ref[3 * ref_stride]));

    ref_row0 = _mm_unpacklo_epi32(ref_row0, ref_row1);
    ref_row1 = _mm_unpacklo_epi32(ref_row2, ref_row3);
    ref_epi16 = _mm256_cvtepu8_epi16(_mm_unpacklo_epi64(ref_row0, ref_row1) );

    rec_row0 = _mm_cvtsi32_si128(*(int32_t*)&(rec[0 * rec_stride]));
    rec_row1 = _mm_cvtsi32_si128(*(int32_t*)&(rec[1 * rec_stride]));
    rec_row2 = _mm_cvtsi32_si128(*(int32_t*)&(rec[2 * rec_stride]));
    rec_row3 = _mm_cvtsi32_si128(*(int32_t*)&(rec[3 * rec_stride]));

    rec_row0 = _mm_unpacklo_epi32(rec_row0, rec_row1);
    rec_row1 = _mm_unpacklo_epi32(rec_row2, rec_row3);
    rec_epi16 = _mm256_cvtepu8_epi16(_mm_unpacklo_epi64(rec_row0, rec_row1) );

    diff = _mm256_sub_epi16(ref_epi16, rec_epi16);
    ssd_part =  _mm256_madd_epi16(diff, diff);

    sum = _mm_add_epi32(_mm256_castsi256_si128(ssd_part), _mm256_extracti128_si256(ssd_part, 1));
    sum = _mm_add_epi32(sum, _mm_shuffle_epi32(sum, _MM_SHUFFLE(1, 0, 3, 2)));
    sum = _mm_add_epi32(sum, _mm_shuffle_epi32(sum, _MM_SHUFFLE(0, 1, 0, 1)));

    ssd = _mm_cvtsi128_si32(sum);

    return ssd >> (2*(KVZ_BIT_DEPTH-8));
    break;

  default:

    ssd_part = _mm256_setzero_si256();
    for (int y = 0; y < width; y += 8) {
      for (int x = 0; x < width; x += 8) {
        for (int i = 0; i < 8; i += 2) {
          ref_epi16 = _mm256_cvtepu8_epi16(_mm_unpacklo_epi64(_mm_loadl_epi64((__m128i*)&(ref[x + (y + i) * ref_stride])), _mm_loadl_epi64((__m128i*)&(ref[x + (y + i + 1) * ref_stride]))));
          rec_epi16 = _mm256_cvtepu8_epi16(_mm_unpacklo_epi64(_mm_loadl_epi64((__m128i*)&(rec[x + (y + i) * rec_stride])), _mm_loadl_epi64((__m128i*)&(rec[x + (y + i + 1) * rec_stride]))));
          diff = _mm256_sub_epi16(ref_epi16, rec_epi16);
          ssd_part = _mm256_add_epi32(ssd_part, _mm256_madd_epi16(diff, diff));
        }
      }
    }

    sum = _mm_add_epi32(_mm256_castsi256_si128(ssd_part), _mm256_extracti128_si256(ssd_part, 1));
    sum = _mm_add_epi32(sum, _mm_shuffle_epi32(sum, _MM_SHUFFLE(1, 0, 3, 2)));
    sum = _mm_add_epi32(sum, _mm_shuffle_epi32(sum, _MM_SHUFFLE(0, 1, 0, 1)));

    ssd = _mm_cvtsi128_si32(sum);

    return ssd >> (2*(KVZ_BIT_DEPTH-8));
    break;
  }
}

#endif //COMPILE_INTEL_AVX2


int kvz_strategy_register_picture_avx2(void* opaque, uint8_t bitdepth)
{
  bool success = true;
#if COMPILE_INTEL_AVX2
  // We don't actually use SAD for intra right now, other than 4x4 for
  // transform skip, but we might again one day and this is some of the
  // simplest code to look at for anyone interested in doing more
  // optimizations, so it's worth it to keep this maintained.
  if (bitdepth == 8){
    success &= kvz_strategyselector_register(opaque, "sad_8x8", "avx2", 40, &sad_8bit_8x8_avx2);
    success &= kvz_strategyselector_register(opaque, "sad_16x16", "avx2", 40, &sad_8bit_16x16_avx2);
    success &= kvz_strategyselector_register(opaque, "sad_32x32", "avx2", 40, &sad_8bit_32x32_avx2);
    success &= kvz_strategyselector_register(opaque, "sad_64x64", "avx2", 40, &sad_8bit_64x64_avx2);

    success &= kvz_strategyselector_register(opaque, "satd_4x4", "avx2", 40, &satd_4x4_8bit_avx2);
    success &= kvz_strategyselector_register(opaque, "satd_8x8", "avx2", 40, &satd_8x8_8bit_avx2);
    success &= kvz_strategyselector_register(opaque, "satd_16x16", "avx2", 40, &satd_16x16_8bit_avx2);
    success &= kvz_strategyselector_register(opaque, "satd_32x32", "avx2", 40, &satd_32x32_8bit_avx2);
    success &= kvz_strategyselector_register(opaque, "satd_64x64", "avx2", 40, &satd_64x64_8bit_avx2);

    success &= kvz_strategyselector_register(opaque, "satd_4x4_dual", "avx2", 40, &satd_8bit_4x4_dual_avx2);
    success &= kvz_strategyselector_register(opaque, "satd_8x8_dual", "avx2", 40, &satd_8bit_8x8_dual_avx2);
    success &= kvz_strategyselector_register(opaque, "satd_16x16_dual", "avx2", 40, &satd_8bit_16x16_dual_avx2);
    success &= kvz_strategyselector_register(opaque, "satd_32x32_dual", "avx2", 40, &satd_8bit_32x32_dual_avx2);
    success &= kvz_strategyselector_register(opaque, "satd_64x64_dual", "avx2", 40, &satd_8bit_64x64_dual_avx2);
    success &= kvz_strategyselector_register(opaque, "satd_any_size", "avx2", 40, &satd_any_size_8bit_avx2);
    success &= kvz_strategyselector_register(opaque, "satd_any_size_quad", "avx2", 40, &satd_any_size_quad_avx2);

    success &= kvz_strategyselector_register(opaque, "pixels_calc_ssd", "avx2", 40, &pixels_calc_ssd_avx2);
  }
#endif
  return success;
}
