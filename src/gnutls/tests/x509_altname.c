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
    "MIIE6zCCA9OgAwIBAgIBdjANBgkqhkiG9w0BAQUFADBQMQswCQYDVQQGEwJTRTEf\n"
    "MB0GA1UEChMWU3RvY2tob2xtcyB1bml2ZXJzaXRldDEgMB4GA1UEAxMXU3RvY2to\n"
    "b2xtIFVuaXZlcnNpdHkgQ0EwHhcNMDYwMzIyMDkxNTI4WhcNMDcwMzIyMDkxNTI4\n"
    "WjBDMQswCQYDVQQGEwJTRTEfMB0GA1UEChMWU3RvY2tob2xtcyB1bml2ZXJzaXRl\n"
    "dDETMBEGA1UEAxMKc2lwMS5zdS5zZTCBnzANBgkqhkiG9w0BAQEFAAOBjQAwgYkC\n"
    "gYEArUzXTD36ZK7CwZJH/faUNTcdaqM7JyiZsfrO703d7cT/bJ3wKxT8trOOh/Ou\n"
    "WwgGFX2+r7ykun3aIUXUuD13Yle/yHqH/4g9vWX7UeFCBlSI0tAxnlqt0QqlPgSd\n"
    "GLHcoO4PPyjon9jj0A/zpJGZHiRUCooo63YqE9MYfr5HBfkCAwEAAaOCAl8wggJb\n"
    "MAsGA1UdDwQEAwIF4DAdBgNVHSUEFjAUBggrBgEFBQcDAQYIKwYBBQUHAwIwHQYD\n"
    "VR0OBBYEFDpcXNHMLJ7fc/c72BtZseq4MDXFMH8GA1UdIwR4MHaAFJ4uMLo32VFE\n"
    "yZ2/GCHxvX7utYZIoVukWTBXMQswCQYDVQQGEwJTRTEYMBYGA1UEChMPVW1lYSBV\n"
    "bml2ZXJzaXR5MRMwEQYDVQQLEwpTd1VQS0ktUENBMRkwFwYDVQQDExBTd1VQS0kg\n"
    "UG9saWN5IENBggEQMDIGA1UdHwQrMCkwJ6AloCOGIWh0dHA6Ly9jYS5zdS5zZS8y\n"
    "MDA1LTEvY3JsLXYyLmNybDB5BgNVHSAEcjBwMG4GCCqFcCsCAQEBMGIwHwYIKwYB\n"
    "BQUHAgEWE2h0dHA6Ly9jYS5zdS5zZS9DUFMwPwYIKwYBBQUHAgIwMxoxTGltaXRl\n"
    "ZCBMaWFiaWxpdHksIHNlZSBodHRwOi8vd3d3LnN3dXBraS5zdS5zZS9DUDAkBgNV\n"
    "HRIEHTAbgQhjYUBzdS5zZYYPaHR0cDovL2NhLnN1LnNlMIG3BgNVHREEga8wgayC\n"
    "F2luY29taW5ncHJveHkuc2lwLnN1LnNlghhpbmNvbWluZ3Byb3h5MS5zaXAuc3Uu\n"
    "c2WCF291dGdvaW5ncHJveHkuc2lwLnN1LnNlghhvdXRnb2luZ3Byb3h5MS5zaXAu\n"
    "c3Uuc2WCDW91dC5zaXAuc3Uuc2WCE2FwcHNlcnZlci5zaXAuc3Uuc2WCFGFwcHNl\n"
    "cnZlcjEuc2lwLnN1LnNlggpzaXAxLnN1LnNlMA0GCSqGSIb3DQEBBQUAA4IBAQAR\n"
    "FYg7ytcph0E7WmvM44AN/8qru7tRX6aSFWrjLyVr/1Wk4prCK4y5JpfNw5dh9Z8f\n"
    "/gyFsr1iFsb6fS3nJTTd3fVlWRfcNCGIx5g8KuSb3u6f7VznkGOeiRMRESQc1G8B\n"
    "eh0zbdZS7BYO2g9EKlbGST5PwQnc4g9K7pqPyKSNVkzb60Nujg/+qYje7MCcN+ZR\n"
    "nUBo6U2NZ06/QEUFm+uUIhZ8IGM1gLehC7Q3G4+d4c38CDJxQnSPOgWiXuSvhhQm\n"
    "KDsbrKzRaeBRh5eEJbTkA8Dp0Emb0UrkRVhixeg97stxUcATAjdGljJ9MLnuHXnI\n"
    "7ihGdUfg5q/105vpsQpO\n" "-----END CERTIFICATE-----\n";

#define MAX_DATA_SIZE 1024

void doit(void)
{
	int ret;
	gnutls_datum_t derCert = { (void *) pem, sizeof(pem) };
	gnutls_x509_crt_t cert;
	size_t data_len = MAX_DATA_SIZE;
	char data[MAX_DATA_SIZE];
	unsigned int critical = 0;
	int alt_name_count = 0;

	ret = global_init();
	if (ret < 0)
		fail("init %d\n", ret);

	ret = gnutls_x509_crt_init(&cert);
	if (ret < 0)
		fail("crt_init %d\n", ret);

	ret = gnutls_x509_crt_import(cert, &derCert, GNUTLS_X509_FMT_PEM);
	if (ret < 0)
		fail("crt_import %d\n", ret);

	for (alt_name_count = 0;; ++alt_name_count) {
		ret =
		    gnutls_x509_crt_get_issuer_alt_name(cert,
							alt_name_count,
							data, &data_len,
							&critical);
		if (ret == GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE)
			break;

		if (ret < 0)
			fail("get_issuer_alt_name: %d\n", ret);

		// TODO: print out / check results
		if (GNUTLS_SAN_URI == ret) {
			if (strcmp(data, "http://ca.su.se") != 0) {
				fail("unexpected issuer GNUTLS_SAN_URI: %s\n", data);
			}
		} else if (GNUTLS_SAN_RFC822NAME == ret) {
			if (strcmp(data, "ca@su.se") != 0) {
				fail("unexpected issuer GNUTLS_SAN_RFC822NAME: %s\n", data);
			}
		} else {
			fail("unexpected alt name type: %d\n", ret);
		}
		data_len = MAX_DATA_SIZE;
	}

	if (alt_name_count != 2) {
		fail("unexpected number of alt names: %i\n",
		     alt_name_count);
	}

	if (debug)
		success("done\n");

	gnutls_x509_crt_deinit(cert);
	gnutls_global_deinit();
}
