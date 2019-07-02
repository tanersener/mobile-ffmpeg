/*
 * Copyright (C) 2008-2012 Free Software Foundation, Inc.
 *
 * Author: Simon Josefsson
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
#include <gnutls/gnutls.h>
#include "utils.h"
#include "eagain-common.h"
#include "cert-common.h"

/* This program tests server initiated rehandshake under TLS 1.3.
 * Although rehandshake doesn't happen under TLS1.3 this tests
 * whether the old APIs would still work.
 */

const char *side = "";

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "%s|<%d>| %s", side, level, str);
}

static
void server_initiated_handshake(void)
{
	/* Server stuff. */
	gnutls_certificate_credentials_t serverx509cred;
	gnutls_session_t server;
	int sret = GNUTLS_E_AGAIN;
	/* Client stuff. */
	gnutls_certificate_credentials_t clientx509cred;
	gnutls_session_t client;
	unsigned char buffer[64];
	int cret = GNUTLS_E_AGAIN;
	size_t transferred = 0;

	success("testing server initiated re-handshake\n");

	/* General init. */
	global_init();
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(2);

	/* Init server */
	gnutls_certificate_allocate_credentials(&serverx509cred);
	gnutls_certificate_set_x509_key_mem(serverx509cred,
					    &server_cert, &server_key,
					    GNUTLS_X509_FMT_PEM);
	gnutls_init(&server, GNUTLS_SERVER);
	gnutls_credentials_set(server, GNUTLS_CRD_CERTIFICATE,
				serverx509cred);
	gnutls_priority_set_direct(server, "NORMAL:-VERS-TLS-ALL:+VERS-TLS1.3", NULL);
	gnutls_transport_set_push_function(server, server_push);
	gnutls_transport_set_pull_function(server, server_pull);
	gnutls_transport_set_ptr(server, server);

	/* Init client */
	gnutls_certificate_allocate_credentials(&clientx509cred);
	gnutls_init(&client, GNUTLS_CLIENT);
	gnutls_credentials_set(client, GNUTLS_CRD_CERTIFICATE,
				clientx509cred);
	gnutls_priority_set_direct(client, "NORMAL:+VERS-TLS1.3", NULL);
	gnutls_transport_set_push_function(client, client_push);
	gnutls_transport_set_pull_function(client, client_pull);
	gnutls_transport_set_ptr(client, client);

	HANDSHAKE(client, server);

	if (gnutls_protocol_get_version(client) != GNUTLS_TLS1_3)
		fail("TLS1.3 was not negotiated\n");

	sret = gnutls_rehandshake(server);
	if (debug) {
		tls_log_func(0, "gnutls_rehandshake (server)...\n");
		tls_log_func(0, gnutls_strerror(sret));
		tls_log_func(0, "\n");
	}

	{
		ssize_t n;
		char b[1];
		n = gnutls_record_recv(client, b, 1);
		/* in TLS1.2 we get REHANDSHAKE error, here nothing */
		if (n != GNUTLS_E_AGAIN) {
			fail("error msg: %s\n", gnutls_strerror(n));
		}
	}

	TRANSFER(client, server, "xxxx", 4, buffer, sizeof(buffer));

	gnutls_bye(client, GNUTLS_SHUT_RDWR);
	gnutls_bye(server, GNUTLS_SHUT_RDWR);

	gnutls_deinit(client);
	gnutls_deinit(server);

	gnutls_certificate_free_credentials(serverx509cred);
	gnutls_certificate_free_credentials(clientx509cred);

	gnutls_global_deinit();

	reset_buffers();
}

static
void client_initiated_handshake(void)
{
	/* Server stuff. */
	gnutls_certificate_credentials_t serverx509cred;
	gnutls_session_t server;
	int sret = GNUTLS_E_AGAIN;
	/* Client stuff. */
	gnutls_certificate_credentials_t clientx509cred;
	gnutls_session_t client;
	unsigned char buffer[64];
	int cret = GNUTLS_E_AGAIN;
	size_t transferred = 0;

	success("testing client initiated re-handshake\n");

	/* General init. */
	global_init();
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(2);

	/* Init server */
	gnutls_certificate_allocate_credentials(&serverx509cred);
	gnutls_certificate_set_x509_key_mem(serverx509cred,
					    &server_cert, &server_key,
					    GNUTLS_X509_FMT_PEM);
	gnutls_init(&server, GNUTLS_SERVER);
	gnutls_credentials_set(server, GNUTLS_CRD_CERTIFICATE,
				serverx509cred);
	gnutls_priority_set_direct(server, "NORMAL:-VERS-TLS-ALL:+VERS-TLS1.3", NULL);
	gnutls_transport_set_push_function(server, server_push);
	gnutls_transport_set_pull_function(server, server_pull);
	gnutls_transport_set_ptr(server, server);

	/* Init client */
	gnutls_certificate_allocate_credentials(&clientx509cred);
	gnutls_init(&client, GNUTLS_CLIENT);
	gnutls_credentials_set(client, GNUTLS_CRD_CERTIFICATE,
				clientx509cred);
	gnutls_priority_set_direct(client, "NORMAL:+VERS-TLS1.3", NULL);
	gnutls_transport_set_push_function(client, client_push);
	gnutls_transport_set_pull_function(client, client_pull);
	gnutls_transport_set_ptr(client, client);

	HANDSHAKE(client, server);

	if (gnutls_protocol_get_version(client) != GNUTLS_TLS1_3)
		fail("TLS1.3 was not negotiated\n");

	HANDSHAKE(client, server);

	TRANSFER(client, server, "xxxx", 4, buffer, sizeof(buffer));

	gnutls_bye(client, GNUTLS_SHUT_RDWR);
	gnutls_bye(server, GNUTLS_SHUT_RDWR);

	gnutls_deinit(client);
	gnutls_deinit(server);

	gnutls_certificate_free_credentials(serverx509cred);
	gnutls_certificate_free_credentials(clientx509cred);

	gnutls_global_deinit();

	reset_buffers();
}

void doit(void)
{
	server_initiated_handshake();
	client_initiated_handshake();
}
