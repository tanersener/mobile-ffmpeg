/*
 * Copyright (C) 2015 Red Hat, Inc.
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

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <gnutls/gnutls.h>
#include "utils.h"

#define SKIP16(pos, total) { \
	uint16_t _s; \
	if (pos+2 > total) fail("error\n"); \
	_s = (msg->data[pos] << 8) | msg->data[pos+1]; \
	if ((size_t)(pos+2+_s) > total) fail("error\n"); \
	pos += 2+_s; \
	}

#define SKIP8(pos, total) { \
	uint8_t _s; \
	if (pos+1 > total) fail("error\n"); \
	_s = msg->data[pos]; \
	if ((size_t)(pos+1+_s) > total) fail("error\n"); \
	pos += 1+_s; \
	}

#define HANDSHAKE_SESSION_ID_POS 34

static size_t server_max_send_size;
static size_t client_max_send_size;

#define SERVER_PUSH_ADD if (len > server_max_send_size + 5+32) fail("max record set to %d, len: %d\n", (int)server_max_send_size, (int)len);
#include "eagain-common.h"

#include "cert-common.h"

/* This tests whether the max-record extension is respected on TLS.
 */

const char *side;

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "%s|<%d>| %s", side, level, str);
}

struct handshake_cb_data_st {
	gnutls_session_t session;
	bool found_max_record_size;
	bool found_record_size_limit;
};

static struct handshake_cb_data_st server_handshake_cb_data;
static struct handshake_cb_data_st client_handshake_cb_data;

static int ext_callback(void *ctx, unsigned tls_id, const unsigned char *data, unsigned size)
{
	struct handshake_cb_data_st *cb_data = ctx;
	if (tls_id == 1) {	/* max record size */
		cb_data->found_max_record_size = 1;
	} else if (tls_id == 28) { /* record size limit */
		cb_data->found_record_size_limit = 1;
	}
	return 0;
}

static int handshake_callback(gnutls_session_t session, unsigned int htype,
	unsigned post, unsigned int incoming, const gnutls_datum_t *msg)
{
	int ret;
	unsigned pos;
	gnutls_datum_t mmsg;

	if (!post)
		return 0;

	switch (htype) {
	case GNUTLS_HANDSHAKE_CLIENT_HELLO:
		assert(msg->size >= HANDSHAKE_SESSION_ID_POS);
		pos = HANDSHAKE_SESSION_ID_POS;
		SKIP8(pos, msg->size);
		SKIP16(pos, msg->size);
		SKIP8(pos, msg->size);

		mmsg.data = &msg->data[pos];
		mmsg.size = msg->size - pos;
		ret = gnutls_ext_raw_parse(&server_handshake_cb_data, ext_callback, &mmsg, 0);
		assert(ret >= 0);
		break;
	case GNUTLS_HANDSHAKE_ENCRYPTED_EXTENSIONS:
		ret = gnutls_ext_raw_parse(&client_handshake_cb_data, ext_callback, msg, 0);
		assert(ret >= 0);
		break;
	case GNUTLS_HANDSHAKE_SERVER_HELLO:
		assert(msg->size >= HANDSHAKE_SESSION_ID_POS);
		pos = HANDSHAKE_SESSION_ID_POS;
		SKIP8(pos, msg->size);
		pos += 3;

		mmsg.data = &msg->data[pos];
		mmsg.size = msg->size - pos;
		ret = gnutls_ext_raw_parse(&client_handshake_cb_data, ext_callback, &mmsg, 0);
		assert(ret >= 0);
		break;
	default:
		break;
	}
	return 0;
}

#define MAX_BUF 16384
static char buffer[MAX_BUF];

struct test_exp_st {
	int error;
	size_t size;
	bool max_record_size;
	bool record_size_limit;
};

struct test_st {
	const char *prio;
	size_t server_max_size;
	size_t client_max_size;

	struct test_exp_st server_exp;
	struct test_exp_st client_exp;
};

static void check_exts(const struct test_exp_st *exp,
		       struct handshake_cb_data_st *data)
{
	if (exp->max_record_size && !data->found_max_record_size)
		fail("%s: didn't see max_record_size\n", side);
	if (!exp->max_record_size && data->found_max_record_size)
		fail("%s: did see max_record_size\n", side);

	if (exp->record_size_limit && !data->found_record_size_limit)
		fail("%s: didn't see record_size_limit\n", side);
	if (!exp->record_size_limit && data->found_record_size_limit)
		fail("%s: did see record_size_limit\n", side);
}

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

	memset(&server_handshake_cb_data, 0, sizeof(server_handshake_cb_data));
	memset(&client_handshake_cb_data, 0, sizeof(client_handshake_cb_data));

	global_init();

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

	gnutls_priority_set_direct(server, test->prio, NULL);

	ret = gnutls_record_set_max_size(server, test->server_max_size);
	if (ret != test->server_exp.error)
		fail("server: unexpected error from gnutls_record_set_max_size()");
	if (ret == 0)
		server_max_send_size = test->server_max_size;
	else
		server_max_send_size = MAX_BUF;

	gnutls_transport_set_push_function(server, server_push);
	gnutls_transport_set_pull_function(server, server_pull);
	gnutls_transport_set_pull_timeout_function(server,
						   server_pull_timeout_func);
	gnutls_transport_set_ptr(server, server);

	server_handshake_cb_data.session = server;
	gnutls_handshake_set_hook_function(server,
					   GNUTLS_HANDSHAKE_CLIENT_HELLO,
					   GNUTLS_HOOK_POST,
					   handshake_callback);


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

	ret = gnutls_priority_set_direct(client, test->prio, NULL);
	if (ret < 0)
		exit(1);

	ret = gnutls_record_set_max_size(client, test->client_max_size);
	if (ret != test->client_exp.error)
		fail("client: unexpected error from gnutls_record_set_max_size()");
	if (ret == 0)
		client_max_send_size = test->client_max_size;
	else
		client_max_send_size = MAX_BUF;

	gnutls_transport_set_push_function(client, client_push);
	gnutls_transport_set_pull_function(client, client_pull);
	gnutls_transport_set_pull_timeout_function(client,
						   client_pull_timeout_func);
	gnutls_transport_set_ptr(client, client);

	client_handshake_cb_data.session = client;
	gnutls_handshake_set_hook_function(client,
					   GNUTLS_HANDSHAKE_ANY,
					   GNUTLS_HOOK_POST,
					   handshake_callback);

	HANDSHAKE(client, server);

	memset(buffer, 1, sizeof(buffer));
	ret = gnutls_record_send(server, buffer, server_max_send_size + 1);
	if (ret < 0) {
		gnutls_perror(ret);
		exit(1);
	}
	if (ret != (int)test->server_exp.size)
		fail("server: unexpected record size sent: %d (%d)\n",
		     ret, (int)test->server_exp.size);
	success("server: did not send a %d-byte packet\n", (int)server_max_send_size + 1);

	ret = gnutls_record_send(server, buffer, server_max_send_size);
	if (ret < 0) {
		gnutls_perror(ret);
		exit(1);
	}
	success("server: did send a %d-byte packet\n", (int)server_max_send_size);

	ret = gnutls_record_send(client, buffer, client_max_send_size + 1);
	if (ret < 0) {
		gnutls_perror(ret);
		exit(1);
	}
	if (ret != (int)test->client_exp.size)
		fail("client: unexpected record size sent: %d (%d)\n",
		     ret, (int)test->client_exp.size);
	success("client: did not send a %d-byte packet\n", (int)client_max_send_size + 1);

	ret = gnutls_record_send(client, buffer, client_max_send_size);
	if (ret < 0) {
		gnutls_perror(ret);
		exit(1);
	}
	success("client: did send a %d-byte packet\n", (int)client_max_send_size);

	gnutls_bye(client, GNUTLS_SHUT_RDWR);
	gnutls_bye(server, GNUTLS_SHUT_RDWR);

	gnutls_deinit(client);
	gnutls_deinit(server);

	gnutls_certificate_free_credentials(serverx509cred);
	gnutls_certificate_free_credentials(clientx509cred);

	gnutls_global_deinit();

	reset_buffers();

	check_exts(&test->server_exp,
		   &server_handshake_cb_data);
	check_exts(&test->client_exp,
		   &client_handshake_cb_data);
}

static const struct test_st tests[] = {
	{
		.prio = "NORMAL:-VERS-ALL:+VERS-TLS1.2",
		.server_max_size = 511,
		.client_max_size = 511,
		.server_exp = {
			.error = GNUTLS_E_INVALID_REQUEST,
			.size = 16384,
			.max_record_size = 0,
			.record_size_limit = 1
		},
		.client_exp = {
			.error = GNUTLS_E_INVALID_REQUEST,
			.size = 16384,
			.max_record_size = 0,
			.record_size_limit = 1
		}
	},
	{
		.prio = "NORMAL:-VERS-ALL:+VERS-TLS1.2",
		.server_max_size = 512,
		.client_max_size = 512,
		.server_exp = {
			.error = 0,
			.size = 512,
			.max_record_size = 1,
			.record_size_limit = 1
		},
		.client_exp = {
			.error = 0,
			.size = 512,
			.max_record_size = 0,
			.record_size_limit = 1
		}
	},
	{
		.prio = "NORMAL:-VERS-ALL:+VERS-TLS1.2",
		.server_max_size = 8192,
		.client_max_size = 8192,
		.server_exp = {
			.error = 0,
			.size = 8192,
			.max_record_size = 0,
			.record_size_limit = 1
		},
		.client_exp = {
			.error = 0,
			.size = 8192,
			.max_record_size = 0,
			.record_size_limit = 1
		}
	},
	{
		.prio = "NORMAL:-VERS-ALL:+VERS-TLS1.2",
		.server_max_size = 16384,
		.client_max_size = 16384,
		.server_exp = {
			.error = 0,
			.size = 16384,
			.max_record_size = 0,
			.record_size_limit = 1
		},
		.client_exp = {
			.error = 0,
			.size = 16384,
			.max_record_size = 0,
			.record_size_limit = 1
		}
	},
	{
		.prio = "NORMAL:-VERS-ALL:+VERS-TLS1.2",
		.server_max_size = 16385,
		.client_max_size = 16385,
		.server_exp = {
			.error = GNUTLS_E_INVALID_REQUEST,
			.size = 16384,
			.max_record_size = 0,
			.record_size_limit = 1
		},
		.client_exp = {
			.error = GNUTLS_E_INVALID_REQUEST,
			.size = 16384,
			.max_record_size = 0,
			.record_size_limit = 1
		}
	},

	{
		.prio = "NORMAL:-VERS-ALL:+VERS-TLS1.3",
		.server_max_size = 511,
		.client_max_size = 511,
		.server_exp = {
			.error = GNUTLS_E_INVALID_REQUEST,
			.size = 16384,
			.max_record_size = 0,
			.record_size_limit = 1
		},
		.client_exp = {
			.error = GNUTLS_E_INVALID_REQUEST,
			.size = 16384,
			.max_record_size = 0,
			.record_size_limit = 1
		}
	},
	{
		.prio = "NORMAL:-VERS-ALL:+VERS-TLS1.3",
		.server_max_size = 512,
		.client_max_size = 512,
		.server_exp = {
			.error = 0,
			.size = 512,
			.max_record_size = 1,
			.record_size_limit = 1
		},
		.client_exp = {
			.error = 0,
			.size = 512,
			.max_record_size = 0,
			.record_size_limit = 1
		}
	},
	{
		.prio = "NORMAL:-VERS-ALL:+VERS-TLS1.3",
		.server_max_size = 8192,
		.client_max_size = 8192,
		.server_exp = {
			.error = 0,
			.size = 8192,
			.max_record_size = 0,
			.record_size_limit = 1
		},
		.client_exp = {
			.error = 0,
			.size = 8192,
			.max_record_size = 0,
			.record_size_limit = 1
		}
	},
	{
		.prio = "NORMAL:-VERS-ALL:+VERS-TLS1.3",
		.server_max_size = 16384,
		.client_max_size = 16384,
		.server_exp = {
			.error = 0,
			.size = 16384,
			.max_record_size = 0,
			.record_size_limit = 1
		},
		.client_exp = {
			.error = 0,
			.size = 16384,
			.max_record_size = 0,
			.record_size_limit = 1
		}
	},
	{
		.prio = "NORMAL:-VERS-ALL:+VERS-TLS1.3",
		.server_max_size = 16383,
		.client_max_size = 16384,
		.server_exp = {
			.error = 0,
			.size = 16383,
			.max_record_size = 0,
			.record_size_limit = 1
		},
		.client_exp = {
			.error = 0,
			.size = 16383,
			.max_record_size = 0,
			.record_size_limit = 1
		}
	},
	{
		.prio = "NORMAL:-VERS-ALL:+VERS-TLS1.3",
		.server_max_size = 16385,
		.client_max_size = 16385,
		.server_exp = {
			.error = GNUTLS_E_INVALID_REQUEST,
			.size = 16384,
			.max_record_size = 0,
			.record_size_limit = 1
		},
		.client_exp = {
			.error = GNUTLS_E_INVALID_REQUEST,
			.size = 16384,
			.max_record_size = 0,
			.record_size_limit = 1
		}
	}
};

void doit(void)
{
	size_t i;
	for (i = 0; i < sizeof(tests)/sizeof(tests[0]); i++)
		start(&tests[i]);
}
