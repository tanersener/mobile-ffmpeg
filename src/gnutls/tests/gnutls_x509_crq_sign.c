/*
 * Copyright (C) 2008-2012 Free Software Foundation, Inc.
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#include <assert.h>

#include "utils.h"

static unsigned char saved_crq_pem[] =
	"-----BEGIN NEW CERTIFICATE REQUEST-----\n"
	"MIIBhTCB7wIBADAAMIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQC7ZkP18sXX\n"
	"tozMxd/1iDuxyUtqDqGtIFBACIChT1yj0Phsz+Y89+wEdhMXi2SJIlvA3VN8O+18\n"
	"BLuAuSi+jpvGjqClEsv1Vx6i57u3M0mf47tKrmpNaP/JEeIyjc49gAuNde/YAIGP\n"
	"KAQDoCKNYQQH+rY3fSEHSdIJYWmYkKNYqQIDAQABoEYwRAYJKoZIhvcNAQkOMTcw\n"
	"NTAPBgNVHRMBAf8EBTADAgEAMA8GA1UdDwEB/wQFAwMHAAAwEQYDVR0RAQH/BAcw\n"
	"BYIDZm9vMA0GCSqGSIb3DQEBCwUAA4GBAK7iC1R9oKxpHuVHksT1Y8yC0PzxreJz\n"
	"2d4DZKQeycaAAFHGCxVJw3t5S+/W81l0nj1z8vW5VJEsgT8loFRb1LWWlyGDUBHY\n"
	"0aZ/9CLbRFGq4SenPU4dridwiwZVdXzF/NNFIMDp85qbCcw4qZlKinrKolqs3ymE\n"
	"qjSnoJuZmwSQ\n"
	"-----END NEW CERTIFICATE REQUEST-----\n";

const gnutls_datum_t saved_crq = { saved_crq_pem, sizeof(saved_crq_pem)-1 };

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
const gnutls_datum_t key = { key_pem, sizeof(key_pem)-1 };

static time_t mytime(time_t * t)
{
	time_t then = 1207000800;

	if (t)
		*t = then;

	return then;
}

static gnutls_x509_crq_t generate_crq(void)
{
	gnutls_x509_crq_t crq;
	gnutls_x509_privkey_t pkey;
	int ret;
	size_t s = 0;

	ret = gnutls_x509_privkey_init(&pkey);
	if (ret != 0)
		fail("gnutls_x509_privkey_init\n");

	ret = gnutls_x509_privkey_import(pkey, &key, GNUTLS_X509_FMT_PEM);
	if (ret != 0)
		fail("gnutls_x509_privkey_import\n");

	ret = gnutls_x509_crq_init(&crq);
	if (ret != 0)
		fail("gnutls_x509_crq_init\n");

	ret = gnutls_x509_crq_set_version(crq, 0);
	if (ret != 0)
		fail("gnutls_x509_crq_set_version\n");

	ret = gnutls_x509_crq_set_key(crq, pkey);
	if (ret != 0)
		fail("gnutls_x509_crq_set_key\n");

	s = 0;
	ret = gnutls_x509_crq_get_extension_info(crq, 0, NULL, &s, NULL);
	if (ret != GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE)
		fail("gnutls_x509_crq_get_extension_info\n");

	ret = gnutls_x509_crq_set_basic_constraints(crq, 0, 0);
	if (ret != 0)
		fail("gnutls_x509_crq_set_basic_constraints %d\n", ret);

	ret = gnutls_x509_crq_set_key_usage(crq, 0);
	if (ret != 0)
		fail("gnutls_x509_crq_set_key_usage %d\n", ret);

	ret = gnutls_x509_crq_set_subject_alt_name(crq, GNUTLS_SAN_DNSNAME,
						   "foo", 3, 1);
	if (ret != 0)
		fail("gnutls_x509_crq_set_subject_alt_name\n");

	ret = gnutls_x509_crq_sign(crq, pkey);
	if (ret < 0)
		fail("gnutls_x509_crq_sign: %s\n", gnutls_strerror(ret));

	gnutls_x509_privkey_deinit(pkey);

	return crq;
}

static void verify_crq(const gnutls_datum_t *pem)
{
	gnutls_x509_crq_t crq;

	assert(gnutls_x509_crq_init(&crq) >= 0);
	assert(gnutls_x509_crq_import(crq, pem, GNUTLS_X509_FMT_PEM)>=0);
	assert(gnutls_x509_crq_verify(crq, 0) >= 0);
	gnutls_x509_crq_deinit(crq);
}

void doit(void)
{
	gnutls_datum_t out;
	gnutls_x509_crq_t crq;

	gnutls_global_set_time_function(mytime);

	crq = generate_crq();

	assert(gnutls_x509_crq_export2(crq, GNUTLS_X509_FMT_PEM, &out) >= 0);

	if (debug)
		printf("%s\n", out.data);

	assert(out.size == saved_crq.size);
	assert(memcmp(out.data, saved_crq.data, out.size)==0);

	verify_crq(&out);

	gnutls_free(out.data);
	gnutls_x509_crq_deinit(crq);
}
