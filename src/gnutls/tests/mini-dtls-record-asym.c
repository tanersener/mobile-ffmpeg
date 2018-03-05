/*
 * Copyright (C) 2014 Nikos Mavrogiannopoulos
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
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <gnutls/gnutls.h>
#include <gnutls/dtls.h>
#include <signal.h>
#include <unistd.h>
#include "utils.h"

#ifdef _WIN32

void doit(void)
{
	exit(77);
}

#else

#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>

/* Tests whether packing multiple DTLS records in a single UDP packet
 * will be handled correctly, as well as an asymmetry in MTU sizes
 * between server and client.
 */

const char *side = "";

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "%s|<%d>| %s", side, level, str);
}

static ssize_t push(gnutls_transport_ptr_t tr, const void *data, size_t len)
{
	static uint8_t buffer[6 * 1024];
	static unsigned buffer_size = 0;
	const uint8_t *d = data;
	int fd = (long int)tr;
	int ret;

	if (buffer_size + len > sizeof(buffer)) {
		abort();
	}

	memcpy(&buffer[buffer_size], data, len);
	buffer_size += len;

	if (d[0] == 22) {	/* handshake */
		if (d[13] == GNUTLS_HANDSHAKE_CERTIFICATE_PKT ||
		    d[13] == GNUTLS_HANDSHAKE_CERTIFICATE_STATUS ||
		    d[13] == GNUTLS_HANDSHAKE_SERVER_KEY_EXCHANGE ||
		    d[13] == GNUTLS_HANDSHAKE_SERVER_HELLO ||
		    d[13] == GNUTLS_HANDSHAKE_CERTIFICATE_REQUEST ||
		    d[13] == GNUTLS_HANDSHAKE_NEW_SESSION_TICKET ||
		    d[13] == GNUTLS_HANDSHAKE_CERTIFICATE_VERIFY ||
		    d[13] == GNUTLS_HANDSHAKE_CLIENT_KEY_EXCHANGE) {

			if (debug)
				fprintf(stderr, "caching: %s (buffer: %d)\n",
					gnutls_handshake_description_get_name(d[13]),
					buffer_size);
			return len;
		} else if (debug) {
			fprintf(stderr, "sending: %s\n",
				gnutls_handshake_description_get_name(d[13]));

		}
	}

	if (debug)
		fprintf(stderr, "sending %d bytes\n", (int)buffer_size);
	ret = send(fd, buffer, buffer_size, 0);
	if (ret >= 0) {
		if (debug)
			fprintf(stderr, "reset cache\n");
		buffer_size = 0;
	}
	return len;
}

static unsigned char server_cert_pem[] =
  "-----BEGIN CERTIFICATE-----\n"
  "MIICHzCCAaWgAwIBAgIBCTAKBggqhkjOPQQDAjA+MQswCQYDVQQGEwJOTDERMA8G\n"
  "A1UEChMIUG9sYXJTU0wxHDAaBgNVBAMTE1BvbGFyc3NsIFRlc3QgRUMgQ0EwHhcN\n"
  "MTMwOTI0MTU1MjA0WhcNMjMwOTIyMTU1MjA0WjA0MQswCQYDVQQGEwJOTDERMA8G\n"
  "A1UEChMIUG9sYXJTU0wxEjAQBgNVBAMTCWxvY2FsaG9zdDBZMBMGByqGSM49AgEG\n"
  "CCqGSM49AwEHA0IABDfMVtl2CR5acj7HWS3/IG7ufPkGkXTQrRS192giWWKSTuUA\n"
  "2CMR/+ov0jRdXRa9iojCa3cNVc2KKg76Aci07f+jgZ0wgZowCQYDVR0TBAIwADAd\n"
  "BgNVHQ4EFgQUUGGlj9QH2deCAQzlZX+MY0anE74wbgYDVR0jBGcwZYAUnW0gJEkB\n"
  "PyvLeLUZvH4kydv7NnyhQqRAMD4xCzAJBgNVBAYTAk5MMREwDwYDVQQKEwhQb2xh\n"
  "clNTTDEcMBoGA1UEAxMTUG9sYXJzc2wgVGVzdCBFQyBDQYIJAMFD4n5iQ8zoMAoG\n"
  "CCqGSM49BAMCA2gAMGUCMQCaLFzXptui5WQN8LlO3ddh1hMxx6tzgLvT03MTVK2S\n"
  "C12r0Lz3ri/moSEpNZWqPjkCMCE2f53GXcYLqyfyJR078c/xNSUU5+Xxl7VZ414V\n"
  "fGa5kHvHARBPc8YAIVIqDvHH1Q==\n"
  "-----END CERTIFICATE-----\n";

const gnutls_datum_t server_cert = { server_cert_pem,
	sizeof(server_cert_pem)
};

static unsigned char server_key_pem[] =
  "-----BEGIN EC PRIVATE KEY-----\n"
  "MHcCAQEEIPEqEyB2AnCoPL/9U/YDHvdqXYbIogTywwyp6/UfDw6noAoGCCqGSM49\n"
  "AwEHoUQDQgAEN8xW2XYJHlpyPsdZLf8gbu58+QaRdNCtFLX3aCJZYpJO5QDYIxH/\n"
  "6i/SNF1dFr2KiMJrdw1VzYoqDvoByLTt/w==\n"
  "-----END EC PRIVATE KEY-----\n";

const gnutls_datum_t server_key = { server_key_pem,
	sizeof(server_key_pem)
};

static void client(int fd, unsigned cache)
{
	int ret;
	gnutls_certificate_credentials_t x509_cred;
	gnutls_session_t session;
	/* Need to enable anonymous KX specifically. */

	fd_set rfds;
	struct timeval tv;

	global_init();

	if (debug) {
		side = "client";
		gnutls_global_set_log_function(tls_log_func);
		gnutls_global_set_log_level(4711);
	}

	gnutls_certificate_allocate_credentials(&x509_cred);

	/* Initialize TLS session
	 */
	gnutls_init(&session, GNUTLS_CLIENT | GNUTLS_DATAGRAM);
	gnutls_dtls_set_mtu(session, 1500);
	gnutls_dtls_set_timeouts(session, 6 * 1000, 60 * 1000);
	//gnutls_transport_set_push_function(session, push);

	/* Use default priorities */
	gnutls_priority_set_direct(session,
				   "NONE:+VERS-DTLS-ALL:+CIPHER-ALL:+MAC-ALL:+SIGN-ALL:+COMP-ALL:+ECDHE-ECDSA:+CURVE-ALL",
				   NULL);

	/* put the anonymous credentials to the current session
	 */
	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, x509_cred);

	gnutls_transport_set_int(session, fd);

	/* Perform the TLS handshake
	 */
	do {
		ret = gnutls_handshake(session);

		if (ret == GNUTLS_E_AGAIN && gnutls_record_get_direction(session) == 0) {
			int rv;
			FD_ZERO(&rfds);
			FD_SET(fd, &rfds);

			tv.tv_sec = 2;
			tv.tv_usec = 0;

			rv = select(fd+1, &rfds, NULL, NULL, &tv);
			if (rv == -1)
				perror("select()");
			else if (!rv)
				fail("test %d: No data were received.\n", cache);
		}
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
	int status;

	kill(child, SIGTERM);
	wait(&status);
	exit(1);
}

static void server(int fd, unsigned cache)
{
	int ret;
	gnutls_certificate_credentials_t x509_cred;
	gnutls_session_t session;

	/* this must be called once in the program
	 */
	global_init();

#if 0
	if (debug) {
		side = "server";
		gnutls_global_set_log_function(tls_log_func);
		gnutls_global_set_log_level(4711);
	}
#endif

	gnutls_certificate_allocate_credentials(&x509_cred);
	gnutls_certificate_set_x509_key_mem(x509_cred, &server_cert,
					    &server_key,
					    GNUTLS_X509_FMT_PEM);

	gnutls_init(&session, GNUTLS_SERVER | GNUTLS_DATAGRAM);
	gnutls_dtls_set_timeouts(session, 5 * 1000, 60 * 1000);
	gnutls_dtls_set_mtu(session, 400);
	if (cache != 0)
		gnutls_transport_set_push_function(session, push);

	/* avoid calling all the priority functions, since the defaults
	 * are adequate.
	 */
	gnutls_priority_set_direct(session,
				   "NONE:+VERS-DTLS1.2:+CIPHER-ALL:+MAC-ALL:+SIGN-ALL:+COMP-ALL:+ECDHE-ECDSA:+CURVE-ALL",
				   NULL);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, x509_cred);

	gnutls_transport_set_int(session, fd);

	do {
		ret = gnutls_handshake(session);
	} while (ret < 0 && gnutls_error_is_fatal(ret) == 0);
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

static
void run(unsigned cache)
{
	int fd[2];
	int ret;

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
		int status;
		/* parent */

		close(fd[1]);
		client(fd[0], cache);
		wait(&status);
		check_wait_status(status);
	} else {
		close(fd[0]);
		server(fd[1], cache);
		exit(0);
	}
}

void doit(void)
{
	signal(SIGPIPE, SIG_IGN);
	run(0);
	run(1);
}
#endif				/* _WIN32 */
