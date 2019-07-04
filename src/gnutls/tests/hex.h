/*
 * Copyright (C) 2017 Red Hat, Inc.
 *
 * Author: Nikos Mavrogiannopoulos
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

#ifndef GNUTLS_TESTS_HEX_H
#define GNUTLS_TESTS_HEX_H

#include <stdio.h>
#include <string.h>
#include <gnutls/gnutls.h>

inline static gnutls_datum_t SHEX(const char *hex)
{
	gnutls_datum_t input, output;
	int ret;

	input.data = (void*)hex;
	input.size = strlen(hex);

	ret = gnutls_hex_decode2(&input, &output);
	assert_int_equal(ret, 0);
	return output;
}

inline static gnutls_datum_t SDATA(const char *txt)
{
	gnutls_datum_t output;
	output.data = (void*)gnutls_strdup(txt);
	output.size = strlen(txt);
	return output;
}

#endif /* GNUTLS_TESTS_HEX_H */
