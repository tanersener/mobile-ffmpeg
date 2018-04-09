#ifndef FILTER_H_
#define FILTER_H_
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
 * Deblocking filter.
 */

#include "encoderstate.h"
#include "global.h" // IWYU pragma: keep


/**
 * \brief Edge direction.
 */
typedef enum edge_dir {
  EDGE_VER = 0, // vertical
  EDGE_HOR = 1, // horizontal
} edge_dir;


void kvz_filter_deblock_lcu(encoder_state_t *state, int x_px, int y_px);

#endif
