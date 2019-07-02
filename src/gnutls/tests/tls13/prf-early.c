/*
 * Copyright (C) 2015-2019 Red Hat, Inc.
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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#if !defined(__linux__) || !defined(__GNUC__)

int main(int argc, char **argv)
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
#include <gnutls/crypto.h>

#include "cert-common.h"
#include "utils.h"
#include "virt-time.h"

static void terminate(void);

#define SESSIONS 2
#define MAX_BUF 5*1024
#define MSG "Hello TLS"

extern unsigned int _gnutls_global_version;

/* This program tests whether the gnutls_prf() works as
 * expected.
 */

static void server_log_func(int level, const char *str)
{
	fprintf(stderr, "server|<%d>| %s", level, str);
}

static void client_log_func(int level, const char *str)
{
	fprintf(stderr, "client|<%d>| %s", level, str);
}

/* These are global */
static pid_t child;

static const
gnutls_datum_t hrnd = {(void*)"\x00\x02\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 32};
static const
gnutls_datum_t hsrnd = {(void*)"\x00\x05\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 32};

static int gnutls_rnd_works;

int __attribute__ ((visibility ("protected")))
gnutls_rnd(gnutls_rnd_level_t level, void *data, size_t len)
{
	gnutls_rnd_works = 1;

	memset(data, 0xff, len);

	/* Flip the first byte to avoid infinite loop in the RSA
	 * blinding code of Nettle */
	if (len > 0)
		memset(data, 0x0, 1);
	return 0;
}

static gnutls_datum_t session_ticket_key = { NULL, 0 };

static void dump(const char *name, const uint8_t *data, unsigned data_size)
{
	unsigned i;

	fprintf(stderr, "%s", name);
	for (i=0;i<data_size;i++)
		fprintf(stderr, "\\x%.2x", (unsigned)data[i]);
	fprintf(stderr, "\n");
}

#define TRY(label_size, label, extra_size, extra, size, exp) \
	{ \
	ret = gnutls_prf_early(session, label_size, label, extra_size, extra, size, \
			 (void*)key_material); \
	if (ret < 0) { \
		fprintf(stderr, "gnutls_prf_early: error in %d\n", __LINE__); \
		gnutls_perror(ret); \
		exit(1); \
	} \
	if (memcmp(key_material, exp, size) != 0) { \
		fprintf(stderr, "gnutls_prf_early: output doesn't match for '%s'\n", label); \
		dump("got ", key_material, size); \
		dump("expected ", exp, size); \
		exit(1); \
	} \
	}

#define KEY_EXP_VALUE "\xc0\x1e\xc2\xa4\xb7\xb4\x04\xaa\x91\x5d\xaf\xe8\xf7\x4d\x19\xdf\xd0\xe6\x08\xd6\xb4\x3b\xcf\xca\xc9\x32\x75\x3b\xe3\x11\x19\xb1\xac\x68"
#define HELLO_VALUE "\x77\xdb\x10\x0b\xe8\xd0\xb9\x38\xbc\x49\xe6\xbe\xf2\x47\x2a\xcc\x6b\xea\xce\x85\x04\xd3\x9e\xd8\x06\x16\xad\xff\xcd\xbf\x4b"
#define CONTEXT_VALUE "\xf2\x17\x9f\xf2\x66\x56\x87\x66\xf9\x5c\x8a\xd7\x4e\x1d\x46\xee\x0e\x44\x41\x4c\xcd\xac\xcb\xc0\x31\x41\x2a\xb6\xd7\x01\x62"
#define NULL_CONTEXT_VALUE "\xcd\x79\x07\x93\xeb\x96\x07\x3e\xec\x78\x90\x89\xf7\x16\x42\x6d\x27\x87\x56\x7c\x7b\x60\x2b\x20\x44\xd1\xea\x0c\x89\xfb\x8b"

static int handshake_callback_called;

static int handshake_callback(gnutls_session_t session, unsigned int htype,
			      unsigned post, unsigned int incoming, const gnutls_datum_t *msg)
{
	unsigned char key_material[512];
	int ret;

	assert(post == GNUTLS_HOOK_POST);

	handshake_callback_called++;

	TRY(13, "key expansion", 0, NULL, 34, (uint8_t*)KEY_EXP_VALUE);
	TRY(6, "hello", 0, NULL, 31, (uint8_t*)HELLO_VALUE);
	TRY(7, "context", 5, "abcd\xfa", 31, (uint8_t*)CONTEXT_VALUE);
	TRY(12, "null-context", 0, "", 31, (uint8_t*)NULL_CONTEXT_VALUE);

	return 0;
}

static void client(int sds[])
{
	gnutls_session_t session;
	int ret, ii;
	gnutls_certificate_credentials_t clientx509cred;
	const char *err;
	int t;
	gnutls_datum_t session_data = {NULL, 0};
	char buffer[MAX_BUF + 1];

	global_init();

	virt_time_init();

	if (debug) {
		gnutls_global_set_log_function(client_log_func);
		gnutls_global_set_log_level(4711);
	}

	gnutls_certificate_allocate_credentials(&clientx509cred);

	for (t = 0; t < SESSIONS; t++) {
		/* Initialize TLS session
		 */
		gnutls_init(&session, GNUTLS_CLIENT);

		/* Use default priorities */
		ret = gnutls_priority_set_direct(session,
						 "NONE:+VERS-TLS1.3:+AES-256-GCM:+AEAD:+SIGN-RSA-PSS-RSAE-SHA384:+GROUP-SECP256R1",
						 &err);
		if (ret < 0) {
			fail("client: priority set failed (%s): %s\n",
			     gnutls_strerror(ret), err);
			exit(1);
		}

		ret = gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE,
					     clientx509cred);
		if (ret < 0)
			exit(1);

		gnutls_handshake_set_random(session, &hrnd);
		gnutls_transport_set_int(session, sds[t]);

		if (t > 0) {
			gnutls_session_set_data(session, session_data.data,
						session_data.size);
			gnutls_handshake_set_hook_function(session, GNUTLS_HANDSHAKE_CLIENT_HELLO,
							   GNUTLS_HOOK_POST,
							   handshake_callback);
		}

		/* Perform the TLS handshake
		 */
		do {
			ret = gnutls_handshake(session);
		}
		while (ret < 0 && gnutls_error_is_fatal(ret) == 0);

		if (ret < 0) {
			fail("client: Handshake failed: %s\n", strerror(ret));
			exit(1);
		} else {
			if (debug)
				success("client: Handshake was completed\n");
		}

		if (debug)
			success("client: TLS version is: %s\n",
				gnutls_protocol_get_name
				(gnutls_protocol_get_version(session)));

		ret = gnutls_cipher_get(session);
		if (ret != GNUTLS_CIPHER_AES_256_GCM) {
			fprintf(stderr, "negotiated unexpected cipher: %s\n", gnutls_cipher_get_name(ret));
			exit(1);
		}

		ret = gnutls_mac_get(session);
		if (ret != GNUTLS_MAC_AEAD) {
			fprintf(stderr, "negotiated unexpected mac: %s\n", gnutls_mac_get_name(ret));
			exit(1);
		}

		if (t == 0) {
			/* get the session data size */
			ret =
			    gnutls_session_get_data2(session,
						     &session_data);
			if (ret < 0)
				fail("Getting resume data failed\n");

			if (handshake_callback_called != 0)
				fail("client: handshake callback is called\n");
		} else {
			if (handshake_callback_called != t)
				fail("client: handshake callback is not called\n");
		}

		gnutls_record_send(session, MSG, strlen(MSG));

		do {
			ret = gnutls_record_recv(session, buffer, MAX_BUF);
		} while (ret == GNUTLS_E_AGAIN);
		if (ret == 0) {
			if (debug)
				success
				    ("client: Peer has closed the TLS connection\n");
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

		gnutls_bye(session, GNUTLS_SHUT_WR);

		close(sds[t]);

		gnutls_deinit(session);
	}

	gnutls_free(session_data.data);
	gnutls_certificate_free_credentials(clientx509cred);

	gnutls_global_deinit();
}

static void terminate(void)
{
	int status = 0;

	kill(child, SIGTERM);
	wait(&status);
	exit(1);
}

static void server(int sds[])
{
	int ret;
	gnutls_session_t session;
	gnutls_certificate_credentials_t serverx509cred;
	int t;
	char buffer[MAX_BUF + 1];

	/* this must be called once in the program
	 */
	global_init();

	virt_time_init();

	if (debug) {
		gnutls_global_set_log_function(server_log_func);
		gnutls_global_set_log_level(4711);
	}

	gnutls_certificate_allocate_credentials(&serverx509cred);

	gnutls_session_ticket_key_generate(&session_ticket_key);

	for (t = 0; t < SESSIONS; t++) {
		gnutls_init(&session, GNUTLS_SERVER);

		gnutls_session_ticket_enable_server(session,
						    &session_ticket_key);

		/* avoid calling all the priority functions, since the defaults
		 * are adequate.
		 */
		ret = gnutls_priority_set_direct(session,
						 "NORMAL:-VERS-ALL:+VERS-TLS1.3:-KX-ALL:-SIGN-ALL:+SIGN-RSA-PSS-RSAE-SHA384:-GROUP-ALL:+GROUP-SECP256R1", NULL);
		if (ret < 0) {
			fail("server: priority set failed (%s)\n\n",
			     gnutls_strerror(ret));
			terminate();
		}

		gnutls_certificate_set_x509_key_mem(serverx509cred,
						    &server_cert, &server_key,
						    GNUTLS_X509_FMT_PEM);
		gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE,
				       serverx509cred);

		gnutls_handshake_set_random(session, &hsrnd);
		gnutls_transport_set_int(session, sds[t]);

		if (t > 0) {
			if (!gnutls_rnd_works) {
				fprintf(stderr, "gnutls_rnd() could not be overridden, skipping prf checks see #584\n");
				exit(77);
			} else {
				gnutls_handshake_set_hook_function(session, GNUTLS_HANDSHAKE_CLIENT_HELLO,
								   GNUTLS_HOOK_POST,
								   handshake_callback);
			}
		}

		do {
			ret = gnutls_handshake(session);
		}
		while (ret < 0 && gnutls_error_is_fatal(ret) == 0);
		if (ret < 0) {
			close(sds[t]);
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

		if (t == 0) {
			if (handshake_callback_called != 0)
				fail("server: handshake callback is called\n");
		} else {
			if (handshake_callback_called != t)
				fail("server: handshake callback is not called\n");
		}

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

		close(sds[t]);
		gnutls_deinit(session);
	}

	gnutls_certificate_free_credentials(serverx509cred);

	gnutls_free(session_ticket_key.data);
	session_ticket_key.data = NULL;

	gnutls_global_deinit();

	if (debug)
		success("server: finished\n");
}

void doit(void)
{
	int client_sds[SESSIONS], server_sds[SESSIONS];
	int i;
	int ret;

	_gnutls_global_version = 0x030607;
	signal(SIGPIPE, SIG_IGN);

	for (i = 0; i < SESSIONS; i++) {
		int sockets[2];

		ret = socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
		if (ret == -1) {
			perror("socketpair");
			fail("socketpair failed\n");
			return;
		}

		server_sds[i] = sockets[0];
		client_sds[i] = sockets[1];
	}

	child = fork();
	if (child < 0) {
		perror("fork");
		fail("fork");
		exit(1);
	}

	if (child) {
		int status = 0;
		/* parent */

		for (i = 0; i < SESSIONS; i++)
			close(client_sds[i]);
		server(server_sds);
		wait(&status);
		check_wait_status(status);
	} else {
		for (i = 0; i < SESSIONS; i++)
			close(server_sds[i]);
		client(client_sds);
		exit(0);
	}
}

#endif				/* _WIN32 */
