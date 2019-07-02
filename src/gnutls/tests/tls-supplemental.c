/*
 * Copyright (C) 2004-2012 Free Software Foundation, Inc.
 *
 * Author: Thierry Quemerais
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

/* Parts copied from GnuTLS example programs. */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

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
#endif
#include <unistd.h>
#include <gnutls/gnutls.h>

#include "utils.h"
#include "cert-common.h"

/* A very basic TLS client, with supplemental data
 */

extern void _gnutls_supplemental_deinit(void);

const char *side = "";

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "%s|<%d>| %s", side, level, str);
}

#define TLS_SUPPLEMENTALDATATYPE_SAMPLE	0xBABE

static int TLS_SUPPLEMENTALDATA_client_sent     = 0;
static int TLS_SUPPLEMENTALDATA_client_received	= 0;
static int TLS_SUPPLEMENTALDATA_server_sent	= 0;
static int TLS_SUPPLEMENTALDATA_server_received	= 0;

static const unsigned char supp_data[] =
{	
	0xFE,
	0xED
};

static
int supp_client_recv_func(gnutls_session_t session, const unsigned char *buf, size_t buflen)
{
	TLS_SUPPLEMENTALDATA_client_received = 1;

	if (buflen != sizeof(supp_data))
		fail("supp_client_recv_func: Invalid input buffer len\n");

	if (memcmp(buf, supp_data, sizeof(supp_data)) != 0)
		fail("supp_client_recv_func: Invalid input buffer data\n");

	return GNUTLS_E_SUCCESS;
}

static
int supp_client_send_func(gnutls_session_t session, gnutls_buffer_t buf)
{
	TLS_SUPPLEMENTALDATA_client_sent = 1;
	gnutls_buffer_append_data(buf, supp_data, sizeof(supp_data));
	return GNUTLS_E_SUCCESS;
}

static
int supp_server_recv_func(gnutls_session_t session, const unsigned char *buf, size_t buflen)
{
	TLS_SUPPLEMENTALDATA_server_received = 1;

	if (buflen != sizeof(supp_data))
		fail("supp_server_recv_func: Invalid input buffer len\n");

	if (memcmp(buf, supp_data, sizeof(supp_data)) != 0)
		fail("supp_server_recv_func: Invalid input buffer data\n");

	return GNUTLS_E_SUCCESS;
}

static
int supp_server_send_func(gnutls_session_t session, gnutls_buffer_t buf)
{
	TLS_SUPPLEMENTALDATA_server_sent = 1;
	gnutls_buffer_append_data(buf, supp_data, sizeof(supp_data));
	return GNUTLS_E_SUCCESS;
}

static void client(int sd, const char *prio, unsigned server_only)
{
	int ret;
	gnutls_session_t session;
	gnutls_certificate_credentials_t clientx509cred;

	if (!server_only) {
		assert(gnutls_supplemental_register("supplemental_client", TLS_SUPPLEMENTALDATATYPE_SAMPLE, supp_client_recv_func, supp_client_send_func)>=0);
	}

	side = "client";

	gnutls_certificate_allocate_credentials(&clientx509cred);

	assert(gnutls_init(&session, GNUTLS_CLIENT)>=0);

	/* Use default priorities */
	gnutls_priority_set_direct(session, prio, NULL);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE,
				clientx509cred);

	gnutls_transport_set_int(session, sd);

	if (!server_only) {
		gnutls_supplemental_recv(session, 1);
		gnutls_supplemental_send(session, 1);
	}

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

	if (!server_only) {
		if (TLS_SUPPLEMENTALDATA_client_sent != 1 || TLS_SUPPLEMENTALDATA_client_received != 1)
			fail("client: extension not properly sent/received (%d.%d)\n",
			     TLS_SUPPLEMENTALDATA_client_sent, TLS_SUPPLEMENTALDATA_client_received);
	} else {
		/* we expect TLS1.2 handshake as TLS1.3 is not (yet) defined
		 * with supplemental data */
		assert(gnutls_protocol_get_version(session) == GNUTLS_TLS1_2);
	}

	gnutls_bye(session, GNUTLS_SHUT_RDWR);

end:
	close(sd);

	gnutls_deinit(session);

	gnutls_certificate_free_credentials(clientx509cred);
}

static void server(int sd, const char *prio, unsigned server_only)
{
	gnutls_certificate_credentials_t serverx509cred;
	int ret;
	gnutls_session_t session;
	static unsigned registered = 0;

	side = "server";

	if (!registered) {
		assert(gnutls_supplemental_register("supplemental_server", TLS_SUPPLEMENTALDATATYPE_SAMPLE, supp_server_recv_func, supp_server_send_func)>=0);
		registered = 0;
	}

	gnutls_certificate_allocate_credentials(&serverx509cred);
	gnutls_certificate_set_x509_key_mem(serverx509cred,
					    &server_cert, &server_key,
					    GNUTLS_X509_FMT_PEM);

	gnutls_init(&session, GNUTLS_SERVER);

	assert(gnutls_priority_set_direct(session, prio, NULL) >= 0);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE,
				serverx509cred);

	if (!server_only) {
		gnutls_supplemental_recv(session, 1);
		gnutls_supplemental_send(session, 1);
	}

	gnutls_transport_set_int(session, sd);
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

	if (!server_only) {
		if (TLS_SUPPLEMENTALDATA_server_sent != 1 || TLS_SUPPLEMENTALDATA_server_received != 1)
			fail("server: extension not properly sent/received\n");
	}

	/* do not wait for the peer to close the connection.
	 */
	gnutls_bye(session, GNUTLS_SHUT_WR);

	close(sd);
	gnutls_deinit(session);

	gnutls_certificate_free_credentials(serverx509cred);

	if (debug)
		success("server: finished\n");

	_gnutls_supplemental_deinit();
}

static
void start(const char *prio, unsigned server_only)
{
	pid_t child;
	int sockets[2], err;

	TLS_SUPPLEMENTALDATA_client_sent = 0;
	TLS_SUPPLEMENTALDATA_client_received = 0;
	TLS_SUPPLEMENTALDATA_server_sent = 0;
	TLS_SUPPLEMENTALDATA_server_received = 0;

	success("running with %s\n", prio);

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
		server(sockets[0], prio, server_only);
		wait(&status);
		check_wait_status(status);
	} else {
		close(sockets[0]);
		client(sockets[1], prio, server_only);
		exit(0);
	}
}

void doit(void)
{
	signal(SIGPIPE, SIG_IGN);

	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(6);


	start("NORMAL:-VERS-ALL:+VERS-TLS1.3:+VERS-TLS1.2", 0);
	start("NORMAL:-VERS-ALL:+VERS-TLS1.2", 0);
	start("NORMAL", 0);
	start("NORMAL:-VERS-ALL:+VERS-TLS1.3:+VERS-TLS1.2", 1);
	start("NORMAL", 1);
}

#endif				/* _WIN32 */
