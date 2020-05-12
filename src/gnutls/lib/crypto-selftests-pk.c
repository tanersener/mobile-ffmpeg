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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>
 *
 */

#include "gnutls_int.h"
#include "errors.h"
#include "fips.h"
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

/* RSA 2048 private key and signature */
static const char rsa_2048_privkey[] =
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

static const char rsa_2048_sig[] =
	"\x7a\xb3\xf8\xb0\xf9\xf0\x52\x88\x37\x17\x97\x9f\xbe\x61\xb4\xd2"
	"\x43\x78\x9f\x79\x92\xd0\xad\x08\xdb\xbd\x3c\x72\x7a\xb5\x51\x59"
	"\x63\xd6\x7d\xf1\x9c\x1e\x10\x7b\x27\xab\xf8\xd4\x9d\xcd\xc5\xf9"
	"\xae\xf7\x09\x6b\x40\x93\xc5\xe9\x1c\x0f\xb4\x82\xa1\x47\x86\x54"
	"\x63\xd2\x4d\x40\x9a\x80\xb9\x38\x45\x69\xa2\xd6\x92\xb6\x69\x7f"
	"\x3f\xf3\x5b\xa5\x1d\xac\x06\xad\xdf\x4e\xbb\xe6\xda\x68\x0d\xe5"
	"\xab\xef\xd2\xf0\xc5\xd8\xc0\xed\x80\xe2\xd4\x76\x98\xec\x44\xa2"
	"\xfc\x3f\xce\x2e\x8b\xc4\x4b\xab\xb0\x70\x24\x52\x85\x2a\x36\xcd"
	"\x9a\xb5\x05\x00\xea\x98\x7c\x72\x06\x68\xb1\x38\x44\x16\x80\x6a"
	"\x3b\x64\x72\xbb\xfd\x4b\xc9\xdd\xda\x2a\x68\xde\x7f\x6e\x48\x28"
	"\xc1\x63\x57\x2b\xde\x83\xa3\x27\x34\xd7\xa6\x87\x18\x35\x10\xff"
	"\x31\xd9\x47\xc9\x84\x35\xe1\xaa\xe2\xf7\x98\xfa\x19\xd3\xf1\x94"
	"\x25\x2a\x96\xe4\xa8\xa7\x05\x10\x93\x87\xde\x96\x85\xe5\x68\xb8"
	"\xe5\x4e\xbf\x66\x85\x91\xbd\x52\x5b\x3d\x9f\x1b\x79\xea\xe3\x8b"
	"\xef\x62\x18\x39\x7a\x50\x01\x46\x1b\xde\x8d\x37\xbc\x90\x6c\x07"
	"\xc0\x07\xed\x60\xce\x2e\x31\xd6\x8f\xe8\x75\xdb\x45\x21\xc6\xcb";

/* DSA 2048 private key and signature */
static const char dsa_2048_privkey[] =
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

static const char dsa_2048_sig[] =
	"\x30\x3d\x02\x1d\x00\xbe\x87\x2f\xcf\xa1\xe4\x86\x5c\x72\x58\x4a"
	"\x7b\x8f\x32\x7f\xa5\x1b\xdc\x5c\xae\xda\x98\xea\x15\x32\xed\x0c"
	"\x4e\x02\x1c\x4c\x76\x01\x2b\xcd\xb9\x33\x95\xf2\xfa\xde\x56\x01"
	"\xb7\xaa\xe4\x5a\x4a\x2e\xf1\x24\x5a\xd1\xb5\x83\x9a\x93\x61";

/* secp256r1 private key and signature */
static const char ecdsa_secp256r1_privkey[] =
	"-----BEGIN EC PRIVATE KEY-----\n"
	"MHcCAQEEIPAKWV7+pZe9c5EubMNfAEKWRQtP/MvlO9HehwHmJssNoAoGCCqGSM49\n"
	"AwEHoUQDQgAE2CNONRio3ciuXtoomJKs3MdbzLbd44VPhtzJN30VLFm5gvnfiCj2\n"
	"zzz7pl9Cv0ECHl6yedNI8QEKdcwCDgEmkQ==\n"
	"-----END EC PRIVATE KEY-----\n";

static const char ecdsa_secp256r1_sig[] =
	"\x30\x45\x02\x21\x00\x80\x67\x18\xb9\x72\xc6\x0b\xe1\xc9\x89\x9b"
	"\x85\x11\x49\x29\x08\xd9\x86\x76\xcc\xfb\xc1\xf4\xd0\xa2\x5e\xa7"
	"\xb9\x12\xfb\x1a\x68\x02\x20\x67\x12\xb1\x89\x9e\x1d\x9d\x5c\x0f"
	"\xef\x6e\xa7\x2a\x95\x8c\xfa\x54\x20\x80\xc8\x30\x7c\xff\x06\xbc"
	"\xc8\xe2\x9a\x2f\x05\x2f\x67";

#ifdef ENABLE_NON_SUITEB_CURVES
/* secp192r1 private key and signature */
static const char ecdsa_secp192r1_privkey[] =
	"-----BEGIN EC PRIVATE KEY-----"
	"MF8CAQEEGLjezFcbgDMeApVrdtZHvu/k1a8/tVZ41KAKBggqhkjOPQMBAaE0AzIA"
	"BO1lciKdgxeRH8k64vxcaV1OYIK9akVrW02Dw21MXhRLP0l0wzCw6LGSr5rS6AaL"
	"Fg==" "-----END EC PRIVATE KEY-----";

static const char ecdsa_secp192r1_sig[] =
	"\x30\x34\x02\x18\x7c\x43\xe3\xb7\x26\x90\x43\xb5\xf5\x63\x8f\xee"
	"\xac\x78\x3d\xac\x35\x35\xd0\x1e\x83\x17\x2b\x64\x02\x18\x14\x6e"
	"\x94\xd5\x7e\xac\x43\x42\x0b\x71\x7a\xc8\x29\xe6\xe3\xda\xf2\x95"
	"\x0e\xe0\x63\x24\xed\xf2";

/* secp224r1 private key and signature */
static const char ecdsa_secp224r1_privkey[] =
	"-----BEGIN EC PRIVATE KEY-----"
	"MGgCAQEEHOKWJFdWdrR/CgVrUeTeawOrJ9GozE9KKx2a8PmgBwYFK4EEACGhPAM6"
	"AAQKQj3YpenWT7lFR41SnBvmj/+Bj+kgzQnaF65qWAtPRJsZXFlLTu3/IUNqSRu9"
	"DqPsk8xBHAB7pA==" "-----END EC PRIVATE KEY-----";

static const char ecdsa_secp224r1_sig[] =
	"\x30\x3d\x02\x1c\x14\x22\x09\xa1\x51\x33\x37\xfd\x78\x73\xbd\x84"
	"\x6e\x76\xa8\x60\x90\xf5\xb6\x57\x34\x25\xe0\x79\xe3\x01\x61\xa9"
	"\x02\x1d\x00\xb1\xee\xdb\xae\xb3\xe6\x9c\x04\x68\xd5\xe1\x0d\xb6"
	"\xfc\x5c\x45\xc3\x4f\xbf\x2b\xa5\xe0\x89\x37\x84\x04\x82\x5f";
#endif

/* secp384r1 private key and signature */
static const char ecdsa_secp384r1_privkey[] =
	"-----BEGIN EC PRIVATE KEY-----"
	"MIGkAgEBBDDevshD6gb+4rZpC9vwFcIwNs4KmGzdqCxyyN40a8uOWRbyf7aHdiSS"
	"03oAyKtc4JCgBwYFK4EEACKhZANiAARO1KkPMno2tnNXx1S9EZkp8SOpDCZ4aobH"
	"IYv8RHnSmKf8I3OKD6TaoeR+1MwJmNJUH90Bj45WXla68/vsPiFcfVKboxsZYe/n"
	"pv8e4ugXagVQVBXNZJ859iYPdJR24vo=" "-----END EC PRIVATE KEY-----";

static const char ecdsa_secp384r1_sig[] =
	"\x30\x65\x02\x31\x00\xa7\x73\x60\x16\xdb\xf9\x1f\xfc\x9e\xd2\x12"
	"\x23\xd4\x04\xa7\x31\x1f\x15\x28\xfd\x87\x9c\x2c\xb1\xf3\x38\x35"
	"\x23\x3b\x6e\xfe\x6a\x5d\x89\x34\xbe\x02\x82\xc6\x27\xea\x45\x53"
	"\xa9\x87\xc5\x31\x0a\x02\x30\x76\x32\x80\x6b\x43\x3c\xb4\xfd\x90"
	"\x03\xe0\x1d\x5d\x77\x18\x45\xf6\x71\x29\xa9\x05\x87\x49\x75\x3a"
	"\x78\x9c\x49\xe5\x6c\x8e\x18\xcd\x5d\xee\x2c\x6f\x92\xf7\x15\xd3"
	"\x38\xd5\xf9\x9b\x9d\x1a\xf4";

/* secp521r1 private key and signature */
static const char ecdsa_secp521r1_privkey[] =
	"-----BEGIN EC PRIVATE KEY-----"
	"MIHbAgEBBEGO2n7NN363qSCvJVdlQtCvudtaW4o0fEufXRjE1AsCrle+VXX0Zh0w"
	"Y1slSeDHMndpakoiF+XkQ+bhcB867UV6aKAHBgUrgQQAI6GBiQOBhgAEAQb6jDpo"
	"byy1tF8Zucg0TMGUzIN2DK+RZJ3QQRdWdirO25OIC3FoFi1Yird6rpoB6HlNyJ7R"
	"0bNG9Uv34bSHMn8yAFoiqxUCdJZQbEenMoZsi6COaePe3e0QqvDMr0hEWT23Sr3t"
	"LpEV7eZGFfFIJw5wSUp2KOcs+O9WjmoukTWtDKNV"
	"-----END EC PRIVATE KEY-----";

static const char ecdsa_secp521r1_sig[] =
	"\x30\x81\x88\x02\x42\x01\x9d\x13\x2e\xc9\x75\x1b\x60\x10\x62\xc5"
	"\x0d\xcb\x08\x9e\x86\x01\xd3\xc9\x8c\xee\x2e\x16\x3d\x8c\xc2\x65"
	"\x80\xe1\x32\x56\xc3\x02\x9d\xf0\x4a\x89\x8d\x2e\x33\x2a\x90\x4e"
	"\x72\x1d\xaa\x84\x14\xe8\xcb\xdf\x7a\x4a\xc9\x67\x2e\xba\xa3\xf2"
	"\xc2\x07\xf7\x1b\xa5\x91\xbd\x02\x42\x01\xe3\x32\xd2\x25\xeb\x2e"
	"\xaf\xb4\x6c\xc0\xaa\x5c\xc1\x56\x14\x13\x23\x7f\x62\xcf\x4c\xb8"
	"\xd1\x96\xe0\x29\x6d\xed\x74\xdd\x23\x64\xf9\x29\x86\x40\x22\x2f"
	"\xb6\x8d\x4c\x8e\x0b\x7a\xda\xdb\x03\x44\x01\x9b\x81\x1c\x3c\xab"
	"\x78\xee\xf2\xc5\x24\x33\x61\x65\x01\x87\x66";

/* GOST01 private key */
static const char gost01_privkey[] =
	"-----BEGIN PRIVATE KEY-----\n"
	"MEUCAQAwHAYGKoUDAgITMBIGByqFAwICIwEGByqFAwICHgEEIgQgdNfuHGmmTdPm\n"
	"p5dAa3ea9UYxpdYQPP9lbDwzQwG2bJM=\n"
	"-----END PRIVATE KEY-----\n";

/* GOST12 256 private key */
static const char gost12_256_privkey[] =
	"-----BEGIN PRIVATE KEY-----\n"
	"MEgCAQAwHwYIKoUDBwEBAQEwEwYHKoUDAgIjAQYIKoUDBwEBAgIEIgQgKOF96tom\n"
	"D61rhSnzKjyrmO3fv0gdlHei+6ovrc8SnBk=\n"
	"-----END PRIVATE KEY-----\n";

/* GOST12 512 private key */
static const char gost12_512_privkey[] =
	"-----BEGIN PRIVATE KEY-----\n"
	"MGoCAQAwIQYIKoUDBwEBAQIwFQYJKoUDBwECAQIBBggqhQMHAQECAwRCBECjFpvp\n"
	"B0vdc7u59b99TCNXhHiB69JJtUjvieNkGYJpoaaIvoKZTNCjpSZASsZcQZCHOTof\n"
	"hsQ3JCCy4xnd5jWT\n"
	"-----END PRIVATE KEY-----\n";

static int test_rsa_enc(gnutls_pk_algorithm_t pk,
			unsigned bits, gnutls_digest_algorithm_t ign)
{
	int ret;
	gnutls_datum_t enc = { NULL, 0 };
	gnutls_datum_t dec = { NULL, 0 };
	gnutls_datum_t raw_rsa_key = { (void*)rsa_2048_privkey, sizeof(rsa_2048_privkey)-1 };
	gnutls_privkey_t key;
	gnutls_pubkey_t pub = NULL;
	unsigned char plaintext2[sizeof(DATASTR) - 1];

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

	ret = gnutls_privkey_decrypt_data2(key, 0, &enc, plaintext2,
					   signed_data.size);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}
	if (memcmp(plaintext2, signed_data.data, signed_data.size) != 0) {
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
		    unsigned bits, gnutls_sign_algorithm_t sigalgo)
{
	int ret;
	gnutls_privkey_t key;
	gnutls_datum_t raw_key;
	gnutls_datum_t sig = { NULL, 0 };
	gnutls_pubkey_t pub = NULL;
	char param_name[32];

	ret = gnutls_privkey_init(&key);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = gnutls_pubkey_init(&pub);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	switch(pk) {
		case GNUTLS_PK_RSA:
				raw_key.data = (void*)rsa_2048_privkey;
				raw_key.size = sizeof(rsa_2048_privkey) - 1;
				snprintf(param_name, sizeof(param_name), "%u", bits);
			break;
		case GNUTLS_PK_RSA_PSS:
				raw_key.data = (void*)rsa_2048_privkey;
				raw_key.size = sizeof(rsa_2048_privkey) - 1;
				snprintf(param_name, sizeof(param_name), "%u", bits);
			break;
		case GNUTLS_PK_DSA:
				raw_key.data = (void*)dsa_2048_privkey;
				raw_key.size = sizeof(dsa_2048_privkey) - 1;
				snprintf(param_name, sizeof(param_name), "%u", bits);
			break;
		case GNUTLS_PK_ECC:
			switch(bits) {
#ifdef ENABLE_NON_SUITEB_CURVES
				case GNUTLS_CURVE_TO_BITS(GNUTLS_ECC_CURVE_SECP192R1):
					raw_key.data = (void*)ecdsa_secp192r1_privkey;
					raw_key.size = sizeof(ecdsa_secp192r1_privkey) - 1;
					break;
				case GNUTLS_CURVE_TO_BITS(GNUTLS_ECC_CURVE_SECP224R1):
					raw_key.data = (void*)ecdsa_secp224r1_privkey;
					raw_key.size = sizeof(ecdsa_secp224r1_privkey) - 1;
					break;
#endif
				case GNUTLS_CURVE_TO_BITS(GNUTLS_ECC_CURVE_SECP256R1):
					raw_key.data = (void*)ecdsa_secp256r1_privkey;
					raw_key.size = sizeof(ecdsa_secp256r1_privkey) - 1;
					break;
				case GNUTLS_CURVE_TO_BITS(GNUTLS_ECC_CURVE_SECP384R1):
					raw_key.data = (void*)ecdsa_secp384r1_privkey;
					raw_key.size = sizeof(ecdsa_secp384r1_privkey) - 1;
					break;
				case GNUTLS_CURVE_TO_BITS(GNUTLS_ECC_CURVE_SECP521R1):
					raw_key.data = (void*)ecdsa_secp521r1_privkey;
					raw_key.size = sizeof(ecdsa_secp521r1_privkey) - 1;
					break;
				default:
					gnutls_assert();
					ret = GNUTLS_E_INTERNAL_ERROR;
					goto cleanup;
			}
			snprintf(param_name, sizeof(param_name), "%s",
					 gnutls_ecc_curve_get_name(GNUTLS_BITS_TO_CURVE
					 (bits)));
			break;
		case GNUTLS_PK_GOST_01:
			raw_key.data = (void*)gost01_privkey;
			raw_key.size = sizeof(gost01_privkey) - 1;
			snprintf(param_name, sizeof(param_name), "%s",
					 gnutls_ecc_curve_get_name(GNUTLS_BITS_TO_CURVE
					 (bits)));
			break;
		case GNUTLS_PK_GOST_12_256:
			raw_key.data = (void*)gost12_256_privkey;
			raw_key.size = sizeof(gost12_256_privkey) - 1;
			snprintf(param_name, sizeof(param_name), "%s",
					 gnutls_ecc_curve_get_name(GNUTLS_BITS_TO_CURVE
					 (bits)));
			break;
		case GNUTLS_PK_GOST_12_512:
			raw_key.data = (void*)gost12_512_privkey;
			raw_key.size = sizeof(gost12_512_privkey) - 1;
			snprintf(param_name, sizeof(param_name), "%s",
					 gnutls_ecc_curve_get_name(GNUTLS_BITS_TO_CURVE
					 (bits)));
			break;
		default:
			gnutls_assert();
			ret = GNUTLS_E_INTERNAL_ERROR;
			goto cleanup;
	}

	ret = gnutls_privkey_import_x509_raw(key, &raw_key, GNUTLS_X509_FMT_PEM, NULL, 0);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = gnutls_pubkey_import_privkey(pub, key, 0, 0);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = gnutls_privkey_sign_data2(key, sigalgo, 0, &signed_data, &sig);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret =
	    gnutls_pubkey_verify_data2(pub, sigalgo, 0,
				       &signed_data, &sig);
	if (ret < 0) {
		ret = GNUTLS_E_SELF_TEST_ERROR;
		gnutls_assert();
		goto cleanup;
	}

	ret =
	    gnutls_pubkey_verify_data2(pub, sigalgo, 0,
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

static int test_known_sig(gnutls_pk_algorithm_t pk, unsigned bits,
			  gnutls_digest_algorithm_t dig,
			  const void *privkey, size_t privkey_size,
			  const void *stored_sig, size_t stored_sig_size,
			  gnutls_privkey_flags_t flags)
{
	int ret;
	gnutls_datum_t sig = { NULL, 0 };
	gnutls_datum_t t, ssig;
	gnutls_pubkey_t pub = NULL;
	gnutls_privkey_t key;
	char param_name[32];

	if (pk == GNUTLS_PK_EC ||
	    pk == GNUTLS_PK_GOST_01 ||
	    pk == GNUTLS_PK_GOST_12_256 ||
	    pk == GNUTLS_PK_GOST_12_512)
	{
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

	ret = gnutls_privkey_sign_data(key, dig, flags, &signed_data, &sig);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	/* Test if the generated signature matches the stored */
	ssig.data = (void *) stored_sig;
	ssig.size = stored_sig_size;

	if (sig.size != ssig.size
		|| memcmp(sig.data, ssig.data, sig.size) != 0) {
		ret = GNUTLS_E_SELF_TEST_ERROR;
#if 0
		unsigned i;
		fprintf(stderr, "Algorithm: %s-%s\n",
				  gnutls_pk_get_name(pk), param_name);
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

	/* Test if we can verify the generated signature */

	ret = gnutls_pubkey_import_privkey(pub, key, 0, 0);
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

	/* Test if a broken signature will cause verification error */

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

#define PK_TEST(pk, func, bits, sigalgo) \
			ret = func(pk, bits, sigalgo); \
			if (ret < 0) { \
				gnutls_assert(); \
				goto cleanup; \
			}

#define PK_KNOWN_TEST(pk, bits, dig, pkey, sig, flags) \
			ret = test_known_sig(pk, bits, dig, pkey, sizeof(pkey)-1, sig, sizeof(sig)-1, flags); \
			if (ret < 0) { \
				gnutls_assert(); \
				goto cleanup; \
			}


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
	
	priv.curve = GNUTLS_ECC_CURVE_SECP256R1;
	pub.curve = GNUTLS_ECC_CURVE_SECP256R1;
	
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

	bool is_post = false;
	bool is_fips140_mode_enabled = false;

	if (flags & GNUTLS_SELF_TEST_FLAG_ALL)
		pk = GNUTLS_PK_UNKNOWN;

	if (_gnutls_get_lib_state() == LIB_STATE_SELFTEST)
		is_post = true;

	if (gnutls_fips140_mode_enabled())
		is_fips140_mode_enabled = true;

	switch (pk) {
	case GNUTLS_PK_UNKNOWN:
		FALLTHROUGH;
	case GNUTLS_PK_DH:
		ret = test_dh();
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		if (!(flags & GNUTLS_SELF_TEST_FLAG_ALL))
			return 0;
		FALLTHROUGH;
	case GNUTLS_PK_RSA:
		PK_KNOWN_TEST(GNUTLS_PK_RSA, 2048, GNUTLS_DIG_SHA256,
			      rsa_2048_privkey, rsa_2048_sig, 0);

		PK_TEST(GNUTLS_PK_RSA, test_rsa_enc, 2048, 0);

		if (!(flags & GNUTLS_SELF_TEST_FLAG_ALL))
			return 0;

		FALLTHROUGH;
	case GNUTLS_PK_RSA_PSS:
		PK_TEST(GNUTLS_PK_RSA_PSS, test_sig, 2048,
			GNUTLS_SIGN_RSA_PSS_RSAE_SHA256);

		if (!(flags & GNUTLS_SELF_TEST_FLAG_ALL))
			return 0;

		FALLTHROUGH;
	case GNUTLS_PK_DSA:
		if (is_post || !is_fips140_mode_enabled) {
			PK_KNOWN_TEST(GNUTLS_PK_DSA, 2048, GNUTLS_DIG_SHA256,
				      dsa_2048_privkey, dsa_2048_sig,
				      GNUTLS_PRIVKEY_FLAG_REPRODUCIBLE);
		} else {
			PK_TEST(GNUTLS_PK_DSA, test_sig, 2048,
				GNUTLS_SIGN_DSA_SHA256);
		}

		if (!(flags & GNUTLS_SELF_TEST_FLAG_ALL))
			return 0;

		FALLTHROUGH;
	case GNUTLS_PK_EC:
		/* Test ECDH and ECDSA */
		ret = test_ecdh();
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		/* Test ECDSA */
		if (is_post || !is_fips140_mode_enabled) {
			PK_KNOWN_TEST(GNUTLS_PK_EC,
					  GNUTLS_CURVE_TO_BITS(GNUTLS_ECC_CURVE_SECP256R1),
					  GNUTLS_DIG_SHA256, ecdsa_secp256r1_privkey,
					  ecdsa_secp256r1_sig, GNUTLS_PRIVKEY_FLAG_REPRODUCIBLE);
		} else {
			PK_TEST(GNUTLS_PK_EC, test_sig,
				GNUTLS_CURVE_TO_BITS(GNUTLS_ECC_CURVE_SECP256R1),
				GNUTLS_SIGN_ECDSA_SHA256);
		}

		if (!(flags & GNUTLS_SELF_TEST_FLAG_ALL))
			return 0;

		if (is_post || !is_fips140_mode_enabled) {
			PK_KNOWN_TEST(GNUTLS_PK_EC,
					  GNUTLS_CURVE_TO_BITS(GNUTLS_ECC_CURVE_SECP384R1),
					  GNUTLS_DIG_SHA384, ecdsa_secp384r1_privkey,
					  ecdsa_secp384r1_sig, GNUTLS_PRIVKEY_FLAG_REPRODUCIBLE);
		} else {
			PK_TEST(GNUTLS_PK_EC, test_sig,
				GNUTLS_CURVE_TO_BITS(GNUTLS_ECC_CURVE_SECP384R1),
				GNUTLS_SIGN_ECDSA_SHA384);
		}

		if (is_post || !is_fips140_mode_enabled) {
			PK_KNOWN_TEST(GNUTLS_PK_EC,
					  GNUTLS_CURVE_TO_BITS(GNUTLS_ECC_CURVE_SECP521R1),
					  GNUTLS_DIG_SHA512, ecdsa_secp521r1_privkey,
					  ecdsa_secp521r1_sig, GNUTLS_PRIVKEY_FLAG_REPRODUCIBLE);
		} else {
			PK_TEST(GNUTLS_PK_EC, test_sig,
				GNUTLS_CURVE_TO_BITS(GNUTLS_ECC_CURVE_SECP521R1),
				GNUTLS_SIGN_ECDSA_SHA512);
		}

#ifdef ENABLE_NON_SUITEB_CURVES
		if (is_post || !is_fips140_mode_enabled) {
			PK_KNOWN_TEST(GNUTLS_PK_EC,
					  GNUTLS_CURVE_TO_BITS(GNUTLS_ECC_CURVE_SECP192R1),
					  GNUTLS_DIG_SHA256, ecdsa_secp192r1_privkey,
					  ecdsa_secp192r1_sig, GNUTLS_PRIVKEY_FLAG_REPRODUCIBLE);
		} else {
			PK_TEST(GNUTLS_PK_EC, test_sig,
				GNUTLS_CURVE_TO_BITS(GNUTLS_ECC_CURVE_SECP192R1),
				GNUTLS_SIGN_ECDSA_SHA256);
		}

		if (is_post || !is_fips140_mode_enabled) {
			PK_KNOWN_TEST(GNUTLS_PK_EC,
					  GNUTLS_CURVE_TO_BITS(GNUTLS_ECC_CURVE_SECP224R1),
					  GNUTLS_DIG_SHA256, ecdsa_secp224r1_privkey,
					  ecdsa_secp224r1_sig, GNUTLS_PRIVKEY_FLAG_REPRODUCIBLE);
		} else {
			PK_TEST(GNUTLS_PK_EC, test_sig,
				GNUTLS_CURVE_TO_BITS(GNUTLS_ECC_CURVE_SECP224R1),
				GNUTLS_SIGN_ECDSA_SHA256);
		}
#endif


#if ENABLE_GOST
		FALLTHROUGH;
	case GNUTLS_PK_GOST_01:
		PK_TEST(GNUTLS_PK_GOST_01, test_sig,
			GNUTLS_CURVE_TO_BITS(GNUTLS_ECC_CURVE_GOST256CPA),
			GNUTLS_SIGN_GOST_94);

		if (!(flags & GNUTLS_SELF_TEST_FLAG_ALL))
			return 0;

		FALLTHROUGH;
	case GNUTLS_PK_GOST_12_256:
		PK_TEST(GNUTLS_PK_GOST_12_256, test_sig,
			GNUTLS_CURVE_TO_BITS(GNUTLS_ECC_CURVE_GOST256CPA),
			GNUTLS_SIGN_GOST_256);

		if (!(flags & GNUTLS_SELF_TEST_FLAG_ALL))
			return 0;

		FALLTHROUGH;
	case GNUTLS_PK_GOST_12_512:
		PK_TEST(GNUTLS_PK_GOST_12_512, test_sig,
			GNUTLS_CURVE_TO_BITS(GNUTLS_ECC_CURVE_GOST512A),
			GNUTLS_SIGN_GOST_512);

		if (!(flags & GNUTLS_SELF_TEST_FLAG_ALL))
			return 0;
#endif
		break;
	default:
		return gnutls_assert_val(GNUTLS_E_NO_SELF_TEST);
	}

	ret = 0;

      cleanup:
	return ret;
}
