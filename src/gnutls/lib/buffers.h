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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>
 *
 */

#ifndef GNUTLS_LIB_BUFFERS_H
#define GNUTLS_LIB_BUFFERS_H

#include "mbuffers.h"

#define MBUFFER_FLUSH 1

void
_gnutls_record_buffer_put(gnutls_session_t session,
			  content_type_t type, uint64_t seq,
			  mbuffer_st * bufel);

inline static int _gnutls_record_buffer_get_size(gnutls_session_t session)
{
	return session->internals.record_buffer.byte_length;
}

#define NO_TIMEOUT_FUNC_SET(session) unlikely(session->internals.pull_timeout_func == gnutls_system_recv_timeout \
	     && session->internals.pull_func != system_read)

/*-
 * record_check_unprocessed:
 * @session: is a #gnutls_session_t structure.
 *
 * This function checks if there are unprocessed data
 * in the gnutls record buffers. Those data might not
 * be complete records.
 *
 * Returns: Returns the size of the data or zero.
 -*/
inline static size_t record_check_unprocessed(gnutls_session_t session)
{
	return session->internals.record_recv_buffer.byte_length;
}

int _gnutls_record_buffer_get(content_type_t type,
			      gnutls_session_t session, uint8_t * data,
			      size_t length, uint8_t seq[8]);
int _gnutls_record_buffer_get_packet(content_type_t type,
				     gnutls_session_t session,
				     gnutls_packet_t *packet);
ssize_t _gnutls_io_read_buffered(gnutls_session_t, size_t n,
				 content_type_t, unsigned int *ms);
int _gnutls_io_clear_peeked_data(gnutls_session_t session);

ssize_t _gnutls_io_write_buffered(gnutls_session_t session,
				  mbuffer_st * bufel, unsigned int mflag);

int _gnutls_handshake_io_cache_int(gnutls_session_t,
				   gnutls_handshake_description_t,
				   mbuffer_st * bufel);

ssize_t
_gnutls_handshake_io_recv_int(gnutls_session_t session,
			      gnutls_handshake_description_t htype,
			      handshake_buffer_st * hsk,
			      unsigned int optional);

ssize_t _gnutls_io_write_flush(gnutls_session_t session);
int _gnutls_io_check_recv(gnutls_session_t session, unsigned int ms);
ssize_t _gnutls_handshake_io_write_flush(gnutls_session_t session);

inline static void _gnutls_handshake_buffer_clear(handshake_buffer_st *
						  hsk)
{
	_gnutls_buffer_clear(&hsk->data);
	hsk->htype = -1;
}

inline static void _gnutls_handshake_buffer_init(handshake_buffer_st * hsk)
{
	memset(hsk, 0, sizeof(*hsk));
	_gnutls_buffer_init(&hsk->data);
	hsk->htype = -1;
}

inline static void _gnutls_handshake_recv_buffer_clear(gnutls_session_t
						       session)
{
	int i;
	for (i = 0; i < session->internals.handshake_recv_buffer_size; i++)
		_gnutls_handshake_buffer_clear(&session->internals.
					       handshake_recv_buffer[i]);
	session->internals.handshake_recv_buffer_size = 0;
	_mbuffer_head_clear(&session->internals.handshake_header_recv_buffer);
}

inline static void _gnutls_handshake_recv_buffer_init(gnutls_session_t
						      session)
{
	int i;
	for (i = 0; i < MAX_HANDSHAKE_MSGS; i++) {
		_gnutls_handshake_buffer_init(&session->internals.
					      handshake_recv_buffer[i]);
	}
	session->internals.handshake_recv_buffer_size = 0;
	_mbuffer_head_init(&session->internals.handshake_header_recv_buffer);
}

int _gnutls_parse_record_buffered_msgs(gnutls_session_t session);

ssize_t
_gnutls_recv_in_buffers(gnutls_session_t session, content_type_t type,
			gnutls_handshake_description_t htype,
			unsigned int ms);

#define _gnutls_handshake_io_buffer_clear( session) \
	_mbuffer_head_clear( &session->internals.handshake_send_buffer); \
	_gnutls_handshake_recv_buffer_clear( session);

#endif /* GNUTLS_LIB_BUFFERS_H */
