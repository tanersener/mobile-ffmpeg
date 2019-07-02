/*
 * Copyright (C) 2001-2012 Free Software Foundation, Inc.
 *
 * Author: Nikos Mavrogiannopoulos
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

/* contains functions that make it easier to
 * write vectors of <size|data>. The destination size
 * should be preallocated (datum.size+(bits/8))
 */

#include "gnutls_int.h"
#include <num.h>
#include <datum.h>
#include "errors.h"

int
_gnutls_set_datum(gnutls_datum_t * dat, const void *data, size_t data_size)
{
	if (data_size == 0 || data == NULL) {
		dat->data = NULL;
		dat->size = 0;
		return 0;
	}

	dat->data = gnutls_malloc(data_size);
	if (dat->data == NULL)
		return GNUTLS_E_MEMORY_ERROR;

	dat->size = data_size;
	memcpy(dat->data, data, data_size);

	return 0;
}

/* ensures that the data set are null-terminated
 * The function always returns an allocated string in @dat on success.
 */
int
_gnutls_set_strdatum(gnutls_datum_t * dat, const void *data, size_t data_size)
{
	if (data_size == 0 || data == NULL) {
		dat->data = gnutls_calloc(1, 1);
		dat->size = 0;
		return 0;
	}

	dat->data = gnutls_malloc(data_size+1);
	if (dat->data == NULL)
		return GNUTLS_E_MEMORY_ERROR;

	dat->size = data_size;
	memcpy(dat->data, data, data_size);
	dat->data[data_size] = 0;

	return 0;
}

