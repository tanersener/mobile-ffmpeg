/*
 * Copyright (C) 2004-2016 Free Software Foundation, Inc.
 * Copyright (C) 2013 Adam Sampson <ats@offog.org>
 * Copyright (C) 2016-2019 Red Hat, Inc.
 *
 * Author: Simon Josefsson, Nikos Mavrogiannopoulos, Daiki Ueno
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

/* Parts copied from GnuTLS example programs. */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#if defined(_WIN32)

/* socketpair isn't supported on Win32. */
int main(int argc, char **argv)
{
	exit(77);
}

#else

#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#if !defined(_WIN32)
#include <sys/wait.h>
#endif
#include <unistd.h>
#include <gnutls/gnutls.h>
#include <sys/wait.h>
#include <signal.h>
#include <assert.h>
#include "utils.h"
#include "cert-common.h"
#include "virt-time.h"

#define SKIP8(pos, total) { \
	uint8_t _s; \
	if (pos+1 > total) fail("error\n"); \
	_s = msg->data[pos]; \
	if ((size_t)(pos+1+_s) > total) fail("error\n"); \
	pos += 1+_s; \
	}

pid_t child;

/* A very basic TLS client, with anonymous authentication.
 */

#define SESSIONS 2
#define MAX_BUF 5*1024
#define MSG "Hello TLS"

/* 2^13, which is not supported by max_fragment_length */
#define MAX_DATA_SIZE 8192

#define HANDSHAKE_SESSION_ID_POS (2+32)

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "%s |<%d>| %s", child ? "server" : "client", level,
		str);
}

static int ext_callback(void *ctx, unsigned tls_id, const unsigned char *data, unsigned size)
{
	if (tls_id == 28) { /* record size limit */
		uint16_t max_data_size;

		assert(size == 2);
		max_data_size = (data[0] << 8) | data[1];
		if (max_data_size == MAX_DATA_SIZE)
			fail("record_size_limit is not reset: %u == %u\n",
			     max_data_size, MAX_DATA_SIZE);
	}
	return 0;
}

static int handshake_callback(gnutls_session_t session, unsigned int htype,
	unsigned post, unsigned int incoming, const gnutls_datum_t *msg)
{
	int ret;
	unsigned pos;
	gnutls_datum_t mmsg;

	if (!post)
		return 0;

	switch (htype) {
	case GNUTLS_HANDSHAKE_ENCRYPTED_EXTENSIONS:
		ret = gnutls_ext_raw_parse(NULL, ext_callback, msg, 0);
		assert(ret >= 0);
		break;
	case GNUTLS_HANDSHAKE_SERVER_HELLO:
		assert(msg->size >= HANDSHAKE_SESSION_ID_POS);
		pos = HANDSHAKE_SESSION_ID_POS;
		SKIP8(pos, msg->size);
		pos += 3;

		mmsg.data = &msg->data[pos];
		mmsg.size = msg->size - pos;
		ret = gnutls_ext_raw_parse(NULL, ext_callback, &mmsg, 0);
		assert(ret >= 0);
		break;
	default:
		break;
	}
	return 0;
}

static void client(int sds[], const char *prio)
{
	int ret, ii;
	gnutls_session_t session;
	char buffer[MAX_BUF + 1];
	gnutls_certificate_credentials_t clientx509cred;

	/* variables used in session resuming
	 */
	int t;
	gnutls_datum_t session_data = {NULL, 0};

	if (debug) {
		gnutls_global_set_log_function(tls_log_func);
		gnutls_global_set_log_level(4);
	}

	gnutls_certificate_allocate_credentials(&clientx509cred);

	assert(gnutls_certificate_set_x509_key_mem(clientx509cred,
						   &cli_cert, &cli_key,
						   GNUTLS_X509_FMT_PEM) >= 0);

	for (t = 0; t < SESSIONS; t++) {
		int sd = sds[t];

		assert(gnutls_init(&session, GNUTLS_CLIENT)>=0);

		ret = gnutls_priority_set_direct(session, prio, NULL);
		if (ret < 0) {
			fail("prio: %s\n", gnutls_strerror(ret));
		}

		gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, clientx509cred);

		if (t == 0) {
			ret = gnutls_record_set_max_size(session, MAX_DATA_SIZE);
			if (ret < 0)
				fail("gnutls_set_max_size: %s\n", gnutls_strerror(ret));
		}

		if (t > 0) {
			/* if this is not the first time we connect */
			gnutls_session_set_data(session, session_data.data,
						session_data.size);

			gnutls_handshake_set_hook_function(session,
							   GNUTLS_HANDSHAKE_ANY,
							   GNUTLS_HOOK_POST,
							   handshake_callback);
		}

		gnutls_transport_set_int(session, sd);

		/* Perform the TLS handshake
		 */
		gnutls_handshake_set_timeout(session, 20 * 1000);
		do {
			ret = gnutls_handshake(session);
		} while (ret < 0 && gnutls_error_is_fatal(ret) == 0);

		if (ret < 0) {
			fail("client: Handshake failed\n");
			gnutls_perror(ret);
			break;
		} else {
			if (debug)
				success
				    ("client: Handshake was completed\n");
		}

		if (t == 0) {
			/* get the session data size */
			ret =
			    gnutls_session_get_data2(session,
						     &session_data);
			if (ret < 0)
				fail("Getting resume data failed\n");

		} else {	/* the second time we connect */
			/* check if we actually resumed the previous session */
			if (gnutls_session_is_resumed(session) == 0) {
				fail("- Previous session was resumed but NOT expected\n");
			}
		}

		gnutls_record_send(session, MSG, strlen(MSG));

		do {
			ret = gnutls_record_recv(session, buffer, MAX_BUF);
		} while (ret == GNUTLS_E_AGAIN);
		if (ret == 0) {
			if (debug)
				success
				    ("client: Peer has closed the TLS connection\n");
			break;
		} else if (ret < 0) {
			fail("client: Error: %s\n", gnutls_strerror(ret));
		}

		if (debug) {
			printf("- Received %d bytes: ", ret);
			for (ii = 0; ii < ret; ii++) {
				fputc(buffer[ii], stdout);
			}
			fputs("\n", stdout);
		}

		gnutls_bye(session, GNUTLS_SHUT_RDWR);

		close(sd);

		gnutls_deinit(session);
	}
	gnutls_free(session_data.data);

	gnutls_certificate_free_credentials(clientx509cred);
}

/* These are global */
static gnutls_datum_t session_ticket_key = { NULL, 0 };


gnutls_certificate_credentials_t serverx509cred;

static void global_stop(void)
{
	if (debug)
		success("global stop\n");

	gnutls_certificate_free_credentials(serverx509cred);
}

static void server(int sds[], const char *prio)
{
	int t;
	int ret;
	gnutls_session_t session;
	char buffer[MAX_BUF + 1];
	unsigned iflags = GNUTLS_SERVER;

	virt_time_init();

	/* this must be called once in the program, it is mostly for the server.
	 */
	if (debug) {
		gnutls_global_set_log_function(tls_log_func);
		gnutls_global_set_log_level(4);
	}

	gnutls_certificate_allocate_credentials(&serverx509cred);
	assert(gnutls_certificate_set_x509_key_mem(serverx509cred,
		&server_cert, &server_key, GNUTLS_X509_FMT_PEM) >= 0);

	gnutls_session_ticket_key_generate(&session_ticket_key);

	for (t = 0; t < SESSIONS; t++) {
		int sd = sds[t];

		assert(gnutls_init(&session, iflags) >= 0);

		/* avoid calling all the priority functions, since the defaults
		 * are adequate.
		 */
		assert(gnutls_priority_set_direct(session, prio, NULL) >= 0);


		gnutls_session_ticket_enable_server(session,
						    &session_ticket_key);

		gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, serverx509cred);
		gnutls_transport_set_int(session, sd);
		gnutls_handshake_set_timeout(session, 20 * 1000);

		do {
			ret = gnutls_handshake(session);
		} while (ret < 0 && gnutls_error_is_fatal(ret) == 0);
		if (ret < 0) {
			close(sd);
			gnutls_deinit(session);
			kill(child, SIGTERM);
			fail("server: Handshake has failed (%s)\n\n",
			     gnutls_strerror(ret));
			return;
		}
		if (debug)
			success("server: Handshake was completed\n");

		if (t > 0) {
			ret = gnutls_session_is_resumed(session);
			if (ret == 0) {
				fail("server: session_is_resumed error (%d)\n", t);
			}
		}

		/* see the Getting peer's information example */
		/* print_info(session); */

		for (;;) {
			memset(buffer, 0, MAX_BUF + 1);
			ret = gnutls_record_recv(session, buffer, MAX_BUF);

			if (ret == 0) {
				if (debug)
					success
					    ("server: Peer has closed the GnuTLS connection\n");
				break;
			} else if (ret < 0) {
				kill(child, SIGTERM);
				fail("server: Received corrupted data(%d). Closing...\n", ret);
				break;
			} else if (ret > 0) {
				/* echo data back to the client
				 */
				gnutls_record_send(session, buffer,
						   strlen(buffer));
			}
		}
		/* do not wait for the peer to close the connection.
		 */
		gnutls_bye(session, GNUTLS_SHUT_WR);

		close(sd);

		gnutls_deinit(session);
	}

	gnutls_free(session_ticket_key.data);
	session_ticket_key.data = NULL;

	if (debug)
		success("server: finished\n");
}

static void run(const char *prio)
{
	int client_sds[SESSIONS], server_sds[SESSIONS];
	int j;
	int err;

	signal(SIGCHLD, SIG_IGN);
	signal(SIGPIPE, SIG_IGN);

	for (j = 0; j < SESSIONS; j++) {
		int sockets[2];

		err = socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
		if (err == -1) {
			perror("socketpair");
			fail("socketpair failed\n");
			return;
		}

		server_sds[j] = sockets[0];
		client_sds[j] = sockets[1];
	}

	child = fork();
	if (child < 0) {
		perror("fork");
		fail("fork\n");
		return;
	}

	if (child) {
		int status = 0;
		/* parent */
		for (j = 0; j < SESSIONS; j++)
			close(client_sds[j]);
		server(server_sds, prio);

		waitpid(child, &status, 0);
		check_wait_status(status);
		global_stop();
	} else {
		for (j = 0; j < SESSIONS; j++)
			close(server_sds[j]);
		client(client_sds, prio);
		exit(0);
	}
}

void doit(void)
{
	run("NORMAL:-VERS-ALL:+VERS-TLS1.2");
	run("NORMAL:-VERS-ALL:+VERS-TLS1.3");
}

#endif				/* _WIN32 */
