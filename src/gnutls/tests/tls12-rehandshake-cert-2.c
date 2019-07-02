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
#include <gnutls/gnutls.h>
#include <gnutls/dtls.h>
#include <signal.h>

#include "utils.h"
#include "cert-common.h"

static void terminate(void);

/* This program tests client and server initiated rehandshake
 * behavior when they are refused by the peer.
 */

static void server_log_func(int level, const char *str)
{
	fprintf(stderr, "server|<%d>| %s", level, str);
}

static void client_log_func(int level, const char *str)
{
	fprintf(stderr, "client|<%d>| %s", level, str);
}


#define MAX_BUF 1024

static void client(int fd, unsigned test)
{
	int ret;
	char buffer[MAX_BUF + 1];
	gnutls_anon_client_credentials_t anoncred;
	gnutls_certificate_credentials_t x509_cred;
	gnutls_session_t session;
	/* Need to enable anonymous KX specifically. */

	global_init();
	memset(buffer, 2, sizeof(buffer));

	if (debug) {
		gnutls_global_set_log_function(client_log_func);
		gnutls_global_set_log_level(3);
	}

	gnutls_anon_allocate_client_credentials(&anoncred);
	gnutls_certificate_allocate_credentials(&x509_cred);

	/* Initialize TLS session
	 */
	gnutls_init(&session, GNUTLS_CLIENT);
	gnutls_handshake_set_timeout(session, 20 * 1000);

	/* Use default priorities */
	gnutls_priority_set_direct(session, "NORMAL:-VERS-TLS-ALL:+VERS-TLS1.1:+VERS-TLS1.2", NULL);

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

	if (debug)
		success("client: test %d\n", test);

	if (test != 0) {
		ret = gnutls_handshake(session);
		if (ret != GNUTLS_E_GOT_APPLICATION_DATA) {
			fail("client: error in code after rehandshake: %s\n",
			     gnutls_strerror(ret));
			exit(1);
		}

		do {
			do {
				ret =
				    gnutls_record_recv(session, buffer,
							MAX_BUF);
			} while (ret == GNUTLS_E_AGAIN
				 || ret == GNUTLS_E_INTERRUPTED);
		} while (ret > 0);

	} else {
		do {
			do {
				ret =
				    gnutls_record_recv(session, buffer,
							MAX_BUF);
			} while (ret == GNUTLS_E_AGAIN
				 || ret == GNUTLS_E_INTERRUPTED);
		} while (ret > 0);

		if (ret != GNUTLS_E_REHANDSHAKE) {
			fail("client: Error receiving rehandshake: %s\n",
			     gnutls_strerror(ret));
			exit(1);
		}

		do {
			ret =
			    gnutls_record_send(session, buffer, sizeof(buffer));
		} while (ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED);

		if (ret < 0) {
			fail("Error sending %d byte packet: %s\n",
			     (int)sizeof(buffer), gnutls_strerror(ret));
			exit(1);
		}

		do {
			ret =
			    gnutls_record_send(session, buffer, sizeof(buffer));
		} while (ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED);

		if (ret < 0) {
			fail("Error sending %d byte packet: %s\n",
			     (int)sizeof(buffer), gnutls_strerror(ret));
			exit(1);
		}

	}

	gnutls_bye(session, GNUTLS_SHUT_WR);

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

static void server(int fd, unsigned test)
{
	int ret;
	char buffer[MAX_BUF + 1];
	gnutls_session_t session;
	gnutls_anon_server_credentials_t anoncred;
	gnutls_certificate_credentials_t x509_cred;

	/* this must be called once in the program
	 */
	global_init();
	memset(buffer, 0, sizeof(buffer));

	if (debug) {
		gnutls_global_set_log_function(server_log_func);
		gnutls_global_set_log_level(3);
	}

	gnutls_certificate_allocate_credentials(&x509_cred);
	gnutls_certificate_set_x509_key_mem(x509_cred, &server_cert,
					    &server_key, GNUTLS_X509_FMT_PEM);

	gnutls_anon_allocate_server_credentials(&anoncred);

	gnutls_init(&session, GNUTLS_SERVER);
	gnutls_handshake_set_timeout(session, 20 * 1000);

	/* avoid calling all the priority functions, since the defaults
	 * are adequate.
	 */
	gnutls_priority_set_direct(session, "NORMAL:-VERS-TLS-ALL:+VERS-TLS1.1:+VERS-TLS1.2", NULL);

	gnutls_credentials_set(session, GNUTLS_CRD_ANON, anoncred);
	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, x509_cred);

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

	if (debug)
		success("server: test %d\n", test);

	if (test != 0) {

		do {
			do {
				ret =
				    gnutls_record_recv(session, buffer,
							MAX_BUF);
			} while (ret == GNUTLS_E_AGAIN
				 || ret == GNUTLS_E_INTERRUPTED);
		} while (ret > 0);

		if (ret != GNUTLS_E_REHANDSHAKE) {
			fail("server: Error receiving client handshake request: %s\n", gnutls_strerror(ret));
			terminate();
		}

		do {
			ret =
			    gnutls_record_send(session, buffer, sizeof(buffer));
		} while (ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED);

		if (ret < 0) {
			fail("Error sending %d byte packet: %s\n",
			     (int)sizeof(buffer), gnutls_strerror(ret));
			terminate();
		}

	} else {
		if (debug)
			success("server: sending rehandshake request\n");

		ret = gnutls_rehandshake(session);
		if (ret < 0) {
			fail("Error sending %d byte packet: %s\n",
			     (int)sizeof(buffer), gnutls_strerror(ret));
			terminate();
		}

		if (debug)
			success("server: starting handshake\n");

		ret = gnutls_handshake(session);

		if (ret != GNUTLS_E_GOT_APPLICATION_DATA) {
			fail("server: didn't receive GNUTLS_E_GOT_APPLICATION_DATA: %s\n", gnutls_strerror(ret));
			terminate();
		}

		if (debug)
			success("server: got application data error code: %s\n",
				gnutls_strerror(ret));

		do {
			do {
				ret =
				    gnutls_record_recv(session, buffer,
							MAX_BUF);
			} while (ret == GNUTLS_E_AGAIN
				 || ret == GNUTLS_E_INTERRUPTED);
		} while (ret > 0);

		if (debug)
			success("server: final ret: %s\n",
				gnutls_strerror(ret));

		if (ret < 0) {
			fail("Error receiving final packet: %s\n",
			     gnutls_strerror(ret));
			terminate();
		}

	}

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

static void start(unsigned test)
{
	int fd[2];
	int ret;
	int status = 0;

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
		server(fd[0], test);
		wait(&status);
		check_wait_status(status);
	} else {
		close(fd[0]);
		client(fd[1], test);
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

	start(0);
	start(1);
}

#endif				/* _WIN32 */
