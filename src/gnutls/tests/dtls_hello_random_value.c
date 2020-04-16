/*
 * Copyright (C) 2017-2020 Red Hat, Inc.
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

/* This program tests whether the second DTLS client hello contains the same
 * random value, and whether it is initialized.
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
#include <gnutls/dtls.h>
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

static int hello_callback(gnutls_session_t session, unsigned int htype,
			  unsigned post, unsigned int incoming, const gnutls_datum_t *msg)
{
	unsigned non_zero = 0, i;

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
					non_zero++;
				}
			}

			if (non_zero <= 8) {
				fail("the client random value seems uninitialized\n");
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
	} if (htype == GNUTLS_HANDSHAKE_SERVER_HELLO && post == GNUTLS_HOOK_POST) {
		gnutls_datum_t tmp;
		gnutls_session_get_random(session, NULL, &tmp);
		assert(tmp.size == 32);

		for (i=0;i<32;i++) {
			if (tmp.data[i] != 0) {
				non_zero++;
			}
		}
		if (non_zero <= 8) {
			fail("the server random value seems uninitialized\n");
		}
	}

	return 0;
}

static void client(int sd, const char *priority)
{
	int ret;
	gnutls_session_t session;
	gnutls_certificate_credentials_t clientx509cred;

	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(4711);

	side = "client";

	gnutls_certificate_allocate_credentials(&clientx509cred);

	assert(gnutls_init(&session, GNUTLS_CLIENT|GNUTLS_DATAGRAM)>=0);

	if (!priority) {
		assert(gnutls_set_default_priority(session) >= 0);
	} else {
		assert(gnutls_priority_set_direct(session, priority, NULL) >= 0);
	}

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE,
				clientx509cred);

	gnutls_transport_set_int(session, sd);
	gnutls_dtls_set_mtu(session, 1500);
	gnutls_handshake_set_timeout(session, 20 * 1000);

	gnutls_handshake_set_hook_function(session, GNUTLS_HANDSHAKE_ANY,
					   GNUTLS_HOOK_BOTH, hello_callback);

	ret = gnutls_handshake(session);

	if (ret < 0) {
		fail("client: Handshake failed: %s\n", gnutls_strerror(ret));
	} else {
		if (debug)
			success("client: Handshake was completed\n");
	}

	if (cb_called != 2) {
		fail("client: the callback was not seen twice!\n");
	}

	gnutls_bye(session, GNUTLS_SHUT_WR);
	close(sd);

	gnutls_deinit(session);

	gnutls_certificate_free_credentials(clientx509cred);
}

#define MAX_BUF 1024
#define CLI_ADDR (void*)"test"
#define CLI_ADDR_LEN 4

static ssize_t
push(gnutls_transport_ptr_t tr, const void *data, size_t len)
{
	int fd = (long int) tr;

	return send(fd, data, len, 0);
}

static void server(int sd, const char *priority)
{
	int ret, csend = 0;
	char buffer[MAX_BUF + 1];
	gnutls_certificate_credentials_t serverx509cred;
	gnutls_dtls_prestate_st prestate;
	gnutls_session_t session;
	gnutls_datum_t cookie_key;

	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(4711);

	side = "server";

	ret = gnutls_key_generate(&cookie_key, GNUTLS_COOKIE_KEY_SIZE);
	if (ret < 0) {
		fail("Cannot generate key: %s\n", gnutls_strerror(ret));
	}

	for (;;) {
		ret = recv(sd, buffer, sizeof(buffer), MSG_PEEK);
		if (ret < 0) {
			fail("Cannot receive data\n");
		}

		memset(&prestate, 0, sizeof(prestate));
		ret =
		    gnutls_dtls_cookie_verify(&cookie_key, CLI_ADDR,
					      CLI_ADDR_LEN, buffer, ret,
					      &prestate);
		if (ret < 0) {	/* cookie not valid */
			if (debug)
				success("Sending hello verify request\n");

			ret =
			    gnutls_dtls_cookie_send(&cookie_key, CLI_ADDR,
						    CLI_ADDR_LEN,
						    &prestate,
						    (gnutls_transport_ptr_t)
						    (long) sd, push);
			if (ret < 0) {
				fail("Cannot send data\n");
			}

			/* discard peeked data */
			recv(sd, buffer, sizeof(buffer), 0);
			csend++;

			if (csend > 2) {
				fail("too many cookies sent\n");
			}

			continue;
		}

		/* success */
		break;
	}

	assert(gnutls_certificate_allocate_credentials(&serverx509cred)>=0);
	assert(gnutls_certificate_set_x509_key_mem(serverx509cred,
					    &server_cert, &server_key,
					    GNUTLS_X509_FMT_PEM)>=0);

	assert(gnutls_init(&session, GNUTLS_SERVER|GNUTLS_DATAGRAM)>=0);
	assert(session != NULL);

	if (!priority) {
		assert(gnutls_set_default_priority(session) >= 0);
	} else {
		assert(gnutls_priority_set_direct(session, priority, NULL) >= 0);
	}

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE,
				serverx509cred);

	gnutls_transport_set_int(session, sd);
	gnutls_handshake_set_timeout(session, 20 * 1000);
	gnutls_dtls_set_mtu(session, 1500);

	gnutls_dtls_prestate_set(session, &prestate);

	ret = gnutls_handshake(session);
	if (ret < 0) {
		fail("server: Handshake has failed: %s\n\n",
		     gnutls_strerror(ret));
	}
	if (debug)
		success("server: Handshake was completed\n");

	/* do not wait for the peer to close the connection.
	 */
	gnutls_bye(session, GNUTLS_SHUT_WR);
	close(sd);
	gnutls_deinit(session);

	gnutls_certificate_free_credentials(serverx509cred);
	gnutls_free(cookie_key.data);

	if (debug)
		success("server: finished\n");
}

static void start(const char *name, const char *priority)
{
	pid_t child;
	int sockets[2];
	int err;

	success("testing: %s\n", name);
	cb_called = 0;

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
		client(sockets[0], priority);
		wait(&status);
		check_wait_status(status);
	} else {
		close(sockets[0]);
		server(sockets[1], priority);
		exit(0);
	}
}

void doit(void)
{
	signal(SIGPIPE, SIG_IGN);

	start("default", NULL);
	start("dtls1.2", "NORMAL:-VERS-ALL:+VERS-DTLS1.2");
	start("dtls1.0", "NORMAL:-VERS-ALL:+VERS-DTLS1.0");
}

#endif				/* _WIN32 */
