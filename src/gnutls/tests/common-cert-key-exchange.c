/*
 * Copyright (C) 2015-2016 Red Hat, Inc.
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
#include <assert.h>

#include "eagain-common.h"
#include "common-cert-key-exchange.h"

const char *side;
const char *server_priority = NULL;

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "%s|<%d>| %s", side, level, str);
}

#define MSG "hello there ppl"

void try_with_key_fail(const char *name, const char *client_prio,
			int server_err, int client_err,
			const gnutls_datum_t *serv_cert,
			const gnutls_datum_t *serv_key,
			const gnutls_datum_t *cli_cert,
			const gnutls_datum_t *cli_key)
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
	const char *err;

	/* General init. */
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(6);

	reset_buffers();
	/* Init server */
	assert(gnutls_certificate_allocate_credentials(&serverx509cred)>=0);

	ret = gnutls_certificate_set_x509_key_mem(serverx509cred,
						  serv_cert, serv_key,
						  GNUTLS_X509_FMT_PEM);
	if (ret < 0)
		fail("Could not set key/cert: %s\n", gnutls_strerror(ret));

	assert(gnutls_init(&server, GNUTLS_SERVER)>=0);
	if (server_priority)
		assert(gnutls_priority_set_direct(server, server_priority, NULL) >= 0);
	else
		assert(gnutls_priority_set_direct(server, client_prio, NULL) >= 0);

	assert(gnutls_credentials_set(server, GNUTLS_CRD_CERTIFICATE,
				      serverx509cred)>=0);

	gnutls_transport_set_push_function(server, server_push);
	gnutls_transport_set_pull_function(server, server_pull);
	gnutls_transport_set_ptr(server, server);

	/* Init client */
	ret = gnutls_certificate_allocate_credentials(&clientx509cred);
	if (ret < 0)
		exit(1);

	if (cli_cert) {
		gnutls_certificate_set_x509_key_mem(clientx509cred,
						    cli_cert, cli_key,
						    GNUTLS_X509_FMT_PEM);
		gnutls_certificate_server_set_request(server, GNUTLS_CERT_REQUIRE);
	}

	ret = gnutls_init(&client, GNUTLS_CLIENT);
	if (ret < 0)
		exit(1);

	gnutls_transport_set_push_function(client, client_push);
	gnutls_transport_set_pull_function(client, client_pull);
	gnutls_transport_set_ptr(client, client);

	ret = gnutls_priority_set_direct(client, client_prio, &err);
	if (ret < 0) {
		if (ret == GNUTLS_E_INVALID_REQUEST)
			fprintf(stderr, "Error in %s\n", err);
		exit(1);
	}

	ret = gnutls_credentials_set(client, GNUTLS_CRD_CERTIFICATE,
				clientx509cred);
	if (ret < 0)
		exit(1);

	success("negotiating %s\n", name);
	HANDSHAKE_EXPECT(client, server, client_err, server_err);

	gnutls_deinit(client);
	gnutls_deinit(server);

	gnutls_certificate_free_credentials(serverx509cred);
	gnutls_certificate_free_credentials(clientx509cred);
}

void try_with_rawpk_key_fail(const char *name, const char *client_prio,
			     int server_err, int client_err,
			     const gnutls_datum_t *serv_cert,
			     const gnutls_datum_t *serv_key,
			     unsigned server_ku,
			     const gnutls_datum_t *cli_cert,
			     const gnutls_datum_t *cli_key,
			     unsigned client_ku)
{
	int ret;
	/* Server stuff. */
	gnutls_certificate_credentials_t server_cred;
	gnutls_session_t server;
	int sret = GNUTLS_E_AGAIN;
	/* Client stuff. */
	gnutls_certificate_credentials_t client_cred;
	gnutls_session_t client;
	int cret = GNUTLS_E_AGAIN;
	const char *err;

	/* General init. */
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(6);

	reset_buffers();
	/* Init server */
	gnutls_certificate_allocate_credentials(&server_cred);

	ret = gnutls_certificate_set_rawpk_key_mem(server_cred,
						   serv_cert, serv_key, GNUTLS_X509_FMT_PEM, NULL, server_ku,
						   NULL, 0, 0);
	if (ret < 0)
		fail("Could not set key/cert: %s\n", gnutls_strerror(ret));

	assert(gnutls_init(&server, GNUTLS_SERVER | GNUTLS_ENABLE_RAWPK) >= 0);
	if (server_priority)
		assert(gnutls_priority_set_direct(server, server_priority, NULL) >= 0);
	else
		assert(gnutls_priority_set_direct(server, client_prio, NULL) >= 0);

	gnutls_credentials_set(server, GNUTLS_CRD_CERTIFICATE,
			       server_cred);

	gnutls_transport_set_push_function(server, server_push);
	gnutls_transport_set_pull_function(server, server_pull);
	gnutls_transport_set_ptr(server, server);

	/* Init client */
	ret = gnutls_certificate_allocate_credentials(&client_cred);
	if (ret < 0)
		exit(1);

	if (cli_cert) {
		ret = gnutls_certificate_set_rawpk_key_mem(client_cred,
							   cli_cert, cli_key, GNUTLS_X509_FMT_PEM, NULL, client_ku,
							   NULL, 0, 0);
		if (ret < 0)
			fail("Could not set key/cert: %s\n", gnutls_strerror(ret));
		gnutls_certificate_server_set_request(server, GNUTLS_CERT_REQUIRE);
	}

	ret = gnutls_init(&client, GNUTLS_CLIENT|GNUTLS_ENABLE_RAWPK);
	if (ret < 0)
		exit(1);

	gnutls_transport_set_push_function(client, client_push);
	gnutls_transport_set_pull_function(client, client_pull);
	gnutls_transport_set_ptr(client, client);

	ret = gnutls_priority_set_direct(client, client_prio, &err);
	if (ret < 0) {
		if (ret == GNUTLS_E_INVALID_REQUEST)
			fprintf(stderr, "Error in %s\n", err);
		exit(1);
	}

	ret = gnutls_credentials_set(client, GNUTLS_CRD_CERTIFICATE,
				client_cred);
	if (ret < 0)
		exit(1);

	success("negotiating %s\n", name);
	HANDSHAKE_EXPECT(client, server, client_err, server_err);

	gnutls_deinit(client);
	gnutls_deinit(server);

	gnutls_certificate_free_credentials(server_cred);
	gnutls_certificate_free_credentials(client_cred);
}

void try_with_key_ks(const char *name, const char *client_prio, gnutls_kx_algorithm_t client_kx,
		gnutls_sign_algorithm_t server_sign_algo,
		gnutls_sign_algorithm_t client_sign_algo,
		const gnutls_datum_t *serv_cert,
		const gnutls_datum_t *serv_key,
		const gnutls_datum_t *client_cert,
		const gnutls_datum_t *client_key,
		unsigned cert_flags,
		unsigned exp_group,
		gnutls_certificate_type_t server_ctype,
		gnutls_certificate_type_t client_ctype)
{
	int ret;
	char buffer[256];
	/* Server stuff. */
	gnutls_certificate_credentials_t server_cred;
	gnutls_anon_server_credentials_t s_anoncred;
	gnutls_dh_params_t dh_params;
	const gnutls_datum_t p3 =
	    { (unsigned char *) pkcs3, strlen(pkcs3) };
	gnutls_session_t server;
	int sret = GNUTLS_E_AGAIN;
	/* Client stuff. */
	gnutls_certificate_credentials_t client_cred;
	gnutls_anon_client_credentials_t c_anoncred;
	gnutls_session_t client;
	int cret = GNUTLS_E_AGAIN, version;
	const char *err;

	/* General init. */
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(6);

	reset_buffers();
	/* Init server */
	assert(gnutls_anon_allocate_server_credentials(&s_anoncred)>=0);
	assert(gnutls_certificate_allocate_credentials(&server_cred)>=0);

	// Set server crt creds based on ctype
	switch (server_ctype) {
		case GNUTLS_CRT_X509:
			ret = gnutls_certificate_set_x509_key_mem(server_cred,
									serv_cert, serv_key,
									GNUTLS_X509_FMT_PEM);
			break;
		case GNUTLS_CRT_RAWPK:
			ret = gnutls_certificate_set_rawpk_key_mem(server_cred,
									serv_cert, serv_key, GNUTLS_X509_FMT_PEM, NULL, 0,
									NULL, 0, 0);
			break;
		default:
			ret = GNUTLS_E_UNSUPPORTED_CERTIFICATE_TYPE;
	}

	if (ret < 0) {
		fail("Could not set key/cert: %s\n", gnutls_strerror(ret));
	}

	gnutls_dh_params_init(&dh_params);
	gnutls_dh_params_import_pkcs3(dh_params, &p3, GNUTLS_X509_FMT_PEM);
	gnutls_certificate_set_dh_params(server_cred, dh_params);
	gnutls_anon_set_server_dh_params(s_anoncred, dh_params);

	assert(gnutls_init(&server, GNUTLS_SERVER | GNUTLS_ENABLE_RAWPK)>=0);
	assert(gnutls_credentials_set(server, GNUTLS_CRD_CERTIFICATE,
				      server_cred)>=0);
	assert(gnutls_credentials_set(server, GNUTLS_CRD_ANON, s_anoncred)>=0);

	if (server_priority)
		assert(gnutls_priority_set_direct(server, server_priority, NULL) >= 0);
	else
		assert(gnutls_priority_set_direct(server,
				   "NORMAL:+VERS-SSL3.0:+ANON-ECDH:+ANON-DH:+ECDHE-RSA:+DHE-RSA:+RSA:+ECDHE-ECDSA:+CURVE-X25519:+SIGN-EDDSA-ED25519:+CTYPE-ALL",
				   NULL)>=0);
	gnutls_transport_set_push_function(server, server_push);
	gnutls_transport_set_pull_function(server, server_pull);
	gnutls_transport_set_ptr(server, server);

	/* Init client */
	ret = gnutls_certificate_allocate_credentials(&client_cred);
	if (ret < 0)
		exit(1);

	if (cert_flags == USE_CERT) {
		// Set client crt creds based on ctype
		switch (client_ctype) {
			case GNUTLS_CRT_X509:
				gnutls_certificate_set_x509_key_mem(client_cred,
										client_cert, client_key,
										GNUTLS_X509_FMT_PEM);
				break;
			case GNUTLS_CRT_RAWPK:
				gnutls_certificate_set_rawpk_key_mem(client_cred,
									client_cert, client_key, GNUTLS_X509_FMT_PEM, NULL, 0,
									NULL, 0, 0);
				break;
			default:
				fail("Illegal client certificate type given\n");
		}

		gnutls_certificate_server_set_request(server, GNUTLS_CERT_REQUIRE);
	} else if (cert_flags == ASK_CERT) {
		gnutls_certificate_server_set_request(server, GNUTLS_CERT_REQUEST);
	}

#if 0
	ret = gnutls_certificate_set_x509_trust_mem(client_cred, &ca_cert, GNUTLS_X509_FMT_PEM);
	if (ret < 0)
		exit(1);
#endif
	ret = gnutls_init(&client, GNUTLS_CLIENT | GNUTLS_ENABLE_RAWPK);
	if (ret < 0)
		exit(1);


	assert(gnutls_anon_allocate_client_credentials(&c_anoncred)>=0);
	assert(gnutls_credentials_set(client, GNUTLS_CRD_ANON, c_anoncred)>=0);
	ret = gnutls_credentials_set(client, GNUTLS_CRD_CERTIFICATE,
				client_cred);
	if (ret < 0)
		exit(1);

	gnutls_transport_set_push_function(client, client_push);
	gnutls_transport_set_pull_function(client, client_pull);
	gnutls_transport_set_ptr(client, client);

	ret = gnutls_priority_set_direct(client, client_prio, &err);
	if (ret < 0) {
		if (ret == GNUTLS_E_INVALID_REQUEST)
			fprintf(stderr, "Error in %s\n", err);
		exit(1);
	}
	success("negotiating %s\n", name);
	HANDSHAKE(client, server);

	if (gnutls_kx_get(client) != client_kx) {
		fail("%s: got unexpected key exchange algorithm: %s (expected %s)\n", name, gnutls_kx_get_name(gnutls_kx_get(client)),
			gnutls_kx_get_name(client_kx));
		exit(1);
	}

	/* test signature algorithm match */
	version = gnutls_protocol_get_version(client);
	if (version >= GNUTLS_TLS1_2) {
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

	if (exp_group != 0) {
		ret = gnutls_group_get(server);
		if (ret != (int)exp_group) {
			fail("%s: got unexpected server group: %d/%s\n", name, ret, gnutls_group_get_name(ret));
		}

		ret = gnutls_group_get(client);
		if (ret != (int)exp_group) {
			fail("%s: got unexpected client group: %d/%s\n", name, ret, gnutls_group_get_name(ret));
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

	gnutls_certificate_free_credentials(server_cred);
	gnutls_certificate_free_credentials(client_cred);
	gnutls_anon_free_server_credentials(s_anoncred);
	gnutls_anon_free_client_credentials(c_anoncred);
	gnutls_dh_params_deinit(dh_params);
}

void dtls_try_with_key_mtu(const char *name, const char *client_prio, gnutls_kx_algorithm_t client_kx,
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

	assert(gnutls_init(&server, GNUTLS_SERVER|GNUTLS_DATAGRAM|GNUTLS_NONBLOCK)>=0);
	assert(gnutls_credentials_set(server, GNUTLS_CRD_CERTIFICATE,
				      serverx509cred)>=0);
	assert(gnutls_credentials_set(server, GNUTLS_CRD_ANON, s_anoncred)>=0);

	assert(gnutls_priority_set_direct(server,
					  "NORMAL:+ANON-ECDH:+ANON-DH:+ECDHE-RSA:+DHE-RSA:+RSA:+ECDHE-ECDSA:+CURVE-X25519",
					   NULL)>=0);
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

	assert(gnutls_anon_allocate_client_credentials(&c_anoncred)>=0);
	assert(gnutls_credentials_set(client, GNUTLS_CRD_ANON, c_anoncred)>=0);
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

