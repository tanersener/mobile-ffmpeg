/*
 * Copyright (C) 2012-2018 Free Software Foundation, Inc.
 *
 * Author: Nikos Mavrogiannopoulos, Daiki Ueno
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

int main(void)
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

#define MAX_BUF 1024
#define HIGH(x) (3*x)
static void terminate(void);
static size_t total;

/* This program tests the robustness of record sending with padding.
 */

static void server_log_func(int level, const char *str)
{
	fprintf(stderr, "server|<%d>| %s", level, str);
}

static void client_log_func(int level, const char *str)
{
	fprintf(stderr, "client|<%d>| %s", level, str);
}


/* A very basic TLS client, with anonymous authentication.
 */



static ssize_t
push(gnutls_transport_ptr_t tr, const void *data, size_t len)
{
	int fd = (long int) tr;

	total += len;
	return send(fd, data, len, 0);
}

struct test_st {
	const char *name;
	size_t pad;
	size_t data;
	const char *prio;
	unsigned flags;
	int sret;
};

static void client(int fd, struct test_st *test)
{
	int ret;
	char buffer[MAX_BUF + 1];
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

	assert(gnutls_init(&session, GNUTLS_CLIENT|test->flags)>=0);
	assert(gnutls_priority_set_direct(session, test->prio, NULL)>=0);

	/* put the anonymous credentials to the current session
	 */
	gnutls_credentials_set(session, GNUTLS_CRD_ANON, anoncred);
	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, x509_cred);

	gnutls_transport_set_int(session, fd);

	/* Perform the TLS handshake
	 */
	do {
		ret = gnutls_handshake(session);
	}
	while (ret < 0 && gnutls_error_is_fatal(ret) == 0);

	if (ret < 0) {
		fail("client: Handshake failed\n");
		gnutls_perror(ret);
		exit(1);
	} else {
		if (debug)
			success("client: Handshake was completed\n");
	}

	if (debug)
		success("client: TLS version is: %s\n",
			gnutls_protocol_get_name
			(gnutls_protocol_get_version(session)));

	do {
		do {
			ret = gnutls_record_recv(session, buffer, sizeof(buffer));
		} while (ret == GNUTLS_E_AGAIN
			 || ret == GNUTLS_E_INTERRUPTED);
	} while (ret > 0);

	if (ret == 0) {
		if (debug)
			success
			    ("client: Peer has closed the TLS connection\n");
		goto end;
	} else if (ret < 0) {
		if (ret != 0) {
			fail("client: Error: %s\n", gnutls_strerror(ret));
			exit(1);
		}
	}

	gnutls_bye(session, GNUTLS_SHUT_WR);

 end:

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

static void server(int fd, struct test_st *test)
{
	int ret;
	char buffer[MAX_BUF + 1];
	gnutls_session_t session;
	gnutls_anon_server_credentials_t anoncred;
	gnutls_certificate_credentials_t x509_cred;
	size_t expected;

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

	assert(gnutls_init(&session, GNUTLS_SERVER|test->flags)>=0);

	assert(gnutls_priority_set_direct(session, test->prio, NULL)>=0);

	gnutls_credentials_set(session, GNUTLS_CRD_ANON, anoncred);
	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, x509_cred);

	gnutls_transport_set_int(session, fd);

	do {
		ret = gnutls_handshake(session);
	}
	while (ret < 0 && gnutls_error_is_fatal(ret) == 0);
	if (ret < 0) {
		close(fd);
		gnutls_deinit(session);
		fail("server: Handshake has failed (%s)\n\n",
		     gnutls_strerror(ret));
		terminate();
	}
	if (debug)
		success("server: Handshake was completed\n");

	if (debug)
		success("server: TLS version is: %s\n",
			gnutls_protocol_get_name
			(gnutls_protocol_get_version(session)));

	gnutls_transport_set_push_function(session, push);

	assert(test->data <= sizeof(buffer));

	total = 0;
	do {
		ret =
		    gnutls_record_send2(session, buffer,
					test->data,
					test->pad, 0);
	} while (ret == GNUTLS_E_AGAIN
		 || ret == GNUTLS_E_INTERRUPTED);

	if (test->sret < 0) {
		if (ret >= 0)
			fail("server: expected failure got success!\n");
		if (ret != test->sret)
			fail("server: expected different failure: '%s', got: '%s'\n",
			     gnutls_strerror(test->sret), gnutls_strerror(ret));
		goto finish;
	}

	if (ret < 0) {
		fail("Error sending packet: %s\n",
		     gnutls_strerror(ret));
		terminate();
	}

	expected = test->data + test->pad + gnutls_record_overhead_size(session);
	if (total != expected) {
		fail("Sent data (%u) are lower than expected (%u)\n",
		     (unsigned) total, (unsigned) expected);
		terminate();
	}

 finish:
	/* do not wait for the peer to close the connection.
	 */
	gnutls_bye(session, GNUTLS_SHUT_WR);

	close(fd);
	gnutls_deinit(session);

	gnutls_anon_free_server_credentials(anoncred);
	gnutls_certificate_free_credentials(x509_cred);

	gnutls_global_deinit();

	if (debug)
		success("server: finished\n");
}

static void start(struct test_st *test)
{
	int fd[2];
	int ret;

	success("running %s\n", test->name);

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
		server(fd[0], test);
		kill(child, SIGTERM);
	} else {
		close(fd[0]);
		client(fd[1], test);
		exit(0);
	}
}

#define AES_GCM "NONE:+VERS-TLS1.3:+AES-256-GCM:+AEAD:+SIGN-ALL:+GROUP-ALL"

static void ch_handler(int sig)
{
	int status;
	wait(&status);
	check_wait_status(status);
	return;
}

struct test_st tests[] =
{
	{
		.name = "AES-GCM with max pad",
		.pad = HIGH(MAX_BUF+1)-(MAX_BUF+1),
		.data = MAX_BUF,
		.prio = AES_GCM,
		.flags = 0
	},
	{
		.name = "AES-GCM with zero pad",
		.pad = 0,
		.data = MAX_BUF,
		.prio = AES_GCM,
		.flags = 0
	},
	{
		.name = "AES-GCM with 1-byte pad",
		.pad = 1,
		.data = MAX_BUF,
		.prio = AES_GCM,
		.flags = 0
	},
	{
		.name = "AES-GCM with pad, but no data",
		.pad = 16,
		.data = 0,
		.prio = AES_GCM,
		.flags = 0
	},
	{
		.name = "AES-GCM with max pad and safe padding check",
		.pad = HIGH(MAX_BUF+1)-(MAX_BUF+1),
		.data = MAX_BUF,
		.prio = AES_GCM,
		.flags = GNUTLS_SAFE_PADDING_CHECK
	},
	{
		.name = "AES-GCM with zero pad and safe padding check",
		.pad = 0,
		.data = MAX_BUF,
		.prio = AES_GCM,
		.flags = GNUTLS_SAFE_PADDING_CHECK
	},
	{
		.name = "AES-GCM with 1-byte pad and safe padding check",
		.pad = 1,
		.data = MAX_BUF,
		.prio = AES_GCM,
		.flags = GNUTLS_SAFE_PADDING_CHECK
	},
	{
		.name = "AES-GCM with pad, but no data and safe padding check",
		.pad = 16,
		.data = 0,
		.prio = AES_GCM,
		.flags = GNUTLS_SAFE_PADDING_CHECK
	},
	{
		.name = "AES-GCM with pad, but no data and no pad",
		.pad = 0,
		.data = 0,
		.prio = AES_GCM,
		.flags = GNUTLS_SAFE_PADDING_CHECK,
		.sret = GNUTLS_E_INVALID_REQUEST
	},
};

void doit(void)
{
	unsigned i;
	signal(SIGCHLD, ch_handler);

	for (i=0;i<sizeof(tests)/sizeof(tests[0]);i++) {
		start(&tests[i]);
	}
}

#endif				/* _WIN32 */
