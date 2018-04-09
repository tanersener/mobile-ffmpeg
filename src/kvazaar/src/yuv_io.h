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

#ifndef YUV_IO_H_
#define YUV_IO_H_

/*
 * \file
 * \brief Functions related to reading YUV input and output.
 */

#include <stdio.h>

#include "global.h" // IWYU pragma: keep
#include "kvazaar.h"

int yuv_io_read(FILE* file,
                unsigned input_width, unsigned input_height,
                unsigned from_bitdepth, unsigned to_bitdepth,
                kvz_picture *img_out);

int yuv_io_seek(FILE* file, unsigned frames,
                unsigned input_width, unsigned input_height);

int yuv_io_write(FILE* file,
                const kvz_picture *img,
                unsigned output_width, unsigned output_height);

#endif // YUV_IO_H_
