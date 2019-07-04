/*
 * Copyright (C) 2017 Nikos Mavrogiannopoulos
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
#include "softhsm.h"

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "<%d> %s", level, str);
}

#define CONFIG_NAME "softhsm-neg"
#define CONFIG CONFIG_NAME".config"
#define PIN "1234"

#define testfail(fmt, ...) \
	fail("%s: "fmt, name, ##__VA_ARGS__)

static unsigned verify_eddsa_presence(void)
{
	unsigned i;
	unsigned long mechanism;
	int ret;

	i = 0;
	do {
		ret = gnutls_pkcs11_token_get_mechanism("pkcs11:", i++, &mechanism);
		if (ret >= 0 && mechanism == 0x1057 /* CKM_EDDSA */)
			return 1;
	} while(ret>=0);

	return 0;
}

static gnutls_privkey_t load_virt_privkey(const char *name, const gnutls_datum_t *txtkey,
					  int exp_key_err, unsigned needs_decryption)
{
	unsigned flags;
	gnutls_privkey_t privkey;
	gnutls_x509_privkey_t tmp;
	int ret;

	ret = gnutls_x509_privkey_init(&tmp);
	if (ret < 0)
		testfail("gnutls_privkey_init\n");

	ret = gnutls_x509_privkey_import(tmp, txtkey, GNUTLS_X509_FMT_PEM);
	if (ret < 0)
		testfail("gnutls_privkey_import: %s\n", gnutls_strerror(ret));

	if (needs_decryption)
		flags = GNUTLS_KEY_KEY_ENCIPHERMENT;
	else
		flags = GNUTLS_KEY_DIGITAL_SIGNATURE;

	ret = gnutls_pkcs11_copy_x509_privkey(SOFTHSM_URL, tmp, "key", flags,
					      GNUTLS_PKCS11_OBJ_FLAG_MARK_PRIVATE|GNUTLS_PKCS11_OBJ_FLAG_MARK_SENSITIVE|GNUTLS_PKCS11_OBJ_FLAG_LOGIN);
	gnutls_x509_privkey_deinit(tmp);

	if (ret < 0) {
		if (ret == exp_key_err) {
			return NULL;
		}
		fail("gnutls_pkcs11_copy_x509_privkey: %s\n", gnutls_strerror(ret));
	}

	ret = gnutls_privkey_init(&privkey);
	if (ret < 0)
		testfail("gnutls_privkey_init\n");

	ret =
	    gnutls_privkey_import_url(privkey, SOFTHSM_URL";object=key", 0);
	if (ret < 0) {
		if (ret == exp_key_err) {
			gnutls_privkey_deinit(privkey);
			return NULL;
		}
		testfail("gnutls_privkey_import: %s\n", gnutls_strerror(ret));
	}

	if (exp_key_err) {
		testfail("did not fail in key import, although expected\n");
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

	assert(gnutls_priority_set_direct(server,
					  "NORMAL:+VERS-SSL3.0:+ANON-ECDH:+ANON-DH:+ECDHE-RSA:+DHE-RSA:+RSA:+ECDHE-ECDSA:+CURVE-X25519:+SIGN-EDDSA-ED25519",
					  NULL) >= 0);
	gnutls_transport_set_push_function(server, server_push);
	gnutls_transport_set_pull_function(server, server_pull);
	gnutls_transport_set_ptr(server, server);

	/* Init client */

	ret = gnutls_certificate_allocate_credentials(&c_xcred);
	if (ret < 0)
		exit(1);

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
		testfail("%s: got unexpected key exchange algorithm: %s (expected %s)\n", name, gnutls_kx_get_name(gnutls_kx_get(client)),
			gnutls_kx_get_name(client_kx));
		exit(1);
	}

	/* test signature algorithm match */
	version = gnutls_protocol_get_version(client);
	if (version >= GNUTLS_TLS1_2) {
		ret = gnutls_sign_algorithm_get(server);
		if (ret != (int)server_sign_algo && server_sign_algo != 0) {
			testfail("%s: got unexpected server signature algorithm: %d/%s\n", name, ret, gnutls_sign_get_name(ret));
			exit(1);
		}

		ret = gnutls_sign_algorithm_get_client(server);
		if (ret != (int)client_sign_algo && client_sign_algo != 0) {
			testfail("%s: got unexpected client signature algorithm: %d/%s\n", name, ret, gnutls_sign_get_name(ret));
			exit(1);
		}

		ret = gnutls_sign_algorithm_get(client);
		if (ret != (int)server_sign_algo && server_sign_algo != 0) {
			testfail("%s: cl: got unexpected server signature algorithm: %d/%s\n", name, ret, gnutls_sign_get_name(ret));
			exit(1);
		}

		ret = gnutls_sign_algorithm_get_client(client);
		if (ret != (int)client_sign_algo && client_sign_algo != 0) {
			testfail("%s: cl: got unexpected client signature algorithm: %d/%s\n", name, ret, gnutls_sign_get_name(ret));
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
	int exp_key_err;
	int exp_serv_err;
	int needs_eddsa;
	int needs_decryption;
	unsigned requires_pkcs11_pss;
} test_st;

static const test_st tests[] = {
	{.name = "tls1.2: rsa-decryption key",
	 .pk = GNUTLS_PK_RSA,
	 .prio = "NORMAL:-KX-ALL:+RSA:-VERS-TLS-ALL:+VERS-TLS1.2",
	 .cert = &server_ca3_localhost_rsa_decrypt_cert,
	 .key = &server_ca3_key,
	 .exp_kx = GNUTLS_KX_RSA,
	 .needs_decryption = 1
	},
	{.name = "tls1.2: rsa-decryption key, signatures prioritized",
	 .pk = GNUTLS_PK_RSA,
	 .prio = "NORMAL:-KX-ALL:+ECDHE-RSA:+RSA:-VERS-TLS-ALL:+VERS-TLS1.2:-SIGN-ALL:+SIGN-RSA-PSS-RSAE-SHA256",
	 .cert = &server_ca3_localhost_cert,
	 .key = &server_ca3_key,
	 .exp_kx = GNUTLS_KX_RSA,
	 .needs_decryption = 1
	},
	{.name = "tls1.2: ecc key",
	 .pk = GNUTLS_PK_ECDSA,
	 .prio = "NORMAL:-KX-ALL:+ECDHE-RSA:+ECDHE-ECDSA:-VERS-TLS-ALL:+VERS-TLS1.2",
	 .cert = &server_ca3_localhost_ecc_cert,
	 .key = &server_ca3_ecc_key,
	 .exp_kx = GNUTLS_KX_ECDHE_ECDSA
	},
	{.name = "tls1.2: rsa-sign key",
	 .pk = GNUTLS_PK_RSA,
	 .prio = "NORMAL:+ECDHE-RSA:+ECDHE-ECDSA:-VERS-TLS-ALL:+VERS-TLS1.2",
	 .cert = &server_ca3_localhost_cert,
	 .key = &server_ca3_key,
	 .exp_kx = GNUTLS_KX_ECDHE_RSA
	},
	{.name = "tls1.2: rsa-sign key with rsa-pss sigs prioritized",
	 .pk = GNUTLS_PK_RSA,
	 .prio = "NORMAL:+ECDHE-RSA:+ECDHE-ECDSA:-SIGN-ALL:+SIGN-RSA-PSS-SHA256:+SIGN-RSA-PSS-SHA384:+SIGN-RSA-PSS-SHA512:+SIGN-RSA-SHA256:+SIGN-RSA-SHA384:+SIGN-RSA-SHA512:-VERS-TLS-ALL:+VERS-TLS1.2",
	 .cert = &server_ca3_localhost_cert,
	 .key = &server_ca3_key,
	 .exp_kx = GNUTLS_KX_ECDHE_RSA
	},
	{.name = "tls1.2: rsa-pss-sign key",
	 .pk = GNUTLS_PK_RSA_PSS,
	 .prio = "NORMAL:+ECDHE-RSA:+ECDHE-ECDSA:-VERS-TLS-ALL:+VERS-TLS1.2",
	 .cert = &server_ca3_rsa_pss2_cert,
	 .key = &server_ca3_rsa_pss2_key,
	 .exp_kx = GNUTLS_KX_ECDHE_RSA,
	 .requires_pkcs11_pss = 1,
	},
	{.name = "tls1.2: rsa-pss cert, rsa-sign key",
	 .pk = GNUTLS_PK_RSA,
	 .prio = "NORMAL:+ECDHE-RSA:+ECDHE-ECDSA:-VERS-TLS-ALL:+VERS-TLS1.2",
	 .cert = &server_ca3_rsa_pss_cert,
	 .key = &server_ca3_rsa_pss_key,
	 .exp_kx = GNUTLS_KX_ECDHE_RSA,
	 .requires_pkcs11_pss = 1,
	},
	{.name = "tls1.2: rsa-pss cert, rsa-sign key no PSS signatures",
	 .pk = GNUTLS_PK_RSA,
	 .prio = "NORMAL:+ECDHE-RSA:+ECDHE-ECDSA:-VERS-TLS-ALL:+VERS-TLS1.2:-SIGN-RSA-PSS-SHA256:-SIGN-RSA-PSS-SHA384:-SIGN-RSA-PSS-SHA512:-SIGN-RSA-PSS-RSAE-SHA256:-SIGN-RSA-PSS-RSAE-SHA384:-SIGN-RSA-PSS-RSAE-SHA512",
	 .cert = &server_ca3_rsa_pss_cert,
	 .key = &server_ca3_rsa_pss_key,
	 .exp_kx = GNUTLS_KX_ECDHE_RSA,
	 .exp_serv_err = GNUTLS_E_NO_CIPHER_SUITES
	},
	{.name = "tls1.2: ed25519 cert, ed25519 key",
	 .pk = GNUTLS_PK_EDDSA_ED25519,
	 .needs_eddsa = 1,
	 .prio = "NORMAL:+ECDHE-RSA:+ECDHE-ECDSA",
	 .cert = &server_ca3_eddsa_cert,
	 .key = &server_ca3_eddsa_key,
	 .exp_kx = GNUTLS_KX_ECDHE_RSA,
	},
	{.name = "tls1.3: ecc key",
	 .pk = GNUTLS_PK_ECDSA,
	 .prio = "NORMAL:-KX-ALL:+ECDHE-RSA:+ECDHE-ECDSA:-VERS-TLS-ALL:+VERS-TLS1.3",
	 .cert = &server_ca3_localhost_ecc_cert,
	 .key = &server_ca3_ecc_key,
	 .exp_kx = GNUTLS_KX_ECDHE_RSA
	},
	{.name = "tls1.3: rsa-sign key",
	 .pk = GNUTLS_PK_RSA,
	 .prio = "NORMAL:+ECDHE-RSA:+ECDHE-ECDSA:-VERS-TLS-ALL:+VERS-TLS1.3",
	 .cert = &server_ca3_localhost_cert,
	 .key = &server_ca3_key,
	 .exp_kx = GNUTLS_KX_ECDHE_RSA
	},
	{.name = "tls1.3: rsa-sign key with rsa-pss sigs prioritized",
	 .pk = GNUTLS_PK_RSA,
	 .prio = "NORMAL:-SIGN-ALL:+SIGN-RSA-PSS-SHA256:+SIGN-RSA-PSS-SHA384:+SIGN-RSA-PSS-SHA512:+SIGN-RSA-PSS-RSAE-SHA256:+SIGN-RSA-PSS-RSAE-SHA384:+SIGN-RSA-PSS-RSAE-SHA512:-VERS-TLS-ALL:+VERS-TLS1.3",
	 .cert = &server_ca3_localhost_cert,
	 .key = &server_ca3_key,
	 .exp_kx = GNUTLS_KX_ECDHE_RSA
	},
	{.name = "tls1.3: rsa-pss-sign key",
	 .pk = GNUTLS_PK_RSA_PSS,
	 .prio = "NORMAL:+ECDHE-RSA:+ECDHE-ECDSA:-VERS-TLS-ALL:+VERS-TLS1.3",
	 .cert = &server_ca3_rsa_pss2_cert,
	 .key = &server_ca3_rsa_pss2_key,
	 .exp_kx = GNUTLS_KX_ECDHE_RSA,
	 .requires_pkcs11_pss = 1,
	},
	{.name = "tls1.3: rsa-pss cert, rsa-sign key",
	 .pk = GNUTLS_PK_RSA,
	 .prio = "NORMAL:+ECDHE-RSA:+ECDHE-ECDSA:-VERS-TLS-ALL:+VERS-TLS1.3",
	 .cert = &server_ca3_rsa_pss_cert,
	 .key = &server_ca3_rsa_pss_key,
	 .exp_kx = GNUTLS_KX_ECDHE_RSA,
	 .requires_pkcs11_pss = 1,
	},
	{.name = "tls1.3: rsa-pss cert, rsa-sign key no PSS signatures",
	 .pk = GNUTLS_PK_RSA,
	 .prio = "NORMAL:+ECDHE-RSA:+ECDHE-ECDSA:-VERS-TLS-ALL:+VERS-TLS1.3:-SIGN-RSA-PSS-SHA256:-SIGN-RSA-PSS-SHA384:-SIGN-RSA-PSS-SHA512:-SIGN-RSA-PSS-RSAE-SHA256:-SIGN-RSA-PSS-RSAE-SHA384:-SIGN-RSA-PSS-RSAE-SHA512",
	 .cert = &server_ca3_rsa_pss_cert,
	 .key = &server_ca3_rsa_pss_key,
	 .exp_kx = GNUTLS_KX_ECDHE_RSA,
	 .exp_serv_err = GNUTLS_E_NO_CIPHER_SUITES
	},
	{.name = "tls1.3: ed25519 cert, ed25519 key",
	 .needs_eddsa = 1,
	 .pk = GNUTLS_PK_EDDSA_ED25519,
	 .prio = "NORMAL:+ECDHE-RSA:+ECDHE-ECDSA",
	 .cert = &server_ca3_eddsa_cert,
	 .key = &server_ca3_eddsa_key,
	 .exp_kx = GNUTLS_KX_ECDHE_RSA
	}
};

static
int pin_func(void* userdata, int attempt, const char* url, const char *label,
		unsigned flags, char *pin, size_t pin_max)
{
	if (attempt == 0) {
		strcpy(pin, PIN);
		return 0;
	}
	return -1;
}

#ifndef CKM_RSA_PKCS_PSS
# define CKM_RSA_PKCS_PSS (0xdUL)
#endif

void doit(void)
{
	gnutls_privkey_t privkey;
	const char *bin, *lib;
	char buf[512];
	unsigned int i, have_eddsa;
	int ret;

#ifdef _WIN32
	exit(77);
#endif
	bin = softhsm_bin();

	lib = softhsm_lib();

	global_init();

	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(6);


	/* initialize token */
	gnutls_pkcs11_set_pin_function(pin_func, NULL);
	set_softhsm_conf(CONFIG);
	snprintf(buf, sizeof(buf), "%s --init-token --slot 0 --label test --so-pin "PIN" --pin "PIN, bin);
	system(buf);

	ret = gnutls_pkcs11_add_provider(lib, NULL);
	if (ret < 0) {
		fail("gnutls_pkcs11_add_provider: %s\n",
			gnutls_strerror(ret));
	}

	have_eddsa = verify_eddsa_presence();

	for (i=0;i<sizeof(tests)/sizeof(tests[0]);i++) {
		if (tests[i].needs_eddsa && !have_eddsa)
			continue;

		success("checking: %s\n", tests[i].name);

		if (tests[i].requires_pkcs11_pss) {
			ret = gnutls_pkcs11_token_check_mechanism("pkcs11:", CKM_RSA_PKCS_PSS, NULL, 0, 0);
			if (ret == 0) {
				fprintf(stderr, "softhsm2 doesn't support CKM_RSA_PKCS_PSS; skipping test\n");
				continue;
			}
		}

		privkey = load_virt_privkey(tests[i].name, tests[i].key, tests[i].exp_key_err, tests[i].needs_decryption);
		if (privkey == NULL && tests[i].exp_key_err < 0)
			continue;
		assert(privkey != 0);

		try_with_key(tests[i].name, tests[i].prio,
			     tests[i].exp_kx, 0, 0,
			     tests[i].cert, privkey,
			     tests[i].exp_serv_err);

		gnutls_pkcs11_delete_url(SOFTHSM_URL";object=key", GNUTLS_PKCS11_OBJ_FLAG_LOGIN);
		remove(CONFIG);
	}

	gnutls_global_deinit();
}
