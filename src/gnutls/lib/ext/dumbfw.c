/*
 * Copyright (C) 2013-2018 Nikos Mavrogiannopoulos
 * Copyright (C) 2018 Red Hat, Inc.
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
#include "auth.h"
#include "errors.h"
#include "num.h"
#include <ext/dumbfw.h>

/* This extension adds additional padding data in the TLS client hello.
 * There is an issue with some firewalls [0] rejecting TLS client hello
 * data that are between 256 and 511 bytes, and this extension will
 * make sure that client hello isn't in this range.
 *
 * [0]. https://www.ietf.org/mail-archive/web/tls/current/msg10423.html
 */

static int _gnutls_dumbfw_send_params(gnutls_session_t session,
				    gnutls_buffer_st * extdata);

const hello_ext_entry_st ext_mod_dumbfw = {
	.name = "ClientHello Padding",
	.tls_id = 21,
	.gid = GNUTLS_EXTENSION_DUMBFW,
	.client_parse_point = GNUTLS_EXT_APPLICATION,
	.server_parse_point = GNUTLS_EXT_APPLICATION,
	.validity = GNUTLS_EXT_FLAG_TLS | GNUTLS_EXT_FLAG_CLIENT_HELLO,
	.recv_func = NULL,
	.send_func = _gnutls_dumbfw_send_params,
	.pack_func = NULL,
	.unpack_func = NULL,
	.deinit_func = NULL,
	.cannot_be_overriden = 0
};

static int
_gnutls_dumbfw_send_params(gnutls_session_t session,
			 gnutls_buffer_st * extdata)
{
	int total_size = 0, ret;
	uint8_t pad[257];
	unsigned pad_size;
	ssize_t len = extdata->length - sizeof(mbuffer_st);

	if (session->security_parameters.entity == GNUTLS_SERVER ||
	    session->internals.dumbfw == 0 ||
	    IS_DTLS(session) != 0 ||
	    (len < 256 || len >= 512)) {
		return 0;
	} else {
		/* 256 <= extdata->length < 512 */
		pad_size = 512 - len;
		memset(pad, 0, pad_size);

		ret =
		    gnutls_buffer_append_data(extdata, pad,
						       pad_size);
		if (ret < 0)
			return gnutls_assert_val(ret);

		total_size += pad_size;
	}

	return total_size;
}

