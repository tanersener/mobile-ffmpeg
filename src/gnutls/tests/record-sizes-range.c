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
#include "eagain-common.h"

#include "utils.h"

const char *side = "";

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "%s|<%d>| %s", side, level, str);
}

/* This test attempts to transfer various sizes using AES-128-CBC.
 */

#define MAX_BUF 32*1024
#define MAX_SEND 16384
static char buffer1[MAX_BUF + 1];
static char buffer[MAX_BUF + 1];

static void try_send(gnutls_session_t client, gnutls_session_t server,
		     void *b1, ssize_t b1_size, void *b2, ssize_t b2_size,
		     gnutls_range_st * range)
{
	int ret, recvd;

	/* Try sending various other sizes */
	ret = gnutls_record_send_range(client, b1, b1_size, range);
	if (ret < 0) {
		fprintf(stderr, "Error sending %d bytes: %s\n",
			(int) b1_size, gnutls_strerror(ret));
		exit(1);
	}

	if (ret != b1_size) {
		fprintf(stderr, "Couldn't send %d bytes\n", (int) b1_size);
		exit(1);
	}

	recvd = 0;
	do {
		ret = gnutls_record_recv(server, b2, b2_size);
		if (ret < 0) {
			fprintf(stderr, "Error receiving %d bytes: %s\n",
				(int) b2_size, gnutls_strerror(ret));
			exit(1);
		}
		recvd += ret;
	}
	while (recvd < b1_size);

	if (recvd != b1_size) {
		fprintf(stderr, "Couldn't receive %d bytes, received %d\n",
			(int) b1_size, recvd);
		exit(1);
	}

}

void doit(void)
{
	/* Server stuff. */
	gnutls_anon_server_credentials_t s_anoncred;
	const gnutls_datum_t p3 =
	    { (unsigned char *) pkcs3, strlen(pkcs3) };
	static gnutls_dh_params_t dh_params;
	gnutls_session_t server;
	int sret = GNUTLS_E_AGAIN;
	/* Client stuff. */
	gnutls_anon_client_credentials_t c_anoncred;
	gnutls_session_t client;
	int cret = GNUTLS_E_AGAIN;
	/* Need to enable anonymous KX specifically. */
	gnutls_range_st range;

	/* General init. */
	global_init();
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(4711);

	/* Init server */
	gnutls_anon_allocate_server_credentials(&s_anoncred);
	gnutls_dh_params_init(&dh_params);
	gnutls_dh_params_import_pkcs3(dh_params, &p3, GNUTLS_X509_FMT_PEM);
	gnutls_anon_set_server_dh_params(s_anoncred, dh_params);
	gnutls_init(&server, GNUTLS_SERVER);
	gnutls_priority_set_direct(server,
				   "NONE:+VERS-TLS1.2:+AES-128-CBC:+MAC-ALL:+SIGN-ALL:+COMP-NULL:+ANON-DH",
				   NULL);
	gnutls_credentials_set(server, GNUTLS_CRD_ANON, s_anoncred);
	gnutls_transport_set_push_function(server, server_push);
	gnutls_transport_set_pull_function(server, server_pull);
	gnutls_transport_set_ptr(server, server);

	/* Init client */
	gnutls_anon_allocate_client_credentials(&c_anoncred);
	gnutls_init(&client, GNUTLS_CLIENT);
	gnutls_priority_set_direct(client,
				   "NONE:+VERS-TLS1.2:+CIPHER-ALL:+MAC-ALL:+SIGN-ALL:+COMP-NULL:+ANON-DH",
				   NULL);
	gnutls_credentials_set(client, GNUTLS_CRD_ANON, c_anoncred);
	gnutls_transport_set_push_function(client, client_push);
	gnutls_transport_set_pull_function(client, client_pull);
	gnutls_transport_set_ptr(client, client);

	memset(buffer1, 0, sizeof(buffer1));
	HANDSHAKE(client, server);

	if (debug)
		success("Handshake established\n");

	memset(buffer1, 1, MAX_BUF);

	range.low = 1024;
	range.high = MAX_SEND;


	try_send(client, server, buffer1, MAX_SEND, buffer, MAX_BUF, &range);
	try_send(client, server, buffer1, 1024, buffer, MAX_BUF, &range);
	try_send(client, server, buffer1, 4096, buffer, MAX_BUF, &range);
	/*try_send(client, server, buffer1, 128, buffer, MAX_BUF, &range) */ ;


	if (debug)
		fputs("\n", stdout);


	gnutls_bye(client, GNUTLS_SHUT_RDWR);
	gnutls_bye(server, GNUTLS_SHUT_RDWR);

	gnutls_deinit(client);
	gnutls_deinit(server);

	gnutls_anon_free_client_credentials(c_anoncred);
	gnutls_anon_free_server_credentials(s_anoncred);

	gnutls_dh_params_deinit(dh_params);

	gnutls_global_deinit();
}
