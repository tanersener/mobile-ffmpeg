/*
 * Copyright (C) 2015, 2019 Red Hat, Inc.
 *
 * Author: Nikos Mavrogiannopoulos, Daiki Ueno
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

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
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

#define MAX_BUF 16384
static char buffer[MAX_BUF];

struct test_exp_st {
	int error;
	size_t size;
};

struct test_st {
	const char *prio;
	size_t server_max_size;
	size_t client_max_size;

	struct test_exp_st server_exp;
	struct test_exp_st client_exp;
};

static void start(const struct test_st *test)
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

	printf("testing server limit %d and client limit %d in %s\n",
	       (int)test->server_max_size, (int)test->client_max_size,
	       test->prio);

	global_init();

	/* General init. */
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(6);

	/* Init server */
	assert(gnutls_certificate_allocate_credentials(&serverx509cred) >= 0);
	assert(gnutls_certificate_set_x509_key_mem(serverx509cred,
						   &server2_cert, &server2_key,
						   GNUTLS_X509_FMT_PEM) >= 0);

	assert(gnutls_init(&server, GNUTLS_SERVER) >= 0);
	assert(gnutls_credentials_set(server, GNUTLS_CRD_CERTIFICATE,
				      serverx509cred) >= 0);

	assert(gnutls_priority_set_direct(server, test->prio, NULL) >= 0);

	ret = gnutls_record_set_max_recv_size(server, test->server_max_size);
	if (ret != test->server_exp.error)
		fail("server: unexpected error from gnutls_record_set_max_recv_size()");

	gnutls_transport_set_push_function(server, server_push);
	gnutls_transport_set_pull_function(server, server_pull);
	gnutls_transport_set_pull_timeout_function(server,
						   server_pull_timeout_func);
	gnutls_transport_set_ptr(server, server);


	/* Init client */
	assert(gnutls_certificate_allocate_credentials(&clientx509cred) >= 0);

	assert(gnutls_certificate_set_x509_trust_mem(clientx509cred, &ca2_cert, GNUTLS_X509_FMT_PEM) >= 0);

	assert(gnutls_init(&client, GNUTLS_CLIENT) >= 0);

	assert(gnutls_credentials_set(client, GNUTLS_CRD_CERTIFICATE,
				      clientx509cred) >= 0);

	assert(gnutls_priority_set_direct(client, test->prio, NULL) >= 0);

	ret = gnutls_record_set_max_recv_size(client, test->client_max_size);
	if (ret != test->client_exp.error)
		fail("client: unexpected error from gnutls_record_set_max_recv_size()");

	gnutls_transport_set_push_function(client, client_push);
	gnutls_transport_set_pull_function(client, client_pull);
	gnutls_transport_set_pull_timeout_function(client,
						   client_pull_timeout_func);
	gnutls_transport_set_ptr(client, client);

	HANDSHAKE(client, server);

	memset(buffer, 1, sizeof(buffer));
	ret = gnutls_record_send(server, buffer, test->client_max_size + 1);
	if (ret < 0) {
		gnutls_perror(ret);
		exit(1);
	}
	if (ret != (int)test->server_exp.size)
		fail("server: unexpected record size sent: %d (%d)\n",
		     ret, (int)test->server_exp.size);
	success("server: did not send a %d-byte packet\n",
		(int)test->server_exp.size);

	ret = gnutls_record_send(server, buffer, test->client_max_size);
	if (ret < 0) {
		gnutls_perror(ret);
		exit(1);
	}
	success("server: did send a %d-byte packet\n",
		(int)test->client_max_size);

	ret = gnutls_record_send(client, buffer, test->server_max_size + 1);
	if (ret < 0) {
		gnutls_perror(ret);
		exit(1);
	}
	if (ret != (int)test->client_exp.size)
		fail("client: unexpected record size sent: %d (%d)\n",
		     ret, (int)test->client_exp.size);
	success("client: did not send a %d-byte packet\n",
		(int)test->server_max_size + 1);

	ret = gnutls_record_send(client, buffer, test->server_max_size);
	if (ret < 0) {
		gnutls_perror(ret);
		exit(1);
	}
	success("client: did send a %d-byte packet\n",
		(int)test->server_max_size);

	gnutls_bye(client, GNUTLS_SHUT_RDWR);
	gnutls_bye(server, GNUTLS_SHUT_RDWR);

	gnutls_deinit(client);
	gnutls_deinit(server);

	gnutls_certificate_free_credentials(serverx509cred);
	gnutls_certificate_free_credentials(clientx509cred);

	gnutls_global_deinit();

	reset_buffers();
}

static const struct test_st tests[] = {
	{
		.prio = "NORMAL:-VERS-ALL:+VERS-TLS1.2",
		.server_max_size = 512,
		.client_max_size = 16384,
		.server_exp = {
			.error = 0,
			.size = 16384,
		},
		.client_exp = {
			.error = 0,
			.size = 512,
		}
	},
	{
		.prio = "NORMAL:-VERS-ALL:+VERS-TLS1.2",
		.server_max_size = 16384,
		.client_max_size = 512,
		.server_exp = {
			.error = 0,
			.size = 512,
		},
		.client_exp = {
			.error = 0,
			.size = 16384,
		}
	},
	{
		.prio = "NORMAL:-VERS-ALL:+VERS-TLS1.3",
		.server_max_size = 512,
		.client_max_size = 16384,
		.server_exp = {
			.error = 0,
			.size = 16384,
		},
		.client_exp = {
			.error = 0,
			.size = 512,
		}
	},
	{
		.prio = "NORMAL:-VERS-ALL:+VERS-TLS1.3",
		.server_max_size = 16384,
		.client_max_size = 512,
		.server_exp = {
			.error = 0,
			.size = 512,
		},
		.client_exp = {
			.error = 0,
			.size = 16384,
		}
	},
	{
		.prio = "NORMAL",
		.server_max_size = 512,
		.client_max_size = 16384,
		.server_exp = {
			.error = 0,
			.size = 16384,
		},
		.client_exp = {
			.error = 0,
			.size = 512,
		}
	},
	{
		.prio = "NORMAL",
		.server_max_size = 16384,
		.client_max_size = 512,
		.server_exp = {
			.error = 0,
			.size = 512,
		},
		.client_exp = {
			.error = 0,
			.size = 16384,
		}
	}
};

void doit(void)
{
	size_t i;
	for (i = 0; i < sizeof(tests)/sizeof(tests[0]); i++)
		start(&tests[i]);
}
