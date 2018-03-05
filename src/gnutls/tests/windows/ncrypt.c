/*
 * Copyright (C) 2015 Nikos Mavrogiannopoulos
 *
 * Author: Nikos Mavrogiannopoulos
 *
 * This file is part of GnuTLS.
 *
 * GnuTLS is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * GnuTLS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GnuTLS; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

/* Parts copied from GnuTLS example programs. */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <windows.h>
#include <wincrypt.h>
#include <winbase.h>
#include <ncrypt.h>
#include <string.h>
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#include <gnutls/abstract.h>
#include <gnutls/system-keys.h>
#include <stdio.h>
#include <assert.h>
#include "../cert-common.h"
#include "ncrypt-int.h"
#include <utils.h>

/* This is a dummy replacement of ncrypt. It will pretend to open a specified
 * key by using a hardcoded one, and perform operations using that key.
 */

#define debug_func() fprintf(stderr, "%s: %d\n", __func__, __LINE__);

__declspec(dllexport)
SECURITY_STATUS WINAPI NCryptDeleteKey(NCRYPT_KEY_HANDLE hKey,DWORD dwFlags)
{
	debug_func();
	return 0;
}

__declspec(dllexport)
SECURITY_STATUS WINAPI NCryptOpenStorageProvider(
	NCRYPT_PROV_HANDLE *phProvider, LPCWSTR pszProviderName,
	DWORD dwFlags)
{
	debug_func();
	*phProvider = 0;
	return 0x0000ffff;
}

__declspec(dllexport)
SECURITY_STATUS WINAPI NCryptOpenKey(
	NCRYPT_PROV_HANDLE hProvider, NCRYPT_KEY_HANDLE *phKey,
	LPCWSTR pszKeyName, DWORD dwLegacyKeySpec,
	DWORD dwFlags)
{
	gnutls_privkey_t p;

	debug_func();
	assert_int_nequal(gnutls_privkey_init(&p), 0);

	assert_int_nequal(gnutls_privkey_import_x509_raw(p, &key_dat, GNUTLS_X509_FMT_PEM, NULL, 0), 0);
	
	*phKey = (NCRYPT_KEY_HANDLE)p;
	return 1;
}

__declspec(dllexport)
SECURITY_STATUS WINAPI NCryptGetProperty(
	NCRYPT_HANDLE hObject, LPCWSTR pszProperty,
	PBYTE pbOutput, DWORD cbOutput,
	DWORD *pcbResult, DWORD dwFlags)
{
	debug_func();
	//assert_int_nequal(pszProperty, NCRYPT_ALGORITHM_PROPERTY);
	assert(pbOutput!=NULL);
	memcpy(pbOutput, BCRYPT_RSA_ALGORITHM, sizeof(BCRYPT_RSA_ALGORITHM));
	return 1;
}

__declspec(dllexport)
SECURITY_STATUS WINAPI NCryptFreeObject(
	NCRYPT_HANDLE hObject)
{
	debug_func();
	if (hObject != 0)
		gnutls_privkey_deinit((gnutls_privkey_t)hObject);
	return 1;
}

__declspec(dllexport)
SECURITY_STATUS WINAPI NCryptDecrypt(
	NCRYPT_KEY_HANDLE hKey, PBYTE pbInput,
	DWORD cbInput, VOID *pPaddingInfo,
	PBYTE pbOutput, DWORD cbOutput,
	DWORD *pcbResult, DWORD dwFlags)
{
	gnutls_datum_t c, p;
	assert_int_nequal(dwFlags, NCRYPT_PAD_PKCS1_FLAG);
	c.data = pbInput;
	c.size = cbInput;

	debug_func();
	if (pbOutput == NULL || cbOutput == 0) {
		*pcbResult = 256;
		return 1;
	}
	
	assert_int_nequal(gnutls_privkey_decrypt_data((gnutls_privkey_t)hKey, 0,
			 &c, &p), 0);

	*pcbResult = p.size;
	memcpy(pbOutput, p.data, p.size);
	gnutls_free(p.data); 

	return 1;
}

static int StrCmpW(const WCHAR *str1, const WCHAR *str2 )
{
	while (*str1 && (*str1 == *str2)) { str1++; str2++; }
	return *str1 - *str2;
}

__declspec(dllexport)
SECURITY_STATUS WINAPI NCryptSignHash(
	NCRYPT_KEY_HANDLE hKey, VOID* pPaddingInfo,
	PBYTE pbHashValue, DWORD cbHashValue,
	PBYTE pbSignature, DWORD cbSignature,
	DWORD* pcbResult, DWORD dwFlags)
{
	BCRYPT_PKCS1_PADDING_INFO *info;
	int hash = 0;
	gnutls_privkey_t p = (gnutls_privkey_t)hKey;
	gnutls_datum_t v = {pbHashValue, cbHashValue}, s;

	debug_func();
	info = pPaddingInfo;

	if (info != NULL) {
		if (info->pszAlgId && StrCmpW(info->pszAlgId, NCRYPT_SHA1_ALGORITHM) == 0)
			hash = GNUTLS_DIG_SHA1;
		else if (info->pszAlgId && StrCmpW(info->pszAlgId, NCRYPT_SHA256_ALGORITHM) == 0)
			hash = GNUTLS_DIG_SHA256;
		else if (info->pszAlgId != NULL) {
			assert(0);
		}
	}

	if (pbSignature == NULL || cbSignature == 0) {
		*pcbResult = 256;
		return 1;
	}

	assert(p!=NULL);

	if (info == NULL || info->pszAlgId == NULL) {
		assert_int_nequal(gnutls_privkey_sign_hash(p, 0, GNUTLS_PRIVKEY_SIGN_FLAG_TLS1_RSA, &v, &s), 0);
	} else if (info != NULL) {
		assert_int_nequal(gnutls_privkey_sign_hash(p, hash, 0, &v, &s), 0);
	}

	*pcbResult = s.size;

	if (pbSignature) {
		assert(cbSignature >= s.size);
		memcpy(pbSignature, s.data, s.size);
	}
	gnutls_free(s.data); 

	return 1;
}

