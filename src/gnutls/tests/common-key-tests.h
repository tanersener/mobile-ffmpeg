/*
 * Copyright (C) 2017 Red Hat Inc.
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

#ifndef GNUTLS_TESTS_COMMON_KEY_TESTS_H
#define GNUTLS_TESTS_COMMON_KEY_TESTS_H

#include "cert-common.h"

#include <gnutls/abstract.h>

struct _key_tests_st {
	const char *name;
	gnutls_datum_t key;
	gnutls_datum_t cert;
	gnutls_pk_algorithm_t pk;
	unsigned data_only;
	unsigned digest;
	unsigned sigalgo;
	unsigned sign_flags;
};

static const
struct _key_tests_st common_key_tests[] = {
	{
		.name = "rsa key",
		.cert = {(void *) cli_ca3_cert_pem, sizeof(cli_ca3_cert_pem)-1},
		.key = {(void *) cli_ca3_key_pem, sizeof(cli_ca3_key_pem)-1},
		.pk = GNUTLS_PK_RSA,
		.digest = GNUTLS_DIG_SHA256,
		.sigalgo = GNUTLS_SIGN_RSA_SHA256
	},
	{
		.name = "dsa key",
		.key = {(void *) clidsa_ca3_key_pem, sizeof(clidsa_ca3_key_pem)-1},
		.cert = {(void *) clidsa_ca3_cert_pem, sizeof(clidsa_ca3_cert_pem)-1},
		.pk = GNUTLS_PK_DSA,
		.digest = GNUTLS_DIG_SHA1,
		.sigalgo = GNUTLS_SIGN_DSA_SHA1
	},
	{
		.name = "ecdsa key",
		.key = {(void *) server_ca3_ecc_key_pem, sizeof(server_ca3_ecc_key_pem)-1},
		.cert = {(void *) server_localhost_ca3_ecc_cert_pem, sizeof(server_localhost_ca3_ecc_cert_pem)-1},
		.pk = GNUTLS_PK_ECDSA,
		.digest = GNUTLS_DIG_SHA256,
		.sigalgo = GNUTLS_SIGN_ECDSA_SHA256
	},
	{
		.name = "ecdsa key",
		.key = {(void *) server_ca3_ecc_key_pem, sizeof(server_ca3_ecc_key_pem)-1},
		.cert = {(void *) server_localhost_ca3_ecc_cert_pem, sizeof(server_localhost_ca3_ecc_cert_pem)-1},
		.pk = GNUTLS_PK_ECDSA,
		.digest = GNUTLS_DIG_SHA256,
		.sigalgo = GNUTLS_SIGN_ECDSA_SECP256R1_SHA256
	},
	{
		.name = "rsa pss key",
		.key = {(void *) server_ca3_rsa_pss_key_pem, sizeof(server_ca3_rsa_pss_key_pem)-1},
		.cert = {(void *) server_ca3_rsa_pss_cert_pem, sizeof(server_ca3_rsa_pss_cert_pem)-1},
		.pk = GNUTLS_PK_RSA_PSS,
		.digest = GNUTLS_DIG_SHA256,
		.sign_flags = GNUTLS_PRIVKEY_SIGN_FLAG_RSA_PSS,
		.sigalgo = GNUTLS_SIGN_RSA_PSS_SHA256
	},
	{
		.name = "eddsa key",
		.key = {(void *) server_ca3_eddsa_key_pem, sizeof(server_ca3_eddsa_key_pem)-1},
		.cert = {(void *) server_ca3_eddsa_cert_pem, sizeof(server_ca3_eddsa_cert_pem)-1},
		.pk = GNUTLS_PK_EDDSA_ED25519,
		.digest = GNUTLS_DIG_SHA512,
		.sigalgo = GNUTLS_SIGN_EDDSA_ED25519,
		.data_only = 1
	}
};

#endif /* GNUTLS_TESTS_COMMON_KEY_TESTS_H */
