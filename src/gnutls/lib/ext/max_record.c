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

/* This file contains the code for the Max Record Size TLS extension.
 */

#include "gnutls_int.h"
#include "errors.h"
#include "num.h"
#include <hello_ext.h>
#include <ext/max_record.h>

static int _gnutls_max_record_recv_params(gnutls_session_t session,
					  const uint8_t * data,
					  size_t data_size);
static int _gnutls_max_record_send_params(gnutls_session_t session,
					  gnutls_buffer_st * extdata);

/* Maps record size to numbers according to the
 * extensions draft.
 */
static int _gnutls_mre_num2record(int num);
static int _gnutls_mre_record2num(uint16_t record_size);


const hello_ext_entry_st ext_mod_max_record_size = {
	.name = "Maximum Record Size",
	.tls_id = 1,
	.gid = GNUTLS_EXTENSION_MAX_RECORD_SIZE,
	.parse_type = GNUTLS_EXT_TLS,
	.validity = GNUTLS_EXT_FLAG_TLS | GNUTLS_EXT_FLAG_DTLS | GNUTLS_EXT_FLAG_CLIENT_HELLO |
		    GNUTLS_EXT_FLAG_EE | GNUTLS_EXT_FLAG_TLS12_SERVER_HELLO,
	.recv_func = _gnutls_max_record_recv_params,
	.send_func = _gnutls_max_record_send_params
};

/* 
 * In case of a server: if a MAX_RECORD_SIZE extension type is received then it stores
 * into the session the new value. The server may use gnutls_get_max_record_size(),
 * in order to access it.
 *
 * In case of a client: If a different max record size (than the default) has
 * been specified then it sends the extension.
 *
 */

static int
_gnutls_max_record_recv_params(gnutls_session_t session,
			       const uint8_t * data, size_t _data_size)
{
	ssize_t new_size;
	ssize_t data_size = _data_size;

	if (session->internals.hsk_flags & HSK_RECORD_SIZE_LIMIT_NEGOTIATED)
		return 0;

	if (session->security_parameters.entity == GNUTLS_SERVER) {
		if (data_size > 0) {
			DECR_LEN(data_size, 1);

			new_size = _gnutls_mre_num2record(data[0]);

			if (new_size < 0) {
				gnutls_assert();
				return new_size;
			}

			session->security_parameters.max_record_send_size =
			    new_size;
			session->security_parameters.max_record_recv_size =
			    new_size;
		}
	} else {		/* CLIENT SIDE - we must check if the sent record size is the right one 
				 */
		if (data_size > 0) {
			if (data_size != 1) {
				gnutls_assert();
				return GNUTLS_E_UNEXPECTED_PACKET_LENGTH;
			}

			new_size = _gnutls_mre_num2record(data[0]);

			if (new_size < 0) {
				gnutls_assert();
				return new_size;
			}

			if (new_size != session->security_parameters.
			    max_user_record_send_size) {
				gnutls_assert();
				return GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER;
			} else {
				session->security_parameters.
				    max_record_send_size = new_size;
				session->security_parameters.
				    max_record_recv_size = new_size;
			}

		}


	}

	return 0;
}

/* returns data_size or a negative number on failure
 */
static int
_gnutls_max_record_send_params(gnutls_session_t session,
			       gnutls_buffer_st * extdata)
{
	uint8_t p;
	int ret;

	/* this function sends the client extension data (dnsname) */
	if (session->security_parameters.entity == GNUTLS_CLIENT) {
		/* if the user limits for sending and receiving are
		 * different, that means the programmer had chosen to
		 * use record_size_limit instead */
		if (session->security_parameters.max_user_record_send_size !=
		    session->security_parameters.max_user_record_recv_size)
			return 0;

		if (session->security_parameters.max_user_record_send_size !=
		    DEFAULT_MAX_RECORD_SIZE) {
			ret = _gnutls_mre_record2num
			      (session->security_parameters.
			       max_user_record_send_size);

			/* it's not an error, as long as we send the
			 * record_size_limit extension with that value */
			if (ret < 0)
				return 0;

			p = (uint8_t) ret;
			ret = _gnutls_buffer_append_data(extdata, &p, 1);
			if (ret < 0)
				return gnutls_assert_val(ret);

			return 1;
		}

	} else {		/* server side */

		if (session->internals.hsk_flags & HSK_RECORD_SIZE_LIMIT_SENT)
			return 0;

		if (session->security_parameters.max_record_recv_size !=
		    DEFAULT_MAX_RECORD_SIZE) {
			ret = _gnutls_mre_record2num
			      (session->security_parameters.
			       max_record_recv_size);
			if (ret < 0)
				return gnutls_assert_val(ret);

			p = (uint8_t) ret;
			ret = _gnutls_buffer_append_data(extdata, &p, 1);
			if (ret < 0)
				return gnutls_assert_val(ret);

			return 1;
		}
	}

	return 0;
}


/* Maps numbers to record sizes according to the
 * extensions draft.
 */
static int _gnutls_mre_num2record(int num)
{
	switch (num) {
	case 1:
		return 512;
	case 2:
		return 1024;
	case 3:
		return 2048;
	case 4:
		return 4096;
	default:
		return GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER;
	}
}

/* Maps record size to numbers according to the
 * extensions draft.
 */
static int _gnutls_mre_record2num(uint16_t record_size)
{
	switch (record_size) {
	case 512:
		return 1;
	case 1024:
		return 2;
	case 2048:
		return 3;
	case 4096:
		return 4;
	default:
		return GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER;
	}

}

/**
 * gnutls_record_get_max_size:
 * @session: is a #gnutls_session_t type.
 *
 * Get the record size.  The maximum record size is negotiated by the
 * client after the first handshake message.
 *
 * Returns: The maximum record packet size in this connection.
 **/
size_t gnutls_record_get_max_size(gnutls_session_t session)
{
	/* Recv will hold the negotiated max record size
	 * always.
	 */
	return session->security_parameters.max_record_recv_size;
}


/**
 * gnutls_record_set_max_size:
 * @session: is a #gnutls_session_t type.
 * @size: is the new size
 *
 * This function sets the maximum amount of plaintext sent and
 * received in a record in this connection.
 *
 * Prior to 3.6.4, this function was implemented using a TLS extension
 * called 'max fragment length', which limits the acceptable values to
 * 512(=2^9), 1024(=2^10), 2048(=2^11) and 4096(=2^12).
 *
 * Since 3.6.4, the limit is also negotiated through a new TLS
 * extension called 'record size limit', which doesn't have the
 * limitation, as long as the value ranges between 512 and 16384.
 * Note that while the 'record size limit' extension is preferred, not
 * all TLS implementations use or even understand the extension.
 *
 * Deprecated: if the client can assume that the 'record size limit'
 * extension is supported by the server, we recommend using
 * gnutls_record_set_max_recv_size() instead.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned,
 *   otherwise a negative error code is returned.
 **/
ssize_t gnutls_record_set_max_size(gnutls_session_t session, size_t size)
{
	if (size < MIN_RECORD_SIZE || size > DEFAULT_MAX_RECORD_SIZE)
		return GNUTLS_E_INVALID_REQUEST;

	if (session->internals.handshake_in_progress)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	session->security_parameters.max_user_record_send_size = size;
	session->security_parameters.max_user_record_recv_size = size;

	return 0;
}

/**
 * gnutls_record_set_max_recv_size:
 * @session: is a #gnutls_session_t type.
 * @size: is the new size
 *
 * This function sets the maximum amount of plaintext received in a
 * record in this connection.
 *
 * The limit is also negotiated through a TLS extension called 'record
 * size limit'.  Note that while the 'record size limit' extension is
 * preferred, not all TLS implementations use or even understand the
 * extension.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned,
 *   otherwise a negative error code is returned.
 *
 * Since: 3.6.8
 **/
ssize_t gnutls_record_set_max_recv_size(gnutls_session_t session, size_t size)
{
	if (size <
	    (session->internals.allow_small_records ?
	     MIN_RECORD_SIZE_SMALL : MIN_RECORD_SIZE) ||
	    size > DEFAULT_MAX_RECORD_SIZE)
		return GNUTLS_E_INVALID_REQUEST;

	if (session->internals.handshake_in_progress)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	session->security_parameters.max_user_record_recv_size = size;

	return 0;
}
