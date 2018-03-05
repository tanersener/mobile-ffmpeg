/*
 * Copyright (C) 2000-2012 Free Software Foundation, Inc.
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

/* 
 * This file holds all the buffering code used in gnutls.
 * The buffering code works as:
 *
 * RECORD LAYER: 
 *  1. uses a buffer to hold data (application/handshake),
 *    we got but they were not requested, yet.
 *  (see gnutls_record_buffer_put(), gnutls_record_buffer_get_size() etc.)
 *
 *  2. uses a buffer to hold data that were incomplete (ie the read/write
 *    was interrupted)
 *  (see _gnutls_io_read_buffered(), _gnutls_io_write_buffered() etc.)
 * 
 * HANDSHAKE LAYER:
 *  1. Uses buffer to hold the last received handshake message.
 *  (see _gnutls_handshake_hash_buffer_put() etc.)
 *
 */

#include "gnutls_int.h"
#include "errors.h"
#include <num.h>
#include <record.h>
#include <buffers.h>
#include <mbuffers.h>
#include <state.h>
#include <dtls.h>
#include <system.h>
#include <constate.h>	/* gnutls_epoch_get */
#include <handshake.h>	/* remaining_time() */
#include <errno.h>
#include <system.h>
#include "debug.h"

#ifndef EAGAIN
#define EAGAIN EWOULDBLOCK
#endif

/* this is the maximum number of messages allowed to queue.
 */
#define MAX_QUEUE 32

/* Buffers received packets of type APPLICATION DATA,
 * HANDSHAKE DATA and HEARTBEAT.
 */
void
_gnutls_record_buffer_put(gnutls_session_t session,
			  content_type_t type, gnutls_uint64 * seq,
			  mbuffer_st * bufel)
{

	bufel->type = type;
	memcpy(&bufel->record_sequence, seq, sizeof(*seq));

	_mbuffer_enqueue(&session->internals.record_buffer, bufel);
	_gnutls_buffers_log("BUF[REC]: Inserted %d bytes of Data(%d)\n",
			    (int) bufel->msg.size, (int) type);

	return;
}

/**
 * gnutls_record_check_pending:
 * @session: is a #gnutls_session_t type.
 *
 * This function checks if there are unread data
 * in the gnutls buffers. If the return value is
 * non-zero the next call to gnutls_record_recv()
 * is guaranteed not to block.
 *
 * Returns: Returns the size of the data or zero.
 **/
size_t gnutls_record_check_pending(gnutls_session_t session)
{
	return _gnutls_record_buffer_get_size(session);
}

/**
 * gnutls_record_check_corked:
 * @session: is a #gnutls_session_t type.
 *
 * This function checks if there pending corked
 * data in the gnutls buffers --see gnutls_record_cork(). 
 *
 * Returns: Returns the size of the corked data or zero.
 *
 * Since: 3.2.8
 **/
size_t gnutls_record_check_corked(gnutls_session_t session)
{
	return session->internals.record_presend_buffer.length;
}

int
_gnutls_record_buffer_get(content_type_t type,
			  gnutls_session_t session, uint8_t * data,
			  size_t length, uint8_t seq[8])
{
	gnutls_datum_t msg;
	mbuffer_st *bufel;

	if (length == 0 || data == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	bufel =
	    _mbuffer_head_get_first(&session->internals.record_buffer,
				    &msg);
	if (bufel == NULL)
		return
		    gnutls_assert_val
		    (GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE);

	if (type != bufel->type) {
		if (IS_DTLS(session))
			_gnutls_audit_log(session,
					  "Discarded unexpected %s (%d) packet (expecting: %s (%d))\n",
					  _gnutls_packet2str(bufel->type),
					  (int) bufel->type,
					  _gnutls_packet2str(type),
					  (int) type);
		else
			_gnutls_debug_log("received unexpected packet: %s(%d)\n",
						_gnutls_packet2str(bufel->type), (int)bufel->type);

		_mbuffer_head_remove_bytes(&session->internals.
					   record_buffer, msg.size);
		return gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET);
	}

	if (msg.size <= length)
		length = msg.size;

	if (seq)
		memcpy(seq, bufel->record_sequence.i, 8);

	memcpy(data, msg.data, length);
	_mbuffer_head_remove_bytes(&session->internals.record_buffer,
				   length);

	return length;
}

int
_gnutls_record_buffer_get_packet(content_type_t type, gnutls_session_t session, gnutls_packet_t *packet)
{
	mbuffer_st *bufel;

	bufel =
	    _mbuffer_head_pop_first(&session->internals.record_buffer);
	if (bufel == NULL)
		return
		    gnutls_assert_val
		    (GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE);

	if (type != bufel->type) {
		if (IS_DTLS(session))
			_gnutls_audit_log(session,
					  "Discarded unexpected %s (%d) packet (expecting: %s)\n",
					  _gnutls_packet2str(bufel->type),
					  (int) bufel->type,
					  _gnutls_packet2str(type));
		_mbuffer_head_remove_bytes(&session->internals.
					   record_buffer, bufel->msg.size);
		return gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET);
	}

	*packet = bufel;

	return bufel->msg.size - bufel->mark;
}

inline static void reset_errno(gnutls_session_t session)
{
	session->internals.errnum = 0;
}

inline static int get_errno(gnutls_session_t session)
{
	int ret;

	if (session->internals.errnum != 0)
		ret = session->internals.errnum;
	else
		ret =
		    session->internals.errno_func(session->internals.
						  transport_recv_ptr);
	return ret;
}

inline static
int errno_to_gerr(int err, unsigned dtls)
{
	switch (err) {
	case EAGAIN:
		return GNUTLS_E_AGAIN;
	case EINTR:
		return GNUTLS_E_INTERRUPTED;
	case EMSGSIZE:
		if (dtls != 0)
			return GNUTLS_E_LARGE_PACKET;
		else
			return GNUTLS_E_PUSH_ERROR;
	default:
		gnutls_assert();
		return GNUTLS_E_PUSH_ERROR;
	}
}

static ssize_t
_gnutls_dgram_read(gnutls_session_t session, mbuffer_st ** bufel,
		   gnutls_pull_func pull_func, unsigned int *ms)
{
	ssize_t i, ret;
	uint8_t *ptr;
	struct timespec t1, t2;
	size_t max_size, recv_size;
	gnutls_transport_ptr_t fd = session->internals.transport_recv_ptr;
	unsigned int diff;

	max_size = max_record_recv_size(session);
	recv_size = max_size;

	session->internals.direction = 0;

	if (ms && *ms > 0) {
		ret = _gnutls_io_check_recv(session, *ms);
		if (ret < 0)
			return gnutls_assert_val(ret);
		gettime(&t1);
	}

	*bufel = _mbuffer_alloc_align16(max_size, get_total_headers(session));
	if (*bufel == NULL)
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

	ptr = (*bufel)->msg.data;

	reset_errno(session);
	i = pull_func(fd, ptr, recv_size);

	if (i < 0) {
		int err = get_errno(session);

		_gnutls_read_log("READ: %d returned from %p, errno=%d\n",
				 (int) i, fd, err);

		ret = errno_to_gerr(err, 1);
		goto cleanup;
	} else {
		_gnutls_read_log("READ: Got %d bytes from %p\n", (int) i,
				 fd);
		if (i == 0) {
			/* If we get here, we likely have a stream socket.
			 * FIXME: this probably breaks DCCP. */
			gnutls_assert();
			ret = 0;
			goto cleanup;
		}

		_mbuffer_set_udata_size(*bufel, i);
	}

	if (ms && *ms > 0) {
		gettime(&t2);
		diff = timespec_sub_ms(&t2, &t1);
		if (diff < *ms)
			*ms -= diff;
		else {
			ret = gnutls_assert_val(GNUTLS_E_TIMEDOUT);
			goto cleanup;
		}
	}

	_gnutls_read_log("READ: read %d bytes from %p\n", (int) i, fd);

	return i;

      cleanup:
	_mbuffer_xfree(bufel);
	return ret;
}

static ssize_t
_gnutls_stream_read(gnutls_session_t session, mbuffer_st ** bufel,
		    size_t size, gnutls_pull_func pull_func,
		    unsigned int *ms)
{
	size_t left;
	ssize_t i = 0;
	size_t max_size = max_record_recv_size(session);
	uint8_t *ptr;
	gnutls_transport_ptr_t fd = session->internals.transport_recv_ptr;
	int ret;
	struct timespec t1, t2;
	unsigned int diff;

	session->internals.direction = 0;

	*bufel = _mbuffer_alloc_align16(MAX(max_size, size), get_total_headers(session));
	if (!*bufel) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}
	ptr = (*bufel)->msg.data;

	left = size;
	while (left > 0) {
		if (ms && *ms > 0) {
			ret = _gnutls_io_check_recv(session, *ms);
			if (ret < 0) {
				gnutls_assert();
				goto cleanup;
			}

			gettime(&t1);
		}

		reset_errno(session);

		i = pull_func(fd, &ptr[size - left], left);

		if (i < 0) {
			int err = get_errno(session);

			_gnutls_read_log
			    ("READ: %d returned from %p, errno=%d gerrno=%d\n",
			     (int) i, fd, errno,
			     session->internals.errnum);

			if (err == EAGAIN || err == EINTR) {
				if (size - left > 0) {

					_gnutls_read_log
					    ("READ: returning %d bytes from %p\n",
					     (int) (size - left), fd);

					goto finish;
				}

				ret = errno_to_gerr(err, 0);
				goto cleanup;
			} else {
				gnutls_assert();
				ret = GNUTLS_E_PULL_ERROR;
				goto cleanup;
			}
		} else {

			_gnutls_read_log("READ: Got %d bytes from %p\n",
					 (int) i, fd);

			if (i == 0)
				break;	/* EOF */
		}

		left -= i;
		(*bufel)->msg.size += i;

		if (ms && *ms > 0 && *ms != GNUTLS_INDEFINITE_TIMEOUT) {
			gettime(&t2);
			diff = timespec_sub_ms(&t2, &t1);
			if (diff < *ms)
				*ms -= diff;
			else {
				ret = gnutls_assert_val(GNUTLS_E_TIMEDOUT);
				goto cleanup;
			}
		}
	}

      finish:

	_gnutls_read_log("READ: read %d bytes from %p\n",
			 (int) (size - left), fd);

	if (size - left == 0)
		_mbuffer_xfree(bufel);

	return (size - left);

      cleanup:
	_mbuffer_xfree(bufel);
	return ret;
}


/* This function is like read. But it does not return -1 on error.
 * It does return gnutls_errno instead.
 *
 * Flags are only used if the default recv() function is being used.
 */
static ssize_t
_gnutls_read(gnutls_session_t session, mbuffer_st ** bufel,
	     size_t size, gnutls_pull_func pull_func, unsigned int *ms)
{
	if (IS_DTLS(session))
		/* Size is not passed, since a whole datagram will be read. */
		return _gnutls_dgram_read(session, bufel, pull_func, ms);
	else
		return _gnutls_stream_read(session, bufel, size, pull_func,
					   ms);
}

/* @vec: if non-zero then the vector function will be used to
 *       push the data.
 */
static ssize_t
_gnutls_writev_emu(gnutls_session_t session, gnutls_transport_ptr_t fd,
		   const giovec_t * giovec, unsigned int giovec_cnt, unsigned vec)
{
	unsigned int j = 0;
	size_t total = 0;
	ssize_t ret = 0;

	for (j = 0; j < giovec_cnt; j++) {
		if (vec) {
			ret = session->internals.vec_push_func(fd, &giovec[j], 1);
		} else {
			size_t sent = 0;
			ssize_t left = giovec[j].iov_len;
			char *p = giovec[j].iov_base;
			do {
				ret =
				    session->internals.push_func(fd, p,
								 left);
				if (ret > 0) {
					sent += ret;
					left -= ret;
					p += ret;
				}
			} while(ret > 0 && left > 0);

			if (sent > 0)
				ret = sent;
		}

		if (ret == -1) {
			gnutls_assert();
			break;
		}

		total += ret;

		if ((size_t) ret != giovec[j].iov_len)
			break;
	}

	if (total > 0)
		return total;

	return ret;
}

/* @total: The sum of the data in giovec
 */
static ssize_t
_gnutls_writev(gnutls_session_t session, const giovec_t * giovec,
	       unsigned giovec_cnt, unsigned total)
{
	int i;
	bool is_dtls = IS_DTLS(session);
	unsigned no_writev = 0;
	gnutls_transport_ptr_t fd = session->internals.transport_send_ptr;

	reset_errno(session);

	if (session->internals.vec_push_func != NULL) {
		if (is_dtls && giovec_cnt > 1) {
			if (total > session->internals.dtls.mtu) {
				no_writev = 1;
			}
		}

		if (no_writev == 0) {
			i = session->internals.vec_push_func(fd, giovec, giovec_cnt);
		} else {
			i = _gnutls_writev_emu(session, fd, giovec, giovec_cnt, 1);
		}
	} else if (session->internals.push_func != NULL) {
		i = _gnutls_writev_emu(session, fd, giovec, giovec_cnt, 0);
	} else
		return gnutls_assert_val(GNUTLS_E_PUSH_ERROR);

	if (i == -1) {
		int err = get_errno(session);
		_gnutls_debug_log("WRITE: %d returned from %p, errno: %d\n",
				  i, fd, err);

		return errno_to_gerr(err, is_dtls);
	}
	return i;
}

/* 
 * @ms: a pointer to the number of milliseconds to wait for data. Use zero or NULL for indefinite.
 *
 * This function is like recv(with MSG_PEEK). But it does not return -1 on error.
 * It does return gnutls_errno instead.
 * This function reads data from the socket and keeps them in a buffer, of up to
 * max_record_recv_size. 
 *
 * This is not a general purpose function. It returns EXACTLY the data requested,
 * which are stored in a local (in the session) buffer.
 *
 * If the @ms parameter is non zero then this function will return before
 * the given amount of milliseconds or return GNUTLS_E_TIMEDOUT.
 *
 */
ssize_t
_gnutls_io_read_buffered(gnutls_session_t session, size_t total,
			 content_type_t recv_type, unsigned int *ms)
{
	ssize_t ret;
	size_t min;
	mbuffer_st *bufel = NULL;
	size_t recvdata, readsize;

	if (total > max_record_recv_size(session) || total == 0) {
		gnutls_assert();	/* internal error */
		return GNUTLS_E_INVALID_REQUEST;
	}

	/* calculate the actual size, ie. get the minimum of the
	 * buffered data and the requested data.
	 */
	min =
	    MIN(session->internals.record_recv_buffer.byte_length, total);
	if (min > 0) {
		/* if we have enough buffered data
		 * then just return them.
		 */
		if (min == total) {
			return min;
		}
	}

	/* min is over zero. recvdata is the data we must
	 * receive in order to return the requested data.
	 */
	recvdata = total - min;
	readsize = recvdata;

	/* Check if the previously read data plus the new data to
	 * receive are longer than the maximum receive buffer size.
	 */
	if ((session->internals.record_recv_buffer.byte_length +
	     recvdata) > max_record_recv_size(session)) {
		gnutls_assert();	/* internal error */
		return GNUTLS_E_INVALID_REQUEST;
	}

	/* READ DATA
	 */
	if (readsize > 0) {
		ret =
		    _gnutls_read(session, &bufel, readsize,
				 session->internals.pull_func, ms);

		/* return immediately if we got an interrupt or eagain
		 * error.
		 */
		if (ret < 0) {
			return gnutls_assert_val(ret);
		}

		if (ret == 0)	/* EOF */
			return gnutls_assert_val(0);

		/* copy fresh data to our buffer.
		 */
		_gnutls_read_log
		    ("RB: Have %d bytes into buffer. Adding %d bytes.\n",
		     (int) session->internals.record_recv_buffer.
		     byte_length, (int) ret);
		_gnutls_read_log("RB: Requested %d bytes\n", (int) total);

		_mbuffer_enqueue(&session->internals.record_recv_buffer,
				 bufel);

		if (IS_DTLS(session))
			ret =
			    MIN(total,
				session->internals.record_recv_buffer.
				byte_length);
		else
			ret =
			    session->internals.record_recv_buffer.
			    byte_length;

		if ((ret > 0) && ((size_t) ret < total))	/* Short Read */
			return gnutls_assert_val(GNUTLS_E_AGAIN);
		else
			return ret;
	} else
		return gnutls_assert_val(0);
}

/* This function is like write. But it does not return -1 on error.
 * It does return gnutls_errno instead.
 *
 * This function takes full responsibility of freeing msg->data.
 *
 * In case of E_AGAIN and E_INTERRUPTED errors, you must call
 * gnutls_write_flush(), until it returns ok (0).
 *
 * We need to push exactly the data in msg->size, since we cannot send
 * less data. In TLS the peer must receive the whole packet in order
 * to decrypt and verify the integrity.
 *
 */
ssize_t
_gnutls_io_write_buffered(gnutls_session_t session,
			  mbuffer_st * bufel, unsigned int mflag)
{
	mbuffer_head_st *const send_buffer =
	    &session->internals.record_send_buffer;

	/* to know where the procedure was interrupted.
	 */
	session->internals.direction = 1;

	_mbuffer_enqueue(send_buffer, bufel);

	_gnutls_write_log
	    ("WRITE: enqueued %d bytes for %p. Total %d bytes.\n",
	     (int) bufel->msg.size, session->internals.transport_recv_ptr,
	     (int) send_buffer->byte_length);

	if (mflag == MBUFFER_FLUSH)
		return _gnutls_io_write_flush(session);
	else
		return bufel->msg.size;
}

typedef ssize_t(*send_func) (gnutls_session_t, const giovec_t *, int);

/* This function writes the data that are left in the
 * TLS write buffer (ie. because the previous write was
 * interrupted.
 */
ssize_t _gnutls_io_write_flush(gnutls_session_t session)
{
	gnutls_datum_t msg;
	mbuffer_head_st *send_buffer =
	    &session->internals.record_send_buffer;
	int ret;
	ssize_t sent = 0, tosend = 0;
	giovec_t iovec[MAX_QUEUE];
	int i = 0;
	mbuffer_st *cur;

	session->internals.direction = 1;
	_gnutls_write_log("WRITE FLUSH: %d bytes in buffer.\n",
			  (int) send_buffer->byte_length);

	for (cur = _mbuffer_head_get_first(send_buffer, &msg);
	     cur != NULL; cur = _mbuffer_head_get_next(cur, &msg)) {
		iovec[i].iov_base = msg.data;
		iovec[i++].iov_len = msg.size;
		tosend += msg.size;

		/* we buffer up to MAX_QUEUE messages */
		if (i >= MAX_QUEUE) {
			gnutls_assert();
			return GNUTLS_E_INTERNAL_ERROR;
		}
	}

	if (tosend == 0) {
		gnutls_assert();
		return 0;
	}

	ret = _gnutls_writev(session, iovec, i, tosend);
	if (ret >= 0) {
		_mbuffer_head_remove_bytes(send_buffer, ret);
		_gnutls_write_log
		    ("WRITE: wrote %d bytes, %d bytes left.\n", ret,
		     (int) send_buffer->byte_length);

		sent += ret;
	} else if (ret == GNUTLS_E_INTERRUPTED || ret == GNUTLS_E_AGAIN) {
		_gnutls_write_log("WRITE interrupted: %d bytes left.\n",
				  (int) send_buffer->byte_length);
		return ret;
	} else if (ret == GNUTLS_E_LARGE_PACKET) {
		_mbuffer_head_remove_bytes(send_buffer, tosend);
		_gnutls_write_log
		    ("WRITE cannot send large packet (%u bytes).\n",
		     (unsigned int) tosend);
		return ret;
	} else {
		_gnutls_write_log("WRITE error: code %d, %d bytes left.\n",
				  ret, (int) send_buffer->byte_length);

		gnutls_assert();
		return ret;
	}

	if (sent < tosend) {
		return gnutls_assert_val(GNUTLS_E_AGAIN);
	}

	return sent;
}

/* Checks whether there are received data within
 * a timeframe.
 *
 * Returns 0 if data were received, GNUTLS_E_TIMEDOUT
 * on timeout and a negative error code on error.
 */
int _gnutls_io_check_recv(gnutls_session_t session, unsigned int ms)
{
	gnutls_transport_ptr_t fd = session->internals.transport_recv_ptr;
	int ret = 0, err;

	if (unlikely
	    (session->internals.pull_timeout_func == gnutls_system_recv_timeout
	     && session->internals.pull_func != system_read)) {
		_gnutls_debug_log("The pull function has been replaced but not the pull timeout.");
		return gnutls_assert_val(GNUTLS_E_PULL_ERROR);
	}

	reset_errno(session);

	ret = session->internals.pull_timeout_func(fd, ms);
	if (ret == -1) {
		err = get_errno(session);
		_gnutls_read_log
		    ("READ_TIMEOUT: %d returned from %p, errno=%d (timeout: %u)\n",
		     (int) ret, fd, err, ms);
		return errno_to_gerr(err, IS_DTLS(session));
	}

	if (ret > 0)
		return 0;
	else
		return GNUTLS_E_TIMEDOUT;
}

/* HANDSHAKE buffers part 
 */

/* This function writes the data that are left in the
 * Handshake write buffer (ie. because the previous write was
 * interrupted.
 *
 */
ssize_t _gnutls_handshake_io_write_flush(gnutls_session_t session)
{
	mbuffer_head_st *const send_buffer =
	    &session->internals.handshake_send_buffer;
	gnutls_datum_t msg;
	int ret;
	uint16_t epoch;
	ssize_t total = 0;
	mbuffer_st *cur;

	_gnutls_write_log("HWRITE FLUSH: %d bytes in buffer.\n",
			  (int) send_buffer->byte_length);

	if (IS_DTLS(session))
		return _dtls_transmit(session);

	for (cur = _mbuffer_head_get_first(send_buffer, &msg);
	     cur != NULL; cur = _mbuffer_head_get_first(send_buffer, &msg))
	{
		epoch = cur->epoch;

		ret = _gnutls_send_int(session, cur->type,
				       cur->htype,
				       epoch, msg.data, msg.size, 0);

		if (ret >= 0) {
			total += ret;

			ret = _mbuffer_head_remove_bytes(send_buffer, ret);
			if (ret == 1)
				_gnutls_epoch_refcount_dec(session, epoch);

			_gnutls_write_log
			    ("HWRITE: wrote %d bytes, %d bytes left.\n",
			     ret, (int) send_buffer->byte_length);

		} else {
			_gnutls_write_log
			    ("HWRITE error: code %d, %d bytes left.\n",
			     ret, (int) send_buffer->byte_length);

			gnutls_assert();
			return ret;
		}
	}

	return _gnutls_io_write_flush(session);
}


/* This is a send function for the gnutls handshake 
 * protocol. Just makes sure that all data have been sent.
 *
 */
int
_gnutls_handshake_io_cache_int(gnutls_session_t session,
			       gnutls_handshake_description_t htype,
			       mbuffer_st * bufel)
{
	mbuffer_head_st *send_buffer;

	if (IS_DTLS(session)) {
		bufel->handshake_sequence =
		    session->internals.dtls.hsk_write_seq - 1;
	}

	send_buffer = &session->internals.handshake_send_buffer;

	bufel->epoch =
	    (uint16_t) _gnutls_epoch_refcount_inc(session,
						  EPOCH_WRITE_CURRENT);
	bufel->htype = htype;
	if (bufel->htype == GNUTLS_HANDSHAKE_CHANGE_CIPHER_SPEC)
		bufel->type = GNUTLS_CHANGE_CIPHER_SPEC;
	else
		bufel->type = GNUTLS_HANDSHAKE;

	_mbuffer_enqueue(send_buffer, bufel);

	_gnutls_write_log
	    ("HWRITE: enqueued [%s] %d. Total %d bytes.\n",
	     _gnutls_handshake2str(bufel->htype), (int) bufel->msg.size,
	     (int) send_buffer->byte_length);

	return 0;
}

static int handshake_compare(const void *_e1, const void *_e2)
{
	const handshake_buffer_st *e1 = _e1;
	const handshake_buffer_st *e2 = _e2;

	if (e1->sequence <= e2->sequence)
		return 1;
	else
		return -1;
}

#define SSL2_HEADERS 1
static int
parse_handshake_header(gnutls_session_t session, mbuffer_st * bufel,
		       handshake_buffer_st * hsk)
{
	uint8_t *dataptr = NULL;	/* for realloc */
	size_t handshake_header_size =
	    HANDSHAKE_HEADER_SIZE(session), data_size;

	/* Note: SSL2_HEADERS == 1 */
	if (_mbuffer_get_udata_size(bufel) < handshake_header_size)
		return
		    gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET_LENGTH);

	dataptr = _mbuffer_get_udata_ptr(bufel);

	/* if reading a client hello of SSLv2 */
#ifdef ENABLE_SSL2
	if (unlikely
	    (!IS_DTLS(session)
	     && bufel->htype == GNUTLS_HANDSHAKE_CLIENT_HELLO_V2)) {
		handshake_header_size = SSL2_HEADERS;	/* we've already read one byte */

		hsk->length = _mbuffer_get_udata_size(bufel) - handshake_header_size;	/* we've read the first byte */

		if (dataptr[0] != GNUTLS_HANDSHAKE_CLIENT_HELLO)
			return
			    gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET);

		hsk->htype = GNUTLS_HANDSHAKE_CLIENT_HELLO_V2;

		hsk->sequence = 0;
		hsk->start_offset = 0;
		hsk->end_offset = hsk->length;
	} else
#endif
	{		/* TLS or DTLS handshake headers */


		hsk->htype = dataptr[0];

		/* we do not use DECR_LEN because we know
		 * that the packet has enough data.
		 */
		hsk->length = _gnutls_read_uint24(&dataptr[1]);
		handshake_header_size = HANDSHAKE_HEADER_SIZE(session);

		if (IS_DTLS(session)) {
			hsk->sequence = _gnutls_read_uint16(&dataptr[4]);
			hsk->start_offset =
			    _gnutls_read_uint24(&dataptr[6]);
			hsk->end_offset =
			    hsk->start_offset +
			    _gnutls_read_uint24(&dataptr[9]);
		} else {
			hsk->sequence = 0;
			hsk->start_offset = 0;
			hsk->end_offset =
			    MIN((_mbuffer_get_udata_size(bufel) -
				 handshake_header_size), hsk->length);
		}
	}
	data_size = _mbuffer_get_udata_size(bufel) - handshake_header_size;

	/* make the length offset */
	if (hsk->end_offset > 0)
		hsk->end_offset--;

	_gnutls_handshake_log
	    ("HSK[%p]: %s (%u) was received. Length %d[%d], frag offset %d, frag length: %d, sequence: %d\n",
	     session, _gnutls_handshake2str(hsk->htype),
	     (unsigned) hsk->htype, (int) hsk->length, (int) data_size,
	     hsk->start_offset, hsk->end_offset - hsk->start_offset + 1,
	     (int) hsk->sequence);

	hsk->header_size = handshake_header_size;
	memcpy(hsk->header, _mbuffer_get_udata_ptr(bufel),
	       handshake_header_size);

	if (hsk->length > 0 &&
	    (hsk->end_offset - hsk->start_offset >= data_size))
		return
		    gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET_LENGTH);

	if (hsk->length > 0 && (hsk->start_offset > hsk->end_offset ||
				hsk->end_offset - hsk->start_offset >=
				data_size
				|| hsk->end_offset >= hsk->length)) {
		return
		    gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET_LENGTH);
	}
	else if (hsk->length == 0 && hsk->end_offset != 0
		 && hsk->start_offset != 0)
		return
		    gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET_LENGTH);

	return handshake_header_size;
}

static void _gnutls_handshake_buffer_move(handshake_buffer_st * dst,
					  handshake_buffer_st * src)
{
	memcpy(dst, src, sizeof(*dst));
	memset(src, 0, sizeof(*src));
	src->htype = -1;
}

/* will merge the given handshake_buffer_st to the handshake_recv_buffer
 * list. The given hsk packet will be released in any case (success or failure).
 * Only used in DTLS.
 */
static int merge_handshake_packet(gnutls_session_t session,
				  handshake_buffer_st * hsk)
{
	int exists = 0, i, pos = 0;
	int ret;

	for (i = 0; i < session->internals.handshake_recv_buffer_size; i++) {
		if (session->internals.handshake_recv_buffer[i].htype ==
		    hsk->htype) {
			exists = 1;
			pos = i;
			break;
		}
	}

	if (!exists)
		pos = session->internals.handshake_recv_buffer_size;

	if (pos >= MAX_HANDSHAKE_MSGS)
		return
		    gnutls_assert_val(GNUTLS_E_TOO_MANY_HANDSHAKE_PACKETS);

	if (!exists) {
		if (hsk->length > 0 && hsk->end_offset > 0
		    && hsk->end_offset - hsk->start_offset + 1 !=
		    hsk->length) {
			ret =
			    _gnutls_buffer_resize(&hsk->data, hsk->length);
			if (ret < 0)
				return gnutls_assert_val(ret);

			hsk->data.length = hsk->length;

			memmove(&hsk->data.data[hsk->start_offset],
				hsk->data.data,
				hsk->end_offset - hsk->start_offset + 1);
		}

		session->internals.handshake_recv_buffer_size++;

		/* rewrite headers to make them look as each packet came as a single fragment */
		_gnutls_write_uint24(hsk->length, &hsk->header[1]);
		_gnutls_write_uint24(0, &hsk->header[6]);
		_gnutls_write_uint24(hsk->length, &hsk->header[9]);

		_gnutls_handshake_buffer_move(&session->internals.
					      handshake_recv_buffer[pos],
					      hsk);

	} else {
		if (hsk->start_offset <
		    session->internals.handshake_recv_buffer[pos].
		    start_offset
		    && hsk->end_offset + 1 >=
		    session->internals.handshake_recv_buffer[pos].
		    start_offset) {
			memcpy(&session->internals.
			       handshake_recv_buffer[pos].data.data[hsk->
								    start_offset],
			       hsk->data.data, hsk->data.length);
			session->internals.handshake_recv_buffer[pos].
			    start_offset = hsk->start_offset;
			session->internals.handshake_recv_buffer[pos].
			    end_offset =
			    MIN(hsk->end_offset,
				session->internals.
				handshake_recv_buffer[pos].end_offset);
		} else if (hsk->end_offset >
			   session->internals.handshake_recv_buffer[pos].
			   end_offset
			   && hsk->start_offset <=
			   session->internals.handshake_recv_buffer[pos].
			   end_offset + 1) {
			memcpy(&session->internals.
			       handshake_recv_buffer[pos].data.data[hsk->
								    start_offset],
			       hsk->data.data, hsk->data.length);

			session->internals.handshake_recv_buffer[pos].
			    end_offset = hsk->end_offset;
			session->internals.handshake_recv_buffer[pos].
			    start_offset =
			    MIN(hsk->start_offset,
				session->internals.
				handshake_recv_buffer[pos].start_offset);
		}
		_gnutls_handshake_buffer_clear(hsk);
	}

	return 0;
}

/* returns non-zero on match and zero on mismatch
 */
inline static int cmp_hsk_types(gnutls_handshake_description_t expected,
				gnutls_handshake_description_t recvd)
{
#ifdef ENABLE_SSL2
	if (expected == GNUTLS_HANDSHAKE_CLIENT_HELLO
	     && recvd == GNUTLS_HANDSHAKE_CLIENT_HELLO_V2)
		return 1;
#endif
	if (expected != recvd)
		return 0;

	return 1;
}

#define LAST_ELEMENT (session->internals.handshake_recv_buffer_size-1)

/* returns the last stored handshake packet.
 */
static int get_last_packet(gnutls_session_t session,
			   gnutls_handshake_description_t htype,
			   handshake_buffer_st * hsk,
			   unsigned int optional)
{
	handshake_buffer_st *recv_buf =
	    session->internals.handshake_recv_buffer;

	if (IS_DTLS(session)) {
		if (session->internals.handshake_recv_buffer_size == 0 ||
		    (session->internals.dtls.hsk_read_seq !=
		     recv_buf[LAST_ELEMENT].sequence))
			goto timeout;

		if (htype != recv_buf[LAST_ELEMENT].htype) {
			if (optional == 0)
				_gnutls_audit_log(session,
						  "Received unexpected handshake message '%s' (%d). Expected '%s' (%d)\n",
						  _gnutls_handshake2str
						  (recv_buf[0].htype),
						  (int) recv_buf[0].htype,
						  _gnutls_handshake2str
						  (htype), (int) htype);

			return
			    gnutls_assert_val
			    (GNUTLS_E_UNEXPECTED_HANDSHAKE_PACKET);
		}

		else if ((recv_buf[LAST_ELEMENT].start_offset == 0 &&
			  recv_buf[LAST_ELEMENT].end_offset ==
			  recv_buf[LAST_ELEMENT].length - 1)
			 || recv_buf[LAST_ELEMENT].length == 0) {
			session->internals.dtls.hsk_read_seq++;
			_gnutls_handshake_buffer_move(hsk,
						      &recv_buf
						      [LAST_ELEMENT]);
			session->internals.handshake_recv_buffer_size--;
			return 0;
		} else {
			/* if we don't have a complete handshake message, but we
			 * have queued data waiting, try again to reconstruct the
			 * handshake packet, using the queued */
			if (recv_buf[LAST_ELEMENT].end_offset != recv_buf[LAST_ELEMENT].length - 1 &&
			    record_check_unprocessed(session) > 0)
				return gnutls_assert_val(GNUTLS_E_INT_CHECK_AGAIN);
			else
				goto timeout;
		}
	} else {		/* TLS */

		if (session->internals.handshake_recv_buffer_size > 0
		    && recv_buf[0].length == recv_buf[0].data.length) {
			if (cmp_hsk_types(htype, recv_buf[0].htype) == 0) {
				return
				    gnutls_assert_val
				    (GNUTLS_E_UNEXPECTED_HANDSHAKE_PACKET);
			}

			_gnutls_handshake_buffer_move(hsk, &recv_buf[0]);
			session->internals.handshake_recv_buffer_size--;
			return 0;
		} else
			return
			    gnutls_assert_val
			    (GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE);
	}

      timeout:
	RETURN_DTLS_EAGAIN_OR_TIMEOUT(session, 0);
}

/* This is a receive function for the gnutls handshake 
 * protocol. Makes sure that we have received all data.
 *
 * htype is the next handshake packet expected.
 */
int _gnutls_parse_record_buffered_msgs(gnutls_session_t session)
{
	gnutls_datum_t msg;
	mbuffer_st *bufel = NULL, *prev = NULL;
	int ret;
	size_t data_size;
	handshake_buffer_st *recv_buf =
	    session->internals.handshake_recv_buffer;

	bufel =
	    _mbuffer_head_get_first(&session->internals.record_buffer,
				    &msg);
	if (bufel == NULL)
		return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;

	if (!IS_DTLS(session)) {
		ssize_t remain, append, header_size;

		do {
			if (bufel->type != GNUTLS_HANDSHAKE)
				return
				    gnutls_assert_val
				    (GNUTLS_E_UNEXPECTED_PACKET);

			/* if we have a half received message then complete it.
			 */
			remain = recv_buf[0].length -
			    recv_buf[0].data.length;

			/* this is the rest of a previous message */
			if (session->internals.handshake_recv_buffer_size >
			    0 && recv_buf[0].length > 0 && remain > 0) {
				if ((ssize_t) msg.size <= remain)
					append = msg.size;
				else
					append = remain;

				ret =
				    _gnutls_buffer_append_data(&recv_buf
							       [0].data,
							       msg.data,
							       append);
				if (ret < 0)
					return gnutls_assert_val(ret);

				_mbuffer_head_remove_bytes(&session->
							   internals.
							   record_buffer,
							   append);
			} else {	/* received new message */

				ret =
				    parse_handshake_header(session, bufel,
							   &recv_buf[0]);
				if (ret < 0)
					return gnutls_assert_val(ret);

				header_size = ret;
				session->internals.
				    handshake_recv_buffer_size = 1;

				_mbuffer_set_uhead_size(bufel,
							header_size);

				data_size =
				    MIN(recv_buf[0].length,
					_mbuffer_get_udata_size(bufel));
				ret =
				    _gnutls_buffer_append_data(&recv_buf
							       [0].data,
							       _mbuffer_get_udata_ptr
							       (bufel),
							       data_size);
				if (ret < 0)
					return gnutls_assert_val(ret);
				_mbuffer_set_uhead_size(bufel, 0);
				_mbuffer_head_remove_bytes(&session->
							   internals.
							   record_buffer,
							   data_size +
							   header_size);
			}

			/* if packet is complete then return it
			 */
			if (recv_buf[0].length == recv_buf[0].data.length) {
				return 0;
			}
			bufel =
			    _mbuffer_head_get_first(&session->internals.
						    record_buffer, &msg);
		}
		while (bufel != NULL);

		/* if we are here it means that the received packets were not
		 * enough to complete the handshake packet.
		 */
		return gnutls_assert_val(GNUTLS_E_AGAIN);
	} else {		/* DTLS */

		handshake_buffer_st tmp;

		do {
			/* we now 
			 * 0. parse headers
			 * 1. insert to handshake_recv_buffer
			 * 2. sort handshake_recv_buffer on sequence numbers
			 * 3. return first packet if completed or GNUTLS_E_AGAIN.
			 */
			do {
				if (bufel->type != GNUTLS_HANDSHAKE) {
					gnutls_assert();
					goto next;	/* ignore packet */
				}

				_gnutls_handshake_buffer_init(&tmp);

				ret =
				    parse_handshake_header(session, bufel,
							   &tmp);
				if (ret < 0) {
					gnutls_assert();
					_gnutls_audit_log(session,
							  "Invalid handshake packet headers. Discarding.\n");
					break;
				}

				_mbuffer_consume(&session->internals.
						 record_buffer, bufel,
						 ret);

				data_size =
				    MIN(tmp.length,
					tmp.end_offset - tmp.start_offset +
					1);

				ret =
				    _gnutls_buffer_append_data(&tmp.data,
							       _mbuffer_get_udata_ptr
							       (bufel),
							       data_size);
				if (ret < 0)
					return gnutls_assert_val(ret);

				_mbuffer_consume(&session->internals.
						 record_buffer, bufel,
						 data_size);

				ret =
				    merge_handshake_packet(session, &tmp);
				if (ret < 0)
					return gnutls_assert_val(ret);

			}
			while (_mbuffer_get_udata_size(bufel) > 0);

			prev = bufel;
			bufel =
			    _mbuffer_dequeue(&session->internals.
					     record_buffer, bufel);

			_mbuffer_xfree(&prev);
			continue;

		      next:
			bufel = _mbuffer_head_get_next(bufel, NULL);
		}
		while (bufel != NULL);

		/* sort in descending order */
		if (session->internals.handshake_recv_buffer_size > 1)
			qsort(recv_buf,
			      session->internals.
			      handshake_recv_buffer_size,
			      sizeof(recv_buf[0]), handshake_compare);

		while (session->internals.handshake_recv_buffer_size > 0 &&
		       recv_buf[LAST_ELEMENT].sequence <
		       session->internals.dtls.hsk_read_seq) {
			_gnutls_audit_log(session,
					  "Discarded replayed handshake packet with sequence %d\n",
					  recv_buf[LAST_ELEMENT].sequence);
			_gnutls_handshake_buffer_clear(&recv_buf
						       [LAST_ELEMENT]);
			session->internals.handshake_recv_buffer_size--;
		}

		return 0;
	}
}

/* This is a receive function for the gnutls handshake 
 * protocol. Makes sure that we have received all data.
 */
ssize_t
_gnutls_handshake_io_recv_int(gnutls_session_t session,
			      gnutls_handshake_description_t htype,
			      handshake_buffer_st * hsk,
			      unsigned int optional)
{
	int ret;
	unsigned int tleft = 0;
	int retries = 7;

	ret = get_last_packet(session, htype, hsk, optional);
	if (ret != GNUTLS_E_AGAIN && ret != GNUTLS_E_INTERRUPTED &&
	    ret != GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE &&
	    ret != GNUTLS_E_INT_CHECK_AGAIN) {
		return gnutls_assert_val(ret);
	}

	/* try using the already existing records before
	 * trying to receive.
	 */
	ret = _gnutls_parse_record_buffered_msgs(session);

	if (ret == 0) {
		ret = get_last_packet(session, htype, hsk, optional);
	}

	if (IS_DTLS(session)) {
		if (ret >= 0)
			return ret;
	} else {
		if ((ret != GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE
		     && ret < 0) || ret >= 0)
			return gnutls_assert_val(ret);
	}

	if (htype != (gnutls_handshake_description_t) -1) {
		ret = handshake_remaining_time(session);
		if (ret < 0)
			return gnutls_assert_val(ret);
		tleft = ret;
	}

	do {
		/* if we don't have a complete message waiting for us, try 
		 * receiving more */
		ret =
		    _gnutls_recv_in_buffers(session, GNUTLS_HANDSHAKE, htype,
					    tleft);
		if (ret < 0)
			return gnutls_assert_val_fatal(ret);

		ret = _gnutls_parse_record_buffered_msgs(session);
		if (ret == 0) {
			ret = get_last_packet(session, htype, hsk, optional);
		}
		/* we put an upper limit (retries) to the number of partial handshake
		 * messages in a record packet. */
	} while(IS_DTLS(session) && ret == GNUTLS_E_INT_CHECK_AGAIN && retries-- > 0);

	if (unlikely(IS_DTLS(session) && ret == GNUTLS_E_INT_CHECK_AGAIN)) {
		ret = gnutls_assert_val(GNUTLS_E_TOO_MANY_HANDSHAKE_PACKETS);
	}

	return ret;
}
