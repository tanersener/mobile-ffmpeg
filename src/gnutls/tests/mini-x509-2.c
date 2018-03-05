/*
 * Copyright (C) 2008-2012 Free Software Foundation, Inc.
 *
 * Author: Simon Josefsson
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
#include <gnutls/x509.h>
#include "utils.h"
#include "eagain-common.h"

/* This tests gnutls_certificate_set_x509_key() */

const char *side;

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "%s|<%d>| %s", side, level, str);
}

static unsigned char ca_cert_pem[] =
"-----BEGIN CERTIFICATE-----\n"
"MIIC4DCCAcigAwIBAgIBADANBgkqhkiG9w0BAQsFADAPMQ0wCwYDVQQDEwRDQS0w\n"
"MCIYDzIwMTQwNDA5MDgwMjM0WhgPOTk5OTEyMzEyMzU5NTlaMA8xDTALBgNVBAMT\n"
"BENBLTAwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQCuLSye8pe3yWKZ\n"
"Yp7tLQ4ImwLqqh1aN7x9pc5spLDj6krVArzkyyYDcWvtQNDjErEfLUrZZrCc4aIl\n"
"oU1Ghb92kI8ofZnHFbj3z5zdcWqiPppj5Y+hRdc4LszTWb+itrD9Ht/D67EK+m7W\n"
"ev6xxUdyiBYUmb2O3CnPZpUVshMRtEe45EDGI5hUgL2n4Msj41htTq8hATYPXgoq\n"
"gQUyXFpKAX5XDCyOG+FC6jmEys7UCRYv3SCl7TPWJ4cm+lHcFI2/OTOCBvMlKN2J\n"
"mWCdfnudZldqthin+8fR9l4nbuutOfPNt1Dj9InDzWZ1W/o4LrjKa7fsvszj2Z5A\n"
"Fn+xN/4zAgMBAAGjQzBBMA8GA1UdEwEB/wQFMAMBAf8wDwYDVR0PAQH/BAUDAwcE\n"
"ADAdBgNVHQ4EFgQUwRHwbXyPosKNNkBiZduEwL5ZCwswDQYJKoZIhvcNAQELBQAD\n"
"ggEBAEKr0b7WoJL+L8St/LEITU/i7FwFrCP6DkbaNo0kgzPmwnvNmw88MLI6UKwE\n"
"JecnjFhurRBBZ4FA85ucNyizeBnuXqFcyJ20+XziaXGPKV/ugKyYv9KBoTYkQOCh\n"
"nbOthmDqjvy2UYQj0BU2dOywkjUKWhYHEZLBpZYck0Orynxydwil5Ncsz4t3smJw\n"
"ahzCW8SzBFTiO99qQBCH2RH1PbUYzfAnJxZS2VScpcqlu9pr+Qv7r8E3p9qHxnQM\n"
"gO5laWO6lc13rNsbZRrtlCvacsiDSuDnS8EVXm0ih4fAntpRHacPbXZbOPQqJ/+1\n"
"G7/qJ6cDC/9aW+fU80ogTkAoFg4=\n"
"-----END CERTIFICATE-----\n";

const gnutls_datum_t ca_cert = { ca_cert_pem,
	sizeof(ca_cert_pem)
};

static unsigned char server_cert_pem[] =
"-----BEGIN CERTIFICATE-----\n"
"MIIDOjCCAiKgAwIBAgIMU0T+mwoDu5uVLKeeMA0GCSqGSIb3DQEBCwUAMA8xDTAL\n"
"BgNVBAMTBENBLTEwIhgPMjAxNDA0MDkwODAyMzVaGA85OTk5MTIzMTIzNTk1OVow\n"
"EzERMA8GA1UEAxMIc2VydmVyLTIwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEK\n"
"AoIBAQDXfvgsMWXHNf3iUaEoZSNztZZr6+UdBkoUhbdWJDR+GwR+GHfnYaYHsuqb\n"
"bNEl/QFI+8Jeth0SmG7TNB+b/AlHFoBm8TwBt7H+Mn6AQIdo872Vs262UkHgbZN6\n"
"dEQeRCgiXmlsOVe+MVpf79Xi32MYz1FZ/ueS6tr8sIDhECThIZkq2eulVjAV86N2\n"
"zQ72Ml1k8rPw4SdK5OFhcXNdXr6CsAol8MmiORKDF0iAZxwtFVc00nBGqQC5rwrN\n"
"3A8czH5TsvyvrcW0mwV2XOVvZM5kFM1T/X0jF6RQHiGGFBYK4s6JZxSSOhJMFYYh\n"
"koPEKsuVZdmBJ2yTTdGumHZfG9LDAgMBAAGjgY0wgYowDAYDVR0TAQH/BAIwADAU\n"
"BgNVHREEDTALgglsb2NhbGhvc3QwEwYDVR0lBAwwCgYIKwYBBQUHAwEwDwYDVR0P\n"
"AQH/BAUDAwegADAdBgNVHQ4EFgQURXiN5VD5vgqAprhd/37ldGKv4/4wHwYDVR0j\n"
"BBgwFoAU8MUzmkotjSmVa5r1ejMkMQ6BiZYwDQYJKoZIhvcNAQELBQADggEBABSU\n"
"cmMX0nGeg43itPnLjSTIUuYEamRhfsFDwgRYQn5w+BcFG1p0scBRxLAShUEb9A2A\n"
"oEJV4rQDpCn9bcMrMHhTCR5sOlLh/2o9BROjK0+DjQLDkooQK5xa+1GYEiy6QYCx\n"
"QjdCCnMhHh24oP2/vUggRKhevvD2QQFKcCDT6n13RFYm+HX82gIh6SAtRs0oahY5\n"
"k9CM9TYRPzXy+tQqhZisJzc8BLTW/XA97kAJW6+hUhPir7AYR6BKJhNeIxcN/yMy\n"
"jsHzWDLezip/8q+kzw658V5e40hne7ZaJycGUaUdLVnJcpNtBgGE82TRS/XZSQKF\n"
"fpy8FLGcJynqlIOzdKs=\n"
"-----END CERTIFICATE-----\n"
"-----BEGIN CERTIFICATE-----\n"
"MIIDATCCAemgAwIBAgIBATANBgkqhkiG9w0BAQsFADAPMQ0wCwYDVQQDEwRDQS0w\n"
"MCIYDzIwMTQwNDA5MDgwMjM0WhgPOTk5OTEyMzEyMzU5NTlaMA8xDTALBgNVBAMT\n"
"BENBLTEwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDZq3sA+mjFadII\n"
"EMDHfj1fYh+UOUSa8c814E9NfCdYZ9Z11BmPpBeR5mXV12j1DKjkTlqTUL7s4lVR\n"
"RKfyAdCpQIfeXHDeTYYUq2uBnbi5YMG5Y+WbCiYacgRU3IypYrSzaeh1mY7GiEFe\n"
"U/NaImHLCf+TdAvTJ3Fo0QPe5QN2Lrv6l//cqOv7enZ91KRWxClDMM6EAr+C/7dk\n"
"rOTXRrCuH/e/KVBXEJ/YeSYPmBIwolGktRrGdsVagdqYArr4dhJ7VThIVRUX1Ijl\n"
"THCLstI/LuD8WkDccU3ZSdm47f2U43p/+rSO0MiNOXiaskeK56G/9DbJEeETUbzm\n"
"/B2712MVAgMBAAGjZDBiMA8GA1UdEwEB/wQFMAMBAf8wDwYDVR0PAQH/BAUDAwcE\n"
"ADAdBgNVHQ4EFgQU8MUzmkotjSmVa5r1ejMkMQ6BiZYwHwYDVR0jBBgwFoAUwRHw\n"
"bXyPosKNNkBiZduEwL5ZCwswDQYJKoZIhvcNAQELBQADggEBACKxBPj9u1t52uIF\n"
"eQ2JPb8/u+MBttvSLo0qPKXwpc4q8hNclh66dpqGWiF0iSumsKyKU54r6CIF9Ikm\n"
"t1V1GR9Ll4iTnz3NdIt1w3ns8rSlU5O/dgKysK/1C/5xJWEUYtEO5mnyi4Zaf8FB\n"
"hKmQ1aWF5dTB81PVAQxyCiFEnH7YumK7pJeIpnCOPIqLZLUHfrTUeL8zONF4i5Sb\n"
"7taZ8SQ6b7IaioU+NJ50uT2wy34lsyvCWf76Azezv9bggkdNDo/7ktMgsfRrSyM8\n"
"+MVob5ePGTjKx5yMy/sy2vUkkefwW3RiEss/y2JRb8Hw7nDlA9ttilYKFwGFwRvw\n"
"KRsXqo8=\n"
"-----END CERTIFICATE-----\n";

const gnutls_datum_t server_cert = { server_cert_pem,
	sizeof(server_cert_pem)
};

static unsigned char server_key_pem[] =
"-----BEGIN RSA PRIVATE KEY-----\n"
"MIIEpAIBAAKCAQEA1374LDFlxzX94lGhKGUjc7WWa+vlHQZKFIW3ViQ0fhsEfhh3\n"
"52GmB7Lqm2zRJf0BSPvCXrYdEphu0zQfm/wJRxaAZvE8Abex/jJ+gECHaPO9lbNu\n"
"tlJB4G2TenREHkQoIl5pbDlXvjFaX+/V4t9jGM9RWf7nkura/LCA4RAk4SGZKtnr\n"
"pVYwFfOjds0O9jJdZPKz8OEnSuThYXFzXV6+grAKJfDJojkSgxdIgGccLRVXNNJw\n"
"RqkAua8KzdwPHMx+U7L8r63FtJsFdlzlb2TOZBTNU/19IxekUB4hhhQWCuLOiWcU\n"
"kjoSTBWGIZKDxCrLlWXZgSdsk03Rrph2XxvSwwIDAQABAoIBAB7trDS7ij4DM8MN\n"
"sDGaAnKS91nZ63I0+uDjKCMG4znOKuDmJh9hVnD4bs+L2KC5JTwSVh09ygJnOlC5\n"
"xGegzrwTMK6VpOUiNjujh6BkooqfoPAhZpxoReguEeKbWUN2yMPWBQ9xU3SKpMvs\n"
"IiiDozdmWeiuuxHM/00REA49QO3Gnx2logeB+fcvXXD1UiZV3x0xxSApiJt1sr2r\n"
"NmqSyGdNUgpmnTP8zbKnDaRe5Wj4tj1TCTLE/HZ0tzdRuwlkIqvcpGg1LMtKm5N8\n"
"xIWjTGMFwGjG+OF8LGqHLH+28pI3iMB6QqO2YLwOp+WZKImKP3+Dp3s8lCw8t8cm\n"
"q5/Qc9ECgYEA2xwxm+pFkrFmZNLCakP/6S5AZqpfSBRUlF/uX2pBKO7o6I6aOV9o\n"
"zq2QWYIZfdyD+9MvAFUQ36sWfTVWpGA34WGtsGtcRRygKKTigpJHvBldaPxiuYuk\n"
"xbS54nWUdix/JzyQAy22xJXlp4XJvtFJjHhA2td0XA7tfng9n8jmvEUCgYEA+8cA\n"
"uFIQFbaZ2y6pnOvlVj8OH0f1hZa9M+3q01fWy1rnDAsLrIzJy8TZnBtpDwy9lAun\n"
"Sa6wzu6qeHmF17xwk5U7BCyK2Qj/9KhRLg1mnDebQ/CiLSAaJVnrYFp9Du96fTkN\n"
"ollvbFiGF92QwPTDf2f1gHZQEPwa+f/ox37ad2cCgYEAwMgXpfUD7cOEMeV2BQV7\n"
"XnDBXRM97i9lE38sPmtAlYFPD36Yly4pCt+PCBH9181zmtf+nK47wG/Jw7RwXQQD\n"
"ZpwItBZiArTi/Z/FY9jMoOU4WKznOBVzjjgq7ONDEo6n+Z/BnepUyraQb0q5bNi7\n"
"e4o6ldHHoU/JCeNFZRbgXHkCgYA6vJU9at+XwS6phHxLQHkTIsivoYD0tlLTX4it\n"
"30sby8wk8hq6GWomYHkHwxlCSo2bkRBozxkuXV1ll6wSxUJaG7FV6vJFaaUUtYOi\n"
"w7uRbCOLuQKMlnWjCxQvOUz9g/7GYd39ZvHoi8pUnPrdGPzWpzEN1AwfukCs2/e5\n"
"Oq3KtwKBgQCkHmDU8h0kOfN28f8ZiyjJemQMNoOGiJqnGexaKvsRd+bt4H+7DsWQ\n"
"OnyKm/oR0wCCSmFM5aQc6GgzPD7orueKVYHChbY7HLTWKRHNs6Rlk+6hXJvOld0i\n"
"Cl7KqL2x2ibGMtt4LtSntdzWqa87N7vCWMSTmvd8uLgflBs33xUIiQ==\n"
"-----END RSA PRIVATE KEY-----\n";

static unsigned char cert_pem[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIICHjCCAYmgAwIBAgIERiYdNzALBgkqhkiG9w0BAQUwGTEXMBUGA1UEAxMOR251\n"
    "VExTIHRlc3QgQ0EwHhcNMDcwNDE4MTMyOTI3WhcNMDgwNDE3MTMyOTI3WjAdMRsw\n"
    "GQYDVQQDExJHbnVUTFMgdGVzdCBjbGllbnQwgZwwCwYJKoZIhvcNAQEBA4GMADCB\n"
    "iAKBgLtmQ/Xyxde2jMzF3/WIO7HJS2oOoa0gUEAIgKFPXKPQ+GzP5jz37AR2ExeL\n"
    "ZIkiW8DdU3w77XwEu4C5KL6Om8aOoKUSy/VXHqLnu7czSZ/ju0quak1o/8kR4jKN\n"
    "zj2AC41179gAgY8oBAOgIo1hBAf6tjd9IQdJ0glhaZiQo1ipAgMBAAGjdjB0MAwG\n"
    "A1UdEwEB/wQCMAAwEwYDVR0lBAwwCgYIKwYBBQUHAwIwDwYDVR0PAQH/BAUDAweg\n"
    "ADAdBgNVHQ4EFgQUTLkKm/odNON+3svSBxX+odrLaJEwHwYDVR0jBBgwFoAU6Twc\n"
    "+62SbuYGpFYsouHAUyfI8pUwCwYJKoZIhvcNAQEFA4GBALujmBJVZnvaTXr9cFRJ\n"
    "jpfc/3X7sLUsMvumcDE01ls/cG5mIatmiyEU9qI3jbgUf82z23ON/acwJf875D3/\n"
    "U7jyOsBJ44SEQITbin2yUeJMIm1tievvdNXBDfW95AM507ShzP12sfiJkJfjjdhy\n"
    "dc8Siq5JojruiMizAf0pA7in\n" "-----END CERTIFICATE-----\n";
const gnutls_datum_t cli_cert = { cert_pem, sizeof(cert_pem) - 1};

static unsigned char key_pem[] =
    "-----BEGIN RSA PRIVATE KEY-----\n"
    "MIICXAIBAAKBgQC7ZkP18sXXtozMxd/1iDuxyUtqDqGtIFBACIChT1yj0Phsz+Y8\n"
    "9+wEdhMXi2SJIlvA3VN8O+18BLuAuSi+jpvGjqClEsv1Vx6i57u3M0mf47tKrmpN\n"
    "aP/JEeIyjc49gAuNde/YAIGPKAQDoCKNYQQH+rY3fSEHSdIJYWmYkKNYqQIDAQAB\n"
    "AoGADpmARG5CQxS+AesNkGmpauepiCz1JBF/JwnyiX6vEzUh0Ypd39SZztwrDxvF\n"
    "PJjQaKVljml1zkJpIDVsqvHdyVdse8M+Qn6hw4x2p5rogdvhhIL1mdWo7jWeVJTF\n"
    "RKB7zLdMPs3ySdtcIQaF9nUAQ2KJEvldkO3m/bRJFEp54k0CQQDYy+RlTmwRD6hy\n"
    "7UtMjR0H3CSZJeQ8svMCxHLmOluG9H1UKk55ZBYfRTsXniqUkJBZ5wuV1L+pR9EK\n"
    "ca89a+1VAkEA3UmBelwEv2u9cAU1QjKjmwju1JgXbrjEohK+3B5y0ESEXPAwNQT9\n"
    "TrDM1m9AyxYTWLxX93dI5QwNFJtmbtjeBQJARSCWXhsoaDRG8QZrCSjBxfzTCqZD\n"
    "ZXtl807ymCipgJm60LiAt0JLr4LiucAsMZz6+j+quQbSakbFCACB8SLV1QJBAKZQ\n"
    "YKf+EPNtnmta/rRKKvySsi3GQZZN+Dt3q0r094XgeTsAqrqujVNfPhTMeP4qEVBX\n"
    "/iVX2cmMTSh3w3z8MaECQEp0XJWDVKOwcTW6Ajp9SowtmiZ3YDYo1LF9igb4iaLv\n"
    "sWZGfbnU3ryjvkb6YuFjgtzbZDZHWQCo8/cOtOBmPdk=\n"
    "-----END RSA PRIVATE KEY-----\n";
const gnutls_datum_t cli_key = { key_pem, sizeof(key_pem) - 1};

const gnutls_datum_t server_key = { server_key_pem,
	sizeof(server_key_pem)
};

void doit(void)
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
	gnutls_x509_crt_t *crts;
	unsigned int crts_size;
	unsigned i;
	gnutls_x509_privkey_t pkey;

	/* General init. */
	global_init();
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(2);

	ret = gnutls_x509_crt_list_import2(&crts, &crts_size, &server_cert, GNUTLS_X509_FMT_PEM,
			GNUTLS_X509_CRT_LIST_FAIL_IF_UNSORTED);
	if (ret < 0) {
		fprintf(stderr, "error: %s\n", gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_x509_privkey_init(&pkey);
	if (ret < 0) {
		fprintf(stderr, "error: %s\n", gnutls_strerror(ret));
		exit(1);
	}

	ret =
	    gnutls_x509_privkey_import(pkey, &server_key,
					GNUTLS_X509_FMT_PEM);
	if (ret < 0) {
		fprintf(stderr, "error: %s\n", gnutls_strerror(ret));
		exit(1);
	}

	/* Init server */
	gnutls_certificate_allocate_credentials(&serverx509cred);
	gnutls_certificate_set_x509_key(serverx509cred, crts, crts_size, pkey);
	gnutls_x509_privkey_deinit(pkey);
	for (i=0;i<crts_size;i++)
		gnutls_x509_crt_deinit(crts[i]);
	gnutls_free(crts);

	gnutls_init(&server, GNUTLS_SERVER);
	gnutls_credentials_set(server, GNUTLS_CRD_CERTIFICATE,
				serverx509cred);
	gnutls_priority_set_direct(server,
				   "NORMAL:-CIPHER-ALL:+AES-128-GCM",
				   NULL);
	gnutls_transport_set_push_function(server, server_push);
	gnutls_transport_set_pull_function(server, server_pull);
	gnutls_transport_set_ptr(server, server);
	gnutls_certificate_server_set_request(server, GNUTLS_CERT_REQUEST);

	/* Init client */
	/* Init client */
	ret = gnutls_certificate_allocate_credentials(&clientx509cred);
	if (ret < 0)
		exit(1);

	ret = gnutls_certificate_set_x509_trust_mem(clientx509cred, &ca_cert, GNUTLS_X509_FMT_PEM);
	if (ret < 0)
		exit(1);

	ret = gnutls_certificate_set_x509_key_mem(clientx509cred,
						  &cli_cert, &cli_key,
						  GNUTLS_X509_FMT_PEM);

	ret = gnutls_init(&client, GNUTLS_CLIENT);
	if (ret < 0)
		exit(1);

	ret = gnutls_credentials_set(client, GNUTLS_CRD_CERTIFICATE,
				clientx509cred);
	if (ret < 0)
		exit(1);

	gnutls_priority_set_direct(client, "NORMAL", NULL);
	gnutls_transport_set_push_function(client, client_push);
	gnutls_transport_set_pull_function(client, client_pull);
	gnutls_transport_set_ptr(client, client);

	HANDSHAKE(client, server);

	/* check gnutls_certificate_get_ours() - server side */
	{
		const gnutls_datum_t *mcert;
		gnutls_datum_t scert;
		gnutls_x509_crt_t crt;

		mcert = gnutls_certificate_get_ours(server);
		if (mcert == NULL) {
			fail("gnutls_certificate_get_ours(): failed\n");
			exit(1);
		}

		gnutls_x509_crt_init(&crt);
		ret = gnutls_x509_crt_import(crt, &server_cert, GNUTLS_X509_FMT_PEM);
		if (ret < 0) {
			fail("gnutls_x509_crt_import: %s\n", gnutls_strerror(ret));
			exit(1);
		}

		ret = gnutls_x509_crt_export2(crt, GNUTLS_X509_FMT_DER, &scert);
		if (ret < 0) {
			fail("gnutls_x509_crt_export2: %s\n", gnutls_strerror(ret));
			exit(1);
		}
		gnutls_x509_crt_deinit(crt);

		if (scert.size != mcert->size || memcmp(scert.data, mcert->data, mcert->size) != 0) {
			fail("gnutls_certificate_get_ours output doesn't match cert\n");
			exit(1);
		}
		gnutls_free(scert.data);
	}

	/* check gnutls_certificate_get_ours() - client side */
	{
		const gnutls_datum_t *mcert;
		gnutls_datum_t ccert;
		gnutls_x509_crt_t crt;

		mcert = gnutls_certificate_get_ours(client);
		if (mcert == NULL) {
			fail("gnutls_certificate_get_ours(): failed\n");
			exit(1);
		}

		gnutls_x509_crt_init(&crt);
		ret = gnutls_x509_crt_import(crt, &cli_cert, GNUTLS_X509_FMT_PEM);
		if (ret < 0) {
			fail("gnutls_x509_crt_import: %s\n", gnutls_strerror(ret));
			exit(1);
		}

		ret = gnutls_x509_crt_export2(crt, GNUTLS_X509_FMT_DER, &ccert);
		if (ret < 0) {
			fail("gnutls_x509_crt_export2: %s\n", gnutls_strerror(ret));
			exit(1);
		}
		gnutls_x509_crt_deinit(crt);

		if (ccert.size != mcert->size || memcmp(ccert.data, mcert->data, mcert->size) != 0) {
			fail("gnutls_certificate_get_ours output doesn't match cert\n");
			exit(1);
		}
		gnutls_free(ccert.data);
	}

	/* check the number of certificates received */
	{
		unsigned cert_list_size = 0;
		gnutls_typed_vdata_st data[2];
		unsigned status;

		memset(data, 0, sizeof(data));

		/* check with wrong hostname */
		data[0].type = GNUTLS_DT_DNS_HOSTNAME;
		data[0].data = (void*)"localhost1";

		data[1].type = GNUTLS_DT_KEY_PURPOSE_OID;
		data[1].data = (void*)GNUTLS_KP_TLS_WWW_SERVER;

		gnutls_certificate_get_peers(client, &cert_list_size);
		if (cert_list_size < 2) {
			fprintf(stderr, "received a certificate list of %d!\n", cert_list_size);
			exit(1);
		}

		ret = gnutls_certificate_verify_peers(client, data, 2, &status);
		if (ret < 0) {
			fprintf(stderr, "could not verify certificate: %s\n", gnutls_strerror(ret));
			exit(1);
		}

		if (status == 0) {
			fprintf(stderr, "should not have accepted!\n");
			exit(1);
		}

		/* check with wrong purpose */
		data[0].type = GNUTLS_DT_DNS_HOSTNAME;
		data[0].data = (void*)"localhost";

		data[1].type = GNUTLS_DT_KEY_PURPOSE_OID;
		data[1].data = (void*)GNUTLS_KP_TLS_WWW_CLIENT;

		gnutls_certificate_get_peers(client, &cert_list_size);
		if (cert_list_size < 2) {
			fprintf(stderr, "received a certificate list of %d!\n", cert_list_size);
			exit(1);
		}

		ret = gnutls_certificate_verify_peers(client, data, 2, &status);
		if (ret < 0) {
			fprintf(stderr, "could not verify certificate: %s\n", gnutls_strerror(ret));
			exit(1);
		}

		if (status == 0) {
			fprintf(stderr, "should not have accepted!\n");
			exit(1);
		}

		/* check with correct purpose */
		data[0].type = GNUTLS_DT_DNS_HOSTNAME;
		data[0].data = (void*)"localhost";

		data[1].type = GNUTLS_DT_KEY_PURPOSE_OID;
		data[1].data = (void*)GNUTLS_KP_TLS_WWW_SERVER;

		ret = gnutls_certificate_verify_peers(client, data, 2, &status);
		if (ret < 0) {
			fprintf(stderr, "could not verify certificate: %s\n", gnutls_strerror(ret));
			exit(1);
		}

		if (status != 0) {
			fprintf(stderr, "could not verify certificate: %.4x\n", status);
			exit(1);
		}
	}

	gnutls_bye(client, GNUTLS_SHUT_RDWR);
	gnutls_bye(server, GNUTLS_SHUT_RDWR);

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
