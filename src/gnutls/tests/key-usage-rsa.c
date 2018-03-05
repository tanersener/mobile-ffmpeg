/*
 * Copyright (C) 2015-2016 Red Hat, Inc.
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

const char *side;

/* This test checks whether a key usage violation is detected when
 * using a certificate for digital signatures in the RSA ciphersuites.
 */

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "%s|<%d>| %s", side, level, str);
}

static unsigned char ca_cert_pem[] =
"-----BEGIN CERTIFICATE-----\n"
"MIIC8zCCAdugAwIBAgIBADANBgkqhkiG9w0BAQsFADAPMQ0wCwYDVQQDEwRDQS0w\n"
"MCAXDTE1MDgxNDA5MzUxMVoYDzk5OTkxMjMxMjM1OTU5WjAPMQ0wCwYDVQQDEwRD\n"
"QS0wMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4O2BYEx/hl7URXhb\n"
"52erURO6HrlfacZjG0fQ2WqRcJJTqg2baAbA7+1SLdhphZ+KJDypEjJWmOgbaehI\n"
"hlK7zDZb+0r0uXlUQ11mgf7FcCDQoJBmC1dcN3o6zPeXg5hkWV+ZV2h7hhJTwkRc\n"
"C4DXTbaDKy8PNiC0MRMOGjeMfnL26oaxzuHNgH4u1J02+XUZ0UcSDrUc52O1lJ02\n"
"i1SbD+fTNBgmFQADXyAllZYJ/xwbxf44TFhQjiOvVpz/9EB2+/x5H0r1YvwKGY6v\n"
"5mfkUsEAE5+uxDXdZT84ltEKkAjbZ9cIgdmXRuD4mkyo3NHLh7oHCdsRRE/S/rZe\n"
"ikmGpQIDAQABo1gwVjAPBgNVHRMBAf8EBTADAQH/MBMGA1UdJQQMMAoGCCsGAQUF\n"
"BwMJMA8GA1UdDwEB/wQFAwMHBgAwHQYDVR0OBBYEFEvjsNoFTfqDEKbcwFnxKId+\n"
"ZQP8MA0GCSqGSIb3DQEBCwUAA4IBAQAKrbc6hER0xAjn5driLNyoz0JJr5P07PDI\n"
"d8AR3ZC56DSJNdvKDqdFIvAoo/JePCTFSdhbaqu+08MoTtRK5TKqjRiDiG4XCxiz\n"
"Ado7QouS+ZgDP1Uxv8j2YWeSpkusD+oIEK96wbeDaYi0ENbLWbm9zWqvHaaEYn4c\n"
"ov78n+7VvP3I2OFuJ0EPy+r55GPxSCRCh6apL78yAc6TfcyOwwTihvCF5ejCqRg/\n"
"T1As5NCCpdYP2nejRymjO6wMRsRFBX9+gndO9qVQZJr8zBTw8k8/pMtDubjkYqEv\n"
"qRME4/3q8+Sm8HlZ8FPpcU9XbLl+ASd+SWr8jCTGLSxF2hME8Lgg\n"
"-----END CERTIFICATE-----\n";

const gnutls_datum_t ca_cert = { ca_cert_pem,
	sizeof(ca_cert_pem)
};

static unsigned char server_cert_pem[] =
"-----BEGIN CERTIFICATE-----\n"
"MIIDITCCAgmgAwIBAgIMVc22UBIVIpQdKaDeMA0GCSqGSIb3DQEBCwUAMA8xDTAL\n"
"BgNVBAMTBENBLTAwIBcNMTUwODE0MDkzNTEyWhgPOTk5OTEyMzEyMzU5NTlaMBMx\n"
"ETAPBgNVBAMTCHNlcnZlci0xMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKC\n"
"AQEAvhX+gDD8GkLW0GVH5C+AdbCFKAcj0tG+S+OuCpDp8NGZN4GXtbljUk5U82ha\n"
"nyq52eJCptCSspXNKq6Hn0H/eSXlRndnIblB49Dqy6kHq0i1ysmrbdbe9BWrUqeU\n"
"uKSZ8O98ANzHfVDOxCvhqGfytvrgudfk5JZxqAD2CXU6R5AjG60cnR49xGkplfKS\n"
"31fpdshDkQMm+w2hfa97wqjrTbQ7K4SIgB9AYbRNvHd8PAo6fxXrLaBPZkQu9AiP\n"
"D+sEz5bGrhzlIwz5SdcGAjuysB1WAygrWcTZ2zvX96lVTMhRF4umo8Rd1rzapB6G\n"
"Uj64cKtkyJjcGV54Ifd6E/lmDwIDAQABo3cwdTAMBgNVHRMBAf8EAjAAMBQGA1Ud\n"
"EQQNMAuCCWxvY2FsaG9zdDAPBgNVHQ8BAf8EBQMDB4AAMB0GA1UdDgQWBBSTZZoN\n"
"JNpaTuLaiXd+abUidelNDDAfBgNVHSMEGDAWgBRL47DaBU36gxCm3MBZ8SiHfmUD\n"
"/DANBgkqhkiG9w0BAQsFAAOCAQEANot3py74nzCijhKilXyHz44LnpzbZGxMzbdr\n"
"gK9maqqfiOWJMohOmSezYvMItudDn/Z3Bu7xzDxchDF80sBN+4UiDxl47uYbNl6o\n"
"UFfpFu4GmO0HfeWkbM1ZqVJGBa6zOCkc3aw0LK7O2YRcBcsjzdIPQpePf/jRpppJ\n"
"mz4qShtGa37Vfv4XxoXFPJdfil3uXl8Pe3qo+f8+DiMIIuxzKyQatu0DP4CjuEf1\n"
"6sgcBFbeUMAJsCh0qFbqObWyOe9XxFEukLMPV7s2EKnRcY7Xhyuf6wyNI/oPkmon\n"
"+m/yxJVZSWkpERsyXW1ZkR0Xw2KnJ4bzdQkDTs73ijOd4jFQvA==\n"
"-----END CERTIFICATE-----\n"
"-----BEGIN CERTIFICATE-----\n"
"MIIC8zCCAdugAwIBAgIBADANBgkqhkiG9w0BAQsFADAPMQ0wCwYDVQQDEwRDQS0w\n"
"MCAXDTE1MDgxNDA5MzUxMVoYDzk5OTkxMjMxMjM1OTU5WjAPMQ0wCwYDVQQDEwRD\n"
"QS0wMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4O2BYEx/hl7URXhb\n"
"52erURO6HrlfacZjG0fQ2WqRcJJTqg2baAbA7+1SLdhphZ+KJDypEjJWmOgbaehI\n"
"hlK7zDZb+0r0uXlUQ11mgf7FcCDQoJBmC1dcN3o6zPeXg5hkWV+ZV2h7hhJTwkRc\n"
"C4DXTbaDKy8PNiC0MRMOGjeMfnL26oaxzuHNgH4u1J02+XUZ0UcSDrUc52O1lJ02\n"
"i1SbD+fTNBgmFQADXyAllZYJ/xwbxf44TFhQjiOvVpz/9EB2+/x5H0r1YvwKGY6v\n"
"5mfkUsEAE5+uxDXdZT84ltEKkAjbZ9cIgdmXRuD4mkyo3NHLh7oHCdsRRE/S/rZe\n"
"ikmGpQIDAQABo1gwVjAPBgNVHRMBAf8EBTADAQH/MBMGA1UdJQQMMAoGCCsGAQUF\n"
"BwMJMA8GA1UdDwEB/wQFAwMHBgAwHQYDVR0OBBYEFEvjsNoFTfqDEKbcwFnxKId+\n"
"ZQP8MA0GCSqGSIb3DQEBCwUAA4IBAQAKrbc6hER0xAjn5driLNyoz0JJr5P07PDI\n"
"d8AR3ZC56DSJNdvKDqdFIvAoo/JePCTFSdhbaqu+08MoTtRK5TKqjRiDiG4XCxiz\n"
"Ado7QouS+ZgDP1Uxv8j2YWeSpkusD+oIEK96wbeDaYi0ENbLWbm9zWqvHaaEYn4c\n"
"ov78n+7VvP3I2OFuJ0EPy+r55GPxSCRCh6apL78yAc6TfcyOwwTihvCF5ejCqRg/\n"
"T1As5NCCpdYP2nejRymjO6wMRsRFBX9+gndO9qVQZJr8zBTw8k8/pMtDubjkYqEv\n"
"qRME4/3q8+Sm8HlZ8FPpcU9XbLl+ASd+SWr8jCTGLSxF2hME8Lgg\n"
"-----END CERTIFICATE-----\n";

const gnutls_datum_t server_cert = { server_cert_pem,
	sizeof(server_cert_pem)
};

static unsigned char server_key_pem[] =
"-----BEGIN RSA PRIVATE KEY-----\n"
"MIIEowIBAAKCAQEAvhX+gDD8GkLW0GVH5C+AdbCFKAcj0tG+S+OuCpDp8NGZN4GX\n"
"tbljUk5U82hanyq52eJCptCSspXNKq6Hn0H/eSXlRndnIblB49Dqy6kHq0i1ysmr\n"
"bdbe9BWrUqeUuKSZ8O98ANzHfVDOxCvhqGfytvrgudfk5JZxqAD2CXU6R5AjG60c\n"
"nR49xGkplfKS31fpdshDkQMm+w2hfa97wqjrTbQ7K4SIgB9AYbRNvHd8PAo6fxXr\n"
"LaBPZkQu9AiPD+sEz5bGrhzlIwz5SdcGAjuysB1WAygrWcTZ2zvX96lVTMhRF4um\n"
"o8Rd1rzapB6GUj64cKtkyJjcGV54Ifd6E/lmDwIDAQABAoIBAQCPPDOSlVbi0wrb\n"
"7fXGVKUQCfvMtdSgv7wNo3s6KwidltNFqDmRjijxlGUfJbtjxOZW8NAYs4JXX9pC\n"
"F1HLCAhiWdPyzXbBSsAD0yGaZbyJrTiPnne3RPqsIsf+eJjwqdf2Xf+rBrKsE4A7\n"
"AnYAWJPknhdI8w5f0Z3DYzYC2nsYAI/FvJCpQvs9qMfVznctzcLUpvquDYrkDaFA\n"
"Rk14xQ8zhXKSugx1N2QAabk9YhMIDgBRHvZkQfBYJ/bNhfpLveQZX14QLn++EuFZ\n"
"F0QpoOtJhWNZDbDdroud2G821dl5bLKsKx0cD63Bsz1uV8vUQF0F2xx8t64SPhz9\n"
"zC/eZB+BAoGBAM57D4Nav1zreqBJZnWVtR7qr54AIg3nKccFPXLeezhI1iJi07tn\n"
"Fc2YdP+5NzRAVBOBKaMwuJ4ZdLnclsKD8A/LzMgerRfuV6EDHOPKAgWISU/+Up6x\n"
"Q5tQ2ocPjQFHb5gK3Le9lMkBHt1j6ZIptUIXTqzzwKYSDDYkdMCmSyBXAoGBAOus\n"
"XvHE/DIV6idE4k590nq+o9OdMet+LWUzmyTjlbVhPZ09vTSHs+3U1Fe4te6aNUI+\n"
"KkhizCHMvx+M0uzCwy31TDdLe9QbmtkQet0AAX/Qsb5IQrDi5iLl7UuvZMa7tCUe\n"
"R0puBRBzvZg4LQWDgJ9U4fO3YO0c3VBRpicQbvUJAoGAFN6bUst5TAsA+fJxSLE4\n"
"/Ub7OR0KVB1pO5RsAZA7JBU4j4EtpWNl8MHYEYDG86EM3mvPqY7jGhe4lJCXLFHp\n"
"ka8no5J2LFUKxltqMBva2HRN9Kff8eo4yxoA/GW1+ssdnGB8rpWa1DYoyHeww/Uz\n"
"PNreONzqO97XHSHSKyajsUECgYAe/3ENg8dYHyHJQHozsMD6fBC4SLjELLhz0zHY\n"
"zEZosP2VrQUx35d+9LtpPlZPp+DRcbPGCZin6XJKCA/GLGfXp6f6reb/oxHe8xf1\n"
"8YZA9YYrbP24nl9+v5dSmSM8MHwlVbIyy/3GiDKrzte9HerRCi0eDUSma2GAqvyb\n"
"rsGpYQKBgCj7dXo0LKYaEJ17NXCD6Cu7gMP9haYo0HHfkhBnIgYs/Cytgnedzp6k\n"
"kRcVr4yllg5yEgiqPvg+PyuL1sm0epQ85qeYOaR2CsbN6mYnwX8/8LLZ7Ep4v3vv\n"
"m0SlmY5Hgw6lit1DOr1HDoZZKzbpT3H//TrMMhvBPdcBQwjcHMHl\n"
"-----END RSA PRIVATE KEY-----\n";

const gnutls_datum_t server_key = { server_key_pem,
	sizeof(server_key_pem)
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
					    &server_cert, &server_key,
					    GNUTLS_X509_FMT_PEM);

	gnutls_init(&server, GNUTLS_SERVER);
	gnutls_credentials_set(server, GNUTLS_CRD_CERTIFICATE,
				serverx509cred);
	gnutls_priority_set_direct(server,
				   "NORMAL:-KX-ALL:+RSA",
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

	gnutls_priority_set_direct(client, "NORMAL:+RSA", NULL);
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

		ret = gnutls_pcert_import_x509_raw(p, &server_cert, GNUTLS_X509_FMT_PEM, 0);
		if (ret < 0)
			return -1;

		ret = gnutls_privkey_init(&lkey);
		if (ret < 0)
			return -1;

		ret = gnutls_privkey_import_x509_raw(lkey, &server_key, GNUTLS_X509_FMT_PEM, NULL, 0);
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
				   "NORMAL:-KX-ALL:+RSA:%DEBUG_ALLOW_KEY_USAGE_VIOLATIONS",
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

	gnutls_priority_set_direct(client, "NORMAL:+RSA", NULL);
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
