/*
 * Copyright (C) 2015-2017 Red Hat, Inc.
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

/* This program tests the various certificate key exchange methods supported
 * in gnutls */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <gnutls/gnutls.h>
#include <gnutls/dtls.h>
#include "utils.h"
#include "eagain-common.h"
#include "cert-repro-20170915.h"

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "%s|<%d>| %s", side, level, str);
}

#define USE_CERT 1
#define ASK_CERT 2
#define MSG "hello there ppl"

static void dtls_try_with_key_mtu(const char *name, const char *client_prio, gnutls_kx_algorithm_t client_kx,
		gnutls_sign_algorithm_t server_sign_algo,
		gnutls_sign_algorithm_t client_sign_algo,
		const gnutls_datum_t *serv_cert,
		const gnutls_datum_t *serv_key,
		const gnutls_datum_t *client_cert,
		const gnutls_datum_t *client_key,
		unsigned cert_flags,
		unsigned smtu)
{
	int ret;
	char buffer[256];
	/* Server stuff. */
	gnutls_certificate_credentials_t serverx509cred;
	gnutls_anon_server_credentials_t s_anoncred;
	gnutls_dh_params_t dh_params;
	const gnutls_datum_t p3 =
	    { (unsigned char *) pkcs3, strlen(pkcs3) };
	gnutls_session_t server;
	int sret = GNUTLS_E_AGAIN;
	/* Client stuff. */
	gnutls_certificate_credentials_t clientx509cred;
	gnutls_anon_client_credentials_t c_anoncred;
	gnutls_session_t client;
	int cret = GNUTLS_E_AGAIN, version;

	/* General init. */
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(6);

	reset_buffers();
	/* Init server */
	gnutls_anon_allocate_server_credentials(&s_anoncred);
	gnutls_certificate_allocate_credentials(&serverx509cred);

	ret = gnutls_certificate_set_x509_key_mem(serverx509cred,
					    serv_cert, serv_key,
					    GNUTLS_X509_FMT_PEM);
	if (ret < 0) {
		fail("Could not set key/cert: %s\n", gnutls_strerror(ret));
	}

	gnutls_dh_params_init(&dh_params);
	gnutls_dh_params_import_pkcs3(dh_params, &p3, GNUTLS_X509_FMT_PEM);
	gnutls_certificate_set_dh_params(serverx509cred, dh_params);
	gnutls_anon_set_server_dh_params(s_anoncred, dh_params);

	gnutls_init(&server, GNUTLS_SERVER|GNUTLS_DATAGRAM|GNUTLS_NONBLOCK);
	gnutls_credentials_set(server, GNUTLS_CRD_CERTIFICATE,
				serverx509cred);
	gnutls_credentials_set(server, GNUTLS_CRD_ANON, s_anoncred);

	gnutls_priority_set_direct(server,
				   "NORMAL:+ANON-ECDH:+ANON-DH:+ECDHE-RSA:+DHE-RSA:+RSA:+ECDHE-ECDSA:+CURVE-X25519",
				   NULL);
	gnutls_transport_set_push_function(server, server_push);
	gnutls_transport_set_pull_function(server, server_pull);
	gnutls_transport_set_pull_timeout_function(server, server_pull_timeout_func);
	gnutls_transport_set_ptr(server, server);
	if (smtu)
		gnutls_dtls_set_mtu (server, smtu);

	/* Init client */

	ret = gnutls_certificate_allocate_credentials(&clientx509cred);
	if (ret < 0)
		exit(1);

	if (cert_flags == USE_CERT) {
		ret = gnutls_certificate_set_x509_key_mem(clientx509cred,
						    client_cert, client_key,
						    GNUTLS_X509_FMT_PEM);
		if (ret < 0) {
			fail("Could not set key/cert: %s\n", gnutls_strerror(ret));
		}
		gnutls_certificate_server_set_request(server, GNUTLS_CERT_REQUIRE);
	} else if (cert_flags == ASK_CERT) {
		gnutls_certificate_server_set_request(server, GNUTLS_CERT_REQUEST);
	}

#if 0
	ret = gnutls_certificate_set_x509_trust_mem(clientx509cred, &ca_cert, GNUTLS_X509_FMT_PEM);
	if (ret < 0)
		exit(1);
#endif

	ret = gnutls_init(&client, GNUTLS_CLIENT|GNUTLS_DATAGRAM|GNUTLS_NONBLOCK);
	if (ret < 0)
		exit(1);

	gnutls_anon_allocate_client_credentials(&c_anoncred);
	gnutls_credentials_set(client, GNUTLS_CRD_ANON, c_anoncred);
	ret = gnutls_credentials_set(client, GNUTLS_CRD_CERTIFICATE,
				clientx509cred);
	if (ret < 0)
		exit(1);

	gnutls_transport_set_push_function(client, client_push);
	gnutls_transport_set_pull_function(client, client_pull);
	gnutls_transport_set_pull_timeout_function(client, client_pull_timeout_func);
	
	gnutls_transport_set_ptr(client, client);
	if (smtu)
		gnutls_dtls_set_mtu (client, smtu);

	ret = gnutls_priority_set_direct(client, client_prio, NULL);
	if (ret < 0) {
		exit(1);
	}
	success("negotiating %s\n", name);
	HANDSHAKE_DTLS(client, server);

	if (gnutls_kx_get(client) != client_kx) {
		fail("%s: got unexpected key exchange algorithm: %s (expected %s)\n", name, gnutls_kx_get_name(gnutls_kx_get(client)),
			gnutls_kx_get_name(client_kx));
		exit(1);
	}


	/* test signature algorithm match */
	version = gnutls_protocol_get_version(client);
	if (version >= GNUTLS_DTLS1_2) {
		ret = gnutls_sign_algorithm_get(server);
		if (ret != (int)server_sign_algo) {
			fail("%s: got unexpected server signature algorithm: %d/%s\n", name, ret, gnutls_sign_get_name(ret));
			exit(1);
		}

		ret = gnutls_sign_algorithm_get_client(server);
		if (ret != (int)client_sign_algo) {
			fail("%s: got unexpected client signature algorithm: %d/%s\n", name, ret, gnutls_sign_get_name(ret));
			exit(1);
		}

		ret = gnutls_sign_algorithm_get(client);
		if (ret != (int)server_sign_algo) {
			fail("%s: cl: got unexpected server signature algorithm: %d/%s\n", name, ret, gnutls_sign_get_name(ret));
			exit(1);
		}

		ret = gnutls_sign_algorithm_get_client(client);
		if (ret != (int)client_sign_algo) {
			fail("%s: cl: got unexpected client signature algorithm: %d/%s\n", name, ret, gnutls_sign_get_name(ret));
			exit(1);
		}
	}

	gnutls_record_send(server, MSG, strlen(MSG));

	ret = gnutls_record_recv(client, buffer, sizeof(buffer));
	if (ret == 0) {
		fail("client: Peer has closed the TLS connection\n");
		exit(1);
	} else if (ret < 0) {
		fail("client: Error: %s\n", gnutls_strerror(ret));
		exit(1);
	}

	if (ret != strlen(MSG) || memcmp(MSG, buffer, ret) != 0) {
		fail("client: Error in data received. Expected %d, got %d\n", (int)strlen(MSG), ret);
		exit(1);
	}

	gnutls_bye(client, GNUTLS_SHUT_RDWR);
	gnutls_bye(server, GNUTLS_SHUT_RDWR);

	gnutls_deinit(client);
	gnutls_deinit(server);

	gnutls_certificate_free_credentials(serverx509cred);
	gnutls_certificate_free_credentials(clientx509cred);
	gnutls_anon_free_server_credentials(s_anoncred);
	gnutls_anon_free_client_credentials(c_anoncred);
	gnutls_dh_params_deinit(dh_params);
}

void doit(void)
{
	global_init();

	dtls_try_with_key_mtu("DTLS 1.2 with cli-cert", "NONE:+VERS-DTLS1.0:+MAC-ALL:+KX-ALL:+CIPHER-ALL:+SIGN-ALL:+COMP-ALL:+CURVE-ALL", GNUTLS_KX_ECDHE_RSA, GNUTLS_SIGN_RSA_SHA256, GNUTLS_SIGN_RSA_SHA256,
		&server_repro_cert, &server_repro_key, &client_repro_cert, &client_repro_key, USE_CERT, 1452);

	gnutls_global_deinit();
}
