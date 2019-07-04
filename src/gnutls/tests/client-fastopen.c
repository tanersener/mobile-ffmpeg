/*
 * Copyright (C) 2016-2018 Red Hat, Inc.
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

/* This tests gnutls_transport_set_fastopen() operation.
 */

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
#include <gnutls/socket.h>
#include <errno.h>
#include <assert.h>
#include "cert-common.h"
#include "utils.h"

static void terminate(void);

static void server_log_func(int level, const char *str)
{
	fprintf(stderr, "server|<%d>| %s", level, str);
}

static void client_log_func(int level, const char *str)
{
	fprintf(stderr, "client|<%d>| %s", level, str);
}

#define MAX_BUF 1024

static void client(int fd, struct sockaddr *connect_addr, socklen_t connect_addrlen,
		   const char *prio)
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
	gnutls_init(&session, GNUTLS_CLIENT);
	gnutls_handshake_set_timeout(session, 20 * 1000);

	/* Use default priorities */
	assert(gnutls_priority_set_direct(session, prio, NULL) >= 0);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);

	gnutls_transport_set_fastopen(session, fd, connect_addr, connect_addrlen, 0);

	/* Perform the TLS handshake
	 */
	do {
		ret = gnutls_handshake(session);
	}
	while (ret < 0 && gnutls_error_is_fatal(ret) == 0);

	if (ret < 0) {
		fail("client: Handshake failed: %s\n", gnutls_strerror(ret));
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

	ret = gnutls_bye(session, GNUTLS_SHUT_RDWR);
	if (ret < 0) {
		fail("server: error in closing session: %s\n", gnutls_strerror(ret));
	}

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
	kill(child, SIGTERM);
	exit(1);
}

static void server(int fd, const char *prio)
{
	int ret;
	gnutls_certificate_credentials_t xcred;
	char buffer[MAX_BUF + 1];
	gnutls_session_t session;

	/* this must be called once in the program
	 */
	global_init();

	if (debug) {
		gnutls_global_set_log_function(server_log_func);
		gnutls_global_set_log_level(4711);
	}

	gnutls_certificate_allocate_credentials(&xcred);

	ret = gnutls_certificate_set_x509_key_mem(xcred,
					    &server_cert, &server_key,
					    GNUTLS_X509_FMT_PEM);
	if (ret < 0)
		exit(1);

	gnutls_init(&session, GNUTLS_SERVER);
	gnutls_handshake_set_timeout(session, 20 * 1000);

	/* avoid calling all the priority functions, since the defaults
	 * are adequate.
	 */
	assert(gnutls_priority_set_direct(session, prio, NULL) >= 0);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);

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

	ret = gnutls_bye(session, GNUTLS_SHUT_RDWR);
	if (ret < 0) {
		fail("server: error in closing session: %s\n", gnutls_strerror(ret));
	}

	close(fd);
	gnutls_deinit(session);

	gnutls_certificate_free_credentials(xcred);

	gnutls_global_deinit();

	if (debug)
		success("server: finished\n");
}

static void ch_handler(int sig)
{
	return;
}

static
void run(const char *name, const char *prio)
{
	int ret;
	struct sockaddr_in saddr;
	socklen_t addrlen;
	int listener;
	int fd;

	success("running fast open test for %s\n", name);

	signal(SIGCHLD, ch_handler);
	signal(SIGPIPE, SIG_IGN);

	listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listener == -1)
		fail("error in socket(): %s\n", strerror(errno));

	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	saddr.sin_port = 0;

	ret = bind(listener, (struct sockaddr*)&saddr, sizeof(saddr));
	if (ret == -1)
		fail("error in bind(): %s\n", strerror(errno));

	addrlen = sizeof(saddr);
	ret = getsockname(listener, (struct sockaddr*)&saddr, &addrlen);
	if (ret == -1)
		fail("error in getsockname(): %s\n", strerror(errno));

	child = fork();
	if (child < 0) {
		perror("fork");
		fail("fork");
		exit(1);
	}

	if (child) {
		int status;
		/* parent */

		ret = listen(listener, 1);
		if (ret == -1)
			fail("error in listen(): %s\n", strerror(errno));

		fd = accept(listener, NULL, NULL);
		if (fd == -1)
			fail("error in accept: %s\n", strerror(errno));

		server(fd, prio);

		wait(&status);
		check_wait_status(status);
	} else {
		fd = socket(AF_INET, SOCK_STREAM, 0);

		usleep(1000000);
		client(fd, (struct sockaddr*)&saddr, addrlen, prio);
		exit(0);
	}
}

void doit(void)
{
	run("tls1.2", "NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+ECDHE-RSA");
	run("tls1.3", "NORMAL:-VERS-ALL:+VERS-TLS1.3");
}

#endif				/* _WIN32 */
