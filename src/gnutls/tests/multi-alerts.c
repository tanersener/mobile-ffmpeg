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

/* In this test we check whether the server will bail out after receiving
 * a bunch of warning alerts. That is to avoid DoS due to the assymetry of
 * cost of sending an alert vs the cost of receiving.
 */

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
#include <assert.h>
#include <gnutls/gnutls.h>

#include "utils.h"
#include "cert-common.h"

pid_t child;

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "%s |<%d>| %s", child ? "server" : "client", level,
		str);
}

static unsigned char tls_hello[] =
	"\x16\x03\x01\x01\x38\x01\x00\x01"
	"\x34\x03\x03\xfc\x77\xa8\xc7\x46"
	"\xf7\xfd\x04\x5b\x3c\xc6\xfa\xa4"
	"\xea\x3e\xfa\x76\x99\xfe\x1a\x2e"
	"\xe0\x79\x17\xb2\x27\x06\xc4\x5c"
	"\xd8\x78\x31\x00\x00\xb6\xc0\x30"
	"\xc0\x2c\xc0\x28\xc0\x24\xc0\x14"
	"\xc0\x0a\x00\xa5\x00\xa3\x00\xa1"
	"\x00\x9f\x00\x6b\x00\x6a\x00\x69"
	"\x00\x68\x00\x39\x00\x38\x00\x37"
	"\x00\x36\x00\x88\x00\x87\x00\x86"
	"\x00\x85\xc0\x32\xc0\x2e\xc0\x2a"
	"\xc0\x26\xc0\x0f\xc0\x05\x00\x9d"
	"\x00\x3d\x00\x35\x00\x84\xc0\x2f"
	"\xc0\x2b\xc0\x27\xc0\x23\xc0\x13"
	"\xc0\x09\x00\xa4\x00\xa2\x00\xa0"
	"\x00\x9e\x00\x67\x00\x40\x00\x3f"
	"\x00\x3e\x00\x33\x00\x32\x00\x31"
	"\x00\x30\x00\x9a\x00\x99\x00\x98"
	"\x00\x97\x00\x45\x00\x44\x00\x43"
	"\x00\x42\xc0\x31\xc0\x2d\xc0\x29"
	"\xc0\x25\xc0\x0e\xc0\x04\x00\x9c"
	"\x00\x3c\x00\x2f\x00\x96\x00\x41"
	"\x00\x07\xc0\x11\xc0\x07\xc0\x0c"
	"\xc0\x02\x00\x05\x00\x04\xc0\x12"
	"\xc0\x08\x00\x16\x00\x13\x00\x10"
	"\x00\x0d\xc0\x0d\xc0\x03\x00\x0a"
	"\x00\x15\x00\x12\x00\x0f\x00\x0c"
	"\x00\x09\x00\xff\x01\x00\x00\x55"
	"\x00\x0b\x00\x04\x03\x00\x01\x02"
	"\x00\x0a\x00\x1c\x00\x1a\x00\x17"
	"\x00\x19\x00\x1c\x00\x1b\x00\x18"
	"\x00\x1a\x00\x16\x00\x0e\x00\x0d"
	"\x00\x0b\x00\x0c\x00\x09\x00\x0a"
	"\x00\x23\x00\x00\x00\x0d\x00\x20"
	"\x00\x1e\x06\x01\x06\x02\x06\x03"
	"\x05\x01\x05\x02\x05\x03\x04\x01"
	"\x04\x02\x04\x03\x03\x01\x03\x02"
	"\x03\x03\x02\x01\x02\x02\x02\x03"
	"\x00\x0f\x00\x01\x01";

static unsigned char tls_alert[] = 
	"\x15\x03\x03\x00\x02\x00\x0A";

static void client(int sd)
{
	char buf[1024];
	int ret;
	unsigned i;

	/* send a TLS hello, and then a list of warning alerts */

	ret = send(sd, tls_hello, sizeof(tls_hello)-1, 0);
	if (ret < 0)
		fail("error sending hello\n");

	ret = recv(sd, buf, sizeof(buf), 0);
	if (ret < 0)
		fail("error receiving hello\n");

	for (i=0;i<128;i++) {
		ret = send(sd, tls_alert, sizeof(tls_alert)-1, 0);
		if (ret < 0)
			fail("error sending hello\n");
	}

	close(sd);
}

static void server(int sd)
{
	gnutls_certificate_credentials_t x509_cred;
	gnutls_session_t session;
	int ret;
	unsigned loops;

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
	assert(gnutls_priority_set_direct(session, "NORMAL:-VERS-ALL:+VERS-TLS1.2", NULL)>=0);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, x509_cred);

	gnutls_transport_set_int(session, sd);
	loops = 0;
	do {
		ret = gnutls_handshake(session);
		loops++;
		if (loops > 64)
			fail("Too many loops in the handshake!\n");
	} while (ret == GNUTLS_E_INTERRUPTED || ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_WARNING_ALERT_RECEIVED);

	if (ret >= 0) {
		fail("server: Handshake succeeded unexpectedly\n");
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
		check_wait_status(status);
	} else {
		client(sockets[1]);
		exit(0);
	}
}

#endif				/* _WIN32 */
