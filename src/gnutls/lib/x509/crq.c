/*
 * Copyright (C) 2003-2016 Free Software Foundation, Inc.
 * Copyright (C) 2012-2016 Nikos Mavrogiannopoulos
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

/* This file contains functions to handle PKCS #10 certificate
   requests, see RFC 2986.
 */

#include "gnutls_int.h"

#include <datum.h>
#include <global.h>
#include "errors.h"
#include <common.h>
#include <x509.h>
#include <x509_b64.h>
#include <gnutls/x509-ext.h>
#include "x509_int.h"
#include <libtasn1.h>
#include <pk.h>
#include "attributes.h"

/**
 * gnutls_x509_crq_init:
 * @crq: A pointer to the type to be initialized
 *
 * This function will initialize a PKCS#10 certificate request
 * structure.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int gnutls_x509_crq_init(gnutls_x509_crq_t * crq)
{
	int result;
	
	FAIL_IF_LIB_ERROR;

	*crq = gnutls_calloc(1, sizeof(gnutls_x509_crq_int));
	if (!*crq)
		return GNUTLS_E_MEMORY_ERROR;

	result = asn1_create_element(_gnutls_get_pkix(),
				     "PKIX1.pkcs-10-CertificationRequest",
				     &((*crq)->crq));
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		gnutls_free(*crq);
		return _gnutls_asn2err(result);
	}

	return 0;
}

/**
 * gnutls_x509_crq_deinit:
 * @crq: the type to be deinitialized
 *
 * This function will deinitialize a PKCS#10 certificate request
 * structure.
 **/
void gnutls_x509_crq_deinit(gnutls_x509_crq_t crq)
{
	if (!crq)
		return;

	if (crq->crq)
		asn1_delete_structure(&crq->crq);

	gnutls_free(crq);
}

#define PEM_CRQ "NEW CERTIFICATE REQUEST"
#define PEM_CRQ2 "CERTIFICATE REQUEST"

/**
 * gnutls_x509_crq_import:
 * @crq: The data to store the parsed certificate request.
 * @data: The DER or PEM encoded certificate.
 * @format: One of DER or PEM
 *
 * This function will convert the given DER or PEM encoded certificate
 * request to a #gnutls_x509_crq_t type.  The output will be
 * stored in @crq.
 *
 * If the Certificate is PEM encoded it should have a header of "NEW
 * CERTIFICATE REQUEST".
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_x509_crq_import(gnutls_x509_crq_t crq,
		       const gnutls_datum_t * data,
		       gnutls_x509_crt_fmt_t format)
{
	int result = 0, need_free = 0;
	gnutls_datum_t _data;

	if (crq == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	_data.data = data->data;
	_data.size = data->size;

	/* If the Certificate is in PEM format then decode it
	 */
	if (format == GNUTLS_X509_FMT_PEM) {
		/* Try the first header */
		result =
		    _gnutls_fbase64_decode(PEM_CRQ, data->data, data->size,
					   &_data);

		if (result < 0)	/* Go for the second header */
			result =
			    _gnutls_fbase64_decode(PEM_CRQ2, data->data,
						   data->size, &_data);

		if (result < 0) {
			gnutls_assert();
			return result;
		}

		need_free = 1;
	}

	result =
	    _asn1_strict_der_decode(&crq->crq, _data.data, _data.size, NULL);
	if (result != ASN1_SUCCESS) {
		result = _gnutls_asn2err(result);
		gnutls_assert();
		goto cleanup;
	}

	result = 0;

      cleanup:
	if (need_free)
		_gnutls_free_datum(&_data);
	return result;
}

/**
 * gnutls_x509_crq_get_signature_algorithm:
 * @crq: should contain a #gnutls_x509_cr_t type
 *
 * This function will return a value of the #gnutls_sign_algorithm_t
 * enumeration that is the signature algorithm that has been used to
 * sign this certificate request.
 *
 * Since 3.6.0 this function never returns a negative error code.
 * Error cases and unknown/unsupported signature algorithms are
 * mapped to %GNUTLS_SIGN_UNKNOWN.
 *
 * Returns: a #gnutls_sign_algorithm_t value
 *
 * Since: 3.4.0
 **/
int gnutls_x509_crq_get_signature_algorithm(gnutls_x509_crq_t crq)
{
	return map_errs_to_zero(_gnutls_x509_get_signature_algorithm(crq->crq,
						    "signatureAlgorithm"));
}

/**
 * gnutls_x509_crq_get_private_key_usage_period:
 * @crq: should contain a #gnutls_x509_crq_t type
 * @activation: The activation time
 * @expiration: The expiration time
 * @critical: the extension status
 *
 * This function will return the expiration and activation
 * times of the private key of the certificate.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE
 * if the extension is not present, otherwise a negative error value.
 **/
int
gnutls_x509_crq_get_private_key_usage_period(gnutls_x509_crq_t crq,
					     time_t * activation,
					     time_t * expiration,
					     unsigned int *critical)
{
	int result, ret;
	ASN1_TYPE c2 = ASN1_TYPE_EMPTY;
	uint8_t buf[128];
	size_t buf_size = sizeof(buf);

	if (crq == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	ret = gnutls_x509_crq_get_extension_by_oid(crq, "2.5.29.16", 0,
						   buf, &buf_size,
						   critical);
	if (ret < 0)
		return gnutls_assert_val(ret);

	result = asn1_create_element
	    (_gnutls_get_pkix(), "PKIX1.PrivateKeyUsagePeriod", &c2);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		ret = _gnutls_asn2err(result);
		goto cleanup;
	}

	result = _asn1_strict_der_decode(&c2, buf, buf_size, NULL);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		ret = _gnutls_asn2err(result);
		goto cleanup;
	}

	if (activation)
		*activation = _gnutls_x509_get_time(c2, "notBefore", 1);

	if (expiration)
		*expiration = _gnutls_x509_get_time(c2, "notAfter", 1);

	ret = 0;

      cleanup:
	asn1_delete_structure(&c2);

	return ret;
}


/**
 * gnutls_x509_crq_get_dn:
 * @crq: should contain a #gnutls_x509_crq_t type
 * @buf: a pointer to a structure to hold the name (may be %NULL)
 * @buf_size: initially holds the size of @buf
 *
 * This function will copy the name of the Certificate request subject
 * to the provided buffer.  The name will be in the form
 * "C=xxxx,O=yyyy,CN=zzzz" as described in RFC 2253. The output string
 * @buf will be ASCII or UTF-8 encoded, depending on the certificate
 * data.
 *
 * This function does not output a fully RFC4514 compliant string, if
 * that is required see gnutls_x509_crq_get_dn3().
 *
 * Returns: %GNUTLS_E_SHORT_MEMORY_BUFFER if the provided buffer is not
 *   long enough, and in that case the *@buf_size will be updated with
 *   the required size.  On success 0 is returned.
 **/
int
gnutls_x509_crq_get_dn(gnutls_x509_crq_t crq, char *buf, size_t * buf_size)
{
	if (crq == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	return _gnutls_x509_parse_dn(crq->crq,
				     "certificationRequestInfo.subject.rdnSequence",
				     buf, buf_size, GNUTLS_X509_DN_FLAG_COMPAT);
}

/**
 * gnutls_x509_crq_get_dn2:
 * @crq: should contain a #gnutls_x509_crq_t type
 * @dn: a pointer to a structure to hold the name
 *
 * This function will allocate buffer and copy the name of the Certificate 
 * request. The name will be in the form "C=xxxx,O=yyyy,CN=zzzz" as
 * described in RFC4514. The output string will be ASCII or UTF-8
 * encoded, depending on the certificate data.
 *
 * This function does not output a fully RFC4514 compliant string, if
 * that is required see gnutls_x509_crq_get_dn3().
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value. and a negative error code on error.
 *
 * Since: 3.1.10
 **/
int gnutls_x509_crq_get_dn2(gnutls_x509_crq_t crq, gnutls_datum_t * dn)
{
	if (crq == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	return _gnutls_x509_get_dn(crq->crq,
				   "certificationRequestInfo.subject.rdnSequence",
				   dn, GNUTLS_X509_DN_FLAG_COMPAT);
}

/**
 * gnutls_x509_crq_get_dn3:
 * @crq: should contain a #gnutls_x509_crq_t type
 * @dn: a pointer to a structure to hold the name
 * @flags: zero or %GNUTLS_X509_DN_FLAG_COMPAT
 *
 * This function will allocate buffer and copy the name of the Certificate 
 * request. The name will be in the form "C=xxxx,O=yyyy,CN=zzzz" as
 * described in RFC4514. The output string will be ASCII or UTF-8
 * encoded, depending on the certificate data.
 *
 * When the flag %GNUTLS_X509_DN_FLAG_COMPAT is specified, the output
 * format will match the format output by previous to 3.5.6 versions of GnuTLS
 * which was not not fully RFC4514-compliant.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value. and a negative error code on error.
 *
 * Since: 3.5.7
 **/
int gnutls_x509_crq_get_dn3(gnutls_x509_crq_t crq, gnutls_datum_t * dn, unsigned flags)
{
	if (crq == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	return _gnutls_x509_get_dn(crq->crq,
				   "certificationRequestInfo.subject.rdnSequence",
				   dn, flags);
}

/**
 * gnutls_x509_crq_get_dn_by_oid:
 * @crq: should contain a gnutls_x509_crq_t type
 * @oid: holds an Object Identifier in a null terminated string
 * @indx: In case multiple same OIDs exist in the RDN, this specifies
 *   which to get. Use (0) to get the first one.
 * @raw_flag: If non-zero returns the raw DER data of the DN part.
 * @buf: a pointer to a structure to hold the name (may be %NULL)
 * @buf_size: initially holds the size of @buf
 *
 * This function will extract the part of the name of the Certificate
 * request subject, specified by the given OID. The output will be
 * encoded as described in RFC2253. The output string will be ASCII
 * or UTF-8 encoded, depending on the certificate data.
 *
 * Some helper macros with popular OIDs can be found in gnutls/x509.h
 * If raw flag is (0), this function will only return known OIDs as
 * text. Other OIDs will be DER encoded, as described in RFC2253 --
 * in hex format with a '\#' prefix.  You can check about known OIDs
 * using gnutls_x509_dn_oid_known().
 *
 * Returns: %GNUTLS_E_SHORT_MEMORY_BUFFER if the provided buffer is
 *   not long enough, and in that case the *@buf_size will be
 *   updated with the required size.  On success 0 is returned.
 **/
int
gnutls_x509_crq_get_dn_by_oid(gnutls_x509_crq_t crq, const char *oid,
			      unsigned indx, unsigned int raw_flag,
			      void *buf, size_t * buf_size)
{
	gnutls_datum_t td;
	int ret;

	if (crq == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	ret = _gnutls_x509_parse_dn_oid
	    (crq->crq,
	     "certificationRequestInfo.subject.rdnSequence",
	     oid, indx, raw_flag, &td);
	if (ret < 0)
		return gnutls_assert_val(ret);

	return _gnutls_strdatum_to_buf(&td, buf, buf_size);
}

/**
 * gnutls_x509_crq_get_dn_oid:
 * @crq: should contain a gnutls_x509_crq_t type
 * @indx: Specifies which DN OID to get. Use (0) to get the first one.
 * @oid: a pointer to a structure to hold the name (may be %NULL)
 * @sizeof_oid: initially holds the size of @oid
 *
 * This function will extract the requested OID of the name of the
 * certificate request subject, specified by the given index.
 *
 * Returns: %GNUTLS_E_SHORT_MEMORY_BUFFER if the provided buffer is
 *   not long enough, and in that case the *@sizeof_oid will be
 *   updated with the required size.  On success 0 is returned.
 **/
int
gnutls_x509_crq_get_dn_oid(gnutls_x509_crq_t crq,
			   unsigned indx, void *oid, size_t * sizeof_oid)
{
	if (crq == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	return _gnutls_x509_get_dn_oid(crq->crq,
				       "certificationRequestInfo.subject.rdnSequence",
				       indx, oid, sizeof_oid);
}

/**
 * gnutls_x509_crq_get_challenge_password:
 * @crq: should contain a #gnutls_x509_crq_t type
 * @pass: will hold a (0)-terminated password string
 * @pass_size: Initially holds the size of @pass.
 *
 * This function will return the challenge password in the request.
 * The challenge password is intended to be used for requesting a
 * revocation of the certificate.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_x509_crq_get_challenge_password(gnutls_x509_crq_t crq,
				       char *pass, size_t * pass_size)
{
	gnutls_datum_t td;
	int ret;

	if (crq == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	ret =
	    _x509_parse_attribute(crq->crq,
			    "certificationRequestInfo.attributes",
			    "1.2.840.113549.1.9.7", 0, 0, &td);
	if (ret < 0)
		return gnutls_assert_val(ret);

	return _gnutls_strdatum_to_buf(&td, pass, pass_size);
}

/**
 * gnutls_x509_crq_set_attribute_by_oid:
 * @crq: should contain a #gnutls_x509_crq_t type
 * @oid: holds an Object Identifier in a null-terminated string
 * @buf: a pointer to a structure that holds the attribute data
 * @buf_size: holds the size of @buf
 *
 * This function will set the attribute in the certificate request
 * specified by the given Object ID. The provided attribute must be be DER
 * encoded.
 *
 * Attributes in a certificate request is an optional set of data
 * appended to the request. Their interpretation depends on the CA policy.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_x509_crq_set_attribute_by_oid(gnutls_x509_crq_t crq,
				     const char *oid, void *buf,
				     size_t buf_size)
{
	gnutls_datum_t data;

	data.data = buf;
	data.size = buf_size;

	if (crq == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	return _x509_set_attribute(crq->crq,
			     "certificationRequestInfo.attributes", oid,
			     &data);
}

/**
 * gnutls_x509_crq_get_attribute_by_oid:
 * @crq: should contain a #gnutls_x509_crq_t type
 * @oid: holds an Object Identifier in null-terminated string
 * @indx: In case multiple same OIDs exist in the attribute list, this
 *   specifies which to get, use (0) to get the first one
 * @buf: a pointer to a structure to hold the attribute data (may be %NULL)
 * @buf_size: initially holds the size of @buf
 *
 * This function will return the attribute in the certificate request
 * specified by the given Object ID.  The attribute will be DER
 * encoded.
 *
 * Attributes in a certificate request is an optional set of data
 * appended to the request. Their interpretation depends on the CA policy.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_x509_crq_get_attribute_by_oid(gnutls_x509_crq_t crq,
				     const char *oid, unsigned indx, void *buf,
				     size_t * buf_size)
{
	int ret;
	gnutls_datum_t td;

	if (crq == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	ret =
	    _x509_parse_attribute(crq->crq,
			    "certificationRequestInfo.attributes", oid,
			    indx, 1, &td);
	if (ret < 0)
		return gnutls_assert_val(ret);

	return _gnutls_strdatum_to_buf(&td, buf, buf_size);
}

/**
 * gnutls_x509_crq_set_dn_by_oid:
 * @crq: should contain a #gnutls_x509_crq_t type
 * @oid: holds an Object Identifier in a (0)-terminated string
 * @raw_flag: must be 0, or 1 if the data are DER encoded
 * @data: a pointer to the input data
 * @sizeof_data: holds the size of @data
 *
 * This function will set the part of the name of the Certificate
 * request subject, specified by the given OID.  The input string
 * should be ASCII or UTF-8 encoded.
 *
 * Some helper macros with popular OIDs can be found in gnutls/x509.h
 * With this function you can only set the known OIDs.  You can test
 * for known OIDs using gnutls_x509_dn_oid_known().  For OIDs that are
 * not known (by gnutls) you should properly DER encode your data, and
 * call this function with raw_flag set.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_x509_crq_set_dn_by_oid(gnutls_x509_crq_t crq, const char *oid,
			      unsigned int raw_flag, const void *data,
			      unsigned int sizeof_data)
{
	if (sizeof_data == 0 || data == NULL || crq == NULL) {
		return GNUTLS_E_INVALID_REQUEST;
	}

	return _gnutls_x509_set_dn_oid(crq->crq,
				       "certificationRequestInfo.subject",
				       oid, raw_flag, data, sizeof_data);
}

/**
 * gnutls_x509_crq_set_version:
 * @crq: should contain a #gnutls_x509_crq_t type
 * @version: holds the version number, for v1 Requests must be 1
 *
 * This function will set the version of the certificate request.  For
 * version 1 requests this must be one.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_x509_crq_set_version(gnutls_x509_crq_t crq, unsigned int version)
{
	int result;
	unsigned char null = version;

	if (crq == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	if (null > 0)
		null--;

	result =
	    asn1_write_value(crq->crq, "certificationRequestInfo.version",
			     &null, 1);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	return 0;
}

/**
 * gnutls_x509_crq_get_version:
 * @crq: should contain a #gnutls_x509_crq_t type
 *
 * This function will return the version of the specified Certificate
 * request.
 *
 * Returns: version of certificate request, or a negative error code on
 *   error.
 **/
int gnutls_x509_crq_get_version(gnutls_x509_crq_t crq)
{
	uint8_t version[8];
	int len, result;

	if (crq == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	len = sizeof(version);
	if ((result =
	     asn1_read_value(crq->crq, "certificationRequestInfo.version",
			     version, &len)) != ASN1_SUCCESS) {

		if (result == ASN1_ELEMENT_NOT_FOUND)
			return 1;	/* the DEFAULT version */
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	return (int) version[0] + 1;
}

/**
 * gnutls_x509_crq_set_key:
 * @crq: should contain a #gnutls_x509_crq_t type
 * @key: holds a private key
 *
 * This function will set the public parameters from the given private
 * key to the request.  
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_x509_crq_set_key(gnutls_x509_crq_t crq, gnutls_x509_privkey_t key)
{
	int result;

	if (crq == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	result = _gnutls_x509_encode_and_copy_PKI_params
	    (crq->crq,
	     "certificationRequestInfo.subjectPKInfo",
	     &key->params);

	if (result < 0) {
		gnutls_assert();
		return result;
	}

	return 0;
}

/**
 * gnutls_x509_crq_get_key_rsa_raw:
 * @crq: Holds the certificate
 * @m: will hold the modulus
 * @e: will hold the public exponent
 *
 * This function will export the RSA public key's parameters found in
 * the given structure.  The new parameters will be allocated using
 * gnutls_malloc() and will be stored in the appropriate datum.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 2.8.0
 **/
int
gnutls_x509_crq_get_key_rsa_raw(gnutls_x509_crq_t crq,
				gnutls_datum_t * m, gnutls_datum_t * e)
{
	int ret;
	gnutls_pk_params_st params;

	gnutls_pk_params_init(&params);

	if (crq == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	ret = gnutls_x509_crq_get_pk_algorithm(crq, NULL);
	if (ret != GNUTLS_PK_RSA) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	ret = _gnutls_x509_crq_get_mpis(crq, &params);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	ret = _gnutls_mpi_dprint(params.params[0], m);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = _gnutls_mpi_dprint(params.params[1], e);
	if (ret < 0) {
		gnutls_assert();
		_gnutls_free_datum(m);
		goto cleanup;
	}

	ret = 0;

      cleanup:
	gnutls_pk_params_release(&params);
	return ret;
}

/**
 * gnutls_x509_crq_set_key_rsa_raw:
 * @crq: should contain a #gnutls_x509_crq_t type
 * @m: holds the modulus
 * @e: holds the public exponent
 *
 * This function will set the public parameters from the given private
 * key to the request. Only RSA keys are currently supported.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 2.6.0
 **/
int
gnutls_x509_crq_set_key_rsa_raw(gnutls_x509_crq_t crq,
				const gnutls_datum_t * m,
				const gnutls_datum_t * e)
{
	int result, ret;
	size_t siz = 0;
	gnutls_pk_params_st temp_params;

	gnutls_pk_params_init(&temp_params);

	if (crq == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	memset(&temp_params, 0, sizeof(temp_params));

	siz = m->size;
	if (_gnutls_mpi_init_scan_nz(&temp_params.params[0], m->data, siz)) {
		gnutls_assert();
		ret = GNUTLS_E_MPI_SCAN_FAILED;
		goto error;
	}

	siz = e->size;
	if (_gnutls_mpi_init_scan_nz(&temp_params.params[1], e->data, siz)) {
		gnutls_assert();
		ret = GNUTLS_E_MPI_SCAN_FAILED;
		goto error;
	}

	temp_params.params_nr = RSA_PUBLIC_PARAMS;
	temp_params.algo = GNUTLS_PK_RSA;

	result = _gnutls_x509_encode_and_copy_PKI_params
	    (crq->crq,
	     "certificationRequestInfo.subjectPKInfo",
	     &temp_params);

	if (result < 0) {
		gnutls_assert();
		ret = result;
		goto error;
	}

	ret = 0;

      error:
	gnutls_pk_params_release(&temp_params);
	return ret;
}

/**
 * gnutls_x509_crq_set_challenge_password:
 * @crq: should contain a #gnutls_x509_crq_t type
 * @pass: holds a (0)-terminated password
 *
 * This function will set a challenge password to be used when
 * revoking the request.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_x509_crq_set_challenge_password(gnutls_x509_crq_t crq,
				       const char *pass)
{
	int result;
	char *password = NULL;

	if (crq == NULL || pass == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	/* Add the attribute.
	 */
	result =
	    asn1_write_value(crq->crq,
			     "certificationRequestInfo.attributes", "NEW",
			     1);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	if (pass) {
		gnutls_datum_t out;
		result = _gnutls_utf8_password_normalize(pass, strlen(pass), &out, 0);
		if (result < 0)
			return gnutls_assert_val(result);

		password = (char*)out.data;
	}

	assert(password != NULL);

	result = _gnutls_x509_encode_and_write_attribute
	    ("1.2.840.113549.1.9.7", crq->crq,
	     "certificationRequestInfo.attributes.?LAST", password,
	     strlen(password), 1);
	if (result < 0) {
		gnutls_assert();
		goto cleanup;
	}

	result = 0;

 cleanup:
	gnutls_free(password);
	return result;
}

/**
 * gnutls_x509_crq_sign2:
 * @crq: should contain a #gnutls_x509_crq_t type
 * @key: holds a private key
 * @dig: The message digest to use, i.e., %GNUTLS_DIG_SHA256
 * @flags: must be 0
 *
 * This function will sign the certificate request with a private key.
 * This must be the same key as the one used in
 * gnutls_x509_crt_set_key() since a certificate request is self
 * signed.
 *
 * This must be the last step in a certificate request generation
 * since all the previously set parameters are now signed.
 *
 * A known limitation of this function is, that a newly-signed request will not
 * be fully functional (e.g., for signature verification), until it
 * is exported an re-imported.
 *
 * After GnuTLS 3.6.1 the value of @dig may be %GNUTLS_DIG_UNKNOWN,
 * and in that case, a suitable but reasonable for the key algorithm will be selected.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, otherwise a negative error code.
 *   %GNUTLS_E_ASN1_VALUE_NOT_FOUND is returned if you didn't set all
 *   information in the certificate request (e.g., the version using
 *   gnutls_x509_crq_set_version()).
 *
 **/
int
gnutls_x509_crq_sign2(gnutls_x509_crq_t crq, gnutls_x509_privkey_t key,
		      gnutls_digest_algorithm_t dig, unsigned int flags)
{
	int result;
	gnutls_privkey_t privkey;

	if (crq == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	result = gnutls_privkey_init(&privkey);
	if (result < 0) {
		gnutls_assert();
		return result;
	}

	result = gnutls_privkey_import_x509(privkey, key, 0);
	if (result < 0) {
		gnutls_assert();
		goto fail;
	}

	result = gnutls_x509_crq_privkey_sign(crq, privkey, dig, flags);
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
 * gnutls_x509_crq_sign:
 * @crq: should contain a #gnutls_x509_crq_t type
 * @key: holds a private key
 *
 * This function is the same a gnutls_x509_crq_sign2() with no flags,
 * and an appropriate hash algorithm. The hash algorithm used may
 * vary between versions of GnuTLS, and it is tied to the security
 * level of the issuer's public key.
 *
 * A known limitation of this function is, that a newly-signed request will not
 * be fully functional (e.g., for signature verification), until it
 * is exported an re-imported.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 */
int gnutls_x509_crq_sign(gnutls_x509_crq_t crq, gnutls_x509_privkey_t key)
{
	return gnutls_x509_crq_sign2(crq, key, 0, 0);
}

/**
 * gnutls_x509_crq_export:
 * @crq: should contain a #gnutls_x509_crq_t type
 * @format: the format of output params. One of PEM or DER.
 * @output_data: will contain a certificate request PEM or DER encoded
 * @output_data_size: holds the size of output_data (and will be
 *   replaced by the actual size of parameters)
 *
 * This function will export the certificate request to a PEM or DER
 * encoded PKCS10 structure.
 *
 * If the buffer provided is not long enough to hold the output, then
 * %GNUTLS_E_SHORT_MEMORY_BUFFER will be returned and
 * *@output_data_size will be updated.
 *
 * If the structure is PEM encoded, it will have a header of "BEGIN
 * NEW CERTIFICATE REQUEST".
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_x509_crq_export(gnutls_x509_crq_t crq,
		       gnutls_x509_crt_fmt_t format, void *output_data,
		       size_t * output_data_size)
{
	if (crq == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	return _gnutls_x509_export_int(crq->crq, format, PEM_CRQ,
				       output_data, output_data_size);
}

/**
 * gnutls_x509_crq_export2:
 * @crq: should contain a #gnutls_x509_crq_t type
 * @format: the format of output params. One of PEM or DER.
 * @out: will contain a certificate request PEM or DER encoded
 *
 * This function will export the certificate request to a PEM or DER
 * encoded PKCS10 structure.
 *
 * The output buffer is allocated using gnutls_malloc().
 *
 * If the structure is PEM encoded, it will have a header of "BEGIN
 * NEW CERTIFICATE REQUEST".
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since 3.1.3
 **/
int
gnutls_x509_crq_export2(gnutls_x509_crq_t crq,
			gnutls_x509_crt_fmt_t format, gnutls_datum_t * out)
{
	if (crq == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	return _gnutls_x509_export_int2(crq->crq, format, PEM_CRQ, out);
}

/**
 * gnutls_x509_crq_get_pk_algorithm:
 * @crq: should contain a #gnutls_x509_crq_t type
 * @bits: if bits is non-%NULL it will hold the size of the parameters' in bits
 *
 * This function will return the public key algorithm of a PKCS#10
 * certificate request.
 *
 * If bits is non-%NULL, it should have enough size to hold the
 * parameters size in bits.  For RSA the bits returned is the modulus.
 * For DSA the bits returned are of the public exponent.
 *
 * Returns: a member of the #gnutls_pk_algorithm_t enumeration on
 *   success, or a negative error code on error.
 **/
int
gnutls_x509_crq_get_pk_algorithm(gnutls_x509_crq_t crq, unsigned int *bits)
{
	int result;

	if (crq == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	result = _gnutls_x509_get_pk_algorithm
	    (crq->crq, "certificationRequestInfo.subjectPKInfo", NULL, bits);
	if (result < 0) {
		gnutls_assert();
		return result;
	}

	return result;
}

/**
 * gnutls_x509_crq_get_spki;
 * @crq: should contain a #gnutls_x509_crq_t type
 * @spki: a SubjectPublicKeyInfo structure of type #gnutls_x509_spki_t
 * @flags: must be zero
 *
 * This function will return the public key information of a PKCS#10
 * certificate request. The provided @spki must be initialized.
 *
 * Returns: Zero on success, or a negative error code on error.
 **/
int
gnutls_x509_crq_get_spki(gnutls_x509_crq_t crq,
			 gnutls_x509_spki_t spki,
			 unsigned int flags)
{
	int result;
	gnutls_x509_spki_st params;

	if (crq == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	memset(&params, 0, sizeof(params));

	spki->pk = gnutls_x509_crq_get_pk_algorithm(crq, NULL);

	result = _gnutls_x509_crq_read_spki_params(crq, &params);
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

/**
 * gnutls_x509_crq_get_signature_oid:
 * @crq: should contain a #gnutls_x509_crq_t type
 * @oid: a pointer to a buffer to hold the OID (may be null)
 * @oid_size: initially holds the size of @oid
 *
 * This function will return the OID of the signature algorithm
 * that has been used to sign this certificate request. This is function
 * is useful in the case gnutls_x509_crq_get_signature_algorithm()
 * returned %GNUTLS_SIGN_UNKNOWN.
 *
 * Returns: zero or a negative error code on error.
 *
 * Since: 3.5.0
 **/
int gnutls_x509_crq_get_signature_oid(gnutls_x509_crq_t crq, char *oid, size_t *oid_size)
{
	char str[MAX_OID_SIZE];
	int len, result, ret;
	gnutls_datum_t out;

	len = sizeof(str);
	result = asn1_read_value(crq->crq, "signatureAlgorithm.algorithm", str, &len);
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
 * gnutls_x509_crq_get_pk_oid:
 * @crq: should contain a #gnutls_x509_crq_t type
 * @oid: a pointer to a buffer to hold the OID (may be null)
 * @oid_size: initially holds the size of @oid
 *
 * This function will return the OID of the public key algorithm
 * on that certificate request. This is function
 * is useful in the case gnutls_x509_crq_get_pk_algorithm()
 * returned %GNUTLS_PK_UNKNOWN.
 *
 * Returns: zero or a negative error code on error.
 *
 * Since: 3.5.0
 **/
int gnutls_x509_crq_get_pk_oid(gnutls_x509_crq_t crq, char *oid, size_t *oid_size)
{
	char str[MAX_OID_SIZE];
	int len, result, ret;
	gnutls_datum_t out;

	len = sizeof(str);
	result = asn1_read_value(crq->crq, "certificationRequestInfo.subjectPKInfo.algorithm.algorithm", str, &len);
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
 * gnutls_x509_crq_get_attribute_info:
 * @crq: should contain a #gnutls_x509_crq_t type
 * @indx: Specifies which attribute number to get. Use (0) to get the first one.
 * @oid: a pointer to a structure to hold the OID
 * @sizeof_oid: initially holds the maximum size of @oid, on return
 *   holds actual size of @oid.
 *
 * This function will return the requested attribute OID in the
 * certificate, and the critical flag for it.  The attribute OID will
 * be stored as a string in the provided buffer.  Use
 * gnutls_x509_crq_get_attribute_data() to extract the data.
 *
 * If the buffer provided is not long enough to hold the output, then
 * *@sizeof_oid is updated and %GNUTLS_E_SHORT_MEMORY_BUFFER will be
 * returned.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error code in case of an error.  If your have reached the
 *   last extension available %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE
 *   will be returned.
 *
 * Since: 2.8.0
 **/
int
gnutls_x509_crq_get_attribute_info(gnutls_x509_crq_t crq, unsigned indx,
				   void *oid, size_t * sizeof_oid)
{
	int result;
	char name[MAX_NAME_SIZE];
	int len;

	if (!crq) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	snprintf(name, sizeof(name),
		 "certificationRequestInfo.attributes.?%u.type", indx + 1);

	len = *sizeof_oid;
	result = asn1_read_value(crq->crq, name, oid, &len);
	*sizeof_oid = len;

	if (result == ASN1_ELEMENT_NOT_FOUND)
		return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
	else if (result < 0) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	return 0;

}

/**
 * gnutls_x509_crq_get_attribute_data:
 * @crq: should contain a #gnutls_x509_crq_t type
 * @indx: Specifies which attribute number to get. Use (0) to get the first one.
 * @data: a pointer to a structure to hold the data (may be null)
 * @sizeof_data: initially holds the size of @oid
 *
 * This function will return the requested attribute data in the
 * certificate request.  The attribute data will be stored as a string in the
 * provided buffer.
 *
 * Use gnutls_x509_crq_get_attribute_info() to extract the OID.
 * Use gnutls_x509_crq_get_attribute_by_oid() instead,
 * if you want to get data indexed by the attribute OID rather than
 * sequence.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error code in case of an error.  If your have reached the
 *   last extension available %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE
 *   will be returned.
 *
 * Since: 2.8.0
 **/
int
gnutls_x509_crq_get_attribute_data(gnutls_x509_crq_t crq, unsigned indx,
				   void *data, size_t * sizeof_data)
{
	int result, len;
	char name[MAX_NAME_SIZE];

	if (!crq) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	snprintf(name, sizeof(name),
		 "certificationRequestInfo.attributes.?%u.values.?1",
		 indx + 1);

	len = *sizeof_data;
	result = asn1_read_value(crq->crq, name, data, &len);
	*sizeof_data = len;

	if (result == ASN1_ELEMENT_NOT_FOUND)
		return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
	else if (result < 0) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	return 0;
}

/**
 * gnutls_x509_crq_get_extension_info:
 * @crq: should contain a #gnutls_x509_crq_t type
 * @indx: Specifies which extension number to get. Use (0) to get the first one.
 * @oid: a pointer to store the OID
 * @sizeof_oid: initially holds the maximum size of @oid, on return
 *   holds actual size of @oid.
 * @critical: output variable with critical flag, may be NULL.
 *
 * This function will return the requested extension OID in the
 * certificate, and the critical flag for it.  The extension OID will
 * be stored as a string in the provided buffer.  Use
 * gnutls_x509_crq_get_extension_data() to extract the data.
 *
 * If the buffer provided is not long enough to hold the output, then
 * *@sizeof_oid is updated and %GNUTLS_E_SHORT_MEMORY_BUFFER will be
 * returned.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error code in case of an error.  If your have reached the
 *   last extension available %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE
 *   will be returned.
 *
 * Since: 2.8.0
 **/
int
gnutls_x509_crq_get_extension_info(gnutls_x509_crq_t crq, unsigned indx,
				   void *oid, size_t * sizeof_oid,
				   unsigned int *critical)
{
	int result;
	char str_critical[10];
	char name[MAX_NAME_SIZE];
	char *extensions = NULL;
	size_t extensions_size = 0;
	ASN1_TYPE c2;
	int len;

	if (!crq) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	/* read extensionRequest */
	result =
	    gnutls_x509_crq_get_attribute_by_oid(crq,
						 "1.2.840.113549.1.9.14",
						 0, NULL,
						 &extensions_size);
	if (result == GNUTLS_E_SHORT_MEMORY_BUFFER) {
		extensions = gnutls_malloc(extensions_size);
		if (extensions == NULL) {
			gnutls_assert();
			return GNUTLS_E_MEMORY_ERROR;
		}

		result = gnutls_x509_crq_get_attribute_by_oid(crq,
							      "1.2.840.113549.1.9.14",
							      0,
							      extensions,
							      &extensions_size);
	}
	if (result < 0) {
		gnutls_assert();
		goto out;
	}

	result =
	    asn1_create_element(_gnutls_get_pkix(), "PKIX1.Extensions",
				&c2);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto out;
	}

	result = _asn1_strict_der_decode(&c2, extensions, extensions_size, NULL);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		asn1_delete_structure(&c2);
		result = _gnutls_asn2err(result);
		goto out;
	}

	snprintf(name, sizeof(name), "?%u.extnID", indx + 1);

	len = *sizeof_oid;
	result = asn1_read_value(c2, name, oid, &len);
	*sizeof_oid = len;

	if (result == ASN1_ELEMENT_NOT_FOUND) {
		asn1_delete_structure(&c2);
		result = GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
		goto out;
	} else if (result < 0) {
		gnutls_assert();
		asn1_delete_structure(&c2);
		result = _gnutls_asn2err(result);
		goto out;
	}

	snprintf(name, sizeof(name), "?%u.critical", indx + 1);
	len = sizeof(str_critical);
	result = asn1_read_value(c2, name, str_critical, &len);

	asn1_delete_structure(&c2);

	if (result < 0) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto out;
	}

	if (critical) {
		if (str_critical[0] == 'T')
			*critical = 1;
		else
			*critical = 0;
	}

	result = 0;

      out:
	gnutls_free(extensions);
	return result;
}

/**
 * gnutls_x509_crq_get_extension_data:
 * @crq: should contain a #gnutls_x509_crq_t type
 * @indx: Specifies which extension number to get. Use (0) to get the first one.
 * @data: a pointer to a structure to hold the data (may be null)
 * @sizeof_data: initially holds the size of @oid
 *
 * This function will return the requested extension data in the
 * certificate.  The extension data will be stored as a string in the
 * provided buffer.
 *
 * Use gnutls_x509_crq_get_extension_info() to extract the OID and
 * critical flag.  Use gnutls_x509_crq_get_extension_by_oid() instead,
 * if you want to get data indexed by the extension OID rather than
 * sequence.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error code in case of an error.  If your have reached the
 *   last extension available %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE
 *   will be returned.
 *
 * Since: 2.8.0
 **/
int
gnutls_x509_crq_get_extension_data(gnutls_x509_crq_t crq, unsigned indx,
				   void *data, size_t * sizeof_data)
{
	int ret;
	gnutls_datum_t raw;

	ret = gnutls_x509_crq_get_extension_data2(crq, indx, &raw);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = _gnutls_copy_data(&raw, data, sizeof_data);
	if (ret == GNUTLS_E_SHORT_MEMORY_BUFFER && data == NULL)
		ret = 0;
	gnutls_free(raw.data);
	return ret;
}

/**
 * gnutls_x509_crq_get_extension_data2:
 * @crq: should contain a #gnutls_x509_crq_t type
 * @extension_id: An X.509 extension OID.
 * @indx: Specifies which extension OID to read. Use (0) to get the first one.
 * @data: will contain the extension DER-encoded data
 *
 * This function will return the requested extension data in the
 * certificate request.  The extension data will be allocated using
 * gnutls_malloc().
 *
 * Use gnutls_x509_crq_get_extension_info() to extract the OID.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned,
 *   otherwise a negative error code is returned.  If you have reached the
 *   last extension available %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE
 *   will be returned.
 *
 * Since: 3.3.0
 **/
int
gnutls_x509_crq_get_extension_data2(gnutls_x509_crq_t crq,
			       unsigned indx, gnutls_datum_t * data)
{
	int ret, result;
	char name[MAX_NAME_SIZE];
	unsigned char *extensions = NULL;
	size_t extensions_size = 0;
	ASN1_TYPE c2 = ASN1_TYPE_EMPTY;

	if (!crq) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	/* read extensionRequest */
	ret =
	    gnutls_x509_crq_get_attribute_by_oid(crq,
						 "1.2.840.113549.1.9.14",
						 0, NULL,
						 &extensions_size);
	if (ret != GNUTLS_E_SHORT_MEMORY_BUFFER) {
		gnutls_assert();
		if (ret == 0)
			return GNUTLS_E_INTERNAL_ERROR;
		return ret;
	}

	extensions = gnutls_malloc(extensions_size);
	if (extensions == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	ret =
	    gnutls_x509_crq_get_attribute_by_oid(crq,
						 "1.2.840.113549.1.9.14",
						 0, extensions,
						 &extensions_size);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	result =
	    asn1_create_element(_gnutls_get_pkix(), "PKIX1.Extensions",
				&c2);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		ret = _gnutls_asn2err(result);
		goto cleanup;
	}

	result = _asn1_strict_der_decode(&c2, extensions, extensions_size, NULL);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		ret = _gnutls_asn2err(result);
		goto cleanup;
	}

	snprintf(name, sizeof(name), "?%u.extnValue", indx + 1);

	ret = _gnutls_x509_read_value(c2, name, data);
	if (ret == GNUTLS_E_ASN1_ELEMENT_NOT_FOUND) {
		ret = GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
		goto cleanup;
	} else if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = 0;
 cleanup:
	asn1_delete_structure(&c2);
	gnutls_free(extensions);
	return ret;
}

/**
 * gnutls_x509_crq_get_key_usage:
 * @crq: should contain a #gnutls_x509_crq_t type
 * @key_usage: where the key usage bits will be stored
 * @critical: will be non-zero if the extension is marked as critical
 *
 * This function will return certificate's key usage, by reading the
 * keyUsage X.509 extension (2.5.29.15).  The key usage value will
 * ORed values of the: %GNUTLS_KEY_DIGITAL_SIGNATURE,
 * %GNUTLS_KEY_NON_REPUDIATION, %GNUTLS_KEY_KEY_ENCIPHERMENT,
 * %GNUTLS_KEY_DATA_ENCIPHERMENT, %GNUTLS_KEY_KEY_AGREEMENT,
 * %GNUTLS_KEY_KEY_CERT_SIGN, %GNUTLS_KEY_CRL_SIGN,
 * %GNUTLS_KEY_ENCIPHER_ONLY, %GNUTLS_KEY_DECIPHER_ONLY.
 *
 * Returns: the certificate key usage, or a negative error code in case of
 *   parsing error.  If the certificate does not contain the keyUsage
 *   extension %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE will be
 *   returned.
 *
 * Since: 2.8.0
 **/
int
gnutls_x509_crq_get_key_usage(gnutls_x509_crq_t crq,
			      unsigned int *key_usage,
			      unsigned int *critical)
{
	int result;
	uint8_t buf[128];
	size_t buf_size = sizeof(buf);
	gnutls_datum_t bd;

	if (crq == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	result = gnutls_x509_crq_get_extension_by_oid(crq, "2.5.29.15", 0,
						      buf, &buf_size,
						      critical);
	if (result < 0) {
		gnutls_assert();
		return result;
	}

	bd.data = buf;
	bd.size = buf_size;
	result = gnutls_x509_ext_import_key_usage(&bd, key_usage);
	if (result < 0) {
		gnutls_assert();
		return result;
	}

	return 0;
}

/**
 * gnutls_x509_crq_get_basic_constraints:
 * @crq: should contain a #gnutls_x509_crq_t type
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
 *   returned, or (0) if the certificate does not have CA flag set.
 *   A negative error code may be returned in case of errors.  If the
 *   certificate does not contain the basicConstraints extension
 *   %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE will be returned.
 *
 * Since: 2.8.0
 **/
int
gnutls_x509_crq_get_basic_constraints(gnutls_x509_crq_t crq,
				      unsigned int *critical,
				      unsigned int *ca, int *pathlen)
{
	int result;
	unsigned int tmp_ca;
	uint8_t buf[256];
	size_t buf_size = sizeof(buf);
	gnutls_datum_t bd;

	if (crq == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	result = gnutls_x509_crq_get_extension_by_oid(crq, "2.5.29.19", 0,
						      buf, &buf_size,
						      critical);
	if (result < 0) {
		gnutls_assert();
		return result;
	}

	bd.data = buf;
	bd.size = buf_size;
	result = gnutls_x509_ext_import_basic_constraints(&bd, &tmp_ca, pathlen);
	if (ca)
		*ca = tmp_ca;

	if (result < 0) {
		gnutls_assert();
		return result;
	}

	return tmp_ca;
}

static int
get_subject_alt_name(gnutls_x509_crq_t crq,
		     unsigned int seq, void *ret,
		     size_t * ret_size, unsigned int *ret_type,
		     unsigned int *critical, int othername_oid)
{
	int result;
	ASN1_TYPE c2 = ASN1_TYPE_EMPTY;
	gnutls_x509_subject_alt_name_t type;
	gnutls_datum_t dnsname = { NULL, 0 };
	size_t dns_size = 0;

	if (crq == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	if (ret)
		memset(ret, 0, *ret_size);
	else
		*ret_size = 0;

	/* Extract extension.
	 */
	result = gnutls_x509_crq_get_extension_by_oid(crq, "2.5.29.17", 0,
						      NULL, &dns_size,
						      critical);
	if (result < 0) {
		gnutls_assert();
		return result;
	}

	dnsname.size = dns_size;
	dnsname.data = gnutls_malloc(dnsname.size);
	if (dnsname.data == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	result = gnutls_x509_crq_get_extension_by_oid(crq, "2.5.29.17", 0,
						      dnsname.data,
						      &dns_size, critical);
	if (result < 0) {
		gnutls_assert();
		gnutls_free(dnsname.data);
		return result;
	}

	result = asn1_create_element
	    (_gnutls_get_pkix(), "PKIX1.SubjectAltName", &c2);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		gnutls_free(dnsname.data);
		return _gnutls_asn2err(result);
	}

	result = _asn1_strict_der_decode(&c2, dnsname.data, dnsname.size, NULL);
	gnutls_free(dnsname.data);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		asn1_delete_structure(&c2);
		return _gnutls_asn2err(result);
	}

	result = _gnutls_parse_general_name(c2, "", seq, ret, ret_size,
					    ret_type, othername_oid);
	asn1_delete_structure(&c2);
	if (result < 0) {
		return result;
	}

	type = result;

	return type;
}

/**
 * gnutls_x509_crq_get_subject_alt_name:
 * @crq: should contain a #gnutls_x509_crq_t type
 * @seq: specifies the sequence number of the alt name, 0 for the
 *   first one, 1 for the second etc.
 * @ret: is the place where the alternative name will be copied to
 * @ret_size: holds the size of ret.
 * @ret_type: holds the #gnutls_x509_subject_alt_name_t name type
 * @critical: will be non-zero if the extension is marked as critical
 *   (may be null)
 *
 * This function will return the alternative names, contained in the
 * given certificate.  It is the same as
 * gnutls_x509_crq_get_subject_alt_name() except for the fact that it
 * will return the type of the alternative name in @ret_type even if
 * the function fails for some reason (i.e.  the buffer provided is
 * not enough).
 *
 * Returns: the alternative subject name type on success, one of the
 *   enumerated #gnutls_x509_subject_alt_name_t.  It will return
 *   %GNUTLS_E_SHORT_MEMORY_BUFFER if @ret_size is not large enough to
 *   hold the value.  In that case @ret_size will be updated with the
 *   required size.  If the certificate request does not have an
 *   Alternative name with the specified sequence number then
 *   %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE is returned.
 *
 * Since: 2.8.0
 **/
int
gnutls_x509_crq_get_subject_alt_name(gnutls_x509_crq_t crq,
				     unsigned int seq, void *ret,
				     size_t * ret_size,
				     unsigned int *ret_type,
				     unsigned int *critical)
{
	return get_subject_alt_name(crq, seq, ret, ret_size, ret_type,
				    critical, 0);
}

/**
 * gnutls_x509_crq_get_subject_alt_othername_oid:
 * @crq: should contain a #gnutls_x509_crq_t type
 * @seq: specifies the sequence number of the alt name (0 for the first one, 1 for the second etc.)
 * @ret: is the place where the otherName OID will be copied to
 * @ret_size: holds the size of ret.
 *
 * This function will extract the type OID of an otherName Subject
 * Alternative Name, contained in the given certificate, and return
 * the type as an enumerated element.
 *
 * This function is only useful if
 * gnutls_x509_crq_get_subject_alt_name() returned
 * %GNUTLS_SAN_OTHERNAME.
 *
 * Returns: the alternative subject name type on success, one of the
 *   enumerated gnutls_x509_subject_alt_name_t.  For supported OIDs,
 *   it will return one of the virtual (GNUTLS_SAN_OTHERNAME_*) types,
 *   e.g. %GNUTLS_SAN_OTHERNAME_XMPP, and %GNUTLS_SAN_OTHERNAME for
 *   unknown OIDs.  It will return %GNUTLS_E_SHORT_MEMORY_BUFFER if
 *   @ret_size is not large enough to hold the value.  In that case
 *   @ret_size will be updated with the required size.  If the
 *   certificate does not have an Alternative name with the specified
 *   sequence number and with the otherName type then
 *   %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE is returned.
 *
 * Since: 2.8.0
 **/
int
gnutls_x509_crq_get_subject_alt_othername_oid(gnutls_x509_crq_t crq,
					      unsigned int seq,
					      void *ret, size_t * ret_size)
{
	return get_subject_alt_name(crq, seq, ret, ret_size, NULL, NULL,
				    1);
}

/**
 * gnutls_x509_crq_get_extension_by_oid:
 * @crq: should contain a #gnutls_x509_crq_t type
 * @oid: holds an Object Identifier in a null terminated string
 * @indx: In case multiple same OIDs exist in the extensions, this
 *   specifies which to get. Use (0) to get the first one.
 * @buf: a pointer to a structure to hold the name (may be null)
 * @buf_size: initially holds the size of @buf
 * @critical: will be non-zero if the extension is marked as critical
 *
 * This function will return the extension specified by the OID in
 * the certificate.  The extensions will be returned as binary data
 * DER encoded, in the provided buffer.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error code in case of an error.  If the certificate does not
 *   contain the specified extension
 *   %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE will be returned.
 *
 * Since: 2.8.0
 **/
int
gnutls_x509_crq_get_extension_by_oid(gnutls_x509_crq_t crq,
				     const char *oid, unsigned indx,
				     void *buf, size_t * buf_size,
				     unsigned int *critical)
{
	int result;
	unsigned int i;
	char _oid[MAX_OID_SIZE];
	size_t oid_size;

	for (i = 0;; i++) {
		oid_size = sizeof(_oid);
		result =
		    gnutls_x509_crq_get_extension_info(crq, i, _oid,
						       &oid_size,
						       critical);
		if (result < 0) {
			gnutls_assert();
			return result;
		}

		if (strcmp(oid, _oid) == 0) {	/* found */
			if (indx == 0)
				return
				    gnutls_x509_crq_get_extension_data(crq,
								       i,
								       buf,
								       buf_size);
			else
				indx--;
		}
	}


	return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;

}

/**
 * gnutls_x509_crq_get_extension_by_oid2:
 * @crq: should contain a #gnutls_x509_crq_t type
 * @oid: holds an Object Identifier in a null terminated string
 * @indx: In case multiple same OIDs exist in the extensions, this
 *   specifies which to get. Use (0) to get the first one.
 * @output: will hold the allocated extension data
 * @critical: will be non-zero if the extension is marked as critical
 *
 * This function will return the extension specified by the OID in
 * the certificate.  The extensions will be returned as binary data
 * DER encoded, in the provided buffer.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error code in case of an error.  If the certificate does not
 *   contain the specified extension
 *   %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE will be returned.
 *
 * Since: 3.3.8
 **/
int
gnutls_x509_crq_get_extension_by_oid2(gnutls_x509_crq_t crq,
				     const char *oid, unsigned indx,
				     gnutls_datum_t *output,
				     unsigned int *critical)
{
	int result;
	unsigned int i;
	char _oid[MAX_OID_SIZE];
	size_t oid_size;

	for (i = 0;; i++) {
		oid_size = sizeof(_oid);
		result =
		    gnutls_x509_crq_get_extension_info(crq, i, _oid,
						       &oid_size,
						       critical);
		if (result < 0) {
			gnutls_assert();
			return result;
		}

		if (strcmp(oid, _oid) == 0) {	/* found */
			if (indx == 0)
				return
				    gnutls_x509_crq_get_extension_data2(crq,
								       i,
								       output);
			else
				indx--;
		}
	}

	return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;

}

/**
 * gnutls_x509_crq_set_subject_alt_name:
 * @crq: a certificate request of type #gnutls_x509_crq_t
 * @nt: is one of the #gnutls_x509_subject_alt_name_t enumerations
 * @data: The data to be set
 * @data_size: The size of data to be set
 * @flags: %GNUTLS_FSAN_SET to clear previous data or
 *   %GNUTLS_FSAN_APPEND to append.
 *
 * This function will set the subject alternative name certificate
 * extension.  It can set the following types:
 *
 * %GNUTLS_SAN_DNSNAME: as a text string
 *
 * %GNUTLS_SAN_RFC822NAME: as a text string
 *
 * %GNUTLS_SAN_URI: as a text string
 *
 * %GNUTLS_SAN_IPADDRESS: as a binary IP address (4 or 16 bytes)
 *
 * %GNUTLS_SAN_OTHERNAME_XMPP: as a UTF8 string
 *
 * Since version 3.5.7 the %GNUTLS_SAN_RFC822NAME, %GNUTLS_SAN_DNSNAME, and
 * %GNUTLS_SAN_OTHERNAME_XMPP are converted to ACE format when necessary.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 2.8.0
 **/
int
gnutls_x509_crq_set_subject_alt_name(gnutls_x509_crq_t crq,
				     gnutls_x509_subject_alt_name_t nt,
				     const void *data,
				     unsigned int data_size,
				     unsigned int flags)
{
	int result = 0;
	gnutls_datum_t der_data = { NULL, 0 };
	gnutls_datum_t prev_der_data = { NULL, 0 };
	unsigned int critical = 0;
	size_t prev_data_size = 0;

	if (crq == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	/* Check if the extension already exists.
	 */
	if (flags & GNUTLS_FSAN_APPEND) {
		result =
		    gnutls_x509_crq_get_extension_by_oid(crq, "2.5.29.17",
							 0, NULL,
							 &prev_data_size,
							 &critical);
		prev_der_data.size = prev_data_size;

		switch (result) {
		case GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE:
			/* Replacing non-existing data means the same as set data. */
			break;

		case GNUTLS_E_SUCCESS:
			prev_der_data.data =
			    gnutls_malloc(prev_der_data.size);
			if (prev_der_data.data == NULL) {
				gnutls_assert();
				return GNUTLS_E_MEMORY_ERROR;
			}

			result =
			    gnutls_x509_crq_get_extension_by_oid(crq,
								 "2.5.29.17",
								 0,
								 prev_der_data.
								 data,
								 &prev_data_size,
								 &critical);
			if (result < 0) {
				gnutls_assert();
				gnutls_free(prev_der_data.data);
				return result;
			}
			break;

		default:
			gnutls_assert();
			return result;
		}
	}

	/* generate the extension.
	 */
	result = _gnutls_x509_ext_gen_subject_alt_name(nt, NULL, data, data_size,
						       &prev_der_data,
						       &der_data);
	gnutls_free(prev_der_data.data);
	if (result < 0) {
		gnutls_assert();
		goto finish;
	}

	result =
	    _gnutls_x509_crq_set_extension(crq, "2.5.29.17", &der_data,
					   critical);

	_gnutls_free_datum(&der_data);

	if (result < 0) {
		gnutls_assert();
		return result;
	}

	return 0;

      finish:
	return result;
}

/**
 * gnutls_x509_crq_set_subject_alt_othername:
 * @crq: a certificate request of type #gnutls_x509_crq_t
 * @oid: is the othername OID
 * @data: The data to be set
 * @data_size: The size of data to be set
 * @flags: %GNUTLS_FSAN_SET to clear previous data or
 *   %GNUTLS_FSAN_APPEND to append.
 *
 * This function will set the subject alternative name certificate
 * extension.  It can set the following types:
 *
 * The values set must be binary values and must be properly DER encoded.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.5.0
 **/
int
gnutls_x509_crq_set_subject_alt_othername(gnutls_x509_crq_t crq,
				     const char *oid,
				     const void *data,
				     unsigned int data_size,
				     unsigned int flags)
{
	int result = 0;
	gnutls_datum_t der_data = { NULL, 0 };
	gnutls_datum_t encoded_data = { NULL, 0 };
	gnutls_datum_t prev_der_data = { NULL, 0 };
	unsigned int critical = 0;
	size_t prev_data_size = 0;

	if (crq == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	/* Check if the extension already exists.
	 */
	if (flags & GNUTLS_FSAN_APPEND) {
		result =
		    gnutls_x509_crq_get_extension_by_oid(crq, "2.5.29.17",
							 0, NULL,
							 &prev_data_size,
							 &critical);
		prev_der_data.size = prev_data_size;

		switch (result) {
		case GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE:
			/* Replacing non-existing data means the same as set data. */
			break;

		case GNUTLS_E_SUCCESS:
			prev_der_data.data =
			    gnutls_malloc(prev_der_data.size);
			if (prev_der_data.data == NULL) {
				gnutls_assert();
				return GNUTLS_E_MEMORY_ERROR;
			}

			result =
			    gnutls_x509_crq_get_extension_by_oid(crq,
								 "2.5.29.17",
								 0,
								 prev_der_data.
								 data,
								 &prev_data_size,
								 &critical);
			if (result < 0) {
				gnutls_assert();
				goto finish;
			}
			break;

		default:
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
	result = _gnutls_x509_ext_gen_subject_alt_name(GNUTLS_SAN_OTHERNAME, oid,
						       encoded_data.data, encoded_data.size,
						       &prev_der_data,
						       &der_data);
	if (result < 0) {
		gnutls_assert();
		goto finish;
	}

	result =
	    _gnutls_x509_crq_set_extension(crq, "2.5.29.17", &der_data,
					   critical);

	if (result < 0) {
		gnutls_assert();
		goto finish;
	}

	result = 0;

      finish:
	_gnutls_free_datum(&prev_der_data);
	_gnutls_free_datum(&der_data);
	_gnutls_free_datum(&encoded_data);
	return result;
}

/**
 * gnutls_x509_crq_set_basic_constraints:
 * @crq: a certificate request of type #gnutls_x509_crq_t
 * @ca: true(1) or false(0) depending on the Certificate authority status.
 * @pathLenConstraint: non-negative error codes indicate maximum length of path,
 *   and negative error codes indicate that the pathLenConstraints field should
 *   not be present.
 *
 * This function will set the basicConstraints certificate extension.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 2.8.0
 **/
int
gnutls_x509_crq_set_basic_constraints(gnutls_x509_crq_t crq,
				      unsigned int ca,
				      int pathLenConstraint)
{
	int result;
	gnutls_datum_t der_data;

	if (crq == NULL) {
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
	    _gnutls_x509_crq_set_extension(crq, "2.5.29.19", &der_data, 1);

	_gnutls_free_datum(&der_data);

	if (result < 0) {
		gnutls_assert();
		return result;
	}

	return 0;
}

/**
 * gnutls_x509_crq_set_key_usage:
 * @crq: a certificate request of type #gnutls_x509_crq_t
 * @usage: an ORed sequence of the GNUTLS_KEY_* elements.
 *
 * This function will set the keyUsage certificate extension.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 2.8.0
 **/
int
gnutls_x509_crq_set_key_usage(gnutls_x509_crq_t crq, unsigned int usage)
{
	int result;
	gnutls_datum_t der_data;

	if (crq == NULL) {
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
	    _gnutls_x509_crq_set_extension(crq, "2.5.29.15", &der_data, 1);

	_gnutls_free_datum(&der_data);

	if (result < 0) {
		gnutls_assert();
		return result;
	}

	return 0;
}

/**
 * gnutls_x509_crq_get_key_purpose_oid:
 * @crq: should contain a #gnutls_x509_crq_t type
 * @indx: This specifies which OID to return, use (0) to get the first one
 * @oid: a pointer to store the OID (may be %NULL)
 * @sizeof_oid: initially holds the size of @oid
 * @critical: output variable with critical flag, may be %NULL.
 *
 * This function will extract the key purpose OIDs of the Certificate
 * specified by the given index.  These are stored in the Extended Key
 * Usage extension (2.5.29.37).  See the GNUTLS_KP_* definitions for
 * human readable names.
 *
 * Returns: %GNUTLS_E_SHORT_MEMORY_BUFFER if the provided buffer is
 *   not long enough, and in that case the *@sizeof_oid will be
 *   updated with the required size.  On success 0 is returned.
 *
 * Since: 2.8.0
 **/
int
gnutls_x509_crq_get_key_purpose_oid(gnutls_x509_crq_t crq,
				    unsigned indx, void *oid,
				    size_t * sizeof_oid,
				    unsigned int *critical)
{
	char tmpstr[MAX_NAME_SIZE];
	int result, len;
	gnutls_datum_t prev = { NULL, 0 };
	ASN1_TYPE c2 = ASN1_TYPE_EMPTY;
	size_t prev_size = 0;

	if (oid)
		memset(oid, 0, *sizeof_oid);
	else
		*sizeof_oid = 0;

	/* Extract extension.
	 */
	result = gnutls_x509_crq_get_extension_by_oid(crq, "2.5.29.37", 0,
						      NULL, &prev_size,
						      critical);
	prev.size = prev_size;

	if (result < 0) {
		gnutls_assert();
		return result;
	}

	prev.data = gnutls_malloc(prev.size);
	if (prev.data == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	result = gnutls_x509_crq_get_extension_by_oid(crq, "2.5.29.37", 0,
						      prev.data,
						      &prev_size,
						      critical);
	if (result < 0) {
		gnutls_assert();
		gnutls_free(prev.data);
		return result;
	}

	result = asn1_create_element
	    (_gnutls_get_pkix(), "PKIX1.ExtKeyUsageSyntax", &c2);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		gnutls_free(prev.data);
		return _gnutls_asn2err(result);
	}

	result = _asn1_strict_der_decode(&c2, prev.data, prev.size, NULL);
	gnutls_free(prev.data);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		asn1_delete_structure(&c2);
		return _gnutls_asn2err(result);
	}

	indx++;
	/* create a string like "?1"
	 */
	snprintf(tmpstr, sizeof(tmpstr), "?%u", indx);

	len = *sizeof_oid;
	result = asn1_read_value(c2, tmpstr, oid, &len);

	*sizeof_oid = len;
	asn1_delete_structure(&c2);

	if (result == ASN1_VALUE_NOT_FOUND
	    || result == ASN1_ELEMENT_NOT_FOUND) {
		return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
	}

	if (result != ASN1_SUCCESS) {
		if (result != ASN1_MEM_ERROR)
			gnutls_assert();
		return _gnutls_asn2err(result);
	}

	return 0;
}

/**
 * gnutls_x509_crq_set_key_purpose_oid:
 * @crq: a certificate of type #gnutls_x509_crq_t
 * @oid: a pointer to a null-terminated string that holds the OID
 * @critical: Whether this extension will be critical or not
 *
 * This function will set the key purpose OIDs of the Certificate.
 * These are stored in the Extended Key Usage extension (2.5.29.37)
 * See the GNUTLS_KP_* definitions for human readable names.
 *
 * Subsequent calls to this function will append OIDs to the OID list.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 2.8.0
 **/
int
gnutls_x509_crq_set_key_purpose_oid(gnutls_x509_crq_t crq,
				    const void *oid, unsigned int critical)
{
	int result;
	gnutls_datum_t prev = { NULL, 0 }, der_data;
	ASN1_TYPE c2 = ASN1_TYPE_EMPTY;
	size_t prev_size = 0;

	/* Read existing extension, if there is one.
	 */
	result = gnutls_x509_crq_get_extension_by_oid(crq, "2.5.29.37", 0,
						      NULL, &prev_size,
						      &critical);
	prev.size = prev_size;

	switch (result) {
	case GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE:
		/* No existing extension, that's fine. */
		break;

	case GNUTLS_E_SUCCESS:
		prev.data = gnutls_malloc(prev.size);
		if (prev.data == NULL) {
			gnutls_assert();
			return GNUTLS_E_MEMORY_ERROR;
		}

		result =
		    gnutls_x509_crq_get_extension_by_oid(crq, "2.5.29.37",
							 0, prev.data,
							 &prev_size,
							 &critical);
		if (result < 0) {
			gnutls_assert();
			gnutls_free(prev.data);
			return result;
		}
		break;

	default:
		gnutls_assert();
		return result;
	}

	result = asn1_create_element(_gnutls_get_pkix(),
				     "PKIX1.ExtKeyUsageSyntax", &c2);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		gnutls_free(prev.data);
		return _gnutls_asn2err(result);
	}

	if (prev.data) {
		/* decode it.
		 */
		result =
		    _asn1_strict_der_decode(&c2, prev.data, prev.size, NULL);
		gnutls_free(prev.data);
		if (result != ASN1_SUCCESS) {
			gnutls_assert();
			asn1_delete_structure(&c2);
			return _gnutls_asn2err(result);
		}
	}

	/* generate the extension.
	 */
	/* 1. create a new element.
	 */
	result = asn1_write_value(c2, "", "NEW", 1);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		asn1_delete_structure(&c2);
		return _gnutls_asn2err(result);
	}

	/* 2. Add the OID.
	 */
	result = asn1_write_value(c2, "?LAST", oid, 1);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		asn1_delete_structure(&c2);
		return _gnutls_asn2err(result);
	}

	result = _gnutls_x509_der_encode(c2, "", &der_data, 0);
	asn1_delete_structure(&c2);

	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	result = _gnutls_x509_crq_set_extension(crq, "2.5.29.37",
						&der_data, critical);
	_gnutls_free_datum(&der_data);
	if (result < 0) {
		gnutls_assert();
		return result;
	}

	return 0;
}

/**
 * gnutls_x509_crq_get_key_id:
 * @crq: a certificate of type #gnutls_x509_crq_t
 * @flags: should be one of the flags from %gnutls_keyid_flags_t
 * @output_data: will contain the key ID
 * @output_data_size: holds the size of output_data (and will be
 *   replaced by the actual size of parameters)
 *
 * This function will return a unique ID that depends on the public key
 * parameters.  This ID can be used in checking whether a certificate
 * corresponds to the given private key.
 *
 * If the buffer provided is not long enough to hold the output, then
 * *@output_data_size is updated and GNUTLS_E_SHORT_MEMORY_BUFFER will
 * be returned.  The output will normally be a SHA-1 hash output,
 * which is 20 bytes.
 *
 * Returns: In case of failure a negative error code will be
 *   returned, and 0 on success.
 *
 * Since: 2.8.0
 **/
int
gnutls_x509_crq_get_key_id(gnutls_x509_crq_t crq, unsigned int flags,
			   unsigned char *output_data,
			   size_t * output_data_size)
{
	int ret = 0;
	gnutls_pk_params_st params;

	if (crq == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	ret = _gnutls_x509_crq_get_mpis(crq, &params);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	ret =
	    _gnutls_get_key_id(&params, output_data, output_data_size, flags);

	gnutls_pk_params_release(&params);

	return ret;
}

/**
 * gnutls_x509_crq_privkey_sign:
 * @crq: should contain a #gnutls_x509_crq_t type
 * @key: holds a private key
 * @dig: The message digest to use, i.e., %GNUTLS_DIG_SHA1
 * @flags: must be 0
 *
 * This function will sign the certificate request with a private key.
 * This must be the same key as the one used in
 * gnutls_x509_crt_set_key() since a certificate request is self
 * signed.
 *
 * This must be the last step in a certificate request generation
 * since all the previously set parameters are now signed.
 *
 * A known limitation of this function is, that a newly-signed request will not
 * be fully functional (e.g., for signature verification), until it
 * is exported an re-imported.
 *
 * After GnuTLS 3.6.1 the value of @dig may be %GNUTLS_DIG_UNKNOWN,
 * and in that case, a suitable but reasonable for the key algorithm will be selected.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, otherwise a negative error code.
 *   %GNUTLS_E_ASN1_VALUE_NOT_FOUND is returned if you didn't set all
 *   information in the certificate request (e.g., the version using
 *   gnutls_x509_crq_set_version()).
 *
 * Since: 2.12.0
 **/
int
gnutls_x509_crq_privkey_sign(gnutls_x509_crq_t crq, gnutls_privkey_t key,
			     gnutls_digest_algorithm_t dig,
			     unsigned int flags)
{
	int result;
	gnutls_datum_t signature;
	gnutls_datum_t tbs;
	gnutls_pk_algorithm_t pk;
	gnutls_x509_spki_st params;
	const gnutls_sign_entry_st *se;

	if (crq == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	/* Make sure version field is set. */
	if (gnutls_x509_crq_get_version(crq) ==
	    GNUTLS_E_ASN1_VALUE_NOT_FOUND) {
		result = gnutls_x509_crq_set_version(crq, 1);
		if (result < 0) {
			gnutls_assert();
			return result;
		}
	}

	if (dig == 0) {
		/* attempt to find a reasonable choice */
		gnutls_pubkey_t pubkey;
		int ret;

		ret = gnutls_pubkey_init(&pubkey);
		if (ret < 0)
			return gnutls_assert_val(ret);

		ret = gnutls_pubkey_import_privkey(pubkey, key, 0, 0);
		if (ret < 0) {
			gnutls_pubkey_deinit(pubkey);
			return gnutls_assert_val(ret);
		}
		ret = gnutls_pubkey_get_preferred_hash_algorithm(pubkey, &dig, NULL);
		gnutls_pubkey_deinit(pubkey);

		if (ret < 0)
			return gnutls_assert_val(ret);
	}

	result = _gnutls_privkey_get_spki_params(key, &params);
	if (result < 0) {
		gnutls_assert();
		return result;
	}

	pk = gnutls_privkey_get_pk_algorithm(key, NULL);
	result = _gnutls_privkey_update_spki_params(key, pk, dig, 0, &params);
	if (result < 0) {
		gnutls_assert();
		return result;
	}

	/* Step 1. Self sign the request.
	 */
	result =
	    _gnutls_x509_get_tbs(crq->crq, "certificationRequestInfo",
				 &tbs);

	if (result < 0) {
		gnutls_assert();
		return result;
	}

	se = _gnutls_pk_to_sign_entry(params.pk, dig);
	if (se == NULL)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	FIX_SIGN_PARAMS(params, flags, dig);

	result = privkey_sign_and_hash_data(key, se,
					    &tbs, &signature, &params);
	gnutls_free(tbs.data);

	if (result < 0) {
		gnutls_assert();
		return result;
	}

	/* Step 2. write the signature (bits)
	 */
	result =
	    asn1_write_value(crq->crq, "signature", signature.data,
			     signature.size * 8);

	_gnutls_free_datum(&signature);

	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	/* Step 3. Write the signatureAlgorithm field.
	 */
	result =
	    _gnutls_x509_write_sign_params(crq->crq, "signatureAlgorithm",
					   se, &params);
	if (result < 0) {
		gnutls_assert();
		return result;
	}

	return 0;
}


/**
 * gnutls_x509_crq_verify:
 * @crq: is the crq to be verified
 * @flags: Flags that may be used to change the verification algorithm. Use OR of the gnutls_certificate_verify_flags enumerations.
 *
 * This function will verify self signature in the certificate
 * request and return its status.
 *
 * Returns: In case of a verification failure %GNUTLS_E_PK_SIG_VERIFY_FAILED 
 * is returned, and zero or positive code on success.
 *
 * Since 2.12.0
 **/
int gnutls_x509_crq_verify(gnutls_x509_crq_t crq, unsigned int flags)
{
	gnutls_datum_t data = { NULL, 0 };
	gnutls_datum_t signature = { NULL, 0 };
	gnutls_pk_params_st params;
	gnutls_x509_spki_st sign_params;
	const gnutls_sign_entry_st *se;
	int ret;

	gnutls_pk_params_init(&params);

	ret =
	    _gnutls_x509_get_signed_data(crq->crq, NULL,
					 "certificationRequestInfo",
					 &data);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	ret =
	    _gnutls_x509_get_signature_algorithm(crq->crq,
						 "signatureAlgorithm");
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	se = _gnutls_sign_to_entry(ret);
	if (se == NULL) {
		gnutls_assert();
		ret = GNUTLS_E_UNSUPPORTED_SIGNATURE_ALGORITHM;
		goto cleanup;
	}

	ret =
	    _gnutls_x509_get_signature(crq->crq, "signature", &signature);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = _gnutls_x509_crq_get_mpis(crq, &params);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = _gnutls_x509_read_sign_params(crq->crq,
					    "signatureAlgorithm",
					    &sign_params);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret =
	    pubkey_verify_data(se, hash_to_entry(se->hash), &data, &signature,
			       &params, &sign_params, flags);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = 0;

      cleanup:
	_gnutls_free_datum(&data);
	_gnutls_free_datum(&signature);
	gnutls_pk_params_release(&params);

	return ret;
}

/**
 * gnutls_x509_crq_set_private_key_usage_period:
 * @crq: a certificate of type #gnutls_x509_crq_t
 * @activation: The activation time
 * @expiration: The expiration time
 *
 * This function will set the private key usage period extension (2.5.29.16).
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_x509_crq_set_private_key_usage_period(gnutls_x509_crq_t crq,
					     time_t activation,
					     time_t expiration)
{
	int result;
	gnutls_datum_t der_data;
	ASN1_TYPE c2 = ASN1_TYPE_EMPTY;

	if (crq == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	result =
	    asn1_create_element(_gnutls_get_pkix(),
				"PKIX1.PrivateKeyUsagePeriod", &c2);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	result = _gnutls_x509_set_time(c2, "notBefore", activation, 1);
	if (result < 0) {
		gnutls_assert();
		goto cleanup;
	}

	result = _gnutls_x509_set_time(c2, "notAfter", expiration, 1);
	if (result < 0) {
		gnutls_assert();
		goto cleanup;
	}

	result = _gnutls_x509_der_encode(c2, "", &der_data, 0);
	if (result < 0) {
		gnutls_assert();
		goto cleanup;
	}

	result = _gnutls_x509_crq_set_extension(crq, "2.5.29.16",
						&der_data, 0);

	_gnutls_free_datum(&der_data);

      cleanup:
	asn1_delete_structure(&c2);

	return result;
}

/**
 * gnutls_x509_crq_get_tlsfeatures:
 * @crq: An X.509 certificate request
 * @features: If the function succeeds, the
 *   features will be stored in this variable.
 * @flags: zero or %GNUTLS_EXT_FLAG_APPEND
 * @critical: the extension status
 *
 * This function will get the X.509 TLS features
 * extension structure from the certificate request.
 * The returned structure needs to be freed using
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
int gnutls_x509_crq_get_tlsfeatures(gnutls_x509_crq_t crq,
				    gnutls_x509_tlsfeatures_t features,
				    unsigned int flags,
				    unsigned int *critical)
{
	int ret;
	gnutls_datum_t der = {NULL, 0};

	if (crq == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	if ((ret =
		 gnutls_x509_crq_get_extension_by_oid2(crq, GNUTLS_X509EXT_OID_TLSFEATURES, 0,
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
 * gnutls_x509_crq_set_tlsfeatures:
 * @crq: An X.509 certificate request
 * @features: If the function succeeds, the
 *   features will be added to the certificate
 *   request.
 *
 * This function will set the certificate request's
 * X.509 TLS extension from the given structure.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned,
 *   otherwise a negative error value.
 *
 * Since: 3.5.1
 **/
int gnutls_x509_crq_set_tlsfeatures(gnutls_x509_crq_t crq,
				    gnutls_x509_tlsfeatures_t features)
{
	int ret;
	gnutls_datum_t der;

	if (crq == NULL || features == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	ret = gnutls_x509_ext_export_tlsfeatures(features, &der);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	ret = _gnutls_x509_crq_set_extension(crq, GNUTLS_X509EXT_OID_TLSFEATURES, &der, 0);

	_gnutls_free_datum(&der);

	if (ret < 0) {
		gnutls_assert();
	}

	return ret;
}

/**
 * gnutls_x509_crq_set_extension_by_oid:
 * @crq: a certificate of type #gnutls_x509_crq_t
 * @oid: holds an Object Identifier in null terminated string
 * @buf: a pointer to a DER encoded data
 * @sizeof_buf: holds the size of @buf
 * @critical: should be non-zero if the extension is to be marked as critical
 *
 * This function will set an the extension, by the specified OID, in
 * the certificate request.  The extension data should be binary data DER
 * encoded.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_x509_crq_set_extension_by_oid(gnutls_x509_crq_t crq,
				     const char *oid, const void *buf,
				     size_t sizeof_buf,
				     unsigned int critical)
{
	int result;
	gnutls_datum_t der_data;

	der_data.data = (void *) buf;
	der_data.size = sizeof_buf;

	if (crq == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	result =
	    _gnutls_x509_crq_set_extension(crq, oid, &der_data, critical);
	if (result < 0) {
		gnutls_assert();
		return result;
	}

	return 0;

}

/**
 * gnutls_x509_crq_set_spki:
 * @crq: a certificate request of type #gnutls_x509_crq_t
 * @spki: a SubjectPublicKeyInfo structure of type #gnutls_x509_spki_t
 * @flags: must be zero
 *
 * This function will set the certificate request's subject public key
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
gnutls_x509_crq_set_spki(gnutls_x509_crq_t crq,
			 const gnutls_x509_spki_t spki,
			 unsigned int flags)
{
	int ret;
	gnutls_pk_algorithm_t crq_pk;
	gnutls_x509_spki_st tpki;
	gnutls_pk_params_st params;
	unsigned bits;

	if (crq == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	ret = _gnutls_x509_crq_get_mpis(crq, &params);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	bits = pubkey_to_bits(&params);
	crq_pk = params.algo;

	if (!_gnutls_pk_are_compat(crq_pk, spki->pk)) {
                ret = gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);
                goto cleanup;
	}

	if (spki->pk != GNUTLS_PK_RSA_PSS) {
		if (crq_pk == spki->pk) {
			ret = 0;
			goto cleanup;
		}

		gnutls_assert();
		ret = GNUTLS_E_INVALID_REQUEST;
		goto cleanup;
	}

	memset(&tpki, 0, sizeof(gnutls_x509_spki_st));

	if (crq_pk == GNUTLS_PK_RSA) {
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
			ret =
			    _gnutls_find_rsa_pss_salt_size(bits, me,
							   spki->salt_size);
			if (ret < 0) {
				gnutls_assert();
				goto cleanup;
			}
			tpki.salt_size = ret;
		} else
			tpki.salt_size = spki->salt_size;
	} else if (crq_pk == GNUTLS_PK_RSA_PSS) {
		ret = _gnutls_x509_crq_read_spki_params(crq, &tpki);
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

	ret = _gnutls_x509_write_spki_params(crq->crq,
						"certificationRequestInfo."
						"subjectPKInfo."
						"algorithm",
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
