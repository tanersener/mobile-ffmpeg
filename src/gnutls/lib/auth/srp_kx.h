/*
 * Copyright (C) 2001-2012 Free Software Foundation, Inc.
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

#ifndef GNUTLS_LIB_AUTH_SRP_KX_H
#define GNUTLS_LIB_AUTH_SRP_KX_H

#include <auth.h>

typedef struct gnutls_srp_client_credentials_st {
	char *username;
	char *password;
	gnutls_srp_client_credentials_function *get_function;
} srp_client_credentials_st;

typedef struct gnutls_srp_server_credentials_st {
	char *password_file;
	char *password_conf_file;
	/* callback function, instead of reading the
	 * password files.
	 */
	gnutls_srp_server_credentials_function *pwd_callback;
	gnutls_datum_t fake_salt_seed;
	unsigned int fake_salt_length;
} srp_server_cred_st;

/* these structures should not use allocated data */
typedef struct srp_server_auth_info_st {
	char username[MAX_USERNAME_SIZE + 1];
} *srp_server_auth_info_t;

#ifdef ENABLE_SRP

int _gnutls_proc_srp_server_hello(gnutls_session_t state,
				  const uint8_t * data, size_t data_size);
int _gnutls_gen_srp_server_hello(gnutls_session_t state, uint8_t * data,
				 size_t data_size);

int _gnutls_gen_srp_server_kx(gnutls_session_t, gnutls_buffer_st *);
int _gnutls_gen_srp_client_kx(gnutls_session_t, gnutls_buffer_st *);

int _gnutls_proc_srp_server_kx(gnutls_session_t, uint8_t *, size_t);
int _gnutls_proc_srp_client_kx(gnutls_session_t, uint8_t *, size_t);

typedef struct srp_server_auth_info_st srp_server_auth_info_st;

/* MAC algorithm used to generate fake salts for unknown usernames
 */
#define SRP_FAKE_SALT_MAC GNUTLS_MAC_SHA1

#endif				/* ENABLE_SRP */

#endif /* GNUTLS_LIB_AUTH_SRP_KX_H */
