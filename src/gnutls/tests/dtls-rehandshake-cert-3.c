/*
 * Copyright (C) 2016 Red Hat, Inc.
 * Copyright (C) 2016 Guillaume Roguez
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

#define MTU 1500
#define MAX_BUF 4096
#define MSG "Hello TLS"

static int server_fd = -1;
static char pkt_buf[MAX_BUF];
static int pkt_found = 0;
static int pkt_delivered = 0;

static void terminate(void);

/* This program tests the rehandshake from anon
 * to certificate auth in DTLS, but will account for
 * packet reconstruction (with loss/delay) for the certificate packet.
 */

static void server_log_func(int level, const char *str)
{
	fprintf(stderr, "server|<%d>| %s", level, str);
}

static void client_log_func(int level, const char *str)
{
	fprintf(stderr, "client|<%d>| %s", level, str);
}

static ssize_t push(gnutls_transport_ptr_t tr, const void *data, size_t len)
{
	int fd = (long int)tr;

	if (fd == server_fd) {
		if (!pkt_found && len > 1200) {
			memcpy(pkt_buf, data, len);
			pkt_found = 1;
			if (debug)
				success("*** packet delayed\n");
			return len;
		}
		if (pkt_found && !pkt_delivered) {
			int res = send(fd, data, len, 0);
			send(fd, pkt_buf, MTU, 0);
			pkt_delivered = 1;
			if (debug)
				success("*** swap done\n");
			return res;
		}
	}

	return send(fd, data, len, 0);
}

static void client(int fd, const char *prio)
{
	int ret;
	char buffer[MAX_BUF + 1];
	gnutls_certificate_credentials_t clientx509cred;
	gnutls_anon_client_credentials_t anoncred;
	gnutls_session_t session;

	global_init();

	if (debug) {
		gnutls_global_set_log_function(client_log_func);
		gnutls_global_set_log_level(4711);
	}

	assert(gnutls_anon_allocate_client_credentials(&anoncred) >= 0);
	assert(gnutls_certificate_allocate_credentials(&clientx509cred) >= 0);

	/* Initialize TLS session
	 */
	gnutls_init(&session, GNUTLS_CLIENT | GNUTLS_DATAGRAM);
	gnutls_dtls_set_mtu(session, MTU);

	snprintf(buffer, sizeof(buffer), "%s:+ANON-ECDH", prio);
	assert(gnutls_priority_set_direct(session,
					  buffer,
					  NULL) >= 0);

	gnutls_credentials_set(session, GNUTLS_CRD_ANON, anoncred);
	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, clientx509cred);

	gnutls_transport_set_int(session, fd);
	gnutls_transport_set_push_function(session, push);
	gnutls_dtls_set_timeouts(session, 2000, 30 * 1000);

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

	/* update priorities to allow cert auth */
	snprintf(buffer, sizeof(buffer), "%s:+ECDHE-RSA", prio);
	assert(gnutls_priority_set_direct(session,
					  buffer,
					  NULL) >= 0);

	do {
		ret = gnutls_record_recv(session, buffer, MAX_BUF);
	} while (ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED);

	if (ret == 0) {
		if (debug)
			success("client: Peer has closed the TLS connection\n");
		goto end;
	} else if (ret < 0) {
		if (ret == GNUTLS_E_REHANDSHAKE) {
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

static void server(int fd, const char *prio)
{
	int ret;
	char buffer[MAX_BUF + 1];
	gnutls_certificate_credentials_t serverx509cred;
	gnutls_anon_server_credentials_t anoncred;
	gnutls_session_t session;
	/* this must be called once in the program
	 */
	global_init();

	if (debug) {
		gnutls_global_set_log_function(server_log_func);
		gnutls_global_set_log_level(4711);
	}

	assert(gnutls_anon_allocate_server_credentials(&anoncred) >= 0);
	assert(gnutls_certificate_allocate_credentials(&serverx509cred) >= 0);
	assert(gnutls_certificate_set_x509_key_mem(serverx509cred,
						   &server_cert, &server_key,
						   GNUTLS_X509_FMT_PEM) >= 0);

	gnutls_init(&session, GNUTLS_SERVER | GNUTLS_DATAGRAM);
	gnutls_dtls_set_mtu(session, MTU);

	/* avoid calling all the priority functions, since the defaults
	 * are adequate.
	 */
	snprintf(buffer, sizeof(buffer), "%s:+ECDHE-RSA:+ANON-ECDH", prio);
	assert(gnutls_priority_set_direct(session,
					  buffer,
					  NULL) >= 0);

	gnutls_credentials_set(session, GNUTLS_CRD_ANON, anoncred);
	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, serverx509cred);

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

	if (gnutls_kx_get(session) != GNUTLS_KX_ANON_ECDH) {
		fail("did not negotiate an anonymous ciphersuite on initial auth\n");
	}

	/* see the Getting peer's information example */
	/* print_info(session); */

	if (debug)
		success("server: Sending dummy packet\n");
	ret = gnutls_rehandshake(session);
	if (ret < 0) {
		fail("gnutls_rehandshake: %s\n", gnutls_strerror(ret));
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

	for (;;) {
		memset(buffer, 0, MAX_BUF + 1);

		do {
			ret = gnutls_record_recv(session, buffer, MAX_BUF);
		} while (ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED);

		if (ret == 0) {
			if (debug)
				success
				    ("server: Peer has closed the GnuTLS connection\n");
			break;
		} else if (ret < 0) {
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

	if (gnutls_kx_get(session) != GNUTLS_KX_ECDHE_RSA) {
		fail("did not negotiate a certificate ciphersuite on second auth\n");
	}

	/* do not wait for the peer to close the connection.
	 */
	gnutls_bye(session, GNUTLS_SHUT_WR);

	close(fd);
	gnutls_deinit(session);

	gnutls_certificate_free_credentials(serverx509cred);
	gnutls_anon_free_server_credentials(anoncred);

	gnutls_global_deinit();

	if (debug)
		success("server: finished\n");
}

static void start(const char *prio)
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

		server_fd = fd[0];
		server(fd[0], prio);
		wait(&status);
		check_wait_status(status);
	} else {
		close(fd[0]);
		client(fd[1], prio);
		exit(0);
	}
}

void doit(void)
{
	start("NONE:+VERS-DTLS1.2:+CIPHER-ALL:+MAC-ALL:+SIGN-ALL:+COMP-ALL:+CURVE-ALL");
}

#endif				/* _WIN32 */
