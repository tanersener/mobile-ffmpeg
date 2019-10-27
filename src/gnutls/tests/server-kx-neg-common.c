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
	int server_ret;
	int client_ret;
	unsigned have_anon_cred;
	unsigned have_anon_dh_params;
	unsigned have_anon_exp_dh_params;
	unsigned have_srp_cred;
	unsigned have_psk_cred;
	unsigned have_psk_dh_params;
	unsigned have_psk_exp_dh_params;
	unsigned have_cert_cred;
	unsigned have_cert_dh_params;
	unsigned have_cert_exp_dh_params;
	unsigned have_rsa_sign_cert;
	unsigned have_ecc_sign_cert;
	unsigned have_ed25519_sign_cert;
	unsigned have_rsa_decrypt_cert;
	unsigned not_on_fips;
	unsigned exp_version;
	const char *client_prio;
	const char *server_prio;
} test_case_st;

static int
serv_psk_func(gnutls_session_t session, const char *username,
	gnutls_datum_t * key) {
	key->data = gnutls_malloc(4);
	assert(key->data != NULL);
	key->data[0] = 0xDE;
	key->data[1] = 0xAD;
	key->data[2] = 0xBE;
	key->data[3] = 0xEF;
	key->size = 4;
	return 0;
}

#define SALT_TEST1 "3a3xX3Myzb9YJn5X0R7sbx"
#define VERIF_TEST1 "CEqjUZBlkQCocfOR0E4AsPZKOFYPGjKFMHW7KDcnpE4sH4.iGMbkygb/bViRNjskF9/TQdD46Mvlt6pLs5MZoTn8mO3G.RGyXdWuIrhnVn29p41Cpc5RhTLaeUm3asW6LF60VTKnGERC0eB37xZUsaTpzmaTNdD4mOoYCN3bD9Y"
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

gnutls_datum_t test1_salt = 
{
	.data = (void*)SALT_TEST1,
	.size = sizeof(SALT_TEST1)-1
};

const char *side;
#define switch_side(str) side = str

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "%s|<%d>| %s", side, level, str);
}

static int
serv_srp_func(gnutls_session_t session, const char *username,
	      gnutls_datum_t *salt, gnutls_datum_t *verifier, gnutls_datum_t *generator,
	      gnutls_datum_t *prime)
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
	} else {
		fail("Unknown username %s\n", username);
	}

	return 0;
}

static void try(test_case_st *test)
{
	int sret, cret, ret;
	gnutls_anon_client_credentials_t c_anon_cred;
	gnutls_anon_server_credentials_t s_anon_cred;
	gnutls_psk_client_credentials_t c_psk_cred;
	gnutls_psk_server_credentials_t s_psk_cred;
	gnutls_certificate_credentials_t s_cert_cred;
	gnutls_certificate_credentials_t c_cert_cred;
	gnutls_srp_server_credentials_t s_srp_cred;
	gnutls_srp_client_credentials_t c_srp_cred;
	const gnutls_datum_t p3_2048 =
	    { (void *)pkcs3_2048, strlen(pkcs3_2048) };
	gnutls_dh_params_t dh_params = NULL;

	gnutls_session_t server, client;
	const gnutls_datum_t pskkey = { (void *) "DEADBEEF", 8 };

	if (test->not_on_fips && gnutls_fips140_mode_enabled()) {
		success("Skipping %s...\n", test->name);
		return;
	}

	success("Running %s...\n", test->name);

	/* General init. */
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(6);

	assert(gnutls_anon_allocate_client_credentials(&c_anon_cred) >= 0);
	assert(gnutls_anon_allocate_server_credentials(&s_anon_cred) >= 0);
	assert(gnutls_psk_allocate_client_credentials(&c_psk_cred) >= 0);
	assert(gnutls_psk_allocate_server_credentials(&s_psk_cred) >= 0);
	assert(gnutls_srp_allocate_client_credentials(&c_srp_cred) >= 0);
	assert(gnutls_srp_allocate_server_credentials(&s_srp_cred) >= 0);
	assert(gnutls_certificate_allocate_credentials(&s_cert_cred) >= 0);
	assert(gnutls_certificate_allocate_credentials(&c_cert_cred) >= 0);
	assert(gnutls_dh_params_init(&dh_params) >= 0);

	assert(gnutls_init(&server, GNUTLS_SERVER)>=0);
	assert(gnutls_init(&client, GNUTLS_CLIENT)>=0);

	if (test->have_anon_cred) {
		gnutls_credentials_set(server, GNUTLS_CRD_ANON, s_anon_cred);
		if (test->have_anon_dh_params)
			gnutls_anon_set_server_known_dh_params(s_anon_cred, GNUTLS_SEC_PARAM_MEDIUM);
		else if (test->have_anon_exp_dh_params) {
			ret = gnutls_dh_params_import_pkcs3(dh_params, &p3_2048,
							    GNUTLS_X509_FMT_PEM);
			assert(ret>=0);
			gnutls_anon_set_server_dh_params(s_anon_cred, dh_params);
		}
	}

	if (test->have_cert_cred) {
		gnutls_credentials_set(server, GNUTLS_CRD_CERTIFICATE, s_cert_cred);
		if (test->have_cert_dh_params)
			gnutls_certificate_set_known_dh_params(s_cert_cred, GNUTLS_SEC_PARAM_MEDIUM);
		else if (test->have_cert_exp_dh_params) {
			ret = gnutls_dh_params_import_pkcs3(dh_params, &p3_2048,
							    GNUTLS_X509_FMT_PEM);
			assert(ret>=0);
			gnutls_certificate_set_dh_params(s_cert_cred, dh_params);
		}
	}

	if (test->have_psk_cred) {
		gnutls_credentials_set(server, GNUTLS_CRD_PSK, s_psk_cred);
		if (test->have_psk_dh_params)
			gnutls_psk_set_server_known_dh_params(s_psk_cred, GNUTLS_SEC_PARAM_MEDIUM);
		else if (test->have_psk_exp_dh_params) {
			ret = gnutls_dh_params_import_pkcs3(dh_params, &p3_2048,
							    GNUTLS_X509_FMT_PEM);
			assert(ret>=0);
			gnutls_psk_set_server_dh_params(s_psk_cred, dh_params);
		}

		gnutls_psk_set_server_credentials_function(s_psk_cred, serv_psk_func);
	}

	if (test->have_srp_cred) {
		gnutls_credentials_set(server, GNUTLS_CRD_SRP, s_srp_cred);

		gnutls_srp_set_server_credentials_function(s_srp_cred, serv_srp_func);
	}

	if (test->have_rsa_decrypt_cert) {
		assert(gnutls_certificate_set_x509_key_mem(s_cert_cred, &server_ca3_localhost_rsa_decrypt_cert, &server_ca3_key, GNUTLS_X509_FMT_PEM) >= 0);
	}

	if (test->have_ecc_sign_cert) {
		assert(gnutls_certificate_set_x509_key_mem(s_cert_cred, &server_ca3_localhost_ecc_cert, &server_ca3_ecc_key, GNUTLS_X509_FMT_PEM) >= 0);
	}

	if (test->have_ed25519_sign_cert) {
		assert(gnutls_certificate_set_x509_key_mem(s_cert_cred, &server_ca3_eddsa_cert, &server_ca3_eddsa_key, GNUTLS_X509_FMT_PEM) >= 0);
	}

	if (test->have_rsa_sign_cert) {
		assert(gnutls_certificate_set_x509_key_mem(s_cert_cred, &server_ca3_localhost_rsa_sign_cert, &server_ca3_key, GNUTLS_X509_FMT_PEM) >= 0);
	}

	/* client does everything */
	gnutls_credentials_set(client, GNUTLS_CRD_ANON, c_anon_cred);
	gnutls_credentials_set(client, GNUTLS_CRD_CERTIFICATE, c_cert_cred);
	gnutls_credentials_set(client, GNUTLS_CRD_PSK, c_psk_cred);
	gnutls_credentials_set(client, GNUTLS_CRD_SRP, c_srp_cred);

	assert(gnutls_psk_set_client_credentials(c_psk_cred, "psk", &pskkey, GNUTLS_PSK_KEY_HEX) >= 0);

	assert(gnutls_srp_set_client_credentials(c_srp_cred, "test1", "test") >= 0);

	gnutls_transport_set_push_function(server, server_push);
	gnutls_transport_set_pull_function(server, server_pull);
	gnutls_transport_set_ptr(server, server);
	assert(gnutls_priority_set_direct(server, test->server_prio, 0) >= 0);

	gnutls_transport_set_push_function(client, client_push);
	gnutls_transport_set_pull_function(client, client_pull);
	gnutls_transport_set_ptr(client, client);
	assert(gnutls_priority_set_direct(client, test->client_prio, 0) >= 0);

	HANDSHAKE_EXPECT(client, server, test->client_ret, test->server_ret);

	if (test->client_ret == 0 && test->server_ret == 0 && test->exp_version) {
		if (gnutls_protocol_get_version(client) != test->exp_version)
			fail("expected version (%s) does not match %s\n",
			     gnutls_protocol_get_name(test->exp_version),
			     gnutls_protocol_get_name(gnutls_protocol_get_version(client)));
	}

	gnutls_deinit(server);
	gnutls_deinit(client);
	gnutls_anon_free_client_credentials(c_anon_cred);
	gnutls_anon_free_server_credentials(s_anon_cred);
	gnutls_psk_free_client_credentials(c_psk_cred);
	gnutls_psk_free_server_credentials(s_psk_cred);
	gnutls_srp_free_client_credentials(c_srp_cred);
	gnutls_srp_free_server_credentials(s_srp_cred);
	gnutls_certificate_free_credentials(s_cert_cred);
	gnutls_certificate_free_credentials(c_cert_cred);
	if (dh_params)
		gnutls_dh_params_deinit(dh_params);

	reset_buffers();
}
