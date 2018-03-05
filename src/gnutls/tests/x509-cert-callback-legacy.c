/*
 * Copyright (C) 2015 Red Hat, Inc.
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

static int
cert_callback(gnutls_session_t session,
	      const gnutls_datum_t * req_ca_rdn, int nreqs,
	      const gnutls_pk_algorithm_t * pk_algos,
	      int pk_algos_length,
	      gnutls_retr2_st *st)
{
	int ret;
	gnutls_x509_crt_t *crts;
	unsigned int crts_size;
	gnutls_x509_privkey_t pkey;

	if (gnutls_certificate_client_get_request_status(session) == 0) {
		fail("gnutls_certificate_client_get_request_status failed\n");
		return -1;
	}

	st->cert_type = GNUTLS_CRT_X509;

	ret = gnutls_x509_crt_list_import2(&crts, &crts_size, &cli_ca3_cert_chain, GNUTLS_X509_FMT_PEM,
		GNUTLS_X509_CRT_LIST_FAIL_IF_UNSORTED);
	if (ret < 0) {
		fail("error: %s\n", gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_x509_privkey_init(&pkey);
	if (ret < 0) {
		fail("error: %s\n", gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_x509_privkey_import(pkey, &cli_ca3_key, GNUTLS_X509_FMT_PEM);
	if (ret < 0) {
		fail("error: %s\n", gnutls_strerror(ret));
		exit(1);
	}

	st->cert.x509 = crts;
	st->ncerts = crts_size;

	st->key.x509 = pkey;
	st->deinit_all = 1;

	return 0;
}

static int
server_cert_callback(gnutls_session_t session,
	      const gnutls_datum_t * req_ca_rdn, int nreqs,
	      const gnutls_pk_algorithm_t * pk_algos,
	      int pk_algos_length,
	      gnutls_retr2_st *st)
{
	int ret;
	gnutls_x509_crt_t *crts;
	unsigned int crts_size;
	gnutls_x509_privkey_t pkey;

	st->cert_type = GNUTLS_CRT_X509;

	ret = gnutls_x509_crt_list_import2(&crts, &crts_size, &server_ca3_cert_chain, GNUTLS_X509_FMT_PEM,
		GNUTLS_X509_CRT_LIST_FAIL_IF_UNSORTED);
	if (ret < 0) {
		fail("error: %s\n", gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_x509_privkey_init(&pkey);
	if (ret < 0) {
		fail("error: %s\n", gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_x509_privkey_import(pkey, &server_ca3_key, GNUTLS_X509_FMT_PEM);
	if (ret < 0) {
		fail("error: %s\n", gnutls_strerror(ret));
		exit(1);
	}

	st->cert.x509 = crts;
	st->ncerts = crts_size;

	st->key.x509 = pkey;
	st->deinit_all = 1;

	return 0;
}

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

	/* General init. */
	global_init();
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(2);

	/* Init server */
	gnutls_certificate_allocate_credentials(&serverx509cred);

	gnutls_certificate_set_retrieve_function(serverx509cred,
						 server_cert_callback);

	gnutls_init(&server, GNUTLS_SERVER);
	gnutls_credentials_set(server, GNUTLS_CRD_CERTIFICATE, serverx509cred);
	gnutls_priority_set_direct(server,
				   "NORMAL:-CIPHER-ALL:+AES-128-GCM", NULL);
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

	gnutls_certificate_set_retrieve_function(clientx509cred,
						 cert_callback);

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

	if (gnutls_certificate_get_ours(client) == NULL) {
		fail("client certificate was not sent!\n");
		exit(1);
	}

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
		ret =
		    gnutls_x509_crt_import(crt, &server_ca3_localhost_cert_chain,
					   GNUTLS_X509_FMT_PEM);
		if (ret < 0) {
			fail("gnutls_x509_crt_import: %s\n",
			     gnutls_strerror(ret));
			exit(1);
		}

		ret = gnutls_x509_crt_export2(crt, GNUTLS_X509_FMT_DER, &scert);
		if (ret < 0) {
			fail("gnutls_x509_crt_export2: %s\n",
			     gnutls_strerror(ret));
			exit(1);
		}
		gnutls_x509_crt_deinit(crt);

		if (scert.size != mcert->size
		    || memcmp(scert.data, mcert->data, mcert->size) != 0) {
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
		ret =
		    gnutls_x509_crt_import(crt, &cli_ca3_cert_chain,
					   GNUTLS_X509_FMT_PEM);
		if (ret < 0) {
			fail("gnutls_x509_crt_import: %s\n",
			     gnutls_strerror(ret));
			exit(1);
		}

		ret = gnutls_x509_crt_export2(crt, GNUTLS_X509_FMT_DER, &ccert);
		if (ret < 0) {
			fail("gnutls_x509_crt_export2: %s\n",
			     gnutls_strerror(ret));
			exit(1);
		}
		gnutls_x509_crt_deinit(crt);

		if (ccert.size != mcert->size
		    || memcmp(ccert.data, mcert->data, mcert->size) != 0) {
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
		data[0].data = (void *)"localhost1";

		data[1].type = GNUTLS_DT_KEY_PURPOSE_OID;
		data[1].data = (void *)GNUTLS_KP_TLS_WWW_SERVER;

		gnutls_certificate_get_peers(client, &cert_list_size);
		if (cert_list_size != 2) {
			fprintf(stderr, "received a certificate list of %d!\n",
				cert_list_size);
			exit(1);
		}

		ret = gnutls_certificate_verify_peers(client, data, 2, &status);
		if (ret < 0) {
			fprintf(stderr, "could not verify certificate: %s\n",
				gnutls_strerror(ret));
			exit(1);
		}

		if (status == 0) {
			fprintf(stderr, "should not have accepted!\n");
			exit(1);
		}

		/* check with wrong purpose */
		data[0].type = GNUTLS_DT_DNS_HOSTNAME;
		data[0].data = (void *)"localhost";

		data[1].type = GNUTLS_DT_KEY_PURPOSE_OID;
		data[1].data = (void *)GNUTLS_KP_TLS_WWW_CLIENT;

		gnutls_certificate_get_peers(client, &cert_list_size);
		if (cert_list_size != 2) {
			fprintf(stderr, "received a certificate list of %d!\n",
				cert_list_size);
			exit(1);
		}

		ret = gnutls_certificate_verify_peers(client, data, 2, &status);
		if (ret < 0) {
			fprintf(stderr, "could not verify certificate: %s\n",
				gnutls_strerror(ret));
			exit(1);
		}

		if (status == 0) {
			fprintf(stderr, "should not have accepted!\n");
			exit(1);
		}

		/* check with correct purpose */
		data[0].type = GNUTLS_DT_DNS_HOSTNAME;
		data[0].data = (void *)"localhost";

		data[1].type = GNUTLS_DT_KEY_PURPOSE_OID;
		data[1].data = (void *)GNUTLS_KP_TLS_WWW_SERVER;

		ret = gnutls_certificate_verify_peers(client, data, 2, &status);
		if (ret < 0) {
			fprintf(stderr, "could not verify certificate: %s\n",
				gnutls_strerror(ret));
			exit(1);
		}

		if (status != 0) {
			fprintf(stderr, "could not verify certificate: %.4x\n",
				status);
			exit(1);
		}
	}

	if (gnutls_certificate_client_get_request_status(client) == 0) {
		fail("gnutls_certificate_client_get_request_status - 2 failed\n");
		exit(1);
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
