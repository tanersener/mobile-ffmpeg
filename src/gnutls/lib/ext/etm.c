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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>
 *
 */

/* This file contains the code for the Max Record Size TLS extension.
 */

#include "gnutls_int.h"
#include "errors.h"
#include "num.h"
#include <hello_ext.h>
#include <ext/etm.h>

static int _gnutls_ext_etm_recv_params(gnutls_session_t session,
					  const uint8_t * data,
					  size_t data_size);
static int _gnutls_ext_etm_send_params(gnutls_session_t session,
					  gnutls_buffer_st * extdata);

const hello_ext_entry_st ext_mod_etm = {
	.name = "Encrypt-then-MAC",
	.tls_id = 22,
	.gid = GNUTLS_EXTENSION_ETM,
	.parse_type = GNUTLS_EXT_MANDATORY,
	.validity = GNUTLS_EXT_FLAG_TLS | GNUTLS_EXT_FLAG_DTLS | GNUTLS_EXT_FLAG_CLIENT_HELLO |
		    GNUTLS_EXT_FLAG_TLS12_SERVER_HELLO,
	.recv_func = _gnutls_ext_etm_recv_params,
	.send_func = _gnutls_ext_etm_send_params,
	.pack_func = NULL,
	.unpack_func = NULL,
	.deinit_func = NULL,
	.cannot_be_overriden = 1
};

/* 
 * In case of a server: if an EXT_MASTER_SECRET extension type is received then it
 * sets a flag into the session security parameters.
 *
 */
static int
_gnutls_ext_etm_recv_params(gnutls_session_t session,
			       const uint8_t * data, size_t _data_size)
{
	ssize_t data_size = _data_size;

	if (data_size != 0) {
		return gnutls_assert_val(GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER);
	}

	if (session->security_parameters.entity == GNUTLS_SERVER) {
		gnutls_ext_priv_data_t epriv;

		if (session->internals.no_etm != 0)
			return 0;

		epriv = (void*)(intptr_t)1;
		_gnutls_hello_ext_set_priv(session,
						   GNUTLS_EXTENSION_ETM,
						   epriv);

		/* don't decide now, decide on send */
		return 0;
	} else { /* client */
		const gnutls_cipher_suite_entry_st *e = 
			session->security_parameters.cs;
		if (e != NULL) {
			const cipher_entry_st *c;
			c = cipher_to_entry(e->block_algorithm);
			if (c == NULL || (c->type == CIPHER_AEAD || c->type == CIPHER_STREAM))
				return 0;

			session->security_parameters.etm = 1;
		}
	}

	return 0;
}

/* returns data_size or a negative number on failure
 */
static int
_gnutls_ext_etm_send_params(gnutls_session_t session,
			       gnutls_buffer_st * extdata)
{
	if (session->internals.no_etm != 0)
		return 0;

	/* this function sends the client extension data */
	if (session->security_parameters.entity == GNUTLS_CLIENT) {
		if (session->internals.priorities->have_cbc != 0)
			return GNUTLS_E_INT_RET_0;
		else
			return 0;
	} else { /* server side */
		const gnutls_cipher_suite_entry_st *e;
		const cipher_entry_st *c;
		int ret;
		gnutls_ext_priv_data_t epriv;

		e = session->security_parameters.cs;
		if (e != NULL) {
			c = cipher_to_entry(e->block_algorithm);
			if (c == NULL || (c->type == CIPHER_AEAD || c->type == CIPHER_STREAM))
				return 0;

			ret = _gnutls_hello_ext_get_priv(session,
							   GNUTLS_EXTENSION_ETM,
							   &epriv);
			if (ret < 0 || ((intptr_t)epriv) == 0)
				return 0;

			session->security_parameters.etm = 1;
			return GNUTLS_E_INT_RET_0;
		}
	}

	return 0;
}

/**
 * gnutls_session_etm_status:
 * @session: is a #gnutls_session_t type.
 *
 * Get the status of the encrypt-then-mac extension negotiation.
 * This is in accordance to rfc7366
 *
 * Returns: Non-zero if the negotiation was successful or zero otherwise.
 **/
unsigned gnutls_session_etm_status(gnutls_session_t session)
{
	return session->security_parameters.etm;
}
