/*
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
#include <signal.h>
#include <assert.h>

#include "utils.h"
#include "cert-common.h"

static void terminate(void);

/* This program tests whether EtM is negotiated as expected.
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

static void client(int fd, const char *prio, unsigned etm)
{
	int ret;
	char buffer[MAX_BUF + 1];
	gnutls_anon_client_credentials_t anoncred;
	gnutls_certificate_credentials_t x509_cred;
	gnutls_session_t session;
	/* Need to enable anonymous KX specifically. */

	global_init();

	if (debug) {
		gnutls_global_set_log_function(client_log_func);
		gnutls_global_set_log_level(7);
	}

	gnutls_anon_allocate_client_credentials(&anoncred);
	gnutls_certificate_allocate_credentials(&x509_cred);

	/* Initialize TLS session
	 */
	gnutls_init(&session, GNUTLS_CLIENT);

	/* Use default priorities */
	assert(gnutls_priority_set_direct(session, prio, NULL)>=0);

	/* put the anonymous credentials to the current session
	 */
	gnutls_credentials_set(session, GNUTLS_CRD_ANON, anoncred);
	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, x509_cred);

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

	if (etm != 0 && gnutls_session_etm_status(session) == 0) {
		fail("client: EtM was not negotiated with %s!\n", prio);
		exit(1);
	} else if (etm == 0 && gnutls_session_etm_status(session) != 0) {
		fail("client: EtM was negotiated with %s!\n", prio);
		exit(1);
	}

	if (etm != 0 && ((gnutls_session_get_flags(session) & GNUTLS_SFLAGS_ETM) == 0)) {
		fail("client: EtM was not negotiated with %s!\n", prio);
		exit(1);
	} else if (etm == 0 && ((gnutls_session_get_flags(session) & GNUTLS_SFLAGS_ETM) != 0)) {
		fail("client: EtM was negotiated with %s!\n", prio);
		exit(1);
	}

	do {
		do {
			ret = gnutls_record_recv(session, buffer, MAX_BUF);
		} while (ret == GNUTLS_E_AGAIN
			 || ret == GNUTLS_E_INTERRUPTED);
	} while (ret > 0);

	if (ret == 0) {
		if (debug)
			success
			    ("client: Peer has closed the TLS connection\n");
		goto end;
	} else if (ret < 0) {
		if (ret != 0) {
			fail("client: Error: %s\n", gnutls_strerror(ret));
			exit(1);
		}
	}

	gnutls_bye(session, GNUTLS_SHUT_WR);

      end:

	close(fd);

	gnutls_deinit(session);

	gnutls_anon_free_client_credentials(anoncred);
	gnutls_certificate_free_credentials(x509_cred);

	gnutls_global_deinit();
}


/* These are global */
pid_t child;

static void terminate(void)
{
	kill(child, SIGTERM);
	exit(1);
}

static void server(int fd, const char *prio, unsigned etm)
{
	int ret;
	char buffer[MAX_BUF + 1];
	gnutls_session_t session;
	gnutls_anon_server_credentials_t anoncred;
	gnutls_certificate_credentials_t x509_cred;
	unsigned to_send = sizeof(buffer)/4;

	/* this must be called once in the program
	 */
	global_init();
	memset(buffer, 0, sizeof(buffer));

	if (debug) {
		gnutls_global_set_log_function(server_log_func);
		gnutls_global_set_log_level(4711);
	}

	gnutls_certificate_allocate_credentials(&x509_cred);
	gnutls_certificate_set_x509_key_mem(x509_cred, &server_cert,
					    &server_key,
					    GNUTLS_X509_FMT_PEM);

	gnutls_anon_allocate_server_credentials(&anoncred);

	gnutls_init(&session, GNUTLS_SERVER);

	/* avoid calling all the priority functions, since the defaults
	 * are adequate.
	 */
	assert(gnutls_priority_set_direct(session, prio, NULL) >= 0);

	gnutls_credentials_set(session, GNUTLS_CRD_ANON, anoncred);
	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, x509_cred);

	gnutls_transport_set_int(session, fd);

	do {
		ret = gnutls_handshake(session);
	} while (ret < 0 && gnutls_error_is_fatal(ret) == 0);
	if (ret < 0) {
		close(fd);
		gnutls_deinit(session);
		fail("server: Handshake has failed (%s)\n\n",
		     gnutls_strerror(ret));
		terminate();
	}

	if (etm != 0 && gnutls_session_etm_status(session) == 0) {
		fail("server: EtM was not negotiated with %s!\n", prio);
		exit(1);
	} else if (etm == 0 && gnutls_session_etm_status(session) != 0) {
		fail("server: EtM was negotiated with %s!\n", prio);
		exit(1);
	}

	if (etm != 0 && ((gnutls_session_get_flags(session) & GNUTLS_SFLAGS_ETM) == 0)) {
		fail("server: EtM was not negotiated with %s!\n", prio);
		exit(1);
	} else if (etm == 0 && ((gnutls_session_get_flags(session) & GNUTLS_SFLAGS_ETM) != 0)) {
		fail("server: EtM was negotiated with %s!\n", prio);
		exit(1);
	}

	if (debug)
		success("server: Handshake was completed\n");

	if (debug)
		success("server: TLS version is: %s\n",
			gnutls_protocol_get_name
			(gnutls_protocol_get_version(session)));

	do {
		do {
			ret =
			    gnutls_record_send(session, buffer,
						sizeof(buffer));
		} while (ret == GNUTLS_E_AGAIN
			 || ret == GNUTLS_E_INTERRUPTED);

		if (ret < 0) {
			fail("Error sending %d byte packet: %s\n", to_send,
			     gnutls_strerror(ret));
			terminate();
		}
		to_send++;
	}
	while (to_send < 64);

	to_send = -1;
	/* do not wait for the peer to close the connection.
	 */
	gnutls_bye(session, GNUTLS_SHUT_WR);

	close(fd);
	gnutls_deinit(session);

	gnutls_anon_free_server_credentials(anoncred);
	gnutls_certificate_free_credentials(x509_cred);

	gnutls_global_deinit();

	if (debug)
		success("server: finished\n");
}

static void start(const char *prio, unsigned etm)
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
		/* parent */
		close(fd[1]);
		server(fd[0], prio, etm);
		kill(child, SIGTERM);
	} else {
		close(fd[0]);
		client(fd[1], prio, etm);
		exit(0);
	}
}

#define AES_CBC "NONE:+VERS-TLS1.0:-CIPHER-ALL:+AES-128-CBC:+SHA1:+SIGN-ALL:+COMP-ALL:+ANON-ECDH:+CURVE-ALL"
#define AES_CBC_SHA256 "NONE:+VERS-TLS1.2:-CIPHER-ALL:+RSA:+AES-128-CBC:+AES-256-CBC:+SHA256:+SIGN-ALL:+COMP-ALL:+ANON-ECDH:+CURVE-ALL"
#define AES_GCM "NONE:+VERS-TLS1.2:-CIPHER-ALL:+RSA:+AES-128-GCM:+MAC-ALL:+SIGN-ALL:+COMP-ALL:+ANON-ECDH:+CURVE-ALL"

static void ch_handler(int sig)
{
	int status;
	wait(&status);
	check_wait_status(status);
	return;
}

void doit(void)
{
	signal(SIGCHLD, ch_handler);

	start(AES_CBC, 1);
	start(AES_CBC_SHA256, 1);
	start(AES_GCM, 0);
}

#endif				/* _WIN32 */
