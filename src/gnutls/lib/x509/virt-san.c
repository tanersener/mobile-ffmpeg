/*
 * Copyright (C) 2015-2016 Nikos Mavrogiannopoulos
 * Copyright (C) 2015-2016 Red Hat, Inc.
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

/* This file contains functions to handle the virtual subject alternative names,
 * based on othernames, such as GNUTLS_SAN_OTHERNAME_XMPP.
 */

#include "gnutls_int.h"
#include "x509_int.h"
#include "x509_ext_int.h"
#include "common.h"
#include "krb5.h"
#include "virt-san.h"

static
int san_othername_to_virtual(const char *oid, size_t size)
{
	if (oid) {
		if ((unsigned) size == (sizeof(XMPP_OID)-1)
		    && memcmp(oid, XMPP_OID, sizeof(XMPP_OID)-1) == 0)
			return GNUTLS_SAN_OTHERNAME_XMPP;
		else if ((unsigned) size == (sizeof(KRB5_PRINCIPAL_OID)-1)
		    && memcmp(oid, KRB5_PRINCIPAL_OID, sizeof(KRB5_PRINCIPAL_OID)-1) == 0)
			return GNUTLS_SAN_OTHERNAME_KRB5PRINCIPAL;
	}

	return GNUTLS_SAN_OTHERNAME;
}

static
const char * virtual_to_othername_oid(unsigned type)
{
	switch(type) {
		case GNUTLS_SAN_OTHERNAME_XMPP:
			return XMPP_OID;
		case GNUTLS_SAN_OTHERNAME_KRB5PRINCIPAL:
			return KRB5_PRINCIPAL_OID;
		default:
			return NULL;
	}
}

int _gnutls_alt_name_assign_virt_type(struct name_st *name, unsigned type, gnutls_datum_t *san, const char *othername_oid, unsigned raw)
{
	gnutls_datum_t encoded = {NULL, 0};
	gnutls_datum_t xmpp = {NULL,0};
	int ret;

	if (type < 1000) {
		name->type = type;
		ret = _gnutls_alt_name_process(&name->san, type, san, raw);
		if (ret < 0)
			return gnutls_assert_val(ret);
		gnutls_free(san->data);
		san->data = NULL;

		if (othername_oid) {
			name->othername_oid.data = (uint8_t *) othername_oid;
			name->othername_oid.size = strlen(othername_oid);
		} else {
			name->othername_oid.data = NULL;
			name->othername_oid.size = 0;
		}
	} else { /* virtual types */
		const char *oid = virtual_to_othername_oid(type);

		if (oid == NULL)
			return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

		switch(type) {
			case GNUTLS_SAN_OTHERNAME_XMPP:

				ret = gnutls_idna_map((char*)san->data, san->size, &xmpp, 0);
				if (ret < 0)
					return gnutls_assert_val(ret);

				ret = _gnutls_x509_encode_string(ASN1_ETYPE_UTF8_STRING,
					xmpp.data, xmpp.size, &encoded);

				gnutls_free(xmpp.data);
				if (ret < 0)
					return gnutls_assert_val(ret);

				name->type = GNUTLS_SAN_OTHERNAME;
				name->san.data = encoded.data;
				name->san.size = encoded.size;
				name->othername_oid.data = (void*)gnutls_strdup(oid);
				name->othername_oid.size = strlen(oid);
				break;

			case GNUTLS_SAN_OTHERNAME_KRB5PRINCIPAL:
				ret = _gnutls_krb5_principal_to_der((char*)san->data, &name->san);
				if (ret < 0)
					return gnutls_assert_val(ret);

				name->othername_oid.data = (void*)gnutls_strdup(oid);
				name->othername_oid.size = strlen(oid);
				name->type = GNUTLS_SAN_OTHERNAME;
				break;

			default:
				return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);
		}

		gnutls_free(san->data);
	}

	return 0;
}

/**
 * gnutls_x509_othername_to_virtual:
 * @oid: The othername object identifier
 * @othername: The othername data
 * @virt_type: GNUTLS_SAN_OTHERNAME_XXX
 * @virt: allocated printable data
 *
 * This function will parse and convert the othername data to a virtual
 * type supported by gnutls.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a negative error value.
 *
 * Since: 3.3.8
 **/
int gnutls_x509_othername_to_virtual(const char *oid, 
				     const gnutls_datum_t *othername,
				     unsigned int *virt_type,
				     gnutls_datum_t *virt)
{
	int ret;
	unsigned type;

	type = san_othername_to_virtual(oid, strlen(oid));
	if (type == GNUTLS_SAN_OTHERNAME)
		return gnutls_assert_val(GNUTLS_E_X509_UNKNOWN_SAN);

	if (virt_type)
		*virt_type = type;

	switch(type) {
		case GNUTLS_SAN_OTHERNAME_XMPP:
			ret = _gnutls_x509_decode_string
				    (ASN1_ETYPE_UTF8_STRING, othername->data,
				     othername->size, virt, 0);
			if (ret < 0) {
				gnutls_assert();
				return ret;
			}
			return 0;
		case GNUTLS_SAN_OTHERNAME_KRB5PRINCIPAL:
			ret = _gnutls_krb5_der_to_principal(othername, virt);
			if (ret < 0) {
				gnutls_assert();
				return ret;
			}
			return 0;
		default:
			return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);
	}
}
