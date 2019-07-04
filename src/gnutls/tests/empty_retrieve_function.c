/*
 * Copyright (C) 2011-2012 Free Software Foundation, Inc.
 * Copyright (C) 2018 Dmitry Eremin-Solenikov
 * Copyright (C) 2018 Red Hat, Inc.
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
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#include <gnutls/abstract.h>

#include "utils.h"
#include "cert-common.h"

/* Test for behavior of the library when certificate callbacks
 * return no certificates.
 */

static int cert_cb1(gnutls_session_t session,
		const gnutls_datum_t * req_ca_rdn,
		int nreqs,
		const gnutls_pk_algorithm_t * pk_algos,
		int pk_algos_length,
		gnutls_retr2_st *retr)
{
	memset(retr, 0, sizeof(*retr));
	return 0;
}

static int cert_cb2(gnutls_session_t session,
		const gnutls_datum_t *req_ca_rdn,
		int nreqs,
		const gnutls_pk_algorithm_t *pk_algos,
		int pk_algos_length,
		gnutls_pcert_st** pcert,
		unsigned int *pcert_length,
		gnutls_privkey_t *privkey)
{
	*pcert_length = 0;
	*privkey = NULL;
	*pcert = NULL;

	return 0;
}

static int cert_cb3(gnutls_session_t session,
		const struct gnutls_cert_retr_st *info,
		gnutls_pcert_st **certs,
		unsigned int *pcert_length,
		gnutls_ocsp_data_st **ocsp,
		unsigned int *ocsp_length,
		gnutls_privkey_t *privkey,
		unsigned int *flags)
{
	*privkey = NULL;
	*ocsp_length = 0;
	*pcert_length = 0;
	return 0;
}


static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "<%d>| %s", level, str);
}

void doit(void)
{
	gnutls_certificate_credentials_t x509_cred;
	gnutls_certificate_credentials_t clicred;
	int ret;

	/* this must be called once in the program
	 */
	global_init();

	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(6);

	gnutls_certificate_allocate_credentials(&x509_cred);

	ret = gnutls_certificate_set_x509_key_mem(x509_cred, &server_ca3_localhost_cert_chain,
					    &server_ca3_key,
					    GNUTLS_X509_FMT_PEM);
	if (ret < 0) {
		fail("error in error code\n");
		exit(1);
	}

	gnutls_certificate_allocate_credentials(&clicred);
	gnutls_certificate_set_retrieve_function(clicred, cert_cb1);
	_test_cli_serv(x509_cred, clicred, "NORMAL", "NORMAL", "localhost", NULL, NULL, NULL, 0, 1, GNUTLS_E_CERTIFICATE_REQUIRED, -1);
	gnutls_certificate_free_credentials(clicred);

	gnutls_certificate_allocate_credentials(&clicred);
	gnutls_certificate_set_retrieve_function2(clicred, cert_cb2);
	_test_cli_serv(x509_cred, clicred, "NORMAL", "NORMAL", "localhost", NULL, NULL, NULL, 0, 1, GNUTLS_E_CERTIFICATE_REQUIRED, -1);
	gnutls_certificate_free_credentials(clicred);

	gnutls_certificate_allocate_credentials(&clicred);
	gnutls_certificate_set_retrieve_function3(clicred, cert_cb3);
	_test_cli_serv(x509_cred, clicred, "NORMAL", "NORMAL", "localhost", NULL, NULL, NULL, 0, 1, GNUTLS_E_CERTIFICATE_REQUIRED, -1);
	gnutls_certificate_free_credentials(clicred);

	gnutls_certificate_free_credentials(x509_cred);

	gnutls_global_deinit();

	if (debug)
		success("success");
}
