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

#include "av1/encoder/tune_vmaf.h"

#include "aom_dsp/vmaf.h"
#include "aom_ports/system_state.h"
#include "av1/encoder/extend.h"

// TODO(sdeng): Add the SIMD implementation.
static AOM_INLINE void unsharp_rect(const uint8_t *source, int source_stride,
                                    const uint8_t *blurred, int blurred_stride,
                                    uint8_t *dst, int dst_stride, int w, int h,
                                    double amount) {
  for (int i = 0; i < h; ++i) {
    for (int j = 0; j < w; ++j) {
      const double val =
          (double)source[j] + amount * ((double)source[j] - (double)blurred[j]);
      dst[j] = (uint8_t)clamp((int)(val + 0.5), 0, 255);
    }
    source += source_stride;
    blurred += blurred_stride;
    dst += dst_stride;
  }
}

static AOM_INLINE void unsharp(const YV12_BUFFER_CONFIG *source,
                               const YV12_BUFFER_CONFIG *blurred,
                               const YV12_BUFFER_CONFIG *dst, double amount) {
  unsharp_rect(source->y_buffer, source->y_stride, blurred->y_buffer,
               blurred->y_stride, dst->y_buffer, dst->y_stride, source->y_width,
               source->y_height, amount);
}

// 8-tap Gaussian convolution filter with sigma = 1.0, sums to 128,
// all co-efficients must be even.
DECLARE_ALIGNED(16, static const int16_t, gauss_filter[8]) = { 0,  8, 30, 52,
                                                               30, 8, 0,  0 };
static AOM_INLINE void gaussian_blur(const AV1_COMP *const cpi,
                                     const YV12_BUFFER_CONFIG *source,
                                     const YV12_BUFFER_CONFIG *dst) {
  const AV1_COMMON *cm = &cpi->common;
  const ThreadData *td = &cpi->td;
  const MACROBLOCK *x = &td->mb;
  const MACROBLOCKD *xd = &x->e_mbd;

  const int block_size = BLOCK_128X128;

  const int num_mi_w = mi_size_wide[block_size];
  const int num_mi_h = mi_size_high[block_size];
  const int num_cols = (cm->mi_cols + num_mi_w - 1) / num_mi_w;
  const int num_rows = (cm->mi_rows + num_mi_h - 1) / num_mi_h;
  int row, col;
  const int use_hbd = source->flags & YV12_FLAG_HIGHBITDEPTH;

  ConvolveParams conv_params = get_conv_params(0, 0, xd->bd);
  InterpFilterParams filter = { .filter_ptr = gauss_filter,
                                .taps = 8,
                                .subpel_shifts = 0,
                                .interp_filter = EIGHTTAP_REGULAR };

  for (row = 0; row < num_rows; ++row) {
    for (col = 0; col < num_cols; ++col) {
      const int mi_row = row * num_mi_h;
      const int mi_col = col * num_mi_w;

      const int row_offset_y = mi_row << 2;
      const int col_offset_y = mi_col << 2;

      uint8_t *src_buf =
          source->y_buffer + row_offset_y * source->y_stride + col_offset_y;
      uint8_t *dst_buf =
          dst->y_buffer + row_offset_y * dst->y_stride + col_offset_y;

      if (use_hbd) {
        av1_highbd_convolve_2d_sr(
            CONVERT_TO_SHORTPTR(src_buf), source->y_stride,
            CONVERT_TO_SHORTPTR(dst_buf), dst->y_stride, num_mi_w << 2,
            num_mi_h << 2, &filter, &filter, 0, 0, &conv_params, xd->bd);
      } else {
        av1_convolve_2d_sr(src_buf, source->y_stride, dst_buf, dst->y_stride,
                           num_mi_w << 2, num_mi_h << 2, &filter, &filter, 0, 0,
                           &conv_params);
      }
    }
  }
}

static double find_best_frame_unsharp_amount(
    const AV1_COMP *const cpi, YV12_BUFFER_CONFIG *const source,
    YV12_BUFFER_CONFIG *const blurred) {
  const AV1_COMMON *const cm = &cpi->common;
  const int width = source->y_width;
  const int height = source->y_height;

  YV12_BUFFER_CONFIG sharpened;
  memset(&sharpened, 0, sizeof(sharpened));
  aom_alloc_frame_buffer(&sharpened, width, height, 1, 1,
                         cm->seq_params.use_highbitdepth,
                         cpi->oxcf.border_in_pixels, cm->byte_alignment);

  double unsharp_amount = 0.0;
  const double step_size = 0.05;
  const double max_vmaf_score = 100.0;

  double best_vmaf;
  aom_calc_vmaf(cpi->oxcf.vmaf_model_path, source, source, &best_vmaf);

  // We may get the same best VMAF scores for different unsharp_amount values.
  // We pick the average value in this case.
  double best_unsharp_amount_begin = best_vmaf == max_vmaf_score ? 0.0 : -1.0;
  bool exit_loop = false;

  int loop_count = 0;
  const int max_loop_count = 20;
  while (!exit_loop) {
    unsharp_amount += step_size;
    unsharp(source, blurred, &sharpened, unsharp_amount);
    double new_vmaf;
    aom_calc_vmaf(cpi->oxcf.vmaf_model_path, source, &sharpened, &new_vmaf);
    if (new_vmaf < best_vmaf || loop_count == max_loop_count) {
      exit_loop = true;
    } else {
      if (new_vmaf == max_vmaf_score && best_unsharp_amount_begin < 0.0) {
        best_unsharp_amount_begin = unsharp_amount;
      }
      best_vmaf = new_vmaf;
    }
    loop_count++;
  }

  aom_free_frame_buffer(&sharpened);

  unsharp_amount -= step_size;
  if (best_unsharp_amount_begin >= 0.0) {
    unsharp_amount = (unsharp_amount + best_unsharp_amount_begin) / 2.0;
  }

  return unsharp_amount;
}

void av1_vmaf_preprocessing(const AV1_COMP *const cpi,
                            YV12_BUFFER_CONFIG *const source,
                            bool use_block_based_method) {
  const int use_hbd = source->flags & YV12_FLAG_HIGHBITDEPTH;
  // TODO(sdeng): Add high bit depth support.
  if (use_hbd) {
    printf(
        "VMAF preprocessing for high bit depth videos is unsupported yet.\n");
    exit(0);
  }

  aom_clear_system_state();
  const AV1_COMMON *const cm = &cpi->common;
  const int width = source->y_width;
  const int height = source->y_height;
  YV12_BUFFER_CONFIG source_extended, blurred, sharpened;
  memset(&source_extended, 0, sizeof(source_extended));
  memset(&blurred, 0, sizeof(blurred));
  memset(&sharpened, 0, sizeof(sharpened));
  aom_alloc_frame_buffer(&source_extended, width, height, 1, 1,
                         cm->seq_params.use_highbitdepth,
                         cpi->oxcf.border_in_pixels, cm->byte_alignment);
  aom_alloc_frame_buffer(&blurred, width, height, 1, 1,
                         cm->seq_params.use_highbitdepth,
                         cpi->oxcf.border_in_pixels, cm->byte_alignment);
  aom_alloc_frame_buffer(&sharpened, width, height, 1, 1,
                         cm->seq_params.use_highbitdepth,
                         cpi->oxcf.border_in_pixels, cm->byte_alignment);

  av1_copy_and_extend_frame(source, &source_extended);
  av1_copy_and_extend_frame(source, &sharpened);

  gaussian_blur(cpi, &source_extended, &blurred);
  aom_free_frame_buffer(&source_extended);
  const double best_frame_unsharp_amount =
      find_best_frame_unsharp_amount(cpi, source, &blurred);

  if (!use_block_based_method) {
    unsharp(source, &blurred, source, best_frame_unsharp_amount);
    aom_free_frame_buffer(&sharpened);
    aom_free_frame_buffer(&blurred);
    aom_clear_system_state();
    return;
  }

  const int block_size = BLOCK_128X128;
  const int num_mi_w = mi_size_wide[block_size];
  const int num_mi_h = mi_size_high[block_size];
  const int num_cols = (cm->mi_cols + num_mi_w - 1) / num_mi_w;
  const int num_rows = (cm->mi_rows + num_mi_h - 1) / num_mi_h;
  const int block_w = num_mi_w << 2;
  const int block_h = num_mi_h << 2;
  double *best_unsharp_amounts =
      aom_malloc(sizeof(*best_unsharp_amounts) * num_cols * num_rows);
  memset(best_unsharp_amounts, 0,
         sizeof(*best_unsharp_amounts) * num_cols * num_rows);

  for (int row = 0; row < num_rows; ++row) {
    for (int col = 0; col < num_cols; ++col) {
      const int mi_row = row * num_mi_h;
      const int mi_col = col * num_mi_w;

      const int row_offset_y = mi_row << 2;
      const int col_offset_y = mi_col << 2;

      const int block_width = AOMMIN(source->y_width - col_offset_y, block_w);
      const int block_height = AOMMIN(source->y_height - row_offset_y, block_h);

      uint8_t *src_buf =
          source->y_buffer + row_offset_y * source->y_stride + col_offset_y;
      uint8_t *blurred_buf =
          blurred.y_buffer + row_offset_y * blurred.y_stride + col_offset_y;
      uint8_t *dst_buf =
          sharpened.y_buffer + row_offset_y * sharpened.y_stride + col_offset_y;

      const int index = col + row * num_cols;
      const double step_size = 0.1;
      double amount = AOMMAX(best_frame_unsharp_amount - 0.2, step_size);
      unsharp_rect(src_buf, source->y_stride, blurred_buf, blurred.y_stride,
                   dst_buf, sharpened.y_stride, block_width, block_height,
                   amount);
      double best_vmaf;
      aom_calc_vmaf(cpi->oxcf.vmaf_model_path, source, &sharpened, &best_vmaf);

      // Find the best unsharp amount.
      bool exit_loop = false;
      while (!exit_loop && amount < best_frame_unsharp_amount + 0.2) {
        amount += step_size;
        unsharp_rect(src_buf, source->y_stride, blurred_buf, blurred.y_stride,
                     dst_buf, sharpened.y_stride, block_width, block_height,
                     amount);

        double new_vmaf;
        aom_calc_vmaf(cpi->oxcf.vmaf_model_path, source, &sharpened, &new_vmaf);
        if (new_vmaf <= best_vmaf) {
          exit_loop = true;
          amount -= step_size;
        } else {
          best_vmaf = new_vmaf;
        }
      }
      best_unsharp_amounts[index] = amount;
      // Reset blurred frame
      unsharp_rect(src_buf, source->y_stride, blurred_buf, blurred.y_stride,
                   dst_buf, sharpened.y_stride, block_width, block_height, 0.0);
    }
  }

  // Apply best blur amounts
  for (int row = 0; row < num_rows; ++row) {
    for (int col = 0; col < num_cols; ++col) {
      const int mi_row = row * num_mi_h;
      const int mi_col = col * num_mi_w;
      const int row_offset_y = mi_row << 2;
      const int col_offset_y = mi_col << 2;
      const int block_width = AOMMIN(source->y_width - col_offset_y, block_w);
      const int block_height = AOMMIN(source->y_height - row_offset_y, block_h);
      const int index = col + row * num_cols;
      uint8_t *src_buf =
          source->y_buffer + row_offset_y * source->y_stride + col_offset_y;
      uint8_t *blurred_buf =
          blurred.y_buffer + row_offset_y * blurred.y_stride + col_offset_y;
      unsharp_rect(src_buf, source->y_stride, blurred_buf, blurred.y_stride,
                   src_buf, source->y_stride, block_width, block_height,
                   best_unsharp_amounts[index]);
    }
  }

  aom_free_frame_buffer(&sharpened);
  aom_free_frame_buffer(&blurred);
  aom_free(best_unsharp_amounts);
  aom_clear_system_state();
}

// TODO(sdeng): replace it with the SIMD version.
static AOM_INLINE double image_mse_c(const uint8_t *src, int src_stride,
                                     const uint8_t *ref, int ref_stride, int w,
                                     int h) {
  double accum = 0.0;
  int i, j;

  for (i = 0; i < h; ++i) {
    for (j = 0; j < w; ++j) {
      double img1px = src[i * src_stride + j];
      double img2px = ref[i * ref_stride + j];

      accum += (img1px - img2px) * (img1px - img2px);
    }
  }

  return accum / (double)(w * h);
}

void av1_set_mb_vmaf_rdmult_scaling(AV1_COMP *cpi) {
  AV1_COMMON *cm = &cpi->common;
  ThreadData *td = &cpi->td;
  MACROBLOCK *x = &td->mb;
  MACROBLOCKD *xd = &x->e_mbd;
  uint8_t *const y_buffer = cpi->source->y_buffer;
  const int y_stride = cpi->source->y_stride;
  const int y_width = cpi->source->y_width;
  const int y_height = cpi->source->y_height;
  const int block_size = BLOCK_64X64;

  const int num_mi_w = mi_size_wide[block_size];
  const int num_mi_h = mi_size_high[block_size];
  const int num_cols = (cm->mi_cols + num_mi_w - 1) / num_mi_w;
  const int num_rows = (cm->mi_rows + num_mi_h - 1) / num_mi_h;
  const int block_w = num_mi_w << 2;
  const int block_h = num_mi_h << 2;
  const int use_hbd = cpi->source->flags & YV12_FLAG_HIGHBITDEPTH;
  // TODO(sdeng): Add high bit depth support.
  if (use_hbd) {
    printf("VMAF RDO for high bit depth videos is unsupported yet.\n");
    exit(0);
  }

  aom_clear_system_state();
  YV12_BUFFER_CONFIG fake_recon, blurred;
  memset(&fake_recon, 0, sizeof(fake_recon));
  memset(&blurred, 0, sizeof(blurred));
  aom_alloc_frame_buffer(&fake_recon, y_width, y_height, 1, 1,
                         cm->seq_params.use_highbitdepth,
                         cpi->oxcf.border_in_pixels, cm->byte_alignment);
  aom_alloc_frame_buffer(&blurred, y_width, y_height, 1, 1,
                         cm->seq_params.use_highbitdepth,
                         cpi->oxcf.border_in_pixels, cm->byte_alignment);

  gaussian_blur(cpi, cpi->source, &blurred);

  // baseline vmaf
  double baseline_mse = 0.0, baseline_vmaf = 0.0;
  aom_calc_vmaf(cpi->oxcf.vmaf_model_path, cpi->source, cpi->source,
                &baseline_vmaf);
  av1_copy_and_extend_frame(cpi->source, &fake_recon);

  // Loop through each 'block_size' block.
  for (int row = 0; row < num_rows; ++row) {
    for (int col = 0; col < num_cols; ++col) {
      const int mi_row = row * num_mi_h;
      const int mi_col = col * num_mi_w;
      const int index = row * num_cols + col;
      const int row_offset_y = mi_row << 2;
      const int col_offset_y = mi_col << 2;

      uint8_t *const orig_buf =
          y_buffer + row_offset_y * y_stride + col_offset_y;
      uint8_t *const blurred_buf =
          blurred.y_buffer + row_offset_y * blurred.y_stride + col_offset_y;
      uint8_t *const fybuf = fake_recon.y_buffer +
                             row_offset_y * fake_recon.y_stride + col_offset_y;

      const int block_width = AOMMIN(y_width - col_offset_y, block_w);
      const int block_height = AOMMIN(y_height - row_offset_y, block_h);

      // Set the blurred block.
      unsharp_rect(orig_buf, y_stride, blurred_buf, blurred.y_stride, fybuf,
                   fake_recon.y_stride, block_width, block_height, -1.0);

      double vmaf, mse;
      aom_calc_vmaf(cpi->oxcf.vmaf_model_path, cpi->source, &fake_recon, &vmaf);
      mse = image_mse_c(y_buffer, y_stride, fake_recon.y_buffer,
                        fake_recon.y_stride, y_width, y_height);

      double weight = 0.0;
      const double dvmaf = baseline_vmaf - vmaf;
      const double dmse = mse - baseline_mse;
      const double eps = 0.01 / (num_rows * num_cols);

      if (dvmaf < eps || dmse < eps) {
        weight = 1.0;
      } else {
        weight = dmse / dvmaf;
      }

      // Normalize it with a data fitted model.
      weight = 6.0 * (1.0 - exp(-0.05 * weight)) + 0.8;
      cpi->vmaf_rdmult_scaling_factors[index] = weight;

      // Reset blurred block.
      unsharp_rect(orig_buf, y_stride, blurred_buf, blurred.y_stride, fybuf,
                   fake_recon.y_stride, block_width, block_height, 0.0);
    }
  }

  aom_free_frame_buffer(&fake_recon);
  aom_free_frame_buffer(&blurred);
  aom_clear_system_state();
  (void)xd;
}

void av1_set_vmaf_rdmult(const AV1_COMP *const cpi, MACROBLOCK *const x,
                         const BLOCK_SIZE bsize, const int mi_row,
                         const int mi_col, int *const rdmult) {
  const AV1_COMMON *const cm = &cpi->common;

  const int bsize_base = BLOCK_64X64;
  const int num_mi_w = mi_size_wide[bsize_base];
  const int num_mi_h = mi_size_high[bsize_base];
  const int num_cols = (cm->mi_cols + num_mi_w - 1) / num_mi_w;
  const int num_rows = (cm->mi_rows + num_mi_h - 1) / num_mi_h;
  const int num_bcols = (mi_size_wide[bsize] + num_mi_w - 1) / num_mi_w;
  const int num_brows = (mi_size_high[bsize] + num_mi_h - 1) / num_mi_h;
  int row, col;
  double num_of_mi = 0.0;
  double geom_mean_of_scale = 0.0;

  aom_clear_system_state();
  for (row = mi_row / num_mi_w;
       row < num_rows && row < mi_row / num_mi_w + num_brows; ++row) {
    for (col = mi_col / num_mi_h;
         col < num_cols && col < mi_col / num_mi_h + num_bcols; ++col) {
      const int index = row * num_cols + col;
      geom_mean_of_scale += log(cpi->vmaf_rdmult_scaling_factors[index]);
      num_of_mi += 1.0;
    }
  }
  geom_mean_of_scale = exp(geom_mean_of_scale / num_of_mi);

  *rdmult = (int)((double)(*rdmult) * geom_mean_of_scale + 0.5);
  *rdmult = AOMMAX(*rdmult, 0);
  set_error_per_bit(x, *rdmult);
  aom_clear_system_state();
}
