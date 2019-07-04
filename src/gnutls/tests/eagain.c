/*
 * Copyright (C) 2008-2012 Free Software Foundation, Inc.
 * Copyright (C) 2018 Red Hat, Inc.
 *
 * Author: Simon Josefsson, Nikos Mavrogiannopoulos
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
#include <gnutls/crypto.h>

#define RANDOMIZE
#include "cert-common.h"
#include "cmocka-common.h"

/* This tests operation under non-blocking mode in TLS1.2/TLS1.3
 * as well as operation under TLS1.2 re-handshake.
 */
static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "<%d>| %s", level, str);
}

#define MAX_BUF 1024
#define MSG "Hello TLS, and hi and how are you and more data here... and more... and even more and even more more data..."

static void async_handshake(void **glob_state, const char *prio, unsigned rehsk)
{
	/* Server stuff. */
	gnutls_certificate_credentials_t serverx509cred;
	gnutls_session_t server;
	int sret, cret;
	/* Client stuff. */
	gnutls_certificate_credentials_t clientx509cred;
	gnutls_session_t client;
	/* Need to enable anonymous KX specifically. */
	char buffer[MAX_BUF + 1];
	int ret, transferred = 0, msglen;

	/* General init. */
	reset_buffers();
	gnutls_global_init();
	gnutls_global_set_log_function(tls_log_func);

	/* Init server */
	assert_return_code(gnutls_certificate_allocate_credentials(&serverx509cred), 0);
	assert_return_code(gnutls_certificate_set_x509_key_mem(serverx509cred,
					    &server_cert, &server_key,
					    GNUTLS_X509_FMT_PEM), 0);
	ret = gnutls_init(&server, GNUTLS_SERVER);
	assert_return_code(ret, 0);

	ret =
	    gnutls_priority_set_direct(server,
					prio,
					NULL);
	assert_return_code(ret, 0);

	ret = gnutls_credentials_set(server, GNUTLS_CRD_CERTIFICATE, serverx509cred);
	assert_return_code(ret, 0);

	gnutls_transport_set_push_function(server, server_push);
	gnutls_transport_set_pull_function(server, server_pull);
	gnutls_transport_set_ptr(server, server);

	/* Init client */

	ret = gnutls_certificate_allocate_credentials(&clientx509cred);
	assert_return_code(ret, 0);

	ret = gnutls_init(&client, GNUTLS_CLIENT);
	ret =
	    gnutls_priority_set_direct(client,
					prio,
					NULL);
	assert_return_code(ret, 0);

	ret = gnutls_credentials_set(client, GNUTLS_CRD_CERTIFICATE, clientx509cred);
	assert_return_code(ret, 0);

	gnutls_transport_set_push_function(client, client_push);
	gnutls_transport_set_pull_function(client, client_pull);
	gnutls_transport_set_ptr(client, client);

	HANDSHAKE(client, server);

	if (rehsk == 1 || rehsk == 3) {
		ssize_t n;
		char b[1];

		do {
			sret = gnutls_rehandshake(server);
		} while (sret == GNUTLS_E_AGAIN);

		do {
			n = gnutls_record_recv(client, b, 1);
		} while(n == GNUTLS_E_AGAIN);

		assert_int_equal(n, GNUTLS_E_REHANDSHAKE);

		if (rehsk == 3) {
			/* client sends app data and the server ignores them */
			do {
				cret = gnutls_record_send(client, "x", 1);
			} while (cret == GNUTLS_E_AGAIN);

			do {
				sret = gnutls_handshake(server);
			} while (sret == GNUTLS_E_AGAIN);
			assert_int_equal(sret, GNUTLS_E_GOT_APPLICATION_DATA);

			do {
				n = gnutls_record_recv(server, buffer, sizeof(buffer));
			} while(n == GNUTLS_E_AGAIN);
		}

		HANDSHAKE(client, server);
	} else if (rehsk == 2) {
		HANDSHAKE(client, server);
	}

	msglen = strlen(MSG);
	TRANSFER(client, server, MSG, msglen, buffer, MAX_BUF);

	gnutls_bye(client, GNUTLS_SHUT_WR);
	gnutls_bye(server, GNUTLS_SHUT_WR);

	gnutls_deinit(client);
	gnutls_deinit(server);

	gnutls_certificate_free_credentials(serverx509cred);
	gnutls_certificate_free_credentials(clientx509cred);

	gnutls_global_deinit();
}

static void tls12_async_handshake(void **glob_state)
{
	async_handshake(glob_state, "NORMAL:-VERS-ALL:+VERS-TLS1.2", 0);
}

static void tls12_async_rehandshake_client(void **glob_state)
{
	async_handshake(glob_state, "NORMAL:-VERS-ALL:+VERS-TLS1.2", 1);
}

static void tls12_async_rehandshake_server(void **glob_state)
{
	async_handshake(glob_state, "NORMAL:-VERS-ALL:+VERS-TLS1.2", 2);
}

static void tls12_async_rehandshake_server_appdata(void **glob_state)
{
	async_handshake(glob_state, "NORMAL:-VERS-ALL:+VERS-TLS1.2", 3);
}

static void tls13_async_handshake(void **glob_state)
{
	async_handshake(glob_state, "NORMAL:-VERS-ALL:+VERS-TLS1.3", 0);
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(tls12_async_handshake),
		cmocka_unit_test(tls12_async_rehandshake_client),
		cmocka_unit_test(tls12_async_rehandshake_server),
		cmocka_unit_test(tls12_async_rehandshake_server_appdata),
		cmocka_unit_test(tls13_async_handshake),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
