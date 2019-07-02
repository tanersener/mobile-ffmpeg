/*
 * Copyright (C) 2008-2012 Free Software Foundation, Inc.
 * Copyright (C) 2018 Red Hat, Inc.
 *
 * Author: Simon Josefsson, Nikos Mavrogiannopoulos
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
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <gnutls/gnutls.h>

#include "cert-common.h"
#include "utils.h"
#include "eagain-common.h"

/* This program tests whether setting a new priority string
 * with TLS1.3 enabled on a running TLS1.2 session will
 * not prohibit or affect a following rehandshake.
 *
 * Seen in https://bugzilla.redhat.com/show_bug.cgi?id=1634736
 */

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "<%d>| %s", level, str);
}

#define MAX_REHANDSHAKES 16

static void test_rehandshake(void)
{
	/* Server stuff. */
	gnutls_certificate_credentials_t serverx509cred;
	gnutls_session_t server;
	int sret = GNUTLS_E_AGAIN;
	/* Client stuff. */
	gnutls_certificate_credentials_t clientx509cred;
	gnutls_session_t client;
	int cret = GNUTLS_E_AGAIN;
	unsigned i;

	reset_buffers();
	assert(gnutls_global_init()>=0);

	gnutls_global_set_log_function(tls_log_func);

	/* Init server */
	assert(gnutls_certificate_allocate_credentials(&serverx509cred)>=0);

	assert(gnutls_certificate_set_x509_key_mem(serverx509cred,
						  &server_cert, &server_key,
						  GNUTLS_X509_FMT_PEM)>=0);

	assert(gnutls_init(&server, GNUTLS_SERVER)>=0);

	assert(gnutls_credentials_set(server, GNUTLS_CRD_CERTIFICATE,
				     serverx509cred)>=0);

	assert(gnutls_priority_set_direct(server, "NORMAL:-VERS-TLS-ALL:+VERS-TLS1.1:+VERS-TLS1.2", NULL)>=0);

	gnutls_transport_set_push_function(server, server_push);
	gnutls_transport_set_pull_function(server, server_pull);
	gnutls_transport_set_ptr(server, server);

	/* Init client */
	assert(gnutls_certificate_allocate_credentials(&clientx509cred)>=0);

	assert(gnutls_init(&client, GNUTLS_CLIENT)>=0);

	assert(gnutls_credentials_set(client, GNUTLS_CRD_CERTIFICATE,
				     clientx509cred)>=0);

	assert(gnutls_priority_set_direct(client, "NORMAL:-VERS-TLS-ALL:+VERS-TLS1.1:+VERS-TLS1.2", NULL)>=0);

	gnutls_transport_set_push_function(client, client_push);
	gnutls_transport_set_pull_function(client, client_pull);
	gnutls_transport_set_ptr(client, client);

	HANDSHAKE(client, server);

	{
		ssize_t n;
		char b[1];

		for (i=0;i<MAX_REHANDSHAKES;i++) {
			sret = gnutls_rehandshake(server);

			n = gnutls_record_recv(client, b, 1);
			assert(n == GNUTLS_E_REHANDSHAKE);

			/* includes TLS1.3 */
			assert(gnutls_priority_set_direct(client, "NORMAL", NULL)>=0);

			HANDSHAKE(client, server);
		}
	}

	gnutls_bye(client, GNUTLS_SHUT_RDWR);
	gnutls_bye(server, GNUTLS_SHUT_RDWR);

	gnutls_deinit(client);
	gnutls_deinit(server);

	gnutls_certificate_free_credentials(serverx509cred);
	gnutls_certificate_free_credentials(clientx509cred);

	gnutls_global_deinit();
}

void doit(void)
{
	test_rehandshake();
}
