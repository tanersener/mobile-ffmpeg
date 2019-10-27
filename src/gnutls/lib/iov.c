/*
 * Copyright (C) 2019 Red Hat, Inc.
 *
 * Author: Daiki Ueno
 *
 * This file is part of GnuTLS.
 *
 * The GnuTLS is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>
 *
 */

#include "gnutls_int.h"
#include "iov.h"

/**
 * _gnutls_iov_iter_init:
 * @iter: the iterator
 * @iov: the data buffers
 * @iov_count: the number of data buffers
 * @block_size: block size to iterate
 *
 * Initialize the iterator.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise
 *   an error code is returned
 */
int
_gnutls_iov_iter_init(struct iov_iter_st *iter,
		      const giovec_t *iov, size_t iov_count,
		      size_t block_size)
{
	if (unlikely(block_size > MAX_CIPHER_BLOCK_SIZE))
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	iter->iov = iov;
	iter->iov_count = iov_count;
	iter->iov_index = 0;
	iter->iov_offset = 0;
	iter->block_size = block_size;
	iter->block_offset = 0;
	return 0;
}

/**
 * _gnutls_iov_iter_next:
 * @iter: the iterator
 * @data: the return location of extracted data
 *
 * Retrieve block(s) pointed by @iter and advance it to the next
 * position.  It returns the number of consecutive blocks in @data.
 * At the end of iteration, 0 is returned.
 *
 * If the data stored in @iter is not multiple of the block size, the
 * remaining data is stored in the "block" field of @iter with the
 * size stored in the "block_offset" field.
 *
 * Returns: On success, a value greater than or equal to zero is
 *   returned, otherwise a negative error code is returned
 */
ssize_t
_gnutls_iov_iter_next(struct iov_iter_st *iter, uint8_t **data)
{
	while (iter->iov_index < iter->iov_count) {
		const giovec_t *iov = &iter->iov[iter->iov_index];
		uint8_t *p = iov->iov_base;
		size_t len = iov->iov_len;
		size_t block_left;

		if (unlikely(len < iter->iov_offset))
			return gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET_LENGTH);
		len -= iter->iov_offset;
		p += iter->iov_offset;

		/* We have at least one full block, return a whole set
		 * of full blocks immediately. */
		if (iter->block_offset == 0 && len >= iter->block_size) {
			if ((len % iter->block_size) == 0) {
				iter->iov_index++;
				iter->iov_offset = 0;
			} else
				iter->iov_offset +=
					len - (len % iter->block_size);

			/* Return the blocks. */
			*data = p;
			return len / iter->block_size;
		}

		/* We can complete one full block to return. */
		block_left = iter->block_size - iter->block_offset;
		if (len >= block_left) {
			memcpy(iter->block + iter->block_offset, p, block_left);
			iter->iov_offset += block_left;
			iter->block_offset = 0;

			/* Return the filled block. */
			*data = iter->block;
			return 1;
		}

		/* Not enough data for a full block, store in temp
		 * memory and continue. */
		memcpy(iter->block + iter->block_offset, p, len);
		iter->block_offset += len;
		iter->iov_index++;
		iter->iov_offset = 0;
	}
	return 0;
}
