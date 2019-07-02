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
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>
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
#include "utils.h"
#include "eagain-common.h"
#include "cert-common.h"

#define USE_CERT 1
#define ASK_CERT 2

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "%s|<%d>| %s", side, level, str);
}

static
void try_with_key(const char *name,
		const char *server_prio,
		const char *client_prio,
		const gnutls_datum_t *serv_cert,
		const gnutls_datum_t *serv_key,
		const gnutls_datum_t *client_cert,
		const gnutls_datum_t *client_key,
		unsigned cert_flags,
		int exp_error_server,
		int exp_error_client)
{
	int ret;
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
	int cret = GNUTLS_E_AGAIN;
	const char *err;

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

	gnutls_init(&server, GNUTLS_SERVER);
	gnutls_credentials_set(server, GNUTLS_CRD_CERTIFICATE,
				serverx509cred);
	gnutls_credentials_set(server, GNUTLS_CRD_ANON, s_anoncred);

	ret = gnutls_priority_set_direct(server, server_prio, &err);
	if (ret < 0) {
		if (ret == GNUTLS_E_INVALID_REQUEST)
			fprintf(stderr, "Error in server: %s\n", err);
		exit(1);
	}

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

#if 0
	ret = gnutls_certificate_set_x509_trust_mem(clientx509cred, &ca_cert, GNUTLS_X509_FMT_PEM);
	if (ret < 0)
		exit(1);
#endif
	ret = gnutls_init(&client, GNUTLS_CLIENT);
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
	gnutls_transport_set_ptr(client, client);

	ret = gnutls_priority_set_direct(client, client_prio, &err);
	if (ret < 0) {
		if (ret == GNUTLS_E_INVALID_REQUEST)
			fprintf(stderr, "Error in %s\n", err);
		exit(1);
	}
	success("negotiating %s\n", name);
	HANDSHAKE_EXPECT(client, server, exp_error_client, exp_error_server);

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

	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(6);

	/* check compatibility and handling of SIGN-ECDSA-SECP256R1-SHA256 which
	 * is available under TLS1.3 but not TLS1.2 */
	try_with_key("TLS 1.2 with ecdhe ecdsa with ECDSA-SECP256R1-SHA256",
		NULL, "NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+ECDHE-ECDSA:-SIGN-ALL:+SIGN-ECDSA-SECP256R1-SHA256:+SIGN-ECDSA-SECP384R1-SHA384:+SIGN-ECDSA-SECP521R1-SHA512:+SIGN-RSA-SHA256",
		&server_ca3_localhost_ecc_cert, &server_ca3_ecc_key, NULL, NULL, 0, GNUTLS_E_NO_CIPHER_SUITES, GNUTLS_E_AGAIN);

	try_with_key("TLS 1.2 with ecdhe ecdsa with ECDSA-SHA256",
		NULL, "NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+ECDHE-ECDSA:-SIGN-ALL:+SIGN-ECDSA-SHA256:+SIGN-RSA-SHA256",
		&server_ca3_localhost_ecc_cert, &server_ca3_ecc_key, NULL, NULL, 0, 0, 0);

	gnutls_global_deinit();
}
