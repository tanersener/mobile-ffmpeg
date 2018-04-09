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

#include "sao.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "cabac.h"
#include "image.h"
#include "rdo.h"
#include "strategies/strategies-sao.h"


static void init_sao_info(sao_info_t *sao) {
  sao->type = SAO_TYPE_NONE;
  sao->merge_left_flag = 0;
  sao->merge_up_flag = 0;
}


static float sao_mode_bits_none(const encoder_state_t * const state, sao_info_t *sao_top, sao_info_t *sao_left)
{
  float mode_bits = 0.0;
  const cabac_data_t * const cabac = &state->cabac;
  const cabac_ctx_t *ctx = NULL;
  // FL coded merges.
  if (sao_left != NULL) {
    ctx = &(cabac->ctx.sao_merge_flag_model);
    mode_bits += CTX_ENTROPY_FBITS(ctx, 0);
  }
  if (sao_top != NULL) {    
    ctx = &(cabac->ctx.sao_merge_flag_model);
    mode_bits += CTX_ENTROPY_FBITS(ctx, 0);
  }

  // TR coded type_idx_, none = 0
  ctx = &(cabac->ctx.sao_type_idx_model);
  mode_bits += CTX_ENTROPY_FBITS(ctx, 0);

  return mode_bits;
}

static float sao_mode_bits_merge(const encoder_state_t * const state,
                                 int8_t merge_cand) {
  float mode_bits = 0.0;
  const cabac_data_t * const cabac = &state->cabac;
  const cabac_ctx_t *ctx = NULL;
  // FL coded merges.
  ctx = &(cabac->ctx.sao_merge_flag_model);

  mode_bits += CTX_ENTROPY_FBITS(ctx, merge_cand == 1);
  if (merge_cand == 1) return mode_bits;
  mode_bits += CTX_ENTROPY_FBITS(ctx, merge_cand == 2);
  return mode_bits;
}


static float sao_mode_bits_edge(const encoder_state_t * const state,
                              int edge_class, int offsets[NUM_SAO_EDGE_CATEGORIES],
                              sao_info_t *sao_top, sao_info_t *sao_left, unsigned buf_cnt)
{
  float mode_bits = 0.0;
  const cabac_data_t * const cabac = &state->cabac;
  const cabac_ctx_t *ctx = NULL;
  // FL coded merges.
  if (sao_left != NULL) {
    ctx = &(cabac->ctx.sao_merge_flag_model);   
    mode_bits += CTX_ENTROPY_FBITS(ctx, 0);
  }
  if (sao_top != NULL) {
    ctx = &(cabac->ctx.sao_merge_flag_model);
    mode_bits += CTX_ENTROPY_FBITS(ctx, 0);
  }

  // TR coded type_idx_, edge = 2 = cMax
  ctx = &(cabac->ctx.sao_type_idx_model);
  mode_bits += CTX_ENTROPY_FBITS(ctx, 1) + 1.0;

  // TR coded offsets.
  for (unsigned buf_index = 0; buf_index < buf_cnt; buf_index++) {
    sao_eo_cat edge_cat;
    for (edge_cat = SAO_EO_CAT1; edge_cat <= SAO_EO_CAT4; ++edge_cat) {
      int abs_offset = abs(offsets[edge_cat+5*buf_index]);
      if (abs_offset == 0 || abs_offset == SAO_ABS_OFFSET_MAX) {
        mode_bits += abs_offset + 1;
      } else {
        mode_bits += abs_offset + 2;
      }
    }    
  }

  mode_bits += 2.0;

  return mode_bits;
}


static float sao_mode_bits_band(const encoder_state_t * const state,
                              int band_position[2], int offsets[10],
                              sao_info_t *sao_top, sao_info_t *sao_left, unsigned buf_cnt)
{
  float mode_bits = 0.0;
  const cabac_data_t * const cabac = &state->cabac;
  const cabac_ctx_t *ctx = NULL;
  // FL coded merges.
  if (sao_left != NULL) {
    ctx = &(cabac->ctx.sao_merge_flag_model);
    mode_bits += CTX_ENTROPY_FBITS(ctx, 0);
  }
  if (sao_top != NULL) {
    ctx = &(cabac->ctx.sao_merge_flag_model);
    mode_bits += CTX_ENTROPY_FBITS(ctx, 0);
  }

  // TR coded sao_type_idx_, band = 1
  ctx = &(cabac->ctx.sao_type_idx_model);
  mode_bits += CTX_ENTROPY_FBITS(ctx, 1) + 1.0;

  // TR coded offsets and possible FL coded offset signs.
  for (unsigned buf_index = 0; buf_index < buf_cnt; buf_index++)
  {
    int i;
    for (i = 0; i < 4; ++i) {
      int abs_offset = abs(offsets[i + 1 + buf_index*5]);
      if (abs_offset == 0) {
        mode_bits += abs_offset + 1;
      } else if(abs_offset == SAO_ABS_OFFSET_MAX) {
        mode_bits += abs_offset + 1 + 1;
      } else {
        mode_bits += abs_offset + 2 + 1;
      }      
    }
  }

  // FL coded band position.
  mode_bits += 5.0 * buf_cnt;

  return mode_bits;
}


/**
 * \brief calculate an array of intensity correlations for each intensity value
 */
void kvz_calc_sao_offset_array(const encoder_control_t * const encoder, const sao_info_t *sao, int *offset, color_t color_i)
{
  int val;
  int values = (1<<encoder->bitdepth);
  int shift = encoder->bitdepth-5;
  int band_pos = (color_i == COLOR_V) ? 1 : 0;

  // Loop through all intensity values and construct an offset array
  for (val = 0; val < values; val++) {
    int cur_band = val>>shift;
    if (cur_band >= sao->band_position[band_pos] && cur_band < sao->band_position[band_pos] + 4) {
      offset[val] = CLIP(0, values - 1, val + sao->offsets[cur_band - sao->band_position[band_pos] + 1 + 5 * band_pos]);
    } else {
      offset[val] = val;
    }
  }
}


/**
 * \param orig_data  Original pixel data. 64x64 for luma, 32x32 for chroma.
 * \param rec_data  Reconstructed pixel data. 64x64 for luma, 32x32 for chroma.
 * \param sao_bands an array of bands for original and reconstructed block
 */
static int calc_sao_band_offsets(int sao_bands[2][32], int offsets[4],
                                 int *band_position)
{
  int band;
  int offset;
  int best_dist;
  int temp_dist;
  int dist[32];
  int temp_offsets[32];
  int temp_rate[32];
  int best_dist_pos = 0;

  FILL(dist, 0);
  FILL(temp_rate, 0);

  // Calculate distortion for each band using N*h^2 - 2*h*E
  for (band = 0; band < 32; band++) {
    best_dist = INT_MAX;
    offset = 0;
    if (sao_bands[1][band] != 0) {
      offset = (sao_bands[0][band] + (sao_bands[1][band] >> 1)) / sao_bands[1][band];
      offset = CLIP(-SAO_ABS_OFFSET_MAX, SAO_ABS_OFFSET_MAX, offset);
    }
    dist[band] = offset==0?0:INT_MAX;
    temp_offsets[band] = 0;
    while(offset != 0) {
      temp_dist = sao_bands[1][band]*offset*offset - 2*offset*sao_bands[0][band];

      // Store best distortion and offset
      if(temp_dist < best_dist) {
        dist[band] = temp_dist;
        temp_offsets[band] = offset;
      }
      offset += (offset > 0) ? -1:1;
    }
  }

  best_dist = INT_MAX;
  //Find starting pos for best 4 band distortions
  for (band = 0; band < 28; band++) {
    temp_dist = dist[band] + dist[band+1] + dist[band+2] + dist[band+3];
    if(temp_dist < best_dist) {
      best_dist = temp_dist;
      best_dist_pos = band;
    }
  }
  // Copy best offsets to output
  memcpy(offsets, &temp_offsets[best_dist_pos], 4*sizeof(int));

  *band_position = best_dist_pos;

  return best_dist;
}

/**
 * \param orig_data  Original pixel data. 64x64 for luma, 32x32 for chroma.
 * \param rec_data  Reconstructed pixel data. 64x64 for luma, 32x32 for chroma.
 * \param sao_bands an array of bands for original and reconstructed block
 */
static void calc_sao_bands(const encoder_state_t * const state, const kvz_pixel *orig_data, const kvz_pixel *rec_data,
                           int block_width, int block_height,
                           int sao_bands[2][32])
{
  int y, x;
  int shift = state->encoder_control->bitdepth-5;

  //Loop pixels and take top 5 bits to classify different bands
  for (y = 0; y < block_height; ++y) {
    for (x = 0; x < block_width; ++x) {
      sao_bands[0][rec_data[y * block_width + x]>>shift] += orig_data[y * block_width + x] - rec_data[y * block_width + x];
      sao_bands[1][rec_data[y * block_width + x]>>shift]++;
    }
  }
}


/**
 * \brief Reconstruct SAO.
 *
 * \param encoder         encoder state
 * \param buffer          Buffer containing the deblocked input pixels. The
 *                        area to filter starts at index 0.
 * \param stride          stride of buffer
 * \param frame_x         x-coordinate of the top-left corner in pixels
 * \param frame_y         y-coordinate of the top-left corner in pixels
 * \param width           width of the area to filter
 * \param height          height of the area to filter
 * \param sao             SAO information
 * \param color           color plane index
 */
void kvz_sao_reconstruct(const encoder_state_t *state,
                         const kvz_pixel *buffer,
                         int stride,
                         int frame_x,
                         int frame_y,
                         int width,
                         int height,
                         const sao_info_t *sao,
                         color_t color)
{
  const encoder_control_t *const ctrl = state->encoder_control;
  videoframe_t *const frame = state->tile->frame;
  const int shift = color == COLOR_Y ? 0 : 1;

  const int frame_width = frame->width >> shift;
  const int frame_height = frame->height >> shift;
  const int frame_stride = frame->rec->stride >> shift;
  kvz_pixel *output = &frame->rec->data[color][frame_x + frame_y * frame_stride];

  if (sao->type == SAO_TYPE_EDGE) {
    const vector2d_t *offset = g_sao_edge_offsets[sao->eo_class];

    if (frame_x + width + offset[0].x > frame_width ||
        frame_x + width + offset[1].x > frame_width)
    {
      // Nothing to do for the rightmost column.
      width -= 1;
    }
    if (frame_x + offset[0].x < 0 || frame_x + offset[1].x < 0) {
      // Nothing to do for the leftmost column.
      buffer += 1;
      output += 1;
      width -= 1;
    }
    if (frame_y + height + offset[0].y > frame_height ||
        frame_y + height + offset[1].y > frame_height)
    {
      // Nothing to do for the bottommost row.
      height -= 1;
    }
    if (frame_y + offset[0].y < 0 || frame_y + offset[1].y < 0) {
      // Nothing to do for the topmost row.
      buffer += stride;
      output += frame_stride;
      height -= 1;
    }
  }

  if (sao->type != SAO_TYPE_NONE) {
    kvz_sao_reconstruct_color(ctrl,
                              buffer,
                              output,
                              sao,
                              stride,
                              frame_stride,
                              width,
                              height,
                              color);
  }
}


static void sao_search_edge_sao(const encoder_state_t * const state, 
                                const kvz_pixel * data[], const kvz_pixel * recdata[],
                                int block_width, int block_height,
                                unsigned buf_cnt,
                                sao_info_t *sao_out, sao_info_t *sao_top,
                                sao_info_t *sao_left)
{
  sao_eo_class edge_class;
  // This array is used to calculate the mean offset used to minimize distortion.
  int cat_sum_cnt[2][NUM_SAO_EDGE_CATEGORIES];
  unsigned i = 0;
  

  sao_out->type = SAO_TYPE_EDGE;
  sao_out->ddistortion = INT_MAX;

  for (edge_class = SAO_EO0; edge_class <= SAO_EO3; ++edge_class) {
    int edge_offset[NUM_SAO_EDGE_CATEGORIES*2];
    int sum_ddistortion = 0;
    sao_eo_cat edge_cat;

    // Call calc_sao_edge_dir once for luma and twice for chroma.
    for (i = 0; i < buf_cnt; ++i) {
      FILL(cat_sum_cnt, 0);
      kvz_calc_sao_edge_dir(data[i], recdata[i], edge_class,
                        block_width, block_height, cat_sum_cnt);
    

      for (edge_cat = SAO_EO_CAT1; edge_cat <= SAO_EO_CAT4; ++edge_cat) {
        int cat_sum = cat_sum_cnt[0][edge_cat];
        int cat_cnt = cat_sum_cnt[1][edge_cat];

        // The optimum offset can be calculated by getting the minima of the
        // fast ddistortion estimation formula. The minima is the mean error
        // and we round that to the nearest integer.
        int offset = 0;
        if (cat_cnt != 0) {
          offset = (cat_sum + (cat_cnt >> 1)) / cat_cnt;
          offset = CLIP(-SAO_ABS_OFFSET_MAX, SAO_ABS_OFFSET_MAX, offset);
        }

        // Sharpening edge offsets can't be encoded, so set them to 0 here.
        if (edge_cat >= SAO_EO_CAT1 && edge_cat <= SAO_EO_CAT2 && offset < 0) {
          offset = 0;
        }
        if (edge_cat >= SAO_EO_CAT3 && edge_cat <= SAO_EO_CAT4 && offset > 0) {
          offset = 0;
        }

        edge_offset[edge_cat+5*i] = offset;
        // The ddistortion is amount by which the SSE of data changes. It should
        // be negative for all categories, if offset was chosen correctly.
        // ddistortion = N * h^2 - 2 * h * E, where N is the number of samples
        // and E is the sum of errors.
        // It basically says that all pixels that are not improved by offset
        // increase increase SSE by h^2 and all pixels that are improved by
        // offset decrease SSE by h*E.
        sum_ddistortion += cat_cnt * offset * offset - 2 * offset * cat_sum;
      }
    }

    {
      float mode_bits = sao_mode_bits_edge(state, edge_class, edge_offset, sao_top, sao_left, buf_cnt);
      sum_ddistortion += (int)((double)mode_bits*state->lambda +0.5);
    }
    // SAO is not applied for category 0.
    edge_offset[SAO_EO_CAT0] = 0;
    edge_offset[SAO_EO_CAT0 + 5] = 0;

    // Choose the offset class that offers the least error after offset.
    if (sum_ddistortion < sao_out->ddistortion) {
      sao_out->eo_class = edge_class;
      sao_out->ddistortion = sum_ddistortion;
      memcpy(sao_out->offsets, edge_offset, sizeof(int) * NUM_SAO_EDGE_CATEGORIES * 2);
    }
  }
}


static void sao_search_band_sao(const encoder_state_t * const state, const kvz_pixel * data[], const kvz_pixel * recdata[],
                               int block_width, int block_height,
                               unsigned buf_cnt,
                               sao_info_t *sao_out, sao_info_t *sao_top,
                               sao_info_t *sao_left)
{
  unsigned i;

  sao_out->type = SAO_TYPE_BAND;
  sao_out->ddistortion = MAX_INT;

  // Band offset
  {
    int sao_bands[2][32];
    int temp_offsets[10];
    int ddistortion = 0;
    float temp_rate = 0.0;
    
    for (i = 0; i < buf_cnt; ++i) {
      FILL(sao_bands, 0);
      calc_sao_bands(state, data[i], recdata[i],block_width,
                     block_height,sao_bands);
    

      ddistortion += calc_sao_band_offsets(sao_bands, &temp_offsets[1+5*i], &sao_out->band_position[i]);      
    }

    temp_rate = sao_mode_bits_band(state, sao_out->band_position, temp_offsets, sao_top, sao_left, buf_cnt);
    ddistortion += (int)((double)temp_rate*state->lambda + 0.5);

    // Select band sao over edge sao when distortion is lower
    if (ddistortion < sao_out->ddistortion) {
      sao_out->type = SAO_TYPE_BAND;
      sao_out->ddistortion = ddistortion;
      memcpy(&sao_out->offsets[0], &temp_offsets[0], sizeof(int) * buf_cnt * 5);
    }
  }
}


/**
 * \param data     Array of pointers to reference pixels.
 * \param recdata  Array of pointers to reconstructed pixels.
 * \param block_width   Width of the area to be examined.
 * \param block_height  Height of the area to be examined.
 * \param buf_cnt  Number of pointers data and recdata have.
 * \param sao_out  Output parameter for the best sao parameters.
 */
static void sao_search_best_mode(const encoder_state_t * const state, const kvz_pixel * data[], const kvz_pixel * recdata[],
                                 int block_width, int block_height,
                                 unsigned buf_cnt,
                                 sao_info_t *sao_out, sao_info_t *sao_top,
                                 sao_info_t *sao_left, int32_t merge_cost[3])
{
  sao_info_t edge_sao;
  sao_info_t band_sao;

  init_sao_info(&edge_sao);
  init_sao_info(&band_sao);
  
  //Avoid "random" uninitialized value
  edge_sao.band_position[0] = edge_sao.band_position[1] = 0;
  edge_sao.eo_class = SAO_EO0;
  band_sao.offsets[0] = 0;
  band_sao.offsets[5] = 0;
  band_sao.eo_class = SAO_EO0;

  if (state->encoder_control->cfg.sao_type & 1){
    sao_search_edge_sao(state, data, recdata, block_width, block_height, buf_cnt, &edge_sao, sao_top, sao_left);
    float mode_bits = sao_mode_bits_edge(state, edge_sao.eo_class, edge_sao.offsets, sao_top, sao_left, buf_cnt);
    int ddistortion = (int)(mode_bits * state->lambda + 0.5);
    unsigned buf_i;
    
    for (buf_i = 0; buf_i < buf_cnt; ++buf_i) {
      ddistortion += kvz_sao_edge_ddistortion(data[buf_i], recdata[buf_i], 
                                          block_width, block_height,
                                          edge_sao.eo_class, &edge_sao.offsets[5 * buf_i]);
    }
    
    edge_sao.ddistortion = ddistortion;
  }
  else{
    edge_sao.ddistortion = INT_MAX;
  }

  if (state->encoder_control->cfg.sao_type & 2){
    sao_search_band_sao(state, data, recdata, block_width, block_height, buf_cnt, &band_sao, sao_top, sao_left);
    float mode_bits = sao_mode_bits_band(state, band_sao.band_position, band_sao.offsets, sao_top, sao_left, buf_cnt);
    int ddistortion = (int)(mode_bits * state->lambda + 0.5);
    unsigned buf_i;
    
    for (buf_i = 0; buf_i < buf_cnt; ++buf_i) {
      ddistortion += kvz_sao_band_ddistortion(state, data[buf_i], recdata[buf_i], 
                                          block_width, block_height, 
                                          band_sao.band_position[buf_i], &band_sao.offsets[1 + 5 * buf_i]);
    }
    
    band_sao.ddistortion = ddistortion;
  }
  else{
    band_sao.ddistortion = INT_MAX;
  }

  if (edge_sao.ddistortion <= band_sao.ddistortion) {
    *sao_out = edge_sao;
    merge_cost[0] = edge_sao.ddistortion;
  } else {
    *sao_out = band_sao;
    merge_cost[0] = band_sao.ddistortion;
  }

  // Choose between SAO and doing nothing, taking into account the
  // rate-distortion cost of coding do nothing.
  {
    int cost_of_nothing = (int)(sao_mode_bits_none(state, sao_top, sao_left) * state->lambda + 0.5);
    if (sao_out->ddistortion >= cost_of_nothing) {
      sao_out->type = SAO_TYPE_NONE;
      merge_cost[0] = cost_of_nothing;
    }
  }

  // Calculate merge costs
  if (sao_top || sao_left) {
    sao_info_t* merge_sao[2] = { sao_left, sao_top};
    int i;
    for (i = 0; i < 2; i++) {
      sao_info_t* merge_cand = merge_sao[i];

      if (merge_cand) {
        unsigned buf_i;
        float mode_bits = sao_mode_bits_merge(state, i + 1);
        int ddistortion = (int)(mode_bits * state->lambda + 0.5);

        switch (merge_cand->type) {
          case SAO_TYPE_EDGE:
                for (buf_i = 0; buf_i < buf_cnt; ++buf_i) {
                  ddistortion += kvz_sao_edge_ddistortion(data[buf_i], recdata[buf_i],
                    block_width, block_height,
                    merge_cand->eo_class, &merge_cand->offsets[5 * buf_i]);
                }
                merge_cost[i + 1] = ddistortion;
            break;
          case SAO_TYPE_BAND:
              for (buf_i = 0; buf_i < buf_cnt; ++buf_i) {
                ddistortion += kvz_sao_band_ddistortion(state, data[buf_i], recdata[buf_i],
                  block_width, block_height,
                  merge_cand->band_position[buf_i], &merge_cand->offsets[1 + 5 * buf_i]);
              }
              merge_cost[i + 1] = ddistortion;
            break;
          case SAO_TYPE_NONE:
            merge_cost[i + 1] = ddistortion;
            break;
          }
      }
    }
  }

  return;
}

static void sao_search_chroma(const encoder_state_t * const state, const videoframe_t *frame, unsigned x_ctb, unsigned y_ctb, sao_info_t *sao, sao_info_t *sao_top, sao_info_t *sao_left, int32_t merge_cost[3])
{
  int block_width  = (LCU_WIDTH / 2);
  int block_height = (LCU_WIDTH / 2);
  const kvz_pixel *orig_list[2];
  const kvz_pixel *rec_list[2];
  kvz_pixel orig[2][LCU_CHROMA_SIZE];
  kvz_pixel rec[2][LCU_CHROMA_SIZE];
  color_t color_i;

  // Check for right and bottom boundaries.
  if (x_ctb * (LCU_WIDTH / 2) + (LCU_WIDTH / 2) >= (unsigned)frame->width / 2) {
    block_width = (frame->width - x_ctb * LCU_WIDTH) / 2;
  }
  if (y_ctb * (LCU_WIDTH / 2) + (LCU_WIDTH / 2) >= (unsigned)frame->height / 2) {
    block_height = (frame->height - y_ctb * LCU_WIDTH) / 2;
  }

  sao->type = SAO_TYPE_EDGE;

  // Copy data to temporary buffers and init orig and rec lists to point to those buffers.
  for (color_i = COLOR_U; color_i <= COLOR_V; ++color_i) {
    kvz_pixel *data = &frame->source->data[color_i][CU_TO_PIXEL(x_ctb, y_ctb, 1, frame->source->stride / 2)];
    kvz_pixel *recdata = &frame->rec->data[color_i][CU_TO_PIXEL(x_ctb, y_ctb, 1, frame->rec->stride / 2)];
    kvz_pixels_blit(data, orig[color_i - 1], block_width, block_height,
                        frame->source->stride / 2, block_width);
    kvz_pixels_blit(recdata, rec[color_i - 1], block_width, block_height,
                        frame->rec->stride / 2, block_width);
    orig_list[color_i - 1] = &orig[color_i - 1][0];
    rec_list[color_i - 1] = &rec[color_i - 1][0];
  }

  // Calculate
  sao_search_best_mode(state, orig_list, rec_list, block_width, block_height, 2, sao, sao_top, sao_left, merge_cost);
}

static void sao_search_luma(const encoder_state_t * const state, const videoframe_t *frame, unsigned x_ctb, unsigned y_ctb, sao_info_t *sao, sao_info_t *sao_top, sao_info_t *sao_left, int32_t merge_cost[3])
{
  kvz_pixel orig[LCU_LUMA_SIZE];
  kvz_pixel rec[LCU_LUMA_SIZE];
  const kvz_pixel * orig_list[1] = { NULL };
  const kvz_pixel * rec_list[1] = { NULL };
  kvz_pixel *data = &frame->source->y[CU_TO_PIXEL(x_ctb, y_ctb, 0, frame->source->stride)];
  kvz_pixel *recdata = &frame->rec->y[CU_TO_PIXEL(x_ctb, y_ctb, 0, frame->rec->stride)];
  int block_width = LCU_WIDTH;
  int block_height = LCU_WIDTH;

  // Check for right and bottom boundaries.
  if (x_ctb * LCU_WIDTH + LCU_WIDTH >= (unsigned)frame->width) {
    block_width = frame->width - x_ctb * LCU_WIDTH;
  }
  if (y_ctb * LCU_WIDTH + LCU_WIDTH >= (unsigned)frame->height) {
    block_height = frame->height - y_ctb * LCU_WIDTH;
  }

  sao->type = SAO_TYPE_EDGE;

  // Fill temporary buffers with picture data.
  kvz_pixels_blit(data, orig, block_width, block_height, frame->source->stride, block_width);
  kvz_pixels_blit(recdata, rec, block_width, block_height, frame->rec->stride, block_width);

  orig_list[0] = orig;
  rec_list[0] = rec;
  sao_search_best_mode(state, orig_list, rec_list, block_width, block_height, 1, sao, sao_top, sao_left, merge_cost);
}

void kvz_sao_search_lcu(const encoder_state_t* const state, int lcu_x, int lcu_y)
{
  assert(!state->encoder_control->cfg.lossless);

  videoframe_t* const frame = state->tile->frame;
  const int stride = frame->width_in_lcu;
  int32_t merge_cost_luma[3] = { INT32_MAX };
  int32_t merge_cost_chroma[3] = { INT32_MAX };
  sao_info_t *sao_luma = &frame->sao_luma[lcu_y * stride + lcu_x];
  sao_info_t *sao_chroma = NULL;
  int enable_chroma = state->encoder_control->chroma_format != KVZ_CSP_400;
  if (enable_chroma) {
    sao_chroma = &frame->sao_chroma[lcu_y * stride + lcu_x];
  }

  // Merge candidates
  sao_info_t *sao_top_luma    = lcu_y != 0 ? &frame->sao_luma  [(lcu_y - 1) * stride + lcu_x] : NULL;
  sao_info_t *sao_left_luma   = lcu_x != 0 ? &frame->sao_luma  [lcu_y       * stride + lcu_x - 1] : NULL;
  sao_info_t *sao_top_chroma  = NULL;
  sao_info_t *sao_left_chroma = NULL;
  if (enable_chroma) {
    if (lcu_y != 0) sao_top_chroma =  &frame->sao_chroma[(lcu_y - 1) * stride + lcu_x];
    if (lcu_x != 0) sao_left_chroma = &frame->sao_chroma[lcu_y       * stride + lcu_x - 1];
  }

  sao_search_luma(state, frame, lcu_x, lcu_y, sao_luma, sao_top_luma, sao_left_luma, merge_cost_luma);
  if (enable_chroma) {
    sao_search_chroma(state, frame, lcu_x, lcu_y, sao_chroma, sao_top_chroma, sao_left_chroma, merge_cost_chroma);
  } else {
    merge_cost_chroma[0] = 0;
    merge_cost_chroma[1] = 0;
    merge_cost_chroma[2] = 0;
  }

  sao_luma->merge_up_flag = sao_luma->merge_left_flag = 0;
  // Check merge costs
  if (sao_top_luma) {
    // Merge up if cost is equal or smaller to the searched mode cost
    if (merge_cost_luma[2] + merge_cost_chroma[2] <= merge_cost_luma[0] + merge_cost_chroma[0]) {
      *sao_luma = *sao_top_luma;
      if (sao_top_chroma) *sao_chroma = *sao_top_chroma;
      sao_luma->merge_up_flag = 1;
      sao_luma->merge_left_flag = 0;
    }
  }
  if (sao_left_luma) {
    // Merge left if cost is equal or smaller to the searched mode cost
    // AND smaller than merge up cost, if merge up was already chosen
    if (merge_cost_luma[1] + merge_cost_chroma[1] <= merge_cost_luma[0] + merge_cost_chroma[0]) {
      if (!sao_luma->merge_up_flag || merge_cost_luma[1] + merge_cost_chroma[1] < merge_cost_luma[2] + merge_cost_chroma[2]) {
        *sao_luma = *sao_left_luma;
        if (sao_left_chroma) *sao_chroma = *sao_left_chroma;
        sao_luma->merge_left_flag = 1;
        sao_luma->merge_up_flag = 0;
      }
    }
  }
  assert(sao_luma->eo_class < SAO_NUM_EO);
  CHECKPOINT_SAO_INFO("sao_luma", *sao_luma);

  if (sao_chroma) {
    assert(sao_chroma->eo_class < SAO_NUM_EO);
    CHECKPOINT_SAO_INFO("sao_chroma", *sao_chroma);
  }
}
