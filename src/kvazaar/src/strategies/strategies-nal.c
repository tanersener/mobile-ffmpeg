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

#include "strategies/strategies-nal.h"

#include "strategies/generic/nal-generic.h"


void (*kvz_array_checksum)(const kvz_pixel* data,
                       const int height, const int width,
                       const int stride,
                       unsigned char checksum_out[SEI_HASH_MAX_LENGTH], const uint8_t bitdepth);
void (*kvz_array_md5)(const kvz_pixel* data,
                      const int height, const int width,
                      const int stride,
                      unsigned char checksum_out[SEI_HASH_MAX_LENGTH], const uint8_t bitdepth);


int kvz_strategy_register_nal(void* opaque, uint8_t bitdepth) {
  bool success = true;

  success &= kvz_strategy_register_nal_generic(opaque, bitdepth);
  
  return success;
}
