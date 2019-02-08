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

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>
#include <assert.h>

#include "config/aom_dsp_rtcd.h"

#include "av1/encoder/global_motion.h"

#include "av1/common/convolve.h"
#include "av1/common/resize.h"
#include "av1/common/warped_motion.h"

#include "av1/encoder/segmentation.h"
#include "av1/encoder/corner_detect.h"
#include "av1/encoder/corner_match.h"
#include "av1/encoder/ransac.h"

#define MAX_CORNERS 4096
#define MIN_INLIER_PROB 0.1

#define MIN_TRANS_THRESH (1 * GM_TRANS_DECODE_FACTOR)
#define USE_GM_FEATURE_BASED 1

// Border over which to compute the global motion
#define ERRORADV_BORDER 0

// Number of pyramid levels in disflow computation
#define N_LEVELS 2
// Size of square patches in the disflow dense grid
#define PATCH_SIZE 8
// Center point of square patch
#define PATCH_CENTER ((PATCH_SIZE + 1) >> 1)
// Step size between patches, lower value means greater patch overlap
#define PATCH_STEP 1
// Minimum size of border padding for disflow
#define MIN_PAD 7
// Warp error convergence threshold for disflow
#define DISFLOW_ERROR_TR 0.01
// Max number of iterations if warp convergence is not found
#define DISFLOW_MAX_ITR 10

// Struct for an image pyramid
typedef struct {
  int n_levels;
  int pad_size;
  int has_gradient;
  int widths[N_LEVELS];
  int heights[N_LEVELS];
  int strides[N_LEVELS];
  int level_loc[N_LEVELS];
  unsigned char *level_buffer;
  double *level_dx_buffer;
  double *level_dy_buffer;
} ImagePyramid;

static const double erroradv_tr[] = { 0.65, 0.60, 0.55 };
static const double erroradv_prod_tr[] = { 20000, 18000, 16000 };

int av1_is_enough_erroradvantage(double best_erroradvantage, int params_cost,
                                 int erroradv_type) {
  assert(erroradv_type < GM_ERRORADV_TR_TYPES);
  return best_erroradvantage < erroradv_tr[erroradv_type] &&
         best_erroradvantage * params_cost < erroradv_prod_tr[erroradv_type];
}

static void convert_to_params(const double *params, int32_t *model) {
  int i;
  int alpha_present = 0;
  model[0] = (int32_t)floor(params[0] * (1 << GM_TRANS_PREC_BITS) + 0.5);
  model[1] = (int32_t)floor(params[1] * (1 << GM_TRANS_PREC_BITS) + 0.5);
  model[0] = (int32_t)clamp(model[0], GM_TRANS_MIN, GM_TRANS_MAX) *
             GM_TRANS_DECODE_FACTOR;
  model[1] = (int32_t)clamp(model[1], GM_TRANS_MIN, GM_TRANS_MAX) *
             GM_TRANS_DECODE_FACTOR;

  for (i = 2; i < 6; ++i) {
    const int diag_value = ((i == 2 || i == 5) ? (1 << GM_ALPHA_PREC_BITS) : 0);
    model[i] = (int32_t)floor(params[i] * (1 << GM_ALPHA_PREC_BITS) + 0.5);
    model[i] =
        (int32_t)clamp(model[i] - diag_value, GM_ALPHA_MIN, GM_ALPHA_MAX);
    alpha_present |= (model[i] != 0);
    model[i] = (model[i] + diag_value) * GM_ALPHA_DECODE_FACTOR;
  }
  for (; i < 8; ++i) {
    model[i] = (int32_t)floor(params[i] * (1 << GM_ROW3HOMO_PREC_BITS) + 0.5);
    model[i] = (int32_t)clamp(model[i], GM_ROW3HOMO_MIN, GM_ROW3HOMO_MAX) *
               GM_ROW3HOMO_DECODE_FACTOR;
    alpha_present |= (model[i] != 0);
  }

  if (!alpha_present) {
    if (abs(model[0]) < MIN_TRANS_THRESH && abs(model[1]) < MIN_TRANS_THRESH) {
      model[0] = 0;
      model[1] = 0;
    }
  }
}

void av1_convert_model_to_params(const double *params,
                                 WarpedMotionParams *model) {
  convert_to_params(params, model->wmmat);
  model->wmtype = get_wmtype(model);
  model->invalid = 0;
}

// Adds some offset to a global motion parameter and handles
// all of the necessary precision shifts, clamping, and
// zero-centering.
static int32_t add_param_offset(int param_index, int32_t param_value,
                                int32_t offset) {
  const int scale_vals[3] = { GM_TRANS_PREC_DIFF, GM_ALPHA_PREC_DIFF,
                              GM_ROW3HOMO_PREC_DIFF };
  const int clamp_vals[3] = { GM_TRANS_MAX, GM_ALPHA_MAX, GM_ROW3HOMO_MAX };
  // type of param: 0 - translation, 1 - affine, 2 - homography
  const int param_type = (param_index < 2 ? 0 : (param_index < 6 ? 1 : 2));
  const int is_one_centered = (param_index == 2 || param_index == 5);

  // Make parameter zero-centered and offset the shift that was done to make
  // it compatible with the warped model
  param_value = (param_value - (is_one_centered << WARPEDMODEL_PREC_BITS)) >>
                scale_vals[param_type];
  // Add desired offset to the rescaled/zero-centered parameter
  param_value += offset;
  // Clamp the parameter so it does not overflow the number of bits allotted
  // to it in the bitstream
  param_value = (int32_t)clamp(param_value, -clamp_vals[param_type],
                               clamp_vals[param_type]);
  // Rescale the parameter to WARPEDMODEL_PRECISION_BITS so it is compatible
  // with the warped motion library
  param_value *= (1 << scale_vals[param_type]);

  // Undo the zero-centering step if necessary
  return param_value + (is_one_centered << WARPEDMODEL_PREC_BITS);
}

static void force_wmtype(WarpedMotionParams *wm, TransformationType wmtype) {
  switch (wmtype) {
    case IDENTITY:
      wm->wmmat[0] = 0;
      wm->wmmat[1] = 0;
      AOM_FALLTHROUGH_INTENDED;
    case TRANSLATION:
      wm->wmmat[2] = 1 << WARPEDMODEL_PREC_BITS;
      wm->wmmat[3] = 0;
      AOM_FALLTHROUGH_INTENDED;
    case ROTZOOM:
      wm->wmmat[4] = -wm->wmmat[3];
      wm->wmmat[5] = wm->wmmat[2];
      AOM_FALLTHROUGH_INTENDED;
    case AFFINE: wm->wmmat[6] = wm->wmmat[7] = 0; break;
    default: assert(0);
  }
  wm->wmtype = wmtype;
}

int64_t av1_refine_integerized_param(WarpedMotionParams *wm,
                                     TransformationType wmtype, int use_hbd,
                                     int bd, uint8_t *ref, int r_width,
                                     int r_height, int r_stride, uint8_t *dst,
                                     int d_width, int d_height, int d_stride,
                                     int n_refinements,
                                     int64_t best_frame_error) {
  static const int max_trans_model_params[TRANS_TYPES] = { 0, 2, 4, 6 };
  const int border = ERRORADV_BORDER;
  int i = 0, p;
  int n_params = max_trans_model_params[wmtype];
  int32_t *param_mat = wm->wmmat;
  int64_t step_error, best_error;
  int32_t step;
  int32_t *param;
  int32_t curr_param;
  int32_t best_param;

  force_wmtype(wm, wmtype);
  best_error = av1_warp_error(wm, use_hbd, bd, ref, r_width, r_height, r_stride,
                              dst + border * d_stride + border, border, border,
                              d_width - 2 * border, d_height - 2 * border,
                              d_stride, 0, 0, best_frame_error);
  best_error = AOMMIN(best_error, best_frame_error);
  step = 1 << (n_refinements - 1);
  for (i = 0; i < n_refinements; i++, step >>= 1) {
    for (p = 0; p < n_params; ++p) {
      int step_dir = 0;
      // Skip searches for parameters that are forced to be 0
      param = param_mat + p;
      curr_param = *param;
      best_param = curr_param;
      // look to the left
      *param = add_param_offset(p, curr_param, -step);
      step_error =
          av1_warp_error(wm, use_hbd, bd, ref, r_width, r_height, r_stride,
                         dst + border * d_stride + border, border, border,
                         d_width - 2 * border, d_height - 2 * border, d_stride,
                         0, 0, best_error);
      if (step_error < best_error) {
        best_error = step_error;
        best_param = *param;
        step_dir = -1;
      }

      // look to the right
      *param = add_param_offset(p, curr_param, step);
      step_error =
          av1_warp_error(wm, use_hbd, bd, ref, r_width, r_height, r_stride,
                         dst + border * d_stride + border, border, border,
                         d_width - 2 * border, d_height - 2 * border, d_stride,
                         0, 0, best_error);
      if (step_error < best_error) {
        best_error = step_error;
        best_param = *param;
        step_dir = 1;
      }
      *param = best_param;

      // look to the direction chosen above repeatedly until error increases
      // for the biggest step size
      while (step_dir) {
        *param = add_param_offset(p, best_param, step * step_dir);
        step_error =
            av1_warp_error(wm, use_hbd, bd, ref, r_width, r_height, r_stride,
                           dst + border * d_stride + border, border, border,
                           d_width - 2 * border, d_height - 2 * border,
                           d_stride, 0, 0, best_error);
        if (step_error < best_error) {
          best_error = step_error;
          best_param = *param;
        } else {
          *param = best_param;
          step_dir = 0;
        }
      }
    }
  }
  force_wmtype(wm, wmtype);
  wm->wmtype = get_wmtype(wm);
  return best_error;
}

static INLINE RansacFunc get_ransac_type(TransformationType type) {
  switch (type) {
    case AFFINE: return ransac_affine;
    case ROTZOOM: return ransac_rotzoom;
    case TRANSLATION: return ransac_translation;
    default: assert(0); return NULL;
  }
}

static unsigned char *downconvert_frame(YV12_BUFFER_CONFIG *frm,
                                        int bit_depth) {
  int i, j;
  uint16_t *orig_buf = CONVERT_TO_SHORTPTR(frm->y_buffer);
  uint8_t *buf_8bit = frm->y_buffer_8bit;
  assert(buf_8bit);
  if (!frm->buf_8bit_valid) {
    for (i = 0; i < frm->y_height; ++i) {
      for (j = 0; j < frm->y_width; ++j) {
        buf_8bit[i * frm->y_stride + j] =
            orig_buf[i * frm->y_stride + j] >> (bit_depth - 8);
      }
    }
    frm->buf_8bit_valid = 1;
  }
  return buf_8bit;
}

#if USE_GM_FEATURE_BASED
static int compute_global_motion_feature_based(
    TransformationType type, YV12_BUFFER_CONFIG *frm, YV12_BUFFER_CONFIG *ref,
    int bit_depth, int *num_inliers_by_motion, double *params_by_motion,
    int num_motions) {
  int i;
  int num_frm_corners, num_ref_corners;
  int num_correspondences;
  int *correspondences;
  int frm_corners[2 * MAX_CORNERS], ref_corners[2 * MAX_CORNERS];
  unsigned char *frm_buffer = frm->y_buffer;
  unsigned char *ref_buffer = ref->y_buffer;
  RansacFunc ransac = get_ransac_type(type);

  if (frm->flags & YV12_FLAG_HIGHBITDEPTH) {
    // The frame buffer is 16-bit, so we need to convert to 8 bits for the
    // following code. We cache the result until the frame is released.
    frm_buffer = downconvert_frame(frm, bit_depth);
  }
  if (ref->flags & YV12_FLAG_HIGHBITDEPTH) {
    ref_buffer = downconvert_frame(ref, bit_depth);
  }

  // compute interest points in images using FAST features
  num_frm_corners = fast_corner_detect(frm_buffer, frm->y_width, frm->y_height,
                                       frm->y_stride, frm_corners, MAX_CORNERS);
  num_ref_corners = fast_corner_detect(ref_buffer, ref->y_width, ref->y_height,
                                       ref->y_stride, ref_corners, MAX_CORNERS);

  // find correspondences between the two images
  correspondences =
      (int *)malloc(num_frm_corners * 4 * sizeof(*correspondences));
  num_correspondences = determine_correspondence(
      frm_buffer, (int *)frm_corners, num_frm_corners, ref_buffer,
      (int *)ref_corners, num_ref_corners, frm->y_width, frm->y_height,
      frm->y_stride, ref->y_stride, correspondences);

  ransac(correspondences, num_correspondences, num_inliers_by_motion,
         params_by_motion, num_motions);

  free(correspondences);

  // Set num_inliers = 0 for motions with too few inliers so they are ignored.
  for (i = 0; i < num_motions; ++i) {
    if (num_inliers_by_motion[i] < MIN_INLIER_PROB * num_correspondences) {
      num_inliers_by_motion[i] = 0;
    }
  }

  // Return true if any one of the motions has inliers.
  for (i = 0; i < num_motions; ++i) {
    if (num_inliers_by_motion[i] > 0) return 1;
  }
  return 0;
}
#else
static INLINE RansacFuncDouble
get_ransac_double_prec_type(TransformationType type) {
  switch (type) {
    case AFFINE: return ransac_affine_double_prec;
    case ROTZOOM: return ransac_rotzoom_double_prec;
    case TRANSLATION: return ransac_translation_double_prec;
    default: assert(0); return NULL;
  }
}

// Don't use points around the frame border since they are less reliable
static INLINE int valid_point(int x, int y, int width, int height) {
  return (x > (PATCH_SIZE + PATCH_CENTER)) &&
         (x < (width - PATCH_SIZE - PATCH_CENTER)) &&
         (y > (PATCH_SIZE + PATCH_CENTER)) &&
         (y < (height - PATCH_SIZE - PATCH_CENTER));
}

static int determine_disflow_correspondence(int *frm_corners,
                                            int num_frm_corners, double *flow_u,
                                            double *flow_v, int width,
                                            int height, int stride,
                                            double *correspondences) {
  int num_correspondences = 0;
  int x, y;
  for (int i = 0; i < num_frm_corners; ++i) {
    x = frm_corners[2 * i];
    y = frm_corners[2 * i + 1];
    if (valid_point(x, y, width, height)) {
      correspondences[4 * num_correspondences] = x;
      correspondences[4 * num_correspondences + 1] = y;
      correspondences[4 * num_correspondences + 2] = x + flow_u[y * stride + x];
      correspondences[4 * num_correspondences + 3] = y + flow_v[y * stride + x];
      num_correspondences++;
    }
  }
  return num_correspondences;
}

double getCubicValue(double p[4], double x) {
  return p[1] + 0.5 * x *
                    (p[2] - p[0] +
                     x * (2.0 * p[0] - 5.0 * p[1] + 4.0 * p[2] - p[3] +
                          x * (3.0 * (p[1] - p[2]) + p[3] - p[0])));
}

void get_subcolumn(unsigned char *ref, double col[4], int stride, int x,
                   int y_start) {
  int i;
  for (i = 0; i < 4; ++i) {
    col[i] = ref[(i + y_start) * stride + x];
  }
}

double bicubic(unsigned char *ref, double x, double y, int stride) {
  double arr[4];
  int k;
  int i = (int)x;
  int j = (int)y;
  for (k = 0; k < 4; ++k) {
    double arr_temp[4];
    get_subcolumn(ref, arr_temp, stride, i + k - 1, j - 1);
    arr[k] = getCubicValue(arr_temp, y - j);
  }
  return getCubicValue(arr, x - i);
}

// Interpolate a warped block using bicubic interpolation when possible
unsigned char interpolate(unsigned char *ref, double x, double y, int width,
                          int height, int stride) {
  if (x < 0 && y < 0)
    return ref[0];
  else if (x < 0 && y > height - 1)
    return ref[(height - 1) * stride];
  else if (x > width - 1 && y < 0)
    return ref[width - 1];
  else if (x > width - 1 && y > height - 1)
    return ref[(height - 1) * stride + (width - 1)];
  else if (x < 0) {
    int v;
    int i = (int)y;
    double a = y - i;
    if (y > 1 && y < height - 2) {
      double arr[4];
      get_subcolumn(ref, arr, stride, 0, i - 1);
      return clamp((int)(getCubicValue(arr, a) + 0.5), 0, 255);
    }
    v = (int)(ref[i * stride] * (1 - a) + ref[(i + 1) * stride] * a + 0.5);
    return clamp(v, 0, 255);
  } else if (y < 0) {
    int v;
    int j = (int)x;
    double b = x - j;
    if (x > 1 && x < width - 2) {
      double arr[4] = { ref[j - 1], ref[j], ref[j + 1], ref[j + 2] };
      return clamp((int)(getCubicValue(arr, b) + 0.5), 0, 255);
    }
    v = (int)(ref[j] * (1 - b) + ref[j + 1] * b + 0.5);
    return clamp(v, 0, 255);
  } else if (x > width - 1) {
    int v;
    int i = (int)y;
    double a = y - i;
    if (y > 1 && y < height - 2) {
      double arr[4];
      get_subcolumn(ref, arr, stride, width - 1, i - 1);
      return clamp((int)(getCubicValue(arr, a) + 0.5), 0, 255);
    }
    v = (int)(ref[i * stride + width - 1] * (1 - a) +
              ref[(i + 1) * stride + width - 1] * a + 0.5);
    return clamp(v, 0, 255);
  } else if (y > height - 1) {
    int v;
    int j = (int)x;
    double b = x - j;
    if (x > 1 && x < width - 2) {
      int row = (height - 1) * stride;
      double arr[4] = { ref[row + j - 1], ref[row + j], ref[row + j + 1],
                        ref[row + j + 2] };
      return clamp((int)(getCubicValue(arr, b) + 0.5), 0, 255);
    }
    v = (int)(ref[(height - 1) * stride + j] * (1 - b) +
              ref[(height - 1) * stride + j + 1] * b + 0.5);
    return clamp(v, 0, 255);
  } else if (x > 1 && y > 1 && x < width - 2 && y < height - 2) {
    return clamp((int)(bicubic(ref, x, y, stride) + 0.5), 0, 255);
  } else {
    int i = (int)y;
    int j = (int)x;
    double a = y - i;
    double b = x - j;
    int v = (int)(ref[i * stride + j] * (1 - a) * (1 - b) +
                  ref[i * stride + j + 1] * (1 - a) * b +
                  ref[(i + 1) * stride + j] * a * (1 - b) +
                  ref[(i + 1) * stride + j + 1] * a * b);
    return clamp(v, 0, 255);
  }
}

// Warps a block using flow vector [u, v] and computes the mse
double compute_warp_and_error(unsigned char *ref, unsigned char *frm, int width,
                              int height, int stride, int x, int y, double u,
                              double v, int16_t *dt) {
  int i, j;
  unsigned char warped;
  double x_w, y_w;
  double mse = 0;
  int16_t err = 0;
  for (i = y; i < y + PATCH_SIZE; ++i)
    for (j = x; j < x + PATCH_SIZE; ++j) {
      x_w = (double)j + u;
      y_w = (double)i + v;
      warped = interpolate(ref, x_w, y_w, width, height, stride);
      err = warped - frm[j + i * stride];
      mse += err * err;
      dt[(i - y) * PATCH_SIZE + (j - x)] = err;
    }

  mse /= (PATCH_SIZE * PATCH_SIZE);
  return mse;
}

// Computes the components of the system of equations used to solve for
// a flow vector. This includes:
// 1.) The hessian matrix for optical flow. This matrix is in the
// form of:
//
//       M = |sum(dx * dx)  sum(dx * dy)|
//           |sum(dx * dy)  sum(dy * dy)|
//
// 2.)   b = |sum(dx * dt)|
//           |sum(dy * dt)|
// Where the sums are computed over a square window of PATCH_SIZE.
static INLINE void compute_flow_system(const double *dx, int dx_stride,
                                       const double *dy, int dy_stride,
                                       const int16_t *dt, int dt_stride,
                                       double *M, double *b) {
  for (int i = 0; i < PATCH_SIZE; i++) {
    for (int j = 0; j < PATCH_SIZE; j++) {
      M[0] += dx[i * dx_stride + j] * dx[i * dx_stride + j];
      M[1] += dx[i * dx_stride + j] * dy[i * dy_stride + j];
      M[3] += dy[i * dy_stride + j] * dy[i * dy_stride + j];

      b[0] += dx[i * dx_stride + j] * dt[i * dt_stride + j];
      b[1] += dy[i * dy_stride + j] * dt[i * dt_stride + j];
    }
  }

  M[2] = M[1];
}

// Solves a general Mx = b where M is a 2x2 matrix and b is a 2x1 matrix
static INLINE void solve_2x2_system(const double *M, const double *b,
                                    double *output_vec) {
  double M_0 = M[0];
  double M_3 = M[3];
  double det = (M_0 * M_3) - (M[1] * M[2]);
  if (det < 1e-5) {
    // Handle singular matrix
    // TODO(sarahparker) compare results using pseudo inverse instead
    M_0 += 1e-10;
    M_3 += 1e-10;
    det = (M_0 * M_3) - (M[1] * M[2]);
  }
  const double det_inv = 1 / det;
  const double mult_b0 = det_inv * b[0];
  const double mult_b1 = det_inv * b[1];
  output_vec[0] = M_3 * mult_b0 - M[1] * mult_b1;
  output_vec[1] = -M[2] * mult_b0 + M_0 * mult_b1;
}

static INLINE void image_difference(const uint8_t *src, int src_stride,
                                    const uint8_t *ref, int ref_stride,
                                    int16_t *dst, int dst_stride, int height,
                                    int width) {
  const int block_unit = 8;
  // Take difference in 8x8 blocks to make use of optimized diff function
  for (int i = 0; i < height; i += block_unit) {
    for (int j = 0; j < width; j += block_unit) {
      aom_subtract_block(block_unit, block_unit, dst + i * dst_stride + j,
                         dst_stride, src + i * src_stride + j, src_stride,
                         ref + i * ref_stride + j, ref_stride);
    }
  }
}

// Compute an image gradient using a sobel filter.
// If dir == 1, compute the x gradient. If dir == 0, compute y. This function
// assumes the images have been padded so that they can be processed in units
// of 8.
static INLINE void sobel_xy_image_gradient(const uint8_t *src, int src_stride,
                                           double *dst, int dst_stride,
                                           int height, int width, int dir) {
  double norm = 1.0;
  // TODO(sarahparker) experiment with doing this over larger block sizes
  const int block_unit = 8;
  // Filter in 8x8 blocks to eventually make use of optimized convolve function
  for (int i = 0; i < height; i += block_unit) {
    for (int j = 0; j < width; j += block_unit) {
      av1_convolve_2d_sobel_y_c(src + i * src_stride + j, src_stride,
                                dst + i * dst_stride + j, dst_stride,
                                block_unit, block_unit, dir, norm);
    }
  }
}

static ImagePyramid *alloc_pyramid(int width, int height, int pad_size,
                                   int compute_gradient) {
  ImagePyramid *pyr = aom_malloc(sizeof(*pyr));
  pyr->has_gradient = compute_gradient;
  // 2 * width * height is the upper bound for a buffer that fits
  // all pyramid levels + padding for each level
  const int buffer_size = sizeof(*pyr->level_buffer) * 2 * width * height +
                          (width + 2 * pad_size) * 2 * pad_size * N_LEVELS;
  pyr->level_buffer = aom_malloc(buffer_size);
  memset(pyr->level_buffer, 0, buffer_size);

  if (compute_gradient) {
    const int gradient_size =
        sizeof(*pyr->level_dx_buffer) * 2 * width * height +
        (width + 2 * pad_size) * 2 * pad_size * N_LEVELS;
    pyr->level_dx_buffer = aom_malloc(gradient_size);
    pyr->level_dy_buffer = aom_malloc(gradient_size);
    memset(pyr->level_dx_buffer, 0, gradient_size);
    memset(pyr->level_dy_buffer, 0, gradient_size);
  }
  return pyr;
}

static void free_pyramid(ImagePyramid *pyr) {
  aom_free(pyr->level_buffer);
  if (pyr->has_gradient) {
    aom_free(pyr->level_dx_buffer);
    aom_free(pyr->level_dy_buffer);
  }
  aom_free(pyr);
}

static INLINE void update_level_dims(ImagePyramid *frm_pyr, int level) {
  frm_pyr->widths[level] = frm_pyr->widths[level - 1] >> 1;
  frm_pyr->heights[level] = frm_pyr->heights[level - 1] >> 1;
  frm_pyr->strides[level] = frm_pyr->widths[level] + 2 * frm_pyr->pad_size;
  // Point the beginning of the next level buffer to the correct location inside
  // the padded border
  frm_pyr->level_loc[level] =
      frm_pyr->level_loc[level - 1] +
      frm_pyr->strides[level - 1] *
          (2 * frm_pyr->pad_size + frm_pyr->heights[level - 1]);
}

// Compute coarse to fine pyramids for a frame
static void compute_flow_pyramids(unsigned char *frm, const int frm_width,
                                  const int frm_height, const int frm_stride,
                                  int n_levels, int pad_size, int compute_grad,
                                  ImagePyramid *frm_pyr) {
  int cur_width, cur_height, cur_stride, cur_loc;
  assert((frm_width >> n_levels) > 0);
  assert((frm_height >> n_levels) > 0);

  // Initialize first level
  frm_pyr->n_levels = n_levels;
  frm_pyr->pad_size = pad_size;
  frm_pyr->widths[0] = frm_width;
  frm_pyr->heights[0] = frm_height;
  frm_pyr->strides[0] = frm_width + 2 * frm_pyr->pad_size;
  // Point the beginning of the level buffer to the location inside
  // the padded border
  frm_pyr->level_loc[0] =
      frm_pyr->strides[0] * frm_pyr->pad_size + frm_pyr->pad_size;
  // This essentially copies the original buffer into the pyramid buffer
  // without the original padding
  av1_resize_plane(frm, frm_height, frm_width, frm_stride,
                   frm_pyr->level_buffer + frm_pyr->level_loc[0],
                   frm_pyr->heights[0], frm_pyr->widths[0],
                   frm_pyr->strides[0]);

  if (compute_grad) {
    cur_width = frm_pyr->widths[0];
    cur_height = frm_pyr->heights[0];
    cur_stride = frm_pyr->strides[0];
    cur_loc = frm_pyr->level_loc[0];
    assert(frm_pyr->has_gradient && frm_pyr->level_dx_buffer != NULL &&
           frm_pyr->level_dy_buffer != NULL);
    // Computation x gradient
    sobel_xy_image_gradient(frm_pyr->level_buffer + cur_loc, cur_stride,
                            frm_pyr->level_dx_buffer + cur_loc, cur_stride,
                            cur_height, cur_width, 1);

    // Computation y gradient
    sobel_xy_image_gradient(frm_pyr->level_buffer + cur_loc, cur_stride,
                            frm_pyr->level_dy_buffer + cur_loc, cur_stride,
                            cur_height, cur_width, 0);
  }

  // Start at the finest level and resize down to the coarsest level
  for (int level = 1; level < n_levels; ++level) {
    update_level_dims(frm_pyr, level);
    cur_width = frm_pyr->widths[level];
    cur_height = frm_pyr->heights[level];
    cur_stride = frm_pyr->strides[level];
    cur_loc = frm_pyr->level_loc[level];

    av1_resize_plane(frm_pyr->level_buffer + frm_pyr->level_loc[level - 1],
                     frm_pyr->heights[level - 1], frm_pyr->widths[level - 1],
                     frm_pyr->strides[level - 1],
                     frm_pyr->level_buffer + cur_loc, cur_height, cur_width,
                     cur_stride);

    if (compute_grad) {
      assert(frm_pyr->has_gradient && frm_pyr->level_dx_buffer != NULL &&
             frm_pyr->level_dy_buffer != NULL);
      // Computation x gradient
      sobel_xy_image_gradient(frm_pyr->level_buffer + cur_loc, cur_stride,
                              frm_pyr->level_dx_buffer + cur_loc, cur_stride,
                              cur_height, cur_width, 1);

      // Computation y gradient
      sobel_xy_image_gradient(frm_pyr->level_buffer + cur_loc, cur_stride,
                              frm_pyr->level_dy_buffer + cur_loc, cur_stride,
                              cur_height, cur_width, 0);
    }
  }
}

static INLINE void compute_flow_at_point(unsigned char *frm, unsigned char *ref,
                                         double *dx, double *dy, int x, int y,
                                         int width, int height, int stride,
                                         double *u, double *v) {
  double M[4] = { 0 };
  double b[2] = { 0 };
  double tmp_output_vec[2] = { 0 };
  double error = 0;
  int16_t dt[PATCH_SIZE * PATCH_SIZE];
  double o_u = *u;
  double o_v = *v;

  for (int itr = 0; itr < DISFLOW_MAX_ITR; itr++) {
    error = compute_warp_and_error(ref, frm, width, height, stride, x, y, *u,
                                   *v, dt);
    if (error <= DISFLOW_ERROR_TR) break;
    compute_flow_system(dx, stride, dy, stride, dt, PATCH_SIZE, M, b);
    solve_2x2_system(M, b, tmp_output_vec);
    *u += tmp_output_vec[0];
    *v += tmp_output_vec[1];
  }
  if (fabs(*u - o_u) > PATCH_SIZE || fabs(*v - o_u) > PATCH_SIZE) {
    *u = o_u;
    *v = o_v;
  }
}

// make sure flow_u and flow_v start at 0
static void compute_flow_field(ImagePyramid *frm_pyr, ImagePyramid *ref_pyr,
                               double *flow_u, double *flow_v) {
  int cur_width, cur_height, cur_stride, cur_loc, patch_loc, patch_center;
  double *u_upscale =
      aom_malloc(frm_pyr->strides[0] * frm_pyr->heights[0] * sizeof(*flow_u));
  double *v_upscale =
      aom_malloc(frm_pyr->strides[0] * frm_pyr->heights[0] * sizeof(*flow_v));

  assert(frm_pyr->n_levels == ref_pyr->n_levels);

  // Compute flow field from coarsest to finest level of the pyramid
  for (int level = frm_pyr->n_levels - 1; level >= 0; --level) {
    cur_width = frm_pyr->widths[level];
    cur_height = frm_pyr->heights[level];
    cur_stride = frm_pyr->strides[level];
    cur_loc = frm_pyr->level_loc[level];

    for (int i = PATCH_SIZE; i < cur_height - PATCH_SIZE; i += PATCH_STEP) {
      for (int j = PATCH_SIZE; j < cur_width - PATCH_SIZE; j += PATCH_STEP) {
        patch_loc = i * cur_stride + j;
        patch_center = patch_loc + PATCH_CENTER * cur_stride + PATCH_CENTER;
        compute_flow_at_point(frm_pyr->level_buffer + cur_loc,
                              ref_pyr->level_buffer + cur_loc,
                              frm_pyr->level_dx_buffer + cur_loc + patch_loc,
                              frm_pyr->level_dy_buffer + cur_loc + patch_loc, j,
                              i, cur_width, cur_height, cur_stride,
                              flow_u + patch_center, flow_v + patch_center);
      }
    }
    // TODO(sarahparker) Replace this with upscale function in resize.c
    if (level > 0) {
      int h_upscale = frm_pyr->heights[level - 1];
      int w_upscale = frm_pyr->widths[level - 1];
      int s_upscale = frm_pyr->strides[level - 1];
      for (int i = 0; i < h_upscale; ++i) {
        for (int j = 0; j < w_upscale; ++j) {
          u_upscale[j + i * s_upscale] =
              flow_u[(int)(j >> 1) + (int)(i >> 1) * cur_stride];
          v_upscale[j + i * s_upscale] =
              flow_v[(int)(j >> 1) + (int)(i >> 1) * cur_stride];
        }
      }
      memcpy(flow_u, u_upscale,
             frm_pyr->strides[0] * frm_pyr->heights[0] * sizeof(*flow_u));
      memcpy(flow_v, v_upscale,
             frm_pyr->strides[0] * frm_pyr->heights[0] * sizeof(*flow_v));
    }
  }
  aom_free(u_upscale);
  aom_free(v_upscale);
}

static int compute_global_motion_disflow_based(
    TransformationType type, YV12_BUFFER_CONFIG *frm, YV12_BUFFER_CONFIG *ref,
    int bit_depth, int *num_inliers_by_motion, double *params_by_motion,
    int num_motions) {
  unsigned char *frm_buffer = frm->y_buffer;
  unsigned char *ref_buffer = ref->y_buffer;
  const int frm_width = frm->y_width;
  const int frm_height = frm->y_height;
  const int ref_width = ref->y_width;
  const int ref_height = ref->y_height;
  const int pad_size = AOMMAX(PATCH_SIZE, MIN_PAD);
  int num_frm_corners;
  int num_correspondences;
  double *correspondences;
  int frm_corners[2 * MAX_CORNERS];
  RansacFuncDouble ransac = get_ransac_double_prec_type(type);
  assert(frm_width == ref_width);
  assert(frm_height == ref_height);

  // Ensure the number of pyramid levels will work with the frame resolution
  const int msb =
      frm_width < frm_height ? get_msb(frm_width) : get_msb(frm_height);
  const int n_levels = AOMMIN(msb, N_LEVELS);

  if (frm->flags & YV12_FLAG_HIGHBITDEPTH) {
    // The frame buffer is 16-bit, so we need to convert to 8 bits for the
    // following code. We cache the result until the frame is released.
    frm_buffer = downconvert_frame(frm, bit_depth);
  }
  if (ref->flags & YV12_FLAG_HIGHBITDEPTH) {
    ref_buffer = downconvert_frame(ref, bit_depth);
  }

  // TODO(sarahparker) We will want to do the source pyramid computation
  // outside of this function so it doesn't get recomputed for every
  // reference. We also don't need to compute every pyramid level for the
  // reference in advance, since lower levels can be overwritten once their
  // flow field is computed and upscaled. I'll add these optimizations
  // once the full implementation is working.
  // Allocate frm image pyramids
  int compute_gradient = 1;
  ImagePyramid *frm_pyr =
      alloc_pyramid(frm_width, frm_height, pad_size, compute_gradient);
  compute_flow_pyramids(frm_buffer, frm_width, frm_height, frm->y_stride,
                        n_levels, pad_size, compute_gradient, frm_pyr);
  // Allocate ref image pyramids
  compute_gradient = 0;
  ImagePyramid *ref_pyr =
      alloc_pyramid(ref_width, ref_height, pad_size, compute_gradient);
  compute_flow_pyramids(ref_buffer, ref_width, ref_height, ref->y_stride,
                        n_levels, pad_size, compute_gradient, ref_pyr);

  double *flow_u =
      aom_malloc(frm_pyr->strides[0] * frm_pyr->heights[0] * sizeof(*flow_u));
  double *flow_v =
      aom_malloc(frm_pyr->strides[0] * frm_pyr->heights[0] * sizeof(*flow_v));

  memset(flow_u, 0,
         frm_pyr->strides[0] * frm_pyr->heights[0] * sizeof(*flow_u));
  memset(flow_v, 0,
         frm_pyr->strides[0] * frm_pyr->heights[0] * sizeof(*flow_v));

  compute_flow_field(frm_pyr, ref_pyr, flow_u, flow_v);

  // compute interest points in images using FAST features
  num_frm_corners = fast_corner_detect(frm_buffer, frm_width, frm_height,
                                       frm->y_stride, frm_corners, MAX_CORNERS);
  // find correspondences between the two images using the flow field
  correspondences = aom_malloc(num_frm_corners * 4 * sizeof(*correspondences));
  num_correspondences = determine_disflow_correspondence(
      frm_corners, num_frm_corners, flow_u, flow_v, frm_width, frm_height,
      frm_pyr->strides[0], correspondences);
  ransac(correspondences, num_correspondences, num_inliers_by_motion,
         params_by_motion, num_motions);

  free_pyramid(frm_pyr);
  free_pyramid(ref_pyr);
  aom_free(correspondences);
  aom_free(flow_u);
  aom_free(flow_v);
  // Set num_inliers = 0 for motions with too few inliers so they are ignored.
  for (int i = 0; i < num_motions; ++i) {
    if (num_inliers_by_motion[i] < MIN_INLIER_PROB * num_correspondences) {
      num_inliers_by_motion[i] = 0;
    }
  }

  // Return true if any one of the motions has inliers.
  for (int i = 0; i < num_motions; ++i) {
    if (num_inliers_by_motion[i] > 0) return 1;
  }
  return 0;
}
#endif

int av1_compute_global_motion(TransformationType type, YV12_BUFFER_CONFIG *frm,
                              YV12_BUFFER_CONFIG *ref, int bit_depth,
                              int *num_inliers_by_motion,
                              double *params_by_motion, int num_motions) {
#if USE_GM_FEATURE_BASED
  return compute_global_motion_feature_based(type, frm, ref, bit_depth,
                                             num_inliers_by_motion,
                                             params_by_motion, num_motions);
#else
  return compute_global_motion_disflow_based(type, frm, ref, bit_depth,
                                             num_inliers_by_motion,
                                             params_by_motion, num_motions);
#endif
}
