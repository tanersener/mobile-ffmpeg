/*
 * Copyright (C) 2019 Tim Rühsen
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
 * You should have received a copy of the GNU General Public License
 * along with GnuTLS; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gnutls_int.h>
#include "utils.h"

void doit(void)
{
	static const struct test_data {
		const char *
			input;
		const char *
			output;
	} test_data[] = {
		{ "%20%20", "  ", },
		{ "%20", " ", },
		{ "%2z", "%2z", },
		{ "%2", "%2", },
		{ "%", "%", },
		{ "", "", },
	};

	for (unsigned it = 0; it < countof(test_data); it++) {
		const struct test_data *t = &test_data[it];
		gnutls_buffer_st str;
		int ret;

		_gnutls_buffer_init(&str);

		ret = _gnutls_buffer_append_data(&str, t->input, strlen(t->input));
		if (ret < 0)
			fail("_gnutls_buffer_append_str: %s\n", gnutls_strerror(ret));

		ret = _gnutls_buffer_unescape(&str);
		if (ret < 0)
			fail("_gnutls_buffer_unescape: %s\n", gnutls_strerror(ret));

		ret = _gnutls_buffer_append_data(&str, "", 1);
		if (ret < 0)
			fail("_gnutls_buffer_append_data: %s\n", gnutls_strerror(ret));

		/* using malloc() instead of stack memory for better buffer overflow detection */
		gnutls_datum output;

		_gnutls_buffer_pop_datum(&str, &output, strlen(t->output) + 1);

		if (strcmp(t->output, (char *) output.data))
			fail("output differs [%d]: expected '%s', seen '%s'\n", it, t->output, (char *) output.data);

		_gnutls_buffer_clear(&str);
	}
}

