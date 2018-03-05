/*
 * Copyright (C) 2016 Red Hat, Inc.
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
#include <gnutls/gnutls.h>
#include <gnutls/dtls.h>
#include <assert.h>

#include "cert-common.h"
#include "utils.h"

static void terminate(void);

/* This program tests the rehandshake in DTLS
 */

static void server_log_func(int level, const char *str)
{
	fprintf(stderr, "server|<%d>| %s", level, str);
}

static void client_log_func(int level, const char *str)
{
	fprintf(stderr, "client|<%d>| %s", level, str);
}

/* Tests the operation of rehandshake under DTLS using
 * certificates.
 */

#define MAX_BUF 1024
#define MSG "Hello TLS"

static ssize_t
push(gnutls_transport_ptr_t tr, const void *data, size_t len)
{
	int fd = (long int) tr;

	return send(fd, data, len, 0);
}

static void client(int fd, int server_init)
{
	int ret;
	char buffer[MAX_BUF + 1];
	gnutls_certificate_credentials_t clientx509cred;
	gnutls_session_t session;

	global_init();

	if (debug) {
		gnutls_global_set_log_function(client_log_func);
		gnutls_global_set_log_level(4711);
	}

	assert(gnutls_certificate_allocate_credentials(&clientx509cred) >= 0);

	/* Initialize TLS session
	 */
	gnutls_init(&session, GNUTLS_CLIENT | GNUTLS_DATAGRAM);
	gnutls_dtls_set_mtu(session, 1500);

	/* Use default priorities */
	assert(gnutls_priority_set_direct(session,
				   "NONE:+VERS-DTLS-ALL:+CIPHER-ALL:+MAC-ALL:+SIGN-ALL:+COMP-ALL:+ECDHE-RSA:+CURVE-ALL",
				   NULL) >= 0);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE,
				clientx509cred);

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

	if (!server_init) {
		sec_sleep(60);
		if (debug)
			success("Initiating client rehandshake\n");
		do {
			ret = gnutls_handshake(session);
		}
		while (ret < 0 && gnutls_error_is_fatal(ret) == 0);

		if (ret < 0) {
			fail("2nd client gnutls_handshake: %s\n",
			     gnutls_strerror(ret));
			terminate();
		}
	} else {
		do {
			ret = gnutls_record_recv(session, buffer, MAX_BUF);
		} while (ret == GNUTLS_E_AGAIN
			 || ret == GNUTLS_E_INTERRUPTED);
	}

	if (ret == 0) {
		if (debug)
			success
			    ("client: Peer has closed the TLS connection\n");
		goto end;
	} else if (ret < 0) {
		if (server_init && ret == GNUTLS_E_REHANDSHAKE) {
			if (debug)
				success
				    ("Initiating rehandshake due to server request\n");
			do {
				ret = gnutls_handshake(session);
			}
			while (ret < 0 && gnutls_error_is_fatal(ret) == 0);
		}

		if (ret != 0) {
			fail("client: Error: %s\n", gnutls_strerror(ret));
			exit(1);
		}
	}

	do {
		ret = gnutls_record_send(session, MSG, strlen(MSG));
	} while (ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED);
	gnutls_bye(session, GNUTLS_SHUT_WR);

      end:

	close(fd);

	gnutls_deinit(session);

	gnutls_certificate_free_credentials(clientx509cred);

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

static void server(int fd, int server_init)
{
	int ret;
	char buffer[MAX_BUF + 1];
	gnutls_certificate_credentials_t serverx509cred;
	gnutls_session_t session;
	/* this must be called once in the program
	 */
	global_init();

	if (debug) {
		gnutls_global_set_log_function(server_log_func);
		gnutls_global_set_log_level(4711);
	}

	assert(gnutls_certificate_allocate_credentials(&serverx509cred) >= 0);
	assert(gnutls_certificate_set_x509_key_mem(serverx509cred,
					    &server_cert, &server_key,
					    GNUTLS_X509_FMT_PEM) >= 0);

	gnutls_init(&session, GNUTLS_SERVER | GNUTLS_DATAGRAM);
	gnutls_dtls_set_mtu(session, 1500);

	/* avoid calling all the priority functions, since the defaults
	 * are adequate.
	 */
	assert(gnutls_priority_set_direct(session,
				   "NONE:+VERS-DTLS-ALL:+CIPHER-ALL:+MAC-ALL:+SIGN-ALL:+COMP-ALL:+ECDHE-RSA:+CURVE-ALL",
				   NULL) >= 0);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE,
				serverx509cred);

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

	if (server_init) {
		if (debug)
			success("server: Sending dummy packet\n");
		ret = gnutls_rehandshake(session);
		if (ret < 0) {
			fail("gnutls_rehandshake: %s\n",
			     gnutls_strerror(ret));
			terminate();
		}

		if (debug)
			success("server: Initiating rehandshake\n");
		do {
			ret = gnutls_handshake(session);
		}
		while (ret < 0 && gnutls_error_is_fatal(ret) == 0);

		if (ret < 0) {
			fail("server: 2nd gnutls_handshake: %s\n",
			     gnutls_strerror(ret));
			terminate();
		}
	}

	for (;;) {
		memset(buffer, 0, MAX_BUF + 1);

		do {
			ret = gnutls_record_recv(session, buffer, MAX_BUF);
		} while (ret == GNUTLS_E_AGAIN
			 || ret == GNUTLS_E_INTERRUPTED);

		if (ret == 0) {
			if (debug)
				success
				    ("server: Peer has closed the GnuTLS connection\n");
			break;
		} else if (ret < 0) {
			if (!server_init && ret == GNUTLS_E_REHANDSHAKE) {
				if (debug)
					success
					    ("Initiating rehandshake due to client request\n");
				do {
					ret = gnutls_handshake(session);
				}
				while (ret < 0
					&& gnutls_error_is_fatal(ret) == 0);
				if (ret == 0)
					break;
			}

			fail("server: Received corrupted data(%s). Closing...\n", gnutls_strerror(ret));
			terminate();
		} else if (ret > 0) {
			/* echo data back to the client
			 */
			do {
				ret =
				    gnutls_record_send(session, buffer,
							strlen(buffer));
			} while (ret == GNUTLS_E_AGAIN
				 || ret == GNUTLS_E_INTERRUPTED);
		}
	}


	/* do not wait for the peer to close the connection.
	 */
	gnutls_bye(session, GNUTLS_SHUT_WR);

	close(fd);
	gnutls_deinit(session);

	gnutls_certificate_free_credentials(serverx509cred);

	gnutls_global_deinit();

	if (debug)
		success("server: finished\n");
}

static void start(int server_initiated)
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
		int status = 0;
		/* parent */

		server(fd[0], server_initiated);
		wait(&status);
		check_wait_status(status);
	} else {
		close(fd[0]);
		client(fd[1], server_initiated);
		exit(0);
	}
}

void doit(void)
{
	start(0);
	start(1);
}

#endif				/* _WIN32 */
