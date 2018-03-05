/*
 * Copyright (C) 2004-2012 Free Software Foundation, Inc.
 * Copyright (C) 2013 Adam Sampson <ats@offog.org>
 *
 * Author: Simon Josefsson
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

/* Parts copied from GnuTLS example programs. */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include "cert-common.h"

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
#include <signal.h>
#include <gnutls/gnutls.h>
#include <gnutls/abstract.h>

#include "utils.h"

#include "ex-session-info.c"
#include "ex-x509-info.c"

pid_t child;

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "%s |<%d>| %s", child ? "server" : "client", level,
		str);
}

/* A very basic TLS client, with anonymous authentication.
 */

#define MAX_BUF 1024
#define MSG "Hello TLS"

#define EXPECT_RDN0 "CA-3"

static int
cert_callback(gnutls_session_t session,
	      const gnutls_datum_t * req_ca_rdn, int nreqs,
	      const gnutls_pk_algorithm_t * sign_algos,
	      int sign_algos_length, gnutls_pcert_st ** pcert,
	      unsigned int *pcert_length, gnutls_privkey_t * pkey)
{
	int result;
	gnutls_x509_dn_t dn;

	if (nreqs != 1) {
		fail("client: invoked to provide client cert, but %d CAs are requested by server.\n",
		     nreqs);
		return -1;
	}

	if (debug)
		success("client: invoked to provide client cert.\n");

	result = gnutls_x509_dn_init(&dn);
	if (result < 0) {
		fail("client: could not initialize DN.\n");
		return -1;
	}

	result = gnutls_x509_dn_import(dn, req_ca_rdn);
	if (result == 0) {
		gnutls_x509_ava_st val;

		if (debug)
			success("client: imported DN.\n");

		if (gnutls_x509_dn_get_rdn_ava(dn, 0, 0, &val) == 0) {
			if (debug)
				success("client: got RDN 0.\n");

			if (val.value.size == strlen(EXPECT_RDN0)
			    && strncmp((char *) val.value.data,
					EXPECT_RDN0, val.value.size) == 0) {
				if (debug)
					success
					    ("client: RND 0 correct.\n");
			} else {
				fail("client: RND 0 bad: %.*s\n",
				     val.value.size, val.value.data);
				return -1;
			}
		} else {
			fail("client: could not retrieve RDN 0.\n");
			return -1;
		}

		gnutls_x509_dn_deinit(dn);
	} else {
		fail("client: failed to parse RDN: %s\n",
		     gnutls_strerror(result));
	}

	return 0;
}


static void client(int sd)
{
	int ret, ii;
	gnutls_session_t session;
	char buffer[MAX_BUF + 1];
	gnutls_certificate_credentials_t xcred;

	global_init();

	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(4711);

	gnutls_certificate_allocate_credentials(&xcred);

	/* sets the trusted cas file
	 */
	ret = gnutls_certificate_set_x509_trust_mem(xcred, &ca3_cert,
					      GNUTLS_X509_FMT_PEM);
	if (ret <= 0) {
		fail("client: no CAs loaded!\n");
		goto end;
	}

	gnutls_certificate_set_retrieve_function2(xcred, cert_callback);

	/* Initialize TLS session
	 */
	gnutls_init(&session, GNUTLS_CLIENT);

	/* Use default priorities */
	gnutls_priority_set_direct(session, "NORMAL", NULL);

	/* put the x509 credentials to the current session
	 */
	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);

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
		success("client: TLS version is: %s\n",
			gnutls_protocol_get_name
			(gnutls_protocol_get_version(session)));

	/* see the Getting peer's information example */
	if (debug)
		print_info(session);

	gnutls_record_send(session, MSG, strlen(MSG));

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

	gnutls_certificate_free_credentials(xcred);

	gnutls_global_deinit();
}

/* This is a sample TLS 1.0 echo server, using X.509 authentication.
 */

#define MAX_BUF 1024
#define DH_BITS 1024

/* These are global */

static gnutls_dh_params_t dh_params;

static int generate_dh_params(void)
{
	const gnutls_datum_t p3 = { (void *) pkcs3, strlen(pkcs3) };
	/* Generate Diffie-Hellman parameters - for use with DHE
	 * kx algorithms. These should be discarded and regenerated
	 * once a day, once a week or once a month. Depending on the
	 * security requirements.
	 */
	gnutls_dh_params_init(&dh_params);
	return gnutls_dh_params_import_pkcs3(dh_params, &p3,
					     GNUTLS_X509_FMT_PEM);
}



static void server(int sd)
{
gnutls_certificate_credentials_t x509_cred;
int ret;
gnutls_session_t session;
char buffer[MAX_BUF + 1];
	/* this must be called once in the program
	 */
	global_init();

	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(4711);

	gnutls_certificate_allocate_credentials(&x509_cred);
	ret = gnutls_certificate_set_x509_trust_mem(x509_cred, &ca3_cert,
					      GNUTLS_X509_FMT_PEM);
	if (ret == 0) {
		fail("server: no CAs loaded\n");
	}

	gnutls_certificate_set_x509_key_mem(x509_cred,
					    &server_ca3_localhost_cert,
					    &server_ca3_key,
					    GNUTLS_X509_FMT_PEM);

	if (debug)
		success("Launched, generating DH parameters...\n");

	generate_dh_params();

	gnutls_certificate_set_dh_params(x509_cred, dh_params);

	gnutls_init(&session, GNUTLS_SERVER);

	/* avoid calling all the priority functions, since the defaults
	 * are adequate.
	 */
	gnutls_priority_set_direct(session, "NORMAL", NULL);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, x509_cred);

	/* request client certificate if any.
	 */
	gnutls_certificate_server_set_request(session,
					      GNUTLS_CERT_REQUEST);

	gnutls_dh_set_prime_bits(session, DH_BITS);

	gnutls_transport_set_int(session, sd);
	gnutls_handshake_set_timeout(session, 20 * 1000);

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

	/* see the Getting peer's information example */
	if (debug)
		print_info(session);

	for (;;) {
		memset(buffer, 0, MAX_BUF + 1);
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
	/* do not wait for the peer to close the connection.
	 */
	gnutls_bye(session, GNUTLS_SHUT_WR);

	close(sd);
	gnutls_deinit(session);

	gnutls_certificate_free_credentials(x509_cred);

	gnutls_dh_params_deinit(dh_params);

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
		/* parent */
		close(sockets[1]);
		server(sockets[0]);
		wait(&status);
		check_wait_status(status);
	} else {
		close(sockets[0]);
		client(sockets[1]);
	}
}

#endif				/* _WIN32 */
