/*
 *  PKCS11-MOCK - PKCS#11 mock module
 *  Copyright (c) 2015 JWC s.r.o. <http://www.jwc.sk>
 *  Author: Jaroslav Imrich <jimrich@jimrich.sk>
 *
 *  Licensing for open source projects:
 *  PKCS11-MOCK is available under the terms of the GNU Affero General 
 *  Public License version 3 as published by the Free Software Foundation.
 *  Please see <http://www.gnu.org/licenses/agpl-3.0.html> for more details.
 *
 *  Licensing for other types of projects:
 *  PKCS11-MOCK is available under the terms of flexible commercial license.
 *  Please contact JWC s.r.o. at <info@pkcs11interop.net> for more details.
 */


#define _POSIX_C_SOURCE 200809
#include <config.h>
#include <stdio.h>
#include <string.h>

// PKCS#11 related stuff
#define CK_PTR *
#define CK_DEFINE_FUNCTION(returnType, name) returnType name
#define CK_DECLARE_FUNCTION(returnType, name) returnType name
#define CK_DECLARE_FUNCTION_POINTER(returnType, name) returnType (* name)
#define CK_CALLBACK_FUNCTION(returnType, name) returnType (* name)

#include <p11-kit/pkcs11.h>
#include <p11-kit/pkcs11x.h>

#ifndef NULL_PTR
#define NULL_PTR 0
#endif

#define IGNORE(P) (void)(P)

#define PKCS11_MOCK_CK_INFO_MANUFACTURER_ID "Pkcs11Interop Project"
#define PKCS11_MOCK_CK_INFO_LIBRARY_DESCRIPTION "Mock module"

#define PKCS11_MOCK_CK_SLOT_ID 1
#define PKCS11_MOCK_CK_SLOT_INFO_SLOT_DESCRIPTION "Mock slot"
#define PKCS11_MOCK_CK_SLOT_INFO_MANUFACTURER_ID "Pkcs11Interop Project"

#define PKCS11_MOCK_CK_TOKEN_INFO_LABEL "Pkcs11Interop"
#define PKCS11_MOCK_CK_TOKEN_INFO_MANUFACTURER_ID "Pkcs11Interop Project"
#define PKCS11_MOCK_CK_TOKEN_INFO_MODEL "Mock token"
#define PKCS11_MOCK_CK_TOKEN_INFO_SERIAL_NUMBER "0123456789A"
#define PKCS11_MOCK_CK_TOKEN_INFO_MAX_PIN_LEN 256
#define PKCS11_MOCK_CK_TOKEN_INFO_MIN_PIN_LEN 4

#define PKCS11_MOCK_CK_SESSION_ID 1

#define PKCS11_MOCK_CK_OBJECT_CKA_LABEL "Pkcs11Interop"
#define PKCS11_MOCK_CK_OBJECT_CKA_VALUE "Hello world!"
#define PKCS11_MOCK_CK_OBJECT_SIZE 256
#define PKCS11_MOCK_CK_OBJECT_HANDLE_DATA 1
#define PKCS11_MOCK_CK_OBJECT_HANDLE_SECRET_KEY 2
#define PKCS11_MOCK_CK_OBJECT_HANDLE_PUBLIC_KEY 3
#define PKCS11_MOCK_CK_OBJECT_HANDLE_PRIVATE_KEY 4
#define PKCS11_MOCK_CK_OBJECT_HANDLE_CERTIFICATE_EXTENSION 5
#define PKCS11_MOCK_CK_OBJECT_HANDLE_CERTIFICATE 6

typedef enum
{
	PKCS11_MOCK_CK_OPERATION_NONE,
	PKCS11_MOCK_CK_OPERATION_FIND,
	PKCS11_MOCK_CK_OPERATION_ENCRYPT,
	PKCS11_MOCK_CK_OPERATION_DECRYPT,
	PKCS11_MOCK_CK_OPERATION_DIGEST,
	PKCS11_MOCK_CK_OPERATION_SIGN,
	PKCS11_MOCK_CK_OPERATION_SIGN_RECOVER,
	PKCS11_MOCK_CK_OPERATION_VERIFY,
	PKCS11_MOCK_CK_OPERATION_VERIFY_RECOVER,
	PKCS11_MOCK_CK_OPERATION_DIGEST_ENCRYPT,
	PKCS11_MOCK_CK_OPERATION_DECRYPT_DIGEST,
	PKCS11_MOCK_CK_OPERATION_SIGN_ENCRYPT,
	PKCS11_MOCK_CK_OPERATION_DECRYPT_VERIFY
}
PKCS11_MOCK_CK_OPERATION;

struct find_ptr_st {
	int remaining_data;
	PKCS11_MOCK_CK_OPERATION active_operation;
	CK_OBJECT_HANDLE find_result;
};

typedef struct session_ptr_st {
	char *find_label;
	CK_ULONG state;

	struct find_ptr_st find_op;
} session_ptr_st;

