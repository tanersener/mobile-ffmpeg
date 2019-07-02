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
#include <record.h>
#include <debug.h>
#include "str.h"

typedef struct {
	gnutls_alert_description_t alert;
	const char *name;
	const char *desc;
} gnutls_alert_entry;

#define ALERT_ENTRY(x,y) \
  {x, #x, y}

static const gnutls_alert_entry sup_alerts[] = {
	ALERT_ENTRY(GNUTLS_A_CLOSE_NOTIFY, N_("Close notify")),
	ALERT_ENTRY(GNUTLS_A_UNEXPECTED_MESSAGE, N_("Unexpected message")),
	ALERT_ENTRY(GNUTLS_A_BAD_RECORD_MAC, N_("Bad record MAC")),
	ALERT_ENTRY(GNUTLS_A_DECRYPTION_FAILED, N_("Decryption failed")),
	ALERT_ENTRY(GNUTLS_A_RECORD_OVERFLOW, N_("Record overflow")),
	ALERT_ENTRY(GNUTLS_A_DECOMPRESSION_FAILURE,
		    N_("Decompression failed")),
	ALERT_ENTRY(GNUTLS_A_HANDSHAKE_FAILURE, N_("Handshake failed")),
	ALERT_ENTRY(GNUTLS_A_BAD_CERTIFICATE, N_("Certificate is bad")),
	ALERT_ENTRY(GNUTLS_A_UNSUPPORTED_CERTIFICATE,
		    N_("Certificate is not supported")),
	ALERT_ENTRY(GNUTLS_A_CERTIFICATE_REVOKED,
		    N_("Certificate was revoked")),
	ALERT_ENTRY(GNUTLS_A_CERTIFICATE_EXPIRED,
		    N_("Certificate is expired")),
	ALERT_ENTRY(GNUTLS_A_CERTIFICATE_UNKNOWN,
		    N_("Unknown certificate")),
	ALERT_ENTRY(GNUTLS_A_ILLEGAL_PARAMETER, N_("Illegal parameter")),
	ALERT_ENTRY(GNUTLS_A_UNKNOWN_CA, N_("CA is unknown")),
	ALERT_ENTRY(GNUTLS_A_ACCESS_DENIED, N_("Access was denied")),
	ALERT_ENTRY(GNUTLS_A_DECODE_ERROR, N_("Decode error")),
	ALERT_ENTRY(GNUTLS_A_DECRYPT_ERROR, N_("Decrypt error")),
	ALERT_ENTRY(GNUTLS_A_EXPORT_RESTRICTION, N_("Export restriction")),
	ALERT_ENTRY(GNUTLS_A_PROTOCOL_VERSION,
		    N_("Error in protocol version")),
	ALERT_ENTRY(GNUTLS_A_INSUFFICIENT_SECURITY,
		    N_("Insufficient security")),
	ALERT_ENTRY(GNUTLS_A_USER_CANCELED, N_("User canceled")),
	ALERT_ENTRY(GNUTLS_A_SSL3_NO_CERTIFICATE,
		    N_("No certificate (SSL 3.0)")),
	ALERT_ENTRY(GNUTLS_A_INTERNAL_ERROR, N_("Internal error")),
	ALERT_ENTRY(GNUTLS_A_INAPPROPRIATE_FALLBACK,
		    N_("Inappropriate fallback")),
	ALERT_ENTRY(GNUTLS_A_NO_RENEGOTIATION,
		    N_("No renegotiation is allowed")),
	ALERT_ENTRY(GNUTLS_A_CERTIFICATE_UNOBTAINABLE,
		    N_("Could not retrieve the specified certificate")),
	ALERT_ENTRY(GNUTLS_A_UNSUPPORTED_EXTENSION,
		    N_("An unsupported extension was sent")),
	ALERT_ENTRY(GNUTLS_A_UNRECOGNIZED_NAME,
		    N_("The server name sent was not recognized")),
	ALERT_ENTRY(GNUTLS_A_UNKNOWN_PSK_IDENTITY,
		    N_("The SRP/PSK username is missing or not known")),
	ALERT_ENTRY(GNUTLS_A_MISSING_EXTENSION,
		    N_("An extension was expected but was not seen")),
	ALERT_ENTRY(GNUTLS_A_NO_APPLICATION_PROTOCOL,
		    N_
		    ("No supported application protocol could be negotiated")),
	ALERT_ENTRY(GNUTLS_A_CERTIFICATE_REQUIRED,
		    N_("Certificate is required")),
	{0, NULL, NULL}
};

/**
 * gnutls_alert_get_name:
 * @alert: is an alert number.
 *
 * This function will return a string that describes the given alert
 * number, or %NULL.  See gnutls_alert_get().
 *
 * Returns: string corresponding to #gnutls_alert_description_t value.
 **/
const char *gnutls_alert_get_name(gnutls_alert_description_t alert)
{
	const gnutls_alert_entry *p;

	for (p = sup_alerts; p->desc != NULL; p++)
		if (p->alert == alert)
			return _(p->desc);

	return NULL;
}

/**
 * gnutls_alert_get_strname:
 * @alert: is an alert number.
 *
 * This function will return a string of the name of the alert.
 *
 * Returns: string corresponding to #gnutls_alert_description_t value.
 *
 * Since: 3.0
 **/
const char *gnutls_alert_get_strname(gnutls_alert_description_t alert)
{
	const gnutls_alert_entry *p;

	for (p = sup_alerts; p->name != NULL; p++)
		if (p->alert == alert)
			return p->name;

	return NULL;
}

/**
 * gnutls_alert_send:
 * @session: is a #gnutls_session_t type.
 * @level: is the level of the alert
 * @desc: is the alert description
 *
 * This function will send an alert to the peer in order to inform
 * him of something important (eg. his Certificate could not be verified).
 * If the alert level is Fatal then the peer is expected to close the
 * connection, otherwise he may ignore the alert and continue.
 *
 * The error code of the underlying record send function will be
 * returned, so you may also receive %GNUTLS_E_INTERRUPTED or
 * %GNUTLS_E_AGAIN as well.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise
 *   an error code is returned.
 **/
int
gnutls_alert_send(gnutls_session_t session, gnutls_alert_level_t level,
		  gnutls_alert_description_t desc)
{
	uint8_t data[2];
	int ret;
	const char *name;

	data[0] = (uint8_t) level;
	data[1] = (uint8_t) desc;

	name = gnutls_alert_get_name((gnutls_alert_description_t) data[1]);
	if (name == NULL)
		name = "(unknown)";
	_gnutls_record_log("REC: Sending Alert[%d|%d] - %s\n", data[0],
			   data[1], name);

	if ((ret =
	     _gnutls_send_int(session, GNUTLS_ALERT, -1,
			      EPOCH_WRITE_CURRENT, data, 2,
			      MBUFFER_FLUSH)) >= 0)
		return 0;
	else
		return ret;
}

/**
 * gnutls_error_to_alert:
 * @err: is a negative integer
 * @level: the alert level will be stored there
 *
 * Get an alert depending on the error code returned by a gnutls
 * function.  All alerts sent by this function should be considered
 * fatal.  The only exception is when @err is %GNUTLS_E_REHANDSHAKE,
 * where a warning alert should be sent to the peer indicating that no
 * renegotiation will be performed.
 *
 * If there is no mapping to a valid alert the alert to indicate
 * internal error (%GNUTLS_A_INTERNAL_ERROR) is returned.
 *
 * Returns: the alert code to use for a particular error code.
 **/
int gnutls_error_to_alert(int err, int *level)
{
	int ret, _level = -1;

	switch (err) {		/* send appropriate alert */
	case GNUTLS_E_PK_SIG_VERIFY_FAILED:
	case GNUTLS_E_ERROR_IN_FINISHED_PACKET:
		ret = GNUTLS_A_DECRYPT_ERROR;
		_level = GNUTLS_AL_FATAL;
		break;
	case GNUTLS_E_DECRYPTION_FAILED:
		/* GNUTLS_A_DECRYPTION_FAILED is not sent, because
		 * it is not defined in SSL3. Note that we must
		 * not distinguish Decryption failures from mac
		 * check failures, due to the possibility of some
		 * attacks.
		 */
		ret = GNUTLS_A_BAD_RECORD_MAC;
		_level = GNUTLS_AL_FATAL;
		break;
	case GNUTLS_E_UNEXPECTED_PACKET_LENGTH:
	case GNUTLS_E_UNEXPECTED_EXTENSIONS_LENGTH:
	case GNUTLS_E_NO_CERTIFICATE_FOUND:
	case GNUTLS_E_HANDSHAKE_TOO_LARGE:
		ret = GNUTLS_A_DECODE_ERROR;
		_level = GNUTLS_AL_FATAL;
		break;
	case GNUTLS_E_DECOMPRESSION_FAILED:
		ret = GNUTLS_A_DECOMPRESSION_FAILURE;
		_level = GNUTLS_AL_FATAL;
		break;
	case GNUTLS_E_ILLEGAL_PARAMETER:
	case GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER:
	case GNUTLS_E_ILLEGAL_SRP_USERNAME:
	case GNUTLS_E_PK_INVALID_PUBKEY:
	case GNUTLS_E_UNKNOWN_COMPRESSION_ALGORITHM:
	case GNUTLS_E_RECEIVED_DISALLOWED_NAME:
		ret = GNUTLS_A_ILLEGAL_PARAMETER;
		_level = GNUTLS_AL_FATAL;
		break;
	case GNUTLS_E_UNKNOWN_SRP_USERNAME:
		ret = GNUTLS_A_UNKNOWN_PSK_IDENTITY;
		_level = GNUTLS_AL_FATAL;
		break;
	case GNUTLS_E_ASN1_ELEMENT_NOT_FOUND:
	case GNUTLS_E_ASN1_IDENTIFIER_NOT_FOUND:
	case GNUTLS_E_ASN1_DER_ERROR:
	case GNUTLS_E_ASN1_VALUE_NOT_FOUND:
	case GNUTLS_E_ASN1_GENERIC_ERROR:
	case GNUTLS_E_ASN1_VALUE_NOT_VALID:
	case GNUTLS_E_ASN1_TAG_ERROR:
	case GNUTLS_E_ASN1_TAG_IMPLICIT:
	case GNUTLS_E_ASN1_TYPE_ANY_ERROR:
	case GNUTLS_E_ASN1_SYNTAX_ERROR:
	case GNUTLS_E_ASN1_DER_OVERFLOW:
	case GNUTLS_E_CERTIFICATE_ERROR:
	case GNUTLS_E_CERTIFICATE_VERIFICATION_ERROR:
		ret = GNUTLS_A_BAD_CERTIFICATE;
		_level = GNUTLS_AL_FATAL;
		break;
	case GNUTLS_E_UNKNOWN_CIPHER_SUITE:
	case GNUTLS_E_INSUFFICIENT_CREDENTIALS:
	case GNUTLS_E_NO_CIPHER_SUITES:
	case GNUTLS_E_NO_COMPRESSION_ALGORITHMS:
	case GNUTLS_E_UNSUPPORTED_SIGNATURE_ALGORITHM:
	case GNUTLS_E_SAFE_RENEGOTIATION_FAILED:
	case GNUTLS_E_INCOMPAT_DSA_KEY_WITH_TLS_PROTOCOL:
	case GNUTLS_E_UNKNOWN_PK_ALGORITHM:
	case GNUTLS_E_UNWANTED_ALGORITHM:
	case GNUTLS_E_NO_COMMON_KEY_SHARE:
	case GNUTLS_E_ECC_NO_SUPPORTED_CURVES:
	case GNUTLS_E_ECC_UNSUPPORTED_CURVE:
		ret = GNUTLS_A_HANDSHAKE_FAILURE;
		_level = GNUTLS_AL_FATAL;
		break;
	case GNUTLS_E_RECEIVED_ILLEGAL_EXTENSION:
		ret = GNUTLS_A_UNSUPPORTED_EXTENSION;
		_level = GNUTLS_AL_FATAL;
		break;
	case GNUTLS_E_MISSING_EXTENSION:
		ret = GNUTLS_A_MISSING_EXTENSION;
		_level = GNUTLS_AL_FATAL;
		break;
	case GNUTLS_E_USER_ERROR:
		ret = GNUTLS_A_USER_CANCELED;
		_level = GNUTLS_AL_FATAL;
		break;
	case GNUTLS_E_UNEXPECTED_PACKET:
	case GNUTLS_E_UNEXPECTED_HANDSHAKE_PACKET:
	case GNUTLS_E_PREMATURE_TERMINATION:
		ret = GNUTLS_A_UNEXPECTED_MESSAGE;
		_level = GNUTLS_AL_FATAL;
		break;
	case GNUTLS_E_REHANDSHAKE:
	case GNUTLS_E_UNSAFE_RENEGOTIATION_DENIED:
		ret = GNUTLS_A_NO_RENEGOTIATION;
		_level = GNUTLS_AL_WARNING;
		break;
	case GNUTLS_E_UNSUPPORTED_VERSION_PACKET:
		ret = GNUTLS_A_PROTOCOL_VERSION;
		_level = GNUTLS_AL_FATAL;
		break;
	case GNUTLS_E_UNSUPPORTED_CERTIFICATE_TYPE:
		ret = GNUTLS_A_UNSUPPORTED_CERTIFICATE;
		_level = GNUTLS_AL_FATAL;
		break;
	case GNUTLS_E_RECORD_OVERFLOW:
		ret = GNUTLS_A_RECORD_OVERFLOW;
		_level = GNUTLS_AL_FATAL;
		break;
	case GNUTLS_E_INTERNAL_ERROR:
	case GNUTLS_E_NO_TEMPORARY_DH_PARAMS:
	case GNUTLS_E_NO_TEMPORARY_RSA_PARAMS:
		ret = GNUTLS_A_INTERNAL_ERROR;
		_level = GNUTLS_AL_FATAL;
		break;
	case GNUTLS_E_INAPPROPRIATE_FALLBACK:
		ret = GNUTLS_A_INAPPROPRIATE_FALLBACK;
		_level = GNUTLS_AL_FATAL;
		break;
	case GNUTLS_E_OPENPGP_GETKEY_FAILED:
		ret = GNUTLS_A_CERTIFICATE_UNOBTAINABLE;
		_level = GNUTLS_AL_FATAL;
		break;
	case GNUTLS_E_DH_PRIME_UNACCEPTABLE:
	case GNUTLS_E_SESSION_USER_ID_CHANGED:
	case GNUTLS_E_INSUFFICIENT_SECURITY:
		ret = GNUTLS_A_INSUFFICIENT_SECURITY;
		_level = GNUTLS_AL_FATAL;
		break;
	case GNUTLS_E_NO_APPLICATION_PROTOCOL:
		ret = GNUTLS_A_NO_APPLICATION_PROTOCOL;
		_level = GNUTLS_AL_FATAL;
		break;
	case GNUTLS_E_UNRECOGNIZED_NAME:
		ret = GNUTLS_A_UNRECOGNIZED_NAME;
		_level = GNUTLS_AL_FATAL;
		break;
	case GNUTLS_E_CERTIFICATE_REQUIRED:
		ret = GNUTLS_A_CERTIFICATE_REQUIRED;
		_level = GNUTLS_AL_FATAL;
		break;
	default:
		ret = GNUTLS_A_INTERNAL_ERROR;
		_level = GNUTLS_AL_FATAL;
		break;
	}

	if (level != NULL)
		*level = _level;

	return ret;
}

/**
 * gnutls_alert_send_appropriate:
 * @session: is a #gnutls_session_t type.
 * @err: is an error code returned by another GnuTLS function
 *
 * Sends an alert to the peer depending on the error code returned by
 * a gnutls function. This function will call gnutls_error_to_alert()
 * to determine the appropriate alert to send.
 *
 * This function may also return %GNUTLS_E_AGAIN, or
 * %GNUTLS_E_INTERRUPTED.
 *
 * This function historically was always sending an alert to the
 * peer, even if @err was inappropriate to respond with an alert
 * (e.g., %GNUTLS_E_SUCCESS). Since 3.6.6 this function returns
 * success without transmitting any data on error codes that
 * should not result to an alert.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise
 *   an error code is returned.
 */
int gnutls_alert_send_appropriate(gnutls_session_t session, int err)
{
	int alert;
	int level;

	if (err != GNUTLS_E_REHANDSHAKE && (!gnutls_error_is_fatal(err) ||
	    err == GNUTLS_E_FATAL_ALERT_RECEIVED))
		return gnutls_assert_val(0);

	alert = gnutls_error_to_alert(err, &level);

	return gnutls_alert_send(session, (gnutls_alert_level_t)level, alert);
}

/**
 * gnutls_alert_get:
 * @session: is a #gnutls_session_t type.
 *
 * This function will return the last alert number received.  This
 * function should be called when %GNUTLS_E_WARNING_ALERT_RECEIVED or
 * %GNUTLS_E_FATAL_ALERT_RECEIVED errors are returned by a gnutls
 * function.  The peer may send alerts if he encounters an error.
 * If no alert has been received the returned value is undefined.
 *
 * Returns: the last alert received, a
 *   #gnutls_alert_description_t value.
 **/
gnutls_alert_description_t gnutls_alert_get(gnutls_session_t session)
{
	return (gnutls_alert_description_t)session->internals.last_alert;
}
