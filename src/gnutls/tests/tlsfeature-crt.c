/*
 * Copyright (C) 2016 Red Hat, Inc.
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
#include <gnutls/x509-ext.h>
#include <assert.h>
#include "utils.h"

static unsigned char server_cert_pem[] =
		"-----BEGIN CERTIFICATE-----\n"
		"MIICBzCCAXCgAwIBAgIMVpjt8TL5Io/frpvkMA0GCSqGSIb3DQEBCwUAMCIxIDAe\n"
		"BgNVBAMTF0dudVRMUyB0ZXN0IGNlcnRpZmljYXRlMB4XDTE2MDExNTEzMDI0MVoX\n"
		"DTMyMDYxOTEzMDI0MVowIjEgMB4GA1UEAxMXR251VExTIHRlc3QgY2VydGlmaWNh\n"
		"dGUwgZ8wDQYJKoZIhvcNAQEBBQADgY0AMIGJAoGBANe6XK+jDPAuqSdWqlOOqOt/\n"
		"gXVMa5i+Suq3HvhLw2rE2g0AuOpMEx82FpPecu/GpF6ybrbKCohVdZCW7aZXvAw7\n"
		"dg2XHr3p7H/Tqez7hWSga6BIznd+c5wxE/89yK6lYG7Ztoxamm+2vp9qvafwoDMn\n"
		"9bcdkuWWnHNS1p/WyI6xAgMBAAGjQjBAMBEGCCsGAQUFBwEYBAUwAwIBBTAMBgNV\n"
		"HRMBAf8EAjAAMB0GA1UdDgQWBBRTSzvcXshETAIgvzlIb0z+zSVSEDANBgkqhkiG\n"
		"9w0BAQsFAAOBgQB+VcJuLPL2PMog0HZ8RRbqVvLU5d209ROg3s1oXUBFW8+AV+71\n"
		"CsHg9Xx7vqKVwyKGI9ghds1B44lNPxGH2Sk1v2czjKbzwujo9+kLnDS6i0jyrDdn\n"
		"um4ivpkwmlUFSQVXvENLwe9gTlIgN4+0I9WLcMTCDtHWkcxMRwCm2BMsXw==\n"
		"-----END CERTIFICATE-----\n";

const gnutls_datum_t server_cert = { server_cert_pem,
	sizeof(server_cert_pem)
};

void doit(void)
{
	int ret;
	gnutls_x509_tlsfeatures_t feat;
	unsigned int out;
	gnutls_x509_crt_t crt;
	unsigned critical = 5;

	ret = global_init();
	if (ret < 0)
		fail("init %d\n", ret);

	assert(gnutls_x509_tlsfeatures_init(&feat) >= 0);
	assert(gnutls_x509_crt_init(&crt) >= 0);

	assert(gnutls_x509_crt_import(crt, &server_cert, GNUTLS_X509_FMT_PEM) >= 0);


	assert(gnutls_x509_crt_get_tlsfeatures(crt, feat, 0, &critical) >= 0);
	assert(critical == 0);

	assert(gnutls_x509_tlsfeatures_get(feat, 0, &out) >= 0);
	assert(out == 5);

	assert(gnutls_x509_tlsfeatures_get(feat, 1, &out) == GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE);

	assert(gnutls_x509_tlsfeatures_check_crt(feat, crt) != 0);

	/* append more features */
	assert(gnutls_x509_tlsfeatures_add(feat, 6) >= 0);
	assert(gnutls_x509_tlsfeatures_check_crt(feat, crt) == 0);

	assert(gnutls_x509_tlsfeatures_add(feat, 8) >= 0);
	assert(gnutls_x509_tlsfeatures_check_crt(feat, crt) == 0);

	gnutls_x509_tlsfeatures_deinit(feat);

	/* check whether a single TLSFeat with another value will fail verification */
	assert(gnutls_x509_tlsfeatures_init(&feat) >= 0);

	assert(gnutls_x509_tlsfeatures_add(feat, 8) >= 0);
	assert(gnutls_x509_tlsfeatures_check_crt(feat, crt) == 0);

	gnutls_x509_tlsfeatures_deinit(feat);
	gnutls_x509_crt_deinit(crt);
	gnutls_global_deinit();
}

