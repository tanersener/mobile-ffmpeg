/*
 * Copyright (C) 2016-2018 Red Hat, Inc
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
#include <time.h>
#include <gnutls/gnutls.h>
#include <gnutls/dtls.h>
#include <signal.h>
#include <assert.h>

#include "cert-common.h"
#include "utils.h"

static void terminate(void);

/* This program tests that handshakes do not include a session ticket
 * if the flag GNUTLS_NO_TICKETS is specified under TLS 1.2.
 *
 * Under TLS 1.3 it verifies that not enabling session tickets doesn't
 * result in a ticket being sent.
 */

static time_t mytime(time_t * t)
{
	time_t then = 1464610242;
	if (t)
		*t = then;

	return then;
}

static void server_log_func(int level, const char *str)
{
	fprintf(stderr, "server|<%d>| %s", level, str);
}

static void client_log_func(int level, const char *str)
{
	fprintf(stderr, "client|<%d>| %s", level, str);
}

static int sent = 0;

static int handshake_callback(gnutls_session_t session, unsigned int htype,
	unsigned post, unsigned int incoming, const gnutls_datum_t *msg)
{
	if (htype != GNUTLS_HANDSHAKE_NEW_SESSION_TICKET)
		return 0;
	fail("sent session ticket\n");
	sent = 1;
	return 0;
}

#define MAX_BUF 1024

static void client(int fd, const char *prio, unsigned int flags)
{
	int ret;
	gnutls_certificate_credentials_t x509_cred;
	gnutls_session_t session;
	/* Need to enable anonymous KX specifically. */

	flags |= GNUTLS_CLIENT;

	gnutls_global_set_time_function(mytime);
	global_init();

	if (debug) {
		gnutls_global_set_log_function(client_log_func);
		gnutls_global_set_log_level(7);
	}

	gnutls_certificate_allocate_credentials(&x509_cred);

	/* Initialize TLS session
	 */
	gnutls_init(&session, flags);

	assert(gnutls_priority_set_direct(session, prio, NULL)>=0);

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

	if (ret == GNUTLS_E_UNSUPPORTED_SIGNATURE_ALGORITHM) {
		/* success */
		goto end;
	}

	if (ret < 0) {
		fail("client: Handshake failed: %s\n", gnutls_strerror(ret));
		terminate();
	} else {
		if (debug)
			success("client: Handshake was completed\n");
	}

	if (debug)
		success("client: TLS version is: %s\n",
			gnutls_protocol_get_name
			(gnutls_protocol_get_version(session)));

	gnutls_bye(session, GNUTLS_SHUT_WR);

      end:

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

static void server(int fd, const char *prio, unsigned int flags)
{
	int ret;
	char buffer[MAX_BUF + 1];
	gnutls_session_t session;
	gnutls_certificate_credentials_t x509_cred;
	gnutls_datum_t skey = {NULL, 0};

	flags |= GNUTLS_SERVER;

	/* this must be called once in the program
	 */
	global_init();
	memset(buffer, 0, sizeof(buffer));

	if (debug) {
		gnutls_global_set_log_function(server_log_func);
		gnutls_global_set_log_level(4711);
	}

	assert(gnutls_certificate_allocate_credentials(&x509_cred)>=0);
	assert(gnutls_certificate_set_x509_key_mem(x509_cred, &server_cert,
					    &server_key,
					    GNUTLS_X509_FMT_PEM)>=0);

	assert(gnutls_init(&session, flags)>=0);

	if (!(flags & GNUTLS_NO_TICKETS)) {
		assert(gnutls_session_ticket_key_generate(&skey)>=0);
		assert(gnutls_session_ticket_enable_server(session, &skey) >= 0);
	}

	gnutls_handshake_set_hook_function(session, GNUTLS_HANDSHAKE_NEW_SESSION_TICKET,
					   GNUTLS_HOOK_POST,
					   handshake_callback);

	/* avoid calling all the priority functions, since the defaults
	 * are adequate.
	 */
	assert(gnutls_priority_set_direct(session, prio, NULL)>=0);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, x509_cred);

	gnutls_transport_set_int(session, fd);

	do {
		ret = gnutls_handshake(session);
	} while (ret < 0 && gnutls_error_is_fatal(ret) == 0);
	if (ret < 0) {
		/* failure is expected here */
		goto end;
	}

	if (debug) {
		success("server: Handshake was completed\n");
	}

	if (debug)
		success("server: TLS version is: %s\n",
			gnutls_protocol_get_name
			(gnutls_protocol_get_version(session)));

	if (sent != 0) {
		fail("new session ticket was sent\n");
		exit(1);
	}		

	/* do not wait for the peer to close the connection.
	 */
	gnutls_bye(session, GNUTLS_SHUT_WR);

 end:
	close(fd);
	gnutls_deinit(session);
	gnutls_free(skey.data);

	gnutls_certificate_free_credentials(x509_cred);

	gnutls_global_deinit();

	if (debug)
		success("server: finished\n");
}

static void ch_handler(int sig)
{
	return;
}

static
void start2(const char *prio, const char *sprio, unsigned int flags, unsigned int sflags)
{
	int fd[2];
	int ret, status = 0;

	success("trying %s\n", prio);

	sent = 0;
	signal(SIGCHLD, ch_handler);
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
		/* parent */
		close(fd[1]);
		server(fd[0], sprio, sflags);
		waitpid(child, &status, 0);
		check_wait_status(status);
	} else {
		close(fd[0]);
		client(fd[1], prio, flags);
		exit(0);
	}

	return;
}

static
void start(const char *prio, unsigned int flags)
{
	start2(prio, prio, GNUTLS_NO_TICKETS, flags);
}

void doit(void)
{
	start("NORMAL:-VERS-ALL:+VERS-TLS1.2", 0);
	/* Under TLS 1.3 session tickets are not negotiated; they are
	 * always sent unless server sets GNUTLS_NO_TICKETS... */
	start("NORMAL:-VERS-ALL:+VERS-TLS1.3", GNUTLS_NO_TICKETS);
	/* ...or there is no overlap between PSK key exchange modes */
	start2("NORMAL:-VERS-ALL:+VERS-TLS1.3:+PSK:-DHE-PSK", "NORMAL:-VERS-ALL:+VERS-TLS1.3", 0, 0);
	start("NORMAL", GNUTLS_NO_TICKETS);
}

#endif				/* _WIN32 */
