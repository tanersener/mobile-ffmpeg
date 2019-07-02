/*
 * Copyright (C) 2000-2012 Free Software Foundation, Inc.
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

/* This file contains the Anonymous Diffie-Hellman key exchange part of
 * the anonymous authentication. The functions here are used in the
 * handshake.
 */

#include "gnutls_int.h"

#if defined(ENABLE_ANON) && defined(ENABLE_ECDHE)

#include "auth.h"
#include "errors.h"
#include "dh.h"
#include "auth/anon.h"
#include "num.h"
#include "mpi.h"
#include <state.h>
#include <auth/ecdhe.h>
#include <ext/supported_groups.h>

static int gen_anon_ecdh_server_kx(gnutls_session_t, gnutls_buffer_st *);
static int proc_anon_ecdh_client_kx(gnutls_session_t, uint8_t *, size_t);
static int proc_anon_ecdh_server_kx(gnutls_session_t, uint8_t *, size_t);

const mod_auth_st anon_ecdh_auth_struct = {
	"ANON ECDH",
	NULL,
	NULL,
	gen_anon_ecdh_server_kx,
	_gnutls_gen_ecdh_common_client_kx,	/* this can be shared */
	NULL,
	NULL,

	NULL,
	NULL,			/* certificate */
	proc_anon_ecdh_server_kx,
	proc_anon_ecdh_client_kx,
	NULL,
	NULL
};

static int
gen_anon_ecdh_server_kx(gnutls_session_t session, gnutls_buffer_st * data)
{
	int ret;
	gnutls_anon_server_credentials_t cred;

	cred = (gnutls_anon_server_credentials_t)
	    _gnutls_get_cred(session, GNUTLS_CRD_ANON);
	if (cred == NULL) {
		gnutls_assert();
		return GNUTLS_E_INSUFFICIENT_CREDENTIALS;
	}

	if ((ret =
	     _gnutls_auth_info_init(session, GNUTLS_CRD_ANON,
				   sizeof(anon_auth_info_st), 1)) < 0) {
		gnutls_assert();
		return ret;
	}

	ret =
	    _gnutls_ecdh_common_print_server_kx(session, data,
						get_group
						(session));
	if (ret < 0) {
		gnutls_assert();
	}

	return ret;
}


static int
proc_anon_ecdh_client_kx(gnutls_session_t session, uint8_t * data,
			 size_t _data_size)
{
	gnutls_anon_server_credentials_t cred;

	cred = (gnutls_anon_server_credentials_t)
	    _gnutls_get_cred(session, GNUTLS_CRD_ANON);
	if (cred == NULL) {
		gnutls_assert();
		return GNUTLS_E_INSUFFICIENT_CREDENTIALS;
	}

	return _gnutls_proc_ecdh_common_client_kx(session, data,
						  _data_size,
						  get_group
						  (session), NULL);
}

int
proc_anon_ecdh_server_kx(gnutls_session_t session, uint8_t * data,
			 size_t _data_size)
{

	int ret;

	/* set auth_info */
	if ((ret =
	     _gnutls_auth_info_init(session, GNUTLS_CRD_ANON,
				   sizeof(anon_auth_info_st), 1)) < 0) {
		gnutls_assert();
		return ret;
	}

	ret =
	    _gnutls_proc_ecdh_common_server_kx(session, data, _data_size);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	return 0;
}

#endif				/* ENABLE_ANON */
