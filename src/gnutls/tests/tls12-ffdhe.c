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
#include "utils.h"
#include "cert-common.h"
#include "eagain-common.h"

#include <assert.h>

typedef struct test_case_st {
	const char *name;
	int server_ret;
	int client_ret;
	unsigned have_anon_cred;
	unsigned have_psk_cred;
	unsigned have_cert_cred;
	unsigned have_rsa_sign_cert;
	unsigned have_ecc_sign_cert;
	unsigned have_rsa_decrypt_cert;
	unsigned not_on_fips;
	unsigned group; /* expected */
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

static void try(test_case_st *test)
{
	int sret, cret;
	gnutls_anon_client_credentials_t c_anon_cred;
	gnutls_anon_server_credentials_t s_anon_cred;
	gnutls_psk_client_credentials_t c_psk_cred;
	gnutls_psk_server_credentials_t s_psk_cred;
	gnutls_certificate_credentials_t s_cert_cred;
	gnutls_certificate_credentials_t c_cert_cred;

	gnutls_session_t server, client;
	const gnutls_datum_t pskkey = { (void *) "DEADBEEF", 8 };

	if (test->not_on_fips && gnutls_fips140_mode_enabled()) {
		success("Skipping %s...\n", test->name);
		return;
	}

	success("Running %s...\n", test->name);

	assert(gnutls_anon_allocate_client_credentials(&c_anon_cred) >= 0);
	assert(gnutls_anon_allocate_server_credentials(&s_anon_cred) >= 0);
	assert(gnutls_psk_allocate_client_credentials(&c_psk_cred) >= 0);
	assert(gnutls_psk_allocate_server_credentials(&s_psk_cred) >= 0);
	assert(gnutls_certificate_allocate_credentials(&s_cert_cred) >= 0);
	assert(gnutls_certificate_allocate_credentials(&c_cert_cred) >= 0);

	assert(gnutls_init(&server, GNUTLS_SERVER)>=0);
	assert(gnutls_init(&client, GNUTLS_CLIENT)>=0);

	if (test->have_anon_cred) {
		gnutls_credentials_set(server, GNUTLS_CRD_ANON, s_anon_cred);
	}

	if (test->have_cert_cred) {
		gnutls_credentials_set(server, GNUTLS_CRD_CERTIFICATE, s_cert_cred);
	}

	if (test->have_psk_cred) {
		gnutls_credentials_set(server, GNUTLS_CRD_PSK, s_psk_cred);

		gnutls_psk_set_server_credentials_function(s_psk_cred, serv_psk_func);
	}

	if (test->have_rsa_decrypt_cert) {
		assert(gnutls_certificate_set_x509_key_mem(s_cert_cred, &server_ca3_localhost_rsa_decrypt_cert, &server_ca3_key, GNUTLS_X509_FMT_PEM) >= 0);
	}

	if (test->have_ecc_sign_cert) {
		assert(gnutls_certificate_set_x509_key_mem(s_cert_cred, &server_ca3_localhost_ecc_cert, &server_ca3_ecc_key, GNUTLS_X509_FMT_PEM) >= 0);
	}

	if (test->have_rsa_sign_cert) {
		assert(gnutls_certificate_set_x509_key_mem(s_cert_cred, &server_ca3_localhost_rsa_sign_cert, &server_ca3_key, GNUTLS_X509_FMT_PEM) >= 0);
	}

	/* client does everything */
	gnutls_credentials_set(client, GNUTLS_CRD_ANON, c_anon_cred);
	gnutls_credentials_set(client, GNUTLS_CRD_CERTIFICATE, c_cert_cred);
	gnutls_credentials_set(client, GNUTLS_CRD_PSK, c_psk_cred);

	assert(gnutls_psk_set_client_credentials(c_psk_cred, "psk", &pskkey, GNUTLS_PSK_KEY_HEX) >= 0);

	gnutls_transport_set_push_function(server, server_push);
	gnutls_transport_set_pull_function(server, server_pull);
	gnutls_transport_set_ptr(server, server);
	assert(gnutls_priority_set_direct(server, test->server_prio, 0) >= 0);

	gnutls_transport_set_push_function(client, client_push);
	gnutls_transport_set_pull_function(client, client_pull);
	gnutls_transport_set_ptr(client, client);
	assert(gnutls_priority_set_direct(client, test->client_prio, 0) >= 0);

	HANDSHAKE_EXPECT(client, server, test->client_ret, test->server_ret);

	if (gnutls_group_get(client) != gnutls_group_get(server)) {
		fail("%s: server's group doesn't match client's\n", test->name);
	}

	if (test->group != 0 && gnutls_group_get(client) != test->group) {
		fail("%s: group doesn't match the expected\n", test->name);
	}

	if (test->group) {
		if (test->group == GNUTLS_GROUP_FFDHE2048 || test->group == GNUTLS_GROUP_FFDHE3072 ||
		    test->group == GNUTLS_GROUP_FFDHE4096 || test->group == GNUTLS_GROUP_FFDHE6144 ||
		    test->group == GNUTLS_GROUP_FFDHE8192) {
			if (!(gnutls_session_get_flags(client) & GNUTLS_SFLAGS_RFC7919)) {
				fail("%s: gnutls_session_get_flags(client) reports that no RFC7919 negotiation was performed!\n", test->name);
			}

			if (!(gnutls_session_get_flags(server) & GNUTLS_SFLAGS_RFC7919)) {
				fail("%s: gnutls_session_get_flags(server) reports that no RFC7919 negotiation was performed!\n", test->name);
			}
		}
	}
	gnutls_deinit(server);
	gnutls_deinit(client);
	gnutls_anon_free_client_credentials(c_anon_cred);
	gnutls_anon_free_server_credentials(s_anon_cred);
	gnutls_psk_free_client_credentials(c_psk_cred);
	gnutls_psk_free_server_credentials(s_psk_cred);
	gnutls_certificate_free_credentials(s_cert_cred);
	gnutls_certificate_free_credentials(c_cert_cred);

	reset_buffers();
}

test_case_st tests[] = {
	{
		.name = "TLS 1.2 ANON-DH (defaults)",
		.client_ret = 0,
		.server_ret = 0,
		.have_anon_cred = 1,
		.server_prio = "NORMAL:-KX-ALL:+ANON-DH:-VERS-ALL:+VERS-TLS1.2",
		.client_prio = "NORMAL:-KX-ALL:+ANON-DH:-VERS-ALL:+VERS-TLS1.2"
	},
	{
		.name = "TLS 1.2 ANON-DH (FFDHE2048)",
		.group = GNUTLS_GROUP_FFDHE2048,
		.client_ret = 0,
		.server_ret = 0,
		.have_anon_cred = 1,
		.server_prio = "NORMAL:-KX-ALL:+ANON-DH:-VERS-ALL:+VERS-TLS1.2:-GROUP-ALL:+GROUP-FFDHE2048",
		.client_prio = "NORMAL:-KX-ALL:+ANON-DH:-VERS-ALL:+VERS-TLS1.2:-GROUP-ALL:+GROUP-FFDHE2048"
	},
	{
		.name = "TLS 1.2 ANON-DH (FFDHE3072)",
		.group = GNUTLS_GROUP_FFDHE3072,
		.client_ret = 0,
		.server_ret = 0,
		.have_anon_cred = 1,
		.server_prio = "NORMAL:-KX-ALL:+ANON-DH:-VERS-ALL:+VERS-TLS1.2:-GROUP-ALL:+GROUP-FFDHE3072",
		.client_prio = "NORMAL:-KX-ALL:+ANON-DH:-VERS-ALL:+VERS-TLS1.2:-GROUP-ALL:+GROUP-FFDHE3072"
	},
	{
		.name = "TLS 1.2 ANON-DH (FFDHE4096)",
		.group = GNUTLS_GROUP_FFDHE4096,
		.client_ret = 0,
		.server_ret = 0,
		.have_anon_cred = 1,
		.server_prio = "NORMAL:-KX-ALL:+ANON-DH:-VERS-ALL:+VERS-TLS1.2:-GROUP-ALL:+GROUP-FFDHE4096",
		.client_prio = "NORMAL:-KX-ALL:+ANON-DH:-VERS-ALL:+VERS-TLS1.2:-GROUP-ALL:+GROUP-FFDHE4096"
	},
	{
		.name = "TLS 1.2 ANON-DH (FFDHE6144)",
		.group = GNUTLS_GROUP_FFDHE6144,
		.client_ret = 0,
		.server_ret = 0,
		.have_anon_cred = 1,
		.server_prio = "NORMAL:-KX-ALL:+ANON-DH:-VERS-ALL:+VERS-TLS1.2:-GROUP-ALL:+GROUP-FFDHE6144",
		.client_prio = "NORMAL:-KX-ALL:+ANON-DH:-VERS-ALL:+VERS-TLS1.2:-GROUP-ALL:+GROUP-FFDHE6144"
	},
	{
		.name = "TLS 1.2 ANON-DH (FFDHE8192)",
		.group = GNUTLS_GROUP_FFDHE8192,
		.client_ret = 0,
		.server_ret = 0,
		.have_anon_cred = 1,
		.server_prio = "NORMAL:-KX-ALL:+ANON-DH:-VERS-ALL:+VERS-TLS1.2:-GROUP-ALL:+GROUP-FFDHE8192",
		.client_prio = "NORMAL:-KX-ALL:+ANON-DH:-VERS-ALL:+VERS-TLS1.2:-GROUP-ALL:+GROUP-FFDHE8192"
	},
	{
		.name = "TLS 1.2 DHE-PSK (defaults)",
		.client_ret = 0,
		.server_ret = 0,
		.have_psk_cred = 1,
		.server_prio = "NORMAL:-KX-ALL:+DHE-PSK:-VERS-ALL:+VERS-TLS1.2",
		.client_prio = "NORMAL:-KX-ALL:+DHE-PSK:-VERS-ALL:+VERS-TLS1.2"
	},
	{
		.name = "TLS 1.2 DHE-PSK (FFDHE2048)",
		.client_ret = 0,
		.server_ret = 0,
		.group = GNUTLS_GROUP_FFDHE2048,
		.have_psk_cred = 1,
		.server_prio = "NORMAL:-KX-ALL:+DHE-PSK:-VERS-ALL:+VERS-TLS1.2:-GROUP-ALL:+GROUP-FFDHE2048",
		.client_prio = "NORMAL:-KX-ALL:+DHE-PSK:-VERS-ALL:+VERS-TLS1.2:-GROUP-ALL:+GROUP-FFDHE2048"
	},
	{
		.name = "TLS 1.2 DHE-PSK (FFDHE3072)",
		.group = GNUTLS_GROUP_FFDHE3072,
		.client_ret = 0,
		.server_ret = 0,
		.have_psk_cred = 1,
		.server_prio = "NORMAL:-KX-ALL:+DHE-PSK:-VERS-ALL:+VERS-TLS1.2:-GROUP-ALL:+GROUP-FFDHE3072",
		.client_prio = "NORMAL:-KX-ALL:+DHE-PSK:-VERS-ALL:+VERS-TLS1.2:-GROUP-ALL:+GROUP-FFDHE3072"
	},
	{
		.name = "TLS 1.2 DHE-PSK (FFDHE4096)",
		.group = GNUTLS_GROUP_FFDHE4096,
		.client_ret = 0,
		.server_ret = 0,
		.have_psk_cred = 1,
		.server_prio = "NORMAL:-KX-ALL:+DHE-PSK:-VERS-ALL:+VERS-TLS1.2:-GROUP-ALL:+GROUP-FFDHE4096",
		.client_prio = "NORMAL:-KX-ALL:+DHE-PSK:-VERS-ALL:+VERS-TLS1.2:-GROUP-ALL:+GROUP-FFDHE4096"
	},
	{
		.name = "TLS 1.2 DHE-PSK (FFDHE6144)",
		.client_ret = 0,
		.server_ret = 0,
		.group = GNUTLS_GROUP_FFDHE6144,
		.have_psk_cred = 1,
		.server_prio = "NORMAL:-KX-ALL:+DHE-PSK:-VERS-ALL:+VERS-TLS1.2:-GROUP-ALL:+GROUP-FFDHE6144",
		.client_prio = "NORMAL:-KX-ALL:+DHE-PSK:-VERS-ALL:+VERS-TLS1.2:-GROUP-ALL:+GROUP-FFDHE6144"
	},
	{
		.name = "TLS 1.2 DHE-PSK (FFDHE8192)",
		.group = GNUTLS_GROUP_FFDHE8192,
		.client_ret = 0,
		.server_ret = 0,
		.have_psk_cred = 1,
		.server_prio = "NORMAL:-KX-ALL:+DHE-PSK:-VERS-ALL:+VERS-TLS1.2:-GROUP-ALL:+GROUP-FFDHE8192",
		.client_prio = "NORMAL:-KX-ALL:+DHE-PSK:-VERS-ALL:+VERS-TLS1.2:-GROUP-ALL:+GROUP-FFDHE8192"
	},
	{
		.name = "TLS 1.2 DHE-RSA (defaults)",
		.client_ret = 0,
		.server_ret = 0,
		.have_cert_cred = 1,
		.have_rsa_sign_cert = 1,
		.server_prio = "NORMAL:-KX-ALL:+DHE-RSA:-VERS-ALL:+VERS-TLS1.2",
		.client_prio = "NORMAL:-KX-ALL:+DHE-RSA:-VERS-ALL:+VERS-TLS1.2"
	},
	{
		.name = "TLS 1.2 DHE-RSA (FFDHE2048)",
		.group = GNUTLS_GROUP_FFDHE2048,
		.client_ret = 0,
		.server_ret = 0,
		.have_cert_cred = 1,
		.have_rsa_sign_cert = 1,
		.server_prio = "NORMAL:-KX-ALL:+DHE-RSA:-VERS-ALL:+VERS-TLS1.2:-GROUP-ALL:+GROUP-FFDHE2048",
		.client_prio = "NORMAL:-KX-ALL:+DHE-RSA:-VERS-ALL:+VERS-TLS1.2:-GROUP-ALL:+GROUP-FFDHE2048"
	},
	{
		.name = "TLS 1.2 DHE-RSA (FFDHE3072)",
		.group = GNUTLS_GROUP_FFDHE3072,
		.client_ret = 0,
		.server_ret = 0,
		.have_cert_cred = 1,
		.have_rsa_sign_cert = 1,
		.server_prio = "NORMAL:-KX-ALL:+DHE-RSA:-VERS-ALL:+VERS-TLS1.2:-GROUP-ALL:+GROUP-FFDHE3072",
		.client_prio = "NORMAL:-KX-ALL:+DHE-RSA:-VERS-ALL:+VERS-TLS1.2:-GROUP-ALL:+GROUP-FFDHE3072"
	},
	{
		.name = "TLS 1.2 DHE-RSA (FFDHE4096)",
		.group = GNUTLS_GROUP_FFDHE4096,
		.client_ret = 0,
		.server_ret = 0,
		.have_cert_cred = 1,
		.have_rsa_sign_cert = 1,
		.server_prio = "NORMAL:-KX-ALL:+DHE-RSA:-VERS-ALL:+VERS-TLS1.2:-GROUP-ALL:+GROUP-FFDHE4096",
		.client_prio = "NORMAL:-KX-ALL:+DHE-RSA:-VERS-ALL:+VERS-TLS1.2:-GROUP-ALL:+GROUP-FFDHE4096"
	},
	{
		.name = "TLS 1.2 DHE-RSA (FFDHE6144)",
		.group = GNUTLS_GROUP_FFDHE6144,
		.client_ret = 0,
		.server_ret = 0,
		.have_cert_cred = 1,
		.have_rsa_sign_cert = 1,
		.server_prio = "NORMAL:-KX-ALL:+DHE-RSA:-VERS-ALL:+VERS-TLS1.2:-GROUP-ALL:+GROUP-FFDHE6144",
		.client_prio = "NORMAL:-KX-ALL:+DHE-RSA:-VERS-ALL:+VERS-TLS1.2:-GROUP-ALL:+GROUP-FFDHE6144"
	},
	{
		.name = "TLS 1.2 DHE-RSA (FFDHE8192)",
		.group = GNUTLS_GROUP_FFDHE8192,
		.client_ret = 0,
		.server_ret = 0,
		.have_cert_cred = 1,
		.have_rsa_sign_cert = 1,
		.server_prio = "NORMAL:-KX-ALL:+DHE-RSA:-VERS-ALL:+VERS-TLS1.2:-GROUP-ALL:+GROUP-FFDHE8192",
		.client_prio = "NORMAL:-KX-ALL:+DHE-RSA:-VERS-ALL:+VERS-TLS1.2:-GROUP-ALL:+GROUP-FFDHE8192"
	},
	{
		.name = "TLS 1.2 DHE-RSA (incompatible options)",
		.client_ret = GNUTLS_E_AGAIN,
		.server_ret = GNUTLS_E_NO_CIPHER_SUITES,
		.have_cert_cred = 1,
		.have_rsa_sign_cert = 1,
		.server_prio = "NORMAL:-KX-ALL:+DHE-RSA:-VERS-ALL:+VERS-TLS1.2:-GROUP-ALL:+GROUP-FFDHE8192",
		.client_prio = "NORMAL:-KX-ALL:+DHE-RSA:-VERS-ALL:+VERS-TLS1.2:-GROUP-ALL:+GROUP-FFDHE3072"
	},
	{
		.name = "TLS 1.2 DHE-RSA (complex neg)",
		.group = GNUTLS_GROUP_FFDHE3072,
		.client_ret = 0,
		.server_ret = 0,
		.have_cert_cred = 1,
		.have_rsa_sign_cert = 1,
		.server_prio = "NORMAL:-KX-ALL:+DHE-RSA:-VERS-ALL:+VERS-TLS1.2:-GROUP-ALL:+GROUP-FFDHE8192:+GROUP-FFDHE2048:+GROUP-FFDHE3072",
		.client_prio = "NORMAL:-KX-ALL:+DHE-RSA:-VERS-ALL:+VERS-TLS1.2:-GROUP-ALL:+GROUP-FFDHE3072"
	},
	{
		.name = "TLS 1.2 DHE-RSA (negotiation over ECDHE)",
		.group = GNUTLS_GROUP_FFDHE3072,
		.client_ret = 0,
		.server_ret = 0,
		.have_cert_cred = 1,
		.have_rsa_sign_cert = 1,
		.server_prio = "NORMAL:-KX-ALL:+DHE-RSA:+ECDHE-RSA:-VERS-ALL:+VERS-TLS1.2:-GROUP-ALL:+GROUP-SECP256R1:+GROUP-FFDHE8192:+GROUP-FFDHE2048:+GROUP-FFDHE3072",
		.client_prio = "NORMAL:-KX-ALL:+DHE-RSA:+ECDHE-RSA:-VERS-ALL:+VERS-TLS1.2:-GROUP-ALL:+GROUP-SECP256R1:+GROUP-FFDHE3072"
	},
	{
		.name = "TLS 1.2 DHE-RSA (negotiation over ECDHE - prio on ECDHE)",
		.group = GNUTLS_GROUP_SECP256R1,
		.client_ret = 0,
		.server_ret = 0,
		.have_cert_cred = 1,
		.have_rsa_sign_cert = 1,
		.server_prio = "NORMAL:-KX-ALL:+ECDHE-RSA:+DHE-RSA:-VERS-ALL:+VERS-TLS1.2:-GROUP-ALL:+GROUP-FFDHE8192:+GROUP-FFDHE2048:+GROUP-FFDHE3072:+GROUP-SECP256R1",
		.client_prio = "NORMAL:-KX-ALL:+ECDHE-RSA:+DHE-RSA:-VERS-ALL:+VERS-TLS1.2:-GROUP-ALL:+GROUP-FFDHE3072:+GROUP-SECP256R1"
	}
};

void doit(void)
{
	unsigned i;
	global_init();

	for (i=0;i<sizeof(tests)/sizeof(tests[0]);i++) {
		try(&tests[i]);
	}

	gnutls_global_deinit();
}
