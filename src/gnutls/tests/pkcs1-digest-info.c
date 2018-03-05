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
#include <stdint.h>
#include <gnutls/gnutls.h>
#include <gnutls/crypto.h>

#include "utils.h"

static void encode(const char *test_name, gnutls_digest_algorithm_t hash, const gnutls_datum_t *raw, const gnutls_datum_t *expected)
{
	int ret;
	gnutls_datum_t out;
	gnutls_digest_algorithm_t thash;
	uint8_t digest[128];
	unsigned int digest_size;

	ret = gnutls_encode_ber_digest_info(hash, raw, &out);
	if (ret < 0) {
		fail("%s: gnutls_encode_ber_digest_info: %s\n", test_name, gnutls_strerror(ret));
		exit(1);
	}

	if (expected->size!=out.size) {
		hexprint(out.data, out.size);
		fail("%s: gnutls_encode_ber_digest_info: output has incorrect size (%d, expected %d)\n", test_name, (int)out.size, expected->size);
		exit(1);
	}

	if (memcmp(expected->data, out.data, out.size) != 0) {
		hexprint(out.data, out.size);
		fail("%s: gnutls_encode_ber_digest_info: output does not match the expected\n", test_name);
		exit(1);
	}

	digest_size = sizeof(digest);
	ret = gnutls_decode_ber_digest_info(&out, &thash, digest, &digest_size);
	if (ret < 0) {
		fail("%s: gnutls_decode_ber_digest_info: %s\n", test_name, gnutls_strerror(ret));
		exit(1);
	}

	if (thash != hash) {
		fail("%s: gnutls_decode_ber_digest_info: wrong hash, got: %d, expected %d\n", test_name, (int)thash, (int)hash);
		exit(1);
	}

	if (raw->size!=digest_size) {
		fail("%s: gnutls_decode_ber_digest_info: output has incorrect size (%d, expected %d)\n", test_name, digest_size, raw->size);
		exit(1);
	}

	if (memcmp(raw->data, digest, digest_size) != 0) {
		fail("%s: gnutls_decode_ber_digest_info: output does not match the expected\n", test_name);
		exit(1);
	}

	gnutls_free(out.data);

	return;
}

static void decode(const char *test_name, gnutls_digest_algorithm_t hash, const gnutls_datum_t *raw, const gnutls_datum_t *di, int res)
{
	int ret;
	uint8_t digest[128];
	unsigned digest_size;
	gnutls_digest_algorithm_t thash;

	digest_size = sizeof(digest);
	ret = gnutls_decode_ber_digest_info(di, &thash, digest, &digest_size);
	if (res != ret) {
		fail("%s: gnutls_decode_ber_digest_info: %s\n", test_name, gnutls_strerror(ret));
		exit(1);
	}

	if (ret < 0) {
		return;
	}

	if (thash != hash) {
		fail("%s: gnutls_decode_ber_digest_info: wrong hash, got: %d, expected %d\n", test_name, (int)thash, (int)hash);
		exit(1);
	}

	if (raw->size!=digest_size) {
		fail("%s: gnutls_decode_ber_digest_info: output has incorrect size (%d, expected %d)\n", test_name, digest_size, raw->size);
		exit(1);
	}

	if (memcmp(raw->data, digest, digest_size) != 0) {
		fail("%s: gnutls_decode_ber_digest_info: output does not match the expected\n", test_name);
		exit(1);
	}

	return;
}

struct encode_tests_st {
	const char *name;
	gnutls_digest_algorithm_t hash;
	const gnutls_datum_t raw;
	const gnutls_datum_t di;
};

struct encode_tests_st encode_tests[] = {
	{
		.name = "rnd1",
		.hash = GNUTLS_DIG_SHA1,
		.raw = {(void*)"\xf6\x9a\x46\x8a\x84\x69\x7a\x28\x83\xda\x52\xcd\x60\x2f\x39\x78\xff\xa1\x32\x12", 20},
		.di = {(void*)"\x30\x21\x30\x09\x06\x05\x2b\x0e\x03\x02\x1a\x05\x00\x04\x14\xf6\x9a\x46\x8a\x84\x69\x7a\x28\x83\xda\x52\xcd\x60\x2f\x39\x78\xff\xa1\x32\x12",35}
	},
	{
		.name = "rnd2",
		.hash = GNUTLS_DIG_SHA256,
		.raw = {(void*)"\x0b\x68\xdf\x4b\x27\xac\xc5\xc5\x52\x43\x74\x32\x39\x5c\x1e\xf5\x6a\xe2\x19\x5a\x58\x75\x81\xa5\x6a\xf5\xbf\x98\x85\xe3\xf9\x25", 32},
		.di = {(void*)"\x30\x31\x30\x0d\x06\x09\x60\x86\x48\x01\x65\x03\x04\x02\x01\x05\x00\x04\x20\x0b\x68\xdf\x4b\x27\xac\xc5\xc5\x52\x43\x74\x32\x39\x5c\x1e\xf5\x6a\xe2\x19\x5a\x58\x75\x81\xa5\x6a\xf5\xbf\x98\x85\xe3\xf9\x25",51}
	}
};

struct decode_tests_st {
	const char *name;
	gnutls_digest_algorithm_t hash;
	const gnutls_datum_t raw;
	const gnutls_datum_t di;
	int res;
};

struct decode_tests_st decode_tests[] = {
	{
		.name = "dec-rnd1",
		.hash = GNUTLS_DIG_SHA1,
		.di = {(void*)"\x30\x21\x30\x09\x06\x05\x2b\x0e\x03\x02\x1a\x05\x00\x04\x14\xf6\x9a\x46\x8a\x84\x69\x7a\x28\x83\xda\x52\xcd\x60\x2f\x39\x78\xff\xa1\x32\x12",35},
		.raw = {(void*)"\xf6\x9a\x46\x8a\x84\x69\x7a\x28\x83\xda\x52\xcd\x60\x2f\x39\x78\xff\xa1\x32\x12", 20},
		.res = 0,
	},
	{
		.name = "dec-rnd2",
		.hash = GNUTLS_DIG_SHA256,
		.raw = {(void*)"\x0b\x68\xdf\x4b\x27\xac\xc5\xc5\x52\x43\x74\x32\x39\x5c\x1e\xf5\x6a\xe2\x19\x5a\x58\x75\x81\xa5\x6a\xf5\xbf\x98\x85\xe3\xf9\x25", 32},
		.di = {(void*)"\x30\x31\x30\x0d\x06\x09\x60\x86\x48\x01\x65\x03\x04\x02\x01\x05\x00\x04\x20\x0b\x68\xdf\x4b\x27\xac\xc5\xc5\x52\x43\x74\x32\x39\x5c\x1e\xf5\x6a\xe2\x19\x5a\x58\x75\x81\xa5\x6a\xf5\xbf\x98\x85\xe3\xf9\x25",51},
		.res = 0,
	},
	{
		.name = "dec-wrong-tag",
		.hash = GNUTLS_DIG_SHA256,
		.raw = {(void*)"\x0b\x68\xdf\x4b\x27\xac\xc5\xc5\x52\x43\x74\x32\x39\x5c\x1e\xf5\x6a\xe2\x19\x5a\x58\x75\x81\xa5\x6a\xf5\xbf\x98\x85\xe3\xf9\x25", 32},
		.di = {(void*)"\x31\x31\x30\x0d\x06\x09\x60\x86\x48\x01\x65\x03\x04\x02\x01\x05\x00\x04\x20\x0b\x68\xdf\x4b\x27\xac\xc5\xc5\x52\x43\x74\x32\x39\x5c\x1e\xf5\x6a\xe2\x19\x5a\x58\x75\x81\xa5\x6a\xf5\xbf\x98\x85\xe3\xf9\x25",51},
		.res = GNUTLS_E_ASN1_TAG_ERROR
	},
	{
		.name = "dec-wrong-der",
		.hash = GNUTLS_DIG_SHA256,
		.raw = {(void*)"\x0b\x68\xdf\x4b\x27\xac\xc5\xc5\x52\x43\x74\x32\x39\x5c\x1e\xf5\x6a\xe2\x19\x5a\x58\x75\x81\xa5\x6a\xf5\xbf\x98\x85\xe3\xf9\x25", 32},
		.di = {(void*)"\x30\x31\x30\x0c\x06\x09\x60\x86\x48\x01\x65\x03\x04\x02\x01\x05\x00\x04\x20\x0b\x68\xdf\x4b\x27\xac\xc5\xc5\x52\x43\x74\x32\x39\x5c\x1e\xf5\x6a\xe2\x19\x5a\x58\x75\x81\xa5\x6a\xf5\xbf\x98\x86\xe3\xf9\x25",51},
		.res = GNUTLS_E_ASN1_DER_ERROR
	},
	{
		.name = "dec-wrong-hash",
		.hash = GNUTLS_DIG_SHA256,
		.raw = {(void*)"\x0b\x68\xdf\x4b\x27\xac\xc5\xc5\x52\x43\x74\x32\x39\x5c\x1e\xf5\x6a\xe2\x19\x5a\x58\x75\x81\xa5\x6a\xf5\xbf\x98\x85\xe3\xf9\x25", 32},
		.di = {(void*)"\x30\x31\x30\x0d\x06\x09\x61\x86\x48\x01\x65\x03\x04\x02\x01\x05\x00\x04\x20\x0b\x68\xdf\x4b\x27\xac\xc5\xc5\x52\x43\x74\x32\x39\x5c\x1e\xf5\x6a\xe2\x19\x5a\x58\x75\x81\xa5\x6a\xf5\xbf\x98\x86\xe3\xf9\x25",51},
		.res = GNUTLS_E_UNKNOWN_HASH_ALGORITHM
	},
};

void doit(void)
{
	unsigned i;

	for (i=0;i<sizeof(encode_tests)/sizeof(encode_tests[0]);i++) {
		encode(encode_tests[i].name, encode_tests[i].hash, &encode_tests[i].raw, &encode_tests[i].di);
	}

	for (i=0;i<sizeof(decode_tests)/sizeof(decode_tests[0]);i++) {
		decode(decode_tests[i].name, decode_tests[i].hash, &decode_tests[i].raw, &decode_tests[i].di, decode_tests[i].res);
	}
}

