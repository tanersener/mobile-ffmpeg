#ifndef INTER_H_
#define INTER_H_
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
 * \ingroup Reconstruction
 * \file
 * Inter prediction.
 */

#include "cu.h"
#include "encoderstate.h"
#include "global.h" // IWYU pragma: keep
#include "image.h"
#include "kvazaar.h"


typedef struct {
  uint8_t dir;
  uint8_t ref[2]; // index to L0/L1
  int16_t mv[2][2];

} inter_merge_cand_t;

void kvz_inter_recon_cu(const encoder_state_t * const state,
                        lcu_t *lcu,
                        int32_t x,
                        int32_t y,
                        int32_t width,
                        bool predict_luma,
                        bool predict_chroma);

void kvz_inter_pred_pu(const encoder_state_t * const state,
  lcu_t *lcu,
  int32_t x,
  int32_t y,
  int32_t width,
  bool predict_luma,
  bool predict_chroma,
  int i_pu);

void kvz_inter_recon_bipred(const encoder_state_t * const state,
                            const kvz_picture * ref1,
                            const kvz_picture * ref2,
                            int32_t xpos,
                            int32_t ypos,
                            int32_t width,
                            int32_t height,
                            int16_t mv_param[2][2],
                            lcu_t* lcu,
                            bool predict_luma,
                            bool predict_chroma);


void kvz_inter_get_mv_cand(const encoder_state_t * const state,
                           int32_t x,
                           int32_t y,
                           int32_t width,
                           int32_t height,
                           int16_t mv_cand[2][2],
                           cu_info_t* cur_cu,
                           lcu_t *lcu,
                           int8_t reflist);

void kvz_inter_get_mv_cand_cua(const encoder_state_t * const state,
                               int32_t x,
                               int32_t y,
                               int32_t width,
                               int32_t height,
                               int16_t mv_cand[2][2],
                               const cu_info_t* cur_cu,
                               int8_t reflist);

uint8_t kvz_inter_get_merge_cand(const encoder_state_t * const state,
                                 int32_t x, int32_t y,
                                 int32_t width, int32_t height,
                                 bool use_a1, bool use_b1,
                                 inter_merge_cand_t mv_cand[MRG_MAX_NUM_CANDS],
                                 lcu_t *lcu);
#endif
