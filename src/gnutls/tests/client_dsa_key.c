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
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>

#include "utils.h"
#include "cert-common.h"

/* Test for correct operation when a client uses a DSA key and
 * explicitly enables DSA signatures under TLS 1.2.
 *
 */

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "<%d>| %s", level, str);
}

void doit(void)
{
	gnutls_certificate_credentials_t serv_cred;
	gnutls_certificate_credentials_t cli_cred;
	int ret;

	/* this must be called once in the program
	 */
	global_init();

	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(6);

	assert(gnutls_certificate_allocate_credentials(&cli_cred) >= 0);

	ret = gnutls_certificate_set_x509_trust_mem(cli_cred, &ca3_cert, GNUTLS_X509_FMT_PEM);
	if (ret < 0)
		fail("set_x509_trust_file failed: %s\n", gnutls_strerror(ret));


	ret = gnutls_certificate_set_x509_key_mem(cli_cred, &clidsa_ca3_cert,
					    &clidsa_ca3_key,
					    GNUTLS_X509_FMT_PEM);
	if (ret < 0) {
		fail("error in error code: %s\n", gnutls_strerror(ret));
		exit(1);
	}

	/* test gnutls_certificate_flags() */
	gnutls_certificate_allocate_credentials(&serv_cred);
	gnutls_certificate_set_flags(serv_cred, GNUTLS_CERTIFICATE_SKIP_KEY_CERT_MATCH);
	gnutls_certificate_set_verify_flags(serv_cred, GNUTLS_VERIFY_ALLOW_SIGN_WITH_SHA1);

	ret = gnutls_certificate_set_x509_trust_mem(serv_cred, &ca3_cert, GNUTLS_X509_FMT_PEM);
	if (ret < 0)
		fail("set_x509_trust_file failed: %s\n", gnutls_strerror(ret));

	ret = gnutls_certificate_set_x509_key_mem(serv_cred, &server_ca3_localhost_cert_chain,
					    &server_ca3_key,
					    GNUTLS_X509_FMT_PEM);
	if (ret < 0) {
		fail("error in error code\n");
		exit(1);
	}

	test_cli_serv_cert(serv_cred, cli_cred, "NORMAL:+DHE-DSS:+SIGN-DSA-SHA1", "NORMAL:-VERS-TLS-ALL:+VERS-TLS1.2:-DHE-DSS:+SIGN-DSA-SHA1", "localhost");

	gnutls_certificate_free_credentials(serv_cred);
	gnutls_certificate_free_credentials(cli_cred);

	gnutls_global_deinit();

	if (debug)
		success("success");
}
