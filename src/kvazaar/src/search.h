#ifndef SEARCH_H_
#define SEARCH_H_
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
 * \brief Compression of a single coding tree unit (CTU).
 */

#include "cu.h"
#include "encoderstate.h"
#include "global.h" // IWYU pragma: keep
#include "image.h"
#include "constraint.h"

void kvz_sort_modes(int8_t *__restrict modes, double *__restrict costs, uint8_t length);

void kvz_search_lcu(encoder_state_t *state, int x, int y, const yuv_t *hor_buf, const yuv_t *ver_buf);

double kvz_cu_rd_cost_luma(const encoder_state_t *const state,
                       const int x_px, const int y_px, const int depth,
                       const cu_info_t *const pred_cu,
                       lcu_t *const lcu);
double kvz_cu_rd_cost_chroma(const encoder_state_t *const state,
                         const int x_px, const int y_px, const int depth,
                         const cu_info_t *const pred_cu,
                         lcu_t *const lcu);
void kvz_lcu_fill_trdepth(lcu_t *lcu, int x_px, int y_px, int depth, int tr_depth);

void kvz_intra_recon_lcu_luma(encoder_state_t * const state, int x, int y, int depth, int8_t intra_mode, cu_info_t *cur_cu, lcu_t *lcu);
void kvz_intra_recon_lcu_chroma(encoder_state_t * const state, int x, int y, int depth, int8_t intra_mode, cu_info_t *cur_cu, lcu_t *lcu);

#endif
