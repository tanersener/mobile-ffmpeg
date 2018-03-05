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
#include <gnutls/gnutls.h>
#include "utils.h"
#include "eagain-common.h"

#ifndef ENABLE_SRP

void doit(void)
{
	exit(77);
}

#else

/* This test checks whether the server switching certificates is detected
 * by the client */

const char *side;

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "%s|<%d>| %s", side, level, str);
}

#include "cert-common.h"


#define VERIF_TEST1 "CEqjUZBlkQCocfOR0E4AsPZKOFYPGjKFMHW7KDcnpE4sH4.iGMbkygb/bViRNjskF9/TQdD46Mvlt6pLs5MZoTn8mO3G.RGyXdWuIrhnVn29p41Cpc5RhTLaeUm3asW6LF60VTKnGERC0eB37xZUsaTpzmaTNdD4mOoYCN3bD9Y"
#define VERIF_TEST2 "EEbMk8afwXz/0oV5Yo9To7V6c6xkYid8meqEByxM33XjM4xeKUjeN7Ft2.xvjo4S6Js7mEs9Ov.uZtBp3ugCAbvl6G7bdfYF6z.tAD4mNYhH7iI7SwQy.ntmbJ3uJ1qB5MHW7ajSdWvA7l3SSsyyAVMe9HVQcxZKJRf4mzwm06s"

#define SALT_TEST1 "3a3xX3Myzb9YJn5X0R7sbx"
#define SALT_TEST2 "25J9FArvl1ZDrTSFsvZ4Jb"

#define PRIME "Ewl2hcjiutMd3Fu2lgFnUXWSc67TVyy2vwYCKoS9MLsrdJVT9RgWTCuEqWJrfB6uE3LsE9GkOlaZabS7M29sj5TnzUqOLJMjiwEzArfiLr9WbMRANlF68N5AVLcPWvNx6Zjl3m5Scp0BzJBz9TkgfhzKJZ.WtP3Mv/67I/0wmRZ"
gnutls_datum_t tprime = 
{
	.data = (void*)PRIME,
	.size = sizeof(PRIME)-1
};

gnutls_datum_t test1_verif = 
{
	.data = (void*)VERIF_TEST1,
	.size = sizeof(VERIF_TEST1)-1
};

gnutls_datum_t test2_verif = 
{
	.data = (void*)VERIF_TEST2,
	.size = sizeof(VERIF_TEST2)-1
};

gnutls_datum_t test1_salt = 
{
	.data = (void*)SALT_TEST1,
	.size = sizeof(SALT_TEST1)-1
};

gnutls_datum_t test2_salt = 
{
	.data = (void*)SALT_TEST2,
	.size = sizeof(SALT_TEST2)-1
};

static int
srpfunc(gnutls_session_t session, const char *username,
	gnutls_datum_t *salt, gnutls_datum_t *verifier, gnutls_datum_t *generator, gnutls_datum_t *prime)
{
	int ret;
	if (debug)
		printf("srp: username %s\n", username);

	generator->data = gnutls_malloc(1);
	generator->data[0] = 2;
	generator->size = 1;

	ret = gnutls_srp_base64_decode2(&tprime, prime);
	if (ret < 0)
		fail("error in gnutls_srp_base64_decode2 -prime\n");

	if (strcmp(username, "test1") == 0) {
		ret = gnutls_srp_base64_decode2(&test1_verif, verifier);
		if (ret < 0)
			fail("error in gnutls_srp_base64_decode2 -verif\n");


		ret = gnutls_srp_base64_decode2(&test1_salt, salt);
		if (ret < 0)
			fail("error in gnutls_srp_base64_decode2 -salt\n");
	} else if (strcmp(username, "test2") == 0) {
		ret = gnutls_srp_base64_decode2(&test2_verif, verifier);
		if (ret < 0)
			fail("error in gnutls_srp_base64_decode2 -verif\n");

		ret = gnutls_srp_base64_decode2(&test2_salt, salt);
		if (ret < 0)
			fail("error in gnutls_srp_base64_decode2 -salt\n");
	} else {
		fail("Unknown username %s\n", username);
	}

	return 0;
}

static void try(const char *prio, gnutls_kx_algorithm_t kx, unsigned allow_change)
{
	int ret;
	/* Server stuff. */
	gnutls_srp_server_credentials_t server_srp_cred;
	gnutls_certificate_credentials_t server_x509_cred;
	gnutls_dh_params_t dh_params;
	const gnutls_datum_t p3 =
	    { (unsigned char *) pkcs3, strlen(pkcs3) };
	gnutls_session_t server;
	int sret = GNUTLS_E_AGAIN;
	/* Client stuff. */
	gnutls_certificate_credentials_t client_x509_cred;
	gnutls_srp_client_credentials_t client_srp_cred;
	gnutls_srp_client_credentials_t client_srp_cred2;
	gnutls_session_t client;
	int cret = GNUTLS_E_AGAIN;

	/* General init. */
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(6);

	/* Init server */
	gnutls_srp_allocate_server_credentials(&server_srp_cred);

	gnutls_certificate_allocate_credentials(&server_x509_cred);
	gnutls_certificate_set_x509_key_mem(server_x509_cred,
					    &server_cert, &server_key,
					    GNUTLS_X509_FMT_PEM);

	gnutls_dh_params_init(&dh_params);
	gnutls_dh_params_import_pkcs3(dh_params, &p3, GNUTLS_X509_FMT_PEM);
	gnutls_certificate_set_dh_params(server_x509_cred, dh_params);

	gnutls_srp_set_server_credentials_function(server_srp_cred, srpfunc);

	if (allow_change)
		gnutls_init(&server, GNUTLS_SERVER|GNUTLS_ALLOW_ID_CHANGE);
	else
		gnutls_init(&server, GNUTLS_SERVER);
	gnutls_credentials_set(server, GNUTLS_CRD_SRP,
				server_srp_cred);
	gnutls_credentials_set(server, GNUTLS_CRD_CERTIFICATE,
				server_x509_cred);

	gnutls_priority_set_direct(server,
				   prio,
				   NULL);
	gnutls_transport_set_push_function(server, server_push);
	gnutls_transport_set_pull_function(server, server_pull);
	gnutls_transport_set_ptr(server, server);

	/* Init client */

	gnutls_srp_allocate_client_credentials(&client_srp_cred);
	gnutls_srp_allocate_client_credentials(&client_srp_cred2);

	gnutls_srp_set_client_credentials(client_srp_cred, "test1", "test");
	gnutls_srp_set_client_credentials(client_srp_cred2, "test2", "test");

	ret = gnutls_init(&client, GNUTLS_CLIENT);
	if (ret < 0)
		exit(1);

	ret = gnutls_credentials_set(client, GNUTLS_CRD_SRP, client_srp_cred);
	if (ret < 0)
		exit(1);

	ret = gnutls_certificate_allocate_credentials(&client_x509_cred);
	if (ret < 0)
		exit(1);

	ret = gnutls_certificate_set_x509_trust_mem(client_x509_cred, &ca_cert, GNUTLS_X509_FMT_PEM);
	if (ret < 0)
		exit(1);

	ret = gnutls_credentials_set(client, GNUTLS_CRD_CERTIFICATE,
				client_x509_cred);
	if (ret < 0)
		exit(1);

	ret = gnutls_priority_set_direct(client, prio, NULL);
	if (ret < 0)
		exit(1);

	gnutls_transport_set_push_function(client, client_push);
	gnutls_transport_set_pull_function(client, client_pull);
	gnutls_transport_set_ptr(client, client);

	HANDSHAKE(client, server);

	if (gnutls_kx_get(client) != kx) {
		fail("got unexpected key exchange algorithm: %s (expected %s)\n",
			gnutls_kx_get_name(gnutls_kx_get(client)),
			gnutls_kx_get_name(kx));
		exit(1);
	}

	/* switch client's username and rehandshake */
	ret = gnutls_credentials_set(client, GNUTLS_CRD_SRP, client_srp_cred2);
	if (ret < 0)
		exit(1);

	if (allow_change) {
		HANDSHAKE(client, server);
	} else {
		HANDSHAKE_EXPECT(client, server, GNUTLS_E_AGAIN, GNUTLS_E_SESSION_USER_ID_CHANGED);
	}

	gnutls_deinit(client);
	gnutls_deinit(server);

	gnutls_certificate_free_credentials(server_x509_cred);
	gnutls_srp_free_server_credentials(server_srp_cred);

	gnutls_certificate_free_credentials(client_x509_cred);
	gnutls_srp_free_client_credentials(client_srp_cred);
	gnutls_srp_free_client_credentials(client_srp_cred2);
	gnutls_dh_params_deinit(dh_params);
}

void doit(void)
{
	global_init();
	/* Allow change of ID */
	try("NORMAL:-KX-ALL:+SRP", GNUTLS_KX_SRP, 0);
	reset_buffers();
	try("NORMAL:-KX-ALL:+SRP-RSA", GNUTLS_KX_SRP_RSA, 0);
	reset_buffers();

	/* Prohibit (default) change of ID */
	try("NORMAL:-KX-ALL:+SRP", GNUTLS_KX_SRP, 1);
	reset_buffers();
	try("NORMAL:-KX-ALL:+SRP-RSA", GNUTLS_KX_SRP_RSA, 1);
	reset_buffers();
	gnutls_global_deinit();
}

#endif
