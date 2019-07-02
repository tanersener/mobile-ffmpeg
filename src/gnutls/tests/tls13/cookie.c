/*
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

/* This program tests whether a cookie sent by the server is repeated
 * by the gnutls client. */

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
#include <assert.h>

#include "utils.h"
#include "cert-common.h"

const char *side = "";

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "%s|<%d>| %s", side, level, str);
}

static int TLSEXT_TYPE_server_sent		= 0;
static int TLSEXT_TYPE_server_received		= 0;

static const unsigned char ext_data[] =
{
	0x00,
	0x03,
	0xFE,
	0xED,
	0xFF
};

static int ext_recv_server_cookie(gnutls_session_t session, const unsigned char *buf, size_t buflen)
{
	if (buflen != sizeof(ext_data))
		fail("ext_recv_server_params: Invalid input buffer length\n");

	if (memcmp(buf, ext_data, sizeof(ext_data)) != 0)
		fail("ext_recv_server_params: Invalid input buffer data\n");

	TLSEXT_TYPE_server_received = 1;

	return 0; //Success
}

static int ext_send_server_cookie(gnutls_session_t session, gnutls_buffer_t extdata)
{
	if (gnutls_ext_get_current_msg(session) == GNUTLS_EXT_FLAG_HRR) {
		TLSEXT_TYPE_server_sent = 1;

		gnutls_buffer_append_data(extdata, ext_data, sizeof(ext_data));
		return sizeof(ext_data);
	}
	return 0;
}

static void client(int sd)
{
	int ret;
	gnutls_session_t session;
	gnutls_certificate_credentials_t clientx509cred;

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
	assert(gnutls_priority_set_direct(session, "NORMAL:-VERS-ALL:+VERS-TLS1.3",
				   NULL)>=0);

	/* put the anonymous credentials to the current session
	 */
	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE,
				clientx509cred);

	gnutls_transport_set_int(session, sd);
	gnutls_handshake_set_timeout(session, 20 * 1000);

	/* Perform the TLS handshake
	 */
	ret = gnutls_handshake(session);

	if (ret < 0) {
		fail("client: Handshake failed: %s\n", gnutls_strerror(ret));
		goto end;
	} else {
		if (debug)
			success("client: Handshake was completed\n");
	}

	gnutls_bye(session, GNUTLS_SHUT_WR);

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


	/* force a hello retry request by disabling all the groups that are
	 * enabled by default. */
	assert(gnutls_priority_set_direct(session,
					  "NORMAL:-VERS-ALL:+VERS-TLS1.3:"
					  "-GROUP-SECP256R1:-GROUP-X25519:-GROUP-FFDHE2048",
					  NULL)>=0);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE,
				serverx509cred);

	ret = gnutls_session_ext_register(session, "cookie_server", 44, GNUTLS_EXT_TLS, ext_recv_server_cookie, ext_send_server_cookie,
					  NULL, NULL, NULL,
					  GNUTLS_EXT_FLAG_CLIENT_HELLO|GNUTLS_EXT_FLAG_HRR|GNUTLS_EXT_FLAG_OVERRIDE_INTERNAL|GNUTLS_EXT_FLAG_IGNORE_CLIENT_REQUEST);
	if (ret != 0)
		fail("server: cannot register: %s", gnutls_strerror(ret));

	gnutls_transport_set_int(session, sd);
	gnutls_handshake_set_timeout(session, 20 * 1000);

	ret = gnutls_handshake(session);
	if (ret < 0) {
		fail("server: Handshake has failed: %s\n\n",
		     gnutls_strerror(ret));
		goto end;
	}
	if (debug)
		success("server: Handshake was completed\n");

	if (TLSEXT_TYPE_server_sent != 1)
		fail("server: extension not properly sent\n");

	if (TLSEXT_TYPE_server_received != 1)
		fail("server: extension not properly received\n");

	/* do not wait for the peer to close the connection.
	 */
	gnutls_bye(session, GNUTLS_SHUT_WR);

end:
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

	TLSEXT_TYPE_server_sent	= 0;
	TLSEXT_TYPE_server_received = 0;

	child = fork();
	if (child < 0) {
		perror("fork");
		fail("fork");
		return;
	}

	if (child) {
		int status = 0;
		/* parent */
		close(sockets[1]);
		server(sockets[0]);
		wait(&status);
	} else {
		close(sockets[0]);
		client(sockets[1]);
		exit(0);
	}
}

#endif				/* _WIN32 */
