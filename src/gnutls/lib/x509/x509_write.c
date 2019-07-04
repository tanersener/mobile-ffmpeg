/*
 * Copyright (C) 2003-2016 Free Software Foundation, Inc.
 * Copyright (C) 2016-2017 Red Hat, Inc.
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

/* This file contains functions to handle X.509 certificate generation.
 */

#include "gnutls_int.h"

#include <datum.h>
#include <global.h>
#include "errors.h"
#include <common.h>
#include <x509.h>
#include <gnutls/x509-ext.h>
#include <x509_b64.h>
#include "x509_int.h"
#include <libtasn1.h>
#include <pk.h>

static void disable_optional_stuff(gnutls_x509_crt_t cert);

/**
 * gnutls_x509_crt_set_dn_by_oid:
 * @crt: a certificate of type #gnutls_x509_crt_t
 * @oid: holds an Object Identifier in a null terminated string
 * @raw_flag: must be 0, or 1 if the data are DER encoded
 * @name: a pointer to the name
 * @sizeof_name: holds the size of @name
 *
 * This function will set the part of the name of the Certificate
 * subject, specified by the given OID. The input string should be
 * ASCII or UTF-8 encoded.
 *
 * Some helper macros with popular OIDs can be found in gnutls/x509.h
 * With this function you can only set the known OIDs. You can test
 * for known OIDs using gnutls_x509_dn_oid_known(). For OIDs that are
 * not known (by gnutls) you should properly DER encode your data,
 * and call this function with @raw_flag set.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_x509_crt_set_dn_by_oid(gnutls_x509_crt_t crt, const char *oid,
			      unsigned int raw_flag, const void *name,
			      unsigned int sizeof_name)
{
	if (sizeof_name == 0 || name == NULL || crt == NULL) {
		return GNUTLS_E_INVALID_REQUEST;
	}

	MODIFIED(crt);

	return _gnutls_x509_set_dn_oid(crt->cert, "tbsCertificate.subject",
				       oid, raw_flag, name, sizeof_name);
}

/**
 * gnutls_x509_crt_set_issuer_dn_by_oid:
 * @crt: a certificate of type #gnutls_x509_crt_t
 * @oid: holds an Object Identifier in a null terminated string
 * @raw_flag: must be 0, or 1 if the data are DER encoded
 * @name: a pointer to the name
 * @sizeof_name: holds the size of @name
 *
 * This function will set the part of the name of the Certificate
 * issuer, specified by the given OID.  The input string should be
 * ASCII or UTF-8 encoded.
 *
 * Some helper macros with popular OIDs can be found in gnutls/x509.h
 * With this function you can only set the known OIDs. You can test
 * for known OIDs using gnutls_x509_dn_oid_known(). For OIDs that are
 * not known (by gnutls) you should properly DER encode your data,
 * and call this function with @raw_flag set.
 *
 * Normally you do not need to call this function, since the signing
 * operation will copy the signer's name as the issuer of the
 * certificate.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_x509_crt_set_issuer_dn_by_oid(gnutls_x509_crt_t crt,
				     const char *oid,
				     unsigned int raw_flag,
				     const void *name,
				     unsigned int sizeof_name)
{
	if (sizeof_name == 0 || name == NULL || crt == NULL) {
		return GNUTLS_E_INVALID_REQUEST;
	}

	MODIFIED(crt);

	return _gnutls_x509_set_dn_oid(crt->cert, "tbsCertificate.issuer",
				       oid, raw_flag, name, sizeof_name);
}

/**
 * gnutls_x509_crt_set_proxy_dn:
 * @crt: a gnutls_x509_crt_t type with the new proxy cert
 * @eecrt: the end entity certificate that will be issuing the proxy
 * @raw_flag: must be 0, or 1 if the CN is DER encoded
 * @name: a pointer to the CN name, may be NULL (but MUST then be added later)
 * @sizeof_name: holds the size of @name
 *
 * This function will set the subject in @crt to the end entity's
 * @eecrt subject name, and add a single Common Name component @name
 * of size @sizeof_name.  This corresponds to the required proxy
 * certificate naming style.  Note that if @name is %NULL, you MUST
 * set it later by using gnutls_x509_crt_set_dn_by_oid() or similar.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_x509_crt_set_proxy_dn(gnutls_x509_crt_t crt,
			     gnutls_x509_crt_t eecrt,
			     unsigned int raw_flag, const void *name,
			     unsigned int sizeof_name)
{
	int result;

	if (crt == NULL || eecrt == NULL) {
		return GNUTLS_E_INVALID_REQUEST;
	}

	MODIFIED(crt);

	result = asn1_copy_node(crt->cert, "tbsCertificate.subject",
				eecrt->cert, "tbsCertificate.subject");
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	if (name && sizeof_name) {
		return _gnutls_x509_set_dn_oid(crt->cert,
					       "tbsCertificate.subject",
					       GNUTLS_OID_X520_COMMON_NAME,
					       raw_flag, name,
					       sizeof_name);
	}

	return 0;
}

/**
 * gnutls_x509_crt_set_version:
 * @crt: a certificate of type #gnutls_x509_crt_t
 * @version: holds the version number. For X.509v1 certificates must be 1.
 *
 * This function will set the version of the certificate.  This must
 * be one for X.509 version 1, and so on.  Plain certificates without
 * extensions must have version set to one.
 *
 * To create well-formed certificates, you must specify version 3 if
 * you use any certificate extensions.  Extensions are created by
 * functions such as gnutls_x509_crt_set_subject_alt_name()
 * or gnutls_x509_crt_set_key_usage().
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_x509_crt_set_version(gnutls_x509_crt_t crt, unsigned int version)
{
	int result;
	unsigned char null = version;

	if (crt == NULL || version == 0 || version >= 0x80) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	MODIFIED(crt);

	if (null > 0)
		null--;

	result =
	    asn1_write_value(crt->cert, "tbsCertificate.version", &null,
			     1);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	return 0;
}

/**
 * gnutls_x509_crt_set_key:
 * @crt: a certificate of type #gnutls_x509_crt_t
 * @key: holds a private key
 *
 * This function will set the public parameters from the given
 * private key to the certificate.
 *
 * To export the public key (i.e., the SubjectPublicKeyInfo part), check
 * gnutls_pubkey_import_x509().
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 **/
int
gnutls_x509_crt_set_key(gnutls_x509_crt_t crt, gnutls_x509_privkey_t key)
{
	int result;

	if (crt == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	MODIFIED(crt);

	result = _gnutls_x509_encode_and_copy_PKI_params(crt->cert,
							 "tbsCertificate.subjectPublicKeyInfo",
							 &key->params);

	if (result < 0) {
		gnutls_assert();
		return result;
	}

	return 0;
}

/**
 * gnutls_x509_crt_set_crq:
 * @crt: a certificate of type #gnutls_x509_crt_t
 * @crq: holds a certificate request
 *
 * This function will set the name and public parameters as well as
 * the extensions from the given certificate request to the certificate. 
 * Only RSA keys are currently supported.
 *
 * Note that this function will only set the @crq if it is self
 * signed and the signature is correct. See gnutls_x509_crq_sign2().
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int gnutls_x509_crt_set_crq(gnutls_x509_crt_t crt, gnutls_x509_crq_t crq)
{
	int result;

	if (crt == NULL || crq == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	MODIFIED(crt);

	result = gnutls_x509_crq_verify(crq, 0);
	if (result < 0)
		return gnutls_assert_val(result);

	result = asn1_copy_node(crt->cert, "tbsCertificate.subject",
				crq->crq,
				"certificationRequestInfo.subject");
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	result =
	    asn1_copy_node(crt->cert,
			   "tbsCertificate.subjectPublicKeyInfo", crq->crq,
			   "certificationRequestInfo.subjectPKInfo");
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	return 0;
}

/**
 * gnutls_x509_crt_set_crq_extensions:
 * @crt: a certificate of type #gnutls_x509_crt_t
 * @crq: holds a certificate request
 *
 * This function will set the extensions from the given request to the
 * certificate.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 2.8.0
 **/
int
gnutls_x509_crt_set_crq_extensions(gnutls_x509_crt_t crt,
				   gnutls_x509_crq_t crq)
{
	return gnutls_x509_crt_set_crq_extension_by_oid(crt, crq, NULL, 0);
}

/**
 * gnutls_x509_crt_set_crq_extension_by_oid:
 * @crt: a certificate of type #gnutls_x509_crt_t
 * @crq: holds a certificate request
 * @oid: the object identifier of the OID to copy
 * @flags: should be zero
 *
 * This function will set the extension specify by @oid from the given request to the
 * certificate.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.5.1
 **/
int
gnutls_x509_crt_set_crq_extension_by_oid(gnutls_x509_crt_t crt,
					 gnutls_x509_crq_t crq, const char *oid,
					 unsigned flags)
{
	size_t i;

	if (crt == NULL || crq == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	MODIFIED(crt);

	for (i = 0;; i++) {
		int result;
		char local_oid[MAX_OID_SIZE];
		size_t local_oid_size;
		uint8_t *extensions;
		size_t extensions_size;
		unsigned int critical;
		gnutls_datum_t ext;

		local_oid_size = sizeof(local_oid);
		result = gnutls_x509_crq_get_extension_info(crq, i, local_oid,
							    &local_oid_size,
							    &critical);
		if (result < 0) {
			if (result ==
			    GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE)
				break;

			gnutls_assert();
			return result;
		}

		if (oid && strcmp(local_oid, oid) != 0)
			continue;

		extensions_size = 0;
		result = gnutls_x509_crq_get_extension_data(crq, i, NULL,
							    &extensions_size);
		if (result < 0) {
			gnutls_assert();
			return result;
		}

		extensions = gnutls_malloc(extensions_size);
		if (extensions == NULL) {
			gnutls_assert();
			return GNUTLS_E_MEMORY_ERROR;
		}

		result =
		    gnutls_x509_crq_get_extension_data(crq, i, extensions,
						       &extensions_size);
		if (result < 0) {
			gnutls_assert();
			gnutls_free(extensions);
			return result;
		}

		ext.data = extensions;
		ext.size = extensions_size;

		result =
		    _gnutls_x509_crt_set_extension(crt, local_oid, &ext,
						   critical);
		gnutls_free(extensions);
		if (result < 0) {
			gnutls_assert();
			return result;
		}
	}

	return 0;
}

/**
 * gnutls_x509_crt_set_extension_by_oid:
 * @crt: a certificate of type #gnutls_x509_crt_t
 * @oid: holds an Object Identifier in null terminated string
 * @buf: a pointer to a DER encoded data
 * @sizeof_buf: holds the size of @buf
 * @critical: should be non-zero if the extension is to be marked as critical
 *
 * This function will set an the extension, by the specified OID, in
 * the certificate.  The extension data should be binary data DER
 * encoded.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_x509_crt_set_extension_by_oid(gnutls_x509_crt_t crt,
				     const char *oid, const void *buf,
				     size_t sizeof_buf,
				     unsigned int critical)
{
	int result;
	gnutls_datum_t der_data;

	der_data.data = (void *) buf;
	der_data.size = sizeof_buf;

	if (crt == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	result =
	    _gnutls_x509_crt_set_extension(crt, oid, &der_data, critical);
	if (result < 0) {
		gnutls_assert();
		return result;
	}

	return 0;

}

/**
 * gnutls_x509_crt_set_basic_constraints:
 * @crt: a certificate of type #gnutls_x509_crt_t
 * @ca: true(1) or false(0). Depending on the Certificate authority status.
 * @pathLenConstraint: non-negative error codes indicate maximum length of path,
 *   and negative error codes indicate that the pathLenConstraints field should
 *   not be present.
 *
 * This function will set the basicConstraints certificate extension.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_x509_crt_set_basic_constraints(gnutls_x509_crt_t crt,
				      unsigned int ca,
				      int pathLenConstraint)
{
	int result;
	gnutls_datum_t der_data;

	if (crt == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	/* generate the extension.
	 */
	result = gnutls_x509_ext_export_basic_constraints(ca, pathLenConstraint, &der_data);
	if (result < 0) {
		gnutls_assert();
		return result;
	}

	result =
	    _gnutls_x509_crt_set_extension(crt, "2.5.29.19", &der_data, 1);

	_gnutls_free_datum(&der_data);

	if (result < 0) {
		gnutls_assert();
		return result;
	}

	return 0;
}

/**
 * gnutls_x509_crt_set_ca_status:
 * @crt: a certificate of type #gnutls_x509_crt_t
 * @ca: true(1) or false(0). Depending on the Certificate authority status.
 *
 * This function will set the basicConstraints certificate extension.
 * Use gnutls_x509_crt_set_basic_constraints() if you want to control
 * the pathLenConstraint field too.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int gnutls_x509_crt_set_ca_status(gnutls_x509_crt_t crt, unsigned int ca)
{
	return gnutls_x509_crt_set_basic_constraints(crt, ca, -1);
}

/**
 * gnutls_x509_crt_set_key_usage:
 * @crt: a certificate of type #gnutls_x509_crt_t
 * @usage: an ORed sequence of the GNUTLS_KEY_* elements.
 *
 * This function will set the keyUsage certificate extension.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_x509_crt_set_key_usage(gnutls_x509_crt_t crt, unsigned int usage)
{
	int result;
	gnutls_datum_t der_data;

	if (crt == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	/* generate the extension.
	 */
	result =
	    gnutls_x509_ext_export_key_usage(usage, &der_data);
	if (result < 0) {
		gnutls_assert();
		return result;
	}

	result =
	    _gnutls_x509_crt_set_extension(crt, "2.5.29.15", &der_data, 1);

	_gnutls_free_datum(&der_data);

	if (result < 0) {
		gnutls_assert();
		return result;
	}

	return 0;
}

/**
 * gnutls_x509_crt_set_inhibit_anypolicy:
 * @crt: a certificate of type #gnutls_x509_crt_t
 * @skipcerts: number of certificates after which anypolicy is no longer acceptable.
 *
 * This function will set the Inhibit anyPolicy certificate extension.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_x509_crt_set_inhibit_anypolicy(gnutls_x509_crt_t crt, unsigned int skipcerts)
{
	int ret;
	gnutls_datum_t der_data;

	if (crt == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	/* generate the extension.
	 */
	ret =
	    gnutls_x509_ext_export_inhibit_anypolicy(skipcerts, &der_data);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	ret =
	    _gnutls_x509_crt_set_extension(crt, "2.5.29.54", &der_data, 1);
	_gnutls_free_datum(&der_data);

	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	return 0;
}

/**
 * gnutls_x509_crt_set_subject_alternative_name:
 * @crt: a certificate of type #gnutls_x509_crt_t
 * @type: is one of the gnutls_x509_subject_alt_name_t enumerations
 * @data_string: The data to be set, a (0) terminated string
 *
 * This function will set the subject alternative name certificate
 * extension. This function assumes that data can be expressed as a null
 * terminated string.
 *
 * The name of the function is unfortunate since it is inconsistent with
 * gnutls_x509_crt_get_subject_alt_name().
 *
 * See gnutls_x509_crt_set_subject_alt_name() for more information.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_x509_crt_set_subject_alternative_name(gnutls_x509_crt_t crt,
					     gnutls_x509_subject_alt_name_t
					     type, const char *data_string)
{
	if (crt == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	/* only handle text extensions */
	if (type != GNUTLS_SAN_DNSNAME && type != GNUTLS_SAN_RFC822NAME &&
	    type != GNUTLS_SAN_URI) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	return gnutls_x509_crt_set_subject_alt_name(crt, type, data_string,
						    strlen(data_string),
						    GNUTLS_FSAN_SET);
}

/**
 * gnutls_x509_crt_set_subject_alt_name:
 * @crt: a certificate of type #gnutls_x509_crt_t
 * @type: is one of the gnutls_x509_subject_alt_name_t enumerations
 * @data: The data to be set
 * @data_size: The size of data to be set
 * @flags: GNUTLS_FSAN_SET to clear previous data or GNUTLS_FSAN_APPEND to append. 
 *
 * This function will set the subject alternative name certificate
 * extension. It can set the following types: %GNUTLS_SAN_DNSNAME as a text string,
 * %GNUTLS_SAN_RFC822NAME as a text string, %GNUTLS_SAN_URI as a text string,
 * %GNUTLS_SAN_IPADDRESS as a binary IP address (4 or 16 bytes),
 * %GNUTLS_SAN_OTHERNAME_XMPP as a UTF8 string (since 3.5.0).
 *
 * Since version 3.5.7 the %GNUTLS_SAN_RFC822NAME, %GNUTLS_SAN_DNSNAME, and
 * %GNUTLS_SAN_OTHERNAME_XMPP are converted to ACE format when necessary.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 2.6.0
 **/
int
gnutls_x509_crt_set_subject_alt_name(gnutls_x509_crt_t crt,
				     gnutls_x509_subject_alt_name_t type,
				     const void *data,
				     unsigned int data_size,
				     unsigned int flags)
{
	int result;
	gnutls_datum_t der_data = { NULL, 0 };
	gnutls_datum_t prev_der_data = { NULL, 0 };
	unsigned int critical = 0;

	if (crt == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	/* Check if the extension already exists.
	 */

	if (flags & GNUTLS_FSAN_APPEND) {
		result =
		    _gnutls_x509_crt_get_extension(crt, "2.5.29.17", 0,
						   &prev_der_data,
						   &critical);
		if (result < 0
		    && result != GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE) {
			gnutls_assert();
			return result;
		}
	}

	/* generate the extension.
	 */
	result =
	    _gnutls_x509_ext_gen_subject_alt_name(type, NULL, data, data_size,
						  &prev_der_data,
						  &der_data);

	if (result < 0) {
		gnutls_assert();
		goto finish;
	}

	result =
	    _gnutls_x509_crt_set_extension(crt, "2.5.29.17", &der_data,
					   critical);

	_gnutls_free_datum(&der_data);

	if (result < 0) {
		gnutls_assert();
		return result;
	}

	result = 0;

      finish:
	_gnutls_free_datum(&prev_der_data);
	return result;
}

/**
 * gnutls_x509_crt_set_issuer_alt_name:
 * @crt: a certificate of type #gnutls_x509_crt_t
 * @type: is one of the gnutls_x509_subject_alt_name_t enumerations
 * @data: The data to be set
 * @data_size: The size of data to be set
 * @flags: GNUTLS_FSAN_SET to clear previous data or GNUTLS_FSAN_APPEND to append. 
 *
 * This function will set the issuer alternative name certificate
 * extension. It can set the same types as gnutls_x509_crt_set_subject_alt_name().
 *
 * Since version 3.5.7 the %GNUTLS_SAN_RFC822NAME, %GNUTLS_SAN_DNSNAME, and
 * %GNUTLS_SAN_OTHERNAME_XMPP are converted to ACE format when necessary.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.3.0
 **/
int
gnutls_x509_crt_set_issuer_alt_name(gnutls_x509_crt_t crt,
				     gnutls_x509_subject_alt_name_t type,
				     const void *data,
				     unsigned int data_size,
				     unsigned int flags)
{
	int result;
	gnutls_datum_t der_data = { NULL, 0 };
	gnutls_datum_t prev_der_data = { NULL, 0 };
	unsigned int critical = 0;

	if (crt == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	/* Check if the extension already exists.
	 */

	if (flags & GNUTLS_FSAN_APPEND) {
		result =
		    _gnutls_x509_crt_get_extension(crt, "2.5.29.18", 0,
						   &prev_der_data,
						   &critical);
		if (result < 0
		    && result != GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE) {
			gnutls_assert();
			return result;
		}
	}

	/* generate the extension.
	 */
	result =
	    _gnutls_x509_ext_gen_subject_alt_name(type, NULL, data, data_size,
						  &prev_der_data,
						  &der_data);

	if (result < 0) {
		gnutls_assert();
		goto finish;
	}

	result =
	    _gnutls_x509_crt_set_extension(crt, "2.5.29.18", &der_data,
					   critical);

	_gnutls_free_datum(&der_data);

	if (result < 0) {
		gnutls_assert();
		return result;
	}

	result = 0;

      finish:
	_gnutls_free_datum(&prev_der_data);
	return result;
}

int _gnutls_encode_othername_data(unsigned flags, const void *data, unsigned data_size, gnutls_datum_t *output)
{
	int ret;
	if (flags & GNUTLS_FSAN_ENCODE_OCTET_STRING) {
		ret = _gnutls_x509_encode_string(ASN1_ETYPE_OCTET_STRING,
			data, data_size, output);
	} else if (flags & GNUTLS_FSAN_ENCODE_UTF8_STRING) {
		ret = _gnutls_x509_encode_string(ASN1_ETYPE_UTF8_STRING,
			data, data_size, output);
	} else {
		ret = _gnutls_set_datum(output, data, data_size);
	}
	return ret;
}

/**
 * gnutls_x509_crt_set_subject_alt_othername:
 * @crt: a certificate of type #gnutls_x509_crt_t
 * @oid: The other name OID
 * @data: The data to be set
 * @data_size: The size of data to be set
 * @flags: GNUTLS_FSAN_SET to clear previous data or GNUTLS_FSAN_APPEND to append. 
 *
 * This function will set an "othername" to the subject alternative name certificate
 * extension.
 *
 * The values set are set as binary values and are expected to have the proper DER encoding.
 * For convenience the flags %GNUTLS_FSAN_ENCODE_OCTET_STRING and %GNUTLS_FSAN_ENCODE_UTF8_STRING
 * can be used to encode the provided data.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.5.0
 **/
int
gnutls_x509_crt_set_subject_alt_othername(gnutls_x509_crt_t crt,
				     const char *oid,
				     const void *data,
				     unsigned int data_size,
				     unsigned int flags)
{
	int result;
	gnutls_datum_t der_data = { NULL, 0 };
	gnutls_datum_t prev_der_data = { NULL, 0 };
	gnutls_datum_t encoded_data = { NULL, 0 };
	unsigned int critical = 0;

	if (crt == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	/* Check if the extension already exists.
	 */

	if (flags & GNUTLS_FSAN_APPEND) {
		result =
		    _gnutls_x509_crt_get_extension(crt, "2.5.29.17", 0,
						   &prev_der_data,
						   &critical);
		if (result < 0
		    && result != GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE) {
			gnutls_assert();
			return result;
		}
	}

	result = _gnutls_encode_othername_data(flags, data, data_size, &encoded_data);
	if (result < 0) {
		gnutls_assert();
		goto finish;
	}

	/* generate the extension.
	 */
	result =
		_gnutls_x509_ext_gen_subject_alt_name(GNUTLS_SAN_OTHERNAME, oid,
						      encoded_data.data, encoded_data.size,
						      &prev_der_data, &der_data);

	if (result < 0) {
		gnutls_assert();
		goto finish;
	}

	result =
	    _gnutls_x509_crt_set_extension(crt, "2.5.29.17", &der_data,
					   critical);


	if (result < 0) {
		gnutls_assert();
		goto finish;
	}

	result = 0;

      finish:
	_gnutls_free_datum(&der_data);
	_gnutls_free_datum(&prev_der_data);
	_gnutls_free_datum(&encoded_data);
	return result;
}

/**
 * gnutls_x509_crt_set_issuer_alt_othername:
 * @crt: a certificate of type #gnutls_x509_crt_t
 * @oid: The other name OID
 * @data: The data to be set
 * @data_size: The size of data to be set
 * @flags: GNUTLS_FSAN_SET to clear previous data or GNUTLS_FSAN_APPEND to append. 
 *
 * This function will set an "othername" to the issuer alternative name certificate
 * extension.
 *
 * The values set are set as binary values and are expected to have the proper DER encoding.
 * For convenience the flags %GNUTLS_FSAN_ENCODE_OCTET_STRING and %GNUTLS_FSAN_ENCODE_UTF8_STRING
 * can be used to encode the provided data.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.5.0
 **/
int
gnutls_x509_crt_set_issuer_alt_othername(gnutls_x509_crt_t crt,
				     const char *oid,
				     const void *data,
				     unsigned int data_size,
				     unsigned int flags)
{
	int result;
	gnutls_datum_t der_data = { NULL, 0 };
	gnutls_datum_t prev_der_data = { NULL, 0 };
	gnutls_datum_t encoded_data = {NULL, 0};
	unsigned int critical = 0;

	if (crt == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	/* Check if the extension already exists.
	 */

	if (flags & GNUTLS_FSAN_APPEND) {
		result =
		    _gnutls_x509_crt_get_extension(crt, "2.5.29.18", 0,
						   &prev_der_data,
						   &critical);
		if (result < 0
		    && result != GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE) {
			gnutls_assert();
			return result;
		}
	}

	result = _gnutls_encode_othername_data(flags, data, data_size, &encoded_data);
	if (result < 0) {
		gnutls_assert();
		goto finish;
	}

	/* generate the extension.
	 */
	result =
		_gnutls_x509_ext_gen_subject_alt_name(GNUTLS_SAN_OTHERNAME, oid,
						      encoded_data.data, encoded_data.size,
						      &prev_der_data, &der_data);
	if (result < 0) {
		gnutls_assert();
		goto finish;
	}

	result =
	    _gnutls_x509_crt_set_extension(crt, "2.5.29.18", &der_data,
					   critical);

	if (result < 0) {
		gnutls_assert();
		goto finish;
	}

	result = 0;

      finish:
	_gnutls_free_datum(&der_data);
	_gnutls_free_datum(&prev_der_data);
	_gnutls_free_datum(&encoded_data);
	return result;
}

/**
 * gnutls_x509_crt_set_proxy:
 * @crt: a certificate of type #gnutls_x509_crt_t
 * @pathLenConstraint: non-negative error codes indicate maximum length of path,
 *   and negative error codes indicate that the pathLenConstraints field should
 *   not be present.
 * @policyLanguage: OID describing the language of @policy.
 * @policy: uint8_t byte array with policy language, can be %NULL
 * @sizeof_policy: size of @policy.
 *
 * This function will set the proxyCertInfo extension.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_x509_crt_set_proxy(gnutls_x509_crt_t crt,
			  int pathLenConstraint,
			  const char *policyLanguage,
			  const char *policy, size_t sizeof_policy)
{
	int result;
	gnutls_datum_t der_data;

	if (crt == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	/* generate the extension.
	 */
	result = gnutls_x509_ext_export_proxy(pathLenConstraint,
					    policyLanguage,
					    policy, sizeof_policy,
					    &der_data);
	if (result < 0) {
		gnutls_assert();
		return result;
	}

	result = _gnutls_x509_crt_set_extension(crt, "1.3.6.1.5.5.7.1.14",
						&der_data, 1);

	_gnutls_free_datum(&der_data);

	if (result < 0) {
		gnutls_assert();
		return result;
	}

	return 0;
}

/**
 * gnutls_x509_crt_set_private_key_usage_period:
 * @crt: a certificate of type #gnutls_x509_crt_t
 * @activation: The activation time
 * @expiration: The expiration time
 *
 * This function will set the private key usage period extension (2.5.29.16).
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_x509_crt_set_private_key_usage_period(gnutls_x509_crt_t crt,
					     time_t activation,
					     time_t expiration)
{
	int result;
	gnutls_datum_t der_data;

	if (crt == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	result = gnutls_x509_ext_export_private_key_usage_period(activation,
		expiration, &der_data);
	if (result < 0) {
		gnutls_assert();
		goto cleanup;
	}

	result = _gnutls_x509_crt_set_extension(crt, "2.5.29.16",
						&der_data, 0);

	_gnutls_free_datum(&der_data);

 cleanup:
	return result;
}

/**
 * gnutls_x509_crt_sign2:
 * @crt: a certificate of type #gnutls_x509_crt_t
 * @issuer: is the certificate of the certificate issuer
 * @issuer_key: holds the issuer's private key
 * @dig: The message digest to use, %GNUTLS_DIG_SHA256 is a safe choice
 * @flags: must be 0
 *
 * This function will sign the certificate with the issuer's private key, and
 * will copy the issuer's information into the certificate.
 *
 * This must be the last step in a certificate generation since all
 * the previously set parameters are now signed.
 *
 * A known limitation of this function is, that a newly-signed certificate will not
 * be fully functional (e.g., for signature verification), until it
 * is exported an re-imported.
 *
 * After GnuTLS 3.6.1 the value of @dig may be %GNUTLS_DIG_UNKNOWN,
 * and in that case, a suitable but reasonable for the key algorithm will be selected.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_x509_crt_sign2(gnutls_x509_crt_t crt, gnutls_x509_crt_t issuer,
		      gnutls_x509_privkey_t issuer_key,
		      gnutls_digest_algorithm_t dig, unsigned int flags)
{
	int result;
	gnutls_privkey_t privkey;

	if (crt == NULL || issuer == NULL || issuer_key == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	MODIFIED(crt);

	result = gnutls_privkey_init(&privkey);
	if (result < 0) {
		gnutls_assert();
		return result;
	}

	result = gnutls_privkey_import_x509(privkey, issuer_key, 0);
	if (result < 0) {
		gnutls_assert();
		goto fail;
	}

	result =
	    gnutls_x509_crt_privkey_sign(crt, issuer, privkey, dig, flags);
	if (result < 0) {
		gnutls_assert();
		goto fail;
	}

	result = 0;

      fail:
	gnutls_privkey_deinit(privkey);

	return result;
}

/**
 * gnutls_x509_crt_sign:
 * @crt: a certificate of type #gnutls_x509_crt_t
 * @issuer: is the certificate of the certificate issuer
 * @issuer_key: holds the issuer's private key
 *
 * This function is the same a gnutls_x509_crt_sign2() with no flags,
 * and an appropriate hash algorithm. The hash algorithm used may
 * vary between versions of GnuTLS, and it is tied to the security
 * level of the issuer's public key.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_x509_crt_sign(gnutls_x509_crt_t crt, gnutls_x509_crt_t issuer,
		     gnutls_x509_privkey_t issuer_key)
{
	return gnutls_x509_crt_sign2(crt, issuer, issuer_key,
				     0, 0);
}

/**
 * gnutls_x509_crt_set_activation_time:
 * @cert: a certificate of type #gnutls_x509_crt_t
 * @act_time: The actual time
 *
 * This function will set the time this certificate was or will be
 * activated.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_x509_crt_set_activation_time(gnutls_x509_crt_t cert,
				    time_t act_time)
{
	if (cert == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	MODIFIED(cert);

	return _gnutls_x509_set_time(cert->cert,
				     "tbsCertificate.validity.notBefore",
				     act_time, 0);
}

/**
 * gnutls_x509_crt_set_expiration_time:
 * @cert: a certificate of type #gnutls_x509_crt_t
 * @exp_time: The actual time
 *
 * This function will set the time this Certificate will expire.
 * Setting an expiration time to (time_t)-1 will set
 * to the no well-defined expiration date value.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_x509_crt_set_expiration_time(gnutls_x509_crt_t cert,
				    time_t exp_time)
{
	if (cert == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	MODIFIED(cert);

	return _gnutls_x509_set_time(cert->cert,
				     "tbsCertificate.validity.notAfter",
				     exp_time, 0);
}

/**
 * gnutls_x509_crt_set_serial:
 * @cert: a certificate of type #gnutls_x509_crt_t
 * @serial: The serial number
 * @serial_size: Holds the size of the serial field.
 *
 * This function will set the X.509 certificate's serial number.
 * While the serial number is an integer, it is often handled
 * as an opaque field by several CAs. For this reason this function
 * accepts any kind of data as a serial number. To be consistent
 * with the X.509/PKIX specifications the provided @serial should be 
 * a big-endian positive number (i.e. it's leftmost bit should be zero).
 *
 * The size of the serial is restricted to 20 bytes maximum by RFC5280.
 * This function allows writing more than 20 bytes but the generated
 * certificates in that case may be rejected by other implementations.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_x509_crt_set_serial(gnutls_x509_crt_t cert, const void *serial,
			   size_t serial_size)
{
	int ret;
	unsigned all_zero, i;
	const unsigned char *pserial = serial;

	if (cert == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	/* check for non-zero serial */
	all_zero = 1;
	for (i=0;i<serial_size;i++) {
		if (pserial[i] != 0) {
			all_zero = 0;
			break;
		}
	}

	if (all_zero) {
		_gnutls_debug_log("error: certificate serial is zero\n");
		return GNUTLS_E_INVALID_REQUEST;
	}

	MODIFIED(cert);

	ret =
	    asn1_write_value(cert->cert, "tbsCertificate.serialNumber",
			     serial, serial_size);
	if (ret != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(ret);
	}

	return 0;

}

/**
 * gnutls_x509_crt_set_issuer_unique_id:
 * @cert: a certificate of type #gnutls_x509_crt_t
 * @id: The unique ID
 * @id_size: Holds the size of the unique ID.
 *
 * This function will set the X.509 certificate's issuer unique ID field.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.4.7
 **/
int
gnutls_x509_crt_set_issuer_unique_id(gnutls_x509_crt_t cert, const void *id,
			   size_t id_size)
{
	int ret;

	if (cert == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	MODIFIED(cert);

	ret =
	    asn1_write_value(cert->cert, "tbsCertificate.issuerUniqueID",
			     id, id_size*8);
	if (ret != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(ret);
	}

	return 0;
}

/**
 * gnutls_x509_crt_set_subject_unique_id:
 * @cert: a certificate of type #gnutls_x509_crt_t
 * @id: The unique ID
 * @id_size: Holds the size of the unique ID.
 *
 * This function will set the X.509 certificate's subject unique ID field.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.4.7
 **/
int
gnutls_x509_crt_set_subject_unique_id(gnutls_x509_crt_t cert, const void *id,
			   size_t id_size)
{
	int ret;

	if (cert == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	MODIFIED(cert);

	ret =
	    asn1_write_value(cert->cert, "tbsCertificate.subjectUniqueID",
			     id, id_size*8);
	if (ret != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(ret);
	}

	return 0;
}

/* If OPTIONAL fields have not been initialized then
 * disable them.
 */
static void disable_optional_stuff(gnutls_x509_crt_t cert)
{
	asn1_data_node_st n;
	asn1_node node;
	unsigned remove_subject_unique_id = 1;
	unsigned remove_issuer_unique_id = 1;

	node = asn1_find_node(cert->cert, "tbsCertificate.issuerUniqueID");
	if (node) {
		if (asn1_read_node_value(node, &n) == ASN1_SUCCESS && n.value_len != 0)
			remove_issuer_unique_id = 0;
	}

	node = asn1_find_node(cert->cert, "tbsCertificate.subjectUniqueID");
	if (node) {
		if (asn1_read_node_value(node, &n) == ASN1_SUCCESS && n.value_len != 0)
			remove_subject_unique_id = 0;
	}

	if (remove_issuer_unique_id)
		(void)asn1_write_value(cert->cert, "tbsCertificate.issuerUniqueID", NULL,
				 0);

	if (remove_subject_unique_id)
		(void)asn1_write_value(cert->cert, "tbsCertificate.subjectUniqueID",
				 NULL, 0);

	if (cert->use_extensions == 0) {
		_gnutls_debug_log("Disabling X.509 extensions.\n");
		(void)asn1_write_value(cert->cert, "tbsCertificate.extensions",
				 NULL, 0);
	}

	return;
}

/**
 * gnutls_x509_crt_set_crl_dist_points:
 * @crt: a certificate of type #gnutls_x509_crt_t
 * @type: is one of the gnutls_x509_subject_alt_name_t enumerations
 * @data_string: The data to be set
 * @reason_flags: revocation reasons
 *
 * This function will set the CRL distribution points certificate extension.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_x509_crt_set_crl_dist_points(gnutls_x509_crt_t crt,
				    gnutls_x509_subject_alt_name_t type,
				    const void *data_string,
				    unsigned int reason_flags)
{
	return gnutls_x509_crt_set_crl_dist_points2(crt, type, data_string,
						    strlen(data_string),
						    reason_flags);
}

/**
 * gnutls_x509_crt_set_crl_dist_points2:
 * @crt: a certificate of type #gnutls_x509_crt_t
 * @type: is one of the gnutls_x509_subject_alt_name_t enumerations
 * @data: The data to be set
 * @data_size: The data size
 * @reason_flags: revocation reasons
 *
 * This function will set the CRL distribution points certificate extension.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 2.6.0
 **/
int
gnutls_x509_crt_set_crl_dist_points2(gnutls_x509_crt_t crt,
				     gnutls_x509_subject_alt_name_t type,
				     const void *data,
				     unsigned int data_size,
				     unsigned int reason_flags)
{
	int ret;
	gnutls_datum_t der_data = { NULL, 0 };
	gnutls_datum_t old_der = { NULL, 0 };
	unsigned int critical;
	gnutls_x509_crl_dist_points_t cdp = NULL;
	gnutls_datum_t san;

	if (crt == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	ret = gnutls_x509_crl_dist_points_init(&cdp);
	if (ret < 0)
		return gnutls_assert_val(ret);

	/* Check if the extension already exists.
	 */
	ret =
	    _gnutls_x509_crt_get_extension(crt, "2.5.29.31", 0, &old_der,
					   &critical);

	if (ret >= 0 && old_der.data != NULL) {
		ret = gnutls_x509_ext_import_crl_dist_points(&old_der, cdp, 0);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}
	}

	san.data = (void*)data;
	san.size = data_size;
	ret = gnutls_x509_crl_dist_points_set(cdp, type, &san, reason_flags);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	/* generate the extension.
	 */
	ret =
	    gnutls_x509_ext_export_crl_dist_points(cdp, &der_data);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret =
	    _gnutls_x509_crt_set_extension(crt, "2.5.29.31", &der_data, 0);

	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = 0;
 cleanup:
	_gnutls_free_datum(&der_data);
	_gnutls_free_datum(&old_der);
	if (cdp != NULL)
		gnutls_x509_crl_dist_points_deinit(cdp);

	return ret;

}

/**
 * gnutls_x509_crt_cpy_crl_dist_points:
 * @dst: a certificate of type #gnutls_x509_crt_t
 * @src: the certificate where the dist points will be copied from
 *
 * This function will copy the CRL distribution points certificate
 * extension, from the source to the destination certificate.
 * This may be useful to copy from a CA certificate to issued ones.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_x509_crt_cpy_crl_dist_points(gnutls_x509_crt_t dst,
				    gnutls_x509_crt_t src)
{
	int result;
	gnutls_datum_t der_data;
	unsigned int critical;

	if (dst == NULL || src == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	/* Check if the extension already exists.
	 */
	result =
	    _gnutls_x509_crt_get_extension(src, "2.5.29.31", 0, &der_data,
					   &critical);
	if (result < 0) {
		gnutls_assert();
		return result;
	}

	result =
	    _gnutls_x509_crt_set_extension(dst, "2.5.29.31", &der_data,
					   critical);
	_gnutls_free_datum(&der_data);

	if (result < 0) {
		gnutls_assert();
		return result;
	}

	return 0;
}

/**
 * gnutls_x509_crt_set_subject_key_id:
 * @cert: a certificate of type #gnutls_x509_crt_t
 * @id: The key ID
 * @id_size: Holds the size of the subject key ID field.
 *
 * This function will set the X.509 certificate's subject key ID
 * extension.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_x509_crt_set_subject_key_id(gnutls_x509_crt_t cert,
				   const void *id, size_t id_size)
{
	int result;
	gnutls_datum_t old_id, der_data;
	gnutls_datum_t d_id;
	unsigned int critical;

	if (cert == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	/* Check if the extension already exists.
	 */
	result =
	    _gnutls_x509_crt_get_extension(cert, "2.5.29.14", 0, &old_id,
					   &critical);

	if (result >= 0)
		_gnutls_free_datum(&old_id);
	if (result != GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	/* generate the extension.
	 */
	d_id.data = (void*)id;
	d_id.size = id_size;

	result = gnutls_x509_ext_export_subject_key_id(&d_id, &der_data);
	if (result < 0) {
		gnutls_assert();
		return result;
	}

	result =
	    _gnutls_x509_crt_set_extension(cert, "2.5.29.14", &der_data,
					   0);

	_gnutls_free_datum(&der_data);

	if (result < 0) {
		gnutls_assert();
		return result;
	}

	return 0;
}

/**
 * gnutls_x509_crt_set_authority_key_id:
 * @cert: a certificate of type #gnutls_x509_crt_t
 * @id: The key ID
 * @id_size: Holds the size of the key ID field.
 *
 * This function will set the X.509 certificate's authority key ID extension.
 * Only the keyIdentifier field can be set with this function.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_x509_crt_set_authority_key_id(gnutls_x509_crt_t cert,
				     const void *id, size_t id_size)
{
	int result;
	gnutls_datum_t old_id, der_data;
	unsigned int critical;

	if (cert == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	/* Check if the extension already exists.
	 */
	result =
	    _gnutls_x509_crt_get_extension(cert, "2.5.29.35", 0, &old_id,
					   &critical);

	if (result >= 0)
		_gnutls_free_datum(&old_id);
	if (result != GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	/* generate the extension.
	 */
	result = _gnutls_x509_ext_gen_auth_key_id(id, id_size, &der_data);
	if (result < 0) {
		gnutls_assert();
		return result;
	}

	result =
	    _gnutls_x509_crt_set_extension(cert, "2.5.29.35", &der_data,
					   0);

	_gnutls_free_datum(&der_data);

	if (result < 0) {
		gnutls_assert();
		return result;
	}

	return 0;
}

/**
 * gnutls_x509_crt_set_key_purpose_oid:
 * @cert: a certificate of type #gnutls_x509_crt_t
 * @oid: a pointer to a null terminated string that holds the OID
 * @critical: Whether this extension will be critical or not
 *
 * This function will set the key purpose OIDs of the Certificate.
 * These are stored in the Extended Key Usage extension (2.5.29.37)
 * See the GNUTLS_KP_* definitions for human readable names.
 *
 * Subsequent calls to this function will append OIDs to the OID list.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned,
 *   otherwise a negative error code is returned.
 **/
int
gnutls_x509_crt_set_key_purpose_oid(gnutls_x509_crt_t cert,
				    const void *oid, unsigned int critical)
{
	int ret;
	gnutls_datum_t old_id = {NULL,0};
	gnutls_datum_t der = {NULL,0};
	gnutls_x509_key_purposes_t p = NULL;

	if (cert == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	ret = gnutls_x509_key_purpose_init(&p);
	if (ret < 0)
		return gnutls_assert_val(ret);

	/* Check if the extension already exists.
	 */
	ret =
	    _gnutls_x509_crt_get_extension(cert, "2.5.29.37", 0, &old_id,
					   NULL);

	if (ret >= 0) {
		ret = gnutls_x509_ext_import_key_purposes(&old_id, p, 0);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}
	}

	ret = gnutls_x509_key_purpose_set(p, oid);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = gnutls_x509_ext_export_key_purposes(p, &der);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = _gnutls_x509_crt_set_extension(cert, "2.5.29.37",
						&der, critical);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = 0;
 cleanup:
	_gnutls_free_datum(&der);
	_gnutls_free_datum(&old_id);
	if (p != NULL)
		gnutls_x509_key_purpose_deinit(p);

	return ret;

}

/**
 * gnutls_x509_crt_privkey_sign:
 * @crt: a certificate of type #gnutls_x509_crt_t
 * @issuer: is the certificate of the certificate issuer
 * @issuer_key: holds the issuer's private key
 * @dig: The message digest to use, %GNUTLS_DIG_SHA256 is a safe choice
 * @flags: must be 0
 *
 * This function will sign the certificate with the issuer's private key, and
 * will copy the issuer's information into the certificate.
 *
 * This must be the last step in a certificate generation since all
 * the previously set parameters are now signed.
 *
 * A known limitation of this function is, that a newly-signed certificate will not
 * be fully functional (e.g., for signature verification), until it
 * is exported an re-imported.
 *
 * After GnuTLS 3.6.1 the value of @dig may be %GNUTLS_DIG_UNKNOWN,
 * and in that case, a suitable but reasonable for the key algorithm will be selected.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_x509_crt_privkey_sign(gnutls_x509_crt_t crt,
			     gnutls_x509_crt_t issuer,
			     gnutls_privkey_t issuer_key,
			     gnutls_digest_algorithm_t dig,
			     unsigned int flags)
{
	int result;

	if (crt == NULL || issuer == NULL || issuer_key == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	if (dig == 0) {
		result = gnutls_x509_crt_get_preferred_hash_algorithm(issuer, &dig, NULL);
		if (result < 0)
			return gnutls_assert_val(result);
	}

	MODIFIED(crt);

	/* disable all the unneeded OPTIONAL fields.
	 */
	disable_optional_stuff(crt);

	result = _gnutls_check_cert_sanity(crt);
	if (result < 0) {
		gnutls_assert();
		return result;
	}

	result = _gnutls_x509_pkix_sign(crt->cert, "tbsCertificate",
					dig, flags, issuer, issuer_key);
	if (result < 0) {
		gnutls_assert();
		return result;
	}

	return 0;
}

/**
 * gnutls_x509_crt_set_authority_info_access:
 * @crt: Holds the certificate
 * @what: what data to get, a #gnutls_info_access_what_t type.
 * @data: output data to be freed with gnutls_free().
 *
 * This function sets the Authority Information Access (AIA)
 * extension, see RFC 5280 section 4.2.2.1 for more information.  
 *
 * The type of data stored in @data is specified via @what which
 * should be #gnutls_info_access_what_t values.
 *
 * If @what is %GNUTLS_IA_OCSP_URI, @data will hold the OCSP URI.
 * If @what is %GNUTLS_IA_CAISSUERS_URI, @data will hold the caIssuers
 * URI.  
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.0
 **/
int
gnutls_x509_crt_set_authority_info_access(gnutls_x509_crt_t crt,
					  int what, gnutls_datum_t * data)
{
	int ret;
	gnutls_datum_t der = { NULL, 0 };
	gnutls_datum_t new_der = { NULL, 0 };
	gnutls_x509_aia_t aia_ctx = NULL;
	const char *oid;
	unsigned int c;

	if (crt == NULL)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	ret = gnutls_x509_aia_init(&aia_ctx);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	ret = _gnutls_x509_crt_get_extension(crt, GNUTLS_OID_AIA, 0, &der,
					     &c);
	if (ret >= 0) {		/* decode it */
		ret = gnutls_x509_ext_import_aia(&der, aia_ctx, 0);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}
	}

	if (what == GNUTLS_IA_OCSP_URI)
		oid = GNUTLS_OID_AD_OCSP;
	else if (what == GNUTLS_IA_CAISSUERS_URI)
		oid = GNUTLS_OID_AD_CAISSUERS;
	else
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);
	ret = gnutls_x509_aia_set(aia_ctx, oid, GNUTLS_SAN_URI, data);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = gnutls_x509_ext_export_aia(aia_ctx, &new_der);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = _gnutls_x509_crt_set_extension(crt, GNUTLS_OID_AIA,
					     &new_der, 0);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

 cleanup:
	if (aia_ctx != NULL)
		gnutls_x509_aia_deinit(aia_ctx);
	_gnutls_free_datum(&new_der);
	_gnutls_free_datum(&der);

	return ret;
}


/**
 * gnutls_x509_crt_set_policy:
 * @crt: should contain a #gnutls_x509_crt_t type
 * @policy: A pointer to a policy
 * @critical: use non-zero if the extension is marked as critical
 *
 * This function will set the certificate policy extension (2.5.29.32).
 * Multiple calls to this function append a new policy.
 *
 * Note the maximum text size for the qualifier %GNUTLS_X509_QUALIFIER_NOTICE
 * is 200 characters. This function will fail with %GNUTLS_E_INVALID_REQUEST
 * if this is exceeded.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.1.5
 **/
int
gnutls_x509_crt_set_policy(gnutls_x509_crt_t crt,
			   const struct gnutls_x509_policy_st *policy,
			   unsigned int critical)
{
	int ret;
	gnutls_datum_t der_data = {NULL, 0}, prev_der_data = { NULL, 0 };
	gnutls_x509_policies_t policies = NULL;

	if (crt == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	ret = gnutls_x509_policies_init(&policies);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	ret = _gnutls_x509_crt_get_extension(crt, "2.5.29.32", 0,
						&prev_der_data, NULL);
	if (ret < 0 && ret != GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE) {
		gnutls_assert();
		goto cleanup;
	}


	if (ret != GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE) {
		ret = gnutls_x509_ext_import_policies(&prev_der_data,
			policies, 0);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}
	}

	ret = gnutls_x509_policies_set(policies, policy);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = gnutls_x509_ext_export_policies(policies, &der_data);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = _gnutls_x509_crt_set_extension(crt, "2.5.29.32",
						&der_data, 0);

 cleanup:
	if (policies != NULL)
		gnutls_x509_policies_deinit(policies);
	_gnutls_free_datum(&prev_der_data);
	_gnutls_free_datum(&der_data);

	return ret;
}

/**
 * gnutls_x509_crt_set_spki:
 * @crt: a certificate of type #gnutls_x509_crt_t
 * @spki: a SubjectPublicKeyInfo structure of type #gnutls_x509_spki_t
 * @flags: must be zero
 *
 * This function will set the certificate's subject public key
 * information explicitly. This is intended to be used in the cases
 * where a single public key (e.g., RSA) can be used for multiple
 * signature algorithms (RSA PKCS1-1.5, and RSA-PSS).
 *
 * To export the public key (i.e., the SubjectPublicKeyInfo part), check
 * gnutls_pubkey_import_x509().
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.6.0
 **/
int
gnutls_x509_crt_set_spki(gnutls_x509_crt_t crt,
			 const gnutls_x509_spki_t spki,
			 unsigned int flags)
{
	int ret;
	gnutls_pk_algorithm_t crt_pk;
	gnutls_x509_spki_st tpki;
	gnutls_pk_params_st params;
	unsigned bits;

	if (crt == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	ret = _gnutls_x509_crt_get_mpis(crt, &params);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	bits = pubkey_to_bits(&params);
	crt_pk = params.algo;

	if (!_gnutls_pk_are_compat(crt_pk, spki->pk)) {
		ret = gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);
		goto cleanup;
	}

	if (spki->pk != GNUTLS_PK_RSA_PSS) {
		if (crt_pk == spki->pk) {
			ret = 0;
			goto cleanup;
		}

		gnutls_assert();
		ret = GNUTLS_E_INVALID_REQUEST;
		goto cleanup;
	}

	memset(&tpki, 0, sizeof(gnutls_x509_spki_st));

	if (crt_pk == GNUTLS_PK_RSA) {
		const mac_entry_st *me;

		me = hash_to_entry(spki->rsa_pss_dig);
		if (unlikely(me == NULL)) {
			gnutls_assert();
			ret = GNUTLS_E_INVALID_REQUEST;
			goto cleanup;
		}

		tpki.pk = spki->pk;
		tpki.rsa_pss_dig = spki->rsa_pss_dig;

		/* If salt size is zero, find the optimal salt size. */
		if (spki->salt_size == 0) {
			ret = _gnutls_find_rsa_pss_salt_size(bits, me,
							   spki->salt_size);
			if (ret < 0) {
				gnutls_assert();
				goto cleanup;
			}
			tpki.salt_size = ret;
		} else
			tpki.salt_size = spki->salt_size;
	} else if (crt_pk == GNUTLS_PK_RSA_PSS) {
		ret = _gnutls_x509_crt_read_spki_params(crt, &tpki);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		tpki.salt_size = spki->salt_size;
		tpki.rsa_pss_dig = spki->rsa_pss_dig;
	}

	memcpy(&params.spki, &tpki, sizeof(tpki));
	ret = _gnutls_x509_check_pubkey_params(&params);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	MODIFIED(crt);

	ret = _gnutls_x509_write_spki_params(crt->cert,
						"tbsCertificate."
						"subjectPublicKeyInfo.algorithm",
						&tpki);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = 0;
 cleanup:
	gnutls_pk_params_release(&params);
	return ret;
}
