/*
 * Copyright (C) 2002-2012 Free Software Foundation, Inc.
 *
 * Author: Nikos Mavrogiannopoulos
 *
 * This file is part of GnuTLS-EXTRA.
 *
 * GnuTLS-extra is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *               
 * GnuTLS-extra is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *                               
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* This file includes all functions that were in the 0.5.x and 0.8.x
 * gnutls API. They are now implemented over the new certificate parsing
 * API.
 */

#include "gnutls_int.h"

#include <string.h>		/* memset */
#include <x509/x509_int.h>
#include <libtasn1.h>
#include <gnutls/x509.h>
#include <openssl_compat.h>

/*-
 * gnutls_x509_extract_certificate_dn:
 * @cert: should contain an X.509 DER encoded certificate
 * @ret: a pointer to a structure to hold the peer's name
 *
 * This function will return the name of the certificate holder. The name is gnutls_x509_dn structure and
 * is a obtained by the peer's certificate. If the certificate send by the
 * peer is invalid, or in any other failure this function returns error.
 * Returns a negative error code in case of an error.
 -*/
int
gnutls_x509_extract_certificate_dn(const gnutls_datum_t * cert,
				   gnutls_x509_dn * ret)
{
	gnutls_x509_crt_t xcert;
	int result;
	size_t len;

	result = gnutls_x509_crt_init(&xcert);
	if (result < 0)
		return result;

	result = gnutls_x509_crt_import(xcert, cert, GNUTLS_X509_FMT_DER);
	if (result < 0) {
		gnutls_x509_crt_deinit(xcert);
		return result;
	}

	len = sizeof(ret->country);
	gnutls_x509_crt_get_dn_by_oid(xcert, GNUTLS_OID_X520_COUNTRY_NAME,
				      0, 0, ret->country, &len);

	len = sizeof(ret->organization);
	gnutls_x509_crt_get_dn_by_oid(xcert,
				      GNUTLS_OID_X520_ORGANIZATION_NAME, 0,
				      0, ret->organization, &len);

	len = sizeof(ret->organizational_unit_name);
	gnutls_x509_crt_get_dn_by_oid(xcert,
				      GNUTLS_OID_X520_ORGANIZATIONAL_UNIT_NAME,
				      0, 0, ret->organizational_unit_name,
				      &len);

	len = sizeof(ret->common_name);
	gnutls_x509_crt_get_dn_by_oid(xcert, GNUTLS_OID_X520_COMMON_NAME,
				      0, 0, ret->common_name, &len);

	len = sizeof(ret->locality_name);
	gnutls_x509_crt_get_dn_by_oid(xcert, GNUTLS_OID_X520_LOCALITY_NAME,
				      0, 0, ret->locality_name, &len);

	len = sizeof(ret->state_or_province_name);
	gnutls_x509_crt_get_dn_by_oid(xcert,
				      GNUTLS_OID_X520_STATE_OR_PROVINCE_NAME,
				      0, 0, ret->state_or_province_name,
				      &len);

	len = sizeof(ret->email);
	gnutls_x509_crt_get_dn_by_oid(xcert, GNUTLS_OID_PKCS9_EMAIL, 0, 0,
				      ret->email, &len);

	gnutls_x509_crt_deinit(xcert);

	return 0;
}

/*-
 * gnutls_x509_extract_certificate_issuer_dn:
 * @cert: should contain an X.509 DER encoded certificate
 * @ret: a pointer to a structure to hold the issuer's name
 *
 * This function will return the name of the issuer stated in the certificate. The name is a gnutls_x509_dn structure and
 * is a obtained by the peer's certificate. If the certificate send by the
 * peer is invalid, or in any other failure this function returns error.
 * Returns a negative error code in case of an error.
 -*/
int
gnutls_x509_extract_certificate_issuer_dn(const gnutls_datum_t * cert,
					  gnutls_x509_dn * ret)
{
	gnutls_x509_crt_t xcert;
	int result;
	size_t len;

	result = gnutls_x509_crt_init(&xcert);
	if (result < 0)
		return result;

	result = gnutls_x509_crt_import(xcert, cert, GNUTLS_X509_FMT_DER);
	if (result < 0) {
		gnutls_x509_crt_deinit(xcert);
		return result;
	}

	len = sizeof(ret->country);
	gnutls_x509_crt_get_issuer_dn_by_oid(xcert,
					     GNUTLS_OID_X520_COUNTRY_NAME,
					     0, 0, ret->country, &len);

	len = sizeof(ret->organization);
	gnutls_x509_crt_get_issuer_dn_by_oid(xcert,
					     GNUTLS_OID_X520_ORGANIZATION_NAME,
					     0, 0, ret->organization,
					     &len);

	len = sizeof(ret->organizational_unit_name);
	gnutls_x509_crt_get_issuer_dn_by_oid(xcert,
					     GNUTLS_OID_X520_ORGANIZATIONAL_UNIT_NAME,
					     0, 0,
					     ret->organizational_unit_name,
					     &len);

	len = sizeof(ret->common_name);
	gnutls_x509_crt_get_issuer_dn_by_oid(xcert,
					     GNUTLS_OID_X520_COMMON_NAME,
					     0, 0, ret->common_name, &len);

	len = sizeof(ret->locality_name);
	gnutls_x509_crt_get_issuer_dn_by_oid(xcert,
					     GNUTLS_OID_X520_LOCALITY_NAME,
					     0, 0, ret->locality_name,
					     &len);

	len = sizeof(ret->state_or_province_name);
	gnutls_x509_crt_get_issuer_dn_by_oid(xcert,
					     GNUTLS_OID_X520_STATE_OR_PROVINCE_NAME,
					     0, 0,
					     ret->state_or_province_name,
					     &len);

	len = sizeof(ret->email);
	gnutls_x509_crt_get_issuer_dn_by_oid(xcert, GNUTLS_OID_PKCS9_EMAIL,
					     0, 0, ret->email, &len);

	gnutls_x509_crt_deinit(xcert);

	return 0;
}
