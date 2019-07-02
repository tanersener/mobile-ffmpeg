/*
 * Copyright (C) 2017 Red Hat, Inc.
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
#include <assert.h>

typedef struct test_case_st {
	const char *name;
	int cipher;
	int group;
	const char *client_prio;
	const char *server_prio;
	const char *desc;
	unsigned not_on_fips;
} test_case_st;

static void try(test_case_st *test)
{
	int sret, cret;
	gnutls_certificate_credentials_t s_cert_cred;
	gnutls_certificate_credentials_t c_cert_cred;
	gnutls_session_t server, client;

	if (test->not_on_fips && gnutls_fips140_mode_enabled()) {
		success("Skipping %s...\n", test->name);
		return;
	}

	success("Running %s...\n", test->name);

	assert(gnutls_certificate_allocate_credentials(&s_cert_cred) >= 0);
	assert(gnutls_certificate_allocate_credentials(&c_cert_cred) >= 0);

	assert(gnutls_init(&server, GNUTLS_SERVER)>=0);
	assert(gnutls_init(&client, GNUTLS_CLIENT)>=0);

	gnutls_credentials_set(server, GNUTLS_CRD_CERTIFICATE, s_cert_cred);
	gnutls_certificate_set_known_dh_params(s_cert_cred, GNUTLS_SEC_PARAM_MEDIUM);

	assert(gnutls_certificate_set_x509_key_mem(s_cert_cred, &server_ca3_localhost_rsa_decrypt_cert, &server_ca3_key, GNUTLS_X509_FMT_PEM) >= 0);
	assert(gnutls_certificate_set_x509_key_mem(s_cert_cred, &server_ca3_localhost_rsa_sign_cert, &server_ca3_key, GNUTLS_X509_FMT_PEM) >= 0);
	assert(gnutls_certificate_set_x509_key_mem(s_cert_cred, &server_ca3_localhost_ecc_cert, &server_ca3_ecc_key, GNUTLS_X509_FMT_PEM) >= 0);

	gnutls_credentials_set(client, GNUTLS_CRD_CERTIFICATE, c_cert_cred);

	gnutls_transport_set_push_function(server, server_push);
	gnutls_transport_set_pull_function(server, server_pull);
	gnutls_transport_set_ptr(server, server);
	assert(gnutls_priority_set_direct(server, test->server_prio, 0) >= 0);

	gnutls_transport_set_push_function(client, client_push);
	gnutls_transport_set_pull_function(client, client_pull);
	gnutls_transport_set_ptr(client, client);
	assert(gnutls_priority_set_direct(client, test->client_prio, 0) >= 0);

	HANDSHAKE(client, server);

	sret = gnutls_cipher_get(client);
	cret = gnutls_cipher_get(server);

	if (test->desc) {
		char *desc1 = gnutls_session_get_desc(server);
		char *desc2 = gnutls_session_get_desc(client);

		if (strcmp(desc1, desc2) != 0)
			fail("server and client session description don't match (%s, %s)\n", desc1, desc2);

		if (strcmp(desc1, test->desc) != 0)
			fail("session and expected session description don't match (%s, %s)\n", desc1, test->desc);
		gnutls_free(desc1);
		gnutls_free(desc2);
	}

	if (sret != cret) {
		fail("%s: client negotiated different cipher than server (%s, %s)!\n",
			test->name, gnutls_cipher_get_name(cret),
			gnutls_cipher_get_name(sret));
	}

	if (cret != test->cipher) {
		fail("%s: negotiated cipher differs with the expected (%s, %s)!\n",
			test->name, gnutls_cipher_get_name(cret),
			gnutls_cipher_get_name(test->cipher));
	}

	if (test->group) {
		sret = gnutls_group_get(client);
		cret = gnutls_group_get(server);

		if (sret != cret) {
			fail("%s: client negotiated different group than server (%s, %s)!\n",
				test->name, gnutls_group_get_name(cret),
				gnutls_group_get_name(sret));
		}

		if (cret != test->group) {
			fail("%s: negotiated group differs with the expected (%s, %s)!\n",
				test->name, gnutls_group_get_name(cret),
				gnutls_group_get_name(test->group));
		}
	}

	gnutls_deinit(server);
	gnutls_deinit(client);
	gnutls_certificate_free_credentials(s_cert_cred);
	gnutls_certificate_free_credentials(c_cert_cred);

	reset_buffers();
}
