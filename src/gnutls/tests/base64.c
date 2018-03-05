/*
 * Copyright (C) 2016 Red Hat, Inc.
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
 * You should have received a copy of the GNU General Public License
 * along with GnuTLS; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

/* Parts copied from GnuTLS example programs. */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <gnutls/gnutls.h>

#include "utils.h"

static void encode(const char *test_name, const gnutls_datum_t *raw, const char *expected)
{
	int ret;
	gnutls_datum_t out, in;

	ret = gnutls_pem_base64_encode2(test_name, raw, &out);
	if (ret < 0) {
		fail("%s: gnutls_pem_base64_encode2: %s\n", test_name, gnutls_strerror(ret));
		exit(1);
	}

	if (strlen(expected)!=out.size) {
		fail("%s: gnutls_pem_base64_encode2: output has incorrect size (%d, expected %d)\n", test_name, (int)out.size, (int)strlen(expected));
		exit(1);
	}

	if (strncasecmp(expected, (char*)out.data, out.size) != 0) {
		fail("%s: gnutls_pem_base64_encode2: output does not match the expected\n", test_name);
		exit(1);
	}

	gnutls_free(out.data);

	in.data = (void*)expected;
	in.size = strlen(expected);
	ret = gnutls_pem_base64_decode2(test_name, &in, &out);
	if (ret < 0) {
		fail("%s: gnutls_pem_base64_decode2: %s\n", test_name, gnutls_strerror(ret));
		exit(1);
	}

	if (raw->size!=out.size) {
		fail("%s: gnutls_pem_base64_decode2: output has incorrect size (%d, expected %d)\n", test_name, out.size, raw->size);
		exit(1);
	}

	if (memcmp(raw->data, out.data, out.size) != 0) {
		fail("%s: gnutls_pem_base64_decode2: output does not match the expected\n", test_name);
		exit(1);
	}

	gnutls_free(out.data);

	return;
}

static void decode(const char *test_name, const gnutls_datum_t *raw, const char *hex, int res)
{
	int ret;
	gnutls_datum_t out, in;

	in.data = (void*)hex;
	in.size = strlen(hex);
	ret = gnutls_pem_base64_decode2(test_name, &in, &out);
	if (ret < 0) {
		if (res == ret) /* expected */
			return;
		fail("%s: gnutls_pem_base64_decode2: %d/%s\n", test_name, ret, gnutls_strerror(ret));
		exit(1);
	}

	if (res != 0) {
		fail("%s: gnutls_pem_base64_decode2: expected failure, but succeeded!\n", test_name);
		exit(1);
	}

	if (raw->size!=out.size) {
		fail("%s: gnutls_pem_base64_decode2: output has incorrect size (%d, expected %d)\n", test_name, out.size, raw->size);
		exit(1);
	}

	if (memcmp(raw->data, out.data, out.size) != 0) {
		fail("%s: gnutls_pem_base64_decode2: output does not match the expected\n", test_name);
		exit(1);
	}

	gnutls_free(out.data);

	return;
}

struct encode_tests_st {
	const char *name;
	gnutls_datum_t raw;
	const char *pem;
};

struct encode_tests_st encode_tests[] = {
	{
		.name = "rnd1",
		.pem = "-----BEGIN rnd1-----\n"
			"9ppGioRpeiiD2lLNYC85eA==\n"
			"-----END rnd1-----\n",
		.raw = {(void*)"\xf6\x9a\x46\x8a\x84\x69\x7a\x28\x83\xda\x52\xcd\x60\x2f\x39\x78", 16}
	},
	{
		.name = "rnd2",
		.pem = "-----BEGIN rnd2-----\n"
			"LJ/7hUZ3TtPIz2dlc5+YvELe+Q==\n"
			"-----END rnd2-----\n",
		.raw = {(void*)"\x2c\x9f\xfb\x85\x46\x77\x4e\xd3\xc8\xcf\x67\x65\x73\x9f\x98\xbc\x42\xde\xf9", 19}
	}
};

struct decode_tests_st {
	const char *name;
	gnutls_datum_t raw;
	const char *pem;
	int res;
};

struct decode_tests_st decode_tests[] = {
	{
		.name = "dec-rnd1",
		.pem = "-----BEGIN dec-rnd1-----\n"
			"9ppGioRpeiiD2lLNYC85eA==\n"
			"-----END rnd1-----\n",
		.raw = {(void*)"\xf6\x9a\x46\x8a\x84\x69\x7a\x28\x83\xda\x52\xcd\x60\x2f\x39\x78", 16},
		.res = 0
	},
	{
		.name = "dec-rnd2",
		.pem = "-----BEGIN dec-rnd2-----\n"
			"LJ/7hUZ3TtPIz2dlc5+YvELe+Q==\n"
			"-----END rnd2-----\n",
		.raw = {(void*)"\x2c\x9f\xfb\x85\x46\x77\x4e\xd3\xc8\xcf\x67\x65\x73\x9f\x98\xbc\x42\xde\xf9", 19},
		.res = 0
	},
	{
		.name = "dec-extra-chars",
		.pem = "-----BEGIN dec-extra-chars-----   \n\n"
			"\n\n  LJ/7hUZ3TtPIz2dlc5+YvELe+Q==  \n"
			"   -----END rnd2-----  \n  ",
		.raw = {(void*)"\x2c\x9f\xfb\x85\x46\x77\x4e\xd3\xc8\xcf\x67\x65\x73\x9f\x98\xbc\x42\xde\xf9", 19},
		.res = 0
	},
	{
		.name = "dec-invalid-header",
		.pem = "-----BEGIN dec-xxx-----\n"
			"LJ/7hUZ3TtPIz2dlc5+YvELe+Q==\n"
			"-----END rnd2-----\n",
		.raw = {(void*)"\x2c\x9f\xfb\x85\x46\x77\x4e\xd3\xc8\xcf\x67\x65\x73\x9f\x98\xbc\x42\xde\xf9", 19},
		.res = GNUTLS_E_BASE64_UNEXPECTED_HEADER_ERROR
	},
	{
		.name = "dec-invalid-data",
		.pem = "-----BEGIN dec-invalid-data-----\n"
			"XLJ/7hUZ3TtPIz2dlc5+YvELe+Q==\n"
			"-----END rnd2-----\n",
		.raw = {(void*)"\x2c\x9f\xfb\x85\x46\x77\x4e\xd3\xc8\xcf\x67\x65\x73\x9f\x98\xbc\x42\xde\xf9", 19},
		.res = GNUTLS_E_BASE64_DECODING_ERROR
	},
	{
		.name = "dec-invalid-suffix",
		.pem = "-----BEGIN dec-invalid-suffix-----\n"
			"LJ/7hUZ3TtPIz2dlc5+YvELe+Q==XXX\n"
			"-----END rnd2-----\n",
		.raw = {(void*)"\x2c\x9f\xfb\x85\x46\x77\x4e\xd3\xc8\xcf\x67\x65\x73\x9f\x98\xbc\x42\xde\xf9", 19},
		.res = GNUTLS_E_BASE64_DECODING_ERROR
	}
};

void doit(void)
{
	unsigned i;

	for (i=0;i<sizeof(encode_tests)/sizeof(encode_tests[0]);i++) {
		encode(encode_tests[i].name, &encode_tests[i].raw, encode_tests[i].pem);
	}

	for (i=0;i<sizeof(decode_tests)/sizeof(decode_tests[0]);i++) {
		decode(decode_tests[i].name, &decode_tests[i].raw, decode_tests[i].pem, decode_tests[i].res);
	}
}

