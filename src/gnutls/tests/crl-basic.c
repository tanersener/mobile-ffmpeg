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

static const char simple1[] =
/* CRL */
"-----BEGIN X509 CRL-----\n"
"MIIBmjCBgwIBATANBgkqhkiG9w0BAQsFADAPMQ0wCwYDVQQDEwRDQS0wGA8yMDE0\n"
"MDkxMzA5MDUyM1oYDzIwMTUwOTEzMDkwNTIzWjAAoDowODAfBgNVHSMEGDAWgBTx\n"
"Fcb1FYLbhH9yuqg4wlVJUZnYKTAVBgNVHRQEDgIMVBQI0zeJoFHkLaWNMA0GCSqG\n"
"SIb3DQEBCwUAA4IBAQB6SGYePy1MBmtELyWdnlJHlQ4bBgb4vjuLDSfH0X6b4dAS\n"
"MEZws8iA5SaJFIioIP41s3lfQ1Am7GjSoNccHdrLmEcUSTQLwLYaDL8SgxekP5Au\n"
"w8HTu1cz/mnjBBDURq1RvyGNFm6MXf1Rg/bHSea/EpDkn8KY152BT1/46iQ+Uho6\n"
"hz6UUWsTB4Lj25X8F2hlKwQcb3E63Or2XEPBw4rhaCDFAtSZeBaGUUSJ8CLUKXZf\n"
"5b45MjiZ/osgd81tfn3wdQVjDnaQwNtjeRbK+qU0Z4pIKBvHzRS/fZKwTnrK1DLI\n"
"yY/nqBJT/+Q5zdUx5FXp0bwyZuarJ1GHqcES3Rz1\n"
"-----END X509 CRL-----\n";

static const char simple1_constraints[] = 
/* CRL */
"-----BEGIN X509 CRL-----\n"
"MIIBmjCBgwIBATANBgkqhkiG9w0BAQsFADAPMQ0wCwYDVQQDEwRDQS0wGA8yMDE0\n"
"MDkxMzA4NTkxNloYDzIwMTUwOTEzMDg1OTE2WjAAoDowODAfBgNVHSMEGDAWgBTn\n"
"sISO6h9leKTKtOh/HG0jV03AMzAVBgNVHRQEDgIMVBQHZC2mj6EAgMPSMA0GCSqG\n"
"SIb3DQEBCwUAA4IBAQBHUgtxpOn8EHwlajVYoOh6DFCwIoxBIeUA4518W1cHoV7J\n"
"KMif6lmJRodrcbienDX781QcOaQcNnuu/oBEcoBdbZa0VICzXekIteSwEgGsbRve\n"
"QQFPnZn83I4btse1ly5fdxMsliSM+qRwIyNR18VHXZz9GWYrr4tYWnI2b9XrDnaC\n"
"1b3Ywt7I9pNi0/O0C0rE/37/VvPx6HghnC+un7LtT0Y0n+FQP7dhlMvzHaR8wVxs\n"
"WAzaNvSiJ1rVPzL21iCmQJsRQeDTSJBlzm0lWiU8Nys3ugM2KlERezfp8DkFGA3y\n"
"9Yzpq6gAi39ZK+LjopgGDkrQjxzBIaoe2bcDqB7X\n"
"-----END X509 CRL-----\n";

static const char crl_dsa[] =
"-----BEGIN X509 CRL-----\n"
"MIGmMGUwCwYHKoZIzjgEAwUAMDgxCzAJBgNVBAYTAnVzMQwwCgYDVQQKEwNzdW4x\n"
"DDAKBgNVBAsTA2plczENMAsGA1UEAxMEZ3JlZxcNMDUwNTE3MTk1OTQwWhcNMDYw\n"
"NTE3MTk1OTQwWjALBgcqhkjOOAQDBQADMAAwLQIUBBFLGYjUCVrRTGf3GTR6SGs/\n"
"accCFQCUhnSmr+CXCWKq8DtydVwH9FLsRA==\n"
"-----END X509 CRL-----\n";

static const char crl_rsa_sha1[] =
"-----BEGIN X509 CRL-----\n"
"MIIB2zCBxAIBATANBgkqhkiG9w0BAQUFADBnMQswCQYDVQQGEwJOTjExMC8GA1UE\n"
"CgwoRWRlbCBDdXJsIEFyY3RpYyBJbGx1ZGl1bSBSZXNlYXJjaCBDbG91ZDElMCMG\n"
"A1UEAwwcTm90aGVybiBOb3doZXJlIFRydXN0IEFuY2hvchcNMTAwNTI3MjEzNzEx\n"
"WhcNMTAwNjI2MjEzNzExWjAZMBcCBguYlPl8ahcNMTAwNTI3MjEzNzExWqAOMAww\n"
"CgYDVR0UBAMCAQEwDQYJKoZIhvcNAQEFBQADggEBAFuPZJ/cNNCeAzkSxVvPPPRX\n"
"Wsv9T6Dt61C5Fmq9eSNN2kRf7/dq5A5nqTIlHbXXiLdj3UqNhUHXe2oA1UpbdHz9\n"
"0JlfwWm1Y/gMr1fh1n0oFebEtCuOgDRpd07Uiz8AqOUBykDNDUlMvVwR9raHL8hj\n"
"NRwzugsfIxl0CvLLqrBpUWMxW3qemk4cWW39yrDdZgKo6eOZAOR3FQYlLIrw6Jcr\n"
"Kmm0PjdcJIfRgJvNysgyx1dIIKe7QXvFTR/QzdHWIWTkiYIW7wUKSzSICvDCr094\n"
"eo3nr3n9BtOqT61Z1m6FGCP6Mm0wFl6xLTCNd6ygfFo7pcAdWlUsdBgKzics0Kc=\n"
"-----END X509 CRL-----\n";

static struct
{
  const char *name;
  const char *crl;
  unsigned sign_algo;
  const char *sign_oid;
  int crt_count;
  time_t next_update;
  time_t this_update;

  time_t crt_revoke_time;
  size_t crt_serial_size;
  const char *crt_serial;
} crl_list[] =
{
  { .name = "crl-sha256-1", 
    .crl = simple1,
    .sign_algo = GNUTLS_SIGN_RSA_SHA256,
    .sign_oid = "1.2.840.113549.1.1.11",
    .crt_count = 0,
    .this_update = 1410599123,
    .next_update = 1442135123
  },
  { .name = "crl-sha256-2",
    .crl = simple1_constraints,
    .sign_algo = GNUTLS_SIGN_RSA_SHA256,
    .sign_oid = "1.2.840.113549.1.1.11",
    .crt_count = 0,
    .this_update = 1410598756,
    .next_update = 1442134756
  },
  { .name = "crl-dsa",
    .crl = crl_dsa,
    .sign_algo = GNUTLS_SIGN_DSA_SHA1,
    .sign_oid = "1.2.840.10040.4.3",
    .crt_count = 0,
    .this_update = 1116359980,
    .next_update = 1147895980
  },
  { .name = "crl-rsa-sha1",
    .crl = crl_rsa_sha1,
    .sign_algo = GNUTLS_SIGN_RSA_SHA1,
    .sign_oid = "1.2.840.113549.1.1.5",
    .crt_count = 1,
    .this_update = 1274996231,
    .next_update = 1277588231,
    .crt_revoke_time = 1274996231,
    .crt_serial = "\x0b\x98\x94\xf9\x7c\x6a",
    .crt_serial_size = 6
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
	gnutls_x509_crl_t crl;
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

	for (i = 0; crl_list[i].name; i++) {

		if (debug)
			printf("Chain '%s' (%d)...\n", crl_list[i].name,
				(int) i);

		if (debug > 2)
			printf("\tAdding CRL...");

		ret = gnutls_x509_crl_init(&crl);
		if (ret < 0) {
			fprintf(stderr,
				"gnutls_x509_crl_init[%d]: %s\n",
				(int) i,
				gnutls_strerror(ret));
			exit(1);
		}

		tmp.data = (unsigned char*)crl_list[i].crl;
		tmp.size = strlen(crl_list[i].crl);

		ret =
		    gnutls_x509_crl_import(crl, &tmp,
					   GNUTLS_X509_FMT_PEM);
		if (debug > 2)
		printf("done\n");
		if (ret < 0) {
			fprintf(stderr,
				"gnutls_x509_crl_import[%s]: %s\n",
				crl_list[i].name,
				gnutls_strerror(ret));
			exit(1);
		}

		gnutls_x509_crl_print(crl,
				      GNUTLS_CRT_PRINT_ONELINE,
				      &tmp);
		if (debug)
			printf("\tCRL: %.*s\n", 
				tmp.size, tmp.data);
		gnutls_free(tmp.data);

		ret = gnutls_x509_crl_get_signature_algorithm(crl);
		if (ret != (int)crl_list[i].sign_algo) {
			fail("%s: error extracting signature algorithm: %d/%s\n", crl_list[i].name, ret, gnutls_strerror(ret));
			exit(1);
		}

		oid_size = sizeof(oid);
		ret = gnutls_x509_crl_get_signature_oid(crl, oid, &oid_size);
		if (ret < 0) {
			fail("%s: error extracting signature algorithm OID: %s\n", crl_list[i].name, gnutls_strerror(ret));
			exit(1);
		}

		if (strcmp(oid, crl_list[i].sign_oid) != 0) {
			fail("%s: error on the extracted signature algorithm: %s\n", crl_list[i].name, oid);
			exit(1);
		}

		ret = gnutls_x509_crl_get_crt_count(crl);
		if (ret != crl_list[i].crt_count) {
			fail("%s: error on the extracted CRT count: %d\n", crl_list[i].name, ret);
			exit(1);
		}

		if (crl_list[i].crt_count > 0) {
			unsigned char serial[128];
			size_t ssize = sizeof(serial);
			time_t t = 0;

			ret = gnutls_x509_crl_get_crt_serial(crl, 0, serial, &ssize, &t);
			if (ret < 0) {
				fail("%s: error on the extracted serial: %d\n", crl_list[i].name, ret);
			}

			if (t != crl_list[i].crt_revoke_time)
				fail("%s: error on the extracted revocation time: %u\n", crl_list[i].name, (unsigned)t);

			if (ssize != crl_list[i].crt_serial_size || memcmp(serial, crl_list[i].crt_serial, ssize) != 0) {
				for (i=0;i<ssize;i++)
					fprintf(stderr, "%.2x", (unsigned)serial[i]);
				fprintf(stderr, "\n");
				fail("%s: error on the extracted serial\n", crl_list[i].name);
			}
		}

		ret = gnutls_x509_crl_get_this_update(crl);
		if (ret != crl_list[i].this_update) {
			fail("%s: error on the extracted thisUpdate: %d\n", crl_list[i].name, ret);
			exit(1);
		}

		ret = gnutls_x509_crl_get_next_update(crl);
		if (ret != crl_list[i].next_update) {
			fail("%s: error on the extracted nextUpdate: %d\n", crl_list[i].name, ret);
			exit(1);
		}


		gnutls_x509_crl_deinit(crl);

		if (debug)
			printf("done\n\n\n");
	}

	gnutls_global_deinit();

	if (debug)
		printf("Exit status...%d\n", exit_val);

	exit(exit_val);
}
