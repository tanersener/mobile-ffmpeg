/*
 * Copyright (C) 2015 Red Hat, Inc.
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

/* Functions that relate on PKCS7 attribute setting.
 */

#include "gnutls_int.h"

#include <datum.h>
#include <global.h>
#include "errors.h"
#include <common.h>
#include <x509_b64.h>
#include <gnutls/abstract.h>
#include <gnutls/pkcs7.h>

/**
 * gnutls_pkcs7_add_attr:
 * @list: A list of existing attributes or pointer to %NULL for the first one
 * @oid: the OID of the attribute to be set
 * @data: the raw (DER-encoded) data of the attribute to be set
 * @flags: zero or %GNUTLS_PKCS7_ATTR_ENCODE_OCTET_STRING
 *
 * This function will set a PKCS #7 attribute in the provided list.
 * If this function fails, the previous list would be deallocated.
 *
 * Note that any attributes set with this function must either be
 * DER or BER encoded, unless a special flag is present.
 *
 * Returns: On success, the new list head, otherwise %NULL.
 *
 * Since: 3.4.2
 **/
int
gnutls_pkcs7_add_attr(gnutls_pkcs7_attrs_t * list, const char *oid,
		      gnutls_datum_t * data, unsigned flags)
{
	int ret;
	gnutls_pkcs7_attrs_st *r;

	r = gnutls_calloc(1, sizeof(gnutls_pkcs7_attrs_st));
	if (r == NULL)
		goto fail;

	if (flags & GNUTLS_PKCS7_ATTR_ENCODE_OCTET_STRING) {
		ret = _gnutls_x509_encode_string(ASN1_ETYPE_OCTET_STRING,
						 data->data, data->size,
						 &r->data);
	} else {
		ret = _gnutls_set_datum(&r->data, data->data, data->size);
	}
	if (ret < 0)
		goto fail;

	r->oid = gnutls_strdup(oid);
	if (r->oid == NULL)
		goto fail;

	r->next = *list;
	*list = r;

	return 0;
 fail:
	if (r) {
		gnutls_free(r->data.data);
		gnutls_free(r);
	}
	gnutls_pkcs7_attrs_deinit(*list);
	return GNUTLS_E_MEMORY_ERROR;

}

/**
 * gnutls_pkcs7_get_attr:
 * @list: A list of existing attributes or %NULL for the first one
 * @idx: the index of the attribute to get
 * @oid: the OID of the attribute (read-only)
 * @data: the raw data of the attribute
 * @flags: zero or %GNUTLS_PKCS7_ATTR_ENCODE_OCTET_STRING
 *
 * This function will get a PKCS #7 attribute from the provided list.
 * The OID is a constant string, but data will be allocated and must be
 * deinitialized by the caller.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value. %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE is returned
 *   if there are no data in the current index.
 *
 * Since: 3.4.2
 **/
int
gnutls_pkcs7_get_attr(gnutls_pkcs7_attrs_t list, unsigned idx, char **oid,
		      gnutls_datum_t * data, unsigned flags)
{
	unsigned i;
	gnutls_pkcs7_attrs_st *p = list;
	int ret;

	for (i = 0; i < idx; i++) {
		p = p->next;
		if (p == NULL)
			break;
	}

	if (p == NULL)
		return gnutls_assert_val(GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE);

	*oid = p->oid;

	if (flags & GNUTLS_PKCS7_ATTR_ENCODE_OCTET_STRING) {
		ret = _gnutls_x509_decode_string(ASN1_ETYPE_OCTET_STRING,
						 p->data.data, p->data.size,
						 data, 1);
	} else {
		ret = _gnutls_set_datum(data, p->data.data, p->data.size);
	}
	if (ret < 0)
		return gnutls_assert_val(ret);

	return 0;
}

/**
 * gnutls_pkcs7_attrs_deinit:
 * @list: A list of existing attributes
 *
 * This function will clear a PKCS #7 attribute list.
 *
 * Since: 3.4.2
 **/
void gnutls_pkcs7_attrs_deinit(gnutls_pkcs7_attrs_t list)
{
	gnutls_pkcs7_attrs_st *r = list, *next;

	while (r) {
		next = r->next;

		gnutls_free(r->data.data);
		gnutls_free(r->oid);
		gnutls_free(r);
		r = next;
	}
}
