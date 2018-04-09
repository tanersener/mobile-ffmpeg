#ifndef STRATEGIES_NAL_H_
#define STRATEGIES_NAL_H_
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
 * \ingroup Optimization
 * \file
 * Interface for hash functions.
 */

#include "global.h" // IWYU pragma: keep
#include "kvazaar.h"
#include "nal.h"


//Function pointer to kvz_array_checksum
/**
 * \brief Calculate checksum for one color of the picture.
 * \param data Beginning of the pixel data for the picture.
 * \param height Height of the picture.
 * \param width Width of the picture.
 * \param stride Width of one row in the pixel array.
 */
typedef void (*array_checksum_func)(const kvz_pixel* data,
                                    const int height, const int width,
                                    const int stride,
                                    unsigned char checksum_out[SEI_HASH_MAX_LENGTH], const uint8_t bitdepth);
extern array_checksum_func kvz_array_checksum;
extern array_checksum_func kvz_array_md5;


int kvz_strategy_register_nal(void* opaque, uint8_t bitdepth);


#define STRATEGIES_NAL_EXPORTS \
  {"array_checksum", (void**) &kvz_array_checksum},\
  {"array_md5", (void**) &kvz_array_md5},

#endif //STRATEGIES_NAL_H_
