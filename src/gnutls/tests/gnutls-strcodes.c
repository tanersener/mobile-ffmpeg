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

/* Check whether the string functions will return a non-repeated and
 * non null value.
 */

static
void _check_non_null(int line, int i, const char *val)
{
	if (val == NULL)
		fail("issue in line %d, item %d\n", line, i);
}

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

static
void _check_unique(int line, int i, const char *val)
{
	static char previous_val[128];

	if (val == NULL) {
		previous_val[0] = 0;
		return;
	}

	if (strcmp(val, previous_val)==0)
		fail("issue in line %d, item %d: %s\n", line, i, val);

	snprintf(previous_val, sizeof(previous_val), "%s", val);
}

#define check_unique(x) _check_unique(__LINE__, i, x)
#define check_unique_non_null(x) _check_unique_non_null(__LINE__, i, x)
#define check_non_null(x) _check_non_null(__LINE__, i, x)

void doit(void)
{
	int ret;
	int i;

	ret = global_init();
	if (ret < 0) {
		fail("global_init\n");
		exit(1);
	}

	for (i=GNUTLS_E_UNIMPLEMENTED_FEATURE;i<=0;i++) {
		check_unique(gnutls_strerror(i));
		check_unique(gnutls_strerror_name(i));
	}

	for (i=0;i<GNUTLS_HANDSHAKE_CHANGE_CIPHER_SPEC;i++)
		check_non_null(gnutls_handshake_description_get_name(i));

	for (i=GNUTLS_PK_UNKNOWN+1;i<=GNUTLS_PK_MAX;i++)
		check_unique_non_null(gnutls_pk_algorithm_get_name(i));

	for (i=GNUTLS_SIGN_UNKNOWN+1;i<=GNUTLS_SIGN_MAX;i++) {
		if (i==19) continue;
		check_unique_non_null(gnutls_sign_algorithm_get_name(i));
	}

	for (i=GNUTLS_A_CLOSE_NOTIFY;i<=GNUTLS_A_MAX;i++) {
		check_unique(gnutls_alert_get_strname(i));
	}

	for (i=GNUTLS_SEC_PARAM_INSECURE;i<=GNUTLS_SEC_PARAM_MAX;i++) {
		check_non_null(gnutls_sec_param_get_name(i));
	}

	for (i=GNUTLS_ECC_CURVE_INVALID+1;i<=GNUTLS_ECC_CURVE_MAX;i++) {
		check_unique_non_null(gnutls_ecc_curve_get_name(i));
		if (i == GNUTLS_ECC_CURVE_X25519)
			continue; /* no oid yet */
		check_unique_non_null(gnutls_ecc_curve_get_oid(i));
	}

	gnutls_global_deinit();
}
