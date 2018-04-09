#ifndef RATE_CONTROL_H_
#define RATE_CONTROL_H_
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
 * \brief Functions related to rate control.
 */

#include "global.h" // IWYU pragma: keep

#include "encoderstate.h"

void kvz_set_picture_lambda_and_qp(encoder_state_t * const state);

void kvz_set_lcu_lambda_and_qp(encoder_state_t * const state,
                               vector2d_t pos);

#endif // RATE_CONTROL_H_
