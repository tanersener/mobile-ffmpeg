/*
 * Copyright (C) 2019 Red Hat, Inc.
 *
 * Author: Daiki Ueno
 *
 * This file is part of GnuTLS.
 *
 * GnuTLS is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * GnuTLS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "gnutls_int.h"
#include "../lib/iov.h"

#include "utils.h"

struct exp_st {
	ssize_t ret;
	size_t iov_index;
	size_t iov_offset;
	size_t block_offset;
};

struct test_st {
	const char *name;
	const giovec_t *iov;
	size_t iovcnt;
	size_t block_size;
	const struct exp_st *exp;
	size_t expcnt;
	size_t remaining;
};

static const giovec_t iov16[] = {
	{(void *) "0123456789abcdef", 16},
	{(void *) "0123456789abcdef", 16},
	{(void *) "0123456789abcdef", 16},
	{(void *) "0123456789abcdef", 16}
};

static const struct exp_st exp16_64[] = {
	{1, 3, 16, 0},
	{0, 0, 0, 0}
};

static const struct exp_st exp16_32[] = {
	{1, 1, 16, 0},
	{1, 3, 16, 0},
	{0, 0, 0, 0}
};

static const struct exp_st exp16_16[] = {
	{1, 1, 0, 0},
	{1, 2, 0, 0},
	{1, 3, 0, 0},
	{1, 4, 0, 0},
	{0, 0, 0, 0}
};

static const struct exp_st exp16_4[] = {
	{4, 1, 0, 0},
	{4, 2, 0, 0},
	{4, 3, 0, 0},
	{4, 4, 0, 0},
	{0, 0, 0, 0}
};

static const struct exp_st exp16_3[] = {
	{5, 0, 15, 0},
	{1, 1, 2, 0},
	{4, 1, 14, 0},
	{1, 2, 1, 0},
	{5, 3, 0, 0},
	{5, 3, 15, 0},
	{0, 0, 0, 1}
};

static const giovec_t iov8[] = {
	{(void *) "01234567", 8},
	{(void *) "01234567", 8},
	{(void *) "01234567", 8},
	{(void *) "01234567", 8}
};

static const struct exp_st exp8_64[] = {
	{0, 0, 0, 32}
};

static const struct test_st tests[] = {
	{ "16/64", iov16, sizeof(iov16)/sizeof(iov16[0]), 64,
	  exp16_64, sizeof(exp16_64)/sizeof(exp16_64[0]), 0 },
	{ "16/32", iov16, sizeof(iov16)/sizeof(iov16[0]), 32,
	  exp16_32, sizeof(exp16_32)/sizeof(exp16_32[0]), 0 },
	{ "16/16", iov16, sizeof(iov16)/sizeof(iov16[0]), 16,
	  exp16_16, sizeof(exp16_16)/sizeof(exp16_16[0]), 0 },
	{ "16/4", iov16, sizeof(iov16)/sizeof(iov16[0]), 4,
	  exp16_4, sizeof(exp16_4)/sizeof(exp16_4[0]), 0 },
	{ "16/3", iov16, sizeof(iov16)/sizeof(iov16[0]), 3,
	  exp16_3, sizeof(exp16_3)/sizeof(exp16_3[0]), 1 },
	{ "8/64", iov8, sizeof(iov8)/sizeof(iov8[0]), 64,
	  exp8_64, sizeof(exp8_64)/sizeof(exp8_64[0]), 32 }
};

void
doit (void)
{
	size_t i;

	for (i = 0; i < sizeof(tests)/sizeof(tests[0]); i++) {
		struct iov_iter_st iter;
		const struct exp_st *exp = tests[i].exp;
		uint8_t *data;
		size_t j;

		success("%s\n", tests[i].name);
		assert(_gnutls_iov_iter_init(&iter,
					     tests[i].iov, tests[i].iovcnt,
					     tests[i].block_size) == 0);
		for (j = 0; j < tests[i].expcnt; j++) {
			ssize_t ret;

			ret = _gnutls_iov_iter_next(&iter, &data);
			if (ret != exp[j].ret)
				fail("iov_iter_next: %d != %d\n",
				     (int) ret, (int) exp[j].ret);
			else if (debug)
				success("iov_iter_next: %d == %d\n",
					(int) ret, (int) exp[j].ret);
			if (ret == 0)
				break;
			if (ret > 0) {
				if (iter.iov_index != exp[j].iov_index)
					fail("iter.iov_index: %u != %u\n",
					     (unsigned) iter.iov_index, (unsigned) exp[j].iov_index);
				else if (debug)
					success("iter.iov_index: %u == %u\n",
					     (unsigned) iter.iov_index, (unsigned) exp[j].iov_index);
				if (iter.iov_offset != exp[j].iov_offset)
					fail("iter.iov_offset: %u != %u\n",
					     (unsigned) iter.iov_offset, (unsigned) exp[j].iov_offset);
				else if (debug)
					success("iter.iov_offset: %u == %u\n",
					     (unsigned) iter.iov_offset, (unsigned) exp[j].iov_offset);
				if (iter.block_offset != exp[j].block_offset)
					fail("iter.block_offset: %u != %u\n",
					     (unsigned) iter.block_offset, (unsigned) exp[j].block_offset);
				else if (debug)
					success("iter.block_offset: %u == %u\n",
					     (unsigned) iter.block_offset, (unsigned) exp[j].block_offset);
			}
		}
		if (iter.block_offset != tests[i].remaining)
			fail("remaining: %u != %u\n",
			     (unsigned) iter.block_offset, (unsigned) tests[i].remaining);
	}
}
