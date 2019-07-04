/*
 * Copyright (C) 2009-2012 Free Software Foundation, Inc.
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
#include "config.h"
#endif

#include <stdio.h>

#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#include "utils.h"

static char pem[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIFpzCCBI+gAwIBAgIQSOyh48ZYvgTFR8HspnpkMzANBgkqhkiG9w0BAQUFADCB\n"
    "vjELMAkGA1UEBhMCVVMxFzAVBgNVBAoTDlZlcmlTaWduLCBJbmMuMR8wHQYDVQQL\n"
    "ExZWZXJpU2lnbiBUcnVzdCBOZXR3b3JrMTswOQYDVQQLEzJUZXJtcyBvZiB1c2Ug\n"
    "YXQgaHR0cHM6Ly93d3cudmVyaXNpZ24uY29tL3JwYSAoYykwNjE4MDYGA1UEAxMv\n"
    "VmVyaVNpZ24gQ2xhc3MgMyBFeHRlbmRlZCBWYWxpZGF0aW9uIFNTTCBTR0MgQ0Ew\n"
    "HhcNMDgxMTEzMDAwMDAwWhcNMDkxMTEzMjM1OTU5WjCB6zETMBEGCysGAQQBgjc8\n"
    "AgEDEwJERTEZMBcGCysGAQQBgjc8AgEBFAhNdWVuY2hlbjEbMBkGA1UEDxMSVjEu\n"
    "MCwgQ2xhdXNlIDUuKGIpMRMwEQYDVQQFEwpIUkIgMTQ0MjYxMQswCQYDVQQGEwJE\n"
    "RTEOMAwGA1UEERQFODA4MDcxEDAOBgNVBAgTB0JhdmFyaWExETAPBgNVBAcUCE11\n"
    "ZW5jaGVuMR0wGwYDVQQJFBRGcmFua2Z1cnRlciBSaW5nIDEyOTERMA8GA1UEChQI\n"
    "R01YIEdtYkgxEzARBgNVBAMUCnd3dy5nbXguZGUwgZ8wDQYJKoZIhvcNAQEBBQAD\n"
    "gY0AMIGJAoGBAN/ZbLu17YtZo2OGnOfQDwhQlCvks2c+5nJDXjnCHI/ykSGlPH4G\n"
    "5qc7/TScNV1/g0bUTRCA11+aVkvf6haRZfgwbxpY1iySNv8eOlm52QAfh3diJQ9N\n"
    "5LxQblLHMRxXSFCJThl4BYAt70YdNMT9mVD21xx6ae+m3xEuco31aV7ZAgMBAAGj\n"
    "ggH0MIIB8DAJBgNVHRMEAjAAMB0GA1UdDgQWBBTW4UAZN3wEg5TRWaoM1angbgOX\n"
    "tjALBgNVHQ8EBAMCBaAwRAYDVR0gBD0wOzA5BgtghkgBhvhFAQcXBjAqMCgGCCsG\n"
    "AQUFBwIBFhxodHRwczovL3d3dy52ZXJpc2lnbi5jb20vcnBhMD4GA1UdHwQ3MDUw\n"
    "M6AxoC+GLWh0dHA6Ly9FVkludGwtY3JsLnZlcmlzaWduLmNvbS9FVkludGwyMDA2\n"
    "LmNybDAoBgNVHSUEITAfBggrBgEFBQcDAQYIKwYBBQUHAwIGCWCGSAGG+EIEATAf\n"
    "BgNVHSMEGDAWgBROQ8gddu83U3pP8lhvlPM44tW93zB2BggrBgEFBQcBAQRqMGgw\n"
    "KwYIKwYBBQUHMAGGH2h0dHA6Ly9FVkludGwtb2NzcC52ZXJpc2lnbi5jb20wOQYI\n"
    "KwYBBQUHMAKGLWh0dHA6Ly9FVkludGwtYWlhLnZlcmlzaWduLmNvbS9FVkludGwy\n"
    "MDA2LmNlcjBuBggrBgEFBQcBDARiMGChXqBcMFowWDBWFglpbWFnZS9naWYwITAf\n"
    "MAcGBSsOAwIaBBRLa7kolgYMu9BSOJsprEsHiyEFGDAmFiRodHRwOi8vbG9nby52\n"
    "ZXJpc2lnbi5jb20vdnNsb2dvMS5naWYwDQYJKoZIhvcNAQEFBQADggEBAKpNJQYO\n"
    "JTp34I24kvRF01WpOWOmfBx4K1gqruda/7U0UZqgTgBJVvwraKf6WeTZpHRqDCTw\n"
    "iwySv7jil+gLMT0qIZxL1pII90z71tz08h8xYi1MOLeciG87O9C5pteL/iEtiMxB\n"
    "96B6WWBo9mzgwSM1d8LDhrarZ7uQhm+kBAMyEXhmDnCPWhvExvxJzjEmOlxjThyP\n"
    "2yvIgfLyDfplRe+jUbsY7YNe08eEyoLRq1jwPuRWTaEx2gA7C6pq45747/HkJrtF\n"
    "ya3ULM/AJv6Nj6pobxzQ5rEkUGEwKavu7GMjLrSMnHrbVCiQrn1v6c7B9nSPA31L\n"
    "/do1TDFI0vSl5+M=\n" "-----END CERTIFICATE-----\n";

static const char *info =
    "subject `CN=www.gmx.de,O=GMX GmbH,street=Frankfurter Ring 129,L=Muenchen,ST=Bavaria,postalCode=80807,C=DE,serialNumber=HRB 144261,businessCategory=V1.0\\, Clause 5.(b),jurisdictionOfIncorporationLocalityName=Muenchen,jurisdictionOfIncorporationCountryName=DE', issuer `CN=VeriSign Class 3 Extended Validation SSL SGC CA,OU=Terms of use at https://www.verisign.com/rpa (c)06,OU=VeriSign Trust Network,O=VeriSign\\, Inc.,C=US', serial 0x48eca1e3c658be04c547c1eca67a6433, RSA key 1024 bits, signed using RSA-SHA1 (broken!), activated `2008-11-13 00:00:00 UTC', expires `2009-11-13 23:59:59 UTC', pin-sha256=\"sVjloAiiqTbOeTkJWYtVweNaVPijLP/X95L96gJOSvk=\"";

void doit(void)
{
	gnutls_datum_t pem_cert = { (void *) pem, sizeof(pem) };
	gnutls_x509_crt_t cert;
	gnutls_datum_t out;
	int ret;

	ret = global_init();
	if (ret < 0)
		fail("init %d\n", ret);

	ret = gnutls_x509_crt_init(&cert);
	if (ret < 0)
		fail("crt_init %d\n", ret);

	ret = gnutls_x509_crt_import(cert, &pem_cert, GNUTLS_X509_FMT_PEM);
	if (ret < 0)
		fail("crt_import %d\n", ret);

	ret = gnutls_x509_crt_print(cert, GNUTLS_CRT_PRINT_ONELINE, &out);
	if (ret < 0)
		fail("x509_crt_print %d\n", ret);

/* When allowing SHA1, the output is different: no broken! string */
#ifndef ALLOW_SHA1
	if (out.size != strlen(info) ||
	    strcasecmp((char *) out.data, info) != 0) {
		fprintf(stderr, "comparison fail (%d/%d)\nexpected: %s\n\n   got: %.*s\n\n",
		     out.size, (int) strlen(info), info, out.size,
		     out.data);
		fail("comparison failed\n");
	}
#endif

	gnutls_x509_crt_deinit(cert);
	gnutls_global_deinit();
	gnutls_free(out.data);

	if (debug)
		success("done\n");
}
