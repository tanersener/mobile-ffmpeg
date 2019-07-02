/*
 * Copyright (C) 2012 Free Software Foundation, Inc.
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
#include <errno.h>
#include <gnutls/gnutls.h>
#include <unistd.h>
#include "utils.h"
#include "cert-common.h"


/* This will test whether the default public key storage backend
 * is operating properly */

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "|<%d>| %s", level, str);
}

static unsigned char tofu_server_cert_pem[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIICVjCCAcGgAwIBAgIERiYdMTALBgkqhkiG9w0BAQUwGTEXMBUGA1UEAxMOR251\n"
    "VExTIHRlc3QgQ0EwHhcNMDcwNDE4MTMyOTIxWhcNMDgwNDE3MTMyOTIxWjA3MRsw\n"
    "GQYDVQQKExJHbnVUTFMgdGVzdCBzZXJ2ZXIxGDAWBgNVBAMTD3Rlc3QuZ251dGxz\n"
    "Lm9yZzCBnDALBgkqhkiG9w0BAQEDgYwAMIGIAoGA17pcr6MM8C6pJ1aqU46o63+B\n"
    "dUxrmL5K6rce+EvDasTaDQC46kwTHzYWk95y78akXrJutsoKiFV1kJbtple8DDt2\n"
    "DZcevensf9Op7PuFZKBroEjOd35znDET/z3IrqVgbtm2jFqab7a+n2q9p/CgMyf1\n"
    "tx2S5Zacc1LWn9bIjrECAwEAAaOBkzCBkDAMBgNVHRMBAf8EAjAAMBoGA1UdEQQT\n"
    "MBGCD3Rlc3QuZ251dGxzLm9yZzATBgNVHSUEDDAKBggrBgEFBQcDATAPBgNVHQ8B\n"
    "Af8EBQMDB6AAMB0GA1UdDgQWBBTrx0Vu5fglyoyNgw106YbU3VW0dTAfBgNVHSME\n"
    "GDAWgBTpPBz7rZJu5gakViyi4cBTJ8jylTALBgkqhkiG9w0BAQUDgYEAaFEPTt+7\n"
    "bzvBuOf7+QmeQcn29kT6Bsyh1RHJXf8KTk5QRfwp6ogbp94JQWcNQ/S7YDFHglD1\n"
    "AwUNBRXwd3riUsMnsxgeSDxYBfJYbDLeohNBsqaPDJb7XailWbMQKfAbFQ8cnOxg\n"
    "rOKLUQRWJ0K3HyXRMhbqjdLIaQiCvQLuizo=\n" "-----END CERTIFICATE-----\n";

const gnutls_datum_t tofu_server_cert = { tofu_server_cert_pem,
	sizeof(tofu_server_cert_pem)
};

static char client_pem[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIICHjCCAYmgAwIBAgIERiYdNzALBgkqhkiG9w0BAQUwGTEXMBUGA1UEAxMOR251\n"
    "VExTIHRlc3QgQ0EwHhcNMDcwNDE4MTMyOTI3WhcNMDgwNDE3MTMyOTI3WjAdMRsw\n"
    "GQYDVQQDExJHbnVUTFMgdGVzdCBjbGllbnQwgZwwCwYJKoZIhvcNAQEBA4GMADCB\n"
    "iAKBgLtmQ/Xyxde2jMzF3/WIO7HJS2oOoa0gUEAIgKFPXKPQ+GzP5jz37AR2ExeL\n"
    "ZIkiW8DdU3w77XwEu4C5KL6Om8aOoKUSy/VXHqLnu7czSZ/ju0quak1o/8kR4jKN\n"
    "zj2AC41179gAgY8oBAOgIo1hBAf6tjd9IQdJ0glhaZiQo1ipAgMBAAGjdjB0MAwG\n"
    "A1UdEwEB/wQCMAAwEwYDVR0lBAwwCgYIKwYBBQUHAwIwDwYDVR0PAQH/BAUDAweg\n"
    "ADAdBgNVHQ4EFgQUTLkKm/odNON+3svSBxX+odrLaJEwHwYDVR0jBBgwFoAU6Twc\n"
    "+62SbuYGpFYsouHAUyfI8pUwCwYJKoZIhvcNAQEFA4GBALujmBJVZnvaTXr9cFRJ\n"
    "jpfc/3X7sLUsMvumcDE01ls/cG5mIatmiyEU9qI3jbgUf82z23ON/acwJf875D3/\n"
    "U7jyOsBJ44SEQITbin2yUeJMIm1tievvdNXBDfW95AM507ShzP12sfiJkJfjjdhy\n"
    "dc8Siq5JojruiMizAf0pA7in\n" "-----END CERTIFICATE-----\n";
const gnutls_datum_t client_cert =
    { (void *) client_pem, sizeof(client_pem) };

#define TMP_FILE "mini-tdb.tmp"
#define HOSTS_DIR ".gnutls/"
#define HOSTS_FILE HOSTS_DIR"known_hosts"

#define SHA1_HASH "\x53\x4b\x3b\xdc\x5e\xc8\x44\x4c\x02\x20\xbf\x39\x48\x6f\x4c\xfe\xcd\x25\x52\x10"

void doit(void)
{
	gnutls_datum_t der_cert, der_cert2;
	gnutls_datum_t der_rawpk, der_rawpk2;
	int ret;
	gnutls_datum_t hash;
	char path[512];

	/* the sha1 hash of the server's pubkey */
	hash.data = (void *) SHA1_HASH;
	hash.size = sizeof(SHA1_HASH) - 1;

	/* General init. */
	global_init();
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(2);

	// X.509 certificates
	ret =
	    gnutls_pem_base64_decode_alloc("CERTIFICATE", &tofu_server_cert,
					   &der_cert);
	if (ret < 0) {
		fail("base64 decoding\n");
		goto fail;
	}

	ret =
	    gnutls_pem_base64_decode_alloc("CERTIFICATE", &client_cert,
					   &der_cert2);
	if (ret < 0) {
		fail("base64 decoding\n");
		goto fail;
	}

	// Raw public keys
	ret =
	    gnutls_pem_base64_decode_alloc("PUBLIC KEY", &rawpk_public_key1,
					   &der_rawpk);
	if (ret < 0) {
		fail("base64 decoding\n");
		goto fail;
	}

	ret =
	    gnutls_pem_base64_decode_alloc("PUBLIC KEY", &rawpk_public_key2,
					   &der_rawpk2);
	if (ret < 0) {
		fail("base64 decoding\n");
		goto fail;
	}

	remove(HOSTS_FILE);
	remove(TMP_FILE);

	/* verify whether the stored hash verification succeeds */
	ret = gnutls_store_commitment(TMP_FILE, NULL, "localhost", "https",
				      GNUTLS_DIG_SHA1, &hash, 0, GNUTLS_SCOMMIT_FLAG_ALLOW_BROKEN);
	if (ret != 0) {
		fail("commitment storage: %s\n", gnutls_strerror(ret));
		goto fail;
	}

	if (debug)
		success("Commitment storage: passed\n");

	ret =
	    gnutls_verify_stored_pubkey(TMP_FILE, NULL, "localhost",
					"https", GNUTLS_CRT_X509,
					&der_cert, 0);
	remove(TMP_FILE);

	if (ret != 0) {
		fail("commitment verification: %s\n",
		     gnutls_strerror(ret));
		goto fail;
	}

	if (debug)
		success("Commitment verification: passed\n");

	/* Verify access to home dir */
#ifndef _WIN32
	setenv("HOME", getcwd(path, sizeof(path)), 1);

	/* verify whether the stored hash verification succeeeds */
	ret = gnutls_store_commitment(NULL, NULL, "localhost", "https",
				      GNUTLS_DIG_SHA1, &hash, 0, GNUTLS_SCOMMIT_FLAG_ALLOW_BROKEN);
	if (ret != 0) {
		fail("commitment storage: %s\n", gnutls_strerror(ret));
		goto fail;
	}

	if (debug)
		success("Commitment storage: passed\n");

	ret =
	    gnutls_verify_stored_pubkey(NULL, NULL, "localhost",
					"https", GNUTLS_CRT_X509,
					&der_cert, 0);

	if (ret != 0) {
		fail("commitment verification: %s\n",
		     gnutls_strerror(ret));
		goto fail;
	}

	if (debug)
		success("Commitment from homedir verification: passed\n");
#endif

	/* verify whether the stored pubkey verification succeeds */
	// First we test regular X.509 certs
	ret = gnutls_store_pubkey(TMP_FILE, NULL, "localhost", "https",
				  GNUTLS_CRT_X509, &der_cert, 0, 0);
	if (ret != 0) {
		fail("storage: %s\n", gnutls_strerror(ret));
		goto fail;
	}

	if (debug)
		success("Public key storage (from cert): passed\n");

	ret =
	    gnutls_verify_stored_pubkey(TMP_FILE, NULL, "localhost",
					"https", GNUTLS_CRT_X509,
					&der_cert, 0);
	if (ret != 0) {
		fail("pubkey verification (from cert): %s\n", gnutls_strerror(ret));
		goto fail;
	}

	ret =
	    gnutls_verify_stored_pubkey(TMP_FILE, NULL, "localhost",
					"https", GNUTLS_CRT_X509,
					&der_cert2, 0);
	if (ret == 0) {
		fail("verification succeeded when shouldn't!\n");
		goto fail;
	}
	if (ret != GNUTLS_E_CERTIFICATE_KEY_MISMATCH) {
		fail("Wrong error code returned: %s!\n",
		     gnutls_strerror(ret));
		goto fail;
	}

	if (debug)
		success("Public key verification (from cert): passed\n");

	// Secondly we test raw public keys
	ret = gnutls_store_pubkey(TMP_FILE, NULL, "localhost", "https",
				  GNUTLS_CRT_RAWPK, &der_rawpk, 0, 0);
	if (ret != 0) {
		fail("storage: %s\n", gnutls_strerror(ret));
		goto fail;
	}

	if (debug)
		success("Public key storage (from raw pk): passed\n");

	ret =
	    gnutls_verify_stored_pubkey(TMP_FILE, NULL, "localhost",
					"https", GNUTLS_CRT_RAWPK,
					&der_rawpk, 0);
	if (ret != 0) {
		fail("pubkey verification (from raw pk): %s\n", gnutls_strerror(ret));
		goto fail;
	}

	ret =
	    gnutls_verify_stored_pubkey(TMP_FILE, NULL, "localhost",
					"https", GNUTLS_CRT_RAWPK,
					&der_rawpk2, 0);
	if (ret == 0) {
		fail("verification succeeded when shouldn't!\n");
		goto fail;
	}
	if (ret != GNUTLS_E_CERTIFICATE_KEY_MISMATCH) {
		fail("Wrong error code returned: %s!\n",
		     gnutls_strerror(ret));
		goto fail;
	}

	if (debug)
		success("Public key verification (from raw pk): passed\n");

	remove(HOSTS_FILE);
	remove(TMP_FILE);
	rmdir(HOSTS_DIR);

	gnutls_global_deinit();
	gnutls_free(der_cert.data);
	gnutls_free(der_cert2.data);
	gnutls_free(der_rawpk.data);
	gnutls_free(der_rawpk2.data);

	return;
      fail:
	remove(HOSTS_FILE);
	remove(TMP_FILE);
	rmdir(HOSTS_DIR);
	exit(1);
}
