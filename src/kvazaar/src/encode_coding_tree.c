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

#include "encode_coding_tree.h"

#include "cabac.h"
#include "context.h"
#include "cu.h"
#include "encoder.h"
#include "extras/crypto.h"
#include "imagelist.h"
#include "inter.h"
#include "intra.h"
#include "kvazaar.h"
#include "kvz_math.h"
#include "tables.h"
#include "videoframe.h"

/**
 * \brief Encode (X,Y) position of the last significant coefficient
 *
 * \param lastpos_x   X component of last coefficient
 * \param lastpos_y   Y component of last coefficient
 * \param width       Block width
 * \param height      Block height
 * \param type        plane type / luminance or chrominance
 * \param scan        scan type (diag, hor, ver)
 *
 * This method encodes the X and Y component within a block of the last
 * significant coefficient.
 */
static void encode_last_significant_xy(cabac_data_t * const cabac,
                                       uint8_t lastpos_x, uint8_t lastpos_y,
                                       uint8_t width, uint8_t height,
                                       uint8_t type, uint8_t scan)
{
  const int index = kvz_math_floor_log2(width) - 2;
  uint8_t ctx_offset = type ? 0 : (index * 3 + (index + 1) / 4);
  uint8_t shift = type ? index : (index + 3) / 4;

  cabac_ctx_t *base_ctx_x = (type ? cabac->ctx.cu_ctx_last_x_chroma : cabac->ctx.cu_ctx_last_x_luma);
  cabac_ctx_t *base_ctx_y = (type ? cabac->ctx.cu_ctx_last_y_chroma : cabac->ctx.cu_ctx_last_y_luma);

  if (scan == SCAN_VER) {
    SWAP(lastpos_x, lastpos_y, uint8_t);
  }

  const int group_idx_x = g_group_idx[lastpos_x];
  const int group_idx_y = g_group_idx[lastpos_y];

  // x prefix
  for (int last_x = 0; last_x < group_idx_x; last_x++) {
    cabac->cur_ctx = &base_ctx_x[ctx_offset + (last_x >> shift)];
    CABAC_BIN(cabac, 1, "last_sig_coeff_x_prefix");
  }
  if (group_idx_x < g_group_idx[width - 1]) {
    cabac->cur_ctx = &base_ctx_x[ctx_offset + (group_idx_x >> shift)];
    CABAC_BIN(cabac, 0, "last_sig_coeff_x_prefix");
  }

  // y prefix
  for (int last_y = 0; last_y < group_idx_y; last_y++) {
    cabac->cur_ctx = &base_ctx_y[ctx_offset + (last_y >> shift)];
    CABAC_BIN(cabac, 1, "last_sig_coeff_y_prefix");
  }
  if (group_idx_y < g_group_idx[height - 1]) {
    cabac->cur_ctx = &base_ctx_y[ctx_offset + (group_idx_y >> shift)];
    CABAC_BIN(cabac, 0, "last_sig_coeff_y_prefix");
  }

  // last_sig_coeff_x_suffix
  if (group_idx_x > 3) {
    const int suffix = lastpos_x - g_min_in_group[group_idx_x];
    const int bits = (group_idx_x - 2) / 2;
    CABAC_BINS_EP(cabac, suffix, bits, "last_sig_coeff_x_suffix");
  }

  // last_sig_coeff_y_suffix
  if (group_idx_y > 3) {
    const int suffix = lastpos_y - g_min_in_group[group_idx_y];
    const int bits = (group_idx_y - 2) / 2;
    CABAC_BINS_EP(cabac, suffix, bits, "last_sig_coeff_y_suffix");
  }
}

void kvz_encode_coeff_nxn(encoder_state_t * const state,
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
  uint32_t sig_coeffgroup_flag[8 * 8] = { 0 };

  int8_t be_valid = encoder->cfg.signhide_enable;
  int32_t scan_pos_sig;
  uint32_t go_rice_param = 0;
  uint32_t blk_pos, pos_y, pos_x, sig, ctx_sig;

  // CONSTANTS
  const uint32_t num_blk_side    = width >> TR_MIN_LOG2_SIZE;
  const uint32_t log2_block_size = kvz_g_convert_to_bit[width] + 2;
  const uint32_t *scan           =
    kvz_g_sig_last_scan[scan_mode][log2_block_size - 1];
  const uint32_t *scan_cg = g_sig_last_scan_cg[log2_block_size - 2][scan_mode];

  // Init base contexts according to block type
  cabac_ctx_t *base_coeff_group_ctx = &(cabac->ctx.cu_sig_coeff_group_model[type]);
  cabac_ctx_t *baseCtx           = (type == 0) ? &(cabac->ctx.cu_sig_model_luma[0]) :
                                 &(cabac->ctx.cu_sig_model_chroma[0]);

  // Scan all coeff groups to find out which of them have coeffs.
  // Populate sig_coeffgroup_flag with that info.

  unsigned sig_cg_cnt = 0;
  for (int cg_y = 0; cg_y < width / 4; ++cg_y) {
    for (int cg_x = 0; cg_x < width / 4; ++cg_x) {
      unsigned cg_pos = cg_y * width * 4 + cg_x * 4;
      for (int coeff_row = 0; coeff_row < 4; ++coeff_row) {
        // Load four 16-bit coeffs and see if any of them are non-zero.
        unsigned coeff_pos = cg_pos + coeff_row * width;
        uint64_t four_coeffs = *(uint64_t*)(&coeff[coeff_pos]);
        if (four_coeffs) {
          ++sig_cg_cnt;
          unsigned cg_pos_y = (cg_pos >> log2_block_size) >> TR_MIN_LOG2_SIZE;
          unsigned cg_pos_x = (cg_pos & (width - 1)) >> TR_MIN_LOG2_SIZE;
          sig_coeffgroup_flag[cg_pos_x + cg_pos_y * num_blk_side] = 1;
          break;
        }
      }
    }
  }

  // Rest of the code assumes at least one non-zero coeff.
  assert(sig_cg_cnt > 0);

  // Find the last coeff group by going backwards in scan order.
  unsigned scan_cg_last = num_blk_side * num_blk_side - 1;
  while (!sig_coeffgroup_flag[scan_cg[scan_cg_last]]) {
    --scan_cg_last;
  }

  // Find the last coeff by going backwards in scan order.
  unsigned scan_pos_last = scan_cg_last * 16 + 15;
  while (!coeff[scan[scan_pos_last]]) {
    --scan_pos_last;
  }

  int pos_last = scan[scan_pos_last];

  // transform skip flag
  if(width == 4 && encoder->cfg.trskip_enable) {
    cabac->cur_ctx = (type == 0) ? &(cabac->ctx.transform_skip_model_luma) : &(cabac->ctx.transform_skip_model_chroma);
    CABAC_BIN(cabac, tr_skip, "transform_skip_flag");
  }

  last_coeff_x = pos_last & (width - 1);
  last_coeff_y = (uint8_t)(pos_last >> log2_block_size);

  // Code last_coeff_x and last_coeff_y
  encode_last_significant_xy(cabac,
                             last_coeff_x,
                             last_coeff_y,
                             width,
                             width,
                             type,
                             scan_mode);

  scan_pos_sig  = scan_pos_last;

  // significant_coeff_flag
  for (i = scan_cg_last; i >= 0; i--) {
    int32_t sub_pos        = i << 4; // LOG2_SCAN_SET_SIZE;
    int32_t abs_coeff[16];
    int32_t cg_blk_pos     = scan_cg[i];
    int32_t cg_pos_y       = cg_blk_pos / num_blk_side;
    int32_t cg_pos_x       = cg_blk_pos - (cg_pos_y * num_blk_side);

    uint32_t coeff_signs   = 0;
    int32_t last_nz_pos_in_cg = -1;
    int32_t first_nz_pos_in_cg = 16;
    int32_t num_non_zero = 0;
    go_rice_param = 0;

    if (scan_pos_sig == scan_pos_last) {
      abs_coeff[0] = abs(coeff[pos_last]);
      coeff_signs  = (coeff[pos_last] < 0);
      num_non_zero = 1;
      last_nz_pos_in_cg  = scan_pos_sig;
      first_nz_pos_in_cg = scan_pos_sig;
      scan_pos_sig--;
    }

    if (i == scan_cg_last || i == 0) {
      sig_coeffgroup_flag[cg_blk_pos] = 1;
    } else {
      uint32_t sig_coeff_group   = (sig_coeffgroup_flag[cg_blk_pos] != 0);
      uint32_t ctx_sig  = kvz_context_get_sig_coeff_group(sig_coeffgroup_flag, cg_pos_x,
                                                      cg_pos_y, width);
      cabac->cur_ctx = &base_coeff_group_ctx[ctx_sig];
      CABAC_BIN(cabac, sig_coeff_group, "coded_sub_block_flag");
    }

    if (sig_coeffgroup_flag[cg_blk_pos]) {
      int32_t pattern_sig_ctx = kvz_context_calc_pattern_sig_ctx(sig_coeffgroup_flag,
                                                             cg_pos_x, cg_pos_y, width);

      for (; scan_pos_sig >= sub_pos; scan_pos_sig--) {
        blk_pos = scan[scan_pos_sig];
        pos_y   = blk_pos >> log2_block_size;
        pos_x   = blk_pos - (pos_y << log2_block_size);
        sig    = (coeff[blk_pos] != 0) ? 1 : 0;

        if (scan_pos_sig > sub_pos || i == 0 || num_non_zero) {
          ctx_sig  = kvz_context_get_sig_ctx_inc(pattern_sig_ctx, scan_mode, pos_x, pos_y,
                                             log2_block_size, type);
          cabac->cur_ctx = &baseCtx[ctx_sig];
          CABAC_BIN(cabac, sig, "sig_coeff_flag");
        }

        if (sig) {
          abs_coeff[num_non_zero] = abs(coeff[blk_pos]);
          coeff_signs              = 2 * coeff_signs + (coeff[blk_pos] < 0);
          num_non_zero++;

          if (last_nz_pos_in_cg == -1) {
            last_nz_pos_in_cg = scan_pos_sig;
          }

          first_nz_pos_in_cg  = scan_pos_sig;
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
      int32_t num_c1_flag, first_c2_flag_idx, idx, first_coeff2;

      if (c1 == 0) {
        ctx_set++;
      }

      c1 = 1;

      base_ctx_mod     = (type == 0) ? &(cabac->ctx.cu_one_model_luma[4 * ctx_set]) :
                         &(cabac->ctx.cu_one_model_chroma[4 * ctx_set]);
      num_c1_flag      = MIN(num_non_zero, C1FLAG_NUMBER);
      first_c2_flag_idx = -1;

      for (idx = 0; idx < num_c1_flag; idx++) {
        uint32_t symbol = (abs_coeff[idx] > 1) ? 1 : 0;
        cabac->cur_ctx = &base_ctx_mod[c1];
        CABAC_BIN(cabac, symbol, "coeff_abs_level_greater1_flag");

        if (symbol) {
          c1 = 0;

          if (first_c2_flag_idx == -1) {
            first_c2_flag_idx = idx;
          }
        } else if ((c1 < 3) && (c1 > 0)) {
          c1++;
        }
      }

      if (c1 == 0) {
        base_ctx_mod = (type == 0) ? &(cabac->ctx.cu_abs_model_luma[ctx_set]) :
                       &(cabac->ctx.cu_abs_model_chroma[ctx_set]);

        if (first_c2_flag_idx != -1) {
          uint8_t symbol = (abs_coeff[first_c2_flag_idx] > 2) ? 1 : 0;
          cabac->cur_ctx      = &base_ctx_mod[0];
          CABAC_BIN(cabac, symbol, "coeff_abs_level_greater2_flag");
        }
      }
      if (be_valid && sign_hidden) {
    	coeff_signs = coeff_signs >> 1;
    	if (!cabac->only_count)
    	  if (encoder->cfg.crypto_features & KVZ_CRYPTO_TRANSF_COEFF_SIGNS) {
    	    coeff_signs = coeff_signs ^ kvz_crypto_get_key(state->crypto_hdl, num_non_zero-1);
    	  }
        CABAC_BINS_EP(cabac, coeff_signs , (num_non_zero - 1), "coeff_sign_flag");
      } else {
        if (!cabac->only_count)
    	  if (encoder->cfg.crypto_features & KVZ_CRYPTO_TRANSF_COEFF_SIGNS)
    	    coeff_signs = coeff_signs ^ kvz_crypto_get_key(state->crypto_hdl, num_non_zero);
        CABAC_BINS_EP(cabac, coeff_signs, num_non_zero, "coeff_sign_flag");
      }

      if (c1 == 0 || num_non_zero > C1FLAG_NUMBER) {
        first_coeff2 = 1;

        for (idx = 0; idx < num_non_zero; idx++) {
          int32_t base_level  = (idx < C1FLAG_NUMBER) ? (2 + first_coeff2) : 1;

          if (abs_coeff[idx] >= base_level) {
        	if (!cabac->only_count) {
        	  if (encoder->cfg.crypto_features & KVZ_CRYPTO_TRANSF_COEFFS)
                    kvz_cabac_write_coeff_remain_encry(state, cabac, abs_coeff[idx] - base_level, go_rice_param, base_level);
        	  else
        		kvz_cabac_write_coeff_remain(cabac, abs_coeff[idx] - base_level, go_rice_param);
        	} else
              kvz_cabac_write_coeff_remain(cabac, abs_coeff[idx] - base_level, go_rice_param);

            if (abs_coeff[idx] > 3 * (1 << go_rice_param)) {
              go_rice_param = MIN(go_rice_param + 1, 4);
            }
          }

          if (abs_coeff[idx] >= 2) {
            first_coeff2 = 0;
          }
        }
      }
    }
  }
}

static void encode_transform_unit(encoder_state_t * const state,
                                  int x, int y, int depth)
{
  assert(depth >= 1 && depth <= MAX_PU_DEPTH);

  const videoframe_t * const frame = state->tile->frame;
  const uint8_t width = LCU_WIDTH >> depth;
  const uint8_t width_c = (depth == MAX_PU_DEPTH ? width : width / 2);

  const cu_info_t *cur_pu = kvz_cu_array_at_const(frame->cu_array, x, y);

  int8_t scan_idx = kvz_get_scan_order(cur_pu->type, cur_pu->intra.mode, depth);

  int cbf_y = cbf_is_set(cur_pu->cbf, depth, COLOR_Y);

  if (cbf_y) {
    int x_local = x % LCU_WIDTH;
    int y_local = y % LCU_WIDTH;
    const coeff_t *coeff_y = &state->coeff->y[xy_to_zorder(LCU_WIDTH, x_local, y_local)];

    // CoeffNxN
    // Residual Coding
    kvz_encode_coeff_nxn(state,
                         &state->cabac,
                         coeff_y,
                         width,
                         0,
                         scan_idx,
                         cur_pu->intra.tr_skip);
  }

  if (depth == MAX_DEPTH + 1) {
    // For size 4x4 luma transform the corresponding chroma transforms are
    // also of size 4x4 covering 8x8 luma pixels. The residual is coded in
    // the last transform unit.
    if (x % 8 == 0 || y % 8 == 0) {
      // Not the last luma transform block so there is nothing more to do.
      return;
    } else {
      // Time to to code the chroma transform blocks. Move to the top-left
      // corner of the block.
      x -= 4;
      y -= 4;
      cur_pu = kvz_cu_array_at_const(frame->cu_array, x, y);
    }
  }

  bool chroma_cbf_set = cbf_is_set(cur_pu->cbf, depth, COLOR_U) ||
                        cbf_is_set(cur_pu->cbf, depth, COLOR_V);
  if (chroma_cbf_set) {
    int x_local = (x >> 1) % LCU_WIDTH_C;
    int y_local = (y >> 1) % LCU_WIDTH_C;
    scan_idx = kvz_get_scan_order(cur_pu->type, cur_pu->intra.mode_chroma, depth);

    const coeff_t *coeff_u = &state->coeff->u[xy_to_zorder(LCU_WIDTH_C, x_local, y_local)];
    const coeff_t *coeff_v = &state->coeff->v[xy_to_zorder(LCU_WIDTH_C, x_local, y_local)];

    if (cbf_is_set(cur_pu->cbf, depth, COLOR_U)) {
      kvz_encode_coeff_nxn(state, &state->cabac, coeff_u, width_c, 2, scan_idx, 0);
    }

    if (cbf_is_set(cur_pu->cbf, depth, COLOR_V)) {
      kvz_encode_coeff_nxn(state, &state->cabac, coeff_v, width_c, 2, scan_idx, 0);
    }
  }
}

/**
 * \param encoder
 * \param x_pu            Prediction units' x coordinate.
 * \param y_pu            Prediction units' y coordinate.
 * \param depth           Depth from LCU.
 * \param tr_depth        Depth from last CU.
 * \param parent_coeff_u  What was signaled at previous level for cbf_cb.
 * \param parent_coeff_v  What was signlaed at previous level for cbf_cr.
 */
static void encode_transform_coeff(encoder_state_t * const state,
                                   int32_t x,
                                   int32_t y,
                                   int8_t depth,
                                   int8_t tr_depth,
                                   uint8_t parent_coeff_u,
                                   uint8_t parent_coeff_v)
{
  cabac_data_t * const cabac = &state->cabac;
  const encoder_control_t *const ctrl = state->encoder_control;
  const videoframe_t * const frame = state->tile->frame;

  const cu_info_t *cur_pu = kvz_cu_array_at_const(frame->cu_array, x, y);
  // Round coordinates down to a multiple of 8 to get the location of the
  // containing CU.
  const cu_info_t *cur_cu = kvz_cu_array_at_const(frame->cu_array, x & ~7, y & ~7);

  // NxN signifies implicit transform split at the first transform level.
  // There is a similar implicit split for inter, but it is only used when
  // transform hierarchy is not in use.
  int intra_split_flag = (cur_cu->type == CU_INTRA && cur_cu->part_size == SIZE_NxN);

  // The implicit split by intra NxN is not counted towards max_tr_depth.
  int max_tr_depth;
  if (cur_cu->type == CU_INTRA) {
    max_tr_depth = ctrl->cfg.tr_depth_intra + intra_split_flag;
  } else {
    max_tr_depth = ctrl->tr_depth_inter;
  }

  int8_t split = (cur_cu->tr_depth > depth);

  const int cb_flag_y = cbf_is_set(cur_pu->cbf, depth, COLOR_Y);
  const int cb_flag_u = cbf_is_set(cur_cu->cbf, depth, COLOR_U);
  const int cb_flag_v = cbf_is_set(cur_cu->cbf, depth, COLOR_V);

  // The split_transform_flag is not signaled when:
  // - transform size is greater than 32 (depth == 0)
  // - transform size is 4 (depth == MAX_PU_DEPTH)
  // - transform depth is max
  // - cu is intra NxN and it's the first split
  if (depth > 0 &&
      depth < MAX_PU_DEPTH &&
      tr_depth < max_tr_depth &&
      !(intra_split_flag && tr_depth == 0))
  {
    cabac->cur_ctx = &(cabac->ctx.trans_subdiv_model[5 - ((kvz_g_convert_to_bit[LCU_WIDTH] + 2) - depth)]);
    CABAC_BIN(cabac, split, "split_transform_flag");
  }

  // Chroma cb flags are not signaled when one of the following:
  // - transform size is 4 (2x2 chroma transform doesn't exist)
  // - they have already been signaled to 0 previously
  // When they are not present they are inferred to be 0, except for size 4
  // when the flags from previous level are used.
  if (depth < MAX_PU_DEPTH && state->encoder_control->chroma_format != KVZ_CSP_400) {
    cabac->cur_ctx = &(cabac->ctx.qt_cbf_model_chroma[tr_depth]);
    if (tr_depth == 0 || parent_coeff_u) {
      CABAC_BIN(cabac, cb_flag_u, "cbf_cb");
    }
    if (tr_depth == 0 || parent_coeff_v) {
      CABAC_BIN(cabac, cb_flag_v, "cbf_cr");
    }
  }

  if (split) {
    uint8_t offset = LCU_WIDTH >> (depth + 1);
    int x2 = x + offset;
    int y2 = y + offset;
    encode_transform_coeff(state, x,  y,  depth + 1, tr_depth + 1, cb_flag_u, cb_flag_v);
    encode_transform_coeff(state, x2, y,  depth + 1, tr_depth + 1, cb_flag_u, cb_flag_v);
    encode_transform_coeff(state, x,  y2, depth + 1, tr_depth + 1, cb_flag_u, cb_flag_v);
    encode_transform_coeff(state, x2, y2, depth + 1, tr_depth + 1, cb_flag_u, cb_flag_v);
    return;
  }

  // Luma coded block flag is signaled when one of the following:
  // - prediction mode is intra
  // - transform depth > 0
  // - we have chroma coefficients at this level
  // When it is not present, it is inferred to be 1.
  if (cur_cu->type == CU_INTRA || tr_depth > 0 || cb_flag_u || cb_flag_v) {
      cabac->cur_ctx = &(cabac->ctx.qt_cbf_model_luma[!tr_depth]);
      CABAC_BIN(cabac, cb_flag_y, "cbf_luma");
  }

  if (cb_flag_y | cb_flag_u | cb_flag_v) {
    if (state->must_code_qp_delta) {
      const int qp_delta      = state->qp - state->ref_qp;
      const int qp_delta_abs  = ABS(qp_delta);
      cabac_data_t* cabac     = &state->cabac;

      // cu_qp_delta_abs prefix
      cabac->cur_ctx = &cabac->ctx.cu_qp_delta_abs[0];
      kvz_cabac_write_unary_max_symbol(cabac, cabac->ctx.cu_qp_delta_abs, MIN(qp_delta_abs, 5), 1, 5);

      if (qp_delta_abs >= 5) {
        // cu_qp_delta_abs suffix
        kvz_cabac_write_ep_ex_golomb(state, cabac, qp_delta_abs - 5, 0);
      }

      if (qp_delta != 0) {
        CABAC_BIN_EP(cabac, (qp_delta >= 0 ? 0 : 1), "qp_delta_sign_flag");
      }

      state->must_code_qp_delta = false;
      state->ref_qp = state->qp;
    }

    encode_transform_unit(state, x, y, depth);
  }
}

static void encode_inter_prediction_unit(encoder_state_t * const state,
                                         cabac_data_t * const cabac,
                                         const cu_info_t * const cur_cu,
                                         int x, int y, int width, int height,
                                         int depth)
{
  // Mergeflag
  int16_t num_cand = 0;
  cabac->cur_ctx = &(cabac->ctx.cu_merge_flag_ext_model);
  CABAC_BIN(cabac, cur_cu->merged, "MergeFlag");
  num_cand = MRG_MAX_NUM_CANDS;
  if (cur_cu->merged) { //merge
    if (num_cand > 1) {
      int32_t ui;
      for (ui = 0; ui < num_cand - 1; ui++) {
        int32_t symbol = (ui != cur_cu->merge_idx);
        if (ui == 0) {
          cabac->cur_ctx = &(cabac->ctx.cu_merge_idx_ext_model);
          CABAC_BIN(cabac, symbol, "MergeIndex");
        } else {
          CABAC_BIN_EP(cabac,symbol,"MergeIndex");
        }
        if (symbol == 0) break;
      }
    }
  } else {
    uint32_t ref_list_idx;

    // Void TEncSbac::codeInterDir( TComDataCU* pcCU, UInt uiAbsPartIdx )
    if (state->frame->slicetype == KVZ_SLICE_B)
    {
      // Code Inter Dir
      uint8_t inter_dir = cur_cu->inter.mv_dir-1;
      uint8_t ctx = depth;
      

      if (cur_cu->part_size == SIZE_2Nx2N || (LCU_WIDTH >> depth) != 8)
      {
        cabac->cur_ctx = &(cabac->ctx.inter_dir[ctx]);
        CABAC_BIN(cabac, (inter_dir == 2), "inter_pred_idc");
      }
      if (inter_dir < 2)
      {
        cabac->cur_ctx = &(cabac->ctx.inter_dir[4]);
        CABAC_BIN(cabac, inter_dir, "inter_pred_idc");
      }
    }

    for (ref_list_idx = 0; ref_list_idx < 2; ref_list_idx++) {
      if (cur_cu->inter.mv_dir & (1 << ref_list_idx)) {

        // size of the current reference index list (L0/L1)
        uint8_t ref_LX_size = state->frame->ref_LX_size[ref_list_idx];

        if (ref_LX_size > 1) {
          // parseRefFrmIdx
          int32_t ref_frame = cur_cu->inter.mv_ref[ref_list_idx];

          cabac->cur_ctx = &(cabac->ctx.cu_ref_pic_model[0]);
          CABAC_BIN(cabac, (ref_frame != 0), "ref_idx_lX");

          if (ref_frame > 0) {
            int32_t i;
            int32_t ref_num = ref_LX_size - 2;

            cabac->cur_ctx = &(cabac->ctx.cu_ref_pic_model[1]);
            ref_frame--;

            for (i = 0; i < ref_num; ++i) {
              const uint32_t symbol = (i == ref_frame) ? 0 : 1;

              if (i == 0) {
                CABAC_BIN(cabac, symbol, "ref_idx_lX");
              } else {
                CABAC_BIN_EP(cabac, symbol, "ref_idx_lX");
              }
              if (symbol == 0) break;
            }
          }
        }

        if (!(/*pcCU->getSlice()->getMvdL1ZeroFlag() &&*/ state->frame->ref_list == REF_PIC_LIST_1 && cur_cu->inter.mv_dir == 3)) {

          int16_t mv_cand[2][2];
          kvz_inter_get_mv_cand_cua(
              state,
              x, y, width, height,
              mv_cand, cur_cu, ref_list_idx);

          uint8_t cu_mv_cand = CU_GET_MV_CAND(cur_cu, ref_list_idx);

          const int32_t mvd_hor = cur_cu->inter.mv[ref_list_idx][0] - mv_cand[cu_mv_cand][0];
          const int32_t mvd_ver = cur_cu->inter.mv[ref_list_idx][1] - mv_cand[cu_mv_cand][1];
          const int8_t hor_abs_gr0 = mvd_hor != 0;
          const int8_t ver_abs_gr0 = mvd_ver != 0;
          const uint32_t mvd_hor_abs = abs(mvd_hor);
          const uint32_t mvd_ver_abs = abs(mvd_ver);


          cabac->cur_ctx = &(cabac->ctx.cu_mvd_model[0]);
          CABAC_BIN(cabac, (mvd_hor != 0), "abs_mvd_greater0_flag_hor");
          CABAC_BIN(cabac, (mvd_ver != 0), "abs_mvd_greater0_flag_ver");

          cabac->cur_ctx = &(cabac->ctx.cu_mvd_model[1]);

          if (hor_abs_gr0) {
            CABAC_BIN(cabac, (mvd_hor_abs>1), "abs_mvd_greater1_flag_hor");
          }

          if (ver_abs_gr0) {
            CABAC_BIN(cabac, (mvd_ver_abs>1), "abs_mvd_greater1_flag_ver");
          }

          if (hor_abs_gr0) {
            if (mvd_hor_abs > 1) {
              kvz_cabac_write_ep_ex_golomb(state, cabac, mvd_hor_abs-2, 1);
            }
            uint32_t mvd_hor_sign = (mvd_hor>0)?0:1;
            if(!state->cabac.only_count)
              if (state->encoder_control->cfg.crypto_features & KVZ_CRYPTO_MV_SIGNS)
                mvd_hor_sign = mvd_hor_sign^kvz_crypto_get_key(state->crypto_hdl, 1);
            CABAC_BIN_EP(cabac, mvd_hor_sign, "mvd_sign_flag_hor");
          }
          if (ver_abs_gr0) {
            if (mvd_ver_abs > 1) {
              kvz_cabac_write_ep_ex_golomb(state, cabac, mvd_ver_abs-2, 1);
            }
            uint32_t mvd_ver_sign = (mvd_ver>0)?0:1;
            if(!state->cabac.only_count)
              if (state->encoder_control->cfg.crypto_features & KVZ_CRYPTO_MV_SIGNS)
                mvd_ver_sign = mvd_ver_sign^kvz_crypto_get_key(state->crypto_hdl, 1);
            CABAC_BIN_EP(cabac, mvd_ver_sign, "mvd_sign_flag_ver");
          }
        }

        // Signal which candidate MV to use
        kvz_cabac_write_unary_max_symbol(cabac,
                                         cabac->ctx.mvp_idx_model,
                                         CU_GET_MV_CAND(cur_cu, ref_list_idx),
                                         1,
                                         AMVP_MAX_NUM_CANDS - 1);
      }
    } // for ref_list
  } // if !merge
}


static INLINE uint8_t intra_mode_encryption(encoder_state_t * const state,
                                            uint8_t intra_pred_mode)
{
  const uint8_t sets[3][17] =
  {
    {  0,  1,  2,  3,  4,  5, 15, 16, 17, 18, 19, 20, 21, 31, 32, 33, 34},  /* 17 */
    { 22, 23, 24, 25, 27, 28, 29, 30, -1, -1, -1, -1, -1, -1, -1, -1, -1},  /* 9  */
    {  6,  7,  8,  9, 11, 12, 13, 14, -1, -1, -1, -1, -1, -1, -1, -1, -1}   /* 9  */
  };

  const uint8_t nb_elems[3] = {17, 8, 8};

  if (intra_pred_mode == 26 || intra_pred_mode == 10) {
    // correct chroma intra prediction mode
    return intra_pred_mode;

  } else {
    uint8_t keybits, scan_dir, elem_idx=0;

    keybits = kvz_crypto_get_key(state->crypto_hdl, 5);

    scan_dir = SCAN_DIAG;
    if (intra_pred_mode > 5  && intra_pred_mode < 15) {
      scan_dir = SCAN_VER;
    }
    if (intra_pred_mode > 21 && intra_pred_mode < 31) {
      scan_dir = SCAN_HOR;
    }

    for (int i = 0; i < nb_elems[scan_dir]; i++) {
      if (intra_pred_mode == sets[scan_dir][i]) {
        elem_idx = i;
        break;
      }
    }

    keybits = keybits % nb_elems[scan_dir];
    keybits = (elem_idx + keybits) % nb_elems[scan_dir];

    return sets[scan_dir][keybits];
  }
}


static void encode_intra_coding_unit(encoder_state_t * const state,
                                     cabac_data_t * const cabac,
                                     const cu_info_t * const cur_cu,
                                     int x, int y, int depth)
{
  const videoframe_t * const frame = state->tile->frame;
  uint8_t intra_pred_mode_actual[4];
  uint8_t *intra_pred_mode = intra_pred_mode_actual;

#if KVZ_SEL_ENCRYPTION
  const bool do_crypto =
    !state->cabac.only_count &&
    state->encoder_control->cfg.crypto_features & KVZ_CRYPTO_INTRA_MODE;
#else
  const bool do_crypto = false;
#endif

  uint8_t intra_pred_mode_encry[4] = {-1, -1, -1, -1};
  if (do_crypto) {
    intra_pred_mode = intra_pred_mode_encry;
  }

  uint8_t intra_pred_mode_chroma = cur_cu->intra.mode_chroma;
  int8_t intra_preds[4][3] = {{-1, -1, -1},{-1, -1, -1},{-1, -1, -1},{-1, -1, -1}};
  int8_t mpm_preds[4] = {-1, -1, -1, -1};
  uint32_t flag[4];

  #if ENABLE_PCM == 1
  // Code must start after variable initialization
  kvz_cabac_encode_bin_trm(cabac, 0); // IPCMFlag == 0
  #endif

  // PREDINFO CODING
  // If intra prediction mode is found from the predictors,
  // it can be signaled with two EP's. Otherwise we can send
  // 5 EP bins with the full predmode
  const int num_pred_units = kvz_part_mode_num_parts[cur_cu->part_size];
  const int cu_width = LCU_WIDTH >> depth;

  for (int j = 0; j < num_pred_units; ++j) {
    const int pu_x = PU_GET_X(cur_cu->part_size, cu_width, x, j);
    const int pu_y = PU_GET_Y(cur_cu->part_size, cu_width, y, j);
    const cu_info_t *cur_pu = kvz_cu_array_at_const(frame->cu_array, pu_x, pu_y);

    const cu_info_t *left_pu = NULL;
    const cu_info_t *above_pu = NULL;

    if (pu_x > 0) {
      assert(pu_x >> 2 > 0);
      left_pu = kvz_cu_array_at_const(frame->cu_array, pu_x - 1, pu_y);
    }
    // Don't take the above PU across the LCU boundary.
    if (pu_y % LCU_WIDTH > 0 && pu_y > 0) {
      assert(pu_y >> 2 > 0);
      above_pu = kvz_cu_array_at_const(frame->cu_array, pu_x, pu_y - 1);
    }

    if (do_crypto) {
#if KVZ_SEL_ENCRYPTION
      // Need to wrap in preprocessor directives because this function is
      // only defined when KVZ_SEL_ENCRYPTION is defined.
      kvz_intra_get_dir_luma_predictor_encry(pu_x, pu_y,
                                             intra_preds[j],
                                             cur_pu,
                                             left_pu, above_pu);
#endif
    } else {
      kvz_intra_get_dir_luma_predictor(pu_x, pu_y,
                                       intra_preds[j],
                                       cur_pu,
                                       left_pu, above_pu);
    }

    intra_pred_mode_actual[j] = cur_pu->intra.mode;
    if (do_crypto) {
      intra_pred_mode_encry[j] = intra_mode_encryption(state, cur_pu->intra.mode);
    }

    for (int i = 0; i < 3; i++) {
      if (intra_preds[j][i] == intra_pred_mode[j]) {
        mpm_preds[j] = (int8_t)i;
        break;
      }
    }
    flag[j] = (mpm_preds[j] == -1) ? 0 : 1;

#if KVZ_SEL_ENCRYPTION
    // Need to wrap in preprocessor directives because
    // cu_info_t.intra.mode_encry is only defined when KVZ_SEL_ENCRYPTION
    // is defined.
    if (do_crypto) {
      // Set the modified intra_pred_mode of the current pu here to make it
      // available from its neighbours for mpm decision.

      // FIXME: there might be a more efficient way to propagate mode_encry
      // for future use from left and above PUs
      const int pu_width = PU_GET_W(cur_cu->part_size, cu_width, j);
      for (int y = pu_y; y < pu_y + pu_width; y += 4 ) {
        for (int x = pu_x; x < pu_x + pu_width; x += 4) {
          cu_info_t *cu = kvz_cu_array_at(frame->cu_array, x, y);
          cu->intra.mode_encry = intra_pred_mode_encry[j];
        }
      }
    }
#endif
  }

  cabac->cur_ctx = &(cabac->ctx.intra_mode_model);
  for (int j = 0; j < num_pred_units; ++j) {
    CABAC_BIN(cabac, flag[j], "prev_intra_luma_pred_flag");
  }

  for (int j = 0; j < num_pred_units; ++j) {
    // Signal index of the prediction mode in the prediction list.
    if (flag[j]) {
      CABAC_BIN_EP(cabac, (mpm_preds[j] == 0 ? 0 : 1), "mpm_idx");
      if (mpm_preds[j] != 0) {
        CABAC_BIN_EP(cabac, (mpm_preds[j] == 1 ? 0 : 1), "mpm_idx");
      }
    } else {
      // Signal the actual prediction mode.
      int32_t tmp_pred = intra_pred_mode[j];

      // Sort prediction list from lowest to highest.
      if (intra_preds[j][0] > intra_preds[j][1]) SWAP(intra_preds[j][0], intra_preds[j][1], int8_t);
      if (intra_preds[j][0] > intra_preds[j][2]) SWAP(intra_preds[j][0], intra_preds[j][2], int8_t);
      if (intra_preds[j][1] > intra_preds[j][2]) SWAP(intra_preds[j][1], intra_preds[j][2], int8_t);

      // Reduce the index of the signaled prediction mode according to the
      // prediction list, as it has been already signaled that it's not one
      // of the prediction modes.
      for (int i = 2; i >= 0; i--) {
        tmp_pred = (tmp_pred > intra_preds[j][i] ? tmp_pred - 1 : tmp_pred);
      }

      CABAC_BINS_EP(cabac, tmp_pred, 5, "rem_intra_luma_pred_mode");
    }
  }

  // Code chroma prediction mode.
  if (state->encoder_control->chroma_format != KVZ_CSP_400) {
    unsigned pred_mode = 5;
    unsigned chroma_pred_modes[4] = {0, 26, 10, 1};

    if (intra_pred_mode_chroma == intra_pred_mode_actual[0]) {
      pred_mode = 4;
    } else if (intra_pred_mode_chroma == 34) {
      // Angular 34 mode is possible only if intra pred mode is one of the
      // possible chroma pred modes, in which case it is signaled with that
      // duplicate mode.
      for (int i = 0; i < 4; ++i) {
        if (intra_pred_mode_actual[0] == chroma_pred_modes[i]) pred_mode = i;
      }
    } else {
      for (int i = 0; i < 4; ++i) {
        if (intra_pred_mode_chroma == chroma_pred_modes[i]) pred_mode = i;
      }
    }

    // pred_mode == 5 mean intra_pred_mode_chroma is something that can't
    // be coded.
    assert(pred_mode != 5);

    /**
     * Table 9-35 - Binarization for intra_chroma_pred_mode
     *   intra_chroma_pred_mode  bin_string
     *                        4           0
     *                        0         100
     *                        1         101
     *                        2         110
     *                        3         111
     * Table 9-37 - Assignment of ctxInc to syntax elements with context coded bins
     *   intra_chroma_pred_mode[][] = 0, bypass, bypass
     */
    cabac->cur_ctx = &(cabac->ctx.chroma_pred_model[0]);
    if (pred_mode == 4) {
      CABAC_BIN(cabac, 0, "intra_chroma_pred_mode");
    } else {
      CABAC_BIN(cabac, 1, "intra_chroma_pred_mode");
      CABAC_BINS_EP(cabac, pred_mode, 2, "intra_chroma_pred_mode");
    }
  }

  encode_transform_coeff(state, x, y, depth, 0, 0, 0);
}

static void encode_part_mode(encoder_state_t * const state,
                             cabac_data_t * const cabac,
                             const cu_info_t * const cur_cu,
                             int depth)
{
  // Binarization from Table 9-34 of the HEVC spec:
  //
  //                |   log2CbSize >     |    log2CbSize ==
  //                |   MinCbLog2SizeY   |    MinCbLog2SizeY
  // -------+-------+----------+---------+-----------+----------
  //  pred  | part  | AMP      | AMP     |           |
  //  mode  | mode  | disabled | enabled | size == 8 | size > 8
  // -------+-------+----------+---------+-----------+----------
  //  intra | 2Nx2N |        -         - |         1          1
  //        |   NxN |        -         - |         0          0
  // -------+-------+--------------------+----------------------
  //  inter | 2Nx2N |        1         1 |         1          1
  //        |  2NxN |       01       011 |        01         01
  //        |  Nx2N |       00       001 |        00        001
  //        |   NxN |        -         - |         -        000
  //        | 2NxnU |        -      0100 |         -          -
  //        | 2NxnD |        -      0101 |         -          -
  //        | nLx2N |        -      0000 |         -          -
  //        | nRx2N |        -      0001 |         -          -
  // -------+-------+--------------------+----------------------
  //
  //
  // Context indices from Table 9-37 of the HEVC spec:
  //
  //                                      binIdx
  //                               |  0  1  2       3
  // ------------------------------+------------------
  //  log2CbSize == MinCbLog2SizeY |  0  1  2  bypass
  //  log2CbSize >  MinCbLog2SizeY |  0  1  3  bypass
  // ------------------------------+------------------

  if (cur_cu->type == CU_INTRA) {
    if (depth == MAX_DEPTH) {
      cabac->cur_ctx = &(cabac->ctx.part_size_model[0]);
      if (cur_cu->part_size == SIZE_2Nx2N) {
        CABAC_BIN(cabac, 1, "part_mode 2Nx2N");
      } else {
        CABAC_BIN(cabac, 0, "part_mode NxN");
      }
    }
  } else {

    cabac->cur_ctx = &(cabac->ctx.part_size_model[0]);
    if (cur_cu->part_size == SIZE_2Nx2N) {
      CABAC_BIN(cabac, 1, "part_mode 2Nx2N");
      return;
    }
    CABAC_BIN(cabac, 0, "part_mode split");

    cabac->cur_ctx = &(cabac->ctx.part_size_model[1]);
    if (cur_cu->part_size == SIZE_2NxN ||
        cur_cu->part_size == SIZE_2NxnU ||
        cur_cu->part_size == SIZE_2NxnD) {
      CABAC_BIN(cabac, 1, "part_mode vertical");
    } else {
      CABAC_BIN(cabac, 0, "part_mode horizontal");
    }

    if (state->encoder_control->cfg.amp_enable && depth < MAX_DEPTH) {
      cabac->cur_ctx = &(cabac->ctx.part_size_model[3]);

      if (cur_cu->part_size == SIZE_2NxN ||
          cur_cu->part_size == SIZE_Nx2N) {
        CABAC_BIN(cabac, 1, "part_mode SMP");
        return;
      }
      CABAC_BIN(cabac, 0, "part_mode AMP");

      if (cur_cu->part_size == SIZE_2NxnU ||
          cur_cu->part_size == SIZE_nLx2N) {
        CABAC_BINS_EP(cabac, 0, 1, "part_mode AMP");
      } else {
        CABAC_BINS_EP(cabac, 1, 1, "part_mode AMP");
      }
    }
  }
}

void kvz_encode_coding_tree(encoder_state_t * const state,
                            uint16_t x,
                            uint16_t y,
                            uint8_t depth)
{
  cabac_data_t * const cabac = &state->cabac;
  const encoder_control_t * const ctrl = state->encoder_control;
  const videoframe_t * const frame = state->tile->frame;
  const cu_info_t *cur_cu   = kvz_cu_array_at_const(frame->cu_array, x, y);

  const cu_info_t *left_cu  = NULL;
  if (x > 0) {
    left_cu = kvz_cu_array_at_const(frame->cu_array, x - 1, y);
  }
  const cu_info_t *above_cu = NULL;
  if (y > 0) {
    above_cu = kvz_cu_array_at_const(frame->cu_array, x, y - 1);
  }

  uint8_t split_flag = GET_SPLITDATA(cur_cu, depth);
  uint8_t split_model = 0;

  // Absolute coordinates
  uint16_t abs_x = x + state->tile->offset_x;
  uint16_t abs_y = y + state->tile->offset_y;

  // Check for slice border FIXME
  bool border_x = ctrl->in.width  < abs_x + (LCU_WIDTH >> depth);
  bool border_y = ctrl->in.height < abs_y + (LCU_WIDTH >> depth);
  bool border_split_x = ctrl->in.width  >= abs_x + (LCU_WIDTH >> MAX_DEPTH) + (LCU_WIDTH >> (depth + 1));
  bool border_split_y = ctrl->in.height >= abs_y + (LCU_WIDTH >> MAX_DEPTH) + (LCU_WIDTH >> (depth + 1));
  bool border = border_x || border_y; /*!< are we in any border CU */

  // When not in MAX_DEPTH, insert split flag and split the blocks if needed
  if (depth != MAX_DEPTH) {
    // Implisit split flag when on border
    if (!border) {
      // Get left and top block split_flags and if they are present and true, increase model number
      if (left_cu && GET_SPLITDATA(left_cu, depth) == 1) {
        split_model++;
      }

      if (above_cu && GET_SPLITDATA(above_cu, depth) == 1) {
        split_model++;
      }

      cabac->cur_ctx = &(cabac->ctx.split_flag_model[split_model]);
      CABAC_BIN(cabac, split_flag, "SplitFlag");
    }

    if (split_flag || border) {
      // Split blocks and remember to change x and y block positions
      int offset = LCU_WIDTH >> (depth + 1);

      kvz_encode_coding_tree(state, x, y, depth + 1);

      // TODO: fix when other half of the block would not be completely over the border
      if (!border_x || border_split_x) {
        kvz_encode_coding_tree(state, x + offset, y, depth + 1);
      }
      if (!border_y || border_split_y) {
        kvz_encode_coding_tree(state, x, y + offset, depth + 1);
      }
      if (!border || (border_split_x && border_split_y)) {
        kvz_encode_coding_tree(state, x + offset, y + offset, depth + 1);
      }
      return;
    }
  }

  if (state->encoder_control->cfg.lossless) {
    cabac->cur_ctx = &cabac->ctx.cu_transquant_bypass;
    CABAC_BIN(cabac, 1, "cu_transquant_bypass_flag");
  }

  // Encode skip flag
  if (state->frame->slicetype != KVZ_SLICE_I) {
    // uiCtxSkip = aboveskipped + leftskipped;
    int8_t ctx_skip = 0;

    if (left_cu && left_cu->skipped) {
      ctx_skip++;
    }
    if (above_cu && above_cu->skipped) {
      ctx_skip++;
    }

    cabac->cur_ctx = &(cabac->ctx.cu_skip_flag_model[ctx_skip]);
    CABAC_BIN(cabac, cur_cu->skipped, "SkipFlag");

    if (cur_cu->skipped) {
      int16_t num_cand = MRG_MAX_NUM_CANDS;
      if (num_cand > 1) {
        for (int ui = 0; ui < num_cand - 1; ui++) {
          int32_t symbol = (ui != cur_cu->merge_idx);
          if (ui == 0) {
            cabac->cur_ctx = &(cabac->ctx.cu_merge_idx_ext_model);
            CABAC_BIN(cabac, symbol, "MergeIndex");
          } else {
            CABAC_BIN_EP(cabac,symbol,"MergeIndex");
          }
          if (symbol == 0) {
            break;
          }
        }
      }
      return;
    }
  }

  // Prediction mode
  if (state->frame->slicetype != KVZ_SLICE_I) {
    cabac->cur_ctx = &(cabac->ctx.cu_pred_mode_model);
    CABAC_BIN(cabac, (cur_cu->type == CU_INTRA), "PredMode");
  }

  // part_mode
  encode_part_mode(state, cabac, cur_cu, depth);

  if (cur_cu->type == CU_INTER) {
    const int num_pu = kvz_part_mode_num_parts[cur_cu->part_size];
    const int cu_width = LCU_WIDTH >> depth;

    for (int i = 0; i < num_pu; ++i) {
      const int pu_x = PU_GET_X(cur_cu->part_size, cu_width, x, i);
      const int pu_y = PU_GET_Y(cur_cu->part_size, cu_width, y, i);
      const int pu_w = PU_GET_W(cur_cu->part_size, cu_width, i);
      const int pu_h = PU_GET_H(cur_cu->part_size, cu_width, i);
      const cu_info_t *cur_pu = kvz_cu_array_at_const(frame->cu_array, pu_x, pu_y);

      encode_inter_prediction_unit(state, cabac, cur_pu, pu_x, pu_y, pu_w, pu_h, depth);
    }

    {
      int cbf = cbf_is_set_any(cur_cu->cbf, depth);
      // Only need to signal coded block flag if not skipped or merged
      // skip = no coded residual, merge = coded residual
      if (cur_cu->part_size != SIZE_2Nx2N || !cur_cu->merged) {
        cabac->cur_ctx = &(cabac->ctx.cu_qt_root_cbf_model);
        CABAC_BIN(cabac, cbf, "rqt_root_cbf");
      }
      // Code (possible) coeffs to bitstream

      if (cbf) {
        encode_transform_coeff(state, x, y, depth, 0, 0, 0);
      }
    }
  } else if (cur_cu->type == CU_INTRA) {
    encode_intra_coding_unit(state, cabac, cur_cu, x, y, depth);
  }

#if ENABLE_PCM
  // Code IPCM block
  else if (cur_cu->type == CU_PCM) {
    kvz_cabac_encode_bin_trm(cabac, 1); // IPCMFlag == 1
    kvz_cabac_finish(cabac);
    kvz_bitstream_add_rbsp_trailing_bits(cabac.stream);

    // PCM sample
    pixel *base_y = &cur_pic->y_data[x     + y * encoder->in.width];
    pixel *base_u = &cur_pic->u_data[x / 2 + y / 2 * encoder->in.width / 2];
    pixel *base_v = &cur_pic->v_data[x / 2 + y / 2 * encoder->in.width / 2];

    // Luma
    for (unsigned y_px = 0; y_px < LCU_WIDTH >> depth; y_px++) {
      for (unsigned  x_px = 0; x_px < LCU_WIDTH >> depth; x_px++) {
        kvz_bitstream_put(cabac.stream, base_y[x_px + y_px * encoder->in.width], 8);
      }
    }

    // Chroma
    if (encoder->in.video_format != FORMAT_400) {
      for (unsigned y_px = 0; y_px < LCU_WIDTH >> (depth + 1); y_px++) {
        for (unsigned x_px = 0; x_px < LCU_WIDTH >> (depth + 1); x_px++) {
          kvz_bitstream_put(cabac.stream, base_u[x_px + y_px * (encoder->in.width >> 1)], 8);
        }
      }
      for (unsigned y_px = 0; y_px < LCU_WIDTH >> (depth + 1); y_px++) {
        for (unsigned x_px = 0; x_px < LCU_WIDTH >> (depth + 1); x_px++) {
          kvz_bitstream_put(cabac.stream, base_v[x_px + y_px * (encoder->in.width >> 1)], 8);
        }
      }
    }
    kvz_cabac_start(cabac);
  }
#endif

  else {
    // CU type not set. Should not happen.
    assert(0);
    exit(1);
  }
}
