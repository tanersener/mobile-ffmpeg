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
};

struct test_st {
	const char *name;
	const giovec_t *iov;
	size_t iovcnt;
	size_t block_size;
	const struct exp_st *exp;
	size_t expcnt;
};

static const giovec_t iov16[] = {
	{(void *) "0123456789012345", 16},
	{(void *) "0123456789012345", 16},
	{(void *) "0123456789012345", 16},
	{(void *) "0123456789012345", 16}
};

static const struct exp_st exp16_64[] = {
	{64, 4, 0},
	{0, 0, 0}
};

static const struct exp_st exp16_32[] = {
	{32, 2, 0},
	{32, 4, 0},
	{0, 0, 0}
};

static const struct exp_st exp16_16[] = {
	{16, 1, 0},
	{16, 2, 0},
	{16, 3, 0},
	{16, 4, 0},
	{0, 0, 0}
};

static const struct exp_st exp16_4[] = {
	{16, 1, 0},
	{16, 2, 0},
	{16, 3, 0},
	{16, 4, 0},
	{0, 0, 0}
};

static const struct exp_st exp16_3[] = {
	{15, 0, 15},
	{3, 1, 2},
	{12, 1, 14},
	{3, 2, 1},
	{15, 3, 0},
	{15, 3, 15},
	{1, 4, 0},
	{0, 0, 0}
};

static const giovec_t iov8[] = {
	{(void *) "01234567", 8},
	{(void *) "01234567", 8},
	{(void *) "01234567", 8},
	{(void *) "01234567", 8}
};

static const struct exp_st exp8_64[] = {
	{32, 4, 0},
	{0, 0, 0}
};

static const giovec_t iov_odd[] = {
	{(void *) "0", 1},
	{(void *) "012", 3},
	{(void *) "01234", 5},
	{(void *) "0123456", 7},
	{(void *) "012345678", 9},
	{(void *) "01234567890", 11},
	{(void *) "0123456789012", 13},
	{(void *) "012345678901234", 15}
};

static const struct exp_st exp_odd_16[] = {
	{16, 4, 0},
	{16, 5, 7},
	{16, 6, 12},
	{16, 8, 0},
	{0, 0, 0}
};

static const giovec_t iov_skip[] = {
	{(void *) "0123456789012345", 16},
	{(void *) "01234567", 8},
	{(void *) "", 0},
	{(void *) "", 0},
	{(void *) "0123456789012345", 16}
};

static const struct exp_st exp_skip_16[] = {
	{16, 1, 0},
	{16, 4, 8},
	{8, 5, 0},
	{0, 0, 0}
};

static const giovec_t iov_empty[] = {
	{(void *) "", 0},
	{(void *) "", 0},
	{(void *) "", 0},
	{(void *) "", 0}
};

static const struct exp_st exp_empty_16[] = {
	{0, 0, 0}
};

static const struct test_st tests[] = {
	{ "16/64", iov16, sizeof(iov16)/sizeof(iov16[0]), 64,
	  exp16_64, sizeof(exp16_64)/sizeof(exp16_64[0]) },
	{ "16/32", iov16, sizeof(iov16)/sizeof(iov16[0]), 32,
	  exp16_32, sizeof(exp16_32)/sizeof(exp16_32[0]) },
	{ "16/16", iov16, sizeof(iov16)/sizeof(iov16[0]), 16,
	  exp16_16, sizeof(exp16_16)/sizeof(exp16_16[0]) },
	{ "16/4", iov16, sizeof(iov16)/sizeof(iov16[0]), 4,
	  exp16_4, sizeof(exp16_4)/sizeof(exp16_4[0]) },
	{ "16/3", iov16, sizeof(iov16)/sizeof(iov16[0]), 3,
	  exp16_3, sizeof(exp16_3)/sizeof(exp16_3[0]) },
	{ "8/64", iov8, sizeof(iov8)/sizeof(iov8[0]), 64,
	  exp8_64, sizeof(exp8_64)/sizeof(exp8_64[0]) },
	{ "odd/16", iov_odd, sizeof(iov_odd)/sizeof(iov_odd[0]), 16,
	  exp_odd_16, sizeof(exp_odd_16)/sizeof(exp_odd_16[0]) },
	{ "skip/16", iov_skip, sizeof(iov_skip)/sizeof(iov_skip[0]), 16,
	  exp_skip_16, sizeof(exp_skip_16)/sizeof(exp_skip_16[0]) },
	{ "empty/16", iov_empty, sizeof(iov_empty)/sizeof(iov_empty[0]), 16,
	  exp_empty_16, sizeof(exp_empty_16)/sizeof(exp_empty_16[0]) },
};

static void
copy(giovec_t *dst, uint8_t *buffer, const giovec_t *src, size_t iovcnt)
{
	uint8_t *p = buffer;
	size_t i;

	for (i = 0; i < iovcnt; i++) {
		dst[i].iov_base = p;
		dst[i].iov_len = src[i].iov_len;
		memcpy(dst[i].iov_base, src[i].iov_base, src[i].iov_len);
		p += src[i].iov_len;
	}
}

static void
translate(uint8_t *data, size_t len)
{
	for (; len > 0; len--) {
		uint8_t *p = &data[len - 1];
		if (*p >= '0' && *p <= '9')
			*p = 'A' + *p - '0';
		else if (*p >= 'A' && *p <= 'Z')
			*p = '0' + *p - 'A';
	}
}

#define MAX_BUF 1024
#define MAX_IOV 16

void
doit (void)
{
	uint8_t buffer[MAX_BUF];
	size_t i;

	for (i = 0; i < sizeof(tests)/sizeof(tests[0]); i++) {
		giovec_t iov[MAX_IOV];
		struct iov_iter_st iter;
		const struct exp_st *exp = tests[i].exp;
		uint8_t *data;
		size_t j;

		copy(iov, buffer, tests[i].iov, tests[i].iovcnt);

		success("%s\n", tests[i].name);
		assert(_gnutls_iov_iter_init(&iter,
					     iov, tests[i].iovcnt,
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
				if (iter.block_offset != 0)
					fail("iter.block_offset: %u != 0\n",
					     (unsigned) iter.block_offset);
				else if (debug)
					success("iter.block_offset: %u == 0\n",
					     (unsigned) iter.block_offset);

				translate(data, ret);

				ret = _gnutls_iov_iter_sync(&iter, data, ret);
				if (ret < 0)
					fail("sync failed\n");
			}
		}

		for (j = 0; j < tests[i].iovcnt; j++) {
			translate(iov[j].iov_base, iov[j].iov_len);

			if (memcmp(iov[j].iov_base, tests[i].iov[j].iov_base,
				   iov[j].iov_len) != 0)
				fail("iov doesn't match: %*s != %*s\n",
				     (int)iov[j].iov_len,
				     (char *)iov[j].iov_base,
				     (int)tests[i].iov[j].iov_len,
				     (char *)tests[i].iov[j].iov_len);
		}
	}
}
