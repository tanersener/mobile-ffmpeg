/*
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
 * You should have received a copy of the GNU General Public License
 * along with GnuTLS; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#if defined(_WIN32) || !defined(ENABLE_SSL2)

/* socketpair isn't supported on Win32. */
int main(int argc, char **argv)
{
	exit(77);
}

#else

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <gnutls/gnutls.h>
#include <signal.h>
#include <poll.h>
#include <errno.h>

#include "utils.h"
#include "cert-common.h"

pid_t child;

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "%s |<%d>| %s", child ? "server" : "client", level,
		str);
}

/* A very basic TLS client, with anonymous authentication.
 */


static unsigned char tls1_hello[] =
	"\x16\x03\x01\x01\x5e\x01\x00\x01\x5a\x03\x03\x59\x41\x25\x0e\x19"
	"\x02\x56\xa2\xe4\x97\x00\xea\x18\xd2\xb0\x00\xb9\xa2\x8a\x61\xb3"
	"\xdd\x65\xed\xfd\x03\xaf\x93\x8d\xb2\x15\xf3\x00\x00\xd4\xc0\x30"
	"\xcc\xa8\xc0\x8b\xc0\x14\xc0\x28\xc0\x77\xc0\x2f\xc0\x8a\xc0\x13"
	"\xc0\x27\xc0\x76\xc0\x12\xc0\x2c\xc0\xad\xcc\xa9\xc0\x87\xc0\x0a"
	"\xc0\x24\xc0\x73\xc0\x2b\xc0\xac\xc0\x86\xc0\x09\xc0\x23\xc0\x72"
	"\xc0\x08\x00\x9d\xc0\x9d\xc0\x7b\x00\x35\x00\x3d\x00\x84\x00\xc0"
	"\x00\x9c\xc0\x9c\xc0\x7a\x00\x2f\x00\x3c\x00\x41\x00\xba\x00\x0a"
	"\x00\x9f\xc0\x9f\xcc\xaa\xc0\x7d\x00\x39\x00\x6b\x00\x88\x00\xc4"
	"\x00\x9e\xc0\x9e\xc0\x7c\x00\x33\x00\x67\x00\x45\x00\xbe\x00\x16"
	"\x00\xa3\xc0\x81\x00\x38\x00\x6a\x00\x87\x00\xc3\x00\xa2\xc0\x80"
	"\x00\x32\x00\x40\x00\x44\x00\xbd\x00\x13\x00\xa9\xc0\xa5\xcc\xab"
	"\xc0\x8f\x00\x8d\x00\xaf\xc0\x95\x00\xa8\xc0\xa4\xc0\x8e\x00\x8c"
	"\x00\xae\xc0\x94\x00\x8b\x00\xab\xc0\xa7\xcc\xad\xc0\x91\x00\x91"
	"\x00\xb3\xc0\x97\x00\xaa\xc0\xa6\xc0\x90\x00\x90\x00\xb2\xc0\x96"
	"\x00\x8f\xcc\xac\xc0\x36\xc0\x38\xc0\x9b\xc0\x35\xc0\x37\xc0\x9a"
	"\xc0\x34\x01\x00\x00\x5d\x00\x17\x00\x00\x00\x16\x00\x00\x00\x05"
	"\x00\x05\x01\x00\x00\x00\x00\x00\x00\x00\x13\x00\x11\x00\x00\x0e"
	"\x77\x77\x77\x2e\x61\x6d\x61\x7a\x6f\x6e\x2e\x63\x6f\x6d\xff\x01"
	"\x00\x01\x00\x00\x23\x00\x00\x00\x0b\x00\x02\x01\x00\x00\x0b\x00"
	"\x02\x01\x00\x00\x0d\x00\x16\x00\x14\x04\x01\x04\x03\x05\x01\x05"
	"\x03\x06\x01\x06\x03\x03\x01\x03\x03\x02\x01\x02\x03\x00\x0a\x00"
	"\x02\x00\x17";

static void client(int sd)
{
	char buf[1024];
	int ret;
	struct pollfd pfd;

	/* send a TLS 1.x hello with duplicate extensions */
	
	ret = send(sd, tls1_hello, sizeof(tls1_hello)-1, 0);
	if (ret < 0)
		fail("error sending hello\n");

	pfd.fd = sd;
	pfd.events = POLLIN;
	pfd.revents = 0;

	do {
		ret = poll(&pfd, 1, 10000);
	} while (ret == -1 && errno == EINTR);

	if (ret == -1 || ret == 0) {
		fail("timeout waiting for reply\n");
	}

	success("sent hello\n");
	ret = recv(sd, buf, sizeof(buf), 0);
	if (ret < 0)
		fail("error receiving alert\n");

	success("received reply\n");

	if (ret < 7)
		fail("error in size of received alert\n");

	if (buf[0] != 0x15 || buf[1] != 0x03)
		fail("error in received alert data\n");

	success("all ok\n");

	close(sd);
}

static void server(int sd)
{
	gnutls_certificate_credentials_t x509_cred;
	gnutls_session_t session;
	int ret;

	/* this must be called once in the program
	 */
	global_init();

	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(6);

	gnutls_certificate_allocate_credentials(&x509_cred);
	gnutls_certificate_set_x509_trust_mem(x509_cred, &ca3_cert,
					      GNUTLS_X509_FMT_PEM);

	gnutls_certificate_set_x509_key_mem(x509_cred, &server_ca3_localhost_cert,
					    &server_ca3_key,
					    GNUTLS_X509_FMT_PEM);

	gnutls_init(&session, GNUTLS_SERVER);

	/* avoid calling all the priority functions, since the defaults
	 * are adequate.
	 */
	gnutls_priority_set_direct(session, "NORMAL:-VERS-ALL:+VERS-TLS1.0:+VERS-TLS1.1:+VERS-TLS1.2", NULL);
	gnutls_handshake_set_timeout(session, 20 * 1000);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, x509_cred);

	gnutls_transport_set_int(session, sd);
	do {
		ret = gnutls_handshake(session);
	} while(ret == GNUTLS_E_INTERRUPTED || ret == GNUTLS_E_AGAIN);

	if (ret != GNUTLS_E_RECEIVED_ILLEGAL_EXTENSION) {
		fail("server: Handshake succeeded unexpectedly\n");
	}

	gnutls_alert_send_appropriate(session, ret);
	
	close(sd);
	gnutls_deinit(session);

	gnutls_certificate_free_credentials(x509_cred);

	gnutls_global_deinit();

	if (debug)
		success("server: finished\n");
}


void doit(void)
{
	int sockets[2];
	int err;

	signal(SIGPIPE, SIG_IGN);

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

		client(sockets[1]);
		wait(&status);
		check_wait_status(status);
	} else {
		server(sockets[0]);
		_exit(0);
	}
}

#endif				/* _WIN32 */
