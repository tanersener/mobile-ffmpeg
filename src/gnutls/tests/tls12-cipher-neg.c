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

#include "cipher-neg-common.c"

test_case_st tests[] = {
	{
		.name = "server TLS 1.2: NULL (server)",
		.not_on_fips = 1,
		.cipher = GNUTLS_CIPHER_NULL,
		.server_prio = "NORMAL:-CIPHER-ALL:+NULL:+CIPHER-ALL:-VERS-ALL:+VERS-TLS1.2:%SERVER_PRECEDENCE",
		.client_prio = "NORMAL:+NULL",
		.desc = "(TLS1.2)-(ECDHE-SECP256R1)-(ECDSA-SHA256)-(NULL)-(SHA1)",
	},
	{
		.name = "client TLS 1.2: NULL (client)",
		.not_on_fips = 1,
		.cipher = GNUTLS_CIPHER_NULL,
		.server_prio = "NORMAL:+NULL",
		.client_prio = "NORMAL:-CIPHER-ALL:+NULL:+CIPHER-ALL:-VERS-ALL:+VERS-TLS1.2",
		.desc = "(TLS1.2)-(ECDHE-SECP256R1)-(ECDSA-SHA256)-(NULL)-(SHA1)",
	},
	{
		.name = "server TLS 1.2: AES-128-GCM (server)",
		.cipher = GNUTLS_CIPHER_AES_128_GCM,
		.server_prio = "NORMAL:-CIPHER-ALL:+AES-128-GCM:+CIPHER-ALL:-VERS-ALL:+VERS-TLS1.2:%SERVER_PRECEDENCE",
		.client_prio = "NORMAL:+AES-128-GCM",
		.desc = "(TLS1.2)-(ECDHE-SECP256R1)-(ECDSA-SHA256)-(AES-128-GCM)"
	},
	{
		.name = "both TLS 1.2: AES-128-GCM (server)",
		.cipher = GNUTLS_CIPHER_AES_128_GCM,
		.server_prio = "NORMAL:-CIPHER-ALL:+AES-128-GCM:+CIPHER-ALL:-VERS-ALL:+VERS-TLS1.2:%SERVER_PRECEDENCE",
		.client_prio = "NORMAL:+AES-128-GCM:+VERS-TLS1.2",
		.desc = "(TLS1.2)-(ECDHE-SECP256R1)-(ECDSA-SHA256)-(AES-128-GCM)"
	},
	{
		.name = "client TLS 1.2: AES-128-GCM (client)",
		.cipher = GNUTLS_CIPHER_AES_128_GCM,
		.server_prio = "NORMAL:+AES-128-GCM",
		.client_prio = "NORMAL:-CIPHER-ALL:+AES-128-GCM:+CIPHER-ALL:-VERS-ALL:+VERS-TLS1.2",
		.desc = "(TLS1.2)-(ECDHE-SECP256R1)-(ECDSA-SHA256)-(AES-128-GCM)"
	},
	{
		.name = "both TLS 1.2: AES-128-GCM (client)",
		.cipher = GNUTLS_CIPHER_AES_128_GCM,
		.server_prio = "NORMAL:+AES-128-GCM:+VERS-TLS1.2",
		.client_prio = "NORMAL:-CIPHER-ALL:+AES-128-GCM:+CIPHER-ALL:-VERS-ALL:+VERS-TLS1.2",
		.desc = "(TLS1.2)-(ECDHE-SECP256R1)-(ECDSA-SHA256)-(AES-128-GCM)"
	},
	{
		.name = "server TLS 1.2: AES-128-CCM (server)",
		.cipher = GNUTLS_CIPHER_AES_128_CCM,
		.server_prio = "NORMAL:-CIPHER-ALL:+AES-128-CCM:+CIPHER-ALL:-VERS-ALL:+VERS-TLS1.2:%SERVER_PRECEDENCE",
		.client_prio = "NORMAL:+AES-128-CCM",
		.desc = "(TLS1.2)-(ECDHE-SECP256R1)-(ECDSA-SHA256)-(AES-128-CCM)"
	},
	{
		.name = "both TLS 1.2: AES-128-CCM (server)",
		.cipher = GNUTLS_CIPHER_AES_128_CCM,
		.server_prio = "NORMAL:-CIPHER-ALL:+AES-128-CCM:+CIPHER-ALL:-VERS-ALL:+VERS-TLS1.2:%SERVER_PRECEDENCE",
		.client_prio = "NORMAL:+AES-128-CCM:+VERS-TLS1.2",
		.desc = "(TLS1.2)-(ECDHE-SECP256R1)-(ECDSA-SHA256)-(AES-128-CCM)"
	},
	{
		.name = "client TLS 1.2: AES-128-CCM (client)",
		.cipher = GNUTLS_CIPHER_AES_128_CCM,
		.server_prio = "NORMAL:+AES-128-CCM",
		.client_prio = "NORMAL:-CIPHER-ALL:+AES-128-CCM:+CIPHER-ALL:-VERS-ALL:+VERS-TLS1.2",
		.desc = "(TLS1.2)-(ECDHE-SECP256R1)-(ECDSA-SHA256)-(AES-128-CCM)"
	},
	{
		.name = "both TLS 1.2: AES-128-CCM (client)",
		.cipher = GNUTLS_CIPHER_AES_128_CCM,
		.server_prio = "NORMAL:+AES-128-CCM:+VERS-TLS1.2",
		.client_prio = "NORMAL:-CIPHER-ALL:+AES-128-CCM:+CIPHER-ALL:-VERS-ALL:+VERS-TLS1.2",
		.desc = "(TLS1.2)-(ECDHE-SECP256R1)-(ECDSA-SHA256)-(AES-128-CCM)"
	},
	{
		.name = "server TLS 1.2: CHACHA20-POLY (server)",
		.cipher = GNUTLS_CIPHER_CHACHA20_POLY1305,
		.not_on_fips = 1,
		.server_prio = "NORMAL:-CIPHER-ALL:+CHACHA20-POLY1305:+CIPHER-ALL:-VERS-ALL:+VERS-TLS1.2:%SERVER_PRECEDENCE",
		.client_prio = "NORMAL:+CHACHA20-POLY1305",
		.desc = "(TLS1.2)-(ECDHE-SECP256R1)-(ECDSA-SHA256)-(CHACHA20-POLY1305)"
	},
	{
		.name = "both TLS 1.2: CHACHA20-POLY (server)",
		.cipher = GNUTLS_CIPHER_CHACHA20_POLY1305,
		.not_on_fips = 1,
		.server_prio = "NORMAL:-CIPHER-ALL:+CHACHA20-POLY1305:+CIPHER-ALL:-VERS-ALL:+VERS-TLS1.2:%SERVER_PRECEDENCE",
		.client_prio = "NORMAL:+CHACHA20-POLY1305:+VERS-TLS1.2",
		.desc = "(TLS1.2)-(ECDHE-SECP256R1)-(ECDSA-SHA256)-(CHACHA20-POLY1305)"
	},
	{
		.name = "client TLS 1.2: CHACHA20-POLY (client)",
		.cipher = GNUTLS_CIPHER_CHACHA20_POLY1305,
		.not_on_fips = 1,
		.server_prio = "NORMAL:+CHACHA20-POLY1305",
		.client_prio = "NORMAL:-CIPHER-ALL:+CHACHA20-POLY1305:+CIPHER-ALL:-VERS-ALL:+VERS-TLS1.2",
		.desc = "(TLS1.2)-(ECDHE-SECP256R1)-(ECDSA-SHA256)-(CHACHA20-POLY1305)"
	},
	{
		.name = "both TLS 1.2: CHACHA20-POLY (client)",
		.cipher = GNUTLS_CIPHER_CHACHA20_POLY1305,
		.not_on_fips = 1,
		.server_prio = "NORMAL:+CHACHA20-POLY1305:+VERS-TLS1.2",
		.client_prio = "NORMAL:-CIPHER-ALL:+CHACHA20-POLY1305:+CIPHER-ALL:-VERS-ALL:+VERS-TLS1.2",
		.desc = "(TLS1.2)-(ECDHE-SECP256R1)-(ECDSA-SHA256)-(CHACHA20-POLY1305)"
	},
	{
		.name = "server TLS 1.2: AES-128-CBC (server)",
		.cipher = GNUTLS_CIPHER_AES_128_CBC,
		.server_prio = "NORMAL:-CIPHER-ALL:+AES-128-CBC:+CIPHER-ALL:-VERS-ALL:+VERS-TLS1.2:%SERVER_PRECEDENCE",
		.client_prio = "NORMAL:+AES-128-CBC",
		.desc = "(TLS1.2)-(ECDHE-SECP256R1)-(ECDSA-SHA256)-(AES-128-CBC)-(SHA1)"
	},
	{
		.name = "both TLS 1.2: AES-128-CBC (server)",
		.cipher = GNUTLS_CIPHER_AES_128_CBC,
		.server_prio = "NORMAL:-CIPHER-ALL:+AES-128-CBC:+CIPHER-ALL:-VERS-ALL:+VERS-TLS1.2:%SERVER_PRECEDENCE",
		.client_prio = "NORMAL:+AES-128-CBC:+VERS-TLS1.2",
		.desc = "(TLS1.2)-(ECDHE-SECP256R1)-(ECDSA-SHA256)-(AES-128-CBC)-(SHA1)"
	},
	{
		.name = "client TLS 1.2: AES-128-CBC (client)",
		.cipher = GNUTLS_CIPHER_AES_128_CBC,
		.server_prio = "NORMAL:+AES-128-CBC",
		.client_prio = "NORMAL:-CIPHER-ALL:+AES-128-CBC:+CIPHER-ALL:-VERS-ALL:+VERS-TLS1.2",
		.desc = "(TLS1.2)-(ECDHE-SECP256R1)-(ECDSA-SHA256)-(AES-128-CBC)-(SHA1)"
	},
	{
		.name = "both TLS 1.2: AES-128-CBC (client)",
		.cipher = GNUTLS_CIPHER_AES_128_CBC,
		.server_prio = "NORMAL:+AES-128-CBC:+VERS-TLS1.2",
		.client_prio = "NORMAL:-CIPHER-ALL:+AES-128-CBC:+CIPHER-ALL:-VERS-ALL:+VERS-TLS1.2",
		.desc = "(TLS1.2)-(ECDHE-SECP256R1)-(ECDSA-SHA256)-(AES-128-CBC)-(SHA1)"
	},
	{
		.name = "server TLS 1.2: 3DES-CBC (server)",
		.cipher = GNUTLS_CIPHER_3DES_CBC,
		.server_prio = "NORMAL:-CIPHER-ALL:+3DES-CBC:+CIPHER-ALL:-VERS-ALL:+VERS-TLS1.2:%SERVER_PRECEDENCE",
		.client_prio = "NORMAL:+3DES-CBC",
		.desc = "(TLS1.2)-(ECDHE-SECP256R1)-(ECDSA-SHA256)-(3DES-CBC)-(SHA1)"
	},
	{
		.name = "both TLS 1.2: 3DES-CBC (server)",
		.cipher = GNUTLS_CIPHER_3DES_CBC,
		.server_prio = "NORMAL:-CIPHER-ALL:+3DES-CBC:+CIPHER-ALL:-VERS-ALL:+VERS-TLS1.2:%SERVER_PRECEDENCE",
		.client_prio = "NORMAL:+3DES-CBC:+VERS-TLS1.2",
		.desc = "(TLS1.2)-(ECDHE-SECP256R1)-(ECDSA-SHA256)-(3DES-CBC)-(SHA1)"
	},
	{
		.name = "client TLS 1.2: 3DES-CBC (client)",
		.cipher = GNUTLS_CIPHER_3DES_CBC,
		.server_prio = "NORMAL:+3DES-CBC",
		.client_prio = "NORMAL:-CIPHER-ALL:+3DES-CBC:+CIPHER-ALL:-VERS-ALL:+VERS-TLS1.2",
		.desc = "(TLS1.2)-(ECDHE-SECP256R1)-(ECDSA-SHA256)-(3DES-CBC)-(SHA1)"
	},
	{
		.name = "both TLS 1.2: 3DES-CBC (client)",
		.cipher = GNUTLS_CIPHER_3DES_CBC,
		.server_prio = "NORMAL:+3DES-CBC:+VERS-TLS1.2",
		.client_prio = "NORMAL:-CIPHER-ALL:+3DES-CBC:+CIPHER-ALL:-VERS-ALL:+VERS-TLS1.2",
		.desc = "(TLS1.2)-(ECDHE-SECP256R1)-(ECDSA-SHA256)-(3DES-CBC)-(SHA1)"
	},
	{
		.name = "server TLS 1.2: ARCFOUR-128 (server)",
		.cipher = GNUTLS_CIPHER_ARCFOUR_128,
		.not_on_fips = 1,
		.server_prio = "NORMAL:-CIPHER-ALL:+ARCFOUR-128:+CIPHER-ALL:-VERS-ALL:+VERS-TLS1.2:%SERVER_PRECEDENCE",
		.client_prio = "NORMAL:+ARCFOUR-128",
		.desc = "(TLS1.2)-(ECDHE-SECP256R1)-(ECDSA-SHA256)-(ARCFOUR-128)-(SHA1)"
	},
	{
		.name = "both TLS 1.2: ARCFOUR-128 (server)",
		.cipher = GNUTLS_CIPHER_ARCFOUR_128,
		.not_on_fips = 1,
		.server_prio = "NORMAL:-CIPHER-ALL:+ARCFOUR-128:+CIPHER-ALL:-VERS-ALL:+VERS-TLS1.2:%SERVER_PRECEDENCE",
		.client_prio = "NORMAL:+ARCFOUR-128:+VERS-TLS1.2",
		.desc = "(TLS1.2)-(ECDHE-SECP256R1)-(ECDSA-SHA256)-(ARCFOUR-128)-(SHA1)"
	},
	{
		.name = "client TLS 1.2: ARCFOUR-128 (client)",
		.cipher = GNUTLS_CIPHER_ARCFOUR_128,
		.not_on_fips = 1,
		.server_prio = "NORMAL:+ARCFOUR-128",
		.client_prio = "NORMAL:-CIPHER-ALL:+ARCFOUR-128:+CIPHER-ALL:-VERS-ALL:+VERS-TLS1.2",
		.desc = "(TLS1.2)-(ECDHE-SECP256R1)-(ECDSA-SHA256)-(ARCFOUR-128)-(SHA1)"
	},
	{
		.name = "both TLS 1.2: ARCFOUR-128 (client)",
		.cipher = GNUTLS_CIPHER_ARCFOUR_128,
		.not_on_fips = 1,
		.server_prio = "NORMAL:+ARCFOUR-128:+VERS-TLS1.2",
		.client_prio = "NORMAL:-CIPHER-ALL:+ARCFOUR-128:+CIPHER-ALL:-VERS-ALL:+VERS-TLS1.2",
		.desc = "(TLS1.2)-(ECDHE-SECP256R1)-(ECDSA-SHA256)-(ARCFOUR-128)-(SHA1)"
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
