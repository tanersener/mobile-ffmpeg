#ifndef ENCODER_STATE_CTORS_DTORS_H_
#define ENCODER_STATE_CTORS_DTORS_H_
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
 * Creation and destruction of encoder_state_t.
 */

#include "global.h" // IWYU pragma: keep


// Forward declare because including the header would lead  to a cyclic
// dependency.
struct encoder_state_t;


int kvz_encoder_state_init(struct encoder_state_t * child_state, struct encoder_state_t * parent_state);
void kvz_encoder_state_finalize(struct encoder_state_t *state);


#endif // ENCODER_STATE_CTORS_DTORS_H_
