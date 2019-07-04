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
#include <gnutls/dtls.h>
#include <assert.h>
#include "utils.h"
#include "eagain-common.h"
#include "cert-common.h"

/* 
 * That test verifies whether all the supported versions are negotiated
 * with the NORMAL priority string.
 */

const char *side;

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "%s|<%d>| %s", side, level, str);
}

static void try(const char *client_prio, int expected)
{
	int ret;
	gnutls_certificate_credentials_t serverx509cred;
	gnutls_session_t server;
	int sret = GNUTLS_E_AGAIN;
	/* Client stuff. */
	gnutls_certificate_credentials_t clientx509cred;
	gnutls_session_t client;
	int cret = GNUTLS_E_AGAIN;
	unsigned flags = 0;
	unsigned dtls = 0;
	const char *server_prio = "NORMAL:+VERS-TLS-ALL";

	if (expected >= GNUTLS_DTLS_VERSION_MIN && expected <= GNUTLS_DTLS_VERSION_MAX) {
		dtls = 1;
		/* we do not really do negotiation in that version */
		if (expected == GNUTLS_DTLS0_9)
			server_prio = client_prio;
	}

	/* General init. */
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(6);

	/* Init server */
	gnutls_certificate_allocate_credentials(&serverx509cred);
	gnutls_certificate_set_x509_key_mem(serverx509cred,
					    &server_cert, &server_key,
					    GNUTLS_X509_FMT_PEM);

	if (dtls)
		flags |= (GNUTLS_DATAGRAM | GNUTLS_NONBLOCK);

	gnutls_init(&server, GNUTLS_SERVER|flags);
	gnutls_credentials_set(server, GNUTLS_CRD_CERTIFICATE,
				serverx509cred);

	assert(gnutls_priority_set_direct(server,
				   server_prio,
				   NULL) >= 0);
	gnutls_transport_set_push_function(server, server_push);
	gnutls_transport_set_pull_function(server, server_pull);
	gnutls_transport_set_pull_timeout_function(server, server_pull_timeout_func);
	gnutls_transport_set_ptr(server, server);

	/* Init client */

	ret = gnutls_certificate_allocate_credentials(&clientx509cred);
	if (ret < 0)
		exit(1);

	ret = gnutls_certificate_set_x509_trust_mem(clientx509cred, &ca_cert, GNUTLS_X509_FMT_PEM);
	if (ret < 0)
		exit(1);

	ret = gnutls_init(&client, GNUTLS_CLIENT|flags);
	if (ret < 0)
		exit(1);

	ret = gnutls_credentials_set(client, GNUTLS_CRD_CERTIFICATE,
				clientx509cred);
	if (ret < 0)
		exit(1);

	gnutls_transport_set_push_function(client, client_push);
	gnutls_transport_set_pull_function(client, client_pull);
	gnutls_transport_set_pull_timeout_function(client, client_pull_timeout_func);
	gnutls_transport_set_ptr(client, client);

	ret = gnutls_priority_set_direct(client, client_prio, NULL);
	if (ret < 0) {
		fprintf(stderr, "error in %s\n", client_prio);
		exit(1);
	}

	if (expected > 0) {
		success("handshake with %s\n", client_prio);
		if (dtls) {
			HANDSHAKE_DTLS(client, server);
		} else {
			HANDSHAKE(client, server);
		}

		ret = gnutls_protocol_get_version(client);
		if (ret != expected) {
			fail("unexpected negotiated protocol %s (expected %s)\n", gnutls_protocol_get_name(ret),
				gnutls_protocol_get_name(expected));
			exit(1);
		}
	} else {
		HANDSHAKE_EXPECT(client, server, GNUTLS_E_AGAIN, GNUTLS_E_UNSUPPORTED_VERSION_PACKET);
	}

	gnutls_bye(client, GNUTLS_SHUT_RDWR);
	gnutls_bye(server, GNUTLS_SHUT_RDWR);

	gnutls_deinit(client);
	gnutls_deinit(server);

	gnutls_certificate_free_credentials(serverx509cred);
	gnutls_certificate_free_credentials(clientx509cred);
}

void doit(void)
{
	global_init();

	try("NORMAL:-VERS-TLS-ALL:+VERS-TLS1.0", GNUTLS_TLS1_0);
	reset_buffers();
	try("NORMAL:-VERS-TLS-ALL:+VERS-TLS1.1", GNUTLS_TLS1_1);
	reset_buffers();
	try("NORMAL:-VERS-TLS-ALL:+VERS-TLS1.2", GNUTLS_TLS1_2);
	reset_buffers();
	try("NORMAL:-VERS-TLS-ALL:+VERS-TLS1.3", GNUTLS_TLS1_3);
	reset_buffers();
	try("NORMAL:-VERS-TLS-ALL:+VERS-TLS1.3:+VERS-TLS1.0", GNUTLS_TLS1_0);
	reset_buffers();
	/* similar to above test, but checks a different syntax */
	try("NORMAL:-VERS-ALL:+VERS-TLS1.3:+VERS-TLS1.1", GNUTLS_TLS1_1);
	reset_buffers();
	try("NORMAL:-VERS-TLS-ALL:+VERS-TLS1.3:+VERS-TLS1.2", GNUTLS_TLS1_3);
	reset_buffers();
#ifdef ENABLE_SSL3
	try("NORMAL:-VERS-TLS-ALL:+VERS-SSL3.0", -1);
	reset_buffers();
#endif
	try("NORMAL:-VERS-ALL:+VERS-DTLS1.0", GNUTLS_DTLS1_0);
	reset_buffers();
	try("NORMAL:-VERS-DTLS-ALL:+VERS-DTLS1.2", GNUTLS_DTLS1_2);
	reset_buffers();

	/* special test for this legacy crap */
	try("NONE:+VERS-DTLS0.9:+COMP-NULL:+AES-128-CBC:+SHA1:+RSA:%COMPAT", GNUTLS_DTLS0_9);
	reset_buffers();
	gnutls_global_deinit();
}
