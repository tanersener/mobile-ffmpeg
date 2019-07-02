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

/* This program tests whether rehandshake without ext master secret works
 * even if ext master secret was used initially.
 *
 * Note that we do not fail gracefully and we simply continue as if
 * the extension is still valid. This violates the letter of RFC7627.
 */

const char *side = "";

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "%s|<%d>| %s", side, level, str);
}

static void try(unsigned onclient)
{
	/* Server stuff. */
	gnutls_certificate_credentials_t serverx509cred;
	gnutls_session_t server;
	int sret = GNUTLS_E_AGAIN;
	/* Client stuff. */
	gnutls_certificate_credentials_t clientx509cred;
	gnutls_session_t client;
	int cret = GNUTLS_E_AGAIN;

	/* General init. */
	global_init();
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(7);

	/* Init server */
	gnutls_certificate_allocate_credentials(&serverx509cred);
	gnutls_certificate_set_x509_key_mem(serverx509cred,
					    &server_cert, &server_key,
					    GNUTLS_X509_FMT_PEM);
	gnutls_init(&server, GNUTLS_SERVER);
	gnutls_credentials_set(server, GNUTLS_CRD_CERTIFICATE,
				serverx509cred);
	gnutls_priority_set_direct(server, "NORMAL:-VERS-ALL:+VERS-TLS1.2", NULL);
	gnutls_transport_set_push_function(server, server_push);
	gnutls_transport_set_pull_function(server, server_pull);
	gnutls_transport_set_ptr(server, server);

	/* Init client */
	gnutls_certificate_allocate_credentials(&clientx509cred);
	gnutls_init(&client, GNUTLS_CLIENT);
	gnutls_credentials_set(client, GNUTLS_CRD_CERTIFICATE,
				clientx509cred);
	gnutls_priority_set_direct(client, "NORMAL:-VERS-ALL:+VERS-TLS1.2", NULL);
	gnutls_transport_set_push_function(client, client_push);
	gnutls_transport_set_pull_function(client, client_pull);
	gnutls_transport_set_ptr(client, client);

	HANDSHAKE(client, server);

	if (gnutls_session_ext_master_secret_status(server) == 0) {
		fail("%d: ext master secret was not detected by server\n", onclient);
	}

	if (gnutls_session_ext_master_secret_status(client) == 0) {
		fail("%d: ext master secret was not detected by client\n", onclient);
	}

	if ((gnutls_session_get_flags(server) & GNUTLS_SFLAGS_EXT_MASTER_SECRET) == 0) {
		fail("%d: ext master secret was not detected by server\n", onclient);
	}

	if ((gnutls_session_get_flags(client) & GNUTLS_SFLAGS_EXT_MASTER_SECRET) == 0) {
		fail("%d: ext master secret was not detected by client\n", onclient);
	}

	if (onclient)
		gnutls_priority_set_direct(client, "NORMAL:-VERS-ALL:+VERS-TLS1.2:%NO_SESSION_HASH", NULL);
	else
		gnutls_priority_set_direct(server, "NORMAL:-VERS-ALL:+VERS-TLS1.2:%NO_SESSION_HASH", NULL);

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
		if (n != GNUTLS_E_REHANDSHAKE)
			abort();
	}

	if (onclient) {
		HANDSHAKE_EXPECT(client, server, GNUTLS_E_AGAIN, GNUTLS_E_DECRYPTION_FAILED);
	} else {
		HANDSHAKE_EXPECT(client, server, GNUTLS_E_AGAIN, GNUTLS_E_DECRYPTION_FAILED);
	}

	gnutls_deinit(client);
	gnutls_deinit(server);

	gnutls_certificate_free_credentials(serverx509cred);
	gnutls_certificate_free_credentials(clientx509cred);

	gnutls_global_deinit();
}

void doit(void)
{
	try(0);
	reset_buffers();
	try(1);
}
