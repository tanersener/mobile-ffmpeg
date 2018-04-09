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

#include "search_intra.h"

#include <limits.h>

#include "cabac.h"
#include "encoder.h"
#include "encoderstate.h"
#include "image.h"
#include "intra.h"
#include "kvazaar.h"
#include "rdo.h"
#include "search.h"
#include "strategies/strategies-picture.h"
#include "videoframe.h"


// Normalize SAD for comparison against SATD to estimate transform skip
// for 4x4 blocks.
#ifndef TRSKIP_RATIO
# define TRSKIP_RATIO 1.7
#endif


/**
 * \brief Sort modes and costs to ascending order according to costs.
 */
static INLINE void sort_modes(int8_t *__restrict modes, double *__restrict costs, uint8_t length)
{
  // Length is always between 5 and 23, and is either 21, 17, 9 or 8 about
  // 60% of the time, so there should be no need for anything more complex
  // than insertion sort.
  for (uint8_t i = 1; i < length; ++i) {
    const double cur_cost = costs[i];
    const int8_t cur_mode = modes[i];
    uint8_t j = i;
    while (j > 0 && cur_cost < costs[j - 1]) {
      costs[j] = costs[j - 1];
      modes[j] = modes[j - 1];
      --j;
    }
    costs[j] = cur_cost;
    modes[j] = cur_mode;
  }
}


/**
* \brief Select mode with the smallest cost.
*/
static INLINE uint8_t select_best_mode_index(const int8_t *modes, const double *costs, uint8_t length)
{
  uint8_t best_index = 0;
  double best_cost = costs[0];
  
  for (uint8_t i = 1; i < length; ++i) {
    if (costs[i] < best_cost) {
      best_cost = costs[i];
      best_index = i;
    }
  }

  return best_index;
}


/**
 * \brief Calculate quality of the reconstruction.
 *
 * \param pred  Predicted pixels in continous memory.
 * \param orig_block  Orignal (target) pixels in continous memory.
 * \param satd_func  SATD function for this block size.
 * \param sad_func  SAD function this block size.
 * \param width  Pixel width of the block.
 *
 * \return  Estimated RD cost of the reconstruction and signaling the
 *     coefficients of the residual.
 */
static double get_cost(encoder_state_t * const state, 
                       kvz_pixel *pred, kvz_pixel *orig_block,
                       cost_pixel_nxn_func *satd_func,
                       cost_pixel_nxn_func *sad_func,
                       int width)
{
  double satd_cost = satd_func(pred, orig_block);
  if (TRSKIP_RATIO != 0 && width == 4 && state->encoder_control->cfg.trskip_enable) {
    // If the mode looks better with SAD than SATD it might be a good
    // candidate for transform skip. How much better SAD has to be is
    // controlled by TRSKIP_RATIO.

    // Add the offset bit costs of signaling 'luma and chroma use trskip',
    // versus signaling 'luma and chroma don't use trskip' to the SAD cost.
    const cabac_ctx_t *ctx = &state->cabac.ctx.transform_skip_model_luma;
    double trskip_bits = CTX_ENTROPY_FBITS(ctx, 1) - CTX_ENTROPY_FBITS(ctx, 0);

    if (state->encoder_control->chroma_format != KVZ_CSP_400) {
      ctx = &state->cabac.ctx.transform_skip_model_chroma;
      trskip_bits += 2.0 * (CTX_ENTROPY_FBITS(ctx, 1) - CTX_ENTROPY_FBITS(ctx, 0));
    }

    double sad_cost = TRSKIP_RATIO * sad_func(pred, orig_block) + state->lambda_sqrt * trskip_bits;
    if (sad_cost < satd_cost) {
      return sad_cost;
    }
  }
  return satd_cost;
}


/**
 * \brief Calculate quality of the reconstruction.
 *
 * \param a  bc
 *
 * \return  
 */
static void get_cost_dual(encoder_state_t * const state, 
                       const pred_buffer preds, const kvz_pixel *orig_block,
                       cost_pixel_nxn_multi_func *satd_twin_func,
                       cost_pixel_nxn_multi_func *sad_twin_func,
                       int width, double *costs_out)
{
  #define PARALLEL_BLKS 2
  unsigned satd_costs[PARALLEL_BLKS] = { 0 };
  satd_twin_func(preds, orig_block, PARALLEL_BLKS, satd_costs);
  costs_out[0] = (double)satd_costs[0];
  costs_out[1] = (double)satd_costs[1];

  if (TRSKIP_RATIO != 0 && width == 4 && state->encoder_control->cfg.trskip_enable) {
    // If the mode looks better with SAD than SATD it might be a good
    // candidate for transform skip. How much better SAD has to be is
    // controlled by TRSKIP_RATIO.

    // Add the offset bit costs of signaling 'luma and chroma use trskip',
    // versus signaling 'luma and chroma don't use trskip' to the SAD cost.
    const cabac_ctx_t *ctx = &state->cabac.ctx.transform_skip_model_luma;
    double trskip_bits = CTX_ENTROPY_FBITS(ctx, 1) - CTX_ENTROPY_FBITS(ctx, 0);

    if (state->encoder_control->chroma_format != KVZ_CSP_400) {
      ctx = &state->cabac.ctx.transform_skip_model_chroma;
      trskip_bits += 2.0 * (CTX_ENTROPY_FBITS(ctx, 1) - CTX_ENTROPY_FBITS(ctx, 0));
    }

    unsigned unsigned_sad_costs[PARALLEL_BLKS] = { 0 };
    double sad_costs[PARALLEL_BLKS] = { 0 };
    sad_twin_func(preds, orig_block, PARALLEL_BLKS, unsigned_sad_costs);
    for (int i = 0; i < PARALLEL_BLKS; ++i) {
      sad_costs[i] = TRSKIP_RATIO * (double)unsigned_sad_costs[i] + state->lambda_sqrt * trskip_bits;
      if (sad_costs[i] < (double)satd_costs[i]) {
        costs_out[i] = sad_costs[i];
      }
    }
  }

  #undef PARALLEL_BLKS
}

/**
* \brief Perform search for best intra transform split configuration.
*
* This function does a recursive search for the best intra transform split
* configuration for a given intra prediction mode.
*
* \return RD cost of best transform split configuration. Splits in lcu->cu.
* \param depth  Current transform depth.
* \param max_depth  Depth to which TR split will be tried.
* \param intra_mode  Intra prediction mode.
* \param cost_treshold  RD cost at which search can be stopped.
*/
static double search_intra_trdepth(encoder_state_t * const state,
                                   int x_px, int y_px, int depth, int max_depth,
                                   int intra_mode, int cost_treshold,
                                   cu_info_t *const pred_cu,
                                   lcu_t *const lcu)
{
  assert(depth >= 0 && depth <= MAX_PU_DEPTH);

  const int width = LCU_WIDTH >> depth;
  const int width_c = width > TR_MIN_WIDTH ? width / 2 : width;

  const int offset = width / 2;
  const vector2d_t lcu_px = { SUB_SCU(x_px), SUB_SCU(y_px) };
  cu_info_t *const tr_cu = LCU_GET_CU_AT_PX(lcu, lcu_px.x, lcu_px.y);

  const bool reconstruct_chroma = !(x_px & 4 || y_px & 4) && state->encoder_control->chroma_format != KVZ_CSP_400;

  struct {
    kvz_pixel y[TR_MAX_WIDTH*TR_MAX_WIDTH];
    kvz_pixel u[TR_MAX_WIDTH*TR_MAX_WIDTH];
    kvz_pixel v[TR_MAX_WIDTH*TR_MAX_WIDTH];
  } nosplit_pixels;
  uint16_t nosplit_cbf = 0;

  double split_cost = INT32_MAX;
  double nosplit_cost = INT32_MAX;

  if (depth > 0) {
    tr_cu->tr_depth = depth;
    pred_cu->tr_depth = depth;

    nosplit_cost = 0.0;

    cbf_clear(&pred_cu->cbf, depth, COLOR_Y);
    if (reconstruct_chroma) {
      cbf_clear(&pred_cu->cbf, depth, COLOR_U);
      cbf_clear(&pred_cu->cbf, depth, COLOR_V);
    }

    const int8_t chroma_mode = reconstruct_chroma ? intra_mode : -1;
    kvz_intra_recon_cu(state,
                       x_px, y_px,
                       depth,
                       intra_mode, chroma_mode,
                       pred_cu, lcu);

    nosplit_cost += kvz_cu_rd_cost_luma(state, lcu_px.x, lcu_px.y, depth, pred_cu, lcu);
    if (reconstruct_chroma) {
      nosplit_cost += kvz_cu_rd_cost_chroma(state, lcu_px.x, lcu_px.y, depth, pred_cu, lcu);
    }

    // Early stop codition for the recursive search.
    // If the cost of any 1/4th of the transform is already larger than the
    // whole transform, assume that splitting further is a bad idea.
    if (nosplit_cost >= cost_treshold) {
      return nosplit_cost;
    }

    nosplit_cbf = pred_cu->cbf;

    kvz_pixels_blit(lcu->rec.y, nosplit_pixels.y, width, width, LCU_WIDTH, width);
    if (reconstruct_chroma) {
      kvz_pixels_blit(lcu->rec.u, nosplit_pixels.u, width_c, width_c, LCU_WIDTH_C, width_c);
      kvz_pixels_blit(lcu->rec.v, nosplit_pixels.v, width_c, width_c, LCU_WIDTH_C, width_c);
    }
  }

  // Recurse further if all of the following:
  // - Current depth is less than maximum depth of the search (max_depth).
  //   - Maximum transform hierarchy depth is constrained by clipping
  //     max_depth.
  // - Min transform size hasn't been reached (MAX_PU_DEPTH).
  if (depth < max_depth && depth < MAX_PU_DEPTH) {
    split_cost = 3 * state->lambda;

    split_cost += search_intra_trdepth(state, x_px, y_px, depth + 1, max_depth, intra_mode, nosplit_cost, pred_cu, lcu);
    if (split_cost < nosplit_cost) {
      split_cost += search_intra_trdepth(state, x_px + offset, y_px, depth + 1, max_depth, intra_mode, nosplit_cost, pred_cu, lcu);
    }
    if (split_cost < nosplit_cost) {
      split_cost += search_intra_trdepth(state, x_px, y_px + offset, depth + 1, max_depth, intra_mode, nosplit_cost, pred_cu, lcu);
    }
    if (split_cost < nosplit_cost) {
      split_cost += search_intra_trdepth(state, x_px + offset, y_px + offset, depth + 1, max_depth, intra_mode, nosplit_cost, pred_cu, lcu);
    }

    double tr_split_bit = 0.0;
    double cbf_bits = 0.0;

    // Add bits for split_transform_flag = 1, because transform depth search bypasses
    // the normal recursion in the cost functions.
    if (depth >= 1 && depth <= 3) {
      const cabac_ctx_t *ctx = &(state->cabac.ctx.trans_subdiv_model[5 - (6 - depth)]);
      tr_split_bit += CTX_ENTROPY_FBITS(ctx, 1);
    }

    // Add cost of cbf chroma bits on transform tree.
    // All cbf bits are accumulated to pred_cu.cbf and cbf_is_set returns true
    // if cbf is set at any level >= depth, so cbf chroma is assumed to be 0
    // if this and any previous transform block has no chroma coefficients.
    // When searching the first block we don't actually know the real values,
    // so this will code cbf as 0 and not code the cbf at all for descendants.
    if (state->encoder_control->chroma_format != KVZ_CSP_400) {
      const uint8_t tr_depth = depth - pred_cu->depth;

      const cabac_ctx_t *ctx = &(state->cabac.ctx.qt_cbf_model_chroma[tr_depth]);
      if (tr_depth == 0 || cbf_is_set(pred_cu->cbf, depth - 1, COLOR_U)) {
        cbf_bits += CTX_ENTROPY_FBITS(ctx, cbf_is_set(pred_cu->cbf, depth, COLOR_U));
      }
      if (tr_depth == 0 || cbf_is_set(pred_cu->cbf, depth - 1, COLOR_V)) {
        cbf_bits += CTX_ENTROPY_FBITS(ctx, cbf_is_set(pred_cu->cbf, depth, COLOR_V));
      }
    }

    double bits = tr_split_bit + cbf_bits;
    split_cost += bits * state->lambda;
  } else {
    assert(width <= TR_MAX_WIDTH);
  }

  if (depth == 0 || split_cost < nosplit_cost) {
    return split_cost;
  } else {
    kvz_lcu_set_trdepth(lcu, x_px, y_px, depth, depth);

    pred_cu->cbf = nosplit_cbf;

    // We only restore the pixel data and not coefficients or cbf data.
    // The only thing we really need are the border pixels.kvz_intra_get_dir_luma_predictor
    kvz_pixels_blit(nosplit_pixels.y, lcu->rec.y, width, width, width, LCU_WIDTH);
    if (reconstruct_chroma) {
      kvz_pixels_blit(nosplit_pixels.u, lcu->rec.u, width_c, width_c, width_c, LCU_WIDTH_C);
      kvz_pixels_blit(nosplit_pixels.v, lcu->rec.v, width_c, width_c, width_c, LCU_WIDTH_C);
    }

    return nosplit_cost;
  }
}


static void search_intra_chroma_rough(encoder_state_t * const state,
                                      int x_px, int y_px, int depth,
                                      const kvz_pixel *orig_u, const kvz_pixel *orig_v, int16_t origstride,
                                      kvz_intra_references *refs_u, kvz_intra_references *refs_v,
                                      int8_t luma_mode,
                                      int8_t modes[5], double costs[5])
{
  assert(!(x_px & 4 || y_px & 4));

  const unsigned width = MAX(LCU_WIDTH_C >> depth, TR_MIN_WIDTH);
  const int_fast8_t log2_width_c = MAX(LOG2_LCU_WIDTH - (depth + 1), 2);

  for (int i = 0; i < 5; ++i) {
    costs[i] = 0;
  }

  cost_pixel_nxn_func *const satd_func = kvz_pixels_get_satd_func(width);
  //cost_pixel_nxn_func *const sad_func = kvz_pixels_get_sad_func(width);

  kvz_pixel _pred[32 * 32 + SIMD_ALIGNMENT];
  kvz_pixel *pred = ALIGNED_POINTER(_pred, SIMD_ALIGNMENT);

  kvz_pixel _orig_block[32 * 32 + SIMD_ALIGNMENT];
  kvz_pixel *orig_block = ALIGNED_POINTER(_orig_block, SIMD_ALIGNMENT);

  kvz_pixels_blit(orig_u, orig_block, width, width, origstride, width);
  for (int i = 0; i < 5; ++i) {
    if (modes[i] == luma_mode) continue;
    kvz_intra_predict(refs_u, log2_width_c, modes[i], COLOR_U, pred, false);
    //costs[i] += get_cost(encoder_state, pred, orig_block, satd_func, sad_func, width);
    costs[i] += satd_func(pred, orig_block);
  }

  kvz_pixels_blit(orig_v, orig_block, width, width, origstride, width);
  for (int i = 0; i < 5; ++i) {
    if (modes[i] == luma_mode) continue;
    kvz_intra_predict(refs_v, log2_width_c, modes[i], COLOR_V, pred, false);
    //costs[i] += get_cost(encoder_state, pred, orig_block, satd_func, sad_func, width);
    costs[i] += satd_func(pred, orig_block);
  }

  sort_modes(modes, costs, 5);
}


/**
 * \brief  Order the intra prediction modes according to a fast criteria. 
 *
 * This function uses SATD to order the intra prediction modes. For 4x4 modes
 * SAD might be used instead, if the cost given by SAD is much better than the
 * one given by SATD, to take into account that 4x4 modes can be coded with
 * transform skip. This version of the function calculates two costs
 * simultaneously to better utilize large SIMD registers with AVX and newer
 * extensions.
 *
 * The modes are searched using halving search and the total number of modes
 * that are tried is dependent on size of the predicted block. More modes
 * are tried for smaller blocks.
 *
 * \param orig  Pointer to the top-left corner of current CU in the picture
 *     being encoded.
 * \param orig_stride  Stride of param orig..
 * \param rec  Pointer to the top-left corner of current CU in the picture
 *     being encoded.
 * \param rec_stride  Stride of param rec.
 * \param width  Width of the prediction block.
 * \param intra_preds  Array of the 3 predicted intra modes.
 *
 * \param[out] modes  The modes ordered according to their RD costs, from best
 *     to worst. The number of modes and costs output is given by parameter
 *     modes_to_check.
 * \param[out] costs  The RD costs of corresponding modes in param modes.
 *
 * \return  Number of prediction modes in param modes.
 */
static int8_t search_intra_rough(encoder_state_t * const state, 
                                 kvz_pixel *orig, int32_t origstride,
                                 kvz_intra_references *refs,
                                 int log2_width, int8_t *intra_preds,
                                 int8_t modes[35], double costs[35])
{
  #define PARALLEL_BLKS 2 // TODO: use 4 for AVX-512 in the future?
  assert(log2_width >= 2 && log2_width <= 5);
  int_fast8_t width = 1 << log2_width;
  cost_pixel_nxn_func *satd_func = kvz_pixels_get_satd_func(width);
  cost_pixel_nxn_func *sad_func = kvz_pixels_get_sad_func(width);
  cost_pixel_nxn_multi_func *satd_dual_func = kvz_pixels_get_satd_dual_func(width);
  cost_pixel_nxn_multi_func *sad_dual_func = kvz_pixels_get_sad_dual_func(width);

  const kvz_config *cfg = &state->encoder_control->cfg;
  const bool filter_boundary = !(cfg->lossless && cfg->implicit_rdpcm);

  // Temporary block arrays
  kvz_pixel _preds[PARALLEL_BLKS * 32 * 32 + SIMD_ALIGNMENT];
  pred_buffer preds = ALIGNED_POINTER(_preds, SIMD_ALIGNMENT);
  
  kvz_pixel _orig_block[32 * 32 + SIMD_ALIGNMENT];
  kvz_pixel *orig_block = ALIGNED_POINTER(_orig_block, SIMD_ALIGNMENT);

  // Store original block for SAD computation
  kvz_pixels_blit(orig, orig_block, width, width, origstride, width);

  int8_t modes_selected = 0;
  unsigned min_cost = UINT_MAX;
  unsigned max_cost = 0;
  
  // Initial offset decides how many modes are tried before moving on to the
  // recursive search.
  int offset;
  if (state->encoder_control->cfg.full_intra_search) {
    offset = 1;
  } else {
    static const int8_t offsets[4] = { 2, 4, 8, 8 };
    offset = offsets[log2_width - 2];
  }

  // Calculate SAD for evenly spaced modes to select the starting point for 
  // the recursive search.
  for (int mode = 2; mode <= 34; mode += PARALLEL_BLKS * offset) {
    
    double costs_out[PARALLEL_BLKS] = { 0 };
    for (int i = 0; i < PARALLEL_BLKS; ++i) {
      if (mode + i * offset <= 34) {
        kvz_intra_predict(refs, log2_width, mode + i * offset, COLOR_Y, preds[i], filter_boundary);
      }
    }
    
    //TODO: add generic version of get cost  multi
    get_cost_dual(state, preds, orig_block, satd_dual_func, sad_dual_func, width, costs_out);

    for (int i = 0; i < PARALLEL_BLKS; ++i) {
      if (mode + i * offset <= 34) {
        costs[modes_selected] = costs_out[i];
        modes[modes_selected] = mode + i * offset;
        min_cost = MIN(min_cost, costs[modes_selected]);
        max_cost = MAX(max_cost, costs[modes_selected]);
        ++modes_selected;
      }
    }
  }

  int8_t best_mode = modes[select_best_mode_index(modes, costs, modes_selected)];
  double best_cost = min_cost;
  
  // Skip recursive search if all modes have the same cost.
  if (min_cost != max_cost) {
    // Do a recursive search to find the best mode, always centering on the
    // current best mode.
    while (offset > 1) {
      offset >>= 1;

      int8_t center_node = best_mode;
      int8_t test_modes[] = { center_node - offset, center_node + offset };

      double costs_out[PARALLEL_BLKS] = { 0 };
      char mode_in_range = 0;

      for (int i = 0; i < PARALLEL_BLKS; ++i) mode_in_range |= (test_modes[i] >= 2 && test_modes[i] <= 34);

      if (mode_in_range) {
        for (int i = 0; i < PARALLEL_BLKS; ++i) {
          if (test_modes[i] >= 2 && test_modes[i] <= 34) {
            kvz_intra_predict(refs, log2_width, test_modes[i], COLOR_Y, preds[i], filter_boundary);
          }
        }

        //TODO: add generic version of get cost multi
        get_cost_dual(state, preds, orig_block, satd_dual_func, sad_dual_func, width, costs_out);

        for (int i = 0; i < PARALLEL_BLKS; ++i) {
          if (test_modes[i] >= 2 && test_modes[i] <= 34) {
            costs[modes_selected] = costs_out[i];
            modes[modes_selected] = test_modes[i];
            if (costs[modes_selected] < best_cost) {
              best_cost = costs[modes_selected];
              best_mode = modes[modes_selected];
            }
            ++modes_selected;
          }
        }
      }
    }
  }

  int8_t add_modes[5] = {intra_preds[0], intra_preds[1], intra_preds[2], 0, 1};

  // Add DC, planar and missing predicted modes.
  for (int8_t pred_i = 0; pred_i < 5; ++pred_i) {
    bool has_mode = false;
    int8_t mode = add_modes[pred_i];

    for (int mode_i = 0; mode_i < modes_selected; ++mode_i) {
      if (modes[mode_i] == add_modes[pred_i]) {
        has_mode = true;
        break;
      }
    }

    if (!has_mode) {
      kvz_intra_predict(refs, log2_width, mode, COLOR_Y, preds[0], filter_boundary);
      costs[modes_selected] = get_cost(state, preds[0], orig_block, satd_func, sad_func, width);
      modes[modes_selected] = mode;
      ++modes_selected;
    }
  }

  // Add prediction mode coding cost as the last thing. We don't want this
  // affecting the halving search.
  int lambda_cost = (int)(state->lambda_sqrt + 0.5);
  for (int mode_i = 0; mode_i < modes_selected; ++mode_i) {
    costs[mode_i] += lambda_cost * kvz_luma_mode_bits(state, modes[mode_i], intra_preds);
  }

  #undef PARALLEL_BLKS

  return modes_selected;
}

/**
 * \brief  Find best intra mode out of the ones listed in parameter modes.
 *
 * This function perform intra search by doing full quantization,
 * reconstruction and CABAC coding of coefficients. It is very slow
 * but results in better RD quality than using just the rough search.
 *
 * \param x_px  Luma picture coordinate.
 * \param y_px  Luma picture coordinate.
 * \param orig  Pointer to the top-left corner of current CU in the picture
 *     being encoded.
 * \param orig_stride  Stride of param orig.
 * \param rec  Pointer to the top-left corner of current CU in the picture
 *     being encoded.
 * \param rec_stride  Stride of param rec.
 * \param intra_preds  Array of the 3 predicted intra modes.
 * \param modes_to_check  How many of the modes in param modes are checked.
 * \param[in] modes  The intra prediction modes that are to be checked.
 * 
 * \param[out] modes  The modes ordered according to their RD costs, from best
 *     to worst. The number of modes and costs output is given by parameter
 *     modes_to_check.
 * \param[out] costs  The RD costs of corresponding modes in param modes.
 * \param[out] lcu  If transform split searching is used, the transform split
 *     information for the best mode is saved in lcu.cu structure.
 */
static int8_t search_intra_rdo(encoder_state_t * const state, 
                             int x_px, int y_px, int depth,
                             kvz_pixel *orig, int32_t origstride,
                             int8_t *intra_preds,
                             int modes_to_check,
                             int8_t modes[35], double costs[35],
                             lcu_t *lcu)
{
  const int tr_depth = CLIP(1, MAX_PU_DEPTH, depth + state->encoder_control->cfg.tr_depth_intra);
  const int width = LCU_WIDTH >> depth;

  kvz_pixel orig_block[LCU_WIDTH * LCU_WIDTH + 1];

  kvz_pixels_blit(orig, orig_block, width, width, origstride, width);

  // Check that the predicted modes are in the RDO mode list
  if (modes_to_check < 35) {
    for (int pred_mode = 0; pred_mode < 3; pred_mode++) {
      int mode_found = 0;
      for (int rdo_mode = 0; rdo_mode < modes_to_check; rdo_mode++) {
        if (intra_preds[pred_mode] == modes[rdo_mode]) {
          mode_found = 1;
          break;
        }
      }
      // Add this prediction mode to RDO checking
      if (!mode_found) {
        modes[modes_to_check] = intra_preds[pred_mode];
        modes_to_check++;
      }
    }
  }

  for(int rdo_mode = 0; rdo_mode < modes_to_check; rdo_mode ++) {
    int rdo_bitcost = kvz_luma_mode_bits(state, modes[rdo_mode], intra_preds);
    costs[rdo_mode] = rdo_bitcost * (int)(state->lambda + 0.5);

    // Perform transform split search and save mode RD cost for the best one.
    cu_info_t pred_cu;
    pred_cu.depth = depth;
    pred_cu.type = CU_INTRA;
    pred_cu.part_size = ((depth == MAX_PU_DEPTH) ? SIZE_NxN : SIZE_2Nx2N);
    pred_cu.intra.mode = modes[rdo_mode];
    pred_cu.intra.mode_chroma = modes[rdo_mode];
    FILL(pred_cu.cbf, 0);

    // Reset transform split data in lcu.cu for this area.
    kvz_lcu_set_trdepth(lcu, x_px, y_px, depth, depth);

    double mode_cost = search_intra_trdepth(state, x_px, y_px, depth, tr_depth, modes[rdo_mode], MAX_INT, &pred_cu, lcu);
    costs[rdo_mode] += mode_cost;
  }

  // The best transform split hierarchy is not saved anywhere, so to get the
  // transform split hierarchy the search has to be performed again with the
  // best mode.
  if (tr_depth != depth) {
    cu_info_t pred_cu;
    pred_cu.depth = depth;
    pred_cu.type = CU_INTRA;
    pred_cu.part_size = ((depth == MAX_PU_DEPTH) ? SIZE_NxN : SIZE_2Nx2N);
    pred_cu.intra.mode = modes[0];
    pred_cu.intra.mode_chroma = modes[0];
    FILL(pred_cu.cbf, 0);
    search_intra_trdepth(state, x_px, y_px, depth, tr_depth, modes[0], MAX_INT, &pred_cu, lcu);
  }

  return modes_to_check;
}


double kvz_luma_mode_bits(const encoder_state_t *state, int8_t luma_mode, const int8_t *intra_preds)
{
  double mode_bits;

  bool mode_in_preds = false;
  for (int i = 0; i < 3; ++i) {
    if (luma_mode == intra_preds[i]) {
      mode_in_preds = true;
    }
  }

  const cabac_ctx_t *ctx = &(state->cabac.ctx.intra_mode_model);
  mode_bits = CTX_ENTROPY_FBITS(ctx, mode_in_preds);

  if (mode_in_preds) {
    mode_bits += ((luma_mode == intra_preds[0]) ? 1 : 2);
  } else {
    mode_bits += 5;
  }

  return mode_bits;
}


double kvz_chroma_mode_bits(const encoder_state_t *state, int8_t chroma_mode, int8_t luma_mode)
{
  const cabac_ctx_t *ctx = &(state->cabac.ctx.chroma_pred_model[0]);
  double mode_bits;
  if (chroma_mode == luma_mode) {
    mode_bits = CTX_ENTROPY_FBITS(ctx, 0);
  } else {
    mode_bits = 2.0 + CTX_ENTROPY_FBITS(ctx, 1);
  }

  return mode_bits;
}


int8_t kvz_search_intra_chroma_rdo(encoder_state_t * const state,
                                  int x_px, int y_px, int depth,
                                  int8_t intra_mode,
                                  int8_t modes[5], int8_t num_modes,
                                  lcu_t *const lcu)
{
  const bool reconstruct_chroma = !(x_px & 4 || y_px & 4);

  if (reconstruct_chroma) {
    const vector2d_t lcu_px = { SUB_SCU(x_px), SUB_SCU(y_px) };
    cu_info_t *const tr_cu = LCU_GET_CU_AT_PX(lcu, lcu_px.x, lcu_px.y);

    struct {
      double cost;
      int8_t mode;
    } chroma, best_chroma;

    best_chroma.mode = 0;
    best_chroma.cost = MAX_INT;

    for (int8_t chroma_mode_i = 0; chroma_mode_i < num_modes; ++chroma_mode_i) {
      chroma.mode = modes[chroma_mode_i];

      kvz_intra_recon_cu(state,
                         x_px, y_px,
                         depth,
                         -1, chroma.mode, // skip luma
                         NULL, lcu);
      chroma.cost = kvz_cu_rd_cost_chroma(state, lcu_px.x, lcu_px.y, depth, tr_cu, lcu);

      double mode_bits = kvz_chroma_mode_bits(state, chroma.mode, intra_mode);
      chroma.cost += mode_bits * state->lambda;

      if (chroma.cost < best_chroma.cost) {
        best_chroma = chroma;
      }
    }

    return best_chroma.mode;
  }

  return 100;
}


int8_t kvz_search_cu_intra_chroma(encoder_state_t * const state,
                              const int x_px, const int y_px,
                              const int depth, lcu_t *lcu)
{
  const vector2d_t lcu_px = { SUB_SCU(x_px), SUB_SCU(y_px) };

  cu_info_t *cur_pu = LCU_GET_CU_AT_PX(lcu, lcu_px.x, lcu_px.y);
  int8_t intra_mode = cur_pu->intra.mode;

  double costs[5];
  int8_t modes[5] = { 0, 26, 10, 1, 34 };
  if (intra_mode != 0 && intra_mode != 26 && intra_mode != 10 && intra_mode != 1) {
    modes[4] = intra_mode;
  }

  // The number of modes to select for slower chroma search. Luma mode
  // is always one of the modes, so 2 means the final decision is made
  // between luma mode and one other mode that looks the best
  // according to search_intra_chroma_rough.
  const int8_t modes_in_depth[5] = { 1, 1, 1, 1, 2 };
  int num_modes = modes_in_depth[depth];

  if (state->encoder_control->cfg.rdo == 3) {
    num_modes = 5;
  }

  // Don't do rough mode search if all modes are selected.
  // FIXME: It might make more sense to only disable rough search if
  // num_modes is 0.is 0.
  if (num_modes != 1 && num_modes != 5) {
    const int_fast8_t log2_width_c = MAX(LOG2_LCU_WIDTH - depth - 1, 2);
    const vector2d_t pic_px = { state->tile->frame->width, state->tile->frame->height };
    const vector2d_t luma_px = { x_px, y_px };

    kvz_intra_references refs_u;
    kvz_intra_build_reference(log2_width_c, COLOR_U, &luma_px, &pic_px, lcu, &refs_u);

    kvz_intra_references refs_v;
    kvz_intra_build_reference(log2_width_c, COLOR_V, &luma_px, &pic_px, lcu, &refs_v);

    vector2d_t lcu_cpx = { lcu_px.x / 2, lcu_px.y / 2 };
    kvz_pixel *ref_u = &lcu->ref.u[lcu_cpx.x + lcu_cpx.y * LCU_WIDTH_C];
    kvz_pixel *ref_v = &lcu->ref.v[lcu_cpx.x + lcu_cpx.y * LCU_WIDTH_C];

    search_intra_chroma_rough(state, x_px, y_px, depth,
                              ref_u, ref_v, LCU_WIDTH_C,
                              &refs_u, &refs_v,
                              intra_mode, modes, costs);
  }

  int8_t intra_mode_chroma = intra_mode;
  if (num_modes > 1) {
    intra_mode_chroma = kvz_search_intra_chroma_rdo(state, x_px, y_px, depth, intra_mode, modes, num_modes, lcu);
  }

  return intra_mode_chroma;
}


/**
 * Update lcu to have best modes at this depth.
 * \return Cost of best mode.
 */
void kvz_search_cu_intra(encoder_state_t * const state,
                         const int x_px, const int y_px,
                         const int depth, lcu_t *lcu,
                         int8_t *mode_out, double *cost_out)
{
  const vector2d_t lcu_px = { SUB_SCU(x_px), SUB_SCU(y_px) };
  const int8_t cu_width = LCU_WIDTH >> depth;
  const int_fast8_t log2_width = LOG2_LCU_WIDTH - depth;

  cu_info_t *cur_cu = LCU_GET_CU_AT_PX(lcu, lcu_px.x, lcu_px.y);

  kvz_intra_references refs;

  int8_t candidate_modes[3];

  cu_info_t *left_cu = 0;
  cu_info_t *above_cu = 0;

  // Select left and top CUs if they are available.
  // Top CU is not available across LCU boundary.
  if (x_px >= SCU_WIDTH) {
    left_cu = LCU_GET_CU_AT_PX(lcu, lcu_px.x - 1, lcu_px.y);
  }
  if (y_px >= SCU_WIDTH && lcu_px.y > 0) {
    above_cu = LCU_GET_CU_AT_PX(lcu, lcu_px.x, lcu_px.y - 1);
  }
  kvz_intra_get_dir_luma_predictor(x_px, y_px, candidate_modes, cur_cu, left_cu, above_cu);

  if (depth > 0) {
    const vector2d_t luma_px = { x_px, y_px };
    const vector2d_t pic_px = { state->tile->frame->width, state->tile->frame->height };
    kvz_intra_build_reference(log2_width, COLOR_Y, &luma_px, &pic_px, lcu, &refs);
  }

  int8_t modes[35];
  double costs[35];

  // Find best intra mode for 2Nx2N.
  kvz_pixel *ref_pixels = &lcu->ref.y[lcu_px.x + lcu_px.y * LCU_WIDTH];

  int8_t number_of_modes;
  bool skip_rough_search = (depth == 0 || state->encoder_control->cfg.rdo >= 3);
  if (!skip_rough_search) {
    number_of_modes = search_intra_rough(state,
                                         ref_pixels, LCU_WIDTH,
                                         &refs,
                                         log2_width, candidate_modes,
                                         modes, costs);
  } else {
    number_of_modes = 35;
    for (int i = 0; i < number_of_modes; ++i) {
      modes[i] = i;
      costs[i] = MAX_INT;
    }
  }

  // Set transform depth to current depth, meaning no transform splits.
  kvz_lcu_set_trdepth(lcu, x_px, y_px, depth, depth);
  double best_rough_cost = costs[select_best_mode_index(modes, costs, number_of_modes)];
  // Refine results with slower search or get some results if rough search was skipped.
  const int32_t rdo_level = state->encoder_control->cfg.rdo;
  if (rdo_level >= 2 || skip_rough_search) {
    int number_of_modes_to_search;
    if (rdo_level == 3) {
      number_of_modes_to_search = 35;
    } else if (rdo_level == 2) {
      number_of_modes_to_search = (cu_width == 4) ? 3 : 2;
    } else {
      // Check only the predicted modes.
      number_of_modes_to_search = 0;
    }
    int num_modes_to_check = MIN(number_of_modes, number_of_modes_to_search);

    sort_modes(modes, costs, number_of_modes);
    number_of_modes = search_intra_rdo(state,
                      x_px, y_px, depth,
                      ref_pixels, LCU_WIDTH,
                      candidate_modes,
                      num_modes_to_check,
                      modes, costs, lcu);
  }

  uint8_t best_mode_i = select_best_mode_index(modes, costs, number_of_modes);

  *mode_out = modes[best_mode_i];
  *cost_out = skip_rough_search ? costs[best_mode_i]:best_rough_cost;
}
