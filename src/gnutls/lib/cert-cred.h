/*
 * Copyright (C) 2018 Free Software Foundation, Inc.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>
 *
 */

#ifndef GNUTLS_LIB_CERT_CRED_H
#define GNUTLS_LIB_CERT_CRED_H

#include <gnutls/abstract.h>
#include "str_array.h"


int
_gnutls_certificate_credential_append_keypair(gnutls_certificate_credentials_t res,
				       gnutls_privkey_t key,
				       gnutls_str_array_t names,
				       gnutls_pcert_st * crt, int nr);

int
_gnutls_read_key_mem(gnutls_certificate_credentials_t res,
	     const void *key, int key_size, gnutls_x509_crt_fmt_t type,
	     const char *pass, unsigned int flags,
	     gnutls_privkey_t *rkey);

int
_gnutls_read_key_file(gnutls_certificate_credentials_t res,
	      const char *keyfile, gnutls_x509_crt_fmt_t type,
	      const char *pass, unsigned int flags,
	      gnutls_privkey_t *rkey);

int
_gnutls_get_x509_name(gnutls_x509_crt_t crt, gnutls_str_array_t * names);

#define CRED_RET_SUCCESS(cred) \
	if (cred->flags & GNUTLS_CERTIFICATE_API_V2) { \
		return cred->ncerts-1; \
	} else { \
		return 0; \
	}

#endif /* GNUTLS_LIB_CERT_CRED_H */
