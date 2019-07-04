/*
 * Copyright (C) 2018 Red Hat, Inc.
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

#include <gnutls/gnutls.h>
#include <gnutls/gnutlsxx.h>
#include <iostream>

extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include "cert-common.h"
#include <setjmp.h>
#include <cmocka.h>
#include <minmax.h>
}

/* This is a basic test for C++ API */
static void tls_log_func(int level, const char *str)
{
	std::cerr << level << "| " << str << "\n";
}

static char to_server[64 * 1024];
static size_t to_server_len = 0;

static char to_client[64 * 1024];
static size_t to_client_len = 0;

static ssize_t
client_push(gnutls_transport_ptr_t tr, const void *data, size_t len)
{
	size_t newlen;

	len = MIN(len, sizeof(to_server) - to_server_len);

	newlen = to_server_len + len;
	memcpy(to_server + to_server_len, data, len);
	to_server_len = newlen;

	return len;
}

static ssize_t
client_pull(gnutls_transport_ptr_t tr, void *data, size_t len)
{
	if (to_client_len == 0) {
		gnutls_transport_set_errno ((gnutls_session_t)tr, EAGAIN);
		return -1;
	}

	len = MIN(len, to_client_len);

	memcpy(data, to_client, len);

	memmove(to_client, to_client + len, to_client_len - len);
	to_client_len -= len;
	return len;
}

static ssize_t
server_pull(gnutls_transport_ptr_t tr, void *data, size_t len)
{
	if (to_server_len == 0) {
		gnutls_transport_set_errno ((gnutls_session_t)tr, EAGAIN);
		return -1;
	}

	len = MIN(len, to_server_len);
	memcpy(data, to_server, len);

	memmove(to_server, to_server + len, to_server_len - len);
	to_server_len -= len;

	return len;
}

static ssize_t
server_push(gnutls_transport_ptr_t tr, const void *data, size_t len)
{
	size_t newlen;

	len = MIN(len, sizeof(to_client) - to_client_len);

	newlen = to_client_len + len;
	memcpy(to_client + to_client_len, data, len);
	to_client_len = newlen;

	return len;
}

inline static void reset_buffers(void)
{
	to_server_len = 0;
	to_client_len = 0;
}

#define MSG "test message"
static void test_handshake(void **glob_state, const char *prio,
			   gnutls::server_session& server, gnutls::client_session& client)
{
        gnutls::certificate_credentials serverx509cred;
	int sret, cret;
	gnutls::certificate_credentials clientx509cred;
	char buffer[64];
	int ret;

	/* General init. */
	reset_buffers();
	gnutls_global_set_log_function(tls_log_func);

	try {
		serverx509cred.set_x509_key(server_cert, server_key, GNUTLS_X509_FMT_PEM);
		server.set_credentials(serverx509cred);

		server.set_priority(prio, NULL);

		server.set_transport_push_function(server_push);
		server.set_transport_pull_function(server_pull);
		server.set_transport_ptr(server.ptr());

		client.set_priority(prio, NULL);
		client.set_credentials(clientx509cred);

		client.set_transport_push_function(client_push);
		client.set_transport_pull_function(client_pull);
		client.set_transport_ptr(client.ptr());
	}
	catch (std::exception &ex) {
		std::cerr << "Exception caught: " << ex.what() << std::endl;
		fail();
	}

	sret = cret = GNUTLS_E_AGAIN;

	do {
		if (cret == GNUTLS_E_AGAIN) {
			try {
				cret = client.handshake();
			} catch(gnutls::exception &ex) {
				cret = ex.get_code();
				if (cret == GNUTLS_E_INTERRUPTED || cret == GNUTLS_E_AGAIN)
					cret = GNUTLS_E_AGAIN;
			}
		}
		if (sret == GNUTLS_E_AGAIN) {
			try {
				sret = server.handshake();
			} catch(gnutls::exception &ex) {
				sret = ex.get_code();
				if (sret == GNUTLS_E_INTERRUPTED || sret == GNUTLS_E_AGAIN)
					sret = GNUTLS_E_AGAIN;
			}
		}
	}
	while ((cret == GNUTLS_E_AGAIN || (cret == 0 && sret == GNUTLS_E_AGAIN)) &&
	       (sret == GNUTLS_E_AGAIN || (sret == 0 && cret == GNUTLS_E_AGAIN)));

	if (sret < 0 || cret < 0) {
		fail();
	}

	try {
		client.send(MSG, sizeof(MSG)-1);
		ret = server.recv(buffer, sizeof(buffer));

		assert(ret == sizeof(MSG)-1);
		assert(memcmp(buffer, MSG, sizeof(MSG)-1) == 0);
		
		client.bye(GNUTLS_SHUT_WR);
		server.bye(GNUTLS_SHUT_WR);
	}
	catch (std::exception &ex) {
		std::cerr << "Exception caught: " << ex.what() << std::endl;
		fail();
	}

	return;
}

static void tls_handshake(void **glob_state)
{
        gnutls::server_session server;
	gnutls::client_session client;

	test_handshake(glob_state, "NORMAL", server, client);
}

static void tls_handshake_alt(void **glob_state)
{
        gnutls::server_session server(0);
	gnutls::client_session client(0);

	test_handshake(glob_state, "NORMAL", server, client);
}

static void tls12_handshake(void **glob_state)
{
        gnutls::server_session server;
	gnutls::client_session client;

	test_handshake(glob_state, "NORMAL:-VERS-TLS-ALL:+VERS-TLS1.2", server, client);
}

static void tls13_handshake(void **glob_state)
{
        gnutls::server_session server;
	gnutls::client_session client;

	test_handshake(glob_state, "NORMAL:-VERS-TLS-ALL:+VERS-TLS1.3", server, client);
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(tls_handshake),
		cmocka_unit_test(tls_handshake_alt),
		cmocka_unit_test(tls13_handshake),
		cmocka_unit_test(tls12_handshake)
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
