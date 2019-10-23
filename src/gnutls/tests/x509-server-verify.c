/*
 * Copyright (C) 2015 Red Hat, Inc.
 * Copyright (C) 2019 Nikos Mavrogiannopoulos
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
#include "ocsp-common.h"

/* This tests gnutls_certificate_verify_peers2() to verify a client certificate
 * by server. */

const char *side;

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "%s|<%d>| %s", side, level, str);
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
	unsigned index1;

	success("testing %s\n", prio);

	/* General init. */
	global_init();
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(2);

	assert(gnutls_certificate_allocate_credentials(&serverx509cred)>=0);
	ret = gnutls_certificate_set_x509_key_mem2(serverx509cred, &server_ca3_localhost6_cert,
						   &server_ca3_key, GNUTLS_X509_FMT_PEM, NULL, 0);
	assert(ret>=0);
	index1 = ret;

	ret = gnutls_certificate_set_ocsp_status_request_mem(serverx509cred, &ocsp_ca3_localhost6_unknown_pem,
							     index1, GNUTLS_X509_FMT_PEM);
	assert(ret>=0);

	assert(gnutls_init(&server, GNUTLS_SERVER)>=0);
	gnutls_credentials_set(server, GNUTLS_CRD_CERTIFICATE, serverx509cred);
	assert(gnutls_priority_set_direct(server,
				   prio, NULL) >= 0);
	gnutls_transport_set_push_function(server, server_push);
	gnutls_transport_set_pull_function(server, server_pull);
	gnutls_transport_set_ptr(server, server);
	gnutls_certificate_server_set_request(server, GNUTLS_CERT_REQUEST);

	assert(gnutls_certificate_allocate_credentials(&clientx509cred) >= 0);
	assert(gnutls_certificate_set_x509_trust_mem(clientx509cred, &ca3_cert,
						     GNUTLS_X509_FMT_PEM) >= 0);

	ret = gnutls_certificate_set_x509_key_mem2(clientx509cred, &cli_ca3_cert_chain,
						   &cli_ca3_key, GNUTLS_X509_FMT_PEM, NULL, 0);
	assert(ret>=0);
	index1 = ret;

	ret = gnutls_certificate_set_ocsp_status_request_mem(clientx509cred, &ocsp_cli_ca3_good_pem,
							     index1, GNUTLS_X509_FMT_PEM);
	assert(ret>=0);

	assert(gnutls_certificate_set_x509_trust_mem(clientx509cred, &ca3_cert,
						     GNUTLS_X509_FMT_PEM) >= 0);


	assert(gnutls_init(&client, GNUTLS_CLIENT) >= 0);

	assert(gnutls_credentials_set(client, GNUTLS_CRD_CERTIFICATE,
				      clientx509cred) >= 0);

	assert(gnutls_priority_set_direct(client, prio, NULL)>=0);
	gnutls_transport_set_push_function(client, client_push);
	gnutls_transport_set_pull_function(client, client_pull);
	gnutls_transport_set_ptr(client, client);

	HANDSHAKE(client, server);

	/* check verify peers in server side */
	{
		unsigned status;

		ret = gnutls_certificate_verify_peers2(server, &status);
		if (ret < 0) {
			fail("could not verify client certificate: %s\n",
				gnutls_strerror(ret));
		}

		if (status == 0)
			fail("No CAs present but succeeded!\n");

		assert(gnutls_certificate_set_x509_trust_mem(serverx509cred, &ca3_cert,
							     GNUTLS_X509_FMT_PEM) >= 0);

		ret = gnutls_certificate_verify_peers2(server, &status);
		if (ret < 0) {
			fail("could not verify client certificate: %s\n",
				gnutls_strerror(ret));
		}

		if (status != 0)
			fail("Verification should have succeeded!\n");

		/* under TLS1.3 the client can send OCSP responses too */
		if (gnutls_protocol_get_version(server) == GNUTLS_TLS1_3) {
			ret = gnutls_ocsp_status_request_is_checked(server, GNUTLS_OCSP_SR_IS_AVAIL);
			assert(ret >= 0);

			ret = gnutls_ocsp_status_request_is_checked(server, 0);
			assert(ret >= 0);
		} else {
			ret = gnutls_ocsp_status_request_is_checked(server, GNUTLS_OCSP_SR_IS_AVAIL);
			assert(ret == 0);

			ret = gnutls_ocsp_status_request_is_checked(server, 0);
			assert(ret == 0);
		}
	}

	gnutls_bye(client, GNUTLS_SHUT_RDWR);
	gnutls_bye(server, GNUTLS_SHUT_RDWR);

	gnutls_deinit(client);
	gnutls_deinit(server);

	gnutls_certificate_free_credentials(serverx509cred);
	gnutls_certificate_free_credentials(clientx509cred);

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
