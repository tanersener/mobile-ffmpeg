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
#include <gnutls/abstract.h>

#define RANDOMIZE
#include "cert-common.h"
#include "cmocka-common.h"

/* This tests operation under non-blocking mode in TLS1.2/TLS1.3
 * rekey/rehandshake.
 */
static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "<%d>| %s", level, str);
}

#define MAX_BUF 1024
#define MSG "Hello TLS, and hi and how are you and more data here... and more... and even more and even more more data..."

static unsigned int cert_asked = 0;

static int cert_callback(gnutls_session_t session,
			 const gnutls_datum_t * req_ca_rdn, int nreqs,
			 const gnutls_pk_algorithm_t * sign_algos,
			 int sign_algos_length, gnutls_pcert_st ** pcert,
			 unsigned int *pcert_length, gnutls_privkey_t * pkey)
{
	cert_asked = 1;
	*pcert_length = 0;
	*pcert = NULL;
	*pkey = NULL;

	return 0;
}

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
	cert_asked = 0;
	gnutls_global_init();
	gnutls_global_set_log_function(tls_log_func);

	/* Init server */
	assert_return_code(gnutls_certificate_allocate_credentials(&serverx509cred), 0);
	assert_return_code(gnutls_certificate_set_x509_key_mem(serverx509cred,
					    &server_cert, &server_key,
					    GNUTLS_X509_FMT_PEM), 0);
	ret = gnutls_init(&server, GNUTLS_SERVER|GNUTLS_POST_HANDSHAKE_AUTH);
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

	gnutls_certificate_set_retrieve_function2(clientx509cred, cert_callback);

	ret = gnutls_init(&client, GNUTLS_CLIENT|GNUTLS_AUTO_REAUTH|GNUTLS_POST_HANDSHAKE_AUTH);
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

	if (rehsk == 1) {
		char b[1];
		unsigned hstarted = 0;

		do {
			sret = gnutls_rehandshake(server);
		} while (sret == GNUTLS_E_AGAIN || sret == GNUTLS_E_INTERRUPTED);
		assert_true(sret == 0);
		assert_true(gnutls_record_get_direction(server)==1);

		sret = cret = GNUTLS_E_AGAIN;
		do {
			if (!hstarted) {
				sret = gnutls_record_recv(server, b, 1);
				if (sret == GNUTLS_E_INTERRUPTED) sret = GNUTLS_E_AGAIN;

				if (sret == GNUTLS_E_REHANDSHAKE) {
					hstarted = 1;
					sret = GNUTLS_E_AGAIN;
				}
				assert_true(sret == GNUTLS_E_AGAIN);
			}

			if (sret == GNUTLS_E_AGAIN && hstarted) {
				sret = gnutls_handshake (server);
				if (sret == GNUTLS_E_INTERRUPTED) sret = GNUTLS_E_AGAIN;
				assert_true(sret == GNUTLS_E_AGAIN || sret == 0);
			}

			/* we are done in client side */
			if (hstarted && gnutls_record_get_direction(client) == 0 && to_client_len == 0)
				cret = 0;

			if (cret == GNUTLS_E_AGAIN) {
				cret = gnutls_record_recv(client, b, 1);
				if (cret == GNUTLS_E_INTERRUPTED) cret = GNUTLS_E_AGAIN;
			}
			assert_true(cret == GNUTLS_E_AGAIN || cret >= 0);

		} while (cret == GNUTLS_E_AGAIN || sret == GNUTLS_E_AGAIN);
		assert_true(hstarted != 0);
	} else {
		char b[1];

		gnutls_certificate_server_set_request(server, GNUTLS_CERT_REQUEST);

		do {
			sret = gnutls_reauth(server, 0);
		} while (sret == GNUTLS_E_INTERRUPTED);

		assert_true(sret == GNUTLS_E_AGAIN || sret >= 0);

		cret = GNUTLS_E_AGAIN;
		do {
			if (cret == GNUTLS_E_AGAIN) {
				cret = gnutls_record_recv(client, b, 1);
				if (cret == GNUTLS_E_INTERRUPTED) cret = GNUTLS_E_AGAIN;
			}

			if (sret == GNUTLS_E_AGAIN) {
				sret = gnutls_reauth(server, 0);
				if (sret == GNUTLS_E_INTERRUPTED) sret = GNUTLS_E_AGAIN;
			}

			/* we are done in client side */
			if (gnutls_record_get_direction(client) == 0 && to_client_len == 0)
				cret = 0;
		} while (cret == GNUTLS_E_AGAIN || sret == GNUTLS_E_AGAIN);
	}
	assert_return_code(cret, 0);
	assert_return_code(sret, 0);
	assert_return_code(cert_asked, 1);

	msglen = strlen(MSG);
	TRANSFER(client, server, MSG, msglen, buffer, MAX_BUF);

	assert_true(gnutls_bye(client, GNUTLS_SHUT_WR)>=0);
	assert_true(gnutls_bye(server, GNUTLS_SHUT_WR)>=0);

	gnutls_deinit(client);
	gnutls_deinit(server);

	gnutls_certificate_free_credentials(serverx509cred);
	gnutls_certificate_free_credentials(clientx509cred);

	gnutls_global_deinit();
}

static void tls12_async_handshake(void **glob_state)
{
	async_handshake(glob_state, "NORMAL:-VERS-ALL:+VERS-TLS1.2", 1);
}

static void tls13_async_handshake(void **glob_state)
{
	async_handshake(glob_state, "NORMAL:-VERS-ALL:+VERS-TLS1.3", 0);
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(tls12_async_handshake),
		cmocka_unit_test(tls13_async_handshake),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
