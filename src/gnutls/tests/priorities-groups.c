/*
 * Copyright (C) 2017 Red Hat, Inc.
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gnutls/gnutls.h>

#include "utils.h"

static void
try_prio(const char *prio, unsigned group_size, const unsigned int *group_list,
	 unsigned curve_size, const unsigned int *curve_list)
{
	int ret;
	gnutls_priority_t p;
	const char *err;
	unsigned i;
	const unsigned int *list;

	ret = gnutls_priority_init(&p, prio, &err);
	if (ret < 0) {
		fprintf(stderr, "error: %s: %s\n", gnutls_strerror(ret),
			err);
		exit(1);
	}

	ret = gnutls_priority_group_list(p, &list);
	if ((unsigned)ret != group_size) {
		fail("%s: group size (%d) doesn't match expected (%d)\n", prio, ret, group_size);
	}

	for (i=0;i<group_size;i++) {
		if (group_list[i] != list[i]) {
			fail("%s: group listing %d differs to expected\n", prio, i);
		}
	}

	ret = gnutls_priority_ecc_curve_list(p, &list);
	if ((unsigned)ret != curve_size) {
		fail("%s: EC curve size (%d) doesn't match expected (%d)\n", prio, ret, curve_size);
	}

	for (i=0;i<curve_size;i++) {
		if (curve_list[i] != list[i]) {
			fail("%s: curve listing %d differs to expected\n", prio, i);
		}
	}

	gnutls_priority_deinit(p);

	/* fprintf(stderr, "count: %d\n", count); */

	if (debug)
		success("finished: %s\n", prio);
}


void doit(void)
{
	unsigned int list1[16];
	unsigned int list2[16];
	/* this must be called once in the program
	 */
	global_init();

	memset(list1, 0, sizeof(list1));
	memset(list2, 0, sizeof(list2));
	list1[0] = GNUTLS_GROUP_SECP256R1;
	list2[0] = GNUTLS_ECC_CURVE_SECP256R1;
	try_prio("NORMAL:-GROUP-ALL:+GROUP-SECP256R1", 1, list1, 1, list2);

	memset(list1, 0, sizeof(list1));
	memset(list2, 0, sizeof(list2));
	list1[0] = GNUTLS_GROUP_SECP256R1;
	list1[1] = GNUTLS_GROUP_SECP384R1;
	list1[2] = GNUTLS_GROUP_FFDHE2048;
	list2[0] = GNUTLS_ECC_CURVE_SECP256R1;
	list2[1] = GNUTLS_ECC_CURVE_SECP384R1;
	try_prio("NORMAL:-GROUP-ALL:+GROUP-SECP256R1:+GROUP-SECP384R1:+GROUP-FFDHE2048", 3, list1, 2, list2);

	memset(list1, 0, sizeof(list1));
	memset(list2, 0, sizeof(list2));
	list1[0] = GNUTLS_GROUP_SECP521R1;
	list1[1] = GNUTLS_GROUP_SECP384R1;
	list1[2] = GNUTLS_GROUP_FFDHE2048;
	list1[3] = GNUTLS_GROUP_FFDHE3072;
	list2[0] = GNUTLS_ECC_CURVE_SECP521R1;
	list2[1] = GNUTLS_ECC_CURVE_SECP384R1;
	try_prio("NORMAL:-CURVE-ALL:+CURVE-SECP521R1:+GROUP-SECP384R1:+GROUP-FFDHE2048:+GROUP-FFDHE3072", 4, list1, 2, list2);
}

