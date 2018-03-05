/*
 * Copyright (C) 2015 Red Hat, Inc.
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
#ifndef _WIN32
# include <netinet/in.h>
# include <sys/types.h>
# include <sys/socket.h>
# include <sys/wait.h>
#endif
#include "utils.h"

#ifdef _WIN32

void doit(void)
{
	exit(77);
}

#else

/* These are global */
pid_t child;

static void terminate(void)
{
	int status;

	kill(child, SIGTERM);
	wait(&status);
	exit(1);
}

/* Tests whether we can send and receive from different processes
 * using DTLS, either as server or client. DTLS is a superset of
 * TLS, so correct behavior under fork means TLS would operate too.
 */

const char *side = "";

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "%s|<%d>| %s", side, level, str);
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

#define MSG "hello1111"
#define MSG2 "xxxxxxxxxxxx"

static
void do_fork_stuff(gnutls_session_t session)
{
	pid_t pid;
	int ret;
	char buf[64];

	/* separate sending from receiving */
	pid = fork();
	if (pid == -1) {
		exit(1);
	} else if (pid != 0) {
		if (debug)
			success("client: TLS version is: %s\n",
				gnutls_protocol_get_name
				(gnutls_protocol_get_version(session)));
		sec_sleep(1);
		/* the server should reflect our messages */
		ret = gnutls_record_recv(session, buf, sizeof(buf));
		if (ret != sizeof(MSG)-1 || memcmp(buf, MSG, sizeof(MSG)-1) != 0) {
			fail("client: recv failed: %s\n", gnutls_strerror(ret));
			exit(1);
		}

		if (debug) {
			fprintf(stderr, "client received: %.*s\n", ret, buf);
		}

		ret = gnutls_record_recv(session, buf, sizeof(buf));
		if (ret != sizeof(MSG2)-1 || memcmp(buf, MSG2, sizeof(MSG2)-1) != 0) {
			fail("client: recv2 failed: %s\n", gnutls_strerror(ret));
			exit(1);
		}

		if (debug) {
			fprintf(stderr, "client received: %.*s\n", ret, buf);
		}

		ret = gnutls_record_recv(session, buf, sizeof(buf));
		if (ret != 0) {
			fail("client: recv3 failed: %s\n", gnutls_strerror(ret));
			exit(1);
		}
	} else if (pid == 0) { /* child */
		ret = gnutls_record_send(session, MSG, sizeof(MSG)-1);
		if (ret != sizeof(MSG)-1) {
			fail("client: send failed: %s\n", gnutls_strerror(ret));
			exit(1);
		}

		ret = gnutls_record_send(session, MSG2, sizeof(MSG2)-1);
		if (ret != sizeof(MSG2)-1) {
			fail("client: send2 failed: %s\n", gnutls_strerror(ret));
			exit(1);
		}
		sec_sleep(2);
		gnutls_bye(session, GNUTLS_SHUT_WR);
	}
}

static void do_reflect_stuff(gnutls_session_t session)
{
	char buf[64];
	unsigned buf_size;
	int ret;

	do {
		ret = gnutls_record_recv(session, buf, sizeof(buf));
		if (ret < 0) {
			fail("server: recv failed: %s\n", gnutls_strerror(ret));
			terminate();
		}

		if (ret == 0)
			break;

		buf_size = ret;
		if (debug) {
			fprintf(stderr, "server received: %.*s\n", buf_size, buf);
		}

		ret = gnutls_record_send(session, buf, buf_size);
		if (ret < 0) {
			fail("server: send failed: %s\n", gnutls_strerror(ret));
			terminate();
		}
	} while(1);

	/* do not wait for the peer to close the connection.
	 */
	gnutls_bye(session, GNUTLS_SHUT_WR);
}

static void client(int fd, unsigned do_fork)
{
	int ret;
	gnutls_certificate_credentials_t x509_cred;
	gnutls_session_t session;
	/* Need to enable anonymous KX specifically. */

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

	if (do_fork)
		do_fork_stuff(session);
	else
		do_reflect_stuff(session);

	close(fd);

	gnutls_deinit(session);

	gnutls_certificate_free_credentials(x509_cred);

	gnutls_global_deinit();
	exit(0);
}


static void server(int fd, unsigned do_fork)
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

	if (do_fork)
		do_fork_stuff(session);
	else
		do_reflect_stuff(session);


	close(fd);
	gnutls_deinit(session);

	gnutls_certificate_free_credentials(x509_cred);

	gnutls_global_deinit();

	if (debug)
		success("server: finished\n");
}

static
void run(unsigned do_fork)
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
		client(fd[0], do_fork);
		wait(&status);
		check_wait_status(status);
	} else {
		close(fd[0]);
		server(fd[1], 1-do_fork);
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
