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

/* This program tests whether the second client hello contains the same
 * random value.
 */

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
#include <signal.h>
#endif
#include <unistd.h>
#include <gnutls/gnutls.h>
#include <assert.h>

#include "utils.h"
#include "cert-common.h"

const char *side = "";

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "%s|<%d>| %s", side, level, str);
}

unsigned char crandom[32];
static unsigned cb_called = 0;

static int client_hello_callback(gnutls_session_t session, unsigned int htype,
        unsigned post, unsigned int incoming, const gnutls_datum_t *msg)
{
	unsigned ok = 0, i;

	if (htype == GNUTLS_HANDSHAKE_CLIENT_HELLO && post == GNUTLS_HOOK_POST) {
		if (cb_called == 0) {
			/* save first value */
			gnutls_datum_t tmp;
			gnutls_session_get_random(session, &tmp, NULL);
			assert(tmp.size == 32);

			memcpy(crandom, tmp.data, tmp.size);
			cb_called++;

			/* check if uninitialized */
			for (i=0;i<32;i++) {
				if (crandom[i] != 0) {
					ok = 1;
					break;
				}
			}
			if (!ok) {
				fail("the random value seems uninitialized\n");
			}
		} else { /* verify it is the same */
			gnutls_datum_t tmp;
			gnutls_session_get_random(session, &tmp, NULL);

			assert(tmp.size == 32);
			if (memcmp(tmp.data, crandom, tmp.size) != 0) {
				fail("the random values differ!\n");
			}
			cb_called++;
		}
	}

	return 0;
}

static void client(int sd)
{
	int ret;
	gnutls_session_t session;
	gnutls_certificate_credentials_t clientx509cred;

	global_init();
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(4711);

	side = "client";

	gnutls_certificate_allocate_credentials(&clientx509cred);

	/* Initialize TLS session
	 */
	assert(gnutls_init(&session, GNUTLS_CLIENT|GNUTLS_KEY_SHARE_TOP)>=0);

	/* Use default priorities, i.e., SECP256R1 as key primary share
	 * to force hello retry request */
	assert(gnutls_priority_set_direct(session, "NORMAL:-VERS-ALL:+VERS-TLS1.3",
				   NULL)>=0);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE,
				clientx509cred);

	gnutls_transport_set_int(session, sd);
	gnutls_handshake_set_timeout(session, 20 * 1000);
	gnutls_handshake_set_hook_function(session, GNUTLS_HANDSHAKE_ANY,
					   GNUTLS_HOOK_BOTH, client_hello_callback);

	/* Perform the TLS handshake
	 */
	ret = gnutls_handshake(session);

	if (ret < 0) {
		fail("client: Handshake failed: %s\n", gnutls_strerror(ret));
		goto end;
	} else {
		if (debug)
			success("client: Handshake was completed\n");
	}

	if (cb_called != 2) {
		fail("client: the callback was not seen twice!\n");
	}

	gnutls_bye(session, GNUTLS_SHUT_WR);

end:
	close(sd);

	gnutls_deinit(session);

	gnutls_certificate_free_credentials(clientx509cred);

	gnutls_global_deinit();
}

static void server(int sd)
{
	gnutls_certificate_credentials_t serverx509cred;
	int ret;
	gnutls_session_t session;

	/* this must be called once in the program
	 */
	global_init();
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(4711);

	side = "server";

	gnutls_certificate_allocate_credentials(&serverx509cred);
	gnutls_certificate_set_x509_key_mem(serverx509cred,
					    &server_cert, &server_key,
					    GNUTLS_X509_FMT_PEM);

	gnutls_init(&session, GNUTLS_SERVER);


	/* force a hello retry request by disabling all the groups that are
	 * enabled by default. */
	assert(gnutls_priority_set_direct(session,
					  "NORMAL:-VERS-ALL:+VERS-TLS1.3:"
					  "-GROUP-SECP256R1",
					  NULL)>=0);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE,
				serverx509cred);

	gnutls_transport_set_int(session, sd);
	gnutls_handshake_set_timeout(session, 20 * 1000);

	ret = gnutls_handshake(session);
	if (ret < 0) {
		fail("server: Handshake has failed: %s\n\n",
		     gnutls_strerror(ret));
		goto end;
	}
	if (debug)
		success("server: Handshake was completed\n");

	/* do not wait for the peer to close the connection.
	 */
	gnutls_bye(session, GNUTLS_SHUT_WR);

end:
	close(sd);
	gnutls_deinit(session);

	gnutls_certificate_free_credentials(serverx509cred);

	gnutls_global_deinit();

	if (debug)
		success("server: finished\n");
}

void doit(void)
{
	pid_t child;
	int sockets[2];
	int err;

	signal(SIGPIPE, SIG_IGN);

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
		int status = 0;
		/* parent */
		close(sockets[1]);
		client(sockets[0]);
		wait(&status);
		check_wait_status(status);
	} else {
		close(sockets[0]);
		server(sockets[1]);
	}
}

#endif				/* _WIN32 */
