/*
 * Copyright (C) 2006-2012 Free Software Foundation, Inc.
 * Author: Simon Josefsson, Howard Chu
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
#include "config.h"
#endif

#include <stdio.h>

#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#include "utils.h"

static char pem[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIE8jCCAtqgAwIBAgIDAkQVMA0GCSqGSIb3DQEBBQUAMHkxEDAOBgNVBAoTB1Jv\n"
    "b3QgQ0ExHjAcBgNVBAsTFWh0dHA6Ly93d3cuY2FjZXJ0Lm9yZzEiMCAGA1UEAxMZ\n"
    "Q0EgQ2VydCBTaWduaW5nIEF1dGhvcml0eTEhMB8GCSqGSIb3DQEJARYSc3VwcG9y\n"
    "dEBjYWNlcnQub3JnMB4XDTA2MDUxNTE1MjEzMVoXDTA3MDUxNTE1MjEzMVowPjEY\n"
    "MBYGA1UEAxMPQ0FjZXJ0IFdvVCBVc2VyMSIwIAYJKoZIhvcNAQkBFhNzaW1vbkBq\n"
    "b3NlZnNzb24ub3JnMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuddO\n"
    "vxr7gfof8Ejtk/EOC16m0UdymQhwQwfPM5wWKJCJK9l5GoXSHe+s/+6HPLhXo2do\n"
    "byUS6X3w7ODO6MGnlWALJUapUa2LinofYwYWFVlOlwyuN2lW+xQgeQjn24R8Glzl\n"
    "KQ2f5C9JOE3RSGnHr7VH/6JJy+rPovh+gqKHjt9UH6Su1LFEQGUg+x+CVPAluYty\n"
    "ECfHdAad2Gcbgn3vkMyKEF6VAKR/G9uDb7bBVuA73UWkUtDi3dekM882UqH5HQRj\n"
    "mGYoGJk49PQ52jGftXNIDyHDOYWXTl9W64dHKRGaW0LOrkLrodjMPdudTvSsoWzK\n"
    "DpMMdHLsFx2/+MAsPwIDAQABo4G9MIG6MAwGA1UdEwEB/wQCMAAwVgYJYIZIAYb4\n"
    "QgENBEkWR1RvIGdldCB5b3VyIG93biBjZXJ0aWZpY2F0ZSBmb3IgRlJFRSBoZWFk\n"
    "IG92ZXIgdG8gaHR0cDovL3d3dy5DQWNlcnQub3JnMDIGCCsGAQUFBwEBBCYwJDAi\n"
    "BggrBgEFBQcwAYYWaHR0cDovL29jc3AuY2FjZXJ0Lm9yZzAeBgNVHREEFzAVgRNz\n"
    "aW1vbkBqb3NlZnNzb24ub3JnMA0GCSqGSIb3DQEBBQUAA4ICAQCXhyNfM8ozU2Jw\n"
    "H+XEDgrt3lUgnUbXQC+AGXdj4ZIJXQfHOCCQxZOO6Oe9V0rxldO3M5tQi92yRjci\n"
    "aa892MCVPxTkJLR0h4Kx4JfeTtSvl+9nWPSRrZbPTdWZ3ecnCyrfLfEas6pZp1ur\n"
    "lJkaEksAg5dGNrvJGPqBbF6A44b1wlBTCHEBZy2n/7Qml7Nhydymq2nFhDtlQJ6X\n"
    "w+6juM85vaEII6kuNatk2OcMJG9R0JxbC0e+PPI1jk7wuAz4WIMyj+ZudGNOTWKN\n"
    "3ohK9v0/EE1/S+KMy3T7fzMkbKkwAQZzQNoDf8bSzvDwtZsoudA4Kcloz8a/iKEH\n"
    "C9nKYBU8sFBd1cYV7ocFhN2awvuVnBlfsEN4eO5TRA50hmLxwt5D8Vs2v55n1kl6\n"
    "7PBo6H2ZMfbQcws731k4RpOqQcU+2yl/wBlDChOOO95mbJ31tqMh27yIjIemgD6Z\n"
    "jxL92AgHPzSFy/nyqmZ1ADcnB5fC5WsEYyr9tPM1gpjJEsi95YIBrO7Uyt4tj5U3\n"
    "dYDvbU+Mg1r0gJi61wciuyAllwKfu9aqkCjJKQGHrTimWzRa6RPygaojWIEmap89\n"
    "bHarWgDg9CKVP1DggVkcD838s//kE1Vl2DReyfAtEQ1agSXLFncgxL+yOi1o3lcq\n"
    "+dmDgpDn168TY1Iug80uVKg7AfkLrA==\n" "-----END CERTIFICATE-----\n";

static void print_dn(gnutls_x509_dn_t dn)
{
	int i, j, ret = 0;
	gnutls_x509_ava_st ava;

	for (i = 0; ret == 0; i++)
		for (j = 0; ret == 0; j++) {
			ret = gnutls_x509_dn_get_rdn_ava(dn, i, j, &ava);
			if (ret == GNUTLS_E_ASN1_ELEMENT_NOT_FOUND) {
				if (j > 0) {
					j = 0;
					ret = 0;
				}
				break;
			}
			if (ret < 0)
				fail("get_rdn_ava %d\n", ret);
			if (debug)
				printf
				    ("dn[%d][%d] OID=%.*s\n\tDATA=%.*s\n",
				     i, j, ava.oid.size, ava.oid.data,
				     ava.value.size, ava.value.data);
		}
}

void doit(void)
{
	int ret;
	gnutls_datum_t pem_cert = { (unsigned char *) pem, sizeof(pem) };
	gnutls_x509_crt_t cert;
	gnutls_datum_t strdn;
	gnutls_x509_dn_t xdn;

	ret = global_init();
	if (ret < 0)
		fail("init %d\n", ret);

	ret = gnutls_x509_crt_init(&cert);
	if (ret < 0)
		fail("crt_init %d\n", ret);

	ret = gnutls_x509_crt_import(cert, &pem_cert, GNUTLS_X509_FMT_PEM);
	if (ret < 0)
		fail("crt_import %d\n", ret);

	ret = gnutls_x509_crt_get_issuer(cert, &xdn);
	if (ret < 0)
		fail("get_issuer %d\n", ret);

	if (debug) {
		printf("Issuer:\n");
		print_dn(xdn);
	}

	ret = gnutls_x509_crt_get_subject(cert, &xdn);
	if (ret < 0)
		fail("get_subject %d\n", ret);

	/* test the original function behavior */
	ret = gnutls_x509_dn_get_str(xdn, &strdn);
	if (ret < 0)
		fail("gnutls_x509_dn_get_str %d\n", ret);

	if (strdn.size != 44 || strcmp((char*)strdn.data, "CN=CAcert WoT User,EMAIL=simon@josefsson.org") != 0) {
		fail("gnutls_x509_dn_get_str string comparison failed: '%s'/%d\n", strdn.data, strdn.size);
	}
	gnutls_free(strdn.data);

	/* test the new function behavior */
	ret = gnutls_x509_dn_get_str2(xdn, &strdn, 0);
	if (ret < 0)
		fail("gnutls_x509_dn_get_str2 %d\n", ret);
	if (strdn.size != 44 || strcmp((char*)strdn.data, "EMAIL=simon@josefsson.org,CN=CAcert WoT User") != 0) {
		fail("gnutls_x509_dn_get_str2 string comparison failed: '%s'/%d\n", strdn.data, strdn.size);
	}
	gnutls_free(strdn.data);

	/* test the new/compat function behavior */
	ret = gnutls_x509_dn_get_str2(xdn, &strdn, GNUTLS_X509_DN_FLAG_COMPAT);
	if (ret < 0)
		fail("gnutls_x509_dn_get_str2 %d\n", ret);
	if (strdn.size != 44 || strcmp((char*)strdn.data, "CN=CAcert WoT User,EMAIL=simon@josefsson.org") != 0) {
		fail("gnutls_x509_dn_get_str2 string comparison failed: '%s'/%d\n", strdn.data, strdn.size);
	}
	gnutls_free(strdn.data);

	if (debug) {
		printf("Subject:\n");
		print_dn(xdn);
	}

	if (debug)
		success("done\n");

	gnutls_x509_crt_deinit(cert);
	gnutls_global_deinit();
}
