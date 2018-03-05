/*
 * Copyright (C) 2004-2012 Free Software Foundation, Inc.
 * Copyright (C) 2013 Adam Sampson <ats@offog.org>
 * Copyright (C) 2013-2017 Nikos Mavrogiannopoulos
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

/* Parts copied from GnuTLS example programs. */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#if defined(_WIN32)

/* socketpair isn't supported on Win32. */
int main(int argc, char **argv)
{
	exit(77);
}

#else

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <assert.h>
#include <gnutls/gnutls.h>
#include "cert-common.h"
#include "utils.h"

/* A very basic TLS client, with PSK authentication.
 */

const char *side = "";

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "%s|<%d>| %s", side, level, str);
}

static
int psk_cli_func(gnutls_session_t session, char **username, gnutls_datum_t *key)
{
	*username = gnutls_malloc(5);
	assert(*username != NULL);
	strcpy(*username, "test");

	key->data = gnutls_malloc(4);
	assert(key->data != NULL);
	key->data[0] = 0xDE;
	key->data[1] = 0xAD;
	key->data[2] = 0xBE;
	key->data[3] = 0xEF;
	key->size = 4;

	return 0;
}

#define MAX_BUF 1024
#define MSG "Hello TLS"

static void client(int sd)
{
	int ret, ii;
	gnutls_session_t session;
	gnutls_certificate_credentials_t clientx509cred;
	char buffer[MAX_BUF + 1];
	gnutls_psk_client_credentials_t pskcred;
	/* Need to enable anonymous KX specifically. */

	global_init();
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(4711);

	side = "client";

	gnutls_certificate_allocate_credentials(&clientx509cred);

	gnutls_psk_allocate_client_credentials(&pskcred);
	gnutls_psk_set_client_credentials_function(pskcred, psk_cli_func);

	/* Initialize TLS session
	 */
	gnutls_init(&session, GNUTLS_CLIENT);

	/* Use default priorities */
	gnutls_priority_set_direct(session, "NORMAL:-KX-ALL:+RSA-PSK",
				   NULL);

	/* put the anonymous credentials to the current session
	 */
	gnutls_credentials_set(session, GNUTLS_CRD_PSK, pskcred);
	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE,
				clientx509cred);

	gnutls_handshake_set_timeout(session, 20 * 1000);
	gnutls_transport_set_int(session, sd);

	/* Perform the TLS handshake
	 */
	ret = gnutls_handshake(session);

	if (ret < 0) {
		fail("client: Handshake failed\n");
		gnutls_perror(ret);
		goto end;
	} else {
		if (debug)
			success("client: Handshake was completed\n");
	}

	gnutls_record_send(session, MSG, strlen(MSG));

	ret = gnutls_record_recv(session, buffer, MAX_BUF);
	if (ret == 0) {
		if (debug)
			success
			    ("client: Peer has closed the TLS connection\n");
		goto end;
	} else if (ret < 0) {
		fail("client: Error: %s\n", gnutls_strerror(ret));
		goto end;
	}

	if (debug) {
		printf("- Received %d bytes: ", ret);
		for (ii = 0; ii < ret; ii++) {
			fputc(buffer[ii], stdout);
		}
		fputs("\n", stdout);
	}

	gnutls_bye(session, GNUTLS_SHUT_RDWR);

      end:

	close(sd);

	gnutls_deinit(session);

	gnutls_psk_free_client_credentials(pskcred);
	gnutls_certificate_free_credentials(clientx509cred);

	gnutls_global_deinit();
}

/* This is a sample TLS 1.0 echo server, for PSK authentication.
 */

#define MAX_BUF 1024

static int
psk_server_func(gnutls_session_t session, const char *username,
	gnutls_datum_t * key)
{
	if (debug)
		printf("psk: username %s\n", username);

	if (strcmp(username, "test") != 0) {
		fail("error in received username: '%s'\n", username);
	}

	key->data = gnutls_malloc(4);
	assert(key->data != NULL);
	key->data[0] = 0xDE;
	key->data[1] = 0xAD;
	key->data[2] = 0xBE;
	key->data[3] = 0xEF;
	key->size = 4;
	return 0;
}

int err, ret;
char topbuf[512];
gnutls_session_t session;
char buffer[MAX_BUF + 1];
int optval = 1;

static void server(int sd)
{
	gnutls_certificate_credentials_t serverx509cred;
	gnutls_psk_server_credentials_t server_pskcred;

	/* this must be called once in the program
	 */
	global_init();
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(4711);

	side = "server";

	gnutls_psk_allocate_server_credentials(&server_pskcred);
	gnutls_psk_set_server_credentials_function(server_pskcred,
						   psk_server_func);
	gnutls_psk_set_server_credentials_hint(server_pskcred, "hint");
	gnutls_certificate_allocate_credentials(&serverx509cred);
	gnutls_certificate_set_x509_key_mem(serverx509cred,
					    &server_cert, &server_key,
					    GNUTLS_X509_FMT_PEM);

	gnutls_init(&session, GNUTLS_SERVER);

	/* avoid calling all the priority functions, since the defaults
	 * are adequate.
	 */
	gnutls_priority_set_direct(session, "NORMAL:-KX-ALL:+RSA-PSK",
				   NULL);


	gnutls_credentials_set(session, GNUTLS_CRD_PSK, server_pskcred);
	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE,
				serverx509cred);

	gnutls_handshake_set_timeout(session, 20 * 1000);
	gnutls_transport_set_int(session, sd);
	ret = gnutls_handshake(session);
	if (ret < 0) {
		close(sd);
		gnutls_deinit(session);
		fail("server: Handshake has failed (%s)\n\n",
		     gnutls_strerror(ret));
		return;
	}
	if (debug)
		success("server: Handshake was completed\n");

	/* see the Getting peer's information example */
	/* print_info(session); */

	for (;;) {
		memset(buffer, 0, MAX_BUF + 1);
		ret = gnutls_record_recv(session, buffer, MAX_BUF);

		if (ret == 0) {
			if (debug)
				success
				    ("server: Peer has closed the GnuTLS connection\n");
			break;
		} else if (ret < 0) {
			fail("server: Received corrupted data(%d). Closing...\n", ret);
			break;
		} else if (ret > 0) {
			/* echo data back to the client
			 */
			gnutls_record_send(session, buffer,
					   strlen(buffer));
		}
	}
	/* do not wait for the peer to close the connection.
	 */
	gnutls_bye(session, GNUTLS_SHUT_WR);

	close(sd);
	gnutls_deinit(session);

	gnutls_psk_free_server_credentials(server_pskcred);
	gnutls_certificate_free_credentials(serverx509cred);

	gnutls_global_deinit();

	if (debug)
		success("server: finished\n");
}

void doit(void)
{
	pid_t child;
	int sockets[2];

	err = socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
	if (err == -1) {
		perror("socketpair");
		fail("socketpair failed\n");
		return;
	}

	child = fork();
	if (child < 0) {
		perror("fork");
		fail("fork");
		return;
	}

	if (child) {
		int status;
		/* parent */
		server(sockets[0]);
		wait(&status);
	} else
		client(sockets[1]);
}

#endif				/* _WIN32 */
