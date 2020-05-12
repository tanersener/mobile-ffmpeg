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
 * position.  It returns the number of bytes in @data.  At the end of
 * iteration, 0 is returned.
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

		if (!p) {
			// skip NULL iov entries, else we run into issues below
			iter->iov_index++;
			continue;
		}

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
			} else {
				len -= (len % iter->block_size);
				iter->iov_offset += len;
			}

			/* Return the blocks. */
			*data = p;
			return len;
		}

		/* We can complete one full block to return. */
		block_left = iter->block_size - iter->block_offset;
		if (len >= block_left) {
			memcpy(iter->block + iter->block_offset, p, block_left);
			if (len == block_left) {
				iter->iov_index++;
				iter->iov_offset = 0;
			} else
				iter->iov_offset += block_left;
			iter->block_offset = 0;

			/* Return the filled block. */
			*data = iter->block;
			return iter->block_size;
		}

		/* Not enough data for a full block, store in temp
		 * memory and continue. */
		memcpy(iter->block + iter->block_offset, p, len);
		iter->block_offset += len;
		iter->iov_index++;
		iter->iov_offset = 0;
	}

	if (iter->block_offset > 0) {
		size_t len = iter->block_offset;

		/* Return the incomplete block. */
		*data = iter->block;
		iter->block_offset = 0;
		return len;
	}

	return 0;
}

/**
 * _gnutls_iov_iter_sync:
 * @iter: the iterator
 * @data: data returned by _gnutls_iov_iter_next
 * @data_size: size of @data
 *
 * Flush the content of temp buffer (if any) to the data buffer.
 */
int
_gnutls_iov_iter_sync(struct iov_iter_st *iter, const uint8_t *data,
		      size_t data_size)
{
	size_t iov_index;
	size_t iov_offset;

	/* We didn't return the cached block. */
	if (data != iter->block)
		return 0;

	iov_index = iter->iov_index;
	iov_offset = iter->iov_offset;

	/* When syncing a cache block we walk backwards because we only have a
	 * pointer to were the block ends in the iovec, walking backwards is
	 * fine as we are always writing a full block, so the whole content
	 * is written in the right places:
	 * iovec:     |--0--|---1---|--2--|-3-|
	 * block:     |-----------------------|
	 * 1st write                      |---|
	 * 2nd write                |-----
	 * 3rd write        |-------
	 * last write |-----
	 */
	while (data_size > 0) {
		const giovec_t *iov;
		uint8_t *p;
		size_t to_write;

		while (iov_offset == 0) {
			if (unlikely(iov_index == 0))
				return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

			iov_index--;
			iov_offset = iter->iov[iov_index].iov_len;
		}

		iov = &iter->iov[iov_index];
		p = iov->iov_base;
		to_write = MIN(data_size, iov_offset);

		iov_offset -= to_write;
		data_size -= to_write;

		memcpy(p + iov_offset, &iter->block[data_size], to_write);
	}

	return 0;
}
