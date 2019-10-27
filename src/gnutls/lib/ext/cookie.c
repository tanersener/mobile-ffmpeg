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

/* This file contains the code for the Max Record Size TLS extension.
 */

#include "gnutls_int.h"
#include "errors.h"
#include "num.h"
#include "hello_ext_lib.h"
#include <ext/cookie.h>

static int cookie_recv_params(gnutls_session_t session,
					  const uint8_t * data,
					  size_t data_size);
static int cookie_send_params(gnutls_session_t session,
					  gnutls_buffer_st * extdata);

const hello_ext_entry_st ext_mod_cookie = {
	.name = "Cookie",
	.tls_id = 44,
	.gid = GNUTLS_EXTENSION_COOKIE,
	.validity = GNUTLS_EXT_FLAG_TLS | GNUTLS_EXT_FLAG_CLIENT_HELLO |
		    GNUTLS_EXT_FLAG_HRR | GNUTLS_EXT_FLAG_IGNORE_CLIENT_REQUEST,
	.parse_type = GNUTLS_EXT_MANDATORY, /* force parsing prior to EXT_TLS extensions */
	.recv_func = cookie_recv_params,
	.send_func = cookie_send_params,
	.pack_func = NULL,
	.unpack_func = NULL,
	.deinit_func = _gnutls_hello_ext_default_deinit,
	.cannot_be_overriden = 0
};

/* Only client sends this extension. */
static int
cookie_recv_params(gnutls_session_t session,
		   const uint8_t * data, size_t data_size)
{
	size_t csize;
	int ret;
	gnutls_datum_t tmp;

	if (session->security_parameters.entity == GNUTLS_SERVER) {
		/* we don't support it */
		return 0;
	} else { /* client */
		if (_gnutls_ext_get_msg(session) == GNUTLS_EXT_FLAG_HRR) {
			DECR_LEN(data_size, 2);

			csize = _gnutls_read_uint16(data);
			data += 2;

			DECR_LEN(data_size, csize);

			if (data_size != 0)
				return gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET_LENGTH);

			tmp.data = (void*)data;
			tmp.size = csize;

			ret = _gnutls_hello_ext_set_datum(session, GNUTLS_EXTENSION_COOKIE, &tmp);
			if (ret < 0)
				return gnutls_assert_val(ret);

			return 0;
		}

		return gnutls_assert_val(GNUTLS_E_RECEIVED_ILLEGAL_EXTENSION);
	}

	return 0;
}

/* returns data_size or a negative number on failure
 */
static int
cookie_send_params(gnutls_session_t session,
		   gnutls_buffer_st * extdata)
{
	gnutls_datum_t tmp;
	int ret;

	/* this function sends the client extension data (dnsname) */
	if (session->security_parameters.entity == GNUTLS_CLIENT) {
		ret = _gnutls_hello_ext_get_datum(session, GNUTLS_EXTENSION_COOKIE, &tmp);
		if (ret < 0)
			return 0;

		ret = _gnutls_buffer_append_data_prefix(extdata, 16, tmp.data, tmp.size);
		if (ret < 0)
			return gnutls_assert_val(ret);

		return 0;
	}

	return 0;
}
