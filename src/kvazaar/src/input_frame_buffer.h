#ifndef INPUT_FRAME_BUFFER_H_
#define INPUT_FRAME_BUFFER_H_
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
 * Buffering of input for reordering.
 */

#include "global.h" // IWYU pragma: keep
#include "kvazaar.h"


// Forward declaration.
struct encoder_state_t;

typedef struct input_frame_buffer_t {
  /** \brief An array for stroring the input frames. */
  struct kvz_picture *pic_buffer[3 * KVZ_MAX_GOP_LENGTH];

  /** \brief An array for stroring the timestamps. */
  int64_t pts_buffer[3 * KVZ_MAX_GOP_LENGTH];

  /** \brief Number of pictures input. */
  uint64_t num_in;

  /** \brief Number of pictures output. */
  uint64_t num_out;

  /** \brief Value to subtract from the DTS values of the first frames.
   *
   * This will be set to the difference of the PTS values of the first and
   * (cfg->gop_len)th frames, unless the sequence has less that cfg->gop_len
   * frames.
   */
  int64_t delay;

  /** \brief Number of GOP pictures skipped.
   *
   * This is used when the last GOP of the sequence is not full.
   */
  int gop_skipped;

} input_frame_buffer_t;

void kvz_init_input_frame_buffer(input_frame_buffer_t *input_buffer);

kvz_picture* kvz_encoder_feed_frame(input_frame_buffer_t *buf,
                                    struct encoder_state_t *const state,
                                    struct kvz_picture *const img_in,
                                    int first_done);

#endif // INPUT_FRAME_BUFFER_H_
