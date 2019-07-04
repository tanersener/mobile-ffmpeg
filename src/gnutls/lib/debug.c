/*
 * Copyright (C) 2001-2012 Free Software Foundation, Inc.
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
#include <stdio.h>
#include <stdlib.h>
#include "debug.h"
#include <mpi.h>

#ifdef DEBUG
void _gnutls_dump_mpi(const char *prefix, bigint_t a)
{
	char buf[400];
	char buf_hex[2 * sizeof(buf) + 1];
	size_t n = sizeof buf;

	if (_gnutls_mpi_print(a, buf, &n))
		strcpy(buf, "[can't print value]");	/* Flawfinder: ignore */
	_gnutls_debug_log("MPI: length: %d\n\t%s%s\n", (int) n, prefix,
			  _gnutls_bin2hex(buf, n, buf_hex, sizeof(buf_hex),
					  NULL));
}

void
_gnutls_dump_vector(const char *prefix, const uint8_t * a, size_t a_size)
{
	char buf_hex[2 * a_size + 1];

	_gnutls_debug_log("Vector: length: %d\n\t%s%s\n", (int) a_size,
			  prefix, _gnutls_bin2hex(a, a_size, buf_hex,
						  sizeof(buf_hex), NULL));
}
#endif

const char *_gnutls_packet2str(content_type_t packet)
{
	switch (packet) {
	case GNUTLS_CHANGE_CIPHER_SPEC:
		return "ChangeCipherSpec";
	case GNUTLS_ALERT:
		return "Alert";
	case GNUTLS_HANDSHAKE:
		return "Handshake";
	case GNUTLS_APPLICATION_DATA:
		return "Application Data";
	case GNUTLS_HEARTBEAT:
		return "HeartBeat";
	default:
		return "Unknown Packet";
	}
}

/**
 * gnutls_handshake_description_get_name:
 * @type: is a handshake message description
 *
 * Convert a #gnutls_handshake_description_t value to a string.
 *
 * Returns: a string that contains the name of the specified handshake
 *   message or %NULL.
 **/
const char
    *gnutls_handshake_description_get_name(gnutls_handshake_description_t
					   type)
{
	switch (type) {
	case GNUTLS_HANDSHAKE_END_OF_EARLY_DATA:
		return "END OF EARLY DATA";
	case GNUTLS_HANDSHAKE_HELLO_RETRY_REQUEST:
		return "HELLO RETRY REQUEST";
	case GNUTLS_HANDSHAKE_HELLO_REQUEST:
		return "HELLO REQUEST";
	case GNUTLS_HANDSHAKE_CLIENT_HELLO:
		return "CLIENT HELLO";
#ifdef ENABLE_SSL2
	case GNUTLS_HANDSHAKE_CLIENT_HELLO_V2:
		return "SSL2 CLIENT HELLO";
#endif
	case GNUTLS_HANDSHAKE_SERVER_HELLO:
		return "SERVER HELLO";
	case GNUTLS_HANDSHAKE_HELLO_VERIFY_REQUEST:
		return "HELLO VERIFY REQUEST";
	case GNUTLS_HANDSHAKE_CERTIFICATE_PKT:
		return "CERTIFICATE";
	case GNUTLS_HANDSHAKE_ENCRYPTED_EXTENSIONS:
		return "ENCRYPTED EXTENSIONS";
	case GNUTLS_HANDSHAKE_SERVER_KEY_EXCHANGE:
		return "SERVER KEY EXCHANGE";
	case GNUTLS_HANDSHAKE_CERTIFICATE_REQUEST:
		return "CERTIFICATE REQUEST";
	case GNUTLS_HANDSHAKE_SERVER_HELLO_DONE:
		return "SERVER HELLO DONE";
	case GNUTLS_HANDSHAKE_CERTIFICATE_VERIFY:
		return "CERTIFICATE VERIFY";
	case GNUTLS_HANDSHAKE_CLIENT_KEY_EXCHANGE:
		return "CLIENT KEY EXCHANGE";
	case GNUTLS_HANDSHAKE_FINISHED:
		return "FINISHED";
	case GNUTLS_HANDSHAKE_KEY_UPDATE:
		return "KEY_UPDATE";
	case GNUTLS_HANDSHAKE_SUPPLEMENTAL:
		return "SUPPLEMENTAL";
	case GNUTLS_HANDSHAKE_CERTIFICATE_STATUS:
		return "CERTIFICATE STATUS";
	case GNUTLS_HANDSHAKE_NEW_SESSION_TICKET:
		return "NEW SESSION TICKET";
	case GNUTLS_HANDSHAKE_CHANGE_CIPHER_SPEC:
		return "CHANGE CIPHER SPEC";
	default:
		return "Unknown Handshake packet";
	}
}
