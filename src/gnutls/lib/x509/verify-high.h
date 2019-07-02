/*
 * Copyright (C) 2011-2012 Free Software Foundation, Inc.
 *
 * Author: Simon Josefsson
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>
 *
 */

#ifndef GNUTLS_LIB_X509_VERIFY_HIGH_H
#define GNUTLS_LIB_X509_VERIFY_HIGH_H

struct gnutls_x509_trust_list_st {
	unsigned int size;
	struct node_st *node;

	/* holds a sequence of the RDNs of the CAs above.
	 * This is used when using the trust list in TLS.
	 */
	gnutls_datum_t x509_rdn_sequence;

	gnutls_x509_crt_t *blacklisted;
	unsigned int blacklisted_size;

	/* certificates that will be deallocated when this struct
	 * will be deinitialized */
	gnutls_x509_crt_t *keep_certs;
	unsigned int keep_certs_size;
	
	char* pkcs11_token;
};

int _gnutls_trustlist_inlist(gnutls_x509_trust_list_t list,
			     gnutls_x509_crt_t cert);

#endif /* GNUTLS_LIB_X509_VERIFY_HIGH_H */
