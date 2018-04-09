#ifndef ENCODER_STATE_BITSTREAM_H_
#define ENCODER_STATE_BITSTREAM_H_
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
 * \ingroup Bitstream
 * \file
 * Coding of HEVC bitstream elements.
 */

#include "global.h" // IWYU pragma: keep


// Forward declare because including the header would lead  to a cyclic
// dependency.
struct encoder_state_t;

struct bitstream_t;

void kvz_encoder_state_write_bitstream_slice_header(
    struct bitstream_t * const stream,
    struct encoder_state_t * const state,
    bool independent);
void kvz_encoder_state_write_bitstream(struct encoder_state_t * const state);
void kvz_encoder_state_write_bitstream_leaf(struct encoder_state_t * const state);
void kvz_encoder_state_worker_write_bitstream(void * opaque);
void kvz_encoder_state_write_parameter_sets(struct bitstream_t *stream,
                                            struct encoder_state_t * const state);

#endif // ENCODER_STATE_BITSTREAM_H_
