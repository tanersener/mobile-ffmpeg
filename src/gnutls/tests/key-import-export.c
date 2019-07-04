/*
 * Copyright (C) 2015 Nikos Mavrogiannopoulos
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
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <utils.h>
#include <assert.h>

#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#include <gnutls/abstract.h>

#include "cert-common.h"

static char rsa_key_pem[] =
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

const gnutls_datum_t rsa_key = { (void*)rsa_key_pem,
	sizeof(rsa_key_pem)
};

static void dump(const char *name, unsigned char *buf, int buf_size)
{
	int i;
	fprintf(stderr, "%s: ", name);
	for (i = 0; i < buf_size; i++)
		fprintf(stderr, "\\x%.2x", buf[i]);
	fprintf(stderr, "\n");
}

unsigned char dsa_p[] = "\x00\xb9\x84\xf5\x5a\x81\xbe\x1a\x0d\xc5\x8a\x73\x8f\x0c\x9b\x2f\x9b\xb6\x0e\x4b\xc3\x74\x1a\x7f\x64\xad\x9d\xf3\x28\xc5\xa0\x47\xbc\x9b\x57\x56\xf1\x97\xd5\x7e\x37\x03\xe9\xf2\x4c\xf4\xe3\x8b\x7f\x30\xa3\x5d\x2f\xbb\xa1\xa2\x37\xc2\xea\x35\x8f\x1f\xb1\x5f\xa6\xa2\x5f\x01\xf1\x23\x36\x2b\xe4\x4f\x2f\x2d\xdd\x9d\xd5\x3a\xa6\x39\xaf\x7a\x51\x7c\xd2\x25\x8e\x97\x74\xcf\x1e\xc5\x7b\x4b\x76\x43\x81\x07\x1f\x06\x14\xb8\x6e\x58\x12\xe1\x90\xe2\x37\x6f\xd2\x1b\xec\x68\xc5\x58\xe2\xe6\x30\xe0\x6a\x5e\x2c\x63\x78\xec\x07";
unsigned char dsa_q[] = "\x00\x9f\x56\x8c\x48\x64\x2f\xfe\x8d\xaa\x7a\x6d\x96\xdb\x04\x5d\x16\xef\x08\xa5\x71";
unsigned char dsa_g[] = "\x62\x06\x7e\xe4\x5c\x76\x08\xb7\x46\x1a\x5d\xd7\x97\xd4\x2a\x21\xfb\x1f\x31\xc9\xd2\xf4\xfa\x39\xd8\x27\xd1\x9b\xfc\x27\x5d\xa7\x0a\xa7\x1a\xfc\x53\xc1\x2f\x43\xc2\x37\xc8\x85\x7f\x3d\x4c\xab\x5d\x81\x32\xfb\x1d\x5e\x1e\x54\x11\x16\x20\xc6\x80\x5a\xd9\x8c\x9b\x43\xf0\xdd\x6b\xa0\xf4\xc3\xf2\x8a\x9c\x39\xd2\x1c\x7b\x0f\xef\xfa\x28\x93\x8f\xd2\xa1\x22\xeb\xdc\xe0\x8a\x8b\xad\x28\x0e\xcf\xef\x09\x85\xe9\x36\xbd\x8b\x7a\x50\xd5\x7b\xf7\x25\x0d\x6c\x60\x11\xc4\xef\x70\x90\xcf\xd6\x1b\xeb\xbb\x8e\xc6\x3e\x3a\x97";
unsigned char dsa_y[] = "\x0f\x8a\x87\x57\xf2\xd1\xc2\xdc\xac\xdf\x4b\x8b\x0f\x8b\xba\x29\xf7\xe1\x03\xe4\x55\xfa\xb2\x98\x07\xd6\xfd\x12\xb1\x80\xbc\xf5\xba\xb4\x50\xd4\x7f\xa0\x0e\x43\xe7\x9f\xc9\x78\x11\x5f\xe5\xe4\x0c\x2c\x6b\x6a\xa4\x35\xdc\xbd\x54\xe5\x60\x36\x9a\x31\xd1\x8a\x59\x6e\x6b\x1c\xba\xbd\x2e\xba\xeb\x7c\x87\xef\xda\xc8\xdd\xa1\xeb\xa4\x83\xe6\x8b\xad\xfa\xfa\x8e\x5b\xd7\x37\xc8\x32\x3e\x96\xc2\x3e\xf4\x43\xda\x7d\x91\x02\x0f\xb7\xbc\xf8\xef\x8f\xf7\x41\x00\x5e\x96\xdf\x0f\x08\x96\xdc\xea\xb2\xe9\x06\x82\xaf\xd2\x2f";
unsigned char dsa_x[] = "\x4b\x9f\xeb\xff\x6c\x9a\x02\x83\x41\x5e\x37\x81\x8e\x00\x86\x31\xe8\xb6\x9b\xc1";

unsigned char rsa_m[] = "\x00\xbb\x66\x43\xf5\xf2\xc5\xd7\xb6\x8c\xcc\xc5\xdf\xf5\x88\x3b\xb1\xc9\x4b\x6a\x0e\xa1\xad\x20\x50\x40\x08\x80\xa1\x4f\x5c\xa3\xd0\xf8\x6c\xcf\xe6\x3c\xf7\xec\x04\x76\x13\x17\x8b\x64\x89\x22\x5b\xc0\xdd\x53\x7c\x3b\xed\x7c\x04\xbb\x80\xb9\x28\xbe\x8e\x9b\xc6\x8e\xa0\xa5\x12\xcb\xf5\x57\x1e\xa2\xe7\xbb\xb7\x33\x49\x9f\xe3\xbb\x4a\xae\x6a\x4d\x68\xff\xc9\x11\xe2\x32\x8d\xce\x3d\x80\x0b\x8d\x75\xef\xd8\x00\x81\x8f\x28\x04\x03\xa0\x22\x8d\x61\x04\x07\xfa\xb6\x37\x7d\x21\x07\x49\xd2\x09\x61\x69\x98\x90\xa3\x58\xa9";
unsigned char rsa_e[] = "\x01\x00\x01";
unsigned char rsa_d[] = "\x0e\x99\x80\x44\x6e\x42\x43\x14\xbe\x01\xeb\x0d\x90\x69\xa9\x6a\xe7\xa9\x88\x2c\xf5\x24\x11\x7f\x27\x09\xf2\x89\x7e\xaf\x13\x35\x21\xd1\x8a\x5d\xdf\xd4\x99\xce\xdc\x2b\x0f\x1b\xc5\x3c\x98\xd0\x68\xa5\x65\x8e\x69\x75\xce\x42\x69\x20\x35\x6c\xaa\xf1\xdd\xc9\x57\x6c\x7b\xc3\x3e\x42\x7e\xa1\xc3\x8c\x76\xa7\x9a\xe8\x81\xdb\xe1\x84\x82\xf5\x99\xd5\xa8\xee\x35\x9e\x54\x94\xc5\x44\xa0\x7b\xcc\xb7\x4c\x3e\xcd\xf2\x49\xdb\x5c\x21\x06\x85\xf6\x75\x00\x43\x62\x89\x12\xf9\x5d\x90\xed\xe6\xfd\xb4\x49\x14\x4a\x79\xe2\x4d";
unsigned char rsa_p[] = "\x00\xd8\xcb\xe4\x65\x4e\x6c\x11\x0f\xa8\x72\xed\x4b\x4c\x8d\x1d\x07\xdc\x24\x99\x25\xe4\x3c\xb2\xf3\x02\xc4\x72\xe6\x3a\x5b\x86\xf4\x7d\x54\x2a\x4e\x79\x64\x16\x1f\x45\x3b\x17\x9e\x2a\x94\x90\x90\x59\xe7\x0b\x95\xd4\xbf\xa9\x47\xd1\x0a\x71\xaf\x3d\x6b\xed\x55";
unsigned char rsa_q[] = "\x00\xdd\x49\x81\x7a\x5c\x04\xbf\x6b\xbd\x70\x05\x35\x42\x32\xa3\x9b\x08\xee\xd4\x98\x17\x6e\xb8\xc4\xa2\x12\xbe\xdc\x1e\x72\xd0\x44\x84\x5c\xf0\x30\x35\x04\xfd\x4e\xb0\xcc\xd6\x6f\x40\xcb\x16\x13\x58\xbc\x57\xf7\x77\x48\xe5\x0c\x0d\x14\x9b\x66\x6e\xd8\xde\x05";
unsigned char rsa_u[] = "\x4a\x74\x5c\x95\x83\x54\xa3\xb0\x71\x35\xba\x02\x3a\x7d\x4a\x8c\x2d\x9a\x26\x77\x60\x36\x28\xd4\xb1\x7d\x8a\x06\xf8\x89\xa2\xef\xb1\x66\x46\x7d\xb9\xd4\xde\xbc\xa3\xbe\x46\xfa\x62\xe1\x63\x82\xdc\xdb\x64\x36\x47\x59\x00\xa8\xf3\xf7\x0e\xb4\xe0\x66\x3d\xd9";
unsigned char rsa_e1[] = "\x45\x20\x96\x5e\x1b\x28\x68\x34\x46\xf1\x06\x6b\x09\x28\xc1\xc5\xfc\xd3\x0a\xa6\x43\x65\x7b\x65\xf3\x4e\xf2\x98\x28\xa9\x80\x99\xba\xd0\xb8\x80\xb7\x42\x4b\xaf\x82\xe2\xb9\xc0\x2c\x31\x9c\xfa\xfa\x3f\xaa\xb9\x06\xd2\x6a\x46\xc5\x08\x00\x81\xf1\x22\xd5\xd5";
unsigned char rsa_e2[] = "\x00\xa6\x50\x60\xa7\xfe\x10\xf3\x6d\x9e\x6b\x5a\xfe\xb4\x4a\x2a\xfc\x92\xb2\x2d\xc6\x41\x96\x4d\xf8\x3b\x77\xab\x4a\xf4\xf7\x85\xe0\x79\x3b\x00\xaa\xba\xae\x8d\x53\x5f\x3e\x14\xcc\x78\xfe\x2a\x11\x50\x57\xfe\x25\x57\xd9\xc9\x8c\x4d\x28\x77\xc3\x7c\xfc\x31\xa1";

unsigned char ecc_x[] = "\x3c\x15\x6f\x1d\x48\x3e\x64\x59\x13\x2c\x6d\x04\x1a\x38\x0d\x30\x5c\xe4\x3f\x55\xcb\xd9\x17\x15\x46\x72\x71\x92\xc1\xf8\xc6\x33";
unsigned char ecc_y[] = "\x3d\x04\x2e\xc8\xc1\x0f\xc0\x50\x04\x7b\x9f\xc9\x48\xb5\x40\xfa\x6f\x93\x82\x59\x61\x5e\x72\x57\xcb\x83\x06\xbd\xcc\x82\x94\xc1";
unsigned char ecc_k[] = "\x00\xfd\x2b\x00\x80\xf3\x36\x5f\x11\x32\x65\xe3\x8d\x30\x33\x3b\x47\xf5\xce\xf8\x13\xe5\x4c\xc2\xcf\xfd\xe8\x05\x6a\xca\xc9\x41\xb1";

unsigned char false_ed25519_x[] = "\xac\xac\x9a\xb3\xc3\x41\x8d\x41\x22\x21\xc1\x84\xa7\xb8\x70\xfb\x44\x6e\xc7\x7e\x20\x87\x7b\xd9\x22\xa4\x5d\xd2\x97\x09\xd5\x48";
unsigned char ed25519_x[] = "\xab\xaf\x98\xb3\xc3\x41\x8d\x41\x22\x21\xc1\x86\xa7\xb8\x70\xfb\x44\x6e\xc7\x7e\x20\x87\x7b\xd9\x22\xa4\x5d\xd2\x97\x09\xd5\x48";
unsigned char ed25519_k[] = "\x1c\xa9\x23\xdc\x35\xa8\xfd\xd6\x2d\xa8\x98\xb9\x60\x7b\xce\x10\x3d\xf4\x64\xc6\xe5\x4b\x0a\x65\x56\x6a\x3c\x73\x65\x51\xa2\x2f";

unsigned char gost_x[] = "\xd0\xbb\xe9\xf4\xc6\xa8\x60\x3c\x73\x91\x44\x55\xcf\xbd\x50\xdd\x2c\x3d\x5a\xbc\x1a\xd8\x5e\x3c\xdf\x10\xdd\xd2\x63\x88\x0f\xc0";
unsigned char gost_y[] = "\x8a\xec\x96\x3c\x0b\xc8\x33\xff\x57\x5f\x66\x78\x94\x39\xb4\xf5\x24\xc6\xba\x86\x41\xac\x43\x21\x6f\x3c\xb0\xfa\x56\xbd\x5b\x37";
unsigned char gost_k[] = "\x47\x59\x41\x2c\x8a\xf8\x58\x1a\x67\xe0\xc3\x82\x1f\xca\x31\x19\x66\xf9\xd8\x43\xcd\x2f\x78\x23\x34\x98\x90\xb8\x14\x2e\x7f\xa5";

gnutls_datum_t _dsa_p = {dsa_p, sizeof(dsa_p)-1};
gnutls_datum_t _dsa_q = {dsa_q, sizeof(dsa_q)-1};
gnutls_datum_t _dsa_g = {dsa_g, sizeof(dsa_g)-1};
gnutls_datum_t _dsa_y = {dsa_y, sizeof(dsa_y)-1};
gnutls_datum_t _dsa_x = {dsa_x, sizeof(dsa_x)-1};

gnutls_datum_t _rsa_m = {rsa_m, sizeof(rsa_m)-1};
gnutls_datum_t _rsa_e = {rsa_e, sizeof(rsa_e)-1};
gnutls_datum_t _rsa_d = {rsa_d, sizeof(rsa_d)-1};
gnutls_datum_t _rsa_p = {rsa_p, sizeof(rsa_p)-1};
gnutls_datum_t _rsa_q = {rsa_q, sizeof(rsa_q)-1};
gnutls_datum_t _rsa_u = {rsa_u, sizeof(rsa_u)-1};
gnutls_datum_t _rsa_e1 = {rsa_e1, sizeof(rsa_e1)-1};
gnutls_datum_t _rsa_e2 = {rsa_e2, sizeof(rsa_e2)-1};

gnutls_datum_t _ecc_x = {ecc_x, sizeof(ecc_x)-1};
gnutls_datum_t _ecc_y = {ecc_y, sizeof(ecc_y)-1};
gnutls_datum_t _ecc_k = {ecc_k, sizeof(ecc_k)-1};

gnutls_datum_t _false_ed25519_x = {false_ed25519_x, sizeof(false_ed25519_x)-1};
gnutls_datum_t _ed25519_x = {ed25519_x, sizeof(ed25519_x)-1};
gnutls_datum_t _ed25519_k = {ed25519_k, sizeof(ed25519_k)-1};

gnutls_datum_t _gost_x = {gost_x, sizeof(gost_x)-1};
gnutls_datum_t _gost_y = {gost_y, sizeof(gost_y)-1};
gnutls_datum_t _gost_k = {gost_k, sizeof(gost_k)-1};

unsigned char ecc_params[] = "\x06\x08\x2a\x86\x48\xce\x3d\x03\x01\x07";
unsigned char ecc_point[] = "\x04\x41\x04\x3c\x15\x6f\x1d\x48\x3e\x64\x59\x13\x2c\x6d\x04\x1a\x38\x0d\x30\x5c\xe4\x3f\x55\xcb\xd9\x17\x15\x46\x72\x71\x92\xc1\xf8\xc6\x33\x3d\x04\x2e\xc8\xc1\x0f\xc0\x50\x04\x7b\x9f\xc9\x48\xb5\x40\xfa\x6f\x93\x82\x59\x61\x5e\x72\x57\xcb\x83\x06\xbd\xcc\x82\x94\xc1";

static int _gnutls_privkey_export2_pkcs8(gnutls_privkey_t key, gnutls_x509_crt_fmt_t f,
					 const char *password, unsigned flags, gnutls_datum_t *out)
{
	gnutls_x509_privkey_t xkey;
	int ret;

	ret = gnutls_privkey_export_x509(key, &xkey);
	if (ret < 0)
		fail("error in gnutls_privkey_export_x509\n");

	assert(gnutls_x509_privkey_fix(xkey)>=0);

	ret = gnutls_x509_privkey_export2_pkcs8(xkey, f, password, 0, out);
	gnutls_x509_privkey_deinit(xkey);

	return ret;
}

#define CMP(name, dat, v) cmp(name, __LINE__, dat, v, sizeof(v)-1)
static int cmp(const char *name, int line, gnutls_datum_t *v1, unsigned char *v2, unsigned size)
{
	if (size != v1->size) {
		fprintf(stderr, "error in %s:%d size\n", name, line);
		dump("expected", v2, size);
		dump("got", v1->data, v1->size);
		exit(1);
	}

	if (memcmp(v1->data, v2, size) != 0) {
		fprintf(stderr, "error in %s:%d\n", name, line);
		dump("expected", v2, size);
		dump("got", v1->data, v1->size);
		exit(1);
	}
	return 0;
}

/* leading zero on v2 is ignored */
#define CMP_NO_LZ(name, dat, v) cmp_no_lz(name, __LINE__, dat, v, sizeof(v)-1)
static int cmp_no_lz(const char *name, int line, gnutls_datum_t *v1, unsigned char *i2, unsigned size)
{
	gnutls_datum_t v2;
	if (size > 0 && i2[0] == 0) {
		v2.data = &i2[1];
		v2.size = size-1;
	} else {
		v2.data = i2;
		v2.size = size;
	}

	if (v2.size != v1->size) {
		fprintf(stderr, "error in %s:%d size\n", name, line);
		dump("expected", v2.data, v2.size);
		dump("got", v1->data, v1->size);
		exit(1);
	}

	if (memcmp(v1->data, v2.data, v2.size) != 0) {
		fprintf(stderr, "error in %s:%d\n", name, line);
		dump("expected", v2.data, v2.size);
		dump("got", v1->data, v1->size);
		exit(1);
	}
	return 0;
}

static
int check_x509_privkey(void)
{
	gnutls_x509_privkey_t key;
	gnutls_datum_t p, q, g, y, x;
	gnutls_datum_t m, e, u, e1, e2, d;
	gnutls_ecc_curve_t curve;
	int ret;

	global_init();

	ret = gnutls_x509_privkey_init(&key);
	if (ret < 0)
		fail("error\n");

	ret = gnutls_x509_privkey_import(key, &dsa_key, GNUTLS_X509_FMT_PEM);
	if (ret < 0)
		fail("error\n");

	ret = gnutls_x509_privkey_export_dsa_raw(key, &p, &q, &g, &y, &x);
	if (ret < 0)
		fail("error\n");

	CMP("p", &p, dsa_p);
	CMP("q", &q, dsa_q);
	CMP("g", &g, dsa_g);
	CMP("y", &y, dsa_y);
	CMP("x", &x, dsa_x);
	gnutls_free(p.data);
	gnutls_free(q.data);
	gnutls_free(g.data);
	gnutls_free(y.data);
	gnutls_free(x.data);
	gnutls_x509_privkey_deinit(key);

	/* RSA */
	ret = gnutls_x509_privkey_init(&key);
	if (ret < 0)
		fail("error\n");

	ret = gnutls_x509_privkey_import(key, &rsa_key, GNUTLS_X509_FMT_PEM);
	if (ret < 0)
		fail("error\n");

	ret = gnutls_x509_privkey_export_rsa_raw2(key, &m, &e, &d, &p, &q, &u, &e1, &e2);
	if (ret < 0)
		fail("error\n");

	CMP("m", &m, rsa_m);
	CMP("e", &e, rsa_e);
	CMP("d", &d, rsa_d);
	CMP("p", &p, rsa_p);
	CMP("q", &q, rsa_q);
	CMP("u", &u, rsa_u);
	CMP("e1", &e1, rsa_e1);
	CMP("e2", &e2, rsa_e2);
	gnutls_free(m.data);
	gnutls_free(e.data);
	gnutls_free(d.data);
	gnutls_free(p.data);
	gnutls_free(q.data);
	gnutls_free(u.data);
	gnutls_free(e1.data);
	gnutls_free(e2.data);
	gnutls_x509_privkey_deinit(key);

	/* ECC */
	ret = gnutls_x509_privkey_init(&key);
	if (ret < 0)
		fail("error\n");

	ret = gnutls_x509_privkey_import(key, &server_ecc_key, GNUTLS_X509_FMT_PEM);
	if (ret < 0)
		fail("error\n");

	ret = gnutls_x509_privkey_export_ecc_raw(key, &curve, &x, &y, &p);
	if (ret < 0)
		fail("error\n");

	if (curve != GNUTLS_ECC_CURVE_SECP256R1) {
		fprintf(stderr, "unexpected curve value: %d\n", (int)curve);
		exit(1);
	}
	CMP("x", &x, ecc_x);
	CMP("y", &y, ecc_y);
	CMP("k", &p, ecc_k);
	gnutls_free(x.data);
	gnutls_free(y.data);
	gnutls_free(p.data);
	gnutls_x509_privkey_deinit(key);

	return 0;
}

static
int check_privkey_import_export(void)
{
	gnutls_privkey_t key;
	gnutls_datum_t p, q, g, y, x;
	gnutls_datum_t m, e, u, e1, e2, d;
	gnutls_ecc_curve_t curve;
	gnutls_digest_algorithm_t digest;
	gnutls_gost_paramset_t paramset;
	int ret;

	global_init();

	ret = gnutls_privkey_init(&key);
	if (ret < 0)
		fail("error\n");

	ret = gnutls_privkey_import_dsa_raw(key, &_dsa_p, &_dsa_q, &_dsa_g, &_dsa_y, &_dsa_x);
	if (ret < 0)
		fail("error\n");

	ret = gnutls_privkey_export_dsa_raw2(key, &p, &q, &g, &y, &x, 0);
	if (ret < 0)
		fail("error: %s\n", gnutls_strerror(ret));

	CMP("p", &p, dsa_p);
	CMP("q", &q, dsa_q);
	CMP("g", &g, dsa_g);
	CMP("y", &y, dsa_y);
	CMP("x", &x, dsa_x);
	gnutls_free(p.data);
	gnutls_free(q.data);
	gnutls_free(g.data);
	gnutls_free(y.data);
	gnutls_free(x.data);

	ret = gnutls_privkey_export_dsa_raw2(key, &p, &q, &g, &y, &x, GNUTLS_EXPORT_FLAG_NO_LZ);
	if (ret < 0)
		fail("error: %s\n", gnutls_strerror(ret));

	CMP_NO_LZ("p", &p, dsa_p);
	CMP_NO_LZ("q", &q, dsa_q);
	CMP_NO_LZ("g", &g, dsa_g);
	CMP_NO_LZ("y", &y, dsa_y);
	CMP_NO_LZ("x", &x, dsa_x);
	gnutls_free(p.data);
	gnutls_free(q.data);
	gnutls_free(g.data);
	gnutls_free(y.data);
	gnutls_free(x.data);
	gnutls_privkey_deinit(key);

	/* RSA */
	ret = gnutls_privkey_init(&key);
	if (ret < 0)
		fail("error\n");

	ret = gnutls_privkey_import_rsa_raw(key, &_rsa_m, &_rsa_e, &_rsa_d, &_rsa_p, &_rsa_q, &_rsa_u, &_rsa_e1, &_rsa_e2);
	if (ret < 0)
		fail("error\n");

	ret = gnutls_privkey_export_rsa_raw2(key, &m, &e, &d, &p, &q, &u, &e1, &e2, 0);
	if (ret < 0)
		fail("error\n");

	CMP("m", &m, rsa_m);
	CMP("e", &e, rsa_e);
	CMP("d", &d, rsa_d);
	CMP("p", &p, rsa_p);
	CMP("q", &q, rsa_q);
	CMP("u", &u, rsa_u);
	CMP("e1", &e1, rsa_e1);
	CMP("e2", &e2, rsa_e2);
	gnutls_free(m.data);
	gnutls_free(e.data);
	gnutls_free(d.data);
	gnutls_free(p.data);
	gnutls_free(q.data);
	gnutls_free(u.data);
	gnutls_free(e1.data);
	gnutls_free(e2.data);

	ret = gnutls_privkey_export_rsa_raw2(key, &m, &e, &d, &p, &q, &u, &e1, &e2, GNUTLS_EXPORT_FLAG_NO_LZ);
	if (ret < 0)
		fail("error\n");

	CMP_NO_LZ("m", &m, rsa_m);
	CMP_NO_LZ("e", &e, rsa_e);
	CMP_NO_LZ("d", &d, rsa_d);
	CMP_NO_LZ("p", &p, rsa_p);
	CMP_NO_LZ("q", &q, rsa_q);
	CMP_NO_LZ("u", &u, rsa_u);
	CMP_NO_LZ("e1", &e1, rsa_e1);
	CMP_NO_LZ("e2", &e2, rsa_e2);
	gnutls_free(m.data);
	gnutls_free(e.data);
	gnutls_free(d.data);
	gnutls_free(p.data);
	gnutls_free(q.data);
	gnutls_free(u.data);
	gnutls_free(e1.data);
	gnutls_free(e2.data);
	gnutls_privkey_deinit(key);

	/* ECC */
	ret = gnutls_privkey_init(&key);
	if (ret < 0)
		fail("error\n");

	ret = gnutls_privkey_import_ecc_raw(key, GNUTLS_ECC_CURVE_SECP256R1, &_ecc_x, &_ecc_y, &_ecc_k);
	if (ret < 0)
		fail("error\n");

	ret = gnutls_privkey_export_ecc_raw2(key, &curve, &x, &y, &p, 0);
	if (ret < 0)
		fail("error\n");

	if (curve != GNUTLS_ECC_CURVE_SECP256R1) {
		fprintf(stderr, "unexpected curve value: %d\n", (int)curve);
		exit(1);
	}
	CMP("x", &x, ecc_x);
	CMP("y", &y, ecc_y);
	CMP("k", &p, ecc_k);
	gnutls_free(x.data);
	gnutls_free(y.data);
	gnutls_free(p.data);

	ret = gnutls_privkey_export_ecc_raw2(key, &curve, &x, &y, &p, GNUTLS_EXPORT_FLAG_NO_LZ);
	if (ret < 0)
		fail("error\n");

	if (curve != GNUTLS_ECC_CURVE_SECP256R1) {
		fprintf(stderr, "unexpected curve value: %d\n", (int)curve);
		exit(1);
	}
	CMP_NO_LZ("x", &x, ecc_x);
	CMP_NO_LZ("y", &y, ecc_y);
	CMP_NO_LZ("k", &p, ecc_k);
	gnutls_free(x.data);
	gnutls_free(y.data);
	gnutls_free(p.data);
	gnutls_privkey_deinit(key);

	/* Ed25519 */
	ret = gnutls_privkey_init(&key);
	if (ret < 0)
		fail("error\n");

	/* test whether an invalid size would fail */
	ret = gnutls_privkey_import_ecc_raw(key, GNUTLS_ECC_CURVE_ED25519, &_rsa_m, NULL, &_rsa_m);
	if (ret != GNUTLS_E_INVALID_REQUEST)
		fail("error\n");

	ret = gnutls_privkey_import_ecc_raw(key, GNUTLS_ECC_CURVE_ED25519, &_ed25519_x, NULL, &_ed25519_k);
	if (ret < 0)
		fail("error\n");

	ret = gnutls_privkey_verify_params(key);
	if (ret != 0)
		fail("error: %s\n", gnutls_strerror(ret));

	ret = gnutls_privkey_export_ecc_raw(key, &curve, &x, NULL, &p);
	if (ret < 0)
		fail("error\n");

	if (curve != GNUTLS_ECC_CURVE_ED25519) {
		fail("unexpected curve value: %d\n", (int)curve);
	}
	CMP("x", &x, ed25519_x);
	CMP("k", &p, ed25519_k);
	gnutls_free(x.data);
	gnutls_free(p.data);
	gnutls_privkey_deinit(key);

	/* Ed25519 with incorrect public key */
	ret = gnutls_privkey_init(&key);
	if (ret < 0)
		fail("error\n");

	ret = gnutls_privkey_import_ecc_raw(key, GNUTLS_ECC_CURVE_ED25519, &_false_ed25519_x, NULL, &_ed25519_k);
	if (ret < 0)
		fail("error\n");

	ret = gnutls_privkey_verify_params(key);
	if (ret != GNUTLS_E_ILLEGAL_PARAMETER)
		fail("error: %s\n", gnutls_strerror(ret));

	gnutls_privkey_deinit(key);

	/* GOST */
	ret = gnutls_privkey_init(&key);
	if (ret < 0)
		fail("error\n");

	ret = gnutls_privkey_import_gost_raw(key, GNUTLS_ECC_CURVE_GOST256CPXA, GNUTLS_DIG_GOSTR_94, GNUTLS_GOST_PARAMSET_CP_A, &_gost_x, &_gost_y, &_gost_k);
	if (ret < 0)
		fail("error\n");

	ret = gnutls_privkey_export_gost_raw2(key, &curve, &digest, &paramset, &x, &y, &p, 0);
	if (ret < 0)
		fail("error\n");

	if (curve != GNUTLS_ECC_CURVE_GOST256CPXA) {
		fprintf(stderr, "unexpected curve value: %d\n", (int)curve);
		exit(1);
	}
	if (digest != GNUTLS_DIG_GOSTR_94) {
		fprintf(stderr, "unexpected digest value: %d\n", (int)digest);
		exit(1);
	}
	if (paramset != GNUTLS_GOST_PARAMSET_CP_A) {
		fprintf(stderr, "unexpected paramset value: %d\n", (int)paramset);
		exit(1);
	}
	CMP("x", &x, gost_x);
	CMP("y", &y, gost_y);
	CMP("k", &p, gost_k);
	gnutls_free(x.data);
	gnutls_free(y.data);
	gnutls_free(p.data);

	ret = gnutls_privkey_export_gost_raw2(key, &curve, &digest, &paramset, &x, &y, &p, GNUTLS_EXPORT_FLAG_NO_LZ);
	if (ret < 0)
		fail("error\n");

	if (curve != GNUTLS_ECC_CURVE_GOST256CPXA) {
		fprintf(stderr, "unexpected curve value: %d\n", (int)curve);
		exit(1);
	}
	if (digest != GNUTLS_DIG_GOSTR_94) {
		fprintf(stderr, "unexpected digest value: %d\n", (int)digest);
		exit(1);
	}
	if (paramset != GNUTLS_GOST_PARAMSET_CP_A) {
		fprintf(stderr, "unexpected paramset value: %d\n", (int)paramset);
		exit(1);
	}
	CMP_NO_LZ("x", &x, gost_x);
	CMP_NO_LZ("y", &y, gost_y);
	CMP_NO_LZ("k", &p, gost_k);
	gnutls_free(x.data);
	gnutls_free(y.data);
	gnutls_free(p.data);
	gnutls_privkey_deinit(key);

	return 0;
}

static
int check_dsa(void)
{
	gnutls_privkey_t key;
	gnutls_pubkey_t pub;
	gnutls_datum_t p, q, g, y, x;
	int ret;

	global_init();

	success("Checking DSA key operations\n");

	ret = gnutls_privkey_init(&key);
	if (ret < 0)
		fail("error\n");

	ret = gnutls_pubkey_init(&pub);
	if (ret < 0)
		fail("error\n");

	ret = gnutls_privkey_import_x509_raw(key, &dsa_key, GNUTLS_X509_FMT_PEM, 0, 0);
	if (ret < 0)
		fail("error\n");

	ret = gnutls_pubkey_import_privkey(pub, key, 0, 0);
	if (ret < 0)
		fail("error\n");

	ret = gnutls_pubkey_export_dsa_raw2(pub, &p, &q, &g, &y, 0);
	if (ret < 0)
		fail("error\n");

	CMP("p", &p, dsa_p);
	CMP("q", &q, dsa_q);
	CMP("g", &g, dsa_g);
	CMP("y", &y, dsa_y);
	gnutls_free(p.data);
	gnutls_free(q.data);
	gnutls_free(g.data);
	gnutls_free(y.data);

	ret = gnutls_pubkey_export_dsa_raw2(pub, &p, &q, &g, &y, GNUTLS_EXPORT_FLAG_NO_LZ);
	if (ret < 0)
		fail("error\n");

	CMP_NO_LZ("p", &p, dsa_p);
	CMP_NO_LZ("q", &q, dsa_q);
	CMP_NO_LZ("g", &g, dsa_g);
	CMP_NO_LZ("y", &y, dsa_y);
	gnutls_free(p.data);
	gnutls_free(q.data);
	gnutls_free(g.data);
	gnutls_free(y.data);

	ret = gnutls_privkey_export_dsa_raw(key, &p, &q, &g, &y, &x);
	if (ret < 0)
		fail("error\n");
	CMP("p", &p, dsa_p);
	CMP("q", &q, dsa_q);
	CMP("g", &g, dsa_g);
	CMP("y", &y, dsa_y);
	CMP("x", &x, dsa_x);
	gnutls_free(p.data);
	gnutls_free(q.data);
	gnutls_free(g.data);
	gnutls_free(y.data);
	gnutls_free(x.data);

	ret = _gnutls_privkey_export2_pkcs8(key, GNUTLS_X509_FMT_DER, NULL, 0, &x);
	if (ret < 0 || x.size == 0)
		fail("error in pkcs8 export\n");
	gnutls_free(x.data);

	gnutls_privkey_deinit(key);
	gnutls_pubkey_deinit(pub);

	return 0;
}

static
int check_rsa(void)
{
	gnutls_privkey_t key;
	gnutls_pubkey_t pub;
	gnutls_datum_t m, e, d, p, q, u, e1, e2;
	int ret;

	success("Checking RSA key operations\n");

	/* RSA */
	ret = gnutls_privkey_init(&key);
	if (ret < 0)
		fail("error\n");

	ret = gnutls_pubkey_init(&pub);
	if (ret < 0)
		fail("error\n");

	ret = gnutls_privkey_import_x509_raw(key, &rsa_key, GNUTLS_X509_FMT_PEM, 0, 0);
	if (ret < 0)
		fail("error\n");

	ret = gnutls_pubkey_import_privkey(pub, key, 0, 0);
	if (ret < 0)
		fail("error\n");

	ret = gnutls_pubkey_export_rsa_raw2(pub, &m, &e, 0);
	if (ret < 0)
		fail("error\n");

	CMP("m", &m, rsa_m);
	CMP("e", &e, rsa_e);
	gnutls_free(m.data);
	gnutls_free(e.data);

	ret = gnutls_pubkey_export_rsa_raw2(pub, &m, &e, GNUTLS_EXPORT_FLAG_NO_LZ);
	if (ret < 0)
		fail("error\n");

	CMP_NO_LZ("m", &m, rsa_m);
	CMP_NO_LZ("e", &e, rsa_e);
	gnutls_free(m.data);
	gnutls_free(e.data);

	ret = gnutls_privkey_export_rsa_raw(key, &m, &e, &d, &p, &q, &u, &e1, &e2);
	if (ret < 0)
		fail("error\n");

	CMP("m", &m, rsa_m);
	CMP("e", &e, rsa_e);
	CMP("d", &d, rsa_d);
	CMP("p", &p, rsa_p);
	CMP("q", &q, rsa_q);
	CMP("u", &u, rsa_u);
	CMP("e1", &e1, rsa_e1);
	CMP("e2", &e2, rsa_e2);
	gnutls_free(m.data);
	gnutls_free(e.data);
	gnutls_free(d.data);
	gnutls_free(p.data);
	gnutls_free(q.data);
	gnutls_free(u.data);
	gnutls_free(e1.data);
	gnutls_free(e2.data);

	ret = _gnutls_privkey_export2_pkcs8(key, GNUTLS_X509_FMT_DER, NULL, 0, &m);
	if (ret < 0 || m.size == 0)
		fail("error in pkcs8 export\n");
	gnutls_free(m.data);

	gnutls_privkey_deinit(key);
	gnutls_pubkey_deinit(pub);

	return 0;
}

static
int check_ecc(void)
{
	gnutls_privkey_t key;
	gnutls_pubkey_t pub;
	gnutls_datum_t y, x, k;
	gnutls_ecc_curve_t curve;
	int ret;

	success("Checking SECP256R1 key operations\n");

	/* ECC */
	ret = gnutls_privkey_init(&key);
	if (ret < 0)
		fail("error\n");

	ret = gnutls_pubkey_init(&pub);
	if (ret < 0)
		fail("error\n");

	ret = gnutls_privkey_import_x509_raw(key, &server_ecc_key, GNUTLS_X509_FMT_PEM, 0, 0);
	if (ret < 0)
		fail("error\n");

	ret = gnutls_pubkey_import_privkey(pub, key, 0, 0);
	if (ret < 0)
		fail("error\n");

	ret = gnutls_pubkey_export_ecc_raw2(pub, &curve, &x, &y, 0);
	if (ret < 0)
		fail("error\n");

	if (curve != GNUTLS_ECC_CURVE_SECP256R1) {
		fprintf(stderr, "unexpected curve value: %d\n", (int)curve);
		exit(1);
	}
	CMP("x", &x, ecc_x);
	CMP("y", &y, ecc_y);
	gnutls_free(x.data);
	gnutls_free(y.data);

	ret = gnutls_pubkey_export_ecc_raw2(pub, &curve, &x, &y, GNUTLS_EXPORT_FLAG_NO_LZ);
	if (ret < 0)
		fail("error\n");

	if (curve != GNUTLS_ECC_CURVE_SECP256R1) {
		fprintf(stderr, "unexpected curve value: %d\n", (int)curve);
		exit(1);
	}
	CMP_NO_LZ("x", &x, ecc_x);
	CMP_NO_LZ("y", &y, ecc_y);
	gnutls_free(x.data);
	gnutls_free(y.data);


	/* check the private key export */
	ret = gnutls_privkey_export_ecc_raw(key, &curve, &x, &y, &k);
	if (ret < 0)
		fail("error\n");

	if (curve != GNUTLS_ECC_CURVE_SECP256R1) {
		fprintf(stderr, "unexpected curve value: %d\n", (int)curve);
		exit(1);
	}
	CMP("x", &x, ecc_x);
	CMP("y", &y, ecc_y);
	CMP("k", &k, ecc_k);
	gnutls_free(x.data);
	gnutls_free(y.data);
	gnutls_free(k.data);

	ret = _gnutls_privkey_export2_pkcs8(key, GNUTLS_X509_FMT_DER, NULL, 0, &x);
	if (ret < 0 || x.size == 0)
		fail("error in pkcs8 export\n");
	gnutls_free(x.data);

	gnutls_privkey_deinit(key);

	/* More public key ops */

	ret = gnutls_pubkey_export_ecc_x962(pub, &x, &y);
	if (ret < 0)
		fail("error\n");

	CMP("parameters", &x, ecc_params);
	CMP("ecpoint", &y, ecc_point);

	ret = gnutls_pubkey_import_ecc_x962(pub, &x, &y);
	if (ret < 0)
		fail("error\n");
	gnutls_free(x.data);
	gnutls_free(y.data);

	/* check again */
	ret = gnutls_pubkey_export_ecc_raw(pub, &curve, &x, &y);
	if (ret < 0)
		fail("error\n");

	if (curve != GNUTLS_ECC_CURVE_SECP256R1) {
		fprintf(stderr, "unexpected curve value: %d\n", (int)curve);
		fail("error\n");
	}
	CMP("x", &x, ecc_x);
	CMP("y", &y, ecc_y);
	gnutls_free(x.data);
	gnutls_free(y.data);

	gnutls_pubkey_deinit(pub);

	return 0;
}

static
int check_ed25519(void)
{
	gnutls_privkey_t key;
	gnutls_pubkey_t pub;
	gnutls_datum_t y, x, k;
	gnutls_ecc_curve_t curve;
	int ret;

	success("Checking ed25519 key operations\n");

	/* ECC */
	ret = gnutls_privkey_init(&key);
	if (ret < 0)
		fail("error\n");

	ret = gnutls_pubkey_init(&pub);
	if (ret < 0)
		fail("error\n");

	ret = gnutls_privkey_import_x509_raw(key, &server_ca3_eddsa_key, GNUTLS_X509_FMT_PEM, 0, 0);
	if (ret < 0)
		fail("error\n");

	ret = gnutls_pubkey_import_privkey(pub, key, 0, 0);
	if (ret < 0)
		fail("error\n");

	ret = gnutls_pubkey_export_ecc_raw(pub, &curve, &x, NULL);
	if (ret < 0)
		fail("error\n");
	gnutls_free(x.data);

	ret = gnutls_pubkey_export_ecc_raw(pub, &curve, &x, &y);
	if (ret < 0)
		fail("error\n");

	if (curve != GNUTLS_ECC_CURVE_ED25519) {
		fail("unexpected curve value: %d\n", (int)curve);
	}
	CMP("x", &x, ed25519_x);

	if (y.data != NULL) {
		fail("expected NULL value in Y\n");
	}
	gnutls_free(x.data);


	/* check the private key export */
	ret = gnutls_privkey_export_ecc_raw(key, &curve, &x, NULL, &k);
	if (ret < 0)
		fail("error\n");
	gnutls_free(x.data);
	gnutls_free(k.data);

	ret = gnutls_privkey_export_ecc_raw(key, &curve, &x, &y, &k);
	if (ret < 0)
		fail("error\n");

	if (curve != GNUTLS_ECC_CURVE_ED25519) {
		fail("unexpected curve value: %d\n", (int)curve);
	}
	CMP("x", &x, ed25519_x);
	CMP("k", &k, ed25519_k);
	gnutls_free(x.data);
	gnutls_free(k.data);

	if (y.data != NULL) {
		fail("expected NULL value in Y\n");
	}

	ret = _gnutls_privkey_export2_pkcs8(key, GNUTLS_X509_FMT_DER, NULL, 0, &x);
	if (ret < 0 || x.size == 0)
		fail("error in pkcs8 export\n");
	gnutls_free(x.data);

	gnutls_privkey_deinit(key);

	/* More public key ops */

	ret = gnutls_pubkey_export_ecc_x962(pub, &x, &y);
	if (ret != GNUTLS_E_INVALID_REQUEST)
		fail("error\n");

	gnutls_pubkey_deinit(pub);

	return 0;
}

static
int check_gost(void)
{
	gnutls_privkey_t key;
	gnutls_pubkey_t pub;
	gnutls_datum_t y, x, k;
	gnutls_ecc_curve_t curve;
	gnutls_digest_algorithm_t digest;
	gnutls_gost_paramset_t paramset;
	int ret;

	success("Checking GOST key operations\n");

	/* ECC */
	ret = gnutls_privkey_init(&key);
	if (ret < 0)
		fail("error\n");

	ret = gnutls_pubkey_init(&pub);
	if (ret < 0)
		fail("error\n");

	ret = gnutls_privkey_import_x509_raw(key, &server_ca3_gost01_key, GNUTLS_X509_FMT_PEM, 0, 0);
	if (ret < 0)
		fail("error\n");

	ret = gnutls_pubkey_import_privkey(pub, key, 0, 0);
	if (ret < 0)
		fail("error\n");

	ret = gnutls_pubkey_export_gost_raw2(pub, &curve, &digest, &paramset, &x, &y, 0);
	if (ret < 0)
		fail("error\n");

	if (curve != GNUTLS_ECC_CURVE_GOST256CPXA) {
		fprintf(stderr, "unexpected curve value: %d\n", (int)curve);
		exit(1);
	}
	if (digest != GNUTLS_DIG_GOSTR_94) {
		fprintf(stderr, "unexpected digest value: %d\n", (int)digest);
		exit(1);
	}
	if (paramset != GNUTLS_GOST_PARAMSET_CP_A) {
		fprintf(stderr, "unexpected paramset value: %d\n", (int)paramset);
		exit(1);
	}
	CMP("x", &x, gost_x);
	CMP("y", &y, gost_y);
	gnutls_free(x.data);
	gnutls_free(y.data);

	ret = gnutls_pubkey_export_gost_raw2(pub, &curve, &digest, &paramset, &x, &y, GNUTLS_EXPORT_FLAG_NO_LZ);
	if (ret < 0)
		fail("error\n");

	if (curve != GNUTLS_ECC_CURVE_GOST256CPXA) {
		fprintf(stderr, "unexpected curve value: %d\n", (int)curve);
		exit(1);
	}
	if (digest != GNUTLS_DIG_GOSTR_94) {
		fprintf(stderr, "unexpected digest value: %d\n", (int)digest);
		exit(1);
	}
	if (paramset != GNUTLS_GOST_PARAMSET_CP_A) {
		fprintf(stderr, "unexpected paramset value: %d\n", (int)paramset);
		exit(1);
	}
	CMP_NO_LZ("x", &x, gost_x);
	CMP_NO_LZ("y", &y, gost_y);
	gnutls_free(x.data);
	gnutls_free(y.data);


	/* check the private key export */
	ret = gnutls_privkey_export_gost_raw2(key, &curve, &digest, &paramset, &x, &y, &k, 0);
	if (ret < 0)
		fail("error\n");

	if (curve != GNUTLS_ECC_CURVE_GOST256CPXA) {
		fprintf(stderr, "unexpected curve value: %d\n", (int)curve);
		exit(1);
	}
	if (digest != GNUTLS_DIG_GOSTR_94) {
		fprintf(stderr, "unexpected digest value: %d\n", (int)digest);
		exit(1);
	}
	if (paramset != GNUTLS_GOST_PARAMSET_CP_A) {
		fprintf(stderr, "unexpected paramset value: %d\n", (int)paramset);
		exit(1);
	}
	CMP("x", &x, gost_x);
	CMP("y", &y, gost_y);
	CMP("k", &k, gost_k);
	gnutls_free(x.data);
	gnutls_free(y.data);
	gnutls_free(k.data);

	ret = _gnutls_privkey_export2_pkcs8(key, GNUTLS_X509_FMT_DER, NULL, 0, &x);
	if (ret < 0 || x.size == 0)
		fail("error in pkcs8 export\n");
	gnutls_free(x.data);

	gnutls_privkey_deinit(key);

	gnutls_pubkey_deinit(pub);

	return 0;
}

void doit(void)
{
	if (check_x509_privkey() != 0) {
		fail("error in privkey check\n");
		exit(1);
	}

	if (check_privkey_import_export() != 0) {
		fail("error in privkey import/export check\n");
		exit(1);
	}

	if (check_dsa() != 0) {
		fail("error in DSA check\n");
		exit(1);
	}

	if (check_rsa() != 0) {
		fail("error in RSA check\n");
		exit(1);
	}

	if (check_ecc() != 0) {
		fail("error in ecdsa check\n");
	}

	if (check_ed25519() != 0) {
		fail("error in ed25519 check\n");
	}

	if (check_gost() != 0) {
		fail("error in gost check\n");
	}
}
