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

#include <math.h>
#include <limits.h>

#include "config/aom_config.h"

#include "av1/common/alloccommon.h"
#include "av1/common/onyxc_int.h"
#include "av1/common/quant_common.h"
#include "av1/common/reconinter.h"
#include "av1/common/odintrin.h"
#include "av1/encoder/av1_quantize.h"
#include "av1/encoder/extend.h"
#include "av1/encoder/firstpass.h"
#include "av1/encoder/mcomp.h"
#include "av1/encoder/encoder.h"
#include "av1/encoder/ratectrl.h"
#include "av1/encoder/reconinter_enc.h"
#include "av1/encoder/segmentation.h"
#include "av1/encoder/temporal_filter.h"
#include "aom_dsp/aom_dsp_common.h"
#include "aom_mem/aom_mem.h"
#include "aom_ports/mem.h"
#include "aom_ports/aom_timer.h"
#include "aom_scale/aom_scale.h"

#define EDGE_THRESHOLD 50
#define SQRT_PI_BY_2 1.25331413732

static void temporal_filter_predictors_mb_c(
    MACROBLOCKD *xd, uint8_t *y_mb_ptr, uint8_t *u_mb_ptr, uint8_t *v_mb_ptr,
    int stride, int uv_block_width, int uv_block_height, int mv_row, int mv_col,
    uint8_t *pred, struct scale_factors *scale, int x, int y,
    int can_use_previous, int num_planes) {
  const MV mv = { mv_row, mv_col };
  enum mv_precision mv_precision_uv;
  int uv_stride;
  // TODO(angiebird): change plane setting accordingly
  ConvolveParams conv_params = get_conv_params(0, 0, xd->bd);
  const InterpFilters interp_filters =
      av1_make_interp_filters(MULTITAP_SHARP, MULTITAP_SHARP);
  WarpTypesAllowed warp_types;
  memset(&warp_types, 0, sizeof(WarpTypesAllowed));

  if (uv_block_width == (BW >> 1)) {
    uv_stride = (stride + 1) >> 1;
    mv_precision_uv = MV_PRECISION_Q4;
  } else {
    uv_stride = stride;
    mv_precision_uv = MV_PRECISION_Q3;
  }
  av1_build_inter_predictor(y_mb_ptr, stride, &pred[0], BW, &mv, scale, BW, BH,
                            &conv_params, interp_filters, &warp_types, x, y, 0,
                            0, MV_PRECISION_Q3, x, y, xd, can_use_previous);

  if (num_planes > 1) {
    av1_build_inter_predictor(
        u_mb_ptr, uv_stride, &pred[BLK_PELS], uv_block_width, &mv, scale,
        uv_block_width, uv_block_height, &conv_params, interp_filters,
        &warp_types, x, y, 1, 0, mv_precision_uv, x, y, xd, can_use_previous);

    av1_build_inter_predictor(
        v_mb_ptr, uv_stride, &pred[(BLK_PELS << 1)], uv_block_width, &mv, scale,
        uv_block_width, uv_block_height, &conv_params, interp_filters,
        &warp_types, x, y, 2, 0, mv_precision_uv, x, y, xd, can_use_previous);
  }
}

static void apply_temporal_filter_self(const uint8_t *pred, int buf_stride,
                                       unsigned int block_width,
                                       unsigned int block_height,
                                       int filter_weight, uint32_t *accumulator,
                                       uint16_t *count) {
  const int modifier = filter_weight * 16;
  unsigned int i, j, k = 0;
  assert(filter_weight == 2);

  for (i = 0; i < block_height; i++) {
    for (j = 0; j < block_width; j++) {
      const int pixel_value = pred[i * buf_stride + j];
      count[k] += modifier;
      accumulator[k] += modifier * pixel_value;
      ++k;
    }
  }
}

static void highbd_apply_temporal_filter_self(
    const uint8_t *pred8, int buf_stride, unsigned int block_width,
    unsigned int block_height, int filter_weight, uint32_t *accumulator,
    uint16_t *count) {
  const int modifier = filter_weight * 16;
  const uint16_t *pred = CONVERT_TO_SHORTPTR(pred8);
  unsigned int i, j, k = 0;
  assert(filter_weight == 2);

  for (i = 0; i < block_height; i++) {
    for (j = 0; j < block_width; j++) {
      const int pixel_value = pred[i * buf_stride + j];
      count[k] += modifier;
      accumulator[k] += modifier * pixel_value;
      ++k;
    }
  }
}

static INLINE int mod_index(int64_t sum_dist, int index, int rounding,
                            int strength, int filter_weight) {
  int mod = (int)(((sum_dist * 3) / index + rounding) >> strength);
  mod = AOMMIN(16, mod);
  mod = 16 - mod;
  mod *= filter_weight;
  return mod;
}

static INLINE void calculate_squared_errors(const uint8_t *s, int s_stride,
                                            const uint8_t *p, int p_stride,
                                            uint16_t *diff_sse, unsigned int w,
                                            unsigned int h) {
  int idx = 0;
  unsigned int i, j;

  for (i = 0; i < h; i++) {
    for (j = 0; j < w; j++) {
      const int16_t diff = s[i * s_stride + j] - p[i * p_stride + j];
      diff_sse[idx] = diff * diff;
      idx++;
    }
  }
}

static void apply_temporal_filter(
    const uint8_t *y_frame1, int y_stride, const uint8_t *y_pred,
    int y_buf_stride, const uint8_t *u_frame1, const uint8_t *v_frame1,
    int uv_stride, const uint8_t *u_pred, const uint8_t *v_pred,
    int uv_buf_stride, unsigned int block_width, unsigned int block_height,
    int ss_x, int ss_y, int strength, int filter_weight,
    uint32_t *y_accumulator, uint16_t *y_count, uint32_t *u_accumulator,
    uint16_t *u_count, uint32_t *v_accumulator, uint16_t *v_count) {
  unsigned int i, j, k, m;
  int modifier;
  const int rounding = (1 << strength) >> 1;
  const unsigned int uv_block_width = block_width >> ss_x;
  const unsigned int uv_block_height = block_height >> ss_y;
  DECLARE_ALIGNED(16, uint16_t, y_diff_sse[BLK_PELS]);
  DECLARE_ALIGNED(16, uint16_t, u_diff_sse[BLK_PELS]);
  DECLARE_ALIGNED(16, uint16_t, v_diff_sse[BLK_PELS]);

  int idx = 0, idy;

  assert(filter_weight >= 0);
  assert(filter_weight <= 2);

  memset(y_diff_sse, 0, BLK_PELS * sizeof(uint16_t));
  memset(u_diff_sse, 0, BLK_PELS * sizeof(uint16_t));
  memset(v_diff_sse, 0, BLK_PELS * sizeof(uint16_t));

  // Calculate diff^2 for each pixel of the block.
  // TODO(yunqing): the following code needs to be optimized.
  calculate_squared_errors(y_frame1, y_stride, y_pred, y_buf_stride, y_diff_sse,
                           block_width, block_height);
  calculate_squared_errors(u_frame1, uv_stride, u_pred, uv_buf_stride,
                           u_diff_sse, uv_block_width, uv_block_height);
  calculate_squared_errors(v_frame1, uv_stride, v_pred, uv_buf_stride,
                           v_diff_sse, uv_block_width, uv_block_height);

  for (i = 0, k = 0, m = 0; i < block_height; i++) {
    for (j = 0; j < block_width; j++) {
      const int pixel_value = y_pred[i * y_buf_stride + j];

      // non-local mean approach
      int y_index = 0;

      const int uv_r = i >> ss_y;
      const int uv_c = j >> ss_x;
      modifier = 0;

      for (idy = -1; idy <= 1; ++idy) {
        for (idx = -1; idx <= 1; ++idx) {
          const int row = (int)i + idy;
          const int col = (int)j + idx;

          if (row >= 0 && row < (int)block_height && col >= 0 &&
              col < (int)block_width) {
            modifier += y_diff_sse[row * (int)block_width + col];
            ++y_index;
          }
        }
      }

      assert(y_index > 0);

      modifier += u_diff_sse[uv_r * uv_block_width + uv_c];
      modifier += v_diff_sse[uv_r * uv_block_width + uv_c];

      y_index += 2;

      modifier =
          (int)mod_index(modifier, y_index, rounding, strength, filter_weight);

      y_count[k] += modifier;
      y_accumulator[k] += modifier * pixel_value;

      ++k;

      // Process chroma component
      if (!(i & ss_y) && !(j & ss_x)) {
        const int u_pixel_value = u_pred[uv_r * uv_buf_stride + uv_c];
        const int v_pixel_value = v_pred[uv_r * uv_buf_stride + uv_c];

        // non-local mean approach
        int cr_index = 0;
        int u_mod = 0, v_mod = 0;
        int y_diff = 0;

        for (idy = -1; idy <= 1; ++idy) {
          for (idx = -1; idx <= 1; ++idx) {
            const int row = uv_r + idy;
            const int col = uv_c + idx;

            if (row >= 0 && row < (int)uv_block_height && col >= 0 &&
                col < (int)uv_block_width) {
              u_mod += u_diff_sse[row * uv_block_width + col];
              v_mod += v_diff_sse[row * uv_block_width + col];
              ++cr_index;
            }
          }
        }

        assert(cr_index > 0);

        for (idy = 0; idy < 1 + ss_y; ++idy) {
          for (idx = 0; idx < 1 + ss_x; ++idx) {
            const int row = (uv_r << ss_y) + idy;
            const int col = (uv_c << ss_x) + idx;
            y_diff += y_diff_sse[row * (int)block_width + col];
            ++cr_index;
          }
        }

        u_mod += y_diff;
        v_mod += y_diff;

        u_mod =
            (int)mod_index(u_mod, cr_index, rounding, strength, filter_weight);
        v_mod =
            (int)mod_index(v_mod, cr_index, rounding, strength, filter_weight);

        u_count[m] += u_mod;
        u_accumulator[m] += u_mod * u_pixel_value;
        v_count[m] += v_mod;
        v_accumulator[m] += v_mod * v_pixel_value;

        ++m;
      }  // Complete YUV pixel
    }
  }
}

static INLINE void highbd_calculate_squared_errors(
    const uint16_t *s, int s_stride, const uint16_t *p, int p_stride,
    uint32_t *diff_sse, unsigned int w, unsigned int h) {
  int idx = 0;
  unsigned int i, j;

  for (i = 0; i < h; i++) {
    for (j = 0; j < w; j++) {
      const int16_t diff = s[i * s_stride + j] - p[i * p_stride + j];
      diff_sse[idx] = diff * diff;
      idx++;
    }
  }
}

static void highbd_apply_temporal_filter(
    const uint8_t *yf, int y_stride, const uint8_t *yp, int y_buf_stride,
    const uint8_t *uf, const uint8_t *vf, int uv_stride, const uint8_t *up,
    const uint8_t *vp, int uv_buf_stride, unsigned int block_width,
    unsigned int block_height, int ss_x, int ss_y, int strength,
    int filter_weight, uint32_t *y_accumulator, uint16_t *y_count,
    uint32_t *u_accumulator, uint16_t *u_count, uint32_t *v_accumulator,
    uint16_t *v_count) {
  unsigned int i, j, k, m;
  int64_t modifier;
  const int rounding = (1 << strength) >> 1;
  const unsigned int uv_block_width = block_width >> ss_x;
  const unsigned int uv_block_height = block_height >> ss_y;
  DECLARE_ALIGNED(16, uint32_t, y_diff_sse[BLK_PELS]);
  DECLARE_ALIGNED(16, uint32_t, u_diff_sse[BLK_PELS]);
  DECLARE_ALIGNED(16, uint32_t, v_diff_sse[BLK_PELS]);

  const uint16_t *y_frame1 = CONVERT_TO_SHORTPTR(yf);
  const uint16_t *u_frame1 = CONVERT_TO_SHORTPTR(uf);
  const uint16_t *v_frame1 = CONVERT_TO_SHORTPTR(vf);
  const uint16_t *y_pred = CONVERT_TO_SHORTPTR(yp);
  const uint16_t *u_pred = CONVERT_TO_SHORTPTR(up);
  const uint16_t *v_pred = CONVERT_TO_SHORTPTR(vp);
  int idx = 0, idy;

  assert(filter_weight >= 0);
  assert(filter_weight <= 2);

  memset(y_diff_sse, 0, BLK_PELS * sizeof(uint32_t));
  memset(u_diff_sse, 0, BLK_PELS * sizeof(uint32_t));
  memset(v_diff_sse, 0, BLK_PELS * sizeof(uint32_t));

  // Calculate diff^2 for each pixel of the block.
  // TODO(yunqing): the following code needs to be optimized.
  highbd_calculate_squared_errors(y_frame1, y_stride, y_pred, y_buf_stride,
                                  y_diff_sse, block_width, block_height);
  highbd_calculate_squared_errors(u_frame1, uv_stride, u_pred, uv_buf_stride,
                                  u_diff_sse, uv_block_width, uv_block_height);
  highbd_calculate_squared_errors(v_frame1, uv_stride, v_pred, uv_buf_stride,
                                  v_diff_sse, uv_block_width, uv_block_height);

  for (i = 0, k = 0, m = 0; i < block_height; i++) {
    for (j = 0; j < block_width; j++) {
      const int pixel_value = y_pred[i * y_buf_stride + j];

      // non-local mean approach
      int y_index = 0;

      const int uv_r = i >> ss_y;
      const int uv_c = j >> ss_x;
      modifier = 0;

      for (idy = -1; idy <= 1; ++idy) {
        for (idx = -1; idx <= 1; ++idx) {
          const int row = (int)i + idy;
          const int col = (int)j + idx;

          if (row >= 0 && row < (int)block_height && col >= 0 &&
              col < (int)block_width) {
            modifier += y_diff_sse[row * (int)block_width + col];
            ++y_index;
          }
        }
      }

      assert(y_index > 0);

      modifier += u_diff_sse[uv_r * uv_block_width + uv_c];
      modifier += v_diff_sse[uv_r * uv_block_width + uv_c];

      y_index += 2;

      const int final_y_mod =
          mod_index(modifier, y_index, rounding, strength, filter_weight);

      y_count[k] += final_y_mod;
      y_accumulator[k] += final_y_mod * pixel_value;

      ++k;

      // Process chroma component
      if (!(i & ss_y) && !(j & ss_x)) {
        const int u_pixel_value = u_pred[uv_r * uv_buf_stride + uv_c];
        const int v_pixel_value = v_pred[uv_r * uv_buf_stride + uv_c];

        // non-local mean approach
        int cr_index = 0;
        int64_t u_mod = 0, v_mod = 0;
        int y_diff = 0;

        for (idy = -1; idy <= 1; ++idy) {
          for (idx = -1; idx <= 1; ++idx) {
            const int row = uv_r + idy;
            const int col = uv_c + idx;

            if (row >= 0 && row < (int)uv_block_height && col >= 0 &&
                col < (int)uv_block_width) {
              u_mod += u_diff_sse[row * uv_block_width + col];
              v_mod += v_diff_sse[row * uv_block_width + col];
              ++cr_index;
            }
          }
        }

        assert(cr_index > 0);

        for (idy = 0; idy < 1 + ss_y; ++idy) {
          for (idx = 0; idx < 1 + ss_x; ++idx) {
            const int row = (uv_r << ss_y) + idy;
            const int col = (uv_c << ss_x) + idx;
            y_diff += y_diff_sse[row * (int)block_width + col];
            ++cr_index;
          }
        }

        u_mod += y_diff;
        v_mod += y_diff;

        const int final_u_mod =
            mod_index(u_mod, cr_index, rounding, strength, filter_weight);
        const int final_v_mod =
            mod_index(v_mod, cr_index, rounding, strength, filter_weight);

        u_count[m] += final_u_mod;
        u_accumulator[m] += final_u_mod * u_pixel_value;
        v_count[m] += final_v_mod;
        v_accumulator[m] += final_v_mod * v_pixel_value;

        ++m;
      }  // Complete YUV pixel
    }
  }
}

// Only used in single plane case
void av1_temporal_filter_apply_c(uint8_t *frame1, unsigned int stride,
                                 uint8_t *frame2, unsigned int block_width,
                                 unsigned int block_height, int strength,
                                 int filter_weight, unsigned int *accumulator,
                                 uint16_t *count) {
  unsigned int i, j, k;
  int modifier;
  int byte = 0;
  const int rounding = strength > 0 ? 1 << (strength - 1) : 0;

  for (i = 0, k = 0; i < block_height; i++) {
    for (j = 0; j < block_width; j++, k++) {
      int pixel_value = *frame2;

      // non-local mean approach
      int diff_sse[9] = { 0 };
      int idx, idy, index = 0;

      for (idy = -1; idy <= 1; ++idy) {
        for (idx = -1; idx <= 1; ++idx) {
          int row = (int)i + idy;
          int col = (int)j + idx;

          if (row >= 0 && row < (int)block_height && col >= 0 &&
              col < (int)block_width) {
            int diff = frame1[byte + idy * (int)stride + idx] -
                       frame2[idy * (int)block_width + idx];
            diff_sse[index] = diff * diff;
            ++index;
          }
        }
      }

      assert(index > 0);

      modifier = 0;
      for (idx = 0; idx < 9; ++idx) modifier += diff_sse[idx];

      modifier *= 3;
      modifier /= index;

      ++frame2;

      modifier += rounding;
      modifier >>= strength;

      if (modifier > 16) modifier = 16;

      modifier = 16 - modifier;
      modifier *= filter_weight;

      count[k] += modifier;
      accumulator[k] += modifier * pixel_value;

      byte++;
    }

    byte += stride - block_width;
  }
}

// Only used in single plane case
void av1_highbd_temporal_filter_apply_c(
    uint8_t *frame1_8, unsigned int stride, uint8_t *frame2_8,
    unsigned int block_width, unsigned int block_height, int strength,
    int filter_weight, unsigned int *accumulator, uint16_t *count) {
  uint16_t *frame1 = CONVERT_TO_SHORTPTR(frame1_8);
  uint16_t *frame2 = CONVERT_TO_SHORTPTR(frame2_8);
  unsigned int i, j, k;
  int modifier;
  int byte = 0;
  const int rounding = strength > 0 ? 1 << (strength - 1) : 0;

  for (i = 0, k = 0; i < block_height; i++) {
    for (j = 0; j < block_width; j++, k++) {
      int pixel_value = *frame2;

      // non-local mean approach
      int diff_sse[9] = { 0 };
      int idx, idy, index = 0;

      for (idy = -1; idy <= 1; ++idy) {
        for (idx = -1; idx <= 1; ++idx) {
          int row = (int)i + idy;
          int col = (int)j + idx;

          if (row >= 0 && row < (int)block_height && col >= 0 &&
              col < (int)block_width) {
            int diff = frame1[byte + idy * (int)stride + idx] -
                       frame2[idy * (int)block_width + idx];
            diff_sse[index] = diff * diff;
            ++index;
          }
        }
      }

      assert(index > 0);

      modifier = 0;
      for (idx = 0; idx < 9; ++idx) modifier += diff_sse[idx];

      modifier *= 3;
      modifier /= index;

      ++frame2;

      modifier += rounding;
      modifier >>= strength;

      if (modifier > 16) modifier = 16;

      modifier = 16 - modifier;
      modifier *= filter_weight;

      count[k] += modifier;
      accumulator[k] += modifier * pixel_value;

      byte++;
    }

    byte += stride - block_width;
  }
}

static int temporal_filter_find_matching_mb_c(AV1_COMP *cpi,
                                              uint8_t *arf_frame_buf,
                                              uint8_t *frame_ptr_buf,
                                              int stride, int x_pos,
                                              int y_pos) {
  MACROBLOCK *const x = &cpi->td.mb;
  MACROBLOCKD *const xd = &x->e_mbd;
  const MV_SPEED_FEATURES *const mv_sf = &cpi->sf.mv;
  int step_param;
  int sadpb = x->sadperbit16;
  int bestsme = INT_MAX;
  int distortion;
  unsigned int sse;
  int cost_list[5];
  MvLimits tmp_mv_limits = x->mv_limits;

  MV best_ref_mv1 = kZeroMv;
  MV best_ref_mv1_full; /* full-pixel value of best_ref_mv1 */

  // Save input state
  struct buf_2d src = x->plane[0].src;
  struct buf_2d pre = xd->plane[0].pre[0];

  best_ref_mv1_full.col = best_ref_mv1.col >> 3;
  best_ref_mv1_full.row = best_ref_mv1.row >> 3;

  // Setup frame pointers
  x->plane[0].src.buf = arf_frame_buf;
  x->plane[0].src.stride = stride;
  xd->plane[0].pre[0].buf = frame_ptr_buf;
  xd->plane[0].pre[0].stride = stride;

  step_param = mv_sf->reduce_first_step_size;
  step_param = AOMMIN(step_param, MAX_MVSEARCH_STEPS - 2);

  av1_set_mv_search_range(&x->mv_limits, &best_ref_mv1);

  av1_full_pixel_search(cpi, x, TF_BLOCK, &best_ref_mv1_full, step_param, NSTEP,
                        1, sadpb, cond_cost_list(cpi, cost_list), &best_ref_mv1,
                        0, 0, x_pos, y_pos, 0);
  x->mv_limits = tmp_mv_limits;

  // Ignore mv costing by sending NULL pointer instead of cost array
  if (cpi->common.cur_frame_force_integer_mv == 1) {
    const uint8_t *const src_address = x->plane[0].src.buf;
    const int src_stride = x->plane[0].src.stride;
    const uint8_t *const y = xd->plane[0].pre[0].buf;
    const int y_stride = xd->plane[0].pre[0].stride;
    const int offset = x->best_mv.as_mv.row * y_stride + x->best_mv.as_mv.col;

    x->best_mv.as_mv.row *= 8;
    x->best_mv.as_mv.col *= 8;

    bestsme = cpi->fn_ptr[TF_BLOCK].vf(y + offset, y_stride, src_address,
                                       src_stride, &sse);
  } else {
    bestsme = cpi->find_fractional_mv_step(
        x, &cpi->common, 0, 0, &best_ref_mv1,
        cpi->common.allow_high_precision_mv, x->errorperbit,
        &cpi->fn_ptr[TF_BLOCK], 0, mv_sf->subpel_iters_per_step,
        cond_cost_list(cpi, cost_list), NULL, NULL, &distortion, &sse, NULL,
        NULL, 0, 0, 16, 16, USE_8_TAPS, 1);
  }

  x->e_mbd.mi[0]->mv[0] = x->best_mv;

  // Restore input state
  x->plane[0].src = src;
  xd->plane[0].pre[0] = pre;

  return bestsme;
}

static void temporal_filter_iterate_c(AV1_COMP *cpi,
                                      YV12_BUFFER_CONFIG **frames,
                                      int frame_count, int alt_ref_index,
                                      int strength,
                                      struct scale_factors *ref_scale_factors) {
  const AV1_COMMON *cm = &cpi->common;
  const int num_planes = av1_num_planes(cm);
  int byte;
  int frame;
  int mb_col, mb_row;
  unsigned int filter_weight;
  int mb_cols = (frames[alt_ref_index]->y_crop_width + BW - 1) >> BW_LOG2;
  int mb_rows = (frames[alt_ref_index]->y_crop_height + BH - 1) >> BH_LOG2;
  int mb_y_offset = 0;
  int mb_uv_offset = 0;
  DECLARE_ALIGNED(16, unsigned int, accumulator[BLK_PELS * 3]);
  DECLARE_ALIGNED(16, uint16_t, count[BLK_PELS * 3]);
  MACROBLOCKD *mbd = &cpi->td.mb.e_mbd;
  YV12_BUFFER_CONFIG *f = frames[alt_ref_index];
  uint8_t *dst1, *dst2;
  DECLARE_ALIGNED(32, uint16_t, predictor16[BLK_PELS * 3]);
  DECLARE_ALIGNED(32, uint8_t, predictor8[BLK_PELS * 3]);
  uint8_t *predictor;
  const int mb_uv_height = BH >> mbd->plane[1].subsampling_y;
  const int mb_uv_width = BW >> mbd->plane[1].subsampling_x;

  // Save input state
  uint8_t *input_buffer[MAX_MB_PLANE];
  int i;
  if (mbd->cur_buf->flags & YV12_FLAG_HIGHBITDEPTH) {
    predictor = CONVERT_TO_BYTEPTR(predictor16);
  } else {
    predictor = predictor8;
  }

  mbd->block_ref_scale_factors[0] = ref_scale_factors;
  mbd->block_ref_scale_factors[1] = ref_scale_factors;

  for (i = 0; i < num_planes; i++) input_buffer[i] = mbd->plane[i].pre[0].buf;

  for (mb_row = 0; mb_row < mb_rows; mb_row++) {
    // Source frames are extended to 16 pixels. This is different than
    //  L/A/G reference frames that have a border of 32 (AV1ENCBORDERINPIXELS)
    // A 6/8 tap filter is used for motion search.  This requires 2 pixels
    //  before and 3 pixels after.  So the largest Y mv on a border would
    //  then be 16 - AOM_INTERP_EXTEND. The UV blocks are half the size of the
    //  Y and therefore only extended by 8.  The largest mv that a UV block
    //  can support is 8 - AOM_INTERP_EXTEND.  A UV mv is half of a Y mv.
    //  (16 - AOM_INTERP_EXTEND) >> 1 which is greater than
    //  8 - AOM_INTERP_EXTEND.
    // To keep the mv in play for both Y and UV planes the max that it
    //  can be on a border is therefore 16 - (2*AOM_INTERP_EXTEND+1).
    cpi->td.mb.mv_limits.row_min =
        -((mb_row * BH) + (17 - 2 * AOM_INTERP_EXTEND));
    cpi->td.mb.mv_limits.row_max =
        ((mb_rows - 1 - mb_row) * BH) + (17 - 2 * AOM_INTERP_EXTEND);

    for (mb_col = 0; mb_col < mb_cols; mb_col++) {
      int j, k;
      int stride;

      memset(accumulator, 0, BLK_PELS * 3 * sizeof(accumulator[0]));
      memset(count, 0, BLK_PELS * 3 * sizeof(count[0]));

      cpi->td.mb.mv_limits.col_min =
          -((mb_col * BW) + (17 - 2 * AOM_INTERP_EXTEND));
      cpi->td.mb.mv_limits.col_max =
          ((mb_cols - 1 - mb_col) * BW) + (17 - 2 * AOM_INTERP_EXTEND);

      for (frame = 0; frame < frame_count; frame++) {
        // These thresholds need to be modified based on block size.
        int thresh_low = 10000 << THR_SHIFT;
        int thresh_high = 20000 << THR_SHIFT;

        if (frames[frame] == NULL) continue;

        mbd->mi[0]->mv[0].as_mv.row = 0;
        mbd->mi[0]->mv[0].as_mv.col = 0;
        mbd->mi[0]->motion_mode = SIMPLE_TRANSLATION;

        if (frame == alt_ref_index) {
          filter_weight = 2;
        } else {
          // Find best match in this frame by MC
          int err = temporal_filter_find_matching_mb_c(
              cpi, frames[alt_ref_index]->y_buffer + mb_y_offset,
              frames[frame]->y_buffer + mb_y_offset, frames[frame]->y_stride,
              mb_col * BW, mb_row * BH);

          // Assign higher weight to matching MB if it's error
          // score is lower. If not applying MC default behavior
          // is to weight all MBs equal.
          filter_weight = err < thresh_low ? 2 : err < thresh_high ? 1 : 0;
        }

        if (filter_weight != 0) {
          // Construct the predictors
          temporal_filter_predictors_mb_c(
              mbd, frames[frame]->y_buffer + mb_y_offset,
              frames[frame]->u_buffer + mb_uv_offset,
              frames[frame]->v_buffer + mb_uv_offset, frames[frame]->y_stride,
              mb_uv_width, mb_uv_height, mbd->mi[0]->mv[0].as_mv.row,
              mbd->mi[0]->mv[0].as_mv.col, predictor, ref_scale_factors,
              mb_col * BW, mb_row * BH, cm->allow_warped_motion, num_planes);

          // Apply the filter (YUV)
          if (frame == alt_ref_index) {
            uint8_t *pred = predictor;
            uint32_t *accum = accumulator;
            uint16_t *cnt = count;
            int plane;

            for (plane = 0; plane < num_planes; ++plane) {
              const int pred_stride = plane ? mb_uv_width : BW;
              const unsigned int w = plane ? mb_uv_width : BW;
              const unsigned int h = plane ? mb_uv_height : BH;

              if (mbd->cur_buf->flags & YV12_FLAG_HIGHBITDEPTH)
                highbd_apply_temporal_filter_self(pred, pred_stride, w, h,
                                                  filter_weight, accum, cnt);
              else
                apply_temporal_filter_self(pred, pred_stride, w, h,
                                           filter_weight, accum, cnt);

              pred += BLK_PELS;
              accum += BLK_PELS;
              cnt += BLK_PELS;
            }
          } else {
            if (mbd->cur_buf->flags & YV12_FLAG_HIGHBITDEPTH) {
              int adj_strength = strength + 2 * (mbd->bd - 8);

              if (num_planes <= 1) {
                // Single plane case
                av1_highbd_temporal_filter_apply_c(
                    f->y_buffer + mb_y_offset, f->y_stride, predictor, BW, BH,
                    adj_strength, filter_weight, accumulator, count);
              } else {
                // Process 3 planes together.
                highbd_apply_temporal_filter(
                    f->y_buffer + mb_y_offset, f->y_stride, predictor, BW,
                    f->u_buffer + mb_uv_offset, f->v_buffer + mb_uv_offset,
                    f->uv_stride, predictor + BLK_PELS,
                    predictor + (BLK_PELS << 1), mb_uv_width, BW, BH,
                    mbd->plane[1].subsampling_x, mbd->plane[1].subsampling_y,
                    adj_strength, filter_weight, accumulator, count,
                    accumulator + BLK_PELS, count + BLK_PELS,
                    accumulator + (BLK_PELS << 1), count + (BLK_PELS << 1));
              }
            } else {
              if (num_planes <= 1) {
                // Single plane case
                av1_temporal_filter_apply_c(
                    f->y_buffer + mb_y_offset, f->y_stride, predictor, BW, BH,
                    strength, filter_weight, accumulator, count);
              } else {
                // Process 3 planes together.
                apply_temporal_filter(
                    f->y_buffer + mb_y_offset, f->y_stride, predictor, BW,
                    f->u_buffer + mb_uv_offset, f->v_buffer + mb_uv_offset,
                    f->uv_stride, predictor + BLK_PELS,
                    predictor + (BLK_PELS << 1), mb_uv_width, BW, BH,
                    mbd->plane[1].subsampling_x, mbd->plane[1].subsampling_y,
                    strength, filter_weight, accumulator, count,
                    accumulator + BLK_PELS, count + BLK_PELS,
                    accumulator + (BLK_PELS << 1), count + (BLK_PELS << 1));
              }
            }
          }
        }
      }

      // Normalize filter output to produce AltRef frame
      if (mbd->cur_buf->flags & YV12_FLAG_HIGHBITDEPTH) {
        uint16_t *dst1_16;
        uint16_t *dst2_16;
        dst1 = cpi->alt_ref_buffer.y_buffer;
        dst1_16 = CONVERT_TO_SHORTPTR(dst1);
        stride = cpi->alt_ref_buffer.y_stride;
        byte = mb_y_offset;
        for (i = 0, k = 0; i < BH; i++) {
          for (j = 0; j < BW; j++, k++) {
            dst1_16[byte] =
                (uint16_t)OD_DIVU(accumulator[k] + (count[k] >> 1), count[k]);

            // move to next pixel
            byte++;
          }

          byte += stride - BW;
        }
        if (num_planes > 1) {
          dst1 = cpi->alt_ref_buffer.u_buffer;
          dst2 = cpi->alt_ref_buffer.v_buffer;
          dst1_16 = CONVERT_TO_SHORTPTR(dst1);
          dst2_16 = CONVERT_TO_SHORTPTR(dst2);
          stride = cpi->alt_ref_buffer.uv_stride;
          byte = mb_uv_offset;
          for (i = 0, k = BLK_PELS; i < mb_uv_height; i++) {
            for (j = 0; j < mb_uv_width; j++, k++) {
              int m = k + BLK_PELS;
              // U
              dst1_16[byte] =
                  (uint16_t)OD_DIVU(accumulator[k] + (count[k] >> 1), count[k]);
              // V
              dst2_16[byte] =
                  (uint16_t)OD_DIVU(accumulator[m] + (count[m] >> 1), count[m]);
              // move to next pixel
              byte++;
            }
            byte += stride - mb_uv_width;
          }
        }
      } else {
        dst1 = cpi->alt_ref_buffer.y_buffer;
        stride = cpi->alt_ref_buffer.y_stride;
        byte = mb_y_offset;
        for (i = 0, k = 0; i < BH; i++) {
          for (j = 0; j < BW; j++, k++) {
            dst1[byte] =
                (uint8_t)OD_DIVU(accumulator[k] + (count[k] >> 1), count[k]);

            // move to next pixel
            byte++;
          }
          byte += stride - BW;
        }
        if (num_planes > 1) {
          dst1 = cpi->alt_ref_buffer.u_buffer;
          dst2 = cpi->alt_ref_buffer.v_buffer;
          stride = cpi->alt_ref_buffer.uv_stride;
          byte = mb_uv_offset;
          for (i = 0, k = BLK_PELS; i < mb_uv_height; i++) {
            for (j = 0; j < mb_uv_width; j++, k++) {
              int m = k + BLK_PELS;
              // U
              dst1[byte] =
                  (uint8_t)OD_DIVU(accumulator[k] + (count[k] >> 1), count[k]);
              // V
              dst2[byte] =
                  (uint8_t)OD_DIVU(accumulator[m] + (count[m] >> 1), count[m]);
              // move to next pixel
              byte++;
            }
            byte += stride - mb_uv_width;
          }
        }
      }
      mb_y_offset += BW;
      mb_uv_offset += mb_uv_width;
    }
    mb_y_offset += BH * f->y_stride - BW * mb_cols;
    mb_uv_offset += mb_uv_height * f->uv_stride - mb_uv_width * mb_cols;
  }

  // Restore input state
  for (i = 0; i < num_planes; i++) mbd->plane[i].pre[0].buf = input_buffer[i];
}

// This is an adaptation of the mehtod in the following paper:
// Shen-Chuan Tai, Shih-Ming Yang, "A fast method for image noise
// estimation using Laplacian operator and adaptive edge detection,"
// Proc. 3rd International Symposium on Communications, Control and
// Signal Processing, 2008, St Julians, Malta.
//
// Return noise estimate, or -1.0 if there was a failure
static double estimate_noise(const uint8_t *src, int width, int height,
                             int stride, int edge_thresh) {
  int64_t sum = 0;
  int64_t num = 0;
  for (int i = 1; i < height - 1; ++i) {
    for (int j = 1; j < width - 1; ++j) {
      const int k = i * stride + j;
      // Sobel gradients
      const int Gx = (src[k - stride - 1] - src[k - stride + 1]) +
                     (src[k + stride - 1] - src[k + stride + 1]) +
                     2 * (src[k - 1] - src[k + 1]);
      const int Gy = (src[k - stride - 1] - src[k + stride - 1]) +
                     (src[k - stride + 1] - src[k + stride + 1]) +
                     2 * (src[k - stride] - src[k + stride]);
      const int Ga = abs(Gx) + abs(Gy);
      if (Ga < edge_thresh) {  // Smooth pixels
        // Find Laplacian
        const int v =
            4 * src[k] -
            2 * (src[k - 1] + src[k + 1] + src[k - stride] + src[k + stride]) +
            (src[k - stride - 1] + src[k - stride + 1] + src[k + stride - 1] +
             src[k + stride + 1]);
        sum += abs(v);
        ++num;
      }
    }
  }
  // If very few smooth pels, return -1 since the estimate is unreliable
  if (num < 16) return -1.0;

  const double sigma = (double)sum / (6 * num) * SQRT_PI_BY_2;
  return sigma;
}

// Return noise estimate, or -1.0 if there was a failure
static double highbd_estimate_noise(const uint8_t *src8, int width, int height,
                                    int stride, int bd, int edge_thresh) {
  uint16_t *src = CONVERT_TO_SHORTPTR(src8);
  int64_t sum = 0;
  int64_t num = 0;
  for (int i = 1; i < height - 1; ++i) {
    for (int j = 1; j < width - 1; ++j) {
      const int k = i * stride + j;
      // Sobel gradients
      const int Gx = (src[k - stride - 1] - src[k - stride + 1]) +
                     (src[k + stride - 1] - src[k + stride + 1]) +
                     2 * (src[k - 1] - src[k + 1]);
      const int Gy = (src[k - stride - 1] - src[k + stride - 1]) +
                     (src[k - stride + 1] - src[k + stride + 1]) +
                     2 * (src[k - stride] - src[k + stride]);
      const int Ga = ROUND_POWER_OF_TWO(abs(Gx) + abs(Gy), bd - 8);
      if (Ga < edge_thresh) {  // Smooth pixels
        // Find Laplacian
        const int v =
            4 * src[k] -
            2 * (src[k - 1] + src[k + 1] + src[k - stride] + src[k + stride]) +
            (src[k - stride - 1] + src[k - stride + 1] + src[k + stride - 1] +
             src[k + stride + 1]);
        sum += ROUND_POWER_OF_TWO(abs(v), bd - 8);
        ++num;
      }
    }
  }
  // If very few smooth pels, return -1 since the estimate is unreliable
  if (num < 16) return -1.0;

  const double sigma = (double)sum / (6 * num) * SQRT_PI_BY_2;
  return sigma;
}

// Apply buffer limits and context specific adjustments to arnr filter.
static void adjust_arnr_filter(AV1_COMP *cpi, int distance, int group_boost,
                               int *arnr_frames, int *arnr_strength) {
  const AV1EncoderConfig *const oxcf = &cpi->oxcf;
  const int frames_after_arf =
      av1_lookahead_depth(cpi->lookahead) - distance - 1;
  int frames_fwd = (cpi->oxcf.arnr_max_frames - 1) >> 1;
  int frames_bwd;
  int q, frames, strength;

  // Define the forward and backwards filter limits for this arnr group.
  if (frames_fwd > frames_after_arf) frames_fwd = frames_after_arf;
  if (frames_fwd > distance) frames_fwd = distance;

  frames_bwd = frames_fwd;

  // For even length filter there is one more frame backward
  // than forward: e.g. len=6 ==> bbbAff, len=7 ==> bbbAfff.
  if (frames_bwd < distance) frames_bwd += (oxcf->arnr_max_frames + 1) & 0x1;

  // Set the baseline active filter size.
  frames = frames_bwd + 1 + frames_fwd;

  // Adjust the strength based on active max q.
  if (cpi->common.current_frame.frame_number > 1)
    q = ((int)av1_convert_qindex_to_q(cpi->rc.avg_frame_qindex[INTER_FRAME],
                                      cpi->common.seq_params.bit_depth));
  else
    q = ((int)av1_convert_qindex_to_q(cpi->rc.avg_frame_qindex[KEY_FRAME],
                                      cpi->common.seq_params.bit_depth));
  MACROBLOCKD *mbd = &cpi->td.mb.e_mbd;
  struct lookahead_entry *buf = av1_lookahead_peek(cpi->lookahead, distance);
  double noiselevel;
  if (mbd->cur_buf->flags & YV12_FLAG_HIGHBITDEPTH) {
    noiselevel = highbd_estimate_noise(
        buf->img.y_buffer, buf->img.y_crop_width, buf->img.y_crop_height,
        buf->img.y_stride, mbd->bd, EDGE_THRESHOLD);
  } else {
    noiselevel = estimate_noise(buf->img.y_buffer, buf->img.y_crop_width,
                                buf->img.y_crop_height, buf->img.y_stride,
                                EDGE_THRESHOLD);
  }
  int adj_strength = oxcf->arnr_strength;
  if (noiselevel > 0) {
    // Get 4 integer adjustment levels in [-2, 1]
    int noiselevel_adj;
    if (noiselevel < 0.75)
      noiselevel_adj = -2;
    else if (noiselevel < 1.75)
      noiselevel_adj = -1;
    else if (noiselevel < 4.0)
      noiselevel_adj = 0;
    else
      noiselevel_adj = 1;
    adj_strength += noiselevel_adj;
  }
  // printf("[noise level: %g, strength = %d]\n", noiselevel, adj_strength);

  if (q > 16) {
    strength = adj_strength;
  } else {
    strength = adj_strength - ((16 - q) / 2);
    if (strength < 0) strength = 0;
  }

  // Adjust number of frames in filter and strength based on gf boost level.
  if (frames > group_boost / 150) {
    frames = group_boost / 150;
    frames += !(frames & 1);
  }

  if (strength > group_boost / 300) {
    strength = group_boost / 300;
  }

  *arnr_frames = frames;
  *arnr_strength = strength;
}

void av1_temporal_filter(AV1_COMP *cpi, int distance) {
  RATE_CONTROL *const rc = &cpi->rc;
  int frame;
  int frames_to_blur;
  int start_frame;
  int strength;
  int frames_to_blur_backward;
  int frames_to_blur_forward;
  struct scale_factors sf;

  YV12_BUFFER_CONFIG *frames[MAX_LAG_BUFFERS] = { NULL };
  const GF_GROUP *const gf_group = &cpi->twopass.gf_group;
  int rdmult = 0;

  // Apply context specific adjustments to the arnr filter parameters.
  if (gf_group->update_type[gf_group->index] == INTNL_ARF_UPDATE) {
    // TODO(weitinglin): Currently, we enforce the filtering strength on
    //                   extra ARFs' to be zeros. We should investigate in which
    //                   case it is more beneficial to use non-zero strength
    //                   filtering.
    strength = 0;
    frames_to_blur = 1;
  } else {
    adjust_arnr_filter(cpi, distance, rc->gfu_boost, &frames_to_blur,
                       &strength);
  }

  int which_arf = gf_group->arf_update_idx[gf_group->index];

  // Set the temporal filtering status for the corresponding OVERLAY frame
  if (strength == 0 && frames_to_blur == 1)
    cpi->is_arf_filter_off[which_arf] = 1;
  else
    cpi->is_arf_filter_off[which_arf] = 0;
  cpi->common.showable_frame = cpi->is_arf_filter_off[which_arf];

  frames_to_blur_backward = (frames_to_blur / 2);
  frames_to_blur_forward = ((frames_to_blur - 1) / 2);
  start_frame = distance + frames_to_blur_forward;

  // Setup frame pointers, NULL indicates frame not included in filter.
  for (frame = 0; frame < frames_to_blur; ++frame) {
    const int which_buffer = start_frame - frame;
    struct lookahead_entry *buf =
        av1_lookahead_peek(cpi->lookahead, which_buffer);
    frames[frames_to_blur - 1 - frame] = &buf->img;
  }

  if (frames_to_blur > 0) {
    // Setup scaling factors. Scaling on each of the arnr frames is not
    // supported.
    // ARF is produced at the native frame size and resized when coded.
    av1_setup_scale_factors_for_frame(
        &sf, frames[0]->y_crop_width, frames[0]->y_crop_height,
        frames[0]->y_crop_width, frames[0]->y_crop_height);
  }

  // Initialize errorperbit, sadperbit16 and sadperbit4.
  rdmult = av1_compute_rd_mult_based_on_qindex(cpi, ARNR_FILT_QINDEX);
  set_error_per_bit(&cpi->td.mb, rdmult);
  av1_initialize_me_consts(cpi, &cpi->td.mb, ARNR_FILT_QINDEX);
  av1_initialize_cost_tables(&cpi->common, &cpi->td.mb);

  temporal_filter_iterate_c(cpi, frames, frames_to_blur,
                            frames_to_blur_backward, strength, &sf);
}
