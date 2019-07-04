/*
 * Copyright (C) 2017-2018 Red Hat, Inc.
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

#include "gnutls_int.h"
#include "hello_ext.h"
#include "errors.h"
#include "extv.h"

/* Iterates through all extensions found, and calls the cb()
 * function with their data */
int _gnutls_extv_parse(void *ctx,
		       gnutls_ext_raw_process_func cb,
		       const uint8_t * data, int data_size)
{
	int next, ret;
	int pos = 0;
	uint16_t tls_id;
	const uint8_t *sdata;
	uint16_t size;

	if (data_size == 0)
		return 0;

	DECR_LENGTH_RET(data_size, 2, GNUTLS_E_UNEXPECTED_EXTENSIONS_LENGTH);
	next = _gnutls_read_uint16(data);
	pos += 2;

	DECR_LENGTH_RET(data_size, next, GNUTLS_E_UNEXPECTED_EXTENSIONS_LENGTH);

	if (next == 0 && data_size == 0) /* field is present, but has zero length? Ignore it. */
		return 0;
	else if (data_size > 0) /* forbid unaccounted data */
		return gnutls_assert_val(GNUTLS_E_UNEXPECTED_EXTENSIONS_LENGTH);

	do {
		DECR_LENGTH_RET(next, 2, GNUTLS_E_UNEXPECTED_EXTENSIONS_LENGTH);
		tls_id = _gnutls_read_uint16(&data[pos]);
		pos += 2;

		DECR_LENGTH_RET(next, 2, GNUTLS_E_UNEXPECTED_EXTENSIONS_LENGTH);
		size = _gnutls_read_uint16(&data[pos]);
		pos += 2;

		DECR_LENGTH_RET(next, size, GNUTLS_E_UNEXPECTED_EXTENSIONS_LENGTH);
		sdata = &data[pos];
		pos += size;

		ret = cb(ctx, tls_id, sdata, size);
		if (ret < 0)
			return gnutls_assert_val(ret);
	}
	while (next > 2);

	/* forbid leftovers */
	if (next > 0)
		return gnutls_assert_val(GNUTLS_E_UNEXPECTED_EXTENSIONS_LENGTH);

	return 0;
}

#define HANDSHAKE_SESSION_ID_POS (34)
/**
 * gnutls_ext_raw_parse:
 * @ctx: a pointer to pass to callback function
 * @cb: callback function to process each extension found
 * @data: TLS extension data
 * @flags: should be zero or %GNUTLS_EXT_RAW_FLAG_TLS_CLIENT_HELLO or %GNUTLS_EXT_RAW_FLAG_DTLS_CLIENT_HELLO
 *
 * This function iterates through the TLS extensions as passed in
 * @data, passing the individual extension data to callback. The
 * @data must conform to Extension extensions<0..2^16-1> format.
 *
 * If flags is %GNUTLS_EXT_RAW_TLS_FLAG_CLIENT_HELLO then this function
 * will parse the extension data from the position, as if the packet in
 * @data is a client hello (without record or handshake headers) -
 * as provided by gnutls_handshake_set_hook_function().
 *
 * The return value of the callback will be propagated.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, or an error code. On unknown
 *   flags it returns %GNUTLS_E_INVALID_REQUEST.
 *
 * Since: 3.6.3
 **/
int gnutls_ext_raw_parse(void *ctx, gnutls_ext_raw_process_func cb,
			 const gnutls_datum_t *data, unsigned int flags)
{
	if (flags & GNUTLS_EXT_RAW_FLAG_TLS_CLIENT_HELLO) {
		ssize_t size = data->size;
		size_t len;
		uint8_t *p = data->data;

		DECR_LEN(size, HANDSHAKE_SESSION_ID_POS);

		if (p[0] != 0x03)
			return gnutls_assert_val(GNUTLS_E_UNSUPPORTED_VERSION_PACKET);

		p += HANDSHAKE_SESSION_ID_POS;

		/* skip session id */
		DECR_LEN(size, 1);
		len = p[0];
		p++;
		DECR_LEN(size, len);
		p += len;

		/* CipherSuites */
		DECR_LEN(size, 2);
		len = _gnutls_read_uint16(p);
		p += 2;
		DECR_LEN(size, len);
		p += len;

		/* legacy_compression_methods */
		DECR_LEN(size, 1);
		len = p[0];
		p++;
		DECR_LEN(size, len);
		p += len;

		if (size <= 0)
			return gnutls_assert_val(GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE);

		return _gnutls_extv_parse(ctx, cb, p, size);
	} else if (flags & GNUTLS_EXT_RAW_FLAG_DTLS_CLIENT_HELLO) {
		ssize_t size = data->size;
		size_t len;
		uint8_t *p = data->data;

		DECR_LEN(size, HANDSHAKE_SESSION_ID_POS);

		if (p[0] != 254)
			return gnutls_assert_val(GNUTLS_E_UNSUPPORTED_VERSION_PACKET);

		p += HANDSHAKE_SESSION_ID_POS;

		/* skip session id */
		DECR_LEN(size, 1);
		len = p[0];
		p++;
		DECR_LEN(size, len);
		p += len;

		/* skip cookie */
		DECR_LEN(size, 1);
		len = p[0];
		p++;
		DECR_LEN(size, len);
		p += len;

		/* CipherSuites */
		DECR_LEN(size, 2);
		len = _gnutls_read_uint16(p);
		p += 2;
		DECR_LEN(size, len);
		p += len;

		/* legacy_compression_methods */
		DECR_LEN(size, 1);
		len = p[0];
		p++;
		DECR_LEN(size, len);
		p += len;

		if (size <= 0)
			return gnutls_assert_val(GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE);

		return _gnutls_extv_parse(ctx, cb, p, size);
	}

	if (flags != 0)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	return _gnutls_extv_parse(ctx, cb, data->data, data->size);
}

/* Returns:
 *  * On success the number of bytes appended (always positive), or zero if not sent
 *  * On failure, a negative error code.
 */
int _gnutls_extv_append(gnutls_buffer_st *buf,
			uint16_t tls_id,
		        void *ctx,
		        int (*cb)(void *ctx, gnutls_buffer_st *buf))
{
	int size_pos, appended, ret;
	size_t size_prev;

	ret = _gnutls_buffer_append_prefix(buf, 16, tls_id);
	if (ret < 0)
		return gnutls_assert_val(ret);

	size_pos = buf->length;
	ret = _gnutls_buffer_append_prefix(buf, 16, 0);
	if (ret < 0)
		return gnutls_assert_val(ret);

	size_prev = buf->length;
	ret = cb(ctx, buf);
	if (ret < 0 && ret != GNUTLS_E_INT_RET_0) {
		return gnutls_assert_val(ret);
	}

	/* returning GNUTLS_E_INT_RET_0 means to send an empty
	 * extension of this type.
	 */
	appended = buf->length - size_prev;

	if (appended > 0 || ret == GNUTLS_E_INT_RET_0) {
		if (ret == GNUTLS_E_INT_RET_0)
			appended = 0;

		/* write the real size */
		_gnutls_write_uint16(appended,
				     &buf->data[size_pos]);
	} else if (appended == 0) {
		buf->length -= 4;	/* reset type and size */
		return 0;
	}

	return appended + 4;
}

