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

#include "search_inter.h"

#include <limits.h>
#include <stdlib.h>

#include "cabac.h"
#include "encoder.h"
#include "image.h"
#include "imagelist.h"
#include "inter.h"
#include "kvazaar.h"
#include "rdo.h"
#include "search.h"
#include "strategies/strategies-ipol.h"
#include "strategies/strategies-picture.h"
#include "transform.h"
#include "videoframe.h"

typedef struct {
  encoder_state_t *state;

  /**
   * \brief Current frame
   */
  const kvz_picture *pic;
  /**
   * \brief Reference frame
   */
  const kvz_picture *ref;

  /**
   * \brief Index of the reference frame
   */
  int32_t ref_idx;

  /**
   * \brief Top-left corner of the PU
   */
  const vector2d_t origin;
  int32_t width;
  int32_t height;

  int16_t mv_cand[2][2];
  inter_merge_cand_t merge_cand[MRG_MAX_NUM_CANDS];
  int32_t num_merge_cand;

  kvz_mvd_cost_func *mvd_cost_func;

  /**
   * \brief Best motion vector among the ones tested so far
   */
  vector2d_t best_mv;
  /**
   * \brief Cost of best_mv
   */
  uint32_t best_cost;
  /**
   * \brief Bit cost of best_mv
   */
  uint32_t best_bitcost;

  /**
   * \brief Possible optimized SAD implementation for the width, leave as
   *        NULL for arbitrary-width blocks
   */
  optimized_sad_func_ptr_t optimized_sad;

} inter_search_info_t;


/**
 * \return  True if referred block is within current tile.
 */
static INLINE bool fracmv_within_tile(const inter_search_info_t *info, int x, int y)
{
  const encoder_control_t *ctrl = info->state->encoder_control;

  const bool is_frac_luma   = x % 4 != 0 || y % 4 != 0;
  const bool is_frac_chroma = x % 8 != 0 || y % 8 != 0;

  if (ctrl->cfg.owf && ctrl->cfg.wpp) {
    // Check that the block does not reference pixels that are not final.

    // Margin as luma pixels.
    int margin = 0;
    if (is_frac_luma) {
      // Fractional motion estimation needs up to 4 pixels outside the
      // block.
      margin = 4;
    } else if (is_frac_chroma) {
      // Odd chroma interpolation needs up to 2 luma pixels outside the
      // block.
      margin = 2;
    }

    if (ctrl->cfg.sao_type) {
      // Make sure we don't refer to pixels for which SAO reconstruction
      // has not been done.
      margin += SAO_DELAY_PX;
    } else if (ctrl->cfg.deblock_enable) {
      // Make sure we don't refer to pixels that have not been deblocked.
      margin += DEBLOCK_DELAY_PX;
    }

    // Coordinates of the top-left corner of the containing LCU.
    const vector2d_t orig_lcu = {
      .x = info->origin.x / LCU_WIDTH,
      .y = info->origin.y / LCU_WIDTH,
    };
    // Difference between the coordinates of the LCU containing the
    // bottom-left corner of the referenced block and the LCU containing
    // this block.
    const vector2d_t mv_lcu = {
      ((info->origin.x + info->width  + margin) * 4 + x) / (LCU_WIDTH << 2) - orig_lcu.x,
      ((info->origin.y + info->height + margin) * 4 + y) / (LCU_WIDTH << 2) - orig_lcu.y,
    };

    if (mv_lcu.y > ctrl->max_inter_ref_lcu.down) {
      return false;
    }

    if (mv_lcu.x + mv_lcu.y >
        ctrl->max_inter_ref_lcu.down + ctrl->max_inter_ref_lcu.right)
    {
      return false;
    }
  }

  if (ctrl->cfg.mv_constraint == KVZ_MV_CONSTRAIN_NONE) {
    return true;
  }

  // Margin as luma quater pixels.
  int margin = 0;
  if (ctrl->cfg.mv_constraint == KVZ_MV_CONSTRAIN_FRAME_AND_TILE_MARGIN) {
    if (is_frac_luma) {
      margin = 4 << 2;
    } else if (is_frac_chroma) {
      margin = 2 << 2;
    }
  }

  // TODO implement KVZ_MV_CONSTRAIN_FRAM and KVZ_MV_CONSTRAIN_TILE.
  const vector2d_t abs_mv = {
    info->origin.x * 4 + x,
    info->origin.y * 4 + y,
  };

  // Check that both margin constraints are satisfied.
  const int from_right  =
    (info->state->tile->frame->width  << 2) - (abs_mv.x + (info->width  << 2));
  const int from_bottom =
    (info->state->tile->frame->height << 2) - (abs_mv.y + (info->height << 2));

  return abs_mv.x >= margin &&
         abs_mv.y >= margin &&
         from_right >= margin &&
         from_bottom >= margin;
}


/**
 * \return  True if referred block is within current tile.
 */
static INLINE bool intmv_within_tile(const inter_search_info_t *info, int x, int y)
{
  return fracmv_within_tile(info, x * 4, y * 4);
}


/**
 * \brief Calculate cost for an integer motion vector.
 *
 * Updates info->best_mv, info->best_cost and info->best_bitcost to the new
 * motion vector if it yields a lower cost than the current one.
 *
 * If the motion vector violates the MV constraints for tiles or WPP, the
 * cost is not set.
 *
 * \return true if info->best_mv was changed, false otherwise
 */
static bool check_mv_cost(inter_search_info_t *info, int x, int y)
{
  if (!intmv_within_tile(info, x, y)) return false;

  uint32_t bitcost = 0;
  uint32_t cost = kvz_image_calc_sad(
      info->pic,
      info->ref,
      info->origin.x,
      info->origin.y,
      info->state->tile->offset_x + info->origin.x + x,
      info->state->tile->offset_y + info->origin.y + y,
      info->width,
      info->height,
      info->optimized_sad
  );

  if (cost >= info->best_cost) return false;

  cost += info->mvd_cost_func(
      info->state,
      x, y, 2,
      info->mv_cand,
      info->merge_cand,
      info->num_merge_cand,
      info->ref_idx,
      &bitcost
  );

  if (cost >= info->best_cost) return false;

  // Set to motion vector in quarter pixel precision.
  info->best_mv.x = x * 4;
  info->best_mv.y = y * 4;
  info->best_cost = cost;
  info->best_bitcost = bitcost;

  return true;
}


static unsigned get_ep_ex_golomb_bitcost(unsigned symbol)
{
  // Calculate 2 * log2(symbol + 2)

  unsigned bins = 0;
  symbol += 2;
  if (symbol >= 1 << 8) { bins += 16; symbol >>= 8; }
  if (symbol >= 1 << 4) { bins += 8; symbol >>= 4; }
  if (symbol >= 1 << 2) { bins += 4; symbol >>= 2; }
  if (symbol >= 1 << 1) { bins += 2; }

  // TODO: It might be a good idea to put a small slope on this function to
  // make sure any search function that follows the gradient heads towards
  // a smaller MVD, but that would require fractinal costs and bits being
  // used everywhere in inter search.
  // return num_bins + 0.001 * symbol;

  return bins;
}


/**
 * \brief Checks if mv is one of the merge candidates.
 * \return true if found else return false
 */
static bool mv_in_merge(const inter_search_info_t *info, vector2d_t mv)
{
  for (int i = 0; i < info->num_merge_cand; ++i) {
    if (info->merge_cand[i].dir == 3) continue;
    const vector2d_t merge_mv = {
      (info->merge_cand[i].mv[info->merge_cand[i].dir - 1][0] + 2) >> 2,
      (info->merge_cand[i].mv[info->merge_cand[i].dir - 1][1] + 2) >> 2
    };
    if (merge_mv.x == mv.x && merge_mv.y == mv.y) {
      return true;
    }
  }
  return false;
}


/**
 * \brief Select starting point for integer motion estimation search.
 *
 * Checks the zero vector, extra_mv and merge candidates and updates
 * info->best_mv to the best one.
 */
static void select_starting_point(inter_search_info_t *info, vector2d_t extra_mv)
{
  // Check the 0-vector, so we can ignore all 0-vectors in the merge cand list.
  check_mv_cost(info, 0, 0);

  // Change to integer precision.
  extra_mv.x >>= 2;
  extra_mv.y >>= 2;

  // Check mv_in if it's not one of the merge candidates.
  if ((extra_mv.x != 0 || extra_mv.y != 0) && !mv_in_merge(info, extra_mv)) {
    check_mv_cost(info, extra_mv.x, extra_mv.y);
  }

  // Go through candidates
  for (unsigned i = 0; i < info->num_merge_cand; ++i) {
    if (info->merge_cand[i].dir == 3) continue;

    int x = (info->merge_cand[i].mv[info->merge_cand[i].dir - 1][0] + 2) >> 2;
    int y = (info->merge_cand[i].mv[info->merge_cand[i].dir - 1][1] + 2) >> 2;

    if (x == 0 && y == 0) continue;

    check_mv_cost(info, x, y);
  }
}


static uint32_t get_mvd_coding_cost(const encoder_state_t *state,
                                    const cabac_data_t* cabac,
                                    const int32_t mvd_hor,
                                    const int32_t mvd_ver)
{
  unsigned bitcost = 0;
  const vector2d_t abs_mvd = { abs(mvd_hor), abs(mvd_ver) };

  bitcost += get_ep_ex_golomb_bitcost(abs_mvd.x) << CTX_FRAC_BITS;
  bitcost += get_ep_ex_golomb_bitcost(abs_mvd.y) << CTX_FRAC_BITS;

  // Round and shift back to integer bits.
  return (bitcost + CTX_FRAC_HALF_BIT) >> CTX_FRAC_BITS;
}


static int select_mv_cand(const encoder_state_t *state,
                          int16_t mv_cand[2][2],
                          int32_t mv_x,
                          int32_t mv_y,
                          uint32_t *cost_out)
{
  const bool same_cand =
    (mv_cand[0][0] == mv_cand[1][0] && mv_cand[0][1] == mv_cand[1][1]);

  if (same_cand && !cost_out) {
    // Pick the first one if both candidates are the same.
    return 0;
  }

  uint32_t (*mvd_coding_cost)(const encoder_state_t * const state,
                              const cabac_data_t*,
                              int32_t, int32_t);
  if (state->encoder_control->cfg.mv_rdo) {
    mvd_coding_cost = kvz_get_mvd_coding_cost_cabac;
  } else {
    mvd_coding_cost = get_mvd_coding_cost;
  }

  uint32_t cand1_cost = mvd_coding_cost(
      state, &state->cabac,
      mv_x - mv_cand[0][0],
      mv_y - mv_cand[0][1]);

  uint32_t cand2_cost;
  if (same_cand) {
    cand2_cost = cand1_cost;
  } else {
    cand2_cost = mvd_coding_cost(
      state, &state->cabac,
      mv_x - mv_cand[1][0],
      mv_y - mv_cand[1][1]);
  }

  if (cost_out) {
    *cost_out = MIN(cand1_cost, cand2_cost);
  }

  // Pick the second candidate if it has lower cost.
  return cand2_cost < cand1_cost ? 1 : 0;
}


static uint32_t calc_mvd_cost(const encoder_state_t *state,
                              int x,
                              int y,
                              int mv_shift,
                              int16_t mv_cand[2][2],
                              inter_merge_cand_t merge_cand[MRG_MAX_NUM_CANDS],
                              int16_t num_cand,
                              int32_t ref_idx,
                              uint32_t *bitcost)
{
  uint32_t temp_bitcost = 0;
  uint32_t merge_idx;
  int8_t merged      = 0;

  x *= 1 << mv_shift;
  y *= 1 << mv_shift;

  // Check every candidate to find a match
  for(merge_idx = 0; merge_idx < (uint32_t)num_cand; merge_idx++) {
    if (merge_cand[merge_idx].dir == 3) continue;
    if (merge_cand[merge_idx].mv[merge_cand[merge_idx].dir - 1][0] == x &&
        merge_cand[merge_idx].mv[merge_cand[merge_idx].dir - 1][1] == y &&
        state->frame->ref_LX[merge_cand[merge_idx].dir - 1][
          merge_cand[merge_idx].ref[merge_cand[merge_idx].dir - 1]
        ] == ref_idx) {
      temp_bitcost += merge_idx;
      merged = 1;
      break;
    }
  }

  // Check mvd cost only if mv is not merged
  if (!merged) {
    uint32_t mvd_cost = 0;
    select_mv_cand(state, mv_cand, x, y, &mvd_cost);
    temp_bitcost += mvd_cost;
  }
  *bitcost = temp_bitcost;
  return temp_bitcost*(int32_t)(state->lambda_sqrt + 0.5);
}


static bool early_terminate(inter_search_info_t *info)
{
  static const vector2d_t small_hexbs[7] = {
      { 0, -1 }, { -1, 0 }, { 0, 1 }, { 1, 0 },
      { 0, -1 }, { -1, 0 }, { 0, 0 },
  };

  vector2d_t mv = { info->best_mv.x >> 2, info->best_mv.y >> 2 };

  int first_index = 0;
  int last_index = 3;

  for (int k = 0; k < 2; ++k) {
    double threshold;
    if (info->state->encoder_control->cfg.me_early_termination ==
        KVZ_ME_EARLY_TERMINATION_SENSITIVE)
    {
      threshold = info->best_cost * 0.95;
    } else {
      threshold = info->best_cost;
    }

    int best_index = 6;
    for (int i = first_index; i <= last_index; i++) {
      int x = mv.x + small_hexbs[i].x;
      int y = mv.y + small_hexbs[i].y;

      if (check_mv_cost(info, x, y)) {
        best_index = i;
      }
    }

    // Adjust the movement vector
    mv.x += small_hexbs[best_index].x;
    mv.y += small_hexbs[best_index].y;

    // If best match is not better than threshold, we stop the search.
    if (info->best_cost >= threshold) {
      return true;
    }

    first_index = (best_index + 3) % 4;
    last_index = first_index + 2;
  }
  return false;
}


void kvz_tz_pattern_search(inter_search_info_t *info,
                           unsigned pattern_type,
                           const int iDist,
                           vector2d_t mv,
                           int *best_dist)
{
  assert(pattern_type < 4);

  //implemented search patterns
  const vector2d_t pattern[4][8] = {
      //diamond (8 points)
      //[ ][ ][ ][ ][1][ ][ ][ ][ ]
      //[ ][ ][ ][ ][ ][ ][ ][ ][ ]
      //[ ][ ][8][ ][ ][ ][5][ ][ ]
      //[ ][ ][ ][ ][ ][ ][ ][ ][ ]
      //[4][ ][ ][ ][o][ ][ ][ ][2]
      //[ ][ ][ ][ ][ ][ ][ ][ ][ ]
      //[ ][ ][7][ ][ ][ ][6][ ][ ]
      //[ ][ ][ ][ ][ ][ ][ ][ ][ ]
      //[ ][ ][ ][ ][3][ ][ ][ ][ ]
      {
        { 0, iDist }, { iDist, 0 }, { 0, -iDist }, { -iDist, 0 },
        { iDist / 2, iDist / 2 }, { iDist / 2, -iDist / 2 }, { -iDist / 2, -iDist / 2 }, { -iDist / 2, iDist / 2 }
      },

      //square (8 points)
      //[8][ ][ ][ ][1][ ][ ][ ][2]
      //[ ][ ][ ][ ][ ][ ][ ][ ][ ]
      //[ ][ ][ ][ ][ ][ ][ ][ ][ ]
      //[ ][ ][ ][ ][ ][ ][ ][ ][ ]
      //[7][ ][ ][ ][o][ ][ ][ ][3]
      //[ ][ ][ ][ ][ ][ ][ ][ ][ ]
      //[ ][ ][ ][ ][ ][ ][ ][ ][ ]
      //[ ][ ][ ][ ][ ][ ][ ][ ][ ]
      //[6][ ][ ][ ][5][ ][ ][ ][4]
      {
        { 0, iDist }, { iDist, iDist }, { iDist, 0 }, { iDist, -iDist }, { 0, -iDist },
        { -iDist, -iDist }, { -iDist, 0 }, { -iDist, iDist }
      },

      //octagon (8 points)
      //[ ][ ][5][ ][ ][ ][1][ ][ ]
      //[ ][ ][ ][ ][ ][ ][ ][ ][ ]
      //[ ][ ][ ][ ][ ][ ][ ][ ][2]
      //[4][ ][ ][ ][ ][ ][ ][ ][ ]
      //[ ][ ][ ][ ][o][ ][ ][ ][ ]
      //[ ][ ][ ][ ][ ][ ][ ][ ][ ]
      //[8][ ][ ][ ][ ][ ][ ][ ][6]
      //[ ][ ][ ][ ][ ][ ][ ][ ][ ]
      //[ ][ ][7][ ][ ][ ][3][ ][ ]
      {
        { iDist / 2, iDist }, { iDist, iDist / 2 }, { iDist / 2, -iDist }, { -iDist, iDist / 2 },
        { -iDist / 2, iDist }, { iDist, -iDist / 2 }, { -iDist / 2, -iDist }, { -iDist, -iDist / 2 }
      },

      //hexagon (6 points)
      //[ ][ ][5][ ][ ][ ][1][ ][ ]
      //[ ][ ][ ][ ][ ][ ][ ][ ][ ]
      //[ ][ ][ ][ ][ ][ ][ ][ ][ ]
      //[ ][ ][ ][ ][ ][ ][ ][ ][ ]
      //[4][ ][ ][ ][o][ ][ ][ ][2]
      //[ ][ ][ ][ ][ ][ ][ ][ ][ ]
      //[ ][ ][ ][ ][ ][ ][ ][ ][ ]
      //[ ][ ][ ][ ][ ][ ][ ][ ][ ]
      //[ ][ ][6][ ][ ][ ][3][ ][ ]
      {
        { iDist / 2, iDist }, { iDist, 0 }, { iDist / 2, -iDist }, { -iDist, 0 },
        { iDist / 2, iDist }, { -iDist / 2, -iDist }, { 0, 0 }, { 0, 0 }
      }
  };

  // Set the number of points to be checked.
  int n_points;
  if (iDist == 1) {
    switch (pattern_type) {
      case 0:
        n_points = 4;
        break;
      case 2:
        n_points = 4;
        break;
      case 3:
        n_points = 4;
        break;
      default:
        n_points = 8;
        break;
    };
  } else {
    switch (pattern_type) {
      case 3:
        n_points = 6;
        break;
      default:
        n_points = 8;
        break;
    };
  }

  // Compute SAD values for all chosen points.
  int best_index = -1;
  for (int i = 0; i < n_points; i++) {
    vector2d_t offset = pattern[pattern_type][i];
    int x = mv.x + offset.x;
    int y = mv.y + offset.y;

    if (check_mv_cost(info, x, y)) {
      best_index = i;
    }
  }

  if (best_index >= 0) {
    *best_dist = iDist;
  }
}


void kvz_tz_raster_search(inter_search_info_t *info,
                          int iSearchRange,
                          int iRaster)
{
  const vector2d_t mv = { info->best_mv.x >> 2, info->best_mv.y >> 2 };

  //compute SAD values for every point in the iRaster downsampled version of the current search area
  for (int y = iSearchRange; y >= -iSearchRange; y -= iRaster) {
    for (int x = -iSearchRange; x <= iSearchRange; x += iRaster) {
      check_mv_cost(info, mv.x + x, mv.y + y);
    }
  }
}


static void tz_search(inter_search_info_t *info, vector2d_t extra_mv)
{
  //TZ parameters
  const int iSearchRange = 96;  // search range for each stage
  const int iRaster = 5;  // search distance limit and downsampling factor for step 3
  const unsigned step2_type = 0;  // search patterns for steps 2 and 4
  const unsigned step4_type = 0;
  const bool use_raster_scan = false;  // enable step 3
  const bool use_raster_refinement = false;  // enable step 4 mode 1
  const bool use_star_refinement = true;   // enable step 4 mode 2 (only one mode will be executed)

  int best_dist = 0;
  info->best_cost = UINT32_MAX;

  // Select starting point from among merge candidates. These should
  // include both mv_cand vectors and (0, 0).
  select_starting_point(info, extra_mv);

  // Check if we should stop search
  if (info->state->encoder_control->cfg.me_early_termination &&
      early_terminate(info))
  {
    return;
  }

  vector2d_t start = { info->best_mv.x >> 2, info->best_mv.y >> 2 };

  // step 2, grid search
  int rounds_without_improvement = 0;
  for (int iDist = 1; iDist <= iSearchRange; iDist *= 2) {
    kvz_tz_pattern_search(info, step2_type, iDist, start, &best_dist);

    // Break the loop if the last three rounds didn't produce a better MV.
    if (best_dist != iDist) rounds_without_improvement++;
    if (rounds_without_improvement >= 3) break;
  }

  if (start.x != 0 || start.y != 0) {
    // repeat step 2 starting from the zero MV
    start.x = 0;
    start.y = 0;
    rounds_without_improvement = 0;
    for (int iDist = 1; iDist <= iSearchRange/2; iDist *= 2) {
      kvz_tz_pattern_search(info, step2_type, iDist, start, &best_dist);

      if (best_dist != iDist) rounds_without_improvement++;
      if (rounds_without_improvement >= 3) break;
    }
  }

  //step 3, raster scan
  if (use_raster_scan && best_dist > iRaster) {
    best_dist = iRaster;
    kvz_tz_raster_search(info, iSearchRange, iRaster);
  }

  //step 4

  //raster refinement
  if (use_raster_refinement && best_dist > 0) {
    for (int iDist = best_dist >> 1; iDist > 0; iDist >>= 1) {
      start.x = info->best_mv.x >> 2;
      start.y = info->best_mv.y >> 2;
      kvz_tz_pattern_search(info, step4_type, iDist, start, &best_dist);
    }
  }

  //star refinement (repeat step 2 for the current starting point)
  while (use_star_refinement && best_dist > 0) {
    best_dist = 0;
    start.x = info->best_mv.x >> 2;
    start.y = info->best_mv.y >> 2;
    for (int iDist = 1; iDist <= iSearchRange; iDist *= 2) {
      kvz_tz_pattern_search(info, step4_type, iDist, start, &best_dist);
    }
  }
}


/**
 * \brief Do motion search using the HEXBS algorithm.
 *
 * \param info      search info
 * \param extra_mv  extra motion vector to check
 * \param steps     how many steps are done at maximum before exiting, does not affect the final step
 *
 * Motion vector is searched by first searching iteratively with the large
 * hexagon pattern until the best match is at the center of the hexagon.
 * As a final step a smaller hexagon is used to check the adjacent pixels.
 *
 * If a non 0,0 predicted motion vector predictor is given as extra_mv,
 * the 0,0 vector is also tried. This is hoped to help in the case where
 * the predicted motion vector is way off. In the future even more additional
 * points like 0,0 might be used, such as vectors from top or left.
 */
static void hexagon_search(inter_search_info_t *info, vector2d_t extra_mv, uint32_t steps)
{
  // The start of the hexagonal pattern has been repeated at the end so that
  // the indices between 1-6 can be used as the start of a 3-point list of new
  // points to search.
  //   6--1,7
  //  /     \    =)
  // 5   0  2,8
  //  \     /
  //   4---3
  static const vector2d_t large_hexbs[9] = {
      { 0, 0 },
      { 1, -2 }, { 2, 0 }, { 1, 2 }, { -1, 2 }, { -2, 0 }, { -1, -2 },
      { 1, -2 }, { 2, 0 }
  };
  // This is used as the last step of the hexagon search.
  //   1
  // 2 0 3
  //   4
  static const vector2d_t small_hexbs[9] = {
      { 0, 0 },
      { 0, -1 }, { -1, 0 }, { 1, 0 }, { 0, 1 },
      { -1, -1 }, { 1, -1 }, { -1, 1 }, { 1, 1 }
  };

  info->best_cost = UINT32_MAX;

  // Select starting point from among merge candidates. These should
  // include both mv_cand vectors and (0, 0).
  select_starting_point(info, extra_mv);

  // Check if we should stop search
  if (info->state->encoder_control->cfg.me_early_termination &&
      early_terminate(info))
  {
    return;
  }

  vector2d_t mv = { info->best_mv.x >> 2, info->best_mv.y >> 2 };

  // Current best index, either to merge_cands, large_hexbs or small_hexbs.
  int best_index = 0;

  // Search the initial 7 points of the hexagon.
  for (int i = 1; i < 7; ++i) {
    if (check_mv_cost(info, mv.x + large_hexbs[i].x, mv.y + large_hexbs[i].y)) {
      best_index = i;
    }
  }

  // Iteratively search the 3 new points around the best match, until the best
  // match is in the center.
  while (best_index != 0 && steps != 0) {
    // decrement count if enabled
    if (steps > 0) steps -= 1;

    // Starting point of the 3 offsets to be searched.
    unsigned start;
    if (best_index == 1) {
      start = 6;
    } else if (best_index == 8) {
      start = 1;
    } else {
      start = best_index - 1;
    }

    // Move the center to the best match.
    mv.x += large_hexbs[best_index].x;
    mv.y += large_hexbs[best_index].y;
    best_index = 0;

    // Iterate through the next 3 points.
    for (int i = 0; i < 3; ++i) {
      vector2d_t offset = large_hexbs[start + i];
      if (check_mv_cost(info, mv.x + offset.x, mv.y + offset.y)) {
        best_index = start + i;
      }
    }
  }

  // Move the center to the best match.
  //mv.x += large_hexbs[best_index].x;
  //mv.y += large_hexbs[best_index].y;

  // Do the final step of the search with a small pattern.
  for (int i = 1; i < 9; ++i) {
    check_mv_cost(info, mv.x + small_hexbs[i].x, mv.y + small_hexbs[i].y);
  }
}

/**
* \brief Do motion search using the diamond algorithm.
*
* \param info      search info
* \param extra_mv  extra motion vector to check
* \param steps     how many steps are done at maximum before exiting
*
* Motion vector is searched by searching iteratively with a diamond-shaped
* pattern. We take care of not checking the direction we came from, but
* further checking for avoiding visits to already visited points is not done.
*
* If a non 0,0 predicted motion vector predictor is given as extra_mv,
* the 0,0 vector is also tried. This is hoped to help in the case where
* the predicted motion vector is way off. In the future even more additional
* points like 0,0 might be used, such as vectors from top or left.
**/
static void diamond_search(inter_search_info_t *info, vector2d_t extra_mv, uint32_t steps) 
{
  enum diapos {
    DIA_UP = 0,
    DIA_RIGHT = 1,
    DIA_LEFT = 2,
    DIA_DOWN = 3,
    DIA_CENTER = 4,
  };

  // a diamond shape with the center included
  //   0
  // 2 4 1
  //   3
  static const vector2d_t diamond[5] = {
    {0, -1}, {1, 0}, {0, 1}, {-1, 0},
    {0, 0}
  };

  info->best_cost = UINT32_MAX;

  // Select starting point from among merge candidates. These should
  // include both mv_cand vectors and (0, 0).
  select_starting_point(info, extra_mv);

  // Check if we should stop search
  if (info->state->encoder_control->cfg.me_early_termination &&
    early_terminate(info))
  {
    return;
  }
  
  // current motion vector
  vector2d_t mv = { info->best_mv.x >> 2, info->best_mv.y >> 2 };

  // current best index
  enum diapos best_index = DIA_CENTER;

  // initial search of the points of the diamond
  for (int i = 0; i < 5; ++i) {
    if (check_mv_cost(info, mv.x + diamond[i].x, mv.y + diamond[i].y)) {
      best_index = i;
    }
  }

  if (best_index == DIA_CENTER) {
    // the center point was the best in initial check
    return;
  }

  // Move the center to the best match.
  mv.x += diamond[best_index].x;
  mv.y += diamond[best_index].y;

  // the arrival direction, the index of the diamond member that will be excluded
  enum diapos from_dir = DIA_CENTER;

  // whether we found a better candidate this iteration
  uint8_t better_found;

  do {
    better_found = 0;
    // decrement count if enabled
    if (steps > 0) steps -= 1;

    // search the points of the diamond
    for (int i = 0; i < 4; ++i) {
      // this is where we came from so it's checked already
      if (i == from_dir) continue;

      if (check_mv_cost(info, mv.x + diamond[i].x, mv.y + diamond[i].y)) {
        best_index = i;
        better_found = 1;
      }
    }

    if (better_found) {
      // Move the center to the best match.
      mv.x += diamond[best_index].x;
      mv.y += diamond[best_index].y;

      // record where we came from to the next iteration
      // the xor operation flips the orientation
      from_dir = best_index ^ 0x3;
    }
  } while (better_found && steps != 0);
  // and we're done
}


static void search_mv_full(inter_search_info_t *info,
                           int32_t search_range,
                           vector2d_t extra_mv)
{
  // Search around the 0-vector.
  for (int y = -search_range; y <= search_range; y++) {
    for (int x = -search_range; x <= search_range; x++) {
      check_mv_cost(info, x, y);
    }
  }

  // Change to integer precision.
  extra_mv.x >>= 2;
  extra_mv.y >>= 2;

  // Check around extra_mv if it's not one of the merge candidates.
  if (!mv_in_merge(info, extra_mv)) {
    for (int y = -search_range; y <= search_range; y++) {
      for (int x = -search_range; x <= search_range; x++) {
        check_mv_cost(info, extra_mv.x + x, extra_mv.y + y);
      }
    }
  }

  // Select starting point from among merge candidates. These should include
  // both mv_cand vectors and (0, 0).
  for (int i = 0; i < info->num_merge_cand; ++i) {
    if (info->merge_cand[i].dir == 3) continue;

    vector2d_t mv = {
      .x = info->merge_cand[i].mv[info->merge_cand[i].dir - 1][0] >> 2,
      .y = info->merge_cand[i].mv[info->merge_cand[i].dir - 1][1] >> 2,
    };

    // Ignore 0-vector because it has already been checked.
    if (mv.x == 0 && mv.y == 0) continue;

    vector2d_t min_mv = { mv.x - search_range, mv.y - search_range };
    vector2d_t max_mv = { mv.x + search_range, mv.y + search_range };

    for (int y = min_mv.y; y <= max_mv.y; ++y) {
      for (int x = min_mv.x; x <= max_mv.x; ++x) {
        if (!intmv_within_tile(info, x, y)) {
          continue;
        }

        // Avoid calculating the same points over and over again.
        bool already_tested = false;
        for (int j = -1; j < i; ++j) {
          int xx = 0;
          int yy = 0;
          if (j >= 0) {
            if (info->merge_cand[j].dir == 3) continue;
            xx = info->merge_cand[j].mv[info->merge_cand[j].dir - 1][0] >> 2;
            yy = info->merge_cand[j].mv[info->merge_cand[j].dir - 1][1] >> 2;
          }
          if (x >= xx - search_range && x <= xx + search_range &&
              y >= yy - search_range && y <= yy + search_range)
          {
            already_tested = true;
            x = xx + search_range;
            break;
          }
        }
        if (already_tested) continue;

        check_mv_cost(info, x, y);
      }
    }
  }
}


/**
 * \brief Do fractional motion estimation
 *
 * Algoritm first searches 1/2-pel positions around integer mv and after best match is found,
 * refines the search by searching best 1/4-pel postion around best 1/2-pel position.
 */
static void search_frac(inter_search_info_t *info)
{
  // Map indexes to relative coordinates in the following way:
  // 5 3 6
  // 1 0 2
  // 7 4 8
  static const vector2d_t square[9] = {
      {  0,  0 },  { -1,  0 },  {  1,  0 },
      {  0, -1 },  {  0,  1 },  { -1, -1 },
      {  1, -1 },  { -1,  1 },  {  1,  1 }
  };

  // Set mv to pixel precision
  vector2d_t mv = { info->best_mv.x >> 2, info->best_mv.y >> 2 };

  unsigned best_cost = UINT32_MAX;
  uint32_t best_bitcost = 0;
  uint32_t bitcosts[4] = { 0 };
  unsigned best_index = 0;

  unsigned costs[4] = { 0 };

  kvz_extended_block src = { 0, 0, 0, 0 };
  ALIGNED(64) kvz_pixel filtered[4][LCU_WIDTH * LCU_WIDTH];

  // Storage buffers for intermediate horizontally filtered results.
  // Have the first columns in contiguous memory for vectorization.
  ALIGNED(64) int16_t intermediate[5][(KVZ_EXT_BLOCK_W_LUMA + 1) * LCU_WIDTH];
  int16_t hor_first_cols[5][KVZ_EXT_BLOCK_W_LUMA + 1];

  const kvz_picture *ref = info->ref;
  const kvz_picture *pic = info->pic;
  vector2d_t orig = info->origin;
  const int width = info->width;
  const int height = info->height;
  const int internal_width  = ((width  + 7) >> 3) << 3; // Round up to closest 8
  const int internal_height = ((height + 7) >> 3) << 3;

  const encoder_state_t *state = info->state;
  int fme_level = state->encoder_control->cfg.fme_level;
  int8_t sample_off_x = 0;
  int8_t sample_off_y = 0;

  kvz_get_extended_block(orig.x, orig.y, mv.x - 1, mv.y - 1,
                state->tile->offset_x,
                state->tile->offset_y,
                ref->y, ref->width, ref->height, KVZ_LUMA_FILTER_TAPS,
                internal_width+1, internal_height+1,
                &src);

  kvz_pixel *tmp_pic = pic->y + orig.y * pic->stride + orig.x;
  int tmp_stride = pic->stride;
                  
  // Search integer position
  costs[0] = kvz_satd_any_size(width, height,
                            tmp_pic, tmp_stride,
                            src.orig_topleft + src.stride + 1, src.stride);

  costs[0] += info->mvd_cost_func(state,
                                  mv.x, mv.y, 2,
                                  info->mv_cand,
                                  info->merge_cand,
                                  info->num_merge_cand,
                                  info->ref_idx,
                                  &bitcosts[0]);
  best_cost = costs[0];
  best_bitcost = bitcosts[0];
  
  //Set mv to half-pixel precision
  mv.x *= 2;
  mv.y *= 2;

  ipol_blocks_func * filter_steps[4] = {
    kvz_filter_hpel_blocks_hor_ver_luma,
    kvz_filter_hpel_blocks_diag_luma,
    kvz_filter_qpel_blocks_hor_ver_luma,
    kvz_filter_qpel_blocks_diag_luma,
  };

  // Search halfpel positions around best integer mv
  int i = 1;
  for (int step = 0; step < fme_level; ++step){

    const int mv_shift = (step < 2) ? 1 : 0;

    filter_steps[step](state->encoder_control,
      src.orig_topleft,
      src.stride,
      internal_width,
      internal_height,
      filtered,
      intermediate,
      fme_level,
      hor_first_cols,
      sample_off_x,
      sample_off_y);
          
    const vector2d_t *pattern[4] = { &square[i], &square[i + 1], &square[i + 2], &square[i + 3] };

    int8_t within_tile[4];
    for (int j = 0; j < 4; j++) {
      within_tile[j] =
        fracmv_within_tile(info, (mv.x + pattern[j]->x) * (1 << mv_shift), (mv.y + pattern[j]->y) * (1 << mv_shift));
    };

    kvz_pixel *filtered_pos[4] = { 0 };
    filtered_pos[0] = &filtered[0][0];
    filtered_pos[1] = &filtered[1][0];
    filtered_pos[2] = &filtered[2][0];
    filtered_pos[3] = &filtered[3][0];

    kvz_satd_any_size_quad(width, height, (const kvz_pixel **)filtered_pos, LCU_WIDTH, tmp_pic, tmp_stride, 4, costs, within_tile);

    for (int j = 0; j < 4; j++) {
      if (within_tile[j]) {
        costs[j] += info->mvd_cost_func(
            state,
            mv.x + pattern[j]->x,
            mv.y + pattern[j]->y,
            mv_shift,
            info->mv_cand,
            info->merge_cand,
            info->num_merge_cand,
            info->ref_idx,
            &bitcosts[j]
        );
      }
    }

    for (int j = 0; j < 4; ++j) {
      if (within_tile[j] && costs[j] < best_cost) {
        best_cost = costs[j];
        best_bitcost = bitcosts[j];
        best_index = i + j;
      }
    }

    i += 4;

    // Update mv for the best position on current precision
    if (step == 1 || step == fme_level - 1) {
      // Move search to best_index
      mv.x += square[best_index].x;
      mv.y += square[best_index].y;

      // On last hpel step...
      if (step == MIN(fme_level - 1, 1)) {
        //Set mv to quarterpel precision
        mv.x *= 2;
        mv.y *= 2;
        sample_off_x = square[best_index].x;
        sample_off_y = square[best_index].y;
        best_index = 0;
        i = 1;
      }
    }
  }

  info->best_mv = mv;
  info->best_cost = best_cost;
  info->best_bitcost = best_bitcost;

  if (src.malloc_used) free(src.buffer);
}

/**
* \brief Calculate the scaled MV
*/
static INLINE int16_t get_scaled_mv(int16_t mv, int scale)
{
  int32_t scaled = scale * mv;
  return CLIP(-32768, 32767, (scaled + 127 + (scaled < 0)) >> 8);
}
/**
* \brief Scale the MV according to the POC difference
*
* \param current_poc        POC of current frame
* \param current_ref_poc    POC of reference frame
* \param neighbor_poc       POC of neighbor frame
* \param neighbor_ref_poc   POC of neighbors reference frame
* \param mv_cand            MV candidates to scale
*/
static void apply_mv_scaling(int32_t current_poc,
  int32_t current_ref_poc,
  int32_t neighbor_poc,
  int32_t neighbor_ref_poc,
  vector2d_t* mv_cand)
{
  int32_t diff_current = current_poc - current_ref_poc;
  int32_t diff_neighbor = neighbor_poc - neighbor_ref_poc;

  if (diff_current == diff_neighbor) return;
  if (diff_neighbor == 0) return;

  diff_current = CLIP(-128, 127, diff_current);
  diff_neighbor = CLIP(-128, 127, diff_neighbor);

  int scale = CLIP(-4096, 4095,
    (diff_current * ((0x4000 + (abs(diff_neighbor) >> 1)) / diff_neighbor) + 32) >> 6);

  mv_cand->x = get_scaled_mv(mv_cand->x, scale);
  mv_cand->y = get_scaled_mv(mv_cand->y, scale);
}


/**
 * \brief Perform inter search for a single reference frame.
 */
static void search_pu_inter_ref(inter_search_info_t *info,
  int depth,
  lcu_t *lcu, cu_info_t *cur_cu,
  double *inter_cost,
  uint32_t *inter_bitcost,
  double *best_LX_cost,
  cu_info_t *unipred_LX)
{
  const kvz_config *cfg = &info->state->encoder_control->cfg;

  // which list, L0 or L1, ref_idx is in and in what index
  int8_t ref_list = -1;
  // the index of the ref_idx in L0 or L1 list
  int8_t LX_idx;
  // max value of LX_idx plus one
  const int8_t LX_IDX_MAX_PLUS_1 = MAX(info->state->frame->ref_LX_size[0],
    info->state->frame->ref_LX_size[1]);

  for (LX_idx = 0; LX_idx < LX_IDX_MAX_PLUS_1; LX_idx++)
  {
    // check if ref_idx is in L0
    if (LX_idx < info->state->frame->ref_LX_size[0] &&
      info->state->frame->ref_LX[0][LX_idx] == info->ref_idx) {
      ref_list = 0;
      break;
    }

    // check if ref_idx is in L1
    if (LX_idx < info->state->frame->ref_LX_size[1] &&
      info->state->frame->ref_LX[1][LX_idx] == info->ref_idx) {
      ref_list = 1;
      break;
    }
  }
  // ref_idx has to be found in either L0 or L1
  assert(LX_idx < LX_IDX_MAX_PLUS_1);

  // store temp values to be stored back later
  int8_t temp_ref_idx = cur_cu->inter.mv_ref[ref_list];

  // Get MV candidates
  cur_cu->inter.mv_ref[ref_list] = LX_idx;

  kvz_inter_get_mv_cand(info->state,
    info->origin.x,
    info->origin.y,
    info->width,
    info->height,
    info->mv_cand,
    cur_cu,
    lcu,
    ref_list);

  // store old values back
  cur_cu->inter.mv_ref[ref_list] = temp_ref_idx;

  vector2d_t mv = { 0, 0 };

  // Take starting point for MV search from previous frame.
  // When temporal motion vector candidates are added, there is probably
  // no point to this anymore, but for now it helps.
  const int mid_x = info->state->tile->offset_x + info->origin.x + (info->width >> 1);
  const int mid_y = info->state->tile->offset_y + info->origin.y + (info->height >> 1);
  const cu_array_t* ref_array = info->state->frame->ref->cu_arrays[info->ref_idx];
  const cu_info_t* ref_cu = kvz_cu_array_at_const(ref_array, mid_x, mid_y);
  if (ref_cu->type == CU_INTER) {
    if (ref_cu->inter.mv_dir & 1) {
      mv.x = ref_cu->inter.mv[0][0];
      mv.y = ref_cu->inter.mv[0][1];
    }
    else {
      mv.x = ref_cu->inter.mv[1][0];
      mv.y = ref_cu->inter.mv[1][1];
    }
    // Apply mv scaling if neighbor poc is available
    if (info->state->frame->ref_LX_size[ref_list] > 0) {
      // When there are reference pictures from the future (POC > current POC)
      // in L0 or L1, the primary list for the colocated PU is the inverse of
      // collocated_from_l0_flag. Otherwise it is equal to reflist.
      //
      // Kvazaar always sets collocated_from_l0_flag so the list is L1 when
      // there are future references.
      int col_list = ref_list;
      for (int i = 0; i < info->state->frame->ref->used_size; i++) {
        if (info->state->frame->ref->pocs[i] > info->state->frame->poc) {
          col_list = 1;
          break;
        }
      }
      if ((ref_cu->inter.mv_dir & (col_list + 1)) == 0) {
        // Use the other list if the colocated PU does not have a MV for the
        // primary list.
        col_list = 1 - col_list;
      }

      uint8_t neighbor_poc_index = info->state->frame->ref_LX[ref_list][LX_idx];
      // Scaling takes current POC, reference POC, neighbor POC and neighbor reference POC as argument
      apply_mv_scaling(
        info->state->frame->poc,
        info->state->frame->ref->pocs[info->state->frame->ref_LX[ref_list][LX_idx]],
        info->state->frame->ref->pocs[neighbor_poc_index],
        info->state->frame->ref->images[neighbor_poc_index]->ref_pocs[
          info->state->frame->ref->ref_LXs[neighbor_poc_index]
          [col_list]
          [ref_cu->inter.mv_ref[col_list]]
        ],
        &mv
      );
    }
  }

  int search_range = 32;
  switch (cfg->ime_algorithm) {
    case KVZ_IME_FULL64: search_range = 64; break;
    case KVZ_IME_FULL32: search_range = 32; break;
    case KVZ_IME_FULL16: search_range = 16; break;
    case KVZ_IME_FULL8: search_range = 8; break;
    default: break;
  }

  info->best_cost = UINT32_MAX;

  switch (cfg->ime_algorithm) {
    case KVZ_IME_TZ:
      tz_search(info, mv);
      break;

    case KVZ_IME_FULL64:
    case KVZ_IME_FULL32:
    case KVZ_IME_FULL16:
    case KVZ_IME_FULL8:
    case KVZ_IME_FULL:
      search_mv_full(info, search_range, mv);
      break;

    case KVZ_IME_DIA:
      diamond_search(info, mv, info->state->encoder_control->cfg.me_max_steps);
      break;

    default:
      hexagon_search(info, mv, info->state->encoder_control->cfg.me_max_steps);
      break;
  }

  if (cfg->fme_level > 0 && info->best_cost < *inter_cost) {
    search_frac(info);

  } else if (info->best_cost < UINT32_MAX) {
    // Recalculate inter cost with SATD.
    info->best_cost = kvz_image_calc_satd(
        info->state->tile->frame->source,
        info->ref,
        info->origin.x,
        info->origin.y,
        info->state->tile->offset_x + info->origin.x + (info->best_mv.x >> 2),
        info->state->tile->offset_y + info->origin.y + (info->best_mv.y >> 2),
        info->width,
        info->height);
    info->best_cost += info->best_bitcost * (int)(info->state->lambda_sqrt + 0.5);
  }

  mv = info->best_mv;

  int merged = 0;
  int merge_idx = 0;
  // Check every candidate to find a match
  for (merge_idx = 0; merge_idx < info->num_merge_cand; merge_idx++) {
    if (info->merge_cand[merge_idx].dir != 3 &&
        info->merge_cand[merge_idx].mv[info->merge_cand[merge_idx].dir - 1][0] == mv.x &&
        info->merge_cand[merge_idx].mv[info->merge_cand[merge_idx].dir - 1][1] == mv.y &&
        (uint32_t)info->state->frame->ref_LX[info->merge_cand[merge_idx].dir - 1][
        info->merge_cand[merge_idx].ref[info->merge_cand[merge_idx].dir - 1]] == info->ref_idx)
    {
      merged = 1;
      break;
    }
  }

  // Only check when candidates are different
  int cu_mv_cand = 0;
  if (!merged) {
    cu_mv_cand =
      select_mv_cand(info->state, info->mv_cand, mv.x, mv.y, NULL);
  }

  if (info->best_cost < *inter_cost) {
    // Map reference index to L0/L1 pictures
    cur_cu->inter.mv_dir = ref_list+1;
    uint8_t mv_ref_coded = LX_idx;

    cur_cu->merged                  = merged;
    cur_cu->merge_idx               = merge_idx;
    cur_cu->inter.mv_ref[ref_list]  = LX_idx;
    cur_cu->inter.mv[ref_list][0]   = (int16_t)mv.x;
    cur_cu->inter.mv[ref_list][1]   = (int16_t)mv.y;

    CU_SET_MV_CAND(cur_cu, ref_list, cu_mv_cand);

    *inter_cost = info->best_cost;
    *inter_bitcost = info->best_bitcost + cur_cu->inter.mv_dir - 1 + mv_ref_coded;
  }


  // Update best unipreds for biprediction
  if (info->best_cost < best_LX_cost[ref_list]) {
    bool valid_mv = fracmv_within_tile(info, mv.x, mv.y);
    if (valid_mv) {
      // Map reference index to L0/L1 pictures
      unipred_LX[ref_list].inter.mv_dir = ref_list + 1;
      unipred_LX[ref_list].inter.mv_ref[ref_list] = LX_idx;
      unipred_LX[ref_list].inter.mv[ref_list][0] = (int16_t)mv.x;
      unipred_LX[ref_list].inter.mv[ref_list][1] = (int16_t)mv.y;

      CU_SET_MV_CAND(&unipred_LX[ref_list], ref_list, cu_mv_cand);

      best_LX_cost[ref_list] = info->best_cost;
    }
  }
}


/**
 * \brief Search bipred modes for a PU.
 */
static void search_pu_inter_bipred(inter_search_info_t *info,
                                   int depth,
                                   lcu_t *lcu, cu_info_t *cur_cu,
                                   double *inter_cost,
                                   uint32_t *inter_bitcost)
{
  const image_list_t *const ref = info->state->frame->ref;
  uint8_t (*ref_LX)[16] = info->state->frame->ref_LX;
  const videoframe_t * const frame = info->state->tile->frame;
  const int x         = info->origin.x;
  const int y         = info->origin.y;
  const int width     = info->width;
  const int height    = info->height;

  static const uint8_t priorityList0[] = { 0, 1, 0, 2, 1, 2, 0, 3, 1, 3, 2, 3 };
  static const uint8_t priorityList1[] = { 1, 0, 2, 0, 2, 1, 3, 0, 3, 1, 3, 2 };
  const unsigned num_cand_pairs =
    MIN(info->num_merge_cand * (info->num_merge_cand - 1), 12);

  inter_merge_cand_t *merge_cand = info->merge_cand;

  for (int32_t idx = 0; idx < num_cand_pairs; idx++) {
    uint8_t i = priorityList0[idx];
    uint8_t j = priorityList1[idx];
    if (i >= info->num_merge_cand || j >= info->num_merge_cand) break;

    // Find one L0 and L1 candidate according to the priority list
    if (!(merge_cand[i].dir & 0x1) || !(merge_cand[j].dir & 0x2)) continue;

    if (ref_LX[0][merge_cand[i].ref[0]] == ref_LX[1][merge_cand[j].ref[1]] &&
        merge_cand[i].mv[0][0] == merge_cand[j].mv[1][0] &&
        merge_cand[i].mv[0][1] == merge_cand[j].mv[1][1])
    {
      continue;
    }

    int16_t mv[2][2];
    mv[0][0] = merge_cand[i].mv[0][0];
    mv[0][1] = merge_cand[i].mv[0][1];
    mv[1][0] = merge_cand[j].mv[1][0];
    mv[1][1] = merge_cand[j].mv[1][1];

    // Don't try merge candidates that don't satisfy mv constraints.
    if (!fracmv_within_tile(info, mv[0][0], mv[0][1]) ||
        !fracmv_within_tile(info, mv[1][0], mv[1][1]))
    {
      continue;
    }

    kvz_inter_recon_bipred(info->state,
                           ref->images[ref_LX[0][merge_cand[i].ref[0]]],
                           ref->images[ref_LX[1][merge_cand[j].ref[1]]],
                           x, y,
                           width,
                           height,
                           mv,
                           lcu,
                           true,
                           false);

    const kvz_pixel *rec = &lcu->rec.y[SUB_SCU(y) * LCU_WIDTH + SUB_SCU(x)];
    const kvz_pixel *src = &frame->source->y[x + y * frame->source->width];
    uint32_t cost =
      kvz_satd_any_size(width, height, rec, LCU_WIDTH, src, frame->source->width);

    uint32_t bitcost[2] = { 0, 0 };

    cost += info->mvd_cost_func(info->state,
                               merge_cand[i].mv[0][0],
                               merge_cand[i].mv[0][1],
                               0,
                               info->mv_cand,
                               NULL, 0, 0,
                               &bitcost[0]);
    cost += info->mvd_cost_func(info->state,
                               merge_cand[i].mv[1][0],
                               merge_cand[i].mv[1][1],
                               0,
                               info->mv_cand,
                               NULL, 0, 0,
                               &bitcost[1]);

    const uint8_t mv_ref_coded[2] = {
      merge_cand[i].ref[0],
      merge_cand[j].ref[1]
    };
    const int extra_bits = mv_ref_coded[0] + mv_ref_coded[1] + 2 /* mv dir cost */;
    cost += info->state->lambda_sqrt * extra_bits + 0.5;

    if (cost < *inter_cost) {
      cur_cu->inter.mv_dir = 3;

      cur_cu->inter.mv_ref[0] = merge_cand[i].ref[0];
      cur_cu->inter.mv_ref[1] = merge_cand[j].ref[1];

      cur_cu->inter.mv[0][0] = merge_cand[i].mv[0][0];
      cur_cu->inter.mv[0][1] = merge_cand[i].mv[0][1];
      cur_cu->inter.mv[1][0] = merge_cand[j].mv[1][0];
      cur_cu->inter.mv[1][1] = merge_cand[j].mv[1][1];
      cur_cu->merged = 0;

      // Check every candidate to find a match
      for (int merge_idx = 0; merge_idx < info->num_merge_cand; merge_idx++) {
        if (merge_cand[merge_idx].mv[0][0] == cur_cu->inter.mv[0][0] &&
            merge_cand[merge_idx].mv[0][1] == cur_cu->inter.mv[0][1] &&
            merge_cand[merge_idx].mv[1][0] == cur_cu->inter.mv[1][0] &&
            merge_cand[merge_idx].mv[1][1] == cur_cu->inter.mv[1][1] &&
            merge_cand[merge_idx].ref[0] == cur_cu->inter.mv_ref[0] &&
            merge_cand[merge_idx].ref[1] == cur_cu->inter.mv_ref[1])
        {
          cur_cu->merged = 1;
          cur_cu->merge_idx = merge_idx;
          break;
        }
      }

      // Each motion vector has its own candidate
      for (int reflist = 0; reflist < 2; reflist++) {
        kvz_inter_get_mv_cand(info->state, x, y, width, height, info->mv_cand, cur_cu, lcu, reflist);
        int cu_mv_cand = select_mv_cand(
            info->state,
            info->mv_cand,
            cur_cu->inter.mv[reflist][0],
            cur_cu->inter.mv[reflist][1],
            NULL);
        CU_SET_MV_CAND(cur_cu, reflist, cu_mv_cand);
      }

      *inter_cost = cost;
      *inter_bitcost = bitcost[0] + bitcost[1] + extra_bits;
    }
  }
}

/**
 * \brief Check if an identical merge candidate exists in a list
 *
 * \param all_cand        Full list of available merge candidates
 * \param cand_to_add     Merge candidate to be checked for duplicates
 * \param added_idx_list  List of indices of unique merge candidates
 * \param list_size       Size of the list
 *
 * \return                Does an identical candidate exist in list
 */
static bool merge_candidate_in_list(inter_merge_cand_t * all_cands,
                                    inter_merge_cand_t * cand_to_add,
                                    int8_t * added_idx_list,
                                    int list_size)
{
  bool found = false;
  for (int i = 0; i < list_size && !found; ++i) {
    inter_merge_cand_t * list_cand = &all_cands[added_idx_list[i]];

    found = cand_to_add->dir == list_cand->dir &&
        cand_to_add->ref[0] == list_cand->ref[0] &&
        cand_to_add->mv[0][0] == list_cand->mv[0][0] &&
        cand_to_add->mv[0][1] == list_cand->mv[0][1] &&
        cand_to_add->ref[1] == list_cand->ref[1] &&
        cand_to_add->mv[1][0] == list_cand->mv[1][0] &&
        cand_to_add->mv[1][1] == list_cand->mv[1][1];
  }

  return found;
}

/**
 * \brief Update PU to have best modes at this depth.
 *
 * \param state       encoder state
 * \param x_cu        x-coordinate of the containing CU
 * \param y_cu        y-coordinate of the containing CU
 * \param depth       depth of the CU in the quadtree
 * \param part_mode   partition mode of the CU
 * \param i_pu        index of the PU in the CU
 * \param lcu         containing LCU
 *
 * \param inter_cost    Return inter cost of the best mode
 * \param inter_bitcost Return inter bitcost of the best mode
 */
static void search_pu_inter(encoder_state_t * const state,
                            int x_cu, int y_cu,
                            int depth,
                            part_mode_t part_mode,
                            int i_pu,
                            lcu_t *lcu,
                            double *inter_cost,
                            uint32_t *inter_bitcost)
{
  *inter_cost = MAX_INT;
  *inter_bitcost = MAX_INT;

  const kvz_config *cfg = &state->encoder_control->cfg;
  const videoframe_t * const frame = state->tile->frame;
  const int width_cu  = LCU_WIDTH >> depth;
  const int x         = PU_GET_X(part_mode, width_cu, x_cu, i_pu);
  const int y         = PU_GET_Y(part_mode, width_cu, y_cu, i_pu);
  const int width     = PU_GET_W(part_mode, width_cu, i_pu);
  const int height    = PU_GET_H(part_mode, width_cu, i_pu);

  // Merge candidate A1 may not be used for the second PU of Nx2N, nLx2N and
  // nRx2N partitions.
  const bool merge_a1 = i_pu == 0 || width >= height;
  // Merge candidate B1 may not be used for the second PU of 2NxN, 2NxnU and
  // 2NxnD partitions.
  const bool merge_b1 = i_pu == 0 || width <= height;

  const int x_local   = SUB_SCU(x);
  const int y_local   = SUB_SCU(y);
  cu_info_t *cur_cu   = LCU_GET_CU_AT_PX(lcu, x_local, y_local);

  inter_search_info_t info = {
    .state          = state,
    .pic            = frame->source,
    .origin         = { x, y },
    .width          = width,
    .height         = height,
    .mvd_cost_func  = cfg->mv_rdo ? kvz_calc_mvd_cost_cabac : calc_mvd_cost,
    .optimized_sad  = kvz_get_optimized_sad(width),
  };

  // Search for merge mode candidates
  info.num_merge_cand = kvz_inter_get_merge_cand(
      state,
      x, y,
      width, height,
      merge_a1, merge_b1,
      info.merge_cand,
      lcu
  );

  // Default to candidate 0
  CU_SET_MV_CAND(cur_cu, 0, 0);
  CU_SET_MV_CAND(cur_cu, 1, 0);

  // Merge Analysis starts here
  int8_t mrg_cands[MRG_MAX_NUM_CANDS];
  double mrg_costs[MRG_MAX_NUM_CANDS];
  for (int i = 0; i < MRG_MAX_NUM_CANDS; ++i) {
    mrg_cands[i] = -1;
    mrg_costs[i] = MAX_DOUBLE;
  }

  int num_rdo_cands = 0;

  // Check motion vector constraints and perform rough search
  for (int merge_idx = 0; merge_idx < info.num_merge_cand; ++merge_idx) {

    inter_merge_cand_t *cur_cand = &info.merge_cand[merge_idx];
    cur_cu->inter.mv_dir = cur_cand->dir;
    cur_cu->inter.mv_ref[0] = cur_cand->ref[0];
    cur_cu->inter.mv_ref[1] = cur_cand->ref[1];
    cur_cu->inter.mv[0][0] = cur_cand->mv[0][0];
    cur_cu->inter.mv[0][1] = cur_cand->mv[0][1];
    cur_cu->inter.mv[1][0] = cur_cand->mv[1][0];
    cur_cu->inter.mv[1][1] = cur_cand->mv[1][1];

    // If bipred is not enabled, do not try candidates with mv_dir == 3.
    // Bipred is also forbidden for 4x8 and 8x4 blocks by the standard. 
    if (cur_cu->inter.mv_dir == 3 && !state->encoder_control->cfg.bipred) continue;
    if (cur_cu->inter.mv_dir == 3 && !(width + height > 12)) continue;

    bool is_duplicate = merge_candidate_in_list(info.merge_cand, cur_cand,
      mrg_cands, 
      num_rdo_cands);

    // Don't try merge candidates that don't satisfy mv constraints.
    // Don't add duplicates to list
    if (!fracmv_within_tile(&info, cur_cu->inter.mv[0][0], cur_cu->inter.mv[0][1]) ||
        !fracmv_within_tile(&info, cur_cu->inter.mv[1][0], cur_cu->inter.mv[1][1]) ||
        is_duplicate)
    {
      continue;
    }

    kvz_inter_pred_pu(state, lcu, x_cu, y_cu, width_cu, true, false, i_pu);
    mrg_costs[num_rdo_cands] = kvz_satd_any_size(width, height,
      lcu->rec.y + y_local * LCU_WIDTH + x_local, LCU_WIDTH,
      lcu->ref.y + y_local * LCU_WIDTH + x_local, LCU_WIDTH);
    
    // Add cost of coding the merge index
    mrg_costs[num_rdo_cands] += merge_idx * info.state->lambda_sqrt;

    mrg_cands[num_rdo_cands] = merge_idx;
    num_rdo_cands++;
  }

  // Sort candidates by cost
  kvz_sort_modes(mrg_cands, mrg_costs, num_rdo_cands);

  // Limit by availability
  // TODO: Do not limit to just 1
  num_rdo_cands = MIN(1, num_rdo_cands);
    
  // Early Skip Mode Decision
  bool has_chroma = state->encoder_control->chroma_format != KVZ_CSP_400;
  if (cfg->early_skip && cur_cu->part_size == SIZE_2Nx2N) {
    for (int merge_rdo_idx = 0; merge_rdo_idx < num_rdo_cands; ++merge_rdo_idx) {

      // Reconstruct blocks with merge candidate.
      // Check luma CBF. Then, check chroma CBFs if luma CBF is not set
      // and chroma exists.
      // Early terminate if merge candidate with zero CBF is found.
      int merge_idx = mrg_cands[merge_rdo_idx];
      cur_cu->inter.mv_dir = info.merge_cand[merge_idx].dir;
      cur_cu->inter.mv_ref[0] = info.merge_cand[merge_idx].ref[0];
      cur_cu->inter.mv_ref[1] = info.merge_cand[merge_idx].ref[1];
      cur_cu->inter.mv[0][0] = info.merge_cand[merge_idx].mv[0][0];
      cur_cu->inter.mv[0][1] = info.merge_cand[merge_idx].mv[0][1];
      cur_cu->inter.mv[1][0] = info.merge_cand[merge_idx].mv[1][0];
      cur_cu->inter.mv[1][1] = info.merge_cand[merge_idx].mv[1][1];
      kvz_lcu_fill_trdepth(lcu, x, y, depth, MAX(1, depth));
      kvz_inter_recon_cu(state, lcu, x, y, width, true, false);
      kvz_quantize_lcu_residual(state, true, false, x, y, depth, cur_cu, lcu, true);

      if (cbf_is_set(cur_cu->cbf, depth, COLOR_Y)) {
        continue;
      }
      else if (has_chroma) {
        kvz_inter_recon_cu(state, lcu, x, y, width, false, has_chroma);
        kvz_quantize_lcu_residual(state, false, has_chroma, x, y, depth, cur_cu, lcu, true);
        if (!cbf_is_set_any(cur_cu->cbf, depth)) {
          cur_cu->type = CU_INTER;
          cur_cu->merge_idx = merge_idx;
          cur_cu->skipped = true;
          *inter_cost = 0.0;  // TODO: Check this
          *inter_bitcost = merge_idx; // TODO: Check this
          return;
        }
      }
    }
  }

  // AMVP search starts here

  // Store unipred information of L0 and L1 for biprediction
  // Best cost will be left at MAX_DOUBLE if no valid CU is found
  double best_cost_LX[2] = { MAX_DOUBLE, MAX_DOUBLE };
  cu_info_t unipreds[2];

  for (int ref_idx = 0; ref_idx < state->frame->ref->used_size; ref_idx++) {
    info.ref_idx = ref_idx;
    info.ref = state->frame->ref->images[ref_idx];

    search_pu_inter_ref(&info, depth, lcu, cur_cu, inter_cost, inter_bitcost, best_cost_LX, unipreds);
  }

  // Search bi-pred positions
  bool can_use_bipred = state->frame->slicetype == KVZ_SLICE_B
    && cfg->bipred
    && width + height >= 16; // 4x8 and 8x4 PBs are restricted to unipred

  if (can_use_bipred) {

    // Try biprediction from valid acquired unipreds.
    if (best_cost_LX[0] != MAX_DOUBLE && best_cost_LX[1] != MAX_DOUBLE) {

      // TODO: logic is copy paste from search_pu_inter_bipred.
      // Get rid of duplicate code asap.
      const image_list_t *const ref = info.state->frame->ref;
      uint8_t(*ref_LX)[16] = info.state->frame->ref_LX;

      inter_merge_cand_t *merge_cand = info.merge_cand;

      int16_t mv[2][2];
      mv[0][0] = unipreds[0].inter.mv[0][0];
      mv[0][1] = unipreds[0].inter.mv[0][1];
      mv[1][0] = unipreds[1].inter.mv[1][0];
      mv[1][1] = unipreds[1].inter.mv[1][1];

      kvz_inter_recon_bipred(info.state,
        ref->images[ref_LX[0][unipreds[0].inter.mv_ref[0]]],
        ref->images[ref_LX[1][unipreds[1].inter.mv_ref[1]]],
        x, y,
        width,
        height,
        mv,
        lcu,
        true,
        false);

      const kvz_pixel *rec = &lcu->rec.y[SUB_SCU(y) * LCU_WIDTH + SUB_SCU(x)];
      const kvz_pixel *src = &lcu->ref.y[SUB_SCU(y) * LCU_WIDTH + SUB_SCU(x)];
      uint32_t cost =
        kvz_satd_any_size(width, height, rec, LCU_WIDTH, src, LCU_WIDTH);

      uint32_t bitcost[2] = { 0, 0 };

      cost += info.mvd_cost_func(info.state,
        unipreds[0].inter.mv[0][0],
        unipreds[0].inter.mv[0][1],
        0,
        info.mv_cand,
        NULL, 0, 0,
        &bitcost[0]);
      cost += info.mvd_cost_func(info.state,
        unipreds[1].inter.mv[1][0],
        unipreds[1].inter.mv[1][1],
        0,
        info.mv_cand,
        NULL, 0, 0,
        &bitcost[1]);

      const uint8_t mv_ref_coded[2] = {
        unipreds[0].inter.mv_ref[0],
        unipreds[1].inter.mv_ref[1]
      };
      const int extra_bits = mv_ref_coded[0] + mv_ref_coded[1] + 2 /* mv dir cost */;
      cost += info.state->lambda_sqrt * extra_bits + 0.5;

      if (cost < *inter_cost) {
        cur_cu->inter.mv_dir = 3;

        cur_cu->inter.mv_ref[0] = unipreds[0].inter.mv_ref[0];
        cur_cu->inter.mv_ref[1] = unipreds[1].inter.mv_ref[1];

        cur_cu->inter.mv[0][0] = unipreds[0].inter.mv[0][0];
        cur_cu->inter.mv[0][1] = unipreds[0].inter.mv[0][1];
        cur_cu->inter.mv[1][0] = unipreds[1].inter.mv[1][0];
        cur_cu->inter.mv[1][1] = unipreds[1].inter.mv[1][1];
        cur_cu->merged = 0;

        // Check every candidate to find a match
        for (int merge_idx = 0; merge_idx < info.num_merge_cand; merge_idx++) {
          if (merge_cand[merge_idx].mv[0][0] == cur_cu->inter.mv[0][0] &&
            merge_cand[merge_idx].mv[0][1] == cur_cu->inter.mv[0][1] &&
            merge_cand[merge_idx].mv[1][0] == cur_cu->inter.mv[1][0] &&
            merge_cand[merge_idx].mv[1][1] == cur_cu->inter.mv[1][1] &&
            merge_cand[merge_idx].ref[0] == cur_cu->inter.mv_ref[0] &&
            merge_cand[merge_idx].ref[1] == cur_cu->inter.mv_ref[1])
          {
            cur_cu->merged = 1;
            cur_cu->merge_idx = merge_idx;
            break;
          }
        }

        // Each motion vector has its own candidate
        for (int reflist = 0; reflist < 2; reflist++) {
          kvz_inter_get_mv_cand(info.state, x, y, width, height, info.mv_cand, cur_cu, lcu, reflist);
          int cu_mv_cand = select_mv_cand(
            info.state,
            info.mv_cand,
            cur_cu->inter.mv[reflist][0],
            cur_cu->inter.mv[reflist][1],
            NULL);
          CU_SET_MV_CAND(cur_cu, reflist, cu_mv_cand);
        }

        *inter_cost = cost;
        *inter_bitcost = bitcost[0] + bitcost[1] + extra_bits;
      }
    }

    // TODO: this probably should have a separate command line option
    if (cfg->rdo == 3) {
      search_pu_inter_bipred(&info, depth, lcu, cur_cu, inter_cost, inter_bitcost);
    }
  }

  // Compare best merge cost to amvp cost
  if (mrg_costs[0] < *inter_cost) {
    *inter_cost = mrg_costs[0];
    *inter_bitcost = 0; // TODO: Check this
    int merge_idx = mrg_cands[0];
    cur_cu->type = CU_INTER;
    cur_cu->merge_idx = merge_idx;
    cur_cu->inter.mv_dir = info.merge_cand[merge_idx].dir;
    cur_cu->inter.mv_ref[0] = info.merge_cand[merge_idx].ref[0];
    cur_cu->inter.mv_ref[1] = info.merge_cand[merge_idx].ref[1];
    cur_cu->inter.mv[0][0] = info.merge_cand[merge_idx].mv[0][0];
    cur_cu->inter.mv[0][1] = info.merge_cand[merge_idx].mv[0][1];
    cur_cu->inter.mv[1][0] = info.merge_cand[merge_idx].mv[1][0];
    cur_cu->inter.mv[1][1] = info.merge_cand[merge_idx].mv[1][1];
    cur_cu->merged = true;
    cur_cu->skipped = false;
  }

  if (*inter_cost < INT_MAX && cur_cu->inter.mv_dir == 1) {
    assert(fracmv_within_tile(&info, cur_cu->inter.mv[0][0], cur_cu->inter.mv[0][1]));
  }
}

/**
* \brief Calculate inter coding cost for luma and chroma CBs (--rd=2 accuracy).
*
* Calculate inter coding cost of each CB. This should match the intra coding cost
* calculation that is used on this RDO accuracy, since CU type decision is based
* on this.
*
* The cost includes SSD distortion, transform unit tree bits and motion vector bits
* for both luma and chroma if enabled.
*
* \param state       encoder state
* \param x           x-coordinate of the CU
* \param y           y-coordinate of the CU
* \param depth       depth of the CU in the quadtree
* \param lcu         containing LCU
*
* \param inter_cost    Return inter cost
* \param inter_bitcost Return inter bitcost
*/
void kvz_cu_cost_inter_rd2(encoder_state_t * const state,
  int x, int y, int depth,
  lcu_t *lcu,
  double   *inter_cost,
  uint32_t *inter_bitcost){

  cu_info_t *cur_cu = LCU_GET_CU_AT_PX(lcu, SUB_SCU(x), SUB_SCU(y));
  int tr_depth = MAX(1, depth);
  if (cur_cu->part_size != SIZE_2Nx2N) {
    tr_depth = depth + 1;
  }
  kvz_lcu_fill_trdepth(lcu, x, y, depth, tr_depth);

  const bool reconstruct_chroma = state->encoder_control->chroma_format != KVZ_CSP_400;
  kvz_inter_recon_cu(state, lcu, x, y, CU_WIDTH_FROM_DEPTH(depth), true, reconstruct_chroma);
  kvz_quantize_lcu_residual(state, true, reconstruct_chroma,
    x, y, depth,
    NULL,
    lcu,
    false);

  *inter_cost = kvz_cu_rd_cost_luma(state, SUB_SCU(x), SUB_SCU(y), depth, cur_cu, lcu);
  if (reconstruct_chroma) {
    *inter_cost += kvz_cu_rd_cost_chroma(state, SUB_SCU(x), SUB_SCU(y), depth, cur_cu, lcu);
  }

  *inter_cost += *inter_bitcost * state->lambda;
}


/**
 * \brief Update CU to have best modes at this depth.
 *
 * Only searches the 2Nx2N partition mode.
 *
 * \param state       encoder state
 * \param x           x-coordinate of the CU
 * \param y           y-coordinate of the CU
 * \param depth       depth of the CU in the quadtree
 * \param lcu         containing LCU
 *
 * \param inter_cost    Return inter cost
 * \param inter_bitcost Return inter bitcost
 */
void kvz_search_cu_inter(encoder_state_t * const state,
                         int x, int y, int depth,
                         lcu_t *lcu,
                         double   *inter_cost,
                         uint32_t *inter_bitcost)
{
  search_pu_inter(state,
                  x, y, depth,
                  SIZE_2Nx2N, 0,
                  lcu,
                  inter_cost,
                  inter_bitcost);

  // Calculate more accurate cost when needed
  if (state->encoder_control->cfg.rdo >= 2) {
    kvz_cu_cost_inter_rd2(state,
      x, y, depth,
      lcu,
      inter_cost,
      inter_bitcost);
  }
}


/**
 * \brief Update CU to have best modes at this depth.
 *
 * Only searches the given partition mode.
 *
 * \param state       encoder state
 * \param x           x-coordinate of the CU
 * \param y           y-coordinate of the CU
 * \param depth       depth of the CU in the quadtree
 * \param part_mode   partition mode to search
 * \param lcu         containing LCU
 *
 * \param inter_cost    Return inter cost
 * \param inter_bitcost Return inter bitcost
 */
void kvz_search_cu_smp(encoder_state_t * const state,
                       int x, int y,
                       int depth,
                       part_mode_t part_mode,
                       lcu_t *lcu,
                       double *inter_cost,
                       uint32_t *inter_bitcost)
{
  const int num_pu  = kvz_part_mode_num_parts[part_mode];
  const int width   = LCU_WIDTH >> depth;
  const int y_local = SUB_SCU(y);
  const int x_local = SUB_SCU(x);

  *inter_cost    = 0;
  *inter_bitcost = 0;

  for (int i = 0; i < num_pu; ++i) {
    const int x_pu      = PU_GET_X(part_mode, width, x_local, i);
    const int y_pu      = PU_GET_Y(part_mode, width, y_local, i);
    const int width_pu  = PU_GET_W(part_mode, width, i);
    const int height_pu = PU_GET_H(part_mode, width, i);
    cu_info_t *cur_pu   = LCU_GET_CU_AT_PX(lcu, x_pu, y_pu);

    cur_pu->type      = CU_INTER;
    cur_pu->part_size = part_mode;
    cur_pu->depth     = depth;
    cur_pu->qp        = state->qp;

    double cost      = MAX_INT;
    uint32_t bitcost = MAX_INT;

    search_pu_inter(state, x, y, depth, part_mode, i, lcu, &cost, &bitcost);

    if (cost >= MAX_INT) {
      // Could not find any motion vector.
      *inter_cost    = MAX_INT;
      *inter_bitcost = MAX_INT;
      return;
    }

    *inter_cost    += cost;
    *inter_bitcost += bitcost;

    for (int y = y_pu; y < y_pu + height_pu; y += SCU_WIDTH) {
      for (int x = x_pu; x < x_pu + width_pu; x += SCU_WIDTH) {
        cu_info_t *scu = LCU_GET_CU_AT_PX(lcu, x, y);
        scu->type = CU_INTER;
        scu->inter = cur_pu->inter;
      }
    }
  }

  // Calculate more accurate cost when needed
  if (state->encoder_control->cfg.rdo >= 2) {
    kvz_cu_cost_inter_rd2(state,
      x, y, depth,
      lcu,
      inter_cost,
      inter_bitcost);
  }

  // Count bits spent for coding the partition mode.
  int smp_extra_bits = 1; // horizontal or vertical
  if (state->encoder_control->cfg.amp_enable) {
    smp_extra_bits += 1; // symmetric or asymmetric
    if (part_mode != SIZE_2NxN && part_mode != SIZE_Nx2N) {
      smp_extra_bits += 1; // U,L or D,R
    }
  }
  // The transform is split for SMP and AMP blocks so we need more bits for
  // coding the CBF.
  smp_extra_bits += 6;

  *inter_cost += (state->encoder_control->cfg.rdo >= 2 ? state->lambda : state->lambda_sqrt) * smp_extra_bits;
  *inter_bitcost += smp_extra_bits;
}
