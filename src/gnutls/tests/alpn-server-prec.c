/*
 * Copyright (C) 2013-2016 Nikos Mavrogiannopoulos
 * Copyright (C) 2016 Red Hat, Inc.
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

/* This program tests the GNUTLS_ALPN_SERVER_PRECEDENCE
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#if defined(_WIN32) || !defined(ENABLE_ALPN)

int main(int argc, char **argv)
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
#include <gnutls/gnutls.h>
#include <gnutls/dtls.h>

#include "utils.h"

static void terminate(void);

static void server_log_func(int level, const char *str)
{
	fprintf(stderr, "server|<%d>| %s", level, str);
}

static void client_log_func(int level, const char *str)
{
	fprintf(stderr, "client|<%d>| %s", level, str);
}

/* These are global */
static pid_t child;

static void client(int fd, const char *protocol0, const char *protocol1, const char *protocol2)
{
	gnutls_session_t session;
	int ret;
	gnutls_datum_t proto;
	gnutls_anon_client_credentials_t anoncred;
	/* Need to enable anonymous KX specifically. */

	global_init();

	if (debug) {
		gnutls_global_set_log_function(client_log_func);
		gnutls_global_set_log_level(4711);
	}

	gnutls_anon_allocate_client_credentials(&anoncred);

	/* Initialize TLS session
	 */
	gnutls_init(&session, GNUTLS_CLIENT);

	/* Use default priorities */
	gnutls_priority_set_direct(session,
				   "NONE:+VERS-TLS1.0:+CIPHER-ALL:+MAC-ALL:+SIGN-ALL:+COMP-ALL:+ANON-ECDH:+CURVE-ALL",
				   NULL);
	if (protocol1) {
		gnutls_datum_t t[3];
		t[0].data = (void *) protocol0;
		t[0].size = strlen(protocol0);
		t[1].data = (void *) protocol1;
		t[1].size = strlen(protocol1);
		t[2].data = (void *) protocol2;
		t[2].size = strlen(protocol2);

		ret = gnutls_alpn_set_protocols(session, t, 3, 0);
		if (ret < 0) {
			gnutls_perror(ret);
			exit(1);
		}
	}

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

	if (ret < 0) {
		fail("client: Handshake failed\n");
		gnutls_perror(ret);
		exit(1);
	} else {
		if (debug)
			success("client: Handshake was completed\n");
	}

	if (debug)
		success("client: TLS version is: %s\n",
			gnutls_protocol_get_name
			(gnutls_protocol_get_version(session)));

	ret = gnutls_alpn_get_selected_protocol(session, &proto);
	if (ret < 0) {
		gnutls_perror(ret);
		exit(1);
	}

	if (debug) {
		fprintf(stderr, "selected protocol: %.*s\n",
			(int) proto.size, proto.data);
	}


	gnutls_bye(session, GNUTLS_SHUT_WR);

	close(fd);

	gnutls_deinit(session);

	gnutls_anon_free_client_credentials(anoncred);

	gnutls_global_deinit();
}

static void terminate(void)
{
	int status;

	kill(child, SIGTERM);
	wait(&status);
	exit(1);
}

static void server(int fd, const char *protocol1, const char *protocol2, const char *expected)
{
	int ret;
	gnutls_session_t session;
	gnutls_anon_server_credentials_t anoncred;
	gnutls_datum_t t[2];
	gnutls_datum_t selected;

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
	gnutls_priority_set_direct(session,
				   "NONE:+VERS-TLS1.0:+CIPHER-ALL:+MAC-ALL:+SIGN-ALL:+COMP-ALL:+ANON-ECDH:+CURVE-ALL",
				   NULL);

	t[0].data = (void *) protocol1;
	t[0].size = strlen(protocol1);
	t[1].data = (void *) protocol2;
	t[1].size = strlen(protocol2);

	ret = gnutls_alpn_set_protocols(session, t, 2, GNUTLS_ALPN_SERVER_PRECEDENCE);
	if (ret < 0) {
		gnutls_perror(ret);
		exit(1);
	}

	gnutls_credentials_set(session, GNUTLS_CRD_ANON, anoncred);

	gnutls_transport_set_int(session, fd);

	do {
		ret = gnutls_handshake(session);
	}
	while (ret < 0 && gnutls_error_is_fatal(ret) == 0);
	if (ret < 0) {
		close(fd);
		gnutls_deinit(session);
		fail("server: Handshake has failed (%s)\n\n",
		     gnutls_strerror(ret));
		terminate();
	}
	if (debug)
		success("server: Handshake was completed\n");

	if (debug)
		success("server: TLS version is: %s\n",
			gnutls_protocol_get_name
			(gnutls_protocol_get_version(session)));

	ret = gnutls_alpn_get_selected_protocol(session, &selected);
	if (ret < 0) {
		gnutls_perror(ret);
		exit(1);
	}

	if (debug) {
		success("Protocol: %.*s\n", (int) selected.size, selected.data);
	}

	if (selected.size != strlen(expected) || memcmp(selected.data, expected, selected.size) != 0) {
		fail("did not select the expected protocol (selected %.*s, expected %s)\n", selected.size, selected.data, expected);
		exit(1);
	}

	/* do not wait for the peer to close the connection.
	 */
	gnutls_bye(session, GNUTLS_SHUT_WR);

	close(fd);
	gnutls_deinit(session);

	gnutls_anon_free_server_credentials(anoncred);

	gnutls_global_deinit();

	if (debug)
		success("server: finished\n");
}

static void start(const char *p1, const char *p2, const char *cp1, const char *cp2, const char *expected)
{
	int fd[2];
	int ret;

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

		server(fd[0], p1, p2, expected);
		wait(&status);
		check_wait_status(status);
	} else {
		close(fd[0]);
		client(fd[1], cp1, "unknown/1.4", cp2);
		exit(0);
	}
}

void doit(void)
{
	/* A, B - A, B -> A */
	start("h2", "http/1.1", "h2", "http/1.1", "h2");

	/* A, B - B, A -> A */
	start("spdy/3", "spdy/2", "spdy/2", "spdy/3", "spdy/3");

	/* A, B - C, B -> B */
	start("spdy/3", "spdy/2", "h2", "spdy/2", "spdy/2");

	/* A, B - B, C -> B */
	start("h2", "http/1.1", "http/1.1", "h3", "http/1.1");
}

#endif				/* _WIN32 */
