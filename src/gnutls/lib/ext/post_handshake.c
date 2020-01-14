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

/* This file contains the code for the Post-Handshake TLS 1.3 extension.
 */

#include "gnutls_int.h"
#include "errors.h"
#include "num.h"
#include <hello_ext.h>
#include <ext/post_handshake.h>
#include "auth/cert.h"

static int _gnutls_post_handshake_recv_params(gnutls_session_t session,
					  const uint8_t * data,
					  size_t data_size);
static int _gnutls_post_handshake_send_params(gnutls_session_t session,
					  gnutls_buffer_st * extdata);

const hello_ext_entry_st ext_mod_post_handshake = {
	.name = "Post Handshake Auth",
	.tls_id = 49,
	.gid = GNUTLS_EXTENSION_POST_HANDSHAKE,
	.client_parse_point = GNUTLS_EXT_TLS,
	.server_parse_point = GNUTLS_EXT_TLS,
	.validity = GNUTLS_EXT_FLAG_TLS | GNUTLS_EXT_FLAG_CLIENT_HELLO,
	.recv_func = _gnutls_post_handshake_recv_params,
	.send_func = _gnutls_post_handshake_send_params,
	.pack_func = NULL,
	.unpack_func = NULL,
	.deinit_func = NULL,
	.cannot_be_overriden = 1
};

static int
_gnutls_post_handshake_recv_params(gnutls_session_t session,
			       const uint8_t * data, size_t _data_size)
{
	const version_entry_st *vers;

	if (session->security_parameters.entity == GNUTLS_SERVER) {
		vers = get_version(session);
		if (unlikely(vers == NULL))
			return 0;

		if ((session->internals.flags & GNUTLS_POST_HANDSHAKE_AUTH) &&
		    vers->post_handshake_auth)
			session->security_parameters.post_handshake_auth = 1;
	}

	return 0;
}

/* returns data_size or a negative number on failure
 */
static int
_gnutls_post_handshake_send_params(gnutls_session_t session,
			       gnutls_buffer_st * extdata)
{
	gnutls_certificate_credentials_t cred;
	const version_entry_st *max;

	if (session->security_parameters.entity != GNUTLS_CLIENT ||
	    !(session->internals.flags & GNUTLS_POST_HANDSHAKE_AUTH)) {
		/* not sent on server side */
		return 0;
	}

	cred = (gnutls_certificate_credentials_t)
	    _gnutls_get_cred(session, GNUTLS_CRD_CERTIFICATE);
	if (cred == NULL)	/* no certificate authentication */
		return gnutls_assert_val(0);

	max = _gnutls_version_max(session);
	if (unlikely(max == NULL))
		return gnutls_assert_val(0);

	if (max->post_handshake_auth)
		return GNUTLS_E_INT_RET_0;
	else
		return 0;
}
