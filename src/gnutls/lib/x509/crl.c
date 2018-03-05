/*
 * Copyright (C) 2003-2016 Free Software Foundation, Inc.
 * Copyright (C) 2015-2016 Red Hat, Inc.
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 */

#include "gnutls_int.h"
#include <libtasn1.h>

#include <datum.h>
#include <global.h>
#include "errors.h"
#include <common.h>
#include <x509_b64.h>
#include <x509_int.h>
#include <x509.h>

static int crl_reinit(gnutls_x509_crl_t crl)
{
int result;

	if (crl->crl)
		asn1_delete_structure(&crl->crl);

	result = asn1_create_element(_gnutls_get_pkix(),
				 "PKIX1.CertificateList",
				 &crl->crl);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}
	crl->rcache = NULL;
	crl->rcache_idx = 0;
	crl->raw_issuer_dn.size = 0;

	return 0;
}

/**
 * gnutls_x509_crl_init:
 * @crl: A pointer to the type to be initialized
 *
 * This function will initialize a CRL structure. CRL stands for
 * Certificate Revocation List. A revocation list usually contains
 * lists of certificate serial numbers that have been revoked by an
 * Authority. The revocation lists are always signed with the
 * authority's private key.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int gnutls_x509_crl_init(gnutls_x509_crl_t * crl)
{
	FAIL_IF_LIB_ERROR;

	*crl = gnutls_calloc(1, sizeof(gnutls_x509_crl_int));

	if (*crl) {
		int result = crl_reinit(*crl);
		if (result < 0) {
			gnutls_assert();
			gnutls_free(*crl);
			return result;
		}
		return 0;	/* success */
	}
	return GNUTLS_E_MEMORY_ERROR;
}

/**
 * gnutls_x509_crl_deinit:
 * @crl: The data to be deinitialized
 *
 * This function will deinitialize a CRL structure.
 **/
void gnutls_x509_crl_deinit(gnutls_x509_crl_t crl)
{
	if (!crl)
		return;

	if (crl->crl)
		asn1_delete_structure(&crl->crl);
	gnutls_free(crl->der.data);

	gnutls_free(crl);
}

/**
 * gnutls_x509_crl_import:
 * @crl: The data to store the parsed CRL.
 * @data: The DER or PEM encoded CRL.
 * @format: One of DER or PEM
 *
 * This function will convert the given DER or PEM encoded CRL
 * to the native #gnutls_x509_crl_t format. The output will be stored in 'crl'.
 *
 * If the CRL is PEM encoded it should have a header of "X509 CRL".
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_x509_crl_import(gnutls_x509_crl_t crl,
		       const gnutls_datum_t * data,
		       gnutls_x509_crt_fmt_t format)
{
	int result = 0;

	if (crl == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	_gnutls_free_datum(&crl->der);

	/* If the CRL is in PEM format then decode it
	 */
	if (format == GNUTLS_X509_FMT_PEM) {
		result =
		    _gnutls_fbase64_decode(PEM_CRL, data->data, data->size,
					   &crl->der);

		if (result < 0) {
			gnutls_assert();
			return result;
		}
	} else {
		result = _gnutls_set_datum(&crl->der, data->data, data->size);
		if (result < 0) {
			gnutls_assert();
			return result;
		}
	}

	if (crl->expanded) {
		result = crl_reinit(crl);
		if (result < 0) {
			gnutls_assert();
			goto cleanup;
		}
	}
	crl->expanded = 1;

	result =
	    _asn1_strict_der_decode(&crl->crl, crl->der.data, crl->der.size, NULL);
	if (result != ASN1_SUCCESS) {
		result = _gnutls_asn2err(result);
		gnutls_assert();
		goto cleanup;
	}

	result = _gnutls_x509_get_raw_field2(crl->crl, &crl->der,
					  "tbsCertList.issuer.rdnSequence",
					  &crl->raw_issuer_dn);
	if (result < 0) {
		gnutls_assert();
		goto cleanup;
	}

	return 0;

      cleanup:
	_gnutls_free_datum(&crl->der);
	return result;
}


/**
 * gnutls_x509_crl_get_issuer_dn:
 * @crl: should contain a gnutls_x509_crl_t type
 * @buf: a pointer to a structure to hold the peer's name (may be null)
 * @sizeof_buf: initially holds the size of @buf
 *
 * This function will copy the name of the CRL issuer in the provided
 * buffer. The name will be in the form "C=xxxx,O=yyyy,CN=zzzz" as
 * described in RFC4514. The output string will be ASCII or UTF-8
 * encoded, depending on the certificate data.
 *
 * If buf is %NULL then only the size will be filled.
 *
 * This function does not output a fully RFC4514 compliant string, if
 * that is required see gnutls_x509_crl_get_issuer_dn3().
 *
 * Returns: %GNUTLS_E_SHORT_MEMORY_BUFFER if the provided buffer is
 * not long enough, and in that case the sizeof_buf will be updated
 * with the required size, and 0 on success.
 *
 **/
int
gnutls_x509_crl_get_issuer_dn(const gnutls_x509_crl_t crl, char *buf,
			      size_t * sizeof_buf)
{
	if (crl == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	return _gnutls_x509_parse_dn(crl->crl,
				     "tbsCertList.issuer.rdnSequence",
				     buf, sizeof_buf, GNUTLS_X509_DN_FLAG_COMPAT);
}

/**
 * gnutls_x509_crl_get_issuer_dn_by_oid:
 * @crl: should contain a gnutls_x509_crl_t type
 * @oid: holds an Object Identified in null terminated string
 * @indx: In case multiple same OIDs exist in the RDN, this specifies which to send. Use (0) to get the first one.
 * @raw_flag: If non-zero returns the raw DER data of the DN part.
 * @buf: a pointer to a structure to hold the peer's name (may be null)
 * @sizeof_buf: initially holds the size of @buf
 *
 * This function will extract the part of the name of the CRL issuer
 * specified by the given OID. The output will be encoded as described
 * in RFC4514. The output string will be ASCII or UTF-8 encoded,
 * depending on the certificate data.
 *
 * Some helper macros with popular OIDs can be found in gnutls/x509.h
 * If raw flag is (0), this function will only return known OIDs as
 * text. Other OIDs will be DER encoded, as described in RFC4514 -- in
 * hex format with a '#' prefix.  You can check about known OIDs
 * using gnutls_x509_dn_oid_known().
 *
 * If buf is null then only the size will be filled.
 *
 * Returns: %GNUTLS_E_SHORT_MEMORY_BUFFER if the provided buffer is
 * not long enough, and in that case the sizeof_buf will be updated
 * with the required size, and 0 on success.
 **/
int
gnutls_x509_crl_get_issuer_dn_by_oid(gnutls_x509_crl_t crl,
				     const char *oid, unsigned indx,
				     unsigned int raw_flag, void *buf,
				     size_t * sizeof_buf)
{
	gnutls_datum_t td;
	int ret;

	if (crl == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	ret = _gnutls_x509_parse_dn_oid(crl->crl,
					"tbsCertList.issuer.rdnSequence",
					oid, indx, raw_flag, &td);
	if (ret < 0)
		return gnutls_assert_val(ret);

	return _gnutls_strdatum_to_buf(&td, buf, sizeof_buf);
}


/**
 * gnutls_x509_crl_get_dn_oid:
 * @crl: should contain a gnutls_x509_crl_t type
 * @indx: Specifies which DN OID to send. Use (0) to get the first one.
 * @oid: a pointer to store the OID (may be null)
 * @sizeof_oid: initially holds the size of 'oid'
 *
 * This function will extract the requested OID of the name of the CRL
 * issuer, specified by the given index.
 *
 * If oid is null then only the size will be filled.
 *
 * Returns: %GNUTLS_E_SHORT_MEMORY_BUFFER if the provided buffer is
 * not long enough, and in that case the sizeof_oid will be updated
 * with the required size.  On success 0 is returned.
 **/
int
gnutls_x509_crl_get_dn_oid(gnutls_x509_crl_t crl,
			   unsigned indx, void *oid, size_t * sizeof_oid)
{
	if (crl == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	return _gnutls_x509_get_dn_oid(crl->crl,
				       "tbsCertList.issuer.rdnSequence",
				       indx, oid, sizeof_oid);
}

/**
 * gnutls_x509_crl_get_issuer_dn2:
 * @crl: should contain a #gnutls_x509_crl_t type
 * @dn: a pointer to a structure to hold the name
 *
 * This function will allocate buffer and copy the name of the CRL issuer.
 * The name will be in the form "C=xxxx,O=yyyy,CN=zzzz" as
 * described in RFC4514. The output string will be ASCII or UTF-8
 * encoded, depending on the certificate data.
 *
 * This function does not output a fully RFC4514 compliant string, if
 * that is required see gnutls_x509_crl_get_issuer_dn3().
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.1.10
 **/
int
gnutls_x509_crl_get_issuer_dn2(gnutls_x509_crl_t crl, gnutls_datum_t * dn)
{
	if (crl == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	return _gnutls_x509_get_dn(crl->crl,
				   "tbsCertList.issuer.rdnSequence",
				   dn, GNUTLS_X509_DN_FLAG_COMPAT);
}

/**
 * gnutls_x509_crl_get_issuer_dn3:
 * @crl: should contain a #gnutls_x509_crl_t type
 * @dn: a pointer to a structure to hold the name
 * @flags: zero or %GNUTLS_X509_DN_FLAG_COMPAT
 *
 * This function will allocate buffer and copy the name of the CRL issuer.
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
gnutls_x509_crl_get_issuer_dn3(gnutls_x509_crl_t crl, gnutls_datum_t * dn, unsigned flags)
{
	if (crl == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	return _gnutls_x509_get_dn(crl->crl,
				   "tbsCertList.issuer.rdnSequence",
				   dn, flags);
}

/**
 * gnutls_x509_crl_get_signature_algorithm:
 * @crl: should contain a #gnutls_x509_crl_t type
 *
 * This function will return a value of the #gnutls_sign_algorithm_t
 * enumeration that is the signature algorithm.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int gnutls_x509_crl_get_signature_algorithm(gnutls_x509_crl_t crl)
{
	int result;
	gnutls_datum_t sa;

	if (crl == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	/* Read the signature algorithm. Note that parameters are not
	 * read. They will be read from the issuer's certificate if needed.
	 */

	result =
	    _gnutls_x509_read_value(crl->crl,
				    "signatureAlgorithm.algorithm", &sa);

	if (result < 0) {
		gnutls_assert();
		return result;
	}

	result = gnutls_oid_to_sign((const char *) sa.data);

	_gnutls_free_datum(&sa);

	return result;
}

/**
 * gnutls_x509_crl_get_signature_oid:
 * @crl: should contain a #gnutls_x509_crl_t type
 * @oid: a pointer to a buffer to hold the OID (may be null)
 * @oid_size: initially holds the size of @oid
 *
 * This function will return the OID of the signature algorithm
 * that has been used to sign this CRL. This is function
 * is useful in the case gnutls_x509_crl_get_signature_algorithm()
 * returned %GNUTLS_SIGN_UNKNOWN.
 *
 * Returns: zero or a negative error code on error.
 *
 * Since: 3.5.0
 **/
int gnutls_x509_crl_get_signature_oid(gnutls_x509_crl_t crl, char *oid, size_t *oid_size)
{
	char str[MAX_OID_SIZE];
	int len, result, ret;
	gnutls_datum_t out;

	len = sizeof(str);
	result = asn1_read_value(crl->crl, "signatureAlgorithm.algorithm", str, &len);
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
 * gnutls_x509_crl_get_signature:
 * @crl: should contain a gnutls_x509_crl_t type
 * @sig: a pointer where the signature part will be copied (may be null).
 * @sizeof_sig: initially holds the size of @sig
 *
 * This function will extract the signature field of a CRL.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value. 
 **/
int
gnutls_x509_crl_get_signature(gnutls_x509_crl_t crl,
			      char *sig, size_t * sizeof_sig)
{
	int result;
	unsigned int bits;
	int len;

	if (crl == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	len = 0;
	result = asn1_read_value(crl->crl, "signature", NULL, &len);

	if (result != ASN1_MEM_ERROR) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	bits = len;
	if (bits % 8 != 0) {
		gnutls_assert();
		return GNUTLS_E_CERTIFICATE_ERROR;
	}

	len = bits / 8;

	if (*sizeof_sig < (unsigned) len) {
		*sizeof_sig = bits / 8;
		return GNUTLS_E_SHORT_MEMORY_BUFFER;
	}

	result = asn1_read_value(crl->crl, "signature", sig, &len);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	return 0;
}

/**
 * gnutls_x509_crl_get_version:
 * @crl: should contain a #gnutls_x509_crl_t type
 *
 * This function will return the version of the specified CRL.
 *
 * Returns: The version number, or a negative error code on error.
 **/
int gnutls_x509_crl_get_version(gnutls_x509_crl_t crl)
{
	uint8_t version[8];
	int len, result;

	if (crl == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	len = sizeof(version);
	if ((result =
	     asn1_read_value(crl->crl, "tbsCertList.version", version,
			     &len)) != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	return (int) version[0] + 1;
}

/**
 * gnutls_x509_crl_get_this_update:
 * @crl: should contain a #gnutls_x509_crl_t type
 *
 * This function will return the time this CRL was issued.
 *
 * Returns: when the CRL was issued, or (time_t)-1 on error.
 **/
time_t gnutls_x509_crl_get_this_update(gnutls_x509_crl_t crl)
{
	if (crl == NULL) {
		gnutls_assert();
		return (time_t) - 1;
	}

	return _gnutls_x509_get_time(crl->crl, "tbsCertList.thisUpdate",
				     0);
}

/**
 * gnutls_x509_crl_get_next_update:
 * @crl: should contain a #gnutls_x509_crl_t type
 *
 * This function will return the time the next CRL will be issued.
 * This field is optional in a CRL so it might be normal to get an
 * error instead.
 *
 * Returns: when the next CRL will be issued, or (time_t)-1 on error.
 **/
time_t gnutls_x509_crl_get_next_update(gnutls_x509_crl_t crl)
{
	if (crl == NULL) {
		gnutls_assert();
		return (time_t) - 1;
	}

	return _gnutls_x509_get_time(crl->crl, "tbsCertList.nextUpdate",
				     0);
}

/**
 * gnutls_x509_crl_get_crt_count:
 * @crl: should contain a #gnutls_x509_crl_t type
 *
 * This function will return the number of revoked certificates in the
 * given CRL.
 *
 * Returns: number of certificates, a negative error code on failure.
 **/
int gnutls_x509_crl_get_crt_count(gnutls_x509_crl_t crl)
{

	int count, result;

	if (crl == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	result =
	    asn1_number_of_elements(crl->crl,
				    "tbsCertList.revokedCertificates",
				    &count);

	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return 0;	/* no certificates */
	}

	return count;
}

/**
 * gnutls_x509_crl_get_crt_serial:
 * @crl: should contain a #gnutls_x509_crl_t type
 * @indx: the index of the certificate to extract (starting from 0)
 * @serial: where the serial number will be copied
 * @serial_size: initially holds the size of serial
 * @t: if non null, will hold the time this certificate was revoked
 *
 * This function will retrieve the serial number of the specified, by
 * the index, revoked certificate.
 *
 * Note that this function will have performance issues in large sequences
 * of revoked certificates. In that case use gnutls_x509_crl_iter_crt_serial().
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_x509_crl_get_crt_serial(gnutls_x509_crl_t crl, unsigned indx,
			       unsigned char *serial,
			       size_t * serial_size, time_t * t)
{

	int result, _serial_size;
	char serial_name[MAX_NAME_SIZE];
	char date_name[MAX_NAME_SIZE];

	if (crl == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	snprintf(serial_name, sizeof(serial_name),
		 "tbsCertList.revokedCertificates.?%u.userCertificate",
		 indx + 1);
	snprintf(date_name, sizeof(date_name),
		 "tbsCertList.revokedCertificates.?%u.revocationDate",
		 indx + 1);

	_serial_size = *serial_size;
	result =
	    asn1_read_value(crl->crl, serial_name, serial, &_serial_size);

	*serial_size = _serial_size;
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		if (result == ASN1_ELEMENT_NOT_FOUND)
			return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
		return _gnutls_asn2err(result);
	}

	if (t) {
		*t = _gnutls_x509_get_time(crl->crl, date_name, 0);
	}

	return 0;
}

/**
 * gnutls_x509_crl_iter_deinit:
 * @iter: The iterator to be deinitialized
 *
 * This function will deinitialize an iterator type.
 **/
void gnutls_x509_crl_iter_deinit(gnutls_x509_crl_iter_t iter)
{
	if (!iter)
		return;

	gnutls_free(iter);
}

/**
 * gnutls_x509_crl_iter_crt_serial:
 * @crl: should contain a #gnutls_x509_crl_t type
 * @iter: A pointer to an iterator (initially the iterator should be %NULL)
 * @serial: where the serial number will be copied
 * @serial_size: initially holds the size of serial
 * @t: if non null, will hold the time this certificate was revoked
 *
 * This function performs the same as gnutls_x509_crl_get_crt_serial(),
 * but reads sequentially and keeps state in the iterator 
 * between calls. That allows it to provide better performance in sequences 
 * with many elements (50000+).
 *
 * When past the last element is accessed %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE
 * is returned and the iterator is reset.
 *
 * After use, the iterator must be deinitialized using gnutls_x509_crl_iter_deinit().
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_x509_crl_iter_crt_serial(gnutls_x509_crl_t crl,
				gnutls_x509_crl_iter_t *iter,
				unsigned char *serial,
				size_t * serial_size, time_t * t)
{

	int result, _serial_size;
	char serial_name[MAX_NAME_SIZE];
	char date_name[MAX_NAME_SIZE];

	if (crl == NULL || iter == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	if (*iter == NULL) {
		*iter = gnutls_calloc(1, sizeof(struct gnutls_x509_crl_iter));
		if (*iter == NULL)
			return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);
	}

	if ((*iter)->rcache == NULL) {
		(*iter)->rcache = asn1_find_node (crl->crl, "tbsCertList.revokedCertificates.?1");
		(*iter)->rcache_idx = 1;
	} else {
		snprintf(serial_name, sizeof(serial_name),
			 "?%d", (*iter)->rcache_idx);
		(*iter)->rcache = asn1_find_node ((*iter)->rcache, serial_name);
	}
	if ((*iter)->rcache == NULL) {
		/* reset */
		(*iter)->rcache = NULL;
		return gnutls_assert_val(GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE);
	}

	snprintf(serial_name, sizeof(serial_name),
		 "?%d.userCertificate", (*iter)->rcache_idx);

	_serial_size = *serial_size;
	result =
	    asn1_read_value((*iter)->rcache, serial_name, serial, &_serial_size);

	*serial_size = _serial_size;
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		if (result == ASN1_ELEMENT_NOT_FOUND) {
			/* reset */
			(*iter)->rcache = NULL;
			return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
		}
		return _gnutls_asn2err(result);
	}

	if (t) {
		snprintf(date_name, sizeof(date_name),
			 "?%d.revocationDate", (*iter)->rcache_idx);
		*t = _gnutls_x509_get_time((*iter)->rcache, date_name, 0);
	}

	(*iter)->rcache_idx++;

	return 0;
}

/**
 * gnutls_x509_crl_get_raw_issuer_dn:
 * @crl: should contain a gnutls_x509_crl_t type
 * @dn: will hold the starting point of the DN
 *
 * This function will return a pointer to the DER encoded DN structure
 * and the length.
 *
 * Returns: a negative error code on error, and (0) on success.
 *
 * Since: 2.12.0
 **/
int
gnutls_x509_crl_get_raw_issuer_dn(gnutls_x509_crl_t crl,
				  gnutls_datum_t * dn)
{
	if (crl->raw_issuer_dn.size != 0) {
		return _gnutls_set_datum(dn, crl->raw_issuer_dn.data,
					 crl->raw_issuer_dn.size);
	} else {
		return _gnutls_x509_get_raw_field(crl->crl, "tbsCertList.issuer.rdnSequence", dn);
	}
}

/**
 * gnutls_x509_crl_export:
 * @crl: Holds the revocation list
 * @format: the format of output params. One of PEM or DER.
 * @output_data: will contain a private key PEM or DER encoded
 * @output_data_size: holds the size of output_data (and will
 *   be replaced by the actual size of parameters)
 *
 * This function will export the revocation list to DER or PEM format.
 *
 * If the buffer provided is not long enough to hold the output, then
 * %GNUTLS_E_SHORT_MEMORY_BUFFER will be returned.
 *
 * If the structure is PEM encoded, it will have a header
 * of "BEGIN X509 CRL".
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_x509_crl_export(gnutls_x509_crl_t crl,
		       gnutls_x509_crt_fmt_t format, void *output_data,
		       size_t * output_data_size)
{
	if (crl == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	return _gnutls_x509_export_int(crl->crl, format, PEM_CRL,
				       output_data, output_data_size);
}

/**
 * gnutls_x509_crl_export2:
 * @crl: Holds the revocation list
 * @format: the format of output params. One of PEM or DER.
 * @out: will contain a private key PEM or DER encoded
 *
 * This function will export the revocation list to DER or PEM format.
 *
 * The output buffer is allocated using gnutls_malloc().
 *
 * If the structure is PEM encoded, it will have a header
 * of "BEGIN X509 CRL".
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since 3.1.3
 **/
int
gnutls_x509_crl_export2(gnutls_x509_crl_t crl,
			gnutls_x509_crt_fmt_t format, gnutls_datum_t * out)
{
	if (crl == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	return _gnutls_x509_export_int2(crl->crl, format, PEM_CRL, out);
}

/*-
 * _gnutls_x509_crl_cpy - This function copies a gnutls_x509_crl_t type
 * @dest: The data where to copy
 * @src: The data to be copied
 *
 * This function will copy an X.509 certificate structure.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 -*/
int _gnutls_x509_crl_cpy(gnutls_x509_crl_t dest, gnutls_x509_crl_t src)
{
	int ret;
	gnutls_datum_t tmp;

	ret = gnutls_x509_crl_export2(src, GNUTLS_X509_FMT_DER, &tmp);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = gnutls_x509_crl_import(dest, &tmp, GNUTLS_X509_FMT_DER);

	gnutls_free(tmp.data);

	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	return 0;

}

static int
_get_authority_key_id(gnutls_x509_crl_t cert, ASN1_TYPE * c2,
		      unsigned int *critical)
{
	int ret;
	gnutls_datum_t id;

	*c2 = ASN1_TYPE_EMPTY;

	if (cert == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	if ((ret =
	     _gnutls_x509_crl_get_extension(cert, "2.5.29.35", 0, &id,
					    critical)) < 0) {
		return gnutls_assert_val(ret);
	}

	if (id.size == 0 || id.data == NULL) {
		gnutls_assert();
		return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
	}

	ret = asn1_create_element
	    (_gnutls_get_pkix(), "PKIX1.AuthorityKeyIdentifier", c2);
	if (ret != ASN1_SUCCESS) {
		gnutls_assert();
		_gnutls_free_datum(&id);
		return _gnutls_asn2err(ret);
	}

	ret = _asn1_strict_der_decode(c2, id.data, id.size, NULL);
	_gnutls_free_datum(&id);

	if (ret != ASN1_SUCCESS) {
		gnutls_assert();
		asn1_delete_structure(c2);
		return _gnutls_asn2err(ret);
	}

	return 0;
}

/**
 * gnutls_x509_crl_get_authority_key_gn_serial:
 * @crl: should contain a #gnutls_x509_crl_t type
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
 * Returns: Returns 0 on success, or an error code.
 *
 * Since: 3.0
 **/
int
gnutls_x509_crl_get_authority_key_gn_serial(gnutls_x509_crl_t crl,
					    unsigned int seq,
					    void *alt,
					    size_t * alt_size,
					    unsigned int *alt_type,
					    void *serial,
					    size_t * serial_size,
					    unsigned int *critical)
{
	int ret, result, len;
	ASN1_TYPE c2;

	ret = _get_authority_key_id(crl, &c2, critical);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret =
	    _gnutls_parse_general_name(c2, "authorityCertIssuer", seq, alt,
				       alt_size, alt_type, 0);
	if (ret < 0) {
		ret = gnutls_assert_val(ret);
		goto fail;
	}

	if (serial) {
		len = *serial_size;
		result =
		    asn1_read_value(c2, "authorityCertSerialNumber",
				    serial, &len);

		*serial_size = len;

		if (result < 0) {
			ret = _gnutls_asn2err(result);
			goto fail;
		}

	}

	ret = 0;

      fail:
	asn1_delete_structure(&c2);

	return ret;
}


/**
 * gnutls_x509_crl_get_authority_key_id:
 * @crl: should contain a #gnutls_x509_crl_t type
 * @id: The place where the identifier will be copied
 * @id_size: Holds the size of the result field.
 * @critical: will be non-zero if the extension is marked as critical
 *   (may be null)
 *
 * This function will return the CRL authority's key identifier.  This
 * is obtained by the X.509 Authority Key identifier extension field
 * (2.5.29.35).  Note that this function 
 * only returns the keyIdentifier field of the extension and
 * %GNUTLS_E_X509_UNSUPPORTED_EXTENSION, if the extension contains
 * the name and serial number of the certificate. In that case
 * gnutls_x509_crl_get_authority_key_gn_serial() may be used.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error code in case of an error.
 *
 * Since: 2.8.0
 **/
int
gnutls_x509_crl_get_authority_key_id(gnutls_x509_crl_t crl, void *id,
				     size_t * id_size,
				     unsigned int *critical)
{
	int result, len, ret;
	ASN1_TYPE c2;

	ret = _get_authority_key_id(crl, &c2, critical);
	if (ret < 0)
		return gnutls_assert_val(ret);

	len = *id_size;
	result = asn1_read_value(c2, "keyIdentifier", id, &len);

	*id_size = len;
	asn1_delete_structure(&c2);

	if (result == ASN1_VALUE_NOT_FOUND
	    || result == ASN1_ELEMENT_NOT_FOUND)
		return
		    gnutls_assert_val(GNUTLS_E_X509_UNSUPPORTED_EXTENSION);

	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	return 0;
}

/**
 * gnutls_x509_crl_get_number:
 * @crl: should contain a #gnutls_x509_crl_t type
 * @ret: The place where the number will be copied
 * @ret_size: Holds the size of the result field.
 * @critical: will be non-zero if the extension is marked as critical
 *   (may be null)
 *
 * This function will return the CRL number extension.  This is
 * obtained by the CRL Number extension field (2.5.29.20).
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error code in case of an error.
 *
 * Since: 2.8.0
 **/
int
gnutls_x509_crl_get_number(gnutls_x509_crl_t crl, void *ret,
			   size_t * ret_size, unsigned int *critical)
{
	int result;
	gnutls_datum_t id;

	if (crl == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	if (ret)
		memset(ret, 0, *ret_size);
	else
		*ret_size = 0;

	if ((result =
	     _gnutls_x509_crl_get_extension(crl, "2.5.29.20", 0, &id,
					    critical)) < 0) {
		return result;
	}

	if (id.size == 0 || id.data == NULL) {
		gnutls_assert();
		return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
	}

	result =
	    _gnutls_x509_ext_extract_number(ret, ret_size, id.data,
					    id.size);

	_gnutls_free_datum(&id);

	if (result < 0) {
		gnutls_assert();
		return result;
	}

	return 0;
}

/**
 * gnutls_x509_crl_get_extension_oid:
 * @crl: should contain a #gnutls_x509_crl_t type
 * @indx: Specifies which extension OID to send, use (0) to get the first one.
 * @oid: a pointer to store the OID (may be null)
 * @sizeof_oid: initially holds the size of @oid
 *
 * This function will return the requested extension OID in the CRL.
 * The extension OID will be stored as a string in the provided
 * buffer.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error code in case of an error.  If your have reached the
 *   last extension available %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE
 *   will be returned.
 *
 * Since: 2.8.0
 **/
int
gnutls_x509_crl_get_extension_oid(gnutls_x509_crl_t crl, unsigned indx,
				  void *oid, size_t * sizeof_oid)
{
	int result;

	if (crl == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	result =
	    _gnutls_x509_crl_get_extension_oid(crl, indx, oid, sizeof_oid);
	if (result < 0) {
		return result;
	}

	return 0;

}

/**
 * gnutls_x509_crl_get_extension_info:
 * @crl: should contain a #gnutls_x509_crl_t type
 * @indx: Specifies which extension OID to send, use (0) to get the first one.
 * @oid: a pointer to store the OID
 * @sizeof_oid: initially holds the maximum size of @oid, on return
 *   holds actual size of @oid.
 * @critical: output variable with critical flag, may be NULL.
 *
 * This function will return the requested extension OID in the CRL,
 * and the critical flag for it.  The extension OID will be stored as
 * a string in the provided buffer.  Use
 * gnutls_x509_crl_get_extension_data() to extract the data.
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
gnutls_x509_crl_get_extension_info(gnutls_x509_crl_t crl, unsigned indx,
				   void *oid, size_t * sizeof_oid,
				   unsigned int *critical)
{
	int result;
	char str_critical[10];
	char name[MAX_NAME_SIZE];
	int len;

	if (!crl) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	snprintf(name, sizeof(name),
		 "tbsCertList.crlExtensions.?%u.extnID", indx + 1);

	len = *sizeof_oid;
	result = asn1_read_value(crl->crl, name, oid, &len);
	*sizeof_oid = len;

	if (result == ASN1_ELEMENT_NOT_FOUND)
		return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
	else if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	snprintf(name, sizeof(name),
		 "tbsCertList.crlExtensions.?%u.critical", indx + 1);
	len = sizeof(str_critical);
	result = asn1_read_value(crl->crl, name, str_critical, &len);
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
 * gnutls_x509_crl_get_extension_data:
 * @crl: should contain a #gnutls_x509_crl_t type
 * @indx: Specifies which extension OID to send. Use (0) to get the first one.
 * @data: a pointer to a structure to hold the data (may be null)
 * @sizeof_data: initially holds the size of @oid
 *
 * This function will return the requested extension data in the CRL.
 * The extension data will be stored as a string in the provided
 * buffer.
 *
 * Use gnutls_x509_crl_get_extension_info() to extract the OID and
 * critical flag.  Use gnutls_x509_crl_get_extension_info() instead,
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
gnutls_x509_crl_get_extension_data(gnutls_x509_crl_t crl, unsigned indx,
				   void *data, size_t * sizeof_data)
{
	int result, len;
	char name[MAX_NAME_SIZE];

	if (!crl) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	snprintf(name, sizeof(name),
		 "tbsCertList.crlExtensions.?%u.extnValue", indx + 1);

	len = *sizeof_data;
	result = asn1_read_value(crl->crl, name, data, &len);
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
 * gnutls_x509_crl_list_import2:
 * @crls: Will contain the parsed crl list.
 * @size: It will contain the size of the list.
 * @data: The PEM encoded CRL.
 * @format: One of DER or PEM.
 * @flags: must be (0) or an OR'd sequence of gnutls_certificate_import_flags.
 *
 * This function will convert the given PEM encoded CRL list
 * to the native gnutls_x509_crl_t format. The output will be stored
 * in @crls.  They will be automatically initialized.
 *
 * If the Certificate is PEM encoded it should have a header of "X509
 * CRL".
 *
 * Returns: the number of certificates read or a negative error value.
 *
 * Since: 3.0
 **/
int
gnutls_x509_crl_list_import2(gnutls_x509_crl_t ** crls,
			     unsigned int *size,
			     const gnutls_datum_t * data,
			     gnutls_x509_crt_fmt_t format,
			     unsigned int flags)
{
	unsigned int init = 1024;
	int ret;

	*crls = gnutls_malloc(sizeof(gnutls_x509_crl_t) * init);
	if (*crls == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	ret =
	    gnutls_x509_crl_list_import(*crls, &init, data, format,
					flags | GNUTLS_X509_CRT_LIST_IMPORT_FAIL_IF_EXCEED);
	if (ret == GNUTLS_E_SHORT_MEMORY_BUFFER) {
		*crls =
		    gnutls_realloc_fast(*crls,
					sizeof(gnutls_x509_crl_t) * init);
		if (*crls == NULL) {
			gnutls_assert();
			return GNUTLS_E_MEMORY_ERROR;
		}

		ret =
		    gnutls_x509_crl_list_import(*crls, &init, data, format,
						flags);
	}

	if (ret < 0) {
		gnutls_free(*crls);
		*crls = NULL;
		return ret;
	}

	*size = init;
	return 0;
}

/**
 * gnutls_x509_crl_list_import:
 * @crls: Indicates where the parsed CRLs will be copied to. Must not be initialized.
 * @crl_max: Initially must hold the maximum number of crls. It will be updated with the number of crls available.
 * @data: The PEM encoded CRLs
 * @format: One of DER or PEM.
 * @flags: must be (0) or an OR'd sequence of gnutls_certificate_import_flags.
 *
 * This function will convert the given PEM encoded CRL list
 * to the native gnutls_x509_crl_t format. The output will be stored
 * in @crls.  They will be automatically initialized.
 *
 * If the Certificate is PEM encoded it should have a header of "X509 CRL".
 *
 * Returns: the number of certificates read or a negative error value.
 *
 * Since: 3.0
 **/
int
gnutls_x509_crl_list_import(gnutls_x509_crl_t * crls,
			    unsigned int *crl_max,
			    const gnutls_datum_t * data,
			    gnutls_x509_crt_fmt_t format,
			    unsigned int flags)
{
	int size;
	const char *ptr;
	gnutls_datum_t tmp;
	int ret, nocopy = 0;
	unsigned int count = 0, j;

	if (format == GNUTLS_X509_FMT_DER) {
		if (*crl_max < 1) {
			*crl_max = 1;
			return GNUTLS_E_SHORT_MEMORY_BUFFER;
		}

		count = 1;	/* import only the first one */

		ret = gnutls_x509_crl_init(&crls[0]);
		if (ret < 0) {
			gnutls_assert();
			goto error;
		}

		ret = gnutls_x509_crl_import(crls[0], data, format);
		if (ret < 0) {
			gnutls_assert();
			goto error;
		}

		*crl_max = 1;
		return 1;
	}

	/* move to the certificate
	 */
	ptr = memmem(data->data, data->size,
		     PEM_CRL_SEP, sizeof(PEM_CRL_SEP) - 1);
	if (ptr == NULL) {
		gnutls_assert();
		return GNUTLS_E_BASE64_DECODING_ERROR;
	}

	count = 0;

	do {
		if (count >= *crl_max) {
			if (!
			    (flags &
			     GNUTLS_X509_CRT_LIST_IMPORT_FAIL_IF_EXCEED))
				break;
			else
				nocopy = 1;
		}

		if (!nocopy) {
			ret = gnutls_x509_crl_init(&crls[count]);
			if (ret < 0) {
				gnutls_assert();
				goto error;
			}

			tmp.data = (void *) ptr;
			tmp.size =
			    data->size - (ptr - (char *) data->data);

			ret =
			    gnutls_x509_crl_import(crls[count], &tmp,
						   GNUTLS_X509_FMT_PEM);
			if (ret < 0) {
				gnutls_assert();
				goto error;
			}
		}

		/* now we move ptr after the pem header 
		 */
		ptr++;
		/* find the next certificate (if any)
		 */
		size = data->size - (ptr - (char *) data->data);

		if (size > 0) {
			ptr =
			    memmem(ptr, size, PEM_CRL_SEP,
				   sizeof(PEM_CRL_SEP) - 1);
		} else
			ptr = NULL;

		count++;
	}
	while (ptr != NULL);

	*crl_max = count;

	if (nocopy == 0)
		return count;
	else
		return GNUTLS_E_SHORT_MEMORY_BUFFER;

      error:
	for (j = 0; j < count; j++)
		gnutls_x509_crl_deinit(crls[j]);
	return ret;
}
