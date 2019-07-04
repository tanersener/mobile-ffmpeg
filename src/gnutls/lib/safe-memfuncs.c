/*
 * Copyright (C) 2014 Red Hat
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

#include "gnutls_int.h"
#include <string.h>

/**
 * gnutls_memset:
 * @data: the memory to set
 * @c: the constant byte to fill the memory with
 * @size: the size of memory
 *
 * This function will operate similarly to memset(), but will
 * not be optimized out by the compiler.
 *
 * Since: 3.4.0
 **/
void gnutls_memset(void *data, int c, size_t size)
{
	volatile unsigned volatile_zero;
	volatile char *vdata = (volatile char*)data;
#ifdef HAVE_EXPLICIT_BZERO
	if (c == 0) {
		explicit_bzero(data, size);
		return;
	}
#endif
	volatile_zero = 0;

	/* This is based on a nice trick for safe memset,
	 * sent by David Jacobson in the openssl-dev mailing list.
	 */

	if (size > 0) {
		do {
			memset(data, c, size);
		} while(vdata[volatile_zero] != c);
	}
}

/**
 * gnutls_memcmp:
 * @s1: the first address to compare
 * @s2: the second address to compare
 * @n: the size of memory to compare
 *
 * This function will operate similarly to memcmp(), but will operate
 * on time that depends only on the size of the string. That is will
 * not return early if the strings don't match on the first byte.
 *
 * Returns: non zero on difference and zero if the buffers are identical.
 *
 * Since: 3.4.0
 **/
int gnutls_memcmp(const void *s1, const void *s2, size_t n)
{
	unsigned i;
	unsigned status = 0;
	const uint8_t *_s1 = s1;
	const uint8_t *_s2 = s2;

	for (i=0;i<n;i++) {
		status |= (_s1[i] ^ _s2[i]);
	}

	return status;
}

#ifdef TEST_SAFE_MEMSET
int main()
{
	char x[64];

	gnutls_memset(x, 0, sizeof(x));

	return 0;

}

#endif
