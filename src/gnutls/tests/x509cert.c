/*
 * Copyright (C) 2011-2012 Free Software Foundation, Inc.
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

/* Parts copied from GnuTLS example programs. */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#if !defined(_WIN32)
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#endif
#include <unistd.h>
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>

#include "utils.h"
#include "cert-common.h"

/* Test for gnutls_certificate_get_issuer() and implicitly for
 * gnutls_trust_list_get_issuer().
 */

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "<%d>| %s", level, str);
}


#define LIST_SIZE 3
void doit(void)
{
	gnutls_certificate_credentials_t x509_cred;
	int ret;
	unsigned int i;
	gnutls_x509_crt_t issuer;
	gnutls_x509_crt_t list[LIST_SIZE];
	char dn[128];
	size_t dn_size;
	unsigned int list_size;

	gnutls_x509_privkey_t get_key;
	gnutls_x509_crt_t *get_crts;
	unsigned n_get_crts;
	gnutls_datum_t get_datum, chain_datum[2] = {server_ca3_cert, subca3_cert};
	gnutls_x509_trust_list_t trust_list;
	gnutls_x509_trust_list_iter_t trust_iter;
	gnutls_x509_crt_t get_ca_crt;
	unsigned n_get_ca_crts;

	/* this must be called once in the program
	 */
	global_init();

	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(6);

	gnutls_certificate_allocate_credentials(&x509_cred);
	gnutls_certificate_set_x509_trust_mem(x509_cred, &ca3_cert,
					      GNUTLS_X509_FMT_PEM);

	gnutls_certificate_set_x509_key_mem(x509_cred, &server_ca3_cert_chain,
					    &server_ca3_key,
					    GNUTLS_X509_FMT_PEM);

	/* test for gnutls_certificate_get_issuer() */

	/* check whether gnutls_x509_crt_list_import will fail if given a single
	 * certificate */
	list_size = LIST_SIZE;
	ret =
	    gnutls_x509_crt_list_import(list, &list_size, &ca3_cert,
					GNUTLS_X509_FMT_PEM,
					GNUTLS_X509_CRT_LIST_FAIL_IF_UNSORTED);
	if (ret < 0)
		fail("gnutls_x509_crt_list_import (failed with a single cert)");
	gnutls_x509_crt_deinit(list[0]);

	list_size = LIST_SIZE;
	ret =
	    gnutls_x509_crt_list_import(list, &list_size, &cli_ca3_cert_chain,
					GNUTLS_X509_FMT_PEM,
					GNUTLS_X509_CRT_LIST_FAIL_IF_UNSORTED);
	if (ret < 0)
		fail("gnutls_x509_crt_list_import");

	ret =
	    gnutls_certificate_get_issuer(x509_cred, list[list_size-1], &issuer, 0);
	if (ret < 0)
		fail("gnutls_certificate_get_isser");

	ret =
	    gnutls_certificate_get_issuer(x509_cred, list[list_size-1], &issuer, GNUTLS_TL_GET_COPY);
	if (ret < 0)
		fail("gnutls_certificate_get_isser");

	dn_size = sizeof(dn);
	ret = gnutls_x509_crt_get_dn(issuer, dn, &dn_size);
	if (ret < 0)
		fail("gnutls_certificate_get_dn");
	gnutls_x509_crt_deinit(issuer);

	if (dn_size != strlen(dn)) {
		fail("gnutls_x509_crt_get_dn: lengths don't match\n");
		exit(1);
	}

	if (debug)
		fprintf(stderr, "Issuer's DN: %s\n", dn);

	/* test the getter functions of gnutls_certificate_credentials_t */

	ret =
	    gnutls_certificate_get_x509_key(x509_cred, 0, &get_key);
	if (ret < 0)
		fail("gnutls_certificate_get_x509_key");

	ret =
	    gnutls_x509_privkey_export2(get_key,
					GNUTLS_X509_FMT_PEM,
					&get_datum);
	if (ret < 0)
		fail("gnutls_x509_privkey_export2");

	if (get_datum.size != server_ca3_key.size ||
	    memcmp(get_datum.data, server_ca3_key.data, get_datum.size) != 0) {
		fail(
		    "exported key %u vs. %u\n\n%s\n\nvs.\n\n%s",
		    get_datum.size, server_ca3_key.size,
		    get_datum.data, server_ca3_key.data);
	}

	gnutls_free(get_datum.data);

	ret =
	    gnutls_certificate_get_x509_crt(x509_cred, 0, &get_crts, &n_get_crts);
	if (ret < 0)
		fail("gnutls_certificate_get_x509_crt");
	if (n_get_crts != 2)
		fail("gnutls_certificate_get_x509_crt: n_crts != 2");

	for (i = 0; i < n_get_crts; i++) {
		ret =
			gnutls_x509_crt_export2(get_crts[i],
						GNUTLS_X509_FMT_PEM,
						&get_datum);
		if (ret < 0)
			fail("gnutls_x509_crt_export2");

		if (get_datum.size != chain_datum[i].size ||
		    memcmp(get_datum.data, chain_datum[i].data, get_datum.size) != 0) {
			fail(
				"exported certificate %u vs. %u\n\n%s\n\nvs.\n\n%s",
				get_datum.size, chain_datum[i].size,
				get_datum.data, chain_datum[i].data);
		}

		gnutls_free(get_datum.data);
	}

	gnutls_certificate_get_trust_list(x509_cred, &trust_list);

	n_get_ca_crts = 0;
	trust_iter = NULL;
	while (gnutls_x509_trust_list_iter_get_ca(trust_list,
						  &trust_iter,
						  &get_ca_crt) !=
		GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE) {
		ret =
		    gnutls_x509_crt_export2(get_ca_crt,
					    GNUTLS_X509_FMT_PEM,
					    &get_datum);
		if (ret < 0)
			fail("gnutls_x509_crt_export2");

		if (get_datum.size != ca3_cert.size ||
		    memcmp(get_datum.data, ca3_cert.data, get_datum.size) != 0) {
			fail(
			    "exported CA certificate %u vs. %u\n\n%s\n\nvs.\n\n%s",
			    get_datum.size, ca3_cert.size,
			    get_datum.data, ca3_cert.data);
		}

		gnutls_x509_crt_deinit(get_ca_crt);
		gnutls_free(get_datum.data);

		++n_get_ca_crts;
	}

	if (n_get_ca_crts != 1)
		fail("gnutls_x509_trust_list_iter_get_ca: n_cas != 1");
	if (trust_iter != NULL)
		fail("gnutls_x509_trust_list_iter_get_ca: iterator not NULL after iteration");

	gnutls_x509_privkey_deinit(get_key);
	for (i = 0; i < n_get_crts; i++)
		gnutls_x509_crt_deinit(get_crts[i]);
	gnutls_free(get_crts);

	for (i = 0; i < list_size; i++)
		gnutls_x509_crt_deinit(list[i]);
	gnutls_certificate_free_credentials(x509_cred);

	gnutls_global_deinit();

	if (debug)
		success("success");
}
