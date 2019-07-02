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
#include <assert.h>

#ifdef ENABLE_OCSP

#include "ocsp-common.h"
#include "cert-common.h"
#include "utils.h"

/* Tests whether we can send and receive multiple OCSP responses
 * one for each certificate in a chain under TLS 1.3.
 */

static time_t mytime(time_t * t)
{
	time_t then = 1469186559;
	if (t)
		*t = then;

	return then;
}

static const gnutls_datum_t ocsp_resp_localhost[] = {
    { (void*)_ocsp_ca3_localhost_unknown, sizeof(_ocsp_ca3_localhost_unknown) },
    { NULL, 0}};

static const gnutls_datum_t ocsp_resp_localhost6[] = {
    { (void*)_ocsp_ca3_localhost6_unknown, sizeof(_ocsp_ca3_localhost6_unknown) },
    { (void*)_ocsp_subca3_unknown, sizeof(_ocsp_subca3_unknown) }};

typedef struct ctx_st {
	const char *name;
	const gnutls_datum_t *ocsp;
	unsigned nocsp;
} ctx_st;

static ctx_st test_localhost = {"single response", ocsp_resp_localhost, 1};
static ctx_st test_localhost6 = {"two responses", ocsp_resp_localhost6, 2};

#define myfail(fmt, ...) \
	fail("%s: "fmt, test->name, ##__VA_ARGS__)

static void check_response(gnutls_session_t session, void *priv)
{
	int ret;
	gnutls_datum_t resp;
	ctx_st *test = priv;
	unsigned i;

	assert(test != NULL);

	for (i=0;;i++) {
		ret = gnutls_ocsp_status_request_get2(session, i, &resp);
		if (ret == GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE)
			break;
		if (ret < 0) {
			if (test->ocsp[i].size == 0)
				return;
			myfail("no response was received\n");
		}

		if (test->ocsp[i].size == 0) {
			myfail("not expected response, but received one\n");
		}

		if (resp.size != test->ocsp[i].size) {
			myfail("did not receive the expected response size for %d\n", i);
		}

		if (memcmp(resp.data, test->ocsp[i].data, resp.size) != 0) {
			myfail("did not receive the expected response for %d\n", i);
		}
	}

	if (i != test->nocsp) {
		myfail("The number of OCSP responses received (%d) does not match the expected (%d)\n", i, test->nocsp);
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
	char certname1[TMPNAME_SIZE];
	char certname2[TMPNAME_SIZE];
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
	ret = gnutls_certificate_set_ocsp_status_request_mem(xcred, &test_localhost.ocsp[0], index1, GNUTLS_X509_FMT_DER);
	if (ret < 0)
		fail("ocsp file set failed: %s\n", gnutls_strerror(ret));

	/* set OCSP response2 */
	ret = gnutls_certificate_set_ocsp_status_request_mem(xcred, &test_localhost6.ocsp[0], index2, GNUTLS_X509_FMT_DER);
	if (ret < 0)
		fail("ocsp file set failed: %s\n", gnutls_strerror(ret));

	ret = gnutls_certificate_set_ocsp_status_request_mem(xcred, &test_localhost6.ocsp[1], index2, GNUTLS_X509_FMT_DER);
	if (ret < 0)
		fail("ocsp file set failed: %s\n", gnutls_strerror(ret));

	/* make sure that our invalid OCSP responses are not considered in verification
	 */
	gnutls_certificate_set_verify_flags(clicred, GNUTLS_VERIFY_DISABLE_CRL_CHECKS);
	if (gnutls_certificate_get_verify_flags(clicred) != GNUTLS_VERIFY_DISABLE_CRL_CHECKS)
		fail("error in gnutls_certificate_set_verify_flags\n");

	ret = gnutls_certificate_set_x509_trust_mem(clicred, &ca3_cert, GNUTLS_X509_FMT_PEM);
	if (ret < 0) {
		fail("error in setting trust cert: %s\n", gnutls_strerror(ret));
	}

	test_cli_serv(xcred, clicred, "NORMAL:-VERS-TLS-ALL:+VERS-TLS1.3", "localhost", &test_localhost, check_response, NULL);
	test_cli_serv(xcred, clicred, "NORMAL:-VERS-TLS-ALL:+VERS-TLS1.3", "localhost6", &test_localhost6, check_response, NULL);

	gnutls_certificate_free_credentials(xcred);
	gnutls_certificate_free_credentials(clicred);
	gnutls_global_deinit();
	remove(certfile1);
	remove(certfile2);
}

#else
void doit(void)
{
	exit(77);
}
#endif
