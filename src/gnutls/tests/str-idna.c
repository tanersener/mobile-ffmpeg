/*
 * Copyright (C) 2016, 2017 Red Hat, Inc.
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
 * You should have received a copy of the GNU General Public License
 * along with GnuTLS; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <config.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gnutls/gnutls.h>
#include <cmocka.h>

# define GLOBAL_FLAGS 0

#define MATCH_FUNC(fname, str, normalized) \
static void fname(void **glob_state) \
{ \
	gnutls_datum_t out; \
	int ret = gnutls_idna_map(str, strlen(str), &out, GLOBAL_FLAGS); \
	if (normalized == NULL) { /* expect failure */ \
		assert_int_not_equal(ret, 0); \
		return; \
	} else { \
		assert_int_equal(ret, 0); \
	} \
	assert_int_equal(strcmp((char*)out.data, (char*)normalized), 0); \
	gnutls_free(out.data); \
}

#define MATCH_FUNC_TWO_WAY(fname, str, normalized) \
static void fname##_reverse(void **glob_state) \
{ \
	gnutls_datum_t out; \
	int ret; \
	if (normalized == NULL) \
		return; \
	ret = gnutls_idna_reverse_map(normalized, strlen(normalized), &out, 0); \
	assert_int_equal(ret, 0); \
	\
	assert_int_equal(strcmp((char*)out.data, (char*)str), 0); \
	gnutls_free(out.data); \
} \
MATCH_FUNC(fname, str, normalized)

#define EMPTY_FUNC(name) static void name(void **glob_state) { \
	return; }

/* some vectors taken from:
 * http://www.unicode.org/Public/idna/9.0.0/IdnaTest.txt
 */

MATCH_FUNC_TWO_WAY(test_ascii, "localhost", "localhost");
MATCH_FUNC_TWO_WAY(test_ascii_caps, "LOCALHOST", "LOCALHOST");
MATCH_FUNC_TWO_WAY(test_greek1, "βόλοσ.com", "xn--nxasmq6b.com");
MATCH_FUNC_TWO_WAY(test_mix, "简体中文.εξτρα.com", "xn--fiqu1az03c18t.xn--mxah1amo.com");
MATCH_FUNC_TWO_WAY(test_german4, "bücher.de", "xn--bcher-kva.de");
MATCH_FUNC_TWO_WAY(test_u1, "夡夞夜夙", "xn--bssffl");
MATCH_FUNC_TWO_WAY(test_jp2, "日本語.jp", "xn--wgv71a119e.jp");
/* invalid (✌️) symbol in IDNA2008 but valid in IDNA2003. Browsers
 * fallback to IDNA2003, and we do too, so that should work */
#if IDN2_VERSION_NUMBER >= 0x02000002
MATCH_FUNC_TWO_WAY(test_valid_idna2003, "\xe2\x9c\x8c\xef\xb8\x8f.com", "xn--7bi.com");
#else
EMPTY_FUNC(test_valid_idna2003);
#endif

MATCH_FUNC_TWO_WAY(test_greek2, "βόλος.com", "xn--nxasmm1c.com");
MATCH_FUNC_TWO_WAY(test_german1, "faß.de", "xn--fa-hia.de");
# if IDN2_VERSION_NUMBER >= 0x00140000
MATCH_FUNC(test_caps_greek, "ΒΌΛΟΣ.com", "xn--nxasmq6b.com");
MATCH_FUNC(test_caps_german1, "Ü.ü", "xn--tda.xn--tda");
MATCH_FUNC(test_caps_german2, "Bücher.de", "xn--bcher-kva.de");
MATCH_FUNC(test_caps_german3, "Faß.de", "xn--fa-hia.de");
MATCH_FUNC(test_dots, "a.b.c。d。", "a.b.c.d.");

/* without STD3 ASCII rules, the result is: evil.ca/c..example.com */
MATCH_FUNC(test_evil, "evil.c\u2100.example.com", "evil.c.example.com");
# else
EMPTY_FUNC(test_caps_german1);
EMPTY_FUNC(test_caps_german2);
EMPTY_FUNC(test_caps_german3);
EMPTY_FUNC(test_caps_greek);
EMPTY_FUNC(test_dots);
EMPTY_FUNC(test_evil);
# endif

int main(void)
{
	gnutls_datum_t tmp;
	int ret;
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_greek2_reverse),
		cmocka_unit_test(test_german1_reverse),
		cmocka_unit_test(test_ascii),
		cmocka_unit_test(test_ascii_reverse),
		cmocka_unit_test(test_ascii_caps),
		cmocka_unit_test(test_ascii_caps_reverse),
		cmocka_unit_test(test_greek1),
		cmocka_unit_test(test_greek1_reverse),
		cmocka_unit_test(test_greek2),
		cmocka_unit_test(test_caps_greek),
		cmocka_unit_test(test_mix),
		cmocka_unit_test(test_mix_reverse),
		cmocka_unit_test(test_german1),
		cmocka_unit_test(test_caps_german1),
		cmocka_unit_test(test_caps_german2),
		cmocka_unit_test(test_caps_german3),
		cmocka_unit_test(test_german4),
		cmocka_unit_test(test_german4_reverse),
		cmocka_unit_test(test_u1),
		cmocka_unit_test(test_u1_reverse),
		cmocka_unit_test(test_jp2),
		cmocka_unit_test(test_jp2_reverse),
		cmocka_unit_test(test_dots),
		cmocka_unit_test(test_evil),
		cmocka_unit_test(test_valid_idna2003)
	};

	ret = gnutls_idna_map("β", strlen("β"), &tmp, GLOBAL_FLAGS);
	if (ret == GNUTLS_E_UNIMPLEMENTED_FEATURE)
		exit(77);
	else if (ret < 0) {
		fprintf(stderr, "error: %s\n", gnutls_strerror(ret));
		exit(1);
	}
	gnutls_free(tmp.data);

	return cmocka_run_group_tests(tests, NULL, NULL);
}
