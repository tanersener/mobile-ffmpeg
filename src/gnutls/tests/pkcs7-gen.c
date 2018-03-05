/*
 * Copyright (C) 2015 Red Hat, Inc.
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gnutls/gnutls.h>
#include <gnutls/pkcs7.h>
#include <gnutls/abstract.h>

#include "utils.h"

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

const gnutls_datum_t cert = {(void *) pem1_cert, sizeof(pem1_cert)-1};
const gnutls_datum_t key = {(void *) pem1_key, sizeof(pem1_key)-1};

static time_t mytime(time_t * t)
{
	time_t then = 1199142000;

	if (t)
		*t = then;

	return then;
}

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "%s |<%d>| %s", "err", level, str);
}

void doit(void)
{
	gnutls_privkey_t pkey;
	gnutls_x509_crt_t crt;
	gnutls_pkcs7_t pkcs7;
	int ret;
	gnutls_pkcs7_attrs_t list1 = NULL;
	gnutls_pkcs7_attrs_t list2 = NULL;
	gnutls_datum_t out;
	gnutls_datum_t data1 = {(unsigned char*)"xxx", 3};
	gnutls_datum_t data2 = {(unsigned char*)"yyyy", 4};
	gnutls_datum_t data3 = {(unsigned char*)"aaaaa", 5};
	gnutls_pkcs7_signature_info_st info;
	char *oid;
	gnutls_datum_t data;

	gnutls_global_set_time_function(mytime);
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(6);

	ret = gnutls_privkey_init(&pkey);
	if (ret < 0) {
		fail("error in %d: %s\n", __LINE__, gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_x509_crt_init(&crt);
	if (ret < 0) {
		fail("error in %d: %s\n", __LINE__, gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_privkey_import_x509_raw(pkey, &key, GNUTLS_X509_FMT_PEM, 0, 0);
	if (ret < 0) {
		fail("error in %d: %s\n", __LINE__, gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_x509_crt_import(crt, &cert, GNUTLS_X509_FMT_PEM);
	if (ret < 0) {
		fail("error in %d: %s\n", __LINE__, gnutls_strerror(ret));
		exit(1);
	}

	/* generate a PKCS #7 structure */
	ret = gnutls_pkcs7_init(&pkcs7);
	if (ret < 0) {
		fail("error in %d: %s\n", __LINE__, gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_pkcs7_add_attr(&list1, "1.2.3.4", &data1, GNUTLS_PKCS7_ATTR_ENCODE_OCTET_STRING);
	if (ret < 0) {
		fail("error in %d: %s\n", __LINE__, gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_pkcs7_add_attr(&list1, "2.3.4", &data2, GNUTLS_PKCS7_ATTR_ENCODE_OCTET_STRING);
	if (ret < 0) {
		fail("error in %d: %s\n", __LINE__, gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_pkcs7_add_attr(&list2, "2.3.4", &data3, GNUTLS_PKCS7_ATTR_ENCODE_OCTET_STRING);
	if (ret < 0) {
		fail("error in %d: %s\n", __LINE__, gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_pkcs7_sign(pkcs7, crt, pkey, &data3, list1, list2, GNUTLS_DIG_SHA256, 0);
	if (ret < 0) {
		fail("error in %d: %s\n", __LINE__, gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_pkcs7_export2(pkcs7, GNUTLS_X509_FMT_PEM, &out);
	if (ret < 0) {
		fail("error in %d: %s\n", __LINE__, gnutls_strerror(ret));
		exit(1);
	}

	gnutls_pkcs7_deinit(pkcs7);

	/* Import and verify */
	ret = gnutls_pkcs7_init(&pkcs7);
	if (ret < 0) {
		fail("error in %d: %s\n", __LINE__, gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_pkcs7_import(pkcs7, &out, GNUTLS_X509_FMT_PEM);
	if (ret < 0) {
		fail("error in %d: %s\n", __LINE__, gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_pkcs7_get_signature_info(pkcs7, 0, &info);
	if (ret < 0) {
		fail("error in %d: %s\n", __LINE__, gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_pkcs7_get_attr(info.signed_attrs, 1, &oid, &data, 0);
	if (ret < 0) {
		fail("error in %d: %s\n", __LINE__, gnutls_strerror(ret));
		exit(1);
	}

	if (strcmp(oid, "1.2.840.113549.1.9.3") != 0) {
		fail("error in %d: %s\n", __LINE__, oid);
		exit(1);
	}
	gnutls_free(data.data);

	ret = gnutls_pkcs7_get_attr(info.signed_attrs, 2, &oid, &data, GNUTLS_PKCS7_ATTR_ENCODE_OCTET_STRING);
	if (ret < 0) {
		fail("error in %d: %s\n", __LINE__, gnutls_strerror(ret));
		exit(1);
	}

	if (strcmp(oid, "1.2.3.4") != 0 || data.size != data1.size || memcmp(data.data, data1.data, data.size) != 0) {
		fail("error in %d: %s\n", __LINE__, oid);
		exit(1);
	}
	gnutls_free(data.data);

	ret = gnutls_pkcs7_get_attr(info.signed_attrs, 3, &oid, &data, GNUTLS_PKCS7_ATTR_ENCODE_OCTET_STRING);
	if (ret < 0) {
		fail("error in %d: %s\n", __LINE__, gnutls_strerror(ret));
		exit(1);
	}

	if (strcmp(oid, "2.3.4") != 0 || data.size != data2.size || memcmp(data.data, data2.data, data.size) != 0) {
		fail("error in %d: %s\n", __LINE__, oid);
		exit(1);
	}
	gnutls_free(data.data);

	ret = gnutls_pkcs7_get_attr(info.unsigned_attrs, 0, &oid, &data, GNUTLS_PKCS7_ATTR_ENCODE_OCTET_STRING);
	if (ret < 0) {
		fail("error in %d: %s\n", __LINE__, gnutls_strerror(ret));
		exit(1);
	}

	if (strcmp(oid, "2.3.4") != 0 || data.size != data3.size || memcmp(data.data, data3.data, data.size) != 0) {
		fail("error in %d: %s\n", __LINE__, oid);
		exit(1);
	}
	gnutls_free(data.data);

	gnutls_pkcs7_signature_info_deinit(&info);

	ret = gnutls_pkcs7_verify_direct(pkcs7, crt, 0, &data3, 0);
	if (ret < 0) {
		fail("error in %d: %s\n", __LINE__, gnutls_strerror(ret));
		exit(1);
	}

	gnutls_free(out.data);
	gnutls_pkcs7_attrs_deinit(list1);
	gnutls_pkcs7_attrs_deinit(list2);
	gnutls_pkcs7_deinit(pkcs7);
	gnutls_privkey_deinit(pkey);
	gnutls_x509_crt_deinit(crt);
}
