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
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <gnutls/gnutls.h>
#include "utils.h"
#include "eagain-common.h"
#include "cert-common.h"

/* This tests TLS1.3 and gnutls_session_get_data2() when no
 * callback with gnutls_transport_set_pull_timeout_function()
 * is set */

const char *side;

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "%s|<%d>| %s", side, level, str);
}

static time_t mytime(time_t * t)
{
	time_t then = 1461671166;

	if (t)
		*t = then;

	return then;
}

static ssize_t
server_pull_fail(gnutls_transport_ptr_t tr, void *data, size_t len)
{
	fail("unexpected call to pull callback detected\n");
	return -1;
}

void doit(void)
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
	gnutls_datum_t data;
	char buf[128];

	/* General init. */
	global_init();
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(6);

	gnutls_global_set_time_function(mytime);

	assert(gnutls_certificate_allocate_credentials(&serverx509cred) >= 0);
	assert(gnutls_certificate_set_x509_key_mem(serverx509cred,
					    &server_cert, &server_key,
					    GNUTLS_X509_FMT_PEM) >= 0);

	assert(gnutls_init(&server, GNUTLS_SERVER) >= 0);
	gnutls_credentials_set(server, GNUTLS_CRD_CERTIFICATE,
				serverx509cred);
	assert(gnutls_set_default_priority(server)>=0);
	gnutls_transport_set_push_function(server, server_push);
	gnutls_transport_set_pull_function(server, server_pull);
	gnutls_transport_set_ptr(server, server);

	assert(gnutls_certificate_allocate_credentials(&clientx509cred)>=0);

	assert(gnutls_certificate_set_x509_trust_mem(clientx509cred, &ca_cert, GNUTLS_X509_FMT_PEM)>=0);

	assert(gnutls_init(&client, GNUTLS_CLIENT)>=0);

	assert(gnutls_credentials_set(client, GNUTLS_CRD_CERTIFICATE,
				      clientx509cred)>=0);

	assert(gnutls_priority_set_direct(client, "NORMAL:-VERS-ALL:+VERS-TLS1.3", NULL)>=0);
	gnutls_transport_set_push_function(client, client_push);
	gnutls_transport_set_pull_function(client, client_pull);
	gnutls_transport_set_ptr(client, client);

	HANDSHAKE(client, server);

	ret = gnutls_record_recv(client, buf, sizeof(buf));
	if (ret < 0 && ret != GNUTLS_E_AGAIN) {
		fail("unexpected error: %s\n", gnutls_strerror(ret));
	}

	gnutls_transport_set_pull_function(server, server_pull_fail);

	ret = gnutls_session_get_data2(client, &data);
	if (ret != 0) {
		fail("unexpected error: %s\n", gnutls_strerror(ret));
	}
	gnutls_free(data.data);
	gnutls_transport_set_pull_function(server, server_pull);

	ret = gnutls_record_recv(client, buf, sizeof(buf));
	if (ret < 0 && ret != GNUTLS_E_AGAIN) {
		fail("unexpected error: %s\n", gnutls_strerror(ret));
	}

	gnutls_bye(client, GNUTLS_SHUT_RDWR);
	gnutls_bye(server, GNUTLS_SHUT_RDWR);

	gnutls_deinit(client);
	gnutls_deinit(server);

	gnutls_certificate_free_credentials(serverx509cred);
	gnutls_certificate_free_credentials(clientx509cred);

	gnutls_global_deinit();
}
