/*
 * Copyright (C) 2009-2012 Free Software Foundation, Inc.
 *
 * Author: Jonathan Bastien-Filiatrault
 *
 * This file is part of GNUTLS.
 *
 * The GNUTLS library is free software; you can redistribute it and/or
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

#ifndef GNUTLS_MBUFFERS_H
#define GNUTLS_MBUFFERS_H

#include "gnutls_int.h"
#include "errors.h"

void _mbuffer_head_init(mbuffer_head_st * buf);
void _mbuffer_head_clear(mbuffer_head_st * buf);
void _mbuffer_enqueue(mbuffer_head_st * buf, mbuffer_st * bufel);
mbuffer_st *_mbuffer_dequeue(mbuffer_head_st * buf, mbuffer_st * bufel);
int _mbuffer_head_remove_bytes(mbuffer_head_st * buf, size_t bytes);
mbuffer_st *_mbuffer_alloc(size_t maximum_size);
int _mbuffer_linearize(mbuffer_head_st * buf);

mbuffer_st *_mbuffer_head_get_first(mbuffer_head_st * buf,
				    gnutls_datum_t * msg);
mbuffer_st *_mbuffer_head_get_next(mbuffer_st * cur, gnutls_datum_t * msg);

mbuffer_st *_mbuffer_head_pop_first(mbuffer_head_st * buf);

/* This is dangerous since it will replace bufel with a new
 * one.
 */
int _mbuffer_append_data(mbuffer_st * bufel, void *newdata,
			 size_t newdata_size);


/* For "user" use. One can have buffer data and header.
 */

inline static void *_mbuffer_get_uhead_ptr(mbuffer_st * bufel)
{
	return bufel->msg.data + bufel->mark;
}

inline static void *_mbuffer_get_udata_ptr(mbuffer_st * bufel)
{
	return bufel->msg.data + bufel->uhead_mark + bufel->mark;
}

inline static void _mbuffer_set_udata_size(mbuffer_st * bufel, size_t size)
{
	bufel->msg.size = size + bufel->uhead_mark + bufel->mark;
}

inline static void
_mbuffer_set_udata(mbuffer_st * bufel, void *data, size_t data_size)
{
	memcpy(_mbuffer_get_udata_ptr(bufel), data,
	       data_size);
	_mbuffer_set_udata_size(bufel, data_size);
}

inline static size_t _mbuffer_get_udata_size(mbuffer_st * bufel)
{
	return bufel->msg.size - bufel->uhead_mark - bufel->mark;
}

/* discards size bytes from the begging of the buffer */
inline static void
_mbuffer_consume(mbuffer_head_st * buf, mbuffer_st * bufel, size_t size)
{
	bufel->uhead_mark = 0;
	if (bufel->mark + size < bufel->msg.size)
		bufel->mark += size;
	else
		bufel->mark = bufel->msg.size;

	buf->byte_length -= size;
}

inline static size_t _mbuffer_get_uhead_size(mbuffer_st * bufel)
{
	return bufel->uhead_mark;
}

inline static void _mbuffer_set_uhead_size(mbuffer_st * bufel, size_t size)
{
	bufel->uhead_mark = size;
}



inline static mbuffer_st *_gnutls_handshake_alloc(gnutls_session_t session,
						  size_t maximum)
{
	mbuffer_st *bufel =
	    _mbuffer_alloc(HANDSHAKE_HEADER_SIZE(session) + maximum);

	if (!bufel)
		return NULL;

	_mbuffer_set_uhead_size(bufel, HANDSHAKE_HEADER_SIZE(session));
	_mbuffer_set_udata_size(bufel, maximum);

	return bufel;
}

/* Free a segment, if the pointer is not NULL
 *
 * We take a ** to detect and fix double free bugs (the dangling
 * pointer case). It also makes sure the pointer has a known value
 * after freeing.
 */
inline static void _mbuffer_xfree(mbuffer_st ** bufel)
{
	if (*bufel)
		gnutls_free(*bufel);

	*bufel = NULL;
}

#ifdef ENABLE_ALIGN16
mbuffer_st *_mbuffer_alloc_align16(size_t maximum_size, unsigned align_pos);
int _mbuffer_linearize_align16(mbuffer_head_st * buf, unsigned align_pos);
#else
# define _mbuffer_alloc_align16(x,y) _mbuffer_alloc(x)
# define _mbuffer_linearize_align16(x,y) _mbuffer_linearize(x)
#endif

#endif
