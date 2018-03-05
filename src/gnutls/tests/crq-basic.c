/*
 * Copyright (C) 2016 Red Hat, Inc.
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

#include <gnutls/gnutls.h>
#include <gnutls/x509.h>

#include "utils.h"

static const char csr1[] =
"-----BEGIN CERTIFICATE REQUEST-----\n"
"MIICrDCCAZQCAQAwZzELMAkGA1UEBhMCTk4xMTAvBgNVBAoMKEVkZWwgQ3VybCBB\n"
"cmN0aWMgSWxsdWRpdW0gUmVzZWFyY2ggQ2xvdWQxJTAjBgNVBAMMHE5vdGhlcm4g\n"
"Tm93aGVyZSBUcnVzdCBBbmNob3IwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEK\n"
"AoIBAQDBqQrvdgZ9/ng68Q5AbcJbro+Nf/DViZ5CKvAXlNkuZ8ctARyVo7GmhtQS\n"
"PEc6cOZ7HxEG03Ou38okGQPkYgrrZ9Tc750t4IJ3/iowWvtX5bhPNlJML1etEmqU\n"
"PuRIp62lwDrQTgCZiI+9SnC+O1tr/15vKW0Mp1VK4kPnSQ+ZVFlogTTYqfvIDRRa\n"
"QMtwHvU7wEI5BvljrdkpFFOvQhAdGJW2FYrYQdg3MQqXWhsQkKwd/25xM2t+iBgg\n"
"7b41/+dpSAXAeC4ERvTCjU1wbkL6k+vOEjvR9c4/KVyMvVmD5KHBPI4+OFXzmRiw\n"
"3/Z0yY4o9DgNRSDW28BzouaMbpifAgMBAAGgADANBgkqhkiG9w0BAQsFAAOCAQEA\n"
"eFMy55kFke/e9mrGloRUh1o8dxmzSiVwVCw5DTZQzTFNAMSOZXIId8k2IeHSUd84\n"
"ZyJ1UNyJn2EFcwgaYaMtvZ8xMWR2W0C7lBvOOcjvWmiGze9F2Z5XMQzL8cjkK4jW\n"
"RKIq9b0W6TC8lLO5F2eJpW6BoTQ8cBCDiVIDlCm7xZxPRjHowuyM0Tpewq2PltC1\n"
"p8DbQipZWl5LPaHBSZSmIuUgOBU9porH/Vn0oWXxYfts59103VJY5YKkdz0PiqqA\n"
"5kWYCMFDZyL+nZ2aIol4r8nXkN9MuPOU12aHqPGcDlaGS2i5zfm2Ywsg110k+NCk\n"
"AmqhjnrQjvJhif3rGO4+qw==\n"
"-----END CERTIFICATE REQUEST-----\n";

static const char csr2[] = 
"-----BEGIN NEW CERTIFICATE REQUEST-----\n"
"MIICrjCCAZYCAQAwJDEiMCAGA1UEAxMZZGhjcC0yLTEyNy5icnEucmVkaGF0LmNv\n"
"bTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBANiEAXIHON8p4YpVRH+X\n"
"OM546stpyzL5xKdxbRUlfK0pWoqm3iqenRUf43eb9W8RDTx6UhuY21RFETzlYT4r\n"
"+yVXOlvm8K5FLepNcjbqDJb9hngFm2q8u+OM3GKBiyeH43lUMC6/YksqPeEzsmKD\n"
"UlD7rkm9CK8GRyXEsCruFaQ0VA8XB6XK9Av/jfOrGT/gTdmNGKu/fZmoJsjBJh+g\n"
"Yobsi60YyWeuXw2s5zVga73cK1v0JG2ltjZy0M7qSO+CCJa24huO8uvJ4GPOfi/Q\n"
"MPZbsHaZAqrHLQQMfxXJ73gXq7FLIMnCcstWfiagE5QlFZUGj9AnicgiCpMTZMIq\n"
"miECAwEAAaBFMBMGCSqGSIb3DQEJBzEGEwQxMjM0MC4GCSqGSIb3DQEJDjEhMB8w\n"
"DAYDVR0TAQH/BAIwADAPBgNVHQ8BAf8EBQMDB6AAMA0GCSqGSIb3DQEBCwUAA4IB\n"
"AQAqYOqsS3xnfEzLIis3krcjzHGykXxfvnyREDijBIqyXF10lSrmb2byvoIfOuoc\n"
"pSmdT8MaIUTmKnZI4+htEPYcsAMwF2cXL1D2kvJhE0EKHbmv1E0QbJWmbnVz99bs\n"
"GIcFN1die0SYHLgf64bOxKOyq5V8hAaE/lS2yLT7Tf/6+nweYOuE9ONH7KD7zpQo\n"
"LyhsjhH0px75Ftej+yQWEElfokZrNu7iHuwcue3efySlMfpT9G/p4MhQQjFQySkK\n"
"ev17H0d3KBdtcqWjxaS3jDAzmuz6SZwdUxSDkWuqchyAozeBpI+SbIPOgfKHsYc+\n"
"yRKga0201rRJi4NKvt8iqj5r\n"
"-----END NEW CERTIFICATE REQUEST-----\n";

static struct
{
  const char *name;
  const char *crq;
  unsigned version;
  unsigned sign_algo;
  const char *sign_oid;
  unsigned pk_algo;
  const char *pk_oid;
} crq_list[] =
{
  { .name = "crl-1", 
    .crq = csr1,
    .sign_algo = GNUTLS_SIGN_RSA_SHA256,
    .sign_oid = "1.2.840.113549.1.1.11",
    .pk_algo = GNUTLS_PK_RSA,
    .pk_oid = "1.2.840.113549.1.1.1",
    .version = 1,
  },
  { .name = "crl-2",
    .crq = csr2,
    .sign_algo = GNUTLS_SIGN_RSA_SHA256,
    .sign_oid = "1.2.840.113549.1.1.11",
    .pk_algo = GNUTLS_PK_RSA,
    .pk_oid = "1.2.840.113549.1.1.1",
    .version = 1,
  },
  { NULL, NULL, 0, 0}
};

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "|<%d>| %s", level, str);
}

void doit(void)
{
	int exit_val = 0;
	size_t i;
	int ret;
	gnutls_x509_crq_t crq;
	gnutls_datum_t tmp;
	char oid[256];
	size_t oid_size;

	ret = global_init();
	if (ret != 0) {
		fail("%d: %s\n", ret, gnutls_strerror(ret));
		exit(1);
	}

	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(4711);

	for (i = 0; crq_list[i].name; i++) {

		if (debug)
			printf("Chain '%s' (%d)...\n", crq_list[i].name,
				(int) i);

		if (debug > 2)
			printf("\tAdding CRL...");

		ret = gnutls_x509_crq_init(&crq);
		if (ret < 0) {
			fprintf(stderr,
				"gnutls_x509_crq_init[%d]: %s\n",
				(int) i,
				gnutls_strerror(ret));
			exit(1);
		}

		tmp.data = (unsigned char*)crq_list[i].crq;
		tmp.size = strlen(crq_list[i].crq);

		ret =
		    gnutls_x509_crq_import(crq, &tmp,
					   GNUTLS_X509_FMT_PEM);
		if (debug > 2)
		printf("done\n");
		if (ret < 0) {
			fprintf(stderr,
				"gnutls_x509_crq_import[%s]: %s\n",
				crq_list[i].name,
				gnutls_strerror(ret));
			exit(1);
		}

		gnutls_x509_crq_print(crq,
				      GNUTLS_CRT_PRINT_ONELINE,
				      &tmp);
		if (debug)
			printf("\tCRL: %.*s\n", 
				tmp.size, tmp.data);
		gnutls_free(tmp.data);

		ret = gnutls_x509_crq_get_signature_algorithm(crq);
		if (ret != (int)crq_list[i].sign_algo) {
			fail("%s: error extracting signature algorithm: %d/%s\n", crq_list[i].name, ret, gnutls_strerror(ret));
			exit(1);
		}

		oid_size = sizeof(oid);
		ret = gnutls_x509_crq_get_signature_oid(crq, oid, &oid_size);
		if (ret < 0) {
			fail("%s: error extracting signature algorithm OID: %s\n", crq_list[i].name, gnutls_strerror(ret));
			exit(1);
		}

		if (strcmp(oid, crq_list[i].sign_oid) != 0) {
			fail("%s: error on the extracted signature algorithm: %s\n", crq_list[i].name, oid);
			exit(1);
		}

		/* PK */
		ret = gnutls_x509_crq_get_pk_algorithm(crq, NULL);
		if (ret != (int)crq_list[i].pk_algo) {
			fail("%s: error extracting PK algorithm: %d/%s\n", crq_list[i].name, ret, gnutls_strerror(ret));
			exit(1);
		}

		oid_size = sizeof(oid);
		ret = gnutls_x509_crq_get_pk_oid(crq, oid, &oid_size);
		if (ret < 0) {
			fail("%s: error extracting PK algorithm OID: %s\n", crq_list[i].name, gnutls_strerror(ret));
			exit(1);
		}

		if (strcmp(oid, crq_list[i].pk_oid) != 0) {
			fail("%s: error on the extracted PK algorithm: %s\n", crq_list[i].name, oid);
			exit(1);
		}

		ret = gnutls_x509_crq_get_version(crq);
		if (ret != (int)crq_list[i].version) {
			fail("%s: error on the extracted CRQ version: %d\n", crq_list[i].name, ret);
			exit(1);
		}

		gnutls_x509_crq_deinit(crq);

		if (debug)
			printf("done\n\n\n");
	}

	gnutls_global_deinit();

	if (debug)
		printf("Exit status...%d\n", exit_val);

	exit(exit_val);
}
