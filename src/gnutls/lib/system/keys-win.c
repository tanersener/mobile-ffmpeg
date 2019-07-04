/*
 * Copyright © 2014-2016 Red Hat, Inc.
 * Copyright © 2015-2016 Dyalog Ltd.
 *
 * Author: Nikos Mavrogiannopoulos, Bjørn Christensen
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

// Before including any Windows header we need to set _WIN32_WINNT to Vista
// (or higher) so that the NCRYPT stuff can be used.
#if _WIN32_WINNT < 0x600
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x600
#endif

#include "gnutls_int.h"
#include "errors.h"
#include <gnutls/gnutls.h>
#include <gnutls/abstract.h>
#include <gnutls/pkcs12.h>
#include <gnutls/system-keys.h>
#include "system-keys.h"
#include <tls-sig.h>
#include <pk.h>
#include <urls.h>

#if !defined(_WIN32)
#error should not be included
#endif

#include <wincrypt.h>
#include <winbase.h>
#include <winapifamily.h>

#define DYN_NCRYPT

#ifdef __MINGW32__
# include <_mingw.h>
# ifdef __MINGW64_VERSION_MAJOR
/* MinGW64 */
#  include <ncrypt.h>
# else
/* mingw.org's MinGW */
# include <security.h>
# define BCRYPT_PAD_PKCS1 0x00000002
# define BCRYPT_RSA_ALGORITHM L"RSA"
# define BCRYPT_DSA_ALGORITHM L"DSA"
# define BCRYPT_SHA1_ALGORITHM L"SHA1"
# define BCRYPT_SHA256_ALGORITHM L"SHA256"
# define BCRYPT_SHA384_ALGORITHM L"SHA384"
# define BCRYPT_SHA512_ALGORITHM L"SHA512"
# define BCRYPT_ECDSA_P256_ALGORITHM L"ECDSA_P256"
# define BCRYPT_ECDSA_P384_ALGORITHM L"ECDSA_P384"
# define BCRYPT_ECDSA_P521_ALGORITHM L"ECDSA_P521"
typedef ULONG_PTR NCRYPT_HANDLE;
typedef ULONG_PTR NCRYPT_PROV_HANDLE;
typedef ULONG_PTR NCRYPT_KEY_HANDLE;
typedef struct _BCRYPT_PKCS1_PADDING_INFO {
	LPCWSTR pszAlgId;
} BCRYPT_PKCS1_PADDING_INFO;
# endif
#else /* non-mingw */
# include <ncrypt.h>
#endif

// MinGW headers may not have these defines
#ifndef NCRYPT_SHA1_ALGORITHM
#define NCRYPT_SHA1_ALGORITHM	   BCRYPT_SHA1_ALGORITHM
#endif
#ifndef NCRYPT_SHA256_ALGORITHM
#define NCRYPT_SHA256_ALGORITHM	 BCRYPT_SHA256_ALGORITHM
#endif
#ifndef NCRYPT_SHA384_ALGORITHM
#define NCRYPT_SHA384_ALGORITHM	 BCRYPT_SHA384_ALGORITHM
#endif
#ifndef NCRYPT_SHA512_ALGORITHM
#define NCRYPT_SHA512_ALGORITHM	 BCRYPT_SHA512_ALGORITHM
#endif
#ifndef NCRYPT_PAD_PKCS1_FLAG
#define NCRYPT_PAD_PKCS1_FLAG 2
#endif
#ifndef NCRYPT_ALGORITHM_PROPERTY
#define NCRYPT_ALGORITHM_PROPERTY	       L"Algorithm Name"
#endif
#ifndef CERT_NCRYPT_KEY_HANDLE_TRANSFER_PROP_ID
#define CERT_NCRYPT_KEY_HANDLE_TRANSFER_PROP_ID 99
#endif

#define MAX_WID_SIZE 48

static
void *memrev(unsigned char *pvData, DWORD cbData);

struct system_key_iter_st {
	HCERTSTORE store;
	const CERT_CONTEXT *cert;
};

typedef struct priv_st {
	DWORD dwKeySpec;	/* CAPI key */
	HCRYPTPROV hCryptProv;	/* CAPI keystore */
	NCRYPT_KEY_HANDLE nc;	/* CNG Keystore */
	gnutls_pk_algorithm_t pk;
	gnutls_sign_algorithm_t sign_algo;
} priv_st;

typedef SECURITY_STATUS(WINAPI * NCryptDeleteKeyFunc) (NCRYPT_KEY_HANDLE hKey,
						       DWORD dwFlags);

typedef SECURITY_STATUS(WINAPI *
			NCryptOpenStorageProviderFunc) (NCRYPT_PROV_HANDLE *
							phProvider,
							LPCWSTR pszProviderName,
							DWORD dwFlags);

typedef SECURITY_STATUS(WINAPI *
			NCryptOpenKeyFunc) (NCRYPT_PROV_HANDLE hProvider,
					    NCRYPT_KEY_HANDLE * phKey,
					    LPCWSTR pszKeyName,
					    DWORD dwLegacyKeySpec,
					    DWORD dwFlags);

typedef SECURITY_STATUS(WINAPI * NCryptGetPropertyFunc) (NCRYPT_HANDLE hObject,
							 LPCWSTR pszProperty,
							 PBYTE pbOutput,
							 DWORD cbOutput,
							 DWORD * pcbResult,
							 DWORD dwFlags);

typedef SECURITY_STATUS(WINAPI * NCryptFreeObjectFunc) (NCRYPT_HANDLE hObject);

typedef SECURITY_STATUS(WINAPI * NCryptDecryptFunc) (NCRYPT_KEY_HANDLE hKey,
						     PBYTE pbInput,
						     DWORD cbInput,
						     VOID * pPaddingInfo,
						     PBYTE pbOutput,
						     DWORD cbOutput,
						     DWORD * pcbResult,
						     DWORD dwFlags);

typedef SECURITY_STATUS(WINAPI * NCryptSignHashFunc) (NCRYPT_KEY_HANDLE hKey,
						      VOID * pPaddingInfo,
						      PBYTE pbHashValue,
						      DWORD cbHashValue,
						      PBYTE pbSignature,
						      DWORD cbSignature,
						      DWORD * pcbResult,
						      DWORD dwFlags);

static int StrCmpW(const WCHAR * str1, const WCHAR * str2)
{
	while (*str1 && (*str1 == *str2)) {
		str1++;
		str2++;
	}
	return *str1 - *str2;
}

#ifdef DYN_NCRYPT
static NCryptDeleteKeyFunc pNCryptDeleteKey;
static NCryptOpenStorageProviderFunc pNCryptOpenStorageProvider;
static NCryptOpenKeyFunc pNCryptOpenKey;
static NCryptGetPropertyFunc pNCryptGetProperty;
static NCryptFreeObjectFunc pNCryptFreeObject;
static NCryptDecryptFunc pNCryptDecrypt;
static NCryptSignHashFunc pNCryptSignHash;
#else
#define pNCryptDeleteKey NCryptDeleteKey
#define pNCryptOpenStorageProvider NCryptOpenStorageProvider
#define pNCryptOpenKey NCryptOpenKey
#define pNCryptGetProperty NCryptGetProperty
#define pNCryptFreeObject NCryptFreeObject
#define pNCryptDecrypt NCryptDecrypt
#define pNCryptSignHash NCryptSignHash
#endif

static unsigned ncrypt_init = 0;
static HMODULE ncrypt_lib;

#define WIN_URL SYSTEM_URL"win:"
#define WIN_URL_SIZE 11

static int
get_id(const char *url, uint8_t * bin, size_t * bin_size, unsigned cert)
{
	int ret;
	unsigned url_size = strlen(url);
	const char *p = url, *p2;
	gnutls_datum_t tmp;

	if (cert != 0) {
		if (url_size < sizeof(WIN_URL)
		    || strncmp(url, WIN_URL, WIN_URL_SIZE) != 0)
			return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);
	} else {
		if (url_size < sizeof(WIN_URL)
		    || strncmp(url, WIN_URL, WIN_URL_SIZE) != 0)
			return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);
	}

	p += sizeof(WIN_URL) - 1;

	p = strstr(p, "id=");
	if (p == NULL)
		return gnutls_assert_val(GNUTLS_E_PARSING_ERROR);
	p += 3;

	p2 = strchr(p, ';');
	if (p2 == NULL) {
		url_size = strlen(p);
	} else {
		url_size = (p2 - p);
	}

	tmp.data = p;
	tmp.size = url_size;
	ret = gnutls_hex_decode(&tmp, bin, bin_size);
	if (ret < 0)
		return ret;

	return 0;
}

static
void *memrev(unsigned char *pvData, DWORD cbData)
{
	char t;
	DWORD i;

	for (i = 0; i < cbData / 2; i++) {
		t = pvData[i];
		pvData[i] = pvData[cbData - 1 - i];
		pvData[cbData - 1 - i] = t;
	}
	return pvData;
}

static
int capi_sign(gnutls_privkey_t key, void *userdata,
	      const gnutls_datum_t * raw_data, gnutls_datum_t * signature)
{
	priv_st *priv = (priv_st *) userdata;
	ALG_ID Algid;
	HCRYPTHASH hHash = NULL;
	uint8_t digest[MAX_HASH_SIZE];
	unsigned int digest_size;
	gnutls_digest_algorithm_t algo;
	DWORD size1 = 0, sizesize = sizeof(DWORD);
	DWORD ret_sig = 0;
	int ret;

	signature->data = NULL;
	signature->size = 0;

	digest_size = raw_data->size;

	switch (digest_size) {
	case 16:
		Algid = CALG_MD5;
		break;
		//case 35:  size=20;                                    // DigestInfo SHA1
	case 20:
		Algid = CALG_SHA1;
		break;
		//case 51:  size=32;                                    // DigestInto SHA-256
	case 32:
		Algid = CALG_SHA_256;
		break;
	case 36:
		Algid = CALG_SSL3_SHAMD5;
		break;
	case 48:
		Algid = CALG_SHA_384;
		break;
	case 64:
		Algid = CALG_SHA_512;
		break;
	default:
		digest_size = sizeof(digest);
		ret =
		    decode_ber_digest_info(raw_data, &algo, digest,
					   &digest_size);
		if (ret < 0)
			return gnutls_assert_val(ret);

		switch (algo) {
		case GNUTLS_DIG_SHA1:
			Algid = CALG_SHA1;
			break;
#ifdef NCRYPT_SHA224_ALGORITHM
		case GNUTLS_DIG_SHA224:
			Algid = CALG_SHA_224;
			break;
#endif
		case GNUTLS_DIG_SHA256:
			Algid = CALG_SHA_256;
			break;
		case GNUTLS_DIG_SHA384:
			Algid = CALG_SHA_384;
			break;
		case GNUTLS_DIG_SHA512:
			Algid = CALG_SHA_512;
			break;
		default:
			return
			    gnutls_assert_val(GNUTLS_E_UNKNOWN_HASH_ALGORITHM);
		}
	}

	if (!CryptCreateHash(priv->hCryptProv, Algid, 0, 0, &hHash)) {
		gnutls_assert();
		_gnutls_debug_log("error in create hash: %d\n",
				  (int)GetLastError());
		ret = GNUTLS_E_PK_SIGN_FAILED;
		goto fail;
	}

	if (!CryptSetHashParam(hHash, HP_HASHVAL, digest, 0)) {
		gnutls_assert();
		_gnutls_debug_log("error in set hash val: %d\n",
				  (int)GetLastError());
		ret = GNUTLS_E_PK_SIGN_FAILED;
		goto fail;
	}

	if (!CryptGetHashParam
	    (hHash, HP_HASHSIZE, (BYTE *) & size1, &sizesize, 0)
	    || digest_size != size1) {
		gnutls_assert();
		_gnutls_debug_log("error in hash size: %d\n", (int)size1);
		ret = GNUTLS_E_PK_SIGN_FAILED;
		goto fail;
	}

	if (!CryptSignHash(hHash, priv->dwKeySpec, NULL, 0, NULL, &ret_sig)) {
		gnutls_assert();
		_gnutls_debug_log("error in pre-signing: %d\n",
				  (int)GetLastError());
		ret = GNUTLS_E_PK_SIGN_FAILED;
		goto fail;
	}

	signature->size = ret_sig;
	signature->data = (unsigned char *)gnutls_malloc(signature->size);

	if (signature->data == NULL)
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

	if (!CryptSignHash
	    (hHash, priv->dwKeySpec, NULL, 0, signature->data, &ret_sig)) {
		gnutls_assert();
		_gnutls_debug_log("error in signing: %d\n",
				  (int)GetLastError());
		ret = GNUTLS_E_PK_SIGN_FAILED;
		goto fail;
	}

	memrev(signature->data, signature->size);

	CryptDestroyHash(hHash);
	signature->size = ret_sig;

	return 0;
 fail:
	if (hHash != 0)
		CryptDestroyHash(hHash);
	gnutls_free(signature->data);
	return ret;
}

static
int capi_decrypt(gnutls_privkey_t key, void *userdata,
		 const gnutls_datum_t * ciphertext, gnutls_datum_t * plaintext)
{
	priv_st *priv = (priv_st *) userdata;
	DWORD size = 0;
	int ret;

	plaintext->data = NULL;
	plaintext->size = 0;

	if (priv->pk != GNUTLS_PK_RSA) {
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);
	}

	plaintext->size = size = ciphertext->size;
	plaintext->data = (unsigned char *)gnutls_malloc(plaintext->size);
	if (plaintext->data == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	memcpy(plaintext->data, ciphertext->data, size);
	if (0 ==
	    CryptDecrypt(priv->hCryptProv, 0, true, 0, plaintext->data,
			 &size)) {
		gnutls_assert();
		ret = GNUTLS_E_PK_DECRYPTION_FAILED;
		goto fail;
	}

	return 0;
 fail:
	gnutls_free(plaintext->data);
	return ret;
}

static
void capi_deinit(gnutls_privkey_t key, void *userdata)
{
	priv_st *priv = (priv_st *) userdata;
	CryptReleaseContext(priv->hCryptProv, 0);
	gnutls_free(priv);
}

static int capi_info(gnutls_privkey_t key, unsigned int flags, void *userdata)
{
	priv_st *priv = (priv_st *) userdata;

	if (flags & GNUTLS_PRIVKEY_INFO_PK_ALGO)
		return priv->pk;
	if (flags & GNUTLS_PRIVKEY_INFO_SIGN_ALGO)
		return priv->sign_algo;
	return -1;
}

static
int cng_sign(gnutls_privkey_t key, void *userdata,
	     const gnutls_datum_t * raw_data, gnutls_datum_t * signature)
{
	priv_st *priv = userdata;
	BCRYPT_PKCS1_PADDING_INFO _info;
	void *info = NULL;
	DWORD ret_sig = 0;
	int ret;
	DWORD flags = 0;
	gnutls_datum_t data = { raw_data->data, raw_data->size };
	uint8_t digest[MAX_HASH_SIZE];
	unsigned int digest_size;
	gnutls_digest_algorithm_t algo;
	SECURITY_STATUS r;

	signature->data = NULL;
	signature->size = 0;

	if (priv->pk == GNUTLS_PK_RSA) {

		flags = BCRYPT_PAD_PKCS1;
		info = &_info;

		if (raw_data->size == 36) {	/* TLS 1.0 MD5+SHA1 */
			_info.pszAlgId = NULL;
		} else {
			digest_size = sizeof(digest);
			ret =
			    decode_ber_digest_info(raw_data, &algo, digest,
						   &digest_size);
			if (ret < 0)
				return gnutls_assert_val(ret);

			switch (algo) {
			case GNUTLS_DIG_SHA1:
				_info.pszAlgId = NCRYPT_SHA1_ALGORITHM;
				break;
#ifdef NCRYPT_SHA224_ALGORITHM
			case GNUTLS_DIG_SHA224:
				_info.pszAlgId = NCRYPT_SHA224_ALGORITHM;
				break;
#endif
			case GNUTLS_DIG_SHA256:
				_info.pszAlgId = NCRYPT_SHA256_ALGORITHM;
				break;
			case GNUTLS_DIG_SHA384:
				_info.pszAlgId = NCRYPT_SHA384_ALGORITHM;
				break;
			case GNUTLS_DIG_SHA512:
				_info.pszAlgId = NCRYPT_SHA512_ALGORITHM;
				break;
			default:
				return
				    gnutls_assert_val
				    (GNUTLS_E_UNKNOWN_HASH_ALGORITHM);
			}
			data.data = digest;
			data.size = digest_size;
		}
	}

	r = pNCryptSignHash(priv->nc, info, data.data, data.size,
			    NULL, 0, &ret_sig, flags);
	if (FAILED(r)) {
		gnutls_assert();
		_gnutls_debug_log("error in pre-signing: %d\n",
				  (int)GetLastError());
		ret = GNUTLS_E_PK_SIGN_FAILED;
		goto fail;
	}

	signature->size = ret_sig;
	signature->data = gnutls_malloc(signature->size);
	if (signature->data == NULL)
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

	r = pNCryptSignHash(priv->nc, info, data.data, data.size,
			    signature->data, signature->size, &ret_sig, flags);
	if (FAILED(r)) {
		gnutls_assert();
		_gnutls_debug_log("error in signing: %d\n",
				  (int)GetLastError());
		ret = GNUTLS_E_PK_SIGN_FAILED;
		goto fail;
	}

	signature->size = ret_sig;

	return 0;
 fail:
	gnutls_free(signature->data);
	return ret;
}

static
int cng_decrypt(gnutls_privkey_t key, void *userdata,
		const gnutls_datum_t * ciphertext, gnutls_datum_t * plaintext)
{
	priv_st *priv = userdata;
	SECURITY_STATUS r;
	DWORD ret_dec = 0;
	int ret;

	plaintext->data = NULL;
	plaintext->size = 0;

	if (priv->pk != GNUTLS_PK_RSA) {
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);
	}

	r = pNCryptDecrypt(priv->nc, ciphertext->data, ciphertext->size,
			   NULL, NULL, 0, &ret_dec, NCRYPT_PAD_PKCS1_FLAG);
	if (FAILED(r)) {
		gnutls_assert();
		return GNUTLS_E_PK_DECRYPTION_FAILED;
	}

	plaintext->size = ret_dec;
	plaintext->data = gnutls_malloc(plaintext->size);
	if (plaintext->data == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	r = pNCryptDecrypt(priv->nc, ciphertext->data, ciphertext->size,
			   NULL, plaintext->data, plaintext->size,
			   &ret_dec, NCRYPT_PAD_PKCS1_FLAG);
	if (FAILED(r)) {
		gnutls_assert();
		ret = GNUTLS_E_PK_DECRYPTION_FAILED;
		goto fail;
	}
	plaintext->size = ret_dec;

	return 0;
 fail:
	gnutls_free(plaintext->data);
	return ret;
}

static
void cng_deinit(gnutls_privkey_t key, void *userdata)
{
	priv_st *priv = userdata;
	pNCryptFreeObject(priv->nc);
	gnutls_free(priv);
}

static int cng_info(gnutls_privkey_t key, unsigned int flags, void *userdata)
{
	priv_st *priv = userdata;

	if (flags & GNUTLS_PRIVKEY_INFO_PK_ALGO)
		return priv->pk;
	if (flags & GNUTLS_PRIVKEY_INFO_SIGN_ALGO)
		return priv->sign_algo;
	return -1;
}

/*-
 * _gnutls_privkey_import_system:
 * @pkey: The private key
 * @url: The URL of the key
 *
 * This function will import the given private key to the abstract
 * #gnutls_privkey_t type. 
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.4.0
 *
 -*/
int _gnutls_privkey_import_system_url(gnutls_privkey_t pkey, const char *url)
{
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP)
    return gnutls_assert_val(GNUTLS_E_UNIMPLEMENTED_FEATURE);
#else
	uint8_t id[MAX_WID_SIZE];
	HCERTSTORE store = NULL;
	size_t id_size;
	const CERT_CONTEXT *cert = NULL;
	CRYPT_HASH_BLOB blob;
	CRYPT_KEY_PROV_INFO *kpi = NULL;
	NCRYPT_KEY_HANDLE nc = NULL;
	HCRYPTPROV hCryptProv = NULL;
	NCRYPT_PROV_HANDLE sctx = NULL;
	DWORD kpi_size;
	SECURITY_STATUS r;
	int ret, enc_too = 0;
	WCHAR algo_str[64];
	DWORD algo_str_size = 0;
	priv_st *priv;
	DWORD i, dwErrCode = 0;

	if (ncrypt_init == 0)
		return gnutls_assert_val(GNUTLS_E_UNIMPLEMENTED_FEATURE);

	if (url == NULL)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	priv = gnutls_calloc(1, sizeof(*priv));
	if (priv == NULL)
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

	id_size = sizeof(id);
	ret = get_id(url, id, &id_size, 0);
	if (ret < 0)
		return gnutls_assert_val(ret);

	blob.cbData = id_size;
	blob.pbData = id;

	store = CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, 0, CERT_SYSTEM_STORE_CURRENT_USER, L"MY");
	if (store == NULL) {
		gnutls_assert();
		ret = GNUTLS_E_FILE_ERROR;
		goto cleanup;
	}

	cert = CertFindCertificateInStore(store,
					  X509_ASN_ENCODING,
					  0,
					  CERT_FIND_KEY_IDENTIFIER,
					  &blob, NULL);

	if (cert == NULL) {
		char buf[64];
		_gnutls_debug_log("cannot find ID: %s from %s\n",
				  _gnutls_bin2hex(id, id_size,
						  buf, sizeof(buf), NULL), url);
		ret = gnutls_assert_val(GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE);
		goto cleanup;
	}

	kpi_size = 0;
	r = CertGetCertificateContextProperty(cert, CERT_KEY_PROV_INFO_PROP_ID,
					      NULL, &kpi_size);
	if (r == 0) {
		_gnutls_debug_log("error in getting context: %d from %s\n",
				  (int)GetLastError(), url);
		ret = gnutls_assert_val(GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE);
		goto cleanup;
	}

	kpi = gnutls_malloc(kpi_size);
	if (kpi == NULL) {
		gnutls_assert();
		ret = GNUTLS_E_MEMORY_ERROR;
		goto cleanup;
	}

	r = CertGetCertificateContextProperty(cert, CERT_KEY_PROV_INFO_PROP_ID,
					      kpi, &kpi_size);
	if (r == 0) {
		_gnutls_debug_log("error in getting context: %d from %s\n",
				  (int)GetLastError(), url);
		ret = gnutls_assert_val(GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE);
		goto cleanup;
	}

	r = pNCryptOpenStorageProvider(&sctx, kpi->pwszProvName, 0);
	if (!FAILED(r)) {	/* if this works carry on with CNG */

		r = pNCryptOpenKey(sctx, &nc, kpi->pwszContainerName, 0, 0);
		if (FAILED(r)) {
			ret =
			    gnutls_assert_val
			    (GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE);
			goto cleanup;
		}

		r = pNCryptGetProperty(nc, NCRYPT_ALGORITHM_PROPERTY,
				       (BYTE *) algo_str, sizeof(algo_str),
				       &algo_str_size, 0);
		if (FAILED(r)) {
			ret =
			    gnutls_assert_val
			    (GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE);
			goto cleanup;
		}

		if (StrCmpW(algo_str, BCRYPT_RSA_ALGORITHM) == 0) {
			priv->pk = GNUTLS_PK_RSA;
			priv->sign_algo = GNUTLS_SIGN_RSA_SHA256;
			enc_too = 1;
		} else if (StrCmpW(algo_str, BCRYPT_DSA_ALGORITHM) == 0) {
			priv->pk = GNUTLS_PK_DSA;
			priv->sign_algo = GNUTLS_SIGN_DSA_SHA1;
		} else if (StrCmpW(algo_str, BCRYPT_ECDSA_P256_ALGORITHM) == 0) {
			priv->pk = GNUTLS_PK_EC;
			priv->sign_algo = GNUTLS_SIGN_ECDSA_SHA256;
		} else if (StrCmpW(algo_str, BCRYPT_ECDSA_P384_ALGORITHM) == 0) {
			priv->pk = GNUTLS_PK_EC;
			priv->sign_algo = GNUTLS_SIGN_ECDSA_SHA384;
		} else if (StrCmpW(algo_str, BCRYPT_ECDSA_P521_ALGORITHM) == 0) {
			priv->pk = GNUTLS_PK_EC;
			priv->sign_algo = GNUTLS_SIGN_ECDSA_SHA512;
		} else {
			_gnutls_debug_log("unknown key algorithm: %ls\n",
					  algo_str);
			ret = gnutls_assert_val(GNUTLS_E_UNKNOWN_PK_ALGORITHM);
			goto cleanup;
		}
		priv->nc = nc;

		ret = gnutls_privkey_import_ext3(pkey, priv, cng_sign,
						 (enc_too !=
						  0) ? cng_decrypt : NULL,
						 cng_deinit, cng_info, 0);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}
	} else {
		/* this should be CAPI */
		_gnutls_debug_log
		    ("error in opening CNG keystore: %x from %ls\n", (int)r,
		     kpi->pwszProvName);

		if (CryptAcquireContextW(&hCryptProv,
					 kpi->pwszContainerName,
					 kpi->pwszProvName,
					 kpi->dwProvType, kpi->dwFlags)) {
			for (i = 0; i < kpi->cProvParam; i++)
				if (!CryptSetProvParam(hCryptProv,
						       kpi->rgProvParam[i].
						       dwParam,
						       kpi->rgProvParam[i].
						       pbData,
						       kpi->rgProvParam[i].
						       dwFlags)) {
					dwErrCode = GetLastError();
					break;
				};
		} else {
			dwErrCode = GetLastError();
		}

		if (ERROR_SUCCESS != dwErrCode) {
			_gnutls_debug_log
			    ("error in getting cryptprov: %d from %s\n",
			     (int)GetLastError(), url);
			ret =
			    gnutls_assert_val
			    (GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE);
			goto cleanup;
		}

		{
			BYTE buf[100 + sizeof(PROV_ENUMALGS_EX) * 2];
			PROV_ENUMALGS_EX *pAlgo = (PROV_ENUMALGS_EX *) buf;
			DWORD len = sizeof(buf);

			if (CryptGetProvParam
			    (hCryptProv, PP_ENUMALGS_EX, buf, &len,
			     CRYPT_FIRST)) {
				DWORD hash = 0;
				do {
					switch (pAlgo->aiAlgid) {
					case CALG_RSA_SIGN:
						priv->pk = GNUTLS_PK_RSA;
						enc_too = 1;
						break;
					case CALG_DSS_SIGN:
						priv->pk =
						    priv->pk ==
						    GNUTLS_PK_RSA ?
						    GNUTLS_PK_RSA :
						    GNUTLS_PK_DSA;
						break;
					case CALG_SHA1:
						hash = 1;
						break;
					case CALG_SHA_256:
						hash = 256;
						break;
					default:
						break;
					}

					len = sizeof(buf);	// reset the buffer size
				} while (CryptGetProvParam
					 (hCryptProv, PP_ENUMALGS_EX, buf, &len,
					  CRYPT_NEXT));

				if (priv->pk == GNUTLS_PK_DSA)
					priv->sign_algo = GNUTLS_SIGN_DSA_SHA1;
				else
					priv->sign_algo =
					    (hash >
					     1) ? GNUTLS_SIGN_RSA_SHA256 :
					    GNUTLS_SIGN_RSA_SHA1;
			}
		}

		priv->hCryptProv = hCryptProv;
		priv->dwKeySpec = kpi->dwKeySpec;

		ret = gnutls_privkey_import_ext3(pkey, priv, capi_sign,
						 (enc_too !=
						  0) ? capi_decrypt : NULL,
						 capi_deinit, capi_info, 0);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}
	}
	ret = 0;
 cleanup:
	if (ret < 0) {
		if (nc != 0)
			pNCryptFreeObject(nc);
		if (hCryptProv != 0)
			CryptReleaseContext(hCryptProv, 0);
		gnutls_free(priv);
	}
	if (sctx != 0)
		pNCryptFreeObject(sctx);

	gnutls_free(kpi);

	if (cert != 0)
		CertFreeCertificateContext(cert);

	CertCloseStore(store, 0);
	return ret;
#endif
}

int _gnutls_x509_crt_import_system_url(gnutls_x509_crt_t crt, const char *url)
{
	uint8_t id[MAX_WID_SIZE];
	HCERTSTORE store = NULL;
	size_t id_size;
	const CERT_CONTEXT *cert = NULL;
	CRYPT_HASH_BLOB blob;
	int ret;
	gnutls_datum_t data;

	if (ncrypt_init == 0)
		return gnutls_assert_val(GNUTLS_E_UNIMPLEMENTED_FEATURE);

	id_size = sizeof(id);
	ret = get_id(url, id, &id_size, 0);
	if (ret < 0)
		return gnutls_assert_val(ret);

	blob.cbData = id_size;
	blob.pbData = id;

	store = CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, 0, CERT_SYSTEM_STORE_CURRENT_USER, L"MY");
	if (store == NULL) {
		gnutls_assert();
		ret = GNUTLS_E_FILE_ERROR;
		goto cleanup;
	}

	cert = CertFindCertificateInStore(store,
					  X509_ASN_ENCODING,
					  0,
					  CERT_FIND_KEY_IDENTIFIER,
					  &blob, NULL);

	if (cert == NULL) {
		char buf[64];
		_gnutls_debug_log("cannot find ID: %s from %s\n",
				  _gnutls_bin2hex(id, id_size,
						  buf, sizeof(buf), NULL), url);
		ret = gnutls_assert_val(GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE);
		goto cleanup;
	}

	data.data = cert->pbCertEncoded;
	data.size = cert->cbCertEncoded;

	ret = gnutls_x509_crt_import(crt, &data, GNUTLS_X509_FMT_DER);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = 0;
 cleanup:
	if (cert != 0)
		CertFreeCertificateContext(cert);

	CertCloseStore(store, 0);
	return ret;
}

/**
 * gnutls_system_key_iter_deinit:
 * @iter: an iterator of system keys
 *
 * This function will deinitialize the iterator.
 *
 * Since: 3.4.0
 **/
void gnutls_system_key_iter_deinit(gnutls_system_key_iter_t iter)
{
	if (ncrypt_init == 0)
		return;

	CertCloseStore(iter->store, 0);
	gnutls_free(iter);
}

static
int get_win_urls(const CERT_CONTEXT * cert, char **cert_url, char **key_url,
		 char **label, gnutls_datum_t * der)
{
	BOOL r;
	int ret;
	DWORD tl_size;
	gnutls_datum_t tmp_label = { NULL, 0 };
	char name[MAX_CN * 2];
	char hex[MAX_WID_SIZE * 2 + 1];
	gnutls_buffer_st str;
#ifdef WORDS_BIGENDIAN
	const unsigned bigendian = 1;
#else
	const unsigned bigendian = 0;
#endif

	if (cert == NULL)
		return gnutls_assert_val(GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE);

	if (der) {
		der->data = gnutls_malloc(cert->cbCertEncoded);
		if (der->data == NULL)
			return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

		memcpy(der->data, cert->pbCertEncoded, cert->cbCertEncoded);
		der->size = cert->cbCertEncoded;
	}

	_gnutls_buffer_init(&str);
	if (label)
		*label = NULL;
	if (key_url)
		*key_url = NULL;
	if (cert_url)
		*cert_url = NULL;

	tl_size = sizeof(name);
	r = CertGetCertificateContextProperty(cert, CERT_FRIENDLY_NAME_PROP_ID,
					      name, &tl_size);
	if (r != 0) {		/* optional */
		ret =
		    _gnutls_ucs2_to_utf8(name, tl_size, &tmp_label, bigendian);
		if (ret < 0) {
			gnutls_assert();
			goto fail;
		}
		if (label)
			*label = (char *)tmp_label.data;
	}

	tl_size = sizeof(name);
	r = CertGetCertificateContextProperty(cert, CERT_KEY_IDENTIFIER_PROP_ID,
					      name, &tl_size);
	if (r == 0) {
		gnutls_assert();
		ret = GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
		goto fail;
	}

	if (_gnutls_bin2hex(name, tl_size, hex, sizeof(hex), 0) == NULL) {
		ret = gnutls_assert_val(GNUTLS_E_PARSING_ERROR);
		goto fail;
	}

	ret =
	    _gnutls_buffer_append_printf(&str, WIN_URL "id=%s;type=cert", hex);
	if (ret < 0) {
		gnutls_assert();
		goto fail;
	}

	if (tmp_label.data) {
		ret = _gnutls_buffer_append_str(&str, ";name=");
		if (ret < 0) {
			gnutls_assert();
			goto fail;
		}

		ret =
		    _gnutls_buffer_append_escape(&str, tmp_label.data,
						 tmp_label.size, " ");
		if (ret < 0) {
			gnutls_assert();
			goto fail;
		}
	}

	ret = _gnutls_buffer_append_data(&str, "\x00", 1);
	if (ret < 0) {
		gnutls_assert();
		goto fail;
	}

	if (cert_url)
		*cert_url = (char *)str.data;
	_gnutls_buffer_init(&str);

	ret =
	    _gnutls_buffer_append_printf(&str, WIN_URL "id=%s;type=privkey",
					 hex);
	if (ret < 0) {
		gnutls_assert();
		goto fail;
	}

	if (tmp_label.data) {
		ret = _gnutls_buffer_append_str(&str, ";name=");
		if (ret < 0) {
			gnutls_assert();
			goto fail;
		}

		ret =
		    _gnutls_buffer_append_escape(&str, tmp_label.data,
						 tmp_label.size, " ");
		if (ret < 0) {
			gnutls_assert();
			goto fail;
		}
	}

	ret = _gnutls_buffer_append_data(&str, "\x00", 1);
	if (ret < 0) {
		gnutls_assert();
		goto fail;
	}

	if (key_url)
		*key_url = (char *)str.data;
	_gnutls_buffer_init(&str);

	ret = 0;
	goto cleanup;

 fail:
	if (der)
		gnutls_free(der->data);
	if (cert_url)
		gnutls_free(*cert_url);
	if (key_url)
		gnutls_free(*key_url);
	if (label)
		gnutls_free(*label);
 cleanup:
	_gnutls_buffer_clear(&str);
	return ret;
}

/**
 * gnutls_system_key_iter_get_info:
 * @iter: an iterator of the system keys (must be set to %NULL initially)
 * @cert_type: A value of gnutls_certificate_type_t which indicates the type of certificate to look for
 * @cert_url: The certificate URL of the pair (may be %NULL)
 * @key_url: The key URL of the pair (may be %NULL)
 * @label: The friendly name (if any) of the pair (may be %NULL)
 * @der: if non-NULL the DER data of the certificate
 * @flags: should be zero
 *
 * This function will return on each call a certificate
 * and key pair URLs, as well as a label associated with them,
 * and the DER-encoded certificate. When the iteration is complete it will
 * return %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE.
 *
 * Typically @cert_type should be %GNUTLS_CRT_X509.
 *
 * All values set are allocated and must be cleared using gnutls_free(),
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.4.0
 **/
int
gnutls_system_key_iter_get_info(gnutls_system_key_iter_t * iter,
				unsigned cert_type,
				char **cert_url,
				char **key_url,
				char **label,
				gnutls_datum_t * der, unsigned int flags)
{
	if (ncrypt_init == 0)
		return gnutls_assert_val(GNUTLS_E_UNIMPLEMENTED_FEATURE);
	if (cert_type != GNUTLS_CRT_X509)
		return gnutls_assert_val(GNUTLS_E_UNIMPLEMENTED_FEATURE);

	if (*iter == NULL) {
		*iter = gnutls_calloc(1, sizeof(struct system_key_iter_st));
		if (*iter == NULL)
			return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

		(*iter)->store = CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, 0, CERT_SYSTEM_STORE_CURRENT_USER, L"MY");
		if ((*iter)->store == NULL) {
			gnutls_free(*iter);
			*iter = NULL;
			return
			    gnutls_assert_val
			    (GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE);
		}

		(*iter)->cert =
		    CertEnumCertificatesInStore((*iter)->store, NULL);

		return get_win_urls((*iter)->cert, cert_url, key_url, label,
				    der);
	} else {
		if ((*iter)->cert == NULL)
			return
			    gnutls_assert_val
			    (GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE);

		(*iter)->cert =
		    CertEnumCertificatesInStore((*iter)->store, (*iter)->cert);
		return get_win_urls((*iter)->cert, cert_url, key_url, label,
				    der);

	}
}

/**
 * gnutls_system_key_delete:
 * @cert_url: the URL of the certificate
 * @key_url: the URL of the key
 *
 * This function will delete the key and certificate pair.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.4.0
 **/
int gnutls_system_key_delete(const char *cert_url, const char *key_url)
{
	uint8_t id[MAX_WID_SIZE];
	HCERTSTORE store = NULL;
	size_t id_size;
	const CERT_CONTEXT *cert = NULL;
	CRYPT_HASH_BLOB blob;
	NCRYPT_KEY_HANDLE nc;
	DWORD nc_size;
	BOOL r;
	int ret;

	if (ncrypt_init == 0)
		return gnutls_assert_val(GNUTLS_E_UNIMPLEMENTED_FEATURE);

	if (cert_url == NULL && key_url == NULL)
		return 0;

	if (cert_url != NULL) {
		id_size = sizeof(id);
		ret = get_id(cert_url, id, &id_size, 1);
		if (ret < 0)
			return gnutls_assert_val(ret);
	} else {
		id_size = sizeof(id);
		ret = get_id(key_url, id, &id_size, 0);
		if (ret < 0)
			return gnutls_assert_val(ret);
	}

	blob.cbData = id_size;
	blob.pbData = id;

	store = CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, 0, CERT_SYSTEM_STORE_CURRENT_USER, L"MY");
	if (store != NULL) {
		do {
			cert = CertFindCertificateInStore(store,
							  X509_ASN_ENCODING,
							  0,
							  CERT_FIND_KEY_IDENTIFIER,
							  &blob, cert);

			if (cert && key_url) {
				nc_size = sizeof(nc);
				r = CertGetCertificateContextProperty(cert,
								      CERT_NCRYPT_KEY_HANDLE_TRANSFER_PROP_ID,
								      &nc,
								      &nc_size);
				if (r != 0) {
					pNCryptDeleteKey(nc, 0);
					pNCryptFreeObject(nc);
				} else {
					gnutls_assert();
				}
			}

			if (cert && cert_url)
				CertDeleteCertificateFromStore(cert);
		} while (cert != NULL);
		CertCloseStore(store, 0);
	}

	return 0;
}

/**
 * gnutls_system_key_add_x509:
 * @crt: the certificate to be added
 * @privkey: the key to be added
 * @label: the friendly name to describe the key
 * @cert_url: if non-NULL it will contain an allocated value with the certificate URL
 * @key_url: if non-NULL it will contain an allocated value with the key URL
 *
 * This function will added the given key and certificate pair,
 * to the system list.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.4.0
 **/
int gnutls_system_key_add_x509(gnutls_x509_crt_t crt,
			       gnutls_x509_privkey_t privkey, const char *label,
			       char **cert_url, char **key_url)
{
	HCERTSTORE store = NULL;
	CRYPT_DATA_BLOB pfx;
	gnutls_datum_t _pfx = { NULL, 0 };
	gnutls_pkcs12_t p12 = NULL;
	gnutls_pkcs12_bag_t bag1 = NULL, bag2 = NULL;
	uint8_t id[MAX_WID_SIZE];
	size_t id_size;
	gnutls_datum_t kid;
	int ret;

	if (ncrypt_init == 0)
		return gnutls_assert_val(GNUTLS_E_UNIMPLEMENTED_FEATURE);

	if (label == NULL)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	id_size = sizeof(id);
	ret = gnutls_x509_crt_get_key_id(crt, 0, id, &id_size);
	if (ret < 0)
		return gnutls_assert_val(ret);

	kid.data = id;
	kid.size = id_size;

	/* the idea: import the cert and private key into PKCS #12
	 * format, export it into pfx, and import it into store */
	ret = gnutls_pkcs12_init(&p12);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = gnutls_pkcs12_bag_init(&bag1);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = gnutls_pkcs12_bag_set_crt(bag1, crt);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = gnutls_pkcs12_bag_set_key_id(bag1, 0, &kid);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	if (label)
		gnutls_pkcs12_bag_set_friendly_name(bag1, 0, label);

	ret = gnutls_pkcs12_bag_init(&bag2);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = gnutls_pkcs12_bag_set_privkey(bag2, privkey, NULL, 0);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = gnutls_pkcs12_bag_set_key_id(bag2, 0, &kid);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	if (label)
		gnutls_pkcs12_bag_set_friendly_name(bag2, 0, label);

	ret = gnutls_pkcs12_set_bag(p12, bag1);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = gnutls_pkcs12_set_bag(p12, bag2);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = gnutls_pkcs12_generate_mac(p12, "123456");
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = gnutls_pkcs12_export2(p12, GNUTLS_X509_FMT_DER, &_pfx);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	pfx.cbData = _pfx.size;
	pfx.pbData = _pfx.data;

	store = PFXImportCertStore(&pfx, L"123456", 0);
	if (store == NULL) {
		gnutls_assert();
		ret = GNUTLS_E_INVALID_REQUEST;
		goto cleanup;
	}

	if (cert_url || key_url) {
		unsigned char sha[20];
		CRYPT_HASH_BLOB blob;
		const CERT_CONTEXT *cert = NULL;
		gnutls_datum_t data;

		ret = gnutls_x509_crt_export2(crt, GNUTLS_X509_FMT_DER, &data);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		ret =
		    gnutls_hash_fast(GNUTLS_DIG_SHA1, data.data, data.size,
				     sha);
		gnutls_free(data.data);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		blob.cbData = sizeof(sha);
		blob.pbData = sha;

		cert = CertFindCertificateInStore(store,
						  X509_ASN_ENCODING,
						  0,
						  CERT_FIND_SHA1_HASH,
						  &blob, NULL);

		if (cert == NULL) {
			gnutls_assert();
			ret = GNUTLS_E_KEY_IMPORT_FAILED;
			goto cleanup;
		}

		ret = get_win_urls(cert, cert_url, key_url, NULL, NULL);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}
	}

	ret = 0;

 cleanup:
	if (p12 != NULL)
		gnutls_pkcs12_deinit(p12);
	if (bag1 != NULL)
		gnutls_pkcs12_bag_deinit(bag1);
	if (bag2 != NULL)
		gnutls_pkcs12_bag_deinit(bag2);
	if (store != NULL)
		CertCloseStore(store, 0);
	gnutls_free(_pfx.data);
	return ret;
}

int _gnutls_system_key_init(void)
{
	int ret;

#ifdef DYN_NCRYPT
	ncrypt_lib = LoadLibrary(TEXT("ncrypt.dll"));
	if (ncrypt_lib == NULL) {
		return gnutls_assert_val(GNUTLS_E_CRYPTO_INIT_FAILED);
	}

	pNCryptDeleteKey =
	    (NCryptDeleteKeyFunc) GetProcAddress(ncrypt_lib, "NCryptDeleteKey");
	if (pNCryptDeleteKey == NULL) {
		ret = GNUTLS_E_CRYPTO_INIT_FAILED;
		goto fail;
	}

	pNCryptOpenStorageProvider =
	    (NCryptOpenStorageProviderFunc) GetProcAddress(ncrypt_lib,
							   "NCryptOpenStorageProvider");
	if (pNCryptOpenStorageProvider == NULL) {
		ret = GNUTLS_E_CRYPTO_INIT_FAILED;
		goto fail;
	}

	pNCryptOpenKey =
	    (NCryptOpenKeyFunc) GetProcAddress(ncrypt_lib, "NCryptOpenKey");
	if (pNCryptOpenKey == NULL) {
		ret = GNUTLS_E_CRYPTO_INIT_FAILED;
		goto fail;
	}

	pNCryptGetProperty =
	    (NCryptGetPropertyFunc) GetProcAddress(ncrypt_lib,
						   "NCryptGetProperty");
	if (pNCryptGetProperty == NULL) {
		ret = GNUTLS_E_CRYPTO_INIT_FAILED;
		goto fail;
	}

	pNCryptFreeObject =
	    (NCryptFreeObjectFunc) GetProcAddress(ncrypt_lib,
						  "NCryptFreeObject");
	if (pNCryptFreeObject == NULL) {
		ret = GNUTLS_E_CRYPTO_INIT_FAILED;
		goto fail;
	}

	pNCryptDecrypt =
	    (NCryptDecryptFunc) GetProcAddress(ncrypt_lib, "NCryptDecrypt");
	if (pNCryptDecrypt == NULL) {
		ret = GNUTLS_E_CRYPTO_INIT_FAILED;
		goto fail;
	}

	pNCryptSignHash =
	    (NCryptSignHashFunc) GetProcAddress(ncrypt_lib, "NCryptSignHash");
	if (pNCryptSignHash == NULL) {
		ret = GNUTLS_E_CRYPTO_INIT_FAILED;
		goto fail;
	}
#endif
	ncrypt_init = 1;

	return 0;
 fail:
	FreeLibrary(ncrypt_lib);
	return ret;
}

void _gnutls_system_key_deinit(void)
{
	if (ncrypt_init != 0) {
		FreeLibrary(ncrypt_lib);
		ncrypt_init = 0;
	}
}
