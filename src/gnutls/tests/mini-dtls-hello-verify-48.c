/*
 * Copyright (C) 2013-2015 Nikos Mavrogiannopoulos
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

/* This checks the proper behavior of the client when
 * invalid data are sent by the server in the hello verify
 * request.
 */

#if defined(_WIN32)

int main()
{
	exit(77);
}

#else

#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>
#include <errno.h>
#include <signal.h>
#include <gnutls/gnutls.h>
#include <gnutls/dtls.h>

#include "utils.h"

static void server_log_func(int level, const char *str)
{
	fprintf(stderr, "server|<%d>| %s", level, str);
}

static void client_log_func(int level, const char *str)
{
	fprintf(stderr, "client|<%d>| %s", level, str);
}

#define MAX_BUF 1024

static ssize_t
push(gnutls_transport_ptr_t tr, const void *data, size_t len)
{
	int fd = (long int) tr;

	return send(fd, data, len, 0);
}

static void client(int fd)
{
	int ret;
	gnutls_anon_client_credentials_t anoncred;
	gnutls_session_t session;
	/* Need to enable anonymous KX specifically. */

	global_init();

	if (debug) {
		gnutls_global_set_log_function(client_log_func);
		gnutls_global_set_log_level(4711);
	}

	gnutls_anon_allocate_client_credentials(&anoncred);

	/* Initialize TLS session
	 */
	gnutls_init(&session, GNUTLS_CLIENT | GNUTLS_DATAGRAM);
	gnutls_dtls_set_mtu(session, 1500);
	gnutls_handshake_set_timeout(session, 20 * 1000);

	/* Use default priorities */
	gnutls_priority_set_direct(session,
				   "NONE:+VERS-DTLS-ALL:+CIPHER-ALL:+MAC-ALL:+SIGN-ALL:+COMP-ALL:+ANON-ECDH:+CURVE-ALL",
				   NULL);

	/* put the anonymous credentials to the current session
	 */
	gnutls_credentials_set(session, GNUTLS_CRD_ANON, anoncred);

	gnutls_transport_set_int(session, fd);
	gnutls_transport_set_push_function(session, push);

	/* Perform the TLS handshake
	 */
	do {
		ret = gnutls_handshake(session);
	}
	while (ret < 0 && gnutls_error_is_fatal(ret) == 0);

	if (ret < 0) {
		success("client: Handshake failed as expected\n");
		gnutls_perror(ret);
		goto exit;
	} else {
		fail("client: Handshake completed unexpectedly\n");
		goto exit;
	}

 exit:
	gnutls_deinit(session);

	gnutls_anon_free_client_credentials(anoncred);

	gnutls_global_deinit();
}


/* These are global */
pid_t child;

#define CLI_ADDR (void*)"test"
#define CLI_ADDR_LEN 4

static
ssize_t recv_timeout(int sockfd, void *buf, size_t len, unsigned flags, unsigned sec)
{
int ret;
struct timeval tv;
fd_set set;

	tv.tv_sec = sec;
	tv.tv_usec = 0;

	FD_ZERO(&set);
	FD_SET(sockfd, &set);

	do {
		ret = select(sockfd + 1, &set, NULL, NULL, &tv);
	} while (ret == -1 && errno == EINTR);

	if (ret == -1 || ret == 0) {
		errno = ETIMEDOUT;
		return -1;
	}

	return recv(sockfd, buf, len, flags);
}

#define SERV_TIMEOUT 30

static void server(int fd)
{
	int ret, csend = 0;
	gnutls_anon_server_credentials_t anoncred;
	char buffer[MAX_BUF + 1];
	gnutls_datum_t cookie_key;
	gnutls_dtls_prestate_st prestate;
	gnutls_session_t session;
	unsigned try = 0;

	/* this must be called once in the program
	 */
	global_init();

	if (debug) {
		gnutls_global_set_log_function(server_log_func);
		gnutls_global_set_log_level(4711);
	}

	ret = gnutls_key_generate(&cookie_key, GNUTLS_COOKIE_KEY_SIZE);
	if (ret < 0) {
		fail("Cannot generate key: %s\n", gnutls_strerror(ret));
		exit(1);
	}

	gnutls_anon_allocate_server_credentials(&anoncred);

	gnutls_init(&session, GNUTLS_SERVER | GNUTLS_DATAGRAM);
	gnutls_handshake_set_timeout(session, SERV_TIMEOUT * 1000);
	gnutls_dtls_set_mtu(session, 1500);

	/* avoid calling all the priority functions, since the defaults
	 * are adequate.
	 */
	gnutls_priority_set_direct(session,
				   "NONE:+VERS-DTLS1.0:+CIPHER-ALL:+MAC-ALL:+SIGN-ALL:+COMP-ALL:+ANON-ECDH:+CURVE-ALL",
				   NULL);

	gnutls_credentials_set(session, GNUTLS_CRD_ANON, anoncred);

	gnutls_transport_set_int(session, fd);
	gnutls_transport_set_push_function(session, push);

	for (;;) {
		ret = recv_timeout(fd, buffer, sizeof(buffer), MSG_PEEK, SERV_TIMEOUT);
		if (ret < 0) {
			if (try != 0) {
				success("Server was terminated as expected!\n");
				goto exit;
			} else {
				fail("Error receiving first message\n");
				exit(1);
			}
		}
		try++;

		memset(&prestate, 0, sizeof(prestate));
		prestate.record_seq = 105791312;
		prestate.hsk_write_seq = 67166359;
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
						    (long) fd, push);
			if (ret < 0) {
				fail("Cannot send data\n");
				exit(1);
			}

			/* discard peeked data */
			recv_timeout(fd, buffer, sizeof(buffer), 0, SERV_TIMEOUT);
			csend++;

			if (csend > 2) {
				fail("too many cookies sent\n");
				exit(1);
			}

			continue;
		}

		/* success */
		break;
	}

	fail("Shouldn't have reached here\n");
	exit(1);
 exit:
	gnutls_deinit(session);
	gnutls_free(cookie_key.data);

	gnutls_anon_free_server_credentials(anoncred);

	gnutls_global_deinit();
}

void doit(void)
{
	int fd[2];
	int ret;

	signal(SIGPIPE, SIG_IGN);

	ret = socketpair(AF_UNIX, SOCK_STREAM, 0, fd);
	if (ret < 0) {
		perror("socketpair");
		exit(1);
	}

	child = fork();
	if (child < 0) {
		perror("fork");
		fail("fork");
		exit(1);
	}

	if (child) {
		int status;
		/* parent */

		client(fd[0]);
		wait(&status);
		check_wait_status(status);
	} else {
		close(fd[0]);
		server(fd[1]);
		exit(0);
	}
}

#endif				/* _WIN32 */
