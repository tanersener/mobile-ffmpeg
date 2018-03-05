/*
 * Copyright (C) 2004-2016 Free Software Foundation, Inc.
 *
 * Author: Thierry Quemerais, Nikos Mavrogiannopoulos
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

/* This program tests whether an extension is exchanged when registered
 * at the session level */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#if defined(_WIN32)

/* socketpair isn't supported on Win32. */
int main(int argc, char **argv)
{
	exit(77);
}

#else

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#if !defined(_WIN32)
#include <sys/wait.h>
#include <signal.h>
#endif
#include <unistd.h>
#include <gnutls/gnutls.h>

#include "utils.h"
#include "cert-common.h"

const char *side = "";

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "%s|<%d>| %s", side, level, str);
}

#define TLSEXT_TYPE_SAMPLE 0xF1
#define TLSEXT_TYPE_IGN 0xF2

static int TLSEXT_TYPE_client_sent		= 0;
static int TLSEXT_TYPE_client_received		= 0;
static int TLSEXT_TYPE_server_sent		= 0;
static int TLSEXT_TYPE_server_received		= 0;
static int ign_extension_called = 0;

static const unsigned char ext_data[] =
{	
	0xFE,
	0xED
};

static int ext_recv_client_params(gnutls_session_t session, const unsigned char *buf, size_t buflen)
{
	if (buflen != sizeof(ext_data))
		fail("ext_recv_client_params: Invalid input buffer length\n");

	if (memcmp(buf, ext_data, sizeof(ext_data)) != 0)
		fail("ext_recv_client_params: Invalid input buffer data\n");

	TLSEXT_TYPE_client_received = 1;

	gnutls_ext_set_data(session, TLSEXT_TYPE_SAMPLE, session);

	return 0; //Success
}

static int ext_send_client_params(gnutls_session_t session, gnutls_buffer_t extdata)
{
	TLSEXT_TYPE_client_sent = 1;
	gnutls_buffer_append_data(extdata, ext_data, sizeof(ext_data));
	return sizeof(ext_data);
}

static int ext_recv_client_ign_params(gnutls_session_t session, const unsigned char *buf, size_t buflen)
{
	return 0;
}

static int ext_send_client_ign_params(gnutls_session_t session, gnutls_buffer_t extdata)
{
	ign_extension_called = 1;
	return 0;
}

static int ext_recv_server_params(gnutls_session_t session, const unsigned char *buf, size_t buflen)
{
	if (buflen != sizeof(ext_data))
		fail("ext_recv_server_params: Invalid input buffer length\n");

	if (memcmp(buf, ext_data, sizeof(ext_data)) != 0)
		fail("ext_recv_server_params: Invalid input buffer data\n");

	TLSEXT_TYPE_server_received = 1;

	return 0; //Success
}

static int ext_send_server_params(gnutls_session_t session, gnutls_buffer_t extdata)
{
	TLSEXT_TYPE_server_sent = 1;
	gnutls_buffer_append_data(extdata, ext_data, sizeof(ext_data));
	return sizeof(ext_data);
}

static void client(int sd)
{
	int ret;
	gnutls_session_t session;
	gnutls_certificate_credentials_t clientx509cred;
	void *p;

	global_init();
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(4711);

	side = "client";

	gnutls_certificate_allocate_credentials(&clientx509cred);

	/* Initialize TLS session
	 */
	gnutls_init(&session, GNUTLS_CLIENT);

	/* Use default priorities */
	gnutls_priority_set_direct(session, "PERFORMANCE:+ANON-ECDH:+ANON-DH",
				   NULL);

	/* put the anonymous credentials to the current session
	 */
	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE,
				clientx509cred);

	gnutls_transport_set_int(session, sd);
	gnutls_handshake_set_timeout(session, 20 * 1000);

	ret = gnutls_session_ext_register(session, "ext_ign", TLSEXT_TYPE_IGN, GNUTLS_EXT_TLS, ext_recv_client_ign_params, ext_send_client_ign_params, NULL, NULL, NULL, 0);
	if (ret < 0)
		fail("client: register extension\n");

	ret = gnutls_session_ext_register(session, "ext_client", TLSEXT_TYPE_SAMPLE, GNUTLS_EXT_TLS, ext_recv_client_params, ext_send_client_params, NULL, NULL, NULL, 0);
	if (ret < 0)
		fail("client: register extension\n");

	/* Perform the TLS handshake
	 */
	ret = gnutls_handshake(session);

	if (ret < 0) {
		fail("client: Handshake failed\n");
		gnutls_perror(ret);
		goto end;
	} else {
		if (debug)
			success("client: Handshake was completed\n");
	}

	if (TLSEXT_TYPE_client_sent != 1 || TLSEXT_TYPE_client_received != 1)
		fail("client: extension not properly sent/received\n");

	ret = gnutls_ext_get_data(session, TLSEXT_TYPE_SAMPLE, &p);
	if (ret < 0) {
		fail("gnutls_ext_get_data: %s\n", gnutls_strerror(ret));
	}

	if (p != session) {
		fail("client: gnutls_ext_get_data failed\n");
	}

	if (ign_extension_called == 0) {
		fail("registered ign extension was not called\n");
	}

	gnutls_bye(session, GNUTLS_SHUT_RDWR);

end:
	close(sd);

	gnutls_deinit(session);

	gnutls_certificate_free_credentials(clientx509cred);

	gnutls_global_deinit();
}

static void server(int sd)
{
	gnutls_certificate_credentials_t serverx509cred;
	int ret;
	gnutls_session_t session;
	unsigned i;

	/* this must be called once in the program
	 */
	global_init();
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(4711);

	side = "server";

	gnutls_certificate_allocate_credentials(&serverx509cred);
	gnutls_certificate_set_x509_key_mem(serverx509cred,
					    &server_cert, &server_key,
					    GNUTLS_X509_FMT_PEM);

	gnutls_init(&session, GNUTLS_SERVER);

	/* avoid calling all the priority functions, since the defaults
	 * are adequate.
	 */
	gnutls_priority_set_direct(session, "PERFORMANCE:+ANON-ECDH:+ANON-DH",
				   NULL);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE,
				serverx509cred);

	gnutls_session_ext_register(session, "ext_server", TLSEXT_TYPE_SAMPLE, GNUTLS_EXT_TLS, ext_recv_server_params, ext_send_server_params, NULL, NULL, NULL, 0);

	gnutls_transport_set_int(session, sd);
	gnutls_handshake_set_timeout(session, 20 * 1000);

	ret = gnutls_handshake(session);
	if (ret < 0) {
		close(sd);
		gnutls_deinit(session);
		fail("server: Handshake has failed (%s)\n\n",
		     gnutls_strerror(ret));
		return;
	}
	if (debug)
		success("server: Handshake was completed\n");

	if (TLSEXT_TYPE_server_sent != 1 || TLSEXT_TYPE_server_received != 1)
		fail("server: extension not properly sent/received\n");

	/* do not wait for the peer to close the connection.
	 */
	gnutls_bye(session, GNUTLS_SHUT_WR);

	/* check whether we can crash the library by adding many extensions */
	for (i=0;i<64;i++) {
		ret = gnutls_ext_register("ext_serverxx", TLSEXT_TYPE_SAMPLE+i+1, GNUTLS_EXT_TLS, ext_recv_server_params, ext_send_server_params, NULL, NULL, NULL);
		if (ret < 0) {
			success("failed registering extension no %d (expected)\n", i+1);
			break;
		}
	}

	close(sd);
	gnutls_deinit(session);

	gnutls_certificate_free_credentials(serverx509cred);

	gnutls_global_deinit();

	if (debug)
		success("server: finished\n");
}

void doit(void)
{
	pid_t child;
	int sockets[2];
	int err;

	signal(SIGPIPE, SIG_IGN);

	err = socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
	if (err == -1) {
		perror("socketpair");
		fail("socketpair failed\n");
		return;
	}

	child = fork();
	if (child < 0) {
		perror("fork");
		fail("fork");
		return;
	}

	if (child) {
		int status;
		/* parent */
		close(sockets[1]);
		server(sockets[0]);
		wait(&status);
	} else {
		close(sockets[0]);
		client(sockets[1]);
	}
}

#endif				/* _WIN32 */
