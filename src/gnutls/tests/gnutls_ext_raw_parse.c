/*
 * Copyright (C) 2018 Red Hat, Inc.
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
#include <time.h>
#include <gnutls/gnutls.h>
#include <signal.h>
#include <assert.h>

#include "utils.h"
#include "cert-common.h"
#include "tls13/ext-parse.h"

/* This program tests gnutls_ext_raw_parse with GNUTLS_EXT_RAW_FLAG_TLS_CLIENT_HELLO
 * flag.
 */

#define HOSTNAME "example.com"

static void server_log_func(int level, const char *str)
{
	fprintf(stderr, "server|<%d>| %s", level, str);
}

static void client_log_func(int level, const char *str)
{
	fprintf(stderr, "client|<%d>| %s", level, str);
}

static unsigned found_server_name = 0;
static unsigned found_status_req = 0;
static unsigned bare_version = 0;

static int ext_callback(void *ctx, unsigned tls_id, const unsigned char *data, unsigned size)
{
	if (tls_id == 0) { /* server name */
		/* very interesting extension, 4 bytes of sizes
		 * and 1 byte of type. */
		unsigned esize = (data[0] << 8) | data[1];
		assert(esize == strlen(HOSTNAME)+3);

		size -= 2;
		data += 2;

		assert(data[0] == 0);
		data++;
		size--;

		esize = (data[0] << 8) | data[1];

		assert(esize == strlen(HOSTNAME));
		data += 2;
		size -= 2;

		assert(memcmp(data, HOSTNAME, strlen(HOSTNAME)) == 0);
		found_server_name = 1;
	} else if (tls_id == 5) {
		found_status_req = 1;
	} else {
		if (debug)
			success("found extension: %u\n", tls_id);
	}
	return 0;
}

static int handshake_callback(gnutls_session_t session, unsigned int htype,
	unsigned post, unsigned int incoming, const gnutls_datum_t *msg)
{
	int ret;

	if (htype == GNUTLS_HANDSHAKE_CLIENT_HELLO && post) {
		if (bare_version) {
			ret = gnutls_ext_raw_parse(NULL, ext_callback, msg, GNUTLS_EXT_RAW_FLAG_TLS_CLIENT_HELLO);
		} else {
			unsigned pos;
			gnutls_datum_t mmsg;
			assert(msg->size >= HANDSHAKE_SESSION_ID_POS);
			pos = HANDSHAKE_SESSION_ID_POS;
			SKIP8(pos, msg->size);
			SKIP16(pos, msg->size);
			SKIP8(pos, msg->size);

			mmsg.data = &msg->data[pos];
			mmsg.size = msg->size - pos;
			ret = gnutls_ext_raw_parse(NULL, ext_callback, &mmsg, 0);
		}
		assert(ret >= 0);
	}
	return 0;
}

static void client(int fd)
{
	int ret;
	gnutls_certificate_credentials_t x509_cred;
	gnutls_session_t session;

	global_init();

	if (debug) {
		gnutls_global_set_log_function(client_log_func);
		gnutls_global_set_log_level(7);
	}

	gnutls_certificate_allocate_credentials(&x509_cred);

	/* Initialize TLS session
	 */
	gnutls_init(&session, GNUTLS_CLIENT);
	gnutls_handshake_set_timeout(session, 20 * 1000);

	/* Use default priorities */
	gnutls_priority_set_direct(session, "NORMAL:-VERS-ALL:+VERS-TLS1.2", NULL);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, x509_cred);

	gnutls_transport_set_int(session, fd);
	assert(gnutls_server_name_set(session, GNUTLS_NAME_DNS, HOSTNAME, strlen(HOSTNAME))>=0);

	/* Perform the TLS handshake
	 */
	do {
		ret = gnutls_handshake(session);
	}
	while (ret < 0 && gnutls_error_is_fatal(ret) == 0);

	if (ret == GNUTLS_E_UNSUPPORTED_SIGNATURE_ALGORITHM) {
		/* success */
		goto end;
	}

	if (ret < 0) {
		fail("client: Handshake failed: %s\n", gnutls_strerror(ret));
	} else {
		if (debug)
			success("client: Handshake was completed\n");
	}

	if (debug)
		success("client: TLS version is: %s\n",
			gnutls_protocol_get_name
			(gnutls_protocol_get_version(session)));

	gnutls_bye(session, GNUTLS_SHUT_WR);

      end:

	close(fd);

	gnutls_deinit(session);

	gnutls_certificate_free_credentials(x509_cred);

	gnutls_global_deinit();
}


static void server(int fd)
{
	int ret;
	gnutls_session_t session;
	gnutls_certificate_credentials_t x509_cred;

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

	gnutls_init(&session, GNUTLS_SERVER);
	gnutls_handshake_set_timeout(session, 20 * 1000);

	gnutls_handshake_set_hook_function(session, GNUTLS_HANDSHAKE_CLIENT_HELLO,
					   GNUTLS_HOOK_POST,
					   handshake_callback);

	/* avoid calling all the priority functions, since the defaults
	 * are adequate.
	 */
	gnutls_priority_set_direct(session, "NORMAL:-VERS-ALL:+VERS-TLS1.2", NULL);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, x509_cred);

	gnutls_transport_set_int(session, fd);

	do {
		ret = gnutls_handshake(session);
	} while (ret < 0 && gnutls_error_is_fatal(ret) == 0);
	if (ret < 0) {
		/* failure is expected here */
		goto end;
	}

	if (debug)
		success("server: Handshake was completed\n");

	if (debug)
		success("server: TLS version is: %s\n",
			gnutls_protocol_get_name
			(gnutls_protocol_get_version(session)));

	assert(found_server_name != 0);
	assert(found_status_req != 0);

	gnutls_bye(session, GNUTLS_SHUT_WR);

 end:
	close(fd);
	gnutls_deinit(session);

	gnutls_certificate_free_credentials(x509_cred);

	gnutls_global_deinit();

	if (debug)
		success("server: finished\n");
}

static void ch_handler(int sig)
{
	return;
}

static void start(unsigned val)
{
	int fd[2];
	int ret, status = 0;
	pid_t child;

	signal(SIGCHLD, ch_handler);
	signal(SIGPIPE, SIG_IGN);

	bare_version = val;

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
		server(fd[0]);
		waitpid(child, &status, 0);
		check_wait_status(status);
	} else {
		close(fd[0]);
		client(fd[1]);
		exit(0);
	}

	return;
}

void doit(void)
{
	start(0);
	start(1);
}

#endif				/* _WIN32 */
