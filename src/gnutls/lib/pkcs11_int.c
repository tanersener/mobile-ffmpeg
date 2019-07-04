/*
 * GnuTLS PKCS#11 support
 * Copyright (C) 2010-2012 Free Software Foundation, Inc.
 * Copyright (C) 2008, Joe Orton <joe@manyfish.co.uk>
 * 
 * Authors: Nikos Mavrogiannopoulos, Stef Walter
 *
 * Inspired and some parts (pkcs11_login) based on neon PKCS #11 support 
 * by Joe Orton. More ideas came from the pkcs11-helper library by 
 * Alon Bar-Lev.
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
 */

#include "gnutls_int.h"
#include <gnutls/pkcs11.h>
#include <stdio.h>
#include <string.h>
#include "errors.h"
#include <datum.h>

#include <pin.h>
#include <pkcs11_int.h>
#include <p11-kit/p11-kit.h>
#include <p11-kit/pkcs11.h>
#include <p11-kit/pin.h>

ck_rv_t
pkcs11_get_slot_list(struct ck_function_list * module,
		     unsigned char token_present, ck_slot_id_t * slot_list,
		     unsigned long *count)
{
	return (module)->C_GetSlotList(token_present, slot_list, count);
}

ck_rv_t
pkcs11_get_module_info(struct ck_function_list * module,
		       struct ck_info * info)
{
	return (module)->C_GetInfo(info);
}

ck_rv_t
pkcs11_get_slot_info(struct ck_function_list * module,
		     ck_slot_id_t slot_id, struct ck_slot_info * info)
{
	return (module)->C_GetSlotInfo(slot_id, info);
}

ck_rv_t
pkcs11_get_token_info(struct ck_function_list * module,
		      ck_slot_id_t slot_id, struct ck_token_info * info)
{
	return (module)->C_GetTokenInfo(slot_id, info);
}

ck_rv_t
pkcs11_find_objects_init(struct ck_function_list * module,
			 ck_session_handle_t sess,
			 struct ck_attribute * templ, unsigned long count)
{
	return (module)->C_FindObjectsInit(sess, templ, count);
}

ck_rv_t
pkcs11_find_objects(struct ck_function_list * module,
		    ck_session_handle_t sess,
		    ck_object_handle_t * objects,
		    unsigned long max_object_count,
		    unsigned long *object_count)
{
	return (module)->C_FindObjects(sess, objects, max_object_count,
				       object_count);
}

ck_rv_t pkcs11_find_objects_final(struct pkcs11_session_info * sinfo)
{
	return (sinfo->module)->C_FindObjectsFinal(sinfo->pks);
}

ck_rv_t pkcs11_close_session(struct pkcs11_session_info * sinfo)
{
	sinfo->init = 0;
	return (sinfo->module)->C_CloseSession(sinfo->pks);
}

ck_rv_t
pkcs11_set_attribute_value(struct ck_function_list * module,
			   ck_session_handle_t sess,
			   ck_object_handle_t object,
			   struct ck_attribute * templ,
			   unsigned long count)
{
	return (module)->C_SetAttributeValue(sess, object, templ, count);
}

ck_rv_t
pkcs11_get_attribute_value(struct ck_function_list * module,
			   ck_session_handle_t sess,
			   ck_object_handle_t object,
			   struct ck_attribute * templ,
			   unsigned long count)
{
	return (module)->C_GetAttributeValue(sess, object, templ, count);
}

/* Returns only a single attribute value, but allocates its data 
 * Only the type needs to be set.
 */
ck_rv_t
pkcs11_get_attribute_avalue(struct ck_function_list * module,
			   ck_session_handle_t sess,
			   ck_object_handle_t object,
			   ck_attribute_type_t type,
			   gnutls_datum_t *res)
{
	ck_rv_t rv;
	struct ck_attribute templ;
	void *t;

	res->data = NULL;
	res->size = 0;

	templ.type = type;
	templ.value = NULL;
	templ.value_len = 0;
	rv = (module)->C_GetAttributeValue(sess, object, &templ, 1);
	if (rv == CKR_OK) {
		/* PKCS#11 v2.20 requires sensitive values to set a length
		 * of -1. In that case an error should have been returned,
		 * but some implementations return CKR_OK instead. */
		if (templ.value_len == (unsigned long)-1)
			return CKR_ATTRIBUTE_SENSITIVE;

		if (templ.value_len == 0)
			return rv;

		templ.type = type;
		t = gnutls_malloc(templ.value_len);
		if (t == NULL)
			return gnutls_assert_val(CKR_HOST_MEMORY);
		templ.value = t;
		rv = (module)->C_GetAttributeValue(sess, object, &templ, 1);
		if (rv != CKR_OK) {
			gnutls_free(t);
			return rv;
		}
		res->data = t;
		res->size = templ.value_len;
	}
	return rv;
}

ck_rv_t
pkcs11_get_mechanism_list(struct ck_function_list * module,
			  ck_slot_id_t slot_id,
			  ck_mechanism_type_t * mechanism_list,
			  unsigned long *count)
{
	return (module)->C_GetMechanismList(slot_id, mechanism_list,
					    count);
}

ck_rv_t
pkcs11_get_mechanism_info(struct ck_function_list *module,
			  ck_slot_id_t slot_id,
			  ck_mechanism_type_t mechanism,
			  struct ck_mechanism_info *ptr)
{
	return (module)->C_GetMechanismInfo(slot_id, mechanism,
					    ptr);
}

ck_rv_t
pkcs11_sign_init(struct ck_function_list * module,
		 ck_session_handle_t sess,
		 struct ck_mechanism * mechanism, ck_object_handle_t key)
{
	return (module)->C_SignInit(sess, mechanism, key);
}

ck_rv_t
pkcs11_sign(struct ck_function_list * module,
	    ck_session_handle_t sess,
	    unsigned char *data,
	    unsigned long data_len,
	    unsigned char *signature, unsigned long *signature_len)
{
	return (module)->C_Sign(sess, data, data_len, signature,
				signature_len);
}

ck_rv_t
pkcs11_generate_key(struct ck_function_list * module,
		    ck_session_handle_t sess,
		    struct ck_mechanism * mechanism,
		    struct ck_attribute * templ,
		    unsigned long count,
		    ck_object_handle_t * key)
{
	return (module)->C_GenerateKey(sess, mechanism, templ, count, key);
}

ck_rv_t
pkcs11_generate_key_pair(struct ck_function_list * module,
			 ck_session_handle_t sess,
			 struct ck_mechanism * mechanism,
			 struct ck_attribute * pub_templ,
			 unsigned long pub_templ_count,
			 struct ck_attribute * priv_templ,
			 unsigned long priv_templ_count,
			 ck_object_handle_t * pub_ctx,
			 ck_object_handle_t * priv_ctx)
{
	return (module)->C_GenerateKeyPair(sess, mechanism, pub_templ,
					   pub_templ_count, priv_templ,
					   priv_templ_count, pub_ctx, priv_ctx);
}

ck_rv_t
pkcs11_decrypt_init(struct ck_function_list * module,
		    ck_session_handle_t sess,
		    struct ck_mechanism * mechanism,
		    ck_object_handle_t key_ctx)
{
	return (module)->C_DecryptInit(sess, mechanism, key_ctx);
}

ck_rv_t
pkcs11_decrypt(struct ck_function_list * module,
	       ck_session_handle_t sess,
	       unsigned char *encrypted_data,
	       unsigned long encrypted_data_len,
	       unsigned char *data, unsigned long *data_len)
{
	return (module)->C_Decrypt(sess, encrypted_data,
				   encrypted_data_len, data, data_len);
}

ck_rv_t
pkcs11_create_object(struct ck_function_list * module,
		     ck_session_handle_t sess,
		     struct ck_attribute * templ,
		     unsigned long count, ck_object_handle_t * ctx)
{
	return (module)->C_CreateObject(sess, templ, count, ctx);
}

ck_rv_t
pkcs11_destroy_object(struct ck_function_list * module,
		      ck_session_handle_t sess, ck_object_handle_t ctx)
{
	return (module)->C_DestroyObject(sess, ctx);
}

ck_rv_t
pkcs11_init_token(struct ck_function_list * module,
		  ck_slot_id_t slot_id, unsigned char *pin,
		  unsigned long pin_len, unsigned char *label)
{
	return (module)->C_InitToken(slot_id, pin, pin_len, label);
}

ck_rv_t
pkcs11_init_pin(struct ck_function_list * module,
		ck_session_handle_t sess,
		unsigned char *pin, unsigned long pin_len)
{
	return (module)->C_InitPIN(sess, pin, pin_len);
}

ck_rv_t
pkcs11_set_pin(struct ck_function_list * module,
	       ck_session_handle_t sess,
	       const char *old_pin,
	       unsigned long old_len,
	       const char *new_pin, unsigned long new_len)
{
	return (module)->C_SetPIN(sess, (uint8_t *) old_pin, old_len,
				  (uint8_t *) new_pin, new_len);
}

ck_rv_t
_gnutls_pkcs11_get_random(struct ck_function_list * module,
		  ck_session_handle_t sess, void *data, size_t len)
{
	return (module)->C_GenerateRandom(sess, data, len);
}

const char *pkcs11_strerror(ck_rv_t rv)
{
	return p11_kit_strerror(rv);
}
