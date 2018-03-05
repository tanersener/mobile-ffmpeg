/*
 * Copyright (C) 2002-2012 Free Software Foundation, Inc.
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 */

#include "gnutls_int.h"
#include "auth.h"
#include "errors.h"
#include "num.h"
#include "str.h"
#include <ext/server_name.h>

static int _gnutls_server_name_recv_params(gnutls_session_t session,
					   const uint8_t * data,
					   size_t data_size);
static int _gnutls_server_name_send_params(gnutls_session_t session,
					   gnutls_buffer_st * extdata);

static int _gnutls_server_name_unpack(gnutls_buffer_st * ps,
				      extension_priv_data_t * _priv);
static int _gnutls_server_name_pack(extension_priv_data_t _priv,
				    gnutls_buffer_st * ps);
static void _gnutls_server_name_deinit_data(extension_priv_data_t priv);

int
_gnutls_server_name_set_raw(gnutls_session_t session,
		       gnutls_server_name_type_t type,
		       const void *name, size_t name_length);

const extension_entry_st ext_mod_server_name = {
	.name = "Server Name Indication",
	.type = GNUTLS_EXTENSION_SERVER_NAME,
	.parse_type = GNUTLS_EXT_MANDATORY,

	.recv_func = _gnutls_server_name_recv_params,
	.send_func = _gnutls_server_name_send_params,
	.pack_func = _gnutls_server_name_pack,
	.unpack_func = _gnutls_server_name_unpack,
	.deinit_func = _gnutls_server_name_deinit_data,
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
	int i, j;
	const unsigned char *p;
	uint16_t len, type;
	ssize_t data_size = _data_size;
	int server_names = 0;
	server_name_ext_st *priv;
	extension_priv_data_t epriv;

	if (session->security_parameters.entity == GNUTLS_SERVER) {
		DECR_LENGTH_RET(data_size, 2, 0);
		len = _gnutls_read_uint16(data);

		if (len != data_size) {
			/* This is unexpected packet length, but
			 * just ignore it, for now.
			 */
			gnutls_assert();
			return 0;
		}

		p = data + 2;

		/* Count all server_names in the packet. */
		while (data_size > 0) {
			DECR_LENGTH_RET(data_size, 1, 0);
			p++;

			DECR_LEN(data_size, 2);
			len = _gnutls_read_uint16(p);
			p += 2;

			if (len > 0) {
				DECR_LENGTH_RET(data_size, len, 0);
				server_names++;
				p += len;
			} else
				_gnutls_handshake_log
				    ("HSK[%p]: Received (0) size server name (under attack?)\n",
				     session);

		}

		/* we cannot accept more server names.
		 */
		if (server_names > MAX_SERVER_NAME_EXTENSIONS) {
			_gnutls_handshake_log
			    ("HSK[%p]: Too many server names received (under attack?)\n",
			     session);
			server_names = MAX_SERVER_NAME_EXTENSIONS;
		}

		if (server_names == 0)
			return 0;	/* no names found */

		priv = gnutls_calloc(1, sizeof(*priv));
		if (priv == NULL) {
			gnutls_assert();
			return GNUTLS_E_MEMORY_ERROR;
		}

		p = data + 2;
		for (j = i = 0; i < server_names; i++) {
			type = *p;
			p++;

			len = _gnutls_read_uint16(p);
			p += 2;

			switch (type) {
			case 0:	/* NAME_DNS */
				if (len < MAX_SERVER_NAME_SIZE) {
					memcpy(priv->server_names[j].name,
					       p, len);
					priv->server_names[j].name[len] = 0;
					priv->server_names[j].name_length =
					    strlen((char*)priv->server_names[j].name);
					if (priv->server_names[j].name_length == len) {
						/* valid ascii with no embedded NULL */
						priv->server_names[j].type =
						    GNUTLS_NAME_DNS;
						j++;
					}
					break;
				}
			}

			/* move to next record */
			p += len;
		}

		priv->server_names_size = j;

		epriv = priv;
		_gnutls_ext_set_session_data(session,
					     GNUTLS_EXTENSION_SERVER_NAME,
					     epriv);

	}

	return 0;
}

/* returns data_size or a negative number on failure
 */
static int
_gnutls_server_name_send_params(gnutls_session_t session,
				gnutls_buffer_st * extdata)
{
	uint16_t len;
	unsigned i;
	int total_size = 0, ret;
	server_name_ext_st *priv;
	extension_priv_data_t epriv;

	ret =
	    _gnutls_ext_get_session_data(session,
					 GNUTLS_EXTENSION_SERVER_NAME,
					 &epriv);
	if (ret < 0)
		return 0;

	/* this function sends the client extension data (dnsname)
	 */
	if (session->security_parameters.entity == GNUTLS_CLIENT) {
		priv = epriv;

		if (priv->server_names_size == 0)
			return 0;

		/* uint16_t
		 */
		total_size = 2;
		for (i = 0; i < priv->server_names_size; i++) {
			/* count the total size
			 */
			len = priv->server_names[i].name_length;

			/* uint8_t + uint16_t + size
			 */
			total_size += 1 + 2 + len;
		}

		/* UINT16: write total size of all names
		 */
		ret =
		    _gnutls_buffer_append_prefix(extdata, 16,
						 total_size - 2);
		if (ret < 0)
			return gnutls_assert_val(ret);

		for (i = 0; i < priv->server_names_size; i++) {

			switch (priv->server_names[i].type) {
			case GNUTLS_NAME_DNS:
				len = priv->server_names[i].name_length;
				if (len == 0)
					break;

				/* UINT8: type of this extension
				 * UINT16: size of the first name
				 * LEN: the actual server name.
				 */
				ret =
				    _gnutls_buffer_append_prefix(extdata,
								 8, 0);
				if (ret < 0)
					return gnutls_assert_val(ret);

				_gnutls_debug_log("HSK[%p]: sent server name: '%s'\n", session, priv->server_names[i].name);

				ret =
				    _gnutls_buffer_append_data_prefix
				    (extdata, 16,
				     priv->server_names[i].name, len);
				if (ret < 0)
					return gnutls_assert_val(ret);

				break;
			default:
				gnutls_assert();
				return GNUTLS_E_INTERNAL_ERROR;
			}
		}
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
 * @index is used to retrieve more than one server names (if sent by
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
	server_name_ext_st *priv;
	int ret;
	gnutls_datum_t idn_name = {NULL,0};
	extension_priv_data_t epriv;

	if (session->security_parameters.entity == GNUTLS_CLIENT) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	ret =
	    _gnutls_ext_get_session_data(session,
					 GNUTLS_EXTENSION_SERVER_NAME,
					 &epriv);
	if (ret < 0) {
		gnutls_assert();
		return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
	}

	priv = epriv;

	if (indx + 1 > priv->server_names_size) {
		return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
	}

	*type = priv->server_names[indx].type;

	ret = gnutls_idna_map((char*)priv->server_names[indx].name, priv->server_names[indx].name_length, &idn_name, 0);
	if (ret < 0) {
		 _gnutls_debug_log("unable to convert name %s to IDNA2003 format\n", (char*)priv->server_names[indx].name);
		 return GNUTLS_E_IDNA_ERROR;
	}

	if (*data_length >	/* greater since we need one extra byte for the null */
	    idn_name.size) {
		*data_length = idn_name.size;
		memcpy(data, idn_name.data, *data_length);

		if (*type == GNUTLS_NAME_DNS)	/* null terminate */
			_data[(*data_length)] = 0;

	} else {
		*data_length = idn_name.size + 1;
		ret = GNUTLS_E_SHORT_MEMORY_BUFFER;
		goto cleanup;
	}

	ret = 0;
 cleanup:
	gnutls_free(idn_name.data);
	return ret;
}

/* This does not do any conversion not perform any check */
int
_gnutls_server_name_set_raw(gnutls_session_t session,
		       gnutls_server_name_type_t type,
		       const void *name, size_t name_length)
{
	int server_names, ret;
	server_name_ext_st *priv;
	extension_priv_data_t epriv;
	int set = 0;

	if (name_length > MAX_SERVER_NAME_SIZE) {
		return GNUTLS_E_SHORT_MEMORY_BUFFER;
	}

	ret =
	    _gnutls_ext_get_session_data(session,
					 GNUTLS_EXTENSION_SERVER_NAME,
					 &epriv);
	if (ret < 0) {
		set = 1;
	}

	if (set != 0) {
		priv = gnutls_calloc(1, sizeof(*priv));
		if (priv == NULL) {
			gnutls_assert();
			return GNUTLS_E_MEMORY_ERROR;
		}
		epriv = priv;
	} else
		priv = epriv;

	server_names = priv->server_names_size + 1;

	if (server_names > MAX_SERVER_NAME_EXTENSIONS)
		server_names = MAX_SERVER_NAME_EXTENSIONS;

	priv->server_names[server_names - 1].type = type;

	if (name_length > 0) {
		memcpy(priv->server_names[server_names - 1].name, name,
		       name_length);
		priv->server_names[server_names - 1].name[name_length] = 0;
	}
	priv->server_names[server_names - 1].name_length = name_length;

	priv->server_names_size = server_names;

	if (set != 0)
		_gnutls_ext_set_session_data(session,
					     GNUTLS_EXTENSION_SERVER_NAME,
					     epriv);

	return 0;
}

/**
 * gnutls_server_name_set:
 * @session: is a #gnutls_session_t type.
 * @type: specifies the indicator type
 * @name: is a string that contains the server name.
 * @name_length: holds the length of name
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
		_gnutls_ext_unset_session_data(session, GNUTLS_EXTENSION_SERVER_NAME);
		return 0;
	}

	ret = gnutls_idna_map(name, name_length, &idn_name, 0);
	if (ret < 0) {
		 _gnutls_debug_log("unable to convert name %s to IDNA2003 format\n", (char*)name);
		 return ret;
	}

	name = idn_name.data;
	name_length = idn_name.size;

	ret = _gnutls_server_name_set_raw(session, type, name, name_length);
	gnutls_free(idn_name.data);

	return ret;
}

static void _gnutls_server_name_deinit_data(extension_priv_data_t priv)
{
	gnutls_free(priv);
}

static int
_gnutls_server_name_pack(extension_priv_data_t epriv,
			 gnutls_buffer_st * ps)
{
	server_name_ext_st *priv = epriv;
	unsigned int i;
	int ret;

	BUFFER_APPEND_NUM(ps, priv->server_names_size);
	for (i = 0; i < priv->server_names_size; i++) {
		BUFFER_APPEND_NUM(ps, priv->server_names[i].type);
		BUFFER_APPEND_PFX4(ps, priv->server_names[i].name,
				   priv->server_names[i].name_length);
	}
	return 0;
}

static int
_gnutls_server_name_unpack(gnutls_buffer_st * ps,
			   extension_priv_data_t * _priv)
{
	server_name_ext_st *priv;
	unsigned int i;
	int ret;
	extension_priv_data_t epriv;

	priv = gnutls_calloc(1, sizeof(*priv));
	if (priv == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	BUFFER_POP_NUM(ps, priv->server_names_size);
	for (i = 0; i < priv->server_names_size; i++) {
		BUFFER_POP_NUM(ps, priv->server_names[i].type);
		BUFFER_POP_NUM(ps, priv->server_names[i].name_length);
		if (priv->server_names[i].name_length >
		    sizeof(priv->server_names[i].name)) {
			gnutls_assert();
			return GNUTLS_E_PARSING_ERROR;
		}
		BUFFER_POP(ps, priv->server_names[i].name,
			   priv->server_names[i].name_length);
	}

	epriv = priv;
	*_priv = epriv;

	return 0;

      error:
	gnutls_free(priv);
	return ret;
}

unsigned _gnutls_server_name_matches_resumed(gnutls_session_t session)
{
	server_name_ext_st *priv1, *priv2;
	int ret;
	gnutls_ext_priv_data_t epriv;

	ret =
	    _gnutls_ext_get_session_data(session,
					 GNUTLS_EXTENSION_SERVER_NAME,
					 &epriv);
	if (ret < 0) /* no server name in this session */
		priv1 = NULL;
	else
		priv1 = epriv;

	ret =
	    _gnutls_ext_get_resumed_session_data(session,
						 GNUTLS_EXTENSION_SERVER_NAME,
						 &epriv);
	if (ret < 0) /* no server name in extensions */
		priv2 = NULL;
	else
		priv2 = epriv;

	if (priv1 == NULL || priv2 == NULL) {
		if (priv1 == priv2)
			return 1;
		else
			return 0;
	}

	if (priv1->server_names_size != priv2->server_names_size)
		return 0;

	if (priv1->server_names_size == 0)
		return 1;

	if (priv1->server_names[0].name_length != priv2->server_names[0].name_length)
		return 0;

	if (memcmp(priv1->server_names[0].name, priv2->server_names[0].name, priv1->server_names[0].name_length) != 0)
		return 0;

	return 1;
}
