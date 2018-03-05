/*
 * Copyright (C) 2010-2012 Free Software Foundation, Inc.
 * Author: Ludovic Courtès
 *
 * This file is part of GNUTLS.
 *
 * GNUTLS is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * GNUTLS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNUTLS; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#if !defined(_WIN32)

#include <gnutls/gnutls.h>
#include <gnutls/openpgp.h>

#include "utils.h"

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>


/* This is the same test as openpgp-auth but tests
 * openpgp under the latest TLS protocol (TLSv1.2). In
 * addition it tests DSS signatures under that.
 */

static const char g_message[] = "Hello, brave GNU world!";

/* The OpenPGP key pair for use and the key ID in those keys.  */
static const char pub_key_file[] = "../guile/tests/openpgp-pub.asc";
static const char priv_key_file[] = "../guile/tests/openpgp-sec.asc";
static const char *key_id = NULL
    /* FIXME: The values below don't work as expected.  */
    /* "auto" */
    /* "bd572cdcccc07c35" */ ;

static void log_message(int level, const char *message)
{
	fprintf(stderr, "[%5d|%2d] %s", getpid(), level, message);
}


void doit(void)
{
	int err;
	int sockets[2];
	const char *srcdir;
	char pub_key_path[512], priv_key_path[512];
	pid_t child;

	global_init();

	srcdir = getenv("srcdir") ? getenv("srcdir") : ".";

	if (debug) {
		gnutls_global_set_log_level(10);
		gnutls_global_set_log_function(log_message);
	}

	err = socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
	if (err != 0)
		fail("socketpair %s\n", strerror(errno));

	if (sizeof(pub_key_path) <
	    strlen(srcdir) + strlen(pub_key_file) + 2)
		abort();

	strcpy(pub_key_path, srcdir);
	strcat(pub_key_path, "/");
	strcat(pub_key_path, pub_key_file);

	if (sizeof(priv_key_path) <
	    strlen(srcdir) + strlen(priv_key_file) + 2)
		abort();

	strcpy(priv_key_path, srcdir);
	strcat(priv_key_path, "/");
	strcat(priv_key_path, priv_key_file);

	child = fork();
	if (child == -1)
		fail("fork %s\n", strerror(errno));

	if (child == 0) {
		/* Child process (client).  */
		gnutls_session_t session;
		gnutls_certificate_credentials_t cred;
		ssize_t sent;

		if (debug)
			printf("client process %i\n", getpid());

		err = gnutls_init(&session, GNUTLS_CLIENT);
		if (err != 0)
			fail("client session %d\n", err);

		gnutls_priority_set_direct(session,
					   "NONE:+VERS-TLS1.2:+CIPHER-ALL:+MAC-ALL:+SIGN-ALL:+COMP-ALL:+DHE-DSS:+DHE-RSA:+CTYPE-OPENPGP",
					   NULL);
		gnutls_transport_set_int(session, sockets[0]);

		err = gnutls_certificate_allocate_credentials(&cred);
		if (err != 0)
			fail("client credentials %d\n", err);

		err =
		    gnutls_certificate_set_openpgp_key_file2(cred,
							     pub_key_path,
							     priv_key_path,
							     key_id,
							     GNUTLS_OPENPGP_FMT_BASE64);
		if (err != 0)
			fail("client openpgp keys %d\n", err);

		err =
		    gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE,
					   cred);
		if (err != 0)
			fail("client credential_set %d\n", err);

		gnutls_dh_set_prime_bits(session, 1024);

		err = gnutls_handshake(session);
		if (err != 0)
			fail("client handshake %s (%d) \n",
			     gnutls_strerror(err), err);
		else if (debug)
			printf("client handshake successful\n");

		sent =
		    gnutls_record_send(session, g_message, sizeof(g_message));
		if (sent != sizeof(g_message))
			fail("client sent %li vs. %li\n",
			     (long) sent, (long) sizeof(g_message));

		err = gnutls_bye(session, GNUTLS_SHUT_RDWR);
		if (err != 0)
			fail("client bye %d\n", err);

		if (debug)
			printf("client done\n");

		gnutls_deinit(session);
		gnutls_certificate_free_credentials(cred);
	} else {
		/* Parent process (server).  */
		gnutls_session_t session;
		gnutls_dh_params_t dh_params;
		gnutls_certificate_credentials_t cred;
		char greetings[sizeof(g_message) * 2];
		ssize_t received;
		pid_t done;
		int status;
		const gnutls_datum_t p3 =
		    { (void *) pkcs3, strlen(pkcs3) };

		if (debug)
			printf("server process %i (child %i)\n", getpid(),
				child);

		err = gnutls_init(&session, GNUTLS_SERVER);
		if (err != 0)
			fail("server session %d\n", err);

		gnutls_priority_set_direct(session,
					   "NONE:+VERS-TLS1.2:+CIPHER-ALL:+MAC-ALL:+SIGN-ALL:+COMP-ALL:+DHE-DSS:+DHE-RSA:+CTYPE-OPENPGP",
					   NULL);
		gnutls_transport_set_int(session, sockets[1]);

		err = gnutls_certificate_allocate_credentials(&cred);
		if (err != 0)
			fail("server credentials %d\n", err);

		err =
		    gnutls_certificate_set_openpgp_key_file2(cred,
							     pub_key_path,
							     priv_key_path,
							     key_id,
							     GNUTLS_OPENPGP_FMT_BASE64);
		if (err != 0)
			fail("server openpgp keys %d\n", err);

		err = gnutls_dh_params_init(&dh_params);
		if (err)
			fail("server DH params init %d\n", err);

		err =
		    gnutls_dh_params_import_pkcs3(dh_params, &p3,
						  GNUTLS_X509_FMT_PEM);
		if (err)
			fail("server DH params generate %d\n", err);

		gnutls_certificate_set_dh_params(cred, dh_params);

		err =
		    gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE,
					   cred);
		if (err != 0)
			fail("server credential_set %d\n", err);

		gnutls_certificate_server_set_request(session,
						      GNUTLS_CERT_REQUIRE);

		err = gnutls_handshake(session);
		if (err != 0)
			fail("server handshake %s (%d) \n",
			     gnutls_strerror(err), err);

		received =
		    gnutls_record_recv(session, greetings,
					sizeof(greetings));
		if (received != sizeof(g_message)
		    || memcmp(greetings, g_message, sizeof(g_message)))
			fail("server received %li vs. %li\n",
			     (long) received, (long) sizeof(g_message));

		err = gnutls_bye(session, GNUTLS_SHUT_RDWR);
		if (err != 0)
			fail("server bye %s (%d) \n", gnutls_strerror(err),
			     err);

		if (debug)
			printf("server done\n");

		gnutls_deinit(session);
		gnutls_certificate_free_credentials(cred);
		gnutls_dh_params_deinit(dh_params);

		done = wait(&status);
		if (done < 0)
			fail("wait %s\n", strerror(errno));

		if (done != child)
			fail("who's that?! %d\n", done);

		check_wait_status(status);
	}

	gnutls_global_deinit();
}
#else
#include <stdlib.h>

void doit()
{
	exit(77);
}
#endif
