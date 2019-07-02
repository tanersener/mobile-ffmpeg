/*
 * Copyright (C) 2015 Red Hat, Inc.
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
#include <sys/types.h>
#include <unistd.h>

#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#include <gnutls/x509-ext.h>

#include "utils.h"

/* We test whether implicit global initialization can be overridden */

static int weak_symbol_works;

struct gnutls_subject_alt_names_st {
	struct name_st *names;
	unsigned int size;
};

/* gnutls_subject_alt_names_init() is called by gnutls_x509_crt_init().
 * We override it here to test if weak symbols work at all.
 */
__attribute__ ((visibility ("protected")))
int gnutls_subject_alt_names_init(gnutls_subject_alt_names_t * sans)
{
	weak_symbol_works = 1;

	*sans = gnutls_calloc(1, sizeof(struct gnutls_subject_alt_names_st));
	if (*sans == NULL) {
		return GNUTLS_E_MEMORY_ERROR;
	}

	return 0;
}

GNUTLS_SKIP_GLOBAL_INIT

void doit(void)
{

	int ret;
	gnutls_x509_crt_t crt;

	ret = gnutls_x509_crt_init(&crt);
	if (ret >= 0) {
		if (!weak_symbol_works)
			exit(77);

		fail("Library is already initialized\n");
	}

	gnutls_global_init();

	ret = gnutls_x509_crt_init(&crt);
	if (ret < 0) {
		fail("Could not init certificate!\n");
	}
	gnutls_x509_crt_deinit(crt);

	gnutls_global_deinit();
}
