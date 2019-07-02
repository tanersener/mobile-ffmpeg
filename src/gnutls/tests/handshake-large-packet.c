/*
 * Copyright (C) 2016 Red Hat, Inc.
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

/* This test checks whether a large handshake packet is accepted by client
 * and by server. (large is around 64kb)
 */
const char *side = "";

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "%s|<%d>| %s", side, level, str);
}

#define TLSEXT_TYPE1 0xFA
#define TLSEXT_TYPE2 0xFB
#define TLSEXT_TYPE3 0xFC
#define TLSEXT_TYPE4 0xFD
#define TLSEXT_TYPE5 0xFE

static int TLSEXT_TYPE_server_sent  = 0;
static int TLSEXT_TYPE_client_received = 0;

#define MAX_SIZE (12*1024)

static int ext_recv_client_params(gnutls_session_t session, const unsigned char *buf, size_t buflen)
{
	if (buflen != MAX_SIZE)
		fail("ext_recv_client_params: Invalid input buffer length\n");

	TLSEXT_TYPE_client_received++;

	return 0; //Success
}

static int ext_send_client_params(gnutls_session_t session, gnutls_buffer_t extdata)
{
	unsigned char *data = gnutls_calloc(1, MAX_SIZE);
	if (data == NULL)
		return -1;

	gnutls_buffer_append_data(extdata, data, MAX_SIZE);
	gnutls_free(data);
	return MAX_SIZE;
}

static int ext_recv_server_params(gnutls_session_t session, const unsigned char *buf, size_t buflen)
{
	return 0; //Success
}

static int ext_send_server_params(gnutls_session_t session, gnutls_buffer_t extdata)
{
	unsigned char *data = gnutls_calloc(1, MAX_SIZE);
	if (data == NULL)
		return -1;

	TLSEXT_TYPE_server_sent++;
	gnutls_buffer_append_data(extdata, data, MAX_SIZE);
	gnutls_free(data);
	return MAX_SIZE;
}

static void client(int sd, const char *prio)
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
	assert(gnutls_priority_set_direct(session, prio, NULL) >= 0);

	/* put the anonymous credentials to the current session
	 */
	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE,
				clientx509cred);

	gnutls_transport_set_int(session, sd);
	gnutls_handshake_set_timeout(session, 20 * 1000);

	gnutls_session_ext_register(session, "ext_client1", TLSEXT_TYPE1, GNUTLS_EXT_TLS, ext_recv_client_params, ext_send_client_params, NULL, NULL, NULL, 0);
	gnutls_session_ext_register(session, "ext_client2", TLSEXT_TYPE2, GNUTLS_EXT_TLS, ext_recv_client_params, ext_send_client_params, NULL, NULL, NULL, 0);
	gnutls_session_ext_register(session, "ext_client3", TLSEXT_TYPE3, GNUTLS_EXT_TLS, ext_recv_client_params, ext_send_client_params, NULL, NULL, NULL, 0);
	gnutls_session_ext_register(session, "ext_client4", TLSEXT_TYPE4, GNUTLS_EXT_TLS, ext_recv_client_params, ext_send_client_params, NULL, NULL, NULL, 0);
	gnutls_session_ext_register(session, "ext_client5", TLSEXT_TYPE5, GNUTLS_EXT_TLS, ext_recv_client_params, ext_send_client_params, NULL, NULL, NULL, 0);

	/* Perform the TLS handshake
	 */
	do {
		ret = gnutls_handshake(session);
	} while(ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED);

	if (ret < 0) {
		fail("client: Handshake failed\n");
		gnutls_perror(ret);
		goto end;
	} else {
		if (debug)
			success("client: Handshake was completed\n");
	}

	if (TLSEXT_TYPE_client_received != 5)
		fail("client: extensions were not properly sent/received\n");

	gnutls_bye(session, GNUTLS_SHUT_RDWR);

end:
	close(sd);

	gnutls_deinit(session);

	gnutls_certificate_free_credentials(clientx509cred);

	gnutls_global_deinit();
}

static void server(int sd, const char *prio)
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
	assert(gnutls_priority_set_direct(session, prio, NULL) >= 0);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE,
				serverx509cred);

	gnutls_session_ext_register(session, "ext_server1", TLSEXT_TYPE1, GNUTLS_EXT_TLS, ext_recv_server_params, ext_send_server_params, NULL, NULL, NULL, 0);
	gnutls_session_ext_register(session, "ext_server2", TLSEXT_TYPE2, GNUTLS_EXT_TLS, ext_recv_server_params, ext_send_server_params, NULL, NULL, NULL, 0);
	gnutls_session_ext_register(session, "ext_server3", TLSEXT_TYPE3, GNUTLS_EXT_TLS, ext_recv_server_params, ext_send_server_params, NULL, NULL, NULL, 0);
	gnutls_session_ext_register(session, "ext_server4", TLSEXT_TYPE4, GNUTLS_EXT_TLS, ext_recv_server_params, ext_send_server_params, NULL, NULL, NULL, 0);
	gnutls_session_ext_register(session, "ext_server5", TLSEXT_TYPE5, GNUTLS_EXT_TLS, ext_recv_server_params, ext_send_server_params, NULL, NULL, NULL, 0);

	gnutls_transport_set_int(session, sd);
	gnutls_handshake_set_timeout(session, 20 * 1000);

	do {
		ret = gnutls_handshake(session);
	} while(ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED);

	if (ret < 0) {
		close(sd);
		gnutls_deinit(session);
		fail("server: Handshake has failed (%s)\n\n",
		     gnutls_strerror(ret));
		return;
	}
	if (debug)
		success("server: Handshake was completed\n");

	if (TLSEXT_TYPE_server_sent != 5)
		fail("server: extensions were not properly sent\n");

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

static
void start(const char *prio)
{
	pid_t child;
	int sockets[2];
	int err;

	signal(SIGPIPE, SIG_IGN);
	TLSEXT_TYPE_server_sent  = 0;
	TLSEXT_TYPE_client_received = 0;

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
		close(sockets[0]);
		client(sockets[1], prio);
		wait(&status);
		check_wait_status(status);
	} else {
		close(sockets[1]);
		server(sockets[0], prio);
		exit(0);
	}
}

void doit(void)
{
	start("NORMAL:-VERS-ALL:+VERS-TLS1.1");
	start("NORMAL:-VERS-ALL:+VERS-TLS1.2");
	start("NORMAL:-VERS-ALL:+VERS-TLS1.3");
	start("NORMAL");
}

#endif				/* _WIN32 */
