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

#ifndef GNUTLS_LIB_IOV_H
#define GNUTLS_LIB_IOV_H

#include "gnutls_int.h"

struct iov_iter_st {
	const giovec_t *iov;
	size_t iov_count;	/* the number of iov */
	size_t iov_index;	/* index of the current buffer */
	size_t iov_offset;	/* byte offset in the current buffer */

	uint8_t block[MAX_CIPHER_BLOCK_SIZE]; /* incomplete block for reading */
	size_t block_size;	/* actual block size of the cipher */
	size_t block_offset;	/* offset in block */

};

int _gnutls_iov_iter_init(struct iov_iter_st *iter,
			  const giovec_t *iov, size_t iov_count,
			  size_t block_size);

ssize_t _gnutls_iov_iter_next(struct iov_iter_st *iter, uint8_t **data);

#endif /* GNUTLS_LIB_IOV_H */
