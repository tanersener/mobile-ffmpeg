/*
 * Copyright (C) 2012 Free Software Foundation, Inc.
 * Copyright (C) 2017 Red Hat, Inc.

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
#include "cert-common.h"

#ifdef _WIN32

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

#include "utils.h"

#define MAX_BUF 1024
static void terminate(void);

/* This program tests gnutls_server_name_set() and gnutls_server_name_get().
 */

static void server_log_func(int level, const char *str)
{
	fprintf(stderr, "server|<%d>| %s", level, str);
}

static void client_log_func(int level, const char *str)
{
	fprintf(stderr, "client|<%d>| %s", level, str);
}

/* internal function */
int _gnutls_server_name_set_raw(gnutls_session_t session,
				gnutls_server_name_type_t type,
				const void *name, size_t name_length);

static void client(const char *test_name, const char *prio, int fd, unsigned raw, const char *name, unsigned name_len, int server_err)
{
	int ret;
	gnutls_anon_client_credentials_t anoncred;
	gnutls_certificate_credentials_t x509_cred;
	gnutls_session_t session;
	/* Need to enable anonymous KX specifically. */

	global_init();

	if (debug) {
		gnutls_global_set_log_function(client_log_func);
		gnutls_global_set_log_level(7);
	}

	gnutls_anon_allocate_client_credentials(&anoncred);
	gnutls_certificate_allocate_credentials(&x509_cred);

	/* Initialize TLS session
	 */
	gnutls_init(&session, GNUTLS_CLIENT);

	/* Use default priorities */
	gnutls_priority_set_direct(session, prio, NULL);

	/* put the anonymous credentials to the current session
	 */
	gnutls_credentials_set(session, GNUTLS_CRD_ANON, anoncred);
	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, x509_cred);

	gnutls_transport_set_int(session, fd);
	if (raw)
		_gnutls_server_name_set_raw(session, GNUTLS_NAME_DNS, name, name_len);
	else
		gnutls_server_name_set(session, GNUTLS_NAME_DNS, name, name_len);

	/* Perform the TLS handshake
	 */
	do {
		ret = gnutls_handshake(session);
	}
	while (ret < 0 && gnutls_error_is_fatal(ret) == 0);

	if (ret < 0) {
		if (server_err < 0)
			goto cleanup;
		test_fail("Handshake failed\n");
		goto cleanup;
	} else {
		if (debug)
			test_success("Handshake was completed\n");
	}

	if (debug)
		test_success("TLS version is: %s\n",
			gnutls_protocol_get_name
			(gnutls_protocol_get_version(session)));

	gnutls_bye(session, GNUTLS_SHUT_WR);

 cleanup:
	close(fd);

	gnutls_deinit(session);

	gnutls_anon_free_client_credentials(anoncred);
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

static void server(const char *test_name, const char *prio, int fd, const char *name, unsigned name_len, int exp_err)
{
	int ret;
	char buffer[MAX_BUF + 1];
	unsigned int type;
	size_t buffer_size;
	gnutls_session_t session;
	gnutls_anon_server_credentials_t anoncred;
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

	gnutls_anon_allocate_server_credentials(&anoncred);

	gnutls_init(&session, GNUTLS_SERVER);

	/* avoid calling all the priority functions, since the defaults
	 * are adequate.
	 */
	gnutls_priority_set_direct(session, prio, NULL);

	gnutls_credentials_set(session, GNUTLS_CRD_ANON, anoncred);
	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, x509_cred);

	gnutls_transport_set_int(session, fd);

	do {
		ret = gnutls_handshake(session);
	}
	while (ret < 0 && gnutls_error_is_fatal(ret) == 0);
	if (ret < 0) {
		if (exp_err == ret)
			goto cleanup;
		close(fd);
		gnutls_deinit(session);
		test_fail("Handshake has failed (%s)\n\n",
		     gnutls_strerror(ret));
		terminate();
	}
	if (debug)
		test_success("Handshake was completed\n");

	if (debug)
		test_success("TLS version is: %s\n",
			gnutls_protocol_get_name
			(gnutls_protocol_get_version(session)));

	buffer_size = sizeof(buffer);
	ret = gnutls_server_name_get(session, buffer, &buffer_size, &type, 0);

	if ((name == NULL || name[0] == 0) && (ret == GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE || ret == GNUTLS_E_IDNA_ERROR)) {
		/* expected */
		if (debug)
			test_success("empty name matches\n");
	} else if (ret < 0) {
		test_fail("server_name: %s/%d\n", gnutls_strerror(ret), ret);
	} else {
		if (name == NULL || name[0] == 0) {
			test_fail("did not receive the expected name: got: %s\n", buffer);
			exit(1);
		}
		if (buffer_size != strlen(buffer)) {
			test_fail("received name '%s/%d/%d', with embedded null\n", buffer, (int)buffer_size, (int)strlen(buffer));
			exit(1);
		}
		if (name_len != buffer_size || memcmp(name, buffer, name_len) != 0) {
			test_fail("received name '%s/%d', expected '%s/%d'\n", buffer, (int)buffer_size, name, (int)name_len);
			exit(1);
		}
		if (debug)
			test_success("name matches (%s/%s)\n", buffer, name);
	}


	/* do not wait for the peer to close the connection.
	 */
	gnutls_bye(session, GNUTLS_SHUT_WR);
 cleanup:
	close(fd);
	gnutls_deinit(session);

	gnutls_anon_free_server_credentials(anoncred);
	gnutls_certificate_free_credentials(x509_cred);

	gnutls_global_deinit();

	if (debug)
		test_success("finished\n");
}

/* name: the name sent by client
 * server_exp: the name which should be expected by the server to see
 */
static void start(const char *test_name, const char *prio, unsigned raw, const char *name, unsigned len, const char *server_exp, unsigned server_exp_len, int server_error)
{
	int fd[2];
	int ret;

	ret = socketpair(AF_UNIX, SOCK_STREAM, 0, fd);
	if (ret < 0) {
		perror("socketpair");
		exit(1);
	}

	child = fork();
	if (child < 0) {
		perror("fork");
		test_fail("fork");
		exit(1);
	}

	if (child) {
		/* parent */
		close(fd[1]);
		server(test_name, prio, fd[0], server_exp, server_exp_len, server_error);
		kill(child, SIGTERM);
	} else {
		close(fd[0]);
		client(test_name, prio, fd[1], raw, name, len, server_error);
		exit(0);
	}
}

static void ch_handler(int sig)
{
	int status;
	wait(&status);
	check_wait_status(status);
	return;
}

#define PRIO_TLS12 "NORMAL:-VERS-ALL:+VERS-TLS1.2"
#define PRIO_TLS13 "NORMAL:-VERS-ALL:+VERS-TLS1.3"
#define PRIO_NORMAL "NORMAL"

void doit(void)
{
	signal(SIGCHLD, ch_handler);
	signal(SIGPIPE, SIG_IGN);

	start("tls1.2 NULL", PRIO_TLS12, 0, NULL, 0, NULL, 0, 0);
	start("tls1.2 empty", PRIO_TLS12, 0, "", 0, "", 0, 0);
	start("tls1.2 test.example.com", PRIO_TLS12, 0, "test.example.com", strlen("test.example.com"), "test.example.com", strlen("test.example.com"), 0);
	start("tls1.2 longtest.example.com", PRIO_TLS12, 0, "longtest.example.com.", strlen("longtest.example.com"), "longtest.example.com.", strlen("longtest.example.com"), 0);
	/* test embedded NULL */
	start("tls1.2 embedded-NULL", PRIO_TLS12, 1, "invalid\x00.example.com.", sizeof("invalid\x00.example.com")-1, NULL, 0, GNUTLS_E_RECEIVED_DISALLOWED_NAME);

	start("tls1.3 NULL", PRIO_TLS13, 0, NULL, 0, NULL, 0, 0);
	start("tls1.3 empty", PRIO_TLS13, 0, "", 0, "", 0, 0);
	start("tls1.3 test.example.com", PRIO_TLS13, 0, "test.example.com", strlen("test.example.com"), "test.example.com", strlen("test.example.com"), 0);
	start("tls1.3 longtest.example.com", PRIO_TLS13, 0, "longtest.example.com.", strlen("longtest.example.com"), "longtest.example.com.", strlen("longtest.example.com"), 0);
	/* test embedded NULL */
	start("tls1.3 embedded-NULL", PRIO_TLS13, 1, "invalid\x00.example.com.", sizeof("invalid\x00.example.com")-1, NULL, 0, GNUTLS_E_RECEIVED_DISALLOWED_NAME);

	start("NULL", PRIO_NORMAL, 0, NULL, 0, NULL, 0, 0);
	start("empty", PRIO_NORMAL, 0, "", 0, "", 0, 0);
	start("test.example.com", PRIO_NORMAL, 0, "test.example.com", strlen("test.example.com"), "test.example.com", strlen("test.example.com"), 0);
	start("longtest.example.com", PRIO_NORMAL, 0, "longtest.example.com.", strlen("longtest.example.com"), "longtest.example.com.", strlen("longtest.example.com"), 0);
	/* test embedded NULL */
	start("embedded-NULL", PRIO_NORMAL, 1, "invalid\x00.example.com.", sizeof("invalid\x00.example.com")-1, NULL, 0, GNUTLS_E_RECEIVED_DISALLOWED_NAME);
}

#endif				/* _WIN32 */
