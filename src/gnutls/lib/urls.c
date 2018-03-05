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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 */

#include "gnutls_int.h"
#include "errors.h"
#include "str.h"
#include "urls.h"
#include "system-keys.h"

#define MAX_CUSTOM_URLS 8

gnutls_custom_url_st _gnutls_custom_urls[MAX_CUSTOM_URLS];
unsigned _gnutls_custom_urls_size = 0;

/**
 * gnutls_url_is_supported:
 * @url: A URI to be tested
 *
 * Check whether the provided @url is supported.  Depending on the system libraries
 * GnuTLS may support pkcs11, tpmkey or other URLs.
 *
 * Returns: return non-zero if the given URL is supported, and zero if
 * it is not known.
 *
 * Since: 3.1.0
 **/
unsigned gnutls_url_is_supported(const char *url)
{
	unsigned i;

	for (i=0;i<_gnutls_custom_urls_size;i++) {
		if (strncmp(url, _gnutls_custom_urls[i].name, _gnutls_custom_urls[i].name_size) == 0)
			return 1;
	}

#ifdef ENABLE_PKCS11
	if (strncmp(url, PKCS11_URL, sizeof(PKCS11_URL)-1) == 0)
		return 1;
#endif
#ifdef HAVE_TROUSERS
	if (strncmp(url, TPMKEY_URL, sizeof(TPMKEY_URL)-1) == 0)
		return 1;
#endif
	if (strncmp(url, SYSTEM_URL, sizeof(SYSTEM_URL)-1) == 0)
		return _gnutls_system_url_is_supported(url);

	return 0;
}

int _gnutls_url_is_known(const char *url)
{
	unsigned i;

	if (strncmp(url, PKCS11_URL, sizeof(PKCS11_URL)-1) == 0)
		return 1;
	else if (strncmp(url, TPMKEY_URL, sizeof(TPMKEY_URL)-1) == 0)
		return 1;
	else if (strncmp(url, SYSTEM_URL, sizeof(SYSTEM_URL)-1) == 0)
		return 1;
	else {
		for (i=0;i<_gnutls_custom_urls_size;i++) {
			if (strncmp(url, _gnutls_custom_urls[i].name, _gnutls_custom_urls[i].name_size) == 0)
				return 1;
		}

		return 0;
	}
}

/**
 * gnutls_register_custom_url:
 * @st: A %gnutls_custom_url_st structure
 *
 * Register a custom URL. This will affect the following functions:
 * gnutls_url_is_supported(), gnutls_privkey_import_url(),
 * gnutls_pubkey_import_url, gnutls_x509_crt_import_url() 
 * and all functions that depend on
 * them, e.g., gnutls_certificate_set_x509_key_file2().
 *
 * The provided structure and callback functions must be valid throughout
 * the lifetime of the process. The registration of an existing URL type
 * will fail with %GNUTLS_E_INVALID_REQUEST. Since GnuTLS 3.5.0 this function
 * can be used to override the builtin URLs.
 *
 * This function is not thread safe.
 *
 * Returns: returns zero if the given structure was imported or a negative value otherwise.
 *
 * Since: 3.4.0
 **/
int gnutls_register_custom_url(const gnutls_custom_url_st *st)
{
	unsigned i;

	for (i=0;i<_gnutls_custom_urls_size;i++) {
		if (_gnutls_custom_urls[i].name_size == st->name_size &&
		    strcmp(_gnutls_custom_urls[i].name, st->name) == 0) {
		    return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);
		}
	}

	if (_gnutls_custom_urls_size < MAX_CUSTOM_URLS-1) {
		memcpy(&_gnutls_custom_urls[_gnutls_custom_urls_size], st, sizeof(*st));
		_gnutls_custom_urls_size++;
		return 0;
	} else {
		return gnutls_assert_val(GNUTLS_E_UNIMPLEMENTED_FEATURE);
	}
}

/*-
 * _gnutls_get_raw_issuer:
 * @url: A PKCS 11 url identifying a token
 * @cert: is the certificate to find issuer for
 * @issuer: Will hold the issuer if any in an allocated buffer.
 * @flags: Use zero or flags from %GNUTLS_PKCS11_OBJ_FLAG.
 *
 * This function will return the issuer of a given certificate in
 * DER format.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.4.0
 -*/
int _gnutls_get_raw_issuer(const char *url, gnutls_x509_crt_t cert,
				 gnutls_datum_t * issuer,
				 unsigned int flags)
{
	unsigned i;

#ifdef ENABLE_PKCS11
	if (strncmp(url, PKCS11_URL, PKCS11_URL_SIZE) == 0) {
		return gnutls_pkcs11_get_raw_issuer(url, cert, issuer, GNUTLS_X509_FMT_DER, 0);
	}
#endif
	for (i=0;i<_gnutls_custom_urls_size;i++) {
		if (strncmp(url, _gnutls_custom_urls[i].name, _gnutls_custom_urls[i].name_size) == 0) {
			if (_gnutls_custom_urls[i].get_issuer) {
				return _gnutls_custom_urls[i].get_issuer(url, cert, issuer, flags);
			}
			break;
		}
	}

	return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
}
