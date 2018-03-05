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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 */

/* this is not to be included by gnutls_anon.c */
#include <auth.h>
#include <auth/dh_common.h>

typedef struct gnutls_anon_server_credentials_st {
	gnutls_dh_params_t dh_params;
	unsigned deinit_dh_params;

	/* this callback is used to retrieve the DH or RSA
	 * parameters.
	 */
	gnutls_params_function *params_func;
} anon_server_credentials_st;

typedef struct gnutls_anon_client_credentials_st {
	int dummy;
} anon_client_credentials_st;

typedef struct anon_auth_info_st {
	dh_info_st dh;
	gnutls_ecc_curve_t curve;
} *anon_auth_info_t;

typedef struct anon_auth_info_st anon_auth_info_st;
