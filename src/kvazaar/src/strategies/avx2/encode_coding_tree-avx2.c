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

#include "strategyselector.h"

#if COMPILE_INTEL_AVX2
#include "avx2_common_functions.h"
#include "cabac.h"
#include "context.h"
#include "encode_coding_tree-avx2.h"
#include "encode_coding_tree.h"
#include "strategies/missing-intel-intrinsics.h"
#include <immintrin.h>

/*
 * NOTE: Unlike SSE/AVX comparisons that would return 11 or 00 for gt/lte,
 * this'll use 1x and 0x as bit patterns (x: garbage). A couple extra
 * instructions will get you 11 and 00 if you need to use this as a mask
 * somewhere at some point, but we don't need this right now.
 *
 * I'd love to draw a logic circuit here to describe this, but I can't. Two
 * 2-bit uints can be compared for greaterness by first comparing their high
 * bits using AND-NOT; (x AND (NOT y)) == 1 if x > y. If A_hi > B_hi, A > B.
 * If A_hi == B_hi AND A_lo > B_lo, A > B. Otherwise, A <= B. It's really
 * simple when drawn on paper, but quite messy on a general-purpose ALU. But
 * look, just five instructions!
 */
static INLINE uint32_t u32vec_cmpgt_epu2(uint32_t a, uint32_t b)
{
  uint32_t a_gt_b          = _andn_u32(b, a);
  uint32_t a_ne_b          = a ^ b;
  uint32_t a_gt_b_sh       = a_gt_b << 1;
  uint32_t lobit_tiebrk_hi = _andn_u32(a_ne_b, a_gt_b_sh);
  uint32_t res             = a_gt_b | lobit_tiebrk_hi;
  return res;
}

static INLINE uint32_t pack_16x16b_to_16x2b(__m256i src)
{
  /*
   * For each 16-bit element in src:
   * ABCD EFGH IJKL MNOP Original elements
   * 0000 0000 0000 00XY Element clipped to [0, 3] using _mm256_min_epu16
   * 0000 000X Y000 0000 Shift word to align LSBs across byte boundary
   * 0000 0001 1000 0000 Comparison mask to be compared against
   * XXXX XXXX YYYY YYYY Comparison result, for movemask
   */
  const __m256i threes  = _mm256_set1_epi16   (3);
  const __m256i cmpmask = _mm256_slli_epi16   (threes, 7); // 0x0180 (avoid set1)

  __m256i  clipped      = _mm256_min_epu16    (src, threes);
  __m256i  shifted      = _mm256_slli_epi16   (clipped, 7);
  __m256i  cmpres       = _mm256_cmpeq_epi8   (shifted, cmpmask);
  uint32_t result       = _mm256_movemask_epi8(cmpres);

  return result;
}

/**
 * \brief Context derivation process of coeff_abs_significant_flag,
 *        parallelized to handle 16 coeffs at once
 * \param pattern_sig_ctx pattern for current coefficient group
 * \param scan_idx pixel scan type in use
 * \param pos_xs column addresses of current scan positions
 * \param pos_ys row addresses of current scan positions
 * \param block_type log2 value of block size if square block, or 4 otherwise
 * \param width width of the block
 * \param texture_type texture type (TEXT_LUMA...)
 * \returns ctx_inc for current scan position
 */
static INLINE __m256i kvz_context_get_sig_ctx_inc_16x16b(int32_t pattern_sig_ctx, uint32_t scan_idx, __m256i pos_xs,
                                __m256i pos_ys, int32_t block_type, int8_t texture_type)
{
  const __m256i zero   = _mm256_set1_epi8(0);
  const __m256i ff     = _mm256_set1_epi8(0xff);

  const __m256i ones   = _mm256_set1_epi16(1);
  const __m256i twos   = _mm256_set1_epi16(2);
  const __m256i threes = _mm256_set1_epi16(3);

  const __m256i ctx_ind_map[3] = {
    _mm256_setr_epi16(
        0, 2, 1, 6,
        3, 4, 7, 6,
        4, 5, 7, 8,
        5, 8, 8, 8
    ),
    _mm256_setr_epi16(
        0, 1, 4, 5,
        2, 3, 4, 5,
        6, 6, 8, 8,
        7, 7, 8, 8
    ),
    _mm256_setr_epi16(
        0, 2, 6, 7,
        1, 3, 6, 7,
        4, 4, 8, 8,
        5, 5, 8, 8
    ),
  };

  int16_t offset;
  if (block_type == 3)
    if (scan_idx == SCAN_DIAG)
      offset = 9;
    else
      offset = 15;
  else
    if (texture_type == 0)
      offset = 21;
    else
      offset = 12;

  __m256i offsets = _mm256_set1_epi16(offset);

  // This will only ever be compared to 0, 1 and 2, so it's fine to cast down
  // to 16b (and it should never be above 3 anyways)
  __m256i pattern_sig_ctxs = _mm256_set1_epi16((int16_t)(MIN(0xffff, pattern_sig_ctx)));
  __m256i pattern_sig_ctxs_eq_zero = _mm256_cmpeq_epi16(pattern_sig_ctxs, zero);
  __m256i pattern_sig_ctxs_eq_one  = _mm256_cmpeq_epi16(pattern_sig_ctxs, ones);
  __m256i pattern_sig_ctxs_eq_two  = _mm256_cmpeq_epi16(pattern_sig_ctxs, twos);

  __m256i pattern_sig_ctxs_eq_1or2 = _mm256_or_si256 (pattern_sig_ctxs_eq_one,
                                                      pattern_sig_ctxs_eq_two);
  __m256i pattern_sig_ctxs_lt3     = _mm256_or_si256 (pattern_sig_ctxs_eq_1or2,
                                                      pattern_sig_ctxs_eq_zero);
  __m256i pattern_sig_ctxs_other   = _mm256_xor_si256(pattern_sig_ctxs_lt3,
                                                      ff);
  __m256i x_plus_y        = _mm256_add_epi16  (pos_xs,   pos_ys);
  __m256i x_plus_y_zero   = _mm256_cmpeq_epi16(x_plus_y, zero);   // All these should be 0, preempts block_type_two rule

  __m256i texture_types = _mm256_set1_epi16((int16_t)texture_type);

  __m256i block_types     = _mm256_set1_epi16((int16_t)block_type);
  __m256i block_type_two  = _mm256_cmpeq_epi16(block_types, twos);   // All these should be ctx_ind_map[4 * pos_y + pos_x];
  __m256i bt2_vals        = ctx_ind_map[scan_idx];
  __m256i bt2_vals_masked = _mm256_and_si256(bt2_vals, block_type_two);

  __m256i pos_xs_in_subset = _mm256_and_si256(pos_xs, threes);
  __m256i pos_ys_in_subset = _mm256_and_si256(pos_ys, threes);

  __m256i cg_pos_xs        = _mm256_srli_epi16(pos_xs, 2);
  __m256i cg_pos_ys        = _mm256_srli_epi16(pos_ys, 2);
  __m256i cg_pos_xysums    = _mm256_add_epi16 (cg_pos_xs, cg_pos_ys);

  __m256i pos_xy_sums_in_subset = _mm256_add_epi16(pos_xs_in_subset, pos_ys_in_subset);

  /*
   * if (pattern_sig_ctx == 0) {
   *   switch (pos_x_in_subset + pos_y_in_subset) {
   *   case 0:
   *     cnt = 2;
   *     break;
   *   case 1:
   *   case 2:
   *     cnt = 1;
   *     break;
   *   default:
   *     cnt = 0;
   *   }
   * }
   *
   * Equivalent to:
   *
   * if (pattern_sig_ctx == 0) {
   *   subamt = cnt <= 1 ? 1 : 0;
   *   pxyis_max3 = min(3, pos_x_in_subset + pos_y_in_subset);
   *   cnt = (3 - pxyis_max3) - subamt;
   * }
   */
  __m256i pxyis_lte_1     = _mm256_cmpgt_epi16(twos,                  pos_xy_sums_in_subset);
  __m256i subamts         = _mm256_and_si256  (pxyis_lte_1,           ones);
  __m256i pxyis_max3      = _mm256_min_epu16  (pos_xy_sums_in_subset, threes);
  __m256i cnts_tmp        = _mm256_sub_epi16  (threes,                pxyis_max3);
  __m256i cnts_sig_ctx_0  = _mm256_sub_epi16  (cnts_tmp,              subamts);
  __m256i cnts_sc0_masked = _mm256_and_si256  (cnts_sig_ctx_0,        pattern_sig_ctxs_eq_zero);

  /*
   * if (pattern_sig_ctx == 1 || pattern_sig_ctx == 2) {
   *   if (pattern_sig_ctx == 1)
   *     subtrahend = pos_y_in_subset;
   *   else
   *     subtrahend = pos_x_in_subset;
   *   cnt = 2 - min(2, subtrahend);
   * }
   */
  __m256i pos_operands_ctx_1or2 = _mm256_blendv_epi8(pos_ys_in_subset,
                                                     pos_xs_in_subset,
                                                     pattern_sig_ctxs_eq_two);

  __m256i pos_operands_max2     = _mm256_min_epu16  (pos_operands_ctx_1or2, twos);
  __m256i cnts_sig_ctx_1or2     = _mm256_sub_epi16  (twos,                  pos_operands_max2);
  __m256i cnts_sc12_masked      = _mm256_and_si256  (cnts_sig_ctx_1or2,     pattern_sig_ctxs_eq_1or2);

  /*
   * if (pattern_sig_ctx > 2)
   *   cnt = 2;
   */
  __m256i cnts_scother_masked = _mm256_and_si256(twos, pattern_sig_ctxs_other);

  // Select correct count
  __m256i cnts_sc012_masked   = _mm256_or_si256 (cnts_sc0_masked,     cnts_sc12_masked);
  __m256i cnts                = _mm256_or_si256 (cnts_scother_masked, cnts_sc012_masked);

  // Compute final values
  __m256i textype_eq_0     = _mm256_cmpeq_epi16(texture_types, zero);
  __m256i cg_pos_sums_gt_0 = _mm256_cmpgt_epi16(cg_pos_xysums, zero);
  __m256i tmpcond          = _mm256_and_si256  (textype_eq_0,  cg_pos_sums_gt_0);
  __m256i tmp              = _mm256_and_si256  (tmpcond,       threes);
  __m256i tmp_with_offsets = _mm256_add_epi16  (tmp,           offsets);
  __m256i rv_noshortcirc   = _mm256_add_epi16  (cnts,          tmp_with_offsets);

  // Ol' sprite mask method works here!
  __m256i rv1 = _mm256_andnot_si256(block_type_two, rv_noshortcirc);
  __m256i rv2 = _mm256_or_si256    (rv1,            bt2_vals_masked);
  __m256i rv  = _mm256_andnot_si256(x_plus_y_zero,  rv2);
  return rv;
}

void kvz_encode_coeff_nxn_avx2(encoder_state_t * const state,
                               cabac_data_t * const cabac,
                               const coeff_t *coeff,
                               uint8_t width,
                               uint8_t type,
                               int8_t scan_mode,
                               int8_t tr_skip)
{
  const encoder_control_t * const encoder = state->encoder_control;
  int c1 = 1;
  uint8_t last_coeff_x = 0;
  uint8_t last_coeff_y = 0;
  int32_t i;
  uint32_t sig_coeffgroup_nzs[8 * 8] = { 0 };

  int8_t be_valid = encoder->cfg.signhide_enable;
  int32_t scan_pos_sig;
  uint32_t go_rice_param = 0;
  uint32_t ctx_sig;

  // CONSTANTS
  const uint32_t num_blk_side    = width >> TR_MIN_LOG2_SIZE;
  const uint32_t log2_block_size = kvz_g_convert_to_bit[width] + 2;
  const uint32_t *scan           =
    kvz_g_sig_last_scan[scan_mode][log2_block_size - 1];
  const uint32_t *scan_cg = g_sig_last_scan_cg[log2_block_size - 2][scan_mode];
  const uint32_t num_blocks = num_blk_side * num_blk_side;

  const __m256i zero = _mm256_set1_epi8(0);
  const __m256i ones = _mm256_set1_epi16(1);
  const __m256i twos = _mm256_set1_epi16(2);

  // Init base contexts according to block type
  cabac_ctx_t *base_coeff_group_ctx = &(cabac->ctx.cu_sig_coeff_group_model[type]);
  cabac_ctx_t *baseCtx           = (type == 0) ? &(cabac->ctx.cu_sig_model_luma[0]) :
                                 &(cabac->ctx.cu_sig_model_chroma[0]);

  // Scan all coeff groups to find out which of them have coeffs.
  // Populate sig_coeffgroup_nzs with that info.

  // NOTE: Modified the functionality a bit, sig_coeffgroup_flag used to be
  // 1 if true and 0 if false, now it's "undefined but nonzero" if true and
  // 0 if false (not actually undefined, it's a bitmask representing the
  // significant coefficients' position in the group which in itself could
  // be useful information)
  int32_t scan_cg_last = -1;

  for (int32_t i = 0; i < num_blocks; i++) {
    const uint32_t cg_id = scan_cg[i];
    const uint32_t n_xbits = log2_block_size - 2; // How many lowest bits of scan_cg represent X coord
    const uint32_t cg_x = cg_id & ((1 << n_xbits) - 1);
    const uint32_t cg_y = cg_id >> n_xbits;

    const uint32_t cg_pos = cg_y * width * 4 + cg_x * 4;
    const uint32_t cg_pos_y = (cg_pos >> log2_block_size) >> TR_MIN_LOG2_SIZE;
    const uint32_t cg_pos_x = (cg_pos & (width - 1)) >> TR_MIN_LOG2_SIZE;
    const uint32_t idx = cg_pos_x + cg_pos_y * num_blk_side;

    __m128d coeffs_d_upper = _mm_setzero_pd();
    __m128d coeffs_d_lower = _mm_setzero_pd();
    __m128i coeffs_upper;
    __m128i coeffs_lower;
    __m256i cur_coeffs;

    coeffs_d_upper = _mm_loadl_pd(coeffs_d_upper, (double *)(coeff + cg_pos + 0 * width));
    coeffs_d_upper = _mm_loadh_pd(coeffs_d_upper, (double *)(coeff + cg_pos + 1 * width));
    coeffs_d_lower = _mm_loadl_pd(coeffs_d_lower, (double *)(coeff + cg_pos + 2 * width));
    coeffs_d_lower = _mm_loadh_pd(coeffs_d_lower, (double *)(coeff + cg_pos + 3 * width));

    coeffs_upper = _mm_castpd_si128(coeffs_d_upper);
    coeffs_lower = _mm_castpd_si128(coeffs_d_lower);

    cur_coeffs = _mm256_insertf128_si256(_mm256_castsi128_si256(coeffs_upper),
                                         coeffs_lower,
                                         1);

    __m256i coeffs_zero = _mm256_cmpeq_epi16(cur_coeffs, zero);

    uint32_t nz_coeffs_2b = ~((uint32_t)_mm256_movemask_epi8(coeffs_zero));
    sig_coeffgroup_nzs[idx] = nz_coeffs_2b;

    if (nz_coeffs_2b)
      scan_cg_last = i;
  }
  // Rest of the code assumes at least one non-zero coeff.
  assert(scan_cg_last >= 0);

  ALIGNED(64) int16_t coeff_reord[LCU_WIDTH * LCU_WIDTH];
  uint32_t pos_last, scan_pos_last;

  {
    __m256i coeffs_r;
    for (int32_t i = 0; i <= scan_cg_last; i++) {
      int32_t subpos = i * 16;
      scanord_read_vector(&coeff, scan, scan_mode, subpos, width, &coeffs_r, 1);
      _mm256_store_si256((__m256i *)(coeff_reord + subpos), coeffs_r);
    }

    // Find the last coeff by going backwards in scan order. With cmpeq_epi16
    // and movemask, we can generate a dword with 16 2-bit masks that are 11
    // for zero words in the coeff vector, and 00 for nonzero words. By
    // inverting the bits and counting leading zeros, we can determine the
    // number of zero bytes in the vector counting from high to low memory
    // addresses; subtract that from 31 and divide by 2 to get the offset of
    // the last nonzero word.
    uint32_t baseaddr = scan_cg_last * 16;
    __m256i cur_coeffs_zeros = _mm256_cmpeq_epi16(coeffs_r, zero);
    uint32_t nz_bytes = ~(_mm256_movemask_epi8(cur_coeffs_zeros));
    scan_pos_last = baseaddr + ((31 - _lzcnt_u32(nz_bytes)) >> 1);
    pos_last = scan[scan_pos_last];
  }

  // transform skip flag
  if(width == 4 && encoder->cfg.trskip_enable) {
    cabac->cur_ctx = (type == 0) ? &(cabac->ctx.transform_skip_model_luma) : &(cabac->ctx.transform_skip_model_chroma);
    CABAC_BIN(cabac, tr_skip, "transform_skip_flag");
  }

  last_coeff_x = pos_last & (width - 1);
  last_coeff_y = (uint8_t)(pos_last >> log2_block_size);

  // Code last_coeff_x and last_coeff_y
  kvz_encode_last_significant_xy(cabac,
                                 last_coeff_x,
                                 last_coeff_y,
                                 width,
                                 width,
                                 type,
                                 scan_mode);

  scan_pos_sig = scan_pos_last;

  ALIGNED(64) uint16_t abs_coeff[16];
  ALIGNED(32) uint16_t abs_coeff_buf_sb[16];
  ALIGNED(32) int16_t pos_ys_buf[16];
  ALIGNED(32) int16_t pos_xs_buf[16];
  ALIGNED(32) int16_t ctx_sig_buf[16];

  abs_coeff[0] = abs(coeff[pos_last]);
  uint32_t coeff_signs  = (coeff[pos_last] < 0);
  int32_t num_non_zero = 1;
  int32_t last_nz_pos_in_cg  = scan_pos_sig;
  int32_t first_nz_pos_in_cg = scan_pos_sig;
  scan_pos_sig--;

  // significant_coeff_flag
  for (i = scan_cg_last; i >= 0; i--) {
    int32_t sub_pos        = i << 4; // LOG2_SCAN_SET_SIZE;
    int32_t cg_blk_pos     = scan_cg[i];
    int32_t cg_pos_y       = cg_blk_pos / num_blk_side;
    int32_t cg_pos_x       = cg_blk_pos - (cg_pos_y * num_blk_side);

    go_rice_param = 0;

    if (i == scan_cg_last || i == 0) {
      sig_coeffgroup_nzs[cg_blk_pos] = 1;
    } else {
      uint32_t sig_coeff_group   = (sig_coeffgroup_nzs[cg_blk_pos] != 0);
      uint32_t ctx_sig  = kvz_context_get_sig_coeff_group(sig_coeffgroup_nzs, cg_pos_x,
                                                      cg_pos_y, width);
      cabac->cur_ctx = &base_coeff_group_ctx[ctx_sig];
      CABAC_BIN(cabac, sig_coeff_group, "coded_sub_block_flag");
    }

    if (sig_coeffgroup_nzs[cg_blk_pos]) {
      int32_t pattern_sig_ctx = kvz_context_calc_pattern_sig_ctx(sig_coeffgroup_nzs,
                                                             cg_pos_x, cg_pos_y, width);

      // A mask with the first 16-bit word unmasked (bits set ie. 0xffff)
      const __m256i coeff_pos_zero = _mm256_castsi128_si256(_mm_cvtsi32_si128(0xffff));

      const __m128i log2_block_size_128 = _mm_cvtsi32_si128(log2_block_size);

      __m256i coeffs = _mm256_load_si256((__m256i *)(coeff_reord + sub_pos));
      __m256i sigs_inv = _mm256_cmpeq_epi16(coeffs, zero);
      __m256i is = _mm256_set1_epi16(i);
      __m256i is_zero = _mm256_cmpeq_epi16(is, zero);
      __m256i coeffs_negative = _mm256_cmpgt_epi16(zero, coeffs);

      __m256i masked_coeffs = _mm256_andnot_si256(sigs_inv, coeffs);
      __m256i abs_coeffs = _mm256_abs_epi16(masked_coeffs);

      // TODO: obtain 16-bit block positions, maybe? :P
      __m256i blk_poses_hi = _mm256_loadu_si256((__m256i *)(scan + sub_pos + 8));
      __m256i blk_poses_lo = _mm256_loadu_si256((__m256i *)(scan + sub_pos + 0));
      __m256i blk_poses_tmp = _mm256_packs_epi32(blk_poses_lo, blk_poses_hi);
      __m256i blk_poses = _mm256_permute4x64_epi64(blk_poses_tmp, _MM_SHUFFLE(3, 1, 2, 0));

      __m256i pos_ys = _mm256_srl_epi16(blk_poses, log2_block_size_128);
      __m256i pos_xs = _mm256_sub_epi16(blk_poses, _mm256_sll_epi16(pos_ys, log2_block_size_128));

      _mm256_store_si256((__m256i *)pos_ys_buf, pos_ys);
      _mm256_store_si256((__m256i *)pos_xs_buf, pos_xs);

      __m256i encode_sig_coeff_flags_inv = _mm256_andnot_si256(is_zero, coeff_pos_zero);

      get_first_last_nz_int16(masked_coeffs, &first_nz_pos_in_cg, &last_nz_pos_in_cg);
      _mm256_store_si256((__m256i *)abs_coeff_buf_sb, abs_coeffs);

      __m256i ctx_sigs = kvz_context_get_sig_ctx_inc_16x16b(pattern_sig_ctx, scan_mode, pos_xs, pos_ys,
                                             log2_block_size, type);

      _mm256_store_si256((__m256i *)ctx_sig_buf, ctx_sigs);

      uint32_t esc_flags = ~(_mm256_movemask_epi8(encode_sig_coeff_flags_inv));
      uint32_t sigs = ~(_mm256_movemask_epi8(sigs_inv));
      uint32_t coeff_sign_buf = _mm256_movemask_epi8(coeffs_negative);

      for (; scan_pos_sig >= sub_pos; scan_pos_sig--) {
        uint32_t id = scan_pos_sig - sub_pos;
        uint32_t shift = (id << 1) + 1;

        uint32_t curr_sig = (sigs >> shift) & 1;
        uint32_t curr_esc_flag = (esc_flags >> shift) & 1;
        uint32_t curr_coeff_sign = (coeff_sign_buf >> shift) & 1;

        if (curr_esc_flag | num_non_zero) {
          ctx_sig = ctx_sig_buf[id];
          cabac->cur_ctx = &baseCtx[ctx_sig];
          CABAC_BIN(cabac, curr_sig, "sig_coeff_flag");
        }

        if (curr_sig) {
          abs_coeff[num_non_zero]  = abs_coeff_buf_sb[id];
          coeff_signs              = 2 * coeff_signs + curr_coeff_sign;
          num_non_zero++;
        }
      }
    } else {
      scan_pos_sig = sub_pos - 1;
    }

    if (num_non_zero > 0) {
      bool sign_hidden = last_nz_pos_in_cg - first_nz_pos_in_cg >= 4 /* SBH_THRESHOLD */
                         && !encoder->cfg.lossless;
      uint32_t ctx_set  = (i > 0 && type == 0) ? 2 : 0;
      cabac_ctx_t *base_ctx_mod;
      int32_t num_c1_flag, first_c2_flag_idx, idx;

      __m256i abs_coeffs = _mm256_load_si256((__m256i *)abs_coeff);
      __m256i coeffs_gt1 = _mm256_cmpgt_epi16(abs_coeffs, ones);
      __m256i coeffs_gt2 = _mm256_cmpgt_epi16(abs_coeffs, twos);
      uint32_t coeffs_gt1_bits = _mm256_movemask_epi8(coeffs_gt1);
      uint32_t coeffs_gt2_bits = _mm256_movemask_epi8(coeffs_gt2);

      if (c1 == 0) {
        ctx_set++;
      }

      base_ctx_mod     = (type == 0) ? &(cabac->ctx.cu_one_model_luma[4 * ctx_set]) :
                         &(cabac->ctx.cu_one_model_chroma[4 * ctx_set]);
      num_c1_flag      = MIN(num_non_zero, C1FLAG_NUMBER);
      first_c2_flag_idx = -1;


      /*
       * c1s_pattern is 16 base-4 numbers: 3, 3, 3, ... , 3, 2 (c1 will never
       * be less than 0 or greater than 3, so two bits per iter are enough).
       * It's essentially the values that c1 will be for the next iteration as
       * long as we have not encountered any >1 symbols. Count how long run of
       * such symbols there is in the beginning of this CG, and zero all c1's
       * that are located at or after the first >1 symbol.
       */
      const uint32_t c1s_pattern = 0xfffffffe;
      uint32_t n_nongt1_bits = _tzcnt_u32(coeffs_gt1_bits);
      uint32_t c1s_nextiter  = _bzhi_u32(c1s_pattern, n_nongt1_bits);
      first_c2_flag_idx      = n_nongt1_bits >> 1;

      c1 = 1;
      for (idx = 0; idx < num_c1_flag; idx++) {
        uint32_t shift = idx << 1;
        uint32_t symbol = (coeffs_gt1_bits >> shift) & 1;

        cabac->cur_ctx = &base_ctx_mod[c1];
        CABAC_BIN(cabac, symbol, "coeff_abs_level_greater1_flag");

        c1 = (c1s_nextiter >> shift) & 3;
      }

      if (c1 == 0) {
        base_ctx_mod = (type == 0) ? &(cabac->ctx.cu_abs_model_luma[ctx_set]) :
                       &(cabac->ctx.cu_abs_model_chroma[ctx_set]);

        if (first_c2_flag_idx != -1) {
          uint32_t shift = (first_c2_flag_idx << 1) + 1;
          uint8_t symbol = (coeffs_gt2_bits >> shift) & 1;
          cabac->cur_ctx = &base_ctx_mod[0];

          CABAC_BIN(cabac, symbol, "coeff_abs_level_greater2_flag");
        }
      }
      int32_t shiftamt = (be_valid && sign_hidden) ? 1 : 0;
      int32_t nnz = num_non_zero - shiftamt;
      coeff_signs >>= shiftamt;
      if (!cabac->only_count) {
        if (encoder->cfg.crypto_features & KVZ_CRYPTO_TRANSF_COEFF_SIGNS) {
          coeff_signs ^= kvz_crypto_get_key(state->crypto_hdl, nnz);
        }
      }
      CABAC_BINS_EP(cabac, coeff_signs, nnz, "coeff_sign_flag");

      if (c1 == 0 || num_non_zero > C1FLAG_NUMBER) {

        const __m256i ones        = _mm256_set1_epi16(1);

        __m256i abs_coeffs_gt1    = _mm256_cmpgt_epi16  (abs_coeffs, ones);
        uint32_t acgt1_bits       = _mm256_movemask_epi8(abs_coeffs_gt1);
        uint32_t first_acgt1_bpos = _tzcnt_u32(acgt1_bits);

        uint32_t abs_coeffs_base4 = pack_16x16b_to_16x2b(abs_coeffs);

        const uint32_t ones_base4 = 0x55555555;
        const uint32_t twos_base4 = 0xaaaaaaaa;

        const uint32_t c1flag_number_mask_inv = 0xffffffff << (C1FLAG_NUMBER << 1);
        const uint32_t c1flag_number_mask     = ~c1flag_number_mask_inv;

        // The addition will not overflow between 2-bit atoms because
        // first_coeff2s will only be 1 or 0, and the other addend is 2
        uint32_t first_coeff2s    = _bzhi_u32(ones_base4, first_acgt1_bpos + 2);
        uint32_t base_levels      = first_coeff2s + twos_base4;

        base_levels &= c1flag_number_mask;
        base_levels |= (ones_base4 & c1flag_number_mask_inv);

        uint32_t encode_decisions = u32vec_cmpgt_epu2(base_levels, abs_coeffs_base4);

        for (idx = 0; idx < num_non_zero; idx++) {

          uint32_t shift = idx << 1;
          uint32_t dont_encode_curr = (encode_decisions >> shift);
          int16_t base_level        = (base_levels      >> shift) & 3;

          uint16_t curr_abs_coeff = abs_coeff[idx];

          if (!(dont_encode_curr & 2)) {
            uint16_t level_diff = curr_abs_coeff - base_level;
            if (!cabac->only_count && (encoder->cfg.crypto_features & KVZ_CRYPTO_TRANSF_COEFFS)) {
              kvz_cabac_write_coeff_remain_encry(state, cabac, level_diff, go_rice_param, base_level);
            } else {
              kvz_cabac_write_coeff_remain(cabac, level_diff, go_rice_param);
            }

            if (curr_abs_coeff > 3 * (1 << go_rice_param)) {
              go_rice_param = MIN(go_rice_param + 1, 4);
            }
          }

        }
      }
    }
    last_nz_pos_in_cg = -1;
    first_nz_pos_in_cg = 16;
    num_non_zero = 0;
    coeff_signs = 0;
  }
}
#endif // COMPILE_INTEL_AVX2

int kvz_strategy_register_encode_avx2(void* opaque, uint8_t bitdepth)
{
  bool success = true;

#if COMPILE_INTEL_AVX2
  success &= kvz_strategyselector_register(opaque, "encode_coeff_nxn", "avx2", 40, &kvz_encode_coeff_nxn_avx2);
#endif

  return success;
}
