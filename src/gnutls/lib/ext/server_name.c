/*
 * Copyright (C) 2002-2012 Free Software Foundation, Inc.
 * Copyright (C) 2017 Red Hat, Inc.
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
#include "auth.h"
#include "errors.h"
#include "num.h"
#include "str.h"
#include <ext/server_name.h>
#include "hello_ext_lib.h"

static int _gnutls_server_name_recv_params(gnutls_session_t session,
					   const uint8_t * data,
					   size_t data_size);
static int _gnutls_server_name_send_params(gnutls_session_t session,
					   gnutls_buffer_st * extdata);

int
_gnutls_server_name_set_raw(gnutls_session_t session,
			    gnutls_server_name_type_t type,
			    const void *name, size_t name_length);

const hello_ext_entry_st ext_mod_server_name = {
	.name = "Server Name Indication",
	.tls_id = 0,
	.gid = GNUTLS_EXTENSION_SERVER_NAME,
	.validity = GNUTLS_EXT_FLAG_TLS | GNUTLS_EXT_FLAG_DTLS | GNUTLS_EXT_FLAG_CLIENT_HELLO |
		    GNUTLS_EXT_FLAG_EE | GNUTLS_EXT_FLAG_TLS12_SERVER_HELLO,
	.parse_type = GNUTLS_EXT_MANDATORY,
	.recv_func = _gnutls_server_name_recv_params,
	.send_func = _gnutls_server_name_send_params,
	.pack_func = _gnutls_hello_ext_default_pack,
	.unpack_func = _gnutls_hello_ext_default_unpack,
	.deinit_func = _gnutls_hello_ext_default_deinit,
	.cannot_be_overriden = 1
};

/*
 * In case of a server: if a NAME_DNS extension type is received then
 * it stores into the session the value of NAME_DNS. The server may
 * use gnutls_ext_get_server_name(), in order to access it.
 *
 * In case of a client: If a proper NAME_DNS extension type is found
 * in the session then it sends the extension to the peer.
 *
 */
static int
_gnutls_server_name_recv_params(gnutls_session_t session,
				const uint8_t * data, size_t _data_size)
{
	const unsigned char *p;
	uint16_t len, type;
	ssize_t data_size = _data_size;
	gnutls_datum_t name;

	if (session->security_parameters.entity == GNUTLS_SERVER) {
		DECR_LENGTH_RET(data_size, 2, GNUTLS_E_UNEXPECTED_PACKET_LENGTH);
		len = _gnutls_read_uint16(data);
		if (len == 0)
			return gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET_LENGTH);

		if (len != data_size) {
			gnutls_assert();
			return GNUTLS_E_UNEXPECTED_PACKET_LENGTH;
		}

		p = data + 2;

		while (data_size > 0) {
			DECR_LEN(data_size, 1);
			type = *p;
			p++;

			DECR_LEN(data_size, 2);
			len = _gnutls_read_uint16(p);
			p += 2;

			if (len == 0) {
				_gnutls_handshake_log
				    ("HSK[%p]: Received server name size of zero\n",
				     session);
				return gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET_LENGTH);
			}

			DECR_LEN(data_size, len);

			if (type == 0) { /* NAME_DNS */
				if (!_gnutls_dnsname_is_valid((char*)p, len)) {
					_gnutls_handshake_log
					    ("HSK[%p]: Server name is not acceptable: '%.*s'\n",
					     session, (int) len, p);
					return gnutls_assert_val(GNUTLS_E_RECEIVED_DISALLOWED_NAME);
				}

				name.data = (void*)p;
				name.size = len;

				_gnutls_hello_ext_unset_priv(session, GNUTLS_EXTENSION_SERVER_NAME);
				return _gnutls_hello_ext_set_datum(session,
					     GNUTLS_EXTENSION_SERVER_NAME,
					     &name);
			}
			p += len;

		}


	}

	return 0;
}

/* returns data_size or a negative number on failure
 */
static int
_gnutls_server_name_send_params(gnutls_session_t session,
				gnutls_buffer_st * extdata)
{
	int total_size = 0, ret;
	gnutls_datum_t name;

	ret =
	    _gnutls_hello_ext_get_datum(session, GNUTLS_EXTENSION_SERVER_NAME,
					&name);
	if (ret < 0)
		return 0;

	/* this function sends the client extension data (dnsname)
	 */
	if (session->security_parameters.entity == GNUTLS_CLIENT) {
		if (name.size == 0)
			return 0;

		/* uint8_t + uint16_t + size
		 */
		total_size = 2 + 1 + 2 + name.size;

		/* UINT16: write total size of all names
		 */
		ret =
		    _gnutls_buffer_append_prefix(extdata, 16,
						 total_size - 2);
		if (ret < 0)
			return gnutls_assert_val(ret);

		/* UINT8: type of this extension
		 * UINT16: size of the first name
		 * LEN: the actual server name.
		 */
		ret =
		    _gnutls_buffer_append_prefix(extdata, 8, 0);
		if (ret < 0)
			return gnutls_assert_val(ret);

		_gnutls_debug_log("HSK[%p]: sent server name: '%.*s'\n", session, name.size, name.data);

		ret =
		    _gnutls_buffer_append_data_prefix
			    (extdata, 16,
			     name.data, name.size);
		if (ret < 0)
			return gnutls_assert_val(ret);
	} else {
		return 0;
	}

	return total_size;
}

/**
 * gnutls_server_name_get:
 * @session: is a #gnutls_session_t type.
 * @data: will hold the data
 * @data_length: will hold the data length. Must hold the maximum size of data.
 * @type: will hold the server name indicator type
 * @indx: is the index of the server_name
 *
 * This function will allow you to get the name indication (if any), a
 * client has sent.  The name indication may be any of the enumeration
 * gnutls_server_name_type_t.
 *
 * If @type is GNUTLS_NAME_DNS, then this function is to be used by
 * servers that support virtual hosting, and the data will be a null
 * terminated IDNA ACE string (prior to GnuTLS 3.4.0 it was a UTF-8 string).
 *
 * If @data has not enough size to hold the server name
 * GNUTLS_E_SHORT_MEMORY_BUFFER is returned, and @data_length will
 * hold the required size.
 *
 * @indx is used to retrieve more than one server names (if sent by
 * the client).  The first server name has an index of 0, the second 1
 * and so on.  If no name with the given index exists
 * GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE is returned.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, on UTF-8
 *  decoding error %GNUTLS_E_IDNA_ERROR is returned, otherwise a negative
 *  error code is returned.
 **/
int
gnutls_server_name_get(gnutls_session_t session, void *data,
		       size_t * data_length,
		       unsigned int *type, unsigned int indx)
{
	char *_data = data;
	gnutls_datum_t name;
	int ret;

	if (session->security_parameters.entity == GNUTLS_CLIENT) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	if (indx != 0)
		return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;

	ret =
	    _gnutls_hello_ext_get_datum(session, GNUTLS_EXTENSION_SERVER_NAME, &name);
	if (ret < 0) {
		gnutls_assert();
		return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
	}

	if (name.size == 0) {
		return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
	}

	*type = GNUTLS_NAME_DNS;

	if (*data_length > name.size) { /* greater since we need one extra byte for the null */
		*data_length = name.size;
		memcpy(data, name.data, *data_length);

		/* null terminate */
		_data[(*data_length)] = 0;

	} else {
		*data_length = name.size + 1;
		ret = GNUTLS_E_SHORT_MEMORY_BUFFER;
		goto cleanup;
	}

	ret = 0;
 cleanup:
	return ret;
}

/* This does not do any conversion not perform any check */
int
_gnutls_server_name_set_raw(gnutls_session_t session,
			    gnutls_server_name_type_t type,
			    const void *name, size_t name_length)
{
	int ret;
	gnutls_datum_t dname;

	if (name_length >= MAX_SERVER_NAME_SIZE) {
		return GNUTLS_E_INVALID_REQUEST;
	}

	_gnutls_hello_ext_unset_priv(session, GNUTLS_EXTENSION_SERVER_NAME);

	dname.data = (void*)name;
	dname.size = name_length;

	ret = _gnutls_hello_ext_set_datum(session, GNUTLS_EXTENSION_SERVER_NAME, &dname);
	if (ret < 0)
		return gnutls_assert_val(ret);

	return 0;
}

/**
 * gnutls_server_name_set:
 * @session: is a #gnutls_session_t type.
 * @type: specifies the indicator type
 * @name: is a string that contains the server name.
 * @name_length: holds the length of name excluding the terminating null byte
 *
 * This function is to be used by clients that want to inform (via a
 * TLS extension mechanism) the server of the name they connected to.
 * This should be used by clients that connect to servers that do
 * virtual hosting.
 *
 * The value of @name depends on the @type type.  In case of
 * %GNUTLS_NAME_DNS, a UTF-8 null-terminated domain name string,
 * without the trailing dot, is expected.
 *
 * IPv4 or IPv6 addresses are not permitted to be set by this function.
 * If the function is called with a name of @name_length zero it will clear
 * all server names set.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned,
 *   otherwise a negative error code is returned.
 **/
int
gnutls_server_name_set(gnutls_session_t session,
		       gnutls_server_name_type_t type,
		       const void *name, size_t name_length)
{
	int ret;
	gnutls_datum_t idn_name = {NULL,0};

	if (session->security_parameters.entity == GNUTLS_SERVER) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	if (name_length == 0) { /* unset extension */
		_gnutls_hello_ext_unset_priv(session, GNUTLS_EXTENSION_SERVER_NAME);
		return 0;
	}

	ret = gnutls_idna_map(name, name_length, &idn_name, 0);
	if (ret < 0) {
		 _gnutls_debug_log("unable to convert name %s to IDNA2008 format\n", (char*)name);
		 return ret;
	}

	name = idn_name.data;
	name_length = idn_name.size;

	ret = _gnutls_server_name_set_raw(session, type, name, name_length);
	gnutls_free(idn_name.data);

	return ret;
}

unsigned _gnutls_server_name_matches_resumed(gnutls_session_t session)
{
	gnutls_datum_t name1, name2;
	int ret;

	ret =
	    _gnutls_hello_ext_get_datum(session,
					GNUTLS_EXTENSION_SERVER_NAME,
					&name1);
	if (ret < 0) { /* no server name in this session */
		name1.data = NULL;
		name1.size = 0;
	}

	ret =
	    _gnutls_hello_ext_get_resumed_datum(session,
						GNUTLS_EXTENSION_SERVER_NAME,
						&name2);
	if (ret < 0) { /* no server name in this session */
		name2.data = NULL;
		name2.size = 0;
	}

	if (name1.data == NULL || name2.data == NULL) {
		if (name1.data == name2.data)
			return 1;
		else
			return 0;
	}

	if (name1.size != name2.size)
		return 0;

	if (memcmp(name1.data, name2.data, name1.size) != 0)
		return 0;

	return 1;
}
