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

#ifdef ENABLE_SRP

static void encode(const char *test_name, const gnutls_datum_t *raw, const char *expected)
{
	int ret;
	gnutls_datum_t out, in;

	ret = gnutls_srp_base64_encode2(raw, &out);
	if (ret < 0) {
		fail("%s: gnutls_srp_base64_encode2: %s\n", test_name, gnutls_strerror(ret));
		exit(1);
	}

	if (strlen(expected)!=out.size) {
		fail("%s: gnutls_srp_base64_encode2: output has incorrect size (%d, expected %d)\n", test_name, (int)out.size, (int)strlen(expected));
		exit(1);
	}

	if (strncasecmp(expected, (char*)out.data, out.size) != 0) {
		fail("%s: gnutls_srp_base64_encode2: output does not match the expected\n", test_name);
		exit(1);
	}

	gnutls_free(out.data);

	in.data = (void*)expected;
	in.size = strlen(expected);
	ret = gnutls_srp_base64_decode2(&in, &out);
	if (ret < 0) {
		fail("%s: gnutls_srp_base64_decode2: %s\n", test_name, gnutls_strerror(ret));
		exit(1);
	}

	if (raw->size!=out.size) {
		fail("%s: gnutls_srp_base64_decode2: output has incorrect size (%d, expected %d)\n", test_name, out.size, raw->size);
		exit(1);
	}

	if (memcmp(raw->data, out.data, out.size) != 0) {
		fail("%s: gnutls_srp_base64_decode2: output does not match the expected\n", test_name);
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
	ret = gnutls_srp_base64_decode2(&in, &out);
	if (ret < 0) {
		if (res == ret) /* expected */
			return;
		fail("%s: gnutls_srp_base64_decode2: %d/%s\n", test_name, ret, gnutls_strerror(ret));
		exit(1);
	}

	if (res != 0) {
		fail("%s: gnutls_srp_base64_decode2: expected failure, but succeeded!\n", test_name);
		exit(1);
	}

	if (raw->size!=out.size) {
		fail("%s: gnutls_srp_base64_decode2: output has incorrect size (%d, expected %d)\n", test_name, out.size, raw->size);
		exit(1);
	}

	if (memcmp(raw->data, out.data, out.size) != 0) {
		fail("%s: gnutls_srp_base64_decode2: output does not match the expected\n", test_name);
		exit(1);
	}

	gnutls_free(out.data);

	return;
}

struct encode_tests_st {
	const char *name;
	gnutls_datum_t raw;
	const char *sb64;
};

struct encode_tests_st encode_tests[] = {
	{
		.name = "rnd1",
		.sb64 = "3scaQAX6bwA8FQKirWBpbu",
		.raw = {(void*)"\xf6\x9a\x46\x8a\x84\x69\x7a\x28\x83\xda\x52\xcd\x60\x2f\x39\x78", 16}
	},
	{
		.name = "rnd2",
		.sb64 = "id/k5HdTEqyZFPsLpdvYyGjxv",
		.raw = {(void*)"\x2c\x9f\xfb\x85\x46\x77\x4e\xd3\xc8\xcf\x67\x65\x73\x9f\x98\xbc\x42\xde\xf9", 19}
	}
};

struct decode_tests_st {
	const char *name;
	gnutls_datum_t raw;
	const char *sb64;
	int res;
};

struct decode_tests_st decode_tests[] = {
	{
		.name = "dec-rnd1",
		.sb64 = "3scaQAX6bwA8FQKirWBpbu",
		.raw = {(void*)"\xf6\x9a\x46\x8a\x84\x69\x7a\x28\x83\xda\x52\xcd\x60\x2f\x39\x78", 16},
		.res = 0
	},
	{
		.name = "dec-rnd2",
		.sb64 = "id/k5HdTEqyZFPsLpdvYyGjxv",
		.raw = {(void*)"\x2c\x9f\xfb\x85\x46\x77\x4e\xd3\xc8\xcf\x67\x65\x73\x9f\x98\xbc\x42\xde\xf9", 19},
		.res = 0
	},
	{
		.name = "dec-extra-chars",
		.sb64 = "  id/k5HdTEqyZFPsLpdvYyGjxv   ",
		.raw = {(void*)"\x2c\x9f\xfb\x85\x46\x77\x4e\xd3\xc8\xcf\x67\x65\x73\x9f\x98\xbc\x42\xde\xf9", 19},
		.res = GNUTLS_E_BASE64_DECODING_ERROR
	}
};

void doit(void)
{
	unsigned i;

	for (i=0;i<sizeof(encode_tests)/sizeof(encode_tests[0]);i++) {
		encode(encode_tests[i].name, &encode_tests[i].raw, encode_tests[i].sb64);
	}

	for (i=0;i<sizeof(decode_tests)/sizeof(decode_tests[0]);i++) {
		decode(decode_tests[i].name, &decode_tests[i].raw, decode_tests[i].sb64, decode_tests[i].res);
	}
}

#else

void doit(void)
{
	exit(77);
}
#endif
