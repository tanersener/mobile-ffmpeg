/*
 * Copyright (C) 2016 Red Hat, Inc.
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
#if !defined(_WIN32)
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#endif
#include <unistd.h>
#include <assert.h>
#include <time.h>
#include <gnutls/gnutls.h>
#include <gnutls/abstract.h>
#include <gnutls/x509.h>

#include "cert-common.h"
#include "utils.h"

/* Test for detection of certificates with insecure keys (too small)
 *
 */

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "<%d>| %s", level, str);
}

static time_t mytime(time_t * t)
{
	time_t then = 1474109119;
	if (t)
		*t = then;

	return then;
}

void doit(void)
{
	gnutls_certificate_credentials_t x509_cred;
	gnutls_certificate_credentials_t clicred;
	int ret;
	unsigned status;

	/* this must be called once in the program
	 */
	global_init();

	gnutls_global_set_time_function(mytime);

	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(6);

	assert(gnutls_certificate_allocate_credentials(&clicred) >= 0);
	assert(gnutls_certificate_allocate_credentials(&x509_cred)>=0);

	ret = gnutls_certificate_set_x509_trust_mem(clicred, &ca3_cert, GNUTLS_X509_FMT_PEM);
	if (ret < 0)
		fail("set_x509_trust_file failed: %s\n", gnutls_strerror(ret));

	ret = gnutls_certificate_set_x509_key_mem2(x509_cred, &server_ca3_localhost_insecure_cert, &server_ca3_localhost_insecure_key, GNUTLS_X509_FMT_PEM, NULL, 0);
	if (ret < 0)
		fail("%s\n", gnutls_strerror(ret));

	ret = gnutls_certificate_set_x509_key_mem2(x509_cred, &server_ca3_localhost6_cert_chain, &server_ca3_key, GNUTLS_X509_FMT_PEM, NULL, 0);
	if (ret < 0)
		fail("%s\n", gnutls_strerror(ret));

	test_cli_serv(x509_cred, clicred, "NORMAL", "localhost6", NULL, NULL, NULL);
	status = test_cli_serv_vf(x509_cred, clicred, "NORMAL", "localhost");

	assert(status == (GNUTLS_CERT_INVALID|GNUTLS_CERT_INSECURE_ALGORITHM));

	gnutls_certificate_free_credentials(x509_cred);
	gnutls_certificate_free_credentials(clicred);

	gnutls_global_deinit();

	if (debug)
		success("success");
}

