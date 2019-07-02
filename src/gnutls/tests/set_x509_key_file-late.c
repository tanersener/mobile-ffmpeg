/*
 * Copyright (C) 2014-2018 Nikos Mavrogiannopoulos
 * Copyright (C) 2018 Red Hat, Inc.
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

/* This test checks the behavior of handshake process if credentials
 * are set prior to client hello being received but after gnutls_handshake()
 * is called */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>

#include "eagain-common.h"
#include "cert-common.h"
#include "utils.h"

static unsigned set_cert(gnutls_certificate_credentials_t xcred, const gnutls_datum_t *key, const gnutls_datum_t *cert)
{
	const char *certfile;
	FILE *fp;
	int ret;

	certfile = get_tmpname(NULL);

	fp = fopen(certfile, "w");
	if (fp == NULL)
		fail("error in fopen\n");
	assert(fwrite(cert->data, 1, cert->size, fp)>0);
	assert(fwrite(key->data, 1, key->size, fp)>0);
	fclose(fp);

	ret = gnutls_certificate_set_x509_key_file2(xcred, certfile, certfile,
						    GNUTLS_X509_FMT_PEM, NULL, 0);
	if (ret < 0)
		fail("set_x509_key_file failed: %s\n", gnutls_strerror(ret));

	/* return index */
	return ret;
}

static
int handshake_hook_func(gnutls_session_t session, unsigned int htype,
			unsigned when, unsigned int incoming, const gnutls_datum_t *msg)
{
	gnutls_certificate_credentials_t xcred;
	int idx;

	assert(htype == GNUTLS_HANDSHAKE_CLIENT_HELLO);
	assert(when == GNUTLS_HOOK_PRE);
	assert(gnutls_certificate_allocate_credentials(&xcred) >= 0);

	gnutls_certificate_set_flags(xcred, GNUTLS_CERTIFICATE_API_V2);
	idx = set_cert(xcred, &server_ca3_key, &server_ca3_localhost6_cert_chain);
	assert(idx == 0);

	idx = set_cert(xcred, &server_ca3_key, &server_ca3_localhost_cert);
	assert(idx == 1);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE,
				xcred);

	return 0;
}

static void start(const char *prio)
{
	int ret, sret, cret;
	gnutls_certificate_credentials_t xcred, clicred;
	gnutls_session_t server, client;

	global_init();
	track_temp_files();

	assert(gnutls_certificate_allocate_credentials(&clicred) >= 0);

	ret = gnutls_certificate_set_x509_trust_mem(clicred, &subca3_cert, GNUTLS_X509_FMT_PEM);
	if (ret < 0)
		fail("set_x509_trust_file failed: %s\n", gnutls_strerror(ret));


	success("Testing late set of credentials: %s\n", prio);

	assert(gnutls_init(&server, GNUTLS_SERVER) >= 0);
	gnutls_handshake_set_hook_function(server, GNUTLS_HANDSHAKE_CLIENT_HELLO,
					   GNUTLS_HOOK_PRE, handshake_hook_func);
	assert(gnutls_priority_set_direct(server, prio, NULL) >= 0);
	gnutls_transport_set_push_function(server, server_push);
	gnutls_transport_set_pull_function(server, server_pull);
	gnutls_transport_set_ptr(server, server);

	assert(gnutls_init(&client, GNUTLS_CLIENT)>=0);

	assert(gnutls_credentials_set(client, GNUTLS_CRD_CERTIFICATE,
				      clicred) >= 0);
	if (ret < 0)
		exit(1);

	assert(gnutls_priority_set_direct(client, prio, NULL)>=0);
	gnutls_transport_set_push_function(client, client_push);
	gnutls_transport_set_pull_function(client, client_pull);
	gnutls_transport_set_ptr(client, client);

	HANDSHAKE(client, server);

	assert(gnutls_credentials_get(server, GNUTLS_CRD_CERTIFICATE, (void*)&xcred) >= 0);

	gnutls_deinit(client);
	gnutls_deinit(server);

	gnutls_certificate_free_credentials(xcred);
	gnutls_certificate_free_credentials(clicred);
	gnutls_global_deinit();
	delete_temp_files();
}

void doit(void)
{
	start("NORMAL:-VERS-TLS-ALL:+VERS-TLS1.2");
	start("NORMAL:-VERS-TLS-ALL:+VERS-TLS1.3");
	start("NORMAL");
}
