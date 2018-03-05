/*
 * Copyright (C) 2013 Nikos Mavrogiannopoulos
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

#if defined(_WIN32) || !defined(ENABLE_HEARTBEAT)

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
#include <gnutls/gnutls.h>
#include <gnutls/dtls.h>

#include "utils.h"

static void terminate(void);

/* This program tests large packet sending in DTLS
 */

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

/* A very basic DTLS client, with anonymous authentication, that exchanges heartbeats.
 */

#define MAX_BUF 24*1024
#define MAX_MTU 20*1024

static void client(int fd)
{
	gnutls_session_t session;
	int ret;
	char buffer[MAX_BUF + 1];
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
	gnutls_init(&session, GNUTLS_CLIENT | GNUTLS_DATAGRAM);
	gnutls_heartbeat_enable(session, GNUTLS_HB_PEER_ALLOWED_TO_SEND);
	gnutls_dtls_set_mtu(session, 1500);

	/* Use default priorities */
	gnutls_priority_set_direct(session,
				   "NONE:+VERS-DTLS1.0:+CIPHER-ALL:+MAC-ALL:+SIGN-ALL:+COMP-ALL:+ANON-ECDH:+CURVE-ALL",
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
		success("client: DTLS version is: %s\n",
			gnutls_protocol_get_name
			(gnutls_protocol_get_version(session)));

	do {
		ret = gnutls_record_recv(session, buffer, sizeof(buffer));
	}
	while (ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED || ret > 0);

	if (ret < 0) {
		fail("recv: %s\n", gnutls_strerror(ret));
		terminate();
	}

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
}

static void server(int fd)
{
	int ret;
	char buffer[MAX_BUF + 1];
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

	gnutls_init(&session, GNUTLS_SERVER | GNUTLS_DATAGRAM);
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
		terminate();
		fail("server: Handshake has failed (%s)\n\n",
		     gnutls_strerror(ret));
	}
	if (debug)
		success("server: Handshake was completed\n");

	if (debug)
		success("server: TLS version is: %s\n",
			gnutls_protocol_get_name
			(gnutls_protocol_get_version(session)));

	/* see the Getting peer's information example */
	/* print_info(session); */

	/* avoid uninitialized warnings */
	memset(buffer, 1, sizeof(buffer));

	ret =
	    gnutls_record_send(session, buffer,
				gnutls_dtls_get_data_mtu(session) + 12);
	if (ret != GNUTLS_E_LARGE_PACKET) {
		terminate();
		fail("send[%d]: %s\n", __LINE__, gnutls_strerror(ret));
	}

	ret =
	    gnutls_record_send(session, buffer,
				gnutls_dtls_get_data_mtu(session) + 5048);
	if (ret != GNUTLS_E_LARGE_PACKET) {
		terminate();
		fail("send[%d]: %s\n", __LINE__, gnutls_strerror(ret));
	}

	ret =
	    gnutls_record_send(session, buffer,
				gnutls_dtls_get_data_mtu(session));
	if (ret < 0) {
		terminate();
		fail("send[%d]: %s\n", __LINE__, gnutls_strerror(ret));
	}

	gnutls_dtls_set_mtu(session, MAX_MTU);
	ret =
	    gnutls_record_send(session, buffer,
				gnutls_dtls_get_data_mtu(session) + 12);
	if (ret != GNUTLS_E_LARGE_PACKET) {
		terminate();
		fail("send[%d]: %s\n", __LINE__, gnutls_strerror(ret));
	}

	ret =
	    gnutls_record_send(session, buffer,
				gnutls_dtls_get_data_mtu(session) + 5048);
	if (ret != GNUTLS_E_LARGE_PACKET) {
		terminate();
		fail("send[%d]: %s\n", __LINE__, gnutls_strerror(ret));
	}

	ret =
	    gnutls_record_send(session, buffer,
				gnutls_dtls_get_data_mtu(session));
	if (ret > 16384 || ret < 0) {
		terminate();
		fail("send[%d]: %s\n", __LINE__, gnutls_strerror(ret));
	}

	/* test cork and uncork */
	gnutls_record_cork(session);

	ret =
	    gnutls_record_send(session, buffer,
				gnutls_dtls_get_data_mtu(session));
	if (ret < 0) {
		terminate();
		fail("send[%d]: %s\n", __LINE__, gnutls_strerror(ret));
	}

	ret = gnutls_record_uncork(session, 0);
	if (ret < 0) {
		terminate();
		fail("send[%d]: %s\n", __LINE__, gnutls_strerror(ret));
	}

	gnutls_record_cork(session);

	ret =
	    gnutls_record_send(session, buffer,
				gnutls_dtls_get_data_mtu(session) - 16);
	if (ret < 0) {
		terminate();
		fail("send[%d]: %s\n", __LINE__, gnutls_strerror(ret));
	}

	ret =
	    gnutls_record_send(session, buffer,
				gnutls_dtls_get_data_mtu(session));
	if (ret != GNUTLS_E_LARGE_PACKET) {
		terminate();
		fail("send[%d]: %s\n", __LINE__, gnutls_strerror(ret));
	}

	ret = gnutls_record_uncork(session, GNUTLS_RECORD_WAIT);
	if (ret < 0) {
		terminate();
		fail("send[%d]: %s\n", __LINE__, gnutls_strerror(ret));
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

static void start(void)
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

void doit(void)
{
	start();
}

#endif				/* _WIN32 */
