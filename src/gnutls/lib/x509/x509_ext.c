/*
 * Copyright (C) 2014-2016 Free Software Foundation, Inc.
 * Copyright (C) 2016 Red Hat, Inc.
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

/* This file contains functions to handle X.509 certificate extensions (the x509-ext API)
 */

#include "gnutls_int.h"
#include <datum.h>
#include "errors.h"
#include <common.h>
#include <x509.h>
#include <x509_b64.h>
#include "x509_ext_int.h"
#include "virt-san.h"
#include <gnutls/x509-ext.h>

#define MAX_ENTRIES 64
struct gnutls_subject_alt_names_st {
	struct name_st *names;
	unsigned int size;
};

/**
 * gnutls_subject_alt_names_init:
 * @sans: The alternative names
 *
 * This function will initialize an alternative names structure.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a negative error value.
 *
 * Since: 3.3.0
 **/
int gnutls_subject_alt_names_init(gnutls_subject_alt_names_t * sans)
{
	*sans = gnutls_calloc(1, sizeof(struct gnutls_subject_alt_names_st));
	if (*sans == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	return 0;
}

static void subject_alt_names_deinit(gnutls_subject_alt_names_t sans)
{
	unsigned int i;

	for (i = 0; i < sans->size; i++) {
		gnutls_free(sans->names[i].san.data);
		gnutls_free(sans->names[i].othername_oid.data);
	}
	gnutls_free(sans->names);
}

/**
 * gnutls_subject_alt_names_deinit:
 * @sans: The alternative names
 *
 * This function will deinitialize an alternative names structure.
 *
 * Since: 3.3.0
 **/
void gnutls_subject_alt_names_deinit(gnutls_subject_alt_names_t sans)
{
	subject_alt_names_deinit(sans);
	gnutls_free(sans);
}

/**
 * gnutls_subject_alt_names_get:
 * @sans: The alternative names
 * @seq: The index of the name to get
 * @san_type: Will hold the type of the name (of %gnutls_subject_alt_names_t)
 * @san: The alternative name data (should be treated as constant)
 * @othername_oid: The object identifier if @san_type is %GNUTLS_SAN_OTHERNAME (should be treated as constant)
 *
 * This function will return a specific alternative name as stored in
 * the @sans type. The returned values should be treated as constant
 * and valid for the lifetime of @sans.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE
 * if the index is out of bounds, otherwise a negative error value.
 *
 * Since: 3.3.0
 **/
int gnutls_subject_alt_names_get(gnutls_subject_alt_names_t sans,
				 unsigned int seq, unsigned int *san_type,
				 gnutls_datum_t * san,
				 gnutls_datum_t * othername_oid)
{
	if (seq >= sans->size)
		return gnutls_assert_val(GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE);

	if (san) {
		memcpy(san, &sans->names[seq].san, sizeof(gnutls_datum_t));
	}

	if (san_type)
		*san_type = sans->names[seq].type;

	if (othername_oid != NULL && sans->names[seq].type == GNUTLS_SAN_OTHERNAME) {
		othername_oid->data = sans->names[seq].othername_oid.data;
		othername_oid->size = sans->names[seq].othername_oid.size;
	}

	return 0;
}

/* This is the same as gnutls_subject_alt_names_set() but will not
 * copy the strings. It expects all the provided input to be already
 * allocated by gnutls. */
static
int subject_alt_names_set(struct name_st **names,
			  unsigned int *size,
			  unsigned int san_type,
			  gnutls_datum_t * san, char *othername_oid,
			  unsigned raw)
{
	void *tmp;
	int ret;

	tmp = gnutls_realloc(*names, (*size + 1) * sizeof((*names)[0]));
	if (tmp == NULL) {
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);
	}
	*names = tmp;

	ret = _gnutls_alt_name_assign_virt_type(&(*names)[*size], san_type, san, othername_oid, raw);
	if (ret < 0)
		return gnutls_assert_val(ret);

	(*size)++;
	return 0;
}

/**
 * gnutls_subject_alt_names_set:
 * @sans: The alternative names
 * @san_type: The type of the name (of %gnutls_subject_alt_names_t)
 * @san: The alternative name data
 * @othername_oid: The object identifier if @san_type is %GNUTLS_SAN_OTHERNAME
 *
 * This function will store the specified alternative name in
 * the @sans.
 *
 * Since version 3.5.7 the %GNUTLS_SAN_RFC822NAME, %GNUTLS_SAN_DNSNAME, and
 * %GNUTLS_SAN_OTHERNAME_XMPP are converted to ACE format when necessary.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0), otherwise a negative error value.
 *
 * Since: 3.3.0
 **/
int gnutls_subject_alt_names_set(gnutls_subject_alt_names_t sans,
				 unsigned int san_type,
				 const gnutls_datum_t * san,
				 const char *othername_oid)
{
	int ret;
	gnutls_datum_t copy;
	char *ooc;

	ret = _gnutls_set_strdatum(&copy, san->data, san->size);
	if (ret < 0)
		return gnutls_assert_val(ret);

	if (othername_oid != NULL)
		ooc = gnutls_strdup(othername_oid);
	else
		ooc = NULL;
	ret = subject_alt_names_set(&sans->names, &sans->size,
				    san_type, &copy, ooc, 0);
	if (ret < 0) {
		gnutls_free(copy.data);
		return gnutls_assert_val(ret);
	}

	return 0;
}

/**
 * gnutls_x509_ext_import_subject_alt_names:
 * @ext: The DER-encoded extension data
 * @sans: The alternative names
 * @flags: should be zero
 *
 * This function will export the alternative names in the provided DER-encoded
 * SubjectAltName PKIX extension, to a %gnutls_subject_alt_names_t type. @sans
 * must be initialized.
 *
 * This function will succeed even if there no subject alternative names
 * in the structure.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a negative error value.
 *
 * Since: 3.3.0
 **/
int gnutls_x509_ext_import_subject_alt_names(const gnutls_datum_t * ext,
					  gnutls_subject_alt_names_t sans,
					  unsigned int flags)
{
	ASN1_TYPE c2 = ASN1_TYPE_EMPTY;
	int result, ret;
	unsigned int i;
	gnutls_datum_t san, othername_oid;
	unsigned type;

	result =
	    asn1_create_element(_gnutls_get_pkix(), "PKIX1.GeneralNames", &c2);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	result = _asn1_strict_der_decode(&c2, ext->data, ext->size, NULL);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		ret = _gnutls_asn2err(result);
		goto cleanup;
	}

	i = 0;
	do {
		san.data = NULL;
		san.size = 0;
		othername_oid.data = NULL;

		ret = _gnutls_parse_general_name2(c2, "", i, &san, &type, 0);
		if (ret < 0)
			break;

		if (type == GNUTLS_SAN_OTHERNAME) {
			ret =
			    _gnutls_parse_general_name2(c2, "", i,
							&othername_oid,
							NULL, 1);
			if (ret < 0)
				break;

		} else if (san.size == 0 || san.data == NULL) {
			ret = gnutls_assert_val(GNUTLS_E_X509_UNKNOWN_SAN);
			break;
		}

		ret = subject_alt_names_set(&sans->names, &sans->size,
					    type, &san,
					    (char *)othername_oid.data, 1);
		if (ret < 0)
			break;

		i++;
	} while (ret >= 0);

	sans->size = i;
	if (ret < 0 && ret != GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE) {
		gnutls_free(san.data);
		gnutls_free(othername_oid.data);
		gnutls_assert();
		goto cleanup;
	}

	ret = 0;
 cleanup:
	asn1_delete_structure(&c2);
	return ret;
}

/**
 * gnutls_x509_ext_export_subject_alt_names:
 * @sans: The alternative names
 * @ext: The DER-encoded extension data; must be freed using gnutls_free().
 *
 * This function will convert the provided alternative names structure to a
 * DER-encoded SubjectAltName PKIX extension. The output data in @ext will be allocated using
 * gnutls_malloc().
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a negative error value.
 *
 * Since: 3.3.0
 **/
int gnutls_x509_ext_export_subject_alt_names(gnutls_subject_alt_names_t sans,
					  gnutls_datum_t * ext)
{
	ASN1_TYPE c2 = ASN1_TYPE_EMPTY;
	int result, ret;
	unsigned i;

	result =
	    asn1_create_element(_gnutls_get_pkix(), "PKIX1.GeneralNames", &c2);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	for (i = 0; i < sans->size; i++) {
		if (sans->names[i].type == GNUTLS_SAN_OTHERNAME) {
			ret = _gnutls_write_new_othername(c2, "", (char*)sans->names[i].othername_oid.data,
					      sans->names[i].san.data, sans->names[i].san.size);
		} else {
			ret =
			    _gnutls_write_new_general_name(c2, "", sans->names[i].type,
							   sans->names[i].san.data,
							   sans->names[i].san.size);
		}

		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}
	}

	ret = _gnutls_x509_der_encode(c2, "", ext, 0);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = 0;

 cleanup:
	asn1_delete_structure(&c2);
	return ret;
}

/**
 * gnutls_x509_ext_import_name_constraints:
 * @ext: a DER encoded extension
 * @nc: The nameconstraints
 * @flags: zero or %GNUTLS_NAME_CONSTRAINTS_FLAG_APPEND
 *
 * This function will return an intermediate type containing
 * the name constraints of the provided NameConstraints extension. That
 * can be used in combination with gnutls_x509_name_constraints_check()
 * to verify whether a server's name is in accordance with the constraints.
 *
 * When the @flags is set to %GNUTLS_NAME_CONSTRAINTS_FLAG_APPEND, then if 
 * the @nc type is empty this function will behave identically as if the flag was not set.
 * Otherwise if there are elements in the @nc structure then the
 * constraints will be merged with the existing constraints following
 * RFC5280 p6.1.4 (excluded constraints will be appended, permitted
 * will be intersected).
 *
 * Note that @nc must be initialized prior to calling this function.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE
 * if the extension is not present, otherwise a negative error value.
 *
 * Since: 3.3.0
 **/
int gnutls_x509_ext_import_name_constraints(const gnutls_datum_t * ext,
					 gnutls_x509_name_constraints_t nc,
					 unsigned int flags)
{
	int result, ret;
	ASN1_TYPE c2 = ASN1_TYPE_EMPTY;
	gnutls_x509_name_constraints_t nc2 = NULL;

	result = asn1_create_element
	    (_gnutls_get_pkix(), "PKIX1.NameConstraints", &c2);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	result = _asn1_strict_der_decode(&c2, ext->data, ext->size, NULL);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		ret = _gnutls_asn2err(result);
		goto cleanup;
	}

	if (flags & GNUTLS_NAME_CONSTRAINTS_FLAG_APPEND &&
	    (nc->permitted != NULL || nc->excluded != NULL)) {
		ret = gnutls_x509_name_constraints_init (&nc2);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		ret =
		    _gnutls_extract_name_constraints(c2, "permittedSubtrees",
						     &nc2->permitted);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		ret =
		    _gnutls_extract_name_constraints(c2, "excludedSubtrees",
						     &nc2->excluded);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		ret = _gnutls_x509_name_constraints_merge(nc, nc2);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}
	} else {
		_gnutls_name_constraints_node_free(nc->permitted);
		_gnutls_name_constraints_node_free(nc->excluded);

		ret =
		    _gnutls_extract_name_constraints(c2, "permittedSubtrees",
						     &nc->permitted);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		ret =
		    _gnutls_extract_name_constraints(c2, "excludedSubtrees",
						     &nc->excluded);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}
	}

	ret = 0;

 cleanup:
	asn1_delete_structure(&c2);
	if (nc2)
		gnutls_x509_name_constraints_deinit (nc2);

	return ret;
}

/**
 * gnutls_x509_ext_export_name_constraints:
 * @nc: The nameconstraints
 * @ext: The DER-encoded extension data; must be freed using gnutls_free().
 *
 * This function will convert the provided name constraints type to a
 * DER-encoded PKIX NameConstraints (2.5.29.30) extension. The output data in 
 * @ext will be allocated using gnutls_malloc().
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a negative error value.
 *
 * Since: 3.3.0
 **/
int gnutls_x509_ext_export_name_constraints(gnutls_x509_name_constraints_t nc,
					 gnutls_datum_t * ext)
{
	int ret, result;
	uint8_t null = 0;
	ASN1_TYPE c2 = ASN1_TYPE_EMPTY;
	struct name_constraints_node_st *tmp;

	if (nc->permitted == NULL && nc->excluded == NULL)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	result = asn1_create_element
	    (_gnutls_get_pkix(), "PKIX1.NameConstraints", &c2);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	if (nc->permitted == NULL) {
		(void)asn1_write_value(c2, "permittedSubtrees", NULL, 0);
	} else {
		tmp = nc->permitted;
		do {
			result =
			    asn1_write_value(c2, "permittedSubtrees", "NEW", 1);
			if (result != ASN1_SUCCESS) {
				gnutls_assert();
				ret = _gnutls_asn2err(result);
				goto cleanup;
			}

			result =
			    asn1_write_value(c2,
					     "permittedSubtrees.?LAST.maximum",
					     NULL, 0);
			if (result != ASN1_SUCCESS) {
				gnutls_assert();
				ret = _gnutls_asn2err(result);
				goto cleanup;
			}

			result =
			    asn1_write_value(c2,
					     "permittedSubtrees.?LAST.minimum",
					     &null, 1);
			if (result != ASN1_SUCCESS) {
				gnutls_assert();
				ret = _gnutls_asn2err(result);
				goto cleanup;
			}

			ret =
			    _gnutls_write_general_name(c2,
						       "permittedSubtrees.?LAST.base",
						       tmp->type,
						       tmp->name.data,
						       tmp->name.size);
			if (ret < 0) {
				gnutls_assert();
				goto cleanup;
			}
			tmp = tmp->next;
		} while (tmp != NULL);
	}

	if (nc->excluded == NULL) {
		(void)asn1_write_value(c2, "excludedSubtrees", NULL, 0);
	} else {
		tmp = nc->excluded;
		do {
			result =
			    asn1_write_value(c2, "excludedSubtrees", "NEW", 1);
			if (result != ASN1_SUCCESS) {
				gnutls_assert();
				ret = _gnutls_asn2err(result);
				goto cleanup;
			}

			result =
			    asn1_write_value(c2,
					     "excludedSubtrees.?LAST.maximum",
					     NULL, 0);
			if (result != ASN1_SUCCESS) {
				gnutls_assert();
				ret = _gnutls_asn2err(result);
				goto cleanup;
			}

			result =
			    asn1_write_value(c2,
					     "excludedSubtrees.?LAST.minimum",
					     &null, 1);
			if (result != ASN1_SUCCESS) {
				gnutls_assert();
				ret = _gnutls_asn2err(result);
				goto cleanup;
			}

			ret =
			    _gnutls_write_general_name(c2,
						       "excludedSubtrees.?LAST.base",
						       tmp->type,
						       tmp->name.data,
						       tmp->name.size);
			if (ret < 0) {
				gnutls_assert();
				goto cleanup;
			}
			tmp = tmp->next;
		} while (tmp != NULL);

	}

	ret = _gnutls_x509_der_encode(c2, "", ext, 0);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = 0;

 cleanup:
	asn1_delete_structure(&c2);
	return ret;
}

/**
 * gnutls_x509_ext_import_subject_key_id:
 * @ext: a DER encoded extension
 * @id: will contain the subject key ID
 *
 * This function will return the subject key ID stored in the provided
 * SubjectKeyIdentifier extension. The ID will be allocated using
 * gnutls_malloc().
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE
 * if the extension is not present, otherwise a negative error value.
 *
 * Since: 3.3.0
 **/
int gnutls_x509_ext_import_subject_key_id(const gnutls_datum_t * ext,
				       gnutls_datum_t * id)
{
	int result, ret;
	ASN1_TYPE c2 = ASN1_TYPE_EMPTY;

	if (ext->size == 0 || ext->data == NULL) {
		gnutls_assert();
		return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
	}

	result = asn1_create_element
	    (_gnutls_get_pkix(), "PKIX1.SubjectKeyIdentifier", &c2);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	result = _asn1_strict_der_decode(&c2, ext->data, ext->size, NULL);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		ret = _gnutls_asn2err(result);
		goto cleanup;
	}

	ret = _gnutls_x509_read_value(c2, "", id);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = 0;
 cleanup:
	asn1_delete_structure(&c2);

	return ret;

}

/**
 * gnutls_x509_ext_export_subject_key_id:
 * @id: The key identifier
 * @ext: The DER-encoded extension data; must be freed using gnutls_free().
 *
 * This function will convert the provided key identifier to a
 * DER-encoded PKIX SubjectKeyIdentifier extension. 
 * The output data in @ext will be allocated using
 * gnutls_malloc().
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a negative error value.
 *
 * Since: 3.3.0
 **/
int gnutls_x509_ext_export_subject_key_id(const gnutls_datum_t * id,
				       gnutls_datum_t * ext)
{
	ASN1_TYPE c2 = ASN1_TYPE_EMPTY;
	int ret, result;

	result =
	    asn1_create_element(_gnutls_get_pkix(),
				"PKIX1.SubjectKeyIdentifier", &c2);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	result = asn1_write_value(c2, "", id->data, id->size);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		ret = _gnutls_asn2err(result);
		goto cleanup;
	}

	ret = _gnutls_x509_der_encode(c2, "", ext, 0);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = 0;
 cleanup:
	asn1_delete_structure(&c2);
	return ret;
}

struct gnutls_x509_aki_st {
	gnutls_datum_t id;
	struct gnutls_subject_alt_names_st cert_issuer;
	gnutls_datum_t serial;
};

/**
 * gnutls_x509_aki_init:
 * @aki: The authority key ID type
 *
 * This function will initialize an authority key ID.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a negative error value.
 *
 * Since: 3.3.0
 **/
int gnutls_x509_aki_init(gnutls_x509_aki_t * aki)
{
	*aki = gnutls_calloc(1, sizeof(struct gnutls_x509_aki_st));
	if (*aki == NULL)
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

	return 0;
}

/**
 * gnutls_x509_aki_deinit:
 * @aki: The authority key identifier type
 *
 * This function will deinitialize an authority key identifier.
 *
 * Since: 3.3.0
 **/
void gnutls_x509_aki_deinit(gnutls_x509_aki_t aki)
{
	gnutls_free(aki->serial.data);
	gnutls_free(aki->id.data);
	subject_alt_names_deinit(&aki->cert_issuer);
	gnutls_free(aki);
}

/**
 * gnutls_x509_aki_get_id:
 * @aki: The authority key ID
 * @id: Will hold the identifier
 *
 * This function will return the key identifier as stored in
 * the @aki type. The identifier should be treated as constant.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE
 * if the index is out of bounds, otherwise a negative error value.
 *
 * Since: 3.3.0
 **/
int gnutls_x509_aki_get_id(gnutls_x509_aki_t aki, gnutls_datum_t * id)
{
	if (aki->id.size == 0)
		return gnutls_assert_val(GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE);

	memcpy(id, &aki->id, sizeof(gnutls_datum_t));
	return 0;
}

/**
 * gnutls_x509_aki_set_id:
 * @aki: The authority key ID
 * @id: the key identifier
 *
 * This function will set the keyIdentifier to be stored in the @aki
 * type.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a negative error value.
 *
 * Since: 3.3.0
 **/
int gnutls_x509_aki_set_id(gnutls_x509_aki_t aki, const gnutls_datum_t * id)
{
	return _gnutls_set_datum(&aki->id, id->data, id->size);
}

/**
 * gnutls_x509_aki_set_cert_issuer:
 * @aki: The authority key ID
 * @san_type: the type of the name (of %gnutls_subject_alt_names_t), may be null
 * @san: The alternative name data
 * @othername_oid: The object identifier if @san_type is %GNUTLS_SAN_OTHERNAME
 * @serial: The authorityCertSerialNumber number (may be null)
 *
 * This function will set the authorityCertIssuer name and the authorityCertSerialNumber 
 * to be stored in the @aki type. When storing multiple names, the serial
 * should be set on the first call, and subsequent calls should use a %NULL serial.
 *
 * Since version 3.5.7 the %GNUTLS_SAN_RFC822NAME, %GNUTLS_SAN_DNSNAME, and
 * %GNUTLS_SAN_OTHERNAME_XMPP are converted to ACE format when necessary.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a negative error value.
 *
 * Since: 3.3.0
 **/
int gnutls_x509_aki_set_cert_issuer(gnutls_x509_aki_t aki,
				    unsigned int san_type,
				    const gnutls_datum_t *san,
				    const char *othername_oid,
				    const gnutls_datum_t *serial)
{
	int ret;
	gnutls_datum_t t_san, t_othername_oid = { NULL, 0 };

	ret = _gnutls_set_datum(&aki->serial, serial->data, serial->size);
	if (ret < 0)
		return gnutls_assert_val(ret);

	aki->cert_issuer.names[aki->cert_issuer.size].type = san_type;

	ret = _gnutls_set_strdatum(&t_san, san->data, san->size);
	if (ret < 0)
		return gnutls_assert_val(ret);

	if (othername_oid) {
		t_othername_oid.data = (uint8_t *) gnutls_strdup(othername_oid);
		if (t_othername_oid.data == NULL) {
			gnutls_free(t_san.data);
			return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);
		}
		t_othername_oid.size = strlen(othername_oid);
	}

	ret =
	    subject_alt_names_set(&aki->cert_issuer.names,
				  &aki->cert_issuer.size, san_type, &t_san,
				  (char *)t_othername_oid.data, 0);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	return 0;
}

/**
 * gnutls_x509_aki_get_cert_issuer:
 * @aki: The authority key ID
 * @seq: The index of the name to get
 * @san_type: Will hold the type of the name (of %gnutls_subject_alt_names_t)
 * @san: The alternative name data
 * @othername_oid: The object identifier if @san_type is %GNUTLS_SAN_OTHERNAME
 * @serial: The authorityCertSerialNumber number
 *
 * This function will return a specific authorityCertIssuer name as stored in
 * the @aki type, as well as the authorityCertSerialNumber. All the returned
 * values should be treated as constant, and may be set to %NULL when are not required.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE
 * if the index is out of bounds, otherwise a negative error value.
 *
 * Since: 3.3.0
 **/
int gnutls_x509_aki_get_cert_issuer(gnutls_x509_aki_t aki, unsigned int seq,
				    unsigned int *san_type,
				    gnutls_datum_t *san,
				    gnutls_datum_t *othername_oid,
				    gnutls_datum_t *serial)
{
	if (seq >= aki->cert_issuer.size)
		return gnutls_assert_val(GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE);

	if (aki->serial.size == 0)
		return gnutls_assert_val(GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE);

	if (serial)
		memcpy(serial, &aki->serial, sizeof(gnutls_datum_t));

	if (san) {
		memcpy(san, &aki->cert_issuer.names[seq].san,
		       sizeof(gnutls_datum_t));
	}

	if (othername_oid != NULL
	    && aki->cert_issuer.names[seq].type == GNUTLS_SAN_OTHERNAME) {
		othername_oid->data =
		    aki->cert_issuer.names[seq].othername_oid.data;
		othername_oid->size =
		    aki->cert_issuer.names[seq].othername_oid.size;
	}

	if (san_type)
		*san_type = aki->cert_issuer.names[seq].type;

	return 0;

}

/**
 * gnutls_x509_ext_import_authority_key_id:
 * @ext: a DER encoded extension
 * @aki: An initialized authority key identifier type
 * @flags: should be zero
 *
 * This function will return the subject key ID stored in the provided
 * AuthorityKeyIdentifier extension.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE
 * if the extension is not present, otherwise a negative error value.
 *
 * Since: 3.3.0
 **/
int gnutls_x509_ext_import_authority_key_id(const gnutls_datum_t * ext,
					 gnutls_x509_aki_t aki,
					 unsigned int flags)
{
	int ret;
	unsigned i;
	ASN1_TYPE c2 = ASN1_TYPE_EMPTY;
	gnutls_datum_t san, othername_oid;
	unsigned type;

	ret = asn1_create_element
	    (_gnutls_get_pkix(), "PKIX1.AuthorityKeyIdentifier", &c2);
	if (ret != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(ret);
	}

	ret = _asn1_strict_der_decode(&c2, ext->data, ext->size, NULL);
	if (ret != ASN1_SUCCESS) {
		gnutls_assert();
		ret = _gnutls_asn2err(ret);
		goto cleanup;
	}

	/* Read authorityCertIssuer */
	i = 0;
	do {
		san.data = NULL;
		san.size = 0;
		othername_oid.data = NULL;

		ret = _gnutls_parse_general_name2(c2, "authorityCertIssuer", i,
						  &san, &type, 0);
		if (ret < 0)
			break;

		if (type == GNUTLS_SAN_OTHERNAME) {
			ret =
			    _gnutls_parse_general_name2(c2,
							"authorityCertIssuer",
							i,
							&othername_oid,
							NULL, 1);
			if (ret < 0)
				break;
		}

		ret = subject_alt_names_set(&aki->cert_issuer.names,
					    &aki->cert_issuer.size,
					    type, &san,
					    (char *)othername_oid.data, 1);
		if (ret < 0)
			break;

		i++;
	} while (ret >= 0);

	aki->cert_issuer.size = i;
	if (ret < 0 && ret != GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE
	    && ret != GNUTLS_E_ASN1_ELEMENT_NOT_FOUND) {
		gnutls_assert();
		gnutls_free(san.data);
		gnutls_free(othername_oid.data);
		goto cleanup;
	}

	/* Read the serial number */
	ret =
	    _gnutls_x509_read_value(c2, "authorityCertSerialNumber",
				    &aki->serial);
	if (ret < 0 && ret != GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE
	    && ret != GNUTLS_E_ASN1_ELEMENT_NOT_FOUND) {
		gnutls_assert();
		goto cleanup;
	}

	/* Read the key identifier */
	ret = _gnutls_x509_read_value(c2, "keyIdentifier", &aki->id);
	if (ret < 0 && ret != GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE
	    && ret != GNUTLS_E_ASN1_ELEMENT_NOT_FOUND) {
		gnutls_assert();
		goto cleanup;
	}

	ret = 0;

 cleanup:
	asn1_delete_structure(&c2);

	return ret;
}

/**
 * gnutls_x509_ext_export_authority_key_id:
 * @aki: An initialized authority key identifier
 * @ext: The DER-encoded extension data; must be freed using gnutls_free().
 *
 * This function will convert the provided key identifier to a
 * DER-encoded PKIX AuthorityKeyIdentifier extension. 
 * The output data in @ext will be allocated using
 * gnutls_malloc().
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a negative error value.
 *
 * Since: 3.3.0
 **/
int gnutls_x509_ext_export_authority_key_id(gnutls_x509_aki_t aki,
					 gnutls_datum_t * ext)
{
	ASN1_TYPE c2 = ASN1_TYPE_EMPTY;
	unsigned i;
	int result, ret;

	result =
	    asn1_create_element(_gnutls_get_pkix(),
				"PKIX1.AuthorityKeyIdentifier", &c2);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	if (aki->id.data != NULL) {
		result =
		    asn1_write_value(c2, "keyIdentifier", aki->id.data,
				     aki->id.size);
		if (result != ASN1_SUCCESS) {
			gnutls_assert();
			ret = _gnutls_asn2err(result);
			goto cleanup;
		}
	} else {
		(void)asn1_write_value(c2, "keyIdentifier", NULL, 0);
	}

	if (aki->serial.data != NULL) {
		result =
		    asn1_write_value(c2, "authorityCertSerialNumber",
				     aki->serial.data, aki->serial.size);
		if (result != ASN1_SUCCESS) {
			gnutls_assert();
			ret = _gnutls_asn2err(result);
			goto cleanup;
		}
	} else {
		(void)asn1_write_value(c2, "authorityCertSerialNumber", NULL, 0);
	}

	if (aki->cert_issuer.size == 0) {
		(void)asn1_write_value(c2, "authorityCertIssuer", NULL, 0);
	} else {
		for (i = 0; i < aki->cert_issuer.size; i++) {
			ret =
			    _gnutls_write_new_general_name(c2,
							   "authorityCertIssuer",
							   aki->cert_issuer.
							   names[i].type,
							   aki->
							   cert_issuer.names[i].
							   san.data,
							   aki->cert_issuer.
							   names[i].san.size);
			if (ret < 0) {
				gnutls_assert();
				goto cleanup;
			}
		}
	}

	ret = _gnutls_x509_der_encode(c2, "", ext, 0);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = 0;
 cleanup:
	asn1_delete_structure(&c2);
	return ret;

}

/**
 * gnutls_x509_ext_import_key_usage:
 * @ext: the DER encoded extension data
 * @key_usage: where the key usage bits will be stored
 *
 * This function will return certificate's key usage, by reading the DER
 * data of the keyUsage X.509 extension (2.5.29.15). The key usage value will ORed
 * values of the: %GNUTLS_KEY_DIGITAL_SIGNATURE,
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
 * Since: 3.3.0
 **/
int gnutls_x509_ext_import_key_usage(const gnutls_datum_t * ext,
				  unsigned int *key_usage)
{
	ASN1_TYPE c2 = ASN1_TYPE_EMPTY;
	int len, result;
	uint8_t str[2];

	str[0] = str[1] = 0;
	*key_usage = 0;

	if ((result = asn1_create_element
	     (_gnutls_get_pkix(), "PKIX1.KeyUsage", &c2)) != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	result = _asn1_strict_der_decode(&c2, ext->data, ext->size, NULL);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		asn1_delete_structure(&c2);
		return _gnutls_asn2err(result);
	}

	len = sizeof(str);
	result = asn1_read_value(c2, "", str, &len);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		asn1_delete_structure(&c2);
		return 0;
	}

	*key_usage = str[0] | (str[1] << 8);

	asn1_delete_structure(&c2);

	return 0;
}

/**
 * gnutls_x509_ext_export_key_usage:
 * @usage: an ORed sequence of the GNUTLS_KEY_* elements.
 * @ext: The DER-encoded extension data; must be freed using gnutls_free().
 *
 * This function will convert the keyUsage bit string to a DER
 * encoded PKIX extension. The @ext data will be allocated using
 * gnutls_malloc().
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.3.0
 **/
int gnutls_x509_ext_export_key_usage(unsigned int usage, gnutls_datum_t * ext)
{
	ASN1_TYPE c2 = ASN1_TYPE_EMPTY;
	int result;
	uint8_t str[2];

	result = asn1_create_element(_gnutls_get_pkix(), "PKIX1.KeyUsage", &c2);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	str[0] = usage & 0xff;
	str[1] = usage >> 8;

	/* Since KeyUsage is a BIT STRING, the input to asn1_write_value
	 * is the number of bits to be read. */
	result = asn1_write_value(c2, "", str, 9);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		asn1_delete_structure(&c2);
		return _gnutls_asn2err(result);
	}

	result = _gnutls_x509_der_encode(c2, "", ext, 0);

	asn1_delete_structure(&c2);

	if (result < 0) {
		gnutls_assert();
		return result;
	}

	return 0;
}

/**
 * gnutls_x509_ext_import_inhibit_anypolicy:
 * @ext: the DER encoded extension data
 * @skipcerts: will hold the number of certificates after which anypolicy is no longer acceptable.
 *
 * This function will return certificate's value of SkipCerts,
 * by reading the DER data of the Inhibit anyPolicy X.509 extension (2.5.29.54).
 *
 * The @skipcerts value is the number of additional certificates that
 * may appear in the path before the anyPolicy (%GNUTLS_X509_OID_POLICY_ANY)
 * is no longer acceptable.
 *
 * Returns: zero, or a negative error code in case of
 *   parsing error.  If the certificate does not contain the Inhibit anyPolicy
 *   extension %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE will be
 *   returned.
 *
 * Since: 3.6.0
 **/
int gnutls_x509_ext_import_inhibit_anypolicy(const gnutls_datum_t * ext,
				  unsigned int *skipcerts)
{
	int ret;

	ret = _gnutls_x509_read_der_uint(ext->data, ext->size, skipcerts);
	if (ret < 0) {
		gnutls_assert();
	}

	return ret;
}

/**
 * gnutls_x509_ext_export_inhibit_anypolicy:
 * @skipcerts: number of certificates after which anypolicy is no longer acceptable.
 * @ext: The DER-encoded extension data; must be freed using gnutls_free().
 *
 * This function will convert the @skipcerts value to a DER
 * encoded Inhibit AnyPolicy PKIX extension. The @ext data will be allocated using
 * gnutls_malloc().
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.6.0
 **/
int gnutls_x509_ext_export_inhibit_anypolicy(unsigned int skipcerts, gnutls_datum_t * ext)
{
	ASN1_TYPE c2 = ASN1_TYPE_EMPTY;
	int result, ret;

	result = asn1_create_element(_gnutls_get_gnutls_asn(), "GNUTLS.DSAPublicKey", &c2);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	ret = _gnutls_x509_write_uint32(c2, "", skipcerts);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = _gnutls_x509_der_encode(c2, "", ext, 0);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = 0;

 cleanup:
	asn1_delete_structure(&c2);

	return ret;
}

/**
 * gnutls_x509_ext_import_private_key_usage_period:
 * @ext: the DER encoded extension data
 * @activation: Will hold the activation time
 * @expiration: Will hold the expiration time
 *
 * This function will return the expiration and activation
 * times of the private key as written in the
 * PKIX extension 2.5.29.16.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.3.0
 **/
int gnutls_x509_ext_import_private_key_usage_period(const gnutls_datum_t * ext,
						 time_t * activation,
						 time_t * expiration)
{
	int result, ret;
	ASN1_TYPE c2 = ASN1_TYPE_EMPTY;

	result = asn1_create_element
	    (_gnutls_get_pkix(), "PKIX1.PrivateKeyUsagePeriod", &c2);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		ret = _gnutls_asn2err(result);
		goto cleanup;
	}

	result = _asn1_strict_der_decode(&c2, ext->data, ext->size, NULL);
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
 * gnutls_x509_ext_export_private_key_usage_period:
 * @activation: The activation time
 * @expiration: The expiration time
 * @ext: The DER-encoded extension data; must be freed using gnutls_free().
 *
 * This function will convert the periods provided to a private key
 * usage DER encoded extension (2.5.29.16).
 (
 * The @ext data will be allocated using
 * gnutls_malloc().
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.3.0
 **/
int gnutls_x509_ext_export_private_key_usage_period(time_t activation,
						 time_t expiration,
						 gnutls_datum_t * ext)
{
	int result;
	ASN1_TYPE c2 = ASN1_TYPE_EMPTY;

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

	result = _gnutls_x509_der_encode(c2, "", ext, 0);
	if (result < 0) {
		gnutls_assert();
		goto cleanup;
	}

 cleanup:
	asn1_delete_structure(&c2);

	return result;

}

/**
 * gnutls_x509_ext_import_basic_constraints:
 * @ext: the DER encoded extension data
 * @ca: will be non zero if the CA status is true
 * @pathlen: the path length constraint; will be set to -1 for no limit
 *
 * This function will return the CA status and path length constraint
 * as written in the PKIX extension 2.5.29.19.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.3.0
 **/
int gnutls_x509_ext_import_basic_constraints(const gnutls_datum_t * ext,
					  unsigned int *ca, int *pathlen)
{
	ASN1_TYPE c2 = ASN1_TYPE_EMPTY;
	char str[128]="";
	int len, result;

	if ((result = asn1_create_element
	     (_gnutls_get_pkix(), "PKIX1.BasicConstraints",
	      &c2)) != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	result = _asn1_strict_der_decode(&c2, ext->data, ext->size, NULL);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto cleanup;
	}

	if (pathlen) {
		result = _gnutls_x509_read_uint(c2, "pathLenConstraint",
						(unsigned int *)
						pathlen);
		if (result == GNUTLS_E_ASN1_ELEMENT_NOT_FOUND)
			*pathlen = -1;
		else if (result != GNUTLS_E_SUCCESS) {
			gnutls_assert();
			result = _gnutls_asn2err(result);
			goto cleanup;
		}
	}

	/* the default value of cA is false.
	 */
	len = sizeof(str) - 1;
	result = asn1_read_value(c2, "cA", str, &len);
	if (result == ASN1_SUCCESS && strcmp(str, "TRUE") == 0)
		*ca = 1;
	else
		*ca = 0;

	result = 0;
 cleanup:
	asn1_delete_structure(&c2);

	return result;

}

/**
 * gnutls_x509_ext_export_basic_constraints:
 * @ca: non-zero for a CA
 * @pathlen: The path length constraint (set to -1 for no constraint)
 * @ext: The DER-encoded extension data; must be freed using gnutls_free().
 *
 * This function will convert the parameters provided to a basic constraints
 * DER encoded extension (2.5.29.19).
 (
 * The @ext data will be allocated using
 * gnutls_malloc().
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.3.0
 **/
int gnutls_x509_ext_export_basic_constraints(unsigned int ca, int pathlen,
					  gnutls_datum_t * ext)
{
	ASN1_TYPE c2 = ASN1_TYPE_EMPTY;
	const char *str;
	int result;

	if (ca == 0)
		str = "FALSE";
	else
		str = "TRUE";

	result =
	    asn1_create_element(_gnutls_get_pkix(),
				"PKIX1.BasicConstraints", &c2);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto cleanup;
	}

	result = asn1_write_value(c2, "cA", str, 1);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto cleanup;
	}

	if (pathlen < 0) {
		result = asn1_write_value(c2, "pathLenConstraint", NULL, 0);
		if (result < 0)
			result = _gnutls_asn2err(result);
	} else
		result =
		    _gnutls_x509_write_uint32(c2, "pathLenConstraint", pathlen);
	if (result < 0) {
		gnutls_assert();
		goto cleanup;
	}

	result = _gnutls_x509_der_encode(c2, "", ext, 0);
	if (result < 0) {
		gnutls_assert();
		goto cleanup;
	}

	result = 0;

 cleanup:
	asn1_delete_structure(&c2);
	return result;

}

/**
 * gnutls_x509_ext_import_proxy:
 * @ext: the DER encoded extension data
 * @pathlen: pointer to output integer indicating path length (may be
 *   NULL), non-negative error codes indicate a present pCPathLenConstraint
 *   field and the actual value, -1 indicate that the field is absent.
 * @policyLanguage: output variable with OID of policy language
 * @policy: output variable with policy data
 * @sizeof_policy: output variable with size of policy data
 *
 * This function will return the information from a proxy certificate
 * extension. It reads the ProxyCertInfo X.509 extension (1.3.6.1.5.5.7.1.14).
 * The @policyLanguage and @policy values must be deinitialized using gnutls_free() after use.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.3.0
 **/
int gnutls_x509_ext_import_proxy(const gnutls_datum_t *ext, int *pathlen,
			         char **policyLanguage, char **policy,
			         size_t *sizeof_policy)
{
	ASN1_TYPE c2 = ASN1_TYPE_EMPTY;
	int result;
	gnutls_datum_t value1 = { NULL, 0 };
	gnutls_datum_t value2 = { NULL, 0 };

	if ((result = asn1_create_element
	     (_gnutls_get_pkix(), "PKIX1.ProxyCertInfo",
	      &c2)) != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	result = _asn1_strict_der_decode(&c2, ext->data, ext->size, NULL);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto cleanup;
	}

	if (pathlen) {
		result = _gnutls_x509_read_uint(c2, "pCPathLenConstraint",
						(unsigned int *)
						pathlen);
		if (result == GNUTLS_E_ASN1_ELEMENT_NOT_FOUND)
			*pathlen = -1;
		else if (result != GNUTLS_E_SUCCESS) {
			gnutls_assert();
			result = _gnutls_asn2err(result);
			goto cleanup;
		}
	}

	result = _gnutls_x509_read_value(c2, "proxyPolicy.policyLanguage",
					 &value1);
	if (result < 0) {
		gnutls_assert();
		goto cleanup;
	}

	result = _gnutls_x509_read_value(c2, "proxyPolicy.policy", &value2);
	if (result == GNUTLS_E_ASN1_ELEMENT_NOT_FOUND) {
		if (policy)
			*policy = NULL;
		if (sizeof_policy)
			*sizeof_policy = 0;
	} else if (result < 0) {
		gnutls_assert();
		goto cleanup;
	} else {
		if (policy) {
			*policy = (char *)value2.data;
			value2.data = NULL;
		}
		if (sizeof_policy)
			*sizeof_policy = value2.size;
	}

	if (policyLanguage) {
		*policyLanguage = (char *)value1.data;
		value1.data = NULL;
	}

	result = 0;
 cleanup:
	gnutls_free(value1.data);
	gnutls_free(value2.data);
	asn1_delete_structure(&c2);

	return result;
}

/**
 * gnutls_x509_ext_export_proxy:
 * @pathLenConstraint: A negative value will remove the path length constraint,
 *   while non-negative values will be set as the length of the pathLenConstraints field.
 * @policyLanguage: OID describing the language of @policy.
 * @policy: uint8_t byte array with policy language, can be %NULL
 * @sizeof_policy: size of @policy.
 * @ext: The DER-encoded extension data; must be freed using gnutls_free().
 *
 * This function will convert the parameters provided to a proxyCertInfo extension.
 *
 * The @ext data will be allocated using gnutls_malloc().
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.3.0
 **/
int gnutls_x509_ext_export_proxy(int pathLenConstraint, const char *policyLanguage,
			      const char *policy, size_t sizeof_policy,
			      gnutls_datum_t * ext)
{
	ASN1_TYPE c2 = ASN1_TYPE_EMPTY;
	int result;

	result = asn1_create_element(_gnutls_get_pkix(),
				     "PKIX1.ProxyCertInfo", &c2);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	if (pathLenConstraint < 0) {
		result = asn1_write_value(c2, "pCPathLenConstraint", NULL, 0);
		if (result != ASN1_SUCCESS) {
			gnutls_assert();
			result = _gnutls_asn2err(result);
			goto cleanup;
		}
	} else {
		result =
		    _gnutls_x509_write_uint32(c2, "pCPathLenConstraint",
					      pathLenConstraint);

		if (result < 0) {
			gnutls_assert();
			goto cleanup;
		}
	}

	result = asn1_write_value(c2, "proxyPolicy.policyLanguage",
				  policyLanguage, 1);
	if (result < 0) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto cleanup;
	}

	result = asn1_write_value(c2, "proxyPolicy.policy",
				  policy, sizeof_policy);
	if (result < 0) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto cleanup;
	}

	result = _gnutls_x509_der_encode(c2, "", ext, 0);
	if (result < 0) {
		gnutls_assert();
		goto cleanup;
	}

	result = 0;
 cleanup:
	asn1_delete_structure(&c2);
	return result;

}

static int decode_user_notice(const void *data, size_t size,
			      gnutls_datum_t * txt)
{
	ASN1_TYPE c2 = ASN1_TYPE_EMPTY;
	int ret, len;
	char choice_type[64];
	char name[128];
	gnutls_datum_t td = {NULL,0}, utd;

	ret = asn1_create_element(_gnutls_get_pkix(), "PKIX1.UserNotice", &c2);
	if (ret != ASN1_SUCCESS) {
		gnutls_assert();
		ret = GNUTLS_E_PARSING_ERROR;
		goto cleanup;
	}

	ret = _asn1_strict_der_decode(&c2, data, size, NULL);
	if (ret != ASN1_SUCCESS) {
		gnutls_assert();
		ret = GNUTLS_E_PARSING_ERROR;
		goto cleanup;
	}

	len = sizeof(choice_type);
	ret = asn1_read_value(c2, "explicitText", choice_type, &len);
	if (ret != ASN1_SUCCESS) {
		gnutls_assert();
		ret = GNUTLS_E_PARSING_ERROR;
		goto cleanup;
	}

	if (strcmp(choice_type, "utf8String") != 0
	    && strcmp(choice_type, "ia5String") != 0
	    && strcmp(choice_type, "bmpString") != 0
	    && strcmp(choice_type, "visibleString") != 0) {
		gnutls_assert();
		ret = GNUTLS_E_PARSING_ERROR;
		goto cleanup;
	}

	snprintf(name, sizeof(name), "explicitText.%s", choice_type);

	ret = _gnutls_x509_read_value(c2, name, &td);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	if (strcmp(choice_type, "bmpString") == 0) {	/* convert to UTF-8 */
		ret = _gnutls_ucs2_to_utf8(td.data, td.size, &utd, 1);
		_gnutls_free_datum(&td);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		td.data = utd.data;
		td.size = utd.size;
	} else {
		/* _gnutls_x509_read_value allows that */
		td.data[td.size] = 0;
	}

	txt->data = (void *)td.data;
	txt->size = td.size;
	ret = 0;

 cleanup:
	asn1_delete_structure(&c2);
	return ret;

}

struct gnutls_x509_policies_st {
	struct gnutls_x509_policy_st policy[MAX_ENTRIES];
	unsigned int size;
};

/**
 * gnutls_x509_policies_init:
 * @policies: The authority key ID
 *
 * This function will initialize an authority key ID type.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a negative error value.
 *
 * Since: 3.3.0
 **/
int gnutls_x509_policies_init(gnutls_x509_policies_t * policies)
{
	*policies = gnutls_calloc(1, sizeof(struct gnutls_x509_policies_st));
	if (*policies == NULL)
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

	return 0;
}

/**
 * gnutls_x509_policies_deinit:
 * @policies: The authority key identifier
 *
 * This function will deinitialize an authority key identifier type.
 *
 * Since: 3.3.0
 **/
void gnutls_x509_policies_deinit(gnutls_x509_policies_t policies)
{
	unsigned i;

	for (i = 0; i < policies->size; i++) {
		gnutls_x509_policy_release(&policies->policy[i]);
	}
	gnutls_free(policies);
}

/**
 * gnutls_x509_policies_get:
 * @policies: The policies
 * @seq: The index of the name to get
 * @policy: Will hold the policy
 *
 * This function will return a specific policy as stored in
 * the @policies type. The returned values should be treated as constant
 * and valid for the lifetime of @policies.
 *
 * The any policy OID is available as the %GNUTLS_X509_OID_POLICY_ANY macro.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE
 * if the index is out of bounds, otherwise a negative error value.
 *
 * Since: 3.3.0
 **/
int gnutls_x509_policies_get(gnutls_x509_policies_t policies,
			     unsigned int seq,
			     struct gnutls_x509_policy_st *policy)
{
	if (seq >= policies->size)
		return gnutls_assert_val(GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE);

	if (policy) {
		memcpy(policy, &policies->policy[seq],
		       sizeof(struct gnutls_x509_policy_st));
	}

	return 0;
}

void _gnutls_x509_policies_erase(gnutls_x509_policies_t policies,
				 unsigned int seq)
{
	if (seq >= policies->size)
		return;

	memset(&policies->policy[seq], 0, sizeof(struct gnutls_x509_policy_st));
}

/**
 * gnutls_x509_policies_set:
 * @policies: An initialized policies
 * @seq: The index of the name to get
 * @policy: Contains the policy to set
 *
 * This function will store the specified policy in
 * the provided @policies.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0), otherwise a negative error value.
 *
 * Since: 3.3.0
 **/
int gnutls_x509_policies_set(gnutls_x509_policies_t policies,
			     const struct gnutls_x509_policy_st *policy)
{
	unsigned i;

	if (policies->size + 1 > MAX_ENTRIES)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	policies->policy[policies->size].oid = gnutls_strdup(policy->oid);
	if (policies->policy[policies->size].oid == NULL)
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

	for (i = 0; i < policy->qualifiers; i++) {
		policies->policy[policies->size].qualifier[i].type =
		    policy->qualifier[i].type;
		policies->policy[policies->size].qualifier[i].size =
		    policy->qualifier[i].size;
		policies->policy[policies->size].qualifier[i].data =
		    gnutls_malloc(policy->qualifier[i].size + 1);
		if (policies->policy[policies->size].qualifier[i].data == NULL)
			return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);
		memcpy(policies->policy[policies->size].qualifier[i].data,
		       policy->qualifier[i].data, policy->qualifier[i].size);
		policies->policy[policies->size].qualifier[i].data[policy->
								   qualifier[i].
								   size] = 0;
	}

	policies->policy[policies->size].qualifiers = policy->qualifiers;
	policies->size++;

	return 0;
}

/**
 * gnutls_x509_ext_import_policies:
 * @ext: the DER encoded extension data
 * @policies: A pointer to an initialized policies.
 * @flags: should be zero
 *
 * This function will extract the certificate policy extension (2.5.29.32) 
 * and store it the provided policies.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a negative error value.
 *
 * Since: 3.3.0
 **/
int gnutls_x509_ext_import_policies(const gnutls_datum_t * ext,
				 gnutls_x509_policies_t policies,
				 unsigned int flags)
{
	ASN1_TYPE c2 = ASN1_TYPE_EMPTY;
	char tmpstr[128];
	char tmpoid[MAX_OID_SIZE];
	gnutls_datum_t tmpd = { NULL, 0 };
	int ret, len;
	unsigned i, j, current = 0;

	ret = asn1_create_element
	    (_gnutls_get_pkix(), "PKIX1.certificatePolicies", &c2);
	if (ret != ASN1_SUCCESS) {
		gnutls_assert();
		ret = _gnutls_asn2err(ret);
		goto cleanup;
	}

	ret = _asn1_strict_der_decode(&c2, ext->data, ext->size, NULL);
	if (ret != ASN1_SUCCESS) {
		gnutls_assert();
		ret = _gnutls_asn2err(ret);
		goto cleanup;
	}

	for (j = 0;; j++) {
		if (j >= MAX_ENTRIES)
			break;

		memset(&policies->policy[j], 0,
		       sizeof(struct gnutls_x509_policy_st));

		/* create a string like "?1"
		 */
		snprintf(tmpstr, sizeof(tmpstr), "?%u.policyIdentifier", j + 1);
		current = j+1;

		ret = _gnutls_x509_read_value(c2, tmpstr, &tmpd);
		if (ret == GNUTLS_E_ASN1_ELEMENT_NOT_FOUND)
			break;

		if (ret < 0) {
			gnutls_assert();
			goto full_cleanup;
		}

		policies->policy[j].oid = (void *)tmpd.data;
		tmpd.data = NULL;

		for (i = 0; i < GNUTLS_MAX_QUALIFIERS; i++) {
			gnutls_datum_t td;

			snprintf(tmpstr, sizeof(tmpstr),
				 "?%u.policyQualifiers.?%u.policyQualifierId",
				 j + 1, i + 1);

			len = sizeof(tmpoid);
			ret = asn1_read_value(c2, tmpstr, tmpoid, &len);

			if (ret == ASN1_ELEMENT_NOT_FOUND)
				break;	/* finished */

			if (ret != ASN1_SUCCESS) {
				gnutls_assert();
				ret = _gnutls_asn2err(ret);
				goto full_cleanup;
			}

			if (strcmp(tmpoid, "1.3.6.1.5.5.7.2.1") == 0) {
				snprintf(tmpstr, sizeof(tmpstr),
					 "?%u.policyQualifiers.?%u.qualifier",
					 j + 1, i + 1);

				ret =
				    _gnutls_x509_read_string(c2, tmpstr, &td,
							     ASN1_ETYPE_IA5_STRING, 0);
				if (ret < 0) {
					gnutls_assert();
					goto full_cleanup;
				}

				policies->policy[j].qualifier[i].data =
				    (void *)td.data;
				policies->policy[j].qualifier[i].size = td.size;
				td.data = NULL;
				policies->policy[j].qualifier[i].type =
				    GNUTLS_X509_QUALIFIER_URI;
			} else if (strcmp(tmpoid, "1.3.6.1.5.5.7.2.2") == 0) {
				gnutls_datum_t txt = {NULL, 0};

				snprintf(tmpstr, sizeof(tmpstr),
					 "?%u.policyQualifiers.?%u.qualifier",
					 j + 1, i + 1);

				ret = _gnutls_x509_read_value(c2, tmpstr, &td);
				if (ret < 0) {
					gnutls_assert();
					goto full_cleanup;
				}

				ret =
				    decode_user_notice(td.data, td.size, &txt);
				gnutls_free(td.data);

				if (ret < 0) {
					gnutls_assert();
					goto full_cleanup;
				}

				policies->policy[j].qualifier[i].data =
				    (void *)txt.data;
				policies->policy[j].qualifier[i].size =
				    txt.size;
				policies->policy[j].qualifier[i].type =
				    GNUTLS_X509_QUALIFIER_NOTICE;
			} else
				policies->policy[j].qualifier[i].type =
				    GNUTLS_X509_QUALIFIER_UNKNOWN;

			policies->policy[j].qualifiers++;
		}

	}

	policies->size = j;

	ret = 0;
	goto cleanup;

 full_cleanup:
	for (j = 0; j < current; j++)
		gnutls_x509_policy_release(&policies->policy[j]);

 cleanup:
	_gnutls_free_datum(&tmpd);
	asn1_delete_structure(&c2);
	return ret;

}

static int encode_user_notice(const gnutls_datum_t * txt,
			      gnutls_datum_t * der_data)
{
	int result;
	ASN1_TYPE c2 = ASN1_TYPE_EMPTY;

	if ((result =
	     asn1_create_element(_gnutls_get_pkix(),
				 "PKIX1.UserNotice", &c2)) != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto error;
	}

	/* delete noticeRef */
	result = asn1_write_value(c2, "noticeRef", NULL, 0);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto error;
	}

	result = asn1_write_value(c2, "explicitText", "utf8String", 1);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto error;
	}

	result =
	    asn1_write_value(c2, "explicitText.utf8String", txt->data,
			     txt->size);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto error;
	}

	result = _gnutls_x509_der_encode(c2, "", der_data, 0);
	if (result < 0) {
		gnutls_assert();
		goto error;
	}

	result = 0;

 error:
	asn1_delete_structure(&c2);
	return result;

}

/**
 * gnutls_x509_ext_export_policies:
 * @policies: A pointer to an initialized policies.
 * @ext: The DER-encoded extension data; must be freed using gnutls_free().
 *
 * This function will convert the provided policies, to a certificate policy
 * DER encoded extension (2.5.29.32).
 *
 * The @ext data will be allocated using gnutls_malloc().
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a negative error value.
 *
 * Since: 3.3.0
 **/
int gnutls_x509_ext_export_policies(gnutls_x509_policies_t policies,
				 gnutls_datum_t * ext)
{
	int result;
	unsigned i, j;
	gnutls_datum_t der_data = {NULL, 0}, tmpd;
	ASN1_TYPE c2 = ASN1_TYPE_EMPTY;
	const char *oid;

	result =
	    asn1_create_element(_gnutls_get_pkix(),
				"PKIX1.certificatePolicies", &c2);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto cleanup;
	}

	for (j = 0; j < policies->size; j++) {
		/* 1. write a new policy */
		result = asn1_write_value(c2, "", "NEW", 1);
		if (result != ASN1_SUCCESS) {
			gnutls_assert();
			result = _gnutls_asn2err(result);
			goto cleanup;
		}

		/* 2. Add the OID.
		 */
		result =
		    asn1_write_value(c2, "?LAST.policyIdentifier",
				     policies->policy[j].oid, 1);
		if (result != ASN1_SUCCESS) {
			gnutls_assert();
			result = _gnutls_asn2err(result);
			goto cleanup;
		}

		for (i = 0;
		     i < MIN(policies->policy[j].qualifiers,
			     GNUTLS_MAX_QUALIFIERS); i++) {
			result =
			    asn1_write_value(c2, "?LAST.policyQualifiers",
					     "NEW", 1);
			if (result != ASN1_SUCCESS) {
				gnutls_assert();
				result = _gnutls_asn2err(result);
				goto cleanup;
			}

			if (policies->policy[j].qualifier[i].type ==
			    GNUTLS_X509_QUALIFIER_URI)
				oid = "1.3.6.1.5.5.7.2.1";
			else if (policies->policy[j].qualifier[i].type ==
				 GNUTLS_X509_QUALIFIER_NOTICE)
				oid = "1.3.6.1.5.5.7.2.2";
			else {
				result =
				    gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);
				goto cleanup;
			}

			result =
			    asn1_write_value(c2,
					     "?LAST.policyQualifiers.?LAST.policyQualifierId",
					     oid, 1);
			if (result != ASN1_SUCCESS) {
				gnutls_assert();
				result = _gnutls_asn2err(result);
				goto cleanup;
			}

			if (policies->policy[j].qualifier[i].type ==
			    GNUTLS_X509_QUALIFIER_URI) {
				tmpd.data =
				    (void *)policies->policy[j].qualifier[i].
				    data;
				tmpd.size =
				    policies->policy[j].qualifier[i].size;
				result =
				    _gnutls_x509_write_string(c2,
							      "?LAST.policyQualifiers.?LAST.qualifier",
							      &tmpd,
							      ASN1_ETYPE_IA5_STRING);
				if (result < 0) {
					gnutls_assert();
					goto cleanup;
				}
			} else if (policies->policy[j].qualifier[i].type ==
				   GNUTLS_X509_QUALIFIER_NOTICE) {
				tmpd.data =
				    (void *)policies->policy[j].qualifier[i].
				    data;
				tmpd.size =
				    policies->policy[j].qualifier[i].size;

				if (tmpd.size > 200) {
					gnutls_assert();
					result = GNUTLS_E_INVALID_REQUEST;
					goto cleanup;
				}

				result = encode_user_notice(&tmpd, &der_data);
				if (result < 0) {
					gnutls_assert();
					goto cleanup;
				}

				result =
				    _gnutls_x509_write_value(c2,
							     "?LAST.policyQualifiers.?LAST.qualifier",
							     &der_data);
				_gnutls_free_datum(&der_data);
				if (result < 0) {
					gnutls_assert();
					goto cleanup;
				}
			}
		}
	}

	result = _gnutls_x509_der_encode(c2, "", ext, 0);
	if (result < 0) {
		gnutls_assert();
		goto cleanup;
	}

 cleanup:
	asn1_delete_structure(&c2);

	return result;
}

struct crl_dist_point_st {
	unsigned int type;
	gnutls_datum_t san;
	unsigned int reasons;
};

struct gnutls_x509_crl_dist_points_st {
	struct crl_dist_point_st *points;
	unsigned int size;
};

/**
 * gnutls_x509_crl_dist_points_init:
 * @cdp: The CRL distribution points
 *
 * This function will initialize a CRL distribution points type.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a negative error value.
 *
 * Since: 3.3.0
 **/
int gnutls_x509_crl_dist_points_init(gnutls_x509_crl_dist_points_t * cdp)
{
	*cdp = gnutls_calloc(1, sizeof(struct gnutls_x509_crl_dist_points_st));
	if (*cdp == NULL)
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

	return 0;
}

/**
 * gnutls_x509_crl_dist_points_deinit:
 * @cdp: The CRL distribution points
 *
 * This function will deinitialize a CRL distribution points type.
 *
 * Since: 3.3.0
 **/
void gnutls_x509_crl_dist_points_deinit(gnutls_x509_crl_dist_points_t cdp)
{
	unsigned i;

	for (i = 0; i < cdp->size; i++) {
		gnutls_free(cdp->points[i].san.data);
	}
	gnutls_free(cdp->points);
	gnutls_free(cdp);
}

/**
 * gnutls_x509_crl_dist_points_get:
 * @cdp: The CRL distribution points
 * @seq: specifies the sequence number of the distribution point (0 for the first one, 1 for the second etc.)
 * @type: The name type of the corresponding name (gnutls_x509_subject_alt_name_t)
 * @san: The distribution point names (to be treated as constant)
 * @reasons: Revocation reasons. An ORed sequence of flags from %gnutls_x509_crl_reason_flags_t.
 *
 * This function retrieves the individual CRL distribution points (2.5.29.31),
 * contained in provided type. 
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE
 * if the index is out of bounds, otherwise a negative error value.
 **/

int gnutls_x509_crl_dist_points_get(gnutls_x509_crl_dist_points_t cdp,
				    unsigned int seq, unsigned int *type,
				    gnutls_datum_t * san, unsigned int *reasons)
{
	if (seq >= cdp->size)
		return gnutls_assert_val(GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE);

	if (reasons)
		*reasons = cdp->points[seq].reasons;

	if (type)
		*type = cdp->points[seq].type;

	if (san) {
		san->data = cdp->points[seq].san.data;
		san->size = cdp->points[seq].san.size;
	}

	return 0;
}

static
int crl_dist_points_set(gnutls_x509_crl_dist_points_t cdp,
			gnutls_x509_subject_alt_name_t type,
			const gnutls_datum_t * san, unsigned int reasons)
{
	void *tmp;

	/* new dist point */
	tmp =
	    gnutls_realloc(cdp->points,
			   (cdp->size + 1) * sizeof(cdp->points[0]));
	if (tmp == NULL) {
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);
	}
	cdp->points = tmp;

	cdp->points[cdp->size].type = type;
	cdp->points[cdp->size].san.data = san->data;
	cdp->points[cdp->size].san.size = san->size;
	cdp->points[cdp->size].reasons = reasons;

	cdp->size++;
	return 0;

}

/**
 * gnutls_x509_crl_dist_points_set:
 * @cdp: The CRL distribution points
 * @type: The type of the name (of %gnutls_subject_alt_names_t)
 * @san: The point name data
 * @reasons: Revocation reasons. An ORed sequence of flags from %gnutls_x509_crl_reason_flags_t.
 *
 * This function will store the specified CRL distribution point value
 * the @cdp type.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0), otherwise a negative error value.
 *
 * Since: 3.3.0
 **/
int gnutls_x509_crl_dist_points_set(gnutls_x509_crl_dist_points_t cdp,
				    gnutls_x509_subject_alt_name_t type,
				    const gnutls_datum_t * san,
				    unsigned int reasons)
{
	int ret;
	gnutls_datum_t t_san;

	ret = _gnutls_set_datum(&t_san, san->data, san->size);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = crl_dist_points_set(cdp, type, &t_san, reasons);
	if (ret < 0) {
		gnutls_free(t_san.data);
		return gnutls_assert_val(ret);
	}

	return 0;
}

/**
 * gnutls_x509_ext_import_crl_dist_points:
 * @ext: the DER encoded extension data
 * @cdp: A pointer to an initialized CRL distribution points.
 * @flags: should be zero
 *
 * This function will extract the CRL distribution points extension (2.5.29.31) 
 * and store it into the provided type.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a negative error value.
 *
 * Since: 3.3.0
 **/
int gnutls_x509_ext_import_crl_dist_points(const gnutls_datum_t * ext,
					gnutls_x509_crl_dist_points_t cdp,
					unsigned int flags)
{
	int result;
	ASN1_TYPE c2 = ASN1_TYPE_EMPTY;
	char name[MAX_NAME_SIZE];
	int len, ret;
	uint8_t reasons[2];
	unsigned i, type, rflags, j;
	gnutls_datum_t san = {NULL, 0};

	result = asn1_create_element
	    (_gnutls_get_pkix(), "PKIX1.CRLDistributionPoints", &c2);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	result = _asn1_strict_der_decode(&c2, ext->data, ext->size, NULL);

	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		ret = _gnutls_asn2err(result);
		goto cleanup;
	}

	/* Return the different names from the first CRLDistr. point.
	 * The whole thing is a mess.
	 */

	i = 0;
	do {
		snprintf(name, sizeof(name), "?%u.reasons", (unsigned)i + 1);

		len = sizeof(reasons);
		result = asn1_read_value(c2, name, reasons, &len);

		if (result != ASN1_VALUE_NOT_FOUND &&
		    result != ASN1_ELEMENT_NOT_FOUND &&
		    result != ASN1_SUCCESS) {
			gnutls_assert();
			ret = _gnutls_asn2err(result);
			break;
		}

		if (result == ASN1_VALUE_NOT_FOUND
		    || result == ASN1_ELEMENT_NOT_FOUND)
			rflags = 0;
		else
			rflags = reasons[0] | (reasons[1] << 8);

		snprintf(name, sizeof(name),
			 "?%u.distributionPoint.fullName", (unsigned)i + 1);

		j = 0;
		do {
			san.data = NULL;
			san.size = 0;

			ret =
			    _gnutls_parse_general_name2(c2, name, j, &san,
							&type, 0);
			if (j > 0
			    && ret == GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE) {
				ret = 0;
				break;
			}
			if (ret < 0)
				break;

			ret = crl_dist_points_set(cdp, type, &san, rflags);
			if (ret < 0)
				break;
			san.data = NULL; /* it is now in cdp */

			j++;
		} while (ret >= 0);

		i++;
	} while (ret >= 0);

	if (ret < 0 && ret != GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE) {
		gnutls_assert();
		gnutls_free(san.data);
		goto cleanup;
	}

	ret = 0;
 cleanup:
	asn1_delete_structure(&c2);
	return ret;
}

/**
 * gnutls_x509_ext_export_crl_dist_points:
 * @cdp: A pointer to an initialized CRL distribution points.
 * @ext: The DER-encoded extension data; must be freed using gnutls_free().
 *
 * This function will convert the provided policies, to a certificate policy
 * DER encoded extension (2.5.29.31).
 *
 * The @ext data will be allocated using gnutls_malloc().
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a negative error value.
 *
 * Since: 3.3.0
 **/
int gnutls_x509_ext_export_crl_dist_points(gnutls_x509_crl_dist_points_t cdp,
					gnutls_datum_t * ext)
{
	ASN1_TYPE c2 = ASN1_TYPE_EMPTY;
	int result;
	uint8_t reasons[2];
	unsigned i;

	result =
	    asn1_create_element(_gnutls_get_pkix(),
				"PKIX1.CRLDistributionPoints", &c2);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		result = _gnutls_asn2err(result);
		goto cleanup;
	}

	for (i = 0; i < cdp->size; i++) {

		if (i == 0
		    || cdp->points[i].reasons != cdp->points[i - 1].reasons) {
			result = asn1_write_value(c2, "", "NEW", 1);
			if (result != ASN1_SUCCESS) {
				gnutls_assert();
				result = _gnutls_asn2err(result);
				goto cleanup;
			}

			if (cdp->points[i].reasons) {
				reasons[0] = cdp->points[i].reasons & 0xff;
				reasons[1] = cdp->points[i].reasons >> 8;

				result =
				    asn1_write_value(c2, "?LAST.reasons",
						     reasons, 2);
			} else {
				result =
				    asn1_write_value(c2, "?LAST.reasons", NULL,
						     0);
			}

			if (result != ASN1_SUCCESS) {
				gnutls_assert();
				result = _gnutls_asn2err(result);
				goto cleanup;
			}

			result =
			    asn1_write_value(c2, "?LAST.cRLIssuer", NULL, 0);
			if (result != ASN1_SUCCESS) {
				gnutls_assert();
				result = _gnutls_asn2err(result);
				goto cleanup;
			}
			/* When used as type CHOICE.
			 */
			result =
			    asn1_write_value(c2, "?LAST.distributionPoint",
					     "fullName", 1);
			if (result != ASN1_SUCCESS) {
				gnutls_assert();
				result = _gnutls_asn2err(result);
				goto cleanup;
			}
		}

		result =
		    _gnutls_write_new_general_name(c2,
						   "?LAST.distributionPoint.fullName",
						   cdp->points[i].type,
						   cdp->points[i].san.data,
						   cdp->points[i].san.size);
		if (result < 0) {
			gnutls_assert();
			goto cleanup;
		}
	}

	result = _gnutls_x509_der_encode(c2, "", ext, 0);
	if (result < 0) {
		gnutls_assert();
		goto cleanup;
	}

	result = 0;

 cleanup:
	asn1_delete_structure(&c2);

	return result;

}

struct gnutls_x509_aia_st {
	struct {
		gnutls_datum_t oid;
		unsigned int san_type;
		gnutls_datum_t san;
	} *aia;
	unsigned int size;
};

/**
 * gnutls_x509_aia_init:
 * @aia: The authority info access
 *
 * This function will initialize an authority info access type.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a negative error value.
 *
 * Since: 3.3.0
 **/
int gnutls_x509_aia_init(gnutls_x509_aia_t * aia)
{
	*aia = gnutls_calloc(1, sizeof(struct gnutls_x509_aia_st));
	if (*aia == NULL)
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

	return 0;
}

/**
 * gnutls_x509_aia_deinit:
 * @aia: The authority info access
 *
 * This function will deinitialize an authority info access type.
 *
 * Since: 3.3.0
 **/
void gnutls_x509_aia_deinit(gnutls_x509_aia_t aia)
{
	unsigned i;

	for (i = 0; i < aia->size; i++) {
		gnutls_free(aia->aia[i].san.data);
		gnutls_free(aia->aia[i].oid.data);
	}
	gnutls_free(aia->aia);
	gnutls_free(aia);
}

/**
 * gnutls_x509_aia_get:
 * @aia: The authority info access
 * @seq: specifies the sequence number of the access descriptor (0 for the first one, 1 for the second etc.)
 * @oid: the type of available data; to be treated as constant.
 * @san_type: Will hold the type of the name of %gnutls_subject_alt_names_t (may be null).
 * @san: the access location name; to be treated as constant (may be null).
 *
 * This function reads from the Authority Information Access type.
 *
 * The @seq input parameter is used to indicate which member of the
 * sequence the caller is interested in.  The first member is 0, the
 * second member 1 and so on.  When the @seq value is out of bounds,
 * %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE is returned.
 *
 * Typically @oid is %GNUTLS_OID_AD_CAISSUERS or %GNUTLS_OID_AD_OCSP.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a negative error value.
 *
 * Since: 3.3.0
 **/
int gnutls_x509_aia_get(gnutls_x509_aia_t aia, unsigned int seq,
			gnutls_datum_t *oid,
			unsigned *san_type,
			gnutls_datum_t *san)
{
	if (seq >= aia->size)
		return gnutls_assert_val(GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE);

	if (san_type)
		*san_type = aia->aia[seq].san_type;
	if (san) {
		san->data = aia->aia[seq].san.data;
		san->size = aia->aia[seq].san.size;
	}

	if (oid) {
		oid->data = aia->aia[seq].oid.data;
		oid->size = aia->aia[seq].oid.size;
	}

	return 0;
}

int _gnutls_alt_name_process(gnutls_datum_t *out, unsigned type, const gnutls_datum_t *san, unsigned raw)
{
	int ret;
	if (type == GNUTLS_SAN_DNSNAME && !raw) {
		ret = gnutls_idna_map((char*)san->data, san->size, out, 0);
		if (ret < 0) {
			return gnutls_assert_val(ret);
		}
	} else if (type == GNUTLS_SAN_RFC822NAME && !raw) {
		ret = _gnutls_idna_email_map((char*)san->data, san->size, out);
		if (ret < 0) {
			return gnutls_assert_val(ret);
		}
	} else if (type == GNUTLS_SAN_URI && !raw) {
		if (!_gnutls_str_is_print((char*)san->data, san->size)) {
			_gnutls_debug_log("non-ASCII URIs are not supported\n");
			return gnutls_assert_val(GNUTLS_E_UNIMPLEMENTED_FEATURE);
		} else {
			ret = _gnutls_set_strdatum(out, san->data, san->size);
			if (ret < 0)
				return gnutls_assert_val(ret);
		}
	} else {
		ret = _gnutls_set_strdatum(out, san->data, san->size);
		if (ret < 0)
			return gnutls_assert_val(ret);
	}

	return 0;
}

/**
 * gnutls_x509_aia_set:
 * @aia: The authority info access
 * @oid: the type of data.
 * @san_type: The type of the name (of %gnutls_subject_alt_names_t)
 * @san: The alternative name data
 * @othername_oid: The object identifier if @san_type is %GNUTLS_SAN_OTHERNAME
 *
 * This function will store the specified alternative name in
 * the @aia type. 
 *
 * Typically the value for @oid should be %GNUTLS_OID_AD_OCSP, or
 * %GNUTLS_OID_AD_CAISSUERS.
 *
 * Since version 3.5.7 the %GNUTLS_SAN_RFC822NAME, and %GNUTLS_SAN_DNSNAME,
 * are converted to ACE format when necessary.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0), otherwise a negative error value.
 *
 * Since: 3.3.0
 **/
int gnutls_x509_aia_set(gnutls_x509_aia_t aia,
			const char *oid,
			unsigned san_type,
			const gnutls_datum_t * san)
{
	int ret;
	void *tmp;
	unsigned indx;

	tmp = gnutls_realloc(aia->aia, (aia->size + 1) * sizeof(aia->aia[0]));
	if (tmp == NULL) {
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);
	}
	aia->aia = tmp;
	indx = aia->size;
	
	aia->aia[indx].san_type = san_type;
	if (oid) {
		aia->aia[indx].oid.data = (void*)gnutls_strdup(oid);
		aia->aia[indx].oid.size = strlen(oid);
	} else {
		aia->aia[indx].oid.data = NULL;
		aia->aia[indx].oid.size = 0;
	}

	ret = _gnutls_alt_name_process(&aia->aia[indx].san, san_type, san, 0);
	if (ret < 0)
		return gnutls_assert_val(ret);

	aia->size++;

	return 0;	
}


static int parse_aia(ASN1_TYPE c2, gnutls_x509_aia_t aia)
{
	int len;
	char nptr[MAX_NAME_SIZE];
	int ret, result;
	char tmpoid[MAX_OID_SIZE];
	void * tmp;
	unsigned i, indx;

	for (i = 1;; i++) {
		snprintf(nptr, sizeof(nptr), "?%u.accessMethod", i);

		len = sizeof(tmpoid);
		result = asn1_read_value(c2, nptr, tmpoid, &len);
		if (result == ASN1_VALUE_NOT_FOUND
		    || result == ASN1_ELEMENT_NOT_FOUND) {
			ret = GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
			break;
		}

		if (result != ASN1_SUCCESS) {
			gnutls_assert();
			return _gnutls_asn2err(result);
		}

		indx = aia->size;
		tmp = gnutls_realloc(aia->aia, (aia->size + 1) * sizeof(aia->aia[0]));
		if (tmp == NULL) {
			return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);
		}
		aia->aia = tmp;

		snprintf(nptr, sizeof(nptr), "?%u.accessLocation", i);


		ret = _gnutls_parse_general_name2(c2, nptr, -1, &aia->aia[indx].san, 
			&aia->aia[indx].san_type, 0);
		if (ret < 0)
			break;

		/* we do the strdup after parsing to avoid a memory leak */
		aia->aia[indx].oid.data = (void*)gnutls_strdup(tmpoid);
		aia->aia[indx].oid.size = strlen(tmpoid);

		aia->size++;

		if (aia->aia[indx].oid.data == NULL) {
			gnutls_assert();
			return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);
		}
	}
	
	if (ret < 0 && ret != GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE) {
		return ret;
	}

	return 0;
}

/**
 * gnutls_x509_ext_import_aia:
 * @ext: The DER-encoded extension data
 * @aia: The authority info access
 * @flags: should be zero
 *
 * This function extracts the Authority Information Access (AIA)
 * extension from the provided DER-encoded data; see RFC 5280 section 4.2.2.1 
 * for more information on the extension.  The
 * AIA extension holds a sequence of AccessDescription (AD) data.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a negative error value.
 *
 * Since: 3.3.0
 **/
int gnutls_x509_ext_import_aia(const gnutls_datum_t * ext,
					      gnutls_x509_aia_t aia,
					      unsigned int flags)
{
	int ret;
	ASN1_TYPE c2 = ASN1_TYPE_EMPTY;

	if (ext->size == 0 || ext->data == NULL) {
		gnutls_assert();
		return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
	}

	ret = asn1_create_element(_gnutls_get_pkix(),
				  "PKIX1.AuthorityInfoAccessSyntax", &c2);
	if (ret != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(ret);
	}

	ret = _asn1_strict_der_decode(&c2, ext->data, ext->size, NULL);
	if (ret != ASN1_SUCCESS) {
		gnutls_assert();
		ret = _gnutls_asn2err(ret);
		goto cleanup;
	}

	ret = parse_aia(c2, aia);
	if (ret < 0) {
		gnutls_assert();
	}

 cleanup:
	asn1_delete_structure(&c2);

	return ret;

}

/**
 * gnutls_x509_ext_export_aia:
 * @aia: The authority info access
 * @ext: The DER-encoded extension data; must be freed using gnutls_free().
 *
 * This function will DER encode the Authority Information Access (AIA)
 * extension; see RFC 5280 section 4.2.2.1 for more information on the
 * extension.  
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.3.0
 **/
int gnutls_x509_ext_export_aia(gnutls_x509_aia_t aia,
					      gnutls_datum_t * ext)
{
	int ret, result;
	ASN1_TYPE c2 = ASN1_TYPE_EMPTY;
	unsigned int i;

	ret = asn1_create_element(_gnutls_get_pkix(),
				  "PKIX1.AuthorityInfoAccessSyntax", &c2);
	if (ret != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(ret);
	}

	/* 1. create a new element.
	 */
	for (i=0;i<aia->size;i++) {
		result = asn1_write_value(c2, "", "NEW", 1);
		if (result != ASN1_SUCCESS) {
			gnutls_assert();
			ret = _gnutls_asn2err(result);
			goto cleanup;
		}

		/* 2. Add the OID.
		 */
		result = asn1_write_value(c2, "?LAST.accessMethod", aia->aia[i].oid.data, 1);
		if (result != ASN1_SUCCESS) {
			gnutls_assert();
			ret = _gnutls_asn2err(result);
			goto cleanup;
		}

		ret =
		    _gnutls_write_general_name(c2,
						   "?LAST.accessLocation",
						   aia->aia[i].san_type,
						   aia->aia[i].san.data,
						   aia->aia[i].san.size);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}
	}

	ret = _gnutls_x509_der_encode(c2, "", ext, 0);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

 cleanup:
	asn1_delete_structure(&c2);

	return ret;
}


struct gnutls_x509_key_purposes_st {
	gnutls_datum_t oid[MAX_ENTRIES];
	unsigned int size;
};

/**
 * gnutls_subject_alt_names_init:
 * @p: The key purposes
 *
 * This function will initialize an alternative names type.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a negative error value.
 *
 * Since: 3.3.0
 **/
int gnutls_x509_key_purpose_init(gnutls_x509_key_purposes_t * p)
{
	*p = gnutls_calloc(1, sizeof(struct gnutls_x509_key_purposes_st));
	if (*p == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	return 0;
}

static void key_purposes_deinit(gnutls_x509_key_purposes_t p)
{
	unsigned int i;

	for (i = 0; i < p->size; i++) {
		gnutls_free(p->oid[i].data);
	}
}

/**
 * gnutls_x509_key_purpose_deinit:
 * @p: The key purposes
 *
 * This function will deinitialize a key purposes type.
 *
 * Since: 3.3.0
 **/
void gnutls_x509_key_purpose_deinit(gnutls_x509_key_purposes_t p)
{
	key_purposes_deinit(p);
	gnutls_free(p);
}

/**
 * gnutls_x509_key_purpose_set:
 * @p: The key purposes
 * @oid: The object identifier of the key purpose
 *
 * This function will store the specified key purpose in the
 * purposes.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0), otherwise a negative error value.
 *
 * Since: 3.3.0
 **/
int gnutls_x509_key_purpose_set(gnutls_x509_key_purposes_t p, const char *oid)
{
	if (p->size + 1 > MAX_ENTRIES)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	p->oid[p->size].data = (void*)gnutls_strdup(oid);
	if (p->oid[p->size].data == NULL)
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

	p->oid[p->size].size = strlen(oid);
	p->size++;

	return 0;
}

/**
 * gnutls_x509_key_purpose_get:
 * @p: The key purposes
 * @idx: The index of the key purpose to retrieve
 * @oid: Will hold the object identifier of the key purpose (to be treated as constant)
 *
 * This function will retrieve the specified by the index key purpose in the
 * purposes type. The object identifier will be a null terminated string.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE
 * if the index is out of bounds, otherwise a negative error value.
 *
 * Since: 3.3.0
 **/
int gnutls_x509_key_purpose_get(gnutls_x509_key_purposes_t p, unsigned idx, gnutls_datum_t *oid)
{
	if (idx >= p->size)
		return gnutls_assert_val(GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE);

	oid->data = p->oid[idx].data;
	oid->size = p->oid[idx].size;

	return 0;
}

/**
 * gnutls_x509_ext_import_key_purposes:
 * @ext: The DER-encoded extension data
 * @p: The key purposes
 * @flags: should be zero
 *
 * This function will extract the key purposes in the provided DER-encoded
 * ExtKeyUsageSyntax PKIX extension, to a %gnutls_x509_key_purposes_t type. 
 * The data must be initialized.
 * 
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a negative error value.
 *
 * Since: 3.3.0
 **/
int gnutls_x509_ext_import_key_purposes(const gnutls_datum_t * ext,
				     gnutls_x509_key_purposes_t p,
				     unsigned int flags)
{
	char tmpstr[MAX_NAME_SIZE];
	int result, ret;
	ASN1_TYPE c2 = ASN1_TYPE_EMPTY;
	gnutls_datum_t oid = {NULL, 0};
	unsigned i;

	result = asn1_create_element
	    (_gnutls_get_pkix(), "PKIX1.ExtKeyUsageSyntax", &c2);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	result = _asn1_strict_der_decode(&c2, ext->data, ext->size, NULL);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		ret = _gnutls_asn2err(result);
		goto cleanup;
	}

	key_purposes_deinit(p);
	i = 0;
	p->size = 0;

	for (;i<MAX_ENTRIES;i++) {
		/* create a string like "?1"
		 */
		snprintf(tmpstr, sizeof(tmpstr), "?%u", i+1);

		ret = _gnutls_x509_read_value(c2, tmpstr, &oid);
		if (ret == GNUTLS_E_ASN1_ELEMENT_NOT_FOUND) {
			break;
		}

		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		p->oid[i].data = oid.data;
		p->oid[i].size = oid.size;

		oid.data = NULL;
		oid.size = 0;
		p->size++;
	}

	ret = 0;
 cleanup:
	gnutls_free(oid.data);
	asn1_delete_structure(&c2);

	return ret;

}

/**
 * gnutls_x509_ext_export_key_purposes:
 * @p: The key purposes
 * @ext: The DER-encoded extension data; must be freed using gnutls_free().
 *
 * This function will convert the key purposes type to a
 * DER-encoded PKIX ExtKeyUsageSyntax (2.5.29.37) extension. The output data in 
 * @ext will be allocated using gnutls_malloc().
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a negative error value.
 *
 * Since: 3.3.0
 **/
int gnutls_x509_ext_export_key_purposes(gnutls_x509_key_purposes_t p,
				     gnutls_datum_t * ext)
{
	int result, ret;
	ASN1_TYPE c2 = ASN1_TYPE_EMPTY;
	unsigned i;

	result = asn1_create_element
	    (_gnutls_get_pkix(), "PKIX1.ExtKeyUsageSyntax", &c2);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	/* generate the extension.
	 */
	for (i=0;i<p->size;i++) {
		/* 1. create a new element.
		 */
		result = asn1_write_value(c2, "", "NEW", 1);
		if (result != ASN1_SUCCESS) {
			gnutls_assert();
			ret = _gnutls_asn2err(result);
			goto cleanup;
		}

		/* 2. Add the OID.
		 */
		result = asn1_write_value(c2, "?LAST", p->oid[i].data, 1);
		if (result != ASN1_SUCCESS) {
			gnutls_assert();
			ret = _gnutls_asn2err(result);
			goto cleanup;
		}
	}

	ret = _gnutls_x509_der_encode(c2, "", ext, 0);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = 0;

 cleanup:
	asn1_delete_structure(&c2);
	return ret;
}

/**
 * gnutls_ext_deinit:
 * @ext: The extensions structure
 *
 * This function will deinitialize an extensions structure.
 *
 * Since: 3.3.8
 **/
void gnutls_x509_ext_deinit(gnutls_x509_ext_st *ext)
{
	gnutls_free(ext->oid);
	gnutls_free(ext->data.data);
}

int _gnutls_x509_decode_ext(const gnutls_datum_t *der, gnutls_x509_ext_st *out)
{
	ASN1_TYPE c2 = ASN1_TYPE_EMPTY;
	char str_critical[10];
	char oid[MAX_OID_SIZE];
	int result, len, ret;

	memset(out, 0, sizeof(*out));

	/* decode der */
	result = asn1_create_element(_gnutls_get_pkix(), "PKIX1.Extension", &c2);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	result = _asn1_strict_der_decode(&c2, der->data, der->size, NULL);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		ret = _gnutls_asn2err(result);
		goto cleanup;
	}

	len = sizeof(oid)-1;
	result = asn1_read_value(c2, "extnID", oid, &len);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		ret = _gnutls_asn2err(result);
		goto cleanup;
	}

	len = sizeof(str_critical)-1;
	result = asn1_read_value(c2, "critical", str_critical, &len);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		ret = _gnutls_asn2err(result);
		goto cleanup;
	}

	if (str_critical[0] == 'T')
		out->critical = 1;
	else
		out->critical = 0;

	ret = _gnutls_x509_read_value(c2, "extnValue", &out->data);
	if (ret == GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE || ret == GNUTLS_E_ASN1_ELEMENT_NOT_FOUND) {
		out->data.data = NULL;
		out->data.size = 0;
	} else if (ret < 0) {
		gnutls_assert();
		goto fail;
	}

	out->oid = gnutls_strdup(oid);
	if (out->oid == NULL) {
		ret = GNUTLS_E_MEMORY_ERROR;
		goto fail;
	}

	ret = 0;
	goto cleanup;
 fail:
	memset(out, 0, sizeof(*out));
 cleanup:
	asn1_delete_structure(&c2);
	return ret;
	
}

/* flags can be zero or GNUTLS_EXT_FLAG_APPEND
 */
static int parse_tlsfeatures(ASN1_TYPE c2, gnutls_x509_tlsfeatures_t f, unsigned flags)
{
	char nptr[MAX_NAME_SIZE];
	int result;
	unsigned i, indx, j;
	unsigned int feature;

	if (!(flags & GNUTLS_EXT_FLAG_APPEND))
		f->size = 0;

	for (i = 1;; i++) {
		unsigned skip = 0;
		snprintf(nptr, sizeof(nptr), "?%u", i);

		result = _gnutls_x509_read_uint(c2, nptr, &feature);

		if (result == GNUTLS_E_ASN1_ELEMENT_NOT_FOUND || result == GNUTLS_E_ASN1_VALUE_NOT_FOUND) {
			break;
		}
		else if (result != GNUTLS_E_SUCCESS) {
			gnutls_assert();
			return _gnutls_asn2err(result);
		}

		if (feature > UINT16_MAX) {
			gnutls_assert();
			return GNUTLS_E_CERTIFICATE_ERROR;
		}

		/* skip duplicates */
		for (j=0;j<f->size;j++) {
			if (f->feature[j] == feature) {
				skip = 1;
				break;
			}
		}

		if (!skip) {
			if (f->size >= sizeof(f->feature)/sizeof(f->feature[0])) {
				gnutls_assert();
				return GNUTLS_E_INTERNAL_ERROR;
			}

			indx = f->size;
			f->feature[indx] = feature;
			f->size++;
		}
	}

	return 0;
}

/**
 * gnutls_x509_ext_import_tlsfeatures:
 * @ext: The DER-encoded extension data
 * @f: The features structure
 * @flags: zero or %GNUTLS_EXT_FLAG_APPEND
 *
 * This function will export the features in the provided DER-encoded
 * TLS Features PKIX extension, to a %gnutls_x509_tlsfeatures_t type. @f
 * must be initialized.
 *
 * When the @flags is set to %GNUTLS_EXT_FLAG_APPEND,
 * then if the @features structure is empty this function will behave
 * identically as if the flag was not set. Otherwise if there are elements 
 * in the @features structure then they will be merged with.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a negative error value.
 *
 * Since: 3.5.1
 **/
int gnutls_x509_ext_import_tlsfeatures(const gnutls_datum_t * ext,
				       gnutls_x509_tlsfeatures_t f,
				       unsigned int flags)
{
	int ret;
	ASN1_TYPE c2 = ASN1_TYPE_EMPTY;

	if (ext->size == 0 || ext->data == NULL) {
		gnutls_assert();
		return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
	}

	ret = asn1_create_element(_gnutls_get_pkix(),
				  "PKIX1.TlsFeatures", &c2);
	if (ret != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(ret);
	}

	ret = _asn1_strict_der_decode(&c2, ext->data, ext->size, NULL);
	if (ret != ASN1_SUCCESS) {
		gnutls_assert();
		ret = _gnutls_asn2err(ret);
		goto cleanup;
	}

	ret = parse_tlsfeatures(c2, f, flags);
	if (ret < 0) {
		gnutls_assert();
	}

 cleanup:
	asn1_delete_structure(&c2);

	return ret;
}

/**
 * gnutls_x509_ext_export_tlsfeatures:
 * @f: The features structure
 * @ext: The DER-encoded extension data; must be freed using gnutls_free().
 *
 * This function will convert the provided TLS features structure structure to a
 * DER-encoded TLS features PKIX extension. The output data in @ext will be allocated using
 * gnutls_malloc().
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a negative error value.
 *
 * Since: 3.5.1
 **/
int gnutls_x509_ext_export_tlsfeatures(gnutls_x509_tlsfeatures_t f,
					  gnutls_datum_t * ext)
{
	if (f == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	ASN1_TYPE c2 = ASN1_TYPE_EMPTY;
	int ret;
	unsigned i;

	ret = asn1_create_element(_gnutls_get_pkix(), "PKIX1.TlsFeatures", &c2);
	if (ret != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(ret);
	}

	for (i = 0; i < f->size; ++i) {

		ret = asn1_write_value(c2, "", "NEW", 1);
		if (ret != ASN1_SUCCESS) {
			gnutls_assert();
			ret = _gnutls_asn2err(ret);
			goto cleanup;
		}

		ret = _gnutls_x509_write_uint32(c2, "?LAST", f->feature[i]);
		if (ret != GNUTLS_E_SUCCESS) {
			gnutls_assert();
			goto cleanup;
		}
	}

	ret = _gnutls_x509_der_encode(c2, "", ext, 0);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = 0;

 cleanup:
	asn1_delete_structure(&c2);
	return ret;
}

/**
 * gnutls_x509_tlsfeatures_add:
 * @f: The TLS features
 * @feature: The feature to add
 *
 * This function will append a feature to the X.509 TLS features
 * extension structure.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned,
 *   otherwise a negative error value.
 *
 * Since: 3.5.1
 **/
int gnutls_x509_tlsfeatures_add(gnutls_x509_tlsfeatures_t f, unsigned int feature)
{
	if (f == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	if (feature > UINT16_MAX)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	if (f->size >= sizeof(f->feature)/sizeof(f->feature[0]))
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

	f->feature[f->size++] = feature;

	return 0;
}
