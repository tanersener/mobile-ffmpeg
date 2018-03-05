/*
 * GnuTLS PKCS#11 support
 * Copyright (C) 2010-2012 Free Software Foundation, Inc.
 * Copyright (C) 2017 Red Hat, Inc.
 * 
 * Authors: Nikos Mavrogiannopoulos, Stef Walter
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
#include <tls-sig.h>
#include <pk.h>
#include <fips.h>
#include "urls.h"
#include "locks.h"
#include <p11-kit/uri.h>

/* In case of a fork, it will invalidate the open session
 * in the privkey and start another */
#define PKCS11_CHECK_INIT_PRIVKEY(k) \
	ret = _gnutls_pkcs11_check_init(PROV_INIT_MANUAL, k, reopen_privkey_session); \
	if (ret < 0) \
		return gnutls_assert_val(ret)

#define FIND_OBJECT(key) \
	do { \
		int retries = 0; \
		int rret; \
		ret = find_object (&key->sinfo, &key->pin, &key->ref, key->uinfo, \
					  SESSION_LOGIN); \
		if (ret == GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE) { \
			if (_gnutls_token_func) \
			  { \
			    rret = pkcs11_call_token_func (key->uinfo, retries++); \
			    if (rret == 0) continue; \
			  } \
			return gnutls_assert_val(ret); \
		} else if (ret < 0) { \
			return gnutls_assert_val(ret); \
		} \
		break; \
	} while (1);

struct gnutls_pkcs11_privkey_st {
	gnutls_pk_algorithm_t pk_algorithm;
	unsigned int flags;
	struct p11_kit_uri *uinfo;
	char *url;

	struct pkcs11_session_info sinfo;
	ck_object_handle_t ref;	/* the key in the session */
	unsigned reauth; /* whether we need to login on each operation */

	void *mutex; /* lock for operations requiring co-ordination */

	struct pin_info_st pin;
};

/**
 * gnutls_pkcs11_privkey_init:
 * @key: A pointer to the type to be initialized
 *
 * This function will initialize an private key structure. This
 * structure can be used for accessing an underlying PKCS#11 object.
 *
 * In versions of GnuTLS later than 3.5.11 the object is protected
 * using locks and a single %gnutls_pkcs11_privkey_t can be re-used
 * by many threads. However, for performance it is recommended to utilize
 * one object per key per thread.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int gnutls_pkcs11_privkey_init(gnutls_pkcs11_privkey_t * key)
{
	int ret;
	FAIL_IF_LIB_ERROR;

	*key = gnutls_calloc(1, sizeof(struct gnutls_pkcs11_privkey_st));
	if (*key == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	(*key)->uinfo = p11_kit_uri_new();
	if ((*key)->uinfo == NULL) {
		free(*key);
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	ret = gnutls_mutex_init(&(*key)->mutex);
	if (ret < 0) {
		gnutls_assert();
		p11_kit_uri_free((*key)->uinfo);
		free(*key);
		return GNUTLS_E_LOCKING_ERROR;
	}

	return 0;
}

/**
 * gnutls_pkcs11_privkey_cpy:
 * @dst: The destination key, which should be initialized.
 * @src: The source key
 *
 * This function will copy a private key from source to destination
 * key. Destination has to be initialized.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.4.0
 **/
int
gnutls_pkcs11_privkey_cpy(gnutls_pkcs11_privkey_t dst,
			  gnutls_pkcs11_privkey_t src)
{
	return gnutls_pkcs11_privkey_import_url(dst, src->url, src->flags);
}

/**
 * gnutls_pkcs11_privkey_deinit:
 * @key: the key to be deinitialized
 *
 * This function will deinitialize a private key structure.
 **/
void gnutls_pkcs11_privkey_deinit(gnutls_pkcs11_privkey_t key)
{
	p11_kit_uri_free(key->uinfo);
	gnutls_free(key->url);
	if (key->sinfo.init != 0)
		pkcs11_close_session(&key->sinfo);
	gnutls_mutex_deinit(&key->mutex);
	gnutls_free(key);
}

/**
 * gnutls_pkcs11_privkey_get_pk_algorithm:
 * @key: should contain a #gnutls_pkcs11_privkey_t type
 * @bits: if bits is non null it will hold the size of the parameters' in bits
 *
 * This function will return the public key algorithm of a private
 * key.
 *
 * Returns: a member of the #gnutls_pk_algorithm_t enumeration on
 *   success, or a negative error code on error.
 **/
int
gnutls_pkcs11_privkey_get_pk_algorithm(gnutls_pkcs11_privkey_t key,
				       unsigned int *bits)
{
	if (bits)
		*bits = 0;	/* FIXME */
	return key->pk_algorithm;
}

/**
 * gnutls_pkcs11_privkey_get_info:
 * @pkey: should contain a #gnutls_pkcs11_privkey_t type
 * @itype: Denotes the type of information requested
 * @output: where output will be stored
 * @output_size: contains the maximum size of the output and will be overwritten with actual
 *
 * This function will return information about the PKCS 11 private key such
 * as the label, id as well as token information where the key is stored. When
 * output is text it returns null terminated string although #output_size contains
 * the size of the actual data only.
 *
 * Returns: %GNUTLS_E_SUCCESS (0) on success or a negative error code on error.
 **/
int
gnutls_pkcs11_privkey_get_info(gnutls_pkcs11_privkey_t pkey,
			       gnutls_pkcs11_obj_info_t itype,
			       void *output, size_t * output_size)
{
	return pkcs11_get_info(pkey->uinfo, itype, output, output_size);
}

static int
find_object(struct pkcs11_session_info *sinfo,
	    struct pin_info_st *pin_info,
	    ck_object_handle_t * _ctx,
	    struct p11_kit_uri *info, unsigned int flags)
{
	int ret;
	ck_object_handle_t ctx;
	struct ck_attribute *attrs;
	unsigned long attr_count;
	unsigned long count;
	ck_rv_t rv;

	ret =
	    pkcs11_open_session(sinfo, pin_info, info,
				flags & SESSION_LOGIN);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	attrs = p11_kit_uri_get_attributes(info, &attr_count);
	rv = pkcs11_find_objects_init(sinfo->module, sinfo->pks, attrs,
				      attr_count);
	if (rv != CKR_OK) {
		gnutls_assert();
		_gnutls_debug_log("p11: FindObjectsInit failed.\n");
		ret = pkcs11_rv_to_err(rv);
		goto fail;
	}

	if (pkcs11_find_objects(sinfo->module, sinfo->pks, &ctx, 1, &count)
	    == CKR_OK && count == 1) {
		*_ctx = ctx;
		pkcs11_find_objects_final(sinfo);
		return 0;
	}

	ret = GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
	pkcs11_find_objects_final(sinfo);
      fail:
	pkcs11_close_session(sinfo);

	return ret;
}

/* callback function to be passed in _gnutls_pkcs11_check_init().
 * It is run, only when a fork has been detected, and data have
 * been re-initialized. In that case we reset the session and re-open
 * the object. */
static int reopen_privkey_session(void * _privkey)
{
	int ret;
	gnutls_pkcs11_privkey_t privkey = _privkey;

	memset(&privkey->sinfo, 0, sizeof(privkey->sinfo));
	privkey->ref = 0;
	FIND_OBJECT(privkey);

	return 0;
}

#define REPEAT_ON_INVALID_HANDLE(expr) \
	if ((expr) == CKR_SESSION_HANDLE_INVALID) { \
		ret = reopen_privkey_session(key); \
		if (ret < 0) \
			return gnutls_assert_val(ret); \
		expr; \
	}

/*-
 * _gnutls_pkcs11_privkey_sign_hash:
 * @key: Holds the key
 * @hash: holds the data to be signed (should be output of a hash)
 * @signature: will contain the signature allocated with gnutls_malloc()
 *
 * This function will sign the given data using a signature algorithm
 * supported by the private key. It is assumed that the given data
 * are the output of a hash function.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 -*/
int
_gnutls_pkcs11_privkey_sign_hash(gnutls_pkcs11_privkey_t key,
				 const gnutls_datum_t * hash,
				 gnutls_datum_t * signature)
{
	ck_rv_t rv;
	int ret;
	struct ck_mechanism mech;
	gnutls_datum_t tmp = { NULL, 0 };
	unsigned long siglen;
	struct pkcs11_session_info *sinfo;
	unsigned req_login = 0;
	unsigned login_flags = SESSION_LOGIN|SESSION_CONTEXT_SPECIFIC;

	PKCS11_CHECK_INIT_PRIVKEY(key);

	sinfo = &key->sinfo;

	mech.mechanism = pk_to_mech(key->pk_algorithm);
	mech.parameter = NULL;
	mech.parameter_len = 0;

	ret = gnutls_mutex_lock(&key->mutex);
	if (ret != 0)
		return gnutls_assert_val(GNUTLS_E_LOCKING_ERROR);

	/* Initialize signing operation; using the private key discovered
	 * earlier. */
	REPEAT_ON_INVALID_HANDLE(rv = pkcs11_sign_init(sinfo->module, sinfo->pks, &mech, key->ref));
	if (rv != CKR_OK) {
		gnutls_assert();
		ret = pkcs11_rv_to_err(rv);
		goto cleanup;
	}

 retry_login:
	if (key->reauth || req_login) {
		if (req_login)
			login_flags = SESSION_LOGIN|SESSION_FORCE_LOGIN;

		ret =
		    pkcs11_login(&key->sinfo, &key->pin,
				 key->uinfo, login_flags);
		if (ret < 0) {
			gnutls_assert();
			_gnutls_debug_log("PKCS #11 login failed, trying operation anyway\n");
			/* let's try the operation anyway */
		}
	}

	/* Work out how long the signature must be: */
	rv = pkcs11_sign(sinfo->module, sinfo->pks, hash->data, hash->size,
			 NULL, &siglen);
	if (unlikely(rv == CKR_USER_NOT_LOGGED_IN && req_login == 0)) {
		req_login = 1;
		goto retry_login;
	}

	if (rv != CKR_OK) {
		gnutls_assert();
		ret = pkcs11_rv_to_err(rv);
		goto cleanup;
	}

	tmp.data = gnutls_malloc(siglen);
	tmp.size = siglen;

	rv = pkcs11_sign(sinfo->module, sinfo->pks, hash->data, hash->size,
			 tmp.data, &siglen);
	if (rv != CKR_OK) {
		gnutls_assert();
		ret = pkcs11_rv_to_err(rv);
		goto cleanup;
	}


	if (key->pk_algorithm == GNUTLS_PK_EC
	    || key->pk_algorithm == GNUTLS_PK_DSA) {
		unsigned int hlen = siglen / 2;
		gnutls_datum_t r, s;

		if (siglen % 2 != 0) {
			gnutls_assert();
			ret = GNUTLS_E_PK_SIGN_FAILED;
			goto cleanup;
		}

		r.data = tmp.data;
		r.size = hlen;

		s.data = &tmp.data[hlen];
		s.size = hlen;

		ret = _gnutls_encode_ber_rs_raw(signature, &r, &s);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		gnutls_free(tmp.data);
		tmp.data = NULL;
	} else {
		signature->size = siglen;
		signature->data = tmp.data;
	}

	ret = 0;

      cleanup:
	gnutls_mutex_unlock(&key->mutex);
	if (sinfo != &key->sinfo)
		pkcs11_close_session(sinfo);
	if (ret < 0)
		gnutls_free(tmp.data);

	return ret;
}

/**
 * gnutls_pkcs11_privkey_status:
 * @key: Holds the key
 *
 * Checks the status of the private key token.
 *
 * Returns: this function will return non-zero if the token
 * holding the private key is still available (inserted), and zero otherwise.
 * 
 * Since: 3.1.9
 *
 **/
unsigned gnutls_pkcs11_privkey_status(gnutls_pkcs11_privkey_t key)
{
	ck_rv_t rv;
	int ret;
	struct ck_session_info session_info;
	
	PKCS11_CHECK_INIT_PRIVKEY(key);

	REPEAT_ON_INVALID_HANDLE(rv = (key->sinfo.module)->C_GetSessionInfo(key->sinfo.pks, &session_info));
	if (rv != CKR_OK) {
		ret = 0;
		goto cleanup;
	}
	ret = 1;

      cleanup:

	return ret;
}

/**
 * gnutls_pkcs11_privkey_import_url:
 * @pkey: The private key
 * @url: a PKCS 11 url identifying the key
 * @flags: Or sequence of GNUTLS_PKCS11_OBJ_* flags
 *
 * This function will "import" a PKCS 11 URL identifying a private
 * key to the #gnutls_pkcs11_privkey_t type. In reality since
 * in most cases keys cannot be exported, the private key structure
 * is being associated with the available operations on the token.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_pkcs11_privkey_import_url(gnutls_pkcs11_privkey_t pkey,
				 const char *url, unsigned int flags)
{
	int ret;
	struct ck_attribute *attr;
	struct ck_attribute a[4];
	ck_key_type_t key_type;
	ck_bool_t reauth = 0;

	PKCS11_CHECK_INIT;

	memset(&pkey->sinfo, 0, sizeof(pkey->sinfo));

	if (pkey->url) {
		gnutls_free(pkey->url);
		pkey->url = NULL;
	}

	if (pkey->uinfo) {
		p11_kit_uri_free(pkey->uinfo);
		pkey->uinfo = NULL;
	}

	pkey->url = gnutls_strdup(url);
	if (pkey->url == NULL)
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

	ret = pkcs11_url_to_info(pkey->url, &pkey->uinfo, flags|GNUTLS_PKCS11_OBJ_FLAG_EXPECT_PRIVKEY);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	pkey->flags = flags;

	attr = p11_kit_uri_get_attribute(pkey->uinfo, CKA_CLASS);
	if (!attr || attr->value_len != sizeof(ck_object_class_t) ||
	    *(ck_object_class_t *) attr->value != CKO_PRIVATE_KEY) {
		gnutls_assert();
		ret = GNUTLS_E_INVALID_REQUEST;
		goto cleanup;
	}

	attr = p11_kit_uri_get_attribute(pkey->uinfo, CKA_ID);
	if (!attr) {
		attr = p11_kit_uri_get_attribute(pkey->uinfo, CKA_LABEL);
		if (!attr) {
			gnutls_assert();
			ret = GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
			goto cleanup;
		}
	}

	FIND_OBJECT(pkey);

	pkey->pk_algorithm = GNUTLS_PK_UNKNOWN;
	a[0].type = CKA_KEY_TYPE;
	a[0].value = &key_type;
	a[0].value_len = sizeof(key_type);

	if (pkcs11_get_attribute_value(pkey->sinfo.module, pkey->sinfo.pks, pkey->ref, a, 1)
	    == CKR_OK) {
		pkey->pk_algorithm = key_type_to_pk(key_type);
	}

	if (pkey->pk_algorithm == GNUTLS_PK_UNKNOWN) {
		_gnutls_debug_log
		    ("Cannot determine PKCS #11 key algorithm\n");
		ret = GNUTLS_E_UNKNOWN_ALGORITHM;
		goto cleanup;
	}

	a[0].type = CKA_ALWAYS_AUTHENTICATE;
	a[0].value = &reauth;
	a[0].value_len = sizeof(reauth);

	if (pkcs11_get_attribute_value(pkey->sinfo.module, pkey->sinfo.pks, pkey->ref, a, 1)
	    == CKR_OK) {
		pkey->reauth = reauth;
	}

	ret = 0;

	return ret;

      cleanup:
	if (pkey->uinfo != NULL) {
		p11_kit_uri_free(pkey->uinfo);
		pkey->uinfo = NULL;
	}
	gnutls_free(pkey->url);
	pkey->url = NULL;

	return ret;
}

/*-
 * _gnutls_pkcs11_privkey_decrypt_data:
 * @key: Holds the key
 * @flags: should be 0 for now
 * @ciphertext: holds the data to be signed
 * @plaintext: will contain the plaintext, allocated with gnutls_malloc()
 *
 * This function will decrypt the given data using the public key algorithm
 * supported by the private key. 
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 -*/
int
_gnutls_pkcs11_privkey_decrypt_data(gnutls_pkcs11_privkey_t key,
				    unsigned int flags,
				    const gnutls_datum_t * ciphertext,
				    gnutls_datum_t * plaintext)
{
	ck_rv_t rv;
	int ret;
	struct ck_mechanism mech;
	unsigned long siglen;
	unsigned req_login = 0;
	unsigned login_flags = SESSION_LOGIN|SESSION_CONTEXT_SPECIFIC;

	PKCS11_CHECK_INIT_PRIVKEY(key);

	if (key->pk_algorithm != GNUTLS_PK_RSA)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	mech.mechanism = CKM_RSA_PKCS;
	mech.parameter = NULL;
	mech.parameter_len = 0;

	ret = gnutls_mutex_lock(&key->mutex);
	if (ret != 0)
		return gnutls_assert_val(GNUTLS_E_LOCKING_ERROR);

	/* Initialize signing operation; using the private key discovered
	 * earlier. */
	REPEAT_ON_INVALID_HANDLE(rv = pkcs11_decrypt_init(key->sinfo.module, key->sinfo.pks, &mech, key->ref));
	if (rv != CKR_OK) {
		gnutls_assert();
		ret = pkcs11_rv_to_err(rv);
		goto cleanup;
	}

 retry_login:
	if (key->reauth || req_login) {
		if (req_login)
			login_flags = SESSION_LOGIN|SESSION_FORCE_LOGIN;

		ret =
		    pkcs11_login(&key->sinfo, &key->pin,
				 key->uinfo, login_flags);
		if (ret < 0) {
			gnutls_assert();
			_gnutls_debug_log("PKCS #11 login failed, trying operation anyway\n");
			/* let's try the operation anyway */
		}
	}

	/* Work out how long the plaintext must be: */
	rv = pkcs11_decrypt(key->sinfo.module, key->sinfo.pks, ciphertext->data,
			    ciphertext->size, NULL, &siglen);
	if (unlikely(rv == CKR_USER_NOT_LOGGED_IN && req_login == 0)) {
		req_login = 1;
		goto retry_login;
	}

	if (rv != CKR_OK) {
		gnutls_assert();
		ret = pkcs11_rv_to_err(rv);
		goto cleanup;
	}

	plaintext->data = gnutls_malloc(siglen);
	plaintext->size = siglen;

	rv = pkcs11_decrypt(key->sinfo.module, key->sinfo.pks, ciphertext->data,
			    ciphertext->size, plaintext->data, &siglen);
	if (rv != CKR_OK) {
		gnutls_free(plaintext->data);
		gnutls_assert();
		ret = pkcs11_rv_to_err(rv);
		goto cleanup;
	}

	plaintext->size = siglen;

	ret = 0;

      cleanup:
	gnutls_mutex_unlock(&key->mutex);
	return ret;
}

/**
 * gnutls_pkcs11_privkey_export_url:
 * @key: Holds the PKCS 11 key
 * @detailed: non zero if a detailed URL is required
 * @url: will contain an allocated url
 *
 * This function will export a URL identifying the given key.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_pkcs11_privkey_export_url(gnutls_pkcs11_privkey_t key,
				 gnutls_pkcs11_url_type_t detailed,
				 char **url)
{
	int ret;

	ret = pkcs11_info_to_url(key->uinfo, detailed, url);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	return 0;
}

#if 0
/**
 * gnutls_pkcs11_privkey_generate:
 * @url: a token URL
 * @pk: the public key algorithm
 * @bits: the security bits
 * @label: a label
 * @flags: should be zero
 *
 * This function will generate a private key in the specified
 * by the @url token. The private key will be generate within
 * the token and will not be exportable.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.0
 **/
int
gnutls_pkcs11_privkey_generate(const char *url, gnutls_pk_algorithm_t pk,
			       unsigned int bits, const char *label,
			       unsigned int flags)
{
	int x;
}

/**
 * gnutls_pkcs11_privkey_generate2:
 * @url: a token URL
 * @pk: the public key algorithm
 * @bits: the security bits
 * @label: a label
 * @fmt: the format of output params. PEM or DER
 * @pubkey: will hold the public key (may be %NULL)
 * @flags: zero or an OR'ed sequence of %GNUTLS_PKCS11_OBJ_FLAGs
 *
 * This function will generate a private key in the specified
 * by the @url token. The private key will be generate within
 * the token and will not be exportable. This function will
 * store the DER-encoded public key in the SubjectPublicKeyInfo format 
 * in @pubkey. The @pubkey should be deinitialized using gnutls_free().
 *
 * Note that when generating an elliptic curve key, the curve
 * can be substituted in the place of the bits parameter using the
 * GNUTLS_CURVE_TO_BITS() macro.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.1.5
 **/
int
gnutls_pkcs11_privkey_generate2(const char *url, gnutls_pk_algorithm_t pk,
				unsigned int bits, const char *label,
				gnutls_x509_crt_fmt_t fmt,
				gnutls_datum_t * pubkey,
				unsigned int flags)
{
	int x;
}
#endif

static const char def_rsa_pub_exp[3] = { 1,0,1 }; // 65537 = 0x10001

struct dsa_params {
	/* FIPS 186-3 maximal size for L and N length pair is (3072,256). */
	uint8_t prime[384];
	uint8_t subprime[32];
	uint8_t generator[384];
};

static int
_dsa_params_generate(struct ck_function_list *module, ck_session_handle_t session,
		     unsigned long bits, struct dsa_params *params,
		     struct ck_attribute *a, int *a_val)
{
	struct ck_mechanism mech = { CKM_DSA_PARAMETER_GEN };
	struct ck_attribute attr = { CKA_PRIME_BITS, &bits, sizeof(bits) };
	ck_object_handle_t key;
	ck_rv_t rv;

	/* Generate DSA parameters from prime length. */

	rv = pkcs11_generate_key(module, session, &mech, &attr, 1, &key);
	if (rv != CKR_OK) {
		gnutls_assert();
		_gnutls_debug_log("p11: %s\n", pkcs11_strerror(rv));
		return pkcs11_rv_to_err(rv);
	}

	/* Retrieve generated parameters to be used with the new key pair. */

	a[*a_val + 0].type = CKA_PRIME;
	a[*a_val + 0].value = params->prime;
	a[*a_val + 0].value_len = sizeof(params->prime);

	a[*a_val + 1].type = CKA_SUBPRIME;
	a[*a_val + 1].value = params->subprime;
	a[*a_val + 1].value_len = sizeof(params->subprime);

	a[*a_val + 2].type = CKA_BASE;
	a[*a_val + 2].value = params->generator;
	a[*a_val + 2].value_len = sizeof(params->generator);

	rv = pkcs11_get_attribute_value(module, session, key, &a[*a_val], 3);
	if (rv != CKR_OK) {
		gnutls_assert();
		_gnutls_debug_log("p11: %s\n", pkcs11_strerror(rv));
		return pkcs11_rv_to_err(rv);
	}

	*a_val += 3;

	return 0;
}

/**
 * gnutls_pkcs11_privkey_generate3:
 * @url: a token URL
 * @pk: the public key algorithm
 * @bits: the security bits
 * @label: a label
 * @cid: The CKA_ID to use for the new object
 * @fmt: the format of output params. PEM or DER
 * @pubkey: will hold the public key (may be %NULL)
 * @key_usage: One of GNUTLS_KEY_*
 * @flags: zero or an OR'ed sequence of %GNUTLS_PKCS11_OBJ_FLAGs
 *
 * This function will generate a private key in the specified
 * by the @url token. The private key will be generate within
 * the token and will not be exportable. This function will
 * store the DER-encoded public key in the SubjectPublicKeyInfo format 
 * in @pubkey. The @pubkey should be deinitialized using gnutls_free().
 *
 * Note that when generating an elliptic curve key, the curve
 * can be substituted in the place of the bits parameter using the
 * GNUTLS_CURVE_TO_BITS() macro.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.4.0
 **/
int
gnutls_pkcs11_privkey_generate3(const char *url, gnutls_pk_algorithm_t pk,
				unsigned int bits, const char *label,
				const gnutls_datum_t *cid,
				gnutls_x509_crt_fmt_t fmt,
				gnutls_datum_t * pubkey,
				unsigned int key_usage,
				unsigned int flags)
{
	int ret;
	const ck_bool_t tval = 1;
	const ck_bool_t fval = 0;
	struct pkcs11_session_info sinfo;
	struct p11_kit_uri *info = NULL;
	ck_rv_t rv;
	struct ck_attribute a[22], p[22];
	ck_object_handle_t pub_ctx, priv_ctx;
	unsigned long _bits = bits;
	int a_val, p_val;
	struct ck_mechanism mech;
	gnutls_pubkey_t pkey = NULL;
	gnutls_pkcs11_obj_t obj = NULL;
	gnutls_datum_t der = {NULL, 0};
	ck_key_type_t key_type;
	uint8_t id[20];
	struct dsa_params dsa_params;

	PKCS11_CHECK_INIT;
	FIX_KEY_USAGE(pk, key_usage);

	memset(&sinfo, 0, sizeof(sinfo));

	ret = pkcs11_url_to_info(url, &info, 0);
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
		goto cleanup;
	}

	/* a holds the public key template
	 * and p the private key */
	a_val = p_val = 0;
	mech.parameter = NULL;
	mech.parameter_len = 0;
	mech.mechanism = pk_to_genmech(pk, &key_type);

	if (!(flags & GNUTLS_PKCS11_OBJ_FLAG_NO_STORE_PUBKEY)) {
		a[a_val].type = CKA_TOKEN;
		a[a_val].value = (void *) &tval;
		a[a_val].value_len = sizeof(tval);
		a_val++;

		a[a_val].type = CKA_PRIVATE;
		a[a_val].value = (void *) &fval;
		a[a_val].value_len = sizeof(fval);
		a_val++;
	}

	a[a_val].type = CKA_ID;
	if (cid == NULL || cid->size == 0) {
		ret = gnutls_rnd(GNUTLS_RND_NONCE, id, sizeof(id));
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		a[a_val].value = (void *) id;
		a[a_val].value_len = sizeof(id);
	} else {
		a[a_val].value = (void *) cid->data;
		a[a_val].value_len = cid->size;
	}

	p[p_val].type = CKA_ID;
	p[p_val].value = a[a_val].value;
	p[p_val].value_len = a[a_val].value_len;
	a_val++;
	p_val++;

	switch (pk) {
	case GNUTLS_PK_RSA:
		p[p_val].type = CKA_DECRYPT;
		if (key_usage & (GNUTLS_KEY_DECIPHER_ONLY|GNUTLS_KEY_ENCIPHER_ONLY)) {
			p[p_val].value = (void *) &tval;
			p[p_val].value_len = sizeof(tval);
		} else {
			p[p_val].value = (void *) &fval;
			p[p_val].value_len = sizeof(fval);
		}
		p_val++;

		p[p_val].type = CKA_SIGN;
		if (key_usage & GNUTLS_KEY_DIGITAL_SIGNATURE) {
			p[p_val].value = (void *) &tval;
			p[p_val].value_len = sizeof(tval);
		} else {
			p[p_val].value = (void *) &fval;
			p[p_val].value_len = sizeof(fval);
		}
		p_val++;

		a[a_val].type = CKA_ENCRYPT;
		a[a_val].value = (void *) &tval;
		a[a_val].value_len = sizeof(tval);
		a_val++;

		a[a_val].type = CKA_VERIFY;
		a[a_val].value = (void *) &tval;
		a[a_val].value_len = sizeof(tval);
		a_val++;

		a[a_val].type = CKA_MODULUS_BITS;
		a[a_val].value = &_bits;
		a[a_val].value_len = sizeof(_bits);
		a_val++;

		a[a_val].type = CKA_PUBLIC_EXPONENT;
		a[a_val].value = (char*)def_rsa_pub_exp;
		a[a_val].value_len = sizeof(def_rsa_pub_exp);
		a_val++;

		break;
	case GNUTLS_PK_DSA:
		p[p_val].type = CKA_SIGN;
		if (key_usage & GNUTLS_KEY_DIGITAL_SIGNATURE) {
			p[p_val].value = (void *) &tval;
			p[p_val].value_len = sizeof(tval);
		} else {
			p[p_val].value = (void *) &fval;
			p[p_val].value_len = sizeof(fval);
		}
		p_val++;

		a[a_val].type = CKA_VERIFY;
		a[a_val].value = (void *) &tval;
		a[a_val].value_len = sizeof(tval);
		a_val++;

		ret = _dsa_params_generate(sinfo.module, sinfo.pks, _bits,
					   &dsa_params, a, &a_val);
		if (ret < 0) {
			goto cleanup;
		}

		break;
	case GNUTLS_PK_EC:
		p[p_val].type = CKA_SIGN;
		if (key_usage & GNUTLS_KEY_DIGITAL_SIGNATURE) {
			p[p_val].value = (void *) &tval;
			p[p_val].value_len = sizeof(tval);
		} else {
			p[p_val].value = (void *) &fval;
			p[p_val].value_len = sizeof(fval);
		}
		p_val++;

		a[a_val].type = CKA_VERIFY;
		a[a_val].value = (void *) &tval;
		a[a_val].value_len = sizeof(tval);
		a_val++;

		if (GNUTLS_BITS_ARE_CURVE(bits)) {
			bits = GNUTLS_BITS_TO_CURVE(bits);
		} else {
			bits = _gnutls_ecc_bits_to_curve(bits);
		}

		ret = _gnutls_x509_write_ecc_params(bits, &der);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		a[a_val].type = CKA_EC_PARAMS;
		a[a_val].value = der.data;
		a[a_val].value_len = der.size;
		a_val++;
		break;
	default:
		ret = gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);
		goto cleanup;
	}

	/*
	 * on request, add the CKA_WRAP/CKA_UNWRAP key attribute
	 */
	if (flags & GNUTLS_PKCS11_OBJ_FLAG_MARK_KEY_WRAP) {
		p[p_val].type = CKA_UNWRAP;
		p[p_val].value = (void*)&tval;
		p[p_val].value_len = sizeof(tval);
		p_val++;
		a[a_val].type = CKA_WRAP;
		a[a_val].value = (void*)&tval;
		a[a_val].value_len = sizeof(tval);
		a_val++;
	}

	/* a private key is set always as private unless
	 * requested otherwise
	 */
	if (flags & GNUTLS_PKCS11_OBJ_FLAG_MARK_NOT_PRIVATE) {
		p[p_val].type = CKA_PRIVATE;
		p[p_val].value = (void *) &fval;
		p[p_val].value_len = sizeof(fval);
		p_val++;
	} else {
		p[p_val].type = CKA_PRIVATE;
		p[p_val].value = (void *) &tval;
		p[p_val].value_len = sizeof(tval);
		p_val++;
	}

	p[p_val].type = CKA_TOKEN;
	p[p_val].value = (void *) &tval;
	p[p_val].value_len = sizeof(tval);
	p_val++;

	if (label) {
		p[p_val].type = CKA_LABEL;
		p[p_val].value = (void *) label;
		p[p_val].value_len = strlen(label);
		p_val++;

		a[a_val].type = CKA_LABEL;
		a[a_val].value = (void *) label;
		a[a_val].value_len = strlen(label);
		a_val++;
	}

	if (flags & GNUTLS_PKCS11_OBJ_FLAG_MARK_SENSITIVE) {
		p[p_val].type = CKA_SENSITIVE;
		p[p_val].value = (void *) &tval;
		p[p_val].value_len = sizeof(tval);
		p_val++;
	} else {
		p[p_val].type = CKA_SENSITIVE;
		p[p_val].value = (void *) &fval;
		p[p_val].value_len = sizeof(fval);
		p_val++;
	}

	rv = pkcs11_generate_key_pair(sinfo.module, sinfo.pks, &mech, a,
				      a_val, p, p_val, &pub_ctx, &priv_ctx);
	if (rv != CKR_OK) {
		gnutls_assert();
		_gnutls_debug_log("p11: %s\n", pkcs11_strerror(rv));
		ret = pkcs11_rv_to_err(rv);
		goto cleanup;
	}

	/* extract the public key */
	if (pubkey) {

		ret = gnutls_pubkey_init(&pkey);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		ret = gnutls_pkcs11_obj_init(&obj);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		obj->pk_algorithm = pk;
		obj->type = GNUTLS_PKCS11_OBJ_PUBKEY;
		ret =
		    pkcs11_read_pubkey(sinfo.module, sinfo.pks, pub_ctx,
				       key_type, obj);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		ret = gnutls_pubkey_import_pkcs11(pkey, obj, 0);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		ret = gnutls_pubkey_export2(pkey, fmt, pubkey);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}
	}

      cleanup:
	if (obj != NULL)
		gnutls_pkcs11_obj_deinit(obj);
	if (pkey != NULL)
		gnutls_pubkey_deinit(pkey);

	if (sinfo.pks != 0)
		pkcs11_close_session(&sinfo);
	gnutls_free(der.data);

	return ret;
}

/* loads a the corresponding to the private key public key either from 
 * a public key object or from a certificate.
 */
static int load_pubkey_obj(gnutls_pkcs11_privkey_t pkey, gnutls_pubkey_t pub)
{
	int ret, iret;
	gnutls_x509_crt_t crt;

	ret = gnutls_pubkey_import_url(pub, pkey->url, pkey->flags);
	if (ret >= 0) {
		return ret;
	}
	iret = ret;

	/* else try certificate */
	ret = gnutls_x509_crt_init(&crt);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	gnutls_x509_crt_set_pin_function(crt, pkey->pin.cb, pkey->pin.data);

	ret = gnutls_x509_crt_import_url(crt, pkey->url, pkey->flags);
	if (ret < 0) {
		ret = iret;
		goto cleanup;
	}

	ret = gnutls_pubkey_import_x509(pub, crt, 0);

 cleanup:
	gnutls_x509_crt_deinit(crt);
	return ret;
}

int
_pkcs11_privkey_get_pubkey (gnutls_pkcs11_privkey_t pkey, gnutls_pubkey_t *pub, unsigned flags)
{
	gnutls_pubkey_t pubkey = NULL;
	gnutls_pkcs11_obj_t obj = NULL;
	ck_key_type_t key_type;
	int ret;

	PKCS11_CHECK_INIT_PRIVKEY(pkey);

	if (!pkey) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	ret = gnutls_pubkey_init(&pubkey);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = gnutls_pkcs11_obj_init(&obj);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	obj->pk_algorithm = gnutls_pkcs11_privkey_get_pk_algorithm(pkey, 0);
	obj->type = GNUTLS_PKCS11_OBJ_PUBKEY;
	pk_to_genmech(obj->pk_algorithm, &key_type);

	gnutls_pubkey_set_pin_function(pubkey, pkey->pin.cb, pkey->pin.data);

	/* we can only read the public key from RSA keys */
	if (key_type != CKK_RSA) {
		/* try opening the public key object if it exists */
		ret = load_pubkey_obj(pkey, pubkey);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}
	} else {
		ret = pkcs11_read_pubkey(pkey->sinfo.module, pkey->sinfo.pks, pkey->ref, key_type, obj);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		ret = gnutls_pubkey_import_pkcs11(pubkey, obj, 0);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}
	}

	*pub = pubkey;

	pubkey = NULL;
	ret = 0;

 cleanup:
	if (obj != NULL)
		gnutls_pkcs11_obj_deinit(obj);
	if (pubkey != NULL)
		gnutls_pubkey_deinit(pubkey);

	return ret;
}

/**
 * gnutls_pkcs11_privkey_export_pubkey
 * @pkey: The private key
 * @fmt: the format of output params. PEM or DER.
 * @data: will hold the public key
 * @flags: should be zero
 *
 * This function will extract the public key (modulus and public
 * exponent) from the private key specified by the @url private key.
 * This public key will be stored in @pubkey in the format specified
 * by @fmt. @pubkey should be deinitialized using gnutls_free().
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.3.7
 **/
int
gnutls_pkcs11_privkey_export_pubkey(gnutls_pkcs11_privkey_t pkey,
				     gnutls_x509_crt_fmt_t fmt,
				     gnutls_datum_t * data,
				     unsigned int flags)
{
	int ret;
	gnutls_pubkey_t pubkey = NULL;

	ret = _pkcs11_privkey_get_pubkey(pkey, &pubkey, flags);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = gnutls_pubkey_export2(pubkey, fmt, data);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = 0;

      cleanup:
	if (pubkey != NULL)
		gnutls_pubkey_deinit(pubkey);

	return ret;
}

/**
 * gnutls_pkcs11_privkey_set_pin_function:
 * @key: The private key
 * @fn: the callback
 * @userdata: data associated with the callback
 *
 * This function will set a callback function to be used when
 * required to access the object. This function overrides the global
 * set using gnutls_pkcs11_set_pin_function().
 *
 * Since: 3.1.0
 *
 **/
void
gnutls_pkcs11_privkey_set_pin_function(gnutls_pkcs11_privkey_t key,
				       gnutls_pin_callback_t fn,
				       void *userdata)
{
	key->pin.cb = fn;
	key->pin.data = userdata;
}
