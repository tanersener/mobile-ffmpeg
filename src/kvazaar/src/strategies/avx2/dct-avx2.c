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

static INLINE __m256i swap_lanes(__m256i v)
{
  return _mm256_permute4x64_epi64(v, _MM_SHUFFLE(1, 0, 3, 2));
}

static INLINE __m256i truncate(__m256i v, __m256i debias, int32_t shift)
{
  __m256i truncable = _mm256_add_epi32 (v,         debias);
  return              _mm256_srai_epi32(truncable, shift);
}

// 4x4 matrix multiplication with value clipping.
// Parameters: Two 4x4 matrices containing 16-bit values in consecutive addresses,
//             destination for the result and the shift value for clipping.
static __m256i mul_clip_matrix_4x4_avx2(const __m256i left, const __m256i right, int shift)
{
  const int32_t add    = 1 << (shift - 1);
  const __m256i debias = _mm256_set1_epi32(add);

  __m256i right_los = _mm256_permute4x64_epi64(right, _MM_SHUFFLE(2, 0, 2, 0));
  __m256i right_his = _mm256_permute4x64_epi64(right, _MM_SHUFFLE(3, 1, 3, 1));

  __m256i right_cols_up = _mm256_unpacklo_epi16(right_los, right_his);
  __m256i right_cols_dn = _mm256_unpackhi_epi16(right_los, right_his);

  __m256i left_slice1 = _mm256_shuffle_epi32(left, _MM_SHUFFLE(0, 0, 0, 0));
  __m256i left_slice2 = _mm256_shuffle_epi32(left, _MM_SHUFFLE(1, 1, 1, 1));
  __m256i left_slice3 = _mm256_shuffle_epi32(left, _MM_SHUFFLE(2, 2, 2, 2));
  __m256i left_slice4 = _mm256_shuffle_epi32(left, _MM_SHUFFLE(3, 3, 3, 3));

  __m256i prod1 = _mm256_madd_epi16(left_slice1, right_cols_up);
  __m256i prod2 = _mm256_madd_epi16(left_slice2, right_cols_dn);
  __m256i prod3 = _mm256_madd_epi16(left_slice3, right_cols_up);
  __m256i prod4 = _mm256_madd_epi16(left_slice4, right_cols_dn);

  __m256i rows_up = _mm256_add_epi32(prod1, prod2);
  __m256i rows_dn = _mm256_add_epi32(prod3, prod4);

  __m256i rows_up_tr = truncate(rows_up, debias, shift);
  __m256i rows_dn_tr = truncate(rows_dn, debias, shift);

  __m256i result = _mm256_packs_epi32(rows_up_tr, rows_dn_tr);
  return result;
}

static void matrix_dst_4x4_avx2(int8_t bitdepth, const int16_t *input, int16_t *output)
{
  int32_t shift_1st = kvz_g_convert_to_bit[4] + 1 + (bitdepth - 8);
  int32_t shift_2nd = kvz_g_convert_to_bit[4] + 8;
  const int16_t *tdst = &kvz_g_dst_4_t[0][0];
  const int16_t *dst  = &kvz_g_dst_4  [0][0];

  __m256i tdst_v = _mm256_load_si256((const __m256i *) tdst);
  __m256i  dst_v = _mm256_load_si256((const __m256i *)  dst);
  __m256i   in_v = _mm256_load_si256((const __m256i *)input);

  __m256i tmp    = mul_clip_matrix_4x4_avx2(in_v,  tdst_v, shift_1st);
  __m256i result = mul_clip_matrix_4x4_avx2(dst_v, tmp,    shift_2nd);

  _mm256_store_si256((__m256i *)output, result);
}

static void matrix_idst_4x4_avx2(int8_t bitdepth, const int16_t *input, int16_t *output)
{
  int32_t shift_1st = 7;
  int32_t shift_2nd = 12 - (bitdepth - 8);

  const int16_t *tdst = &kvz_g_dst_4_t[0][0];
  const int16_t *dst  = &kvz_g_dst_4  [0][0];

  __m256i tdst_v = _mm256_load_si256((const __m256i *)tdst);
  __m256i  dst_v = _mm256_load_si256((const __m256i *) dst);
  __m256i   in_v = _mm256_load_si256((const __m256i *)input);

  __m256i tmp    = mul_clip_matrix_4x4_avx2(tdst_v, in_v,  shift_1st);
  __m256i result = mul_clip_matrix_4x4_avx2(tmp,    dst_v, shift_2nd);

  _mm256_store_si256((__m256i *)output, result);
}

static void matrix_dct_4x4_avx2(int8_t bitdepth, const int16_t *input, int16_t *output)
{
  int32_t shift_1st = kvz_g_convert_to_bit[4] + 1 + (bitdepth - 8);
  int32_t shift_2nd = kvz_g_convert_to_bit[4] + 8;
  const int16_t *tdct = &kvz_g_dct_4_t[0][0];
  const int16_t *dct  = &kvz_g_dct_4  [0][0];

  __m256i tdct_v = _mm256_load_si256((const __m256i *) tdct);
  __m256i  dct_v = _mm256_load_si256((const __m256i *)  dct);
  __m256i   in_v = _mm256_load_si256((const __m256i *)input);

  __m256i tmp    = mul_clip_matrix_4x4_avx2(in_v,  tdct_v, shift_1st);
  __m256i result = mul_clip_matrix_4x4_avx2(dct_v, tmp,    shift_2nd);

  _mm256_store_si256((__m256i *)output, result);
}

static void matrix_idct_4x4_avx2(int8_t bitdepth, const int16_t *input, int16_t *output)
{
  int32_t shift_1st = 7;
  int32_t shift_2nd = 12 - (bitdepth - 8);

  const int16_t *tdct = &kvz_g_dct_4_t[0][0];
  const int16_t *dct  = &kvz_g_dct_4  [0][0];

  __m256i tdct_v = _mm256_load_si256((const __m256i *)tdct);
  __m256i  dct_v = _mm256_load_si256((const __m256i *) dct);
  __m256i   in_v = _mm256_load_si256((const __m256i *)input);

  __m256i tmp    = mul_clip_matrix_4x4_avx2(tdct_v, in_v,  shift_1st);
  __m256i result = mul_clip_matrix_4x4_avx2(tmp,    dct_v, shift_2nd);

  _mm256_store_si256((__m256i *)output, result);
}

static void mul_clip_matrix_8x8_avx2(const int16_t *left, const int16_t *right, int16_t *dst, const int32_t shift)
{
  const __m256i transp_mask = _mm256_broadcastsi128_si256(_mm_setr_epi8(0, 1, 8, 9, 2, 3, 10, 11, 4, 5, 12, 13, 6, 7, 14, 15));

  const int32_t add    = 1 << (shift - 1);
  const __m256i debias = _mm256_set1_epi32(add);

  __m256i left_dr[4] = {
    _mm256_load_si256((const __m256i *)left + 0),
    _mm256_load_si256((const __m256i *)left + 1),
    _mm256_load_si256((const __m256i *)left + 2),
    _mm256_load_si256((const __m256i *)left + 3),
  };
  __m256i right_dr[4] = {
    _mm256_load_si256((const __m256i *)right + 0),
    _mm256_load_si256((const __m256i *)right + 1),
    _mm256_load_si256((const __m256i *)right + 2),
    _mm256_load_si256((const __m256i *)right + 3),
  };

  __m256i rdrs_rearr[8];

  // Rearrange right matrix
  for (int32_t dry = 0; dry < 4; dry++) {
    __m256i rdr = right_dr[dry];
    __m256i rdr_los = _mm256_permute4x64_epi64(rdr, _MM_SHUFFLE(2, 0, 2, 0));
    __m256i rdr_his = _mm256_permute4x64_epi64(rdr, _MM_SHUFFLE(3, 1, 3, 1));

    __m256i rdr_lo_rearr = _mm256_shuffle_epi8(rdr_los, transp_mask);
    __m256i rdr_hi_rearr = _mm256_shuffle_epi8(rdr_his, transp_mask);

    rdrs_rearr[dry * 2 + 0] = rdr_lo_rearr;
    rdrs_rearr[dry * 2 + 1] = rdr_hi_rearr;
  }

  // Double-Row Y for destination matrix
  for (int32_t dry = 0; dry < 4; dry++) {
    __m256i ldr = left_dr[dry];

    __m256i ldr_slice12 = _mm256_shuffle_epi32(ldr, _MM_SHUFFLE(0, 0, 0, 0));
    __m256i ldr_slice34 = _mm256_shuffle_epi32(ldr, _MM_SHUFFLE(1, 1, 1, 1));
    __m256i ldr_slice56 = _mm256_shuffle_epi32(ldr, _MM_SHUFFLE(2, 2, 2, 2));
    __m256i ldr_slice78 = _mm256_shuffle_epi32(ldr, _MM_SHUFFLE(3, 3, 3, 3));

    __m256i prod1 = _mm256_madd_epi16(ldr_slice12, rdrs_rearr[0]);
    __m256i prod2 = _mm256_madd_epi16(ldr_slice12, rdrs_rearr[1]);
    __m256i prod3 = _mm256_madd_epi16(ldr_slice34, rdrs_rearr[2]);
    __m256i prod4 = _mm256_madd_epi16(ldr_slice34, rdrs_rearr[3]);
    __m256i prod5 = _mm256_madd_epi16(ldr_slice56, rdrs_rearr[4]);
    __m256i prod6 = _mm256_madd_epi16(ldr_slice56, rdrs_rearr[5]);
    __m256i prod7 = _mm256_madd_epi16(ldr_slice78, rdrs_rearr[6]);
    __m256i prod8 = _mm256_madd_epi16(ldr_slice78, rdrs_rearr[7]);

    __m256i lo_1 = _mm256_add_epi32(prod1, prod3);
    __m256i hi_1 = _mm256_add_epi32(prod2, prod4);
    __m256i lo_2 = _mm256_add_epi32(prod5, prod7);
    __m256i hi_2 = _mm256_add_epi32(prod6, prod8);

    __m256i lo   = _mm256_add_epi32(lo_1,  lo_2);
    __m256i hi   = _mm256_add_epi32(hi_1,  hi_2);

    __m256i lo_tr = truncate(lo, debias, shift);
    __m256i hi_tr = truncate(hi, debias, shift);

    __m256i final_dr = _mm256_packs_epi32(lo_tr, hi_tr);

    _mm256_store_si256((__m256i *)dst + dry, final_dr);
  }
}

// Multiplies A by B_T's transpose and stores result's transpose in output,
// which should be an array of 4 __m256i's
static void matmul_8x8_a_bt_t(const int16_t *a, const int16_t *b_t,
    __m256i *output, const int8_t shift)
{
  const int32_t add    = 1 << (shift - 1);
  const __m256i debias = _mm256_set1_epi32(add);

  // Keep upper row intact and swap neighboring 16-bit words in lower row
  const __m256i shuf_lorow_mask =
      _mm256_setr_epi8(0,  1,  2,  3,  4,  5,  6,  7,
                       8,  9,  10, 11, 12, 13, 14, 15,
                       18, 19, 16, 17, 22, 23, 20, 21,
                       26, 27, 24, 25, 30, 31, 28, 29);

  const __m256i *b_t_256 = (const __m256i *)b_t;

  // Dual Rows, because two 8x16b words fit in one YMM
  __m256i a_dr_0      = _mm256_load_si256((__m256i *)a + 0);
  __m256i a_dr_1      = _mm256_load_si256((__m256i *)a + 1);
  __m256i a_dr_2      = _mm256_load_si256((__m256i *)a + 2);
  __m256i a_dr_3      = _mm256_load_si256((__m256i *)a + 3);

  __m256i a_dr_0_swp  = swap_lanes(a_dr_0);
  __m256i a_dr_1_swp  = swap_lanes(a_dr_1);
  __m256i a_dr_2_swp  = swap_lanes(a_dr_2);
  __m256i a_dr_3_swp  = swap_lanes(a_dr_3);

  for (int dry = 0; dry < 4; dry++) {

    // Read dual columns of B matrix by reading rows of its transpose
    __m256i b_dc        = _mm256_load_si256(b_t_256 + dry);

    __m256i prod0       = _mm256_madd_epi16(b_dc,     a_dr_0);
    __m256i prod0_swp   = _mm256_madd_epi16(b_dc,     a_dr_0_swp);
    __m256i prod1       = _mm256_madd_epi16(b_dc,     a_dr_1);
    __m256i prod1_swp   = _mm256_madd_epi16(b_dc,     a_dr_1_swp);
    __m256i prod2       = _mm256_madd_epi16(b_dc,     a_dr_2);
    __m256i prod2_swp   = _mm256_madd_epi16(b_dc,     a_dr_2_swp);
    __m256i prod3       = _mm256_madd_epi16(b_dc,     a_dr_3);
    __m256i prod3_swp   = _mm256_madd_epi16(b_dc,     a_dr_3_swp);

    __m256i hsum0       = _mm256_hadd_epi32(prod0,    prod0_swp);
    __m256i hsum1       = _mm256_hadd_epi32(prod1,    prod1_swp);
    __m256i hsum2       = _mm256_hadd_epi32(prod2,    prod2_swp);
    __m256i hsum3       = _mm256_hadd_epi32(prod3,    prod3_swp);

    __m256i hsum2c_0    = _mm256_hadd_epi32(hsum0,    hsum1);
    __m256i hsum2c_1    = _mm256_hadd_epi32(hsum2,    hsum3);

    __m256i hsum2c_0_tr = truncate(hsum2c_0, debias, shift);
    __m256i hsum2c_1_tr = truncate(hsum2c_1, debias, shift);

    __m256i tmp_dc      = _mm256_packs_epi32(hsum2c_0_tr, hsum2c_1_tr);

    output[dry]         = _mm256_shuffle_epi8(tmp_dc, shuf_lorow_mask);
  }
}

// Multiplies A by B_T's transpose and stores result in output
// which should be an array of 4 __m256i's
static void matmul_8x8_a_bt(const int16_t *a, const __m256i *b_t,
    int16_t *output, const int8_t shift)
{
  const int32_t add    = 1 << (shift - 1);
  const __m256i debias = _mm256_set1_epi32(add);

  const __m256i shuf_lorow_mask =
      _mm256_setr_epi8(0,  1,  2,  3,  4,  5,  6,  7,
                       8,  9,  10, 11, 12, 13, 14, 15,
                       18, 19, 16, 17, 22, 23, 20, 21,
                       26, 27, 24, 25, 30, 31, 28, 29);

  const __m256i *a_256 = (const __m256i *)a;

  __m256i b_dc_0      = b_t[0];
  __m256i b_dc_1      = b_t[1];
  __m256i b_dc_2      = b_t[2];
  __m256i b_dc_3      = b_t[3];

  __m256i b_dc_0_swp  = swap_lanes(b_dc_0);
  __m256i b_dc_1_swp  = swap_lanes(b_dc_1);
  __m256i b_dc_2_swp  = swap_lanes(b_dc_2);
  __m256i b_dc_3_swp  = swap_lanes(b_dc_3);

  for (int dry = 0; dry < 4; dry++) {
    __m256i a_dr        = _mm256_load_si256(a_256 + dry);

    __m256i prod0       = _mm256_madd_epi16(a_dr,     b_dc_0);
    __m256i prod0_swp   = _mm256_madd_epi16(a_dr,     b_dc_0_swp);
    __m256i prod1       = _mm256_madd_epi16(a_dr,     b_dc_1);
    __m256i prod1_swp   = _mm256_madd_epi16(a_dr,     b_dc_1_swp);
    __m256i prod2       = _mm256_madd_epi16(a_dr,     b_dc_2);
    __m256i prod2_swp   = _mm256_madd_epi16(a_dr,     b_dc_2_swp);
    __m256i prod3       = _mm256_madd_epi16(a_dr,     b_dc_3);
    __m256i prod3_swp   = _mm256_madd_epi16(a_dr,     b_dc_3_swp);

    __m256i hsum0       = _mm256_hadd_epi32(prod0,    prod0_swp);
    __m256i hsum1       = _mm256_hadd_epi32(prod1,    prod1_swp);
    __m256i hsum2       = _mm256_hadd_epi32(prod2,    prod2_swp);
    __m256i hsum3       = _mm256_hadd_epi32(prod3,    prod3_swp);

    __m256i hsum2c_0    = _mm256_hadd_epi32(hsum0,    hsum1);
    __m256i hsum2c_1    = _mm256_hadd_epi32(hsum2,    hsum3);

    __m256i hsum2c_0_tr = truncate(hsum2c_0, debias, shift);
    __m256i hsum2c_1_tr = truncate(hsum2c_1, debias, shift);

    __m256i tmp_dr      = _mm256_packs_epi32(hsum2c_0_tr, hsum2c_1_tr);

    __m256i final_dr    = _mm256_shuffle_epi8(tmp_dr, shuf_lorow_mask);

    _mm256_store_si256((__m256i *)output + dry, final_dr);
  }
}

static void matrix_dct_8x8_avx2(int8_t bitdepth, const int16_t *input, int16_t *output)
{
  int32_t shift_1st = kvz_g_convert_to_bit[8] + 1 + (bitdepth - 8);
  int32_t shift_2nd = kvz_g_convert_to_bit[8] + 8;

  const int16_t *dct  = &kvz_g_dct_8[0][0];

  /*
   * Multiply input by the tranpose of DCT matrix into tmpres, and DCT matrix
   * by tmpres - this is then our output matrix
   *
   * It's easier to implement an AVX2 matrix multiplication if you can multiply
   * the left term with the transpose of the right term. Here things are stored
   * row-wise, not column-wise, so we can effectively read DCT_T column-wise
   * into YMM registers by reading DCT row-wise. Also because of this, the
   * first multiplication is hacked to produce the transpose of the result
   * instead, since it will be used in similar fashion as the right operand
   * in the second multiplication.
   */

  __m256i tmpres[4];

  matmul_8x8_a_bt_t(input,  dct, tmpres, shift_1st);
  matmul_8x8_a_bt  (dct, tmpres, output, shift_2nd);
}

static void matrix_idct_8x8_avx2(int8_t bitdepth, const int16_t *input, int16_t *output)
{
  int32_t shift_1st = 7;
  int32_t shift_2nd = 12 - (bitdepth - 8);
  ALIGNED(64) int16_t tmp[8 * 8];

  const int16_t *tdct = &kvz_g_dct_8_t[0][0];
  const int16_t *dct  = &kvz_g_dct_8  [0][0];

  mul_clip_matrix_8x8_avx2(tdct, input, tmp,    shift_1st);
  mul_clip_matrix_8x8_avx2(tmp,  dct,   output, shift_2nd);

  /*
   * Because:
   * out = tdct * input * dct = tdct * (input * dct) = tdct * (input * transpose(tdct))
   * This could almost be done this way:
   *
   * matmul_8x8_a_bt_t(input, tdct, debias1, shift_1st, tmp);
   * matmul_8x8_a_bt  (tdct,  tmp,  debias2, shift_2nd, output);
   *
   * But not really, since it will fall victim to some very occasional
   * rounding errors. Sadly.
   */
}

static void matmul_16x16_a_bt(const __m256i *a,
                              const __m256i *b_t,
                                    __m256i *output,
                              const int32_t  shift)
{
  const int32_t add    = 1 << (shift - 1);
  const __m256i debias = _mm256_set1_epi32(add);

  for (int32_t y = 0; y < 16; y++) {
    __m256i a_r = a[y];
    __m256i results_32[2];

    for (int32_t fco = 0; fco < 2; fco++) {
      // Read first cols 0, 1, 2, 3, 8, 9, 10, 11, and then next 4
      __m256i bt_c0  = b_t[fco * 4 + 0];
      __m256i bt_c1  = b_t[fco * 4 + 1];
      __m256i bt_c2  = b_t[fco * 4 + 2];
      __m256i bt_c3  = b_t[fco * 4 + 3];
      __m256i bt_c8  = b_t[fco * 4 + 8];
      __m256i bt_c9  = b_t[fco * 4 + 9];
      __m256i bt_c10 = b_t[fco * 4 + 10];
      __m256i bt_c11 = b_t[fco * 4 + 11];

      __m256i p0  = _mm256_madd_epi16(a_r, bt_c0);
      __m256i p1  = _mm256_madd_epi16(a_r, bt_c1);
      __m256i p2  = _mm256_madd_epi16(a_r, bt_c2);
      __m256i p3  = _mm256_madd_epi16(a_r, bt_c3);
      __m256i p8  = _mm256_madd_epi16(a_r, bt_c8);
      __m256i p9  = _mm256_madd_epi16(a_r, bt_c9);
      __m256i p10 = _mm256_madd_epi16(a_r, bt_c10);
      __m256i p11 = _mm256_madd_epi16(a_r, bt_c11);

      // Combine low lanes from P0 and P8, high lanes from them, and the same
      // with P1:P9 and so on
      __m256i p0l = _mm256_permute2x128_si256(p0, p8,  0x20);
      __m256i p0h = _mm256_permute2x128_si256(p0, p8,  0x31);
      __m256i p1l = _mm256_permute2x128_si256(p1, p9,  0x20);
      __m256i p1h = _mm256_permute2x128_si256(p1, p9,  0x31);
      __m256i p2l = _mm256_permute2x128_si256(p2, p10, 0x20);
      __m256i p2h = _mm256_permute2x128_si256(p2, p10, 0x31);
      __m256i p3l = _mm256_permute2x128_si256(p3, p11, 0x20);
      __m256i p3h = _mm256_permute2x128_si256(p3, p11, 0x31);

      __m256i s0  = _mm256_add_epi32(p0l, p0h);
      __m256i s1  = _mm256_add_epi32(p1l, p1h);
      __m256i s2  = _mm256_add_epi32(p2l, p2h);
      __m256i s3  = _mm256_add_epi32(p3l, p3h);

      __m256i s4  = _mm256_unpacklo_epi64(s0, s1);
      __m256i s5  = _mm256_unpackhi_epi64(s0, s1);
      __m256i s6  = _mm256_unpacklo_epi64(s2, s3);
      __m256i s7  = _mm256_unpackhi_epi64(s2, s3);

      __m256i s8  = _mm256_add_epi32(s4, s5);
      __m256i s9  = _mm256_add_epi32(s6, s7);

      __m256i res = _mm256_hadd_epi32(s8, s9);
      results_32[fco] = truncate(res, debias, shift);
    }
    output[y] = _mm256_packs_epi32(results_32[0], results_32[1]);
  }
}

// NOTE: The strides measured by s_stride_log2 and d_stride_log2 are in units
// of 16 coeffs, not 1!
static void transpose_16x16_stride(const int16_t *src,
                                         int16_t *dst,
                                         uint8_t  s_stride_log2,
                                         uint8_t  d_stride_log2)
{
  __m256i tmp_128[16];
  for (uint32_t i = 0; i < 16; i += 8) {

    // After every n-bit unpack, 2n-bit units in the vectors will be in
    // correct order. Pair words first, then dwords, then qwords. After that,
    // whole lanes will be correct.
    __m256i tmp_32[8];
    __m256i tmp_64[8];

    __m256i m[8] = {
      _mm256_load_si256((const __m256i *)src + ((i + 0) << s_stride_log2)),
      _mm256_load_si256((const __m256i *)src + ((i + 1) << s_stride_log2)),
      _mm256_load_si256((const __m256i *)src + ((i + 2) << s_stride_log2)),
      _mm256_load_si256((const __m256i *)src + ((i + 3) << s_stride_log2)),
      _mm256_load_si256((const __m256i *)src + ((i + 4) << s_stride_log2)),
      _mm256_load_si256((const __m256i *)src + ((i + 5) << s_stride_log2)),
      _mm256_load_si256((const __m256i *)src + ((i + 6) << s_stride_log2)),
      _mm256_load_si256((const __m256i *)src + ((i + 7) << s_stride_log2)),
    };

    tmp_32[0]      = _mm256_unpacklo_epi16(     m[0],      m[1]);
    tmp_32[1]      = _mm256_unpacklo_epi16(     m[2],      m[3]);
    tmp_32[2]      = _mm256_unpackhi_epi16(     m[0],      m[1]);
    tmp_32[3]      = _mm256_unpackhi_epi16(     m[2],      m[3]);

    tmp_32[4]      = _mm256_unpacklo_epi16(     m[4],      m[5]);
    tmp_32[5]      = _mm256_unpacklo_epi16(     m[6],      m[7]);
    tmp_32[6]      = _mm256_unpackhi_epi16(     m[4],      m[5]);
    tmp_32[7]      = _mm256_unpackhi_epi16(     m[6],      m[7]);


    tmp_64[0]      = _mm256_unpacklo_epi32(tmp_32[0], tmp_32[1]);
    tmp_64[1]      = _mm256_unpacklo_epi32(tmp_32[2], tmp_32[3]);
    tmp_64[2]      = _mm256_unpackhi_epi32(tmp_32[0], tmp_32[1]);
    tmp_64[3]      = _mm256_unpackhi_epi32(tmp_32[2], tmp_32[3]);

    tmp_64[4]      = _mm256_unpacklo_epi32(tmp_32[4], tmp_32[5]);
    tmp_64[5]      = _mm256_unpacklo_epi32(tmp_32[6], tmp_32[7]);
    tmp_64[6]      = _mm256_unpackhi_epi32(tmp_32[4], tmp_32[5]);
    tmp_64[7]      = _mm256_unpackhi_epi32(tmp_32[6], tmp_32[7]);


    tmp_128[i + 0] = _mm256_unpacklo_epi64(tmp_64[0], tmp_64[4]);
    tmp_128[i + 1] = _mm256_unpackhi_epi64(tmp_64[0], tmp_64[4]);
    tmp_128[i + 2] = _mm256_unpacklo_epi64(tmp_64[2], tmp_64[6]);
    tmp_128[i + 3] = _mm256_unpackhi_epi64(tmp_64[2], tmp_64[6]);

    tmp_128[i + 4] = _mm256_unpacklo_epi64(tmp_64[1], tmp_64[5]);
    tmp_128[i + 5] = _mm256_unpackhi_epi64(tmp_64[1], tmp_64[5]);
    tmp_128[i + 6] = _mm256_unpacklo_epi64(tmp_64[3], tmp_64[7]);
    tmp_128[i + 7] = _mm256_unpackhi_epi64(tmp_64[3], tmp_64[7]);
  }

  for (uint32_t i = 0; i < 8; i++) {
    uint32_t loid     = i + 0;
    uint32_t hiid     = i + 8;

    uint32_t dst_loid = loid << d_stride_log2;
    uint32_t dst_hiid = hiid << d_stride_log2;

    __m256i lo       = tmp_128[loid];
    __m256i hi       = tmp_128[hiid];
    __m256i final_lo = _mm256_permute2x128_si256(lo, hi, 0x20);
    __m256i final_hi = _mm256_permute2x128_si256(lo, hi, 0x31);

    _mm256_store_si256((__m256i *)dst + dst_loid, final_lo);
    _mm256_store_si256((__m256i *)dst + dst_hiid, final_hi);
  }
}

static void transpose_16x16(const int16_t *src, int16_t *dst)
{
  transpose_16x16_stride(src, dst, 0, 0);
}

static __m256i truncate_inv(__m256i v, int32_t shift)
{
  int32_t add = 1 << (shift - 1);

  __m256i debias  = _mm256_set1_epi32(add);
  __m256i v2      = _mm256_add_epi32 (v,  debias);
  __m256i trunced = _mm256_srai_epi32(v2, shift);
  return  trunced;
}

static __m256i extract_odds(__m256i v)
{
  // 0 1 2 3 4 5 6 7 | 8 9 a b c d e f => 1 3 5 7 1 3 5 7 | 9 b d f 9 b d f
  const __m256i oddmask = _mm256_setr_epi8( 2,  3,  6,  7, 10, 11, 14, 15,
                                            2,  3,  6,  7, 10, 11, 14, 15,
                                            2,  3,  6,  7, 10, 11, 14, 15,
                                            2,  3,  6,  7, 10, 11, 14, 15);

  __m256i tmp = _mm256_shuffle_epi8 (v,   oddmask);
  return _mm256_permute4x64_epi64   (tmp, _MM_SHUFFLE(3, 1, 2, 0));
}

static __m256i extract_combine_odds(__m256i v0, __m256i v1)
{
  // 0 1 2 3 4 5 6 7 | 8 9 a b c d e f => 1 3 5 7 1 3 5 7 | 9 b d f 9 b d f
  const __m256i oddmask = _mm256_setr_epi8( 2,  3,  6,  7, 10, 11, 14, 15,
                                            2,  3,  6,  7, 10, 11, 14, 15,
                                            2,  3,  6,  7, 10, 11, 14, 15,
                                            2,  3,  6,  7, 10, 11, 14, 15);

  __m256i tmp0 = _mm256_shuffle_epi8(v0,   oddmask);
  __m256i tmp1 = _mm256_shuffle_epi8(v1,   oddmask);

  __m256i tmp2 = _mm256_blend_epi32 (tmp0, tmp1, 0xcc); // 1100 1100

  return _mm256_permute4x64_epi64   (tmp2, _MM_SHUFFLE(3, 1, 2, 0));
}

// Extract items 2, 6, A and E from first four columns of DCT, order them as
// follows:
// D0,2 D0,6 D1,2 D1,6 D1,a D1,e D0,a D0,e | D2,2 D2,6 D3,2 D3,6 D3,a D3,e D2,a D2,e
static __m256i extract_26ae(const __m256i *tdct)
{
  // 02 03 22 23 06 07 26 27 | 0a 0b 2a 2b 02 0f 2e 2f
  // =>
  // 02 06 22 26 02 06 22 26 | 2a 2e 0a 0e 2a 2e 0a 0e
  const __m256i evens_mask = _mm256_setr_epi8( 0,  1,  8,  9,  4,  5, 12, 13,
                                               0,  1,  8,  9,  4,  5, 12, 13,
                                               4,  5, 12, 13,  0,  1,  8,  9,
                                               4,  5, 12, 13,  0,  1,  8,  9);

  __m256i shufd_0 = _mm256_shuffle_epi32(tdct[0], _MM_SHUFFLE(2, 3, 0, 1));
  __m256i shufd_2 = _mm256_shuffle_epi32(tdct[2], _MM_SHUFFLE(2, 3, 0, 1));

  __m256i cmbd_01 = _mm256_blend_epi32(shufd_0, tdct[1], 0xaa); // 1010 1010
  __m256i cmbd_23 = _mm256_blend_epi32(shufd_2, tdct[3], 0xaa); // 1010 1010

  __m256i evens_01 = _mm256_shuffle_epi8(cmbd_01, evens_mask);
  __m256i evens_23 = _mm256_shuffle_epi8(cmbd_23, evens_mask);

  __m256i evens_0123 = _mm256_unpacklo_epi64(evens_01, evens_23);

  return _mm256_permute4x64_epi64(evens_0123, _MM_SHUFFLE(3, 1, 2, 0));
}

// 2 6 2 6 a e a e | 2 6 2 6 a e a e
static __m256i extract_26ae_vec(__m256i col)
{
  const __m256i mask_26ae = _mm256_set1_epi32(0x0d0c0504);

  // 2 6 2 6 2 6 2 6 | a e a e a e a e
  __m256i reord = _mm256_shuffle_epi8     (col,   mask_26ae);
  __m256i final = _mm256_permute4x64_epi64(reord, _MM_SHUFFLE(3, 1, 2, 0));
  return  final;
}

// D00 D80 D01 D81 D41 Dc1 D40 Dc0 | D40 Dc0 D41 Dc1 D01 D81 D00 D80
static __m256i extract_d048c(const __m256i *tdct)
{
  const __m256i final_shuf = _mm256_setr_epi8( 0,  1,  8,  9,  2,  3, 10, 11,
                                               6,  7, 14, 15,  4,  5, 12, 13,
                                               4,  5, 12, 13,  6,  7, 14, 15,
                                               2,  3, 10, 11,  0,  1,  8,  9);
  __m256i c0 = tdct[0];
  __m256i c1 = tdct[1];

  __m256i c1_2  = _mm256_slli_epi32       (c1,    16);
  __m256i cmbd  = _mm256_blend_epi16      (c0,    c1_2, 0x22); // 0010 0010
  __m256i cmbd2 = _mm256_shuffle_epi32    (cmbd,  _MM_SHUFFLE(2, 0, 2, 0));
  __m256i cmbd3 = _mm256_permute4x64_epi64(cmbd2, _MM_SHUFFLE(3, 1, 2, 0));
  __m256i final = _mm256_shuffle_epi8     (cmbd3, final_shuf);

  return final;
}

// 0 8 0 8 4 c 4 c | 4 c 4 c 0 8 0 8
static __m256i extract_d048c_vec(__m256i col)
{
  const __m256i shufmask = _mm256_setr_epi8( 0,  1,  0,  1,  8,  9,  8,  9,
                                             8,  9,  8,  9,  0,  1,  0,  1,
                                             0,  1,  0,  1,  8,  9,  8,  9,
                                             8,  9,  8,  9,  0,  1,  0,  1);

  __m256i col_db4s = _mm256_shuffle_epi8     (col, shufmask);
  __m256i col_los  = _mm256_permute4x64_epi64(col_db4s, _MM_SHUFFLE(1, 1, 0, 0));
  __m256i col_his  = _mm256_permute4x64_epi64(col_db4s, _MM_SHUFFLE(3, 3, 2, 2));

  __m256i final    = _mm256_unpacklo_epi16   (col_los,  col_his);
  return final;
}

static void partial_butterfly_inverse_16_avx2(const int16_t *src, int16_t *dst, int32_t shift)
{
  __m256i tsrc[16];

  const uint32_t width = 16;

  const int16_t *tdct = &kvz_g_dct_16_t[0][0];

  const __m256i  eo_signmask = _mm256_setr_epi32( 1,  1,  1,  1, -1, -1, -1, -1);
  const __m256i eeo_signmask = _mm256_setr_epi32( 1,  1, -1, -1, -1, -1,  1,  1);
  const __m256i   o_signmask = _mm256_set1_epi32(-1);

  const __m256i final_shufmask = _mm256_setr_epi8( 0,  1,  2,  3,  4,  5,  6,  7,
                                                   8,  9, 10, 11, 12, 13, 14, 15,
                                                   6,  7,  4,  5,  2,  3,  0,  1,
                                                  14, 15, 12, 13, 10, 11,  8,  9);
  transpose_16x16(src, (int16_t *)tsrc);

  const __m256i dct_cols[8] = {
    _mm256_load_si256((const __m256i *)tdct + 0),
    _mm256_load_si256((const __m256i *)tdct + 1),
    _mm256_load_si256((const __m256i *)tdct + 2),
    _mm256_load_si256((const __m256i *)tdct + 3),
    _mm256_load_si256((const __m256i *)tdct + 4),
    _mm256_load_si256((const __m256i *)tdct + 5),
    _mm256_load_si256((const __m256i *)tdct + 6),
    _mm256_load_si256((const __m256i *)tdct + 7),
  };

  // These contain: D1,0 D3,0 D5,0 D7,0 D9,0 Db,0 Dd,0 Df,0 | D1,4 D3,4 D5,4 D7,4 D9,4 Db,4 Dd,4 Df,4
  //                D1,1 D3,1 D5,1 D7,1 D9,1 Db,1 Dd,1 Df,1 | D1,5 D3,5 D5,5 D7,5 D9,5 Db,5 Dd,5 Df,5
  //                D1,2 D3,2 D5,2 D7,2 D9,2 Db,2 Dd,2 Df,2 | D1,6 D3,6 D5,6 D7,6 D9,6 Db,6 Dd,6 Df,6
  //                D1,3 D3,3 D5,3 D7,3 D9,3 Db,3 Dd,3 Df,3 | D1,7 D3,7 D5,7 D7,7 D9,7 Db,7 Dd,7 Df,7
  __m256i dct_col_odds[4];
  for (uint32_t j = 0; j < 4; j++) {
    dct_col_odds[j] = extract_combine_odds(dct_cols[j + 0], dct_cols[j + 4]);
  }
  for (uint32_t j = 0; j < width; j++) {
    __m256i col = tsrc[j];
    __m256i odds = extract_odds(col);

    __m256i o04   = _mm256_madd_epi16           (odds,     dct_col_odds[0]);
    __m256i o15   = _mm256_madd_epi16           (odds,     dct_col_odds[1]);
    __m256i o26   = _mm256_madd_epi16           (odds,     dct_col_odds[2]);
    __m256i o37   = _mm256_madd_epi16           (odds,     dct_col_odds[3]);

    __m256i o0145 = _mm256_hadd_epi32           (o04,      o15);
    __m256i o2367 = _mm256_hadd_epi32           (o26,      o37);

    __m256i o     = _mm256_hadd_epi32           (o0145,    o2367);

    // D0,2 D0,6 D1,2 D1,6 D1,a D1,e D0,a D0,e | D2,2 D2,6 D3,2 D3,6 D3,a D3,e D2,a D2,e
    __m256i d_db2 = extract_26ae(dct_cols);

    // 2 6 2 6 a e a e | 2 6 2 6 a e a e
    __m256i t_db2 = extract_26ae_vec            (col);

    __m256i eo_parts  = _mm256_madd_epi16       (d_db2,    t_db2);
    __m256i eo_parts2 = _mm256_shuffle_epi32    (eo_parts, _MM_SHUFFLE(0, 1, 2, 3));

    // EO0 EO1 EO1 EO0 | EO2 EO3 EO3 EO2
    __m256i eo        = _mm256_add_epi32        (eo_parts, eo_parts2);
    __m256i eo2       = _mm256_permute4x64_epi64(eo,       _MM_SHUFFLE(1, 3, 2, 0));
    __m256i eo3       = _mm256_sign_epi32       (eo2,      eo_signmask);

    __m256i d_db4     = extract_d048c           (dct_cols);
    __m256i t_db4     = extract_d048c_vec       (col);
    __m256i eee_eeo   = _mm256_madd_epi16       (d_db4,   t_db4);

    __m256i eee_eee   = _mm256_permute4x64_epi64(eee_eeo,  _MM_SHUFFLE(3, 0, 3, 0));
    __m256i eeo_eeo1  = _mm256_permute4x64_epi64(eee_eeo,  _MM_SHUFFLE(1, 2, 1, 2));

    __m256i eeo_eeo2  = _mm256_sign_epi32       (eeo_eeo1, eeo_signmask);

    // EE0 EE1 EE2 EE3 | EE3 EE2 EE1 EE0
    __m256i ee        = _mm256_add_epi32        (eee_eee,  eeo_eeo2);
    __m256i e         = _mm256_add_epi32        (ee,       eo3);

    __m256i o_neg     = _mm256_sign_epi32       (o,        o_signmask);
    __m256i o_lo      = _mm256_blend_epi32      (o,        o_neg, 0xf0); // 1111 0000
    __m256i o_hi      = _mm256_blend_epi32      (o,        o_neg, 0x0f); // 0000 1111

    __m256i res_lo    = _mm256_add_epi32        (e,        o_lo);
    __m256i res_hi    = _mm256_add_epi32        (e,        o_hi);
    __m256i res_hi2   = _mm256_permute4x64_epi64(res_hi,   _MM_SHUFFLE(1, 0, 3, 2));

    __m256i res_lo_t  = truncate_inv(res_lo,  shift);
    __m256i res_hi_t  = truncate_inv(res_hi2, shift);

    __m256i res_16_1  = _mm256_packs_epi32      (res_lo_t, res_hi_t);
    __m256i final     = _mm256_shuffle_epi8     (res_16_1, final_shufmask);

    _mm256_store_si256((__m256i *)dst + j, final);
  }
}

static void matrix_idct_16x16_avx2(int8_t bitdepth, const int16_t *input, int16_t *output)
{
  int32_t shift_1st = 7;
  int32_t shift_2nd = 12 - (bitdepth - 8);
  ALIGNED(64) int16_t tmp[16 * 16];

  partial_butterfly_inverse_16_avx2(input, tmp,    shift_1st);
  partial_butterfly_inverse_16_avx2(tmp,   output, shift_2nd);
}

static void matrix_dct_16x16_avx2(int8_t bitdepth, const int16_t *input, int16_t *output)
{
  int32_t shift_1st = kvz_g_convert_to_bit[16] + 1 + (bitdepth - 8);
  int32_t shift_2nd = kvz_g_convert_to_bit[16] + 8;

  const int16_t *dct  = &kvz_g_dct_16[0][0];

  /*
   * Multiply input by the tranpose of DCT matrix into tmpres, and DCT matrix
   * by tmpres - this is then our output matrix
   *
   * It's easier to implement an AVX2 matrix multiplication if you can multiply
   * the left term with the transpose of the right term. Here things are stored
   * row-wise, not column-wise, so we can effectively read DCT_T column-wise
   * into YMM registers by reading DCT row-wise. Also because of this, the
   * first multiplication is hacked to produce the transpose of the result
   * instead, since it will be used in similar fashion as the right operand
   * in the second multiplication.
   */

  const __m256i *d_v = (const __m256i *)dct;
  const __m256i *i_v = (const __m256i *)input;
        __m256i *o_v = (      __m256i *)output;
  __m256i tmp[16];

  // Hack! (A * B^T)^T = B * A^T, so we can dispatch the transpose-produciong
  // multiply completely
  matmul_16x16_a_bt(d_v, i_v, tmp, shift_1st);
  matmul_16x16_a_bt(d_v, tmp, o_v, shift_2nd);
}

// 32x32 matrix multiplication with value clipping.
// Parameters: Two 32x32 matrices containing 16-bit values in consecutive addresses,
//             destination for the result and the shift value for clipping.
static void mul_clip_matrix_32x32_avx2(const int16_t *left,
                                       const int16_t *right,
                                             int16_t *dst,
                                       const int32_t  shift)
{
  const int32_t add    = 1 << (shift - 1);
  const __m256i debias = _mm256_set1_epi32(add);

  const uint32_t *l_32  = (const uint32_t *)left;
  const __m256i  *r_v   = (const __m256i *)right;
        __m256i  *dst_v = (      __m256i *)dst;

  __m256i accu[128] = {_mm256_setzero_si256()};
  size_t i, j;

  for (j = 0; j < 64; j += 4) {
    const __m256i r0 = r_v[j + 0];
    const __m256i r1 = r_v[j + 1];
    const __m256i r2 = r_v[j + 2];
    const __m256i r3 = r_v[j + 3];

    __m256i r02l   = _mm256_unpacklo_epi16(r0, r2);
    __m256i r02h   = _mm256_unpackhi_epi16(r0, r2);
    __m256i r13l   = _mm256_unpacklo_epi16(r1, r3);
    __m256i r13h   = _mm256_unpackhi_epi16(r1, r3);

    __m256i r02_07 = _mm256_permute2x128_si256(r02l, r02h, 0x20);
    __m256i r02_8f = _mm256_permute2x128_si256(r02l, r02h, 0x31);

    __m256i r13_07 = _mm256_permute2x128_si256(r13l, r13h, 0x20);
    __m256i r13_8f = _mm256_permute2x128_si256(r13l, r13h, 0x31);

    for (i = 0; i < 32; i += 2) {
      size_t acc_base = i << 2;

      uint32_t curr_e    = l_32[(i + 0) * (32 / 2) + (j >> 2)];
      uint32_t curr_o    = l_32[(i + 1) * (32 / 2) + (j >> 2)];

      __m256i even       = _mm256_set1_epi32(curr_e);
      __m256i odd        = _mm256_set1_epi32(curr_o);

      __m256i p_e0       = _mm256_madd_epi16(even, r02_07);
      __m256i p_e1       = _mm256_madd_epi16(even, r02_8f);
      __m256i p_e2       = _mm256_madd_epi16(even, r13_07);
      __m256i p_e3       = _mm256_madd_epi16(even, r13_8f);

      __m256i p_o0       = _mm256_madd_epi16(odd,  r02_07);
      __m256i p_o1       = _mm256_madd_epi16(odd,  r02_8f);
      __m256i p_o2       = _mm256_madd_epi16(odd,  r13_07);
      __m256i p_o3       = _mm256_madd_epi16(odd,  r13_8f);

      accu[acc_base + 0] = _mm256_add_epi32 (p_e0, accu[acc_base + 0]);
      accu[acc_base + 1] = _mm256_add_epi32 (p_e1, accu[acc_base + 1]);
      accu[acc_base + 2] = _mm256_add_epi32 (p_e2, accu[acc_base + 2]);
      accu[acc_base + 3] = _mm256_add_epi32 (p_e3, accu[acc_base + 3]);

      accu[acc_base + 4] = _mm256_add_epi32 (p_o0, accu[acc_base + 4]);
      accu[acc_base + 5] = _mm256_add_epi32 (p_o1, accu[acc_base + 5]);
      accu[acc_base + 6] = _mm256_add_epi32 (p_o2, accu[acc_base + 6]);
      accu[acc_base + 7] = _mm256_add_epi32 (p_o3, accu[acc_base + 7]);
    }
  }

  for (i = 0; i < 32; i++) {
    size_t acc_base = i << 2;
    size_t dst_base = i << 1;

    __m256i q0  = truncate(accu[acc_base + 0], debias, shift);
    __m256i q1  = truncate(accu[acc_base + 1], debias, shift);
    __m256i q2  = truncate(accu[acc_base + 2], debias, shift);
    __m256i q3  = truncate(accu[acc_base + 3], debias, shift);

    __m256i h01 = _mm256_packs_epi32(q0, q1);
    __m256i h23 = _mm256_packs_epi32(q2, q3);

            h01 = _mm256_permute4x64_epi64(h01, _MM_SHUFFLE(3, 1, 2, 0));
            h23 = _mm256_permute4x64_epi64(h23, _MM_SHUFFLE(3, 1, 2, 0));

    _mm256_store_si256(dst_v + dst_base + 0, h01);
    _mm256_store_si256(dst_v + dst_base + 1, h23);
  }
}

// Macro that generates 2D transform functions with clipping values.
// Sets correct shift values and matrices according to transform type and
// block size. Performs matrix multiplication horizontally and vertically.
#define TRANSFORM(type, n) static void matrix_ ## type ## _ ## n ## x ## n ## _avx2(int8_t bitdepth, const int16_t *input, int16_t *output)\
{\
  int32_t shift_1st = kvz_g_convert_to_bit[n] + 1 + (bitdepth - 8); \
  int32_t shift_2nd = kvz_g_convert_to_bit[n] + 8; \
  ALIGNED(64) int16_t tmp[n * n];\
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
  ALIGNED(64) int16_t tmp[n * n];\
  const int16_t *tdct = &kvz_g_ ## type ## _ ## n ## _t[0][0];\
  const int16_t *dct = &kvz_g_ ## type ## _ ## n [0][0];\
\
  mul_clip_matrix_ ## n ## x ## n ## _avx2(tdct, input, tmp, shift_1st);\
  mul_clip_matrix_ ## n ## x ## n ## _avx2(tmp, dct, output, shift_2nd);\
}\

// Ha, we've got a tailored implementation for these
// TRANSFORM(dst, 4);
// ITRANSFORM(dst, 4);
// TRANSFORM(dct, 4);
// ITRANSFORM(dct, 4);
// TRANSFORM(dct, 8);
// ITRANSFORM(dct, 8);
// TRANSFORM(dct, 16);
// ITRANSFORM(dct, 16);

// Generate all the transform functions

TRANSFORM(dct, 32);
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
