/*
 * Copyright (C) 2012 Free Software Foundation, Inc.
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

#if defined(_WIN32)

int main()
{
	exit(77);
}

#else

#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <gnutls/gnutls.h>
#include <gnutls/dtls.h>
#include <signal.h>

#include "utils.h"

/* This program tests whether the handshake timeout value is enforced.
 */

static void server_log_func(int level, const char *str)
{
	fprintf(stderr, "server|<%d>| %s", level, str);
}

static void client_log_func(int level, const char *str)
{
	fprintf(stderr, "client|<%d>| %s", level, str);
}

static void client(int fd, int tmo_ms)
{
	int ret;
	gnutls_anon_client_credentials_t anoncred;
	gnutls_session_t session;

	global_init();

	if (debug) {
		gnutls_global_set_log_function(client_log_func);
		gnutls_global_set_log_level(4711);
	}

	gnutls_anon_allocate_client_credentials(&anoncred);

	/* Initialize TLS session
	 */
	gnutls_init(&session, GNUTLS_CLIENT);
	gnutls_handshake_set_timeout(session, tmo_ms);

	/* Use default priorities */
	gnutls_priority_set_direct(session, "NORMAL:+ANON-ECDH:-VERS-ALL:+VERS-TLS1.2", NULL);

	/* put the anonymous credentials to the current session
	 */
	gnutls_credentials_set(session, GNUTLS_CRD_ANON, anoncred);

	gnutls_transport_set_int(session, fd);

	/* Perform the TLS handshake
	 */
	do {
		ret = gnutls_handshake(session);
	}
	while (ret < 0 && gnutls_error_is_fatal(ret) == 0);

	gnutls_deinit(session);
	gnutls_anon_free_client_credentials(anoncred);
	gnutls_global_deinit();

	if (ret < 0) {
		if (ret != GNUTLS_E_TIMEDOUT || tmo_ms == 0) {
			if (debug)
				fail("client: unexpected error: %s\n",
				     gnutls_strerror(ret));
			exit(1);
		}
		if (debug)
			success("client: expected timeout occurred\n");
	} else {
		if (tmo_ms != 0) {
			fail("client: handshake was completed unexpectedly\n");
			gnutls_perror(ret);
			exit(1);
		}
	}

	shutdown(fd, SHUT_RDWR);
	return;
}

static void server(int fd, int tmo_ms)
{
	int ret;
	gnutls_session_t session;
	gnutls_anon_server_credentials_t anoncred;

	/* this must be called once in the program
	 */
	global_init();

	if (debug) {
		gnutls_global_set_log_function(server_log_func);
		gnutls_global_set_log_level(4711);
	}

	gnutls_anon_allocate_server_credentials(&anoncred);

	gnutls_init(&session, GNUTLS_SERVER);

	/* avoid calling all the priority functions, since the defaults
	 * are adequate.
	 */
	gnutls_priority_set_direct(session, "NORMAL:+ANON-ECDH:-VERS-ALL:+VERS-TLS1.2", NULL);

	gnutls_credentials_set(session, GNUTLS_CRD_ANON, anoncred);

	gnutls_transport_set_int(session, fd);

	if (tmo_ms) {
		char buf[32];

		// read until client closes connection
		while (read(fd, buf, sizeof(buf)) > 0)
			;
	} else {
		do {
			ret = gnutls_handshake(session);
		}
		while (ret < 0 && gnutls_error_is_fatal(ret) == 0);

		if (ret == 0)
			gnutls_bye(session, GNUTLS_SHUT_RDWR);
	}

	gnutls_deinit(session);
	gnutls_anon_free_server_credentials(anoncred);
	gnutls_global_deinit();
}

static void start(int tmo_ms)
{
	int fd[2];
	int ret;
	pid_t child;

	if (debug && tmo_ms)
		fprintf(stderr, "\nWill test timeout %dms\n", tmo_ms);

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
		int status = 0;
		/* parent */
		close(fd[1]);
		server(fd[0], tmo_ms);
		close(fd[0]);

		wait(&status);
		check_wait_status(status);
	} else {
		close(fd[0]);
		client(fd[1], tmo_ms);
		close(fd[1]);
		exit(0);
	}
}

static void ch_handler(int sig)
{
	return;
}

void doit(void)
{
	signal(SIGCHLD, ch_handler);
	signal(SIGPIPE, SIG_IGN);

	/* make sure that normal handshake occurs */
	start(0);

	/* check the handshake with a 100ms timeout */
	start(100);

	/* check the handshake with a 1000ms timeout */
	start(1000);
}

#endif				/* _WIN32 */
