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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>
 *
 */

#ifndef GNUTLS_LIB_AUTH_PSK_PASSWD_H
#define GNUTLS_LIB_AUTH_PSK_PASSWD_H

/* this is locally allocated. It should be freed using the provided function */
int _gnutls_psk_pwd_find_entry(gnutls_session_t,
			       const char *username, uint16_t username_len,
			       gnutls_datum_t * key);

int _gnutls_find_psk_key(gnutls_session_t session,
			 gnutls_psk_client_credentials_t cred,
			 gnutls_datum_t * username, gnutls_datum_t * key,
			 int *free);

#endif /* GNUTLS_LIB_AUTH_PSK_PASSWD_H */
