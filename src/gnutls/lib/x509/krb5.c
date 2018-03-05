/*
 * Copyright (C) 2015 Red Hat, Inc.
 *
 * This file is part of GnuTLS.
 *
 * GnuTLS is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GnuTLS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <gnutls/gnutls.h>
#include <libtasn1.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <errors.h>
#include "krb5.h"
#include "common.h"

#define _gnutls_asn2err(x) GNUTLS_E_ASN1_DER_ERROR

#define MAX_COMPONENTS 6

typedef struct krb5_principal_data {
	char *realm;
	char *data[MAX_COMPONENTS];
	uint32_t length;
	int8_t type;
} krb5_principal_data;

extern const asn1_static_node krb5_asn1_tab[];

static void cleanup_principal(krb5_principal_data * princ)
{
	unsigned i;
	if (princ) {
		gnutls_free(princ->realm);
		for (i = 0; i < princ->length; i++)
			gnutls_free(princ->data[i]);
		memset(princ, 0, sizeof(*princ));
		gnutls_free(princ);
	}
}

static krb5_principal_data *name_to_principal(const char *_name)
{
	krb5_principal_data *princ;
	char *p, *p2, *sp;
	unsigned pos = 0;
	char *name = NULL;

	princ = gnutls_calloc(1, sizeof(struct krb5_principal_data));
	if (princ == NULL)
		return NULL;

	name = gnutls_strdup(_name);
	if (name == NULL) {
		gnutls_assert();
		goto fail;
	}

	p = strrchr(name, '@');
	p2 = strchr(name, '@');
	if (p == NULL) {
		/* unknown name type */
		gnutls_assert();
		goto fail;
	}

	princ->realm = gnutls_strdup(p + 1);
	if (princ->realm == NULL) {
		gnutls_assert();
		goto fail;
	}
	*p = 0;

	if (p == p2) {
		p = strtok_r(name, "/", &sp);
		while (p) {
			if (pos == MAX_COMPONENTS) {
				_gnutls_debug_log
				    ("%s: Cannot parse names with more than %d components\n",
				     __func__, MAX_COMPONENTS);
				goto fail;
			}

			princ->data[pos] = gnutls_strdup(p);
			if (princ->data[pos] == NULL) {
				gnutls_assert();
				goto fail;
			}

			princ->length++;
			pos++;

			p = strtok_r(NULL, "/", &sp);
		}

		if ((princ->length == 2)
		    && (strcmp(princ->data[0], "krbtgt") == 0)) {
			princ->type = 2;	/* KRB_NT_SRV_INST */
		} else {
			princ->type = 1;	/* KRB_NT_PRINCIPAL */
		}
	} else {		/* enterprise */
		princ->data[0] = gnutls_strdup(name);
		if (princ->data[0] == NULL) {
			gnutls_assert();
			goto fail;
		}

		princ->length++;
		princ->type = 10;	/* KRB_NT_ENTERPRISE */
	}

	goto cleanup;
 fail:
	cleanup_principal(princ);
	princ = NULL;

 cleanup:
	gnutls_free(name);
	return princ;
}

int _gnutls_krb5_principal_to_der(const char *name, gnutls_datum_t * der)
{
	int ret, result;
	ASN1_TYPE c2 = ASN1_TYPE_EMPTY;
	krb5_principal_data *princ;
	unsigned i;

	princ = name_to_principal(name);
	if (princ == NULL) {
		gnutls_assert();
		ret = GNUTLS_E_PARSING_ERROR;
		goto cleanup;
	}

	result =
	    asn1_create_element(_gnutls_get_gnutls_asn(),
				"GNUTLS.KRB5PrincipalName", &c2);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		ret = _gnutls_asn2err(result);
		goto cleanup;
	}

	result =
	    asn1_write_value(c2, "realm", princ->realm, strlen(princ->realm));
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		ret = _gnutls_asn2err(result);
		goto cleanup;
	}

	result =
	    asn1_write_value(c2, "principalName.name-type", &princ->type, 1);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		ret = _gnutls_asn2err(result);
		goto cleanup;
	}

	for (i = 0; i < princ->length; i++) {
		result =
		    asn1_write_value(c2, "principalName.name-string", "NEW", 1);
		if (result != ASN1_SUCCESS) {
			gnutls_assert();
			ret = _gnutls_asn2err(result);
			goto cleanup;
		}

		result =
		    asn1_write_value(c2,
				     "principalName.name-string.?LAST",
				     princ->data[i], strlen(princ->data[i]));
		if (result != ASN1_SUCCESS) {
			gnutls_assert();
			ret = _gnutls_asn2err(result);
			goto cleanup;
		}
	}

	ret = _gnutls_x509_der_encode(c2, "", der, 0);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = 0;
 cleanup:
	cleanup_principal(princ);
	asn1_delete_structure(&c2);
	return ret;
}

static int principal_to_str(ASN1_TYPE c2, gnutls_buffer_st * str)
{
	gnutls_datum_t realm = { NULL, 0 };
	gnutls_datum_t component = { NULL, 0 };
	unsigned char name_type[2];
	int ret, result, len;
	unsigned i;
	char val[128];

	ret = _gnutls_x509_read_value(c2, "realm", &realm);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	len = sizeof(name_type);
	result =
	    asn1_read_value(c2, "principalName.name-type", name_type, &len);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		ret = _gnutls_asn2err(result);
		goto cleanup;
	}

	if (len != 1
	    || (name_type[0] != 1 && name_type[0] != 2 && name_type[0] != 10)) {
		ret = GNUTLS_E_INVALID_REQUEST;
		goto cleanup;
	}

	for (i = 0;; i++) {
		snprintf(val, sizeof(val), "principalName.name-string.?%u",
			 i + 1);
		ret = _gnutls_x509_read_value(c2, val, &component);
		if (ret == GNUTLS_E_ASN1_VALUE_NOT_FOUND
		    || ret == GNUTLS_E_ASN1_ELEMENT_NOT_FOUND)
			break;
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		if (i > 0) {
			ret = _gnutls_buffer_append_data(str, "/", 1);
			if (ret < 0) {
				gnutls_assert();
				goto cleanup;
			}
		}

		ret =
		    _gnutls_buffer_append_data(str, component.data,
					       component.size);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		_gnutls_free_datum(&component);
	}

	ret = _gnutls_buffer_append_data(str, "@", 1);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = _gnutls_buffer_append_data(str, realm.data, realm.size);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = 0;
 cleanup:
	_gnutls_free_datum(&component);
	gnutls_free(realm.data);
	return ret;
}

int _gnutls_krb5_der_to_principal(const gnutls_datum_t * der,
				  gnutls_datum_t * name)
{
	int ret, result;
	ASN1_TYPE c2 = ASN1_TYPE_EMPTY;
	gnutls_buffer_st str;

	_gnutls_buffer_init(&str);

	result =
	    asn1_create_element(_gnutls_get_gnutls_asn(),
				"GNUTLS.KRB5PrincipalName", &c2);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		ret = _gnutls_asn2err(result);
		goto cleanup;
	}

	result = asn1_der_decoding(&c2, der->data, der->size, NULL);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		ret = _gnutls_asn2err(result);
		goto cleanup;
	}

	ret = principal_to_str(c2, &str);
	if (ret < 0) {
		/* for some reason we cannot convert to a human readable string
		 * the principal. Then we use the #HEX format.
		 */
		_gnutls_buffer_reset(&str);
		ret = _gnutls_buffer_append_data(&str, "#", 1);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		_gnutls_buffer_hexprint(&str, der->data, der->size);
	}

	asn1_delete_structure(&c2);
	return _gnutls_buffer_to_datum(&str, name, 1);

 cleanup:
	_gnutls_buffer_clear(&str);
	asn1_delete_structure(&c2);
	return ret;
}
