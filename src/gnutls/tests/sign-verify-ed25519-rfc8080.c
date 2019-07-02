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
 * You should have received a copy of the GNU General Public License
 * along with GnuTLS; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#ifndef _WIN32
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#endif
#include <unistd.h>
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#include <gnutls/abstract.h>
#include "utils.h"

/* verifies whether the sign-data and verify-data APIs
 * operate as expected */

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "<%d> %s", level, str);
}

/* Values from RFC8080 */
static unsigned char ed25519_x[] =
    "\x97\x4d\x96\xa2\x2d\x22\x4b\xc0\x1a\xdb\x91\x50\x91\x47\x7d\x44\xcc\xd9\x1c\x9a\x41\xa1\x14\x30\x01\x01\x17\xd5\x2c\x59\x24\x0e";
static unsigned char ed25519_k[] =
    "\x38\x32\x32\x36\x30\x33\x38\x34\x36\x32\x38\x30\x38\x30\x31\x32\x32\x36\x34\x35\x31\x39\x30\x32\x30\x34\x31\x34\x32\x32\x36\x32";

static gnutls_datum_t _ed25519_x = { ed25519_x, sizeof(ed25519_x) - 1 };
static gnutls_datum_t _ed25519_k = { ed25519_k, sizeof(ed25519_k) - 1 };

/* sha1 hash of "hello" string */
const gnutls_datum_t raw_data = {
	(void *)
	    "\xaa\xf4\xc6\x1d\xdc\xc5\xe8\xa2\xda\xbe"
	    "\xde\x0f\x3b\x48\x2c\xd9\xae\xa9\x43\x4d",
	20
};

const gnutls_datum_t invalid_raw_data = {
	(void *)
	    "\xaa\xf4\xc6\x1d\xdc\xc5\xe8\xa2\xda\xbe"
	    "\xde\x0f\x3b\x48\x3c\xd9\xae\xa9\x43\x4d",
	20
};

void doit(void)
{
	gnutls_pubkey_t pubkey;
	gnutls_privkey_t key;
	gnutls_datum_t signature;
	int ret;

	global_init();

	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(6);

	ret = gnutls_privkey_init(&key);
	if (ret < 0)
		fail("error\n");

	ret =
	    gnutls_privkey_import_ecc_raw(key, GNUTLS_ECC_CURVE_ED25519,
					  &_ed25519_x, NULL, &_ed25519_k);
	if (ret < 0)
		fail("error\n");

	ret = gnutls_privkey_verify_params(key);
	if (ret != 0)
		fail("error: %s\n", gnutls_strerror(ret));

	ret = gnutls_privkey_sign_data(key, GNUTLS_DIG_SHA512, 0,
				       &raw_data, &signature);
	if (ret < 0)
		fail("gnutls_x509_privkey_sign_hash\n");

	/* verification */
	ret = gnutls_pubkey_init(&pubkey);
	if (ret < 0)
		fail("gnutls_privkey_init\n");

	ret = gnutls_pubkey_import_privkey(pubkey, key, 0, 0);
	if (ret < 0)
		fail("gnutls_x509_pubkey_import\n");

	ret =
	    gnutls_pubkey_verify_data2(pubkey, GNUTLS_SIGN_EDDSA_ED25519, 0,
				       &raw_data, &signature);
	if (ret < 0)
		fail("gnutls_x509_pubkey_verify_data2\n");

	gnutls_pubkey_deinit(pubkey);

	/* try importing the pubkey directly */
	ret = gnutls_pubkey_init(&pubkey);
	if (ret < 0)
		fail("gnutls_privkey_init\n");

	ret = gnutls_pubkey_import_ecc_raw(pubkey, GNUTLS_ECC_CURVE_ED25519, &_ed25519_x, NULL);
	if (ret < 0)
		fail("gnutls_x509_pubkey_import_ecc_raw\n");

	ret =
	    gnutls_pubkey_verify_data2(pubkey, GNUTLS_SIGN_EDDSA_ED25519, 0,
				       &raw_data, &signature);
	if (ret < 0)
		fail("gnutls_x509_pubkey_verify_data2\n");

	gnutls_pubkey_deinit(pubkey);

	gnutls_privkey_deinit(key);
	gnutls_free(signature.data);

	gnutls_global_deinit();
}
