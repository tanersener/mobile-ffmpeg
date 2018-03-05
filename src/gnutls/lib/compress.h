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
#ifndef GNUTLS_COMPRESS_H
#define GNUTLS_COMPRESS_H

/* Algorithm handling. */
int _gnutls_supported_compression_methods(gnutls_session_t session,
					  uint8_t * comp, size_t max_comp);
int _gnutls_compression_is_ok(gnutls_compression_method_t algorithm);
int _gnutls_compression_get_num(gnutls_compression_method_t algorithm);
gnutls_compression_method_t _gnutls_compression_get_id(int num);

#ifdef HAVE_LIBZ
#include <zlib.h>
#endif

#define GNUTLS_COMP_FAILED NULL

typedef struct comp_hd_st {
	void *handle;
	gnutls_compression_method_t algo;
} comp_hd_st;

int _gnutls_comp_init(comp_hd_st *, gnutls_compression_method_t, int d);
void _gnutls_comp_deinit(comp_hd_st * handle, int d);

int _gnutls_decompress(comp_hd_st * handle, uint8_t * compressed,
		       size_t compressed_size, uint8_t * plain,
		       size_t max_plain_size);
int _gnutls_compress(comp_hd_st *, const uint8_t * plain,
		     size_t plain_size, uint8_t * compressed,
		     size_t max_comp_size, unsigned int stateless);

struct gnutls_compression_entry {
	const char *name;
	gnutls_compression_method_t id;
	/* the number reserved in TLS for the specific compression method */
	int num;

	/* used in zlib compressor */
	int window_bits;
	int mem_level;
	int comp_level;
};
typedef struct gnutls_compression_entry gnutls_compression_entry;

#endif
