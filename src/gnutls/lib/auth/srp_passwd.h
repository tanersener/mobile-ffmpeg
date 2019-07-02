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

#ifndef GNUTLS_LIB_AUTH_SRP_PASSWD_H
#define GNUTLS_LIB_AUTH_SRP_PASSWD_H

#ifdef ENABLE_SRP

typedef struct {
	char *username;

	gnutls_datum_t salt;
	gnutls_datum_t v;
	gnutls_datum_t g;
	gnutls_datum_t n;
} SRP_PWD_ENTRY;

/* this is locally allocated. It should be freed using the provided function */
int _gnutls_srp_pwd_read_entry(gnutls_session_t state, char *username,
			       SRP_PWD_ENTRY **);
void _gnutls_srp_entry_free(SRP_PWD_ENTRY * entry);
int _gnutls_sbase64_decode(char *data, size_t data_size,
			   uint8_t ** result);

#endif				/* ENABLE_SRP */

#endif /* GNUTLS_LIB_AUTH_SRP_PASSWD_H */
