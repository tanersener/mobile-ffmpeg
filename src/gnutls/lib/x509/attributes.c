/*
 * Copyright (C) 2003-2016 Free Software Foundation, Inc.
 * Copyright (C) 2012-2016 Nikos Mavrogiannopoulos
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

#include "gnutls_int.h"

#include <datum.h>
#include "errors.h"
#include <common.h>
#include <x509.h>
#include "x509_int.h"
#include "attributes.h"

/* Functions to parse and set the PKIX1 Attributes structure.
 */

/* Overwrite the given attribute (using the index)
 * index here starts from one.
 */
static int
overwrite_attribute(ASN1_TYPE asn, const char *root, unsigned indx,
		    const gnutls_datum_t * ext_data)
{
	char name[MAX_NAME_SIZE], name2[MAX_NAME_SIZE];
	int result;

	snprintf(name, sizeof(name), "%s.?%u", root, indx);

	_gnutls_str_cpy(name2, sizeof(name2), name);
	_gnutls_str_cat(name2, sizeof(name2), ".values.?LAST");

	result = _gnutls_x509_write_value(asn, name2, ext_data);
	if (result < 0) {
		gnutls_assert();
		return result;
	}


	return 0;
}

/* Parses an Attribute list in the asn1_struct, and searches for the
 * given OID. The index indicates the attribute value to be returned.
 *
 * If raw==0 only printable data are returned, or
 * GNUTLS_E_X509_UNSUPPORTED_ATTRIBUTE.
 *
 * asn1_attr_name must be a string in the form
 * "certificationRequestInfo.attributes"
 *
 */
int
_x509_parse_attribute(ASN1_TYPE asn1_struct,
		const char *attr_name, const char *given_oid, unsigned indx,
		int raw, gnutls_datum_t * out)
{
	int k1, result;
	char tmpbuffer1[MAX_NAME_SIZE];
	char tmpbuffer3[MAX_NAME_SIZE];
	char value[200];
	gnutls_datum_t td;
	char oid[MAX_OID_SIZE];
	int len;

	k1 = 0;
	do {

		k1++;
		/* create a string like "attribute.?1"
		 */
		if (attr_name[0] != 0)
			snprintf(tmpbuffer1, sizeof(tmpbuffer1), "%s.?%u",
				 attr_name, k1);
		else
			snprintf(tmpbuffer1, sizeof(tmpbuffer1), "?%u",
				 k1);

		len = sizeof(value) - 1;
		result =
		    asn1_read_value(asn1_struct, tmpbuffer1, value, &len);

		if (result == ASN1_ELEMENT_NOT_FOUND) {
			gnutls_assert();
			break;
		}

		if (result != ASN1_VALUE_NOT_FOUND) {
			gnutls_assert();
			result = _gnutls_asn2err(result);
			goto cleanup;
		}

		/* Move to the attribute type and values
		 */
		/* Read the OID
		 */
		_gnutls_str_cpy(tmpbuffer3, sizeof(tmpbuffer3),
				tmpbuffer1);
		_gnutls_str_cat(tmpbuffer3, sizeof(tmpbuffer3), ".type");

		len = sizeof(oid) - 1;
		result =
		    asn1_read_value(asn1_struct, tmpbuffer3, oid, &len);

		if (result == ASN1_ELEMENT_NOT_FOUND)
			break;
		else if (result != ASN1_SUCCESS) {
			gnutls_assert();
			result = _gnutls_asn2err(result);
			goto cleanup;
		}

		if (strcmp(oid, given_oid) == 0) {	/* Found the OID */

			/* Read the Value
			 */
			snprintf(tmpbuffer3, sizeof(tmpbuffer3),
				 "%s.values.?%u", tmpbuffer1, indx + 1);

			len = sizeof(value) - 1;
			result =
			    _gnutls_x509_read_value(asn1_struct,
						    tmpbuffer3, &td);

			if (result != ASN1_SUCCESS) {
				gnutls_assert();
				result = _gnutls_asn2err(result);
				goto cleanup;
			}

			if (raw == 0) {
				result =
				    _gnutls_x509_dn_to_string
				    (oid, td.data, td.size, out);

				_gnutls_free_datum(&td);

				if (result < 0) {
					gnutls_assert();
					goto cleanup;
				}
				return 0;
			} else {	/* raw!=0 */
				out->data = td.data;
				out->size = td.size;

				return 0;
			}
		}

	}
	while (1);

	gnutls_assert();

	result = GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;

      cleanup:
	return result;
}

/* This function will attempt to set the requested attribute in
 * the given X509v3 certificate.
 *
 * Critical will be either 0 or 1.
 */
static int
add_attribute(ASN1_TYPE asn, const char *root, const char *attribute_id,
	      const gnutls_datum_t * ext_data)
{
	int result;
	char name[MAX_NAME_SIZE];

	snprintf(name, sizeof(name), "%s", root);

	/* Add a new attribute in the list.
	 */
	result = asn1_write_value(asn, name, "NEW", 1);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	snprintf(name, sizeof(name), "%s.?LAST.type", root);

	result = asn1_write_value(asn, name, attribute_id, 1);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	snprintf(name, sizeof(name), "%s.?LAST.values", root);

	result = asn1_write_value(asn, name, "NEW", 1);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	snprintf(name, sizeof(name), "%s.?LAST.values.?LAST", root);

	result = _gnutls_x509_write_value(asn, name, ext_data);
	if (result < 0) {
		gnutls_assert();
		return result;
	}

	return 0;
}


int
_x509_set_attribute(ASN1_TYPE asn, const char *root,
	      const char *ext_id, const gnutls_datum_t * ext_data)
{
	int result;
	int k, len;
	char name[MAX_NAME_SIZE], name2[MAX_NAME_SIZE];
	char extnID[MAX_OID_SIZE];

	/* Find the index of the given attribute.
	 */
	k = 0;
	do {
		k++;

		snprintf(name, sizeof(name), "%s.?%u", root, k);

		len = sizeof(extnID) - 1;
		result = asn1_read_value(asn, name, extnID, &len);

		/* move to next
		 */

		if (result == ASN1_ELEMENT_NOT_FOUND) {
			break;
		}

		do {

			_gnutls_str_cpy(name2, sizeof(name2), name);
			_gnutls_str_cat(name2, sizeof(name2), ".type");

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
				/* attribute was found
				 */
				return overwrite_attribute(asn, root, k,
							   ext_data);
			}


		}
		while (0);
	}
	while (1);

	if (result == ASN1_ELEMENT_NOT_FOUND) {
		return add_attribute(asn, root, ext_id, ext_data);
	} else {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}


	return 0;
}
