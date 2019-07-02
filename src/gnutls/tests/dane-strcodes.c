/*
 * Copyright (C) 2017 Red Hat
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gnutls/gnutls.h>
#include <gnutls/dane.h>

#include "utils.h"

/* Check whether the DANE string functions will return a non-repeated and
 * non null value.
 */

static
void _check_unique_non_null(int line, int i, const char *val)
{
	static char previous_val[128];

	if (val == NULL)
		fail("issue in line %d, item %d\n", line, i);

	if (strcmp(val, previous_val)==0) {
		fail("issue in line %d, item %d: %s\n", line, i, val);
	} 

	snprintf(previous_val, sizeof(previous_val), "%s", val);
}

#define check_unique_non_null(x) _check_unique_non_null(__LINE__, i, x)

void doit(void)
{
	int ret;
	int i;

	ret = global_init();
	if (ret < 0) {
		fail("global_init\n");
		exit(1);
	}

	for (i=0;i<4;i++)
		check_unique_non_null(dane_cert_usage_name(i));

	for (i=0;i<1;i++)
		check_unique_non_null(dane_cert_type_name(i));

	for (i=0;i<3;i++)
		check_unique_non_null(dane_match_type_name(i));

	for (i=-14;i<=0;i++) {
		check_unique_non_null(dane_strerror(i));
	}

	gnutls_global_deinit();
}
