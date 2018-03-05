/*
 * Copyright (C) 2003-2016 Free Software Foundation, Inc.
 *
 * Authors: Nikos Mavrogiannopoulos, Simon Josefsson, Howard Chu
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

#include "gnutls_int.h"
#include <datum.h>
#include <global.h>
#include "errors.h"
#include <common.h>
#include <gnutls/x509-ext.h>
#include <x509.h>
#include <x509_b64.h>
#include <x509_int.h>
#include <libtasn1.h>
#include <pk.h>
#include <pkcs11_int.h>
#include "urls.h"

/**
 * gnutls_x509_tlsfeatures_init:
 * @f: The TLS features
 *
 * This function will initialize a X.509 TLS features extension structure
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned,
 *   otherwise a negative error value.
 *
 * Since: 3.5.1
 **/
int gnutls_x509_tlsfeatures_init(gnutls_x509_tlsfeatures_t *f)
{
	*f = gnutls_calloc(1, sizeof(struct gnutls_x509_tlsfeatures_st));
	if (*f == NULL)
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

	return 0;
}

/**
 * gnutls_x509_tlsfeatures_deinit:
 * @f: The TLS features
 *
 * This function will deinitialize a X.509 TLS features extension structure
 *
 * Since: 3.5.1
 **/
void gnutls_x509_tlsfeatures_deinit(gnutls_x509_tlsfeatures_t f)
{
	gnutls_free(f);
}

/**
 * gnutls_x509_tlsfeatures_get:
 * @f: The TLS features
 * @idx: The index of the feature to get
 * @feature: If the function succeeds, the feature will be stored in this variable
 *
 * This function will get a feature from the X.509 TLS features
 * extension structure.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned,
 *   otherwise a negative error value.
 *
 * Since: 3.5.1
 **/
int gnutls_x509_tlsfeatures_get(gnutls_x509_tlsfeatures_t f, unsigned idx, unsigned int *feature)
{
	if (f == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	if (idx >= f->size) {
		return gnutls_assert_val(GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE);
	}

	*feature = f->feature[idx];
	return 0;
}

/**
 * gnutls_x509_crt_get_tlsfeatures:
 * @crt: A X.509 certificate
 * @features: If the function succeeds, the
 *   features will be stored in this variable.
 * @flags: zero or %GNUTLS_EXT_FLAG_APPEND
 * @critical: the extension status
 *
 * This function will get the X.509 TLS features
 * extension structure from the certificate. The
 * returned structure needs to be freed using
 * gnutls_x509_tlsfeatures_deinit().
 *
 * When the @flags is set to %GNUTLS_EXT_FLAG_APPEND,
 * then if the @features structure is empty this function will behave
 * identically as if the flag was not set. Otherwise if there are elements 
 * in the @features structure then they will be merged with.
 *
 * Note that @features must be initialized prior to calling this function.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned,
 *   otherwise a negative error value.
 *
 * Since: 3.5.1
 **/
int gnutls_x509_crt_get_tlsfeatures(gnutls_x509_crt_t crt,
				    gnutls_x509_tlsfeatures_t features,
				    unsigned int flags,
				    unsigned int *critical)
{
	int ret;
	gnutls_datum_t der;

	if (crt == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	if ((ret =
		 _gnutls_x509_crt_get_extension(crt, GNUTLS_X509EXT_OID_TLSFEATURES, 0,
						&der, critical)) < 0)
	{
		return ret;
	}

	if (der.size == 0 || der.data == NULL) {
		gnutls_assert();
		return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
	}

	ret = gnutls_x509_ext_import_tlsfeatures(&der, features, flags);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = 0;
 cleanup:
	gnutls_free(der.data);
	return ret;
}

/**
 * gnutls_x509_crt_set_tlsfeatures:
 * @crt: A X.509 certificate
 * @features: If the function succeeds, the
 *   features will be added to the certificate.
 *
 * This function will set the certificates
 * X.509 TLS extension from the given structure.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned,
 *   otherwise a negative error value.
 *
 * Since: 3.5.1
 **/
int gnutls_x509_crt_set_tlsfeatures(gnutls_x509_crt_t crt,
				    gnutls_x509_tlsfeatures_t features)
{
	int ret;
	gnutls_datum_t der;

	if (crt == NULL || features == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	ret = gnutls_x509_ext_export_tlsfeatures(features, &der);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	ret = _gnutls_x509_crt_set_extension(crt, GNUTLS_X509EXT_OID_TLSFEATURES, &der, 0);

	_gnutls_free_datum(&der);

	if (ret < 0) {
		gnutls_assert();
	}

	return ret;
}

/**
 * gnutls_x509_tls_features_check_crt:
 * @feat: a set of TLSFeatures
 * @cert: the certificate to be checked
 *
 * This function will check the provided certificate against the TLSFeatures
 * set in @feat using the RFC7633 p.4.2.2 rules. It will check whether the certificate
 * contains the features in @feat or a superset.
 *
 * Returns: non-zero if the provided certificate complies, and zero otherwise.
 *
 * Since: 3.5.1
 **/
unsigned gnutls_x509_tlsfeatures_check_crt(gnutls_x509_tlsfeatures_t feat,
					   gnutls_x509_crt_t cert)
{
	int ret;
	gnutls_x509_tlsfeatures_t cfeat;
	unsigned i, j, uret, found;

	if (feat->size == 0)
		return 1; /* shortcut; no constraints to check */

	ret = gnutls_x509_tlsfeatures_init(&cfeat);
	if (ret < 0)
		return gnutls_assert_val(0);

	ret = gnutls_x509_crt_get_tlsfeatures(cert, cfeat, 0, NULL);
	if (ret < 0) {
		gnutls_assert();
		uret = 0;
		goto cleanup;
	}

	/* if cert's features cannot be a superset */
	if (feat->size > cfeat->size) {
		_gnutls_debug_log("certificate has %u, while issuer has %u tlsfeatures\n", cfeat->size, feat->size);
		gnutls_assert();
		uret = 0;
		goto cleanup;
	}

	for (i=0;i<feat->size;i++) {
		found = 0;
		for (j=0;j<cfeat->size;j++) {
			if (feat->feature[i] == cfeat->feature[j]) {
				found = 1;
				break;
			}
		}

		if (found == 0) {
			_gnutls_debug_log("feature %d was not found in cert\n", (int)feat->feature[i]);
			uret = 0;
			goto cleanup;
		}
	}

	uret = 1;
 cleanup:
	gnutls_x509_tlsfeatures_deinit(cfeat);
	return uret;
}
