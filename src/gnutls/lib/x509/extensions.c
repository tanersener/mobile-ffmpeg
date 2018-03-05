/*
 * Copyright (C) 2003-2014 Free Software Foundation, Inc.
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

/* Functions that relate to the X.509 extension parsing.
 */

#include "gnutls_int.h"
#include "errors.h"
#include <global.h>
#include <libtasn1.h>
#include <common.h>
#include <gnutls/x509-ext.h>
#include <gnutls/x509.h>
#include <x509_int.h>
#include <datum.h>

int
_gnutls_get_extension(ASN1_TYPE asn, const char *root,
	      const char *extension_id, int indx,
	      gnutls_datum_t * ret, unsigned int *_critical)
{
	int k, result, len;
	char name[MAX_NAME_SIZE], name2[MAX_NAME_SIZE];
	char str_critical[10];
	int critical = 0;
	char extnID[MAX_OID_SIZE];
	gnutls_datum_t value;
	int indx_counter = 0;

	ret->data = NULL;
	ret->size = 0;

	k = 0;
	do {
		k++;

		snprintf(name, sizeof(name), "%s.?%u", root, k);

		_gnutls_str_cpy(name2, sizeof(name2), name);
		_gnutls_str_cat(name2, sizeof(name2), ".extnID");

		len = sizeof(extnID) - 1;
		result = asn1_read_value(asn, name2, extnID, &len);

		if (result == ASN1_ELEMENT_NOT_FOUND) {
			gnutls_assert();
			break;
		} else if (result != ASN1_SUCCESS) {
			gnutls_assert();
			return _gnutls_asn2err(result);
		}

		/* Handle Extension 
		 */
		if (strcmp(extnID, extension_id) == 0
		    && indx == indx_counter++) {
			/* extension was found 
			 */

			/* read the critical status.
			 */
			_gnutls_str_cpy(name2, sizeof(name2), name);
			_gnutls_str_cat(name2, sizeof(name2), ".critical");

			len = sizeof(str_critical);
			result =
			    asn1_read_value(asn, name2,
					    str_critical, &len);

			if (result == ASN1_ELEMENT_NOT_FOUND) {
				gnutls_assert();
				break;
			} else if (result != ASN1_SUCCESS) {
				gnutls_assert();
				return _gnutls_asn2err(result);
			}

			if (str_critical[0] == 'T')
				critical = 1;
			else
				critical = 0;

			/* read the value.
			 */
			_gnutls_str_cpy(name2, sizeof(name2), name);
			_gnutls_str_cat(name2, sizeof(name2),
					".extnValue");

			result =
			    _gnutls_x509_read_value(asn, name2, &value);
			if (result < 0) {
				gnutls_assert();
				return result;
			}

			ret->data = value.data;
			ret->size = value.size;

			if (_critical)
				*_critical = critical;

			return 0;
		}
	}
	while (1);

	if (result == ASN1_ELEMENT_NOT_FOUND) {
		return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
	} else {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}
}

static int
get_indx_extension(ASN1_TYPE asn, const char *root,
	      int indx, gnutls_datum_t * out)
{
	char name[MAX_NAME_SIZE];
	int ret;

	out->data = NULL;
	out->size = 0;

	snprintf(name, sizeof(name), "%s.?%u.extnValue", root, indx+1);

	ret = _gnutls_x509_read_value(asn, name, out);
	if (ret < 0)
		return gnutls_assert_val(ret);

	return 0;
}

int
_gnutls_x509_crt_get_extension(gnutls_x509_crt_t cert,
			       const char *extension_id, int indx,
			       gnutls_datum_t * data, unsigned int *critical)
{
	return _gnutls_get_extension(cert->cert, "tbsCertificate.extensions",
			     extension_id, indx, data, critical);
}

/**
 * gnutls_x509_crt_get_extension_data2:
 * @cert: should contain a #gnutls_x509_crt_t type
 * @indx: Specifies which extension OID to read. Use (0) to get the first one.
 * @data: will contain the extension DER-encoded data
 *
 * This function will return the requested by the index extension data in the
 * certificate.  The extension data will be allocated using
 * gnutls_malloc().
 *
 * Use gnutls_x509_crt_get_extension_info() to extract the OID.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned,
 *   otherwise a negative error code is returned.  If you have reached the
 *   last extension available %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE
 *   will be returned.
 **/
int
gnutls_x509_crt_get_extension_data2(gnutls_x509_crt_t cert,
			       unsigned indx,
			       gnutls_datum_t * data)
{
	return get_indx_extension(cert->cert, "tbsCertificate.extensions",
			     indx, data);
}

int
_gnutls_x509_crl_get_extension(gnutls_x509_crl_t crl,
			       const char *extension_id, int indx,
			       gnutls_datum_t * data,
			       unsigned int *critical)
{
	return _gnutls_get_extension(crl->crl, "tbsCertList.crlExtensions",
			     extension_id, indx, data, critical);
}

/**
 * gnutls_x509_crl_get_extension_data2:
 * @crl: should contain a #gnutls_x509_crl_t type
 * @indx: Specifies which extension OID to read. Use (0) to get the first one.
 * @data: will contain the extension DER-encoded data
 *
 * This function will return the requested by the index extension data in the
 * certificate revocation list.  The extension data will be allocated using
 * gnutls_malloc().
 *
 * Use gnutls_x509_crt_get_extension_info() to extract the OID.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned,
 *   otherwise a negative error code is returned.  If you have reached the
 *   last extension available %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE
 *   will be returned.
 **/
int
gnutls_x509_crl_get_extension_data2(gnutls_x509_crl_t crl,
			       unsigned indx,
			       gnutls_datum_t * data)
{
	return get_indx_extension(crl->crl, "tbsCertList.crlExtensions",
			     indx, data);
}

/* This function will attempt to return the requested extension OID found in
 * the given X509v3 certificate. 
 *
 * If you have passed the last extension, GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE will
 * be returned.
 */
static int get_extension_oid(ASN1_TYPE asn, const char *root,
		  unsigned indx, void *oid, size_t * sizeof_oid)
{
	int k, result, len;
	char name[MAX_NAME_SIZE], name2[MAX_NAME_SIZE];
	char extnID[MAX_OID_SIZE];
	unsigned indx_counter = 0;

	k = 0;
	do {
		k++;

		snprintf(name, sizeof(name), "%s.?%u", root, k);

		_gnutls_str_cpy(name2, sizeof(name2), name);
		_gnutls_str_cat(name2, sizeof(name2), ".extnID");

		len = sizeof(extnID) - 1;
		result = asn1_read_value(asn, name2, extnID, &len);

		if (result == ASN1_ELEMENT_NOT_FOUND) {
			gnutls_assert();
			break;
		} else if (result != ASN1_SUCCESS) {
			gnutls_assert();
			return _gnutls_asn2err(result);
		}

		/* Handle Extension 
		 */
		if (indx == indx_counter++) {
			len = strlen(extnID) + 1;

			if (*sizeof_oid < (unsigned) len) {
				*sizeof_oid = len;
				gnutls_assert();
				return GNUTLS_E_SHORT_MEMORY_BUFFER;
			}

			memcpy(oid, extnID, len);
			*sizeof_oid = len - 1;

			return 0;
		}


	}
	while (1);

	if (result == ASN1_ELEMENT_NOT_FOUND) {
		return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
	} else {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}
}

/* This function will attempt to return the requested extension OID found in
 * the given X509v3 certificate. 
 *
 * If you have passed the last extension, GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE will
 * be returned.
 */
int
_gnutls_x509_crt_get_extension_oid(gnutls_x509_crt_t cert,
				   int indx, void *oid,
				   size_t * sizeof_oid)
{
	return get_extension_oid(cert->cert, "tbsCertificate.extensions",
				 indx, oid, sizeof_oid);
}

int
_gnutls_x509_crl_get_extension_oid(gnutls_x509_crl_t crl,
				   int indx, void *oid,
				   size_t * sizeof_oid)
{
	return get_extension_oid(crl->crl, "tbsCertList.crlExtensions",
				 indx, oid, sizeof_oid);
}

/* This function will attempt to set the requested extension in
 * the given X509v3 certificate. 
 *
 * Critical will be either 0 or 1.
 */
static int
add_extension(ASN1_TYPE asn, const char *root, const char *extension_id,
	      const gnutls_datum_t * ext_data, unsigned int critical)
{
	int result;
	const char *str;
	char name[MAX_NAME_SIZE];

	snprintf(name, sizeof(name), "%s", root);

	/* Add a new extension in the list.
	 */
	result = asn1_write_value(asn, name, "NEW", 1);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	if (root[0] != 0)
		snprintf(name, sizeof(name), "%s.?LAST.extnID", root);
	else
		snprintf(name, sizeof(name), "?LAST.extnID");

	result = asn1_write_value(asn, name, extension_id, 1);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	if (critical == 0)
		str = "FALSE";
	else
		str = "TRUE";

	if (root[0] != 0)
		snprintf(name, sizeof(name), "%s.?LAST.critical", root);
	else
		snprintf(name, sizeof(name), "?LAST.critical");

	result = asn1_write_value(asn, name, str, 1);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	if (root[0] != 0)
		snprintf(name, sizeof(name), "%s.?LAST.extnValue", root);
	else
		snprintf(name, sizeof(name), "?LAST.extnValue");

	result = _gnutls_x509_write_value(asn, name, ext_data);
	if (result < 0) {
		gnutls_assert();
		return result;
	}

	return 0;
}

/* Overwrite the given extension (using the index)
 * index here starts from one.
 */
static int
overwrite_extension(ASN1_TYPE asn, const char *root, unsigned int indx,
		    const gnutls_datum_t * ext_data, unsigned int critical)
{
	char name[MAX_NAME_SIZE], name2[MAX_NAME_SIZE];
	const char *str;
	int result;

	if (root[0] != 0)
		snprintf(name, sizeof(name), "%s.?%u", root, indx);
	else
		snprintf(name, sizeof(name), "?%u", indx);

	if (critical == 0)
		str = "FALSE";
	else
		str = "TRUE";

	_gnutls_str_cpy(name2, sizeof(name2), name);
	_gnutls_str_cat(name2, sizeof(name2), ".critical");

	result = asn1_write_value(asn, name2, str, 1);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	_gnutls_str_cpy(name2, sizeof(name2), name);
	_gnutls_str_cat(name2, sizeof(name2), ".extnValue");

	result = _gnutls_x509_write_value(asn, name2, ext_data);
	if (result < 0) {
		gnutls_assert();
		return result;
	}

	return 0;
}

int
_gnutls_set_extension(ASN1_TYPE asn, const char *root,
	      const char *ext_id,
	      const gnutls_datum_t * ext_data, unsigned int critical)
{
	int result = 0;
	int k, len;
	char name[MAX_NAME_SIZE], name2[MAX_NAME_SIZE];
	char extnID[MAX_OID_SIZE];

	/* Find the index of the given extension.
	 */
	k = 0;
	do {
		k++;

		if (root[0] != 0)
			snprintf(name, sizeof(name), "%s.?%u", root, k);
		else
			snprintf(name, sizeof(name), "?%u", k);

		len = sizeof(extnID) - 1;
		result = asn1_read_value(asn, name, extnID, &len);

		/* move to next
		 */

		if (result == ASN1_ELEMENT_NOT_FOUND) {
			break;
		}

		do {

			_gnutls_str_cpy(name2, sizeof(name2), name);
			_gnutls_str_cat(name2, sizeof(name2), ".extnID");

			len = sizeof(extnID) - 1;
			result = asn1_read_value(asn, name2, extnID, &len);

			if (result == ASN1_ELEMENT_NOT_FOUND) {
				gnutls_assert();
				break;
			} else if (result != ASN1_SUCCESS) {
				gnutls_assert();
				return _gnutls_asn2err(result);
			}

			/* Handle Extension 
			 */
			if (strcmp(extnID, ext_id) == 0) {
				/* extension was found 
				 */
				return overwrite_extension(asn, root, k,
							   ext_data,
							   critical);
			}


		}
		while (0);
	}
	while (1);

	if (result == ASN1_ELEMENT_NOT_FOUND) {
		return add_extension(asn, root, ext_id, ext_data,
				     critical);
	} else {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}


	return 0;
}

/* This function will attempt to overwrite the requested extension with
 * the given one. 
 *
 * Critical will be either 0 or 1.
 */
int
_gnutls_x509_crt_set_extension(gnutls_x509_crt_t cert,
			       const char *ext_id,
			       const gnutls_datum_t * ext_data,
			       unsigned int critical)
{
	MODIFIED(cert);
	cert->use_extensions = 1;

	return _gnutls_set_extension(cert->cert, "tbsCertificate.extensions",
			     ext_id, ext_data, critical);
}

int
_gnutls_x509_crl_set_extension(gnutls_x509_crl_t crl,
			       const char *ext_id,
			       const gnutls_datum_t * ext_data,
			       unsigned int critical)
{
	return _gnutls_set_extension(crl->crl, "tbsCertList.crlExtensions", ext_id,
			     ext_data, critical);
}

int
_gnutls_x509_crq_set_extension(gnutls_x509_crq_t crq,
			       const char *ext_id,
			       const gnutls_datum_t * ext_data,
			       unsigned int critical)
{
	unsigned char *extensions = NULL;
	size_t extensions_size = 0;
	gnutls_datum_t der;
	ASN1_TYPE c2;
	int result;

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
		if (result == GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE) {
			extensions_size = 0;
		} else {
			gnutls_assert();
			gnutls_free(extensions);
			return result;
		}
	}

	result =
	    asn1_create_element(_gnutls_get_pkix(), "PKIX1.Extensions",
				&c2);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		gnutls_free(extensions);
		return _gnutls_asn2err(result);
	}

	if (extensions_size > 0) {
		result =
		    _asn1_strict_der_decode(&c2, extensions, extensions_size,
				      NULL);
		gnutls_free(extensions);
		if (result != ASN1_SUCCESS) {
			gnutls_assert();
			asn1_delete_structure(&c2);
			return _gnutls_asn2err(result);
		}
	}

	result = _gnutls_set_extension(c2, "", ext_id, ext_data, critical);
	if (result < 0) {
		gnutls_assert();
		asn1_delete_structure(&c2);
		return result;
	}

	result = _gnutls_x509_der_encode(c2, "", &der, 0);

	asn1_delete_structure(&c2);

	if (result < 0) {
		gnutls_assert();
		return result;
	}

	result =
	    gnutls_x509_crq_set_attribute_by_oid(crq,
						 "1.2.840.113549.1.9.14",
						 der.data, der.size);
	gnutls_free(der.data);
	if (result < 0) {
		gnutls_assert();
		return result;
	}


	return 0;
}

/* extract an INTEGER from the DER encoded extension
 */
int
_gnutls_x509_ext_extract_number(uint8_t * number,
				size_t * _nr_size,
				uint8_t * extnValue, int extnValueLen)
{
	ASN1_TYPE ext = ASN1_TYPE_EMPTY;
	int result;
	int nr_size = *_nr_size;

	/* here it doesn't matter so much that we use CertificateSerialNumber. It is equal
	 * to using INTEGER.
	 */
	if ((result = asn1_create_element
	     (_gnutls_get_pkix(), "PKIX1.CertificateSerialNumber",
	      &ext)) != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	result = _asn1_strict_der_decode(&ext, extnValue, extnValueLen, NULL);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		asn1_delete_structure(&ext);
		return _gnutls_asn2err(result);
	}

	/* the default value of cA is false.
	 */
	result = asn1_read_value(ext, "", number, &nr_size);
	if (result != ASN1_SUCCESS)
		result = _gnutls_asn2err(result);
	else
		result = 0;

	*_nr_size = nr_size;

	asn1_delete_structure(&ext);

	return result;
}

/* generate an INTEGER in a DER encoded extension
 */
int
_gnutls_x509_ext_gen_number(const uint8_t * number, size_t nr_size,
			    gnutls_datum_t * der_ext)
{
	ASN1_TYPE ext = ASN1_TYPE_EMPTY;
	int result;

	result =
	    asn1_create_element(_gnutls_get_pkix(),
				"PKIX1.CertificateSerialNumber", &ext);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	result = asn1_write_value(ext, "", number, nr_size);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		asn1_delete_structure(&ext);
		return _gnutls_asn2err(result);
	}

	result = _gnutls_x509_der_encode(ext, "", der_ext, 0);

	asn1_delete_structure(&ext);

	if (result < 0) {
		gnutls_assert();
		return result;
	}

	return 0;
}

int
_gnutls_write_general_name(ASN1_TYPE ext, const char *ext_name,
		       gnutls_x509_subject_alt_name_t type,
		       const void *data, unsigned int data_size)
{
	const char *str;
	int result;
	char name[128];

	if (data == NULL) {
		if (data_size == 0)
			data = (void*)"";
		else
			return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);
	}

	switch (type) {
	case GNUTLS_SAN_DNSNAME:
		str = "dNSName";
		break;
	case GNUTLS_SAN_RFC822NAME:
		str = "rfc822Name";
		break;
	case GNUTLS_SAN_URI:
		str = "uniformResourceIdentifier";
		break;
	case GNUTLS_SAN_IPADDRESS:
		str = "iPAddress";
		break;
	default:
		gnutls_assert();
		return GNUTLS_E_INTERNAL_ERROR;
	}

	result = asn1_write_value(ext, ext_name, str, 1);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	snprintf(name, sizeof(name), "%s.%s", ext_name, str);

	result = asn1_write_value(ext, name, data, data_size);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		asn1_delete_structure(&ext);
		return _gnutls_asn2err(result);
	}

	return 0;
}

int
_gnutls_write_new_general_name(ASN1_TYPE ext, const char *ext_name,
		       gnutls_x509_subject_alt_name_t type,
		       const void *data, unsigned int data_size)
{
	int result;
	char name[128];

	result = asn1_write_value(ext, ext_name, "NEW", 1);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	if (ext_name[0] == 0) {	/* no dot */
		_gnutls_str_cpy(name, sizeof(name), "?LAST");
	} else {
		_gnutls_str_cpy(name, sizeof(name), ext_name);
		_gnutls_str_cat(name, sizeof(name), ".?LAST");
	}

	result = _gnutls_write_general_name(ext, name, type,
		data, data_size);
	if (result < 0) {
		gnutls_assert();
		return result;
	}

	return 0;
}

int
_gnutls_write_new_othername(ASN1_TYPE ext, const char *ext_name,
		       const char *oid,
		       const void *data, unsigned int data_size)
{
	int result;
	char name[128];
	char name2[128];

	result = asn1_write_value(ext, ext_name, "NEW", 1);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	if (ext_name[0] == 0) {	/* no dot */
		_gnutls_str_cpy(name, sizeof(name), "?LAST");
	} else {
		_gnutls_str_cpy(name, sizeof(name), ext_name);
		_gnutls_str_cat(name, sizeof(name), ".?LAST");
	}

	result = asn1_write_value(ext, name, "otherName", 1);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	snprintf(name2, sizeof(name2), "%s.otherName.type-id", name);

	result = asn1_write_value(ext, name2, oid, 1);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		asn1_delete_structure(&ext);
		return _gnutls_asn2err(result);
	}

	snprintf(name2, sizeof(name2), "%s.otherName.value", name);

	result = asn1_write_value(ext, name2, data, data_size);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		asn1_delete_structure(&ext);
		return _gnutls_asn2err(result);
	}

	return 0;
}

/* Convert the given name to GeneralNames in a DER encoded extension.
 * This is the same as subject alternative name.
 */
int
_gnutls_x509_ext_gen_subject_alt_name(gnutls_x509_subject_alt_name_t
				      type,
				      const char *othername_oid,
				      const void *data,
				      unsigned int data_size,
				      const gnutls_datum_t * prev_der_ext,
				      gnutls_datum_t * der_ext)
{
	int ret;
	gnutls_subject_alt_names_t sans = NULL;
	gnutls_datum_t name;

	ret = gnutls_subject_alt_names_init(&sans);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	if (prev_der_ext && prev_der_ext->data != NULL && 
		prev_der_ext->size != 0) {

		ret = gnutls_x509_ext_import_subject_alt_names(prev_der_ext, sans, 0);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}
	}

	name.data = (void*)data;
	name.size = data_size;
	ret = gnutls_subject_alt_names_set(sans, type, &name, othername_oid);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = gnutls_x509_ext_export_subject_alt_names(sans, der_ext);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = 0;
cleanup:
	if (sans != NULL)
		gnutls_subject_alt_names_deinit(sans);

	return ret;
}

/* generate the AuthorityKeyID in a DER encoded extension
 */
int
_gnutls_x509_ext_gen_auth_key_id(const void *id, size_t id_size,
				 gnutls_datum_t * der_ext)
{
	gnutls_x509_aki_t aki;
	int ret;
	gnutls_datum_t l_id;

	ret = gnutls_x509_aki_init(&aki);
	if (ret < 0)
		return gnutls_assert_val(ret);

	l_id.data = (void*)id;
	l_id.size = id_size;
	ret = gnutls_x509_aki_set_id(aki, &l_id);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = gnutls_x509_ext_export_authority_key_id(aki, der_ext);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = 0;

 cleanup:
	gnutls_x509_aki_deinit(aki);
	return ret;
}
