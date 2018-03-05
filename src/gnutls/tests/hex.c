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

	ret = gnutls_hex_encode2(raw, &out);
	if (ret < 0) {
		fail("%s: gnutls_hex_encode2: %s\n", test_name, gnutls_strerror(ret));
		exit(1);
	}

	if (strlen(expected)!=out.size) {
		fail("%s: gnutls_hex_encode2: output has incorrect size (%d, expected %d)\n", test_name, (int)out.size, (int)strlen(expected));
		exit(1);
	}

	if (strncasecmp(expected, (char*)out.data, out.size) != 0) {
		fail("%s: gnutls_hex_encode2: output does not match the expected\n", test_name);
		exit(1);
	}

	gnutls_free(out.data);

	in.data = (void*)expected;
	in.size = strlen(expected);
	ret = gnutls_hex_decode2(&in, &out);
	if (ret < 0) {
		fail("%s: gnutls_hex_decode2: %s\n", test_name, gnutls_strerror(ret));
		exit(1);
	}

	if (raw->size!=out.size) {
		fail("%s: gnutls_hex_decode2: output has incorrect size (%d, expected %d)\n", test_name, out.size, raw->size);
		exit(1);
	}

	if (memcmp(raw->data, out.data, out.size) != 0) {
		fail("%s: gnutls_hex_decode2: output does not match the expected\n", test_name);
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
	ret = gnutls_hex_decode2(&in, &out);
	if (ret < 0) {
		if (res == ret) /* expected */
			return;
		fail("%s: gnutls_hex_decode2: %d/%s\n", test_name, ret, gnutls_strerror(ret));
		exit(1);
	}

	if (res != 0) {
		fail("%s: gnutls_hex_decode2: expected failure, but succeeded!\n", test_name);
		exit(1);
	}

	if (raw->size!=out.size) {
		fail("%s: gnutls_hex_decode2: output has incorrect size (%d, expected %d)\n", test_name, out.size, raw->size);
		exit(1);
	}

	if (memcmp(raw->data, out.data, out.size) != 0) {
		fail("%s: gnutls_hex_decode2: output does not match the expected\n", test_name);
		exit(1);
	}

	gnutls_free(out.data);

	return;
}

static void decode2(const char *test_name, const gnutls_datum_t *raw, const char *hex, int res)
{
	int ret;
	unsigned char output[128];
	size_t outlen;

	outlen = sizeof(output);
	ret = gnutls_hex2bin(hex, strlen(hex), output, &outlen);
	if (ret < 0) {
		if (res == ret) /* expected */
			return;
		fail("%s: gnutls_hex2bin: %d/%s\n", test_name, ret, gnutls_strerror(ret));
		exit(1);
	}

	if (res != 0) {
		fail("%s: gnutls_hex2bin: expected failure, but succeeded!\n", test_name);
		exit(1);
	}

	if (raw->size!=outlen) {
		fail("%s: gnutls_hex2bin: output has incorrect size (%d, expected %d)\n", test_name, (int)outlen, raw->size);
		exit(1);
	}

	if (memcmp(raw->data, output, outlen) != 0) {
		fail("%s: gnutls_hex2bin: output does not match the expected\n", test_name);
		exit(1);
	}

	return;
}

struct encode_tests_st {
	const char *name;
	gnutls_datum_t raw;
	const char *hex;
};

struct encode_tests_st encode_tests[] = {
	{
		.name = "rnd1",
		.hex = "f69a468a84697a2883da52cd602f3978",
		.raw = {(void*)"\xf6\x9a\x46\x8a\x84\x69\x7a\x28\x83\xda\x52\xcd\x60\x2f\x39\x78", 16}
	},
	{
		.name = "rnd2",
		.hex = "2c9ffb8546774ed3c8cf6765739f98bc42def9",
		.raw = {(void*)"\x2c\x9f\xfb\x85\x46\x77\x4e\xd3\xc8\xcf\x67\x65\x73\x9f\x98\xbc\x42\xde\xf9", 19}
	}
};

struct decode_tests_st {
	const char *name;
	gnutls_datum_t raw;
	const char *hex;
	int res;
	int hex2bin_res;
};

struct decode_tests_st decode_tests[] = {
	{
		.name = "dec-rnd1",
		.hex = "f69a468a84697a2883da52cd602f3978",
		.raw = {(void*)"\xf6\x9a\x46\x8a\x84\x69\x7a\x28\x83\xda\x52\xcd\x60\x2f\x39\x78", 16},
		.res = 0,
		.hex2bin_res = 0
	},
	{
		.name = "dec-rnd2",
		.hex = "2c9ffb8546774ed3c8cf6765739f98bc42def9",
		.raw = {(void*)"\x2c\x9f\xfb\x85\x46\x77\x4e\xd3\xc8\xcf\x67\x65\x73\x9f\x98\xbc\x42\xde\xf9", 19},
		.res = 0,
		.hex2bin_res = 0
	},
	{
		.name = "dec-colon",
		.hex = "2c:9f:fb:85:46:77:4e:d3:c8:cf:67:65:73:9f:98:bc:42:de:f9",
		.raw = {(void*)"\x2c\x9f\xfb\x85\x46\x77\x4e\xd3\xc8\xcf\x67\x65\x73\x9f\x98\xbc\x42\xde\xf9", 19},
		.res = GNUTLS_E_PARSING_ERROR,
		.hex2bin_res = 0
	},
	{
		.name = "dec-odd-len",
		.hex = "2c9ffb8546774ed3c8cf6765739f98bc42def9a",
		.raw = {(void*)"\x2c\x9f\xfb\x85\x46\x77\x4e\xd3\xc8\xcf\x67\x65\x73\x9f\x98\xbc\x42\xde\xf9", 19},
		.res = GNUTLS_E_PARSING_ERROR,
		.hex2bin_res = GNUTLS_E_PARSING_ERROR
	}
};

void doit(void)
{
	unsigned i;

	for (i=0;i<sizeof(encode_tests)/sizeof(encode_tests[0]);i++) {
		encode(encode_tests[i].name, &encode_tests[i].raw, encode_tests[i].hex);
	}

	for (i=0;i<sizeof(decode_tests)/sizeof(decode_tests[0]);i++) {
		decode(decode_tests[i].name, &decode_tests[i].raw, decode_tests[i].hex, decode_tests[i].res);
	}

	for (i=0;i<sizeof(decode_tests)/sizeof(decode_tests[0]);i++) {
		decode2(decode_tests[i].name, &decode_tests[i].raw, decode_tests[i].hex, decode_tests[i].hex2bin_res);
	}
}

