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
    ref_main = &tmp_ref[width];

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
    // Unoptimized version for reference.
    for (int y = 0; y < width; ++y) {
      for (int x = 0; x < width; ++x) {
        int_fast16_t hor = (width - 1 - x) * ref_left[y + 1] + (x + 1) * top_right;
        int_fast16_t ver = (width - 1 - y) * ref_top[x + 1] + (y + 1) * bottom_left;
        dst[y * width + x] = (ver + hor + width) >> (log2_width + 1);
      }
    }
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
  }
#endif //COMPILE_INTEL_AVX2 && defined X86_64
  return success;
}
