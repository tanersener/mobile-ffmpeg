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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <gnutls/gnutls.h>
#include <gnutls/abstract.h>
#include <gnutls/x509.h>
#include "utils.h"
#include "eagain-common.h"
#include "cert-common.h"

/* This tests whether the certificate callback receives the
 * corresponding to server certificates relative distinguished names.
 */

const char *side;

#define TOTAL_CAS 2

#define CA1_PTR &ca3_cert
#define CA2_PTR &ca_cert
gnutls_datum_t ca_dn[TOTAL_CAS];

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "%s|<%d>| %s", side, level, str);
}

static int
cert_callback(gnutls_session_t session,
	      const gnutls_datum_t * req_ca_rdn, int nreqs,
	      const gnutls_pk_algorithm_t * pk_algos,
	      int pk_algos_length, gnutls_pcert_st ** pcert,
	      unsigned int *pcert_length, gnutls_privkey_t * pkey)
{
	unsigned i;

	if (nreqs != TOTAL_CAS) {
		fail("cert_callback: found only %d RDNs\n", nreqs);
	}

	for (i = 0;i < TOTAL_CAS;i++) {
		if (req_ca_rdn[i].size != ca_dn[i].size) {
			fail("CA[%d] size mismatch\n", i);
		}

		if (memcmp(req_ca_rdn[i].data, ca_dn[i].data, ca_dn[i].size) != 0) {
			fail("CA[%d] data mismatch\n", i);
		}
	}

	success(" - Both (%d) CAs match\n\n", TOTAL_CAS);

	*pcert_length = 0;
	*pkey = NULL;
	*pcert = NULL;

	return 0;
}

static void start(const char *prio)
{
	int ret;
	/* Server stuff. */
	gnutls_certificate_credentials_t serverx509cred;
	gnutls_session_t server;
	int sret = GNUTLS_E_AGAIN;
	/* Client stuff. */
	gnutls_certificate_credentials_t clientx509cred;
	gnutls_session_t client;
	int cret = GNUTLS_E_AGAIN;

	success("testing %s\n", prio);

	/* General init. */
	global_init();
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(2);

	/* Init server */
	gnutls_certificate_allocate_credentials(&serverx509cred);

	assert(gnutls_certificate_set_x509_key_mem(serverx509cred, &server_cert, &server_key, GNUTLS_X509_FMT_PEM) >= 0);

	assert(gnutls_certificate_set_x509_trust_mem(serverx509cred, CA1_PTR, GNUTLS_X509_FMT_PEM) >= 0);
	assert(gnutls_certificate_set_x509_trust_mem(serverx509cred, CA2_PTR, GNUTLS_X509_FMT_PEM) >= 0);

	gnutls_init(&server, GNUTLS_SERVER);
	gnutls_credentials_set(server, GNUTLS_CRD_CERTIFICATE, serverx509cred);
	assert(gnutls_priority_set_direct(server,
				   prio, NULL) >= 0);
	gnutls_transport_set_push_function(server, server_push);
	gnutls_transport_set_pull_function(server, server_pull);
	gnutls_transport_set_ptr(server, server);
	gnutls_certificate_server_set_request(server, GNUTLS_CERT_REQUEST);

	/* Init client */
	/* Init client */
	ret = gnutls_certificate_allocate_credentials(&clientx509cred);
	if (ret < 0)
		exit(1);

	ret =
	    gnutls_certificate_set_x509_trust_mem(clientx509cred, &ca3_cert,
						  GNUTLS_X509_FMT_PEM);
	if (ret < 0)
		exit(1);

	gnutls_certificate_set_retrieve_function2(clientx509cred,
						  cert_callback);

	ret = gnutls_init(&client, GNUTLS_CLIENT);
	if (ret < 0)
		exit(1);

	ret = gnutls_credentials_set(client, GNUTLS_CRD_CERTIFICATE,
				     clientx509cred);
	if (ret < 0)
		exit(1);

	assert(gnutls_priority_set_direct(client, prio, NULL)>=0);
	gnutls_transport_set_push_function(client, client_push);
	gnutls_transport_set_pull_function(client, client_pull);
	gnutls_transport_set_ptr(client, client);

	HANDSHAKE(client, server);

	gnutls_bye(client, GNUTLS_SHUT_RDWR);
	gnutls_bye(server, GNUTLS_SHUT_RDWR);

	gnutls_deinit(client);
	gnutls_deinit(server);

	gnutls_certificate_free_credentials(serverx509cred);
	gnutls_certificate_free_credentials(clientx509cred);

	gnutls_global_deinit();

	reset_buffers();
}

static void find_dn(const gnutls_datum_t *cert, gnutls_datum_t *dn)
{
	gnutls_x509_crt_t crt;

	assert(gnutls_x509_crt_init(&crt)>=0);
	assert(gnutls_x509_crt_import(crt, cert, GNUTLS_X509_FMT_PEM) >= 0);
	assert(gnutls_x509_crt_get_raw_dn(crt, dn) >= 0);
	gnutls_x509_crt_deinit(crt);
}

void doit(void)
{
	find_dn(CA1_PTR, &ca_dn[0]);
	find_dn(CA2_PTR, &ca_dn[1]);
	start("NORMAL:-VERS-TLS-ALL:+VERS-TLS1.3");
	start("NORMAL:-VERS-TLS-ALL:+VERS-TLS1.2");
	gnutls_free(ca_dn[0].data);
	gnutls_free(ca_dn[1].data);
}
