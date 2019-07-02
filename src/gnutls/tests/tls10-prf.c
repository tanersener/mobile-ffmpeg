/*
 * Copyright (C) 2017 Red Hat, Inc.
 *
 * Authors: Nikos Mavrogiannopoulos
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

#include <config.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <gnutls/gnutls.h>
#include <cmocka.h>
#include "hex.h"

int
_gnutls_prf_raw(gnutls_mac_algorithm_t mac,
		size_t master_size, const void *master,
		size_t label_size, const char *label,
		size_t seed_size, const uint8_t *seed, size_t outsize,
		char *out);

#define MATCH_FUNC(fname, dsecret, dseed, dlabel, doutput) \
static void fname(void **glob_state) \
{ \
	char tmp[512]; \
	gnutls_datum_t secret = dsecret; \
	gnutls_datum_t seed = dseed; \
	gnutls_datum_t label = dlabel; \
	gnutls_datum_t output = doutput; \
	int _rval; \
	_rval = _gnutls_prf_raw(GNUTLS_MAC_MD5_SHA1, secret.size, secret.data, \
		label.size, (char*)label.data, seed.size, seed.data, output.size, tmp); \
	assert_int_equal(_rval, 0); \
	assert_int_equal(memcmp(tmp, output.data, output.size), 0); \
	gnutls_free(secret.data); \
	gnutls_free(label.data); \
	gnutls_free(seed.data); \
	gnutls_free(output.data); \
}


MATCH_FUNC(test1, SHEX("263bdbbb6f6d4c664e058d0aa9d321be"), SHEX("b920573b199601024f04d6dc61966e65"),
	SDATA("test label"), SHEX("6617993765fa6ca703d19ec70dd5dd160ffcc07725fafb714a9f815a2a30bfb7e3bbfb7eee574b3b613eb7fe80eec9691d8c1b0e2d9b3c8b4b02b6b6d6db88e2094623ef6240607eda7abe3c846e82a3"));
MATCH_FUNC(test2, SHEX("bf31fe6c78ebf0ff9ce8bb5dd9d1f83d"), SHEX("7fc4583d19871d962760f358a18696c8"),
	SDATA("test label"), SHEX("8318f382c49fd5af7d6fdb4cbb31dfef"));
MATCH_FUNC(test3, SHEX("0addfc84435b9ac1ef523ef44791a784bf55757dea17837c1a72beec1bdb1850"),
	SHEX("74e849d11ad8a98d9bc2291dbceec26ff9"),
	SDATA("test label"), SHEX("3c221520c48bcb3a0eb3734a"));
MATCH_FUNC(test4, SHEX("4074939b440a08a285bc7208485c531f0bbd4c101d71bdba33ec066791e4678c"),
	SHEX("8aff0c770c1d60455ee48f220c9adb471e5fee27c88c1f33"),
	SDATA("test label"), SHEX("3a9aee040bbf3cf7009210e64bbdad1775ccf1b46b3a965d5f15168e9ddaa7cc6a7c0c117848"));

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test1),
		cmocka_unit_test(test2),
		cmocka_unit_test(test3),
		cmocka_unit_test(test4),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
