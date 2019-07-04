/*
 * Copyright (C) 2016-2017 Red Hat, Inc.
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

/* Tests whether setting an OCSP response to a server with multiple
 * certificate sets, is working as expected */

static time_t mytime(time_t * t)
{
	time_t then = OCSP_RESP_DATE;
	if (t)
		*t = then;

	return then;
}

static void check_response(gnutls_session_t session, void *priv)
{
	int ret;
	gnutls_datum_t resp;
	gnutls_datum_t *exp_resp = priv;

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
	const char *certfile2;
	const char *ocspfile1;
	const char *ocspfile2;
	const char *ocspfile3;
	char certname1[TMPNAME_SIZE], ocspname1[TMPNAME_SIZE];
	char certname2[TMPNAME_SIZE], ocspname2[TMPNAME_SIZE];
	char ocspname3[TMPNAME_SIZE];
	FILE *fp;
	unsigned index1, index2; /* indexes of certs */

	global_init();
	gnutls_global_set_time_function(mytime);

	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(4711);

	assert(gnutls_certificate_allocate_credentials(&xcred) >= 0);
	assert(gnutls_certificate_allocate_credentials(&clicred) >= 0);

	gnutls_certificate_set_flags(xcred, GNUTLS_CERTIFICATE_API_V2);

	/* set cert with localhost name */
	certfile1 = get_tmpname(certname1);

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
	index1 = ret;

	/* set cert with localhost6 name */
	certfile2 = get_tmpname(certname2);

	fp = fopen(certfile2, "wb");
	if (fp == NULL)
		fail("error in fopen\n");
	assert(fwrite(server_localhost6_ca3_cert_chain_pem, 1, strlen(server_localhost6_ca3_cert_chain_pem), fp)>0);
	assert(fwrite(server_ca3_key_pem, 1, strlen((char*)server_ca3_key_pem), fp)>0);
	fclose(fp);

	ret = gnutls_certificate_set_x509_key_file2(xcred, certfile2, certfile2,
						    GNUTLS_X509_FMT_PEM, NULL, 0);
	if (ret < 0)
		fail("set_x509_key_file failed: %s\n", gnutls_strerror(ret));
	index2 = ret;


	/* set OCSP response1 */
	ocspfile1 = get_tmpname(ocspname1);
	fp = fopen(ocspfile1, "wb");
	if (fp == NULL)
		fail("error in fopen\n");
	assert(fwrite(ocsp_ca3_localhost_unknown.data, 1, ocsp_ca3_localhost_unknown.size, fp)>0);
	fclose(fp);

	ret = gnutls_certificate_set_ocsp_status_request_file(xcred, ocspfile1, index1);
	if (ret < 0)
		fail("ocsp file set failed: %s\n", gnutls_strerror(ret));

	/* set OCSP response2 */
	ocspfile2 = get_tmpname(ocspname2);
	fp = fopen(ocspfile2, "wb");
	if (fp == NULL)
		fail("error in fopen\n");
	assert(fwrite(ocsp_ca3_localhost6_unknown.data, 1, ocsp_ca3_localhost6_unknown.size, fp)>0);
	fclose(fp);

	ret = gnutls_certificate_set_ocsp_status_request_file(xcred, ocspfile2, index2);
	if (ret < 0)
		fail("ocsp file set failed: %s\n", gnutls_strerror(ret));

	/* try to set a duplicate OCSP response */
	ocspfile3 = get_tmpname(ocspname3);
	fp = fopen(ocspfile3, "wb");
	if (fp == NULL)
		fail("error in fopen\n");
	assert(fwrite(ocsp_ca3_localhost_unknown_sha1.data, 1, ocsp_ca3_localhost_unknown_sha1.size, fp)>0);
	fclose(fp);

	ret = gnutls_certificate_set_ocsp_status_request_file(xcred, ocspfile3, index1);
	if (ret != 0)
		fail("setting duplicate didn't succeed as expected: %s\n", gnutls_strerror(ret));

	ret = gnutls_certificate_set_ocsp_status_request_file(xcred, ocspfile3, index2);
	if (ret != GNUTLS_E_OCSP_MISMATCH_WITH_CERTS)
		fail("setting invalid didn't fail as expected: %s\n", gnutls_strerror(ret));

	/* re-set the previous duplicate set for index1 to the expected*/
	ret = gnutls_certificate_set_ocsp_status_request_file(xcred, ocspfile1, index1);
	if (ret < 0)
		fail("ocsp file set failed: %s\n", gnutls_strerror(ret));

	/* set an intermediate CA OCSP response */
	fp = fopen(ocspfile3, "wb");
	if (fp == NULL)
		fail("error in fopen\n");
	assert(fwrite(ocsp_subca3_unknown.data, 1, ocsp_subca3_unknown.size, fp)>0);
	fclose(fp);

	ret = gnutls_certificate_set_ocsp_status_request_file(xcred, ocspfile3, index1);
	if (ret < 0)
		fail("setting subCA failed: %s\n", gnutls_strerror(ret));

	ret = gnutls_certificate_set_ocsp_status_request_file(xcred, ocspfile3, index2);
	if (ret < 0)
		fail("setting subCA failed: %s\n", gnutls_strerror(ret));


	ret = gnutls_certificate_set_x509_trust_mem(clicred, &ca3_cert, GNUTLS_X509_FMT_PEM);
	if (ret < 0) {
		fail("error in setting trust cert: %s\n", gnutls_strerror(ret));
	}

	test_cli_serv(xcred, clicred, "NORMAL:-ECDHE-ECDSA:-VERS-TLS-ALL:+VERS-TLS1.2", "localhost", &ocsp_ca3_localhost_unknown, check_response, NULL);
	test_cli_serv(xcred, clicred, "NORMAL:-ECDHE-ECDSA:-VERS-TLS-ALL:+VERS-TLS1.2", "localhost6", &ocsp_ca3_localhost6_unknown, check_response, NULL);

	test_cli_serv(xcred, clicred, "NORMAL:-ECDHE-ECDSA:-VERS-TLS-ALL:+VERS-TLS1.3", "localhost", &ocsp_ca3_localhost_unknown, check_response, NULL);
	test_cli_serv(xcred, clicred, "NORMAL:-ECDHE-ECDSA:-VERS-TLS-ALL:+VERS-TLS1.3", "localhost6", &ocsp_ca3_localhost6_unknown, check_response, NULL);

	gnutls_certificate_free_credentials(xcred);
	gnutls_certificate_free_credentials(clicred);
	gnutls_global_deinit();
	remove(ocspfile1);
	remove(ocspfile2);
	remove(ocspfile3);
	remove(certfile1);
	remove(certfile2);
}

#else
void doit(void)
{
	exit(77);
}
#endif
