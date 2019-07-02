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
#include <gnutls/abstract.h>
#include <gnutls/urls.h>
#include <signal.h>
#include <assert.h>
#include "cert-common.h"

#include "utils.h"

static void terminate(void);
static unsigned url_used = 0;

/* This program tests the registration of custom URL strings.
 */

static void server_log_func(int level, const char *str)
{
	fprintf(stderr, "server|<%d>| %s", level, str);
}

static int url_import_key(gnutls_privkey_t pkey, const char *url, unsigned flags)
{
	if (strcmp(url, "system:key") != 0) {
		fail("unexpected key url: %s\n", url);
		return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
	}
	url_used++;
	return gnutls_privkey_import_x509_raw(pkey, &server_key, GNUTLS_X509_FMT_PEM, NULL, 0);
}

static int url_import_crt(gnutls_x509_crt_t crt, const char *url, unsigned flags)
{
	if (strcmp(url, "system:cert") != 0) {
		abort();
		fail("unexpected cert url: %s\n", url);
		return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
	}
	url_used++;
	return gnutls_x509_crt_import(crt, &server_cert, GNUTLS_X509_FMT_PEM);
}

static void client(int fd)
{
	int ret;
	gnutls_certificate_credentials_t x509_cred;
	gnutls_session_t session;

	gnutls_certificate_allocate_credentials(&x509_cred);

	/* Initialize TLS session
	 */
	gnutls_init(&session, GNUTLS_CLIENT);

	/* Use default priorities */
	assert(gnutls_priority_set_direct(session, "NORMAL:-VERS-ALL:+VERS-TLS1.2", NULL)>=0);

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

	gnutls_bye(session, GNUTLS_SHUT_WR);

	close(fd);

	gnutls_deinit(session);

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

static void server(int fd)
{
	int ret;
	gnutls_session_t session;
	gnutls_certificate_credentials_t x509_cred;

	/* this must be called once in the program
	 */
	gnutls_certificate_allocate_credentials(&x509_cred);

	ret = gnutls_certificate_set_x509_key_file(x509_cred, "system:cert", "system:key",
						   GNUTLS_X509_FMT_PEM);
	if (ret < 0) {
		fail("server: gnutls_certificate_set_x509_key_file (%s)\n\n",
		     gnutls_strerror(ret));
		terminate();
	}

	gnutls_init(&session, GNUTLS_SERVER);

	/* avoid calling all the priority functions, since the defaults
	 * are adequate.
	 */
	assert(gnutls_priority_set_direct(session, "NORMAL:-VERS-ALL:+VERS-TLS1.2", NULL)>=0);

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

	if (url_used != 2) {
		fail("The callbacks were not used\n");
		terminate();
	}

	/* do not wait for the peer to close the connection.
	 */
	gnutls_bye(session, GNUTLS_SHUT_WR);

	close(fd);
	gnutls_deinit(session);

	gnutls_certificate_free_credentials(x509_cred);

	gnutls_global_deinit();

	if (debug)
		success("server: finished\n");
}

const gnutls_custom_url_st custom_url_st = {
	.name = "system:",
	.name_size = sizeof("system:")-1,
	.import_key = url_import_key,
	.import_crt = url_import_crt
};

static void start(void)
{
	int fd[2];
	int ret;

	global_init();

	if (debug) {
		gnutls_global_set_log_function(server_log_func);
		gnutls_global_set_log_level(3);
	}

	ret = gnutls_register_custom_url(&custom_url_st);
	if (ret < 0) {
		fail("error registering: %s\n", gnutls_strerror(ret));
		exit(1);
	}

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
		close(fd[0]);
		client(fd[1]);
		kill(child, SIGTERM);
	} else {
		close(fd[1]);
		server(fd[0]);
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

void doit(void)
{
	signal(SIGCHLD, ch_handler);
	signal(SIGPIPE, SIG_IGN);

	start();
}

#endif				/* _WIN32 */
