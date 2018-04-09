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

#include "context.h"

#include "tables.h"


static const uint8_t INIT_SAO_MERGE_FLAG[3] = { 153, 153, 153 };
static const uint8_t INIT_SAO_TYPE_IDX[3] = { 160, 185, 200 };

static const uint8_t INIT_QT_ROOT_CBF[3][1] = {
  {  79, },
  {  79, },
  { CNU, },
};

static const uint8_t INIT_MVP_IDX[3][2] = {
  { 168,  CNU, },
  { 168,  CNU, },
  { CNU,  CNU, },
};

static const uint8_t INIT_REF_PIC[3][2] = {
  { 153,  153 },
  { 153,  153 },
  { CNU,  CNU },
};

static const uint8_t INIT_MVD[3][2] = {
  { 169,  198, },
  { 140,  198, },
  { CNU,  CNU, },
};

static const uint8_t INIT_MERGE_FLAG_EXT[3][1] = {
  { 154, },
  { 110, },
  { CNU, },
};

static const uint8_t INIT_MERGE_IDX_EXT[3][1] = {
  { 137, },
  { 122, },
  { CNU, },
};

static const uint8_t INIT_CU_TRANSQUANT_BYPASS[3][1] = {
  { 154, },
  { 154, },
  { 154, },
};

static const uint8_t INIT_SKIP_FLAG[3][3] =  {
  { 197,  185,  201, },
  { 197,  185,  201, },
  { CNU,  CNU,  CNU, },
};

static const uint8_t INIT_PRED_MODE[3][1] = {
  { 134, },
  { 149, },
  { CNU, },
};


static const uint8_t INIT_PART_SIZE[3][4] = {
  { 154,  139,  CNU,  CNU, },
  { 154,  139,  CNU,  CNU, },
  { 184,  CNU,  CNU,  CNU, },
};

static const uint8_t  INIT_SPLIT_FLAG[3][3] = {
  { 107,  139,  126 },
  { 107,  139,  126 },
  { 139,  141,  157 },
};

static const uint8_t INIT_INTRA_PRED_MODE[3] = {
  183, 154, 184
};

static const uint8_t INIT_CHROMA_PRED_MODE[3][2] = {
  { 152,  139 },
  { 152,  139 },
  {  63,  139 },
};

static const uint8_t INIT_INTER_DIR[3][5] = {
  {  95,  79,  63,  31,  31, },
  {  95,  79,  63,  31,  31, },
  { CNU, CNU, CNU, CNU, CNU, },
};

static const uint8_t INIT_TRANS_SUBDIV_FLAG[3][3] = {
  { 224,  167,  122 },
  { 124,  138,   94 },
  { 153,  138,  138 },
};

static const uint8_t INIT_QT_CBF[3][8] = {
  { 153,  111,  CNU,  CNU,   149,   92,  167,  154 },
  { 153,  111,  CNU,  CNU,   149,  107,  167,  154 },
  { 111,  141,  CNU,  CNU,    94,  138,  182,  154 },
};

static const uint8_t INIT_CU_QP_DELTA_ABS[3][2] = {
  { 154, 154 },
  { 154, 154 },
  { 154, 154 },
};

static const uint8_t INIT_SIG_CG_FLAG[3][4] = {
  { 121,  140,  61,  154  },
  { 121,  140,  61,  154 },
  {  91,  171,  134,  141  },
};

static const uint8_t INIT_SIG_FLAG[3][42] = {
   {170,154,139,153,139,123,123, 63,124,166,
    183,140,136,153,154,166,183,140,136,153,
    154,166,183,140,136,153,154,170,153,138,
    138,122,121,122,121,167,151,183,140,151,
    183,140},
   {155,154,139,153,139,123,123,63,153,166,
   183,140,136,153,154,166,183,140,136,153,
   154,166,183,140,136,153,154,170,153,123,
   123,107,121,107,121,167,151,183,140,151,
   183,140},
   {111,111,125,110,110,94,124,108,124,107,
   125,141,179,153,125,107,125,141,179,153,
   125,107,125,141,179,153,125,140,139,182,
   182,152,136,152,136,153,136,139,111,136,
   139,111},
};

static const uint8_t INIT_LAST[3][30] = {
  { 125,  110,  124,  110,   95,   94,  125,  111,  111,   79,  125,  126,  111,  111,   79,
    108,  123,   93,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU  },
  { 125,  110,   94,  110,   95,   79,  125,  111,  110,   78,  110,  111,  111,   95,   94,
    108,  123,  108,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU  },
  { 110,  110,  124,  125,  140,  153,  125,  127,  140,  109,  111,  143,  127,  111,   79,
    108,  123,   63,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU  },
};

static const uint8_t INIT_ONE_FLAG[3][24] =
{
  {154,196,167,167,154,152,167,182,182,134,149,136,153,121,136,122,169,208,166,167,154,152,167,182},
  {154,196,196,167,154,152,167,182,182,134,149,136,153,121,136,137,169,194,166,167,154,167,137,182},
  {140, 92,137,138,140,152,138,139,153, 74,149, 92,139,107,122,152,140,179,166,182,140,227,122,197},
};

static const uint8_t INIT_ABS_FLAG[3][6] =
{
  { 107,167, 91,107,107,167},
  { 107,167, 91,122,107,167},
  { 138,153,136,167,152,152},
};

static const uint8_t INIT_TRANSFORMSKIP_FLAG[3][2] =
{
  { 139,  139},
  { 139,  139},
  { 139,  139},
};




/**
 * \brief Initialize struct cabac_ctx.
 */
void kvz_ctx_init(cabac_ctx_t *ctx, uint32_t qp, uint32_t init_value)
{
  int slope = (init_value >> 4) * 5 - 45;
  int offset = ((init_value & 15) << 3) - 16;
  int init_state = MIN(MAX(1, ((slope * (int)qp) >> 4) + offset), 126);

  if (init_state >= 64) {
    ctx->uc_state = ((init_state - 64) << 1) + 1;
  } else {
    ctx->uc_state = (63 - init_state) << 1;
  }
}

/**
 * \brief Initialize cabac context to be used for coding
 * \param encoder encoder control struct
 * \param slice type of slice we are coding (P/B/I)
 */

void kvz_init_contexts(encoder_state_t *state, int8_t QP, int8_t slice)
{
  cabac_data_t * const cabac = &state->cabac;
  uint16_t i;

  // Initialize contexts
  kvz_ctx_init(&cabac->ctx.transform_skip_model_luma, QP, INIT_TRANSFORMSKIP_FLAG[slice][0]);
  kvz_ctx_init(&cabac->ctx.transform_skip_model_chroma, QP, INIT_TRANSFORMSKIP_FLAG[slice][1]);

  kvz_ctx_init(&cabac->ctx.sao_merge_flag_model, QP, INIT_SAO_MERGE_FLAG[slice]);
  kvz_ctx_init(&cabac->ctx.sao_type_idx_model, QP, INIT_SAO_TYPE_IDX[slice]);

  kvz_ctx_init(&cabac->ctx.cu_merge_flag_ext_model, QP, INIT_MERGE_FLAG_EXT[slice][0]);
  kvz_ctx_init(&cabac->ctx.cu_merge_idx_ext_model, QP, INIT_MERGE_IDX_EXT[slice][0]);
  kvz_ctx_init(&cabac->ctx.cu_pred_mode_model, QP, INIT_PRED_MODE[slice][0]);
  kvz_ctx_init(&cabac->ctx.cu_transquant_bypass, QP, INIT_CU_TRANSQUANT_BYPASS[slice][0]);

  kvz_ctx_init(&cabac->ctx.cu_skip_flag_model[0], QP, INIT_SKIP_FLAG[slice][0]);
  kvz_ctx_init(&cabac->ctx.cu_skip_flag_model[1], QP, INIT_SKIP_FLAG[slice][1]);
  kvz_ctx_init(&cabac->ctx.cu_skip_flag_model[2], QP, INIT_SKIP_FLAG[slice][2]);

  kvz_ctx_init(&cabac->ctx.split_flag_model[0], QP, INIT_SPLIT_FLAG[slice][0]);
  kvz_ctx_init(&cabac->ctx.split_flag_model[1], QP, INIT_SPLIT_FLAG[slice][1]);
  kvz_ctx_init(&cabac->ctx.split_flag_model[2], QP, INIT_SPLIT_FLAG[slice][2]);

  kvz_ctx_init(&cabac->ctx.intra_mode_model, QP, INIT_INTRA_PRED_MODE[slice]);

  kvz_ctx_init(&cabac->ctx.chroma_pred_model[0], QP, INIT_CHROMA_PRED_MODE[slice][0]);
  kvz_ctx_init(&cabac->ctx.chroma_pred_model[1], QP, INIT_CHROMA_PRED_MODE[slice][1]);

  kvz_ctx_init(&cabac->ctx.cu_abs_model_chroma[0], QP, INIT_ABS_FLAG[slice][4]);
  kvz_ctx_init(&cabac->ctx.cu_abs_model_chroma[1], QP, INIT_ABS_FLAG[slice][5]);

  //TODO: ignore P/B contexts on intra frame
  kvz_ctx_init(&cabac->ctx.cu_qt_root_cbf_model, QP, INIT_QT_ROOT_CBF[slice][0]);

  kvz_ctx_init(&cabac->ctx.cu_mvd_model[0], QP, INIT_MVD[slice][0]);
  kvz_ctx_init(&cabac->ctx.cu_mvd_model[1], QP, INIT_MVD[slice][1]);
  kvz_ctx_init(&cabac->ctx.cu_ref_pic_model[0], QP, INIT_REF_PIC[slice][0]);
  kvz_ctx_init(&cabac->ctx.cu_ref_pic_model[1], QP, INIT_REF_PIC[slice][1]);
  kvz_ctx_init(&cabac->ctx.mvp_idx_model[0], QP, INIT_MVP_IDX[slice][0]);
  kvz_ctx_init(&cabac->ctx.mvp_idx_model[1], QP, INIT_MVP_IDX[slice][1]);

  kvz_ctx_init(&cabac->ctx.cu_qp_delta_abs[0], QP, INIT_CU_QP_DELTA_ABS[slice][0]);
  kvz_ctx_init(&cabac->ctx.cu_qp_delta_abs[1], QP, INIT_CU_QP_DELTA_ABS[slice][1]);

  for (i = 0; i < 4; i++) {
    kvz_ctx_init(&cabac->ctx.cu_sig_coeff_group_model[i], QP, INIT_SIG_CG_FLAG[slice][i]);
    kvz_ctx_init(&cabac->ctx.cu_abs_model_luma[i], QP, INIT_ABS_FLAG[slice][i]);
    kvz_ctx_init(&cabac->ctx.part_size_model[i], QP, INIT_PART_SIZE[slice][i]);
  }
  for (i = 0; i < 3; i++) {
    kvz_ctx_init(&cabac->ctx.trans_subdiv_model[i], QP, INIT_TRANS_SUBDIV_FLAG[slice][i]);
  }
  for (i = 0; i < 4; i++) {
    kvz_ctx_init(&cabac->ctx.qt_cbf_model_luma[i], QP, INIT_QT_CBF[slice][i]);
    kvz_ctx_init(&cabac->ctx.qt_cbf_model_chroma[i], QP, INIT_QT_CBF[slice][i + 4]);
  }

  for (i = 0; i < 5; i++) {
    kvz_ctx_init(&cabac->ctx.inter_dir[i], QP, INIT_INTER_DIR[slice][i]);
  }

  for (i = 0; i < 8; i++) {
    kvz_ctx_init(&cabac->ctx.cu_one_model_chroma[i], QP, INIT_ONE_FLAG[slice][i+16]);
  }

  for (i = 0; i < 15; i++) {
    kvz_ctx_init(&cabac->ctx.cu_ctx_last_y_luma[i], QP, INIT_LAST[slice][i] );
    kvz_ctx_init(&cabac->ctx.cu_ctx_last_x_luma[i], QP, INIT_LAST[slice][i] );

    kvz_ctx_init(&cabac->ctx.cu_ctx_last_y_chroma[i], QP, INIT_LAST[slice][i+15] );
    kvz_ctx_init(&cabac->ctx.cu_ctx_last_x_chroma[i], QP, INIT_LAST[slice][i+15] );

    kvz_ctx_init(&cabac->ctx.cu_one_model_luma[i], QP, INIT_ONE_FLAG[slice][i]);
  }
  kvz_ctx_init(&cabac->ctx.cu_one_model_luma[15], QP, INIT_ONE_FLAG[slice][15]);

  for (i = 0; i < 27; i++) {
    kvz_ctx_init(&cabac->ctx.cu_sig_model_luma[i], QP, INIT_SIG_FLAG[slice][i]);
    if(i < 15) kvz_ctx_init(&cabac->ctx.cu_sig_model_chroma[i], QP, INIT_SIG_FLAG[slice][i+27]);
  }
}

void kvz_context_copy(encoder_state_t * const target_state, const encoder_state_t * const source_state) {
  cabac_data_t * const target_cabac = &target_state->cabac;
  const cabac_data_t * const source_cabac = &source_state->cabac;
  
  if (target_cabac == source_cabac) return;

  target_cabac->ctx = source_cabac->ctx;
}


uint32_t kvz_context_get_sig_coeff_group( uint32_t *sig_coeff_group_flag,
                                      uint32_t pos_x,
                                      uint32_t pos_y,
                                      int32_t width)
{
  uint32_t uiRight = 0;
  uint32_t uiLower = 0;
  width >>= 2;
  if (pos_x < (uint32_t)width - 1) uiRight = (sig_coeff_group_flag[pos_y * width + pos_x + 1] != 0);
  if (pos_y < (uint32_t)width - 1) uiLower = (sig_coeff_group_flag[(pos_y  + 1 ) * width + pos_x] != 0);

  return uiRight || uiLower;
}


/**
 * \brief Pattern decision for context derivation process of significant_coeff_flag
 * \param sig_coeff_group_flag pointer to prior coded significant coeff group
 * \param pos_x column of current coefficient group
 * \param pos_y row of current coefficient group
 * \param width width of the block
 * \returns pattern for current coefficient group
 */

int32_t kvz_context_calc_pattern_sig_ctx(const uint32_t *sig_coeff_group_flag, uint32_t pos_x, uint32_t pos_y, int32_t width)
{
  uint32_t sigRight = 0;
  uint32_t sigLower = 0;

  if (width == 4) return -1;

  width >>= 2;
  if (pos_x < (uint32_t)width - 1) sigRight = (sig_coeff_group_flag[pos_y * width + pos_x + 1] != 0);
  if (pos_y < (uint32_t)width - 1) sigLower = (sig_coeff_group_flag[(pos_y  + 1 ) * width + pos_x] != 0);

  return sigRight + (sigLower<<1);
}


/**
 * \brief Context derivation process of coeff_abs_significant_flag
 * \param pattern_sig_ctx pattern for current coefficient group
 * \param scan_idx pixel scan type in use
 * \param pos_x column of current scan position
 * \param pos_y row of current scan position
 * \param block_type log2 value of block size if square block, or 4 otherwise
 * \param width width of the block
 * \param texture_type texture type (TEXT_LUMA...)
 * \returns ctx_inc for current scan position
 */

int32_t kvz_context_get_sig_ctx_inc(int32_t pattern_sig_ctx, uint32_t scan_idx, int32_t pos_x,
                                int32_t pos_y, int32_t block_type, int8_t texture_type)
{
  const int32_t ctx_ind_map[16] =
  {
    0, 1, 4, 5,
    2, 3, 4, 5,
    6, 6, 8, 8,
    7, 7, 8, 8
  };

  int32_t cnt,offset,pos_x_in_subset,pos_y_in_subset;

  if (pos_x + pos_y == 0) return 0;

  if (block_type == 2) return ctx_ind_map[4 * pos_y + pos_x];

  offset = (block_type == 3) ? ((scan_idx == SCAN_DIAG) ? 9 : 15) : ((texture_type == 0) ? 21 : 12);
  pos_x_in_subset = pos_x - ((pos_x>>2)<<2);
  pos_y_in_subset = pos_y - ((pos_y>>2)<<2);

  if (pattern_sig_ctx == 0) {
    cnt = (pos_x_in_subset + pos_y_in_subset <= 2) ? ((pos_x_in_subset + pos_y_in_subset==0) ? 2 : 1) : 0;
  } else if (pattern_sig_ctx==1) {
    cnt = (pos_y_in_subset <= 1) ? ((pos_y_in_subset == 0) ? 2 : 1) : 0;
  } else if (pattern_sig_ctx==2) {
    cnt = (pos_x_in_subset <= 1) ? ((pos_x_in_subset == 0) ? 2 : 1) : 0;
  } else {
    cnt = 2;
  }
  return (( texture_type == 0 && ((pos_x>>2) + (pos_y>>2)) > 0 ) ? 3 : 0) + offset + cnt;
}
