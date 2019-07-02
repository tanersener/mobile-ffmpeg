/*
 *  PKCS11-MOCK - PKCS#11 mock module
 *  Copyright (c) 2015 JWC s.r.o. <http://www.jwc.sk>
 *  Author: Jaroslav Imrich <jimrich@jimrich.sk>
 *
 *  Licensing for open source projects:
 *  PKCS11-MOCK is available under the terms of the GNU Affero General 
 *  Public License version 3 as published by the Free Software Foundation.
 *  Please see <https://www.gnu.org/licenses/agpl-3.0.html> for more details.
 *
 *  Licensing for other types of projects:
 *  PKCS11-MOCK is available under the terms of flexible commercial license.
 *  Please contact JWC s.r.o. at <info@pkcs11interop.net> for more details.
 */

#include "pkcs11-mock.h"
#include "pkcs11-mock-ext.h"
#include <string.h>
#include <stdlib.h>

unsigned int pkcs11_mock_flags = 0;

/* This is a very basic mock PKCS #11 module that will return a given fixed
 * certificate, and public key for all searches. It will also provide a
 * CKO_X_CERTIFICATE_EXTENSION so that it can be used as a p11-kit trust
 * module. */

const char mock_certificate[] =
	"\x30\x82\x03\x97\x30\x82\x02\x4f\xa0\x03\x02\x01\x02\x02\x04\x4d"
	"\xa7\x54\x21\x30\x0d\x06\x09\x2a\x86\x48\x86\xf7\x0d\x01\x01\x0b"
	"\x05\x00\x30\x32\x31\x0b\x30\x09\x06\x03\x55\x04\x06\x13\x02\x42"
	"\x45\x31\x0f\x30\x0d\x06\x03\x55\x04\x0a\x13\x06\x47\x6e\x75\x54"
	"\x4c\x53\x31\x12\x30\x10\x06\x03\x55\x04\x03\x13\x09\x6c\x6f\x63"
	"\x61\x6c\x68\x6f\x73\x74\x30\x1e\x17\x0d\x31\x31\x30\x34\x31\x34"
	"\x32\x30\x30\x38\x30\x32\x5a\x17\x0d\x33\x38\x30\x38\x32\x39\x32"
	"\x30\x30\x38\x30\x34\x5a\x30\x32\x31\x0b\x30\x09\x06\x03\x55\x04"
	"\x06\x13\x02\x42\x45\x31\x0f\x30\x0d\x06\x03\x55\x04\x0a\x13\x06"
	"\x47\x6e\x75\x54\x4c\x53\x31\x12\x30\x10\x06\x03\x55\x04\x03\x13"
	"\x09\x6c\x6f\x63\x61\x6c\x68\x6f\x73\x74\x30\x82\x01\x52\x30\x0d"
	"\x06\x09\x2a\x86\x48\x86\xf7\x0d\x01\x01\x01\x05\x00\x03\x82\x01"
	"\x3f\x00\x30\x82\x01\x3a\x02\x82\x01\x31\x00\xdd\xcf\x97\xd2\xa5"
	"\x1d\x95\xdd\x86\x18\xd8\xc4\xb9\xad\xa6\x0c\xb4\x9d\xb6\xdc\xfa"
	"\xdc\x21\xe1\x3a\x62\x34\x07\xe8\x33\xb2\xe8\x97\xee\x2c\x41\xd2"
	"\x12\xf1\x5f\xed\xe4\x76\xff\x65\x26\x1e\x0c\xc7\x41\x15\x69\x5f"
	"\x0d\xf9\xad\x89\x14\x8d\xea\xd7\x16\x52\x9a\x47\xc1\xbb\x00\x02"
	"\xe4\x88\x45\x73\x78\xa4\xae\xdb\x38\xc3\xc6\x07\xd2\x64\x0e\x87"
	"\xed\x74\x8c\x6b\xc4\xc0\x02\x50\x7c\x4e\xa6\xd1\x58\xe9\xe5\x13"
	"\x09\xa9\xdb\x5a\xea\xeb\x0f\x06\x80\x5c\x09\xef\x94\xc8\xe9\xfb"
	"\x37\x2e\x75\xe1\xac\x93\xad\x9b\x37\x13\x4b\x66\x3a\x76\x33\xd8"
	"\xc4\xd7\x4c\xfb\x61\xc8\x92\x21\x07\xfc\xdf\xa9\x88\x54\xe4\xa3"
	"\xa9\x47\xd2\x6c\xb8\xe3\x39\x89\x11\x88\x38\x2d\xa2\xdc\x3e\x5e"
	"\x4a\xa9\xa4\x8e\xd5\x1f\xb2\xd0\xdd\x41\x3c\xda\x10\x68\x9e\x47"
	"\x1b\x65\x02\xa2\xc5\x28\x73\x02\x83\x03\x09\xfd\xf5\x29\x7e\x97"
	"\xdc\x2a\x4e\x4b\xaa\x79\x46\x46\x70\x86\x1b\x9b\xb8\xf6\x8a\xbe"
	"\x29\x87\x7d\x5f\xda\xa5\x97\x6b\xef\xc8\x43\x09\x43\xe2\x1f\x8a"
	"\x16\x7e\x1d\x50\x5d\xf5\xda\x02\xee\xf2\xc3\x2a\x48\xe6\x6b\x30"
	"\xea\x02\xd7\xef\xac\x8b\x0c\xb8\xc1\x85\xd8\xbf\x7c\x85\xa8\x1e"
	"\x83\xbe\x5c\x26\x2e\x79\x7b\x47\xf5\x4a\x3f\x66\x62\x92\xfd\x41"
	"\x20\xb6\x2c\x00\xf0\x52\xca\x26\x06\x2d\x7c\xcf\x7a\x50\x7d\x0f"
	"\xcb\xdd\x97\x20\xc8\x6f\xe4\xe0\x50\xf4\xe3\x02\x03\x01\x00\x01"
	"\xa3\x55\x30\x53\x30\x0c\x06\x03\x55\x1d\x13\x01\x01\xff\x04\x02"
	"\x30\x00\x30\x13\x06\x03\x55\x1d\x25\x04\x0c\x30\x0a\x06\x08\x2b"
	"\x06\x01\x05\x05\x07\x03\x01\x30\x0f\x06\x03\x55\x1d\x0f\x01\x01"
	"\xff\x04\x05\x03\x03\x07\xa0\x00\x30\x1d\x06\x03\x55\x1d\x0e\x04"
	"\x16\x04\x14\x92\x53\xd6\x71\xb9\xf8\x68\xaa\xb3\x53\xf6\x8d\xf5"
	"\x39\x45\x66\x9c\xa7\xe5\x31\x30\x0d\x06\x09\x2a\x86\x48\x86\xf7"
	"\x0d\x01\x01\x0b\x05\x00\x03\x82\x01\x31\x00\x98\xbf\x48\x89\xc1"
	"\xe6\xe6\x15\x13\xcc\xfc\xba\xed\xa0\x89\xe5\x86\x45\x30\x73\x68"
	"\xb2\x79\x1f\x88\x02\x80\xfb\x2d\xc9\xb8\x21\x55\x8d\xc5\xb7\x56"
	"\x1b\xcf\xc3\x76\xee\xd0\xf0\xd9\x22\x3a\x63\x92\xc5\x04\x86\x70"
	"\x1e\x42\x33\x2a\x3b\xc4\x14\x08\xc5\x42\x92\x73\x7c\x3e\x39\xc0"
	"\xee\x34\xc7\x33\x16\x5f\x93\xae\xcf\x1f\x9a\x30\x09\x51\xfe\x2d"
	"\x94\x9c\x28\xad\x2a\x7e\xe4\x14\x81\x45\x6b\x0d\xd7\x11\x21\xfc"
	"\xdb\x27\x17\x74\xb4\xcc\x94\x1a\x6e\x9e\x7b\x58\xa9\xe0\x06\x8d"
	"\xda\x5f\x60\xe1\xb8\x6f\x28\x68\xb6\x58\xbe\xc5\xac\x36\x47\x37"
	"\xf6\xa8\x38\x74\x23\x81\xf3\x22\xbe\x61\xff\x08\x08\x87\xeb\xc2"
	"\x8f\x29\x25\x75\x5d\x4c\xeb\xd5\x09\x28\xab\x7b\x99\xf9\x69\x08"
	"\xa2\xc6\x02\xd2\x2e\xcd\xfa\xf1\x19\xce\x3f\x44\x6a\xa1\x4b\xa8"
	"\x56\xd5\x11\xae\x44\xe3\x68\x05\x50\x57\x8d\x72\x0f\xc7\x21\xdb"
	"\x8f\xa3\x50\x78\x5d\x5a\x39\xcb\x90\x3d\x52\x43\x33\xbf\xea\x89"
	"\x07\x1a\x92\xcc\x85\x27\xa8\x3d\x34\xb8\x5b\x52\xee\xef\x20\xb9"
	"\xb6\xff\xea\xc5\x90\xd3\x47\xc5\x51\x90\xe2\xe6\x3e\x52\xb9\x1e"
	"\x79\x18\xbe\xfd\xe2\x24\xbe\x47\x32\x5a\xb0\x03\x6b\xaa\xdb\xc3"
	"\xdb\xf6\x60\x44\x08\xb6\x2c\x19\x47\xa2\xf0\x43\x7f\xf0\x07\x97"
	"\x57\xab\xec\xa0\xb8\x6a\x49\xce\x08\xe6\xc3\x4d\xf2\xa4\xe9\xb8"
	"\x43\xe7\xf0\x84\xd7\x1a\x72\x14\x5d\x82\x1a";

/* ca == true */
const char mock_cert_ext1[] = "\x30\x0f\x06\x03\x55\x1d\x13\x01\x01\xff\x04\x05\x30\x03\x01\x01\xff";
/* GNUTLS_KEY_ENCIPHER_ONLY | GNUTLS_KEY_KEY_ENCIPHERMENT | GNUTLS_KEY_KEY_CERT_SIGN */
const char mock_cert_ext2[] = "\x30\x0f\x06\x03\x55\x1d\x0f\x01\x01\xff\x04\x05\x03\x03\x07\x25\x00";

const char mock_pubkey[] =
	"\x30\x82\x01\x52\x30\x0d\x06\x09\x2a\x86\x48\x86\xf7\x0d\x01\x01"
	"\x01\x05\x00\x03\x82\x01\x3f\x00\x30\x82\x01\x3a\x02\x82\x01\x31"
	"\x00\xdd\xcf\x97\xd2\xa5\x1d\x95\xdd\x86\x18\xd8\xc4\xb9\xad\xa6"
	"\x0c\xb4\x9d\xb6\xdc\xfa\xdc\x21\xe1\x3a\x62\x34\x07\xe8\x33\xb2"
	"\xe8\x97\xee\x2c\x41\xd2\x12\xf1\x5f\xed\xe4\x76\xff\x65\x26\x1e"
	"\x0c\xc7\x41\x15\x69\x5f\x0d\xf9\xad\x89\x14\x8d\xea\xd7\x16\x52"
	"\x9a\x47\xc1\xbb\x00\x02\xe4\x88\x45\x73\x78\xa4\xae\xdb\x38\xc3"
	"\xc6\x07\xd2\x64\x0e\x87\xed\x74\x8c\x6b\xc4\xc0\x02\x50\x7c\x4e"
	"\xa6\xd1\x58\xe9\xe5\x13\x09\xa9\xdb\x5a\xea\xeb\x0f\x06\x80\x5c"
	"\x09\xef\x94\xc8\xe9\xfb\x37\x2e\x75\xe1\xac\x93\xad\x9b\x37\x13"
	"\x4b\x66\x3a\x76\x33\xd8\xc4\xd7\x4c\xfb\x61\xc8\x92\x21\x07\xfc"
	"\xdf\xa9\x88\x54\xe4\xa3\xa9\x47\xd2\x6c\xb8\xe3\x39\x89\x11\x88"
	"\x38\x2d\xa2\xdc\x3e\x5e\x4a\xa9\xa4\x8e\xd5\x1f\xb2\xd0\xdd\x41"
	"\x3c\xda\x10\x68\x9e\x47\x1b\x65\x02\xa2\xc5\x28\x73\x02\x83\x03"
	"\x09\xfd\xf5\x29\x7e\x97\xdc\x2a\x4e\x4b\xaa\x79\x46\x46\x70\x86"
	"\x1b\x9b\xb8\xf6\x8a\xbe\x29\x87\x7d\x5f\xda\xa5\x97\x6b\xef\xc8"
	"\x43\x09\x43\xe2\x1f\x8a\x16\x7e\x1d\x50\x5d\xf5\xda\x02\xee\xf2"
	"\xc3\x2a\x48\xe6\x6b\x30\xea\x02\xd7\xef\xac\x8b\x0c\xb8\xc1\x85"
	"\xd8\xbf\x7c\x85\xa8\x1e\x83\xbe\x5c\x26\x2e\x79\x7b\x47\xf5\x4a"
	"\x3f\x66\x62\x92\xfd\x41\x20\xb6\x2c\x00\xf0\x52\xca\x26\x06\x2d"
	"\x7c\xcf\x7a\x50\x7d\x0f\xcb\xdd\x97\x20\xc8\x6f\xe4\xe0\x50\xf4"
	"\xe3\x02\x03\x01\x00\x01";
const char mock_public_exponent[] = "\x01\x00\x01";
const char mock_modulus[] =
	"\xDD\xCF\x97\xD2\xA5\x1D\x95\xDD\x86\x18\xD8\xC4\xB9\xAD\xA6\x0C"
	"\xB4\x9D\xB6\xDC\xFA\xDC\x21\xE1\x3A\x62\x34\x07\xE8\x33\xB2\xE8"
	"\x97\xEE\x2C\x41\xD2\x12\xF1\x5F\xED\xE4\x76\xFF\x65\x26\x1E\x0C"
	"\xC7\x41\x15\x69\x5F\x0D\xF9\xAD\x89\x14\x8D\xEA\xD7\x16\x52\x9A"
	"\x47\xC1\xBB\x00\x02\xE4\x88\x45\x73\x78\xA4\xAE\xDB\x38\xC3\xC6"
	"\x07\xD2\x64\x0E\x87\xED\x74\x8C\x6B\xC4\xC0\x02\x50\x7C\x4E\xA6"
	"\xD1\x58\xE9\xE5\x13\x09\xA9\xDB\x5A\xEA\xEB\x0F\x06\x80\x5C\x09"
	"\xEF\x94\xC8\xE9\xFB\x37\x2E\x75\xE1\xAC\x93\xAD\x9B\x37\x13\x4B"
	"\x66\x3A\x76\x33\xD8\xC4\xD7\x4C\xFB\x61\xC8\x92\x21\x07\xFC\xDF"
	"\xA9\x88\x54\xE4\xA3\xA9\x47\xD2\x6C\xB8\xE3\x39\x89\x11\x88\x38"
	"\x2D\xA2\xDC\x3E\x5E\x4A\xA9\xA4\x8E\xD5\x1F\xB2\xD0\xDD\x41\x3C"
	"\xDA\x10\x68\x9E\x47\x1B\x65\x02\xA2\xC5\x28\x73\x02\x83\x03\x09"
	"\xFD\xF5\x29\x7E\x97\xDC\x2A\x4E\x4B\xAA\x79\x46\x46\x70\x86\x1B"
	"\x9B\xB8\xF6\x8A\xBE\x29\x87\x7D\x5F\xDA\xA5\x97\x6B\xEF\xC8\x43"
	"\x09\x43\xE2\x1F\x8A\x16\x7E\x1D\x50\x5D\xF5\xDA\x02\xEE\xF2\xC3"
	"\x2A\x48\xE6\x6B\x30\xEA\x02\xD7\xEF\xAC\x8B\x0C\xB8\xC1\x85\xD8"
	"\xBF\x7C\x85\xA8\x1E\x83\xBE\x5C\x26\x2E\x79\x7B\x47\xF5\x4A\x3F"
	"\x66\x62\x92\xFD\x41\x20\xB6\x2C\x00\xF0\x52\xCA\x26\x06\x2D\x7C"
	"\xCF\x7A\x50\x7D\x0F\xCB\xDD\x97\x20\xC8\x6F\xE4\xE0\x50\xF4\xE3";
const char mock_subject[] =
	"DN: C=US, O=Test Government, OU=Test Department, OU=Test Agency/serialNumber=";

CK_BBOOL pkcs11_mock_initialized = CK_FALSE;
CK_BBOOL pkcs11_mock_session_opened = CK_FALSE;
CK_BBOOL pkcs11_mock_session_reauth = CK_FALSE;

static session_ptr_st *mock_session = NULL;

CK_FUNCTION_LIST pkcs11_mock_functions = 
{
	{2, 20},
	&C_Initialize,
	&C_Finalize,
	&C_GetInfo,
	&C_GetFunctionList,
	&C_GetSlotList,
	&C_GetSlotInfo,
	&C_GetTokenInfo,
	&C_GetMechanismList,
	&C_GetMechanismInfo,
	&C_InitToken,
	&C_InitPIN,
	&C_SetPIN,
	&C_OpenSession,
	&C_CloseSession,
	&C_CloseAllSessions,
	&C_GetSessionInfo,
	&C_GetOperationState,
	&C_SetOperationState,
	&C_Login,
	&C_Logout,
	&C_CreateObject,
	&C_CopyObject,
	&C_DestroyObject,
	&C_GetObjectSize,
	&C_GetAttributeValue,
	&C_SetAttributeValue,
	&C_FindObjectsInit,
	&C_FindObjects,
	&C_FindObjectsFinal,
	&C_EncryptInit,
	&C_Encrypt,
	&C_EncryptUpdate,
	&C_EncryptFinal,
	&C_DecryptInit,
	&C_Decrypt,
	&C_DecryptUpdate,
	&C_DecryptFinal,
	&C_DigestInit,
	&C_Digest,
	&C_DigestUpdate,
	&C_DigestKey,
	&C_DigestFinal,
	&C_SignInit,
	&C_Sign,
	&C_SignUpdate,
	&C_SignFinal,
	&C_SignRecoverInit,
	&C_SignRecover,
	&C_VerifyInit,
	&C_Verify,
	&C_VerifyUpdate,
	&C_VerifyFinal,
	&C_VerifyRecoverInit,
	&C_VerifyRecover,
	&C_DigestEncryptUpdate,
	&C_DecryptDigestUpdate,
	&C_SignEncryptUpdate,
	&C_DecryptVerifyUpdate,
	&C_GenerateKey,
	&C_GenerateKeyPair,
	&C_WrapKey,
	&C_UnwrapKey,
	&C_DeriveKey,
	&C_SeedRandom,
	&C_GenerateRandom,
	&C_GetFunctionStatus,
	&C_CancelFunction,
	&C_WaitForSlotEvent
};

#if defined(HAVE___REGISTER_ATFORK)
extern int __register_atfork(void (*)(void), void(*)(void), void (*)(void), void *);
extern void *__dso_handle;
static unsigned registered_fork_handler = 0;

static void fork_handler(void)
{
	pkcs11_mock_initialized = CK_FALSE;
	pkcs11_mock_session_opened = CK_FALSE;
	if (mock_session) {
		mock_session->state = CKS_RO_PUBLIC_SESSION;
		mock_session->find_op.active_operation = PKCS11_MOCK_CK_OPERATION_NONE;
		free(mock_session->find_label);
	}
	free(mock_session);
	mock_session = NULL;
}
#endif


CK_DEFINE_FUNCTION(CK_RV, C_Initialize)(CK_VOID_PTR pInitArgs)
{
	if (CK_TRUE == pkcs11_mock_initialized)
		return CKR_CRYPTOKI_ALREADY_INITIALIZED;

	IGNORE(pInitArgs);
#if defined(HAVE___REGISTER_ATFORK)
	if (registered_fork_handler == 0) {
		__register_atfork(NULL, NULL, fork_handler, __dso_handle);
		registered_fork_handler = 1;
	}
#endif
	pkcs11_mock_initialized = CK_TRUE;

	return CKR_OK;
}


CK_DEFINE_FUNCTION(CK_RV, C_Finalize)(CK_VOID_PTR pReserved)
{
	if (CK_FALSE == pkcs11_mock_initialized)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	IGNORE(pReserved);

	pkcs11_mock_initialized = CK_FALSE;

	return CKR_OK;
}


CK_DEFINE_FUNCTION(CK_RV, C_GetInfo)(CK_INFO_PTR pInfo)
{
	if (CK_FALSE == pkcs11_mock_initialized)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	if (NULL == pInfo)
		return CKR_ARGUMENTS_BAD;

	pInfo->cryptokiVersion.major = 0x02;
	pInfo->cryptokiVersion.minor = 0x14;
	memset(pInfo->manufacturerID, ' ', sizeof(pInfo->manufacturerID));
	memcpy(pInfo->manufacturerID, PKCS11_MOCK_CK_INFO_MANUFACTURER_ID, strlen(PKCS11_MOCK_CK_INFO_MANUFACTURER_ID));
	pInfo->flags = 0;
	memset(pInfo->libraryDescription, ' ', sizeof(pInfo->libraryDescription));
	memcpy(pInfo->libraryDescription, PKCS11_MOCK_CK_INFO_LIBRARY_DESCRIPTION, strlen(PKCS11_MOCK_CK_INFO_LIBRARY_DESCRIPTION));
	pInfo->libraryVersion.major = 0x01;
	pInfo->libraryVersion.minor = 0x00;

	return CKR_OK;
}


CK_DEFINE_FUNCTION(CK_RV, C_GetFunctionList)(CK_FUNCTION_LIST_PTR_PTR ppFunctionList)
{
	if (NULL == ppFunctionList)
		return CKR_ARGUMENTS_BAD;

	*ppFunctionList = &pkcs11_mock_functions;

	return CKR_OK;
}


CK_DEFINE_FUNCTION(CK_RV, C_GetSlotList)(CK_BBOOL tokenPresent, CK_SLOT_ID_PTR pSlotList, CK_ULONG_PTR pulCount)
{
	if (CK_FALSE == pkcs11_mock_initialized)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	IGNORE(tokenPresent);

	if (NULL == pulCount)
		return CKR_ARGUMENTS_BAD;

	if (NULL == pSlotList)
	{
		*pulCount = 1;
	}
	else
	{
		if (0 == *pulCount)
			return CKR_BUFFER_TOO_SMALL;

		pSlotList[0] = PKCS11_MOCK_CK_SLOT_ID;
		*pulCount = 1;
	}

	return CKR_OK;
}


CK_DEFINE_FUNCTION(CK_RV, C_GetSlotInfo)(CK_SLOT_ID slotID, CK_SLOT_INFO_PTR pInfo)
{
	if (CK_FALSE == pkcs11_mock_initialized)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	if (PKCS11_MOCK_CK_SLOT_ID != slotID)
		return CKR_SLOT_ID_INVALID;

	if (NULL == pInfo)
		return CKR_ARGUMENTS_BAD;

	memset(pInfo->slotDescription, ' ', sizeof(pInfo->slotDescription));
	memcpy(pInfo->slotDescription, PKCS11_MOCK_CK_SLOT_INFO_SLOT_DESCRIPTION, strlen(PKCS11_MOCK_CK_SLOT_INFO_SLOT_DESCRIPTION));
	memset(pInfo->manufacturerID, ' ', sizeof(pInfo->manufacturerID));
	memcpy(pInfo->manufacturerID, PKCS11_MOCK_CK_SLOT_INFO_MANUFACTURER_ID, strlen(PKCS11_MOCK_CK_SLOT_INFO_MANUFACTURER_ID));
	pInfo->flags = CKF_TOKEN_PRESENT;
	pInfo->hardwareVersion.major = 0x01;
	pInfo->hardwareVersion.minor = 0x00;
	pInfo->firmwareVersion.major = 0x01;
	pInfo->firmwareVersion.minor = 0x00;

	return CKR_OK;
}


CK_DEFINE_FUNCTION(CK_RV, C_GetTokenInfo)(CK_SLOT_ID slotID, CK_TOKEN_INFO_PTR pInfo)
{
	if (CK_FALSE == pkcs11_mock_initialized)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	if (PKCS11_MOCK_CK_SLOT_ID != slotID)
		return CKR_SLOT_ID_INVALID;

	if (NULL == pInfo)
		return CKR_ARGUMENTS_BAD;

	memset(pInfo->label, ' ', sizeof(pInfo->label));
	memcpy(pInfo->label, PKCS11_MOCK_CK_TOKEN_INFO_LABEL, strlen(PKCS11_MOCK_CK_TOKEN_INFO_LABEL));
	memset(pInfo->manufacturerID, ' ', sizeof(pInfo->manufacturerID));
	memcpy(pInfo->manufacturerID, PKCS11_MOCK_CK_TOKEN_INFO_MANUFACTURER_ID, strlen(PKCS11_MOCK_CK_TOKEN_INFO_MANUFACTURER_ID));
	memset(pInfo->model, ' ', sizeof(pInfo->model));
	memcpy(pInfo->model, PKCS11_MOCK_CK_TOKEN_INFO_MODEL, strlen(PKCS11_MOCK_CK_TOKEN_INFO_MODEL));
	memset(pInfo->serialNumber, ' ', sizeof(pInfo->serialNumber));
	memcpy(pInfo->serialNumber, PKCS11_MOCK_CK_TOKEN_INFO_SERIAL_NUMBER, strlen(PKCS11_MOCK_CK_TOKEN_INFO_SERIAL_NUMBER));
	pInfo->flags = CKF_RNG | CKF_LOGIN_REQUIRED | CKF_USER_PIN_INITIALIZED | CKF_TOKEN_INITIALIZED;

	if (pkcs11_mock_flags & MOCK_FLAG_SAFENET_ALWAYS_AUTH)
		pInfo->flags &= ~CKF_LOGIN_REQUIRED;

	pInfo->ulMaxSessionCount = CK_EFFECTIVELY_INFINITE;
	pInfo->ulSessionCount = (CK_TRUE == pkcs11_mock_session_opened) ? 1 : 0;
	pInfo->ulMaxRwSessionCount = CK_EFFECTIVELY_INFINITE;
	if ((CK_TRUE == pkcs11_mock_session_opened) && ((CKS_RO_PUBLIC_SESSION != mock_session->state) && (CKS_RO_USER_FUNCTIONS != mock_session->state)))
		pInfo->ulRwSessionCount = 1;
	else
		pInfo->ulRwSessionCount = 0;
	pInfo->ulMaxPinLen = PKCS11_MOCK_CK_TOKEN_INFO_MAX_PIN_LEN;
	pInfo->ulMinPinLen = PKCS11_MOCK_CK_TOKEN_INFO_MIN_PIN_LEN;
	pInfo->ulTotalPublicMemory = CK_UNAVAILABLE_INFORMATION;
	pInfo->ulFreePublicMemory = CK_UNAVAILABLE_INFORMATION;
	pInfo->ulTotalPrivateMemory = CK_UNAVAILABLE_INFORMATION;
	pInfo->ulFreePrivateMemory = CK_UNAVAILABLE_INFORMATION;
	pInfo->hardwareVersion.major = 0x01;
	pInfo->hardwareVersion.minor = 0x00;
	pInfo->firmwareVersion.major = 0x01;
	pInfo->firmwareVersion.minor = 0x00;
	memset(pInfo->utcTime, ' ', sizeof(pInfo->utcTime));

	return CKR_OK;
}


CK_DEFINE_FUNCTION(CK_RV, C_GetMechanismList)(CK_SLOT_ID slotID, CK_MECHANISM_TYPE_PTR pMechanismList, CK_ULONG_PTR pulCount)
{
	if (CK_FALSE == pkcs11_mock_initialized)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	if (PKCS11_MOCK_CK_SLOT_ID != slotID)
		return CKR_SLOT_ID_INVALID;

	if (NULL == pulCount)
		return CKR_ARGUMENTS_BAD;

	if (NULL == pMechanismList)
	{
		*pulCount = 9;
	}
	else
	{
		if (9 > *pulCount)
			return CKR_BUFFER_TOO_SMALL;

		pMechanismList[0] = CKM_RSA_PKCS_KEY_PAIR_GEN;
		pMechanismList[1] = CKM_RSA_PKCS;
		pMechanismList[2] = CKM_SHA1_RSA_PKCS;
		pMechanismList[3] = CKM_RSA_PKCS_OAEP;
		pMechanismList[4] = CKM_DES3_CBC;
		pMechanismList[5] = CKM_DES3_KEY_GEN;
		pMechanismList[6] = CKM_SHA_1;
		pMechanismList[7] = CKM_XOR_BASE_AND_DATA;
		pMechanismList[8] = CKM_AES_CBC;

		*pulCount = 9;
	}

	return CKR_OK;
}


CK_DEFINE_FUNCTION(CK_RV, C_GetMechanismInfo)(CK_SLOT_ID slotID, CK_MECHANISM_TYPE type, CK_MECHANISM_INFO_PTR pInfo)
{
	if (CK_FALSE == pkcs11_mock_initialized)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	if (PKCS11_MOCK_CK_SLOT_ID != slotID)
		return CKR_SLOT_ID_INVALID;

	if (NULL == pInfo)
		return CKR_ARGUMENTS_BAD;

	switch (type)
	{
		case CKM_RSA_PKCS_KEY_PAIR_GEN:
			pInfo->ulMinKeySize = 1024;
			pInfo->ulMaxKeySize = 1024;
			pInfo->flags = CKF_GENERATE_KEY_PAIR;
			break;

		case CKM_RSA_PKCS:
			pInfo->ulMinKeySize = 1024;
			pInfo->ulMaxKeySize = 1024;
			pInfo->flags = CKF_ENCRYPT | CKF_DECRYPT | CKF_SIGN | CKF_SIGN_RECOVER | CKF_VERIFY | CKF_VERIFY_RECOVER | CKF_WRAP | CKF_UNWRAP;
			break;

		case CKM_SHA1_RSA_PKCS:
			pInfo->ulMinKeySize = 1024;
			pInfo->ulMaxKeySize = 1024;
			pInfo->flags = CKF_SIGN | CKF_VERIFY;
			break;

		case CKM_RSA_PKCS_OAEP:
			pInfo->ulMinKeySize = 1024;
			pInfo->ulMaxKeySize = 1024;
			pInfo->flags = CKF_ENCRYPT | CKF_DECRYPT;
			break;

		case CKM_DES3_CBC:
			pInfo->ulMinKeySize = 192;
			pInfo->ulMaxKeySize = 192;
			pInfo->flags = CKF_ENCRYPT | CKF_DECRYPT;
			break;

		case CKM_DES3_KEY_GEN:
			pInfo->ulMinKeySize = 192;
			pInfo->ulMaxKeySize = 192;
			pInfo->flags = CKF_GENERATE;
			break;

		case CKM_SHA_1:
			pInfo->ulMinKeySize = 0;
			pInfo->ulMaxKeySize = 0;
			pInfo->flags = CKF_DIGEST;
			break;

		case CKM_XOR_BASE_AND_DATA:
			pInfo->ulMinKeySize = 128;
			pInfo->ulMaxKeySize = 256;
			pInfo->flags = CKF_DERIVE;
			break;

		case CKM_AES_CBC:
			pInfo->ulMinKeySize = 128;
			pInfo->ulMaxKeySize = 256;
			pInfo->flags = CKF_ENCRYPT | CKF_DECRYPT;
			break;

		default:
			return CKR_MECHANISM_INVALID;
	}

	return CKR_OK;
}


CK_DEFINE_FUNCTION(CK_RV, C_InitToken)(CK_SLOT_ID slotID, CK_UTF8CHAR_PTR pPin, CK_ULONG ulPinLen, CK_UTF8CHAR_PTR pLabel)
{
	if (CK_FALSE == pkcs11_mock_initialized)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	if (PKCS11_MOCK_CK_SLOT_ID != slotID)
		return CKR_SLOT_ID_INVALID;

	if (NULL == pPin)
		return CKR_ARGUMENTS_BAD;

	if ((ulPinLen < PKCS11_MOCK_CK_TOKEN_INFO_MIN_PIN_LEN) || (ulPinLen > PKCS11_MOCK_CK_TOKEN_INFO_MAX_PIN_LEN))
		return CKR_PIN_LEN_RANGE;

	if (NULL == pLabel)
		return CKR_ARGUMENTS_BAD;

	if (CK_TRUE == pkcs11_mock_session_opened)
		return CKR_SESSION_EXISTS;

	return CKR_OK;
}


CK_DEFINE_FUNCTION(CK_RV, C_InitPIN)(CK_SESSION_HANDLE hSession, CK_UTF8CHAR_PTR pPin, CK_ULONG ulPinLen)
{
	if (CK_FALSE == pkcs11_mock_initialized)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	if ((CK_FALSE == pkcs11_mock_session_opened) || (PKCS11_MOCK_CK_SESSION_ID != hSession))
		return CKR_SESSION_HANDLE_INVALID;

	if (CKS_RW_SO_FUNCTIONS != mock_session->state)
		return CKR_USER_NOT_LOGGED_IN;

	if (NULL == pPin)
		return CKR_ARGUMENTS_BAD;

	if ((ulPinLen < PKCS11_MOCK_CK_TOKEN_INFO_MIN_PIN_LEN) || (ulPinLen > PKCS11_MOCK_CK_TOKEN_INFO_MAX_PIN_LEN))
		return CKR_PIN_LEN_RANGE;

	return CKR_OK;
}


CK_DEFINE_FUNCTION(CK_RV, C_SetPIN)(CK_SESSION_HANDLE hSession, CK_UTF8CHAR_PTR pOldPin, CK_ULONG ulOldLen, CK_UTF8CHAR_PTR pNewPin, CK_ULONG ulNewLen)
{
	if (CK_FALSE == pkcs11_mock_initialized)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	if ((CK_FALSE == pkcs11_mock_session_opened) || (PKCS11_MOCK_CK_SESSION_ID != hSession))
		return CKR_SESSION_HANDLE_INVALID;

	if ((CKS_RO_PUBLIC_SESSION == mock_session->state) || (CKS_RO_USER_FUNCTIONS == mock_session->state))
		return CKR_SESSION_READ_ONLY;

	if (NULL == pOldPin)
		return CKR_ARGUMENTS_BAD;

	if ((ulOldLen < PKCS11_MOCK_CK_TOKEN_INFO_MIN_PIN_LEN) || (ulOldLen > PKCS11_MOCK_CK_TOKEN_INFO_MAX_PIN_LEN))
		return CKR_PIN_LEN_RANGE;

	if (NULL == pNewPin)
		return CKR_ARGUMENTS_BAD;

	if ((ulNewLen < PKCS11_MOCK_CK_TOKEN_INFO_MIN_PIN_LEN) || (ulNewLen > PKCS11_MOCK_CK_TOKEN_INFO_MAX_PIN_LEN))
		return CKR_PIN_LEN_RANGE;

	return CKR_OK;
}


CK_DEFINE_FUNCTION(CK_RV, C_OpenSession)(CK_SLOT_ID slotID, CK_FLAGS flags, CK_VOID_PTR pApplication, CK_NOTIFY Notify, CK_SESSION_HANDLE_PTR phSession)
{
	if (CK_FALSE == pkcs11_mock_initialized)
		return CKR_CRYPTOKI_NOT_INITIALIZED;
	if (CK_TRUE == pkcs11_mock_session_opened)
		return CKR_SESSION_COUNT;

	if (PKCS11_MOCK_CK_SLOT_ID != slotID)
		return CKR_SLOT_ID_INVALID;

	if (!(flags & CKF_SERIAL_SESSION))
		return CKR_SESSION_PARALLEL_NOT_SUPPORTED;

	IGNORE(pApplication);

	IGNORE(Notify);

	if (NULL == phSession)
		return CKR_ARGUMENTS_BAD;

	pkcs11_mock_session_opened = CK_TRUE;

	mock_session = calloc(1, sizeof(session_ptr_st));
	if (mock_session == NULL)
		return CKR_HOST_MEMORY;

	mock_session->state = (flags & CKF_RW_SESSION) ? CKS_RW_PUBLIC_SESSION : CKS_RO_PUBLIC_SESSION;

	mock_session->find_op.find_result = CKR_OBJECT_HANDLE_INVALID;
	mock_session->find_op.active_operation = PKCS11_MOCK_CK_OPERATION_NONE;
	mock_session->state = CKS_RO_PUBLIC_SESSION;

	*phSession = PKCS11_MOCK_CK_SESSION_ID;

	return CKR_OK;
}


CK_DEFINE_FUNCTION(CK_RV, C_CloseSession)(CK_SESSION_HANDLE hSession)
{
	if (CK_FALSE == pkcs11_mock_initialized)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	if ((CK_FALSE == pkcs11_mock_session_opened) || (PKCS11_MOCK_CK_SESSION_ID != hSession))
		return CKR_SESSION_HANDLE_INVALID;

	pkcs11_mock_session_opened = CK_FALSE;
	mock_session->state = CKS_RO_PUBLIC_SESSION;

	mock_session->find_op.active_operation = PKCS11_MOCK_CK_OPERATION_NONE;
	free(mock_session->find_label);
	free(mock_session);
	mock_session = NULL;

	return CKR_OK;
}


CK_DEFINE_FUNCTION(CK_RV, C_CloseAllSessions)(CK_SLOT_ID slotID)
{
	return C_CloseSession(PKCS11_MOCK_CK_SESSION_ID);
}


CK_DEFINE_FUNCTION(CK_RV, C_GetSessionInfo)(CK_SESSION_HANDLE hSession, CK_SESSION_INFO_PTR pInfo)
{
	if (CK_FALSE == pkcs11_mock_initialized)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	if ((CK_FALSE == pkcs11_mock_session_opened) || (PKCS11_MOCK_CK_SESSION_ID != hSession))
		return CKR_SESSION_HANDLE_INVALID;

	if (NULL == pInfo)
		return CKR_ARGUMENTS_BAD;

	pInfo->slotID = PKCS11_MOCK_CK_SLOT_ID;
	pInfo->state = mock_session->state;
	pInfo->flags = CKF_SERIAL_SESSION;
	if ((mock_session->state != CKS_RO_PUBLIC_SESSION) && (mock_session->state != CKS_RO_USER_FUNCTIONS))
		pInfo->flags = pInfo->flags | CKF_RW_SESSION;
	pInfo->ulDeviceError = 0;

	return CKR_OK;
}


CK_DEFINE_FUNCTION(CK_RV, C_GetOperationState)(CK_SESSION_HANDLE hSession, CK_BYTE_PTR pOperationState, CK_ULONG_PTR pulOperationStateLen)
{
	if (CK_FALSE == pkcs11_mock_initialized)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	if ((CK_FALSE == pkcs11_mock_session_opened) || (PKCS11_MOCK_CK_SESSION_ID != hSession))
		return CKR_SESSION_HANDLE_INVALID;

	if (NULL == pulOperationStateLen)
		return CKR_ARGUMENTS_BAD;

	if (NULL == pOperationState)
	{
		*pulOperationStateLen = 256;
	}
	else
	{
		if (256 > *pulOperationStateLen)
			return CKR_BUFFER_TOO_SMALL;

		memset(pOperationState, 1, 256);
		*pulOperationStateLen = 256;
	}

	return CKR_OK;
}


CK_DEFINE_FUNCTION(CK_RV, C_SetOperationState)(CK_SESSION_HANDLE hSession, CK_BYTE_PTR pOperationState, CK_ULONG ulOperationStateLen, CK_OBJECT_HANDLE hEncryptionKey, CK_OBJECT_HANDLE hAuthenticationKey)
{
	if (CK_FALSE == pkcs11_mock_initialized)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	if ((CK_FALSE == pkcs11_mock_session_opened) || (PKCS11_MOCK_CK_SESSION_ID != hSession))
		return CKR_SESSION_HANDLE_INVALID;

	if (NULL == pOperationState)
		return CKR_ARGUMENTS_BAD;

	if (256 != ulOperationStateLen)
		return CKR_ARGUMENTS_BAD;

	IGNORE(hEncryptionKey);

	IGNORE(hAuthenticationKey);

	return CKR_OK;
}


CK_DEFINE_FUNCTION(CK_RV, C_Login)(CK_SESSION_HANDLE hSession, CK_USER_TYPE userType, CK_UTF8CHAR_PTR pPin, CK_ULONG ulPinLen)
{
	CK_RV rv = CKR_OK;

	if (CK_FALSE == pkcs11_mock_initialized)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	if ((CK_FALSE == pkcs11_mock_session_opened) || (PKCS11_MOCK_CK_SESSION_ID != hSession))
		return CKR_SESSION_HANDLE_INVALID;

	if ((pkcs11_mock_flags & MOCK_FLAG_SAFENET_ALWAYS_AUTH) && userType == CKU_CONTEXT_SPECIFIC) {
		return CKR_USER_TYPE_INVALID;
	}

	if ((pkcs11_mock_flags & MOCK_FLAG_ALWAYS_AUTH) || (pkcs11_mock_flags & MOCK_FLAG_SAFENET_ALWAYS_AUTH)) {
		if ((CKU_CONTEXT_SPECIFIC != userType) && (CKU_SO != userType) && (CKU_USER != userType))
			return CKR_USER_TYPE_INVALID;
	} else if ((CKU_SO != userType) && (CKU_USER != userType)) {
		return CKR_USER_TYPE_INVALID;
	}

	if (NULL == pPin)
		return CKR_ARGUMENTS_BAD;

	if ((ulPinLen < PKCS11_MOCK_CK_TOKEN_INFO_MIN_PIN_LEN) || (ulPinLen > PKCS11_MOCK_CK_TOKEN_INFO_MAX_PIN_LEN))
		return CKR_PIN_LEN_RANGE;

	switch (mock_session->state)
	{
		case CKS_RO_PUBLIC_SESSION:

			if (CKU_SO == userType)
				rv = CKR_SESSION_READ_ONLY_EXISTS;
			else
				mock_session->state = CKS_RO_USER_FUNCTIONS;

			break;

		case CKS_RO_USER_FUNCTIONS:
		case CKS_RW_USER_FUNCTIONS:

			rv = (CKU_SO == userType) ? CKR_USER_ANOTHER_ALREADY_LOGGED_IN : CKR_USER_ALREADY_LOGGED_IN;

			break;

		case CKS_RW_PUBLIC_SESSION:

			mock_session->state = (CKU_SO == userType) ? CKS_RW_SO_FUNCTIONS : CKS_RW_USER_FUNCTIONS;

			break;

		case CKS_RW_SO_FUNCTIONS:

			rv = (CKU_SO == userType) ? CKR_USER_ALREADY_LOGGED_IN : CKR_USER_ANOTHER_ALREADY_LOGGED_IN;

			break;
	}

	if ((pkcs11_mock_flags & MOCK_FLAG_ALWAYS_AUTH || pkcs11_mock_flags & MOCK_FLAG_SAFENET_ALWAYS_AUTH) && rv == CKR_USER_ALREADY_LOGGED_IN) {
		rv = 0;
	}

	pkcs11_mock_session_reauth = 1;
	return rv;
}


CK_DEFINE_FUNCTION(CK_RV, C_Logout)(CK_SESSION_HANDLE hSession)
{
	if (CK_FALSE == pkcs11_mock_initialized)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	if ((CK_FALSE == pkcs11_mock_session_opened) || (PKCS11_MOCK_CK_SESSION_ID != hSession))
		return CKR_SESSION_HANDLE_INVALID;

	if ((mock_session->state == CKS_RO_PUBLIC_SESSION) || (mock_session->state == CKS_RW_PUBLIC_SESSION))
		return CKR_USER_NOT_LOGGED_IN;

	return CKR_OK;
}


CK_DEFINE_FUNCTION(CK_RV, C_CreateObject)(CK_SESSION_HANDLE hSession, CK_ATTRIBUTE_PTR pTemplate, CK_ULONG ulCount, CK_OBJECT_HANDLE_PTR phObject)
{
	CK_ULONG i = 0;

	if (CK_FALSE == pkcs11_mock_initialized)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	if ((CK_FALSE == pkcs11_mock_session_opened) || (PKCS11_MOCK_CK_SESSION_ID != hSession))
		return CKR_SESSION_HANDLE_INVALID;

	if (NULL == pTemplate)
		return CKR_ARGUMENTS_BAD;

	if (0 >= ulCount)
		return CKR_ARGUMENTS_BAD;

	if (NULL == phObject)
		return CKR_ARGUMENTS_BAD;

	for (i = 0; i < ulCount; i++)
	{
		if (NULL == pTemplate[i].pValue)
			return CKR_ATTRIBUTE_VALUE_INVALID;

		if (0 >= pTemplate[i].ulValueLen)
			return CKR_ATTRIBUTE_VALUE_INVALID;
	}

	*phObject = PKCS11_MOCK_CK_OBJECT_HANDLE_DATA;

	return CKR_OK;
}


CK_DEFINE_FUNCTION(CK_RV, C_CopyObject)(CK_SESSION_HANDLE hSession, CK_OBJECT_HANDLE hObject, CK_ATTRIBUTE_PTR pTemplate, CK_ULONG ulCount, CK_OBJECT_HANDLE_PTR phNewObject)
{
	CK_ULONG i = 0;

	if (CK_FALSE == pkcs11_mock_initialized)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	if ((CK_FALSE == pkcs11_mock_session_opened) || (PKCS11_MOCK_CK_SESSION_ID != hSession))
		return CKR_SESSION_HANDLE_INVALID;

	if (PKCS11_MOCK_CK_OBJECT_HANDLE_DATA != hObject)
		return CKR_OBJECT_HANDLE_INVALID;

	if (NULL == phNewObject)
		return CKR_ARGUMENTS_BAD;

	if ((NULL != pTemplate) && (0 >= ulCount))
	{
		for (i = 0; i < ulCount; i++)
		{
			if (NULL == pTemplate[i].pValue)
				return CKR_ATTRIBUTE_VALUE_INVALID;

			if (0 >= pTemplate[i].ulValueLen)
				return CKR_ATTRIBUTE_VALUE_INVALID;
		}
	}

	*phNewObject = PKCS11_MOCK_CK_OBJECT_HANDLE_DATA;

	return CKR_OK;
}


CK_DEFINE_FUNCTION(CK_RV, C_DestroyObject)(CK_SESSION_HANDLE hSession, CK_OBJECT_HANDLE hObject)
{
	if (CK_FALSE == pkcs11_mock_initialized)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	if ((CK_FALSE == pkcs11_mock_session_opened) || (PKCS11_MOCK_CK_SESSION_ID != hSession))
		return CKR_SESSION_HANDLE_INVALID;

	if ((PKCS11_MOCK_CK_OBJECT_HANDLE_DATA != hObject) &&
		(PKCS11_MOCK_CK_OBJECT_HANDLE_SECRET_KEY != hObject) &&
		(PKCS11_MOCK_CK_OBJECT_HANDLE_PUBLIC_KEY != hObject) &&
		(PKCS11_MOCK_CK_OBJECT_HANDLE_PRIVATE_KEY != hObject))
		return CKR_OBJECT_HANDLE_INVALID;

	return CKR_OK;
}


CK_DEFINE_FUNCTION(CK_RV, C_GetObjectSize)(CK_SESSION_HANDLE hSession, CK_OBJECT_HANDLE hObject, CK_ULONG_PTR pulSize)
{
	if (CK_FALSE == pkcs11_mock_initialized)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	if ((CK_FALSE == pkcs11_mock_session_opened) || (PKCS11_MOCK_CK_SESSION_ID != hSession))
		return CKR_SESSION_HANDLE_INVALID;

	if ((PKCS11_MOCK_CK_OBJECT_HANDLE_DATA != hObject) &&
		(PKCS11_MOCK_CK_OBJECT_HANDLE_SECRET_KEY != hObject) &&
		(PKCS11_MOCK_CK_OBJECT_HANDLE_PUBLIC_KEY != hObject) &&
		(PKCS11_MOCK_CK_OBJECT_HANDLE_PRIVATE_KEY != hObject))
		return CKR_OBJECT_HANDLE_INVALID;

	if (NULL == pulSize)
		return CKR_ARGUMENTS_BAD;

	*pulSize = PKCS11_MOCK_CK_OBJECT_SIZE;

	return CKR_OK;
}


CK_DEFINE_FUNCTION(CK_RV, C_GetAttributeValue)(CK_SESSION_HANDLE hSession, CK_OBJECT_HANDLE hObject, CK_ATTRIBUTE_PTR pTemplate, CK_ULONG ulCount)
{
	CK_ULONG i = 0;

	if (CK_FALSE == pkcs11_mock_initialized)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	if ((CK_FALSE == pkcs11_mock_session_opened) || (PKCS11_MOCK_CK_SESSION_ID != hSession))
		return CKR_SESSION_HANDLE_INVALID;

	if ((PKCS11_MOCK_CK_OBJECT_HANDLE_DATA != hObject) &&
		(PKCS11_MOCK_CK_OBJECT_HANDLE_SECRET_KEY != hObject) &&
		(PKCS11_MOCK_CK_OBJECT_HANDLE_CERTIFICATE_EXTENSION != hObject) &&
		(PKCS11_MOCK_CK_OBJECT_HANDLE_CERTIFICATE != hObject) &&
		(PKCS11_MOCK_CK_OBJECT_HANDLE_PUBLIC_KEY != hObject) &&
		(PKCS11_MOCK_CK_OBJECT_HANDLE_PRIVATE_KEY != hObject))
		return CKR_OBJECT_HANDLE_INVALID;

	if (NULL == pTemplate)
		return CKR_ARGUMENTS_BAD;

	if (0 >= ulCount)
		return CKR_ARGUMENTS_BAD;

	for (i = 0; i < ulCount; i++)
	{
		if (CKA_PUBLIC_KEY_INFO == pTemplate[i].type && 
		    (PKCS11_MOCK_CK_OBJECT_HANDLE_CERTIFICATE == hObject || PKCS11_MOCK_CK_OBJECT_HANDLE_PUBLIC_KEY == hObject))
		{
			if (pTemplate[i].ulValueLen < sizeof(mock_pubkey)-1) {
				pTemplate[i].ulValueLen = sizeof(mock_pubkey)-1;
				if (pTemplate[i].pValue == NULL)
					return CKR_OK;
				else
					return CKR_BUFFER_TOO_SMALL;
			}
			pTemplate[i].ulValueLen = (CK_ULONG) sizeof(mock_pubkey)-1;
			memcpy(pTemplate[i].pValue, mock_pubkey, pTemplate[i].ulValueLen);
		}
		else if (CKA_CLASS == pTemplate[i].type)
		{
			if (NULL != pTemplate[i].pValue)
			{
				if (pTemplate[i].ulValueLen < sizeof(hObject))
					return CKR_BUFFER_TOO_SMALL;
				else
					memcpy(pTemplate[i].pValue, &hObject, sizeof(hObject));
			}

			pTemplate[i].ulValueLen = sizeof(hObject);
		}
		else if (CKA_PUBLIC_EXPONENT == pTemplate[i].type &&
			(PKCS11_MOCK_CK_OBJECT_HANDLE_PUBLIC_KEY == hObject || PKCS11_MOCK_CK_OBJECT_HANDLE_PRIVATE_KEY == hObject))
		{
			if (NULL != pTemplate[i].pValue)
			{
				if (pTemplate[i].ulValueLen < sizeof(mock_public_exponent)-1)
					return CKR_BUFFER_TOO_SMALL;
				else
					memcpy(pTemplate[i].pValue, mock_public_exponent, sizeof(mock_public_exponent)-1);
			}

			pTemplate[i].ulValueLen = sizeof(mock_public_exponent)-1;
		}
		else if (CKA_MODULUS == pTemplate[i].type &&
			(PKCS11_MOCK_CK_OBJECT_HANDLE_PUBLIC_KEY == hObject || PKCS11_MOCK_CK_OBJECT_HANDLE_PRIVATE_KEY == hObject))
		{
			if (NULL != pTemplate[i].pValue)
			{
				if (pTemplate[i].ulValueLen < sizeof(mock_modulus)-1)
					return CKR_BUFFER_TOO_SMALL;
				else
					memcpy(pTemplate[i].pValue, mock_modulus, sizeof(mock_modulus)-1);
			}

			pTemplate[i].ulValueLen = sizeof(mock_modulus)-1;
		}
		else if (CKA_SUBJECT == pTemplate[i].type && PKCS11_MOCK_CK_OBJECT_HANDLE_CERTIFICATE == hObject)
		{
			if (NULL != pTemplate[i].pValue)
			{
				if (pTemplate[i].ulValueLen < strlen(mock_subject))
					return CKR_BUFFER_TOO_SMALL;
				else
					memcpy(pTemplate[i].pValue, mock_subject, strlen(mock_subject));
			}

			pTemplate[i].ulValueLen = strlen(mock_subject);
		}
		else if (CKA_LABEL == pTemplate[i].type)
		{
			if (NULL != pTemplate[i].pValue)
			{
				if (pTemplate[i].ulValueLen < strlen(PKCS11_MOCK_CK_OBJECT_CKA_LABEL))
					return CKR_BUFFER_TOO_SMALL;
				else
					memcpy(pTemplate[i].pValue, PKCS11_MOCK_CK_OBJECT_CKA_LABEL, strlen(PKCS11_MOCK_CK_OBJECT_CKA_LABEL));
			}

			pTemplate[i].ulValueLen = strlen(PKCS11_MOCK_CK_OBJECT_CKA_LABEL);
		}
		else if (CKA_KEY_TYPE == pTemplate[i].type)
		{
			CK_KEY_TYPE t;
			if (pTemplate[i].ulValueLen != sizeof(CK_KEY_TYPE))
				return CKR_ARGUMENTS_BAD;

			t = CKK_RSA;
			memcpy(pTemplate[i].pValue, &t, sizeof(CK_KEY_TYPE));
		}
		else if (CKA_ALWAYS_AUTHENTICATE == pTemplate[i].type)
		{
			CK_BBOOL t;
			if (pkcs11_mock_flags & MOCK_FLAG_SAFENET_ALWAYS_AUTH)
				return CKR_ATTRIBUTE_TYPE_INVALID;

			if (pTemplate[i].ulValueLen != sizeof(CK_BBOOL))
				return CKR_ARGUMENTS_BAD;

			if (!(pkcs11_mock_flags & MOCK_FLAG_ALWAYS_AUTH)) {
				t = CK_FALSE;
			} else {
				t = CK_TRUE;
			}
			memcpy(pTemplate[i].pValue, &t, sizeof(CK_BBOOL));
		}
		else if (CKA_ID == pTemplate[i].type)
		{
			if (NULL != pTemplate[i].pValue)
			{
				if (pTemplate[i].ulValueLen < strlen(PKCS11_MOCK_CK_OBJECT_CKA_LABEL))
					return CKR_BUFFER_TOO_SMALL;
				else
					memcpy(pTemplate[i].pValue, PKCS11_MOCK_CK_OBJECT_CKA_LABEL, strlen(PKCS11_MOCK_CK_OBJECT_CKA_LABEL));
			}

			pTemplate[i].ulValueLen = strlen(PKCS11_MOCK_CK_OBJECT_CKA_LABEL);
		}
		else if (CKA_CERTIFICATE_CATEGORY == pTemplate[i].type)
		{
			CK_ULONG t = 2; /* authority */
			if (pTemplate[i].ulValueLen < sizeof(CK_ULONG))
				return CKR_BUFFER_TOO_SMALL;
			memcpy(pTemplate[i].pValue, &t, sizeof(CK_ULONG));
		}
		else if (CKA_VALUE == pTemplate[i].type)
		{
			if (PKCS11_MOCK_CK_OBJECT_HANDLE_CERTIFICATE_EXTENSION == hObject)
			{
				const void *obj;
				unsigned obj_len;

				if (mock_session->find_op.remaining_data == 1) {
					obj = mock_cert_ext1;
					obj_len = sizeof(mock_cert_ext1)-1;
				} else {
					obj = mock_cert_ext2;
					obj_len = sizeof(mock_cert_ext2)-1;
				}

				if (pTemplate[i].ulValueLen < obj_len) {
					pTemplate[i].ulValueLen = obj_len;
					if (pTemplate[i].pValue == NULL)
						return CKR_OK;
					else
						return CKR_BUFFER_TOO_SMALL;
				}
				pTemplate[i].ulValueLen = (CK_ULONG) obj_len;
				memcpy(pTemplate[i].pValue, obj, pTemplate[i].ulValueLen);
			}
			else if (PKCS11_MOCK_CK_OBJECT_HANDLE_CERTIFICATE == hObject)
			{
				if (pTemplate[i].ulValueLen < sizeof(mock_certificate)-1) {
					pTemplate[i].ulValueLen = sizeof(mock_certificate)-1;
					if (pTemplate[i].pValue == NULL)
						return CKR_OK;
					else
						return CKR_BUFFER_TOO_SMALL;
				}
				pTemplate[i].ulValueLen = (CK_ULONG) sizeof(mock_certificate)-1;
				memcpy(pTemplate[i].pValue, mock_certificate, pTemplate[i].ulValueLen);
			}
			else if (PKCS11_MOCK_CK_OBJECT_HANDLE_PUBLIC_KEY == hObject)
			{
				if (pTemplate[i].ulValueLen < sizeof(mock_pubkey)-1) {
					pTemplate[i].ulValueLen = sizeof(mock_pubkey)-1;
					if (pTemplate[i].pValue == NULL)
						return CKR_OK;
					else
						return CKR_BUFFER_TOO_SMALL;
				}
				pTemplate[i].ulValueLen = (CK_ULONG) sizeof(mock_pubkey)-1;
				memcpy(pTemplate[i].pValue, mock_pubkey, pTemplate[i].ulValueLen);
			}
			else if (PKCS11_MOCK_CK_OBJECT_HANDLE_PRIVATE_KEY == hObject)
			{
				pTemplate[i].ulValueLen = (CK_ULONG) -1;
				if (!(pkcs11_mock_flags & MOCK_FLAG_BROKEN_GET_ATTRIBUTES)) {
					return CKR_ATTRIBUTE_SENSITIVE;
				}
			}
			else
			{
				if (NULL != pTemplate[i].pValue)
				{
					if (pTemplate[i].ulValueLen < strlen(PKCS11_MOCK_CK_OBJECT_CKA_VALUE))
						return CKR_BUFFER_TOO_SMALL;
					else
						memcpy(pTemplate[i].pValue, PKCS11_MOCK_CK_OBJECT_CKA_VALUE, strlen(PKCS11_MOCK_CK_OBJECT_CKA_VALUE));
				}

				pTemplate[i].ulValueLen = strlen(PKCS11_MOCK_CK_OBJECT_CKA_VALUE);
			}
		}
		else
		{
			return CKR_ATTRIBUTE_TYPE_INVALID;
		}
	}

	return CKR_OK;
}


CK_DEFINE_FUNCTION(CK_RV, C_SetAttributeValue)(CK_SESSION_HANDLE hSession, CK_OBJECT_HANDLE hObject, CK_ATTRIBUTE_PTR pTemplate, CK_ULONG ulCount)
{
	CK_ULONG i = 0;

	if (CK_FALSE == pkcs11_mock_initialized)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	if ((CK_FALSE == pkcs11_mock_session_opened) || (PKCS11_MOCK_CK_SESSION_ID != hSession))
		return CKR_SESSION_HANDLE_INVALID;

	if ((PKCS11_MOCK_CK_OBJECT_HANDLE_DATA != hObject) &&
		(PKCS11_MOCK_CK_OBJECT_HANDLE_SECRET_KEY != hObject) &&
		(PKCS11_MOCK_CK_OBJECT_HANDLE_PUBLIC_KEY != hObject) &&
		(PKCS11_MOCK_CK_OBJECT_HANDLE_PRIVATE_KEY != hObject))
		return CKR_OBJECT_HANDLE_INVALID;

	if (NULL == pTemplate)
		return CKR_ARGUMENTS_BAD;

	if (0 >= ulCount)
		return CKR_ARGUMENTS_BAD;

	for (i = 0; i < ulCount; i++)
	{
		if ((CKA_LABEL == pTemplate[i].type) || (CKA_VALUE == pTemplate[i].type))
		{
			if (NULL == pTemplate[i].pValue)
				return CKR_ATTRIBUTE_VALUE_INVALID;

			if (0 >= pTemplate[i].ulValueLen)
				return CKR_ATTRIBUTE_VALUE_INVALID;
		}
		else
		{
			return CKR_ATTRIBUTE_TYPE_INVALID;
		}
	}

	return CKR_OK;
}


CK_DEFINE_FUNCTION(CK_RV, C_FindObjectsInit)(CK_SESSION_HANDLE hSession, CK_ATTRIBUTE_PTR pTemplate, CK_ULONG ulCount)
{
	CK_ULONG i = 0;
	CK_ULONG_PTR cka_class_value = NULL;

	if (CK_FALSE == pkcs11_mock_initialized)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	if (PKCS11_MOCK_CK_OPERATION_NONE != mock_session->find_op.active_operation)
		return CKR_OPERATION_ACTIVE;

	if ((CK_FALSE == pkcs11_mock_session_opened) || (PKCS11_MOCK_CK_SESSION_ID != hSession))
		return CKR_SESSION_HANDLE_INVALID;

	if (NULL == pTemplate)
		return CKR_ARGUMENTS_BAD;

	IGNORE(ulCount);

	mock_session->find_op.find_result = CK_INVALID_HANDLE;

	for (i = 0; i < ulCount; i++)
	{
		if (NULL == pTemplate[i].pValue)
			return CKR_ATTRIBUTE_VALUE_INVALID;

		if (0 >= pTemplate[i].ulValueLen)
			return CKR_ATTRIBUTE_VALUE_INVALID;

		if (CKA_LABEL == pTemplate[i].type)
		{
			free(mock_session->find_label);
			mock_session->find_label = strndup(pTemplate[i].pValue, pTemplate[i].ulValueLen);
		}
		else if (CKA_CLASS == pTemplate[i].type)
		{
			if (sizeof(CK_ULONG) != pTemplate[i].ulValueLen)
				return CKR_ATTRIBUTE_VALUE_INVALID;

			cka_class_value = (CK_ULONG_PTR) pTemplate[i].pValue;

			switch (*cka_class_value)
			{
				case CKO_DATA:
					mock_session->find_op.find_result = PKCS11_MOCK_CK_OBJECT_HANDLE_DATA;
					mock_session->find_op.remaining_data = 2;
					break;
				case CKO_SECRET_KEY:
					mock_session->find_op.find_result = PKCS11_MOCK_CK_OBJECT_HANDLE_SECRET_KEY;
					mock_session->find_op.remaining_data = 1;
					break;
				case CKO_CERTIFICATE:
					mock_session->find_op.find_result = PKCS11_MOCK_CK_OBJECT_HANDLE_CERTIFICATE;
					mock_session->find_op.remaining_data = 1;
					break;
				case CKO_PUBLIC_KEY:
					mock_session->find_op.find_result = PKCS11_MOCK_CK_OBJECT_HANDLE_PUBLIC_KEY;
					mock_session->find_op.remaining_data = 1;
					break;
				case CKO_PRIVATE_KEY:
					mock_session->find_op.find_result = PKCS11_MOCK_CK_OBJECT_HANDLE_PRIVATE_KEY;
					mock_session->find_op.remaining_data = 1;
					break;
				case CKO_X_CERTIFICATE_EXTENSION:
					mock_session->find_op.find_result = PKCS11_MOCK_CK_OBJECT_HANDLE_CERTIFICATE_EXTENSION;
					mock_session->find_op.remaining_data = 2;
					break;
			}
		}
	}

	mock_session->find_op.active_operation = PKCS11_MOCK_CK_OPERATION_FIND;

	return CKR_OK;
}


CK_DEFINE_FUNCTION(CK_RV, C_FindObjects)(CK_SESSION_HANDLE hSession, CK_OBJECT_HANDLE_PTR phObject, CK_ULONG ulMaxObjectCount, CK_ULONG_PTR pulObjectCount)
{

	if (CK_FALSE == pkcs11_mock_initialized)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	if (PKCS11_MOCK_CK_OPERATION_FIND != mock_session->find_op.active_operation)
		return CKR_OPERATION_NOT_INITIALIZED;

	if ((CK_FALSE == pkcs11_mock_session_opened) || (PKCS11_MOCK_CK_SESSION_ID != hSession))
		return CKR_SESSION_HANDLE_INVALID;

	if ((NULL == phObject) && (0 < ulMaxObjectCount))
		return CKR_ARGUMENTS_BAD;

	if (NULL == pulObjectCount)
		return CKR_ARGUMENTS_BAD;

	if (mock_session->find_op.remaining_data <= 0) {
		*pulObjectCount = 0;
		return CKR_OK;
	}

	switch (mock_session->find_op.find_result)
	{
		case PKCS11_MOCK_CK_OBJECT_HANDLE_DATA:
			
			if (ulMaxObjectCount >= 2)
			{
				phObject[0] = mock_session->find_op.find_result;
				phObject[1] = mock_session->find_op.find_result;
			}

			*pulObjectCount = 2;
			mock_session->find_op.remaining_data -= 2;

			break;

		case CK_INVALID_HANDLE:
			
			*pulObjectCount = 0;

			break;

		default:

			if (ulMaxObjectCount >= 1)
			{
				phObject[0] = mock_session->find_op.find_result;
			}

			*pulObjectCount = 1;
			mock_session->find_op.remaining_data --;

			break;
	}

	return CKR_OK;
}


CK_DEFINE_FUNCTION(CK_RV, C_FindObjectsFinal)(CK_SESSION_HANDLE hSession)
{
	if (CK_FALSE == pkcs11_mock_initialized)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	if (PKCS11_MOCK_CK_OPERATION_FIND != mock_session->find_op.active_operation)
		return CKR_OPERATION_NOT_INITIALIZED;

	if ((CK_FALSE == pkcs11_mock_session_opened) || (PKCS11_MOCK_CK_SESSION_ID != hSession))
		return CKR_SESSION_HANDLE_INVALID;

	mock_session->find_op.active_operation = PKCS11_MOCK_CK_OPERATION_NONE;

	return CKR_OK;
}


CK_DEFINE_FUNCTION(CK_RV, C_EncryptInit)(CK_SESSION_HANDLE hSession, CK_MECHANISM_PTR pMechanism, CK_OBJECT_HANDLE hKey)
{
	if (CK_FALSE == pkcs11_mock_initialized)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	if ((PKCS11_MOCK_CK_OPERATION_NONE != mock_session->find_op.active_operation) &&
		(PKCS11_MOCK_CK_OPERATION_DIGEST != mock_session->find_op.active_operation) && 
		(PKCS11_MOCK_CK_OPERATION_SIGN != mock_session->find_op.active_operation))
		return CKR_OPERATION_ACTIVE;

	if ((CK_FALSE == pkcs11_mock_session_opened) || (PKCS11_MOCK_CK_SESSION_ID != hSession))
		return CKR_SESSION_HANDLE_INVALID;

	if (NULL == pMechanism)
		return CKR_ARGUMENTS_BAD;

	switch (pMechanism->mechanism)
	{
		case CKM_RSA_PKCS:

			if ((NULL != pMechanism->pParameter) || (0 != pMechanism->ulParameterLen))
				return CKR_MECHANISM_PARAM_INVALID;

			if (PKCS11_MOCK_CK_OBJECT_HANDLE_PUBLIC_KEY != hKey)
				return CKR_KEY_TYPE_INCONSISTENT;

			break;

#if 0
		case CKM_RSA_PKCS_OAEP:

			if ((NULL == pMechanism->pParameter) || (sizeof(CK_RSA_PKCS_OAEP_PARAMS) != pMechanism->ulParameterLen))
				return CKR_MECHANISM_PARAM_INVALID;

			if (PKCS11_MOCK_CK_OBJECT_HANDLE_PUBLIC_KEY != hKey)
				return CKR_KEY_TYPE_INCONSISTENT;

			break;
#endif
		case CKM_DES3_CBC:

			if ((NULL == pMechanism->pParameter) || (8 != pMechanism->ulParameterLen))
				return CKR_MECHANISM_PARAM_INVALID;

			if (PKCS11_MOCK_CK_OBJECT_HANDLE_SECRET_KEY != hKey)
				return CKR_KEY_TYPE_INCONSISTENT;

			break;

		case CKM_AES_CBC:
			
			if ((NULL == pMechanism->pParameter) || (16 != pMechanism->ulParameterLen))
				return CKR_MECHANISM_PARAM_INVALID;

			if (PKCS11_MOCK_CK_OBJECT_HANDLE_SECRET_KEY != hKey)
				return CKR_KEY_TYPE_INCONSISTENT;

			break;

		default:

			return CKR_MECHANISM_INVALID;
	}

	switch (mock_session->find_op.active_operation)
	{
		case PKCS11_MOCK_CK_OPERATION_NONE:
			mock_session->find_op.active_operation = PKCS11_MOCK_CK_OPERATION_ENCRYPT;
			break;
		case PKCS11_MOCK_CK_OPERATION_DIGEST:
			mock_session->find_op.active_operation = PKCS11_MOCK_CK_OPERATION_DIGEST_ENCRYPT;
			break;
		case PKCS11_MOCK_CK_OPERATION_SIGN:
			mock_session->find_op.active_operation = PKCS11_MOCK_CK_OPERATION_SIGN_ENCRYPT;
			break;
		default:
			return CKR_FUNCTION_FAILED;
	}

	return CKR_OK;
}


CK_DEFINE_FUNCTION(CK_RV, C_Encrypt)(CK_SESSION_HANDLE hSession, CK_BYTE_PTR pData, CK_ULONG ulDataLen, CK_BYTE_PTR pEncryptedData, CK_ULONG_PTR pulEncryptedDataLen)
{
	CK_ULONG i = 0;

	if (CK_FALSE == pkcs11_mock_initialized)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	if (PKCS11_MOCK_CK_OPERATION_ENCRYPT != mock_session->find_op.active_operation)
		return CKR_OPERATION_NOT_INITIALIZED;

	if ((CK_FALSE == pkcs11_mock_session_opened) || (PKCS11_MOCK_CK_SESSION_ID != hSession))
		return CKR_SESSION_HANDLE_INVALID;

	if (NULL == pData)
		return CKR_ARGUMENTS_BAD;

	if (0 >= ulDataLen)
		return CKR_ARGUMENTS_BAD;

	if (NULL == pulEncryptedDataLen)
		return CKR_ARGUMENTS_BAD;

	if (NULL != pEncryptedData)
	{
		if (ulDataLen > *pulEncryptedDataLen)
		{
			return CKR_BUFFER_TOO_SMALL;
		}
		else
		{
			for (i = 0; i < ulDataLen; i++)
				pEncryptedData[i] = pData[i] ^ 0xAB;

			mock_session->find_op.active_operation = PKCS11_MOCK_CK_OPERATION_NONE;
		}
	}

	*pulEncryptedDataLen = ulDataLen;

	return CKR_OK;
}


CK_DEFINE_FUNCTION(CK_RV, C_EncryptUpdate)(CK_SESSION_HANDLE hSession, CK_BYTE_PTR pPart, CK_ULONG ulPartLen, CK_BYTE_PTR pEncryptedPart, CK_ULONG_PTR pulEncryptedPartLen)
{
	CK_ULONG i = 0;

	if (CK_FALSE == pkcs11_mock_initialized)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	if (PKCS11_MOCK_CK_OPERATION_ENCRYPT != mock_session->find_op.active_operation)
		return CKR_OPERATION_NOT_INITIALIZED;

	if ((CK_FALSE == pkcs11_mock_session_opened) || (PKCS11_MOCK_CK_SESSION_ID != hSession))
		return CKR_SESSION_HANDLE_INVALID;

	if (NULL == pPart)
		return CKR_ARGUMENTS_BAD;

	if (0 >= ulPartLen)
		return CKR_ARGUMENTS_BAD;

	if (NULL == pulEncryptedPartLen)
		return CKR_ARGUMENTS_BAD;

	if (NULL != pEncryptedPart)
	{
		if (ulPartLen > *pulEncryptedPartLen)
		{
			return CKR_BUFFER_TOO_SMALL;
		}
		else
		{
			for (i = 0; i < ulPartLen; i++)
				pEncryptedPart[i] = pPart[i] ^ 0xAB;
		}
	}

	*pulEncryptedPartLen = ulPartLen;

	return CKR_OK;
}


CK_DEFINE_FUNCTION(CK_RV, C_EncryptFinal)(CK_SESSION_HANDLE hSession, CK_BYTE_PTR pLastEncryptedPart, CK_ULONG_PTR pulLastEncryptedPartLen)
{
	if (CK_FALSE == pkcs11_mock_initialized)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	if ((PKCS11_MOCK_CK_OPERATION_ENCRYPT != mock_session->find_op.active_operation) &&
		(PKCS11_MOCK_CK_OPERATION_DIGEST_ENCRYPT != mock_session->find_op.active_operation) &&
		(PKCS11_MOCK_CK_OPERATION_SIGN_ENCRYPT != mock_session->find_op.active_operation))
		return CKR_OPERATION_NOT_INITIALIZED;

	if ((CK_FALSE == pkcs11_mock_session_opened) || (PKCS11_MOCK_CK_SESSION_ID != hSession))
		return CKR_SESSION_HANDLE_INVALID;

	if (NULL == pulLastEncryptedPartLen)
		return CKR_ARGUMENTS_BAD;

	if (NULL != pLastEncryptedPart)
	{
		switch (mock_session->find_op.active_operation)
		{
			case PKCS11_MOCK_CK_OPERATION_ENCRYPT:
				mock_session->find_op.active_operation = PKCS11_MOCK_CK_OPERATION_NONE;
				break;
			case PKCS11_MOCK_CK_OPERATION_DIGEST_ENCRYPT:
				mock_session->find_op.active_operation = PKCS11_MOCK_CK_OPERATION_DIGEST;
				break;
			case PKCS11_MOCK_CK_OPERATION_SIGN_ENCRYPT:
				mock_session->find_op.active_operation = PKCS11_MOCK_CK_OPERATION_SIGN;
				break;
			default:
				return CKR_FUNCTION_FAILED;
		}
	}

	*pulLastEncryptedPartLen = 0;

	return CKR_OK;
}


CK_DEFINE_FUNCTION(CK_RV, C_DecryptInit)(CK_SESSION_HANDLE hSession, CK_MECHANISM_PTR pMechanism, CK_OBJECT_HANDLE hKey)
{
	if (CK_FALSE == pkcs11_mock_initialized)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	if ((CK_FALSE == pkcs11_mock_session_opened) || (PKCS11_MOCK_CK_SESSION_ID != hSession))
		return CKR_SESSION_HANDLE_INVALID;

	if ((PKCS11_MOCK_CK_OPERATION_NONE != mock_session->find_op.active_operation) &&
		(PKCS11_MOCK_CK_OPERATION_DIGEST != mock_session->find_op.active_operation) && 
		(PKCS11_MOCK_CK_OPERATION_VERIFY != mock_session->find_op.active_operation))
		return CKR_OPERATION_ACTIVE;

	if (pkcs11_mock_flags & MOCK_FLAG_ALWAYS_AUTH || pkcs11_mock_flags & MOCK_FLAG_SAFENET_ALWAYS_AUTH) {
		mock_session->state = CKS_RO_PUBLIC_SESSION;
		pkcs11_mock_session_reauth = 0;
	}

	if (NULL == pMechanism)
		return CKR_ARGUMENTS_BAD;

	switch (pMechanism->mechanism)
	{
		case CKM_RSA_PKCS:

			if ((NULL != pMechanism->pParameter) || (0 != pMechanism->ulParameterLen))
				return CKR_MECHANISM_PARAM_INVALID;

			if (PKCS11_MOCK_CK_OBJECT_HANDLE_PRIVATE_KEY != hKey)
				return CKR_KEY_TYPE_INCONSISTENT;

			break;
#if 0
		case CKM_RSA_PKCS_OAEP:

			if ((NULL == pMechanism->pParameter) || (sizeof(CK_RSA_PKCS_OAEP_PARAMS) != pMechanism->ulParameterLen))
				return CKR_MECHANISM_PARAM_INVALID;

			if (PKCS11_MOCK_CK_OBJECT_HANDLE_PRIVATE_KEY != hKey)
				return CKR_KEY_TYPE_INCONSISTENT;

			break;
#endif
		case CKM_DES3_CBC:

			if ((NULL == pMechanism->pParameter) || (8 != pMechanism->ulParameterLen))
				return CKR_MECHANISM_PARAM_INVALID;

			if (PKCS11_MOCK_CK_OBJECT_HANDLE_SECRET_KEY != hKey)
				return CKR_KEY_TYPE_INCONSISTENT;

			break;

		case CKM_AES_CBC:
			
			if ((NULL == pMechanism->pParameter) || (16 != pMechanism->ulParameterLen))
				return CKR_MECHANISM_PARAM_INVALID;

			if (PKCS11_MOCK_CK_OBJECT_HANDLE_SECRET_KEY != hKey)
				return CKR_KEY_TYPE_INCONSISTENT;

			break;

		default:

			return CKR_MECHANISM_INVALID;
	}

	switch (mock_session->find_op.active_operation)
	{
		case PKCS11_MOCK_CK_OPERATION_NONE:
			mock_session->find_op.active_operation = PKCS11_MOCK_CK_OPERATION_DECRYPT;
			break;
		case PKCS11_MOCK_CK_OPERATION_DIGEST:
			mock_session->find_op.active_operation = PKCS11_MOCK_CK_OPERATION_DECRYPT_DIGEST;
			break;
		case PKCS11_MOCK_CK_OPERATION_VERIFY:
			mock_session->find_op.active_operation = PKCS11_MOCK_CK_OPERATION_DECRYPT_VERIFY;
			break;
		default:
			return CKR_FUNCTION_FAILED;
	}

	return CKR_OK;
}


CK_DEFINE_FUNCTION(CK_RV, C_Decrypt)(CK_SESSION_HANDLE hSession, CK_BYTE_PTR pEncryptedData, CK_ULONG ulEncryptedDataLen, CK_BYTE_PTR pData, CK_ULONG_PTR pulDataLen)
{
	CK_ULONG i = 0;

	if (CK_FALSE == pkcs11_mock_initialized)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	if ((CK_FALSE == pkcs11_mock_session_opened) || (PKCS11_MOCK_CK_SESSION_ID != hSession))
		return CKR_SESSION_HANDLE_INVALID;

	if (PKCS11_MOCK_CK_OPERATION_DECRYPT != mock_session->find_op.active_operation)
		return CKR_OPERATION_NOT_INITIALIZED;

	if (pkcs11_mock_flags & MOCK_FLAG_ALWAYS_AUTH || pkcs11_mock_flags & MOCK_FLAG_SAFENET_ALWAYS_AUTH) {
		if (!pkcs11_mock_session_reauth) {
			return CKR_USER_NOT_LOGGED_IN;
		}
		if ((pkcs11_mock_flags & MOCK_FLAG_ALWAYS_AUTH) && pData != NULL) {
			pkcs11_mock_session_reauth = 0;
		}
	}

	if (NULL == pEncryptedData)
		return CKR_ARGUMENTS_BAD;

	if (0 >= ulEncryptedDataLen)
		return CKR_ARGUMENTS_BAD;

	if (NULL == pulDataLen)
		return CKR_ARGUMENTS_BAD;

	if (NULL != pData)
	{
		if (ulEncryptedDataLen > *pulDataLen)
		{
			return CKR_BUFFER_TOO_SMALL;
		}
		else
		{
			for (i = 0; i < ulEncryptedDataLen; i++)
				pData[i] = pEncryptedData[i] ^ 0xAB;

			mock_session->find_op.active_operation = PKCS11_MOCK_CK_OPERATION_NONE;
		}
	}

	*pulDataLen = ulEncryptedDataLen;

	return CKR_OK;
}


CK_DEFINE_FUNCTION(CK_RV, C_DecryptUpdate)(CK_SESSION_HANDLE hSession, CK_BYTE_PTR pEncryptedPart, CK_ULONG ulEncryptedPartLen, CK_BYTE_PTR pPart, CK_ULONG_PTR pulPartLen)
{
	CK_ULONG i = 0;

	if (CK_FALSE == pkcs11_mock_initialized)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	if ((CK_FALSE == pkcs11_mock_session_opened) || (PKCS11_MOCK_CK_SESSION_ID != hSession))
		return CKR_SESSION_HANDLE_INVALID;

	if (PKCS11_MOCK_CK_OPERATION_DECRYPT != mock_session->find_op.active_operation)
		return CKR_OPERATION_NOT_INITIALIZED;

	if (NULL == pEncryptedPart)
		return CKR_ARGUMENTS_BAD;

	if (0 >= ulEncryptedPartLen)
		return CKR_ARGUMENTS_BAD;

	if (NULL == pulPartLen)
		return CKR_ARGUMENTS_BAD;

	if (NULL != pPart)
	{
		if (ulEncryptedPartLen > *pulPartLen)
		{
			return CKR_BUFFER_TOO_SMALL;
		}
		else
		{
			for (i = 0; i < ulEncryptedPartLen; i++)
				pPart[i] = pEncryptedPart[i] ^ 0xAB;
		}
	}

	*pulPartLen = ulEncryptedPartLen;

	return CKR_OK;
}


CK_DEFINE_FUNCTION(CK_RV, C_DecryptFinal)(CK_SESSION_HANDLE hSession, CK_BYTE_PTR pLastPart, CK_ULONG_PTR pulLastPartLen)
{
	if (CK_FALSE == pkcs11_mock_initialized)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	if ((CK_FALSE == pkcs11_mock_session_opened) || (PKCS11_MOCK_CK_SESSION_ID != hSession))
		return CKR_SESSION_HANDLE_INVALID;

	if ((PKCS11_MOCK_CK_OPERATION_DECRYPT != mock_session->find_op.active_operation) &&
		(PKCS11_MOCK_CK_OPERATION_DECRYPT_DIGEST != mock_session->find_op.active_operation) &&
		(PKCS11_MOCK_CK_OPERATION_DECRYPT_VERIFY != mock_session->find_op.active_operation))
		return CKR_OPERATION_NOT_INITIALIZED;

	if (NULL == pulLastPartLen)
		return CKR_ARGUMENTS_BAD;

	if (NULL != pLastPart)
	{
		switch (mock_session->find_op.active_operation)
		{
			case PKCS11_MOCK_CK_OPERATION_DECRYPT:
				mock_session->find_op.active_operation = PKCS11_MOCK_CK_OPERATION_NONE;
				break;
			case PKCS11_MOCK_CK_OPERATION_DECRYPT_DIGEST:
				mock_session->find_op.active_operation = PKCS11_MOCK_CK_OPERATION_DIGEST;
				break;
			case PKCS11_MOCK_CK_OPERATION_DECRYPT_VERIFY:
				mock_session->find_op.active_operation = PKCS11_MOCK_CK_OPERATION_VERIFY;
				break;
			default:
				return CKR_FUNCTION_FAILED;
		}
	}

	*pulLastPartLen = 0;

	return CKR_OK;
}


CK_DEFINE_FUNCTION(CK_RV, C_DigestInit)(CK_SESSION_HANDLE hSession, CK_MECHANISM_PTR pMechanism)
{
	if (CK_FALSE == pkcs11_mock_initialized)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	if ((PKCS11_MOCK_CK_OPERATION_NONE != mock_session->find_op.active_operation) &&
		(PKCS11_MOCK_CK_OPERATION_ENCRYPT != mock_session->find_op.active_operation) && 
		(PKCS11_MOCK_CK_OPERATION_DECRYPT != mock_session->find_op.active_operation))
		return CKR_OPERATION_ACTIVE;

	if ((CK_FALSE == pkcs11_mock_session_opened) || (PKCS11_MOCK_CK_SESSION_ID != hSession))
		return CKR_SESSION_HANDLE_INVALID;

	if (NULL == pMechanism)
		return CKR_ARGUMENTS_BAD;

	if (CKM_SHA_1 != pMechanism->mechanism)
		return CKR_MECHANISM_INVALID;

	if ((NULL != pMechanism->pParameter) || (0 != pMechanism->ulParameterLen))
		return CKR_MECHANISM_PARAM_INVALID;

	switch (mock_session->find_op.active_operation)
	{
		case PKCS11_MOCK_CK_OPERATION_NONE:
			mock_session->find_op.active_operation = PKCS11_MOCK_CK_OPERATION_DIGEST;
			break;
		case PKCS11_MOCK_CK_OPERATION_ENCRYPT:
			mock_session->find_op.active_operation = PKCS11_MOCK_CK_OPERATION_DIGEST_ENCRYPT;
			break;
		case PKCS11_MOCK_CK_OPERATION_DECRYPT:
			mock_session->find_op.active_operation = PKCS11_MOCK_CK_OPERATION_DECRYPT_DIGEST;
			break;
		default:
			return CKR_FUNCTION_FAILED;
	}

	return CKR_OK;
}


CK_DEFINE_FUNCTION(CK_RV, C_Digest)(CK_SESSION_HANDLE hSession, CK_BYTE_PTR pData, CK_ULONG ulDataLen, CK_BYTE_PTR pDigest, CK_ULONG_PTR pulDigestLen)
{
	CK_BYTE hash[20] = { 0x7B, 0x50, 0x2C, 0x3A, 0x1F, 0x48, 0xC8, 0x60, 0x9A, 0xE2, 0x12, 0xCD, 0xFB, 0x63, 0x9D, 0xEE, 0x39, 0x67, 0x3F, 0x5E };

	if (CK_FALSE == pkcs11_mock_initialized)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	if (PKCS11_MOCK_CK_OPERATION_DIGEST != mock_session->find_op.active_operation)
		return CKR_OPERATION_NOT_INITIALIZED;

	if ((CK_FALSE == pkcs11_mock_session_opened) || (PKCS11_MOCK_CK_SESSION_ID != hSession))
		return CKR_SESSION_HANDLE_INVALID;

	if (NULL == pData)
		return CKR_ARGUMENTS_BAD;

	if (0 >= ulDataLen)
		return CKR_ARGUMENTS_BAD;

	if (NULL == pulDigestLen)
		return CKR_ARGUMENTS_BAD;

	if (NULL != pDigest)
	{
		if (sizeof(hash) > *pulDigestLen)
		{
			return CKR_BUFFER_TOO_SMALL;
		}
		else
		{
			memcpy(pDigest, hash, sizeof(hash));
			mock_session->find_op.active_operation = PKCS11_MOCK_CK_OPERATION_NONE;
		}
	}

	*pulDigestLen = sizeof(hash);

	return CKR_OK;
}


CK_DEFINE_FUNCTION(CK_RV, C_DigestUpdate)(CK_SESSION_HANDLE hSession, CK_BYTE_PTR pPart, CK_ULONG ulPartLen)
{
	if (CK_FALSE == pkcs11_mock_initialized)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	if (PKCS11_MOCK_CK_OPERATION_DIGEST != mock_session->find_op.active_operation)
		return CKR_OPERATION_NOT_INITIALIZED;

	if ((CK_FALSE == pkcs11_mock_session_opened) || (PKCS11_MOCK_CK_SESSION_ID != hSession))
		return CKR_SESSION_HANDLE_INVALID;

	if (NULL == pPart)
		return CKR_ARGUMENTS_BAD;

	if (0 >= ulPartLen)
		return CKR_ARGUMENTS_BAD;

	return CKR_OK;
}


CK_DEFINE_FUNCTION(CK_RV, C_DigestKey)(CK_SESSION_HANDLE hSession, CK_OBJECT_HANDLE hKey)
{
	if (CK_FALSE == pkcs11_mock_initialized)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	if (PKCS11_MOCK_CK_OPERATION_DIGEST != mock_session->find_op.active_operation)
		return CKR_OPERATION_NOT_INITIALIZED;

	if ((CK_FALSE == pkcs11_mock_session_opened) || (PKCS11_MOCK_CK_SESSION_ID != hSession))
		return CKR_SESSION_HANDLE_INVALID;

	if (PKCS11_MOCK_CK_OBJECT_HANDLE_SECRET_KEY != hKey)
		return CKR_OBJECT_HANDLE_INVALID;

	return CKR_OK;
}


CK_DEFINE_FUNCTION(CK_RV, C_DigestFinal)(CK_SESSION_HANDLE hSession, CK_BYTE_PTR pDigest, CK_ULONG_PTR pulDigestLen)
{
	CK_BYTE hash[20] = { 0x7B, 0x50, 0x2C, 0x3A, 0x1F, 0x48, 0xC8, 0x60, 0x9A, 0xE2, 0x12, 0xCD, 0xFB, 0x63, 0x9D, 0xEE, 0x39, 0x67, 0x3F, 0x5E };

	if (CK_FALSE == pkcs11_mock_initialized)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	if ((PKCS11_MOCK_CK_OPERATION_DIGEST != mock_session->find_op.active_operation) && 
		(PKCS11_MOCK_CK_OPERATION_DIGEST_ENCRYPT != mock_session->find_op.active_operation) && 
		(PKCS11_MOCK_CK_OPERATION_DECRYPT_DIGEST != mock_session->find_op.active_operation))
		return CKR_OPERATION_NOT_INITIALIZED;

	if ((CK_FALSE == pkcs11_mock_session_opened) || (PKCS11_MOCK_CK_SESSION_ID != hSession))
		return CKR_SESSION_HANDLE_INVALID;

	if (NULL == pulDigestLen)
		return CKR_ARGUMENTS_BAD;

	if (NULL != pDigest)
	{
		if (sizeof(hash) > *pulDigestLen)
		{
			return CKR_BUFFER_TOO_SMALL;
		}
		else
		{
			memcpy(pDigest, hash, sizeof(hash));

			switch (mock_session->find_op.active_operation)
			{
				case PKCS11_MOCK_CK_OPERATION_DIGEST:
					mock_session->find_op.active_operation = PKCS11_MOCK_CK_OPERATION_NONE;
					break;
				case PKCS11_MOCK_CK_OPERATION_DIGEST_ENCRYPT:
					mock_session->find_op.active_operation = PKCS11_MOCK_CK_OPERATION_ENCRYPT;
					break;
				case PKCS11_MOCK_CK_OPERATION_DECRYPT_DIGEST:
					mock_session->find_op.active_operation = PKCS11_MOCK_CK_OPERATION_DECRYPT;
					break;
				default:
					return CKR_FUNCTION_FAILED;
			}
		}
	}

	*pulDigestLen = sizeof(hash);

	return CKR_OK;
}


CK_DEFINE_FUNCTION(CK_RV, C_SignInit)(CK_SESSION_HANDLE hSession, CK_MECHANISM_PTR pMechanism, CK_OBJECT_HANDLE hKey)
{
	if (CK_FALSE == pkcs11_mock_initialized)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	if ((CK_FALSE == pkcs11_mock_session_opened) || (PKCS11_MOCK_CK_SESSION_ID != hSession))
		return CKR_SESSION_HANDLE_INVALID;

	if ((PKCS11_MOCK_CK_OPERATION_NONE != mock_session->find_op.active_operation) &&
		(PKCS11_MOCK_CK_OPERATION_ENCRYPT != mock_session->find_op.active_operation))
		return CKR_OPERATION_ACTIVE;

	if (pkcs11_mock_flags & MOCK_FLAG_ALWAYS_AUTH || pkcs11_mock_flags & MOCK_FLAG_SAFENET_ALWAYS_AUTH) {
		mock_session->state = CKS_RO_PUBLIC_SESSION;
		pkcs11_mock_session_reauth = 0;
	}

	if (NULL == pMechanism)
		return CKR_ARGUMENTS_BAD;

	if ((CKM_RSA_PKCS == pMechanism->mechanism) || (CKM_SHA1_RSA_PKCS == pMechanism->mechanism))
	{
		if ((NULL != pMechanism->pParameter) || (0 != pMechanism->ulParameterLen))
			return CKR_MECHANISM_PARAM_INVALID;

		if (PKCS11_MOCK_CK_OBJECT_HANDLE_PRIVATE_KEY != hKey)
			return CKR_KEY_TYPE_INCONSISTENT;
	}
	else
	{
		return CKR_MECHANISM_INVALID;
	}

	if (PKCS11_MOCK_CK_OPERATION_NONE == mock_session->find_op.active_operation)
		mock_session->find_op.active_operation = PKCS11_MOCK_CK_OPERATION_SIGN;
	else
		mock_session->find_op.active_operation = PKCS11_MOCK_CK_OPERATION_SIGN_ENCRYPT;

	return CKR_OK;
}


CK_DEFINE_FUNCTION(CK_RV, C_Sign)(CK_SESSION_HANDLE hSession, CK_BYTE_PTR pData, CK_ULONG ulDataLen, CK_BYTE_PTR pSignature, CK_ULONG_PTR pulSignatureLen)
{
	CK_BYTE signature[10] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09 };

	if (CK_FALSE == pkcs11_mock_initialized)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	if (PKCS11_MOCK_CK_OPERATION_SIGN != mock_session->find_op.active_operation)
		return CKR_OPERATION_NOT_INITIALIZED;

	if ((CK_FALSE == pkcs11_mock_session_opened) || (PKCS11_MOCK_CK_SESSION_ID != hSession))
		return CKR_SESSION_HANDLE_INVALID;

	if (pkcs11_mock_flags & MOCK_FLAG_ALWAYS_AUTH || pkcs11_mock_flags & MOCK_FLAG_SAFENET_ALWAYS_AUTH) {
		if (!pkcs11_mock_session_reauth) {
			return CKR_USER_NOT_LOGGED_IN;
		}

		if ((pkcs11_mock_flags & MOCK_FLAG_ALWAYS_AUTH) && pSignature != NULL) {
			pkcs11_mock_session_reauth = 0;
		}
	}

	if (NULL == pData)
		return CKR_ARGUMENTS_BAD;

	if (0 >= ulDataLen)
		return CKR_ARGUMENTS_BAD;

	if (NULL == pulSignatureLen)
		return CKR_ARGUMENTS_BAD;

	if (NULL != pSignature)
	{
		if (sizeof(signature) > *pulSignatureLen)
		{
			return CKR_BUFFER_TOO_SMALL;
		}
		else
		{
			memcpy(pSignature, signature, sizeof(signature));
			mock_session->find_op.active_operation = PKCS11_MOCK_CK_OPERATION_NONE;
		}
	}

	*pulSignatureLen = sizeof(signature);

	return CKR_OK;
}


CK_DEFINE_FUNCTION(CK_RV, C_SignUpdate)(CK_SESSION_HANDLE hSession, CK_BYTE_PTR pPart, CK_ULONG ulPartLen)
{
	if (CK_FALSE == pkcs11_mock_initialized)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	if (PKCS11_MOCK_CK_OPERATION_SIGN != mock_session->find_op.active_operation)
		return CKR_OPERATION_NOT_INITIALIZED;

	if ((CK_FALSE == pkcs11_mock_session_opened) || (PKCS11_MOCK_CK_SESSION_ID != hSession))
		return CKR_SESSION_HANDLE_INVALID;

	if (NULL == pPart)
		return CKR_ARGUMENTS_BAD;

	if (0 >= ulPartLen)
		return CKR_ARGUMENTS_BAD;

	return CKR_OK;
}


CK_DEFINE_FUNCTION(CK_RV, C_SignFinal)(CK_SESSION_HANDLE hSession, CK_BYTE_PTR pSignature, CK_ULONG_PTR pulSignatureLen)
{
	CK_BYTE signature[10] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09 };

	if (CK_FALSE == pkcs11_mock_initialized)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	if ((PKCS11_MOCK_CK_OPERATION_SIGN != mock_session->find_op.active_operation) && 
		(PKCS11_MOCK_CK_OPERATION_SIGN_ENCRYPT != mock_session->find_op.active_operation))
		return CKR_OPERATION_NOT_INITIALIZED;

	if ((CK_FALSE == pkcs11_mock_session_opened) || (PKCS11_MOCK_CK_SESSION_ID != hSession))
		return CKR_SESSION_HANDLE_INVALID;

	if (NULL == pulSignatureLen)
		return CKR_ARGUMENTS_BAD;

	if (NULL != pSignature)
	{
		if (sizeof(signature) > *pulSignatureLen)
		{
			return CKR_BUFFER_TOO_SMALL;
		}
		else
		{
			memcpy(pSignature, signature, sizeof(signature));

			if (PKCS11_MOCK_CK_OPERATION_SIGN == mock_session->find_op.active_operation)
				mock_session->find_op.active_operation = PKCS11_MOCK_CK_OPERATION_NONE;
			else
				mock_session->find_op.active_operation = PKCS11_MOCK_CK_OPERATION_ENCRYPT;
		}
	}

	*pulSignatureLen = sizeof(signature);

	return CKR_OK;
}


CK_DEFINE_FUNCTION(CK_RV, C_SignRecoverInit)(CK_SESSION_HANDLE hSession, CK_MECHANISM_PTR pMechanism, CK_OBJECT_HANDLE hKey)
{
	if (CK_FALSE == pkcs11_mock_initialized)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	if ((CK_FALSE == pkcs11_mock_session_opened) || (PKCS11_MOCK_CK_SESSION_ID != hSession))
		return CKR_SESSION_HANDLE_INVALID;

	if (PKCS11_MOCK_CK_OPERATION_NONE != mock_session->find_op.active_operation)
		return CKR_OPERATION_ACTIVE;

	if (NULL == pMechanism)
		return CKR_ARGUMENTS_BAD;

	if (CKM_RSA_PKCS == pMechanism->mechanism)
	{
		if ((NULL != pMechanism->pParameter) || (0 != pMechanism->ulParameterLen))
			return CKR_MECHANISM_PARAM_INVALID;

		if (PKCS11_MOCK_CK_OBJECT_HANDLE_PRIVATE_KEY != hKey)
			return CKR_KEY_TYPE_INCONSISTENT;
	}
	else
	{
		return CKR_MECHANISM_INVALID;
	}

	mock_session->find_op.active_operation = PKCS11_MOCK_CK_OPERATION_SIGN_RECOVER;

	return CKR_OK;
}


CK_DEFINE_FUNCTION(CK_RV, C_SignRecover)(CK_SESSION_HANDLE hSession, CK_BYTE_PTR pData, CK_ULONG ulDataLen, CK_BYTE_PTR pSignature, CK_ULONG_PTR pulSignatureLen)
{
	CK_ULONG i = 0;

	if (CK_FALSE == pkcs11_mock_initialized)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	if (PKCS11_MOCK_CK_OPERATION_SIGN_RECOVER != mock_session->find_op.active_operation)
		return CKR_OPERATION_NOT_INITIALIZED;

	if ((CK_FALSE == pkcs11_mock_session_opened) || (PKCS11_MOCK_CK_SESSION_ID != hSession))
		return CKR_SESSION_HANDLE_INVALID;

	if (NULL == pData)
		return CKR_ARGUMENTS_BAD;

	if (0 >= ulDataLen)
		return CKR_ARGUMENTS_BAD;

	if (NULL == pulSignatureLen)
		return CKR_ARGUMENTS_BAD;

	if (NULL != pSignature)
	{
		if (ulDataLen > *pulSignatureLen)
		{
			return CKR_BUFFER_TOO_SMALL;
		}
		else
		{
			for (i = 0; i < ulDataLen; i++)
				pSignature[i] = pData[i] ^ 0xAB;

			mock_session->find_op.active_operation = PKCS11_MOCK_CK_OPERATION_NONE;
		}
	}

	*pulSignatureLen = ulDataLen;

	return CKR_OK;
}


CK_DEFINE_FUNCTION(CK_RV, C_VerifyInit)(CK_SESSION_HANDLE hSession, CK_MECHANISM_PTR pMechanism, CK_OBJECT_HANDLE hKey)
{
	if (CK_FALSE == pkcs11_mock_initialized)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	if ((CK_FALSE == pkcs11_mock_session_opened) || (PKCS11_MOCK_CK_SESSION_ID != hSession))
		return CKR_SESSION_HANDLE_INVALID;

	if ((PKCS11_MOCK_CK_OPERATION_NONE != mock_session->find_op.active_operation) &&
		(PKCS11_MOCK_CK_OPERATION_DECRYPT != mock_session->find_op.active_operation))
		return CKR_OPERATION_ACTIVE;

	if (NULL == pMechanism)
		return CKR_ARGUMENTS_BAD;

	if ((CKM_RSA_PKCS == pMechanism->mechanism) || (CKM_SHA1_RSA_PKCS == pMechanism->mechanism))
	{
		if ((NULL != pMechanism->pParameter) || (0 != pMechanism->ulParameterLen))
			return CKR_MECHANISM_PARAM_INVALID;

		if (PKCS11_MOCK_CK_OBJECT_HANDLE_PUBLIC_KEY != hKey)
			return CKR_KEY_TYPE_INCONSISTENT;
	}
	else
	{
		return CKR_MECHANISM_INVALID;
	}

	if (PKCS11_MOCK_CK_OPERATION_NONE == mock_session->find_op.active_operation)
		mock_session->find_op.active_operation = PKCS11_MOCK_CK_OPERATION_VERIFY;
	else
		mock_session->find_op.active_operation = PKCS11_MOCK_CK_OPERATION_DECRYPT_VERIFY;

	return CKR_OK;
}


CK_DEFINE_FUNCTION(CK_RV, C_Verify)(CK_SESSION_HANDLE hSession, CK_BYTE_PTR pData, CK_ULONG ulDataLen, CK_BYTE_PTR pSignature, CK_ULONG ulSignatureLen)
{
	CK_BYTE signature[10] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09 };

	if (CK_FALSE == pkcs11_mock_initialized)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	if (PKCS11_MOCK_CK_OPERATION_VERIFY != mock_session->find_op.active_operation)
		return CKR_OPERATION_NOT_INITIALIZED;

	if ((CK_FALSE == pkcs11_mock_session_opened) || (PKCS11_MOCK_CK_SESSION_ID != hSession))
		return CKR_SESSION_HANDLE_INVALID;

	if (NULL == pData)
		return CKR_ARGUMENTS_BAD;

	if (0 >= ulDataLen)
		return CKR_ARGUMENTS_BAD;

	if (NULL == pSignature)
		return CKR_ARGUMENTS_BAD;

	if (0 >= ulSignatureLen)
		return CKR_ARGUMENTS_BAD;

	if (sizeof(signature) != ulSignatureLen)
		return CKR_SIGNATURE_LEN_RANGE;

	if (0 != memcmp(pSignature, signature, sizeof(signature)))
		return CKR_SIGNATURE_INVALID;

	mock_session->find_op.active_operation = PKCS11_MOCK_CK_OPERATION_NONE;

	return CKR_OK;
}


CK_DEFINE_FUNCTION(CK_RV, C_VerifyUpdate)(CK_SESSION_HANDLE hSession, CK_BYTE_PTR pPart, CK_ULONG ulPartLen)
{
	if (CK_FALSE == pkcs11_mock_initialized)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	if (PKCS11_MOCK_CK_OPERATION_VERIFY != mock_session->find_op.active_operation)
		return CKR_OPERATION_NOT_INITIALIZED;

	if ((CK_FALSE == pkcs11_mock_session_opened) || (PKCS11_MOCK_CK_SESSION_ID != hSession))
		return CKR_SESSION_HANDLE_INVALID;

	if (NULL == pPart)
		return CKR_ARGUMENTS_BAD;

	if (0 >= ulPartLen)
		return CKR_ARGUMENTS_BAD;

	return CKR_OK;
}


CK_DEFINE_FUNCTION(CK_RV, C_VerifyFinal)(CK_SESSION_HANDLE hSession, CK_BYTE_PTR pSignature, CK_ULONG ulSignatureLen)
{
	CK_BYTE signature[10] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09 };

	if (CK_FALSE == pkcs11_mock_initialized)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	if ((PKCS11_MOCK_CK_OPERATION_VERIFY != mock_session->find_op.active_operation) &&
		(PKCS11_MOCK_CK_OPERATION_DECRYPT_VERIFY != mock_session->find_op.active_operation))
		return CKR_OPERATION_NOT_INITIALIZED;

	if ((CK_FALSE == pkcs11_mock_session_opened) || (PKCS11_MOCK_CK_SESSION_ID != hSession))
		return CKR_SESSION_HANDLE_INVALID;

	if (NULL == pSignature)
		return CKR_ARGUMENTS_BAD;

	if (0 >= ulSignatureLen)
		return CKR_ARGUMENTS_BAD;

	if (sizeof(signature) != ulSignatureLen)
		return CKR_SIGNATURE_LEN_RANGE;

	if (0 != memcmp(pSignature, signature, sizeof(signature)))
		return CKR_SIGNATURE_INVALID;

	if (PKCS11_MOCK_CK_OPERATION_VERIFY == mock_session->find_op.active_operation)
		mock_session->find_op.active_operation = PKCS11_MOCK_CK_OPERATION_NONE;
	else
		mock_session->find_op.active_operation = PKCS11_MOCK_CK_OPERATION_DECRYPT;

	return CKR_OK;
}


CK_DEFINE_FUNCTION(CK_RV, C_VerifyRecoverInit)(CK_SESSION_HANDLE hSession, CK_MECHANISM_PTR pMechanism, CK_OBJECT_HANDLE hKey)
{
	if (CK_FALSE == pkcs11_mock_initialized)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	if ((CK_FALSE == pkcs11_mock_session_opened) || (PKCS11_MOCK_CK_SESSION_ID != hSession))
		return CKR_SESSION_HANDLE_INVALID;

	if (PKCS11_MOCK_CK_OPERATION_NONE != mock_session->find_op.active_operation)
		return CKR_OPERATION_ACTIVE;

	if (NULL == pMechanism)
		return CKR_ARGUMENTS_BAD;

	if (CKM_RSA_PKCS == pMechanism->mechanism)
	{
		if ((NULL != pMechanism->pParameter) || (0 != pMechanism->ulParameterLen))
			return CKR_MECHANISM_PARAM_INVALID;

		if (PKCS11_MOCK_CK_OBJECT_HANDLE_PUBLIC_KEY != hKey)
			return CKR_KEY_TYPE_INCONSISTENT;
	}
	else
	{
		return CKR_MECHANISM_INVALID;
	}

	mock_session->find_op.active_operation = PKCS11_MOCK_CK_OPERATION_VERIFY_RECOVER;

	return CKR_OK;
}


CK_DEFINE_FUNCTION(CK_RV, C_VerifyRecover)(CK_SESSION_HANDLE hSession, CK_BYTE_PTR pSignature, CK_ULONG ulSignatureLen, CK_BYTE_PTR pData, CK_ULONG_PTR pulDataLen)
{
	CK_ULONG i = 0;

	if (CK_FALSE == pkcs11_mock_initialized)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	if (PKCS11_MOCK_CK_OPERATION_VERIFY_RECOVER != mock_session->find_op.active_operation)
		return CKR_OPERATION_NOT_INITIALIZED;

	if ((CK_FALSE == pkcs11_mock_session_opened) || (PKCS11_MOCK_CK_SESSION_ID != hSession))
		return CKR_SESSION_HANDLE_INVALID;

	if (NULL == pSignature)
		return CKR_ARGUMENTS_BAD;

	if (0 >= ulSignatureLen)
		return CKR_ARGUMENTS_BAD;

	if (NULL == pulDataLen)
		return CKR_ARGUMENTS_BAD;

	if (NULL != pData)
	{
		if (ulSignatureLen > *pulDataLen)
		{
			return CKR_BUFFER_TOO_SMALL;
		}
		else
		{
			for (i = 0; i < ulSignatureLen; i++)
				pData[i] = pSignature[i] ^ 0xAB;

			mock_session->find_op.active_operation = PKCS11_MOCK_CK_OPERATION_NONE;
		}
	}

	*pulDataLen = ulSignatureLen;

	return CKR_OK;
}


CK_DEFINE_FUNCTION(CK_RV, C_DigestEncryptUpdate)(CK_SESSION_HANDLE hSession, CK_BYTE_PTR pPart, CK_ULONG ulPartLen, CK_BYTE_PTR pEncryptedPart, CK_ULONG_PTR pulEncryptedPartLen)
{
	CK_ULONG i = 0;

	if (CK_FALSE == pkcs11_mock_initialized)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	if (PKCS11_MOCK_CK_OPERATION_DIGEST_ENCRYPT != mock_session->find_op.active_operation)
		return CKR_OPERATION_NOT_INITIALIZED;

	if ((CK_FALSE == pkcs11_mock_session_opened) || (PKCS11_MOCK_CK_SESSION_ID != hSession))
		return CKR_SESSION_HANDLE_INVALID;

	if (NULL == pPart)
		return CKR_ARGUMENTS_BAD;

	if (0 >= ulPartLen)
		return CKR_ARGUMENTS_BAD;

	if (NULL == pulEncryptedPartLen)
		return CKR_ARGUMENTS_BAD;

	if (NULL != pEncryptedPart)
	{
		if (ulPartLen > *pulEncryptedPartLen)
		{
			return CKR_BUFFER_TOO_SMALL;
		}
		else
		{
			for (i = 0; i < ulPartLen; i++)
				pEncryptedPart[i] = pPart[i] ^ 0xAB;
		}
	}

	*pulEncryptedPartLen = ulPartLen;

	return CKR_OK;
}


CK_DEFINE_FUNCTION(CK_RV, C_DecryptDigestUpdate)(CK_SESSION_HANDLE hSession, CK_BYTE_PTR pEncryptedPart, CK_ULONG ulEncryptedPartLen, CK_BYTE_PTR pPart, CK_ULONG_PTR pulPartLen)
{
	CK_ULONG i = 0;

	if (CK_FALSE == pkcs11_mock_initialized)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	if ((CK_FALSE == pkcs11_mock_session_opened) || (PKCS11_MOCK_CK_SESSION_ID != hSession))
		return CKR_SESSION_HANDLE_INVALID;

	if (PKCS11_MOCK_CK_OPERATION_DECRYPT_DIGEST != mock_session->find_op.active_operation)
		return CKR_OPERATION_NOT_INITIALIZED;

	if (NULL == pEncryptedPart)
		return CKR_ARGUMENTS_BAD;

	if (0 >= ulEncryptedPartLen)
		return CKR_ARGUMENTS_BAD;

	if (NULL == pulPartLen)
		return CKR_ARGUMENTS_BAD;

	if (NULL != pPart)
	{
		if (ulEncryptedPartLen > *pulPartLen)
		{
			return CKR_BUFFER_TOO_SMALL;
		}
		else
		{
			for (i = 0; i < ulEncryptedPartLen; i++)
				pPart[i] = pEncryptedPart[i] ^ 0xAB;
		}
	}

	*pulPartLen = ulEncryptedPartLen;

	return CKR_OK;
}


CK_DEFINE_FUNCTION(CK_RV, C_SignEncryptUpdate)(CK_SESSION_HANDLE hSession, CK_BYTE_PTR pPart, CK_ULONG ulPartLen, CK_BYTE_PTR pEncryptedPart, CK_ULONG_PTR pulEncryptedPartLen)
{
	CK_ULONG i = 0;

	if (CK_FALSE == pkcs11_mock_initialized)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	if ((CK_FALSE == pkcs11_mock_session_opened) || (PKCS11_MOCK_CK_SESSION_ID != hSession))
		return CKR_SESSION_HANDLE_INVALID;

	if (PKCS11_MOCK_CK_OPERATION_SIGN_ENCRYPT != mock_session->find_op.active_operation)
		return CKR_OPERATION_NOT_INITIALIZED;

	if (NULL == pPart)
		return CKR_ARGUMENTS_BAD;

	if (0 >= ulPartLen)
		return CKR_ARGUMENTS_BAD;

	if (NULL == pulEncryptedPartLen)
		return CKR_ARGUMENTS_BAD;

	if (NULL != pEncryptedPart)
	{
		if (ulPartLen > *pulEncryptedPartLen)
		{
			return CKR_BUFFER_TOO_SMALL;
		}
		else
		{
			for (i = 0; i < ulPartLen; i++)
				pEncryptedPart[i] = pPart[i] ^ 0xAB;
		}
	}

	*pulEncryptedPartLen = ulPartLen;

	return CKR_OK;
}


CK_DEFINE_FUNCTION(CK_RV, C_DecryptVerifyUpdate)(CK_SESSION_HANDLE hSession, CK_BYTE_PTR pEncryptedPart, CK_ULONG ulEncryptedPartLen, CK_BYTE_PTR pPart, CK_ULONG_PTR pulPartLen)
{
	CK_ULONG i = 0;

	if (CK_FALSE == pkcs11_mock_initialized)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	if ((CK_FALSE == pkcs11_mock_session_opened) || (PKCS11_MOCK_CK_SESSION_ID != hSession))
		return CKR_SESSION_HANDLE_INVALID;

	if (PKCS11_MOCK_CK_OPERATION_DECRYPT_VERIFY != mock_session->find_op.active_operation)
		return CKR_OPERATION_NOT_INITIALIZED;

	if (NULL == pEncryptedPart)
		return CKR_ARGUMENTS_BAD;

	if (0 >= ulEncryptedPartLen)
		return CKR_ARGUMENTS_BAD;

	if (NULL == pulPartLen)
		return CKR_ARGUMENTS_BAD;

	if (NULL != pPart)
	{
		if (ulEncryptedPartLen > *pulPartLen)
		{
			return CKR_BUFFER_TOO_SMALL;
		}
		else
		{
			for (i = 0; i < ulEncryptedPartLen; i++)
				pPart[i] = pEncryptedPart[i] ^ 0xAB;
		}
	}

	*pulPartLen = ulEncryptedPartLen;

	return CKR_OK;
}


CK_DEFINE_FUNCTION(CK_RV, C_GenerateKey)(CK_SESSION_HANDLE hSession, CK_MECHANISM_PTR pMechanism, CK_ATTRIBUTE_PTR pTemplate, CK_ULONG ulCount, CK_OBJECT_HANDLE_PTR phKey)
{
	CK_ULONG i = 0;

	if (CK_FALSE == pkcs11_mock_initialized)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	if ((CK_FALSE == pkcs11_mock_session_opened) || (PKCS11_MOCK_CK_SESSION_ID != hSession))
		return CKR_SESSION_HANDLE_INVALID;

	if (NULL == pMechanism)
		return CKR_ARGUMENTS_BAD;

	if (CKM_DES3_KEY_GEN != pMechanism->mechanism)
		return CKR_MECHANISM_INVALID;

	if ((NULL != pMechanism->pParameter) || (0 != pMechanism->ulParameterLen))
		return CKR_MECHANISM_PARAM_INVALID;

	if (NULL == pTemplate)
		return CKR_ARGUMENTS_BAD;

	if (0 >= ulCount)
		return CKR_ARGUMENTS_BAD;

	if (NULL == phKey)
		return CKR_ARGUMENTS_BAD;

	for (i = 0; i < ulCount; i++)
	{
		if (NULL == pTemplate[i].pValue)
			return CKR_ATTRIBUTE_VALUE_INVALID;

		if (0 >= pTemplate[i].ulValueLen)
			return CKR_ATTRIBUTE_VALUE_INVALID;
	}

	*phKey = PKCS11_MOCK_CK_OBJECT_HANDLE_SECRET_KEY;

	return CKR_OK;
}


CK_DEFINE_FUNCTION(CK_RV, C_GenerateKeyPair)(CK_SESSION_HANDLE hSession, CK_MECHANISM_PTR pMechanism, CK_ATTRIBUTE_PTR pPublicKeyTemplate, CK_ULONG ulPublicKeyAttributeCount, CK_ATTRIBUTE_PTR pPrivateKeyTemplate, CK_ULONG ulPrivateKeyAttributeCount, CK_OBJECT_HANDLE_PTR phPublicKey, CK_OBJECT_HANDLE_PTR phPrivateKey)
{
	CK_ULONG i = 0;

	if (CK_FALSE == pkcs11_mock_initialized)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	if ((CK_FALSE == pkcs11_mock_session_opened) || (PKCS11_MOCK_CK_SESSION_ID != hSession))
		return CKR_SESSION_HANDLE_INVALID;

	if (NULL == pMechanism)
		return CKR_ARGUMENTS_BAD;

	if (CKM_RSA_PKCS_KEY_PAIR_GEN != pMechanism->mechanism)
		return CKR_MECHANISM_INVALID;

	if ((NULL != pMechanism->pParameter) || (0 != pMechanism->ulParameterLen))
		return CKR_MECHANISM_PARAM_INVALID;

	if (NULL == pPublicKeyTemplate)
		return CKR_ARGUMENTS_BAD;

	if (0 >= ulPublicKeyAttributeCount)
		return CKR_ARGUMENTS_BAD;

	if (NULL == pPrivateKeyTemplate)
		return CKR_ARGUMENTS_BAD;

	if (0 >= ulPrivateKeyAttributeCount)
		return CKR_ARGUMENTS_BAD;

	if (NULL == phPublicKey)
		return CKR_ARGUMENTS_BAD;

	if (NULL == phPrivateKey)
		return CKR_ARGUMENTS_BAD;

	for (i = 0; i < ulPublicKeyAttributeCount; i++)
	{
		if (NULL == pPublicKeyTemplate[i].pValue)
			return CKR_ATTRIBUTE_VALUE_INVALID;

		if (0 >= pPublicKeyTemplate[i].ulValueLen)
			return CKR_ATTRIBUTE_VALUE_INVALID;
	}

	for (i = 0; i < ulPrivateKeyAttributeCount; i++)
	{
		if (NULL == pPrivateKeyTemplate[i].pValue)
			return CKR_ATTRIBUTE_VALUE_INVALID;

		if (0 >= pPrivateKeyTemplate[i].ulValueLen)
			return CKR_ATTRIBUTE_VALUE_INVALID;
	}

	*phPublicKey = PKCS11_MOCK_CK_OBJECT_HANDLE_PUBLIC_KEY;
	*phPrivateKey = PKCS11_MOCK_CK_OBJECT_HANDLE_PRIVATE_KEY;

	return CKR_OK;
}


CK_DEFINE_FUNCTION(CK_RV, C_WrapKey)(CK_SESSION_HANDLE hSession, CK_MECHANISM_PTR pMechanism, CK_OBJECT_HANDLE hWrappingKey, CK_OBJECT_HANDLE hKey, CK_BYTE_PTR pWrappedKey, CK_ULONG_PTR pulWrappedKeyLen)
{
	CK_BYTE wrappedKey[10] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09 };

	if (CK_FALSE == pkcs11_mock_initialized)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	if ((CK_FALSE == pkcs11_mock_session_opened) || (PKCS11_MOCK_CK_SESSION_ID != hSession))
		return CKR_SESSION_HANDLE_INVALID;

	if (NULL == pMechanism)
		return CKR_ARGUMENTS_BAD;

	if (CKM_RSA_PKCS != pMechanism->mechanism)
		return CKR_MECHANISM_INVALID;

	if ((NULL != pMechanism->pParameter) || (0 != pMechanism->ulParameterLen))
		return CKR_MECHANISM_PARAM_INVALID;

	if (PKCS11_MOCK_CK_OBJECT_HANDLE_PUBLIC_KEY != hWrappingKey)
		return CKR_KEY_HANDLE_INVALID;

	if (PKCS11_MOCK_CK_OBJECT_HANDLE_SECRET_KEY != hKey)
		return CKR_KEY_HANDLE_INVALID;

	if (NULL != pWrappedKey)
	{
		if (sizeof(wrappedKey) > *pulWrappedKeyLen)
			return CKR_BUFFER_TOO_SMALL;
		else
			memcpy(pWrappedKey, wrappedKey, sizeof(wrappedKey));
	}

	*pulWrappedKeyLen = sizeof(wrappedKey);

	return CKR_OK;
}


CK_DEFINE_FUNCTION(CK_RV, C_UnwrapKey)(CK_SESSION_HANDLE hSession, CK_MECHANISM_PTR pMechanism, CK_OBJECT_HANDLE hUnwrappingKey, CK_BYTE_PTR pWrappedKey, CK_ULONG ulWrappedKeyLen, CK_ATTRIBUTE_PTR pTemplate, CK_ULONG ulAttributeCount, CK_OBJECT_HANDLE_PTR phKey)
{
	CK_ULONG i = 0;

	if (CK_FALSE == pkcs11_mock_initialized)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	if ((CK_FALSE == pkcs11_mock_session_opened) || (PKCS11_MOCK_CK_SESSION_ID != hSession))
		return CKR_SESSION_HANDLE_INVALID;

	if (NULL == pMechanism)
		return CKR_ARGUMENTS_BAD;

	if (CKM_RSA_PKCS != pMechanism->mechanism)
		return CKR_MECHANISM_INVALID;

	if ((NULL != pMechanism->pParameter) || (0 != pMechanism->ulParameterLen))
		return CKR_MECHANISM_PARAM_INVALID;

	if (PKCS11_MOCK_CK_OBJECT_HANDLE_PRIVATE_KEY != hUnwrappingKey)
		return CKR_KEY_HANDLE_INVALID;

	if (NULL == pWrappedKey)
		return CKR_ARGUMENTS_BAD;

	if (0 >= ulWrappedKeyLen)
		return CKR_ARGUMENTS_BAD;

	if (NULL == pTemplate)
		return CKR_ARGUMENTS_BAD;

	if (0 >= ulAttributeCount)
		return CKR_ARGUMENTS_BAD;

	if (NULL == phKey)
		return CKR_ARGUMENTS_BAD;

	for (i = 0; i < ulAttributeCount; i++)
	{
		if (NULL == pTemplate[i].pValue)
			return CKR_ATTRIBUTE_VALUE_INVALID;

		if (0 >= pTemplate[i].ulValueLen)
			return CKR_ATTRIBUTE_VALUE_INVALID;
	}

	*phKey = PKCS11_MOCK_CK_OBJECT_HANDLE_SECRET_KEY;

	return CKR_OK;
}


CK_DEFINE_FUNCTION(CK_RV, C_DeriveKey)(CK_SESSION_HANDLE hSession, CK_MECHANISM_PTR pMechanism, CK_OBJECT_HANDLE hBaseKey, CK_ATTRIBUTE_PTR pTemplate, CK_ULONG ulAttributeCount, CK_OBJECT_HANDLE_PTR phKey)
{
	return CKR_GENERAL_ERROR;
}


CK_DEFINE_FUNCTION(CK_RV, C_SeedRandom)(CK_SESSION_HANDLE hSession, CK_BYTE_PTR pSeed, CK_ULONG ulSeedLen)
{
	if (CK_FALSE == pkcs11_mock_initialized)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	if ((CK_FALSE == pkcs11_mock_session_opened) || (PKCS11_MOCK_CK_SESSION_ID != hSession))
		return CKR_SESSION_HANDLE_INVALID;

	if (NULL == pSeed)
		return CKR_ARGUMENTS_BAD;

	if (0 >= ulSeedLen)
		return CKR_ARGUMENTS_BAD;

	return CKR_OK;
}


CK_DEFINE_FUNCTION(CK_RV, C_GenerateRandom)(CK_SESSION_HANDLE hSession, CK_BYTE_PTR RandomData, CK_ULONG ulRandomLen)
{
	if (CK_FALSE == pkcs11_mock_initialized)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	if ((CK_FALSE == pkcs11_mock_session_opened) || (PKCS11_MOCK_CK_SESSION_ID != hSession))
		return CKR_SESSION_HANDLE_INVALID;

	if (NULL == RandomData)
		return CKR_ARGUMENTS_BAD;

	if (0 >= ulRandomLen)
		return CKR_ARGUMENTS_BAD;

	memset(RandomData, 1, ulRandomLen);

	return CKR_OK;
}


CK_DEFINE_FUNCTION(CK_RV, C_GetFunctionStatus)(CK_SESSION_HANDLE hSession)
{
	if (CK_FALSE == pkcs11_mock_initialized)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	if ((CK_FALSE == pkcs11_mock_session_opened) || (PKCS11_MOCK_CK_SESSION_ID != hSession))
		return CKR_SESSION_HANDLE_INVALID;
	
	return CKR_FUNCTION_NOT_PARALLEL;
}


CK_DEFINE_FUNCTION(CK_RV, C_CancelFunction)(CK_SESSION_HANDLE hSession)
{
	if (CK_FALSE == pkcs11_mock_initialized)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	if ((CK_FALSE == pkcs11_mock_session_opened) || (PKCS11_MOCK_CK_SESSION_ID != hSession))
		return CKR_SESSION_HANDLE_INVALID;
	
	return CKR_FUNCTION_NOT_PARALLEL;
}


CK_DEFINE_FUNCTION(CK_RV, C_WaitForSlotEvent)(CK_FLAGS flags, CK_SLOT_ID_PTR pSlot, CK_VOID_PTR pReserved)
{
	if (CK_FALSE == pkcs11_mock_initialized)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	if ((0 != flags)  && (CKF_DONT_BLOCK != flags))
		return CKR_ARGUMENTS_BAD;

	if (NULL == pSlot)
		return CKR_ARGUMENTS_BAD;

	if (NULL != pReserved)
		return CKR_ARGUMENTS_BAD;

	return CKR_NO_EVENT;
}

