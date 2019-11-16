/*
 * Copyright (C) 2000-2018 Free Software Foundation, Inc.
 * Copyright (C) 2012-2018 Nikos Mavrogiannopoulos
 * Copyright (C) 2018 Red Hat, Inc.
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

/* Functions that are record layer specific, are included in this file.
 */

/* allocate this many bytes more when encrypting or decrypting, to
 * compensate for broken backends such as cryptodev.
 */
#define CIPHER_SLACK_SIZE 32

#include "gnutls_int.h"
#include "errors.h"
#include "debug.h"
#include "cipher.h"
#include "buffers.h"
#include "mbuffers.h"
#include "handshake.h"
#include "hash_int.h"
#include "cipher_int.h"
#include "algorithms.h"
#include "db.h"
#include "auth.h"
#include "num.h"
#include "record.h"
#include "datum.h"
#include "constate.h"
#include "tls13/key_update.h"
#include <ext/heartbeat.h>
#include <state.h>
#include <dtls.h>
#include <dh.h>
#include <random.h>
#include <xsize.h>
#include "locks.h"

struct tls_record_st {
	uint16_t header_size;
	uint8_t version[2];
	gnutls_uint64 sequence;	/* DTLS */
	uint16_t length;
	uint16_t packet_size;	/* header_size + length */
	content_type_t type;
	uint16_t epoch;		/* valid in DTLS only */
#ifdef ENABLE_SSL2
	unsigned v2:1;		/* whether an SSLv2 client hello */
#endif
	/* the data */
};

/**
 * gnutls_record_disable_padding:  
 * @session: is a #gnutls_session_t type.
 *
 * Used to disabled padding in TLS 1.0 and above.  Normally you do not
 * need to use this function, but there are buggy clients that
 * complain if a server pads the encrypted data.  This of course will
 * disable protection against statistical attacks on the data.
 *
 * This function is defunct since 3.1.7. Random padding is disabled
 * by default unless requested using gnutls_record_send_range().
 *
 **/
void gnutls_record_disable_padding(gnutls_session_t session)
{
	return;
}

/**
 * gnutls_transport_set_ptr:
 * @session: is a #gnutls_session_t type.
 * @ptr: is the value.
 *
 * Used to set the first argument of the transport function (for push
 * and pull callbacks). In berkeley style sockets this function will set the
 * connection descriptor.
 * 
 **/
void
gnutls_transport_set_ptr(gnutls_session_t session,
			 gnutls_transport_ptr_t ptr)
{
	session->internals.transport_recv_ptr = ptr;
	session->internals.transport_send_ptr = ptr;
}

/**
 * gnutls_transport_set_ptr2:
 * @session: is a #gnutls_session_t type.
 * @recv_ptr: is the value for the pull function
 * @send_ptr: is the value for the push function
 *
 * Used to set the first argument of the transport function (for push
 * and pull callbacks). In berkeley style sockets this function will set the
 * connection descriptor.  With this function you can use two different
 * pointers for receiving and sending.
 **/
void
gnutls_transport_set_ptr2(gnutls_session_t session,
			  gnutls_transport_ptr_t recv_ptr,
			  gnutls_transport_ptr_t send_ptr)
{
	session->internals.transport_send_ptr = send_ptr;
	session->internals.transport_recv_ptr = recv_ptr;
}

/**
 * gnutls_transport_set_int2:
 * @session: is a #gnutls_session_t type.
 * @recv_fd: is socket descriptor for the pull function
 * @send_fd: is socket descriptor for the push function
 *
 * This function sets the first argument of the transport functions,
 * such as send() and recv() for the default callbacks using the
 * system's socket API. With this function you can set two different
 * descriptors for receiving and sending.
 *
 * This function is equivalent to calling gnutls_transport_set_ptr2()
 * with the descriptors, but requires no casts.
 *
 * Since: 3.1.9
 **/
void
gnutls_transport_set_int2(gnutls_session_t session,
			  int recv_fd, int send_fd)
{
	session->internals.transport_send_ptr =
	    (gnutls_transport_ptr_t) (long) send_fd;
	session->internals.transport_recv_ptr =
	    (gnutls_transport_ptr_t) (long) recv_fd;
}

#if 0
/* this will be a macro */
/**
 * gnutls_transport_set_int:
 * @session: is a #gnutls_session_t type.
 * @fd: is the socket descriptor for the connection.
 *
 * This function sets the first argument of the transport function, such
 * as send() and recv() for the default callbacks using the
 * system's socket API.
 *
 * This function is equivalent to calling gnutls_transport_set_ptr()
 * with the descriptor, but requires no casts.
 *
 * Since: 3.1.9
 * 
 **/
void gnutls_transport_set_int(gnutls_session_t session, int fd)
{
	session->internals.transport_recv_ptr =
	    (gnutls_transport_ptr_t) (long) fd;
	session->internals.transport_send_ptr =
	    (gnutls_transport_ptr_t) (long) fd;
}
#endif

/**
 * gnutls_transport_get_ptr:
 * @session: is a #gnutls_session_t type.
 *
 * Used to get the first argument of the transport function (like
 * PUSH and PULL).  This must have been set using
 * gnutls_transport_set_ptr().
 *
 * Returns: The first argument of the transport function.
 **/
gnutls_transport_ptr_t gnutls_transport_get_ptr(gnutls_session_t session)
{
	return session->internals.transport_recv_ptr;
}

/**
 * gnutls_transport_get_ptr2:
 * @session: is a #gnutls_session_t type.
 * @recv_ptr: will hold the value for the pull function
 * @send_ptr: will hold the value for the push function
 *
 * Used to get the arguments of the transport functions (like PUSH
 * and PULL).  These should have been set using
 * gnutls_transport_set_ptr2().
 **/
void
gnutls_transport_get_ptr2(gnutls_session_t session,
			  gnutls_transport_ptr_t * recv_ptr,
			  gnutls_transport_ptr_t * send_ptr)
{

	*recv_ptr = session->internals.transport_recv_ptr;
	*send_ptr = session->internals.transport_send_ptr;
}

/**
 * gnutls_transport_get_int2:
 * @session: is a #gnutls_session_t type.
 * @recv_int: will hold the value for the pull function
 * @send_int: will hold the value for the push function
 *
 * Used to get the arguments of the transport functions (like PUSH
 * and PULL).  These should have been set using
 * gnutls_transport_set_int2().
 *
 * Since: 3.1.9
 **/
void
gnutls_transport_get_int2(gnutls_session_t session,
			  int *recv_int, int *send_int)
{

	*recv_int = (long) session->internals.transport_recv_ptr;
	*send_int = (long) session->internals.transport_send_ptr;
}

/**
 * gnutls_transport_get_int:
 * @session: is a #gnutls_session_t type.
 *
 * Used to get the first argument of the transport function (like
 * PUSH and PULL).  This must have been set using
 * gnutls_transport_set_int().
 *
 * Returns: The first argument of the transport function.
 *
 * Since: 3.1.9
 **/
int gnutls_transport_get_int(gnutls_session_t session)
{
	return (long) session->internals.transport_recv_ptr;
}

/**
 * gnutls_bye:
 * @session: is a #gnutls_session_t type.
 * @how: is an integer
 *
 * Terminates the current TLS/SSL connection. The connection should
 * have been initiated using gnutls_handshake().  @how should be one
 * of %GNUTLS_SHUT_RDWR, %GNUTLS_SHUT_WR.
 *
 * In case of %GNUTLS_SHUT_RDWR the TLS session gets
 * terminated and further receives and sends will be disallowed.  If
 * the return value is zero you may continue using the underlying
 * transport layer. %GNUTLS_SHUT_RDWR sends an alert containing a close
 * request and waits for the peer to reply with the same message.
 *
 * In case of %GNUTLS_SHUT_WR the TLS session gets terminated
 * and further sends will be disallowed. In order to reuse the
 * connection you should wait for an EOF from the peer.
 * %GNUTLS_SHUT_WR sends an alert containing a close request.
 *
 * Note that not all implementations will properly terminate a TLS
 * connection.  Some of them, usually for performance reasons, will
 * terminate only the underlying transport layer, and thus not
 * distinguishing between a malicious party prematurely terminating 
 * the connection and normal termination. 
 *
 * This function may also return %GNUTLS_E_AGAIN or
 * %GNUTLS_E_INTERRUPTED; cf.  gnutls_record_get_direction().
 *
 * Returns: %GNUTLS_E_SUCCESS on success, or an error code, see
 *   function documentation for entire semantics.
 **/
int gnutls_bye(gnutls_session_t session, gnutls_close_request_t how)
{
	int ret = 0;

	switch (BYE_STATE) {
	case BYE_STATE0:
		ret = _gnutls_io_write_flush(session);
		BYE_STATE = BYE_STATE0;
		if (ret < 0) {
			gnutls_assert();
			return ret;
		}
		FALLTHROUGH;
	case BYE_STATE1:
		ret =
		    gnutls_alert_send(session, GNUTLS_AL_WARNING,
				      GNUTLS_A_CLOSE_NOTIFY);
		BYE_STATE = BYE_STATE1;
		if (ret < 0) {
			gnutls_assert();
			return ret;
		}
		FALLTHROUGH;
	case BYE_STATE2:
		BYE_STATE = BYE_STATE2;
		if (how == GNUTLS_SHUT_RDWR) {
			do {
				ret =
				    _gnutls_recv_int(session, GNUTLS_ALERT,
						     NULL, 0, NULL,
						     session->internals.
						     record_timeout_ms);
			}
			while (ret == GNUTLS_E_GOT_APPLICATION_DATA);

			if (ret >= 0)
				session->internals.may_not_read = 1;

			if (ret < 0) {
				gnutls_assert();
				return ret;
			}
		}
		BYE_STATE = BYE_STATE2;

		break;
	default:
		gnutls_assert();
		return GNUTLS_E_INTERNAL_ERROR;
	}

	BYE_STATE = BYE_STATE0;

	session->internals.may_not_write = 1;
	return 0;
}

inline static void session_unresumable(gnutls_session_t session)
{
	session->internals.resumable = RESUME_FALSE;
}

/* returns 0 if session is valid
 */
inline static int session_is_valid(gnutls_session_t session)
{
	if (session->internals.invalid_connection != 0)
		return GNUTLS_E_INVALID_SESSION;

	return 0;
}

/* Copies the record version into the headers. The 
 * version must have 2 bytes at least.
 */
inline static int
copy_record_version(gnutls_session_t session,
		    gnutls_handshake_description_t htype,
		    uint8_t version[2])
{
	const version_entry_st *lver;

	lver = get_version(session);
	if (session->internals.initial_negotiation_completed ||
	    htype != GNUTLS_HANDSHAKE_CLIENT_HELLO ||
	    session->internals.default_record_version[0] == 0) {

		if (unlikely(lver == NULL))
			return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

		if (lver->tls13_sem) {
			version[0] = 0x03;
			version[1] = 0x03;
		} else {
			version[0] = lver->major;
			version[1] = lver->minor;
		}
	} else {
		version[0] = session->internals.default_record_version[0];
		version[1] = session->internals.default_record_version[1];
	}

	return 0;
}

/* Increments the sequence value
 */
inline static int
sequence_increment(gnutls_session_t session, gnutls_uint64 * value)
{
	if (IS_DTLS(session)) {
		return _gnutls_uint48pp(value);
	} else {
		return _gnutls_uint64pp(value);
	}
}

/* This function behaves exactly like write(). The only difference is
 * that it accepts, the gnutls_session_t and the content_type_t of data to
 * send (if called by the user the Content is specific)
 * It is intended to transfer data, under the current session.    
 *
 * @type: The content type to send
 * @htype: If this is a handshake message then the handshake type
 * @epoch_rel: %EPOCH_READ_* or %EPOCH_WRITE_*
 * @data: the data to be sent
 * @data_size: the size of the @data
 * @min_pad: the minimum required padding
 * @mflags: zero or %MBUFFER_FLUSH
 *
 * Oct 30 2001: Removed capability to send data more than MAX_RECORD_SIZE.
 * This makes the function much easier to read, and more error resistant
 * (there were cases were the old function could mess everything up).
 * --nmav
 *
 * This function may accept a NULL pointer for data, and 0 for size, if
 * and only if the previous send was interrupted for some reason.
 *
 */
ssize_t
_gnutls_send_tlen_int(gnutls_session_t session, content_type_t type,
		      gnutls_handshake_description_t htype,
		      unsigned int epoch_rel, const void *_data,
		      size_t data_size, size_t min_pad,
		      unsigned int mflags)
{
	mbuffer_st *bufel;
	ssize_t cipher_size;
	int retval, ret;
	int send_data_size;
	uint8_t *headers;
	int header_size;
	const uint8_t *data = _data;
	record_parameters_st *record_params;
	size_t max_send_size;
	record_state_st *record_state;
	const version_entry_st *vers = get_version(session);

	ret = _gnutls_epoch_get(session, epoch_rel, &record_params);
	if (ret < 0)
		return gnutls_assert_val(ret);

	/* Safeguard against processing data with an incomplete cipher state. */
	if (!record_params->initialized)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	record_state = &record_params->write;

	/* Do not allow null pointer if the send buffer is empty.
	 * If the previous send was interrupted then a null pointer is
	 * ok, and means to resume.
	 */
	if (session->internals.record_send_buffer.byte_length == 0 &&
	    (data_size == 0 && _data == NULL)) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	if (type != GNUTLS_ALERT)	/* alert messages are sent anyway */
		if (session_is_valid(session)
		    || session->internals.may_not_write != 0) {
			gnutls_assert();
			return GNUTLS_E_INVALID_SESSION;
		}

	max_send_size = max_record_send_size(session, record_params);

	if (data_size > max_send_size) {
		if (IS_DTLS(session))
			return gnutls_assert_val(GNUTLS_E_LARGE_PACKET);

		send_data_size = max_send_size;
	} else
		send_data_size = data_size;

	/* Only encrypt if we don't have data to send 
	 * from the previous run. - probably interrupted.
	 */
	if (mflags != 0
	    && session->internals.record_send_buffer.byte_length > 0) {
		ret = _gnutls_io_write_flush(session);
		if (ret > 0)
			cipher_size = ret;
		else
			cipher_size = 0;

		retval = session->internals.record_send_buffer_user_size;
	} else {
		if (unlikely((send_data_size == 0 && min_pad == 0)))
			return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

		/* now proceed to packet encryption
		 */
		cipher_size = MAX_RECORD_SEND_SIZE(session);

		bufel = _mbuffer_alloc_align16(cipher_size + CIPHER_SLACK_SIZE, 
			get_total_headers2(session, record_params));
		if (bufel == NULL)
			return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

		headers = _mbuffer_get_uhead_ptr(bufel);
		if (vers->tls13_sem && record_params->cipher->id != GNUTLS_CIPHER_NULL)
			headers[0] = GNUTLS_APPLICATION_DATA;
		else
			headers[0] = type;

		/* Use the default record version, if it is
		 * set. */
		ret = copy_record_version(session, htype, &headers[1]);
		if (ret < 0)
			return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

		/* Adjust header length and add sequence for DTLS */
		if (IS_DTLS(session))
			memcpy(&headers[3],
			       record_state->sequence_number.i, 8);

		_gnutls_record_log
		    ("REC[%p]: Preparing Packet %s(%d) with length: %d and min pad: %d\n",
		     session, _gnutls_packet2str(type), type,
		     (int) data_size, (int) min_pad);

		header_size = RECORD_HEADER_SIZE(session);
		_mbuffer_set_udata_size(bufel, cipher_size);
		_mbuffer_set_uhead_size(bufel, header_size);

		ret =
		    _gnutls_encrypt(session,
				    data, send_data_size, min_pad,
				    bufel, type, record_params);
		if (ret <= 0) {
			gnutls_assert();
			if (ret == 0)
				ret = GNUTLS_E_ENCRYPTION_FAILED;
			gnutls_free(bufel);
			return ret;	/* error */
		}

		cipher_size = _mbuffer_get_udata_size(bufel);
		retval = send_data_size;
		session->internals.record_send_buffer_user_size =
		    send_data_size;

		/* increase sequence number
		 */
		if (sequence_increment
		    (session, &record_state->sequence_number) != 0) {
			session_invalidate(session);
			gnutls_free(bufel);
			return
			    gnutls_assert_val
			    (GNUTLS_E_RECORD_LIMIT_REACHED);
		}

		ret = _gnutls_io_write_buffered(session, bufel, mflags);
	}

	if (ret != cipher_size) {
		/* If we have sent any data then just return
		 * the error value. Do not invalidate the session.
		 */
		if (ret < 0 && gnutls_error_is_fatal(ret) == 0)
			return gnutls_assert_val(ret);

		if (ret > 0)
			ret = gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

		session_unresumable(session);
		session->internals.may_not_write = 1;
		return gnutls_assert_val(ret);
	}

	session->internals.record_send_buffer_user_size = 0;

	_gnutls_record_log
	    ("REC[%p]: Sent Packet[%d] %s(%d) in epoch %d and length: %d\n",
	     session, (unsigned int)
	     _gnutls_uint64touint32(&record_state->sequence_number),
	     _gnutls_packet2str(type), type, (int) record_params->epoch,
	     (int) cipher_size);

	if (vers->tls13_sem && !(session->internals.flags & GNUTLS_NO_AUTO_REKEY) &&
	    !(record_params->cipher->no_rekey)) {
		if (unlikely(record_state->sequence_number.i[7] == 0xfd &&
		    record_state->sequence_number.i[6] == 0xff &&
		    record_state->sequence_number.i[5] == 0xff)) {
			/* After we have sent 2^24 messages, mark the session
			 * as needing a key update. */
			session->internals.rsend_state = RECORD_SEND_KEY_UPDATE_1;
		}
	}

	return retval;
}

inline static int
check_recv_type(gnutls_session_t session, content_type_t recv_type)
{
	switch (recv_type) {
	case GNUTLS_CHANGE_CIPHER_SPEC:
	case GNUTLS_ALERT:
	case GNUTLS_HANDSHAKE:
	case GNUTLS_HEARTBEAT:
	case GNUTLS_APPLICATION_DATA:
		return 0;
	default:
		gnutls_assert();
		_gnutls_audit_log(session,
				  "Received record packet of unknown type %u\n",
				  (unsigned int) recv_type);
		return GNUTLS_E_UNEXPECTED_PACKET;
	}

}


/* Checks if there are pending data in the record buffers. If there are
 * then it copies the data.
 */
static int
get_data_from_buffers(gnutls_session_t session, content_type_t type,
	      uint8_t * data, int data_size, void *seq)
{
	if ((type == GNUTLS_APPLICATION_DATA ||
	     type == GNUTLS_HANDSHAKE || type == GNUTLS_CHANGE_CIPHER_SPEC)
	    && _gnutls_record_buffer_get_size(session) > 0) {
		int ret;
		ret =
		    _gnutls_record_buffer_get(type, session, data,
					      data_size, seq);
		if (ret < 0) {
			if (IS_DTLS(session)) {
				if (ret == GNUTLS_E_UNEXPECTED_PACKET) {
					ret = GNUTLS_E_AGAIN;
				}
			}
			gnutls_assert();
			return ret;
		}

		return ret;
	}

	return 0;
}

/* Checks and retrieves any pending data in the application data record buffers.
 */
static int
get_packet_from_buffers(gnutls_session_t session, content_type_t type,
		     gnutls_packet_t *packet)
{
	if (_gnutls_record_buffer_get_size(session) > 0) {
		int ret;
		ret =
			_gnutls_record_buffer_get_packet(type, session, packet);
		if (ret < 0) {
			if (IS_DTLS(session)) {
				if (ret == GNUTLS_E_UNEXPECTED_PACKET) {
					ret = GNUTLS_E_AGAIN;
				}
			}
			gnutls_assert();
			return ret;
		}

		return ret;
	}

	*packet = NULL;
	return 0;
}



/* Here we check if the advertized version is the one we
 * negotiated in the handshake.
 */
inline static int
record_check_version(gnutls_session_t session,
		     gnutls_handshake_description_t htype,
		     uint8_t version[2])
{
	const version_entry_st *vers = get_version(session);
	int diff = 0;

	if (vers->tls13_sem) {
		/* TLS 1.3 requires version to be 0x0303 */
		if (version[0] != 0x03 || version[1] != 0x03)
			diff = 1;
	} else {
		if (vers->major != version[0] || vers->minor != version[1])
			diff = 1;
	}

	if (!IS_DTLS(session)) {
		if (htype == GNUTLS_HANDSHAKE_CLIENT_HELLO ||
		    htype == GNUTLS_HANDSHAKE_HELLO_RETRY_REQUEST ||
		    htype == GNUTLS_HANDSHAKE_SERVER_HELLO) {
			if (version[0] != 3) {
				gnutls_assert();
				_gnutls_record_log
				    ("REC[%p]: INVALID VERSION PACKET: (%d) %d.%d\n",
				     session, htype, version[0],
				     version[1]);
				return GNUTLS_E_UNSUPPORTED_VERSION_PACKET;
			}
		} else if (diff != 0) {
			/* Reject record packets that have a different version than the
			 * one negotiated. Note that this version is not protected by any
			 * mac. I don't really think that this check serves any purpose.
			 */
			gnutls_assert();
			_gnutls_record_log
			    ("REC[%p]: INVALID VERSION PACKET: (%d) %d.%d\n",
			     session, htype, version[0], version[1]);

			return GNUTLS_E_UNSUPPORTED_VERSION_PACKET;
		}
	} else {		/* DTLS */

		/* In DTLS the only information we have here is whether we
		 * expect a handshake message or not.
		 */
		if (htype == (gnutls_handshake_description_t) - 1) {
			if (diff) {
				/* Reject record packets that have a different version than the
				 * one negotiated. Note that this version is not protected by any
				 * mac. I don't really think that this check serves any purpose.
				 */
				gnutls_assert();
				_gnutls_record_log
				    ("REC[%p]: INVALID VERSION PACKET: (%d) %d.%d\n",
				     session, htype, version[0],
				     version[1]);

				return GNUTLS_E_UNSUPPORTED_VERSION_PACKET;
			}
		} else if (vers->id > GNUTLS_DTLS1_0 && version[0] > 254) {
			gnutls_assert();
			_gnutls_record_log
			    ("REC[%p]: INVALID DTLS VERSION PACKET: (%d) %d.%d\n",
			     session, htype, version[0], version[1]);
			return GNUTLS_E_UNSUPPORTED_VERSION_PACKET;
		} else if (vers->id == GNUTLS_DTLS0_9 && version[0] > 1) {
			gnutls_assert();
			_gnutls_record_log
			    ("REC[%p]: INVALID DTLS VERSION PACKET: (%d) %d.%d\n",
			     session, htype, version[0], version[1]);
			return GNUTLS_E_UNSUPPORTED_VERSION_PACKET;
		}
	}

	return 0;
}

static int
recv_hello_request(gnutls_session_t session, void *data,
		   uint32_t data_size)
{
	uint8_t type;

	if (session->security_parameters.entity == GNUTLS_SERVER)
		return gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET);

	if (data_size < 1)
		return gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET_LENGTH);

	if (session->internals.handshake_in_progress)
		return gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET);

	type = ((uint8_t *) data)[0];
	if (type == GNUTLS_HANDSHAKE_HELLO_REQUEST) {
		if (IS_DTLS(session))
			session->internals.dtls.hsk_read_seq++;
		if (session->internals.flags & GNUTLS_AUTO_REAUTH) {
			session->internals.recv_state = RECV_STATE_REHANDSHAKE;
			return GNUTLS_E_AGAIN;
		} else {
			return GNUTLS_E_REHANDSHAKE;
		}
	} else {
		gnutls_assert();
		return GNUTLS_E_UNEXPECTED_PACKET;
	}
}

/* This function will check if the received record type is
 * the one we actually expect and adds it to the proper
 * buffer. The bufel will be deinitialized after calling
 * this function, even if it fails.
 */
static int
record_add_to_buffers(gnutls_session_t session,
		      struct tls_record_st *recv, content_type_t type,
		      gnutls_handshake_description_t htype,
		      gnutls_uint64 * seq, mbuffer_st * bufel)
{

	int ret;
	const version_entry_st *ver = get_version(session);

	if ((recv->type == type)
	    && (type == GNUTLS_APPLICATION_DATA ||
		type == GNUTLS_CHANGE_CIPHER_SPEC ||
		type == GNUTLS_HANDSHAKE)) {
		if (bufel->msg.size == 0) {
			if (type == GNUTLS_APPLICATION_DATA) {
				/* this is needed to distinguish an empty
				 * message and EOF */
				ret = GNUTLS_E_AGAIN;
				goto cleanup;
			} else {
				ret =
				    gnutls_assert_val
				    (GNUTLS_E_UNEXPECTED_PACKET);
				goto unexpected_packet;
			}
		}

		/* application data cannot be inserted between (async) handshake
		 * messages */
		if (type == GNUTLS_APPLICATION_DATA &&
		    (session->internals.handshake_recv_buffer_size != 0 ||
		     session->internals.handshake_header_recv_buffer.length != 0)) {
			ret = gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET);
			goto unexpected_packet;
		}

		_gnutls_record_buffer_put(session, type, seq, bufel);

		/* if we received application data as expected then we
		 * deactivate the async timer */
		_dtls_async_timer_delete(session);
	} else {
		/* if the expected type is different than the received 
		 */
		switch (recv->type) {
		case GNUTLS_ALERT:
			if (bufel->msg.size < 2) {
				ret =
				    gnutls_assert_val
				    (GNUTLS_E_UNEXPECTED_PACKET);
				goto unexpected_packet;
			}

			_gnutls_record_log
			    ("REC[%p]: Alert[%d|%d] - %s - was received\n",
			     session, bufel->msg.data[0],
			     bufel->msg.data[1],
			     gnutls_alert_get_name((int) bufel->msg.
						   data[1]));

			if (!session->internals.initial_negotiation_completed &&
			    session->internals.handshake_in_progress && STATE == STATE0) { /* handshake hasn't started */
				ret = gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET);
				goto unexpected_packet;
			}

			session->internals.last_alert = bufel->msg.data[1];

			/* if close notify is received and
			 * the alert is not fatal
			 */
			if (bufel->msg.data[1] == GNUTLS_A_CLOSE_NOTIFY
			    && bufel->msg.data[0] != GNUTLS_AL_FATAL) {
				/* If we have been expecting for an alert do 
				 */
				session->internals.read_eof = 1;
				ret = GNUTLS_E_SESSION_EOF;
				goto cleanup;
			} else {
				/* if the alert is FATAL or WARNING
				 * return the appropriate message
				 */
				gnutls_assert();
				ret = GNUTLS_E_WARNING_ALERT_RECEIVED;
				if ((ver && ver->tls13_sem) || bufel->msg.data[0] == GNUTLS_AL_FATAL) {
					session_unresumable(session);
					session_invalidate(session);
					ret =
					    gnutls_assert_val
					    (GNUTLS_E_FATAL_ALERT_RECEIVED);
				}
				goto cleanup;
			}
			break;

		case GNUTLS_CHANGE_CIPHER_SPEC:
			if (!(IS_DTLS(session))) {
				ret = gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET);
				goto cleanup;
			}

			_gnutls_record_buffer_put(session, recv->type, seq,
						  bufel);

			break;

#ifdef ENABLE_HEARTBEAT
		case GNUTLS_HEARTBEAT:
			ret = _gnutls_heartbeat_handle(session, bufel);
			goto cleanup;
#endif

		case GNUTLS_APPLICATION_DATA:
			if (session->internals.
			    initial_negotiation_completed == 0) {
				ret =
				    gnutls_assert_val
				    (GNUTLS_E_UNEXPECTED_PACKET);
				goto unexpected_packet;
			}

			/* In TLS1.3 post-handshake authentication allow application
			 * data error code. */
			if ((ver && ver->tls13_sem) && type == GNUTLS_HANDSHAKE &&
			    htype == GNUTLS_HANDSHAKE_CERTIFICATE_PKT &&
			    session->internals.initial_negotiation_completed) {
				_gnutls_record_buffer_put(session, recv->type,
							  seq, bufel);
				return
				    gnutls_assert_val
				    (GNUTLS_E_GOT_APPLICATION_DATA);
			}

			/* The got_application data is only returned
			 * if expecting client hello (for rehandshake
			 * reasons). Otherwise it is an unexpected packet
			 */
			if (type == GNUTLS_ALERT
			    || ((htype == GNUTLS_HANDSHAKE_SERVER_HELLO ||
			         htype == GNUTLS_HANDSHAKE_CLIENT_HELLO ||
			         htype == GNUTLS_HANDSHAKE_HELLO_RETRY_REQUEST)
				&& type == GNUTLS_HANDSHAKE)) {
				/* even if data is unexpected put it into the buffer */
				_gnutls_record_buffer_put(session, recv->type,
							  seq, bufel);
				return
				    gnutls_assert_val
				    (GNUTLS_E_GOT_APPLICATION_DATA);
			} else {
				ret =
				    gnutls_assert_val
				    (GNUTLS_E_UNEXPECTED_PACKET);
				goto unexpected_packet;
			}

			break;

		case GNUTLS_HANDSHAKE:
			/* In DTLS we might receive a handshake replay from the peer to indicate
			 * the our last TLS handshake messages were not received.
			 */
			if (IS_DTLS(session)) {
				if (type == GNUTLS_CHANGE_CIPHER_SPEC) {
					ret =
					    gnutls_assert_val
					    (GNUTLS_E_UNEXPECTED_PACKET);
					goto unexpected_packet;
				}

				if (_dtls_is_async(session)
				    && _dtls_async_timer_active(session)) {
					if (session->security_parameters.
					    entity == GNUTLS_SERVER
					    && bufel->htype ==
					    GNUTLS_HANDSHAKE_CLIENT_HELLO)
					{
						/* client requested rehandshake. Delete the timer */
						_dtls_async_timer_delete
						    (session);
					} else {
						session->internals.
						    recv_state =
						    RECV_STATE_DTLS_RETRANSMIT;
						ret =
						    _dtls_retransmit
						    (session);
						if (ret == 0) {
							session->internals.
							    recv_state =
							    RECV_STATE_0;
							ret =
							    gnutls_assert_val
							    (GNUTLS_E_AGAIN);
							goto unexpected_packet;
						}
						goto cleanup;
					}
				}
			}

			/* retrieve async handshake messages */
			if (ver && ver->tls13_sem) {
				_gnutls_record_buffer_put(session, recv->type, seq, bufel);

				ret = _gnutls13_recv_async_handshake(session);
				if (ret < 0)
					return gnutls_assert_val(ret);

				/* bufel is now accounted */
				return GNUTLS_E_AGAIN;
			}

			/* This is legal if HELLO_REQUEST is received - and we are a client.
			 * If we are a server, a client may initiate a renegotiation at any time.
			 */
			if (session->security_parameters.entity ==
			    GNUTLS_SERVER
			    && session->internals.handshake_in_progress == 0
			    && bufel->htype ==
			    GNUTLS_HANDSHAKE_CLIENT_HELLO) {
				gnutls_assert();
				_gnutls_record_buffer_put(session,
							      recv->type,
							      seq, bufel);
				return GNUTLS_E_REHANDSHAKE;
			}

			/* If we are already in a handshake then a Hello
			 * Request is illegal. But here we don't really care
			 * since this message will never make it up here.
			 */

			/* So we accept it, if it is a Hello. If not, this will
			 * fail and trigger flight retransmissions after some time. */
			ret =
			    recv_hello_request(session,
					       bufel->msg.data,
					       bufel->msg.size);
			goto unexpected_packet;
		default:

			_gnutls_record_log
			    ("REC[%p]: Received unexpected packet %d (%s) expecting %d (%s)\n",
			     session, recv->type,
			     _gnutls_packet2str(recv->type), type,
			     _gnutls_packet2str(type));

			gnutls_assert();
			ret = GNUTLS_E_UNEXPECTED_PACKET;
			goto unexpected_packet;
		}
	}

	return 0;

 unexpected_packet:

	if (IS_DTLS(session) && ret != GNUTLS_E_REHANDSHAKE) {
		_mbuffer_xfree(&bufel);
		RETURN_DTLS_EAGAIN_OR_TIMEOUT(session, ret);
	}

 cleanup:
	_mbuffer_xfree(&bufel);
	return ret;

}


/* Checks the record headers and returns the length, version and
 * content type.
 */
static void
record_read_headers(gnutls_session_t session,
		    uint8_t headers[MAX_RECORD_HEADER_SIZE],
		    content_type_t type,
		    gnutls_handshake_description_t htype,
		    struct tls_record_st *record)
{

	/* Read the first two bytes to determine if this is a 
	 * version 2 message 
	 */

#ifdef ENABLE_SSL2
	if (htype == GNUTLS_HANDSHAKE_CLIENT_HELLO
	    && type == GNUTLS_HANDSHAKE && headers[0] > 127
	    && !(IS_DTLS(session))) {

		/* if msb set and expecting handshake message
		 * it should be SSL 2 hello 
		 */
		record->version[0] = 3;	/* assume SSL 3.0 */
		record->version[1] = 0;

		record->length = (((headers[0] & 0x7f) << 8)) | headers[1];

		/* SSL 2.0 headers */
		record->header_size = record->packet_size = 2;
		record->type = GNUTLS_HANDSHAKE;	/* we accept only v2 client hello
							 */

		/* in order to assist the handshake protocol.
		 * V2 compatibility is a mess.
		 */
		record->v2 = 1;
		record->epoch = 0;
		memset(&record->sequence, 0, sizeof(record->sequence));

		_gnutls_record_log
		    ("REC[%p]: SSL 2.0 %s packet received. Length: %d\n",
		     session, _gnutls_packet2str(record->type),
		     record->length);

	} else
#endif
	{
		/* dtls version 1.0 and TLS version 1.x */
#ifdef ENABLE_SSL2
		record->v2 = 0;
#endif

		record->type = headers[0];
		record->version[0] = headers[1];
		record->version[1] = headers[2];

		if (IS_DTLS(session)) {
			memcpy(record->sequence.i, &headers[3], 8);
			record->length = _gnutls_read_uint16(&headers[11]);
			record->epoch =
			    _gnutls_read_uint16(record->sequence.i);
		} else {
			memset(&record->sequence, 0,
			       sizeof(record->sequence));
			record->length = _gnutls_read_uint16(&headers[3]);
			record->epoch = session->security_parameters.epoch_read;
		}

		_gnutls_record_log
		    ("REC[%p]: SSL %d.%d %s packet received. Epoch %d, length: %d\n",
		     session, (int) record->version[0],
		     (int) record->version[1],
		     _gnutls_packet2str(record->type), (int) record->epoch,
		     record->length);

	}

	record->packet_size += record->length;
}


static int recv_headers(gnutls_session_t session, 
			record_parameters_st *record_params,
			content_type_t type,
			gnutls_handshake_description_t htype,
			struct tls_record_st *record, unsigned int *ms)
{
	int ret;
	gnutls_datum_t raw;	/* raw headers */
	/* Read the headers.
	 */
	record->header_size = record->packet_size =
	    RECORD_HEADER_SIZE(session);

	ret =
	    _gnutls_io_read_buffered(session, record->header_size, -1, ms);
	if (ret != record->header_size) {
		if (ret < 0 && gnutls_error_is_fatal(ret) == 0)
			return ret;

		if (ret > 0)
			ret = GNUTLS_E_UNEXPECTED_PACKET_LENGTH;
		else if (ret == 0)
			ret = GNUTLS_E_PREMATURE_TERMINATION;

		return gnutls_assert_val(ret);
	}

	ret = _mbuffer_linearize_align16(&session->internals.record_recv_buffer, 
		get_total_headers2(session, record_params));
	if (ret < 0)
		return gnutls_assert_val(ret);

	_mbuffer_head_get_first(&session->internals.record_recv_buffer,
				&raw);
	if (raw.size < RECORD_HEADER_SIZE(session))
		return
		    gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET_LENGTH);

	record_read_headers(session, raw.data, type, htype, record);

	/* Check if the DTLS epoch is valid */
	if (IS_DTLS(session)) {
		if (_gnutls_epoch_is_valid(session, record->epoch) == 0) {
			_gnutls_audit_log(session,
					  "Discarded message[%u] with invalid epoch %u.\n",
					  (unsigned int)
					  _gnutls_uint64touint32(&record->
								 sequence),
					  (unsigned int) record->sequence.
					  i[0] * 256 +
					  (unsigned int) record->sequence.
					  i[1]);
			gnutls_assert();
			/* doesn't matter, just a fatal error */
			return GNUTLS_E_UNEXPECTED_PACKET_LENGTH;
		}
	}

	/* Here we check if the Type of the received packet is
	 * ok. 
	 */
	if ((ret = check_recv_type(session, record->type)) < 0)
		return gnutls_assert_val(ret);

	/* Here we check if the advertized version is the one we
	 * negotiated in the handshake.
	 */
	if ((ret =
	     record_check_version(session, htype, record->version)) < 0)
		return gnutls_assert_val(ret);

	if (record->length == 0 || record->length > max_record_recv_size(session)) {
		_gnutls_audit_log
		    (session, "Received packet with illegal length: %u (max: %u)\n",
		     (unsigned int) record->length, (unsigned)max_record_recv_size(session));

		if (record->length == 0) {
			/* Empty, unencrypted records are always unexpected. */
			if (record_params->cipher->id == GNUTLS_CIPHER_NULL)
				return
				    gnutls_assert_val
				    (GNUTLS_E_UNEXPECTED_PACKET);

			return gnutls_assert_val(GNUTLS_E_DECRYPTION_FAILED);
		}
		return
		    gnutls_assert_val(GNUTLS_E_RECORD_OVERFLOW);
	}

	_gnutls_record_log
	    ("REC[%p]: Expected Packet %s(%d)\n", session,
	     _gnutls_packet2str(type), type);
	_gnutls_record_log
	    ("REC[%p]: Received Packet %s(%d) with length: %d\n", session,
	     _gnutls_packet2str(record->type), record->type,
	     record->length);


	return 0;
}

/* @ms: is the number of milliseconds to wait for data. Use zero for indefinite.
 *
 * This will receive record layer packets and add them to 
 * application_data_buffer and handshake_data_buffer.
 *
 * If the htype is not -1 then handshake timeouts
 * will be enforced.
 */
ssize_t
_gnutls_recv_in_buffers(gnutls_session_t session, content_type_t type,
			gnutls_handshake_description_t htype,
			unsigned int ms)
{
	gnutls_uint64 *packet_sequence;
	gnutls_datum_t ciphertext;
	mbuffer_st *bufel = NULL, *decrypted = NULL;
	gnutls_datum_t t;
	int ret;
	unsigned int n_retries = 0;
	record_parameters_st *record_params;
	record_state_st *record_state;
	struct tls_record_st record;
	const version_entry_st *vers = get_version(session);

 begin:

	if (n_retries > DEFAULT_MAX_EMPTY_RECORDS) {
		gnutls_assert();
		return GNUTLS_E_TOO_MANY_EMPTY_PACKETS;
	}

	if (session->internals.read_eof != 0) {
		/* if we have already read an EOF
		 */
		return 0;
	} else if (session_is_valid(session) != 0
		   || session->internals.may_not_read != 0)
		return gnutls_assert_val(GNUTLS_E_INVALID_SESSION);

	/* get the record state parameters */
	ret =
	    _gnutls_epoch_get(session, EPOCH_READ_CURRENT, &record_params);
	if (ret < 0)
		return gnutls_assert_val(ret);

	/* Safeguard against processing data with an incomplete cipher state. */
	if (!record_params->initialized)
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

	record_state = &record_params->read;

	/* receive headers */
	ret = recv_headers(session, record_params, type, htype, &record, (!(session->internals.flags & GNUTLS_NONBLOCK))?&ms:0);
	if (ret < 0) {
		ret = gnutls_assert_val_fatal(ret);
		goto recv_error;
	}

	if (IS_DTLS(session))
		packet_sequence = &record.sequence;
	else
		packet_sequence = &record_state->sequence_number;

	/* Read the packet data and insert it to record_recv_buffer.
	 */
	ret =
	    _gnutls_io_read_buffered(session, record.packet_size,
				     record.type, (!(session->internals.flags & GNUTLS_NONBLOCK))?&ms:0);
	if (ret != record.packet_size) {
		gnutls_assert();
		goto recv_error;
	}

	/* ok now we are sure that we have read all the data - so
	 * move on !
	 */
	ret = _mbuffer_linearize_align16(&session->internals.record_recv_buffer, 
		get_total_headers2(session, record_params));
	if (ret < 0)
		return gnutls_assert_val(ret);

	bufel =
	    _mbuffer_head_get_first(&session->internals.record_recv_buffer,
				    NULL);
	if (bufel == NULL)
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

	if (vers && vers->tls13_sem && record.type == GNUTLS_CHANGE_CIPHER_SPEC) {
		/* if the CCS has value other than 0x01, or arrives
		 * after Finished, abort the connection */
		if (record.length != 1 ||
		    *((uint8_t *) _mbuffer_get_udata_ptr(bufel) +
		      record.header_size) != 0x01 ||
		    !session->internals.handshake_in_progress)
			return gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET);

		_gnutls_read_log("discarding change cipher spec in TLS1.3\n");
		/* we use the same mechanism to retry as when
		 * receiving multiple empty TLS packets */
		bufel =
		    _mbuffer_head_pop_first(&session->internals.
					    record_recv_buffer);
		_mbuffer_xfree(&bufel);
		n_retries++;
		goto begin;
	}

	/* We allocate the maximum possible to allow few compressed bytes to expand to a
	 * full record. Moreover we add space for any pad and the MAC (in case
	 * they are encrypted).
	 */
	ret = max_decrypted_size(session) + MAX_PAD_SIZE + MAX_HASH_SIZE;
	decrypted = _mbuffer_alloc_align16(ret, 0);
	if (decrypted == NULL)
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

	_mbuffer_set_udata_size(decrypted, ret);
	ciphertext.data =
	    (uint8_t *) _mbuffer_get_udata_ptr(bufel) + record.header_size;
	ciphertext.size = record.length;

	/* decrypt the data we got. 
	 */
	t.data = _mbuffer_get_udata_ptr(decrypted);
	t.size = _mbuffer_get_udata_size(decrypted);
	ret =
	    _gnutls_decrypt(session, &ciphertext, &t,
			    &record.type, record_params, packet_sequence);
	if (ret >= 0)
		_mbuffer_set_udata_size(decrypted, ret);

	_mbuffer_head_remove_bytes(&session->internals.record_recv_buffer,
				   record.header_size + record.length);

	if (session->security_parameters.entity == GNUTLS_SERVER &&
	    session->internals.hsk_flags & HSK_EARLY_DATA_IN_FLIGHT) {
		if (session->internals.hsk_flags & HSK_EARLY_DATA_ACCEPTED) {
			if (ret < 0 ||
			    /* early data must always be encrypted, treat it
			     * as decryption failure if otherwise */
			    record_params->cipher->id == GNUTLS_CIPHER_NULL) {
				_gnutls_record_log
					("REC[%p]: failed to decrypt early data, in epoch %d\n",
					 session,
						record_params->epoch);
				ret = GNUTLS_E_DECRYPTION_FAILED;
				goto sanity_check_error;
			} else if (record.type == GNUTLS_APPLICATION_DATA) {
				size_t decrypted_length =
					_mbuffer_get_udata_size(decrypted);
				_gnutls_record_log
					("REC[%p]: decrypted early data with length: %d, in epoch %d\n",
					 session,
					 (int) decrypted_length,
					 record_params->epoch);
				if (decrypted_length >
				    session->security_parameters.max_early_data_size -
				    session->internals.early_data_received) {
					_gnutls_record_log
						("REC[%p]: max_early_data_size exceeded\n",
						 session);
					ret = GNUTLS_E_UNEXPECTED_PACKET;
					goto sanity_check_error;
				}

				_mbuffer_enqueue(&session->internals.early_data_recv_buffer, decrypted);
				session->internals.early_data_received +=
					decrypted_length;

				/* Increase sequence number. We do both for TLS and DTLS, since in
				 * DTLS we also rely on that number (roughly) since it may get reported
				 * to application via gnutls_record_get_state().
				 */
				if (sequence_increment(session, &record_state->sequence_number) != 0) {
					session_invalidate(session);
					gnutls_assert();
					ret = GNUTLS_E_RECORD_LIMIT_REACHED;
					goto sanity_check_error;
				}

				/* decrypted is now accounted */
				return GNUTLS_E_AGAIN;
			}
		} else {
			/* We do not accept early data: skip decryption
			 * failure up to max_early_data_size. Otherwise,
			 * if the record is properly decrypted, treat it as
			 * the start of client's second flight.
			 */
			if (record.type == GNUTLS_APPLICATION_DATA &&
			    (ret < 0 ||
			     /* early data must always be encrypted, treat it
			      * as decryption failure if otherwise */
			     record_params->cipher->id == GNUTLS_CIPHER_NULL)) {
				if (record.length >
				    session->security_parameters.max_early_data_size -
				    session->internals.early_data_received) {
					_gnutls_record_log
						("REC[%p]: max_early_data_size exceeded\n",
						 session);
					ret = GNUTLS_E_UNEXPECTED_PACKET;
					goto sanity_check_error;
				}

				_gnutls_record_log("REC[%p]: Discarded early data[%u] due to invalid decryption, length: %u\n",
						   session,
						   (unsigned int)
						   _gnutls_uint64touint32(packet_sequence),
						   (unsigned int)
						   record.length);
				session->internals.early_data_received += record.length;
				/* silently discard received data */
				_mbuffer_xfree(&decrypted);
				return gnutls_assert_val(GNUTLS_E_AGAIN);
			} else {
				session->internals.hsk_flags &= ~HSK_EARLY_DATA_IN_FLIGHT;
			}
		}
	}

	if (ret < 0) {
		gnutls_assert();
		_gnutls_audit_log(session,
				  "Discarded message[%u] due to invalid decryption\n",
				  (unsigned int)
				  _gnutls_uint64touint32(packet_sequence));
		goto sanity_check_error;
	}

	if (IS_DTLS(session)) {
		/* check for duplicates. We check after the message
		 * is processed and authenticated to avoid someone
		 * messing with our windows. */
		if (likely(!(session->internals.flags & GNUTLS_NO_REPLAY_PROTECTION))) {
			ret = _dtls_record_check(record_params, packet_sequence);
			if (ret < 0) {
				_gnutls_record_log
				    ("REC[%p]: Discarded duplicate message[%u.%u]: %s\n",
				     session,
				     (unsigned int) record.sequence.i[0] * 256 +
				     (unsigned int) record.sequence.i[1],
				     (unsigned int)
				     _gnutls_uint64touint32(packet_sequence),
				     _gnutls_packet2str(record.type));
				goto sanity_check_error;
			}
		}

		_gnutls_record_log
		    ("REC[%p]: Decrypted Packet[%u.%u] %s(%d) with length: %d\n",
		     session,
		     (unsigned int) record.sequence.i[0] * 256 +
		     (unsigned int) record.sequence.i[1],
		     (unsigned int)
		     _gnutls_uint64touint32(packet_sequence),
		     _gnutls_packet2str(record.type), record.type,
		     (int) _mbuffer_get_udata_size(decrypted));

		/* store the last valid sequence number. We don't use that internally but
		 * callers of gnutls_record_get_state() could take advantage of it. */
		memcpy(&record_state->sequence_number, packet_sequence, 8);
	} else {
		_gnutls_record_log
		    ("REC[%p]: Decrypted Packet[%u] %s(%d) with length: %d\n",
		     session,
		     (unsigned int)
		     _gnutls_uint64touint32(packet_sequence),
		     _gnutls_packet2str(record.type), record.type,
		     (int) _mbuffer_get_udata_size(decrypted));

	}

	/* Increase sequence number. We do both for TLS and DTLS, since in
	 * DTLS we also rely on that number (roughly) since it may get reported
	 * to application via gnutls_record_get_state().
	 */
	if (sequence_increment(session, &record_state->sequence_number) != 0) {
		session_invalidate(session);
		gnutls_assert();
		ret = GNUTLS_E_RECORD_LIMIT_REACHED;
		goto sanity_check_error;
	}

/* (originally for) TLS 1.0 CBC protection. 
 * Actually this code is called if we just received
 * an empty packet. An empty TLS packet is usually
 * sent to protect some vulnerabilities in the CBC mode.
 * In that case we go to the beginning and start reading
 * the next packet.
 */
	if (_mbuffer_get_udata_size(decrypted) == 0 &&
	    /* Under TLS 1.3, there are only AEAD ciphers and this
	     * logic is meaningless. Moreover, the implementation need
	     * to send correct alert upon receiving empty messages in
	     * certain occasions. Skip this and leave
	     * record_add_to_buffers() to handle the empty
	     * messages. */
	    !(vers && vers->tls13_sem)) {
		_mbuffer_xfree(&decrypted);
		n_retries++;
		goto begin;
	}

	if (_mbuffer_get_udata_size(decrypted) > max_decrypted_size(session)) {
		_gnutls_audit_log
		    (session, "Received packet with illegal length: %u\n",
		     (unsigned int) ret);

		ret = gnutls_assert_val(GNUTLS_E_RECORD_OVERFLOW);
		goto sanity_check_error;
	}

#ifdef ENABLE_SSL2
	if (record.v2) {
		decrypted->htype = GNUTLS_HANDSHAKE_CLIENT_HELLO_V2;
	} else
#endif
	{
		uint8_t *p = _mbuffer_get_udata_ptr(decrypted);
		decrypted->htype = p[0];
	}

	ret =
	    record_add_to_buffers(session, &record, type, htype,
				  packet_sequence, decrypted);

	/* decrypted is now either deinitialized or buffered somewhere else */

	if (ret < 0)
		return gnutls_assert_val(ret);

	return ret;

 discard:
	session->internals.dtls.packets_dropped++;

	/* discard the whole received fragment. */
	bufel =
	    _mbuffer_head_pop_first(&session->internals.
				    record_recv_buffer);
	_mbuffer_xfree(&bufel);
	return gnutls_assert_val(GNUTLS_E_AGAIN);

 sanity_check_error:
	if (IS_DTLS(session)) {
		session->internals.dtls.packets_dropped++;
		ret = gnutls_assert_val(GNUTLS_E_AGAIN);
		goto cleanup;
	}

	session_unresumable(session);
	session_invalidate(session);

 cleanup:
	_mbuffer_xfree(&decrypted);
	return ret;

 recv_error:
	if (ret < 0
	    && (gnutls_error_is_fatal(ret) == 0
		|| ret == GNUTLS_E_TIMEDOUT))
		return ret;

	if (type == GNUTLS_ALERT) {	/* we were expecting close notify */
		session_invalidate(session);
		gnutls_assert();
		return 0;
	}

	if (IS_DTLS(session) && (ret == GNUTLS_E_DECRYPTION_FAILED ||
		ret == GNUTLS_E_UNSUPPORTED_VERSION_PACKET ||
		ret == GNUTLS_E_UNEXPECTED_PACKET_LENGTH ||
		ret == GNUTLS_E_RECORD_OVERFLOW ||
		ret == GNUTLS_E_UNEXPECTED_PACKET ||
		ret == GNUTLS_E_ERROR_IN_FINISHED_PACKET ||
		ret == GNUTLS_E_UNEXPECTED_HANDSHAKE_PACKET)) {
		goto discard;
	}

	session_invalidate(session);
	session_unresumable(session);

	if (ret == 0)
		return GNUTLS_E_UNEXPECTED_PACKET_LENGTH;
	else
		return ret;
}

/* Returns a value greater than zero (>= 0) if buffers should be checked
 * for data. */
static ssize_t
check_session_status(gnutls_session_t session, unsigned ms)
{
	int ret;

	if (session->internals.read_eof != 0) {
		/* if we have already read an EOF
		 */
		return 0;
	} else if (session_is_valid(session) != 0
		   || session->internals.may_not_read != 0) {
		gnutls_assert();
		return GNUTLS_E_INVALID_SESSION;
	}

	switch (session->internals.recv_state) {
	case RECV_STATE_REAUTH:
		session->internals.recv_state = RECV_STATE_0;

		ret = gnutls_reauth(session, 0);
		if (ret < 0) {
			/* a temp or fatal error, make sure we reset the state
			 * so we can resume on temp errors */
			session->internals.recv_state = RECV_STATE_REAUTH;
			return gnutls_assert_val(ret);
		}

		return 1;
	case RECV_STATE_REHANDSHAKE:
		session->internals.recv_state = RECV_STATE_0;

		ret = gnutls_handshake(session);
		if (ret < 0) {
			/* a temp or fatal error, make sure we reset the state
			 * so we can resume on temp errors */
			session->internals.recv_state = RECV_STATE_REHANDSHAKE;
			return gnutls_assert_val(ret);
		}

		return 1;
	case RECV_STATE_ASYNC_HANDSHAKE:
		ret = _gnutls_recv_in_buffers(session, GNUTLS_HANDSHAKE, -1, ms);
		if (ret < 0 && ret != GNUTLS_E_SESSION_EOF)
			return gnutls_assert_val(ret);

		ret = _gnutls13_recv_async_handshake(session);
		if (ret < 0)
			return gnutls_assert_val(ret);

		return GNUTLS_E_AGAIN;
	case RECV_STATE_EARLY_START_HANDLING:
	case RECV_STATE_FALSE_START_HANDLING:
		return 1;
	case RECV_STATE_FALSE_START:
		/* if false start is not complete we always expect for handshake packets
		 * prior to anything else. */
		if (session->security_parameters.entity != GNUTLS_CLIENT ||
		    !(session->internals.flags & GNUTLS_ENABLE_FALSE_START))
			return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

		/* Attempt to complete handshake - we only need to receive */
		session->internals.recv_state = RECV_STATE_FALSE_START_HANDLING;
		ret = gnutls_handshake(session);
		if (ret < 0) {
			/* a temp or fatal error, make sure we reset the state
			 * so we can resume on temp errors */
			session->internals.recv_state = RECV_STATE_FALSE_START;
			return gnutls_assert_val(ret);
		}

		session->internals.recv_state = RECV_STATE_0;
		return 1;
	case RECV_STATE_EARLY_START:
		/* if early start is not complete we always expect for handshake packets
		 * prior to anything else. */
		if (session->security_parameters.entity != GNUTLS_SERVER ||
		    !(session->internals.flags & GNUTLS_ENABLE_EARLY_START))
			return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

		/* Attempt to complete handshake - we only need to receive */
		session->internals.recv_state = RECV_STATE_EARLY_START_HANDLING;
		ret = gnutls_handshake(session);
		if (ret < 0) {
			/* a temp or fatal error, make sure we reset the state
			 * so we can resume on temp errors */
			session->internals.recv_state = RECV_STATE_EARLY_START;
			return gnutls_assert_val(ret);
		}

		session->internals.recv_state = RECV_STATE_0;
		return 1;
	case RECV_STATE_DTLS_RETRANSMIT:
		ret = _dtls_retransmit(session);
		if (ret < 0)
			return gnutls_assert_val(ret);

		session->internals.recv_state = RECV_STATE_0;

		FALLTHROUGH;
	case RECV_STATE_0:

		_dtls_async_timer_check(session);
		return 1;
	default:
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);
	}
}

/* This function behaves exactly like read(). The only difference is
 * that it accepts the gnutls_session_t and the content_type_t of data to
 * receive (if called by the user the Content is Userdata only)
 * It is intended to receive data, under the current session.
 */
ssize_t
_gnutls_recv_int(gnutls_session_t session, content_type_t type,
		 uint8_t * data, size_t data_size, void *seq,
		 unsigned int ms)
{
	int ret;

	if ((type != GNUTLS_ALERT && type != GNUTLS_HEARTBEAT)
	    && (data_size == 0 || data == NULL))
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	ret = check_session_status(session, ms);
	if (ret <= 0)
		return ret;

	/* If we have enough data in the cache do not bother receiving
	 * a new packet. (in order to flush the cache)
	 */
	ret = get_data_from_buffers(session, type, data, data_size, seq);
	if (ret != 0)
		return ret;

	ret = _gnutls_recv_in_buffers(session, type, -1, ms);
	if (ret < 0 && ret != GNUTLS_E_SESSION_EOF)
		return gnutls_assert_val(ret);

	return get_data_from_buffers(session, type, data, data_size, seq);
}

/**
 * gnutls_packet_get:
 * @packet: is a #gnutls_packet_t type.
 * @data: will contain the data present in the @packet structure (may be %NULL)
 * @sequence: the 8-bytes of the packet sequence number (may be %NULL)
 *
 * This function returns the data and sequence number associated with
 * the received packet.
 *
 * Since: 3.3.5
 **/

void gnutls_packet_get(gnutls_packet_t packet, gnutls_datum_t *data, unsigned char *sequence)
{
	if (unlikely(packet == NULL)) {
		gnutls_assert();
		if (data) {
			data->data = NULL;
			data->size = 0;
			return;
		}
	}

	assert(packet != NULL);

	if (sequence) {
		memcpy(sequence, packet->record_sequence.i, 8);
	}

	if (data) {
		data->size = packet->msg.size - packet->mark;
		data->data = packet->msg.data + packet->mark;
	}
}

/**
 * gnutls_packet_deinit:
 * @packet: is a pointer to a #gnutls_packet_st structure.
 *
 * This function will deinitialize all data associated with
 * the received packet.
 *
 * Since: 3.3.5
 **/
void gnutls_packet_deinit(gnutls_packet_t packet)
{
	gnutls_free(packet);
}

/**
 * gnutls_record_discard_queued:
 * @session: is a #gnutls_session_t type.
 *
 * This function discards all queued to be sent packets in a TLS or DTLS session.
 * These are the packets queued after an interrupted gnutls_record_send().
 *
 * Returns: The number of bytes discarded.
 *
 * Since: 3.4.0
 **/
size_t
gnutls_record_discard_queued(gnutls_session_t session)
{
	size_t ret = session->internals.record_send_buffer.byte_length;
	_mbuffer_head_clear(&session->internals.record_send_buffer);
	return ret;
}

/**
 * gnutls_record_recv_packet:
 * @session: is a #gnutls_session_t type.
 * @packet: the structure that will hold the packet data
 *
 * This is a lower-level function than gnutls_record_recv() and allows
 * to directly receive the whole decrypted packet. That avoids a
 * memory copy, and is intended to be used by applications seeking high
 * performance.
 *
 * The received packet is accessed using gnutls_packet_get() and 
 * must be deinitialized using gnutls_packet_deinit(). The returned
 * packet will be %NULL if the return value is zero (EOF).
 *
 * Returns: The number of bytes received and zero on EOF (for stream
 * connections).  A negative error code is returned in case of an error.  
 *
 * Since: 3.3.5
 **/
ssize_t
gnutls_record_recv_packet(gnutls_session_t session, 
			  gnutls_packet_t *packet)
{
	int ret;

	if (packet == NULL)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	ret = check_session_status(session, session->internals.record_timeout_ms);
	if (ret <= 0)
		return ret;

	ret = get_packet_from_buffers(session, GNUTLS_APPLICATION_DATA, packet);
	if (ret != 0)
		return ret;

	ret = _gnutls_recv_in_buffers(session, GNUTLS_APPLICATION_DATA, -1,
				      session->internals.record_timeout_ms);
	if (ret < 0 && ret != GNUTLS_E_SESSION_EOF)
		return gnutls_assert_val(ret);

	return get_packet_from_buffers(session, GNUTLS_APPLICATION_DATA, packet);
}

static
ssize_t append_data_to_corked(gnutls_session_t session, const void *data, size_t data_size)
{
	int ret;

	if (IS_DTLS(session)) {
		if (data_size + session->internals.record_presend_buffer.length >
			gnutls_dtls_get_data_mtu(session)) {
			return gnutls_assert_val(GNUTLS_E_LARGE_PACKET);
		}
	}

	ret =
	    _gnutls_buffer_append_data(&session->internals.
				       record_presend_buffer, data,
				       data_size);
	if (ret < 0)
		return gnutls_assert_val(ret);

	return data_size;
}

/**
 * gnutls_record_send:
 * @session: is a #gnutls_session_t type.
 * @data: contains the data to send
 * @data_size: is the length of the data
 *
 * This function has the similar semantics with send().  The only
 * difference is that it accepts a GnuTLS session, and uses different
 * error codes.
 * Note that if the send buffer is full, send() will block this
 * function.  See the send() documentation for more information.  
 *
 * You can replace the default push function which is send(), by using
 * gnutls_transport_set_push_function().
 *
 * If the EINTR is returned by the internal push function 
 * then %GNUTLS_E_INTERRUPTED will be returned. If
 * %GNUTLS_E_INTERRUPTED or %GNUTLS_E_AGAIN is returned, you must
 * call this function again with the exact same parameters, or provide a
 * %NULL pointer for @data and 0 for @data_size, in order to write the
 * same data as before. If you wish to discard the previous data instead
 * of retrying, you must call gnutls_record_discard_queued() before
 * calling this function with different parameters.
 * cf. gnutls_record_get_direction(). 
 *
 * Note that in DTLS this function will return the %GNUTLS_E_LARGE_PACKET
 * error code if the send data exceed the data MTU value - as returned
 * by gnutls_dtls_get_data_mtu(). The errno value EMSGSIZE
 * also maps to %GNUTLS_E_LARGE_PACKET. 
 * Note that since 3.2.13 this function can be called under cork in DTLS
 * mode, and will refuse to send data over the MTU size by returning
 * %GNUTLS_E_LARGE_PACKET.
 *
 * Returns: The number of bytes sent, or a negative error code.  The
 *   number of bytes sent might be less than @data_size.  The maximum
 *   number of bytes this function can send in a single call depends
 *   on the negotiated maximum record size.
 **/
ssize_t
gnutls_record_send(gnutls_session_t session, const void *data,
		   size_t data_size)
{
	return gnutls_record_send2(session, data, data_size, 0, 0);
}

/**
 * gnutls_record_send2:
 * @session: is a #gnutls_session_t type.
 * @data: contains the data to send
 * @data_size: is the length of the data
 * @pad: padding to be added to the record
 * @flags: must be zero
 *
 * This function is identical to gnutls_record_send() except that it
 * takes an extra argument to specify padding to be added the record.
 * To determine the maximum size of padding, use
 * gnutls_record_get_max_size() and gnutls_record_overhead_size().
 *
 * Note that in order for GnuTLS to provide constant time processing
 * of padding and data in TLS1.3, the flag %GNUTLS_SAFE_PADDING_CHECK
 * must be used in gnutls_init().
 *
 * Returns: The number of bytes sent, or a negative error code.  The
 *   number of bytes sent might be less than @data_size.  The maximum
 *   number of bytes this function can send in a single call depends
 *   on the negotiated maximum record size.
 *
 * Since: 3.6.3
 **/
ssize_t
gnutls_record_send2(gnutls_session_t session, const void *data,
		    size_t data_size, size_t pad, unsigned flags)
{
	const version_entry_st *vers = get_version(session);
	size_t max_pad = 0;
	int ret;

	if (unlikely(!session->internals.initial_negotiation_completed)) {
		/* this is to protect buggy applications from sending unencrypted
		 * data. We allow sending however, if we are in false or early start
		 * handshake state. */
		gnutls_mutex_lock(&session->internals.post_negotiation_lock);

		/* we intentionally re-check the initial_negotation_completed variable
		 * to avoid locking during normal operation of gnutls_record_send2() */
		if (!session->internals.initial_negotiation_completed &&
		    session->internals.recv_state != RECV_STATE_FALSE_START &&
		    session->internals.recv_state != RECV_STATE_FALSE_START_HANDLING &&
		    session->internals.recv_state != RECV_STATE_EARLY_START &&
		    session->internals.recv_state != RECV_STATE_EARLY_START_HANDLING &&
		    !(session->internals.hsk_flags & HSK_EARLY_DATA_IN_FLIGHT)) {

			gnutls_mutex_unlock(&session->internals.post_negotiation_lock);
			return gnutls_assert_val(GNUTLS_E_UNAVAILABLE_DURING_HANDSHAKE);
		}
		gnutls_mutex_unlock(&session->internals.post_negotiation_lock);
	}

	if (unlikely(!vers))
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

	if (vers->tls13_sem)
		max_pad = gnutls_record_get_max_size(session) - gnutls_record_overhead_size(session);

	if (pad > max_pad)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	switch(session->internals.rsend_state) {
		case RECORD_SEND_NORMAL:
			return _gnutls_send_tlen_int(session, GNUTLS_APPLICATION_DATA,
						     -1, EPOCH_WRITE_CURRENT, data,
						     data_size, pad, MBUFFER_FLUSH);
		case RECORD_SEND_CORKED:
		case RECORD_SEND_CORKED_TO_KU:
			return append_data_to_corked(session, data, data_size);
		case RECORD_SEND_KEY_UPDATE_1:
			_gnutls_buffer_reset(&session->internals.record_key_update_buffer);

			ret = _gnutls_buffer_append_data(&session->internals.record_key_update_buffer,
							 data, data_size);
			if (ret < 0)
				return gnutls_assert_val(ret);

			session->internals.rsend_state = RECORD_SEND_KEY_UPDATE_2;
			FALLTHROUGH;
		case RECORD_SEND_KEY_UPDATE_2:
			ret = gnutls_session_key_update(session, 0);
			if (ret < 0)
				return gnutls_assert_val(ret);

			session->internals.rsend_state = RECORD_SEND_KEY_UPDATE_3;
			FALLTHROUGH;
		case RECORD_SEND_KEY_UPDATE_3:
			ret = _gnutls_send_int(session, GNUTLS_APPLICATION_DATA,
						-1, EPOCH_WRITE_CURRENT,
						session->internals.record_key_update_buffer.data,
						session->internals.record_key_update_buffer.length,
						MBUFFER_FLUSH);
			_gnutls_buffer_clear(&session->internals.record_key_update_buffer);
			session->internals.rsend_state = RECORD_SEND_NORMAL;
			if (ret < 0)
				gnutls_assert();

			return ret;
		default:
			return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);
	}
}

/**
 * gnutls_record_send_early_data:
 * @session: is a #gnutls_session_t type.
 * @data: contains the data to send
 * @data_size: is the length of the data
 *
 * This function can be used by a client to send data early in the
 * handshake processes when resuming a session.  This is used to
 * implement a zero-roundtrip (0-RTT) mode.  It has the same semantics
 * as gnutls_record_send().
 *
 * There may be a limit to the amount of data sent as early data.  Use
 * gnutls_record_get_max_early_data_size() to check the limit.  If the
 * limit exceeds, this function returns
 * %GNUTLS_E_RECORD_LIMIT_REACHED.
 *
 * Returns: The number of bytes sent, or a negative error code.  The
 *   number of bytes sent might be less than @data_size.  The maximum
 *   number of bytes this function can send in a single call depends
 *   on the negotiated maximum record size.
 *
 * Since: 3.6.5
 **/
ssize_t gnutls_record_send_early_data(gnutls_session_t session,
				      const void *data,
				      size_t data_size)
{
	int ret;

	if (session->security_parameters.entity != GNUTLS_CLIENT)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	if (xsum(session->internals.
		 early_data_presend_buffer.length,
		 data_size) >
	    session->security_parameters.max_early_data_size)
		return gnutls_assert_val(GNUTLS_E_RECORD_LIMIT_REACHED);

	ret =
	    _gnutls_buffer_append_data(&session->internals.
				       early_data_presend_buffer, data,
				       data_size);
	if (ret < 0)
		return gnutls_assert_val(ret);

	return ret;
}

/**
 * gnutls_record_recv_early_data:
 * @session: is a #gnutls_session_t type.
 * @data: the buffer that the data will be read into
 * @data_size: the number of requested bytes
 *
 * This function can be used by a searver to retrieve data sent early
 * in the handshake processes when resuming a session.  This is used
 * to implement a zero-roundtrip (0-RTT) mode.  It has the same
 * semantics as gnutls_record_recv().
 *
 * This function can be called either in a handshake hook, or after
 * the handshake is complete.
 *
 * Returns: The number of bytes received and zero when early data
 * reading is complete.  A negative error code is returned in case of
 * an error.  If no early data is received during the handshake, this
 * function returns %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE.  The
 * number of bytes received might be less than the requested
 * @data_size.
 *
 * Since: 3.6.5
 **/
ssize_t
gnutls_record_recv_early_data(gnutls_session_t session, void *data, size_t data_size)
{
	mbuffer_st *bufel;
	gnutls_datum_t msg;
	size_t length;

	if (session->security_parameters.entity != GNUTLS_SERVER)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	bufel = _mbuffer_head_get_first(&session->internals.early_data_recv_buffer,
					&msg);
	if (bufel == NULL)
		return
		    gnutls_assert_val
		    (GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE);

	length = MIN(msg.size, data_size);
	memcpy(data, msg.data, length);
	_mbuffer_head_remove_bytes(&session->internals.early_data_recv_buffer,
				   length);

	return length;
}

/**
 * gnutls_record_cork:
 * @session: is a #gnutls_session_t type.
 *
 * If called, gnutls_record_send() will no longer send any records.
 * Any sent records will be cached until gnutls_record_uncork() is called.
 *
 * This function is safe to use with DTLS after GnuTLS 3.3.0.
 *
 * Since: 3.1.9
 **/
void gnutls_record_cork(gnutls_session_t session)
{
	session->internals.rsend_state = RECORD_SEND_CORKED;
}

/**
 * gnutls_record_uncork:
 * @session: is a #gnutls_session_t type.
 * @flags: Could be zero or %GNUTLS_RECORD_WAIT
 *
 * This resets the effect of gnutls_record_cork(), and flushes any pending
 * data. If the %GNUTLS_RECORD_WAIT flag is specified then this
 * function will block until the data is sent or a fatal error
 * occurs (i.e., the function will retry on %GNUTLS_E_AGAIN and
 * %GNUTLS_E_INTERRUPTED).
 *
 * If the flag %GNUTLS_RECORD_WAIT is not specified and the function
 * is interrupted then the %GNUTLS_E_AGAIN or %GNUTLS_E_INTERRUPTED
 * errors will be returned. To obtain the data left in the corked
 * buffer use gnutls_record_check_corked().
 *
 * Returns: On success the number of transmitted data is returned, or 
 * otherwise a negative error code. 
 *
 * Since: 3.1.9
 **/
int gnutls_record_uncork(gnutls_session_t session, unsigned int flags)
{
	int ret;
	ssize_t total = 0;
	record_send_state_t orig_state = session->internals.rsend_state;

	if (orig_state == RECORD_SEND_CORKED)
		session->internals.rsend_state = RECORD_SEND_NORMAL;
	else if (orig_state == RECORD_SEND_CORKED_TO_KU)
		session->internals.rsend_state = RECORD_SEND_KEY_UPDATE_1;
	else
		return 0;	/* nothing to be done */

	while (session->internals.record_presend_buffer.length > 0) {
		if (flags == GNUTLS_RECORD_WAIT) {
			do {
				ret =
				    gnutls_record_send(session,
						       session->internals.
						       record_presend_buffer.
						       data,
						       session->internals.
						       record_presend_buffer.
						       length);
			}
			while (ret < 0 && (ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED));
		} else {
			ret =
			    gnutls_record_send(session,
					       session->internals.
					       record_presend_buffer.data,
					       session->internals.
					       record_presend_buffer.
					       length);
		}
		if (ret < 0)
			goto fail;

		session->internals.record_presend_buffer.data += ret;
		session->internals.record_presend_buffer.length -= ret;
		total += ret;
	}

	return total;

      fail:
	session->internals.rsend_state = orig_state;
	return ret;
}

/**
 * gnutls_record_recv:
 * @session: is a #gnutls_session_t type.
 * @data: the buffer that the data will be read into
 * @data_size: the number of requested bytes
 *
 * This function has the similar semantics with recv().  The only
 * difference is that it accepts a GnuTLS session, and uses different
 * error codes.
 * In the special case that the peer requests a renegotiation, the
 * caller will receive an error code of %GNUTLS_E_REHANDSHAKE.  In case
 * of a client, this message may be simply ignored, replied with an alert
 * %GNUTLS_A_NO_RENEGOTIATION, or replied with a new handshake,
 * depending on the client's will. A server receiving this error code
 * can only initiate a new handshake or terminate the session.
 *
 * If %EINTR is returned by the internal pull function (the default
 * is recv()) then %GNUTLS_E_INTERRUPTED will be returned.  If
 * %GNUTLS_E_INTERRUPTED or %GNUTLS_E_AGAIN is returned, you must
 * call this function again to get the data.  See also
 * gnutls_record_get_direction().
 *
 * Returns: The number of bytes received and zero on EOF (for stream
 * connections).  A negative error code is returned in case of an error.  
 * The number of bytes received might be less than the requested @data_size.
 **/
ssize_t
gnutls_record_recv(gnutls_session_t session, void *data, size_t data_size)
{
	if (unlikely(!session->internals.initial_negotiation_completed)) {
		/* this is to protect buggy applications from sending unencrypted
		 * data. We allow sending however, if we are in false start handshake
		 * state. */
		if (session->internals.recv_state != RECV_STATE_FALSE_START &&
		    session->internals.recv_state != RECV_STATE_EARLY_START)
			return gnutls_assert_val(GNUTLS_E_UNAVAILABLE_DURING_HANDSHAKE);
	}

	return _gnutls_recv_int(session, GNUTLS_APPLICATION_DATA,
				data, data_size, NULL,
				session->internals.record_timeout_ms);
}

/**
 * gnutls_record_recv_seq:
 * @session: is a #gnutls_session_t type.
 * @data: the buffer that the data will be read into
 * @data_size: the number of requested bytes
 * @seq: is the packet's 64-bit sequence number. Should have space for 8 bytes.
 *
 * This function is the same as gnutls_record_recv(), except that
 * it returns in addition to data, the sequence number of the data.
 * This is useful in DTLS where record packets might be received
 * out-of-order. The returned 8-byte sequence number is an
 * integer in big-endian format and should be
 * treated as a unique message identification. 
 *
 * Returns: The number of bytes received and zero on EOF.  A negative
 *   error code is returned in case of an error.  The number of bytes
 *   received might be less than @data_size.
 *
 * Since: 3.0
 **/
ssize_t
gnutls_record_recv_seq(gnutls_session_t session, void *data,
		       size_t data_size, unsigned char *seq)
{
	return _gnutls_recv_int(session, GNUTLS_APPLICATION_DATA,
				data, data_size, seq,
				session->internals.record_timeout_ms);
}

/**
 * gnutls_record_set_timeout:
 * @session: is a #gnutls_session_t type.
 * @ms: is a timeout value in milliseconds
 *
 * This function sets the receive timeout for the record layer
 * to the provided value. Use an @ms value of zero to disable
 * timeout (the default), or %GNUTLS_INDEFINITE_TIMEOUT, to
 * set an indefinite timeout.
 *
 * This function requires to set a pull timeout callback. See
 * gnutls_transport_set_pull_timeout_function().
 *
 * Since: 3.1.7
 **/
void gnutls_record_set_timeout(gnutls_session_t session, unsigned int ms)
{
	session->internals.record_timeout_ms = ms;
}
