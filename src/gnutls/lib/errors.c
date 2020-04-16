/*
 * Copyright (C) 2000-2012 Free Software Foundation, Inc.
 *
 * Author: Nikos Mavrogiannopoulos
 *
 * This file is part of GnuTLS.
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
 *
 */

#include "gnutls_int.h"
#include "errors.h"
#include <libtasn1.h>
#ifdef STDC_HEADERS
#include <stdarg.h>
#endif
#include "str.h"

#define ERROR_ENTRY(desc, name) \
	{ desc, #name, name}

struct gnutls_error_entry {
	const char *desc;
	const char *_name;
	int number;
};
typedef struct gnutls_error_entry gnutls_error_entry;

static const gnutls_error_entry error_entries[] = {
	/* "Short Description", Error code define, critical (0,1) -- 1 in most cases */
	ERROR_ENTRY(N_("Could not negotiate a supported cipher suite."),
		    GNUTLS_E_UNKNOWN_CIPHER_SUITE),
	ERROR_ENTRY(N_("No or insufficient priorities were set."),
		    GNUTLS_E_NO_PRIORITIES_WERE_SET),
	ERROR_ENTRY(N_("The cipher type is unsupported."),
		    GNUTLS_E_UNKNOWN_CIPHER_TYPE),
	ERROR_ENTRY(N_("The certificate and the given key do not match."),
		    GNUTLS_E_CERTIFICATE_KEY_MISMATCH),
	ERROR_ENTRY(N_
		    ("Could not negotiate a supported compression method."),
		    GNUTLS_E_UNKNOWN_COMPRESSION_ALGORITHM),
	ERROR_ENTRY(N_("An unknown public key algorithm was encountered."),
		    GNUTLS_E_UNKNOWN_PK_ALGORITHM),

	ERROR_ENTRY(N_("An algorithm that is not enabled was negotiated."),
		    GNUTLS_E_UNWANTED_ALGORITHM),
	ERROR_ENTRY(N_
		    ("A packet with illegal or unsupported version was received."),
		    GNUTLS_E_UNSUPPORTED_VERSION_PACKET),
	ERROR_ENTRY(N_
		    ("The Diffie-Hellman prime sent by the server is not acceptable (not long enough)."),
		    GNUTLS_E_DH_PRIME_UNACCEPTABLE),
	ERROR_ENTRY(N_
		    ("Error decoding the received TLS packet."),
		    GNUTLS_E_UNEXPECTED_PACKET_LENGTH),
	ERROR_ENTRY(N_
		    ("A TLS record packet with invalid length was received."),
		    GNUTLS_E_RECORD_OVERFLOW),
	ERROR_ENTRY(N_("The TLS connection was non-properly terminated."),
		    GNUTLS_E_PREMATURE_TERMINATION),
	ERROR_ENTRY(N_
		    ("The specified session has been invalidated for some reason."),
		    GNUTLS_E_INVALID_SESSION),

	ERROR_ENTRY(N_("GnuTLS internal error."), GNUTLS_E_INTERNAL_ERROR),
	ERROR_ENTRY(N_(
		    "A connection with inappropriate fallback was attempted."),
		    GNUTLS_E_INAPPROPRIATE_FALLBACK),
	ERROR_ENTRY(N_("An illegal TLS extension was received."),
		    GNUTLS_E_RECEIVED_ILLEGAL_EXTENSION),
	ERROR_ENTRY(N_("An required TLS extension was received."),
		    GNUTLS_E_MISSING_EXTENSION),
	ERROR_ENTRY(N_("A TLS fatal alert has been received."),
		    GNUTLS_E_FATAL_ALERT_RECEIVED),
	ERROR_ENTRY(N_("An unexpected TLS packet was received."),
		    GNUTLS_E_UNEXPECTED_PACKET),
	ERROR_ENTRY(N_("Failed to import the key into store."),
		    GNUTLS_E_KEY_IMPORT_FAILED),
	ERROR_ENTRY(N_
		    ("An error was encountered at the TLS Finished packet calculation."),
		    GNUTLS_E_ERROR_IN_FINISHED_PACKET),
	ERROR_ENTRY(N_("No certificate was found."),
		    GNUTLS_E_NO_CERTIFICATE_FOUND),
	ERROR_ENTRY(N_("Certificate is required."),
		    GNUTLS_E_CERTIFICATE_REQUIRED),
	ERROR_ENTRY(N_
		    ("The given DSA key is incompatible with the selected TLS protocol."),
		    GNUTLS_E_INCOMPAT_DSA_KEY_WITH_TLS_PROTOCOL),
	ERROR_ENTRY(N_
		    ("There is already a crypto algorithm with lower priority."),
		    GNUTLS_E_CRYPTO_ALREADY_REGISTERED),

	ERROR_ENTRY(N_("No temporary RSA parameters were found."),
		    GNUTLS_E_NO_TEMPORARY_RSA_PARAMS),
	ERROR_ENTRY(N_("No temporary DH parameters were found."),
		    GNUTLS_E_NO_TEMPORARY_DH_PARAMS),
	ERROR_ENTRY(N_("An unexpected TLS handshake packet was received."),
		    GNUTLS_E_UNEXPECTED_HANDSHAKE_PACKET),
	ERROR_ENTRY(N_("The scanning of a large integer has failed."),
		    GNUTLS_E_MPI_SCAN_FAILED),
	ERROR_ENTRY(N_("Could not export a large integer."),
		    GNUTLS_E_MPI_PRINT_FAILED),
	ERROR_ENTRY(N_("Decryption has failed."),
		    GNUTLS_E_DECRYPTION_FAILED),
	ERROR_ENTRY(N_("Encryption has failed."),
		    GNUTLS_E_ENCRYPTION_FAILED),
	ERROR_ENTRY(N_("Public key decryption has failed."),
		    GNUTLS_E_PK_DECRYPTION_FAILED),
	ERROR_ENTRY(N_("Public key encryption has failed."),
		    GNUTLS_E_PK_ENCRYPTION_FAILED),
	ERROR_ENTRY(N_("Public key signing has failed."),
		    GNUTLS_E_PK_SIGN_FAILED),
	ERROR_ENTRY(N_("Public key signature verification has failed."),
		    GNUTLS_E_PK_SIG_VERIFY_FAILED),
	ERROR_ENTRY(N_
		    ("Decompression of the TLS record packet has failed."),
		    GNUTLS_E_DECOMPRESSION_FAILED),
	ERROR_ENTRY(N_("Compression of the TLS record packet has failed."),
		    GNUTLS_E_COMPRESSION_FAILED),

	ERROR_ENTRY(N_("Internal error in memory allocation."),
		    GNUTLS_E_MEMORY_ERROR),
	ERROR_ENTRY(N_
		    ("An unimplemented or disabled feature has been requested."),
		    GNUTLS_E_UNIMPLEMENTED_FEATURE),
	ERROR_ENTRY(N_("Insufficient credentials for that request."),
		    GNUTLS_E_INSUFFICIENT_CREDENTIALS),
	ERROR_ENTRY(N_("Error in password/key file."), GNUTLS_E_SRP_PWD_ERROR),
	ERROR_ENTRY(N_("Wrong padding in PKCS1 packet."),
		    GNUTLS_E_PKCS1_WRONG_PAD),
	ERROR_ENTRY(N_("The session or certificate has expired."),
		    GNUTLS_E_EXPIRED),
	ERROR_ENTRY(N_("The certificate is not yet activated."),
		    GNUTLS_E_NOT_YET_ACTIVATED),
	ERROR_ENTRY(N_("Hashing has failed."), GNUTLS_E_HASH_FAILED),
	ERROR_ENTRY(N_("Base64 decoding error."),
		    GNUTLS_E_BASE64_DECODING_ERROR),
	ERROR_ENTRY(N_("Base64 unexpected header error."),
		    GNUTLS_E_BASE64_UNEXPECTED_HEADER_ERROR),
	ERROR_ENTRY(N_("Base64 encoding error."),
		    GNUTLS_E_BASE64_ENCODING_ERROR),
	ERROR_ENTRY(N_("Parsing error in password/key file."),
		    GNUTLS_E_SRP_PWD_PARSING_ERROR),
	ERROR_ENTRY(N_("The requested data were not available."),
		    GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE),
	ERROR_ENTRY(N_("There are no embedded data in the structure."),
		    GNUTLS_E_NO_EMBEDDED_DATA),
	ERROR_ENTRY(N_("Error in the pull function."), GNUTLS_E_PULL_ERROR),
	ERROR_ENTRY(N_("Error in the push function."), GNUTLS_E_PUSH_ERROR),
	ERROR_ENTRY(N_
		    ("The upper limit of record packet sequence numbers has been reached. Wow!"),
		    GNUTLS_E_RECORD_LIMIT_REACHED),
	ERROR_ENTRY(N_("Error in the certificate."),
		    GNUTLS_E_CERTIFICATE_ERROR),
	ERROR_ENTRY(N_("Error in the time fields of certificate."),
		    GNUTLS_E_CERTIFICATE_TIME_ERROR),
	ERROR_ENTRY(N_("Error in the certificate verification."),
		    GNUTLS_E_CERTIFICATE_VERIFICATION_ERROR),
	ERROR_ENTRY(N_("Error in the CRL verification."),
		    GNUTLS_E_CRL_VERIFICATION_ERROR),
	ERROR_ENTRY(N_("Error in the private key verification; seed doesn't match."),
		    GNUTLS_E_PRIVKEY_VERIFICATION_ERROR),
	ERROR_ENTRY(N_("Could not authenticate peer."),
		    GNUTLS_E_AUTH_ERROR),
	ERROR_ENTRY(N_
		    ("Unknown Subject Alternative name in X.509 certificate."),
		    GNUTLS_E_X509_UNKNOWN_SAN),
	ERROR_ENTRY(N_
			("CIDR name constraint is malformed in size or structure."),
			GNUTLS_E_MALFORMED_CIDR),

	ERROR_ENTRY(N_
		    ("Unsupported critical extension in X.509 certificate."),
		    GNUTLS_E_X509_UNSUPPORTED_CRITICAL_EXTENSION),
	ERROR_ENTRY(N_("Unsupported extension in X.509 certificate."),
		    GNUTLS_E_X509_UNSUPPORTED_EXTENSION),
	ERROR_ENTRY(N_("Duplicate extension in X.509 certificate."),
		    GNUTLS_E_X509_DUPLICATE_EXTENSION),
	ERROR_ENTRY(N_
		    ("Key usage violation in certificate has been detected."),
		    GNUTLS_E_KEY_USAGE_VIOLATION),
	ERROR_ENTRY(N_("Function was interrupted."), GNUTLS_E_INTERRUPTED),
	ERROR_ENTRY(N_
		    ("TLS Application data were received, while expecting handshake data."),
		    GNUTLS_E_GOT_APPLICATION_DATA),
	ERROR_ENTRY(N_("Error in Database backend."), GNUTLS_E_DB_ERROR),
	ERROR_ENTRY(N_("The Database entry already exists."), GNUTLS_E_DB_ENTRY_EXISTS),
	ERROR_ENTRY(N_("The certificate type is not supported."),
		    GNUTLS_E_UNSUPPORTED_CERTIFICATE_TYPE),
	ERROR_ENTRY(N_
		    ("The given memory buffer is too short to hold parameters."),
		    GNUTLS_E_SHORT_MEMORY_BUFFER),
	ERROR_ENTRY(N_("The request is invalid."),
		    GNUTLS_E_INVALID_REQUEST),
	ERROR_ENTRY(N_("The cookie was bad."), GNUTLS_E_BAD_COOKIE),
	ERROR_ENTRY(N_("An illegal parameter has been received."),
		    GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER),
	ERROR_ENTRY(N_("An illegal parameter was found."),
		    GNUTLS_E_ILLEGAL_PARAMETER),
	ERROR_ENTRY(N_("Error while reading file."), GNUTLS_E_FILE_ERROR),
	ERROR_ENTRY(N_("A disallowed SNI server name has been received."),
		    GNUTLS_E_RECEIVED_DISALLOWED_NAME),

	ERROR_ENTRY(N_("ASN1 parser: Element was not found."),
		    GNUTLS_E_ASN1_ELEMENT_NOT_FOUND),
	ERROR_ENTRY(N_("ASN1 parser: Identifier was not found"),
		    GNUTLS_E_ASN1_IDENTIFIER_NOT_FOUND),
	ERROR_ENTRY(N_("ASN1 parser: Error in DER parsing."),
		    GNUTLS_E_ASN1_DER_ERROR),
	ERROR_ENTRY(N_("ASN1 parser: Value was not found."),
		    GNUTLS_E_ASN1_VALUE_NOT_FOUND),
	ERROR_ENTRY(N_("ASN1 parser: Generic parsing error."),
		    GNUTLS_E_ASN1_GENERIC_ERROR),
	ERROR_ENTRY(N_("ASN1 parser: Value is not valid."),
		    GNUTLS_E_ASN1_VALUE_NOT_VALID),
	ERROR_ENTRY(N_("ASN1 parser: Error in TAG."),
		    GNUTLS_E_ASN1_TAG_ERROR),
	ERROR_ENTRY(N_("ASN1 parser: error in implicit tag"),
		    GNUTLS_E_ASN1_TAG_IMPLICIT),
	ERROR_ENTRY(N_("ASN1 parser: Error in type 'ANY'."),
		    GNUTLS_E_ASN1_TYPE_ANY_ERROR),
	ERROR_ENTRY(N_("ASN1 parser: Syntax error."),
		    GNUTLS_E_ASN1_SYNTAX_ERROR),
	ERROR_ENTRY(N_("ASN1 parser: Overflow in DER parsing."),
		    GNUTLS_E_ASN1_DER_OVERFLOW),

	ERROR_ENTRY(N_
		    ("Too many empty record packets have been received."),
		    GNUTLS_E_TOO_MANY_EMPTY_PACKETS),
	ERROR_ENTRY(N_("Too many handshake packets have been received."),
		    GNUTLS_E_TOO_MANY_HANDSHAKE_PACKETS),
	ERROR_ENTRY(N_("More than a single object matches the criteria."),
		    GNUTLS_E_TOO_MANY_MATCHES),
	ERROR_ENTRY(N_("The crypto library version is too old."),
		    GNUTLS_E_INCOMPATIBLE_GCRYPT_LIBRARY),

	ERROR_ENTRY(N_("The tasn1 library version is too old."),
		    GNUTLS_E_INCOMPATIBLE_LIBTASN1_LIBRARY),
	ERROR_ENTRY(N_("The OpenPGP User ID is revoked."),
		    GNUTLS_E_OPENPGP_UID_REVOKED),
	ERROR_ENTRY(N_("The OpenPGP key has not a preferred key set."),
		    GNUTLS_E_OPENPGP_PREFERRED_KEY_ERROR),
	ERROR_ENTRY(N_("Error loading the keyring."),
		    GNUTLS_E_OPENPGP_KEYRING_ERROR),
	ERROR_ENTRY(N_("The initialization of crypto backend has failed."),
		    GNUTLS_E_CRYPTO_INIT_FAILED),
	ERROR_ENTRY(N_
		    ("No supported compression algorithms have been found."),
		    GNUTLS_E_NO_COMPRESSION_ALGORITHMS),
	ERROR_ENTRY(N_("No supported cipher suites have been found."),
		    GNUTLS_E_NO_CIPHER_SUITES),
	ERROR_ENTRY(N_("Could not get OpenPGP key."),
		    GNUTLS_E_OPENPGP_GETKEY_FAILED),
	ERROR_ENTRY(N_("Could not find OpenPGP subkey."),
		    GNUTLS_E_OPENPGP_SUBKEY_ERROR),
	ERROR_ENTRY(N_("Safe renegotiation failed."),
		    GNUTLS_E_SAFE_RENEGOTIATION_FAILED),
	ERROR_ENTRY(N_("Unsafe renegotiation denied."),
		    GNUTLS_E_UNSAFE_RENEGOTIATION_DENIED),

	ERROR_ENTRY(N_("The SRP username supplied is illegal."),
		    GNUTLS_E_ILLEGAL_SRP_USERNAME),
	ERROR_ENTRY(N_("The username supplied is unknown."),
		    GNUTLS_E_UNKNOWN_SRP_USERNAME),

	ERROR_ENTRY(N_("The OpenPGP fingerprint is not supported."),
		    GNUTLS_E_OPENPGP_FINGERPRINT_UNSUPPORTED),
	ERROR_ENTRY(N_("The signature algorithm is not supported."),
		    GNUTLS_E_UNSUPPORTED_SIGNATURE_ALGORITHM),
	ERROR_ENTRY(N_("The certificate has unsupported attributes."),
		    GNUTLS_E_X509_UNSUPPORTED_ATTRIBUTE),
	ERROR_ENTRY(N_("The OID is not supported."),
		    GNUTLS_E_X509_UNSUPPORTED_OID),
	ERROR_ENTRY(N_("The hash algorithm is unknown."),
		    GNUTLS_E_UNKNOWN_HASH_ALGORITHM),
	ERROR_ENTRY(N_("The PKCS structure's content type is unknown."),
		    GNUTLS_E_UNKNOWN_PKCS_CONTENT_TYPE),
	ERROR_ENTRY(N_("The PKCS structure's bag type is unknown."),
		    GNUTLS_E_UNKNOWN_PKCS_BAG_TYPE),
	ERROR_ENTRY(N_("The given password contains invalid characters."),
		    GNUTLS_E_INVALID_PASSWORD),
	ERROR_ENTRY(N_("The given string contains invalid UTF-8 characters."),
		    GNUTLS_E_INVALID_UTF8_STRING),
	ERROR_ENTRY(N_("The given email string contains non-ASCII characters before '@'."),
		    GNUTLS_E_INVALID_UTF8_EMAIL),
	ERROR_ENTRY(N_("The given password contains invalid characters."),
		    GNUTLS_E_INVALID_PASSWORD_STRING),
	ERROR_ENTRY(N_
		    ("The Message Authentication Code verification failed."),
		    GNUTLS_E_MAC_VERIFY_FAILED),
	ERROR_ENTRY(N_("Some constraint limits were reached."),
		    GNUTLS_E_CONSTRAINT_ERROR),
	ERROR_ENTRY(N_("Failed to acquire random data."),
		    GNUTLS_E_RANDOM_FAILED),
	ERROR_ENTRY(N_("Verifying TLS/IA phase checksum failed"),
		    GNUTLS_E_IA_VERIFY_FAILED),

	ERROR_ENTRY(N_("The specified algorithm or protocol is unknown."),
		    GNUTLS_E_UNKNOWN_ALGORITHM),

	ERROR_ENTRY(N_("The handshake data size is too large."),
		    GNUTLS_E_HANDSHAKE_TOO_LARGE),

	ERROR_ENTRY(N_("Error opening /dev/crypto"),
		    GNUTLS_E_CRYPTODEV_DEVICE_ERROR),

	ERROR_ENTRY(N_("Error interfacing with /dev/crypto"),
		    GNUTLS_E_CRYPTODEV_IOCTL_ERROR),
	ERROR_ENTRY(N_("Peer has terminated the connection"),
		    GNUTLS_E_SESSION_EOF),
	ERROR_ENTRY(N_("Channel binding data not available"),
		    GNUTLS_E_CHANNEL_BINDING_NOT_AVAILABLE),

	ERROR_ENTRY(N_("TPM error."),
		    GNUTLS_E_TPM_ERROR),
	ERROR_ENTRY(N_("The TPM library (trousers) cannot be found."),
		    GNUTLS_E_TPM_NO_LIB),
	ERROR_ENTRY(N_("TPM is not initialized."),
		    GNUTLS_E_TPM_UNINITIALIZED),
	ERROR_ENTRY(N_("TPM key was not found in persistent storage."),
		    GNUTLS_E_TPM_KEY_NOT_FOUND),
	ERROR_ENTRY(N_("Cannot initialize a session with the TPM."),
		    GNUTLS_E_TPM_SESSION_ERROR),
	ERROR_ENTRY(N_("PKCS #11 error."),
		    GNUTLS_E_PKCS11_ERROR),
	ERROR_ENTRY(N_("PKCS #11 initialization error."),
		    GNUTLS_E_PKCS11_LOAD_ERROR),
	ERROR_ENTRY(N_("Error in parsing."),
		    GNUTLS_E_PARSING_ERROR),
	ERROR_ENTRY(N_("Error in provided PIN."),
		    GNUTLS_E_PKCS11_PIN_ERROR),
	ERROR_ENTRY(N_("Error in provided SRK password for TPM."),
		    GNUTLS_E_TPM_SRK_PASSWORD_ERROR),
	ERROR_ENTRY(N_
		    ("Error in provided password for key to be loaded in TPM."),
		    GNUTLS_E_TPM_KEY_PASSWORD_ERROR),
	ERROR_ENTRY(N_("PKCS #11 error in slot"),
		    GNUTLS_E_PKCS11_SLOT_ERROR),
	ERROR_ENTRY(N_("Thread locking error"),
		    GNUTLS_E_LOCKING_ERROR),
	ERROR_ENTRY(N_("PKCS #11 error in attribute"),
		    GNUTLS_E_PKCS11_ATTRIBUTE_ERROR),
	ERROR_ENTRY(N_("PKCS #11 error in device"),
		    GNUTLS_E_PKCS11_DEVICE_ERROR),
	ERROR_ENTRY(N_("PKCS #11 error in data"),
		    GNUTLS_E_PKCS11_DATA_ERROR),
	ERROR_ENTRY(N_("PKCS #11 unsupported feature"),
		    GNUTLS_E_PKCS11_UNSUPPORTED_FEATURE_ERROR),
	ERROR_ENTRY(N_("PKCS #11 error in key"),
		    GNUTLS_E_PKCS11_KEY_ERROR),
	ERROR_ENTRY(N_("PKCS #11 PIN expired"),
		    GNUTLS_E_PKCS11_PIN_EXPIRED),
	ERROR_ENTRY(N_("PKCS #11 PIN locked"),
		    GNUTLS_E_PKCS11_PIN_LOCKED),
	ERROR_ENTRY(N_("PKCS #11 error in session"),
		    GNUTLS_E_PKCS11_SESSION_ERROR),
	ERROR_ENTRY(N_("PKCS #11 error in signature"),
		    GNUTLS_E_PKCS11_SIGNATURE_ERROR),
	ERROR_ENTRY(N_("PKCS #11 error in token"),
		    GNUTLS_E_PKCS11_TOKEN_ERROR),
	ERROR_ENTRY(N_("PKCS #11 user error"),
		    GNUTLS_E_PKCS11_USER_ERROR),
	ERROR_ENTRY(N_("The operation timed out"),
		    GNUTLS_E_TIMEDOUT),
	ERROR_ENTRY(N_("The operation was cancelled due to user error"),
		    GNUTLS_E_USER_ERROR),
	ERROR_ENTRY(N_("No supported ECC curves were found"),
		    GNUTLS_E_ECC_NO_SUPPORTED_CURVES),
	ERROR_ENTRY(N_("The curve is unsupported"),
		    GNUTLS_E_ECC_UNSUPPORTED_CURVE),
	ERROR_ENTRY(N_("The requested PKCS #11 object is not available"),
		    GNUTLS_E_PKCS11_REQUESTED_OBJECT_NOT_AVAILBLE),
	ERROR_ENTRY(N_
		    ("The provided X.509 certificate list is not sorted (in subject to issuer order)"),
		    GNUTLS_E_CERTIFICATE_LIST_UNSORTED),
	ERROR_ENTRY(N_("The OCSP response is invalid"),
		    GNUTLS_E_OCSP_RESPONSE_ERROR),
	ERROR_ENTRY(N_("The OCSP response provided doesn't match the available certificates"),
		    GNUTLS_E_OCSP_MISMATCH_WITH_CERTS),
	ERROR_ENTRY(N_("There is no certificate status (OCSP)."),
		    GNUTLS_E_NO_CERTIFICATE_STATUS),
	ERROR_ENTRY(N_("Error in the system's randomness device."),
		    GNUTLS_E_RANDOM_DEVICE_ERROR),
	ERROR_ENTRY(N_
		    ("No common application protocol could be negotiated."),
		    GNUTLS_E_NO_APPLICATION_PROTOCOL),
	ERROR_ENTRY(N_("Error while performing self checks."),
		    GNUTLS_E_SELF_TEST_ERROR),
	ERROR_ENTRY(N_("There is no self test for this algorithm."),
		    GNUTLS_E_NO_SELF_TEST),
	ERROR_ENTRY(N_("An error has been detected in the library and cannot continue operations."),
		    GNUTLS_E_LIB_IN_ERROR_STATE),
	ERROR_ENTRY(N_("Error in sockets initialization."),
		    GNUTLS_E_SOCKETS_INIT_ERROR),
	ERROR_ENTRY(N_("Error in public key generation."),
		    GNUTLS_E_PK_GENERATION_ERROR),
	ERROR_ENTRY(N_("Invalid TLS extensions length field."),
		    GNUTLS_E_UNEXPECTED_EXTENSIONS_LENGTH),
	ERROR_ENTRY(N_("Peer's certificate or username has changed during a rehandshake."),
		    GNUTLS_E_SESSION_USER_ID_CHANGED),
	ERROR_ENTRY(N_("The provided string has an embedded null."),
		    GNUTLS_E_ASN1_EMBEDDED_NULL_IN_STRING),
	ERROR_ENTRY(N_("Attempted handshake during false start."),
		    GNUTLS_E_HANDSHAKE_DURING_FALSE_START),
	ERROR_ENTRY(N_("The SNI host name not recognised."),
		    GNUTLS_E_UNRECOGNIZED_NAME),
	ERROR_ENTRY(N_("There was an issue converting to or from UTF8."),
		    GNUTLS_E_IDNA_ERROR),
	ERROR_ENTRY(N_("Cannot perform this action while handshake is in progress."),
		    GNUTLS_E_UNAVAILABLE_DURING_HANDSHAKE),
	ERROR_ENTRY(N_("The public key is invalid."),
		    GNUTLS_E_PK_INVALID_PUBKEY),
	ERROR_ENTRY(N_("There are no validation parameters present."),
		    GNUTLS_E_PK_NO_VALIDATION_PARAMS),
	ERROR_ENTRY(N_("The public key parameters are invalid."),
		    GNUTLS_E_PK_INVALID_PUBKEY_PARAMS),
	ERROR_ENTRY(N_("The private key is invalid."),
		    GNUTLS_E_PK_INVALID_PRIVKEY),
	ERROR_ENTRY(N_("The DER time encoding is invalid."),
		    GNUTLS_E_ASN1_TIME_ERROR),
	ERROR_ENTRY(N_("The signature is incompatible with the public key."),
		    GNUTLS_E_INCOMPATIBLE_SIG_WITH_KEY),
	ERROR_ENTRY(N_("One of the involved algorithms has insufficient security level."),
		    GNUTLS_E_INSUFFICIENT_SECURITY),
	ERROR_ENTRY(N_("No common key share with peer."),
		    GNUTLS_E_NO_COMMON_KEY_SHARE),
	ERROR_ENTRY(N_("The early data were rejected."),
		    GNUTLS_E_EARLY_DATA_REJECTED),
	{NULL, NULL, 0}
};

static const gnutls_error_entry non_fatal_error_entries[] = {
	ERROR_ENTRY(N_("Success."), GNUTLS_E_SUCCESS),
	ERROR_ENTRY(N_("A TLS warning alert has been received."),
		    GNUTLS_E_WARNING_ALERT_RECEIVED),
	ERROR_ENTRY(N_("A heartbeat pong message was received."),
		    GNUTLS_E_HEARTBEAT_PONG_RECEIVED),
	ERROR_ENTRY(N_("A heartbeat ping message was received."),
		    GNUTLS_E_HEARTBEAT_PING_RECEIVED),
	ERROR_ENTRY(N_("Resource temporarily unavailable, try again."),
		    GNUTLS_E_AGAIN),
	ERROR_ENTRY(N_("The transmitted packet is too large (EMSGSIZE)."),
		    GNUTLS_E_LARGE_PACKET),
	ERROR_ENTRY(N_("Function was interrupted."), GNUTLS_E_INTERRUPTED),
	ERROR_ENTRY(N_("Rehandshake was requested by the peer."),
		    GNUTLS_E_REHANDSHAKE),
	ERROR_ENTRY(N_("Re-authentication was requested by the peer."),
		    GNUTLS_E_REAUTH_REQUEST),
	/* Only non fatal (for handshake) errors here */
	{NULL, NULL, 0}
};

/**
 * gnutls_error_is_fatal:
 * @error: is a GnuTLS error code, a negative error code
 *
 * If a GnuTLS function returns a negative error code you may feed that
 * value to this function to see if the error condition is fatal to
 * a TLS session (i.e., must be terminated).
 *
 * Note that you may also want to check the error code manually, since some
 * non-fatal errors to the protocol (such as a warning alert or
 * a rehandshake request) may be fatal for your program.
 *
 * This function is only useful if you are dealing with errors from
 * functions that relate to a TLS session (e.g., record layer or handshake
 * layer handling functions).
 *
 * Returns: Non-zero value on fatal errors or zero on non-fatal.
 **/
int gnutls_error_is_fatal(int error)
{
	int ret = 1;
	const gnutls_error_entry *p;

	/* Input sanitzation.  Positive values are not errors at all, and
	   definitely not fatal. */
	if (error > 0)
		return 0;

	for (p = non_fatal_error_entries; p->desc != NULL; p++) {
		if (p->number == error) {
			ret = 0;
			break;
		}
	}

	return ret;
}

/**
 * gnutls_perror:
 * @error: is a GnuTLS error code, a negative error code
 *
 * This function is like perror(). The only difference is that it
 * accepts an error number returned by a gnutls function.
 **/
void gnutls_perror(int error)
{
	fprintf(stderr, "GnuTLS error: %s\n", gnutls_strerror(error));
}


/**
 * gnutls_strerror:
 * @error: is a GnuTLS error code, a negative error code
 *
 * This function is similar to strerror.  The difference is that it
 * accepts an error number returned by a gnutls function; In case of
 * an unknown error a descriptive string is sent instead of %NULL.
 *
 * Error codes are always a negative error code.
 *
 * Returns: A string explaining the GnuTLS error message.
 **/
const char *gnutls_strerror(int error)
{
	const char *ret = NULL;
	const gnutls_error_entry *p;

	for (p = error_entries; p->desc != NULL; p++) {
		if (p->number == error) {
			ret = p->desc;
			break;
		}
	}

	if (ret == NULL) {
		for (p = non_fatal_error_entries; p->desc != NULL; p++) {
			if (p->number == error) {
				ret = p->desc;
				break;
			}
		}
	}

	/* avoid prefix */
	if (ret == NULL)
		return _("(unknown error code)");

	return _(ret);
}

/**
 * gnutls_strerror_name:
 * @error: is an error returned by a gnutls function.
 *
 * Return the GnuTLS error code define as a string.  For example,
 * gnutls_strerror_name (GNUTLS_E_DH_PRIME_UNACCEPTABLE) will return
 * the string "GNUTLS_E_DH_PRIME_UNACCEPTABLE".
 *
 * Returns: A string corresponding to the symbol name of the error
 * code.
 *
 * Since: 2.6.0
 **/
const char *gnutls_strerror_name(int error)
{
	const char *ret = NULL;
	const gnutls_error_entry *p;

	for (p = error_entries; p->desc != NULL; p++) {
		if (p->number == error) {
			ret = p->_name;
			break;
		}
	}

	if (ret == NULL) {
		for (p = non_fatal_error_entries; p->desc != NULL; p++) {
			if (p->number == error) {
				ret = p->_name;
				break;
			}
		}
	}

	return ret;
}

int _gnutls_asn2err(int asn_err)
{
	switch (asn_err) {
#ifdef ASN1_TIME_ENCODING_ERROR
	case ASN1_TIME_ENCODING_ERROR:
		return GNUTLS_E_ASN1_TIME_ERROR;
#endif
	case ASN1_FILE_NOT_FOUND:
		return GNUTLS_E_FILE_ERROR;
	case ASN1_ELEMENT_NOT_FOUND:
		return GNUTLS_E_ASN1_ELEMENT_NOT_FOUND;
	case ASN1_IDENTIFIER_NOT_FOUND:
		return GNUTLS_E_ASN1_IDENTIFIER_NOT_FOUND;
	case ASN1_DER_ERROR:
		return GNUTLS_E_ASN1_DER_ERROR;
	case ASN1_VALUE_NOT_FOUND:
		return GNUTLS_E_ASN1_VALUE_NOT_FOUND;
	case ASN1_GENERIC_ERROR:
		return GNUTLS_E_ASN1_GENERIC_ERROR;
	case ASN1_VALUE_NOT_VALID:
		return GNUTLS_E_ASN1_VALUE_NOT_VALID;
	case ASN1_TAG_ERROR:
		return GNUTLS_E_ASN1_TAG_ERROR;
	case ASN1_TAG_IMPLICIT:
		return GNUTLS_E_ASN1_TAG_IMPLICIT;
	case ASN1_ERROR_TYPE_ANY:
		return GNUTLS_E_ASN1_TYPE_ANY_ERROR;
	case ASN1_SYNTAX_ERROR:
		return GNUTLS_E_ASN1_SYNTAX_ERROR;
	case ASN1_MEM_ERROR:
		return GNUTLS_E_SHORT_MEMORY_BUFFER;
	case ASN1_MEM_ALLOC_ERROR:
		return GNUTLS_E_MEMORY_ERROR;
	case ASN1_DER_OVERFLOW:
		return GNUTLS_E_ASN1_DER_OVERFLOW;
	default:
		return GNUTLS_E_ASN1_GENERIC_ERROR;
	}
}

void _gnutls_mpi_log(const char *prefix, bigint_t a)
{
	size_t binlen = 0;
	void *binbuf;
	size_t hexlen;
	char *hexbuf;
	int res;

	if (_gnutls_log_level < 2)
		return;

	res = _gnutls_mpi_print(a, NULL, &binlen);
	if (res < 0 && res != GNUTLS_E_SHORT_MEMORY_BUFFER) {
		gnutls_assert();
		_gnutls_hard_log("MPI: %s can't print value (%d/%d)\n",
				 prefix, res, (int) binlen);
		return;
	}

	if (binlen > 1024 * 1024) {
		gnutls_assert();
		_gnutls_hard_log("MPI: %s too large mpi (%d)\n", prefix,
				 (int) binlen);
		return;
	}

	binbuf = gnutls_malloc(binlen);
	if (!binbuf) {
		gnutls_assert();
		_gnutls_hard_log("MPI: %s out of memory (%d)\n", prefix,
				 (int) binlen);
		return;
	}

	res = _gnutls_mpi_print(a, binbuf, &binlen);
	if (res != 0) {
		gnutls_assert();
		_gnutls_hard_log("MPI: %s can't print value (%d/%d)\n",
				 prefix, res, (int) binlen);
		gnutls_free(binbuf);
		return;
	}

	hexlen = 2 * binlen + 1;
	hexbuf = gnutls_malloc(hexlen);

	if (!hexbuf) {
		gnutls_assert();
		_gnutls_hard_log("MPI: %s out of memory (hex %d)\n",
				 prefix, (int) hexlen);
		gnutls_free(binbuf);
		return;
	}

	_gnutls_bin2hex(binbuf, binlen, hexbuf, hexlen, NULL);

	_gnutls_hard_log("MPI: length: %d\n\t%s%s\n", (int) binlen, prefix,
			 hexbuf);

	gnutls_free(hexbuf);
	gnutls_free(binbuf);
}

/* this function will output a message using the
 * caller provided function
 */
void _gnutls_log(int level, const char *fmt, ...)
{
	va_list args;
	char *str;
	int ret;

	if (_gnutls_log_func == NULL)
		return;

	va_start(args, fmt);
	ret = vasprintf(&str, fmt, args);
	va_end(args);

	if (ret >= 0) {
		_gnutls_log_func(level, str);
		free(str);
	}
}

void _gnutls_audit_log(gnutls_session_t session, const char *fmt, ...)
{
	va_list args;
	char *str;
	int ret;

	if (_gnutls_audit_log_func == NULL && _gnutls_log_func == NULL)
		return;

	va_start(args, fmt);
	ret = vasprintf(&str, fmt, args);
	va_end(args);

	if (ret >= 0) {
		if (_gnutls_audit_log_func)
			_gnutls_audit_log_func(session, str);
		else
			_gnutls_log_func(1, str);
		free(str);
	}
}

#ifndef DEBUG
#ifndef C99_MACROS

/* Without C99 macros these functions have to
 * be called. This may affect performance.
 */
void _gnutls_null_log(void *x, ...)
{
	return;
}

#endif				/* C99_MACROS */
#endif				/* DEBUG */
