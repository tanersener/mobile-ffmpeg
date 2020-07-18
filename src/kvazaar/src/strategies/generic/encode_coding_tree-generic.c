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

#include "cabac.h"
#include "context.h"
#include "encode_coding_tree-generic.h"
#include "encode_coding_tree.h"

void kvz_encode_coeff_nxn_generic(encoder_state_t * const state,
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
  kvz_encode_last_significant_xy(cabac,
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

int kvz_strategy_register_encode_generic(void* opaque, uint8_t bitdepth)
{
  bool success = true;

  success &= kvz_strategyselector_register(opaque, "encode_coeff_nxn", "generic", 0, &kvz_encode_coeff_nxn_generic);

  return success;
}
