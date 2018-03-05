/*
 * Copyright (C) 2007-2012 Free Software Foundation, Inc.
 *
 * Author: Simon Josefsson
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

#include <utils.h>
#include "../lib/gnutls_int.h"
#include "../lib/x509/x509_int.h"
#include "../lib/debug.h"

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "|<%d>| %s", level, str);
}

static const unsigned char *salt[3] =
    { (void *) "salt1", (void *) "ltsa22", (void *) "balt33" };
static const char *pw[3] = { "secret1", "verysecret2", "veryverysecret3" };

static const char *values[] = {
/* 1.0 */
	"85a3c676a66f0960f4807144a28c8d61a0001b81846f301a1ac164289879972f",
/* 1.2 */
	"e659da7d5989733a3d268e0bf7752c35c116e5c75919449a98f6812f82a15b16",
/* 1.2 */
	"878b8a88bf6166ce803b7498822205b1ac82870d3aec20807148779375a61f1e",
/* 2.0 */
	"1c845be764371d633c7fd1056967a9940385e110e85b58f826d39ae8561a0019",
/* 2.1 */
	"de8dd3ffd59b65d3d5f59a1f71d7add582741f7752a786c045953e727e4465c0",
/* 2.2 */
#ifndef PKCS12_BROKEN_KEYGEN
	"9dd7f19e5e6aee5c5008b5deefd35889ab75193594ed49a605df4e93e7c2a155",
#else
	"9dd7f19e5e6aee5c5008b5deefd35889ab7519356f13478ecdee593c5ed689b1",
#endif
	/* 3.0 */
	"1c165e5a291a1539f3dbcf82a3e6ed566eb9d50ad4b0b3b57b599b08f0531236",
/* 3.1 */
	"5c9abee3cde31656eedfc131b7c2f8061032a3c705961ee2306a826c8b4b1a76",
/* 3.2 */
	"a9c94e0acdaeaea54d1b1b681c3b64916396a352dea7ffe635fb2c11d8502e98"
};

/* Values derived from
   http://www.drh-consultancy.demon.co.uk/test.txt */
static struct {
	int id;
	const char *password;
	const unsigned char *salt;
	size_t iter;
	size_t keylen;
	const char *key;
} tv[] = {
	{
	1, "smeg", (void *) "\x0A\x58\xCF\x64\x53\x0D\x82\x3F", 1,
		    24,
		    "8aaae6297b6cb04642ab5b077851284eb7128f1a2a7fbca3"}, {
	2, "smeg", (void *) "\x0A\x58\xCF\x64\x53\x0D\x82\x3F", 1,
		    8, "79993dfe048d3b76"}, {
	1, "smeg", (void *) "\x64\x2B\x99\xAB\x44\xFB\x4B\x1F", 1,
		    24,
		    "f3a95fec48d7711e985cfe67908c5ab79fa3d7c5caa5d966"}, {
	2, "smeg", (void *) "\x64\x2B\x99\xAB\x44\xFB\x4B\x1F", 1,
		    8, "c0a38d64a79bea1d"}, {
	3, "smeg", (void *) "\x3D\x83\xC0\xE4\x54\x6A\xC1\x40", 1,
		    20, "8d967d88f6caa9d714800ab3d48051d63f73a312"}, {
	1, "queeg", (void *) "\x05\xDE\xC9\x59\xAC\xFF\x72\xF7",
		    1000, 24,
		    "ed2034e36328830ff09df1e1a07dd357185dac0d4f9eb3d4"}, {
	2, "queeg", (void *) "\x05\xDE\xC9\x59\xAC\xFF\x72\xF7",
		    1000, 8, "11dedad7758d4860"}, {
	1, "queeg", (void *) "\x16\x82\xC0\xFC\x5B\x3F\x7E\xC5",
		    1000, 24,
		    "483dd6e919d7de2e8e648ba8f862f3fbfbdc2bcb2c02957f"}, {
	2, "queeg", (void *) "\x16\x82\xC0\xFC\x5B\x3F\x7E\xC5",
		    1000, 8, "9d461d1b00355c50"}, {
	3, "queeg", (void *) "\x26\x32\x16\xFC\xC2\xFA\xB3\x1C",
		    1000, 20, "5ec4c7a80df652294c3925b6489a7ab857c83476"}
};

void doit(void)
{
	int rc;
	unsigned int i, j, x;
	unsigned char key[32];
	char tmp[1024];

	global_init();

	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(99);

	x = 0;
	for (i = 1; i < 4; i++) {
		for (j = 0; j < 3; j++) {
			rc = _gnutls_pkcs12_string_to_key(mac_to_entry(GNUTLS_MAC_SHA1), i, salt[j],
							  strlen((char *)
								 salt[j]),
							  j + i + 15,
							  pw[j],
							  sizeof(key),
							  key);
			if (rc < 0)
				fail("_gnutls_pkcs12_string_to_key failed[0]: %d\n", rc);

			if (strcmp(_gnutls_bin2hex(key, sizeof(key),
						   tmp, sizeof(tmp), NULL),
				   values[x]) != 0)
				fail("_gnutls_pkcs12_string_to_key failed[1]\n");

			if (debug)
				printf("ij: %d.%d: %s\n", i, j,
					_gnutls_bin2hex(key, sizeof(key),
							tmp, sizeof(tmp),
							NULL));
			x++;
		}
	}
	if (debug)
		printf("\n");

	for (i = 0; i < sizeof(tv) / sizeof(tv[0]); i++) {
		rc = _gnutls_pkcs12_string_to_key(mac_to_entry(GNUTLS_MAC_SHA1), tv[i].id, tv[i].salt, 8,
						  tv[i].iter,
						  tv[i].password,
						  tv[i].keylen, key);
		if (rc < 0)
			fail("_gnutls_pkcs12_string_to_key failed[2]: %d\n", rc);

		if (memcmp(_gnutls_bin2hex(key, tv[i].keylen,
					   tmp, sizeof(tmp), NULL),
			   tv[i].key, tv[i].keylen) != 0)
			fail("_gnutls_pkcs12_string_to_key failed[3]\n");

		if (debug)
			printf("tv[%d]: %s\n", i,
				_gnutls_bin2hex(key, tv[i].keylen, tmp,
						sizeof(tmp), NULL));
	}
	if (debug)
		printf("\n");

	gnutls_global_deinit();

	if (debug)
		success("_gnutls_pkcs12_string_to_key ok\n");
}
