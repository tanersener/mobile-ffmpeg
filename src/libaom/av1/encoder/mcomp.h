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

#ifndef AOM_AV1_ENCODER_MCOMP_H_
#define AOM_AV1_ENCODER_MCOMP_H_

#include "av1/encoder/block.h"

#include "aom_dsp/variance.h"

#ifdef __cplusplus
extern "C" {
#endif

// The maximum number of steps in a step search given the largest
// allowed initial step
#define MAX_MVSEARCH_STEPS 11
// Max full pel mv specified in the unit of full pixel
// Enable the use of motion vector in range [-1023, 1023].
#define MAX_FULL_PEL_VAL ((1 << (MAX_MVSEARCH_STEPS - 1)) - 1)
// Maximum size of the first step in full pel units
#define MAX_FIRST_STEP (1 << (MAX_MVSEARCH_STEPS - 1))
// Allowed motion vector pixel distance outside image border
// for Block_16x16
#define BORDER_MV_PIXELS_B16 (16 + AOM_INTERP_EXTEND)

#define SEARCH_RANGE_8P 3
#define SEARCH_GRID_STRIDE_8P (2 * SEARCH_RANGE_8P + 1)
#define SEARCH_GRID_CENTER_8P \
  (SEARCH_RANGE_8P * SEARCH_GRID_STRIDE_8P + SEARCH_RANGE_8P)

// motion search site
typedef struct search_site {
  MV mv;
  int offset;
} search_site;

typedef struct search_site_config {
  search_site ss[8 * MAX_MVSEARCH_STEPS + 1];
  int ss_count;
  int searches_per_step;
  int stride;
} search_site_config;

typedef struct {
  MV coord;
  int coord_offset;
} search_neighbors;

void av1_init_dsmotion_compensation(search_site_config *cfg, int stride);
void av1_init3smotion_compensation(search_site_config *cfg, int stride);

void av1_set_mv_search_range(MvLimits *mv_limits, const MV *mv);

int av1_mv_bit_cost(const MV *mv, const MV *ref, const int *mvjcost,
                    int *mvcost[2], int weight);

// Utility to compute variance + MV rate cost for a given MV
int av1_get_mvpred_var(const MACROBLOCK *x, const MV *best_mv,
                       const MV *center_mv, const aom_variance_fn_ptr_t *vfp,
                       int use_mvcost);
int av1_get_mvpred_av_var(const MACROBLOCK *x, const MV *best_mv,
                          const MV *center_mv, const uint8_t *second_pred,
                          const aom_variance_fn_ptr_t *vfp, int use_mvcost);
int av1_get_mvpred_mask_var(const MACROBLOCK *x, const MV *best_mv,
                            const MV *center_mv, const uint8_t *second_pred,
                            const uint8_t *mask, int mask_stride,
                            int invert_mask, const aom_variance_fn_ptr_t *vfp,
                            int use_mvcost);

struct AV1_COMP;
struct SPEED_FEATURES;

int av1_init_search_range(int size);

int av1_refining_search_sad(struct macroblock *x, MV *ref_mv, int sad_per_bit,
                            int distance, const aom_variance_fn_ptr_t *fn_ptr,
                            const MV *center_mv);

unsigned int av1_int_pro_motion_estimation(const struct AV1_COMP *cpi,
                                           MACROBLOCK *x, BLOCK_SIZE bsize,
                                           int mi_row, int mi_col,
                                           const MV *ref_mv);

// Runs sequence of diamond searches in smaller steps for RD.
int av1_full_pixel_diamond(const struct AV1_COMP *cpi, MACROBLOCK *x,
                           MV *mvp_full, int step_param, int sadpb,
                           int further_steps, int do_refine, int *cost_list,
                           const aom_variance_fn_ptr_t *fn_ptr,
                           const MV *ref_mv, MV *dst_mv);

int av1_hex_search(MACROBLOCK *x, MV *start_mv, int search_param,
                   int sad_per_bit, int do_init_search, int *cost_list,
                   const aom_variance_fn_ptr_t *vfp, int use_mvcost,
                   const MV *center_mv);

typedef int(fractional_mv_step_fp)(
    MACROBLOCK *x, const AV1_COMMON *const cm, int mi_row, int mi_col,
    const MV *ref_mv, int allow_hp, int error_per_bit,
    const aom_variance_fn_ptr_t *vfp,
    int forced_stop,  // 0 - full, 1 - qtr only, 2 - half only
    int iters_per_step, int *cost_list, int *mvjcost, int *mvcost[2],
    int *distortion, unsigned int *sse1, const uint8_t *second_pred,
    const uint8_t *mask, int mask_stride, int invert_mask, int w, int h,
    int use_accurate_subpel_search, const int do_reset_fractional_mv);

extern fractional_mv_step_fp av1_find_best_sub_pixel_tree;
extern fractional_mv_step_fp av1_find_best_sub_pixel_tree_pruned;
extern fractional_mv_step_fp av1_find_best_sub_pixel_tree_pruned_more;
extern fractional_mv_step_fp av1_find_best_sub_pixel_tree_pruned_evenmore;
extern fractional_mv_step_fp av1_return_max_sub_pixel_mv;
extern fractional_mv_step_fp av1_return_min_sub_pixel_mv;

typedef int (*av1_full_search_fn_t)(const MACROBLOCK *x, const MV *ref_mv,
                                    int sad_per_bit, int distance,
                                    const aom_variance_fn_ptr_t *fn_ptr,
                                    const MV *center_mv, MV *best_mv);

typedef int (*av1_diamond_search_fn_t)(
    MACROBLOCK *x, const search_site_config *cfg, MV *ref_mv, MV *best_mv,
    int search_param, int sad_per_bit, int *num00,
    const aom_variance_fn_ptr_t *fn_ptr, const MV *center_mv);

int av1_refining_search_8p_c(MACROBLOCK *x, int error_per_bit, int search_range,
                             const aom_variance_fn_ptr_t *fn_ptr,
                             const uint8_t *mask, int mask_stride,
                             int invert_mask, const MV *center_mv,
                             const uint8_t *second_pred);

int av1_full_pixel_search(const struct AV1_COMP *cpi, MACROBLOCK *x,
                          BLOCK_SIZE bsize, MV *mvp_full, int step_param,
                          int method, int run_mesh_search, int error_per_bit,
                          int *cost_list, const MV *ref_mv, int var_max, int rd,
                          int x_pos, int y_pos, int intra,
                          const search_site_config *cfg,
                          int use_intrabc_mesh_pattern);

int av1_obmc_full_pixel_search(const struct AV1_COMP *cpi, MACROBLOCK *x,
                               MV *mvp_full, int step_param, int sadpb,
                               int further_steps, int do_refine,
                               const aom_variance_fn_ptr_t *fn_ptr,
                               const MV *ref_mv, MV *dst_mv, int is_second,
                               const search_site_config *cfg);
int av1_find_best_obmc_sub_pixel_tree_up(
    MACROBLOCK *x, const AV1_COMMON *const cm, int mi_row, int mi_col,
    MV *bestmv, const MV *ref_mv, int allow_hp, int error_per_bit,
    const aom_variance_fn_ptr_t *vfp, int forced_stop, int iters_per_step,
    int *mvjcost, int *mvcost[2], int *distortion, unsigned int *sse1,
    int is_second, int use_accurate_subpel_search);

unsigned int av1_compute_motion_cost(const struct AV1_COMP *cpi,
                                     MACROBLOCK *const x, BLOCK_SIZE bsize,
                                     int mi_row, int mi_col, const MV *this_mv);
unsigned int av1_refine_warped_mv(const struct AV1_COMP *cpi,
                                  MACROBLOCK *const x, BLOCK_SIZE bsize,
                                  int mi_row, int mi_col, int *pts0,
                                  int *pts_inref0, int total_samples);

// Performs a motion search in SIMPLE_TRANSLATION mode using reference frame
// ref. Note that this sets the offset of mbmi, so we will need to reset it
// after calling this function.
void av1_simple_motion_search(struct AV1_COMP *const cpi, MACROBLOCK *x,
                              int mi_row, int mi_col, BLOCK_SIZE bsize, int ref,
                              MV ref_mv_full, int num_planes, int use_subpixel);

// Performs a simple motion search to calculate the sse and var of the residue
void av1_simple_motion_sse_var(struct AV1_COMP *cpi, MACROBLOCK *x, int mi_row,
                               int mi_col, BLOCK_SIZE bsize,
                               const MV ref_mv_full, int use_subpixel,
                               unsigned int *sse, unsigned int *var);

static INLINE void av1_set_fractional_mv(int_mv *fractional_best_mv) {
  for (int z = 0; z < 3; z++) {
    fractional_best_mv[z].as_int = INVALID_MV;
  }
}

static INLINE void set_subpel_mv_search_range(const MvLimits *mv_limits,
                                              int *col_min, int *col_max,
                                              int *row_min, int *row_max,
                                              const MV *ref_mv) {
  const int max_mv = MAX_FULL_PEL_VAL * 8;
  const int minc = AOMMAX(mv_limits->col_min * 8, ref_mv->col - max_mv);
  const int maxc = AOMMIN(mv_limits->col_max * 8, ref_mv->col + max_mv);
  const int minr = AOMMAX(mv_limits->row_min * 8, ref_mv->row - max_mv);
  const int maxr = AOMMIN(mv_limits->row_max * 8, ref_mv->row + max_mv);

  *col_min = AOMMAX(MV_LOW + 1, minc);
  *col_max = AOMMIN(MV_UPP - 1, maxc);
  *row_min = AOMMAX(MV_LOW + 1, minr);
  *row_max = AOMMIN(MV_UPP - 1, maxr);
}

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // AOM_AV1_ENCODER_MCOMP_H_
