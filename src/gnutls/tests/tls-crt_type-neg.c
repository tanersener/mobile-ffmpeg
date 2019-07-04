/*
 * Copyright (C) 2017 - 2018 ARPA2 project
 *
 * Author: Tom Vrancken (dev@tomvrancken.nl)
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* This program tests the certificate type negotiation mechnism for
 * the handshake as specified in RFC7250 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <gnutls/gnutls.h>
#include "utils.h"
#include "cert-common.h"
#include "eagain-common.h"
#include "crt_type-neg-common.c"

test_case_st tests[] = {
	/* Tests with only a single credential set for client/server.
	 * Tests for X.509 cases.
	 */
		{
		/* Default case A
		 *
		 * Priority cli: NORMAL
		 * Priority srv: NORMAL
		 * Cli creds: None
		 * Srv creds: X.509
		 * Handshake: should complete without errors
		 * Negotiation: cert types should default to X.509
		 */
	 .name = "Default case A. Creds set (CLI/SRV): None/X509.",
	 .client_prio = "NORMAL",
	 .server_prio = "NORMAL",
	 .set_cli_creds = CRED_EMPTY,
	 .set_srv_creds = CRED_X509,
	 .expected_cli_ctype = GNUTLS_CRT_X509,
	 .expected_srv_ctype = GNUTLS_CRT_X509},
	{
		/* Default case B
		 *
		 * Priority: NORMAL
		 * Cli creds: X.509
		 * Srv creds: X.509
		 * Handshake: should complete without errors
		 * Negotiation: cert types should default to X.509
		 */
	 .name = "Default case B. Creds set (CLI/SRV): X509/X509. No cli cert asked.",
	 .client_prio = "NORMAL",
	 .server_prio = "NORMAL",
	 .set_cli_creds = CRED_X509,
	 .set_srv_creds = CRED_X509,
	 .expected_cli_ctype = GNUTLS_CRT_X509,
	 .expected_srv_ctype = GNUTLS_CRT_X509},
	{
		/* Default case C
		 *
		 * Priority: NORMAL
		 * Cli creds: X.509
		 * Srv creds: X.509
		 * Handshake: should complete without errors
		 * Negotiation: cert types should default to X.509
		 */
	 .name = "Default case C. Creds set (CLI/SRV): X509/X509. Cli cert asked.",
	 .client_prio = "NORMAL",
	 .server_prio = "NORMAL",
	 .set_cli_creds = CRED_X509,
	 .set_srv_creds = CRED_X509,
	 .expected_cli_ctype = GNUTLS_CRT_X509,
	 .expected_srv_ctype = GNUTLS_CRT_X509,
	 .request_cli_crt = true},
	{
		/* No server credentials
		 *
		 * Priority: NORMAL
		 * Cli creds: None
		 * Srv creds: None
		 * Handshake: results in errors
		 * Negotiation: cert types are not evaluated
		 */
	 .name = "No server creds. Creds set (CLI/SRV): None/None.",
	 .client_prio = "NORMAL",
	 .server_prio = "NORMAL",
	 .set_cli_creds = CRED_EMPTY,
	 .set_srv_creds = CRED_EMPTY,
	 .client_err = GNUTLS_E_AGAIN,
	 .server_err = GNUTLS_E_NO_CIPHER_SUITES},
	{
		/* Explicit cli/srv ctype negotiation, cli creds x509, srv creds x509
		 *
		 * Priority: NORMAL + request x509 for cli and srv
		 * Cli creds: X.509
		 * Srv creds: X.509
		 * Handshake: should complete without errors
		 * Negotiation: Fallback to default cli X.509, srv X.509 because
		 *   we advertise with only the cert type defaults. Extensions
		 *   will therefore not be activated.
		 */
	 .name = "Negotiate CLI X.509 + SRV X.509. Creds set (CLI/SRV): X.509/X.509.",
	 .client_prio = "NORMAL:+CTYPE-CLI-X509:+CTYPE-SRV-X509",
	 .server_prio = "NORMAL:+CTYPE-CLI-X509:+CTYPE-SRV-X509",
	 .set_cli_creds = CRED_X509,
	 .set_srv_creds = CRED_X509,
	 .expected_cli_ctype = GNUTLS_CRT_X509,
	 .expected_srv_ctype = GNUTLS_CRT_X509},
	{
		/* Explicit cli/srv ctype negotiation, cli creds x509, srv creds x509, no cli cert asked
		 *
		 * Priority: NORMAL + request x509 for cli
		 * Cli creds: X.509
		 * Srv creds: X.509
		 * Handshake: should complete without errors
		 * Negotiation: Fallback to default cli X.509, srv X.509 because
		 *   we advertise with only the cert type defaults. Extensions
		 *   will therefore not be activated.
		 */
	 .name = "Negotiate CLI X.509. Creds set (CLI/SRV): X.509/X.509.",
	 .client_prio = "NORMAL:+CTYPE-CLI-X509",
	 .server_prio = "NORMAL:+CTYPE-CLI-X509",
	 .set_cli_creds = CRED_X509,
	 .set_srv_creds = CRED_X509,
	 .expected_cli_ctype = GNUTLS_CRT_X509,
	 .expected_srv_ctype = GNUTLS_CRT_X509},
	{
		/* Explicit cli/srv ctype negotiation, cli creds x509, srv creds x509, cli cert asked
		 *
		 * Priority: NORMAL + request x509 for cli
		 * Cli creds: X.509
		 * Srv creds: X.509
		 * Handshake: should complete without errors
		 * Negotiation: Fallback to default cli X.509, srv X.509 because
		 *   we advertise with only the cert type defaults. Extensions
		 *   will therefore not be activated.
		 */
	 .name = "Negotiate CLI X.509. Creds set (CLI/SRV): X.509/X.509.",
	 .client_prio = "NORMAL:+CTYPE-CLI-X509",
	 .server_prio = "NORMAL:+CTYPE-CLI-X509",
	 .set_cli_creds = CRED_X509,
	 .set_srv_creds = CRED_X509,
	 .expected_cli_ctype = GNUTLS_CRT_X509,
	 .expected_srv_ctype = GNUTLS_CRT_X509,
	 .request_cli_crt = true},
	{
		/* Explicit cli/srv ctype negotiation, cli creds x509, srv creds x509
		 *
		 * Priority: NORMAL + request x509 for srv
		 * Cli creds: X.509
		 * Srv creds: X.509
		 * Handshake: should complete without errors
		 * Negotiation: Fallback to default cli X.509, srv X.509 because
		 *   we advertise with only the cert type defaults. Extensions
		 *   will therefore not be activated.
		 */
	 .name = "Negotiate SRV X.509. Creds set (CLI/SRV): X.509/X.509.",
	 .client_prio = "NORMAL:+CTYPE-SRV-X509",
	 .server_prio = "NORMAL:+CTYPE-SRV-X509",
	 .set_cli_creds = CRED_X509,
	 .set_srv_creds = CRED_X509,
	 .expected_cli_ctype = GNUTLS_CRT_X509,
	 .expected_srv_ctype = GNUTLS_CRT_X509},
	{
		/* Explicit cli/srv ctype negotiation, all types allowed for CLI, cli creds x509, srv creds x509
		 *
		 * Priority: NORMAL + allow all client cert types
		 * Cli creds: X.509
		 * Srv creds: X.509
		 * Handshake: should complete without errors
		 * Negotiation: cli X.509 and srv X.509 because
		 *   we only have X.509 credentials set.
		 */
	 .name = "Negotiate CLI all. Creds set (CLI/SRV): X.509/X.509.",
	 .client_prio = "NORMAL:+CTYPE-CLI-ALL",
	 .server_prio = "NORMAL:+CTYPE-CLI-ALL",
	 .set_cli_creds = CRED_X509,
	 .set_srv_creds = CRED_X509,
	 .expected_cli_ctype = GNUTLS_CRT_X509,
	 .expected_srv_ctype = GNUTLS_CRT_X509},
	{
		/* Explicit cli/srv ctype negotiation, all types allowed for SRV, cli creds x509, srv creds x509
		 *
		 * Priority: NORMAL + allow all server cert types
		 * Cli creds: X.509
		 * Srv creds: X.509
		 * Handshake: should complete without errors
		 * Negotiation: cli X.509 and srv X.509 because
		 *   we only have X.509 credentials set.
		 */
	 .name = "Negotiate SRV all. Creds set (CLI/SRV): X.509/X.509.",
	 .client_prio = "NORMAL:+CTYPE-SRV-ALL",
	 .server_prio = "NORMAL:+CTYPE-SRV-ALL",
	 .set_cli_creds = CRED_X509,
	 .set_srv_creds = CRED_X509,
	 .expected_cli_ctype = GNUTLS_CRT_X509,
	 .expected_srv_ctype = GNUTLS_CRT_X509},
	{
		/* Explicit cli/srv ctype negotiation, all types allowed for CLI/SRV, cli creds x509, srv creds x509
		 *
		 * Priority: NORMAL + allow all client and server cert types
		 * Cli creds: X.509
		 * Srv creds: X.509
		 * Handshake: should complete without errors
		 * Negotiation: cli X.509 and srv X.509 because
		 *   we only have X.509 credentials set.
		 */
	 .name = "Negotiate CLI/SRV all. Creds set (CLI/SRV): X.509/X.509.",
	 .client_prio = "NORMAL:+CTYPE-CLI-ALL:+CTYPE-SRV-ALL",
	 .server_prio = "NORMAL:+CTYPE-CLI-ALL:+CTYPE-SRV-ALL",
	 .set_cli_creds = CRED_X509,
	 .set_srv_creds = CRED_X509,
	 .expected_cli_ctype = GNUTLS_CRT_X509,
	 .expected_srv_ctype = GNUTLS_CRT_X509},

	/* Tests with only a single credential set for client/server.
	 * Tests for Raw public-key cases.
	 */
	{
		/* Explicit cli/srv ctype negotiation, cli creds Raw PK, srv creds Raw PK, Req. cli cert.
		 *
		 * Priority: NORMAL + request rawpk for cli and srv
		 * Cli creds: Raw PK
		 * Srv creds: Raw PK
		 * Request client cert: yes
		 * Handshake: should complete without errors
		 * Negotiation: both parties should have a Raw PK cert negotiated
		 */
	 .name = "Negotiate CLI Raw PK + SRV Raw PK. Creds set (CLI/SRV): RawPK/RawPK. Cert req.",
	 .client_prio = "NORMAL:+CTYPE-CLI-RAWPK:+CTYPE-SRV-RAWPK",
	 .server_prio = "NORMAL:+CTYPE-CLI-RAWPK:+CTYPE-SRV-RAWPK",
	 .set_cli_creds = CRED_RAWPK,
	 .set_srv_creds = CRED_RAWPK,
	 .expected_cli_ctype = GNUTLS_CRT_RAWPK,
	 .expected_srv_ctype = GNUTLS_CRT_RAWPK,
	 .init_flags_cli = GNUTLS_ENABLE_RAWPK,
	 .init_flags_srv = GNUTLS_ENABLE_RAWPK,
	 .request_cli_crt = true},
	{
		/* Explicit cli/srv ctype negotiation (TLS 1.2), cli creds Raw PK, srv creds Raw PK
		 *
		 * Priority: NORMAL + request rawpk for cli and srv
		 * Cli creds: Raw PK
		 * Srv creds: Raw PK
		 * Request client cert: no
		 * Handshake: should complete without errors
		 * Negotiation: a Raw PK server cert. A diverged state for the client
		 *   cert type. The server picks Raw PK but does not send a response
		 *   to the client (under TLS 1.2). The client therefore falls back to default (X.509).
		 */
	 .name = "Negotiate CLI Raw PK + SRV Raw PK. Creds set (CLI/SRV): RawPK/RawPK.",
	 .client_prio = "NORMAL:-VERS-ALL:+VERS-TLS1.2:+CTYPE-CLI-RAWPK:+CTYPE-SRV-RAWPK",
	 .server_prio = "NORMAL:-VERS-ALL:+VERS-TLS1.2:+CTYPE-CLI-RAWPK:+CTYPE-SRV-RAWPK",
	 .set_cli_creds = CRED_RAWPK,
	 .set_srv_creds = CRED_RAWPK,
	 .expected_cli_cli_ctype = GNUTLS_CRT_X509,
	 .expected_srv_cli_ctype = GNUTLS_CRT_RAWPK,
	 .expected_cli_srv_ctype = GNUTLS_CRT_RAWPK,
	 .expected_srv_srv_ctype = GNUTLS_CRT_RAWPK,
	 .init_flags_cli = GNUTLS_ENABLE_RAWPK,
	 .init_flags_srv = GNUTLS_ENABLE_RAWPK,
	 .request_cli_crt = false,
	 .cli_srv_may_diverge = true},
	{
		/* Explicit cli/srv ctype negotiation (TLS 1.3), cli creds Raw PK, srv creds Raw PK
		 *
		 * Priority: NORMAL + request rawpk for cli and srv
		 * Cli creds: Raw PK
		 * Srv creds: Raw PK
		 * Request client cert: no
		 * Handshake: should complete without errors
		 * Negotiation: a Raw PK server cert and client cert. Under TLS 1.3
		 *   a respons is always sent by the server also when no client
		 *   cert is requested. This is necessary for post-handshake authentication
		 *   to work.
		 */
	 .name = "Negotiate CLI Raw PK + SRV Raw PK. Creds set (CLI/SRV): RawPK/RawPK.",
	 .client_prio = "NORMAL:-VERS-ALL:+VERS-TLS1.3:+CTYPE-CLI-RAWPK:+CTYPE-SRV-RAWPK",
	 .server_prio = "NORMAL:-VERS-ALL:+VERS-TLS1.3:+CTYPE-CLI-RAWPK:+CTYPE-SRV-RAWPK",
	 .set_cli_creds = CRED_RAWPK,
	 .set_srv_creds = CRED_RAWPK,
	 .expected_cli_cli_ctype = GNUTLS_CRT_RAWPK,
	 .expected_srv_cli_ctype = GNUTLS_CRT_RAWPK,
	 .expected_cli_srv_ctype = GNUTLS_CRT_RAWPK,
	 .expected_srv_srv_ctype = GNUTLS_CRT_RAWPK,
	 .init_flags_cli = GNUTLS_ENABLE_RAWPK,
	 .init_flags_srv = GNUTLS_ENABLE_RAWPK,
	 .request_cli_crt = false,
	 .cli_srv_may_diverge = true},
	{
		/* Explicit cli/srv ctype negotiation, cli creds Raw PK, srv creds Raw PK
		 *
		 * Priority: NORMAL + request rawpk for cli
		 * Cli creds: Raw PK
		 * Srv creds: Raw PK
		 * Request client cert: no
		 * Handshake: fails because no valid cred (X.509) can be found for the server.
		 * Negotiation: -
		 */
	 .name = "Negotiate CLI Raw PK. Creds set (CLI/SRV): RawPK/RawPK.",
	 .client_prio = "NORMAL:+CTYPE-CLI-RAWPK",
	 .server_prio = "NORMAL:+CTYPE-CLI-RAWPK",
	 .set_cli_creds = CRED_RAWPK,
	 .set_srv_creds = CRED_RAWPK,
	 .init_flags_cli = GNUTLS_ENABLE_RAWPK,
	 .init_flags_srv = GNUTLS_ENABLE_RAWPK,
	 .client_err = GNUTLS_E_AGAIN,
	 .server_err = GNUTLS_E_NO_CIPHER_SUITES},
	{
		/* Explicit cli/srv ctype negotiation, cli creds Raw PK, srv creds Raw PK, request cli cert.
		 *
		 * Priority: NORMAL + request rawpk for srv
		 * Cli creds: Raw PK
		 * Srv creds: Raw PK
		 * Request client cert: yes
		 * Handshake: should complete without errors
		 * Negotiation: Raw PK will be negotiated for server. Client will
		 *   default to X.509.
		 */
	 .name = "Negotiate SRV Raw PK. Creds set (CLI/SRV): RawPK/RawPK.",
	 .client_prio = "NORMAL:+CTYPE-SRV-RAWPK",
	 .server_prio = "NORMAL:+CTYPE-SRV-RAWPK",
	 .set_cli_creds = CRED_RAWPK,
	 .set_srv_creds = CRED_RAWPK,
	 .expected_cli_ctype = GNUTLS_CRT_X509,
	 .expected_srv_ctype = GNUTLS_CRT_RAWPK,
	 .init_flags_cli = GNUTLS_ENABLE_RAWPK,
	 .init_flags_srv = GNUTLS_ENABLE_RAWPK,
	 .request_cli_crt = true},
	 {
		/* Explicit cli/srv ctype negotiation, cli creds Raw PK, srv creds X.509, Request cli cert.
		 *
		 * Priority: NORMAL + request rawpk for cli and srv
		 * Cli creds: Raw PK
		 * Srv creds: X.509
		 * Request client cert: yes
		 * Handshake: should complete without errors
		 * Negotiation: Raw PK will be negotiated for client. Server will
		 *   default to X.509.
		 */
	 .name = "Negotiate CLI and SRV Raw PK. Creds set (CLI/SRV): RawPK/X.509.",
	 .client_prio = "NORMAL:+CTYPE-CLI-RAWPK:+CTYPE-SRV-RAWPK",
	 .server_prio = "NORMAL:+CTYPE-CLI-RAWPK:+CTYPE-SRV-RAWPK",
	 .set_cli_creds = CRED_RAWPK,
	 .set_srv_creds = CRED_X509,
	 .expected_cli_ctype = GNUTLS_CRT_RAWPK,
	 .expected_srv_ctype = GNUTLS_CRT_X509,
	 .init_flags_cli = GNUTLS_ENABLE_RAWPK,
	 .init_flags_srv = GNUTLS_ENABLE_RAWPK,
	 .request_cli_crt = true},
	 {
		/* All types allowed for CLI, cli creds Raw PK, srv creds X.509
		 *
		 * Priority: NORMAL + allow all client cert types
		 * Cli creds: Raw PK
		 * Srv creds: X.509
		 * Handshake: should complete without errors
		 * Negotiation: cli Raw PK and srv X.509 because
		 *   that are the only credentials set.
		 */
	 .name = "Negotiate CLI all. Creds set (CLI/SRV): Raw PK/X.509.",
	 .client_prio = "NORMAL:+CTYPE-CLI-ALL",
	 .server_prio = "NORMAL:+CTYPE-CLI-ALL",
	 .set_cli_creds = CRED_RAWPK,
	 .set_srv_creds = CRED_X509,
	 .expected_cli_ctype = GNUTLS_CRT_RAWPK,
	 .expected_srv_ctype = GNUTLS_CRT_X509,
	 .init_flags_cli = GNUTLS_ENABLE_RAWPK,
	 .init_flags_srv = GNUTLS_ENABLE_RAWPK,
	 .request_cli_crt = true},
	{
		/* All types allowed for SRV, cli creds x509, srv creds Raw PK
		 *
		 * Priority: NORMAL + allow all server cert types
		 * Cli creds: X.509
		 * Srv creds: Raw PK
		 * Handshake: should complete without errors
		 * Negotiation: cli X.509 and srv Raw PK because
		 *   that are the only credentials set.
		 */
	 .name = "Negotiate SRV all. Creds set (CLI/SRV): X.509/Raw PK.",
	 .client_prio = "NORMAL:+CTYPE-SRV-ALL",
	 .server_prio = "NORMAL:+CTYPE-SRV-ALL",
	 .set_cli_creds = CRED_X509,
	 .set_srv_creds = CRED_RAWPK,
	 .expected_cli_ctype = GNUTLS_CRT_X509,
	 .expected_srv_ctype = GNUTLS_CRT_RAWPK,
	 .init_flags_cli = GNUTLS_ENABLE_RAWPK,
	 .init_flags_srv = GNUTLS_ENABLE_RAWPK,
	 .request_cli_crt = true},
	{
		/* All types allowed for CLI/SRV, cli creds Raw PK, srv creds Raw PK
		 *
		 * Priority: NORMAL + allow all client and server cert types
		 * Cli creds: Raw PK
		 * Srv creds: Raw PK
		 * Handshake: should complete without errors
		 * Negotiation: cli Raw PK and srv Raw PK because
		 *   that are the only credentials set.
		 */
	 .name = "Negotiate CLI/SRV all. Creds set (CLI/SRV): Raw PK/Raw PK.",
	 .client_prio = "NORMAL:+CTYPE-CLI-ALL:+CTYPE-SRV-ALL",
	 .server_prio = "NORMAL:+CTYPE-CLI-ALL:+CTYPE-SRV-ALL",
	 .set_cli_creds = CRED_RAWPK,
	 .set_srv_creds = CRED_RAWPK,
	 .expected_cli_ctype = GNUTLS_CRT_RAWPK,
	 .expected_srv_ctype = GNUTLS_CRT_RAWPK,
	 .init_flags_cli = GNUTLS_ENABLE_RAWPK,
	 .init_flags_srv = GNUTLS_ENABLE_RAWPK,
	 .request_cli_crt = true},

};

void doit(void)
{
	unsigned i;
	global_init();

	for (i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
		try(&tests[i]);
	}

	gnutls_global_deinit();
}
