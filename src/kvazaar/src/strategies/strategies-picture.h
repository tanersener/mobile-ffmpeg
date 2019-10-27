#ifndef STRATEGIES_PICTURE_H_
#define STRATEGIES_PICTURE_H_
/*****************************************************************************
 * This file is part of Kvazaar HEVC encoder.
 *
 * Copyright (C) 2013-2015 Tampere University of Technology and others (see
 * COPYING file).
 *
 * Kvazaar is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * Kvazaar is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with Kvazaar.  If not, see <http://www.gnu.org/licenses/>.
 ****************************************************************************/

/**
 * \ingroup Optimization
 * \file
 * Interface for distortion metric functions.
 */

#include "global.h" // IWYU pragma: keep
#include "kvazaar.h"
#include "encoderstate.h"
#include "strategies/optimized_sad_func_ptr_t.h"


typedef kvz_pixel (*pred_buffer)[32 * 32];

// Function macro for defining hadamard calculating functions
// for fixed size blocks. They calculate hadamard for integer
// multiples of 8x8 with the 8x8 hadamard function.
#define SATD_NxN(suffix, n) \
/* Declare the function in advance, hopefully reducing the probability that the
 * macro expands to something unexpected and silently breaks things. */ \
static cost_pixel_nxn_func satd_ ## n ## x ## n ## _ ## suffix;\
static unsigned satd_ ## n ## x ## n ## _ ## suffix ( \
    const kvz_pixel * const block1, \
    const kvz_pixel * const block2) \
{ \
  unsigned sum = 0; \
  for (unsigned y = 0; y < (n); y += 8) { \
    unsigned row = y * (n); \
    for (unsigned x = 0; x < (n); x += 8) { \
      sum += satd_8x8_subblock_ ## suffix(&block1[row + x], (n), &block2[row + x], (n)); \
    } \
  } \
  return sum >> (KVZ_BIT_DEPTH - 8); \
}


// Function macro for defining hadamard calculating functions for dynamic size
// blocks. They calculate hadamard for integer multiples of 8x8 with the 8x8
// hadamard function.
#define SATD_ANY_SIZE(suffix) \
  static cost_pixel_any_size_func satd_any_size_ ## suffix; \
  static unsigned satd_any_size_ ## suffix ( \
      int width, int height, \
      const kvz_pixel *block1, int stride1, \
      const kvz_pixel *block2, int stride2) \
  { \
    unsigned sum = 0; \
    if (width % 8 != 0) { \
      /* Process the first column using 4x4 blocks. */ \
      for (int y = 0; y < height; y += 4) { \
        sum += kvz_satd_4x4_subblock_ ## suffix(&block1[y * stride1], stride1, \
                                                &block2[y * stride2], stride2); \
      } \
      block1 += 4; \
      block2 += 4; \
      width -= 4; \
    } \
    if (height % 8 != 0) { \
      /* Process the first row using 4x4 blocks. */ \
      for (int x = 0; x < width; x += 4) { \
        sum += kvz_satd_4x4_subblock_ ## suffix(&block1[x], stride1, \
                                                &block2[x], stride2); \
      } \
      block1 += 4 * stride1; \
      block2 += 4 * stride2; \
      height -= 4; \
    } \
    /* The rest can now be processed with 8x8 blocks. */ \
    for (int y = 0; y < height; y += 8) { \
      const kvz_pixel *row1 = &block1[y * stride1]; \
      const kvz_pixel *row2 = &block2[y * stride2]; \
      for (int x = 0; x < width; x += 8) { \
        sum += satd_8x8_subblock_ ## suffix(&row1[x], stride1, \
                                            &row2[x], stride2); \
      } \
    } \
    return sum >> (KVZ_BIT_DEPTH - 8); \
  }

typedef unsigned(reg_sad_func)(const kvz_pixel *const data1, const kvz_pixel *const data2,
  const int width, const int height,
  const unsigned stride1, const unsigned stride2);
typedef unsigned (cost_pixel_nxn_func)(const kvz_pixel *block1, const kvz_pixel *block2);
typedef unsigned (cost_pixel_any_size_func)(
    int width, int height,
    const kvz_pixel *block1, int stride1,
    const kvz_pixel *block2, int stride2
);
typedef void (cost_pixel_nxn_multi_func)(const pred_buffer preds, const kvz_pixel *orig, unsigned num_modes, unsigned *costs_out);
typedef void (cost_pixel_any_size_multi_func)(int width, int height, const kvz_pixel **preds, const int stride, const kvz_pixel *orig, const int orig_stride, unsigned num_modes, unsigned *costs_out, int8_t *valid);

typedef unsigned (pixels_calc_ssd_func)(const kvz_pixel *const ref, const kvz_pixel *const rec, const int ref_stride, const int rec_stride, const int width);
typedef optimized_sad_func_ptr_t (get_optimized_sad_func)(int32_t);
typedef uint32_t (ver_sad_func)(const kvz_pixel *pic_data, const kvz_pixel *ref_data,
                                int32_t block_width, int32_t block_height,
                                uint32_t pic_stride);
typedef uint32_t (hor_sad_func)(const kvz_pixel *pic_data, const kvz_pixel *ref_data,
                                int32_t width, int32_t height, uint32_t pic_stride,
                                uint32_t ref_stride, uint32_t left, uint32_t right);

typedef void (inter_recon_bipred_func)(const int hi_prec_luma_rec0,
	const int hi_prec_luma_rec1,
	const int hi_prec_chroma_rec0,
	const int hi_prec_chroma_rec1,
	int height,
	int width,
	int ypos,
	int xpos,
	const hi_prec_buf_t*high_precision_rec0,
	const hi_prec_buf_t*high_precision_rec1,
	lcu_t* lcu,
	kvz_pixel temp_lcu_y[LCU_WIDTH*LCU_WIDTH],
	kvz_pixel temp_lcu_u[LCU_WIDTH_C*LCU_WIDTH_C],
	kvz_pixel temp_lcu_v[LCU_WIDTH_C*LCU_WIDTH_C]);
	
	

// Declare function pointers.
extern reg_sad_func * kvz_reg_sad;

extern cost_pixel_nxn_func * kvz_sad_4x4;
extern cost_pixel_nxn_func * kvz_sad_8x8;
extern cost_pixel_nxn_func * kvz_sad_16x16;
extern cost_pixel_nxn_func * kvz_sad_32x32;
extern cost_pixel_nxn_func * kvz_sad_64x64;

extern cost_pixel_nxn_func * kvz_satd_4x4;
extern cost_pixel_nxn_func * kvz_satd_8x8;
extern cost_pixel_nxn_func * kvz_satd_16x16;
extern cost_pixel_nxn_func * kvz_satd_32x32;
extern cost_pixel_nxn_func * kvz_satd_64x64;
extern cost_pixel_any_size_func *kvz_satd_any_size;

extern cost_pixel_nxn_multi_func * kvz_sad_4x4_dual;
extern cost_pixel_nxn_multi_func * kvz_sad_8x8_dual;
extern cost_pixel_nxn_multi_func * kvz_sad_16x16_dual;
extern cost_pixel_nxn_multi_func * kvz_sad_32x32_dual;
extern cost_pixel_nxn_multi_func * kvz_sad_64x64_dual;

extern cost_pixel_nxn_multi_func * kvz_satd_4x4_dual;
extern cost_pixel_nxn_multi_func * kvz_satd_8x8_dual;
extern cost_pixel_nxn_multi_func * kvz_satd_16x16_dual;
extern cost_pixel_nxn_multi_func * kvz_satd_32x32_dual;
extern cost_pixel_nxn_multi_func * kvz_satd_64x64_dual;

extern cost_pixel_any_size_multi_func *kvz_satd_any_size_quad;

extern pixels_calc_ssd_func *kvz_pixels_calc_ssd;

extern inter_recon_bipred_func * kvz_inter_recon_bipred_blend;

extern get_optimized_sad_func *kvz_get_optimized_sad;
extern ver_sad_func *kvz_ver_sad;
extern hor_sad_func *kvz_hor_sad;

int kvz_strategy_register_picture(void* opaque, uint8_t bitdepth);
cost_pixel_nxn_func * kvz_pixels_get_satd_func(unsigned n);
cost_pixel_nxn_func * kvz_pixels_get_sad_func(unsigned n);
cost_pixel_nxn_multi_func * kvz_pixels_get_satd_dual_func(unsigned n);
cost_pixel_nxn_multi_func * kvz_pixels_get_sad_dual_func(unsigned n);

#define STRATEGIES_PICTURE_EXPORTS \
  {"reg_sad", (void**) &kvz_reg_sad}, \
  {"sad_4x4", (void**) &kvz_sad_4x4}, \
  {"sad_8x8", (void**) &kvz_sad_8x8}, \
  {"sad_16x16", (void**) &kvz_sad_16x16}, \
  {"sad_32x32", (void**) &kvz_sad_32x32}, \
  {"sad_64x64", (void**) &kvz_sad_64x64}, \
  {"satd_4x4", (void**) &kvz_satd_4x4}, \
  {"satd_8x8", (void**) &kvz_satd_8x8}, \
  {"satd_16x16", (void**) &kvz_satd_16x16}, \
  {"satd_32x32", (void**) &kvz_satd_32x32}, \
  {"satd_64x64", (void**) &kvz_satd_64x64}, \
  {"satd_any_size", (void**) &kvz_satd_any_size}, \
  {"sad_4x4_dual", (void**) &kvz_sad_4x4_dual}, \
  {"sad_8x8_dual", (void**) &kvz_sad_8x8_dual}, \
  {"sad_16x16_dual", (void**) &kvz_sad_16x16_dual}, \
  {"sad_32x32_dual", (void**) &kvz_sad_32x32_dual}, \
  {"sad_64x64_dual", (void**) &kvz_sad_64x64_dual}, \
  {"satd_4x4_dual", (void**) &kvz_satd_4x4_dual}, \
  {"satd_8x8_dual", (void**) &kvz_satd_8x8_dual}, \
  {"satd_16x16_dual", (void**) &kvz_satd_16x16_dual}, \
  {"satd_32x32_dual", (void**) &kvz_satd_32x32_dual}, \
  {"satd_64x64_dual", (void**) &kvz_satd_64x64_dual}, \
  {"satd_any_size_quad", (void**) &kvz_satd_any_size_quad}, \
  {"pixels_calc_ssd", (void**) &kvz_pixels_calc_ssd}, \
  {"inter_recon_bipred", (void**) &kvz_inter_recon_bipred_blend}, \
  {"get_optimized_sad", (void**) &kvz_get_optimized_sad}, \
  {"ver_sad", (void**) &kvz_ver_sad}, \
  {"hor_sad", (void**) &kvz_hor_sad}, \



#endif //STRATEGIES_PICTURE_H_
