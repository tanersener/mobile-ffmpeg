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
#include <gnutls/abstract.h>
#include "utils.h"
#include "eagain-common.h"
#include "cert-common.h"

const char *side;

/* This test checks whether a key usage violation is detected when
 * using a certificate for digital signatures in the RSA ciphersuites.
 */

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "%s|<%d>| %s", side, level, str);
}

static unsigned char encryption_cert_pem[] =
	"-----BEGIN CERTIFICATE-----\n"
	"MIIDoTCCAgmgAwIBAgIIWD7Wvx22i+gwDQYJKoZIhvcNAQELBQAwDzENMAsGA1UE\n"
	"AxMEQ0EtMzAgFw0xNjExMzAxMzQwMTZaGA85OTk5MTIzMTIzNTk1OVowADCCASIw\n"
	"DQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAM3XiAz9NK/9K4mciW5cioUfOrH8\n"
	"W5QlnzgODc5O9vKypx+2Y42BmVArdTNox9ypyQHs4Tf1RVs8MkKLLRPVPvFTTwsB\n"
	"sYYR0WwtjLaUAG6uEQOkQ1tKnkPveR+7Yaz/WurUTFH/6tt9PLkjUa2MFClJfQyA\n"
	"+Ip0DOChfZVWDmKEsGxf0+HDrUwI6Yrue6Xjq4MtQ644vxYuIZrEU53bExNrZ7y9\n"
	"fvwsYa86eNBO3lEierVnusFqvngsXzuhHMTh7Dd1kdewWnNX9cFyXFPU1oxpEqgD\n"
	"9b/WOELpt4/Vyi6GAKthroTADOrgqIS4yVv/IwTE+I75820inSJBXwpVi9sCAwEA\n"
	"AaOBjTCBijAMBgNVHRMBAf8EAjAAMBQGA1UdEQQNMAuCCWxvY2FsaG9zdDATBgNV\n"
	"HSUEDDAKBggrBgEFBQcDATAPBgNVHQ8BAf8EBQMDByAAMB0GA1UdDgQWBBThAci6\n"
	"ST9MfTP8KV4xkB0p2hgsyjAfBgNVHSMEGDAWgBT5qIYZY7akFBNgdg8BmjU27/G0\n"
	"rzANBgkqhkiG9w0BAQsFAAOCAYEAQSaXhGYE0VvpFidR+txfhRXZhoIyO6bCxrOE\n"
	"WFTdglZ3XE9/avlONa18hAVmMqBXJFKiTIMou2qQu7dJ80dMphQPFSOgVTwNP5yD\n"
	"MM0iJHSlcBweukA3+Jfo3lbGSYOGh3D157XwPQ5+dKFSgzFWdQApDAZ2Y5wg1mlD\n"
	"riapOliMXEBHuKaBEAGYHLNQEUoutc/8lpv7FrE8YPp2J5f/kBlL21ygHNCNbRQZ\n"
	"XTTajRgY5dg0R7CPM1wkyk/K1Lke2BgteF4FWlKTzh3b42swWJAlW9oDcqA8xRHu\n"
	"cvU+7PKs3SpXky6dGC+rgWMfV99z00gNICdZJrqTRTd6JvMa+Q8QCChHtyE40LWe\n"
	"MXFfeQW2kWD+q2CUAiY5K/fk4p74w4TtHuln3/+IZd+fwMfq9eD9524n+61AoTvm\n"
	"FM9vezUEwybmHVTx+390aiY2SaAxl4BCopauOgpBTnj8Rcd5dMO3qEW4+QaXKMlU\n"
	"wIEPoaEfCDQ/XXy0bM5zFUFWgTNX\n"
	"-----END CERTIFICATE-----\n";

const gnutls_datum_t enc_cert = { encryption_cert_pem,
	sizeof(encryption_cert_pem)-1
};

static unsigned char encryption_key_pem[] =
	"-----BEGIN RSA PRIVATE KEY-----\n"
	"MIIEpgIBAAKCAQEAzdeIDP00r/0riZyJblyKhR86sfxblCWfOA4Nzk728rKnH7Zj\n"
	"jYGZUCt1M2jH3KnJAezhN/VFWzwyQostE9U+8VNPCwGxhhHRbC2MtpQAbq4RA6RD\n"
	"W0qeQ+95H7thrP9a6tRMUf/q2308uSNRrYwUKUl9DID4inQM4KF9lVYOYoSwbF/T\n"
	"4cOtTAjpiu57peOrgy1Drji/Fi4hmsRTndsTE2tnvL1+/Cxhrzp40E7eUSJ6tWe6\n"
	"wWq+eCxfO6EcxOHsN3WR17Bac1f1wXJcU9TWjGkSqAP1v9Y4Qum3j9XKLoYAq2Gu\n"
	"hMAM6uCohLjJW/8jBMT4jvnzbSKdIkFfClWL2wIDAQABAoIBAQC70D11xI6PSUux\n"
	"St/mj49gOYdfoOeaO92T0tbr+AbAmRt+Bve8xJQznwNX/fHmOBCMriss2KEIxtsA\n"
	"9mYR44+Dt8S2QTxOHPHdZ44thMsEMdSaYwWGRYY0bEszFdDgfTnibASbCQusaw+9\n"
	"ySkcVWSL616qyv57rbmWOCMS4CtN3Sk982mtzSdCkJ8tiq6n3C60QPom/zo5TBS5\n"
	"vaJ70NRnj7Zuq9VPwNKOwhkYW9OUZsAmdwLqenmsLfQEnZnu/ielJ10LI8SrQG5x\n"
	"lANdYRD07W5lpwImJCELUqK5X2iw5ii6/4vl/Si/WcL4pRFpuCOCp1B8SDuSkOKS\n"
	"zebU/Z3hAoGBAPvIN/WlSQ+Iy5TNGsnV5B96Xvl8YrXVInJZ7z4MOrPgyvN8mQXX\n"
	"sQ6D01H2tba3mWt0S16lWwBsOll5LDBj5kcvp+4702xUxoOap79wXPS1Ibi+uXlO\n"
	"5c7V3pa7r2nw7YQL+ehYpgBdaVaYdAnHKn0Mo7zMd+yjNnQEfEcDwNFxAoGBANFK\n"
	"S7y327IEms1wdn0hb1r812PKsn464j4xbnfnrAYzE2cttgLSYsRRYNMo++ZS9Y3v\n"
	"3MZGmgOsKRgpbblxhUxNY5pKeHcXKUy1YtaGJVpeQwI8u69Th9tUDS2/yt7Op4/0\n"
	"p5115DTEfmvKzF//PH7GtX5Ox/JoNSHaPcORT0wLAoGBANXYEZ8zCMCG4NG6+hue\n"
	"7KfHmU6wVG43XZBdzhKW9Gy+aeEvXBBYR2saj6q3rVJI0acwGKuEKaxMP6qqfduD\n"
	"nZusYCa47TK/NfOksQCpgGneRYvRgVoEpq5reyfutGd4V2KlgVXTpPn+XG9OAJAl\n"
	"dnLK/25lAx4a6S7UeHEgQO4hAoGBAKyfch6jK3MGd0RxuVl2RWmv2Fw36MdS/B6+\n"
	"GNaPYITwhdV5j4F+U/aHBKzGRhbwYBcFO3zS6N+UlYSXTyhAqOiJgFjXicr4cJkT\n"
	"lwVIOfDyhKSIwWlYJVtTVVdhtQvXOb/z1Hh8r5CSbY+tAqs/U39hmHsosaSQLRrR\n"
	"7lWrOdOHAoGBAIndZqW8HHfUk5Y6ZlbDzz/GRi81nrU3p2Ii1M17PLFyFhKZcPyM\n"
	"kJDhqStyWEQKN7Xig0uxGvGAFYTBsILmoS/XAFnRpfcmNkF7hXRGHuHFRopZuIic\n"
	"gZ9oloj50/wHdTSU/MExRExhC7DUom2DzihUz3a5uqWOK/SnpfNeIJPs\n"
	"-----END RSA PRIVATE KEY-----\n";
const gnutls_datum_t enc_key = { encryption_key_pem,
	sizeof(encryption_key_pem)-1
};

static
void server_check(void)
{
	int exit_code = EXIT_SUCCESS;
	int ret;
	/* Server stuff. */
	gnutls_certificate_credentials_t serverx509cred;
	gnutls_session_t server;
	int sret = GNUTLS_E_AGAIN;
	/* Client stuff. */
	gnutls_certificate_credentials_t clientx509cred;
	gnutls_session_t client;
	int cret = GNUTLS_E_AGAIN;

	/* General init. */
	global_init();
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(6);

	/* Init server */
	gnutls_certificate_allocate_credentials(&serverx509cred);
	gnutls_certificate_set_x509_key_mem(serverx509cred,
					    &enc_cert, &enc_key,
					    GNUTLS_X509_FMT_PEM);

	gnutls_init(&server, GNUTLS_SERVER);
	gnutls_credentials_set(server, GNUTLS_CRD_CERTIFICATE,
				serverx509cred);
	gnutls_priority_set_direct(server,
				   "NORMAL:-KX-ALL:+ECDHE-RSA",
				   NULL);
	gnutls_transport_set_push_function(server, server_push);
	gnutls_transport_set_pull_function(server, server_pull);
	gnutls_transport_set_ptr(server, server);

	/* Init client */
	ret = gnutls_certificate_allocate_credentials(&clientx509cred);
	if (ret < 0)
		exit(1);

	ret = gnutls_certificate_set_x509_trust_mem(clientx509cred, &ca_cert, GNUTLS_X509_FMT_PEM);
	if (ret < 0)
		exit(1);

	ret = gnutls_init(&client, GNUTLS_CLIENT);
	if (ret < 0)
		exit(1);

	ret = gnutls_credentials_set(client, GNUTLS_CRD_CERTIFICATE,
				clientx509cred);
	if (ret < 0)
		exit(1);

	gnutls_priority_set_direct(client, "NORMAL:+ECDHE-RSA", NULL);
	gnutls_transport_set_push_function(client, client_push);
	gnutls_transport_set_pull_function(client, client_pull);
	gnutls_transport_set_ptr(client, client);

	HANDSHAKE_EXPECT(client, server, GNUTLS_E_AGAIN, GNUTLS_E_NO_CIPHER_SUITES);

	if (debug)
		success("server returned the expected code\n");

	gnutls_deinit(client);
	gnutls_deinit(server);

	gnutls_certificate_free_credentials(serverx509cred);
	gnutls_certificate_free_credentials(clientx509cred);

	gnutls_global_deinit();

	if (debug > 0) {
		if (exit_code == 0)
			puts("Self-test successful");
		else
			puts("Self-test failed");
	}
}

static gnutls_privkey_t g_pkey = NULL;
static gnutls_pcert_st *g_pcert = NULL;

static int
cert_callback(gnutls_session_t session,
	      const gnutls_datum_t * req_ca_rdn, int nreqs,
	      const gnutls_pk_algorithm_t * sign_algos,
	      int sign_algos_length, gnutls_pcert_st ** pcert,
	      unsigned int *pcert_length, gnutls_privkey_t * pkey)
{
	int ret;
	gnutls_pcert_st *p;
	gnutls_privkey_t lkey;

	if (g_pkey == NULL) {
		p = gnutls_malloc(sizeof(*p));
		if (p==NULL)
			return -1;

		ret = gnutls_pcert_import_x509_raw(p, &enc_cert, GNUTLS_X509_FMT_PEM, 0);
		if (ret < 0)
			return -1;

		ret = gnutls_privkey_init(&lkey);
		if (ret < 0)
			return -1;

		ret = gnutls_privkey_import_x509_raw(lkey, &enc_key, GNUTLS_X509_FMT_PEM, NULL, 0);
		if (ret < 0)
			return -1;

		g_pcert = p;
		g_pkey = lkey;

		*pcert = p;
		*pcert_length = 1;
	} else {
		*pcert = g_pcert;
		*pcert_length = 1;
	}
	*pkey = g_pkey;

	return 0;
}

static
void client_check(void)
{
	int exit_code = EXIT_SUCCESS;
	int ret;
	/* Server stuff. */
	gnutls_certificate_credentials_t serverx509cred;
	gnutls_session_t server;
	int sret = GNUTLS_E_AGAIN;
	/* Client stuff. */
	gnutls_certificate_credentials_t clientx509cred;
	gnutls_session_t client;
	int cret = GNUTLS_E_AGAIN;

	/* General init. */
	global_init();
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(6);

	/* Init server */
	gnutls_certificate_allocate_credentials(&serverx509cred);
	gnutls_certificate_set_retrieve_function2(serverx509cred, cert_callback);

	gnutls_init(&server, GNUTLS_SERVER);
	gnutls_credentials_set(server, GNUTLS_CRD_CERTIFICATE,
				serverx509cred);
	gnutls_priority_set_direct(server,
				   "NORMAL:-KX-ALL:+ECDHE-RSA:%DEBUG_ALLOW_KEY_USAGE_VIOLATIONS",
				   NULL);
	gnutls_transport_set_push_function(server, server_push);
	gnutls_transport_set_pull_function(server, server_pull);
	gnutls_transport_set_ptr(server, server);

	/* Init client */
	ret = gnutls_certificate_allocate_credentials(&clientx509cred);
	if (ret < 0)
		exit(1);

	ret = gnutls_certificate_set_x509_trust_mem(clientx509cred, &ca_cert, GNUTLS_X509_FMT_PEM);
	if (ret < 0)
		exit(1);

	ret = gnutls_init(&client, GNUTLS_CLIENT);
	if (ret < 0)
		exit(1);

	ret = gnutls_credentials_set(client, GNUTLS_CRD_CERTIFICATE,
				clientx509cred);
	if (ret < 0)
		exit(1);

	gnutls_priority_set_direct(client, "NORMAL:+ECDHE-RSA", NULL);
	gnutls_transport_set_push_function(client, client_push);
	gnutls_transport_set_pull_function(client, client_pull);
	gnutls_transport_set_ptr(client, client);

	HANDSHAKE_EXPECT(client, server, GNUTLS_E_KEY_USAGE_VIOLATION, GNUTLS_E_AGAIN);

	if (debug)
		success("client returned the expected code: %s\n", gnutls_strerror(cret));

	gnutls_deinit(client);
	gnutls_deinit(server);

	gnutls_certificate_free_credentials(serverx509cred);
	gnutls_certificate_free_credentials(clientx509cred);

	gnutls_global_deinit();

	if (debug > 0) {
		if (exit_code == 0)
			puts("Self-test successful");
		else
			puts("Self-test failed");
	}
}

void doit(void)
{
	server_check();
	reset_buffers();
	client_check();

	if (g_pcert) {
		gnutls_pcert_deinit(g_pcert);
		gnutls_free(g_pcert);
	}
	if (g_pkey) {
		gnutls_privkey_deinit(g_pkey);
	}
}
