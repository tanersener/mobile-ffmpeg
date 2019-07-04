/*
 * Copyright (C) 2008-2016 Free Software Foundation, Inc.
 * Copyright (C) 2016 Red Hat, Inc.
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
#include <assert.h>
#include <gnutls/gnutls.h>
#include "utils.h"
#include "eagain-common.h"

/* internal function */
int _gnutls_server_name_set_raw(gnutls_session_t session,
				gnutls_server_name_type_t type,
				const void *name, size_t name_length);

const char *side = NULL;

/* if @host is NULL certificate check is skipped */
int
_test_cli_serv(gnutls_certificate_credentials_t server_cred,
	      gnutls_certificate_credentials_t client_cred,
	      const char *serv_prio, const char *cli_prio,
	      const char *host,
	      void *priv, callback_func *client_cb, callback_func *server_cb,
	      unsigned expect_verification_failure,
	      unsigned require_cert,
	      int serv_err,
	      int cli_err)
{
	int ret;
	/* Server stuff. */
	gnutls_session_t server;
	int sret = GNUTLS_E_AGAIN;
	/* Client stuff. */
	gnutls_session_t client;
	int cret = GNUTLS_E_AGAIN;

	/* General init. */
	reset_buffers();

	/* Init server */
	gnutls_init(&server, GNUTLS_SERVER);
	gnutls_credentials_set(server, GNUTLS_CRD_CERTIFICATE,
				server_cred);
	ret = gnutls_priority_set_direct(server, serv_prio, NULL);
	if (ret < 0) {
		fail("error in server priority: %s\n", serv_prio);
	}
	gnutls_transport_set_push_function(server, server_push);
	gnutls_transport_set_pull_function(server, server_pull);
	gnutls_transport_set_ptr(server, server);

	if (require_cert)
		gnutls_certificate_server_set_request(server, GNUTLS_CERT_REQUIRE);

	ret = gnutls_init(&client, GNUTLS_CLIENT);
	if (ret < 0)
		exit(1);

	if (host) {
		if (strncmp(host, "raw:", 4) == 0) {
			assert(_gnutls_server_name_set_raw(client, GNUTLS_NAME_DNS, host+4, strlen(host+4))>=0);
			host += 4;
		} else {
			assert(gnutls_server_name_set(client, GNUTLS_NAME_DNS, host, strlen(host))>=0);
		}
	}

	ret = gnutls_credentials_set(client, GNUTLS_CRD_CERTIFICATE,
				client_cred);
	if (ret < 0)
		exit(1);

	ret = gnutls_priority_set_direct(client, cli_prio, NULL);
	if (ret < 0) {
		fail("error in client priority: %s\n", cli_prio);
	}
	gnutls_transport_set_push_function(client, client_push);
	gnutls_transport_set_pull_function(client, client_pull);
	gnutls_transport_set_ptr(client, client);

	if (cli_err == 0 && serv_err == 0) {
		HANDSHAKE(client, server);
	} else {
		HANDSHAKE_EXPECT(client, server, cli_err, serv_err);
		goto cleanup;
	}

	/* check the number of certificates received and verify */
	if (host) {
		gnutls_typed_vdata_st data[2];
		unsigned status;

		memset(data, 0, sizeof(data));

		data[0].type = GNUTLS_DT_DNS_HOSTNAME;
		data[0].data = (void*)host;

		data[1].type = GNUTLS_DT_KEY_PURPOSE_OID;
		data[1].data = (void*)GNUTLS_KP_TLS_WWW_SERVER;

		ret = gnutls_certificate_verify_peers(client, data, 2, &status);
		if (ret < 0) {
			fail("could not verify certificate: %s\n", gnutls_strerror(ret));
			exit(1);
		}

		if (expect_verification_failure && status != 0) {
			ret = status;
			goto cleanup;
		} else if (expect_verification_failure && status == 0) {
			fail("expected verification failure but verification succeeded!\n");
		}

		if (status != 0) {
			gnutls_datum_t t;
			assert(gnutls_certificate_verification_status_print(status, GNUTLS_CRT_X509, &t, 0)>=0);
			fail("could not verify certificate for '%s': %.4x: %s\n", host, status, t.data);
			gnutls_free(t.data);
			exit(1);
		}

		/* check gnutls_certificate_verify_peers3 */
		ret = gnutls_certificate_verify_peers3(client, host, &status);
		if (ret < 0) {
			fail("could not verify certificate: %s\n", gnutls_strerror(ret));
			exit(1);
		}

		if (status != 0) {
			gnutls_datum_t t;
			assert(gnutls_certificate_verification_status_print(status, GNUTLS_CRT_X509, &t, 0)>=0);
			fail("could not verify certificate3: %.4x: %s\n", status, t.data);
			gnutls_free(t.data);
			exit(1);
		}
	}

	if (cret >= 0)
		gnutls_bye(client, GNUTLS_SHUT_RDWR);
	if (sret >= 0)
		gnutls_bye(server, GNUTLS_SHUT_RDWR);

	ret = 0;
 cleanup:
	if (client_cb)
		client_cb(client, priv);
	if (server_cb)
		server_cb(server, priv);

	gnutls_deinit(client);
	gnutls_deinit(server);

	return ret;
}

/* An expected to succeed run */
void
test_cli_serv(gnutls_certificate_credentials_t server_cred,
	      gnutls_certificate_credentials_t client_cred,
	      const char *prio, const char *host,
	      void *priv, callback_func *client_cb, callback_func *server_cb)
{
	_test_cli_serv(server_cred, client_cred, prio, prio, host, priv, client_cb, server_cb, 0, 0, 0, 0);
}

int
test_cli_serv_anon(gnutls_anon_server_credentials_t server_cred,
	      gnutls_anon_client_credentials_t client_cred,
	      const char *prio)
{
	int ret;
	/* Server stuff. */
	gnutls_session_t server;
	int sret = GNUTLS_E_AGAIN;
	/* Client stuff. */
	gnutls_session_t client;
	int cret = GNUTLS_E_AGAIN;

	/* General init. */
	reset_buffers();

	/* Init server */
	gnutls_init(&server, GNUTLS_SERVER);
	gnutls_credentials_set(server, GNUTLS_CRD_ANON,
				server_cred);
	gnutls_priority_set_direct(server, prio, NULL);
	gnutls_transport_set_push_function(server, server_push);
	gnutls_transport_set_pull_function(server, server_pull);
	gnutls_transport_set_ptr(server, server);

	ret = gnutls_init(&client, GNUTLS_CLIENT);
	if (ret < 0)
		exit(1);

	ret = gnutls_credentials_set(client, GNUTLS_CRD_ANON,
				client_cred);
	if (ret < 0)
		exit(1);

	gnutls_priority_set_direct(client, prio, NULL);
	gnutls_transport_set_push_function(client, client_push);
	gnutls_transport_set_pull_function(client, client_pull);
	gnutls_transport_set_ptr(client, client);

	HANDSHAKE(client, server);

	ret = 0;

	gnutls_bye(client, GNUTLS_SHUT_RDWR);
	gnutls_bye(server, GNUTLS_SHUT_RDWR);

	gnutls_deinit(client);
	gnutls_deinit(server);

	return ret;
}

int
test_cli_serv_psk(gnutls_psk_server_credentials_t server_cred,
	      gnutls_psk_client_credentials_t client_cred,
	      const char *prio)
{
	int ret;
	/* Server stuff. */
	gnutls_session_t server;
	int sret = GNUTLS_E_AGAIN;
	/* Client stuff. */
	gnutls_session_t client;
	int cret = GNUTLS_E_AGAIN;

	/* General init. */
	reset_buffers();

	/* Init server */
	gnutls_init(&server, GNUTLS_SERVER);
	gnutls_credentials_set(server, GNUTLS_CRD_PSK,
				server_cred);
	gnutls_priority_set_direct(server, prio, NULL);
	gnutls_transport_set_push_function(server, server_push);
	gnutls_transport_set_pull_function(server, server_pull);
	gnutls_transport_set_ptr(server, server);

	ret = gnutls_init(&client, GNUTLS_CLIENT);
	if (ret < 0)
		exit(1);

	ret = gnutls_credentials_set(client, GNUTLS_CRD_PSK,
				client_cred);
	if (ret < 0)
		exit(1);

	gnutls_priority_set_direct(client, prio, NULL);
	gnutls_transport_set_push_function(client, client_push);
	gnutls_transport_set_pull_function(client, client_pull);
	gnutls_transport_set_ptr(client, client);

	HANDSHAKE(client, server);

	ret = 0;

	gnutls_bye(client, GNUTLS_SHUT_RDWR);
	gnutls_bye(server, GNUTLS_SHUT_RDWR);

	gnutls_deinit(client);
	gnutls_deinit(server);

	return ret;
}

void
test_cli_serv_cert(gnutls_certificate_credentials_t server_cred,
	      gnutls_certificate_credentials_t client_cred,
	      const char *serv_prio, const char *cli_prio, const char *host)
{
	_test_cli_serv(server_cred, client_cred, serv_prio, cli_prio, host, NULL, NULL, NULL, 0, 1, 0, 0);
}

void
test_cli_serv_expect(gnutls_certificate_credentials_t server_cred,
	      gnutls_certificate_credentials_t client_cred,
	      const char *serv_prio, const char *cli_prio, const char *host,
	      int serv_err, int cli_err)
{
	_test_cli_serv(server_cred, client_cred, serv_prio, cli_prio, host, NULL, NULL, NULL, 0, 0, serv_err, cli_err);
}

/* An expected to fail verification run. Returns verification status */
unsigned
test_cli_serv_vf(gnutls_certificate_credentials_t server_cred,
	      gnutls_certificate_credentials_t client_cred,
	      const char *prio, const char *host)
{
	return _test_cli_serv(server_cred, client_cred, prio, prio, host, NULL, NULL, NULL, 1, 0, 0, 0);
}

void print_dh_params_info(gnutls_session_t session)
{
	unsigned i;
	int ret;
	gnutls_datum_t pubkey, gen, prime;

	ret = gnutls_dh_get_prime_bits(session);
	if (ret < 512) {
		fail("client: too small prime size: %d\n", ret);
	}

	ret = gnutls_dh_get_secret_bits(session);
	if (ret < 225) {
		fail("client: too small secret key size: %d\n", ret);
	}

	ret = gnutls_dh_get_pubkey(session, &pubkey);
	if (ret < 0) {
		fail("error retrieving the public key\n");
	}

	if (pubkey.size == 0) {
		fail("retrieved pubkey is empty!\n");
	}

	printf("pubkey: \n");
	for (i=0;i<pubkey.size;i++) {
		printf("%.2x", (unsigned)pubkey.data[i]);
	}
	printf("\n");

	gnutls_free(pubkey.data);

	ret = gnutls_dh_get_group(session, &gen, &prime);
	if (ret < 0 || gen.size == 0 || prime.size == 0) {
		fail("error retrieving the group info\n");
	}

	printf("prime: \n");
	for (i=0;i<prime.size;i++) {
		printf("%.2x", (unsigned)prime.data[i]);
	}
	printf("\n");

	printf("generator: \n");
	for (i=0;i<gen.size;i++) {
		printf("%.2x", (unsigned)gen.data[i]);
	}
	printf("\n");
	gnutls_free(gen.data);
	gnutls_free(prime.data);
}

