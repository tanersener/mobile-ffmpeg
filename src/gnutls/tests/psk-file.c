/*
 * Copyright (C) 2004-2012 Free Software Foundation, Inc.
 * Copyright (C) 2013 Adam Sampson <ats@offog.org>
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
#if !defined(_WIN32)
#include <sys/wait.h>
#endif
#include <unistd.h>
#include <gnutls/gnutls.h>

#include "utils.h"

/* A very basic TLS client, with PSK authentication.
 */

const char *side = "";

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "%s|<%d>| %s", side, level, str);
}

#define MAX_BUF 1024
#define MSG "Hello TLS"

static void client(int sd, const char *user)
{
	int ret, ii;
	gnutls_session_t session;
	char buffer[MAX_BUF + 1];
	gnutls_psk_client_credentials_t pskcred;
	/* Need to enable anonymous KX specifically. */
	const gnutls_datum_t key = { (void *) "9e32cf7786321a828ef7668f09fb35db", 32 };
	const char *hint;

	global_init();
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(4711);

	side = "client";

	gnutls_psk_allocate_client_credentials(&pskcred);
	gnutls_psk_set_client_credentials(pskcred, user, &key,
					  GNUTLS_PSK_KEY_HEX);

	/* Initialize TLS session
	 */
	gnutls_init(&session, GNUTLS_CLIENT);

	/* Use default priorities */
	gnutls_priority_set_direct(session, "NORMAL:-KX-ALL:+PSK", NULL);

	/* put the anonymous credentials to the current session
	 */
	gnutls_credentials_set(session, GNUTLS_CRD_PSK, pskcred);

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

	/* check the hint */
	hint = gnutls_psk_client_get_hint(session);
	if (hint == NULL || strcmp(hint, "hint") != 0) {
		fail("client: hint is not the expected: %s\n", gnutls_psk_client_get_hint(session));
		goto end;
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

	gnutls_global_deinit();
}

/* This is a sample TLS 1.0 echo server, for PSK authentication.
 */

#define MAX_BUF 1024

static void server(int sd, const char *user, unsigned expect_fail)
{
gnutls_psk_server_credentials_t server_pskcred;
int ret;
gnutls_session_t session;
char buffer[MAX_BUF + 1];
char *psk_file = getenv("PSK_FILE");

	/* this must be called once in the program
	 */
	global_init();
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(4711);

	side = "server";

	if (psk_file == NULL)
		psk_file = (char*)"psk.passwd";

	gnutls_psk_allocate_server_credentials(&server_pskcred);
	gnutls_psk_set_server_credentials_hint(server_pskcred, "hint");
	ret = gnutls_psk_set_server_credentials_file(server_pskcred, psk_file);
	if (ret < 0) {
		fail("server: gnutls_psk_set_server_credentials_file failed (%s)\n\n",
		     gnutls_strerror(ret));
		return;
	}

	gnutls_init(&session, GNUTLS_SERVER);

	/* avoid calling all the priority functions, since the defaults
	 * are adequate.
	 */
	gnutls_priority_set_direct(session, "NORMAL:-KX-ALL:+PSK", NULL);

	gnutls_credentials_set(session, GNUTLS_CRD_PSK, server_pskcred);

	gnutls_transport_set_int(session, sd);
	ret = gnutls_handshake(session);
	if (ret < 0) {
		close(sd);
		gnutls_deinit(session);
		gnutls_psk_free_server_credentials(server_pskcred);
		if (expect_fail) {
			success("server: Handshake has failed - expected (%s)\n\n",
			     gnutls_strerror(ret));
		} else {
			fail("server: Handshake has failed (%s)\n\n",
			     gnutls_strerror(ret));
		}
		return;
	}
	if (debug)
		success("server: Handshake was completed\n");

	/* see the Getting peer's information example */
	/* print_info(session); */

	for (;;) {
		memset(buffer, 0, MAX_BUF + 1);
		gnutls_record_set_timeout(session, 10000);
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

	gnutls_global_deinit();

	if (debug)
		success("server: finished\n");
}

static
void run_test(const char *user, unsigned expect_fail)
{
	pid_t child;
	int err;
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
		close(sockets[1]);
		int status;
		/* parent */
		server(sockets[0], user, expect_fail);
		wait(&status);
	} else {
		close(sockets[0]);
		client(sockets[1], user);
	}
}

void doit(void)
{
	run_test("jas", 0);
	run_test("non-hex", 1);
}

#endif				/* _WIN32 */
