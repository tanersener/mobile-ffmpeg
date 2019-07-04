/*
 * Copyright (C) 2018 Red Hat, Inc
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

/* This program is a reproducer for issue #543; the timeout
 * of DTLS handshake when a NewSessionTicket is lost.
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

static void client(int fd, const char *prio)
{
	int ret;
	gnutls_certificate_credentials_t x509_cred;
	gnutls_session_t session;

	if (debug) {
		gnutls_global_set_log_function(client_log_func);
		gnutls_global_set_log_level(6);
	}

	gnutls_certificate_allocate_credentials(&x509_cred);

	assert(gnutls_init(&session, GNUTLS_CLIENT|GNUTLS_DATAGRAM)>=0);

	assert(gnutls_priority_set_direct(session, prio, NULL)>=0);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, x509_cred);

	gnutls_transport_set_int(session, fd);

	do {
		ret = gnutls_handshake(session);
	}
	while (ret < 0 && gnutls_error_is_fatal(ret) == 0);

	if (ret < 0) {
		fail("client: Handshake failed: %s\n", gnutls_strerror(ret));
	} else {
		if (debug)
			success("client: Handshake was completed\n");
	}

	gnutls_record_set_timeout(session, 30*1000);

	do {
		ret = gnutls_bye(session, GNUTLS_SHUT_WR);
	} while(ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED);

	close(fd);

	gnutls_deinit(session);

	gnutls_certificate_free_credentials(x509_cred);
}


static ssize_t
server_push(gnutls_transport_ptr_t tr, const void *data, size_t len)
{
	const uint8_t *d = data;
	static int dropped = 0;

	if (d[13] == GNUTLS_HANDSHAKE_NEW_SESSION_TICKET) {
		if (dropped == 0) {
			success("dropping message: %s\n", gnutls_handshake_description_get_name(d[13]));
			dropped = 1;
			return len;
		}
	}

	return send((long)tr, data, len, 0);
}

static void server(int fd, const char *prio)
{
	int ret;
	char buffer[MAX_BUF + 1];
	gnutls_session_t session;
	gnutls_certificate_credentials_t x509_cred;
	gnutls_datum_t skey;

	memset(buffer, 0, sizeof(buffer));

	if (debug) {
		gnutls_global_set_log_function(server_log_func);
		gnutls_global_set_log_level(6);
	}

	assert(gnutls_certificate_allocate_credentials(&x509_cred)>=0);
	assert(gnutls_certificate_set_x509_key_mem(x509_cred, &server_cert,
					    &server_key,
					    GNUTLS_X509_FMT_PEM)>=0);

	assert(gnutls_init(&session, GNUTLS_SERVER|GNUTLS_DATAGRAM)>=0);

	assert(gnutls_session_ticket_key_generate(&skey)>=0);
	assert(gnutls_session_ticket_enable_server(session, &skey) >= 0);

	gnutls_transport_set_push_function(session, server_push);

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

	gnutls_record_set_timeout(session, 30*1000);

	success("waiting for EOF\n");
	do {
		ret = gnutls_record_recv(session, buffer, sizeof(buffer));
	} while(ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED);
	if (ret != 0)
		fail("error waiting for EOF: %s\n", gnutls_strerror(ret));

 end:
	close(fd);
	gnutls_deinit(session);
	gnutls_free(skey.data);

	gnutls_certificate_free_credentials(x509_cred);

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
	pid_t child;

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
		client(fd[0], prio);
		waitpid(child, &status, 0);
		check_wait_status(status);
	} else {
		close(fd[0]);
		server(fd[1], prio);
		exit(0);
	}

	return;
}

void doit(void)
{
	start("NORMAL:-VERS-ALL:+VERS-DTLS1.2");
}

#endif				/* _WIN32 */
