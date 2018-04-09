#ifndef STRATEGIES_IPOL_H_
#define STRATEGIES_IPOL_H_
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
 * Interface for subpixel interpolation functions.
 */

#include "encoder.h"
#include "global.h" // IWYU pragma: keep
#include "kvazaar.h"
#include "search_inter.h"


typedef struct { kvz_pixel *buffer; kvz_pixel *orig_topleft; unsigned stride; unsigned malloc_used; } kvz_extended_block;

typedef unsigned(ipol_func)(const encoder_control_t * encoder, kvz_pixel *src, int16_t src_stride, int width, int height, kvz_pixel *dst,
  int16_t dst_stride, int8_t hor_flag, int8_t ver_flag);

typedef unsigned(ipol_frac_blocks_func)(const encoder_control_t * encoder, kvz_pixel *src, int16_t src_stride, int width, int height,
  frac_search_block filtered_out[15], int8_t fme_level);

typedef unsigned(epol_func)(int xpos, int ypos, int mv_x, int mv_y, int off_x, int off_y, kvz_pixel *ref, int ref_width, int ref_height,
  int filter_size, int width, int height, kvz_extended_block *out);

typedef void(kvz_sample_quarterpel_luma_func)(const encoder_control_t * const encoder, kvz_pixel *src, int16_t src_stride, int width, int height, kvz_pixel *dst, int16_t dst_stride, int8_t hor_flag, int8_t ver_flag, const int16_t mv[2]);
typedef void(kvz_sample_octpel_chroma_func)(const encoder_control_t * const encoder, kvz_pixel *src, int16_t src_stride, int width, int height, kvz_pixel *dst, int16_t dst_stride, int8_t hor_flag, int8_t ver_flag, const int16_t mv[2]);

typedef void(kvz_sample_14bit_quarterpel_luma_func)(const encoder_control_t * const encoder, kvz_pixel *src, int16_t src_stride, int width, int height, int16_t *dst, int16_t dst_stride, int8_t hor_flag, int8_t ver_flag, const int16_t mv[2]);
typedef void(kvz_sample_14bit_octpel_chroma_func)(const encoder_control_t * const encoder, kvz_pixel *src, int16_t src_stride, int width, int height, int16_t *dst, int16_t dst_stride, int8_t hor_flag, int8_t ver_flag, const int16_t mv[2]);

// Declare function pointers.
extern ipol_func * kvz_filter_inter_quarterpel_luma;
extern ipol_func * kvz_filter_inter_halfpel_chroma;
extern ipol_func * kvz_filter_inter_octpel_chroma;
extern ipol_frac_blocks_func *kvz_filter_frac_blocks_luma;
extern epol_func * kvz_get_extended_block;
extern kvz_sample_quarterpel_luma_func * kvz_sample_quarterpel_luma;
extern kvz_sample_octpel_chroma_func * kvz_sample_octpel_chroma;
extern kvz_sample_14bit_quarterpel_luma_func * kvz_sample_14bit_quarterpel_luma;
extern kvz_sample_14bit_octpel_chroma_func * kvz_sample_14bit_octpel_chroma;


int kvz_strategy_register_ipol(void* opaque, uint8_t bitdepth);


#define STRATEGIES_IPOL_EXPORTS \
  {"filter_inter_quarterpel_luma", (void**) &kvz_filter_inter_quarterpel_luma}, \
  {"filter_inter_halfpel_chroma", (void**) &kvz_filter_inter_halfpel_chroma}, \
  {"filter_inter_octpel_chroma", (void**) &kvz_filter_inter_octpel_chroma}, \
  {"filter_frac_blocks_luma", (void**) &kvz_filter_frac_blocks_luma}, \
  {"sample_quarterpel_luma", (void**) &kvz_sample_quarterpel_luma}, \
  {"sample_octpel_chroma", (void**) &kvz_sample_octpel_chroma}, \
  {"sample_14bit_quarterpel_luma", (void**) &kvz_sample_14bit_quarterpel_luma}, \
  {"sample_14bit_octpel_chroma", (void**) &kvz_sample_14bit_octpel_chroma}, \
  {"get_extended_block", (void**) &kvz_get_extended_block}, \



#endif //STRATEGIES_IPOL_H_
