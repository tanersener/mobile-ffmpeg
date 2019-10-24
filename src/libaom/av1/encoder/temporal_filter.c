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

#include "av1/common/blockd.h"
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
#include "aom_ports/system_state.h"
#include "aom_scale/aom_scale.h"

#define EXPERIMENT_TEMPORAL_FILTER 1
#define WINDOW_LENGTH 2
#define WINDOW_SIZE 25
#define SCALE 1000

static unsigned int index_mult[14] = { 0,     0,     0,     0,     49152,
                                       39322, 32768, 28087, 24576, 21846,
                                       19661, 17874, 0,     15124 };

static int64_t highbd_index_mult[14] = { 0U,          0U,          0U,
                                         0U,          3221225472U, 2576980378U,
                                         2147483648U, 1840700270U, 1610612736U,
                                         1431655766U, 1288490189U, 1171354718U,
                                         0U,          991146300U };

static void temporal_filter_predictors_mb_c(
    MACROBLOCKD *xd, uint8_t *y_mb_ptr, uint8_t *u_mb_ptr, uint8_t *v_mb_ptr,
    int stride, int uv_block_width, int uv_block_height, int mv_row, int mv_col,
    uint8_t *pred, struct scale_factors *scale, int x, int y,
    int can_use_previous, int num_planes, MV *blk_mvs, int use_32x32) {
  mv_precision mv_precision_uv;
  int uv_stride;
  // TODO(angiebird): change plane setting accordingly
  ConvolveParams conv_params = get_conv_params(0, 0, xd->bd);
  const int_interpfilters interp_filters =
      av1_broadcast_interp_filter(MULTITAP_SHARP);
  WarpTypesAllowed warp_types;
  memset(&warp_types, 0, sizeof(WarpTypesAllowed));

  const int ssx = (uv_block_width == (BW >> 1)) ? 1 : 0;
  if (ssx) {
    uv_stride = (stride + 1) >> 1;
    mv_precision_uv = MV_PRECISION_Q4;
  } else {
    uv_stride = stride;
    mv_precision_uv = MV_PRECISION_Q3;
  }

  if (use_32x32) {
    assert(mv_row >= INT16_MIN && mv_row <= INT16_MAX && mv_col >= INT16_MIN &&
           mv_col <= INT16_MAX);
    const MV mv = { (int16_t)mv_row, (int16_t)mv_col };

    av1_build_inter_predictor(y_mb_ptr, stride, &pred[0], BW, &mv, scale, BW,
                              BH, &conv_params, interp_filters, &warp_types, x,
                              y, 0, 0, MV_PRECISION_Q3, x, y, xd,
                              can_use_previous);
    if (num_planes > 1) {
      av1_build_inter_predictor(
          u_mb_ptr, uv_stride, &pred[BLK_PELS], uv_block_width, &mv, scale,
          uv_block_width, uv_block_height, &conv_params, interp_filters,
          &warp_types, x, y, 1, 0, mv_precision_uv, x, y, xd, can_use_previous);
      av1_build_inter_predictor(
          v_mb_ptr, uv_stride, &pred[(BLK_PELS << 1)], uv_block_width, &mv,
          scale, uv_block_width, uv_block_height, &conv_params, interp_filters,
          &warp_types, x, y, 2, 0, mv_precision_uv, x, y, xd, can_use_previous);
    }

    return;
  }

  // While use_32x32 = 0, construct the 32x32 predictor using 4 16x16
  // predictors.
  int i, j, k = 0, ys = (BH >> 1), xs = (BW >> 1);
  // Y predictor
  for (i = 0; i < BH; i += ys) {
    for (j = 0; j < BW; j += xs) {
      const MV mv = blk_mvs[k];
      const int y_offset = i * stride + j;
      const int p_offset = i * BW + j;

      av1_build_inter_predictor(y_mb_ptr + y_offset, stride, &pred[p_offset],
                                BW, &mv, scale, xs, ys, &conv_params,
                                interp_filters, &warp_types, x, y, 0, 0,
                                MV_PRECISION_Q3, x, y, xd, can_use_previous);
      k++;
    }
  }

  // U and V predictors
  if (num_planes > 1) {
    ys = (uv_block_height >> 1);
    xs = (uv_block_width >> 1);
    k = 0;

    for (i = 0; i < uv_block_height; i += ys) {
      for (j = 0; j < uv_block_width; j += xs) {
        const MV mv = blk_mvs[k];
        const int uv_offset = i * uv_stride + j;
        const int p_offset = i * uv_block_width + j;

        av1_build_inter_predictor(u_mb_ptr + uv_offset, uv_stride,
                                  &pred[BLK_PELS + p_offset], uv_block_width,
                                  &mv, scale, xs, ys, &conv_params,
                                  interp_filters, &warp_types, x, y, 1, 0,
                                  mv_precision_uv, x, y, xd, can_use_previous);
        av1_build_inter_predictor(
            v_mb_ptr + uv_offset, uv_stride, &pred[(BLK_PELS << 1) + p_offset],
            uv_block_width, &mv, scale, xs, ys, &conv_params, interp_filters,
            &warp_types, x, y, 2, 0, mv_precision_uv, x, y, xd,
            can_use_previous);
        k++;
      }
    }
  }
}

static void apply_temporal_filter_self(const uint8_t *pred, int buf_stride,
                                       unsigned int block_width,
                                       unsigned int block_height,
                                       int filter_weight, uint32_t *accumulator,
                                       uint16_t *count,
                                       int use_new_temporal_mode) {
  const int modifier = use_new_temporal_mode ? SCALE : filter_weight * 16;
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
    uint16_t *count, int use_new_temporal_mode) {
  const int modifier = use_new_temporal_mode ? SCALE : filter_weight * 16;
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

static INLINE int mod_index(int sum_dist, int index, int rounding, int strength,
                            int filter_weight) {
  assert(index >= 0 && index <= 13);
  assert(index_mult[index] != 0);

  int mod = (clamp(sum_dist, 0, UINT16_MAX) * index_mult[index]) >> 16;
  mod += rounding;
  mod >>= strength;

  mod = AOMMIN(16, mod);

  mod = 16 - mod;
  mod *= filter_weight;

  return mod;
}

static INLINE int highbd_mod_index(int64_t sum_dist, int index, int rounding,
                                   int strength, int filter_weight) {
  assert(index >= 0 && index <= 13);
  assert(highbd_index_mult[index] != 0);

  int mod =
      (int)((AOMMIN(sum_dist, INT32_MAX) * highbd_index_mult[index]) >> 32);
  mod += rounding;
  mod >>= strength;

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

static INLINE int get_filter_weight(unsigned int i, unsigned int j,
                                    unsigned int block_height,
                                    unsigned int block_width, const int *blk_fw,
                                    int use_32x32) {
  if (use_32x32)
    // blk_fw[0] ~ blk_fw[3] are the same.
    return blk_fw[0];

  int filter_weight = 0;
  if (i < block_height / 2) {
    if (j < block_width / 2)
      filter_weight = blk_fw[0];
    else
      filter_weight = blk_fw[1];
  } else {
    if (j < block_width / 2)
      filter_weight = blk_fw[2];
    else
      filter_weight = blk_fw[3];
  }
  return filter_weight;
}

void av1_apply_temporal_filter_c(
    const uint8_t *y_frame1, int y_stride, const uint8_t *y_pred,
    int y_buf_stride, const uint8_t *u_frame1, const uint8_t *v_frame1,
    int uv_stride, const uint8_t *u_pred, const uint8_t *v_pred,
    int uv_buf_stride, unsigned int block_width, unsigned int block_height,
    int ss_x, int ss_y, int strength, const int *blk_fw, int use_32x32,
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
      int filter_weight =
          get_filter_weight(i, j, block_height, block_width, blk_fw, use_32x32);

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

void av1_highbd_apply_temporal_filter_c(
    const uint8_t *yf, int y_stride, const uint8_t *yp, int y_buf_stride,
    const uint8_t *uf, const uint8_t *vf, int uv_stride, const uint8_t *up,
    const uint8_t *vp, int uv_buf_stride, unsigned int block_width,
    unsigned int block_height, int ss_x, int ss_y, int strength,
    const int *blk_fw, int use_32x32, uint32_t *y_accumulator,
    uint16_t *y_count, uint32_t *u_accumulator, uint16_t *u_count,
    uint32_t *v_accumulator, uint16_t *v_count) {
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
      int filter_weight =
          get_filter_weight(i, j, block_height, block_width, blk_fw, use_32x32);

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

      const int final_y_mod = highbd_mod_index(modifier, y_index, rounding,
                                               strength, filter_weight);

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

        const int final_u_mod = highbd_mod_index(u_mod, cr_index, rounding,
                                                 strength, filter_weight);
        const int final_v_mod = highbd_mod_index(v_mod, cr_index, rounding,
                                                 strength, filter_weight);

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
                                 const int *blk_fw, int use_32x32,
                                 unsigned int *accumulator, uint16_t *count) {
  unsigned int i, j, k;
  int modifier;
  int byte = 0;
  const int rounding = strength > 0 ? 1 << (strength - 1) : 0;

  for (i = 0, k = 0; i < block_height; i++) {
    for (j = 0; j < block_width; j++, k++) {
      int pixel_value = *frame2;
      int filter_weight =
          get_filter_weight(i, j, block_height, block_width, blk_fw, use_32x32);

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
    int *blk_fw, int use_32x32, unsigned int *accumulator, uint16_t *count) {
  uint16_t *frame1 = CONVERT_TO_SHORTPTR(frame1_8);
  uint16_t *frame2 = CONVERT_TO_SHORTPTR(frame2_8);
  unsigned int i, j, k;
  int modifier;
  int byte = 0;
  const int rounding = strength > 0 ? 1 << (strength - 1) : 0;

  for (i = 0, k = 0; i < block_height; i++) {
    for (j = 0; j < block_width; j++, k++) {
      int pixel_value = *frame2;
      int filter_weight =
          get_filter_weight(i, j, block_height, block_width, blk_fw, use_32x32);

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

#if EXPERIMENT_TEMPORAL_FILTER
void av1_temporal_filter_plane_c(uint8_t *frame1, unsigned int stride,
                                 uint8_t *frame2, unsigned int stride2,
                                 int block_width, int block_height,
                                 int strength, double sigma, int decay_control,
                                 const int *blk_fw, int use_32x32,
                                 unsigned int *accumulator, uint16_t *count) {
  (void)strength;
  (void)blk_fw;
  (void)use_32x32;
  const double decay = decay_control * exp(1 - sigma);
  const double h = decay * sigma;
  const double beta = 1.0;
  for (int i = 0, k = 0; i < block_height; i++) {
    for (int j = 0; j < block_width; j++, k++) {
      const int pixel_value = frame2[i * stride2 + j];

      int diff_sse = 0;
      for (int idy = -WINDOW_LENGTH; idy <= WINDOW_LENGTH; ++idy) {
        for (int idx = -WINDOW_LENGTH; idx <= WINDOW_LENGTH; ++idx) {
          int row = i + idy;
          int col = j + idx;
          if (row < 0) row = 0;
          if (row >= block_height) row = block_height - 1;
          if (col < 0) col = 0;
          if (col >= block_width) col = block_width - 1;

          int diff = frame1[row * (int)stride + col] -
                     frame2[row * (int)stride2 + col];
          diff_sse += diff * diff;
        }
      }
      diff_sse /= WINDOW_SIZE;

      double scaled_diff = -diff_sse / (2 * beta * h * h);
      // clamp the value to avoid underflow in exp()
      if (scaled_diff < -15) scaled_diff = -15;
      double w = exp(scaled_diff);
      const int weight = (int)(w * SCALE);

      count[k] += weight;
      accumulator[k] += weight * pixel_value;
    }
  }
}

void av1_highbd_temporal_filter_plane_c(
    uint8_t *frame1_8bit, unsigned int stride, uint8_t *frame2_8bit,
    unsigned int stride2, int block_width, int block_height, int strength,
    double sigma, int decay_control, const int *blk_fw, int use_32x32,
    unsigned int *accumulator, uint16_t *count) {
  (void)strength;
  (void)blk_fw;
  (void)use_32x32;
  uint16_t *frame1 = CONVERT_TO_SHORTPTR(frame1_8bit);
  uint16_t *frame2 = CONVERT_TO_SHORTPTR(frame2_8bit);
  const double decay = decay_control * exp(1 - sigma);
  const double h = decay * sigma;
  const double beta = 1.0;
  for (int i = 0, k = 0; i < block_height; i++) {
    for (int j = 0; j < block_width; j++, k++) {
      const int pixel_value = frame2[i * stride2 + j];

      int diff_sse = 0;
      for (int idy = -WINDOW_LENGTH; idy <= WINDOW_LENGTH; ++idy) {
        for (int idx = -WINDOW_LENGTH; idx <= WINDOW_LENGTH; ++idx) {
          int row = i + idy;
          int col = j + idx;
          if (row < 0) row = 0;
          if (row >= block_height) row = block_height - 1;
          if (col < 0) col = 0;
          if (col >= block_width) col = block_width - 1;

          int diff = frame1[row * (int)stride + col] -
                     frame2[row * (int)stride2 + col];
          diff_sse += diff * diff;
        }
      }
      diff_sse /= WINDOW_SIZE;

      double scaled_diff = -diff_sse / (2 * beta * h * h);
      // clamp the value to avoid underflow in exp()
      if (scaled_diff < -20) scaled_diff = -20;
      double w = exp(scaled_diff);
      const int weight = (int)(w * SCALE);

      count[k] += weight;
      accumulator[k] += weight * pixel_value;
    }
  }
}

void apply_temporal_filter_block(YV12_BUFFER_CONFIG *frame, MACROBLOCKD *mbd,
                                 int mb_y_src_offset, int mb_uv_src_offset,
                                 int mb_uv_width, int mb_uv_height,
                                 int num_planes, uint8_t *predictor,
                                 int frame_height, int strength, double sigma,
                                 int *blk_fw, int use_32x32,
                                 unsigned int *accumulator, uint16_t *count,
                                 int use_new_temporal_mode) {
  const int is_hbd = is_cur_buf_hbd(mbd);
  // High bitdepth
  if (is_hbd) {
    if (use_new_temporal_mode) {
      // Apply frame size dependent non-local means filtering.
      int decay_control;
      // The decay is obtained empirically, subject to better tuning.
      if (frame_height >= 720) {
        decay_control = 7;
      } else if (frame_height >= 480) {
        decay_control = 5;
      } else {
        decay_control = 3;
      }
      av1_highbd_temporal_filter_plane_c(frame->y_buffer + mb_y_src_offset,
                                         frame->y_stride, predictor, BW, BW, BH,
                                         strength, sigma, decay_control, blk_fw,
                                         use_32x32, accumulator, count);
      if (num_planes > 1) {
        av1_highbd_temporal_filter_plane_c(
            frame->u_buffer + mb_uv_src_offset, frame->uv_stride,
            predictor + BLK_PELS, mb_uv_width, mb_uv_width, mb_uv_height,
            strength, sigma, decay_control, blk_fw, use_32x32,
            accumulator + BLK_PELS, count + BLK_PELS);
        av1_highbd_temporal_filter_plane_c(
            frame->v_buffer + mb_uv_src_offset, frame->uv_stride,
            predictor + (BLK_PELS << 1), mb_uv_width, mb_uv_width, mb_uv_height,
            strength, sigma, decay_control, blk_fw, use_32x32,
            accumulator + (BLK_PELS << 1), count + (BLK_PELS << 1));
      }
    } else {
      // Apply original non-local means filtering for small resolution
      const int adj_strength = strength + 2 * (mbd->bd - 8);
      if (num_planes <= 1) {
        // Single plane case
        av1_highbd_temporal_filter_apply_c(
            frame->y_buffer + mb_y_src_offset, frame->y_stride, predictor, BW,
            BH, adj_strength, blk_fw, use_32x32, accumulator, count);
      } else {
        // Process 3 planes together.
        av1_highbd_apply_temporal_filter(
            frame->y_buffer + mb_y_src_offset, frame->y_stride, predictor, BW,
            frame->u_buffer + mb_uv_src_offset,
            frame->v_buffer + mb_uv_src_offset, frame->uv_stride,
            predictor + BLK_PELS, predictor + (BLK_PELS << 1), mb_uv_width, BW,
            BH, mbd->plane[1].subsampling_x, mbd->plane[1].subsampling_y,
            adj_strength, blk_fw, use_32x32, accumulator, count,
            accumulator + BLK_PELS, count + BLK_PELS,
            accumulator + (BLK_PELS << 1), count + (BLK_PELS << 1));
      }
    }
    return;
  }

  // Low bitdepth
  if (use_new_temporal_mode) {
    // Apply frame size dependent non-local means filtering.
    int decay_control;
    // The decay is obtained empirically, subject to better tuning.
    if (frame_height >= 720) {
      decay_control = 7;
    } else if (frame_height >= 480) {
      decay_control = 5;
    } else {
      decay_control = 3;
    }
    av1_temporal_filter_plane_c(frame->y_buffer + mb_y_src_offset,
                                frame->y_stride, predictor, BW, BW, BH,
                                strength, sigma, decay_control, blk_fw,
                                use_32x32, accumulator, count);
    if (num_planes > 1) {
      av1_temporal_filter_plane_c(
          frame->u_buffer + mb_uv_src_offset, frame->uv_stride,
          predictor + BLK_PELS, mb_uv_width, mb_uv_width, mb_uv_height,
          strength, sigma, decay_control, blk_fw, use_32x32,
          accumulator + BLK_PELS, count + BLK_PELS);
      av1_temporal_filter_plane_c(
          frame->v_buffer + mb_uv_src_offset, frame->uv_stride,
          predictor + (BLK_PELS << 1), mb_uv_width, mb_uv_width, mb_uv_height,
          strength, sigma, decay_control, blk_fw, use_32x32,
          accumulator + (BLK_PELS << 1), count + (BLK_PELS << 1));
    }
  } else {
    // Apply original non-local means filtering for small resolution
    if (num_planes <= 1) {
      // Single plane case
      av1_temporal_filter_apply_c(frame->y_buffer + mb_y_src_offset,
                                  frame->y_stride, predictor, BW, BH, strength,
                                  blk_fw, use_32x32, accumulator, count);
    } else {
      // Process 3 planes together.
      av1_apply_temporal_filter(
          frame->y_buffer + mb_y_src_offset, frame->y_stride, predictor, BW,
          frame->u_buffer + mb_uv_src_offset,
          frame->v_buffer + mb_uv_src_offset, frame->uv_stride,
          predictor + BLK_PELS, predictor + (BLK_PELS << 1), mb_uv_width, BW,
          BH, mbd->plane[1].subsampling_x, mbd->plane[1].subsampling_y,
          strength, blk_fw, use_32x32, accumulator, count,
          accumulator + BLK_PELS, count + BLK_PELS,
          accumulator + (BLK_PELS << 1), count + (BLK_PELS << 1));
    }
  }
}
#endif  // EXPERIMENT_TEMPORAL_FILTER

static int temporal_filter_find_matching_mb_c(
    AV1_COMP *cpi, uint8_t *arf_frame_buf, uint8_t *frame_ptr_buf, int stride,
    int x_pos, int y_pos, MV *blk_mvs, int *blk_bestsme, MV *best_ref_mv1,
    int step_param) {
  MACROBLOCK *const x = &cpi->td.mb;
  MACROBLOCKD *const xd = &x->e_mbd;
  const MV_SPEED_FEATURES *const mv_sf = &cpi->sf.mv;
  int sadpb = x->sadperbit16;
  int bestsme = INT_MAX;
  int distortion;
  unsigned int sse;
  int cost_list[5];
  MvLimits tmp_mv_limits = x->mv_limits;
  MV best_ref_mv1_full; /* full-pixel value of best_ref_mv1 */
  MV ref_mv = kZeroMv;
  // Save input state
  struct buf_2d src = x->plane[0].src;
  struct buf_2d pre = xd->plane[0].pre[0];
  best_ref_mv1_full.col = best_ref_mv1->col >> 3;
  best_ref_mv1_full.row = best_ref_mv1->row >> 3;

  // Setup frame pointers
  x->plane[0].src.buf = arf_frame_buf;
  x->plane[0].src.stride = stride;
  xd->plane[0].pre[0].buf = frame_ptr_buf;
  xd->plane[0].pre[0].stride = stride;

  av1_set_mv_search_range(&x->mv_limits, &ref_mv);

  // av1_full_pixel_search() parameters: best_ref_mv1_full is the start mv, and
  // ref_mv is for mv rate calculation. The search result is stored in
  // x->best_mv.
  av1_full_pixel_search(cpi, x, TF_BLOCK, &best_ref_mv1_full, step_param, NSTEP,
                        1, sadpb, cond_cost_list(cpi, cost_list), &ref_mv, 0, 0,
                        x_pos, y_pos, 0, &cpi->ss_cfg[SS_CFG_LOOKAHEAD], 0);
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

    x->e_mbd.mi[0]->mv[0] = x->best_mv;

    // Restore input state
    x->plane[0].src = src;
    xd->plane[0].pre[0] = pre;

    return bestsme;
  }

  // find_fractional_mv_step parameters: ref_mv is for mv rate cost
  // calculation. The start full mv and the search result are stored in
  // x->best_mv. mi_row and mi_col are only needed for "av1_is_scaled(sf)=1"
  // case.
  bestsme = cpi->find_fractional_mv_step(
      x, &cpi->common, 0, 0, &ref_mv, cpi->common.allow_high_precision_mv,
      x->errorperbit, &cpi->fn_ptr[TF_BLOCK], 0, mv_sf->subpel_iters_per_step,
      cond_cost_list(cpi, cost_list), NULL, NULL, &distortion, &sse, NULL, NULL,
      0, 0, BW, BH, USE_8_TAPS, 1);

  x->e_mbd.mi[0]->mv[0] = x->best_mv;

  // DO motion search on 4 16x16 sub_blocks.
  int i, j, k = 0;
  best_ref_mv1->row = x->e_mbd.mi[0]->mv[0].as_mv.row;
  best_ref_mv1->col = x->e_mbd.mi[0]->mv[0].as_mv.col;
  best_ref_mv1_full.col = best_ref_mv1->col >> 3;
  best_ref_mv1_full.row = best_ref_mv1->row >> 3;

  for (i = 0; i < BH; i += SUB_BH) {
    for (j = 0; j < BW; j += SUB_BW) {
      // Setup frame pointers
      x->plane[0].src.buf = arf_frame_buf + i * stride + j;
      x->plane[0].src.stride = stride;
      xd->plane[0].pre[0].buf = frame_ptr_buf + i * stride + j;
      xd->plane[0].pre[0].stride = stride;

      av1_set_mv_search_range(&x->mv_limits, &ref_mv);
      av1_full_pixel_search(cpi, x, TF_SUB_BLOCK, &best_ref_mv1_full,
                            step_param, NSTEP, 1, sadpb,
                            cond_cost_list(cpi, cost_list), &ref_mv, 0, 0,
                            x_pos, y_pos, 0, &cpi->ss_cfg[SS_CFG_LOOKAHEAD], 0);
      x->mv_limits = tmp_mv_limits;

      blk_bestsme[k] = cpi->find_fractional_mv_step(
          x, &cpi->common, 0, 0, &ref_mv, cpi->common.allow_high_precision_mv,
          x->errorperbit, &cpi->fn_ptr[TF_SUB_BLOCK], 0,
          mv_sf->subpel_iters_per_step, cond_cost_list(cpi, cost_list), NULL,
          NULL, &distortion, &sse, NULL, NULL, 0, 0, SUB_BW, SUB_BH, USE_8_TAPS,
          1);

      blk_mvs[k] = x->best_mv.as_mv;
      k++;
    }
  }

  // Restore input state
  x->plane[0].src = src;
  xd->plane[0].pre[0] = pre;

  return bestsme;
}

static int get_rows(int h) { return (h + BH - 1) >> BH_LOG2; }
static int get_cols(int w) { return (w + BW - 1) >> BW_LOG2; }

typedef struct {
  int64_t sum;
  int64_t sse;
} FRAME_DIFF;

static FRAME_DIFF temporal_filter_iterate_c(
    AV1_COMP *cpi, YV12_BUFFER_CONFIG **frames, int frame_count,
    int alt_ref_index, int strength, double sigma, int is_key_frame,
    struct scale_factors *ref_scale_factors) {
  const AV1_COMMON *cm = &cpi->common;
  const int num_planes = av1_num_planes(cm);
  const int mb_cols = get_cols(frames[alt_ref_index]->y_crop_width);
  const int mb_rows = get_rows(frames[alt_ref_index]->y_crop_height);
  // TODO(any): the thresholds in this function need to adjusted based on bit_
  // depth, so that they work better in HBD encoding.
  const int bd_shift = cm->seq_params.bit_depth - 8;
  int byte;
  int frame;
  int mb_col, mb_row;
  int mb_y_offset = 0;
  int mb_y_src_offset = 0;
  int mb_uv_offset = 0;
  int mb_uv_src_offset = 0;
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
#if EXPERIMENT_TEMPORAL_FILTER
  const int is_screen_content_type = cm->allow_screen_content_tools != 0;
  const int use_new_temporal_mode = AOMMIN(cm->width, cm->height) >= 480 &&
                                    !is_screen_content_type && !is_key_frame;
#else
  (void)sigma;
  const int use_new_temporal_mode = 0;
#endif

  // Save input state
  uint8_t *input_buffer[MAX_MB_PLANE];
  int i;
  const int is_hbd = is_cur_buf_hbd(mbd);
  if (is_hbd) {
    predictor = CONVERT_TO_BYTEPTR(predictor16);
  } else {
    predictor = predictor8;
  }

  const unsigned int dim = AOMMIN(frames[alt_ref_index]->y_crop_width,
                                  frames[alt_ref_index]->y_crop_height);
  // Decide search param based on image resolution.
  const int step_param = av1_init_search_range(dim);

  mbd->block_ref_scale_factors[0] = ref_scale_factors;
  mbd->block_ref_scale_factors[1] = ref_scale_factors;

  for (i = 0; i < num_planes; i++) input_buffer[i] = mbd->plane[i].pre[0].buf;

  // Make a temporary mbmi for temporal filtering
  MB_MODE_INFO **backup_mi_grid = mbd->mi;
  MB_MODE_INFO mbmi;
  memset(&mbmi, 0, sizeof(mbmi));
  MB_MODE_INFO *mbmi_ptr = &mbmi;
  mbd->mi = &mbmi_ptr;

  FRAME_DIFF diff = { 0, 0 };

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
      MV best_ref_mv1 = kZeroMv;

      memset(accumulator, 0, BLK_PELS * 3 * sizeof(accumulator[0]));
      memset(count, 0, BLK_PELS * 3 * sizeof(count[0]));

      cpi->td.mb.mv_limits.col_min =
          -((mb_col * BW) + (17 - 2 * AOM_INTERP_EXTEND));
      cpi->td.mb.mv_limits.col_max =
          ((mb_cols - 1 - mb_col) * BW) + (17 - 2 * AOM_INTERP_EXTEND);

      for (frame = 0; frame < frame_count; frame++) {
        // MVs for 4 16x16 sub blocks.
        MV blk_mvs[4];
        // Filter weights for 4 16x16 sub blocks.
        int blk_fw[4] = { 0, 0, 0, 0 };
        int use_32x32 = 0;

        if (frames[frame] == NULL) continue;

        mbd->mi[0]->mv[0].as_mv.row = 0;
        mbd->mi[0]->mv[0].as_mv.col = 0;
        mbd->mi[0]->motion_mode = SIMPLE_TRANSLATION;
        blk_mvs[0] = kZeroMv;
        blk_mvs[1] = kZeroMv;
        blk_mvs[2] = kZeroMv;
        blk_mvs[3] = kZeroMv;

        if (frame == alt_ref_index) {
          blk_fw[0] = blk_fw[1] = blk_fw[2] = blk_fw[3] = 2;
          use_32x32 = 1;
          // Change ref_mv sign for following frames.
          best_ref_mv1.row *= -1;
          best_ref_mv1.col *= -1;
        } else {
          int thresh_low = 10000;
          int thresh_high = 20000;
          int blk_bestsme[4] = { INT_MAX, INT_MAX, INT_MAX, INT_MAX };

          // Find best match in this frame by MC
          int err = temporal_filter_find_matching_mb_c(
              cpi, frames[alt_ref_index]->y_buffer + mb_y_src_offset,
              frames[frame]->y_buffer + mb_y_src_offset,
              frames[frame]->y_stride, mb_col * BW, mb_row * BH, blk_mvs,
              blk_bestsme, &best_ref_mv1, step_param);

          int err16 =
              blk_bestsme[0] + blk_bestsme[1] + blk_bestsme[2] + blk_bestsme[3];
          int max_err = INT_MIN, min_err = INT_MAX;
          for (k = 0; k < 4; k++) {
            if (min_err > blk_bestsme[k]) min_err = blk_bestsme[k];
            if (max_err < blk_bestsme[k]) max_err = blk_bestsme[k];
          }

          if (((err * 15 < (err16 << 4)) && max_err - min_err < 12000) ||
              ((err * 14 < (err16 << 4)) && max_err - min_err < 6000)) {
            use_32x32 = 1;
            // Assign higher weight to matching MB if it's error
            // score is lower. If not applying MC default behavior
            // is to weight all MBs equal.
            blk_fw[0] = err < (thresh_low << THR_SHIFT)
                            ? 2
                            : err < (thresh_high << THR_SHIFT) ? 1 : 0;
            blk_fw[1] = blk_fw[2] = blk_fw[3] = blk_fw[0];
          } else {
            use_32x32 = 0;
            for (k = 0; k < 4; k++)
              blk_fw[k] = blk_bestsme[k] < thresh_low
                              ? 2
                              : blk_bestsme[k] < thresh_high ? 1 : 0;
          }

          // Don't use previous frame's mv result if error is large.
          if (err > (3000 << bd_shift)) best_ref_mv1 = kZeroMv;
        }

        if (blk_fw[0] || blk_fw[1] || blk_fw[2] || blk_fw[3]) {
          // Construct the predictors
          temporal_filter_predictors_mb_c(
              mbd, frames[frame]->y_buffer + mb_y_src_offset,
              frames[frame]->u_buffer + mb_uv_src_offset,
              frames[frame]->v_buffer + mb_uv_src_offset,
              frames[frame]->y_stride, mb_uv_width, mb_uv_height,
              mbd->mi[0]->mv[0].as_mv.row, mbd->mi[0]->mv[0].as_mv.col,
              predictor, ref_scale_factors, mb_col * BW, mb_row * BH,
              cm->allow_warped_motion, num_planes, blk_mvs, use_32x32);

          // Apply the filter (YUV)
          if (frame == alt_ref_index) {
            uint8_t *pred = predictor;
            uint32_t *accum = accumulator;
            uint16_t *cnt = count;
            int plane;

            // All 4 blk_fws are equal to 2.
            for (plane = 0; plane < num_planes; ++plane) {
              const int pred_stride = plane ? mb_uv_width : BW;
              const unsigned int w = plane ? mb_uv_width : BW;
              const unsigned int h = plane ? mb_uv_height : BH;

              if (is_hbd) {
                highbd_apply_temporal_filter_self(pred, pred_stride, w, h,
                                                  blk_fw[0], accum, cnt,
                                                  use_new_temporal_mode);
              } else {
                apply_temporal_filter_self(pred, pred_stride, w, h, blk_fw[0],
                                           accum, cnt, use_new_temporal_mode);
              }

              pred += BLK_PELS;
              accum += BLK_PELS;
              cnt += BLK_PELS;
            }
          } else {
            if (is_hbd) {
#if EXPERIMENT_TEMPORAL_FILTER
              apply_temporal_filter_block(
                  f, mbd, mb_y_src_offset, mb_uv_src_offset, mb_uv_width,
                  mb_uv_height, num_planes, predictor, cm->height, strength,
                  sigma, blk_fw, use_32x32, accumulator, count,
                  use_new_temporal_mode);
#else
              const int adj_strength = strength + 2 * (mbd->bd - 8);
              if (num_planes <= 1) {
                // Single plane case
                av1_highbd_temporal_filter_apply_c(
                    f->y_buffer + mb_y_src_offset, f->y_stride, predictor, BW,
                    BH, adj_strength, blk_fw, use_32x32, accumulator, count);
              } else {
                // Process 3 planes together.
                av1_highbd_apply_temporal_filter(
                    f->y_buffer + mb_y_src_offset, f->y_stride, predictor, BW,
                    f->u_buffer + mb_uv_src_offset,
                    f->v_buffer + mb_uv_src_offset, f->uv_stride,
                    predictor + BLK_PELS, predictor + (BLK_PELS << 1),
                    mb_uv_width, BW, BH, mbd->plane[1].subsampling_x,
                    mbd->plane[1].subsampling_y, adj_strength, blk_fw,
                    use_32x32, accumulator, count, accumulator + BLK_PELS,
                    count + BLK_PELS, accumulator + (BLK_PELS << 1),
                    count + (BLK_PELS << 1));
              }
#endif  // EXPERIMENT_TEMPORAL_FILTER
            } else {
#if EXPERIMENT_TEMPORAL_FILTER
              apply_temporal_filter_block(
                  f, mbd, mb_y_src_offset, mb_uv_src_offset, mb_uv_width,
                  mb_uv_height, num_planes, predictor, cm->height, strength,
                  sigma, blk_fw, use_32x32, accumulator, count,
                  use_new_temporal_mode);
#else
              if (num_planes <= 1) {
                // Single plane case
                av1_temporal_filter_apply_c(
                    f->y_buffer + mb_y_src_offset, f->y_stride, predictor, BW,
                    BH, strength, blk_fw, use_32x32, accumulator, count);
              } else {
                // Process 3 planes together.
                av1_apply_temporal_filter(
                    f->y_buffer + mb_y_src_offset, f->y_stride, predictor, BW,
                    f->u_buffer + mb_uv_src_offset,
                    f->v_buffer + mb_uv_src_offset, f->uv_stride,
                    predictor + BLK_PELS, predictor + (BLK_PELS << 1),
                    mb_uv_width, BW, BH, mbd->plane[1].subsampling_x,
                    mbd->plane[1].subsampling_y, strength, blk_fw, use_32x32,
                    accumulator, count, accumulator + BLK_PELS,
                    count + BLK_PELS, accumulator + (BLK_PELS << 1),
                    count + (BLK_PELS << 1));
              }
#endif  // EXPERIMENT_TEMPORAL_FILTER
            }
          }
        }
      }

      // Normalize filter output to produce AltRef frame
      if (is_hbd) {
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

      if (!is_key_frame && cpi->sf.adaptive_overlay_encoding) {
        // Calculate the difference(dist) between source and filtered source.
        dst1 = cpi->alt_ref_buffer.y_buffer + mb_y_offset;
        stride = cpi->alt_ref_buffer.y_stride;
        const uint8_t *src = f->y_buffer + mb_y_src_offset;
        const int src_stride = f->y_stride;
        const BLOCK_SIZE bsize = dims_to_size(BW, BH);
        unsigned int sse = 0;
        cpi->fn_ptr[bsize].vf(src, src_stride, dst1, stride, &sse);

        diff.sum += sse;
        diff.sse += sse * sse;
      }

      mb_y_offset += BW;
      mb_y_src_offset += BW;
      mb_uv_offset += mb_uv_width;
      mb_uv_src_offset += mb_uv_width;
    }
    mb_y_offset += BH * cpi->alt_ref_buffer.y_stride - BW * mb_cols;
    mb_y_src_offset += BH * f->y_stride - BW * mb_cols;
    mb_uv_src_offset += mb_uv_height * f->uv_stride - mb_uv_width * mb_cols;
    mb_uv_offset +=
        mb_uv_height * cpi->alt_ref_buffer.uv_stride - mb_uv_width * mb_cols;
  }

  // Restore input state
  for (i = 0; i < num_planes; i++) mbd->plane[i].pre[0].buf = input_buffer[i];

  mbd->mi = backup_mi_grid;
  return diff;
}

// This is an adaptation of the mehtod in the following paper:
// Shen-Chuan Tai, Shih-Ming Yang, "A fast method for image noise
// estimation using Laplacian operator and adaptive edge detection,"
// Proc. 3rd International Symposium on Communications, Control and
// Signal Processing, 2008, St Julians, Malta.
//
// Return noise estimate, or -1.0 if there was a failure
double estimate_noise(const uint8_t *src, int width, int height, int stride,
                      int edge_thresh) {
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
double highbd_estimate_noise(const uint8_t *src8, int width, int height,
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

static int estimate_strength(AV1_COMP *cpi, int distance, int group_boost,
                             double *sigma) {
  // Adjust the strength based on active max q.
  int q;
  if (cpi->common.current_frame.frame_number > 1)
    q = ((int)av1_convert_qindex_to_q(cpi->rc.avg_frame_qindex[INTER_FRAME],
                                      cpi->common.seq_params.bit_depth));
  else
    q = ((int)av1_convert_qindex_to_q(cpi->rc.avg_frame_qindex[KEY_FRAME],
                                      cpi->common.seq_params.bit_depth));
  MACROBLOCKD *mbd = &cpi->td.mb.e_mbd;
  struct lookahead_entry *buf = av1_lookahead_peek(cpi->lookahead, distance);
  int strength;
  double noiselevel;
  if (is_cur_buf_hbd(mbd)) {
    noiselevel = highbd_estimate_noise(
        buf->img.y_buffer, buf->img.y_crop_width, buf->img.y_crop_height,
        buf->img.y_stride, mbd->bd, EDGE_THRESHOLD);
    *sigma = noiselevel;
  } else {
    noiselevel = estimate_noise(buf->img.y_buffer, buf->img.y_crop_width,
                                buf->img.y_crop_height, buf->img.y_stride,
                                EDGE_THRESHOLD);
    *sigma = noiselevel;
  }
  int adj_strength = cpi->oxcf.arnr_strength;
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

  if (strength > group_boost / 300) {
    strength = group_boost / 300;
  }

  return strength;
}

// Apply buffer limits and context specific adjustments to arnr filter.
static void adjust_arnr_filter(AV1_COMP *cpi, int distance, int group_boost,
                               int *arnr_frames, int *arnr_strength,
                               double *sigma, int *frm_bwd, int *frm_fwd) {
  int frames = cpi->oxcf.arnr_max_frames;

  // Adjust number of frames in filter and strength based on gf boost level.
  if (frames > group_boost / 150) {
    frames = group_boost / 150;
    frames += !(frames & 1);
  }

  const int frames_after_arf =
      av1_lookahead_depth(cpi->lookahead) - distance - 1;
  int frames_fwd = (frames - 1) >> 1;
  int frames_bwd = frames >> 1;

  // Define the forward and backwards filter limits for this arnr group.
  if (frames_fwd > frames_after_arf) frames_fwd = frames_after_arf;
  if (frames_bwd > distance) frames_bwd = distance;

  // Set the baseline active filter size.
  frames = frames_bwd + 1 + frames_fwd;

  *arnr_frames = frames;
  *arnr_strength = estimate_strength(cpi, distance, group_boost, sigma);
  *frm_bwd = frames_bwd;
  *frm_fwd = frames_fwd;
}

int av1_temporal_filter(AV1_COMP *cpi, int distance,
                        int *show_existing_alt_ref) {
  RATE_CONTROL *const rc = &cpi->rc;
  int frame;
  int frames_to_blur;
  int start_frame;
  int strength;
  int frames_to_blur_backward;
  int frames_to_blur_forward;
  struct scale_factors sf;

  YV12_BUFFER_CONFIG *frames[MAX_LAG_BUFFERS] = { NULL };
  const GF_GROUP *const gf_group = &cpi->gf_group;
  int rdmult = 0;
  double sigma = 0;

  // TODO(yunqing): For INTNL_ARF_UPDATE type, the following me initialization
  // is used somewhere unexpectedly. Should be resolved later.
  // Initialize errorperbit, sadperbit16 and sadperbit4.
  rdmult = av1_compute_rd_mult_based_on_qindex(cpi, ARNR_FILT_QINDEX);
  set_error_per_bit(&cpi->td.mb, rdmult);
  av1_initialize_me_consts(cpi, &cpi->td.mb, ARNR_FILT_QINDEX);
  av1_fill_mv_costs(cpi->common.fc, cpi->common.cur_frame_force_integer_mv,
                    cpi->common.allow_high_precision_mv, &cpi->td.mb);

  // Apply context specific adjustments to the arnr filter parameters.
  if (gf_group->update_type[gf_group->index] == INTNL_ARF_UPDATE) {
    // TODO(weitinglin): Currently, we enforce the filtering strength on
    // internal ARFs to be zeros. We should investigate in which case it is more
    // beneficial to use non-zero strength filtering.
    strength = 0;
    frames_to_blur = 1;
    return 0;
  }

  if (distance == -1) {
    // Apply temporal filtering on key frame.
    strength = estimate_strength(cpi, distance, rc->gfu_boost, &sigma);
    // Number of frames for temporal filtering, could be tuned.
    frames_to_blur = NUM_KEY_FRAME_DENOISING;
    frames_to_blur_backward = 0;
    frames_to_blur_forward = frames_to_blur - 1;
    start_frame = distance + frames_to_blur_forward;
  } else {
    adjust_arnr_filter(cpi, distance, rc->gfu_boost, &frames_to_blur, &strength,
                       &sigma, &frames_to_blur_backward,
                       &frames_to_blur_forward);
    start_frame = distance + frames_to_blur_forward;
  }

  cpi->common.showable_frame =
      (strength == 0 && frames_to_blur == 1) ||
      (cpi->oxcf.enable_overlay == 0 || cpi->sf.disable_overlay_frames);

  // Setup frame pointers, NULL indicates frame not included in filter.
  for (frame = 0; frame < frames_to_blur; ++frame) {
    const int which_buffer = start_frame - frame;
    struct lookahead_entry *buf =
        av1_lookahead_peek(cpi->lookahead, which_buffer);
    if (buf == NULL) {
      frames[frames_to_blur - 1 - frame] = NULL;
    } else {
      frames[frames_to_blur - 1 - frame] = &buf->img;
    }
  }

  if (frames_to_blur > 0 && frames[0] != NULL) {
    // Setup scaling factors. Scaling on each of the arnr frames is not
    // supported.
    // ARF is produced at the native frame size and resized when coded.
    av1_setup_scale_factors_for_frame(
        &sf, frames[0]->y_crop_width, frames[0]->y_crop_height,
        frames[0]->y_crop_width, frames[0]->y_crop_height);
  }

  FRAME_DIFF diff = temporal_filter_iterate_c(cpi, frames, frames_to_blur,
                                              frames_to_blur_backward, strength,
                                              sigma, distance == -1, &sf);

  if (distance == -1) return 1;

  if (show_existing_alt_ref != NULL && cpi->sf.adaptive_overlay_encoding) {
    AV1_COMMON *const cm = &cpi->common;
    int top_index = 0, bottom_index = 0;

    aom_clear_system_state();
    // TODO(yunqing): This can be combined with TPL q calculation later.
    cpi->rc.base_frame_target = gf_group->bit_allocation[gf_group->index];
    av1_set_target_rate(cpi, cm->width, cm->height);
    const int q = av1_rc_pick_q_and_bounds(cpi, &cpi->rc, cpi->oxcf.width,
                                           cpi->oxcf.height, gf_group->index,
                                           &bottom_index, &top_index);
    const int ac_q = av1_ac_quant_QTX(q, 0, cm->seq_params.bit_depth);
    const int ac_q_2 = ac_q * ac_q;
    const int mb_cols = get_cols(frames[frames_to_blur_backward]->y_crop_width);
    const int mb_rows =
        get_rows(frames[frames_to_blur_backward]->y_crop_height);
    const int mbs = AOMMAX(1, mb_rows * mb_cols);
    const float mean = (float)diff.sum / mbs;
    const float std = (float)sqrt((float)diff.sse / mbs - mean * mean);
    const float threshold = 0.7f;

    *show_existing_alt_ref = 0;
    if (mean / ac_q_2 < threshold && std < mean * 1.2)
      *show_existing_alt_ref = 1;
    cpi->common.showable_frame |= *show_existing_alt_ref;
  }

  return 1;
}
