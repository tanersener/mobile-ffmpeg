/*
 * Copyright (C) 2012 Free Software Foundation, Inc.
 * Copyright (C) 2017 Red Hat, Inc.
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

#include "utils.h"
#include "cert-common.h"

static void terminate(void);

/* This program tests the robustness of record
 * decoding.
 */

static void server_log_func(int level, const char *str)
{
	fprintf(stderr, "server|<%d>| %s", level, str);
}

static void client_log_func(int level, const char *str)
{
	fprintf(stderr, "client|<%d>| %s", level, str);
}

#define MAX_BUF 1024

static int to_send = -1;
static int mtu = 0;

static ssize_t
push(gnutls_transport_ptr_t tr, const void *data, size_t len)
{
	int fd = (long int) tr;

	return send(fd, data, len, 0);
}

#define RECORD_HEADER_SIZE (5+8)

static ssize_t
push_crippled(gnutls_transport_ptr_t tr, const void *data, size_t len)
{
	int fd = (long int) tr;
	int _len, ret;
	uint8_t *_data = (void *) data;

	if (to_send == -1)
		return send(fd, data, len, 0);
	else {
#if 0
		_len =
		    ((uint8_t *) data)[11] << 8 | ((uint8_t *) data)[12];
		fprintf(stderr, "mtu: %d, len: %d", mtu, (int) _len);
		fprintf(stderr, " send: %d\n", (int) to_send);
#endif

		_len = to_send;
		_data[11] = _len >> 8;
		_data[12] = _len;

		/* correct len */
		ret = send(fd, data, RECORD_HEADER_SIZE + _len, 0);

		if (ret < 0)
			return ret;

		return len;
	}
}

static void client(int fd, const char *prio)
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

	/* Initialize TLS session
	 */
	gnutls_init(&session, GNUTLS_CLIENT | GNUTLS_DATAGRAM);
	gnutls_dtls_set_mtu(session, 1500);

	/* Use default priorities */
	ret = gnutls_priority_set_direct(session, prio, NULL);
	if (ret < 0) {
		fail("error in priority '%s': %s\n", prio, gnutls_strerror(ret));
		exit(1);
	}

	/* put the anonymous credentials to the current session
	 */
	gnutls_credentials_set(session, GNUTLS_CRD_ANON, anoncred);
	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, x509_cred);

	gnutls_transport_set_int(session, fd);
	gnutls_transport_set_push_function(session, push);

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

	/* make sure we are not blocked forever */
	gnutls_record_set_timeout(session, 10000);

	do {
		do {
			ret = gnutls_record_recv(session, buffer, MAX_BUF);
		} while (ret == GNUTLS_E_AGAIN
			 || ret == GNUTLS_E_INTERRUPTED);
	} while (ret > 0);

	if (ret == 0 || ret == GNUTLS_E_TIMEDOUT) {
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

static void server(int fd, const char *prio)
{
	int ret;
	char buffer[MAX_BUF + 1];
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

	gnutls_init(&session, GNUTLS_SERVER | GNUTLS_DATAGRAM);
	gnutls_dtls_set_mtu(session, 1500);

	/* avoid calling all the priority functions, since the defaults
	 * are adequate.
	 */
	ret = gnutls_priority_set_direct(session, prio, NULL);
	if (ret < 0) {
		fail("error in priority '%s': %s\n", prio, gnutls_strerror(ret));
		exit(1);
	}

	gnutls_credentials_set(session, GNUTLS_CRD_ANON, anoncred);
	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, x509_cred);

	gnutls_transport_set_int(session, fd);
	gnutls_transport_set_push_function(session, push_crippled);

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
	mtu = gnutls_dtls_get_mtu(session);

	do {
		usleep(10000); /* some systems like FreeBSD have their buffers full during this send */
		do {
			ret =
			    gnutls_record_send(session, buffer,
						sizeof(buffer));
		} while (ret == GNUTLS_E_AGAIN
			 || ret == GNUTLS_E_INTERRUPTED);

		if (ret < 0) {
			fail("Error sending %d byte packet: %s\n", to_send,
			     gnutls_strerror(ret));
			terminate();
		}
		to_send++;
	}
	while (to_send < 64);

	to_send = -1;

	/* wait for the peer to close the connection.
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

static void start(const char *name, const char *prio)
{
	int fd[2];
	int ret, status = 0;

	ret = socketpair(AF_UNIX, SOCK_DGRAM, 0, fd);
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
		success("trying: %s\n", name);
		close(fd[0]);
		client(fd[1], prio);
		wait(&status);
		check_wait_status(status);
	} else {
		close(fd[1]);
		server(fd[0], prio);
		exit(0);
	}
}

#define AES_CBC "NONE:+VERS-DTLS1.0:-CIPHER-ALL:+AES-128-CBC:+SHA1:+SIGN-ALL:+COMP-ALL:+ANON-ECDH:+CURVE-ALL"
#define AES_CBC_SHA256 "NONE:+VERS-DTLS1.2:-CIPHER-ALL:+RSA:+AES-128-CBC:+AES-256-CBC:+SHA256:+SIGN-ALL:+COMP-ALL:+ANON-ECDH:+CURVE-ALL"
#define AES_GCM "NONE:+VERS-DTLS1.2:-CIPHER-ALL:+RSA:+AES-128-GCM:+MAC-ALL:+SIGN-ALL:+COMP-ALL:+ANON-ECDH:+CURVE-ALL"
#define AES_CCM "NONE:+VERS-DTLS1.2:-CIPHER-ALL:+RSA:+AES-128-CCM:+MAC-ALL:+SIGN-ALL:+COMP-ALL:+ANON-ECDH:+CURVE-ALL"
#define AES_CCM_8 "NONE:+VERS-DTLS1.2:-CIPHER-ALL:+RSA:+AES-128-CCM-8:+MAC-ALL:+SIGN-ALL:+COMP-ALL:+ANON-ECDH:+CURVE-ALL"
#define CHACHA_POLY1305 "NONE:+VERS-DTLS1.2:-CIPHER-ALL:+RSA:+CHACHA20-POLY1305:+MAC-ALL:+SIGN-ALL:+COMP-ALL:+ECDHE-RSA:+CURVE-ALL"

static void ch_handler(int sig)
{
	return;
}

void doit(void)
{
	signal(SIGCHLD, ch_handler);
	signal(SIGPIPE, SIG_IGN);

	start("aes-cbc", AES_CBC);
	start("aes-cbc-sha256", AES_CBC_SHA256);
	start("aes-gcm", AES_GCM);
	start("aes-ccm", AES_CCM);
	start("aes-ccm-8", AES_CCM_8);
	if (!gnutls_fips140_mode_enabled()) {
		start("chacha20", CHACHA_POLY1305);
	}
}

#endif				/* _WIN32 */
