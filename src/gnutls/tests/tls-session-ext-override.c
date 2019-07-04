/*
 * Copyright (C) 2004-2016 Free Software Foundation, Inc.
 * Copyright (C) 2017 Red Hat, Inc.
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
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>
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

static int TLSEXT_TYPE_client_sent		= 0;
static int TLSEXT_TYPE_client_received		= 0;
static int TLSEXT_TYPE_server_sent		= 0;
static int TLSEXT_TYPE_server_received		= 0;
static int overridden_extension = -1;

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

	gnutls_ext_set_data(session, overridden_extension, session);

	return 0; //Success
}

static int ext_send_client_params(gnutls_session_t session, gnutls_buffer_t extdata)
{
	TLSEXT_TYPE_client_sent = 1;
	gnutls_buffer_append_data(extdata, ext_data, sizeof(ext_data));
	return sizeof(ext_data);
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

	ret = gnutls_session_ext_register(session, "ext_client", overridden_extension, GNUTLS_EXT_TLS, ext_recv_client_params, ext_send_client_params, NULL, NULL, NULL, 0);
	if (ret != GNUTLS_E_ALREADY_REGISTERED)
		fail("client: register existing extension (%d)\n", overridden_extension);

	ret = gnutls_session_ext_register(session, "ext_client", 0, GNUTLS_EXT_TLS, ext_recv_client_params, ext_send_client_params, NULL, NULL, NULL, GNUTLS_EXT_FLAG_OVERRIDE_INTERNAL);
	if (ret != GNUTLS_E_ALREADY_REGISTERED)
		fail("client: register extension %d\n", 0);

	ret = gnutls_session_ext_register(session, "ext_client", overridden_extension, GNUTLS_EXT_TLS, ext_recv_client_params, ext_send_client_params, NULL, NULL, NULL, GNUTLS_EXT_FLAG_OVERRIDE_INTERNAL);
	if (ret < 0)
		fail("client: register extension (%d)\n", overridden_extension);

	/* Perform the TLS handshake
	 */
	ret = gnutls_handshake(session);

	if (ret < 0) {
		fail("[%d]: client: Handshake failed\n", overridden_extension);
		gnutls_perror(ret);
		goto end;
	} else {
		if (debug)
			success("client: Handshake was completed\n");
	}

	if (TLSEXT_TYPE_client_sent != 1 || TLSEXT_TYPE_client_received != 1)
		fail("client: extension not properly sent/received\n");

	ret = gnutls_ext_get_data(session, overridden_extension, &p);
	if (ret < 0) {
		fail("gnutls_ext_get_data: %s\n", gnutls_strerror(ret));
	}

	if (p != session) {
		fail("client: gnutls_ext_get_data failed\n");
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

	ret = gnutls_session_ext_register(session, "ext_server", overridden_extension, GNUTLS_EXT_TLS, ext_recv_server_params, ext_send_server_params, NULL, NULL, NULL, 0);
	if (ret != GNUTLS_E_ALREADY_REGISTERED)
		fail("client: register existing extension\n");

	ret = gnutls_session_ext_register(session, "ext_server", overridden_extension, GNUTLS_EXT_TLS, ext_recv_server_params, ext_send_server_params, NULL, NULL, NULL, GNUTLS_EXT_FLAG_OVERRIDE_INTERNAL);
	if (ret < 0)
		fail("client: register extension\n");

	gnutls_transport_set_int(session, sd);
	gnutls_handshake_set_timeout(session, 20 * 1000);

	ret = gnutls_handshake(session);
	if (ret < 0) {
		close(sd);
		gnutls_deinit(session);
		fail("[%d]: server: Handshake has failed (%s)\n\n",
		     overridden_extension, gnutls_strerror(ret));
		return;
	}
	if (debug)
		success("server: Handshake was completed\n");

	if (TLSEXT_TYPE_server_sent != 1 || TLSEXT_TYPE_server_received != 1)
		fail("server: extension not properly sent/received\n");

	/* do not wait for the peer to close the connection.
	 */
	gnutls_bye(session, GNUTLS_SHUT_WR);

	close(sd);
	gnutls_deinit(session);

	gnutls_certificate_free_credentials(serverx509cred);

	gnutls_global_deinit();

	if (debug)
		success("server: finished\n");
}

static void override_ext(unsigned extension)
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

	TLSEXT_TYPE_client_sent	= 0;
	TLSEXT_TYPE_client_received = 0;
	TLSEXT_TYPE_server_sent	= 0;
	TLSEXT_TYPE_server_received = 0;
	overridden_extension = extension;

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
		check_wait_status(status);
	} else {
		close(sockets[0]);
		client(sockets[1]);
		exit(0);
	}
}

void doit(void)
{
	override_ext(1);
	override_ext(21);
}

#endif				/* _WIN32 */
