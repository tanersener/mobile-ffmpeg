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

#ifndef GNUTLS_RECORD_H
#define GNUTLS_RECORD_H

#include <gnutls/gnutls.h>
#include <buffers.h>
#include <constate.h>

ssize_t _gnutls_send_tlen_int(gnutls_session_t session,
			      content_type_t type,
			      gnutls_handshake_description_t htype,
			      unsigned int epoch_rel, const void *data,
			      size_t sizeofdata, size_t min_pad,
			      unsigned int mflags);

inline static ssize_t
_gnutls_send_int(gnutls_session_t session, content_type_t type,
		 gnutls_handshake_description_t htype,
		 unsigned int epoch_rel, const void *_data,
		 size_t data_size, unsigned int mflags)
{
	return _gnutls_send_tlen_int(session, type, htype, epoch_rel,
				     _data, data_size, 0, mflags);
}

ssize_t _gnutls_recv_int(gnutls_session_t session, content_type_t type,
			 uint8_t * data,
			 size_t sizeofdata, void *seq, unsigned int ms);

inline static unsigned max_record_recv_size(gnutls_session_t session)
{
	unsigned size;

	size = MAX_CIPHER_BLOCK_SIZE /*iv*/ + MAX_PAD_SIZE + MAX_HASH_SIZE/*MAC*/;
	
	if (gnutls_compression_get(session)!=GNUTLS_COMP_NULL || session->internals.priorities.allow_large_records != 0)
		size += EXTRA_COMP_SIZE;

	size += session->security_parameters.max_record_recv_size + RECORD_HEADER_SIZE(session);

	return size;
}

inline static unsigned max_decrypted_size(gnutls_session_t session)
{
	unsigned size = 0;

	if (session->internals.priorities.allow_large_records != 0)
		size += EXTRA_COMP_SIZE;

	size += session->security_parameters.max_record_recv_size;

	return size;
}

/* Returns the headers + any IV that the ciphersuite
 * requires */
inline static
unsigned int get_total_headers(gnutls_session_t session)
{
	int ret;
	record_parameters_st *params;
	unsigned total = RECORD_HEADER_SIZE(session);

	ret = _gnutls_epoch_get(session, EPOCH_WRITE_CURRENT, &params);
	if (ret < 0) {
		return total;
	}
	
	return total + _gnutls_cipher_get_explicit_iv_size(params->cipher);
}

inline static
unsigned int get_total_headers2(gnutls_session_t session, record_parameters_st *params)
{
	unsigned total = RECORD_HEADER_SIZE(session);

	return total + _gnutls_cipher_get_explicit_iv_size(params->cipher);
}

inline static void session_invalidate(gnutls_session_t session)
{
	session->internals.invalid_connection = 1;
}

#endif
