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
#include <unistd.h>
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#include <gnutls/abstract.h>

#include "utils.h"

/* Verify whether gnutls_privkey_sign_hash() will fail
 * if the library is in error state.
 */

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "<%d> %s", level, str);
}

/* sha1 hash of "hello" string */
const gnutls_datum_t hash_data = {
	(void *)
	    "\xaa\xf4\xc6\x1d\xdc\xc5\xe8\xa2\xda\xbe"
	    "\xde\x0f\x3b\x48\x2c\xd9\xae\xa9\x43\x4d",
	20
};

const gnutls_datum_t invalid_hash_data = {
	(void *)
	    "\xaa\xf4\xc6\x1d\xdc\xca\xe8\xa2\xda\xbe"
	    "\xde\x0f\x3b\x48\x2c\xb9\xae\xa9\x43\x4d",
	20
};

const gnutls_datum_t raw_data = {
	(void *) "hello",
	5
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

static char pem2_cert[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIDbzCCAtqgAwIBAgIERiYdRTALBgkqhkiG9w0BAQUwGTEXMBUGA1UEAxMOR251\n"
    "VExTIHRlc3QgQ0EwHhcNMDcwNDE4MTMyOTQxWhcNMDgwNDE3MTMyOTQxWjA3MRsw\n"
    "GQYDVQQKExJHbnVUTFMgdGVzdCBzZXJ2ZXIxGDAWBgNVBAMTD3Rlc3QuZ251dGxz\n"
    "Lm9yZzCCAbQwggEpBgcqhkjOOAQBMIIBHAKBgLmE9VqBvhoNxYpzjwybL5u2DkvD\n"
    "dBp/ZK2d8yjFoEe8m1dW8ZfVfjcD6fJM9OOLfzCjXS+7oaI3wuo1jx+xX6aiXwHx\n"
    "IzYr5E8vLd2d1TqmOa96UXzSJY6XdM8exXtLdkOBBx8GFLhuWBLhkOI3b9Ib7GjF\n"
    "WOLmMOBqXixjeOwHAhSfVoxIZC/+jap6bZbbBF0W7wilcQKBgGIGfuRcdgi3Rhpd\n"
    "15fUKiH7HzHJ0vT6Odgn0Zv8J12nCqca/FPBL0PCN8iFfz1Mq12BMvsdXh5UERYg\n"
    "xoBa2YybQ/Dda6D0w/KKnDnSHHsP7/ook4/SoSLr3OCKi60oDs/vCYXpNr2LelDV\n"
    "e/clDWxgEcTvcJDP1hvru47GPjqXA4GEAAKBgA+Kh1fy0cLcrN9Liw+Luin34QPk\n"
    "VfqymAfW/RKxgLz1urRQ1H+gDkPnn8l4EV/l5Awsa2qkNdy9VOVgNpox0YpZbmsc\n"
    "ur0uuut8h+/ayN2h66SD5out+vqOW9c3yDI+lsI+9EPafZECD7e8+O+P90EAXpbf\n"
    "DwiW3Oqy6QaCr9Ivo4GTMIGQMAwGA1UdEwEB/wQCMAAwGgYDVR0RBBMwEYIPdGVz\n"
    "dC5nbnV0bHMub3JnMBMGA1UdJQQMMAoGCCsGAQUFBwMBMA8GA1UdDwEB/wQFAwMH\n"
    "gAAwHQYDVR0OBBYEFL/su87Y6HtwVuzz0SuS1tSZClvzMB8GA1UdIwQYMBaAFOk8\n"
    "HPutkm7mBqRWLKLhwFMnyPKVMAsGCSqGSIb3DQEBBQOBgQBCsrnfD1xzh8/Eih1f\n"
    "x+M0lPoX1Re5L2ElHI6DJpHYOBPwf9glwxnet2+avzgUQDUFwUSxOhodpyeaACXD\n"
    "o0gGVpcH8sOBTQ+aTdM37hGkPxoXjtIkR/LgG5nP2H2JRd5TkW8l13JdM4MJFB4W\n"
    "QcDzQ8REwidsfh9uKAluk1c/KQ==\n" "-----END CERTIFICATE-----\n";

static char pem2_key[] =
    "-----BEGIN DSA PRIVATE KEY-----\n"
    "MIIBugIBAAKBgQC5hPVagb4aDcWKc48Mmy+btg5Lw3Qaf2StnfMoxaBHvJtXVvGX\n"
    "1X43A+nyTPTji38wo10vu6GiN8LqNY8fsV+mol8B8SM2K+RPLy3dndU6pjmvelF8\n"
    "0iWOl3TPHsV7S3ZDgQcfBhS4blgS4ZDiN2/SG+xoxVji5jDgal4sY3jsBwIVAJ9W\n"
    "jEhkL/6NqnptltsEXRbvCKVxAoGAYgZ+5Fx2CLdGGl3Xl9QqIfsfMcnS9Po52CfR\n"
    "m/wnXacKpxr8U8EvQ8I3yIV/PUyrXYEy+x1eHlQRFiDGgFrZjJtD8N1roPTD8oqc\n"
    "OdIcew/v+iiTj9KhIuvc4IqLrSgOz+8Jhek2vYt6UNV79yUNbGARxO9wkM/WG+u7\n"
    "jsY+OpcCgYAPiodX8tHC3KzfS4sPi7op9+ED5FX6spgH1v0SsYC89bq0UNR/oA5D\n"
    "55/JeBFf5eQMLGtqpDXcvVTlYDaaMdGKWW5rHLq9LrrrfIfv2sjdoeukg+aLrfr6\n"
    "jlvXN8gyPpbCPvRD2n2RAg+3vPjvj/dBAF6W3w8IltzqsukGgq/SLwIUS5/r/2ya\n"
    "AoNBXjeBjgCGMei2m8E=\n" "-----END DSA PRIVATE KEY-----\n";

const gnutls_datum_t cert_dat[] = {
	{(void *) pem1_cert, sizeof(pem1_cert)}
	,
	{(void *) pem2_cert, sizeof(pem2_cert)}
};

const gnutls_datum_t key_dat[] = {
	{(void *) pem1_key, sizeof(pem1_key)}
	,
	{(void *) pem2_key, sizeof(pem2_key)}
};

void _gnutls_lib_simulate_error(void);
void _gnutls_lib_force_operational(void);

void doit(void)
{
	gnutls_privkey_t privkey;
	gnutls_datum_t signature2;
	int ret;
	size_t i;

	global_init();

	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(6);

	for (i = 0; i < sizeof(key_dat) / sizeof(key_dat[0]); i++) {
		if (debug)
			success("loop %d\n", (int) i);

		ret = gnutls_privkey_init(&privkey);
		if (ret < 0)
			fail("gnutls_privkey_init\n");

		ret =
		    gnutls_privkey_import_x509_raw(privkey, &key_dat[i],
						GNUTLS_X509_FMT_PEM, NULL, 0);
		if (ret < 0)
			fail("gnutls_privkey_import\n");

		ret = gnutls_privkey_sign_hash(privkey, GNUTLS_DIG_SHA1, 0,
						&hash_data, &signature2);
		if (ret < 0)
			fail("gnutls_privkey_sign_hash\n");

		gnutls_free(signature2.data);

		_gnutls_lib_simulate_error();
		ret = gnutls_privkey_sign_hash(privkey, GNUTLS_DIG_SHA1, 0,
						&hash_data, &signature2);
		if (ret != GNUTLS_E_LIB_IN_ERROR_STATE)
			fail("gnutls_privkey_sign_hash\n");

		_gnutls_lib_force_operational();

		gnutls_privkey_deinit(privkey);
	}

	gnutls_global_deinit();
}
