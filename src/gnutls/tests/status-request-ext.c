/*
 * Copyright (C) 2016 Nikos Mavrogiannopoulos
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
#include "cert-common.h"
#include "utils.h"

static void terminate(void);

/* This program tests that the server does not send the
 * status request extension if no status response exists. That
 * is to provide compatibility with gnutls 3.3.x which requires
 * that behavior.
 */

static void server_log_func(int level, const char *str)
{
	fprintf(stderr, "server|<%d>| %s", level, str);
}

static void client_log_func(int level, const char *str)
{
	fprintf(stderr, "client|<%d>| %s", level, str);
}

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

#define TLS_EXT_STATUS_REQUEST 5
#define HANDSHAKE_SESSION_ID_POS 34

/* This returns either the application-specific ID extension contents,
 * or the session ID contents. The former is used on the new protocol,
 * while the latter on the legacy protocol.
 *
 * Extension ID: 48018
 * opaque ApplicationID<1..2^8-1>;
 *
 * struct {
 *          ExtensionType extension_type;
 *          opaque extension_data<0..2^16-1>;
 *      } Extension;
 *
 *      struct {
 *          ProtocolVersion server_version;
 *          Random random;
 *          SessionID session_id;
 *          CipherSuite cipher_suite;
 *          CompressionMethod compression_method;
 *          Extension server_hello_extension_list<0..2^16-1>;
 *      } ServerHello;
 */
static int handshake_callback(gnutls_session_t session, unsigned int htype,
	unsigned post, unsigned int incoming, const gnutls_datum_t *msg)
{
	size_t pos = 0;
	/* A client hello packet. We can get the session ID and figure
	 * the associated connection. */
	if (msg->size < HANDSHAKE_SESSION_ID_POS+GNUTLS_MAX_SESSION_ID+2) {
		fail("invalid client hello\n");
	}

	/* try to read the extension data */
	pos = HANDSHAKE_SESSION_ID_POS;

	/* session id */
	SKIP8(pos, msg->size);

	/* CipherSuite */
	pos+=2;

	/* CompressionMethod */
	SKIP8(pos, msg->size);

	if (pos+2 > msg->size)
		fail("invalid client hello\n");
	pos+=2;

	/* Extension(s) */
	while (pos < msg->size) {
		uint16_t type;

		if (pos+4 > msg->size)
			fail("invalid client hello\n");

		type = (msg->data[pos] << 8) | msg->data[pos+1];
		pos+=2;
		if (type != TLS_EXT_STATUS_REQUEST) {
			SKIP16(pos, msg->size);
		} else { /* found */
			fail("found extension, although no status response\n");
			break;
		}
	}

	return 0;
}

	

#define MAX_BUF 1024

static void client(int fd)
{
	int ret;
	gnutls_certificate_credentials_t x509_cred;
	gnutls_session_t session;
	/* Need to enable anonymous KX specifically. */

	global_init();

	if (debug) {
		gnutls_global_set_log_function(client_log_func);
		gnutls_global_set_log_level(7);
	}

	gnutls_certificate_allocate_credentials(&x509_cred);

	/* Initialize TLS session
	 */
	gnutls_init(&session, GNUTLS_CLIENT);

	/* Use default priorities */
	gnutls_priority_set_direct(session, "NORMAL:-KX-ALL:+ECDHE-RSA", NULL);

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

	gnutls_handshake_set_hook_function(session, GNUTLS_HANDSHAKE_SERVER_HELLO,
					   GNUTLS_HOOK_POST,
					   handshake_callback);

	/* avoid calling all the priority functions, since the defaults
	 * are adequate.
	 */
	gnutls_priority_set_direct(session, "NORMAL", NULL);

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

	/* do not wait for the peer to close the connection.
	 */
	gnutls_bye(session, GNUTLS_SHUT_WR);

 end:
	close(fd);
	gnutls_deinit(session);

	gnutls_certificate_free_credentials(x509_cred);

	gnutls_global_deinit();

	if (debug)
		success("server: finished\n");
}

static void ch_handler(int sig)
{
	return;
}

void doit(void)
{
	int fd[2];
	int ret, status = 0;

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
		waitpid(child, &status, 0);
		check_wait_status(status);
	} else {
		close(fd[0]);
		client(fd[1]);
		exit(0);
	}
}

#endif				/* _WIN32 */
