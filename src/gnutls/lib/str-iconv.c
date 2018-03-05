/*
 * Copyright (C) 2010-2016 Free Software Foundation, Inc.
 * Copyright (C) 2015-2016 Red Hat, Inc.
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

#include <config.h>
#include <system.h>
#include "gnutls_int.h"
#include "errors.h"

#include <sys/socket.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistr.h>
#include <uninorm.h>
#include "num.h"

static void change_u16_endianness(uint8_t *dst, const uint8_t *src, unsigned size, unsigned be)
{
	unsigned convert = 0;
	unsigned i;
	uint8_t tmp;

#ifdef WORDS_BIGENDIAN
	if (!be)
		convert = 1;
#else
	if (be)
		convert = 1;
#endif

	/* convert to LE */
	if (convert) {
		for (i = 0; i < size; i += 2) {
			tmp = src[i];
			dst[i] = src[1 + i];
			dst[1 + i] = tmp;
		}
	} else {
		if (dst != src)
			memcpy(dst, src, size);
	}
}

int _gnutls_ucs2_to_utf8(const void *data, size_t size,
			 gnutls_datum_t * output, unsigned be)
{
	int ret;
	size_t dstlen;
	uint8_t *src;
	uint8_t *tmp_dst = NULL;
	uint8_t *dst = NULL;

	if (size > 2 && ((uint8_t *) data)[size-1] == 0 && ((uint8_t *) data)[size-2] == 0) {
		size -= 2;
	}

	if (size == 0)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	src = gnutls_malloc(size+2);
	if (src == NULL)
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

	/* convert to LE if needed */
	change_u16_endianness(src, data, size, be);

	dstlen = 0;
	tmp_dst = u16_to_u8((uint16_t*)src, size/2, NULL, &dstlen);
	if (tmp_dst == NULL) {
		ret = gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);
		goto fail;
	}

	dst = gnutls_malloc(dstlen+1);
	if (dst == NULL) {
		gnutls_assert();
		ret = GNUTLS_E_MEMORY_ERROR;
		goto fail;
	}

	memcpy(dst, tmp_dst, dstlen);
	dst[dstlen] = 0;

	output->data = (void *) dst;
	output->size = dstlen;

	ret = 0;
	goto cleanup;

 fail:
	gnutls_free(dst);

 cleanup:
	gnutls_free(src);
	free(tmp_dst);

	return ret;
}

/* This is big-endian output only */
int _gnutls_utf8_to_ucs2(const void *data, size_t size,
			 gnutls_datum_t * output)
{
	int ret;
	size_t dstlen, nrm_size = 0, tmp_size = 0;
	uint16_t *tmp_dst = NULL;
	uint16_t *nrm_dst = NULL;
	uint8_t *dst = NULL;

	if (size == 0)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	dstlen = 0;
	tmp_dst = u8_to_u16(data, size, NULL, &tmp_size);
	if (tmp_dst == NULL)
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

	nrm_dst = u16_normalize(UNINORM_NFC, tmp_dst, tmp_size, NULL, &nrm_size);
	if (nrm_dst == NULL) {
		ret = gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);
		goto fail;
	}

	dstlen = nrm_size * 2; /* convert to bytes */

	dst = gnutls_malloc(dstlen+2);
	if (dst == NULL) {
		gnutls_assert();
		ret = GNUTLS_E_MEMORY_ERROR;
		goto fail;
	}

	/* convert to BE */
	change_u16_endianness(dst, (uint8_t*)tmp_dst, dstlen, 1);
	dst[dstlen] = 0;
	dst[dstlen+1] = 0;

	output->data = (void *) dst;
	output->size = dstlen;

	ret = 0;
	goto cleanup;

 fail:
	gnutls_free(dst);

 cleanup:
	free(tmp_dst);
	free(nrm_dst);

	return ret;
}

