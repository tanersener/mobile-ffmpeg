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

#include "cert-common.h"
#include "cmocka-common.h"

/* This program tests server initiated rehandshake */

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "<%d>| %s", level, str);
}

#define MAX_REHANDSHAKES 16

static void test_rehandshake(void **glob_state, unsigned appdata)
{
	/* Server stuff. */
	gnutls_certificate_credentials_t serverx509cred;
	gnutls_session_t server;
	int sret = GNUTLS_E_AGAIN;
	/* Client stuff. */
	gnutls_certificate_credentials_t clientx509cred;
	gnutls_session_t client;
	int cret = GNUTLS_E_AGAIN;
	char buffer[64];
	int ret;
	unsigned i;

	/* General init. */
	reset_buffers();
	ret = gnutls_global_init();
	assert_return_code(ret, 0);

	gnutls_global_set_log_function(tls_log_func);

	/* Init server */
	ret = gnutls_certificate_allocate_credentials(&serverx509cred);
	assert_return_code(ret, 0);

	ret = gnutls_certificate_set_x509_key_mem(serverx509cred,
						  &server_cert, &server_key,
						  GNUTLS_X509_FMT_PEM);
	assert_return_code(ret, 0);

	ret = gnutls_init(&server, GNUTLS_SERVER);
	assert_return_code(ret, 0);

	ret = gnutls_credentials_set(server, GNUTLS_CRD_CERTIFICATE,
				     serverx509cred);
	assert_return_code(ret, 0);

	ret = gnutls_priority_set_direct(server, "NORMAL:-VERS-TLS-ALL:+VERS-TLS1.1:+VERS-TLS1.2", NULL);
	assert_return_code(ret, 0);

	gnutls_transport_set_push_function(server, server_push);
	gnutls_transport_set_pull_function(server, server_pull);
	gnutls_transport_set_ptr(server, server);

	/* Init client */
	ret = gnutls_certificate_allocate_credentials(&clientx509cred);
	assert_return_code(ret, 0);

	ret = gnutls_init(&client, GNUTLS_CLIENT);
	assert_return_code(ret, 0);

	ret = gnutls_credentials_set(client, GNUTLS_CRD_CERTIFICATE,
				     clientx509cred);
	assert_return_code(ret, 0);

	ret = gnutls_priority_set_direct(client, "NORMAL:-VERS-TLS-ALL:+VERS-TLS1.1:+VERS-TLS1.2", NULL);
	assert_return_code(ret, 0);

	gnutls_transport_set_push_function(client, client_push);
	gnutls_transport_set_pull_function(client, client_pull);
	gnutls_transport_set_ptr(client, client);

	HANDSHAKE(client, server);

	if (appdata) {
		/* send application data prior to handshake */
		ssize_t n;
		char b[1];

		do {
			sret = gnutls_rehandshake(server);
		} while (sret == GNUTLS_E_AGAIN);

		do {
			n = gnutls_record_recv(client, b, 1);
		} while(n == GNUTLS_E_AGAIN);

		assert_int_equal(n, GNUTLS_E_REHANDSHAKE);

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

		HANDSHAKE(client, server);
	} else {
		ssize_t n;
		char b[1];

		for (i=0;i<MAX_REHANDSHAKES;i++) {
			sret = gnutls_rehandshake(server);

			n = gnutls_record_recv(client, b, 1);
			assert_int_equal(n, GNUTLS_E_REHANDSHAKE);

			HANDSHAKE(client, server);
		}
	}

	gnutls_bye(client, GNUTLS_SHUT_RDWR);
	gnutls_bye(server, GNUTLS_SHUT_RDWR);

	gnutls_deinit(client);
	gnutls_deinit(server);

	gnutls_certificate_free_credentials(serverx509cred);
	gnutls_certificate_free_credentials(clientx509cred);

	gnutls_global_deinit();
}

static void tls12_rehandshake_server(void **glob_state)
{
	test_rehandshake(glob_state, 0);
}

static void tls12_rehandshake_server_appdata(void **glob_state)
{
	test_rehandshake(glob_state, 1);
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(tls12_rehandshake_server),
		cmocka_unit_test(tls12_rehandshake_server_appdata),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
