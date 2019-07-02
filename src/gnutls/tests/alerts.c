/*
 * Copyright (C) 2018 Red Hat, Inc.
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
 *
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
#include "cert-common.h"

#include "utils.h"

static void terminate(void);

/* This program tests gnutls_alert_send().
 */

static void server_log_func(int level, const char *str)
{
	fprintf(stderr, "server|<%d>| %s", level, str);
}

static void client_log_func(int level, const char *str)
{
	fprintf(stderr, "client|<%d>| %s", level, str);
}

static void client(int fd, const char *prio, int ign)
{
	int ret;
	char buffer[64];
	const char* err;
	gnutls_certificate_credentials_t x509_cred;
	gnutls_session_t session;
	/* Need to enable anonymous KX specifically. */

	global_init();
	memset(buffer, 2, sizeof(buffer));

	if (debug) {
		gnutls_global_set_log_function(client_log_func);
		gnutls_global_set_log_level(7);
	}

	gnutls_certificate_allocate_credentials(&x509_cred);

	/* Initialize TLS session
	 */
	gnutls_init(&session, GNUTLS_CLIENT);

	/* Use default priorities */
	ret = gnutls_priority_set_direct(session, prio, &err);
	if (ret < 0) {
		fail("error setting priority: %s\n", err);
		exit(1);
	}

	/* put the anonymous credentials to the current session
	 */
	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, x509_cred);

	gnutls_transport_set_int(session, fd);

	/* Perform the TLS handshake
	 */
	do {
		ret = gnutls_handshake(session);
	}
	while (ret < 0 && gnutls_error_is_fatal(ret) == 0);

	if (ret < 0) {
		fail("client (%s): Handshake has failed (%s)\n\n", prio,
		     gnutls_strerror(ret));
		exit(1);
	} else {
		if (debug)
			success("client: Handshake was completed\n");
	}

	if (debug)
		success("client: TLS version is: %s\n",
			gnutls_protocol_get_name
			(gnutls_protocol_get_version(session)));

	ret = gnutls_alert_send(session, GNUTLS_AL_WARNING, GNUTLS_A_USER_CANCELED);
	if (ret < 0) {
		fail("server: Error sending user cancelled alert: %s\n", gnutls_strerror(ret));
		terminate();
	}

	ret = gnutls_alert_send(session, GNUTLS_AL_FATAL, GNUTLS_A_DECRYPT_ERROR);
	if (ret < 0) {
		fail("server: Error sending decrypt error alert: %s\n", gnutls_strerror(ret));
		terminate();
	}

	close(fd);

	gnutls_deinit(session);

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

static void server(int fd, const char *prio, int ign)
{
	int ret;
	const char* err;
	char buffer[64];
	gnutls_session_t session;
	gnutls_certificate_credentials_t x509_cred;

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


	gnutls_init(&session, GNUTLS_SERVER);

	/* avoid calling all the priority functions, since the defaults
	 * are adequate.
	 */
	ret = gnutls_priority_set_direct(session, prio, &err);
	if (ret < 0) {
		fail("error setting priority: %s\n", err);
		exit(1);
	}

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, x509_cred);

	gnutls_transport_set_int(session, fd);

	do {
		ret = gnutls_handshake(session);
	}
	while (ret < 0 && gnutls_error_is_fatal(ret) == 0);
	if (ret < 0) {
		close(fd);
		gnutls_deinit(session);
		fail("server (%s): Handshake has failed (%s)\n\n", prio,
		     gnutls_strerror(ret));
		terminate();
	}
	if (debug)
		success("server: Handshake was completed\n");

	if (debug)
		success("server: TLS version is: %s\n",
			gnutls_protocol_get_name
			(gnutls_protocol_get_version(session)));

	do {
		do {
			ret = gnutls_record_recv(session, buffer, sizeof(buffer));
		} while (ret == GNUTLS_E_AGAIN
			 || ret == GNUTLS_E_INTERRUPTED);
		if (ret > 0)
			fail("error receiving alert: ret: %d\n", ret);
	} while (ret > 0);

	if (ret != GNUTLS_E_WARNING_ALERT_RECEIVED ||
	    gnutls_alert_get(session) != GNUTLS_A_USER_CANCELED) {
		fail("client: Error: %s\n", gnutls_strerror(ret));
	}

	do {
		do {
			ret = gnutls_record_recv(session, buffer, sizeof(buffer));
		} while (ret == GNUTLS_E_AGAIN
			 || ret == GNUTLS_E_INTERRUPTED);
		if (ret > 0)
			fail("error receiving alert: ret: %d\n", ret);
	} while (ret > 0);

	if (ret != GNUTLS_E_FATAL_ALERT_RECEIVED ||
	    gnutls_alert_get(session) != GNUTLS_A_DECRYPT_ERROR) {
		fail("client: Error: %s\n", gnutls_strerror(ret));
	}

	close(fd);
	gnutls_deinit(session);

	gnutls_certificate_free_credentials(x509_cred);

	gnutls_global_deinit();

	if (debug)
		success("server: finished\n");
}

static void start(const char *prio, int ign)
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
		server(fd[0], prio, ign);
	} else {
		close(fd[0]);
		client(fd[1], prio, ign);
		exit(0);
	}
}

#define AES_GCM "NONE:+VERS-TLS1.2:-CIPHER-ALL:+RSA:+AES-128-GCM:+MAC-ALL:+SIGN-ALL:+COMP-NULL:+ANON-ECDH:+CURVE-ALL"

static void ch_handler(int sig)
{
	int status = 0;
	wait(&status);
	check_wait_status(status);
	return;
}

void doit(void)
{
	signal(SIGCHLD, ch_handler);

	start(AES_GCM, 0);
}

#endif				/* _WIN32 */
