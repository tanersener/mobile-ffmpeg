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

void kvz_filter_inter_quarterpel_luma_generic(const encoder_control_t * const encoder, kvz_pixel *src, int16_t src_stride, int width, int height, kvz_pixel *dst, int16_t dst_stride, int8_t hor_flag, int8_t ver_flag)
{
  //TODO: horizontal and vertical only filtering
  int32_t x, y;
  int16_t shift1 = KVZ_BIT_DEPTH - 8;
  int32_t shift2 = 6;
  int32_t shift3 = 14 - KVZ_BIT_DEPTH;
  int32_t offset23 = 1 << (shift2 + shift3 - 1);

  //coefficients for 1/4, 2/4 and 3/4 positions
  int8_t *c0, *c1, *c2, *c3;

  c0 = kvz_g_luma_filter[0];
  c1 = kvz_g_luma_filter[1];
  c2 = kvz_g_luma_filter[2];
  c3 = kvz_g_luma_filter[3];

  #define FILTER_OFFSET 3
  #define FILTER_SIZE 8

  int16_t flipped_hor_filtered[4 * (LCU_WIDTH + 1) + FILTER_SIZE][(LCU_WIDTH + 1) + FILTER_SIZE];

  // Filter horizontally and flip x and y
  for (x = 0; x < width; ++x) {
    for (y = 0; y < height + FILTER_SIZE - 1; ++y) {
      int ypos = y - FILTER_OFFSET;
      int xpos = x - FILTER_OFFSET;
      // Original pixel
      flipped_hor_filtered[4 * x + 0][y] = (c0[FILTER_OFFSET] * src[src_stride*ypos + xpos + FILTER_OFFSET]) >> shift1;
      flipped_hor_filtered[4 * x + 1][y] = kvz_eight_tap_filter_hor_generic(c1, &src[src_stride*ypos + xpos]) >> shift1;
      flipped_hor_filtered[4 * x + 2][y] = kvz_eight_tap_filter_hor_generic(c2, &src[src_stride*ypos + xpos]) >> shift1;
      flipped_hor_filtered[4 * x + 3][y] = kvz_eight_tap_filter_hor_generic(c3, &src[src_stride*ypos + xpos]) >> shift1;
    }
  }

  // Filter vertically and flip x and y
  for (x = 0; x < 4 * width; ++x) {
    for (y = 0; y < height; ++y) {
      int ypos = y;
      int xpos = x;
      dst[(4 * y + 0)*dst_stride + x] = kvz_fast_clip_32bit_to_pixel(((c0[FILTER_OFFSET] * flipped_hor_filtered[xpos][ypos + FILTER_OFFSET] + offset23) >> shift2) >> shift3); 
      dst[(4 * y + 1)*dst_stride + x] = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_hor_16bit_generic(c1, &flipped_hor_filtered[xpos][ypos]) + offset23) >> shift2) >> shift3);
      dst[(4 * y + 2)*dst_stride + x] = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_hor_16bit_generic(c2, &flipped_hor_filtered[xpos][ypos]) + offset23) >> shift2) >> shift3);
      dst[(4 * y + 3)*dst_stride + x] = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_hor_16bit_generic(c3, &flipped_hor_filtered[xpos][ypos]) + offset23) >> shift2) >> shift3);

    }
  }
}

void kvz_sample_quarterpel_luma_generic(const encoder_control_t * const encoder, kvz_pixel *src, int16_t src_stride, int width, int height, kvz_pixel *dst, int16_t dst_stride, int8_t hor_flag, int8_t ver_flag, const int16_t mv[2])
{
  //TODO: horizontal and vertical only filtering
  int32_t x, y;
  int16_t shift1 = KVZ_BIT_DEPTH - 8;
  int32_t shift2 = 6;
  int32_t shift3 = 14 - KVZ_BIT_DEPTH;
  int32_t offset23 = 1 << (shift2 + shift3 - 1);

  //coefficients for 1/4, 2/4 and 3/4 positions
  int8_t *hor_filter = kvz_g_luma_filter[mv[0]&3];
  int8_t *ver_filter = kvz_g_luma_filter[mv[1]&3];

  int16_t flipped_hor_filtered[(LCU_WIDTH + 1) + FILTER_SIZE][(LCU_WIDTH + 1) + FILTER_SIZE];

  // Filter horizontally and flip x and y
  for (x = 0; x < width; ++x) {
    for (y = 0; y < height + FILTER_SIZE - 1; ++y) {
      int ypos = y - FILTER_OFFSET;
      int xpos = x - FILTER_OFFSET;
      flipped_hor_filtered[x][y] = kvz_eight_tap_filter_hor_generic(hor_filter, &src[src_stride*ypos + xpos]) >> shift1;
    }
  }

  // Filter vertically and flip x and y
  for (x = 0; x < width; ++x) {
    for (y = 0; y < height; ++y) {
      int ypos = y;
      int xpos = x;
      dst[y*dst_stride + x] = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_hor_16bit_generic(ver_filter, &flipped_hor_filtered[xpos][ypos]) + offset23) >> shift2) >> shift3);
    }
  }
}

void kvz_sample_14bit_quarterpel_luma_generic(const encoder_control_t * const encoder, kvz_pixel *src, int16_t src_stride, int width, int height, int16_t *dst, int16_t dst_stride, int8_t hor_flag, int8_t ver_flag, const int16_t mv[2])
{
  //TODO: horizontal and vertical only filtering
  int32_t x, y;
  int16_t shift1 = KVZ_BIT_DEPTH - 8;
  int32_t shift2 = 6;

  //coefficients for 1/4, 2/4 and 3/4 positions
  int8_t *hor_filter = kvz_g_luma_filter[mv[0] & 3];
  int8_t *ver_filter = kvz_g_luma_filter[mv[1] & 3];

  int16_t flipped_hor_filtered[(LCU_WIDTH + 1) + FILTER_SIZE][(LCU_WIDTH + 1) + FILTER_SIZE];

  // Filter horizontally and flip x and y
  for (x = 0; x < width; ++x) {
    for (y = 0; y < height + FILTER_SIZE - 1; ++y) {
      int ypos = y - FILTER_OFFSET;
      int xpos = x - FILTER_OFFSET;
      flipped_hor_filtered[x][y] = kvz_eight_tap_filter_hor_generic(hor_filter, &src[src_stride*ypos + xpos]) >> shift1;
    }
  }

  // Filter vertically and flip x and y
  for (x = 0; x < width; ++x) {
    for (y = 0; y < height; ++y) {
      int ypos = y;
      int xpos = x;
      dst[y*dst_stride + x] = (kvz_eight_tap_filter_hor_16bit_generic(ver_filter, &flipped_hor_filtered[xpos][ypos])) >> shift2;
    }
  }
}

/**
 * \brief Interpolation for chroma half-pixel
 * \param src source image in integer pels (-2..width+3, -2..height+3)
 * \param src_stride stride of source image
 * \param width width of source image block
 * \param height height of source image block
 * \param dst destination image in half-pixel resolution
 * \param dst_stride stride of destination image
 *
 */
void kvz_filter_inter_halfpel_chroma_generic(const encoder_control_t * const encoder, kvz_pixel *src, int16_t src_stride, int width, int height, kvz_pixel *dst, int16_t dst_stride, int8_t hor_flag, int8_t ver_flag)
{
  /* ____________
  * | B0,0|ae0,0|
  * |ea0,0|ee0,0|
  *
  * ae0,0 = (-4*B-1,0  + 36*B0,0  + 36*B1,0  - 4*B2,0)  >> shift1
  * ea0,0 = (-4*B0,-1  + 36*B0,0  + 36*B0,1  - 4*B0,2)  >> shift1
  * ee0,0 = (-4*ae0,-1 + 36*ae0,0 + 36*ae0,1 - 4*ae0,2) >> shift2
  */
  int32_t x, y;
  int32_t shift1 = KVZ_BIT_DEPTH - 8;
  int32_t shift2 = 6;
  int32_t shift3 = 14 - KVZ_BIT_DEPTH;
  int32_t offset3 = 1 << (shift3 - 1);
  int32_t offset23 = 1 << (shift2 + shift3 - 1);

  int8_t* c = kvz_g_chroma_filter[4];
  int16_t temp[4] = {0,0,0,0};

  // Loop source pixels and generate four filtered half-pel pixels on each round
  for (y = 0; y < height; y++) {
    int dst_pos_y = (y << 1)*dst_stride;
    int src_pos_y = y*src_stride;
    for (x = 0; x < width; x++) {
      // Calculate current dst and src pixel positions
      int dst_pos = dst_pos_y + (x << 1);
      int src_pos = src_pos_y + x;

      // Original pixel (not really needed)
      dst[dst_pos] = src[src_pos]; //B0,0

      // ae0,0 - We need this only when hor_flag and for ee0,0
      if (hor_flag) {
        temp[1] = kvz_four_tap_filter_hor_generic(c, &src[src_pos - 1]) >> shift1; // ae0,0
      }
      // ea0,0 - needed only when ver_flag
      if (ver_flag) {
        dst[dst_pos + 1 * dst_stride] = kvz_fast_clip_32bit_to_pixel(((kvz_four_tap_filter_ver_generic(c, &src[src_pos - src_stride], src_stride) >> shift1) + (1 << (shift3 - 1))) >> shift3); // ea0,0
      }

      // When both flags, we use _only_ this pixel (but still need ae0,0 for it)
      if (hor_flag && ver_flag) {
        // Calculate temporary values..
        src_pos -= src_stride;  //0,-1
        temp[0] = (kvz_four_tap_filter_hor_generic(c, &src[src_pos - 1]) >> shift1); // ae0,-1
        src_pos += 2 * src_stride;  //0,1
        temp[2] = (kvz_four_tap_filter_hor_generic(c, &src[src_pos - 1]) >> shift1); // ae0,1
        src_pos += src_stride;  //0,2
        temp[3] = (kvz_four_tap_filter_hor_generic(c, &src[src_pos - 1]) >> shift1); // ae0,2
        
        dst[dst_pos + 1 * dst_stride + 1] = kvz_fast_clip_32bit_to_pixel(((kvz_four_tap_filter_hor_16bit_generic(c, temp) + offset23) >> shift2) >> shift3); // ee0,0
      }

      if (hor_flag) {
        dst[dst_pos + 1] = kvz_fast_clip_32bit_to_pixel((temp[1] + offset3) >> shift3);
      }
    }
  }
}

void kvz_filter_inter_octpel_chroma_generic(const encoder_control_t * const encoder, kvz_pixel *src, int16_t src_stride, int width, int height, kvz_pixel *dst, int16_t dst_stride, int8_t hor_flag, int8_t ver_flag)
{

  int32_t x, y;
  int32_t shift1 = KVZ_BIT_DEPTH - 8;
  int32_t shift2 = 6;
  int32_t shift3 = 14 - KVZ_BIT_DEPTH;
  int32_t offset3 = 1 << (shift3 - 1);
  int32_t offset23 = 1 << (shift2 + shift3 - 1);

  //coefficients for 1/8, 2/8, 3/8, 4/8, 5/8, 6/8 and 7/8 positions
  int8_t *c1, *c2, *c3, *c4, *c5, *c6, *c7;

  int i;
  c1 = kvz_g_chroma_filter[1];
  c2 = kvz_g_chroma_filter[2];
  c3 = kvz_g_chroma_filter[3];
  c4 = kvz_g_chroma_filter[4];
  c5 = kvz_g_chroma_filter[5];
  c6 = kvz_g_chroma_filter[6];
  c7 = kvz_g_chroma_filter[7];

  int16_t temp[7][4]; // Temporary horizontal values calculated from integer pixels


  // Loop source pixels and generate 64 filtered 1/8-pel pixels on each round
  for (y = 0; y < height; y++) {
    int dst_pos_y = (y << 3)*dst_stride;
    int src_pos_y = y*src_stride;
    for (x = 0; x < width; x++) {
      // Calculate current dst and src pixel positions
      int dst_pos = dst_pos_y + (x << 3);
      int src_pos = src_pos_y + x;
      
      // Original pixel
      dst[dst_pos] = src[src_pos];

      // Horizontal 1/8-values
      if (hor_flag && !ver_flag) {

        temp[0][1] = (kvz_four_tap_filter_hor_generic(c1, &src[src_pos - 1]) >> shift1); // ae0,0 h0
        temp[1][1] = (kvz_four_tap_filter_hor_generic(c2, &src[src_pos - 1]) >> shift1);
        temp[2][1] = (kvz_four_tap_filter_hor_generic(c3, &src[src_pos - 1]) >> shift1);
        temp[3][1] = (kvz_four_tap_filter_hor_generic(c4, &src[src_pos - 1]) >> shift1);
        temp[4][1] = (kvz_four_tap_filter_hor_generic(c5, &src[src_pos - 1]) >> shift1);
        temp[5][1] = (kvz_four_tap_filter_hor_generic(c6, &src[src_pos - 1]) >> shift1);
        temp[6][1] = (kvz_four_tap_filter_hor_generic(c7, &src[src_pos - 1]) >> shift1);

      }

      // Vertical 1/8-values
      if (ver_flag) {
        dst[dst_pos + 1 * dst_stride] = kvz_fast_clip_32bit_to_pixel(((kvz_four_tap_filter_ver_generic(c1, &src[src_pos - 1 * src_stride], src_stride) >> shift1) + (1 << (shift3 - 1))) >> shift3); //
        dst[dst_pos + 2 * dst_stride] = kvz_fast_clip_32bit_to_pixel(((kvz_four_tap_filter_ver_generic(c2, &src[src_pos - 1 * src_stride], src_stride) >> shift1) + (1 << (shift3 - 1))) >> shift3);
        dst[dst_pos + 3 * dst_stride] = kvz_fast_clip_32bit_to_pixel(((kvz_four_tap_filter_ver_generic(c3, &src[src_pos - 1 * src_stride], src_stride) >> shift1) + (1 << (shift3 - 1))) >> shift3);
        dst[dst_pos + 4 * dst_stride] = kvz_fast_clip_32bit_to_pixel(((kvz_four_tap_filter_ver_generic(c4, &src[src_pos - 1 * src_stride], src_stride) >> shift1) + (1 << (shift3 - 1))) >> shift3);
        dst[dst_pos + 5 * dst_stride] = kvz_fast_clip_32bit_to_pixel(((kvz_four_tap_filter_ver_generic(c5, &src[src_pos - 1 * src_stride], src_stride) >> shift1) + (1 << (shift3 - 1))) >> shift3);
        dst[dst_pos + 6 * dst_stride] = kvz_fast_clip_32bit_to_pixel(((kvz_four_tap_filter_ver_generic(c6, &src[src_pos - 1 * src_stride], src_stride) >> shift1) + (1 << (shift3 - 1))) >> shift3);
        dst[dst_pos + 7 * dst_stride] = kvz_fast_clip_32bit_to_pixel(((kvz_four_tap_filter_ver_generic(c7, &src[src_pos - 1 * src_stride], src_stride) >> shift1) + (1 << (shift3 - 1))) >> shift3);
      }

      // When both flags, interpolate values from temporary horizontal values
      if (hor_flag && ver_flag) {

        // Calculate temporary values
        src_pos -= 1 * src_stride;  //0,-3
        for (i = 0; i < 4; ++i) {

          temp[0][i] = (kvz_four_tap_filter_hor_generic(c1, &src[src_pos + i * src_stride - 1]) >> shift1);
          temp[1][i] = (kvz_four_tap_filter_hor_generic(c2, &src[src_pos + i * src_stride - 1]) >> shift1);
          temp[2][i] = (kvz_four_tap_filter_hor_generic(c3, &src[src_pos + i * src_stride - 1]) >> shift1);
          temp[3][i] = (kvz_four_tap_filter_hor_generic(c4, &src[src_pos + i * src_stride - 1]) >> shift1);
          temp[4][i] = (kvz_four_tap_filter_hor_generic(c5, &src[src_pos + i * src_stride - 1]) >> shift1);
          temp[5][i] = (kvz_four_tap_filter_hor_generic(c6, &src[src_pos + i * src_stride - 1]) >> shift1);
          temp[6][i] = (kvz_four_tap_filter_hor_generic(c7, &src[src_pos + i * src_stride - 1]) >> shift1);

        }


        //Calculate values from temporary horizontal 1/8-values
        for (i = 0; i<7; ++i){
          dst[dst_pos + 1 * dst_stride + i + 1] = kvz_fast_clip_32bit_to_pixel(((kvz_four_tap_filter_hor_16bit_generic(c1, &temp[i][0]) + offset23) >> shift2) >> shift3); // ee0,0
          dst[dst_pos + 2 * dst_stride + i + 1] = kvz_fast_clip_32bit_to_pixel(((kvz_four_tap_filter_hor_16bit_generic(c2, &temp[i][0]) + offset23) >> shift2) >> shift3);
          dst[dst_pos + 3 * dst_stride + i + 1] = kvz_fast_clip_32bit_to_pixel(((kvz_four_tap_filter_hor_16bit_generic(c3, &temp[i][0]) + offset23) >> shift2) >> shift3);
          dst[dst_pos + 4 * dst_stride + i + 1] = kvz_fast_clip_32bit_to_pixel(((kvz_four_tap_filter_hor_16bit_generic(c4, &temp[i][0]) + offset23) >> shift2) >> shift3);
          dst[dst_pos + 5 * dst_stride + i + 1] = kvz_fast_clip_32bit_to_pixel(((kvz_four_tap_filter_hor_16bit_generic(c5, &temp[i][0]) + offset23) >> shift2) >> shift3);
          dst[dst_pos + 6 * dst_stride + i + 1] = kvz_fast_clip_32bit_to_pixel(((kvz_four_tap_filter_hor_16bit_generic(c6, &temp[i][0]) + offset23) >> shift2) >> shift3);
          dst[dst_pos + 7 * dst_stride + i + 1] = kvz_fast_clip_32bit_to_pixel(((kvz_four_tap_filter_hor_16bit_generic(c7, &temp[i][0]) + offset23) >> shift2) >> shift3);
          
        }

      }

      if (hor_flag) {
        dst[dst_pos + 1] = kvz_fast_clip_32bit_to_pixel((temp[0][1] + offset3) >> shift3);
        dst[dst_pos + 2] = kvz_fast_clip_32bit_to_pixel((temp[1][1] + offset3) >> shift3);
        dst[dst_pos + 3] = kvz_fast_clip_32bit_to_pixel((temp[2][1] + offset3) >> shift3);
        dst[dst_pos + 4] = kvz_fast_clip_32bit_to_pixel((temp[3][1] + offset3) >> shift3);
        dst[dst_pos + 5] = kvz_fast_clip_32bit_to_pixel((temp[4][1] + offset3) >> shift3);
        dst[dst_pos + 6] = kvz_fast_clip_32bit_to_pixel((temp[5][1] + offset3) >> shift3);
        dst[dst_pos + 7] = kvz_fast_clip_32bit_to_pixel((temp[6][1] + offset3) >> shift3);
      }


    }
  }
}

void kvz_filter_hpel_blocks_hor_ver_luma_generic(const encoder_control_t * const encoder, kvz_pixel *src, int16_t src_stride, int width, int height, frac_search_block *filtered)
{
  int x, y;
  int16_t shift1 = KVZ_BIT_DEPTH - 8;
  int32_t shift2 = 6;
  int32_t shift3 = 14 - KVZ_BIT_DEPTH;
  int32_t offset23 = 1 << (shift2 + shift3 - 1);

  int8_t *fir0 = kvz_g_luma_filter[0];
  int8_t *fir2 = kvz_g_luma_filter[2];

  int16_t flipped0[(LCU_WIDTH + 1) * (KVZ_EXT_BLOCK_W + 1)];
  int16_t flipped2[(LCU_WIDTH + 1) * (KVZ_EXT_BLOCK_W + 1)];

  int16_t temp_stride = height + KVZ_EXT_PADDING + 1;
  int16_t dst_stride = (LCU_WIDTH + 1);

  // Horizontal positions
  for (x = 0; x < width + 1; ++x) {
    for (y = 0; y < height + KVZ_EXT_PADDING + 1; ++y) {
      int ypos = y - FILTER_OFFSET;
      int xpos = x - FILTER_OFFSET;
      flipped0[x * temp_stride + y] = kvz_eight_tap_filter_hor_generic(fir0, &src[src_stride*ypos + xpos]) >> shift1;
      flipped2[x * temp_stride + y] = kvz_eight_tap_filter_hor_generic(fir2, &src[src_stride*ypos + xpos]) >> shift1;
    }
  }

  // Filter vertically and flip x and y
  for (x = 0; x < width + 1; ++x) {
    for (y = 0; y < height + 1; ++y) {
      filtered[HPEL_POS_HOR][y * dst_stride + x]  = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_hor_16bit_generic(fir0, &flipped2[x * temp_stride + y]) + offset23) >> shift2) >> shift3);
      filtered[HPEL_POS_VER][y * dst_stride + x]  = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_hor_16bit_generic(fir2, &flipped0[x * temp_stride + y]) + offset23) >> shift2) >> shift3);
    }
  }
}

void kvz_filter_hpel_blocks_full_luma_generic(const encoder_control_t * const encoder, kvz_pixel *src, int16_t src_stride, int width, int height, frac_search_block *filtered)
{
  int x, y;
  int16_t shift1 = KVZ_BIT_DEPTH - 8;
  int32_t shift2 = 6;
  int32_t shift3 = 14 - KVZ_BIT_DEPTH;
  int32_t offset23 = 1 << (shift2 + shift3 - 1);

  int8_t *fir0 = kvz_g_luma_filter[0];
  int8_t *fir2 = kvz_g_luma_filter[2];

  int16_t flipped0[(LCU_WIDTH + 1) * (KVZ_EXT_BLOCK_W + 1)];
  int16_t flipped2[(LCU_WIDTH + 1) * (KVZ_EXT_BLOCK_W + 1)];

  int16_t temp_stride = height + KVZ_EXT_PADDING + 1;
  int16_t dst_stride = (LCU_WIDTH + 1);

  // Horizontal positions
  for (x = 0; x < width + 1; ++x) {
    for (y = 0; y < height + KVZ_EXT_PADDING + 1; ++y) {
      int ypos = y - FILTER_OFFSET;
      int xpos = x - FILTER_OFFSET;
      flipped0[x * temp_stride + y] = kvz_eight_tap_filter_hor_generic(fir0, &src[src_stride*ypos + xpos]) >> shift1;
      flipped2[x * temp_stride + y] = kvz_eight_tap_filter_hor_generic(fir2, &src[src_stride*ypos + xpos]) >> shift1;
    }
  }

  // Filter vertically and flip x and y
  for (x = 0; x < width + 1; ++x) {
    for (y = 0; y < height + 1; ++y) {
      filtered[HPEL_POS_HOR][y * dst_stride + x]  = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_hor_16bit_generic(fir0, &flipped2[x * temp_stride + y]) + offset23) >> shift2) >> shift3);
      filtered[HPEL_POS_VER][y * dst_stride + x]  = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_hor_16bit_generic(fir2, &flipped0[x * temp_stride + y]) + offset23) >> shift2) >> shift3);
      filtered[HPEL_POS_DIA][y * dst_stride + x]  = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_hor_16bit_generic(fir2, &flipped2[x * temp_stride + y]) + offset23) >> shift2) >> shift3);
    }
  }
}

void kvz_filter_qpel_blocks_hor_ver_luma_generic(const encoder_control_t * const encoder, kvz_pixel *src, int16_t src_stride, int width, int height, frac_search_block *filtered)
{
  int x, y;
  int16_t shift1 = KVZ_BIT_DEPTH - 8;
  int32_t shift2 = 6;
  int32_t shift3 = 14 - KVZ_BIT_DEPTH;
  int32_t offset23 = 1 << (shift2 + shift3 - 1);

  int8_t *fir0 = kvz_g_luma_filter[0];
  int8_t *fir2 = kvz_g_luma_filter[2];
  int8_t *fir1 = kvz_g_luma_filter[1];
  int8_t *fir3 = kvz_g_luma_filter[3];

  int16_t flipped0[(LCU_WIDTH + 1) * (KVZ_EXT_BLOCK_W + 1)];
  int16_t flipped2[(LCU_WIDTH + 1) * (KVZ_EXT_BLOCK_W + 1)];
  int16_t flipped1[(LCU_WIDTH + 1) * (KVZ_EXT_BLOCK_W + 1)];
  int16_t flipped3[(LCU_WIDTH + 1) * (KVZ_EXT_BLOCK_W + 1)];

  int16_t temp_stride = height + KVZ_EXT_PADDING + 1;
  int16_t dst_stride = (LCU_WIDTH + 1);
  
  // Horizontal positions
  for (x = 0; x < width + 1; ++x) {
    for (y = 0; y < height + KVZ_EXT_PADDING + 1; ++y) {
      int ypos = y - FILTER_OFFSET;
      int xpos = x - FILTER_OFFSET;
      flipped0[x * temp_stride + y] = kvz_eight_tap_filter_hor_generic(fir0, &src[src_stride*ypos + xpos]) >> shift1;
      flipped2[x * temp_stride + y] = kvz_eight_tap_filter_hor_generic(fir2, &src[src_stride*ypos + xpos]) >> shift1;
      flipped1[x * temp_stride + y] = kvz_eight_tap_filter_hor_generic(fir1, &src[src_stride*ypos + xpos]) >> shift1;
      flipped3[x * temp_stride + y] = kvz_eight_tap_filter_hor_generic(fir3, &src[src_stride*ypos + xpos]) >> shift1;
    }
  }

  // Filter vertically and flip x and y
  for (x = 0; x < width + 1; ++x) {
    for (y = 0; y < height + 1; ++y) {
      
      // HPEL
      filtered[ 0][y * dst_stride + x]  = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_hor_16bit_generic(fir0, &flipped2[x * temp_stride + y]) + offset23) >> shift2) >> shift3);
      filtered[ 1][y * dst_stride + x]  = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_hor_16bit_generic(fir2, &flipped0[x * temp_stride + y]) + offset23) >> shift2) >> shift3);
      filtered[ 2][y * dst_stride + x]  = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_hor_16bit_generic(fir2, &flipped2[x * temp_stride + y]) + offset23) >> shift2) >> shift3);
      
      // QPEL
      // Horizontal
      filtered[ 3][y * dst_stride + x]  = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_hor_16bit_generic(fir0, &flipped1[x * temp_stride + y]) + offset23) >> shift2) >> shift3);
      filtered[ 4][y * dst_stride + x]  = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_hor_16bit_generic(fir0, &flipped3[x * temp_stride + y]) + offset23) >> shift2) >> shift3);
      filtered[ 5][y * dst_stride + x]  = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_hor_16bit_generic(fir2, &flipped1[x * temp_stride + y]) + offset23) >> shift2) >> shift3);
      filtered[ 6][y * dst_stride + x]  = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_hor_16bit_generic(fir2, &flipped3[x * temp_stride + y]) + offset23) >> shift2) >> shift3);

      // Vertical
      filtered[ 7][y * dst_stride + x]  = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_hor_16bit_generic(fir1, &flipped0[x * temp_stride + y]) + offset23) >> shift2) >> shift3);
      filtered[ 8][y * dst_stride + x]  = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_hor_16bit_generic(fir1, &flipped2[x * temp_stride + y]) + offset23) >> shift2) >> shift3);
      filtered[ 9][y * dst_stride + x]  = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_hor_16bit_generic(fir3, &flipped0[x * temp_stride + y]) + offset23) >> shift2) >> shift3);
      filtered[10][y * dst_stride + x]  = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_hor_16bit_generic(fir3, &flipped2[x * temp_stride + y]) + offset23) >> shift2) >> shift3);
    }
  }
}

void kvz_filter_qpel_blocks_full_luma_generic(const encoder_control_t * const encoder, kvz_pixel *src, int16_t src_stride, int width, int height, frac_search_block *filtered)
{
  int x, y;
  int16_t shift1 = KVZ_BIT_DEPTH - 8;
  int32_t shift2 = 6;
  int32_t shift3 = 14 - KVZ_BIT_DEPTH;
  int32_t offset23 = 1 << (shift2 + shift3 - 1);

  int8_t *fir0 = kvz_g_luma_filter[0];
  int8_t *fir2 = kvz_g_luma_filter[2];
  int8_t *fir1 = kvz_g_luma_filter[1];
  int8_t *fir3 = kvz_g_luma_filter[3];

  int16_t flipped0[(LCU_WIDTH + 1) * (KVZ_EXT_BLOCK_W + 1)];
  int16_t flipped2[(LCU_WIDTH + 1) * (KVZ_EXT_BLOCK_W + 1)];
  int16_t flipped1[(LCU_WIDTH + 1) * (KVZ_EXT_BLOCK_W + 1)];
  int16_t flipped3[(LCU_WIDTH + 1) * (KVZ_EXT_BLOCK_W + 1)];

  int16_t temp_stride = height + KVZ_EXT_PADDING + 1;
  int16_t dst_stride = (LCU_WIDTH + 1);
  
  // Horizontal positions
  for (x = 0; x < width + 1; ++x) {
    for (y = 0; y < height + KVZ_EXT_PADDING + 1; ++y) {
      int ypos = y - FILTER_OFFSET;
      int xpos = x - FILTER_OFFSET;
      flipped0[x * temp_stride + y] = kvz_eight_tap_filter_hor_generic(fir0, &src[src_stride*ypos + xpos]) >> shift1;
      flipped2[x * temp_stride + y] = kvz_eight_tap_filter_hor_generic(fir2, &src[src_stride*ypos + xpos]) >> shift1;
      flipped1[x * temp_stride + y] = kvz_eight_tap_filter_hor_generic(fir1, &src[src_stride*ypos + xpos]) >> shift1;
      flipped3[x * temp_stride + y] = kvz_eight_tap_filter_hor_generic(fir3, &src[src_stride*ypos + xpos]) >> shift1;
    }
  }

  // Filter vertically and flip x and y
  for (x = 0; x < width + 1; ++x) {
    for (y = 0; y < height + 1; ++y) {
      
      // HPEL
      filtered[ 0][y * dst_stride + x]  = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_hor_16bit_generic(fir0, &flipped2[x * temp_stride + y]) + offset23) >> shift2) >> shift3);
      filtered[ 1][y * dst_stride + x]  = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_hor_16bit_generic(fir2, &flipped0[x * temp_stride + y]) + offset23) >> shift2) >> shift3);
      filtered[ 2][y * dst_stride + x]  = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_hor_16bit_generic(fir2, &flipped2[x * temp_stride + y]) + offset23) >> shift2) >> shift3);
      
      // QPEL
      // Horizontal
      filtered[ 3][y * dst_stride + x]  = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_hor_16bit_generic(fir0, &flipped1[x * temp_stride + y]) + offset23) >> shift2) >> shift3);
      filtered[ 4][y * dst_stride + x]  = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_hor_16bit_generic(fir0, &flipped3[x * temp_stride + y]) + offset23) >> shift2) >> shift3);
      filtered[ 5][y * dst_stride + x]  = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_hor_16bit_generic(fir2, &flipped1[x * temp_stride + y]) + offset23) >> shift2) >> shift3);
      filtered[ 6][y * dst_stride + x]  = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_hor_16bit_generic(fir2, &flipped3[x * temp_stride + y]) + offset23) >> shift2) >> shift3);

      // Vertical
      filtered[ 7][y * dst_stride + x]  = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_hor_16bit_generic(fir1, &flipped0[x * temp_stride + y]) + offset23) >> shift2) >> shift3);
      filtered[ 8][y * dst_stride + x]  = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_hor_16bit_generic(fir1, &flipped2[x * temp_stride + y]) + offset23) >> shift2) >> shift3);
      filtered[ 9][y * dst_stride + x]  = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_hor_16bit_generic(fir3, &flipped0[x * temp_stride + y]) + offset23) >> shift2) >> shift3);
      filtered[10][y * dst_stride + x]  = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_hor_16bit_generic(fir3, &flipped2[x * temp_stride + y]) + offset23) >> shift2) >> shift3);

      // Diagonal
      filtered[11][y * dst_stride + x]  = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_hor_16bit_generic(fir1, &flipped1[x * temp_stride + y]) + offset23) >> shift2) >> shift3);
      filtered[12][y * dst_stride + x]  = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_hor_16bit_generic(fir1, &flipped3[x * temp_stride + y]) + offset23) >> shift2) >> shift3);
      filtered[13][y * dst_stride + x]  = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_hor_16bit_generic(fir3, &flipped1[x * temp_stride + y]) + offset23) >> shift2) >> shift3);
      filtered[14][y * dst_stride + x]  = kvz_fast_clip_32bit_to_pixel(((kvz_eight_tap_filter_hor_16bit_generic(fir3, &flipped3[x * temp_stride + y]) + offset23) >> shift2) >> shift3);    
    }
  }
}

void kvz_filter_frac_blocks_luma_generic(const encoder_control_t * const encoder, kvz_pixel *src, int16_t src_stride, int width, int height, frac_search_block filtered[15], int8_t fme_level)
{
  switch (fme_level) {
    case 1:
      kvz_filter_hpel_blocks_hor_ver_luma_generic(encoder, src, src_stride, width, height, filtered);
      break;
    case 2:
      kvz_filter_hpel_blocks_full_luma_generic(encoder, src, src_stride, width, height, filtered);
      break;
    case 3:
      kvz_filter_qpel_blocks_hor_ver_luma_generic(encoder, src, src_stride, width, height, filtered);
      break;
    default:
      kvz_filter_qpel_blocks_full_luma_generic(encoder, src, src_stride, width, height, filtered);
      break;
  }
}

void kvz_sample_octpel_chroma_generic(const encoder_control_t * const encoder, kvz_pixel *src, int16_t src_stride, int width, int height,kvz_pixel *dst, int16_t dst_stride, int8_t hor_flag, int8_t ver_flag, const int16_t mv[2])
{
  //TODO: horizontal and vertical only filtering
  int32_t x, y;
  int16_t shift1 = KVZ_BIT_DEPTH - 8;
  int32_t shift2 = 6;
  int32_t shift3 = 14 - KVZ_BIT_DEPTH;
  int32_t offset23 = 1 << (shift2 + shift3 - 1);

  int8_t *hor_filter = kvz_g_chroma_filter[mv[0] & 7];
  int8_t *ver_filter = kvz_g_chroma_filter[mv[1] & 7];

#define FILTER_SIZE_C (FILTER_SIZE / 2)
#define FILTER_OFFSET_C (FILTER_OFFSET / 2)
  int16_t flipped_hor_filtered[(LCU_WIDTH_C + 1) + FILTER_SIZE_C][(LCU_WIDTH_C + 1) + FILTER_SIZE_C];

  // Filter horizontally and flip x and y
  for (x = 0; x < width; ++x) {
    for (y = 0; y < height + FILTER_SIZE_C - 1; ++y) {
      int ypos = y - FILTER_OFFSET_C;
      int xpos = x - FILTER_OFFSET_C;
      flipped_hor_filtered[x][y] = kvz_four_tap_filter_hor_generic(hor_filter, &src[src_stride*ypos + xpos]) >> shift1;
    }
  }

  // Filter vertically and flip x and y
  for (x = 0; x < width; ++x) {
    for (y = 0; y < height; ++y) {
      int ypos = y;
      int xpos = x;
      dst[y*dst_stride + x] = kvz_fast_clip_32bit_to_pixel(((kvz_four_tap_filter_hor_16bit_generic(ver_filter, &flipped_hor_filtered[xpos][ypos]) + offset23) >> shift2) >> shift3);
    }
  }
}

void kvz_sample_14bit_octpel_chroma_generic(const encoder_control_t * const encoder, kvz_pixel *src, int16_t src_stride, int width, int height, int16_t *dst, int16_t dst_stride, int8_t hor_flag, int8_t ver_flag, const int16_t mv[2])
{
  //TODO: horizontal and vertical only filtering
  int32_t x, y;
  int16_t shift1 = KVZ_BIT_DEPTH - 8;
  int32_t shift2 = 6;
  int8_t *hor_filter = kvz_g_chroma_filter[mv[0] & 7];
  int8_t *ver_filter = kvz_g_chroma_filter[mv[1] & 7];

#define FILTER_SIZE_C (FILTER_SIZE / 2)
#define FILTER_OFFSET_C (FILTER_OFFSET / 2)
  int16_t flipped_hor_filtered[(LCU_WIDTH_C + 1) + FILTER_SIZE_C][(LCU_WIDTH_C + 1) + FILTER_SIZE_C];

  // Filter horizontally and flip x and y
  for (x = 0; x < width; ++x) {
    for (y = 0; y < height + FILTER_SIZE_C - 1; ++y) {
      int ypos = y - FILTER_OFFSET_C;
      int xpos = x - FILTER_OFFSET_C;
      flipped_hor_filtered[x][y] = kvz_four_tap_filter_hor_generic(hor_filter, &src[src_stride*ypos + xpos]) >> shift1;
    }
  }

  // Filter vertically and flip x and y
  for (x = 0; x < width; ++x) {
    for (y = 0; y < height; ++y) {
      int ypos = y;
      int xpos = x;
      dst[y*dst_stride + x] = (kvz_four_tap_filter_hor_16bit_generic(ver_filter, &flipped_hor_filtered[xpos][ypos])) >> shift2;
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

  success &= kvz_strategyselector_register(opaque, "filter_inter_quarterpel_luma", "generic", 0, &kvz_filter_inter_quarterpel_luma_generic);
  success &= kvz_strategyselector_register(opaque, "filter_inter_halfpel_chroma", "generic", 0, &kvz_filter_inter_halfpel_chroma_generic);
  success &= kvz_strategyselector_register(opaque, "filter_inter_octpel_chroma", "generic", 0, &kvz_filter_inter_octpel_chroma_generic);
  success &= kvz_strategyselector_register(opaque, "filter_frac_blocks_luma", "generic", 0, &kvz_filter_frac_blocks_luma_generic);
  success &= kvz_strategyselector_register(opaque, "sample_quarterpel_luma", "generic", 0, &kvz_sample_quarterpel_luma_generic);
  success &= kvz_strategyselector_register(opaque, "sample_octpel_chroma", "generic", 0, &kvz_sample_octpel_chroma_generic);
  success &= kvz_strategyselector_register(opaque, "sample_14bit_quarterpel_luma", "generic", 0, &kvz_sample_14bit_quarterpel_luma_generic);
  success &= kvz_strategyselector_register(opaque, "sample_14bit_octpel_chroma", "generic", 0, &kvz_sample_14bit_octpel_chroma_generic);
  success &= kvz_strategyselector_register(opaque, "get_extended_block", "generic", 0, &kvz_get_extended_block_generic);

  return success;
}
