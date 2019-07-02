/*
 * Copyright (C) 2009-2012 Free Software Foundation, Inc.
 *
 * Author: Simon Josefsson
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

#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#include <gnutls/pkcs12.h>

#include "utils.h"

#include <stdio.h>
#include <stdlib.h>

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
const gnutls_datum_t client_dat =
    { (void *) client_pem, sizeof(client_pem) };

static char ca_pem[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIB5zCCAVKgAwIBAgIERiYdJzALBgkqhkiG9w0BAQUwGTEXMBUGA1UEAxMOR251\n"
    "VExTIHRlc3QgQ0EwHhcNMDcwNDE4MTMyOTExWhcNMDgwNDE3MTMyOTExWjAZMRcw\n"
    "FQYDVQQDEw5HbnVUTFMgdGVzdCBDQTCBnDALBgkqhkiG9w0BAQEDgYwAMIGIAoGA\n"
    "vuyYeh1vfmslnuggeEKgZAVmQ5ltSdUY7H25WGSygKMUYZ0KT74v8C780qtcNt9T\n"
    "7EPH/N6RvB4BprdssgcQLsthR3XKA84jbjjxNCcaGs33lvOz8A1nf8p3hD+cKfRi\n"
    "kfYSW2JazLrtCC4yRCas/SPOUxu78of+3HiTfFm/oXUCAwEAAaNDMEEwDwYDVR0T\n"
    "AQH/BAUwAwEB/zAPBgNVHQ8BAf8EBQMDBwQAMB0GA1UdDgQWBBTpPBz7rZJu5gak\n"
    "Viyi4cBTJ8jylTALBgkqhkiG9w0BAQUDgYEAiaIRqGfp1jPpNeVhABK60SU0KIAy\n"
    "njuu7kHq5peUgYn8Jd9zNzExBOEp1VOipGsf6G66oQAhDFp2o8zkz7ZH71zR4HEW\n"
    "KoX6n5Emn6DvcEH/9pAhnGxNHJAoS7czTKv/JDZJhkqHxyrE1fuLsg5Qv25DTw7+\n"
    "PfqUpIhz5Bbm7J4=\n" "-----END CERTIFICATE-----\n";
const gnutls_datum_t ca_dat = { (void *) ca_pem, sizeof(ca_pem) };

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "|<%d>| %s", level, str);
}

void doit(void)
{
	gnutls_pkcs12_t pkcs12;
	gnutls_x509_crt_t client;
	gnutls_x509_crt_t ca;
	gnutls_pkcs12_bag_t bag;
	unsigned char key_id_buf[20];
	gnutls_datum_t key_id;
	int ret, indx;
	char outbuf[10240];
	size_t size;
	unsigned tests, i;

	ret = global_init();
	if (ret < 0) {
		fprintf(stderr, "global_init %d", ret);
		exit(1);
	}

	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(4711);

	/* Read certs. */
	ret = gnutls_x509_crt_init(&client);
	if (ret < 0) {
		fprintf(stderr, "crt_init: %d", ret);
		exit(1);
	}

	ret =
	    gnutls_x509_crt_import(client, &client_dat,
				   GNUTLS_X509_FMT_PEM);
	if (ret < 0) {
		fprintf(stderr, "crt_import: %d", ret);
		exit(1);
	}

	ret = gnutls_x509_crt_init(&ca);
	if (ret < 0) {
		fprintf(stderr, "ca_init: %d", ret);
		exit(1);
	}

	ret = gnutls_x509_crt_import(ca, &ca_dat, GNUTLS_X509_FMT_PEM);
	if (ret < 0) {
		fprintf(stderr, "ca_import: %d", ret);
		exit(1);
	}

	/* Create PKCS#12 structure. */
	ret = gnutls_pkcs12_init(&pkcs12);
	if (ret < 0) {
		fprintf(stderr, "pkcs12_init: %d", ret);
		exit(1);
	}

	/* Generate and add PKCS#12 cert bags. */
	if (!gnutls_fips140_mode_enabled()) {
		tests = 2; /* include RC2 */
	} else {
		tests = 1;
	}

	for (i = 0; i < tests; i++) {
		ret = gnutls_pkcs12_bag_init(&bag);
		if (ret < 0) {
			fprintf(stderr, "bag_init: %s (%d)\n", gnutls_strerror(ret), ret);
			exit(1);
		}

		ret = gnutls_pkcs12_bag_set_crt(bag, i == 0 ? client : ca);
		if (ret < 0) {
			fprintf(stderr, "set_crt: %s (%d)\n", gnutls_strerror(ret), ret);
			exit(1);
		}

		indx = ret;

		ret = gnutls_pkcs12_bag_set_friendly_name(bag, indx,
							  i ==
							  0 ? "client" :
							  "ca");
		if (ret < 0) {
			fprintf(stderr, "set_friendly_name: %s (%d)\n", gnutls_strerror(ret), ret);
			exit(1);
		}

		size = sizeof(key_id_buf);
		ret = gnutls_x509_crt_get_key_id(i == 0 ? client : ca, 0,
						 key_id_buf, &size);
		if (ret < 0) {
			fprintf(stderr, "get_key_id: %s (%d)\n", gnutls_strerror(ret), ret);
			exit(1);
		}

		key_id.data = key_id_buf;
		key_id.size = size;

		ret = gnutls_pkcs12_bag_set_key_id(bag, indx, &key_id);
		if (ret < 0) {
			fprintf(stderr, "bag_set_key_id: %s (%d)\n", gnutls_strerror(ret), ret);
			exit(1);
		}

		ret = gnutls_pkcs12_bag_encrypt(bag, "pass",
						i ==
						0 ?
						GNUTLS_PKCS8_USE_PKCS12_3DES
						:
						GNUTLS_PKCS_USE_PKCS12_RC2_40);
		if (ret < 0) {
			fprintf(stderr, "bag_encrypt: %d: %s", ret,
				i == 0 ? "3DES" : "RC2-40");
			exit(1);
		}

		ret = gnutls_pkcs12_set_bag(pkcs12, bag);
		if (ret < 0) {
			fprintf(stderr, "set_bag: %s (%d)\n", gnutls_strerror(ret), ret);
			exit(1);
		}

		gnutls_pkcs12_bag_deinit(bag);
	}

	/* MAC the structure, export and print. */
	ret = gnutls_pkcs12_generate_mac2(pkcs12, GNUTLS_MAC_SHA1, "pass");
	if (ret < 0) {
		fprintf(stderr, "generate_mac: %s (%d)\n", gnutls_strerror(ret), ret);
		exit(1);
	}

	ret = gnutls_pkcs12_verify_mac(pkcs12, "pass");
	if (ret < 0) {
		fprintf(stderr, "verify_mac: %s (%d)\n", gnutls_strerror(ret), ret);
		exit(1);
	}

	ret = gnutls_pkcs12_generate_mac2(pkcs12, GNUTLS_MAC_SHA256, "passwd");
	if (ret < 0) {
		fprintf(stderr, "generate_mac2: %s (%d)\n", gnutls_strerror(ret), ret);
		exit(1);
	}

	ret = gnutls_pkcs12_verify_mac(pkcs12, "passwd");
	if (ret < 0) {
		fprintf(stderr, "verify_mac2: %s (%d)\n", gnutls_strerror(ret), ret);
		exit(1);
	}

	ret = gnutls_pkcs12_generate_mac2(pkcs12, GNUTLS_MAC_SHA512, "passwd1");
	if (ret < 0) {
		fprintf(stderr, "generate_mac2: %s (%d)\n", gnutls_strerror(ret), ret);
		exit(1);
	}

	ret = gnutls_pkcs12_verify_mac(pkcs12, "passwd1");
	if (ret < 0) {
		fprintf(stderr, "verify_mac2: %s (%d)\n", gnutls_strerror(ret), ret);
		exit(1);
	}

	size = sizeof(outbuf);
	ret =
	    gnutls_pkcs12_export(pkcs12, GNUTLS_X509_FMT_PEM, outbuf,
				 &size);
	if (ret < 0) {
		fprintf(stderr, "pkcs12_export: %s (%d)\n", gnutls_strerror(ret), ret);
		exit(1);
	}

	if (debug)
		fwrite(outbuf, size, 1, stdout);

	/* Cleanup. */
	gnutls_pkcs12_deinit(pkcs12);
	gnutls_x509_crt_deinit(client);
	gnutls_x509_crt_deinit(ca);
	gnutls_global_deinit();
}
