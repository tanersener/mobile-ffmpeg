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
		.name = "TLS 1.2 ANON-DH without cred",
		.client_ret = GNUTLS_E_AGAIN,
		.server_ret = GNUTLS_E_INSUFFICIENT_CREDENTIALS,
		.server_prio = "NORMAL:-KX-ALL:+ANON-DH:-VERS-ALL:+VERS-TLS1.2",
		.client_prio = "NORMAL:-KX-ALL:+ANON-DH:-VERS-ALL:+VERS-TLS1.2"
	},
	{
		.name = "TLS 1.2 ANON-DH with cred but no DH params",
		.client_ret = 0,
		.server_ret = 0,
		.have_anon_cred = 1,
		.server_prio = "NORMAL:-KX-ALL:+ANON-DH:-VERS-ALL:+VERS-TLS1.2",
		.client_prio = "NORMAL:-KX-ALL:+ANON-DH:-VERS-ALL:+VERS-TLS1.2"
	},
	{
		.name = "TLS 1.2 ANON-DH with cred and DH params (level)",
		.server_ret = 0,
		.client_ret = 0,
		.have_anon_cred = 1,
		.have_anon_dh_params = 1,
		.server_prio = "NORMAL:-KX-ALL:+ANON-DH:-VERS-ALL:+VERS-TLS1.2",
		.client_prio = "NORMAL:-KX-ALL:+ANON-DH:-VERS-ALL:+VERS-TLS1.2"
	},
	{
		.name = "TLS 1.2 ANON-DH with cred and DH params (explicit)",
		.server_ret = 0,
		.client_ret = 0,
		.have_anon_cred = 1,
		.have_anon_exp_dh_params = 1,
		.server_prio = "NORMAL:-KX-ALL:+ANON-DH:-VERS-ALL:+VERS-TLS1.2",
		.client_prio = "NORMAL:-KX-ALL:+ANON-DH:-VERS-ALL:+VERS-TLS1.2"
	},
	{
		.name = "TLS 1.2 DHE-RSA without cred",
		.client_ret = GNUTLS_E_AGAIN,
		.server_ret = GNUTLS_E_INSUFFICIENT_CREDENTIALS,
		.server_prio = "NORMAL:-KX-ALL:+DHE-RSA:-VERS-ALL:+VERS-TLS1.2",
		.client_prio = "NORMAL:-KX-ALL:+DHE-RSA:-VERS-ALL:+VERS-TLS1.2"
	},
	{
		.name = "TLS 1.2 DHE-RSA with cred but no DH params or cert",
		.client_ret = GNUTLS_E_AGAIN,
		.server_ret = GNUTLS_E_NO_CIPHER_SUITES,
		.have_cert_cred = 1,
		.server_prio = "NORMAL:-KX-ALL:+DHE-RSA:-VERS-ALL:+VERS-TLS1.2",
		.client_prio = "NORMAL:-KX-ALL:+DHE-RSA:-VERS-ALL:+VERS-TLS1.2"
	},
	{
		.name = "TLS 1.2 DHE-RSA with cred and cert but no DH params",
		.client_ret = 0,
		.server_ret = 0,
		.have_cert_cred = 1,
		.have_rsa_sign_cert = 1,
		.server_prio = "NORMAL:-KX-ALL:+DHE-RSA:-VERS-ALL:+VERS-TLS1.2",
		.client_prio = "NORMAL:-KX-ALL:+DHE-RSA:-VERS-ALL:+VERS-TLS1.2"
	},
	{
		.name = "TLS 1.2 DHE-RSA with cred and DH params but no cert",
		.client_ret = GNUTLS_E_AGAIN,
		.server_ret = GNUTLS_E_NO_CIPHER_SUITES,
		.have_cert_cred = 1,
		.have_cert_dh_params = 1,
		.server_prio = "NORMAL:-KX-ALL:+DHE-RSA:-VERS-ALL:+VERS-TLS1.2",
		.client_prio = "NORMAL:-KX-ALL:+DHE-RSA:-VERS-ALL:+VERS-TLS1.2"
	},
	{
		.name = "TLS 1.2 DHE-RSA with cred and incompatible cert and DH params",
		.client_ret = GNUTLS_E_AGAIN,
		.server_ret = GNUTLS_E_NO_CIPHER_SUITES,
		.have_cert_cred = 1,
		.have_rsa_decrypt_cert = 1,
		.have_ecc_sign_cert = 1,
		.have_cert_dh_params = 1,
		.server_prio = "NORMAL:-KX-ALL:+DHE-RSA:-VERS-ALL:+VERS-TLS1.2",
		.client_prio = "NORMAL:-KX-ALL:+DHE-RSA:-VERS-ALL:+VERS-TLS1.2"
	},
	{
		.name = "TLS 1.2 DHE-RSA with cred and cert and DH params (level)",
		.client_ret = 0,
		.server_ret = 0,
		.have_cert_cred = 1,
		.have_rsa_sign_cert = 1,
		.have_cert_dh_params = 1,
		.server_prio = "NORMAL:-KX-ALL:+DHE-RSA:-VERS-ALL:+VERS-TLS1.2",
		.client_prio = "NORMAL:-KX-ALL:+DHE-RSA:-VERS-ALL:+VERS-TLS1.2"
	},
	{
		.name = "TLS 1.2 DHE-RSA with cred and cert and DH params (explicit)",
		.client_ret = 0,
		.server_ret = 0,
		.have_cert_cred = 1,
		.have_rsa_sign_cert = 1,
		.have_cert_exp_dh_params = 1,
		.server_prio = "NORMAL:-KX-ALL:+DHE-RSA:-VERS-ALL:+VERS-TLS1.2",
		.client_prio = "NORMAL:-KX-ALL:+DHE-RSA:-VERS-ALL:+VERS-TLS1.2"
	},
	{
		.name = "TLS 1.2 DHE-RSA with cred and multiple certs and DH params",
		.client_ret = 0,
		.server_ret = 0,
		.have_cert_cred = 1,
		.have_rsa_sign_cert = 1,
		.have_ecc_sign_cert = 1,
		.have_rsa_decrypt_cert = 1,
		.have_cert_dh_params = 1,
		.server_prio = "NORMAL:-KX-ALL:+DHE-RSA:-VERS-ALL:+VERS-TLS1.2",
		.client_prio = "NORMAL:-KX-ALL:+DHE-RSA:-VERS-ALL:+VERS-TLS1.2"
	},
	{
		.name = "TLS 1.2 DHE-PSK without cred",
		.client_ret = GNUTLS_E_AGAIN,
		.server_ret = GNUTLS_E_INSUFFICIENT_CREDENTIALS,
		.server_prio = "NORMAL:-KX-ALL:+DHE-PSK:-VERS-ALL:+VERS-TLS1.2",
		.client_prio = "NORMAL:-KX-ALL:+DHE-PSK:-VERS-ALL:+VERS-TLS1.2"
	},
	{
		.name = "TLS 1.2 DHE-PSK with cred but no DH params",
		.client_ret = 0,
		.server_ret = 0,
		.have_psk_cred = 1,
		.server_prio = "NORMAL:-KX-ALL:+DHE-PSK:-VERS-ALL:+VERS-TLS1.2",
		.client_prio = "NORMAL:-KX-ALL:+DHE-PSK:-VERS-ALL:+VERS-TLS1.2"
	},
	{
		.name = "TLS 1.2 DHE-PSK with cred and DH params (level)",
		.client_ret = 0,
		.server_ret = 0,
		.have_psk_cred = 1,
		.have_psk_dh_params = 1,
		.server_prio = "NORMAL:-KX-ALL:+DHE-PSK:-VERS-ALL:+VERS-TLS1.2",
		.client_prio = "NORMAL:-KX-ALL:+DHE-PSK:-VERS-ALL:+VERS-TLS1.2"
	},
	{
		.name = "TLS 1.2 DHE-PSK with cred and DH params (explicit)",
		.client_ret = 0,
		.server_ret = 0,
		.have_psk_cred = 1,
		.have_psk_exp_dh_params = 1,
		.server_prio = "NORMAL:-KX-ALL:+DHE-PSK:-VERS-ALL:+VERS-TLS1.2",
		.client_prio = "NORMAL:-KX-ALL:+DHE-PSK:-VERS-ALL:+VERS-TLS1.2"
	},
	{
		.name = "TLS 1.2 ECDHE-RSA without cred",
		.client_ret = GNUTLS_E_AGAIN,
		.server_ret = GNUTLS_E_INSUFFICIENT_CREDENTIALS,
		.server_prio = "NORMAL:-KX-ALL:+ECDHE-RSA:-VERS-ALL:+VERS-TLS1.2",
		.client_prio = "NORMAL:-KX-ALL:+ECDHE-RSA:-VERS-ALL:+VERS-TLS1.2"
	},
	{
		.name = "TLS 1.2 ECDHE-RSA with cred but no common curve or cert",
		.client_ret = GNUTLS_E_AGAIN,
		.server_ret = GNUTLS_E_NO_CIPHER_SUITES,
		.have_cert_cred = 1,
		.server_prio = "NORMAL:-KX-ALL:+ECDHE-RSA:-VERS-ALL:+VERS-TLS1.2:-CURVE-ALL:+CURVE-SECP256R1",
		.client_prio = "NORMAL:-KX-ALL:+ECDHE-RSA:-VERS-ALL:+VERS-TLS1.2:-CURVE-ALL:+CURVE-SECP384R1"
	},
	{
		.name = "TLS 1.2 ECDHE-RSA with cred and cert but no common curve",
		.client_ret = GNUTLS_E_AGAIN,
		.server_ret = GNUTLS_E_NO_CIPHER_SUITES,
		.have_cert_cred = 1,
		.have_rsa_sign_cert = 1,
		.server_prio = "NORMAL:-KX-ALL:+ECDHE-RSA:-VERS-ALL:+VERS-TLS1.2:-CURVE-ALL:+CURVE-SECP256R1",
		.client_prio = "NORMAL:-KX-ALL:+ECDHE-RSA:-VERS-ALL:+VERS-TLS1.2:-CURVE-ALL:+CURVE-SECP384R1"
	},
	{
		.name = "TLS 1.2 ECDHE-RSA with cred and common curve but no cert",
		.client_ret = GNUTLS_E_AGAIN,
		.server_ret = GNUTLS_E_NO_CIPHER_SUITES,
		.have_cert_cred = 1,
		.server_prio = "NORMAL:-KX-ALL:+ECDHE-RSA:-VERS-ALL:+VERS-TLS1.2",
		.client_prio = "NORMAL:-KX-ALL:+ECDHE-RSA:-VERS-ALL:+VERS-TLS1.2"
	},
	{
		.name = "TLS 1.2 ECDHE-RSA with cred and incompatible cert and common curve",
		.client_ret = GNUTLS_E_AGAIN,
		.server_ret = GNUTLS_E_NO_CIPHER_SUITES,
		.have_cert_cred = 1,
		.have_rsa_decrypt_cert = 1,
		.server_prio = "NORMAL:-KX-ALL:+ECDHE-RSA:-VERS-ALL:+VERS-TLS1.2",
		.client_prio = "NORMAL:-KX-ALL:+ECDHE-RSA:-VERS-ALL:+VERS-TLS1.2"
	},
	{
		.name = "TLS 1.2 ECDHE-RSA with cred and cert and common curve",
		.server_ret = 0,
		.client_ret = 0,
		.have_cert_cred = 1,
		.have_rsa_sign_cert = 1,
		.server_prio = "NORMAL:-KX-ALL:+ECDHE-RSA:-VERS-ALL:+VERS-TLS1.2",
		.client_prio = "NORMAL:-KX-ALL:+ECDHE-RSA:-VERS-ALL:+VERS-TLS1.2"
	},
	{
		.name = "TLS 1.2 ECDHE-RSA with cred and multiple certs and common curve",
		.server_ret = 0,
		.client_ret = 0,
		.have_cert_cred = 1,
		.have_rsa_decrypt_cert = 1,
		.have_rsa_sign_cert = 1,
		.have_ecc_sign_cert = 1,
		.server_prio = "NORMAL:-KX-ALL:+ECDHE-RSA:-VERS-ALL:+VERS-TLS1.2",
		.client_prio = "NORMAL:-KX-ALL:+ECDHE-RSA:-VERS-ALL:+VERS-TLS1.2"
	},

	{
		.name = "TLS 1.2 ECDHE-ECDSA without cred",
		.client_ret = GNUTLS_E_AGAIN,
		.server_ret = GNUTLS_E_INSUFFICIENT_CREDENTIALS,
		.server_prio = "NORMAL:-KX-ALL:+ECDHE-ECDSA:-VERS-ALL:+VERS-TLS1.2",
		.client_prio = "NORMAL:-KX-ALL:+ECDHE-ECDSA:-VERS-ALL:+VERS-TLS1.2"
	},
	{
		.name = "TLS 1.2 ECDHE-ECDSA with cred but no common curve or cert",
		.client_ret = GNUTLS_E_AGAIN,
		.server_ret = GNUTLS_E_NO_CIPHER_SUITES,
		.have_cert_cred = 1,
		.server_prio = "NORMAL:-KX-ALL:+ECDHE-ECDSA:-VERS-ALL:+VERS-TLS1.2:-CURVE-ALL:+CURVE-SECP256R1",
		.client_prio = "NORMAL:-KX-ALL:+ECDHE-ECDSA:-VERS-ALL:+VERS-TLS1.2:-CURVE-ALL:+CURVE-SECP384R1"
	},
	{
		.name = "TLS 1.2 ECDHE-ECDSA with cred and cert but no common curve",
		.client_ret = GNUTLS_E_AGAIN,
		.server_ret = GNUTLS_E_NO_CIPHER_SUITES,
		.have_cert_cred = 1,
		.have_ecc_sign_cert = 1,
		.server_prio = "NORMAL:-KX-ALL:+ECDHE-ECDSA:-VERS-ALL:+VERS-TLS1.2:-CURVE-ALL:+CURVE-SECP256R1",
		.client_prio = "NORMAL:-KX-ALL:+ECDHE-ECDSA:-VERS-ALL:+VERS-TLS1.2:-CURVE-ALL:+CURVE-SECP384R1"
	},
	{
		.name = "TLS 1.2 ECDHE-ECDSA with cred and common curve but no ECDSA cert",
		.client_ret = GNUTLS_E_AGAIN,
		.server_ret = GNUTLS_E_NO_CIPHER_SUITES,
		.have_cert_cred = 1,
		.have_rsa_sign_cert = 1,
		.have_rsa_decrypt_cert = 1,
		.server_prio = "NORMAL:-KX-ALL:+ECDHE-ECDSA:-VERS-ALL:+VERS-TLS1.2",
		.client_prio = "NORMAL:-KX-ALL:+ECDHE-ECDSA:-VERS-ALL:+VERS-TLS1.2"
	},
	{
		.name = "TLS 1.2 ECDHE-ECDSA with cred and common curve but no cert",
		.client_ret = GNUTLS_E_AGAIN,
		.server_ret = GNUTLS_E_NO_CIPHER_SUITES,
		.have_cert_cred = 1,
		.server_prio = "NORMAL:-KX-ALL:+ECDHE-ECDSA:-VERS-ALL:+VERS-TLS1.2",
		.client_prio = "NORMAL:-KX-ALL:+ECDHE-ECDSA:-VERS-ALL:+VERS-TLS1.2"
	},
	{
		.name = "TLS 1.2 ECDHE-ECDSA with cred and cert and common curve",
		.server_ret = 0,
		.client_ret = 0,
		.have_cert_cred = 1,
		.have_ecc_sign_cert = 1,
		.server_prio = "NORMAL:-KX-ALL:+ECDHE-ECDSA:-VERS-ALL:+VERS-TLS1.2",
		.client_prio = "NORMAL:-KX-ALL:+ECDHE-ECDSA:-VERS-ALL:+VERS-TLS1.2"
	},
	{
		.name = "TLS 1.2 ECDHE-ECDSA with cred and multiple certs and common curve",
		.server_ret = 0,
		.client_ret = 0,
		.have_cert_cred = 1,
		.have_ecc_sign_cert = 1,
		.have_rsa_sign_cert = 1,
		.have_rsa_decrypt_cert = 1,
		.server_prio = "NORMAL:-KX-ALL:+ECDHE-ECDSA:-VERS-ALL:+VERS-TLS1.2",
		.client_prio = "NORMAL:-KX-ALL:+ECDHE-ECDSA:-VERS-ALL:+VERS-TLS1.2"
	},
	{
		.name = "TLS 1.2 ECDHE-ECDSA with cred and ed25519 cert",
		.server_ret = 0,
		.client_ret = 0,
		.have_cert_cred = 1,
		.have_ed25519_sign_cert = 1,
		.server_prio = "NORMAL:-KX-ALL:+ECDHE-ECDSA:-VERS-ALL:+VERS-TLS1.2",
		.client_prio = "NORMAL:-KX-ALL:+ECDHE-ECDSA:-VERS-ALL:+VERS-TLS1.2"
	},
	{
		.name = "TLS 1.2 ECDHE-ECDSA with cred and cert but incompatible (ed25519) curves",
		.server_ret = GNUTLS_E_NO_CIPHER_SUITES,
		.client_ret = GNUTLS_E_AGAIN,
		.have_cert_cred = 1,
		.have_ed25519_sign_cert = 1,
		.server_prio = "NORMAL:-KX-ALL:+ECDHE-ECDSA:-VERS-ALL:+VERS-TLS1.2",
		.client_prio = "NORMAL:-KX-ALL:+ECDHE-ECDSA:-VERS-ALL:+VERS-TLS1.2:-CURVE-ED25519:-SIGN-EDDSA-ED25519"
	},
	{
		.name = "TLS 1.2 ECDHE-PSK without cred",
		.client_ret = GNUTLS_E_AGAIN,
		.server_ret = GNUTLS_E_INSUFFICIENT_CREDENTIALS,
		.server_prio = "NORMAL:-KX-ALL:+ECDHE-PSK:-VERS-ALL:+VERS-TLS1.2",
		.client_prio = "NORMAL:-KX-ALL:+ECDHE-PSK:-VERS-ALL:+VERS-TLS1.2"
	},
	{
		.name = "TLS 1.2 ECDHE-PSK with cred but no common curve",
		.client_ret = GNUTLS_E_AGAIN,
		.server_ret = GNUTLS_E_NO_CIPHER_SUITES,
		.have_psk_cred = 1,
		.server_prio = "NORMAL:-KX-ALL:+ECDHE-PSK:-VERS-ALL:+VERS-TLS1.2:-CURVE-ALL:+CURVE-SECP256R1",
		.client_prio = "NORMAL:-KX-ALL:+ECDHE-PSK:-VERS-ALL:+VERS-TLS1.2:-CURVE-ALL:+CURVE-SECP384R1"
	},
	{
		.name = "TLS 1.2 ECDHE-PSK with cred and common curve",
		.client_ret = 0,
		.server_ret = 0,
		.have_psk_cred = 1,
		.server_prio = "NORMAL:-KX-ALL:+ECDHE-PSK:-VERS-ALL:+VERS-TLS1.2",
		.client_prio = "NORMAL:-KX-ALL:+ECDHE-PSK:-VERS-ALL:+VERS-TLS1.2"
	},
	{
		.name = "TLS 1.2 RSA-PSK without cert cred",
		.client_ret = GNUTLS_E_AGAIN,
		.server_ret = GNUTLS_E_INSUFFICIENT_CREDENTIALS,
		.have_psk_cred = 1,
		.server_prio = "NORMAL:-KX-ALL:+RSA-PSK:-VERS-ALL:+VERS-TLS1.2",
		.client_prio = "NORMAL:-KX-ALL:+RSA-PSK:-VERS-ALL:+VERS-TLS1.2"
	},
	{
		.name = "TLS 1.2 RSA-PSK without psk cred",
		.client_ret = GNUTLS_E_AGAIN,
		.server_ret = GNUTLS_E_NO_CIPHER_SUITES,
		.have_psk_cred = 0,
		.have_cert_cred = 1,
		.server_prio = "NORMAL:-KX-ALL:+RSA-PSK:-VERS-ALL:+VERS-TLS1.2",
		.client_prio = "NORMAL:-KX-ALL:+RSA-PSK:-VERS-ALL:+VERS-TLS1.2"
	},
	{
		.name = "TLS 1.2 RSA-PSK with cred but invalid cert",
		.client_ret = GNUTLS_E_AGAIN,
		.server_ret = GNUTLS_E_NO_CIPHER_SUITES,
		.have_psk_cred = 1,
		.have_cert_cred = 1,
		.have_rsa_sign_cert = 1,
		.have_ecc_sign_cert = 1,
		.server_prio = "NORMAL:-KX-ALL:+RSA-PSK:-VERS-ALL:+VERS-TLS1.2",
		.client_prio = "NORMAL:-KX-ALL:+RSA-PSK:-VERS-ALL:+VERS-TLS1.2"
	},
	{
		.name = "TLS 1.2 RSA-PSK with cred",
		.server_ret = 0,
		.client_ret = 0,
		.have_psk_cred = 1,
		.have_cert_cred = 1,
		.have_rsa_decrypt_cert = 1,
		.server_prio = "NORMAL:-KX-ALL:+RSA-PSK:-VERS-ALL:+VERS-TLS1.2",
		.client_prio = "NORMAL:-KX-ALL:+RSA-PSK:-VERS-ALL:+VERS-TLS1.2"
	},
	{
		.name = "TLS 1.2 RSA-PSK with cred and multiple certs",
		.server_ret = 0,
		.client_ret = 0,
		.have_psk_cred = 1,
		.have_cert_cred = 1,
		.have_rsa_sign_cert = 1,
		.have_ecc_sign_cert = 1,
		.have_rsa_decrypt_cert = 1,
		.server_prio = "NORMAL:-KX-ALL:+RSA-PSK:-VERS-ALL:+VERS-TLS1.2",
		.client_prio = "NORMAL:-KX-ALL:+RSA-PSK:-VERS-ALL:+VERS-TLS1.2"
	},
	{
		.name = "TLS 1.2 SRP-RSA without cert cred",
		.client_ret = GNUTLS_E_AGAIN,
		.server_ret = GNUTLS_E_INSUFFICIENT_CREDENTIALS,
		.have_srp_cred = 1,
		.server_prio = "NORMAL:-KX-ALL:+SRP-RSA:-VERS-ALL:+VERS-TLS1.2",
		.client_prio = "NORMAL:-KX-ALL:+SRP-RSA:-VERS-ALL:+VERS-TLS1.2"
	},
	{
		.name = "TLS 1.2 SRP-RSA without srp cred",
		.client_ret = GNUTLS_E_AGAIN,
		.server_ret = GNUTLS_E_NO_CIPHER_SUITES,
		.have_srp_cred = 0,
		.have_cert_cred = 1,
		.server_prio = "NORMAL:-KX-ALL:+SRP-RSA:-VERS-ALL:+VERS-TLS1.2",
		.client_prio = "NORMAL:-KX-ALL:+SRP-RSA:-VERS-ALL:+VERS-TLS1.2"
	},
	{
		.name = "TLS 1.2 SRP-RSA with cred but invalid cert",
		.client_ret = GNUTLS_E_AGAIN,
		.server_ret = GNUTLS_E_NO_CIPHER_SUITES,
		.have_srp_cred = 1,
		.have_cert_cred = 1,
		.have_rsa_decrypt_cert = 1,
		.have_ecc_sign_cert = 1,
		.server_prio = "NORMAL:-KX-ALL:+SRP-RSA:-VERS-ALL:+VERS-TLS1.2",
		.client_prio = "NORMAL:-KX-ALL:+SRP-RSA:-VERS-ALL:+VERS-TLS1.2"
	},
	{
		.name = "TLS 1.2 SRP-RSA with cred",
		.server_ret = 0,
		.client_ret = 0,
		.have_srp_cred = 1,
		.have_cert_cred = 1,
		.have_rsa_sign_cert = 1,
		.server_prio = "NORMAL:-KX-ALL:+SRP-RSA:-VERS-ALL:+VERS-TLS1.2",
		.client_prio = "NORMAL:-KX-ALL:+SRP-RSA:-VERS-ALL:+VERS-TLS1.2"
	},
	{
		.name = "TLS 1.2 SRP-RSA with cred and multiple certs",
		.server_ret = 0,
		.client_ret = 0,
		.have_srp_cred = 1,
		.have_cert_cred = 1,
		.have_rsa_sign_cert = 1,
		.have_ecc_sign_cert = 1,
		.have_rsa_decrypt_cert = 1,
		.server_prio = "NORMAL:-KX-ALL:+SRP-RSA:-VERS-ALL:+VERS-TLS1.2",
		.client_prio = "NORMAL:-KX-ALL:+SRP-RSA:-VERS-ALL:+VERS-TLS1.2"
	},
	{
		.name = "TLS 1.2 SRP without srp cred",
		.client_ret = GNUTLS_E_AGAIN,
		.server_ret = GNUTLS_E_INSUFFICIENT_CREDENTIALS,
		.have_srp_cred = 0,
		.have_cert_cred = 1,
		.server_prio = "NORMAL:-KX-ALL:+SRP:-VERS-ALL:+VERS-TLS1.2",
		.client_prio = "NORMAL:-KX-ALL:+SRP:-VERS-ALL:+VERS-TLS1.2"
	},
	{
		.name = "TLS 1.2 SRP with cred",
		.server_ret = 0,
		.client_ret = 0,
		.have_srp_cred = 1,
		.server_prio = "NORMAL:-KX-ALL:+SRP:-VERS-ALL:+VERS-TLS1.2",
		.client_prio = "NORMAL:-KX-ALL:+SRP:-VERS-ALL:+VERS-TLS1.2"
	},

#ifdef ENABLE_GOST
	{
		.name = "TLS 1.2 VKO-GOST-12 without cred",
		.client_ret = GNUTLS_E_AGAIN,
		.server_ret = GNUTLS_E_INSUFFICIENT_CREDENTIALS,
		.not_on_fips = 1,
		.server_prio = "NORMAL:-KX-ALL:+VKO-GOST-12:+GROUP-GOST-ALL:+CIPHER-GOST-ALL:+MAC-GOST-ALL:+SIGN-GOST-ALL:-VERS-ALL:+VERS-TLS1.2",
		.client_prio = "NORMAL:-KX-ALL:+VKO-GOST-12:+GROUP-GOST-ALL:+CIPHER-GOST-ALL:+MAC-GOST-ALL:+SIGN-GOST-ALL:-VERS-ALL:+VERS-TLS1.2"
	},
	{
		.name = "TLS 1.2 VKO-GOST-12 with cred but no cert",
		.client_ret = GNUTLS_E_AGAIN,
		.server_ret = GNUTLS_E_NO_CIPHER_SUITES,
		.have_cert_cred = 1,
		.not_on_fips = 1,
		.server_prio = "NORMAL:-KX-ALL:+VKO-GOST-12:+GROUP-GOST-ALL:+CIPHER-GOST-ALL:+MAC-GOST-ALL:+SIGN-GOST-ALL:-VERS-ALL:+VERS-TLS1.2",
		.client_prio = "NORMAL:-KX-ALL:+VKO-GOST-12:+GROUP-GOST-ALL:+CIPHER-GOST-ALL:+MAC-GOST-ALL:+SIGN-GOST-ALL:-VERS-ALL:+VERS-TLS1.2"
	},
	{
		.name = "TLS 1.2 VKO-GOST-12 with cred but no GOST cert",
		.client_ret = GNUTLS_E_AGAIN,
		.server_ret = GNUTLS_E_NO_CIPHER_SUITES,
		.have_cert_cred = 1,
		.have_rsa_sign_cert = 1,
		.have_rsa_decrypt_cert = 1,
		.not_on_fips = 1,
		.server_prio = "NORMAL:-KX-ALL:+VKO-GOST-12:+GROUP-GOST-ALL:+CIPHER-GOST-ALL:+MAC-GOST-ALL:+SIGN-GOST-ALL:-VERS-ALL:+VERS-TLS1.2",
		.client_prio = "NORMAL:-KX-ALL:+VKO-GOST-12:+GROUP-GOST-ALL:+CIPHER-GOST-ALL:+MAC-GOST-ALL:+SIGN-GOST-ALL:-VERS-ALL:+VERS-TLS1.2"
	},
	{
		.name = "TLS 1.2 VKO-GOST-12 with cred and GOST12-256 cert",
		.server_ret = 0,
		.client_ret = 0,
		.have_cert_cred = 1,
		.have_gost12_256_cert = 1,
		.not_on_fips = 1,
		.server_prio = "NORMAL:-KX-ALL:+VKO-GOST-12:+GROUP-GOST-ALL:+CIPHER-GOST-ALL:+MAC-GOST-ALL:+SIGN-GOST-ALL:-VERS-ALL:+VERS-TLS1.2",
		.client_prio = "NORMAL:-KX-ALL:+VKO-GOST-12:+GROUP-GOST-ALL:+CIPHER-GOST-ALL:+MAC-GOST-ALL:+SIGN-GOST-ALL:-VERS-ALL:+VERS-TLS1.2"
	},
	{
		.name = "TLS 1.2 VKO-GOST-12 with cred and GOST12-512 cert",
		.server_ret = 0,
		.client_ret = 0,
		.have_cert_cred = 1,
		.have_gost12_512_cert = 1,
		.not_on_fips = 1,
		.server_prio = "NORMAL:-KX-ALL:+VKO-GOST-12:+GROUP-GOST-ALL:+CIPHER-GOST-ALL:+MAC-GOST-ALL:+SIGN-GOST-ALL:-VERS-ALL:+VERS-TLS1.2",
		.client_prio = "NORMAL:-KX-ALL:+VKO-GOST-12:+GROUP-GOST-ALL:+CIPHER-GOST-ALL:+MAC-GOST-ALL:+SIGN-GOST-ALL:-VERS-ALL:+VERS-TLS1.2"
	},
	{
		.name = "TLS 1.2 VKO-GOST-12 with cred and multiple certs",
		.server_ret = 0,
		.client_ret = 0,
		.have_cert_cred = 1,
		.have_ecc_sign_cert = 1,
		.have_rsa_sign_cert = 1,
		.have_rsa_decrypt_cert = 1,
		.have_gost12_256_cert = 1,
		.have_gost12_512_cert = 1,
		.not_on_fips = 1,
		.server_prio = "NORMAL:-KX-ALL:+VKO-GOST-12:+GROUP-GOST-ALL:+CIPHER-GOST-ALL:+MAC-GOST-ALL:+SIGN-GOST-ALL:-VERS-ALL:+VERS-TLS1.2",
		.client_prio = "NORMAL:-KX-ALL:+VKO-GOST-12:+GROUP-GOST-ALL:+CIPHER-GOST-ALL:+MAC-GOST-ALL:+SIGN-GOST-ALL:-VERS-ALL:+VERS-TLS1.2"
	},
	{
		.name = "TLS 1.2 VKO-GOST-12 with cred and GOST12-256 cert client lacking signature algs (like SChannel)",
		.server_ret = 0,
		.client_ret = 0,
		.have_cert_cred = 1,
		.have_gost12_256_cert = 1,
		.not_on_fips = 1,
		.server_prio = "NORMAL:-KX-ALL:+VKO-GOST-12:+GROUP-GOST-ALL:+CIPHER-GOST-ALL:+MAC-GOST-ALL:+SIGN-GOST-ALL:-VERS-ALL:+VERS-TLS1.2",
		.client_prio = "NONE:+VKO-GOST-12:+GROUP-GOST-ALL:+CIPHER-GOST-ALL:+MAC-GOST-ALL:+VERS-TLS1.2:+SIGN-RSA-SHA256"
	},
	{
		.name = "TLS 1.2 VKO-GOST-12 with cred and GOST12-512 cert client lacking signature algs (like SChannel)",
		.server_ret = 0,
		.client_ret = 0,
		.have_cert_cred = 1,
		.have_gost12_512_cert = 1,
		.not_on_fips = 1,
		.server_prio = "NORMAL:-KX-ALL:+VKO-GOST-12:+GROUP-GOST-ALL:+CIPHER-GOST-ALL:+MAC-GOST-ALL:+SIGN-GOST-ALL:-VERS-ALL:+VERS-TLS1.2",
		.client_prio = "NONE:+VKO-GOST-12:+GROUP-GOST-ALL:+CIPHER-GOST-ALL:+MAC-GOST-ALL:+VERS-TLS1.2:+SIGN-RSA-SHA256"
	},
#endif
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
