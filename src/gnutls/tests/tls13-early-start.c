/*
 * Copyright (C) 2015-2018 Red Hat, Inc.
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

/* This program tests support for early start in TLS1.3 handshake */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <gnutls/gnutls.h>
#include "utils.h"
#include "eagain-common.h"
#include <assert.h>

#define USE_CERT 1
#define ASK_CERT 2

const char *side;

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "%s|<%d>| %s", side, level, str);
}

#define try_ok(name, client_prio) \
	try_with_key(name, client_prio, \
		&server_ca3_localhost_cert, &server_ca3_key, NULL, NULL, 0)

#define MSG "hello there ppl"

static
void try_with_key_fail(const char *name, const char *client_prio,
			const gnutls_datum_t *serv_cert,
			const gnutls_datum_t *serv_key,
			const gnutls_datum_t *cli_cert,
			const gnutls_datum_t *cli_key,
			unsigned init_flags)
{
	int ret;
	char buffer[256];
	gnutls_certificate_credentials_t serverx509cred;
	gnutls_session_t server;
	int sret = GNUTLS_E_AGAIN;
	/* Client stuff. */
	gnutls_certificate_credentials_t clientx509cred;
	gnutls_session_t client;
	int cret = GNUTLS_E_AGAIN, version;
	const char *err;

	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(6);

	reset_buffers();
	/* Init server */
	gnutls_certificate_allocate_credentials(&serverx509cred);

	ret = gnutls_certificate_set_x509_key_mem(serverx509cred,
						  serv_cert, serv_key,
						  GNUTLS_X509_FMT_PEM);
	if (ret < 0)
		fail("Could not set key/cert: %s\n", gnutls_strerror(ret));

	assert(gnutls_init(&server, GNUTLS_SERVER|init_flags)>=0);
	gnutls_credentials_set(server, GNUTLS_CRD_CERTIFICATE,
			       serverx509cred);

	assert(gnutls_priority_set_direct(server, client_prio, NULL) >= 0);

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

	ret = gnutls_credentials_set(client, GNUTLS_CRD_CERTIFICATE,
				clientx509cred);
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

	assert(!(gnutls_session_get_flags(server) & GNUTLS_SFLAGS_EARLY_START));
	assert(!(gnutls_session_get_flags(client) & GNUTLS_SFLAGS_EARLY_START));

	version = gnutls_protocol_get_version(client);
	assert(version == GNUTLS_TLS1_3);

	memset(buffer, 0, sizeof(buffer));
	assert(gnutls_record_send(server, MSG, strlen(MSG))>=0);

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

	memset(buffer, 0, sizeof(buffer));
	assert(gnutls_record_send(client, MSG, strlen(MSG))>=0);

	ret = gnutls_record_recv(server, buffer, sizeof(buffer));
	if (ret == 0) {
		fail("server: Peer has closed the TLS connection\n");
	} else if (ret < 0) {
		fail("server: Error: %s\n", gnutls_strerror(ret));
	}

	if (ret != strlen(MSG) || memcmp(MSG, buffer, ret) != 0) {
		fail("client: Error in data received. Expected %d, got %d\n", (int)strlen(MSG), ret);
		exit(1);
	}

	gnutls_deinit(client);
	gnutls_deinit(server);

	gnutls_certificate_free_credentials(serverx509cred);
	gnutls_certificate_free_credentials(clientx509cred);
}

static
void try_with_key_ks(const char *name, const char *client_prio,
		const gnutls_datum_t *serv_cert,
		const gnutls_datum_t *serv_key,
		const gnutls_datum_t *client_cert,
		const gnutls_datum_t *client_key,
		unsigned cert_flags,
		unsigned init_flags)
{
	int ret;
	char buffer[256];
	/* Server stuff. */
	gnutls_certificate_credentials_t serverx509cred;
	gnutls_session_t server;
	int sret = GNUTLS_E_AGAIN;
	/* Client stuff. */
	gnutls_certificate_credentials_t clientx509cred;
	gnutls_session_t client;
	int cret = GNUTLS_E_AGAIN, version;
	const char *err;

	/* General init. */
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(6);

	reset_buffers();
	/* Init server */
	gnutls_certificate_allocate_credentials(&serverx509cred);

	ret = gnutls_certificate_set_x509_key_mem(serverx509cred,
					    serv_cert, serv_key,
					    GNUTLS_X509_FMT_PEM);
	if (ret < 0) {
		fail("Could not set key/cert: %s\n", gnutls_strerror(ret));
	}

	assert(gnutls_init(&server, GNUTLS_SERVER|init_flags)>=0);
	gnutls_credentials_set(server, GNUTLS_CRD_CERTIFICATE,
				serverx509cred);


	assert(gnutls_priority_set_direct(server,
				   "NORMAL:-VERS-ALL:+VERS-TLS1.3",
				   NULL)>=0);
	gnutls_transport_set_push_function(server, server_push);
	gnutls_transport_set_pull_function(server, server_pull);
	gnutls_transport_set_ptr(server, server);

	/* Init client */

	ret = gnutls_certificate_allocate_credentials(&clientx509cred);
	if (ret < 0)
		exit(1);

	if (cert_flags == USE_CERT) {
		gnutls_certificate_set_x509_key_mem(clientx509cred,
						    client_cert, client_key,
						    GNUTLS_X509_FMT_PEM);
		gnutls_certificate_server_set_request(server, GNUTLS_CERT_REQUIRE);
	} else if (cert_flags == ASK_CERT) {
		gnutls_certificate_server_set_request(server, GNUTLS_CERT_REQUEST);
	}

	ret = gnutls_init(&client, GNUTLS_CLIENT);
	if (ret < 0)
		exit(1);


	ret = gnutls_credentials_set(client, GNUTLS_CRD_CERTIFICATE,
				clientx509cred);
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

	assert(gnutls_session_get_flags(server) & GNUTLS_SFLAGS_EARLY_START);
	assert(!(gnutls_session_get_flags(client) & GNUTLS_SFLAGS_EARLY_START));

	version = gnutls_protocol_get_version(client);
	assert(version == GNUTLS_TLS1_3);

	memset(buffer, 0, sizeof(buffer));
	assert(gnutls_record_send(server, MSG, strlen(MSG))>=0);

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

	memset(buffer, 0, sizeof(buffer));
	assert(gnutls_record_send(client, MSG, strlen(MSG))>=0);

	ret = gnutls_record_recv(server, buffer, sizeof(buffer));
	if (ret == 0) {
		fail("server: Peer has closed the TLS connection\n");
	} else if (ret < 0) {
		fail("server: Error: %s\n", gnutls_strerror(ret));
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
}

static
void try_with_key(const char *name, const char *client_prio,
		const gnutls_datum_t *serv_cert,
		const gnutls_datum_t *serv_key,
		const gnutls_datum_t *cli_cert,
		const gnutls_datum_t *cli_key,
		unsigned cert_flags)
{
	return try_with_key_ks(name, client_prio,
			       serv_cert, serv_key, cli_cert, cli_key, cert_flags, GNUTLS_ENABLE_EARLY_START);
}

#include "cert-common.h"

void doit(void)
{
	/* TLS 1.3 no client cert: early start expected */
	try_ok("TLS 1.3 with ffdhe2048 rsa no-cli-cert", "NORMAL:-VERS-ALL:+VERS-TLS1.3:-GROUP-ALL:+GROUP-FFDHE2048");
	try_ok("TLS 1.3 with secp256r1 rsa no-cli-cert", "NORMAL:-VERS-ALL:+VERS-TLS1.3:-GROUP-ALL:+GROUP-SECP256R1");
	try_ok("TLS 1.3 with x25519 rsa no-cli-cert", "NORMAL:-VERS-ALL:+VERS-TLS1.3:-GROUP-ALL:+GROUP-X25519");

	try_with_key_ks("TLS 1.3 with secp256r1 ecdsa no-cli-cert", "NORMAL:-VERS-ALL:+VERS-TLS1.3:-GROUP-ALL:+GROUP-SECP256R1",
		&server_ca3_localhost_ecc_cert, &server_ca3_ecc_key, NULL, NULL, 0, GNUTLS_ENABLE_EARLY_START);

	/* client authentication: no early start possible */
	try_with_key_fail("TLS 1.3 with rsa-pss cli-cert", "NORMAL:-VERS-ALL:+VERS-TLS1.3:-KX-ALL:+ECDHE-RSA",
		&server_ca3_localhost_ecc_cert, &server_ca3_ecc_key, &cli_ca3_rsa_pss_cert, &cli_ca3_rsa_pss_key, GNUTLS_ENABLE_EARLY_START);
	try_with_key_fail("TLS 1.3 with rsa cli-cert", "NORMAL:-VERS-ALL:+VERS-TLS1.3:-KX-ALL:+ECDHE-RSA",
		&server_ca3_localhost_ecc_cert, &server_ca3_ecc_key, &cli_ca3_cert, &cli_ca3_key, GNUTLS_ENABLE_EARLY_START);
	try_with_key_fail("TLS 1.3 with ecdsa cli-cert", "NORMAL:-VERS-ALL:+VERS-TLS1.3:-KX-ALL:+ECDHE-RSA",
		&server_ca3_localhost_ecc_cert, &server_ca3_ecc_key, &server_ca3_localhost_ecc_cert, &server_ca3_ecc_key, GNUTLS_ENABLE_EARLY_START);

	/* TLS 1.3 no client cert: no early start flag specified */
	try_with_key_fail("TLS 1.3 with rsa-pss cli-cert", "NORMAL:-VERS-ALL:+VERS-TLS1.3:-KX-ALL:+ECDHE-RSA",
		&server_ca3_localhost_ecc_cert, &server_ca3_ecc_key, NULL, NULL, 0);
}
