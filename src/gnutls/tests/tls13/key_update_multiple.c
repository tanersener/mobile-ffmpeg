/*
 * Copyright (C) 2017-2019 Red Hat, Inc.
 *
 * Author: Daiki Ueno
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <gnutls/gnutls.h>
#include <gnutls/crypto.h>
#include <assert.h>
#include "cert-common.h"

#include "utils.h"
#include "virt-time.h"
#define RANDOMIZE
#include "eagain-common.h"

const char *side = "";

/* This program tests whether multiple key update messages are handled
 * properly with rate-limit. */

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "%s|<%d>| %s", side, level, str);
}

#define MAX_BUF 1024
#define MSG "Hello TLS, and hi and how are you and more data here... and more... and even more and even more more data..."

/* These must match the definitions in lib/tls13/key_update.c. */
#define KEY_UPDATES_WINDOW 1000
#define KEY_UPDATES_PER_WINDOW 8

static unsigned key_update_msg_inc = 0;
static unsigned key_update_msg_out = 0;

static int hsk_callback(gnutls_session_t session, unsigned int htype,
			unsigned post, unsigned int incoming, const gnutls_datum_t *msg)
{
	assert(post == GNUTLS_HOOK_PRE);

	assert(msg->size == 1);

	if (htype == GNUTLS_HANDSHAKE_KEY_UPDATE) {
		if (incoming)
			key_update_msg_inc++;
		else
			key_update_msg_out++;
	}

	return 0;
}

static void run(const char *name, bool exceed_limit)
{
	/* Server stuff. */
	gnutls_certificate_credentials_t ccred;
	gnutls_certificate_credentials_t scred;
	gnutls_session_t server;
	int sret, cret;
	/* Client stuff. */
	gnutls_session_t client;
	/* Need to enable anonymous KX specifically. */
	char buffer[MAX_BUF + 1];
	int ret, transferred = 0;
	size_t i;

	success("%s\n", name);

	/* General init. */
	global_init();
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(9);

	/* Init server */
	assert(gnutls_certificate_allocate_credentials(&scred) >= 0);
	assert(gnutls_certificate_set_x509_key_mem(scred,
						   &server_ca3_localhost_cert,
						   &server_ca3_key,
						   GNUTLS_X509_FMT_PEM) >= 0);

	assert(gnutls_init(&server, GNUTLS_SERVER) >= 0);
	ret =
	    gnutls_priority_set_direct(server,
				       "NORMAL:-VERS-TLS-ALL:+VERS-TLS1.3",
				       NULL);
	if (ret < 0)
		exit(1);

	gnutls_credentials_set(server, GNUTLS_CRD_CERTIFICATE, scred);
	gnutls_transport_set_push_function(server, server_push);
	gnutls_transport_set_pull_function(server, server_pull);
	gnutls_transport_set_ptr(server, server);

	/* Init client */
	assert(gnutls_certificate_allocate_credentials(&ccred) >= 0);
	assert(gnutls_certificate_set_x509_trust_mem
	       (ccred, &ca3_cert, GNUTLS_X509_FMT_PEM) >= 0);

	gnutls_init(&client, GNUTLS_CLIENT);
	ret =
	    gnutls_priority_set_direct(client,
				       "NORMAL:-VERS-TLS-ALL:+VERS-TLS1.3",
				       NULL);
	assert(ret >= 0);

	ret = gnutls_credentials_set(client, GNUTLS_CRD_CERTIFICATE, ccred);
	if (ret < 0)
		exit(1);

	gnutls_transport_set_push_function(client, client_push);
	gnutls_transport_set_pull_function(client, client_pull);
	gnutls_transport_set_ptr(client, client);


	HANDSHAKE(client, server);
	if (debug)
		success("Handshake established\n");

	key_update_msg_inc = 0;
	key_update_msg_out = 0;

	gnutls_handshake_set_hook_function(client, -1, GNUTLS_HOOK_PRE, hsk_callback);

	/* schedule multiple key updates */
	for (i = 0; i < KEY_UPDATES_PER_WINDOW; i++) {
		do {
			ret = gnutls_session_key_update(client, 1);
		} while (ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED);
		if (ret < 0)
			fail("error in key update: %s\n", gnutls_strerror(ret));
	}

	/* server receives the client key update and sends data */
	TRANSFER(client, server, MSG, strlen(MSG), buffer, MAX_BUF);
	TRANSFER(server, client, MSG, strlen(MSG), buffer, MAX_BUF);
	EMPTY_BUF(server, client, buffer, MAX_BUF);

	if (key_update_msg_out != KEY_UPDATES_PER_WINDOW)
		fail("unexpected number of key updates are sent: %d\n",
			key_update_msg_out);
	else {
		if (debug)
			success("successfully sent %d key updates\n",
				KEY_UPDATES_PER_WINDOW);
	}
	if (key_update_msg_inc != 1)
		fail("unexpected number of key updates received: %d\n",
			key_update_msg_inc);
	else {
		if (debug)
			success("successfully received 1 key update\n");
	}

	if (exceed_limit) {
		/* excessive key update in the same time window should
		 * be rejected by the peer */
		do {
			ret = gnutls_session_key_update(client, 1);
		} while (ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED);

		/* server receives the client key update and sends data */
		ret = record_send_loop(client, MSG, strlen(MSG), 0);
		assert(ret == strlen(MSG));
		ret = gnutls_record_recv(server, buffer, MAX_BUF);
		if (ret != GNUTLS_E_TOO_MANY_HANDSHAKE_PACKETS)
			fail("server didn't reject excessive number of key updates\n");
		else {
			if (debug)
				success("server rejected excessive number of key updates\n");
		}
	} else {
		virt_sec_sleep(KEY_UPDATES_WINDOW / 1000 + 1);

		/* the time window should be rolled over now */
		do {
			ret = gnutls_session_key_update(client, 1);
		} while (ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED);
		if (ret < 0)
			fail("error in key update: %s\n", gnutls_strerror(ret));

		/* server receives the client key update and sends data */
		TRANSFER(client, server, MSG, strlen(MSG), buffer, MAX_BUF);
		TRANSFER(server, client, MSG, strlen(MSG), buffer, MAX_BUF);
		EMPTY_BUF(server, client, buffer, MAX_BUF);
	}

	gnutls_bye(client, GNUTLS_SHUT_WR);
	gnutls_bye(server, GNUTLS_SHUT_WR);

	gnutls_deinit(client);
	gnutls_deinit(server);

	gnutls_certificate_free_credentials(scred);
	gnutls_certificate_free_credentials(ccred);

	gnutls_global_deinit();
	reset_buffers();
}

void doit(void)
{
	virt_time_init();

	run("not exceeding limit", 0);
	run("exceeding limit", 1);
}
