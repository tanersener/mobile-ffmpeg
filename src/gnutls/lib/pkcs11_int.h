/*
 * GnuTLS PKCS#11 support
 * Copyright (C) 2010-2012 Free Software Foundation, Inc.
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

#ifndef PKCS11_INT_H
#define PKCS11_INT_H

#ifdef ENABLE_PKCS11

#define CRYPTOKI_GNU
#include <p11-kit/pkcs11.h>
#include <gnutls/pkcs11.h>
#include <x509/x509_int.h>

#define PKCS11_ID_SIZE 128
#define PKCS11_LABEL_SIZE 128

#include <p11-kit/uri.h>
typedef unsigned char ck_bool_t;

struct pkcs11_session_info {
	struct ck_function_list *module;
	struct ck_token_info tinfo;
	struct ck_slot_info slot_info;
	ck_session_handle_t pks;
	ck_slot_id_t sid;
	unsigned int init;
	unsigned int trusted; /* whether module is marked as trusted */
};

struct gnutls_pkcs11_obj_st {
	gnutls_datum_t raw;
	gnutls_pkcs11_obj_type_t type;
	ck_object_class_t class;

	unsigned int flags;
	struct p11_kit_uri *info;

	/* only when pubkey */
	gnutls_datum_t pubkey[MAX_PUBLIC_PARAMS_SIZE];
	unsigned pubkey_size;
	gnutls_pk_algorithm_t pk_algorithm;
	unsigned int key_usage;

	struct pin_info_st pin;
};

/* This must be called on every function that uses a PKCS #11 function
 * directly. It can be provided a callback function to run when a reinitialization
 * occurs. */
typedef int (*pkcs11_reinit_function)(void *priv);

typedef enum init_level_t {
	PROV_UNINITIALIZED = 0,
	PROV_INIT_MANUAL,
	PROV_INIT_MANUAL_TRUSTED,
	PROV_INIT_TRUSTED,
	PROV_INIT_ALL
} init_level_t;

/* See _gnutls_pkcs11_check_init() for possible Transitions.
 */

int _gnutls_pkcs11_check_init(init_level_t req_level, void *priv, pkcs11_reinit_function cb);

#define FIX_KEY_USAGE(pk, usage) \
	if (usage == 0) { \
		if (pk == GNUTLS_PK_RSA) \
			usage = GNUTLS_KEY_DECIPHER_ONLY|GNUTLS_KEY_DIGITAL_SIGNATURE; \
		else \
			usage = GNUTLS_KEY_DIGITAL_SIGNATURE; \
	}

#define PKCS11_CHECK_INIT \
	ret = _gnutls_pkcs11_check_init(PROV_INIT_ALL, NULL, NULL); \
	if (ret < 0) \
		return gnutls_assert_val(ret)

#define PKCS11_CHECK_INIT_RET(x) \
	ret = _gnutls_pkcs11_check_init(PROV_INIT_ALL, NULL, NULL); \
	if (ret < 0) \
		return gnutls_assert_val(x)

#define PKCS11_CHECK_INIT_FLAGS(f) \
	ret = _gnutls_pkcs11_check_init((f & GNUTLS_PKCS11_OBJ_FLAG_PRESENT_IN_TRUSTED_MODULE)?PROV_INIT_TRUSTED:PROV_INIT_ALL, NULL, NULL); \
	if (ret < 0) \
		return gnutls_assert_val(ret)

#define PKCS11_CHECK_INIT_FLAGS_RET(f, x) \
	ret = _gnutls_pkcs11_check_init((f & GNUTLS_PKCS11_OBJ_FLAG_PRESENT_IN_TRUSTED_MODULE)?PROV_INIT_TRUSTED:PROV_INIT_ALL, NULL, NULL); \
	if (ret < 0) \
		return gnutls_assert_val(x)


/* thus function is called for every token in the traverse_tokens
 * function. Once everything is traversed it is called with NULL tinfo.
 * It should return 0 if found what it was looking for.
 */
typedef int (*find_func_t) (struct ck_function_list *, struct pkcs11_session_info *,
			    struct ck_token_info * tinfo, struct ck_info *,
			    void *input);

int pkcs11_rv_to_err(ck_rv_t rv);
int pkcs11_url_to_info(const char *url, struct p11_kit_uri **info, unsigned flags);
int
pkcs11_find_slot(struct ck_function_list **module, ck_slot_id_t * slot,
		 struct p11_kit_uri *info, struct ck_token_info *_tinfo,
		 struct ck_slot_info *_slot_info, unsigned int *trusted);

int pkcs11_read_pubkey(struct ck_function_list *module,
		       ck_session_handle_t pks, ck_object_handle_t obj,
		       ck_key_type_t key_type, gnutls_pkcs11_obj_t pobj);

int pkcs11_override_cert_exts(struct pkcs11_session_info *sinfo, gnutls_datum_t *spki, gnutls_datum_t *der);

int pkcs11_get_info(struct p11_kit_uri *info,
		    gnutls_pkcs11_obj_info_t itype, void *output,
		    size_t * output_size);
int pkcs11_login(struct pkcs11_session_info *sinfo,
		 struct pin_info_st *pin_info,
		 struct p11_kit_uri *info, unsigned flags);

int pkcs11_call_token_func(struct p11_kit_uri *info, const unsigned retry);

extern gnutls_pkcs11_token_callback_t _gnutls_token_func;
extern void *_gnutls_token_data;

void pkcs11_rescan_slots(void);
int pkcs11_info_to_url(struct p11_kit_uri *info,
		       gnutls_pkcs11_url_type_t detailed, char **url);

int
_gnutls_x509_crt_import_pkcs11_url(gnutls_x509_crt_t crt,
				  const char *url, unsigned int flags);

#define SESSION_WRITE (1<<0)
#define SESSION_LOGIN (1<<1)
#define SESSION_SO (1<<2)	/* security officer session */
#define SESSION_TRUSTED (1<<3) /* session on a marked as trusted (p11-kit) module */
#define SESSION_FORCE_LOGIN (1<<4) /* force login even when CFK_LOGIN_REQUIRED is not set */
#define SESSION_CONTEXT_SPECIFIC (1<<5)

int pkcs11_open_session(struct pkcs11_session_info *sinfo,
			struct pin_info_st *pin_info,
			struct p11_kit_uri *info, unsigned int flags);
int _pkcs11_traverse_tokens(find_func_t find_func, void *input,
			    struct p11_kit_uri *info,
			    struct pin_info_st *pin_info,
			    unsigned int flags);
ck_object_class_t pkcs11_strtype_to_class(const char *type);

/* Additional internal flags for gnutls_pkcs11_obj_flags */
/* @GNUTLS_PKCS11_OBJ_FLAG_EXPECT_CERT: When importing an object, provide a hint on the type, to allow incomplete URLs
 * @GNUTLS_PKCS11_OBJ_FLAG_EXPECT_PRIVKEY: Hint for private key */
#define GNUTLS_PKCS11_OBJ_FLAG_EXPECT_CERT (1<<29)
#define GNUTLS_PKCS11_OBJ_FLAG_EXPECT_PRIVKEY (1<<30)
#define GNUTLS_PKCS11_OBJ_FLAG_EXPECT_PUBKEY ((unsigned int)1<<31)

int pkcs11_token_matches_info(struct p11_kit_uri *info,
			      struct ck_token_info *tinfo,
			      struct ck_info *lib_info);

unsigned int pkcs11_obj_flags_to_int(unsigned int flags);

int
_gnutls_pkcs11_privkey_sign_hash(gnutls_pkcs11_privkey_t key,
				 const gnutls_datum_t * hash,
				 gnutls_datum_t * signature);

int
_gnutls_pkcs11_privkey_decrypt_data(gnutls_pkcs11_privkey_t key,
				    unsigned int flags,
				    const gnutls_datum_t * ciphertext,
				    gnutls_datum_t * plaintext);

int
_pkcs11_privkey_get_pubkey (gnutls_pkcs11_privkey_t pkey, gnutls_pubkey_t *pub, unsigned flags);

static inline int pk_to_mech(gnutls_pk_algorithm_t pk)
{
	if (pk == GNUTLS_PK_DSA)
		return CKM_DSA;
	else if (pk == GNUTLS_PK_EC)
		return CKM_ECDSA;
	else
		return CKM_RSA_PKCS;
}

static inline int pk_to_key_type(gnutls_pk_algorithm_t pk)
{
	if (pk == GNUTLS_PK_DSA)
		return CKK_DSA;
	else if (pk == GNUTLS_PK_EC)
		return CKK_ECDSA;
	else
		return CKK_RSA;
}

static inline gnutls_pk_algorithm_t key_type_to_pk(ck_key_type_t m)
{
	if (m == CKK_RSA)
		return GNUTLS_PK_RSA;
	else if (m == CKK_DSA)
		return GNUTLS_PK_DSA;
	else if (m == CKK_ECDSA)
		return GNUTLS_PK_EC;
	else
		return GNUTLS_PK_UNKNOWN;
}

static inline int pk_to_genmech(gnutls_pk_algorithm_t pk, ck_key_type_t *type)
{
	if (pk == GNUTLS_PK_DSA) {
		*type = CKK_DSA;
		return CKM_DSA_KEY_PAIR_GEN;
	} else if (pk == GNUTLS_PK_EC) {
		*type = CKK_ECDSA;
		return CKM_ECDSA_KEY_PAIR_GEN;
	} else {
		*type = CKK_RSA;
		return CKM_RSA_PKCS_KEY_PAIR_GEN;
	}
}

ck_object_class_t pkcs11_type_to_class(gnutls_pkcs11_obj_type_t type);

ck_rv_t
pkcs11_generate_key(struct ck_function_list * module,
		    ck_session_handle_t sess,
		    struct ck_mechanism * mechanism,
		    struct ck_attribute * templ,
		    unsigned long count,
		    ck_object_handle_t * key);

ck_rv_t
pkcs11_generate_key_pair(struct ck_function_list * module,
			 ck_session_handle_t sess,
			 struct ck_mechanism * mechanism,
			 struct ck_attribute * pub_templ,
			 unsigned long pub_templ_count,
			 struct ck_attribute * priv_templ,
			 unsigned long priv_templ_count,
			 ck_object_handle_t * pub,
			 ck_object_handle_t * priv);

ck_rv_t
pkcs11_get_slot_list(struct ck_function_list *module,
		     unsigned char token_present,
		     ck_slot_id_t * slot_list, unsigned long *count);

ck_rv_t
pkcs11_get_module_info(struct ck_function_list *module,
		       struct ck_info *info);

ck_rv_t
pkcs11_get_slot_info(struct ck_function_list *module,
		     ck_slot_id_t slot_id, struct ck_slot_info *info);

ck_rv_t
pkcs11_get_token_info(struct ck_function_list *module,
		      ck_slot_id_t slot_id, struct ck_token_info *info);

ck_rv_t
pkcs11_find_objects_init(struct ck_function_list *module,
			 ck_session_handle_t sess,
			 struct ck_attribute *templ, unsigned long count);

ck_rv_t
pkcs11_find_objects(struct ck_function_list *module,
		    ck_session_handle_t sess,
		    ck_object_handle_t * objects,
		    unsigned long max_object_count,
		    unsigned long *object_count);

ck_rv_t pkcs11_find_objects_final(struct pkcs11_session_info *);

ck_rv_t pkcs11_close_session(struct pkcs11_session_info *);

ck_rv_t
pkcs11_set_attribute_value(struct ck_function_list * module,
			   ck_session_handle_t sess,
			   ck_object_handle_t object,
			   struct ck_attribute * templ,
			   unsigned long count);

ck_rv_t
pkcs11_get_attribute_value(struct ck_function_list *module,
			   ck_session_handle_t sess,
			   ck_object_handle_t object,
			   struct ck_attribute *templ,
			   unsigned long count);

ck_rv_t
pkcs11_get_attribute_avalue(struct ck_function_list * module,
			   ck_session_handle_t sess,
			   ck_object_handle_t object,
			   ck_attribute_type_t type,
			   gnutls_datum_t *res);

ck_rv_t
pkcs11_get_mechanism_list(struct ck_function_list *module,
			  ck_slot_id_t slot_id,
			  ck_mechanism_type_t * mechanism_list,
			  unsigned long *count);

ck_rv_t
pkcs11_sign_init(struct ck_function_list *module,
		 ck_session_handle_t sess,
		 struct ck_mechanism *mechanism, ck_object_handle_t key);

ck_rv_t
pkcs11_sign(struct ck_function_list *module,
	    ck_session_handle_t sess,
	    unsigned char *data,
	    unsigned long data_len,
	    unsigned char *signature, unsigned long *signature_len);

ck_rv_t
pkcs11_decrypt_init(struct ck_function_list *module,
		    ck_session_handle_t sess,
		    struct ck_mechanism *mechanism,
		    ck_object_handle_t key);

ck_rv_t
pkcs11_decrypt(struct ck_function_list *module,
	       ck_session_handle_t sess,
	       unsigned char *encrypted_data,
	       unsigned long encrypted_data_len,
	       unsigned char *data, unsigned long *data_len);

ck_rv_t
pkcs11_create_object(struct ck_function_list *module,
		     ck_session_handle_t sess,
		     struct ck_attribute *templ,
		     unsigned long count, ck_object_handle_t * object);

ck_rv_t
pkcs11_destroy_object(struct ck_function_list *module,
		      ck_session_handle_t sess, ck_object_handle_t object);

ck_rv_t
pkcs11_init_token(struct ck_function_list *module,
		  ck_slot_id_t slot_id, unsigned char *pin,
		  unsigned long pin_len, unsigned char *label);

ck_rv_t
pkcs11_init_pin(struct ck_function_list *module,
		ck_session_handle_t sess,
		unsigned char *pin, unsigned long pin_len);

ck_rv_t
pkcs11_set_pin(struct ck_function_list *module,
	       ck_session_handle_t sess,
	       const char *old_pin,
	       unsigned long old_len,
	       const char *new_pin, unsigned long new_len);

ck_rv_t
_gnutls_pkcs11_get_random(struct ck_function_list *module,
		  ck_session_handle_t sess, void *data, size_t len);


const char *pkcs11_strerror(ck_rv_t rv);

/* Returns 1 if the provided URL is an object, rather than
 * a token. */
inline static bool is_pkcs11_url_object(const char *url)
{
	if (strstr(url, "id=") != 0 || strstr(url, "object=") != 0)
		return 1;
	return 0;
}

#endif				/* ENABLE_PKCS11 */

#endif
