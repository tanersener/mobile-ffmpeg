#ifndef ENCODER_STATE_GEOMETRY_H_
#define ENCODER_STATE_GEOMETRY_H_
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
 * \ingroup Control
 * \file
 * Helper functions for tiles and slices.
 */

#include "global.h" // IWYU pragma: keep


// Forward declare because including the header would lead  to a cyclic
// dependency.
struct encoder_control_t;
struct encoder_state_t;


int kvz_lcu_at_slice_start(const struct  encoder_control_t * const encoder, int lcu_addr_in_ts);
int kvz_lcu_at_slice_end(const struct  encoder_control_t * const encoder, int lcu_addr_in_ts);
int kvz_lcu_at_tile_start(const struct  encoder_control_t * const encoder, int lcu_addr_in_ts);
int kvz_lcu_at_tile_end(const struct  encoder_control_t * const encoder, int lcu_addr_in_ts);
int kvz_lcu_in_first_row(const struct encoder_state_t * const encoder_state, int lcu_addr_in_ts);
int kvz_lcu_in_last_row(const struct encoder_state_t * const encoder_state, int lcu_addr_in_ts);
int kvz_lcu_in_first_column(const struct encoder_state_t * const encoder_state, int lcu_addr_in_ts);
int kvz_lcu_in_last_column(const struct encoder_state_t * const encoder_state, int lcu_addr_in_ts);


#endif // ENCODER_STATE_GEOMETRY_H_
