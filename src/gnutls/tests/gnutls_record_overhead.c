/*
 * Copyright (C) 2017 Red Hat, Inc.
 *
 * Author: Nikos Mavrogiannopoulos
 *
 * This file is part of GnuTLS.
 *
 * GnuTLS is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GnuTLS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GnuTLS.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* This is a unit test of _gnutls_record_overhead. */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <errno.h>
#include <stdint.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>


#include <gnutls/gnutls.h>
#include "../lib/gnutls_int.h"

#undef _gnutls_debug_log
#undef gnutls_assert
#undef gnutls_assert_val
#define _gnutls_debug_log printf
#define gnutls_assert()
#define gnutls_assert_val(val) val

/* #pragma doesn't work to suppress preprocessor warnings like -Wunused-macros.
 * So we just use the above defined macros here. */
#if defined _gnutls_debug_log && defined gnutls_assert && defined gnutls_assert_val
#include "../lib/algorithms.h"
#endif

unsigned _gnutls_record_overhead(const version_entry_st *ver,
				 const cipher_entry_st *cipher,
				 const mac_entry_st *mac,
				 unsigned max);

#define OVERHEAD(v, c, m)						\
	_gnutls_record_overhead(version_to_entry(v), cipher_to_entry(c), mac_to_entry(m), \
				0)

#define MAX_OVERHEAD(v, c, m)						\
	_gnutls_record_overhead(version_to_entry(v), cipher_to_entry(c), mac_to_entry(m), \
				1)

static void check_aes_gcm(void **glob_state)
{
	const unsigned ov = 16+8;
	/* Under AES-GCM the overhead is constant */
	assert_int_equal(OVERHEAD(GNUTLS_TLS1_2, GNUTLS_CIPHER_AES_128_GCM, GNUTLS_MAC_AEAD), ov);
	assert_int_equal(MAX_OVERHEAD(GNUTLS_TLS1_2, GNUTLS_CIPHER_AES_128_GCM, GNUTLS_MAC_AEAD), ov);
}

static void check_tls13_aes_gcm(void **glob_state)
{
	const unsigned ov = 16+1;
	/* Under AES-GCM the overhead is constant */
	assert_int_equal(OVERHEAD(GNUTLS_TLS1_3, GNUTLS_CIPHER_AES_128_GCM, GNUTLS_MAC_AEAD), ov);
	assert_int_equal(MAX_OVERHEAD(GNUTLS_TLS1_3, GNUTLS_CIPHER_AES_128_GCM, GNUTLS_MAC_AEAD), ov);
}

static void check_aes_sha1_min(void **glob_state)
{
	const unsigned mac = 20;
	const unsigned block = 16;
	assert_int_equal(OVERHEAD(GNUTLS_TLS1_2, GNUTLS_CIPHER_AES_128_CBC, GNUTLS_MAC_SHA1), 1+mac+block);
}

static void check_aes_sha1_max(void **glob_state)
{
	const unsigned mac = 20;
	const unsigned block = 16;

	assert_int_equal(MAX_OVERHEAD(GNUTLS_TLS1_2, GNUTLS_CIPHER_AES_128_CBC, GNUTLS_MAC_SHA1), block+mac+block);
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(check_aes_gcm),
		cmocka_unit_test(check_tls13_aes_gcm),
		cmocka_unit_test(check_aes_sha1_min),
		cmocka_unit_test(check_aes_sha1_max)
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
