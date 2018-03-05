/*
 * Copyright (C) 2016 Attila Molnar
 * Copyright (C) 2013-2016 Nikos Mavrogiannopoulos
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

/* This program tests if gnutls_certificate_set_ocsp_status_request_file()
 * leaks memory if called more than once with the same credentials.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <gnutls/gnutls.h>

#include "utils.h"

#if !defined(ENABLE_OCSP)

int main(int argc, char **argv)
{
	exit(77);
}

#else

void doit(void)
{
	gnutls_certificate_credentials_t x509_cred;

	gnutls_certificate_allocate_credentials(&x509_cred);
	/* The file does not need to exist for this test
	 */
	gnutls_certificate_set_ocsp_status_request_file
	    (x509_cred, "ocsp-status.der", 0);
	gnutls_certificate_set_ocsp_status_request_file
	    (x509_cred, "ocsp-status.der", 0);

	gnutls_certificate_free_credentials(x509_cred);
}

#endif
