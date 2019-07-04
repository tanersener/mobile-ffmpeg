/*
 * Copyright (C) 2017 Red Hat, Inc.
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
#include <errno.h>

#include "cert-common.h"
#include "utils.h"

/* This program tests whether the ChangeCipherSpec message
 * is ignored during handshake.
 */

static void server_log_func(int level, const char *str)
{
	fprintf(stderr, "server|<%d>| %s", level, str);
}

static void client_log_func(int level, const char *str)
{
	fprintf(stderr, "client|<%d>| %s", level, str);
}

static unsigned client_sent_ccs = 0;
static unsigned server_sent_ccs = 0;

static int cli_hsk_callback(gnutls_session_t session, unsigned int htype,
	unsigned post, unsigned int incoming, const gnutls_datum_t *msg);

static void client(int fd, unsigned ccs_check)
{
	int ret;
	gnutls_certificate_credentials_t x509_cred;
	gnutls_session_t session;
	char buf[64];

	global_init();
	client_sent_ccs = 0;
	server_sent_ccs = 0;

	if (debug) {
		gnutls_global_set_log_function(client_log_func);
		gnutls_global_set_log_level(7);
	}

	gnutls_certificate_allocate_credentials(&x509_cred);

	/* Initialize TLS session
	 */
	gnutls_init(&session, GNUTLS_CLIENT|GNUTLS_POST_HANDSHAKE_AUTH);

	gnutls_session_set_ptr(session, &ccs_check);
	gnutls_handshake_set_timeout(session, 20 * 1000);
	if (ccs_check) {
		gnutls_handshake_set_hook_function(session, GNUTLS_HANDSHAKE_ANY,
						   GNUTLS_HOOK_PRE,
						   cli_hsk_callback);
	}

	ret = gnutls_priority_set_direct(session, "NORMAL:-VERS-ALL:+VERS-TLS1.3:+VERS-TLS1.2:+VERS-TLS1.0", NULL);
	if (ret < 0)
		fail("cannot set TLS 1.3 priorities\n");


	gnutls_certificate_set_x509_key_mem(x509_cred, &cli_ca3_cert,
					    &cli_ca3_key,
					    GNUTLS_X509_FMT_PEM);

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

	if (ret != 0)
		fail("handshake failed: %s\n", gnutls_strerror(ret));
	success("client handshake completed\n");

	do {
		ret = gnutls_record_recv(session, buf, sizeof(buf));
	} while (ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED);

	if (ret < 0)
		fail("client: recv did not succeed as expected: %s\n", gnutls_strerror(ret));

	/* send change cipher spec, this should fail in the server */
	do {
		ret = send(fd, "\x14\x03\x03\x00\x01\x01", 6, 0);
	} while(ret == -1 && (errno == EINTR || errno == EAGAIN));

	close(fd);

	gnutls_deinit(session);

	if (ccs_check) {
		if (client_sent_ccs != 1) {
			fail("client: did not sent CCS\n");
		}
	}

	gnutls_certificate_free_credentials(x509_cred);

	gnutls_global_deinit();
}

static int cli_hsk_callback(gnutls_session_t session, unsigned int htype,
	unsigned post, unsigned int incoming, const gnutls_datum_t *msg)
{
	unsigned *p;
	unsigned ccs_check;
	static unsigned hello_received = 0;

	p = gnutls_session_get_ptr(session);
	ccs_check = *p;

	assert(ccs_check != 0);
	assert(post == GNUTLS_HOOK_PRE);

	if (htype == GNUTLS_HANDSHAKE_CLIENT_HELLO && !incoming) {
		hello_received = 1;

		gnutls_handshake_set_hook_function(session, GNUTLS_HANDSHAKE_CHANGE_CIPHER_SPEC,
						   GNUTLS_HOOK_PRE,
						   cli_hsk_callback);
	}

	if (htype == GNUTLS_HANDSHAKE_CHANGE_CIPHER_SPEC && !incoming && hello_received) {
		client_sent_ccs++;
		assert(msg->size == 1 && msg->data[0] == 0x01);
	}


	return 0;
}

static int hsk_callback(gnutls_session_t session, unsigned int htype,
	unsigned post, unsigned int incoming, const gnutls_datum_t *msg)
{
	int ret;
	int fd;
	unsigned *p;
	unsigned ccs_check;

	p = gnutls_session_get_ptr(session);
	ccs_check = *p;

	assert(post == GNUTLS_HOOK_PRE);

	if (!ccs_check) {
		if (!incoming || htype == GNUTLS_HANDSHAKE_CLIENT_HELLO ||
		    htype == GNUTLS_HANDSHAKE_FINISHED)
			return 0;

		fd = gnutls_transport_get_int(session);

		/* send change cipher spec */
		do {
			ret = send(fd, "\x14\x03\x03\x00\x01\x01", 6, 0);
		} while(ret == -1 && (errno == EINTR || errno == EAGAIN));
	} else { /* checking whether server received it */
		if (htype == GNUTLS_HANDSHAKE_CHANGE_CIPHER_SPEC && !incoming) {
			server_sent_ccs++;
			assert(msg->size == 1 && msg->data[0] == 0x01);
		}
	}
	return 0;
}

static void server(int fd, unsigned ccs_check)
{
	int ret;
	gnutls_session_t session;
	gnutls_certificate_credentials_t x509_cred;
	char buf[64];

	/* this must be called once in the program
	 */
	global_init();

	client_sent_ccs = 0;
	server_sent_ccs = 0;

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

	if (ccs_check)
		gnutls_handshake_set_hook_function(session, GNUTLS_HANDSHAKE_CHANGE_CIPHER_SPEC,
						   GNUTLS_HOOK_PRE,
						   hsk_callback);
	else
		gnutls_handshake_set_hook_function(session, GNUTLS_HANDSHAKE_ANY,
						   GNUTLS_HOOK_PRE,
						   hsk_callback);

	/* avoid calling all the priority functions, since the defaults
	 * are adequate.
	 */
	assert(gnutls_priority_set_direct(session, "NORMAL:+VERS-TLS1.3", NULL) >= 0);
	gnutls_session_set_ptr(session, &ccs_check);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, x509_cred);

	gnutls_transport_set_int(session, fd);

	do {
		ret = gnutls_handshake(session);
	} while (ret < 0 && gnutls_error_is_fatal(ret) == 0);

	if (ret != 0)
		fail("handshake failed: %s\n", gnutls_strerror(ret));

	success("server handshake completed\n");

	gnutls_certificate_server_set_request(session, GNUTLS_CERT_REQUIRE);
	/* ask peer for re-authentication */
	do {
		ret = gnutls_record_send(session, "\x00", 1);
	} while (ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED);

	if (ret < 0)
		fail("server: gnutls_record_send did not succeed as expected: %s\n", gnutls_strerror(ret));

	/* receive CCS and fail */
	do {
		ret = gnutls_record_recv(session, buf, sizeof(buf));
	} while (ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED);

	if (ret != GNUTLS_E_UNEXPECTED_PACKET)
		fail("server: incorrect alert sent: %d != %d\n",
		     ret, GNUTLS_E_UNEXPECTED_PACKET);

	close(fd);
	gnutls_deinit(session);

	gnutls_certificate_free_credentials(x509_cred);

	if (ccs_check) {
		if (server_sent_ccs != 1) {
			fail("server: did not sent CCS\n");
		}
	}

	gnutls_global_deinit();

	if (debug)
		success("server: client/server hello were verified\n");
}

static void ch_handler(int sig)
{
	int status;
	wait(&status);
	check_wait_status(status);
	return;
}

static
void start(unsigned ccs_check)
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
		server(fd[0], ccs_check);
		kill(child, SIGTERM);
	} else {
		close(fd[0]);
		client(fd[1], ccs_check);
		exit(0);
	}
}

void doit(void)
{
	start(0);
	start(1);
}

#endif				/* _WIN32 */
