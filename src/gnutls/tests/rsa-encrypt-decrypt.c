/*
 * Copyright (C) 2004-2012 Free Software Foundation, Inc.
 *
 * Author: Nikos Mavrogiannopoulos, Simon Josefsson
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

/* Parts copied from GnuTLS example programs. */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#ifndef _WIN32
# include <netinet/in.h>
# include <sys/socket.h>
# include <arpa/inet.h>
#endif
#include <unistd.h>
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#include <gnutls/abstract.h>

#include "utils.h"

/* sha1 hash of "hello" string */
const gnutls_datum_t hash_data = {
	(void *)
	    "\xaa\xf4\xc6\x1d\xdc\xc5\xe8\xa2\xda\xbe"
	    "\xde\x0f\x3b\x48\x2c\xd9\xae\xa9\x43\x4d",
	20
};

const gnutls_datum_t raw_data = {
	(void *) "hello there",
	11
};

static char pem1_cert[] =
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

static char pem1_key[] =
    "-----BEGIN RSA PRIVATE KEY-----\n"
    "MIICXAIBAAKBgQC7ZkP18sXXtozMxd/1iDuxyUtqDqGtIFBACIChT1yj0Phsz+Y8\n"
    "9+wEdhMXi2SJIlvA3VN8O+18BLuAuSi+jpvGjqClEsv1Vx6i57u3M0mf47tKrmpN\n"
    "aP/JEeIyjc49gAuNde/YAIGPKAQDoCKNYQQH+rY3fSEHSdIJYWmYkKNYqQIDAQAB\n"
    "AoGADpmARG5CQxS+AesNkGmpauepiCz1JBF/JwnyiX6vEzUh0Ypd39SZztwrDxvF\n"
    "PJjQaKVljml1zkJpIDVsqvHdyVdse8M+Qn6hw4x2p5rogdvhhIL1mdWo7jWeVJTF\n"
    "RKB7zLdMPs3ySdtcIQaF9nUAQ2KJEvldkO3m/bRJFEp54k0CQQDYy+RlTmwRD6hy\n"
    "7UtMjR0H3CSZJeQ8svMCxHLmOluG9H1UKk55ZBYfRTsXniqUkJBZ5wuV1L+pR9EK\n"
    "ca89a+1VAkEA3UmBelwEv2u9cAU1QjKjmwju1JgXbrjEohK+3B5y0ESEXPAwNQT9\n"
    "TrDM1m9AyxYTWLxX93dI5QwNFJtmbtjeBQJARSCWXhsoaDRG8QZrCSjBxfzTCqZD\n"
    "ZXtl807ymCipgJm60LiAt0JLr4LiucAsMZz6+j+quQbSakbFCACB8SLV1QJBAKZQ\n"
    "YKf+EPNtnmta/rRKKvySsi3GQZZN+Dt3q0r094XgeTsAqrqujVNfPhTMeP4qEVBX\n"
    "/iVX2cmMTSh3w3z8MaECQEp0XJWDVKOwcTW6Ajp9SowtmiZ3YDYo1LF9igb4iaLv\n"
    "sWZGfbnU3ryjvkb6YuFjgtzbZDZHWQCo8/cOtOBmPdk=\n"
    "-----END RSA PRIVATE KEY-----\n";

const gnutls_datum_t cert_dat[] = {
	{(void *) pem1_cert, sizeof(pem1_cert)}
};

const gnutls_datum_t key_dat[] = {
	{(void *) pem1_key, sizeof(pem1_key)}
};

void doit(void)
{
	gnutls_x509_privkey_t key;
	gnutls_x509_crt_t crt;
	gnutls_pubkey_t pubkey;
	gnutls_privkey_t privkey;
	gnutls_datum_t out, out2;
	int ret;
	size_t i;

	global_init();

	for (i = 0; i < sizeof(key_dat) / sizeof(key_dat[0]); i++) {
		if (debug)
			success("loop %d\n", (int) i);

		ret = gnutls_x509_privkey_init(&key);
		if (ret < 0)
			fail("gnutls_x509_privkey_init\n");

		ret =
		    gnutls_x509_privkey_import(key, &key_dat[i],
						GNUTLS_X509_FMT_PEM);
		if (ret < 0)
			fail("gnutls_x509_privkey_import\n");

		ret = gnutls_pubkey_init(&pubkey);
		if (ret < 0)
			fail("gnutls_privkey_init\n");

		ret = gnutls_privkey_init(&privkey);
		if (ret < 0)
			fail("gnutls_pubkey_init\n");

		ret = gnutls_privkey_import_x509(privkey, key, 0);
		if (ret < 0)
			fail("gnutls_privkey_import_x509\n");

		ret = gnutls_x509_crt_init(&crt);
		if (ret < 0)
			fail("gnutls_x509_crt_init\n");

		ret =
		    gnutls_x509_crt_import(crt, &cert_dat[i],
					   GNUTLS_X509_FMT_PEM);
		if (ret < 0)
			fail("gnutls_x509_crt_import\n");

		ret = gnutls_pubkey_import_x509(pubkey, crt, 0);
		if (ret < 0)
			fail("gnutls_x509_pubkey_import\n");


		ret =
		    gnutls_pubkey_encrypt_data(pubkey, 0, &hash_data,
						&out);
		if (ret < 0)
			fail("gnutls_pubkey_encrypt_data\n");


		ret = gnutls_privkey_decrypt_data(privkey, 0, &out, &out2);
		if (ret < 0)
			fail("gnutls_privkey_decrypt_data\n");

		if (out2.size != hash_data.size)
			fail("Decrypted data don't match original (1)\n");

		if (memcmp(out2.data, hash_data.data, hash_data.size) != 0)
			fail("Decrypted data don't match original (2)\n");

		gnutls_free(out.data);
		gnutls_free(out2.data);

		ret =
		    gnutls_pubkey_encrypt_data(pubkey, 0, &raw_data, &out);
		if (ret < 0)
			fail("gnutls_pubkey_encrypt_data\n");

		ret = gnutls_privkey_decrypt_data(privkey, 0, &out, &out2);
		if (ret < 0)
			fail("gnutls_privkey_decrypt_data\n");

		if (out2.size != raw_data.size)
			fail("Decrypted data don't match original (3)\n");

		if (memcmp(out2.data, raw_data.data, raw_data.size) != 0)
			fail("Decrypted data don't match original (4)\n");

		if (debug)
			success("ok\n");

		gnutls_free(out.data);
		gnutls_free(out2.data);
		gnutls_x509_privkey_deinit(key);
		gnutls_x509_crt_deinit(crt);
		gnutls_privkey_deinit(privkey);
		gnutls_pubkey_deinit(pubkey);
	}

	gnutls_global_deinit();
}
