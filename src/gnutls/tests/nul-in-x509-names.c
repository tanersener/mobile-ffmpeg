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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>

#include "utils.h"

/* Thanks to Tomas Hoger <thoger@redhat.com> for generating the two
   certs that trigger this bug. */

static char badguy_nul_cn_data[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIDjTCCAnWgAwIBAgIBATANBgkqhkiG9w0BAQUFADB0MQswCQYDVQQGEwJHQjES\n"
    "MBAGA1UECBMJQmVya3NoaXJlMRAwDgYDVQQHEwdOZXdidXJ5MRcwFQYDVQQKEw5N\n"
    "eSBDb21wYW55IEx0ZDELMAkGA1UECxMCQ0ExGTAXBgNVBAMTEE5VTEwtZnJpZW5k\n"
    "bHkgQ0EwHhcNMDkwODA0MDczMzQzWhcNMTkwODAyMDczMzQzWjAjMSEwHwYDVQQD\n"
    "Exh3d3cuYmFuay5jb20ALmJhZGd1eS5jb20wggEiMA0GCSqGSIb3DQEBAQUAA4IB\n"
    "DwAwggEKAoIBAQDNJnCWqaZdPpztDwgVWnwXJWhorxO5rUH6ElTihHJ9WNHiQELB\n"
    "We0FPaoQU3AAiDp3oMBWnqx9ISpxRFEIvBcH2qijdtxRvBuK9gIaVb9GtERrJ16+\n"
    "5ReLVrLGgjYRg6i/9y8NF/bNR7VvK6ZBto0zX+rqi7Ea4pk4/1lbCqFxE8o3P7mw\n"
    "HpGayJM1DErgnfTSYcdOW0EKfDFUmdv1Zc6A08ICN2T9VBJ76qyFWVwX4S720Kjy\n"
    "0C6UWS/Cpl/aB957LhQH7eQnJDedCS6x+VpIuYAkQ+bLx24139VpNP/m1p7odmZu\n"
    "X1kBPJY77HILPB6VD85oE5wi3Ru1RChQSgV/AgMBAAGjezB5MAkGA1UdEwQCMAAw\n"
    "LAYJYIZIAYb4QgENBB8WHU9wZW5TU0wgR2VuZXJhdGVkIENlcnRpZmljYXRlMB0G\n"
    "A1UdDgQWBBQzFSS+2mY6BovZJzQ6r2JA5JVmXTAfBgNVHSMEGDAWgBQKaTlfnTAE\n"
    "GAguAg7m6p2yJvbiajANBgkqhkiG9w0BAQUFAAOCAQEAMmUjH8jZU4SC0ArrFFEk\n"
    "A7xsGypa/hvw6GkMKxmGz38ydtgr0s+LxNG2W5xgo5kuknIGzt6L0qLSiXwTqQtO\n"
    "vhIJ5dYoOqynJlaUfxPuZH3elGB1wbxVl9SqE44C2LCwcFOuGFPOqrIshT7j8+Em\n"
    "8/pc7vh7C8Y5tQQzXq64Xg5mzKjAag3sYMHF2TnqvRuPHH0WOLHoyDcBqkuZ3+QP\n"
    "EL5h7prPzScFRgBg2Gp0CDI8i5ABagczDGyQ2+r7ahcadrtzFCfhpH7V3TCxXfIO\n"
    "qtSy1Uz2T5EqB/Q3wc9IGcX+fpKWqN9QajGSo7EU/kHMSWKYTerFugUtScMicu9B\n"
    "CQ==\n" "-----END CERTIFICATE-----\n";

const gnutls_datum_t badguy_nul_cn = {
	(void *) badguy_nul_cn_data, sizeof(badguy_nul_cn_data)
};

static char badguy_nul_san_data[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIDrTCCApWgAwIBAgIBADANBgkqhkiG9w0BAQUFADB0MQswCQYDVQQGEwJHQjES\n"
    "MBAGA1UECBMJQmVya3NoaXJlMRAwDgYDVQQHEwdOZXdidXJ5MRcwFQYDVQQKEw5N\n"
    "eSBDb21wYW55IEx0ZDELMAkGA1UECxMCQ0ExGTAXBgNVBAMTEE5VTEwtZnJpZW5k\n"
    "bHkgQ0EwHhcNMDkwODA0MDY1MzA1WhcNMTkwODAyMDY1MzA1WjAZMRcwFQYDVQQD\n"
    "Ew53d3cuYmFkZ3V5LmNvbTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEB\n"
    "AM0mcJappl0+nO0PCBVafBclaGivE7mtQfoSVOKEcn1Y0eJAQsFZ7QU9qhBTcACI\n"
    "OnegwFaerH0hKnFEUQi8FwfaqKN23FG8G4r2AhpVv0a0RGsnXr7lF4tWssaCNhGD\n"
    "qL/3Lw0X9s1HtW8rpkG2jTNf6uqLsRrimTj/WVsKoXETyjc/ubAekZrIkzUMSuCd\n"
    "9NJhx05bQQp8MVSZ2/VlzoDTwgI3ZP1UEnvqrIVZXBfhLvbQqPLQLpRZL8KmX9oH\n"
    "3nsuFAft5CckN50JLrH5Wki5gCRD5svHbjXf1Wk0/+bWnuh2Zm5fWQE8ljvscgs8\n"
    "HpUPzmgTnCLdG7VEKFBKBX8CAwEAAaOBpDCBoTAJBgNVHRMEAjAAMCwGCWCGSAGG\n"
    "+EIBDQQfFh1PcGVuU1NMIEdlbmVyYXRlZCBDZXJ0aWZpY2F0ZTAdBgNVHQ4EFgQU\n"
    "MxUkvtpmOgaL2Sc0Oq9iQOSVZl0wHwYDVR0jBBgwFoAUCmk5X50wBBgILgIO5uqd\n"
    "sib24mowJgYDVR0RBB8wHYIbd3d3LmJhbmsuY29tAHd3dy5iYWRndXkuY29tMA0G\n"
    "CSqGSIb3DQEBBQUAA4IBAQAnbn2zqYZSV2qgxjBsHpQJp2+t/hGfvjKNAXuLlGbX\n"
    "fLaxkPzk9bYyvGxxI7EYiNZHvNoHx15GcTrmQG7Bfx1WlnBl2FGp3J6lBgCY5x4Q\n"
    "vIK6AOVOog8+7Irdb8bJweztbXwxPmaHR6GLFTwhfuwheD0hcHK6cMNk+B1P2dAn\n"
    "PD5+olmuvprTAESncjrjP8ibxY+xlP4AD264FIjxA1CRUa/wHve4WqRXNS3xrciu\n"
    "3SlhFH3q0TSAXBv960PcIW3GRPk7VHbEkVuspI5y59gk/6dawO8nw9fk+X9VjQ0w\n"
    "7KLZbch29L6UPRIySpFP28PndgdaEpcYtxUAmFkhiT41\n"
    "-----END CERTIFICATE-----\n";

const gnutls_datum_t badguy_nul_san = {
	(void *) badguy_nul_san_data, sizeof(badguy_nul_san_data)
};

void doit(void)
{
	gnutls_x509_crt_t crt;
	int ret;

	ret = global_init();
	if (ret < 0) {
		fail("global_init");
		exit(1);
	}

	ret = gnutls_x509_crt_init(&crt);
	if (ret != 0) {
		fail("gnutls_x509_crt_init");
		exit(1);
	}

	ret =
	    gnutls_x509_crt_import(crt, &badguy_nul_cn,
				   GNUTLS_X509_FMT_PEM);
	if (ret < 0) {
		fail("gnutls_x509_crt_import");
		exit(1);
	}

	ret = gnutls_x509_crt_check_hostname(crt, "www.bank.com");
	if (ret == 0) {
		if (debug)
			success
			    ("gnutls_x509_crt_check_hostname OK (NUL-IN-CN)");
	} else {
		fail("gnutls_x509_crt_check_hostname BROKEN (NUL-IN-CN)");
	}

	ret =
	    gnutls_x509_crt_import(crt, &badguy_nul_san,
				   GNUTLS_X509_FMT_PEM);
	if (ret < 0) {
		fail("gnutls_x509_crt_import");
		exit(1);
	}

	ret = gnutls_x509_crt_check_hostname(crt, "www.bank.com");
	if (ret == 0) {
		if (debug)
			success
			    ("gnutls_x509_crt_check_hostname OK (NUL-IN-SAN)");
	} else {
		fail("gnutls_x509_crt_check_hostname BROKEN (NUL-IN-SAN)");
	}

	gnutls_x509_crt_deinit(crt);

	gnutls_global_deinit();

}
