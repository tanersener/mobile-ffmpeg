/*
 * Copyright (C) 2018 Red Hat, Inc.
 *
 * Author: Daiki Ueno
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

/* This file contains the code for the Record Size Limit TLS extension.
 */

#include "gnutls_int.h"
#include "errors.h"
#include "num.h"
#include <hello_ext.h>
#include <ext/record_size_limit.h>

static int _gnutls_record_size_limit_recv_params(gnutls_session_t session,
						 const uint8_t * data,
						 size_t data_size);
static int _gnutls_record_size_limit_send_params(gnutls_session_t session,
						 gnutls_buffer_st * extdata);

const hello_ext_entry_st ext_mod_record_size_limit = {
	.name = "Record Size Limit",
	.tls_id = 28,
	.gid = GNUTLS_EXTENSION_RECORD_SIZE_LIMIT,
	.client_parse_point = GNUTLS_EXT_MANDATORY,
	.server_parse_point = GNUTLS_EXT_MANDATORY,
	.validity = GNUTLS_EXT_FLAG_TLS | GNUTLS_EXT_FLAG_DTLS | GNUTLS_EXT_FLAG_CLIENT_HELLO |
		    GNUTLS_EXT_FLAG_EE | GNUTLS_EXT_FLAG_TLS12_SERVER_HELLO,
	.recv_func = _gnutls_record_size_limit_recv_params,
	.send_func = _gnutls_record_size_limit_send_params
};

static int
_gnutls_record_size_limit_recv_params(gnutls_session_t session,
				      const uint8_t * data, size_t data_size)
{
	ssize_t new_size;
	const version_entry_st *vers;

	DECR_LEN(data_size, 2);
	if (data_size != 0)
		return gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET_LENGTH);
	new_size = _gnutls_read_uint16(data);

	/* protocol error */
	if (new_size < 64)
		return gnutls_assert_val(GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER);

	session->internals.hsk_flags |= HSK_RECORD_SIZE_LIMIT_RECEIVED;

	/* we do not want to accept sizes outside of our supported range */
	if (new_size <
	    (session->internals.allow_small_records ?
	     MIN_RECORD_SIZE_SMALL : MIN_RECORD_SIZE)) {
		/* for server, reject it by omitting the extension in the reply */
		if (session->security_parameters.entity == GNUTLS_SERVER) {
			_gnutls_handshake_log("EXT[%p]: client requested too small record_size_limit %u; ignoring\n",
					      session, (unsigned)new_size);
			return gnutls_assert_val(0);
		} else {
			_gnutls_handshake_log("EXT[%p]: server requested too small record_size_limit %u; closing the connection\n",
					      session, (unsigned)new_size);
			return gnutls_assert_val(GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER);
		}
	}

	session->internals.hsk_flags |= HSK_RECORD_SIZE_LIMIT_NEGOTIATED;

	/* client uses the reception of this extension as an
	 * indication of the request was accepted by the server */
	if (session->security_parameters.entity == GNUTLS_CLIENT)
		session->security_parameters.max_record_recv_size =
			session->security_parameters.max_user_record_recv_size;

	_gnutls_handshake_log("EXT[%p]: record_size_limit %u negotiated\n",
			      session, (unsigned)new_size);

	/* subtract 1 octet for content type */
	vers = get_version(session);
	if (unlikely(vers == NULL))
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

	session->security_parameters.max_record_send_size =
		MIN(new_size - vers->tls13_sem,
		    session->security_parameters.max_user_record_send_size);

	return 0;
}

/* returns data_size or a negative number on failure
 */
static int
_gnutls_record_size_limit_send_params(gnutls_session_t session,
				      gnutls_buffer_st * extdata)
{
	int ret;
	uint16_t send_size;

	assert(session->security_parameters.max_user_record_recv_size >= 64 &&
	       session->security_parameters.max_user_record_recv_size <=
	       DEFAULT_MAX_RECORD_SIZE);

	send_size = session->security_parameters.max_user_record_recv_size;

	if (session->security_parameters.entity == GNUTLS_SERVER) {
		const version_entry_st *vers;

		/* if we had received the extension and rejected, don't send it */
		if (session->internals.hsk_flags & HSK_RECORD_SIZE_LIMIT_RECEIVED &&
		    !(session->internals.hsk_flags & HSK_RECORD_SIZE_LIMIT_NEGOTIATED))
			return gnutls_assert_val(0);

		/* add 1 octet for content type */
		vers = get_version(session);
		if (unlikely(vers == NULL))
			return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

		session->security_parameters.max_record_recv_size =
			send_size;

		send_size += vers->tls13_sem;
	} else {
		const version_entry_st *vers;

		/* add 1 octet for content type */
		vers = _gnutls_version_max(session);
		if (unlikely(vers == NULL))
			return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

		send_size += vers->tls13_sem;
	}

	ret = _gnutls_buffer_append_prefix(extdata, 16, send_size);
	if (ret < 0)
		return gnutls_assert_val(ret);

	session->internals.hsk_flags |= HSK_RECORD_SIZE_LIMIT_SENT;

	return 2;
}
