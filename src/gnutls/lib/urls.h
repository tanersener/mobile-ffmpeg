/*
 * Copyright © 2014 Nikos Mavrogiannopoulos
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>
 *
 */

#ifndef GNUTLS_LIB_URLS_H
#define GNUTLS_LIB_URLS_H

#define PKCS11_URL "pkcs11:"
#define SYSTEM_URL "system:"
#define TPMKEY_URL "tpmkey:"

#define PKCS11_URL_SIZE (sizeof(PKCS11_URL)-1)
#define SYSTEM_URL_SIZE (sizeof(SYSTEM_URL)-1)
#define TPMKEY_URL_SIZE (sizeof(TPMKEY_URL)-1)

#include <gnutls/urls.h>

extern gnutls_custom_url_st _gnutls_custom_urls[];
extern unsigned _gnutls_custom_urls_size;

int _gnutls_url_is_known(const char *url);

int _gnutls_get_raw_issuer(const char *url, gnutls_x509_crt_t cert,
				 gnutls_datum_t * issuer,
				 unsigned int flags);

#endif /* GNUTLS_LIB_URLS_H */
