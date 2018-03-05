/*
 * GnuTLS PKCS#11 support
 * Copyright (C) 2010-2012 Free Software Foundation, Inc.
 * 
 * Author: Nikos Mavrogiannopoulos, Stef Walter
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
 */

#include "gnutls_int.h"
#include <gnutls/pkcs11.h>
#include <stdio.h>
#include <string.h>
#include "errors.h"
#include <datum.h>
#include <pkcs11_int.h>
#include <random.h>

/**
 * gnutls_pkcs11_copy_secret_key:
 * @token_url: A PKCS #11 URL specifying a token
 * @key: The raw key
 * @label: A name to be used for the stored data
 * @key_usage: One of GNUTLS_KEY_*
 * @flags: One of GNUTLS_PKCS11_OBJ_FLAG_*
 *
 * This function will copy a raw secret (symmetric) key into a PKCS #11 
 * token specified by a URL. The key can be marked as sensitive or not.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 2.12.0
 **/
int
gnutls_pkcs11_copy_secret_key(const char *token_url, gnutls_datum_t * key,
			      const char *label,
			      unsigned int key_usage, unsigned int flags
			      /* GNUTLS_PKCS11_OBJ_FLAG_* */ )
{
	int ret;
	struct p11_kit_uri *info = NULL;
	ck_rv_t rv;
	struct ck_attribute a[12];
	ck_object_class_t class = CKO_SECRET_KEY;
	ck_object_handle_t ctx;
	ck_key_type_t keytype = CKK_GENERIC_SECRET;
	ck_bool_t tval = 1;
	int a_val;
	uint8_t id[16];
	struct pkcs11_session_info sinfo;
	
	PKCS11_CHECK_INIT;

	memset(&sinfo, 0, sizeof(sinfo));

	ret = pkcs11_url_to_info(token_url, &info, 0);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	/* generate a unique ID */
	ret = gnutls_rnd(GNUTLS_RND_NONCE, id, sizeof(id));
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

	/* FIXME: copy key usage flags */

	a[0].type = CKA_CLASS;
	a[0].value = &class;
	a[0].value_len = sizeof(class);
	a[1].type = CKA_VALUE;
	a[1].value = key->data;
	a[1].value_len = key->size;
	a[2].type = CKA_TOKEN;
	a[2].value = &tval;
	a[2].value_len = sizeof(tval);
	a[3].type = CKA_PRIVATE;
	a[3].value = &tval;
	a[3].value_len = sizeof(tval);
	a[4].type = CKA_KEY_TYPE;
	a[4].value = &keytype;
	a[4].value_len = sizeof(keytype);
	a[5].type = CKA_ID;
	a[5].value = id;
	a[5].value_len = sizeof(id);

	a_val = 6;

	if (label) {
		a[a_val].type = CKA_LABEL;
		a[a_val].value = (void *) label;
		a[a_val].value_len = strlen(label);
		a_val++;
	}

	if (flags & GNUTLS_PKCS11_OBJ_FLAG_MARK_SENSITIVE)
		tval = 1;
	else
		tval = 0;

	a[a_val].type = CKA_SENSITIVE;
	a[a_val].value = &tval;
	a[a_val].value_len = sizeof(tval);
	a_val++;

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
	pkcs11_close_session(&sinfo);

	return ret;

}
