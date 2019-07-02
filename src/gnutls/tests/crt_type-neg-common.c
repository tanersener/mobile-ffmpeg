/*
 * Copyright (C) 2017 - 2018 ARPA2 project
 *
 * Author: Tom Vrancken (dev@tomvrancken.nl)
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>
 */
#include <assert.h>
#include <gnutls/gnutls.h>
#include <gnutls/abstract.h>
#include <stdbool.h>

// Credential type flags
#define CRED_EMPTY 1<<0
#define CRED_X509 1<<1
#define CRED_RAWPK 1<<2

// Test case definition
typedef struct test_case_st {
	const char *name;
	const char *client_prio;
	const char *server_prio;
	const char set_cli_creds;
	const char set_srv_creds;
	gnutls_certificate_type_t expected_cli_ctype;
	gnutls_certificate_type_t expected_srv_ctype;
	int client_err;
	int server_err;
	unsigned int init_flags_cli;
	unsigned int init_flags_srv;
	bool request_cli_crt;
	bool cli_srv_may_diverge;
	// only needed when may_diverge is true
	gnutls_certificate_type_t expected_cli_cli_ctype; // negotiated cli ctype on the client
	gnutls_certificate_type_t expected_srv_cli_ctype; // negotiated cli ctype on the server
	gnutls_certificate_type_t expected_cli_srv_ctype; // negotiated srv ctype on the client
	gnutls_certificate_type_t expected_srv_srv_ctype; // negotiated srv ctype on the server
} test_case_st;


static void try(test_case_st * test)
{
	int sret, cret;		// Needed for HANDSHAKE macro
	/* To hold negotiated certificate types */
	gnutls_certificate_type_t srv_srv_ctype, srv_cli_ctype;
	gnutls_certificate_type_t cli_srv_ctype, cli_cli_ctype;
	/* To hold certificate credentials */
	gnutls_certificate_credentials_t client_creds = NULL;
	gnutls_certificate_credentials_t server_creds = NULL;

	gnutls_session_t server, client;

	// Initialize creds
	assert(gnutls_certificate_allocate_credentials(&client_creds) >= 0);
	assert(gnutls_certificate_allocate_credentials(&server_creds) >= 0);

	// Print test
	success("Running %s...\n", test->name);

	// Init client/server
	if(test->init_flags_cli) {
		assert(gnutls_init(&client, GNUTLS_CLIENT | test->init_flags_cli) >= 0);
	} else {
		assert(gnutls_init(&client, GNUTLS_CLIENT) >= 0);
	}

	if (test->init_flags_srv) {
		assert(gnutls_init(&server, GNUTLS_SERVER | test->init_flags_srv) >= 0);
	} else {
		assert(gnutls_init(&server, GNUTLS_SERVER) >= 0);
	}

	/* Set up our credentials for this handshake */
	// Test for using empty cli credentials
	if (test->set_cli_creds == CRED_EMPTY) {
		gnutls_credentials_set(client, GNUTLS_CRD_CERTIFICATE, client_creds);
	} else {
		// Test for using X509 cli credentials
		if (test->set_cli_creds & CRED_X509) {
			assert(gnutls_certificate_set_x509_key_mem
						 (client_creds, &cli_ca3_cert,	&cli_ca3_key, GNUTLS_X509_FMT_PEM) >= 0);
		}

		// Test for using RawPubKey cli credentials
		if (test->set_cli_creds & CRED_RAWPK) {
			assert(gnutls_certificate_set_rawpk_key_mem(client_creds,
				&rawpk_public_key1, &rawpk_private_key1, GNUTLS_X509_FMT_PEM,
				NULL, 0, NULL, 0, 0) >= 0);
		}

		// -- Add extra ctype creds here in the future --

		// Finally set the credentials
		gnutls_credentials_set(client, GNUTLS_CRD_CERTIFICATE, client_creds);
	}

	// Test for using empty srv credentials
	if (test->set_srv_creds == CRED_EMPTY) {
		gnutls_credentials_set(server, GNUTLS_CRD_CERTIFICATE, server_creds);
	} else {
		// Test for using X509 srv credentials
		if (test->set_srv_creds & CRED_X509) {
			assert(gnutls_certificate_set_x509_key_mem
						 (server_creds, &server_ca3_localhost_rsa_decrypt_cert,
				&server_ca3_key, GNUTLS_X509_FMT_PEM) >= 0);
			assert(gnutls_certificate_set_x509_key_mem
						 (server_creds, &server_ca3_localhost_ecc_cert,
				&server_ca3_ecc_key, GNUTLS_X509_FMT_PEM) >= 0);
			assert(gnutls_certificate_set_x509_key_mem
						 (server_creds, &server_ca3_localhost_rsa_sign_cert,
				&server_ca3_key, GNUTLS_X509_FMT_PEM) >= 0);
			gnutls_certificate_set_known_dh_params(server_creds,
										 GNUTLS_SEC_PARAM_MEDIUM);
		}

		// Test for using RawPubKey srv credentials
		if (test->set_srv_creds & CRED_RAWPK) {
			assert(gnutls_certificate_set_rawpk_key_mem(server_creds,
				&rawpk_public_key2, &rawpk_private_key2, GNUTLS_X509_FMT_PEM,
				NULL, 0, NULL, 0, 0) >= 0);
		}

		// -- Add extra ctype creds here in the future --

		// Finally set the credentials
		gnutls_credentials_set(server, GNUTLS_CRD_CERTIFICATE, server_creds);
	}

	// Server settings
	gnutls_transport_set_push_function(server, server_push);
	gnutls_transport_set_pull_function(server, server_pull);
	gnutls_transport_set_ptr(server, server);
	assert(gnutls_priority_set_direct(server, test->server_prio, 0) >= 0);

	if (test->request_cli_crt)
		gnutls_certificate_server_set_request(server, GNUTLS_CERT_REQUEST);

	// Client settings
	gnutls_transport_set_push_function(client, client_push);
	gnutls_transport_set_pull_function(client, client_pull);
	gnutls_transport_set_ptr(client, client);
	assert(gnutls_priority_set_direct(client, test->client_prio, 0) >= 0);

	// Try handshake
	if (test->client_err && test->server_err) {
		/* We expect errors during the handshake and don't check
		 * any negotiated certificate types */
		HANDSHAKE_EXPECT(client, server, test->client_err, test->server_err);
	} else {
		/* We expect a handshake without errors and check the negotiated
		 * certificate types */
		HANDSHAKE(client, server);

		/* Get the negotiated certificate types */
		srv_srv_ctype =
				gnutls_certificate_type_get2(server, GNUTLS_CTYPE_SERVER);
		srv_cli_ctype =
				gnutls_certificate_type_get2(server, GNUTLS_CTYPE_CLIENT);
		cli_srv_ctype =
				gnutls_certificate_type_get2(client, GNUTLS_CTYPE_SERVER);
		cli_cli_ctype =
				gnutls_certificate_type_get2(client, GNUTLS_CTYPE_CLIENT);

		// For debugging
		if (debug) {
				success("Srv srv ctype: %s\n", gnutls_certificate_type_get_name(srv_srv_ctype));
				success("Srv cli ctype: %s\n", gnutls_certificate_type_get_name(srv_cli_ctype));
				success("Cli srv ctype: %s\n", gnutls_certificate_type_get_name(cli_srv_ctype));
				success("Cli srv ctype: %s\n", gnutls_certificate_type_get_name(cli_cli_ctype));
		}

		/* Check whether the negotiated certificate types match the expected results */
		if (test->cli_srv_may_diverge) {
			// Matching expected client ctype at client
			if (cli_cli_ctype != test->expected_cli_cli_ctype) {
				fail("%s: negotiated client ctype at client diffs the expected (%s, %s)!\n", test->name, gnutls_certificate_type_get_name(cli_cli_ctype), gnutls_certificate_type_get_name(test->expected_cli_cli_ctype));
			}
			// Matching expected server ctype at client
			if (cli_srv_ctype != test->expected_cli_srv_ctype) {
				fail("%s: negotiated server ctype at client diffs the expected (%s, %s)!\n", test->name, gnutls_certificate_type_get_name(cli_srv_ctype), gnutls_certificate_type_get_name(test->expected_cli_srv_ctype));
			}
			// Matching expected client ctype at server
			if (srv_cli_ctype != test->expected_srv_cli_ctype) {
				fail("%s: negotiated client ctype at server diffs the expected (%s, %s)!\n", test->name, gnutls_certificate_type_get_name(srv_cli_ctype), gnutls_certificate_type_get_name(test->expected_srv_cli_ctype));
			}
			// Matching expected server ctype at server
			if (srv_srv_ctype != test->expected_srv_srv_ctype) {
				fail("%s: negotiated client ctype at client diffs the expected (%s, %s)!\n", test->name, gnutls_certificate_type_get_name(srv_srv_ctype), gnutls_certificate_type_get_name(test->expected_srv_srv_ctype));
			}
		} else {
			// Matching server ctype
			if (srv_srv_ctype != cli_srv_ctype) {
				fail("%s: client negotiated different server ctype than server (%s, %s)!\n", test->name, gnutls_certificate_type_get_name(cli_srv_ctype), gnutls_certificate_type_get_name(srv_srv_ctype));
			}
			// Matching client ctype
			if (srv_cli_ctype != cli_cli_ctype) {
				fail("%s: client negotiated different client ctype than server (%s, %s)!\n", test->name, gnutls_certificate_type_get_name(cli_cli_ctype), gnutls_certificate_type_get_name(srv_cli_ctype));
			}
			// Matching expected server ctype
			if (srv_srv_ctype != test->expected_srv_ctype) {
				fail("%s: negotiated server ctype diffs the expected (%s, %s)!\n", test->name, gnutls_certificate_type_get_name(srv_srv_ctype), gnutls_certificate_type_get_name(test->expected_srv_ctype));
			}
			// Matching expected client ctype
			if (srv_cli_ctype != test->expected_cli_ctype) {
				fail("%s: negotiated client ctype diffs the expected (%s, %s)!\n", test->name, gnutls_certificate_type_get_name(srv_cli_ctype), gnutls_certificate_type_get_name(test->expected_cli_ctype));
			}

			/* Check whether the API functions return the correct cert types for OURS and PEERS */
			assert(srv_srv_ctype == gnutls_certificate_type_get2(server, GNUTLS_CTYPE_OURS));
			assert(srv_srv_ctype == gnutls_certificate_type_get2(client, GNUTLS_CTYPE_PEERS));
			assert(cli_cli_ctype == gnutls_certificate_type_get2(server, GNUTLS_CTYPE_PEERS));
			assert(cli_cli_ctype == gnutls_certificate_type_get2(client, GNUTLS_CTYPE_OURS));
		}
	}

	// Cleanup
	gnutls_deinit(server);
	gnutls_deinit(client);
	gnutls_certificate_free_credentials(client_creds);
	gnutls_certificate_free_credentials(server_creds);

	reset_buffers();
}
