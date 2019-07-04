/*
 * Copyright (C) 2016-2018 Red Hat, Inc.
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
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <gnutls/gnutls.h>
#include <assert.h>
#include "utils.h"
#include "virt-time.h"
#include "eagain-common.h"
#include "cert-common.h"

/* This test checks whether the lifetime of a resumed session can
 * be extended past the designated one */

const char *side;

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "%s|<%d>| %s", side, level, str);
}

struct hsk_st {
	unsigned sent_nst; /* whether the new session ticket was sent */
	unsigned sent_psk; /* whether the PSK extension was sent */
	unsigned sleep_at_finished; /* how long to wait at finished message reception */

};

static int ext_hook_func(void *ctx, unsigned tls_id,
			 const unsigned char *data, unsigned size)
{
	if (tls_id == 41) {
		struct hsk_st *h = ctx;
		h->sent_psk = 1;
	}
	return 0;
}

static int handshake_callback(gnutls_session_t session, unsigned int htype,
	unsigned post, unsigned int incoming, const gnutls_datum_t *msg)
{
	struct hsk_st *h = gnutls_session_get_ptr(session);

	if (htype == GNUTLS_HANDSHAKE_FINISHED && !incoming) {
		if (h->sleep_at_finished)
			virt_sec_sleep(h->sleep_at_finished);
		return 0;
	} else if (htype == GNUTLS_HANDSHAKE_CLIENT_HELLO) {
		gnutls_ext_raw_parse(h, ext_hook_func, msg,
				     GNUTLS_EXT_RAW_FLAG_TLS_CLIENT_HELLO);
	}

	if (htype != GNUTLS_HANDSHAKE_NEW_SESSION_TICKET)
		return 0;

	if (h)
		h->sent_nst = 1;
	return 0;
}

/* Returns true if resumed */
static unsigned handshake(const char *prio, unsigned t, const gnutls_datum_t *sdata,
			  gnutls_datum_t *ndata,
			  gnutls_datum_t *skey,
			  struct hsk_st *h)
{
	int ret;
	/* Server stuff. */
	gnutls_certificate_credentials_t serverx509cred;
	gnutls_session_t server;
	int sret = GNUTLS_E_AGAIN;
	/* Client stuff. */
	gnutls_certificate_credentials_t clientx509cred;
	gnutls_session_t client;
	int cret = GNUTLS_E_AGAIN;
	char buf[128];

	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(6);

	assert(gnutls_certificate_allocate_credentials(&serverx509cred)>=0);
	assert(gnutls_certificate_set_x509_key_mem(serverx509cred,
					    &server_cert, &server_key,
					    GNUTLS_X509_FMT_PEM)>=0);

	assert(gnutls_init(&server, GNUTLS_SERVER)>=0);
	gnutls_credentials_set(server, GNUTLS_CRD_CERTIFICATE,
				serverx509cred);
	assert(gnutls_priority_set_direct(server, prio, NULL)>=0);
	gnutls_transport_set_push_function(server, server_push);
	gnutls_transport_set_pull_function(server, server_pull);
	gnutls_transport_set_ptr(server, server);
	gnutls_session_set_ptr(server, h);

	gnutls_db_set_cache_expiration(server, t);
	assert(gnutls_session_ticket_enable_server(server, skey) >= 0);

	gnutls_handshake_set_hook_function(server, -1,
					   GNUTLS_HOOK_POST,
					   handshake_callback);

	assert(gnutls_certificate_allocate_credentials(&clientx509cred)>=0);
	assert(gnutls_certificate_set_x509_trust_mem(clientx509cred, &ca_cert, GNUTLS_X509_FMT_PEM)>=0);
	assert(gnutls_init(&client, GNUTLS_CLIENT)>=0);

	assert(gnutls_credentials_set(client, GNUTLS_CRD_CERTIFICATE,
				      clientx509cred)>=0);

	assert(gnutls_priority_set_direct(client, prio, NULL)>=0);
	gnutls_transport_set_push_function(client, client_push);
	gnutls_transport_set_pull_function(client, client_pull);
	gnutls_transport_set_ptr(client, client);

	if (sdata) {
		assert(gnutls_session_set_data(client, sdata->data, sdata->size)>=0);
	}

	memset(buf, 0, sizeof(buf));
	ret = gnutls_session_set_data(client, buf, sizeof(buf));
	if (ret != GNUTLS_E_DB_ERROR) {
		fail("unexpected error: %s\n", gnutls_strerror(ret));
	}

	HANDSHAKE(client, server);

	gnutls_record_recv(client, buf, sizeof(buf));

	if (ndata) {
		ret = gnutls_session_get_data2(client, ndata);
		if (ret < 0) {
			fail("unexpected error: %s\n", gnutls_strerror(ret));
		}
	}

	ret = gnutls_session_is_resumed(client);

	gnutls_deinit(server);
	gnutls_deinit(client);

	gnutls_certificate_free_credentials(serverx509cred);
	gnutls_certificate_free_credentials(clientx509cred);

	reset_buffers();
	return ret;
}

/* @t is the lifetime of the first ticket, @s is the
 * time to wait before asking for a ticket the last try */
static void start(const char *name, const char *prio, unsigned t, unsigned s)
{
	gnutls_datum_t sdata, ndata, skey;
	unsigned ret;
	struct hsk_st h;
	memset(&h, 0, sizeof(h));

	success("trying %s\n", name);

	assert(gnutls_session_ticket_key_generate(&skey)>=0);

	/* step1: get a fresh ticket */
	ret = handshake(prio, t, NULL, &sdata, &skey, &h);
	assert(ret == 0);
	assert(h.sent_nst != 0);
	memset(&h, 0, sizeof(h));

	if (debug)
		success("completed first handshake\n");

	if (s)
		virt_sec_sleep(s);

	/* step2: get a ticket from the resumed session of the first */
	ret = handshake(prio, t, &sdata, &ndata, &skey, &h);
	assert(ret != 0);
	assert(h.sent_nst != 0);
	memset(&h, 0, sizeof(h));

	/* wait until the ticket we got in step1 is invalid, although
	 * the ticket we got in step2 is valid */

	if (debug)
		success("completed second handshake\n");

	if (s)
		virt_sec_sleep(s);

	ret = handshake(prio, t, &ndata, NULL, &skey, &h);

	if (s) {
		if (ret != 0)
			fail("server resumed session even if ticket expired!\n");

		/* we shouldn't have sent the PSK extension as the ticket was expired */
		assert(h.sent_psk == 0);
	}

	gnutls_free(ndata.data);
	gnutls_free(sdata.data);
	gnutls_free(skey.data);
}

/* @t is the lifetime of the first ticket, @s is the
 * time to wait before asking for a ticket the last try
 *
 * This makes the ticket to expire during handshake (after resumtion),
 * but before the client receives the new session ticket. In that
 * case the server shouldn't send a session ticket.
 */
static void start2(const char *name, const char *prio, unsigned t, unsigned s)
{
	gnutls_datum_t sdata, ndata, skey;
	unsigned ret;
	struct hsk_st h;
	memset(&h, 0, sizeof(h));

	success("trying %s\n", name);

	assert(gnutls_session_ticket_key_generate(&skey)>=0);

	/* step1: get a fresh ticket */
	ret = handshake(prio, t, NULL, &sdata, &skey, &h);
	assert(ret == 0);
	assert(h.sent_nst != 0);
	memset(&h, 0, sizeof(h));

	/* step2: get a ticket from the resumed session of the first */
	ret = handshake(prio, t, &sdata, &ndata, &skey, &h);
	assert(ret != 0);
	assert(h.sent_nst != 0);
	memset(&h, 0, sizeof(h));

	/* wait until the ticket we got in step1 is invalid, although
	 * the ticket we got in step2 is valid */

	if (s)
		h.sleep_at_finished = s;

	ret = handshake(prio, t, &ndata, NULL, &skey, &h);

	assert(ret != 0);
	if (h.sent_nst != 0)
		fail("server sent session ticket even if ticket expired!\n");

	gnutls_free(ndata.data);
	gnutls_free(sdata.data);
	gnutls_free(skey.data);
}

void doit(void)
{
	virt_time_init();

	start("TLS1.3 sanity", "NORMAL:-VERS-ALL:+VERS-TLS1.3", 64, 0);
	start("TLS1.3 ticket extension", "NORMAL:-VERS-ALL:+VERS-TLS1.3", 5, 3);
	start2("TLS1.3 ticket extension - expires at handshake", "NORMAL:-VERS-ALL:+VERS-TLS1.3", 2, 3);
}
