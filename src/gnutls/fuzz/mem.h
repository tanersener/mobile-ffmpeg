/*
 * Copyright (C) 2017 Nikos Mavrogiannopoulos
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef MEM_H
# define MEM_H

typedef struct mem_st {
	const uint8_t *data;
	size_t size;
} mem_st;

#define MIN(x,y) ((x)<(y)?(x):(y))
static ssize_t
mem_push(gnutls_transport_ptr_t tr, const void *data, size_t len)
{
	return len;
}

static ssize_t mem_pull(gnutls_transport_ptr_t tr, void *data, size_t len)
{
	struct mem_st *p = (struct mem_st *)tr;

	if (p->size == 0) {
		return 0;
	}

	len = MIN(len, p->size);
	memcpy(data, p->data, len);

	p->size -= len;
	p->data += len;

	return len;
}

static
int mem_pull_timeout(gnutls_transport_ptr_t tr, unsigned int ms)
{
	struct mem_st *p = (struct mem_st *)tr;

	if (p->size > 0)
		return 1;	/* available data */
	else
		return 0;	/* timeout */
}

#endif
