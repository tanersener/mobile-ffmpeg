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
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <gnutls/gnutls.h>
#include <stdint.h>
#include "../lib/tls13/psk_ext_parser.h"

#include "utils.h"

/* Tests the PSK-extension decoding part */

static void decode(const char *test_name, const gnutls_datum_t *raw, const gnutls_datum_t *id,
		   const gnutls_datum_t *b, unsigned idx, int res)
{
	int ret;
	psk_ext_parser_st p;
	psk_ext_iter_st iter;
	struct psk_st psk;
	gnutls_datum_t binder;
	unsigned found = 0;
	unsigned i, j;

	ret = _gnutls13_psk_ext_parser_init(&p, raw->data, raw->size);
	if (ret < 0) {
		if (res == ret) /* expected */
			return;
		fail("%s: _gnutls13_psk_ext_parser_init: %d/%s\n", test_name, ret, gnutls_strerror(ret));
		exit(1);
	}

	_gnutls13_psk_ext_iter_init(&iter, &p);
	for (i = 0; ; i++) {
		ret = _gnutls13_psk_ext_iter_next_identity(&iter, &psk);
		if (ret < 0) {
			if (ret == GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE)
				break;
			if (res == ret) /* expected */
				return;
		}
		if (i == idx) {
			if (psk.identity.size == id->size && memcmp(psk.identity.data, id->data, id->size) == 0) {
				if (debug)
					success("%s: found id\n", test_name);
				found = 1;
				break;
			} else {
				fail("%s: did not found identity on index %d\n", test_name, idx);
			}
		}
	}

	if (found == 0)
		fail("%s: did not found identity!\n", test_name);

	_gnutls13_psk_ext_iter_init(&iter, &p);
	for (j = 0; j <= i; j++) {
		ret = _gnutls13_psk_ext_iter_next_binder(&iter, &binder);
		if (ret < 0) {
			if (res == ret) /* expected */
				return;
			fail("%s: could not extract binder: %s\n",
			     test_name, gnutls_strerror(ret));
		}
	}

	if (debug)
		success("%s: found binder\n", test_name);

	if (binder.size != b->size || memcmp(binder.data, b->data, b->size) != 0) {
		hexprint(binder.data, binder.size);
		fail("%s: did not match binder on index %d\n", test_name, idx);
	}

	return;
}

struct decode_tests_st {
	const char *name;
	gnutls_datum_t psk;
	unsigned idx; /* the ID index */
	gnutls_datum_t id;
	gnutls_datum_t binder;
	int res;
};

struct decode_tests_st decode_tests[] = {
	{
		.name = "single PSK",
		.psk = { (unsigned char*)"\x00\x0a\x00\x04\x6e\x6d\x61\x76\x00\x00\x00\x00\x00\x21\x20\xc4\xda\xe5\x7e\x05\x59\xf7\xae\x9b\xba\x90\xd2\x6e\x12\x68\xf6\xc1\xc7\xb9\x7e\xdc\xed\x9e\x67\x4e\xa5\x91\x2d\x7c\xb4\xf0\xab", 47},
		.id = { (unsigned char*)"nmav", 4 },
		.binder = { (unsigned char*)"\xc4\xda\xe5\x7e\x05\x59\xf7\xae\x9b\xba\x90\xd2\x6e\x12\x68\xf6\xc1\xc7\xb9\x7e\xdc\xed\x9e\x67\x4e\xa5\x91\x2d\x7c\xb4\xf0\xab", 32 },
		.idx = 0,
		.res = 0
	},
	{
		.name = "multiple psks id0",
		.psk = { (unsigned char*)"\x00\x20\x00\x04\x70\x73\x6b\x31\x00\x00\x00\x00"
				"\x00\x06\x70\x73\x6b\x69\x64\x00\x00\x00\x00\x00"
				"\x00\x04\x74\x65\x73\x74\x00\x00\x00\x00\x00\x63"
				"\x20\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
				"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
				"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x20\x01\x00"
				"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
				"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
				"\x00\x00\x00\x00\x00\x00\x20\x71\x83\x89\x3d\xcc"
				"\x46\xad\x83\x18\x98\x59\x46\x0b\xb2\x51\x24\x53"
				"\x41\xb4\x35\x04\x22\x90\x02\xac\x5e\xc1\xe7\xbc"
				"\xca\x52\x16", 135},
		.id = { (unsigned char*)"psk1", 4 },
		.binder = { (unsigned char*)"\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 32},
		.idx = 0,
		.res = 0
	},
	{
		.name = "multiple psks id1",
		.psk = { (unsigned char*)"\x00\x20\x00\x04\x70\x73\x6b\x31\x00\x00\x00\x00"
				"\x00\x06\x70\x73\x6b\x69\x64\x00\x00\x00\x00\x00"
				"\x00\x04\x74\x65\x73\x74\x00\x00\x00\x00\x00\x63"
				"\x20\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
				"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
				"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x20\x01\x00"
				"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
				"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
				"\x00\x00\x00\x00\x00\x00\x20\x71\x83\x89\x3d\xcc"
				"\x46\xad\x83\x18\x98\x59\x46\x0b\xb2\x51\x24\x53"
				"\x41\xb4\x35\x04\x22\x90\x02\xac\x5e\xc1\xe7\xbc"
				"\xca\x52\x16", 135},
		.id = { (unsigned char*)"pskid", 6 },
		.binder = { (unsigned char*)"\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 32},
		.idx = 1,
		.res = 0
	},
	{
		.name = "multiple psks id2",
		.psk = { (unsigned char*)"\x00\x20\x00\x04\x70\x73\x6b\x31\x00\x00\x00\x00"
				"\x00\x06\x70\x73\x6b\x69\x64\x00\x00\x00\x00\x00"
				"\x00\x04\x74\x65\x73\x74\x00\x00\x00\x00\x00\x63"
				"\x20\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
				"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
				"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x20\x01\x00"
				"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
				"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
				"\x00\x00\x00\x00\x00\x00\x20\x71\x83\x89\x3d\xcc"
				"\x46\xad\x83\x18\x98\x59\x46\x0b\xb2\x51\x24\x53"
				"\x41\xb4\x35\x04\x22\x90\x02\xac\x5e\xc1\xe7\xbc"
				"\xca\x52\x16", 135},
		.id = { (unsigned char*)"test", 4 },
		.binder = { (unsigned char*)"\x71\x83\x89\x3d\xcc\x46\xad\x83\x18\x98\x59\x46\x0b\xb2\x51\x24\x53\x41\xb4\x35\x04\x22\x90\x02\xac\x5e\xc1\xe7\xbc\xca\x52\x16", 32},
		.idx = 2,
		.res = 0
	},
	{
		.name = "multiple psks id3",
		.psk = { (unsigned char*)"\x00\x20\x00\x04\x70\x73\x6b\x31\x00\x00\x00\x00"
				"\x00\x06\x70\x73\x6b\x69\x64\x00\x00\x00\x00\x00"
				"\x00\x04\x74\x65\x73\x74\x00\x00\x00\x00\x00\x42"
				"\x20\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
				"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
				"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x20\x01\x00"
				"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
				"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
				"\x00\x00\x00\x00\x00\x00", 102},
		.id = { (unsigned char*)"test", 4 },
		.binder = { NULL, 0 },
		.idx = 2,
		.res = GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE
	}
};

void doit(void)
{
	unsigned i;

	for (i=0;i<sizeof(decode_tests)/sizeof(decode_tests[0]);i++) {
		decode(decode_tests[i].name, &decode_tests[i].psk, &decode_tests[i].id,
		       &decode_tests[i].binder, decode_tests[i].idx, decode_tests[i].res);
	}
}

