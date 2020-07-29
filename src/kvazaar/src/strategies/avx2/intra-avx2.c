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

#include "strategies/avx2/intra-avx2.h"

#if COMPILE_INTEL_AVX2 && defined X86_64
#include <immintrin.h>
#include <stdlib.h>

#include "kvazaar.h"
#include "strategyselector.h"
#include "strategies/missing-intel-intrinsics.h"


 /**
 * \brief Linear interpolation for 4 pixels. Returns 4 filtered pixels in lowest 32-bits of the register.
 * \param ref_main      Reference pixels
 * \param delta_pos     Fractional pixel precise position of sample displacement
 * \param x             Sample offset in direction x in ref_main array
 */
static INLINE __m128i filter_4x1_avx2(const kvz_pixel *ref_main, int16_t delta_pos, int x){

  int8_t delta_int = delta_pos >> 5;
  int8_t delta_fract = delta_pos & (32-1);
  __m128i sample0 = _mm_cvtsi32_si128(*(uint32_t*)&(ref_main[x + delta_int]));
  __m128i sample1 = _mm_cvtsi32_si128(*(uint32_t*)&(ref_main[x + delta_int + 1]));
  __m128i pairs = _mm_unpacklo_epi8(sample0, sample1);
  __m128i weight = _mm_set1_epi16( (delta_fract << 8) | (32 - delta_fract) );
  sample0 = _mm_maddubs_epi16(pairs, weight);
  sample0 = _mm_add_epi16(sample0, _mm_set1_epi16(16));
  sample0 = _mm_srli_epi16(sample0, 5);
  sample0 = _mm_packus_epi16(sample0, sample0);

  return sample0;
}

 /**
 * \brief Linear interpolation for 4x4 block. Writes filtered 4x4 block to dst.
 * \param dst           Destination buffer
 * \param ref_main      Reference pixels
 * \param sample_disp   Sample displacement per row
 * \param vertical_mode Mode direction, true if vertical
 */
static void filter_4x4_avx2(kvz_pixel *dst, const kvz_pixel *ref_main, int sample_disp, bool vertical_mode){

  __m128i row0 = filter_4x1_avx2(ref_main, 1 * sample_disp, 0);
  __m128i row1 = filter_4x1_avx2(ref_main, 2 * sample_disp, 0);
  __m128i row2 = filter_4x1_avx2(ref_main, 3 * sample_disp, 0);
  __m128i row3 = filter_4x1_avx2(ref_main, 4 * sample_disp, 0);

  //Transpose if horizontal mode
  if (!vertical_mode) {
    __m128i temp = _mm_unpacklo_epi16(_mm_unpacklo_epi8(row0, row1), _mm_unpacklo_epi8(row2, row3));
    row0 = _mm_cvtsi32_si128(_mm_extract_epi32(temp, 0));
    row1 = _mm_cvtsi32_si128(_mm_extract_epi32(temp, 1));
    row2 = _mm_cvtsi32_si128(_mm_extract_epi32(temp, 2));
    row3 = _mm_cvtsi32_si128(_mm_extract_epi32(temp, 3));
  }

  *(int32_t*)(dst + 0 * 4) = _mm_cvtsi128_si32(row0);
  *(int32_t*)(dst + 1 * 4) = _mm_cvtsi128_si32(row1);
  *(int32_t*)(dst + 2 * 4) = _mm_cvtsi128_si32(row2);
  *(int32_t*)(dst + 3 * 4) = _mm_cvtsi128_si32(row3);
}

 /**
 * \brief Linear interpolation for 8 pixels. Returns 8 filtered pixels in lower 64-bits of the register.
 * \param ref_main      Reference pixels
 * \param delta_pos     Fractional pixel precise position of sample displacement
 * \param x             Sample offset in direction x in ref_main array
 */
static INLINE __m128i filter_8x1_avx2(const kvz_pixel *ref_main, int16_t delta_pos, int x){

  int8_t delta_int = delta_pos >> 5;
  int8_t delta_fract = delta_pos & (32-1);
  __m128i sample0 = _mm_cvtsi64_si128(*(uint64_t*)&(ref_main[x + delta_int]));
  __m128i sample1 = _mm_cvtsi64_si128(*(uint64_t*)&(ref_main[x + delta_int + 1]));
  __m128i pairs_lo = _mm_unpacklo_epi8(sample0, sample1);

  __m128i weight = _mm_set1_epi16( (delta_fract << 8) | (32 - delta_fract) );
  __m128i v_temp_lo = _mm_maddubs_epi16(pairs_lo, weight);
  v_temp_lo = _mm_add_epi16(v_temp_lo, _mm_set1_epi16(16));
  v_temp_lo = _mm_srli_epi16(v_temp_lo, 5);
  sample0 = _mm_packus_epi16(v_temp_lo, v_temp_lo);

  return sample0;
}

 /**
 * \brief Linear interpolation for 8x8 block. Writes filtered 8x8 block to dst.
 * \param dst           Destination buffer
 * \param ref_main      Reference pixels
 * \param sample_disp   Sample displacement per row
 * \param vertical_mode Mode direction, true if vertical
 */
static void filter_8x8_avx2(kvz_pixel *dst, const kvz_pixel *ref_main, int sample_disp, bool vertical_mode){
  __m128i row0 = filter_8x1_avx2(ref_main, 1 * sample_disp, 0);
  __m128i row1 = filter_8x1_avx2(ref_main, 2 * sample_disp, 0);
  __m128i row2 = filter_8x1_avx2(ref_main, 3 * sample_disp, 0);
  __m128i row3 = filter_8x1_avx2(ref_main, 4 * sample_disp, 0);
  __m128i row4 = filter_8x1_avx2(ref_main, 5 * sample_disp, 0);
  __m128i row5 = filter_8x1_avx2(ref_main, 6 * sample_disp, 0);
  __m128i row6 = filter_8x1_avx2(ref_main, 7 * sample_disp, 0);
  __m128i row7 = filter_8x1_avx2(ref_main, 8 * sample_disp, 0);

  //Transpose if horizontal mode
  if (!vertical_mode) {
    __m128i q0 = _mm_unpacklo_epi8(row0, row1);
    __m128i q1 = _mm_unpacklo_epi8(row2, row3);
    __m128i q2 = _mm_unpacklo_epi8(row4, row5);
    __m128i q3 = _mm_unpacklo_epi8(row6, row7);

    __m128i h0 = _mm_unpacklo_epi16(q0, q1);
    __m128i h1 = _mm_unpacklo_epi16(q2, q3);
    __m128i h2 = _mm_unpackhi_epi16(q0, q1);
    __m128i h3 = _mm_unpackhi_epi16(q2, q3);

    __m128i temp0 = _mm_unpacklo_epi32(h0, h1);
    __m128i temp1 = _mm_unpackhi_epi32(h0, h1);
    __m128i temp2 = _mm_unpacklo_epi32(h2, h3);
    __m128i temp3 = _mm_unpackhi_epi32(h2, h3);

    row0 = _mm_cvtsi64_si128(_mm_extract_epi64(temp0, 0));
    row1 = _mm_cvtsi64_si128(_mm_extract_epi64(temp0, 1));
    row2 = _mm_cvtsi64_si128(_mm_extract_epi64(temp1, 0));
    row3 = _mm_cvtsi64_si128(_mm_extract_epi64(temp1, 1));
    row4 = _mm_cvtsi64_si128(_mm_extract_epi64(temp2, 0));
    row5 = _mm_cvtsi64_si128(_mm_extract_epi64(temp2, 1));
    row6 = _mm_cvtsi64_si128(_mm_extract_epi64(temp3, 0));
    row7 = _mm_cvtsi64_si128(_mm_extract_epi64(temp3, 1));
  }
      
  _mm_storel_epi64((__m128i*)(dst + 0 * 8), row0);
  _mm_storel_epi64((__m128i*)(dst + 1 * 8), row1);
  _mm_storel_epi64((__m128i*)(dst + 2 * 8), row2);
  _mm_storel_epi64((__m128i*)(dst + 3 * 8), row3);
  _mm_storel_epi64((__m128i*)(dst + 4 * 8), row4);
  _mm_storel_epi64((__m128i*)(dst + 5 * 8), row5);
  _mm_storel_epi64((__m128i*)(dst + 6 * 8), row6);
  _mm_storel_epi64((__m128i*)(dst + 7 * 8), row7);
} 

 /**
 * \brief Linear interpolation for two 16 pixels. Returns 8 filtered pixels in lower 64-bits of both lanes of the YMM register.
 * \param ref_main      Reference pixels
 * \param delta_pos     Fractional pixel precise position of sample displacement
 * \param x             Sample offset in direction x in ref_main array
 */
static INLINE __m256i filter_16x1_avx2(const kvz_pixel *ref_main, int16_t delta_pos, int x){

  int8_t delta_int = delta_pos >> 5;
  int8_t delta_fract = delta_pos & (32-1);
  __m256i sample0 = _mm256_cvtepu8_epi16(_mm_loadu_si128((__m128i*)&(ref_main[x + delta_int])));
  sample0 = _mm256_packus_epi16(sample0, sample0);
  __m256i sample1 = _mm256_cvtepu8_epi16(_mm_loadu_si128((__m128i*)&(ref_main[x + delta_int + 1])));
  sample1 = _mm256_packus_epi16(sample1, sample1);
  __m256i pairs_lo = _mm256_unpacklo_epi8(sample0, sample1);

  __m256i weight = _mm256_set1_epi16( (delta_fract << 8) | (32 - delta_fract) );
  __m256i v_temp_lo = _mm256_maddubs_epi16(pairs_lo, weight);
  v_temp_lo = _mm256_add_epi16(v_temp_lo, _mm256_set1_epi16(16));
  v_temp_lo = _mm256_srli_epi16(v_temp_lo, 5);
  sample0 = _mm256_packus_epi16(v_temp_lo, v_temp_lo);

  return sample0;
}

 /**
 * \brief Linear interpolation for 16x16 block. Writes filtered 16x16 block to dst.
 * \param dst           Destination buffer
 * \param ref_main      Reference pixels
 * \param sample_disp   Sample displacement per row
 * \param vertical_mode Mode direction, true if vertical
 */
static void filter_16x16_avx2(kvz_pixel *dst, const kvz_pixel *ref_main, int sample_disp, bool vertical_mode){
  for (int y = 0; y < 16; y += 8) {
    __m256i row0 = filter_16x1_avx2(ref_main, (y + 1) * sample_disp, 0);
    __m256i row1 = filter_16x1_avx2(ref_main, (y + 2) * sample_disp, 0);
    __m256i row2 = filter_16x1_avx2(ref_main, (y + 3) * sample_disp, 0);
    __m256i row3 = filter_16x1_avx2(ref_main, (y + 4) * sample_disp, 0);
    __m256i row4 = filter_16x1_avx2(ref_main, (y + 5) * sample_disp, 0);
    __m256i row5 = filter_16x1_avx2(ref_main, (y + 6) * sample_disp, 0);
    __m256i row6 = filter_16x1_avx2(ref_main, (y + 7) * sample_disp, 0);
    __m256i row7 = filter_16x1_avx2(ref_main, (y + 8) * sample_disp, 0);

    if (!vertical_mode) {
      __m256i q0 = _mm256_unpacklo_epi8(row0, row1);
      __m256i q1 = _mm256_unpacklo_epi8(row2, row3);
      __m256i q2 = _mm256_unpacklo_epi8(row4, row5);
      __m256i q3 = _mm256_unpacklo_epi8(row6, row7);

      __m256i h0 = _mm256_unpacklo_epi16(q0, q1);
      __m256i h1 = _mm256_unpacklo_epi16(q2, q3);
      __m256i h2 = _mm256_unpackhi_epi16(q0, q1);
      __m256i h3 = _mm256_unpackhi_epi16(q2, q3);

      __m256i temp0 = _mm256_unpacklo_epi32(h0, h1);
      __m256i temp1 = _mm256_unpackhi_epi32(h0, h1);
      __m256i temp2 = _mm256_unpacklo_epi32(h2, h3);
      __m256i temp3 = _mm256_unpackhi_epi32(h2, h3);

      row0 = _mm256_unpacklo_epi64(temp0, temp0);
      row1 = _mm256_unpackhi_epi64(temp0, temp0);
      row2 = _mm256_unpacklo_epi64(temp1, temp1);
      row3 = _mm256_unpackhi_epi64(temp1, temp1);
      row4 = _mm256_unpacklo_epi64(temp2, temp2);
      row5 = _mm256_unpackhi_epi64(temp2, temp2);
      row6 = _mm256_unpacklo_epi64(temp3, temp3);
      row7 = _mm256_unpackhi_epi64(temp3, temp3);

      //x and y must be flipped due to transpose
      int rx = y;
      int ry = 0;

      *(int64_t*)(dst + (ry + 0) * 16 + rx) = _mm_cvtsi128_si64(_mm256_castsi256_si128(row0));
      *(int64_t*)(dst + (ry + 1) * 16 + rx) = _mm_cvtsi128_si64(_mm256_castsi256_si128(row1));
      *(int64_t*)(dst + (ry + 2) * 16 + rx) = _mm_cvtsi128_si64(_mm256_castsi256_si128(row2));
      *(int64_t*)(dst + (ry + 3) * 16 + rx) = _mm_cvtsi128_si64(_mm256_castsi256_si128(row3));
      *(int64_t*)(dst + (ry + 4) * 16 + rx) = _mm_cvtsi128_si64(_mm256_castsi256_si128(row4));
      *(int64_t*)(dst + (ry + 5) * 16 + rx) = _mm_cvtsi128_si64(_mm256_castsi256_si128(row5));
      *(int64_t*)(dst + (ry + 6) * 16 + rx) = _mm_cvtsi128_si64(_mm256_castsi256_si128(row6));
      *(int64_t*)(dst + (ry + 7) * 16 + rx) = _mm_cvtsi128_si64(_mm256_castsi256_si128(row7));

      *(int64_t*)(dst + (ry + 8) * 16 + rx) = _mm_cvtsi128_si64(_mm256_extracti128_si256(row0, 1));
      *(int64_t*)(dst + (ry + 9) * 16 + rx) = _mm_cvtsi128_si64(_mm256_extracti128_si256(row1, 1));
      *(int64_t*)(dst + (ry + 10) * 16 + rx) = _mm_cvtsi128_si64(_mm256_extracti128_si256(row2, 1));
      *(int64_t*)(dst + (ry + 11) * 16 + rx) = _mm_cvtsi128_si64(_mm256_extracti128_si256(row3, 1));
      *(int64_t*)(dst + (ry + 12) * 16 + rx) = _mm_cvtsi128_si64(_mm256_extracti128_si256(row4, 1));
      *(int64_t*)(dst + (ry + 13) * 16 + rx) = _mm_cvtsi128_si64(_mm256_extracti128_si256(row5, 1));
      *(int64_t*)(dst + (ry + 14) * 16 + rx) = _mm_cvtsi128_si64(_mm256_extracti128_si256(row6, 1));
      *(int64_t*)(dst + (ry + 15) * 16 + rx) = _mm_cvtsi128_si64(_mm256_extracti128_si256(row7, 1));
    } else {

      //Set ry for the lower half of the block
      int rx = 0;
      int ry = y;

      row0 = _mm256_permute4x64_epi64(row0, _MM_SHUFFLE(3,1,2,0));
      row1 = _mm256_permute4x64_epi64(row1, _MM_SHUFFLE(2,0,3,1));
      row2 = _mm256_permute4x64_epi64(row2, _MM_SHUFFLE(3,1,2,0));
      row3 = _mm256_permute4x64_epi64(row3, _MM_SHUFFLE(2,0,3,1));
      row4 = _mm256_permute4x64_epi64(row4, _MM_SHUFFLE(3,1,2,0));
      row5 = _mm256_permute4x64_epi64(row5, _MM_SHUFFLE(2,0,3,1));
      row6 = _mm256_permute4x64_epi64(row6, _MM_SHUFFLE(3,1,2,0));
      row7 = _mm256_permute4x64_epi64(row7, _MM_SHUFFLE(2,0,3,1));

      _mm_storeu_si128((__m128i*)(dst + (ry + 0) * 16 + rx), _mm256_castsi256_si128(row0));
      _mm_storeu_si128((__m128i*)(dst + (ry + 1) * 16 + rx), _mm256_castsi256_si128(row1));
      _mm_storeu_si128((__m128i*)(dst + (ry + 2) * 16 + rx), _mm256_castsi256_si128(row2));
      _mm_storeu_si128((__m128i*)(dst + (ry + 3) * 16 + rx), _mm256_castsi256_si128(row3));
      _mm_storeu_si128((__m128i*)(dst + (ry + 4) * 16 + rx), _mm256_castsi256_si128(row4));
      _mm_storeu_si128((__m128i*)(dst + (ry + 5) * 16 + rx), _mm256_castsi256_si128(row5));
      _mm_storeu_si128((__m128i*)(dst + (ry + 6) * 16 + rx), _mm256_castsi256_si128(row6));
      _mm_storeu_si128((__m128i*)(dst + (ry + 7) * 16 + rx), _mm256_castsi256_si128(row7));
    }
  }
}

 /**
 * \brief Linear interpolation for NxN blocks 16x16 and larger. Writes filtered NxN block to dst.
 * \param dst           Destination buffer
 * \param ref_main      Reference pixels
 * \param sample_disp   Sample displacement per row
 * \param vertical_mode Mode direction, true if vertical
 * \param width         Block width
 */
static void filter_NxN_avx2(kvz_pixel *dst, const kvz_pixel *ref_main, int sample_disp, bool vertical_mode, int width){
  for (int y = 0; y < width; y += 8) {
    for (int x = 0; x < width; x += 16) {
      __m256i row0 = filter_16x1_avx2(ref_main, (y + 1) * sample_disp, x);
      __m256i row1 = filter_16x1_avx2(ref_main, (y + 2) * sample_disp, x);
      __m256i row2 = filter_16x1_avx2(ref_main, (y + 3) * sample_disp, x);
      __m256i row3 = filter_16x1_avx2(ref_main, (y + 4) * sample_disp, x);
      __m256i row4 = filter_16x1_avx2(ref_main, (y + 5) * sample_disp, x);
      __m256i row5 = filter_16x1_avx2(ref_main, (y + 6) * sample_disp, x);
      __m256i row6 = filter_16x1_avx2(ref_main, (y + 7) * sample_disp, x);
      __m256i row7 = filter_16x1_avx2(ref_main, (y + 8) * sample_disp, x);

      //Transpose if horizontal mode
      if (!vertical_mode) {
        __m256i q0 = _mm256_unpacklo_epi8(row0, row1);
        __m256i q1 = _mm256_unpacklo_epi8(row2, row3);
        __m256i q2 = _mm256_unpacklo_epi8(row4, row5);
        __m256i q3 = _mm256_unpacklo_epi8(row6, row7);

        __m256i h0 = _mm256_unpacklo_epi16(q0, q1);
        __m256i h1 = _mm256_unpacklo_epi16(q2, q3);
        __m256i h2 = _mm256_unpackhi_epi16(q0, q1);
        __m256i h3 = _mm256_unpackhi_epi16(q2, q3);

        __m256i temp0 = _mm256_unpacklo_epi32(h0, h1);
        __m256i temp1 = _mm256_unpackhi_epi32(h0, h1);
        __m256i temp2 = _mm256_unpacklo_epi32(h2, h3);
        __m256i temp3 = _mm256_unpackhi_epi32(h2, h3);

        row0 = _mm256_unpacklo_epi64(temp0, temp0);
        row1 = _mm256_unpackhi_epi64(temp0, temp0);
        row2 = _mm256_unpacklo_epi64(temp1, temp1);
        row3 = _mm256_unpackhi_epi64(temp1, temp1);
        row4 = _mm256_unpacklo_epi64(temp2, temp2);
        row5 = _mm256_unpackhi_epi64(temp2, temp2);
        row6 = _mm256_unpacklo_epi64(temp3, temp3);
        row7 = _mm256_unpackhi_epi64(temp3, temp3);

        //x and y must be flipped due to transpose
        int rx = y;
        int ry = x;

        *(int64_t*)(dst + (ry + 0) * width + rx) = _mm_cvtsi128_si64(_mm256_castsi256_si128(row0));
        *(int64_t*)(dst + (ry + 1) * width + rx) = _mm_cvtsi128_si64(_mm256_castsi256_si128(row1));
        *(int64_t*)(dst + (ry + 2) * width + rx) = _mm_cvtsi128_si64(_mm256_castsi256_si128(row2));
        *(int64_t*)(dst + (ry + 3) * width + rx) = _mm_cvtsi128_si64(_mm256_castsi256_si128(row3));
        *(int64_t*)(dst + (ry + 4) * width + rx) = _mm_cvtsi128_si64(_mm256_castsi256_si128(row4));
        *(int64_t*)(dst + (ry + 5) * width + rx) = _mm_cvtsi128_si64(_mm256_castsi256_si128(row5));
        *(int64_t*)(dst + (ry + 6) * width + rx) = _mm_cvtsi128_si64(_mm256_castsi256_si128(row6));
        *(int64_t*)(dst + (ry + 7) * width + rx) = _mm_cvtsi128_si64(_mm256_castsi256_si128(row7));

        *(int64_t*)(dst + (ry + 8) * width + rx) = _mm_cvtsi128_si64(_mm256_extracti128_si256(row0, 1));
        *(int64_t*)(dst + (ry + 9) * width + rx) = _mm_cvtsi128_si64(_mm256_extracti128_si256(row1, 1));
        *(int64_t*)(dst + (ry + 10) * width + rx) = _mm_cvtsi128_si64(_mm256_extracti128_si256(row2, 1));
        *(int64_t*)(dst + (ry + 11) * width + rx) = _mm_cvtsi128_si64(_mm256_extracti128_si256(row3, 1));
        *(int64_t*)(dst + (ry + 12) * width + rx) = _mm_cvtsi128_si64(_mm256_extracti128_si256(row4, 1));
        *(int64_t*)(dst + (ry + 13) * width + rx) = _mm_cvtsi128_si64(_mm256_extracti128_si256(row5, 1));
        *(int64_t*)(dst + (ry + 14) * width + rx) = _mm_cvtsi128_si64(_mm256_extracti128_si256(row6, 1));
        *(int64_t*)(dst + (ry + 15) * width + rx) = _mm_cvtsi128_si64(_mm256_extracti128_si256(row7, 1));
      } else {

        //Move all filtered pixels to the lower lane to reduce memory accesses
        row0 = _mm256_permute4x64_epi64(row0, _MM_SHUFFLE(3,1,2,0));
        row1 = _mm256_permute4x64_epi64(row1, _MM_SHUFFLE(2,0,3,1));
        row2 = _mm256_permute4x64_epi64(row2, _MM_SHUFFLE(3,1,2,0));
        row3 = _mm256_permute4x64_epi64(row3, _MM_SHUFFLE(2,0,3,1));
        row4 = _mm256_permute4x64_epi64(row4, _MM_SHUFFLE(3,1,2,0));
        row5 = _mm256_permute4x64_epi64(row5, _MM_SHUFFLE(2,0,3,1));
        row6 = _mm256_permute4x64_epi64(row6, _MM_SHUFFLE(3,1,2,0));
        row7 = _mm256_permute4x64_epi64(row7, _MM_SHUFFLE(2,0,3,1));

        _mm_storeu_si128((__m128i*)(dst + (y + 0) * width + x), _mm256_castsi256_si128(row0));
        _mm_storeu_si128((__m128i*)(dst + (y + 1) * width + x), _mm256_castsi256_si128(row1));
        _mm_storeu_si128((__m128i*)(dst + (y + 2) * width + x), _mm256_castsi256_si128(row2));
        _mm_storeu_si128((__m128i*)(dst + (y + 3) * width + x), _mm256_castsi256_si128(row3));
        _mm_storeu_si128((__m128i*)(dst + (y + 4) * width + x), _mm256_castsi256_si128(row4));
        _mm_storeu_si128((__m128i*)(dst + (y + 5) * width + x), _mm256_castsi256_si128(row5));
        _mm_storeu_si128((__m128i*)(dst + (y + 6) * width + x), _mm256_castsi256_si128(row6));
        _mm_storeu_si128((__m128i*)(dst + (y + 7) * width + x), _mm256_castsi256_si128(row7));
      }
    }
  }
}

 /**
 * \brief Generage angular predictions.
 * \param log2_width    Log2 of width, range 2..5.
 * \param intra_mode    Angular mode in range 2..34.
 * \param in_ref_above  Pointer to -1 index of above reference, length=width*2+1.
 * \param in_ref_left   Pointer to -1 index of left reference, length=width*2+1.
 * \param dst           Buffer of size width*width.
 */
static void kvz_angular_pred_avx2(
  const int_fast8_t log2_width,
  const int_fast8_t intra_mode,
  const kvz_pixel *const in_ref_above,
  const kvz_pixel *const in_ref_left,
  kvz_pixel *const dst)
{
  assert(log2_width >= 2 && log2_width <= 5);
  assert(intra_mode >= 2 && intra_mode <= 34);

  static const int8_t modedisp2sampledisp[9] = { 0, 2, 5, 9, 13, 17, 21, 26, 32 };
  static const int16_t modedisp2invsampledisp[9] = { 0, 4096, 1638, 910, 630, 482, 390, 315, 256 }; // (256 * 32) / sampledisp

                                                    // Temporary buffer for modes 11-25.
                                                    // It only needs to be big enough to hold indices from -width to width-1.
  kvz_pixel tmp_ref[2 * 32];
  const int_fast8_t width = 1 << log2_width;

  // Whether to swap references to always project on the left reference row.
  const bool vertical_mode = intra_mode >= 18;
  // Modes distance to horizontal or vertical mode.
  const int_fast8_t mode_disp = vertical_mode ? intra_mode - 26 : 10 - intra_mode;
  // Sample displacement per column in fractions of 32.
  const int_fast8_t sample_disp = (mode_disp < 0 ? -1 : 1) * modedisp2sampledisp[abs(mode_disp)];

  // Pointer for the reference we are interpolating from.
  const kvz_pixel *ref_main;
  // Pointer for the other reference.
  const kvz_pixel *ref_side;

  // Set ref_main and ref_side such that, when indexed with 0, they point to
  // index 0 in block coordinates.
  if (sample_disp < 0) {
    // Negative sample_disp means, we need to use both references.

    ref_side = (vertical_mode ? in_ref_left : in_ref_above) + 1;
    ref_main = (vertical_mode ? in_ref_above : in_ref_left) + 1;

    // Move the reference pixels to start from the middle to the later half of
    // the tmp_ref, so there is room for negative indices.
    for (int_fast8_t x = -1; x < width; ++x) {
      tmp_ref[x + width] = ref_main[x];
    }
    // Get a pointer to block index 0 in tmp_ref.
    ref_main = tmp_ref + width;

    // Extend the side reference to the negative indices of main reference.
    int_fast32_t col_sample_disp = 128; // rounding for the ">> 8"
    int_fast16_t inv_abs_sample_disp = modedisp2invsampledisp[abs(mode_disp)];
    int_fast8_t most_negative_index = (width * sample_disp) >> 5;
    for (int_fast8_t x = -2; x >= most_negative_index; --x) {
      col_sample_disp += inv_abs_sample_disp;
      int_fast8_t side_index = col_sample_disp >> 8;
      tmp_ref[x + width] = ref_side[side_index - 1];
    }
  }
  else {
    // sample_disp >= 0 means we don't need to refer to negative indices,
    // which means we can just use the references as is.
    ref_main = (vertical_mode ? in_ref_above : in_ref_left) + 1;
    ref_side = (vertical_mode ? in_ref_left : in_ref_above) + 1;
  }


  // The mode is not horizontal or vertical, we have to do interpolation.
  switch (width) {
    case 4:
      filter_4x4_avx2(dst, ref_main, sample_disp, vertical_mode);
      break;
    case 8:
      filter_8x8_avx2(dst, ref_main, sample_disp, vertical_mode);
      break;
    case 16:
      filter_16x16_avx2(dst, ref_main, sample_disp, vertical_mode);
      break;
    default:
      filter_NxN_avx2(dst, ref_main, sample_disp, vertical_mode, width);
      break;
  }
}

/**
 * \brief Generate planar prediction.
 * \param log2_width    Log2 of width, range 2..5.
 * \param in_ref_above  Pointer to -1 index of above reference, length=width*2+1.
 * \param in_ref_left   Pointer to -1 index of left reference, length=width*2+1.
 * \param dst           Buffer of size width*width.
 */
static void kvz_intra_pred_planar_avx2(
  const int_fast8_t log2_width,
  const kvz_pixel *const ref_top,
  const kvz_pixel *const ref_left,
  kvz_pixel *const dst)
{
  assert(log2_width >= 2 && log2_width <= 5);

  const int_fast8_t width = 1 << log2_width;
  const kvz_pixel top_right = ref_top[width + 1];
  const kvz_pixel bottom_left = ref_left[width + 1];

  if (log2_width > 2) {
    
    __m128i v_width = _mm_set1_epi16(width);
    __m128i v_top_right = _mm_set1_epi16(top_right);
    __m128i v_bottom_left = _mm_set1_epi16(bottom_left);

    for (int y = 0; y < width; ++y) {

      __m128i x_plus_1 = _mm_setr_epi16(-7, -6, -5, -4, -3, -2, -1, 0);
      __m128i v_ref_left = _mm_set1_epi16(ref_left[y + 1]);
      __m128i y_plus_1 = _mm_set1_epi16(y + 1);

      for (int x = 0; x < width; x += 8) {
        x_plus_1 = _mm_add_epi16(x_plus_1, _mm_set1_epi16(8));
        __m128i v_ref_top = _mm_loadl_epi64((__m128i*)&(ref_top[x + 1]));
        v_ref_top = _mm_cvtepu8_epi16(v_ref_top);

        __m128i hor = _mm_add_epi16(_mm_mullo_epi16(_mm_sub_epi16(v_width, x_plus_1), v_ref_left), _mm_mullo_epi16(x_plus_1, v_top_right));
        __m128i ver = _mm_add_epi16(_mm_mullo_epi16(_mm_sub_epi16(v_width, y_plus_1), v_ref_top), _mm_mullo_epi16(y_plus_1, v_bottom_left));

        //dst[y * width + x] = ho

        __m128i chunk = _mm_srli_epi16(_mm_add_epi16(_mm_add_epi16(ver, hor), v_width), (log2_width + 1));
        chunk = _mm_packus_epi16(chunk, chunk);
        _mm_storel_epi64((__m128i*)&(dst[y * width + x]), chunk);
      }
    }
  } else {
    // Only if log2_width == 2 <=> width == 4
    assert(width == 4);
    const __m128i rl_shufmask = _mm_setr_epi32(0x04040404, 0x05050505,
                                               0x06060606, 0x07070707);

    const __m128i xp1   = _mm_set1_epi32  (0x04030201);
    const __m128i yp1   = _mm_shuffle_epi8(xp1,   rl_shufmask);

    const __m128i rdist = _mm_set1_epi32  (0x00010203);
    const __m128i bdist = _mm_shuffle_epi8(rdist, rl_shufmask);

    const __m128i wid16 = _mm_set1_epi16  (width);
    const __m128i tr    = _mm_set1_epi8   (top_right);
    const __m128i bl    = _mm_set1_epi8   (bottom_left);

    uint32_t rt14    = *(const uint32_t *)(ref_top  + 1);
    uint32_t rl14    = *(const uint32_t *)(ref_left + 1);
    uint64_t rt14_64 = (uint64_t)rt14;
    uint64_t rl14_64 = (uint64_t)rl14;
    uint64_t rtl14   = rt14_64 | (rl14_64 << 32);

    __m128i rtl_v    = _mm_cvtsi64_si128   (rtl14);
    __m128i rt       = _mm_broadcastd_epi32(rtl_v);
    __m128i rl       = _mm_shuffle_epi8    (rtl_v,    rl_shufmask);

    __m128i rtrl_l   = _mm_unpacklo_epi8   (rt,       rl);
    __m128i rtrl_h   = _mm_unpackhi_epi8   (rt,       rl);

    __m128i bdrd_l   = _mm_unpacklo_epi8   (bdist,    rdist);
    __m128i bdrd_h   = _mm_unpackhi_epi8   (bdist,    rdist);

    __m128i hvs_lo   = _mm_maddubs_epi16   (rtrl_l,   bdrd_l);
    __m128i hvs_hi   = _mm_maddubs_epi16   (rtrl_h,   bdrd_h);

    __m128i xp1yp1_l = _mm_unpacklo_epi8   (xp1,      yp1);
    __m128i xp1yp1_h = _mm_unpackhi_epi8   (xp1,      yp1);
    __m128i trbl_lh  = _mm_unpacklo_epi8   (tr,       bl);

    __m128i addend_l = _mm_maddubs_epi16   (trbl_lh,  xp1yp1_l);
    __m128i addend_h = _mm_maddubs_epi16   (trbl_lh,  xp1yp1_h);

            addend_l = _mm_add_epi16       (addend_l, wid16);
            addend_h = _mm_add_epi16       (addend_h, wid16);

    __m128i sum_l    = _mm_add_epi16       (hvs_lo,   addend_l);
    __m128i sum_h    = _mm_add_epi16       (hvs_hi,   addend_h);

    // Shift right by log2_width + 1
    __m128i sum_l_t  = _mm_srli_epi16      (sum_l,    3);
    __m128i sum_h_t  = _mm_srli_epi16      (sum_h,    3);
    __m128i result   = _mm_packus_epi16    (sum_l_t,  sum_h_t);
    _mm_storeu_si128((__m128i *)dst, result);
  }
}

// Calculate the DC value for a 4x4 block. The algorithm uses slightly
// different addends, multipliers etc for different pixels in the block,
// but for a fixed-size implementation one vector wide, all the weights,
// addends etc can be preinitialized for each position.
static void pred_filtered_dc_4x4(const uint8_t *ref_top,
                                 const uint8_t *ref_left,
                                       uint8_t *out_block)
{
  const uint32_t rt_u32 = *(const uint32_t *)(ref_top  + 1);
  const uint32_t rl_u32 = *(const uint32_t *)(ref_left + 1);

  const __m128i zero    = _mm_setzero_si128();
  const __m128i twos    = _mm_set1_epi8(2);

  // Hack. Move 4 u8's to bit positions 0, 64, 128 and 192 in two regs, to
  // expand them to 16 bits sort of "for free". Set highest bits on all the
  // other bytes in vectors to zero those bits in the result vector.
  const __m128i rl_shuf_lo = _mm_setr_epi32(0x80808000, 0x80808080,
                                            0x80808001, 0x80808080);
  const __m128i rl_shuf_hi = _mm_add_epi8  (rl_shuf_lo, twos);

  // Every second multiplier is 1, because we want maddubs to calculate
  // a + bc = 1 * a + bc (actually 2 + bc). We need to fill a vector with
  // ((u8)2)'s for other stuff anyway, so that can also be used here.
  const __m128i mult_lo = _mm_setr_epi32(0x01030102, 0x01030103,
                                         0x01040103, 0x01040104);
  const __m128i mult_hi = _mm_setr_epi32(0x01040103, 0x01040104,
                                         0x01040103, 0x01040104);
  __m128i four         = _mm_cvtsi32_si128  (4);
  __m128i rt           = _mm_cvtsi32_si128  (rt_u32);
  __m128i rl           = _mm_cvtsi32_si128  (rl_u32);
  __m128i rtrl         = _mm_unpacklo_epi32 (rt, rl);

  __m128i sad0         = _mm_sad_epu8       (rtrl, zero);
  __m128i sad1         = _mm_shuffle_epi32  (sad0, _MM_SHUFFLE(1, 0, 3, 2));
  __m128i sad2         = _mm_add_epi64      (sad0, sad1);
  __m128i sad3         = _mm_add_epi64      (sad2, four);

  __m128i dc_64        = _mm_srli_epi64     (sad3, 3);
  __m128i dc_8         = _mm_broadcastb_epi8(dc_64);

  __m128i rl_lo        = _mm_shuffle_epi8   (rl, rl_shuf_lo);
  __m128i rl_hi        = _mm_shuffle_epi8   (rl, rl_shuf_hi);

  __m128i rt_lo        = _mm_unpacklo_epi8  (rt, zero);
  __m128i rt_hi        = zero;

  __m128i dc_addend    = _mm_unpacklo_epi8(dc_8, twos);

  __m128i dc_multd_lo  = _mm_maddubs_epi16(dc_addend,    mult_lo);
  __m128i dc_multd_hi  = _mm_maddubs_epi16(dc_addend,    mult_hi);

  __m128i rl_rt_lo     = _mm_add_epi16    (rl_lo,        rt_lo);
  __m128i rl_rt_hi     = _mm_add_epi16    (rl_hi,        rt_hi);

  __m128i res_lo       = _mm_add_epi16    (dc_multd_lo,  rl_rt_lo);
  __m128i res_hi       = _mm_add_epi16    (dc_multd_hi,  rl_rt_hi);

          res_lo       = _mm_srli_epi16   (res_lo,       2);
          res_hi       = _mm_srli_epi16   (res_hi,       2);

  __m128i final        = _mm_packus_epi16 (res_lo,       res_hi);
  _mm_storeu_si128((__m128i *)out_block, final);
}

static void pred_filtered_dc_8x8(const uint8_t *ref_top,
                                 const uint8_t *ref_left,
                                       uint8_t *out_block)
{
  const uint64_t rt_u64 = *(const uint64_t *)(ref_top  + 1);
  const uint64_t rl_u64 = *(const uint64_t *)(ref_left + 1);

  const __m128i zero128 = _mm_setzero_si128();
  const __m256i twos    = _mm256_set1_epi8(2);

  // DC multiplier is 2 at (0, 0), 3 at (*, 0) and (0, *), and 4 at (*, *).
  // There is a constant addend of 2 on each pixel, use values from the twos
  // register and multipliers of 1 for that, to use maddubs for an (a*b)+c
  // operation.
  const __m256i mult_up_lo = _mm256_setr_epi32(0x01030102, 0x01030103,
                                               0x01030103, 0x01030103,
                                               0x01040103, 0x01040104,
                                               0x01040104, 0x01040104);

  // The 6 lowest rows have same multipliers, also the DC values and addends
  // are the same so this works for all of those
  const __m256i mult_rest  = _mm256_permute4x64_epi64(mult_up_lo, _MM_SHUFFLE(3, 2, 3, 2));

  // Every 8-pixel row starts with the next pixel of ref_left. Along with
  // doing the shuffling, also expand u8->u16, ie. move bytes 0 and 1 from
  // ref_left to bit positions 0 and 128 in rl_up_lo, 2 and 3 to rl_up_hi,
  // etc. The places to be zeroed out are 0x80 instead of the usual 0xff,
  // because this allows us to form new masks on the fly by adding 0x02-bytes
  // to this mask and still retain the highest bits as 1 where things should
  // be zeroed out.
  const __m256i rl_shuf_up_lo = _mm256_setr_epi32(0x80808000, 0x80808080,
                                                  0x80808080, 0x80808080,
                                                  0x80808001, 0x80808080,
                                                  0x80808080, 0x80808080);
  // And don't waste memory or architectural regs, hope these instructions
  // will be placed in between the shuffles by the compiler to only use one
  // register for the shufmasks, and executed way ahead of time because their
  // regs can be renamed.
  const __m256i rl_shuf_up_hi = _mm256_add_epi8 (rl_shuf_up_lo, twos);
  const __m256i rl_shuf_dn_lo = _mm256_add_epi8 (rl_shuf_up_hi, twos);
  const __m256i rl_shuf_dn_hi = _mm256_add_epi8 (rl_shuf_dn_lo, twos);

  __m128i eight         = _mm_cvtsi32_si128     (8);
  __m128i rt            = _mm_cvtsi64_si128     (rt_u64);
  __m128i rl            = _mm_cvtsi64_si128     (rl_u64);
  __m128i rtrl          = _mm_unpacklo_epi64    (rt, rl);

  __m128i sad0          = _mm_sad_epu8          (rtrl, zero128);
  __m128i sad1          = _mm_shuffle_epi32     (sad0, _MM_SHUFFLE(1, 0, 3, 2));
  __m128i sad2          = _mm_add_epi64         (sad0, sad1);
  __m128i sad3          = _mm_add_epi64         (sad2, eight);

  __m128i dc_64         = _mm_srli_epi64        (sad3, 4);
  __m256i dc_8          = _mm256_broadcastb_epi8(dc_64);

  __m256i dc_addend     = _mm256_unpacklo_epi8  (dc_8, twos);

  __m256i dc_up_lo      = _mm256_maddubs_epi16  (dc_addend, mult_up_lo);
  __m256i dc_rest       = _mm256_maddubs_epi16  (dc_addend, mult_rest);

  // rt_dn is all zeros, as is rt_up_hi. This'll get us the rl and rt parts
  // in A|B, C|D order instead of A|C, B|D that could be packed into abcd
  // order, so these need to be permuted before adding to the weighed DC
  // values.
  __m256i rt_up_lo      = _mm256_cvtepu8_epi16   (rt);

  __m256i rlrlrlrl      = _mm256_broadcastq_epi64(rl);
  __m256i rl_up_lo      = _mm256_shuffle_epi8    (rlrlrlrl, rl_shuf_up_lo);

  // Everything ref_top is zero except on the very first row
  __m256i rt_rl_up_hi   = _mm256_shuffle_epi8    (rlrlrlrl, rl_shuf_up_hi);
  __m256i rt_rl_dn_lo   = _mm256_shuffle_epi8    (rlrlrlrl, rl_shuf_dn_lo);
  __m256i rt_rl_dn_hi   = _mm256_shuffle_epi8    (rlrlrlrl, rl_shuf_dn_hi);

  __m256i rt_rl_up_lo   = _mm256_add_epi16       (rt_up_lo, rl_up_lo);

  __m256i rt_rl_up_lo_2 = _mm256_permute2x128_si256(rt_rl_up_lo, rt_rl_up_hi, 0x20);
  __m256i rt_rl_up_hi_2 = _mm256_permute2x128_si256(rt_rl_up_lo, rt_rl_up_hi, 0x31);
  __m256i rt_rl_dn_lo_2 = _mm256_permute2x128_si256(rt_rl_dn_lo, rt_rl_dn_hi, 0x20);
  __m256i rt_rl_dn_hi_2 = _mm256_permute2x128_si256(rt_rl_dn_lo, rt_rl_dn_hi, 0x31);

  __m256i up_lo = _mm256_add_epi16(rt_rl_up_lo_2, dc_up_lo);
  __m256i up_hi = _mm256_add_epi16(rt_rl_up_hi_2, dc_rest);
  __m256i dn_lo = _mm256_add_epi16(rt_rl_dn_lo_2, dc_rest);
  __m256i dn_hi = _mm256_add_epi16(rt_rl_dn_hi_2, dc_rest);

          up_lo = _mm256_srli_epi16(up_lo, 2);
          up_hi = _mm256_srli_epi16(up_hi, 2);
          dn_lo = _mm256_srli_epi16(dn_lo, 2);
          dn_hi = _mm256_srli_epi16(dn_hi, 2);

  __m256i res_up = _mm256_packus_epi16(up_lo, up_hi);
  __m256i res_dn = _mm256_packus_epi16(dn_lo, dn_hi);

  _mm256_storeu_si256(((__m256i *)out_block) + 0, res_up);
  _mm256_storeu_si256(((__m256i *)out_block) + 1, res_dn);
}

static INLINE __m256i cvt_u32_si256(const uint32_t u)
{
  const __m256i zero = _mm256_setzero_si256();
  return _mm256_insert_epi32(zero, u, 0);
}

static void pred_filtered_dc_16x16(const uint8_t *ref_top,
                                   const uint8_t *ref_left,
                                         uint8_t *out_block)
{
  const __m128i rt_128 = _mm_loadu_si128((const __m128i *)(ref_top  + 1));
  const __m128i rl_128 = _mm_loadu_si128((const __m128i *)(ref_left + 1));

  const __m128i zero_128 = _mm_setzero_si128();
  const __m256i zero     = _mm256_setzero_si256();
  const __m256i twos     = _mm256_set1_epi8(2);

  const __m256i mult_r0  = _mm256_setr_epi32(0x01030102, 0x01030103,
                                             0x01030103, 0x01030103,
                                             0x01030103, 0x01030103,
                                             0x01030103, 0x01030103);

  const __m256i mult_left = _mm256_set1_epi16(0x0103);

  // Leftmost bytes' blend mask, to move bytes (pixels) from the leftmost
  // column vector to the result row
  const __m256i lm8_bmask = _mm256_setr_epi32(0xff, 0, 0, 0, 0xff, 0, 0, 0);

  __m128i sixteen = _mm_cvtsi32_si128(16);
  __m128i sad0_t  = _mm_sad_epu8 (rt_128, zero_128);
  __m128i sad0_l  = _mm_sad_epu8 (rl_128, zero_128);
  __m128i sad0    = _mm_add_epi64(sad0_t, sad0_l);

  __m128i sad1    = _mm_shuffle_epi32      (sad0, _MM_SHUFFLE(1, 0, 3, 2));
  __m128i sad2    = _mm_add_epi64          (sad0, sad1);
  __m128i sad3    = _mm_add_epi64          (sad2, sixteen);

  __m128i dc_64   = _mm_srli_epi64         (sad3, 5);
  __m256i dc_8    = _mm256_broadcastb_epi8 (dc_64);

  __m256i rt      = _mm256_cvtepu8_epi16   (rt_128);
  __m256i rl      = _mm256_cvtepu8_epi16   (rl_128);

  uint8_t rl0       = *(uint8_t *)(ref_left + 1);
  __m256i rl_r0     = cvt_u32_si256((uint32_t)rl0);

  __m256i rlrt_r0   = _mm256_add_epi16(rl_r0, rt);

  __m256i dc_addend = _mm256_unpacklo_epi8(dc_8, twos);
  __m256i r0        = _mm256_maddubs_epi16(dc_addend, mult_r0);
  __m256i left_dcs  = _mm256_maddubs_epi16(dc_addend, mult_left);

          r0        = _mm256_add_epi16    (r0,       rlrt_r0);
          r0        = _mm256_srli_epi16   (r0, 2);
  __m256i r0r0      = _mm256_packus_epi16 (r0, r0);
          r0r0      = _mm256_permute4x64_epi64(r0r0, _MM_SHUFFLE(3, 1, 2, 0));

  __m256i leftmosts = _mm256_add_epi16    (left_dcs,  rl);
          leftmosts = _mm256_srli_epi16   (leftmosts, 2);

  // Contain the leftmost column's bytes in both lanes of lm_8
  __m256i lm_8      = _mm256_packus_epi16 (leftmosts, zero);
          lm_8      = _mm256_permute4x64_epi64(lm_8,  _MM_SHUFFLE(2, 0, 2, 0));

  __m256i lm8_r1    = _mm256_srli_epi32       (lm_8, 8);
  __m256i r1r1      = _mm256_blendv_epi8      (dc_8, lm8_r1, lm8_bmask);
  __m256i r0r1      = _mm256_blend_epi32      (r0r0, r1r1, 0xf0);

  _mm256_storeu_si256((__m256i *)out_block, r0r1);

  // Starts from 2 because row 0 (and row 1) is handled separately
  __m256i lm8_l     = _mm256_bsrli_epi128     (lm_8, 2);
  __m256i lm8_h     = _mm256_bsrli_epi128     (lm_8, 3);
          lm_8      = _mm256_blend_epi32      (lm8_l, lm8_h, 0xf0);

  for (uint32_t y = 2; y < 16; y += 2) {
    __m256i curr_row = _mm256_blendv_epi8 (dc_8, lm_8, lm8_bmask);
    _mm256_storeu_si256((__m256i *)(out_block + (y << 4)), curr_row);
    lm_8 = _mm256_bsrli_epi128(lm_8, 2);
  }
}

static void pred_filtered_dc_32x32(const uint8_t *ref_top,
                                   const uint8_t *ref_left,
                                         uint8_t *out_block)
{
  const __m256i rt = _mm256_loadu_si256((const __m256i *)(ref_top  + 1));
  const __m256i rl = _mm256_loadu_si256((const __m256i *)(ref_left + 1));

  const __m256i zero = _mm256_setzero_si256();
  const __m256i twos = _mm256_set1_epi8(2);

  const __m256i mult_r0lo = _mm256_setr_epi32(0x01030102, 0x01030103,
                                              0x01030103, 0x01030103,
                                              0x01030103, 0x01030103,
                                              0x01030103, 0x01030103);

  const __m256i mult_left = _mm256_set1_epi16(0x0103);
  const __m256i lm8_bmask = cvt_u32_si256    (0xff);

  const __m256i bshif_msk = _mm256_setr_epi32(0x04030201, 0x08070605,
                                              0x0c0b0a09, 0x800f0e0d,
                                              0x03020100, 0x07060504,
                                              0x0b0a0908, 0x0f0e0d0c);
  __m256i debias = cvt_u32_si256(32);
  __m256i sad0_t = _mm256_sad_epu8         (rt,     zero);
  __m256i sad0_l = _mm256_sad_epu8         (rl,     zero);
  __m256i sad0   = _mm256_add_epi64        (sad0_t, sad0_l);

  __m256i sad1   = _mm256_permute4x64_epi64(sad0,   _MM_SHUFFLE(1, 0, 3, 2));
  __m256i sad2   = _mm256_add_epi64        (sad0,   sad1);
  __m256i sad3   = _mm256_shuffle_epi32    (sad2,   _MM_SHUFFLE(1, 0, 3, 2));
  __m256i sad4   = _mm256_add_epi64        (sad2,   sad3);
  __m256i sad5   = _mm256_add_epi64        (sad4,   debias);
  __m256i dc_64  = _mm256_srli_epi64       (sad5,   6);

  __m128i dc_64_ = _mm256_castsi256_si128  (dc_64);
  __m256i dc_8   = _mm256_broadcastb_epi8  (dc_64_);

  __m256i rtlo   = _mm256_unpacklo_epi8    (rt, zero);
  __m256i rllo   = _mm256_unpacklo_epi8    (rl, zero);
  __m256i rthi   = _mm256_unpackhi_epi8    (rt, zero);
  __m256i rlhi   = _mm256_unpackhi_epi8    (rl, zero);

  __m256i dc_addend = _mm256_unpacklo_epi8 (dc_8, twos);
  __m256i r0lo   = _mm256_maddubs_epi16    (dc_addend, mult_r0lo);
  __m256i r0hi   = _mm256_maddubs_epi16    (dc_addend, mult_left);
  __m256i c0dc   = r0hi;

          r0lo   = _mm256_add_epi16        (r0lo, rtlo);
          r0hi   = _mm256_add_epi16        (r0hi, rthi);

  __m256i rlr0   = _mm256_blendv_epi8      (zero, rl, lm8_bmask);
          r0lo   = _mm256_add_epi16        (r0lo, rlr0);

          r0lo   = _mm256_srli_epi16       (r0lo, 2);
          r0hi   = _mm256_srli_epi16       (r0hi, 2);
  __m256i r0     = _mm256_packus_epi16     (r0lo, r0hi);

  _mm256_storeu_si256((__m256i *)out_block, r0);

  __m256i c0lo   = _mm256_add_epi16        (c0dc, rllo);
  __m256i c0hi   = _mm256_add_epi16        (c0dc, rlhi);
          c0lo   = _mm256_srli_epi16       (c0lo, 2);
          c0hi   = _mm256_srli_epi16       (c0hi, 2);

  __m256i c0     = _mm256_packus_epi16     (c0lo, c0hi);

  // r0 already handled!
  for (uint32_t y = 1; y < 32; y++) {
    if (y == 16) {
      c0 = _mm256_permute4x64_epi64(c0, _MM_SHUFFLE(1, 0, 3, 2));
    } else {
      c0 = _mm256_shuffle_epi8     (c0, bshif_msk);
    }
    __m256i curr_row = _mm256_blendv_epi8 (dc_8, c0, lm8_bmask);
    _mm256_storeu_si256(((__m256i *)out_block) + y, curr_row);
  }
}

/**
* \brief Generage intra DC prediction with post filtering applied.
* \param log2_width    Log2 of width, range 2..5.
* \param in_ref_above  Pointer to -1 index of above reference, length=width*2+1.
* \param in_ref_left   Pointer to -1 index of left reference, length=width*2+1.
* \param dst           Buffer of size width*width.
*/
static void kvz_intra_pred_filtered_dc_avx2(
  const int_fast8_t log2_width,
  const kvz_pixel *ref_top,
  const kvz_pixel *ref_left,
        kvz_pixel *out_block)
{
  assert(log2_width >= 2 && log2_width <= 5);
  assert(sizeof(kvz_pixel) == sizeof(uint8_t));

  if (log2_width == 2) {
    pred_filtered_dc_4x4(ref_top, ref_left, out_block);
  } else if (log2_width == 3) {
    pred_filtered_dc_8x8(ref_top, ref_left, out_block);
  } else if (log2_width == 4) {
    pred_filtered_dc_16x16(ref_top, ref_left, out_block);
  } else if (log2_width == 5) {
    pred_filtered_dc_32x32(ref_top, ref_left, out_block);
  }
}

#endif //COMPILE_INTEL_AVX2 && defined X86_64

int kvz_strategy_register_intra_avx2(void* opaque, uint8_t bitdepth)
{
  bool success = true;
#if COMPILE_INTEL_AVX2 && defined X86_64
  if (bitdepth == 8) {
    success &= kvz_strategyselector_register(opaque, "angular_pred", "avx2", 40, &kvz_angular_pred_avx2);
    success &= kvz_strategyselector_register(opaque, "intra_pred_planar", "avx2", 40, &kvz_intra_pred_planar_avx2);
    success &= kvz_strategyselector_register(opaque, "intra_pred_filtered_dc", "avx2", 40, &kvz_intra_pred_filtered_dc_avx2);
  }
#endif //COMPILE_INTEL_AVX2 && defined X86_64
  return success;
}

