/*
 * Copyright (C) 2016 Red Hat, Inc
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

#include "cert-common.h"
#include "utils.h"

static void terminate(void);

/* This program tests that handshakes include a session ticket.
 */

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

	if (debug)
		success("sent session ticket\n");
	sent = 1;
	return 0;
}
	
#define MAX_BUF 1024

static void client(int fd, const char *prio)
{
	int ret;
	gnutls_certificate_credentials_t x509_cred;
	gnutls_session_t session;

	global_init();

	if (debug) {
		gnutls_global_set_log_function(client_log_func);
		gnutls_global_set_log_level(7);
	}

	gnutls_certificate_allocate_credentials(&x509_cred);

	assert(gnutls_init(&session, GNUTLS_CLIENT)>=0);

	assert(gnutls_priority_set_direct(session, prio, NULL)>=0);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, x509_cred);

	gnutls_transport_set_int(session, fd);

	do {
		ret = gnutls_handshake(session);
	}
	while (ret < 0 && gnutls_error_is_fatal(ret) == 0);

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

static void server(int fd, const char *prio)
{
	int ret;
	char buffer[MAX_BUF + 1];
	gnutls_session_t session;
	gnutls_certificate_credentials_t x509_cred;
	gnutls_datum_t skey;

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

	assert(gnutls_init(&session, GNUTLS_SERVER)>=0);

	assert(gnutls_session_ticket_key_generate(&skey)>=0);
	assert(gnutls_session_ticket_enable_server(session, &skey) >= 0);

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

	if (sent == 0) {
		fail("client: didn't send new sessiont ticket\n");
		terminate();
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
void start(const char *prio)
{
	int fd[2];
	int ret, status = 0;

	sent = 0;
	success("trying %s\n", prio);
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
		server(fd[0], prio);
		waitpid(child, &status, 0);
		check_wait_status(status);
	} else {
		close(fd[0]);
		client(fd[1], prio);
		exit(0);
	}

	return;
}

void doit(void)
{
	start("NORMAL:-VERS-ALL:+VERS-TLS1.2");
	start("NORMAL:-VERS-ALL:+VERS-TLS1.3");
	start("NORMAL");
}

#endif				/* _WIN32 */
