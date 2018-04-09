#ifndef CABAC_H_
#define CABAC_H_
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

/**
 * \ingroup CABAC
 * \file
 * Coding bins using CABAC.
 */

#include "global.h" // IWYU pragma: keep

#include "bitstream.h"

struct encoder_state_t;

// Types
typedef struct
{
  uint8_t  uc_state;
} cabac_ctx_t;

typedef struct
{
  cabac_ctx_t *cur_ctx;
  uint32_t   low;
  uint32_t   range;
  uint32_t   buffered_byte;
  int32_t    num_buffered_bytes;
  int32_t    bits_left;
  int8_t     only_count;
  bitstream_t *stream;

  // CONTEXTS
  struct {
    cabac_ctx_t sao_merge_flag_model;
    cabac_ctx_t sao_type_idx_model;
    cabac_ctx_t split_flag_model[3]; //!< \brief split flag context models
    cabac_ctx_t intra_mode_model;    //!< \brief intra mode context models
    cabac_ctx_t chroma_pred_model[2];
    cabac_ctx_t inter_dir[5];
    cabac_ctx_t trans_subdiv_model[3]; //!< \brief intra mode context models
    cabac_ctx_t qt_cbf_model_luma[4];
    cabac_ctx_t qt_cbf_model_chroma[4];
    cabac_ctx_t cu_qp_delta_abs[4];
    cabac_ctx_t part_size_model[4];
    cabac_ctx_t cu_sig_coeff_group_model[4];
    cabac_ctx_t cu_sig_model_luma[27];
    cabac_ctx_t cu_sig_model_chroma[15];
    cabac_ctx_t cu_ctx_last_y_luma[15];
    cabac_ctx_t cu_ctx_last_y_chroma[15];
    cabac_ctx_t cu_ctx_last_x_luma[15];
    cabac_ctx_t cu_ctx_last_x_chroma[15];
    cabac_ctx_t cu_one_model_luma[16];
    cabac_ctx_t cu_one_model_chroma[8];
    cabac_ctx_t cu_abs_model_luma[4];
    cabac_ctx_t cu_abs_model_chroma[2];
    cabac_ctx_t cu_pred_mode_model;
    cabac_ctx_t cu_skip_flag_model[3];
    cabac_ctx_t cu_merge_idx_ext_model;
    cabac_ctx_t cu_merge_flag_ext_model;
    cabac_ctx_t cu_transquant_bypass;
    cabac_ctx_t cu_mvd_model[2];
    cabac_ctx_t cu_ref_pic_model[2];
    cabac_ctx_t mvp_idx_model[2];
    cabac_ctx_t cu_qt_root_cbf_model;
    cabac_ctx_t transform_skip_model_luma;
    cabac_ctx_t transform_skip_model_chroma;
  } ctx;
} cabac_data_t;


// Globals
extern const uint8_t kvz_g_auc_next_state_mps[128];
extern const uint8_t kvz_g_auc_next_state_lps[128];
extern const uint8_t kvz_g_auc_lpst_table[64][4];
extern const uint8_t kvz_g_auc_renorm_table[32];


// Functions
void kvz_cabac_start(cabac_data_t *data);
void kvz_cabac_encode_bin(cabac_data_t *data, uint32_t bin_value);
void kvz_cabac_encode_bin_ep(cabac_data_t *data, uint32_t bin_value);
void kvz_cabac_encode_bins_ep(cabac_data_t *data, uint32_t bin_values, int num_bins);
void kvz_cabac_encode_bin_trm(cabac_data_t *data, uint8_t bin_value);
void kvz_cabac_write(cabac_data_t *data);
void kvz_cabac_finish(cabac_data_t *data);
void kvz_cabac_write_coeff_remain(cabac_data_t *cabac, uint32_t symbol,
                              uint32_t r_param);
void kvz_cabac_write_coeff_remain_encry(struct encoder_state_t * const state, cabac_data_t * const cabac, const uint32_t symbol,
		const uint32_t r_param, int32_t base_level);
void kvz_cabac_write_ep_ex_golomb(struct encoder_state_t * const state, cabac_data_t *data,
								uint32_t symbol, uint32_t count);
void kvz_cabac_write_unary_max_symbol(cabac_data_t *data, cabac_ctx_t *ctx,
                                  uint32_t symbol, int32_t offset,
                                  uint32_t max_symbol);
void kvz_cabac_write_unary_max_symbol_ep(cabac_data_t *data, unsigned int symbol, unsigned int max_symbol);


// Macros
#define CTX_STATE(ctx) ((ctx)->uc_state >> 1)
#define CTX_MPS(ctx) ((ctx)->uc_state & 1)
#define CTX_UPDATE_LPS(ctx) { (ctx)->uc_state = kvz_g_auc_next_state_lps[ (ctx)->uc_state ]; }
#define CTX_UPDATE_MPS(ctx) { (ctx)->uc_state = kvz_g_auc_next_state_mps[ (ctx)->uc_state ]; }

#ifdef VERBOSE
  #define CABAC_BIN(data, value, name) { \
    uint32_t prev_state = (data)->ctx->uc_state; \
    kvz_cabac_encode_bin((data), (value)) \
    printf("%s = %u, state = %u -> %u\n", \
           (name), (uint32_t)(value), prev_state, (data)->ctx->uc_state); }

  #define CABAC_BINS_EP(data, value, bins, name) { \
    uint32_t prev_state = (data)->ctx->uc_state; \
    kvz_cabac_encode_bins_ep((data), (value), (bins)); \
    printf("%s = %u(%u bins), state = %u -> %u\n", \
           (name), (uint32_t)(value), (bins), prev_state, (data)->ctx->uc_state); }

  #define CABAC_BIN_EP(data, value, name) { \
    uint32_t prev_state = (data)->ctx->uc_state; \
    kvz_cabac_encode_bin_ep((data), (value)); \
    printf("%s = %u, state = %u -> %u\n", \
           (name), (uint32_t)(value), prev_state, (data)->ctx->uc_state); }
#else
  #define CABAC_BIN(data, value, name) \
    kvz_cabac_encode_bin((data), (value));
  #define CABAC_BINS_EP(data, value, bins, name) \
    kvz_cabac_encode_bins_ep((data), (value), (bins));
  #define CABAC_BIN_EP(data, value, name) \
    kvz_cabac_encode_bin_ep((data), (value));
#endif

#endif
