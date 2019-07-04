/*
 * Copyright (C) 2016 Red Hat, Inc.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <gnutls/gnutls.h>
#include "utils.h"
#include "eagain-common.h"

/* This test checks whether the client switching certificates is detected
 * by the server */

const char *side;

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "%s|<%d>| %s", side, level, str);
}

#include "cert-common.h"

#define SENT 0
#define NOT_SENT 1

enum {
	INCORRECT_CA_FORCE = 0,
	INCORRECT_CA = 1,
	CORRECT_CA = 2,
	NO_CA = 3
};

static void try(const char *prio, unsigned expect, unsigned ca_type)
{
	int ret;
	/* Server stuff. */
	gnutls_certificate_credentials_t serverx509cred;
	gnutls_dh_params_t dh_params;
	const gnutls_datum_t p3 =
	    { (unsigned char *) pkcs3, strlen(pkcs3) };
	gnutls_session_t server;
	int sret = GNUTLS_E_AGAIN;
	/* Client stuff. */
	gnutls_certificate_credentials_t clientx509cred;
	unsigned flags = 0;
	gnutls_session_t client;
	int cret = GNUTLS_E_AGAIN;

	/* General init. */
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(6);

	/* Init server */
	gnutls_certificate_allocate_credentials(&serverx509cred);
	gnutls_certificate_set_x509_key_mem(serverx509cred,
					    &server_ca3_cert_chain, &server_ca3_key,
					    GNUTLS_X509_FMT_PEM);

	gnutls_dh_params_init(&dh_params);
	gnutls_dh_params_import_pkcs3(dh_params, &p3, GNUTLS_X509_FMT_PEM);
	gnutls_certificate_set_dh_params(serverx509cred, dh_params);

	gnutls_init(&server, GNUTLS_SERVER);
	gnutls_certificate_server_set_request(server, GNUTLS_CERT_REQUEST);

	if (ca_type == CORRECT_CA) {
		ret = gnutls_certificate_set_x509_trust_mem(serverx509cred, &ca3_cert, GNUTLS_X509_FMT_PEM);
	} else if (ca_type == INCORRECT_CA || ca_type == INCORRECT_CA_FORCE) {
		ret = gnutls_certificate_set_x509_trust_mem(serverx509cred, &unknown_ca_cert, GNUTLS_X509_FMT_PEM);
	} else if (ca_type == NO_CA) {
		ret = 0;
	} else {
		abort();
	}
		ret = 0;
	if (ret < 0)
		exit(1);


	gnutls_credentials_set(server, GNUTLS_CRD_CERTIFICATE,
				serverx509cred);

	assert(gnutls_priority_set_direct(server, prio, NULL) >= 0);
	gnutls_transport_set_push_function(server, server_push);
	gnutls_transport_set_pull_function(server, server_pull);
	gnutls_transport_set_ptr(server, server);

	/* Init client */

	ret = gnutls_certificate_allocate_credentials(&clientx509cred);
	if (ret < 0)
		exit(1);

	ret = gnutls_certificate_set_x509_key_mem(clientx509cred,
						  &cli_ca3_cert_chain, &cli_ca3_key,
						  GNUTLS_X509_FMT_PEM);
	if (ret < 0)
		exit(1);


	ret = gnutls_certificate_set_x509_trust_mem(clientx509cred, &ca3_cert, GNUTLS_X509_FMT_PEM);
	if (ret < 0)
		exit(1);

	if (ca_type == INCORRECT_CA_FORCE) {
		flags |= GNUTLS_FORCE_CLIENT_CERT;
	}

	ret = gnutls_init(&client, GNUTLS_CLIENT|flags);
	if (ret < 0)
		exit(1);

	ret = gnutls_credentials_set(client, GNUTLS_CRD_CERTIFICATE,
				clientx509cred);
	if (ret < 0)
		exit(1);

	ret = gnutls_priority_set_direct(client, prio, NULL);
	if (ret < 0)
		exit(1);

	gnutls_transport_set_push_function(client, client_push);
	gnutls_transport_set_pull_function(client, client_pull);
	gnutls_transport_set_ptr(client, client);

	success("Testing CA type %d, expecting %s\n", ca_type, expect==SENT?"sent":"not sent");

	HANDSHAKE(client, server);

	if (expect == SENT) {
		if (gnutls_certificate_get_ours(client) == NULL) {
			fail("Test %d: client didn't send any certificate\n", ca_type);
			exit(1);
		}
	} else {
		if (gnutls_certificate_get_ours(client) != NULL) {
			fail("Test %d: client sent a certificate, although not expected\n", ca_type);
			exit(1);
		}
	}

	gnutls_deinit(client);
	gnutls_deinit(server);

	gnutls_certificate_free_credentials(serverx509cred);
	gnutls_certificate_free_credentials(clientx509cred);
	gnutls_dh_params_deinit(dh_params);
}

static void start(const char *prio)
{
	global_init();

	success("trying %s\n", prio);

	try(prio, SENT, NO_CA);
	try(prio, SENT, CORRECT_CA);
	try(prio, NOT_SENT, INCORRECT_CA);
	try(prio, SENT, INCORRECT_CA_FORCE);
	gnutls_global_deinit();
}

void doit(void)
{
	start("NORMAL:-VERS-ALL:+VERS-TLS1.2");
	start("NORMAL:-VERS-ALL:+VERS-TLS1.3");
	start("NORMAL");
}
