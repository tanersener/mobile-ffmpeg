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

#include <gnutls/gnutls.h>
#include <gnutls/openpgp.h>

#include "utils.h"

#include <unistd.h>
#include <stdlib.h>
#if !defined(_WIN32)
# include <sys/types.h>
# include <sys/socket.h>
#include <sys/wait.h>
#endif
#include <string.h>
#include <errno.h>
#include <stdio.h>
#if !defined(_WIN32)
static const char message[] = "Hello, brave GNU world!";

/* The OpenPGP key pair for use and the key ID in those keys.  */
static const char pub_key_file[] = "../guile/tests/openpgp-pub.asc";
static const char priv_key_file[] = "../guile/tests/openpgp-sec.asc";
static const char *key_id = NULL;
static gnutls_datum_t stored_cli_cert = { NULL, 0 };

static void log_message(int level, const char *msg)
{
	fprintf(stderr, "[%5d|%2d] %s", getpid(), level, msg);
}

static
int key_recv_func(gnutls_session_t session, const unsigned char *keyfpr,
		  unsigned int keyfpr_length, gnutls_datum_t * key)
{
	key->data = gnutls_malloc(stored_cli_cert.size);
	memcpy(key->data, stored_cli_cert.data, stored_cli_cert.size);
	key->size = stored_cli_cert.size;

	return 0;
}

static
void check_loaded_key(gnutls_certificate_credentials_t cred)
{
	int err;
	gnutls_openpgp_privkey_t key;
	gnutls_openpgp_crt_t *crts;
	unsigned n_crts;
	gnutls_openpgp_keyid_t keyid;
	unsigned i;

	/* check that the getter functions for openpgp keys of
	 * gnutls_certificate_credentials_t work and deliver the
	 * expected key ID. */

	err = gnutls_certificate_get_openpgp_key(cred, 0, &key);
	if (err != 0)
		fail("get openpgp key %s\n",
		     gnutls_strerror(err));

	gnutls_openpgp_privkey_get_subkey_id(key, 0, keyid);
	if (keyid[0] != 0xf3 || keyid[1] != 0x0f || keyid[2] != 0xd4 || keyid[3] != 0x23 ||
	    keyid[4] != 0xc1 || keyid[5] != 0x43 || keyid[6] != 0xe7 || keyid[7] != 0xba)
		fail("incorrect key id (privkey)\n");

	err = gnutls_certificate_get_openpgp_crt(cred, 0, &crts, &n_crts);
	if (err != 0)
		fail("get openpgp crts %s\n",
		     gnutls_strerror(err));

	if (n_crts != 1)
		fail("openpgp n_crts != 1\n");

	gnutls_openpgp_crt_get_subkey_id(crts[0], 0, keyid);
	if (keyid[0] != 0xf3 || keyid[1] != 0x0f || keyid[2] != 0xd4 || keyid[3] != 0x23 ||
	    keyid[4] != 0xc1 || keyid[5] != 0x43 || keyid[6] != 0xe7 || keyid[7] != 0xba)
		fail("incorrect key id (pubkey)\n");

	for (i = 0; i < n_crts; ++i)
		gnutls_openpgp_crt_deinit(crts[i]);
	gnutls_free(crts);
	gnutls_openpgp_privkey_deinit(key);
}

void doit(void)
{
	int err, i;
	int sockets[2];
	const char *srcdir;
	pid_t child;
	char pub_key_path[512], priv_key_path[512];

	global_init();

	srcdir = getenv("srcdir") ? getenv("srcdir") : ".";

	for (i = 0; i < 5; i++) {
		if (i <= 1)
			key_id = NULL;	/* try using the master key */
		else if (i == 2)
			key_id = "auto";	/* test auto */
		else if (i >= 3)
			key_id = "f30fd423c143e7ba";

		if (debug) {
			gnutls_global_set_log_level(5);
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

			if (i == 0)	/* we use the primary key which is RSA. Test the RSA ciphersuite */
				gnutls_priority_set_direct(session,
							   "NONE:+VERS-TLS1.0:+CIPHER-ALL:+MAC-ALL:+SIGN-ALL:+COMP-ALL:+RSA:+CTYPE-OPENPGP",
							   NULL);
			else
				gnutls_priority_set_direct(session,
							   "NONE:+VERS-TLS1.0:+CIPHER-ALL:+MAC-ALL:+SIGN-ALL:+COMP-ALL:+DHE-DSS:+DHE-RSA:+CTYPE-OPENPGP",
							   NULL);
			gnutls_transport_set_int(session, sockets[0]);

			err =
			    gnutls_certificate_allocate_credentials(&cred);
			if (err != 0)
				fail("client credentials %d\n", err);

			err =
			    gnutls_certificate_set_openpgp_key_file2(cred,
								     pub_key_path,
								     priv_key_path,
								     key_id,
								     GNUTLS_OPENPGP_FMT_BASE64);
			if (err != 0)
				fail("client openpgp keys %s\n",
				     gnutls_strerror(err));

			check_loaded_key(cred);

			err =
			    gnutls_credentials_set(session,
						   GNUTLS_CRD_CERTIFICATE,
						   cred);
			if (err != 0)
				fail("client credential_set %d\n", err);

			gnutls_dh_set_prime_bits(session, 1024);

			if (i == 4)
				gnutls_openpgp_send_cert(session,
							 GNUTLS_OPENPGP_CERT_FINGERPRINT);

			err = gnutls_handshake(session);
			if (err != 0)
				fail("client handshake %s (%d) \n",
				     gnutls_strerror(err), err);
			else if (debug)
				printf("client handshake successful\n");

			sent =
			    gnutls_record_send(session, message,
						sizeof(message));
			if (sent != sizeof(message))
				fail("client sent %li vs. %li\n",
				     (long) sent, (long) sizeof(message));

			err = gnutls_bye(session, GNUTLS_SHUT_RDWR);
			if (err != 0)
				fail("client bye %d\n", err);

			if (debug)
				printf("client done\n");

			gnutls_deinit(session);
			gnutls_certificate_free_credentials(cred);
			gnutls_free(stored_cli_cert.data);
			gnutls_global_deinit();
			return;
		} else {
			/* Parent process (server).  */
			gnutls_session_t session;
			gnutls_dh_params_t dh_params;
			gnutls_certificate_credentials_t cred;
			char greetings[sizeof(message) * 2];
			ssize_t received;
			pid_t done;
			int status;
			const gnutls_datum_t p3 =
			    { (void *) pkcs3, strlen(pkcs3) };

			if (debug)
				printf("server process %i (child %i)\n",
					getpid(), child);

			err = gnutls_init(&session, GNUTLS_SERVER);
			if (err != 0)
				fail("server session %d\n", err);

			gnutls_priority_set_direct(session,
						   "NONE:+VERS-TLS1.0:+CIPHER-ALL:+MAC-ALL:+SIGN-ALL:+COMP-ALL:+DHE-DSS:+DHE-RSA:+RSA:+CTYPE-OPENPGP",
						   NULL);
			gnutls_transport_set_int(session, sockets[1]);

			err =
			    gnutls_certificate_allocate_credentials(&cred);
			if (err != 0)
				fail("server credentials %d\n", err);

			err =
			    gnutls_certificate_set_openpgp_key_file2(cred,
								     pub_key_path,
								     priv_key_path,
								     key_id,
								     GNUTLS_OPENPGP_FMT_BASE64);
			if (err != 0)
				fail("server openpgp keys %s\n",
				     gnutls_strerror(err));

			check_loaded_key(cred);

			err = gnutls_dh_params_init(&dh_params);
			if (err)
				fail("server DH params init %d\n", err);

			err =
			    gnutls_dh_params_import_pkcs3(dh_params, &p3,
							  GNUTLS_X509_FMT_PEM);
			if (err)
				fail("server DH params generate %d\n",
				     err);

			gnutls_certificate_set_dh_params(cred, dh_params);

			err =
			    gnutls_credentials_set(session,
						   GNUTLS_CRD_CERTIFICATE,
						   cred);
			if (err != 0)
				fail("server credential_set %d\n", err);

			gnutls_certificate_server_set_request(session,
							      GNUTLS_CERT_REQUIRE);

			if (i == 4)
				gnutls_openpgp_set_recv_key_function
				    (session, key_recv_func);

			err = gnutls_handshake(session);
			if (err != 0)
				fail("server handshake %s (%d) \n",
				     gnutls_strerror(err), err);

			if (stored_cli_cert.data == NULL) {
				const gnutls_datum_t *d;
				unsigned int d_size;
				d = gnutls_certificate_get_peers(session,
								 &d_size);
				if (d != NULL) {
					stored_cli_cert.data =
					    gnutls_malloc(d[0].size);
					memcpy(stored_cli_cert.data,
						d[0].data, d[0].size);
					stored_cli_cert.size = d[0].size;
				}
			}

			received =
			    gnutls_record_recv(session, greetings,
						sizeof(greetings));
			if (received != sizeof(message)
			    || memcmp(greetings, message, sizeof(message)))
				fail("server received %li vs. %li\n",
				     (long) received,
				     (long) sizeof(message));

			err = gnutls_bye(session, GNUTLS_SHUT_RDWR);
			if (err != 0)
				fail("server bye %s (%d) \n",
				     gnutls_strerror(err), err);

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
	}

	gnutls_free(stored_cli_cert.data);
	gnutls_global_deinit();
}
#else
void doit()
{
	exit(77);
}
#endif
