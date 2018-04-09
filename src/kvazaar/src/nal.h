#ifndef NAL_H_
#define NAL_H_
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

/*
 * \ingroup Bitstream
 * \file
 * Network Abstraction Layer (NAL) messages.
 */

#include "bitstream.h"
#include "global.h" // IWYU pragma: keep
#include "kvazaar.h"


#define SEI_HASH_MAX_LENGTH 16

//////////////////////////////////////////////////////////////////////////
// FUNCTIONS
void kvz_nal_write(bitstream_t * const bitstream, const uint8_t nal_type,
               const uint8_t temporal_id, const int long_start_code);
void kvz_image_checksum(const kvz_picture *im,
                      unsigned char checksum_out[][SEI_HASH_MAX_LENGTH], const uint8_t bitdepth);
void kvz_image_md5(const kvz_picture *im,
                   unsigned char checksum_out[][SEI_HASH_MAX_LENGTH],
                   const uint8_t bitdepth);



#endif
