/*
 * Copyright (C) 2005-2012 Free Software Foundation, Inc.
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

#ifndef AUTH_PSK_H
#define AUTH_PSK_H

#include <auth.h>
#include <auth/dh_common.h>

typedef struct gnutls_psk_client_credentials_st {
	gnutls_datum_t username;
	gnutls_datum_t key;
	gnutls_psk_client_credentials_function *get_function;
} psk_client_credentials_st;

typedef struct gnutls_psk_server_credentials_st {
	char *password_file;
	/* callback function, instead of reading the
	 * password files.
	 */
	gnutls_psk_server_credentials_function *pwd_callback;

	/* For DHE_PSK */
	gnutls_dh_params_t dh_params;
	unsigned int deinit_dh_params;
	/* this callback is used to retrieve the DH or RSA
	 * parameters.
	 */
	gnutls_params_function *params_func;

	/* Identity hint. */
	char *hint;
} psk_server_cred_st;

/* these structures should not use allocated data */
typedef struct psk_auth_info_st {
	char username[MAX_USERNAME_SIZE + 1];
	dh_info_st dh;
	char hint[MAX_USERNAME_SIZE + 1];
} *psk_auth_info_t;


#ifdef ENABLE_PSK

typedef struct psk_auth_info_st psk_auth_info_st;

int
_gnutls_set_psk_session_key(gnutls_session_t session, gnutls_datum_t * key,
			    gnutls_datum_t * psk2);
int _gnutls_gen_psk_server_kx(gnutls_session_t session,
			      gnutls_buffer_st * data);
int _gnutls_gen_psk_client_kx(gnutls_session_t, gnutls_buffer_st *);

int _gnutls_find_psk_key(gnutls_session_t session,
			 gnutls_psk_client_credentials_t cred,
			 gnutls_datum_t * username, gnutls_datum_t * key,
			 int *free);

#else
#define _gnutls_set_psk_session_key(x,y,z) GNUTLS_E_UNIMPLEMENTED_FEATURE
#endif				/* ENABLE_PSK */

#endif
