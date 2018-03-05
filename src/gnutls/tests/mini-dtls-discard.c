/*
 * Copyright (C) 2014 Nikos Mavrogiannopoulos
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
#include <errno.h>
#include <gnutls/gnutls.h>
#include <gnutls/dtls.h>

#include "utils.h"

#define TXT1 "hello there"
#define TXT1_SIZE (sizeof(TXT1)-1)

#define TXT2 "2hello there"
#define TXT2_SIZE (sizeof(TXT2)-1)

static void terminate(void);

/* This program tests the client hello verify in DTLS
 */

static void server_log_func(int level, const char *str)
{
	fprintf(stderr, "server|<%d>| %s", level, str);
}

static void client_log_func(int level, const char *str)
{
	fprintf(stderr, "client|<%d>| %s", level, str);
}

/* A very basic TLS client, with anonymous authentication.
 */

#define MAX_BUF 1024

static ssize_t
push(gnutls_transport_ptr_t tr, const void *data, size_t len)
{
	int fd = (long int) tr;
	static int counter = 0;

	if (counter % 2 == 0) {
		errno = EAGAIN;
		counter++;
		return -1;
	}
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

	gnutls_transport_set_push_function(session, push);
	do {
		ret = gnutls_record_send(session, TXT1, TXT1_SIZE);
		if (ret == GNUTLS_E_AGAIN) {
			if (debug) success("discarding\n");
			gnutls_record_discard_queued(session);
		}
			
	} while (ret == GNUTLS_E_INTERRUPTED);

	do {
		ret = gnutls_record_send(session, TXT2, TXT2_SIZE);
	} while (ret == GNUTLS_E_INTERRUPTED || ret == GNUTLS_E_AGAIN);

	gnutls_bye(session, GNUTLS_SHUT_WR);

	close(fd);

	gnutls_deinit(session);

	gnutls_anon_free_client_credentials(anoncred);

	gnutls_global_deinit();
}


/* These are global */
pid_t child;

static void terminate(void)
{
	int status;

	kill(child, SIGTERM);
	wait(&status);
	exit(1);
}

static void server(int fd)
{
	int ret;
	gnutls_anon_server_credentials_t anoncred;
	char buffer[MAX_BUF + 1];
	gnutls_session_t session;

	/* this must be called once in the program
	 */
	global_init();

	if (debug) {
		gnutls_global_set_log_function(server_log_func);
		gnutls_global_set_log_level(4711);
	}

	gnutls_anon_allocate_server_credentials(&anoncred);

	gnutls_init(&session, GNUTLS_SERVER | GNUTLS_DATAGRAM);
	gnutls_handshake_set_timeout(session, 20 * 1000);
	gnutls_dtls_set_mtu(session, 1500);

	/* avoid calling all the priority functions, since the defaults
	 * are adequate.
	 */
	gnutls_priority_set_direct(session,
				   "NONE:+VERS-DTLS1.0:+CIPHER-ALL:+MAC-ALL:+SIGN-ALL:+COMP-ALL:+ANON-ECDH:+CURVE-ALL",
				   NULL);

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

	/* see the Getting peer's information example */
	/* print_info(session); */

	do {
		ret = gnutls_record_recv(session, buffer, sizeof(buffer));
	} while (ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED);

	if (ret != TXT2_SIZE || memcmp(buffer, TXT2, TXT2_SIZE) != 0) {
		fail("didn't receive the expected data!\n");
		terminate();
	}

	if (ret < 0) {
		close(fd);
		gnutls_deinit(session);
		fail("server: data sending has failed (%s)\n\n",
		     gnutls_strerror(ret));
		terminate();
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

void doit(void)
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

		server(fd[0]);
		wait(&status);
		check_wait_status(status);
	} else {
		close(fd[0]);
		client(fd[1]);
		exit(0);
	}
}

#endif				/* _WIN32 */
