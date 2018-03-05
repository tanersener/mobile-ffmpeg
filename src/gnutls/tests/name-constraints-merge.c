/*
 * Copyright (C) 2016 Red Hat, Inc.
 *
 * Authors: Nikos Mavrogiannopoulos, Martin Ukrop
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
#include <sys/types.h>
#include <unistd.h>
#include "../lib/gnutls_int.h"
#include "../lib/x509/x509_int.h"

#include "utils.h"

/* Test for name constraints PKIX extension.
 */

static void check_for_error(int ret) {
	if (ret != GNUTLS_E_SUCCESS)
		fail("error in %d: %s\n", __LINE__, gnutls_strerror(ret));
}

#define NAME_ACCEPTED 1
#define NAME_REJECTED 0

static void check_test_result(int suite, int ret, int expected_outcome,
							  gnutls_datum_t *tested_data) {
	if (expected_outcome == NAME_ACCEPTED ? ret == 0 : ret != 0) {
		if (expected_outcome == NAME_ACCEPTED) {
			fail("Checking \"%.*s\" should have succeeded (suite %d).\n",
				 tested_data->size, tested_data->data, suite);
		} else {
			fail("Checking \"%.*s\" should have failed (suite %d).\n",
				 tested_data->size, tested_data->data, suite);
		}
	}
}

static void set_name(const char *name, gnutls_datum_t *datum) {
	datum->data = (unsigned char*) name;
	datum->size = strlen((char*) name);
}

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "<%d>| %s", level, str);
}

void doit(void)
{
	int ret, suite;
	gnutls_x509_name_constraints_t nc1, nc2;
	gnutls_datum_t name;

	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(1000);

	/* 0: test the merge permitted name constraints
	 * NC1: permitted DNS org
	 *      permitted DNS ccc.com
	 *      permitted email ccc.com
	 * NC2: permitted DNS org
	 *      permitted DNS aaa.bbb.ccc.com
	 */
	suite = 0;

	ret = gnutls_x509_name_constraints_init(&nc1);
	check_for_error(ret);

	ret = gnutls_x509_name_constraints_init(&nc2);
	check_for_error(ret);

	set_name("org", &name);
	ret = gnutls_x509_name_constraints_add_permitted(nc1, GNUTLS_SAN_DNSNAME, &name);
	check_for_error(ret);

	set_name("ccc.com", &name);
	ret = gnutls_x509_name_constraints_add_permitted(nc1, GNUTLS_SAN_DNSNAME, &name);
	check_for_error(ret);

	set_name("ccc.com", &name);
	ret = gnutls_x509_name_constraints_add_permitted(nc1, GNUTLS_SAN_RFC822NAME, &name);
	check_for_error(ret);

	set_name("org", &name);
	ret = gnutls_x509_name_constraints_add_permitted(nc2, GNUTLS_SAN_DNSNAME, &name);
	check_for_error(ret);

	set_name("aaa.bbb.ccc.com", &name);
	ret = gnutls_x509_name_constraints_add_permitted(nc2, GNUTLS_SAN_DNSNAME, &name);
	check_for_error(ret);

	ret = _gnutls_x509_name_constraints_merge(nc1, nc2);
	check_for_error(ret);

	/* unrelated */
	set_name("xxx.example.com", &name);
	ret = gnutls_x509_name_constraints_check(nc1, GNUTLS_SAN_DNSNAME, &name);
	check_test_result(suite, ret, NAME_REJECTED, &name);

	set_name("example.org", &name);
	ret = gnutls_x509_name_constraints_check(nc1, GNUTLS_SAN_DNSNAME, &name);
	check_test_result(suite, ret, NAME_ACCEPTED, &name);

	set_name("com", &name);
	ret = gnutls_x509_name_constraints_check(nc1, GNUTLS_SAN_DNSNAME, &name);
	check_test_result(suite, ret, NAME_REJECTED, &name);

	set_name("xxx.com", &name);
	ret = gnutls_x509_name_constraints_check(nc1, GNUTLS_SAN_DNSNAME, &name);
	check_test_result(suite, ret, NAME_REJECTED, &name);

	set_name("ccc.com", &name);
	ret = gnutls_x509_name_constraints_check(nc1, GNUTLS_SAN_DNSNAME, &name);
	check_test_result(suite, ret, NAME_REJECTED, &name);

	/* check intersection of permitted */
	set_name("xxx.aaa.bbb.ccc.com", &name);
	ret = gnutls_x509_name_constraints_check(nc1, GNUTLS_SAN_DNSNAME, &name);
	check_test_result(suite, ret, NAME_ACCEPTED, &name);

	set_name("aaa.bbb.ccc.com", &name);
	ret = gnutls_x509_name_constraints_check(nc1, GNUTLS_SAN_DNSNAME, &name);
	check_test_result(suite, ret, NAME_ACCEPTED, &name);

	set_name("xxx.bbb.ccc.com", &name);
	ret = gnutls_x509_name_constraints_check(nc1, GNUTLS_SAN_DNSNAME, &name);
	check_test_result(suite, ret, NAME_REJECTED, &name);

	set_name("xxx.ccc.com", &name);
	ret = gnutls_x509_name_constraints_check(nc1, GNUTLS_SAN_DNSNAME, &name);
	check_test_result(suite, ret, NAME_REJECTED, &name);

	set_name("ccc.com", &name);
	ret = gnutls_x509_name_constraints_check(nc1, GNUTLS_SAN_DNSNAME, &name);
	check_test_result(suite, ret, NAME_REJECTED, &name);

	set_name("ccc.com", &name);
	ret = gnutls_x509_name_constraints_check(nc1, GNUTLS_SAN_RFC822NAME, &name);
	check_test_result(suite, ret, NAME_ACCEPTED, &name);

	set_name("xxx.ccc.com", &name);
	ret = gnutls_x509_name_constraints_check(nc1, GNUTLS_SAN_RFC822NAME, &name);
	check_test_result(suite, ret, NAME_REJECTED, &name);

	gnutls_x509_name_constraints_deinit(nc1);
	gnutls_x509_name_constraints_deinit(nc2);

	/* 1: test the merge of excluded name constraints
	 * NC1: denied DNS example.com
	 * NC2: denied DNS example.net
	 */
	suite = 1;

	ret = gnutls_x509_name_constraints_init(&nc1);
	check_for_error(ret);

	ret = gnutls_x509_name_constraints_init(&nc2);
	check_for_error(ret);

	set_name("example.com", &name);
	ret = gnutls_x509_name_constraints_add_excluded(nc1, GNUTLS_SAN_DNSNAME, &name);
	check_for_error(ret);

	set_name("example.net", &name);
	ret = gnutls_x509_name_constraints_add_excluded(nc2, GNUTLS_SAN_DNSNAME, &name);
	check_for_error(ret);

	ret = _gnutls_x509_name_constraints_merge(nc1, nc2);
	check_for_error(ret);

	set_name("xxx.example.com", &name);
	ret = gnutls_x509_name_constraints_check(nc1, GNUTLS_SAN_DNSNAME, &name);
	check_test_result(suite, ret, NAME_REJECTED, &name);

	set_name("xxx.example.net", &name);
	ret = gnutls_x509_name_constraints_check(nc1, GNUTLS_SAN_DNSNAME, &name);
	check_test_result(suite, ret, NAME_REJECTED, &name);

	set_name("example.com", &name);
	ret = gnutls_x509_name_constraints_check(nc1, GNUTLS_SAN_DNSNAME, &name);
	check_test_result(suite, ret, NAME_REJECTED, &name);

	set_name("example.net", &name);
	ret = gnutls_x509_name_constraints_check(nc1, GNUTLS_SAN_DNSNAME, &name);
	check_test_result(suite, ret, NAME_REJECTED, &name);

	set_name("example.org", &name);
	ret = gnutls_x509_name_constraints_check(nc1, GNUTLS_SAN_DNSNAME, &name);
	check_test_result(suite, ret, NAME_ACCEPTED, &name);

	gnutls_x509_name_constraints_deinit(nc1);
	gnutls_x509_name_constraints_deinit(nc2);

	/* 2: test permitted constraints with empty intersection
	 *    (no permitted nodes remain)
	 * NC1: permitted DNS one.example.com
	 * NC2: permitted DNS two.example.com
	 */
	suite = 2;

	ret = gnutls_x509_name_constraints_init(&nc1);
	check_for_error(ret);

	ret = gnutls_x509_name_constraints_init(&nc2);
	check_for_error(ret);

	set_name("one.example.com", &name);
	ret = gnutls_x509_name_constraints_add_permitted(nc1, GNUTLS_SAN_DNSNAME, &name);
	check_for_error(ret);

	set_name("two.example.com", &name);
	ret = gnutls_x509_name_constraints_add_permitted(nc2, GNUTLS_SAN_DNSNAME, &name);
	check_for_error(ret);

	ret = _gnutls_x509_name_constraints_merge(nc1, nc2);
	check_for_error(ret);

	set_name("one.example.com", &name);
	ret = gnutls_x509_name_constraints_check(nc1, GNUTLS_SAN_DNSNAME, &name);
	check_test_result(suite, ret, NAME_REJECTED, &name);

	set_name("two.example.com", &name);
	ret = gnutls_x509_name_constraints_check(nc1, GNUTLS_SAN_DNSNAME, &name);
	check_test_result(suite, ret, NAME_REJECTED, &name);

	set_name("three.example.com", &name);
	ret = gnutls_x509_name_constraints_check(nc1, GNUTLS_SAN_DNSNAME, &name);
	check_test_result(suite, ret, NAME_REJECTED, &name);

	set_name("example.com", &name);
	ret = gnutls_x509_name_constraints_check(nc1, GNUTLS_SAN_DNSNAME, &name);
	check_test_result(suite, ret, NAME_REJECTED, &name);

	set_name("org", &name);
	ret = gnutls_x509_name_constraints_check(nc1, GNUTLS_SAN_DNSNAME, &name);
	check_test_result(suite, ret, NAME_REJECTED, &name);

	gnutls_x509_name_constraints_deinit(nc1);
	gnutls_x509_name_constraints_deinit(nc2);

	/* 3: test more permitted constraints, some with empty intersection
	 * NC1: permitted DNS foo.com
	 *      permitted DNS bar.com
	 *      permitted email redhat.com
	 * NC2: permitted DNS sub.foo.com
	 */
	suite = 3;

	ret = gnutls_x509_name_constraints_init(&nc1);
	check_for_error(ret);

	ret = gnutls_x509_name_constraints_init(&nc2);
	check_for_error(ret);

	set_name("foo.com", &name);
	ret = gnutls_x509_name_constraints_add_permitted(nc1, GNUTLS_SAN_DNSNAME, &name);
	check_for_error(ret);

	set_name("bar.com", &name);
	ret = gnutls_x509_name_constraints_add_permitted(nc1, GNUTLS_SAN_DNSNAME, &name);
	check_for_error(ret);

	set_name("sub.foo.com", &name);
	ret = gnutls_x509_name_constraints_add_permitted(nc2, GNUTLS_SAN_DNSNAME, &name);
	check_for_error(ret);

	ret = _gnutls_x509_name_constraints_merge(nc1, nc2);
	check_for_error(ret);

	set_name("foo.com", &name);
	ret = gnutls_x509_name_constraints_check(nc1, GNUTLS_SAN_DNSNAME, &name);
	check_test_result(suite, ret, NAME_REJECTED, &name);

	set_name("bar.com", &name);
	ret = gnutls_x509_name_constraints_check(nc1, GNUTLS_SAN_DNSNAME, &name);
	check_test_result(suite, ret, NAME_REJECTED, &name);

	set_name("sub.foo.com", &name);
	ret = gnutls_x509_name_constraints_check(nc1, GNUTLS_SAN_DNSNAME, &name);
	check_test_result(suite, ret, NAME_ACCEPTED, &name);

	set_name("anothersub.foo.com", &name);
	ret = gnutls_x509_name_constraints_check(nc1, GNUTLS_SAN_DNSNAME, &name);
	check_test_result(suite, ret, NAME_REJECTED, &name);

	set_name("com", &name);
	ret = gnutls_x509_name_constraints_check(nc1, GNUTLS_SAN_DNSNAME, &name);
	check_test_result(suite, ret, NAME_REJECTED, &name);

	gnutls_x509_name_constraints_deinit(nc1);
	gnutls_x509_name_constraints_deinit(nc2);

	/* 4: test permitted constraints with empty intersection
	 *    almost identical to 2, but extra name constraint of different type
	 *    that remains after intersection
	 * NC1: permitted DNS three.example.com
	 *      permitted email redhat.com
	 * NC2: permitted DNS four.example.com
	 */
	suite = 4;

	ret = gnutls_x509_name_constraints_init(&nc1);
	check_for_error(ret);

	ret = gnutls_x509_name_constraints_init(&nc2);
	check_for_error(ret);

	set_name("three.example.com", &name);
	ret = gnutls_x509_name_constraints_add_permitted(nc1, GNUTLS_SAN_DNSNAME, &name);
	check_for_error(ret);

	set_name("redhat.com", &name);
	ret = gnutls_x509_name_constraints_add_permitted(nc1, GNUTLS_SAN_RFC822NAME, &name);
	check_for_error(ret);

	set_name("four.example.com", &name);
	ret = gnutls_x509_name_constraints_add_permitted(nc2, GNUTLS_SAN_DNSNAME, &name);
	check_for_error(ret);

	ret = _gnutls_x509_name_constraints_merge(nc1, nc2);
	check_for_error(ret);

	set_name("three.example.com", &name);
	ret = gnutls_x509_name_constraints_check(nc1, GNUTLS_SAN_DNSNAME, &name);
	check_test_result(suite, ret, NAME_REJECTED, &name);

	set_name("four.example.com", &name);
	ret = gnutls_x509_name_constraints_check(nc1, GNUTLS_SAN_DNSNAME, &name);
	check_test_result(suite, ret, NAME_REJECTED, &name);

	set_name("five.example.com", &name);
	ret = gnutls_x509_name_constraints_check(nc1, GNUTLS_SAN_DNSNAME, &name);
	check_test_result(suite, ret, NAME_REJECTED, &name);

	set_name("example.com", &name);
	ret = gnutls_x509_name_constraints_check(nc1, GNUTLS_SAN_DNSNAME, &name);
	check_test_result(suite, ret, NAME_REJECTED, &name);

	set_name("org", &name);
	ret = gnutls_x509_name_constraints_check(nc1, GNUTLS_SAN_DNSNAME, &name);
	check_test_result(suite, ret, NAME_REJECTED, &name);

	gnutls_x509_name_constraints_deinit(nc1);
	gnutls_x509_name_constraints_deinit(nc2);

	/* Test footer */

	if (debug)
		success("Test success.\n");
}
