/*
 * Copyright (C) 2017-2018 Red Hat, Inc.
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

#include "cert-common.h"
#include "utils.h"

/* This program tests the ProtocolVersion of Client Hello
 * and whether the supported_versions extension is present and
 * contains 0x0304 (TLS 1.3).
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

static void client(int fd)
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

	/* Initialize TLS session
	 */
	gnutls_init(&session, GNUTLS_CLIENT);

	gnutls_handshake_set_timeout(session, 20 * 1000);

	ret = gnutls_priority_set_direct(session, "NORMAL:-VERS-ALL:+VERS-TLS1.3:+VERS-TLS1.2:+VERS-TLS1.0", NULL);
	if (ret < 0)
		fail("cannot set TLS 1.3 priorities\n");

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

	close(fd);

	gnutls_deinit(session);

	gnutls_certificate_free_credentials(x509_cred);

	gnutls_global_deinit();
}

static unsigned client_hello_ok = 0;
static unsigned server_hello_ok = 0;

#define HANDSHAKE_SESSION_ID_POS 34
#define TLS_EXT_SUPPORTED_VERSIONS 43

#define SKIP16(pos, total) { \
	uint16_t _s; \
	if (pos+2 > total) fail("error\n"); \
	_s = (msg->data[pos] << 8) | msg->data[pos+1]; \
	if ((size_t)(pos+2+_s) > total) fail("error\n"); \
	pos += 2+_s; \
	}

#define SKIP8(pos, total) { \
	uint8_t _s; \
	if (pos+1 > total) fail("error\n"); \
	_s = msg->data[pos]; \
	if ((size_t)(pos+1+_s) > total) fail("error\n"); \
	pos += 1+_s; \
	}

static int client_hello_callback(gnutls_session_t session, unsigned int htype,
	unsigned post, unsigned int incoming, const gnutls_datum_t *msg)
{
	ssize_t pos;

	if (htype == GNUTLS_HANDSHAKE_SERVER_HELLO && post == GNUTLS_HOOK_POST) {
		/* check whether TLS 1.3 is negotiated */
		pos = 0;
		if (msg->size < 2) {
			fail("error in server hello size\n");
		}

		success("server hello:\n\t%d.%d\n",
			(int)msg->data[pos], (int)msg->data[pos+1]);

		if (msg->data[pos] != 0x03 || msg->data[pos+1] != 0x03) {
			fail("fail expected TLS 1.2 in server hello, got %d.%d\n", (int)msg->data[pos], (int)msg->data[pos+1]);
		}

		server_hello_ok = 1;
		return GNUTLS_E_INTERRUPTED;
	}

	if (htype != GNUTLS_HANDSHAKE_CLIENT_HELLO || post != GNUTLS_HOOK_PRE)
		return 0;

	if (msg->size < HANDSHAKE_SESSION_ID_POS)
		return -1;

	/* we expect the legacy version to be present */
	/* ProtocolVersion legacy_version = 0x0303 */
	if (msg->data[0] != 0x03 || msg->data[1] != 0x03) {
		fail("ProtocolVersion contains %d.%d\n", (int)msg->data[0], (int)msg->data[1]);
	}

	pos = HANDSHAKE_SESSION_ID_POS;
	/* legacy_session_id */
	SKIP8(pos, msg->size);

	/* CipherSuites */
	SKIP16(pos, msg->size);

	/* legacy_compression_methods */
	SKIP8(pos, msg->size);

	pos += 2;

	while (pos < msg->size) {
		uint16_t type;

		if (pos+4 > msg->size)
			fail("invalid client hello\n");

		type = (msg->data[pos] << 8) | msg->data[pos+1];
		pos+=2;

		success("Found extension %d\n", (int)type);

		if (type != TLS_EXT_SUPPORTED_VERSIONS) {
			SKIP16(pos, msg->size);
		} else { /* found */
			ssize_t size = (msg->data[pos] << 8) | msg->data[pos+1];
			pos+=2;

			size = msg->data[pos];

			if (size > msg->size+pos) {
				fail("error in extension length\n");
			}

			if (size % 2 == 1) {
				fail("extension length is odd!\n");
			}

			if (size != 6) {
				fail("expected three versions only (%d)!\n", (int)size);
			}
			pos++;

			success("client hello:\n\t%d.%d\n\t%d.%d\n\t%d.%d\n",
				(int)msg->data[pos], (int)msg->data[pos+1], 
				(int)msg->data[pos+2], (int)msg->data[pos+3], 
				(int)msg->data[pos+4], (int)msg->data[pos+5]);

			if (msg->data[pos] != 0x03 || msg->data[pos+1] != 0x04) {
				fail("fail expected TLS 1.3, got %d.%d\n", (int)msg->data[pos], (int)msg->data[pos+1]);
			}
			pos+=2;

			if (msg->data[pos] != 0x03 || msg->data[pos+1] != 0x03) {
				fail("fail expected TLS 1.2, got %d.%d\n", (int)msg->data[pos], (int)msg->data[pos+1]);
			}
			pos+=2;

			if (msg->data[pos] != 0x03 || msg->data[pos+1] != 0x01) {
				fail("fail expected TLS 1.0, got %d.%d\n", (int)msg->data[pos], (int)msg->data[pos+1]);
			}
			pos+=2;
			client_hello_ok = 1;
			break;
		}
	}

	return 0;
}

static void server(int fd)
{
	int ret;
	char buffer[MAX_BUF + 1];
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

	gnutls_handshake_set_timeout(session, 20 * 1000);
	gnutls_handshake_set_hook_function(session, GNUTLS_HANDSHAKE_ANY,
					   GNUTLS_HOOK_BOTH,
					   client_hello_callback);

	/* avoid calling all the priority functions, since the defaults
	 * are adequate.
	 */
	gnutls_priority_set_direct(session, "NORMAL:+VERS-TLS1.3", NULL);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, x509_cred);

	gnutls_transport_set_int(session, fd);

	do {
		ret = gnutls_handshake(session);
		if (ret == GNUTLS_E_INTERRUPTED) { /* expected */
			break;
		}
	} while (ret < 0 && gnutls_error_is_fatal(ret) == 0);


	if (client_hello_ok == 0) {
		fail("server: did not verify the client hello\n");
	}

	if (server_hello_ok == 0) {
		fail("server: did not verify the server hello contents\n");
	}

	close(fd);
	gnutls_deinit(session);

	gnutls_certificate_free_credentials(x509_cred);

	gnutls_global_deinit();

	if (debug)
		success("server: client/server hello were verified\n");
}

static void ch_handler(int sig)
{
	int status = 0;
	wait(&status);
	check_wait_status(status);
	return;
}

void doit(void)
{
	int fd[2];
	int ret;
	pid_t child;

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
		server(fd[0]);
		kill(child, SIGTERM);
	} else {
		close(fd[0]);
		client(fd[1]);
		exit(0);
	}
}

#endif				/* _WIN32 */
