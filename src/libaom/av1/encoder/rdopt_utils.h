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

#ifndef AOM_AV1_ENCODER_RDOPT_UTILS_H_
#define AOM_AV1_ENCODER_RDOPT_UTILS_H_

#include "aom/aom_integer.h"
#include "av1/encoder/block.h"
#include "av1/encoder/rdopt_data_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/* clang-format on */
// Calculate rd threshold based on ref best rd and relevant scaling factors
static INLINE int64_t get_rd_thresh_from_best_rd(int64_t ref_best_rd,
                                                 int mul_factor,
                                                 int div_factor) {
  int64_t rd_thresh = ref_best_rd;
  if (div_factor != 0) {
    rd_thresh = ref_best_rd < (div_factor * (INT64_MAX / mul_factor))
                    ? ((ref_best_rd / div_factor) * mul_factor)
                    : INT64_MAX;
  }
  return rd_thresh;
}

static THR_MODES get_prediction_mode_idx(PREDICTION_MODE this_mode,
                                         MV_REFERENCE_FRAME ref_frame,
                                         MV_REFERENCE_FRAME second_ref_frame) {
  if (this_mode < INTRA_MODE_END) {
    assert(ref_frame == INTRA_FRAME);
    assert(second_ref_frame == NONE_FRAME);
    return intra_to_mode_idx[this_mode - INTRA_MODE_START];
  }
  if (this_mode >= SINGLE_INTER_MODE_START &&
      this_mode < SINGLE_INTER_MODE_END) {
    assert((ref_frame > INTRA_FRAME) && (ref_frame <= ALTREF_FRAME));
    return single_inter_to_mode_idx[this_mode - SINGLE_INTER_MODE_START]
                                   [ref_frame];
  }
  if (this_mode >= COMP_INTER_MODE_START && this_mode < COMP_INTER_MODE_END) {
    assert((ref_frame > INTRA_FRAME) && (ref_frame <= ALTREF_FRAME));
    assert((second_ref_frame > INTRA_FRAME) &&
           (second_ref_frame <= ALTREF_FRAME));
    return comp_inter_to_mode_idx[this_mode - COMP_INTER_MODE_START][ref_frame]
                                 [second_ref_frame];
  }
  assert(0);
  return THR_INVALID;
}

static int inter_mode_data_block_idx(BLOCK_SIZE bsize) {
  if (bsize == BLOCK_4X4 || bsize == BLOCK_4X8 || bsize == BLOCK_8X4 ||
      bsize == BLOCK_4X16 || bsize == BLOCK_16X4) {
    return -1;
  }
  return 1;
}

// Get transform block visible dimensions cropped to the MI units.
static AOM_INLINE void get_txb_dimensions(const MACROBLOCKD *xd, int plane,
                                          BLOCK_SIZE plane_bsize, int blk_row,
                                          int blk_col, BLOCK_SIZE tx_bsize,
                                          int *width, int *height,
                                          int *visible_width,
                                          int *visible_height) {
  assert(tx_bsize <= plane_bsize);
  const int txb_height = block_size_high[tx_bsize];
  const int txb_width = block_size_wide[tx_bsize];
  const struct macroblockd_plane *const pd = &xd->plane[plane];

  // TODO(aconverse@google.com): Investigate using crop_width/height here rather
  // than the MI size
  if (xd->mb_to_bottom_edge >= 0) {
    *visible_height = txb_height;
  } else {
    const int block_height = block_size_high[plane_bsize];
    const int block_rows =
        (xd->mb_to_bottom_edge >> (3 + pd->subsampling_y)) + block_height;
    *visible_height =
        clamp(block_rows - (blk_row << MI_SIZE_LOG2), 0, txb_height);
  }
  if (height) *height = txb_height;

  if (xd->mb_to_right_edge >= 0) {
    *visible_width = txb_width;
  } else {
    const int block_width = block_size_wide[plane_bsize];
    const int block_cols =
        (xd->mb_to_right_edge >> (3 + pd->subsampling_x)) + block_width;
    *visible_width =
        clamp(block_cols - (blk_col << MI_SIZE_LOG2), 0, txb_width);
  }
  if (width) *width = txb_width;
}

static INLINE int bsize_to_num_blk(BLOCK_SIZE bsize) {
  int num_blk = 1 << (num_pels_log2_lookup[bsize] - 2 * MI_SIZE_LOG2);
  return num_blk;
}

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // AOM_AV1_ENCODER_RDOPT_UTILS_H_
