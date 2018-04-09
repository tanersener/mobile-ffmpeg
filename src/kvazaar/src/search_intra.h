#ifndef SEARCH_INTRA_H_
#define SEARCH_INTRA_H_
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
 * Intra prediction parameter search.
 */

#include "cu.h"
#include "encoderstate.h"
#include "global.h" // IWYU pragma: keep


double kvz_luma_mode_bits(const encoder_state_t *state, 
                      int8_t luma_mode, const int8_t *intra_preds);
                       
double kvz_chroma_mode_bits(const encoder_state_t *state,
                        int8_t chroma_mode, int8_t luma_mode);

int8_t kvz_search_cu_intra_chroma(encoder_state_t * const state,
                              const int x_px, const int y_px,
                              const int depth, lcu_t *lcu);

void kvz_search_cu_intra(encoder_state_t * const state,
                         const int x_px, const int y_px,
                         const int depth, lcu_t *lcu,
                         int8_t *mode_out, double *cost_out);

#endif // SEARCH_INTRA_H_
