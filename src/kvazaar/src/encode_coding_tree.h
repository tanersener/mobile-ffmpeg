#ifndef ENCODE_CODING_TREE_H_
#define ENCODE_CODING_TREE_H_

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
 * \file
 * Functions for writing the coding quadtree and related syntax.
 */

#include "encoderstate.h"
#include "global.h"

void kvz_encode_coding_tree(encoder_state_t * const state,
                            uint16_t x_ctb,
                            uint16_t y_ctb,
                            uint8_t depth);

void kvz_encode_mvd(encoder_state_t * const state,
                    cabac_data_t *cabac,
                    int32_t mvd_hor,
                    int32_t mvd_ver);

void kvz_encode_last_significant_xy(cabac_data_t * const cabac,
                                    uint8_t lastpos_x, uint8_t lastpos_y,
                                    uint8_t width, uint8_t height,
                                    uint8_t type, uint8_t scan);

#endif // ENCODE_CODING_TREE_H_
