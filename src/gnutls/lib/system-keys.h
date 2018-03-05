/*
 * Copyright © 2014 Red Hat, Inc.
 *
 * Author: Nikos Mavrogiannopoulos
 *
 * GnuTLS is free software; you can redistribute it and/or
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

#ifndef SYSTEM_KEYS_H
# define SYSTEM_KEYS_H

#ifdef _WIN32
# define _gnutls_system_url_is_supported(x) 1
#else
# define _gnutls_system_url_is_supported(x) 0
#endif

int
_gnutls_x509_crt_import_system_url(gnutls_x509_crt_t crt, const char *url);

int
_gnutls_privkey_import_system_url(gnutls_privkey_t pkey,
				  const char *url);

void _gnutls_system_key_deinit(void);
int _gnutls_system_key_init(void);

#endif
