/*
 * Copyright (C) 2011-2012 Free Software Foundation, Inc.
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

/* Parts copied from GnuTLS example programs. */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#if !defined(_WIN32)
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#endif
#include <unistd.h>
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>

#include "utils.h"

/* Test for gnutls_certificate_get_issuer() and implicitly for
 * gnutls_trust_list_get_issuer().
 */

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "<%d>| %s", level, str);
}

/* This certificate is modified to contain invalid DER. In older
 * gnutls versions that would still be parsed and the wrong DER was
 * "corrected" but now we should reject these */
static unsigned char cert_pem[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIFXzCCBEegAwIBAgIQHYWDpKNVUzEFx4Pq8yjxbTANBgkqhkiG9w0BAQUFADCBtTELMAkGA1UE\n"
    "BhMCVVMxFzAVBgNVBAoTDlZlcmlTaWduLCBJbmMuMR8wHQYDVQQLExZWZXJpU2lnbiBUcnVzdCBO\n"
    "ZXR3b3JrMTswOQYDVQQLEzJUZXJtcyBvZiB1c2UgYXQgaHR0cHM6Ly93d3cudmVyaXNpZ24uY29t\n"
    "L3JwYSAoYykxMDEvMC0GA1UEAxMmVmVyaVNpZ24gQ2xhc3MgMyBTZWN1cmUgU2VydmVyIENBIC0g\n"
    "RzMwHxcOMTQwMjI3MDAwMDAwWgAXDTE1MDIyODIzNTk1OVowZzELMAkGA1UEBhMCVVMxEzARBgNV\n"
    "BAgTCldhc2hpbmd0b24xEDAOBgNVBAcUB1NlYXR0bGUxGDAWBgNVBAoUD0FtYXpvbi5jb20gSW5j\n"
    "LjEXMBUGA1UEAxQOd3d3LmFtYXpvbi5jb20wggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIB\n"
    "AQCXX4njj63+AK39SJXnf4ove+NO2Z46WgeccZuPUOD89/ucZg9C2K3uwo59QO1t2ZR5IucxVWaV\n"
    "vSW/9z30hA2ObJco5Cw9o3ZdoFXn0rYUmbWMW+XmL+/bSBDdFPQGfP1WhsFKJJfJ9TIrXBAsTSzH\n"
    "uC6qFZktvZ1yE0081+bdyOHVHjAQzSPsYFaSUqccMwPvy/sMaI+Um+GCf2PolJJwpI1+j6WmTEVg\n"
    "RBNHarxtNqpcV3rAFdJ5imL427agMqFur4Iz/OYeoCRBEiKk02ctRzoBaTvF09OQqRg3I4T9bE71\n"
    "xe1cdWo/sQ4nRiy1tfPBt+aBSiIRMh0Fdle780QFAgMBAAGjggG1MIIBsTBQBgNVHREESTBHghF1\n"
    "ZWRhdGEuYW1hem9uLmNvbYIKYW1hem9uLmNvbYIIYW16bi5jb22CDHd3dy5hbXpuLmNvbYIOd3d3\n"
    "LmFtYXpvbi5jb20wCQYDVR0TBAIwADAOBgNVHQ8BAf8EBAMCBaAwHQYDVR0lBBYwFAYIKwYBBQUH\n"
    "AwEGCCsGAQUFBwMCMEMGA1UdIAQ8MDowOAYKYIZIAYb4RQEHNjAqMCgGCCsGAQUFBwIBFhxodHRw\n"
    "czovL3d3dy52ZXJpc2lnbi5jb20vY3BzMB8GA1UdIwQYMBaAFA1EXBZTRMGCfh0gqyX0AWPYvnml\n"
    "MEUGA1UdHwQ+MDwwOqA4oDaGNGh0dHA6Ly9TVlJTZWN1cmUtRzMtY3JsLnZlcmlzaWduLmNvbS9T\n"
    "VlJTZWN1cmVHMy5jcmwwdgYIKwYBBQUHAQEEajBoMCQGCCsGAQUFBzABhhhodHRwOi8vb2NzcC52\n"
    "ZXJpc2lnbi5jb20wQAYIKwYBBQUHMAKGNGh0dHA6Ly9TVlJTZWN1cmUtRzMtYWlhLnZlcmlzaWdu\n"
    "LmNvbS9TVlJTZWN1cmVHMy5jZXIwDQYJKoZIhvcNAQEFBQADggEBADnmX45CNMkf57rQjB6ef7gf\n"
    "3r5AfKiGMYdSim4TwU5qcpJicYiyqwQXAQbvZFuZTGzT0jXJROLAsjdHcQiR8D5u7mzVMbJg0kz0\n"
    "yTsdDM5dFmVWme3l958NZI/I0qCtH+Z/O0cyivOTMARbBJ+92dqQ78U3He9gRNE9VCS3FNgObhwC\n"
    "cr5tkKTlgSESpSRyBwnLucY4+ci5xjvYndHIzoxII/X9TKOIc2sC+b0H5KP8RcQLAO9G5Nra7+eJ\n"
    "IC74ZgFvgejqTd2f8QeJljTsNxvG4P7vqQi73fCkTuVfCk5YDtTU2joGAujgBd1EjTIbjWYeoebV\n"
    "gN5gPKxa/GbGsoQ=\n"
    "-----END CERTIFICATE-----\n";

const gnutls_datum_t cert = { cert_pem, sizeof(cert_pem) - 1};

void doit(void)
{
	int ret;
	gnutls_x509_crt_t crt;

	/* this must be called once in the program
	 */
	global_init();

	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(6);

	gnutls_x509_crt_init(&crt);

	ret =
	    gnutls_x509_crt_import(crt, &cert, GNUTLS_X509_FMT_PEM);
	if (ret >= 0) {
		fail("gnutls_x509_crt_import allowed loading a cert with invalid DER\n");
		exit(1);
	}
	gnutls_x509_crt_deinit(crt);

	gnutls_global_deinit();

	if (debug)
		success("success");
}
