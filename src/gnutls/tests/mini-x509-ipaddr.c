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
#include <assert.h>
#include "utils.h"
#include "eagain-common.h"
#include "cert-common.h"

const char *side;

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "%s|<%d>| %s", side, level, str);
}

static time_t mytime(time_t * t)
{
	time_t then = 1490171562;

	if (t)
		*t = then;

	return then;
}

/* A unit test for GNUTLS_DT_IP_ADDRESS option of
 * gnutls_certificate_verify_peers().
 */

void doit(void)
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
	gnutls_typed_vdata_st data[2];
	gnutls_datum_t t;
	unsigned status;

	/* General init. */
	global_init();
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(6);

	gnutls_global_set_time_function(mytime);

	/* Init server */
	assert(gnutls_certificate_allocate_credentials(&serverx509cred)>=0);
	ret = gnutls_certificate_set_x509_key_mem(serverx509cred,
					    &server_ca3_ipaddr_cert, &server_ca3_key,
					    GNUTLS_X509_FMT_PEM);
	if (ret < 0) {
		fail("could not import cert: %s\n", gnutls_strerror(ret));
	}

	assert(gnutls_init(&server, GNUTLS_SERVER)>=0);
	gnutls_credentials_set(server, GNUTLS_CRD_CERTIFICATE,
				serverx509cred);
	assert(gnutls_set_default_priority(server)>=0);
	gnutls_transport_set_push_function(server, server_push);
	gnutls_transport_set_pull_function(server, server_pull);
	gnutls_transport_set_ptr(server, server);

	/* Init client */
	ret = gnutls_certificate_allocate_credentials(&clientx509cred);
	if (ret < 0)
		exit(1);

	ret = gnutls_certificate_set_x509_trust_mem(clientx509cred, &ca3_cert, GNUTLS_X509_FMT_PEM);
	if (ret < 0)
		exit(1);

	ret = gnutls_init(&client, GNUTLS_CLIENT);
	if (ret < 0)
		exit(1);

	ret = gnutls_credentials_set(client, GNUTLS_CRD_CERTIFICATE,
				clientx509cred);
	if (ret < 0)
		exit(1);

	assert(gnutls_set_default_priority(client) >= 0);
	gnutls_transport_set_push_function(client, client_push);
	gnutls_transport_set_pull_function(client, client_pull);
	gnutls_transport_set_ptr(client, client);

	HANDSHAKE(client, server);

	/* attempt to verify */
	{

		/* try hostname - which is invalid */
		memset(data, 0, sizeof(data));

		data[0].type = GNUTLS_DT_DNS_HOSTNAME;
		data[0].data = (void*)"localhost1";

		data[1].type = GNUTLS_DT_KEY_PURPOSE_OID;
		data[1].data = (void*)GNUTLS_KP_TLS_WWW_SERVER;

		ret = gnutls_certificate_verify_peers(client, data, 2, &status);
		if (ret < 0) {
			fail("could not verify certificate: %s\n", gnutls_strerror(ret));
			exit(1);
		}

		if (status == 0) {
			fail("should not have accepted!\n");
			exit(1);
		}

		/* try bogus IP */
		memset(data, 0, sizeof(data));

		data[0].type = GNUTLS_DT_IP_ADDRESS;
		data[0].data = (void*)"\x01\x00\x01\x02";
		data[0].size = 4;

		data[1].type = GNUTLS_DT_KEY_PURPOSE_OID;
		data[1].data = (void*)GNUTLS_KP_TLS_WWW_SERVER;

		ret = gnutls_certificate_verify_peers(client, data, 2, &status);
		if (ret < 0) {
			fail("could not verify certificate: %s\n", gnutls_strerror(ret));
			exit(1);
		}

		if (status == 0) {
			fail("should not have accepted!\n");
			exit(1);
		}

		/* try correct IP */
		memset(data, 0, sizeof(data));

		data[0].type = GNUTLS_DT_IP_ADDRESS;
		data[0].data = (void*)"\x7f\x00\x00\x01";
		data[0].size = 4;

		data[1].type = GNUTLS_DT_KEY_PURPOSE_OID;
		data[1].data = (void*)GNUTLS_KP_TLS_WWW_SERVER;

		ret = gnutls_certificate_verify_peers(client, data, 2, &status);
		if (ret < 0) {
			fail("could not verify certificate: %s\n", gnutls_strerror(ret));
			exit(1);
		}

		if (status != 0) {
			assert(gnutls_certificate_verification_status_print(status, GNUTLS_CRT_X509, &t, 0)>=0);
			fail("could not verify: %s/%.4x!\n", t.data, status);
		}

		/* try the other verification functions */
		ret = gnutls_certificate_verify_peers3(client, "127.0.0.1", &status);
		if (ret < 0) {
			fail("could not verify certificate: %s\n", gnutls_strerror(ret));
			exit(1);
		}

		if (status != 0) {
			assert(gnutls_certificate_verification_status_print(status, GNUTLS_CRT_X509, &t, 0)>=0);
			fail("could not verify: %s/%.4x!\n", t.data, status);
		}
	}

	{
		/* change the flags */
		gnutls_certificate_set_verify_flags(clientx509cred, GNUTLS_VERIFY_DO_NOT_ALLOW_IP_MATCHES);

		/* now the compatibility option should fail */
		ret = gnutls_certificate_verify_peers3(client, "127.0.0.1", &status);
		if (ret < 0) {
			fail("could not verify certificate: %s\n", gnutls_strerror(ret));
			exit(1);
		}

		if (status == 0) {
			fail("should not have accepted!\n");
			exit(1);
		}

		memset(data, 0, sizeof(data));

		data[0].type = GNUTLS_DT_DNS_HOSTNAME;
		data[0].data = (void*)"127.0.0.1";

		data[1].type = GNUTLS_DT_KEY_PURPOSE_OID;
		data[1].data = (void*)GNUTLS_KP_TLS_WWW_SERVER;

		ret = gnutls_certificate_verify_peers(client, data, 2, &status);
		if (ret < 0) {
			fail("could not verify certificate: %s\n", gnutls_strerror(ret));
			exit(1);
		}

		if (status == 0) {
			fail("should not have accepted!\n");
			exit(1);
		}

		/* try again the right one */
		memset(data, 0, sizeof(data));

		data[0].type = GNUTLS_DT_IP_ADDRESS;
		data[0].data = (void*)"\x7f\x00\x00\x01";
		data[0].size = 4;

		data[1].type = GNUTLS_DT_KEY_PURPOSE_OID;
		data[1].data = (void*)GNUTLS_KP_TLS_WWW_SERVER;

		ret = gnutls_certificate_verify_peers(client, data, 2, &status);
		if (ret < 0) {
			fail("could not verify certificate: %s\n", gnutls_strerror(ret));
			exit(1);
		}

		if (status != 0) {
			assert(gnutls_certificate_verification_status_print(status, GNUTLS_CRT_X509, &t, 0)>=0);
			fail("could not verify: %s/%.4x!\n", t.data, status);
		}
	}

	gnutls_bye(client, GNUTLS_SHUT_RDWR);
	gnutls_bye(server, GNUTLS_SHUT_RDWR);

	gnutls_deinit(client);
	gnutls_deinit(server);

	gnutls_certificate_free_credentials(serverx509cred);
	gnutls_certificate_free_credentials(clientx509cred);

	gnutls_global_deinit();
}
