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

int _gnutls_fbase64_encode(const char *msg, const uint8_t * data,
			   size_t data_size, gnutls_datum_t * result);
int _gnutls_fbase64_decode(const char *header, const uint8_t * data,
			   size_t data_size, gnutls_datum_t * result);

int
_gnutls_base64_decode(const uint8_t * data, size_t data_size,
		      gnutls_datum_t * result);

#define B64SIZE( data_size) ((data_size%3==0)?((data_size*4)/3):(4+((data_size/3)*4)))

/* The size for B64 encoding + newlines plus header
 */

#define B64FSIZE( hsize, dsize) \
	(B64SIZE(dsize) + (hsize) + /*newlines*/ \
	B64SIZE(dsize)/64 + (((B64SIZE(dsize) % 64) > 0) ? 1 : 0))
