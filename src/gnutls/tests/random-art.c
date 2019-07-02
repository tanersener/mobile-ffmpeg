/*
 * Copyright (C) 2016, 2017 Red Hat, Inc.
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

/* This checks random art encoding */

static void encode(const char *test_name, const char *type, unsigned key_size, unsigned char *input, unsigned input_size, const char *expected)
{
	int ret;
	gnutls_datum_t out;

	ret = gnutls_random_art(GNUTLS_RANDOM_ART_OPENSSH, type, key_size, input, input_size, &out);
	if (ret < 0) {
		fail("%s: gnutls_random_art: %s\n", test_name, gnutls_strerror(ret));
		exit(1);
	}

	if (strlen(expected)!=out.size) {
		fail("%s: gnutls_random_art: output has incorrect size (%d, expected %d)\n%s\n", test_name, (int)out.size, (int)strlen(expected), out.data);
		exit(1);
	}

	if (strncasecmp(expected, (char*)out.data, out.size) != 0) {
		fail("%s: gnutls_random_art: output does not match the expected:\n%s\n", test_name, out.data);
		exit(1);
	}

	gnutls_free(out.data);

	return;
}


struct encode_tests_st {
	const char *name;
	unsigned char *raw;
	unsigned raw_size;
	const char *key_type;
	unsigned key_size;
	const char *art;
};

struct encode_tests_st encode_tests[] = {
	{
		.name = "key1",
		.raw = (void*)"\x38\x17\x0c\x08\xcb\x45\x8f\xd4\x87\x9c\x34\xb6\xf6\x08\x29\x4c\x50\x31\x2b\xbb",
		.raw_size = 20,
		.key_type = "RSA",
		.key_size = 2048,
		.art =	"+--[ RSA 2048]----+\n"
			"|.o*++==o         |\n"
			"| + *.===.        |\n"
			"|. * + +.o        |\n"
			"| o . o + .       |\n"
			"|.     + S        |\n"
			"| .     o         |\n"
			"|E                |\n"
			"|                 |\n"
			"|                 |\n"
			"+-----------------+"
	},
	{
		.name = "key2",
		.raw = (void*)"\xf8\xa7\x1c\x08\x76\x47\x2c\x08\x38\x17\x0c\x08\x38\x17\x0c\x08\xcb\x45\x8f\xd4\x87\x9c\xa4\xb6\xf6\xf8\x29\xfc\x50\x3f\x2b\xbb",
		.raw_size = 32,
		.key_type = "RSA",
		.key_size = 3072,
		.art =	"+--[ RSA 3072]----+\n"
			"|@*=*+.o          |\n"
			"|O.B.+* o         |\n"
			"|.* +..o o        |\n"
			"|  . .  +         |\n"
			"|   oo.o S        |\n"
			"|  ..+o.+         |\n"
			"|  .o ..oo .      |\n"
			"|   oo...o+       |\n"
			"|    oE+.o        |\n"
			"+-----------------+"
	},
	{
		.name = "key3",
		.raw = (void*)"\x38\xf7\x0c\x08\xcb\x34\x8a\xd4\xb7\x9c\x34\xb4\xf6\x08\x29\x4c\x50\x3f\x2b\xbb",
		.raw_size = 20,
		.key_type = "ECDSA",
		.key_size = 256,
		.art =	"+--[ECDSA  256]---+\n"
			"|oo.  .           |\n"
			"|o ..o .          |\n"
			"| + +**           |\n"
			"|...+***o         |\n"
			"|. o +=+.S        |\n"
			"|   o   o +       |\n"
			"|  .       o      |\n"
			"|   .             |\n"
			"|  E              |\n"
			"+-----------------+"
	}
};

void doit(void)
{
	unsigned i;

	for (i=0;i<sizeof(encode_tests)/sizeof(encode_tests[0]);i++) {
		encode(encode_tests[i].name, encode_tests[i].key_type, encode_tests[i].key_size,
			encode_tests[i].raw, encode_tests[i].raw_size, encode_tests[i].art);
	}
}

