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

#include "strategies/generic/ipol-generic.h"

#include <stdio.h>
#include <string.h>

#include "encoder.h"
#include "strategies/generic/picture-generic.h"
#include "strategies/strategies-ipol.h"
#include "strategyselector.h"

extern int8_t kvz_g_luma_filter[4][8];
extern int8_t kvz_g_chroma_filter[8][4];

int32_t kvz_eight_tap_filter_hor_generic(int8_t *filter, kvz_pixel *data)
{
  int32_t temp = 0;
  for (int i = 0; i < 8; ++i)
  {
    temp += filter[i] * data[i];
  }

  return temp;
}

int32_t kvz_eight_tap_filter_hor_16bit_generic(int8_t *filter, int16_t *data)
{
  int32_t temp = 0;
  for (int i = 0; i < 8; ++i)
  {
    temp += filter[i] * data[i];
  }

  return temp;
}

int32_t kvz_eight_tap_filter_ver_generic(int8_t *filter, kvz_pixel *data, int16_t stride)
{
  int32_t temp = 0;
  for (int i = 0; i < 8; ++i)
  {
    temp += filter[i] * data[stride * i];
  }

  return temp;
}

int32_t kvz_eight_tap_filter_ver_16bit_generic(int8_t *filter, int16_t *data, int16_t stride)
{
  int32_t temp = 0;
  for (int i = 0; i < 8; ++i)
  {
    temp += filter[i] * data[stride * i];
  }

  return temp;
}

int32_t kvz_four_tap_filter_hor_generic(int8_t *filter, kvz_pixel *data)
{
  int32_t temp = 0;
  for (int i = 0; i < 4; ++i)
  {
    temp += filter[i] * data[i];
  }

  return temp;
}

int32_t kvz_four_tap_filter_hor_16bit_generic(int8_t *filter, int16_t *data)
{
  int32_t temp = 0;
  for (int i = 0; i < 4; ++i)
  {
    temp += filter[i] * data[i];
  }

  return temp;
}

int32_t kvz_four_tap_filter_ver_generic(int8_t *filter, kvz_pixel *data, int16_t stride)
{
  int32_t temp = 0;
  for (int i = 0; i < 4; ++i)
  {
    temp += filter[i] * data[stride * i];
  }

  return temp;
}

int32_t kvz_four_tap_filter_ver_16bit_generic(int8_t *filter, int16_t *data, int16_t stride)
{
  int32_t temp = 0;
  for (int i = 0; i < 4; ++i)
  {
    temp += filter[i] * data[stride * i];
  }

  return temp;
}

void kvz_sample_quarterpel_luma_generic(const encoder_control_t * const encoder, kvz_pixel *src, int16_t src_stride, int width, int height, kvz_pixel *dst, int16_t dst_stride, int8_t hor_flag, int8_t ver_flag, const int16_t mv[2])
{
  //TODO: horizontal and vertical only filtering
  int32_t x, y;

  // Interpolation filter shifts
  int16_t shift1 = KVZ_BIT_DEPTH - 8;
  int32_t shift2 = 6;

  // Weighted prediction offset and shift
  int32_t wp_shift1 = 14 - KVZ_BIT_DEPTH;
  int32_t wp_offset1 = 1 << (wp_shift1 - 1);

  // Select filters according to the fractional part of the x and y mv components
  int8_t *hor_filter = kvz_g_luma_filter[mv[0] & 3];
  int8_t *ver_filter = kvz_g_luma_filter[mv[1] & 3];

  int16_t hor_filtered[KVZ_EXT_BLOCK_W_LUMA][LCU_WIDTH];
  int16_t hor_stride = LCU_WIDTH;

  // Filter horizontally
  for (y = 0; y < height + KVZ_EXT_PADDING_LUMA; ++y) {
    for (x = 0; x < width; ++x) {
      int ypos = y - KVZ_LUMA_FILTER_OFFSET;
      int xpos = x - KVZ_LUMA_FILTER_OFFSET;
      hor_filtered[y][x] = kvz_eight_tap_filter_hor_generic(hor_filter, &src[src_stride * ypos + xpos]) >> shift1;
    }
  }

  // Filter vertically
  for (y = 0; y < height; ++y) {
    for (x = 0; x < width; ++x) {
      dst[y * dst_stride + x] = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_ver_16bit_generic(ver_filter, &hor_filtered[y][x], hor_stride) >> shift2) + wp_offset1) >> wp_shift1);
    }
  }
}

void kvz_sample_14bit_quarterpel_luma_generic(const encoder_control_t * const encoder, kvz_pixel *src, int16_t src_stride, int width, int height, int16_t *dst, int16_t dst_stride, int8_t hor_flag, int8_t ver_flag, const int16_t mv[2])
{
  //TODO: horizontal and vertical only filtering
  int32_t x, y;

  // Interpolation filter shifts
  int16_t shift1 = KVZ_BIT_DEPTH - 8;
  int32_t shift2 = 6;

  // Select filters according to the fractional part of the x and y mv components
  int8_t *hor_filter = kvz_g_luma_filter[mv[0] & 3];
  int8_t *ver_filter = kvz_g_luma_filter[mv[1] & 3];

  int16_t hor_filtered[KVZ_EXT_BLOCK_W_LUMA][LCU_WIDTH];
  int16_t hor_stride = LCU_WIDTH;

  // Filter horizontally
  for (y = 0; y < height + KVZ_EXT_PADDING_LUMA; ++y) {
    for (x = 0; x < width; ++x) {
      int ypos = y - KVZ_LUMA_FILTER_OFFSET;
      int xpos = x - KVZ_LUMA_FILTER_OFFSET;
      hor_filtered[y][x] = kvz_eight_tap_filter_hor_generic(hor_filter, &src[src_stride * ypos + xpos]) >> shift1;
    }
  }

  // Filter vertically
  for (y = 0; y < height; ++y) {
    for (x = 0; x < width; ++x) {
      dst[y * dst_stride + x] = kvz_eight_tap_filter_ver_16bit_generic(ver_filter, &hor_filtered[y][x], hor_stride) >> shift2;
    }
  }
}

void kvz_filter_hpel_blocks_hor_ver_luma_generic(const encoder_control_t * encoder, 
  kvz_pixel *src,
  int16_t src_stride,
  int width,
  int height,
  kvz_pixel filtered[4][LCU_WIDTH * LCU_WIDTH],
  int16_t hor_intermediate[5][(KVZ_EXT_BLOCK_W_LUMA + 1) * LCU_WIDTH],
  int8_t fme_level,
  int16_t hor_first_cols[5][KVZ_EXT_BLOCK_W_LUMA + 1],
  int8_t hpel_off_x, int8_t hpel_off_y)
{
  int x, y, first_y;

  // Interpolation filter shifts
  int16_t shift1 = KVZ_BIT_DEPTH - 8;

  // Weighted prediction offset and shift
  int32_t wp_shift1 = 14 - KVZ_BIT_DEPTH;
  int32_t wp_offset1 = 1 << (wp_shift1 - 1);

  int8_t *fir0 = kvz_g_luma_filter[0];
  int8_t *fir2 = kvz_g_luma_filter[2];

  int16_t dst_stride = LCU_WIDTH;
  int16_t hor_stride = LCU_WIDTH;
  int32_t first_row_offset = (KVZ_LUMA_FILTER_OFFSET + 1) * hor_stride;

  int16_t *col_pos0 = hor_first_cols[0];
  int16_t *col_pos2 = hor_first_cols[2];

  // Horizontally filtered samples from the top row are
  // not needed unless samples for diagonal positions are filtered later.
  first_y = fme_level > 1 ? 0 : 1; 
                                             
  // HORIZONTAL STEP
  // Integer pixels
  for (y = 0; y < height + KVZ_EXT_PADDING_LUMA + 1; ++y) {
    for (x = 0; x < width; ++x) {
      int ypos = y - KVZ_LUMA_FILTER_OFFSET;
      int xpos = x - KVZ_LUMA_FILTER_OFFSET + 1;
      hor_intermediate[0][y * hor_stride + x] = kvz_eight_tap_filter_hor_generic(fir0, &src[src_stride*ypos + xpos]) >> shift1;
    }
  }

  // Write the first column in contiguous memory
  x = 0;
  for (y = 0; y < height + KVZ_EXT_PADDING_LUMA + 1; ++y) {
    int ypos = y - KVZ_LUMA_FILTER_OFFSET;
    int xpos = x - KVZ_LUMA_FILTER_OFFSET;
    col_pos0[y] = kvz_eight_tap_filter_hor_generic(fir0, &src[src_stride*ypos + xpos]) >> shift1;
  }

  // Half pixels
  for (y = first_y; y < height + KVZ_EXT_PADDING_LUMA + 1; ++y) {
    for (x = 0; x < width; ++x) {
      int ypos = y - KVZ_LUMA_FILTER_OFFSET;
      int xpos = x - KVZ_LUMA_FILTER_OFFSET + 1;
      hor_intermediate[1][y * hor_stride + x] = kvz_eight_tap_filter_hor_generic(fir2, &src[src_stride*ypos + xpos]) >> shift1;
    }
  }

  // Write the first column in contiguous memory
  x = 0;
  for (y = first_y; y < height + KVZ_EXT_PADDING_LUMA + 1; ++y) {
    int ypos = y - KVZ_LUMA_FILTER_OFFSET;
    int xpos = x - KVZ_LUMA_FILTER_OFFSET;
    col_pos2[y] = kvz_eight_tap_filter_hor_generic(fir2, &src[src_stride*ypos + xpos]) >> shift1;
  }

  // VERTICAL STEP

  // Right
  // Only horizontal filter
  for (y = 0; y < height; ++y) {
    for (x = 0; x < width; ++x) {
      filtered[1][y * dst_stride + x] = kvz_fast_clip_16bit_to_pixel((hor_intermediate[1][first_row_offset + y * hor_stride + x] + wp_offset1) >> wp_shift1);
    }
  }

  // Left
  // Copy from the right filtered block and the extra column
  for (y = 0; y < height; ++y) {
    x = 0;
    filtered[0][y * dst_stride + x] = kvz_fast_clip_16bit_to_pixel((col_pos2[y + KVZ_LUMA_FILTER_OFFSET + 1] + wp_offset1) >> wp_shift1);
    for (x = 1; x < width; ++x) filtered[0][y * dst_stride + x] = filtered[1][y * dst_stride + x - 1];
  }

  // Top
  // Only vertical filter
  for (y = 0; y < height; ++y) {
    int ypos = y - KVZ_LUMA_FILTER_OFFSET;
    for (x = 0; x < width; ++x) {
      int xpos = x;
      int16_t sample = kvz_eight_tap_filter_ver_generic(fir2, &src[src_stride*ypos + xpos + 1], src_stride) >> shift1;
      sample = kvz_fast_clip_16bit_to_pixel((sample + wp_offset1) >> wp_shift1);
      filtered[2][y * dst_stride + x] = sample;
    }
  }

  // Bottom
  // Copy what can be copied from the top filtered values.
  // Then filter the last row from horizontal intermediate buffer.
  for (y = 0; y < height - 1; ++y) {
    for (x = 0; x < width; ++x) filtered[3][y * dst_stride + x] = filtered[2][(y + 1) * dst_stride + x];
  }

  int ypos = y - KVZ_LUMA_FILTER_OFFSET;
  for (x = 0; x < width; ++x) {
    int xpos = x;
    int16_t sample = kvz_eight_tap_filter_ver_generic(fir2, &src[src_stride*(ypos + 1) + xpos + 1], src_stride) >> shift1;
    sample = kvz_fast_clip_16bit_to_pixel((sample + wp_offset1) >> wp_shift1);
    filtered[3][y * dst_stride + x] = sample;
  }
}

void kvz_filter_hpel_blocks_diag_luma_generic(const encoder_control_t * encoder,
  kvz_pixel *src,
  int16_t src_stride,
  int width,
  int height,
  kvz_pixel filtered[4][LCU_WIDTH * LCU_WIDTH],
  int16_t hor_intermediate[5][(KVZ_EXT_BLOCK_W_LUMA + 1) * LCU_WIDTH],
  int8_t fme_level,
  int16_t hor_first_cols[5][KVZ_EXT_BLOCK_W_LUMA + 1],
  int8_t hpel_off_x, int8_t hpel_off_y)
{
  int x, y;

  // Interpolation filter shifts
  int32_t shift2 = 6;

  // Weighted prediction offset and shift
  int32_t wp_shift1 = 14 - KVZ_BIT_DEPTH;
  int32_t wp_offset1 = 1 << (wp_shift1 - 1);

  int8_t *fir2 = kvz_g_luma_filter[2];

  int16_t dst_stride = LCU_WIDTH;
  int16_t hor_stride = LCU_WIDTH;

  // Horizontal positions
  int16_t *col_pos2 = hor_first_cols[2];

  // VERTICAL STEP

  // Top-right
  for (y = 0; y < height; ++y) {
    for (x = 0; x < width; ++x) {
      int16_t sample = kvz_eight_tap_filter_ver_16bit_generic(fir2, &hor_intermediate[1][y * hor_stride + x], hor_stride) >> shift2;
      sample = kvz_fast_clip_16bit_to_pixel((sample + wp_offset1) >> wp_shift1);
      filtered[1][y * dst_stride + x] = sample;
    }
  }

  for (y = 0; y < height; ++y) {
    x = 0;
    filtered[0][y * dst_stride + x] = kvz_fast_clip_16bit_to_pixel((col_pos2[y + KVZ_LUMA_FILTER_OFFSET + 1] + wp_offset1) >> wp_shift1);
    for (x = 1; x < width; ++x) filtered[0][y * dst_stride + x] = filtered[1][y * dst_stride + x - 1];
  }

  // Top-left
  // Copy what can be copied from top-right filtered values. Filter the first column from the column array.
  for (y = 0; y < height; ++y) {
    x = 0;
    int16_t sample = kvz_eight_tap_filter_hor_16bit_generic(fir2, &col_pos2[y]) >> shift2;
    sample = kvz_fast_clip_16bit_to_pixel((sample + wp_offset1) >> wp_shift1);
    filtered[0][y * dst_stride + x] = sample;
    for (x = 1; x < width; ++x) filtered[0][y * dst_stride + x] = filtered[1][y * dst_stride + x - 1];
  }

  // Bottom-right
  // Copy what can be copied from top-right filtered values. Filter the last row.
  for (y = 0; y < height - 1; ++y) {
    for (x = 0; x < width; ++x) filtered[3][y* dst_stride + x] = filtered[1][(y + 1) * dst_stride + x];
  }

  for (x = 0; x < width; ++x) {
    int16_t sample = kvz_eight_tap_filter_ver_16bit_generic(fir2, &hor_intermediate[1][(y + 1) * hor_stride + x], hor_stride) >> shift2;
    sample = kvz_fast_clip_16bit_to_pixel((sample + wp_offset1) >> wp_shift1);
    filtered[3][y * dst_stride + x] = sample;
  }

  // Bottom-left
  // Copy what can be copied from the top-left filtered values.
  // Copy what can be copied from the bottom-right filtered values.
  // Finally filter the last pixel from the column array.
  for (y = 0; y < height - 1; ++y) {
    for (x = 0; x < width; ++x) filtered[2][y * dst_stride + x] = filtered[0][(y + 1) * dst_stride + x];
  }
  for (x = 1; x < width; ++x) filtered[2][y * dst_stride + x] = filtered[3][y * dst_stride + x - 1];
  x = 0;
  int16_t sample = kvz_eight_tap_filter_hor_16bit_generic(fir2, &col_pos2[(y + 1)]) >> shift2;
  sample = kvz_fast_clip_16bit_to_pixel((sample + wp_offset1) >> wp_shift1);
  filtered[2][y * dst_stride + x] = sample;
}

void kvz_filter_qpel_blocks_hor_ver_luma_generic(const encoder_control_t * encoder,
  kvz_pixel *src,
  int16_t src_stride,
  int width,
  int height,
  kvz_pixel filtered[4][LCU_WIDTH * LCU_WIDTH],
  int16_t hor_intermediate[5][(KVZ_EXT_BLOCK_W_LUMA + 1) * LCU_WIDTH],
  int8_t fme_level,
  int16_t hor_first_cols[5][KVZ_EXT_BLOCK_W_LUMA + 1],
  int8_t hpel_off_x, int8_t hpel_off_y)
{
  int x, y;

  // Interpolation filter shifts
  int16_t shift1 = KVZ_BIT_DEPTH - 8;
  int32_t shift2 = 6;

  // Weighted prediction offset and shift
  int32_t wp_shift1 = 14 - KVZ_BIT_DEPTH;
  int32_t wp_offset1 = 1 << (wp_shift1 - 1);

  int8_t *fir0 = kvz_g_luma_filter[0];
  int8_t *fir2 = kvz_g_luma_filter[2];
  int8_t *fir1 = kvz_g_luma_filter[1];
  int8_t *fir3 = kvz_g_luma_filter[3];

  // Horiziontal positions. Positions 0 and 2 have already been calculated in filtered.
  int16_t *hor_pos0 = hor_intermediate[0];
  int16_t *hor_pos2 = hor_intermediate[1];
  int16_t *hor_pos_l = hor_intermediate[3];
  int16_t *hor_pos_r = hor_intermediate[4];
  int8_t *hor_fir_l  = hpel_off_x != 0 ? fir1 : fir3;
  int8_t *hor_fir_r  = hpel_off_x != 0 ? fir3 : fir1;
  int16_t *col_pos_l = hor_first_cols[1];
  int16_t *col_pos_r = hor_first_cols[3];

  int16_t dst_stride = LCU_WIDTH;
  int16_t hor_stride = LCU_WIDTH;

  int16_t *hor_hpel_pos = hpel_off_x != 0 ? hor_pos2 : hor_pos0;
  int16_t *col_pos_hor  = hpel_off_x != 0 ? hor_first_cols[2] : hor_first_cols[0];

  // Specify if integer pixels are filtered from left or/and top integer samples
  int off_x_fir_l = hpel_off_x < 1 ? 0 : 1;
  int off_x_fir_r = hpel_off_x < 0 ? 0 : 1;
  int off_y_fir_t = hpel_off_y < 1 ? 0 : 1;
  int off_y_fir_b = hpel_off_y < 0 ? 0 : 1;
  
  // HORIZONTAL STEP
  // Left QPEL
  int sample_off_y = hpel_off_y < 0 ? 0 : 1;
  for (y = 0; y < height + KVZ_EXT_PADDING_LUMA + 1; ++y) {
    for (x = 0; x < width; ++x) {
      int ypos = y - KVZ_LUMA_FILTER_OFFSET;
      int xpos = x - KVZ_LUMA_FILTER_OFFSET + 1;
      hor_pos_l[y * hor_stride + x] = kvz_eight_tap_filter_hor_generic(hor_fir_l, &src[src_stride*ypos + xpos]) >> shift1;
    }
  }

  // Write the first column in contiguous memory
  x = 0;
  for (y = 0; y < height + KVZ_EXT_PADDING_LUMA + 1; ++y) {
    int ypos = y - KVZ_LUMA_FILTER_OFFSET;
    int xpos = x - KVZ_LUMA_FILTER_OFFSET;
    col_pos_l[y] = kvz_eight_tap_filter_hor_generic(hor_fir_l, &src[src_stride*ypos + xpos]) >> shift1;
  }

  // Right QPEL
  for (y = 0; y < height + KVZ_EXT_PADDING_LUMA + 1; ++y) {
    for (x = 0; x < width; ++x) {
      int ypos = y - KVZ_LUMA_FILTER_OFFSET;
      int xpos = x - KVZ_LUMA_FILTER_OFFSET + 1;
      hor_pos_r[y * hor_stride + x] = kvz_eight_tap_filter_hor_generic(hor_fir_r, &src[src_stride*ypos + xpos]) >> shift1;
    }
  }

  // Write the first column in contiguous memory
  x = 0;
  for (y = 0; y < height + KVZ_EXT_PADDING_LUMA + 1; ++y) {
    int ypos = y - KVZ_LUMA_FILTER_OFFSET;
    int xpos = x - KVZ_LUMA_FILTER_OFFSET;
    col_pos_r[y] = kvz_eight_tap_filter_hor_generic(hor_fir_r, &src[src_stride*ypos + xpos]) >> shift1;
  }

  // VERTICAL STEP
  int8_t *ver_fir_l = hpel_off_y != 0 ? fir2 : fir0;
  int8_t *ver_fir_r = hpel_off_y != 0 ? fir2 : fir0;
  int8_t *ver_fir_t = hpel_off_y != 0 ? fir1 : fir3;
  int8_t *ver_fir_b = hpel_off_y != 0 ? fir3 : fir1;

  // Left QPEL (1/4 or 3/4 x positions) 
  for (y = 0; y < height; ++y) {
    if (!off_x_fir_l) {
      x = 0;
      int16_t sample = kvz_eight_tap_filter_hor_16bit_generic(ver_fir_l, &col_pos_l[y + sample_off_y]) >> shift2;
      sample = kvz_fast_clip_16bit_to_pixel((sample + wp_offset1) >> wp_shift1);
      filtered[0][y * dst_stride + x] = sample;
    }
    for (x = !off_x_fir_l; x < width; ++x) {
      int ypos = y + sample_off_y;
      int xpos = x - !off_x_fir_l;
      int16_t sample = kvz_eight_tap_filter_ver_16bit_generic(ver_fir_l, &hor_pos_l[ypos * hor_stride + xpos], hor_stride) >> shift2;
      sample = kvz_fast_clip_16bit_to_pixel((sample + wp_offset1) >> wp_shift1);
      filtered[0][y * dst_stride + x] = sample;
    }
  }

  // Right QPEL (3/4 or 1/4 x positions)
  for (y = 0; y < height; ++y) {
    if (!off_x_fir_r) {
      x = 0;
      int16_t sample = kvz_eight_tap_filter_hor_16bit_generic(ver_fir_r, &col_pos_r[y + sample_off_y]) >> shift2;
      sample = kvz_fast_clip_16bit_to_pixel((sample + wp_offset1) >> wp_shift1);
      filtered[1][y * dst_stride + x] = sample;
    }
    for (x = !off_x_fir_r; x < width; ++x) {
      int ypos = y + sample_off_y;
      int xpos = x - !off_x_fir_r;
      int16_t sample = kvz_eight_tap_filter_ver_16bit_generic(ver_fir_r, &hor_pos_r[ypos * hor_stride + xpos], hor_stride) >> shift2;
      sample = kvz_fast_clip_16bit_to_pixel((sample + wp_offset1) >> wp_shift1);
      filtered[1][y * dst_stride + x] = sample;
    }
  }

  // Top QPEL (1/4 or 3/4 y positions)
  int sample_off_x = (hpel_off_x > -1 ? 1 : 0);
  for (y = 0; y < height; ++y) {
    if (!sample_off_x) {
      x = 0;
      int16_t sample = kvz_eight_tap_filter_hor_16bit_generic(ver_fir_t, &col_pos_hor[y + off_y_fir_t]) >> shift2;
      sample = kvz_fast_clip_16bit_to_pixel((sample + wp_offset1) >> wp_shift1);
      filtered[2][y * dst_stride + x] = sample;
    }
    for (x = !sample_off_x; x < width; ++x) {
      int ypos = y + off_y_fir_t;
      int xpos = x - !sample_off_x;
      int16_t sample = kvz_eight_tap_filter_ver_16bit_generic(ver_fir_t, &hor_hpel_pos[ypos * hor_stride + xpos], hor_stride) >> shift2;
      sample = kvz_fast_clip_16bit_to_pixel((sample + wp_offset1) >> wp_shift1);
      filtered[2][y * dst_stride + x] = sample;
    }
  }

  // Bottom QPEL (3/4 or 1/4 y positions)
  for (y = 0; y < height; ++y) {
    if (!sample_off_x) {
      x = 0;
      int16_t sample = kvz_eight_tap_filter_hor_16bit_generic(ver_fir_b, &col_pos_hor[y + off_y_fir_b]) >> shift2;
      sample = kvz_fast_clip_16bit_to_pixel((sample + wp_offset1) >> wp_shift1);
      filtered[3][y * dst_stride + x] = sample;
    }
    for (x = !sample_off_x; x < width; ++x) {
      int ypos = y + off_y_fir_b;
      int xpos = x - !sample_off_x;
      int16_t sample = kvz_eight_tap_filter_ver_16bit_generic(ver_fir_b, &hor_hpel_pos[ypos * hor_stride + xpos], hor_stride) >> shift2;
      sample = kvz_fast_clip_16bit_to_pixel((sample + wp_offset1) >> wp_shift1);
      filtered[3][y * dst_stride + x] = sample;
    }
  }
}

void kvz_filter_qpel_blocks_diag_luma_generic(const encoder_control_t * encoder,
  kvz_pixel *src,
  int16_t src_stride,
  int width,
  int height,
  kvz_pixel filtered[4][LCU_WIDTH * LCU_WIDTH],
  int16_t hor_intermediate[5][(KVZ_EXT_BLOCK_W_LUMA + 1) * LCU_WIDTH],
  int8_t fme_level,
  int16_t hor_first_cols[5][KVZ_EXT_BLOCK_W_LUMA + 1],
  int8_t hpel_off_x, int8_t hpel_off_y)
{
  int x, y;

  // Interpolation filter shifts
  int32_t shift2 = 6;

  // Weighted prediction offset and shift
  int32_t wp_shift1 = 14 - KVZ_BIT_DEPTH;
  int32_t wp_offset1 = 1 << (wp_shift1 - 1);

  int8_t *fir1 = kvz_g_luma_filter[1];
  int8_t *fir3 = kvz_g_luma_filter[3];

  // Horiziontal positions.
  int16_t *hor_pos_l = hor_intermediate[3];
  int16_t *hor_pos_r = hor_intermediate[4];

  int16_t *col_pos_l = hor_first_cols[1];
  int16_t *col_pos_r = hor_first_cols[3];

  int16_t dst_stride = LCU_WIDTH;
  int16_t hor_stride = LCU_WIDTH;

  // VERTICAL STEP
  int8_t *ver_fir_t = hpel_off_y != 0 ? fir1 : fir3;
  int8_t *ver_fir_b = hpel_off_y != 0 ? fir3 : fir1;

  // Specify if integer pixels are filtered from left or/and top integer samples
  int off_x_fir_l = hpel_off_x < 1 ? 0 : 1;
  int off_x_fir_r = hpel_off_x < 0 ? 0 : 1;
  int off_y_fir_t = hpel_off_y < 1 ? 0 : 1;
  int off_y_fir_b = hpel_off_y < 0 ? 0 : 1;

  // Top-left QPEL
  for (y = 0; y < height; ++y) {
    if (!off_x_fir_l) {
      x = 0;
      int16_t sample = kvz_eight_tap_filter_hor_16bit_generic(ver_fir_t, &col_pos_l[y + off_y_fir_t]) >> shift2;
      sample = kvz_fast_clip_16bit_to_pixel((sample + wp_offset1) >> wp_shift1);
      filtered[0][y * dst_stride + x] = sample;
    }
    for (x = !off_x_fir_l; x < width; ++x) {
      int ypos = y + off_y_fir_t;
      int xpos = x - !off_x_fir_l;
      int16_t sample = kvz_eight_tap_filter_ver_16bit_generic(ver_fir_t, &hor_pos_l[ypos * hor_stride + xpos], hor_stride) >> shift2;
      sample = kvz_fast_clip_16bit_to_pixel((sample + wp_offset1) >> wp_shift1);
      filtered[0][y * dst_stride + x] = sample;
    }
  }

  // Top-right QPEL
  for (y = 0; y < height; ++y) {
    if (!off_x_fir_r) {
      x = 0;
      int16_t sample = kvz_eight_tap_filter_hor_16bit_generic(ver_fir_t, &col_pos_r[y + off_y_fir_t]) >> shift2;
      sample = kvz_fast_clip_16bit_to_pixel((sample + wp_offset1) >> wp_shift1);
      filtered[1][y * dst_stride + x] = sample;
    }
    for (x = !off_x_fir_r; x < width; ++x) {
      int ypos = y + off_y_fir_t;
      int xpos = x - !off_x_fir_r;
      int16_t sample = kvz_eight_tap_filter_ver_16bit_generic(ver_fir_t, &hor_pos_r[ypos * hor_stride + xpos], hor_stride) >> shift2;
      sample = kvz_fast_clip_16bit_to_pixel((sample + wp_offset1) >> wp_shift1);
      filtered[1][y * dst_stride + x] = sample;
    }
  }

  // Bottom-left QPEL
  for (y = 0; y < height; ++y) {
    if (!off_x_fir_l) {
      x = 0;
      int16_t sample = kvz_eight_tap_filter_hor_16bit_generic(ver_fir_b, &col_pos_l[y + off_y_fir_b]) >> shift2;
      sample = kvz_fast_clip_16bit_to_pixel((sample + wp_offset1) >> wp_shift1);
      filtered[2][y * dst_stride + x] = sample;
    }
    for (x = !off_x_fir_l; x < width; ++x) {
      int ypos = y + off_y_fir_b;
      int xpos = x - !off_x_fir_l;
      int16_t sample = kvz_eight_tap_filter_ver_16bit_generic(ver_fir_b, &hor_pos_l[ypos * hor_stride + xpos], hor_stride) >> shift2;
      sample = kvz_fast_clip_16bit_to_pixel((sample + wp_offset1) >> wp_shift1);
      filtered[2][y * dst_stride + x] = sample;
    }
  }

  // Bottom-right QPEL
  for (y = 0; y < height; ++y) {
    if (!off_x_fir_r) {
      x = 0;
      int16_t sample = kvz_eight_tap_filter_hor_16bit_generic(ver_fir_b, &col_pos_r[y + off_y_fir_b]) >> shift2;
      sample = kvz_fast_clip_16bit_to_pixel((sample + wp_offset1) >> wp_shift1);
      filtered[3][y * dst_stride + x] = sample;
    }
    for (x = !off_x_fir_r; x < width; ++x) {
      int ypos = y + off_y_fir_b;
      int xpos = x - !off_x_fir_r;
      int16_t sample = kvz_eight_tap_filter_ver_16bit_generic(ver_fir_b, &hor_pos_r[ypos * hor_stride + xpos], hor_stride) >> shift2;
      sample = kvz_fast_clip_16bit_to_pixel((sample + wp_offset1) >> wp_shift1);
      filtered[3][y * dst_stride + x] = sample;
    }
  }
}

void kvz_sample_octpel_chroma_generic(const encoder_control_t * const encoder, kvz_pixel *src, int16_t src_stride, int width, int height,kvz_pixel *dst, int16_t dst_stride, int8_t hor_flag, int8_t ver_flag, const int16_t mv[2])
{
  //TODO: horizontal and vertical only filtering
  int32_t x, y;

  // Interpolation filter shifts
  int16_t shift1 = KVZ_BIT_DEPTH - 8;
  int32_t shift2 = 6;

  // Weighted prediction offset and shift
  int32_t wp_shift1 = 14 - KVZ_BIT_DEPTH;
  int32_t wp_offset1 = 1 << (wp_shift1 - 1);

  // Select filters according to the fractional part of the x and y mv components
  int8_t *hor_filter = kvz_g_chroma_filter[mv[0] & 7];
  int8_t *ver_filter = kvz_g_chroma_filter[mv[1] & 7];

  int16_t hor_filtered[KVZ_EXT_BLOCK_W_CHROMA][LCU_WIDTH_C];
  int16_t hor_stride = LCU_WIDTH_C;

  // Filter horizontally
  for (y = 0; y < height + KVZ_EXT_PADDING_CHROMA; ++y) {
    for (x = 0; x < width; ++x) {
      int ypos = y - KVZ_CHROMA_FILTER_OFFSET;
      int xpos = x - KVZ_CHROMA_FILTER_OFFSET;
      hor_filtered[y][x] = kvz_four_tap_filter_hor_generic(hor_filter, &src[src_stride * ypos + xpos]) >> shift1;
    }
  }

  // Filter vertically
  for (y = 0; y < height; ++y) {
    for (x = 0; x < width; ++x) {
      dst[y * dst_stride + x] = kvz_fast_clip_32bit_to_pixel(((kvz_four_tap_filter_ver_16bit_generic(ver_filter, &hor_filtered[y][x], hor_stride) >> shift2) + wp_offset1) >> wp_shift1);
    }
  }
}

void kvz_sample_14bit_octpel_chroma_generic(const encoder_control_t * const encoder, kvz_pixel *src, int16_t src_stride, int width, int height, int16_t *dst, int16_t dst_stride, int8_t hor_flag, int8_t ver_flag, const int16_t mv[2])
{
  //TODO: horizontal and vertical only filtering
  int32_t x, y;

  // Interpolation filter shifts
  int16_t shift1 = KVZ_BIT_DEPTH - 8;
  int32_t shift2 = 6;

  // Select filters according to the fractional part of the x and y mv components
  int8_t *hor_filter = kvz_g_chroma_filter[mv[0] & 7];
  int8_t *ver_filter = kvz_g_chroma_filter[mv[1] & 7];

  int16_t hor_filtered[KVZ_EXT_BLOCK_W_CHROMA][LCU_WIDTH_C];
  int16_t hor_stride = LCU_WIDTH_C;

  // Filter horizontally
  for (y = 0; y < height + KVZ_EXT_PADDING_CHROMA; ++y) {
    for (x = 0; x < width; ++x) {
      int ypos = y - KVZ_CHROMA_FILTER_OFFSET;
      int xpos = x - KVZ_CHROMA_FILTER_OFFSET;
      hor_filtered[y][x] = kvz_four_tap_filter_hor_generic(hor_filter, &src[src_stride * ypos + xpos]) >> shift1;
    }
  }

  // Filter vertically
  for (y = 0; y < height; ++y) {
    for (x = 0; x < width; ++x) {
      dst[y * dst_stride + x] = kvz_four_tap_filter_ver_16bit_generic(ver_filter, &hor_filtered[y][x], hor_stride) >> shift2;
    }
  }
}


void kvz_get_extended_block_generic(int xpos, int ypos, int mv_x, int mv_y, int off_x, int off_y, kvz_pixel *ref, int ref_width, int ref_height,
  int filter_size, int width, int height, kvz_extended_block *out) {

  int half_filter_size = filter_size >> 1;

  out->buffer = ref + (ypos - half_filter_size + off_y + mv_y) * ref_width + (xpos - half_filter_size + off_x + mv_x);
  out->stride = ref_width;
  out->orig_topleft = out->buffer + out->stride * half_filter_size + half_filter_size;
  out->malloc_used = 0;

  int min_y = ypos - half_filter_size + off_y + mv_y;
  int max_y = min_y + height + filter_size;
  int out_of_bounds_y = (min_y < 0) || (max_y >= ref_height);

  int min_x = xpos - half_filter_size + off_x + mv_x;
  int max_x = min_x + width + filter_size;
  int out_of_bounds_x = (min_x < 0) || (max_x >= ref_width);

  int sample_out_of_bounds = out_of_bounds_y || out_of_bounds_x;

  if (sample_out_of_bounds){
    out->buffer = MALLOC(kvz_pixel, (width + filter_size) * (height + filter_size));
    if (!out->buffer){
      fprintf(stderr, "Memory allocation failed!\n");
      assert(0);
    }
    out->stride = width + filter_size;
    out->orig_topleft = out->buffer + out->stride * half_filter_size + half_filter_size;
    out->malloc_used = 1;

    int dst_y; int y; int dst_x; int x; int coord_x; int coord_y;

    for (dst_y = 0, y = ypos - half_filter_size; y < ((ypos + height)) + half_filter_size; dst_y++, y++) {

      // calculate y-pixel offset
      coord_y = y + off_y + mv_y;
      coord_y = CLIP(0, (ref_height)-1, coord_y);
      coord_y *= ref_width;

      if (!out_of_bounds_x){
        memcpy(&out->buffer[dst_y * out->stride + 0], &ref[coord_y + min_x], out->stride * sizeof(kvz_pixel));
      } else {
        for (dst_x = 0, x = (xpos)-half_filter_size; x < ((xpos + width)) + half_filter_size; dst_x++, x++) {

          coord_x = x + off_x + mv_x;
          coord_x = CLIP(0, (ref_width)-1, coord_x);

          // Store source block data (with extended borders)
          out->buffer[dst_y * out->stride + dst_x] = ref[coord_y + coord_x];
        }
      }
    }
  } 
}

int kvz_strategy_register_ipol_generic(void* opaque, uint8_t bitdepth)
{
  bool success = true;

  success &= kvz_strategyselector_register(opaque, "filter_hpel_blocks_hor_ver_luma", "generic", 0, &kvz_filter_hpel_blocks_hor_ver_luma_generic);
  success &= kvz_strategyselector_register(opaque, "filter_hpel_blocks_diag_luma", "generic", 0, &kvz_filter_hpel_blocks_diag_luma_generic);
  success &= kvz_strategyselector_register(opaque, "filter_qpel_blocks_hor_ver_luma", "generic", 0, &kvz_filter_qpel_blocks_hor_ver_luma_generic);
  success &= kvz_strategyselector_register(opaque, "filter_qpel_blocks_diag_luma", "generic", 0, &kvz_filter_qpel_blocks_diag_luma_generic);
  success &= kvz_strategyselector_register(opaque, "sample_quarterpel_luma", "generic", 0, &kvz_sample_quarterpel_luma_generic);
  success &= kvz_strategyselector_register(opaque, "sample_octpel_chroma", "generic", 0, &kvz_sample_octpel_chroma_generic);
  success &= kvz_strategyselector_register(opaque, "sample_14bit_quarterpel_luma", "generic", 0, &kvz_sample_14bit_quarterpel_luma_generic);
  success &= kvz_strategyselector_register(opaque, "sample_14bit_octpel_chroma", "generic", 0, &kvz_sample_14bit_octpel_chroma_generic);
  success &= kvz_strategyselector_register(opaque, "get_extended_block", "generic", 0, &kvz_get_extended_block_generic);

  return success;
}
