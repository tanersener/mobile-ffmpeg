/*
 * Copyright (C) 2007-2012 Free Software Foundation, Inc.
 *
 * Author: Simon Josefsson
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

/* This file contains support functions for 'TLS Handshake Message for
 * Supplemental Data' (RFC 4680).
 *
 * The idea here is simple.  gnutls_handshake() in gnuts_handshake.c
 * will call _gnutls_gen_supplemental and _gnutls_parse_supplemental
 * when some extension requested that supplemental data be sent or
 * received.  Extension request this by setting the flags
 * do_recv_supplemental or do_send_supplemental in the session.
 *
 * The functions in this file iterate through the _gnutls_supplemental
 * array, and calls the send/recv functions for each respective data
 * type.
 *
 * The receive function of each data type is responsible for decoding
 * its own data.  If the extension did not expect to receive
 * supplemental data, it should return GNUTLS_E_UNEXPECTED_PACKET.
 * Otherwise, it just parse the data as normal.
 *
 * The send function needs to append the 2-byte data format type, and
 * append the 2-byte length of its data, and the data.  If it doesn't
 * want to send any data, it is fine to return without doing anything.
 */

#include "gnutls_int.h"
#include <gnutls/gnutls.h>
#include "supplemental.h"
#include "errors.h"
#include "num.h"

typedef struct gnutls_supplemental_entry_st {
	char *name;
	gnutls_supplemental_data_format_type_t type;
	gnutls_supp_recv_func supp_recv_func;
	gnutls_supp_send_func supp_send_func;
} gnutls_supplemental_entry_st;

static size_t suppfunc_size = 0;
static gnutls_supplemental_entry_st *suppfunc = NULL;

/**
 * gnutls_supplemental_get_name:
 * @type: is a supplemental data format type
 *
 * Convert a #gnutls_supplemental_data_format_type_t value to a
 * string.
 *
 * Returns: a string that contains the name of the specified
 *   supplemental data format type, or %NULL for unknown types.
 **/
const char
    *gnutls_supplemental_get_name(gnutls_supplemental_data_format_type_t
				  type)
{
	size_t i;

	for (i = 0; i < suppfunc_size; i++) {
		if (suppfunc[i].type == type)
			return suppfunc[i].name;
	}

	return NULL;
}

void _gnutls_supplemental_deinit(void)
{
	unsigned i;

	for (i = 0; i < suppfunc_size; i++) {
		gnutls_free(suppfunc[i].name);
	}
	gnutls_free(suppfunc);

	suppfunc = NULL;
	suppfunc_size = 0;
}

static gnutls_supp_recv_func
get_supp_func_recv(gnutls_session_t session, gnutls_supplemental_data_format_type_t type)
{
	size_t i;

	for (i = 0; i < session->internals.rsup_size; i++) {
		if (session->internals.rsup[i].type == type)
			return session->internals.rsup[i].supp_recv_func;
	}

	for (i = 0; i < suppfunc_size; i++) {
		if (suppfunc[i].type == type)
			return suppfunc[i].supp_recv_func;
	}

	return NULL;
}

static int gen_supplemental(gnutls_session_t session, const gnutls_supplemental_entry_st *supp,
			    gnutls_buffer_st * buf)
{
	int ret;
	gnutls_supp_send_func supp_send = supp->supp_send_func;
	size_t sizepos = buf->length;

	/* Make room for supplement type and length byte length field. */
	ret = _gnutls_buffer_append_data(buf, "\0\0\0\0", 4);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	ret = supp_send(session, buf);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	/* If data were added, store type+length, otherwise reset. */
	if (buf->length > sizepos + 4) {
		buf->data[sizepos] = (supp->type >> 8) & 0xFF;
		buf->data[sizepos + 1] = supp->type & 0xFF;
		buf->data[sizepos + 2] =
		    ((buf->length - sizepos - 4) >> 8) & 0xFF;
		buf->data[sizepos + 3] =
		    (buf->length - sizepos - 4) & 0xFF;
	} else
		buf->length -= 4;

	return 0;
}

int
_gnutls_gen_supplemental(gnutls_session_t session, gnutls_buffer_st * buf)
{
	size_t i;
	int ret;
	unsigned init_pos = buf->length;

	/* Make room for 3 byte length field. */
	ret = _gnutls_buffer_append_data(buf, "\0\0\0", 3);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	for (i = 0; i < session->internals.rsup_size; i++) {
		ret = gen_supplemental(session, &session->internals.rsup[i], buf);
		if (ret < 0)
			return gnutls_assert_val(ret);
	}

	for (i = 0; i < suppfunc_size; i++) {
		ret = gen_supplemental(session, &suppfunc[i], buf);
		if (ret < 0)
			return gnutls_assert_val(ret);
	}

	i = buf->length - init_pos - 3;

	buf->data[init_pos] = (i >> 16) & 0xFF;
	buf->data[init_pos+1] = (i >> 8) & 0xFF;
	buf->data[init_pos+2] = i & 0xFF;

	_gnutls_debug_log
	    ("EXT[%p]: Sending %d bytes of supplemental data\n", session,
	     (int) buf->length);

	return buf->length - init_pos;
}

int
_gnutls_parse_supplemental(gnutls_session_t session,
			   const uint8_t * data, int datalen)
{
	const uint8_t *p = data;
	size_t dsize = datalen;
	size_t total_size;

	DECR_LEN(dsize, 3);
	total_size = _gnutls_read_uint24(p);
	p += 3;

	if (dsize != total_size) {
		gnutls_assert();
		return GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER;
	}

	do {
		uint16_t supp_data_type;
		uint16_t supp_data_length;
		gnutls_supp_recv_func recv_func;

		DECR_LEN(dsize, 2);
		supp_data_type = _gnutls_read_uint16(p);
		p += 2;

		DECR_LEN(dsize, 2);
		supp_data_length = _gnutls_read_uint16(p);
		p += 2;

		_gnutls_debug_log
		    ("EXT[%p]: Got supplemental type=%02x length=%d\n",
		     session, supp_data_type, supp_data_length);

		recv_func = get_supp_func_recv(session, supp_data_type);
		if (recv_func) {
			int ret = recv_func(session, p, supp_data_length);
			if (ret < 0) {
				gnutls_assert();
				return ret;
			}
		} else {
			gnutls_assert();
			return GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER;
		}

		DECR_LEN(dsize, supp_data_length);
		p += supp_data_length;
	}
	while (dsize > 0);

	return 0;
}

static int
_gnutls_supplemental_register(gnutls_supplemental_entry_st *entry)
{
	gnutls_supplemental_entry_st *p;
	unsigned i;

	for (i = 0; i < suppfunc_size; i++) {
		if (entry->type == suppfunc[i].type)
			return gnutls_assert_val(GNUTLS_E_ALREADY_REGISTERED);
	}

	p = gnutls_realloc_fast(suppfunc,
				sizeof(*suppfunc) * (suppfunc_size + 1));
	if (!p) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	suppfunc = p;

	memcpy(&suppfunc[suppfunc_size], entry, sizeof(*entry));

	suppfunc_size++;

	return GNUTLS_E_SUCCESS;
}

/**
 * gnutls_supplemental_register:
 * @name: the name of the supplemental data to register
 * @type: the type of the supplemental data format
 * @recv_func: the function to receive the data
 * @send_func: the function to send the data
 *
 * This function will register a new supplemental data type (rfc4680).
 * The registered data will remain until gnutls_global_deinit()
 * is called. The provided @type must be an unassigned type in
 * %gnutls_supplemental_data_format_type_t. If the type is already
 * registered or handled by GnuTLS internally %GNUTLS_E_ALREADY_REGISTERED
 * will be returned.
 *
 * This function is not thread safe. As supplemental data are not defined under
 * TLS 1.3, this function will disable TLS 1.3 support globally.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, otherwise a negative error code.
 *
 * Since: 3.4.0
 **/
int
gnutls_supplemental_register(const char *name, gnutls_supplemental_data_format_type_t type,
			     gnutls_supp_recv_func recv_func, gnutls_supp_send_func send_func)
{
	gnutls_supplemental_entry_st tmp_entry;
	int ret;

	tmp_entry.name = gnutls_strdup(name);
	tmp_entry.type = type;
	tmp_entry.supp_recv_func = recv_func;
	tmp_entry.supp_send_func = send_func;
	
	ret = _gnutls_supplemental_register(&tmp_entry);
	if (ret < 0) {
		gnutls_free(tmp_entry.name);
	}

	_gnutls_disable_tls13 = 1;

	return ret;
}

/**
 * gnutls_session_supplemental_register:
 * @session: the session for which this will be registered
 * @name: the name of the supplemental data to register
 * @type: the type of the supplemental data format
 * @recv_func: the function to receive the data
 * @send_func: the function to send the data
 * @flags: must be zero
 *
 * This function will register a new supplemental data type (rfc4680).
 * The registered supplemental functions will be used for that specific
 * session. The provided @type must be an unassigned type in
 * %gnutls_supplemental_data_format_type_t.
 *
 * If the type is already registered or handled by GnuTLS internally
 * %GNUTLS_E_ALREADY_REGISTERED will be returned.
 *
 * As supplemental data are not defined under TLS 1.3, this function will
 * disable TLS 1.3 support for the given session.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, otherwise a negative error code.
 *
 * Since: 3.5.5
 **/
int
gnutls_session_supplemental_register(gnutls_session_t session, const char *name,
				     gnutls_supplemental_data_format_type_t type,
				     gnutls_supp_recv_func recv_func,
				     gnutls_supp_send_func send_func,
				     unsigned flags)
{
	gnutls_supplemental_entry_st tmp_entry;
	gnutls_supplemental_entry_st *p;
	unsigned i;

	tmp_entry.name = NULL;
	tmp_entry.type = type;
	tmp_entry.supp_recv_func = recv_func;
	tmp_entry.supp_send_func = send_func;

	for (i = 0; i < suppfunc_size; i++) {
		if (type == suppfunc[i].type)
			return gnutls_assert_val(GNUTLS_E_ALREADY_REGISTERED);
	}

	p = gnutls_realloc(session->internals.rsup,
			   sizeof(gnutls_supplemental_entry_st)*(session->internals.rsup_size + 1));
	if (!p)
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

	session->internals.rsup = p;

	memcpy(&session->internals.rsup[session->internals.rsup_size], &tmp_entry, sizeof(tmp_entry));
	session->internals.rsup_size++;

	session->internals.flags |= INT_FLAG_NO_TLS13;

	return GNUTLS_E_SUCCESS;
}

/**
 * gnutls_supplemental_recv:
 * @session: is a #gnutls_session_t type.
 * @do_recv_supplemental: non-zero in order to expect supplemental data
 *
 * This function is to be called by an extension handler to
 * instruct gnutls to attempt to receive supplemental data
 * during the handshake process.
 *
 * Since: 3.4.0
 **/
void
gnutls_supplemental_recv(gnutls_session_t session, unsigned do_recv_supplemental)
{
	session->security_parameters.do_recv_supplemental = do_recv_supplemental;
}

/**
 * gnutls_supplemental_send:
 * @session: is a #gnutls_session_t type.
 * @do_send_supplemental: non-zero in order to send supplemental data
 *
 * This function is to be called by an extension handler to
 * instruct gnutls to send supplemental data during the handshake process.
 *
 * Since: 3.4.0
 **/
void
gnutls_supplemental_send(gnutls_session_t session, unsigned do_send_supplemental)
{
	session->security_parameters.do_send_supplemental = do_send_supplemental;
}
