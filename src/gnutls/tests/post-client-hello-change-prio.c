/*
 * Copyright (C) 2018 Nikos Mavrogiannopoulos
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
#include "utils.h"
#include "eagain-common.h"
#include "cert-common.h"

/* Tests whether the post_client_hello callback can modify
 * the avalable priorities. This is used by apache's mod_gnutls.
 */

const char *side;
static int pch_ok = 0;
const char *override_prio = NULL;

static int post_client_hello_callback(gnutls_session_t session)
{
	assert(gnutls_priority_set_direct(session, override_prio, NULL) >= 0);
	pch_ok = 1;
	return 0;
}

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "%s|<%d>| %s", side, level, str);
}

static
void start(const char *name, const char *prio, gnutls_protocol_t exp_version)
{
	/* Server stuff. */
	gnutls_certificate_credentials_t serverx509cred;
	gnutls_session_t server;
	int sret = GNUTLS_E_AGAIN;
	/* Client stuff. */
	gnutls_certificate_credentials_t clientx509cred;
	gnutls_session_t client;
	int cret = GNUTLS_E_AGAIN;

	success("trying %s\n", name);

	pch_ok = 0;

	/* General init. */
	global_init();
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(4);

	/* Init server */
	assert(gnutls_certificate_allocate_credentials(&serverx509cred)>=0);
	assert(gnutls_certificate_set_x509_key_mem(serverx509cred,
					    &server_cert, &server_key,
					    GNUTLS_X509_FMT_PEM)>=0);
	assert(gnutls_init(&server, GNUTLS_SERVER)>=0);
	gnutls_credentials_set(server, GNUTLS_CRD_CERTIFICATE,
				serverx509cred);
	assert(gnutls_priority_set_direct(server, prio, NULL)>=0);
	gnutls_transport_set_push_function(server, server_push);
	gnutls_transport_set_pull_function(server, server_pull);
	gnutls_transport_set_ptr(server, server);
	gnutls_handshake_set_post_client_hello_function(server,
							post_client_hello_callback);

	assert(gnutls_certificate_allocate_credentials(&clientx509cred)>=0);
	assert(gnutls_init(&client, GNUTLS_CLIENT)>=0);
	gnutls_credentials_set(client, GNUTLS_CRD_CERTIFICATE,
				clientx509cred);
	assert(gnutls_priority_set_direct(client, prio, NULL)>=0);
	gnutls_transport_set_push_function(client, client_push);
	gnutls_transport_set_pull_function(client, client_pull);
	gnutls_transport_set_ptr(client, client);

	HANDSHAKE(client, server);

	assert(exp_version == gnutls_protocol_get_version(client));
	assert(exp_version == gnutls_protocol_get_version(server));

	gnutls_bye(client, GNUTLS_SHUT_RDWR);
	gnutls_bye(server, GNUTLS_SHUT_RDWR);

	gnutls_deinit(client);
	gnutls_deinit(server);

	gnutls_certificate_free_credentials(serverx509cred);
	gnutls_certificate_free_credentials(clientx509cred);

	gnutls_global_deinit();

	if (pch_ok == 0)
		fail("Post client hello callback wasn't called\n");

	reset_buffers();
}

void doit(void)
{
	override_prio = "NORMAL";
	start("tls1.2-only", "NORMAL:-VERS-ALL:+VERS-TLS1.2", GNUTLS_TLS1_2);
	start("tls1.3-only", "NORMAL:-VERS-ALL:+VERS-TLS1.3", GNUTLS_TLS1_3);
	start("default", "NORMAL", GNUTLS_TLS1_3);
	override_prio = "NORMAL:-VERS-ALL:+VERS-TLS1.2";
	start("default overriden to TLS1.2-only", "NORMAL", GNUTLS_TLS1_2);
}
