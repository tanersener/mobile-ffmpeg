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

#ifndef AOM_AV1_ENCODER_RDOPT_H_
#define AOM_AV1_ENCODER_RDOPT_H_

#include <stdbool.h>

#include "av1/common/blockd.h"
#include "av1/common/txb_common.h"

#include "av1/encoder/block.h"
#include "av1/encoder/context_tree.h"
#include "av1/encoder/encoder.h"
#include "av1/encoder/encodetxb.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_REF_MV_SEARCH 3
#define INTER_INTRA_RD_THRESH_SCALE 9
#define INTER_INTRA_RD_THRESH_SHIFT 4
#define COMP_TYPE_RD_THRESH_SCALE 11
#define COMP_TYPE_RD_THRESH_SHIFT 4

struct TileInfo;
struct macroblock;
struct RD_STATS;

#if CONFIG_RD_DEBUG
static INLINE void av1_update_txb_coeff_cost(RD_STATS *rd_stats, int plane,
                                             TX_SIZE tx_size, int blk_row,
                                             int blk_col, int txb_coeff_cost) {
  (void)blk_row;
  (void)blk_col;
  (void)tx_size;
  rd_stats->txb_coeff_cost[plane] += txb_coeff_cost;

  {
    const int txb_h = tx_size_high_unit[tx_size];
    const int txb_w = tx_size_wide_unit[tx_size];
    int idx, idy;
    for (idy = 0; idy < txb_h; ++idy)
      for (idx = 0; idx < txb_w; ++idx)
        rd_stats->txb_coeff_cost_map[plane][blk_row + idy][blk_col + idx] = 0;

    rd_stats->txb_coeff_cost_map[plane][blk_row][blk_col] = txb_coeff_cost;
  }
  assert(blk_row < TXB_COEFF_COST_MAP_SIZE);
  assert(blk_col < TXB_COEFF_COST_MAP_SIZE);
}
#endif

// Returns the number of colors in 'src'.
int av1_count_colors(const uint8_t *src, int stride, int rows, int cols,
                     int *val_count);
// Same as av1_count_colors(), but for high-bitdepth mode.
int av1_count_colors_highbd(const uint8_t *src8, int stride, int rows, int cols,
                            int bit_depth, int *val_count);

#if CONFIG_DIST_8X8
int64_t av1_dist_8x8(const struct AV1_COMP *const cpi, const MACROBLOCK *x,
                     const uint8_t *src, int src_stride, const uint8_t *dst,
                     int dst_stride, const BLOCK_SIZE tx_bsize, int bsw,
                     int bsh, int visible_w, int visible_h, int qindex);
#endif

static INLINE int av1_cost_skip_txb(MACROBLOCK *x, const TXB_CTX *const txb_ctx,
                                    int plane, TX_SIZE tx_size) {
  const TX_SIZE txs_ctx = get_txsize_entropy_ctx(tx_size);
  const PLANE_TYPE plane_type = get_plane_type(plane);
  const LV_MAP_COEFF_COST *const coeff_costs =
      &x->coeff_costs[txs_ctx][plane_type];
  return coeff_costs->txb_skip_cost[txb_ctx->txb_skip_ctx][1];
}

static INLINE int av1_cost_coeffs(const AV1_COMMON *const cm, MACROBLOCK *x,
                                  int plane, int block, TX_SIZE tx_size,
                                  const TX_TYPE tx_type,
                                  const TXB_CTX *const txb_ctx,
                                  int use_fast_coef_costing) {
#if TXCOEFF_COST_TIMER
  struct aom_usec_timer timer;
  aom_usec_timer_start(&timer);
#endif
  (void)use_fast_coef_costing;
  const int cost =
      av1_cost_coeffs_txb(cm, x, plane, block, tx_size, tx_type, txb_ctx);
#if TXCOEFF_COST_TIMER
  AV1_COMMON *tmp_cm = (AV1_COMMON *)&cpi->common;
  aom_usec_timer_mark(&timer);
  const int64_t elapsed_time = aom_usec_timer_elapsed(&timer);
  tmp_cm->txcoeff_cost_timer += elapsed_time;
  ++tmp_cm->txcoeff_cost_count;
#endif
  return cost;
}

void av1_rd_pick_intra_mode_sb(const struct AV1_COMP *cpi, struct macroblock *x,
                               int mi_row, int mi_col, struct RD_STATS *rd_cost,
                               BLOCK_SIZE bsize, PICK_MODE_CONTEXT *ctx,
                               int64_t best_rd);

unsigned int av1_get_sby_perpixel_variance(const struct AV1_COMP *cpi,
                                           const struct buf_2d *ref,
                                           BLOCK_SIZE bs);
unsigned int av1_high_get_sby_perpixel_variance(const struct AV1_COMP *cpi,
                                                const struct buf_2d *ref,
                                                BLOCK_SIZE bs, int bd);

void av1_rd_pick_inter_mode_sb(struct AV1_COMP *cpi,
                               struct TileDataEnc *tile_data,
                               struct macroblock *x, int mi_row, int mi_col,
                               struct RD_STATS *rd_cost, BLOCK_SIZE bsize,
                               PICK_MODE_CONTEXT *ctx, int64_t best_rd_so_far);

void av1_fast_nonrd_pick_inter_mode_sb(struct AV1_COMP *cpi,
                                       struct TileDataEnc *tile_data,
                                       struct macroblock *x, int mi_row,
                                       int mi_col, struct RD_STATS *rd_cost,
                                       BLOCK_SIZE bsize, PICK_MODE_CONTEXT *ctx,
                                       int64_t best_rd_so_far);

void av1_nonrd_pick_inter_mode_sb(struct AV1_COMP *cpi,
                                  struct TileDataEnc *tile_data,
                                  struct macroblock *x, int mi_row, int mi_col,
                                  struct RD_STATS *rd_cost, BLOCK_SIZE bsize,
                                  PICK_MODE_CONTEXT *ctx,
                                  int64_t best_rd_so_far);

void av1_rd_pick_inter_mode_sb_seg_skip(
    const struct AV1_COMP *cpi, struct TileDataEnc *tile_data,
    struct macroblock *x, int mi_row, int mi_col, struct RD_STATS *rd_cost,
    BLOCK_SIZE bsize, PICK_MODE_CONTEXT *ctx, int64_t best_rd_so_far);

// The best edge strength seen in the block, as well as the best x and y
// components of edge strength seen.
typedef struct {
  uint16_t magnitude;
  uint16_t x;
  uint16_t y;
} EdgeInfo;

/** Returns an integer indicating the strength of the edge.
 * 0 means no edge found, 556 is the strength of a solid black/white edge,
 * and the number may range higher if the signal is even stronger (e.g., on a
 * corner). high_bd is a bool indicating the source should be treated
 * as a 16-bit array. bd is the bit depth.
 */
EdgeInfo av1_edge_exists(const uint8_t *src, int src_stride, int w, int h,
                         bool high_bd, int bd);

/** Applies a Gaussian blur with sigma = 1.3. Used by av1_edge_exists and
 * tests.
 */
void av1_gaussian_blur(const uint8_t *src, int src_stride, int w, int h,
                       uint8_t *dst, bool high_bd, int bd);

/* Applies standard 3x3 Sobel matrix. */
typedef struct {
  int16_t x;
  int16_t y;
} sobel_xy;

sobel_xy av1_sobel(const uint8_t *input, int stride, int i, int j,
                   bool high_bd);

void av1_inter_mode_data_init(struct TileDataEnc *tile_data);
void av1_inter_mode_data_fit(TileDataEnc *tile_data, int rdmult);

typedef int64_t (*pick_interinter_mask_type)(
    const AV1_COMP *const cpi, MACROBLOCK *x, const BLOCK_SIZE bsize,
    const uint8_t *const p0, const uint8_t *const p1,
    const int16_t *const residual1, const int16_t *const diff10);

static INLINE int av1_encoder_get_relative_dist(const OrderHintInfo *oh, int a,
                                                int b) {
  if (!oh->enable_order_hint) return 0;

  assert(a >= 0 && b >= 0);
  return (a - b);
}

// This function will return number of mi's in a superblock.
static INLINE int av1_get_sb_mi_size(const AV1_COMMON *const cm) {
  const int mi_alloc_size_1d = mi_size_wide[cm->mi_alloc_bsize];
  int sb_mi_rows =
      (mi_size_wide[cm->seq_params.sb_size] + mi_alloc_size_1d - 1) /
      mi_alloc_size_1d;
  assert(mi_size_wide[cm->seq_params.sb_size] ==
         mi_size_high[cm->seq_params.sb_size]);
  int sb_mi_size = sb_mi_rows * sb_mi_rows;

  return sb_mi_size;
}

// This function will copy usable ref_mv_stack[ref_frame][4] and
// weight[ref_frame][4] information from ref_mv_stack[ref_frame][8] and
// weight[ref_frame][8].
static INLINE void av1_copy_usable_ref_mv_stack_and_weight(
    const MACROBLOCKD *xd, MB_MODE_INFO_EXT *const mbmi_ext,
    MV_REFERENCE_FRAME ref_frame) {
  memcpy(mbmi_ext->weight[ref_frame], xd->weight[ref_frame],
         USABLE_REF_MV_STACK_SIZE * sizeof(xd->weight[0][0]));
  memcpy(mbmi_ext->ref_mv_stack[ref_frame], xd->ref_mv_stack[ref_frame],
         USABLE_REF_MV_STACK_SIZE * sizeof(xd->ref_mv_stack[0][0]));
}

static TX_MODE select_tx_mode(
    const AV1_COMP *cpi, const TX_SIZE_SEARCH_METHOD tx_size_search_method) {
  if (cpi->common.coded_lossless) return ONLY_4X4;
  if (tx_size_search_method == USE_LARGESTALL)
    return TX_MODE_LARGEST;
  else if (tx_size_search_method == USE_FULL_RD ||
           tx_size_search_method == USE_FAST_RD)
    return TX_MODE_SELECT;
  else
    return cpi->common.tx_mode;
}

static INLINE TX_MODE get_eval_tx_mode(const AV1_COMP *cpi,
                                       MODE_EVAL_TYPE eval_type) {
  TX_MODE tx_mode;
  if (cpi->sf.enable_winner_mode_for_tx_size_srch)
    tx_mode = select_tx_mode(cpi, cpi->tx_size_search_methods[eval_type]);
  else
    tx_mode = select_tx_mode(cpi, cpi->tx_size_search_methods[DEFAULT_EVAL]);

  return tx_mode;
}

static INLINE void set_tx_size_search_method(
    const struct AV1_COMP *cpi, MACROBLOCK *x,
    int enable_winner_mode_for_tx_size_srch, int is_winner_mode) {
  // Populate transform size search method/transform mode appropriately
  x->tx_size_search_method = cpi->tx_size_search_methods[DEFAULT_EVAL];
  if (enable_winner_mode_for_tx_size_srch) {
    if (is_winner_mode)
      x->tx_size_search_method = cpi->tx_size_search_methods[WINNER_MODE_EVAL];
    else
      x->tx_size_search_method = cpi->tx_size_search_methods[MODE_EVAL];
  }
  x->tx_mode = select_tx_mode(cpi, x->tx_size_search_method);
}

static INLINE void set_tx_domain_dist_params(
    const struct AV1_COMP *cpi, MACROBLOCK *x,
    int enable_winner_mode_for_tx_domain_dist, int is_winner_mode) {
  if (!enable_winner_mode_for_tx_domain_dist) {
    x->use_transform_domain_distortion =
        cpi->use_transform_domain_distortion[DEFAULT_EVAL];
    x->tx_domain_dist_threshold = cpi->tx_domain_dist_threshold[DEFAULT_EVAL];
    return;
  }

  if (is_winner_mode) {
    x->use_transform_domain_distortion =
        cpi->use_transform_domain_distortion[WINNER_MODE_EVAL];
    x->tx_domain_dist_threshold =
        cpi->tx_domain_dist_threshold[WINNER_MODE_EVAL];
  } else {
    x->use_transform_domain_distortion =
        cpi->use_transform_domain_distortion[MODE_EVAL];
    x->tx_domain_dist_threshold = cpi->tx_domain_dist_threshold[MODE_EVAL];
  }
}

// Checks the conditions to enable winner mode processing
static INLINE int is_winner_mode_processing_enabled(
    const struct AV1_COMP *cpi, MB_MODE_INFO *const mbmi,
    const PREDICTION_MODE best_mode) {
  const SPEED_FEATURES *sf = &cpi->sf;

  // TODO(any): Move block independent condition checks to frame level
  if (is_inter_block(mbmi)) {
    if (is_inter_mode(best_mode) &&
        sf->tx_type_search.fast_inter_tx_type_search &&
        !cpi->oxcf.use_inter_dct_only)
      return 1;
  } else {
    if (sf->tx_type_search.fast_intra_tx_type_search &&
        !cpi->oxcf.use_intra_default_tx_only && !cpi->oxcf.use_intra_dct_only)
      return 1;
  }

  // Check speed feature related to winner mode processing
  if (sf->enable_winner_mode_for_coeff_opt &&
      cpi->optimize_seg_arr[mbmi->segment_id] != NO_TRELLIS_OPT &&
      cpi->optimize_seg_arr[mbmi->segment_id] != FINAL_PASS_TRELLIS_OPT)
    return 1;
  if (sf->enable_winner_mode_for_tx_size_srch) return 1;

  return 0;
}

// This function sets mode parameters for different mode evaluation stages
static INLINE void set_mode_eval_params(const struct AV1_COMP *cpi,
                                        MACROBLOCK *x,
                                        MODE_EVAL_TYPE mode_eval_type) {
  const SPEED_FEATURES *sf = &cpi->sf;

  switch (mode_eval_type) {
    case DEFAULT_EVAL:
      x->use_default_inter_tx_type = 0;
      x->use_default_intra_tx_type = 0;
      // Set default transform domain distortion type
      set_tx_domain_dist_params(cpi, x, 0, 0);

      // Get default threshold for R-D optimization of coefficients
      x->coeff_opt_dist_threshold =
          get_rd_opt_coeff_thresh(cpi->coeff_opt_dist_threshold, 0, 0);
      // Set default transform size search method
      set_tx_size_search_method(cpi, x, 0, 0);
      break;
    case MODE_EVAL:
      x->use_default_intra_tx_type =
          (cpi->sf.tx_type_search.fast_intra_tx_type_search ||
           cpi->oxcf.use_intra_default_tx_only);
      x->use_default_inter_tx_type =
          cpi->sf.tx_type_search.fast_inter_tx_type_search;

      // Set transform domain distortion type for mode evaluation
      set_tx_domain_dist_params(
          cpi, x, sf->enable_winner_mode_for_use_tx_domain_dist, 0);

      // Get threshold for R-D optimization of coefficients during mode
      // evaluation
      x->coeff_opt_dist_threshold =
          get_rd_opt_coeff_thresh(cpi->coeff_opt_dist_threshold,
                                  sf->enable_winner_mode_for_coeff_opt, 0);
      // Set the transform size search method for mode evaluation
      set_tx_size_search_method(cpi, x, sf->enable_winner_mode_for_tx_size_srch,
                                0);
      break;
    case WINNER_MODE_EVAL:
      x->use_default_inter_tx_type = 0;
      x->use_default_intra_tx_type = 0;

      // Set transform domain distortion type for winner mode evaluation
      set_tx_domain_dist_params(
          cpi, x, sf->enable_winner_mode_for_use_tx_domain_dist, 1);

      // Get threshold for R-D optimization of coefficients for winner mode
      // evaluation
      x->coeff_opt_dist_threshold =
          get_rd_opt_coeff_thresh(cpi->coeff_opt_dist_threshold,
                                  sf->enable_winner_mode_for_coeff_opt, 1);
      // Set the transform size search method for winner mode evaluation
      set_tx_size_search_method(cpi, x, sf->enable_winner_mode_for_tx_size_srch,
                                1);
      break;
    default: assert(0);
  }
}

static INLINE int prune_ref_by_selective_ref_frame(
    const AV1_COMP *const cpi, const MV_REFERENCE_FRAME *const ref_frame,
    const unsigned int *const ref_display_order_hint,
    const unsigned int cur_frame_display_order_hint) {
  const SPEED_FEATURES *const sf = &cpi->sf;
  if (sf->selective_ref_frame) {
    const AV1_COMMON *const cm = &cpi->common;
    const OrderHintInfo *const order_hint_info =
        &cm->seq_params.order_hint_info;
    const int comp_pred = ref_frame[1] > INTRA_FRAME;
    if (sf->selective_ref_frame >= 2 ||
        (sf->selective_ref_frame == 1 && comp_pred)) {
      if (ref_frame[0] == LAST3_FRAME || ref_frame[1] == LAST3_FRAME) {
        if (av1_encoder_get_relative_dist(
                order_hint_info,
                ref_display_order_hint[LAST3_FRAME - LAST_FRAME],
                ref_display_order_hint[GOLDEN_FRAME - LAST_FRAME]) <= 0)
          return 1;
      }
      if (ref_frame[0] == LAST2_FRAME || ref_frame[1] == LAST2_FRAME) {
        if (av1_encoder_get_relative_dist(
                order_hint_info,
                ref_display_order_hint[LAST2_FRAME - LAST_FRAME],
                ref_display_order_hint[GOLDEN_FRAME - LAST_FRAME]) <= 0)
          return 1;
      }
    }

    // One-sided compound is used only when all reference frames are one-sided.
    if (sf->selective_ref_frame >= 2 && comp_pred && !cpi->all_one_sided_refs) {
      unsigned int ref_offsets[2];
      for (int i = 0; i < 2; ++i) {
        const RefCntBuffer *const buf = get_ref_frame_buf(cm, ref_frame[i]);
        assert(buf != NULL);
        ref_offsets[i] = buf->display_order_hint;
      }
      const int ref0_dist = av1_encoder_get_relative_dist(
          order_hint_info, ref_offsets[0], cur_frame_display_order_hint);
      const int ref1_dist = av1_encoder_get_relative_dist(
          order_hint_info, ref_offsets[1], cur_frame_display_order_hint);
      if ((ref0_dist <= 0 && ref1_dist <= 0) ||
          (ref0_dist > 0 && ref1_dist > 0)) {
        return 1;
      }
    }

    if (sf->selective_ref_frame >= 3) {
      if (ref_frame[0] == ALTREF2_FRAME || ref_frame[1] == ALTREF2_FRAME)
        if (av1_encoder_get_relative_dist(
                order_hint_info,
                ref_display_order_hint[ALTREF2_FRAME - LAST_FRAME],
                cur_frame_display_order_hint) < 0)
          return 1;
      if (ref_frame[0] == BWDREF_FRAME || ref_frame[1] == BWDREF_FRAME)
        if (av1_encoder_get_relative_dist(
                order_hint_info,
                ref_display_order_hint[BWDREF_FRAME - LAST_FRAME],
                cur_frame_display_order_hint) < 0)
          return 1;
    }

    if (sf->selective_ref_frame >= 4 && comp_pred) {
      // Check if one of the reference is ALTREF2_FRAME and BWDREF_FRAME is a
      // valid reference.
      if ((ref_frame[0] == ALTREF2_FRAME || ref_frame[1] == ALTREF2_FRAME) &&
          (cpi->ref_frame_flags & av1_ref_frame_flag_list[BWDREF_FRAME])) {
        // Check if both ALTREF2_FRAME and BWDREF_FRAME are future references.
        const int arf2_dist = av1_encoder_get_relative_dist(
            order_hint_info, ref_display_order_hint[ALTREF2_FRAME - LAST_FRAME],
            cur_frame_display_order_hint);
        const int bwd_dist = av1_encoder_get_relative_dist(
            order_hint_info, ref_display_order_hint[BWDREF_FRAME - LAST_FRAME],
            cur_frame_display_order_hint);
        if (arf2_dist > 0 && bwd_dist > 0 && bwd_dist <= arf2_dist) {
          // Drop ALTREF2_FRAME as a reference if BWDREF_FRAME is a closer
          // reference to the current frame than ALTREF2_FRAME
          assert(get_ref_frame_buf(cm, ALTREF2_FRAME) != NULL);
          assert(get_ref_frame_buf(cm, BWDREF_FRAME) != NULL);
          return 1;
        }
      }
    }
  }
  return 0;
}
#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // AOM_AV1_ENCODER_RDOPT_H_
