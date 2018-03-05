/*
 * Copyright (C) 2012,2013 Free Software Foundation, Inc.
 * Copyright (C) 2013 Nikos Mavrogiannopoulos
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

/* This file implements the TLS heartbeat extension.
 */

#include "errors.h"
#include "gnutls_int.h"
#include <dtls.h>
#include <record.h>
#include <ext/heartbeat.h>
#include <extensions.h>
#include <random.h>

#ifdef ENABLE_HEARTBEAT
/**
  * gnutls_heartbeat_enable:
  * @session: is a #gnutls_session_t type.
  * @type: one of the GNUTLS_HB_* flags
  *
  * If this function is called with the %GNUTLS_HB_PEER_ALLOWED_TO_SEND
  * @type, GnuTLS will allow heartbeat messages to be received. Moreover it also
  * request the peer to accept heartbeat messages.
  *
  * If the @type used is %GNUTLS_HB_LOCAL_ALLOWED_TO_SEND, then the peer
  * will be asked to accept heartbeat messages but not send ones.
  *
  * The function gnutls_heartbeat_allowed() can be used to test Whether
  * locally generated heartbeat messages can be accepted by the peer.
  *
  * Since: 3.1.2
  **/
void gnutls_heartbeat_enable(gnutls_session_t session, unsigned int type)
{
	extension_priv_data_t epriv;

	epriv = (void*)(intptr_t)type;
	_gnutls_ext_set_session_data(session, GNUTLS_EXTENSION_HEARTBEAT,
				     epriv);
}

/**
  * gnutls_heartbeat_allowed:
  * @session: is a #gnutls_session_t type.
  * @type: one of %GNUTLS_HB_LOCAL_ALLOWED_TO_SEND and %GNUTLS_HB_PEER_ALLOWED_TO_SEND
  *
  * This function will check whether heartbeats are allowed
  * to be sent or received in this session. 
  *
  * Returns: Non zero if heartbeats are allowed.
  *
  * Since: 3.1.2
  **/
int gnutls_heartbeat_allowed(gnutls_session_t session, unsigned int type)
{
	extension_priv_data_t epriv;

	if (session->internals.handshake_in_progress != 0)
		return 0; /* not allowed */

	if (_gnutls_ext_get_session_data
	    (session, GNUTLS_EXTENSION_HEARTBEAT, &epriv) < 0)
		return 0;	/* Not enabled */

	if (type == GNUTLS_HB_LOCAL_ALLOWED_TO_SEND) {
		if (((intptr_t)epriv) & LOCAL_ALLOWED_TO_SEND)
			return 1;
	} else if (((intptr_t)epriv) & GNUTLS_HB_PEER_ALLOWED_TO_SEND)
		return 1;

	return 0;
}

#define DEFAULT_PADDING_SIZE 16

/*
 * Sends heartbeat data.
 */
static int
heartbeat_send_data(gnutls_session_t session, const void *data,
		    size_t data_size, uint8_t type)
{
	int ret, pos;
	uint8_t *response;

	response = gnutls_malloc(1 + 2 + data_size + DEFAULT_PADDING_SIZE);
	if (response == NULL)
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

	pos = 0;
	response[pos++] = type;

	_gnutls_write_uint16(data_size, &response[pos]);
	pos += 2;

	memcpy(&response[pos], data, data_size);
	pos += data_size;

	ret =
	    gnutls_rnd(GNUTLS_RND_NONCE, &response[pos],
		       DEFAULT_PADDING_SIZE);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}
	pos += DEFAULT_PADDING_SIZE;

	ret =
	    _gnutls_send_int(session, GNUTLS_HEARTBEAT, -1,
			     EPOCH_WRITE_CURRENT, response, pos,
			     MBUFFER_FLUSH);

      cleanup:
	gnutls_free(response);
	return ret;
}

/**
 * gnutls_heartbeat_ping:
 * @session: is a #gnutls_session_t type.
 * @data_size: is the length of the ping payload.
 * @max_tries: if flags is %GNUTLS_HEARTBEAT_WAIT then this sets the number of retransmissions. Use zero for indefinite (until timeout).
 * @flags: if %GNUTLS_HEARTBEAT_WAIT then wait for pong or timeout instead of returning immediately.
 *
 * This function sends a ping to the peer. If the @flags is set
 * to %GNUTLS_HEARTBEAT_WAIT then it waits for a reply from the peer.
 * 
 * Note that it is highly recommended to use this function with the
 * flag %GNUTLS_HEARTBEAT_WAIT, or you need to handle retransmissions
 * and timeouts manually.
 *
 * The total TLS data transmitted as part of the ping message are given by
 * the following formula: MAX(16, @data_size)+gnutls_record_overhead_size()+3.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, otherwise a negative error code.
 *
 * Since: 3.1.2
 **/
int
gnutls_heartbeat_ping(gnutls_session_t session, size_t data_size,
		      unsigned int max_tries, unsigned int flags)
{
	int ret;
	unsigned int retries = 1, diff;
	struct timespec now;

	if (data_size > MAX_HEARTBEAT_LENGTH)
		return
		    gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET_LENGTH);

	if (gnutls_heartbeat_allowed
	    (session, GNUTLS_HB_LOCAL_ALLOWED_TO_SEND) == 0)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	/* resume previous call if interrupted */
	if (session->internals.record_send_buffer.byte_length > 0 &&
	    session->internals.record_send_buffer.head != NULL &&
	    session->internals.record_send_buffer.head->type ==
	    GNUTLS_HEARTBEAT)
		return _gnutls_io_write_flush(session);

	switch (session->internals.hb_state) {
	case SHB_SEND1:
		if (data_size > DEFAULT_PADDING_SIZE)
			data_size -= DEFAULT_PADDING_SIZE;
		else
			data_size = 0;

		_gnutls_buffer_reset(&session->internals.hb_local_data);

		ret =
		    _gnutls_buffer_resize(&session->internals.
					  hb_local_data, data_size);
		if (ret < 0)
			return gnutls_assert_val(ret);

		ret =
		    gnutls_rnd(GNUTLS_RND_NONCE,
				session->internals.hb_local_data.data,
				data_size);
		if (ret < 0)
			return gnutls_assert_val(ret);

		gettime(&session->internals.hb_ping_start);
		session->internals.hb_local_data.length = data_size;
		session->internals.hb_state = SHB_SEND2;

		/* fallthrough */
	case SHB_SEND2:
		session->internals.hb_actual_retrans_timeout_ms =
		    session->internals.hb_retrans_timeout_ms;
	      retry:
		ret =
		    heartbeat_send_data(session,
					session->internals.hb_local_data.
					data,
					session->internals.hb_local_data.
					length, HEARTBEAT_REQUEST);
		if (ret < 0)
			return gnutls_assert_val(ret);

		gettime(&session->internals.hb_ping_sent);

		if (!(flags & GNUTLS_HEARTBEAT_WAIT)) {
			session->internals.hb_state = SHB_SEND1;
			break;
		}

		session->internals.hb_state = SHB_RECV;
		/* fallthrough */

	case SHB_RECV:
		ret =
		    _gnutls_recv_int(session, GNUTLS_HEARTBEAT,
				     NULL, 0, NULL,
				     session->internals.
				     hb_actual_retrans_timeout_ms);
		if (ret == GNUTLS_E_HEARTBEAT_PONG_RECEIVED) {
			session->internals.hb_state = SHB_SEND1;
			break;
		} else if (ret == GNUTLS_E_TIMEDOUT) {
			retries++;
			if (max_tries > 0 && retries > max_tries) {
				session->internals.hb_state = SHB_SEND1;
				return gnutls_assert_val(ret);
			}

			gettime(&now);
			diff =
			    timespec_sub_ms(&now,
					    &session->internals.
					    hb_ping_start);
			if (diff > session->internals.hb_total_timeout_ms) {
				session->internals.hb_state = SHB_SEND1;
				return
				    gnutls_assert_val(GNUTLS_E_TIMEDOUT);
			}

			session->internals.hb_actual_retrans_timeout_ms *=
			    2;
			session->internals.hb_actual_retrans_timeout_ms %=
			    MAX_DTLS_TIMEOUT;

			session->internals.hb_state = SHB_SEND2;
			goto retry;
		} else if (ret < 0) {
			session->internals.hb_state = SHB_SEND1;
			return gnutls_assert_val(ret);
		}
	}

	return 0;
}

/**
 * gnutls_heartbeat_pong:
 * @session: is a #gnutls_session_t type.
 * @flags: should be zero
 *
 * This function replies to a ping by sending a pong to the peer.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, otherwise a negative error code.
 *
 * Since: 3.1.2
 **/
int gnutls_heartbeat_pong(gnutls_session_t session, unsigned int flags)
{
	int ret;

	if (session->internals.record_send_buffer.byte_length > 0 &&
	    session->internals.record_send_buffer.head != NULL &&
	    session->internals.record_send_buffer.head->type ==
	    GNUTLS_HEARTBEAT)
		return _gnutls_io_write_flush(session);

	if (session->internals.hb_remote_data.length == 0)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	ret =
	    heartbeat_send_data(session,
				session->internals.hb_remote_data.data,
				session->internals.hb_remote_data.length,
				HEARTBEAT_RESPONSE);

	_gnutls_buffer_reset(&session->internals.hb_remote_data);

	if (ret < 0)
		return gnutls_assert_val(ret);

	return 0;
}

/*
 * Processes a heartbeat message. 
 */
int _gnutls_heartbeat_handle(gnutls_session_t session, mbuffer_st * bufel)
{
	int ret;
	unsigned type;
	unsigned pos;
	uint8_t *msg = _mbuffer_get_udata_ptr(bufel);
	size_t hb_len, len = _mbuffer_get_udata_size(bufel);

	if (gnutls_heartbeat_allowed
	    (session, GNUTLS_HB_PEER_ALLOWED_TO_SEND) == 0)
		return gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET);

	if (len < 3 + DEFAULT_PADDING_SIZE)
		return
		    gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET_LENGTH);

	pos = 0;
	type = msg[pos++];

	hb_len = _gnutls_read_uint16(&msg[pos]);
	if (hb_len > len - 3 - DEFAULT_PADDING_SIZE)
		return
		    gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET_LENGTH);

	pos += 2;

	switch (type) {
	case HEARTBEAT_REQUEST:
		_gnutls_buffer_reset(&session->internals.hb_remote_data);

		ret =
		    _gnutls_buffer_resize(&session->internals.
					  hb_remote_data, hb_len);
		if (ret < 0)
			return gnutls_assert_val(ret);

		if (hb_len > 0)
			memcpy(session->internals.hb_remote_data.data,
			       &msg[pos], hb_len);
		session->internals.hb_remote_data.length = hb_len;

		return gnutls_assert_val(GNUTLS_E_HEARTBEAT_PING_RECEIVED);

	case HEARTBEAT_RESPONSE:

		if (hb_len != session->internals.hb_local_data.length)
			return
			    gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET);

		if (hb_len > 0 &&
		    memcmp(&msg[pos],
			   session->internals.hb_local_data.data,
			   hb_len) != 0) {
			if (IS_DTLS(session))
				return gnutls_assert_val(GNUTLS_E_AGAIN);	/* ignore it */
			else
				return
				    gnutls_assert_val
				    (GNUTLS_E_UNEXPECTED_PACKET);
		}

		_gnutls_buffer_reset(&session->internals.hb_local_data);

		return gnutls_assert_val(GNUTLS_E_HEARTBEAT_PONG_RECEIVED);
	default:
		_gnutls_record_log
		    ("REC[%p]: HB: received unknown type %u\n", session,
		     type);
		return gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET);
	}
}

/**
 * gnutls_heartbeat_get_timeout:
 * @session: is a #gnutls_session_t type.
 *
 * This function will return the milliseconds remaining
 * for a retransmission of the previously sent ping
 * message. This function is useful when ping is used in
 * non-blocking mode, to estimate when to call gnutls_heartbeat_ping()
 * if no packets have been received.
 *
 * Returns: the remaining time in milliseconds.
 *
 * Since: 3.1.2
 **/
unsigned int gnutls_heartbeat_get_timeout(gnutls_session_t session)
{
	struct timespec now;
	unsigned int diff;

	gettime(&now);
	diff = timespec_sub_ms(&now, &session->internals.hb_ping_sent);
	if (diff >= session->internals.hb_actual_retrans_timeout_ms)
		return 0;
	else
		return session->internals.hb_actual_retrans_timeout_ms -
		    diff;
}

/**
 * gnutls_heartbeat_set_timeouts:
 * @session: is a #gnutls_session_t type.
 * @retrans_timeout: The time at which a retransmission will occur in milliseconds
 * @total_timeout: The time at which the connection will be aborted, in milliseconds.
 *
 * This function will override the timeouts for the DTLS heartbeat
 * protocol. The retransmission timeout is the time after which a
 * message from the peer is not received, the previous request will
 * be retransmitted. The total timeout is the time after which the
 * handshake will be aborted with %GNUTLS_E_TIMEDOUT.
 *
 * Since: 3.1.2
 **/
void gnutls_heartbeat_set_timeouts(gnutls_session_t session,
				   unsigned int retrans_timeout,
				   unsigned int total_timeout)
{
	session->internals.hb_retrans_timeout_ms = retrans_timeout;
	session->internals.hb_total_timeout_ms = total_timeout;
}


static int
_gnutls_heartbeat_recv_params(gnutls_session_t session,
			      const uint8_t * data, size_t _data_size)
{
	unsigned policy;
	extension_priv_data_t epriv;

	if (_gnutls_ext_get_session_data
	    (session, GNUTLS_EXTENSION_HEARTBEAT, &epriv) < 0) {
		if (session->security_parameters.entity == GNUTLS_CLIENT)
			return
			    gnutls_assert_val
			    (GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER);
		return 0;	/* Not enabled */
	}

	if (_data_size == 0)
		return GNUTLS_E_UNEXPECTED_PACKET_LENGTH;

	policy = (intptr_t)epriv;

	if (data[0] == 1)
		policy |= LOCAL_ALLOWED_TO_SEND;
	else if (data[0] == 2)
		policy |= LOCAL_NOT_ALLOWED_TO_SEND;
	else
		return
		    gnutls_assert_val(GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER);

	epriv = (void*)(intptr_t)policy;
	_gnutls_ext_set_session_data(session, GNUTLS_EXTENSION_HEARTBEAT,
				     epriv);

	return 0;
}

static int
_gnutls_heartbeat_send_params(gnutls_session_t session,
			      gnutls_buffer_st * extdata)
{
	extension_priv_data_t epriv;
	uint8_t p;

	if (_gnutls_ext_get_session_data
	    (session, GNUTLS_EXTENSION_HEARTBEAT, &epriv) < 0)
		return 0;	/* nothing to send - not enabled */

	if (((intptr_t)epriv) & GNUTLS_HB_PEER_ALLOWED_TO_SEND)
		p = 1;
	else			/*if (epriv.num & GNUTLS_HB_PEER_NOT_ALLOWED_TO_SEND) */
		p = 2;

	if (_gnutls_buffer_append_data(extdata, &p, 1) < 0)
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

	return 1;
}

static int
_gnutls_heartbeat_pack(extension_priv_data_t epriv, gnutls_buffer_st * ps)
{
	int ret;

	BUFFER_APPEND_NUM(ps, (intptr_t)epriv);

	return 0;

}

static int
_gnutls_heartbeat_unpack(gnutls_buffer_st * ps,
			 extension_priv_data_t * _priv)
{
	extension_priv_data_t epriv;
	int ret;

	BUFFER_POP_CAST_NUM(ps, epriv);

	*_priv = epriv;

	ret = 0;
      error:
	return ret;
}

const extension_entry_st ext_mod_heartbeat = {
	.name = "Heartbeat",
	.type = GNUTLS_EXTENSION_HEARTBEAT,
	.parse_type = GNUTLS_EXT_TLS,

	.recv_func = _gnutls_heartbeat_recv_params,
	.send_func = _gnutls_heartbeat_send_params,
	.pack_func = _gnutls_heartbeat_pack,
	.unpack_func = _gnutls_heartbeat_unpack,
	.deinit_func = NULL
};

#else
void gnutls_heartbeat_enable(gnutls_session_t session, unsigned int type)
{
}

int gnutls_heartbeat_allowed(gnutls_session_t session, unsigned int type)
{
	return 0;
}

int
gnutls_heartbeat_ping(gnutls_session_t session, size_t data_size,
		      unsigned int max_tries, unsigned int flags)
{
	return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);
}

int gnutls_heartbeat_pong(gnutls_session_t session, unsigned int flags)
{
	return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);
}

unsigned int gnutls_heartbeat_get_timeout(gnutls_session_t session)
{
	return 0;
}

void gnutls_heartbeat_set_timeouts(gnutls_session_t session,
				   unsigned int retrans_timeout,
				   unsigned int total_timeout)
{
	return;
}
#endif
