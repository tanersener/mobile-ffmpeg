/*
 * GnuTLS PKCS#11 support
 * Copyright (C) 2010-2012 Free Software Foundation, Inc.
 * 
 * Authors: Nikos Mavrogiannopoulos, Stef Walter
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#include "gnutls_int.h"
#include <gnutls/pkcs11.h>
#include <stdio.h>
#include <string.h>
#include "errors.h"
#include <datum.h>
#include <pkcs11_int.h>
#include "pkcs11x.h"
#include <x509/common.h>

static const ck_bool_t tval = 1;
static const ck_bool_t fval = 0;

#define MAX_ASIZE 24

static void mark_flags(unsigned flags, struct ck_attribute *a, unsigned *a_val, unsigned trusted)
{
	static const unsigned long category = 2;

	if (flags & GNUTLS_PKCS11_OBJ_FLAG_MARK_CA) {
		a[*a_val].type = CKA_CERTIFICATE_CATEGORY;
		a[*a_val].value = (void *) &category;
		a[*a_val].value_len = sizeof(category);
		(*a_val)++;
	}

	if (flags & GNUTLS_PKCS11_OBJ_FLAG_MARK_DISTRUSTED) {
		if (trusted) {
			a[*a_val].type = CKA_X_DISTRUSTED;
			a[*a_val].value = (void *) &tval;
			a[*a_val].value_len = sizeof(tval);
			(*a_val)++;
		} else {
			_gnutls_debug_log("p11: ignoring the distrusted flag as it is not valid on non-p11-kit-trust modules\n");
		}
	}

	if (flags & GNUTLS_PKCS11_OBJ_FLAG_MARK_TRUSTED) {
		a[*a_val].type = CKA_TRUSTED;
		a[*a_val].value = (void *) &tval;
		a[*a_val].value_len = sizeof(tval);
		(*a_val)++;

		a[*a_val].type = CKA_PRIVATE;
		a[*a_val].value = (void *) &fval;
		a[*a_val].value_len = sizeof(fval);
		(*a_val)++;
	} else {
		if (flags & GNUTLS_PKCS11_OBJ_FLAG_MARK_PRIVATE) {
			a[*a_val].type = CKA_PRIVATE;
			a[*a_val].value = (void *) &tval;
			a[*a_val].value_len = sizeof(tval);
			(*a_val)++;
		} else if (flags & GNUTLS_PKCS11_OBJ_FLAG_MARK_NOT_PRIVATE) {
			a[*a_val].type = CKA_PRIVATE;
			a[*a_val].value = (void *) &fval;
			a[*a_val].value_len = sizeof(fval);
			(*a_val)++;
		}
	}
}

/**
 * gnutls_pkcs11_copy_x509_crt2:
 * @token_url: A PKCS #11 URL specifying a token
 * @crt: The certificate to copy
 * @label: The name to be used for the stored data
 * @cid: The CKA_ID to set for the object -if NULL, the ID will be derived from the public key
 * @flags: One of GNUTLS_PKCS11_OBJ_FLAG_*
 *
 * This function will copy a certificate into a PKCS #11 token specified by
 * a URL. Valid flags to mark the certificate: %GNUTLS_PKCS11_OBJ_FLAG_MARK_TRUSTED,
 * %GNUTLS_PKCS11_OBJ_FLAG_MARK_SENSITIVE, %GNUTLS_PKCS11_OBJ_FLAG_MARK_PRIVATE,
 * %GNUTLS_PKCS11_OBJ_FLAG_MARK_CA, %GNUTLS_PKCS11_OBJ_FLAG_MARK_ALWAYS_AUTH.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.4.0
 **/
int
gnutls_pkcs11_copy_x509_crt2(const char *token_url,
			    gnutls_x509_crt_t crt, const char *label,
			    const gnutls_datum_t *cid,
			    unsigned int flags)
{
	int ret;
	struct p11_kit_uri *info = NULL;
	ck_rv_t rv;
	size_t der_size, id_size, serial_size;
	gnutls_datum_t serial_der = {NULL, 0};
	uint8_t *der = NULL;
	uint8_t serial[128];
	uint8_t id[20];
	struct ck_attribute a[MAX_ASIZE];
	ck_object_class_t class = CKO_CERTIFICATE;
	ck_certificate_type_t type = CKC_X_509;
	ck_object_handle_t ctx;
	unsigned a_val;
	struct pkcs11_session_info sinfo;
	
	PKCS11_CHECK_INIT;

	ret = pkcs11_url_to_info(token_url, &info, 0);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	ret =
	    pkcs11_open_session(&sinfo, NULL, info,
				SESSION_WRITE |
				pkcs11_obj_flags_to_int(flags));
	p11_kit_uri_free(info);

	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	der_size = 0;
	ret =
	    gnutls_x509_crt_export(crt, GNUTLS_X509_FMT_DER, NULL,
				   &der_size);
	if (ret < 0 && ret != GNUTLS_E_SHORT_MEMORY_BUFFER) {
		gnutls_assert();
		goto cleanup;
	}

	der = gnutls_malloc(der_size);
	if (der == NULL) {
		gnutls_assert();
		ret = GNUTLS_E_MEMORY_ERROR;
		goto cleanup;
	}

	ret =
	    gnutls_x509_crt_export(crt, GNUTLS_X509_FMT_DER, der,
				   &der_size);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	a[0].type = CKA_CLASS;
	a[0].value = &class;
	a[0].value_len = sizeof(class);

	a[1].type = CKA_ID;
	if (cid == NULL || cid->size == 0) {
		id_size = sizeof(id);
		ret = gnutls_x509_crt_get_subject_key_id(crt, id, &id_size, NULL);
		if (ret < 0) {
			id_size = sizeof(id);
			ret = gnutls_x509_crt_get_key_id(crt, 0, id, &id_size);
			if (ret < 0) {
			  gnutls_assert();
			  goto cleanup;
			}
		}

		a[1].value = id;
		a[1].value_len = id_size;
	} else {
		a[1].value = cid->data;
		a[1].value_len = cid->size;
	}

	a[2].type = CKA_VALUE;
	a[2].value = der;
	a[2].value_len = der_size;
	a[3].type = CKA_TOKEN;
	a[3].value = (void *) &tval;
	a[3].value_len = sizeof(tval);
	a[4].type = CKA_CERTIFICATE_TYPE;
	a[4].value = &type;
	a[4].value_len = sizeof(type);
	/* FIXME: copy key usage flags */

	a_val = 5;

	a[a_val].type = CKA_SUBJECT;
	a[a_val].value = crt->raw_dn.data;
	a[a_val].value_len = crt->raw_dn.size;
	a_val++;

	a[a_val].type = CKA_ISSUER;
	a[a_val].value = crt->raw_issuer_dn.data;
	a[a_val].value_len = crt->raw_issuer_dn.size;
	a_val++;

	serial_size = sizeof(serial);
	if (gnutls_x509_crt_get_serial(crt, serial, &serial_size) >= 0) {
		ret = _gnutls_x509_ext_gen_number(serial, serial_size, &serial_der);
		if (ret >= 0) {
			a[a_val].type = CKA_SERIAL_NUMBER;
			a[a_val].value = (void *) serial_der.data;
			a[a_val].value_len = serial_der.size;
			a_val++;
		}
	}

	if (label) {
		a[a_val].type = CKA_LABEL;
		a[a_val].value = (void *) label;
		a[a_val].value_len = strlen(label);
		a_val++;
	}

	mark_flags(flags, a, &a_val, sinfo.trusted);

	rv = pkcs11_create_object(sinfo.module, sinfo.pks, a, a_val, &ctx);
	if (rv != CKR_OK) {
		gnutls_assert();
		_gnutls_debug_log("p11: %s\n", pkcs11_strerror(rv));
		ret = pkcs11_rv_to_err(rv);
		goto cleanup;
	}

	/* generated! 
	 */

	ret = 0;

      cleanup:
	gnutls_free(der);
	gnutls_free(serial_der.data);
	pkcs11_close_session(&sinfo);
	return ret;

}

static void clean_pubkey(struct ck_attribute *a, unsigned a_val)
{
	unsigned i;

	for (i=0;i<a_val;i++) {
		switch(a[i].type) {
			case CKA_MODULUS:
			case CKA_PUBLIC_EXPONENT:
			case CKA_PRIME:
			case CKA_SUBPRIME:
			case CKA_VALUE:
			case CKA_BASE:
			case CKA_EC_PARAMS:
			case CKA_EC_POINT:
				gnutls_free(a[i].value);
				a[i].value = NULL;
				break;
		}
	}
}

static void skip_leading_zeros(gnutls_datum_t *d)
{
	unsigned nr = 0;

	while(nr < d->size && d->data[nr] == 0)
		nr++;
	if (nr > 0) {
		d->size -= nr;
		if (d->size > 0)
			memmove(d->data, &d->data[nr], d->size);
	}
}

static int add_pubkey(gnutls_pubkey_t pubkey, struct ck_attribute *a, unsigned *a_val)
{
	gnutls_pk_algorithm_t pk;
	int ret;

	pk = gnutls_pubkey_get_pk_algorithm(pubkey, NULL);

	switch (pk) {
	case GNUTLS_PK_RSA: {
		gnutls_datum_t m, e;

		ret = gnutls_pubkey_export_rsa_raw(pubkey, &m, &e);
		if (ret < 0) {
			gnutls_assert();
			return ret;
		}

		/* PKCS#11 defines integers as unsigned having most significant byte
		 * first, e.g., 32768 = 0x80 0x00. This is interpreted literraly by
		 * some HSMs which do not accept an integer with a leading zero */
		skip_leading_zeros(&m);
		skip_leading_zeros(&e);

		a[*a_val].type = CKA_MODULUS;
		a[*a_val].value = m.data;
		a[*a_val].value_len = m.size;
		(*a_val)++;

		a[*a_val].type = CKA_PUBLIC_EXPONENT;
		a[*a_val].value = e.data;
		a[*a_val].value_len = e.size;
		(*a_val)++;
		break;
	}
	case GNUTLS_PK_DSA: {
		gnutls_datum_t p, q, g, y;

		ret = gnutls_pubkey_export_dsa_raw(pubkey, &p, &q, &g, &y);
		if (ret < 0) {
			gnutls_assert();
			return ret;
		}

		skip_leading_zeros(&p);
		skip_leading_zeros(&q);
		skip_leading_zeros(&g);
		skip_leading_zeros(&y);

		a[*a_val].type = CKA_PRIME;
		a[*a_val].value = p.data;
		a[*a_val].value_len = p.size;
		(*a_val)++;

		a[*a_val].type = CKA_SUBPRIME;
		a[*a_val].value = q.data;
		a[*a_val].value_len = q.size;
		(*a_val)++;

		a[*a_val].type = CKA_BASE;
		a[*a_val].value = g.data;
		a[*a_val].value_len = g.size;
		(*a_val)++;

		a[*a_val].type = CKA_VALUE;
		a[*a_val].value = y.data;
		a[*a_val].value_len = y.size;
		(*a_val)++;
		break;
	}
	case GNUTLS_PK_EC: {
		gnutls_datum_t params, point;

		ret = gnutls_pubkey_export_ecc_x962(pubkey, &params, &point);
		if (ret < 0) {
			gnutls_assert();
			return ret;
		}

		a[*a_val].type = CKA_EC_PARAMS;
		a[*a_val].value = params.data;
		a[*a_val].value_len = params.size;
		(*a_val)++;

		a[*a_val].type = CKA_EC_POINT;
		a[*a_val].value = point.data;
		a[*a_val].value_len = point.size;
		(*a_val)++;
		break;
	}
	default:
		_gnutls_debug_log("requested writing public key of unsupported type %u\n", (unsigned)pk);
		return gnutls_assert_val(GNUTLS_E_UNIMPLEMENTED_FEATURE);
	}

	return 0;
}

/**
 * gnutls_pkcs11_copy_pubkey:
 * @token_url: A PKCS #11 URL specifying a token
 * @pubkey: The public key to copy
 * @label: The name to be used for the stored data
 * @cid: The CKA_ID to set for the object -if NULL, the ID will be derived from the public key
 * @key_usage: One of GNUTLS_KEY_*
 * @flags: One of GNUTLS_PKCS11_OBJ_FLAG_*
 *
 * This function will copy a public key object into a PKCS #11 token specified by
 * a URL. Valid flags to mark the key: %GNUTLS_PKCS11_OBJ_FLAG_MARK_TRUSTED,
 * %GNUTLS_PKCS11_OBJ_FLAG_MARK_SENSITIVE, %GNUTLS_PKCS11_OBJ_FLAG_MARK_PRIVATE,
 * %GNUTLS_PKCS11_OBJ_FLAG_MARK_CA, %GNUTLS_PKCS11_OBJ_FLAG_MARK_ALWAYS_AUTH.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.4.6
 **/
int
gnutls_pkcs11_copy_pubkey(const char *token_url,
			  gnutls_pubkey_t pubkey, const char *label,
			  const gnutls_datum_t *cid,
			  unsigned int key_usage, unsigned int flags)
{
	int ret;
	struct p11_kit_uri *info = NULL;
	ck_rv_t rv;
	size_t id_size;
	uint8_t id[20];
	struct ck_attribute a[MAX_ASIZE];
	gnutls_pk_algorithm_t pk;
	ck_object_class_t class = CKO_PUBLIC_KEY;
	ck_object_handle_t ctx;
	unsigned a_val;
	ck_key_type_t type;
	struct pkcs11_session_info sinfo;
	
	PKCS11_CHECK_INIT;

	ret = pkcs11_url_to_info(token_url, &info, 0);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	ret =
	    pkcs11_open_session(&sinfo, NULL, info,
				SESSION_WRITE |
				pkcs11_obj_flags_to_int(flags));
	p11_kit_uri_free(info);

	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	a[0].type = CKA_CLASS;
	a[0].value = &class;
	a[0].value_len = sizeof(class);

	a[1].type = CKA_TOKEN;
	a[1].value = (void *) &tval;
	a[1].value_len = sizeof(tval);

	a_val = 2;

	ret = add_pubkey(pubkey, a, &a_val);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	if (label) {
		a[a_val].type = CKA_LABEL;
		a[a_val].value = (void *) label;
		a[a_val].value_len = strlen(label);
		a_val++;
	}

	pk = gnutls_pubkey_get_pk_algorithm(pubkey, NULL);
	type = pk_to_key_type(pk);
	FIX_KEY_USAGE(pk, key_usage);

	a[a_val].type = CKA_KEY_TYPE;
	a[a_val].value = &type;
	a[a_val].value_len = sizeof(type);
	a_val++;

	a[a_val].type = CKA_ID;
	if (cid == NULL || cid->size == 0) {
		id_size = sizeof(id);
		ret = gnutls_pubkey_get_key_id(pubkey, 0, id, &id_size);
		if (ret < 0) {
			  gnutls_assert();
			  goto cleanup;
		}

		a[a_val].value = id;
		a[a_val].value_len = id_size;
	} else {
		a[a_val].value = cid->data;
		a[a_val].value_len = cid->size;
	}
	a_val++;

	mark_flags(flags, a, &a_val, sinfo.trusted);

	a[a_val].type = CKA_VERIFY;
	if (key_usage & GNUTLS_KEY_DIGITAL_SIGNATURE) {
		a[a_val].value = (void*)&tval;
		a[a_val].value_len = sizeof(tval);
	} else {
		a[a_val].value = (void*)&fval;
		a[a_val].value_len = sizeof(fval);
	}
	a_val++;

	if (pk == GNUTLS_PK_RSA) {
		a[a_val].type = CKA_ENCRYPT;
		if (key_usage & (GNUTLS_KEY_ENCIPHER_ONLY|GNUTLS_KEY_DECIPHER_ONLY)) {
			a[a_val].value = (void*)&tval;
			a[a_val].value_len = sizeof(tval);
		} else {
			a[a_val].value = (void*)&fval;
			a[a_val].value_len = sizeof(fval);
		}
		a_val++;
	}

	rv = pkcs11_create_object(sinfo.module, sinfo.pks, a, a_val, &ctx);
	if (rv != CKR_OK) {
		gnutls_assert();
		_gnutls_debug_log("p11: %s\n", pkcs11_strerror(rv));
		ret = pkcs11_rv_to_err(rv);
		goto cleanup;
	}

	/* generated! 
	 */

	ret = 0;

      cleanup:
	clean_pubkey(a, a_val);
	pkcs11_close_session(&sinfo);
	return ret;

}


/**
 * gnutls_pkcs11_copy_attached_extension:
 * @token_url: A PKCS #11 URL specifying a token
 * @crt: An X.509 certificate object
 * @data: the attached extension
 * @label: A name to be used for the attached extension (may be %NULL)
 * @flags: One of GNUTLS_PKCS11_OBJ_FLAG_*
 *
 * This function will copy an the attached extension in @data for
 * the certificate provided in @crt in the PKCS #11 token specified
 * by the URL (typically a trust module). The extension must be in
 * RFC5280 Extension format.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.3.8
 **/
int
gnutls_pkcs11_copy_attached_extension(const char *token_url,
		       gnutls_x509_crt_t crt,
		       gnutls_datum_t *data,
		       const char *label,
		       unsigned int flags)
{
	int ret;
	struct p11_kit_uri *info = NULL;
	ck_rv_t rv;
	struct ck_attribute a[MAX_ASIZE];
	ck_object_handle_t ctx;
	unsigned a_vals;
	struct pkcs11_session_info sinfo;
	ck_object_class_t class;
	gnutls_datum_t spki = {NULL, 0};
	
	PKCS11_CHECK_INIT;

	ret = pkcs11_url_to_info(token_url, &info, 0);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	ret =
	    pkcs11_open_session(&sinfo, NULL, info,
				SESSION_WRITE |
				pkcs11_obj_flags_to_int(flags));
	p11_kit_uri_free(info);

	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	ret = x509_crt_to_raw_pubkey(crt, &spki);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	class = CKO_X_CERTIFICATE_EXTENSION;
	a_vals = 0;
	a[a_vals].type = CKA_CLASS;
	a[a_vals].value = &class;
	a[a_vals++].value_len = sizeof(class);

	a[a_vals].type = CKA_PUBLIC_KEY_INFO;
	a[a_vals].value = spki.data;
	a[a_vals++].value_len = spki.size;

	a[a_vals].type = CKA_VALUE;
	a[a_vals].value = data->data;
	a[a_vals++].value_len = data->size;

	a[a_vals].type = CKA_TOKEN;
	a[a_vals].value = (void *) &tval;
	a[a_vals++].value_len = sizeof(tval);

	if (label) {
		a[a_vals].type = CKA_LABEL;
		a[a_vals].value = (void *) label;
		a[a_vals++].value_len = strlen(label);
	}

	rv = pkcs11_create_object(sinfo.module, sinfo.pks, a, a_vals, &ctx);
	if (rv != CKR_OK) {
		gnutls_assert();
		_gnutls_debug_log("p11: %s\n", pkcs11_strerror(rv));
		ret = pkcs11_rv_to_err(rv);
		goto cleanup;
	}

	ret = 0;

      cleanup:
	pkcs11_close_session(&sinfo);
	gnutls_free(spki.data);
	return ret;

}

/**
 * gnutls_pkcs11_copy_x509_privkey2:
 * @token_url: A PKCS #11 URL specifying a token
 * @key: A private key
 * @label: A name to be used for the stored data
 * @cid: The CKA_ID to set for the object -if NULL, the ID will be derived from the public key
 * @key_usage: One of GNUTLS_KEY_*
 * @flags: One of GNUTLS_PKCS11_OBJ_* flags
 *
 * This function will copy a private key into a PKCS #11 token specified by
 * a URL. It is highly recommended flags to contain %GNUTLS_PKCS11_OBJ_FLAG_MARK_SENSITIVE
 * unless there is a strong reason not to.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.4.0
 **/
int
gnutls_pkcs11_copy_x509_privkey2(const char *token_url,
				gnutls_x509_privkey_t key,
				const char *label,
				const gnutls_datum_t *cid,
				unsigned int key_usage, unsigned int flags)
{
	int ret;
	struct p11_kit_uri *info = NULL;
	ck_rv_t rv;
	size_t id_size;
	uint8_t id[20];
	struct ck_attribute a[32];
	ck_object_class_t class = CKO_PRIVATE_KEY;
	ck_object_handle_t ctx;
	ck_key_type_t type;
	int a_val;
	gnutls_pk_algorithm_t pk;
	gnutls_datum_t p, q, g, y, x;
	gnutls_datum_t m, e, d, u, exp1, exp2;
	struct pkcs11_session_info sinfo;

	PKCS11_CHECK_INIT;

	memset(&p, 0, sizeof(p));
	memset(&q, 0, sizeof(q));
	memset(&g, 0, sizeof(g));
	memset(&y, 0, sizeof(y));
	memset(&x, 0, sizeof(x));
	memset(&m, 0, sizeof(m));
	memset(&e, 0, sizeof(e));
	memset(&d, 0, sizeof(d));
	memset(&u, 0, sizeof(u));
	memset(&exp1, 0, sizeof(exp1));
	memset(&exp2, 0, sizeof(exp2));

	ret = pkcs11_url_to_info(token_url, &info, 0);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	ret =
	    pkcs11_open_session(&sinfo, NULL, info,
				SESSION_WRITE |
				pkcs11_obj_flags_to_int(flags));
	p11_kit_uri_free(info);

	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	pk = gnutls_x509_privkey_get_pk_algorithm(key);
	FIX_KEY_USAGE(pk, key_usage);

	/* FIXME: copy key usage flags */
	a_val = 0;
	a[a_val].type = CKA_CLASS;
	a[a_val].value = &class;
	a[a_val].value_len = sizeof(class);
	a_val++;

	a[a_val].type = CKA_ID;
	if (cid == NULL || cid->size == 0) {
		id_size = sizeof(id);
		ret = gnutls_x509_privkey_get_key_id(key, 0, id, &id_size);
		if (ret < 0) {
			p11_kit_uri_free(info);
			gnutls_assert();
			return ret;
		}

		a[a_val].value = id;
		a[a_val].value_len = id_size;
	} else {
		a[a_val].value = cid->data;
		a[a_val].value_len = cid->size;
	}
	a_val++;

	a[a_val].type = CKA_SIGN;
	if (key_usage & GNUTLS_KEY_DIGITAL_SIGNATURE) {
		a[a_val].value = (void*)&tval;
		a[a_val].value_len = sizeof(tval);
	} else {
		a[a_val].value = (void*)&fval;
		a[a_val].value_len = sizeof(fval);
	}
	a_val++;

	if (pk == GNUTLS_PK_RSA) {
		a[a_val].type = CKA_DECRYPT;
		if (key_usage & (GNUTLS_KEY_ENCIPHER_ONLY|GNUTLS_KEY_DECIPHER_ONLY)) {
			a[a_val].value = (void*)&tval;
			a[a_val].value_len = sizeof(tval);
		} else {
			a[a_val].value = (void*)&fval;
			a[a_val].value_len = sizeof(fval);
		}
		a_val++;
	}

	a[a_val].type = CKA_TOKEN;
	a[a_val].value = (void *) &tval;
	a[a_val].value_len = sizeof(tval);
	a_val++;

	/* a private key is set always as private unless
	 * requested otherwise
	 */
	if (flags & GNUTLS_PKCS11_OBJ_FLAG_MARK_NOT_PRIVATE) {
		a[a_val].type = CKA_PRIVATE;
		a[a_val].value = (void *) &fval;
		a[a_val].value_len = sizeof(fval);
		a_val++;
	} else {
		a[a_val].type = CKA_PRIVATE;
		a[a_val].value = (void *) &tval;
		a[a_val].value_len = sizeof(tval);
		a_val++;
	}

	if (flags & GNUTLS_PKCS11_OBJ_FLAG_MARK_ALWAYS_AUTH) {
		a[a_val].type = CKA_ALWAYS_AUTHENTICATE;
		a[a_val].value = (void *) &tval;
		a[a_val].value_len = sizeof(tval);
		a_val++;
	}

	if (flags & GNUTLS_PKCS11_OBJ_FLAG_MARK_EXTRACTABLE) {
		a[a_val].type = CKA_EXTRACTABLE;
		a[a_val].value = (void *) &tval;
		a[a_val].value_len = sizeof(tval);
		(a_val)++;
	} else {
		a[a_val].type = CKA_EXTRACTABLE;
		a[a_val].value = (void *) &fval;
		a[a_val].value_len = sizeof(fval);
		(a_val)++;
	}

	if (label) {
		a[a_val].type = CKA_LABEL;
		a[a_val].value = (void *) label;
		a[a_val].value_len = strlen(label);
		a_val++;
	}

	if (flags & GNUTLS_PKCS11_OBJ_FLAG_MARK_SENSITIVE) {
		a[a_val].type = CKA_SENSITIVE;
		a[a_val].value = (void *) &tval;
		a[a_val].value_len = sizeof(tval);
		a_val++;
	} else {
		a[a_val].type = CKA_SENSITIVE;
		a[a_val].value = (void *) &fval;
		a[a_val].value_len = sizeof(fval);
		a_val++;
	}

	switch (pk) {
	case GNUTLS_PK_RSA:
		{

			ret =
			    gnutls_x509_privkey_export_rsa_raw2(key, &m,
								&e, &d, &p,
								&q, &u,
								&exp1,
								&exp2);
			if (ret < 0) {
				gnutls_assert();
				goto cleanup;
			}

			type = CKK_RSA;

			skip_leading_zeros(&m);
			skip_leading_zeros(&e);
			skip_leading_zeros(&d);
			skip_leading_zeros(&p);
			skip_leading_zeros(&q);
			skip_leading_zeros(&u);
			skip_leading_zeros(&exp1);
			skip_leading_zeros(&exp2);

			a[a_val].type = CKA_MODULUS;
			a[a_val].value = m.data;
			a[a_val].value_len = m.size;
			a_val++;

			a[a_val].type = CKA_PUBLIC_EXPONENT;
			a[a_val].value = e.data;
			a[a_val].value_len = e.size;
			a_val++;

			a[a_val].type = CKA_PRIVATE_EXPONENT;
			a[a_val].value = d.data;
			a[a_val].value_len = d.size;
			a_val++;

			a[a_val].type = CKA_PRIME_1;
			a[a_val].value = p.data;
			a[a_val].value_len = p.size;
			a_val++;

			a[a_val].type = CKA_PRIME_2;
			a[a_val].value = q.data;
			a[a_val].value_len = q.size;
			a_val++;

			a[a_val].type = CKA_COEFFICIENT;
			a[a_val].value = u.data;
			a[a_val].value_len = u.size;
			a_val++;

			a[a_val].type = CKA_EXPONENT_1;
			a[a_val].value = exp1.data;
			a[a_val].value_len = exp1.size;
			a_val++;

			a[a_val].type = CKA_EXPONENT_2;
			a[a_val].value = exp2.data;
			a[a_val].value_len = exp2.size;
			a_val++;

			break;
		}
	case GNUTLS_PK_DSA:
		{
			ret =
			    gnutls_x509_privkey_export_dsa_raw(key, &p, &q,
							       &g, &y, &x);
			if (ret < 0) {
				gnutls_assert();
				goto cleanup;
			}

			type = CKK_DSA;

			skip_leading_zeros(&p);
			skip_leading_zeros(&q);
			skip_leading_zeros(&g);
			skip_leading_zeros(&y);
			skip_leading_zeros(&x);

			a[a_val].type = CKA_PRIME;
			a[a_val].value = p.data;
			a[a_val].value_len = p.size;
			a_val++;

			a[a_val].type = CKA_SUBPRIME;
			a[a_val].value = q.data;
			a[a_val].value_len = q.size;
			a_val++;

			a[a_val].type = CKA_BASE;
			a[a_val].value = g.data;
			a[a_val].value_len = g.size;
			a_val++;

			a[a_val].type = CKA_VALUE;
			a[a_val].value = x.data;
			a[a_val].value_len = x.size;
			a_val++;

			break;
		}
	case GNUTLS_PK_EC:
		{
			ret =
			    _gnutls_x509_write_ecc_params(key->params.flags,
							  &p);
			if (ret < 0) {
				gnutls_assert();
				goto cleanup;
			}

			ret =
			    _gnutls_mpi_dprint(key->params.
						params[ECC_K], &x);
			if (ret < 0) {
				gnutls_assert();
				goto cleanup;
			}

			type = CKK_ECDSA;

			a[a_val].type = CKA_EC_PARAMS;
			a[a_val].value = p.data;
			a[a_val].value_len = p.size;
			a_val++;

			a[a_val].type = CKA_VALUE;
			a[a_val].value = x.data;
			a[a_val].value_len = x.size;
			a_val++;

			break;
		}
	default:
		gnutls_assert();
		ret = GNUTLS_E_INVALID_REQUEST;
		goto cleanup;
	}

	a[a_val].type = CKA_KEY_TYPE;
	a[a_val].value = &type;
	a[a_val].value_len = sizeof(type);
	a_val++;

	rv = pkcs11_create_object(sinfo.module, sinfo.pks, a, a_val, &ctx);
	if (rv != CKR_OK) {
		gnutls_assert();
		_gnutls_debug_log("p11: %s\n", pkcs11_strerror(rv));
		ret = pkcs11_rv_to_err(rv);
		goto cleanup;
	}

	ret = 0;

      cleanup:
	switch (pk) {
	case GNUTLS_PK_RSA:
		{
			gnutls_free(m.data);
			gnutls_free(e.data);
			gnutls_free(d.data);
			gnutls_free(p.data);
			gnutls_free(q.data);
			gnutls_free(u.data);
			gnutls_free(exp1.data);
			gnutls_free(exp2.data);
			break;
		}
	case GNUTLS_PK_DSA:
		{
			gnutls_free(p.data);
			gnutls_free(q.data);
			gnutls_free(g.data);
			gnutls_free(y.data);
			gnutls_free(x.data);
			break;
		}
	case GNUTLS_PK_EC:
		{
			gnutls_free(p.data);
			gnutls_free(x.data);
			break;
		}
	default:
		gnutls_assert();
		ret = GNUTLS_E_INVALID_REQUEST;
		break;
	}

	if (sinfo.pks != 0)
		pkcs11_close_session(&sinfo);

	return ret;

}

struct delete_data_st {
	struct p11_kit_uri *info;
	unsigned int deleted;	/* how many */
};

static int
delete_obj_url_cb(struct ck_function_list *module, struct pkcs11_session_info *sinfo,
		  struct ck_token_info *tinfo,
		  struct ck_info *lib_info, void *input)
{
	struct delete_data_st *find_data = input;
	struct ck_attribute a[4];
	struct ck_attribute *attr;
	ck_object_class_t class;
	ck_certificate_type_t type = (ck_certificate_type_t) - 1;
	ck_rv_t rv;
	ck_object_handle_t ctx;
	unsigned long count, a_vals;
	int found = 0, ret;

	if (tinfo == NULL) {	/* we don't support multiple calls */
		gnutls_assert();
		return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
	}

	/* do not bother reading the token if basic fields do not match
	 */
	if (!p11_kit_uri_match_module_info(find_data->info, lib_info) ||
	    !p11_kit_uri_match_token_info(find_data->info, tinfo)) {
		gnutls_assert();
		return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
	}

	/* Find objects with given class and type */
	class = CKO_CERTIFICATE;	/* default  */
	a_vals = 0;

	attr = p11_kit_uri_get_attribute(find_data->info, CKA_CLASS);
	if (attr != NULL) {
		if (attr->value
		    && attr->value_len == sizeof(ck_object_class_t))
			class = *((ck_object_class_t *) attr->value);
		if (class == CKO_CERTIFICATE)
			type = CKC_X_509;

		a[a_vals].type = CKA_CLASS;
		a[a_vals].value = &class;
		a[a_vals].value_len = sizeof(class);
		a_vals++;
	}

	attr = p11_kit_uri_get_attribute(find_data->info, CKA_ID);
	if (attr != NULL) {
		memcpy(a + a_vals, attr, sizeof(struct ck_attribute));
		a_vals++;
	}

	if (type != (ck_certificate_type_t) - 1) {
		a[a_vals].type = CKA_CERTIFICATE_TYPE;
		a[a_vals].value = &type;
		a[a_vals].value_len = sizeof type;
		a_vals++;
	}

	attr = p11_kit_uri_get_attribute(find_data->info, CKA_LABEL);
	if (attr != NULL) {
		memcpy(a + a_vals, attr, sizeof(struct ck_attribute));
		a_vals++;
	}

	rv = pkcs11_find_objects_init(sinfo->module, sinfo->pks, a,
				      a_vals);
	if (rv != CKR_OK) {
		gnutls_assert();
		_gnutls_debug_log("p11: FindObjectsInit failed.\n");
		ret = pkcs11_rv_to_err(rv);
		goto cleanup;
	}

	while (pkcs11_find_objects
	       (sinfo->module, sinfo->pks, &ctx, 1, &count) == CKR_OK
	       && count == 1) {
		rv = pkcs11_destroy_object(sinfo->module, sinfo->pks, ctx);
		if (rv != CKR_OK) {
			_gnutls_debug_log
			    ("p11: Cannot destroy object: %s\n",
			     pkcs11_strerror(rv));
		} else {
			find_data->deleted++;
		}

		found = 1;
	}

	if (found == 0) {
		gnutls_assert();
		ret = GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
	} else {
		ret = 0;
	}

      cleanup:
	pkcs11_find_objects_final(sinfo);

	return ret;
}


/**
 * gnutls_pkcs11_delete_url:
 * @object_url: The URL of the object to delete.
 * @flags: One of GNUTLS_PKCS11_OBJ_* flags
 * 
 * This function will delete objects matching the given URL.
 * Note that not all tokens support the delete operation.
 *
 * Returns: On success, the number of objects deleted is returned, otherwise a
 *   negative error value.
 *
 * Since: 2.12.0
 **/
int gnutls_pkcs11_delete_url(const char *object_url, unsigned int flags)
{
	int ret;
	struct delete_data_st find_data;

	PKCS11_CHECK_INIT;

	memset(&find_data, 0, sizeof(find_data));

	ret = pkcs11_url_to_info(object_url, &find_data.info, 0);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	ret =
	    _pkcs11_traverse_tokens(delete_obj_url_cb, &find_data,
				    find_data.info, NULL,
				    SESSION_WRITE |
				    pkcs11_obj_flags_to_int(flags));
	p11_kit_uri_free(find_data.info);

	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	return find_data.deleted;

}

/**
 * gnutls_pkcs11_token_init:
 * @token_url: A PKCS #11 URL specifying a token
 * @so_pin: Security Officer's PIN
 * @label: A name to be used for the token
 *
 * This function will initialize (format) a token. If the token is
 * at a factory defaults state the security officer's PIN given will be
 * set to be the default. Otherwise it should match the officer's PIN.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_pkcs11_token_init(const char *token_url,
			 const char *so_pin, const char *label)
{
	int ret;
	struct p11_kit_uri *info = NULL;
	ck_rv_t rv;
	struct ck_function_list *module;
	ck_slot_id_t slot;
	char flabel[32];

	PKCS11_CHECK_INIT;

	ret = pkcs11_url_to_info(token_url, &info, 0);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	ret = pkcs11_find_slot(&module, &slot, info, NULL, NULL, NULL);
	p11_kit_uri_free(info);

	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	/* so it seems memset has other uses than zeroing! */
	memset(flabel, ' ', sizeof(flabel));
	if (label != NULL)
		memcpy(flabel, label, strlen(label));

	rv = pkcs11_init_token(module, slot, (uint8_t *) so_pin,
			       strlen(so_pin), (uint8_t *) flabel);
	if (rv != CKR_OK) {
		gnutls_assert();
		_gnutls_debug_log("p11: %s\n", pkcs11_strerror(rv));
		return pkcs11_rv_to_err(rv);
	}

	return 0;

}

/**
 * gnutls_pkcs11_token_set_pin:
 * @token_url: A PKCS #11 URL specifying a token
 * @oldpin: old user's PIN
 * @newpin: new user's PIN
 * @flags: one of #gnutls_pin_flag_t.
 *
 * This function will modify or set a user's PIN for the given token. 
 * If it is called to set a user pin for first time the oldpin must
 * be NULL.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_pkcs11_token_set_pin(const char *token_url,
			    const char *oldpin,
			    const char *newpin, unsigned int flags)
{
	int ret;
	struct p11_kit_uri *info = NULL;
	ck_rv_t rv;
	unsigned int ses_flags;
	struct pkcs11_session_info sinfo;

	PKCS11_CHECK_INIT;

	ret = pkcs11_url_to_info(token_url, &info, 0);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	if (((flags & GNUTLS_PIN_USER) && oldpin == NULL) ||
	    (flags & GNUTLS_PIN_SO))
		ses_flags = SESSION_WRITE | SESSION_LOGIN | SESSION_SO;
	else
		ses_flags = SESSION_WRITE | SESSION_LOGIN;

	ret = pkcs11_open_session(&sinfo, NULL, info, ses_flags);
	p11_kit_uri_free(info);

	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	if (oldpin == NULL) {
		rv = pkcs11_init_pin(sinfo.module, sinfo.pks,
				     (uint8_t *) newpin, strlen(newpin));
		if (rv != CKR_OK) {
			gnutls_assert();
			_gnutls_debug_log("p11: %s\n",
					  pkcs11_strerror(rv));
			ret = pkcs11_rv_to_err(rv);
			goto finish;
		}
	} else {
		rv = pkcs11_set_pin(sinfo.module, sinfo.pks,
				    oldpin, strlen(oldpin),
				    newpin, strlen(newpin));
		if (rv != CKR_OK) {
			gnutls_assert();
			_gnutls_debug_log("p11: %s\n",
					  pkcs11_strerror(rv));
			ret = pkcs11_rv_to_err(rv);
			goto finish;
		}
	}

	ret = 0;

      finish:
	pkcs11_close_session(&sinfo);
	return ret;

}

/**
 * gnutls_pkcs11_token_get_random:
 * @token_url: A PKCS #11 URL specifying a token
 * @len:       The number of bytes of randomness to request
 * @rnddata:   A pointer to the memory area to be filled with random data
 *
 * This function will get random data from the given token.
 * It will store rnddata and fill the memory pointed to by rnddata with
 * len random bytes from the token.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_pkcs11_token_get_random(const char *token_url,
			       void *rnddata, size_t len)
{
	int ret;
	struct p11_kit_uri *info = NULL;
	ck_rv_t rv;
	struct pkcs11_session_info sinfo;

	PKCS11_CHECK_INIT;

	ret = pkcs11_url_to_info(token_url, &info, 0);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	ret = pkcs11_open_session(&sinfo, NULL, info, 0);
	p11_kit_uri_free(info);

	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	rv = _gnutls_pkcs11_get_random(sinfo.module, sinfo.pks, rnddata, len);
	if (rv != CKR_OK) {
		gnutls_assert();
		_gnutls_debug_log("p11: %s\n", pkcs11_strerror(rv));
		ret = pkcs11_rv_to_err(rv);
		goto finish;
	}

	ret = 0;

      finish:
	pkcs11_close_session(&sinfo);
	return ret;

}

#if 0
/* For documentation purposes */


/**
 * gnutls_pkcs11_copy_x509_crt:
 * @token_url: A PKCS #11 URL specifying a token
 * @crt: A certificate
 * @label: A name to be used for the stored data
 * @flags: One of GNUTLS_PKCS11_OBJ_FLAG_*
 *
 * This function will copy a certificate into a PKCS #11 token specified by
 * a URL. The certificate can be marked as trusted or not.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 2.12.0
 **/
int gnutls_pkcs11_copy_x509_crt(const char *token_url,
			    gnutls_x509_crt_t crt, const char *label,
			    unsigned int flags)
{
	int x;
}

/**
 * gnutls_pkcs11_copy_x509_privkey:
 * @token_url: A PKCS #11 URL specifying a token
 * @key: A private key
 * @label: A name to be used for the stored data
 * @key_usage: One of GNUTLS_KEY_*
 * @flags: One of GNUTLS_PKCS11_OBJ_* flags
 *
 * This function will copy a private key into a PKCS #11 token specified by
 * a URL. It is highly recommended flags to contain %GNUTLS_PKCS11_OBJ_FLAG_MARK_SENSITIVE
 * unless there is a strong reason not to.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 2.12.0
 **/
int gnutls_pkcs11_copy_x509_privkey(const char *token_url,
				gnutls_x509_privkey_t key,
				const char *label,
				unsigned int key_usage, unsigned int flags)
{
	int x;
}

#endif
