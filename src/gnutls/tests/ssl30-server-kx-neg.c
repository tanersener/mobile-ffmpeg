/*
 * Copyright (C) 2017 Red Hat, Inc.
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

/* This program tests the ciphersuite negotiation for various key exchange
 * methods and options. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <gnutls/gnutls.h>
#include "utils.h"
#include "cert-common.h"
#include "eagain-common.h"

#include "server-kx-neg-common.c"

test_case_st tests[] = {
	{
		.name = "SSL 3.0 ANON-DH without cred",
		.client_ret = GNUTLS_E_AGAIN,
		.server_ret = GNUTLS_E_NO_CIPHER_SUITES,
		.server_prio = "NORMAL:-KX-ALL:+ANON-DH:-VERS-ALL:+VERS-SSL3.0",
		.client_prio = "NORMAL:-KX-ALL:+ANON-DH:-VERS-ALL:+VERS-SSL3.0"
	},
	{
		.name = "SSL 3.0 ANON-DH with cred but no DH params",
		.client_ret = GNUTLS_E_AGAIN,
		.server_ret = GNUTLS_E_NO_CIPHER_SUITES,
		.have_anon_cred = 1,
		.server_prio = "NORMAL:-KX-ALL:+ANON-DH:-VERS-ALL:+VERS-SSL3.0",
		.client_prio = "NORMAL:-KX-ALL:+ANON-DH:-VERS-ALL:+VERS-SSL3.0"
	},
	{
		.name = "SSL 3.0 ANON-DH with cred and DH params",
		.server_ret = 0,
		.client_ret = 0,
		.have_anon_cred = 1,
		.have_anon_dh_params = 1,
		.server_prio = "NORMAL:-KX-ALL:+ANON-DH:-VERS-ALL:+VERS-SSL3.0",
		.client_prio = "NORMAL:-KX-ALL:+ANON-DH:-VERS-ALL:+VERS-SSL3.0"
	},
	{
		.name = "SSL 3.0 DHE-RSA without cred",
		.client_ret = GNUTLS_E_AGAIN,
		.server_ret = GNUTLS_E_NO_CIPHER_SUITES,
		.server_prio = "NORMAL:-KX-ALL:+DHE-RSA:-VERS-ALL:+VERS-SSL3.0",
		.client_prio = "NORMAL:-KX-ALL:+DHE-RSA:-VERS-ALL:+VERS-SSL3.0"
	},
	{
		.name = "SSL 3.0 DHE-RSA with cred but no DH params or cert",
		.client_ret = GNUTLS_E_AGAIN,
		.server_ret = GNUTLS_E_NO_CIPHER_SUITES,
		.have_cert_cred = 1,
		.server_prio = "NORMAL:-KX-ALL:+DHE-RSA:-VERS-ALL:+VERS-SSL3.0",
		.client_prio = "NORMAL:-KX-ALL:+DHE-RSA:-VERS-ALL:+VERS-SSL3.0"
	},
	{
		.name = "SSL 3.0 DHE-RSA with cred and cert but no DH params",
		.client_ret = GNUTLS_E_AGAIN,
		.server_ret = GNUTLS_E_NO_CIPHER_SUITES,
		.have_cert_cred = 1,
		.have_rsa_sign_cert = 1,
		.server_prio = "NORMAL:-KX-ALL:+DHE-RSA:-VERS-ALL:+VERS-SSL3.0",
		.client_prio = "NORMAL:-KX-ALL:+DHE-RSA:-VERS-ALL:+VERS-SSL3.0"
	},
	{
		.name = "SSL 3.0 DHE-RSA with cred and DH params but no cert",
		.client_ret = GNUTLS_E_AGAIN,
		.server_ret = GNUTLS_E_NO_CIPHER_SUITES,
		.have_cert_cred = 1,
		.have_cert_dh_params = 1,
		.server_prio = "NORMAL:-KX-ALL:+DHE-RSA:-VERS-ALL:+VERS-SSL3.0",
		.client_prio = "NORMAL:-KX-ALL:+DHE-RSA:-VERS-ALL:+VERS-SSL3.0"
	},
	{
		.name = "SSL 3.0 DHE-RSA with cred and incompatible cert and DH params",
		.client_ret = GNUTLS_E_AGAIN,
		.server_ret = GNUTLS_E_NO_CIPHER_SUITES,
		.have_cert_cred = 1,
		.have_rsa_decrypt_cert = 1,
		.have_ecc_sign_cert = 1,
		.have_cert_dh_params = 1,
		.server_prio = "NORMAL:-KX-ALL:+DHE-RSA:-VERS-ALL:+VERS-SSL3.0",
		.client_prio = "NORMAL:-KX-ALL:+DHE-RSA:-VERS-ALL:+VERS-SSL3.0"
	},
	{
		.name = "SSL 3.0 DHE-RSA with cred and cert and DH params",
		.client_ret = 0,
		.server_ret = 0,
		.have_cert_cred = 1,
		.have_rsa_sign_cert = 1,
		.have_cert_dh_params = 1,
		.server_prio = "NORMAL:-KX-ALL:+DHE-RSA:-VERS-ALL:+VERS-SSL3.0",
		.client_prio = "NORMAL:-KX-ALL:+DHE-RSA:-VERS-ALL:+VERS-SSL3.0"
	},
	{
		.name = "SSL 3.0 DHE-RSA with cred and multiple certs and DH params",
		.client_ret = 0,
		.server_ret = 0,
		.have_cert_cred = 1,
		.have_rsa_sign_cert = 1,
		.have_ecc_sign_cert = 1,
		.have_rsa_decrypt_cert = 1,
		.have_cert_dh_params = 1,
		.server_prio = "NORMAL:-KX-ALL:+DHE-RSA:-VERS-ALL:+VERS-SSL3.0",
		.client_prio = "NORMAL:-KX-ALL:+DHE-RSA:-VERS-ALL:+VERS-SSL3.0"
	},
	{
		.name = "SSL 3.0 DHE-PSK without cred",
		.client_ret = GNUTLS_E_AGAIN,
		.server_ret = GNUTLS_E_NO_CIPHER_SUITES,
		.server_prio = "NORMAL:-KX-ALL:+DHE-PSK:-VERS-ALL:+VERS-SSL3.0",
		.client_prio = "NORMAL:-KX-ALL:+DHE-PSK:-VERS-ALL:+VERS-SSL3.0"
	},
	{
		.name = "SSL 3.0 DHE-PSK with cred but no DH params",
		.client_ret = GNUTLS_E_AGAIN,
		.server_ret = GNUTLS_E_NO_CIPHER_SUITES,
		.have_psk_cred = 1,
		.server_prio = "NORMAL:-KX-ALL:+DHE-PSK:-VERS-ALL:+VERS-SSL3.0",
		.client_prio = "NORMAL:-KX-ALL:+DHE-PSK:-VERS-ALL:+VERS-SSL3.0"
	},
	{
		.name = "SSL 3.0 DHE-PSK with cred DH params",
		.client_ret = 0,
		.server_ret = 0,
		.have_psk_cred = 1,
		.have_psk_dh_params = 1,
		.server_prio = "NORMAL:-KX-ALL:+DHE-PSK:-VERS-ALL:+VERS-SSL3.0",
		.client_prio = "NORMAL:-KX-ALL:+DHE-PSK:-VERS-ALL:+VERS-SSL3.0"
	}
};

void doit(void)
{
#ifdef ENABLE_SSL3
	unsigned i;
	global_init();

	for (i=0;i<sizeof(tests)/sizeof(tests[0]);i++) {
		try(&tests[i]);
	}

	gnutls_global_deinit();
#else
	exit(77);
#endif
}
