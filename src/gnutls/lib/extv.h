/*
 * Copyright (C) 2017 Red Hat, Inc.
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

#ifndef GNUTLS_LIB_EXTV_H
#define GNUTLS_LIB_EXTV_H

#include <gnutls/gnutls.h>
#include "str.h"

/* Iterates through all TLS-type extensions in data, and
 * calls the callback function for each of them. The ctx, flags
 * and parse_type are passed verbatim to callback. */
int _gnutls_extv_parse(void *ctx,
		       gnutls_ext_raw_process_func cb,
		       const uint8_t * data, int data_size);

inline static
int _gnutls_extv_append_init(gnutls_buffer_st *buf)
{
	unsigned pos;
	int ret;

	pos = buf->length;

	ret = _gnutls_buffer_append_prefix(buf, 16, 0);
	if (ret < 0)
		return gnutls_assert_val(ret);

	return pos;
}

/* its input is the buffer and the return value of _gnutls_extv_append_init()
 * @is_hello: should be true for client and server hello messages.
 */
inline static
int _gnutls_extv_append_final(gnutls_buffer_st *buf, unsigned init, unsigned is_hello)
{
	unsigned size = buf->length - init - 2;

	if (size > UINT16_MAX) /* sent too many extensions */
		return gnutls_assert_val(GNUTLS_E_HANDSHAKE_TOO_LARGE);

	if (size > 0)
		_gnutls_write_uint16(size, &buf->data[init]);
	else if (is_hello && size == 0) {
		/* there is no point to send empty extension bytes, and
		 * they are known to break certain clients */
		buf->length -= 2;
	}

	return 0;
}

typedef int (*extv_append_func)(void *ctx, gnutls_buffer_st *buf);

int _gnutls_extv_append(gnutls_buffer_st *buf,
			uint16_t tls_id,
		        void *ctx,
		        extv_append_func cb);


#endif /* GNUTLS_LIB_EXTV_H */
