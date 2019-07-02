/*
 * Copyright (C) 2015-2018 Red Hat, Inc.
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* This program tests the various certificate key exchange methods supported
 * in gnutls */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <gnutls/gnutls.h>
#include "utils.h"
#include "common-cert-key-exchange.h"
#include "cert-common.h"

void doit(void)
{
	global_init();

	/** X.509 tests **/
	try_x509("TLS 1.2 with anon-ecdh", "NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+ANON-ECDH", GNUTLS_KX_ANON_ECDH, GNUTLS_SIGN_UNKNOWN, GNUTLS_SIGN_UNKNOWN);
	try_x509("TLS 1.2 with anon-dh", "NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+ANON-DH", GNUTLS_KX_ANON_DH, GNUTLS_SIGN_UNKNOWN, GNUTLS_SIGN_UNKNOWN);
	try_x509("TLS 1.2 with dhe-rsa no-cli-cert (ctype X.509)", "NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+DHE-RSA", GNUTLS_KX_DHE_RSA, GNUTLS_SIGN_RSA_SHA256, GNUTLS_SIGN_UNKNOWN);
	try_x509("TLS 1.2 with ecdhe x25519 rsa no-cli-cert (ctype X.509)", "NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+ECDHE-RSA:-CURVE-ALL:+CURVE-X25519", GNUTLS_KX_ECDHE_RSA, GNUTLS_SIGN_RSA_SHA256, GNUTLS_SIGN_UNKNOWN);
	try_x509("TLS 1.2 with ecdhe rsa no-cli-cert (ctype X.509)", "NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+ECDHE-RSA", GNUTLS_KX_ECDHE_RSA, GNUTLS_SIGN_RSA_SHA256, GNUTLS_SIGN_UNKNOWN);
	try_with_key("TLS 1.2 with ecdhe ecdsa no-cli-cert (ctype X.509)", "NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+ECDHE-ECDSA", GNUTLS_KX_ECDHE_ECDSA, GNUTLS_SIGN_ECDSA_SHA256, GNUTLS_SIGN_UNKNOWN,
		&server_ca3_localhost_ecc_cert, &server_ca3_ecc_key, NULL, NULL, 0, GNUTLS_CRT_X509, GNUTLS_CRT_UNKNOWN);
	try_x509("TLS 1.2 with ecdhe rsa-pss sig no-cli-cert (ctype X.509)", "NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+ECDHE-RSA:-SIGN-ALL:+SIGN-RSA-PSS-RSAE-SHA256", GNUTLS_KX_ECDHE_RSA, GNUTLS_SIGN_RSA_PSS_RSAE_SHA256, GNUTLS_SIGN_UNKNOWN);

	/* Test RSA-PSS cert/key combo issues */
	try_with_key("TLS 1.2 with ecdhe with rsa-pss-sha256 key no-cli-cert (ctype X.509)", "NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+ECDHE-RSA", GNUTLS_KX_ECDHE_RSA, GNUTLS_SIGN_RSA_PSS_SHA256, GNUTLS_SIGN_UNKNOWN,
		&server_ca3_rsa_pss2_cert, &server_ca3_rsa_pss2_key, NULL, NULL, 0, GNUTLS_CRT_X509, GNUTLS_CRT_UNKNOWN);
	try_with_key("TLS 1.2 with ecdhe with rsa-pss-sha256 key and 1 sig no-cli-cert (ctype X.509)", "NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+ECDHE-RSA:-SIGN-ALL:+SIGN-RSA-PSS-SHA256", GNUTLS_KX_ECDHE_RSA, GNUTLS_SIGN_RSA_PSS_SHA256, GNUTLS_SIGN_UNKNOWN,
		&server_ca3_rsa_pss2_cert, &server_ca3_rsa_pss2_key, NULL, NULL, 0, GNUTLS_CRT_X509, GNUTLS_CRT_UNKNOWN);
	try_with_key("TLS 1.2 with ecdhe with rsa-pss-sha256 key and rsa-pss-sha384 first sig no-cli-cert (ctype X.509)", "NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+ECDHE-RSA:-SIGN-ALL:+SIGN-RSA-PSS-SHA384:+SIGN-RSA-PSS-SHA256", GNUTLS_KX_ECDHE_RSA, GNUTLS_SIGN_RSA_PSS_SHA256, GNUTLS_SIGN_UNKNOWN,
		&server_ca3_rsa_pss2_cert, &server_ca3_rsa_pss2_key, NULL, NULL, 0, GNUTLS_CRT_X509, GNUTLS_CRT_UNKNOWN);
	try_with_key("TLS 1.2 with ecdhe with rsa-pss-sha256 key and rsa-pss-sha512 first sig no-cli-cert (ctype X.509)", "NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+ECDHE-RSA:-SIGN-ALL:+SIGN-RSA-PSS-SHA512:+SIGN-RSA-PSS-SHA384:+SIGN-RSA-PSS-SHA256", GNUTLS_KX_ECDHE_RSA, GNUTLS_SIGN_RSA_PSS_SHA256, GNUTLS_SIGN_UNKNOWN,
		&server_ca3_rsa_pss2_cert, &server_ca3_rsa_pss2_key, NULL, NULL, 0, GNUTLS_CRT_X509, GNUTLS_CRT_UNKNOWN);

	try_x509("TLS 1.2 with ecdhe rsa-pss no-cli-cert (ctype X.509)", "NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+ECDHE-RSA:-SIGN-ALL:+SIGN-RSA-PSS-RSAE-SHA256", GNUTLS_KX_ECDHE_RSA, GNUTLS_SIGN_RSA_PSS_RSAE_SHA256, GNUTLS_SIGN_UNKNOWN);
	try_with_key("TLS 1.2 with ecdhe rsa-pss/rsa-pss no-cli-cert (ctype X.509)", "NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+ECDHE-RSA:-SIGN-ALL:+SIGN-RSA-PSS-SHA256", GNUTLS_KX_ECDHE_RSA, GNUTLS_SIGN_RSA_PSS_SHA256, GNUTLS_SIGN_UNKNOWN,
		&server_ca3_rsa_pss_cert, &server_ca3_rsa_pss_key, NULL, NULL, 0, GNUTLS_CRT_X509, GNUTLS_CRT_UNKNOWN);
	try_x509("TLS 1.2 with rsa no-cli-cert (ctype X.509)", "NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+RSA", GNUTLS_KX_RSA, GNUTLS_SIGN_UNKNOWN, GNUTLS_SIGN_UNKNOWN);
	try_with_key("TLS 1.2 with ecdhe x25519 ed25519 no-cli-cert (ctype X.509)", "NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+ECDHE-ECDSA:-CURVE-ALL:+CURVE-X25519:-SIGN-ALL:+SIGN-EDDSA-ED25519", GNUTLS_KX_ECDHE_ECDSA, GNUTLS_SIGN_EDDSA_ED25519, GNUTLS_SIGN_UNKNOWN,
		&server_ca3_eddsa_cert, &server_ca3_eddsa_key, NULL, NULL, 0, GNUTLS_CRT_X509, GNUTLS_CRT_UNKNOWN);

	try_x509_cli("TLS 1.2 with dhe-rsa cli-cert (ctype X.509)", "NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+DHE-RSA", GNUTLS_KX_DHE_RSA, GNUTLS_SIGN_RSA_SHA256, GNUTLS_SIGN_RSA_SHA256, USE_CERT);
	try_x509_cli("TLS 1.2 with ecdhe-rsa cli-cert (ctype X.509)", "NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+ECDHE-RSA", GNUTLS_KX_ECDHE_RSA, GNUTLS_SIGN_RSA_SHA256, GNUTLS_SIGN_RSA_SHA256, USE_CERT);
	try_x509_cli("TLS 1.2 with rsa cli-cert (ctype X.509)", "NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+RSA", GNUTLS_KX_RSA, GNUTLS_SIGN_UNKNOWN, GNUTLS_SIGN_RSA_SHA256, USE_CERT);
	try_with_key("TLS 1.2 with ecdhe ecdsa cli-cert (ctype X.509)", "NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+ECDHE-ECDSA", GNUTLS_KX_ECDHE_ECDSA, GNUTLS_SIGN_ECDSA_SHA256, GNUTLS_SIGN_RSA_SHA256,
		&server_ca3_localhost_ecc_cert, &server_ca3_ecc_key, &cli_ca3_cert, &cli_ca3_key, USE_CERT, GNUTLS_CRT_X509, GNUTLS_CRT_X509);
	try_x509_cli("TLS 1.2 with ecdhe-rsa-pss cli-cert (ctype X.509)", "NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+ECDHE-RSA:-SIGN-ALL:+SIGN-RSA-PSS-RSAE-SHA256", GNUTLS_KX_ECDHE_RSA, GNUTLS_SIGN_RSA_PSS_RSAE_SHA256, GNUTLS_SIGN_RSA_PSS_RSAE_SHA256, USE_CERT);
	try_with_key("TLS 1.2 with ecdhe-rsa-pss/rsa-pss cli-cert (ctype X.509)", "NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+ECDHE-RSA:-SIGN-ALL:+SIGN-RSA-PSS-SHA256", GNUTLS_KX_ECDHE_RSA, GNUTLS_SIGN_RSA_PSS_SHA256, GNUTLS_SIGN_RSA_PSS_SHA256,
		&server_ca3_rsa_pss_cert, &server_ca3_rsa_pss_key, &cli_ca3_rsa_pss_cert, &cli_ca3_rsa_pss_key, USE_CERT, GNUTLS_CRT_X509, GNUTLS_CRT_X509);

	try_with_key("TLS 1.2 with ecdhe x25519 ed25519 cli-cert (ctype X.509)", "NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+ECDHE-ECDSA:-CURVE-ALL:+CURVE-X25519:-SIGN-ALL:+SIGN-EDDSA-ED25519", GNUTLS_KX_ECDHE_ECDSA, GNUTLS_SIGN_EDDSA_ED25519, GNUTLS_SIGN_EDDSA_ED25519,
		&server_ca3_eddsa_cert, &server_ca3_eddsa_key, &server_ca3_eddsa_cert, &server_ca3_eddsa_key, USE_CERT, GNUTLS_CRT_X509, GNUTLS_CRT_X509);

	try_x509_cli("TLS 1.2 with dhe-rsa ask cli-cert (ctype X.509)", "NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+DHE-RSA", GNUTLS_KX_DHE_RSA, GNUTLS_SIGN_RSA_SHA256, GNUTLS_SIGN_UNKNOWN, ASK_CERT);
	try_x509_cli("TLS 1.2 with ecdhe-rsa ask cli-cert (ctype X.509)", "NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+ECDHE-RSA", GNUTLS_KX_ECDHE_RSA, GNUTLS_SIGN_RSA_SHA256, GNUTLS_SIGN_UNKNOWN, ASK_CERT);
	try_x509_cli("TLS 1.2 with rsa ask cli-cert (ctype X.509)", "NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+RSA", GNUTLS_KX_RSA, GNUTLS_SIGN_UNKNOWN, GNUTLS_SIGN_UNKNOWN, ASK_CERT);
	try_with_key("TLS 1.2 with ecdhe ecdsa cli-cert (ctype X.509)", "NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+ECDHE-ECDSA", GNUTLS_KX_ECDHE_ECDSA, GNUTLS_SIGN_ECDSA_SHA256, GNUTLS_SIGN_UNKNOWN,
		&server_ca3_localhost_ecc_cert, &server_ca3_ecc_key, &cli_ca3_cert, &cli_ca3_key, ASK_CERT, GNUTLS_CRT_X509, GNUTLS_CRT_X509);

	/** Raw public-key tests **/
	try_rawpk("TLS 1.2 with dhe-rsa no-cli-cert (ctype Raw PK)", "NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+DHE-RSA:+CTYPE-ALL", GNUTLS_KX_DHE_RSA, GNUTLS_SIGN_RSA_SHA256, GNUTLS_SIGN_UNKNOWN);
	try_rawpk("TLS 1.2 with ecdhe x25519 rsa no-cli-cert (ctype Raw PK)", "NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+ECDHE-RSA:-CURVE-ALL:+CURVE-X25519:+CTYPE-ALL", GNUTLS_KX_ECDHE_RSA, GNUTLS_SIGN_RSA_SHA256, GNUTLS_SIGN_UNKNOWN);
	try_rawpk("TLS 1.2 with ecdhe rsa no-cli-cert (ctype Raw PK)", "NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+ECDHE-RSA:+CTYPE-ALL", GNUTLS_KX_ECDHE_RSA, GNUTLS_SIGN_RSA_SHA256, GNUTLS_SIGN_UNKNOWN);
	try_rawpk("TLS 1.2 with ecdhe rsa-pss sig no-cli-cert (ctype Raw PK)", "NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+ECDHE-RSA:-SIGN-ALL:+SIGN-RSA-PSS-RSAE-SHA256:+CTYPE-ALL", GNUTLS_KX_ECDHE_RSA, GNUTLS_SIGN_RSA_PSS_RSAE_SHA256, GNUTLS_SIGN_UNKNOWN);
	try_rawpk("TLS 1.2 with ecdhe rsa-pss no-cli-cert (ctype Raw PK)", "NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+ECDHE-RSA:-SIGN-ALL:+SIGN-RSA-PSS-RSAE-SHA256:+CTYPE-ALL", GNUTLS_KX_ECDHE_RSA, GNUTLS_SIGN_RSA_PSS_RSAE_SHA256, GNUTLS_SIGN_UNKNOWN);
	try_rawpk("TLS 1.2 with rsa no-cli-cert (ctype Raw PK)", "NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+RSA:+CTYPE-ALL", GNUTLS_KX_RSA, GNUTLS_SIGN_UNKNOWN, GNUTLS_SIGN_UNKNOWN);

	try_rawpk_cli("TLS 1.2 with dhe-rsa cli-cert (ctype Raw PK)", "NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+DHE-RSA:+CTYPE-ALL", GNUTLS_KX_DHE_RSA, GNUTLS_SIGN_RSA_SHA256, GNUTLS_SIGN_RSA_SHA256, USE_CERT);
	try_rawpk_cli("TLS 1.2 with ecdhe-rsa cli-cert (ctype Raw PK)", "NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+ECDHE-RSA:+CTYPE-ALL", GNUTLS_KX_ECDHE_RSA, GNUTLS_SIGN_RSA_SHA256, GNUTLS_SIGN_RSA_SHA256, USE_CERT);
	try_rawpk_cli("TLS 1.2 with rsa cli-cert (ctype Raw PK)", "NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+RSA:+CTYPE-ALL", GNUTLS_KX_RSA, GNUTLS_SIGN_UNKNOWN, GNUTLS_SIGN_RSA_SHA256, USE_CERT);
	try_rawpk_cli("TLS 1.2 with ecdhe-rsa-pss cli-cert (ctype Raw PK)", "NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+ECDHE-RSA:-SIGN-ALL:+SIGN-RSA-PSS-RSAE-SHA256:+CTYPE-ALL", GNUTLS_KX_ECDHE_RSA, GNUTLS_SIGN_RSA_PSS_RSAE_SHA256, GNUTLS_SIGN_RSA_PSS_RSAE_SHA256, USE_CERT);
	try_rawpk_cli("TLS 1.2 with dhe-rsa ask cli-cert (ctype Raw PK)", "NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+DHE-RSA:+CTYPE-ALL", GNUTLS_KX_DHE_RSA, GNUTLS_SIGN_RSA_SHA256, GNUTLS_SIGN_UNKNOWN, ASK_CERT);
	try_rawpk_cli("TLS 1.2 with ecdhe-rsa ask cli-cert (ctype Raw PK)", "NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+ECDHE-RSA:+CTYPE-ALL", GNUTLS_KX_ECDHE_RSA, GNUTLS_SIGN_RSA_SHA256, GNUTLS_SIGN_UNKNOWN, ASK_CERT);
	try_rawpk_cli("TLS 1.2 with rsa ask cli-cert (ctype Raw PK)", "NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+RSA:+CTYPE-ALL", GNUTLS_KX_RSA, GNUTLS_SIGN_UNKNOWN, GNUTLS_SIGN_UNKNOWN, ASK_CERT);


	/** Illegal setups **/
	server_priority = "NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+ECDHE-RSA";
	try_with_key_fail("TLS 1.2 with rsa cert and only RSA-PSS sig algos in client",
			"NORMAL:-VERS-ALL:+VERS-TLS1.2:-SIGN-ALL:+SIGN-RSA-PSS-SHA256:+SIGN-RSA-PSS-SHA384:+SIGN-RSA-PSS-SHA512",
			GNUTLS_E_NO_CIPHER_SUITES, GNUTLS_E_AGAIN,
			&server_ca3_localhost_cert, &server_ca3_key, NULL, NULL);

	server_priority = NULL;
	try_with_key_fail("TLS 1.2 with rsa cert and only RSA-PSS sig algos",
			"NORMAL:-VERS-ALL:+VERS-TLS1.2:-KX-ALL:+ECDHE-RSA:-SIGN-ALL:+SIGN-RSA-PSS-SHA256:+SIGN-RSA-PSS-SHA384:+SIGN-RSA-PSS-SHA512",
			GNUTLS_E_NO_CIPHER_SUITES, GNUTLS_E_AGAIN,
			&server_ca3_localhost_cert, &server_ca3_key, NULL, NULL);

	try_with_key_fail("TLS 1.2 with rsa-pss cert and rsa cli cert with only RSA-PSS sig algos",
			"NORMAL:-VERS-ALL:+VERS-TLS1.2:-SIGN-ALL:+SIGN-RSA-PSS-SHA256:+SIGN-RSA-PSS-SHA384:+SIGN-RSA-PSS-SHA512",
			GNUTLS_E_AGAIN, GNUTLS_E_UNWANTED_ALGORITHM,
			&server_ca3_rsa_pss_cert, &server_ca3_rsa_pss_key, &cli_ca3_cert, &cli_ca3_key);

	try_with_key_fail("TLS 1.2 with rsa encryption cert without RSA",
			"NORMAL:-VERS-ALL:+VERS-TLS1.2:-RSA",
			GNUTLS_E_NO_CIPHER_SUITES, GNUTLS_E_AGAIN,
			&server_ca3_localhost_rsa_decrypt_cert, &server_ca3_key, NULL, NULL);

	try_with_key_fail("TLS 1.2 with (forced) rsa encryption cert and no RSA - client should detect",
			"NORMAL:-VERS-ALL:+VERS-TLS1.2:-RSA:%DEBUG_ALLOW_KEY_USAGE_VIOLATIONS",
			GNUTLS_E_AGAIN, GNUTLS_E_KEY_USAGE_VIOLATION,
			&server_ca3_localhost_rsa_decrypt_cert, &server_ca3_key, NULL, NULL);

	try_with_key_fail("TLS 1.2 with client rsa encryption cert",
			"NORMAL:-VERS-ALL:+VERS-TLS1.2",
			GNUTLS_E_AGAIN, GNUTLS_E_KEY_USAGE_VIOLATION,
			&server_ca3_rsa_pss_cert, &server_ca3_rsa_pss_key, &server_ca3_localhost_rsa_decrypt_cert, &server_ca3_key);

	try_with_key_fail("TLS 1.2 with (forced) client rsa encryption cert - server should detect",
			"NORMAL:-VERS-ALL:+VERS-TLS1.2:%DEBUG_ALLOW_KEY_USAGE_VIOLATIONS",
			GNUTLS_E_KEY_USAGE_VIOLATION, GNUTLS_E_AGAIN,
			&server_ca3_rsa_pss_cert, &server_ca3_rsa_pss_key, &server_ca3_localhost_rsa_decrypt_cert, &server_ca3_key);

	try_with_rawpk_key_fail("rawpk TLS 1.2 with rsa encryption cert without KX-RSA",
			"NORMAL:-VERS-ALL:+VERS-TLS1.2:+CTYPE-RAWPK:-RSA",
			GNUTLS_E_NO_CIPHER_SUITES, GNUTLS_E_AGAIN,
			&rawpk_public_key1, &rawpk_private_key1, GNUTLS_KEY_KEY_ENCIPHERMENT, NULL, NULL, 0);

	try_with_rawpk_key_fail("rawpk TLS 1.2 with client rsa encryption cert without KX-RSA",
			"NORMAL:-VERS-ALL:+VERS-TLS1.2:+CTYPE-RAWPK:-RSA",
			GNUTLS_E_AGAIN, GNUTLS_E_KEY_USAGE_VIOLATION,
			&rawpk_public_key2, &rawpk_private_key2, 0, &rawpk_public_key1, &rawpk_private_key1, GNUTLS_KEY_KEY_ENCIPHERMENT);

	gnutls_global_deinit();
}
