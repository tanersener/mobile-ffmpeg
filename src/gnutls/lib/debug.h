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

#ifndef GNUTLS_LIB_DEBUG_H
#define GNUTLS_LIB_DEBUG_H

const char *_gnutls_packet2str(content_type_t packet);
inline static const char *_gnutls_handshake2str(unsigned x)
{
	const char *s = gnutls_handshake_description_get_name(x);
	if (s == NULL)
		return "Unknown Handshake packet";
	else
		return s;
}

void _gnutls_dump_mpi(const char *prefix, bigint_t a);
void _gnutls_dump_vector(const char *prefix, const uint8_t * a,
			 size_t a_size);

#endif /* GNUTLS_LIB_DEBUG_H */
