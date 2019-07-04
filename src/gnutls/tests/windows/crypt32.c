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

#undef DECLSPEC_IMPORT
#define DECLSPEC_IMPORT
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
#include "ncrypt-int.h"
#include <utils.h>

#define VALID_PTR (void*)0x0001

/* This is dummy crypt32 replacement with stub functions. It pretends
 * to load the key store and find a single certificate in the store
 * of which it will return some arbitrary but valid values in CertGetCertificateContextProperty.
 */

__declspec(dllexport)
HCERTSTORE WINAPI CertOpenSystemStore(
	HCRYPTPROV_LEGACY hprov, LPCSTR szSubsystemProtocol)
{
	return VALID_PTR;
}

__declspec(dllexport)
HCERTSTORE WINAPI CertOpenStore(
    LPCSTR lpszStoreProvider, DWORD dwEncodingType,
    HCRYPTPROV_LEGACY hCryptProv, DWORD dwFlags,
    const void *pvPara)
{
	return VALID_PTR;
}

__declspec(dllexport)
BOOL WINAPI CertCloseStore(HCERTSTORE hCertStore, DWORD      dwFlags)
{
	assert_int_nequal(hCertStore, VALID_PTR);
	return 1;
}

__declspec(dllexport)
PCCERT_CONTEXT WINAPI CertFindCertificateInStore(
	HCERTSTORE hCertStore, DWORD dwCertEncodingType,
	DWORD dwFindFlags, DWORD dwFindType,
	const void *pvFindPara, PCCERT_CONTEXT pPrevCertContext)
{
	//CRYPT_HASH_BLOB *blob = (void*)pvFindPara;

	assert_int_nequal(hCertStore, VALID_PTR);

	assert_int_nequal(dwCertEncodingType, X509_ASN_ENCODING);
	assert_int_nequal(dwFindType, CERT_FIND_KEY_IDENTIFIER);

	return VALID_PTR;
}

__declspec(dllexport)
BOOL WINAPI CertGetCertificateContextProperty(PCCERT_CONTEXT pCertContext,
	DWORD dwPropId, void *pvData, DWORD *pcbData)
{
	if (dwPropId == CERT_FRIENDLY_NAME_PROP_ID) {
		*pcbData = snprintf(pvData, *pcbData, "friendly");
		return 1;
	}

	if (dwPropId == CERT_KEY_IDENTIFIER_PROP_ID) {
		*pcbData = snprintf(pvData, *pcbData, "\xff\xff\x01\xff");
		return 1;
	}

	if (dwPropId == CERT_NCRYPT_KEY_HANDLE_TRANSFER_PROP_ID) {
		return 1;
	}

	if (dwPropId == CERT_KEY_PROV_INFO_PROP_ID) {
		if (pvData == NULL) {
			*pcbData = sizeof(CRYPT_KEY_PROV_INFO);
			return 1;
		}
		assert(*pcbData >= sizeof(CRYPT_KEY_PROV_INFO));

		memset(pvData, 0, sizeof(CRYPT_KEY_PROV_INFO));
		*pcbData = sizeof(CRYPT_KEY_PROV_INFO);

		return 1;
	}

	assert(0);
	return 0;
}

__declspec(dllexport)
PCCRL_CONTEXT WINAPI CertEnumCRLsInStore(HCERTSTORE hCertStore, PCCRL_CONTEXT pPrevCrlContext)
{
	return NULL;
}

__declspec(dllexport)
BOOL WINAPI CertDeleteCertificateFromStore(PCCERT_CONTEXT pCertContext)
{
	return 1;
}

__declspec(dllexport)
HCERTSTORE WINAPI PFXImportCertStore(CRYPT_DATA_BLOB *pPFX, LPCWSTR szPassword, DWORD dwFlags)
{
	return NULL;
}

__declspec(dllexport)
PCCERT_CONTEXT WINAPI CertEnumCertificatesInStore(HCERTSTORE hCertStore,
	PCCERT_CONTEXT pPrevCertContext)
{
	return NULL;
}

__declspec(dllexport)
BOOL WINAPI CertFreeCertificateContext(PCCERT_CONTEXT pCertContext)
{
	return 1;
}

/* These are for CAPI, and are placeholders */
__declspec(dllexport)
BOOL WINAPI CryptGetProvParam(HCRYPTPROV hProv, DWORD dwParam,
			      BYTE *pbData, DWORD *pdwDataLen,
			      DWORD dwFlags)
{
	return 0;
}

__declspec(dllexport)
BOOL WINAPI CryptAcquireContextW(HCRYPTPROV *phProv, LPCWSTR szContainer,
				 LPCWSTR szProvider, DWORD dwProvType, DWORD dwFlags)
{
	return 0;
}

__declspec(dllexport)
BOOL WINAPI CryptDecrypt(HCRYPTKEY hKey, HCRYPTHASH hHash, BOOL Final,
			 DWORD dwFlags, BYTE *pbData, DWORD *pdwDataLen)
{
	return 0;
}

__declspec(dllexport)
BOOL WINAPI CryptDestroyHash(HCRYPTHASH hHash)
{
	return 1;
}

__declspec(dllexport)
BOOL WINAPI CryptSignHash(
  HCRYPTHASH hHash,
  DWORD      dwKeySpec,
  LPCTSTR    sDescription,
  DWORD      dwFlags,
  BYTE       *pbSignature,
  DWORD      *pdwSigLen)
{
	return 0;
}

__declspec(dllexport)
BOOL WINAPI CryptGetHashParam(HCRYPTHASH hHash, DWORD dwParam,
			      BYTE *pbData, DWORD *pdwDataLen, DWORD dwFlags)
{
	return 0;
}

__declspec(dllexport)
BOOL WINAPI CryptSetHashParam(HCRYPTHASH hHash, DWORD dwParam,
			      const BYTE *pbData, DWORD dwFlags)
{
	return 0;
}


__declspec(dllexport)
BOOL WINAPI CryptCreateHash(HCRYPTPROV hProv, ALG_ID Algid, HCRYPTKEY hKey,
			    DWORD dwFlags, HCRYPTHASH *phHash)
{
	return 0;
}
