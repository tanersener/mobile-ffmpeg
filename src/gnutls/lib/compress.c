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

/* This file contains the functions which convert the TLS plaintext
 * packet to TLS compressed packet.
 */

#include "gnutls_int.h"
#include "compress.h"
#include "errors.h"
#include "constate.h"
#include <algorithms.h>
#include <gnutls/gnutls.h>

/* Compression Section */
#define GNUTLS_COMPRESSION_ENTRY(name, id, wb, ml, cl)	\
  { #name, name, id, wb, ml, cl}


#define MAX_COMP_METHODS 5
const int _gnutls_comp_algorithms_size = MAX_COMP_METHODS;

gnutls_compression_entry _gnutls_compression_algorithms[MAX_COMP_METHODS] = {
	GNUTLS_COMPRESSION_ENTRY(GNUTLS_COMP_NULL, 0x00, 0, 0, 0),
#ifdef HAVE_LIBZ
	/* draft-ietf-tls-compression-02 */
	GNUTLS_COMPRESSION_ENTRY(GNUTLS_COMP_DEFLATE, 0x01, 15, 8, 3),
#endif
	{0, 0, 0, 0, 0, 0}
};

static const gnutls_compression_method_t supported_compressions[] = {
#ifdef HAVE_LIBZ
	GNUTLS_COMP_DEFLATE,
#endif
	GNUTLS_COMP_NULL,
	0
};

#define GNUTLS_COMPRESSION_LOOP(b)	   \
  const gnutls_compression_entry *p;					\
  for(p = _gnutls_compression_algorithms; p->name != NULL; p++) { b ; }
#define GNUTLS_COMPRESSION_ALG_LOOP(a)					\
  GNUTLS_COMPRESSION_LOOP( if(p->id == algorithm) { a; break; } )
#define GNUTLS_COMPRESSION_ALG_LOOP_NUM(a)				\
  GNUTLS_COMPRESSION_LOOP( if(p->num == num) { a; break; } )

/* Compression Functions */

/**
 * gnutls_compression_get_name:
 * @algorithm: is a Compression algorithm
 *
 * Convert a #gnutls_compression_method_t value to a string.
 *
 * Returns: a pointer to a string that contains the name of the
 *   specified compression algorithm, or %NULL.
 **/
const char *gnutls_compression_get_name(gnutls_compression_method_t
					algorithm)
{
	const char *ret = NULL;

	/* avoid prefix */
	GNUTLS_COMPRESSION_ALG_LOOP(ret =
				    p->name + sizeof("GNUTLS_COMP_") - 1);

	return ret;
}

/**
 * gnutls_compression_get_id:
 * @name: is a compression method name
 *
 * The names are compared in a case insensitive way.
 *
 * Returns: an id of the specified in a string compression method, or
 *   %GNUTLS_COMP_UNKNOWN on error.
 **/
gnutls_compression_method_t gnutls_compression_get_id(const char *name)
{
	gnutls_compression_method_t ret = GNUTLS_COMP_UNKNOWN;

	GNUTLS_COMPRESSION_LOOP(if
				(strcasecmp
				 (p->name + sizeof("GNUTLS_COMP_") - 1,
				  name) == 0) ret = p->id);

	return ret;
}

/**
 * gnutls_compression_list:
 *
 * Get a list of compression methods.  
 *
 * Returns: a zero-terminated list of #gnutls_compression_method_t
 *   integers indicating the available compression methods.
 **/
const gnutls_compression_method_t *gnutls_compression_list(void)
{
	return supported_compressions;
}

/* return the tls number of the specified algorithm */
int _gnutls_compression_get_num(gnutls_compression_method_t algorithm)
{
	int ret = -1;

	/* avoid prefix */
	GNUTLS_COMPRESSION_ALG_LOOP(ret = p->num);

	return ret;
}

#ifdef HAVE_LIBZ

static int get_wbits(gnutls_compression_method_t algorithm)
{
	int ret = -1;
	/* avoid prefix */
	GNUTLS_COMPRESSION_ALG_LOOP(ret = p->window_bits);
	return ret;
}

static int get_mem_level(gnutls_compression_method_t algorithm)
{
	int ret = -1;
	/* avoid prefix */
	GNUTLS_COMPRESSION_ALG_LOOP(ret = p->mem_level);
	return ret;
}

static int get_comp_level(gnutls_compression_method_t algorithm)
{
	int ret = -1;
	/* avoid prefix */
	GNUTLS_COMPRESSION_ALG_LOOP(ret = p->comp_level);
	return ret;
}

#endif

/* returns the gnutls internal ID of the TLS compression
 * method num
 */
gnutls_compression_method_t _gnutls_compression_get_id(int num)
{
	gnutls_compression_method_t ret = -1;

	/* avoid prefix */
	GNUTLS_COMPRESSION_ALG_LOOP_NUM(ret = p->id);

	return ret;
}

int _gnutls_compression_is_ok(gnutls_compression_method_t algorithm)
{
	ssize_t ret = -1;
	GNUTLS_COMPRESSION_ALG_LOOP(ret = p->id);
	if (ret >= 0)
		ret = 0;
	else
		ret = 1;
	return ret;
}



/* For compression  */

#define MIN_PRIVATE_COMP_ALGO 0xEF

/* returns the TLS numbers of the compression methods we support
 */
#define SUPPORTED_COMPRESSION_METHODS session->internals.priorities.compression.algorithms
int
_gnutls_supported_compression_methods(gnutls_session_t session,
				      uint8_t * comp, size_t comp_size)
{
	unsigned int i, j;
	int tmp;

	if (comp_size < SUPPORTED_COMPRESSION_METHODS)
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

	for (i = j = 0; i < SUPPORTED_COMPRESSION_METHODS; i++) {
		if (IS_DTLS(session) && session->internals.priorities.compression.priority[i] != GNUTLS_COMP_NULL) {
			gnutls_assert();
			continue;
		}

		tmp =
		    _gnutls_compression_get_num(session->
						internals.priorities.
						compression.priority[i]);

		/* remove private compression algorithms, if requested.
		 */
		if (tmp == -1 || (tmp >= MIN_PRIVATE_COMP_ALGO &&
				  session->internals.enable_private == 0))
		{
			gnutls_assert();
			continue;
		}

		comp[j] = (uint8_t) tmp;
		j++;
	}

	if (j == 0) {
		gnutls_assert();
		return GNUTLS_E_NO_COMPRESSION_ALGORITHMS;
	}
	return j;
}


/* The flag d is the direction (compress, decompress). Non zero is
 * decompress.
 */
int _gnutls_comp_init(comp_hd_st * handle,
		      gnutls_compression_method_t method, int d)
{
	handle->algo = method;
	handle->handle = NULL;

	switch (method) {
	case GNUTLS_COMP_DEFLATE:
#ifdef HAVE_LIBZ
		{
			int window_bits, mem_level;
			int comp_level;
			z_stream *zhandle;
			int err;

			window_bits = get_wbits(method);
			mem_level = get_mem_level(method);
			comp_level = get_comp_level(method);

			handle->handle = gnutls_malloc(sizeof(z_stream));
			if (handle->handle == NULL)
				return
				    gnutls_assert_val
				    (GNUTLS_E_MEMORY_ERROR);

			zhandle = handle->handle;

			zhandle->zalloc = (alloc_func) 0;
			zhandle->zfree = (free_func) 0;
			zhandle->opaque = (voidpf) 0;

			if (d)
				err = inflateInit2(zhandle, window_bits);
			else {
				err = deflateInit2(zhandle,
						   comp_level, Z_DEFLATED,
						   window_bits, mem_level,
						   Z_DEFAULT_STRATEGY);
			}
			if (err != Z_OK) {
				gnutls_assert();
				gnutls_free(handle->handle);
				return GNUTLS_E_COMPRESSION_FAILED;
			}
		}
		break;
#endif
	case GNUTLS_COMP_NULL:
	case GNUTLS_COMP_UNKNOWN:
		break;
	default:
		return GNUTLS_E_UNKNOWN_COMPRESSION_ALGORITHM;
	}

	return 0;
}

/* The flag d is the direction (compress, decompress). Non zero is
 * decompress.
 */
void _gnutls_comp_deinit(comp_hd_st * handle, int d)
{
	if (handle != NULL) {
		switch (handle->algo) {
#ifdef HAVE_LIBZ
		case GNUTLS_COMP_DEFLATE:
			{
				if (d)
					inflateEnd(handle->handle);
				else
					deflateEnd(handle->handle);
				break;
			}
#endif
		default:
			break;
		}
		gnutls_free(handle->handle);
		handle->handle = NULL;
	}
}

/* These functions are memory consuming 
 */

int
_gnutls_compress(comp_hd_st * handle, const uint8_t * plain,
		 size_t plain_size, uint8_t * compressed,
		 size_t max_comp_size, unsigned int stateless)
{
	int compressed_size = GNUTLS_E_COMPRESSION_FAILED;

	/* NULL compression is not handled here
	 */
	if (handle == NULL) {
		gnutls_assert();
		return GNUTLS_E_INTERNAL_ERROR;
	}

	switch (handle->algo) {
#ifdef HAVE_LIBZ
	case GNUTLS_COMP_DEFLATE:
		{
			z_stream *zhandle;
			int err;
			int type;

			if (stateless) {
				type = Z_FULL_FLUSH;
			} else
				type = Z_SYNC_FLUSH;

			zhandle = handle->handle;

			zhandle->next_in = (Bytef *) plain;
			zhandle->avail_in = plain_size;
			zhandle->next_out = (Bytef *) compressed;
			zhandle->avail_out = max_comp_size;

			err = deflate(zhandle, type);
			if (err != Z_OK || zhandle->avail_in != 0)
				return
				    gnutls_assert_val
				    (GNUTLS_E_COMPRESSION_FAILED);


			compressed_size =
			    max_comp_size - zhandle->avail_out;
			break;
		}
#endif
	default:
		gnutls_assert();
		return GNUTLS_E_INTERNAL_ERROR;
	}			/* switch */

#ifdef COMPRESSION_DEBUG
	_gnutls_debug_log("Compression ratio: %f\n",
			  (float) ((float) compressed_size /
				   (float) plain_size));
#endif

	return compressed_size;
}



int
_gnutls_decompress(comp_hd_st * handle, uint8_t * compressed,
		   size_t compressed_size, uint8_t * plain,
		   size_t max_plain_size)
{
	int plain_size = GNUTLS_E_DECOMPRESSION_FAILED;

	if (compressed_size > max_plain_size + EXTRA_COMP_SIZE) {
		gnutls_assert();
		return GNUTLS_E_DECOMPRESSION_FAILED;
	}

	/* NULL compression is not handled here
	 */

	if (handle == NULL) {
		gnutls_assert();
		return GNUTLS_E_INTERNAL_ERROR;
	}

	switch (handle->algo) {
#ifdef HAVE_LIBZ
	case GNUTLS_COMP_DEFLATE:
		{
			z_stream *zhandle;
			int err;

			zhandle = handle->handle;

			zhandle->next_in = (Bytef *) compressed;
			zhandle->avail_in = compressed_size;

			zhandle->next_out = (Bytef *) plain;
			zhandle->avail_out = max_plain_size;
			err = inflate(zhandle, Z_SYNC_FLUSH);

			if (err != Z_OK)
				return
				    gnutls_assert_val
				    (GNUTLS_E_DECOMPRESSION_FAILED);

			plain_size = max_plain_size - zhandle->avail_out;
			break;
		}
#endif
	default:
		gnutls_assert();
		return GNUTLS_E_INTERNAL_ERROR;
	}			/* switch */

	return plain_size;
}
