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

/* This program tests whether a DTLS handshake would timeout
 * in the expected time.
 */

static void print_type(const unsigned char *buf, int size)
{
	if (buf[0] == 22 && size >= 13) {
		if (buf[13] == 1)
			fprintf(stderr, "Client Hello\n");
		else if (buf[13] == 2)
			fprintf(stderr, "Server Hello\n");
		else if (buf[13] == 12)
			fprintf(stderr, "Server Key exchange\n");
		else if (buf[13] == 14)
			fprintf(stderr, "Server Hello Done\n");
		else if (buf[13] == 11)
			fprintf(stderr, "Certificate\n");
		else if (buf[13] == 16)
			fprintf(stderr, "Client Key Exchange\n");
		else if (buf[4] == 1)
			fprintf(stderr, "Finished\n");
		else
			fprintf(stderr, "Unknown handshake\n");
	} else if (buf[0] == 20) {
		fprintf(stderr, "Change Cipher Spec\n");
	} else
		fprintf(stderr, "Unknown\n");
}

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

static int counter;
static int packet_to_lose;

static ssize_t
push(gnutls_transport_ptr_t tr, const void *data, size_t len)
{
	int fd = (long int) tr;

	counter++;

	if (packet_to_lose != -1 && packet_to_lose == counter) {
		if (debug) {
			fprintf(stderr, "Discarding packet %d: ", counter);
			print_type(data, len);
		}

		packet_to_lose = 1;
		counter = 0;
		return len;
	}
	return send(fd, data, len, 0);
}

static void client(int fd, unsigned timeout)
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
	gnutls_dtls_set_timeouts(session, 1 * 1000, timeout * 1000);

	/* Use default priorities */
	gnutls_priority_set_direct(session,
				   "NONE:+VERS-DTLS1.0:+CIPHER-ALL:+MAC-ALL:+SIGN-ALL:+COMP-ALL:+ANON-ECDH:+CURVE-ALL",
				   NULL);

	/* put the anonymous credentials to the current session
	 */
	gnutls_credentials_set(session, GNUTLS_CRD_ANON, anoncred);

	counter = 0;

	gnutls_transport_set_int(session, fd);

	/* Perform the TLS handshake
	 */
	do {
		ret = gnutls_handshake(session);
	}
	while (ret < 0 && (ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED));

	gnutls_deinit(session);
	gnutls_anon_free_client_credentials(anoncred);
	gnutls_global_deinit();

	if (ret < 0) {
		if (ret == GNUTLS_E_TIMEDOUT) {
			if (debug)
				success("client: received timeout\n");
			return;
		}
		fail("client: Handshake failed with unexpected reason: %s\n", gnutls_strerror(ret));
	} else {
		fail("client: Handshake was completed (unexpected)\n");
	}
}


/* These are global */
pid_t child;

static void server(int fd, int packet, unsigned timeout)
{
	gnutls_anon_server_credentials_t anoncred;
	gnutls_session_t session;
	int ret;
	/* this must be called once in the program
	 */
	global_init();

	if (debug) {
		gnutls_global_set_log_function(server_log_func);
		gnutls_global_set_log_level(4711);
	}

	gnutls_anon_allocate_server_credentials(&anoncred);

	gnutls_init(&session, GNUTLS_SERVER | GNUTLS_DATAGRAM);
	gnutls_dtls_set_mtu(session, 1500);
	gnutls_dtls_set_timeouts(session, 1 * 1000, timeout * 1000);

	/* avoid calling all the priority functions, since the defaults
	 * are adequate.
	 */
	gnutls_priority_set_direct(session,
				   "NONE:+VERS-DTLS1.0:+CIPHER-ALL:+MAC-ALL:+SIGN-ALL:+COMP-ALL:+ANON-ECDH:+CURVE-ALL",
				   NULL);

	gnutls_credentials_set(session, GNUTLS_CRD_ANON, anoncred);

	counter = 0;
	packet_to_lose = packet;

	gnutls_transport_set_int(session, fd);
	gnutls_transport_set_push_function(session, push);

	do {
		ret = gnutls_handshake(session);
	}
	while (ret < 0 && (ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED));

	gnutls_deinit(session);
	gnutls_anon_free_server_credentials(anoncred);
	gnutls_global_deinit();

	if (ret < 0) {
		if (ret == GNUTLS_E_TIMEDOUT) {
			if (debug)
				success("server received timeout\n");
			return;
		}
		fail("server: Handshake failed with unexpected reason: %s\n", gnutls_strerror(ret));
	} else {
		fail("server: Handshake was completed (unexpected)\n");
	}

	if (ret < 0) {
		return;
	}
}

static void start(int server_packet, int wait_server)
{
	int fd[2];
	int ret;

	if (debug)
		fprintf(stderr, "\nWill discard server packet %d\n",
			server_packet);

	/* we need dgram in this test */
	ret = socketpair(AF_UNIX, SOCK_DGRAM, 0, fd);
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
		/* parent */
		close(fd[0]);
		if (wait_server)
			server(fd[1], server_packet, 30);
		else
			client(fd[1], 30);
		close(fd[1]);
		kill(child, SIGTERM);
	} else {
		close(fd[1]);
		if (wait_server)
			client(fd[0], 32);
		else
			server(fd[0], server_packet, 32);
		close(fd[0]);
		exit(0);
	}
}

static void ch_handler(int sig)
{
	int status = 0;
	wait(&status);
	check_wait_status(status);
	return;
}

void doit(void)
{
	time_t tstart, tstop;
	int tries = 5; /* we try multiple times because in very busy systems the suite may fail to finish on time */

	signal(SIGCHLD, ch_handler);
	signal(SIGPIPE, SIG_IGN);

	for (;tries>=0;tries--) {
		tstart = time(0);
		start(2, 0);

		tstop = time(0);

		tstop = tstop - tstart;

		if (!(tstop < 40 && tstop > 25)) {
			if (tries == 0)
				fail("Client wait time difference: %u\n", (unsigned) tstop);
			else if (debug)
				success("Client wait time difference: %u\n", (unsigned) tstop);
		} else break;
	}

	for (;tries>=0;tries--) {
		tstart = time(0);
		start(2, 1);

		tstop = time(0);

		tstop = tstop - tstart;

		if (!(tstop < 40 && tstop > 25)) {
			if (tries == 0)
				fail("Server wait time difference: %u\n", (unsigned) tstop);
			else if (debug)
				success("Server wait time difference: %u\n", (unsigned) tstop);
		} else break;
	}
}

#endif				/* _WIN32 */
