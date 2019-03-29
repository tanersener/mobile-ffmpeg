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

#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>

#include "config/aom_config.h"
#include "config/aom_dsp_rtcd.h"
#include "config/av1_rtcd.h"

#include "aom_dsp/aom_dsp_common.h"
#include "aom_dsp/binary_codes_writer.h"
#include "aom_ports/mem.h"
#include "aom_ports/aom_timer.h"
#include "aom_ports/system_state.h"

#include "av1/common/reconinter.h"
#include "av1/common/blockd.h"

#include "av1/encoder/encodeframe.h"
#include "av1/encoder/var_based_part.h"
#include "av1/encoder/reconinter_enc.h"

extern const uint8_t AV1_VAR_OFFS[];

typedef struct {
  // TODO(kyslov): consider changing to 64bit

  // This struct is used for computing variance in choose_partitioning(), where
  // the max number of samples within a superblock is 32x32 (with 4x4 avg).
  // With 8bit bitdepth, uint32_t is enough for sum_square_error (2^8 * 2^8 * 32
  // * 32 = 2^26). For high bitdepth we need to consider changing this to 64 bit
  uint32_t sum_square_error;
  int32_t sum_error;
  int log2_count;
  int variance;
} var;

typedef struct {
  var none;
  var horz[2];
  var vert[2];
} partition_variance;

typedef struct {
  partition_variance part_variances;
  var split[4];
} v4x4;

typedef struct {
  partition_variance part_variances;
  v4x4 split[4];
} v8x8;

typedef struct {
  partition_variance part_variances;
  v8x8 split[4];
} v16x16;

typedef struct {
  partition_variance part_variances;
  v16x16 split[4];
} v32x32;

typedef struct {
  partition_variance part_variances;
  v32x32 split[4];
} v64x64;

typedef struct {
  partition_variance part_variances;
  v64x64 split[4];
} v128x128;

typedef struct {
  partition_variance *part_variances;
  var *split[4];
} variance_node;

static void tree_to_node(void *data, BLOCK_SIZE bsize, variance_node *node) {
  int i;
  node->part_variances = NULL;
  switch (bsize) {
    case BLOCK_128X128: {
      v128x128 *vt = (v128x128 *)data;
      node->part_variances = &vt->part_variances;
      for (i = 0; i < 4; i++)
        node->split[i] = &vt->split[i].part_variances.none;
      break;
    }
    case BLOCK_64X64: {
      v64x64 *vt = (v64x64 *)data;
      node->part_variances = &vt->part_variances;
      for (i = 0; i < 4; i++)
        node->split[i] = &vt->split[i].part_variances.none;
      break;
    }
    case BLOCK_32X32: {
      v32x32 *vt = (v32x32 *)data;
      node->part_variances = &vt->part_variances;
      for (i = 0; i < 4; i++)
        node->split[i] = &vt->split[i].part_variances.none;
      break;
    }
    case BLOCK_16X16: {
      v16x16 *vt = (v16x16 *)data;
      node->part_variances = &vt->part_variances;
      for (i = 0; i < 4; i++)
        node->split[i] = &vt->split[i].part_variances.none;
      break;
    }
    case BLOCK_8X8: {
      v8x8 *vt = (v8x8 *)data;
      node->part_variances = &vt->part_variances;
      for (i = 0; i < 4; i++)
        node->split[i] = &vt->split[i].part_variances.none;
      break;
    }
    default: {
      v4x4 *vt = (v4x4 *)data;
      assert(bsize == BLOCK_4X4);
      node->part_variances = &vt->part_variances;
      for (i = 0; i < 4; i++) node->split[i] = &vt->split[i];
      break;
    }
  }
}

// Set variance values given sum square error, sum error, count.
static void fill_variance(uint32_t s2, int32_t s, int c, var *v) {
  v->sum_square_error = s2;
  v->sum_error = s;
  v->log2_count = c;
}

static void get_variance(var *v) {
  v->variance =
      (int)(256 * (v->sum_square_error -
                   (uint32_t)(((int64_t)v->sum_error * v->sum_error) >>
                              v->log2_count)) >>
            v->log2_count);
}

static void sum_2_variances(const var *a, const var *b, var *r) {
  assert(a->log2_count == b->log2_count);
  fill_variance(a->sum_square_error + b->sum_square_error,
                a->sum_error + b->sum_error, a->log2_count + 1, r);
}

static void fill_variance_tree(void *data, BLOCK_SIZE bsize) {
  variance_node node;
  memset(&node, 0, sizeof(node));
  tree_to_node(data, bsize, &node);
  sum_2_variances(node.split[0], node.split[1], &node.part_variances->horz[0]);
  sum_2_variances(node.split[2], node.split[3], &node.part_variances->horz[1]);
  sum_2_variances(node.split[0], node.split[2], &node.part_variances->vert[0]);
  sum_2_variances(node.split[1], node.split[3], &node.part_variances->vert[1]);
  sum_2_variances(&node.part_variances->vert[0], &node.part_variances->vert[1],
                  &node.part_variances->none);
}

static void set_block_size(AV1_COMP *const cpi, MACROBLOCK *const x,
                           MACROBLOCKD *const xd, int mi_row, int mi_col,
                           BLOCK_SIZE bsize) {
  if (cpi->common.mi_cols > mi_col && cpi->common.mi_rows > mi_row) {
    set_mode_info_offsets(cpi, x, xd, mi_row, mi_col);
    xd->mi[0]->sb_type = bsize;
  }
}

static int set_vt_partitioning(AV1_COMP *cpi, MACROBLOCK *const x,
                               MACROBLOCKD *const xd,
                               const TileInfo *const tile, void *data,
                               BLOCK_SIZE bsize, int mi_row, int mi_col,
                               int64_t threshold, BLOCK_SIZE bsize_min,
                               int force_split) {
  AV1_COMMON *const cm = &cpi->common;
  variance_node vt;
  const int block_width = mi_size_wide[bsize];
  const int block_height = mi_size_high[bsize];

  assert(block_height == block_width);
  tree_to_node(data, bsize, &vt);

  if (force_split == 1) return 0;

  if (mi_col + block_width > tile->mi_col_end ||
      mi_row + block_height > tile->mi_row_end)
    return 0;

  // For bsize=bsize_min (16x16/8x8 for 8x8/4x4 downsampling), select if
  // variance is below threshold, otherwise split will be selected.
  // No check for vert/horiz split as too few samples for variance.
  if (bsize == bsize_min) {
    // Variance already computed to set the force_split.
    if (frame_is_intra_only(cm)) get_variance(&vt.part_variances->none);
    if (mi_col + block_width / 2 < cm->mi_cols &&
        mi_row + block_height / 2 < cm->mi_rows &&
        vt.part_variances->none.variance < threshold) {
      set_block_size(cpi, x, xd, mi_row, mi_col, bsize);
      return 1;
    }
    return 0;
  } else if (bsize > bsize_min) {
    // Variance already computed to set the force_split.
    if (frame_is_intra_only(cm)) get_variance(&vt.part_variances->none);
    // For key frame: take split for bsize above 32X32 or very high variance.
    if (frame_is_intra_only(cm) &&
        (bsize > BLOCK_32X32 ||
         vt.part_variances->none.variance > (threshold << 4))) {
      return 0;
    }
    // If variance is low, take the bsize (no split).
    if (mi_col + block_width / 2 < cm->mi_cols &&
        mi_row + block_height / 2 < cm->mi_rows &&
        vt.part_variances->none.variance < threshold) {
      set_block_size(cpi, x, xd, mi_row, mi_col, bsize);
      return 1;
    }

    // Check vertical split.
    if (mi_row + block_height / 2 < cm->mi_rows) {
      BLOCK_SIZE subsize = get_partition_subsize(bsize, PARTITION_VERT);
      get_variance(&vt.part_variances->vert[0]);
      get_variance(&vt.part_variances->vert[1]);
      if (vt.part_variances->vert[0].variance < threshold &&
          vt.part_variances->vert[1].variance < threshold &&
          get_plane_block_size(subsize, xd->plane[1].subsampling_x,
                               xd->plane[1].subsampling_y) < BLOCK_INVALID) {
        set_block_size(cpi, x, xd, mi_row, mi_col, subsize);
        set_block_size(cpi, x, xd, mi_row, mi_col + block_width / 2, subsize);
        return 1;
      }
    }
    // Check horizontal split.
    if (mi_col + block_width / 2 < cm->mi_cols) {
      BLOCK_SIZE subsize = get_partition_subsize(bsize, PARTITION_HORZ);
      get_variance(&vt.part_variances->horz[0]);
      get_variance(&vt.part_variances->horz[1]);
      if (vt.part_variances->horz[0].variance < threshold &&
          vt.part_variances->horz[1].variance < threshold &&
          get_plane_block_size(subsize, xd->plane[1].subsampling_x,
                               xd->plane[1].subsampling_y) < BLOCK_INVALID) {
        set_block_size(cpi, x, xd, mi_row, mi_col, subsize);
        set_block_size(cpi, x, xd, mi_row + block_height / 2, mi_col, subsize);
        return 1;
      }
    }

    return 0;
  }
  return 0;
}

static void fill_variance_8x8avg(const uint8_t *s, int sp, const uint8_t *d,
                                 int dp, int x16_idx, int y16_idx, v16x16 *vst,
                                 int pixels_wide, int pixels_high,
                                 int is_key_frame) {
  int k;
  for (k = 0; k < 4; k++) {
    int x8_idx = x16_idx + ((k & 1) << 3);
    int y8_idx = y16_idx + ((k >> 1) << 3);
    unsigned int sse = 0;
    int sum = 0;
    if (x8_idx < pixels_wide && y8_idx < pixels_high) {
      int s_avg;
      int d_avg = 128;
      s_avg = aom_avg_8x8(s + y8_idx * sp + x8_idx, sp);
      if (!is_key_frame) d_avg = aom_avg_8x8(d + y8_idx * dp + x8_idx, dp);

      sum = s_avg - d_avg;
      sse = sum * sum;
    }
    fill_variance(sse, sum, 0, &vst->split[k].part_variances.none);
  }
}

static int compute_minmax_8x8(const uint8_t *s, int sp, const uint8_t *d,
                              int dp, int x16_idx, int y16_idx, int pixels_wide,
                              int pixels_high) {
  int k;
  int minmax_max = 0;
  int minmax_min = 255;
  // Loop over the 4 8x8 subblocks.
  for (k = 0; k < 4; k++) {
    int x8_idx = x16_idx + ((k & 1) << 3);
    int y8_idx = y16_idx + ((k >> 1) << 3);
    int min = 0;
    int max = 0;
    if (x8_idx < pixels_wide && y8_idx < pixels_high) {
      aom_minmax_8x8(s + y8_idx * sp + x8_idx, sp, d + y8_idx * dp + x8_idx, dp,
                     &min, &max);
      if ((max - min) > minmax_max) minmax_max = (max - min);
      if ((max - min) < minmax_min) minmax_min = (max - min);
    }
  }
  return (minmax_max - minmax_min);
}

static void fill_variance_4x4avg(const uint8_t *s, int sp, const uint8_t *d,
                                 int dp, int x8_idx, int y8_idx, v8x8 *vst,
                                 int pixels_wide, int pixels_high,
                                 int is_key_frame) {
  int k;
  for (k = 0; k < 4; k++) {
    int x4_idx = x8_idx + ((k & 1) << 2);
    int y4_idx = y8_idx + ((k >> 1) << 2);
    unsigned int sse = 0;
    int sum = 0;
    if (x4_idx < pixels_wide && y4_idx < pixels_high) {
      int s_avg;
      int d_avg = 128;
      s_avg = aom_avg_4x4(s + y4_idx * sp + x4_idx, sp);
      if (!is_key_frame) d_avg = aom_avg_4x4(d + y4_idx * dp + x4_idx, dp);
      sum = s_avg - d_avg;
      sse = sum * sum;
    }
    fill_variance(sse, sum, 0, &vst->split[k].part_variances.none);
  }
}

static int64_t scale_part_thresh_sumdiff(int64_t threshold_base, int speed,
                                         int width, int height,
                                         int content_state) {
  if (speed >= 8) {
    if (width <= 640 && height <= 480)
      return (5 * threshold_base) >> 2;
    else if ((content_state == kLowSadLowSumdiff) ||
             (content_state == kHighSadLowSumdiff) ||
             (content_state == kLowVarHighSumdiff))
      return (5 * threshold_base) >> 2;
  } else if (speed == 7) {
    if ((content_state == kLowSadLowSumdiff) ||
        (content_state == kHighSadLowSumdiff) ||
        (content_state == kLowVarHighSumdiff)) {
      return (5 * threshold_base) >> 2;
    }
  }
  return threshold_base;
}

// Set the variance split thresholds for following the block sizes:
// 0 - threshold_128x128, 1 - threshold_64x64, 2 - threshold_32x32,
// 3 - vbp_threshold_16x16. 4 - vbp_threshold_8x8 (to split to 4x4 partition) is
// currently only used on key frame.
static void set_vbp_thresholds(AV1_COMP *cpi, int64_t thresholds[], int q,
                               int content_state) {
  AV1_COMMON *const cm = &cpi->common;
  const int is_key_frame = frame_is_intra_only(cm);
  const int threshold_multiplier = is_key_frame ? 40 : 1;
  int64_t threshold_base =
      (int64_t)(threshold_multiplier * cpi->dequants.y_dequant_QTX[q][1]);

  if (is_key_frame) {
    thresholds[0] = threshold_base;
    thresholds[1] = threshold_base;
    thresholds[2] = threshold_base >> 2;
    thresholds[3] = threshold_base >> 2;
    thresholds[4] = threshold_base << 2;
  } else {
    // Increase base variance threshold based on content_state/sum_diff level.
    threshold_base = scale_part_thresh_sumdiff(
        threshold_base, cpi->oxcf.speed, cm->width, cm->height, content_state);

    thresholds[1] = threshold_base;
    thresholds[3] = threshold_base << cpi->oxcf.speed;
    if (cm->width >= 1280 && cm->height >= 720)
      thresholds[3] = thresholds[3] << 1;
    if (cm->width <= 352 && cm->height <= 288) {
      thresholds[1] = threshold_base >> 3;
      thresholds[2] = threshold_base >> 1;
      thresholds[3] = threshold_base << 3;
    } else if (cm->width < 1280 && cm->height < 720) {
      thresholds[2] = (5 * threshold_base) >> 2;
    } else if (cm->width < 1920 && cm->height < 1080) {
      thresholds[2] = threshold_base << 1;
      thresholds[3] <<= 2;
    } else {
      thresholds[2] = (5 * threshold_base) >> 1;
    }
  }
}

void av1_set_variance_partition_thresholds(AV1_COMP *cpi, int q,
                                           int content_state) {
  AV1_COMMON *const cm = &cpi->common;
  SPEED_FEATURES *const sf = &cpi->sf;
  const int is_key_frame = frame_is_intra_only(cm);
  if (sf->partition_search_type != VAR_BASED_PARTITION) {
    return;
  } else {
    set_vbp_thresholds(cpi, cpi->vbp_thresholds, q, content_state);
    // The thresholds below are not changed locally.
    if (is_key_frame) {
      cpi->vbp_threshold_sad = 0;
      cpi->vbp_threshold_copy = 0;
      cpi->vbp_bsize_min = BLOCK_8X8;
    } else {
      if (cm->width <= 352 && cm->height <= 288)
        cpi->vbp_threshold_sad = 10;
      else
        cpi->vbp_threshold_sad = (cpi->dequants.y_dequant_QTX[q][1] << 1) > 1000
                                     ? (cpi->dequants.y_dequant_QTX[q][1] << 1)
                                     : 1000;
      cpi->vbp_bsize_min = BLOCK_16X16;
      if (cm->width <= 352 && cm->height <= 288)
        cpi->vbp_threshold_copy = 4000;
      else if (cm->width <= 640 && cm->height <= 360)
        cpi->vbp_threshold_copy = 8000;
      else
        cpi->vbp_threshold_copy =
            (cpi->dequants.y_dequant_QTX[q][1] << 3) > 8000
                ? (cpi->dequants.y_dequant_QTX[q][1] << 3)
                : 8000;
    }
    cpi->vbp_threshold_minmax = 15 + (q >> 3);
  }
}

// This function chooses partitioning based on the variance between source and
// reconstructed last, where variance is computed for down-sampled inputs.
// TODO(kyslov): lot of things. Bring back noise estimation, brush up partition
// selection and most of all - retune the thresholds
int av1_choose_var_based_partitioning(AV1_COMP *cpi, const TileInfo *const tile,
                                      MACROBLOCK *x, int mi_row, int mi_col) {
  AV1_COMMON *const cm = &cpi->common;
  MACROBLOCKD *xd = &x->e_mbd;

  int i, j, k, m;
  v128x128 *vt;
  v16x16 *vt2 = NULL;
  unsigned char force_split[85];
  int avg_32x32;
  int max_var_32x32 = 0;
  int min_var_32x32 = INT_MAX;
  int var_32x32;
  int var_64x64;
  int min_var_64x64 = INT_MAX;
  int max_var_64x64 = 0;
  int avg_16x16[4];
  int maxvar_16x16[4];
  int minvar_16x16[4];
  int64_t threshold_4x4avg;
  int content_state = 0;
  uint8_t *s;
  const uint8_t *d;
  int sp;
  int dp;
  int compute_minmax_variance = 1;
  int is_key_frame = frame_is_intra_only(cm);
  int pixels_wide = 128, pixels_high = 128;
  assert(cm->seq_params.sb_size == BLOCK_64X64 ||
         cm->seq_params.sb_size == BLOCK_128X128);
  const int is_small_sb = (cm->seq_params.sb_size == BLOCK_64X64);
  const int num_64x64_blocks = is_small_sb ? 1 : 4;

  CHECK_MEM_ERROR(cm, vt, aom_calloc(1, sizeof(*vt)));

  int64_t thresholds[5] = { cpi->vbp_thresholds[0], cpi->vbp_thresholds[1],
                            cpi->vbp_thresholds[2], cpi->vbp_thresholds[3],
                            cpi->vbp_thresholds[4] };

  const int low_res = (cm->width <= 352 && cm->height <= 288);
  int variance4x4downsample[64];
  int segment_id;
  const int num_planes = av1_num_planes(cm);

  segment_id = xd->mi[0]->segment_id;

  set_vbp_thresholds(cpi, thresholds, cm->base_qindex, content_state);

  if (is_small_sb) {
    pixels_wide = 64;
    pixels_high = 64;
  }

  // For non keyframes, disable 4x4 average for low resolution when speed = 8
  threshold_4x4avg = INT64_MAX;

  if (xd->mb_to_right_edge < 0) pixels_wide += (xd->mb_to_right_edge >> 3);
  if (xd->mb_to_bottom_edge < 0) pixels_high += (xd->mb_to_bottom_edge >> 3);

  s = x->plane[0].src.buf;
  sp = x->plane[0].src.stride;

  // Index for force_split: 0 for 64x64, 1-4 for 32x32 blocks,
  // 5-20 for the 16x16 blocks.
  force_split[0] = 0;

  if (!is_key_frame) {
    // TODO(kyslov): we are assuming that the ref is LAST_FRAME! Check if it
    // is!!
    MB_MODE_INFO *mi = xd->mi[0];
    const YV12_BUFFER_CONFIG *yv12 = get_ref_frame_yv12_buf(cm, LAST_FRAME);

    assert(yv12 != NULL);

    av1_setup_pre_planes(xd, 0, yv12, mi_row, mi_col,
                         get_ref_scale_factors(cm, LAST_FRAME), num_planes);
    mi->ref_frame[0] = LAST_FRAME;
    mi->ref_frame[1] = NONE_FRAME;
    mi->sb_type = cm->seq_params.sb_size;
    mi->mv[0].as_int = 0;
    mi->interp_filters = av1_make_interp_filters(BILINEAR, BILINEAR);
    if (xd->mb_to_right_edge >= 0 && xd->mb_to_bottom_edge >= 0) {
      const MV dummy_mv = { 0, 0 };
      av1_int_pro_motion_estimation(cpi, x, cm->seq_params.sb_size, mi_row,
                                    mi_col, &dummy_mv);
    }

// TODO(kyslov): bring the small SAD functionality back
#if 0
    y_sad = cpi->fn_ptr[bsize].sdf(x->plane[0].src.buf, x->plane[0].src.stride,
                                   xd->plane[0].pre[0].buf,
                                   xd->plane[0].pre[0].stride);
#endif
    x->pred_mv[LAST_FRAME] = mi->mv[0].as_mv;

    set_ref_ptrs(cm, xd, mi->ref_frame[0], mi->ref_frame[1]);
    av1_enc_build_inter_predictor(cm, xd, mi_row, mi_col, NULL,
                                  cm->seq_params.sb_size, AOM_PLANE_Y,
                                  AOM_PLANE_Y);

    d = xd->plane[0].dst.buf;
    dp = xd->plane[0].dst.stride;

    // If the y_sad is very small, take 64x64 as partition and exit.
    // Don't check on boosted segment for now, as 64x64 is suppressed there.
#if 0
        if (segment_id == CR_SEGMENT_ID_BASE && y_sad < cpi->vbp_threshold_sad)
       { const int block_width = num_8x8_blocks_wide_lookup[BLOCK_64X64]; const
       int block_height = num_8x8_blocks_high_lookup[BLOCK_64X64]; if (mi_col +
       block_width / 2 < cm->mi_cols && mi_row + block_height / 2 < cm->mi_rows)
       { set_block_size(cpi, x, xd, mi_row, mi_col, BLOCK_128X128);
            x->variance_low[0] = 1;
            return 0;
          }
        }
#endif
  } else {
    d = AV1_VAR_OFFS;
    dp = 0;
  }

  if (low_res && threshold_4x4avg < INT64_MAX)
    CHECK_MEM_ERROR(cm, vt2, aom_calloc(64, sizeof(*vt2)));
  // Fill in the entire tree of 8x8 (or 4x4 under some conditions) variances
  // for splits.
  for (m = 0; m < num_64x64_blocks; m++) {
    const int x64_idx = ((m & 1) << 6);
    const int y64_idx = ((m >> 1) << 6);
    const int m2 = m << 2;
    force_split[m + 1] = 0;
    for (i = 0; i < 4; i++) {
      const int x32_idx = x64_idx + ((i & 1) << 5);
      const int y32_idx = y64_idx + ((i >> 1) << 5);
      const int i2 = (m2 + i) << 2;
      force_split[5 + m2 + i] = 0;
      avg_16x16[i] = 0;
      maxvar_16x16[i] = 0;
      minvar_16x16[i] = INT_MAX;
      for (j = 0; j < 4; j++) {
        const int x16_idx = x32_idx + ((j & 1) << 4);
        const int y16_idx = y32_idx + ((j >> 1) << 4);
        const int split_index = 21 + i2 + j;
        v16x16 *vst = &vt->split[m].split[i].split[j];
        force_split[split_index] = 0;
        variance4x4downsample[i2 + j] = 0;
        if (!is_key_frame) {
          fill_variance_8x8avg(s, sp, d, dp, x16_idx, y16_idx, vst, pixels_wide,
                               pixels_high, is_key_frame);
          fill_variance_tree(&vt->split[m].split[i].split[j], BLOCK_16X16);
          get_variance(&vt->split[m].split[i].split[j].part_variances.none);
          avg_16x16[i] +=
              vt->split[m].split[i].split[j].part_variances.none.variance;
          if (vt->split[m].split[i].split[j].part_variances.none.variance <
              minvar_16x16[i])
            minvar_16x16[i] =
                vt->split[m].split[i].split[j].part_variances.none.variance;
          if (vt->split[m].split[i].split[j].part_variances.none.variance >
              maxvar_16x16[i])
            maxvar_16x16[i] =
                vt->split[m].split[i].split[j].part_variances.none.variance;
          if (vt->split[m].split[i].split[j].part_variances.none.variance >
              thresholds[3]) {
            // 16X16 variance is above threshold for split, so force split to
            // 8x8 for this 16x16 block (this also forces splits for upper
            // levels).
            force_split[split_index] = 1;
            force_split[5 + m2 + i] = 1;
            force_split[m + 1] = 1;
            force_split[0] = 1;
          } else if (compute_minmax_variance &&
                     vt->split[m]
                             .split[i]
                             .split[j]
                             .part_variances.none.variance > thresholds[2] &&
                     !cyclic_refresh_segment_id_boosted(segment_id)) {
            // We have some nominal amount of 16x16 variance (based on average),
            // compute the minmax over the 8x8 sub-blocks, and if above
            // threshold, force split to 8x8 block for this 16x16 block.
            int minmax = compute_minmax_8x8(s, sp, d, dp, x16_idx, y16_idx,
                                            pixels_wide, pixels_high);
            int thresh_minmax = (int)cpi->vbp_threshold_minmax;
            if (minmax > thresh_minmax) {
              force_split[split_index] = 1;
              force_split[5 + m2 + i] = 1;
              force_split[m + 1] = 1;
              force_split[0] = 1;
            }
          }
        }
        if (is_key_frame) {
          force_split[split_index] = 0;
          // Go down to 4x4 down-sampling for variance.
          variance4x4downsample[i2 + j] = 1;
          for (k = 0; k < 4; k++) {
            int x8_idx = x16_idx + ((k & 1) << 3);
            int y8_idx = y16_idx + ((k >> 1) << 3);
            v8x8 *vst2 = is_key_frame ? &vst->split[k] : &vt2[i2 + j].split[k];
            fill_variance_4x4avg(s, sp, d, dp, x8_idx, y8_idx, vst2,
                                 pixels_wide, pixels_high, is_key_frame);
          }
        }
      }
    }
  }

  // Fill the rest of the variance tree by summing split partition values.
  for (m = 0; m < num_64x64_blocks; ++m) {
    avg_32x32 = 0;
    const int m2 = m << 2;
    for (i = 0; i < 4; i++) {
      const int i2 = (m2 + i) << 2;
      for (j = 0; j < 4; j++) {
        const int split_index = 21 + i2 + j;
        if (variance4x4downsample[i2 + j] == 1) {
          v16x16 *vtemp =
              (!is_key_frame) ? &vt2[i2 + j] : &vt->split[m].split[i].split[j];
          for (k = 0; k < 4; k++)
            fill_variance_tree(&vtemp->split[k], BLOCK_8X8);
          fill_variance_tree(vtemp, BLOCK_16X16);
          // If variance of this 16x16 block is above the threshold, force block
          // to split. This also forces a split on the upper levels.
          get_variance(&vtemp->part_variances.none);
          if (vtemp->part_variances.none.variance > thresholds[3]) {
            force_split[split_index] = 1;
            force_split[5 + m2 + i] = 1;
            force_split[m + 1] = 1;
            force_split[0] = 1;
          }
        }
      }
      fill_variance_tree(&vt->split[m].split[i], BLOCK_32X32);
      // If variance of this 32x32 block is above the threshold, or if its above
      // (some threshold of) the average variance over the sub-16x16 blocks,
      // then force this block to split. This also forces a split on the upper
      // (64x64) level.
      if (!force_split[5 + m2 + i]) {
        get_variance(&vt->split[m].split[i].part_variances.none);
        var_32x32 = vt->split[m].split[i].part_variances.none.variance;
        max_var_32x32 = AOMMAX(var_32x32, max_var_32x32);
        min_var_32x32 = AOMMIN(var_32x32, min_var_32x32);
        if (vt->split[m].split[i].part_variances.none.variance >
                thresholds[2] ||
            (!is_key_frame &&
             vt->split[m].split[i].part_variances.none.variance >
                 (thresholds[2] >> 1) &&
             vt->split[m].split[i].part_variances.none.variance >
                 (avg_16x16[i] >> 1))) {
          force_split[5 + m2 + i] = 1;
          force_split[m + 1] = 1;
          force_split[0] = 1;
        } else if (!is_key_frame && cm->height <= 360 &&
                   (maxvar_16x16[i] - minvar_16x16[i]) > (thresholds[2] >> 1) &&
                   maxvar_16x16[i] > thresholds[2]) {
          force_split[5 + m2 + i] = 1;
          force_split[m + 1] = 1;
          force_split[0] = 1;
        }
        avg_32x32 += var_32x32;
      }
    }
    if (!force_split[1 + m]) {
      fill_variance_tree(&vt->split[m], BLOCK_64X64);
      get_variance(&vt->split[m].part_variances.none);
      var_64x64 = vt->split[m].part_variances.none.variance;
      max_var_64x64 = AOMMAX(var_64x64, max_var_64x64);
      min_var_64x64 = AOMMIN(var_64x64, min_var_64x64);
      // If variance of this 64x64 block is above (some threshold of) the
      // average variance over the sub-32x32 blocks, then force this block to
      // split. Only checking this for noise level >= medium for now.

      if (!is_key_frame &&
          (max_var_32x32 - min_var_32x32) > 3 * (thresholds[1] >> 3) &&
          max_var_32x32 > thresholds[1] >> 1)
        force_split[1 + m] = 1;
    }
    if (is_small_sb) force_split[0] = 1;
  }

  if (!force_split[0]) {
    fill_variance_tree(vt, BLOCK_128X128);
    get_variance(&vt->part_variances.none);
    if (!is_key_frame &&
        (max_var_64x64 - min_var_64x64) > 3 * (thresholds[0] >> 3) &&
        max_var_64x64 > thresholds[0] >> 1)
      force_split[0] = 1;
  }

  if (!set_vt_partitioning(cpi, x, xd, tile, vt, BLOCK_128X128, mi_row, mi_col,
                           thresholds[0], BLOCK_16X16, force_split[0])) {
    for (m = 0; m < num_64x64_blocks; ++m) {
      const int x64_idx = ((m & 1) << 4);
      const int y64_idx = ((m >> 1) << 4);
      const int m2 = m << 2;

      // Now go through the entire structure, splitting every block size until
      // we get to one that's got a variance lower than our threshold.
      if (!set_vt_partitioning(cpi, x, xd, tile, &vt->split[m], BLOCK_64X64,
                               mi_row + y64_idx, mi_col + x64_idx,
                               thresholds[1], BLOCK_16X16,
                               force_split[1 + m])) {
        for (i = 0; i < 4; ++i) {
          const int x32_idx = ((i & 1) << 3);
          const int y32_idx = ((i >> 1) << 3);
          const int i2 = (m2 + i) << 2;
          if (!set_vt_partitioning(cpi, x, xd, tile, &vt->split[m].split[i],
                                   BLOCK_32X32, (mi_row + y64_idx + y32_idx),
                                   (mi_col + x64_idx + x32_idx), thresholds[2],
                                   BLOCK_16X16, force_split[5 + m2 + i])) {
            for (j = 0; j < 4; ++j) {
              const int x16_idx = ((j & 1) << 2);
              const int y16_idx = ((j >> 1) << 2);
              const int split_index = 21 + i2 + j;
              // For inter frames: if variance4x4downsample[] == 1 for this
              // 16x16 block, then the variance is based on 4x4 down-sampling,
              // so use vt2 in set_vt_partioning(), otherwise use vt.
              v16x16 *vtemp =
                  (!is_key_frame && variance4x4downsample[i2 + j] == 1)
                      ? &vt2[i2 + j]
                      : &vt->split[m].split[i].split[j];
              if (!set_vt_partitioning(cpi, x, xd, tile, vtemp, BLOCK_16X16,
                                       mi_row + y64_idx + y32_idx + y16_idx,
                                       mi_col + x64_idx + x32_idx + x16_idx,
                                       thresholds[3], BLOCK_8X8,
                                       force_split[split_index])) {
                for (k = 0; k < 4; ++k) {
                  const int x8_idx = (k & 1) << 1;
                  const int y8_idx = (k >> 1) << 1;
                  set_block_size(
                      cpi, x, xd,
                      (mi_row + y64_idx + y32_idx + y16_idx + y8_idx),
                      (mi_col + x64_idx + x32_idx + x16_idx + x8_idx),
                      BLOCK_8X8);
                }
              }
            }
          }
        }
      }
    }
  }

  if (vt2) aom_free(vt2);
  if (vt) aom_free(vt);
  return 0;
}
