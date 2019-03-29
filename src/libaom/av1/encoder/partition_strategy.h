/*
 * Copyright (c) 2019, Alliance for Open Media. All rights reserved
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */

#ifndef AOM_AV1_ENCODER_PARTITION_STRATEGY_H_
#define AOM_AV1_ENCODER_PARTITION_STRATEGY_H_

#include "av1/encoder/encodeframe.h"
#include "av1/encoder/encodemb.h"
#include "av1/encoder/encoder.h"

#define FEATURE_SIZE_SMS_PRUNE_PART 25
#define FEATURE_SIZE_SMS_TERM_NONE 28
#define FEATURE_SIZE_FP_SMS_TERM_NONE 20
#define FEATURE_SIZE_MAX_MIN_PART_PRED 13
#define MAX_NUM_CLASSES_MAX_MIN_PART_PRED 4

// Performs a simple_motion_search with a single reference frame and extract
// the variance of residues. Then use the features to determine whether we want
// to go straight to splitting without trying PARTITION_NONE
void av1_simple_motion_search_based_split(
    AV1_COMP *const cpi, MACROBLOCK *x, int mi_row, int mi_col,
    BLOCK_SIZE bsize, int *partition_none_allowed, int *partition_horz_allowed,
    int *partition_vert_allowed, int *do_rectangular_split,
    int *do_square_split);

// Performs a simple_motion_search with two reference frames and extract
// the variance of residues. Then use the features to determine whether we want
// to prune some partitions.
void av1_simple_motion_search_prune_part(
    AV1_COMP *const cpi, MACROBLOCK *x, PC_TREE *pc_tree, int mi_row,
    int mi_col, BLOCK_SIZE bsize, int *partition_none_allowed,
    int *partition_horz_allowed, int *partition_vert_allowed,
    int *do_square_split, int *do_rectangular_split, int *prune_horz,
    int *prune_vert, float *features, int *valid);

// Early terminates PARTITION_NONE using simple_motion_search features and the
// rate, distortion, and rdcost of PARTITION_NONE. This is only called when:
//  - The frame is a show frame
//  - The frame is not intra only
//  - The current bsize is > BLOCK_8X8
//  - blk_row + blk_height/2 < total_rows and blk_col + blk_width/2 < total_cols
void av1_simple_motion_search_early_term_none(
    AV1_COMP *const cpi, MACROBLOCK *x, PC_TREE *pc_tree, int mi_row,
    int mi_col, BLOCK_SIZE bsize, const RD_STATS *none_rdc,
    int *early_terminate, float *simple_motion_features,
    int *simple_motion_features_are_valid);

// Early terminates after PARTITION_NONE in firstpass of two pass partition
// search.
void av1_firstpass_simple_motion_search_early_term(AV1_COMP *const cpi,
                                                   MACROBLOCK *x,
                                                   PC_TREE *pc_tree, int mi_row,
                                                   int mi_col, BLOCK_SIZE bsize,
                                                   const RD_STATS *none_rdc,
                                                   int *do_square_split);

// Get the features for selecting the max and min partition size. Currently this
// performs simple_motion_search on 16X16 subblocks of the currnet superblock,
// and then extract the statistics of sse and motion vectors as features.
void av1_get_max_min_partition_features(AV1_COMP *const cpi, MACROBLOCK *x,
                                        int mi_row, int mi_col,
                                        float *features);

// Predict the maximum BLOCK_SIZE to be used to encoder the current superblock.
BLOCK_SIZE av1_predict_max_partition(AV1_COMP *const cpi, MACROBLOCK *const x,
                                     const float *features);

// A simplified version of set_offsets meant to be used for
// simple_motion_search.
static INLINE void set_offsets_for_motion_search(const AV1_COMP *const cpi,
                                                 MACROBLOCK *const x,
                                                 int mi_row, int mi_col,
                                                 BLOCK_SIZE bsize) {
  const AV1_COMMON *const cm = &cpi->common;
  const int num_planes = av1_num_planes(cm);
  MACROBLOCKD *const xd = &x->e_mbd;
  const int mi_width = mi_size_wide[bsize];
  const int mi_height = mi_size_high[bsize];

  set_mode_info_offsets(cpi, x, xd, mi_row, mi_col);

  // Set up destination pointers.
  av1_setup_dst_planes(xd->plane, bsize, &cm->cur_frame->buf, mi_row, mi_col, 0,
                       num_planes);

  // Set up limit values for MV components.
  // Mv beyond the range do not produce new/different prediction block.
  x->mv_limits.row_min =
      -(((mi_row + mi_height) * MI_SIZE) + AOM_INTERP_EXTEND);
  x->mv_limits.col_min = -(((mi_col + mi_width) * MI_SIZE) + AOM_INTERP_EXTEND);
  x->mv_limits.row_max = (cm->mi_rows - mi_row) * MI_SIZE + AOM_INTERP_EXTEND;
  x->mv_limits.col_max = (cm->mi_cols - mi_col) * MI_SIZE + AOM_INTERP_EXTEND;

  set_plane_n4(xd, mi_width, mi_height, num_planes);

  // Set up distance of MB to edge of frame in 1/8th pel units.
  assert(!(mi_col & (mi_width - 1)) && !(mi_row & (mi_height - 1)));
  xd->mb_to_top_edge = -((mi_row * MI_SIZE) * 8);
  xd->mb_to_bottom_edge = ((cm->mi_rows - mi_height - mi_row) * MI_SIZE) * 8;
  xd->mb_to_left_edge = -((mi_col * MI_SIZE) * 8);
  xd->mb_to_right_edge = ((cm->mi_cols - mi_width - mi_col) * MI_SIZE) * 8;

  // Set up source buffers.
  av1_setup_src_planes(x, cpi->source, mi_row, mi_col, num_planes, bsize);

  // R/D setup.
  x->rdmult = cpi->rd.RDMULT;
}

static INLINE void init_simple_motion_search_mvs(PC_TREE *pc_tree) {
  for (int idx = 0; idx < REF_FRAMES; idx++) {
    pc_tree->mv_ref_fulls[idx].row = 0;
    pc_tree->mv_ref_fulls[idx].col = 0;
  }
  if (pc_tree->block_size >= BLOCK_8X8) {
    init_simple_motion_search_mvs(pc_tree->split[0]);
    init_simple_motion_search_mvs(pc_tree->split[1]);
    init_simple_motion_search_mvs(pc_tree->split[2]);
    init_simple_motion_search_mvs(pc_tree->split[3]);
  }
}

static INLINE int is_full_sb(AV1_COMMON *const cm, int mi_row, int mi_col,
                             BLOCK_SIZE sb_size) {
  const int sb_mi_wide = mi_size_wide[sb_size];
  const int sb_mi_high = mi_size_high[sb_size];

  return (mi_row + sb_mi_high) <= cm->mi_rows &&
         (mi_col + sb_mi_wide) <= cm->mi_cols;
}

static INLINE int use_auto_max_partition(AV1_COMP *const cpi,
                                         BLOCK_SIZE sb_size, int mi_row,
                                         int mi_col) {
  AV1_COMMON *const cm = &cpi->common;

  return !frame_is_intra_only(cm) &&
         cpi->sf.auto_max_partition_based_on_simple_motion != NOT_IN_USE &&
         sb_size == BLOCK_128X128 && is_full_sb(cm, mi_row, mi_col, sb_size) &&
         cpi->twopass.gf_group.update_type[cpi->twopass.gf_group.index] !=
             OVERLAY_UPDATE &&
         cpi->twopass.gf_group.update_type[cpi->twopass.gf_group.index] !=
             INTNL_OVERLAY_UPDATE;
}

#endif  // AOM_AV1_ENCODER_PARTITION_STRATEGY_H_
