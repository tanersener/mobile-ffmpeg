/*
 * Copyright (C) 2004-2012 Free Software Foundation, Inc.
 * Copyright (C) 2013 Adam Sampson <ats@offog.org>
 * Copyright (C) 2018 Red Hat, Inc.
 *
 * Author: Simon Josefsson, Nikos Mavrogiannopoulos
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

/* This program tests anonymous authentication as well as the gnutls_record_recv_packet.
 */

#if defined(_WIN32)

int main(int argc, char **argv)
{
	exit(77);
}

#else

#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#if !defined(_WIN32)
#include <sys/wait.h>
#endif
#include <unistd.h>
#include <assert.h>
#include <gnutls/gnutls.h>

#include "utils.h"

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "|<%d>| %s", level, str);
}

#define MSG "Hello TLS"
#define MAX_BUF 1024

static void client(int sd, const char *prio)
{
	int ret, ii;
	gnutls_session_t session;
	char buffer[MAX_BUF + 1];
	gnutls_anon_client_credentials_t anoncred;
	/* Need to enable anonymous KX specifically. */

	global_init();

	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(4711);

	gnutls_anon_allocate_client_credentials(&anoncred);

	/* Initialize TLS session
	 */
	gnutls_init(&session, GNUTLS_CLIENT);

	/* Use default priorities */
	assert(gnutls_priority_set_direct(session,
					  prio,
					  NULL) >= 0);

	/* put the anonymous credentials to the current session
	 */
	gnutls_credentials_set(session, GNUTLS_CRD_ANON, anoncred);

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

	if (debug)
		print_dh_params_info(session);

	if (debug)
		success("client: TLS version is: %s\n",
			gnutls_protocol_get_name
			(gnutls_protocol_get_version(session)));

	ret = gnutls_record_send(session, MSG, sizeof(MSG)-1);
	if (ret != sizeof(MSG)-1) {
		fail("return value of gnutls_record_send() is bogus\n");
		exit(1);
	}

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

	if (ret != sizeof(MSG)-1 || memcmp(buffer, MSG, ret) != 0) {
		fail("client: received data of different size! (expected: %d, have: %d)\n", 
			(int)strlen(MSG), ret);
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

	gnutls_anon_free_client_credentials(anoncred);

	gnutls_global_deinit();
}

#define DH_BITS 1024

static void server(int sd, const char *prio)
{
	const gnutls_datum_t p3 = { (void *) pkcs3, strlen(pkcs3) };
	gnutls_anon_server_credentials_t anoncred;
	gnutls_dh_params_t dh_params;
	int ret;
	gnutls_session_t session;
	gnutls_packet_t packet;

	/* this must be called once in the program
	 */
	global_init();

	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(4711);

	gnutls_anon_allocate_server_credentials(&anoncred);

	if (debug)
		success("Launched, generating DH parameters...\n");

	assert(gnutls_dh_params_init(&dh_params)>=0);
	assert(gnutls_dh_params_import_pkcs3(dh_params, &p3,
					     GNUTLS_X509_FMT_PEM)>=0);

	gnutls_anon_set_server_dh_params(anoncred, dh_params);

	assert(gnutls_init(&session, GNUTLS_SERVER)>=0);

	assert(gnutls_priority_set_direct(session, prio, NULL) >= 0);

	gnutls_handshake_set_timeout(session, 20 * 1000);
	gnutls_credentials_set(session, GNUTLS_CRD_ANON, anoncred);

	gnutls_dh_set_prime_bits(session, DH_BITS);

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

	if (debug)
		success("server: TLS version is: %s\n",
			gnutls_protocol_get_name
			(gnutls_protocol_get_version(session)));

	if (debug)
		print_dh_params_info(session);

	for (;;) {
		ret = gnutls_record_recv_packet(session, &packet);

		if (ret == 0) {
			gnutls_packet_deinit(packet);
			if (debug)
				success
				    ("server: Peer has closed the GnuTLS connection\n");
			break;
		} else if (ret < 0) {
			fail("server: Received corrupted data(%d). Closing...\n", ret);
			break;
		} else if (ret > 0) {
			gnutls_datum_t pdata;

			gnutls_packet_get(packet, &pdata, NULL);
			/* echo data back to the client
			 */
			gnutls_record_send(session, pdata.data,
					   pdata.size);
			gnutls_packet_deinit(packet);
		}
	}
	/* do not wait for the peer to close the connection.
	 */
	gnutls_bye(session, GNUTLS_SHUT_WR);

	close(sd);
	gnutls_deinit(session);

	gnutls_anon_free_server_credentials(anoncred);

	gnutls_dh_params_deinit(dh_params);

	gnutls_global_deinit();

	if (debug)
		success("server: finished\n");
}

static
void start(const char *name, const char *prio)
{
	pid_t child;
	int sockets[2], err;

	success("testing: %s\n", name);
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
		int status;
		/* parent */
		server(sockets[0], prio);
		wait(&status);
		check_wait_status(status);
	} else {
		client(sockets[1], prio);
		exit(0);
	}
}

void doit(void)
{
	start("tls1.2 anon-dh", "NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+ANON-DH");
	start("tls1.2 anon-ecdh", "NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+ANON-ECDH");
	start("tls1.3 anon-dh", "NORMAL:-VERS-ALL:+VERS-TLS1.3:+VERS-TLS1.2:-KX-ALL:+ANON-DH");
	start("tls1.3 anon-ecdh", "NORMAL:-VERS-ALL:+VERS-TLS1.3:+VERS-TLS1.2:-KX-ALL:+ANON-ECDH");
	start("default anon-dh", "NORMAL:-KX-ALL:+ANON-DH");
	start("default anon-ecdh", "NORMAL:-KX-ALL:+ANON-ECDH");
}

#endif				/* _WIN32 */
