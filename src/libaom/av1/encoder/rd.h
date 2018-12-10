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

#ifndef AOM_AV1_ENCODER_RD_H_
#define AOM_AV1_ENCODER_RD_H_

#include <limits.h>

#include "av1/common/blockd.h"

#include "av1/encoder/block.h"
#include "av1/encoder/context_tree.h"
#include "av1/encoder/cost.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RDDIV_BITS 7
#define RD_EPB_SHIFT 6

#define RDCOST(RM, R, D)                                            \
  (ROUND_POWER_OF_TWO(((int64_t)(R)) * (RM), AV1_PROB_COST_SHIFT) + \
   ((D) * (1 << RDDIV_BITS)))

#define RDCOST_DBL(RM, R, D)                                       \
  (((((double)(R)) * (RM)) / (double)(1 << AV1_PROB_COST_SHIFT)) + \
   ((double)(D) * (1 << RDDIV_BITS)))

#define QIDX_SKIP_THRESH 115

#define MV_COST_WEIGHT 108
#define MV_COST_WEIGHT_SUB 120

#define RD_THRESH_MAX_FACT 64
#define RD_THRESH_INC 1

// Factor to weigh the rate for switchable interp filters.
#define SWITCHABLE_INTERP_RATE_FACTOR 1

// This enumerator type needs to be kept aligned with the mode order in
// const MODE_DEFINITION av1_mode_order[MAX_MODES] used in the rd code.
typedef enum {
  THR_NEARESTMV,
  THR_NEARESTL2,
  THR_NEARESTL3,
  THR_NEARESTB,
  THR_NEARESTA2,
  THR_NEARESTA,
  THR_NEARESTG,

  THR_NEWMV,
  THR_NEWL2,
  THR_NEWL3,
  THR_NEWB,
  THR_NEWA2,
  THR_NEWA,
  THR_NEWG,

  THR_NEARMV,
  THR_NEARL2,
  THR_NEARL3,
  THR_NEARB,
  THR_NEARA2,
  THR_NEARA,
  THR_NEARG,

  THR_GLOBALMV,
  THR_GLOBALL2,
  THR_GLOBALL3,
  THR_GLOBALB,
  THR_GLOBALA2,
  THR_GLOBALA,
  THR_GLOBALG,

  THR_COMP_NEAREST_NEARESTLA,
  THR_COMP_NEAREST_NEARESTL2A,
  THR_COMP_NEAREST_NEARESTL3A,
  THR_COMP_NEAREST_NEARESTGA,
  THR_COMP_NEAREST_NEARESTLB,
  THR_COMP_NEAREST_NEARESTL2B,
  THR_COMP_NEAREST_NEARESTL3B,
  THR_COMP_NEAREST_NEARESTGB,
  THR_COMP_NEAREST_NEARESTLA2,
  THR_COMP_NEAREST_NEARESTL2A2,
  THR_COMP_NEAREST_NEARESTL3A2,
  THR_COMP_NEAREST_NEARESTGA2,
  THR_COMP_NEAREST_NEARESTLL2,
  THR_COMP_NEAREST_NEARESTLL3,
  THR_COMP_NEAREST_NEARESTLG,
  THR_COMP_NEAREST_NEARESTBA,

  THR_COMP_NEAR_NEARLA,
  THR_COMP_NEW_NEARESTLA,
  THR_COMP_NEAREST_NEWLA,
  THR_COMP_NEW_NEARLA,
  THR_COMP_NEAR_NEWLA,
  THR_COMP_NEW_NEWLA,
  THR_COMP_GLOBAL_GLOBALLA,

  THR_COMP_NEAR_NEARL2A,
  THR_COMP_NEW_NEARESTL2A,
  THR_COMP_NEAREST_NEWL2A,
  THR_COMP_NEW_NEARL2A,
  THR_COMP_NEAR_NEWL2A,
  THR_COMP_NEW_NEWL2A,
  THR_COMP_GLOBAL_GLOBALL2A,

  THR_COMP_NEAR_NEARL3A,
  THR_COMP_NEW_NEARESTL3A,
  THR_COMP_NEAREST_NEWL3A,
  THR_COMP_NEW_NEARL3A,
  THR_COMP_NEAR_NEWL3A,
  THR_COMP_NEW_NEWL3A,
  THR_COMP_GLOBAL_GLOBALL3A,

  THR_COMP_NEAR_NEARGA,
  THR_COMP_NEW_NEARESTGA,
  THR_COMP_NEAREST_NEWGA,
  THR_COMP_NEW_NEARGA,
  THR_COMP_NEAR_NEWGA,
  THR_COMP_NEW_NEWGA,
  THR_COMP_GLOBAL_GLOBALGA,

  THR_COMP_NEAR_NEARLB,
  THR_COMP_NEW_NEARESTLB,
  THR_COMP_NEAREST_NEWLB,
  THR_COMP_NEW_NEARLB,
  THR_COMP_NEAR_NEWLB,
  THR_COMP_NEW_NEWLB,
  THR_COMP_GLOBAL_GLOBALLB,

  THR_COMP_NEAR_NEARL2B,
  THR_COMP_NEW_NEARESTL2B,
  THR_COMP_NEAREST_NEWL2B,
  THR_COMP_NEW_NEARL2B,
  THR_COMP_NEAR_NEWL2B,
  THR_COMP_NEW_NEWL2B,
  THR_COMP_GLOBAL_GLOBALL2B,

  THR_COMP_NEAR_NEARL3B,
  THR_COMP_NEW_NEARESTL3B,
  THR_COMP_NEAREST_NEWL3B,
  THR_COMP_NEW_NEARL3B,
  THR_COMP_NEAR_NEWL3B,
  THR_COMP_NEW_NEWL3B,
  THR_COMP_GLOBAL_GLOBALL3B,

  THR_COMP_NEAR_NEARGB,
  THR_COMP_NEW_NEARESTGB,
  THR_COMP_NEAREST_NEWGB,
  THR_COMP_NEW_NEARGB,
  THR_COMP_NEAR_NEWGB,
  THR_COMP_NEW_NEWGB,
  THR_COMP_GLOBAL_GLOBALGB,

  THR_COMP_NEAR_NEARLA2,
  THR_COMP_NEW_NEARESTLA2,
  THR_COMP_NEAREST_NEWLA2,
  THR_COMP_NEW_NEARLA2,
  THR_COMP_NEAR_NEWLA2,
  THR_COMP_NEW_NEWLA2,
  THR_COMP_GLOBAL_GLOBALLA2,

  THR_COMP_NEAR_NEARL2A2,
  THR_COMP_NEW_NEARESTL2A2,
  THR_COMP_NEAREST_NEWL2A2,
  THR_COMP_NEW_NEARL2A2,
  THR_COMP_NEAR_NEWL2A2,
  THR_COMP_NEW_NEWL2A2,
  THR_COMP_GLOBAL_GLOBALL2A2,

  THR_COMP_NEAR_NEARL3A2,
  THR_COMP_NEW_NEARESTL3A2,
  THR_COMP_NEAREST_NEWL3A2,
  THR_COMP_NEW_NEARL3A2,
  THR_COMP_NEAR_NEWL3A2,
  THR_COMP_NEW_NEWL3A2,
  THR_COMP_GLOBAL_GLOBALL3A2,

  THR_COMP_NEAR_NEARGA2,
  THR_COMP_NEW_NEARESTGA2,
  THR_COMP_NEAREST_NEWGA2,
  THR_COMP_NEW_NEARGA2,
  THR_COMP_NEAR_NEWGA2,
  THR_COMP_NEW_NEWGA2,
  THR_COMP_GLOBAL_GLOBALGA2,

  THR_COMP_NEAR_NEARLL2,
  THR_COMP_NEW_NEARESTLL2,
  THR_COMP_NEAREST_NEWLL2,
  THR_COMP_NEW_NEARLL2,
  THR_COMP_NEAR_NEWLL2,
  THR_COMP_NEW_NEWLL2,
  THR_COMP_GLOBAL_GLOBALLL2,

  THR_COMP_NEAR_NEARLL3,
  THR_COMP_NEW_NEARESTLL3,
  THR_COMP_NEAREST_NEWLL3,
  THR_COMP_NEW_NEARLL3,
  THR_COMP_NEAR_NEWLL3,
  THR_COMP_NEW_NEWLL3,
  THR_COMP_GLOBAL_GLOBALLL3,

  THR_COMP_NEAR_NEARLG,
  THR_COMP_NEW_NEARESTLG,
  THR_COMP_NEAREST_NEWLG,
  THR_COMP_NEW_NEARLG,
  THR_COMP_NEAR_NEWLG,
  THR_COMP_NEW_NEWLG,
  THR_COMP_GLOBAL_GLOBALLG,

  THR_COMP_NEAR_NEARBA,
  THR_COMP_NEW_NEARESTBA,
  THR_COMP_NEAREST_NEWBA,
  THR_COMP_NEW_NEARBA,
  THR_COMP_NEAR_NEWBA,
  THR_COMP_NEW_NEWBA,
  THR_COMP_GLOBAL_GLOBALBA,

  THR_DC,
  THR_PAETH,
  THR_SMOOTH,
  THR_SMOOTH_V,
  THR_SMOOTH_H,
  THR_H_PRED,
  THR_V_PRED,
  THR_D135_PRED,
  THR_D203_PRED,
  THR_D157_PRED,
  THR_D67_PRED,
  THR_D113_PRED,
  THR_D45_PRED,

  MAX_MODES,

  LAST_SINGLE_REF_MODES = THR_GLOBALG,
  MAX_SINGLE_REF_MODES = LAST_SINGLE_REF_MODES + 1,
  LAST_COMP_REF_MODES = THR_COMP_GLOBAL_GLOBALBA,
  MAX_COMP_REF_MODES = LAST_COMP_REF_MODES + 1
} THR_MODES;

typedef enum {
  THR_LAST,
  THR_LAST2,
  THR_LAST3,
  THR_BWDR,
  THR_ALTR2,
  THR_GOLD,
  THR_ALTR,

  THR_COMP_LA,
  THR_COMP_L2A,
  THR_COMP_L3A,
  THR_COMP_GA,

  THR_COMP_LB,
  THR_COMP_L2B,
  THR_COMP_L3B,
  THR_COMP_GB,

  THR_COMP_LA2,
  THR_COMP_L2A2,
  THR_COMP_L3A2,
  THR_COMP_GA2,

  THR_INTRA,

  MAX_REFS
} THR_MODES_SUB8X8;

typedef struct RD_OPT {
  // Thresh_mult is used to set a threshold for the rd score. A higher value
  // means that we will accept the best mode so far more often. This number
  // is used in combination with the current block size, and thresh_freq_fact
  // to pick a threshold.
  int thresh_mult[MAX_MODES];
  int thresh_mult_sub8x8[MAX_REFS];

  int threshes[MAX_SEGMENTS][BLOCK_SIZES_ALL][MAX_MODES];

  int64_t prediction_type_threshes[REF_FRAMES][REFERENCE_MODES];

  int RDMULT;

  double r0;
} RD_OPT;

static INLINE void av1_init_rd_stats(RD_STATS *rd_stats) {
#if CONFIG_RD_DEBUG
  int plane;
#endif
  rd_stats->rate = 0;
  rd_stats->dist = 0;
  rd_stats->rdcost = 0;
  rd_stats->sse = 0;
  rd_stats->skip = 1;
  rd_stats->zero_rate = 0;
  rd_stats->invalid_rate = 0;
  rd_stats->ref_rdcost = INT64_MAX;
#if CONFIG_RD_DEBUG
  // This may run into problems when monochrome video is
  // encoded, as there will only be 1 plane
  for (plane = 0; plane < MAX_MB_PLANE; ++plane) {
    rd_stats->txb_coeff_cost[plane] = 0;
    {
      int r, c;
      for (r = 0; r < TXB_COEFF_COST_MAP_SIZE; ++r)
        for (c = 0; c < TXB_COEFF_COST_MAP_SIZE; ++c)
          rd_stats->txb_coeff_cost_map[plane][r][c] = 0;
    }
  }
#endif
#if CONFIG_ONE_PASS_SVM
  rd_stats->eob = 0;
  rd_stats->eob_0 = 0;
  rd_stats->eob_1 = 0;
  rd_stats->eob_2 = 0;
  rd_stats->eob_3 = 0;

  rd_stats->rd = 0;
  rd_stats->rd_0 = 0;
  rd_stats->rd_1 = 0;
  rd_stats->rd_2 = 0;
  rd_stats->rd_3 = 0;

  rd_stats->y_sse = 0;
  rd_stats->sse_0 = 0;
  rd_stats->sse_1 = 0;
  rd_stats->sse_2 = 0;
  rd_stats->sse_3 = 0;
#endif
}

static INLINE void av1_invalid_rd_stats(RD_STATS *rd_stats) {
#if CONFIG_RD_DEBUG
  int plane;
#endif
  rd_stats->rate = INT_MAX;
  rd_stats->dist = INT64_MAX;
  rd_stats->rdcost = INT64_MAX;
  rd_stats->sse = INT64_MAX;
  rd_stats->skip = 0;
  rd_stats->zero_rate = 0;
  rd_stats->invalid_rate = 1;
  rd_stats->ref_rdcost = INT64_MAX;
#if CONFIG_RD_DEBUG
  // This may run into problems when monochrome video is
  // encoded, as there will only be 1 plane
  for (plane = 0; plane < MAX_MB_PLANE; ++plane) {
    rd_stats->txb_coeff_cost[plane] = INT_MAX;
    {
      int r, c;
      for (r = 0; r < TXB_COEFF_COST_MAP_SIZE; ++r)
        for (c = 0; c < TXB_COEFF_COST_MAP_SIZE; ++c)
          rd_stats->txb_coeff_cost_map[plane][r][c] = INT_MAX;
    }
  }
#endif
#if CONFIG_ONE_PASS_SVM
  // TODO(chiyotsai@google.com): Change invalid values to INT_MAX and
  // INT64_MAX. Currently there are some code paths where rd_stats's properties
  // are set directly without calling av1_init_rd_stats, so changing it now will
  // break this speed feature. Need to hunt down all places where rd_stats is
  // used without initialized.
  rd_stats->eob = 0;
  rd_stats->eob_0 = 0;
  rd_stats->eob_1 = 0;
  rd_stats->eob_2 = 0;
  rd_stats->eob_3 = 0;

  rd_stats->rd = 0;
  rd_stats->rd_0 = 0;
  rd_stats->rd_1 = 0;
  rd_stats->rd_2 = 0;
  rd_stats->rd_3 = 0;

  rd_stats->y_sse = 0;
  rd_stats->sse_0 = 0;
  rd_stats->sse_1 = 0;
  rd_stats->sse_2 = 0;
  rd_stats->sse_3 = 0;
#endif
}

static INLINE void av1_merge_rd_stats(RD_STATS *rd_stats_dst,
                                      const RD_STATS *rd_stats_src) {
#if CONFIG_RD_DEBUG
  int plane;
#endif
  rd_stats_dst->rate += rd_stats_src->rate;
  if (!rd_stats_dst->zero_rate)
    rd_stats_dst->zero_rate = rd_stats_src->zero_rate;
  rd_stats_dst->dist += rd_stats_src->dist;
  rd_stats_dst->sse += rd_stats_src->sse;
  rd_stats_dst->skip &= rd_stats_src->skip;
  rd_stats_dst->invalid_rate &= rd_stats_src->invalid_rate;
#if CONFIG_RD_DEBUG
  // This may run into problems when monochrome video is
  // encoded, as there will only be 1 plane
  for (plane = 0; plane < MAX_MB_PLANE; ++plane) {
    rd_stats_dst->txb_coeff_cost[plane] += rd_stats_src->txb_coeff_cost[plane];
    {
      // TODO(angiebird): optimize this part
      int r, c;
      int ref_txb_coeff_cost = 0;
      for (r = 0; r < TXB_COEFF_COST_MAP_SIZE; ++r)
        for (c = 0; c < TXB_COEFF_COST_MAP_SIZE; ++c) {
          rd_stats_dst->txb_coeff_cost_map[plane][r][c] +=
              rd_stats_src->txb_coeff_cost_map[plane][r][c];
          ref_txb_coeff_cost += rd_stats_dst->txb_coeff_cost_map[plane][r][c];
        }
      assert(ref_txb_coeff_cost == rd_stats_dst->txb_coeff_cost[plane]);
    }
  }
#endif
#if CONFIG_ONE_PASS_SVM
  rd_stats_dst->eob += rd_stats_src->eob;
  rd_stats_dst->eob_0 += rd_stats_src->eob_0;
  rd_stats_dst->eob_1 += rd_stats_src->eob_1;
  rd_stats_dst->eob_2 += rd_stats_src->eob_2;
  rd_stats_dst->eob_3 += rd_stats_src->eob_3;

  rd_stats_dst->rd += rd_stats_src->rd;
  rd_stats_dst->rd_0 += rd_stats_src->rd_0;
  rd_stats_dst->rd_1 += rd_stats_src->rd_1;
  rd_stats_dst->rd_2 += rd_stats_src->rd_2;
  rd_stats_dst->rd_3 += rd_stats_src->rd_3;

  rd_stats_dst->y_sse += rd_stats_src->y_sse;
  rd_stats_dst->sse_0 += rd_stats_src->sse_0;
  rd_stats_dst->sse_1 += rd_stats_src->sse_1;
  rd_stats_dst->sse_2 += rd_stats_src->sse_2;
  rd_stats_dst->sse_3 += rd_stats_src->sse_3;
#endif
}

#if CONFIG_ONE_PASS_SVM
static INLINE void av1_add_reg_stat(RD_STATS *rd_stats, int eob, int64_t rd,
                                    int64_t sse, int blk_row, int blk_col,
                                    BLOCK_SIZE bsize, BLOCK_SIZE crop_bsize) {
  // NOTE: Currently the calculation of regional features works by assuming
  // bsize is square so that each transform block of size crop_bsize either
  // 1. locates completely within a quadrant or
  // 2. is exactly half of bsize or
  // 3. is the entire prediction block
  // Size of TX block and SB
  const int block_width_mi = mi_size_wide[bsize];
  const int block_height_mi = mi_size_high[bsize];
  const int crop_width_mi = mi_size_wide[crop_bsize];
  const int crop_height_mi = mi_size_high[crop_bsize];

  // Increment the eob proportionally to how much the tx_block overlaps with
  // each quadrant. We will scale it by MAX_MIB_SIZE * MAX_MIB_SIZE to avoid
  // being truncated.
  const int max_scaling_factor = MAX_MIB_SIZE * MAX_MIB_SIZE;

  // Update the stats
  rd_stats->eob = eob;
  rd_stats->rd = rd;
  rd_stats->y_sse = sse;

  if (crop_width_mi <= block_width_mi / 2 &&
      crop_height_mi <= block_width_mi / 2) {
    // The transform block lies completely in a quadrant.
    const int scaling_factor = max_scaling_factor;
    const int r_eob = eob * scaling_factor, r_rd = rd * scaling_factor,
              r_sse = sse * scaling_factor;

    if (blk_row < block_height_mi / 2 && blk_col < block_width_mi / 2) {
      rd_stats->eob_0 = r_eob;
      rd_stats->rd_0 = r_rd;
      rd_stats->sse_0 = r_sse;
    } else if (blk_row < block_height_mi / 2 && blk_col >= block_width_mi / 2) {
      rd_stats->eob_1 = r_eob;
      rd_stats->rd_1 = r_rd;
      rd_stats->sse_1 = r_sse;
    } else if (blk_row >= block_height_mi / 2 && blk_col < block_width_mi / 2) {
      rd_stats->eob_2 = r_eob;
      rd_stats->rd_2 = r_rd;
      rd_stats->sse_2 = r_sse;
    } else {
      rd_stats->eob_3 = r_eob;
      rd_stats->rd_3 = r_rd;
      rd_stats->sse_3 = r_sse;
    }
  } else if (crop_height_mi == block_height_mi &&
             crop_width_mi == block_width_mi) {
    // The transform block is the whole prediction block
    const int scaling_factor = max_scaling_factor;
    const int r_eob = eob * scaling_factor, r_rd = rd * scaling_factor,
              r_sse = sse * scaling_factor;

    rd_stats->eob_0 = r_eob;
    rd_stats->rd_0 = r_rd;
    rd_stats->sse_0 = r_sse;

    rd_stats->eob_1 = r_eob;
    rd_stats->rd_1 = r_rd;
    rd_stats->sse_1 = r_sse;

    rd_stats->eob_2 = r_eob;
    rd_stats->rd_2 = r_rd;
    rd_stats->sse_2 = r_sse;

    rd_stats->eob_3 = r_eob;
    rd_stats->rd_3 = r_rd;
    rd_stats->sse_3 = r_sse;
  } else if (crop_height_mi == block_height_mi) {
    // The tranform block is a vertical block
    const int scaling_factor = max_scaling_factor / 2;
    const int r_eob = eob * scaling_factor, r_rd = rd * scaling_factor,
              r_sse = sse * scaling_factor;

    if (blk_col < block_width_mi / 2) {
      rd_stats->eob_0 = r_eob;
      rd_stats->rd_0 = r_rd;
      rd_stats->sse_0 = r_sse;

      rd_stats->eob_2 = r_eob;
      rd_stats->rd_2 = r_rd;
      rd_stats->sse_2 = r_sse;
    } else {
      rd_stats->eob_1 = r_eob;
      rd_stats->rd_1 = r_rd;
      rd_stats->sse_1 = r_sse;

      rd_stats->eob_3 = r_eob;
      rd_stats->rd_3 = r_rd;
      rd_stats->sse_3 = r_sse;
    }
  } else if (crop_width_mi == block_width_mi) {
    // The tranform block is a horizontal block half the size of predition block
    const int scaling_factor = max_scaling_factor / 2;
    const int r_eob = eob * scaling_factor, r_rd = rd * scaling_factor,
              r_sse = sse * scaling_factor;

    if (blk_row < block_height_mi / 2) {
      rd_stats->eob_0 = r_eob;
      rd_stats->rd_0 = r_rd;
      rd_stats->sse_0 = r_sse;

      rd_stats->eob_1 = r_eob;
      rd_stats->rd_1 = r_rd;
      rd_stats->sse_1 = r_sse;
    } else {
      rd_stats->eob_2 = r_eob;
      rd_stats->rd_2 = r_rd;
      rd_stats->sse_2 = r_sse;

      rd_stats->eob_3 = r_eob;
      rd_stats->rd_3 = r_rd;
      rd_stats->sse_3 = r_sse;
    }
  } else {
    assert(0 && "Unexpected transform size");
  }
}

static INLINE void av1_reg_stat_skipmode_update(RD_STATS *rd_stats,
                                                int rdmult) {
  // Update the stats
  rd_stats->eob = 0;
  rd_stats->eob_0 = 0;
  rd_stats->eob_1 = 0;
  rd_stats->eob_2 = 0;
  rd_stats->eob_3 = 0;

  rd_stats->rd = RDCOST(rdmult, 0, rd_stats->sse);
  rd_stats->rd_0 = RDCOST(rdmult, 0, rd_stats->sse_0);
  rd_stats->rd_1 = RDCOST(rdmult, 0, rd_stats->sse_1);
  rd_stats->rd_2 = RDCOST(rdmult, 0, rd_stats->sse_2);
  rd_stats->rd_3 = RDCOST(rdmult, 0, rd_stats->sse_3);
}

static INLINE void av1_copy_reg_stat(RD_STATS *rd_stats_dst,
                                     RD_STATS *rd_stats_src) {
  rd_stats_dst->eob = rd_stats_src->eob;
  rd_stats_dst->eob_0 = rd_stats_src->eob_0;
  rd_stats_dst->eob_1 = rd_stats_src->eob_1;
  rd_stats_dst->eob_2 = rd_stats_src->eob_2;
  rd_stats_dst->eob_3 = rd_stats_src->eob_3;

  rd_stats_dst->rd = rd_stats_src->rd;
  rd_stats_dst->rd_0 = rd_stats_src->rd_0;
  rd_stats_dst->rd_1 = rd_stats_src->rd_1;
  rd_stats_dst->rd_2 = rd_stats_src->rd_2;
  rd_stats_dst->rd_3 = rd_stats_src->rd_3;

  rd_stats_dst->y_sse = rd_stats_src->y_sse;
  rd_stats_dst->sse_0 = rd_stats_src->sse_0;
  rd_stats_dst->sse_1 = rd_stats_src->sse_1;
  rd_stats_dst->sse_2 = rd_stats_src->sse_2;
  rd_stats_dst->sse_3 = rd_stats_src->sse_3;
}

static INLINE void av1_unpack_reg_stat(RD_STATS *rd_stats, int *eob, int *eob_0,
                                       int *eob_1, int *eob_2, int *eob_3,
                                       int64_t *rd, int64_t *rd_0,
                                       int64_t *rd_1, int64_t *rd_2,
                                       int64_t *rd_3) {
  *rd = rd_stats->rd;
  *rd_0 = rd_stats->rd_0;
  *rd_1 = rd_stats->rd_1;
  *rd_2 = rd_stats->rd_2;
  *rd_3 = rd_stats->rd_3;

  *eob = rd_stats->eob;
  *eob_0 = rd_stats->eob_0;
  *eob_1 = rd_stats->eob_1;
  *eob_2 = rd_stats->eob_2;
  *eob_3 = rd_stats->eob_3;
}

static INLINE void av1_set_reg_stat(RD_STATS *rd_stats, int eob, int eob_0,
                                    int eob_1, int eob_2, int eob_3, int64_t rd,
                                    int64_t rd_0, int64_t rd_1, int64_t rd_2,
                                    int64_t rd_3) {
  rd_stats->rd = rd;
  rd_stats->rd_0 = rd_0;
  rd_stats->rd_1 = rd_1;
  rd_stats->rd_2 = rd_2;
  rd_stats->rd_3 = rd_3;

  rd_stats->eob = eob;
  rd_stats->eob_0 = eob_0;
  rd_stats->eob_1 = eob_1;
  rd_stats->eob_2 = eob_2;
  rd_stats->eob_3 = eob_3;
}
#endif

struct TileInfo;
struct TileDataEnc;
struct AV1_COMP;
struct macroblock;

int av1_compute_rd_mult_based_on_qindex(const struct AV1_COMP *cpi, int qindex);

int av1_compute_rd_mult(const struct AV1_COMP *cpi, int qindex);

void av1_initialize_rd_consts(struct AV1_COMP *cpi);

void av1_initialize_cost_tables(const AV1_COMMON *const cm, MACROBLOCK *x);

void av1_initialize_me_consts(const struct AV1_COMP *cpi, MACROBLOCK *x,
                              int qindex);

void av1_model_rd_from_var_lapndz(int64_t var, unsigned int n,
                                  unsigned int qstep, int *rate, int64_t *dist);

void av1_model_rd_curvfit(double xqr, double *rate_f, double *distbysse_f);
void av1_model_rd_surffit(double xm, double yl, double *rate_f,
                          double *distbysse_f);

int av1_get_switchable_rate(const AV1_COMMON *const cm, MACROBLOCK *x,
                            const MACROBLOCKD *xd);

int av1_raster_block_offset(BLOCK_SIZE plane_bsize, int raster_block,
                            int stride);

int16_t *av1_raster_block_offset_int16(BLOCK_SIZE plane_bsize, int raster_block,
                                       int16_t *base);

YV12_BUFFER_CONFIG *av1_get_scaled_ref_frame(const struct AV1_COMP *cpi,
                                             int ref_frame);

void av1_init_me_luts(void);

void av1_set_mvcost(MACROBLOCK *x, int ref, int ref_mv_idx);

void av1_get_entropy_contexts(BLOCK_SIZE bsize,
                              const struct macroblockd_plane *pd,
                              ENTROPY_CONTEXT t_above[MAX_MIB_SIZE],
                              ENTROPY_CONTEXT t_left[MAX_MIB_SIZE]);

void av1_set_rd_speed_thresholds(struct AV1_COMP *cpi);

void av1_set_rd_speed_thresholds_sub8x8(struct AV1_COMP *cpi);

void av1_update_rd_thresh_fact(const AV1_COMMON *const cm,
                               int (*fact)[MAX_MODES], int rd_thresh, int bsize,
                               int best_mode_index);

static INLINE int rd_less_than_thresh(int64_t best_rd, int thresh,
                                      int thresh_fact) {
  return best_rd < ((int64_t)thresh * thresh_fact >> 5) || thresh == INT_MAX;
}

void av1_mv_pred(const struct AV1_COMP *cpi, MACROBLOCK *x,
                 uint8_t *ref_y_buffer, int ref_y_stride, int ref_frame,
                 BLOCK_SIZE block_size);

static INLINE void set_error_per_bit(MACROBLOCK *x, int rdmult) {
  x->errorperbit = rdmult >> RD_EPB_SHIFT;
  x->errorperbit += (x->errorperbit == 0);
}

void av1_setup_pred_block(const MACROBLOCKD *xd,
                          struct buf_2d dst[MAX_MB_PLANE],
                          const YV12_BUFFER_CONFIG *src, int mi_row, int mi_col,
                          const struct scale_factors *scale,
                          const struct scale_factors *scale_uv,
                          const int num_planes);

int av1_get_intra_cost_penalty(int qindex, int qdelta,
                               aom_bit_depth_t bit_depth);

void av1_fill_mode_rates(AV1_COMMON *const cm, MACROBLOCK *x,
                         FRAME_CONTEXT *fc);

void av1_fill_coeff_costs(MACROBLOCK *x, FRAME_CONTEXT *fc,
                          const int num_planes);

int av1_get_adaptive_rdmult(const struct AV1_COMP *cpi, double beta);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // AOM_AV1_ENCODER_RD_H_
