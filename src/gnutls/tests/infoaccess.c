/*
 * Copyright (C) 2011-2012 Free Software Foundation, Inc.
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>

#include "utils.h"

static char cert_with_aia_data[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIF0DCCBLigAwIBAgIEOrZQizANBgkqhkiG9w0BAQUFADB/MQswCQYDVQQGEwJC\n"
    "TTEZMBcGA1UEChMQUXVvVmFkaXMgTGltaXRlZDElMCMGA1UECxMcUm9vdCBDZXJ0\n"
    "aWZpY2F0aW9uIEF1dGhvcml0eTEuMCwGA1UEAxMlUXVvVmFkaXMgUm9vdCBDZXJ0\n"
    "aWZpY2F0aW9uIEF1dGhvcml0eTAeFw0wMTAzMTkxODMzMzNaFw0yMTAzMTcxODMz\n"
    "MzNaMH8xCzAJBgNVBAYTAkJNMRkwFwYDVQQKExBRdW9WYWRpcyBMaW1pdGVkMSUw\n"
    "IwYDVQQLExxSb290IENlcnRpZmljYXRpb24gQXV0aG9yaXR5MS4wLAYDVQQDEyVR\n"
    "dW9WYWRpcyBSb290IENlcnRpZmljYXRpb24gQXV0aG9yaXR5MIIBIjANBgkqhkiG\n"
    "9w0BAQEFAAOCAQ8AMIIBCgKCAQEAv2G1lVO6V/z68mcLOhrfEYBklbTRvM16z/Yp\n"
    "li4kVEAkOPcahdxYTMukJ0KX0J+DisPkBgNbAKVRHnAEdOLB1Dqr1607BxgFjv2D\n"
    "rOpm2RgbaIr1VxqYuvXtdj182d6UajtLF8HVj71lODqV0D1VNk7feVcxKh7YWWVJ\n"
    "WCCYfqtffp/p1k3sg3Spx2zY7ilKhSoGFPlU5tPaZQeLYzcS19Dsw3sgQUSj7cug\n"
    "F+FxZc4dZjH3dgEZyH0DWLaVSR2mEiboxgx24ONmy+pdpibu5cxfvWenAScOospU\n"
    "xbF6lR1xHkopigPcakXBpBlebzbNw6Kwt/5cOOJSvPhEQ+aQuwIDAQABo4ICUjCC\n"
    "Ak4wPQYIKwYBBQUHAQEEMTAvMC0GCCsGAQUFBzABhiFodHRwczovL29jc3AucXVv\n"
    "dmFkaXNvZmZzaG9yZS5jb20wDwYDVR0TAQH/BAUwAwEB/zCCARoGA1UdIASCAREw\n"
    "ggENMIIBCQYJKwYBBAG+WAABMIH7MIHUBggrBgEFBQcCAjCBxxqBxFJlbGlhbmNl\n"
    "IG9uIHRoZSBRdW9WYWRpcyBSb290IENlcnRpZmljYXRlIGJ5IGFueSBwYXJ0eSBh\n"
    "c3N1bWVzIGFjY2VwdGFuY2Ugb2YgdGhlIHRoZW4gYXBwbGljYWJsZSBzdGFuZGFy\n"
    "ZCB0ZXJtcyBhbmQgY29uZGl0aW9ucyBvZiB1c2UsIGNlcnRpZmljYXRpb24gcHJh\n"
    "Y3RpY2VzLCBhbmQgdGhlIFF1b1ZhZGlzIENlcnRpZmljYXRlIFBvbGljeS4wIgYI\n"
    "KwYBBQUHAgEWFmh0dHA6Ly93d3cucXVvdmFkaXMuYm0wHQYDVR0OBBYEFItLbe3T\n"
    "KbkGGew5Oanwl4Rqy+/fMIGuBgNVHSMEgaYwgaOAFItLbe3TKbkGGew5Oanwl4Rq\n"
    "y+/foYGEpIGBMH8xCzAJBgNVBAYTAkJNMRkwFwYDVQQKExBRdW9WYWRpcyBMaW1p\n"
    "dGVkMSUwIwYDVQQLExxSb290IENlcnRpZmljYXRpb24gQXV0aG9yaXR5MS4wLAYD\n"
    "VQQDEyVRdW9WYWRpcyBSb290IENlcnRpZmljYXRpb24gQXV0aG9yaXR5ggQ6tlCL\n"
    "MA4GA1UdDwEB/wQEAwIBBjANBgkqhkiG9w0BAQUFAAOCAQEAitQUtf70mpKnGdSk\n"
    "fnIYj9lofFIk3WdvOXrEql494liwTXCYhGHoG+NpGA7O+0dQoE7/8CQfvbLO9Sf8\n"
    "7C9TqnN7Az10buYWnuulLsS/VidQK2K6vkscPFVcQR0kvoIgR13VRH56FmjffU1R\n"
    "cHhXHTMe/QKZnAzNCgVPx7uOpHX6Sm2xgI4JVrmcGmD+XcHXetwReNDWXcG31a0y\n"
    "mQM6isxUJTkxgXsTIlG6Rmyhu576BGxJJnSP0nPrzDCi5upZIof4l/UO/erMkqQW\n"
    "xFIY6iHOsfHmhIHluqmGKPJDWl0Snawe2ajlCmqnf6CHKc/yiU3U7MXi5nrQNiOK\n"
    "SnQ2+Q==\n" "-----END CERTIFICATE-----\n";

const gnutls_datum_t cert_with_aia = {
	(void *) cert_with_aia_data, sizeof(cert_with_aia_data)
};

void doit(void)
{
	gnutls_x509_crt_t crt;
	int ret;
	gnutls_datum_t data;
	unsigned int critical;

	ret = global_init();
	if (ret < 0) {
		fail("global_init\n");
		exit(1);
	}

	ret = gnutls_x509_crt_init(&crt);
	if (ret != 0) {
		fail("gnutls_x509_crt_init\n");
		exit(1);
	}

	ret =
	    gnutls_x509_crt_import(crt, &cert_with_aia,
				   GNUTLS_X509_FMT_PEM);
	if (ret < 0) {
		fail("gnutls_x509_crt_import\n");
		exit(1);
	}

	/* test null input */
	ret =
	    gnutls_x509_crt_get_authority_info_access(NULL, 0, 0, NULL,
						      NULL);
	if (ret != GNUTLS_E_INVALID_REQUEST) {
		fail("gnutls_x509_crt_get_authority_info_access null input\n");
		exit(1);
	}

	/* test unused enum */
	ret =
	    gnutls_x509_crt_get_authority_info_access(crt, 0, 44, NULL,
						      NULL);
	if (ret != GNUTLS_E_INVALID_REQUEST) {
		fail("gnutls_x509_crt_get_authority_info_access insane input\n");
		exit(1);
	}

	/* test basic query with null output */
	ret = gnutls_x509_crt_get_authority_info_access
	    (crt, 0, GNUTLS_IA_ACCESSMETHOD_OID, NULL, NULL);
	if (ret < 0) {
		fail("gnutls_x509_crt_get_authority_info_access "
		     "GNUTLS_IA_ACCESSMETHOD_OID null output critical\n");
		exit(1);
	}

	/* test same as previous but also check that critical flag is
	   correct */
	ret = gnutls_x509_crt_get_authority_info_access
	    (crt, 0, GNUTLS_IA_ACCESSMETHOD_OID, NULL, &critical);
	if (ret < 0) {
		fail("gnutls_x509_crt_get_authority_info_access "
		     "GNUTLS_IA_ACCESSMETHOD_OID null output\n");
		exit(1);
	}

	if (critical != 0) {
		fail("gnutls_x509_crt_get_authority_info_access "
		     "critical failed: %d\n", critical);
		exit(1);
	}

	/* basic query of another type */
	ret = gnutls_x509_crt_get_authority_info_access
	    (crt, 0, GNUTLS_IA_ACCESSLOCATION_GENERALNAME_TYPE, NULL,
	     NULL);
	if (ret < 0) {
		fail("gnutls_x509_crt_get_authority_info_access "
		     "GNUTLS_IA_ACCESSLOCATION_GENERALNAME_TYPE null output\n");
		exit(1);
	}

	/* basic query of another type, with out-of-bound sequence */
	ret = gnutls_x509_crt_get_authority_info_access
	    (crt, 1, GNUTLS_IA_ACCESSLOCATION_GENERALNAME_TYPE, NULL,
	     NULL);
	if (ret != GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE) {
		fail("gnutls_x509_crt_get_authority_info_access "
		     "GNUTLS_IA_ACCESSLOCATION_GENERALNAME_TYPE out-of-bounds\n");
		exit(1);
	}

	/* basic query and check output value */
	ret = gnutls_x509_crt_get_authority_info_access
	    (crt, 0, GNUTLS_IA_ACCESSMETHOD_OID, &data, NULL);
	if (ret < 0) {
		fail("gnutls_x509_crt_get_authority_info_access "
		     "GNUTLS_IA_ACCESSMETHOD_OID\n");
		exit(1);
	}

	if (memcmp("1.3.6.1.5.5.7.48.1", data.data, data.size) != 0) {
		fail("memcmp OCSP OID failed\n");
		exit(1);
	}
	gnutls_free(data.data);

	/* basic query of another type and check output value */
	ret = gnutls_x509_crt_get_authority_info_access
	    (crt, 0, GNUTLS_IA_ACCESSLOCATION_GENERALNAME_TYPE, &data,
	     NULL);
	if (ret < 0) {
		fail("gnutls_x509_crt_get_authority_info_access "
		     "GNUTLS_IA_ACCESSLOCATION_GENERALNAME_TYPE\n");
		exit(1);
	}

	if (memcmp("uniformResourceIdentifier", data.data, data.size) != 0) {
		fail("memcmp URI failed\n");
		exit(1);
	}
	gnutls_free(data.data);

	/* specific query */
	ret = gnutls_x509_crt_get_authority_info_access
	    (crt, 0, GNUTLS_IA_URI, &data, NULL);
	if (ret < 0) {
		fail("gnutls_x509_crt_get_authority_info_access GNUTLS_IA_URI\n");
		exit(1);
	}

	if (memcmp
	    ("https://ocsp.quovadisoffshore.com", data.data,
	     data.size) != 0) {
		fail("memcmp URI value failed\n");
		exit(1);
	}
	gnutls_free(data.data);

	/* even more specific query */
	ret = gnutls_x509_crt_get_authority_info_access
	    (crt, 0, GNUTLS_IA_OCSP_URI, &data, NULL);
	if (ret < 0) {
		fail("gnutls_x509_crt_get_authority_info_access GNUTLS_IA_OCSP_URI\n");
		exit(1);
	}

	if (memcmp
	    ("https://ocsp.quovadisoffshore.com", data.data,
	     data.size) != 0) {
		fail("memcmp URI value failed\n");
		exit(1);
	}
	gnutls_free(data.data);

	gnutls_x509_crt_deinit(crt);

	gnutls_global_deinit();

}
