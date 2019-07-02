/*
 * Copyright (C) 2016 - 2018 ARPA2 project
 *
 * Author: Tom Vrancken (dev@tomvrancken.nl)
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
 * This file is part of the server_certificate_type extension as
 * defined in RFC7250 (https://tools.ietf.org/html/rfc7250).
 *
 * The server_certificate_type extension in the client hello indicates
 * the types of certificates the client is able to process when provided
 * by the server in a subsequent certificate payload.
 */

#include <gnutls_int.h>
#include <gnutls/gnutls.h>
#include "ext/cert_types.h"
#include "ext/server_cert_type.h"
#include "hello_ext.h"
#include "hello_ext_lib.h"
#include "errors.h"
#include "state.h"
#include "datum.h"


static int _gnutls_server_cert_type_recv_params(gnutls_session_t session,
						const uint8_t* data,
						size_t data_size);
static int _gnutls_server_cert_type_send_params(gnutls_session_t session,
						gnutls_buffer_st* data);


const hello_ext_entry_st ext_mod_server_cert_type = {
	.name = "Server Certificate Type",
	.tls_id = 20,
	.gid = GNUTLS_EXTENSION_SERVER_CERT_TYPE,
	.parse_type = GNUTLS_EXT_TLS,
	.validity = GNUTLS_EXT_FLAG_TLS |
		GNUTLS_EXT_FLAG_DTLS |
		GNUTLS_EXT_FLAG_CLIENT_HELLO |
		GNUTLS_EXT_FLAG_TLS12_SERVER_HELLO |
		GNUTLS_EXT_FLAG_EE,
	.recv_func = _gnutls_server_cert_type_recv_params,
	.send_func = _gnutls_server_cert_type_send_params,
	.pack_func = _gnutls_hello_ext_default_pack,
	.unpack_func = _gnutls_hello_ext_default_unpack,
	.deinit_func = _gnutls_hello_ext_default_deinit,
	.cannot_be_overriden = 1
};


static int _gnutls_server_cert_type_recv_params(gnutls_session_t session,
						const uint8_t* data,
						size_t data_size)
{
	int ret;
	gnutls_datum_t cert_types; // Holds the received cert types
	gnutls_datum_t sent_cert_types; // Holds the previously sent cert types
	gnutls_certificate_type_t cert_type;

	uint8_t i, found = 0;
	ssize_t len = data_size;
	const uint8_t* pdata = data;

	/* Only activate this extension if we have cert credentials set
	 * and alternative cert types are allowed */
	if (!are_alternative_cert_types_allowed(session) ||
		(_gnutls_get_cred(session, GNUTLS_CRD_CERTIFICATE) == NULL))
		return 0;

	if (!IS_SERVER(session)) {	// client mode

		/* Compare packet length with expected packet length. For the
		 * client this is a single byte. */
		if (len != 1) {
			return
					gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET_LENGTH);
		}

		/* The server picked one of the offered cert types if he supports
		 * at least one of them. If both parties play by the rules then we
		 * may only receive a cert type that we offered, i.e. one that we
		 * support. Because the world isn't as beautiful as it may seem,
		 * we're going to check it nevertheless. */
		cert_type = IANA2cert_type(pdata[0]);

		// Check validity of cert type
		if (cert_type == GNUTLS_CRT_UNKNOWN) {
			return gnutls_assert_val(GNUTLS_E_UNSUPPORTED_CERTIFICATE_TYPE);
		}

		/* Get the cert types that we sent to the server (they were stored
		 * in IANA representation.
		 */
		ret = _gnutls_hello_ext_get_datum(session,
							 GNUTLS_EXTENSION_SERVER_CERT_TYPE,
							 &sent_cert_types);
		if (ret < 0) {
			/* This should not happen and indicate a memory corruption!
			 * Assertion are always on in production code so execution
			 * will halt here. */
			assert(false);
		}

		// Check whether what we got back is actually offered by us
		for (i = 0; i < sent_cert_types.size; i++) {
			if (IANA2cert_type(sent_cert_types.data[i]) == cert_type)
				found = 1;
		}

		if (found) {
			// Everything OK, now set the server certificate type
			_gnutls_session_server_cert_type_set(session, cert_type);
			ret = GNUTLS_E_SUCCESS;
		} else {
			// No valid cert type found
			ret = GNUTLS_E_UNSUPPORTED_CERTIFICATE_TYPE;
		}

		return ret;

	} else {		// server mode
		// Compare packet length with expected packet length.
		DECR_LEN(len, 1);
		if (data[0] != len) {
			return
					gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET_LENGTH);
		}
		pdata += 1;

		// Assign the contents of our data buffer to a gnutls_datum_t
		cert_types.data = (uint8_t*)pdata; // Need casting to get rid of 'discards const qualifier' warning
		cert_types.size = len;

		// Store the server certificate types in our session
		_gnutls_hello_ext_set_datum(session,
							 GNUTLS_EXTENSION_SERVER_CERT_TYPE,
							 &cert_types);

		/* We receive a list of supported certificate types that the client
		 * is able to process when offered by the server via a subsequent
		 * Certificate message. This list is sorted by order of preference.
		 * We now check in this order of preference whether we support any
		 * of these certificate types.
		 */
		for (i = 0; i < cert_types.size; i++) {
			// Convert to internal representation
			cert_type = IANA2cert_type(cert_types.data[i]);

			// If we have an invalid cert id then continue to the next
			if (cert_type == GNUTLS_CRT_UNKNOWN)
				continue;

			// Check for support of this cert type
			if (_gnutls_session_cert_type_supported
					(session, cert_type, true, GNUTLS_CTYPE_SERVER) == 0) {
				found = 1;
				break;
			}
		}

		// We found a matching ctype, we pick this one
		if (found) {
			_gnutls_session_server_cert_type_set(session, cert_type);
			ret = GNUTLS_E_SUCCESS;
		} else {
			/* If no supported certificate type can be found we terminate
			 * with a fatal alert of type "unsupported_certificate"
			 * (according to specification rfc7250).
			 */
			ret = GNUTLS_E_UNSUPPORTED_CERTIFICATE_TYPE;
		}

		return ret;
	}
}

static int _gnutls_server_cert_type_send_params(gnutls_session_t session,
						gnutls_buffer_st* data)
{
	int ret;
	uint8_t cert_type; // Holds an IANA cert type ID
	uint8_t i = 0, num_cert_types = 0;
	priority_st* cert_priorities;
	gnutls_datum_t tmp_cert_types; // For type conversion
	uint8_t cert_types[GNUTLS_CRT_MAX]; // The list with supported cert types. Inv: 0 <= cert type Id < 256

	/* Only activate this extension if we have cert credentials set
	 * and alternative cert types are allowed */
	if (!are_alternative_cert_types_allowed(session) ||
		(_gnutls_get_cred(session, GNUTLS_CRD_CERTIFICATE) == NULL))
		return 0;

	if (!IS_SERVER(session)) {	// Client mode
		// For brevity
		cert_priorities =
				&session->internals.priorities->server_ctype;

		/* Retrieve server certificate type priorities if any. If no
		 * priorities are set then the default server certificate type
		 * initialization values apply. This default is currently set to
		 * X.509 in which case we don't enable this extension.
		 */
		if (cert_priorities->num_priorities > 0) {	// Priorities are explicitly set
			/* If the certificate priority is explicitly set to only
			 * X.509 (default) then, according to spec we don't send
			 * this extension. We check this here to avoid further work in
			 * this routine. We also check it below after pruning supported
			 * types.
			 */
			if (cert_priorities->num_priorities == 1 &&
					cert_priorities->priorities[0] == DEFAULT_CERT_TYPE) {
				_gnutls_handshake_log
						("EXT[%p]: Server certificate type was set to default cert type (%s). "
						 "We therefore do not send this extension.\n",
						 session,
						 gnutls_certificate_type_get_name(DEFAULT_CERT_TYPE));

				// Explicitly set but default ctype, so don't send anything
				return 0;
			}

			/* We are only allowed to send certificate types that we support.
			 * Therefore we check this here and prune our original list.
			 * This check might seem redundant now because we don't check for
			 * credentials (they are not needed for a client) and only check the
			 * priorities over which we already iterate. In the future,
			 * additional checks might be necessary and they can be easily
			 * added in the ..type_supported() routine without modifying the
			 * structure of the code here.
			 */
			for (i = 0; i < cert_priorities->num_priorities; i++) {
				if (_gnutls_session_cert_type_supported
						(session, cert_priorities->priorities[i],
						 false, GNUTLS_CTYPE_SERVER) == 0) {
					/* Check whether we are allowed to store another cert type
					 * in our buffer. In other words, prevent a possible buffer
					 * overflow. This situation can occur when a user sets
					 * duplicate cert types in the priority strings. */
					if (num_cert_types >= GNUTLS_CRT_MAX)
						return gnutls_assert_val(GNUTLS_E_SHORT_MEMORY_BUFFER);

					// Convert to IANA representation
					ret = cert_type2IANA(cert_priorities->priorities[i]);

					if (ret < 0)
						return gnutls_assert_val(ret);

					cert_type = ret; // For readability

					// Add this cert type to our list with supported types
					cert_types[num_cert_types] = cert_type;
					num_cert_types++;

					_gnutls_handshake_log
							("EXT[%p]: Server certificate type %s (%d) was queued.\n",
							 session,
							 gnutls_certificate_type_get_name(cert_priorities->priorities[i]),
							 cert_type);
				}
			}

			/* Check whether there are any supported certificate types left
			 * after the previous pruning step. If not, we do not send this
			 * extension. Also, if the only supported type is the default type
			 * we do not send this extension (according to RFC7250).
			 */
			if (num_cert_types == 0) {	// For now, this should not occur since we only check priorities while pruning.
				_gnutls_handshake_log
						("EXT[%p]: Server certificate types were set but none of them is supported. "
						 "We do not send this extension.\n",
						 session);

				return 0;
			} else if (num_cert_types == 1 &&
					 IANA2cert_type(cert_types[0]) == DEFAULT_CERT_TYPE) {
				_gnutls_handshake_log
						("EXT[%p]: The only supported server certificate type is (%s) which is the default. "
						 "We therefore do not send this extension.\n",
						 session,
						 gnutls_certificate_type_get_name(DEFAULT_CERT_TYPE));

				return 0;
			}

			/* We have data to send and store a copy internally. We convert
			 * our list with supported cert types to a datum_t in order to
			 * be able to make the ..._set_datum call.
			 */
			tmp_cert_types.data = cert_types;
			tmp_cert_types.size = num_cert_types;

			_gnutls_hello_ext_set_datum(session,
								 GNUTLS_EXTENSION_SERVER_CERT_TYPE,
								 &tmp_cert_types);

			/* Serialize the certificate types into a sequence of octets
			 * uint8: length of sequence of cert types (1 octet)
			 * uint8: cert types (0 <= #octets <= 255)
			 */
			ret = _gnutls_buffer_append_data_prefix(data, 8,
								cert_types,
								num_cert_types);

			// Check for errors and cleanup in case of error
			if (ret < 0) {
				return gnutls_assert_val(ret);
			} else {
				// Number of bytes we are sending
				return num_cert_types + 1;
			}
		}
	} else {	// Server mode
		// Retrieve negotiated server certificate type and send it
		ret = cert_type2IANA(get_certificate_type(
					session, GNUTLS_CTYPE_SERVER));

		if (ret < 0)
			return gnutls_assert_val(ret);

		cert_type = ret; // For readability

		ret = gnutls_buffer_append_data(data, &cert_type, 1);

		if (ret < 0)
			return gnutls_assert_val(ret);

		return 1;	// sent one byte
	}

	// In all other cases don't enable this extension
	return 0;
}


/** Extension interface **/

/* The interface is defined in state.c:
 * Public:
 * - gnutls_certificate_type_get2
 *
 * Private:
 * - _gnutls_session_server_cert_type_set
 */
