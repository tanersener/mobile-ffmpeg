/*
 * GnuTLS PKCS#11 support
 * Copyright (C) 2010-2016 Free Software Foundation, Inc.
 * Copyright (C) 2016 Red Hat
 * 
 * Authors: Nikos Mavrogiannopoulos
 *
 * GnuTLS is free software; you can redistribute it and/or
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
 */

#include "gnutls_int.h"
#include <gnutls/pkcs11.h>
#include <global.h>
#include "errors.h"
#include "x509/common.h"

#include <pkcs11_int.h>
#include <p11-kit/p11-kit.h>
#include "pkcs11x.h"

struct find_ext_data_st {
	/* in */
	gnutls_pkcs11_obj_t obj;
	gnutls_datum_t spki;

	/* out */
	gnutls_x509_ext_st *exts;
	unsigned int exts_size;
};

static int override_ext(gnutls_x509_crt_t crt, gnutls_datum_t *ext)
{
	gnutls_x509_ext_st parsed;
	int ret;

	ret = _gnutls_x509_decode_ext(ext, &parsed);
	if (ret < 0)
		return gnutls_assert_val(ret);

	/* set the new extension */
	ret = _gnutls_x509_crt_set_extension(crt, parsed.oid, &parsed.data, parsed.critical);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}
	
	ret = 0;
 cleanup:
	gnutls_x509_ext_deinit(&parsed);
	return ret;
}

/* This function re-encodes a certificate to contain its stapled extensions.
 * That assumes that the certificate is not in the distrusted list.
 */
int pkcs11_override_cert_exts(struct pkcs11_session_info *sinfo, gnutls_datum_t *spki, gnutls_datum_t *der)
{
	int ret;
	gnutls_datum_t new_der = {NULL, 0};
	struct ck_attribute a[2];
	struct ck_attribute b[1];
	unsigned long count;
	unsigned ext_data_size = der->size;
	uint8_t *ext_data = NULL;
	ck_object_class_t class = -1;
	gnutls_x509_crt_t crt = NULL;
	unsigned finalize = 0;
	ck_rv_t rv;
	ck_object_handle_t obj;

	if (sinfo->trusted == 0) {
		_gnutls_debug_log("p11: cannot override extensions on a non-p11-kit trust module\n");
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);
	}

	/* retrieve the extensions */
	class = CKO_X_CERTIFICATE_EXTENSION;
	a[0].type = CKA_CLASS;
	a[0].value = &class;
	a[0].value_len = sizeof class;

	a[1].type = CKA_PUBLIC_KEY_INFO;
	a[1].value = spki->data;
	a[1].value_len = spki->size;

	rv = pkcs11_find_objects_init(sinfo->module, sinfo->pks, a, 2);
	if (rv != CKR_OK) {
		gnutls_assert();
		_gnutls_debug_log
		    ("p11: FindObjectsInit failed for cert extensions.\n");
		ret = pkcs11_rv_to_err(rv);
		goto cleanup;
	}
	finalize = 1;

	rv = pkcs11_find_objects(sinfo->module, sinfo->pks, &obj, 1, &count);
	if (rv == CKR_OK && count == 1) {
		ext_data = gnutls_malloc(ext_data_size);
		if (ext_data == NULL) {
			gnutls_assert();
			ret = GNUTLS_E_MEMORY_ERROR;
			goto cleanup;
		}

		ret = gnutls_x509_crt_init(&crt);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		ret = gnutls_x509_crt_import(crt, der, GNUTLS_X509_FMT_DER);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		do {

			b[0].type = CKA_VALUE;
			b[0].value = ext_data;
			b[0].value_len = ext_data_size;

			if (pkcs11_get_attribute_value
			    (sinfo->module, sinfo->pks, obj, b, 1) == CKR_OK) {
				gnutls_datum_t data = { b[0].value, b[0].value_len };

				ret = override_ext(crt, &data);
				if (ret < 0) {
					gnutls_assert();
					goto cleanup;
				}
			}
		} while (pkcs11_find_objects(sinfo->module, sinfo->pks, &obj, 1, &count) == CKR_OK && count == 1);

		/* overwrite the old certificate with the new */
		ret = gnutls_x509_crt_export2(crt, GNUTLS_X509_FMT_DER, &new_der);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		gnutls_free(der->data);
		der->data = new_der.data;
		der->size = new_der.size;
	}

	ret = 0;
 cleanup:
	if (crt != NULL)
		gnutls_x509_crt_deinit(crt);
	if (finalize != 0)
		pkcs11_find_objects_final(sinfo);
	gnutls_free(ext_data);
	return ret;

}

static int
find_ext_cb(struct ck_function_list *module, struct pkcs11_session_info *sinfo,
	    struct ck_token_info *tinfo, struct ck_info *lib_info,
	    void *input)
{
	struct find_ext_data_st *find_data = input;
	struct ck_attribute a[4];
	ck_object_class_t class = -1;
	unsigned long count;
	ck_rv_t rv;
	ck_object_handle_t obj;
	int ret;
	gnutls_datum_t ext;

	if (tinfo == NULL) {	/* we don't support multiple calls */
		gnutls_assert();
		return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
	}

	/* do not bother reading the token if basic fields do not match
	 */
	if (!p11_kit_uri_match_token_info
	    (find_data->obj->info, tinfo)
	    || !p11_kit_uri_match_module_info(find_data->obj->info,
					      lib_info)) {
		gnutls_assert();
		return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
	}

	/* retrieve the extensions */
	class = CKO_X_CERTIFICATE_EXTENSION;
	a[0].type = CKA_CLASS;
	a[0].value = &class;
	a[0].value_len = sizeof class;

	a[1].type = CKA_PUBLIC_KEY_INFO;
	a[1].value = find_data->spki.data;
	a[1].value_len = find_data->spki.size;

	rv = pkcs11_find_objects_init(sinfo->module, sinfo->pks, a, 2);
	if (rv != CKR_OK) {
		gnutls_assert();
		_gnutls_debug_log
		    ("p11: FindObjectsInit failed for cert extensions.\n");
		return pkcs11_rv_to_err(rv);
	}

	while(pkcs11_find_objects(sinfo->module, sinfo->pks, &obj, 1, &count) == CKR_OK && count == 1) {
		rv = pkcs11_get_attribute_avalue(sinfo->module, sinfo->pks, obj, CKA_VALUE, &ext);
		if (rv == CKR_OK) {

			find_data->exts = gnutls_realloc_fast(find_data->exts, (1+find_data->exts_size)*sizeof(find_data->exts[0]));
			if (find_data->exts == NULL) {
				gnutls_assert();
				ret = pkcs11_rv_to_err(rv);
				goto cleanup;
			}

			if (_gnutls_x509_decode_ext(&ext, &find_data->exts[find_data->exts_size]) == 0) {
				find_data->exts_size++;
			}
			gnutls_free(ext.data);
		}
	}

	ret = 0;
 cleanup:
	pkcs11_find_objects_final(sinfo);
	return ret;
}

/**
 * gnutls_pkcs11_obj_get_exts:
 * @obj: should contain a #gnutls_pkcs11_obj_t type
 * @exts: a pointer to a %gnutls_x509_ext_st pointer
 * @exts_size: will be updated with the number of @exts
 * @flags: Or sequence of %GNUTLS_PKCS11_OBJ_* flags 
 *
 * This function will return information about attached extensions
 * that associate to the provided object (which should be a certificate).
 * The extensions are the attached p11-kit trust module extensions.
 *
 * Each element of @exts must be deinitialized using gnutls_x509_ext_deinit()
 * while @exts should be deallocated using gnutls_free().
 *
 * Returns: %GNUTLS_E_SUCCESS (0) on success or a negative error code on error.
 *
 * Since: 3.3.8
 **/
int
gnutls_pkcs11_obj_get_exts(gnutls_pkcs11_obj_t obj,
			   gnutls_x509_ext_st **exts, unsigned int *exts_size,
			   unsigned int flags)
{
	int ret;
	gnutls_datum_t spki = {NULL, 0};
	struct find_ext_data_st find_data;
	unsigned deinit_spki = 0;

	PKCS11_CHECK_INIT;
	memset(&find_data, 0, sizeof(find_data));

	*exts_size = 0;

	if (obj->type != GNUTLS_PKCS11_OBJ_X509_CRT && obj->type != GNUTLS_PKCS11_OBJ_PUBKEY)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	if (obj->type == GNUTLS_PKCS11_OBJ_PUBKEY) {
		spki.data = obj->raw.data;
		spki.size = obj->raw.size;
	} else {
		ret = x509_raw_crt_to_raw_pubkey(&obj->raw, &spki);
		if (ret < 0)
			return gnutls_assert_val(ret);
		deinit_spki = 1;
	}

	find_data.spki.data = spki.data;
	find_data.spki.size = spki.size;
	find_data.obj = obj;
	ret =
	    _pkcs11_traverse_tokens(find_ext_cb, &find_data, obj->info,
				    &obj->pin,
				    pkcs11_obj_flags_to_int(flags));
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	*exts = find_data.exts;
	*exts_size = find_data.exts_size;

	ret = 0;
 cleanup:
	if (deinit_spki)
		gnutls_free(spki.data);
	return ret;
}

