/*
 * Copyright (C) 2016 Red Hat, Inc.
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
#include "utils.h"
#include "eagain-common.h"
#include "cert-common.h"

const char *side;

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "%s|<%d>| %s", side, level, str);
}

#define TESTDATA "xxxtesttest1234"

/* counts the number of calls of send_testdata() within a handshake */
enum {
	TEST_SEND_RECV,
	TEST_RECV_SEND,
	TEST_HANDSHAKE_CALL,
	TEST_BYE,
	TESTNO_MAX
};

#define myfail(fmt, ...) \
	fail("%s%s %d: "fmt, dtls?"-dtls":"", name, testno, ##__VA_ARGS__)

static void try(const char *name, unsigned testno, unsigned fs,
		const char *prio, unsigned dhsize,
		unsigned dtls)
{
	int ret;
	/* Server stuff. */
	gnutls_certificate_credentials_t serverx509cred;
	gnutls_anon_client_credentials_t clientanoncred;
	gnutls_anon_server_credentials_t serveranoncred;
	gnutls_session_t server;
	int sret = GNUTLS_E_AGAIN;
	char buffer[512];
	/* Client stuff. */
	gnutls_certificate_credentials_t clientx509cred;
	gnutls_session_t client;
	int cret = GNUTLS_E_AGAIN;
	const gnutls_datum_t p3_2048 =
	    { (void *)pkcs3_2048, strlen(pkcs3_2048) };
	const gnutls_datum_t p3_3072 =
	    { (void *)pkcs3_3072, strlen(pkcs3_3072) };
	gnutls_dh_params_t dh_params;
	unsigned flags = 0;

	if (testno == TEST_HANDSHAKE_CALL && fs == 0)
		return;

	if (dtls)
		flags |= GNUTLS_DATAGRAM|GNUTLS_NONBLOCK;

	/* General init. */
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(999);

	gnutls_dh_params_init(&dh_params);

	if (dhsize < 3072) {
		ret = gnutls_dh_params_import_pkcs3(dh_params, &p3_2048,
						    GNUTLS_X509_FMT_PEM);
	} else {
		ret = gnutls_dh_params_import_pkcs3(dh_params, &p3_3072,
						    GNUTLS_X509_FMT_PEM);
	}

	/* Init server */
	gnutls_anon_allocate_server_credentials(&serveranoncred);
	gnutls_anon_set_server_dh_params(serveranoncred, dh_params);

	gnutls_certificate_allocate_credentials(&serverx509cred);
	gnutls_certificate_set_x509_key_mem(serverx509cred,
					    &server_cert, &server_key,
					    GNUTLS_X509_FMT_PEM);
	gnutls_certificate_set_x509_key_mem(serverx509cred,
					    &server_ecc_cert, &server_ecc_key,
					    GNUTLS_X509_FMT_PEM);
	gnutls_certificate_set_dh_params(serverx509cred, dh_params);

	gnutls_init(&server, GNUTLS_SERVER|flags);
	gnutls_credentials_set(server, GNUTLS_CRD_CERTIFICATE, serverx509cred);

	gnutls_credentials_set(server, GNUTLS_CRD_ANON, serveranoncred);

	gnutls_priority_set_direct(server, prio, NULL);
	gnutls_transport_set_push_function(server, server_push);
	gnutls_transport_set_pull_function(server, server_pull);
	gnutls_transport_set_pull_timeout_function(server, server_pull_timeout_func);
	gnutls_transport_set_ptr(server, server);

	/* Init client */

	gnutls_anon_allocate_client_credentials(&clientanoncred);
	ret = gnutls_certificate_allocate_credentials(&clientx509cred);
	if (ret < 0)
		exit(1);

	ret =
	    gnutls_certificate_set_x509_trust_mem(clientx509cred, &ca_cert,
						  GNUTLS_X509_FMT_PEM);
	if (ret < 0)
		exit(1);

	ret = gnutls_init(&client, GNUTLS_CLIENT|GNUTLS_ENABLE_FALSE_START|flags);
	if (ret < 0)
		exit(1);

	ret = gnutls_credentials_set(client, GNUTLS_CRD_CERTIFICATE,
				     clientx509cred);
	if (ret < 0)
		exit(1);

	ret = gnutls_credentials_set(client, GNUTLS_CRD_ANON, clientanoncred);
	if (ret < 0)
		exit(1);

	ret = gnutls_priority_set_direct(client, prio, NULL);
	if (ret < 0)
		exit(1);

	gnutls_transport_set_push_function(client, client_push);
	gnutls_transport_set_pull_function(client, client_pull);
	gnutls_transport_set_pull_timeout_function(client, client_pull_timeout_func);
	gnutls_transport_set_ptr(client, client);

	HANDSHAKE(client, server);

	if ((gnutls_session_get_flags(client) & GNUTLS_SFLAGS_FALSE_START)
	    && !fs) {
		myfail("%d: false start occurred but not expected\n", __LINE__);
	}

	if (!(gnutls_session_get_flags(client) & GNUTLS_SFLAGS_FALSE_START)
	    && fs) {
		myfail("%d: false start expected but not happened\n", __LINE__);
	}

	if (testno == TEST_SEND_RECV) {
		side = "client";
		ret =
		    gnutls_record_send(client, TESTDATA, sizeof(TESTDATA) - 1);
		if (ret < 0) {
			myfail("%d: error sending false start data: %s\n",
				__LINE__, gnutls_strerror(ret));
			exit(1);
		}

		side = "server";
		/* verify whether the server received the expected data */
		ret = gnutls_record_recv(server, buffer, sizeof(buffer));
		if (ret < 0) {
			myfail("%d: error receiving data: %s\n", __LINE__,
				gnutls_strerror(ret));
		}

		if (ret != sizeof(TESTDATA) - 1) {
			myfail("%d: error in received data size\n", __LINE__);
		}

		if (memcmp(buffer, TESTDATA, ret) != 0) {
			myfail("%d: error in received data\n", __LINE__);
		}

		/* check handshake completion */
		ret =
		    gnutls_record_send(server, TESTDATA, sizeof(TESTDATA) - 1);
		if (ret < 0) {
			myfail("%d: error sending false start data: %s\n",
				__LINE__, gnutls_strerror(ret));
			exit(1);
		}

		side = "client";
		do {
			ret =
			    gnutls_record_recv(client, buffer, sizeof(buffer));
		} while (ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED);
		if (ret < 0) {
			myfail("%d: error receiving data: %s\n", __LINE__,
				gnutls_strerror(ret));
		}
	} else if (testno == TEST_RECV_SEND) {
		side = "server";
		ret =
		    gnutls_record_send(server, TESTDATA, sizeof(TESTDATA) - 1);
		if (ret < 0) {
			myfail("%d: error sending false start data: %s\n",
				__LINE__, gnutls_strerror(ret));
			exit(1);
		}

		side = "client";
		/* verify whether the server received the expected data */
		ret = gnutls_record_recv(client, buffer, sizeof(buffer));
		if (ret < 0) {
			myfail("%d: error receiving data: %s\n", __LINE__,
				gnutls_strerror(ret));
		}

		if (ret != sizeof(TESTDATA) - 1) {
			myfail("%d: error in received data size\n", __LINE__);
		}

		if (memcmp(buffer, TESTDATA, ret) != 0) {
			myfail("%d: error in received data\n", __LINE__);
		}
	} else if (testno == TEST_HANDSHAKE_CALL) {
		/* explicit completion by caller */
		ret = gnutls_handshake(client);
		if (ret != GNUTLS_E_HANDSHAKE_DURING_FALSE_START) {
			myfail
			    ("%d: error in explicit handshake after false start: %s\n",
			     __LINE__, gnutls_strerror(ret));
			exit(1);
		}

		goto exit;
	}

	side = "server";
	ret = gnutls_bye(server, GNUTLS_SHUT_WR);
	if (ret < 0) {
		myfail("%d: error in server bye: %s\n", __LINE__,
			gnutls_strerror(ret));
	}

	side = "client";
	ret = gnutls_bye(client, GNUTLS_SHUT_RDWR);
	if (ret < 0) {
		myfail("%d: error in client bye: %s\n", __LINE__,
			gnutls_strerror(ret));
	}

	success("%5s%s \tok\n", dtls?"dtls-":"", name);
 exit:
	gnutls_deinit(client);
	gnutls_deinit(server);

	gnutls_dh_params_deinit(dh_params);
	gnutls_anon_free_server_credentials(serveranoncred);
	gnutls_anon_free_client_credentials(clientanoncred);
	gnutls_certificate_free_credentials(serverx509cred);
	gnutls_certificate_free_credentials(clientx509cred);
}

void doit(void)
{
	unsigned i, j;

	global_init();


	for (j=0;j<2;j++) {
		for (i = 0; i < TESTNO_MAX; i++) {
			try("anon-dh  :", i, 0, "NORMAL:-KX-ALL:+ANON-DH", 3072, j);
			reset_buffers();
			try("anon-ecdh:", i, 0, "NORMAL:-KX-ALL:+ANON-ECDH", 2048, j);
			reset_buffers();
			try("ecdhe-rsa:", i, 1, "NORMAL:-KX-ALL:+ECDHE-RSA", 2048, j);
			reset_buffers();
			try("ecdhe-x25519-rsa:", i, 1, "NORMAL:-KX-ALL:+ECDHE-RSA:-CURVE-ALL:+CURVE-X25519", 2048, j);
			reset_buffers();
			try("ecdhe-ecdsa:", i, 1, "NORMAL:-KX-ALL:+ECDHE-ECDSA", 2048, j);
			reset_buffers();
			try("dhe-rsa-2048:", i, 0, "NORMAL:-KX-ALL:+DHE-RSA", 2048, j);
			reset_buffers();
			try("dhe-rsa-3072:", i, 1, "NORMAL:-KX-ALL:+DHE-RSA", 3072, j);
			reset_buffers();
		}
	}
	gnutls_global_deinit();
}
