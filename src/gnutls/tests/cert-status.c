/*
 * Copyright (C) 2012 Free Software Foundation, Inc.
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
#include <assert.h>
#include <signal.h>

#include "utils.h"
#include "cert-common.h"

/* This program tests whether the GNUTLS_CERT_* flags
 * work as expected.
 */

static void server_log_func(int level, const char *str)
{
//  fprintf (stderr, "server|<%d>| %s", level, str);
}

static void client_log_func(int level, const char *str)
{
	fprintf(stderr, "client|<%d>| %s", level, str);
}

#define MAX_BUF 1024

static void client(int fd, const char *prio)
{
	int ret;
	const char *p;
	gnutls_certificate_credentials_t x509_cred;
	gnutls_session_t session;

	global_init();

	if (debug) {
		gnutls_global_set_log_function(client_log_func);
		gnutls_global_set_log_level(7);
	}

	gnutls_certificate_allocate_credentials(&x509_cred);

	gnutls_init(&session, GNUTLS_CLIENT);

	ret =
	    gnutls_priority_set_direct(session,
					prio, &p);
	if (ret < 0) {
		fail("error in setting priority: %s\n", p);
		exit(1);
	}

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, x509_cred);

	gnutls_transport_set_int(session, fd);

	/* Perform the TLS handshake
	 */
	do {
		ret = gnutls_handshake(session);
	}
	while (ret < 0 && gnutls_error_is_fatal(ret) == 0);

	if (ret < 0) {
		if (debug) {
			fail("client: Handshake failed\n");
			gnutls_perror(ret);
		}
		exit(1);
	} else {
		if (debug)
			success("client: Handshake was completed\n");
	}

	if (debug)
		success("client: TLS version is: %s\n",
			gnutls_protocol_get_name
			(gnutls_protocol_get_version(session)));

	close(fd);

	gnutls_deinit(session);

	gnutls_certificate_free_credentials(x509_cred);

	gnutls_global_deinit();
}


static void server(int fd, const char *prio, unsigned status, int expected)
{
	int ret;
	char buffer[MAX_BUF + 1];
	gnutls_session_t session;
	gnutls_certificate_credentials_t x509_cred;

	/* this must be called once in the program
	 */
	global_init();
	memset(buffer, 0, sizeof(buffer));

	gnutls_init(&session, GNUTLS_SERVER);

	assert(gnutls_priority_set_direct(session,
					  prio, NULL)>=0);

	if (debug) {
		gnutls_global_set_log_function(server_log_func);
		gnutls_global_set_log_level(4711);
	}

	gnutls_certificate_allocate_credentials(&x509_cred);
	gnutls_certificate_set_x509_key_mem(x509_cred, &server_cert,
					    &server_key,
					    GNUTLS_X509_FMT_PEM);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, x509_cred);

	gnutls_transport_set_int(session, fd);
	gnutls_certificate_server_set_request(session, status);

	do {
		ret = gnutls_handshake(session);
	}
	while (ret < 0 && gnutls_error_is_fatal(ret) == 0);

	if (ret == expected) {
		if (debug)
			success
			    ("server: Handshake finished as expected (%d)\n", ret);
		goto finish;
	} else {
		fail("expected %d, handshake returned %d\n", expected,
		     ret);
	}

	if (debug)
		success("server: Handshake was completed\n");

	if (debug)
		success("server: TLS version is: %s\n",
			gnutls_protocol_get_name
			(gnutls_protocol_get_version(session)));
      finish:
	close(fd);
	gnutls_deinit(session);

	gnutls_certificate_free_credentials(x509_cred);

	gnutls_global_deinit();

	if (debug)
		success("server: finished\n");
}

static void start(const char *prio, unsigned status, int expected)
{
	int fd[2];
	int ret;
	pid_t child;
	int pstatus = 0;

	success("testing: %s (%d,%d)\n", prio, status, expected);

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
		server(fd[0], prio, status, expected);
		waitpid(-1, &pstatus, 0);
		check_wait_status_for_sig(pstatus);
	} else {
		close(fd[0]);
		client(fd[1], prio);
		exit(0);
	}
}

static void ch_handler(int sig)
{
	return;
}


void doit(void)
{
	signal(SIGCHLD, ch_handler);
	signal(SIGPIPE, SIG_IGN);

	start("NONE:+VERS-TLS1.0:+AES-128-CBC:+SHA1:+SIGN-ALL:+COMP-NULL:+RSA", GNUTLS_CERT_IGNORE, 0);
	start("NONE:+VERS-TLS1.0:+AES-128-CBC:+SHA1:+SIGN-ALL:+COMP-NULL:+RSA", GNUTLS_CERT_REQUEST, 0);
	start("NONE:+VERS-TLS1.0:+AES-128-CBC:+SHA1:+SIGN-ALL:+COMP-NULL:+RSA", GNUTLS_CERT_REQUIRE, GNUTLS_E_NO_CERTIFICATE_FOUND);

	start("NORMAL:-VERS-ALL:+VERS-TLS1.2", GNUTLS_CERT_IGNORE, 0);
	start("NORMAL:-VERS-ALL:+VERS-TLS1.2", GNUTLS_CERT_REQUEST, 0);
	start("NORMAL:-VERS-ALL:+VERS-TLS1.2", GNUTLS_CERT_REQUIRE, GNUTLS_E_NO_CERTIFICATE_FOUND);

	start("NORMAL:-VERS-ALL:+VERS-TLS1.3", GNUTLS_CERT_IGNORE, 0);
	start("NORMAL:-VERS-ALL:+VERS-TLS1.3", GNUTLS_CERT_REQUEST, 0);
	start("NORMAL:-VERS-ALL:+VERS-TLS1.3", GNUTLS_CERT_REQUIRE, GNUTLS_E_CERTIFICATE_REQUIRED);

	start("NORMAL", GNUTLS_CERT_IGNORE, 0);
	start("NORMAL", GNUTLS_CERT_REQUEST, 0);
	start("NORMAL", GNUTLS_CERT_REQUIRE, GNUTLS_E_CERTIFICATE_REQUIRED);
}

#endif				/* _WIN32 */
