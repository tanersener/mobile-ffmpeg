#ifndef SEARCH_INTER_H_
#define SEARCH_INTER_H_
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
 * \ingroup Compression
 * \file
 * Inter prediction parameter search.
 */

#include "cu.h"
#include "encoderstate.h"
#include "global.h" // IWYU pragma: keep
#include "inter.h"
#include "kvazaar.h"

#define KVZ_LUMA_FILTER_TAPS 8
#define KVZ_LUMA_FILTER_OFFSET 3
#define KVZ_CHROMA_FILTER_TAPS 4
#define KVZ_CHROMA_FILTER_OFFSET 1

 // Maximum extra width a block needs to filter 
 // a fractional pixel with positive fractional mv.x and mv.y
#define KVZ_EXT_PADDING_LUMA (KVZ_LUMA_FILTER_TAPS - 1)
#define KVZ_EXT_PADDING_CHROMA (KVZ_CHROMA_FILTER_TAPS - 1)

 // Maximum block width for extended block
#define KVZ_EXT_BLOCK_W_LUMA (LCU_WIDTH + KVZ_EXT_PADDING_LUMA)
#define KVZ_EXT_BLOCK_W_CHROMA (LCU_WIDTH_C + KVZ_EXT_PADDING_CHROMA)

enum hpel_position {
  HPEL_POS_HOR = 0,
  HPEL_POS_VER = 1,
  HPEL_POS_DIA = 2
};

typedef uint32_t kvz_mvd_cost_func(const encoder_state_t *state,
                                  int x, int y,
                                  int mv_shift,
                                  int16_t mv_cand[2][2],
                                  inter_merge_cand_t merge_cand[MRG_MAX_NUM_CANDS],
                                  int16_t num_cand,
                                  int32_t ref_idx,
                                  uint32_t *bitcost);

void kvz_search_cu_inter(encoder_state_t * const state,
                         int x, int y, int depth,
                         lcu_t *lcu,
                         double *inter_cost,
                         uint32_t *inter_bitcost);

void kvz_search_cu_smp(encoder_state_t * const state,
                       int x, int y,
                       int depth,
                       part_mode_t part_mode,
                       lcu_t *lcu,
                       double *inter_cost,
                       uint32_t *inter_bitcost);


unsigned kvz_inter_satd_cost(const encoder_state_t* state,
                             const lcu_t *lcu,
                             int x,
                             int y);

#endif // SEARCH_INTER_H_
