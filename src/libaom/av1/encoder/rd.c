/*
 * Copyright (c) 2016, Alliance for Open Media. All rights reserved
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */

#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "config/av1_rtcd.h"

#include "aom_dsp/aom_dsp_common.h"
#include "aom_mem/aom_mem.h"
#include "aom_ports/bitops.h"
#include "aom_ports/mem.h"
#include "aom_ports/system_state.h"

#include "av1/common/common.h"
#include "av1/common/entropy.h"
#include "av1/common/entropymode.h"
#include "av1/common/mvref_common.h"
#include "av1/common/pred_common.h"
#include "av1/common/quant_common.h"
#include "av1/common/reconinter.h"
#include "av1/common/reconintra.h"
#include "av1/common/seg_common.h"

#include "av1/encoder/av1_quantize.h"
#include "av1/encoder/cost.h"
#include "av1/encoder/encodemb.h"
#include "av1/encoder/encodemv.h"
#include "av1/encoder/encoder.h"
#include "av1/encoder/encodetxb.h"
#include "av1/encoder/mcomp.h"
#include "av1/encoder/ratectrl.h"
#include "av1/encoder/rd.h"
#include "av1/encoder/tokenize.h"

#define RD_THRESH_POW 1.25

// The baseline rd thresholds for breaking out of the rd loop for
// certain modes are assumed to be based on 8x8 blocks.
// This table is used to correct for block size.
// The factors here are << 2 (2 = x0.5, 32 = x8 etc).
static const uint8_t rd_thresh_block_size_factor[BLOCK_SIZES_ALL] = {
  2, 3, 3, 4, 6, 6, 8, 12, 12, 16, 24, 24, 32, 48, 48, 64, 4, 4, 8, 8, 16, 16
};

static const int use_intra_ext_tx_for_txsize[EXT_TX_SETS_INTRA][EXT_TX_SIZES] =
    {
      { 1, 1, 1, 1 },  // unused
      { 1, 1, 0, 0 },
      { 0, 0, 1, 0 },
    };

static const int use_inter_ext_tx_for_txsize[EXT_TX_SETS_INTER][EXT_TX_SIZES] =
    {
      { 1, 1, 1, 1 },  // unused
      { 1, 1, 0, 0 },
      { 0, 0, 1, 0 },
      { 0, 0, 0, 1 },
    };

static const int av1_ext_tx_set_idx_to_type[2][AOMMAX(EXT_TX_SETS_INTRA,
                                                      EXT_TX_SETS_INTER)] = {
  {
      // Intra
      EXT_TX_SET_DCTONLY,
      EXT_TX_SET_DTT4_IDTX_1DDCT,
      EXT_TX_SET_DTT4_IDTX,
  },
  {
      // Inter
      EXT_TX_SET_DCTONLY,
      EXT_TX_SET_ALL16,
      EXT_TX_SET_DTT9_IDTX_1DDCT,
      EXT_TX_SET_DCT_IDTX,
  },
};

void av1_fill_mode_rates(AV1_COMMON *const cm, MACROBLOCK *x,
                         FRAME_CONTEXT *fc) {
  int i, j;

  for (i = 0; i < PARTITION_CONTEXTS; ++i)
    av1_cost_tokens_from_cdf(x->partition_cost[i], fc->partition_cdf[i], NULL);

  if (cm->current_frame.skip_mode_info.skip_mode_flag) {
    for (i = 0; i < SKIP_CONTEXTS; ++i) {
      av1_cost_tokens_from_cdf(x->skip_mode_cost[i], fc->skip_mode_cdfs[i],
                               NULL);
    }
  }

  for (i = 0; i < SKIP_CONTEXTS; ++i) {
    av1_cost_tokens_from_cdf(x->skip_cost[i], fc->skip_cdfs[i], NULL);
  }

  for (i = 0; i < KF_MODE_CONTEXTS; ++i)
    for (j = 0; j < KF_MODE_CONTEXTS; ++j)
      av1_cost_tokens_from_cdf(x->y_mode_costs[i][j], fc->kf_y_cdf[i][j], NULL);

  for (i = 0; i < BLOCK_SIZE_GROUPS; ++i)
    av1_cost_tokens_from_cdf(x->mbmode_cost[i], fc->y_mode_cdf[i], NULL);
  for (i = 0; i < CFL_ALLOWED_TYPES; ++i)
    for (j = 0; j < INTRA_MODES; ++j)
      av1_cost_tokens_from_cdf(x->intra_uv_mode_cost[i][j],
                               fc->uv_mode_cdf[i][j], NULL);

  av1_cost_tokens_from_cdf(x->filter_intra_mode_cost, fc->filter_intra_mode_cdf,
                           NULL);
  for (i = 0; i < BLOCK_SIZES_ALL; ++i) {
    if (av1_filter_intra_allowed_bsize(cm, i))
      av1_cost_tokens_from_cdf(x->filter_intra_cost[i],
                               fc->filter_intra_cdfs[i], NULL);
  }

  for (i = 0; i < SWITCHABLE_FILTER_CONTEXTS; ++i)
    av1_cost_tokens_from_cdf(x->switchable_interp_costs[i],
                             fc->switchable_interp_cdf[i], NULL);

  for (i = 0; i < PALATTE_BSIZE_CTXS; ++i) {
    av1_cost_tokens_from_cdf(x->palette_y_size_cost[i],
                             fc->palette_y_size_cdf[i], NULL);
    av1_cost_tokens_from_cdf(x->palette_uv_size_cost[i],
                             fc->palette_uv_size_cdf[i], NULL);
    for (j = 0; j < PALETTE_Y_MODE_CONTEXTS; ++j) {
      av1_cost_tokens_from_cdf(x->palette_y_mode_cost[i][j],
                               fc->palette_y_mode_cdf[i][j], NULL);
    }
  }

  for (i = 0; i < PALETTE_UV_MODE_CONTEXTS; ++i) {
    av1_cost_tokens_from_cdf(x->palette_uv_mode_cost[i],
                             fc->palette_uv_mode_cdf[i], NULL);
  }

  for (i = 0; i < PALETTE_SIZES; ++i) {
    for (j = 0; j < PALETTE_COLOR_INDEX_CONTEXTS; ++j) {
      av1_cost_tokens_from_cdf(x->palette_y_color_cost[i][j],
                               fc->palette_y_color_index_cdf[i][j], NULL);
      av1_cost_tokens_from_cdf(x->palette_uv_color_cost[i][j],
                               fc->palette_uv_color_index_cdf[i][j], NULL);
    }
  }

  int sign_cost[CFL_JOINT_SIGNS];
  av1_cost_tokens_from_cdf(sign_cost, fc->cfl_sign_cdf, NULL);
  for (int joint_sign = 0; joint_sign < CFL_JOINT_SIGNS; joint_sign++) {
    int *cost_u = x->cfl_cost[joint_sign][CFL_PRED_U];
    int *cost_v = x->cfl_cost[joint_sign][CFL_PRED_V];
    if (CFL_SIGN_U(joint_sign) == CFL_SIGN_ZERO) {
      memset(cost_u, 0, CFL_ALPHABET_SIZE * sizeof(*cost_u));
    } else {
      const aom_cdf_prob *cdf_u = fc->cfl_alpha_cdf[CFL_CONTEXT_U(joint_sign)];
      av1_cost_tokens_from_cdf(cost_u, cdf_u, NULL);
    }
    if (CFL_SIGN_V(joint_sign) == CFL_SIGN_ZERO) {
      memset(cost_v, 0, CFL_ALPHABET_SIZE * sizeof(*cost_v));
    } else {
      const aom_cdf_prob *cdf_v = fc->cfl_alpha_cdf[CFL_CONTEXT_V(joint_sign)];
      av1_cost_tokens_from_cdf(cost_v, cdf_v, NULL);
    }
    for (int u = 0; u < CFL_ALPHABET_SIZE; u++)
      cost_u[u] += sign_cost[joint_sign];
  }

  for (i = 0; i < MAX_TX_CATS; ++i)
    for (j = 0; j < TX_SIZE_CONTEXTS; ++j)
      av1_cost_tokens_from_cdf(x->tx_size_cost[i][j], fc->tx_size_cdf[i][j],
                               NULL);

  for (i = 0; i < TXFM_PARTITION_CONTEXTS; ++i) {
    av1_cost_tokens_from_cdf(x->txfm_partition_cost[i],
                             fc->txfm_partition_cdf[i], NULL);
  }

  for (i = TX_4X4; i < EXT_TX_SIZES; ++i) {
    int s;
    for (s = 1; s < EXT_TX_SETS_INTER; ++s) {
      if (use_inter_ext_tx_for_txsize[s][i]) {
        av1_cost_tokens_from_cdf(
            x->inter_tx_type_costs[s][i], fc->inter_ext_tx_cdf[s][i],
            av1_ext_tx_inv[av1_ext_tx_set_idx_to_type[1][s]]);
      }
    }
    for (s = 1; s < EXT_TX_SETS_INTRA; ++s) {
      if (use_intra_ext_tx_for_txsize[s][i]) {
        for (j = 0; j < INTRA_MODES; ++j) {
          av1_cost_tokens_from_cdf(
              x->intra_tx_type_costs[s][i][j], fc->intra_ext_tx_cdf[s][i][j],
              av1_ext_tx_inv[av1_ext_tx_set_idx_to_type[0][s]]);
        }
      }
    }
  }
  for (i = 0; i < DIRECTIONAL_MODES; ++i) {
    av1_cost_tokens_from_cdf(x->angle_delta_cost[i], fc->angle_delta_cdf[i],
                             NULL);
  }
  av1_cost_tokens_from_cdf(x->switchable_restore_cost,
                           fc->switchable_restore_cdf, NULL);
  av1_cost_tokens_from_cdf(x->wiener_restore_cost, fc->wiener_restore_cdf,
                           NULL);
  av1_cost_tokens_from_cdf(x->sgrproj_restore_cost, fc->sgrproj_restore_cdf,
                           NULL);
  av1_cost_tokens_from_cdf(x->intrabc_cost, fc->intrabc_cdf, NULL);

  if (!frame_is_intra_only(cm)) {
    for (i = 0; i < COMP_INTER_CONTEXTS; ++i) {
      av1_cost_tokens_from_cdf(x->comp_inter_cost[i], fc->comp_inter_cdf[i],
                               NULL);
    }

    for (i = 0; i < REF_CONTEXTS; ++i) {
      for (j = 0; j < SINGLE_REFS - 1; ++j) {
        av1_cost_tokens_from_cdf(x->single_ref_cost[i][j],
                                 fc->single_ref_cdf[i][j], NULL);
      }
    }

    for (i = 0; i < COMP_REF_TYPE_CONTEXTS; ++i) {
      av1_cost_tokens_from_cdf(x->comp_ref_type_cost[i],
                               fc->comp_ref_type_cdf[i], NULL);
    }

    for (i = 0; i < UNI_COMP_REF_CONTEXTS; ++i) {
      for (j = 0; j < UNIDIR_COMP_REFS - 1; ++j) {
        av1_cost_tokens_from_cdf(x->uni_comp_ref_cost[i][j],
                                 fc->uni_comp_ref_cdf[i][j], NULL);
      }
    }

    for (i = 0; i < REF_CONTEXTS; ++i) {
      for (j = 0; j < FWD_REFS - 1; ++j) {
        av1_cost_tokens_from_cdf(x->comp_ref_cost[i][j], fc->comp_ref_cdf[i][j],
                                 NULL);
      }
    }

    for (i = 0; i < REF_CONTEXTS; ++i) {
      for (j = 0; j < BWD_REFS - 1; ++j) {
        av1_cost_tokens_from_cdf(x->comp_bwdref_cost[i][j],
                                 fc->comp_bwdref_cdf[i][j], NULL);
      }
    }

    for (i = 0; i < INTRA_INTER_CONTEXTS; ++i) {
      av1_cost_tokens_from_cdf(x->intra_inter_cost[i], fc->intra_inter_cdf[i],
                               NULL);
    }

    for (i = 0; i < NEWMV_MODE_CONTEXTS; ++i) {
      av1_cost_tokens_from_cdf(x->newmv_mode_cost[i], fc->newmv_cdf[i], NULL);
    }

    for (i = 0; i < GLOBALMV_MODE_CONTEXTS; ++i) {
      av1_cost_tokens_from_cdf(x->zeromv_mode_cost[i], fc->zeromv_cdf[i], NULL);
    }

    for (i = 0; i < REFMV_MODE_CONTEXTS; ++i) {
      av1_cost_tokens_from_cdf(x->refmv_mode_cost[i], fc->refmv_cdf[i], NULL);
    }

    for (i = 0; i < DRL_MODE_CONTEXTS; ++i) {
      av1_cost_tokens_from_cdf(x->drl_mode_cost0[i], fc->drl_cdf[i], NULL);
    }
    for (i = 0; i < INTER_MODE_CONTEXTS; ++i)
      av1_cost_tokens_from_cdf(x->inter_compound_mode_cost[i],
                               fc->inter_compound_mode_cdf[i], NULL);
    for (i = 0; i < BLOCK_SIZES_ALL; ++i)
      av1_cost_tokens_from_cdf(x->compound_type_cost[i],
                               fc->compound_type_cdf[i], NULL);
    for (i = 0; i < BLOCK_SIZES_ALL; ++i) {
      if (get_interinter_wedge_bits(i)) {
        av1_cost_tokens_from_cdf(x->wedge_idx_cost[i], fc->wedge_idx_cdf[i],
                                 NULL);
      }
    }
    for (i = 0; i < BLOCK_SIZE_GROUPS; ++i) {
      av1_cost_tokens_from_cdf(x->interintra_cost[i], fc->interintra_cdf[i],
                               NULL);
      av1_cost_tokens_from_cdf(x->interintra_mode_cost[i],
                               fc->interintra_mode_cdf[i], NULL);
    }
    for (i = 0; i < BLOCK_SIZES_ALL; ++i) {
      av1_cost_tokens_from_cdf(x->wedge_interintra_cost[i],
                               fc->wedge_interintra_cdf[i], NULL);
    }
    for (i = BLOCK_8X8; i < BLOCK_SIZES_ALL; i++) {
      av1_cost_tokens_from_cdf(x->motion_mode_cost[i], fc->motion_mode_cdf[i],
                               NULL);
    }
    for (i = BLOCK_8X8; i < BLOCK_SIZES_ALL; i++) {
      av1_cost_tokens_from_cdf(x->motion_mode_cost1[i], fc->obmc_cdf[i], NULL);
    }
    for (i = 0; i < COMP_INDEX_CONTEXTS; ++i) {
      av1_cost_tokens_from_cdf(x->comp_idx_cost[i], fc->compound_index_cdf[i],
                               NULL);
    }
    for (i = 0; i < COMP_GROUP_IDX_CONTEXTS; ++i) {
      av1_cost_tokens_from_cdf(x->comp_group_idx_cost[i],
                               fc->comp_group_idx_cdf[i], NULL);
    }
  }
}

// Values are now correlated to quantizer.
static int sad_per_bit16lut_8[QINDEX_RANGE];
static int sad_per_bit4lut_8[QINDEX_RANGE];
static int sad_per_bit16lut_10[QINDEX_RANGE];
static int sad_per_bit4lut_10[QINDEX_RANGE];
static int sad_per_bit16lut_12[QINDEX_RANGE];
static int sad_per_bit4lut_12[QINDEX_RANGE];

static void init_me_luts_bd(int *bit16lut, int *bit4lut, int range,
                            aom_bit_depth_t bit_depth) {
  int i;
  // Initialize the sad lut tables using a formulaic calculation for now.
  // This is to make it easier to resolve the impact of experimental changes
  // to the quantizer tables.
  for (i = 0; i < range; i++) {
    const double q = av1_convert_qindex_to_q(i, bit_depth);
    bit16lut[i] = (int)(0.0418 * q + 2.4107);
    bit4lut[i] = (int)(0.063 * q + 2.742);
  }
}

void av1_init_me_luts(void) {
  init_me_luts_bd(sad_per_bit16lut_8, sad_per_bit4lut_8, QINDEX_RANGE,
                  AOM_BITS_8);
  init_me_luts_bd(sad_per_bit16lut_10, sad_per_bit4lut_10, QINDEX_RANGE,
                  AOM_BITS_10);
  init_me_luts_bd(sad_per_bit16lut_12, sad_per_bit4lut_12, QINDEX_RANGE,
                  AOM_BITS_12);
}

static const int rd_boost_factor[16] = { 64, 32, 32, 32, 24, 16, 12, 12,
                                         8,  8,  4,  4,  2,  2,  1,  0 };
static const int rd_frame_type_factor[FRAME_UPDATE_TYPES] = {
  128, 144, 128, 128, 144,
  // TODO(zoeliu): To adjust further following factor values.
  128, 128, 128,
  // TODO(weitinglin): We should investigate if the values should be the same
  //                   as the value used by OVERLAY frame
  144,  // INTNL_OVERLAY_UPDATE
  128   // INTNL_ARF_UPDATE
};

int av1_compute_rd_mult_based_on_qindex(const AV1_COMP *cpi, int qindex) {
  const int q = av1_dc_quant_Q3(qindex, 0, cpi->common.seq_params.bit_depth);
  int rdmult = q * q;
  rdmult = rdmult * 3 + (rdmult * 2 / 3);
  switch (cpi->common.seq_params.bit_depth) {
    case AOM_BITS_8: break;
    case AOM_BITS_10: rdmult = ROUND_POWER_OF_TWO(rdmult, 4); break;
    case AOM_BITS_12: rdmult = ROUND_POWER_OF_TWO(rdmult, 8); break;
    default:
      assert(0 && "bit_depth should be AOM_BITS_8, AOM_BITS_10 or AOM_BITS_12");
      return -1;
  }
  return rdmult > 0 ? rdmult : 1;
}

int av1_compute_rd_mult(const AV1_COMP *cpi, int qindex) {
  int64_t rdmult = av1_compute_rd_mult_based_on_qindex(cpi, qindex);
  if (cpi->oxcf.pass == 2 &&
      (cpi->common.current_frame.frame_type != KEY_FRAME)) {
    const GF_GROUP *const gf_group = &cpi->twopass.gf_group;
    const FRAME_UPDATE_TYPE frame_type = gf_group->update_type[gf_group->index];
    const int boost_index = AOMMIN(15, (cpi->rc.gfu_boost / 100));

    rdmult = (rdmult * rd_frame_type_factor[frame_type]) >> 7;
    rdmult += ((rdmult * rd_boost_factor[boost_index]) >> 7);
  }
  return (int)rdmult;
}

int av1_get_adaptive_rdmult(const AV1_COMP *cpi, double beta) {
  const AV1_COMMON *cm = &cpi->common;
  int64_t q =
      av1_dc_quant_Q3(cm->base_qindex, 0, cpi->common.seq_params.bit_depth);
  int64_t rdmult = 0;

  switch (cpi->common.seq_params.bit_depth) {
    case AOM_BITS_8: rdmult = (int)((88 * q * q / beta) / 24); break;
    case AOM_BITS_10:
      rdmult = ROUND_POWER_OF_TWO((int)((88 * q * q / beta) / 24), 4);
      break;
    default:
      assert(cpi->common.seq_params.bit_depth == AOM_BITS_12);
      rdmult = ROUND_POWER_OF_TWO((int)((88 * q * q / beta) / 24), 8);
      break;
  }

  if (cpi->oxcf.pass == 2 &&
      (cpi->common.current_frame.frame_type != KEY_FRAME)) {
    const GF_GROUP *const gf_group = &cpi->twopass.gf_group;
    const FRAME_UPDATE_TYPE frame_type = gf_group->update_type[gf_group->index];
    const int boost_index = AOMMIN(15, (cpi->rc.gfu_boost / 100));

    rdmult = (rdmult * rd_frame_type_factor[frame_type]) >> 7;
    rdmult += ((rdmult * rd_boost_factor[boost_index]) >> 7);
  }
  if (rdmult < 1) rdmult = 1;
  return (int)rdmult;
}

static int compute_rd_thresh_factor(int qindex, aom_bit_depth_t bit_depth) {
  double q;
  switch (bit_depth) {
    case AOM_BITS_8: q = av1_dc_quant_Q3(qindex, 0, AOM_BITS_8) / 4.0; break;
    case AOM_BITS_10: q = av1_dc_quant_Q3(qindex, 0, AOM_BITS_10) / 16.0; break;
    case AOM_BITS_12: q = av1_dc_quant_Q3(qindex, 0, AOM_BITS_12) / 64.0; break;
    default:
      assert(0 && "bit_depth should be AOM_BITS_8, AOM_BITS_10 or AOM_BITS_12");
      return -1;
  }
  // TODO(debargha): Adjust the function below.
  return AOMMAX((int)(pow(q, RD_THRESH_POW) * 5.12), 8);
}

void av1_initialize_me_consts(const AV1_COMP *cpi, MACROBLOCK *x, int qindex) {
  switch (cpi->common.seq_params.bit_depth) {
    case AOM_BITS_8:
      x->sadperbit16 = sad_per_bit16lut_8[qindex];
      x->sadperbit4 = sad_per_bit4lut_8[qindex];
      break;
    case AOM_BITS_10:
      x->sadperbit16 = sad_per_bit16lut_10[qindex];
      x->sadperbit4 = sad_per_bit4lut_10[qindex];
      break;
    case AOM_BITS_12:
      x->sadperbit16 = sad_per_bit16lut_12[qindex];
      x->sadperbit4 = sad_per_bit4lut_12[qindex];
      break;
    default:
      assert(0 && "bit_depth should be AOM_BITS_8, AOM_BITS_10 or AOM_BITS_12");
  }
}

static void set_block_thresholds(const AV1_COMMON *cm, RD_OPT *rd) {
  int i, bsize, segment_id;

  for (segment_id = 0; segment_id < MAX_SEGMENTS; ++segment_id) {
    const int qindex =
        clamp(av1_get_qindex(&cm->seg, segment_id, cm->base_qindex) +
                  cm->y_dc_delta_q,
              0, MAXQ);
    const int q = compute_rd_thresh_factor(qindex, cm->seq_params.bit_depth);

    for (bsize = 0; bsize < BLOCK_SIZES_ALL; ++bsize) {
      // Threshold here seems unnecessarily harsh but fine given actual
      // range of values used for cpi->sf.thresh_mult[].
      const int t = q * rd_thresh_block_size_factor[bsize];
      const int thresh_max = INT_MAX / t;

      for (i = 0; i < MAX_MODES; ++i)
        rd->threshes[segment_id][bsize][i] = rd->thresh_mult[i] < thresh_max
                                                 ? rd->thresh_mult[i] * t / 4
                                                 : INT_MAX;
    }
  }
}

void av1_fill_coeff_costs(MACROBLOCK *x, FRAME_CONTEXT *fc,
                          const int num_planes) {
  const int nplanes = AOMMIN(num_planes, PLANE_TYPES);
  for (int eob_multi_size = 0; eob_multi_size < 7; ++eob_multi_size) {
    for (int plane = 0; plane < nplanes; ++plane) {
      LV_MAP_EOB_COST *pcost = &x->eob_costs[eob_multi_size][plane];

      for (int ctx = 0; ctx < 2; ++ctx) {
        aom_cdf_prob *pcdf;
        switch (eob_multi_size) {
          case 0: pcdf = fc->eob_flag_cdf16[plane][ctx]; break;
          case 1: pcdf = fc->eob_flag_cdf32[plane][ctx]; break;
          case 2: pcdf = fc->eob_flag_cdf64[plane][ctx]; break;
          case 3: pcdf = fc->eob_flag_cdf128[plane][ctx]; break;
          case 4: pcdf = fc->eob_flag_cdf256[plane][ctx]; break;
          case 5: pcdf = fc->eob_flag_cdf512[plane][ctx]; break;
          case 6:
          default: pcdf = fc->eob_flag_cdf1024[plane][ctx]; break;
        }
        av1_cost_tokens_from_cdf(pcost->eob_cost[ctx], pcdf, NULL);
      }
    }
  }
  for (int tx_size = 0; tx_size < TX_SIZES; ++tx_size) {
    for (int plane = 0; plane < nplanes; ++plane) {
      LV_MAP_COEFF_COST *pcost = &x->coeff_costs[tx_size][plane];

      for (int ctx = 0; ctx < TXB_SKIP_CONTEXTS; ++ctx)
        av1_cost_tokens_from_cdf(pcost->txb_skip_cost[ctx],
                                 fc->txb_skip_cdf[tx_size][ctx], NULL);

      for (int ctx = 0; ctx < SIG_COEF_CONTEXTS_EOB; ++ctx)
        av1_cost_tokens_from_cdf(pcost->base_eob_cost[ctx],
                                 fc->coeff_base_eob_cdf[tx_size][plane][ctx],
                                 NULL);
      for (int ctx = 0; ctx < SIG_COEF_CONTEXTS; ++ctx)
        av1_cost_tokens_from_cdf(pcost->base_cost[ctx],
                                 fc->coeff_base_cdf[tx_size][plane][ctx], NULL);

      for (int ctx = 0; ctx < SIG_COEF_CONTEXTS; ++ctx) {
        pcost->base_cost[ctx][4] = 0;
        pcost->base_cost[ctx][5] = pcost->base_cost[ctx][1] +
                                   av1_cost_literal(1) -
                                   pcost->base_cost[ctx][0];
        pcost->base_cost[ctx][6] =
            pcost->base_cost[ctx][2] - pcost->base_cost[ctx][1];
        pcost->base_cost[ctx][7] =
            pcost->base_cost[ctx][3] - pcost->base_cost[ctx][2];
      }

      for (int ctx = 0; ctx < EOB_COEF_CONTEXTS; ++ctx)
        av1_cost_tokens_from_cdf(pcost->eob_extra_cost[ctx],
                                 fc->eob_extra_cdf[tx_size][plane][ctx], NULL);

      for (int ctx = 0; ctx < DC_SIGN_CONTEXTS; ++ctx)
        av1_cost_tokens_from_cdf(pcost->dc_sign_cost[ctx],
                                 fc->dc_sign_cdf[plane][ctx], NULL);

      for (int ctx = 0; ctx < LEVEL_CONTEXTS; ++ctx) {
        int br_rate[BR_CDF_SIZE];
        int prev_cost = 0;
        int i, j;
        av1_cost_tokens_from_cdf(br_rate, fc->coeff_br_cdf[tx_size][plane][ctx],
                                 NULL);
        // printf("br_rate: ");
        // for(j = 0; j < BR_CDF_SIZE; j++)
        //  printf("%4d ", br_rate[j]);
        // printf("\n");
        for (i = 0; i < COEFF_BASE_RANGE; i += BR_CDF_SIZE - 1) {
          for (j = 0; j < BR_CDF_SIZE - 1; j++) {
            pcost->lps_cost[ctx][i + j] = prev_cost + br_rate[j];
          }
          prev_cost += br_rate[j];
        }
        pcost->lps_cost[ctx][i] = prev_cost;
        // printf("lps_cost: %d %d %2d : ", tx_size, plane, ctx);
        // for (i = 0; i <= COEFF_BASE_RANGE; i++)
        //  printf("%5d ", pcost->lps_cost[ctx][i]);
        // printf("\n");
      }
      for (int ctx = 0; ctx < LEVEL_CONTEXTS; ++ctx) {
        pcost->lps_cost[ctx][0 + COEFF_BASE_RANGE + 1] =
            pcost->lps_cost[ctx][0];
        for (int i = 1; i <= COEFF_BASE_RANGE; ++i) {
          pcost->lps_cost[ctx][i + COEFF_BASE_RANGE + 1] =
              pcost->lps_cost[ctx][i] - pcost->lps_cost[ctx][i - 1];
        }
      }
    }
  }
}

void av1_initialize_cost_tables(const AV1_COMMON *const cm, MACROBLOCK *x) {
  if (cm->cur_frame_force_integer_mv) {
    av1_build_nmv_cost_table(x->nmv_vec_cost, x->nmvcost, &cm->fc->nmvc,
                             MV_SUBPEL_NONE);
  } else {
    av1_build_nmv_cost_table(
        x->nmv_vec_cost,
        cm->allow_high_precision_mv ? x->nmvcost_hp : x->nmvcost, &cm->fc->nmvc,
        cm->allow_high_precision_mv);
  }
}

void av1_initialize_rd_consts(AV1_COMP *cpi) {
  AV1_COMMON *const cm = &cpi->common;
  MACROBLOCK *const x = &cpi->td.mb;
  RD_OPT *const rd = &cpi->rd;

  aom_clear_system_state();

  rd->RDMULT = av1_compute_rd_mult(cpi, cm->base_qindex + cm->y_dc_delta_q);

  set_error_per_bit(x, rd->RDMULT);

  set_block_thresholds(cm, rd);

  av1_initialize_cost_tables(cm, x);

  if (frame_is_intra_only(cm) && cm->allow_screen_content_tools &&
      cpi->oxcf.pass != 1) {
    int *dvcost[2] = { &cpi->dv_cost[0][MV_MAX], &cpi->dv_cost[1][MV_MAX] };
    av1_build_nmv_cost_table(cpi->dv_joint_cost, dvcost, &cm->fc->ndvc,
                             MV_SUBPEL_NONE);
  }

  if (cpi->oxcf.pass != 1) {
    for (int i = 0; i < TRANS_TYPES; ++i)
      // IDENTITY: 1 bit
      // TRANSLATION: 3 bits
      // ROTZOOM: 2 bits
      // AFFINE: 3 bits
      cpi->gmtype_cost[i] = (1 + (i > 0 ? (i == ROTZOOM ? 1 : 2) : 0))
                            << AV1_PROB_COST_SHIFT;
  }
}

static void model_rd_norm(int xsq_q10, int *r_q10, int *d_q10) {
  // NOTE: The tables below must be of the same size.

  // The functions described below are sampled at the four most significant
  // bits of x^2 + 8 / 256.

  // Normalized rate:
  // This table models the rate for a Laplacian source with given variance
  // when quantized with a uniform quantizer with given stepsize. The
  // closed form expression is:
  // Rn(x) = H(sqrt(r)) + sqrt(r)*[1 + H(r)/(1 - r)],
  // where r = exp(-sqrt(2) * x) and x = qpstep / sqrt(variance),
  // and H(x) is the binary entropy function.
  static const int rate_tab_q10[] = {
    65536, 6086, 5574, 5275, 5063, 4899, 4764, 4651, 4553, 4389, 4255, 4142,
    4044,  3958, 3881, 3811, 3748, 3635, 3538, 3453, 3376, 3307, 3244, 3186,
    3133,  3037, 2952, 2877, 2809, 2747, 2690, 2638, 2589, 2501, 2423, 2353,
    2290,  2232, 2179, 2130, 2084, 2001, 1928, 1862, 1802, 1748, 1698, 1651,
    1608,  1530, 1460, 1398, 1342, 1290, 1243, 1199, 1159, 1086, 1021, 963,
    911,   864,  821,  781,  745,  680,  623,  574,  530,  490,  455,  424,
    395,   345,  304,  269,  239,  213,  190,  171,  154,  126,  104,  87,
    73,    61,   52,   44,   38,   28,   21,   16,   12,   10,   8,    6,
    5,     3,    2,    1,    1,    1,    0,    0,
  };
  // Normalized distortion:
  // This table models the normalized distortion for a Laplacian source
  // with given variance when quantized with a uniform quantizer
  // with given stepsize. The closed form expression is:
  // Dn(x) = 1 - 1/sqrt(2) * x / sinh(x/sqrt(2))
  // where x = qpstep / sqrt(variance).
  // Note the actual distortion is Dn * variance.
  static const int dist_tab_q10[] = {
    0,    0,    1,    1,    1,    2,    2,    2,    3,    3,    4,    5,
    5,    6,    7,    7,    8,    9,    11,   12,   13,   15,   16,   17,
    18,   21,   24,   26,   29,   31,   34,   36,   39,   44,   49,   54,
    59,   64,   69,   73,   78,   88,   97,   106,  115,  124,  133,  142,
    151,  167,  184,  200,  215,  231,  245,  260,  274,  301,  327,  351,
    375,  397,  418,  439,  458,  495,  528,  559,  587,  613,  637,  659,
    680,  717,  749,  777,  801,  823,  842,  859,  874,  899,  919,  936,
    949,  960,  969,  977,  983,  994,  1001, 1006, 1010, 1013, 1015, 1017,
    1018, 1020, 1022, 1022, 1023, 1023, 1023, 1024,
  };
  static const int xsq_iq_q10[] = {
    0,      4,      8,      12,     16,     20,     24,     28,     32,
    40,     48,     56,     64,     72,     80,     88,     96,     112,
    128,    144,    160,    176,    192,    208,    224,    256,    288,
    320,    352,    384,    416,    448,    480,    544,    608,    672,
    736,    800,    864,    928,    992,    1120,   1248,   1376,   1504,
    1632,   1760,   1888,   2016,   2272,   2528,   2784,   3040,   3296,
    3552,   3808,   4064,   4576,   5088,   5600,   6112,   6624,   7136,
    7648,   8160,   9184,   10208,  11232,  12256,  13280,  14304,  15328,
    16352,  18400,  20448,  22496,  24544,  26592,  28640,  30688,  32736,
    36832,  40928,  45024,  49120,  53216,  57312,  61408,  65504,  73696,
    81888,  90080,  98272,  106464, 114656, 122848, 131040, 147424, 163808,
    180192, 196576, 212960, 229344, 245728,
  };
  const int tmp = (xsq_q10 >> 2) + 8;
  const int k = get_msb(tmp) - 3;
  const int xq = (k << 3) + ((tmp >> k) & 0x7);
  const int one_q10 = 1 << 10;
  const int a_q10 = ((xsq_q10 - xsq_iq_q10[xq]) << 10) >> (2 + k);
  const int b_q10 = one_q10 - a_q10;
  *r_q10 = (rate_tab_q10[xq] * b_q10 + rate_tab_q10[xq + 1] * a_q10) >> 10;
  *d_q10 = (dist_tab_q10[xq] * b_q10 + dist_tab_q10[xq + 1] * a_q10) >> 10;
}

void av1_model_rd_from_var_lapndz(int64_t var, unsigned int n_log2,
                                  unsigned int qstep, int *rate,
                                  int64_t *dist) {
  // This function models the rate and distortion for a Laplacian
  // source with given variance when quantized with a uniform quantizer
  // with given stepsize. The closed form expressions are in:
  // Hang and Chen, "Source Model for transform video coder and its
  // application - Part I: Fundamental Theory", IEEE Trans. Circ.
  // Sys. for Video Tech., April 1997.
  if (var == 0) {
    *rate = 0;
    *dist = 0;
  } else {
    int d_q10, r_q10;
    static const uint32_t MAX_XSQ_Q10 = 245727;
    const uint64_t xsq_q10_64 =
        (((uint64_t)qstep * qstep << (n_log2 + 10)) + (var >> 1)) / var;
    const int xsq_q10 = (int)AOMMIN(xsq_q10_64, MAX_XSQ_Q10);
    model_rd_norm(xsq_q10, &r_q10, &d_q10);
    *rate = ROUND_POWER_OF_TWO(r_q10 << n_log2, 10 - AV1_PROB_COST_SHIFT);
    *dist = (var * (int64_t)d_q10 + 512) >> 10;
  }
}

static double interp_cubic(const double *p, double x) {
  return p[1] + 0.5 * x *
                    (p[2] - p[0] +
                     x * (2.0 * p[0] - 5.0 * p[1] + 4.0 * p[2] - p[3] +
                          x * (3.0 * (p[1] - p[2]) + p[3] - p[0])));
}

static double interp_bicubic(const double *p, int p_stride, double x,
                             double y) {
  double q[4];
  q[0] = interp_cubic(p, x);
  q[1] = interp_cubic(p + p_stride, x);
  q[2] = interp_cubic(p + 2 * p_stride, x);
  q[3] = interp_cubic(p + 3 * p_stride, x);
  return interp_cubic(q, y);
}

static const uint8_t bsize_model_cat_lookup[BLOCK_SIZES_ALL] = {
  0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3, 3, 3, 3, 0, 0, 1, 1, 2, 2
};

static int sse_norm_model_cat_lookup(double sse_norm) {
  return (sse_norm > 16.0);
}

static const double interp_rgrid_surf[4][33 * 18] = {
  {
      29.726102,   30.738006,   25.294088,   25.736759,   41.255961,
      41.937828,   41.937901,   42.110487,   46.038554,   46.593284,
      55.290144,   55.672257,   55.672289,   55.672284,   55.641633,
      54.251766,   38.404522,   36.186584,   30.977201,   32.031695,
      26.358675,   26.819956,   42.992269,   43.702888,   43.702966,
      43.882830,   47.992288,   48.920311,   57.633255,   58.015369,
      58.015396,   57.984745,   56.594876,   40.717227,   39.327227,
      37.709563,   30.977308,   32.041872,   26.587891,   26.801416,
      42.339251,   43.674340,   43.710377,   44.048333,   48.365747,
      57.251292,   57.999483,   58.015595,   57.984977,   56.595076,
      40.717426,   39.327559,   39.313319,   38.085580,   30.977328,
      32.271000,   31.802769,   26.377378,   27.473270,   43.021187,
      43.875643,   47.811034,   48.547182,   57.617338,   58.015563,
      58.001989,   56.982277,   40.734438,   39.327560,   39.313487,
      40.065111,   46.657660,   30.977329,   32.281067,   32.031899,
      26.358835,   26.820108,   42.992490,   43.882906,   47.992438,
      48.920477,   57.633450,   58.001989,   57.369478,   49.547092,
      39.714761,   39.313520,   40.065295,   48.834535,   51.525122,
      30.977329,   32.281072,   32.077511,   27.396520,   26.865699,
      42.992474,   43.882906,   48.358483,   57.251290,   57.985906,
      57.369511,   49.564105,   40.101962,   39.330532,   40.065297,
      48.835069,   57.817665,   145.394655,  30.977369,   32.281197,
      33.076074,   50.151877,   28.497451,   42.792931,   43.512994,
      48.377350,   57.604551,   57.386489,   49.951306,   40.118976,
      39.330567,   40.065298,   48.835069,   57.818203,   151.867755,
      153.625733,  31.447893,   32.750906,   32.266221,   32.302627,
      42.268290,   38.854272,   35.574619,   49.427510,   56.878041,
      50.330896,   48.931678,   39.717770,   40.065331,   48.835069,
      57.818191,   151.733328,  156.892368,  150.600520,  42.125514,
      43.448292,   33.901056,   49.284440,   47.710751,   52.761179,
      46.446220,   71.330907,   47.520016,   49.126258,   40.678046,
      40.107037,   48.835077,   57.818191,   151.733068,  153.820869,
      83.953859,   77.615621,   42.078145,   44.460837,   53.413056,
      53.860750,   51.935802,   60.688133,   54.022549,   59.058236,
      61.487243,   49.511225,   53.558701,   50.619860,   57.871196,
      151.733230,  153.821045,  83.819613,   77.759808,   76.318016,
      47.334708,   55.115021,   63.088351,   65.025437,   66.447005,
      66.685816,   72.840860,   70.204772,   75.247159,   75.769300,
      62.167236,   83.948064,   149.878947,  155.679346,  86.031186,
      83.012511,   86.265353,   124.454706,  64.496779,   78.702984,
      76.659370,   83.931191,   87.076287,   91.454837,   94.323380,
      104.708654,  109.932924,  118.596035,  126.154595,  114.409068,
      89.213630,   131.569970,  134.824613,  205.866155,  239.628810,
      233.070258,  72.308997,   94.104934,   101.404120,  121.159064,
      139.921349,  156.802796,  164.908506,  170.895200,  184.099064,
      198.250167,  207.100063,  207.922921,  207.455763,  210.915553,
      241.757520,  246.227265,  247.706317,  237.758893,  138.726610,
      135.459290,  198.494473,  240.094885,  275.356590,  295.461068,
      304.435273,  303.110796,  296.273053,  317.439936,  319.323966,
      331.200883,  330.246858,  256.379818,  247.994095,  247.767952,
      247.767210,  237.760453,  243.406633,  292.096487,  381.480937,
      421.494757,  458.357268,  483.808573,  487.800297,  481.212962,
      472.226349,  472.996935,  480.732289,  475.694812,  418.477476,
      412.710307,  268.338558,  261.989349,  261.987913,  251.406810,
      469.068582,  568.835648,  608.315967,  642.717253,  653.879813,
      681.008793,  688.659203,  677.706251,  668.348079,  652.995530,
      636.214467,  632.005555,  550.175654,  522.412011,  588.754096,
      591.721361,  591.719586,  567.821360,  667.228892,  787.133995,
      830.015474,  848.841400,  863.280871,  882.475988,  896.321308,
      889.205975,  878.059084,  871.467549,  860.073920,  814.054753,
      726.521001,  747.583633,  751.922326,  752.065525,  752.062970,
      721.688835,  795.516527,  916.651129,  1045.662316, 1064.160855,
      1070.930169, 1060.177543, 1080.056283, 1096.204211, 1095.337870,
      1098.401980, 1087.336698, 1022.507849, 916.586633,  938.424837,
      942.750167,  945.352203,  945.457385,  907.272493,  943.653980,
      1021.248655, 1129.544563, 1267.035138, 1281.163505, 1235.783117,
      1258.747831, 1285.425350, 1293.324247, 1314.802791, 1320.786990,
      1253.910457, 1138.645930, 1091.248129, 1161.397763, 1220.617170,
      1223.081462, 1173.684183, 1077.194216, 1216.741493, 1278.148407,
      1383.489545, 1497.622061, 1505.771495, 1422.262568, 1462.870558,
      1491.544736, 1494.922359, 1533.495237, 1509.573542, 1356.235452,
      1322.878872, 1324.869188, 1327.897500, 1337.708123, 1284.090364,
      1177.946473, 1335.104679, 1414.612819, 1508.772208, 1656.204257,
      1636.726625, 1693.515385, 1658.176544, 1626.718453, 1699.881842,
      1721.127481, 1715.540667, 1610.027798, 1528.475428, 1525.090509,
      1534.987574, 1760.238672, 1698.643936, 1285.821939, 1417.714456,
      1527.788112, 1660.663143, 1648.660011, 1838.036905, 1838.669356,
      1803.685279, 1895.404145, 1878.456820, 1907.761437, 1891.806178,
      1798.474576, 1772.695413, 1777.922135, 1782.961020, 1891.441373,
      1819.624352, 1363.652278, 1525.975687, 1664.733346, 1725.645678,
      1842.181220, 1845.935374, 1847.645643, 2084.649919, 2156.327082,
      2070.132265, 2093.920913, 2066.856209, 1935.233655, 1757.519179,
      1894.998543, 1904.433642, 1909.668939, 1832.757449, 1466.924321,
      1622.799826, 1723.952077, 1846.075992, 1857.802996, 1849.640165,
      2127.098845, 2370.422248, 2375.277013, 2471.317955, 2282.670767,
      2242.894713, 2110.737592, 1924.405669, 2022.847422, 2092.614827,
      2113.100659, 2028.502155, 1561.024406, 1719.325091, 1845.872940,
      1857.825889, 1850.155079, 2127.184115, 2372.221523, 2397.560183,
      2793.802030, 2525.955468, 2512.515727, 2465.117634, 2335.991284,
      2104.381407, 2124.161214, 2128.328755, 2129.221941, 2043.260174,
      1651.007418, 1782.604815, 1855.046326, 1850.154844, 2127.184127,
      2372.221529, 2397.560385, 2794.384764, 2544.781302, 2651.443272,
      2739.000803, 2702.474497, 2522.919941, 2276.563499, 2274.919319,
      2275.308573, 2275.300889, 2183.406597, 1713.105119, 1852.260885,
      1850.032717, 2127.168115, 2371.857312, 2397.544382, 2794.384763,
      2544.781370, 2651.673334, 2746.255820, 2763.494638, 2872.508140,
      2846.367495, 2305.137739, 2281.712444, 2281.725436, 2281.717621,
      2189.564169, 1780.286046, 1850.026376, 2127.168073, 2371.493095,
      2389.254845, 2794.020546, 2544.781339, 2651.673334, 2746.255845,
      2763.564597, 2874.688737, 2884.399707, 2860.263639, 2306.077266,
      2281.728045, 2281.725981, 2281.718165, 2189.564691, 1775.067091,
      2127.161715, 2371.493094, 2389.238842, 2793.656329, 2544.765337,
      2651.673332, 2746.255845, 2763.564597, 2874.688743, 2884.401957,
      2884.614099, 2860.264919, 2306.079244, 2281.728130, 2281.725981,
      2281.718165, 2189.564691, 2052.948444, 2371.486799, 2389.238842,
      2793.656328, 2544.765306, 2651.673331, 2746.255845, 2763.564597,
      2874.688743, 2884.401957, 2884.616164, 2884.614218, 2861.248329,
      2328.461556, 2282.711540, 2281.726065, 2281.718165, 2189.564691,
      2285.514142, 2389.231550, 2793.656328, 2544.765306, 2651.673331,
      2746.255845, 2763.564597, 2874.688743, 2884.401957, 2884.616164,
      2884.616200, 2884.616117, 2883.630641, 2837.880625, 2305.093852,
      2281.727963, 2281.718165, 2189.564691, 2293.011609, 2793.639080,
      2544.755886, 2651.663879, 2746.245982, 2763.554715, 2874.678861,
      2884.392075, 2884.606282, 2884.606318, 2884.606318, 2884.606318,
      2884.604170, 2860.253139, 2306.069362, 2281.720230, 2281.710349,
      2189.557190, 2600.476153, 2437.607663, 2540.606072, 2634.813199,
      2647.265343, 2758.176106, 2767.889302, 2768.103509, 2768.103545,
      2768.103545, 2768.103545, 2768.103545, 2768.101564, 2744.735758,
      2212.932478, 2189.566673, 2189.557190, 2101.125891,
  },
  {
      8.370360,    8.509764,    8.942497,    8.962541,    8.993650,
      9.401615,    9.414854,    9.308823,    9.304164,    9.304163,
      9.304164,    9.304609,    9.324803,    9.555049,    9.565277,
      9.571194,    9.647370,    9.451428,    8.722648,    8.867100,
      9.300225,    9.352116,    9.672213,    9.809754,    9.794381,
      9.699874,    9.695753,    9.695753,    9.696199,    9.716392,
      9.947083,    9.967407,    9.973767,    10.050124,   10.454503,
      14.384681,   8.722677,    8.848489,    8.875968,    9.334087,
      9.685450,    9.793645,    9.413799,    9.683184,    9.695785,
      9.696231,    9.716425,    9.947116,    9.967440,    9.973802,
      10.050158,   10.454591,   15.023561,   15.153937,   8.723107,
      8.857467,    8.857756,    9.333268,    9.685451,    9.792911,
      9.397076,    9.682449,    9.695786,    9.706329,    9.946672,
      9.967440,    9.973802,    10.050158,   10.454591,   15.023647,
      16.163497,   23.616388,   8.732904,    9.080445,    8.867552,
      9.333269,    9.685451,    9.792910,    9.397075,    9.682449,
      9.696231,    9.716869,    9.957343,    9.973801,    10.050158,
      10.454591,   15.023647,   16.163589,   24.700350,   25.680478,
      8.733342,    9.090251,    8.868006,    9.333704,    9.685477,
      9.792913,    9.397086,    9.682895,    9.716424,    9.947246,
      9.973357,    10.050158,   10.454591,   15.023647,   16.163589,
      24.700554,   28.087109,   55.858717,   8.827122,    9.186580,
      8.932093,    9.387586,    9.780396,    9.827339,    9.522726,
      9.715771,    9.936594,    9.972878,    10.050158,   10.454591,
      15.023647,   16.163589,   24.700554,   28.087318,   58.323463,
      58.504090,   11.020305,   11.418729,   10.432068,   10.462985,
      11.980204,   10.705178,   12.392570,   10.364495,   9.829351,
      10.102947,   10.457228,   15.023647,   16.163589,   24.700554,
      28.087319,   58.323704,   60.984846,   58.926665,   12.554530,
      12.511580,   11.828923,   12.308025,   13.298760,   13.044522,
      12.945689,   13.556029,   12.673484,   12.067338,   15.194674,
      16.168241,   24.700556,   28.087321,   58.339708,   61.349323,
      61.787659,   67.232638,   15.008518,   15.485683,   15.872869,
      16.107583,   15.835820,   16.877645,   17.678106,   18.822352,
      20.208378,   19.665073,   19.090588,   25.203257,   28.104179,
      58.339741,   61.713565,   70.078019,   70.458033,   67.961299,
      17.761908,   35.462897,   20.615440,   19.929266,   20.679593,
      22.692816,   25.941528,   29.215386,   34.956494,   38.908872,
      35.866805,   37.824252,   59.112479,   61.729892,   70.173287,
      72.702736,   72.797652,   69.857555,   20.506234,   39.710259,
      24.054829,   25.703729,   31.621917,   38.509103,   44.229053,
      56.544042,   67.468427,   79.581678,   80.144809,   78.556111,
      74.399792,   73.852202,   76.027762,   117.654284,  119.482115,
      114.656650,  25.548773,   48.434418,   42.617322,   59.026359,
      82.885084,   103.416136,  110.596012,  118.893312,  139.741780,
      150.511338,  161.901080,  167.647532,  162.429576,  152.184833,
      152.445359,  168.398896,  169.099625,  162.269514,  82.673806,
      70.110218,   132.312464,  183.332020,  215.910819,  237.790158,
      239.207817,  229.469490,  230.174506,  250.717349,  253.902836,
      250.549622,  207.721430,  175.337781,  174.022082,  174.844559,
      179.095719,  164.620978,  109.303621,  158.273825,  282.689646,
      346.940666,  386.914531,  410.989383,  397.460378,  370.331321,
      364.107004,  366.410030,  380.333542,  363.501310,  294.254304,
      262.048447,  260.790777,  265.975674,  362.177173,  183.336338,
      404.520239,  473.542827,  507.606249,  529.861510,  559.718099,
      588.632991,  577.691379,  533.799138,  522.780262,  528.340923,
      525.905829,  507.879853,  415.994095,  425.708669,  426.932731,
      441.248848,  446.721366,  435.160437,  604.741350,  709.039378,
      780.416591,  732.829427,  733.032326,  766.737728,  758.413295,
      720.856125,  705.746779,  717.211527,  703.753940,  661.340782,
      552.231698,  523.727433,  522.707356,  523.327476,  523.380946,
      502.845082,  745.431184,  827.490462,  962.723712,  998.756854,
      911.671845,  907.248783,  925.966430,  901.879365,  900.337680,
      919.439892,  914.334537,  842.878262,  704.639674,  702.883177,
      703.067185,  703.067254,  703.064850,  674.669690,  894.169844,
      1025.903412, 1134.439771, 1219.193918, 1158.526825, 1079.781195,
      1077.325043, 1076.106019, 1075.934290, 1117.511500, 1135.469693,
      1063.699731, 901.900714,  920.553711,  921.681648,  921.681860,
      921.679125,  884.454592,  994.327035,  1175.512444, 1231.834682,
      1307.830164, 1407.421778, 1326.963114, 1253.371428, 1250.787362,
      1269.653825, 1282.857814, 1343.429863, 1307.501250, 1114.048857,
      983.551449,  978.228429,  979.601009,  984.578211,  945.021162,
      1135.950879, 1278.106415, 1310.418579, 1419.457570, 1488.462247,
      1521.009066, 1542.754625, 1434.182316, 1454.788272, 1471.133321,
      1506.092266, 1521.256433, 1327.843116, 1309.574134, 1310.302024,
      1341.725430, 1459.021892, 1404.992306, 1232.172010, 1349.471091,
      1428.291402, 1487.206606, 1535.398237, 1746.731849, 1716.656287,
      1635.940921, 1639.303784, 1673.047286, 1693.682270, 1671.177816,
      1517.793233, 1479.700025, 1478.369392, 1483.694229, 1578.610159,
      1518.853600, 1297.751330, 1428.849626, 1485.708645, 1493.412575,
      1741.940596, 1728.475751, 1735.779511, 1858.578642, 1918.863689,
      1863.724239, 1891.004802, 1842.815568, 1675.715043, 1588.071911,
      1589.814363, 1592.306606, 1596.345158, 1532.039001, 1373.297412,
      1461.343441, 1492.261194, 1736.797392, 1653.192795, 1733.476694,
      1888.263820, 2082.789610, 1991.468137, 1883.805569, 2116.637406,
      2071.956368, 1871.698996, 1644.776129, 1756.448731, 1809.269770,
      1811.354640, 1738.198277, 1403.551810, 1491.185946, 1736.750346,
      1653.047461, 1730.172539, 1888.230584, 2085.624280, 1993.438043,
      1786.471183, 2337.838134, 2448.464613, 2336.935884, 2097.581279,
      1808.183187, 1828.142934, 1821.170551, 1820.795051, 1747.257197,
      1427.896429, 1736.558511, 1653.047441, 1730.172527, 1888.230448,
      2087.322730, 2032.094869, 1787.954651, 2341.066895, 2641.531977,
      2539.340235, 2507.302262, 2309.116599, 2030.685235, 2078.550644,
      1842.720183, 1832.216691, 1758.216718, 1580.198506, 1648.792285,
      1730.172167, 1888.230448, 2087.322736, 2032.169494, 1789.653110,
      2341.141530, 2641.873005, 2555.239313, 2709.221434, 2715.218691,
      2682.937371, 2124.893499, 2102.967545, 2092.601766, 2092.133383,
      2007.636776, 1578.033231, 1729.979789, 1888.230432, 2087.322736,
      2032.169494, 1789.653117, 2341.141675, 2641.873012, 2555.239371,
      2709.566764, 2723.749115, 2724.064349, 2699.003101, 2128.626010,
      2103.563660, 2103.560569, 2103.553324, 2018.595528, 1663.710423,
      1888.224796, 2087.322736, 2032.169494, 1789.653117, 2341.141675,
      2641.873012, 2555.239371, 2709.566764, 2723.749145, 2724.067198,
      2724.065212, 2700.016608, 2151.662616, 2104.575854, 2103.561585,
      2103.554293, 2018.596458, 1818.473656, 2087.316820, 2032.169494,
      1789.653117, 2341.141675, 2641.873012, 2555.239371, 2709.566764,
      2723.749145, 2724.067198, 2724.067252, 2724.067166, 2723.052897,
      2675.966135, 2127.612143, 2103.563538, 2103.554293, 2018.596458,
      2011.142209, 2032.163033, 1789.653117, 2341.141675, 2641.873012,
      2555.239371, 2709.566764, 2723.749145, 2724.067198, 2724.067252,
      2724.067252, 2724.067252, 2724.065041, 2699.002510, 2128.626241,
      2103.563710, 2103.554293, 2018.596458, 1947.414707, 1789.636821,
      2341.132961, 2641.863707, 2555.230039, 2709.557432, 2723.739813,
      2724.057867, 2724.057920, 2724.057920, 2724.057920, 2724.057920,
      2724.055881, 2700.007276, 2151.653370, 2104.568562, 2103.547173,
      2018.589543, 1614.950117, 2233.740132, 2539.133260, 2445.528333,
      2599.538876, 2613.721229, 2614.039283, 2614.039337, 2614.039337,
      2614.039337, 2614.039337, 2614.039337, 2614.039251, 2613.025067,
      2566.950536, 2041.634786, 2018.591496, 1937.063243,
  },
  {
      2.147514,    2.237898,    2.237803,    2.237151,    2.275320,
      2.279324,    2.332179,    2.334501,    2.322470,    2.048664,
      2.036634,    2.036633,    2.043731,    2.205267,    2.213955,
      2.286115,    3.114672,    3.150236,    2.237900,    2.332085,
      2.329658,    2.278309,    2.368754,    2.375255,    2.430335,
      2.432248,    2.408693,    2.134381,    2.122351,    2.122650,
      2.136546,    2.299971,    2.379228,    3.209393,    3.546162,
      6.294600,    2.271800,    2.333582,    2.329564,    2.275984,
      2.368873,    2.380129,    2.430556,    2.420732,    2.146419,
      2.122864,    2.122657,    2.136254,    2.293180,    2.378937,
      3.209400,    3.546195,    6.591552,    7.028860,    3.043184,
      2.367477,    2.329567,    2.275984,    2.373739,    2.490860,
      2.435421,    2.420225,    2.134895,    2.122658,    2.136254,
      2.293180,    2.378936,    3.209400,    3.546195,    6.591617,
      7.795421,    17.743823,   3.078566,    2.402859,    2.331056,
      2.275985,    2.373953,    2.495725,    2.435129,    2.408702,
      2.134688,    2.136255,    2.293180,    2.378936,    3.209400,
      3.546195,    6.591617,    7.795492,    18.578087,   19.754344,
      3.112459,    3.174471,    2.370323,    2.279235,    2.375398,
      2.494530,    2.423939,    2.146738,    2.136763,    2.293180,
      2.378939,    3.209400,    3.546195,    6.591617,    7.795492,
      18.578249,   21.659892,   44.204400,   3.089303,    3.189642,
      2.509317,    2.372448,    2.432176,    2.489915,    2.461894,
      2.170933,    2.301935,    2.380378,    3.234280,    3.547288,
      6.591617,    7.795492,    18.578249,   21.660057,   46.157016,
      46.346972,   2.545327,    2.662725,    2.893299,    2.840660,
      3.006570,    3.048065,    3.224317,    2.907751,    2.627415,
      3.268959,    4.113541,    6.616496,    7.795494,    18.578249,
      21.660057,   46.157182,   48.299592,   46.394222,   2.942757,
      3.155386,    3.688593,    3.481851,    3.715109,    4.145502,
      4.681446,    5.080585,    4.600294,    4.278096,    6.726591,
      7.800142,    18.578250,   21.660057,   46.157182,   48.299758,
      48.346843,   46.394230,   3.793838,    5.473983,    4.608110,
      4.665834,    4.593326,    5.621563,    7.102711,    9.025169,
      10.122234,   10.159723,   10.023976,   18.857332,   21.668316,
      46.157261,   48.299839,   48.347091,   48.346956,   46.394491,
      4.271176,    6.466779,    5.484235,    6.027476,    7.301091,
      9.352794,    12.715546,   16.192475,   22.560070,   27.958500,
      27.052873,   26.766614,   46.406220,   49.221059,   49.307058,
      49.315280,   49.587875,   49.469333,   4.312908,    7.475387,
      7.833860,    10.223714,   16.775457,   24.234924,   32.175390,
      45.584094,   54.342692,   64.941124,   66.654416,   62.626466,
      52.266425,   72.572219,   73.485826,   73.672773,   79.880600,
      119.538616,  9.063034,    10.103318,   19.539238,   36.472083,
      62.703355,   82.709235,   91.988082,   100.285999,  115.687111,
      124.329614,  125.634719,  122.315209,  112.379054,  130.322056,
      131.130875,  131.139174,  131.411947,  127.988734,  19.796341,
      25.882925,   93.253237,   153.548489,  189.401787,  213.347763,
      206.988813,  189.758327,  189.286642,  204.628607,  202.531100,
      201.472079,  186.352604,  214.273591,  215.531911,  215.769339,
      221.170008,  212.465343,  169.309448,  175.824536,  224.720662,
      303.438505,  343.063029,  370.535703,  343.624360,  300.549614,
      289.296503,  298.082336,  310.854450,  294.466802,  270.163254,
      249.832086,  248.983097,  254.384429,  377.318513,  367.262701,
      353.359003,  463.883442,  447.419222,  453.002989,  489.006379,
      528.921013,  500.655371,  439.860852,  423.703334,  423.144668,
      431.190760,  422.628418,  346.619232,  383.306337,  385.067653,
      385.305123,  390.705188,  375.153218,  539.087109,  664.575466,
      708.656277,  662.482491,  618.723324,  674.195070,  656.531281,
      596.084694,  576.463478,  584.628279,  577.147684,  562.515800,
      453.416064,  428.149599,  427.247359,  427.247313,  427.246307,
      409.990807,  687.292802,  777.058828,  905.030245,  912.381642,
      788.119446,  769.561998,  795.174180,  761.722895,  738.637994,
      753.718308,  761.364923,  712.753474,  603.465105,  591.662313,
      594.079856,  594.200005,  594.197958,  570.199152,  779.060173,
      951.796204,  1069.175314, 1170.755086, 1067.610429, 949.009502,
      934.793193,  914.719926,  899.055222,  929.548018,  948.443491,
      883.741844,  754.604267,  719.256051,  780.076337,  782.811403,
      782.547500,  745.230787,  922.165691,  1108.252645, 1185.274121,
      1237.276156, 1266.012843, 1226.812751, 1102.465568, 1085.286013,
      1075.433883, 1091.747391, 1130.914031, 1079.716598, 927.463157,
      942.209973,  945.879069,  946.263934,  946.332394,  778.375752,
      1069.516638, 1181.232895, 1244.763295, 1280.392765, 1419.980489,
      1443.722749, 1374.005679, 1247.118477, 1252.786860, 1265.078694,
      1280.267151, 1273.804486, 1085.050290, 1037.883124, 1036.171983,
      1042.457032, 1185.241611, 1137.692305, 1136.326843, 1246.301355,
      1321.499026, 1422.581245, 1459.231744, 1574.624247, 1593.556714,
      1554.290382, 1474.539686, 1480.100956, 1464.679108, 1412.507057,
      1251.278472, 1197.245685, 1195.175062, 1201.168704, 1337.571298,
      1289.300549, 1198.629451, 1322.569335, 1424.892277, 1470.063106,
      1576.317080, 1629.881049, 1759.007438, 1685.455960, 1823.183401,
      1696.763728, 1720.897219, 1582.701816, 1409.816846, 1352.692221,
      1350.761108, 1356.950874, 1362.935844, 1308.131482, 1273.245632,
      1447.544983, 1471.079493, 1576.789596, 1629.891022, 1759.113129,
      1704.167080, 2077.760859, 1944.393655, 2008.282739, 1951.682502,
      1905.896173, 1653.621076, 1524.210363, 1526.775296, 1662.009621,
      1667.942643, 1600.578688, 1394.223453, 1472.070445, 1576.833336,
      1629.890970, 1757.978972, 1678.353822, 2077.085726, 1963.548056,
      2214.617152, 2129.636312, 2170.262437, 2204.652056, 1892.295487,
      1652.520236, 1673.779173, 1681.078895, 1681.334159, 1613.428869,
      1413.634564, 1576.837954, 1629.890974, 1757.978960, 1678.303800,
      2075.951574, 1963.498706, 2214.984962, 2140.104142, 2224.748512,
      2358.489855, 2338.333779, 2147.326062, 1959.161343, 1704.142189,
      1693.264072, 1693.257371, 1624.870507, 1522.455341, 1630.102341,
      1757.978970, 1678.205343, 2073.710713, 1963.613482, 2219.840292,
      2140.317509, 2224.830630, 2367.413225, 2514.706448, 2519.715697,
      2490.578281, 1997.672226, 1965.092163, 1964.608831, 1964.602061,
      1885.256194, 1566.312385, 1757.983049, 1678.205154, 2071.469838,
      1912.611809, 2222.454760, 2250.824415, 2229.685973, 2367.413669,
      2515.007972, 2527.205354, 2527.466626, 2505.215096, 1998.786468,
      1976.532889, 1976.530885, 1976.524114, 1896.696743, 1692.302152,
      1678.199584, 2071.469829, 1912.513351, 2220.213903, 2250.939287,
      2234.541308, 2367.626998, 2515.007990, 2527.205379, 2527.469148,
      2527.467382, 2506.115004, 2019.240121, 1977.432526, 1976.531972,
      1976.525125, 1896.697713, 1606.783592, 2071.463772, 1912.513351,
      2220.213894, 2250.939097, 2234.541317, 2367.627410, 2515.008008,
      2527.205379, 2527.469148, 2527.469193, 2527.469116, 2526.568562,
      2484.760968, 1997.886085, 1976.533707, 1976.525125, 1896.697713,
      2004.589287, 1912.507679, 2220.213894, 2250.939097, 2234.541317,
      2367.627410, 2515.008008, 2527.205379, 2527.469148, 2527.469193,
      2527.469193, 2527.469193, 2527.467229, 2505.214602, 1998.786486,
      1976.533860, 1976.525125, 1896.697713, 1827.779426, 2220.199076,
      2250.930988, 2234.532682, 2367.618752, 2514.999350, 2527.196721,
      2527.460490, 2527.460534, 2527.460534, 2527.460534, 2527.460534,
      2527.458723, 2506.106346, 2019.231539, 1977.425679, 1976.518431,
      1896.691216, 2056.541124, 2160.944440, 2138.934044, 2265.803052,
      2412.920916, 2425.118264, 2425.382033, 2425.382078, 2425.382078,
      2425.382078, 2425.382078, 2425.382078, 2425.382002, 2424.481524,
      2383.572672, 1917.153083, 1896.692951, 1820.088116,
  },
  {
      0.254129,    0.265575,    0.267769,    0.316933,    0.320396,
      0.350337,    0.358368,    0.358663,    0.358663,    0.358675,
      0.359199,    0.365181,    0.366036,    0.392895,    0.701286,
      0.812014,    1.926722,    2.011743,    0.247749,    0.276002,
      0.279039,    0.330191,    0.332015,    0.365000,    0.373450,
      0.373758,    0.373759,    0.374032,    0.380276,    0.381405,
      0.408276,    0.717257,    0.841415,    1.958868,    2.298681,
      4.856667,    0.246998,    0.275970,    0.279040,    0.328326,
      0.289563,    0.363136,    0.373451,    0.373760,    0.373771,
      0.374308,    0.381144,    0.408277,    0.717258,    0.841416,
      1.958870,    2.298703,    5.088461,    5.484975,    0.246965,
      0.275218,    0.279007,    0.328244,    0.287698,    0.363054,
      0.373451,    0.373771,    0.374296,    0.380881,    0.408266,
      0.717258,    0.841416,    1.958870,    2.298703,    5.088514,
      6.102534,    14.287379,   0.246213,    0.258111,    0.278255,
      0.328244,    0.287698,    0.363054,    0.373452,    0.374034,
      0.380870,    0.408644,    0.725849,    0.841794,    1.958870,
      2.298703,    5.088514,    6.102591,    14.959455,   15.913957,
      0.246180,    0.257402,    0.279186,    0.328526,    0.290138,
      0.364877,    0.376133,    0.375014,    0.408382,    0.734439,
      1.037308,    1.967461,    2.298704,    5.088514,    6.102591,
      14.959584,   17.441072,   35.429097,   0.250019,    0.262303,
      0.303891,    0.340011,    0.350106,    0.413735,    0.446832,
      0.410301,    0.734390,    1.038841,    2.002354,    2.300237,
      5.088514,    6.102591,    14.959584,   17.441204,   36.992539,
      37.110962,   0.339750,    0.354696,    0.373261,    0.461008,
      0.462495,    0.598987,    0.695421,    0.760530,    1.068705,
      2.029958,    2.898902,    5.114817,    6.102593,    14.959584,
      17.441205,   36.992675,   38.648954,   36.568097,   0.400795,
      0.421342,    0.508307,    0.588014,    0.677109,    0.972744,
      1.364551,    2.005727,    2.968255,    3.091537,    5.221096,
      6.106993,    14.959585,   17.441202,   36.995441,   38.712050,
      38.170575,   37.947398,   0.489106,    0.499732,    0.682198,
      0.797985,    0.928841,    1.529889,    3.407135,    6.047262,
      9.020716,    9.566838,    8.225222,    15.250390,   17.449015,
      36.967227,   38.773771,   39.603706,   39.612863,   38.072105,
      0.782539,    0.400752,    1.185871,    1.379942,    2.131362,
      4.169824,    9.556239,    12.949414,   21.418888,   24.451152,
      23.409241,   22.939144,   37.166430,   38.131684,   39.591435,
      39.985395,   39.999803,   38.384307,   1.616514,    0.576268,
      3.109295,    3.930083,    8.248113,    15.543679,   28.108995,
      37.139521,   46.111565,   52.306471,   54.245700,   51.814241,
      40.882242,   41.810659,   42.231519,   48.976557,   49.261637,
      47.271630,   5.094715,    1.500966,    10.566421,   19.002838,
      38.783971,   59.976285,   70.642488,   79.940911,   88.863960,
      91.805239,   95.624856,   92.381504,   94.587340,   93.842585,
      93.403266,   93.670318,   93.432487,   89.648397,   0.664794,
      7.849196,    61.154449,   96.443290,   137.609372,  162.556446,
      148.037064,  138.292519,  136.715840,  145.772994,  149.246615,
      151.612049,  131.470885,  109.201663,  101.316335,  101.904288,
      101.932270,  97.814994,   6.925953,    50.062367,   155.094765,
      216.414869,  252.438993,  271.718839,  232.337598,  204.289605,
      199.194140,  205.331210,  215.677297,  214.590331,  188.109722,
      172.081271,  228.048207,  250.892300,  251.787562,  241.618501,
      134.674271,  296.106246,  298.718911,  312.356451,  323.413855,
      342.631007,  307.749299,  283.067516,  265.987870,  265.460375,
      276.156298,  267.602576,  244.127989,  255.341106,  257.373708,
      258.333723,  258.372207,  247.937137,  342.349647,  510.603738,
      581.005531,  478.386434,  373.143790,  378.600404,  377.561939,
      356.141074,  338.631477,  331.842551,  340.566852,  327.567230,
      297.260507,  287.191935,  263.781497,  262.767950,  262.766967,
      252.154399,  475.742336,  629.567678,  578.444958,  564.378757,
      438.113186,  396.420987,  414.959234,  431.302969,  421.606208,
      403.786618,  405.430259,  389.856615,  363.613524,  366.303795,
      365.460005,  365.415394,  365.414130,  350.655776,  624.537781,
      642.232055,  604.750655,  630.663539,  588.111844,  475.625654,
      468.814426,  477.664040,  494.124835,  483.676981,  477.873259,
      452.815795,  423.548326,  431.959314,  432.336669,  431.259561,
      431.158392,  412.599201,  599.136090,  596.383540,  649.766837,
      714.641904,  647.024168,  546.886099,  544.047374,  542.897551,
      558.506982,  555.978025,  571.819113,  532.351774,  510.677525,
      495.125872,  493.402635,  468.981876,  468.871284,  423.950331,
      572.225475,  705.639818,  1024.735591, 667.143308,  595.280675,
      762.720257,  552.209148,  613.853858,  630.029121,  653.039002,
      643.569732,  623.030443,  573.888949,  571.441956,  571.380435,
      572.467453,  621.621663,  597.446835,  681.815611,  1037.097688,
      905.438670,  599.972561,  762.219180,  517.929098,  688.053947,
      744.181205,  679.524546,  682.569374,  713.780286,  707.985178,
      646.983619,  636.062414,  635.699032,  635.794133,  637.956277,
      612.281811,  1009.018046, 905.871087,  604.028246,  629.564692,
      511.991374,  689.885845,  836.027416,  766.534552,  881.860194,
      735.849782,  783.725360,  809.653496,  750.636979,  811.765460,
      814.639525,  816.244480,  818.253804,  785.288396,  867.862571,
      604.231326,  629.308626,  506.143862,  689.628928,  836.193031,
      774.742632,  1007.244245, 1293.627165, 897.984304,  931.611215,
      973.236387,  945.068518,  947.276414,  950.892263,  987.500595,
      1033.288662, 993.423200,  674.214249,  634.009759,  506.144239,
      689.628432,  836.193010,  774.742663,  1007.434892, 1306.991055,
      1109.422389, 1067.304686, 1126.861498, 1124.180515, 989.502672,
      997.732672,  1042.608506, 1046.158446, 1048.166963, 1005.915872,
      633.151925,  507.355996,  689.628570,  836.193810,  774.742698,
      1007.434892, 1306.991101, 1109.775470, 1079.300073, 1239.065289,
      1628.963638, 1322.149598, 1152.072478, 1133.642807, 1135.074392,
      1135.159950, 1135.156239, 1089.309834, 1002.301724, 712.557708,
      836.610527,  784.181750,  1007.849615, 1306.991136, 1109.775470,
      1079.300117, 1239.231971, 1644.540785, 1609.385289, 1575.587962,
      1175.113940, 1139.762609, 1138.978460, 1138.978401, 1138.974499,
      1092.973876, 693.249751,  850.650934,  794.193638,  1222.681498,
      1316.430188, 1109.776271, 1079.300117, 1239.231971, 1644.540839,
      1609.866572, 1604.515037, 1603.618487, 1568.302641, 1157.017568,
      1138.980254, 1138.978724, 1138.974823, 1092.974187, 834.755585,
      1090.883669, 1236.131974, 1325.870346, 1110.190994, 1079.300152,
      1239.231971, 1644.540839, 1609.866572, 1604.515142, 1604.380213,
      1604.378532, 1585.580675, 1157.776710, 1138.980319, 1138.978724,
      1138.974823, 1092.974187, 1056.722338, 1249.164781, 1326.443132,
      1110.191843, 1079.300187, 1239.231971, 1644.540839, 1609.866572,
      1604.515142, 1604.380213, 1604.380190, 1604.378596, 1585.582205,
      1157.778240, 1138.980383, 1138.978724, 1138.974823, 1092.974187,
      1204.957557, 1326.440488, 1110.191892, 1079.300187, 1239.231971,
      1644.540839, 1609.866572, 1604.515142, 1604.380213, 1604.380190,
      1604.380190, 1604.378661, 1586.341349, 1175.056279, 1139.739527,
      1138.978789, 1138.974823, 1092.974187, 1275.872102, 1110.183396,
      1079.294485, 1239.226464, 1644.535343, 1609.861076, 1604.509646,
      1604.374717, 1604.374695, 1604.374695, 1604.374695, 1604.374630,
      1603.613892, 1568.297204, 1157.013600, 1138.976353, 1138.970921,
      1092.970443, 1014.576161, 1029.697894, 1172.002606, 1579.603765,
      1545.063972, 1539.712554, 1539.577625, 1539.577603, 1539.577603,
      1539.577603, 1539.577603, 1539.577603, 1539.576008, 1521.537360,
      1111.012900, 1092.975717, 1092.970443, 1048.827820,
  },
};

static const double interp_dgrid_surf[33 * 18] = {
  15.491252, 15.496413, 15.496379, 15.394447, 15.394431, 15.446941, 15.480480,
  15.480491, 15.480491, 15.480491, 15.480573, 15.603112, 15.603153, 15.602880,
  15.196621, 15.195983, 14.447585, 14.442076, 15.599420, 15.501644, 15.501575,
  15.399610, 15.399669, 15.452121, 15.485671, 15.485682, 15.485682, 15.485724,
  15.608303, 15.608385, 15.608113, 15.201854, 15.201079, 14.452681, 14.451350,
  13.210484, 15.599455, 15.501644, 15.501575, 15.399686, 15.626744, 15.452197,
  15.485671, 15.485682, 15.485682, 15.485765, 15.608344, 15.608113, 15.201854,
  15.201079, 14.452681, 14.451350, 13.214913, 13.208705, 15.599455, 15.501679,
  15.501575, 15.399686, 15.626820, 15.452197, 15.485671, 15.485682, 15.485765,
  15.608303, 15.608113, 15.201854, 15.201079, 14.452681, 14.451350, 13.214913,
  13.212184, 10.377543, 15.599489, 15.604652, 15.501610, 15.399686, 15.626820,
  15.452197, 15.485671, 15.485724, 15.608303, 15.608113, 15.201794, 15.201079,
  14.452681, 14.451350, 13.214913, 13.212184, 10.381022, 10.373758, 15.599489,
  15.604686, 15.501645, 15.399727, 15.626807, 15.452213, 15.485673, 15.485765,
  15.608072, 15.201734, 15.022022, 14.452621, 14.451350, 13.214913, 13.212184,
  10.381022, 10.375351, 4.752366,  15.599553, 15.604785, 15.606745, 15.523109,
  15.588440, 15.498592, 15.490939, 15.608113, 15.201734, 15.022022, 14.452398,
  14.451350, 13.214913, 13.212184, 10.381022, 10.375351, 4.753958,  4.748594,
  15.788491, 15.793763, 15.727178, 15.593404, 15.576185, 15.541155, 15.528638,
  15.443234, 15.022046, 14.452234, 13.963908, 13.214750, 13.212184, 10.381022,
  10.375351, 4.753958,  4.750187,  4.748513,  15.783667, 15.788604, 15.715040,
  15.574608, 15.604897, 15.345511, 15.322323, 15.003579, 14.280362, 13.963611,
  13.214382, 13.212184, 10.381022, 10.375351, 4.753958,  4.750106,  4.750025,
  4.508804,  15.747709, 14.765834, 15.706678, 15.619092, 15.637946, 15.437393,
  14.705235, 13.886464, 13.092309, 12.499506, 12.601594, 10.380302, 10.375350,
  4.753958,  4.750026,  4.510253,  4.510173,  4.508580,  15.641901, 14.968111,
  15.663074, 15.583828, 15.420627, 14.983936, 13.912901, 13.425841, 11.601603,
  10.600757, 10.110971, 8.839811,  4.753363,  4.749963,  4.510190,  4.322919,
  4.322857,  4.321407,  15.545263, 14.859883, 15.429167, 15.185696, 14.527376,
  13.650042, 12.244185, 10.755706, 9.356137,  8.229212,  8.166627,  7.324414,
  4.511146,  4.322709,  4.322566,  4.322503,  4.322503,  4.321054,  15.151031,
  14.579126, 14.592456, 13.733213, 11.980508, 10.685709, 9.143695,  7.466555,
  6.434770,  5.659204,  5.592407,  5.356211,  4.597869,  3.456439,  3.456056,
  3.456176,  3.456176,  3.455018,  13.478598, 13.969965, 10.921203, 9.274388,
  7.530261,  6.614303,  5.617812,  4.558886,  4.008929,  3.739688,  3.597707,
  3.510418,  3.041638,  3.096611,  3.096750,  3.455008,  3.455038,  3.453880,
  11.982351, 11.102072, 6.384662,  5.366178,  4.365376,  3.782955,  3.285890,
  2.758904,  2.439067,  2.338593,  2.257332,  2.190385,  2.088145,  1.867744,
  1.867445,  1.196878,  0.929332,  0.928931,  6.882587,  5.102224,  3.928701,
  3.021205,  2.614349,  2.162491,  1.870802,  1.628127,  1.448370,  1.352404,
  1.349332,  1.277236,  1.128399,  0.928668,  0.928601,  0.928485,  0.928395,
  0.928084,  4.066496,  3.233175,  2.413117,  1.830193,  1.473572,  1.273461,
  1.065156,  0.929824,  0.844729,  0.777037,  0.754300,  0.759737,  0.658871,
  0.603118,  0.603209,  0.928096,  0.928205,  0.927893,  2.747433,  2.039019,
  1.400016,  1.123698,  0.918772,  0.744824,  0.627536,  0.528263,  0.480634,
  0.446093,  0.423444,  0.401394,  0.394353,  0.360914,  0.360903,  0.361012,
  0.361012,  0.360891,  1.709038,  1.258373,  0.985529,  0.673582,  0.559577,
  0.456411,  0.372560,  0.312622,  0.268425,  0.251698,  0.238132,  0.222477,
  0.209643,  0.165264,  0.165254,  0.165254,  0.165254,  0.165199,  1.088160,
  0.806677,  0.673453,  0.490758,  0.350173,  0.277724,  0.229342,  0.190245,
  0.158717,  0.138414,  0.132405,  0.123800,  0.120550,  0.128355,  0.142808,
  0.142812,  0.142800,  0.142752,  0.806227,  0.588621,  0.490714,  0.350136,
  0.236497,  0.188583,  0.143991,  0.115039,  0.097317,  0.081406,  0.071159,
  0.067980,  0.062769,  0.066575,  0.066581,  0.066569,  0.030318,  0.030291,
  0.588339,  0.456572,  0.302189,  0.236473,  0.188564,  0.126491,  0.101248,
  0.075242,  0.059591,  0.051942,  0.042824,  0.037390,  0.034523,  0.030277,
  0.030275,  0.030275,  0.030258,  0.014677,  0.456374,  0.302172,  0.236448,
  0.164278,  0.126481,  0.094667,  0.068489,  0.053835,  0.038513,  0.031335,
  0.028947,  0.022753,  0.019347,  0.014676,  0.014675,  0.014674,  0.014674,
  0.014664,  0.302014,  0.221625,  0.164273,  0.126473,  0.094667,  0.068483,
  0.053831,  0.034298,  0.027445,  0.022220,  0.016315,  0.015692,  0.012949,
  0.009448,  0.009442,  0.007914,  0.007913,  0.007910,  0.221524,  0.164268,
  0.126473,  0.094667,  0.068480,  0.044800,  0.034295,  0.027442,  0.019666,
  0.014389,  0.011041,  0.011550,  0.009117,  0.021326,  0.007915,  0.007911,
  0.007911,  0.007908,  0.164193,  0.126473,  0.094667,  0.068480,  0.044800,
  0.034291,  0.027442,  0.019665,  0.014388,  0.008515,  0.008320,  0.009117,
  0.007357,  0.004348,  0.004343,  0.007908,  0.007910,  0.007907,  0.125623,
  0.094667,  0.068480,  0.044800,  0.034289,  0.023364,  0.019663,  0.011887,
  0.008514,  0.008318,  0.007208,  0.007208,  0.007206,  0.004342,  0.004341,
  0.004342,  0.004342,  0.004341,  0.094624,  0.068480,  0.044800,  0.034289,
  0.023364,  0.019662,  0.011887,  0.008514,  0.008318,  0.007208,  0.007207,
  0.007207,  0.007206,  0.004342,  0.004341,  0.004341,  0.004341,  0.004340,
  0.068449,  0.044800,  0.034289,  0.023364,  0.019662,  0.011887,  0.008514,
  0.008318,  0.007208,  0.007207,  0.007207,  0.007207,  0.007206,  0.004343,
  0.004341,  0.004341,  0.004341,  0.004340,  0.044777,  0.034289,  0.023364,
  0.019662,  0.011887,  0.008514,  0.008318,  0.007208,  0.007207,  0.007207,
  0.007207,  0.007207,  0.007207,  0.007205,  0.004342,  0.004341,  0.004341,
  0.004340,  0.034274,  0.023364,  0.019662,  0.011887,  0.008514,  0.008318,
  0.007208,  0.007207,  0.007207,  0.007207,  0.007207,  0.007207,  0.007207,
  0.007206,  0.004342,  0.004341,  0.004341,  0.004340,  0.023353,  0.019662,
  0.011887,  0.008514,  0.008318,  0.007208,  0.007207,  0.007207,  0.007207,
  0.007207,  0.007207,  0.007207,  0.007207,  0.007206,  0.004343,  0.004341,
  0.004341,  0.004340,  0.019650,  0.011884,  0.008511,  0.008316,  0.007205,
  0.007205,  0.007205,  0.007205,  0.007205,  0.007205,  0.007205,  0.007205,
  0.007205,  0.007205,  0.007203,  0.004341,  0.004340,  0.004338,
};

void av1_model_rd_surffit(BLOCK_SIZE bsize, double sse_norm, double xm,
                          double yl, double *rate_f, double *dist_f) {
  (void)sse_norm;
  const double x_start = -0.5;
  const double x_end = 16.5;
  const double x_step = 1.0;
  const double y_start = -15.5;
  const double y_end = 16.5;
  const double y_step = 1.0;
  const double epsilon = 1e-6;
  const int stride = (int)rint((x_end - x_start) / x_step) + 1;
  const int rcat = bsize_model_cat_lookup[bsize];
  (void)y_end;

  xm = AOMMAX(xm, x_start + x_step + epsilon);
  xm = AOMMIN(xm, x_end - x_step - epsilon);
  yl = AOMMAX(yl, y_start + y_step + epsilon);
  yl = AOMMIN(yl, y_end - y_step - epsilon);

  const double y = (yl - y_start) / y_step;
  const double x = (xm - x_start) / x_step;

  const int yi = (int)floor(y);
  const int xi = (int)floor(x);
  assert(xi > 0);
  assert(yi > 0);

  const double yo = y - yi;
  const double xo = x - xi;
  const double *prate = &interp_rgrid_surf[rcat][(yi - 1) * stride + (xi - 1)];
  const double *pdist = &interp_dgrid_surf[(yi - 1) * stride + (xi - 1)];
  *rate_f = interp_bicubic(prate, stride, xo, yo);
  *dist_f = interp_bicubic(pdist, stride, xo, yo);
}

static const double interp_rgrid_curv[4][65] = {
  {
      0.000000,    0.000000,    0.000000,    0.000000,    0.000000,
      0.000000,    0.000000,    0.000000,    0.000000,    0.000000,
      0.000000,    23.801499,   28.387688,   33.388795,   42.298282,
      41.525408,   51.597692,   49.566271,   54.632979,   60.321507,
      67.730678,   75.766165,   85.324032,   96.600012,   120.839562,
      173.917577,  255.974908,  354.107573,  458.063476,  562.345966,
      668.568424,  772.072881,  878.598490,  982.202274,  1082.708946,
      1188.037853, 1287.702240, 1395.588773, 1490.825830, 1584.231230,
      1691.386090, 1766.822555, 1869.630904, 1926.743565, 2002.949495,
      2047.431137, 2138.486068, 2154.743767, 2209.242472, 2277.593051,
      2290.996432, 2307.452938, 2343.567091, 2397.654644, 2469.425868,
      2558.591037, 2664.860422, 2787.944296, 2927.552932, 3083.396602,
      3255.185579, 3442.630134, 3645.440541, 3863.327072, 4096.000000,
  },
  {
      0.000000,    0.000000,    0.000000,    0.000000,    0.000000,
      0.000000,    0.000000,    0.000000,    0.000000,    0.000000,
      0.000000,    8.998436,    9.439592,    9.731837,    10.865931,
      11.561347,   12.578139,   14.205101,   16.770584,   19.094853,
      21.330863,   23.298907,   26.901921,   34.501017,   57.891733,
      112.234763,  194.853189,  288.302032,  380.499422,  472.625309,
      560.226809,  647.928463,  734.155122,  817.489721,  906.265783,
      999.260562,  1094.489206, 1197.062998, 1293.296825, 1378.926484,
      1472.760990, 1552.663779, 1635.196884, 1692.451951, 1759.741063,
      1822.162720, 1916.515921, 1966.686071, 2031.647506, 2033.700134,
      2087.847688, 2161.688858, 2242.536028, 2334.023491, 2436.337802,
      2549.665519, 2674.193198, 2810.107395, 2957.594666, 3116.841567,
      3288.034655, 3471.360486, 3667.005616, 3875.156602, 4096.000000,
  },
  {
      0.000000,    0.000000,    0.000000,    0.000000,    0.000000,
      0.000000,    0.000000,    0.000000,    0.000000,    0.000000,
      0.000000,    2.377584,    2.557185,    2.732445,    2.851114,
      3.281800,    3.765589,    4.342578,    5.145582,    5.611038,
      6.642238,    7.945977,    11.800522,   17.346624,   37.501413,
      87.216800,   165.860942,  253.865564,  332.039345,  408.518863,
      478.120452,  547.268590,  616.067676,  680.022540,  753.863541,
      834.529973,  919.489191,  1008.264989, 1092.230318, 1173.971886,
      1249.514122, 1330.510941, 1399.523249, 1466.923387, 1530.533471,
      1586.515722, 1695.197774, 1746.648696, 1837.136959, 1909.075485,
      1975.074651, 2060.159200, 2155.335095, 2259.762505, 2373.710437,
      2497.447898, 2631.243895, 2775.367434, 2930.087523, 3095.673170,
      3272.393380, 3460.517161, 3660.313520, 3872.051464, 4096.000000,
  },
  {
      0.000000,    0.000000,    0.000000,    0.000000,    0.000000,
      0.000000,    0.000000,    0.000000,    0.000000,    0.000000,
      0.000000,    0.296997,    0.342545,    0.403097,    0.472889,
      0.614483,    0.842937,    1.050824,    1.326663,    1.717750,
      2.530591,    3.582302,    6.995373,    9.973335,    24.042464,
      56.598240,   113.680735,  180.018689,  231.050567,  266.101082,
      294.957934,  323.326511,  349.434429,  380.443211,  408.171987,
      441.214916,  475.716772,  512.900000,  551.186939,  592.364455,
      624.527378,  661.940693,  679.185473,  724.800679,  764.781792,
      873.050019,  950.299001,  939.292954,  1052.406153, 1033.893184,
      1112.182406, 1219.174326, 1337.296681, 1471.648357, 1622.492809,
      1790.093491, 1974.713858, 2176.617364, 2396.067465, 2633.327614,
      2888.661266, 3162.331876, 3454.602899, 3765.737789, 4096.000000,
  },
};

static const double interp_dgrid_curv[2][65] = {
  {
      16.000000, 15.962891, 15.925174, 15.886888, 15.848074, 15.808770,
      15.769015, 15.728850, 15.688313, 15.647445, 15.606284, 15.564870,
      15.525918, 15.483820, 15.373330, 15.126844, 14.637442, 14.184387,
      13.560070, 12.880717, 12.165995, 11.378144, 10.438769, 9.130790,
      7.487633,  5.688649,  4.267515,  3.196300,  2.434201,  1.834064,
      1.369920,  1.035921,  0.775279,  0.574895,  0.427232,  0.314123,
      0.233236,  0.171440,  0.128188,  0.092762,  0.067569,  0.049324,
      0.036330,  0.027008,  0.019853,  0.015539,  0.011093,  0.008733,
      0.007624,  0.008105,  0.005427,  0.004065,  0.003427,  0.002848,
      0.002328,  0.001865,  0.001457,  0.001103,  0.000801,  0.000550,
      0.000348,  0.000193,  0.000085,  0.000021,  0.000000,
  },
  {
      16.000000, 15.996116, 15.984769, 15.966413, 15.941505, 15.910501,
      15.873856, 15.832026, 15.785466, 15.734633, 15.679981, 15.621967,
      15.560961, 15.460157, 15.288367, 15.052462, 14.466922, 13.921212,
      13.073692, 12.222005, 11.237799, 9.985848,  8.898823,  7.423519,
      5.995325,  4.773152,  3.744032,  2.938217,  2.294526,  1.762412,
      1.327145,  1.020728,  0.765535,  0.570548,  0.425833,  0.313825,
      0.232959,  0.171324,  0.128174,  0.092750,  0.067558,  0.049319,
      0.036330,  0.027008,  0.019853,  0.015539,  0.011093,  0.008733,
      0.007624,  0.008105,  0.005427,  0.004065,  0.003427,  0.002848,
      0.002328,  0.001865,  0.001457,  0.001103,  0.000801,  0.000550,
      0.000348,  0.000193,  0.000085,  0.000021,  -0.000000,
  },
};

void av1_model_rd_curvfit(BLOCK_SIZE bsize, double sse_norm, double xqr,
                          double *rate_f, double *distbysse_f) {
  const double x_start = -15.5;
  const double x_end = 16.5;
  const double x_step = 0.5;
  const double epsilon = 1e-6;
  const int rcat = bsize_model_cat_lookup[bsize];
  const int dcat = sse_norm_model_cat_lookup(sse_norm);
  (void)x_end;

  xqr = AOMMAX(xqr, x_start + x_step + epsilon);
  xqr = AOMMIN(xqr, x_end - x_step - epsilon);
  const double x = (xqr - x_start) / x_step;
  const int xi = (int)floor(x);
  const double xo = x - xi;

  assert(xi > 0);

  const double *prate = &interp_rgrid_curv[rcat][(xi - 1)];
  *rate_f = interp_cubic(prate, xo);
  const double *pdist = &interp_dgrid_curv[dcat][(xi - 1)];
  *distbysse_f = interp_cubic(pdist, xo);
}

static void get_entropy_contexts_plane(BLOCK_SIZE plane_bsize,
                                       const struct macroblockd_plane *pd,
                                       ENTROPY_CONTEXT t_above[MAX_MIB_SIZE],
                                       ENTROPY_CONTEXT t_left[MAX_MIB_SIZE]) {
  const int num_4x4_w = block_size_wide[plane_bsize] >> tx_size_wide_log2[0];
  const int num_4x4_h = block_size_high[plane_bsize] >> tx_size_high_log2[0];
  const ENTROPY_CONTEXT *const above = pd->above_context;
  const ENTROPY_CONTEXT *const left = pd->left_context;

  memcpy(t_above, above, sizeof(ENTROPY_CONTEXT) * num_4x4_w);
  memcpy(t_left, left, sizeof(ENTROPY_CONTEXT) * num_4x4_h);
}

void av1_get_entropy_contexts(BLOCK_SIZE bsize,
                              const struct macroblockd_plane *pd,
                              ENTROPY_CONTEXT t_above[MAX_MIB_SIZE],
                              ENTROPY_CONTEXT t_left[MAX_MIB_SIZE]) {
  const BLOCK_SIZE plane_bsize =
      get_plane_block_size(bsize, pd->subsampling_x, pd->subsampling_y);
  get_entropy_contexts_plane(plane_bsize, pd, t_above, t_left);
}

void av1_mv_pred(const AV1_COMP *cpi, MACROBLOCK *x, uint8_t *ref_y_buffer,
                 int ref_y_stride, int ref_frame, BLOCK_SIZE block_size) {
  int i;
  int zero_seen = 0;
  int best_sad = INT_MAX;
  int this_sad = INT_MAX;
  int max_mv = 0;
  uint8_t *src_y_ptr = x->plane[0].src.buf;
  uint8_t *ref_y_ptr;
  MV pred_mv[MAX_MV_REF_CANDIDATES + 1];
  int num_mv_refs = 0;
  const MV_REFERENCE_FRAME ref_frames[2] = { ref_frame, NONE_FRAME };
  const int_mv ref_mv =
      av1_get_ref_mv_from_stack(0, ref_frames, 0, x->mbmi_ext);
  const int_mv ref_mv1 =
      av1_get_ref_mv_from_stack(0, ref_frames, 1, x->mbmi_ext);

  pred_mv[num_mv_refs++] = ref_mv.as_mv;
  if (ref_mv.as_int != ref_mv1.as_int) {
    pred_mv[num_mv_refs++] = ref_mv1.as_mv;
  }
  if (cpi->sf.adaptive_motion_search && block_size < x->max_partition_size)
    pred_mv[num_mv_refs++] = x->pred_mv[ref_frame];

  assert(num_mv_refs <= (int)(sizeof(pred_mv) / sizeof(pred_mv[0])));

  // Get the sad for each candidate reference mv.
  for (i = 0; i < num_mv_refs; ++i) {
    const MV *this_mv = &pred_mv[i];
    int fp_row, fp_col;
    fp_row = (this_mv->row + 3 + (this_mv->row >= 0)) >> 3;
    fp_col = (this_mv->col + 3 + (this_mv->col >= 0)) >> 3;
    max_mv = AOMMAX(max_mv, AOMMAX(abs(this_mv->row), abs(this_mv->col)) >> 3);

    if (fp_row == 0 && fp_col == 0 && zero_seen) continue;
    zero_seen |= (fp_row == 0 && fp_col == 0);

    ref_y_ptr = &ref_y_buffer[ref_y_stride * fp_row + fp_col];
    // Find sad for current vector.
    this_sad = cpi->fn_ptr[block_size].sdf(src_y_ptr, x->plane[0].src.stride,
                                           ref_y_ptr, ref_y_stride);
    // Note if it is the best so far.
    if (this_sad < best_sad) {
      best_sad = this_sad;
    }
  }

  // Note the index of the mv that worked best in the reference list.
  x->max_mv_context[ref_frame] = max_mv;
  x->pred_mv_sad[ref_frame] = best_sad;
}

void av1_setup_pred_block(const MACROBLOCKD *xd,
                          struct buf_2d dst[MAX_MB_PLANE],
                          const YV12_BUFFER_CONFIG *src, int mi_row, int mi_col,
                          const struct scale_factors *scale,
                          const struct scale_factors *scale_uv,
                          const int num_planes) {
  int i;

  dst[0].buf = src->y_buffer;
  dst[0].stride = src->y_stride;
  dst[1].buf = src->u_buffer;
  dst[2].buf = src->v_buffer;
  dst[1].stride = dst[2].stride = src->uv_stride;

  for (i = 0; i < num_planes; ++i) {
    setup_pred_plane(dst + i, xd->mi[0]->sb_type, dst[i].buf,
                     i ? src->uv_crop_width : src->y_crop_width,
                     i ? src->uv_crop_height : src->y_crop_height,
                     dst[i].stride, mi_row, mi_col, i ? scale_uv : scale,
                     xd->plane[i].subsampling_x, xd->plane[i].subsampling_y);
  }
}

int av1_raster_block_offset(BLOCK_SIZE plane_bsize, int raster_block,
                            int stride) {
  const int bw = mi_size_wide_log2[plane_bsize];
  const int y = 4 * (raster_block >> bw);
  const int x = 4 * (raster_block & ((1 << bw) - 1));
  return y * stride + x;
}

int16_t *av1_raster_block_offset_int16(BLOCK_SIZE plane_bsize, int raster_block,
                                       int16_t *base) {
  const int stride = block_size_wide[plane_bsize];
  return base + av1_raster_block_offset(plane_bsize, raster_block, stride);
}

YV12_BUFFER_CONFIG *av1_get_scaled_ref_frame(const AV1_COMP *cpi,
                                             int ref_frame) {
  assert(ref_frame >= LAST_FRAME && ref_frame <= ALTREF_FRAME);
  RefCntBuffer *const scaled_buf = cpi->scaled_ref_buf[ref_frame - 1];
  const RefCntBuffer *const ref_buf =
      get_ref_frame_buf(&cpi->common, ref_frame);
  return (scaled_buf != ref_buf && scaled_buf != NULL) ? &scaled_buf->buf
                                                       : NULL;
}

int av1_get_switchable_rate(const AV1_COMMON *const cm, MACROBLOCK *x,
                            const MACROBLOCKD *xd) {
  if (cm->interp_filter == SWITCHABLE) {
    const MB_MODE_INFO *const mbmi = xd->mi[0];
    int inter_filter_cost = 0;
    int dir;

    for (dir = 0; dir < 2; ++dir) {
      const int ctx = av1_get_pred_context_switchable_interp(xd, dir);
      const InterpFilter filter =
          av1_extract_interp_filter(mbmi->interp_filters, dir);
      inter_filter_cost += x->switchable_interp_costs[ctx][filter];
    }
    return SWITCHABLE_INTERP_RATE_FACTOR * inter_filter_cost;
  } else {
    return 0;
  }
}

void av1_set_rd_speed_thresholds(AV1_COMP *cpi) {
  int i;
  RD_OPT *const rd = &cpi->rd;
  SPEED_FEATURES *const sf = &cpi->sf;

  // Set baseline threshold values.
  for (i = 0; i < MAX_MODES; ++i) rd->thresh_mult[i] = cpi->oxcf.mode == 0;

  if (sf->adaptive_rd_thresh) {
    rd->thresh_mult[THR_NEARESTMV] = 300;
    rd->thresh_mult[THR_NEARESTL2] = 300;
    rd->thresh_mult[THR_NEARESTL3] = 300;
    rd->thresh_mult[THR_NEARESTB] = 300;
    rd->thresh_mult[THR_NEARESTA2] = 300;
    rd->thresh_mult[THR_NEARESTA] = 300;
    rd->thresh_mult[THR_NEARESTG] = 300;
  } else {
    rd->thresh_mult[THR_NEARESTMV] = 0;
    rd->thresh_mult[THR_NEARESTL2] = 0;
    rd->thresh_mult[THR_NEARESTL3] = 100;
    rd->thresh_mult[THR_NEARESTB] = 0;
    rd->thresh_mult[THR_NEARESTA2] = 0;
    rd->thresh_mult[THR_NEARESTA] = 0;
    rd->thresh_mult[THR_NEARESTG] = 0;
  }

  rd->thresh_mult[THR_NEWMV] += 1000;
  rd->thresh_mult[THR_NEWL2] += 1000;
  rd->thresh_mult[THR_NEWL3] += 1000;
  rd->thresh_mult[THR_NEWB] += 1000;
  rd->thresh_mult[THR_NEWA2] = 1100;
  rd->thresh_mult[THR_NEWA] += 1000;
  rd->thresh_mult[THR_NEWG] += 1000;

  rd->thresh_mult[THR_NEARMV] += 1000;
  rd->thresh_mult[THR_NEARL2] += 1000;
  rd->thresh_mult[THR_NEARL3] += 1000;
  rd->thresh_mult[THR_NEARB] += 1000;
  rd->thresh_mult[THR_NEARA2] = 1000;
  rd->thresh_mult[THR_NEARA] += 1000;
  rd->thresh_mult[THR_NEARG] += 1000;

  rd->thresh_mult[THR_GLOBALMV] += 2200;
  rd->thresh_mult[THR_GLOBALL2] += 2000;
  rd->thresh_mult[THR_GLOBALL3] += 2000;
  rd->thresh_mult[THR_GLOBALB] += 2400;
  rd->thresh_mult[THR_GLOBALA2] = 2000;
  rd->thresh_mult[THR_GLOBALG] += 2000;
  rd->thresh_mult[THR_GLOBALA] += 2400;

  rd->thresh_mult[THR_COMP_NEAREST_NEARESTLA] += 1100;
  rd->thresh_mult[THR_COMP_NEAREST_NEARESTL2A] += 1000;
  rd->thresh_mult[THR_COMP_NEAREST_NEARESTL3A] += 800;
  rd->thresh_mult[THR_COMP_NEAREST_NEARESTGA] += 900;
  rd->thresh_mult[THR_COMP_NEAREST_NEARESTLB] += 1000;
  rd->thresh_mult[THR_COMP_NEAREST_NEARESTL2B] += 1000;
  rd->thresh_mult[THR_COMP_NEAREST_NEARESTL3B] += 1000;
  rd->thresh_mult[THR_COMP_NEAREST_NEARESTGB] += 1000;
  rd->thresh_mult[THR_COMP_NEAREST_NEARESTLA2] += 1000;
  rd->thresh_mult[THR_COMP_NEAREST_NEARESTL2A2] += 1000;
  rd->thresh_mult[THR_COMP_NEAREST_NEARESTL3A2] += 1000;
  rd->thresh_mult[THR_COMP_NEAREST_NEARESTGA2] += 1000;

  rd->thresh_mult[THR_COMP_NEAREST_NEARESTLL2] += 2000;
  rd->thresh_mult[THR_COMP_NEAREST_NEARESTLL3] += 2000;
  rd->thresh_mult[THR_COMP_NEAREST_NEARESTLG] += 2000;
  rd->thresh_mult[THR_COMP_NEAREST_NEARESTBA] += 2000;

  rd->thresh_mult[THR_COMP_NEAR_NEARLA] += 1200;
  rd->thresh_mult[THR_COMP_NEAREST_NEWLA] += 1500;
  rd->thresh_mult[THR_COMP_NEW_NEARESTLA] += 1500;
  rd->thresh_mult[THR_COMP_NEAR_NEWLA] += 1530;
  rd->thresh_mult[THR_COMP_NEW_NEARLA] += 1870;
  rd->thresh_mult[THR_COMP_NEW_NEWLA] += 2400;
  rd->thresh_mult[THR_COMP_GLOBAL_GLOBALLA] += 2750;

  rd->thresh_mult[THR_COMP_NEAR_NEARL2A] += 1200;
  rd->thresh_mult[THR_COMP_NEAREST_NEWL2A] += 1500;
  rd->thresh_mult[THR_COMP_NEW_NEARESTL2A] += 1500;
  rd->thresh_mult[THR_COMP_NEAR_NEWL2A] += 1870;
  rd->thresh_mult[THR_COMP_NEW_NEARL2A] += 1700;
  rd->thresh_mult[THR_COMP_NEW_NEWL2A] += 1800;
  rd->thresh_mult[THR_COMP_GLOBAL_GLOBALL2A] += 2500;

  rd->thresh_mult[THR_COMP_NEAR_NEARL3A] += 1200;
  rd->thresh_mult[THR_COMP_NEAREST_NEWL3A] += 1500;
  rd->thresh_mult[THR_COMP_NEW_NEARESTL3A] += 1500;
  rd->thresh_mult[THR_COMP_NEAR_NEWL3A] += 1700;
  rd->thresh_mult[THR_COMP_NEW_NEARL3A] += 1700;
  rd->thresh_mult[THR_COMP_NEW_NEWL3A] += 2000;
  rd->thresh_mult[THR_COMP_GLOBAL_GLOBALL3A] += 3000;

  rd->thresh_mult[THR_COMP_NEAR_NEARGA] += 1320;
  rd->thresh_mult[THR_COMP_NEAREST_NEWGA] += 1500;
  rd->thresh_mult[THR_COMP_NEW_NEARESTGA] += 1500;
  rd->thresh_mult[THR_COMP_NEAR_NEWGA] += 2040;
  rd->thresh_mult[THR_COMP_NEW_NEARGA] += 1700;
  rd->thresh_mult[THR_COMP_NEW_NEWGA] += 2000;
  rd->thresh_mult[THR_COMP_GLOBAL_GLOBALGA] += 2250;

  rd->thresh_mult[THR_COMP_NEAR_NEARLB] += 1200;
  rd->thresh_mult[THR_COMP_NEAREST_NEWLB] += 1500;
  rd->thresh_mult[THR_COMP_NEW_NEARESTLB] += 1500;
  rd->thresh_mult[THR_COMP_NEAR_NEWLB] += 1360;
  rd->thresh_mult[THR_COMP_NEW_NEARLB] += 1700;
  rd->thresh_mult[THR_COMP_NEW_NEWLB] += 2400;
  rd->thresh_mult[THR_COMP_GLOBAL_GLOBALLB] += 2250;

  rd->thresh_mult[THR_COMP_NEAR_NEARL2B] += 1200;
  rd->thresh_mult[THR_COMP_NEAREST_NEWL2B] += 1500;
  rd->thresh_mult[THR_COMP_NEW_NEARESTL2B] += 1500;
  rd->thresh_mult[THR_COMP_NEAR_NEWL2B] += 1700;
  rd->thresh_mult[THR_COMP_NEW_NEARL2B] += 1700;
  rd->thresh_mult[THR_COMP_NEW_NEWL2B] += 2000;
  rd->thresh_mult[THR_COMP_GLOBAL_GLOBALL2B] += 2500;

  rd->thresh_mult[THR_COMP_NEAR_NEARL3B] += 1200;
  rd->thresh_mult[THR_COMP_NEAREST_NEWL3B] += 1500;
  rd->thresh_mult[THR_COMP_NEW_NEARESTL3B] += 1500;
  rd->thresh_mult[THR_COMP_NEAR_NEWL3B] += 1870;
  rd->thresh_mult[THR_COMP_NEW_NEARL3B] += 1700;
  rd->thresh_mult[THR_COMP_NEW_NEWL3B] += 2000;
  rd->thresh_mult[THR_COMP_GLOBAL_GLOBALL3B] += 2500;

  rd->thresh_mult[THR_COMP_NEAR_NEARGB] += 1200;
  rd->thresh_mult[THR_COMP_NEAREST_NEWGB] += 1500;
  rd->thresh_mult[THR_COMP_NEW_NEARESTGB] += 1500;
  rd->thresh_mult[THR_COMP_NEAR_NEWGB] += 1700;
  rd->thresh_mult[THR_COMP_NEW_NEARGB] += 1700;
  rd->thresh_mult[THR_COMP_NEW_NEWGB] += 2000;
  rd->thresh_mult[THR_COMP_GLOBAL_GLOBALGB] += 2500;

  rd->thresh_mult[THR_COMP_NEAR_NEARLA2] += 1200;
  rd->thresh_mult[THR_COMP_NEAREST_NEWLA2] += 1800;
  rd->thresh_mult[THR_COMP_NEW_NEARESTLA2] += 1500;
  rd->thresh_mult[THR_COMP_NEAR_NEWLA2] += 1700;
  rd->thresh_mult[THR_COMP_NEW_NEARLA2] += 1700;
  rd->thresh_mult[THR_COMP_NEW_NEWLA2] += 2000;
  rd->thresh_mult[THR_COMP_GLOBAL_GLOBALLA2] += 2500;

  rd->thresh_mult[THR_COMP_NEAR_NEARL2A2] += 1200;
  rd->thresh_mult[THR_COMP_NEAREST_NEWL2A2] += 1500;
  rd->thresh_mult[THR_COMP_NEW_NEARESTL2A2] += 1500;
  rd->thresh_mult[THR_COMP_NEAR_NEWL2A2] += 1700;
  rd->thresh_mult[THR_COMP_NEW_NEARL2A2] += 1700;
  rd->thresh_mult[THR_COMP_NEW_NEWL2A2] += 2000;
  rd->thresh_mult[THR_COMP_GLOBAL_GLOBALL2A2] += 2500;

  rd->thresh_mult[THR_COMP_NEAR_NEARL3A2] += 1440;
  rd->thresh_mult[THR_COMP_NEAREST_NEWL3A2] += 1500;
  rd->thresh_mult[THR_COMP_NEW_NEARESTL3A2] += 1500;
  rd->thresh_mult[THR_COMP_NEAR_NEWL3A2] += 1700;
  rd->thresh_mult[THR_COMP_NEW_NEARL3A2] += 1700;
  rd->thresh_mult[THR_COMP_NEW_NEWL3A2] += 2000;
  rd->thresh_mult[THR_COMP_GLOBAL_GLOBALL3A2] += 2500;

  rd->thresh_mult[THR_COMP_NEAR_NEARGA2] += 1200;
  rd->thresh_mult[THR_COMP_NEAREST_NEWGA2] += 1500;
  rd->thresh_mult[THR_COMP_NEW_NEARESTGA2] += 1500;
  rd->thresh_mult[THR_COMP_NEAR_NEWGA2] += 1700;
  rd->thresh_mult[THR_COMP_NEW_NEARGA2] += 1700;
  rd->thresh_mult[THR_COMP_NEW_NEWGA2] += 2000;
  rd->thresh_mult[THR_COMP_GLOBAL_GLOBALGA2] += 2750;

  rd->thresh_mult[THR_COMP_NEAR_NEARLL2] += 1600;
  rd->thresh_mult[THR_COMP_NEAREST_NEWLL2] += 2000;
  rd->thresh_mult[THR_COMP_NEW_NEARESTLL2] += 2000;
  rd->thresh_mult[THR_COMP_NEAR_NEWLL2] += 2640;
  rd->thresh_mult[THR_COMP_NEW_NEARLL2] += 2200;
  rd->thresh_mult[THR_COMP_NEW_NEWLL2] += 2400;
  rd->thresh_mult[THR_COMP_GLOBAL_GLOBALLL2] += 3200;

  rd->thresh_mult[THR_COMP_NEAR_NEARLL3] += 1600;
  rd->thresh_mult[THR_COMP_NEAREST_NEWLL3] += 2000;
  rd->thresh_mult[THR_COMP_NEW_NEARESTLL3] += 1800;
  rd->thresh_mult[THR_COMP_NEAR_NEWLL3] += 2200;
  rd->thresh_mult[THR_COMP_NEW_NEARLL3] += 2200;
  rd->thresh_mult[THR_COMP_NEW_NEWLL3] += 2400;
  rd->thresh_mult[THR_COMP_GLOBAL_GLOBALLL3] += 3200;

  rd->thresh_mult[THR_COMP_NEAR_NEARLG] += 1760;
  rd->thresh_mult[THR_COMP_NEAREST_NEWLG] += 2400;
  rd->thresh_mult[THR_COMP_NEW_NEARESTLG] += 2000;
  rd->thresh_mult[THR_COMP_NEAR_NEWLG] += 1760;
  rd->thresh_mult[THR_COMP_NEW_NEARLG] += 2640;
  rd->thresh_mult[THR_COMP_NEW_NEWLG] += 2400;
  rd->thresh_mult[THR_COMP_GLOBAL_GLOBALLG] += 3200;

  rd->thresh_mult[THR_COMP_NEAR_NEARBA] += 1600;
  rd->thresh_mult[THR_COMP_NEAREST_NEWBA] += 2000;
  rd->thresh_mult[THR_COMP_NEW_NEARESTBA] += 2000;
  rd->thresh_mult[THR_COMP_NEAR_NEWBA] += 2200;
  rd->thresh_mult[THR_COMP_NEW_NEARBA] += 1980;
  rd->thresh_mult[THR_COMP_NEW_NEWBA] += 2640;
  rd->thresh_mult[THR_COMP_GLOBAL_GLOBALBA] += 3200;

  rd->thresh_mult[THR_DC] += 1000;
  rd->thresh_mult[THR_PAETH] += 1000;
  rd->thresh_mult[THR_SMOOTH] += 2200;
  rd->thresh_mult[THR_SMOOTH_V] += 2000;
  rd->thresh_mult[THR_SMOOTH_H] += 2000;
  rd->thresh_mult[THR_H_PRED] += 2000;
  rd->thresh_mult[THR_V_PRED] += 1800;
  rd->thresh_mult[THR_D135_PRED] += 2500;
  rd->thresh_mult[THR_D203_PRED] += 2000;
  rd->thresh_mult[THR_D157_PRED] += 2500;
  rd->thresh_mult[THR_D67_PRED] += 2000;
  rd->thresh_mult[THR_D113_PRED] += 2500;
  rd->thresh_mult[THR_D45_PRED] += 2500;
}

void av1_update_rd_thresh_fact(const AV1_COMMON *const cm,
                               int (*factor_buf)[MAX_MODES], int rd_thresh,
                               int bsize, int best_mode_index) {
  if (rd_thresh > 0) {
    const int top_mode = MAX_MODES;
    int mode;
    for (mode = 0; mode < top_mode; ++mode) {
      const BLOCK_SIZE min_size = AOMMAX(bsize - 1, BLOCK_4X4);
      const BLOCK_SIZE max_size =
          AOMMIN(bsize + 2, (int)cm->seq_params.sb_size);
      BLOCK_SIZE bs;
      for (bs = min_size; bs <= max_size; ++bs) {
        int *const fact = &factor_buf[bs][mode];
        if (mode == best_mode_index) {
          *fact -= (*fact >> 4);
        } else {
          *fact = AOMMIN(*fact + RD_THRESH_INC, rd_thresh * RD_THRESH_MAX_FACT);
        }
      }
    }
  }
}

int av1_get_intra_cost_penalty(int qindex, int qdelta,
                               aom_bit_depth_t bit_depth) {
  const int q = av1_dc_quant_Q3(qindex, qdelta, bit_depth);
  switch (bit_depth) {
    case AOM_BITS_8: return 20 * q;
    case AOM_BITS_10: return 5 * q;
    case AOM_BITS_12: return ROUND_POWER_OF_TWO(5 * q, 2);
    default:
      assert(0 && "bit_depth should be AOM_BITS_8, AOM_BITS_10 or AOM_BITS_12");
      return -1;
  }
}
