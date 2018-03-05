/*
 * Copyright (C) 2004-2014 Free Software Foundation, Inc.
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

#if defined(_WIN32) || !defined(ENABLE_OPENPGP)

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

	ret =
	    gnutls_certificate_set_openpgp_key_mem2
		    (xcred, &cert, &key, "auto",
		     GNUTLS_OPENPGP_FMT_BASE64);
	if (ret < 0) {
		fail("error[%d]: %s\n", __LINE__, gnutls_strerror(ret));
	}

	/* Initialize TLS session
	 */
	gnutls_init(&session, GNUTLS_CLIENT);

	/* Use default priorities */
	gnutls_priority_set_direct(session, "NORMAL:+CTYPE-OPENPGP:+DHE-DSS:+SIGN-DSA-SHA256:+SIGN-DSA-SHA1", NULL);

	/* put the x509 credentials to the current session
	 */
	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);

	gnutls_transport_set_int(session, sd);

	/* Perform the TLS handshake
	 */
	do {
		ret = gnutls_handshake(session);
	} while(ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED);

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

static gnutls_privkey_t g_pkey = NULL;
static gnutls_pcert_st *g_pcert = NULL;

static int
cert_callback(gnutls_session_t session,
	      const gnutls_datum_t * req_ca_rdn, int nreqs,
	      const gnutls_pk_algorithm_t * sign_algos,
	      int sign_algos_length, gnutls_pcert_st ** pcert,
	      unsigned int *pcert_length, gnutls_privkey_t * pkey)
{
	int ret;
	gnutls_pcert_st *p;
	gnutls_privkey_t lkey;

	p = gnutls_malloc(sizeof(*p));
	if (p==NULL)
		return -1;

	if (g_pkey == NULL) {
		ret = gnutls_pcert_import_openpgp_raw(p, &server_crt, GNUTLS_OPENPGP_FMT_BASE64, NULL, 0);
		if (ret < 0)
			return -1;

		ret = gnutls_privkey_init(&lkey);
		if (ret < 0)
			return -1;

		ret = gnutls_privkey_import_openpgp_raw(lkey, &server_key, GNUTLS_OPENPGP_FMT_BASE64, NULL, NULL);
		if (ret < 0)
			return -1;

		g_pcert = p;
		g_pkey = lkey;

		*pcert = p;
		*pcert_length = 1;
		*pkey = lkey;
	} else {
		*pcert = g_pcert;
		*pcert_length = 1;
		*pkey = g_pkey;
	}

	return 0;
}

static void server(int sd)
{
gnutls_certificate_credentials_t pgp_cred;
int ret;
gnutls_session_t session;
gnutls_dh_params_t dh_params;
const gnutls_datum_t p3 = { (void *) pkcs3, strlen(pkcs3) };

	/* this must be called once in the program
	 */
	global_init();

	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(4711);

	gnutls_certificate_allocate_credentials(&pgp_cred);

	gnutls_certificate_set_retrieve_function2(pgp_cred, cert_callback);

	gnutls_dh_params_init(&dh_params);
	gnutls_dh_params_import_pkcs3(dh_params, &p3,
					     GNUTLS_X509_FMT_PEM);

	gnutls_certificate_set_dh_params(pgp_cred, dh_params);

	gnutls_init(&session, GNUTLS_SERVER);

	/* avoid calling all the priority functions, since the defaults
	 * are adequate.
	 */
	gnutls_priority_set_direct(session, "NORMAL:+CTYPE-OPENPGP:-CTYPE-X509:-RSA:+DHE-DSS:+SIGN-DSA-SHA256:+SIGN-DSA-SHA1", NULL);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, pgp_cred);

	/* request client certificate if any.
	 */
	gnutls_certificate_server_set_request(session,
					      GNUTLS_CERT_REQUEST);

	gnutls_transport_set_int(session, sd);
	gnutls_handshake_set_timeout(session, 20 * 1000);

	do {
		ret = gnutls_handshake(session);
	} while(ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED);
	if (ret < 0) {
		close(sd);
		gnutls_deinit(session);
		fail("server: Handshake has failed (%s)\n\n",
		     gnutls_strerror(ret));
		return;
	}
	if (debug)
		success("server: Handshake was completed\n");

	if (gnutls_certificate_get_ours(session) == NULL) {
		fail("our certificate was not sent!\n");
		exit(1);
	}

	if (debug)
		success("server: TLS version is: %s\n",
			gnutls_protocol_get_name
			(gnutls_protocol_get_version(session)));

	/* see the Getting peer's information example */
	if (debug)
		print_info(session);

	/* do not wait for the peer to close the connection.
	 */
	gnutls_bye(session, GNUTLS_SHUT_WR);

	close(sd);
	gnutls_deinit(session);

	gnutls_certificate_free_credentials(pgp_cred);
	gnutls_pcert_deinit(&g_pcert[0]);
	gnutls_privkey_deinit(g_pkey);

	gnutls_dh_params_deinit(dh_params);
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
		/* parent */
		client(sockets[0]);
		wait(&status);
		check_wait_status(status);
	} else
		server(sockets[1]);
}

#endif				/* _WIN32 */
