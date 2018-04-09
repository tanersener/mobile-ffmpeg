#ifndef BITSTREAM_H_
#define BITSTREAM_H_
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
 * \ingroup CABAC
 * \file
 * Appending bits into an Annex-B coded bitstream.
 */

#include "global.h" // IWYU pragma: keep

#include "kvazaar.h"


/**
 * A stream of bits.
 */
typedef struct bitstream_t
{
  /// \brief Total number of complete bytes.
  uint32_t len;

  /// \brief Pointer to the first chunk, or NULL.
  kvz_data_chunk *first;

  /// \brief Pointer to the last chunk, or NULL.
  kvz_data_chunk *last;

  /// \brief The incomplete byte.
  uint8_t data;

  /// \brief Number of bits in the incomplete byte.
  uint8_t cur_bit;

  uint8_t zerocount;
} bitstream_t;

typedef struct
{
  uint8_t len;
  uint32_t value;
} bit_table_t;

void kvz_bitstream_init(bitstream_t * stream);
kvz_data_chunk * kvz_bitstream_alloc_chunk();
kvz_data_chunk * kvz_bitstream_take_chunks(bitstream_t *stream);
void kvz_bitstream_free_chunks(kvz_data_chunk *chunk);
void kvz_bitstream_finalize(bitstream_t * stream);

uint64_t kvz_bitstream_tell(const bitstream_t * stream);

void kvz_bitstream_writebyte(bitstream_t *stream, uint8_t byte);
void kvz_bitstream_move(bitstream_t *dst, bitstream_t *src);
void kvz_bitstream_clear(bitstream_t *stream);

void kvz_bitstream_put(bitstream_t *stream, uint32_t data, uint8_t bits);
void kvz_bitstream_put_byte(bitstream_t *const stream, const uint32_t data);

void kvz_bitstream_put_ue(bitstream_t *stream, uint32_t data);
void kvz_bitstream_put_se(bitstream_t *stream, int32_t data);

void kvz_bitstream_add_rbsp_trailing_bits(bitstream_t *stream);
void kvz_bitstream_align(bitstream_t *stream);
void kvz_bitstream_align_zero(bitstream_t *stream);

/* In debug mode print out some extra info */
#ifdef KVZ_DEBUG_PRINT_CABAC
/* Counter to keep up with bits written */
#define WRITE_U(stream, data, bits, name) { printf("%-40s u(%d) : %d\n", name,bits,data); kvz_bitstream_put(stream,data,bits);}
#define WRITE_UE(stream, data, name) { printf("%-40s ue(v): %d\n", name,data); kvz_bitstream_put_ue(stream,data);}
#define WRITE_SE(stream, data, name) { printf("%-40s se(v): %d\n", name,data); kvz_bitstream_put_se(stream,(data));}
#else
#define WRITE_U(stream, data, bits, name) { kvz_bitstream_put(stream,data,bits); }
#define WRITE_UE(stream, data, name) { kvz_bitstream_put_ue(stream,data); }
#define WRITE_SE(stream, data, name) { kvz_bitstream_put_se(stream,data); }
#endif


#endif
