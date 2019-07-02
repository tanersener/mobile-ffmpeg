/*
 * Copyright (C) 2018 ARPA2 project
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <gnutls/gnutls.h>
#include <gnutls/abstract.h>
#include "utils.h"
#include <assert.h>
#include "cert-common.h"

/* Here we test the raw public-key API */

void doit(void)
{
	int ret;
	gnutls_certificate_credentials_t cred = NULL;
	gnutls_pcert_st* pcert;
	gnutls_pubkey_t pubkey;
	const char *src;
	char rawpk_pub_path[256];
	char rawpk_priv_path[256];

	// Get current src dir
	src = getenv("srcdir");
	if (src == NULL)
		src = ".";

	// Set file paths for pem files
	snprintf(rawpk_pub_path, sizeof(rawpk_pub_path), "%s/%s", src, "certs/rawpk_pub.pem");
	snprintf(rawpk_priv_path, sizeof(rawpk_priv_path), "%s/%s", src, "certs/rawpk_priv.pem");


	global_init();

	// Initialize creds
	assert(gnutls_certificate_allocate_credentials(&cred) >= 0);
	assert((pcert = gnutls_calloc(1, sizeof(*pcert))) != NULL);
	assert(gnutls_pubkey_init(&pubkey) >= 0);
	assert(gnutls_pubkey_import(pubkey, &rawpk_public_key1, GNUTLS_X509_FMT_PEM) >= 0);


	/* Tests for gnutls_certificate_set_rawpk_key_mem() */
	success("Testing gnutls_certificate_set_rawpk_key_mem()...\n");
	// Positive tests
	ret = gnutls_certificate_set_rawpk_key_mem(cred,
					&rawpk_public_key2, &rawpk_private_key2, GNUTLS_X509_FMT_PEM,
					NULL, 0, NULL, 0, 0);
	if (ret < 0) {
		fail("Failed to load credentials with error: %d\n", ret);
	}
	// Negative tests
	ret = gnutls_certificate_set_rawpk_key_mem(cred,
					NULL, &rawpk_private_key2, GNUTLS_X509_FMT_PEM,
					NULL, 0, NULL, 0, 0);
	if (ret != GNUTLS_E_INSUFFICIENT_CREDENTIALS) {
		fail("Failed to detect falsy input. Expected error: %d\n", GNUTLS_E_INSUFFICIENT_CREDENTIALS);
	}
	ret = gnutls_certificate_set_rawpk_key_mem(cred,
				&rawpk_public_key2, NULL, GNUTLS_X509_FMT_PEM,
				NULL, 0, NULL, 0, 0);
	if (ret != GNUTLS_E_INSUFFICIENT_CREDENTIALS) {
		fail("Failed to detect falsy input. Expected error: %d\n", GNUTLS_E_INSUFFICIENT_CREDENTIALS);
	}


	/* Tests for gnutls_certificate_set_rawpk_key_file() */
	success("Testing gnutls_certificate_set_rawpk_key_file()...\n");
	// Positive tests
	ret = gnutls_certificate_set_rawpk_key_file(cred, rawpk_pub_path, rawpk_priv_path, GNUTLS_X509_FMT_PEM, NULL, 0, NULL, 0, 0, 0);
	if (ret < 0) {
		fail("Failed to load credentials with error: %d\n", ret);
	}
	// Negative tests
	ret = gnutls_certificate_set_rawpk_key_file(cred, NULL, rawpk_priv_path, GNUTLS_X509_FMT_PEM, NULL, 0, NULL, 0, 0, 0);
	if (ret != GNUTLS_E_INSUFFICIENT_CREDENTIALS) {
		fail("Failed to detect falsy input. Expected error: %d\n", GNUTLS_E_INSUFFICIENT_CREDENTIALS);
	}
	ret = gnutls_certificate_set_rawpk_key_file(cred, rawpk_pub_path, NULL, GNUTLS_X509_FMT_PEM, NULL, 0, NULL, 0, 0, 0);
	if (ret != GNUTLS_E_INSUFFICIENT_CREDENTIALS) {
		fail("Failed to detect falsy input. Expected error: %d\n", GNUTLS_E_INSUFFICIENT_CREDENTIALS);
	}


	/* Tests for gnutls_pcert_import_rawpk() */
	success("Testing gnutls_pcert_import_rawpk()...\n");
	// Positive tests
	ret = gnutls_pcert_import_rawpk(pcert, pubkey, 0);
	if (ret < 0) {
		fail("Failed to import raw public-key into pcert with error: %d\n", ret);
	}
	// Negative tests
	ret = gnutls_pcert_import_rawpk(pcert, NULL, 0);
	if (ret != GNUTLS_E_INSUFFICIENT_CREDENTIALS) {
		fail("Failed to detect falsy input. Expected error: %d\n", GNUTLS_E_INSUFFICIENT_CREDENTIALS);
	}
	// Cleanup to prevent subsequent API calls to produce memory leaks
	gnutls_pcert_deinit(pcert);


	/* Tests for gnutls_pcert_import_rawpk_raw() */
	success("Testing gnutls_pcert_import_rawpk_raw()...\n");
	// Positive tests
	ret = gnutls_pcert_import_rawpk_raw(pcert, &rawpk_public_key1, GNUTLS_X509_FMT_PEM, 0, 0);
	if (ret < 0) {
		fail("Failed to import raw public-key into pcert with error: %d\n", ret);
	}
	// Negative tests
	ret = gnutls_pcert_import_rawpk_raw(pcert, NULL, GNUTLS_X509_FMT_PEM, 0, 0);
	if (ret != GNUTLS_E_INSUFFICIENT_CREDENTIALS) {
		fail("Failed to detect falsy input. Expected error: %d\n", GNUTLS_E_INSUFFICIENT_CREDENTIALS);
	}
	// Cleanup to prevent subsequent API calls to produce memory leaks
	gnutls_pcert_deinit(pcert);


	// Generic cleanup
	gnutls_free(pcert);
	gnutls_certificate_free_credentials(cred);

	gnutls_global_deinit();
}

