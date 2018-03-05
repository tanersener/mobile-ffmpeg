/*
 * Copyright (C) 2006-2012 Free Software Foundation, Inc.
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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <utils.h>
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>

static char g_crl[] =
    "-----BEGIN X509 CRL-----\n"
    "MIIB9DCCAV8CAQEwCwYJKoZIhvcNAQEFMIIBCDEXMBUGA1UEChMOVmVyaVNpZ24s\n"
    "IEluYy4xHzAdBgNVBAsTFlZlcmlTaWduIFRydXN0IE5ldHdvcmsxRjBEBgNVBAsT\n"
    "PXd3dy52ZXJpc2lnbi5jb20vcmVwb3NpdG9yeS9SUEEgSW5jb3JwLiBieSBSZWYu\n"
    "LExJQUIuTFREKGMpOTgxHjAcBgNVBAsTFVBlcnNvbmEgTm90IFZhbGlkYXRlZDEm\n"
    "MCQGA1UECxMdRGlnaXRhbCBJRCBDbGFzcyAxIC0gTmV0c2NhcGUxGDAWBgNVBAMU\n"
    "D1NpbW9uIEpvc2Vmc3NvbjEiMCAGCSqGSIb3DQEJARYTc2ltb25Aam9zZWZzc29u\n"
    "Lm9yZxcNMDYxMjI3MDgwMjM0WhcNMDcwMjA3MDgwMjM1WjAjMCECEC4QNwPfRoWd\n"
    "elUNpllhhTgXDTA2MTIyNzA4MDIzNFowCwYJKoZIhvcNAQEFA4GBAD0zX+J2hkcc\n"
    "Nbrq1Dn5IKL8nXLgPGcHv1I/le1MNo9t1ohGQxB5HnFUkRPAY82fR6Epor4aHgVy\n"
    "b+5y+neKN9Kn2mPF4iiun+a4o26CjJ0pArojCL1p8T0yyi9Xxvyc/ezaZ98HiIyP\n"
    "c3DGMNR+oUmSjKZ0jIhAYmeLxaPHfQwR\n" "-----END X509 CRL-----\n";

/* Test regression of bug reported by Max Kellermann <max@duempel.org>
   in Message-ID: <20061211075202.GA1517@roonstrasse.net> to the
   gnutls-dev@gnupg.org list. */

int main(void)
{
	int rc;
	gnutls_certificate_credentials_t crt;
	gnutls_datum_t crldatum = { (uint8_t *) g_crl, strlen(g_crl) };
	gnutls_x509_crl_t crl;

	rc = global_init();
	if (rc) {
		printf("global_init rc %d: %s\n", rc, gnutls_strerror(rc));
		return 1;
	}

	rc = gnutls_certificate_allocate_credentials(&crt);
	if (rc) {
		printf
		    ("gnutls_certificate_allocate_credentials rc %d: %s\n",
		     rc, gnutls_strerror(rc));
		return 1;
	}

	rc = gnutls_certificate_set_x509_crl_mem(crt, &crldatum,
						 GNUTLS_X509_FMT_PEM);
	if (rc != 1) {
		printf("gnutls_certificate_set_x509_crl_mem num %d\n", rc);
		return 1;
	}

	rc = gnutls_x509_crl_init(&crl);
	if (rc) {
		printf("gnutls_x509_crl_init rc %d: %s\n", rc,
			gnutls_strerror(rc));
		return 1;
	}

	rc = gnutls_x509_crl_import(crl, &crldatum, GNUTLS_X509_FMT_PEM);
	if (rc) {
		printf("gnutls_x509_crl_import rc %d: %s\n", rc,
			gnutls_strerror(rc));
		return 1;
	}

	rc = gnutls_certificate_set_x509_crl(crt, &crl, 1);
	if (rc < 0) {
		printf("gnutls_certificate_set_x509_crl rc %d: %s\n",
			rc, gnutls_strerror(rc));
		return 1;
	}

	gnutls_x509_crl_deinit(crl);

	gnutls_certificate_free_credentials(crt);

	gnutls_global_deinit();

	return 0;
}
