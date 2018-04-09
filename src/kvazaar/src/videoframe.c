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

#include "videoframe.h"

#include <stdlib.h>

#include "image.h"
#include "sao.h"


/**
 * \brief Allocate new frame
 * \param pic picture pointer
 * \return picture pointer
 */
videoframe_t * kvz_videoframe_alloc(int32_t width,
                                    int32_t height,
                                    enum kvz_chroma_format chroma_format)
{
  videoframe_t *frame = calloc(1, sizeof(videoframe_t));
  if (!frame) return 0;

  frame->width  = width;
  frame->height = height;
  frame->width_in_lcu  = CEILDIV(frame->width,  LCU_WIDTH);
  frame->height_in_lcu = CEILDIV(frame->height, LCU_WIDTH);

  frame->sao_luma = MALLOC(sao_info_t, frame->width_in_lcu * frame->height_in_lcu);
  if (chroma_format != KVZ_CSP_400) {
    frame->sao_chroma = MALLOC(sao_info_t, frame->width_in_lcu * frame->height_in_lcu);
  }

  return frame;
}

/**
 * \brief Free memory allocated to frame
 * \param pic picture pointer
 * \return 1 on success, 0 on failure
 */
int kvz_videoframe_free(videoframe_t * const frame)
{
  kvz_image_free(frame->source);
  frame->source = NULL;
  kvz_image_free(frame->rec);
  frame->rec = NULL;

  kvz_cu_array_free(&frame->cu_array);

  FREE_POINTER(frame->sao_luma);
  FREE_POINTER(frame->sao_chroma);

  free(frame);

  return 1;
}

void kvz_videoframe_set_poc(videoframe_t * const frame, const int32_t poc) {
  frame->poc = poc;
}
