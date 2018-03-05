/*
 * Copyright (C) 2001-2016 Free Software Foundation, Inc.
 * Copyright (C) 2015-2016 Red Hat, Inc.
 *
 * Author: Nikos Mavrogiannopoulos, Simon Josefsson
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

/* Functions that relate to the TLS hello extension parsing.
 * Hello extensions are packets appended in the TLS hello packet, and
 * allow for extra functionality.
 */

#include "gnutls_int.h"
#include "extensions.h"
#include "errors.h"
#include "ext/max_record.h"
#include <ext/cert_type.h>
#include <ext/server_name.h>
#include <ext/srp.h>
#include <ext/heartbeat.h>
#include <ext/session_ticket.h>
#include <ext/safe_renegotiation.h>
#include <ext/signature.h>
#include <ext/safe_renegotiation.h>
#include <ext/ecc.h>
#include <ext/status_request.h>
#include <ext/ext_master_secret.h>
#include <ext/srtp.h>
#include <ext/alpn.h>
#include <ext/dumbfw.h>
#include <ext/etm.h>
#include <num.h>


static int ext_register(extension_entry_st * mod);
static void _gnutls_ext_unset_resumed_session_data(gnutls_session_t
						   session, uint16_t type);

static extension_entry_st const *extfunc[MAX_EXT_TYPES+1] = {
	&ext_mod_max_record_size,
	&ext_mod_ext_master_secret,
	&ext_mod_etm,
#ifdef ENABLE_OCSP
	&ext_mod_status_request,
#endif
#ifdef ENABLE_OPENPGP
	&ext_mod_cert_type,
#endif
	&ext_mod_server_name,
	&ext_mod_sr,
#ifdef ENABLE_SRP
	&ext_mod_srp,
#endif
#ifdef ENABLE_HEARTBEAT
	&ext_mod_heartbeat,
#endif
#ifdef ENABLE_SESSION_TICKETS
	&ext_mod_session_ticket,
#endif
	&ext_mod_supported_ecc,
	&ext_mod_supported_ecc_pf,
	&ext_mod_sig,
#ifdef ENABLE_DTLS_SRTP
	&ext_mod_srtp,
#endif
#ifdef ENABLE_ALPN
	&ext_mod_alpn,
#endif
	/* This must be the last extension registered.
	 */
	&ext_mod_dumbfw,
	NULL
};

static gnutls_ext_parse_type_t _gnutls_ext_parse_type(gnutls_session_t session, uint16_t type)
{
	size_t i;

	for (i=0;i<session->internals.rexts_size;i++) {
		if (session->internals.rexts[i].type == type)
			return session->internals.rexts[i].parse_type;
	}

	for (i = 0; extfunc[i] != NULL; i++) {
		if (extfunc[i]->type == type)
			return extfunc[i]->parse_type;
	}

	return GNUTLS_EXT_NONE;
}

static gnutls_ext_recv_func
_gnutls_ext_func_recv(gnutls_session_t session, uint16_t type, gnutls_ext_parse_type_t parse_type)
{
	size_t i;

	for (i=0;i<session->internals.rexts_size;i++) {
		if (session->internals.rexts[i].type == type)
			if (parse_type == GNUTLS_EXT_ANY
			    || session->internals.rexts[i].parse_type == parse_type)
				return session->internals.rexts[i].recv_func;
	}

	for (i = 0; extfunc[i] != NULL; i++)
		if (extfunc[i]->type == type)
			if (parse_type == GNUTLS_EXT_ANY
			    || extfunc[i]->parse_type == parse_type)
				return extfunc[i]->recv_func;

	return NULL;
}

static gnutls_ext_deinit_data_func _gnutls_ext_func_deinit(gnutls_session_t session, uint16_t type)
{
	size_t i;

	for (i=0;i<session->internals.rexts_size;i++) {
		if (session->internals.rexts[i].type == type)
			return session->internals.rexts[i].deinit_func;
	}

	for (i = 0; extfunc[i] != NULL; i++)
		if (extfunc[i]->type == type)
			return extfunc[i]->deinit_func;

	return NULL;
}

static gnutls_ext_unpack_func _gnutls_ext_func_unpack(gnutls_session_t session, uint16_t type)
{
	size_t i;

	for (i=0;i<session->internals.rexts_size;i++) {
		if (session->internals.rexts[i].type == type)
			return session->internals.rexts[i].unpack_func;
	}

	for (i = 0; extfunc[i] != NULL; i++)
		if (extfunc[i]->type == type)
			return extfunc[i]->unpack_func;

	return NULL;
}


/**
 * gnutls_ext_get_name:
 * @ext: is a TLS extension numeric ID
 *
 * Convert a TLS extension numeric ID to a printable string.
 *
 * Returns: a pointer to a string that contains the name of the
 *   specified cipher, or %NULL.
 **/
const char *gnutls_ext_get_name(unsigned int ext)
{
	size_t i;

	for (i = 0; extfunc[i] != NULL; i++)
		if (extfunc[i]->type == ext)
			return extfunc[i]->name;

	return NULL;
}

/* Checks if the extension @type provided has been requested
 * by us (in client side). In that case it returns zero, 
 * otherwise a negative error value.
 */
int
_gnutls_extension_list_check(gnutls_session_t session, uint16_t type)
{
	int i;

	for (i = 0; i < session->internals.extensions_sent_size; i++) {
		if (type == session->internals.extensions_sent[i])
			return 0;	/* ok found */
	}

	return GNUTLS_E_RECEIVED_ILLEGAL_EXTENSION;
}

int
_gnutls_parse_extensions(gnutls_session_t session,
			 gnutls_ext_parse_type_t parse_type,
			 const uint8_t * data, int data_size)
{
	int next, ret;
	int pos = 0;
	uint16_t type;
	const uint8_t *sdata;
	gnutls_ext_recv_func ext_recv;
	uint16_t size;

#ifdef DEBUG
	int i;

	if (session->security_parameters.entity == GNUTLS_CLIENT)
		for (i = 0; i < session->internals.extensions_sent_size;
		     i++) {
			_gnutls_handshake_log
			    ("EXT[%d]: expecting extension '%s'\n",
			     session,
			     gnutls_ext_get_name(session->internals.
							extensions_sent
							[i]));
		}
#endif

	DECR_LENGTH_RET(data_size, 2, 0);
	next = _gnutls_read_uint16(data);
	pos += 2;

	DECR_LENGTH_RET(data_size, next, GNUTLS_E_UNEXPECTED_EXTENSIONS_LENGTH);

	do {
		DECR_LENGTH_RET(next, 2, GNUTLS_E_UNEXPECTED_EXTENSIONS_LENGTH);
		type = _gnutls_read_uint16(&data[pos]);
		pos += 2;

		if (session->security_parameters.entity == GNUTLS_CLIENT) {
			if ((ret =
			     _gnutls_extension_list_check(session, type)) < 0) {
				gnutls_assert();
				return ret;
			}
		}

		DECR_LENGTH_RET(next, 2, GNUTLS_E_UNEXPECTED_EXTENSIONS_LENGTH);
		size = _gnutls_read_uint16(&data[pos]);
		pos += 2;

		DECR_LENGTH_RET(next, size, GNUTLS_E_UNEXPECTED_EXTENSIONS_LENGTH);
		sdata = &data[pos];
		pos += size;

		ext_recv = _gnutls_ext_func_recv(session, type, parse_type);
		if (ext_recv == NULL) {
			_gnutls_handshake_log
			    ("EXT[%p]: Found extension '%s/%d'\n", session,
			     gnutls_ext_get_name(type), type);

			continue;
		}

		/* only store the extension number if we support it */
		if (session->security_parameters.entity == GNUTLS_SERVER) {
			_gnutls_extension_list_add(session, type);
		}

		_gnutls_handshake_log
		    ("EXT[%p]: Parsing extension '%s/%d' (%d bytes)\n",
		     session, gnutls_ext_get_name(type), type,
		     size);

		if ((ret = ext_recv(session, sdata, size)) < 0) {
			gnutls_assert();
			return ret;
		}

	}
	while (next > 2);

	return 0;

}

/* Adds the extension we want to send in the extensions list.
 * This list is used in client side to check whether the (later) received
 * extensions are the ones we requested.
 *
 * In server side, this list is used to ensure we don't send
 * extensions that we didn't receive a corresponding value.
 */
void _gnutls_extension_list_add(gnutls_session_t session, uint16_t type)
{

	if (session->internals.extensions_sent_size <
	    MAX_EXT_TYPES) {
		session->internals.extensions_sent[session->
						   internals.extensions_sent_size]
		    = type;
		session->internals.extensions_sent_size++;
	} else {
		_gnutls_handshake_log
		    ("extensions: Increase MAX_EXT_TYPES\n");
	}
}

static
int send_extension(gnutls_session_t session, const extension_entry_st *p,
		   gnutls_buffer_st *extdata, gnutls_ext_parse_type_t parse_type)
{
	int size_pos, size, ret;

	if (p->send_func == NULL)
		return 0;

	if (parse_type != GNUTLS_EXT_ANY
	    && p->parse_type != parse_type)
		return 0;

	/* ensure we are sending only what we received */
	if (session->security_parameters.entity == GNUTLS_SERVER) {
		if ((ret =
		     _gnutls_extension_list_check(session, p->type)) < 0) {
			return 0;
		}
	}

	ret = _gnutls_buffer_append_prefix(extdata, 16, p->type);
	if (ret < 0)
		return gnutls_assert_val(ret);

	size_pos = extdata->length;
	ret = _gnutls_buffer_append_prefix(extdata, 16, 0);
	if (ret < 0)
		return gnutls_assert_val(ret);

	size = p->send_func(session, extdata);
	/* returning GNUTLS_E_INT_RET_0 means to send an empty
	 * extension of this type.
	 */
	if (size > 0 || size == GNUTLS_E_INT_RET_0) {
		if (size == GNUTLS_E_INT_RET_0)
			size = 0;

		/* write the real size */
		_gnutls_write_uint16(size,
				     &extdata->data[size_pos]);

		/* add this extension to the extension list
		 */
		if (session->security_parameters.entity == GNUTLS_CLIENT)
			_gnutls_extension_list_add(session, p->type);

		_gnutls_handshake_log
			    ("EXT[%p]: Sending extension %s (%d bytes)\n",
			     session, p->name, size);
	} else if (size < 0) {
		gnutls_assert();
		return size;
	} else if (size == 0)
		extdata->length -= 4;	/* reset type and size */

	return 0;
}

int
_gnutls_gen_extensions(gnutls_session_t session,
		       gnutls_buffer_st * extdata,
		       gnutls_ext_parse_type_t parse_type)
{
	int size;
	int pos, ret;
	size_t i, init_size = extdata->length;
	const extension_entry_st *extp;

	pos = extdata->length;	/* we will store length later on */

	ret = _gnutls_buffer_append_prefix(extdata, 16, 0);
	if (ret < 0)
		return gnutls_assert_val(ret);

	for (i=0;i<session->internals.rexts_size;i++) {
		extp = &session->internals.rexts[i];

		ret = send_extension(session, extp, extdata, parse_type);
		if (ret < 0)
			return gnutls_assert_val(ret);
	}

	for (i = 0; extfunc[i] != NULL; i++) {
		extp = extfunc[i];

		ret = send_extension(session, extp, extdata, parse_type);
		if (ret < 0)
			return gnutls_assert_val(ret);
	}

	/* remove any initial data, and the size of the header */
	size = extdata->length - init_size - 2;

	if (size > UINT16_MAX) /* sent too many extensions */
		return gnutls_assert_val(GNUTLS_E_HANDSHAKE_TOO_LARGE);

	if (size > 0)
		_gnutls_write_uint16(size, &extdata->data[pos]);
	else if (size == 0)
		extdata->length -= 2;	/* the length bytes */

	return size;
}

int _gnutls_ext_init(void)
{
	return GNUTLS_E_SUCCESS;
}

void _gnutls_ext_deinit(void)
{
	unsigned i;
	for (i = 0; extfunc[i] != NULL; i++) {
		if (extfunc[i]->free_struct != 0) {
			gnutls_free((void*)extfunc[i]->name);
			gnutls_free((void*)extfunc[i]);
			extfunc[i] = NULL;
		}
	}
}

static
int ext_register(extension_entry_st * mod)
{
	unsigned i = 0;

	while(extfunc[i] != NULL) {
		i++;
	}

	if (i >= MAX_EXT_TYPES-1) {
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);
	}

	extfunc[i] = mod;
	extfunc[i+1] = NULL;
	return GNUTLS_E_SUCCESS;
}

static int pack_extension(gnutls_session_t session, const extension_entry_st *extp,
			  gnutls_buffer_st *packed)
{
	int ret;
	int size_offset;
	int cur_size;
	gnutls_ext_priv_data_t data;
	int rval = 0;

	ret =
	    _gnutls_ext_get_session_data(session, extp->type,
					 &data);
	if (ret >= 0 && extp->pack_func != NULL) {
		BUFFER_APPEND_NUM(packed, extp->type);

		size_offset = packed->length;
		BUFFER_APPEND_NUM(packed, 0);

		cur_size = packed->length;

		ret = extp->pack_func(data, packed);
		if (ret < 0) {
			gnutls_assert();
			return ret;
		}

		rval = 1;
		/* write the actual size */
		_gnutls_write_uint32(packed->length - cur_size,
				     packed->data + size_offset);
	}

	return rval;
}

int _gnutls_ext_pack(gnutls_session_t session, gnutls_buffer_st *packed)
{
	unsigned int i;
	int ret;
	int total_exts_pos;
	int exts = 0;

	total_exts_pos = packed->length;
	BUFFER_APPEND_NUM(packed, 0);

	for (i = 0; i < session->internals.rexts_size; i++) {
		ret = pack_extension(session, &session->internals.rexts[i], packed);
		if (ret < 0)
			return gnutls_assert_val(ret);

		if (ret > 0)
			exts++;
	}

	for (i = 0; extfunc[i] != NULL; i++) {
		ret = pack_extension(session, extfunc[i], packed);
		if (ret < 0)
			return gnutls_assert_val(ret);

		if (ret > 0)
			exts++;
	}

	_gnutls_write_uint32(exts, packed->data + total_exts_pos);

	return 0;
}

void _gnutls_ext_restore_resumed_session(gnutls_session_t session)
{
	int i;

	/* clear everything except MANDATORY extensions */
	for (i = 0; i < MAX_EXT_TYPES; i++) {
		if (session->internals.extension_int_data[i].set != 0 &&
		    _gnutls_ext_parse_type(session, session->
					   internals.extension_int_data[i].
					   type) != GNUTLS_EXT_MANDATORY) {
			_gnutls_ext_unset_session_data(session,
						       session->internals.
						       extension_int_data
						       [i].type);
		}
	}

	/* copy resumed to main */
	for (i = 0; i < MAX_EXT_TYPES; i++) {
		if (session->internals.resumed_extension_int_data[i].set !=
		    0
		    && _gnutls_ext_parse_type(session, session->internals.
					      resumed_extension_int_data
					      [i].type) !=
		    GNUTLS_EXT_MANDATORY) {
			_gnutls_ext_set_session_data(session,
						     session->internals.
						     resumed_extension_int_data
						     [i].type,
						     session->internals.
						     resumed_extension_int_data
						     [i].priv);
			session->internals.resumed_extension_int_data[i].
			    set = 0;
		}
	}

}


static void
_gnutls_ext_set_resumed_session_data(gnutls_session_t session,
				     uint16_t type,
				     gnutls_ext_priv_data_t data)
{
	int i;

	for (i = 0; i < MAX_EXT_TYPES; i++) {
		if (session->internals.resumed_extension_int_data[i].
		    type == type
		    || session->internals.resumed_extension_int_data[i].
		    set == 0) {

			if (session->internals.
			    resumed_extension_int_data[i].set != 0)
				_gnutls_ext_unset_resumed_session_data
				    (session, type);

			session->internals.resumed_extension_int_data[i].
			    type = type;
			session->internals.resumed_extension_int_data[i].
			    priv = data;
			session->internals.resumed_extension_int_data[i].
			    set = 1;
			return;
		}
	}
}

int _gnutls_ext_unpack(gnutls_session_t session, gnutls_buffer_st * packed)
{
	int i, ret;
	gnutls_ext_priv_data_t data;
	gnutls_ext_unpack_func unpack;
	int max_exts = 0;
	uint16_t type;
	int size_for_type, cur_pos;


	BUFFER_POP_NUM(packed, max_exts);
	for (i = 0; i < max_exts; i++) {
		BUFFER_POP_NUM(packed, type);
		BUFFER_POP_NUM(packed, size_for_type);

		cur_pos = packed->length;

		unpack = _gnutls_ext_func_unpack(session, type);
		if (unpack == NULL) {
			gnutls_assert();
			return GNUTLS_E_PARSING_ERROR;
		}

		ret = unpack(packed, &data);
		if (ret < 0) {
			gnutls_assert();
			return ret;
		}

		/* verify that unpack read the correct bytes */
		cur_pos = cur_pos - packed->length;
		if (cur_pos /* read length */  != size_for_type) {
			gnutls_assert();
			return GNUTLS_E_PARSING_ERROR;
		}

		_gnutls_ext_set_resumed_session_data(session, type, data);
	}

	return 0;

      error:
	return ret;
}

void
_gnutls_ext_unset_session_data(gnutls_session_t session, uint16_t type)
{
	gnutls_ext_deinit_data_func deinit;
	gnutls_ext_priv_data_t data;
	int ret, i;

	deinit = _gnutls_ext_func_deinit(session, type);
	ret = _gnutls_ext_get_session_data(session, type, &data);

	if (ret >= 0 && deinit != NULL) {
		deinit(data);
	}

	for (i = 0; i < MAX_EXT_TYPES; i++) {
		if (session->internals.extension_int_data[i].type == type) {
			session->internals.extension_int_data[i].set = 0;
			return;
		}
	}

}

static void
_gnutls_ext_unset_resumed_session_data(gnutls_session_t session,
				       uint16_t type)
{
	gnutls_ext_deinit_data_func deinit;
	gnutls_ext_priv_data_t data;
	int ret, i;

	deinit = _gnutls_ext_func_deinit(session, type);
	ret = _gnutls_ext_get_resumed_session_data(session, type, &data);

	if (ret >= 0 && deinit != NULL) {
		deinit(data);
	}

	for (i = 0; i < MAX_EXT_TYPES; i++) {
		if (session->internals.resumed_extension_int_data[i].
		    type == type) {
			session->internals.resumed_extension_int_data[i].
			    set = 0;
			return;
		}
	}

}

/* Deinitializes all data that are associated with TLS extensions.
 */
void _gnutls_ext_free_session_data(gnutls_session_t session)
{
	unsigned int i;

	for (i = 0; i < session->internals.rexts_size; i++) {
		_gnutls_ext_unset_session_data(session, session->internals.rexts[i].type);
		_gnutls_ext_unset_resumed_session_data(session,
						       session->internals.rexts[i].type);
	}

	for (i = 0; extfunc[i] != NULL; i++) {
		_gnutls_ext_unset_session_data(session, extfunc[i]->type);
		_gnutls_ext_unset_resumed_session_data(session,
						       extfunc[i]->type);
	}

}

/* This function allows an extension to store data in the current session
 * and retrieve them later on. We use functions instead of a pointer to a
 * private pointer, to allow API additions by individual extensions.
 */
void
_gnutls_ext_set_session_data(gnutls_session_t session, uint16_t type,
			     gnutls_ext_priv_data_t data)
{
	unsigned int i;
	gnutls_ext_deinit_data_func deinit;

	deinit = _gnutls_ext_func_deinit(session, type);

	for (i = 0; i < MAX_EXT_TYPES; i++) {
		if (session->internals.extension_int_data[i].type == type
		    || session->internals.extension_int_data[i].set == 0) {
			if (session->internals.extension_int_data[i].set !=
			    0) {
				if (deinit)
					deinit(session->internals.
					       extension_int_data[i].priv);
			}
			session->internals.extension_int_data[i].type =
			    type;
			session->internals.extension_int_data[i].priv =
			    data;
			session->internals.extension_int_data[i].set = 1;
			return;
		}
	}
}

int
_gnutls_ext_get_session_data(gnutls_session_t session,
			     uint16_t type, gnutls_ext_priv_data_t * data)
{
	int i;

	for (i = 0; i < MAX_EXT_TYPES; i++) {
		if (session->internals.extension_int_data[i].set != 0 &&
		    session->internals.extension_int_data[i].type == type)
		{
			*data =
			    session->internals.extension_int_data[i].priv;
			return 0;
		}
	}
	return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
}

int
_gnutls_ext_get_resumed_session_data(gnutls_session_t session,
				     uint16_t type,
				     gnutls_ext_priv_data_t * data)
{
	int i;

	for (i = 0; i < MAX_EXT_TYPES; i++) {
		if (session->internals.resumed_extension_int_data[i].set !=
		    0
		    && session->internals.resumed_extension_int_data[i].
		    type == type) {
			*data =
			    session->internals.
			    resumed_extension_int_data[i].priv;
			return 0;
		}
	}
	return GNUTLS_E_INVALID_REQUEST;
}

/**
 * gnutls_ext_register:
 * @name: the name of the extension to register
 * @type: the numeric id of the extension
 * @parse_type: the parse type of the extension (see gnutls_ext_parse_type_t)
 * @recv_func: a function to receive the data
 * @send_func: a function to send the data
 * @deinit_func: a function deinitialize any private data
 * @pack_func: a function which serializes the extension's private data (used on session packing for resumption)
 * @unpack_func: a function which will deserialize the extension's private data
 *
 * This function will register a new extension type. The extension will remain
 * registered until gnutls_global_deinit() is called. If the extension type
 * is already registered then %GNUTLS_E_ALREADY_REGISTERED will be returned.
 *
 * Each registered extension can store temporary data into the gnutls_session_t
 * structure using gnutls_ext_set_data(), and they can be retrieved using
 * gnutls_ext_get_data().
 *
 * This function is not thread safe.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, otherwise a negative error code.
 *
 * Since: 3.4.0
 **/
int 
gnutls_ext_register(const char *name, int type, gnutls_ext_parse_type_t parse_type,
		    gnutls_ext_recv_func recv_func, gnutls_ext_send_func send_func, 
		    gnutls_ext_deinit_data_func deinit_func, gnutls_ext_pack_func pack_func,
		    gnutls_ext_unpack_func unpack_func)
{
	extension_entry_st *tmp_mod;
	int ret;
	unsigned i;

	for (i = 0; extfunc[i] != NULL; i++) {
		if (extfunc[i]->type == type)
			return gnutls_assert_val(GNUTLS_E_ALREADY_REGISTERED);
	}

	tmp_mod = gnutls_calloc(1, sizeof(*tmp_mod));
	if (tmp_mod == NULL)
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

	tmp_mod->name = gnutls_strdup(name);
	tmp_mod->free_struct = 1;
	tmp_mod->type = type;
	tmp_mod->parse_type = parse_type;
	tmp_mod->recv_func = recv_func;
	tmp_mod->send_func = send_func;
	tmp_mod->deinit_func = deinit_func;
	tmp_mod->pack_func = pack_func;
	tmp_mod->unpack_func = unpack_func;

	ret = ext_register(tmp_mod);
	if (ret < 0) {
		gnutls_free((void*)tmp_mod->name);
		gnutls_free(tmp_mod);
	}
	return ret;
}

/**
 * gnutls_session_ext_register:
 * @session: the session for which this extension will be set
 * @name: the name of the extension to register
 * @type: the numeric id of the extension
 * @parse_type: the parse type of the extension (see gnutls_ext_parse_type_t)
 * @recv_func: a function to receive the data
 * @send_func: a function to send the data
 * @deinit_func: a function deinitialize any private data
 * @pack_func: a function which serializes the extension's private data (used on session packing for resumption)
 * @unpack_func: a function which will deserialize the extension's private data
 * @flags: must be zero or flags from %gnutls_ext_flags_t
 *
 * This function will register a new extension type. The extension will be
 * only usable within the registered session. If the extension type
 * is already registered then %GNUTLS_E_ALREADY_REGISTERED will be returned,
 * unless the flag %GNUTLS_EXT_FLAG_OVERRIDE_INTERNAL is specified. The latter
 * flag when specified can be used to override certain extensions introduced
 * after 3.5.12. It is expected to be used by applications which handle
 * custom extensions that are not currently supported in GnuTLS, but direct
 * support for them may be added in the future.
 *
 * Each registered extension can store temporary data into the gnutls_session_t
 * structure using gnutls_ext_set_data(), and they can be retrieved using
 * gnutls_ext_get_data().
 *
 * Returns: %GNUTLS_E_SUCCESS on success, otherwise a negative error code.
 *
 * Since: 3.5.5
 **/
int 
gnutls_session_ext_register(gnutls_session_t session,
			    const char *name, int type, gnutls_ext_parse_type_t parse_type,
			    gnutls_ext_recv_func recv_func, gnutls_ext_send_func send_func, 
			    gnutls_ext_deinit_data_func deinit_func, gnutls_ext_pack_func pack_func,
			    gnutls_ext_unpack_func unpack_func, unsigned flags)
{
	extension_entry_st tmp_mod;
	extension_entry_st *exts;
	unsigned i;

	/* FIXME: handle GNUTLS_EXT_FLAG_OVERRIDE_INTERNAL for new exts */
	for (i = 0; extfunc[i] != NULL; i++) {
		if (extfunc[i]->type == type)
			return gnutls_assert_val(GNUTLS_E_ALREADY_REGISTERED);
	}

	tmp_mod.name = NULL;
	tmp_mod.free_struct = 1;
	tmp_mod.type = type;
	tmp_mod.parse_type = parse_type;
	tmp_mod.recv_func = recv_func;
	tmp_mod.send_func = send_func;
	tmp_mod.deinit_func = deinit_func;
	tmp_mod.pack_func = pack_func;
	tmp_mod.unpack_func = unpack_func;

	exts = gnutls_realloc(session->internals.rexts, (session->internals.rexts_size+1)*sizeof(*exts));
	if (exts == NULL) {
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);
	}

	session->internals.rexts = exts;

	memcpy(&session->internals.rexts[session->internals.rexts_size], &tmp_mod, sizeof(extension_entry_st));
	session->internals.rexts_size++;

	return 0;
}

/**
 * gnutls_ext_set_data:
 * @session: a #gnutls_session_t opaque pointer
 * @type: the numeric id of the extension
 * @data: the private data to set
 *
 * This function allows an extension handler to store data in the current session
 * and retrieve them later on. The set data will be deallocated using
 * the gnutls_ext_deinit_data_func.
 *
 * Since: 3.4.0
 **/
void
gnutls_ext_set_data(gnutls_session_t session, unsigned type,
		    gnutls_ext_priv_data_t data)
{
	_gnutls_ext_set_session_data(session, type, data);
}

/**
 * gnutls_ext_get_data:
 * @session: a #gnutls_session_t opaque pointer
 * @type: the numeric id of the extension
 * @data: a pointer to the private data to retrieve
 *
 * This function retrieves any data previously stored with gnutls_ext_set_data().
 *
 * Returns: %GNUTLS_E_SUCCESS on success, otherwise a negative error code.
 *
 * Since: 3.4.0
 **/
int
gnutls_ext_get_data(gnutls_session_t session,
		    unsigned type, gnutls_ext_priv_data_t *data)
{
	return _gnutls_ext_get_session_data(session, type, data);
}
