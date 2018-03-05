/*
 * Copyright (C) 2008-2012 Free Software Foundation, Inc.
 *
 * Author: Joe Orton
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
#include <unistd.h>

#include <gnutls/gnutls.h>
#include <gnutls/x509.h>

#include "utils.h"

static const char cert_pem[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIICHjCCAYmgAwIBAgIERiYdNzALBgkqhkiG9w0BAQUwGTEXMBUGA1UEAxMOR251\n"
    "VExTIHRlc3QgQ0EwHhcNMDcwNDE4MTMyOTI3WhcNMDgwNDE3MTMyOTI3WjAdMRsw\n"
    "GQYDVQQDExJHbnVUTFMgdGVzdCBjbGllbnQwgZwwCwYJKoZIhvcNAQEBA4GMADCB\n"
    "iAKBgLtmQ/Xyxde2jMzF3/WIO7HJS2oOoa0gUEAIgKFPXKPQ+GzP5jz37AR2ExeL\n"
    "ZIkiW8DdU3w77XwEu4C5KL6Om8aOoKUSy/VXHqLnu7czSZ/ju0quak1o/8kR4jKN\n"
    "zj2AC41179gAgY8oBAOgIo1hBAf6tjd9IQdJ0glhaZiQo1ipAgMBAAGjdjB0MAwG\n"
    "A1UdEwEB/wQCMAAwEwYDVR0lBAwwCgYIKwYBBQUHAwIwDwYDVR0PAQH/BAUDAweg\n"
    "ADAdBgNVHQ4EFgQUTLkKm/odNON+3svSBxX+odrLaJEwHwYDVR0jBBgwFoAU6Twc\n"
    "+62SbuYGpFYsouHAUyfI8pUwCwYJKoZIhvcNAQEFA4GBALujmBJVZnvaTXr9cFRJ\n"
    "jpfc/3X7sLUsMvumcDE01ls/cG5mIatmiyEU9qI3jbgUf82z23ON/acwJf875D3/\n"
    "U7jyOsBJ44SEQITbin2yUeJMIm1tievvdNXBDfW95AM507ShzP12sfiJkJfjjdhy\n"
    "dc8Siq5JojruiMizAf0pA7in\n" "-----END CERTIFICATE-----\n";
static const gnutls_datum_t cert_datum = { (unsigned char *) cert_pem,
	sizeof(cert_pem)
};

void doit(void)
{
	gnutls_x509_crt_t cert;
	gnutls_x509_dn_t sdn, dn2;
	unsigned char buf[8192], buf2[8192];
	size_t buflen, buf2len;
	gnutls_datum_t datum;
	int rv;

	global_init();

	if (gnutls_x509_crt_init(&cert) != 0)
		fail("cert init failure\n");

	if (gnutls_x509_crt_import(cert, &cert_datum, GNUTLS_X509_FMT_PEM)
	    != 0)
		fail("FAIL: could not import PEM cert\n");

	if (gnutls_x509_crt_get_subject(cert, &sdn) != 0)
		fail("FAIL: could not get subject DN.\n");

	buflen = sizeof buf;
	rv = gnutls_x509_dn_export(sdn, GNUTLS_X509_FMT_DER, buf, &buflen);
	if (rv != 0)
		fail("FAIL: could not export subject DN: %s\n",
		     gnutls_strerror(rv));

	if (gnutls_x509_dn_init(&dn2) != 0)
		fail("FAIL: DN init.\n");

	datum.data = buf;
	datum.size = buflen;

	if (gnutls_x509_dn_import(dn2, &datum) != 0)
		fail("FAIL: re-import subject DN.\n");

	buf2len = sizeof buf2;
	rv = gnutls_x509_dn_export(dn2, GNUTLS_X509_FMT_DER, buf2,
				   &buf2len);
	if (rv != 0)
		fail("FAIL: could not export subject DN: %s\n",
		     gnutls_strerror(rv));

	if (buflen == buf2len && memcmp(buf, buf2, buflen) != 0)
		fail("FAIL: export/import/export differ.\n");

	gnutls_x509_dn_deinit(dn2);

	gnutls_x509_crt_deinit(cert);

	gnutls_global_deinit();
}
