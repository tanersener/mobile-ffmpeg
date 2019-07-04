/*
 * Copyright (C) 2017-2018 Red Hat, Inc.
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

/* This program tests the negotiation for various key exchange
 * methods and options which are considered legacy in TLS1.3. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <gnutls/gnutls.h>
#include "utils.h"
#include "cert-common.h"
#include "eagain-common.h"

#include "server-kx-neg-common.c"

#define PVERSION "-VERS-ALL:+VERS-TLS1.3:+VERS-TLS1.2"

test_case_st tests[] = {
	{
		.name = "TLS 1.3 DHE-PSK without cred",
		.client_ret = GNUTLS_E_AGAIN,
		.server_ret = GNUTLS_E_INSUFFICIENT_CREDENTIALS,
		.server_prio = "NORMAL:-KX-ALL:+DHE-PSK:"PVERSION,
		.client_prio = "NORMAL:-KX-ALL:+DHE-PSK:"PVERSION,
		.exp_version = GNUTLS_TLS1_3,
	},
	{
		.name = "TLS 1.3 DHE-PSK with cred but no DH params",
		.client_ret = 0,
		.server_ret = 0,
		.have_psk_cred = 1,
		.server_prio = "NORMAL:-KX-ALL:+DHE-PSK:"PVERSION,
		.client_prio = "NORMAL:-KX-ALL:+DHE-PSK:"PVERSION,
		.exp_version = GNUTLS_TLS1_3,
	},
	{
		.name = "TLS 1.3 DHE-PSK with cred and DH params (level)",
		.client_ret = 0,
		.server_ret = 0,
		.have_psk_cred = 1,
		.have_psk_dh_params = 1,
		.server_prio = "NORMAL:-KX-ALL:+DHE-PSK:"PVERSION,
		.client_prio = "NORMAL:-KX-ALL:+DHE-PSK:"PVERSION,
		.exp_version = GNUTLS_TLS1_3,
	},
	{
		.name = "TLS 1.3 DHE-PSK with cred and DH params (explicit)",
		.client_ret = 0,
		.server_ret = 0,
		.have_psk_cred = 1,
		.have_psk_exp_dh_params = 1,
		.server_prio = "NORMAL:-KX-ALL:+DHE-PSK:"PVERSION,
		.client_prio = "NORMAL:-KX-ALL:+DHE-PSK:"PVERSION,
		.exp_version = GNUTLS_TLS1_3,
	},
	{
		.name = "TLS 1.3 ECDHE-PSK with cred but no common curve",
		.client_ret = GNUTLS_E_AGAIN,
		.server_ret = GNUTLS_E_NO_COMMON_KEY_SHARE,
		.have_psk_cred = 1,
		.server_prio = "NORMAL:-KX-ALL:+ECDHE-PSK:-CURVE-ALL:+CURVE-SECP256R1:"PVERSION,
		.client_prio = "NORMAL:-KX-ALL:+ECDHE-PSK:-CURVE-ALL:+CURVE-SECP384R1:"PVERSION,
		.exp_version = GNUTLS_TLS1_3,
	},
	{
		.name = "TLS 1.3 ECDHE-PSK with cred and common curve",
		.client_ret = 0,
		.server_ret = 0,
		.have_psk_cred = 1,
		.server_prio = "NORMAL:-KX-ALL:+ECDHE-PSK:"PVERSION,
		.client_prio = "NORMAL:-KX-ALL:+ECDHE-PSK:"PVERSION,
		.exp_version = GNUTLS_TLS1_3,
	},
	{
		.name = "TLS 1.3 RSA-PSK without cert cred",
		.client_ret = GNUTLS_E_AGAIN,
		.server_ret = GNUTLS_E_INSUFFICIENT_CREDENTIALS,
		.have_psk_cred = 1,
		.server_prio = "NORMAL:-KX-ALL:+RSA-PSK:"PVERSION,
		.client_prio = "NORMAL:-KX-ALL:+RSA-PSK:"PVERSION,
		.exp_version = GNUTLS_TLS1_2,
	},
	{
		.name = "TLS 1.3 RSA-PSK without psk cred",
		.client_ret = GNUTLS_E_AGAIN,
		.server_ret = GNUTLS_E_NO_CIPHER_SUITES,
		.have_psk_cred = 0,
		.have_cert_cred = 1,
		.server_prio = "NORMAL:-KX-ALL:+RSA-PSK:"PVERSION,
		.client_prio = "NORMAL:-KX-ALL:+RSA-PSK:"PVERSION,
		.exp_version = GNUTLS_TLS1_2,
	},
	{
		.name = "TLS 1.3 RSA-PSK with cred but invalid cert",
		.client_ret = GNUTLS_E_AGAIN,
		.server_ret = GNUTLS_E_NO_CIPHER_SUITES,
		.have_psk_cred = 1,
		.have_cert_cred = 1,
		.have_rsa_sign_cert = 1,
		.have_ecc_sign_cert = 1,
		.server_prio = "NORMAL:-KX-ALL:+RSA-PSK:"PVERSION,
		.client_prio = "NORMAL:-KX-ALL:+RSA-PSK:"PVERSION,
		.exp_version = GNUTLS_TLS1_2,
	},
	{
		.name = "TLS 1.3 RSA-PSK with cred",
		.server_ret = 0,
		.client_ret = 0,
		.have_psk_cred = 1,
		.have_cert_cred = 1,
		.have_rsa_decrypt_cert = 1,
		.server_prio = "NORMAL:-KX-ALL:+RSA-PSK:"PVERSION,
		.client_prio = "NORMAL:-KX-ALL:+RSA-PSK:"PVERSION,
		.exp_version = GNUTLS_TLS1_2,
	},
	{
		.name = "TLS 1.3 RSA-PSK with cred and multiple certs",
		.server_ret = 0,
		.client_ret = 0,
		.have_psk_cred = 1,
		.have_cert_cred = 1,
		.have_rsa_sign_cert = 1,
		.have_ecc_sign_cert = 1,
		.have_rsa_decrypt_cert = 1,
		.server_prio = "NORMAL:-KX-ALL:+RSA-PSK:"PVERSION,
		.client_prio = "NORMAL:-KX-ALL:+RSA-PSK:"PVERSION,
		.exp_version = GNUTLS_TLS1_2,
	},
	{
		.name = "TLS 1.3 SRP-RSA without cert cred",
		.client_ret = GNUTLS_E_AGAIN,
		.server_ret = GNUTLS_E_INSUFFICIENT_CREDENTIALS,
		.have_srp_cred = 1,
		.server_prio = "NORMAL:-KX-ALL:+SRP-RSA:"PVERSION,
		.client_prio = "NORMAL:-KX-ALL:+SRP-RSA:"PVERSION,
		.exp_version = GNUTLS_TLS1_2,
	},
	{
		.name = "TLS 1.3 SRP-RSA without srp cred",
		.client_ret = GNUTLS_E_AGAIN,
		.server_ret = GNUTLS_E_NO_CIPHER_SUITES,
		.have_srp_cred = 0,
		.have_cert_cred = 1,
		.server_prio = "NORMAL:-KX-ALL:+SRP-RSA:"PVERSION,
		.client_prio = "NORMAL:-KX-ALL:+SRP-RSA:"PVERSION,
		.exp_version = GNUTLS_TLS1_2,
	},
	{
		.name = "TLS 1.3 SRP-RSA with cred but invalid cert",
		.client_ret = GNUTLS_E_AGAIN,
		.server_ret = GNUTLS_E_NO_CIPHER_SUITES,
		.have_srp_cred = 1,
		.have_cert_cred = 1,
		.have_rsa_decrypt_cert = 1,
		.have_ecc_sign_cert = 1,
		.server_prio = "NORMAL:-KX-ALL:+SRP-RSA:"PVERSION,
		.client_prio = "NORMAL:-KX-ALL:+SRP-RSA:"PVERSION,
		.exp_version = GNUTLS_TLS1_2,
	},
	{
		.name = "TLS 1.3 SRP-RSA with cred",
		.server_ret = 0,
		.client_ret = 0,
		.have_srp_cred = 1,
		.have_cert_cred = 1,
		.have_rsa_sign_cert = 1,
		.server_prio = "NORMAL:-KX-ALL:+SRP-RSA:"PVERSION,
		.client_prio = "NORMAL:-KX-ALL:+SRP-RSA:"PVERSION,
		.exp_version = GNUTLS_TLS1_2,
	},
	{
		.name = "TLS 1.3 SRP-RSA with cred and multiple certs",
		.server_ret = 0,
		.client_ret = 0,
		.have_srp_cred = 1,
		.have_cert_cred = 1,
		.have_rsa_sign_cert = 1,
		.have_ecc_sign_cert = 1,
		.have_rsa_decrypt_cert = 1,
		.server_prio = "NORMAL:-KX-ALL:+SRP-RSA:"PVERSION,
		.client_prio = "NORMAL:-KX-ALL:+SRP-RSA:"PVERSION,
		.exp_version = GNUTLS_TLS1_2,
	},
	{
		.name = "TLS 1.3 SRP without srp cred",
		.client_ret = GNUTLS_E_AGAIN,
		.server_ret = GNUTLS_E_INSUFFICIENT_CREDENTIALS,
		.have_srp_cred = 0,
		.have_cert_cred = 1,
		.server_prio = "NORMAL:-KX-ALL:+SRP:"PVERSION,
		.client_prio = "NORMAL:-KX-ALL:+SRP:"PVERSION,
		.exp_version = GNUTLS_TLS1_2,
	},
	{
		.name = "TLS 1.3 SRP with cred",
		.server_ret = 0,
		.client_ret = 0,
		.have_srp_cred = 1,
		.server_prio = "NORMAL:-KX-ALL:+SRP:"PVERSION,
		.client_prio = "NORMAL:-KX-ALL:+SRP:"PVERSION,
		.exp_version = GNUTLS_TLS1_2,
	}
};

void doit(void)
{
	unsigned i;
	global_init();

	for (i=0;i<sizeof(tests)/sizeof(tests[0]);i++) {
		try(&tests[i]);
	}

	gnutls_global_deinit();
}
