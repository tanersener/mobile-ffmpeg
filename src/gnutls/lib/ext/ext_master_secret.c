/*
 * Copyright (C) 2014 Red Hat, Inc.
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 */

/* This file contains the code for the Max Record Size TLS extension.
 */

#include "gnutls_int.h"
#include "errors.h"
#include "num.h"
#include <extensions.h>
#include <ext/ext_master_secret.h>

static int _gnutls_ext_master_secret_recv_params(gnutls_session_t session,
					  const uint8_t * data,
					  size_t data_size);
static int _gnutls_ext_master_secret_send_params(gnutls_session_t session,
					  gnutls_buffer_st * extdata);

const extension_entry_st ext_mod_ext_master_secret = {
	.name = "Extended Master Secret",
	.type = GNUTLS_EXTENSION_EXT_MASTER_SECRET,
	.parse_type = GNUTLS_EXT_MANDATORY,

	.recv_func = _gnutls_ext_master_secret_recv_params,
	.send_func = _gnutls_ext_master_secret_send_params,
	.pack_func = NULL,
	.unpack_func = NULL,
	.deinit_func = NULL
};

#ifdef ENABLE_SSL3
static inline unsigned have_only_ssl3_enabled(gnutls_session_t session)
{
	if (session->internals.priorities.protocol.algorithms == 1 &&
	    session->internals.priorities.protocol.priority[0] == GNUTLS_SSL3)
	    return 1;
	return 0;
}
#endif

/* 
 * In case of a server: if an EXT_MASTER_SECRET extension type is received then it
 * sets a flag into the session security parameters.
 *
 */
static int
_gnutls_ext_master_secret_recv_params(gnutls_session_t session,
			       const uint8_t * data, size_t _data_size)
{
	ssize_t data_size = _data_size;

	if ((session->internals.flags & GNUTLS_NO_EXTENSIONS) ||
	    session->internals.priorities.no_ext_master_secret != 0) {
		return 0;
	}

	if (data_size != 0) {
		return gnutls_assert_val(GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER);
	}

#ifdef ENABLE_SSL3
	if (session->security_parameters.entity == GNUTLS_CLIENT) {
		const version_entry_st *ver = get_version(session);

		if (unlikely(ver == NULL))
			return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

		if (ver->id != GNUTLS_SSL3)
			session->security_parameters.ext_master_secret = 1;
	/* do not enable ext master secret if SSL 3.0 is the only protocol supported by server */
	} else if (!have_only_ssl3_enabled(session))
#endif
		session->security_parameters.ext_master_secret = 1;

	return 0;
}

/* returns data_size or a negative number on failure
 */
static int
_gnutls_ext_master_secret_send_params(gnutls_session_t session,
			       gnutls_buffer_st * extdata)
{
	if ((session->internals.flags & GNUTLS_NO_EXTENSIONS) ||
	    session->internals.priorities.no_ext_master_secret != 0) {
	    session->security_parameters.ext_master_secret = 0;
	    return 0;
	}

	/* this function sends the client extension data */
#ifdef ENABLE_SSL3
	if (session->security_parameters.entity == GNUTLS_CLIENT) {
		if (have_only_ssl3_enabled(session))
		    return 0; /* this extension isn't available for SSL 3.0 */

		return GNUTLS_E_INT_RET_0;
	} else { /* server side */
		const version_entry_st *ver = get_version(session);
		if (unlikely(ver == NULL))
			return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

		if (ver->id != GNUTLS_SSL3 && session->security_parameters.ext_master_secret != 0)
			return GNUTLS_E_INT_RET_0;
	}


	return 0;
#else
	if (session->security_parameters.entity == GNUTLS_CLIENT ||
	    session->security_parameters.ext_master_secret != 0) 
		return GNUTLS_E_INT_RET_0;
	return 0;
#endif
}

/**
 * gnutls_session_ext_master_secret_status:
 * @session: is a #gnutls_session_t type.
 *
 * Get the status of the extended master secret extension negotiation.
 * This is in accordance to draft-ietf-tls-session-hash-01
 *
 * Returns: Non-zero if the negotiation was successful or zero otherwise.
 **/
unsigned gnutls_session_ext_master_secret_status(gnutls_session_t session)
{
	return session->security_parameters.ext_master_secret;
}

