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
#include <string.h>
#include <errno.h>
#include <gnutls/gnutls.h>
#include "utils.h"
#include "eagain-common.h"

/* This test checks whether the server switching certificates is detected
 * by the client */

const char *side;

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "%s|<%d>| %s", side, level, str);
}

#include "cert-common.h"

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

static void try(const char *prio, gnutls_kx_algorithm_t kx, unsigned allow_change)
{
	int ret;
	/* Server stuff. */
	gnutls_psk_server_credentials_t serverpskcred;
	gnutls_dh_params_t dh_params;
	const gnutls_datum_t p3 =
	    { (unsigned char *) pkcs3, strlen(pkcs3) };
	gnutls_session_t server;
	int sret = GNUTLS_E_AGAIN;
	/* Client stuff. */
	gnutls_psk_client_credentials_t clientpskcred;
	gnutls_psk_client_credentials_t clientpskcred2;
	gnutls_session_t client;
	const gnutls_datum_t key = { (void *) "DEADBEEF", 8 };
	int cret = GNUTLS_E_AGAIN;

	/* General init. */
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(6);

	/* Init server */
	gnutls_psk_allocate_server_credentials(&serverpskcred);

	gnutls_psk_set_server_credentials_function(serverpskcred,
						   pskfunc);

	gnutls_dh_params_init(&dh_params);
	gnutls_dh_params_import_pkcs3(dh_params, &p3, GNUTLS_X509_FMT_PEM);
	gnutls_psk_set_server_dh_params(serverpskcred, dh_params);

	if (allow_change)
		gnutls_init(&server, GNUTLS_SERVER|GNUTLS_ALLOW_ID_CHANGE);
	else
		gnutls_init(&server, GNUTLS_SERVER);
	gnutls_credentials_set(server, GNUTLS_CRD_PSK,
				serverpskcred);

	gnutls_priority_set_direct(server,
				   prio,
				   NULL);
	gnutls_transport_set_push_function(server, server_push);
	gnutls_transport_set_pull_function(server, server_pull);
	gnutls_transport_set_ptr(server, server);

	/* Init client */

	ret = gnutls_psk_allocate_client_credentials(&clientpskcred);
	if (ret < 0)
		exit(1);

	gnutls_psk_set_client_credentials(clientpskcred, "test1", &key,
					  GNUTLS_PSK_KEY_HEX);

	ret = gnutls_psk_allocate_client_credentials(&clientpskcred2);
	if (ret < 0)
		exit(1);

	gnutls_psk_set_client_credentials(clientpskcred2, "test2", &key,
					  GNUTLS_PSK_KEY_HEX);

	ret = gnutls_init(&client, GNUTLS_CLIENT);
	if (ret < 0)
		exit(1);

	ret = gnutls_credentials_set(client, GNUTLS_CRD_PSK,
				clientpskcred);
	if (ret < 0)
		exit(1);

	ret = gnutls_priority_set_direct(client, prio, NULL);
	if (ret < 0)
		exit(1);

	gnutls_transport_set_push_function(client, client_push);
	gnutls_transport_set_pull_function(client, client_pull);
	gnutls_transport_set_ptr(client, client);

	HANDSHAKE(client, server);

	if (gnutls_kx_get(client) != kx) {
		fail("got unexpected key exchange algorithm: %s (expected %s)\n",
			gnutls_kx_get_name(gnutls_kx_get(client)),
			gnutls_kx_get_name(kx));
		exit(1);
	}

	/* switch client's username and rehandshake */
	ret = gnutls_credentials_set(client, GNUTLS_CRD_PSK,
				clientpskcred2);
	if (ret < 0)
		exit(1);

	if (allow_change) {
		HANDSHAKE(client, server);
	} else {
		HANDSHAKE_EXPECT(client, server, GNUTLS_E_AGAIN, GNUTLS_E_SESSION_USER_ID_CHANGED);
	}

	gnutls_deinit(client);
	gnutls_deinit(server);

	gnutls_psk_free_server_credentials(serverpskcred);
	gnutls_psk_free_client_credentials(clientpskcred);
	gnutls_psk_free_client_credentials(clientpskcred2);
	gnutls_dh_params_deinit(dh_params);
}

void doit(void)
{
	global_init();

	/* Allow change of ID */
	try("NORMAL:-KX-ALL:+PSK", GNUTLS_KX_PSK, 0);
	reset_buffers();
	try("NORMAL:-KX-ALL:+DHE-PSK", GNUTLS_KX_DHE_PSK, 0);
	reset_buffers();
	try("NORMAL:-KX-ALL:+ECDHE-PSK", GNUTLS_KX_ECDHE_PSK, 0);
	reset_buffers();

	/* Prohibit (default) change of ID */
	try("NORMAL:-KX-ALL:+PSK", GNUTLS_KX_PSK, 1);
	reset_buffers();
	try("NORMAL:-KX-ALL:+DHE-PSK", GNUTLS_KX_DHE_PSK, 1);
	reset_buffers();
	try("NORMAL:-KX-ALL:+ECDHE-PSK", GNUTLS_KX_ECDHE_PSK, 1);
	reset_buffers();
	gnutls_global_deinit();
}
