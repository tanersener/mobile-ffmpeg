/*
 * Copyright (C) 2003-2012 Free Software Foundation, Inc.
 * Copyright (C) 2012 Nikos Mavrogiannopoulos
 * Copyright (C) 2017 Red Hat, Inc.
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

/* Functions that relate on PKCS12 packet parsing.
 */

#include "gnutls_int.h"
#include <libtasn1.h>

#include <datum.h>
#include <global.h>
#include "errors.h"
#include <num.h>
#include <common.h>
#include <x509_b64.h>
#include "x509_int.h"
#include "pkcs7_int.h"
#include <random.h>


/* Decodes the PKCS #12 auth_safe, and returns the allocated raw data,
 * which holds them. Returns an ASN1_TYPE of authenticatedSafe.
 */
static int
_decode_pkcs12_auth_safe(ASN1_TYPE pkcs12, ASN1_TYPE * authen_safe,
			 gnutls_datum_t * raw)
{
	char oid[MAX_OID_SIZE];
	ASN1_TYPE c2 = ASN1_TYPE_EMPTY;
	gnutls_datum_t auth_safe = { NULL, 0 };
	int len, result;
	char error_str[ASN1_MAX_ERROR_DESCRIPTION_SIZE];

	len = sizeof(oid) - 1;
	result =
	    asn1_read_value(pkcs12, "authSafe.contentType", oid, &len);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	if (strcmp(oid, DATA_OID) != 0) {
		gnutls_assert();
		_gnutls_debug_log("Unknown PKCS12 Content OID '%s'\n",
				  oid);
		return GNUTLS_E_UNKNOWN_PKCS_CONTENT_TYPE;
	}

	/* Step 1. Read the content data
	 */

	result =
	    _gnutls_x509_read_string(pkcs12, "authSafe.content",
				     &auth_safe, ASN1_ETYPE_OCTET_STRING, 1);
	if (result < 0) {
		gnutls_assert();
		goto cleanup;
	}

	/* Step 2. Extract the authenticatedSafe.
	 */

	if ((result = asn1_create_element
	     (_gnutls_get_pkix(), "PKIX1.pkcs-12-AuthenticatedSafe",
	      &c2)) != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto cleanup;
	}

	result =
	    asn1_der_decoding(&c2, auth_safe.data, auth_safe.size,
			      error_str);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		_gnutls_debug_log("DER error: %s\n", error_str);
		result = _gnutls_asn2err(result);
		goto cleanup;
	}

	if (raw == NULL) {
		_gnutls_free_datum(&auth_safe);
	} else {
		raw->data = auth_safe.data;
		raw->size = auth_safe.size;
	}

	if (authen_safe)
		*authen_safe = c2;
	else
		asn1_delete_structure(&c2);

	return 0;

      cleanup:
	if (c2)
		asn1_delete_structure(&c2);
	_gnutls_free_datum(&auth_safe);
	return result;
}

static int pkcs12_reinit(gnutls_pkcs12_t pkcs12)
{
int result;

	if (pkcs12->pkcs12)
		asn1_delete_structure(&pkcs12->pkcs12);

	result = asn1_create_element(_gnutls_get_pkix(),
					 "PKIX1.pkcs-12-PFX",
					 &pkcs12->pkcs12);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	return 0;
}

/**
 * gnutls_pkcs12_init:
 * @pkcs12: A pointer to the type to be initialized
 *
 * This function will initialize a PKCS12 type. PKCS12 structures
 * usually contain lists of X.509 Certificates and X.509 Certificate
 * revocation lists.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int gnutls_pkcs12_init(gnutls_pkcs12_t * pkcs12)
{
	*pkcs12 = gnutls_calloc(1, sizeof(gnutls_pkcs12_int));

	if (*pkcs12) {
		int result = pkcs12_reinit(*pkcs12);
		if (result < 0) {
			gnutls_assert();
			gnutls_free(*pkcs12);
			return result;
		}
		return 0;	/* success */
	}
	return GNUTLS_E_MEMORY_ERROR;
}

/**
 * gnutls_pkcs12_deinit:
 * @pkcs12: The type to be initialized
 *
 * This function will deinitialize a PKCS12 type.
 **/
void gnutls_pkcs12_deinit(gnutls_pkcs12_t pkcs12)
{
	if (!pkcs12)
		return;

	if (pkcs12->pkcs12)
		asn1_delete_structure(&pkcs12->pkcs12);

	gnutls_free(pkcs12);
}

/**
 * gnutls_pkcs12_import:
 * @pkcs12: The data to store the parsed PKCS12.
 * @data: The DER or PEM encoded PKCS12.
 * @format: One of DER or PEM
 * @flags: an ORed sequence of gnutls_privkey_pkcs8_flags
 *
 * This function will convert the given DER or PEM encoded PKCS12
 * to the native gnutls_pkcs12_t format. The output will be stored in 'pkcs12'.
 *
 * If the PKCS12 is PEM encoded it should have a header of "PKCS12".
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_pkcs12_import(gnutls_pkcs12_t pkcs12,
		     const gnutls_datum_t * data,
		     gnutls_x509_crt_fmt_t format, unsigned int flags)
{
	int result = 0, need_free = 0;
	gnutls_datum_t _data;
	char error_str[ASN1_MAX_ERROR_DESCRIPTION_SIZE];

	_data.data = data->data;
	_data.size = data->size;

	if (pkcs12 == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	/* If the PKCS12 is in PEM format then decode it
	 */
	if (format == GNUTLS_X509_FMT_PEM) {
		result =
		    _gnutls_fbase64_decode(PEM_PKCS12, data->data,
					   data->size, &_data);

		if (result < 0) {
			gnutls_assert();
			return result;
		}

		need_free = 1;
	}

	if (pkcs12->expanded) {
		result = pkcs12_reinit(pkcs12);
		if (result < 0) {
			gnutls_assert();
			goto cleanup;
		}
	}
	pkcs12->expanded = 1;

	result =
	    asn1_der_decoding(&pkcs12->pkcs12, _data.data, _data.size,
			      error_str);
	if (result != ASN1_SUCCESS) {
		result = _gnutls_asn2err(result);
		_gnutls_debug_log("DER error: %s\n", error_str);
		gnutls_assert();
		goto cleanup;
	}

	if (need_free)
		_gnutls_free_datum(&_data);

	return 0;

      cleanup:
	if (need_free)
		_gnutls_free_datum(&_data);
	return result;
}


/**
 * gnutls_pkcs12_export:
 * @pkcs12: A pkcs12 type
 * @format: the format of output params. One of PEM or DER.
 * @output_data: will contain a structure PEM or DER encoded
 * @output_data_size: holds the size of output_data (and will be
 *   replaced by the actual size of parameters)
 *
 * This function will export the pkcs12 structure to DER or PEM format.
 *
 * If the buffer provided is not long enough to hold the output, then
 * *output_data_size will be updated and GNUTLS_E_SHORT_MEMORY_BUFFER
 * will be returned.
 *
 * If the structure is PEM encoded, it will have a header
 * of "BEGIN PKCS12".
 *
 * Returns: In case of failure a negative error code will be
 *   returned, and 0 on success.
 **/
int
gnutls_pkcs12_export(gnutls_pkcs12_t pkcs12,
		     gnutls_x509_crt_fmt_t format, void *output_data,
		     size_t * output_data_size)
{
	if (pkcs12 == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	return _gnutls_x509_export_int(pkcs12->pkcs12, format, PEM_PKCS12,
				       output_data, output_data_size);
}

/**
 * gnutls_pkcs12_export2:
 * @pkcs12: A pkcs12 type
 * @format: the format of output params. One of PEM or DER.
 * @out: will contain a structure PEM or DER encoded
 *
 * This function will export the pkcs12 structure to DER or PEM format.
 *
 * The output buffer is allocated using gnutls_malloc().
 *
 * If the structure is PEM encoded, it will have a header
 * of "BEGIN PKCS12".
 *
 * Returns: In case of failure a negative error code will be
 *   returned, and 0 on success.
 *
 * Since: 3.1.3
 **/
int
gnutls_pkcs12_export2(gnutls_pkcs12_t pkcs12,
		      gnutls_x509_crt_fmt_t format, gnutls_datum_t * out)
{
	if (pkcs12 == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	return _gnutls_x509_export_int2(pkcs12->pkcs12, format, PEM_PKCS12,
					out);
}

static int oid2bag(const char *oid)
{
	if (strcmp(oid, BAG_PKCS8_KEY) == 0)
		return GNUTLS_BAG_PKCS8_KEY;
	if (strcmp(oid, BAG_PKCS8_ENCRYPTED_KEY) == 0)
		return GNUTLS_BAG_PKCS8_ENCRYPTED_KEY;
	if (strcmp(oid, BAG_CERTIFICATE) == 0)
		return GNUTLS_BAG_CERTIFICATE;
	if (strcmp(oid, BAG_CRL) == 0)
		return GNUTLS_BAG_CRL;
	if (strcmp(oid, BAG_SECRET) == 0)
		return GNUTLS_BAG_SECRET;

	return GNUTLS_BAG_UNKNOWN;
}

static const char *bag_to_oid(int bag)
{
	switch (bag) {
	case GNUTLS_BAG_PKCS8_KEY:
		return BAG_PKCS8_KEY;
	case GNUTLS_BAG_PKCS8_ENCRYPTED_KEY:
		return BAG_PKCS8_ENCRYPTED_KEY;
	case GNUTLS_BAG_CERTIFICATE:
		return BAG_CERTIFICATE;
	case GNUTLS_BAG_CRL:
		return BAG_CRL;
	case GNUTLS_BAG_SECRET:
		return BAG_SECRET;
	}
	return NULL;
}

/* Decodes the SafeContents, and puts the output in
 * the given bag. 
 */
int
_pkcs12_decode_safe_contents(const gnutls_datum_t * content,
			     gnutls_pkcs12_bag_t bag)
{
	char oid[MAX_OID_SIZE], root[MAX_NAME_SIZE];
	ASN1_TYPE c2 = ASN1_TYPE_EMPTY;
	int len, result;
	int bag_type;
	gnutls_datum_t attr_val;
	gnutls_datum_t t;
	int count = 0, attributes, j;
	unsigned i;

	/* Step 1. Extract the SEQUENCE.
	 */

	if ((result = asn1_create_element
	     (_gnutls_get_pkix(), "PKIX1.pkcs-12-SafeContents",
	      &c2)) != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto cleanup;
	}

	result =
	    asn1_der_decoding(&c2, content->data, content->size, NULL);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto cleanup;
	}

	/* Count the number of bags
	 */
	result = asn1_number_of_elements(c2, "", &count);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto cleanup;
	}

	bag->bag_elements = MIN(MAX_BAG_ELEMENTS, count);

	for (i = 0; i < bag->bag_elements; i++) {

		snprintf(root, sizeof(root), "?%u.bagId", i + 1);

		len = sizeof(oid);
		result = asn1_read_value(c2, root, oid, &len);
		if (result != ASN1_SUCCESS) {
			gnutls_assert();
			result = _gnutls_asn2err(result);
			goto cleanup;
		}

		/* Read the Bag type
		 */
		bag_type = oid2bag(oid);

		if (bag_type < 0) {
			gnutls_assert();
			goto cleanup;
		}

		/* Read the Bag Value
		 */

		snprintf(root, sizeof(root), "?%u.bagValue", i + 1);

		result =
		    _gnutls_x509_read_value(c2, root,
					    &bag->element[i].data);
		if (result < 0) {
			gnutls_assert();
			goto cleanup;
		}

		if (bag_type == GNUTLS_BAG_CERTIFICATE
		    || bag_type == GNUTLS_BAG_CRL
		    || bag_type == GNUTLS_BAG_SECRET) {
			gnutls_datum_t tmp = bag->element[i].data;
			bag->element[i].data.data = NULL;
			bag->element[i].data.size = 0;

			result =
			    _pkcs12_decode_crt_bag(bag_type, &tmp,
						   &bag->element[i].data);
			_gnutls_free_datum(&tmp);
			if (result < 0) {
				gnutls_assert();
				goto cleanup;
			}
		}

		/* read the bag attributes
		 */
		snprintf(root, sizeof(root), "?%u.bagAttributes", i + 1);

		result = asn1_number_of_elements(c2, root, &attributes);
		if (result != ASN1_SUCCESS
		    && result != ASN1_ELEMENT_NOT_FOUND) {
			gnutls_assert();
			result = _gnutls_asn2err(result);
			goto cleanup;
		}

		if (attributes < 0)
			attributes = 1;

		if (result != ASN1_ELEMENT_NOT_FOUND)
			for (j = 0; j < attributes; j++) {

				snprintf(root, sizeof(root),
					 "?%u.bagAttributes.?%u", i + 1,
					 j + 1);

				result =
				    _gnutls_x509_decode_and_read_attribute
				    (c2, root, oid, sizeof(oid), &attr_val,
				     1, 0);

				if (result < 0) {
					gnutls_assert();
					continue;	/* continue in case we find some known attributes */
				}

				if (strcmp(oid, KEY_ID_OID) == 0) {
					result =
					    _gnutls_x509_decode_string
					    (ASN1_ETYPE_OCTET_STRING,
					     attr_val.data, attr_val.size,
					     &t, 1);

					_gnutls_free_datum(&attr_val);
					if (result < 0) {
						gnutls_assert();
						_gnutls_debug_log
						    ("Error decoding PKCS12 Bag Attribute OID '%s'\n",
						     oid);
						continue;
					}

					_gnutls_free_datum(&bag->element[i].local_key_id);
					bag->element[i].local_key_id.data = t.data;
					bag->element[i].local_key_id.size = t.size;
				} else if (strcmp(oid, FRIENDLY_NAME_OID) == 0 && bag->element[i].friendly_name == NULL) {
					result =
					    _gnutls_x509_decode_string
					    (ASN1_ETYPE_BMP_STRING,
					     attr_val.data, attr_val.size,
					     &t, 1);

					_gnutls_free_datum(&attr_val);
					if (result < 0) {
						gnutls_assert();
						_gnutls_debug_log
						    ("Error decoding PKCS12 Bag Attribute OID '%s'\n",
						     oid);
						continue;
					}

					gnutls_free(bag->element[i].friendly_name);
					bag->element[i].friendly_name = (char *) t.data;
				} else {
					_gnutls_free_datum(&attr_val);
					_gnutls_debug_log
					    ("Unknown PKCS12 Bag Attribute OID '%s'\n",
					     oid);
				}
			}


		bag->element[i].type = bag_type;

	}

	result = 0;

      cleanup:
	if (c2)
		asn1_delete_structure(&c2);
	return result;

}


static int
_parse_safe_contents(ASN1_TYPE sc, const char *sc_name,
		     gnutls_pkcs12_bag_t bag)
{
	gnutls_datum_t content = { NULL, 0 };
	int result;

	/* Step 1. Extract the content.
	 */

	result =
	    _gnutls_x509_read_string(sc, sc_name, &content,
				     ASN1_ETYPE_OCTET_STRING, 1);
	if (result < 0) {
		gnutls_assert();
		goto cleanup;
	}

	result = _pkcs12_decode_safe_contents(&content, bag);
	if (result < 0) {
		gnutls_assert();
		goto cleanup;
	}

	_gnutls_free_datum(&content);

	return 0;

      cleanup:
	_gnutls_free_datum(&content);
	return result;
}


/**
 * gnutls_pkcs12_get_bag:
 * @pkcs12: A pkcs12 type
 * @indx: contains the index of the bag to extract
 * @bag: An initialized bag, where the contents of the bag will be copied
 *
 * This function will return a Bag from the PKCS12 structure.
 *
 * After the last Bag has been read
 * %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE will be returned.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_pkcs12_get_bag(gnutls_pkcs12_t pkcs12,
		      int indx, gnutls_pkcs12_bag_t bag)
{
	ASN1_TYPE c2 = ASN1_TYPE_EMPTY;
	int result, len;
	char root2[MAX_NAME_SIZE];
	char oid[MAX_OID_SIZE];

	if (pkcs12 == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	/* Step 1. decode the data.
	 */
	result = _decode_pkcs12_auth_safe(pkcs12->pkcs12, &c2, NULL);
	if (result < 0) {
		gnutls_assert();
		return result;
	}

	/* Step 2. Parse the AuthenticatedSafe
	 */

	snprintf(root2, sizeof(root2), "?%u.contentType", indx + 1);

	len = sizeof(oid) - 1;
	result = asn1_read_value(c2, root2, oid, &len);

	if (result == ASN1_ELEMENT_NOT_FOUND) {
		result = GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
		goto cleanup;
	}

	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto cleanup;
	}

	/* Not encrypted Bag
	 */

	snprintf(root2, sizeof(root2), "?%u.content", indx + 1);

	if (strcmp(oid, DATA_OID) == 0) {
		result = _parse_safe_contents(c2, root2, bag);
		goto cleanup;
	}

	/* ENC_DATA_OID needs decryption */

	result = _gnutls_x509_read_value(c2, root2, &bag->element[0].data);
	if (result < 0) {
		gnutls_assert();
		goto cleanup;
	}

	bag->element[0].type = GNUTLS_BAG_ENCRYPTED;
	bag->bag_elements = 1;

	result = 0;

      cleanup:
	if (c2)
		asn1_delete_structure(&c2);
	return result;
}

/* Creates an empty PFX structure for the PKCS12 structure.
 */
static int create_empty_pfx(ASN1_TYPE pkcs12)
{
	uint8_t three = 3;
	int result;
	ASN1_TYPE c2 = ASN1_TYPE_EMPTY;

	/* Use version 3
	 */
	result = asn1_write_value(pkcs12, "version", &three, 1);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto cleanup;
	}

	/* Write the content type of the data
	 */
	result =
	    asn1_write_value(pkcs12, "authSafe.contentType", DATA_OID, 1);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto cleanup;
	}

	/* Check if the authenticatedSafe content is empty, and encode a
	 * null one in that case.
	 */

	if ((result = asn1_create_element
	     (_gnutls_get_pkix(), "PKIX1.pkcs-12-AuthenticatedSafe",
	      &c2)) != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto cleanup;
	}

	result =
	    _gnutls_x509_der_encode_and_copy(c2, "", pkcs12,
					     "authSafe.content", 1);
	if (result < 0) {
		gnutls_assert();
		goto cleanup;
	}
	asn1_delete_structure(&c2);

	return 0;

      cleanup:
	asn1_delete_structure(&c2);
	return result;

}

/**
 * gnutls_pkcs12_set_bag:
 * @pkcs12: should contain a gnutls_pkcs12_t type
 * @bag: An initialized bag
 *
 * This function will insert a Bag into the PKCS12 structure.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int gnutls_pkcs12_set_bag(gnutls_pkcs12_t pkcs12, gnutls_pkcs12_bag_t bag)
{
	ASN1_TYPE c2 = ASN1_TYPE_EMPTY;
	ASN1_TYPE safe_cont = ASN1_TYPE_EMPTY;
	int result;
	int enc = 0, dum = 1;
	char null;

	if (pkcs12 == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	/* Step 1. Check if the pkcs12 structure is empty. In that
	 * case generate an empty PFX.
	 */
	result =
	    asn1_read_value(pkcs12->pkcs12, "authSafe.content", &null,
			    &dum);
	if (result == ASN1_VALUE_NOT_FOUND) {
		result = create_empty_pfx(pkcs12->pkcs12);
		if (result < 0) {
			gnutls_assert();
			return result;
		}
	}

	/* Step 2. decode the authenticatedSafe.
	 */
	result = _decode_pkcs12_auth_safe(pkcs12->pkcs12, &c2, NULL);
	if (result < 0) {
		gnutls_assert();
		return result;
	}

	/* Step 3. Encode the bag elements into a SafeContents 
	 * structure.
	 */
	result = _pkcs12_encode_safe_contents(bag, &safe_cont, &enc);
	if (result < 0) {
		gnutls_assert();
		return result;
	}

	/* Step 4. Insert the encoded SafeContents into the AuthenticatedSafe
	 * structure.
	 */
	result = asn1_write_value(c2, "", "NEW", 1);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto cleanup;
	}

	if (enc)
		result =
		    asn1_write_value(c2, "?LAST.contentType", ENC_DATA_OID,
				     1);
	else
		result =
		    asn1_write_value(c2, "?LAST.contentType", DATA_OID, 1);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto cleanup;
	}

	if (enc) {
		/* Encrypted packets are written directly.
		 */
		result =
		    asn1_write_value(c2, "?LAST.content",
				     bag->element[0].data.data,
				     bag->element[0].data.size);
		if (result != ASN1_SUCCESS) {
			gnutls_assert();
			result = _gnutls_asn2err(result);
			goto cleanup;
		}
	} else {
		result =
		    _gnutls_x509_der_encode_and_copy(safe_cont, "", c2,
						     "?LAST.content", 1);
		if (result < 0) {
			gnutls_assert();
			goto cleanup;
		}
	}

	asn1_delete_structure(&safe_cont);


	/* Step 5. Re-encode and copy the AuthenticatedSafe into the pkcs12
	 * structure.
	 */
	result =
	    _gnutls_x509_der_encode_and_copy(c2, "", pkcs12->pkcs12,
					     "authSafe.content", 1);
	if (result < 0) {
		gnutls_assert();
		goto cleanup;
	}

	asn1_delete_structure(&c2);

	return 0;

      cleanup:
	asn1_delete_structure(&c2);
	asn1_delete_structure(&safe_cont);
	return result;
}

#if ENABLE_GOST
/*
 * Russian differs from PKCS#12 here. It described proprietary way
 * to obtain MAC key instead of using standard mechanism.
 *
 * See https://wwwold.tc26.ru/standard/rs/%D0%A0%2050.1.112-2016.pdf
 * section 5.
 */
static int
_gnutls_pkcs12_gost_string_to_key(gnutls_mac_algorithm_t algo,
				  const uint8_t * salt,
				  unsigned int salt_size, unsigned int iter,
				  const char *pass, unsigned int req_keylen,
				  uint8_t * keybuf)
{
	uint8_t temp[96];
	size_t temp_len = sizeof(temp);
	gnutls_datum_t key;
	gnutls_datum_t _salt;
	int ret;

	if (iter == 0)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	key.data = (void *)pass;
	key.size = pass ? strlen(pass) : 0;

	_salt.data = (void *)salt;
	_salt.size = salt_size;

	ret = gnutls_pbkdf2(algo, &key, &_salt, iter, temp, temp_len);
	if (ret < 0)
		return gnutls_assert_val(ret);

	memcpy(keybuf, temp + temp_len - req_keylen, req_keylen);

	return 0;
}
#endif

/**
 * gnutls_pkcs12_generate_mac2:
 * @pkcs12: A pkcs12 type
 * @mac: the MAC algorithm to use
 * @pass: The password for the MAC
 *
 * This function will generate a MAC for the PKCS12 structure.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int gnutls_pkcs12_generate_mac2(gnutls_pkcs12_t pkcs12, gnutls_mac_algorithm_t mac, const char *pass)
{
	uint8_t salt[8], key[MAX_HASH_SIZE];
	int result;
	const int iter = 10*1024;
	mac_hd_st td1;
	gnutls_datum_t tmp = { NULL, 0 };
	unsigned mac_size, key_len;
	uint8_t mac_out[MAX_HASH_SIZE];
	const mac_entry_st *me = mac_to_entry(mac);

	if (pkcs12 == NULL || me == NULL)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	if (me->oid == NULL)
		return gnutls_assert_val(GNUTLS_E_UNIMPLEMENTED_FEATURE);

	mac_size = _gnutls_mac_get_algo_len(me);
	key_len = mac_size;

	/* Generate the salt.
	 */
	result = gnutls_rnd(GNUTLS_RND_NONCE, salt, sizeof(salt));
	if (result < 0) {
		gnutls_assert();
		return result;
	}

	/* Write the salt into the structure.
	 */
	result =
	    asn1_write_value(pkcs12->pkcs12, "macData.macSalt", salt,
			     sizeof(salt));
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto cleanup;
	}

	/* write the iterations
	 */

	if (iter > 1) {
		result =
		    _gnutls_x509_write_uint32(pkcs12->pkcs12,
					      "macData.iterations", iter);
		if (result < 0) {
			gnutls_assert();
			goto cleanup;
		}
	}

	/* Generate the key.
	 */
#if ENABLE_GOST
	if (me->id == GNUTLS_MAC_GOSTR_94 ||
	    me->id == GNUTLS_MAC_STREEBOG_256 ||
	    me->id == GNUTLS_MAC_STREEBOG_512) {
		key_len = 32;
		result = _gnutls_pkcs12_gost_string_to_key(me->id,
							   salt,
							   sizeof(salt),
							   iter,
							   pass,
							   key_len,
							   key);
	} else
#endif
		result = _gnutls_pkcs12_string_to_key(me, 3 /*MAC*/,
						      salt, sizeof(salt),
						      iter, pass,
						      mac_size, key);
	if (result < 0) {
		gnutls_assert();
		goto cleanup;
	}

	/* Get the data to be MACed
	 */
	result = _decode_pkcs12_auth_safe(pkcs12->pkcs12, NULL, &tmp);
	if (result < 0) {
		gnutls_assert();
		goto cleanup;
	}

	/* MAC the data
	 */
	result = _gnutls_mac_init(&td1, me,
				  key, key_len);
	if (result < 0) {
		gnutls_assert();
		goto cleanup;
	}

	_gnutls_mac(&td1, tmp.data, tmp.size);
	_gnutls_free_datum(&tmp);

	_gnutls_mac_deinit(&td1, mac_out);


	result =
	    asn1_write_value(pkcs12->pkcs12, "macData.mac.digest", mac_out,
			     mac_size);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto cleanup;
	}

	result =
	    asn1_write_value(pkcs12->pkcs12,
			     "macData.mac.digestAlgorithm.parameters",
			     NULL, 0);
	if (result != ASN1_SUCCESS && result != ASN1_ELEMENT_NOT_FOUND) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto cleanup;
	}

	result =
	    asn1_write_value(pkcs12->pkcs12,
			     "macData.mac.digestAlgorithm.algorithm",
			     me->oid, 1);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto cleanup;
	}

	return 0;

      cleanup:
	_gnutls_free_datum(&tmp);
	return result;
}

/**
 * gnutls_pkcs12_generate_mac:
 * @pkcs12: A pkcs12 type
 * @pass: The password for the MAC
 *
 * This function will generate a MAC for the PKCS12 structure.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int gnutls_pkcs12_generate_mac(gnutls_pkcs12_t pkcs12, const char *pass)
{
	return gnutls_pkcs12_generate_mac2(pkcs12, GNUTLS_MAC_SHA1, pass);
}

/**
 * gnutls_pkcs12_verify_mac:
 * @pkcs12: should contain a gnutls_pkcs12_t type
 * @pass: The password for the MAC
 *
 * This function will verify the MAC for the PKCS12 structure.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int gnutls_pkcs12_verify_mac(gnutls_pkcs12_t pkcs12, const char *pass)
{
	uint8_t key[MAX_HASH_SIZE];
	char oid[MAX_OID_SIZE];
	int result;
	unsigned int iter;
	int len;
	mac_hd_st td1;
	gnutls_datum_t tmp = { NULL, 0 }, salt = {
	NULL, 0};
	uint8_t mac_output[MAX_HASH_SIZE];
	uint8_t mac_output_orig[MAX_HASH_SIZE];
	gnutls_mac_algorithm_t algo;
	unsigned mac_len, key_len;
	const mac_entry_st *entry;
#if ENABLE_GOST
	int gost_retry = 0;
#endif

	if (pkcs12 == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	/* read the iterations
	 */
	result =
	    _gnutls_x509_read_uint(pkcs12->pkcs12, "macData.iterations",
				   &iter);
	if (result < 0) {
		iter = 1;	/* the default */
	}

	len = sizeof(oid);
	result =
	    asn1_read_value(pkcs12->pkcs12, "macData.mac.digestAlgorithm.algorithm",
			    oid, &len);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	algo = gnutls_oid_to_digest(oid);
	if (algo == GNUTLS_MAC_UNKNOWN) {
 unknown_mac:
		gnutls_assert();
		return GNUTLS_E_UNKNOWN_HASH_ALGORITHM;
	}

	entry = mac_to_entry(algo);
	if (entry == NULL)
		goto unknown_mac;

	mac_len = _gnutls_mac_get_algo_len(entry);
	key_len = mac_len;

	/* Read the salt from the structure.
	 */
	result =
	    _gnutls_x509_read_null_value(pkcs12->pkcs12, "macData.macSalt",
				    &salt);
	if (result < 0) {
		gnutls_assert();
		goto cleanup;
	}

	/* Generate the key.
	 */
	result = _gnutls_pkcs12_string_to_key(entry, 3 /*MAC*/,
					      salt.data, salt.size,
					      iter, pass,
					      key_len, key);
	if (result < 0) {
		gnutls_assert();
		goto cleanup;
	}

	/* Get the data to be MACed
	 */
	result = _decode_pkcs12_auth_safe(pkcs12->pkcs12, NULL, &tmp);
	if (result < 0) {
		gnutls_assert();
		goto cleanup;
	}

#if ENABLE_GOST
	/* GOST PKCS#12 files use either PKCS#12 scheme or proprietary
	 * HMAC-based scheme to generate MAC key. */
pkcs12_try_gost:
#endif

	/* MAC the data
	 */
	result = _gnutls_mac_init(&td1, entry, key, key_len);
	if (result < 0) {
		gnutls_assert();
		goto cleanup;
	}

	_gnutls_mac(&td1, tmp.data, tmp.size);

	_gnutls_mac_deinit(&td1, mac_output);

	len = sizeof(mac_output_orig);
	result =
	    asn1_read_value(pkcs12->pkcs12, "macData.mac.digest",
			    mac_output_orig, &len);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto cleanup;
	}

	if ((unsigned)len != mac_len ||
	    memcmp(mac_output_orig, mac_output, len) != 0) {

#if ENABLE_GOST
		/* It is possible that GOST files use proprietary
		 * key generation scheme */
		if (!gost_retry &&
		    (algo == GNUTLS_MAC_GOSTR_94 ||
		     algo == GNUTLS_MAC_STREEBOG_256 ||
		     algo == GNUTLS_MAC_STREEBOG_512)) {
			gost_retry = 1;
			key_len = 32;
			result = _gnutls_pkcs12_gost_string_to_key(algo,
								   salt.data,
								   salt.size,
								   iter,
								   pass,
								   key_len,
								   key);
			if (result < 0) {
				gnutls_assert();
				goto cleanup;
			}

			goto pkcs12_try_gost;
		}
#endif

		gnutls_assert();
		result = GNUTLS_E_MAC_VERIFY_FAILED;
		goto cleanup;
	}

	result = 0;
 cleanup:
	_gnutls_free_datum(&tmp);
	_gnutls_free_datum(&salt);
	return result;
}


static int
write_attributes(gnutls_pkcs12_bag_t bag, int elem,
		 ASN1_TYPE c2, const char *where)
{
	int result;
	char root[128];

	/* If the bag attributes are empty, then write
	 * nothing to the attribute field.
	 */
	if (bag->element[elem].friendly_name == NULL &&
	    bag->element[elem].local_key_id.data == NULL) {
		/* no attributes
		 */
		result = asn1_write_value(c2, where, NULL, 0);
		if (result != ASN1_SUCCESS) {
			gnutls_assert();
			return _gnutls_asn2err(result);
		}

		return 0;
	}

	if (bag->element[elem].local_key_id.data != NULL) {

		/* Add a new Attribute
		 */
		result = asn1_write_value(c2, where, "NEW", 1);
		if (result != ASN1_SUCCESS) {
			gnutls_assert();
			return _gnutls_asn2err(result);
		}

		_gnutls_str_cpy(root, sizeof(root), where);
		_gnutls_str_cat(root, sizeof(root), ".?LAST");

		result =
		    _gnutls_x509_encode_and_write_attribute(KEY_ID_OID, c2,
							    root,
							    bag->element
							    [elem].
							    local_key_id.data,
							    bag->element
							    [elem].
							    local_key_id.size,
							    1);
		if (result < 0) {
			gnutls_assert();
			return result;
		}
	}

	if (bag->element[elem].friendly_name != NULL) {
		uint8_t *name;
		int size, i;
		const char *p;

		/* Add a new Attribute
		 */
		result = asn1_write_value(c2, where, "NEW", 1);
		if (result != ASN1_SUCCESS) {
			gnutls_assert();
			return _gnutls_asn2err(result);
		}

		/* convert name to BMPString
		 */
		size = strlen(bag->element[elem].friendly_name) * 2;
		name = gnutls_malloc(size);

		if (name == NULL) {
			gnutls_assert();
			return GNUTLS_E_MEMORY_ERROR;
		}

		p = bag->element[elem].friendly_name;
		for (i = 0; i < size; i += 2) {
			name[i] = 0;
			name[i + 1] = *p;
			p++;
		}

		_gnutls_str_cpy(root, sizeof(root), where);
		_gnutls_str_cat(root, sizeof(root), ".?LAST");

		result =
		    _gnutls_x509_encode_and_write_attribute
		    (FRIENDLY_NAME_OID, c2, root, name, size, 1);

		gnutls_free(name);

		if (result < 0) {
			gnutls_assert();
			return result;
		}
	}

	return 0;
}


/* Encodes the bag into a SafeContents structure, and puts the output in
 * the given datum. Enc is set to non-zero if the data are encrypted;
 */
int
_pkcs12_encode_safe_contents(gnutls_pkcs12_bag_t bag, ASN1_TYPE * contents,
			     int *enc)
{
	ASN1_TYPE c2 = ASN1_TYPE_EMPTY;
	int result;
	unsigned i;
	const char *oid;

	if (bag->element[0].type == GNUTLS_BAG_ENCRYPTED && enc) {
		*enc = 1;
		return 0;	/* ENCRYPTED BAG, do nothing. */
	} else if (enc)
		*enc = 0;

	/* Step 1. Create the SEQUENCE.
	 */

	if ((result = asn1_create_element
	     (_gnutls_get_pkix(), "PKIX1.pkcs-12-SafeContents",
	      &c2)) != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto cleanup;
	}

	for (i = 0; i < bag->bag_elements; i++) {

		oid = bag_to_oid(bag->element[i].type);
		if (oid == NULL) {
			gnutls_assert();
			continue;
		}

		result = asn1_write_value(c2, "", "NEW", 1);
		if (result != ASN1_SUCCESS) {
			gnutls_assert();
			result = _gnutls_asn2err(result);
			goto cleanup;
		}

		/* Copy the bag type.
		 */
		result = asn1_write_value(c2, "?LAST.bagId", oid, 1);
		if (result != ASN1_SUCCESS) {
			gnutls_assert();
			result = _gnutls_asn2err(result);
			goto cleanup;
		}

		/* Set empty attributes
		 */
		result =
		    write_attributes(bag, i, c2, "?LAST.bagAttributes");
		if (result < 0) {
			gnutls_assert();
			goto cleanup;
		}


		/* Copy the Bag Value
		 */

		if (bag->element[i].type == GNUTLS_BAG_CERTIFICATE ||
		    bag->element[i].type == GNUTLS_BAG_SECRET ||
		    bag->element[i].type == GNUTLS_BAG_CRL) {
			gnutls_datum_t tmp;

			/* in that case encode it to a CertBag or
			 * a CrlBag.
			 */

			result =
			    _pkcs12_encode_crt_bag(bag->element[i].type,
						   &bag->element[i].data,
						   &tmp);

			if (result < 0) {
				gnutls_assert();
				goto cleanup;
			}

			result =
			    _gnutls_x509_write_value(c2, "?LAST.bagValue",
						     &tmp);

			_gnutls_free_datum(&tmp);

		} else {

			result =
			    _gnutls_x509_write_value(c2, "?LAST.bagValue",
						     &bag->element[i].
						     data);
		}

		if (result < 0) {
			gnutls_assert();
			goto cleanup;
		}

	}

	/* Encode the data and copy them into the datum
	 */
	*contents = c2;

	return 0;

      cleanup:
	if (c2)
		asn1_delete_structure(&c2);
	return result;

}

/* Checks if the extra_certs contain certificates that may form a chain
 * with the first certificate in chain (it is expected that chain_len==1)
 * and appends those in the chain.
 */
static int make_chain(gnutls_x509_crt_t ** chain, unsigned int *chain_len,
		      gnutls_x509_crt_t ** extra_certs,
		      unsigned int *extra_certs_len, unsigned int flags)
{
	unsigned int i;

	if (*chain_len != 1)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	i = 0;
	while (i < *extra_certs_len) {
		/* if it is an issuer but not a self-signed one */
		if (gnutls_x509_crt_check_issuer
		    ((*chain)[*chain_len - 1], (*extra_certs)[i]) != 0) {
			if (!(flags & GNUTLS_PKCS12_SP_INCLUDE_SELF_SIGNED)
			    &&
			    gnutls_x509_crt_check_issuer((*extra_certs)[i],
							 (*extra_certs)[i])
			    != 0)
				goto skip;

			*chain =
			    gnutls_realloc_fast(*chain,
						sizeof((*chain)[0]) *
						++(*chain_len));
			if (*chain == NULL) {
				gnutls_assert();
				return GNUTLS_E_MEMORY_ERROR;
			}
			(*chain)[*chain_len - 1] = (*extra_certs)[i];

			(*extra_certs)[i] =
			    (*extra_certs)[*extra_certs_len - 1];
			(*extra_certs_len)--;

			i = 0;
			continue;
		}

	      skip:
		i++;
	}
	return 0;
}

/**
 * gnutls_pkcs12_simple_parse:
 * @p12: A pkcs12 type
 * @password: optional password used to decrypt the structure, bags and keys.
 * @key: a structure to store the parsed private key.
 * @chain: the corresponding to key certificate chain (may be %NULL)
 * @chain_len: will be updated with the number of additional (may be %NULL)
 * @extra_certs: optional pointer to receive an array of additional
 *	       certificates found in the PKCS12 structure (may be %NULL).
 * @extra_certs_len: will be updated with the number of additional
 *		   certs (may be %NULL).
 * @crl: an optional structure to store the parsed CRL (may be %NULL).
 * @flags: should be zero or one of GNUTLS_PKCS12_SP_*
 *
 * This function parses a PKCS12 structure in @pkcs12 and extracts the
 * private key, the corresponding certificate chain, any additional
 * certificates and a CRL. The structures in @key, @chain @crl, and @extra_certs
 * must not be initialized.
 *
 * The @extra_certs and @extra_certs_len parameters are optional
 * and both may be set to %NULL. If either is non-%NULL, then both must
 * be set. The value for @extra_certs is allocated
 * using gnutls_malloc().
 * 
 * Encrypted PKCS12 bags and PKCS8 private keys are supported, but
 * only with password based security and the same password for all
 * operations.
 *
 * Note that a PKCS12 structure may contain many keys and/or certificates,
 * and there is no way to identify which key/certificate pair you want.
 * For this reason this function is useful for PKCS12 files that contain 
 * only one key/certificate pair and/or one CRL.
 *
 * If the provided structure has encrypted fields but no password
 * is provided then this function returns %GNUTLS_E_DECRYPTION_FAILED.
 *
 * Note that normally the chain constructed does not include self signed
 * certificates, to comply with TLS' requirements. If, however, the flag 
 * %GNUTLS_PKCS12_SP_INCLUDE_SELF_SIGNED is specified then
 * self signed certificates will be included in the chain.
 *
 * Prior to using this function the PKCS #12 structure integrity must
 * be verified using gnutls_pkcs12_verify_mac().
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.1.0
 **/
int
gnutls_pkcs12_simple_parse(gnutls_pkcs12_t p12,
			   const char *password,
			   gnutls_x509_privkey_t * key,
			   gnutls_x509_crt_t ** chain,
			   unsigned int *chain_len,
			   gnutls_x509_crt_t ** extra_certs,
			   unsigned int *extra_certs_len,
			   gnutls_x509_crl_t * crl, unsigned int flags)
{
	gnutls_pkcs12_bag_t bag = NULL;
	gnutls_x509_crt_t *_extra_certs = NULL;
	unsigned int _extra_certs_len = 0;
	gnutls_x509_crt_t *_chain = NULL;
	unsigned int _chain_len = 0;
	int idx = 0;
	int ret;
	size_t cert_id_size = 0;
	size_t key_id_size = 0;
	uint8_t cert_id[20];
	uint8_t key_id[20];
	int privkey_ok = 0;
	unsigned int i;
	int elements_in_bag;

	*key = NULL;

	if (crl)
		*crl = NULL;

	/* find the first private key */
	for (;;) {

		ret = gnutls_pkcs12_bag_init(&bag);
		if (ret < 0) {
			bag = NULL;
			gnutls_assert();
			goto done;
		}

		ret = gnutls_pkcs12_get_bag(p12, idx, bag);
		if (ret == GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE) {
			gnutls_pkcs12_bag_deinit(bag);
			bag = NULL;
			break;
		}
		if (ret < 0) {
			gnutls_assert();
			goto done;
		}

		ret = gnutls_pkcs12_bag_get_type(bag, 0);
		if (ret < 0) {
			gnutls_assert();
			goto done;
		}

		if (ret == GNUTLS_BAG_ENCRYPTED) {
			if (password == NULL) {
				ret =
				    gnutls_assert_val
				    (GNUTLS_E_DECRYPTION_FAILED);
				goto done;
			}

			ret = gnutls_pkcs12_bag_decrypt(bag, password);
			if (ret < 0) {
				gnutls_assert();
				goto done;
			}
		}

		elements_in_bag = gnutls_pkcs12_bag_get_count(bag);
		if (elements_in_bag < 0) {
			gnutls_assert();
			goto done;
		}

		for (i = 0; i < (unsigned)elements_in_bag; i++) {
			int type;
			gnutls_datum_t data;

			type = gnutls_pkcs12_bag_get_type(bag, i);
			if (type < 0) {
				gnutls_assert();
				goto done;
			}

			ret = gnutls_pkcs12_bag_get_data(bag, i, &data);
			if (ret < 0) {
				gnutls_assert();
				goto done;
			}

			switch (type) {
			case GNUTLS_BAG_PKCS8_ENCRYPTED_KEY:
				if (password == NULL) {
					ret =
					    gnutls_assert_val
					    (GNUTLS_E_DECRYPTION_FAILED);
					goto done;
				}

				FALLTHROUGH;
			case GNUTLS_BAG_PKCS8_KEY:
				if (*key != NULL) {	/* too simple to continue */
					gnutls_assert();
					break;
				}

				ret = gnutls_x509_privkey_init(key);
				if (ret < 0) {
					gnutls_assert();
					goto done;
				}

				ret = gnutls_x509_privkey_import_pkcs8
				    (*key, &data, GNUTLS_X509_FMT_DER,
				     password,
				     type ==
				     GNUTLS_BAG_PKCS8_KEY ?
				     GNUTLS_PKCS_PLAIN : 0);
				if (ret < 0) {
					gnutls_assert();
					goto done;
				}

				key_id_size = sizeof(key_id);
				ret =
				    gnutls_x509_privkey_get_key_id(*key, 0,
								   key_id,
								   &key_id_size);
				if (ret < 0) {
					gnutls_assert();
					goto done;
				}

				privkey_ok = 1;	/* break */
				break;
			default:
				break;
			}
		}

		idx++;
		gnutls_pkcs12_bag_deinit(bag);
		bag = NULL;

		if (privkey_ok != 0)	/* private key was found */
			break;
	}

	if (privkey_ok == 0) {	/* no private key */
		gnutls_assert();
		return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
	}

	/* now find the corresponding certificate 
	 */
	idx = 0;
	bag = NULL;
	for (;;) {
		ret = gnutls_pkcs12_bag_init(&bag);
		if (ret < 0) {
			bag = NULL;
			gnutls_assert();
			goto done;
		}

		ret = gnutls_pkcs12_get_bag(p12, idx, bag);
		if (ret == GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE) {
			gnutls_pkcs12_bag_deinit(bag);
			bag = NULL;
			break;
		}
		if (ret < 0) {
			gnutls_assert();
			goto done;
		}

		ret = gnutls_pkcs12_bag_get_type(bag, 0);
		if (ret < 0) {
			gnutls_assert();
			goto done;
		}

		if (ret == GNUTLS_BAG_ENCRYPTED) {
			ret = gnutls_pkcs12_bag_decrypt(bag, password);
			if (ret < 0) {
				gnutls_assert();
				goto done;
			}
		}

		elements_in_bag = gnutls_pkcs12_bag_get_count(bag);
		if (elements_in_bag < 0) {
			gnutls_assert();
			goto done;
		}

		for (i = 0; i < (unsigned)elements_in_bag; i++) {
			int type;
			gnutls_datum_t data;
			gnutls_x509_crt_t this_cert;

			type = gnutls_pkcs12_bag_get_type(bag, i);
			if (type < 0) {
				gnutls_assert();
				goto done;
			}

			ret = gnutls_pkcs12_bag_get_data(bag, i, &data);
			if (ret < 0) {
				gnutls_assert();
				goto done;
			}

			switch (type) {
			case GNUTLS_BAG_CERTIFICATE:
				ret = gnutls_x509_crt_init(&this_cert);
				if (ret < 0) {
					gnutls_assert();
					goto done;
				}

				ret =
				    gnutls_x509_crt_import(this_cert,
							   &data,
							   GNUTLS_X509_FMT_DER);
				if (ret < 0) {
					gnutls_assert();
					gnutls_x509_crt_deinit(this_cert);
					this_cert = NULL;
					goto done;
				}

				/* check if the key id match */
				cert_id_size = sizeof(cert_id);
				ret =
				    gnutls_x509_crt_get_key_id(this_cert,
							       0, cert_id,
							       &cert_id_size);
				if (ret < 0) {
					gnutls_assert();
					gnutls_x509_crt_deinit(this_cert);
					this_cert = NULL;
					goto done;
				}

				if (memcmp(cert_id, key_id, cert_id_size) != 0) {	/* they don't match - skip the certificate */
					_extra_certs =
						gnutls_realloc_fast
						(_extra_certs,
						 sizeof(_extra_certs
							[0]) *
						 ++_extra_certs_len);
					if (!_extra_certs) {
						gnutls_assert();
						ret =
							GNUTLS_E_MEMORY_ERROR;
						goto done;
					}
					_extra_certs
						[_extra_certs_len -
						 1] = this_cert;
					this_cert = NULL;
				} else {
					if (chain && _chain_len == 0) {
						_chain =
						    gnutls_malloc(sizeof
								  (_chain
								   [0]) *
								  (++_chain_len));
						if (!_chain) {
							gnutls_assert();
							ret =
							    GNUTLS_E_MEMORY_ERROR;
							goto done;
						}
						_chain[_chain_len - 1] =
						    this_cert;
						this_cert = NULL;
					} else {
						gnutls_x509_crt_deinit
						    (this_cert);
						this_cert = NULL;
					}
				}
				break;

			case GNUTLS_BAG_CRL:
				if (crl == NULL || *crl != NULL) {
					gnutls_assert();
					break;
				}

				ret = gnutls_x509_crl_init(crl);
				if (ret < 0) {
					gnutls_assert();
					goto done;
				}

				ret =
				    gnutls_x509_crl_import(*crl, &data,
							   GNUTLS_X509_FMT_DER);
				if (ret < 0) {
					gnutls_assert();
					gnutls_x509_crl_deinit(*crl);
					*crl = NULL;
					goto done;
				}
				break;

			case GNUTLS_BAG_ENCRYPTED:
				/* XXX Bother to recurse one level down?  Unlikely to
				   use the same password anyway. */
			case GNUTLS_BAG_EMPTY:
			default:
				break;
			}
		}

		idx++;
		gnutls_pkcs12_bag_deinit(bag);
		bag = NULL;
	}

	if (chain != NULL) {
		if (_chain_len != 1) {
			ret = GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
			goto done;
		}

		ret =
		    make_chain(&_chain, &_chain_len, &_extra_certs,
			       &_extra_certs_len, flags);
		if (ret < 0) {
			gnutls_assert();
			goto done;
		}
	}

	ret = 0;

      done:
	if (bag)
		gnutls_pkcs12_bag_deinit(bag);

	if (ret < 0) {
		if (*key) {
			gnutls_x509_privkey_deinit(*key);
			*key = NULL;
		}
		if (crl != NULL && *crl != NULL) {
			gnutls_x509_crl_deinit(*crl);
			*crl = NULL;
		}
		if (_extra_certs_len && _extra_certs != NULL) {
			for (i = 0; i < _extra_certs_len; i++)
				gnutls_x509_crt_deinit(_extra_certs[i]);
			gnutls_free(_extra_certs);
		}
		if (_chain_len && _chain != NULL) {
			for (i = 0; i < _chain_len; i++)
				gnutls_x509_crt_deinit(_chain[i]);
			gnutls_free(_chain);
		}

		return ret;
	}

	if (extra_certs && _extra_certs_len > 0) {
		*extra_certs = _extra_certs;
		*extra_certs_len = _extra_certs_len;
	} else {
		if (extra_certs) {
			*extra_certs = NULL;
			*extra_certs_len = 0;
		}
		for (i = 0; i < _extra_certs_len; i++)
			gnutls_x509_crt_deinit(_extra_certs[i]);
		gnutls_free(_extra_certs);
	}

	if (chain != NULL) {
		*chain = _chain;
		*chain_len = _chain_len;
	}

	return ret;
}


/**
 * gnutls_pkcs12_mac_info:
 * @pkcs12: A pkcs12 type
 * @mac: the MAC algorithm used as %gnutls_mac_algorithm_t
 * @salt: the salt used for string to key (if non-NULL then @salt_size initially holds its size)
 * @salt_size: string to key salt size
 * @iter_count: string to key iteration count
 * @oid: if non-NULL it will contain an allocated null-terminated variable with the OID
 *
 * This function will provide information on the MAC algorithm used
 * in a PKCS #12 structure. If the structure algorithms
 * are unknown the code %GNUTLS_E_UNKNOWN_HASH_ALGORITHM will be returned,
 * and only @oid, will be set. That is, @oid will be set on structures
 * with a MAC whether supported or not. It must be deinitialized using gnutls_free().
 * The other variables are only set on supported structures.
 *
 * Returns: %GNUTLS_E_INVALID_REQUEST if the provided structure doesn't contain a MAC,
 *  %GNUTLS_E_UNKNOWN_HASH_ALGORITHM if the structure's MAC isn't supported, or
 *  another negative error code in case of a failure. Zero on success.
 **/
int
gnutls_pkcs12_mac_info(gnutls_pkcs12_t pkcs12, unsigned int *mac,
	void *salt, unsigned int *salt_size, unsigned int *iter_count, char **oid)
{
	int ret;
	gnutls_datum_t tmp = { NULL, 0 }, dsalt = {
	NULL, 0};
	gnutls_mac_algorithm_t algo;

	if (oid)
		*oid = NULL;

	if (pkcs12 == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	ret =
	    _gnutls_x509_read_value(pkcs12->pkcs12, "macData.mac.digestAlgorithm.algorithm",
				    &tmp);
	if (ret < 0) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	if (oid) {
		*oid = (char*)tmp.data;
	}

	algo = gnutls_oid_to_digest((char*)tmp.data);
	if (algo == GNUTLS_MAC_UNKNOWN || mac_to_entry(algo) == NULL) {
		gnutls_assert();
		return GNUTLS_E_UNKNOWN_HASH_ALGORITHM;
	}

	if (oid) {
		tmp.data = NULL;
	}

	if (mac) {
		*mac = algo;
	}

	if (iter_count) {
		ret =
		    _gnutls_x509_read_uint(pkcs12->pkcs12, "macData.iterations",
				   iter_count);
		if (ret < 0) {
			*iter_count = 1;	/* the default */
		}
	}

	if (salt) {
		/* Read the salt from the structure.
		 */
		ret =
		    _gnutls_x509_read_null_value(pkcs12->pkcs12, "macData.macSalt",
					    &dsalt);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		if (*salt_size >= (unsigned)dsalt.size) {
			*salt_size = dsalt.size;
			if (dsalt.size > 0)
				memcpy(salt, dsalt.data, dsalt.size);
		} else {
			*salt_size = dsalt.size;
			ret = gnutls_assert_val(GNUTLS_E_SHORT_MEMORY_BUFFER);
			goto cleanup;
		}
	}

	ret = 0;
 cleanup:
	_gnutls_free_datum(&tmp);
	_gnutls_free_datum(&dsalt);
	return ret;

}

