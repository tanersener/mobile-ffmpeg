/*
 * Copyright (C) 2003-2012 Free Software Foundation, Inc.
 * Copyright (C) 2002 Andrew McDonald
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

#include "gnutls_int.h"
#include <str.h>
#include <x509_int.h>
#include <common.h>
#include "errors.h"
#include <system.h>

/**
 * gnutls_x509_crt_check_email:
 * @cert: should contain an gnutls_x509_crt_t type
 * @email: A null terminated string that contains an email address (RFC822)
 * @flags: should be zero
 *
 * This function will check if the given certificate's subject matches
 * the given email address.
 *
 * Returns: non-zero for a successful match, and zero on failure.
 **/
unsigned
gnutls_x509_crt_check_email(gnutls_x509_crt_t cert,
			    const char *email, unsigned int flags)
{
	char rfc822name[MAX_CN];
	size_t rfc822namesize;
	int found_rfc822name = 0;
	int ret = 0;
	int i = 0;
	char *a_email;
	gnutls_datum_t out;

	/* convert the provided email to ACE-Labels domain. */
	ret = _gnutls_idna_email_map(email, strlen(email), &out);
	if (ret < 0) {
		_gnutls_debug_log("unable to convert email %s to IDNA format\n", email);
		a_email = (char*)email;
	} else {
		a_email = (char*)out.data;
	}

	/* try matching against:
	 *  1) an address as an alternative name (subjectAltName) extension
	 *     in the certificate
	 *  2) the EMAIL field in the certificate
	 *
	 *  only try (2) if there is no subjectAltName extension of
	 *  type RFC822Name, and there is a single EMAIL.
	 */

	/* Check through all included subjectAltName extensions, comparing
	 * against all those of type RFC822Name.
	 */
	for (i = 0; !(ret < 0); i++) {

		rfc822namesize = sizeof(rfc822name);
		ret = gnutls_x509_crt_get_subject_alt_name(cert, i,
							   rfc822name,
							   &rfc822namesize,
							   NULL);

		if (ret == GNUTLS_SAN_RFC822NAME) {
			found_rfc822name = 1;

			if (_gnutls_has_embedded_null(rfc822name, rfc822namesize)) {
				_gnutls_debug_log("certificate has %s with embedded null in rfc822name\n", rfc822name);
				continue;
			}

			if (!_gnutls_str_is_print(rfc822name, rfc822namesize)) {
				_gnutls_debug_log("invalid (non-ASCII) email in certificate %.*s\n", (int)rfc822namesize, rfc822name);
				continue;
			}

			ret = _gnutls_hostname_compare(rfc822name, rfc822namesize, a_email, GNUTLS_VERIFY_DO_NOT_ALLOW_WILDCARDS);
			if (ret != 0) {
				ret = 1;
				goto cleanup;
			}
		}
	}

	if (!found_rfc822name) {
		/* did not get the necessary extension, use CN instead
		 */

		/* enforce the RFC6125 (§1.8) requirement that only
		 * a single CN must be present */
		rfc822namesize = sizeof(rfc822name);
		ret = gnutls_x509_crt_get_dn_by_oid
			(cert, GNUTLS_OID_PKCS9_EMAIL, 1, 0, rfc822name,
			 &rfc822namesize);
		if (ret != GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE) {
			ret = 0;
			goto cleanup;
		}

		rfc822namesize = sizeof(rfc822name);
		ret = gnutls_x509_crt_get_dn_by_oid
			(cert, GNUTLS_OID_PKCS9_EMAIL, 0, 0, rfc822name,
			 &rfc822namesize);
		if (ret < 0) {
			ret = 0;
			goto cleanup;
		}

		if (_gnutls_has_embedded_null(rfc822name, rfc822namesize)) {
			_gnutls_debug_log("certificate has EMAIL %s with embedded null in name\n", rfc822name);
			ret = 0;
			goto cleanup;
		}

		if (!_gnutls_str_is_print(rfc822name, rfc822namesize)) {
			_gnutls_debug_log("invalid (non-ASCII) email in certificate DN %.*s\n", (int)rfc822namesize, rfc822name);
			ret = 0;
			goto cleanup;
		}

		ret = _gnutls_hostname_compare(rfc822name, rfc822namesize, a_email, GNUTLS_VERIFY_DO_NOT_ALLOW_WILDCARDS);
		if (ret != 0) {
			ret = 1;
			goto cleanup;
		}
	}

	/* not found a matching name
	 */
	ret = 0;
 cleanup:
	if (a_email != email) {
		gnutls_free(a_email);
	}
	return ret;
}
