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

#include "cert-common.h"
#include "utils.h"
#include "tls13/ext-parse.h"

/* This program tests the Key Share behavior in Client Hello,
 * and whether the flags to gnutls_init for key share are followed.
 */

const char *testname = "";

#define myfail(fmt, ...) \
	fail("%s: "fmt, testname, ##__VA_ARGS__)

static void server_log_func(int level, const char *str)
{
	fprintf(stderr, "server|<%d>| %s", level, str);
}

static void client_log_func(int level, const char *str)
{
	fprintf(stderr, "client|<%d>| %s", level, str);
}



#define MAX_BUF 1024

static void client(int fd, unsigned flag, const char *prio)
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
	gnutls_init(&session, GNUTLS_CLIENT|flag);

	gnutls_handshake_set_timeout(session, 20 * 1000);

	ret = gnutls_priority_set_direct(session, prio, NULL);
	if (ret < 0)
		myfail("cannot set TLS 1.3 priorities\n");

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

	close(fd);

	gnutls_deinit(session);

	gnutls_certificate_free_credentials(x509_cred);

	gnutls_global_deinit();
}

unsigned int tls_id_to_group[] = {
	[23] = GNUTLS_GROUP_SECP256R1,
	[24] = GNUTLS_GROUP_SECP384R1,
	[29] = GNUTLS_GROUP_X25519,
	[0x100] = GNUTLS_GROUP_FFDHE2048,
	[0x101] = GNUTLS_GROUP_FFDHE3072
};


#define TLS_EXT_KEY_SHARE 51

typedef struct ctx_st {
	gnutls_group_t group;
	unsigned ngroups;
} ctx_st;

static
void check_ks_contents(void *priv, gnutls_datum_t *msg)
{
	ctx_st *ctx;
	int len;
	gnutls_session_t session = priv;
	int pos;
	unsigned total = 0, id;
	unsigned found = 0;

	ctx = gnutls_session_get_ptr(session);

	len = (msg->data[0] << 8) | msg->data[1];
	if (len+2 != (int)msg->size)
		myfail("mismatch in length (%d vs %d)!\n", len, (int)msg->size);

	pos = 2;

	while((unsigned)pos < msg->size) {
		id = (msg->data[pos] << 8) | msg->data[pos+1];
		pos += 2;
		len -= 2;

		if (debug)
			success("found group: %u\n", id);
		if (id < sizeof(tls_id_to_group)/sizeof(tls_id_to_group[0])) {
			if (tls_id_to_group[id] == ctx->group)
				found = 1;
		}
		total++;

		SKIP16(pos, msg->size);
	}

	if (total != ctx->ngroups) {
		myfail("found %d groups, expected %d\n", total, ctx->ngroups);
	}

	if (found == 0) {
		myfail("did not find group %s\n", gnutls_group_get_name(ctx->group));
	}
}

static int client_hello_callback(gnutls_session_t session, unsigned int htype,
	unsigned post, unsigned int incoming, const gnutls_datum_t *msg)
{
	if (htype == GNUTLS_HANDSHAKE_CLIENT_HELLO && post == GNUTLS_HOOK_POST) {
		if (find_client_extension(msg, TLS_EXT_KEY_SHARE, session, check_ks_contents) == 0)
			fail("Could not find key share extension!\n");
	}

	return 0;
}

static void server(int fd, gnutls_group_t exp_group, unsigned ngroups)
{
	int ret;
	char buffer[MAX_BUF + 1];
	gnutls_session_t session;
	gnutls_certificate_credentials_t x509_cred;
	ctx_st ctx;

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

	gnutls_init(&session, GNUTLS_SERVER);

	gnutls_handshake_set_timeout(session, 20 * 1000);
	gnutls_handshake_set_hook_function(session, GNUTLS_HANDSHAKE_ANY,
					   GNUTLS_HOOK_BOTH,
					   client_hello_callback);
	ctx.group = exp_group;
	ctx.ngroups = ngroups;
	gnutls_session_set_ptr(session, &ctx);

	/* avoid calling all the priority functions, since the defaults
	 * are adequate.
	 */
	gnutls_priority_set_direct(session, "NORMAL:+VERS-TLS1.3", NULL);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, x509_cred);

	gnutls_transport_set_int(session, fd);

	do {
		ret = gnutls_handshake(session);
		if (ret == GNUTLS_E_INTERRUPTED) { /* expected */
			break;
		}
	} while (ret < 0 && gnutls_error_is_fatal(ret) == 0);

	if (ret < 0)
		myfail("handshake error: %s\n", gnutls_strerror(ret));

	if (gnutls_group_get(session) != exp_group)
		myfail("group doesn't match the expected: %s\n", gnutls_group_get_name(gnutls_group_get(session)));

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

static void start(const char *name, const char *prio, unsigned flag, gnutls_group_t group, unsigned ngroups)
{
	int fd[2];
	int ret;
	pid_t child;

	signal(SIGCHLD, ch_handler);
	signal(SIGPIPE, SIG_IGN);

	testname = name;
	success("== test %s ==\n", testname);

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
		server(fd[0], group, ngroups);
		kill(child, SIGTERM);
	} else {
		close(fd[0]);
		client(fd[1], flag, prio);
		exit(0);
	}
}

void doit(void)
{
	start("single group: default secp256r1", "NORMAL:-VERS-ALL:+VERS-TLS1.3", GNUTLS_KEY_SHARE_TOP, GNUTLS_GROUP_SECP256R1, 1);
	start("single group: secp256r1", "NORMAL:-VERS-ALL:+VERS-TLS1.3:-GROUP-ALL:+GROUP-SECP256R1:+GROUP-SECP384R1:+GROUP-X25519:+GROUP-FFDHE2048", GNUTLS_KEY_SHARE_TOP, GNUTLS_GROUP_SECP256R1, 1);
	start("single group: x25519", "NORMAL:-VERS-ALL:+VERS-TLS1.3:-GROUP-ALL:+GROUP-X25519:+GROUP-SECP256R1:+GROUP-SECP384R1:+GROUP-FFDHE2048", GNUTLS_KEY_SHARE_TOP, GNUTLS_GROUP_X25519, 1);

	/* unfortunately we strictly follow the rfc7919 RFC and we prioritize groups
	 * based on ciphersuite listing as well. To prioritize the FFDHE groups we need
	 * to prioritize the non-EC ciphersuites first. */
	start("single group: ffdhe2048", "NORMAL:-KX-ALL:+DHE-RSA:+ECDHE-RSA:-VERS-ALL:+VERS-TLS1.3:-GROUP-ALL:+GROUP-FFDHE2048:+GROUP-SECP256R1:+GROUP-SECP384R1:+GROUP-X25519:+GROUP-FFDHE3072", GNUTLS_KEY_SHARE_TOP, GNUTLS_GROUP_FFDHE2048, 1);

	start("two groups: default secp256r1", "NORMAL:-VERS-ALL:+VERS-TLS1.3", GNUTLS_KEY_SHARE_TOP2, GNUTLS_GROUP_SECP256R1, 2);
	start("two groups: secp256r1", "NORMAL:-VERS-ALL:+VERS-TLS1.3:-GROUP-ALL:+GROUP-SECP256R1:+GROUP-SECP384R1:+GROUP-X25519:+GROUP-FFDHE2048", GNUTLS_KEY_SHARE_TOP2, GNUTLS_GROUP_SECP256R1, 2);
	start("two groups: x25519", "NORMAL:-VERS-ALL:+VERS-TLS1.3:-GROUP-ALL:+GROUP-X25519:+GROUP-SECP256R1:+GROUP-SECP384R1:+GROUP-FFDHE2048", GNUTLS_KEY_SHARE_TOP2, GNUTLS_GROUP_X25519, 2);
	start("two groups: ffdhe2048", "NORMAL:-KX-ALL:+DHE-RSA:+ECDHE-RSA:-VERS-ALL:+VERS-TLS1.3:-GROUP-ALL:+GROUP-FFDHE2048:+GROUP-SECP256R1:+GROUP-SECP384R1:+GROUP-X25519:+GROUP-FFDHE3072", GNUTLS_KEY_SHARE_TOP2, GNUTLS_GROUP_FFDHE2048, 2);

	start("three groups: default secp256r1", "NORMAL:-VERS-ALL:+VERS-TLS1.3", GNUTLS_KEY_SHARE_TOP3, GNUTLS_GROUP_SECP256R1, 3);
	start("three groups: secp256r1", "NORMAL:-VERS-ALL:+VERS-TLS1.3:-GROUP-ALL:+GROUP-SECP256R1:+GROUP-SECP384R1:+GROUP-X25519:+GROUP-FFDHE2048", GNUTLS_KEY_SHARE_TOP3, GNUTLS_GROUP_SECP256R1, 3);
	start("three groups: x25519", "NORMAL:-VERS-ALL:+VERS-TLS1.3:-GROUP-ALL:+GROUP-X25519:+GROUP-SECP256R1:+GROUP-SECP384R1:+GROUP-FFDHE2048", GNUTLS_KEY_SHARE_TOP3, GNUTLS_GROUP_X25519, 3);
	start("three groups: ffdhe2048", "NORMAL:-KX-ALL:+DHE-RSA:+ECDHE-RSA:-VERS-ALL:+VERS-TLS1.3:-GROUP-ALL:+GROUP-FFDHE2048:+GROUP-SECP256R1:+GROUP-SECP384R1:+GROUP-X25519:+GROUP-FFDHE3072", GNUTLS_KEY_SHARE_TOP3, GNUTLS_GROUP_FFDHE2048, 3);

	/* test default behavior */
	start("default groups(2): default secp256r1", "NORMAL:-VERS-ALL:+VERS-TLS1.3", 0, GNUTLS_GROUP_SECP256R1, 2);
	start("default groups(2): secp256r1", "NORMAL:-VERS-ALL:+VERS-TLS1.3:-GROUP-ALL:+GROUP-SECP256R1:+GROUP-SECP384R1:+GROUP-X25519:+GROUP-FFDHE2048", 0, GNUTLS_GROUP_SECP256R1, 2);
	start("default groups(2): x25519", "NORMAL:-VERS-ALL:+VERS-TLS1.3:-GROUP-ALL:+GROUP-X25519:+GROUP-SECP256R1:+GROUP-SECP384R1:+GROUP-FFDHE2048", 0, GNUTLS_GROUP_X25519, 2);
	start("default groups(2): ffdhe2048", "NORMAL:-KX-ALL:+DHE-RSA:+ECDHE-RSA:-VERS-ALL:+VERS-TLS1.3:-GROUP-ALL:+GROUP-FFDHE2048:+GROUP-SECP256R1:+GROUP-SECP384R1:+GROUP-X25519:+GROUP-FFDHE3072", 0, GNUTLS_GROUP_FFDHE2048, 2);
}

#endif				/* _WIN32 */
