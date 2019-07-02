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

#ifndef GNUTLS_LIB_AUTH_H
#define GNUTLS_LIB_AUTH_H

#include "str.h"

typedef struct mod_auth_st_int {
	const char *name;	/* null terminated */
	int (*gnutls_generate_server_certificate) (gnutls_session_t,
						   gnutls_buffer_st *);
	int (*gnutls_generate_client_certificate) (gnutls_session_t,
						   gnutls_buffer_st *);
	int (*gnutls_generate_server_kx) (gnutls_session_t,
					  gnutls_buffer_st *);
	int (*gnutls_generate_client_kx) (gnutls_session_t, gnutls_buffer_st *);	/* used in SRP */
	int (*gnutls_generate_client_crt_vrfy) (gnutls_session_t,
						gnutls_buffer_st *);
	int (*gnutls_generate_server_crt_request) (gnutls_session_t,
						   gnutls_buffer_st *);

	int (*gnutls_process_server_certificate) (gnutls_session_t,
						  uint8_t *, size_t);
	int (*gnutls_process_client_certificate) (gnutls_session_t,
						  uint8_t *, size_t);
	int (*gnutls_process_server_kx) (gnutls_session_t, uint8_t *,
					 size_t);
	int (*gnutls_process_client_kx) (gnutls_session_t, uint8_t *,
					 size_t);
	int (*gnutls_process_client_crt_vrfy) (gnutls_session_t, uint8_t *,
					       size_t);
	int (*gnutls_process_server_crt_request) (gnutls_session_t,
						  uint8_t *, size_t);
} mod_auth_st;

const void *_gnutls_get_cred(gnutls_session_t session,
			     gnutls_credentials_type_t type);
const void *_gnutls_get_kx_cred(gnutls_session_t session,
				gnutls_kx_algorithm_t algo);
int _gnutls_auth_info_init(gnutls_session_t session,
			  gnutls_credentials_type_t type, int size,
			  int allow_change);

/*-
 * _gnutls_get_auth_info - Returns a pointer to authentication information.
 * @session: is a #gnutls_session_t structure.
 *
 * This function must be called after a successful gnutls_handshake().
 * Returns a pointer to authentication information. That information
 * is data obtained by the handshake protocol, the key exchange algorithm,
 * and the TLS extensions messages.
 *
 * In case of GNUTLS_CRD_ANON returns a type of &anon_(server/client)_auth_info_t;
 * In case of GNUTLS_CRD_CERTIFICATE returns a type of &cert_auth_info_t;
 * In case of GNUTLS_CRD_SRP returns a type of &srp_(server/client)_auth_info_t;
 -*/
inline static
void *_gnutls_get_auth_info(gnutls_session_t session, gnutls_credentials_type_t type)
{
	if (type == session->key.auth_info_type)
		return session->key.auth_info;
	else
		return NULL;
}

#endif /* GNUTLS_LIB_AUTH_H */
