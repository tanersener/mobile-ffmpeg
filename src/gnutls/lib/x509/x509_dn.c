/*
 * Copyright (C) 2013-2016 Nikos Mavrogiannopoulos
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
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
#include <x509_b64.h>
#include <c-ctype.h>

typedef int (*set_dn_func) (void *, const char *oid, unsigned int raw_flag,
			    const void *name, unsigned int name_size);

static
int dn_attr_crt_set(set_dn_func f, void *crt, const gnutls_datum_t * name,
		    const gnutls_datum_t * val, unsigned is_raw)
{
	char _oid[MAX_OID_SIZE];
	gnutls_datum_t tmp;
	const char *oid;
	int ret;
	unsigned i,j;

	if (name->size == 0 || val->size == 0)
		return gnutls_assert_val(GNUTLS_E_PARSING_ERROR);

	if (c_isdigit(name->data[0]) != 0) {
		if (name->size >= sizeof(_oid))
			return gnutls_assert_val(GNUTLS_E_PARSING_ERROR);

		memcpy(_oid, name->data, name->size);
		_oid[name->size] = 0;

		oid = _oid;

		if (gnutls_x509_dn_oid_known(oid) == 0 && !is_raw) {
			_gnutls_debug_log("Unknown OID: '%s'\n", oid);
			return gnutls_assert_val(GNUTLS_E_PARSING_ERROR);
		}
	} else {
		oid =
		    _gnutls_ldap_string_to_oid((char *) name->data,
					       name->size);
	}

	if (oid == NULL) {
		_gnutls_debug_log("Unknown DN attribute: '%.*s'\n",
				  (int) name->size, name->data);
		return gnutls_assert_val(GNUTLS_E_PARSING_ERROR);
	}

	if (is_raw) {
		gnutls_datum_t hex = {val->data+1, val->size-1};

		ret = gnutls_hex_decode2(&hex, &tmp);
		if (ret < 0)
			return gnutls_assert_val(GNUTLS_E_PARSING_ERROR);
	} else {
		tmp.size = val->size;
		tmp.data = gnutls_malloc(tmp.size+1);
		if (tmp.data == NULL) {
			return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);
		}

		/* unescape */
		for (j=i=0;i<tmp.size;i++) {
			if (1+j!=val->size && val->data[j] == '\\') {
				if (val->data[j+1] == ',' || val->data[j+1] == '#' ||
				    val->data[j+1] == ' ' || val->data[j+1] == '+' ||
				    val->data[j+1] == '"' || val->data[j+1] == '<' ||
				    val->data[j+1] == '>' || val->data[j+1] == ';' ||
				    val->data[j+1] == '\\' || val->data[j+1] == '=') {
					tmp.data[i] = val->data[j+1];
					j+=2;
					tmp.size--;
				} else {
					ret = gnutls_assert_val(GNUTLS_E_PARSING_ERROR);
					goto fail;
				}
			} else {
				tmp.data[i] = val->data[j++];
			}
		}
		tmp.data[tmp.size] = 0;
	}

	ret = f(crt, oid, is_raw, tmp.data, tmp.size);
	if (ret < 0) {
		gnutls_assert();
		goto fail;
	}

	ret = 0;
 fail:
	gnutls_free(tmp.data);
	return ret;
}

static int read_attr_and_val(const char **ptr,
			     gnutls_datum_t *name, gnutls_datum_t *val,
			     unsigned *is_raw)
{
	const unsigned char *p = (void *) *ptr;

	*is_raw = 0;

	/* skip any space */
	while (c_isspace(*p))
		p++;

	/* Read the name */
	name->data = (void *) p;
	while (*p != '=' && *p != 0 && !c_isspace(*p))
		p++;

	name->size = p - name->data;

	/* skip any space */
	while (c_isspace(*p))
		p++;

	if (*p != '=')
		return gnutls_assert_val(GNUTLS_E_PARSING_ERROR);
	p++;

	while (c_isspace(*p))
		p++;

	if (*p == '#') {
		*is_raw = 1;
	}

	/* Read value */
	val->data = (void *) p;
	while (*p != 0 && (*p != ',' || (*p == ',' && *(p - 1) == '\\'))
	       && *p != '\n') {
		p++;
	}
	val->size = p - (val->data);
	*ptr = (void*)p;

	p = val->data;
	/* check for unescaped '+' - we do not support them */
	while (*p != 0) {
		if (*p == '+' && (*(p - 1) != '\\'))
			return gnutls_assert_val(GNUTLS_E_PARSING_ERROR);
		p++;
	}

	/* remove spaces from the end */
	while(val->size > 0 && c_isspace(val->data[val->size-1])) {
		if (val->size-2 > 0 && val->data[val->size-2] == '\\')
			break;
		val->size--;
	}

	if (val->size == 0 || name->size == 0)
		return gnutls_assert_val(GNUTLS_E_PARSING_ERROR);

	return 0;
}

typedef struct elem_list_st {
	gnutls_datum_t name;
	gnutls_datum_t val;
	const char *pos;
	unsigned is_raw;
	struct elem_list_st *next;
} elem_list_st;

static int add_new_elem(elem_list_st **head, const gnutls_datum_t *name, const gnutls_datum_t *val, const char *pos, unsigned is_raw)
{
	elem_list_st *elem = gnutls_malloc(sizeof(*elem));
	if (elem == NULL)
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

	memcpy(&elem->name, name, sizeof(*name));
	memcpy(&elem->val, val, sizeof(*val));
	elem->pos = pos;
	elem->is_raw = is_raw;
	elem->next = *head;
	*head = elem;

	return 0;
}

static int
crt_set_dn(set_dn_func f, void *crt, const char *dn, const char **err)
{
	const char *p = dn;
	int ret;
	gnutls_datum_t name, val;
	unsigned is_raw;
	elem_list_st *list = NULL, *plist, *next;

	if (crt == NULL || dn == NULL)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	/* We parse the string and set all elements to a linked list in
	 * reverse order. That way we can encode in reverse order,
	 * the way RFC4514 requires. */

	/* For each element */
	while (*p != 0 && *p != '\n') {
		if (err)
			*err = p;

		is_raw = 0;
		ret = read_attr_and_val(&p, &name, &val, &is_raw);
		if (ret < 0) {
			gnutls_assert();
			goto fail;
		}

		/* skip spaces and look for comma */
		while (c_isspace(*p))
			p++;

		ret = add_new_elem(&list, &name, &val, p, is_raw);
		if (ret < 0) {
			gnutls_assert();
			goto fail;
		}

		if (*p != ',' && *p != 0 && *p != '\n') {
			ret = gnutls_assert_val(GNUTLS_E_PARSING_ERROR);
			goto fail;
		}
		if (*p == ',')
			p++;
	}

	plist = list;
	while(plist) {
		if (err)
			*err = plist->pos;
		ret = dn_attr_crt_set(f, crt, &plist->name, &plist->val, plist->is_raw);
		if (ret < 0)
			goto fail;

		plist = plist->next;
	}

	ret = 0;
fail:
	plist = list;
	while(plist) {
		next = plist->next;
		gnutls_free(plist);
		plist = next;
	}
	return ret;
}


/**
 * gnutls_x509_crt_set_dn:
 * @crt: a certificate of type #gnutls_x509_crt_t
 * @dn: a comma separated DN string (RFC4514)
 * @err: indicates the error position (if any)
 *
 * This function will set the DN on the provided certificate.
 * The input string should be plain ASCII or UTF-8 encoded. On
 * DN parsing error %GNUTLS_E_PARSING_ERROR is returned.
 *
 * Note that DNs are not expected to hold DNS information, and thus
 * no automatic IDNA conversions are attempted when using this function.
 * If that is required (e.g., store a domain in CN), process the corresponding
 * input with gnutls_idna_map().
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_x509_crt_set_dn(gnutls_x509_crt_t crt, const char *dn,
		       const char **err)
{
	return crt_set_dn((set_dn_func) gnutls_x509_crt_set_dn_by_oid, crt,
			  dn, err);
}

/**
 * gnutls_x509_crt_set_issuer_dn:
 * @crt: a certificate of type #gnutls_x509_crt_t
 * @dn: a comma separated DN string (RFC4514)
 * @err: indicates the error position (if any)
 *
 * This function will set the DN on the provided certificate.
 * The input string should be plain ASCII or UTF-8 encoded. On
 * DN parsing error %GNUTLS_E_PARSING_ERROR is returned.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_x509_crt_set_issuer_dn(gnutls_x509_crt_t crt, const char *dn,
			      const char **err)
{
	return crt_set_dn((set_dn_func)
			  gnutls_x509_crt_set_issuer_dn_by_oid, crt, dn,
			  err);
}

/**
 * gnutls_x509_crq_set_dn:
 * @crq: a certificate of type #gnutls_x509_crq_t
 * @dn: a comma separated DN string (RFC4514)
 * @err: indicates the error position (if any)
 *
 * This function will set the DN on the provided certificate.
 * The input string should be plain ASCII or UTF-8 encoded. On
 * DN parsing error %GNUTLS_E_PARSING_ERROR is returned.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_x509_crq_set_dn(gnutls_x509_crq_t crq, const char *dn,
		       const char **err)
{
	return crt_set_dn((set_dn_func) gnutls_x509_crq_set_dn_by_oid, crq,
			  dn, err);
}

static
int set_dn_by_oid(gnutls_x509_dn_t dn, const char *oid, unsigned int raw_flag, const void *name, unsigned name_size)
{
	return _gnutls_x509_set_dn_oid(dn->asn, "", oid, raw_flag, name, name_size);
}

/**
 * gnutls_x509_dn_set_str:
 * @dn: a pointer to DN
 * @str: a comma separated DN string (RFC4514)
 * @err: indicates the error position (if any)
 *
 * This function will set the DN on the provided DN structure.
 * The input string should be plain ASCII or UTF-8 encoded. On
 * DN parsing error %GNUTLS_E_PARSING_ERROR is returned.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.5.3
 **/
int
gnutls_x509_dn_set_str(gnutls_x509_dn_t dn, const char *str, const char **err)
{
	if (dn == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	return crt_set_dn((set_dn_func) set_dn_by_oid, dn,
			  str, err);
}

/**
 * gnutls_x509_dn_init:
 * @dn: the object to be initialized
 *
 * This function initializes a #gnutls_x509_dn_t type.
 *
 * The object returned must be deallocated using
 * gnutls_x509_dn_deinit().
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 2.4.0
 **/
int gnutls_x509_dn_init(gnutls_x509_dn_t * dn)
{
	int result;

	*dn = gnutls_calloc(1, sizeof(gnutls_x509_dn_st));

	if ((result =
	     asn1_create_element(_gnutls_get_pkix(),
				 "PKIX1.Name", &(*dn)->asn)) != ASN1_SUCCESS) {
		gnutls_assert();
		gnutls_free(*dn);
		return _gnutls_asn2err(result);
	}

	return 0;
}

/**
 * gnutls_x509_dn_import:
 * @dn: the structure that will hold the imported DN
 * @data: should contain a DER encoded RDN sequence
 *
 * This function parses an RDN sequence and stores the result to a
 * #gnutls_x509_dn_t type. The data must have been initialized
 * with gnutls_x509_dn_init(). You may use gnutls_x509_dn_get_rdn_ava() to
 * decode the DN.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 2.4.0
 **/
int gnutls_x509_dn_import(gnutls_x509_dn_t dn, const gnutls_datum_t * data)
{
	int result;
	char err[ASN1_MAX_ERROR_DESCRIPTION_SIZE];

	if (data->data == NULL || data->size == 0)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	result = _asn1_strict_der_decode(&dn->asn,
				   data->data, data->size, err);
	if (result != ASN1_SUCCESS) {
		/* couldn't decode DER */
		_gnutls_debug_log("ASN.1 Decoding error: %s\n", err);
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	return 0;
}

/**
 * gnutls_x509_dn_deinit:
 * @dn: a DN uint8_t object pointer.
 *
 * This function deallocates the DN object as returned by
 * gnutls_x509_dn_import().
 *
 * Since: 2.4.0
 **/
void gnutls_x509_dn_deinit(gnutls_x509_dn_t dn)
{
	asn1_delete_structure(&dn->asn);
	gnutls_free(dn);
}

/**
 * gnutls_x509_dn_export:
 * @dn: Holds the uint8_t DN object
 * @format: the format of output params. One of PEM or DER.
 * @output_data: will contain a DN PEM or DER encoded
 * @output_data_size: holds the size of output_data (and will be
 *   replaced by the actual size of parameters)
 *
 * This function will export the DN to DER or PEM format.
 *
 * If the buffer provided is not long enough to hold the output, then
 * *@output_data_size is updated and %GNUTLS_E_SHORT_MEMORY_BUFFER
 * will be returned.
 *
 * If the structure is PEM encoded, it will have a header
 * of "BEGIN NAME".
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_x509_dn_export(gnutls_x509_dn_t dn,
		      gnutls_x509_crt_fmt_t format, void *output_data,
		      size_t * output_data_size)
{
	if (dn == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	return _gnutls_x509_export_int_named(dn->asn, "rdnSequence",
					     format, "NAME",
					     output_data,
					     output_data_size);
}

/**
 * gnutls_x509_dn_export2:
 * @dn: Holds the uint8_t DN object
 * @format: the format of output params. One of PEM or DER.
 * @out: will contain a DN PEM or DER encoded
 *
 * This function will export the DN to DER or PEM format.
 *
 * The output buffer is allocated using gnutls_malloc().
 *
 * If the structure is PEM encoded, it will have a header
 * of "BEGIN NAME".
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.1.3
 **/
int
gnutls_x509_dn_export2(gnutls_x509_dn_t dn,
		       gnutls_x509_crt_fmt_t format, gnutls_datum_t * out)
{
	if (dn == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	return _gnutls_x509_export_int_named2(dn->asn, "rdnSequence",
					      format, "NAME", out);
}

/**
 * gnutls_x509_dn_get_rdn_ava:
 * @dn: a pointer to DN
 * @irdn: index of RDN
 * @iava: index of AVA.
 * @ava: Pointer to structure which will hold output information.
 *
 * Get pointers to data within the DN. The format of the @ava structure
 * is shown below.
 *
 *  struct gnutls_x509_ava_st {
 *    gnutls_datum_t oid;
 *    gnutls_datum_t value;
 *    unsigned long value_tag;
 *  };
 *
 * The X.509 distinguished name is a sequence of sequences of strings
 * and this is what the @irdn and @iava indexes model.
 *
 * Note that @ava will contain pointers into the @dn structure which
 * in turns points to the original certificate. Thus you should not
 * modify any data or deallocate any of those.
 *
 * This is a low-level function that requires the caller to do the
 * value conversions when necessary (e.g. from UCS-2).
 *
 * Returns: Returns 0 on success, or an error code.
 **/
int
gnutls_x509_dn_get_rdn_ava(gnutls_x509_dn_t dn,
			   int irdn, int iava, gnutls_x509_ava_st * ava)
{
	ASN1_TYPE rdn, elem;
	ASN1_DATA_NODE vnode;
	long len;
	int lenlen, remlen, ret;
	char rbuf[MAX_NAME_SIZE];
	unsigned char cls;
	const unsigned char *ptr;

	iava++;
	irdn++;			/* 0->1, 1->2 etc */

	snprintf(rbuf, sizeof(rbuf), "rdnSequence.?%d.?%d", irdn, iava);
	rdn = asn1_find_node(dn->asn, rbuf);
	if (!rdn) {
		gnutls_assert();
		return GNUTLS_E_ASN1_ELEMENT_NOT_FOUND;
	}

	snprintf(rbuf, sizeof(rbuf), "?%d.type", iava);
	elem = asn1_find_node(rdn, rbuf);
	if (!elem) {
		gnutls_assert();
		return GNUTLS_E_ASN1_ELEMENT_NOT_FOUND;
	}

	ret = asn1_read_node_value(elem, &vnode);
	if (ret != ASN1_SUCCESS) {
		gnutls_assert();
		return GNUTLS_E_ASN1_ELEMENT_NOT_FOUND;
	}

	ava->oid.data = (void *) vnode.value;
	ava->oid.size = vnode.value_len;

	snprintf(rbuf, sizeof(rbuf), "?%d.value", iava);
	elem = asn1_find_node(rdn, rbuf);
	if (!elem) {
		gnutls_assert();
		return GNUTLS_E_ASN1_ELEMENT_NOT_FOUND;
	}

	ret = asn1_read_node_value(elem, &vnode);
	if (ret != ASN1_SUCCESS) {
		gnutls_assert();
		return GNUTLS_E_ASN1_ELEMENT_NOT_FOUND;
	}
	/* The value still has the previous tag's length bytes, plus the
	 * current value's tag and length bytes. Decode them.
	 */

	ptr = vnode.value;
	remlen = vnode.value_len;
	len = asn1_get_length_der(ptr, remlen, &lenlen);
	if (len < 0) {
		gnutls_assert();
		return GNUTLS_E_ASN1_DER_ERROR;
	}

	ptr += lenlen;
	remlen -= lenlen;
	ret =
	    asn1_get_tag_der(ptr, remlen, &cls, &lenlen, &ava->value_tag);
	if (ret) {
		gnutls_assert();
		return _gnutls_asn2err(ret);
	}

	ptr += lenlen;
	remlen -= lenlen;

	{
		signed long tmp;

		tmp = asn1_get_length_der(ptr, remlen, &lenlen);
		if (tmp < 0) {
			gnutls_assert();
			return GNUTLS_E_ASN1_DER_ERROR;
		}
		ava->value.size = tmp;
	}
	ava->value.data = (void *) (ptr + lenlen);

	return 0;
}

/**
 * gnutls_x509_dn_get_str:
 * @dn: a pointer to DN
 * @str: a datum that will hold the name
 *
 * This function will allocate buffer and copy the name in the provided DN.
 * The name will be in the form "C=xxxx,O=yyyy,CN=zzzz" as
 * described in RFC4514. The output string will be ASCII or UTF-8
 * encoded, depending on the certificate data.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.4.2
 **/
int
gnutls_x509_dn_get_str(gnutls_x509_dn_t dn, gnutls_datum_t *str)
{
	if (dn == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	return _gnutls_x509_get_dn(dn->asn, "rdnSequence", str, GNUTLS_X509_DN_FLAG_COMPAT);
}

/**
 * gnutls_x509_dn_get_str:
 * @dn: a pointer to DN
 * @str: a datum that will hold the name
 * @flags: zero or %GNUTLS_X509_DN_FLAG_COMPAT
 *
 * This function will allocate buffer and copy the name in the provided DN.
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
gnutls_x509_dn_get_str2(gnutls_x509_dn_t dn, gnutls_datum_t *str, unsigned flags)
{
	if (dn == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	return _gnutls_x509_get_dn(dn->asn, "rdnSequence", str, flags);
}
