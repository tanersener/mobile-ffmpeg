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
#include <limits.h>
#include <math.h>
#include <stdio.h>

#include "config/aom_dsp_rtcd.h"
#include "config/av1_rtcd.h"

#include "aom_dsp/aom_dsp_common.h"
#include "aom_dsp/blend.h"
#include "aom_mem/aom_mem.h"
#include "aom_ports/aom_timer.h"
#include "aom_ports/mem.h"
#include "aom_ports/system_state.h"

#include "av1/common/mvref_common.h"
#include "av1/common/pred_common.h"
#include "av1/common/reconinter.h"
#include "av1/common/reconintra.h"

#include "av1/encoder/encodemv.h"
#include "av1/encoder/rdopt.h"
#include "av1/encoder/reconinter_enc.h"

#define _TMP_USE_CURVFIT_ 0

extern int g_pick_inter_mode_cnt;
typedef struct {
  uint8_t *data;
  int stride;
  int in_use;
} PRED_BUFFER;

typedef struct {
  PRED_BUFFER *best_pred;
  PREDICTION_MODE best_mode;
  TX_SIZE best_tx_size;
  TX_SIZE best_intra_tx_size;
  MV_REFERENCE_FRAME best_ref_frame;
  MV_REFERENCE_FRAME best_second_ref_frame;
  uint8_t best_mode_skip_txfm;
  InterpFilters best_pred_filter;
} BEST_PICKMODE;

typedef struct {
  MV_REFERENCE_FRAME ref_frame;
  PREDICTION_MODE pred_mode;
} REF_MODE;

#define RT_INTER_MODES 9
static const REF_MODE ref_mode_set[RT_INTER_MODES] = {
  { LAST_FRAME, NEARESTMV },   { LAST_FRAME, NEARMV },
  { LAST_FRAME, NEWMV },       { GOLDEN_FRAME, NEARESTMV },
  { GOLDEN_FRAME, NEARMV },    { GOLDEN_FRAME, NEWMV },
  { ALTREF_FRAME, NEARESTMV }, { ALTREF_FRAME, NEARMV },
  { ALTREF_FRAME, NEWMV }
};

static const THR_MODES mode_idx[REF_FRAMES][4] = {
  { THR_DC, THR_V_PRED, THR_H_PRED, THR_SMOOTH },
  { THR_NEARESTMV, THR_NEARMV, THR_GLOBALMV, THR_NEWMV },
  { THR_NEARESTG, THR_NEARG, THR_GLOBALMV, THR_NEWG },
  { THR_NEARESTA, THR_NEARA, THR_GLOBALMV, THR_NEWA },
};

static const PREDICTION_MODE intra_mode_list[] = { DC_PRED, V_PRED, H_PRED,
                                                   SMOOTH_PRED };

static INLINE int mode_offset(const PREDICTION_MODE mode) {
  if (mode >= NEARESTMV) {
    return INTER_OFFSET(mode);
  } else {
    switch (mode) {
      case DC_PRED: return 0;
      case V_PRED: return 1;
      case H_PRED: return 2;
      case SMOOTH_PRED: return 3;
      default: assert(0); return -1;
    }
  }
}

typedef struct {
  PREDICTION_MODE mode;
  MV_REFERENCE_FRAME ref_frame[2];
} MODE_DEFINITION;

enum {
  //  INTER_ALL = (1 << NEARESTMV) | (1 << NEARMV) | (1 << NEWMV),
  INTER_NEAREST = (1 << NEARESTMV),
  INTER_NEAREST_NEW = (1 << NEARESTMV) | (1 << NEWMV),
  INTER_NEAREST_NEAR = (1 << NEARESTMV) | (1 << NEARMV),
  INTER_NEAR_NEW = (1 << NEARMV) | (1 << NEWMV),
};

static INLINE void init_best_pickmode(BEST_PICKMODE *bp) {
  bp->best_mode = NEARESTMV;
  bp->best_ref_frame = LAST_FRAME;
  bp->best_tx_size = TX_8X8;
  bp->best_intra_tx_size = TX_8X8;
  bp->best_pred_filter = av1_broadcast_interp_filter(EIGHTTAP_REGULAR);
  bp->best_mode_skip_txfm = 0;
  bp->best_second_ref_frame = NONE_FRAME;
  bp->best_pred = NULL;
}

static int combined_motion_search(AV1_COMP *cpi, MACROBLOCK *x,
                                  BLOCK_SIZE bsize, int mi_row, int mi_col,
                                  int_mv *tmp_mv, int *rate_mv,
                                  int64_t best_rd_sofar, int use_base_mv) {
  MACROBLOCKD *xd = &x->e_mbd;
  const AV1_COMMON *cm = &cpi->common;
  const int num_planes = av1_num_planes(cm);
  MB_MODE_INFO *mi = xd->mi[0];
  struct buf_2d backup_yv12[MAX_MB_PLANE] = { { 0, 0, 0, 0, 0 } };
  int step_param = cpi->mv_step_param;
  const int sadpb = x->sadperbit16;
  MV mvp_full;
  const int ref = mi->ref_frame[0];
  const MV ref_mv = av1_get_ref_mv(x, mi->ref_mv_idx).as_mv;
  MV center_mv;
  int dis;
  const MvLimits tmp_mv_limits = x->mv_limits;
  int rv = 0;
  int cost_list[5];
  int search_subpel = 1;
  const YV12_BUFFER_CONFIG *scaled_ref_frame =
      av1_get_scaled_ref_frame(cpi, ref);

  if (scaled_ref_frame) {
    int i;
    // Swap out the reference frame for a version that's been scaled to
    // match the resolution of the current frame, allowing the existing
    // motion search code to be used without additional modifications.
    for (i = 0; i < MAX_MB_PLANE; i++) backup_yv12[i] = xd->plane[i].pre[0];
    av1_setup_pre_planes(xd, 0, scaled_ref_frame, mi_row, mi_col, NULL,
                         num_planes);
  }
  av1_set_mv_search_range(&x->mv_limits, &ref_mv);

  mvp_full = ref_mv;

  mvp_full.col >>= 3;
  mvp_full.row >>= 3;

  if (!use_base_mv)
    center_mv = ref_mv;
  else
    center_mv = tmp_mv->as_mv;

  av1_full_pixel_search(
      cpi, x, bsize, &mvp_full, step_param, cpi->sf.mv.search_method, 0, sadpb,
      cond_cost_list(cpi, cost_list), &center_mv, INT_MAX, 0,
      (MI_SIZE * mi_col), (MI_SIZE * mi_row), 0, &cpi->ss_cfg[SS_CFG_SRC]);

  x->mv_limits = tmp_mv_limits;
  *tmp_mv = x->best_mv;
  // calculate the bit cost on motion vector
  mvp_full.row = tmp_mv->as_mv.row * 8;
  mvp_full.col = tmp_mv->as_mv.col * 8;

  *rate_mv = av1_mv_bit_cost(&mvp_full, &ref_mv, x->nmv_vec_cost,
                             x->mv_cost_stack, MV_COST_WEIGHT);

  // TODO(kyslov) Account for Rate Mode!
  rv = !(RDCOST(x->rdmult, (*rate_mv), 0) > best_rd_sofar);

  if (rv && search_subpel) {
    SUBPEL_FORCE_STOP subpel_force_stop = cpi->sf.mv.subpel_force_stop;
    cpi->find_fractional_mv_step(
        x, cm, mi_row, mi_col, &ref_mv, cpi->common.allow_high_precision_mv,
        x->errorperbit, &cpi->fn_ptr[bsize], subpel_force_stop,
        cpi->sf.mv.subpel_iters_per_step, cond_cost_list(cpi, cost_list),
        x->nmv_vec_cost, x->mv_cost_stack, &dis, &x->pred_sse[ref], NULL, NULL,
        0, 0, 0, 0, 0, 1);
    *tmp_mv = x->best_mv;
    *rate_mv = av1_mv_bit_cost(&tmp_mv->as_mv, &ref_mv, x->nmv_vec_cost,
                               x->mv_cost_stack, MV_COST_WEIGHT);
  }

  if (scaled_ref_frame) {
    int i;
    for (i = 0; i < MAX_MB_PLANE; i++) xd->plane[i].pre[0] = backup_yv12[i];
  }
  return rv;
}

static int search_new_mv(AV1_COMP *cpi, MACROBLOCK *x,
                         int_mv frame_mv[][REF_FRAMES],
                         MV_REFERENCE_FRAME ref_frame, int gf_temporal_ref,
                         BLOCK_SIZE bsize, int mi_row, int mi_col,
                         int best_pred_sad, int *rate_mv,
                         int64_t best_sse_sofar, RD_STATS *best_rdc) {
  MACROBLOCKD *const xd = &x->e_mbd;
  MB_MODE_INFO *const mi = xd->mi[0];
  AV1_COMMON *cm = &cpi->common;
  (void)best_sse_sofar;
  if (ref_frame > LAST_FRAME && gf_temporal_ref &&
      cpi->oxcf.rc_mode == AOM_CBR) {
    int tmp_sad;
    int dis;
    int cost_list[5] = { INT_MAX, INT_MAX, INT_MAX, INT_MAX, INT_MAX };

    if (bsize < BLOCK_16X16) return -1;

    tmp_sad = av1_int_pro_motion_estimation(
        cpi, x, bsize, mi_row, mi_col,
        &x->mbmi_ext->ref_mv_stack[ref_frame][0].this_mv.as_mv);

    if (tmp_sad > x->pred_mv_sad[LAST_FRAME]) return -1;
    if (tmp_sad + (num_pels_log2_lookup[bsize] << 4) > best_pred_sad) return -1;

    frame_mv[NEWMV][ref_frame].as_int = mi->mv[0].as_int;
    MV ref_mv = av1_get_ref_mv(x, 0).as_mv;

    *rate_mv =
        av1_mv_bit_cost(&frame_mv[NEWMV][ref_frame].as_mv, &ref_mv,
                        x->nmv_vec_cost, x->mv_cost_stack, MV_COST_WEIGHT);
    frame_mv[NEWMV][ref_frame].as_mv.row >>= 3;
    frame_mv[NEWMV][ref_frame].as_mv.col >>= 3;

    cpi->find_fractional_mv_step(
        x, cm, mi_row, mi_col, &ref_mv, cm->allow_high_precision_mv,
        x->errorperbit, &cpi->fn_ptr[bsize], cpi->sf.mv.subpel_force_stop,
        cpi->sf.mv.subpel_iters_per_step, cond_cost_list(cpi, cost_list),
        x->nmv_vec_cost, x->mv_cost_stack, &dis, &x->pred_sse[ref_frame], NULL,
        NULL, 0, 0, 0, 0, 0, 1);
  } else if (!combined_motion_search(cpi, x, bsize, mi_row, mi_col,
                                     &frame_mv[NEWMV][ref_frame], rate_mv,
                                     best_rdc->rdcost, 0)) {
    return -1;
  }

  return 0;
}

static INLINE void find_predictors(
    AV1_COMP *cpi, MACROBLOCK *x, MV_REFERENCE_FRAME ref_frame,
    int_mv frame_mv[MB_MODE_COUNT][REF_FRAMES], int const_motion[REF_FRAMES],
    int *ref_frame_skip_mask, const int flag_list[4], TileDataEnc *tile_data,
    int mi_row, int mi_col, struct buf_2d yv12_mb[4][MAX_MB_PLANE],
    BLOCK_SIZE bsize, int force_skip_low_temp_var, int comp_pred_allowed) {
  AV1_COMMON *const cm = &cpi->common;
  MACROBLOCKD *const xd = &x->e_mbd;
  MB_MODE_INFO *const mbmi = xd->mi[0];
  MB_MODE_INFO_EXT *const mbmi_ext = x->mbmi_ext;
  const YV12_BUFFER_CONFIG *yv12 = get_ref_frame_yv12_buf(cm, ref_frame);
  const int num_planes = av1_num_planes(cm);
  (void)tile_data;
  (void)const_motion;
  (void)comp_pred_allowed;

  x->pred_mv_sad[ref_frame] = INT_MAX;
  frame_mv[NEWMV][ref_frame].as_int = INVALID_MV;
  // TODO(kyslov) this needs various further optimizations. to be continued..
  if ((cpi->ref_frame_flags & flag_list[ref_frame]) && (yv12 != NULL)) {
    const struct scale_factors *const sf =
        get_ref_scale_factors_const(cm, ref_frame);
    av1_setup_pred_block(xd, yv12_mb[ref_frame], yv12, mi_row, mi_col, sf, sf,
                         num_planes);
    av1_find_mv_refs(cm, xd, mbmi, ref_frame, mbmi_ext->ref_mv_count,
                     mbmi_ext->ref_mv_stack, mbmi_ext->weight, NULL,
                     mbmi_ext->global_mvs, mi_row, mi_col,
                     mbmi_ext->mode_context);
    av1_find_best_ref_mvs_from_stack(cm->allow_high_precision_mv, mbmi_ext,
                                     ref_frame, &frame_mv[NEARESTMV][ref_frame],
                                     &frame_mv[NEARMV][ref_frame], 0);
    // Early exit for golden frame if force_skip_low_temp_var is set.
    if (!av1_is_scaled(sf) && bsize >= BLOCK_8X8 &&
        !(force_skip_low_temp_var && ref_frame == GOLDEN_FRAME)) {
      av1_mv_pred(cpi, x, yv12_mb[ref_frame][0].buf, yv12->y_stride, ref_frame,
                  bsize);
    }
  } else {
    *ref_frame_skip_mask |= (1 << ref_frame);
  }
  av1_count_overlappable_neighbors(cm, xd, mi_row, mi_col);
  mbmi->num_proj_ref = 1;
}

static void estimate_single_ref_frame_costs(const AV1_COMMON *cm,
                                            const MACROBLOCKD *xd,
                                            const MACROBLOCK *x, int segment_id,
                                            unsigned int *ref_costs_single) {
  int seg_ref_active =
      segfeature_active(&cm->seg, segment_id, SEG_LVL_REF_FRAME);
  if (seg_ref_active) {
    memset(ref_costs_single, 0, REF_FRAMES * sizeof(*ref_costs_single));
  } else {
    int intra_inter_ctx = av1_get_intra_inter_context(xd);
    ref_costs_single[INTRA_FRAME] = x->intra_inter_cost[intra_inter_ctx][0];
    unsigned int base_cost = x->intra_inter_cost[intra_inter_ctx][1];

    for (int i = LAST_FRAME; i <= ALTREF_FRAME; ++i)
      ref_costs_single[i] = base_cost;

    const int ctx_p1 = av1_get_pred_context_single_ref_p1(xd);
    const int ctx_p2 = av1_get_pred_context_single_ref_p2(xd);
    const int ctx_p3 = av1_get_pred_context_single_ref_p3(xd);
    const int ctx_p4 = av1_get_pred_context_single_ref_p4(xd);
    const int ctx_p5 = av1_get_pred_context_single_ref_p5(xd);
    const int ctx_p6 = av1_get_pred_context_single_ref_p6(xd);

    // Determine cost of a single ref frame, where frame types are represented
    // by a tree:
    // Level 0: add cost whether this ref is a forward or backward ref
    ref_costs_single[LAST_FRAME] += x->single_ref_cost[ctx_p1][0][0];
    ref_costs_single[LAST2_FRAME] += x->single_ref_cost[ctx_p1][0][0];
    ref_costs_single[LAST3_FRAME] += x->single_ref_cost[ctx_p1][0][0];
    ref_costs_single[GOLDEN_FRAME] += x->single_ref_cost[ctx_p1][0][0];
    ref_costs_single[BWDREF_FRAME] += x->single_ref_cost[ctx_p1][0][1];
    ref_costs_single[ALTREF2_FRAME] += x->single_ref_cost[ctx_p1][0][1];
    ref_costs_single[ALTREF_FRAME] += x->single_ref_cost[ctx_p1][0][1];

    // Level 1: if this ref is forward ref,
    // add cost whether it is last/last2 or last3/golden
    ref_costs_single[LAST_FRAME] += x->single_ref_cost[ctx_p3][2][0];
    ref_costs_single[LAST2_FRAME] += x->single_ref_cost[ctx_p3][2][0];
    ref_costs_single[LAST3_FRAME] += x->single_ref_cost[ctx_p3][2][1];
    ref_costs_single[GOLDEN_FRAME] += x->single_ref_cost[ctx_p3][2][1];

    // Level 1: if this ref is backward ref
    // then add cost whether this ref is altref or backward ref
    ref_costs_single[BWDREF_FRAME] += x->single_ref_cost[ctx_p2][1][0];
    ref_costs_single[ALTREF2_FRAME] += x->single_ref_cost[ctx_p2][1][0];
    ref_costs_single[ALTREF_FRAME] += x->single_ref_cost[ctx_p2][1][1];

    // Level 2: further add cost whether this ref is last or last2
    ref_costs_single[LAST_FRAME] += x->single_ref_cost[ctx_p4][3][0];
    ref_costs_single[LAST2_FRAME] += x->single_ref_cost[ctx_p4][3][1];

    // Level 2: last3 or golden
    ref_costs_single[LAST3_FRAME] += x->single_ref_cost[ctx_p5][4][0];
    ref_costs_single[GOLDEN_FRAME] += x->single_ref_cost[ctx_p5][4][1];

    // Level 2: bwdref or altref2
    ref_costs_single[BWDREF_FRAME] += x->single_ref_cost[ctx_p6][5][0];
    ref_costs_single[ALTREF2_FRAME] += x->single_ref_cost[ctx_p6][5][1];
  }
}

static void estimate_comp_ref_frame_costs(
    const AV1_COMMON *cm, const MACROBLOCKD *xd, const MACROBLOCK *x,
    int segment_id, unsigned int (*ref_costs_comp)[REF_FRAMES]) {
  if (segfeature_active(&cm->seg, segment_id, SEG_LVL_REF_FRAME)) {
    for (int ref_frame = 0; ref_frame < REF_FRAMES; ++ref_frame)
      memset(ref_costs_comp[ref_frame], 0,
             REF_FRAMES * sizeof((*ref_costs_comp)[0]));
  } else {
    int intra_inter_ctx = av1_get_intra_inter_context(xd);
    unsigned int base_cost = x->intra_inter_cost[intra_inter_ctx][1];

    if (cm->current_frame.reference_mode != SINGLE_REFERENCE) {
      // Similar to single ref, determine cost of compound ref frames.
      // cost_compound_refs = cost_first_ref + cost_second_ref
      const int bwdref_comp_ctx_p = av1_get_pred_context_comp_bwdref_p(xd);
      const int bwdref_comp_ctx_p1 = av1_get_pred_context_comp_bwdref_p1(xd);
      const int ref_comp_ctx_p = av1_get_pred_context_comp_ref_p(xd);
      const int ref_comp_ctx_p1 = av1_get_pred_context_comp_ref_p1(xd);
      const int ref_comp_ctx_p2 = av1_get_pred_context_comp_ref_p2(xd);

      const int comp_ref_type_ctx = av1_get_comp_reference_type_context(xd);
      unsigned int ref_bicomp_costs[REF_FRAMES] = { 0 };

      ref_bicomp_costs[LAST_FRAME] = ref_bicomp_costs[LAST2_FRAME] =
          ref_bicomp_costs[LAST3_FRAME] = ref_bicomp_costs[GOLDEN_FRAME] =
              base_cost + x->comp_ref_type_cost[comp_ref_type_ctx][1];
      ref_bicomp_costs[BWDREF_FRAME] = ref_bicomp_costs[ALTREF2_FRAME] = 0;
      ref_bicomp_costs[ALTREF_FRAME] = 0;

      // cost of first ref frame
      ref_bicomp_costs[LAST_FRAME] += x->comp_ref_cost[ref_comp_ctx_p][0][0];
      ref_bicomp_costs[LAST2_FRAME] += x->comp_ref_cost[ref_comp_ctx_p][0][0];
      ref_bicomp_costs[LAST3_FRAME] += x->comp_ref_cost[ref_comp_ctx_p][0][1];
      ref_bicomp_costs[GOLDEN_FRAME] += x->comp_ref_cost[ref_comp_ctx_p][0][1];

      ref_bicomp_costs[LAST_FRAME] += x->comp_ref_cost[ref_comp_ctx_p1][1][0];
      ref_bicomp_costs[LAST2_FRAME] += x->comp_ref_cost[ref_comp_ctx_p1][1][1];

      ref_bicomp_costs[LAST3_FRAME] += x->comp_ref_cost[ref_comp_ctx_p2][2][0];
      ref_bicomp_costs[GOLDEN_FRAME] += x->comp_ref_cost[ref_comp_ctx_p2][2][1];

      // cost of second ref frame
      ref_bicomp_costs[BWDREF_FRAME] +=
          x->comp_bwdref_cost[bwdref_comp_ctx_p][0][0];
      ref_bicomp_costs[ALTREF2_FRAME] +=
          x->comp_bwdref_cost[bwdref_comp_ctx_p][0][0];
      ref_bicomp_costs[ALTREF_FRAME] +=
          x->comp_bwdref_cost[bwdref_comp_ctx_p][0][1];

      ref_bicomp_costs[BWDREF_FRAME] +=
          x->comp_bwdref_cost[bwdref_comp_ctx_p1][1][0];
      ref_bicomp_costs[ALTREF2_FRAME] +=
          x->comp_bwdref_cost[bwdref_comp_ctx_p1][1][1];

      // cost: if one ref frame is forward ref, the other ref is backward ref
      for (int ref0 = LAST_FRAME; ref0 <= GOLDEN_FRAME; ++ref0) {
        for (int ref1 = BWDREF_FRAME; ref1 <= ALTREF_FRAME; ++ref1) {
          ref_costs_comp[ref0][ref1] =
              ref_bicomp_costs[ref0] + ref_bicomp_costs[ref1];
        }
      }

      // cost: if both ref frames are the same side.
      const int uni_comp_ref_ctx_p = av1_get_pred_context_uni_comp_ref_p(xd);
      const int uni_comp_ref_ctx_p1 = av1_get_pred_context_uni_comp_ref_p1(xd);
      const int uni_comp_ref_ctx_p2 = av1_get_pred_context_uni_comp_ref_p2(xd);
      ref_costs_comp[LAST_FRAME][LAST2_FRAME] =
          base_cost + x->comp_ref_type_cost[comp_ref_type_ctx][0] +
          x->uni_comp_ref_cost[uni_comp_ref_ctx_p][0][0] +
          x->uni_comp_ref_cost[uni_comp_ref_ctx_p1][1][0];
      ref_costs_comp[LAST_FRAME][LAST3_FRAME] =
          base_cost + x->comp_ref_type_cost[comp_ref_type_ctx][0] +
          x->uni_comp_ref_cost[uni_comp_ref_ctx_p][0][0] +
          x->uni_comp_ref_cost[uni_comp_ref_ctx_p1][1][1] +
          x->uni_comp_ref_cost[uni_comp_ref_ctx_p2][2][0];
      ref_costs_comp[LAST_FRAME][GOLDEN_FRAME] =
          base_cost + x->comp_ref_type_cost[comp_ref_type_ctx][0] +
          x->uni_comp_ref_cost[uni_comp_ref_ctx_p][0][0] +
          x->uni_comp_ref_cost[uni_comp_ref_ctx_p1][1][1] +
          x->uni_comp_ref_cost[uni_comp_ref_ctx_p2][2][1];
      ref_costs_comp[BWDREF_FRAME][ALTREF_FRAME] =
          base_cost + x->comp_ref_type_cost[comp_ref_type_ctx][0] +
          x->uni_comp_ref_cost[uni_comp_ref_ctx_p][0][1];
    } else {
      for (int ref0 = LAST_FRAME; ref0 <= GOLDEN_FRAME; ++ref0) {
        for (int ref1 = BWDREF_FRAME; ref1 <= ALTREF_FRAME; ++ref1)
          ref_costs_comp[ref0][ref1] = 512;
      }
      ref_costs_comp[LAST_FRAME][LAST2_FRAME] = 512;
      ref_costs_comp[LAST_FRAME][LAST3_FRAME] = 512;
      ref_costs_comp[LAST_FRAME][GOLDEN_FRAME] = 512;
      ref_costs_comp[BWDREF_FRAME][ALTREF_FRAME] = 512;
    }
  }
}

#if _TMP_USE_CURVFIT_
static void model_rd_with_curvfit(const AV1_COMP *const cpi,
                                  const MACROBLOCK *const x,
                                  BLOCK_SIZE plane_bsize, int plane,
                                  int64_t sse, int num_samples, int *rate,
                                  int64_t *dist) {
  (void)cpi;
  (void)plane_bsize;
  const MACROBLOCKD *const xd = &x->e_mbd;
  const struct macroblock_plane *const p = &x->plane[plane];
  const int dequant_shift = (is_cur_buf_hbd(xd)) ? xd->bd - 5 : 3;
  const int qstep = AOMMAX(p->dequant_QTX[1] >> dequant_shift, 1);

  if (sse == 0) {
    if (rate) *rate = 0;
    if (dist) *dist = 0;
    return;
  }
  aom_clear_system_state();
  const double sse_norm = (double)sse / num_samples;
  const double qstepsqr = (double)qstep * qstep;
  const double xqr = log2(sse_norm / qstepsqr);

  double rate_f, dist_by_sse_norm_f;
  av1_model_rd_curvfit(plane_bsize, sse_norm, xqr, &rate_f,
                       &dist_by_sse_norm_f);

  const double dist_f = dist_by_sse_norm_f * sse_norm;
  int rate_i = (int)(AOMMAX(0.0, rate_f * num_samples) + 0.5);
  int64_t dist_i = (int64_t)(AOMMAX(0.0, dist_f * num_samples) + 0.5);
  aom_clear_system_state();

  // Check if skip is better
  if (rate_i == 0) {
    dist_i = sse << 4;
  } else if (RDCOST(x->rdmult, rate_i, dist_i) >=
             RDCOST(x->rdmult, 0, sse << 4)) {
    rate_i = 0;
    dist_i = sse << 4;
  }

  if (rate) *rate = rate_i;
  if (dist) *dist = dist_i;
}
#endif

static TX_SIZE calculate_tx_size(const AV1_COMP *const cpi, BLOCK_SIZE bsize,
                                 MACROBLOCKD *const xd, unsigned int var,
                                 unsigned int sse) {
  TX_SIZE tx_size;
  if (cpi->common.tx_mode == TX_MODE_SELECT) {
    if (sse > (var << 2))
      tx_size = AOMMIN(max_txsize_lookup[bsize],
                       tx_mode_to_biggest_tx_size[cpi->common.tx_mode]);
    else
      tx_size = TX_8X8;

    if (cpi->oxcf.aq_mode == CYCLIC_REFRESH_AQ &&
        cyclic_refresh_segment_id_boosted(xd->mi[0]->segment_id))
      tx_size = TX_8X8;
    else if (tx_size > TX_16X16)
      tx_size = TX_16X16;
  } else {
    tx_size = AOMMIN(max_txsize_lookup[bsize],
                     tx_mode_to_biggest_tx_size[cpi->common.tx_mode]);
  }
  if (bsize > BLOCK_32X32) tx_size = TX_16X16;
  return AOMMIN(tx_size, TX_16X16);
}

static const uint8_t b_width_log2_lookup[BLOCK_SIZES] = { 0, 0, 1, 1, 1, 2,
                                                          2, 2, 3, 3, 3, 4,
                                                          4, 4, 5, 5 };
static const uint8_t b_height_log2_lookup[BLOCK_SIZES] = { 0, 1, 0, 1, 2, 1,
                                                           2, 3, 2, 3, 4, 3,
                                                           4, 5, 4, 5 };

static void block_variance(const uint8_t *src, int src_stride,
                           const uint8_t *ref, int ref_stride, int w, int h,
                           unsigned int *sse, int *sum, int block_size,
                           uint32_t *sse8x8, int *sum8x8, uint32_t *var8x8) {
  int i, j, k = 0;

  *sse = 0;
  *sum = 0;

  for (i = 0; i < h; i += block_size) {
    for (j = 0; j < w; j += block_size) {
      aom_get8x8var(src + src_stride * i + j, src_stride,
                    ref + ref_stride * i + j, ref_stride, &sse8x8[k],
                    &sum8x8[k]);
      *sse += sse8x8[k];
      *sum += sum8x8[k];
      var8x8[k] = sse8x8[k] - (uint32_t)(((int64_t)sum8x8[k] * sum8x8[k]) >> 6);
      k++;
    }
  }
}

static void calculate_variance(int bw, int bh, TX_SIZE tx_size,
                               unsigned int *sse_i, int *sum_i,
                               unsigned int *var_o, unsigned int *sse_o,
                               int *sum_o) {
  const BLOCK_SIZE unit_size = txsize_to_bsize[tx_size];
  const int nw = 1 << (bw - b_width_log2_lookup[unit_size]);
  const int nh = 1 << (bh - b_height_log2_lookup[unit_size]);
  int i, j, k = 0;

  for (i = 0; i < nh; i += 2) {
    for (j = 0; j < nw; j += 2) {
      sse_o[k] = sse_i[i * nw + j] + sse_i[i * nw + j + 1] +
                 sse_i[(i + 1) * nw + j] + sse_i[(i + 1) * nw + j + 1];
      sum_o[k] = sum_i[i * nw + j] + sum_i[i * nw + j + 1] +
                 sum_i[(i + 1) * nw + j] + sum_i[(i + 1) * nw + j + 1];
      var_o[k] = sse_o[k] - (uint32_t)(((int64_t)sum_o[k] * sum_o[k]) >>
                                       (b_width_log2_lookup[unit_size] +
                                        b_height_log2_lookup[unit_size] + 6));
      k++;
    }
  }
}

// Adjust the ac_thr according to speed, width, height and normalized sum
static int ac_thr_factor(const int speed, const int width, const int height,
                         const int norm_sum) {
  if (speed >= 8 && norm_sum < 5) {
    if (width <= 640 && height <= 480)
      return 4;
    else
      return 2;
  }
  return 1;
}

static void model_skip_for_sb_y_large(AV1_COMP *cpi, BLOCK_SIZE bsize,
                                      MACROBLOCK *x, MACROBLOCKD *xd,
                                      unsigned int *var_y, unsigned int *sse_y,
                                      int *early_term) {
  // Note our transform coeffs are 8 times an orthogonal transform.
  // Hence quantizer step is also 8 times. To get effective quantizer
  // we need to divide by 8 before sending to modeling function.
  unsigned int sse;
  struct macroblock_plane *const p = &x->plane[0];
  struct macroblockd_plane *const pd = &xd->plane[0];
  const uint32_t dc_quant = p->dequant_QTX[0];
  const uint32_t ac_quant = p->dequant_QTX[1];
  const int64_t dc_thr = dc_quant * dc_quant >> 6;
  int64_t ac_thr = ac_quant * ac_quant >> 6;
  unsigned int var;
  int sum;

  const int bw = b_width_log2_lookup[bsize];
  const int bh = b_height_log2_lookup[bsize];
  const int num8x8 = 1 << (bw + bh - 2);
  unsigned int sse8x8[256] = { 0 };
  int sum8x8[256] = { 0 };
  unsigned int var8x8[256] = { 0 };
  TX_SIZE tx_size;
  int k;
  // Calculate variance for whole partition, and also save 8x8 blocks' variance
  // to be used in following transform skipping test.
  block_variance(p->src.buf, p->src.stride, pd->dst.buf, pd->dst.stride,
                 4 << bw, 4 << bh, &sse, &sum, 8, sse8x8, sum8x8, var8x8);
  var = sse - (unsigned int)(((int64_t)sum * sum) >> (bw + bh + 4));

  *var_y = var;
  *sse_y = sse;

  ac_thr *= ac_thr_factor(cpi->oxcf.speed, cpi->common.width,
                          cpi->common.height, abs(sum) >> (bw + bh));

  tx_size = calculate_tx_size(cpi, bsize, xd, var, sse);
  // The code below for setting skip flag assumes tranform size of at least 8x8,
  // so force this lower limit on transform.
  if (tx_size < TX_8X8) tx_size = TX_8X8;
  xd->mi[0]->tx_size = tx_size;

  // Evaluate if the partition block is a skippable block in Y plane.
  {
    unsigned int sse16x16[64] = { 0 };
    int sum16x16[64] = { 0 };
    unsigned int var16x16[64] = { 0 };
    const int num16x16 = num8x8 >> 2;

    unsigned int sse32x32[16] = { 0 };
    int sum32x32[16] = { 0 };
    unsigned int var32x32[16] = { 0 };
    const int num32x32 = num8x8 >> 4;

    int ac_test = 1;
    int dc_test = 1;
    const int num = (tx_size == TX_8X8)
                        ? num8x8
                        : ((tx_size == TX_16X16) ? num16x16 : num32x32);
    const unsigned int *sse_tx =
        (tx_size == TX_8X8) ? sse8x8
                            : ((tx_size == TX_16X16) ? sse16x16 : sse32x32);
    const unsigned int *var_tx =
        (tx_size == TX_8X8) ? var8x8
                            : ((tx_size == TX_16X16) ? var16x16 : var32x32);

    // Calculate variance if tx_size > TX_8X8
    if (tx_size >= TX_16X16)
      calculate_variance(bw, bh, TX_8X8, sse8x8, sum8x8, var16x16, sse16x16,
                         sum16x16);
    if (tx_size == TX_32X32)
      calculate_variance(bw, bh, TX_16X16, sse16x16, sum16x16, var32x32,
                         sse32x32, sum32x32);

    // Skipping test
    *early_term = 0;
    for (k = 0; k < num; k++)
      // Check if all ac coefficients can be quantized to zero.
      if (!(var_tx[k] < ac_thr || var == 0)) {
        ac_test = 0;
        break;
      }

    for (k = 0; k < num; k++)
      // Check if dc coefficient can be quantized to zero.
      if (!(sse_tx[k] - var_tx[k] < dc_thr || sse == var)) {
        dc_test = 0;
        break;
      }

    if (ac_test && dc_test) {
      *early_term = 1;
    }
  }
}

static void model_rd_for_sb_y(const AV1_COMP *const cpi, BLOCK_SIZE bsize,
                              MACROBLOCK *x, MACROBLOCKD *xd, int *out_rate_sum,
                              int64_t *out_dist_sum, int *skip_txfm_sb,
                              int64_t *skip_sse_sb, unsigned int *var_y,
                              unsigned int *sse_y) {
  // Note our transform coeffs are 8 times an orthogonal transform.
  // Hence quantizer step is also 8 times. To get effective quantizer
  // we need to divide by 8 before sending to modeling function.
  const int ref = xd->mi[0]->ref_frame[0];

  assert(bsize < BLOCK_SIZES_ALL);

  struct macroblock_plane *const p = &x->plane[0];
  struct macroblockd_plane *const pd = &xd->plane[0];
  unsigned int sse;
  int rate;
  int64_t dist;

  unsigned int var = cpi->fn_ptr[bsize].vf(p->src.buf, p->src.stride,
                                           pd->dst.buf, pd->dst.stride, &sse);
  xd->mi[0]->tx_size = calculate_tx_size(cpi, bsize, xd, var, sse);

#if _TMP_USE_CURVFIT_
  model_rd_with_curvfit(cpi, x, plane_bsize, plane, sse, bw * bh, &rate, &dist);
#else
  (void)cpi;
  rate = INT_MAX;  // this will be overwritten later with block_yrd
  dist = INT_MAX;
#endif
  *var_y = var;
  *sse_y = sse;
  x->pred_sse[ref] = (unsigned int)AOMMIN(sse, UINT_MAX);

  assert(rate >= 0);

  if (skip_txfm_sb) *skip_txfm_sb = rate == 0;
  if (skip_sse_sb) *skip_sse_sb = sse << 4;
  rate = AOMMIN(rate, INT_MAX);
  *out_rate_sum = (int)rate;
  *out_dist_sum = dist;
}

static void block_yrd(AV1_COMP *cpi, MACROBLOCK *x, int mi_row, int mi_col,
                      RD_STATS *this_rdc, int *skippable, int64_t *sse,
                      BLOCK_SIZE bsize, TX_SIZE tx_size) {
  MACROBLOCKD *xd = &x->e_mbd;
  const struct macroblockd_plane *pd = &xd->plane[0];
  struct macroblock_plane *const p = &x->plane[0];
  const int num_4x4_w = mi_size_wide[bsize];
  const int num_4x4_h = mi_size_high[bsize];
  const int step = 1 << (tx_size << 1);
  const int block_step = (1 << tx_size);
  int block = 0;
  const int max_blocks_wide =
      num_4x4_w + (xd->mb_to_right_edge >= 0 ? 0 : xd->mb_to_right_edge >> 5);
  const int max_blocks_high =
      num_4x4_h + (xd->mb_to_bottom_edge >= 0 ? 0 : xd->mb_to_bottom_edge >> 5);
  int eob_cost = 0;
  const int bw = 4 * num_4x4_w;
  const int bh = 4 * num_4x4_h;

  assert(tx_size > 0 && tx_size <= 4);

  (void)mi_row;
  (void)mi_col;
  (void)cpi;

  aom_subtract_block(bh, bw, p->src_diff, bw, p->src.buf, p->src.stride,
                     pd->dst.buf, pd->dst.stride);
  *skippable = 1;
  // Keep track of the row and column of the blocks we use so that we know
  // if we are in the unrestricted motion border.
  for (int r = 0; r < max_blocks_high; r += block_step) {
    for (int c = 0; c < num_4x4_w; c += block_step) {
      if (c < max_blocks_wide) {
        const SCAN_ORDER *const scan_order = &av1_default_scan_orders[tx_size];
        tran_low_t *const coeff = BLOCK_OFFSET(p->coeff, block);
        tran_low_t *const qcoeff = BLOCK_OFFSET(p->qcoeff, block);
        tran_low_t *const dqcoeff = BLOCK_OFFSET(pd->dqcoeff, block);
        uint16_t *const eob = &p->eobs[block];
        const int diff_stride = bw;
        const int16_t *src_diff;
        src_diff = &p->src_diff[(r * diff_stride + c) << 2];

        switch (tx_size) {
          case TX_64X64:
            assert(0);  // Not implemented
            break;
          case TX_32X32:
            aom_hadamard_32x32(src_diff, diff_stride, coeff);
            av1_quantize_fp(coeff, 32 * 32, p->zbin_QTX, p->round_fp_QTX,
                            p->quant_fp_QTX, p->quant_shift_QTX, qcoeff,
                            dqcoeff, p->dequant_QTX, eob, scan_order->scan,
                            scan_order->iscan);
            break;
          case TX_16X16:
            aom_hadamard_16x16(src_diff, diff_stride, coeff);
            av1_quantize_fp(coeff, 16 * 16, p->zbin_QTX, p->round_fp_QTX,
                            p->quant_fp_QTX, p->quant_shift_QTX, qcoeff,
                            dqcoeff, p->dequant_QTX, eob, scan_order->scan,
                            scan_order->iscan);
            break;
          case TX_8X8:
            aom_hadamard_8x8(src_diff, diff_stride, coeff);
            av1_quantize_fp(coeff, 8 * 8, p->zbin_QTX, p->round_fp_QTX,
                            p->quant_fp_QTX, p->quant_shift_QTX, qcoeff,
                            dqcoeff, p->dequant_QTX, eob, scan_order->scan,
                            scan_order->iscan);
            break;
          default: assert(0); break;
        }
        *skippable &= (*eob == 0);
        eob_cost += 1;
      }
      block += step;
    }
  }
  this_rdc->skip = *skippable;
  this_rdc->rate = 0;
  if (*sse < INT64_MAX) {
    *sse = (*sse << 6) >> 2;
    if (*skippable) {
      this_rdc->dist = *sse;
      return;
    }
  }

  block = 0;
  this_rdc->dist = 0;
  for (int r = 0; r < max_blocks_high; r += block_step) {
    for (int c = 0; c < num_4x4_w; c += block_step) {
      if (c < max_blocks_wide) {
        int64_t dummy;
        tran_low_t *const coeff = BLOCK_OFFSET(p->coeff, block);
        tran_low_t *const qcoeff = BLOCK_OFFSET(p->qcoeff, block);
        tran_low_t *const dqcoeff = BLOCK_OFFSET(pd->dqcoeff, block);
        uint16_t *const eob = &p->eobs[block];

        if (*eob == 1)
          this_rdc->rate += (int)abs(qcoeff[0]);
        else if (*eob > 1)
          this_rdc->rate += aom_satd(qcoeff, step << 4);

        this_rdc->dist +=
            av1_block_error(coeff, dqcoeff, step << 4, &dummy) >> 2;
      }
      block += step;
    }
  }

  // If skippable is set, rate gets clobbered later.
  this_rdc->rate <<= (2 + AV1_PROB_COST_SHIFT);
  this_rdc->rate += (eob_cost << AV1_PROB_COST_SHIFT);
}

static INLINE void init_mbmi(MB_MODE_INFO *mbmi, PREDICTION_MODE pred_mode,
                             MV_REFERENCE_FRAME ref_frame0,
                             MV_REFERENCE_FRAME ref_frame1,
                             const AV1_COMMON *cm) {
  PALETTE_MODE_INFO *const pmi = &mbmi->palette_mode_info;
  mbmi->ref_mv_idx = 0;
  mbmi->mode = pred_mode;
  mbmi->uv_mode = UV_DC_PRED;
  mbmi->ref_frame[0] = ref_frame0;
  mbmi->ref_frame[1] = ref_frame1;
  pmi->palette_size[0] = 0;
  pmi->palette_size[1] = 0;
  mbmi->filter_intra_mode_info.use_filter_intra = 0;
  mbmi->mv[0].as_int = mbmi->mv[1].as_int = 0;
  mbmi->motion_mode = SIMPLE_TRANSLATION;
  mbmi->num_proj_ref = 1;
  mbmi->interintra_mode = 0;
  set_default_interp_filters(mbmi, cm->interp_filter);
}

static void store_coding_context(MACROBLOCK *x, PICK_MODE_CONTEXT *ctx,
                                 int mode_index) {
  MACROBLOCKD *const xd = &x->e_mbd;

  // Take a snapshot of the coding context so it can be
  // restored if we decide to encode this way
  ctx->rd_stats.skip = x->skip;
  memcpy(ctx->blk_skip, x->blk_skip, sizeof(x->blk_skip[0]) * ctx->num_4x4_blk);
  ctx->skippable = x->skip;
  ctx->best_mode_index = mode_index;
  ctx->mic = *xd->mi[0];
  ctx->mbmi_ext = *x->mbmi_ext;
  ctx->comp_pred_diff = 0;
  ctx->hybrid_pred_diff = 0;
  ctx->single_pred_diff = 0;
}

static int get_pred_buffer(PRED_BUFFER *p, int len) {
  for (int i = 0; i < len; i++) {
    if (!p[i].in_use) {
      p[i].in_use = 1;
      return i;
    }
  }
  return -1;
}

static void free_pred_buffer(PRED_BUFFER *p) {
  if (p != NULL) p->in_use = 0;
}

static int cost_mv_ref(const MACROBLOCK *const x, PREDICTION_MODE mode,
                       int16_t mode_context) {
  if (is_inter_compound_mode(mode)) {
    return x
        ->inter_compound_mode_cost[mode_context][INTER_COMPOUND_OFFSET(mode)];
  }

  int mode_cost = 0;
  int16_t mode_ctx = mode_context & NEWMV_CTX_MASK;

  assert(is_inter_mode(mode));

  if (mode == NEWMV) {
    mode_cost = x->newmv_mode_cost[mode_ctx][0];
    return mode_cost;
  } else {
    mode_cost = x->newmv_mode_cost[mode_ctx][1];
    mode_ctx = (mode_context >> GLOBALMV_OFFSET) & GLOBALMV_CTX_MASK;

    if (mode == GLOBALMV) {
      mode_cost += x->zeromv_mode_cost[mode_ctx][0];
      return mode_cost;
    } else {
      mode_cost += x->zeromv_mode_cost[mode_ctx][1];
      mode_ctx = (mode_context >> REFMV_OFFSET) & REFMV_CTX_MASK;
      mode_cost += x->refmv_mode_cost[mode_ctx][mode != NEARESTMV];
      return mode_cost;
    }
  }
}

static void newmv_diff_bias(MACROBLOCKD *xd, PREDICTION_MODE this_mode,
                            RD_STATS *this_rdc, BLOCK_SIZE bsize, int mv_row,
                            int mv_col, int is_last_frame) {
  // Bias against MVs associated with NEWMV mode that are very different from
  // top/left neighbors.
  if (this_mode == NEWMV) {
    int al_mv_average_row;
    int al_mv_average_col;
    int left_row, left_col;
    int row_diff, col_diff;
    int above_mv_valid = 0;
    int left_mv_valid = 0;
    int above_row = 0;
    int above_col = 0;

    if (xd->above_mbmi) {
      above_mv_valid = xd->above_mbmi->mv[0].as_int != INVALID_MV;
      above_row = xd->above_mbmi->mv[0].as_mv.row;
      above_col = xd->above_mbmi->mv[0].as_mv.col;
    }
    if (xd->left_mbmi) {
      left_mv_valid = xd->left_mbmi->mv[0].as_int != INVALID_MV;
      left_row = xd->left_mbmi->mv[0].as_mv.row;
      left_col = xd->left_mbmi->mv[0].as_mv.col;
    }
    if (above_mv_valid && left_mv_valid) {
      al_mv_average_row = (above_row + left_row + 1) >> 1;
      al_mv_average_col = (above_col + left_col + 1) >> 1;
    } else if (above_mv_valid) {
      al_mv_average_row = above_row;
      al_mv_average_col = above_col;
    } else if (left_mv_valid) {
      al_mv_average_row = left_row;
      al_mv_average_col = left_col;
    } else {
      al_mv_average_row = al_mv_average_col = 0;
    }
    row_diff = al_mv_average_row - mv_row;
    col_diff = al_mv_average_col - mv_col;
    if (row_diff > 48 || row_diff < -48 || col_diff > 48 || col_diff < -48) {
      if (bsize > BLOCK_32X32)
        this_rdc->rdcost = this_rdc->rdcost << 1;
      else
        this_rdc->rdcost = 5 * this_rdc->rdcost >> 2;
    }
  }
  if (bsize >= BLOCK_16X16 && is_last_frame && mv_row < 16 && mv_row > -16 &&
      mv_col < 16 && mv_col > -16)
    this_rdc->rdcost = 7 * (this_rdc->rdcost >> 3);
}

struct estimate_block_intra_args {
  AV1_COMP *cpi;
  MACROBLOCK *x;
  PREDICTION_MODE mode;
  int skippable;
  RD_STATS *rdc;
};

static void estimate_block_intra(int plane, int block, int row, int col,
                                 BLOCK_SIZE plane_bsize, TX_SIZE tx_size,
                                 void *arg) {
  struct estimate_block_intra_args *const args = arg;
  AV1_COMP *const cpi = args->cpi;
  AV1_COMMON *const cm = &cpi->common;
  MACROBLOCK *const x = args->x;
  MACROBLOCKD *const xd = &x->e_mbd;
  struct macroblock_plane *const p = &x->plane[plane];
  struct macroblockd_plane *const pd = &xd->plane[plane];
  uint8_t *const src_buf_base = p->src.buf;
  uint8_t *const dst_buf_base = pd->dst.buf;
  const int64_t src_stride = p->src.stride;
  const int64_t dst_stride = pd->dst.stride;
  RD_STATS this_rdc;

  p->src.buf = &src_buf_base[4 * (row * src_stride + col)];
  pd->dst.buf = &dst_buf_base[4 * (row * dst_stride + col)];

  const int stepr = tx_size_high_unit[tx_size];
  const int stepc = tx_size_wide_unit[tx_size];
  const int max_blocks_wide = max_block_wide(xd, block, 0);
  const int max_blocks_high = max_block_high(xd, block, 0);
  // Prediction.
  for (int trow = 0; trow < max_blocks_high; trow += stepr) {
    for (int tcol = 0; tcol < max_blocks_wide; tcol += stepc) {
      av1_predict_intra_block_facade(cm, xd, 0, tcol, trow, tx_size);
    }
  }

  assert(plane == 0);

  if (plane == 0) {
    int64_t this_sse = INT64_MAX;
    block_yrd(cpi, x, 0, 0, &this_rdc, &args->skippable, &this_sse, plane_bsize,
              AOMMIN(tx_size, TX_16X16));
  } else {
    return;
  }

  p->src.buf = src_buf_base;
  pd->dst.buf = dst_buf_base;
  args->rdc->rate += this_rdc.rate;
  args->rdc->dist += this_rdc.dist;
}

void av1_fast_nonrd_pick_inter_mode_sb(AV1_COMP *cpi, TileDataEnc *tile_data,
                                       MACROBLOCK *x, int mi_row, int mi_col,
                                       RD_STATS *rd_cost, BLOCK_SIZE bsize,
                                       PICK_MODE_CONTEXT *ctx,
                                       int64_t best_rd_so_far) {
  AV1_COMMON *const cm = &cpi->common;
  MACROBLOCKD *const xd = &x->e_mbd;
  MB_MODE_INFO *const mi = xd->mi[0];
  struct macroblockd_plane *const pd = &xd->plane[0];

  BEST_PICKMODE best_pickmode;
  int inter_mode_mask[BLOCK_SIZES];

  MV_REFERENCE_FRAME ref_frame;
  MV_REFERENCE_FRAME usable_ref_frame, second_ref_frame;
  int_mv frame_mv[MB_MODE_COUNT][REF_FRAMES];
  uint8_t mode_checked[MB_MODE_COUNT][REF_FRAMES];
  struct buf_2d yv12_mb[6][MAX_MB_PLANE];
  static const int flag_list[5] = { 0, AOM_LAST_FLAG, AOM_LAST2_FLAG,
                                    AOM_LAST3_FLAG, AOM_GOLD_FLAG };
  RD_STATS this_rdc, best_rdc;
  // var_y and sse_y are saved to be used in skipping checking
  unsigned int sse_y = UINT_MAX;
  unsigned int var_y = UINT_MAX;
  const int *const rd_threshes = cpi->rd.threshes[mi->segment_id][bsize];
  const int *const rd_thresh_freq_fact = tile_data->thresh_freq_fact[bsize];

  InterpFilter filter_ref;
  int const_motion[REF_FRAMES] = { 0 };
  int ref_frame_skip_mask = 0;
  int best_pred_sad = INT_MAX;
  int best_early_term = 0;
  unsigned int ref_costs_single[REF_FRAMES],
      ref_costs_comp[REF_FRAMES][REF_FRAMES];
  int use_golden_nonzeromv = 1;
  int force_skip_low_temp_var = 0;
  int skip_ref_find_pred[5] = { 0 };
  int64_t best_sse_sofar = INT64_MAX;
  int gf_temporal_ref = 0;
  const struct segmentation *const seg = &cm->seg;
  int comp_modes = 0;
  int num_inter_modes = RT_INTER_MODES;
  unsigned char segment_id = mi->segment_id;
  InterpFilter best_filter = EIGHTTAP_REGULAR;
  PRED_BUFFER tmp[4];
  DECLARE_ALIGNED(16, uint8_t, pred_buf[3 * 64 * 64]);
  PRED_BUFFER *this_mode_pred = NULL;
  const int reuse_inter_pred = cpi->sf.reuse_inter_pred_nonrd;
  const int bh = block_size_high[bsize];
  const int bw = block_size_wide[bsize];
  const int pixels_in_block = bh * bw;
  struct buf_2d orig_dst = pd->dst;

  const int intra_cost_penalty = av1_get_intra_cost_penalty(
      cm->base_qindex, cm->y_dc_delta_q, cm->seq_params.bit_depth);
  const int64_t inter_mode_thresh = RDCOST(x->rdmult, intra_cost_penalty, 0);
  const int perform_intra_pred = cpi->sf.check_intra_pred_nonrd;

  (void)best_rd_so_far;

  init_best_pickmode(&best_pickmode);

  for (int i = 0; i < BLOCK_SIZES; ++i) inter_mode_mask[i] = INTER_ALL;

  // TODO(kyslov) Move this to Speed Features
  inter_mode_mask[BLOCK_128X128] = INTER_NEAREST_NEAR;

  x->source_variance = UINT_MAX;

  struct scale_factors *const sf_last = get_ref_scale_factors(cm, LAST_FRAME);
  struct scale_factors *const sf_golden =
      get_ref_scale_factors(cm, GOLDEN_FRAME);
  gf_temporal_ref = 1;
  // For temporal long term prediction, check that the golden reference
  // is same scale as last reference, otherwise disable.
  if ((sf_last->x_scale_fp != sf_golden->x_scale_fp) ||
      (sf_last->y_scale_fp != sf_golden->y_scale_fp)) {
    gf_temporal_ref = 0;
  }

  av1_collect_neighbors_ref_counts(xd);
  av1_count_overlappable_neighbors(cm, xd, mi_row, mi_col);

  estimate_single_ref_frame_costs(cm, xd, x, segment_id, ref_costs_single);
  if (cpi->sf.use_comp_ref_nonrd)
    estimate_comp_ref_frame_costs(cm, xd, x, segment_id, ref_costs_comp);

  memset(&mode_checked[0][0], 0, MB_MODE_COUNT * REF_FRAMES);
  if (reuse_inter_pred) {
    for (int i = 0; i < 3; i++) {
      tmp[i].data = &pred_buf[pixels_in_block * i];
      tmp[i].stride = bw;
      tmp[i].in_use = 0;
    }
    tmp[3].data = pd->dst.buf;
    tmp[3].stride = pd->dst.stride;
    tmp[3].in_use = 0;
  }

  x->skip = 0;

  // Instead of using av1_get_pred_context_switchable_interp(xd) to assign
  // filter_ref, we use a less strict condition on assigning filter_ref.
  // This is to reduce the probabily of entering the flow of not assigning
  // filter_ref and then skip filter search.
  filter_ref = cm->interp_filter;

  // initialize mode decisions
  av1_invalid_rd_stats(&best_rdc);
  av1_invalid_rd_stats(&this_rdc);
  av1_invalid_rd_stats(rd_cost);
  mi->sb_type = bsize;
  mi->ref_frame[0] = NONE_FRAME;
  mi->ref_frame[1] = NONE_FRAME;

// TODO(kyslov) Refine logic of selecting REF FRAME SET.
// For now only LAST_FRAME is used
#if 0
  if (cpi->rc.frames_since_golden == 0 && gf_temporal_ref) {
    usable_ref_frame = LAST_FRAME;
  } else {
    usable_ref_frame = GOLDEN_FRAME;
  }

  force_skip_low_temp_var = get_force_skip_low_temp_var(&x->variance_low[0],
                                  mi_row, mi_col, bsize);
  // If force_skip_low_temp_var is set, and for short circuit mode = 1 and 3,
  // skip golden reference.
  if (force_skip_low_temp_var) {
    usable_ref_frame = LAST_FRAME;
  }

  if (!((cpi->ref_frame_flags & flag_list[GOLDEN_FRAME]) &&
        !force_skip_low_temp_var))
    use_golden_nonzeromv = 0;

  // If the segment reference frame feature is enabled and it's set to GOLDEN
  // reference, then make sure we don't skip checking GOLDEN, this is to
  // prevent possibility of not picking any mode.
  if (segfeature_active(seg, mi->segment_id, SEG_LVL_REF_FRAME) &&
      get_segdata(seg, mi->segment_id, SEG_LVL_REF_FRAME) == GOLDEN_FRAME) {
    usable_ref_frame = GOLDEN_FRAME;
    skip_ref_find_pred[GOLDEN_FRAME] = 0;
  }
#endif
  usable_ref_frame = LAST_FRAME;

  for (MV_REFERENCE_FRAME ref_frame_iter = LAST_FRAME;
       ref_frame_iter <= usable_ref_frame; ++ref_frame_iter) {
    // Skip find_predictor if the reference frame is not in the
    // ref_frame_flags (i.e., not used as a reference for this frame).
    skip_ref_find_pred[ref_frame_iter] =
        !(cpi->ref_frame_flags & flag_list[ref_frame_iter]);
    if (!skip_ref_find_pred[ref_frame_iter]) {
      find_predictors(cpi, x, ref_frame_iter, frame_mv, const_motion,
                      &ref_frame_skip_mask, flag_list, tile_data, mi_row,
                      mi_col, yv12_mb, bsize, force_skip_low_temp_var,
                      comp_modes > 0);
    }
  }
  const int large_block = bsize >= BLOCK_32X32;
  const int use_model_yrd_large =
      cpi->oxcf.rc_mode == AOM_CBR && large_block &&
      !cyclic_refresh_segment_id_boosted(xd->mi[0]->segment_id) &&
      cm->base_qindex;

  for (int idx = 0; idx < num_inter_modes; ++idx) {
    int rate_mv = 0;
    int mode_rd_thresh;
    int mode_index;
#if !_TMP_USE_CURVFIT_
    int64_t this_sse;
    int is_skippable;
#endif
    int this_early_term = 0;
    int skip_this_mv = 0;
    int comp_pred = 0;
    int force_mv_inter_layer = 0;
    PREDICTION_MODE this_mode;
    MB_MODE_INFO_EXT *const mbmi_ext = x->mbmi_ext;
    second_ref_frame = NONE_FRAME;

    this_mode = ref_mode_set[idx].pred_mode;
    ref_frame = ref_mode_set[idx].ref_frame;
    init_mbmi(mi, this_mode, ref_frame, NONE_FRAME, cm);

    mi->tx_size = AOMMIN(AOMMIN(max_txsize_lookup[bsize],
                                tx_mode_to_biggest_tx_size[cm->tx_mode]),
                         TX_16X16);
    memset(mi->inter_tx_size, mi->tx_size, sizeof(mi->inter_tx_size));
    memset(mi->txk_type, DCT_DCT, sizeof(mi->txk_type[0]) * TXK_TYPE_BUF_LEN);
    av1_zero(x->blk_skip);

    if (ref_frame > usable_ref_frame) continue;
    if (skip_ref_find_pred[ref_frame]) continue;

    // If the segment reference frame feature is enabled then do nothing if the
    // current ref frame is not allowed.
    if (segfeature_active(seg, mi->segment_id, SEG_LVL_REF_FRAME) &&
        get_segdata(seg, mi->segment_id, SEG_LVL_REF_FRAME) != (int)ref_frame)
      continue;

    if (!(cpi->ref_frame_flags & flag_list[ref_frame])) continue;

    if (!(inter_mode_mask[bsize] & (1 << this_mode))) continue;

    if (const_motion[ref_frame] && this_mode == NEARMV) continue;

    // Skip non-zeromv mode search for golden frame if force_skip_low_temp_var
    // is set. If nearestmv for golden frame is 0, zeromv mode will be skipped
    // later.
    if (!force_mv_inter_layer && force_skip_low_temp_var &&
        ref_frame == GOLDEN_FRAME &&
        frame_mv[this_mode][ref_frame].as_int != 0) {
      continue;
    }

// TODO(kyslov) Refine logic of pruning reference .
// For now only LAST_FRAME is used
#if 0
        if (x->content_state_sb != kVeryHighSad &&
            (cpi->sf.short_circuit_low_temp_var >= 2 ||
             (cpi->sf.short_circuit_low_temp_var == 1 && bsize == BLOCK_64X64))
       && force_skip_low_temp_var && ref_frame == LAST_FRAME && this_mode ==
       NEWMV) { continue;
        }

        // Disable this drop out case if the ref frame segment level feature is
        // enabled for this segment. This is to prevent the possibility that we
       end
        // up unable to pick any mode.
        if (!segfeature_active(seg, mi->segment_id, SEG_LVL_REF_FRAME)) {
          if (sf->reference_masking &&
              !(frame_mv[this_mode][ref_frame].as_int == 0 &&
                ref_frame == LAST_FRAME)) {
            if (usable_ref_frame < ALTREF_FRAME) {
              if (!force_skip_low_temp_var && usable_ref_frame > LAST_FRAME) {
                i = (ref_frame == LAST_FRAME) ? GOLDEN_FRAME : LAST_FRAME;
                if ((cpi->ref_frame_flags & flag_list[i]))
                  if (x->pred_mv_sad[ref_frame] > (x->pred_mv_sad[i] << 1))
                    ref_frame_skip_mask |= (1 << ref_frame);
              }
            } else if (!cpi->rc.is_src_frame_alt_ref &&
                       !(frame_mv[this_mode][ref_frame].as_int == 0 &&
                         ref_frame == ALTREF_FRAME)) {
              int ref1 = (ref_frame == GOLDEN_FRAME) ? LAST_FRAME :
       GOLDEN_FRAME; int ref2 = (ref_frame == ALTREF_FRAME) ? LAST_FRAME :
       ALTREF_FRAME; if (((cpi->ref_frame_flags & flag_list[ref1]) &&
                   (x->pred_mv_sad[ref_frame] > (x->pred_mv_sad[ref1] << 1))) ||
                  ((cpi->ref_frame_flags & flag_list[ref2]) &&
                   (x->pred_mv_sad[ref_frame] > (x->pred_mv_sad[ref2] << 1))))
                ref_frame_skip_mask |= (1 << ref_frame);
            }
          }
          if (ref_frame_skip_mask & (1 << ref_frame)) continue;
        }
#endif

    // Select prediction reference frames.
    for (int i = 0; i < MAX_MB_PLANE; i++) {
      xd->plane[i].pre[0] = yv12_mb[ref_frame][i];
    }

    mi->ref_frame[0] = ref_frame;
    mi->ref_frame[1] = second_ref_frame;
    set_ref_ptrs(cm, xd, ref_frame, second_ref_frame);

    mode_index = mode_idx[ref_frame][INTER_OFFSET(this_mode)];
    mode_rd_thresh = best_pickmode.best_mode_skip_txfm
                         ? rd_threshes[mode_index] << 1
                         : rd_threshes[mode_index];

    // Increase mode_rd_thresh value for GOLDEN_FRAME for improved encoding
    // speed
    if (ref_frame == GOLDEN_FRAME && cpi->rc.frames_since_golden > 4)
      mode_rd_thresh = mode_rd_thresh << 3;

    if (rd_less_than_thresh(best_rdc.rdcost, mode_rd_thresh,
                            rd_thresh_freq_fact[mode_index]))
      if (frame_mv[this_mode][ref_frame].as_int != 0) continue;

    if (this_mode == NEWMV && !force_mv_inter_layer) {
      if (search_new_mv(cpi, x, frame_mv, ref_frame, gf_temporal_ref, bsize,
                        mi_row, mi_col, best_pred_sad, &rate_mv, best_sse_sofar,
                        &best_rdc))
        continue;
    }

    for (PREDICTION_MODE inter_mv_mode = NEARESTMV; inter_mv_mode <= NEWMV;
         inter_mv_mode++) {
      if (inter_mv_mode == this_mode || comp_pred) continue;
      if (mode_checked[inter_mv_mode][ref_frame] &&
          frame_mv[this_mode][ref_frame].as_int ==
              frame_mv[inter_mv_mode][ref_frame].as_int &&
          frame_mv[inter_mv_mode][ref_frame].as_int == 0) {
        skip_this_mv = 1;
        break;
      }
    }

    if (skip_this_mv) continue;

    // If use_golden_nonzeromv is false, NEWMV mode is skipped for golden, no
    // need to compute best_pred_sad which is only used to skip golden NEWMV.
    if (use_golden_nonzeromv && this_mode == NEWMV && ref_frame == LAST_FRAME &&
        frame_mv[NEWMV][LAST_FRAME].as_int != INVALID_MV) {
      const int pre_stride = xd->plane[0].pre[0].stride;
      const uint8_t *const pre_buf =
          xd->plane[0].pre[0].buf +
          (frame_mv[NEWMV][LAST_FRAME].as_mv.row >> 3) * pre_stride +
          (frame_mv[NEWMV][LAST_FRAME].as_mv.col >> 3);
      best_pred_sad = cpi->fn_ptr[bsize].sdf(
          x->plane[0].src.buf, x->plane[0].src.stride, pre_buf, pre_stride);
      x->pred_mv_sad[LAST_FRAME] = best_pred_sad;
    }

    if (this_mode != NEARESTMV && !comp_pred &&
        frame_mv[this_mode][ref_frame].as_int ==
            frame_mv[NEARESTMV][ref_frame].as_int)
      continue;

    mi->mode = this_mode;
    mi->mv[0].as_int = frame_mv[this_mode][ref_frame].as_int;
    mi->mv[1].as_int = 0;
    if (reuse_inter_pred) {
      if (!this_mode_pred) {
        this_mode_pred = &tmp[3];
      } else {
        this_mode_pred = &tmp[get_pred_buffer(tmp, 3)];
        pd->dst.buf = this_mode_pred->data;
        pd->dst.stride = bw;
      }
    }

    // TODO(kyslov) bring back filter search
    mi->interp_filters = (filter_ref == SWITCHABLE)
                             ? av1_broadcast_interp_filter(EIGHTTAP_REGULAR)
                             : av1_broadcast_interp_filter(filter_ref);
    av1_enc_build_inter_predictor(cm, xd, mi_row, mi_col, NULL, bsize,
                                  AOM_PLANE_Y, AOM_PLANE_Y);
#if !_TMP_USE_CURVFIT_
    if (use_model_yrd_large) {
      model_skip_for_sb_y_large(cpi, bsize, x, xd, &var_y, &sse_y,
                                &this_early_term);
    } else {
#endif
      model_rd_for_sb_y(cpi, bsize, x, xd, &this_rdc.rate, &this_rdc.dist,
                        &this_rdc.skip, NULL, &var_y, &sse_y);
#if !_TMP_USE_CURVFIT_
    }
#endif

    if (sse_y < best_sse_sofar) best_sse_sofar = sse_y;

    const int skip_ctx = av1_get_skip_context(xd);
    const int skip_cost = x->skip_cost[skip_ctx][1];
    const int no_skip_cost = x->skip_cost[skip_ctx][0];
    if (!this_early_term) {
#if !_TMP_USE_CURVFIT_
      this_sse = (int64_t)sse_y;
      block_yrd(cpi, x, mi_row, mi_col, &this_rdc, &is_skippable, &this_sse,
                bsize, mi->tx_size);
#endif

      x->skip = this_rdc.skip;
      if (this_rdc.skip) {
        this_rdc.rate = skip_cost;
      } else {
#if !_TMP_USE_CURVFIT_
        // on CurvFit this condition is checked inside curvfit modeling
        if (RDCOST(x->rdmult, this_rdc.rate, this_rdc.dist) >=
            RDCOST(
                x->rdmult, 0,
                this_sse)) {  // this_sse already multiplied by 16 in block_yrd
          x->skip = 1;
          this_rdc.rate = skip_cost;
          this_rdc.dist = this_sse;
        } else
#endif
        {
          this_rdc.rate += no_skip_cost;
        }
      }
    } else {
      x->skip = 1;
      this_rdc.rate = skip_cost;
      this_rdc.dist = sse_y << 4;
    }

    // TODO(kyslov) account for UV prediction cost
    this_rdc.rate += rate_mv;
    const int16_t mode_ctx =
        av1_mode_context_analyzer(mbmi_ext->mode_context, mi->ref_frame);
    this_rdc.rate += cost_mv_ref(x, this_mode, mode_ctx);

    this_rdc.rate += ref_costs_single[ref_frame];

    this_rdc.rdcost = RDCOST(x->rdmult, this_rdc.rate, this_rdc.dist);
    if (cpi->oxcf.rc_mode == AOM_CBR && cpi->oxcf.speed >= 8) {
      newmv_diff_bias(xd, this_mode, &this_rdc, bsize,
                      frame_mv[this_mode][ref_frame].as_mv.row,
                      frame_mv[this_mode][ref_frame].as_mv.col,
                      ref_frame == LAST_FRAME);
    }

    mode_checked[this_mode][ref_frame] = 1;

    if (this_rdc.rdcost < best_rdc.rdcost) {
      best_rdc = this_rdc;
      best_early_term = this_early_term;
      best_pickmode.best_mode = this_mode;
      best_pickmode.best_pred_filter = mi->interp_filters;
      best_pickmode.best_tx_size = mi->tx_size;
      best_pickmode.best_ref_frame = ref_frame;
      best_pickmode.best_mode_skip_txfm = x->skip;
      best_pickmode.best_second_ref_frame = second_ref_frame;
      if (reuse_inter_pred) {
        free_pred_buffer(best_pickmode.best_pred);
        best_pickmode.best_pred = this_mode_pred;
      }
    } else {
      if (reuse_inter_pred) free_pred_buffer(this_mode_pred);
    }
    if (best_early_term && idx > 0) {
      x->skip = 1;
      break;
    }
  }

  mi->mode = best_pickmode.best_mode;
  mi->interp_filters = av1_broadcast_interp_filter(best_filter);
  mi->tx_size = best_pickmode.best_tx_size;
  memset(mi->inter_tx_size, mi->tx_size, sizeof(mi->inter_tx_size));
  mi->ref_frame[0] = best_pickmode.best_ref_frame;
  mi->mv[0].as_int =
      frame_mv[best_pickmode.best_mode][best_pickmode.best_ref_frame].as_int;
  mi->ref_frame[1] = best_pickmode.best_second_ref_frame;

  // Perform intra prediction search, if the best SAD is above a certain
  // threshold.
  mi->angle_delta[PLANE_TYPE_Y] = 0;
  mi->angle_delta[PLANE_TYPE_UV] = 0;
  mi->filter_intra_mode_info.use_filter_intra = 0;
  // TODO(kyslov@) Need to adjust inter_mode_thresh
  if (best_rdc.rdcost == INT64_MAX ||
      (perform_intra_pred && !x->skip && best_rdc.rdcost > inter_mode_thresh &&
       bsize <= cpi->sf.max_intra_bsize)) {
    struct estimate_block_intra_args args = { cpi, x, DC_PRED, 1, 0 };
    PRED_BUFFER *const best_pred = best_pickmode.best_pred;
    TX_SIZE intra_tx_size =
        AOMMIN(AOMMIN(max_txsize_lookup[bsize],
                      tx_mode_to_biggest_tx_size[cpi->common.tx_mode]),
               TX_16X16);

    if (reuse_inter_pred && best_pred != NULL) {
      if (best_pred->data == orig_dst.buf) {
        this_mode_pred = &tmp[get_pred_buffer(tmp, 3)];
        aom_convolve_copy(best_pred->data, best_pred->stride,
                          this_mode_pred->data, this_mode_pred->stride, 0, 0, 0,
                          0, bw, bh);
        best_pickmode.best_pred = this_mode_pred;
      }
    }
    pd->dst = orig_dst;

    for (int i = 0; i < 4; ++i) {
      const PREDICTION_MODE this_mode = intra_mode_list[i];
      const THR_MODES mode_index =
          mode_idx[INTRA_FRAME][mode_offset(this_mode)];
      const int mode_rd_thresh = rd_threshes[mode_index];

      if (rd_less_than_thresh(best_rdc.rdcost, mode_rd_thresh,
                              rd_thresh_freq_fact[mode_index])) {
        continue;
      }

      mi->mode = this_mode;
      mi->ref_frame[0] = INTRA_FRAME;
      mi->ref_frame[1] = NONE_FRAME;

      this_rdc.dist = this_rdc.rate = 0;
      args.mode = this_mode;
      args.skippable = 1;
      args.rdc = &this_rdc;
      mi->tx_size = intra_tx_size;
      av1_foreach_transformed_block_in_plane(xd, bsize, 0, estimate_block_intra,
                                             &args);
      // TODO(kyslov@) Need to account for skippable
      int mode_cost = 0;
      if (av1_is_directional_mode(this_mode) && av1_use_angle_delta(bsize)) {
        mode_cost += x->angle_delta_cost[this_mode - V_PRED]
                                        [MAX_ANGLE_DELTA +
                                         mi->angle_delta[PLANE_TYPE_Y]];
      }
      if (this_mode == DC_PRED && av1_filter_intra_allowed_bsize(cm, bsize)) {
        mode_cost += x->filter_intra_cost[bsize][0];
      }
      this_rdc.rate += ref_costs_single[INTRA_FRAME];
      this_rdc.rate += intra_cost_penalty;
      this_rdc.rate += mode_cost;
      this_rdc.rdcost = RDCOST(x->rdmult, this_rdc.rate, this_rdc.dist);

      if (this_rdc.rdcost < best_rdc.rdcost) {
        best_rdc = this_rdc;
        best_pickmode.best_mode = this_mode;
        best_pickmode.best_intra_tx_size = mi->tx_size;
        best_pickmode.best_ref_frame = INTRA_FRAME;
        best_pickmode.best_second_ref_frame = NONE_FRAME;
        mi->uv_mode = this_mode;
        mi->mv[0].as_int = INVALID_MV;
        mi->mv[1].as_int = INVALID_MV;
      }
    }

    // Reset mb_mode_info to the best inter mode.
    if (best_pickmode.best_ref_frame != INTRA_FRAME) {
      mi->tx_size = best_pickmode.best_tx_size;
    } else {
      mi->tx_size = best_pickmode.best_intra_tx_size;
    }
  }

  pd->dst = orig_dst;
  mi->mode = best_pickmode.best_mode;
  mi->ref_frame[0] = best_pickmode.best_ref_frame;
  mi->ref_frame[1] = best_pickmode.best_second_ref_frame;

  if (!is_inter_block(mi)) {
    mi->interp_filters = av1_broadcast_interp_filter(SWITCHABLE_FILTERS);
  }

  if (reuse_inter_pred && best_pickmode.best_pred != NULL) {
    PRED_BUFFER *const best_pred = best_pickmode.best_pred;
    if (best_pred->data != orig_dst.buf && is_inter_mode(mi->mode)) {
      aom_convolve_copy(best_pred->data, best_pred->stride, pd->dst.buf,
                        pd->dst.stride, 0, 0, 0, 0, bw, bh);
    }
  }

  store_coding_context(x, ctx, mi->mode);
  *rd_cost = best_rdc;
}
