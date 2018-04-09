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

#include "strategies/generic/sao-generic.h"

#include "cu.h"
#include "encoder.h"
#include "encoderstate.h"
#include "kvazaar.h"
#include "sao.h"
#include "strategyselector.h"


// Mapping of edge_idx values to eo-classes.
static int sao_calc_eo_cat(kvz_pixel a, kvz_pixel b, kvz_pixel c)
{
  // Mapping relationships between a, b and c to eo_idx.
  static const int sao_eo_idx_to_eo_category[] = { 1, 2, 0, 3, 4 };

  int eo_idx = 2 + SIGN3((int)c - (int)a) + SIGN3((int)c - (int)b);

  return sao_eo_idx_to_eo_category[eo_idx];
}


static int sao_edge_ddistortion_generic(const kvz_pixel *orig_data,
                                        const kvz_pixel *rec_data,
                                        int block_width,
                                        int block_height,
                                        int eo_class,
                                        int offsets[NUM_SAO_EDGE_CATEGORIES])
{
  int y, x;
  int sum = 0;
  vector2d_t a_ofs = g_sao_edge_offsets[eo_class][0];
  vector2d_t b_ofs = g_sao_edge_offsets[eo_class][1];

  for (y = 1; y < block_height - 1; ++y) {
    for (x = 1; x < block_width - 1; ++x) {
      const kvz_pixel *c_data = &rec_data[y * block_width + x];
      kvz_pixel a = c_data[a_ofs.y * block_width + a_ofs.x];
      kvz_pixel c = c_data[0];
      kvz_pixel b = c_data[b_ofs.y * block_width + b_ofs.x];

      int offset = offsets[sao_calc_eo_cat(a, b, c)];

      if (offset != 0) {
        int diff = orig_data[y * block_width + x] - c;
        // Offset is applied to reconstruction, so it is subtracted from diff.
        sum += (diff - offset) * (diff - offset) - diff * diff;
      }
    }
  }

  return sum;
}


/**
 * \param orig_data  Original pixel data. 64x64 for luma, 32x32 for chroma.
 * \param rec_data  Reconstructed pixel data. 64x64 for luma, 32x32 for chroma.
 * \param dir_offsets
 * \param is_chroma  0 for luma, 1 for chroma. Indicates
 */
static void calc_sao_edge_dir_generic(const kvz_pixel *orig_data,
                                      const kvz_pixel *rec_data,
                                      int eo_class,
                                      int block_width,
                                      int block_height,
                                      int cat_sum_cnt[2][NUM_SAO_EDGE_CATEGORIES])
{
  int y, x;
  vector2d_t a_ofs = g_sao_edge_offsets[eo_class][0];
  vector2d_t b_ofs = g_sao_edge_offsets[eo_class][1];
  // Arrays orig_data and rec_data are quarter size for chroma.

  // Don't sample the edge pixels because this function doesn't have access to
  // their neighbours.
  for (y = 1; y < block_height - 1; ++y) {
    for (x = 1; x < block_width - 1; ++x) {
      const kvz_pixel *c_data = &rec_data[y * block_width + x];
      kvz_pixel a = c_data[a_ofs.y * block_width + a_ofs.x];
      kvz_pixel c = c_data[0];
      kvz_pixel b = c_data[b_ofs.y * block_width + b_ofs.x];

      int eo_cat = sao_calc_eo_cat(a, b, c);

      cat_sum_cnt[0][eo_cat] += orig_data[y * block_width + x] - c;
      cat_sum_cnt[1][eo_cat] += 1;
    }
  }
}


static void sao_reconstruct_color_generic(const encoder_control_t * const encoder,
                                          const kvz_pixel *rec_data,
                                          kvz_pixel *new_rec_data,
                                          const sao_info_t *sao,
                                          int stride,
                                          int new_stride,
                                          int block_width,
                                          int block_height,
                                          color_t color_i)
{
  // Arrays orig_data and rec_data are quarter size for chroma.
  int offset_v = color_i == COLOR_V ? 5 : 0;

  if (sao->type == SAO_TYPE_BAND) {
    int offsets[1<<KVZ_BIT_DEPTH];
    kvz_calc_sao_offset_array(encoder, sao, offsets, color_i);
    for (int y = 0; y < block_height; ++y) {
      for (int x = 0; x < block_width; ++x) {
        new_rec_data[y * new_stride + x] = offsets[rec_data[y * stride + x]];
      }
    }
  } else {
    // Don't sample the edge pixels because this function doesn't have access to
    // their neighbours.
    for (int y = 0; y < block_height; ++y) {
      for (int x = 0; x < block_width; ++x) {
        vector2d_t a_ofs = g_sao_edge_offsets[sao->eo_class][0];
        vector2d_t b_ofs = g_sao_edge_offsets[sao->eo_class][1];
        const kvz_pixel *c_data = &rec_data[y * stride + x];
        kvz_pixel *new_data = &new_rec_data[y * new_stride + x];
        kvz_pixel a = c_data[a_ofs.y * stride + a_ofs.x];
        kvz_pixel c = c_data[0];
        kvz_pixel b = c_data[b_ofs.y * stride + b_ofs.x];

        int eo_cat = sao_calc_eo_cat(a, b, c);

        new_data[0] = (kvz_pixel)CLIP(0, (1 << KVZ_BIT_DEPTH) - 1, c_data[0] + sao->offsets[eo_cat + offset_v]);
      }
    }
  }
}


static int sao_band_ddistortion_generic(const encoder_state_t * const state,
                                        const kvz_pixel *orig_data,
                                        const kvz_pixel *rec_data,
                                        int block_width,
                                        int block_height,
                                        int band_pos,
                                        int sao_bands[4])
{
  int y, x;
  int shift = state->encoder_control->bitdepth-5;
  int sum = 0;

  for (y = 0; y < block_height; ++y) {
    for (x = 0; x < block_width; ++x) {
      int band = (rec_data[y * block_width + x] >> shift) - band_pos;
      int offset = 0;
      if (band >= 0 && band < 4) {
        offset = sao_bands[band];
      }
      if (offset != 0) {
        int diff = orig_data[y * block_width + x] - rec_data[y * block_width + x];
        // Offset is applied to reconstruction, so it is subtracted from diff.
        sum += (diff - offset) * (diff - offset) - diff * diff;
      }
    }
  }

  return sum;
}


int kvz_strategy_register_sao_generic(void* opaque, uint8_t bitdepth)
{
  bool success = true;

  success &= kvz_strategyselector_register(opaque, "sao_edge_ddistortion", "generic", 0, &sao_edge_ddistortion_generic);
  success &= kvz_strategyselector_register(opaque, "calc_sao_edge_dir", "generic", 0, &calc_sao_edge_dir_generic);
  success &= kvz_strategyselector_register(opaque, "sao_reconstruct_color", "generic", 0, &sao_reconstruct_color_generic);
  success &= kvz_strategyselector_register(opaque, "sao_band_ddistortion", "generic", 0, &sao_band_ddistortion_generic);

  return success;
}
