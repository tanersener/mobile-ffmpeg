/*
 * Copyright (C) 2015 Nikos Mavrogiannopoulos
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

/* This does some basic functionality testing for the system-keys in
 * windows for CNG keys.
 * It relies on the ncrypt, and crypt32 replacements found in the dir.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef _WIN32

#include <stdlib.h>

void doit()
{
	exit(77);
}

#else

#include <windows.h>
#include <wincrypt.h>
#include <winbase.h>
#include <ncrypt.h>
#include <string.h>
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#include <gnutls/abstract.h>
#include <gnutls/system-keys.h>
#include <gnutls/crypto.h>
#include <stdio.h>
#include <assert.h>
#include <utils.h>
#include "../cert-common.h"
#include "ncrypt-int.h"

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "<%d> %s", level, str);
}

/* sha1 hash of "hello" string */
const gnutls_datum_t sha256_hash_data = {
	(void *)
	    "\x2c\xf2\x4d\xba\x5f\xb0\xa3\x0e\x26\xe8"
	    "\x3b\x2a\xc5\xb9\xe2\x9e\x1b\x16\x1e\x5c"
	    "\x1f\xa7\x42\x5e\x73\x04\x33\x62\x93\x8b"
	    "\x98\x24",
	32
};

const gnutls_datum_t md5sha1_hash_data = {
	(void *)
	    "\x5d\x41\x40\x2a\xbc\x4b\x2a\x76\xb9\x71\x9d\x91\x10\x17\xc5\x92"
	    "\xaa\xf4\xc6\x1d\xdc\xc5\xe8\xa2\xda\xbe\xde\x0f\x3b\x48\x2c\xd9\xae\xa9\x43\x4d",
	36
};

const gnutls_datum_t invalid_hash_data = {
	(void *)
	    "\xaa\xf4\xc6\x1d\xdc\xca\xe8\xa2\xda\xbe"
	    "\xde\x0f\x3b\x48\x2c\xb9\xae\xa9\x43\x4d",
	20
};

const gnutls_datum_t raw_data = {
	(void *)"hello",
	5
};

static
void test_sig(void)
{
	gnutls_pubkey_t pubkey;
	gnutls_privkey_t privkey;
	gnutls_sign_algorithm_t sign_algo = GNUTLS_SIGN_RSA_SHA256;
	gnutls_datum_t signature, digest_info;
	int ret;

	assert_int_nequal(gnutls_pubkey_init(&pubkey), 0);

	assert_int_nequal(gnutls_privkey_init(&privkey), 0);

	assert_int_nequal(gnutls_privkey_import_url(privkey, "system:win:id=123456", 0), 0);

	assert_int_nequal(gnutls_pubkey_import_x509_raw(pubkey, &cert_dat, GNUTLS_X509_FMT_PEM, 0), 0);

	assert_int_nequal(gnutls_privkey_sign_hash(privkey, GNUTLS_DIG_SHA256,
				     0, &sha256_hash_data,
				     &signature), 0);


	ret =
	    gnutls_pubkey_verify_hash2(pubkey,
				       sign_algo, 0,
				       &sha256_hash_data, &signature);
	assert(ret >= 0);

	/* should fail */
	ret =
	    gnutls_pubkey_verify_hash2(pubkey,
				       sign_algo, 0,
				       &invalid_hash_data,
				       &signature);
	assert(ret == GNUTLS_E_PK_SIG_VERIFY_FAILED);

	gnutls_free(signature.data);
	signature.data = NULL;
 /* test the raw interface (MD5+SHA1)
  */
	ret =
	    gnutls_privkey_sign_hash(privkey,
				     0,
				     GNUTLS_PRIVKEY_SIGN_FLAG_TLS1_RSA,
				     &md5sha1_hash_data,
				     &signature);
	assert(ret >= 0);

	ret =
	    gnutls_pubkey_verify_hash2(pubkey,
				       0,
				       GNUTLS_PUBKEY_VERIFY_FLAG_TLS1_RSA,
				       &md5sha1_hash_data,
				       &signature);
	assert(ret >= 0);

	gnutls_free(signature.data);
	signature.data = NULL;

 /* test the raw interface DigestInfo
  */
	ret = gnutls_encode_ber_digest_info(GNUTLS_DIG_SHA256, &sha256_hash_data, &digest_info);
	assert(ret >= 0);

	ret =
	    gnutls_privkey_sign_hash(privkey,
				     0,
				     GNUTLS_PRIVKEY_SIGN_FLAG_TLS1_RSA,
				     &digest_info,
				     &signature);
	assert(ret >= 0);

	ret =
	    gnutls_pubkey_verify_hash2(pubkey,
				       0,
				       GNUTLS_PUBKEY_VERIFY_FLAG_TLS1_RSA,
				       &digest_info,
				       &signature);
	assert(ret >= 0);

	gnutls_free(signature.data);
	gnutls_free(digest_info.data);
	gnutls_privkey_deinit(privkey);
	gnutls_pubkey_deinit(pubkey);
}

void doit(void)
{
	gnutls_global_init();
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(6);

	test_sig();

	gnutls_global_deinit();
	return;
}

#endif
