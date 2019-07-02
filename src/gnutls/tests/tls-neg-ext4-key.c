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

/* This tests TLS negotiation using the gnutls_privkey_import_ext2() APIs */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#ifndef _WIN32
# include <netinet/in.h>
# include <sys/socket.h>
# include <arpa/inet.h>
#endif
#include <unistd.h>
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#include <gnutls/abstract.h>
#include <assert.h>
#include "cert-common.h"
#include "eagain-common.h"
#include "utils.h"

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "<%d> %s", level, str);
}

const gnutls_datum_t raw_data = {
	(void *) "hello",
	5
};


struct key_cb_data {
	gnutls_privkey_t rkey; /* the real thing */
	unsigned pk;
	unsigned sig;
	unsigned bits;
};

static int key_cb_info_func(gnutls_privkey_t key, unsigned int flags, void *userdata)
{
	struct key_cb_data *p = userdata;

	if (flags & GNUTLS_PRIVKEY_INFO_PK_ALGO)
		return p->pk;
	else if (flags & GNUTLS_PRIVKEY_INFO_PK_ALGO_BITS)
		return p->bits;
	else if (flags & GNUTLS_PRIVKEY_INFO_HAVE_SIGN_ALGO) {
		unsigned sig = GNUTLS_FLAGS_TO_SIGN_ALGO(flags);

		if (sig == p->sig)
			return 1;

		return 0;
	}

	return -1;
}

static
int key_cb_sign_data_func(gnutls_privkey_t key, gnutls_sign_algorithm_t sig,
			  void* userdata, unsigned int flags, const gnutls_datum_t *data,
			  gnutls_datum_t *signature)
{
	struct key_cb_data *p = userdata;

	if (debug)
		fprintf(stderr, "signing data with: %s\n", gnutls_sign_get_name(sig));
	return gnutls_privkey_sign_data2(p->rkey, sig, 0, data, signature);
}

static
int key_cb_sign_hash_func(gnutls_privkey_t key, gnutls_sign_algorithm_t sig,
			  void* userdata, unsigned int flags, const gnutls_datum_t *data,
			  gnutls_datum_t *signature)
{
	struct key_cb_data *p = userdata;

	if (sig == GNUTLS_SIGN_RSA_RAW) {
		if (debug)
			fprintf(stderr, "signing digestinfo with: raw RSA\n");
		return gnutls_privkey_sign_hash(p->rkey, 0, GNUTLS_PRIVKEY_SIGN_FLAG_TLS1_RSA, data, signature);
	} else {
		if (debug)
			fprintf(stderr, "signing hash with: %s\n", gnutls_sign_get_name(sig));
		return gnutls_privkey_sign_hash2(p->rkey, sig, 0, data, signature);
	}
}

static
int key_cb_decrypt_func(gnutls_privkey_t key, void *userdata, const gnutls_datum_t *ciphertext,
			gnutls_datum_t *plaintext)
{
	struct key_cb_data *p = userdata;

	return gnutls_privkey_decrypt_data(p->rkey, 0, ciphertext, plaintext);
}

static void key_cb_deinit_func(gnutls_privkey_t key, void* userdata)
{
	struct key_cb_data *p = userdata;
	gnutls_privkey_deinit(p->rkey);
	free(userdata);
}

#define testfail(fmt, ...) \
	fail("%s: "fmt, name, ##__VA_ARGS__)

static gnutls_privkey_t load_virt_privkey(const char *name, const gnutls_datum_t *txtkey,
					  gnutls_pk_algorithm_t pk, gnutls_sign_algorithm_t sig,
					  int exp_ret)
{
	gnutls_privkey_t privkey;
	struct key_cb_data *userdata;
	int ret;

	userdata = calloc(1, sizeof(struct key_cb_data));
	if (userdata == NULL) {
		testfail("memory error\n");
	}

	ret = gnutls_privkey_init(&privkey);
	if (ret < 0)
		testfail("gnutls_privkey_init\n");

	ret = gnutls_privkey_init(&userdata->rkey);
	if (ret < 0)
		testfail("gnutls_privkey_init\n");

	ret =
	    gnutls_privkey_import_x509_raw(userdata->rkey, txtkey, GNUTLS_X509_FMT_PEM, NULL, 0);
	if (ret < 0)
		testfail("gnutls_privkey_import\n");

	gnutls_privkey_get_pk_algorithm(userdata->rkey, &userdata->bits);

	userdata->pk = pk;
	userdata->sig = sig;

	ret = gnutls_privkey_import_ext4(privkey, userdata, key_cb_sign_data_func,
					 key_cb_sign_hash_func, key_cb_decrypt_func,
					 key_cb_deinit_func, key_cb_info_func,
					 GNUTLS_PRIVKEY_IMPORT_AUTO_RELEASE);
	if (ret < 0) {
		if (ret == exp_ret) {
			gnutls_privkey_deinit(userdata->rkey);
			gnutls_privkey_deinit(privkey);
			free(userdata);
			return NULL;
		}
		testfail("gnutls_privkey_import_ext2: %s\n", gnutls_strerror(ret));
	}

	return privkey;
}

static
void try_with_key(const char *name, const char *client_prio,
		 gnutls_kx_algorithm_t client_kx,
		 gnutls_sign_algorithm_t server_sign_algo,
		 gnutls_sign_algorithm_t client_sign_algo,
		 const gnutls_datum_t *serv_cert,
		 gnutls_privkey_t key,
		 int exp_serv_err)
{
	int ret;
	gnutls_pcert_st pcert_list[4];
	unsigned pcert_list_size;
	/* Server stuff. */
	gnutls_certificate_credentials_t s_xcred;
	gnutls_session_t server;
	int sret = GNUTLS_E_AGAIN;
	/* Client stuff. */
	gnutls_certificate_credentials_t c_xcred;
	gnutls_session_t client;
	int cret = GNUTLS_E_AGAIN, version;
	const char *err;

	/* General init. */
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(6);

	reset_buffers();
	/* Init server */
	gnutls_certificate_allocate_credentials(&s_xcred);

	pcert_list_size = sizeof(pcert_list)/sizeof(pcert_list[0]);
	ret = gnutls_pcert_list_import_x509_raw(pcert_list, &pcert_list_size,
		serv_cert, GNUTLS_X509_FMT_PEM, 0);
	if (ret < 0) {
		testfail("error in gnutls_pcert_list_import_x509_raw: %s\n", gnutls_strerror(ret));
	}

	ret = gnutls_certificate_set_key(s_xcred, NULL, 0, pcert_list,
				pcert_list_size, key);
	if (ret < 0) {
		testfail("Could not set key/cert: %s\n", gnutls_strerror(ret));
	}

	gnutls_init(&server, GNUTLS_SERVER);
	gnutls_credentials_set(server, GNUTLS_CRD_CERTIFICATE,
				s_xcred);

	assert(gnutls_priority_set_direct(server, "NORMAL", NULL)>=0);
	gnutls_transport_set_push_function(server, server_push);
	gnutls_transport_set_pull_function(server, server_pull);
	gnutls_transport_set_ptr(server, server);

	/* Init client */

	ret = gnutls_certificate_allocate_credentials(&c_xcred);
	if (ret < 0)
		exit(1);

#if 0
	ret = gnutls_certificate_set_x509_trust_mem(c_xcred, &ca_cert, GNUTLS_X509_FMT_PEM);
	if (ret < 0)
		exit(1);
#endif
	ret = gnutls_init(&client, GNUTLS_CLIENT);
	if (ret < 0)
		exit(1);

	ret = gnutls_credentials_set(client, GNUTLS_CRD_CERTIFICATE,
				c_xcred);
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
	if (exp_serv_err) {
		HANDSHAKE_EXPECT(client, server, GNUTLS_E_AGAIN, exp_serv_err);
		goto cleanup;
	} else {
		HANDSHAKE(client, server);
	}

	if (gnutls_kx_get(client) != client_kx) {
		testfail("got unexpected key exchange algorithm: %s (expected %s)\n", gnutls_kx_get_name(gnutls_kx_get(client)),
			gnutls_kx_get_name(client_kx));
		exit(1);
	}

	/* test signature algorithm match */
	version = gnutls_protocol_get_version(client);
	if (version >= GNUTLS_TLS1_2) {
		ret = gnutls_sign_algorithm_get(server);
		if (ret != (int)server_sign_algo && server_sign_algo != 0) {
			testfail("got unexpected server signature algorithm: %d/%s\n", ret, gnutls_sign_get_name(ret));
			exit(1);
		}

		ret = gnutls_sign_algorithm_get_client(server);
		if (ret != (int)client_sign_algo && client_sign_algo != 0) {
			testfail("got unexpected client signature algorithm: %d/%s\n", ret, gnutls_sign_get_name(ret));
			exit(1);
		}

		ret = gnutls_sign_algorithm_get(client);
		if (ret != (int)server_sign_algo && server_sign_algo != 0) {
			testfail("cl: got unexpected server signature algorithm: %d/%s\n", ret, gnutls_sign_get_name(ret));
			exit(1);
		}

		ret = gnutls_sign_algorithm_get_client(client);
		if (ret != (int)client_sign_algo && client_sign_algo != 0) {
			testfail("cl: got unexpected client signature algorithm: %d/%s\n", ret, gnutls_sign_get_name(ret));
			exit(1);
		}
	}

	gnutls_bye(client, GNUTLS_SHUT_RDWR);
	gnutls_bye(server, GNUTLS_SHUT_RDWR);

 cleanup:
	gnutls_deinit(client);
	gnutls_deinit(server);

	gnutls_certificate_free_credentials(s_xcred);
	gnutls_certificate_free_credentials(c_xcred);
}

typedef struct test_st {
	const char *name;
	gnutls_pk_algorithm_t pk;
	const char *prio;
	const gnutls_datum_t *cert;
	const gnutls_datum_t *key;
	gnutls_kx_algorithm_t exp_kx;
	unsigned sig;
	int exp_key_err;
	int exp_serv_err;
} test_st;

static const test_st tests[] = {
	{.name = "tls1.2 ecc key",
	 .pk = GNUTLS_PK_ECDSA,
	 .prio = "NORMAL:-KX-ALL:+ECDHE-RSA:+ECDHE-ECDSA:-VERS-ALL:+VERS-TLS1.2",
	 .cert = &server_ca3_localhost_ecc_cert,
	 .key = &server_ca3_ecc_key,
	 .sig = GNUTLS_SIGN_ECDSA_SHA256,
	 .exp_kx = GNUTLS_KX_ECDHE_ECDSA
	},
	{.name = "tls1.0 ecc key",
	 .pk = GNUTLS_PK_ECDSA,
	 .prio = "NORMAL:-KX-ALL:+ECDHE-RSA:+ECDHE-ECDSA:-VERS-ALL:+VERS-TLS1.0",
	 .cert = &server_ca3_localhost_ecc_cert,
	 .key = &server_ca3_ecc_key,
	 .sig = GNUTLS_SIGN_ECDSA_SHA256,
	 .exp_kx = GNUTLS_KX_ECDHE_ECDSA
	},
	{.name = "tls1.1 ecc key",
	 .pk = GNUTLS_PK_ECDSA,
	 .prio = "NORMAL:-KX-ALL:+ECDHE-RSA:+ECDHE-ECDSA:-VERS-ALL:+VERS-TLS1.1",
	 .cert = &server_ca3_localhost_ecc_cert,
	 .key = &server_ca3_ecc_key,
	 .sig = GNUTLS_SIGN_ECDSA_SHA256,
	 .exp_kx = GNUTLS_KX_ECDHE_ECDSA
	},
	{.name = "tls1.2 rsa-sign key",
	 .pk = GNUTLS_PK_RSA,
	 .prio = "NORMAL:+ECDHE-RSA:+ECDHE-ECDSA:-VERS-ALL:+VERS-TLS1.2",
	 .cert = &server_ca3_localhost_cert,
	 .key = &server_ca3_key,
	 .sig = GNUTLS_SIGN_RSA_SHA256,
	 .exp_kx = GNUTLS_KX_ECDHE_RSA
	},
	{.name = "tls1.0 rsa-sign key",
	 .pk = GNUTLS_PK_RSA,
	 .prio = "NORMAL:+ECDHE-RSA:+ECDHE-ECDSA:-VERS-ALL:+VERS-TLS1.0",
	 .cert = &server_ca3_localhost_cert,
	 .key = &server_ca3_key,
	 .sig = GNUTLS_SIGN_RSA_SHA256,
	 .exp_kx = GNUTLS_KX_ECDHE_RSA
	},
	{.name = "tls1.0 rsa-decrypt key",
	 .pk = GNUTLS_PK_RSA,
	 .prio = "NORMAL:-KX-ALL:+RSA:-VERS-ALL:+VERS-TLS1.0",
	 .cert = &server_ca3_localhost_cert,
	 .key = &server_ca3_key,
	 .exp_kx = GNUTLS_KX_RSA
	},
	{.name = "tls1.1 rsa-sign key",
	 .pk = GNUTLS_PK_RSA,
	 .prio = "NORMAL:+ECDHE-RSA:+ECDHE-ECDSA:-VERS-ALL:+VERS-TLS1.1",
	 .cert = &server_ca3_localhost_cert,
	 .key = &server_ca3_key,
	 .sig = GNUTLS_SIGN_RSA_SHA256,
	 .exp_kx = GNUTLS_KX_ECDHE_RSA
	},
	{.name = "tls1.2 rsa-sign key with rsa-pss sigs prioritized",
	 .pk = GNUTLS_PK_RSA,
	 .prio = "NORMAL:+ECDHE-RSA:+ECDHE-ECDSA:-SIGN-ALL:+SIGN-RSA-PSS-SHA256:+SIGN-RSA-PSS-SHA384:+SIGN-RSA-PSS-SHA512:+SIGN-RSA-SHA256:+SIGN-RSA-SHA384:+SIGN-RSA-SHA512:-VERS-ALL:+VERS-TLS1.2",
	 .cert = &server_ca3_localhost_cert,
	 .key = &server_ca3_key,
	 .sig = GNUTLS_SIGN_RSA_SHA256,
	 .exp_kx = GNUTLS_KX_ECDHE_RSA
	},
	{.name = "tls1.2 rsa-pss-sign key",
	 .pk = GNUTLS_PK_RSA_PSS,
	 .prio = "NORMAL:+ECDHE-RSA:+ECDHE-ECDSA:-VERS-ALL:+VERS-TLS1.2",
	 .cert = &server_ca3_rsa_pss2_cert,
	 .key = &server_ca3_rsa_pss2_key,
	 .sig = GNUTLS_SIGN_RSA_PSS_SHA256,
	 .exp_kx = GNUTLS_KX_ECDHE_RSA,
	},
	{.name = "tls1.2 rsa-pss cert, rsa-sign key", /* we expect the server to refuse negotiating */
	 .pk = GNUTLS_PK_RSA,
	 .prio = "NORMAL:+ECDHE-RSA:+ECDHE-ECDSA:-VERS-ALL:+VERS-TLS1.2",
	 .cert = &server_ca3_rsa_pss_cert,
	 .key = &server_ca3_rsa_pss_key,
	 .exp_kx = GNUTLS_KX_ECDHE_RSA,
	 .exp_serv_err = GNUTLS_E_NO_CIPHER_SUITES
	},
	{.name = "tls1.2 ed25519 cert, ed25519 key",
	 .pk = GNUTLS_PK_EDDSA_ED25519,
	 .prio = "NORMAL:+ECDHE-RSA:+ECDHE-ECDSA:-VERS-ALL:+VERS-TLS1.2",
	 .cert = &server_ca3_eddsa_cert,
	 .key = &server_ca3_eddsa_key,
	 .sig = GNUTLS_SIGN_EDDSA_ED25519,
	 .exp_kx = GNUTLS_KX_ECDHE_ECDSA,
	},
	{.name = "tls1.2 rsa-decrypt key",
	 .pk = GNUTLS_PK_RSA,
	 .prio = "NORMAL:-KX-ALL:+RSA:-VERS-ALL:+VERS-TLS1.2",
	 .cert = &server_ca3_localhost_cert,
	 .key = &server_ca3_key,
	 .exp_kx = GNUTLS_KX_RSA
	},
	{.name = "tls1.3 ecc key",
	 .pk = GNUTLS_PK_ECDSA,
	 .prio = "NORMAL:-VERS-ALL:+VERS-TLS1.3",
	 .cert = &server_ca3_localhost_ecc_cert,
	 .key = &server_ca3_ecc_key,
	 .sig = GNUTLS_SIGN_ECDSA_SECP256R1_SHA256,
	 .exp_kx = GNUTLS_KX_ECDHE_RSA
	},
	{.name = "tls1.3 rsa-sign key",
	 .pk = GNUTLS_PK_RSA,
	 .prio = "NORMAL:-VERS-ALL:+VERS-TLS1.3",
	 .cert = &server_ca3_localhost_cert,
	 .key = &server_ca3_key,
	 .sig = GNUTLS_SIGN_RSA_PSS_RSAE_SHA256,
	 .exp_kx = GNUTLS_KX_ECDHE_RSA
	},
	{.name = "tls1.3 rsa-pss-sign key",
	 .pk = GNUTLS_PK_RSA_PSS,
	 .prio = "NORMAL:-VERS-ALL:+VERS-TLS1.3",
	 .cert = &server_ca3_rsa_pss2_cert,
	 .key = &server_ca3_rsa_pss2_key,
	 .sig = GNUTLS_SIGN_RSA_PSS_SHA256,
	 .exp_kx = GNUTLS_KX_ECDHE_RSA,
	},
	{.name = "tls1.3 rsa-pss cert, rsa-sign key", /* we expect the server to attempt to downgrade to TLS 1.2, but it is not possible because it is not enabled */
	 .pk = GNUTLS_PK_RSA,
	 .prio = "NORMAL:-VERS-ALL:+VERS-TLS1.3",
	 .cert = &server_ca3_rsa_pss_cert,
	 .key = &server_ca3_rsa_pss_key,
	 .sig = GNUTLS_SIGN_RSA_SHA256,
	 .exp_kx = GNUTLS_KX_ECDHE_RSA,
	 .exp_serv_err = GNUTLS_E_NO_CIPHER_SUITES
	},
	{.name = "tls1.3 rsa-pss cert, rsa-sign key, downgrade to tls1.2", /* we expect the server to downgrade to TLS 1.2 and refuse negotiating */
	 .pk = GNUTLS_PK_RSA,
	 .prio = "NORMAL:-VERS-ALL:+VERS-TLS1.3:+VERS-TLS1.2",
	 .cert = &server_ca3_rsa_pss_cert,
	 .key = &server_ca3_rsa_pss_key,
	 .sig = GNUTLS_SIGN_RSA_SHA256,
	 .exp_kx = GNUTLS_KX_ECDHE_RSA,
	 .exp_serv_err = GNUTLS_E_NO_CIPHER_SUITES
	},
	{.name = "tls1.3 ed25519 cert, ed25519 key",
	 .pk = GNUTLS_PK_EDDSA_ED25519,
	 .prio = "NORMAL:-VERS-ALL:+VERS-TLS1.3",
	 .cert = &server_ca3_eddsa_cert,
	 .key = &server_ca3_eddsa_key,
	 .sig = GNUTLS_SIGN_EDDSA_ED25519,
	 .exp_kx = GNUTLS_KX_ECDHE_RSA,
	}
};

void doit(void)
{
	gnutls_privkey_t privkey;
	unsigned int i;

	global_init();

	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(6);

	for (i=0;i<sizeof(tests)/sizeof(tests[0]);i++) {
		success("checking: %s\n", tests[i].name);

		privkey = load_virt_privkey(tests[i].name, tests[i].key, tests[i].pk,
					    tests[i].sig,
					    tests[i].exp_key_err);
		if (privkey == NULL && tests[i].exp_key_err < 0)
			continue;
		assert(privkey != 0);

		try_with_key(tests[i].name, tests[i].prio,
			     tests[i].exp_kx, 0, 0,
			     tests[i].cert, privkey,
			     tests[i].exp_serv_err);
	}

	gnutls_global_deinit();
}
