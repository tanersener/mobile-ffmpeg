/*
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
#include "errors.h"
#include "handshake.h"
#include "tls13/key_update.h"
#include "mem.h"
#include "mbuffers.h"
#include "secrets.h"

#define KEY_UPDATES_WINDOW 1000
#define KEY_UPDATES_PER_WINDOW 8

static int update_keys(gnutls_session_t session, hs_stage_t stage)
{
	int ret;

	ret = _tls13_update_secret(session, session->key.proto.tls13.temp_secret,
				   session->key.proto.tls13.temp_secret_size);
	if (ret < 0)
		return gnutls_assert_val(ret);

	_gnutls_epoch_bump(session);
	ret = _gnutls_epoch_dup(session, EPOCH_READ_CURRENT);
	if (ret < 0)
		return gnutls_assert_val(ret);

	/* If we send a key update during early start, only update our
	 * write keys */
	if (session->internals.recv_state == RECV_STATE_EARLY_START) {
		ret = _tls13_write_connection_state_init(session, stage);
	} else {
		ret = _tls13_connection_state_init(session, stage);
	}
	if (ret < 0)
		return gnutls_assert_val(ret);

	return 0;
}

int _gnutls13_recv_key_update(gnutls_session_t session, gnutls_buffer_st *buf)
{
	int ret;
	struct timespec now;

	if (buf->length != 1)
		return gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET_LENGTH);

	gnutls_gettime(&now);

	/* Roll over the counter if the time window has elapsed */
	if (session->internals.key_update_count == 0 ||
	    timespec_sub_ms(&now, &session->internals.last_key_update) >
	    KEY_UPDATES_WINDOW) {
		session->internals.last_key_update = now;
		session->internals.key_update_count = 0;
	}

	if (unlikely(++session->internals.key_update_count >
		     KEY_UPDATES_PER_WINDOW)) {
		_gnutls_debug_log("reached maximum number of key updates per %d milliseconds (%d)\n",
				  KEY_UPDATES_WINDOW,
				  KEY_UPDATES_PER_WINDOW);
		return gnutls_assert_val(GNUTLS_E_TOO_MANY_HANDSHAKE_PACKETS);
	}

	_gnutls_epoch_gc(session);

	_gnutls_handshake_log("HSK[%p]: received TLS 1.3 key update (%u)\n",
			      session, (unsigned)buf->data[0]);

	switch(buf->data[0]) {
	case 0:
		/* peer updated its key, not requested our key update */
		ret = update_keys(session, STAGE_UPD_PEERS);
		if (ret < 0)
			return gnutls_assert_val(ret);

		break;
	case 1:
		if (session->internals.hsk_flags & HSK_KEY_UPDATE_ASKED) {
			/* if we had asked a key update we shouldn't get this
			 * reply */
			return gnutls_assert_val(GNUTLS_E_ILLEGAL_PARAMETER);
		}

		/* peer updated its key, requested our key update */
		ret = update_keys(session, STAGE_UPD_PEERS);
		if (ret < 0)
			return gnutls_assert_val(ret);

		/* we mark that a key update is schedule, and it
		 * will be performed prior to sending the next application
		 * message.
		 */
		if (session->internals.rsend_state == RECORD_SEND_NORMAL)
			session->internals.rsend_state = RECORD_SEND_KEY_UPDATE_1;
		else if (session->internals.rsend_state == RECORD_SEND_CORKED)
			session->internals.rsend_state = RECORD_SEND_CORKED_TO_KU;

		break;
	default:
		return gnutls_assert_val(GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER);
	}

	session->internals.hsk_flags &= ~(unsigned)(HSK_KEY_UPDATE_ASKED);

	return 0;
}

int _gnutls13_send_key_update(gnutls_session_t session, unsigned again, unsigned flags /* GNUTLS_KU_* */)
{
	int ret;
	mbuffer_st *bufel = NULL;
	uint8_t val;

	if (again == 0) {
		if (flags & GNUTLS_KU_PEER) {
			/* mark that we asked a key update to prevent an
			 * infinite ping pong when receiving the reply */
			session->internals.hsk_flags |= HSK_KEY_UPDATE_ASKED;
			val = 0x01;
		} else {
			val = 0x00;
		}

		_gnutls_handshake_log("HSK[%p]: sending key update (%u)\n", session, (unsigned)val);

		bufel = _gnutls_handshake_alloc(session, 1);
		if (bufel == NULL)
			return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

		_mbuffer_set_udata_size(bufel, 0);
		ret = _mbuffer_append_data(bufel, &val, 1);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

	}

	return _gnutls_send_handshake(session, bufel, GNUTLS_HANDSHAKE_KEY_UPDATE);

cleanup:
	_mbuffer_xfree(&bufel);
	return ret;
}

/**
 * gnutls_session_key_update:
 * @session: is a #gnutls_session_t type.
 * @flags: zero of %GNUTLS_KU_PEER
 *
 * This function will update/refresh the session keys when the
 * TLS protocol is 1.3 or better. The peer is notified of the
 * update by sending a message, so this function should be
 * treated similarly to gnutls_record_send() --i.e., it may
 * return %GNUTLS_E_AGAIN or %GNUTLS_E_INTERRUPTED.
 *
 * When this flag %GNUTLS_KU_PEER is specified, this function
 * in addition to updating the local keys, will ask the peer to
 * refresh its keys too.
 *
 * If the negotiated version is not TLS 1.3 or better this
 * function will return %GNUTLS_E_INVALID_REQUEST.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, otherwise a negative error code.
 *
 * Since: 3.6.3
 **/
int gnutls_session_key_update(gnutls_session_t session, unsigned flags)
{
	int ret;
	const version_entry_st *vers = get_version(session);

	if (!vers->tls13_sem)
		return GNUTLS_E_INVALID_REQUEST;

	ret =
	    _gnutls13_send_key_update(session, AGAIN(STATE150), flags);
	STATE = STATE150;

	if (ret < 0) {
		gnutls_assert();
		return ret;
	}
	STATE = STATE0;

	_gnutls_epoch_gc(session);

	/* it was completely sent, update the keys */
	ret = update_keys(session, STAGE_UPD_OURS);
	if (ret < 0)
		return gnutls_assert_val(ret);

	return 0;
}
