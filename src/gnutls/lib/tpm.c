/*
 * OpenConnect (SSL + DTLS) VPN client
 *
 * Copyright © 2012 Free Software Foundation.
 * Copyright © 2008-2012 Intel Corporation.
 * Copyright © 2015-2016 Red Hat, Inc.
 *
 * Author: David Woodhouse <dwmw2@infradead.org>
 * Author: Nikos Mavrogiannopoulos
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>
 *
 */

/*
 * TPM code based on client-tpm.c from
 * Carolin Latze <latze@angry-red-pla.net> and Tobias Soder
 */

#include <config.h>
#include <gnutls/gnutls.h>
#include <gnutls/abstract.h>
#include <gnutls/tpm.h>
#include "gnutls_int.h"

#ifdef HAVE_TROUSERS

#include "errors.h"
#include <pkcs11_int.h>
#include <x509/common.h>
#include <x509_b64.h>
#include <random.h>
#include <pin.h>
#include <c-ctype.h>

#include <dlfcn.h>
#include <trousers/tss.h>
#include <trousers/trousers.h>

typedef char *(*Trspi_Error_Layer_func)(TSS_RESULT);
typedef char *(*Trspi_Error_String_func)(TSS_RESULT);
typedef TSS_RESULT (*Trspi_Error_Code_func)(TSS_RESULT);

typedef TSS_RESULT (*Tspi_Context_CloseObject_func)(TSS_HCONTEXT, TSS_HOBJECT);
typedef TSS_RESULT (*Tspi_Context_Close_func)(TSS_HCONTEXT);
typedef TSS_RESULT (*Tspi_Context_CreateObject_func)(TSS_HCONTEXT, TSS_FLAG, TSS_FLAG, TSS_HOBJECT*);
typedef TSS_RESULT (*Tspi_Context_FreeMemory_func)(TSS_HCONTEXT, BYTE*);
typedef TSS_RESULT (*Tspi_Context_GetTpmObject_func)(TSS_HCONTEXT, TSS_HTPM*);
typedef TSS_RESULT (*Tspi_Context_LoadKeyByUUID_func)(TSS_HCONTEXT, TSS_FLAG, TSS_UUID, TSS_HKEY*);
typedef TSS_RESULT (*Tspi_Context_RegisterKey_func)(TSS_HCONTEXT, TSS_HKEY, TSS_FLAG, TSS_UUID, TSS_FLAG, TSS_UUID);
typedef TSS_RESULT (*Tspi_Context_UnregisterKey_func)(TSS_HCONTEXT, TSS_FLAG, TSS_UUID, TSS_HKEY*);
typedef TSS_RESULT (*Tspi_Key_CreateKey_func)(TSS_HKEY, TSS_HKEY, TSS_HPCRS);
typedef TSS_RESULT (*Tspi_Hash_SetHashValue_func)(TSS_HHASH, UINT32, BYTE*);
typedef TSS_RESULT (*Tspi_Hash_Sign_func)(TSS_HHASH, TSS_HKEY, UINT32*, BYTE**);
typedef TSS_RESULT (*Tspi_Policy_SetSecret_func)(TSS_HPOLICY, TSS_FLAG, UINT32, BYTE*);
typedef TSS_RESULT (*Tspi_Context_Create_func)(TSS_HCONTEXT*);
typedef TSS_RESULT (*Tspi_Context_Connect_func)(TSS_HCONTEXT, TSS_UNICODE*);
typedef TSS_RESULT (*Tspi_GetPolicyObject_func)(TSS_HOBJECT, TSS_FLAG, TSS_HPOLICY*);
typedef TSS_RESULT (*Tspi_DecodeBER_TssBlob_func)(UINT32, BYTE*, UINT32*, UINT32*, BYTE*);
typedef TSS_RESULT (*Tspi_Context_LoadKeyByBlob_func)(TSS_HCONTEXT, TSS_HKEY, UINT32, BYTE*, TSS_HKEY*);
typedef TSS_RESULT (*Tspi_Policy_AssignToObject_func)(TSS_HPOLICY, TSS_HOBJECT);
typedef TSS_RESULT (*Tspi_GetAttribData_func)(TSS_HOBJECT, TSS_FLAG, TSS_FLAG, UINT32*, BYTE**);
typedef TSS_RESULT (*Tspi_GetAttribUint32_func)(TSS_HOBJECT, TSS_FLAG, TSS_FLAG, UINT32*);
typedef TSS_RESULT (*Tspi_TPM_StirRandom_func)(TSS_HTPM, UINT32, BYTE*);
typedef TSS_RESULT (*Tspi_SetAttribUint32_func)(TSS_HOBJECT, TSS_FLAG, TSS_FLAG, UINT32);
typedef TSS_RESULT (*Tspi_EncodeDER_TssBlob_func)(UINT32, BYTE*, UINT32, UINT32*, BYTE*);
typedef TSS_RESULT (*Tspi_Context_GetRegisteredKeysByUUID2_func)(TSS_HCONTEXT, TSS_FLAG, TSS_UUID*, UINT32*, TSS_KM_KEYINFO2**);

static Tspi_Context_CloseObject_func pTspi_Context_CloseObject;
static Tspi_Context_Close_func pTspi_Context_Close;
static Tspi_Context_CreateObject_func pTspi_Context_CreateObject;
static Tspi_Context_FreeMemory_func pTspi_Context_FreeMemory;
static Tspi_Context_GetTpmObject_func pTspi_Context_GetTpmObject;
static Tspi_Context_LoadKeyByUUID_func pTspi_Context_LoadKeyByUUID;
static Tspi_Context_RegisterKey_func pTspi_Context_RegisterKey;
static Tspi_Context_UnregisterKey_func pTspi_Context_UnregisterKey;
static Tspi_Key_CreateKey_func pTspi_Key_CreateKey;
static Tspi_Hash_SetHashValue_func pTspi_Hash_SetHashValue;
static Tspi_Hash_Sign_func pTspi_Hash_Sign;
static Tspi_Policy_SetSecret_func pTspi_Policy_SetSecret;
static Tspi_Context_Create_func pTspi_Context_Create;
static Tspi_Context_Connect_func pTspi_Context_Connect;
static Tspi_GetPolicyObject_func pTspi_GetPolicyObject;
static Tspi_DecodeBER_TssBlob_func pTspi_DecodeBER_TssBlob;
static Tspi_Context_LoadKeyByBlob_func pTspi_Context_LoadKeyByBlob;
static Tspi_Policy_AssignToObject_func pTspi_Policy_AssignToObject;
static Tspi_GetAttribData_func pTspi_GetAttribData;
static Tspi_GetAttribUint32_func pTspi_GetAttribUint32;
static Tspi_Context_GetTpmObject_func pTspi_Context_GetTpmObject;
static Tspi_TPM_StirRandom_func pTspi_TPM_StirRandom;
static Tspi_SetAttribUint32_func pTspi_SetAttribUint32;
static Tspi_EncodeDER_TssBlob_func pTspi_EncodeDER_TssBlob;
static Tspi_Context_GetRegisteredKeysByUUID2_func pTspi_Context_GetRegisteredKeysByUUID2;

static Trspi_Error_Layer_func pTrspi_Error_Layer;
static Trspi_Error_String_func pTrspi_Error_String;
static Trspi_Error_Code_func pTrspi_Error_Code;

static void *tpm_dl = NULL;

#define _DLSYM(dl, sym) \
	p##sym = dlsym(dl, #sym); \
	if (p##sym == NULL) { \
		dlclose(dl); \
		dl = NULL; \
		return -1; \
	}

static int check_init(void)
{
	if (tpm_dl == NULL) {
		tpm_dl = dlopen(TROUSERS_LIB, RTLD_LAZY);
		if (tpm_dl == NULL) {
			_gnutls_debug_log("couldn't open %s\n", TROUSERS_LIB);
			return -1;
		}

		_DLSYM(tpm_dl,Tspi_Context_CloseObject);
		_DLSYM(tpm_dl,Tspi_Context_Close);
		_DLSYM(tpm_dl,Tspi_Context_CreateObject);
		_DLSYM(tpm_dl,Tspi_Context_FreeMemory);
		_DLSYM(tpm_dl,Tspi_Context_GetTpmObject);
		_DLSYM(tpm_dl,Tspi_Context_LoadKeyByUUID);
		_DLSYM(tpm_dl,Tspi_Context_RegisterKey);
		_DLSYM(tpm_dl,Tspi_Context_UnregisterKey);
		_DLSYM(tpm_dl,Tspi_Key_CreateKey);
		_DLSYM(tpm_dl,Tspi_Hash_SetHashValue);
		_DLSYM(tpm_dl,Tspi_Hash_Sign);
		_DLSYM(tpm_dl,Tspi_Policy_SetSecret);
		_DLSYM(tpm_dl,Tspi_Context_Create);
		_DLSYM(tpm_dl,Tspi_Context_Connect);
		_DLSYM(tpm_dl,Tspi_GetPolicyObject);
		_DLSYM(tpm_dl,Tspi_DecodeBER_TssBlob);
		_DLSYM(tpm_dl,Tspi_Context_LoadKeyByBlob);
		_DLSYM(tpm_dl,Tspi_Policy_AssignToObject);
		_DLSYM(tpm_dl,Tspi_GetAttribData);
		_DLSYM(tpm_dl,Tspi_GetAttribUint32);
		_DLSYM(tpm_dl,Tspi_Context_GetTpmObject);
		_DLSYM(tpm_dl,Tspi_TPM_StirRandom);
		_DLSYM(tpm_dl,Tspi_SetAttribUint32);
		_DLSYM(tpm_dl,Tspi_EncodeDER_TssBlob);
		_DLSYM(tpm_dl,Tspi_Context_GetRegisteredKeysByUUID2);

		_DLSYM(tpm_dl,Trspi_Error_Layer);
		_DLSYM(tpm_dl,Trspi_Error_String);
		_DLSYM(tpm_dl,Trspi_Error_Code);
	}

	return 0;
}

#define CHECK_INIT \
		if (check_init() < 0) return gnutls_assert_val(GNUTLS_E_TPM_NO_LIB)

#define CHECK_INIT_VOID \
		if (check_init() < 0) return

void _gnutls_tpm_global_deinit(void)
{
	if (tpm_dl) {
		dlclose(tpm_dl);
		tpm_dl = NULL;
	}
}

struct tpm_ctx_st {
	TSS_HCONTEXT tpm_ctx;
	TSS_HKEY tpm_key;
	TSS_HPOLICY tpm_key_policy;
	TSS_HKEY srk;
	TSS_HPOLICY srk_policy;
};

struct tpm_key_list_st {
	UINT32 size;
	TSS_KM_KEYINFO2 *ki;
	TSS_HCONTEXT tpm_ctx;
};

static void tpm_close_session(struct tpm_ctx_st *s);
static int import_tpm_key(gnutls_privkey_t pkey,
			  const gnutls_datum_t * fdata,
			  gnutls_tpmkey_fmt_t format,
			  TSS_UUID * uuid,
			  TSS_FLAG storage_type,
			  const char *srk_password,
			  const char *key_password);
static int encode_tpmkey_url(char **url, const TSS_UUID * uuid,
			     TSS_FLAG storage);

/* TPM URL format: (draft-mavrogiannopoulos-tpmuri-01)
 *
 * tpmkey:file=/path/to/file
 * tpmkey:uuid=7f468c16-cb7f-11e1-824d-b3a4f4b20343;storage=user
 * tpmkey:uuid=7f468c16-cb7f-11e1-824d-b3a4f4b20343;storage=system
 *
 */


static int tss_err_pwd(TSS_RESULT err, int pwd_error)
{
	_gnutls_debug_log("TPM (%s) error: %s (%x)\n",
			  pTrspi_Error_Layer(err), pTrspi_Error_String(err),
			  (unsigned int) pTrspi_Error_Code(err));

	switch (ERROR_LAYER(err)) {
	case TSS_LAYER_TPM:
		switch (ERROR_CODE(err)) {
		case TPM_E_AUTHFAIL:
			return pwd_error;
		case TPM_E_NOSRK:
			return GNUTLS_E_TPM_UNINITIALIZED;
		default:
			return gnutls_assert_val(GNUTLS_E_TPM_ERROR);
		}
	case TSS_LAYER_TCS:
	case TSS_LAYER_TSP:
		switch (ERROR_CODE(err)) {
		case TSS_E_COMM_FAILURE:
		case TSS_E_NO_CONNECTION:
		case TSS_E_CONNECTION_FAILED:
		case TSS_E_CONNECTION_BROKEN:
			return GNUTLS_E_TPM_SESSION_ERROR;
		case TSS_E_PS_KEY_NOTFOUND:
			return GNUTLS_E_TPM_KEY_NOT_FOUND;
		default:
			return gnutls_assert_val(GNUTLS_E_TPM_ERROR);
		}
	default:
		return gnutls_assert_val(GNUTLS_E_TPM_ERROR);
	}
}

#define tss_err(x) tss_err_pwd(x, GNUTLS_E_TPM_SRK_PASSWORD_ERROR)
#define tss_err_key(x) tss_err_pwd(x, GNUTLS_E_TPM_KEY_PASSWORD_ERROR)

static void tpm_deinit_fn(gnutls_privkey_t key, void *_s)
{
	struct tpm_ctx_st *s = _s;

	pTspi_Context_CloseObject(s->tpm_ctx, s->tpm_key_policy);
	pTspi_Context_CloseObject(s->tpm_ctx, s->tpm_key);

	tpm_close_session(s);
	gnutls_free(s);
}

static int
tpm_sign_fn(gnutls_privkey_t key, void *_s,
	    const gnutls_datum_t * data, gnutls_datum_t * sig)
{
	struct tpm_ctx_st *s = _s;
	TSS_HHASH hash;
	int err;

	_gnutls_debug_log("TPM sign function called for %u bytes.\n",
			  data->size);

	err =
	    pTspi_Context_CreateObject(s->tpm_ctx,
				      TSS_OBJECT_TYPE_HASH, TSS_HASH_OTHER,
				      &hash);
	if (err) {
		gnutls_assert();
		_gnutls_debug_log("Failed to create TPM hash object: %s\n",
				  pTrspi_Error_String(err));
		return GNUTLS_E_PK_SIGN_FAILED;
	}
	err = pTspi_Hash_SetHashValue(hash, data->size, data->data);
	if (err) {
		gnutls_assert();
		_gnutls_debug_log
		    ("Failed to set value in TPM hash object: %s\n",
		     pTrspi_Error_String(err));
		pTspi_Context_CloseObject(s->tpm_ctx, hash);
		return GNUTLS_E_PK_SIGN_FAILED;
	}
	err = pTspi_Hash_Sign(hash, s->tpm_key, &sig->size, &sig->data);
	pTspi_Context_CloseObject(s->tpm_ctx, hash);
	if (err) {
		if (s->tpm_key_policy || err != TPM_E_AUTHFAIL)
			_gnutls_debug_log
			    ("TPM hash signature failed: %s\n",
			     pTrspi_Error_String(err));
		if (err == TPM_E_AUTHFAIL)
			return GNUTLS_E_TPM_KEY_PASSWORD_ERROR;
		else
			return GNUTLS_E_PK_SIGN_FAILED;
	}
	return 0;
}

static const unsigned char nullpass[20];
static const gnutls_datum_t nulldata = { (void *) nullpass, 20 };

const TSS_UUID srk_uuid = TSS_UUID_SRK;

static int tpm_pin(struct pin_info_st *pin_info, const TSS_UUID * uuid,
		   TSS_FLAG storage, char *pin, unsigned int pin_size,
		   unsigned int attempts)
{
	unsigned int flags = 0;
	const char *label;
	char *url = NULL;
	int ret;

	if (attempts > 0)
		flags |= GNUTLS_PIN_WRONG;

	if (uuid) {
		if (memcmp(uuid, &srk_uuid, sizeof(TSS_UUID)) == 0) {
			label = "SRK";

			ret = encode_tpmkey_url(&url, uuid, storage);
			if (ret < 0)
				return gnutls_assert_val(ret);
		} else {
			label = "TPM";

			ret = encode_tpmkey_url(&url, uuid, storage);
			if (ret < 0)
				return gnutls_assert_val(ret);
		}
	} else
		label = "unknown";

	ret = _gnutls_retrieve_pin(pin_info, url, label, flags, pin, pin_size);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = 0;
      cleanup:
	gnutls_free(url);
	return ret;
}


static TSS_RESULT myTspi_Policy_SetSecret(TSS_HPOLICY hPolicy,
					  UINT32 ulSecretLength,
					  BYTE * rgbSecret)
{
	if (rgbSecret == NULL) {
		/* Well known NULL key */
		return pTspi_Policy_SetSecret(hPolicy,
					     TSS_SECRET_MODE_SHA1,
					     sizeof(nullpass),
					     (BYTE *) nullpass);
	} else {		/* key is given */

		return pTspi_Policy_SetSecret(hPolicy,
					     TSS_SECRET_MODE_PLAIN,
					     ulSecretLength, rgbSecret);
	}
}

#define SAFE_LEN(x) (x==NULL?0:strlen(x))

static int tpm_open_session(struct tpm_ctx_st *s, const char *_srk_password, unsigned allow_invalid_pass)
{
	int err, ret;
	char *password = NULL;

	err = pTspi_Context_Create(&s->tpm_ctx);
	if (err) {
		gnutls_assert();
		return tss_err(err);
	}

	if (_srk_password != NULL) {
		gnutls_datum_t pout;
		ret = _gnutls_utf8_password_normalize(_srk_password, strlen(_srk_password), &pout, allow_invalid_pass);
		if (ret < 0) {
			gnutls_assert();
			goto out_tspi_ctx;
		}
		password = (char*)pout.data;
	}

	err = pTspi_Context_Connect(s->tpm_ctx, NULL);
	if (err) {
		gnutls_assert();
		ret = tss_err(err);
		goto out_tspi_ctx;
	}

	err =
	    pTspi_Context_LoadKeyByUUID(s->tpm_ctx, TSS_PS_TYPE_SYSTEM,
				       srk_uuid, &s->srk);
	if (err) {
		gnutls_assert();
		ret = tss_err(err);
		goto out_tspi_ctx;
	}

	err =
	    pTspi_GetPolicyObject(s->srk, TSS_POLICY_USAGE, &s->srk_policy);
	if (err) {
		gnutls_assert();
		ret = tss_err(err);
		goto out_srk;
	}

	err = myTspi_Policy_SetSecret(s->srk_policy,
				      SAFE_LEN(password),
				      (BYTE *) password);
	if (err) {
		gnutls_assert();
		ret = tss_err(err);
		goto out_srkpol;
	}
	gnutls_free(password);

	return 0;

      out_srkpol:
	pTspi_Context_CloseObject(s->tpm_ctx, s->srk_policy);
	s->srk_policy = 0;
      out_srk:
	pTspi_Context_CloseObject(s->tpm_ctx, s->srk);
	s->srk = 0;
      out_tspi_ctx:
	pTspi_Context_Close(s->tpm_ctx);
	s->tpm_ctx = 0;
	gnutls_free(password);
	return ret;

}

static void tpm_close_session(struct tpm_ctx_st *s)
{
	pTspi_Context_CloseObject(s->tpm_ctx, s->srk_policy);
	s->srk_policy = 0;
	pTspi_Context_CloseObject(s->tpm_ctx, s->srk);
	s->srk = 0;
	pTspi_Context_Close(s->tpm_ctx);
	s->tpm_ctx = 0;
}

static int
import_tpm_key_cb(gnutls_privkey_t pkey, const gnutls_datum_t * fdata,
		  gnutls_tpmkey_fmt_t format, TSS_UUID * uuid,
		  TSS_FLAG storage, const char *srk_password,
		  const char *key_password)
{
	unsigned int attempts = 0;
	char pin1[GNUTLS_PKCS11_MAX_PIN_LEN];
	char pin2[GNUTLS_PKCS11_MAX_PIN_LEN];
	int ret, ret2;

	do {
		ret =
		    import_tpm_key(pkey, fdata, format, uuid, storage,
				   srk_password, key_password);

		if (attempts > 3)
			break;

		if (ret == GNUTLS_E_TPM_SRK_PASSWORD_ERROR) {
			ret2 =
			    tpm_pin(&pkey->pin, &srk_uuid, storage, pin1,
				    sizeof(pin1), attempts++);
			if (ret2 < 0) {
				gnutls_assert();
				return GNUTLS_E_TPM_SRK_PASSWORD_ERROR;
			}
			srk_password = pin1;
		}

		if (ret == GNUTLS_E_TPM_KEY_PASSWORD_ERROR) {
			ret2 =
			    tpm_pin(&pkey->pin, uuid, storage, pin2,
				    sizeof(pin2), attempts++);
			if (ret2 < 0) {
				gnutls_assert();
				return GNUTLS_E_TPM_KEY_PASSWORD_ERROR;
			}
			key_password = pin2;
		}
	}
	while (ret == GNUTLS_E_TPM_KEY_PASSWORD_ERROR
	       || ret == GNUTLS_E_TPM_SRK_PASSWORD_ERROR);

	if (ret < 0)
		gnutls_assert();
	return ret;
}

static int load_key(TSS_HCONTEXT tpm_ctx, TSS_HKEY srk,
		    const gnutls_datum_t * fdata,
		    gnutls_tpmkey_fmt_t format, TSS_HKEY * tpm_key)
{
	int ret, err;
	gnutls_datum_t asn1 = { NULL, 0 };

	if (format == GNUTLS_TPMKEY_FMT_CTK_PEM) {
		gnutls_datum_t td;

		ret =
		    gnutls_pem_base64_decode2("TSS KEY BLOB", fdata,
					      &asn1);
		if (ret) {
			gnutls_assert();
			_gnutls_debug_log
			    ("Error decoding TSS key blob: %s\n",
			     gnutls_strerror(ret));
			return ret;
		}

		ret =
		    _gnutls_x509_decode_string(ASN1_ETYPE_OCTET_STRING,
					       asn1.data, asn1.size, &td, 0);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}
		gnutls_free(asn1.data);
		asn1.data = td.data;
		asn1.size = td.size;
	} else {		/* DER */

		UINT32 tint2;
		UINT32 type;

		asn1.size = fdata->size;
		asn1.data = gnutls_malloc(asn1.size);
		if (asn1.data == NULL) {
			gnutls_assert();
			return GNUTLS_E_MEMORY_ERROR;
		}

		tint2 = asn1.size;
		err =
		    pTspi_DecodeBER_TssBlob(fdata->size, fdata->data, &type,
					   &tint2, asn1.data);
		if (err != 0) {
			gnutls_assert();
			ret = tss_err(err);
			goto cleanup;
		}

		asn1.size = tint2;
	}

	/* ... we get it here instead. */
	err = pTspi_Context_LoadKeyByBlob(tpm_ctx, srk,
					 asn1.size, asn1.data, tpm_key);
	if (err != 0) {
		gnutls_assert();
		ret = tss_err(err);
		goto cleanup;
	}

	ret = 0;

      cleanup:
	gnutls_free(asn1.data);

	return ret;
}

static int
import_tpm_key(gnutls_privkey_t pkey,
	       const gnutls_datum_t * fdata,
	       gnutls_tpmkey_fmt_t format,
	       TSS_UUID * uuid,
	       TSS_FLAG storage,
	       const char *srk_password, const char *_key_password)
{
	int err, ret;
	struct tpm_ctx_st *s;
	gnutls_datum_t tmp_sig;
	char *key_password = NULL;
	uint32_t authusage;

	s = gnutls_malloc(sizeof(*s));
	if (s == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	if (_key_password != NULL) {
		gnutls_datum_t pout;
		ret = _gnutls_utf8_password_normalize(_key_password, strlen(_key_password), &pout, 1);
		if (ret < 0) {
			gnutls_assert();
			goto out_ctx;
		}
		key_password = (char*)pout.data;
	}

	/* normalization of srk_password happens in tpm_open_session() */

	ret = tpm_open_session(s, srk_password, 1);
	if (ret < 0) {
		gnutls_assert();
		goto out_ctx;
	}

	if (fdata != NULL) {
		ret =
		    load_key(s->tpm_ctx, s->srk, fdata, format,
			     &s->tpm_key);
		if (ret < 0) {
			gnutls_assert();
			goto out_session;
		}
	} else if (uuid) {
		err =
		    pTspi_Context_LoadKeyByUUID(s->tpm_ctx, storage,
					       *uuid, &s->tpm_key);

		if (err) {
			gnutls_assert();
			ret = tss_err(err);
			goto out_session;
		}
	} else {
		gnutls_assert();
		ret = GNUTLS_E_INVALID_REQUEST;
		goto out_session;
	}

	err = pTspi_GetAttribUint32(s->tpm_key, TSS_TSPATTRIB_KEY_INFO,
				    TSS_TSPATTRIB_KEYINFO_AUTHUSAGE,
				    &authusage);
	if (err) {
		gnutls_assert();
		ret = tss_err(err);
		goto out_session;
	}

	if (authusage) {
		if (!_key_password) {
			ret = GNUTLS_E_TPM_KEY_PASSWORD_ERROR;
			goto out_session;
		}

		err = pTspi_Context_CreateObject(s->tpm_ctx,
						 TSS_OBJECT_TYPE_POLICY,
						 TSS_POLICY_USAGE,
						 &s->tpm_key_policy);
		if (err) {
			gnutls_assert();
			ret = tss_err(err);
			goto out_key;
		}

		err = pTspi_Policy_AssignToObject(s->tpm_key_policy,
						       s->tpm_key);
		if (err) {
			gnutls_assert();
			ret = tss_err(err);
			goto out_key_policy;
		}

		err = myTspi_Policy_SetSecret(s->tpm_key_policy,
					      SAFE_LEN(key_password),
					      (void *) key_password);

		if (err) {
			gnutls_assert();
			ret = tss_err_key(err);
			goto out_key_policy;
		}
	}

	ret =
	    gnutls_privkey_import_ext2(pkey, GNUTLS_PK_RSA, s,
				       tpm_sign_fn, NULL, tpm_deinit_fn,
				       0);
	if (ret < 0) {
		gnutls_assert();
		goto out_session;
	}

	ret =
	    gnutls_privkey_sign_data(pkey, GNUTLS_DIG_SHA1, 0, &nulldata,
				     &tmp_sig);
	if (ret < 0) {
		gnutls_assert();
		goto out_session;
	}

	gnutls_free(key_password);

	return 0;
      out_key_policy:
	pTspi_Context_CloseObject(s->tpm_ctx, s->tpm_key_policy);
	s->tpm_key_policy = 0;
      out_key:
	pTspi_Context_CloseObject(s->tpm_ctx, s->tpm_key);
	s->tpm_key = 0;
      out_session:
	_gnutls_privkey_cleanup(pkey);
	tpm_close_session(s);
      out_ctx:
	gnutls_free(s);
	gnutls_free(key_password);
	return ret;
}

/**
 * gnutls_privkey_import_tpm_raw:
 * @pkey: The private key
 * @fdata: The TPM key to be imported
 * @format: The format of the private key
 * @srk_password: The password for the SRK key (optional)
 * @key_password: A password for the key (optional)
 * @flags: should be zero
 *
 * This function will import the given private key to the abstract
 * #gnutls_privkey_t type. 
 *
 * With respect to passwords the same as in gnutls_privkey_import_tpm_url() apply.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.1.0
 *
 **/
int
gnutls_privkey_import_tpm_raw(gnutls_privkey_t pkey,
			      const gnutls_datum_t * fdata,
			      gnutls_tpmkey_fmt_t format,
			      const char *srk_password,
			      const char *key_password, unsigned int flags)
{
	CHECK_INIT;

	if (flags & GNUTLS_PRIVKEY_DISABLE_CALLBACKS)
		return import_tpm_key(pkey, fdata, format, NULL, 0,
				      srk_password, key_password);
	else
		return import_tpm_key_cb(pkey, fdata, format, NULL, 0,
					 srk_password, key_password);
}

struct tpmkey_url_st {
	char *filename;
	TSS_UUID uuid;
	TSS_FLAG storage;
	unsigned int uuid_set;
};

static void clear_tpmkey_url(struct tpmkey_url_st *s)
{
	gnutls_free(s->filename);
	memset(s, 0, sizeof(*s));
}

static int
unescape_string(char *output, const char *input, size_t * size,
		char terminator)
{
	gnutls_buffer_st str;
	int ret = 0;
	char *p;
	int len;

	_gnutls_buffer_init(&str);

	/* find terminator */
	p = strchr(input, terminator);
	if (p != NULL)
		len = p - input;
	else
		len = strlen(input);

	ret = _gnutls_buffer_append_data(&str, input, len);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	ret = _gnutls_buffer_unescape(&str);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	ret = _gnutls_buffer_append_data(&str, "", 1);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	ret = _gnutls_buffer_pop_data(&str, output, str.length);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	_gnutls_buffer_clear(&str);

	return ret;
}

#define UUID_SIZE 16

static int randomize_uuid(TSS_UUID * uuid)
{
	uint8_t raw_uuid[16];
	int ret;

	ret = gnutls_rnd(GNUTLS_RND_NONCE, raw_uuid, sizeof(raw_uuid));
	if (ret < 0)
		return gnutls_assert_val(ret);

	/* mark it as random uuid */
	raw_uuid[6] &= 0x0f;
	raw_uuid[6] |= 0x40;
	raw_uuid[8] &= 0x0f;
	raw_uuid[8] |= 0x80;

	memcpy(&uuid->ulTimeLow, raw_uuid, 4);
	memcpy(&uuid->usTimeMid, &raw_uuid[4], 2);
	memcpy(&uuid->usTimeHigh, &raw_uuid[6], 2);
	uuid->bClockSeqHigh = raw_uuid[8];
	uuid->bClockSeqLow = raw_uuid[9];
	memcpy(&uuid->rgbNode, &raw_uuid[10], 6);

	return 0;
}

static int encode_tpmkey_url(char **url, const TSS_UUID * uuid,
			     TSS_FLAG storage)
{
	uint8_t u1[UUID_SIZE];
	gnutls_buffer_st buf;
	gnutls_datum_t dret;
	int ret;

	_gnutls_buffer_init(&buf);

	memcpy(u1, &uuid->ulTimeLow, 4);
	memcpy(&u1[4], &uuid->usTimeMid, 2);
	memcpy(&u1[6], &uuid->usTimeHigh, 2);
	u1[8] = uuid->bClockSeqHigh;
	u1[9] = uuid->bClockSeqLow;
	memcpy(&u1[10], uuid->rgbNode, 6);

	ret = _gnutls_buffer_append_str(&buf, "tpmkey:uuid=");
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret =
	    _gnutls_buffer_append_printf(&buf,
					 "%.2x%.2x%.2x%.2x-%.2x%.2x-%.2x%.2x-%.2x%.2x-%.2x%.2x%.2x%.2x%.2x%.2x",
					 (unsigned int) u1[0],
					 (unsigned int) u1[1],
					 (unsigned int) u1[2],
					 (unsigned int) u1[3],
					 (unsigned int) u1[4],
					 (unsigned int) u1[5],
					 (unsigned int) u1[6],
					 (unsigned int) u1[7],
					 (unsigned int) u1[8],
					 (unsigned int) u1[9],
					 (unsigned int) u1[10],
					 (unsigned int) u1[11],
					 (unsigned int) u1[12],
					 (unsigned int) u1[13],
					 (unsigned int) u1[14],
					 (unsigned int) u1[15]);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret =
	    _gnutls_buffer_append_printf(&buf, ";storage=%s",
					 (storage ==
					  TSS_PS_TYPE_USER) ? "user" :
					 "system");
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = _gnutls_buffer_to_datum(&buf, &dret, 1);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	*url = (char *) dret.data;

	return 0;
      cleanup:
	_gnutls_buffer_clear(&buf);
	return ret;
}

static int decode_tpmkey_url(const char *url, struct tpmkey_url_st *s)
{
	char *p;
	size_t size;
	int ret;
	unsigned int i, j;

	if (strstr(url, "tpmkey:") == NULL)
		return gnutls_assert_val(GNUTLS_E_PARSING_ERROR);

	memset(s, 0, sizeof(*s));

	p = strstr(url, "file=");
	if (p != NULL) {
		p += sizeof("file=") - 1;
		size = strlen(p);
		s->filename = gnutls_malloc(size + 1);
		if (s->filename == NULL)
			return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

		ret = unescape_string(s->filename, p, &size, ';');
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}
		s->filename[size] = 0;
	} else if ((p = strstr(url, "uuid=")) != NULL) {
		char tmp_uuid[33];
		uint8_t raw_uuid[16];
		gnutls_datum_t tmp;

		p += sizeof("uuid=") - 1;
		size = strlen(p);

		for (j = i = 0; i < size; i++) {
			if (j == sizeof(tmp_uuid) - 1) {
				break;
			}
			if (c_isalnum(p[i]))
				tmp_uuid[j++] = p[i];
		}
		tmp_uuid[j] = 0;

		tmp.data = (void*)tmp_uuid;
		tmp.size = strlen(tmp_uuid);
		size = sizeof(raw_uuid);
		ret =
		    gnutls_hex_decode(&tmp, raw_uuid,
				      &size);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		memcpy(&s->uuid.ulTimeLow, raw_uuid, 4);
		memcpy(&s->uuid.usTimeMid, &raw_uuid[4], 2);
		memcpy(&s->uuid.usTimeHigh, &raw_uuid[6], 2);
		s->uuid.bClockSeqHigh = raw_uuid[8];
		s->uuid.bClockSeqLow = raw_uuid[9];
		memcpy(&s->uuid.rgbNode, &raw_uuid[10], 6);
		s->uuid_set = 1;
	} else {
		return gnutls_assert_val(GNUTLS_E_PARSING_ERROR);
	}

	if (strstr(url, "storage=user") != NULL)
		s->storage = TSS_PS_TYPE_USER;
	else
		s->storage = TSS_PS_TYPE_SYSTEM;

	return 0;

      cleanup:
	clear_tpmkey_url(s);
	return ret;
}

/**
 * gnutls_privkey_import_tpm_url:
 * @pkey: The private key
 * @url: The URL of the TPM key to be imported
 * @srk_password: The password for the SRK key (optional)
 * @key_password: A password for the key (optional)
 * @flags: One of the GNUTLS_PRIVKEY_* flags
 *
 * This function will import the given private key to the abstract
 * #gnutls_privkey_t type.
 *
 * Note that unless %GNUTLS_PRIVKEY_DISABLE_CALLBACKS
 * is specified, if incorrect (or NULL) passwords are given
 * the PKCS11 callback functions will be used to obtain the
 * correct passwords. Otherwise if the SRK password is wrong
 * %GNUTLS_E_TPM_SRK_PASSWORD_ERROR is returned and if the key password
 * is wrong or not provided then %GNUTLS_E_TPM_KEY_PASSWORD_ERROR
 * is returned. 
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.1.0
 *
 **/
int
gnutls_privkey_import_tpm_url(gnutls_privkey_t pkey,
			      const char *url,
			      const char *srk_password,
			      const char *key_password, unsigned int flags)
{
	struct tpmkey_url_st durl;
	gnutls_datum_t fdata = { NULL, 0 };
	int ret;

	CHECK_INIT;

	ret = decode_tpmkey_url(url, &durl);
	if (ret < 0)
		return gnutls_assert_val(ret);

	if (durl.filename) {
		ret = gnutls_load_file(durl.filename, &fdata);
		if (ret < 0) {
			gnutls_assert();
			_gnutls_debug_log("Error loading %s\n",
					  durl.filename);
			goto cleanup;
		}

		ret =
		    gnutls_privkey_import_tpm_raw(pkey, &fdata,
						  GNUTLS_TPMKEY_FMT_CTK_PEM,
						  srk_password,
						  key_password, flags);
		if (ret == GNUTLS_E_BASE64_UNEXPECTED_HEADER_ERROR)
			ret =
			    gnutls_privkey_import_tpm_raw(pkey, &fdata,
							  GNUTLS_TPMKEY_FMT_RAW,
							  srk_password,
							  key_password,
							  flags);

		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}
	} else if (durl.uuid_set) {
		if (flags & GNUTLS_PRIVKEY_DISABLE_CALLBACKS)
			ret =
			    import_tpm_key(pkey, NULL, 0, &durl.uuid,
					   durl.storage, srk_password,
					   key_password);
		else
			ret =
			    import_tpm_key_cb(pkey, NULL, 0, &durl.uuid,
					      durl.storage, srk_password,
					      key_password);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}
	}

	ret = 0;
      cleanup:
	gnutls_free(fdata.data);
	clear_tpmkey_url(&durl);
	return ret;
}


/* reads the RSA public key from the given TSS key.
 * If psize is non-null it contains the total size of the parameters
 * in bytes */
static int read_pubkey(gnutls_pubkey_t pub, TSS_HKEY key_ctx,
		       size_t * psize)
{
	void *tdata;
	UINT32 tint;
	TSS_RESULT tssret;
	gnutls_datum_t m, e;
	int ret;

	/* read the public key */

	tssret = pTspi_GetAttribData(key_ctx, TSS_TSPATTRIB_RSAKEY_INFO,
				    TSS_TSPATTRIB_KEYINFO_RSA_MODULUS,
				    &tint, (void *) &tdata);
	if (tssret != 0) {
		gnutls_assert();
		return tss_err(tssret);
	}

	m.data = tdata;
	m.size = tint;

	tssret = pTspi_GetAttribData(key_ctx, TSS_TSPATTRIB_RSAKEY_INFO,
				    TSS_TSPATTRIB_KEYINFO_RSA_EXPONENT,
				    &tint, (void *) &tdata);
	if (tssret != 0) {
		gnutls_assert();
		pTspi_Context_FreeMemory(key_ctx, m.data);
		return tss_err(tssret);
	}

	e.data = tdata;
	e.size = tint;

	ret = gnutls_pubkey_import_rsa_raw(pub, &m, &e);

	pTspi_Context_FreeMemory(key_ctx, m.data);
	pTspi_Context_FreeMemory(key_ctx, e.data);

	if (ret < 0)
		return gnutls_assert_val(ret);

	if (psize)
		*psize = e.size + m.size;

	return 0;
}



static int
import_tpm_pubkey(gnutls_pubkey_t pkey,
		  const gnutls_datum_t * fdata,
		  gnutls_tpmkey_fmt_t format,
		  TSS_UUID * uuid,
		  TSS_FLAG storage, const char *srk_password)
{
	int err, ret;
	struct tpm_ctx_st s;

	ret = tpm_open_session(&s, srk_password, 1);
	if (ret < 0)
		return gnutls_assert_val(ret);

	if (fdata != NULL) {
		ret =
		    load_key(s.tpm_ctx, s.srk, fdata, format, &s.tpm_key);
		if (ret < 0) {
			gnutls_assert();
			goto out_session;
		}
	} else if (uuid) {
		err =
		    pTspi_Context_LoadKeyByUUID(s.tpm_ctx, storage,
					       *uuid, &s.tpm_key);
		if (err) {
			gnutls_assert();
			ret = tss_err(err);
			goto out_session;
		}
	} else {
		gnutls_assert();
		ret = GNUTLS_E_INVALID_REQUEST;
		goto out_session;
	}

	ret = read_pubkey(pkey, s.tpm_key, NULL);
	if (ret < 0) {
		gnutls_assert();
		goto out_session;
	}

	ret = 0;
      out_session:
	tpm_close_session(&s);
	return ret;
}

static int
import_tpm_pubkey_cb(gnutls_pubkey_t pkey,
		     const gnutls_datum_t * fdata,
		     gnutls_tpmkey_fmt_t format,
		     TSS_UUID * uuid,
		     TSS_FLAG storage, const char *srk_password)
{
	unsigned int attempts = 0;
	char pin1[GNUTLS_PKCS11_MAX_PIN_LEN];
	int ret, sret;

	do {
		ret =
		    import_tpm_pubkey(pkey, fdata, format, uuid, storage,
				      srk_password);

		if (attempts > 3)
			break;

		if (ret == GNUTLS_E_TPM_SRK_PASSWORD_ERROR) {
			sret =
			    tpm_pin(&pkey->pin, &srk_uuid, storage, pin1,
				    sizeof(pin1), attempts++);
			if (sret < 0) {
				gnutls_assert();
				return GNUTLS_E_TPM_SRK_PASSWORD_ERROR;
			}
			srk_password = pin1;
		}
	}
	while (ret == GNUTLS_E_TPM_SRK_PASSWORD_ERROR);

	if (ret < 0)
		gnutls_assert();
	return ret;
}


/**
 * gnutls_pubkey_import_tpm_raw:
 * @pkey: The public key
 * @fdata: The TPM key to be imported
 * @format: The format of the private key
 * @srk_password: The password for the SRK key (optional)
 * @flags: One of the GNUTLS_PUBKEY_* flags
 *
 * This function will import the public key from the provided TPM key
 * structure.
 *
 * With respect to passwords the same as in
 * gnutls_pubkey_import_tpm_url() apply.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.1.0
 **/
int
gnutls_pubkey_import_tpm_raw(gnutls_pubkey_t pkey,
			     const gnutls_datum_t * fdata,
			     gnutls_tpmkey_fmt_t format,
			     const char *srk_password, unsigned int flags)
{
	CHECK_INIT;

	if (flags & GNUTLS_PUBKEY_DISABLE_CALLBACKS)
		return import_tpm_pubkey_cb(pkey, fdata, format, NULL, 0,
					    srk_password);
	else
		return import_tpm_pubkey(pkey, fdata, format, NULL, 0,
					 srk_password);
}

/**
 * gnutls_pubkey_import_tpm_url:
 * @pkey: The public key
 * @url: The URL of the TPM key to be imported
 * @srk_password: The password for the SRK key (optional)
 * @flags: should be zero
 *
 * This function will import the given private key to the abstract
 * #gnutls_privkey_t type. 
 *
 * Note that unless %GNUTLS_PUBKEY_DISABLE_CALLBACKS
 * is specified, if incorrect (or NULL) passwords are given
 * the PKCS11 callback functions will be used to obtain the
 * correct passwords. Otherwise if the SRK password is wrong
 * %GNUTLS_E_TPM_SRK_PASSWORD_ERROR is returned.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.1.0
 *
 **/
int
gnutls_pubkey_import_tpm_url(gnutls_pubkey_t pkey,
			     const char *url,
			     const char *srk_password, unsigned int flags)
{
	struct tpmkey_url_st durl;
	gnutls_datum_t fdata = { NULL, 0 };
	int ret;

	CHECK_INIT;

	ret = decode_tpmkey_url(url, &durl);
	if (ret < 0)
		return gnutls_assert_val(ret);

	if (durl.filename) {

		ret = gnutls_load_file(durl.filename, &fdata);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		ret =
		    gnutls_pubkey_import_tpm_raw(pkey, &fdata,
						 GNUTLS_TPMKEY_FMT_CTK_PEM,
						 srk_password, flags);
		if (ret == GNUTLS_E_BASE64_UNEXPECTED_HEADER_ERROR)
			ret =
			    gnutls_pubkey_import_tpm_raw(pkey, &fdata,
							 GNUTLS_TPMKEY_FMT_RAW,
							 srk_password,
							 flags);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}
	} else if (durl.uuid_set) {
		if (flags & GNUTLS_PUBKEY_DISABLE_CALLBACKS)
			ret =
			    import_tpm_pubkey(pkey, NULL, 0, &durl.uuid,
					      durl.storage, srk_password);
		else
			ret =
			    import_tpm_pubkey_cb(pkey, NULL, 0, &durl.uuid,
						 durl.storage,
						 srk_password);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}
	}

	ret = 0;
      cleanup:
	gnutls_free(fdata.data);
	clear_tpmkey_url(&durl);
	return ret;
}


/**
 * gnutls_tpm_privkey_generate:
 * @pk: the public key algorithm
 * @bits: the security bits
 * @srk_password: a password to protect the exported key (optional)
 * @key_password: the password for the TPM (optional)
 * @format: the format of the private key
 * @pub_format: the format of the public key
 * @privkey: the generated key
 * @pubkey: the corresponding public key (may be null)
 * @flags: should be a list of GNUTLS_TPM_* flags
 *
 * This function will generate a private key in the TPM
 * chip. The private key will be generated within the chip
 * and will be exported in a wrapped with TPM's master key
 * form. Furthermore the wrapped key can be protected with
 * the provided @password.
 *
 * Note that bits in TPM is quantized value. If the input value
 * is not one of the allowed values, then it will be quantized to
 * one of 512, 1024, 2048, 4096, 8192 and 16384.
 *
 * Allowed flags are:
 *
 * %GNUTLS_TPM_KEY_SIGNING: Generate a signing key instead of a legacy,
 *
 * %GNUTLS_TPM_REGISTER_KEY: Register the generate key in TPM. In that
 * case @privkey would contain a URL with the UUID.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.1.0
 **/
int
gnutls_tpm_privkey_generate(gnutls_pk_algorithm_t pk, unsigned int bits,
			    const char *srk_password,
			    const char *key_password,
			    gnutls_tpmkey_fmt_t format,
			    gnutls_x509_crt_fmt_t pub_format,
			    gnutls_datum_t * privkey,
			    gnutls_datum_t * pubkey, unsigned int flags)
{
	TSS_FLAG tpm_flags = TSS_KEY_VOLATILE;
	TSS_HKEY key_ctx;
	TSS_RESULT tssret;
	int ret;
	void *tdata;
	UINT32 tint;
	gnutls_datum_t tmpkey = { NULL, 0 };
	TSS_HPOLICY key_policy;
	gnutls_pubkey_t pub;
	struct tpm_ctx_st s;
	TSS_FLAG storage_type;
	TSS_HTPM htpm;
	uint8_t buf[32];

	CHECK_INIT;

	privkey->data = NULL;
	if (pubkey != NULL)
		pubkey->data = NULL;

	if (flags & GNUTLS_TPM_KEY_SIGNING)
		tpm_flags |= TSS_KEY_TYPE_SIGNING;
	else
		tpm_flags |= TSS_KEY_TYPE_LEGACY;

	if (flags & GNUTLS_TPM_KEY_USER)
		storage_type = TSS_PS_TYPE_USER;
	else
		storage_type = TSS_PS_TYPE_SYSTEM;

	if (bits <= 512)
		tpm_flags |= TSS_KEY_SIZE_512;
	else if (bits <= 1024)
		tpm_flags |= TSS_KEY_SIZE_1024;
	else if (bits <= 2048)
		tpm_flags |= TSS_KEY_SIZE_2048;
	else if (bits <= 4096)
		tpm_flags |= TSS_KEY_SIZE_4096;
	else if (bits <= 8192)
		tpm_flags |= TSS_KEY_SIZE_8192;
	else
		tpm_flags |= TSS_KEY_SIZE_16384;

	ret = tpm_open_session(&s, srk_password, 0);
	if (ret < 0)
		return gnutls_assert_val(ret);

	/* put some randomness into TPM. 
	 * Let's not trust it completely.
	 */
	tssret = pTspi_Context_GetTpmObject(s.tpm_ctx, &htpm);
	if (tssret != 0) {
		gnutls_assert();
		ret = tss_err(tssret);
		goto err_cc;
	}


	ret = gnutls_rnd(GNUTLS_RND_RANDOM, buf, sizeof(buf));
	if (ret < 0) {
		gnutls_assert();
		goto err_cc;
	}

	tssret = pTspi_TPM_StirRandom(htpm, sizeof(buf), buf);
	if (tssret) {
		gnutls_assert();
	}

	tssret =
	    pTspi_Context_CreateObject(s.tpm_ctx, TSS_OBJECT_TYPE_RSAKEY,
				      tpm_flags, &key_ctx);
	if (tssret != 0) {
		gnutls_assert();
		ret = tss_err(tssret);
		goto err_cc;
	}

	tssret =
	    pTspi_SetAttribUint32(key_ctx, TSS_TSPATTRIB_KEY_INFO,
				 TSS_TSPATTRIB_KEYINFO_SIGSCHEME,
				 TSS_SS_RSASSAPKCS1V15_DER);
	if (tssret != 0) {
		gnutls_assert();
		ret = tss_err(tssret);
		goto err_sa;
	}

	/* set the password of the actual key */
	if (key_password) {
		gnutls_datum_t pout;
		char *password = NULL;

		tssret =
		    pTspi_GetPolicyObject(key_ctx, TSS_POLICY_USAGE,
					 &key_policy);
		if (tssret != 0) {
			gnutls_assert();
			ret = tss_err(tssret);
			goto err_sa;
		}

		ret = _gnutls_utf8_password_normalize(key_password, strlen(key_password), &pout, 0);
		if (ret < 0) {
			gnutls_assert();
			goto err_sa;
		}
		password = (char*)pout.data;

		tssret = myTspi_Policy_SetSecret(key_policy,
						 SAFE_LEN(password),
						 (void *)password);
		gnutls_free(password);
		if (tssret != 0) {
			gnutls_assert();
			ret = tss_err(tssret);
			goto err_sa;
		}
	}

	tssret = pTspi_Key_CreateKey(key_ctx, s.srk, 0);
	if (tssret != 0) {
		gnutls_assert();
		ret = tss_err(tssret);
		goto err_sa;
	}

	if (flags & GNUTLS_TPM_REGISTER_KEY) {
		TSS_UUID key_uuid;

		ret = randomize_uuid(&key_uuid);
		if (ret < 0) {
			gnutls_assert();
			goto err_sa;
		}

		tssret =
		    pTspi_Context_RegisterKey(s.tpm_ctx, key_ctx,
					     storage_type, key_uuid,
					     TSS_PS_TYPE_SYSTEM, srk_uuid);
		if (tssret != 0) {
			gnutls_assert();
			ret = tss_err(tssret);
			goto err_sa;
		}

		ret =
		    encode_tpmkey_url((char **) &privkey->data, &key_uuid,
				      storage_type);
		if (ret < 0) {
			TSS_HKEY tkey;

			pTspi_Context_UnregisterKey(s.tpm_ctx, storage_type,
						   key_uuid, &tkey);
			gnutls_assert();
			goto err_sa;
		}
		privkey->size = strlen((char *) privkey->data);

	} else {		/* get the key as blob */


		tssret =
		    pTspi_GetAttribData(key_ctx, TSS_TSPATTRIB_KEY_BLOB,
				       TSS_TSPATTRIB_KEYBLOB_BLOB, &tint,
				       (void *) &tdata);
		if (tssret != 0) {
			gnutls_assert();
			ret = tss_err(tssret);
			goto err_sa;
		}


		if (format == GNUTLS_TPMKEY_FMT_CTK_PEM) {
			ret =
			    _gnutls_x509_encode_string
			    (ASN1_ETYPE_OCTET_STRING, tdata, tint,
			     &tmpkey);
			if (ret < 0) {
				gnutls_assert();
				goto cleanup;
			}

			ret =
			    _gnutls_fbase64_encode("TSS KEY BLOB",
						   tmpkey.data,
						   tmpkey.size, privkey);
			if (ret < 0) {
				gnutls_assert();
				goto cleanup;
			}
		} else {
			UINT32 tint2;

			tmpkey.size = tint + 32;	/* spec says no more than 20 */
			tmpkey.data = gnutls_malloc(tmpkey.size);
			if (tmpkey.data == NULL) {
				gnutls_assert();
				ret = GNUTLS_E_MEMORY_ERROR;
				goto cleanup;
			}

			tint2 = tmpkey.size;
			tssret =
			    pTspi_EncodeDER_TssBlob(tint, tdata,
						   TSS_BLOB_TYPE_PRIVATEKEY,
						   &tint2, tmpkey.data);
			if (tssret != 0) {
				gnutls_assert();
				ret = tss_err(tssret);
				goto cleanup;
			}

			tmpkey.size = tint2;

			privkey->data = tmpkey.data;
			privkey->size = tmpkey.size;
			tmpkey.data = NULL;
		}
	}

	/* read the public key */
	if (pubkey != NULL) {
		size_t psize = 0;

		ret = gnutls_pubkey_init(&pub);
		if (ret < 0) {
			gnutls_assert();
			goto privkey_cleanup;
		}

		ret = read_pubkey(pub, key_ctx, &psize);
		if (ret < 0) {
			gnutls_assert();
			goto privkey_cleanup;
		}
		psize += 512;

		pubkey->data = gnutls_malloc(psize);
		if (pubkey->data == NULL) {
			gnutls_assert();
			ret = GNUTLS_E_MEMORY_ERROR;
			goto pubkey_cleanup;
		}

		ret =
		    gnutls_pubkey_export(pub, pub_format, pubkey->data,
					 &psize);
		if (ret < 0) {
			gnutls_assert();
			goto pubkey_cleanup;
		}
		pubkey->size = psize;

		gnutls_pubkey_deinit(pub);
	}

	ret = 0;
	goto cleanup;

      pubkey_cleanup:
	gnutls_pubkey_deinit(pub);
      privkey_cleanup:
	gnutls_free(privkey->data);
      cleanup:
	gnutls_free(tmpkey.data);
      err_sa:
	pTspi_Context_CloseObject(s.tpm_ctx, key_ctx);
      err_cc:
	tpm_close_session(&s);
	return ret;
}


/**
 * gnutls_tpm_key_list_deinit:
 * @list: a list of the keys
 *
 * This function will deinitialize the list of stored keys in the TPM.
 *
 * Since: 3.1.0
 **/
void gnutls_tpm_key_list_deinit(gnutls_tpm_key_list_t list)
{
	CHECK_INIT_VOID;

	if (list->tpm_ctx != 0)
		pTspi_Context_Close(list->tpm_ctx);
	gnutls_free(list);
}

/**
 * gnutls_tpm_key_list_get_url:
 * @list: a list of the keys
 * @idx: The index of the key (starting from zero)
 * @url: The URL to be returned
 * @flags: should be zero
 *
 * This function will return for each given index a URL of
 * the corresponding key.
 * If the provided index is out of bounds then %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE
 * is returned.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.1.0
 **/
int
gnutls_tpm_key_list_get_url(gnutls_tpm_key_list_t list, unsigned int idx,
			    char **url, unsigned int flags)
{
	CHECK_INIT;

	if (idx >= list->size)
		return
		    gnutls_assert_val
		    (GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE);

	return encode_tpmkey_url(url, &list->ki[idx].keyUUID,
				 list->ki[idx].persistentStorageType);
}

/**
 * gnutls_tpm_get_registered:
 * @list: a list to store the keys
 *
 * This function will get a list of stored keys in the TPM. The uuid
 * of those keys
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.1.0
 **/
int gnutls_tpm_get_registered(gnutls_tpm_key_list_t * list)
{
	TSS_RESULT tssret;
	int ret;

	CHECK_INIT;

	*list = gnutls_calloc(1, sizeof(struct tpm_key_list_st));
	if (*list == NULL)
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

	tssret = pTspi_Context_Create(&(*list)->tpm_ctx);
	if (tssret) {
		gnutls_assert();
		ret = tss_err(tssret);
		goto cleanup;
	}

	tssret = pTspi_Context_Connect((*list)->tpm_ctx, NULL);
	if (tssret) {
		gnutls_assert();
		ret = tss_err(tssret);
		goto cleanup;
	}

	tssret =
	    pTspi_Context_GetRegisteredKeysByUUID2((*list)->tpm_ctx,
						  TSS_PS_TYPE_SYSTEM, NULL,
						  &(*list)->size,
						  &(*list)->ki);
	if (tssret) {
		gnutls_assert();
		ret = tss_err(tssret);
		goto cleanup;
	}
	return 0;

      cleanup:
	gnutls_tpm_key_list_deinit(*list);

	return ret;
}

/**
 * gnutls_tpm_privkey_delete:
 * @url: the URL describing the key
 * @srk_password: a password for the SRK key
 *
 * This function will unregister the private key from the TPM
 * chip. 
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.1.0
 **/
int gnutls_tpm_privkey_delete(const char *url, const char *srk_password)
{
	struct tpm_ctx_st s;
	struct tpmkey_url_st durl;
	TSS_RESULT tssret;
	TSS_HKEY tkey;
	int ret;

	CHECK_INIT;

	ret = decode_tpmkey_url(url, &durl);
	if (ret < 0)
		return gnutls_assert_val(ret);

	if (durl.uuid_set == 0)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	ret = tpm_open_session(&s, srk_password, 1);
	if (ret < 0)
		return gnutls_assert_val(ret);

	tssret =
	    pTspi_Context_UnregisterKey(s.tpm_ctx, durl.storage, durl.uuid,
				       &tkey);
	if (tssret != 0) {
		gnutls_assert();
		ret = tss_err(tssret);
		goto err_cc;
	}

	ret = 0;
      err_cc:
	tpm_close_session(&s);
	return ret;
}
#else				/* HAVE_TROUSERS */
int
gnutls_privkey_import_tpm_raw(gnutls_privkey_t pkey,
			      const gnutls_datum_t * fdata,
			      gnutls_tpmkey_fmt_t format,
			      const char *srk_password,
			      const char *key_password, unsigned int flags)
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}

int
gnutls_privkey_import_tpm_url(gnutls_privkey_t pkey,
			      const char *url,
			      const char *srk_password,
			      const char *key_password, unsigned int flags)
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}

int
gnutls_pubkey_import_tpm_raw(gnutls_pubkey_t pkey,
			     const gnutls_datum_t * fdata,
			     gnutls_tpmkey_fmt_t format,
			     const char *srk_password, unsigned int flags)
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}

int
gnutls_pubkey_import_tpm_url(gnutls_pubkey_t pkey,
			     const char *url,
			     const char *srk_password, unsigned int flags)
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}

int
gnutls_tpm_privkey_generate(gnutls_pk_algorithm_t pk, unsigned int bits,
			    const char *srk_password,
			    const char *key_password,
			    gnutls_tpmkey_fmt_t format,
			    gnutls_x509_crt_fmt_t pub_format,
			    gnutls_datum_t * privkey,
			    gnutls_datum_t * pubkey, unsigned int flags)
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}

void gnutls_tpm_key_list_deinit(gnutls_tpm_key_list_t list)
{
	return;
}

int
gnutls_tpm_key_list_get_url(gnutls_tpm_key_list_t list, unsigned int idx,
			    char **url, unsigned int flags)
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}

int gnutls_tpm_get_registered(gnutls_tpm_key_list_t * list)
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}

int gnutls_tpm_privkey_delete(const char *url, const char *srk_password)
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}
#endif				/* HAVE_TROUSERS */
