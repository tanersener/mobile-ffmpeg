/*
 * Copyright (C) 2003-2018 Free Software Foundation, Inc.
 * Copyright (C) 2018 Red Hat, Inc.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>
 *
 */

/* Functions on X.509 Certificate parsing
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
#include "system-keys.h"

static int crt_reinit(gnutls_x509_crt_t crt)
{
	int result;

	_gnutls_free_datum(&crt->der);
	crt->raw_dn.size = 0;
	crt->raw_issuer_dn.size = 0;
	crt->raw_spki.size = 0;

	asn1_delete_structure(&crt->cert);

	result = asn1_create_element(_gnutls_get_pkix(),
				     "PKIX1.Certificate",
				     &crt->cert);
	if (result != ASN1_SUCCESS) {
		result = _gnutls_asn2err(result);
		gnutls_assert();
		return result;
	}

	gnutls_subject_alt_names_deinit(crt->san);
	result = gnutls_subject_alt_names_init(&crt->san);
	if (result < 0) {
		gnutls_assert();
		return result;
	}

	gnutls_subject_alt_names_deinit(crt->ian);
	result = gnutls_subject_alt_names_init(&crt->ian);
	if (result < 0) {
		gnutls_assert();
		return result;
	}

	return 0;
}

/**
 * gnutls_x509_crt_equals - This function compares two gnutls_x509_crt_t certificates
 * @cert1: The first certificate
 * @cert2: The second certificate
 *
 * This function will compare two X.509 certificate structures.
 *
 * Returns: On equality non-zero is returned, otherwise zero.
 *
 * Since: 3.5.0
 **/
unsigned gnutls_x509_crt_equals(gnutls_x509_crt_t cert1,
				gnutls_x509_crt_t cert2)
{
	int ret;
	bool result;

	if (cert1->modified == 0 && cert2->modified == 0 &&
	    cert1->raw_dn.size > 0 && cert2->raw_dn.size > 0) {
		ret = _gnutls_is_same_dn(cert1, cert2);
		if (ret == 0)
			return 0;
	}

	if (cert1->der.size == 0 || cert2->der.size == 0 ||
	    cert1->modified != 0 || cert2->modified != 0) {
		gnutls_datum_t tmp1, tmp2;

		/* on uninitialized or modified certificates, we have to re-encode */
		ret =
		    gnutls_x509_crt_export2(cert1, GNUTLS_X509_FMT_DER, &tmp1);
		if (ret < 0)
			return gnutls_assert_val(0);

		ret =
		    gnutls_x509_crt_export2(cert2, GNUTLS_X509_FMT_DER, &tmp2);
		if (ret < 0) {
			gnutls_free(tmp1.data);
			return gnutls_assert_val(0);
		}

		if ((tmp1.size == tmp2.size) &&
		    (memcmp(tmp1.data, tmp2.data, tmp1.size) == 0))
			result = 1;
		else
			result = 0;

		gnutls_free(tmp1.data);
		gnutls_free(tmp2.data);
	} else {
		if ((cert1->der.size == cert2->der.size) &&
		    (memcmp(cert1->der.data, cert2->der.data, cert1->der.size) == 0))
			result = 1;
		else
			result = 0;
	}

	return result;
}

/**
 * gnutls_x509_crt_equals2 - This function compares a gnutls_x509_crt_t cert with DER data
 * @cert1: The first certificate
 * @der: A DER encoded certificate
 *
 * This function will compare an X.509 certificate structures, with DER
 * encoded certificate data.
 *
 * Returns: On equality non-zero is returned, otherwise zero.
 *
 * Since: 3.5.0
 **/
unsigned
gnutls_x509_crt_equals2(gnutls_x509_crt_t cert1,
			const gnutls_datum_t * der)
{
	bool result;

	if (cert1 == NULL || der == NULL)
		return 0;

	if (cert1->der.size == 0 || cert1->modified) {
		gnutls_datum_t tmp1;
		int ret;

		/* on uninitialized or modified certificates, we have to re-encode */
		ret =
		    gnutls_x509_crt_export2(cert1, GNUTLS_X509_FMT_DER, &tmp1);
		if (ret < 0)
			return gnutls_assert_val(0);

		if ((tmp1.size == der->size) &&
		    (memcmp(tmp1.data, der->data, tmp1.size) == 0))
			result = 1;
		else
			result = 0;

		gnutls_free(tmp1.data);
	} else {
		if ((cert1->der.size == der->size) &&
		    (memcmp(cert1->der.data, der->data, cert1->der.size) == 0))
			result = 1;
		else
			result = 0;
	}

	return result;
}

/**
 * gnutls_x509_crt_init:
 * @cert: A pointer to the type to be initialized
 *
 * This function will initialize an X.509 certificate structure.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int gnutls_x509_crt_init(gnutls_x509_crt_t * cert)
{
	gnutls_x509_crt_t tmp;
	int result;

	FAIL_IF_LIB_ERROR;

	tmp =
	    gnutls_calloc(1, sizeof(gnutls_x509_crt_int));

	if (!tmp)
		return GNUTLS_E_MEMORY_ERROR;

	result = asn1_create_element(_gnutls_get_pkix(),
				     "PKIX1.Certificate", &tmp->cert);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		gnutls_free(tmp);
		return _gnutls_asn2err(result);
	}

	result = gnutls_subject_alt_names_init(&tmp->san);
	if (result < 0) {
		gnutls_assert();
		asn1_delete_structure(&tmp->cert);
		gnutls_free(tmp);
		return result;
	}

	result = gnutls_subject_alt_names_init(&tmp->ian);
	if (result < 0) {
		gnutls_assert();
		asn1_delete_structure(&tmp->cert);
		gnutls_subject_alt_names_deinit(tmp->san);
		gnutls_free(tmp);
		return result;
	}

	/* If you add anything here, be sure to check if it has to be added
	   to gnutls_x509_crt_import as well. */

	*cert = tmp;

	return 0;		/* success */
}

/*-
 * _gnutls_x509_crt_cpy - This function copies a gnutls_x509_crt_t type
 * @dest: The data where to copy
 * @src: The data to be copied
 * @flags: zero or CRT_CPY_FAST
 *
 * This function will copy an X.509 certificate structure.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 -*/
int _gnutls_x509_crt_cpy(gnutls_x509_crt_t dest, gnutls_x509_crt_t src)
{
	int ret;
	gnutls_datum_t tmp;
	unsigned dealloc = 0;

	if (src->der.size == 0 || src->modified) {
		ret =
		    gnutls_x509_crt_export2(src, GNUTLS_X509_FMT_DER, &tmp);
		if (ret < 0)
			return gnutls_assert_val(ret);
		dealloc = 1;
	} else {
		tmp.data = src->der.data;
		tmp.size = src->der.size;
	}

	ret = gnutls_x509_crt_import(dest, &tmp, GNUTLS_X509_FMT_DER);

	if (dealloc) {
		gnutls_free(tmp.data);
	}

	if (ret < 0)
		return gnutls_assert_val(ret);

	return 0;
}

/**
 * gnutls_x509_crt_deinit:
 * @cert: The data to be deinitialized
 *
 * This function will deinitialize a certificate structure.
 **/
void gnutls_x509_crt_deinit(gnutls_x509_crt_t cert)
{
	if (!cert)
		return;

	if (cert->cert)
		asn1_delete_structure(&cert->cert);
	gnutls_free(cert->der.data);
	gnutls_subject_alt_names_deinit(cert->san);
	gnutls_subject_alt_names_deinit(cert->ian);
	gnutls_free(cert);
}

static int compare_sig_algorithm(gnutls_x509_crt_t cert)
{
	int ret, len1, len2, result;
	char oid1[MAX_OID_SIZE];
	char oid2[MAX_OID_SIZE];
	gnutls_datum_t sp1 = {NULL, 0};
	gnutls_datum_t sp2 = {NULL, 0};
	unsigned empty1 = 0, empty2 = 0;

	len1 = sizeof(oid1);
	result = asn1_read_value(cert->cert, "signatureAlgorithm.algorithm", oid1, &len1);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	len2 = sizeof(oid2);
	result = asn1_read_value(cert->cert, "tbsCertificate.signature.algorithm", oid2, &len2);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	if (len1 != len2 || memcmp(oid1, oid2, len1) != 0) {
		_gnutls_debug_log("signatureAlgorithm.algorithm differs from tbsCertificate.signature.algorithm: %s, %s\n",
			oid1, oid2);
		gnutls_assert();
		return GNUTLS_E_CERTIFICATE_ERROR;
	}

	/* compare the parameters */
	ret = _gnutls_x509_read_value(cert->cert, "signatureAlgorithm.parameters", &sp1);
	if (ret == GNUTLS_E_ASN1_ELEMENT_NOT_FOUND) {
		empty1 = 1;
	} else if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	ret = _gnutls_x509_read_value(cert->cert, "tbsCertificate.signature.parameters", &sp2);
	if (ret == GNUTLS_E_ASN1_ELEMENT_NOT_FOUND) {
		empty2 = 1;
	} else if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	/* handle equally empty parameters with missing parameters */
	if (sp1.size == 2 && memcmp(sp1.data, "\x05\x00", 2) == 0) {
		empty1 = 1;
		_gnutls_free_datum(&sp1);
	}

	if (sp2.size == 2 && memcmp(sp2.data, "\x05\x00", 2) == 0) {
		empty2 = 1;
		_gnutls_free_datum(&sp2);
	}

	if (empty1 != empty2 || 
	    sp1.size != sp2.size || safe_memcmp(sp1.data, sp2.data, sp1.size) != 0) {
		gnutls_assert();
		ret = GNUTLS_E_CERTIFICATE_ERROR;
		goto cleanup;
	}

	ret = 0;
 cleanup:
	_gnutls_free_datum(&sp1);
	_gnutls_free_datum(&sp2);
	return ret;
}

static int cache_alt_names(gnutls_x509_crt_t cert)
{
	gnutls_datum_t tmpder = {NULL, 0};
	int ret;

	/* pre-parse subject alt name */
	ret = _gnutls_x509_crt_get_extension(cert, "2.5.29.17", 0, &tmpder, NULL);
	if (ret < 0 && ret != GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE) {
		gnutls_free(tmpder.data);
		return gnutls_assert_val(ret);
	}

	if (ret >= 0) {
		ret = gnutls_x509_ext_import_subject_alt_names(&tmpder, cert->san, 0);
		gnutls_free(tmpder.data);
		if (ret < 0)
			return gnutls_assert_val(ret);
	}

	ret = _gnutls_x509_crt_get_extension(cert, "2.5.29.18", 0, &tmpder, NULL);
	if (ret < 0 && ret != GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE)
		return gnutls_assert_val(ret);

	if (ret >= 0) {
		ret = gnutls_x509_ext_import_subject_alt_names(&tmpder, cert->ian, 0);
		gnutls_free(tmpder.data);
		if (ret < 0)
			return gnutls_assert_val(ret);
	}

	return 0;
}

int _gnutls_check_cert_sanity(gnutls_x509_crt_t cert)
{
	int result = 0, version;
	gnutls_datum_t exts;

	if (cert->flags & GNUTLS_X509_CRT_FLAG_IGNORE_SANITY)
		return 0;

	/* enforce the rule that only version 3 certificates carry extensions */
	result = gnutls_x509_crt_get_version(cert);
	if (result < 0) {
		gnutls_assert();
		goto cleanup;
	}

	version = result;

	if (version < 3) {
		if (!cert->modified) {
			result = _gnutls_x509_get_raw_field2(cert->cert, &cert->der,
				"tbsCertificate.extensions", &exts);
			if (result >= 0 && exts.size > 0) {
				gnutls_assert();
				_gnutls_debug_log("error: extensions present in certificate with version %d\n", version);
				result = GNUTLS_E_X509_CERTIFICATE_ERROR;
				goto cleanup;
			}
		} else {
			if (cert->use_extensions) {
				gnutls_assert();
				_gnutls_debug_log("error: extensions set in certificate with version %d\n", version);
				result = GNUTLS_E_X509_CERTIFICATE_ERROR;
				goto cleanup;
			}
		}
	}

	if (version < 2) {
		char id[128];
		size_t id_size;

		id_size = sizeof(id);
		result = gnutls_x509_crt_get_subject_unique_id(cert, id, &id_size);
		if (result >= 0 || result == GNUTLS_E_SHORT_MEMORY_BUFFER) {
			gnutls_assert();
			_gnutls_debug_log("error: subjectUniqueID present in certificate with version %d\n", version);
			result = GNUTLS_E_X509_CERTIFICATE_ERROR;
			goto cleanup;
		}

		id_size = sizeof(id);
		result = gnutls_x509_crt_get_issuer_unique_id(cert, id, &id_size);
		if (result >= 0 || result == GNUTLS_E_SHORT_MEMORY_BUFFER) {
			gnutls_assert();
			_gnutls_debug_log("error: subjectUniqueID present in certificate with version %d\n", version);
			result = GNUTLS_E_X509_CERTIFICATE_ERROR;
			goto cleanup;
		}
	}

	if (gnutls_x509_crt_get_expiration_time(cert) == -1 ||
	    gnutls_x509_crt_get_activation_time(cert) == -1) {
		gnutls_assert();
		_gnutls_debug_log("error: invalid expiration or activation time in certificate\n");
		result = GNUTLS_E_CERTIFICATE_TIME_ERROR;
		goto cleanup;
	}

	result = 0;

 cleanup:
	return result;
}

/**
 * gnutls_x509_crt_import:
 * @cert: The data to store the parsed certificate.
 * @data: The DER or PEM encoded certificate.
 * @format: One of DER or PEM
 *
 * This function will convert the given DER or PEM encoded Certificate
 * to the native gnutls_x509_crt_t format. The output will be stored
 * in @cert.
 *
 * If the Certificate is PEM encoded it should have a header of "X509
 * CERTIFICATE", or "CERTIFICATE".
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_x509_crt_import(gnutls_x509_crt_t cert,
		       const gnutls_datum_t * data,
		       gnutls_x509_crt_fmt_t format)
{
	int result;

	if (cert == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	if (cert->expanded) {
		/* Any earlier _asn1_strict_der_decode will modify the ASN.1
		   structure, so we need to replace it with a fresh
		   structure. */
		result = crt_reinit(cert);
		if (result < 0) {
			gnutls_assert();
			goto cleanup;
		}
	}

	/* If the Certificate is in PEM format then decode it
	 */
	if (format == GNUTLS_X509_FMT_PEM) {
		/* Try the first header */
		result =
		    _gnutls_fbase64_decode(PEM_X509_CERT2, data->data,
					   data->size, &cert->der);

		if (result < 0) {
			/* try for the second header */
			result =
			    _gnutls_fbase64_decode(PEM_X509_CERT,
						   data->data, data->size,
						   &cert->der);

			if (result < 0) {
				gnutls_assert();
				return result;
			}
		}
	} else {
		result = _gnutls_set_datum(&cert->der, data->data, data->size);
		if (result < 0) {
			gnutls_assert();
			return result;
		}
	}

	cert->expanded = 1;
	cert->modified = 0;

	result =
	    _asn1_strict_der_decode(&cert->cert, cert->der.data, cert->der.size, NULL);
	if (result != ASN1_SUCCESS) {
		result = _gnutls_asn2err(result);
		gnutls_assert();
		goto cleanup;
	}

	result = compare_sig_algorithm(cert);
	if (result < 0) {
		gnutls_assert();
		goto cleanup;
	}

	/* The following do not allocate but rather point to DER data */
	result = _gnutls_x509_get_raw_field2(cert->cert, &cert->der,
					  "tbsCertificate.issuer.rdnSequence",
					  &cert->raw_issuer_dn);
	if (result < 0) {
		gnutls_assert();
		goto cleanup;
	}

	result = _gnutls_x509_get_raw_field2(cert->cert, &cert->der,
					  "tbsCertificate.subject.rdnSequence",
					  &cert->raw_dn);
	if (result < 0) {
		gnutls_assert();
		goto cleanup;
	}

	result = _gnutls_x509_get_raw_field2(cert->cert, &cert->der,
					  "tbsCertificate.subjectPublicKeyInfo",
					  &cert->raw_spki);
	if (result < 0) {
		gnutls_assert();
		goto cleanup;
	}

	result = cache_alt_names(cert);
	if (result < 0) {
		gnutls_assert();
		goto cleanup;
	}

	result = _gnutls_check_cert_sanity(cert);
	if (result < 0) {
		gnutls_assert();
		goto cleanup;
	}

	/* Since we do not want to disable any extension
	 */
	cert->use_extensions = 1;

	return 0;

      cleanup:
	_gnutls_free_datum(&cert->der);
	return result;
}


/**
 * gnutls_x509_crt_get_issuer_dn:
 * @cert: should contain a #gnutls_x509_crt_t type
 * @buf: a pointer to a structure to hold the name (may be null)
 * @buf_size: initially holds the size of @buf
 *
 * This function will copy the name of the Certificate issuer in the
 * provided buffer. The name will be in the form
 * "C=xxxx,O=yyyy,CN=zzzz" as described in RFC4514. The output string
 * will be ASCII or UTF-8 encoded, depending on the certificate data.
 *
 * If @buf is null then only the size will be filled. 
 *
 * This function does not output a fully RFC4514 compliant string, if
 * that is required see gnutls_x509_crt_get_issuer_dn3().
 *
 * Returns: %GNUTLS_E_SHORT_MEMORY_BUFFER if the provided buffer is not
 *   long enough, and in that case the @buf_size will be updated
 *   with the required size. %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE if
 *   the DN does not exist, or another error value on error. On success 0 is returned.
 **/
int
gnutls_x509_crt_get_issuer_dn(gnutls_x509_crt_t cert, char *buf,
			      size_t * buf_size)
{
	if (cert == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	return _gnutls_x509_parse_dn(cert->cert,
				     "tbsCertificate.issuer.rdnSequence",
				     buf, buf_size, GNUTLS_X509_DN_FLAG_COMPAT);
}

/**
 * gnutls_x509_crt_get_issuer_dn2:
 * @cert: should contain a #gnutls_x509_crt_t type
 * @dn: a pointer to a structure to hold the name
 *
 * This function will allocate buffer and copy the name of issuer of the Certificate.
 * The name will be in the form "C=xxxx,O=yyyy,CN=zzzz" as
 * described in RFC4514. The output string will be ASCII or UTF-8
 * encoded, depending on the certificate data.
 *
 * This function does not output a fully RFC4514 compliant string, if
 * that is required see gnutls_x509_crt_get_issuer_dn3().
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.1.10
 **/
int
gnutls_x509_crt_get_issuer_dn2(gnutls_x509_crt_t cert, gnutls_datum_t * dn)
{
	if (cert == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	return _gnutls_x509_get_dn(cert->cert,
				   "tbsCertificate.issuer.rdnSequence",
				   dn, GNUTLS_X509_DN_FLAG_COMPAT);
}

/**
 * gnutls_x509_crt_get_issuer_dn3:
 * @cert: should contain a #gnutls_x509_crt_t type
 * @dn: a pointer to a structure to hold the name
 * @flags: zero or %GNUTLS_X509_DN_FLAG_COMPAT
 *
 * This function will allocate buffer and copy the name of issuer of the Certificate.
 * The name will be in the form "C=xxxx,O=yyyy,CN=zzzz" as
 * described in RFC4514. The output string will be ASCII or UTF-8
 * encoded, depending on the certificate data.
 *
 * When the flag %GNUTLS_X509_DN_FLAG_COMPAT is specified, the output
 * format will match the format output by previous to 3.5.6 versions of GnuTLS
 * which was not not fully RFC4514-compliant.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.5.7
 **/
int
gnutls_x509_crt_get_issuer_dn3(gnutls_x509_crt_t cert, gnutls_datum_t *dn, unsigned flags)
{
	if (cert == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	return _gnutls_x509_get_dn(cert->cert,
				   "tbsCertificate.issuer.rdnSequence",
				   dn, flags);
}

/**
 * gnutls_x509_crt_get_issuer_dn_by_oid:
 * @cert: should contain a #gnutls_x509_crt_t type
 * @oid: holds an Object Identified in null terminated string
 * @indx: In case multiple same OIDs exist in the RDN, this specifies which to send. Use (0) to get the first one.
 * @raw_flag: If non-zero returns the raw DER data of the DN part.
 * @buf: a pointer to a structure to hold the name (may be null)
 * @buf_size: initially holds the size of @buf
 *
 * This function will extract the part of the name of the Certificate
 * issuer specified by the given OID. The output, if the raw flag is not
 * used, will be encoded as described in RFC4514. Thus a string that is
 * ASCII or UTF-8 encoded, depending on the certificate data.
 *
 * Some helper macros with popular OIDs can be found in gnutls/x509.h
 * If raw flag is (0), this function will only return known OIDs as
 * text. Other OIDs will be DER encoded, as described in RFC4514 --
 * in hex format with a '#' prefix.  You can check about known OIDs
 * using gnutls_x509_dn_oid_known().
 *
 * If @buf is null then only the size will be filled. If the @raw_flag
 * is not specified the output is always null terminated, although the
 * @buf_size will not include the null character.
 *
 * Returns: %GNUTLS_E_SHORT_MEMORY_BUFFER if the provided buffer is not
 *   long enough, and in that case the @buf_size will be updated with
 *   the required size. %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE if there 
 *   are no data in the current index. On success 0 is returned.
 **/
int
gnutls_x509_crt_get_issuer_dn_by_oid(gnutls_x509_crt_t cert,
				     const char *oid, unsigned indx,
				     unsigned int raw_flag, void *buf,
				     size_t * buf_size)
{
	gnutls_datum_t td;
	int ret;

	if (cert == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	ret = _gnutls_x509_parse_dn_oid(cert->cert,
					"tbsCertificate.issuer.rdnSequence",
					oid, indx, raw_flag, &td);
	if (ret < 0)
		return gnutls_assert_val(ret);

	return _gnutls_strdatum_to_buf(&td, buf, buf_size);
}

/**
 * gnutls_x509_crt_get_issuer_dn_oid:
 * @cert: should contain a #gnutls_x509_crt_t type
 * @indx: This specifies which OID to return. Use (0) to get the first one.
 * @oid: a pointer to a buffer to hold the OID (may be null)
 * @oid_size: initially holds the size of @oid
 *
 * This function will extract the OIDs of the name of the Certificate
 * issuer specified by the given index.
 *
 * If @oid is null then only the size will be filled. The @oid
 * returned will be null terminated, although @oid_size will not
 * account for the trailing null.
 *
 * Returns: %GNUTLS_E_SHORT_MEMORY_BUFFER if the provided buffer is not
 *   long enough, and in that case the @buf_size will be updated with
 *   the required size. %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE if there 
 *   are no data in the current index. On success 0 is returned.
 **/
int
gnutls_x509_crt_get_issuer_dn_oid(gnutls_x509_crt_t cert,
				  unsigned indx, void *oid, size_t * oid_size)
{
	if (cert == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	return _gnutls_x509_get_dn_oid(cert->cert,
				       "tbsCertificate.issuer.rdnSequence",
				       indx, oid, oid_size);
}

/**
 * gnutls_x509_crt_get_dn:
 * @cert: should contain a #gnutls_x509_crt_t type
 * @buf: a pointer to a structure to hold the name (may be null)
 * @buf_size: initially holds the size of @buf
 *
 * This function will copy the name of the Certificate in the provided
 * buffer. The name will be in the form "C=xxxx,O=yyyy,CN=zzzz" as
 * described in RFC4514. The output string will be ASCII or UTF-8
 * encoded, depending on the certificate data.
 *
 * If @buf is null then only the size will be filled. 
 *
 * This function does not output a fully RFC4514 compliant string, if
 * that is required see gnutls_x509_crt_get_dn3().
 *
 * Returns: %GNUTLS_E_SHORT_MEMORY_BUFFER if the provided buffer is not
 *   long enough, and in that case the @buf_size will be updated
 *   with the required size. %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE if
 *   the DN does not exist, or another error value on error. On success 0 is returned.
 **/
int
gnutls_x509_crt_get_dn(gnutls_x509_crt_t cert, char *buf,
		       size_t * buf_size)
{
	if (cert == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	return _gnutls_x509_parse_dn(cert->cert,
				     "tbsCertificate.subject.rdnSequence",
				     buf, buf_size, GNUTLS_X509_DN_FLAG_COMPAT);
}

/**
 * gnutls_x509_crt_get_dn2:
 * @cert: should contain a #gnutls_x509_crt_t type
 * @dn: a pointer to a structure to hold the name
 *
 * This function will allocate buffer and copy the name of the Certificate.
 * The name will be in the form "C=xxxx,O=yyyy,CN=zzzz" as
 * described in RFC4514. The output string will be ASCII or UTF-8
 * encoded, depending on the certificate data.
 *
 * This function does not output a fully RFC4514 compliant string, if
 * that is required see gnutls_x509_crt_get_dn3().
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.1.10
 **/
int gnutls_x509_crt_get_dn2(gnutls_x509_crt_t cert, gnutls_datum_t * dn)
{
	if (cert == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	return _gnutls_x509_get_dn(cert->cert,
				   "tbsCertificate.subject.rdnSequence",
				   dn, GNUTLS_X509_DN_FLAG_COMPAT);
}

/**
 * gnutls_x509_crt_get_dn3:
 * @cert: should contain a #gnutls_x509_crt_t type
 * @dn: a pointer to a structure to hold the name
 * @flags: zero or %GNUTLS_X509_DN_FLAG_COMPAT
 *
 * This function will allocate buffer and copy the name of the Certificate.
 * The name will be in the form "C=xxxx,O=yyyy,CN=zzzz" as
 * described in RFC4514. The output string will be ASCII or UTF-8
 * encoded, depending on the certificate data.
 *
 * When the flag %GNUTLS_X509_DN_FLAG_COMPAT is specified, the output
 * format will match the format output by previous to 3.5.6 versions of GnuTLS
 * which was not not fully RFC4514-compliant.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.5.7
 **/
int gnutls_x509_crt_get_dn3(gnutls_x509_crt_t cert, gnutls_datum_t *dn, unsigned flags)
{
	if (cert == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	return _gnutls_x509_get_dn(cert->cert,
				   "tbsCertificate.subject.rdnSequence",
				   dn, flags);
}

/**
 * gnutls_x509_crt_get_dn_by_oid:
 * @cert: should contain a #gnutls_x509_crt_t type
 * @oid: holds an Object Identified in null terminated string
 * @indx: In case multiple same OIDs exist in the RDN, this specifies which to send. Use (0) to get the first one.
 * @raw_flag: If non-zero returns the raw DER data of the DN part.
 * @buf: a pointer where the DN part will be copied (may be null).
 * @buf_size: initially holds the size of @buf
 *
 * This function will extract the part of the name of the Certificate
 * subject specified by the given OID. The output, if the raw flag is
 * not used, will be encoded as described in RFC4514. Thus a string
 * that is ASCII or UTF-8 encoded, depending on the certificate data.
 *
 * Some helper macros with popular OIDs can be found in gnutls/x509.h
 * If raw flag is (0), this function will only return known OIDs as
 * text. Other OIDs will be DER encoded, as described in RFC4514 --
 * in hex format with a '#' prefix.  You can check about known OIDs
 * using gnutls_x509_dn_oid_known().
 *
 * If @buf is null then only the size will be filled. If the @raw_flag
 * is not specified the output is always null terminated, although the
 * @buf_size will not include the null character.
 *
 * Returns: %GNUTLS_E_SHORT_MEMORY_BUFFER if the provided buffer is not
 *   long enough, and in that case the @buf_size will be updated with
 *   the required size. %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE if there 
 *   are no data in the current index. On success 0 is returned.
 **/
int
gnutls_x509_crt_get_dn_by_oid(gnutls_x509_crt_t cert, const char *oid,
			      unsigned indx, unsigned int raw_flag,
			      void *buf, size_t * buf_size)
{
	gnutls_datum_t td;
	int ret;

	if (cert == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	ret = _gnutls_x509_parse_dn_oid(cert->cert,
					"tbsCertificate.subject.rdnSequence",
					oid, indx, raw_flag, &td);
	if (ret < 0)
		return gnutls_assert_val(ret);

	return _gnutls_strdatum_to_buf(&td, buf, buf_size);
}

/**
 * gnutls_x509_crt_get_dn_oid:
 * @cert: should contain a #gnutls_x509_crt_t type
 * @indx: This specifies which OID to return. Use (0) to get the first one.
 * @oid: a pointer to a buffer to hold the OID (may be null)
 * @oid_size: initially holds the size of @oid
 *
 * This function will extract the OIDs of the name of the Certificate
 * subject specified by the given index.
 *
 * If @oid is null then only the size will be filled. The @oid
 * returned will be null terminated, although @oid_size will not
 * account for the trailing null.
 *
 * Returns: %GNUTLS_E_SHORT_MEMORY_BUFFER if the provided buffer is not
 *   long enough, and in that case the @buf_size will be updated with
 *   the required size. %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE if there 
 *   are no data in the current index. On success 0 is returned.
 **/
int
gnutls_x509_crt_get_dn_oid(gnutls_x509_crt_t cert,
			   unsigned indx, void *oid, size_t * oid_size)
{
	if (cert == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	return _gnutls_x509_get_dn_oid(cert->cert,
				       "tbsCertificate.subject.rdnSequence",
				       indx, oid, oid_size);
}

/**
 * gnutls_x509_crt_get_signature_algorithm:
 * @cert: should contain a #gnutls_x509_crt_t type
 *
 * This function will return a value of the #gnutls_sign_algorithm_t
 * enumeration that is the signature algorithm that has been used to
 * sign this certificate.
 *
 * Since 3.6.0 this function never returns a negative error code.
 * Error cases and unknown/unsupported signature algorithms are
 * mapped to %GNUTLS_SIGN_UNKNOWN.
 *
 * Returns: a #gnutls_sign_algorithm_t value
 **/
int gnutls_x509_crt_get_signature_algorithm(gnutls_x509_crt_t cert)
{
	return map_errs_to_zero(_gnutls_x509_get_signature_algorithm(cert->cert,
						    "signatureAlgorithm"));
}

/**
 * gnutls_x509_crt_get_signature_oid:
 * @cert: should contain a #gnutls_x509_crt_t type
 * @oid: a pointer to a buffer to hold the OID (may be null)
 * @oid_size: initially holds the size of @oid
 *
 * This function will return the OID of the signature algorithm
 * that has been used to sign this certificate. This is function
 * is useful in the case gnutls_x509_crt_get_signature_algorithm()
 * returned %GNUTLS_SIGN_UNKNOWN.
 *
 * Returns: zero or a negative error code on error.
 *
 * Since: 3.5.0
 **/
int gnutls_x509_crt_get_signature_oid(gnutls_x509_crt_t cert, char *oid, size_t *oid_size)
{
	char str[MAX_OID_SIZE];
	int len, result, ret;
	gnutls_datum_t out;

	len = sizeof(str);
	result = asn1_read_value(cert->cert, "signatureAlgorithm.algorithm", str, &len);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	out.data = (void*)str;
	out.size = len;

	ret = _gnutls_copy_string(&out, (void*)oid, oid_size);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	return 0;
}

/**
 * gnutls_x509_crt_get_pk_oid:
 * @cert: should contain a #gnutls_x509_crt_t type
 * @oid: a pointer to a buffer to hold the OID (may be null)
 * @oid_size: initially holds the size of @oid
 *
 * This function will return the OID of the public key algorithm
 * on that certificate. This is function
 * is useful in the case gnutls_x509_crt_get_pk_algorithm()
 * returned %GNUTLS_PK_UNKNOWN.
 *
 * Returns: zero or a negative error code on error.
 *
 * Since: 3.5.0
 **/
int gnutls_x509_crt_get_pk_oid(gnutls_x509_crt_t cert, char *oid, size_t *oid_size)
{
	char str[MAX_OID_SIZE];
	int len, result, ret;
	gnutls_datum_t out;

	len = sizeof(str);
	result = asn1_read_value(cert->cert, "tbsCertificate.subjectPublicKeyInfo.algorithm.algorithm", str, &len);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	out.data = (void*)str;
	out.size = len;

	ret = _gnutls_copy_string(&out, (void*)oid, oid_size);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	return 0;
}

/**
 * gnutls_x509_crt_get_signature:
 * @cert: should contain a #gnutls_x509_crt_t type
 * @sig: a pointer where the signature part will be copied (may be null).
 * @sig_size: initially holds the size of @sig
 *
 * This function will extract the signature field of a certificate.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_x509_crt_get_signature(gnutls_x509_crt_t cert,
			      char *sig, size_t * sig_size)
{
	gnutls_datum_t dsig = {NULL, 0};
	int ret;

	if (cert == NULL)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	ret = _gnutls_x509_get_signature(cert->cert, "signature", &dsig);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = _gnutls_copy_data(&dsig, (uint8_t*)sig, sig_size);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = 0;
 cleanup:
	gnutls_free(dsig.data);
	return ret;
}

/**
 * gnutls_x509_crt_get_version:
 * @cert: should contain a #gnutls_x509_crt_t type
 *
 * This function will return the version of the specified Certificate.
 *
 * Returns: version of certificate, or a negative error code on error.
 **/
int gnutls_x509_crt_get_version(gnutls_x509_crt_t cert)
{
	uint8_t version[8];
	int len, result;

	if (cert == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	len = sizeof(version);
	if ((result =
	     asn1_read_value(cert->cert, "tbsCertificate.version", version,
			     &len)) != ASN1_SUCCESS) {

		if (result == ASN1_ELEMENT_NOT_FOUND)
			return 1;	/* the DEFAULT version */
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	if (len != 1 || version[0] >= 0x80)
		return gnutls_assert_val(GNUTLS_E_CERTIFICATE_ERROR);

	return (int) version[0] + 1;
}

/**
 * gnutls_x509_crt_get_activation_time:
 * @cert: should contain a #gnutls_x509_crt_t type
 *
 * This function will return the time this Certificate was or will be
 * activated.
 *
 * Returns: activation time, or (time_t)-1 on error.
 **/
time_t gnutls_x509_crt_get_activation_time(gnutls_x509_crt_t cert)
{
	if (cert == NULL) {
		gnutls_assert();
		return (time_t) - 1;
	}

	return _gnutls_x509_get_time(cert->cert,
				     "tbsCertificate.validity.notBefore",
				     0);
}

/**
 * gnutls_x509_crt_get_expiration_time:
 * @cert: should contain a #gnutls_x509_crt_t type
 *
 * This function will return the time this certificate was or will be
 * expired.
 *
 * Returns: expiration time, or (time_t)-1 on error.
 **/
time_t gnutls_x509_crt_get_expiration_time(gnutls_x509_crt_t cert)
{
	if (cert == NULL) {
		gnutls_assert();
		return (time_t) - 1;
	}

	return _gnutls_x509_get_time(cert->cert,
				     "tbsCertificate.validity.notAfter",
				     0);
}

/**
 * gnutls_x509_crt_get_private_key_usage_period:
 * @cert: should contain a #gnutls_x509_crt_t type
 * @activation: The activation time
 * @expiration: The expiration time
 * @critical: the extension status
 *
 * This function will return the expiration and activation
 * times of the private key of the certificate. It relies on
 * the PKIX extension 2.5.29.16 being present.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE
 * if the extension is not present, otherwise a negative error value.
 **/
int
gnutls_x509_crt_get_private_key_usage_period(gnutls_x509_crt_t cert,
					     time_t * activation,
					     time_t * expiration,
					     unsigned int *critical)
{
	int ret;
	gnutls_datum_t der = { NULL, 0 };

	if (cert == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	ret =
	    _gnutls_x509_crt_get_extension(cert, "2.5.29.16", 0, &der,
					   critical);
	if (ret < 0)
		return gnutls_assert_val(ret);

	if (der.size == 0 || der.data == NULL)
		return
		    gnutls_assert_val
		    (GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE);

	ret = gnutls_x509_ext_import_private_key_usage_period(&der, activation, expiration);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = 0;

      cleanup:
	_gnutls_free_datum(&der);

	return ret;
}


/**
 * gnutls_x509_crt_get_serial:
 * @cert: should contain a #gnutls_x509_crt_t type
 * @result: The place where the serial number will be copied
 * @result_size: Holds the size of the result field.
 *
 * This function will return the X.509 certificate's serial number.
 * This is obtained by the X509 Certificate serialNumber field. Serial
 * is not always a 32 or 64bit number. Some CAs use large serial
 * numbers, thus it may be wise to handle it as something uint8_t.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_x509_crt_get_serial(gnutls_x509_crt_t cert, void *result,
			   size_t * result_size)
{
	int ret, len;

	if (cert == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	len = *result_size;
	ret =
	    asn1_read_value(cert->cert, "tbsCertificate.serialNumber",
			    result, &len);
	*result_size = len;

	if (ret != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(ret);
	}

	return 0;
}

/**
 * gnutls_x509_crt_get_subject_key_id:
 * @cert: should contain a #gnutls_x509_crt_t type
 * @ret: The place where the identifier will be copied
 * @ret_size: Holds the size of the result field.
 * @critical: will be non-zero if the extension is marked as critical (may be null)
 *
 * This function will return the X.509v3 certificate's subject key
 * identifier.  This is obtained by the X.509 Subject Key identifier
 * extension field (2.5.29.14).
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE
 * if the extension is not present, otherwise a negative error value.
 **/
int
gnutls_x509_crt_get_subject_key_id(gnutls_x509_crt_t cert, void *ret,
				   size_t * ret_size,
				   unsigned int *critical)
{
	int result;
	gnutls_datum_t id = {NULL,0};
	gnutls_datum_t der = {NULL, 0};

	if (cert == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	if (ret == NULL)
		*ret_size = 0;

	if ((result =
	     _gnutls_x509_crt_get_extension(cert, "2.5.29.14", 0, &der,
					    critical)) < 0) {
		return result;
	}

	result = gnutls_x509_ext_import_subject_key_id(&der, &id);
	if (result < 0) {
		gnutls_assert();
		goto cleanup;
	}

	result = _gnutls_copy_data(&id, ret, ret_size);
	if (result < 0) {
		gnutls_assert();
		goto cleanup;
	}

	result = 0;

 cleanup:
	gnutls_free(der.data);
	gnutls_free(id.data);
	return result;
}

inline static int is_type_printable(int type)
{
	if (type == GNUTLS_SAN_DNSNAME || type == GNUTLS_SAN_RFC822NAME ||
	    type == GNUTLS_SAN_URI || type == GNUTLS_SAN_OTHERNAME_XMPP ||
	    type == GNUTLS_SAN_OTHERNAME || type == GNUTLS_SAN_REGISTERED_ID)
		return 1;
	else
		return 0;
}

/**
 * gnutls_x509_crt_get_authority_key_gn_serial:
 * @cert: should contain a #gnutls_x509_crt_t type
 * @seq: specifies the sequence number of the alt name (0 for the first one, 1 for the second etc.)
 * @alt: is the place where the alternative name will be copied to
 * @alt_size: holds the size of alt.
 * @alt_type: holds the type of the alternative name (one of gnutls_x509_subject_alt_name_t).
 * @serial: buffer to store the serial number (may be null)
 * @serial_size: Holds the size of the serial field (may be null)
 * @critical: will be non-zero if the extension is marked as critical (may be null)
 *
 * This function will return the X.509 authority key
 * identifier when stored as a general name (authorityCertIssuer) 
 * and serial number.
 *
 * Because more than one general names might be stored
 * @seq can be used as a counter to request them all until 
 * %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE is returned.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE
 * if the extension is not present, otherwise a negative error value.
 *
 * Since: 3.0
 **/
int
gnutls_x509_crt_get_authority_key_gn_serial(gnutls_x509_crt_t cert,
					    unsigned int seq, void *alt,
					    size_t * alt_size,
					    unsigned int *alt_type,
					    void *serial,
					    size_t * serial_size,
					    unsigned int *critical)
{
	int ret;
	gnutls_datum_t der, san, iserial;
	gnutls_x509_aki_t aki = NULL;
	unsigned san_type;

	if (cert == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	if ((ret =
	     _gnutls_x509_crt_get_extension(cert, "2.5.29.35", 0, &der,
					    critical)) < 0) {
		return gnutls_assert_val(ret);
	}

	if (der.size == 0 || der.data == NULL) {
		gnutls_assert();
		return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
	}

	ret = gnutls_x509_aki_init(&aki);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = gnutls_x509_ext_import_authority_key_id(&der, aki, 0);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = gnutls_x509_aki_get_cert_issuer(aki, seq, &san_type, &san, NULL, &iserial);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	if (is_type_printable(san_type))
		ret = _gnutls_copy_string(&san, alt, alt_size);
	else
		ret = _gnutls_copy_data(&san, alt, alt_size);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	if (alt_type)
		*alt_type = san_type;

	ret = _gnutls_copy_data(&iserial, serial, serial_size);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = 0;
 cleanup:
	if (aki != NULL)
		gnutls_x509_aki_deinit(aki);
	gnutls_free(der.data);
	return ret;
}

/**
 * gnutls_x509_crt_get_authority_key_id:
 * @cert: should contain a #gnutls_x509_crt_t type
 * @id: The place where the identifier will be copied
 * @id_size: Holds the size of the id field.
 * @critical: will be non-zero if the extension is marked as critical (may be null)
 *
 * This function will return the X.509v3 certificate authority's key
 * identifier.  This is obtained by the X.509 Authority Key
 * identifier extension field (2.5.29.35). Note that this function
 * only returns the keyIdentifier field of the extension and
 * %GNUTLS_E_X509_UNSUPPORTED_EXTENSION, if the extension contains
 * the name and serial number of the certificate. In that case
 * gnutls_x509_crt_get_authority_key_gn_serial() may be used.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE
 * if the extension is not present, otherwise a negative error value.
 **/
int
gnutls_x509_crt_get_authority_key_id(gnutls_x509_crt_t cert, void *id,
				     size_t * id_size,
				     unsigned int *critical)
{
	int ret;
	gnutls_datum_t der, l_id;
	gnutls_x509_aki_t aki = NULL;

	if (cert == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	if ((ret =
	     _gnutls_x509_crt_get_extension(cert, "2.5.29.35", 0, &der,
					    critical)) < 0) {
		return gnutls_assert_val(ret);
	}

	if (der.size == 0 || der.data == NULL) {
		gnutls_assert();
		return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
	}

	ret = gnutls_x509_aki_init(&aki);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = gnutls_x509_ext_import_authority_key_id(&der, aki, 0);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = gnutls_x509_aki_get_id(aki, &l_id);

	if (ret == GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE) {
		gnutls_datum_t serial;
		ret = gnutls_x509_aki_get_cert_issuer(aki, 0, NULL, NULL, NULL, &serial);
		if (ret >= 0) {
			ret = gnutls_assert_val(GNUTLS_E_X509_UNSUPPORTED_EXTENSION);
		} else {
			ret = gnutls_assert_val(GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE);
		}
	}

	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = _gnutls_copy_data(&l_id, id, id_size);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = 0;
 cleanup:
	if (aki != NULL)
		gnutls_x509_aki_deinit(aki);
	gnutls_free(der.data);
	return ret;
}

/**
 * gnutls_x509_crt_get_pk_algorithm:
 * @cert: should contain a #gnutls_x509_crt_t type
 * @bits: if bits is non null it will hold the size of the parameters' in bits
 *
 * This function will return the public key algorithm of an X.509
 * certificate.
 *
 * If bits is non null, it should have enough size to hold the parameters
 * size in bits. For RSA the bits returned is the modulus.
 * For DSA the bits returned are of the public
 * exponent.
 *
 * Unknown/unsupported algorithms are mapped to %GNUTLS_PK_UNKNOWN.
 *
 * Returns: a member of the #gnutls_pk_algorithm_t enumeration on
 * success, or a negative error code on error.
 **/
int
gnutls_x509_crt_get_pk_algorithm(gnutls_x509_crt_t cert,
				 unsigned int *bits)
{
	int result;

	if (cert == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	if (bits)
		*bits = 0;

	result =
	    _gnutls_x509_get_pk_algorithm(cert->cert,
					  "tbsCertificate.subjectPublicKeyInfo",
					  NULL,
					  bits);

	if (result < 0) {
		gnutls_assert();
		return result;
	}

	return result;
}

/**
 * gnutls_x509_crt_get_spki:
 * @cert: a certificate of type #gnutls_x509_crt_t
 * @spki: a SubjectPublicKeyInfo structure of type #gnutls_x509_spki_t
 * @flags: must be zero
 *
 * This function will return the public key information of an X.509
 * certificate. The provided @spki must be initialized.
 *
 * Since: 3.6.0
 **/
int
gnutls_x509_crt_get_spki(gnutls_x509_crt_t cert, gnutls_x509_spki_t spki, unsigned int flags)
{
	int result;
	gnutls_x509_spki_st params;

	if (cert == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}


	spki->pk = gnutls_x509_crt_get_pk_algorithm(cert, NULL);

	memset(&params, 0, sizeof(params));

	result = _gnutls_x509_crt_read_spki_params(cert, &params);
	if (result < 0) {
		gnutls_assert();
		return result;
	}

	if (params.pk == GNUTLS_PK_UNKNOWN)
		return gnutls_assert_val(GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE);

	spki->rsa_pss_dig = params.rsa_pss_dig;
	spki->salt_size = params.salt_size;

	return 0;
}

/* returns the type and the name on success.
 * Type is also returned as a parameter in case of an error.
 *
 * @seq: in case of GeneralNames it will return the corresponding name.
 *       in case of GeneralName, it must be -1
 * @dname: the name returned
 * @ret_type: The type of the name
 * @othername_oid: if the name is otherName return the OID
 *
 */
int
_gnutls_parse_general_name2(ASN1_TYPE src, const char *src_name,
			   int seq, gnutls_datum_t *dname, 
			   unsigned int *ret_type, int othername_oid)
{
	int len, ret;
	char nptr[MAX_NAME_SIZE];
	int result;
	gnutls_datum_t tmp = {NULL, 0};
	char choice_type[128];
	gnutls_x509_subject_alt_name_t type;

	if (seq != -1) {
		seq++;	/* 0->1, 1->2 etc */

		if (src_name[0] != 0)
			snprintf(nptr, sizeof(nptr), "%s.?%u", src_name, seq);
		else
			snprintf(nptr, sizeof(nptr), "?%u", seq);
	} else {
		snprintf(nptr, sizeof(nptr), "%s", src_name);
	}

	len = sizeof(choice_type);
	result = asn1_read_value(src, nptr, choice_type, &len);
	if (result == ASN1_VALUE_NOT_FOUND
	    || result == ASN1_ELEMENT_NOT_FOUND) {
		return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
	}

	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	type = _gnutls_x509_san_find_type(choice_type);
	if (type == (gnutls_x509_subject_alt_name_t) - 1) {
		gnutls_assert();
		return GNUTLS_E_X509_UNKNOWN_SAN;
	}

	if (ret_type)
		*ret_type = type;

	if (type == GNUTLS_SAN_OTHERNAME) {
		if (othername_oid)
			_gnutls_str_cat(nptr, sizeof(nptr),
					".otherName.type-id");
		else
			_gnutls_str_cat(nptr, sizeof(nptr),
					".otherName.value");

		ret = _gnutls_x509_read_value(src, nptr, &tmp);
		if (ret < 0) {
			gnutls_assert();
			return ret;
		}

		if (othername_oid) {
			dname->size = tmp.size;
			dname->data = tmp.data;
		} else {
			char oid[MAX_OID_SIZE];

			if (src_name[0] != 0)
				snprintf(nptr, sizeof(nptr),
					 "%s.?%u.otherName.type-id",
					 src_name, seq);
			else
				snprintf(nptr, sizeof(nptr),
					 "?%u.otherName.type-id", seq);

			len = sizeof(oid);

			result = asn1_read_value(src, nptr, oid, &len);
			if (result != ASN1_SUCCESS) {
				gnutls_assert();
				ret = _gnutls_asn2err(result);
				goto cleanup;
			}
			if (len > 0) len--;

			dname->size = tmp.size;
			dname->data = tmp.data;
		}
	} else if (type == GNUTLS_SAN_DN) {
		_gnutls_str_cat(nptr, sizeof(nptr), ".directoryName");
		ret = _gnutls_x509_get_dn(src, nptr, dname, 0);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}
	} else if (othername_oid) {
		gnutls_assert();
		ret = GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
		goto cleanup;
	} else {
		_gnutls_str_cat(nptr, sizeof(nptr), ".");
		_gnutls_str_cat(nptr, sizeof(nptr), choice_type);

		ret = _gnutls_x509_read_null_value(src, nptr, &tmp);
		if (ret < 0) {
			gnutls_assert();
			return ret;
		}

		if (type == GNUTLS_SAN_REGISTERED_ID && tmp.size > 0) {
			/* see #805; OIDs contain the null termination byte */
			assert(tmp.data[tmp.size-1] == 0);
			tmp.size--;
		}

		/* _gnutls_x509_read_value() null terminates */
		dname->size = tmp.size;
		dname->data = tmp.data;
	}

	return type;

 cleanup:
	gnutls_free(tmp.data);
	return ret;
}

/* returns the type and the name on success.
 * Type is also returned as a parameter in case of an error.
 */
int
_gnutls_parse_general_name(ASN1_TYPE src, const char *src_name,
			   int seq, void *name, size_t * name_size,
			   unsigned int *ret_type, int othername_oid)
{
	int ret;
	gnutls_datum_t res = {NULL,0};
	unsigned type;

	ret = _gnutls_parse_general_name2(src, src_name, seq, &res, ret_type, othername_oid);
	if (ret < 0)
		return gnutls_assert_val(ret);

	type = ret;

	if (is_type_printable(type)) {
		ret = _gnutls_copy_string(&res, name, name_size);
	} else {
		ret = _gnutls_copy_data(&res, name, name_size);
	}

	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = type;
cleanup:
	gnutls_free(res.data);
	return ret;
}

static int
get_alt_name(gnutls_subject_alt_names_t san,
	     unsigned int seq, uint8_t *alt,
	     size_t * alt_size, unsigned int *alt_type,
	     unsigned int *critical, int othername_oid)
{
	int ret;
	gnutls_datum_t ooid = {NULL, 0};
	gnutls_datum_t oname;
	gnutls_datum_t virt = {NULL, 0};
	unsigned int type;

	if (san == NULL) {
		gnutls_assert();
		return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
	}

	if (alt == NULL)
		*alt_size = 0;

	ret = gnutls_subject_alt_names_get(san, seq, &type, &oname, &ooid);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	if (othername_oid && type == GNUTLS_SAN_OTHERNAME) {
		unsigned vtype;
		ret = gnutls_x509_othername_to_virtual((char*)ooid.data, &oname, &vtype, &virt);
		if (ret >= 0) {
			type = vtype;
			oname.data = virt.data;
			oname.size = virt.size;
		}
	}

	if (alt_type)
		*alt_type = type;

	if (othername_oid) {
		ret = _gnutls_copy_string(&ooid, alt, alt_size);
	} else {
		if (is_type_printable(type)) {
			ret = _gnutls_copy_string(&oname, alt, alt_size);
		} else {
			ret = _gnutls_copy_data(&oname, alt, alt_size);
		}
	}

	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = type;
cleanup:
	gnutls_free(virt.data);

	return ret;
}

/**
 * gnutls_x509_crt_get_subject_alt_name:
 * @cert: should contain a #gnutls_x509_crt_t type
 * @seq: specifies the sequence number of the alt name (0 for the first one, 1 for the second etc.)
 * @san: is the place where the alternative name will be copied to
 * @san_size: holds the size of san.
 * @critical: will be non-zero if the extension is marked as critical (may be null)
 *
 * This function retrieves the Alternative Name (2.5.29.17), contained
 * in the given certificate in the X509v3 Certificate Extensions.
 *
 * When the SAN type is otherName, it will extract the data in the
 * otherName's value field, and %GNUTLS_SAN_OTHERNAME is returned.
 * You may use gnutls_x509_crt_get_subject_alt_othername_oid() to get
 * the corresponding OID and the "virtual" SAN types (e.g.,
 * %GNUTLS_SAN_OTHERNAME_XMPP).
 *
 * If an otherName OID is known, the data will be decoded.  Otherwise
 * the returned data will be DER encoded, and you will have to decode
 * it yourself.  Currently, only the RFC 3920 id-on-xmppAddr SAN is
 * recognized.
 *
 * Returns: the alternative subject name type on success, one of the
 *   enumerated #gnutls_x509_subject_alt_name_t.  It will return
 *   %GNUTLS_E_SHORT_MEMORY_BUFFER if @san_size is not large enough to
 *   hold the value.  In that case @san_size will be updated with the
 *   required size.  If the certificate does not have an Alternative
 *   name with the specified sequence number then
 *   %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE is returned.
 **/
int
gnutls_x509_crt_get_subject_alt_name(gnutls_x509_crt_t cert,
				     unsigned int seq, void *san,
				     size_t * san_size,
				     unsigned int *critical)
{
	return get_alt_name(cert->san, seq, san, san_size, NULL,
			    critical, 0);
}

/**
 * gnutls_x509_crt_get_issuer_alt_name:
 * @cert: should contain a #gnutls_x509_crt_t type
 * @seq: specifies the sequence number of the alt name (0 for the first one, 1 for the second etc.)
 * @ian: is the place where the alternative name will be copied to
 * @ian_size: holds the size of ian.
 * @critical: will be non-zero if the extension is marked as critical (may be null)
 *
 * This function retrieves the Issuer Alternative Name (2.5.29.18),
 * contained in the given certificate in the X509v3 Certificate
 * Extensions.
 *
 * When the SAN type is otherName, it will extract the data in the
 * otherName's value field, and %GNUTLS_SAN_OTHERNAME is returned.
 * You may use gnutls_x509_crt_get_subject_alt_othername_oid() to get
 * the corresponding OID and the "virtual" SAN types (e.g.,
 * %GNUTLS_SAN_OTHERNAME_XMPP).
 *
 * If an otherName OID is known, the data will be decoded.  Otherwise
 * the returned data will be DER encoded, and you will have to decode
 * it yourself.  Currently, only the RFC 3920 id-on-xmppAddr Issuer
 * AltName is recognized.
 *
 * Returns: the alternative issuer name type on success, one of the
 *   enumerated #gnutls_x509_subject_alt_name_t.  It will return
 *   %GNUTLS_E_SHORT_MEMORY_BUFFER if @ian_size is not large enough
 *   to hold the value.  In that case @ian_size will be updated with
 *   the required size.  If the certificate does not have an
 *   Alternative name with the specified sequence number then
 *   %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE is returned.
 *
 * Since: 2.10.0
 **/
int
gnutls_x509_crt_get_issuer_alt_name(gnutls_x509_crt_t cert,
				    unsigned int seq, void *ian,
				    size_t * ian_size,
				    unsigned int *critical)
{
	return get_alt_name(cert->ian, seq, ian, ian_size, NULL,
			    critical, 0);
}

/**
 * gnutls_x509_crt_get_subject_alt_name2:
 * @cert: should contain a #gnutls_x509_crt_t type
 * @seq: specifies the sequence number of the alt name (0 for the first one, 1 for the second etc.)
 * @san: is the place where the alternative name will be copied to
 * @san_size: holds the size of ret.
 * @san_type: holds the type of the alternative name (one of gnutls_x509_subject_alt_name_t).
 * @critical: will be non-zero if the extension is marked as critical (may be null)
 *
 * This function will return the alternative names, contained in the
 * given certificate. It is the same as
 * gnutls_x509_crt_get_subject_alt_name() except for the fact that it
 * will return the type of the alternative name in @san_type even if
 * the function fails for some reason (i.e.  the buffer provided is
 * not enough).
 *
 * Returns: the alternative subject name type on success, one of the
 *   enumerated #gnutls_x509_subject_alt_name_t.  It will return
 *   %GNUTLS_E_SHORT_MEMORY_BUFFER if @san_size is not large enough
 *   to hold the value.  In that case @san_size will be updated with
 *   the required size.  If the certificate does not have an
 *   Alternative name with the specified sequence number then
 *   %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE is returned.
 **/
int
gnutls_x509_crt_get_subject_alt_name2(gnutls_x509_crt_t cert,
				      unsigned int seq, void *san,
				      size_t * san_size,
				      unsigned int *san_type,
				      unsigned int *critical)
{
	return get_alt_name(cert->san, seq, san, san_size,
			    san_type, critical, 0);
}

/**
 * gnutls_x509_crt_get_issuer_alt_name2:
 * @cert: should contain a #gnutls_x509_crt_t type
 * @seq: specifies the sequence number of the alt name (0 for the first one, 1 for the second etc.)
 * @ian: is the place where the alternative name will be copied to
 * @ian_size: holds the size of ret.
 * @ian_type: holds the type of the alternative name (one of gnutls_x509_subject_alt_name_t).
 * @critical: will be non-zero if the extension is marked as critical (may be null)
 *
 * This function will return the alternative names, contained in the
 * given certificate. It is the same as
 * gnutls_x509_crt_get_issuer_alt_name() except for the fact that it
 * will return the type of the alternative name in @ian_type even if
 * the function fails for some reason (i.e.  the buffer provided is
 * not enough).
 *
 * Returns: the alternative issuer name type on success, one of the
 *   enumerated #gnutls_x509_subject_alt_name_t.  It will return
 *   %GNUTLS_E_SHORT_MEMORY_BUFFER if @ian_size is not large enough
 *   to hold the value.  In that case @ian_size will be updated with
 *   the required size.  If the certificate does not have an
 *   Alternative name with the specified sequence number then
 *   %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE is returned.
 *
 * Since: 2.10.0
 *
 **/
int
gnutls_x509_crt_get_issuer_alt_name2(gnutls_x509_crt_t cert,
				     unsigned int seq, void *ian,
				     size_t * ian_size,
				     unsigned int *ian_type,
				     unsigned int *critical)
{
	return get_alt_name(cert->ian, seq, ian, ian_size,
			    ian_type, critical, 0);
}

/**
 * gnutls_x509_crt_get_subject_alt_othername_oid:
 * @cert: should contain a #gnutls_x509_crt_t type
 * @seq: specifies the sequence number of the alt name (0 for the first one, 1 for the second etc.)
 * @oid: is the place where the otherName OID will be copied to
 * @oid_size: holds the size of ret.
 *
 * This function will extract the type OID of an otherName Subject
 * Alternative Name, contained in the given certificate, and return
 * the type as an enumerated element.
 *
 * This function is only useful if
 * gnutls_x509_crt_get_subject_alt_name() returned
 * %GNUTLS_SAN_OTHERNAME.
 *
 * If @oid is null then only the size will be filled. The @oid
 * returned will be null terminated, although @oid_size will not
 * account for the trailing null.
 *
 * Returns: the alternative subject name type on success, one of the
 * enumerated gnutls_x509_subject_alt_name_t.  For supported OIDs, it
 * will return one of the virtual (GNUTLS_SAN_OTHERNAME_*) types,
 * e.g. %GNUTLS_SAN_OTHERNAME_XMPP, and %GNUTLS_SAN_OTHERNAME for
 * unknown OIDs.  It will return %GNUTLS_E_SHORT_MEMORY_BUFFER if
 * @ian_size is not large enough to hold the value.  In that case
 * @ian_size will be updated with the required size.  If the
 * certificate does not have an Alternative name with the specified
 * sequence number and with the otherName type then
 * %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE is returned.
 **/
int
gnutls_x509_crt_get_subject_alt_othername_oid(gnutls_x509_crt_t cert,
					      unsigned int seq,
					      void *oid, size_t * oid_size)
{
	return get_alt_name(cert->san, seq, oid, oid_size, NULL,
			    NULL, 1);
}

/**
 * gnutls_x509_crt_get_issuer_alt_othername_oid:
 * @cert: should contain a #gnutls_x509_crt_t type
 * @seq: specifies the sequence number of the alt name (0 for the first one, 1 for the second etc.)
 * @ret: is the place where the otherName OID will be copied to
 * @ret_size: holds the size of ret.
 *
 * This function will extract the type OID of an otherName Subject
 * Alternative Name, contained in the given certificate, and return
 * the type as an enumerated element.
 *
 * If @oid is null then only the size will be filled. The @oid
 * returned will be null terminated, although @oid_size will not
 * account for the trailing null.
 *
 * This function is only useful if
 * gnutls_x509_crt_get_issuer_alt_name() returned
 * %GNUTLS_SAN_OTHERNAME.
 *
 * Returns: the alternative issuer name type on success, one of the
 * enumerated gnutls_x509_subject_alt_name_t.  For supported OIDs, it
 * will return one of the virtual (GNUTLS_SAN_OTHERNAME_*) types,
 * e.g. %GNUTLS_SAN_OTHERNAME_XMPP, and %GNUTLS_SAN_OTHERNAME for
 * unknown OIDs.  It will return %GNUTLS_E_SHORT_MEMORY_BUFFER if
 * @ret_size is not large enough to hold the value.  In that case
 * @ret_size will be updated with the required size.  If the
 * certificate does not have an Alternative name with the specified
 * sequence number and with the otherName type then
 * %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE is returned.
 *
 * Since: 2.10.0
 **/
int
gnutls_x509_crt_get_issuer_alt_othername_oid(gnutls_x509_crt_t cert,
					     unsigned int seq,
					     void *ret, size_t * ret_size)
{
	return get_alt_name(cert->ian, seq, ret, ret_size, NULL,
			    NULL, 1);
}

/**
 * gnutls_x509_crt_get_basic_constraints:
 * @cert: should contain a #gnutls_x509_crt_t type
 * @critical: will be non-zero if the extension is marked as critical
 * @ca: pointer to output integer indicating CA status, may be NULL,
 *   value is 1 if the certificate CA flag is set, 0 otherwise.
 * @pathlen: pointer to output integer indicating path length (may be
 *   NULL), non-negative error codes indicate a present pathLenConstraint
 *   field and the actual value, -1 indicate that the field is absent.
 *
 * This function will read the certificate's basic constraints, and
 * return the certificates CA status.  It reads the basicConstraints
 * X.509 extension (2.5.29.19).
 *
 * Returns: If the certificate is a CA a positive value will be
 * returned, or (0) if the certificate does not have CA flag set.  A
 * negative error code may be returned in case of errors.  If the
 * certificate does not contain the basicConstraints extension
 * GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE will be returned.
 **/
int
gnutls_x509_crt_get_basic_constraints(gnutls_x509_crt_t cert,
				      unsigned int *critical,
				      unsigned int *ca, int *pathlen)
{
	int result;
	gnutls_datum_t basicConstraints;
	unsigned int tmp_ca;

	if (cert == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	if ((result =
	     _gnutls_x509_crt_get_extension(cert, "2.5.29.19", 0,
					    &basicConstraints,
					    critical)) < 0) {
		return result;
	}

	if (basicConstraints.size == 0 || basicConstraints.data == NULL) {
		gnutls_assert();
		return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
	}

	result = gnutls_x509_ext_import_basic_constraints(&basicConstraints, &tmp_ca, pathlen);
	if (ca)
		*ca = tmp_ca;

	_gnutls_free_datum(&basicConstraints);

	if (result < 0) {
		gnutls_assert();
		return result;
	}

	return tmp_ca;
}

/**
 * gnutls_x509_crt_get_ca_status:
 * @cert: should contain a #gnutls_x509_crt_t type
 * @critical: will be non-zero if the extension is marked as critical
 *
 * This function will return certificates CA status, by reading the
 * basicConstraints X.509 extension (2.5.29.19). If the certificate is
 * a CA a positive value will be returned, or (0) if the certificate
 * does not have CA flag set.
 *
 * Use gnutls_x509_crt_get_basic_constraints() if you want to read the
 * pathLenConstraint field too.
 *
 * Returns: If the certificate is a CA a positive value will be
 * returned, or (0) if the certificate does not have CA flag set.  A
 * negative error code may be returned in case of errors.  If the
 * certificate does not contain the basicConstraints extension
 * GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE will be returned.
 **/
int
gnutls_x509_crt_get_ca_status(gnutls_x509_crt_t cert,
			      unsigned int *critical)
{
	int pathlen;
	unsigned int ca;
	return gnutls_x509_crt_get_basic_constraints(cert, critical, &ca,
						     &pathlen);
}

/**
 * gnutls_x509_crt_get_key_usage:
 * @cert: should contain a #gnutls_x509_crt_t type
 * @key_usage: where the key usage bits will be stored
 * @critical: will be non-zero if the extension is marked as critical
 *
 * This function will return certificate's key usage, by reading the
 * keyUsage X.509 extension (2.5.29.15). The key usage value will ORed
 * values of the: %GNUTLS_KEY_DIGITAL_SIGNATURE,
 * %GNUTLS_KEY_NON_REPUDIATION, %GNUTLS_KEY_KEY_ENCIPHERMENT,
 * %GNUTLS_KEY_DATA_ENCIPHERMENT, %GNUTLS_KEY_KEY_AGREEMENT,
 * %GNUTLS_KEY_KEY_CERT_SIGN, %GNUTLS_KEY_CRL_SIGN,
 * %GNUTLS_KEY_ENCIPHER_ONLY, %GNUTLS_KEY_DECIPHER_ONLY.
 *
 * Returns: zero on success, or a negative error code in case of
 *   parsing error.  If the certificate does not contain the keyUsage
 *   extension %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE will be
 *   returned.
 **/
int
gnutls_x509_crt_get_key_usage(gnutls_x509_crt_t cert,
			      unsigned int *key_usage,
			      unsigned int *critical)
{
	int result;
	gnutls_datum_t keyUsage;

	if (cert == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	if ((result =
	     _gnutls_x509_crt_get_extension(cert, "2.5.29.15", 0,
					    &keyUsage, critical)) < 0) {
		return result;
	}

	if (keyUsage.size == 0 || keyUsage.data == NULL) {
		gnutls_assert();
		return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
	}

	result = gnutls_x509_ext_import_key_usage(&keyUsage, key_usage);
	_gnutls_free_datum(&keyUsage);

	if (result < 0) {
		gnutls_assert();
		return result;
	}

	return 0;
}

/**
 * gnutls_x509_crt_get_inhibit_anypolicy:
 * @cert: should contain a #gnutls_x509_crt_t type
 * @skipcerts: will hold the number of certificates after which anypolicy is no longer acceptable.
 * @critical: will be non-zero if the extension is marked as critical
 *
 * This function will return certificate's value of the SkipCerts, i.e.,
 * the Inhibit anyPolicy X.509 extension (2.5.29.54).
 *
 * The returned value is the number of additional certificates that
 * may appear in the path before the anyPolicy is no longer acceptable.

 * Returns: zero on success, or a negative error code in case of
 *   parsing error.  If the certificate does not contain the Inhibit anyPolicy
 *   extension %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE will be
 *   returned.
 *
 * Since: 3.6.0
 **/
int
gnutls_x509_crt_get_inhibit_anypolicy(gnutls_x509_crt_t cert,
			      unsigned int *skipcerts,
			      unsigned int *critical)
{
	int ret;
	gnutls_datum_t ext;

	if (cert == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	if ((ret =
	     _gnutls_x509_crt_get_extension(cert, "2.5.29.54", 0,
					    &ext, critical)) < 0) {
		return ret;
	}

	if (ext.size == 0 || ext.data == NULL) {
		gnutls_assert();
		return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
	}

	ret = gnutls_x509_ext_import_key_usage(&ext, skipcerts);
	_gnutls_free_datum(&ext);

	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	return 0;
}

/**
 * gnutls_x509_crt_get_proxy:
 * @cert: should contain a #gnutls_x509_crt_t type
 * @critical: will be non-zero if the extension is marked as critical
 * @pathlen: pointer to output integer indicating path length (may be
 *   NULL), non-negative error codes indicate a present pCPathLenConstraint
 *   field and the actual value, -1 indicate that the field is absent.
 * @policyLanguage: output variable with OID of policy language
 * @policy: output variable with policy data
 * @sizeof_policy: output variable size of policy data
 *
 * This function will get information from a proxy certificate.  It
 * reads the ProxyCertInfo X.509 extension (1.3.6.1.5.5.7.1.14).
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned,
 *   otherwise a negative error code is returned.
 **/
int
gnutls_x509_crt_get_proxy(gnutls_x509_crt_t cert,
			  unsigned int *critical,
			  int *pathlen,
			  char **policyLanguage,
			  char **policy, size_t * sizeof_policy)
{
	int result;
	gnutls_datum_t proxyCertInfo;

	if (cert == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	if ((result =
	     _gnutls_x509_crt_get_extension(cert, "1.3.6.1.5.5.7.1.14", 0,
					    &proxyCertInfo, critical)) < 0)
	{
		return result;
	}

	if (proxyCertInfo.size == 0 || proxyCertInfo.data == NULL) {
		gnutls_assert();
		return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
	}

	result = gnutls_x509_ext_import_proxy(&proxyCertInfo, pathlen,
							policyLanguage,
							policy,
							sizeof_policy);
	_gnutls_free_datum(&proxyCertInfo);
	if (result < 0) {
		gnutls_assert();
		return result;
	}

	return 0;
}


/**
 * gnutls_x509_policy_release:
 * @policy: a certificate policy
 *
 * This function will deinitialize all memory associated with the provided
 * @policy. The policy is allocated using gnutls_x509_crt_get_policy().
 *
 * Since: 3.1.5
 **/
void gnutls_x509_policy_release(struct gnutls_x509_policy_st *policy)
{
	unsigned i;

	gnutls_free(policy->oid);
	for (i = 0; i < policy->qualifiers; i++)
		gnutls_free(policy->qualifier[i].data);
}


/**
 * gnutls_x509_crt_get_policy:
 * @crt: should contain a #gnutls_x509_crt_t type
 * @indx: This specifies which policy to return. Use (0) to get the first one.
 * @policy: A pointer to a policy structure.
 * @critical: will be non-zero if the extension is marked as critical
 *
 * This function will extract the certificate policy (extension 2.5.29.32) 
 * specified by the given index. 
 *
 * The policy returned by this function must be deinitialized by using
 * gnutls_x509_policy_release().
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE
 * if the extension is not present, otherwise a negative error value.
 *
 * Since: 3.1.5
 **/
int
gnutls_x509_crt_get_policy(gnutls_x509_crt_t crt, unsigned indx,
			   struct gnutls_x509_policy_st *policy,
			   unsigned int *critical)
{
	gnutls_datum_t tmpd = { NULL, 0 };
	int ret;
	gnutls_x509_policies_t policies = NULL;

	if (crt == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	memset(policy, 0, sizeof(*policy));

	ret = gnutls_x509_policies_init(&policies);
	if (ret < 0)
		return gnutls_assert_val(ret);

	if ((ret =
	     _gnutls_x509_crt_get_extension(crt, "2.5.29.32", 0, &tmpd,
					    critical)) < 0) {
		goto cleanup;
	}

	if (tmpd.size == 0 || tmpd.data == NULL) {
		gnutls_assert();
		ret = GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
		goto cleanup;
	}

	ret = gnutls_x509_ext_import_policies(&tmpd, policies, 0);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = gnutls_x509_policies_get(policies, indx, policy);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	_gnutls_x509_policies_erase(policies, indx);

	ret = 0;

 cleanup:
	if (policies != NULL)
		gnutls_x509_policies_deinit(policies);
	_gnutls_free_datum(&tmpd);

	return ret;
}


/**
 * gnutls_x509_crt_get_extension_by_oid:
 * @cert: should contain a #gnutls_x509_crt_t type
 * @oid: holds an Object Identified in null terminated string
 * @indx: In case multiple same OIDs exist in the extensions, this specifies which to send. Use (0) to get the first one.
 * @buf: a pointer to a structure to hold the name (may be null)
 * @buf_size: initially holds the size of @buf
 * @critical: will be non-zero if the extension is marked as critical
 *
 * This function will return the extension specified by the OID in the
 * certificate.  The extensions will be returned as binary data DER
 * encoded, in the provided buffer.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned,
 *   otherwise a negative error code is returned. If the certificate does not
 *   contain the specified extension
 *   GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE will be returned.
 **/
int
gnutls_x509_crt_get_extension_by_oid(gnutls_x509_crt_t cert,
				     const char *oid, unsigned indx,
				     void *buf, size_t * buf_size,
				     unsigned int *critical)
{
	int result;
	gnutls_datum_t output;

	if (cert == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	if ((result =
	     _gnutls_x509_crt_get_extension(cert, oid, indx, &output,
					    critical)) < 0) {
		gnutls_assert();
		return result;
	}

	if (output.size == 0 || output.data == NULL) {
		gnutls_assert();
		return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
	}

	if (output.size > (unsigned int) *buf_size) {
		*buf_size = output.size;
		_gnutls_free_datum(&output);
		return GNUTLS_E_SHORT_MEMORY_BUFFER;
	}

	*buf_size = output.size;

	if (buf)
		memcpy(buf, output.data, output.size);

	_gnutls_free_datum(&output);

	return 0;
}

/**
 * gnutls_x509_crt_get_extension_by_oid2:
 * @cert: should contain a #gnutls_x509_crt_t type
 * @oid: holds an Object Identified in null terminated string
 * @indx: In case multiple same OIDs exist in the extensions, this specifies which to send. Use (0) to get the first one.
 * @output: will hold the allocated extension data
 * @critical: will be non-zero if the extension is marked as critical
 *
 * This function will return the extension specified by the OID in the
 * certificate.  The extensions will be returned as binary data DER
 * encoded, in the provided buffer.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned,
 *   otherwise a negative error code is returned. If the certificate does not
 *   contain the specified extension
 *   GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE will be returned.
 *
 * Since: 3.3.8
 **/
int
gnutls_x509_crt_get_extension_by_oid2(gnutls_x509_crt_t cert,
				     const char *oid, unsigned indx,
				     gnutls_datum_t *output,
				     unsigned int *critical)
{
	int ret;
	
	if (cert == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	if ((ret =
	     _gnutls_x509_crt_get_extension(cert, oid, indx, output,
					    critical)) < 0) {
		gnutls_assert();
		return ret;
	}

	if (output->size == 0 || output->data == NULL) {
		gnutls_assert();
		return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
	}

	return 0;
}

/**
 * gnutls_x509_crt_get_extension_oid:
 * @cert: should contain a #gnutls_x509_crt_t type
 * @indx: Specifies which extension OID to send. Use (0) to get the first one.
 * @oid: a pointer to a structure to hold the OID (may be null)
 * @oid_size: initially holds the size of @oid
 *
 * This function will return the requested extension OID in the certificate.
 * The extension OID will be stored as a string in the provided buffer.
 *
 * The @oid returned will be null terminated, although @oid_size will not
 * account for the trailing null.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned,
 *   otherwise a negative error code is returned.  If you have reached the
 *   last extension available %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE
 *   will be returned.
 **/
int
gnutls_x509_crt_get_extension_oid(gnutls_x509_crt_t cert, unsigned indx,
				  void *oid, size_t * oid_size)
{
	int result;

	if (cert == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	result =
	    _gnutls_x509_crt_get_extension_oid(cert, indx, oid, oid_size);
	if (result < 0) {
		return result;
	}

	return 0;

}

/**
 * gnutls_x509_crt_get_extension_info:
 * @cert: should contain a #gnutls_x509_crt_t type
 * @indx: Specifies which extension OID to send. Use (0) to get the first one.
 * @oid: a pointer to a structure to hold the OID
 * @oid_size: initially holds the maximum size of @oid, on return
 *   holds actual size of @oid.
 * @critical: output variable with critical flag, may be NULL.
 *
 * This function will return the requested extension OID in the
 * certificate, and the critical flag for it.  The extension OID will
 * be stored as a string in the provided buffer.  Use
 * gnutls_x509_crt_get_extension() to extract the data.
 *
 * If the buffer provided is not long enough to hold the output, then
 * @oid_size is updated and %GNUTLS_E_SHORT_MEMORY_BUFFER will be
 * returned. The @oid returned will be null terminated, although 
 * @oid_size will not account for the trailing null (the latter is not
 * true for GnuTLS prior to 3.6.0).
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned,
 *   otherwise a negative error code is returned.  If you have reached the
 *   last extension available %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE
 *   will be returned.
 **/
int
gnutls_x509_crt_get_extension_info(gnutls_x509_crt_t cert, unsigned indx,
				   void *oid, size_t * oid_size,
				   unsigned int *critical)
{
	int result;
	char str_critical[10];
	char name[MAX_NAME_SIZE];
	int len;

	if (!cert) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	snprintf(name, sizeof(name),
		 "tbsCertificate.extensions.?%u.extnID", indx + 1);

	len = *oid_size;
	result = asn1_read_value(cert->cert, name, oid, &len);
	*oid_size = len;

	if (result == ASN1_ELEMENT_NOT_FOUND)
		return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
	else if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	/* remove any trailing null */
	if (oid && len > 0 && ((uint8_t*)oid)[len-1] == 0)
		(*oid_size)--;

	snprintf(name, sizeof(name),
		 "tbsCertificate.extensions.?%u.critical", indx + 1);
	len = sizeof(str_critical);
	result = asn1_read_value(cert->cert, name, str_critical, &len);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	if (critical) {
		if (str_critical[0] == 'T')
			*critical = 1;
		else
			*critical = 0;
	}

	return 0;

}

/**
 * gnutls_x509_crt_get_extension_data:
 * @cert: should contain a #gnutls_x509_crt_t type
 * @indx: Specifies which extension OID to send. Use (0) to get the first one.
 * @data: a pointer to a structure to hold the data (may be null)
 * @sizeof_data: initially holds the size of @data
 *
 * This function will return the requested extension data in the
 * certificate.  The extension data will be stored in the
 * provided buffer.
 *
 * Use gnutls_x509_crt_get_extension_info() to extract the OID and
 * critical flag.  Use gnutls_x509_crt_get_extension_by_oid() instead,
 * if you want to get data indexed by the extension OID rather than
 * sequence.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned,
 *   otherwise a negative error code is returned.  If you have reached the
 *   last extension available %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE
 *   will be returned.
 **/
int
gnutls_x509_crt_get_extension_data(gnutls_x509_crt_t cert, unsigned indx,
				   void *data, size_t * sizeof_data)
{
	int result, len;
	char name[MAX_NAME_SIZE];

	if (!cert) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	snprintf(name, sizeof(name),
		 "tbsCertificate.extensions.?%u.extnValue", indx + 1);

	len = *sizeof_data;
	result = asn1_read_value(cert->cert, name, data, &len);
	*sizeof_data = len;

	if (result == ASN1_ELEMENT_NOT_FOUND) {
		return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
	} else if (result == ASN1_MEM_ERROR && data == NULL) {
		/* normally we should return GNUTLS_E_SHORT_MEMORY_BUFFER,
		 * but we haven't done that for long time, so use
		 * backwards compatible behavior */
		return 0;
	} else if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	return 0;
}

/**
 * gnutls_x509_crt_get_raw_issuer_dn:
 * @cert: should contain a #gnutls_x509_crt_t type
 * @dn: will hold the starting point of the DN
 *
 * This function will return a pointer to the DER encoded DN structure
 * and the length. This points to allocated data that must be free'd using gnutls_free().
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.or a negative error code on error.
 *
 **/
int
gnutls_x509_crt_get_raw_issuer_dn(gnutls_x509_crt_t cert,
				  gnutls_datum_t * dn)
{
	if (cert->raw_issuer_dn.size > 0 && cert->modified == 0) {
		return _gnutls_set_datum(dn, cert->raw_issuer_dn.data,
					 cert->raw_issuer_dn.size);
	} else {
		return _gnutls_x509_get_raw_field(cert->cert, "tbsCertificate.issuer.rdnSequence", dn);
	}
}

/**
 * gnutls_x509_crt_get_raw_dn:
 * @cert: should contain a #gnutls_x509_crt_t type
 * @dn: will hold the starting point of the DN
 *
 * This function will return a pointer to the DER encoded DN structure and
 * the length. This points to allocated data that must be free'd using gnutls_free().
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value. or a negative error code on error.
 *
 **/
int gnutls_x509_crt_get_raw_dn(gnutls_x509_crt_t cert, gnutls_datum_t * dn)
{
	if (cert->raw_dn.size > 0 && cert->modified == 0) {
		return _gnutls_set_datum(dn, cert->raw_dn.data, cert->raw_dn.size);
	} else {
		return _gnutls_x509_get_raw_field(cert->cert, "tbsCertificate.subject.rdnSequence", dn);
	}
}

static int
get_dn(gnutls_x509_crt_t cert, const char *whom, gnutls_x509_dn_t * dn, unsigned subject)
{
	gnutls_x509_dn_st *store;

	if (subject)
		store = &cert->dn;
	else
		store = &cert->idn;

	store->asn = asn1_find_node(cert->cert, whom);
	if (!store->asn)
		return GNUTLS_E_ASN1_ELEMENT_NOT_FOUND;

	*dn = store;

	return 0;
}

/**
 * gnutls_x509_crt_get_subject:
 * @cert: should contain a #gnutls_x509_crt_t type
 * @dn: output variable with pointer to uint8_t DN.
 *
 * Return the Certificate's Subject DN as a %gnutls_x509_dn_t data type,
 * that can be decoded using gnutls_x509_dn_get_rdn_ava(). 
 *
 * Note that @dn should be treated as constant. Because it points 
 * into the @cert object, you should not use @dn after @cert is
 * deallocated.
 *
 * Returns: Returns 0 on success, or an error code.
 **/
int
gnutls_x509_crt_get_subject(gnutls_x509_crt_t cert, gnutls_x509_dn_t * dn)
{
	return get_dn(cert, "tbsCertificate.subject.rdnSequence", dn, 1);
}

/**
 * gnutls_x509_crt_get_issuer:
 * @cert: should contain a #gnutls_x509_crt_t type
 * @dn: output variable with pointer to uint8_t DN
 *
 * Return the Certificate's Issuer DN as a %gnutls_x509_dn_t data type,
 * that can be decoded using gnutls_x509_dn_get_rdn_ava(). 
 *
 * Note that @dn should be treated as constant. Because it points 
 * into the @cert object, you should not use @dn after @cert is
 * deallocated.
 *
 * Returns: Returns 0 on success, or an error code.
 **/
int
gnutls_x509_crt_get_issuer(gnutls_x509_crt_t cert, gnutls_x509_dn_t * dn)
{
	return get_dn(cert, "tbsCertificate.issuer.rdnSequence", dn, 0);
}

/**
 * gnutls_x509_crt_get_fingerprint:
 * @cert: should contain a #gnutls_x509_crt_t type
 * @algo: is a digest algorithm
 * @buf: a pointer to a structure to hold the fingerprint (may be null)
 * @buf_size: initially holds the size of @buf
 *
 * This function will calculate and copy the certificate's fingerprint
 * in the provided buffer. The fingerprint is a hash of the DER-encoded
 * data of the certificate.
 *
 * If the buffer is null then only the size will be filled.
 *
 * Returns: %GNUTLS_E_SHORT_MEMORY_BUFFER if the provided buffer is
 *   not long enough, and in that case the *buf_size will be updated
 *   with the required size.  On success 0 is returned.
 **/
int
gnutls_x509_crt_get_fingerprint(gnutls_x509_crt_t cert,
				gnutls_digest_algorithm_t algo,
				void *buf, size_t * buf_size)
{
	uint8_t *cert_buf;
	int cert_buf_size;
	int result;
	gnutls_datum_t tmp;

	if (buf_size == 0 || cert == NULL) {
		return GNUTLS_E_INVALID_REQUEST;
	}

	cert_buf_size = 0;
	result = asn1_der_coding(cert->cert, "", NULL, &cert_buf_size, NULL);
	if (result != ASN1_MEM_ERROR) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	cert_buf = gnutls_malloc(cert_buf_size);
	if (cert_buf == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	result =
	    asn1_der_coding(cert->cert, "", cert_buf, &cert_buf_size,
			    NULL);

	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		gnutls_free(cert_buf);
		return _gnutls_asn2err(result);
	}

	tmp.data = cert_buf;
	tmp.size = cert_buf_size;

	result = gnutls_fingerprint(algo, &tmp, buf, buf_size);
	gnutls_free(cert_buf);

	return result;
}

/**
 * gnutls_x509_crt_export:
 * @cert: Holds the certificate
 * @format: the format of output params. One of PEM or DER.
 * @output_data: will contain a certificate PEM or DER encoded
 * @output_data_size: holds the size of output_data (and will be
 *   replaced by the actual size of parameters)
 *
 * This function will export the certificate to DER or PEM format.
 *
 * If the buffer provided is not long enough to hold the output, then
 * *output_data_size is updated and GNUTLS_E_SHORT_MEMORY_BUFFER will
 * be returned.
 *
 * If the structure is PEM encoded, it will have a header
 * of "BEGIN CERTIFICATE".
 *
 * Returns: In case of failure a negative error code will be
 *   returned, and 0 on success.
 **/
int
gnutls_x509_crt_export(gnutls_x509_crt_t cert,
		       gnutls_x509_crt_fmt_t format, void *output_data,
		       size_t * output_data_size)
{
	gnutls_datum_t out;
	int ret;

	ret = gnutls_x509_crt_export2(cert, format, &out);
	if (ret < 0)
		return gnutls_assert_val(ret);

	if (format == GNUTLS_X509_FMT_PEM)
		ret = _gnutls_copy_string(&out, (uint8_t*)output_data, output_data_size);
	else
		ret = _gnutls_copy_data(&out, (uint8_t*)output_data, output_data_size);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = 0;
 cleanup:
	gnutls_free(out.data);
	return ret;
}

/**
 * gnutls_x509_crt_export2:
 * @cert: Holds the certificate
 * @format: the format of output params. One of PEM or DER.
 * @out: will contain a certificate PEM or DER encoded
 *
 * This function will export the certificate to DER or PEM format.
 * The output buffer is allocated using gnutls_malloc().
 *
 * If the structure is PEM encoded, it will have a header
 * of "BEGIN CERTIFICATE".
 *
 * Returns: In case of failure a negative error code will be
 *   returned, and 0 on success.
 *
 * Since: 3.1.3
 **/
int
gnutls_x509_crt_export2(gnutls_x509_crt_t cert,
			gnutls_x509_crt_fmt_t format, gnutls_datum_t * out)
{
	if (cert == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	if (!cert->modified && cert->der.size) {
		if (format == GNUTLS_X509_FMT_DER)
			return _gnutls_set_datum(out, cert->der.data, cert->der.size);
		else
			return _gnutls_fbase64_encode(PEM_X509_CERT2, cert->der.data,
						      cert->der.size, out);

	}

	return _gnutls_x509_export_int2(cert->cert, format, PEM_X509_CERT2,
					out);
}

int
_gnutls_get_key_id(gnutls_pk_params_st * params,
		   unsigned char *output_data, size_t * output_data_size,
		   unsigned flags)
{
	int ret = 0;
	gnutls_datum_t der = { NULL, 0 };
	gnutls_digest_algorithm_t hash = GNUTLS_DIG_SHA1;
	unsigned int digest_len;

	if ((flags & GNUTLS_KEYID_USE_SHA512) || (flags & GNUTLS_KEYID_USE_BEST_KNOWN))
		hash = GNUTLS_DIG_SHA512;
	else if (flags & GNUTLS_KEYID_USE_SHA256)
		hash = GNUTLS_DIG_SHA256;

	digest_len =
	    _gnutls_hash_get_algo_len(hash_to_entry(hash));

	if (output_data == NULL || *output_data_size < digest_len) {
		gnutls_assert();
		*output_data_size = digest_len;
		return GNUTLS_E_SHORT_MEMORY_BUFFER;
	}

	ret = _gnutls_x509_encode_PKI_params(&der, params);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = _gnutls_hash_fast(hash, der.data, der.size, output_data);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}
	*output_data_size = digest_len;

	ret = 0;

      cleanup:

	_gnutls_free_datum(&der);
	return ret;
}

/**
 * gnutls_x509_crt_get_key_id:
 * @crt: Holds the certificate
 * @flags: should be one of the flags from %gnutls_keyid_flags_t
 * @output_data: will contain the key ID
 * @output_data_size: holds the size of output_data (and will be
 *   replaced by the actual size of parameters)
 *
 * This function will return a unique ID that depends on the public
 * key parameters. This ID can be used in checking whether a
 * certificate corresponds to the given private key.
 *
 * If the buffer provided is not long enough to hold the output, then
 * *output_data_size is updated and GNUTLS_E_SHORT_MEMORY_BUFFER will
 * be returned.  The output will normally be a SHA-1 hash output,
 * which is 20 bytes.
 *
 * Returns: In case of failure a negative error code will be
 *   returned, and 0 on success.
 **/
int
gnutls_x509_crt_get_key_id(gnutls_x509_crt_t crt, unsigned int flags,
			   unsigned char *output_data,
			   size_t * output_data_size)
{
	int ret = 0;
	gnutls_pk_params_st params;

	if (crt == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	/* initializes params */
	ret = _gnutls_x509_crt_get_mpis(crt, &params);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	ret =
	    _gnutls_get_key_id(&params, output_data, output_data_size, flags);

	gnutls_pk_params_release(&params);

	return ret;
}

static int
crl_issuer_matches(gnutls_x509_crl_t crl, gnutls_x509_crt_t cert)
{
	if (_gnutls_x509_compare_raw_dn
	    (&crl->raw_issuer_dn, &cert->raw_issuer_dn) != 0)
		return 1;
	else
		return 0;
}

/* This is exactly as gnutls_x509_crt_check_revocation() except that
 * it calls func.
 */
int
_gnutls_x509_crt_check_revocation(gnutls_x509_crt_t cert,
				  const gnutls_x509_crl_t * crl_list,
				  int crl_list_length,
				  gnutls_verify_output_function func)
{
	uint8_t serial[128];
	uint8_t cert_serial[128];
	size_t serial_size, cert_serial_size;
	int ret, j;
	gnutls_x509_crl_iter_t iter = NULL;

	if (cert == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	for (j = 0; j < crl_list_length; j++) {	/* do for all the crls */

		/* Step 1. check if issuer's DN match
		 */
		ret = crl_issuer_matches(crl_list[j], cert);
		if (ret == 0) {
			/* issuers do not match so don't even
			 * bother checking.
			 */
			gnutls_assert();
			continue;
		}

		/* Step 2. Read the certificate's serial number
		 */
		cert_serial_size = sizeof(cert_serial);
		ret =
		    gnutls_x509_crt_get_serial(cert, cert_serial,
					       &cert_serial_size);
		if (ret < 0) {
			gnutls_assert();
			return ret;
		}

		/* Step 3. cycle through the CRL serials and compare with
		 *   certificate serial we have.
		 */

		iter = NULL;
		do {
			serial_size = sizeof(serial);
			ret =
			    gnutls_x509_crl_iter_crt_serial(crl_list[j],
							    &iter,
							    serial,
							    &serial_size,
							    NULL);
			if (ret == GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE) {
				break;
			} else if (ret < 0) {
				gnutls_assert();
				goto fail;
			}

			if (serial_size == cert_serial_size) {
				if (memcmp
				    (serial, cert_serial,
				     serial_size) == 0) {
					/* serials match */
					if (func)
						func(cert, NULL,
						     crl_list[j],
						     GNUTLS_CERT_REVOKED |
						     GNUTLS_CERT_INVALID);
					ret = 1;	/* revoked! */
					goto fail;
				}
			}
		} while(1);

		gnutls_x509_crl_iter_deinit(iter);
		iter = NULL;

		if (func)
			func(cert, NULL, crl_list[j], 0);

	}
	return 0;		/* not revoked. */

 fail:
	gnutls_x509_crl_iter_deinit(iter);
	return ret;
}


/**
 * gnutls_x509_crt_check_revocation:
 * @cert: should contain a #gnutls_x509_crt_t type
 * @crl_list: should contain a list of gnutls_x509_crl_t types
 * @crl_list_length: the length of the crl_list
 *
 * This function will check if the given certificate is
 * revoked.  It is assumed that the CRLs have been verified before.
 *
 * Returns: 0 if the certificate is NOT revoked, and 1 if it is.  A
 * negative error code is returned on error.
 **/
int
gnutls_x509_crt_check_revocation(gnutls_x509_crt_t cert,
				 const gnutls_x509_crl_t * crl_list,
				 unsigned crl_list_length)
{
	return _gnutls_x509_crt_check_revocation(cert, crl_list,
						 crl_list_length, NULL);
}

/**
 * gnutls_x509_crt_check_key_purpose:
 * @cert: should contain a #gnutls_x509_crt_t type
 * @purpose: a key purpose OID (e.g., %GNUTLS_KP_CODE_SIGNING)
 * @flags: zero or %GNUTLS_KP_FLAG_DISALLOW_ANY
 *
 * This function will check whether the given certificate matches
 * the provided key purpose. If @flags contains %GNUTLS_KP_FLAG_ALLOW_ANY then
 * it a certificate marked for any purpose will not match.
 *
 * Returns: zero if the key purpose doesn't match, and non-zero otherwise.
 *
 * Since: 3.5.6
 **/
unsigned
gnutls_x509_crt_check_key_purpose(gnutls_x509_crt_t cert,
				 const char *purpose,
				 unsigned flags)
{
	return _gnutls_check_key_purpose(cert, purpose, (flags&GNUTLS_KP_FLAG_DISALLOW_ANY)?1:0);
}

/**
 * gnutls_x509_crt_get_preferred_hash_algorithm:
 * @crt: Holds the certificate
 * @hash: The result of the call with the hash algorithm used for signature
 * @mand: If non-zero it means that the algorithm MUST use this hash. May be %NULL.
 *
 * This function will read the certificate and return the appropriate digest
 * algorithm to use for signing with this certificate. Some certificates (i.e.
 * DSA might not be able to sign without the preferred algorithm).
 *
 * Deprecated: Please use gnutls_pubkey_get_preferred_hash_algorithm().
 *
 * Returns: the 0 if the hash algorithm is found. A negative error code is
 * returned on error.
 *
 * Since: 2.12.0
 **/
int
gnutls_x509_crt_get_preferred_hash_algorithm(gnutls_x509_crt_t crt,
					     gnutls_digest_algorithm_t *
					     hash, unsigned int *mand)
{
	int ret;
	gnutls_pubkey_t pubkey;

	if (crt == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	ret = gnutls_pubkey_init(&pubkey);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = gnutls_pubkey_import_x509(pubkey, crt, 0);
	if (ret < 0) {  
		gnutls_assert();
		goto cleanup;
	}

	ret = gnutls_pubkey_get_preferred_hash_algorithm(pubkey, hash, mand);
	if (ret < 0) {  
		gnutls_assert();
		goto cleanup;
	}

 cleanup:
	gnutls_pubkey_deinit(pubkey);
	return ret;
}

/**
 * gnutls_x509_crt_get_crl_dist_points:
 * @cert: should contain a #gnutls_x509_crt_t type
 * @seq: specifies the sequence number of the distribution point (0 for the first one, 1 for the second etc.)
 * @san: is the place where the distribution point will be copied to
 * @san_size: holds the size of ret.
 * @reason_flags: Revocation reasons. An ORed sequence of flags from %gnutls_x509_crl_reason_flags_t.
 * @critical: will be non-zero if the extension is marked as critical (may be null)
 *
 * This function retrieves the CRL distribution points (2.5.29.31),
 * contained in the given certificate in the X509v3 Certificate
 * Extensions.
 *
 * Returns: %GNUTLS_E_SHORT_MEMORY_BUFFER and updates @ret_size if
 *   @ret_size is not enough to hold the distribution point, or the
 *   type of the distribution point if everything was ok. The type is
 *   one of the enumerated %gnutls_x509_subject_alt_name_t.  If the
 *   certificate does not have an Alternative name with the specified
 *   sequence number then %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE is
 *   returned.
 **/
int
gnutls_x509_crt_get_crl_dist_points(gnutls_x509_crt_t cert,
				    unsigned int seq, void *san,
				    size_t * san_size,
				    unsigned int *reason_flags,
				    unsigned int *critical)
{
	int ret;
	gnutls_datum_t dist_points = { NULL, 0 };
	unsigned type;
	gnutls_x509_crl_dist_points_t cdp = NULL;
	gnutls_datum_t t_san;

	if (cert == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	ret = gnutls_x509_crl_dist_points_init(&cdp);
	if (ret < 0)
		return gnutls_assert_val(ret);

	if (reason_flags)
		*reason_flags = 0;

	ret =
	    _gnutls_x509_crt_get_extension(cert, "2.5.29.31", 0,
					   &dist_points, critical);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	if (dist_points.size == 0 || dist_points.data == NULL) {
		gnutls_assert();
		ret = GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
		goto cleanup;
	}

	ret = gnutls_x509_ext_import_crl_dist_points(&dist_points, cdp, 0);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = gnutls_x509_crl_dist_points_get(cdp, seq, &type, &t_san, reason_flags);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = _gnutls_copy_string(&t_san, san, san_size);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = type;

 cleanup:
	_gnutls_free_datum(&dist_points);
	if (cdp != NULL)
		gnutls_x509_crl_dist_points_deinit(cdp);

	return ret;
}

/**
 * gnutls_x509_crt_get_key_purpose_oid:
 * @cert: should contain a #gnutls_x509_crt_t type
 * @indx: This specifies which OID to return. Use (0) to get the first one.
 * @oid: a pointer to a buffer to hold the OID (may be null)
 * @oid_size: initially holds the size of @oid
 * @critical: output flag to indicate criticality of extension
 *
 * This function will extract the key purpose OIDs of the Certificate
 * specified by the given index.  These are stored in the Extended Key
 * Usage extension (2.5.29.37) See the GNUTLS_KP_* definitions for
 * human readable names.
 *
 * If @oid is null then only the size will be filled. The @oid
 * returned will be null terminated, although @oid_size will not
 * account for the trailing null.
 *
 * Returns: %GNUTLS_E_SHORT_MEMORY_BUFFER if the provided buffer is
 *   not long enough, and in that case the *oid_size will be updated
 *   with the required size.  On success 0 is returned.
 **/
int
gnutls_x509_crt_get_key_purpose_oid(gnutls_x509_crt_t cert,
				    unsigned indx, void *oid, size_t * oid_size,
				    unsigned int *critical)
{
	int ret;
	gnutls_datum_t ext;
	gnutls_x509_key_purposes_t p = NULL;
	gnutls_datum_t out;

	if (cert == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	if (oid)
		memset(oid, 0, *oid_size);
	else
		*oid_size = 0;

	if ((ret =
	     _gnutls_x509_crt_get_extension(cert, "2.5.29.37", 0, &ext,
					    critical)) < 0) {
		return ret;
	}

	if (ext.size == 0 || ext.data == NULL) {
		gnutls_assert();
		return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
	}

	ret = gnutls_x509_key_purpose_init(&p);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = gnutls_x509_ext_import_key_purposes(&ext, p, 0);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = gnutls_x509_key_purpose_get(p, indx, &out);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = _gnutls_copy_string(&out, oid, oid_size);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = 0;

 cleanup:
	gnutls_free(ext.data);
	if (p!=NULL)
		gnutls_x509_key_purpose_deinit(p);
	return ret;
}

/**
 * gnutls_x509_crt_get_pk_rsa_raw:
 * @crt: Holds the certificate
 * @m: will hold the modulus
 * @e: will hold the public exponent
 *
 * This function will export the RSA public key's parameters found in
 * the given structure.  The new parameters will be allocated using
 * gnutls_malloc() and will be stored in the appropriate datum.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, otherwise a negative error code.
 **/
int
gnutls_x509_crt_get_pk_rsa_raw(gnutls_x509_crt_t crt,
			       gnutls_datum_t * m, gnutls_datum_t * e)
{
	int ret;
	gnutls_pubkey_t pubkey;

	if (crt == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	ret = gnutls_pubkey_init(&pubkey);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = gnutls_pubkey_import_x509(pubkey, crt, 0);
	if (ret < 0) {  
		gnutls_assert();
		goto cleanup;
	}

	ret = gnutls_pubkey_export_rsa_raw(pubkey, m, e);
	if (ret < 0) {  
		gnutls_assert();
		goto cleanup;
	}

 cleanup:
	gnutls_pubkey_deinit(pubkey);
	return ret;
}

/**
 * gnutls_x509_crt_get_pk_ecc_raw:
 * @crt: Holds the certificate
 * @curve: will hold the curve
 * @x: will hold the x-coordinate
 * @y: will hold the y-coordinate
 *
 * This function will export the ECC public key's parameters found in
 * the given certificate.  The new parameters will be allocated using
 * gnutls_malloc() and will be stored in the appropriate datum.
 *
 * In EdDSA curves the @y parameter will be %NULL and the other parameters
 * will be in the native format for the curve.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, otherwise a negative error code.
 *
 * Since: 3.4.1
 **/
int
gnutls_x509_crt_get_pk_ecc_raw(gnutls_x509_crt_t crt,
			       gnutls_ecc_curve_t *curve,
			       gnutls_datum_t *x, gnutls_datum_t *y)
{
	int ret;
	gnutls_pubkey_t pubkey;

	if (crt == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	ret = gnutls_pubkey_init(&pubkey);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = gnutls_pubkey_import_x509(pubkey, crt, 0);
	if (ret < 0) {  
		gnutls_assert();
		goto cleanup;
	}

	ret = gnutls_pubkey_export_ecc_raw(pubkey, curve, x, y);
	if (ret < 0) {  
		gnutls_assert();
		goto cleanup;
	}

 cleanup:
	gnutls_pubkey_deinit(pubkey);
	return ret;
}

/**
 * gnutls_x509_crt_get_pk_gost_raw:
 * @crt: Holds the certificate
 * @curve: will hold the curve
 * @digest: will hold the digest
 * @paramset: will hold the GOST parameter set ID
 * @x: will hold the x-coordinate
 * @y: will hold the y-coordinate
 *
 * This function will export the GOST public key's parameters found in
 * the given certificate.  The new parameters will be allocated using
 * gnutls_malloc() and will be stored in the appropriate datum.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, otherwise a negative error code.
 *
 * Since: 3.6.3
 **/
int
gnutls_x509_crt_get_pk_gost_raw(gnutls_x509_crt_t crt,
				gnutls_ecc_curve_t *curve,
				gnutls_digest_algorithm_t *digest,
				gnutls_gost_paramset_t *paramset,
				gnutls_datum_t *x, gnutls_datum_t *y)
{
	int ret;
	gnutls_pubkey_t pubkey;

	if (crt == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	ret = gnutls_pubkey_init(&pubkey);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = gnutls_pubkey_import_x509(pubkey, crt, 0);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = gnutls_pubkey_export_gost_raw2(pubkey, curve, digest,
					     paramset, x, y, 0);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

 cleanup:
	gnutls_pubkey_deinit(pubkey);
	return ret;
}

/**
 * gnutls_x509_crt_get_pk_dsa_raw:
 * @crt: Holds the certificate
 * @p: will hold the p
 * @q: will hold the q
 * @g: will hold the g
 * @y: will hold the y
 *
 * This function will export the DSA public key's parameters found in
 * the given certificate.  The new parameters will be allocated using
 * gnutls_malloc() and will be stored in the appropriate datum.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, otherwise a negative error code.
 **/
int
gnutls_x509_crt_get_pk_dsa_raw(gnutls_x509_crt_t crt,
			       gnutls_datum_t * p, gnutls_datum_t * q,
			       gnutls_datum_t * g, gnutls_datum_t * y)
{
	int ret;
	gnutls_pubkey_t pubkey;

	if (crt == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	ret = gnutls_pubkey_init(&pubkey);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = gnutls_pubkey_import_x509(pubkey, crt, 0);
	if (ret < 0) {  
		gnutls_assert();
		goto cleanup;
	}

	ret = gnutls_pubkey_export_dsa_raw(pubkey, p, q, g, y);
	if (ret < 0) {  
		gnutls_assert();
		goto cleanup;
	}

 cleanup:
	gnutls_pubkey_deinit(pubkey);
	return ret;
}

/**
 * gnutls_x509_crt_list_import2:
 * @certs: Will hold the parsed certificate list.
 * @size: It will contain the size of the list.
 * @data: The PEM encoded certificate.
 * @format: One of DER or PEM.
 * @flags: must be (0) or an OR'd sequence of gnutls_certificate_import_flags.
 *
 * This function will convert the given PEM encoded certificate list
 * to the native gnutls_x509_crt_t format. The output will be stored
 * in @certs which will be allocated and initialized.
 *
 * If the Certificate is PEM encoded it should have a header of "X509
 * CERTIFICATE", or "CERTIFICATE".
 *
 * To deinitialize @certs, you need to deinitialize each crt structure
 * independently, and use gnutls_free() at @certs.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, otherwise a negative error code.
 *
 * Since: 3.0
 **/
int
gnutls_x509_crt_list_import2(gnutls_x509_crt_t ** certs,
			     unsigned int *size,
			     const gnutls_datum_t * data,
			     gnutls_x509_crt_fmt_t format,
			     unsigned int flags)
{
	unsigned int init = 1024;
	int ret;

	*certs = gnutls_malloc(sizeof(gnutls_x509_crt_t) * init);
	if (*certs == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	ret =
	    gnutls_x509_crt_list_import(*certs, &init, data, format,
					flags | GNUTLS_X509_CRT_LIST_IMPORT_FAIL_IF_EXCEED);
	if (ret == GNUTLS_E_SHORT_MEMORY_BUFFER) {
		*certs =
		    gnutls_realloc_fast(*certs,
					sizeof(gnutls_x509_crt_t) * init);
		if (*certs == NULL) {
			gnutls_assert();
			return GNUTLS_E_MEMORY_ERROR;
		}

		ret =
		    gnutls_x509_crt_list_import(*certs, &init, data,
						format, flags);
	}

	if (ret < 0) {
		gnutls_free(*certs);
		return ret;
	}

	*size = init;
	return 0;
}

/**
 * gnutls_x509_crt_list_import:
 * @certs: Indicates where the parsed list will be copied to. Must not be initialized.
 * @cert_max: Initially must hold the maximum number of certs. It will be updated with the number of certs available.
 * @data: The PEM encoded certificate.
 * @format: One of DER or PEM.
 * @flags: must be (0) or an OR'd sequence of gnutls_certificate_import_flags.
 *
 * This function will convert the given PEM encoded certificate list
 * to the native gnutls_x509_crt_t format. The output will be stored
 * in @certs.  They will be automatically initialized.
 *
 * The flag %GNUTLS_X509_CRT_LIST_IMPORT_FAIL_IF_EXCEED will cause
 * import to fail if the certificates in the provided buffer are more
 * than the available structures. The %GNUTLS_X509_CRT_LIST_FAIL_IF_UNSORTED
 * flag will cause the function to fail if the provided list is not
 * sorted from subject to issuer.
 *
 * If the Certificate is PEM encoded it should have a header of "X509
 * CERTIFICATE", or "CERTIFICATE".
 *
 * Returns: the number of certificates read or a negative error value.
 **/
int
gnutls_x509_crt_list_import(gnutls_x509_crt_t * certs,
			    unsigned int *cert_max,
			    const gnutls_datum_t * data,
			    gnutls_x509_crt_fmt_t format,
			    unsigned int flags)
{
	int size;
	const char *ptr;
	gnutls_datum_t tmp;
	int ret, nocopy = 0;
	unsigned int count = 0, j, copied = 0;

	if (format == GNUTLS_X509_FMT_DER) {
		if (*cert_max < 1) {
			*cert_max = 1;
			return GNUTLS_E_SHORT_MEMORY_BUFFER;
		}

		count = 1;	/* import only the first one */

		ret = gnutls_x509_crt_init(&certs[0]);
		if (ret < 0) {
			gnutls_assert();
			goto error;
		}

		ret = gnutls_x509_crt_import(certs[0], data, format);
		if (ret < 0) {
			gnutls_assert();
			goto error;
		}

		*cert_max = 1;
		return 1;
	}

	/* move to the certificate
	 */
	ptr = memmem(data->data, data->size,
		     PEM_CERT_SEP, sizeof(PEM_CERT_SEP) - 1);
	if (ptr == NULL)
		ptr = memmem(data->data, data->size,
			     PEM_CERT_SEP2, sizeof(PEM_CERT_SEP2) - 1);

	if (ptr == NULL)
		return gnutls_assert_val(GNUTLS_E_NO_CERTIFICATE_FOUND);

	count = 0;

	do {
		if (count >= *cert_max) {
			if (!
			    (flags &
			     GNUTLS_X509_CRT_LIST_IMPORT_FAIL_IF_EXCEED))
				break;
			else
				nocopy = 1;
		}

		if (!nocopy) {
			ret = gnutls_x509_crt_init(&certs[count]);
			if (ret < 0) {
				gnutls_assert();
				goto error;
			}

			tmp.data = (void *) ptr;
			tmp.size =
			    data->size - (ptr - (char *) data->data);

			ret =
			    gnutls_x509_crt_import(certs[count], &tmp,
						   GNUTLS_X509_FMT_PEM);
			if (ret < 0) {
				count++;
				gnutls_assert();
				goto error;
			}

			copied++;
		}

		/* now we move ptr after the pem header 
		 */
		ptr++;
		/* find the next certificate (if any)
		 */
		size = data->size - (ptr - (char *) data->data);

		if (size > 0) {
			char *ptr2;

			ptr2 =
			    memmem(ptr, size, PEM_CERT_SEP,
				   sizeof(PEM_CERT_SEP) - 1);
			if (ptr2 == NULL)
				ptr2 = memmem(ptr, size, PEM_CERT_SEP2,
					      sizeof(PEM_CERT_SEP2) - 1);

			ptr = ptr2;
		} else
			ptr = NULL;

		count++;
	}
	while (ptr != NULL);

	*cert_max = count;

	if (nocopy == 0) {
		if (flags & GNUTLS_X509_CRT_LIST_SORT && *cert_max > 1) {
			gnutls_x509_crt_t sorted[DEFAULT_MAX_VERIFY_DEPTH];
			gnutls_x509_crt_t *s;

			s = _gnutls_sort_clist(sorted, certs, cert_max, gnutls_x509_crt_deinit);
			if (s == certs) {
				gnutls_assert();
				ret = GNUTLS_E_UNIMPLEMENTED_FEATURE;
				goto error;
			}

			count = *cert_max;
			if (s == sorted) {
				memcpy(certs, s, (*cert_max)*sizeof(gnutls_x509_crt_t));
			}
		}

		if (flags & GNUTLS_X509_CRT_LIST_FAIL_IF_UNSORTED) {
			ret = _gnutls_check_if_sorted(certs, *cert_max);
			if (ret < 0) {
				gnutls_assert();
				goto error;
			}
		}

		return count;
	} else {
		count = copied;
		ret = GNUTLS_E_SHORT_MEMORY_BUFFER;
	}

      error:
	for (j = 0; j < count; j++)
		gnutls_x509_crt_deinit(certs[j]);
	return ret;
}

/**
 * gnutls_x509_crt_get_subject_unique_id:
 * @crt: Holds the certificate
 * @buf: user allocated memory buffer, will hold the unique id
 * @buf_size: size of user allocated memory buffer (on input), will hold
 * actual size of the unique ID on return.
 *
 * This function will extract the subjectUniqueID value (if present) for
 * the given certificate.
 *
 * If the user allocated memory buffer is not large enough to hold the
 * full subjectUniqueID, then a GNUTLS_E_SHORT_MEMORY_BUFFER error will be
 * returned, and buf_size will be set to the actual length.
 *
 * This function had a bug prior to 3.4.8 that prevented the setting
 * of %NULL @buf to discover the @buf_size. To use this function safely
 * with the older versions the @buf must be a valid buffer that can hold
 * at least a single byte if @buf_size is zero.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, otherwise a negative error code.
 **/
int
gnutls_x509_crt_get_subject_unique_id(gnutls_x509_crt_t crt, char *buf,
				      size_t * buf_size)
{
	int result;
	gnutls_datum_t datum = { NULL, 0 };

	result =
	    _gnutls_x509_read_value(crt->cert,
				    "tbsCertificate.subjectUniqueID",
				    &datum);
	if (result < 0)
		return gnutls_assert_val(result);

	if (datum.size > *buf_size) {	/* then we're not going to fit */
		*buf_size = datum.size;
		result = GNUTLS_E_SHORT_MEMORY_BUFFER;
	} else {
		*buf_size = datum.size;
		memcpy(buf, datum.data, datum.size);
	}

	_gnutls_free_datum(&datum);

	return result;
}

/**
 * gnutls_x509_crt_get_issuer_unique_id:
 * @crt: Holds the certificate
 * @buf: user allocated memory buffer, will hold the unique id
 * @buf_size: size of user allocated memory buffer (on input), will hold
 * actual size of the unique ID on return.
 *
 * This function will extract the issuerUniqueID value (if present) for
 * the given certificate.
 *
 * If the user allocated memory buffer is not large enough to hold the
 * full subjectUniqueID, then a GNUTLS_E_SHORT_MEMORY_BUFFER error will be
 * returned, and buf_size will be set to the actual length.
 *
 * This function had a bug prior to 3.4.8 that prevented the setting
 * of %NULL @buf to discover the @buf_size. To use this function safely
 * with the older versions the @buf must be a valid buffer that can hold
 * at least a single byte if @buf_size is zero.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, otherwise a negative error code.
 *
 * Since: 2.12.0
 **/
int
gnutls_x509_crt_get_issuer_unique_id(gnutls_x509_crt_t crt, char *buf,
				     size_t * buf_size)
{
	int result;
	gnutls_datum_t datum = { NULL, 0 };

	result =
	    _gnutls_x509_read_value(crt->cert,
				    "tbsCertificate.issuerUniqueID",
				    &datum);
	if (result < 0)
		return gnutls_assert_val(result);

	if (datum.size > *buf_size) {	/* then we're not going to fit */
		*buf_size = datum.size;
		result = GNUTLS_E_SHORT_MEMORY_BUFFER;
	} else {
		*buf_size = datum.size;
		memcpy(buf, datum.data, datum.size);
	}

	_gnutls_free_datum(&datum);

	return result;
}

static int
legacy_parse_aia(ASN1_TYPE src,
		  unsigned int seq, int what, gnutls_datum_t * data)
{
	int len;
	char nptr[MAX_NAME_SIZE];
	int result;
	gnutls_datum_t d;
	const char *oid = NULL;

	seq++;			/* 0->1, 1->2 etc */
	switch (what) {
	case GNUTLS_IA_ACCESSMETHOD_OID:
		snprintf(nptr, sizeof(nptr), "?%u.accessMethod", seq);
		break;

	case GNUTLS_IA_ACCESSLOCATION_GENERALNAME_TYPE:
		snprintf(nptr, sizeof(nptr), "?%u.accessLocation", seq);
		break;

	case GNUTLS_IA_CAISSUERS_URI:
		oid = GNUTLS_OID_AD_CAISSUERS;
		FALLTHROUGH;

	case GNUTLS_IA_OCSP_URI:
		if (oid == NULL)
			oid = GNUTLS_OID_AD_OCSP;
		{
			char tmpoid[MAX_OID_SIZE];
			snprintf(nptr, sizeof(nptr), "?%u.accessMethod",
				 seq);
			len = sizeof(tmpoid);
			result = asn1_read_value(src, nptr, tmpoid, &len);

			if (result == ASN1_VALUE_NOT_FOUND
			    || result == ASN1_ELEMENT_NOT_FOUND)
				return
				    gnutls_assert_val
				    (GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE);

			if (result != ASN1_SUCCESS) {
				gnutls_assert();
				return _gnutls_asn2err(result);
			}
			if ((unsigned) len != strlen(oid) + 1
			    || memcmp(tmpoid, oid, len) != 0)
				return
				    gnutls_assert_val
				    (GNUTLS_E_UNKNOWN_ALGORITHM);
		}
		FALLTHROUGH;

	case GNUTLS_IA_URI:
		snprintf(nptr, sizeof(nptr),
			 "?%u.accessLocation.uniformResourceIdentifier",
			 seq);
		break;

	default:
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);
	}

	len = 0;
	result = asn1_read_value(src, nptr, NULL, &len);
	if (result == ASN1_VALUE_NOT_FOUND
	    || result == ASN1_ELEMENT_NOT_FOUND)
		return
		    gnutls_assert_val
		    (GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE);

	if (result != ASN1_MEM_ERROR) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	d.size = len;

	d.data = gnutls_malloc(d.size);
	if (d.data == NULL)
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

	result = asn1_read_value(src, nptr, d.data, &len);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		gnutls_free(d.data);
		return _gnutls_asn2err(result);
	}

	if (data) {
		data->data = d.data;
		data->size = d.size;
	} else
		gnutls_free(d.data);

	return 0;
}

/**
 * gnutls_x509_crt_get_authority_info_access:
 * @crt: Holds the certificate
 * @seq: specifies the sequence number of the access descriptor (0 for the first one, 1 for the second etc.)
 * @what: what data to get, a #gnutls_info_access_what_t type.
 * @data: output data to be freed with gnutls_free().
 * @critical: pointer to output integer that is set to non-zero if the extension is marked as critical (may be %NULL)
 *
 * Note that a simpler API to access the authority info data is provided
 * by gnutls_x509_aia_get() and gnutls_x509_ext_import_aia().
 * 
 * This function extracts the Authority Information Access (AIA)
 * extension, see RFC 5280 section 4.2.2.1 for more information.  The
 * AIA extension holds a sequence of AccessDescription (AD) data.
 *
 * The @seq input parameter is used to indicate which member of the
 * sequence the caller is interested in.  The first member is 0, the
 * second member 1 and so on.  When the @seq value is out of bounds,
 * %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE is returned.
 *
 * The type of data returned in @data is specified via @what which
 * should be #gnutls_info_access_what_t values.
 *
 * If @what is %GNUTLS_IA_ACCESSMETHOD_OID then @data will hold the
 * accessMethod OID (e.g., "1.3.6.1.5.5.7.48.1").
 *
 * If @what is %GNUTLS_IA_ACCESSLOCATION_GENERALNAME_TYPE, @data will
 * hold the accessLocation GeneralName type (e.g.,
 * "uniformResourceIdentifier").
 *
 * If @what is %GNUTLS_IA_URI, @data will hold the accessLocation URI
 * data.  Requesting this @what value leads to an error if the
 * accessLocation is not of the "uniformResourceIdentifier" type. 
 *
 * If @what is %GNUTLS_IA_OCSP_URI, @data will hold the OCSP URI.
 * Requesting this @what value leads to an error if the accessMethod
 * is not 1.3.6.1.5.5.7.48.1 aka OCSP, or if accessLocation is not of
 * the "uniformResourceIdentifier" type. In that case %GNUTLS_E_UNKNOWN_ALGORITHM
 * will be returned, and @seq should be increased and this function
 * called again.
 *
 * If @what is %GNUTLS_IA_CAISSUERS_URI, @data will hold the caIssuers
 * URI.  Requesting this @what value leads to an error if the
 * accessMethod is not 1.3.6.1.5.5.7.48.2 aka caIssuers, or if
 * accessLocation is not of the "uniformResourceIdentifier" type.
 * In that case handle as in %GNUTLS_IA_OCSP_URI.
 *
 * More @what values may be allocated in the future as needed.
 *
 * If @data is NULL, the function does the same without storing the
 * output data, that is, it will set @critical and do error checking
 * as usual.
 *
 * The value of the critical flag is returned in *@critical.  Supply a
 * NULL @critical if you want the function to make sure the extension
 * is non-critical, as required by RFC 5280.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, %GNUTLS_E_INVALID_REQUEST on
 * invalid @crt, %GNUTLS_E_CONSTRAINT_ERROR if the extension is
 * incorrectly marked as critical (use a non-NULL @critical to
 * override), %GNUTLS_E_UNKNOWN_ALGORITHM if the requested OID does
 * not match (e.g., when using %GNUTLS_IA_OCSP_URI), otherwise a
 * negative error code.
 *
 * Since: 3.0
 **/
int
gnutls_x509_crt_get_authority_info_access(gnutls_x509_crt_t crt,
					  unsigned int seq,
					  int what,
					  gnutls_datum_t * data,
					  unsigned int *critical)
{
	int ret;
	gnutls_datum_t aia;
	ASN1_TYPE c2 = ASN1_TYPE_EMPTY;

	if (crt == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	if ((ret =
	     _gnutls_x509_crt_get_extension(crt, GNUTLS_OID_AIA, 0, &aia,
					    critical)) < 0)
		return ret;

	if (aia.size == 0 || aia.data == NULL) {
		gnutls_assert();
		return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
	}

	if (critical && *critical)
		return GNUTLS_E_CONSTRAINT_ERROR;

	ret = asn1_create_element(_gnutls_get_pkix(),
				  "PKIX1.AuthorityInfoAccessSyntax", &c2);
	if (ret != ASN1_SUCCESS) {
		gnutls_assert();
		_gnutls_free_datum(&aia);
		return _gnutls_asn2err(ret);
	}

	ret = _asn1_strict_der_decode(&c2, aia.data, aia.size, NULL);
	/* asn1_print_structure (stdout, c2, "", ASN1_PRINT_ALL); */
	_gnutls_free_datum(&aia);
	if (ret != ASN1_SUCCESS) {
		gnutls_assert();
		asn1_delete_structure(&c2);
		return _gnutls_asn2err(ret);
	}

	ret = legacy_parse_aia(c2, seq, what, data);

	asn1_delete_structure(&c2);
	if (ret < 0)
		gnutls_assert();

	return ret;
}

/**
 * gnutls_x509_crt_set_pin_function:
 * @crt: The certificate structure
 * @fn: the callback
 * @userdata: data associated with the callback
 *
 * This function will set a callback function to be used when
 * it is required to access a protected object. This function overrides 
 * the global function set using gnutls_pkcs11_set_pin_function().
 *
 * Note that this callback is currently used only during the import
 * of a PKCS #11 certificate with gnutls_x509_crt_import_url().
 *
 * Since: 3.1.0
 *
 **/
void gnutls_x509_crt_set_pin_function(gnutls_x509_crt_t crt,
				      gnutls_pin_callback_t fn,
				      void *userdata)
{
	if (crt) {
		crt->pin.cb = fn;
		crt->pin.data = userdata;
	}
}

/**
 * gnutls_x509_crt_import_url:
 * @crt: A certificate of type #gnutls_x509_crt_t
 * @url: A PKCS 11 url
 * @flags: One of GNUTLS_PKCS11_OBJ_* flags for PKCS#11 URLs or zero otherwise
 *
 * This function will import a certificate present in a PKCS#11 token
 * or any type of back-end that supports URLs.
 *
 * In previous versions of gnutls this function was named
 * gnutls_x509_crt_import_pkcs11_url, and the old name is
 * an alias to this one.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.4.0
 **/
int
gnutls_x509_crt_import_url(gnutls_x509_crt_t crt,
			   const char *url, unsigned int flags)
{
	int ret;
	unsigned i;

	for (i=0;i<_gnutls_custom_urls_size;i++) {
		if (strncmp(url, _gnutls_custom_urls[i].name, _gnutls_custom_urls[i].name_size) == 0) {
			if (_gnutls_custom_urls[i].import_crt) {
				ret = _gnutls_custom_urls[i].import_crt(crt, url, flags);
				goto cleanup;
			}
			break;
		}
	}

	if (strncmp(url, SYSTEM_URL, SYSTEM_URL_SIZE) == 0) {
		ret = _gnutls_x509_crt_import_system_url(crt, url);
#ifdef ENABLE_PKCS11
	} else if (strncmp(url, PKCS11_URL, PKCS11_URL_SIZE) == 0) {
			ret = _gnutls_x509_crt_import_pkcs11_url(crt, url, flags);
#endif
	} else {
		ret = gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);
	}

 cleanup:
	return ret;
}

/**
 * gnutls_x509_crt_list_import_url:
 * @certs: Will hold the allocated certificate list.
 * @size: It will contain the size of the list.
 * @url: A PKCS 11 url
 * @pin_fn: a PIN callback if not globally set
 * @pin_fn_userdata: parameter for the PIN callback
 * @flags: One of GNUTLS_PKCS11_OBJ_* flags for PKCS#11 URLs or zero otherwise
 *
 * This function will import a certificate chain present in a PKCS#11 token
 * or any type of back-end that supports URLs. The certificates
 * must be deinitialized afterwards using gnutls_x509_crt_deinit()
 * and the returned pointer must be freed using gnutls_free().
 *
 * The URI provided must be the first certificate in the chain; subsequent
 * certificates will be retrieved using gnutls_pkcs11_get_raw_issuer() or
 * equivalent functionality for the supported URI.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.6.3
 **/
int
gnutls_x509_crt_list_import_url(gnutls_x509_crt_t **certs,
				unsigned int *size,
				const char *url,
				gnutls_pin_callback_t pin_fn,
				void *pin_fn_userdata,
				unsigned int flags)
{
	int ret;
	unsigned i;
	gnutls_x509_crt_t crts[DEFAULT_MAX_VERIFY_DEPTH];
	gnutls_datum_t issuer = {NULL, 0};
	unsigned total = 0;

	memset(crts, 0, sizeof(crts));

	ret = gnutls_x509_crt_init(&crts[0]);
	if (ret < 0)
		return gnutls_assert_val(ret);

	gnutls_x509_crt_set_pin_function(crts[0], pin_fn, pin_fn_userdata);

	total = 1;

	ret = gnutls_x509_crt_import_url(crts[0], url, flags);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	for (i=1;i<DEFAULT_MAX_VERIFY_DEPTH;i++) {
		ret = _gnutls_get_raw_issuer(url, crts[i-1], &issuer, flags|GNUTLS_PKCS11_OBJ_FLAG_RETRIEVE_ANY);
		if (ret < 0) {
			issuer.data = NULL;
			break;
		}

		if (gnutls_x509_crt_equals2(crts[i-1], &issuer)) {
			gnutls_free(issuer.data);
			break;
		}

		ret = gnutls_x509_crt_init(&crts[i]);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		total++;

		gnutls_x509_crt_set_pin_function(crts[i], pin_fn, pin_fn_userdata);

		ret = gnutls_x509_crt_import(crts[i], &issuer, GNUTLS_X509_FMT_DER);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		gnutls_free(issuer.data);
	}

	*certs = gnutls_malloc(total*sizeof(gnutls_x509_crt_t));
	if (*certs == NULL) {
		ret = GNUTLS_E_MEMORY_ERROR;
		goto cleanup;
	}

	memcpy(*certs, crts, total*sizeof(gnutls_x509_crt_t));
	*size = total;

	return 0;
 cleanup:
	gnutls_free(issuer.data);
	for (i=0;i<total;i++)
		gnutls_x509_crt_deinit(crts[i]);

	return ret;
}

/*-
 * gnutls_x509_crt_verify_data3:
 * @crt: Holds the certificate to verify with
 * @algo: The signature algorithm used
 * @flags: Zero or an OR list of #gnutls_certificate_verify_flags
 * @data: holds the signed data
 * @signature: contains the signature
 *
 * This function will verify the given signed data, using the
 * parameters from the certificate.
 *
 * Returns: In case of a verification failure %GNUTLS_E_PK_SIG_VERIFY_FAILED 
 * is returned, %GNUTLS_E_EXPIRED or %GNUTLS_E_NOT_YET_ACTIVATED on expired
 * or not yet activated certificate and zero or positive code on success.
 *
 * Since: 3.5.6
 -*/
int
gnutls_x509_crt_verify_data3(gnutls_x509_crt_t crt,
			     gnutls_sign_algorithm_t algo,
			     gnutls_typed_vdata_st *vdata,
			     unsigned int vdata_size,
			     const gnutls_datum_t *data,
			     const gnutls_datum_t *signature,
			     unsigned int flags)
{
	int ret;
	gnutls_pubkey_t pubkey;

	if (crt == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}


	ret = gnutls_pubkey_init(&pubkey);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = gnutls_pubkey_import_x509(pubkey, crt, 0);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = gnutls_pubkey_verify_data2(pubkey, algo, flags, data, signature);
	gnutls_pubkey_deinit(pubkey);

	if (ret >= 0) {
		time_t now = gnutls_time(0);
		int res;
		unsigned usage, i;

		if (!(flags & GNUTLS_VERIFY_DISABLE_TRUSTED_TIME_CHECKS) ||
		    !(flags & GNUTLS_VERIFY_DISABLE_TIME_CHECKS)) {
			if (now > gnutls_x509_crt_get_expiration_time(crt)) {
				return gnutls_assert_val(GNUTLS_E_EXPIRED);
			}

			if (now < gnutls_x509_crt_get_activation_time(crt)) {
				return gnutls_assert_val(GNUTLS_E_NOT_YET_ACTIVATED);
			}
		}

		res = gnutls_x509_crt_get_key_usage(crt, &usage, NULL);
		if (res >= 0) {
			if (!(usage & GNUTLS_KEY_DIGITAL_SIGNATURE)) {
				return gnutls_assert_val(GNUTLS_CERT_SIGNER_CONSTRAINTS_FAILURE);
			}
		}

		for (i=0;i<vdata_size;i++) {
			if (vdata[i].type == GNUTLS_DT_KEY_PURPOSE_OID) {
				res = _gnutls_check_key_purpose(crt, (char *)vdata[i].data, 0);
				if (res == 0)
					return gnutls_assert_val(GNUTLS_CERT_SIGNER_CONSTRAINTS_FAILURE);
				break;
			}
		}
	}

	return ret;
}

/**
 * gnutls_x509_crt_verify_data2:
 * @crt: Holds the certificate to verify with
 * @algo: The signature algorithm used
 * @flags: Zero or an OR list of #gnutls_certificate_verify_flags
 * @data: holds the signed data
 * @signature: contains the signature
 *
 * This function will verify the given signed data, using the
 * parameters from the certificate.
 *
 * Returns: In case of a verification failure %GNUTLS_E_PK_SIG_VERIFY_FAILED 
 * is returned, %GNUTLS_E_EXPIRED or %GNUTLS_E_NOT_YET_ACTIVATED on expired
 * or not yet activated certificate and zero or positive code on success.
 *
 * Note that since GnuTLS 3.5.6 this function introduces checks in the
 * end certificate (@crt), including time checks and key usage checks.
 *
 * Since: 3.4.0
 **/
int
gnutls_x509_crt_verify_data2(gnutls_x509_crt_t crt,
			   gnutls_sign_algorithm_t algo,
			   unsigned int flags,
			   const gnutls_datum_t *data,
			   const gnutls_datum_t *signature)
{
	return gnutls_x509_crt_verify_data3(crt, algo, NULL, 0,
				data, signature, flags);
}

/**
 * gnutls_x509_crt_set_flags:
 * @cert: A type #gnutls_x509_crt_t
 * @flags: flags from the %gnutls_x509_crt_flags
 *
 * This function will set flags for the specified certificate.
 * Currently this is useful for the %GNUTLS_X509_CRT_FLAG_IGNORE_SANITY
 * which allows importing certificates even if they have known issues.
 *
 * Since: 3.6.0
 *
 **/
void gnutls_x509_crt_set_flags(gnutls_x509_crt_t cert,
				   unsigned int flags)
{
	cert->flags = flags;
}

