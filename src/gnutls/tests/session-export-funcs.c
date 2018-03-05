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
#include <errno.h>
#include <gnutls/gnutls.h>
#include "utils.h"
#include "eagain-common.h"

#include "cert-common.h"

/* This tests whether the max-record extension is respected on TLS.
 */

const char *side;

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "%s|<%d>| %s", side, level, str);
}

void doit(void)
{
	global_init();

	int ret;
	/* Server stuff. */
	gnutls_certificate_credentials_t serverx509cred;
	gnutls_session_t server;
	int sret = GNUTLS_E_AGAIN;
	/* Client stuff. */
	gnutls_certificate_credentials_t clientx509cred;
	gnutls_session_t client;
	int cret = GNUTLS_E_AGAIN;
	gnutls_datum_t v1, v2;
	char client_random[32];
	char server_random[32];

	memset(client_random, 1, sizeof(client_random));
	memset(server_random, 2, sizeof(server_random));

	/* General init. */
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(6);

	/* Init server */
	gnutls_certificate_allocate_credentials(&serverx509cred);
	gnutls_certificate_set_x509_key_mem(serverx509cred,
					    &server2_cert, &server2_key,
					    GNUTLS_X509_FMT_PEM);

	gnutls_init(&server, GNUTLS_SERVER);
	gnutls_credentials_set(server, GNUTLS_CRD_CERTIFICATE,
				serverx509cred);

	gnutls_priority_set_direct(server,
				   "NORMAL",
				   NULL);
	gnutls_transport_set_push_function(server, server_push);
	gnutls_transport_set_pull_function(server, server_pull);
	gnutls_transport_set_pull_timeout_function(server,
						   server_pull_timeout_func);
	gnutls_transport_set_ptr(server, server);

	/* Init client */

	ret = gnutls_certificate_allocate_credentials(&clientx509cred);
	if (ret < 0)
		exit(1);

	ret = gnutls_certificate_set_x509_trust_mem(clientx509cred, &ca2_cert, GNUTLS_X509_FMT_PEM);
	if (ret < 0)
		exit(1);

	ret = gnutls_init(&client, GNUTLS_CLIENT);
	if (ret < 0)
		exit(1);

	ret = gnutls_credentials_set(client, GNUTLS_CRD_CERTIFICATE,
				clientx509cred);
	if (ret < 0)
		exit(1);

	ret = gnutls_priority_set_direct(client, "NORMAL", NULL);
	if (ret < 0)
		exit(1);

	gnutls_transport_set_push_function(client, client_push);
	gnutls_transport_set_pull_function(client, client_pull);
	gnutls_transport_set_pull_timeout_function(client,
						   client_pull_timeout_func);
	gnutls_transport_set_ptr(client, client);

	v1.data = (void*)client_random;
	v1.size = sizeof(client_random);
	gnutls_handshake_set_random(client, &v1);

	v1.data = (void*)server_random;
	v1.size = sizeof(server_random);
	gnutls_handshake_set_random(server, &v1);

	memset(&v1, 0, sizeof(v1));
	memset(&v2, 0, sizeof(v2));

	HANDSHAKE(client, server);

	gnutls_session_get_random(client, &v1, &v2);
	if (v1.size != 32 || memcmp(v1.data, client_random, 32) != 0) {
		fail("error in client's random view 1\n");
	}

	if (v2.size != 32 || memcmp(v2.data, server_random, 32) != 0) {
		fail("error in client's random view 2\n");
	}

	memset(&v1, 0, sizeof(v1));
	memset(&v2, 0, sizeof(v2));

	gnutls_session_get_random(server, &v1, &v2);
	if (v1.size != 32 || memcmp(v1.data, client_random, 32) != 0) {
		fail("error in server's random view 1\n");
	}

	if (v2.size != 32 || memcmp(v2.data, server_random, 32) != 0) {
		fail("error in server's random view 2\n");
	}

	memset(&v1, 0, sizeof(v1));
	memset(&v2, 0, sizeof(v2));

	/* check master secret */
	gnutls_session_get_master_secret(server, &v1);
	if (v1.size <= 0) {
		fail("error in server's master secret\n");
	}

	gnutls_session_get_master_secret(client, &v2);
	if (v2.size <= 0) {
		fail("error in client's master secret\n");
	}

	if (v1.size != v2.size || memcmp(v1.data, v2.data, v1.size) != 0) {
		fail("master secret don't match!\n");
	}
	
	gnutls_bye(client, GNUTLS_SHUT_RDWR);
	gnutls_bye(server, GNUTLS_SHUT_RDWR);

	gnutls_deinit(client);
	gnutls_deinit(server);

	gnutls_certificate_free_credentials(serverx509cred);
	gnutls_certificate_free_credentials(clientx509cred);

	gnutls_global_deinit();
}
