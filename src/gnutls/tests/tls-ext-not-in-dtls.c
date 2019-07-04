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

#include <stdint.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <gnutls/gnutls.h>
#include <gnutls/dtls.h>
#include <signal.h>
#include <string.h>
#include <assert.h>

#include "utils.h"
#include "cert-common.h"

enum {
TEST_DEF_HANDHAKE,
TEST_CUSTOM_EXT
};

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

static int ext_send(gnutls_session_t session, gnutls_buffer_t extdata)
{
	gnutls_buffer_append_data(extdata, "\xff", 1);
	return 0;
}

static int ext_recv(gnutls_session_t session, const unsigned char *buf, size_t buflen)
{
	return 0;
}

#define TLS_EXT_IMPL_DTLS 0xfeee
#define TLS_EXT_EXPL_TLS 0xfeea

static void client(int fd, int type)
{
	int ret;
	gnutls_certificate_credentials_t x509_cred;
	gnutls_session_t session;

	assert(gnutls_global_init() >= 0);

	gnutls_global_set_log_function(client_log_func);

	assert(gnutls_certificate_allocate_credentials(&x509_cred) >= 0);

	assert(gnutls_init(&session, GNUTLS_CLIENT|GNUTLS_DATAGRAM) >= 0);

	if (type == TEST_CUSTOM_EXT) {
		assert(gnutls_session_ext_register(session, "implicit-dtls", TLS_EXT_IMPL_DTLS, GNUTLS_EXT_TLS, ext_recv, ext_send, NULL, NULL, NULL, GNUTLS_EXT_FLAG_CLIENT_HELLO|GNUTLS_EXT_FLAG_TLS12_SERVER_HELLO|GNUTLS_EXT_FLAG_TLS13_SERVER_HELLO)>=0);
		assert(gnutls_session_ext_register(session, "explicit-tls", TLS_EXT_EXPL_TLS, GNUTLS_EXT_TLS, ext_recv, ext_send, NULL, NULL, NULL, GNUTLS_EXT_FLAG_CLIENT_HELLO|GNUTLS_EXT_FLAG_TLS12_SERVER_HELLO|GNUTLS_EXT_FLAG_TLS13_SERVER_HELLO|GNUTLS_EXT_FLAG_TLS)>=0);
	}

	gnutls_handshake_set_timeout(session, 20 * 1000);

	assert(gnutls_priority_set_direct(session, "NORMAL:-VERS-TLS-ALL:+VERS-TLS1.3:+VERS-TLS1.2:+VERS-TLS1.0", NULL) >= 0);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, x509_cred);

	gnutls_transport_set_int(session, fd);

	do {
		ret = gnutls_handshake(session);
	}
	while (ret < 0 && gnutls_error_is_fatal(ret) == 0);

	if (ret < 0)
		fail("handshake: %s\n", gnutls_strerror(ret));

	close(fd);

	gnutls_deinit(session);

	gnutls_certificate_free_credentials(x509_cred);

	gnutls_global_deinit();
}

#define TLS_EXT_KEY_SHARE 51
#define TLS_EXT_POST_HANDSHAKE 49

struct ext_ctx_st {
	int extno;
	int found;
};

static int parse_ext(void *ctx, unsigned tls_id, const unsigned char *data, unsigned data_size)
{
	struct ext_ctx_st *s = ctx;

	if (s->extno == (int)tls_id)
		s->found = 1;

	return 0;
}

static unsigned find_client_extension(const gnutls_datum_t *msg, int extno)
{
	int ret;
	struct ext_ctx_st s;

	memset(&s, 0, sizeof(s));
	s.extno = extno;

	ret = gnutls_ext_raw_parse(&s, parse_ext, msg, GNUTLS_EXT_RAW_FLAG_DTLS_CLIENT_HELLO);
	assert(ret>=0);

	if (s.found)
		return 1;

	return 0;
}

static int hellos_callback(gnutls_session_t session, unsigned int htype,
	unsigned post, unsigned int incoming, const gnutls_datum_t *msg)
{
	int *type;

	if (htype != GNUTLS_HANDSHAKE_CLIENT_HELLO || post != GNUTLS_HOOK_PRE)
		return 0;

	type = gnutls_session_get_ptr(session);

	if (find_client_extension(msg, TLS_EXT_KEY_SHARE))
		fail("Key share extension seen in client hello!\n");

	if (find_client_extension(msg, TLS_EXT_POST_HANDSHAKE))
		fail("Key share extension seen in client hello!\n");

	if (*type == TEST_CUSTOM_EXT) {
		if (!find_client_extension(msg, TLS_EXT_IMPL_DTLS))
			fail("Implicit DTLS extension not seen in client hello!\n");

		if (find_client_extension(msg, TLS_EXT_EXPL_TLS))
			fail("Explicit TLS extension seen in client hello!\n");
	}

	return 0;
}

static void server(int fd, int type)
{
	int ret;
	gnutls_session_t session;
	gnutls_certificate_credentials_t x509_cred;

	assert(gnutls_global_init() >= 0);

	gnutls_global_set_log_function(server_log_func);

	assert(gnutls_certificate_allocate_credentials(&x509_cred) >= 0);
	assert(gnutls_certificate_set_x509_key_mem(x509_cred, &server_cert,
					    &server_key,
					    GNUTLS_X509_FMT_PEM) >= 0);

	assert(gnutls_init(&session, GNUTLS_SERVER|GNUTLS_POST_HANDSHAKE_AUTH|GNUTLS_DATAGRAM) >= 0);

	gnutls_handshake_set_timeout(session, 20 * 1000);
	gnutls_handshake_set_hook_function(session, GNUTLS_HANDSHAKE_ANY,
					   GNUTLS_HOOK_BOTH,
					   hellos_callback);

	gnutls_session_set_ptr(session, &type);
	assert(gnutls_priority_set_direct(session, "NORMAL:-VERS-TLS-ALL:+VERS-TLS1.3:+VERS-TLS1.2", NULL) >= 0);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, x509_cred);

	gnutls_transport_set_int(session, fd);

	do {
		ret = gnutls_handshake(session);
	} while (ret < 0 && gnutls_error_is_fatal(ret) == 0);

	if (ret != 0)
		fail("handshake failed: %s\n", gnutls_strerror(ret));

	close(fd);
	gnutls_deinit(session);

	gnutls_certificate_free_credentials(x509_cred);

	gnutls_global_deinit();
}

static void ch_handler(int sig)
{
	int status;
	wait(&status);
	check_wait_status(status);
	return;
}

static
void start(const char *name, int type)
{
	int fd[2];
	int ret;
	pid_t child;

	signal(SIGCHLD, ch_handler);
	success("%s\n", name);

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
		server(fd[0], type);
		kill(child, SIGTERM);
	} else {
		close(fd[0]);
		client(fd[1], type);
		exit(0);
	}

}

void doit(void) {
	start("check default extensions", TEST_DEF_HANDHAKE);
	start("check registered extensions", TEST_CUSTOM_EXT);
}

#endif				/* _WIN32 */
