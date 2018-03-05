/*
 * Copyright (C) 2002-2012 Free Software Foundation, Inc.
 *
 * Author: Nikos Mavrogiannopoulos
 *
 * This file is part of GnuTLS-EXTRA.
 *
 * GnuTLS-extra is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.
 *
 * GnuTLS-extra is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GnuTLS-EXTRA; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

#ifndef GNUTLS_COMPAT8_H
#define GNUTLS_COMPAT8_H

/* Extra definitions */
#include <gnutls/openssl.h>

int gnutls_x509_extract_certificate_dn(const gnutls_datum_t *,
				       gnutls_x509_dn *);
int gnutls_x509_extract_certificate_issuer_dn(const gnutls_datum_t *,
					      gnutls_x509_dn *);

#endif
