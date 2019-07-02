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
#include <assert.h>

#include "cert-common.h"
#include "tls13/ext-parse.h"
#include "utils.h"

#define MAX_AUTHS 4

/* This program tests whether the Post Handshake Auth extension is
 * present in the client hello, and whether it is missing from server
 * hello. In addition it contains basic functionality test for
 * post handshake authentication.
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
#define MAX_APP_DATA 3

static void client(int fd, unsigned send_cert, unsigned max_auths)
{
	int ret;
	gnutls_certificate_credentials_t x509_cred;
	gnutls_session_t session;
	char buf[64];
	unsigned i, j;

	global_init();

	if (debug) {
		gnutls_global_set_log_function(client_log_func);
		gnutls_global_set_log_level(7);
	}

	assert(gnutls_certificate_allocate_credentials(&x509_cred)>=0);

	/* Initialize TLS session
	 */
	assert(gnutls_init(&session, GNUTLS_CLIENT|GNUTLS_POST_HANDSHAKE_AUTH)>=0);

	gnutls_handshake_set_timeout(session, 20 * 1000);

	ret = gnutls_priority_set_direct(session, "NORMAL:-VERS-ALL:+VERS-TLS1.3:+VERS-TLS1.2:+VERS-TLS1.0", NULL);
	if (ret < 0)
		fail("cannot set TLS 1.3 priorities\n");


	if (send_cert) {
		assert(gnutls_certificate_set_x509_key_mem(x509_cred, &cli_ca3_cert,
						    &cli_ca3_key,
						    GNUTLS_X509_FMT_PEM)>=0);
	}

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, x509_cred);

	gnutls_transport_set_int(session, fd);

	/* Perform the TLS handshake
	 */
	do {
		ret = gnutls_handshake(session);
	}
	while (ret < 0 && gnutls_error_is_fatal(ret) == 0);

	if (ret != 0)
		fail("handshake failed: %s\n", gnutls_strerror(ret));

	if (debug)
		success("client handshake completed\n");

	gnutls_record_set_timeout(session, 20 * 1000);

	for (i=0;i<max_auths;i++) {
		if (debug)
			success("waiting for auth nr %d\n", i);

		do {
			ret = gnutls_record_recv(session, buf, sizeof(buf));
		} while (ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED);

		if (ret != GNUTLS_E_REAUTH_REQUEST) {
			fail("recv: unexpected error: %s\n", gnutls_strerror(ret));
		}

		/* send application data to check if server tolerates them */
		if (i==0) {
			for (j=0;j<MAX_APP_DATA;j++) {
				memset(buf, j, sizeof(buf));
				do {
					ret = gnutls_record_send(session, buf, sizeof(buf));
				} while (ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED);
				assert(ret>=0);
			}
		}

		if (debug)
			success("received reauth request\n");
		do {
			ret = gnutls_reauth(session, 0);
		} while (ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED);

		if (ret != 0)
			fail("client: gnutls_reauth did not succeed as expected: %s\n", gnutls_strerror(ret));
	}


	close(fd);

	gnutls_deinit(session);

	gnutls_certificate_free_credentials(x509_cred);

	gnutls_global_deinit();
}

static unsigned client_hello_ok = 0;
static unsigned server_hello_ok = 0;

#define TLS_EXT_POST_HANDSHAKE 49

static void parse_ext(void *priv, gnutls_datum_t *msg)
{
	if (msg->size != 0) {
		fail("error in extension length: %d\n", (int)msg->size);
	}
}

static int hellos_callback(gnutls_session_t session, unsigned int htype,
	unsigned post, unsigned int incoming, const gnutls_datum_t *msg)
{
	if (htype == GNUTLS_HANDSHAKE_SERVER_HELLO && post == GNUTLS_HOOK_POST) {
		if (find_server_extension(msg, TLS_EXT_POST_HANDSHAKE, NULL, NULL)) {
			fail("Post handshake extension seen in server hello!\n");
		}
		server_hello_ok = 1;

		return GNUTLS_E_INTERRUPTED;
	}

	if (htype != GNUTLS_HANDSHAKE_CLIENT_HELLO || post != GNUTLS_HOOK_PRE)
		return 0;

	if (find_client_extension(msg, TLS_EXT_POST_HANDSHAKE, NULL, parse_ext))
		client_hello_ok = 1;
	else
		fail("Post handshake extension NOT seen in client hello!\n");

	return 0;
}

static void server(int fd, int err, int type, unsigned max_auths)
{
	int ret;
	char buffer[MAX_BUF + 1];
	gnutls_session_t session;
	gnutls_certificate_credentials_t x509_cred;
	unsigned i, retries;

	/* this must be called once in the program
	 */
	global_init();
	memset(buffer, 0, sizeof(buffer));

	if (debug) {
		gnutls_global_set_log_function(server_log_func);
		gnutls_global_set_log_level(6);
	}

	gnutls_certificate_allocate_credentials(&x509_cred);
	gnutls_certificate_set_x509_key_mem(x509_cred, &server_cert,
					    &server_key,
					    GNUTLS_X509_FMT_PEM);

	gnutls_init(&session, GNUTLS_SERVER|GNUTLS_POST_HANDSHAKE_AUTH);

	gnutls_handshake_set_timeout(session, 20 * 1000);
	gnutls_handshake_set_hook_function(session, GNUTLS_HANDSHAKE_ANY,
					   GNUTLS_HOOK_BOTH,
					   hellos_callback);

	/* avoid calling all the priority functions, since the defaults
	 * are adequate.
	 */
	gnutls_priority_set_direct(session, "NORMAL:+VERS-TLS1.3", NULL);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, x509_cred);

	gnutls_transport_set_int(session, fd);

	do {
		ret = gnutls_handshake(session);
	} while (ret < 0 && gnutls_error_is_fatal(ret) == 0);

	if (ret != 0)
		fail("handshake failed: %s\n", gnutls_strerror(ret));

	if (!(gnutls_session_get_flags(session) & GNUTLS_SFLAGS_POST_HANDSHAKE_AUTH)) {
		fail("server: session flags did not contain GNUTLS_SFLAGS_POST_HANDSHAKE_AUTH\n");
	}


	if (client_hello_ok == 0) {
		fail("server: did not verify the client hello\n");
	}

	if (server_hello_ok == 0) {
		fail("server: did not verify the server hello contents\n");
	}

	if (debug)
		success("server handshake completed\n");

	gnutls_certificate_server_set_request(session, type);

	/* i = 0 */
	/* ask peer for re-authentication */
	retries = 0;
	do {
		do {
			ret = gnutls_reauth(session, 0);
		} while (ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED);

		if (ret == GNUTLS_E_GOT_APPLICATION_DATA) {
			int ret2;
			do {
				ret2 = gnutls_record_recv(session, buffer, sizeof(buffer));
			} while (ret2 == GNUTLS_E_AGAIN || ret2 == GNUTLS_E_INTERRUPTED);

			if (ret2 < 0)
				fail("error receiving app data: %s\n", gnutls_strerror(ret2));

			/* sender memsets the message with the retry attempt */
			assert((uint8_t)buffer[0] == retries);
			assert(retries < MAX_APP_DATA);
		}

		retries++;
	} while (ret == GNUTLS_E_GOT_APPLICATION_DATA);

	if (err) {
		if (ret != err)
			fail("server: expected error %s, got: %s\n", gnutls_strerror(err),
			     gnutls_strerror(ret));
	} else if (ret != 0)
		fail("server: gnutls_reauth did not succeed as expected: %s\n", gnutls_strerror(ret));


	for (i=1;i<max_auths;i++) {
		/* ask peer for re-authentication */
		do {
			ret = gnutls_reauth(session, 0);
		} while (ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED);

		if (err) {
			if (ret != err)
				fail("server: expected error %s, got: %s\n", gnutls_strerror(err),
				     gnutls_strerror(ret));
		} else if (ret != 0)
			fail("server: gnutls_reauth did not succeed as expected: %s\n", gnutls_strerror(ret));
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

static
void start(const char *name, int err, int type, unsigned max_auths, unsigned send_cert)
{
	int fd[2];
	int ret;
	pid_t child;

	success("testing %s\n", name);

	client_hello_ok = 0;
	server_hello_ok = 0;

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
		server(fd[0], err, type, max_auths);
		kill(child, SIGTERM);
	} else {
		close(fd[0]);
		client(fd[1], send_cert, max_auths);
		exit(0);
	}

}

void doit(void)
{
	start("multi-reauth", 0, GNUTLS_CERT_REQUIRE, MAX_AUTHS, 1);
	start("reauth-require with no-cert", GNUTLS_E_CERTIFICATE_REQUIRED, GNUTLS_CERT_REQUIRE, 1, 0);
	start("reauth-request with no-cert", 0, GNUTLS_CERT_REQUEST, 1, 0);
}
#endif				/* _WIN32 */
