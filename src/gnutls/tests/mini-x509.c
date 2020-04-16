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
#include "utils.h"
#include "eagain-common.h"
#include "cert-common.h"
#include <assert.h>

const char *side;

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "%s|<%d>| %s", side, level, str);
}

static time_t mytime(time_t * t)
{
	time_t then = 1461671166;

	if (t)
		*t = then;

	return then;
}

static
void start(const char *prio, unsigned expect_max)
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

	success("trying %s\n", prio);

	/* General init. */
	global_init();
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(6);

	gnutls_global_set_time_function(mytime);

	/* Init server */
	gnutls_certificate_allocate_credentials(&serverx509cred);
	gnutls_certificate_set_x509_key_mem(serverx509cred,
					    &server_cert, &server_key,
					    GNUTLS_X509_FMT_PEM);

	gnutls_init(&server, GNUTLS_SERVER);
	gnutls_credentials_set(server, GNUTLS_CRD_CERTIFICATE,
				serverx509cred);
	assert(gnutls_priority_set_direct(server, prio, NULL)>=0);
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

	gnutls_priority_set_direct(client, prio, NULL);
	gnutls_transport_set_push_function(client, client_push);
	gnutls_transport_set_pull_function(client, client_pull);
	gnutls_transport_set_ptr(client, client);

	HANDSHAKE(client, server);

	assert((gnutls_session_get_flags(server) & GNUTLS_SFLAGS_CLI_REQUESTED_OCSP) != 0);
	assert((gnutls_session_get_flags(client) & GNUTLS_SFLAGS_CLI_REQUESTED_OCSP) != 0);

	/* check gnutls_certificate_get_ours() - client side */
	{
		const gnutls_datum_t *mcert;

		mcert = gnutls_certificate_get_ours(client);
		if (mcert != NULL) {
			fail("gnutls_certificate_get_ours(): failed\n");
			exit(1);
		}
	}

	assert(gnutls_certificate_type_get(server)==GNUTLS_CRT_X509);
	assert(gnutls_certificate_type_get(client)==GNUTLS_CRT_X509);

	/* check the number of certificates received and verify */
	{
		unsigned cert_list_size = 0;
		gnutls_typed_vdata_st data[2];
		unsigned status;

		memset(data, 0, sizeof(data));

		data[0].type = GNUTLS_DT_DNS_HOSTNAME;
		data[0].data = (void*)"localhost1";

		data[1].type = GNUTLS_DT_KEY_PURPOSE_OID;
		data[1].data = (void*)GNUTLS_KP_TLS_WWW_SERVER;

		gnutls_certificate_get_peers(client, &cert_list_size);
		if (cert_list_size < 2) {
			fail("received a certificate list of %d!\n", cert_list_size);
			exit(1);
		}

		ret = gnutls_certificate_verify_peers(client, data, 2, &status);
		if (ret < 0) {
			fail("could not verify certificate: %s\n", gnutls_strerror(ret));
			exit(1);
		}

		if (status == 0) {
			fail("should not have accepted!\n");
			exit(1);
		}

		data[0].type = GNUTLS_DT_DNS_HOSTNAME;
		data[0].data = (void*)"localhost";

		ret = gnutls_certificate_verify_peers(client, data, 2, &status);
		if (ret < 0) {
			fail("could not verify certificate: %s\n", gnutls_strerror(ret));
			exit(1);
		}

		if (status != 0) {
			fail("could not verify certificate: %.4x\n", status);
			exit(1);
		}

		/* check gnutls_certificate_verify_peers3 */
		ret = gnutls_certificate_verify_peers3(client, "localhost1", &status);
		if (ret < 0) {
			fail("could not verify certificate: %s\n", gnutls_strerror(ret));
			exit(1);
		}

		if (status == 0) {
			fail("should not have accepted!\n");
			exit(1);
		}

		ret = gnutls_certificate_verify_peers3(client, "localhost", &status);
		if (ret < 0) {
			fail("could not verify certificate: %s\n", gnutls_strerror(ret));
			exit(1);
		}

		if (status != 0) {
			fail("could not verify certificate: %.4x\n", status);
			exit(1);
		}

		/* check gnutls_certificate_verify_peers2 */
		ret = gnutls_certificate_verify_peers2(client, &status);
		if (ret < 0) {
			fail("could not verify certificate: %s\n", gnutls_strerror(ret));
			exit(1);
		}

		if (status != 0) {
			fail("could not verify certificate: %.4x\n", status);
			exit(1);
		}
	}

	{
		/* check the expiration and activation time legacy functions */
		time_t t;

		t = gnutls_certificate_activation_time_peers(client);
		if (t != 1396641545) {
			fail("unexpected activation time: %lu\n", (long unsigned)t);
		}

		if (sizeof(time_t) >= 8) {
			t = gnutls_certificate_expiration_time_peers(client);
			if (t != (time_t)253402300799UL) {
				fail("unexpected expiration time: %lu\n", (long unsigned)t);
			}
		}
	}

	if (expect_max) {
		if (gnutls_protocol_get_version(client) != GNUTLS_TLS_VERSION_MAX) {
			fail("The negotiated TLS protocol is not the maximum supported\n");
		}
	}

	if (gnutls_protocol_get_version(client) == GNUTLS_TLS1_2) {
		ret = gnutls_session_ext_master_secret_status(client);
		if (ret != 1) {
			fail("Extended master secret wasn't negotiated by default (client ret: %d)\n", ret);
		}

		ret = gnutls_session_ext_master_secret_status(server);
		if (ret != 1) {
			fail("Extended master secret wasn't negotiated by default (server ret: %d)\n", ret);
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
	start("NORMAL:-VERS-ALL:+VERS-TLS1.2", 0);
	start("NORMAL:-VERS-ALL:+VERS-TLS1.3", 0);
	start("NORMAL", 1);
}
