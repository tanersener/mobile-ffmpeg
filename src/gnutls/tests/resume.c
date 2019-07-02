/*
 * Copyright (C) 2004-2016 Free Software Foundation, Inc.
 * Copyright (C) 2013 Adam Sampson <ats@offog.org>
 * Copyright (C) 2016-2018 Red Hat, Inc.
 *
 * Author: Simon Josefsson, Nikos Mavrogiannopoulos
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
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>
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

#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#if !defined(_WIN32)
#include <sys/wait.h>
#endif
#include <unistd.h>
#include <gnutls/gnutls.h>
#include <sys/wait.h>
#include <signal.h>
#include <assert.h>
#include "utils.h"
#include "cert-common.h"
#include "virt-time.h"

static void wrap_db_init(void);
static void wrap_db_deinit(void);
static int wrap_db_store(void *dbf, gnutls_datum_t key,
			 gnutls_datum_t data);
static gnutls_datum_t wrap_db_fetch(void *dbf, gnutls_datum_t key);
static int wrap_db_delete(void *dbf, gnutls_datum_t key);

#define TLS_SESSION_CACHE 50

struct params_res {
	const char *desc;
	int enable_db;
	int enable_session_ticket_server;
	int enable_session_ticket_client;
	int expect_resume;
	int call_post_client_hello;
	int client_cert;
	int first_no_ext_master;
	int second_no_ext_master;
	int try_alpn;
	int try_resumed_data;
	int try_diff_sni;
	int try_sni;
	int expire_ticket;
	int change_ciphersuite;
	int early_start;
	int no_early_start;
};

pid_t child;

struct params_res resume_tests[] = {
#ifndef TLS13
	{.desc = "try to resume from db",
	 .enable_db = 1,
	 .enable_session_ticket_server = 0,
	 .enable_session_ticket_client = 0,
	 .expect_resume = 1},
	{.desc = "try to resume from db with post_client_hello",
	 .enable_db = 1,
	 .enable_session_ticket_server = 0,
	 .enable_session_ticket_client = 0,
	 .call_post_client_hello = 1,
	 .expect_resume = 1},
	{.desc = "try to resume from db using resumed session's data",
	 .enable_db = 1,
	 .enable_session_ticket_server = 0,
	 .enable_session_ticket_client = 0,
	 .try_resumed_data = 1,
	 .expect_resume = 1},
	{.desc = "try to resume from db and check ALPN",
	 .enable_db = 1,
	 .enable_session_ticket_server = 0,
	 .enable_session_ticket_client = 0,
	 .try_alpn = 1,
	 .expect_resume = 1},
	{.desc = "try to resume from db (ext master secret -> none)",
	 .enable_db = 1,
	 .enable_session_ticket_server = 0,
	 .enable_session_ticket_client = 0,
	 .expect_resume = 0,
	 .first_no_ext_master = 0,
	 .second_no_ext_master = 1},
	{.desc = "try to resume from db (none -> ext master secret)",
	 .enable_db = 1,
	 .enable_session_ticket_server = 0,
	 .enable_session_ticket_client = 0,
	 .expect_resume = 0,
	 .first_no_ext_master = 1,
	 .second_no_ext_master = 0},
#endif
#if defined(TLS13)
	/* only makes sense under TLS1.3 as negotiation involves a new
	 * handshake with different parameters */
	{.desc = "try to resume from session ticket (different cipher order)",
	 .enable_db = 0,
	 .enable_session_ticket_server = 1,
	 .enable_session_ticket_client = 1,
	 .change_ciphersuite = 1,
	 .expect_resume = 1},
	{.desc = "try to resume from session ticket with post_client_hello",
	 .enable_db = 0,
	 .enable_session_ticket_server = 1,
	 .enable_session_ticket_client = 1,
	 .call_post_client_hello = 1,
	 .expect_resume = 1},
#endif
#if defined(TLS13) && !defined(USE_PSK)
	{.desc = "try to resume from session ticket (early start)",
	 .enable_db = 0,
	 .enable_session_ticket_server = 1,
	 .enable_session_ticket_client = 1,
	 .early_start = 1,
	 .expect_resume = 1},
#endif
#if defined(TLS13) && defined(USE_PSK)
	/* early start should no happen on PSK. */
	{.desc = "try to resume from session ticket (early start)",
	 .enable_db = 0,
	 .enable_session_ticket_server = 1,
	 .enable_session_ticket_client = 1,
	 .no_early_start = 1,
	 .expect_resume = 1},
#endif
	{.desc = "try to resume from session ticket",
	 .enable_db = 0,
	 .enable_session_ticket_server = 1,
	 .enable_session_ticket_client = 1,
	 .expect_resume = 1},
	{.desc = "try to resume from session ticket (client cert)",
	 .enable_db = 0,
	 .client_cert = 1,
	 .enable_session_ticket_server = 1,
	 .enable_session_ticket_client = 1,
	 .expect_resume = 1},
	{.desc = "try to resume from session ticket (expired)",
	 .enable_db = 0,
	 .enable_session_ticket_server = 1,
	 .enable_session_ticket_client = 1,
	 .expire_ticket = 1,
	 .expect_resume = 0},
	{.desc = "try to resume from session ticket using resumed session's data",
	 .enable_db = 0,
	 .enable_session_ticket_server = 1,
	 .enable_session_ticket_client = 1,
	 .try_resumed_data = 1,
	 .expect_resume = 1},
#ifndef TLS13
	{.desc = "try to resume from session ticket (ext master secret -> none)",
	 .enable_db = 0,
	 .enable_session_ticket_server = 1,
	 .enable_session_ticket_client = 1,
	 .expect_resume = 0,
	 .first_no_ext_master = 0,
	 .second_no_ext_master = 1},
	{.desc = "try to resume from session ticket (none -> ext master secret)",
	 .enable_db = 0,
	 .enable_session_ticket_server = 1,
	 .enable_session_ticket_client = 1,
	 .expect_resume = 0,
	 .first_no_ext_master = 1,
	 .second_no_ext_master = 0},
	{.desc = "try to resume from session ticket (server only)",
	  .enable_db = 0,
	  .enable_session_ticket_server = 1,
	  .enable_session_ticket_client = 0,
	  .expect_resume = 0},
	{.desc = "try to resume from session ticket (client only)",
	 .enable_db = 0,
	 .enable_session_ticket_server = 0,
	 .enable_session_ticket_client = 1,
	 .expect_resume = 0},
	{.desc = "try to resume from db and ticket",
	 .enable_db = 1,
	 .enable_session_ticket_server = 1,
	 .enable_session_ticket_client = 1,
	 .expect_resume = 1},
	{.desc = "try to resume from db and different SNI",
	 .enable_db = 1,
	 .try_sni = 1,
	 .try_diff_sni = 1,
	 .expect_resume = 0},
	{.desc = "try to resume with ticket and different SNI",
	 .enable_session_ticket_server = 1,
	 .enable_session_ticket_client = 1,
	 .try_sni = 1,
	 .try_diff_sni = 1,
	 .expect_resume = 0},
	{.desc = "try to resume from db and same SNI",
	 .enable_db = 1,
	 .try_sni = 1,
	 .expect_resume = 1},
#endif
	{.desc = "try to resume with ticket and same SNI",
	 .enable_session_ticket_server = 1,
	 .enable_session_ticket_client = 1,
	 .try_sni = 1,
	 .expect_resume = 1},
	{NULL, -1}
};

/* A very basic TLS client, with anonymous authentication.
 */

#define SESSIONS 3
#define MAX_BUF 5*1024
#define MSG "Hello TLS"

#define HANDSHAKE_SESSION_ID_POS (2+32)

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "%s |<%d>| %s", child ? "server" : "client", level,
		str);
}

static int post_client_hello_callback(gnutls_session_t session)
{
	/* switches the supported ciphersuites to something compatible */
	assert(gnutls_priority_set_direct(session, gnutls_session_get_ptr(session), NULL) >= 0);
	return 0;
}

static int hsk_hook_cb(gnutls_session_t session, unsigned int htype, unsigned post,
			unsigned int incoming, const gnutls_datum_t *_msg)
{
	unsigned size;
	gnutls_datum_t msg = {_msg->data, _msg->size};

	/* skip up to session ID */
	if (msg.size <= HANDSHAKE_SESSION_ID_POS+6) {
		fail("Cannot parse server hello\n");
		return -1;
	}

	msg.data += HANDSHAKE_SESSION_ID_POS;
	msg.size -= HANDSHAKE_SESSION_ID_POS;
	size = msg.data[0];

	if (msg.size <= size) {
		fail("Cannot parse server hello 2\n");
		return -1;
	}

	msg.data += size;
	msg.size -= size;

	if (memmem(msg.data, msg.size, "\x00\x17\x00\x00", 4) == 0) {
		fail("Extended master secret extension was not found in resumed session hello\n");
		exit(1);
	}
	return 0;
}

static void append_alpn(gnutls_session_t session, struct params_res *params, unsigned alpn_counter)
{
	gnutls_datum_t protocol;
	int ret;
	char str[64];

	if (!params->try_alpn)
		return;

	snprintf(str, sizeof(str), "myproto-%d", alpn_counter);

	protocol.data = (void*)str;
	protocol.size = strlen(str);

	ret = gnutls_alpn_set_protocols(session, &protocol, 1, 0);
	if (ret < 0) {
		gnutls_perror(ret);
		exit(1);
	}
}

static void verify_alpn(gnutls_session_t session, struct params_res *params, unsigned alpn_counter)
{
	int ret;
	gnutls_datum_t selected;
	char str[64];

	if (!params->try_alpn)
		return;

	snprintf(str, sizeof(str), "myproto-%d", alpn_counter);

	ret = gnutls_alpn_get_selected_protocol(session, &selected);
	if (ret < 0) {
		gnutls_perror(ret);
		exit(1);
	}

	if (strlen(str) != selected.size || memcmp(str, selected.data, selected.size) != 0) {
		fail("expected protocol %s, got %.*s\n", str, selected.size, selected.data);
		exit(1);
	}

	if (debug)
		success("ALPN got: %s\n", str);
}

static void verify_group(gnutls_session_t session, gnutls_group_t *group, unsigned counter)
{
	if (counter == 0) {
		*group = gnutls_group_get(session);
		return;
	}

	if (gnutls_group_get(session) != *group) {
		fail("expected group %s, got group %s\n", gnutls_group_get_name(*group),
		     gnutls_group_get_name(gnutls_group_get(session)));
	}
}

static void verify_server_params(gnutls_session_t session, unsigned counter, struct params_res *params)
{
	static char id[GNUTLS_MAX_SESSION_ID];
	static size_t id_size = 0;
#if defined(USE_PSK)
	const char *username;
	username = gnutls_psk_server_get_username(session);
	if (counter != 0) {
		if (username == NULL)
			fail("no username was returned on server side resumption\n");

		if (strcmp(username, "test") != 0)
			fail("wrong username was returned on server side resumption\n");
	}
#endif

	if (counter == 0 && params->early_start) {
		if (!(gnutls_session_get_flags(session) & GNUTLS_SFLAGS_EARLY_START)) {
			fail("early start did not happen on %d!\n", counter);
		}
	}

	if (counter > 0) {
		if (gnutls_session_resumption_requested(session) == 0) {
			fail("client did not request resumption!\n");
		}
	}

	if (params->no_early_start) {
		if (gnutls_session_get_flags(session) & GNUTLS_SFLAGS_EARLY_START) {
			fail("early start did happen on %d but was not expected!\n", counter);
		}
	}

#if defined(USE_X509)
	unsigned int l;

	if (gnutls_certificate_type_get(session) != GNUTLS_CRT_X509)
		fail("did not find the expected X509 certificate type! (%d)\n", gnutls_certificate_type_get(session));

	if (counter == 0 && gnutls_certificate_get_ours(session) == NULL)
		fail("no certificate returned on server side (%s)\n", counter ? "resumed session" : "first session");
	else if (counter != 0 && gnutls_certificate_get_ours(session) != NULL)
		fail("certificate was returned on server side (%s)\n", counter ? "resumed session" : "first session");

	if (params->client_cert) {
		if (gnutls_certificate_get_peers(session, &l) == NULL || l < 1)
			fail("no client certificate returned on server side (%s)\n", counter ? "resumed session" : "first session");
	}
#endif

	/* verify whether the session ID remains the same between sessions */
	if (counter == 0) {
		id_size = sizeof(id);
		assert(gnutls_session_get_id(session, id, &id_size) >= 0);
	} else {
		char id2[GNUTLS_MAX_SESSION_ID];
		size_t id2_size = sizeof(id2);

		if (id_size == 0)
			fail("no session ID was set\n");

		assert(gnutls_session_get_id(session, id2, &id2_size) >= 0);

		if (id_size != id2_size || memcmp(id, id2, id_size) != 0) {
			hexprint(id, id_size);
			printf("\n");
			hexprint(id2, id2_size);
			fail("resumed session ID does not match original\n");
		}
	}

	return;
}

static void verify_client_params(gnutls_session_t session, unsigned counter)
{
#if defined(USE_X509)
	unsigned int l;
	if (gnutls_certificate_get_peers(session, &l) == NULL || l < 1)
		fail("no server certificate returned on client side (%s)\n", counter ? "resumed session" : "first session");
#else
	return;
#endif
}

#ifdef TLS12
# define VERS_STR "+VERS-TLS1.2"
#endif
#ifdef TLS13
# define VERS_STR "-VERS-ALL:+VERS-TLS1.3"
#endif

static void client(int sds[], struct params_res *params)
{
	int ret, ii;
	gnutls_session_t session;
	char buffer[MAX_BUF + 1];
	unsigned int ext_master_secret_check = 0;
	gnutls_group_t pgroup;
	char prio_str[256];
	const char *dns_name1 = "example.com";
	const char *dns_name2 = "www.example.com";
#ifdef USE_PSK
# define PRIO_STR "NONE:"VERS_STR":+CIPHER-ALL:+MAC-ALL:+SIGN-ALL:+COMP-ALL:+PSK:+CURVE-ALL"
	const gnutls_datum_t pskkey = { (void *) "DEADBEEF", 8 };
	gnutls_psk_client_credentials_t pskcred;
#elif defined(USE_ANON)
# define PRIO_STR "NONE:"VERS_STR":+CIPHER-ALL:+MAC-ALL:+SIGN-ALL:+COMP-ALL:+ANON-ECDH:+ANON-DH:+CURVE-ALL"
	gnutls_anon_client_credentials_t anoncred;
#elif defined(USE_X509)
# define PRIO_STR "NONE:"VERS_STR":+CIPHER-ALL:+MAC-ALL:+SIGN-ALL:+COMP-ALL:+ECDHE-RSA:+RSA:+CURVE-ALL"
	gnutls_certificate_credentials_t clientx509cred;
#endif

	/* Need to enable anonymous KX specifically. */

	/* variables used in session resuming
	 */
	int t;
	gnutls_datum_t session_data = {NULL, 0};

	if (debug) {
		gnutls_global_set_log_function(tls_log_func);
		gnutls_global_set_log_level(4);
	}

#ifdef USE_PSK
	gnutls_psk_allocate_client_credentials(&pskcred);
	gnutls_psk_set_client_credentials(pskcred, "test", &pskkey, GNUTLS_PSK_KEY_HEX);
#elif defined(USE_ANON)
	gnutls_anon_allocate_client_credentials(&anoncred);
#elif defined(USE_X509)
	gnutls_certificate_allocate_credentials(&clientx509cred);

	if (params->client_cert) {
		assert(gnutls_certificate_set_x509_key_mem(clientx509cred,
							   &cli_cert, &cli_key,
							   GNUTLS_X509_FMT_PEM) >= 0);
	}
#endif

	for (t = 0; t < SESSIONS; t++) {
		int sd = sds[t];

		assert(gnutls_init(&session, GNUTLS_CLIENT)>=0);

		snprintf(prio_str, sizeof(prio_str), "%s", PRIO_STR);

		/* Use default priorities */
		if (params->enable_session_ticket_client == 0) {
			strcat(prio_str, ":%NO_TICKETS");
		}

		if (params->first_no_ext_master && t == 0) {
			strcat(prio_str, ":%NO_SESSION_HASH");
			ext_master_secret_check = 0;
		}

		if (params->second_no_ext_master && t > 0) {
			strcat(prio_str, ":%NO_SESSION_HASH");
			ext_master_secret_check = 0;
		}

		if (params->change_ciphersuite) {
			if (t > 0)
				strcat(prio_str, ":-CIPHER-ALL:+AES-256-GCM:+AES-128-GCM");
			else
				strcat(prio_str, ":-CIPHER-ALL:+AES-128-GCM");
		}

		append_alpn(session, params, t);

		ret = gnutls_priority_set_direct(session, prio_str, NULL);
		if (ret < 0) {
			fail("prio: %s\n", gnutls_strerror(ret));
		}

		/* put the anonymous credentials to the current session
		 */
#ifdef USE_PSK
		gnutls_credentials_set(session, GNUTLS_CRD_PSK, pskcred);
#elif defined(USE_ANON)
		gnutls_credentials_set(session, GNUTLS_CRD_ANON, anoncred);
#elif defined(USE_X509)
		gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, clientx509cred);
#endif

		if (t > 0) {
			/* if this is not the first time we connect */
			gnutls_session_set_data(session, session_data.data,
						session_data.size);
			if (params->try_diff_sni)
				gnutls_server_name_set(session, GNUTLS_NAME_DNS, dns_name1, strlen(dns_name1));
			else if (params->try_sni)
				gnutls_server_name_set(session, GNUTLS_NAME_DNS, dns_name2, strlen(dns_name2));

		} else {
			if (params->try_sni)
				gnutls_server_name_set(session, GNUTLS_NAME_DNS, dns_name2, strlen(dns_name2));
		}

		if (ext_master_secret_check)
			gnutls_handshake_set_hook_function(session, GNUTLS_HANDSHAKE_SERVER_HELLO, GNUTLS_HOOK_PRE, hsk_hook_cb);
		gnutls_transport_set_int(session, sd);

		/* Perform the TLS handshake
		 */
		gnutls_handshake_set_timeout(session, 20 * 1000);
		do {
			ret = gnutls_handshake(session);
		} while (ret < 0 && gnutls_error_is_fatal(ret) == 0);

		if (ret < 0) {
			fail("client: Handshake failed\n");
			gnutls_perror(ret);
			goto end;
		} else {
			if (debug)
				success
				    ("client: Handshake was completed\n");
		}

		ext_master_secret_check = 0;
		if (t == 0) {
			ext_master_secret_check =  gnutls_session_ext_master_secret_status(session);

			/* get the session data size */
			ret =
			    gnutls_session_get_data2(session,
						     &session_data);
			if (ret < 0)
				fail("Getting resume data failed\n");

		} else {	/* the second time we connect */
			if (params->try_resumed_data) {
				gnutls_free(session_data.data);
				ret =
				    gnutls_session_get_data2(session,
							     &session_data);
				if (ret < 0)
					fail("Getting resume data failed\n");
			}

			/* check if we actually resumed the previous session */
			if (gnutls_session_is_resumed(session) != 0) {
				if (params->expect_resume) {
					if (debug)
						success
						    ("- Previous session was resumed\n");
				} else
					fail("- Previous session was resumed but NOT expected\n");
			} else {
				if (params->expect_resume) {
					fail("*** Previous session was NOT resumed\n");
				} else {
					if (debug)
						success
						    ("*** Previous session was NOT resumed (expected)\n");
				}
			}

			if (params->change_ciphersuite) {
				/* check if the expected cipher was negotiated */
				if (gnutls_cipher_get(session) != GNUTLS_CIPHER_AES_128_GCM) {
					fail("negotiated different cipher: %s\n",
					     gnutls_cipher_get_name(gnutls_cipher_get(session)));
				}
			}
		}

		verify_alpn(session, params, t);
		verify_group(session, &pgroup, t);

		if (params->expect_resume)
			verify_client_params(session, t);

		gnutls_record_send(session, MSG, strlen(MSG));

		do {
			ret = gnutls_record_recv(session, buffer, MAX_BUF);
		} while (ret == GNUTLS_E_AGAIN);
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
	gnutls_free(session_data.data);

      end:
#ifdef USE_PSK
	gnutls_psk_free_client_credentials(pskcred);
#elif defined(USE_ANON)
	gnutls_anon_free_client_credentials(anoncred);
#elif defined(USE_X509)
	gnutls_certificate_free_credentials(clientx509cred);
#endif
}

#define DH_BITS 1024

/* These are global */
static gnutls_datum_t session_ticket_key = { NULL, 0 };


static gnutls_dh_params_t dh_params;

#ifdef USE_PSK
gnutls_psk_server_credentials_t pskcred;
#elif defined(USE_ANON)
gnutls_anon_server_credentials_t anoncred;
#elif defined(USE_X509)
gnutls_certificate_credentials_t serverx509cred;
#endif

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


static void global_stop(void)
{
	if (debug)
		success("global stop\n");

#ifdef USE_PSK
	gnutls_psk_free_server_credentials(pskcred);
#elif defined(USE_ANON)
	gnutls_anon_free_server_credentials(anoncred);
#elif defined(USE_X509)
	gnutls_certificate_free_credentials(serverx509cred);
#endif
	gnutls_dh_params_deinit(dh_params);
}

#ifdef USE_PSK
static int
pskfunc(gnutls_session_t session, const char *username,
	gnutls_datum_t * key)
{
	if (debug)
		printf("psk: username %s\n", username);
	key->data = gnutls_malloc(4);
	key->data[0] = 0xDE;
	key->data[1] = 0xAD;
	key->data[2] = 0xBE;
	key->data[3] = 0xEF;
	key->size = 4;
	return 0;
}
#endif

static void server(int sds[], struct params_res *params)
{
	int t;
	int ret;
	gnutls_session_t session;
	char buffer[MAX_BUF + 1];
	gnutls_group_t pgroup;
	unsigned iflags = GNUTLS_SERVER;

	virt_time_init();

	if (params->early_start || params->no_early_start)
		iflags |= GNUTLS_ENABLE_EARLY_START;

	/* this must be called once in the program, it is mostly for the server.
	 */
	if (debug) {
		gnutls_global_set_log_function(tls_log_func);
		gnutls_global_set_log_level(4);
	}

#ifdef USE_PSK
	gnutls_psk_allocate_server_credentials(&pskcred);
	gnutls_psk_set_server_credentials_function(pskcred, pskfunc);
#elif defined(USE_ANON)
	gnutls_anon_allocate_server_credentials(&anoncred);
#elif defined(USE_X509)
	gnutls_certificate_allocate_credentials(&serverx509cred);
	assert(gnutls_certificate_set_x509_key_mem(serverx509cred,
		&server_cert, &server_key, GNUTLS_X509_FMT_PEM) >= 0);
#endif

	if (debug)
		success("Launched, generating DH parameters...\n");

	generate_dh_params();

#if USE_ANON
	gnutls_anon_set_server_dh_params(anoncred, dh_params);
#endif

	if (params->enable_db) {
		wrap_db_init();
	}

	if (params->enable_session_ticket_server)
		gnutls_session_ticket_key_generate(&session_ticket_key);

	for (t = 0; t < SESSIONS; t++) {
		int sd = sds[t];

		assert(gnutls_init(&session, iflags) >= 0);

		/* avoid calling all the priority functions, since the defaults
		 * are adequate.
		 */
		assert(gnutls_priority_set_direct(session,
						  PRIO_STR,
						  NULL) >= 0);


#if defined(USE_X509)
		if (params->client_cert) {
			gnutls_certificate_server_set_request(session,
							      GNUTLS_CERT_REQUIRE);
		}
#endif

		gnutls_dh_set_prime_bits(session, DH_BITS);

		if (params->enable_db) {
			gnutls_db_set_retrieve_function(session, wrap_db_fetch);
			gnutls_db_set_remove_function(session, wrap_db_delete);
			gnutls_db_set_store_function(session, wrap_db_store);
			gnutls_db_set_ptr(session, NULL);
		}

		if (params->enable_session_ticket_server)
			gnutls_session_ticket_enable_server(session,
							    &session_ticket_key);

		append_alpn(session, params, t);

		if (params->expire_ticket) {
			gnutls_db_set_cache_expiration(session, 45);
			virt_sec_sleep(60);
		}
#ifdef USE_PSK
		gnutls_credentials_set(session, GNUTLS_CRD_PSK, pskcred);
#elif defined(USE_ANON)
		gnutls_credentials_set(session, GNUTLS_CRD_ANON, anoncred);
#elif defined(USE_X509)
		gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, serverx509cred);
#endif
		gnutls_transport_set_int(session, sd);
		gnutls_handshake_set_timeout(session, 20 * 1000);

		if (params->call_post_client_hello) {
			gnutls_session_set_ptr(session, PRIO_STR);
			gnutls_handshake_set_post_client_hello_function(session,
									post_client_hello_callback);
		}


		do {
			ret = gnutls_handshake(session);
		} while (ret < 0 && gnutls_error_is_fatal(ret) == 0);
		if (ret < 0) {
			close(sd);
			gnutls_deinit(session);
			kill(child, SIGTERM);
			fail("server: Handshake has failed (%s)\n\n",
			     gnutls_strerror(ret));
			return;
		}
		if (debug)
			success("server: Handshake was completed\n");

		if (t > 0 && params->expect_resume) {
			ret = gnutls_session_is_resumed(session);
			if (ret == 0) {
				fail("server: session_is_resumed error (%d)\n", t);
			}
		}

		verify_alpn(session, params, t);
		verify_group(session, &pgroup, t);

		if (params->expect_resume)
			verify_server_params(session, t, params);

		/* see the Getting peer's information example */
		/* print_info(session); */

		for (;;) {
			memset(buffer, 0, MAX_BUF + 1);
			ret = gnutls_record_recv(session, buffer, MAX_BUF);

			if (ret == 0) {
				if (debug)
					success
					    ("server: Peer has closed the GnuTLS connection\n");
				break;
			} else if (ret < 0) {
				kill(child, SIGTERM);
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
	}

	if (params->enable_db) {
		wrap_db_deinit();
	}

	gnutls_free(session_ticket_key.data);

	if (debug)
		success("server: finished\n");
}

void doit(void)
{
	int i, err;

	signal(SIGCHLD, SIG_IGN);
	signal(SIGPIPE, SIG_IGN);

	for (i = 0; resume_tests[i].desc; i++) {
		int client_sds[SESSIONS], server_sds[SESSIONS];
		int j;

		printf("%s\n", resume_tests[i].desc);

		for (j = 0; j < SESSIONS; j++) {
			int sockets[2];

			err = socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
			if (err == -1) {
				perror("socketpair");
				fail("socketpair failed\n");
				return;
			}

			server_sds[j] = sockets[0];
			client_sds[j] = sockets[1];
		}

		child = fork();
		if (child < 0) {
			perror("fork");
			fail("fork");
			return;
		}

		if (child) {
			int status = 0;
			/* parent */
			for (j = 0; j < SESSIONS; j++)
				close(client_sds[j]);
			server(server_sds, &resume_tests[i]);

			waitpid(child, &status, 0);
			check_wait_status(status);
			global_stop();
		} else {
			for (j = 0; j < SESSIONS; j++)
				close(server_sds[j]);
			client(client_sds, &resume_tests[i]);
			exit(0);
		}
	}
}

/* Functions and other stuff needed for session resuming.
 * This is done using a very simple list which holds session ids
 * and session data.
 */

#define MAX_SESSION_ID_SIZE 32
#define MAX_SESSION_DATA_SIZE 1024

typedef struct {
	unsigned char session_id[MAX_SESSION_ID_SIZE];
	unsigned int session_id_size;

	char session_data[MAX_SESSION_DATA_SIZE];
	int session_data_size;
} CACHE;

static CACHE *cache_db;
static int cache_db_ptr = 0;

static void wrap_db_init(void)
{

	/* allocate cache_db */
	cache_db = calloc(1, TLS_SESSION_CACHE * sizeof(CACHE));
}

static void wrap_db_deinit(void)
{
	free(cache_db);
	cache_db = NULL;
	return;
}

static int
wrap_db_store(void *dbf, gnutls_datum_t key, gnutls_datum_t data)
{
	time_t t, e, now = time(0);

#ifdef DEBUG_CACHE
	if (debug) {
		unsigned int i;
		fprintf(stderr, "resume db storing (%d-%d): ", key.size,
			data.size);
		for (i = 0; i < key.size; i++) {
			fprintf(stderr, "%02x", key.data[i] & 0xFF);
		}
		fprintf(stderr, "\n");
		fprintf(stderr, "data: ");
		for (i = 0; i < data.size; i++) {
			fprintf(stderr, "%02x", data.data[i] & 0xFF);
		}
		fprintf(stderr, "\n");
	}
#endif

	/* check the correctness of gnutls_db_check_entry_time() */
	t = gnutls_db_check_entry_time(&data);
	if (t < now - 10 || t > now + 10) {
		fail("Time returned by gnutls_db_check_entry_time is bogus\n");
		exit(1);
	}

	/* check the correctness of gnutls_db_check_entry_expire_time() */
	e = gnutls_db_check_entry_expire_time(&data);
	if (e < t) {
		fail("Time returned by gnutls_db_check_entry_expire_time is bogus\n");
		exit(1);
	}

	if (cache_db == NULL)
		return -1;

	if (key.size > MAX_SESSION_ID_SIZE) {
		fail("Key size is too large\n");
		return -1;
	}

	if (data.size > MAX_SESSION_DATA_SIZE) {
		fail("Data size is too large\n");
		return -1;
	}

	memcpy(cache_db[cache_db_ptr].session_id, key.data, key.size);
	cache_db[cache_db_ptr].session_id_size = key.size;

	memcpy(cache_db[cache_db_ptr].session_data, data.data, data.size);
	cache_db[cache_db_ptr].session_data_size = data.size;

	cache_db_ptr++;
	cache_db_ptr %= TLS_SESSION_CACHE;

	return 0;
}

static gnutls_datum_t wrap_db_fetch(void *dbf, gnutls_datum_t key)
{
	gnutls_datum_t res = { NULL, 0 };
	unsigned i;

	if (debug) {
		fprintf(stderr, "resume db looking for (%d): ", key.size);
		for (i = 0; i < key.size; i++) {
			fprintf(stderr, "%02x", key.data[i] & 0xFF);
		}
		fprintf(stderr, "\n");
	}

	if (cache_db == NULL)
		return res;

	for (i = 0; i < TLS_SESSION_CACHE; i++) {
		if (key.size == cache_db[i].session_id_size &&
		    memcmp(key.data, cache_db[i].session_id,
			   key.size) == 0) {
			if (debug)
				success
				    ("resume db fetch... return info\n");

			res.size = cache_db[i].session_data_size;

			res.data = gnutls_malloc(res.size);
			if (res.data == NULL)
				return res;

			memcpy(res.data, cache_db[i].session_data,
				res.size);

#ifdef DEBUG_CACHE
			if (debug) {
				unsigned int j;
				printf("data:\n");
				for (j = 0; j < res.size; j++) {
					printf("%02x ",
						res.data[j] & 0xFF);
					if ((j + 1) % 16 == 0)
						printf("\n");
				}
				printf("\n");
			}
#endif
			return res;
		}
	}

	if (debug)
		success("resume db fetch... NOT FOUND\n");
	return res;
}

static int wrap_db_delete(void *dbf, gnutls_datum_t key)
{
	int i;

	if (cache_db == NULL)
		return -1;

	for (i = 0; i < TLS_SESSION_CACHE; i++) {
		if (key.size == cache_db[i].session_id_size &&
		    memcmp(key.data, cache_db[i].session_id,
			   key.size) == 0) {

			cache_db[i].session_id_size = 0;
			cache_db[i].session_data_size = 0;

			return 0;
		}
	}

	return -1;

}

#endif				/* _WIN32 */
