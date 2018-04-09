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

#include "strategies/avx2/dct-avx2.h"

#if COMPILE_INTEL_AVX2
#include <immintrin.h>

#include "strategyselector.h"
#include "tables.h"

extern const int16_t kvz_g_dst_4[4][4];
extern const int16_t kvz_g_dct_4[4][4];
extern const int16_t kvz_g_dct_8[8][8];
extern const int16_t kvz_g_dct_16[16][16];
extern const int16_t kvz_g_dct_32[32][32];

extern const int16_t kvz_g_dst_4_t[4][4];
extern const int16_t kvz_g_dct_4_t[4][4];
extern const int16_t kvz_g_dct_8_t[8][8];
extern const int16_t kvz_g_dct_16_t[16][16];
extern const int16_t kvz_g_dct_32_t[32][32];

/*
* \file
* \brief AVX2 transformations.
*/

// 4x4 matrix multiplication with value clipping.
// Parameters: Two 4x4 matrices containing 16-bit values in consecutive addresses,
//             destination for the result and the shift value for clipping.
static void mul_clip_matrix_4x4_avx2(const int16_t *left, const int16_t *right, int16_t *dst, int32_t shift)
{
  __m256i b[2], a, result, even[2], odd[2];

  const int32_t add = 1 << (shift - 1);

  a = _mm256_loadu_si256((__m256i*) left);
  b[0] = _mm256_loadu_si256((__m256i*) right);

  // Interleave values in both 128-bit lanes
  b[0] = _mm256_unpacklo_epi16(b[0], _mm256_srli_si256(b[0], 8));
  b[1] = _mm256_permute2x128_si256(b[0], b[0], 1 + 16);
  b[0] = _mm256_permute2x128_si256(b[0], b[0], 0);

  // Fill both 128-lanes with the first pair of 16-bit factors in the lane.
  even[0] = _mm256_shuffle_epi32(a, 0);
  odd[0] = _mm256_shuffle_epi32(a, 1 + 4 + 16 + 64);

  // Multiply packed elements and sum pairs. Input 16-bit output 32-bit.
  even[0] = _mm256_madd_epi16(even[0], b[0]);
  odd[0] = _mm256_madd_epi16(odd[0], b[1]);

  // Add the halves of the dot product and
  // round.
  result = _mm256_add_epi32(even[0], odd[0]);
  result = _mm256_add_epi32(result, _mm256_set1_epi32(add));
  result = _mm256_srai_epi32(result, shift);

  //Repeat for the remaining parts
  even[1] = _mm256_shuffle_epi32(a, 2 + 8 + 32 + 128);
  odd[1] = _mm256_shuffle_epi32(a, 3 + 12 + 48 + 192);

  even[1] = _mm256_madd_epi16(even[1], b[0]);
  odd[1] = _mm256_madd_epi16(odd[1], b[1]);

  odd[1] = _mm256_add_epi32(even[1], odd[1]);
  odd[1] = _mm256_add_epi32(odd[1], _mm256_set1_epi32(add));
  odd[1] = _mm256_srai_epi32(odd[1], shift);

  // Truncate to 16-bit values
  result = _mm256_packs_epi32(result, odd[1]);

  _mm256_storeu_si256((__m256i*)dst, result);
}

// 8x8 matrix multiplication with value clipping.
// Parameters: Two 8x8 matrices containing 16-bit values in consecutive addresses,
//             destination for the result and the shift value for clipping.
//
static void mul_clip_matrix_8x8_avx2(const int16_t *left, const int16_t *right, int16_t *dst, const int32_t shift)
{
  int i, j;
  __m256i b[2], accu[8], even[2], odd[2];

  const int32_t add = 1 << (shift - 1);

  b[0] = _mm256_loadu_si256((__m256i*) right);

  b[1] = _mm256_unpackhi_epi16(b[0], _mm256_castsi128_si256(_mm256_extracti128_si256(b[0], 1)));
  b[0] = _mm256_unpacklo_epi16(b[0], _mm256_castsi128_si256(_mm256_extracti128_si256(b[0], 1)));
  b[0] = _mm256_inserti128_si256(b[0], _mm256_castsi256_si128(b[1]), 1);

  for (i = 0; i < 8; i += 2) {

    even[0] = _mm256_set1_epi32(((int32_t*)left)[4 * i]);
    even[0] = _mm256_madd_epi16(even[0], b[0]);
    accu[i] = even[0];

    odd[0] = _mm256_set1_epi32(((int32_t*)left)[4 * (i + 1)]);
    odd[0] = _mm256_madd_epi16(odd[0], b[0]);
    accu[i + 1] = odd[0];
  }

  for (j = 1; j < 4; ++j) {

    b[0] = _mm256_loadu_si256((__m256i*)right + j);

    b[1] = _mm256_unpackhi_epi16(b[0], _mm256_castsi128_si256(_mm256_extracti128_si256(b[0], 1)));
    b[0] = _mm256_unpacklo_epi16(b[0], _mm256_castsi128_si256(_mm256_extracti128_si256(b[0], 1)));
    b[0] = _mm256_inserti128_si256(b[0], _mm256_castsi256_si128(b[1]), 1);

    for (i = 0; i < 8; i += 2) {

      even[0] = _mm256_set1_epi32(((int32_t*)left)[4 * i + j]);
      even[0] = _mm256_madd_epi16(even[0], b[0]);
      accu[i] = _mm256_add_epi32(accu[i], even[0]);

      odd[0] = _mm256_set1_epi32(((int32_t*)left)[4 * (i + 1) + j]);
      odd[0] = _mm256_madd_epi16(odd[0], b[0]);
      accu[i + 1] = _mm256_add_epi32(accu[i + 1], odd[0]);
    }
  }

  for (i = 0; i < 8; i += 2) {
    __m256i result, first_half, second_half;

    first_half = _mm256_srai_epi32(_mm256_add_epi32(accu[i], _mm256_set1_epi32(add)), shift);
    second_half = _mm256_srai_epi32(_mm256_add_epi32(accu[i + 1], _mm256_set1_epi32(add)), shift);
    result = _mm256_permute4x64_epi64(_mm256_packs_epi32(first_half, second_half), 0 + 8 + 16 + 192);
    _mm256_storeu_si256((__m256i*)dst + i / 2, result);

  }
}

// 16x16 matrix multiplication with value clipping.
// Parameters: Two 16x16 matrices containing 16-bit values in consecutive addresses,
//             destination for the result and the shift value for clipping.
static void mul_clip_matrix_16x16_avx2(const int16_t *left, const int16_t *right, int16_t *dst, const int32_t shift)
{
  int i, j;
  __m256i row[4], accu[16][2], even, odd;

  const int32_t stride = 8;

  const int32_t add = 1 << (shift - 1);

  row[0] = _mm256_loadu_si256((__m256i*) right);
  row[1] = _mm256_loadu_si256((__m256i*) right + 1);
  row[2] = _mm256_unpacklo_epi16(row[0], row[1]);
  row[3] = _mm256_unpackhi_epi16(row[0], row[1]);
  row[0] = _mm256_permute2x128_si256(row[2], row[3], 0 + 32);
  row[1] = _mm256_permute2x128_si256(row[2], row[3], 1 + 48);

  for (i = 0; i < 16; i += 2) {

    even = _mm256_set1_epi32(((int32_t*)left)[stride * i]);
    accu[i][0] = _mm256_madd_epi16(even, row[0]);
    accu[i][1] = _mm256_madd_epi16(even, row[1]);

    odd = _mm256_set1_epi32(((int32_t*)left)[stride * (i + 1)]);
    accu[i + 1][0] = _mm256_madd_epi16(odd, row[0]);
    accu[i + 1][1] = _mm256_madd_epi16(odd, row[1]);
  }

  for (j = 2; j < 16; j += 2) {

    row[0] = _mm256_loadu_si256((__m256i*)right + j);
    row[1] = _mm256_loadu_si256((__m256i*)right + j + 1);
    row[2] = _mm256_unpacklo_epi16(row[0], row[1]);
    row[3] = _mm256_unpackhi_epi16(row[0], row[1]);
    row[0] = _mm256_permute2x128_si256(row[2], row[3], 0 + 32);
    row[1] = _mm256_permute2x128_si256(row[2], row[3], 1 + 48);

    for (i = 0; i < 16; i += 2) {

      even = _mm256_set1_epi32(((int32_t*)left)[stride * i + j / 2]);
      accu[i][0] = _mm256_add_epi32(accu[i][0], _mm256_madd_epi16(even, row[0]));
      accu[i][1] = _mm256_add_epi32(accu[i][1], _mm256_madd_epi16(even, row[1]));

      odd = _mm256_set1_epi32(((int32_t*)left)[stride * (i + 1) + j / 2]);
      accu[i + 1][0] = _mm256_add_epi32(accu[i + 1][0], _mm256_madd_epi16(odd, row[0]));
      accu[i + 1][1] = _mm256_add_epi32(accu[i + 1][1], _mm256_madd_epi16(odd, row[1]));

    }
  }

  for (i = 0; i < 16; ++i) {
    __m256i result, first_half, second_half;

    first_half = _mm256_srai_epi32(_mm256_add_epi32(accu[i][0], _mm256_set1_epi32(add)), shift);
    second_half = _mm256_srai_epi32(_mm256_add_epi32(accu[i][1], _mm256_set1_epi32(add)), shift);
    result = _mm256_permute4x64_epi64(_mm256_packs_epi32(first_half, second_half), 0 + 8 + 16 + 192);
    _mm256_storeu_si256((__m256i*)dst + i, result);

  }
}

// 32x32 matrix multiplication with value clipping.
// Parameters: Two 32x32 matrices containing 16-bit values in consecutive addresses,
//             destination for the result and the shift value for clipping.
static void mul_clip_matrix_32x32_avx2(const int16_t *left, const int16_t *right, int16_t *dst, const int32_t shift)
{
  int i, j;
  __m256i row[4], tmp[2], accu[32][4], even, odd;

  const int32_t stride = 16;

  const int32_t add = 1 << (shift - 1);

  row[0] = _mm256_loadu_si256((__m256i*) right);
  row[1] = _mm256_loadu_si256((__m256i*) right + 2);
  tmp[0] = _mm256_unpacklo_epi16(row[0], row[1]);
  tmp[1] = _mm256_unpackhi_epi16(row[0], row[1]);
  row[0] = _mm256_permute2x128_si256(tmp[0], tmp[1], 0 + 32);
  row[1] = _mm256_permute2x128_si256(tmp[0], tmp[1], 1 + 48);

  row[2] = _mm256_loadu_si256((__m256i*) right + 1);
  row[3] = _mm256_loadu_si256((__m256i*) right + 3);
  tmp[0] = _mm256_unpacklo_epi16(row[2], row[3]);
  tmp[1] = _mm256_unpackhi_epi16(row[2], row[3]);
  row[2] = _mm256_permute2x128_si256(tmp[0], tmp[1], 0 + 32);
  row[3] = _mm256_permute2x128_si256(tmp[0], tmp[1], 1 + 48);

  for (i = 0; i < 32; i += 2) {

    even = _mm256_set1_epi32(((int32_t*)left)[stride * i]);
    accu[i][0] = _mm256_madd_epi16(even, row[0]);
    accu[i][1] = _mm256_madd_epi16(even, row[1]);
    accu[i][2] = _mm256_madd_epi16(even, row[2]);
    accu[i][3] = _mm256_madd_epi16(even, row[3]);

    odd = _mm256_set1_epi32(((int32_t*)left)[stride * (i + 1)]);
    accu[i + 1][0] = _mm256_madd_epi16(odd, row[0]);
    accu[i + 1][1] = _mm256_madd_epi16(odd, row[1]);
    accu[i + 1][2] = _mm256_madd_epi16(odd, row[2]);
    accu[i + 1][3] = _mm256_madd_epi16(odd, row[3]);
  }

  for (j = 4; j < 64; j += 4) {

    row[0] = _mm256_loadu_si256((__m256i*)right + j);
    row[1] = _mm256_loadu_si256((__m256i*)right + j + 2);
    tmp[0] = _mm256_unpacklo_epi16(row[0], row[1]);
    tmp[1] = _mm256_unpackhi_epi16(row[0], row[1]);
    row[0] = _mm256_permute2x128_si256(tmp[0], tmp[1], 0 + 32);
    row[1] = _mm256_permute2x128_si256(tmp[0], tmp[1], 1 + 48);

    row[2] = _mm256_loadu_si256((__m256i*) right + j + 1);
    row[3] = _mm256_loadu_si256((__m256i*) right + j + 3);
    tmp[0] = _mm256_unpacklo_epi16(row[2], row[3]);
    tmp[1] = _mm256_unpackhi_epi16(row[2], row[3]);
    row[2] = _mm256_permute2x128_si256(tmp[0], tmp[1], 0 + 32);
    row[3] = _mm256_permute2x128_si256(tmp[0], tmp[1], 1 + 48);

    for (i = 0; i < 32; i += 2) {

      even = _mm256_set1_epi32(((int32_t*)left)[stride * i + j / 4]);
      accu[i][0] = _mm256_add_epi32(accu[i][0], _mm256_madd_epi16(even, row[0]));
      accu[i][1] = _mm256_add_epi32(accu[i][1], _mm256_madd_epi16(even, row[1]));
      accu[i][2] = _mm256_add_epi32(accu[i][2], _mm256_madd_epi16(even, row[2]));
      accu[i][3] = _mm256_add_epi32(accu[i][3], _mm256_madd_epi16(even, row[3]));

      odd = _mm256_set1_epi32(((int32_t*)left)[stride * (i + 1) + j / 4]);
      accu[i + 1][0] = _mm256_add_epi32(accu[i + 1][0], _mm256_madd_epi16(odd, row[0]));
      accu[i + 1][1] = _mm256_add_epi32(accu[i + 1][1], _mm256_madd_epi16(odd, row[1]));
      accu[i + 1][2] = _mm256_add_epi32(accu[i + 1][2], _mm256_madd_epi16(odd, row[2]));
      accu[i + 1][3] = _mm256_add_epi32(accu[i + 1][3], _mm256_madd_epi16(odd, row[3]));

    }
  }

  for (i = 0; i < 32; ++i) {
    __m256i result, first_quarter, second_quarter, third_quarter, fourth_quarter;

    first_quarter = _mm256_srai_epi32(_mm256_add_epi32(accu[i][0], _mm256_set1_epi32(add)), shift);
    second_quarter = _mm256_srai_epi32(_mm256_add_epi32(accu[i][1], _mm256_set1_epi32(add)), shift);
    third_quarter = _mm256_srai_epi32(_mm256_add_epi32(accu[i][2], _mm256_set1_epi32(add)), shift);
    fourth_quarter = _mm256_srai_epi32(_mm256_add_epi32(accu[i][3], _mm256_set1_epi32(add)), shift);
    result = _mm256_permute4x64_epi64(_mm256_packs_epi32(first_quarter, second_quarter), 0 + 8 + 16 + 192);
    _mm256_storeu_si256((__m256i*)dst + 2 * i, result);
    result = _mm256_permute4x64_epi64(_mm256_packs_epi32(third_quarter, fourth_quarter), 0 + 8 + 16 + 192);
    _mm256_storeu_si256((__m256i*)dst + 2 * i + 1, result);

  }
}

// Macro that generates 2D transform functions with clipping values.
// Sets correct shift values and matrices according to transform type and
// block size. Performs matrix multiplication horizontally and vertically.
#define TRANSFORM(type, n) static void matrix_ ## type ## _ ## n ## x ## n ## _avx2(int8_t bitdepth, const int16_t *input, int16_t *output)\
{\
  int32_t shift_1st = kvz_g_convert_to_bit[n] + 1 + (bitdepth - 8); \
  int32_t shift_2nd = kvz_g_convert_to_bit[n] + 8; \
  int16_t tmp[n * n];\
  const int16_t *tdct = &kvz_g_ ## type ## _ ## n ## _t[0][0];\
  const int16_t *dct = &kvz_g_ ## type ## _ ## n [0][0];\
\
  mul_clip_matrix_ ## n ## x ## n ## _avx2(input, tdct, tmp, shift_1st);\
  mul_clip_matrix_ ## n ## x ## n ## _avx2(dct, tmp, output, shift_2nd);\
}\

// Macro that generates 2D inverse transform functions with clipping values.
// Sets correct shift values and matrices according to transform type and
// block size. Performs matrix multiplication horizontally and vertically.
#define ITRANSFORM(type, n) \
static void matrix_i ## type ## _## n ## x ## n ## _avx2(int8_t bitdepth, const int16_t *input, int16_t *output)\
{\
  int32_t shift_1st = 7; \
  int32_t shift_2nd = 12 - (bitdepth - 8); \
  int16_t tmp[n * n];\
  const int16_t *tdct = &kvz_g_ ## type ## _ ## n ## _t[0][0];\
  const int16_t *dct = &kvz_g_ ## type ## _ ## n [0][0];\
\
  mul_clip_matrix_ ## n ## x ## n ## _avx2(tdct, input, tmp, shift_1st);\
  mul_clip_matrix_ ## n ## x ## n ## _avx2(tmp, dct, output, shift_2nd);\
}\

// Generate all the transform functions
TRANSFORM(dst, 4);
TRANSFORM(dct, 4);
TRANSFORM(dct, 8);
TRANSFORM(dct, 16);
TRANSFORM(dct, 32);

ITRANSFORM(dst, 4);
ITRANSFORM(dct, 4);
ITRANSFORM(dct, 8);
ITRANSFORM(dct, 16);
ITRANSFORM(dct, 32);

#endif //COMPILE_INTEL_AVX2

int kvz_strategy_register_dct_avx2(void* opaque, uint8_t bitdepth)
{
  bool success = true;
#if COMPILE_INTEL_AVX2
  if (bitdepth == 8){
    success &= kvz_strategyselector_register(opaque, "fast_forward_dst_4x4", "avx2", 40, &matrix_dst_4x4_avx2);

    success &= kvz_strategyselector_register(opaque, "dct_4x4", "avx2", 40, &matrix_dct_4x4_avx2);
    success &= kvz_strategyselector_register(opaque, "dct_8x8", "avx2", 40, &matrix_dct_8x8_avx2);
    success &= kvz_strategyselector_register(opaque, "dct_16x16", "avx2", 40, &matrix_dct_16x16_avx2);
    success &= kvz_strategyselector_register(opaque, "dct_32x32", "avx2", 40, &matrix_dct_32x32_avx2);

    success &= kvz_strategyselector_register(opaque, "fast_inverse_dst_4x4", "avx2", 40, &matrix_idst_4x4_avx2);

    success &= kvz_strategyselector_register(opaque, "idct_4x4", "avx2", 40, &matrix_idct_4x4_avx2);
    success &= kvz_strategyselector_register(opaque, "idct_8x8", "avx2", 40, &matrix_idct_8x8_avx2);
    success &= kvz_strategyselector_register(opaque, "idct_16x16", "avx2", 40, &matrix_idct_16x16_avx2);
    success &= kvz_strategyselector_register(opaque, "idct_32x32", "avx2", 40, &matrix_idct_32x32_avx2);
  }
#endif //COMPILE_INTEL_AVX2  
  return success;
}
