/*
 * Copyright (C) 2001-2012 Free Software Foundation, Inc.
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

#include "gnutls_int.h"
#include "errors.h"
#include <num.h>
#include <xsize.h>

gnutls_alloc_function gnutls_secure_malloc = malloc;
gnutls_alloc_function gnutls_malloc = malloc;
gnutls_free_function gnutls_free = free;
gnutls_realloc_function gnutls_realloc = realloc;

void *(*gnutls_calloc) (size_t, size_t) = calloc;
char *(*gnutls_strdup) (const char *) = _gnutls_strdup;

void *_gnutls_calloc(size_t nmemb, size_t size)
{
	void *ret;
	size_t n = xtimes(nmemb, size);
	ret = (size_in_bounds_p(n) ? gnutls_malloc(n) : NULL);
	if (ret != NULL)
		memset(ret, 0, size);
	return ret;
}

/* This realloc will free ptr in case realloc
 * fails.
 */
void *gnutls_realloc_fast(void *ptr, size_t size)
{
	void *ret;

	if (size == 0)
		return ptr;

	ret = gnutls_realloc(ptr, size);
	if (ret == NULL) {
		gnutls_free(ptr);
	}

	return ret;
}

char *_gnutls_strdup(const char *str)
{
	size_t siz;
	char *ret;

	if(unlikely(!str))
		return NULL;

	siz = strlen(str) + 1;

	ret = gnutls_malloc(siz);
	if (ret != NULL)
		memcpy(ret, str, siz);
	return ret;
}

#if 0
/* don't use them. They are included for documentation.
 */

/**
 * gnutls_malloc:
 * @s: size to allocate in bytes
 *
 * This function will allocate 's' bytes data, and
 * return a pointer to memory. This function is supposed
 * to be used by callbacks.
 *
 * The allocation function used is the one set by
 * gnutls_global_set_mem_functions().
 **/
void *gnutls_malloc(size_t s)
{
	int x;
}

/**
 * gnutls_free:
 * @ptr: pointer to memory
 *
 * This function will free data pointed by ptr.
 *
 * The deallocation function used is the one set by
 * gnutls_global_set_mem_functions().
 *
 **/
void gnutls_free(void *ptr)
{
	int x;
}

#endif

/* Returns 1 if the provided buffer is all zero.
 * It leaks no information via timing.
 */
unsigned _gnutls_mem_is_zero(const uint8_t *ptr, unsigned size)
{
	unsigned i;
	uint8_t res = 0;

	for (i=0;i<size;i++) {
		res |= ptr[i];
	}

	return ((res==0)?1:0);
}
