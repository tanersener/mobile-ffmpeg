/*
 * Copyright (C) 2020 Red Hat, Inc.
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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>

#ifdef ENABLE_OCSP

#include "cert-common.h"
#include "ocsp-common.h"
#include "utils.h"

/* Tests whether setting an OCSP response to a client
 * is working as expected */

static time_t mytime(time_t * t)
{
	time_t then = OCSP_RESP_DATE;
	if (t)
		*t = then;

	return then;
}

static void check_cli(gnutls_session_t session, void *priv)
{
	assert((gnutls_session_get_flags(session) & GNUTLS_SFLAGS_SERV_REQUESTED_OCSP) != 0);
}

static void check_serv(gnutls_session_t session, void *priv)
{
	int ret;
	unsigned int status;
	gnutls_datum_t resp;
	gnutls_datum_t *exp_resp = priv;

	assert((gnutls_session_get_flags(session) & GNUTLS_SFLAGS_SERV_REQUESTED_OCSP) != 0);

	ret = gnutls_ocsp_status_request_get(session, &resp);
	if (ret < 0) {
		if (priv == NULL)
			return;
		fail("no response was received\n");
	}

	if (priv == NULL) {
		fail("not expected response, but received one\n");
	}

	if (resp.size != exp_resp->size || memcmp(resp.data, exp_resp->data, resp.size) != 0) {
		fail("did not receive the expected response\n");
	}

	/* Check intermediate response */
	if (gnutls_protocol_get_version(session) == GNUTLS_TLS1_3) {
		ret = gnutls_ocsp_status_request_get2(session, 1, &resp);
		if (ret < 0) {
			fail("no intermediate response was received\n");
		}

		if (resp.size != ocsp_subca3_unknown.size || memcmp(resp.data, ocsp_subca3_unknown.data, resp.size) != 0) {
			fail("did not receive the expected intermediate response\n");
		}
	}

	ret = gnutls_certificate_verify_peers2(session, &status);
	if (ret != 0)
		fail("error in verification (%s)\n", gnutls_strerror(ret));

	ret = gnutls_ocsp_status_request_is_checked(session, GNUTLS_OCSP_SR_IS_AVAIL);
	if (ret == 0) {
		fail("did not receive the expected value (%d)\n", ret);
	}

	ret = gnutls_ocsp_status_request_is_checked(session, 0);
	if (ret == 0) {
		fail("did not receive the expected value (%d)\n", ret);
	}
}

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "|<%d>| %s", level, str);
}

void doit(void)
{
	int ret;
	gnutls_certificate_credentials_t xcred;
	gnutls_certificate_credentials_t clicred;
	const char *certfile1;
	const char *ocspfile1;
	char certname1[TMPNAME_SIZE], ocspname1[TMPNAME_SIZE];
	FILE *fp;
	unsigned index1;
	time_t t;

	global_init();
	gnutls_global_set_time_function(mytime);

	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(4711);

	assert(gnutls_certificate_allocate_credentials(&xcred) >= 0);
	assert(gnutls_certificate_allocate_credentials(&clicred) >= 0);

	gnutls_certificate_set_flags(xcred, GNUTLS_CERTIFICATE_API_V2);

	certfile1 = get_tmpname(certname1);

	/* set cert with localhost name */
	fp = fopen(certfile1, "wb");
	if (fp == NULL)
		fail("error in fopen\n");
	assert(fwrite(server_localhost_ca3_cert_chain_pem, 1, strlen(server_localhost_ca3_cert_chain_pem), fp)>0);
	assert(fwrite(server_ca3_key_pem, 1, strlen((char*)server_ca3_key_pem), fp)>0);
	fclose(fp);

	ret = gnutls_certificate_set_x509_key_file2(xcred, certfile1, certfile1,
						    GNUTLS_X509_FMT_PEM, NULL, 0);
	if (ret < 0)
		fail("set_x509_key_file failed: %s\n", gnutls_strerror(ret));

	ret = gnutls_certificate_set_x509_key_file2(clicred, certfile1, certfile1,
						    GNUTLS_X509_FMT_PEM, NULL, 0);
	if (ret < 0)
		fail("set_x509_key_file failed: %s\n", gnutls_strerror(ret));
	index1 = ret;

	/* set OCSP response1, include an unrelated OCSP response */
	ocspfile1 = get_tmpname(ocspname1);
	fp = fopen(ocspfile1, "wb");
	if (fp == NULL)
		fail("error in fopen\n");
	assert(fwrite(ocsp_subca3_unknown_pem.data, 1, ocsp_subca3_unknown_pem.size, fp)>0);
	assert(fwrite(ocsp_ca3_localhost_unknown_pem.data, 1, ocsp_ca3_localhost_unknown_pem.size, fp)>0);
	assert(fwrite(ocsp_ca3_localhost6_unknown_pem.data, 1, ocsp_ca3_localhost6_unknown_pem.size, fp)>0);
	fclose(fp);

	ret = gnutls_certificate_set_ocsp_status_request_file2(clicred, ocspfile1, index1,
							       GNUTLS_X509_FMT_PEM);
	if (ret != GNUTLS_E_OCSP_MISMATCH_WITH_CERTS)
		fail("ocsp file set failed: %s\n", gnutls_strerror(ret));

	/* set OCSP response1, include correct responses */
	remove(ocspfile1);
	fp = fopen(ocspfile1, "wb");
	if (fp == NULL)
		fail("error in fopen\n");
	assert(fwrite(ocsp_subca3_unknown_pem.data, 1, ocsp_subca3_unknown_pem.size, fp)>0);
	assert(fwrite(ocsp_ca3_localhost_unknown_pem.data, 1, ocsp_ca3_localhost_unknown_pem.size, fp)>0);
	fclose(fp);

	ret = gnutls_certificate_set_ocsp_status_request_file2(clicred, ocspfile1, index1,
							       GNUTLS_X509_FMT_PEM);
	if (ret < 0)
		fail("ocsp file set failed: %s\n", gnutls_strerror(ret));

	ret = gnutls_certificate_set_x509_trust_mem(clicred, &ca3_cert, GNUTLS_X509_FMT_PEM);
	if (ret < 0) {
		fail("error in setting trust cert: %s\n", gnutls_strerror(ret));
	}

	t = gnutls_certificate_get_ocsp_expiration(clicred, 0, 0, 0);
	if (t != 1509625639)
		fail("error in OCSP validity time: %ld\n", (long int)t);

	t = gnutls_certificate_get_ocsp_expiration(clicred, 0, 1, 0);
	if (t != 1509625639)
		fail("error in OCSP validity time: %ld\n", (long int)t);

	t = gnutls_certificate_get_ocsp_expiration(clicred, 0, -1, 0);
	if (t != 1509625639)
		fail("error in OCSP validity time: %ld\n", (long int)t);

#define PRIO "NORMAL:-ECDHE-ECDSA:-VERS-TLS-ALL:+VERS-TLS1.3"
	_test_cli_serv(xcred, clicred, PRIO, PRIO, "localhost", &ocsp_ca3_localhost_unknown, check_cli,
		       check_serv, 0, 1, 0, 0);

	gnutls_certificate_free_credentials(xcred);
	gnutls_certificate_free_credentials(clicred);
	gnutls_global_deinit();
	remove(ocspfile1);
	remove(certfile1);
}

#else
void doit(void)
{
	exit(77);
}
#endif
