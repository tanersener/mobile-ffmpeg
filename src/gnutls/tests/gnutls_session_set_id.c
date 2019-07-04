/*
 * Copyright (C) 2018 Nikos Mavrogiannopoulos
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
#include <assert.h>
#include <gnutls/gnutls.h>
#include "utils.h"
#include "eagain-common.h"
#include "cert-common.h"

const char *side;

static gnutls_datum_t test_id = { (void*)"\xff\xff\xff\xff\xff\xff", 6 };

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "%s|<%d>| %s", side, level, str);
}

static gnutls_datum_t dbdata = {NULL, 0};

static int
wrap_db_store(void *dbf, gnutls_datum_t key, gnutls_datum_t data)
{
	unsigned *try_resume = dbf;

	assert(dbdata.data == NULL);

	if (!(*try_resume))
		return 0;

	dbdata.data = gnutls_malloc(data.size);
	assert(dbdata.data != NULL);

	memcpy(dbdata.data, data.data, data.size);
	dbdata.size = data.size;
	return 0;
}

static gnutls_datum_t wrap_db_fetch(void *dbf, gnutls_datum_t key)
{
	unsigned *try_resume = dbf;
	gnutls_datum_t r = {NULL, 0};

	if (key.size != test_id.size || memcmp(test_id.data, key.data, test_id.size) != 0)
		fail("received ID does not match the expected\n");

	if (!(*try_resume))
		return r;

	r.data = gnutls_malloc(dbdata.size);
	assert(r.data != NULL);

	memcpy(r.data, dbdata.data, dbdata.size);
	r.size = dbdata.size;

	return r;
}

static int wrap_db_delete(void *dbf, gnutls_datum_t key)
{
	return 0;
}

static void start(const char *test, unsigned try_resume)
{
	int ret;
	gnutls_certificate_credentials_t serverx509cred;
	gnutls_session_t server;
	int sret = GNUTLS_E_AGAIN;
	gnutls_certificate_credentials_t clientx509cred;
	gnutls_session_t client;
	int cret = GNUTLS_E_AGAIN;
	gnutls_datum_t data;
	char buf[128];

	success("%s\n", test);
	reset_buffers();

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
	gnutls_set_default_priority(server);
	gnutls_transport_set_push_function(server, server_push);
	gnutls_transport_set_pull_function(server, server_pull);
	gnutls_transport_set_ptr(server, server);

	gnutls_db_set_retrieve_function(server, wrap_db_fetch);
	gnutls_db_set_remove_function(server, wrap_db_delete);
	gnutls_db_set_store_function(server, wrap_db_store);
	gnutls_db_set_ptr(server, &try_resume);

	assert(gnutls_certificate_allocate_credentials(&clientx509cred)>=0);
	assert(gnutls_certificate_set_x509_trust_mem(clientx509cred, &ca_cert, GNUTLS_X509_FMT_PEM)>=0);

	assert(gnutls_init(&client, GNUTLS_CLIENT)>=0);
	assert(gnutls_credentials_set(client, GNUTLS_CRD_CERTIFICATE,
				      clientx509cred)>=0);

	assert(gnutls_priority_set_direct(client, "NORMAL:-VERS-ALL:+VERS-TLS1.2", NULL)>=0);
	gnutls_transport_set_push_function(client, client_push);
	gnutls_transport_set_pull_function(client, client_pull);
	gnutls_transport_set_ptr(client, client);

	memset(buf, 0, sizeof(buf));
	ret = gnutls_session_set_data(client, buf, sizeof(buf));
	if (ret != GNUTLS_E_DB_ERROR) {
		fail("unexpected error: %s\n", gnutls_strerror(ret));
	}

	HANDSHAKE(client, server);

	ret = gnutls_session_get_data2(client, &data);
	if (ret != 0) {
		fail("unexpected error: %s\n", gnutls_strerror(ret));
	}

	gnutls_deinit(client);
	gnutls_deinit(server);

	assert(gnutls_init(&server, GNUTLS_SERVER)>=0);
	gnutls_credentials_set(server, GNUTLS_CRD_CERTIFICATE,
				serverx509cred);
	gnutls_set_default_priority(server);
	gnutls_transport_set_push_function(server, server_push);
	gnutls_transport_set_pull_function(server, server_pull);
	gnutls_transport_set_ptr(server, server);

	gnutls_db_set_retrieve_function(server, wrap_db_fetch);
	gnutls_db_set_remove_function(server, wrap_db_delete);
	gnutls_db_set_store_function(server, wrap_db_store);
	gnutls_db_set_ptr(server, &try_resume);

	assert(gnutls_init(&client, GNUTLS_CLIENT)>=0);

	assert(gnutls_credentials_set(client, GNUTLS_CRD_CERTIFICATE,
				      clientx509cred)>=0);

	assert(gnutls_priority_set_direct(client, "NORMAL:-VERS-ALL:+VERS-TLS1.2", NULL)>=0);
	gnutls_transport_set_push_function(client, client_push);
	gnutls_transport_set_pull_function(client, client_pull);
	gnutls_transport_set_ptr(client, client);

	memset(buf, 0, sizeof(buf));
	ret = gnutls_session_set_id(client, &test_id);
	if (ret != 0) {
		fail("unexpected error: %s\n", gnutls_strerror(ret));
	}
	gnutls_free(data.data);

	if (try_resume) {
		HANDSHAKE_EXPECT(client, server, GNUTLS_E_UNEXPECTED_PACKET, GNUTLS_E_AGAIN);
	} else {
		HANDSHAKE(client, server);
	}

	assert(gnutls_session_resumption_requested(client) == 0);

	gnutls_bye(client, GNUTLS_SHUT_RDWR);
	gnutls_bye(server, GNUTLS_SHUT_RDWR);

	gnutls_deinit(client);
	gnutls_deinit(server);

	gnutls_certificate_free_credentials(serverx509cred);
	gnutls_certificate_free_credentials(clientx509cred);

	gnutls_free(dbdata.data);
	dbdata.size = 0;
}

void doit(void)
{
	start("functional: see if session ID is sent", 0);
	start("negative: see if the expected error is seen on client side", 1);
}
