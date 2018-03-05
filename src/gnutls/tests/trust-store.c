/*
 * Copyright (C) 2017 Nikos Mavrogiannopoulos
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
#include <sys/types.h>
#include <gnutls/gnutls.h>
#include <assert.h>

#include "utils.h"

/* Test for gnutls_certificate_set_x509_system_trust()
 */

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "<%d>| %s", level, str);
}


void doit(void)
{
	gnutls_certificate_credentials_t x509_cred;
	int ret;

	gnutls_global_init();

	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(6);

	assert(gnutls_certificate_allocate_credentials(&x509_cred) >= 0);

	ret = gnutls_certificate_set_x509_system_trust(x509_cred);
	if (ret == GNUTLS_E_UNIMPLEMENTED_FEATURE) {
		exit(77);
	} else if (ret < 0) {
		fail("error loading system trust store: %s\n", gnutls_strerror(ret));
	} else if (ret == 0) {
		fail("no certificates were found in system trust store!\n");
	}

	gnutls_certificate_free_credentials(x509_cred);

	gnutls_global_deinit();

	if (debug)
		success("success");
}
