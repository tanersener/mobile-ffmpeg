/*
 * Copyright (C) 2019 Red Hat, Inc.
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
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <gnutls/gnutls.h>
#include "utils.h"
#include "eagain-common.h"
#include "cert-common.h"
#include "tls13/ext-parse.h"
#include <assert.h>

const char *side;

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "%s|<%d>| %s", side, level, str);
}

static int server_handshake_callback(gnutls_session_t session, unsigned int htype,
				     unsigned post, unsigned int incoming,
				     const gnutls_datum_t *msg)
{
	unsigned pos;
	gnutls_datum_t mmsg;

	assert(post && htype == GNUTLS_HANDSHAKE_SERVER_HELLO);

	switch (htype) {
	case GNUTLS_HANDSHAKE_SERVER_HELLO:
		assert(msg->size >= HANDSHAKE_SESSION_ID_POS);
		pos = HANDSHAKE_SESSION_ID_POS;
		SKIP8(pos, msg->size);
		pos += 3; /* ciphersuite + compression */

		mmsg.data = &msg->data[pos];
		mmsg.size = msg->size - pos;
		if (pos != msg->size) {
			if (pos < msg->size-1) {
				fprintf(stderr, "additional bytes: %.2x%.2x\n", mmsg.data[0], mmsg.data[1]);
			}
			fail("the server hello contains additional bytes\n");
		}
		break;
	}
	return 0;
}

static int client_handshake_callback(gnutls_session_t session, unsigned int htype,
				     unsigned post, unsigned int incoming,
				     const gnutls_datum_t *msg)
{
	unsigned pos;
	gnutls_datum_t mmsg;

	assert(!post && htype == GNUTLS_HANDSHAKE_CLIENT_HELLO);

	switch (htype) {
	case GNUTLS_HANDSHAKE_CLIENT_HELLO:
		assert(msg->size >= HANDSHAKE_SESSION_ID_POS);
		pos = HANDSHAKE_SESSION_ID_POS;
		SKIP8(pos, msg->size);
		SKIP16(pos, msg->size);
		SKIP8(pos, msg->size);

		mmsg.data = &msg->data[pos];
		mmsg.size = msg->size - pos;

		if (pos != msg->size) {
			if (pos < msg->size-1) {
				fprintf(stderr, "additional bytes: %.2x%.2x\n", mmsg.data[0], mmsg.data[1]);
			}
			fail("the client hello contains additional bytes\n");
		}
		break;
	}
	return 0;
}

static
void start(const char *prio, gnutls_protocol_t exp_version)
{
	int ret;
	/* Server stuff. */
	gnutls_certificate_credentials_t serverx509cred;
	gnutls_session_t server;
	int sret = GNUTLS_E_AGAIN;
	/* Client stuff. */
	gnutls_certificate_credentials_t clientx509cred;
	gnutls_session_t client;
	int cret = GNUTLS_E_AGAIN;

	success("trying %s\n", prio);

	/* General init. */
	global_init();
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(6);

	/* Init server */
	gnutls_certificate_allocate_credentials(&serverx509cred);
	gnutls_certificate_set_x509_key_mem(serverx509cred,
					    &server_cert, &server_key,
					    GNUTLS_X509_FMT_PEM);

	gnutls_init(&server, GNUTLS_SERVER);
	gnutls_credentials_set(server, GNUTLS_CRD_CERTIFICATE,
				serverx509cred);
	assert(gnutls_priority_set_direct(server, prio, NULL)>=0);
	gnutls_transport_set_push_function(server, server_push);
	gnutls_transport_set_pull_function(server, server_pull);
	gnutls_transport_set_ptr(server, server);

	gnutls_handshake_set_hook_function(server,
					   GNUTLS_HANDSHAKE_SERVER_HELLO,
					   GNUTLS_HOOK_POST,
					   server_handshake_callback);

	/* Init client */
	ret = gnutls_certificate_allocate_credentials(&clientx509cred);
	if (ret < 0)
		exit(1);

	ret = gnutls_certificate_set_x509_trust_mem(clientx509cred, &ca_cert, GNUTLS_X509_FMT_PEM);
	if (ret < 0)
		exit(1);

	ret = gnutls_init(&client, GNUTLS_CLIENT);
	if (ret < 0)
		exit(1);

	ret = gnutls_credentials_set(client, GNUTLS_CRD_CERTIFICATE,
				clientx509cred);
	if (ret < 0)
		exit(1);

	gnutls_priority_set_direct(client, prio, NULL);
	gnutls_transport_set_push_function(client, client_push);
	gnutls_transport_set_pull_function(client, client_pull);
	gnutls_transport_set_ptr(client, client);

	gnutls_handshake_set_hook_function(client,
					   GNUTLS_HANDSHAKE_CLIENT_HELLO,
					   GNUTLS_HOOK_PRE,
					   client_handshake_callback);

	HANDSHAKE(client, server);

	/* check gnutls_certificate_get_ours() - client side */
	{
		const gnutls_datum_t *mcert;

		mcert = gnutls_certificate_get_ours(client);
		if (mcert != NULL) {
			fail("gnutls_certificate_get_ours(): failed\n");
			exit(1);
		}
	}

	assert(gnutls_protocol_get_version(server) == exp_version);

	assert(gnutls_certificate_type_get(server)==GNUTLS_CRT_X509);
	assert(gnutls_certificate_type_get(client)==GNUTLS_CRT_X509);

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
	start("NORMAL:-VERS-ALL:+VERS-TLS1.0:%NO_EXTENSIONS", GNUTLS_TLS1_0);
	start("NORMAL:-VERS-ALL:+VERS-TLS1.1:%NO_EXTENSIONS", GNUTLS_TLS1_1);
	start("NORMAL:-VERS-ALL:+VERS-TLS1.2:%NO_EXTENSIONS", GNUTLS_TLS1_2);
	start("NORMAL:-VERS-ALL:+VERS-TLS1.3:+VERS-TLS1.2:%NO_EXTENSIONS", GNUTLS_TLS1_2);
}
