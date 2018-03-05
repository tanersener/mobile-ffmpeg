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
#include <gnutls/gnutls.h>
#include <gnutls/openpgp.h>

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

#define SESSIONS 2
#define MAX_BUF 1024
#define MSG "Hello TLS"

static unsigned char cert_txt[] =
    "-----BEGIN PGP PUBLIC KEY BLOCK-----\n"
    "Version: GnuPG v1.4.10 (GNU/Linux)\n"
    "Comment: Test key for GnuTLS\n"
    "\n"
    "mI0ETYD2OQEEAMHmDBtJii82NbWuYcvEWCYnwa7GTcz2PYikYCcq/t5nkyb5Bfmx\n"
    "mh2hpto7Lr5d1L/shvab1gXCcrWEAREgNNk9LiowtLuTHBdeOFlJ1u1P1rvdFVKq\n"
    "2a6ft77Q5VltUDKPgTqz4NWH2KUlLfTvwJDnq2DxYsbwVpBDURuUocXhABEBAAG0\n"
    "CVRlc3QgdXNlcoi4BBMBAgAiBQJNgPY5AhsvBgsJCAcDAgYVCAIJCgsEFgIDAQIe\n"
    "AQIXgAAKCRAMTrFUBnAKMOVDA/9GEw7AokwJSGvHREriXcvMMKp6c6SYqa0TVsTg\n"
    "Gh3ENu/KTfGJIM5p+zR6xy+5u5DfP5qLrRdCnoczncR5w9fn3RsP8ju/Ga5z23Q+\n"
    "6XxRKRkXjE/E0ZFulbuaBom/nhrOmmfqKe7Mor9Y4QwzL2wL3sf6jWLglwdFYS/X\n"
    "W3wqjLkBogRNgPY5EQQApafdUhCAHj8LLXYCqOXRSPZbKzvB55NwWrdvnod0seUW\n"
    "aiTSWBlKnSvIomdcII/E3bjdngK4fTJ+Xr5pEJuzBnW3w787r6jBJSq2Lp0T9SP4\n"
    "CBzd0gXcOQkILvX1VzxAsYVULJA0mhAR3IHFcywjX6ENKuvs7ApniBNoXqi6d3cA\n"
    "oIAzYKrjyZ+guM4IUlRRrB8abx5vBACJPV+d15GYgzt1d8zLvOl/mzs85Twj2SB1\n"
    "ZqzK6H/6QxQkEZpP/UVFpXaUGUly3nGEqg1yw4cgqW4SSxgLFz6B23Si+cTsssE6\n"
    "CYziN1UI6NjxkoG/npMm0wRp7Z+KylEolAdbFBAAprORkt58CrGgpYe8O/35+PWc\n"
    "J9rjhwxxkQP/VCpbZLugkL4XHWGWFGG35S6k9F3xPPTPoX9Zoud+0bOeoOK5RQHo\n"
    "e99sVNN4hxxPTM/rJXfTTZUoB6o84yulTSxb6C9ueHotDV0eB9QX1ov/ltmwy3XS\n"
    "fXEyWtI0CDBuZgEww26Up0pzg4XTBYMkmXrxx3J9ihcCIYyAHoE13EWI5wQYAQIA\n"
    "CQUCTYD2OQIbIgBSCRAMTrFUBnAKMEcgBBkRAgAGBQJNgPY5AAoJEPMP1CPBQ+e6\n"
    "3fQAnR7HWLnQTbxCIhlBTZiuJv2HC6cbAJwJ6VsSU6ADCkMuGT3LLNo+UnckK+4i\n"
    "BACcivWsW40ddtEQ0wno1uP65TmKq3aJrdODXTAnqkmNQKL7X7Fz+nmEWiS+LBH8\n"
    "lRvAaeRPX2LV+DCJDbAPrYd7LkOHyuM0I+ZApto5cjem/EnO7op2QwkCCa6oUp0l\n"
    "YA6i6aGF2KGx7WQwi2URIMPhihpOvAbkjfszYpFL4VP5wQ==\n"
    "=ydIq\n" "-----END PGP PUBLIC KEY BLOCK-----\n";

const gnutls_datum_t cert = { cert_txt, sizeof(cert_txt) };

static unsigned char key_txt[] =
    "-----BEGIN PGP PRIVATE KEY BLOCK-----\n"
    "Version: GnuPG v1.4.10 (GNU/Linux)\n"
    "Comment: Test key for GnuTLS\n"
    "\n"
    "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\n"
    "YpFL4VP5wQ==\n" "=zzoN\n" "-----END PGP PRIVATE KEY BLOCK-----\n";

const gnutls_datum_t key = { key_txt, sizeof(key_txt) };


static void client(int sds[])
{
	int ret, ii, j;
	gnutls_session_t session;
	char buffer[MAX_BUF + 1];
	gnutls_certificate_credentials_t xcred;

	global_init();

	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(9);

	gnutls_certificate_allocate_credentials(&xcred);

	/* sets the trusted cas file
	 */
	if (debug)
		success("Setting key files...\n");

	ret = gnutls_certificate_set_openpgp_key_mem(xcred, &cert, &key,
						     GNUTLS_OPENPGP_FMT_BASE64);
	if (ret < 0) {
		fail("Could not set key files...\n");
		return;
	}

	for (j = 0; j < SESSIONS; j++) {
		int sd = sds[j];

		/* Initialize TLS session
		 */
		gnutls_init(&session, GNUTLS_CLIENT);

		/* Use default priorities */
		gnutls_priority_set_direct(session,
					   "NORMAL:+CTYPE-OPENPGP:+DHE-DSS:+DHE-DSS:+SIGN-DSA-SHA1:+SIGN-DSA-SHA256", NULL);

		/* put the x509 credentials to the current session
		 */
		gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE,
					xcred);

		gnutls_transport_set_int(session, sd);

		/* Perform the TLS handshake
		 */
		ret = gnutls_handshake(session);

		if (ret < 0) {
			fail("client: Handshake %d failed\n", j);
			gnutls_perror(ret);
			goto end;
		} else if (debug) {
			success("client: Handshake %d was completed\n", j);
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

		close(sd);

		gnutls_deinit(session);

	}

      end:

	gnutls_certificate_free_credentials(xcred);

	gnutls_global_deinit();
}

/* This is a sample TLS 1.0 echo server, using X.509 authentication.
 */

#define MAX_BUF 1024
#define DH_BITS 1024

/* These are global */
gnutls_certificate_credentials_t pgp_cred;

static gnutls_session_t initialize_tls_session(void)
{
	gnutls_session_t session;

	gnutls_init(&session, GNUTLS_SERVER);

	/* avoid calling all the priority functions, since the defaults
	 * are adequate.
	 */
	gnutls_priority_set_direct(session, "NORMAL:+CTYPE-OPENPGP:+DHE-DSS:+SIGN-DSA-SHA1:+SIGN-DSA-SHA256", NULL);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, pgp_cred);

	/* request client certificate if any.
	 */
	gnutls_certificate_server_set_request(session,
					      GNUTLS_CERT_REQUEST);

	gnutls_dh_set_prime_bits(session, DH_BITS);

	return session;
}

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

int err, ret;
char topbuf[512];
gnutls_session_t session;
char buffer[MAX_BUF + 1];
int optval = 1;

static unsigned char server_crt_txt[] =
    "-----BEGIN PGP PUBLIC KEY BLOCK-----\n"
    "Version: GnuPG v1.4.6 (GNU/Linux)\n"
    "\n"
    "mNEER2PogwEGINdIR4u5PR4SwADWwj/ztgtoi7XVbmlfbQTHpBYFxTSC88pISSNy\n"
    "V/rgnlqunYP77F7aHL4KUReN3v9sKw01xSGEfox/JmlqUUg6CVvTjdeLfkuVIBnH\n"
    "j+2KMlaxezp7IxtPaTXpXcSf8iOuVq7UX7p6tKbppKXO5GgmfA88VUVvGBs1/PQp\n"
    "WKQdGrj+6I3RRmDN/hna1jGU/N23230Hbx+bu7g9cviiSh10ri7rdDhVJ67tRkRG\n"
    "Usy3XO6dWC7EmzZlEO8AEQEAAbQQdGVzdDMuZ251dGxzLm9yZ4kBAAQTAQIAJgUC\n"
    "R2PogwIbAwUJCWYBgAYLCQgHAwIEFQIIAwQWAgMBAh4BAheAAAoJEKAh4/gImZBR\n"
    "96QGH3E3zynETuQS3++hGMvMXq2mDJeT2e8964y/ifIOBpr2K2isuLYnrtGKyxi+\n"
    "ZptyHv6ymR3bDvio50cjnoT/WK1onosOJvtijGBS+U/ooq3im7ExpeQYXc/zpYsX\n"
    "OmB5m6BvdomUp2PMqdxsmOPoaRkSYx5R2Rlo/z3csodl6sp3k465Y/jg7L4gkxDz\n"
    "XJM+CS1xMhcOF0gBhppqLnG67x0ow847Pydstzkw0sOqedkLPuScaHNnlAWQ7QH6\n"
    "mbbpqHJwekS4jQRHiKV8AQQA0iZ81WXypLI4ZE2+hYfBCnfMVfQF/vPgvASxhwri\n"
    "GDa9Zc2f/VfakfNiwZgHH6iCeppHBiP2jljnbuOsL6f1R+0FsnyTVwHbuEU7IU2y\n"
    "+J0/s0z3wcx9sx8T7brP5z5F2hdagBsD9YFGCifHDAEew4mmAisY0i2QHVIuXJFj\n"
    "4RMAEQEAAYkBhwQYAQIADwUCR4ilfAIbAgUJEOrPgACoCRCgIeP4CJmQUZ0gBBkB\n"
    "AgAGBQJHiKV8AAoJEIN7b7QuD+F2AEcEAKAjhO9kSOE8UuwEOKlwsWL9LUUSkHJj\n"
    "c/ca0asLAerzrHsldRAcwCbWkVxBBHySw2CLFjzpgdXhwRtsytMgHaapfAPbinAW\n"
    "jCPIEJx2gDZeZnTgi4DVbZn5E3UzHGyL69MEoXr5t+vpiemQFd/nGD+h/Q2A76od\n"
    "gvAryRvS1Soj8bcGHjUflayXGOSvaD8P2V5Vz0hS82QZcqWxD8qUBqbcB8atokmO\n"
    "IYxhKyRmO58T5Ma+iaxBTUIwee+pBYDgdH6E2dh9xLlwwzZKaCcIRCQcObkLsMVo\n"
    "fZJo+m0Xf8zI57NeQF+hXJhW7lIrWgQVr8IVp/lgo76acLHfL/t1n0Nhg4r2srz2\n"
    "fpP2w5laQ0qImYLnZhGFHU+rJUyFaHfhD8/svN2LuZkO570pjV/K68EaHnEfk5b8\n"
    "jWu/euohwcCwf20M1kTo3Bg=\n"
    "=Xjon\n" "-----END PGP PUBLIC KEY BLOCK-----\n";
const gnutls_datum_t server_crt =
    { server_crt_txt, sizeof(server_crt_txt) };

static unsigned char server_key_txt[] =
    "-----BEGIN PGP PRIVATE KEY BLOCK-----\n"
    "Version: GnuPG v1.4.6 (GNU/Linux)\n"
    "\n"
    "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\n"
    "=mZnW\n" "-----END PGP PRIVATE KEY BLOCK-----\n";
const gnutls_datum_t server_key =
    { server_key_txt, sizeof(server_key_txt) };

static unsigned char cert2048_txt[] =
    "-----BEGIN PGP PUBLIC KEY BLOCK-----\n"
    "Version: GnuPG v1.4.10 (GNU/Linux)\n"
    "Comment: Test key for GnuTLS\n"
    "\n"
    "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\n"
    "=v3gg\n" "-----END PGP PUBLIC KEY BLOCK-----\n";

const gnutls_datum_t cert2048 = { cert2048_txt, sizeof(cert2048_txt) };

static unsigned char key2048_txt[] =
    "-----BEGIN PGP PRIVATE KEY BLOCK-----\n"
    "Version: GnuPG v1.4.10 (GNU/Linux)\n"
    "Comment: Test key for GnuTLS\n"
    "\n"
    "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\n"
    "9UquUquXdK1TY965biHIQ70uaOU4Hw==\n"
    "=Rrkw\n" "-----END PGP PRIVATE KEY BLOCK-----\n";

const gnutls_datum_t key2048 = { key2048_txt, sizeof(key2048_txt) };


static void server(int sds[])
{
	int j;
	/* this must be called once in the program
	 */
	global_init();

	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(5);

	if (debug)
		success("Launched, setting DH parameters...\n");

	generate_dh_params();

	for (j = 0; j < SESSIONS; j++) {
		int sd = sds[j];

		if (j == 0) {
			gnutls_certificate_allocate_credentials(&pgp_cred);
			ret =
			    gnutls_certificate_set_openpgp_key_mem2
			    (pgp_cred, &server_crt, &server_key, "auto",
			     GNUTLS_OPENPGP_FMT_BASE64);
		} else {
			gnutls_certificate_free_credentials(pgp_cred);
			gnutls_certificate_allocate_credentials(&pgp_cred);
			ret =
			    gnutls_certificate_set_openpgp_key_mem2
			    (pgp_cred, &cert2048, &key2048, "auto",
			     GNUTLS_OPENPGP_FMT_BASE64);
		}

		if (ret < 0) {
			fail("Could not set server key files...\n");
			goto end;
		}

		gnutls_certificate_set_dh_params(pgp_cred, dh_params);

		session = initialize_tls_session();

		gnutls_transport_set_int(session, sd);
		ret = gnutls_handshake(session);
		if (ret < 0) {
			close(sd);
			gnutls_deinit(session);
			fail("server: Handshake %d has failed (%s)\n\n",
			     j, gnutls_strerror(ret));
			goto end;
		}
		if (debug)
			success("server: Handshake %d was completed\n", j);

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
				goto end;
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
	}

      end:
	gnutls_certificate_free_credentials(pgp_cred);

	gnutls_dh_params_deinit(dh_params);

	gnutls_global_deinit();

	if (debug)
		success("server: finished\n");
}

void doit(void)
{
	int client_sds[SESSIONS], server_sds[SESSIONS];
	int i;

	for (i = 0; i < SESSIONS; i++) {
		int sockets[2];

		err = socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
		if (err == -1) {
			perror("socketpair");
			fail("socketpair failed\n");
			return;
		}

		server_sds[i] = sockets[0];
		client_sds[i] = sockets[1];
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
		server(server_sds);
		wait(&status);
	} else
		client(client_sds);
}

#endif				/* _WIN32 */
