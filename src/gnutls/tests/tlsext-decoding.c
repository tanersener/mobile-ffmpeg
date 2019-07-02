/*
 * Copyright (C) 2015 Nikos Mavrogiannopoulos
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
#include "cert-common.h"

/* This test checks whether an invalid extensions field will lead
 * to a GNUTLS_E_UNEXPECTED_EXTENSIONS_LENGTH. */

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
#include <assert.h>
#include <gnutls/gnutls.h>

#include "utils.h"

static void terminate(void);
static unsigned reduce = 0;

/* This program tests the client hello verify in DTLS
 */

static void server_log_func(int level, const char *str)
{
	fprintf(stderr, "server|<%d>| %s", level, str);
}

static void client_log_func(int level, const char *str)
{
	fprintf(stderr, "client|<%d>| %s", level, str);
}

#define RECORD_PAYLOAD_POS 5
#define HANDSHAKE_CS_POS (39)
static ssize_t odd_push(gnutls_transport_ptr_t tr, const void *data, size_t len)
{
	uint8_t *d = (void*)data;
	int fd = (long)tr;
	uint16_t csize, osize;
	int pos;

	if (d[0] == 22 && d[5] == GNUTLS_HANDSHAKE_CLIENT_HELLO) {
		/* skip ciphersuites */
		csize = d[RECORD_PAYLOAD_POS+HANDSHAKE_CS_POS+1] + (d[RECORD_PAYLOAD_POS+HANDSHAKE_CS_POS] << 8);
		csize += 2;

		/* skip compression methods */
		osize = d[RECORD_PAYLOAD_POS+HANDSHAKE_CS_POS+csize];
		osize += 1;

		pos = RECORD_PAYLOAD_POS+HANDSHAKE_CS_POS+csize+osize;

		if (reduce) {
			if (d[pos+1] != 0x00) {
				d[pos+1] = d[pos+1] - 1;
			} else {
				d[pos] = d[pos] - 1;
				d[pos+1] = 0xff;
			}
		} else {
			if (d[pos+1] != 0xff) {
				d[pos+1] = d[pos+1] + 1;
			} else {
				d[pos] = d[pos] + 1;
				d[pos+1] = 0x00;
			}
		
		}
	}

	return send(fd, data, len, 0);
}

/* A very basic DTLS client handling DTLS 0.9 which sets premaster secret.
 */

static void client(int fd, const char *prio)
{
	int ret;
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

	assert(gnutls_priority_set_direct(session, prio, NULL) >= 0);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);

	gnutls_transport_set_int(session, fd);
	gnutls_transport_set_push_function(session, odd_push);

	/* Perform the TLS handshake
	 */
	do {
		ret = gnutls_handshake(session);
	}
	while (ret < 0 && gnutls_error_is_fatal(ret) == 0);

	if (ret >= 0) {
		fail("client: Handshake succeeded!\n");
		exit(1);
	}

	close(fd);

	gnutls_deinit(session);

	gnutls_certificate_free_credentials(xcred);

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
	gnutls_certificate_credentials_t xcred;
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

	if (ret != GNUTLS_E_UNEXPECTED_EXTENSIONS_LENGTH) {
		close(fd);
		gnutls_deinit(session);
		fail("server: Handshake did not fail with GNUTLS_E_UNEXPECTED_EXTENSIONS_LENGTH (%s)\n\n",
		     gnutls_strerror(ret));
		terminate();
	}

	close(fd);
	gnutls_deinit(session);

	gnutls_certificate_free_credentials(xcred);

	gnutls_global_deinit();

	if (debug)
		success("server: Handshake failed as expected\n");
}

static void start(const char *prio)
{
	int fd[2];
	int ret;

	success("trying %s\n", prio);
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
		int status;
		/* parent */

		close(fd[0]);
		server(fd[1], prio);

		wait(&status);
		check_wait_status(status);
		close(fd[1]);
	} else {
		close(fd[1]);
		client(fd[0], prio);
		exit(0);
	}
}

void doit(void)
{
	success("checking overflow\n");
	start("NORMAL:-VERS-ALL:+VERS-TLS1.2");
	start("NORMAL:-VERS-ALL:+VERS-TLS1.3");
	start("NORMAL");

	success("checking underflow\n");
	reduce = 1;
	start("NORMAL:-VERS-ALL:+VERS-TLS1.2");
	start("NORMAL:-VERS-ALL:+VERS-TLS1.3");
	start("NORMAL");
}
#endif				/* _WIN32 */
