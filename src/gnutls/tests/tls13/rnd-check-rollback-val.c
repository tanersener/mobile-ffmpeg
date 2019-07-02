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

#include "cert-common.h"
#include "utils.h"

/* This program checks whether a TLS 1.3 client will detect
 * a TLS 1.2 rollback attempt via the server random value.
 */

static void server_log_func(int level, const char *str)
{
	fprintf(stderr, "server|<%d>| %s", level, str);
}

static void client_log_func(int level, const char *str)
{
	fprintf(stderr, "client|<%d>| %s", level, str);
}

#ifdef TLS12
# define name "TLS1.2"
# define RND tls12_rnd
# define PRIO "NORMAL:-VERS-TLS-ALL:+VERS-TLS1.2"
#elif TLS11
# define name "TLS1.1"
# define RND tls11_rnd
# define PRIO "NORMAL:-VERS-TLS-ALL:+VERS-TLS1.1:+VERS-TLS1.0"
#else
# error unknown version to test
#endif

gnutls_datum_t tls12_rnd = {(void*)"\x44\x4F\x57\x4E\x47\x52\x44\x01",
			      8};

gnutls_datum_t tls11_rnd = {(void*)"\x44\x4F\x57\x4E\x47\x52\x44\x00",
			      8};


static void client(int fd)
{
	int ret;
	gnutls_certificate_credentials_t x509_cred;
	gnutls_session_t session;
	gnutls_datum_t srandom;
	unsigned try = 0;
	gnutls_datum_t session_data = { NULL, 0 };

	global_init();

	if (debug) {
		gnutls_global_set_log_function(client_log_func);
		gnutls_global_set_log_level(7);
	}

	gnutls_certificate_allocate_credentials(&x509_cred);
	gnutls_certificate_set_x509_key_mem(x509_cred, &cli_ca3_cert,
					    &cli_ca3_key,
					    GNUTLS_X509_FMT_PEM);

 retry:
	/* Initialize TLS session
	 */
	gnutls_init(&session, GNUTLS_CLIENT);

	gnutls_handshake_set_timeout(session, 20 * 1000);

	ret = gnutls_priority_set_direct(session, PRIO, NULL);
	if (ret < 0)
		fail("cannot set TLS priorities\n");

	if (try > 0)
		gnutls_session_set_data(session, session_data.data, session_data.size);

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

	if (ret < 0) {
		fail("error in handshake: %s\n", gnutls_strerror(ret));
	}

	if (try > 0)
		assert(gnutls_session_is_resumed(session));

	gnutls_session_get_random(session, NULL, &srandom);

	if (srandom.size != 32)
		fail("unexpected random size\n");

	if (memcmp(&srandom.data[32-8], RND.data, 8) != 0) {
		unsigned i;
		printf("expected: ");
		for (i=0;i<8;i++)
			printf("%.2x", (unsigned)RND.data[i]);
		printf("\n");
		printf("got:      ");
		for (i=0;i<8;i++)
			printf("%.2x", (unsigned)srandom.data[32-8+i]);
		printf("\n");
		fail("unexpected random data for %s\n", name);
	}

	do {
		ret = gnutls_record_send(session, "\x00", 1);
	} while (ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED);

	if (try == 0) {
		ret = gnutls_session_get_data2(session, &session_data);
		if (ret < 0)
			fail("couldn't retrieve session data: %s\n",
			     gnutls_strerror(ret));
	}

	gnutls_deinit(session);

	if (try == 0) {
		try++;
		goto retry;
	}

	close(fd);

	gnutls_free(session_data.data);

	gnutls_certificate_free_credentials(x509_cred);

	gnutls_global_deinit();
}


static void server(int fd)
{
	int ret;
	gnutls_session_t session;
	gnutls_certificate_credentials_t x509_cred;
	gnutls_datum_t skey;
	unsigned try = 0;
	unsigned char buf[16];

	/* this must be called once in the program
	 */
	global_init();

	if (debug) {
		gnutls_global_set_log_function(server_log_func);
		gnutls_global_set_log_level(4711);
	}

	gnutls_certificate_allocate_credentials(&x509_cred);
	gnutls_certificate_set_x509_key_mem(x509_cred, &server_cert,
					    &server_key,
					    GNUTLS_X509_FMT_PEM);

	assert(gnutls_session_ticket_key_generate(&skey) >= 0);

 retry:
	gnutls_init(&session, GNUTLS_SERVER);

	gnutls_handshake_set_timeout(session, 20 * 1000);

	assert(gnutls_priority_set_direct(session, "NORMAL:-VERS-TLS-ALL:+VERS-TLS1.3:+VERS-TLS1.2:+VERS-TLS1.1:+VERS-TLS1.0", NULL)>=0);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, x509_cred);

	assert(gnutls_session_ticket_enable_server(session, &skey) >= 0);

	gnutls_transport_set_int(session, fd);

	do {
		ret = gnutls_handshake(session);
		if (ret == GNUTLS_E_INTERRUPTED) { /* expected */
			break;
		}
	} while (ret < 0 && gnutls_error_is_fatal(ret) == 0);

	if (ret < 0)
		fail("error in handshake: %s\n", gnutls_strerror(ret));

	if (try > 0)
		assert(gnutls_session_is_resumed(session));

	do {
		ret = gnutls_record_recv(session, buf, sizeof(buf));
	} while (ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED);

	if (ret < 0)
		fail("server: recv did not succeed as expected: %s\n", gnutls_strerror(ret));

	gnutls_deinit(session);

	if (try == 0) {
		try++;
		goto retry;
	}

	close(fd);

	gnutls_free(skey.data);
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
		client(fd[0]);
		kill(child, SIGTERM);
	} else {
		close(fd[0]);
		server(fd[1]);
		exit(0);
	}
}

#endif				/* _WIN32 */
