/*
 * Copyright (C) 2019 Free Software Foundation, Inc.
 *
 * Author: Aniketh Girish
 *
 * This file is part of GnuTLS.
 *
 * The GnuTLS is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
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
#include <assert.h>

#if !defined(__linux__) || !defined(__GNUC__)

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
#include <gnutls/crypto.h>

#include "cert-common.h"
#include "utils.h"

/* This program tests whether a keylog function is called.
 */

static void terminate(void);

static void server_log_func(int level, const char *str)
{
	fprintf(stderr, "server|<%d>| %s", level, str);
}

static void client_log_func(int level, const char *str)
{
	fprintf(stderr, "client|<%d>| %s", level, str);
}

const char *side = "";

/* These are global */
static pid_t child;
#define MAX_BUF 1024
#define MSG "Hello TLS"

static int
keylog_func(gnutls_session_t session,
	    const char *label,
	    const gnutls_datum_t *secret)
{
	unsigned int *call_count = gnutls_session_get_ptr(session);
	static const char *exp_labels[] = {
		"CLIENT_HANDSHAKE_TRAFFIC_SECRET",
		"SERVER_HANDSHAKE_TRAFFIC_SECRET",
		"EXPORTER_SECRET",
		"CLIENT_TRAFFIC_SECRET_0",
		"SERVER_TRAFFIC_SECRET_0"
	};

	if (*call_count >= sizeof(exp_labels)/sizeof(exp_labels[0]))
		fail("unexpected secret at call count %u\n",
		     *call_count);

	if (strcmp(label, exp_labels[*call_count]) != 0)
		fail("unexpected %s at call count %u\n",
		     label, *call_count);
	else if (debug)
		success("received %s at call count %u\n",
			label, *call_count);

	(*call_count)++;
	return 0;
}

static void client(int fd, const char *prio, unsigned int exp_call_count)
{
	gnutls_session_t session;
	char buffer[MAX_BUF + 1];
	unsigned int call_count = 0;
	int ret, ii;
	gnutls_certificate_credentials_t clientx509cred;
	const char *err;
	/* Need to enable anonymous KX specifically. */

	global_init();

	if (debug) {
		gnutls_global_set_log_function(client_log_func);
		gnutls_global_set_log_level(4711);
	}

	gnutls_certificate_allocate_credentials(&clientx509cred);

	/* Initialize TLS session
	 */
	gnutls_init(&session, GNUTLS_CLIENT);

	gnutls_session_set_ptr(session, &call_count);

	/* Use default priorities */
	ret = gnutls_priority_set_direct(session, prio, &err);
	if (ret < 0) {
		fail("client: priority set failed (%s): %s\n",
		     gnutls_strerror(ret), err);
		exit(1);
	}

	ret = gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE,
				clientx509cred);
	if (ret < 0)
		exit(1);

	gnutls_transport_set_int(session, fd);

	gnutls_session_set_keylog_function(session, keylog_func);
	assert(gnutls_session_get_keylog_function(session) == keylog_func);

	/* Perform the TLS handshake
	 */
	do {
		ret = gnutls_handshake(session);
	}
	while (ret < 0 && gnutls_error_is_fatal(ret) == 0);

	if (ret < 0)
		fail("client: Handshake failed: %s\n", gnutls_strerror(ret));
	else {
		if (debug)
			success("client: Handshake was completed\n");
	}

	if (debug)
		success("client: TLS version is: %s\n",
			gnutls_protocol_get_name
			(gnutls_protocol_get_version(session)));

	gnutls_record_send(session, MSG, strlen(MSG));

	do {
		ret = gnutls_record_recv(session, buffer, MAX_BUF);
	} while (ret == GNUTLS_E_AGAIN);
	if (ret == 0) {
		if (debug)
			success
				("client: Peer has closed the TLS connection\n");
	} else if (ret < 0) {
		fail("client: Error: %s\n", gnutls_strerror(ret));
	}

	if (debug) {
		printf("- Received %d bytes: ", ret);
		for (ii = 0; ii < ret; ii++) {
			fputc(buffer[ii], stdout);
		}
		fputs("\n", stdout);
	}

	if (call_count != exp_call_count)
		fail("secret hook is not called %u times (%u)\n",
		     call_count, exp_call_count);

	gnutls_bye(session, GNUTLS_SHUT_WR);

	close(fd);

	gnutls_deinit(session);

	gnutls_certificate_free_credentials(clientx509cred);

	gnutls_global_deinit();
}

static void server(int fd, const char *prio, unsigned int exp_call_count)
{
	int ret;
	char buffer[MAX_BUF + 1];
	gnutls_session_t session;
	unsigned int call_count = 0;
	gnutls_certificate_credentials_t serverx509cred;

	/* this must be called once in the program
	 */
	global_init();

	if (debug) {
		gnutls_global_set_log_function(server_log_func);
		gnutls_global_set_log_level(4711);
	}

	gnutls_certificate_allocate_credentials(&serverx509cred);

	gnutls_init(&session, GNUTLS_SERVER);

	gnutls_session_set_ptr(session, &call_count);

	/* avoid calling all the priority functions, since the defaults
	 * are adequate.
	 */
	ret = gnutls_priority_set_direct(session,
					 "NORMAL:-VERS-ALL:+VERS-TLS1.3:-KX-ALL:-SIGN-ALL:+SIGN-RSA-PSS-RSAE-SHA384:-GROUP-ALL:+GROUP-SECP256R1", NULL);
	if (ret < 0) {
		fail("server: priority set failed (%s)\n\n",
		     gnutls_strerror(ret));
		terminate();
	}

	gnutls_certificate_set_x509_key_mem(serverx509cred,
					    &server_cert, &server_key,
					    GNUTLS_X509_FMT_PEM);
	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE,
				serverx509cred);

	gnutls_transport_set_int(session, fd);

	gnutls_session_set_keylog_function(session, keylog_func);

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
	if (debug) {
		success("server: Handshake was completed\n");
	}
	if (debug)
		success("server: TLS version is: %s\n",
			gnutls_protocol_get_name
			(gnutls_protocol_get_version(session)));

	memset(buffer, 0, MAX_BUF + 1);

	do {
		ret = gnutls_record_recv(session, buffer, MAX_BUF);
	} while (ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED);

	if (ret == 0) {
		if (debug)
			success("server: Peer has closed the GnuTLS connection\n");
	} else if (ret < 0) {
		fail("server: Received corrupted data(%d). Closing...\n", ret);
	} else if (ret > 0) {
		/* echo data back to the client
		 */
		gnutls_record_send(session, buffer,
				   strlen(buffer));
	}

	if (call_count != exp_call_count)
		fail("secret hook is not called %u times (%u)\n",
		     call_count, exp_call_count);

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

static void terminate(void)
{
	int status = 0;

	kill(child, SIGTERM);
	wait(&status);
	exit(1);
}

static void
run(const char *prio, unsigned int exp_call_count)
{
	int fd[2];
	int ret;

	signal(SIGPIPE, SIG_IGN);

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

		server(fd[0], prio, exp_call_count);
		wait(&status);
		check_wait_status(status);
	} else {
		close(fd[0]);
		client(fd[1], prio, exp_call_count);
		exit(0);
	}
}

void doit(void)
{
	run("NORMAL:-VERS-ALL:+VERS-TLS1.3", 5);
}

#endif				/* _WIN32 */
