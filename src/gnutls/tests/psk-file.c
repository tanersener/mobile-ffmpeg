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

#include <stdbool.h>
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
#include <signal.h>
#include <assert.h>
#include <gnutls/gnutls.h>

#include "utils.h"

static char hexchar(unsigned int val)
{
	if (val < 10)
		return '0' + val;
	if (val < 16)
		return 'a' + val - 10;
	abort();
}

static bool hex_encode(const void *buf, size_t bufsize, char *dest, size_t destsize)
{
	size_t used = 0;

	if (destsize < 1)
		return false;

	while (used < bufsize) {
		unsigned int c = ((const unsigned char *)buf)[used];
		if (destsize < 3)
			return false;
		*(dest++) = hexchar(c >> 4);
		*(dest++) = hexchar(c & 0xF);
		used++;
		destsize -= 2;
	}
	*dest = '\0';

	return used + 1;
}

/* A very basic TLS client, with PSK authentication.
 */

const char *side = "";

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "%s|<%d>| %s", side, level, str);
}

#define MAX_BUF 1024
#define MSG "Hello TLS"

static void client(int sd, const char *prio, const gnutls_datum_t *user, const gnutls_datum_t *key,
		   unsigned expect_hint, int expect_fail, int exp_kx, unsigned binary_user)
{
	int ret, ii, kx;
	gnutls_session_t session;
	char buffer[MAX_BUF + 1];
	gnutls_psk_client_credentials_t pskcred;
	const char *hint;

	global_init();
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(4711);

	side = "client";

	gnutls_psk_allocate_client_credentials(&pskcred);

	if (binary_user) {
		gnutls_psk_set_client_credentials2(pskcred, user, key, GNUTLS_PSK_KEY_HEX);
	} else {
		gnutls_psk_set_client_credentials(pskcred, (const char *) user->data, key,
						  GNUTLS_PSK_KEY_HEX);
	}

	assert(gnutls_init(&session, GNUTLS_CLIENT|GNUTLS_KEY_SHARE_TOP)>=0);

	assert(gnutls_priority_set_direct(session, prio, NULL)>=0);

	/* put the anonymous credentials to the current session
	 */
	gnutls_credentials_set(session, GNUTLS_CRD_PSK, pskcred);

	gnutls_transport_set_int(session, sd);

	/* Perform the TLS handshake
	 */
	ret = gnutls_handshake(session);

	if (ret < 0) {
		if (!expect_fail)
			fail("client: Handshake failed\n");
		if (ret != expect_fail) {
			fail("expected cli error %d (%s), got %d (%s)\n", expect_fail, gnutls_strerror(expect_fail),
								      ret, gnutls_strerror(ret));
		}
		goto end;
	} else {
		if (debug)
			success("client: Handshake was completed\n");
	}

	/* check the hint */
	if (expect_hint) {
		hint = gnutls_psk_client_get_hint(session);
		if (hint == NULL || strcmp(hint, "hint") != 0) {
			fail("client: hint is not the expected: %s\n", gnutls_psk_client_get_hint(session));
			goto end;
		}
	}

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

	kx = gnutls_kx_get(session);

	if (debug) {
		printf("- Received %d bytes: ", ret);
		for (ii = 0; ii < ret; ii++) {
			fputc(buffer[ii], stdout);
		}
		fputs("\n", stdout);
	}

	gnutls_bye(session, GNUTLS_SHUT_RDWR);

	if (expect_fail)
		fail("client: expected failure but connection succeeded!\n");

	if (exp_kx && kx != exp_kx) {
		fail("client: expected key exchange %s, but got %s\n",
		     gnutls_kx_get_name(exp_kx),
		     gnutls_kx_get_name(kx));
	}

      end:

	close(sd);

	gnutls_deinit(session);

	gnutls_psk_free_client_credentials(pskcred);

	gnutls_global_deinit();
}

/* This is a sample TLS 1.0 echo server, for PSK authentication.
 */

#define MAX_BUF 1024

static void server(int sd, const char *prio, const gnutls_datum_t *user, bool no_cred,
		   int expect_fail, int exp_kx, unsigned binary_user)
{
	gnutls_psk_server_credentials_t server_pskcred;
	int ret, kx;
	gnutls_session_t session;
	const char *pskid;
	gnutls_datum_t pskid_binary;
	char buffer[MAX_BUF + 1];
	char *psk_file = getenv("PSK_FILE");
	char *desc;

	/* this must be called once in the program
	 */
	global_init();
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(4711);

	side = "server";

	if (psk_file == NULL)
		psk_file = (char*)"psk.passwd";

	gnutls_psk_allocate_server_credentials(&server_pskcred);
	gnutls_psk_set_server_credentials_hint(server_pskcred, "hint");
	ret = gnutls_psk_set_server_credentials_file(server_pskcred, psk_file);
	if (ret < 0) {
		fail("server: gnutls_psk_set_server_credentials_file failed (%s)\n\n",
		     gnutls_strerror(ret));
		return;
	}

	gnutls_init(&session, GNUTLS_SERVER);

	assert(gnutls_priority_set_direct(session, prio, NULL)>=0);

	if (!no_cred)
		gnutls_credentials_set(session, GNUTLS_CRD_PSK, server_pskcred);

	gnutls_transport_set_int(session, sd);
	ret = gnutls_handshake(session);
	if (ret < 0) {
		unsigned char seq[8], buf[16];
		gnutls_alert_send_appropriate(session, ret);

		/* We have to make sure that we do not close connection till
		 * test client reads our fatal alert, otherwise it might exit
		 * with GNUTLS_E_PUSH_ERROR instead */
		gnutls_session_force_valid(session);
		while ((gnutls_record_recv_seq(session, buf, sizeof(buf), seq)) >= 0)
			;

		if (expect_fail) {
			if (ret != expect_fail) {
				fail("expected error %d (%s), got %d (%s)\n", expect_fail,
									      gnutls_strerror(expect_fail),
									      ret, gnutls_strerror(ret));
			}

			if (debug)
				success("server: Handshake has failed - expected (%s)\n\n",
				     gnutls_strerror(ret));
		} else {
			fail("server: Handshake has failed (%s)\n\n",
			     gnutls_strerror(ret));
		}
		goto end;
	}
	if (debug)
		success("server: Handshake was completed\n");

	/* see the Getting peer's information example */
	/* print_info(session); */

	for (;;) {
		memset(buffer, 0, MAX_BUF + 1);
		gnutls_record_set_timeout(session, 10000);
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

	kx = gnutls_kx_get(session);

	desc = gnutls_session_get_desc(session);
	success("  - connected with: %s\n", desc);
	gnutls_free(desc);
	/* do not wait for the peer to close the connection.
	 */
	gnutls_bye(session, GNUTLS_SHUT_WR);

	if (expect_fail)
		fail("server: expected failure but connection succeeded!\n");

	if (!no_cred) {
		if (binary_user) {
			char pskid_bin[1024], userdata_bin[1024];

			if (gnutls_psk_server_get_username2(session, &pskid_binary))
				fail("server: Could not get binary pskid\n");

			if (memcmp(pskid_binary.data, user->data, user->size) != 0) {
				hex_encode(user->data, user->size, userdata_bin, sizeof(userdata_bin));
				hex_encode(pskid_binary.data, pskid_binary.size, pskid_bin, sizeof(pskid_bin));
				fail("server: binary username (%s) does not match expected (%s)\n",
						pskid_bin, userdata_bin);
			}
		} else {
			pskid = gnutls_psk_server_get_username(session);
			if (pskid == NULL || strcmp(pskid, (const char *) user->data) != 0) {
				fail("server: username (%s), does not match expected (%s)\n",
				     pskid, (const char *) user->data);
			}
		}
	}

	if (exp_kx && kx != exp_kx) {
		fail("server: expected key exchange %s, but got %s\n",
		     gnutls_kx_get_name(exp_kx),
		     gnutls_kx_get_name(kx));
	}

 end:
	close(sd);
	gnutls_deinit(session);

	gnutls_psk_free_server_credentials(server_pskcred);

	gnutls_global_deinit();

	if (debug)
		success("server: finished\n");
}

static void print_user(const char *caption, const char *prio, const gnutls_datum_t *user, unsigned binary_user)
{
	char hexuser[100];

	if (binary_user) {
		hex_encode(user->data, user->size, hexuser, sizeof(hexuser));
		success("%s %s (user:%s)\n", caption, prio, hexuser);
	} else
		success("%s %s (user:%s)\n", caption, prio, (const char *) user->data);
}

static
void run_test3(const char *prio, const char *sprio, const gnutls_datum_t *user, const gnutls_datum_t *key, bool no_cred,
	      unsigned expect_hint, int exp_kx, int expect_fail_cli, int expect_fail_serv, unsigned binary_user)
{
	pid_t child;
	int err;
	int sockets[2];

	signal(SIGPIPE, SIG_IGN);

	if (expect_fail_serv || expect_fail_cli)
		print_user("ntest", prio, user, binary_user);
	else
		print_user("test", prio, user, binary_user);

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
		close(sockets[1]);
		int status;
		/* parent */
		server(sockets[0], sprio?sprio:prio, user, no_cred, expect_fail_serv, exp_kx, binary_user);
		wait(&status);
		check_wait_status(status);
	} else {
		close(sockets[0]);
		client(sockets[1], prio, user, key, expect_hint, expect_fail_cli, exp_kx, binary_user);
		exit(0);
	}
}

static
void run_test2(const char *prio, const char *sprio, const gnutls_datum_t *user, const gnutls_datum_t *key,
	      unsigned expect_hint, int exp_kx, int expect_fail_cli, int expect_fail_serv, unsigned binary_user)
{
	run_test3(prio, sprio, user, key, 0, expect_hint, exp_kx, expect_fail_cli, expect_fail_serv, binary_user);
}

static
void run_test_ok(const char *prio, const gnutls_datum_t *user, const gnutls_datum_t *key, unsigned expect_hint, int expect_fail, unsigned binary_user)
{
	run_test2(prio, NULL, user, key, expect_hint, GNUTLS_KX_PSK, expect_fail, expect_fail, binary_user);
}

static
void run_ectest_ok(const char *prio, const gnutls_datum_t *user, const gnutls_datum_t *key, unsigned expect_hint, int expect_fail, unsigned binary_user)
{
	run_test2(prio, NULL, user, key, expect_hint, GNUTLS_KX_ECDHE_PSK, expect_fail, expect_fail, binary_user);
}

static
void run_dhtest_ok(const char *prio, const gnutls_datum_t *user, const gnutls_datum_t *key, unsigned expect_hint, int expect_fail, unsigned binary_user)
{
	run_test2(prio, NULL, user, key, expect_hint, GNUTLS_KX_DHE_PSK, expect_fail, expect_fail, binary_user);
}

void doit(void)
{
	char hexuser[] = { 0xde, 0xad, 0xbe, 0xef },
			nulluser1[] = { 0 },
			nulluser2[] = { 0, 0, 0xaa, 0 };
	const gnutls_datum_t user_jas = { (void *) "jas", strlen("jas") };
	const gnutls_datum_t user_unknown = { (void *) "unknown", strlen("unknown") };
	const gnutls_datum_t user_nonhex = { (void *) "non-hex", strlen("non-hex") };
	const gnutls_datum_t user_hex = { (void *) hexuser, sizeof(hexuser) };
	const gnutls_datum_t user_null_1 = { (void *) nulluser1, sizeof(nulluser1) };
	const gnutls_datum_t user_null_2 = { (void *) nulluser2, sizeof(nulluser2) };
	const gnutls_datum_t key = { (void *) "9e32cf7786321a828ef7668f09fb35db", 32 };
	const gnutls_datum_t wrong_key = { (void *) "9e31cf7786321a828ef7668f09fb35db", 32 };

	run_test_ok("NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+PSK", &user_jas, &key, 1, 0, 0);
	run_test_ok("NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+PSK", &user_hex, &key, 1, 0, 1);
	run_test_ok("NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+PSK", &user_null_1, &key, 1, 0, 1);
	run_test_ok("NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+PSK", &user_null_2, &key, 1, 0, 1);
	run_dhtest_ok("NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+DHE-PSK", &user_jas, &key, 1, 0, 0);
	run_dhtest_ok("NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+DHE-PSK", &user_hex, &key, 1, 0, 1);
	run_dhtest_ok("NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+DHE-PSK", &user_null_1, &key, 1, 0, 1);
	run_dhtest_ok("NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+DHE-PSK", &user_null_2, &key, 1, 0, 1);
	run_ectest_ok("NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+ECDHE-PSK", &user_jas, &key, 1, 0, 0);
	run_ectest_ok("NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+ECDHE-PSK", &user_hex, &key, 1, 0, 1);
	run_ectest_ok("NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+ECDHE-PSK", &user_null_1, &key, 1, 0, 1);
	run_ectest_ok("NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+ECDHE-PSK", &user_null_2, &key, 1, 0, 1);
	run_test2("NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+PSK", NULL, &user_unknown, &key, 1, 0, GNUTLS_E_FATAL_ALERT_RECEIVED, GNUTLS_E_DECRYPTION_FAILED, 0);
	run_test2("NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+PSK", NULL, &user_jas, &wrong_key, 1, 0, GNUTLS_E_FATAL_ALERT_RECEIVED, GNUTLS_E_DECRYPTION_FAILED, 0);
	run_test2("NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+PSK", NULL, &user_nonhex, &key, 1, 0, GNUTLS_E_FATAL_ALERT_RECEIVED, GNUTLS_E_KEYFILE_ERROR, 0);
	run_test2("NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+PSK", NULL, &user_hex, &wrong_key, 1, 0, GNUTLS_E_FATAL_ALERT_RECEIVED, GNUTLS_E_DECRYPTION_FAILED, 1);
	run_test2("NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+PSK", NULL, &user_null_1, &wrong_key, 1, 0, GNUTLS_E_FATAL_ALERT_RECEIVED, GNUTLS_E_DECRYPTION_FAILED, 1);
	run_test2("NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+PSK", NULL, &user_null_2, &wrong_key, 1, 0, GNUTLS_E_FATAL_ALERT_RECEIVED, GNUTLS_E_DECRYPTION_FAILED, 1);

	run_test_ok("NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+PSK", &user_jas, &key, 1, 0, 0);
	run_test_ok("NORMAL:-KX-ALL:+PSK", &user_jas, &key, 0, 0, 0);
	run_test_ok("NORMAL:-KX-ALL:+PSK", &user_hex, &key, 0, 0, 1);
	run_test_ok("NORMAL:-KX-ALL:+PSK", &user_null_1, &key, 0, 0, 1);
	run_test_ok("NORMAL:-KX-ALL:+PSK", &user_null_2, &key, 0, 0, 1);
	run_test2("NORMAL:+PSK", NULL, &user_unknown, &key, 1, 0, GNUTLS_E_FATAL_ALERT_RECEIVED, GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER, 0);
	run_test2("NORMAL:+PSK", NULL, &user_jas, &wrong_key, 1, 0, GNUTLS_E_FATAL_ALERT_RECEIVED, GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER, 0);
	run_test2("NORMAL:+PSK", NULL, &user_hex, &wrong_key, 1, 0, GNUTLS_E_FATAL_ALERT_RECEIVED, GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER, 1);
	run_test2("NORMAL:+PSK", NULL, &user_null_1, &wrong_key, 1, 0, GNUTLS_E_FATAL_ALERT_RECEIVED, GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER, 1);
	run_test2("NORMAL:+PSK", NULL, &user_null_2, &wrong_key, 1, 0, GNUTLS_E_FATAL_ALERT_RECEIVED, GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER, 1);
	run_test2("NORMAL:-KX-ALL:+PSK", NULL, &user_nonhex, &key, 1, 0, GNUTLS_E_FATAL_ALERT_RECEIVED, GNUTLS_E_KEYFILE_ERROR, 0);

	run_dhtest_ok("NORMAL:-VERS-ALL:+VERS-TLS1.3:+DHE-PSK:-GROUP-EC-ALL", &user_jas, &key, 0, 0, 0);
	run_dhtest_ok("NORMAL:-VERS-ALL:+VERS-TLS1.3:+DHE-PSK:-GROUP-EC-ALL", &user_hex, &key, 0, 0, 1);
	run_dhtest_ok("NORMAL:-VERS-ALL:+VERS-TLS1.3:+DHE-PSK:-GROUP-EC-ALL", &user_null_1, &key, 0, 0, 1);
	run_dhtest_ok("NORMAL:-VERS-ALL:+VERS-TLS1.3:+DHE-PSK:-GROUP-EC-ALL", &user_null_2, &key, 0, 0, 1);
	run_test_ok("NORMAL:-VERS-ALL:+VERS-TLS1.3:+PSK", &user_jas, &key, 0, 0, 0);
	run_test_ok("NORMAL:-VERS-ALL:+VERS-TLS1.3:+PSK", &user_hex, &key, 0, 0, 1);
	run_test_ok("NORMAL:-VERS-ALL:+VERS-TLS1.3:+PSK", &user_null_1, &key, 0, 0, 1);
	run_test_ok("NORMAL:-VERS-ALL:+VERS-TLS1.3:+PSK", &user_null_2, &key, 0, 0, 1);

	/* test priorities of DHE-PSK and PSK */
	run_ectest_ok("NORMAL:-VERS-ALL:+VERS-TLS1.3:-CIPHER-ALL:+AES-128-GCM:+DHE-PSK:+PSK:-GROUP-DH-ALL", &user_jas, &key, 0, 0, 0);
	run_ectest_ok("NORMAL:-VERS-ALL:+VERS-TLS1.3:-CIPHER-ALL:+AES-128-GCM:+DHE-PSK:+PSK:-GROUP-DH-ALL", &user_hex, &key, 0, 0, 1);
	run_ectest_ok("NORMAL:-VERS-ALL:+VERS-TLS1.3:-CIPHER-ALL:+AES-128-GCM:+DHE-PSK:+PSK:-GROUP-DH-ALL", &user_null_1, &key, 0, 0, 1);
	run_ectest_ok("NORMAL:-VERS-ALL:+VERS-TLS1.3:-CIPHER-ALL:+AES-128-GCM:+DHE-PSK:+PSK:-GROUP-DH-ALL", &user_null_2, &key, 0, 0, 1);
	run_test_ok("NORMAL:-VERS-ALL:+VERS-TLS1.3:-CIPHER-ALL:+AES-128-GCM:+PSK:+DHE-PSK:-GROUP-DH-ALL", &user_jas, &key, 0, 0, 0);
	run_test_ok("NORMAL:-VERS-ALL:+VERS-TLS1.3:-CIPHER-ALL:+AES-128-GCM:+PSK:+DHE-PSK:-GROUP-DH-ALL", &user_hex, &key, 0, 0, 1);
	run_test_ok("NORMAL:-VERS-ALL:+VERS-TLS1.3:-CIPHER-ALL:+AES-128-GCM:+PSK:+DHE-PSK:-GROUP-DH-ALL", &user_null_1, &key, 0, 0, 1);
	run_test_ok("NORMAL:-VERS-ALL:+VERS-TLS1.3:-CIPHER-ALL:+AES-128-GCM:+PSK:+DHE-PSK:-GROUP-DH-ALL", &user_null_2, &key, 0, 0, 1);
	run_test2("NORMAL:-VERS-ALL:+VERS-TLS1.3:-CIPHER-ALL:+AES-128-GCM:+DHE-PSK:+PSK:-GROUP-DH-ALL", 
		  "NORMAL:-VERS-ALL:+VERS-TLS1.3:-CIPHER-ALL:+AES-128-GCM:+PSK:+DHE-PSK:%SERVER_PRECEDENCE:-GROUP-DH-ALL",
		  &user_jas, &key, 0, GNUTLS_KX_PSK, 0, 0, 0);
	run_test2("NORMAL:-VERS-ALL:+VERS-TLS1.3:-CIPHER-ALL:+AES-128-GCM:+DHE-PSK:+PSK:-GROUP-DH-ALL",
		  "NORMAL:-VERS-ALL:+VERS-TLS1.3:-CIPHER-ALL:+AES-128-GCM:+PSK:+DHE-PSK:%SERVER_PRECEDENCE:-GROUP-DH-ALL",
		  &user_hex, &key, 0, GNUTLS_KX_PSK, 0, 0, 1);
	run_test2("NORMAL:-VERS-ALL:+VERS-TLS1.3:-CIPHER-ALL:+AES-128-GCM:+DHE-PSK:+PSK:-GROUP-DH-ALL",
		  "NORMAL:-VERS-ALL:+VERS-TLS1.3:-CIPHER-ALL:+AES-128-GCM:+PSK:+DHE-PSK:%SERVER_PRECEDENCE:-GROUP-DH-ALL",
		  &user_null_1, &key, 0, GNUTLS_KX_PSK, 0, 0, 1);
	run_test2("NORMAL:-VERS-ALL:+VERS-TLS1.3:-CIPHER-ALL:+AES-128-GCM:+DHE-PSK:+PSK:-GROUP-DH-ALL",
		  "NORMAL:-VERS-ALL:+VERS-TLS1.3:-CIPHER-ALL:+AES-128-GCM:+PSK:+DHE-PSK:%SERVER_PRECEDENCE:-GROUP-DH-ALL",
		  &user_null_2, &key, 0, GNUTLS_KX_PSK, 0, 0, 1);
	/* try with PRF that doesn't match binder (SHA256) */
	run_test2("NORMAL:-VERS-ALL:+VERS-TLS1.3:-CIPHER-ALL:+AES-256-GCM:+PSK:+DHE-PSK", NULL, &user_jas, &key, 0, 0, GNUTLS_E_FATAL_ALERT_RECEIVED, GNUTLS_E_NO_CIPHER_SUITES, 0);
	run_test2("NORMAL:-VERS-ALL:+VERS-TLS1.3:-CIPHER-ALL:+AES-256-GCM:+PSK:+DHE-PSK", NULL, &user_hex, &key, 0, 0, GNUTLS_E_FATAL_ALERT_RECEIVED, GNUTLS_E_NO_CIPHER_SUITES, 1);
	run_test2("NORMAL:-VERS-ALL:+VERS-TLS1.3:-CIPHER-ALL:+AES-256-GCM:+PSK:+DHE-PSK", NULL, &user_null_1, &key, 0, 0, GNUTLS_E_FATAL_ALERT_RECEIVED, GNUTLS_E_NO_CIPHER_SUITES, 1);
	run_test2("NORMAL:-VERS-ALL:+VERS-TLS1.3:-CIPHER-ALL:+AES-256-GCM:+PSK:+DHE-PSK", NULL, &user_null_2, &key, 0, 0, GNUTLS_E_FATAL_ALERT_RECEIVED, GNUTLS_E_NO_CIPHER_SUITES, 1);
	/* try with no groups and PSK */
	run_test_ok("NORMAL:-VERS-ALL:+VERS-TLS1.3:+PSK:-GROUP-ALL", &user_jas, &key, 0, 0, 0);
	run_test_ok("NORMAL:-VERS-ALL:+VERS-TLS1.3:+PSK:-GROUP-ALL", &user_hex, &key, 0, 0, 1);
	run_test_ok("NORMAL:-VERS-ALL:+VERS-TLS1.3:+PSK:-GROUP-ALL", &user_null_1, &key, 0, 0, 1);
	run_test_ok("NORMAL:-VERS-ALL:+VERS-TLS1.3:+PSK:-GROUP-ALL", &user_null_2, &key, 0, 0, 1);
	/* try without any groups but DHE-PSK */
	run_test2("NORMAL:-VERS-ALL:+VERS-TLS1.3:+DHE-PSK:-GROUP-ALL", "NORMAL:-VERS-ALL:+VERS-TLS1.3:+DHE-PSK:+PSK", &user_jas, &key, 0, 0, GNUTLS_E_FATAL_ALERT_RECEIVED, GNUTLS_E_NO_COMMON_KEY_SHARE, 0);
	run_test2("NORMAL:-VERS-ALL:+VERS-TLS1.3:+DHE-PSK:-GROUP-ALL", "NORMAL:-VERS-ALL:+VERS-TLS1.3:+DHE-PSK:+PSK:-GROUP-ALL", &user_jas, &key, 0, 0, GNUTLS_E_FATAL_ALERT_RECEIVED, GNUTLS_E_NO_COMMON_KEY_SHARE, 0);
	run_test2("NORMAL:-VERS-ALL:+VERS-TLS1.3:+DHE-PSK:-GROUP-ALL", "NORMAL:-VERS-ALL:+VERS-TLS1.3:+DHE-PSK:+PSK", &user_hex, &key, 0, 0, GNUTLS_E_FATAL_ALERT_RECEIVED, GNUTLS_E_NO_COMMON_KEY_SHARE, 1);
	run_test2("NORMAL:-VERS-ALL:+VERS-TLS1.3:+DHE-PSK:-GROUP-ALL", "NORMAL:-VERS-ALL:+VERS-TLS1.3:+DHE-PSK:+PSK:-GROUP-ALL", &user_hex, &key, 0, 0, GNUTLS_E_FATAL_ALERT_RECEIVED, GNUTLS_E_NO_COMMON_KEY_SHARE, 1);
	run_test2("NORMAL:-VERS-ALL:+VERS-TLS1.3:+DHE-PSK:-GROUP-ALL", "NORMAL:-VERS-ALL:+VERS-TLS1.3:+DHE-PSK:+PSK", &user_null_1, &key, 0, 0, GNUTLS_E_FATAL_ALERT_RECEIVED, GNUTLS_E_NO_COMMON_KEY_SHARE, 1);
	run_test2("NORMAL:-VERS-ALL:+VERS-TLS1.3:+DHE-PSK:-GROUP-ALL", "NORMAL:-VERS-ALL:+VERS-TLS1.3:+DHE-PSK:+PSK:-GROUP-ALL", &user_null_1, &key, 0, 0, GNUTLS_E_FATAL_ALERT_RECEIVED, GNUTLS_E_NO_COMMON_KEY_SHARE, 1);
	run_test2("NORMAL:-VERS-ALL:+VERS-TLS1.3:+DHE-PSK:-GROUP-ALL", "NORMAL:-VERS-ALL:+VERS-TLS1.3:+DHE-PSK:+PSK", &user_null_2, &key, 0, 0, GNUTLS_E_FATAL_ALERT_RECEIVED, GNUTLS_E_NO_COMMON_KEY_SHARE, 1);
	run_test2("NORMAL:-VERS-ALL:+VERS-TLS1.3:+DHE-PSK:-GROUP-ALL", "NORMAL:-VERS-ALL:+VERS-TLS1.3:+DHE-PSK:+PSK:-GROUP-ALL", &user_null_2, &key, 0, 0, GNUTLS_E_FATAL_ALERT_RECEIVED, GNUTLS_E_NO_COMMON_KEY_SHARE, 1);

	/* if user invalid we continue without PSK */
	run_test2("NORMAL:-VERS-ALL:+VERS-TLS1.3:+PSK:+DHE-PSK", NULL, &user_nonhex, &key, 0, 0, GNUTLS_E_FATAL_ALERT_RECEIVED, GNUTLS_E_KEYFILE_ERROR, 0);
	run_test2("NORMAL:-VERS-ALL:+VERS-TLS1.3:+PSK:+DHE-PSK", NULL, &user_unknown, &key, 0, 0, GNUTLS_E_FATAL_ALERT_RECEIVED, GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER, 0);
	run_test2("NORMAL:-VERS-ALL:+VERS-TLS1.3:+PSK:+DHE-PSK", NULL, &user_jas, &wrong_key, 0, 0, GNUTLS_E_FATAL_ALERT_RECEIVED, GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER, 0);
	run_test2("NORMAL:-VERS-ALL:+VERS-TLS1.3:+PSK:+DHE-PSK", NULL, &user_hex, &wrong_key, 0, 0, GNUTLS_E_FATAL_ALERT_RECEIVED, GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER, 1);
	run_test2("NORMAL:-VERS-ALL:+VERS-TLS1.3:+PSK:+DHE-PSK", NULL, &user_null_1, &wrong_key, 0, 0, GNUTLS_E_FATAL_ALERT_RECEIVED, GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER, 1);
	run_test2("NORMAL:-VERS-ALL:+VERS-TLS1.3:+PSK:+DHE-PSK", NULL, &user_null_2, &wrong_key, 0, 0, GNUTLS_E_FATAL_ALERT_RECEIVED, GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER, 1);

	/* try with HelloRetryRequest and PSK */
	run_test2("NORMAL:-VERS-ALL:+VERS-TLS1.3:+DHE-PSK:-GROUP-ALL:+GROUP-FFDHE2048:+GROUP-FFDHE4096", "NORMAL:-VERS-ALL:+VERS-TLS1.3:+DHE-PSK:-GROUP-ALL:+GROUP-FFDHE4096", &user_jas, &key, 0, GNUTLS_KX_DHE_PSK, 0, 0, 0);
	run_test2("NORMAL:-VERS-ALL:+VERS-TLS1.3:+DHE-PSK:-GROUP-ALL:+GROUP-FFDHE2048:+GROUP-FFDHE4096", "NORMAL:-VERS-ALL:+VERS-TLS1.3:+DHE-PSK:-GROUP-ALL:+GROUP-FFDHE4096", &user_hex, &key, 0, GNUTLS_KX_DHE_PSK, 0, 0, 1);
	run_test2("NORMAL:-VERS-ALL:+VERS-TLS1.3:+DHE-PSK:-GROUP-ALL:+GROUP-FFDHE2048:+GROUP-FFDHE4096", "NORMAL:-VERS-ALL:+VERS-TLS1.3:+DHE-PSK:-GROUP-ALL:+GROUP-FFDHE4096", &user_null_1, &key, 0, GNUTLS_KX_DHE_PSK, 0, 0, 1);
	run_test2("NORMAL:-VERS-ALL:+VERS-TLS1.3:+DHE-PSK:-GROUP-ALL:+GROUP-FFDHE2048:+GROUP-FFDHE4096", "NORMAL:-VERS-ALL:+VERS-TLS1.3:+DHE-PSK:-GROUP-ALL:+GROUP-FFDHE4096", &user_null_2, &key, 0, GNUTLS_KX_DHE_PSK, 0, 0, 1);

	/* try without server credentials */
	run_test3("NORMAL:-VERS-ALL:+VERS-TLS1.3:+PSK:+DHE-PSK", NULL, &user_jas, &key, 1, 0, 0, GNUTLS_E_FATAL_ALERT_RECEIVED, GNUTLS_E_INSUFFICIENT_CREDENTIALS, 0);
	run_test3("NORMAL:-VERS-ALL:+VERS-TLS1.3:+PSK:+DHE-PSK", NULL, &user_hex, &key, 1, 0, 0, GNUTLS_E_FATAL_ALERT_RECEIVED, GNUTLS_E_INSUFFICIENT_CREDENTIALS, 1);
	run_test3("NORMAL:-VERS-ALL:+VERS-TLS1.3:+PSK:+DHE-PSK", NULL, &user_null_1, &key, 1, 0, 0, GNUTLS_E_FATAL_ALERT_RECEIVED, GNUTLS_E_INSUFFICIENT_CREDENTIALS, 1);
	run_test3("NORMAL:-VERS-ALL:+VERS-TLS1.3:+PSK:+DHE-PSK", NULL, &user_null_2, &key, 1, 0, 0, GNUTLS_E_FATAL_ALERT_RECEIVED, GNUTLS_E_INSUFFICIENT_CREDENTIALS, 1);
}

#endif				/* _WIN32 */
