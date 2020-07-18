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

#include "strategies/avx2/sao-avx2.h"

#if COMPILE_INTEL_AVX2
#include <immintrin.h>
#include <nmmintrin.h>

// Use a couple generic functions from here as a worst-case fallback
#include "strategies/generic/sao_shared_generics.h"
#include "strategies/avx2/avx2_common_functions.h"
#include "strategies/missing-intel-intrinsics.h"
#include "cu.h"
#include "encoder.h"
#include "encoderstate.h"
#include "kvazaar.h"
#include "sao.h"
#include "strategyselector.h"

// These optimizations are based heavily on sao-generic.c.
// Might be useful to check that if (when) this file
// is difficult to understand.

// Do the SIGN3 operation for the difference a-b
static INLINE __m256i sign3_diff_epu8(const __m256i a, const __m256i b)
{
  // Subtract 0x80 from unsigneds to compare them as signed
  const __m256i epu2epi = _mm256_set1_epi8  (0x80);
  const __m256i ones    = _mm256_set1_epi8  (0x01);

  __m256i a_signed      = _mm256_sub_epi8   (a,        epu2epi);
  __m256i b_signed      = _mm256_sub_epi8   (b,        epu2epi);

  __m256i diff          = _mm256_subs_epi8  (a_signed, b_signed);
  return                  _mm256_sign_epi8  (ones,     diff);
}

// Mapping of edge_idx values to eo-classes, 32x8b at once
static __m256i FIX_W32 calc_eo_cat(const __m256i a,
                                   const __m256i b,
                                   const __m256i c)
{
  const __m256i twos       = _mm256_set1_epi8  (0x02);
  const __m256i idx_to_cat = _mm256_setr_epi64x(0x0403000201, 0,
                                                0x0403000201, 0);

  __m256i c_a_sign         = sign3_diff_epu8    (c, a);
  __m256i c_b_sign         = sign3_diff_epu8    (c, b);

  __m256i signsum          = _mm256_add_epi8    (c_a_sign,   c_b_sign);
  __m256i eo_idx           = _mm256_add_epi8    (signsum,    twos);

  return                     _mm256_shuffle_epi8(idx_to_cat, eo_idx);
}

static INLINE __m256i srli_epi8(const __m256i  v,
                                const uint32_t shift)
{
  const uint8_t hibit_mask     = 0xff >> shift;
  const __m256i hibit_mask_256 = _mm256_set1_epi8(hibit_mask);

  __m256i v_shifted = _mm256_srli_epi32(v,         shift);
  __m256i v_masked  = _mm256_and_si256 (v_shifted, hibit_mask_256);

  return v_masked;
}

static INLINE void cvt_epu8_epi16(const __m256i  v,
                                        __m256i *res_lo,
                                        __m256i *res_hi)
{
  const __m256i zero  = _mm256_setzero_si256();
             *res_lo  = _mm256_unpacklo_epi8(v, zero);
             *res_hi  = _mm256_unpackhi_epi8(v, zero);
}

static INLINE void cvt_epi8_epi16(const __m256i  v,
                                        __m256i *res_lo,
                                        __m256i *res_hi)
{
  const __m256i zero  = _mm256_setzero_si256();
        __m256i signs = _mm256_cmpgt_epi8   (zero, v);
             *res_lo  = _mm256_unpacklo_epi8(v,    signs);
             *res_hi  = _mm256_unpackhi_epi8(v,    signs);
}

static INLINE void diff_epi8_epi16(const __m256i  a,
                                   const __m256i  b,
                                         __m256i *res_lo,
                                         __m256i *res_hi)
{
  const __m256i invmask = _mm256_set1_epi16(0xff01);

  __m256i composite_lo  = _mm256_unpacklo_epi8(a, b);
  __m256i composite_hi  = _mm256_unpackhi_epi8(a, b);

         *res_lo        = _mm256_maddubs_epi16(composite_lo, invmask);
         *res_hi        = _mm256_maddubs_epi16(composite_hi, invmask);
}

// Convert a byte-addressed mask for VPSHUFB into two word-addressed ones, for
// example:
// 7 3 6 2 5 1 4 0 => e f 6 7 c d 4 5 a b 2 3 8 9 0 1
static INLINE void cvt_shufmask_epi8_epi16(const __m256i  v,
                                                 __m256i *res_lo,
                                                 __m256i *res_hi)
{
  const __m256i zero = _mm256_setzero_si256();
  const __m256i ones = _mm256_set1_epi8(1);

  // There's no 8-bit shift, so highest bit could bleed into neighboring byte
  // if set. To avoid it, reset all sign bits with max. The only valid input
  // values for v are [0, 7] anyway and invalid places should be masked out by
  // caller, so it doesn't matter that we turn negative bytes into garbage.
  __m256i v_nonnegs  = _mm256_max_epi8  (zero,      v);
  __m256i v_lobytes  = _mm256_slli_epi32(v_nonnegs, 1);
  __m256i v_hibytes  = _mm256_add_epi8  (v_lobytes, ones);

          *res_lo    = _mm256_unpacklo_epi8(v_lobytes, v_hibytes);
          *res_hi    = _mm256_unpackhi_epi8(v_lobytes, v_hibytes);
}

// Check if all 4 dwords of v are in [-128, 127] and can be truncated to
// 8 bits each. Returns -1 if everything is fine
static INLINE uint16_t epi32v_fits_in_epi8s(const __m128i v)
{
  // Compare most significant 25 bits of SAO bands to the sign bit to assert
  // that the i32's are between -128 and 127 (only comparing 24 would fail to
  // detect values of 128...255)
  __m128i  v_ms25b = _mm_srai_epi32   (v,  7);
  __m128i  v_signs = _mm_srai_epi32   (v, 31);
  __m128i  ok_i32s = _mm_cmpeq_epi32  (v_ms25b, v_signs);
  return             _mm_movemask_epi8(ok_i32s);
}

static INLINE __m128i truncate_epi32_epi8(const __m128i v)
{
  // LSBs of each dword, the values values must fit in 8 bits anyway for
  // what this intended for (use epi32v_fits_in_epi8s to check if needed)
  const __m128i trunc_shufmask = _mm_set1_epi32  (0x0c080400);
        __m128i sbs_8          = _mm_shuffle_epi8(v, trunc_shufmask);
  return        sbs_8;
}

// Read 0-3 bytes (pixels) into uint32_t
static INLINE uint32_t load_border_bytes(const uint8_t *buf,
                                         const int32_t  start_pos,
                                         const int32_t  width_rest)
{
  uint32_t last_dword = 0;
  for (int32_t i = 0; i < width_rest; i++) {
    uint8_t  currb = buf[start_pos + i];
    uint32_t currd = ((uint32_t)currb) << (i * 8);
    last_dword |= currd;
  }
  return last_dword;
}

static INLINE void store_border_bytes(      uint8_t  *buf,
                                      const uint32_t  start_pos,
                                      const int32_t   width_rest,
                                            uint32_t  data)
{
  for (uint32_t i = 0; i < width_rest; i++) {
    uint8_t currb = data & 0xff;
    buf[start_pos + i] = currb;
    data >>= 8;
  }
}

// Mask all inexistent bytes to 0xFF for functions that count particular byte
// values, so they won't count anywhere
static INLINE __m256i gen_badbyte_mask(const __m256i db4_mask,
                                       const int32_t width_rest)
{
  const __m256i zero    = _mm256_setzero_si256();

  uint32_t last_badbytes = 0xffffffff << (width_rest << 3);
  __m256i  badbyte_mask  = _mm256_cmpeq_epi8  (db4_mask,     zero);
  return                   _mm256_insert_epi32(badbyte_mask, last_badbytes, 7);
}

// Ok, so the broadcast si128->si256 instruction only works with a memory
// source operand..
static INLINE __m256i broadcast_xmm2ymm(const __m128i v)
{
  __m256i res = _mm256_castsi128_si256 (v);
  return        _mm256_inserti128_si256(res, v, 1);
}

// Used for edge_ddistortion and band_ddistortion
static __m256i FIX_W32 calc_diff_off_delta(const __m256i diff_lo,
                                           const __m256i diff_hi,
                                           const __m256i offsets,
                                           const __m256i orig)
{
  const __m256i zero          = _mm256_setzero_si256();
  const __m256i negate_hiword = _mm256_set1_epi32(0xffff0001);

  __m256i orig_lo, orig_hi, offsets_lo, offsets_hi;

  cvt_epu8_epi16(orig,    &orig_lo,    &orig_hi);
  cvt_epi8_epi16(offsets, &offsets_lo, &offsets_hi);

  __m256i offsets_0_lo = _mm256_cmpeq_epi16   (offsets_lo,   zero);
  __m256i offsets_0_hi = _mm256_cmpeq_epi16   (offsets_hi,   zero);

  __m256i delta_lo     = _mm256_sub_epi16     (diff_lo,      offsets_lo);
  __m256i delta_hi     = _mm256_sub_epi16     (diff_hi,      offsets_hi);

  __m256i diff_lo_m    = _mm256_andnot_si256  (offsets_0_lo, diff_lo);
  __m256i diff_hi_m    = _mm256_andnot_si256  (offsets_0_hi, diff_hi);
  __m256i delta_lo_m   = _mm256_andnot_si256  (offsets_0_lo, delta_lo);
  __m256i delta_hi_m   = _mm256_andnot_si256  (offsets_0_hi, delta_hi);

  __m256i dd0_lo       = _mm256_unpacklo_epi16(delta_lo_m,   diff_lo_m);
  __m256i dd0_hi       = _mm256_unpackhi_epi16(delta_lo_m,   diff_lo_m);
  __m256i dd1_lo       = _mm256_unpacklo_epi16(delta_hi_m,   diff_hi_m);
  __m256i dd1_hi       = _mm256_unpackhi_epi16(delta_hi_m,   diff_hi_m);

  __m256i dd0_lo_n     = _mm256_sign_epi16    (dd0_lo,       negate_hiword);
  __m256i dd0_hi_n     = _mm256_sign_epi16    (dd0_hi,       negate_hiword);
  __m256i dd1_lo_n     = _mm256_sign_epi16    (dd1_lo,       negate_hiword);
  __m256i dd1_hi_n     = _mm256_sign_epi16    (dd1_hi,       negate_hiword);

  __m256i sum0_lo      = _mm256_madd_epi16    (dd0_lo,       dd0_lo_n);
  __m256i sum0_hi      = _mm256_madd_epi16    (dd0_hi,       dd0_hi_n);
  __m256i sum1_lo      = _mm256_madd_epi16    (dd1_lo,       dd1_lo_n);
  __m256i sum1_hi      = _mm256_madd_epi16    (dd1_hi,       dd1_hi_n);

  __m256i sum0         = _mm256_add_epi32     (sum0_lo,      sum0_hi);
  __m256i sum1         = _mm256_add_epi32     (sum1_lo,      sum1_hi);
  return                 _mm256_add_epi32     (sum0,         sum1);
}

static INLINE __m256i FIX_W32 do_one_edge_ymm(const __m256i a,
                                              const __m256i b,
                                              const __m256i c,
                                              const __m256i orig,
                                              const __m256i badbyte_mask,
                                              const __m256i offsets_256)
{
  __m256i eo_cat = calc_eo_cat(a, b, c);
          eo_cat = _mm256_or_si256    (eo_cat,      badbyte_mask);
  __m256i offset = _mm256_shuffle_epi8(offsets_256, eo_cat);

  __m256i offset_lo, offset_hi;
  cvt_epi8_epi16(offset, &offset_lo, &offset_hi);

  __m256i diff_lo, diff_hi;
  diff_epi8_epi16(orig, c, &diff_lo, &diff_hi);

  return calc_diff_off_delta(diff_lo, diff_hi, offset, orig);
}

static int32_t sao_edge_ddistortion_avx2(const kvz_pixel *orig_data,
                                         const kvz_pixel *rec_data,
                                               int32_t    block_width,
                                               int32_t    block_height,
                                               int32_t    eo_class,
                                         const int32_t    offsets[NUM_SAO_EDGE_CATEGORIES])
{
  int32_t y, x;
  vector2d_t a_ofs = g_sao_edge_offsets[eo_class][0];
  vector2d_t b_ofs = g_sao_edge_offsets[eo_class][1];

   int32_t scan_width = block_width -   2;
  uint32_t width_db32 = scan_width  & ~31;
  uint32_t width_db4  = scan_width  &  ~3;
  uint32_t width_rest = scan_width  &   3;

  // Form the load&store mask
  const __m256i wdb4_256      = _mm256_set1_epi32 (width_db4 & 31);
  const __m256i indexes       = _mm256_setr_epi32 (3, 7, 11, 15, 19, 23, 27, 31);
  const __m256i db4_mask      = _mm256_cmpgt_epi32(wdb4_256, indexes);

  const __m256i zero          = _mm256_setzero_si256();

  __m128i offsets03 = _mm_loadu_si128((const __m128i *)offsets);
  __m128i offsets4  = _mm_cvtsi32_si128(offsets[4]);

  uint16_t offsets_ok = epi32v_fits_in_epi8s(offsets03) &
                        epi32v_fits_in_epi8s(offsets4);

  assert(NUM_SAO_EDGE_CATEGORIES == 5);

  if (offsets_ok != 0xffff) {
    return sao_edge_ddistortion_generic(orig_data,
                                        rec_data,
                                        block_width,
                                        block_height,
                                        eo_class,
                                        offsets);
  }

  __m128i offsets03_8b = truncate_epi32_epi8(offsets03);
  __m128i offsets4_8b  = truncate_epi32_epi8(offsets4);
  __m128i offsets_8b   = _mm_unpacklo_epi32 (offsets03_8b, offsets4_8b);
  __m256i offsets_256  = broadcast_xmm2ymm  (offsets_8b);

  __m256i sum = _mm256_setzero_si256();
  for (y = 1; y < block_height - 1; y++) {
    for (x = 1; x < width_db32 + 1; x += 32) {
      uint32_t c_pos =  y            * block_width + x;
      uint32_t a_pos = (y + a_ofs.y) * block_width + x + a_ofs.x;
      uint32_t b_pos = (y + b_ofs.y) * block_width + x + b_ofs.x;

      __m256i a      = _mm256_loadu_si256((const __m256i *)(rec_data  + a_pos));
      __m256i b      = _mm256_loadu_si256((const __m256i *)(rec_data  + b_pos));
      __m256i c      = _mm256_loadu_si256((const __m256i *)(rec_data  + c_pos));
      __m256i orig   = _mm256_loadu_si256((const __m256i *)(orig_data + c_pos));

      __m256i curr   = do_one_edge_ymm(a, b, c, orig, zero, offsets_256);
              sum    = _mm256_add_epi32(sum, curr);
    }
    if (scan_width > width_db32) {
      const uint32_t curr_cpos   =  y            * block_width + x;
      const uint32_t rest_cpos   =  y            * block_width + width_db4 + 1;

      const  int32_t curr_apos   = (y + a_ofs.y) * block_width + x + a_ofs.x;
      const  int32_t rest_apos   = (y + a_ofs.y) * block_width + width_db4 + a_ofs.x + 1;

      const  int32_t curr_bpos   = (y + b_ofs.y) * block_width + x + b_ofs.x;
      const  int32_t rest_bpos   = (y + b_ofs.y) * block_width + width_db4 + b_ofs.x + 1;

      // Same trick to read a narrow line as there is in the band SAO routine
      uint32_t a_last         = load_border_bytes(rec_data,  rest_apos, width_rest);
      uint32_t b_last         = load_border_bytes(rec_data,  rest_bpos, width_rest);
      uint32_t c_last         = load_border_bytes(rec_data,  rest_cpos, width_rest);
      uint32_t orig_last      = load_border_bytes(orig_data, rest_cpos, width_rest);

      const int32_t *a_ptr    = (const int32_t *)(rec_data  + curr_apos);
      const int32_t *b_ptr    = (const int32_t *)(rec_data  + curr_bpos);
      const int32_t *c_ptr    = (const int32_t *)(rec_data  + curr_cpos);
      const int32_t *orig_ptr = (const int32_t *)(orig_data + curr_cpos);

      __m256i a    = _mm256_maskload_epi32(a_ptr,    db4_mask);
      __m256i b    = _mm256_maskload_epi32(b_ptr,    db4_mask);
      __m256i c    = _mm256_maskload_epi32(c_ptr,    db4_mask);
      __m256i orig = _mm256_maskload_epi32(orig_ptr, db4_mask);

              a    = _mm256_insert_epi32  (a,        a_last,    7);
              b    = _mm256_insert_epi32  (b,        b_last,    7);
              c    = _mm256_insert_epi32  (c,        c_last,    7);
              orig = _mm256_insert_epi32  (orig,     orig_last, 7);

      // Mask all unused bytes to 0xFF, so they won't count anywhere
      __m256i badbyte_mask = gen_badbyte_mask(db4_mask, width_rest);

      __m256i curr  = do_one_edge_ymm(a, b, c, orig, badbyte_mask, offsets_256);
              sum   = _mm256_add_epi32(sum, curr);
    }
  }
  return hsum_8x32b(sum);
}

static void FIX_W32 calc_edge_dir_one_ymm(const __m256i  a,
                                          const __m256i  b,
                                          const __m256i  c,
                                          const __m256i  orig,
                                          const __m256i  badbyte_mask,
                                                __m256i *diff_accum,
                                                int32_t *hit_cnt)
{
  const __m256i ones_16 = _mm256_set1_epi16(1);
        __m256i eo_cat  = calc_eo_cat      (a, b, c);
                eo_cat  = _mm256_or_si256  (eo_cat, badbyte_mask);

  __m256i diffs_lo, diffs_hi;
  diff_epi8_epi16(orig, c, &diffs_lo, &diffs_hi);

  for (uint32_t i = 0; i < 5; i++) {
    __m256i  curr_id       = _mm256_set1_epi8    (i);
    __m256i  eoc_mask      = _mm256_cmpeq_epi8   (eo_cat, curr_id);
    uint32_t eoc_bits      = _mm256_movemask_epi8(eoc_mask);
    uint32_t eoc_hits      = _mm_popcnt_u32      (eoc_bits);

    __m256i  eoc_mask_lo   = _mm256_unpacklo_epi8(eoc_mask,      eoc_mask);
    __m256i  eoc_mask_hi   = _mm256_unpackhi_epi8(eoc_mask,      eoc_mask);

    __m256i  eoc_diffs_lo  = _mm256_and_si256    (diffs_lo,      eoc_mask_lo);
    __m256i  eoc_diffs_hi  = _mm256_and_si256    (diffs_hi,      eoc_mask_hi);

    __m256i  eoc_diffs_16  = _mm256_add_epi16    (eoc_diffs_lo,  eoc_diffs_hi);
    __m256i  eoc_diffs_32  = _mm256_madd_epi16   (eoc_diffs_16,  ones_16);

             diff_accum[i] = _mm256_add_epi32    (diff_accum[i], eoc_diffs_32);
             hit_cnt[i]   += eoc_hits;
  }
}

static void calc_sao_edge_dir_avx2(const kvz_pixel *orig_data,
                                   const kvz_pixel *rec_data,
                                         int32_t    eo_class,
                                         int32_t    block_width,
                                         int32_t    block_height,
                                         int32_t    cat_sum_cnt[2][NUM_SAO_EDGE_CATEGORIES])
{
  vector2d_t a_ofs = g_sao_edge_offsets[eo_class][0];
  vector2d_t b_ofs = g_sao_edge_offsets[eo_class][1];

  int32_t *diff_sum   = cat_sum_cnt[0];
  int32_t *hit_cnt    = cat_sum_cnt[1];

  int32_t scan_width  = block_width -   2;
  int32_t width_db32  = scan_width  & ~31;
  int32_t width_db4   = scan_width  &  ~3;
  int32_t width_rest  = scan_width  &   3;

  const __m256i zero          = _mm256_setzero_si256();

  // Form the load&store mask
  const __m256i wdb4_256      = _mm256_set1_epi32 (width_db4 & 31);
  const __m256i indexes       = _mm256_setr_epi32 (3, 7, 11, 15, 19, 23, 27, 31);
  const __m256i db4_mask      = _mm256_cmpgt_epi32(wdb4_256, indexes);

  __m256i diff_accum[5] = { _mm256_setzero_si256() };

  int32_t y, x;
  for (y = 1; y < block_height - 1; y++) {
    for (x = 1; x < width_db32 + 1; x += 32) {
      const uint32_t a_off = (y + a_ofs.y) * block_width + x + a_ofs.x;
      const uint32_t b_off = (y + b_ofs.y) * block_width + x + b_ofs.x;
      const uint32_t c_off =  y            * block_width + x;

      __m256i a      = _mm256_loadu_si256((const __m256i *)(rec_data  + a_off));
      __m256i b      = _mm256_loadu_si256((const __m256i *)(rec_data  + b_off));
      __m256i c      = _mm256_loadu_si256((const __m256i *)(rec_data  + c_off));
      __m256i orig   = _mm256_loadu_si256((const __m256i *)(orig_data + c_off));

      calc_edge_dir_one_ymm(a, b, c, orig, zero, diff_accum, hit_cnt);
    }
    if (scan_width > width_db32) {
      const uint32_t curr_cpos   =  y            * block_width + x;
      const uint32_t rest_cpos   =  y            * block_width + width_db4 + 1;

      const  int32_t curr_apos   = (y + a_ofs.y) * block_width + x + a_ofs.x;
      const  int32_t rest_apos   = (y + a_ofs.y) * block_width + width_db4 + a_ofs.x + 1;

      const  int32_t curr_bpos   = (y + b_ofs.y) * block_width + x + b_ofs.x;
      const  int32_t rest_bpos   = (y + b_ofs.y) * block_width + width_db4 + b_ofs.x + 1;

            uint32_t a_last      = load_border_bytes(rec_data,  rest_apos, width_rest);
            uint32_t b_last      = load_border_bytes(rec_data,  rest_bpos, width_rest);
            uint32_t c_last      = load_border_bytes(rec_data,  rest_cpos, width_rest);
            uint32_t orig_last   = load_border_bytes(orig_data, rest_cpos, width_rest);

      const int32_t *a_ptr       = (const int32_t *)(rec_data  + curr_apos);
      const int32_t *b_ptr       = (const int32_t *)(rec_data  + curr_bpos);
      const int32_t *c_ptr       = (const int32_t *)(rec_data  + curr_cpos);
      const int32_t *orig_ptr    = (const int32_t *)(orig_data + curr_cpos);

      __m256i a    = _mm256_maskload_epi32(a_ptr,    db4_mask);
      __m256i b    = _mm256_maskload_epi32(b_ptr,    db4_mask);
      __m256i c    = _mm256_maskload_epi32(c_ptr,    db4_mask);
      __m256i orig = _mm256_maskload_epi32(orig_ptr, db4_mask);

              a    = _mm256_insert_epi32  (a,        a_last,    7);
              b    = _mm256_insert_epi32  (b,        b_last,    7);
              c    = _mm256_insert_epi32  (c,        c_last,    7);
              orig = _mm256_insert_epi32  (orig,     orig_last, 7);

      __m256i badbyte_mask = gen_badbyte_mask(db4_mask, width_rest);

      calc_edge_dir_one_ymm(a, b, c, orig, badbyte_mask, diff_accum, hit_cnt);
    }
  }
  for (uint32_t i = 0; i < 5; i++) {
    int32_t sum = hsum_8x32b(diff_accum[i]);
    diff_sum[i] += sum;
  }
}

/*
 * Calculate an array of intensity correlations for each intensity value.
 * Return array as 16 YMM vectors, each containing 2x16 unsigned bytes
 * (to ease array lookup from YMMs using the shuffle trick, the low and
 * high lanes of each vector are duplicates). Have fun scaling this to
 * 16-bit picture data!
 */
static void calc_sao_offset_array_avx2(const encoder_control_t *encoder,
                                       const sao_info_t        *sao,
                                             __m256i           *offsets,
                                             color_t            color_i)
{
  const uint32_t band_pos   = (color_i == COLOR_V) ? 1 : 0;
  const  int32_t cur_bp     = sao->band_position[band_pos];

  const __m256i  zero       = _mm256_setzero_si256();
  const __m256i  threes     = _mm256_set1_epi8  (  3);

  const __m256i  band_pos_v = _mm256_set1_epi8  (band_pos << 2);
  const __m256i  cur_bp_v   = _mm256_set1_epi8  (cur_bp);
  const __m256i  val_incr   = _mm256_set1_epi8  (16);
  const __m256i  band_incr  = _mm256_set1_epi8  ( 2);
        __m256i  vals       = _mm256_setr_epi8  ( 0,  1,  2,  3,  4,  5,  6,  7,
                                                  8,  9, 10, 11, 12, 13, 14, 15,
                                                  0,  1,  2,  3,  4,  5,  6,  7,
                                                  8,  9, 10, 11, 12, 13, 14, 15);

        __m256i  bands     = _mm256_setr_epi32 (0, 0, 0x01010101, 0x01010101,
                                                0, 0, 0x01010101, 0x01010101);

  // We'll only ever address SAO offsets 1, 2, 3, 4, 6, 7, 8, 9, so only load
  // them and truncate into signed 16 bits (anything out of that range will
  // anyway saturate anything they're used to do)
  __m128i sao_offs_lo  = _mm_loadu_si128((const __m128i *)(sao->offsets + 1));
  __m128i sao_offs_hi  = _mm_loadu_si128((const __m128i *)(sao->offsets + 6));

  __m128i sao_offs_xmm = _mm_packs_epi32  (sao_offs_lo, sao_offs_hi);
  __m256i sao_offs     = broadcast_xmm2ymm(sao_offs_xmm);

  for (uint32_t i = 0; i < 16; i++) {
    // bands will always be in [0, 31], and cur_bp in [0, 27], so no overflow
    // can occur
    __m256i band_m_bp = _mm256_sub_epi8    (bands,  cur_bp_v);

    // If (x & ~3) != 0 for any signed x, then x < 0 or x > 3
    __m256i bmbp_bads = _mm256_andnot_si256(threes,    band_m_bp);
    __m256i in_band   = _mm256_cmpeq_epi8  (zero,      bmbp_bads);

    __m256i offset_id = _mm256_add_epi8    (band_m_bp, band_pos_v);

    __m256i val_lo, val_hi;
    cvt_epu8_epi16(vals, &val_lo, &val_hi);

    __m256i offid_lo, offid_hi;
    cvt_shufmask_epi8_epi16(offset_id, &offid_lo, &offid_hi);

    __m256i offs_lo = _mm256_shuffle_epi8(sao_offs, offid_lo);
    __m256i offs_hi = _mm256_shuffle_epi8(sao_offs, offid_hi);

    __m256i sums_lo = _mm256_adds_epi16  (val_lo,   offs_lo);
    __m256i sums_hi = _mm256_adds_epi16  (val_hi,   offs_hi);

            sums_lo = _mm256_max_epi16   (sums_lo,  zero);
            sums_hi = _mm256_max_epi16   (sums_hi,  zero);

    __m256i offs    = _mm256_packus_epi16(sums_lo,  sums_hi);

    offsets[i]      = _mm256_blendv_epi8 (vals,     offs, in_band);

            vals    = _mm256_add_epi8    (vals,     val_incr);
            bands   = _mm256_add_epi8    (bands,    band_incr);
  }
}

static __m256i lookup_color_band_ymm(const __m256i  curr_row,
                                     const __m256i *offsets)
{
  const __m256i select_nibble = _mm256_set1_epi8   (0x0f);
  const __m256i lo_nibbles    = _mm256_and_si256   (select_nibble, curr_row);
  const __m256i hi_nibbles    = _mm256_andnot_si256(select_nibble, curr_row);

  // Loop through the offset vectors, the 0xi'th one always holding
  // offsets 0xi0...0xif. Use shuffle to do a lookup on the current
  // offset vector, then check which pixels actually should be looked
  // up from this vector (ie. whether their values are 0xi0...0xif) and
  // mask out any but correct ones.
  __m256i result_row = _mm256_setzero_si256();
  for (uint8_t i = 0; i < 16; i += 4) {

    __m256i curr_hinib0   = _mm256_set1_epi8   ((i + 0) << 4);
    __m256i curr_hinib1   = _mm256_set1_epi8   ((i + 1) << 4);
    __m256i curr_hinib2   = _mm256_set1_epi8   ((i + 2) << 4);
    __m256i curr_hinib3   = _mm256_set1_epi8   ((i + 3) << 4);

    __m256i hinib_select0 = _mm256_cmpeq_epi8  (curr_hinib0,    hi_nibbles);
    __m256i hinib_select1 = _mm256_cmpeq_epi8  (curr_hinib1,    hi_nibbles);
    __m256i hinib_select2 = _mm256_cmpeq_epi8  (curr_hinib2,    hi_nibbles);
    __m256i hinib_select3 = _mm256_cmpeq_epi8  (curr_hinib3,    hi_nibbles);

    __m256i lonib_lookup0 = _mm256_shuffle_epi8(offsets[i + 0], lo_nibbles);
    __m256i lonib_lookup1 = _mm256_shuffle_epi8(offsets[i + 1], lo_nibbles);
    __m256i lonib_lookup2 = _mm256_shuffle_epi8(offsets[i + 2], lo_nibbles);
    __m256i lonib_lookup3 = _mm256_shuffle_epi8(offsets[i + 3], lo_nibbles);

    __m256i lookup_mskd0  = _mm256_and_si256   (hinib_select0,  lonib_lookup0);
    __m256i lookup_mskd1  = _mm256_and_si256   (hinib_select1,  lonib_lookup1);
    __m256i lookup_mskd2  = _mm256_and_si256   (hinib_select2,  lonib_lookup2);
    __m256i lookup_mskd3  = _mm256_and_si256   (hinib_select3,  lonib_lookup3);

    __m256i lookup_mskd01 = _mm256_or_si256    (lookup_mskd0,   lookup_mskd1);
    __m256i lookup_mskd23 = _mm256_or_si256    (lookup_mskd2,   lookup_mskd3);
    __m256i lookup_res    = _mm256_or_si256    (lookup_mskd01,  lookup_mskd23);

            result_row    = _mm256_or_si256    (result_row,     lookup_res);
  }
  return result_row;
}

static INLINE void reconstruct_color_band(const encoder_control_t *encoder,
                                          const kvz_pixel         *rec_data,
                                                kvz_pixel         *new_rec_data,
                                          const sao_info_t        *sao,
                                                int32_t            stride,
                                                int32_t            new_stride,
                                                int32_t            block_width,
                                                int32_t            block_height,
                                                color_t            color_i)
{
  const uint32_t width_db32 = block_width & ~31;
  const uint32_t width_db4  = block_width &  ~3;
  const uint32_t width_rest = block_width &   3;

  // Form the load&store mask
  const __m256i wdb4_256      = _mm256_set1_epi32 (width_db4 & 31);
  const __m256i indexes       = _mm256_setr_epi32 (3, 7, 11, 15, 19, 23, 27, 31);
  const __m256i db4_mask      = _mm256_cmpgt_epi32(wdb4_256, indexes);

  // Each of the 256 offsets is a byte, but only 16 are held in one YMM since
  // lanes must be duplicated to use shuffle.
  __m256i offsets[16];
  calc_sao_offset_array_avx2(encoder, sao, offsets, color_i);

  for (uint32_t y = 0; y < block_height; y++) {
    uint32_t x = 0;
    for (; x < width_db32; x += 32) {
      const uint32_t curr_srcpos = y *     stride + x;
      const uint32_t curr_dstpos = y * new_stride + x;

      __m256i curr_row = _mm256_loadu_si256((const __m256i *)(rec_data + curr_srcpos));
      __m256i result   = lookup_color_band_ymm(curr_row, offsets);
      _mm256_storeu_si256((__m256i *)(new_rec_data + curr_dstpos), result);
    }
    if (block_width > width_db32) {
      const uint32_t curr_srcpos = y *     stride + x;
      const uint32_t curr_dstpos = y * new_stride + x;
      const uint32_t rest_srcpos = y *     stride + width_db4;
      const uint32_t rest_dstpos = y * new_stride + width_db4;

      // Read the very last pixels byte by byte and pack them into one dword.
      // Piggyback said dword as the highest dword of the row vector variable,
      // that particular place can never be loaded into by the maskmove
      // (otherwise that vector would go through the divisible-by-32 code
      // path).
      uint32_t last_dword = load_border_bytes(rec_data, rest_srcpos, width_rest);

      const int32_t *src_ptr = (const int32_t *)(    rec_data + curr_srcpos);
            int32_t *dst_ptr = (      int32_t *)(new_rec_data + curr_dstpos);

      __m256i curr_row = _mm256_maskload_epi32(src_ptr,  db4_mask);
              curr_row = _mm256_insert_epi32  (curr_row, last_dword, 7);
      __m256i result   = lookup_color_band_ymm(curr_row, offsets);

      _mm256_maskstore_epi32(dst_ptr, db4_mask, result);
      uint32_t last_dword_dst = _mm256_extract_epi32(result, 7);

      store_border_bytes(new_rec_data, rest_dstpos, width_rest, last_dword_dst);
    }
  }
}

static __m256i FIX_W32 do_one_nonband_ymm(const __m256i a,
                                          const __m256i b,
                                          const __m256i c,
                                          const __m256i sao_offs)
{
  const __m256i zero = _mm256_setzero_si256();

  __m256i eo_cat = calc_eo_cat(a, b, c);
  __m256i eo_cat_lo, eo_cat_hi, c_lo, c_hi;
  cvt_shufmask_epi8_epi16(eo_cat, &eo_cat_lo, &eo_cat_hi);
  cvt_epu8_epi16         (c,      &c_lo,      &c_hi);

  __m256i offs_lo = _mm256_shuffle_epi8(sao_offs, eo_cat_lo);
  __m256i offs_hi = _mm256_shuffle_epi8(sao_offs, eo_cat_hi);

  __m256i res_lo  = _mm256_adds_epi16  (offs_lo,  c_lo);
  __m256i res_hi  = _mm256_adds_epi16  (offs_hi,  c_hi);

          res_lo  = _mm256_max_epi16   (res_lo,   zero);
          res_hi  = _mm256_max_epi16   (res_hi,   zero);

  __m256i res     = _mm256_packus_epi16(res_lo,   res_hi);
  return res;
}

static INLINE void reconstruct_color_other(const encoder_control_t *encoder,
                                           const kvz_pixel         *rec_data,
                                                 kvz_pixel         *new_rec_data,
                                           const sao_info_t        *sao,
                                                 int32_t            stride,
                                                 int32_t            new_stride,
                                                 int32_t            block_width,
                                                 int32_t            block_height,
                                                 color_t            color_i)
{
  const uint32_t   offset_v    = color_i == COLOR_V ? 5 : 0;
  const vector2d_t a_ofs       = g_sao_edge_offsets[sao->eo_class][0];
  const vector2d_t b_ofs       = g_sao_edge_offsets[sao->eo_class][1];

  const uint32_t   width_db32  = block_width & ~31;
  const uint32_t   width_db4   = block_width &  ~3;
  const uint32_t   width_rest  = block_width &   3;

  // Form the load&store mask
  const __m256i    wdb4_256    = _mm256_set1_epi32 (width_db4 & 31);
  const __m256i    indexes     = _mm256_setr_epi32 (3, 7, 11, 15, 19, 23, 27, 31);
  const __m256i    db4_mask    = _mm256_cmpgt_epi32(wdb4_256, indexes);

  // Again, saturate offsets to signed 16 bits, because anything outside of
  // [-255, 255] will saturate anything these are used with
  const __m128i    sao_offs_lo = _mm_loadu_si128  ((const __m128i *)(sao->offsets + offset_v + 0));
  const __m128i    sao_offs_hi = _mm_cvtsi32_si128(sao->offsets[offset_v + 4]);
  const __m128i    sao_offs_16 = _mm_packs_epi32  (sao_offs_lo, sao_offs_hi);

  const __m256i    sao_offs    = broadcast_xmm2ymm(sao_offs_16);

  for (uint32_t y = 0; y < block_height; y++) {
    uint32_t x;
    for (x = 0; x < width_db32; x += 32) {
      const uint32_t  src_pos = y *     stride + x;
      const uint32_t  dst_pos = y * new_stride + x;

      // TODO: these will go negative, but that's a defect of the original
      // code already since 2013 (98f2a1aedc5f4933c2729ae15412549dea9e5549)
      const int32_t   a_pos   = (y + a_ofs.y) * stride + x + a_ofs.x;
      const int32_t   b_pos   = (y + b_ofs.y) * stride + x + b_ofs.x;

      __m256i a = _mm256_loadu_si256((const __m256i *)(rec_data + a_pos));
      __m256i b = _mm256_loadu_si256((const __m256i *)(rec_data + b_pos));
      __m256i c = _mm256_loadu_si256((const __m256i *)(rec_data + src_pos));

      __m256i res = do_one_nonband_ymm(a, b, c, sao_offs);
      _mm256_storeu_si256((__m256i *)(new_rec_data + dst_pos), res);
    }
    if (block_width > width_db32) {
      const uint32_t curr_srcpos =  y            * stride + x;
      const uint32_t rest_srcpos =  y            * stride + width_db4;

      const  int32_t curr_apos   = (y + a_ofs.y) * stride + a_ofs.x + x;
      const  int32_t rest_apos   = (y + a_ofs.y) * stride + a_ofs.x + width_db4;

      const  int32_t curr_bpos   = (y + b_ofs.y) * stride + b_ofs.x + x;
      const  int32_t rest_bpos   = (y + b_ofs.y) * stride + b_ofs.x + width_db4;

      const uint32_t curr_dstpos = y * new_stride + x;
      const uint32_t rest_dstpos = y * new_stride + width_db4;

      uint32_t a_last        = load_border_bytes(rec_data, rest_apos,   width_rest);
      uint32_t b_last        = load_border_bytes(rec_data, rest_bpos,   width_rest);
      uint32_t c_last        = load_border_bytes(rec_data, rest_srcpos, width_rest);

      const int32_t   *a_ptr = (const int32_t *)(    rec_data + curr_apos);
      const int32_t   *b_ptr = (const int32_t *)(    rec_data + curr_bpos);
      const int32_t   *c_ptr = (const int32_t *)(    rec_data + curr_srcpos);
            int32_t *dst_ptr = (      int32_t *)(new_rec_data + curr_dstpos);

      __m256i a = _mm256_maskload_epi32(a_ptr, db4_mask);
      __m256i b = _mm256_maskload_epi32(b_ptr, db4_mask);
      __m256i c = _mm256_maskload_epi32(c_ptr, db4_mask);

              a = _mm256_insert_epi32  (a, a_last, 7);
              b = _mm256_insert_epi32  (b, b_last, 7);
              c = _mm256_insert_epi32  (c, c_last, 7);

      __m256i res = do_one_nonband_ymm(a, b, c, sao_offs);
      _mm256_maskstore_epi32(dst_ptr, db4_mask, res);

      uint32_t last_dword = _mm256_extract_epi32(res, 7);

      store_border_bytes(new_rec_data, rest_dstpos, width_rest, last_dword);
    }
  }
}

static void sao_reconstruct_color_avx2(const encoder_control_t *encoder,
                                       const kvz_pixel         *rec_data,
                                             kvz_pixel         *new_rec_data,
                                       const sao_info_t        *sao,
                                             int32_t            stride,
                                             int32_t            new_stride,
                                             int32_t            block_width,
                                             int32_t            block_height,
                                             color_t            color_i)
{
  if (sao->type == SAO_TYPE_BAND) {
    reconstruct_color_band (encoder, rec_data, new_rec_data, sao, stride, new_stride, block_width, block_height, color_i);
  } else {
    reconstruct_color_other(encoder, rec_data, new_rec_data, sao, stride, new_stride, block_width, block_height, color_i);
  }
}

static int32_t sao_band_ddistortion_avx2(const encoder_state_t *state,
                                         const uint8_t         *orig_data,
                                         const uint8_t         *rec_data,
                                               int32_t          block_width,
                                               int32_t          block_height,
                                               int32_t          band_pos,
                                         const int32_t          sao_bands[4])
{
  const uint32_t bitdepth = 8;
  const uint32_t shift    = bitdepth - 5;

  // Clamp band_pos to 32 from above. It'll be subtracted from the shifted
  // rec_data values, which in 8-bit depth will always be clamped to [0, 31],
  // so if it ever exceeds 32, all the band values will be negative and
  // ignored. Ditto for less than -4.
  __m128i bp_128   = _mm_cvtsi32_si128    (band_pos);
  __m128i hilimit  = _mm_cvtsi32_si128    (32);
  __m128i lolimit  = _mm_cvtsi32_si128    (-4);

          bp_128   = _mm_min_epi8         (bp_128, hilimit);
          bp_128   = _mm_max_epi8         (bp_128, lolimit);

  __m256i bp_256  = _mm256_broadcastb_epi8(bp_128);

  __m128i sbs_32   = _mm_loadu_si128((const __m128i *)sao_bands);
  __m128i sbs_8    = truncate_epi32_epi8(sbs_32);
  __m256i sb_256   = broadcast_xmm2ymm  (sbs_8);

  // These should trigger like, never, at least the later condition of block
  // not being a multiple of 32 wide. Rather safe than sorry though, huge SAO
  // bands are more tricky of these two because the algorithm needs a complete
  // reimplementation to work on 16-bit values.
  if (epi32v_fits_in_epi8s(sbs_32) != 0xffff)
    goto use_generic;

  // If VVC or something will start using SAO on blocks with width a multiple
  // of 16, feel free to implement a XMM variant of this algorithm
  if ((block_width & 31) != 0)
    goto use_generic;

  const __m256i zero          = _mm256_setzero_si256();
  const __m256i threes        = _mm256_set1_epi8 (3);

  __m256i sum = _mm256_setzero_si256();
  for (uint32_t y = 0; y < block_height; y++) {
    for (uint32_t x = 0; x < block_width; x += 32) {
      const int32_t curr_pos = y * block_width + x;

      __m256i   rd = _mm256_loadu_si256((const __m256i *)( rec_data + curr_pos));
      __m256i orig = _mm256_loadu_si256((const __m256i *)(orig_data + curr_pos));

      __m256i orig_lo, orig_hi, rd_lo, rd_hi;
      cvt_epu8_epi16(orig, &orig_lo, &orig_hi);
      cvt_epu8_epi16(rd,   &rd_lo,   &rd_hi);

      __m256i diff_lo      = _mm256_sub_epi16     (orig_lo,      rd_lo);
      __m256i diff_hi      = _mm256_sub_epi16     (orig_hi,      rd_hi);

      // The shift will clamp band to 0...31; band_pos on the other
      // hand is always between 0...32, so band will be -1...31. Anything
      // below zero is ignored, so we can clamp band_pos to 32.
      __m256i rd_divd      = srli_epi8           (rd,            shift);
      __m256i band         = _mm256_sub_epi8     (rd_divd,       bp_256);

      // Force all <0 or >3 bands to 0xff, which will zero the shuffle result
      __m256i band_lt_0    = _mm256_cmpgt_epi8   (zero,          band);
      __m256i band_gt_3    = _mm256_cmpgt_epi8   (band,          threes);
      __m256i band_inv     = _mm256_or_si256     (band_lt_0,     band_gt_3);

              band         = _mm256_or_si256     (band,          band_inv);

      __m256i offsets      = _mm256_shuffle_epi8 (sb_256,        band);

      __m256i curr_sum     = calc_diff_off_delta (diff_lo, diff_hi, offsets, orig);
              sum          = _mm256_add_epi32    (sum,          curr_sum);
    }
  }
  return hsum_8x32b(sum);

use_generic:
  return sao_band_ddistortion_generic(state, orig_data, rec_data, block_width,
      block_height, band_pos, sao_bands);
}

#endif //COMPILE_INTEL_AVX2

int kvz_strategy_register_sao_avx2(void* opaque, uint8_t bitdepth)
{
  bool success = true;
#if COMPILE_INTEL_AVX2
  if (bitdepth == 8) {
    success &= kvz_strategyselector_register(opaque, "sao_edge_ddistortion", "avx2", 40, &sao_edge_ddistortion_avx2);
    success &= kvz_strategyselector_register(opaque, "calc_sao_edge_dir", "avx2", 40, &calc_sao_edge_dir_avx2);
    success &= kvz_strategyselector_register(opaque, "sao_reconstruct_color", "avx2", 40, &sao_reconstruct_color_avx2);
    success &= kvz_strategyselector_register(opaque, "sao_band_ddistortion", "avx2", 40, &sao_band_ddistortion_avx2);
  }
#endif //COMPILE_INTEL_AVX2
  return success;
}
