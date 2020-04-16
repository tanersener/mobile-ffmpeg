/*
 * Copyright (C) 2015-2017 Red Hat, Inc.
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

/* This tests gnutls_certificate_set_x509_key() */

const char *side;

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "%s|<%d>| %s", side, level, str);
}

static gnutls_privkey_t server_pkey = NULL;
static gnutls_pcert_st *server_pcert = NULL;
static gnutls_ocsp_data_st ocspdata[2];

#define OCSP_SIZE 16
#define OCSP_DATA "\xff\xff\xf0\xf0\xff\xff\xf0\xf0\xff\xff\xf0\xf0\xff\xff\xf0\xf0"

static int
server_cert_callback(gnutls_session_t session,
		     const struct gnutls_cert_retr_st *info,
		     gnutls_pcert_st **pcert,
		     unsigned int *pcert_length,
		     gnutls_ocsp_data_st **ocsp,
		     unsigned int *ocsp_length,
		     gnutls_privkey_t *pkey,
		     unsigned int *flags)
{
	int ret;
	gnutls_pcert_st *p;
	gnutls_privkey_t lkey;
	gnutls_x509_crt_t *certs;
	unsigned certs_size, i;

	if (server_pkey == NULL) {
		p = gnutls_malloc(2 * sizeof(*p));
		if (p == NULL)
			return -1;

		ocspdata[0].response.data = (void*)OCSP_DATA;
		ocspdata[0].response.size = OCSP_SIZE;
		ocspdata[0].exptime = 0;

		ocspdata[1].response.data = (void*)OCSP_DATA;
		ocspdata[1].response.size = OCSP_SIZE;
		ocspdata[1].exptime = 0;

		ret = gnutls_x509_crt_list_import2(&certs, &certs_size,
						   &server_ca3_localhost_cert_chain,
						   GNUTLS_X509_FMT_PEM, 0);
		if (ret < 0)
			return -1;
		ret = gnutls_pcert_import_x509_list(p, certs, &certs_size, 0);
		if (ret < 0)
			return -1;
		for (i = 0; i < certs_size; i++)
			gnutls_x509_crt_deinit(certs[i]);
		gnutls_free(certs);

		ret = gnutls_privkey_init(&lkey);
		if (ret < 0)
			return -1;

		ret =
		    gnutls_privkey_import_x509_raw(lkey, &server_ca3_key,
						   GNUTLS_X509_FMT_PEM, NULL,
						   0);
		if (ret < 0)
			return -1;

		server_pcert = p;
		server_pkey = lkey;

		*pcert = p;
		*pcert_length = 2;
		*pkey = lkey;
		*ocsp = ocspdata;
		*ocsp_length = 2;
	} else {
		*pcert = server_pcert;
		*pcert_length = 2;
		*pkey = server_pkey;
		*ocsp = ocspdata;
		*ocsp_length = 2;
	}

	return 0;
}

static void start(const char *prio)
{
	int ret;
	/* Server stuff. */
	gnutls_certificate_credentials_t scred;
	gnutls_session_t server;
	gnutls_datum_t response;
	int sret = GNUTLS_E_AGAIN;
	/* Client stuff. */
	gnutls_certificate_credentials_t ccred;
	gnutls_session_t client;
	int cret = GNUTLS_E_AGAIN;

	success("testing %s\n", prio);

	/* General init. */
	global_init();
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(4);

	/* Init server */
	gnutls_certificate_allocate_credentials(&scred);

	gnutls_certificate_set_retrieve_function3(scred,
						  server_cert_callback);

	gnutls_init(&server, GNUTLS_SERVER);
	gnutls_credentials_set(server, GNUTLS_CRD_CERTIFICATE, scred);
	assert(gnutls_priority_set_direct(server,
				   prio, NULL) >= 0);
	gnutls_transport_set_push_function(server, server_push);
	gnutls_transport_set_pull_function(server, server_pull);
	gnutls_transport_set_ptr(server, server);
	gnutls_certificate_server_set_request(server, GNUTLS_CERT_REQUEST);

	/* Init client */
	ret = gnutls_certificate_allocate_credentials(&ccred);
	if (ret < 0)
		exit(1);

	gnutls_certificate_set_verify_flags(ccred, GNUTLS_VERIFY_DISABLE_CRL_CHECKS);

	ret =
	    gnutls_certificate_set_x509_trust_mem(ccred, &ca3_cert,
						  GNUTLS_X509_FMT_PEM);
	if (ret < 0)
		exit(1);

	ret = gnutls_init(&client, GNUTLS_CLIENT);
	if (ret < 0)
		exit(1);

	ret = gnutls_credentials_set(client, GNUTLS_CRD_CERTIFICATE,
				     ccred);
	if (ret < 0)
		exit(1);

	assert(gnutls_priority_set_direct(client, prio, NULL)>=0);
	gnutls_transport_set_push_function(client, client_push);
	gnutls_transport_set_pull_function(client, client_pull);
	gnutls_transport_set_ptr(client, client);

	HANDSHAKE(client, server);

	assert((gnutls_session_get_flags(server) & GNUTLS_SFLAGS_CLI_REQUESTED_OCSP) != 0);
	assert((gnutls_session_get_flags(client) & GNUTLS_SFLAGS_CLI_REQUESTED_OCSP) != 0);

	ret = gnutls_ocsp_status_request_get(client, &response);
	if (ret != 0)
		fail("no response was found: %s\n", gnutls_strerror(ret));

	assert(response.size == OCSP_SIZE);
	assert(memcmp(response.data, OCSP_DATA, OCSP_SIZE) == 0);

	if (gnutls_protocol_get_version(client) == GNUTLS_TLS1_3) {
		ret = gnutls_ocsp_status_request_get2(client, 1, &response);
		if (ret != 0)
			fail("no response was found for 1: %s\n", gnutls_strerror(ret));

		assert(response.size == OCSP_SIZE);
		assert(memcmp(response.data, OCSP_DATA, OCSP_SIZE) == 0);
	}

	ret = gnutls_ocsp_status_request_get2(client, 2, &response);
	if (ret != GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE) {
		fail("found response in index 2: %s\n", gnutls_strerror(ret));
	}

	gnutls_bye(client, GNUTLS_SHUT_WR);
	gnutls_bye(server, GNUTLS_SHUT_WR);

	gnutls_deinit(client);
	gnutls_deinit(server);

	gnutls_certificate_free_credentials(scred);
	gnutls_certificate_free_credentials(ccred);

	gnutls_global_deinit();

	reset_buffers();
}

void doit(void)
{
	start("NORMAL:-VERS-TLS-ALL:+VERS-TLS1.3");
	start("NORMAL:-VERS-TLS-ALL:+VERS-TLS1.2");
	start("NORMAL:-VERS-TLS-ALL:+VERS-TLS1.1");
	start("NORMAL");
}
