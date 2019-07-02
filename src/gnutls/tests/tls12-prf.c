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

#define MATCH_FUNC_SHA256(fname, dsecret, dseed, dlabel, doutput) \
static void fname(void **glob_state) \
{ \
	char tmp[512]; \
	gnutls_datum_t secret = dsecret; \
	gnutls_datum_t seed = dseed; \
	gnutls_datum_t label = dlabel; \
	gnutls_datum_t output = doutput; \
	int _rval; \
	_rval = _gnutls_prf_raw(GNUTLS_MAC_SHA256, secret.size, secret.data, \
		label.size, (char*)label.data, seed.size, seed.data, output.size, tmp); \
	assert_int_equal(_rval, 0); \
	assert_int_equal(memcmp(tmp, output.data, output.size), 0); \
	gnutls_free(secret.data); \
	gnutls_free(label.data); \
	gnutls_free(seed.data); \
	gnutls_free(output.data); \
}

#define MATCH_FUNC_SHA384(fname, dsecret, dseed, dlabel, doutput) \
static void fname(void **glob_state) \
{ \
	char tmp[512]; \
	gnutls_datum_t secret = dsecret; \
	gnutls_datum_t seed = dseed; \
	gnutls_datum_t label = dlabel; \
	gnutls_datum_t output = doutput; \
	int _rval; \
	_rval = _gnutls_prf_raw(GNUTLS_MAC_SHA384, secret.size, secret.data, \
		label.size, (char*)label.data, seed.size, seed.data, output.size, tmp); \
	assert_int_equal(_rval, 0); \
	assert_int_equal(memcmp(tmp, output.data, output.size), 0); \
	gnutls_free(secret.data); \
	gnutls_free(label.data); \
	gnutls_free(seed.data); \
	gnutls_free(output.data); \
}

MATCH_FUNC_SHA256(sha256_test1, SHEX("0450b0ea9ecd3602ee0d76c5c3c86f4a"),
	SHEX("207acc0254b867f5b925b45a33601d8b"),
	SDATA("test label"), SHEX("ae679e0e714f5975763768b166979e1d"));

MATCH_FUNC_SHA256(sha256_test2, SHEX("34204a9df0be6eb4e925a8027cf6c602"),
	SHEX("98b2c40bcd664c83bb920c18201a6395"),
	SDATA("test label"), SHEX("afa9312453c22fa83d2b511b372d73a402a2a62873239a51fade45082faf3fd2bb7ffb3e9bf36e28b3141aaba484005332a9f9e388a4d329f1587a4b317da07708ea1ba95a53f8786724bd83ce4b03af"));

MATCH_FUNC_SHA256(sha256_test3, SHEX("a3691aa1f6814b80592bf1cf2acf1697"),
	SHEX("5523d41e320e694d0c1ff5734d830b933e46927071c92621"),
	SDATA("test label"), SHEX("6ad0984fa06f78fe161bd46d7c261de43340d728dddc3d0ff0dd7e0d"));

MATCH_FUNC_SHA256(sha256_test4, SHEX("210ec937069707e5465bc46bf779e104108b18fdb793be7b218dbf145c8641f3"), SHEX("1e351a0baf35c79945924394b881cfe31dae8f1c1ed54d3b"),
	SDATA("test label"), SHEX("7653fa809cde3b553c4a17e2cdbcc918f36527f22219a7d7f95d97243ff2d5dee8265ef0af03"));

/* https://www.ietf.org/mail-archive/web/tls/current/msg03416.html */
MATCH_FUNC_SHA384(sha384_test1, SHEX("b80b733d6ceefcdc71566ea48e5567df"), SHEX("cd665cf6a8447dd6ff8b27555edb7465"),
	SDATA("test label"), SHEX("7b0c18e9ced410ed1804f2cfa34a336a1c14dffb4900bb5fd7942107e81c83cde9ca0faa60be9fe34f82b1233c9146a0e534cb400fed2700884f9dc236f80edd8bfa961144c9e8d792eca722a7b32fc3d416d473ebc2c5fd4abfdad05d9184259b5bf8cd4d90fa0d31e2dec479e4f1a26066f2eea9a69236a3e52655c9e9aee691c8f3a26854308d5eaa3be85e0990703d73e56f"));


int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(sha256_test1),
		cmocka_unit_test(sha256_test2),
		cmocka_unit_test(sha256_test3),
		cmocka_unit_test(sha256_test4),
		cmocka_unit_test(sha384_test1),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
