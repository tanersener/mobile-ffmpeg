/*
 * Copyright (C) 2015 Nikos Mavrogiannopoulos
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
#include <sys/types.h>
#include <unistd.h>
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>

#include "utils.h"

/* gnutls_trust_list_*().
 */

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "<%d>| %s", level, str);
}

/* this has a different signature algorithm in tbsCertificate and signatureAlgorithm.
 * the algorithm in signatureAlgorithm is wrong */
static unsigned char inconsistent_pem[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIICwzCCAXugAwIBAgIIVOei+gI+zMYwDQYJKoZIhvcNAQELBQAwGTEXMBUGA1UE\n"
    "AxMOR251VExTIFRlc3QgQ0EwIhgPMjAxNTAyMjAyMTExMjJaGA85OTk5MTIzMTIz\n"
    "NTk1OVowFzEVMBMGA1UEAxMMV3Jvbmcgc2lnYWxnMIGfMA0GCSqGSIb3DQEBAQUA\n"
    "A4GNADCBiQKBgQDKivjLpeml2GINsAimC6xwTxj44mLcxS+u69yFXFg2Z/AepUU+\n"
    "IvfqVOeRVgg1WHrh+DZLuoC6kwn7a2afUTzytrITKni+J14ENa/ZcF2MrhSM8WZ1\n"
    "NWrmvUltjkbJQIwyVPuIweRH1ECqSFxVqBT8RwYZ27FzTL8WF1JnlSlKuQIDAQAB\n"
    "o2EwXzAMBgNVHRMBAf8EAjAAMA8GA1UdDwEB/wQFAwMHoAAwHQYDVR0OBBYEFK9V\n"
    "bbSoqbHWgZwkzN57nbmAyyTwMB8GA1UdIwQYMBaAFE1Wt2oAWPFnkvSmdVUbjlMB\n"
    "A+/PMA0GCSqGSIb3DQEBBAUAA4IBMQCT2A88WEahnJgfXTjLbThqc/ICOg4dnk61\n"
    "zhaTkgK3is7T8gQrTqEbaVF4qu5gOLN6Z+xluii+ApZKKpKSyYLXS6MS3nJ6xGTi\n"
    "SOqixmPv7qfQnkUvUTagZymnWQ3GxRxjAv65YpmGyti+/TdkYWDQ9R/D/sWPJO8o\n"
    "YrFNw1ZXAaNMg4EhhGZ4likMlww+e5NPfJsJ32AovveTFKqSrvabb4UtrUJTwsC4\n"
    "Bd018g2MEhTkxeTQTqzIL98CoSBJjbbZD/YW13J/3xU590QpHTgni5hAni27IFLr\n"
    "1V+UJAglBs8qYiUzv/GjwbRt8TDzYVjvc+5MvPaGpoTcmdQyi9/L+3s8J6dX3i93\n"
    "TneIXeExwjTmXKL7NG+KQz9/F4FJChRXR6X1zsSB45DzoCoGMmzD\n"
    "-----END CERTIFICATE-----\n";

/* this has a different signature algorithm in tbsCertificate and signatureAlgorithm.
 * the algorithm in tbsCertificate is wrong */
static unsigned char inconsistent2_pem[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIC1DCCAYygAwIBAgIIVOem0AaBE/EwDQYJKoZIhvcNAQEEBQAwGTEXMBUGA1UE\n"
    "AxMOR251VExTIFRlc3QgQ0EwIhgPMjAxNTAyMjAyMTI3NDRaGA85OTk5MTIzMTIz\n"
    "NTk1OVowKDEmMCQGA1UEAxMdSW52YWxpZCB0YnNDZXJ0aWZpY2F0ZSBzaWdhbGcw\n"
    "gZ8wDQYJKoZIhvcNAQEBBQADgY0AMIGJAoGBAMqK+Mul6aXYYg2wCKYLrHBPGPji\n"
    "YtzFL67r3IVcWDZn8B6lRT4i9+pU55FWCDVYeuH4Nku6gLqTCftrZp9RPPK2shMq\n"
    "eL4nXgQ1r9lwXYyuFIzxZnU1aua9SW2ORslAjDJU+4jB5EfUQKpIXFWoFPxHBhnb\n"
    "sXNMvxYXUmeVKUq5AgMBAAGjYTBfMAwGA1UdEwEB/wQCMAAwDwYDVR0PAQH/BAUD\n"
    "AwegADAdBgNVHQ4EFgQUr1VttKipsdaBnCTM3nuduYDLJPAwHwYDVR0jBBgwFoAU\n"
    "TVa3agBY8WeS9KZ1VRuOUwED788wDQYJKoZIhvcNAQELBQADggExAEsjzyOB8ntk\n"
    "1BW4UhHdDSOZNrR4Ep0y2B3tjoOlXmcQD50WQb7NF/vYGeZN/y+WHEF9OAnneEIi\n"
    "5wRHLnm1jP/bXd5Po3EsaTLmpE7rW99DYlHaNRcF5z+a+qTdj7mRsnUtv6o2ItNT\n"
    "m81yQr0Lw0D31agU9IAzeXZy+Dm6dQnO1GAaHlOJQR1PZIOzOtYxqodla0qxuvga\n"
    "nL+quIR29t8nb7j+n8l1+2WxCUoxEO0wv37t3MQxjXUxzGfo5NDcXqH1364UBzdM\n"
    "rOBPX50B4LUyV5gNdWMIGVSMX3fTE+j3b+60w6NALXDzGoSGLQH48hpi/Mxzqctt\n"
    "gl58/RqS+nTNQ7c6QMhTj+dgaCE/DUGJJf0354dYp7p43nabr+ZtaMPUaGUQ/1UC\n"
    "C5/QFweC23w=\n"
    "-----END CERTIFICATE-----\n";

const gnutls_datum_t inconsistent = { inconsistent_pem, sizeof(inconsistent_pem)-1 };
const gnutls_datum_t inconsistent2 = { inconsistent2_pem, sizeof(inconsistent2_pem)-1 };

static time_t mytime(time_t * t)
{
	time_t then = 1424466893;

	if (t)
		*t = then;

	return then;
}

void doit(void)
{
	int ret;
	gnutls_x509_crt_t crt;

	/* this must be called once in the program
	 */
	global_init();

	gnutls_global_set_time_function(mytime);
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(6);

	gnutls_x509_crt_init(&crt);

	ret =
	    gnutls_x509_crt_import(crt, &inconsistent, GNUTLS_X509_FMT_PEM);
	if (ret >= 0)
		fail("1: gnutls_x509_crt_import");

	gnutls_x509_crt_deinit(crt);

	gnutls_x509_crt_init(&crt);

	ret =
	    gnutls_x509_crt_import(crt, &inconsistent2, GNUTLS_X509_FMT_PEM);
	if (ret >= 0)
		fail("2: gnutls_x509_crt_import");

	gnutls_x509_crt_deinit(crt);

	gnutls_global_deinit();

	if (debug)
		success("success");
}
