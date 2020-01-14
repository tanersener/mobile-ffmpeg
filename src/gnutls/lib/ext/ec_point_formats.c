/*
 * Copyright (C) 2011-2012 Free Software Foundation, Inc.
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

/* This file contains the code for the Elliptic Curve Point Formats extension.
 */

#include "ext/ec_point_formats.h"
#include "str.h"
#include "state.h"
#include <gnutls/gnutls.h>


static int _gnutls_supported_ec_point_formats_recv_params(gnutls_session_t session,
						const uint8_t * data,
						size_t data_size);
static int _gnutls_supported_ec_point_formats_send_params(gnutls_session_t session,
						gnutls_buffer_st * extdata);


const hello_ext_entry_st ext_mod_supported_ec_point_formats = {
	.name = "Supported EC Point Formats",
	.tls_id = 11,
	.gid = GNUTLS_EXTENSION_SUPPORTED_EC_POINT_FORMATS,
	.client_parse_point = GNUTLS_EXT_TLS,
	.server_parse_point = GNUTLS_EXT_TLS,
	.validity = GNUTLS_EXT_FLAG_TLS | GNUTLS_EXT_FLAG_DTLS |
		    GNUTLS_EXT_FLAG_CLIENT_HELLO | GNUTLS_EXT_FLAG_TLS12_SERVER_HELLO,
	.recv_func = _gnutls_supported_ec_point_formats_recv_params,
	.send_func = _gnutls_supported_ec_point_formats_send_params,
	.pack_func = NULL,
	.unpack_func = NULL,
	.deinit_func = NULL
};


/* Receive point formats
 */
static int
_gnutls_supported_ec_point_formats_recv_params(gnutls_session_t session,
				     const uint8_t * data,
				     size_t data_size)
{
	size_t len, i;
	int uncompressed = 0;

	if (session->security_parameters.entity == GNUTLS_CLIENT) {
		if (data_size < 1)
			return
			    gnutls_assert_val
			    (GNUTLS_E_RECEIVED_ILLEGAL_EXTENSION);

		len = data[0];
		if (len < 1)
			return
			    gnutls_assert_val
			    (GNUTLS_E_RECEIVED_ILLEGAL_EXTENSION);

		DECR_LEN(data_size, len + 1);

		for (i = 1; i <= len; i++)
			if (data[i] == 0) {	/* uncompressed */
				uncompressed = 1;
				break;
			}

		if (uncompressed == 0)
			return
			    gnutls_assert_val
			    (GNUTLS_E_UNKNOWN_PK_ALGORITHM);
	} else {
		/* only sanity check here. We only support uncompressed points
		 * and a client must support it thus nothing to check.
		 */
		if (data_size < 1)
			return
			    gnutls_assert_val
			    (GNUTLS_E_RECEIVED_ILLEGAL_EXTENSION);
	}

	return 0;
}

/* returns data_size or a negative number on failure
 */
static int
_gnutls_supported_ec_point_formats_send_params(gnutls_session_t session,
				     gnutls_buffer_st * extdata)
{
	const uint8_t p[2] = { 0x01, 0x00 };	/* only support uncompressed point format */
	int ret;

	if (session->security_parameters.entity == GNUTLS_SERVER
	    && !_gnutls_session_is_ecc(session))
		return 0;

	if (session->internals.priorities->groups.size > 0) {
		ret = _gnutls_buffer_append_data(extdata, p, 2);
		if (ret < 0)
			return gnutls_assert_val(ret);

		return 2;
	}
	return 0;
}
