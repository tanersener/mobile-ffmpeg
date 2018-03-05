/*
 * Copyright (C) 2016 Red Hat, Inc.
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

/* In this test we check the parsing of SSL 2.0 client hellos. As
 * We can only read but not generate we use a fixed hello message
 * followed by an alert. That way we detect whether the handshake
 * completed hello parsing and reached the alert message.
 */

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
#if !defined(_WIN32)
#include <sys/wait.h>
#endif
#include <unistd.h>
#include <gnutls/gnutls.h>

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


static unsigned char ssl2_hello[] =
	"\x80\x59\x01\x03\x01\x00\x30\x00\x00\x00\x20\x00\x00\x39\x00\x00"
	"\x38\x00\x00\x35\x00\x00\x16\x00\x00\x13\x00\x00\x0a\x00\x00\x33"
	"\x00\x00\x32\x00\x00\x2f\x00\x00\x07\x00\x00\x05\x00\x00\x04\x00"
	"\x00\x15\x00\x00\x12\x00\x00\x09\x00\x00\xff\xb1\xc9\x95\x1a\x02"
	"\x6c\xd6\x42\x11\x6e\x99\xe2\x84\x97\xc9\x17\x53\xaf\x53\xf7\xfc"
	"\x8d\x1e\x72\x87\x18\x53\xee\xa6\x7d\x18\xc6";

static unsigned char tls_alert[] = 
	"\x15\x03\x01\x00\x02\x02\x5A";

static void client(int sd)
{
	char buf[1024];
	int ret;

	/* send an SSL 2.0 hello, and then an alert */
	
	ret = send(sd, ssl2_hello, sizeof(ssl2_hello)-1, 0);
	if (ret < 0)
		fail("error sending hello\n");

	ret = recv(sd, buf, sizeof(buf), 0);
	if (ret < 0)
		fail("error receiving hello\n");

	ret = send(sd, tls_alert, sizeof(tls_alert)-1, 0);
	if (ret < 0)
		fail("error sending hello\n");

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

	if (debug)
		success("Launched, generating DH parameters...\n");

	gnutls_init(&session, GNUTLS_SERVER);

	/* avoid calling all the priority functions, since the defaults
	 * are adequate.
	 */
	gnutls_priority_set_direct(session, "NORMAL", NULL);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, x509_cred);

	gnutls_transport_set_int(session, sd);
	ret = gnutls_handshake(session);
	if (ret != GNUTLS_E_FATAL_ALERT_RECEIVED || gnutls_alert_get(session) != GNUTLS_A_USER_CANCELED) {
		fail("server: Handshake failed unexpectedly (%s)\n\n",
		     gnutls_strerror(ret));
		return;
	}

	if (debug) {
		success("server: Handshake parsed the SSL2.0 client hello\n");
	}

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

		server(sockets[0]);
		wait(&status);
	} else
		client(sockets[1]);
}

#endif				/* _WIN32 */
