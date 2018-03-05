/*
 * Copyright (C) 2000-2012 Free Software Foundation, Inc.
 *
 * Author: Nikos Mavrogiannopoulos
 *
 * This file is part of GnuTLS.
 *
 * The GnuTLS is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 */

#include <libtasn1.h>
#include <gnutls/abstract.h>

int _gnutls_x509_cert_verify_peers(gnutls_session_t session,
				   gnutls_typed_vdata_st * data,
				   unsigned int elements,
				   unsigned int *status);

#define PEM_CERT_SEP2 "-----BEGIN X509 CERTIFICATE"
#define PEM_CERT_SEP "-----BEGIN CERTIFICATE"

#define PEM_CRL_SEP "-----BEGIN X509 CRL"

int _gnutls_x509_raw_privkey_to_gkey(gnutls_privkey_t * privkey,
				     const gnutls_datum_t * raw_key,
				     gnutls_x509_crt_fmt_t type);
