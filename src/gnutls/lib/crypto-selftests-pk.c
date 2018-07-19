/*
 * Copyright (C) 2013 Red Hat
 *
 * Author: Nikos Mavrogiannopoulos
 *
 * This file is part of GnuTLS.
 *
 * The GnuTLS is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 */

#include "gnutls_int.h"
#include "errors.h"
#include <cipher_int.h>
#include <datum.h>
#include <gnutls/crypto.h>
#include <gnutls/self-test.h>
#include "errors.h"
#include <gnutls/abstract.h>
#include <pk.h>
#include <debug.h>

#define DATASTR "Hello there!"
static const gnutls_datum_t signed_data = {
	.data = (void *) DATASTR,
	.size = sizeof(DATASTR) - 1
};

static const gnutls_datum_t bad_data = {
	.data = (void *) DATASTR,
	.size = sizeof(DATASTR) - 2
};

static const char rsa_key2048[] =
 "-----BEGIN RSA PRIVATE KEY-----\n"
 "MIIEogIBAAKCAQEA6yCv+BLrRP/dMPBXJWK21c0aqxIX6JkODL4K+zlyEURt8/Wp\n"
 "nw37CJwHD3VrimSnk2SJvBfTNhzYhCsLShDOPvi4qBrLZ1WozjoVJ8tRE4VCcjQJ\n"
 "snpJ7ldiV+Eos1Z3FkbV/uQcw5CYCb/TciSukaWlI+G/xas9EOOFt4aELbc1yDe0\n"
 "hyfPDtoaKfek4GhT9qT1I8pTC40P9OrA9Jt8lblqxHWwqmdunLTjPjB5zJT6QgI+\n"
 "j1xuq7ZOQhveNA/AOyzh574GIpgsuvPPLBQwsCQkscr7cFnCsyOPgYJrQW3De2+l\n"
 "wjp2D7gZeeQcFQKazXcFoiqNpJWoBWmU0qqsgwIDAQABAoIBAAghNzRioxPdrO42\n"
 "QS0fvqah0tw7Yew+7oduQr7w+4qxTQP0aIsBVr6zdmMIclF0rX6hKUoBoOHsGWho\n"
 "fJlw/1CaFPhrBMFr6sxGodigZQtBvkxolDVBmTDOgK39MQUSZke0501K4du5MiiU\n"
 "I2F89zQ9//m/onvZMeFVnJf95LAX5qHr/FLARQFtOpgWzcGVxdvJdJlYb1zMUril\n"
 "PqyAZXo1j0vgHWwSd54k8mBLus7l8KT57VFce8+9nBPrOrqW4rDVXzs/go3S+kiI\n"
 "OyzYeUs9czg1N1e3VhEaC+EdYUawc0ASuEkbsJ53L8pwDvS+2ly2ykYziJp95Fjv\n"
 "bzyd1dECgYEA8FzGCxu7A6/ei9Dn0Fmi8Ns/QvEgbdlGw4v4MlXHjrGJYdOB0BwG\n"
 "2D2k0ODNYKlUX2J4hi5x8aCH33y/v0EcOHyuqM33vOWBVbdcumCqcOmp341UebAO\n"
 "uCPgDJNhjxXaeDVPnizqnOBA1B9sTxwmCOmFIiFRLbR+XluvDh3t8L0CgYEA+my6\n"
 "124Rw7kcFx+9JoB/Z+bUJDYpefUT91gBUhhEdEMx5fujhMzAbLpIRjFQq+75Qb7v\n"
 "0NyIS09B4oKOqQYzVEJwqKY7H71BTl7QuzJ8Qtuh/DMZsVIt6xpvdeuAKpEOqz44\n"
 "ZD3fW1B59A3ja7kqZadCqq2b02UTk+gdeOrYBj8CgYACX3gZDfoHrEnPKY3QUcI5\n"
 "DIEQYR8H1phLP+uAW7ZvozMPAy6J5mzu35Tr9vwwExvhITC9amH3l7UfsLSX58Wm\n"
 "jRyQUBA9Dir7tKa2tFOab8Qcj+GgnetXSAtjNGVHK1kPzL7vedQLHm+laHYCRe3e\n"
 "Mqf80UVi5SBGQDN3OTZrJQKBgEkj2oozDqMwfGDQl0kYfJ2XEFynKQQCrVsva+tT\n"
 "RSMDwR4fmcmel5Dp81P08U/WExy9rIM+9duxAVgrs4jwU6uHYCoRqvEBMIK4NJSI\n"
 "ETzhsvTa4+UjUF/7L5SsPJmyFiuzl3rHi2W7InNCXyrGQPjBmjoJTJq4SbiIMZtw\n"
 "U7m3AoGACG2rE/Ud71kyOJcKwxzEt8kd+2CMuaZeE/xk+3zLSSjXJzKPficogM3I\n"
 "K37/N7N0FjhdQ5hRuD3GH1fcjv9AKdGHsH7RuaG+jHTRUjS1glr17SSQzh6xXnWj\n"
 "jG0M4UZm5P9STL09nZuWH0wfpr/eg+9+A6yOVfnADI13v+Ygk7k=\n"
 "-----END RSA PRIVATE KEY-----\n";

static const char ecc_key[] =
 "-----BEGIN EC PRIVATE KEY-----\n"
 "MHgCAQEEIQDzaOg65+brGV6INN0zXrUodxwRuocGG+GtKzN7ko26v6AKBggqhkjO\n"
 "PQMBB6FEA0IABEppi34ngyNQ2a9kJmnT5QxIHAUGI1SpnsAyCfze4j6MJ7o/g7qx\n"
 "MSHpe5vd0TQz+/GAa1zxle8mB/Cdh0JaTrA=\n"
 "-----END EC PRIVATE KEY-----\n";

static const char dsa_key[] =
 "-----BEGIN DSA PRIVATE KEY-----\n"
 "MIH4AgEAAkEA6KUOSXfFNcInFLPdOlLlKNCe79zJrkxnsQN+lllxuk1ifZrE07r2\n"
 "3edTrc4riQNnZ2nZ372tYUAMJg+5jM6IIwIVAOa58exwZ+42Tl+p3b4Kbpyu2Ron\n"
 "AkBocj7gkiBYHtv6HMIIzooaxn4vpGR0Ns6wBfroBUGvrnSAgfT3WyiNaHkIF28e\n"
 "quWcEeOJjUgFvatcM8gcY288AkEAyKWlgzBurIYST8TM3j4PuQJDTvdHDaGoAUAa\n"
 "EfjmOw2UXKwqTmwPiT5BYKgCo2ILS87ttlTpd8vndH37pmnmVQIUQIVuKpZ8y9Bw\n"
 "VzO8qcrLCFvTOXY=\n"
 "-----END DSA PRIVATE KEY-----\n";


static int test_rsa_enc(gnutls_pk_algorithm_t pk,
			unsigned bits, gnutls_digest_algorithm_t ign)
{
	int ret;
	gnutls_datum_t enc = { NULL, 0 };
	gnutls_datum_t dec = { NULL, 0 };
	gnutls_datum_t raw_rsa_key = { (void*)rsa_key2048, sizeof(rsa_key2048)-1 };
	gnutls_privkey_t key;
	gnutls_pubkey_t pub = NULL;

	ret = gnutls_privkey_init(&key);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = gnutls_pubkey_init(&pub);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = gnutls_privkey_import_x509_raw(key, &raw_rsa_key, GNUTLS_X509_FMT_PEM, NULL, 0);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = gnutls_pubkey_import_privkey(pub, key, 0, 0);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = gnutls_pubkey_encrypt_data(pub, 0, &signed_data, &enc);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	if (enc.size == signed_data.size && memcmp(signed_data.data, enc.data,
		enc.size) == 0) {
		gnutls_assert();
		ret = GNUTLS_E_SELF_TEST_ERROR;
		goto cleanup;
	}

	ret = gnutls_privkey_decrypt_data(key, 0, &enc, &dec);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	if (dec.size != signed_data.size
	    || memcmp(dec.data, signed_data.data, dec.size) != 0) {
		ret = GNUTLS_E_SELF_TEST_ERROR;
		gnutls_assert();
		goto cleanup;
	}

	ret = 0;
      cleanup:
	if (pub != NULL)
		gnutls_pubkey_deinit(pub);
	gnutls_privkey_deinit(key);
	gnutls_free(enc.data);
	gnutls_free(dec.data);

	if (ret == 0)
		_gnutls_debug_log("%s-%u-enc self test succeeded\n",
				  gnutls_pk_get_name(pk), bits);
	else
		_gnutls_debug_log("%s-%u-enc self test failed\n",
				  gnutls_pk_get_name(pk), bits);

	return ret;
}

static int test_sig(gnutls_pk_algorithm_t pk,
		    unsigned bits, gnutls_digest_algorithm_t dig)
{
	int ret;
	gnutls_datum_t sig = { NULL, 0 };
	gnutls_datum_t raw_rsa_key = { (void*)rsa_key2048, sizeof(rsa_key2048)-1 };
	gnutls_datum_t raw_dsa_key = { (void*)dsa_key, sizeof(dsa_key)-1 };
	gnutls_datum_t raw_ecc_key = { (void*)ecc_key, sizeof(ecc_key)-1 };
	gnutls_privkey_t key;
	gnutls_pubkey_t pub = NULL;
	char param_name[32];

	if (pk == GNUTLS_PK_EC) {
		snprintf(param_name, sizeof(param_name), "%s",
			 gnutls_ecc_curve_get_name(GNUTLS_BITS_TO_CURVE
						   (bits)));
	} else {
		snprintf(param_name, sizeof(param_name), "%u", bits);
	}

	ret = gnutls_privkey_init(&key);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = gnutls_pubkey_init(&pub);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	if (pk == GNUTLS_PK_RSA) {
		ret = gnutls_privkey_import_x509_raw(key, &raw_rsa_key, GNUTLS_X509_FMT_PEM, NULL, 0);
	} else if (pk == GNUTLS_PK_DSA) {
		ret = gnutls_privkey_import_x509_raw(key, &raw_dsa_key, GNUTLS_X509_FMT_PEM, NULL, 0);
	} else if (pk == GNUTLS_PK_ECC) {
		ret = gnutls_privkey_import_x509_raw(key, &raw_ecc_key, GNUTLS_X509_FMT_PEM, NULL, 0);
	} else {
		gnutls_assert();
		ret = GNUTLS_E_INTERNAL_ERROR;
	}

	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = gnutls_pubkey_import_privkey(pub, key, 0, 0);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = gnutls_privkey_sign_data(key, dig, 0, &signed_data, &sig);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret =
	    gnutls_pubkey_verify_data2(pub, gnutls_pk_to_sign(pk, dig), 0,
				       &signed_data, &sig);
	if (ret < 0) {
		ret = GNUTLS_E_SELF_TEST_ERROR;
		gnutls_assert();
		goto cleanup;
	}

	ret =
	    gnutls_pubkey_verify_data2(pub, gnutls_pk_to_sign(pk, dig), 0,
				       &bad_data, &sig);

	if (ret != GNUTLS_E_PK_SIG_VERIFY_FAILED) {
		ret = GNUTLS_E_SELF_TEST_ERROR;
		gnutls_assert();
		goto cleanup;
	}

	ret = 0;
      cleanup:
	if (pub != NULL)
		gnutls_pubkey_deinit(pub);
	gnutls_privkey_deinit(key);
	gnutls_free(sig.data);

	if (ret == 0)
		_gnutls_debug_log("%s-%s-sig self test succeeded\n",
				  gnutls_pk_get_name(pk), param_name);
	else
		_gnutls_debug_log("%s-%s-sig self test failed\n",
				  gnutls_pk_get_name(pk), param_name);

	return ret;
}

/* A precomputed RSA-SHA1 signature using the rsa_key2048 */
static const char rsa_sig[] =
    "\x7a\xb3\xf8\xb0\xf9\xf0\x52\x88\x37\x17\x97\x9f\xbe\x61\xb4\xd2\x43\x78\x9f\x79\x92\xd0\xad\x08\xdb\xbd\x3c\x72\x7a\xb5\x51\x59\x63\xd6\x7d\xf1\x9c\x1e\x10\x7b\x27\xab\xf8\xd4\x9d\xcd\xc5\xf9\xae\xf7\x09\x6b\x40\x93\xc5\xe9\x1c\x0f\xb4\x82\xa1\x47\x86\x54\x63\xd2\x4d\x40\x9a\x80\xb9\x38\x45\x69\xa2\xd6\x92\xb6\x69\x7f\x3f\xf3\x5b\xa5\x1d\xac\x06\xad\xdf\x4e\xbb\xe6\xda\x68\x0d\xe5\xab\xef\xd2\xf0\xc5\xd8\xc0\xed\x80\xe2\xd4\x76\x98\xec\x44\xa2\xfc\x3f\xce\x2e\x8b\xc4\x4b\xab\xb0\x70\x24\x52\x85\x2a\x36\xcd\x9a\xb5\x05\x00\xea\x98\x7c\x72\x06\x68\xb1\x38\x44\x16\x80\x6a\x3b\x64\x72\xbb\xfd\x4b\xc9\xdd\xda\x2a\x68\xde\x7f\x6e\x48\x28\xc1\x63\x57\x2b\xde\x83\xa3\x27\x34\xd7\xa6\x87\x18\x35\x10\xff\x31\xd9\x47\xc9\x84\x35\xe1\xaa\xe2\xf7\x98\xfa\x19\xd3\xf1\x94\x25\x2a\x96\xe4\xa8\xa7\x05\x10\x93\x87\xde\x96\x85\xe5\x68\xb8\xe5\x4e\xbf\x66\x85\x91\xbd\x52\x5b\x3d\x9f\x1b\x79\xea\xe3\x8b\xef\x62\x18\x39\x7a\x50\x01\x46\x1b\xde\x8d\x37\xbc\x90\x6c\x07\xc0\x07\xed\x60\xce\x2e\x31\xd6\x8f\xe8\x75\xdb\x45\x21\xc6\xcb";

/* ECDSA key and signature */
static const char ecdsa_secp256r1_privkey[] =
    "-----BEGIN EC PRIVATE KEY-----\n"
    "MHcCAQEEIPAKWV7+pZe9c5EubMNfAEKWRQtP/MvlO9HehwHmJssNoAoGCCqGSM49\n"
    "AwEHoUQDQgAE2CNONRio3ciuXtoomJKs3MdbzLbd44VPhtzJN30VLFm5gvnfiCj2\n"
    "zzz7pl9Cv0ECHl6yedNI8QEKdcwCDgEmkQ==\n"
    "-----END EC PRIVATE KEY-----\n";

static const char ecdsa_secp256r1_sig[] =
    "\x30\x45\x02\x21\x00\x9b\x8f\x60\xed\x9e\x40\x8d\x74\x82\x73\xab\x20\x1a\x69\xfc\xf9\xee\x3c\x41\x80\xc0\x39\xdd\x21\x1a\x64\xfd\xbf\x7e\xaa\x43\x70\x02\x20\x44\x28\x05\xdd\x30\x47\x58\x96\x18\x39\x94\x18\xba\xe7\x7a\xf6\x1e\x2d\xba\xb1\xe0\x7d\x73\x9e\x2f\x58\xee\x0c\x2a\x89\xe8\x35";

#ifdef ENABLE_NON_SUITEB_CURVES
/* sha256 */
static const char ecdsa_secp192r1_privkey[] =
    "-----BEGIN EC PRIVATE KEY-----"
    "MF8CAQEEGLjezFcbgDMeApVrdtZHvu/k1a8/tVZ41KAKBggqhkjOPQMBAaE0AzIA"
    "BO1lciKdgxeRH8k64vxcaV1OYIK9akVrW02Dw21MXhRLP0l0wzCw6LGSr5rS6AaL"
    "Fg==" "-----END EC PRIVATE KEY-----";

static const char ecdsa_secp192r1_sig[] =
    "\x30\x34\x02\x18\x5f\xb3\x10\x4b\x4d\x44\x48\x29\x4b\xfd\xa7\x8e\xce\x57\xac\x36\x38\x54\xab\x73\xdb\xed\xb8\x5f\x02\x18\x0b\x8b\xf3\xae\x49\x50\x0e\x47\xca\x89\x1a\x00\xca\x23\xf5\x8d\xd6\xe3\xce\x9a\xff\x2e\x4f\x5c";

static const char ecdsa_secp224r1_privkey[] =
    "-----BEGIN EC PRIVATE KEY-----"
    "MGgCAQEEHOKWJFdWdrR/CgVrUeTeawOrJ9GozE9KKx2a8PmgBwYFK4EEACGhPAM6"
    "AAQKQj3YpenWT7lFR41SnBvmj/+Bj+kgzQnaF65qWAtPRJsZXFlLTu3/IUNqSRu9"
    "DqPsk8xBHAB7pA==" "-----END EC PRIVATE KEY-----";

static const char ecdsa_secp224r1_sig[] =
    "\x30\x3d\x02\x1c\x76\x03\x8d\x74\xf4\xd3\x09\x2a\xb5\xdf\x6b\x5b\xf4\x4b\x86\xb8\x62\x81\x5d\x7b\x7a\xbb\x37\xfc\xf1\x46\x1c\x2b\x02\x1d\x00\xa0\x98\x5d\x80\x43\x89\xe5\xee\x1a\xec\x46\x08\x04\x55\xbc\x50\xfa\x2a\xd5\xa6\x18\x92\x19\xdb\x68\xa0\x2a\xda";
#endif

static const char ecdsa_secp384r1_privkey[] =
    "-----BEGIN EC PRIVATE KEY-----"
    "MIGkAgEBBDDevshD6gb+4rZpC9vwFcIwNs4KmGzdqCxyyN40a8uOWRbyf7aHdiSS"
    "03oAyKtc4JCgBwYFK4EEACKhZANiAARO1KkPMno2tnNXx1S9EZkp8SOpDCZ4aobH"
    "IYv8RHnSmKf8I3OKD6TaoeR+1MwJmNJUH90Bj45WXla68/vsPiFcfVKboxsZYe/n"
    "pv8e4ugXagVQVBXNZJ859iYPdJR24vo=" "-----END EC PRIVATE KEY-----";

static const char ecdsa_secp384r1_sig[] =
    "\x30\x66\x02\x31\x00\xbb\x4d\x25\x30\x13\x1b\x3b\x75\x60\x07\xed\x53\x8b\x52\xee\xd8\x6e\xf1\x9d\xa8\x36\x0e\x2e\x20\x31\x51\x11\x48\x78\xdd\xaf\x24\x38\x64\x81\x71\x6b\xa6\xb7\x29\x58\x28\x82\x32\xba\x29\x29\xd9\x02\x31\x00\xeb\x70\x09\x87\xac\x7b\x78\x0d\x4c\x4f\x08\x2b\x86\x27\xe2\x60\x1f\xc9\x11\x9f\x1d\xf5\x82\x4c\xc7\x3d\xb0\x27\xc8\x93\x29\xc7\xd0\x0e\x88\x02\x09\x93\xc2\x72\xce\xa5\x74\x8c\x3d\xe0\x8c\xad";

static const char ecdsa_secp521r1_privkey[] =
    "-----BEGIN EC PRIVATE KEY-----"
    "MIHbAgEBBEGO2n7NN363qSCvJVdlQtCvudtaW4o0fEufXRjE1AsCrle+VXX0Zh0w"
    "Y1slSeDHMndpakoiF+XkQ+bhcB867UV6aKAHBgUrgQQAI6GBiQOBhgAEAQb6jDpo"
    "byy1tF8Zucg0TMGUzIN2DK+RZJ3QQRdWdirO25OIC3FoFi1Yird6rpoB6HlNyJ7R"
    "0bNG9Uv34bSHMn8yAFoiqxUCdJZQbEenMoZsi6COaePe3e0QqvDMr0hEWT23Sr3t"
    "LpEV7eZGFfFIJw5wSUp2KOcs+O9WjmoukTWtDKNV"
    "-----END EC PRIVATE KEY-----";

static const char ecdsa_secp521r1_sig[] =
    "\x30\x81\x87\x02\x42\x01\xb8\xcb\x52\x9e\x10\xa8\x49\x3f\xe1\x9e\x14\x0a\xcf\x96\xed\x7e\xab\x7d\x0c\xe1\x9b\xa4\x97\xdf\x01\xf5\x35\x42\x5f\x5b\x28\x15\x24\x33\x6e\x59\x6c\xaf\x10\x8b\x98\x8e\xe9\x4c\x23\x0d\x76\x92\x03\xdd\x6d\x8d\x08\x47\x15\x5b\xf8\x66\x75\x75\x40\xe8\xf4\xa0\x52\x02\x41\x15\x27\x7c\x5f\xa6\x33\xa6\x29\x68\x3f\x55\x8d\x7f\x1d\x4f\x88\xc6\x61\x6e\xac\x21\xdf\x2b\x7b\xde\x76\x9a\xdc\xe6\x3b\x94\x3f\x03\x9c\xa2\xa6\xa3\x63\x39\x48\xbd\x79\x70\x21\xf2\x6b\xff\x58\x66\xf1\x58\xc2\x58\xad\x4f\x84\x14\x5d\x05\x12\x83\xd0\x87\xbd\xf3";

/* DSA key and signature */
static const char dsa_privkey[] =
 "-----BEGIN DSA PRIVATE KEY-----\n"
 "MIIDTQIBAAKCAQEAh60B6yPMRIT7udq2kKuwnQDohvT1U0w+RJcSr23C05cM/Ovn\n"
 "UP/8Rrj6T8K+uYhMbKgLaZiJJW9q04jaPQk0cfUphbLvRjzVHwE/0Bkb+Y1Rv7ni\n"
 "Jot2IFMq5iuNraf889PC0WREvFCcIkSFY2Ac4WT7mCcBtfx/raGFXDUjcUrJ0HwZ\n"
 "IOhjQDfcXUsztuyYsYA75ociEY8kyDZq/ixyr5++R1VjNf30Re8AbQlXOEGxEN5t\n"
 "t+Tvpq8K5L3prQs2KNSzyOUmedjb/ojH4T4qe/RL9EVjjeuIGHDNUT6F197yZ91y\n"
 "qLLTf1WjnUyZcKij5rryX0LJBBWawEZjNSHZawIdAMQlyycia4NigCdiDR+QptUn\n"
 "2xrj9o14fXkIrXcCggEAXRZm1rbPhsjSTo6cpCVrmDzO1grv83EHiBH4MvRQQnP8\n"
 "FpAREsBA5cYju97XvLaLhioZeMjLn08kU7TUbHRUB+ULTuVvE2dQbBpGuKiLRRt9\n"
 "6U2T0eD3xGLoM+o8EY/kpqaWGEpZv7hzM9xuo4vy55+viAZgFWULqmltwfG/7w7V\n"
 "NXUHNv5H4Ipw//fSDLTPqzUlNqSSswDLz6pCjWEs0rWAqNAMaOiLTz4id9pL48Oe\n"
 "oAfpcQR9tgTEnwyXfZBnrJVclHhkHKGeXvU05IgCzpKO76Z5R+By50T0i/JV7vzM\n"
 "l2yS9aAl/cprT6U7yI3oU/blldCVNpMcFAFb+fO8DAKCAQBVMo8xptyvQOJeSvbO\n"
 "SSYdJ3IiI/0GdkcGWXblWg9z7mrPaWEnT7OquEm/+vYtWd3GHDtyNM+jzsN4Xgjc\n"
 "TL3AEd2hLiozJQ1BFKw25VU08UHAYTzUxZhO4Vwtmp46Kwj8YLDQ3NHRWCBxpDQR\n"
 "fbiFvyXP+qXap6plMfrydnUD1mae/JSOWOYgdB7tFIehstLxVXx/cAnjwgFU03Df\n"
 "grjsad92zA1Hc9wIjbsgAQdTR5DWnFRkRt3UtayBwoyqm6QceZHsv1NAGvkQ4ion\n"
 "bEjkHkjF9YCkR9/rspR8cLghRIXMjOpypuSbaRPeeWq0gP2UOxFL/d3iWH0ETr/L\n"
 "kTlCAhxYGpVgtfB96qmJukyl9GOGvfkwFTgEyIDoV84M\n"
 "-----END DSA PRIVATE KEY-----\n";

static const char dsa_sig[] =
    "\x30\x3d\x02\x1c\x2e\x40\x14\xb3\x7a\x3f\xc0\x4f\x06\x74\x4f\xa6\x5f\xc2\x0a\x46\x35\x38\x88\xb4\x1a\xcf\x94\x02\x40\x42\x7c\x7f\x02\x1d\x00\x98\xfc\xf1\x08\x66\xf1\x86\x28\xc9\x73\x9e\x2b\x5d\xce\x57\xe8\xb5\xeb\xcf\xa3\xf6\x60\xf6\x63\x16\x0e\xc0\x42";

static int test_known_sig(gnutls_pk_algorithm_t pk, unsigned bits,
			  gnutls_digest_algorithm_t dig,
			  const void *privkey, size_t privkey_size,
			  const void *stored_sig, size_t stored_sig_size,
			  unsigned deterministic_sigs)
{
	int ret;
	gnutls_datum_t sig = { NULL, 0 };
	gnutls_datum_t t, ssig;
	gnutls_pubkey_t pub = NULL;
	gnutls_privkey_t key;
	char param_name[32];

	if (pk == GNUTLS_PK_EC) {
		snprintf(param_name, sizeof(param_name), "%s",
			 gnutls_ecc_curve_get_name(GNUTLS_BITS_TO_CURVE
						   (bits)));
	} else {
		snprintf(param_name, sizeof(param_name), "%u", bits);
	}

	ret = gnutls_privkey_init(&key);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = gnutls_pubkey_init(&pub);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	t.data = (void *) privkey;
	t.size = privkey_size;

	ret =
	    gnutls_privkey_import_x509_raw(key, &t, GNUTLS_X509_FMT_PEM,
					   NULL, 0);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	if (pk != (unsigned) gnutls_privkey_get_pk_algorithm(key, NULL)) {
		ret = GNUTLS_E_SELF_TEST_ERROR;
		goto cleanup;
	}

	/* Test if the signature we generate matches the stored */
	ssig.data = (void *) stored_sig;
	ssig.size = stored_sig_size;

	if (deterministic_sigs != 0) {	/* do not compare against stored signature if not provided */
		ret =
		    gnutls_privkey_sign_data(key, dig, 0, &signed_data,
					     &sig);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		if (sig.size != ssig.size
		    || memcmp(sig.data, ssig.data, sig.size) != 0) {
			ret = GNUTLS_E_SELF_TEST_ERROR;
#if 0
			unsigned i;
			fprintf(stderr, "\nstored[%d]: ", ssig.size);
			for (i = 0; i < ssig.size; i++)
				fprintf(stderr, "\\x%.2x", ssig.data[i]);

			fprintf(stderr, "\ngenerated[%d]: ", sig.size);
			for (i = 0; i < sig.size; i++)
				fprintf(stderr, "\\x%.2x", sig.data[i]);
			fprintf(stderr, "\n");
#endif
			gnutls_assert();
			goto cleanup;
		}
	}

	/* Test if we can verify the signature */

	ret = gnutls_pubkey_import_privkey(pub, key, 0, 0);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret =
	    gnutls_pubkey_verify_data2(pub, gnutls_pk_to_sign(pk, dig), 0,
				       &signed_data, &ssig);
	if (ret < 0) {
		ret = GNUTLS_E_SELF_TEST_ERROR;
		gnutls_assert();
		goto cleanup;
	}

	/* Test if a broken signature will cause verification error */

	ret =
	    gnutls_pubkey_verify_data2(pub, gnutls_pk_to_sign(pk, dig), 0,
				       &bad_data, &ssig);

	if (ret != GNUTLS_E_PK_SIG_VERIFY_FAILED) {
		ret = GNUTLS_E_SELF_TEST_ERROR;
		gnutls_assert();
		goto cleanup;
	}

	ret = 0;

      cleanup:
	gnutls_free(sig.data);
	if (pub != 0)
		gnutls_pubkey_deinit(pub);
	gnutls_privkey_deinit(key);

	if (ret == 0)
		_gnutls_debug_log("%s-%s-known-sig self test succeeded\n",
				  gnutls_pk_get_name(pk), param_name);
	else
		_gnutls_debug_log("%s-%s-known-sig self test failed\n",
				  gnutls_pk_get_name(pk), param_name);

	return ret;
}

#define PK_TEST(pk, func, bits, dig) \
			ret = func(pk, bits, dig); \
			if (ret < 0) { \
				gnutls_assert(); \
				goto cleanup; \
			} \
			if (!(flags & GNUTLS_SELF_TEST_FLAG_ALL)) \
				return 0

#define PK_KNOWN_TEST(pk, det, bits, dig, pkey, sig) \
			ret = test_known_sig(pk, bits, dig, pkey, sizeof(pkey)-1, sig, sizeof(sig)-1, det); \
			if (ret < 0) { \
				gnutls_assert(); \
				goto cleanup; \
			} \
			if (!(flags & GNUTLS_SELF_TEST_FLAG_ALL)) \
				return 0


/* This file is also included by the test app in tests/slow/cipher-test, so in that
 * case we cannot depend on gnutls internals */
#ifndef AVOID_INTERNALS
/* Known answer tests for DH */
static int test_dh(void)
{
	int ret;
	gnutls_pk_params_st priv;
	gnutls_pk_params_st pub;
	gnutls_datum_t out = {NULL, 0};
	static const uint8_t known_dh_k[] = {
		0x10, 0x25, 0x04, 0xb5, 0xc6, 0xc2, 0xcb, 
		0x0c, 0xe9, 0xc5, 0x58, 0x0d, 0x22, 0x62};
	static const uint8_t test_p[] = {
		0x24, 0x85, 0xdd, 0x3a, 0x74, 0x42, 0xe4, 
		0xb3, 0xf1, 0x0b, 0x13, 0xf9, 0x17, 0x4d };
	static const uint8_t test_g[] = { 0x02 };
	static const uint8_t test_x[] = {
		0x06, 0x2c, 0x96, 0xae, 0x0e, 0x9e, 0x9b, 
		0xbb, 0x41, 0x51, 0x7a, 0xa7, 0xc5, 0xfe };
	static const uint8_t test_y[] = { /* y=g^x mod p */
		0x1e, 0xca, 0x23, 0x2a, 0xfd, 0x34, 0xe1, 
		0x10, 0x7a, 0xff, 0xaf, 0x2d, 0xaa, 0x53 };

	gnutls_pk_params_init(&priv);
	gnutls_pk_params_init(&pub);
	
	priv.algo = pub.algo = GNUTLS_PK_DH;
	
	ret = _gnutls_mpi_init_scan(&priv.params[DH_P], test_p, sizeof(test_p));
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = _gnutls_mpi_init_scan(&priv.params[DH_G], test_g, sizeof(test_g));
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = _gnutls_mpi_init_scan(&priv.params[DH_X], test_x, sizeof(test_x));
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = _gnutls_mpi_init_scan(&pub.params[DH_Y], test_y, sizeof(test_y));
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	/* check whether Y^X mod p is the expected value */
	ret = _gnutls_pk_derive(GNUTLS_PK_DH, &out, &priv, &pub);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	if (sizeof(known_dh_k) != out.size) {
		ret = GNUTLS_E_SELF_TEST_ERROR;
		gnutls_assert();
		goto cleanup;
	}

	if (memcmp(out.data, known_dh_k, out.size) != 0) {
		ret = GNUTLS_E_SELF_TEST_ERROR;
		gnutls_assert();
		goto cleanup;
	}
	
	
	ret = 0;
cleanup:
	_gnutls_mpi_release(&pub.params[DH_Y]);
	_gnutls_mpi_release(&priv.params[DH_G]);
	_gnutls_mpi_release(&priv.params[DH_P]);
	_gnutls_mpi_release(&priv.params[DH_X]);
	gnutls_free(out.data);

	if (ret < 0) {
		_gnutls_debug_log("DH self test failed\n");
	} else {
		_gnutls_debug_log("DH self test succeeded\n");
	}

	return ret;
}

/* Known answer tests for DH */
static int test_ecdh(void)
{
	int ret;
	gnutls_pk_params_st priv;
	gnutls_pk_params_st pub;
	gnutls_datum_t out = {NULL, 0};
	static const uint8_t known_key[] = { 
		0x22, 0x7a, 0x95, 0x98, 0x5f, 0xb1, 0x25, 0x79, 
		0xee, 0x07, 0xe3, 0x8b, 0x1a, 0x97, 0x1d, 0x63, 
		0x53, 0xa8, 0xbd, 0xde, 0x67, 0x4b, 0xcf, 0xa4, 
		0x5f, 0x5e, 0x67, 0x27, 0x6d, 0x86, 0x27, 0x26 };
	static const uint8_t test_k[] = { /* priv */
		0x52, 0x9c, 0x30, 0xac, 0x6b, 0xce, 0x71, 0x9a, 
		0x37, 0xcd, 0x40, 0x93, 0xbf, 0xf0, 0x36, 0x89, 
		0x53, 0xcc, 0x0e, 0x17, 0xc6, 0xb6, 0xe2, 0x6a, 
		0x3c, 0x2c, 0x51, 0xdb, 0xa6, 0x69, 0x8c, 0xb1 };
	static const uint8_t test_x[] = {
		0x51, 0x35, 0xd1, 0xd2, 0xb6, 0xad, 0x13, 0xf4, 
		0xa2, 0x25, 0xd3, 0x85, 0x83, 0xbe, 0x42, 0x1e, 
		0x19, 0x09, 0x54, 0x39, 0x00, 0x46, 0x91, 0x49, 
		0x0f, 0x3f, 0xaf, 0x3f, 0x67, 0xda, 0x10, 0x6f };
	static const uint8_t test_y[] = { /* y=g^x mod p */
		0x07, 0x3a, 0xa1, 0xa2, 0x47, 0x3d, 0xa2, 0x74, 
		0x74, 0xc2, 0xde, 0x62, 0xb6, 0xb9, 0x59, 0xc9, 
		0x56, 0xf6, 0x9e, 0x17, 0xea, 0xbf, 0x7d, 0xa1, 
		0xd7, 0x65, 0xd6, 0x7b, 0xac, 0xca, 0xd5, 0xe3 };
	gnutls_pk_params_init(&priv);
	gnutls_pk_params_init(&pub);
	
	priv.flags = GNUTLS_ECC_CURVE_SECP256R1;
	pub.flags = GNUTLS_ECC_CURVE_SECP256R1;
	
	priv.algo = pub.algo = GNUTLS_PK_EC;
	
	ret = _gnutls_mpi_init_scan(&priv.params[ECC_K], test_k, sizeof(test_k));
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = _gnutls_mpi_init_scan(&priv.params[ECC_X], test_x, sizeof(test_x));
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = _gnutls_mpi_init_scan(&priv.params[ECC_Y], test_y, sizeof(test_y));
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = _gnutls_mpi_init_scan(&pub.params[ECC_X], test_x, sizeof(test_x));
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = _gnutls_mpi_init_scan(&pub.params[ECC_Y], test_y, sizeof(test_y));
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	/* check whether Y^X mod p is the expected value */
	ret = _gnutls_pk_derive(GNUTLS_PK_EC, &out, &priv, &pub);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	if (sizeof(known_key) != out.size) {
		ret = GNUTLS_E_SELF_TEST_ERROR;
		gnutls_assert();
		goto cleanup;
	}

	if (memcmp(out.data, known_key, out.size) != 0) {
		ret = GNUTLS_E_SELF_TEST_ERROR;
		gnutls_assert();
		goto cleanup;
	}
	
	ret = 0;
cleanup:
	_gnutls_mpi_release(&pub.params[ECC_Y]);
	_gnutls_mpi_release(&pub.params[ECC_X]);
	_gnutls_mpi_release(&priv.params[ECC_K]);
	_gnutls_mpi_release(&priv.params[ECC_X]);
	_gnutls_mpi_release(&priv.params[ECC_Y]);
	gnutls_free(out.data);

	if (ret < 0) {
		_gnutls_debug_log("ECDH self test failed\n");
	} else {
		_gnutls_debug_log("ECDH self test succeeded\n");
	}

	return ret;
}
#endif

/*-
 * gnutls_pk_self_test:
 * @flags: GNUTLS_SELF_TEST_FLAG flags
 * @pk: the algorithm to use
 *
 * This function will run self tests on the provided public key algorithm.
 *
 * Returns: Zero or a negative error code on error.
 *
 * Since: 3.3.0-FIPS140
 -*/
int gnutls_pk_self_test(unsigned flags, gnutls_pk_algorithm_t pk)
{
	int ret;

	if (flags & GNUTLS_SELF_TEST_FLAG_ALL)
		pk = GNUTLS_PK_UNKNOWN;

	switch (pk) {
	case GNUTLS_PK_UNKNOWN:
		FALLTHROUGH;
	case GNUTLS_PK_DH:
#ifndef AVOID_INTERNALS
		ret = test_dh();
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		if (!(flags & GNUTLS_SELF_TEST_FLAG_ALL))
			return 0;
#endif
		FALLTHROUGH;
	case GNUTLS_PK_RSA:
		PK_KNOWN_TEST(GNUTLS_PK_RSA, 1, 2048, GNUTLS_DIG_SHA256,
			      rsa_key2048, rsa_sig);
		PK_TEST(GNUTLS_PK_RSA, test_rsa_enc, 2048, 0);
		PK_TEST(GNUTLS_PK_RSA, test_sig, 3072, GNUTLS_DIG_SHA256);

		if (!(flags & GNUTLS_SELF_TEST_FLAG_ALL))
			return 0;

		FALLTHROUGH;
	case GNUTLS_PK_DSA:
		PK_KNOWN_TEST(GNUTLS_PK_DSA, 0, 2048, GNUTLS_DIG_SHA256,
			      dsa_privkey, dsa_sig);
		PK_TEST(GNUTLS_PK_DSA, test_sig, 3072, GNUTLS_DIG_SHA256);

		if (!(flags & GNUTLS_SELF_TEST_FLAG_ALL))
			return 0;

		FALLTHROUGH;
	case GNUTLS_PK_EC:	/* Testing ECDSA */
		/* Test ECDH */
#ifndef AVOID_INTERNALS
		ret = test_ecdh();
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		if (!(flags & GNUTLS_SELF_TEST_FLAG_ALL))
			return 0;
#endif

		/* Test ECDSA */
#ifdef ENABLE_NON_SUITEB_CURVES
		PK_KNOWN_TEST(GNUTLS_PK_EC, 0,
			      GNUTLS_CURVE_TO_BITS
			      (GNUTLS_ECC_CURVE_SECP192R1),
			      GNUTLS_DIG_SHA256, ecdsa_secp192r1_privkey,
			      ecdsa_secp192r1_sig);
		PK_TEST(GNUTLS_PK_EC, test_sig,
			GNUTLS_CURVE_TO_BITS(GNUTLS_ECC_CURVE_SECP192R1),
			GNUTLS_DIG_SHA256);

		PK_KNOWN_TEST(GNUTLS_PK_EC, 0,
			      GNUTLS_CURVE_TO_BITS
			      (GNUTLS_ECC_CURVE_SECP224R1),
			      GNUTLS_DIG_SHA256, ecdsa_secp224r1_privkey,
			      ecdsa_secp224r1_sig);
		PK_TEST(GNUTLS_PK_EC, test_sig,
			GNUTLS_CURVE_TO_BITS(GNUTLS_ECC_CURVE_SECP224R1),
			GNUTLS_DIG_SHA256);
#endif
		PK_KNOWN_TEST(GNUTLS_PK_EC, 0,
			      GNUTLS_CURVE_TO_BITS
			      (GNUTLS_ECC_CURVE_SECP256R1),
			      GNUTLS_DIG_SHA256, ecdsa_secp256r1_privkey,
			      ecdsa_secp256r1_sig);
		PK_TEST(GNUTLS_PK_EC, test_sig,
			GNUTLS_CURVE_TO_BITS(GNUTLS_ECC_CURVE_SECP256R1),
			GNUTLS_DIG_SHA256);

		PK_KNOWN_TEST(GNUTLS_PK_EC, 0,
			      GNUTLS_CURVE_TO_BITS
			      (GNUTLS_ECC_CURVE_SECP384R1),
			      GNUTLS_DIG_SHA256, ecdsa_secp384r1_privkey,
			      ecdsa_secp384r1_sig);
		PK_TEST(GNUTLS_PK_EC, test_sig,
			GNUTLS_CURVE_TO_BITS(GNUTLS_ECC_CURVE_SECP384R1),
			GNUTLS_DIG_SHA384);

		PK_KNOWN_TEST(GNUTLS_PK_EC, 0,
			      GNUTLS_CURVE_TO_BITS
			      (GNUTLS_ECC_CURVE_SECP521R1),
			      GNUTLS_DIG_SHA512, ecdsa_secp521r1_privkey,
			      ecdsa_secp521r1_sig);
		PK_TEST(GNUTLS_PK_EC, test_sig,
			GNUTLS_CURVE_TO_BITS(GNUTLS_ECC_CURVE_SECP521R1),
			GNUTLS_DIG_SHA512);

		break;
	default:
		return gnutls_assert_val(GNUTLS_E_NO_SELF_TEST);
	}

	ret = 0;

      cleanup:
	return ret;
}
