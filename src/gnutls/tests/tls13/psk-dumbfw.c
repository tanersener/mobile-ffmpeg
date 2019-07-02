/*
 * Copyright (C) 2004-2012 Free Software Foundation, Inc.
 * Copyright (C) 2013 Adam Sampson <ats@offog.org>
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
#include <stdint.h>

#if defined(_WIN32)

/* socketpair isn't supported on Win32. */
int main(int argc, char **argv)
{
	exit(77);
}

#else

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#if !defined(_WIN32)
#include <sys/wait.h>
#endif
#include <unistd.h>
#include <gnutls/gnutls.h>
#include <assert.h>
#include <signal.h>

#include "tls13/ext-parse.h"

#include "utils.h"

/* Tests whether the pre-shared key extension will always be last
 * even if the dumbfw extension is present.
 */

const char *side = "";

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "%s|<%d>| %s", side, level, str);
}

#define MAX_BUF 1024
#define MSG "Hello TLS"

static void client(int sd, const char *prio)
{
	int ret, ii;
	gnutls_session_t session;
	char buffer[MAX_BUF + 1];
	gnutls_psk_client_credentials_t pskcred;
	/* Need to enable anonymous KX specifically. */
	const gnutls_datum_t key = { (void *) "DEADBEEF", 8 };

	global_init();
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(6);

	side = "client";

	gnutls_psk_allocate_client_credentials(&pskcred);
	gnutls_psk_set_client_credentials(pskcred, "test", &key,
					  GNUTLS_PSK_KEY_HEX);

	assert(gnutls_init(&session, GNUTLS_CLIENT|GNUTLS_KEY_SHARE_TOP)>=0);

	assert(gnutls_priority_set_direct(session, prio, NULL)>=0);
	assert(gnutls_credentials_set(session, GNUTLS_CRD_PSK, pskcred)>=0);

	gnutls_transport_set_int(session, sd);

	/* Perform the TLS handshake
	 */
	ret = gnutls_handshake(session);

	if (ret < 0) {
		fail("client: Handshake failed\n");
		gnutls_perror(ret);
		goto end;
	} else {
		if (debug)
			success("client: Handshake was completed\n");
	}

	assert(gnutls_record_send(session, MSG, strlen(MSG))>=0);

	ret = gnutls_record_recv(session, buffer, MAX_BUF);
	if (ret == 0) {
		if (debug)
			success
			    ("client: Peer has closed the TLS connection\n");
		goto end;
	} else if (ret < 0) {
		fail("client: Error: %s\n", gnutls_strerror(ret));
		goto end;
	}

	if (debug) {
		printf("- Received %d bytes: ", ret);
		for (ii = 0; ii < ret; ii++) {
			fputc(buffer[ii], stdout);
		}
		fputs("\n", stdout);
	}

	gnutls_bye(session, GNUTLS_SHUT_RDWR);

      end:

	close(sd);

	gnutls_deinit(session);

	gnutls_psk_free_client_credentials(pskcred);

	gnutls_global_deinit();
}

static int
pskfunc(gnutls_session_t session, const char *username,
	gnutls_datum_t * key)
{
	if (debug)
		printf("psk: username %s\n", username);
	key->data = gnutls_malloc(4);
	key->data[0] = 0xDE;
	key->data[1] = 0xAD;
	key->data[2] = 0xBE;
	key->data[3] = 0xEF;
	key->size = 4;
	return 0;
}

#define EXT_CLIENTHELLO_PADDING 21
#define EXT_PRE_SHARED_KEY 41

struct ctx_st {
	unsigned long pos;
	void *base;
};

static
void check_ext_pos(void *priv, gnutls_datum_t *msg)
{
	struct ctx_st *ctx = priv;

	ctx->pos = (ptrdiff_t)((ptrdiff_t)msg->data - (ptrdiff_t)ctx->base);
}

static int client_hello_callback(gnutls_session_t session, unsigned int htype,
	unsigned post, unsigned int incoming, const gnutls_datum_t *msg)
{
	unsigned long pos_psk;
	unsigned long pos_pad;

	if (htype == GNUTLS_HANDSHAKE_CLIENT_HELLO && post == GNUTLS_HOOK_POST) {
		struct ctx_st ctx;

		ctx.base = msg->data;
		if (find_client_extension(msg, EXT_CLIENTHELLO_PADDING, &ctx, check_ext_pos) == 0)
			fail("Could not find dumbfw/client hello padding extension!\n");
		pos_pad = ctx.pos;

		ctx.base = msg->data;
		if (find_client_extension(msg, EXT_PRE_SHARED_KEY, &ctx, check_ext_pos) == 0)
			fail("Could not find psk extension!\n");
		pos_psk = ctx.pos;

		if (pos_psk < pos_pad) {
			fail("The dumbfw extension was sent after pre-shared key!\n");
		}

		/* check if we are the last extension in general */
		if (!is_client_extension_last(msg, EXT_PRE_SHARED_KEY)) {
			fail("pre-shared key extension wasn't the last one!\n");
		}
	}

	return 0;
}


static void server(int sd, const char *prio)
{
	gnutls_psk_server_credentials_t server_pskcred;
	int ret;
	gnutls_session_t session;
	char buffer[MAX_BUF + 1];

	global_init();
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(6);

	side = "server";


	assert(gnutls_psk_allocate_server_credentials(&server_pskcred)>=0);
	gnutls_psk_set_server_credentials_function(server_pskcred,
						   pskfunc);

	assert(gnutls_init(&session, GNUTLS_SERVER)>=0);

	assert(gnutls_priority_set_direct(session, prio, NULL)>=0);
	gnutls_credentials_set(session, GNUTLS_CRD_PSK, server_pskcred);

	gnutls_handshake_set_hook_function(session, GNUTLS_HANDSHAKE_ANY,
					   GNUTLS_HOOK_BOTH,
					   client_hello_callback);

	gnutls_transport_set_int(session, sd);
	ret = gnutls_handshake(session);
	if (ret < 0) {
		close(sd);
		gnutls_deinit(session);
		fail("server: Handshake has failed (%s)\n\n",
		     gnutls_strerror(ret));
		return;
	}
	if (debug)
		success("server: Handshake was completed\n");

	for (;;) {
		memset(buffer, 0, MAX_BUF + 1);
		gnutls_record_set_timeout(session, 10000);
		ret = gnutls_record_recv(session, buffer, MAX_BUF);

		if (ret == 0) {
			if (debug)
				success
				    ("server: Peer has closed the GnuTLS connection\n");
			break;
		} else if (ret < 0) {
			fail("server: Received corrupted data(%d). Closing...\n", ret);
			break;
		} else if (ret > 0) {
			/* echo data back to the client
			 */
			gnutls_record_send(session, buffer,
					   strlen(buffer));
		}
	}

	gnutls_bye(session, GNUTLS_SHUT_WR);

	close(sd);
	gnutls_deinit(session);

	gnutls_psk_free_server_credentials(server_pskcred);

	gnutls_global_deinit();

	if (debug)
		success("server: finished\n");
}

static void ch_handler(int sig)
{
	int status = 0;
	wait(&status);
	check_wait_status(status);
	return;
}


static
void run_test(const char *prio)
{
	pid_t child;
	int err;
	int sockets[2];

	signal(SIGCHLD, ch_handler);
	signal(SIGPIPE, SIG_IGN);

	success("trying with %s\n", prio);

	err = socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
	if (err == -1) {
		perror("socketpair");
		fail("socketpair failed\n");
		return;
	}

	child = fork();
	if (child < 0) {
		perror("fork");
		fail("fork");
		return;
	}

	if (child) {
		int status = 0;
		/* parent */
		close(sockets[1]);
		server(sockets[0], prio);
		wait(&status);
		check_wait_status(status);
	} else {
		close(sockets[0]);
		client(sockets[1], prio);
		exit(0);
	}
}

void doit(void)
{
	run_test("NORMAL:-VERS-ALL:+VERS-TLS1.3:+VERS-TLS1.2:+PSK:%DUMBFW:-GROUP-ALL:+GROUP-FFDHE2048");
}

#endif				/* _WIN32 */
