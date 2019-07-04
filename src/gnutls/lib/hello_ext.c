/*
 * Copyright (C) 2001-2018 Free Software Foundation, Inc.
 * Copyright (C) 2015-2018 Red Hat, Inc.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>
 *
 */

/* Functions that relate to the TLS hello extension parsing.
 * Hello extensions are packets appended in the TLS hello packet, and
 * allow for extra functionality.
 */

#include "gnutls_int.h"
#include "hello_ext.h"
#include "errors.h"
#include "ext/max_record.h"
#include <ext/server_name.h>
#include <ext/srp.h>
#include <ext/heartbeat.h>
#include <ext/session_ticket.h>
#include <ext/safe_renegotiation.h>
#include <ext/signature.h>
#include <ext/safe_renegotiation.h>
#include "ext/supported_groups.h"
#include "ext/ec_point_formats.h"
#include <ext/status_request.h>
#include <ext/ext_master_secret.h>
#include <ext/supported_versions.h>
#include <ext/post_handshake.h>
#include <ext/srtp.h>
#include <ext/alpn.h>
#include <ext/dumbfw.h>
#include <ext/key_share.h>
#include <ext/pre_shared_key.h>
#include <ext/psk_ke_modes.h>
#include <ext/etm.h>
#include <ext/cookie.h>
#include <ext/early_data.h>
#include <ext/record_size_limit.h>
#include "extv.h"
#include <num.h>
#include <ext/client_cert_type.h>
#include <ext/server_cert_type.h>

static void
unset_ext_data(gnutls_session_t session, const struct hello_ext_entry_st *, unsigned idx);

static void unset_resumed_ext_data(gnutls_session_t session, const struct hello_ext_entry_st *, unsigned idx);

static hello_ext_entry_st const *extfunc[MAX_EXT_TYPES+1] = {
	[GNUTLS_EXTENSION_EXT_MASTER_SECRET] = &ext_mod_ext_master_secret,
	[GNUTLS_EXTENSION_SUPPORTED_VERSIONS] = &ext_mod_supported_versions,
	[GNUTLS_EXTENSION_POST_HANDSHAKE] = &ext_mod_post_handshake,
	[GNUTLS_EXTENSION_ETM] = &ext_mod_etm,
#ifdef ENABLE_OCSP
	[GNUTLS_EXTENSION_STATUS_REQUEST] = &ext_mod_status_request,
#endif
	[GNUTLS_EXTENSION_SERVER_NAME] = &ext_mod_server_name,
	[GNUTLS_EXTENSION_SAFE_RENEGOTIATION] = &ext_mod_sr,
#ifdef ENABLE_SRP
	[GNUTLS_EXTENSION_SRP] = &ext_mod_srp,
#endif
#ifdef ENABLE_HEARTBEAT
	[GNUTLS_EXTENSION_HEARTBEAT] = &ext_mod_heartbeat,
#endif
	[GNUTLS_EXTENSION_SESSION_TICKET] = &ext_mod_session_ticket,
	[GNUTLS_EXTENSION_CLIENT_CERT_TYPE] = &ext_mod_client_cert_type,
	[GNUTLS_EXTENSION_SERVER_CERT_TYPE] = &ext_mod_server_cert_type,
	[GNUTLS_EXTENSION_SUPPORTED_GROUPS] = &ext_mod_supported_groups,
	[GNUTLS_EXTENSION_SUPPORTED_EC_POINT_FORMATS] = &ext_mod_supported_ec_point_formats,
	[GNUTLS_EXTENSION_SIGNATURE_ALGORITHMS] = &ext_mod_sig,
	[GNUTLS_EXTENSION_KEY_SHARE] = &ext_mod_key_share,
	[GNUTLS_EXTENSION_COOKIE] = &ext_mod_cookie,
	[GNUTLS_EXTENSION_EARLY_DATA] = &ext_mod_early_data,
#ifdef ENABLE_DTLS_SRTP
	[GNUTLS_EXTENSION_SRTP] = &ext_mod_srtp,
#endif
#ifdef ENABLE_ALPN
	[GNUTLS_EXTENSION_ALPN] = &ext_mod_alpn,
#endif
	[GNUTLS_EXTENSION_RECORD_SIZE_LIMIT] = &ext_mod_record_size_limit,
	[GNUTLS_EXTENSION_MAX_RECORD_SIZE] = &ext_mod_max_record_size,
	[GNUTLS_EXTENSION_PSK_KE_MODES] = &ext_mod_psk_ke_modes,
	[GNUTLS_EXTENSION_PRE_SHARED_KEY] = &ext_mod_pre_shared_key,
	/* This must be the last extension registered.
	 */
	[GNUTLS_EXTENSION_DUMBFW] = &ext_mod_dumbfw,
};

static const hello_ext_entry_st *
gid_to_ext_entry(gnutls_session_t session, extensions_t id)
{
	unsigned i;

	assert(id < MAX_EXT_TYPES);

	for (i=0;i<session->internals.rexts_size;i++) {
		if (session->internals.rexts[i].gid == id) {
			return &session->internals.rexts[i];
		}
	}

	return extfunc[id];
}

static const hello_ext_entry_st *
tls_id_to_ext_entry(gnutls_session_t session, uint16_t tls_id, gnutls_ext_parse_type_t parse_type)
{
	unsigned i;
	const hello_ext_entry_st *e;

	for (i=0;i<session->internals.rexts_size;i++) {
		if (session->internals.rexts[i].tls_id == tls_id) {
			e = &session->internals.rexts[i];
			goto done;
		}
	}

	for (i = 0; i < MAX_EXT_TYPES; i++) {
		if (!extfunc[i])
			continue;

		if (extfunc[i]->tls_id == tls_id) {
			e = extfunc[i];
			goto done;
		}
	}

	return NULL;
done:
	if (parse_type == GNUTLS_EXT_ANY || e->parse_type == parse_type) {
		return e;
	} else {
		return NULL;
	}
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

	for (i = 0; i < MAX_EXT_TYPES; i++) {
		if (!extfunc[i])
			continue;

		if (extfunc[i]->tls_id == ext)
			return extfunc[i]->name;
	}

	return NULL;
}

/* Returns %GNUTLS_EXTENSION_INVALID on error
 */
static unsigned tls_id_to_gid(gnutls_session_t session, unsigned tls_id)
{
	unsigned i;

	for (i=0; i < session->internals.rexts_size; i++) {
		if (session->internals.rexts[i].tls_id == tls_id)
			return session->internals.rexts[i].gid;
	}

	for (i = 0; i < MAX_EXT_TYPES; i++) {
		if (!extfunc[i])
			continue;

		if (extfunc[i]->tls_id == tls_id)
			return extfunc[i]->gid;
	}

	return GNUTLS_EXTENSION_INVALID;
}

typedef struct hello_ext_ctx_st {
	gnutls_session_t session;
	gnutls_ext_flags_t msg;
	gnutls_ext_parse_type_t parse_type;
	const hello_ext_entry_st *ext; /* used during send */
	unsigned seen_pre_shared_key;
} hello_ext_ctx_st;

static
int hello_ext_parse(void *_ctx, unsigned tls_id, const uint8_t *data, unsigned data_size)
{
	hello_ext_ctx_st *ctx = _ctx;
	gnutls_session_t session = ctx->session;
	const hello_ext_entry_st *ext;
	int ret;

	if (tls_id == PRE_SHARED_KEY_TLS_ID) {
		ctx->seen_pre_shared_key = 1;
	} else if (ctx->seen_pre_shared_key && session->security_parameters.entity == GNUTLS_SERVER) {
		/* the pre-shared key extension must always be the last one,
		 * draft-ietf-tls-tls13-28: 4.2.11 */
		return gnutls_assert_val(GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER);
	}

	ext = tls_id_to_ext_entry(session, tls_id, ctx->parse_type);
	if (ext == NULL || ext->recv_func == NULL) {
		goto ignore;
	}

	/* we do not hard fail when extensions defined for TLS are used for
	 * DTLS and vice-versa. They may extend their role in the future. */
	if (IS_DTLS(session)) {
		if (!(ext->validity & GNUTLS_EXT_FLAG_DTLS)) {
			gnutls_assert();
			goto ignore;
		}
	} else {
		if (!(ext->validity & GNUTLS_EXT_FLAG_TLS)) {
			gnutls_assert();
			goto ignore;
		}
	}

	if (session->security_parameters.entity == GNUTLS_CLIENT) {
		if (!(ext->validity & GNUTLS_EXT_FLAG_IGNORE_CLIENT_REQUEST) &&
		    !_gnutls_hello_ext_is_present(session, ext->gid)) {
			_gnutls_debug_log("EXT[%p]: Received unexpected extension '%s/%d'\n", session,
					ext->name, (int)tls_id);
			return gnutls_assert_val(GNUTLS_E_RECEIVED_ILLEGAL_EXTENSION);
		}
	}

	if ((ext->validity & ctx->msg) == 0) {
		_gnutls_debug_log("EXT[%p]: Received unexpected extension (%s/%d) for '%s'\n", session,
				  ext->name, (int)tls_id,
				  ext_msg_validity_to_str(ctx->msg));
		return gnutls_assert_val(GNUTLS_E_RECEIVED_ILLEGAL_EXTENSION);
	}

	if (session->security_parameters.entity == GNUTLS_SERVER) {
		ret = _gnutls_hello_ext_save(session, ext->gid, 1);
		if (ret == 0)
			return gnutls_assert_val(GNUTLS_E_RECEIVED_ILLEGAL_EXTENSION);
	}

	_gnutls_handshake_log
	    ("EXT[%p]: Parsing extension '%s/%d' (%d bytes)\n",
	     session, ext->name, (int)tls_id,
	     data_size);

	_gnutls_ext_set_msg(session, ctx->msg);
	if ((ret = ext->recv_func(session, data, data_size)) < 0) {
		gnutls_assert();
		return ret;
	}

	return 0;

 ignore:
	if (ext) {
		_gnutls_handshake_log
		    ("EXT[%p]: Ignoring extension '%s/%d'\n", session,
		     ext->name, (int)tls_id);
	}
	return 0;
}

int
_gnutls_parse_hello_extensions(gnutls_session_t session,
			       gnutls_ext_flags_t msg,
			       gnutls_ext_parse_type_t parse_type,
			       const uint8_t * data, int data_size)
{
	int ret;
	hello_ext_ctx_st ctx;

	msg &= GNUTLS_EXT_FLAG_SET_ONLY_FLAGS_MASK;

	ctx.session = session;
	ctx.msg = msg;
	ctx.parse_type = parse_type;
	ctx.seen_pre_shared_key = 0;

	ret = _gnutls_extv_parse(&ctx, hello_ext_parse, data, data_size);
	if (ret < 0)
		return gnutls_assert_val(ret);

	return 0;
}

static
int hello_ext_send(void *_ctx, gnutls_buffer_st *buf)
{
	hello_ext_ctx_st *ctx = _ctx;
	int ret;
	const hello_ext_entry_st *p = ctx->ext;
	gnutls_session_t session = ctx->session;
	int appended;
	size_t size_prev;

	if (unlikely(p->send_func == NULL))
		return 0;

	if (ctx->parse_type != GNUTLS_EXT_ANY
	    && p->parse_type != ctx->parse_type) {
		return 0;
	}

	if (IS_DTLS(session)) {
		if (!(p->validity & GNUTLS_EXT_FLAG_DTLS)) {
			gnutls_assert();
			goto skip;
		}
	} else {
		if (!(p->validity & GNUTLS_EXT_FLAG_TLS)) {
			gnutls_assert();
			goto skip;
		}
	}

	if ((ctx->msg & p->validity) == 0) {
		goto skip;
	} else {
		_gnutls_handshake_log("EXT[%p]: Preparing extension (%s/%d) for '%s'\n", session,
				  p->name, (int)p->tls_id,
				  ext_msg_validity_to_str(ctx->msg));
	}

	/* ensure we don't send something twice (i.e, overridden extensions in
	 * client), and ensure we are sending only what we received in server. */
	ret = _gnutls_hello_ext_is_present(session, p->gid);

	if (session->security_parameters.entity == GNUTLS_SERVER) {
		/* if client didn't advertise and the override flag is not present */
		if (!(p->validity & GNUTLS_EXT_FLAG_IGNORE_CLIENT_REQUEST) && ret == 0)
			return 0;
	} else {
		if (ret != 0) /* already sent */
			return 0;
	}


	size_prev = buf->length;

	_gnutls_ext_set_msg(session, ctx->msg);
	ret = p->send_func(session, buf);
	if (ret < 0 && ret != GNUTLS_E_INT_RET_0) {
		return gnutls_assert_val(ret);
	}

	appended = buf->length - size_prev;

	/* add this extension to the extension list, to know which extensions
	 * to expect.
	 */
	if ((appended > 0 || ret == GNUTLS_E_INT_RET_0) &&
	    session->security_parameters.entity == GNUTLS_CLIENT) {

		_gnutls_hello_ext_save(session, p->gid, 0);
	}

	return ret;

 skip:
	_gnutls_handshake_log("EXT[%p]: Not sending extension (%s/%d) for '%s'\n", session,
			  p->name, (int)p->tls_id,
			  ext_msg_validity_to_str(ctx->msg));
	return 0;
}

int
_gnutls_gen_hello_extensions(gnutls_session_t session,
			     gnutls_buffer_st * buf,
			     gnutls_ext_flags_t msg,
			     gnutls_ext_parse_type_t parse_type)
{
	int pos, ret;
	size_t i;
	hello_ext_ctx_st ctx;

	msg &= GNUTLS_EXT_FLAG_SET_ONLY_FLAGS_MASK;

	ctx.session = session;
	ctx.msg = msg;
	ctx.parse_type = parse_type;

	ret = _gnutls_extv_append_init(buf);
	if (ret < 0)
		return gnutls_assert_val(ret);

	pos = ret;
	_gnutls_ext_set_extensions_offset(session, pos);

	for (i=0; i < session->internals.rexts_size; i++) {
		ctx.ext = &session->internals.rexts[i];
		ret = _gnutls_extv_append(buf, session->internals.rexts[i].tls_id,
					  &ctx, hello_ext_send);
		if (ret < 0)
			return gnutls_assert_val(ret);

		if (ret > 0)
			_gnutls_handshake_log
				    ("EXT[%p]: Sending extension %s/%d (%d bytes)\n",
				     session, ctx.ext->name, (int)ctx.ext->tls_id, ret-4);
	}

	/* hello_ext_send() ensures we don't send duplicates, in case
	 * of overridden extensions */
	for (i = 0; i < MAX_EXT_TYPES; i++) {
		if (!extfunc[i])
			continue;

		ctx.ext = extfunc[i];
		ret = _gnutls_extv_append(buf, extfunc[i]->tls_id,
					  &ctx, hello_ext_send);
		if (ret < 0)
			return gnutls_assert_val(ret);

		if (ret > 0)
			_gnutls_handshake_log
				    ("EXT[%p]: Sending extension %s/%d (%d bytes)\n",
				     session, ctx.ext->name, (int)ctx.ext->tls_id, ret-4);
	}

	ret = _gnutls_extv_append_final(buf, pos, !(msg & GNUTLS_EXT_FLAG_EE));
	if (ret < 0)
		return gnutls_assert_val(ret);

	return 0;
}

/* Global deinit and init of global extensions */
int _gnutls_hello_ext_init(void)
{
	return GNUTLS_E_SUCCESS;
}

void _gnutls_hello_ext_deinit(void)
{
	unsigned i;

	for (i = 0; i < MAX_EXT_TYPES; i++) {
		if (!extfunc[i])
			continue;

		if (extfunc[i]->free_struct != 0) {
			gnutls_free(((hello_ext_entry_st *)extfunc[i])->name);
			gnutls_free(extfunc[i]);
		}
	}
}

/* Packing of extension data (for use in resumption) */
static int pack_extension(gnutls_session_t session, const hello_ext_entry_st *extp,
			  gnutls_buffer_st *packed)
{
	int ret;
	int size_offset;
	int cur_size;
	gnutls_ext_priv_data_t data;
	int rval = 0;

	ret =
	    _gnutls_hello_ext_get_priv(session, extp->gid,
					 &data);
	if (ret >= 0 && extp->pack_func != NULL) {
		BUFFER_APPEND_NUM(packed, extp->gid);

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

int _gnutls_hello_ext_pack(gnutls_session_t session, gnutls_buffer_st *packed)
{
	unsigned int i;
	int ret;
	int total_exts_pos;
	int n_exts = 0;
	const struct hello_ext_entry_st *ext;

	total_exts_pos = packed->length;
	BUFFER_APPEND_NUM(packed, 0);

	for (i = 0; i <= GNUTLS_EXTENSION_MAX_VALUE; i++) {
		if (session->internals.used_exts & (1<<i)) {

			ext = gid_to_ext_entry(session, i);
			if (ext == NULL)
				continue;

			ret = pack_extension(session, ext, packed);
			if (ret < 0)
				return gnutls_assert_val(ret);

			if (ret > 0)
				n_exts++;
		}
	}

	_gnutls_write_uint32(n_exts, packed->data + total_exts_pos);

	return 0;
}

int _gnutls_ext_set_full_client_hello(gnutls_session_t session,
				      handshake_buffer_st *recv_buf)
{
	int ret;
	gnutls_buffer_st *buf = &session->internals.full_client_hello;

	_gnutls_buffer_clear(buf);

	if ((ret = _gnutls_buffer_append_prefix(buf, 8, recv_buf->htype)) < 0)
		return gnutls_assert_val(ret);
	if ((ret = _gnutls_buffer_append_prefix(buf, 24, recv_buf->data.length)) < 0)
		return gnutls_assert_val(ret);
	if ((ret = _gnutls_buffer_append_data(buf, recv_buf->data.data, recv_buf->data.length)) < 0)
		return gnutls_assert_val(ret);

	return 0;
}

unsigned _gnutls_ext_get_full_client_hello(gnutls_session_t session,
				           gnutls_datum_t *d)
{
	gnutls_buffer_st *buf = &session->internals.full_client_hello;

	if (!buf->length)
		return 0;

	d->data = buf->data;
	d->size = buf->length;

	return 1;
}

static void
_gnutls_ext_set_resumed_session_data(gnutls_session_t session,
				     extensions_t id,
				     gnutls_ext_priv_data_t data)
{
	const struct hello_ext_entry_st *ext;

	/* If this happens we need to increase the max */
	assert(id < MAX_EXT_TYPES);

	ext = gid_to_ext_entry(session, id);
	assert(ext != NULL);

	if (session->internals.ext_data[id].resumed_set != 0)
		unset_resumed_ext_data(session, ext, id);

	session->internals.ext_data[id].resumed_priv = data;
	session->internals.ext_data[id].resumed_set = 1;
	return;
}

int _gnutls_hello_ext_unpack(gnutls_session_t session, gnutls_buffer_st * packed)
{
	int i, ret;
	gnutls_ext_priv_data_t data;
	int max_exts = 0;
	extensions_t id;
	int size_for_id, cur_pos;
	const struct hello_ext_entry_st *ext;

	BUFFER_POP_NUM(packed, max_exts);
	for (i = 0; i < max_exts; i++) {
		BUFFER_POP_NUM(packed, id);
		BUFFER_POP_NUM(packed, size_for_id);

		cur_pos = packed->length;

		ext = gid_to_ext_entry(session, id);
		if (ext == NULL || ext->unpack_func == NULL) {
			gnutls_assert();
			return GNUTLS_E_PARSING_ERROR;
		}

		ret = ext->unpack_func(packed, &data);
		if (ret < 0) {
			gnutls_assert();
			return ret;
		}

		/* verify that unpack read the correct bytes */
		cur_pos = cur_pos - packed->length;
		if (cur_pos /* read length */  != size_for_id) {
			gnutls_assert();
			return GNUTLS_E_PARSING_ERROR;
		}

		_gnutls_ext_set_resumed_session_data(session, id, data);
	}

	return 0;

      error:
	return ret;
}

static void
unset_ext_data(gnutls_session_t session, const struct hello_ext_entry_st *ext, unsigned idx)
{
	if (session->internals.ext_data[idx].set == 0)
		return;

	if (ext && ext->deinit_func && session->internals.ext_data[idx].priv != NULL)
		ext->deinit_func(session->internals.ext_data[idx].priv);
	session->internals.ext_data[idx].set = 0;
}

void
_gnutls_hello_ext_unset_priv(gnutls_session_t session,
			      extensions_t id)
{
	const struct hello_ext_entry_st *ext;

	ext = gid_to_ext_entry(session, id);
	if (ext)
		unset_ext_data(session, ext, id);
}

static void unset_resumed_ext_data(gnutls_session_t session, const struct hello_ext_entry_st *ext, unsigned idx)
{
	if (session->internals.ext_data[idx].resumed_set == 0)
		return;

	if (ext && ext->deinit_func && session->internals.ext_data[idx].resumed_priv) {
		ext->deinit_func(session->internals.ext_data[idx].resumed_priv);
	}
	session->internals.ext_data[idx].resumed_set = 0;
}

/* Deinitializes all data that are associated with TLS extensions.
 */
void _gnutls_hello_ext_priv_deinit(gnutls_session_t session)
{
	unsigned int i;
	const struct hello_ext_entry_st *ext;

	for (i = 0; i < MAX_EXT_TYPES; i++) {
		if (!session->internals.ext_data[i].set && !session->internals.ext_data[i].resumed_set)
			continue;

		ext = gid_to_ext_entry(session, i);
		if (ext) {
			unset_ext_data(session, ext, i);
			unset_resumed_ext_data(session, ext, i);
		}
	}
}

/* This function allows an extension to store data in the current session
 * and retrieve them later on. We use functions instead of a pointer to a
 * private pointer, to allow API additions by individual extensions.
 */
void
_gnutls_hello_ext_set_priv(gnutls_session_t session, extensions_t id,
			     gnutls_ext_priv_data_t data)
{
	const struct hello_ext_entry_st *ext;

	assert(id < MAX_EXT_TYPES);

	ext = gid_to_ext_entry(session, id);
	assert(ext != NULL);

	if (session->internals.ext_data[id].set != 0) {
		unset_ext_data(session, ext, id);
	}
	session->internals.ext_data[id].priv = data;
	session->internals.ext_data[id].set = 1;

	return;
}

int
_gnutls_hello_ext_get_priv(gnutls_session_t session,
			    extensions_t id, gnutls_ext_priv_data_t * data)
{
	if (session->internals.ext_data[id].set != 0) {
		*data =
		    session->internals.ext_data[id].priv;
		return 0;
	}

	return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
}

int
_gnutls_hello_ext_get_resumed_priv(gnutls_session_t session,
				    extensions_t id,
				    gnutls_ext_priv_data_t * data)
{
	if (session->internals.ext_data[id].resumed_set != 0) {
		*data =
		    session->internals.ext_data[id].resumed_priv;
		return 0;
	}

	return GNUTLS_E_INVALID_REQUEST;
}

/**
 * gnutls_ext_register:
 * @name: the name of the extension to register
 * @id: the numeric TLS id of the extension
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
 * Any extensions registered with this function are valid for the client
 * and TLS1.2 server hello (or encrypted extensions for TLS1.3).
 *
 * This function is not thread safe.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, otherwise a negative error code.
 *
 * Since: 3.4.0
 **/
int
gnutls_ext_register(const char *name, int id, gnutls_ext_parse_type_t parse_type,
		    gnutls_ext_recv_func recv_func, gnutls_ext_send_func send_func,
		    gnutls_ext_deinit_data_func deinit_func, gnutls_ext_pack_func pack_func,
		    gnutls_ext_unpack_func unpack_func)
{
	hello_ext_entry_st *tmp_mod;
	unsigned i;
	unsigned gid = GNUTLS_EXTENSION_MAX+1;

	for (i = 0; i < MAX_EXT_TYPES; i++) {
		if (!extfunc[i])
			continue;

		if (extfunc[i]->tls_id == id)
			return gnutls_assert_val(GNUTLS_E_ALREADY_REGISTERED);

		if (extfunc[i]->gid >= gid)
			gid = extfunc[i]->gid + 1;
	}

	if (gid > GNUTLS_EXTENSION_MAX_VALUE || gid >= sizeof(extfunc)/sizeof(extfunc[0]))
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

	tmp_mod = gnutls_calloc(1, sizeof(*tmp_mod));
	if (tmp_mod == NULL)
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

	tmp_mod->name = gnutls_strdup(name);
	tmp_mod->free_struct = 1;
	tmp_mod->tls_id = id;
	tmp_mod->gid = gid;
	tmp_mod->parse_type = parse_type;
	tmp_mod->recv_func = recv_func;
	tmp_mod->send_func = send_func;
	tmp_mod->deinit_func = deinit_func;
	tmp_mod->pack_func = pack_func;
	tmp_mod->unpack_func = unpack_func;
	tmp_mod->validity = GNUTLS_EXT_FLAG_CLIENT_HELLO | GNUTLS_EXT_FLAG_TLS12_SERVER_HELLO |
			    GNUTLS_EXT_FLAG_EE | GNUTLS_EXT_FLAG_DTLS | GNUTLS_EXT_FLAG_TLS;

	assert(extfunc[gid] == NULL);
	extfunc[gid] = tmp_mod;

	return 0;
}

#define VALIDITY_MASK (GNUTLS_EXT_FLAG_CLIENT_HELLO | GNUTLS_EXT_FLAG_TLS12_SERVER_HELLO | \
			GNUTLS_EXT_FLAG_TLS13_SERVER_HELLO | \
			GNUTLS_EXT_FLAG_EE | GNUTLS_EXT_FLAG_HRR)

/**
 * gnutls_session_ext_register:
 * @session: the session for which this extension will be set
 * @name: the name of the extension to register
 * @id: the numeric id of the extension
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
 * after 3.6.0. It is expected to be used by applications which handle
 * custom extensions that are not currently supported in GnuTLS, but direct
 * support for them may be added in the future.
 *
 * Each registered extension can store temporary data into the gnutls_session_t
 * structure using gnutls_ext_set_data(), and they can be retrieved using
 * gnutls_ext_get_data().
 *
 * The validity of the extension registered can be given by the appropriate flags
 * of %gnutls_ext_flags_t. If no validity is given, then the registered extension
 * will be valid for client and TLS1.2 server hello (or encrypted extensions for TLS1.3).
 *
 * Returns: %GNUTLS_E_SUCCESS on success, otherwise a negative error code.
 *
 * Since: 3.5.5
 **/
int
gnutls_session_ext_register(gnutls_session_t session,
			    const char *name, int id, gnutls_ext_parse_type_t parse_type,
			    gnutls_ext_recv_func recv_func, gnutls_ext_send_func send_func,
			    gnutls_ext_deinit_data_func deinit_func, gnutls_ext_pack_func pack_func,
			    gnutls_ext_unpack_func unpack_func, unsigned flags)
{
	hello_ext_entry_st tmp_mod;
	hello_ext_entry_st *exts;
	unsigned i;
	unsigned gid = GNUTLS_EXTENSION_MAX+1;

	/* reject handling any extensions which modify the TLS handshake
	 * in any way, or are mapped to an exported API. */
	for (i = 0; i < GNUTLS_EXTENSION_MAX; i++) {
		if (!extfunc[i])
			continue;

		if (extfunc[i]->tls_id == id) {
			if (!(flags & GNUTLS_EXT_FLAG_OVERRIDE_INTERNAL)) {
				return gnutls_assert_val(GNUTLS_E_ALREADY_REGISTERED);
			} else if (extfunc[i]->cannot_be_overriden) {
				return gnutls_assert_val(GNUTLS_E_ALREADY_REGISTERED);
			}
			break;
		}

		if (extfunc[i]->gid >= gid)
			gid = extfunc[i]->gid + 1;
	}

	for (i=0;i<session->internals.rexts_size;i++) {
		if (session->internals.rexts[i].tls_id == id) {
			return gnutls_assert_val(GNUTLS_E_ALREADY_REGISTERED);
		}

		if (session->internals.rexts[i].gid >= gid)
			gid = session->internals.rexts[i].gid + 1;
	}

	if (gid > GNUTLS_EXTENSION_MAX_VALUE)
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

	memset(&tmp_mod, 0, sizeof(hello_ext_entry_st));
	tmp_mod.free_struct = 1;
	tmp_mod.tls_id = id;
	tmp_mod.gid = gid;
	tmp_mod.parse_type = parse_type;
	tmp_mod.recv_func = recv_func;
	tmp_mod.send_func = send_func;
	tmp_mod.deinit_func = deinit_func;
	tmp_mod.pack_func = pack_func;
	tmp_mod.unpack_func = unpack_func;
	tmp_mod.validity = flags;

	if ((tmp_mod.validity & VALIDITY_MASK) == 0) {
		tmp_mod.validity = GNUTLS_EXT_FLAG_CLIENT_HELLO | GNUTLS_EXT_FLAG_TLS12_SERVER_HELLO |
				   GNUTLS_EXT_FLAG_EE;
	}

	if ((tmp_mod.validity & (GNUTLS_EXT_FLAG_DTLS | GNUTLS_EXT_FLAG_TLS)) == 0) {
		if (IS_DTLS(session))
			tmp_mod.validity |= GNUTLS_EXT_FLAG_DTLS;
		else
			tmp_mod.validity |= GNUTLS_EXT_FLAG_TLS;
	}

	exts = gnutls_realloc(session->internals.rexts, (session->internals.rexts_size+1)*sizeof(*exts));
	if (exts == NULL) {
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);
	}

	session->internals.rexts = exts;

	memcpy(&session->internals.rexts[session->internals.rexts_size], &tmp_mod, sizeof(hello_ext_entry_st));
	session->internals.rexts_size++;

	return 0;
}

/**
 * gnutls_ext_set_data:
 * @session: a #gnutls_session_t opaque pointer
 * @tls_id: the numeric id of the extension
 * @data: the private data to set
 *
 * This function allows an extension handler to store data in the current session
 * and retrieve them later on. The set data will be deallocated using
 * the gnutls_ext_deinit_data_func.
 *
 * Since: 3.4.0
 **/
void
gnutls_ext_set_data(gnutls_session_t session, unsigned tls_id,
		    gnutls_ext_priv_data_t data)
{
	unsigned id = tls_id_to_gid(session, tls_id);
	if (id == GNUTLS_EXTENSION_INVALID)
		return;

	_gnutls_hello_ext_set_priv(session, id, data);
}

/**
 * gnutls_ext_get_data:
 * @session: a #gnutls_session_t opaque pointer
 * @tls_id: the numeric id of the extension
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
		    unsigned tls_id, gnutls_ext_priv_data_t *data)
{
	unsigned id = tls_id_to_gid(session, tls_id);
	if (id == GNUTLS_EXTENSION_INVALID)
		return gnutls_assert_val(GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE);

	return _gnutls_hello_ext_get_priv(session, id, data);
}

/**
 * gnutls_ext_get_current_msg:
 * @session: a #gnutls_session_t opaque pointer
 *
 * This function allows an extension handler to obtain the message
 * this extension is being called from. The returned value is a single
 * entry of the %gnutls_ext_flags_t enumeration. That is, if an
 * extension was registered with the %GNUTLS_EXT_FLAG_HRR and
 * %GNUTLS_EXT_FLAG_EE flags, the value when called during parsing of the
 * encrypted extensions message will be %GNUTLS_EXT_FLAG_EE.
 *
 * If not called under an extension handler, its value is undefined.
 *
 * Since: 3.6.3
 **/
unsigned gnutls_ext_get_current_msg(gnutls_session_t session)
{
	return _gnutls_ext_get_msg(session);
}
