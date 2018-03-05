/*
 * Copyright (C) 2015 Nikos Mavrogiannopoulos
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
#include <signal.h>
#include <gnutls/gnutls.h>
#include <gnutls/dtls.h>

#include "utils.h"

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

/* A very basic DTLS client handling DTLS 0.9 which sets premaster secret.
 */

#define MAX_BUF 1024

static ssize_t
push(gnutls_transport_ptr_t tr, const void *data, size_t len)
{
	int fd = (long int) tr;

	return send(fd, data, len, 0);
}

static gnutls_datum_t master =
    { (void*)"\x44\x66\x44\xa9\xb6\x29\xed\x6e\xd6\x93\x15\xdb\xf0\x7d\x4b\x2e\x18\xb1\x9d\xed\xff\x6a\x86\x76\xc9\x0e\x16\xab\xc2\x10\xbb\x17\x99\x24\xb1\xd9\xb9\x95\xe7\xea\xea\xea\xea\xea\xff\xaa\xac", 48};
static gnutls_datum_t sess_id =
    { (void*)"\xd9\xb9\x95\xe7\xea", 5};

static void client(int fd, int proto, int cipher, int mac)
{
	int ret;
	char buffer[MAX_BUF + 1];
	gnutls_certificate_credentials_t xcred;
	gnutls_session_t session;

	global_init();

	if (debug) {
		gnutls_global_set_log_function(client_log_func);
		gnutls_global_set_log_level(4711);
	}

	gnutls_certificate_allocate_credentials(&xcred);

	/* Initialize TLS session
	 */
	gnutls_init(&session, GNUTLS_CLIENT | GNUTLS_DATAGRAM);
	gnutls_dtls_set_mtu(session, 1500);
	gnutls_handshake_set_timeout(session, 20 * 1000);

	/* Use default priorities */
	gnutls_priority_set_direct(session,
				   "NONE:+VERS-DTLS0.9:+COMP-NULL:+AES-128-GCM:+AEAD:+AES-128-CBC:+SHA1:+RSA:%COMPAT",
				   NULL);

	ret = gnutls_session_set_premaster(session, GNUTLS_CLIENT,
		proto, GNUTLS_KX_RSA,
		cipher, mac,
		GNUTLS_COMP_NULL, &master, &sess_id);
	if (ret < 0) {
		fail("client: gnutls_session_set_premaster failed: %s\n", gnutls_strerror(ret));
		exit(1);
	}

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);

	gnutls_transport_set_int(session, fd);
	gnutls_transport_set_push_function(session, push);

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

	do {
		ret = gnutls_record_recv(session, buffer, sizeof(buffer)-1);
	} while (ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED);

	if (ret == 0) {
		if (debug)
			success
			    ("client: Peer has closed the TLS connection\n");
		goto end;
	} else if (ret < 0) {
		fail("client: Error: %s\n", gnutls_strerror(ret));
		exit(1);
	}

	gnutls_bye(session, GNUTLS_SHUT_WR);

      end:

	close(fd);

	gnutls_deinit(session);

	gnutls_certificate_free_credentials(xcred);

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

static void server(int fd, int proto, int cipher, int mac)
{
	int ret;
	gnutls_certificate_credentials_t xcred;
	char buffer[MAX_BUF + 1];
	gnutls_session_t session;

	/* this must be called once in the program
	 */
	global_init();

	success("testing for %s-%s\n", gnutls_cipher_get_name(cipher), gnutls_mac_get_name(mac));

	if (debug) {
		gnutls_global_set_log_function(server_log_func);
		gnutls_global_set_log_level(4711);
	}

	gnutls_certificate_allocate_credentials(&xcred);

	gnutls_init(&session, GNUTLS_SERVER | GNUTLS_DATAGRAM);
	gnutls_handshake_set_timeout(session, 20 * 1000);
	gnutls_dtls_set_mtu(session, 1500);

	/* avoid calling all the priority functions, since the defaults
	 * are adequate.
	 */
	gnutls_priority_set_direct(session,
				   "NONE:+VERS-DTLS0.9:+COMP-NULL:+AES-128-CBC:+AES-128-GCM:+AEAD:+SHA1:+RSA:%COMPAT",
				   NULL);

	ret = gnutls_session_set_premaster(session, GNUTLS_SERVER,
		proto, GNUTLS_KX_RSA,
		cipher, mac,
		GNUTLS_COMP_NULL, &master, &sess_id);
	if (ret < 0) {
		fail("server: gnutls_session_set_premaster failed: %s\n", gnutls_strerror(ret));
		exit(1);
	}

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);

	gnutls_transport_set_int(session, fd);
	gnutls_transport_set_push_function(session, push);

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

	memset(buffer, 1, sizeof(buffer));
	do {
		ret = gnutls_record_send(session, buffer, sizeof(buffer)-1);
	} while (ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED);

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

	gnutls_certificate_free_credentials(xcred);

	gnutls_global_deinit();

	if (debug)
		success("server: finished\n");
}

static void run(int proto, int cipher, int mac)
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

		close(fd[1]);
		server(fd[0], proto, cipher, mac);
		wait(&status);
		check_wait_status(status);
		close(fd[0]);
	} else {
		close(fd[0]);
		client(fd[1], proto, cipher, mac);
		close(fd[1]);
		exit(0);
	}
}

void doit(void)
{
	signal(SIGPIPE, SIG_IGN);

	run(GNUTLS_DTLS0_9, GNUTLS_CIPHER_AES_128_CBC, GNUTLS_MAC_SHA1);
	run(GNUTLS_DTLS0_9, GNUTLS_CIPHER_AES_128_GCM, GNUTLS_MAC_AEAD);
}

#endif				/* _WIN32 */
