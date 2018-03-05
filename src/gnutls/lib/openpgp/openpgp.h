/*
 * Copyright (C) 2002-2012 Free Software Foundation, Inc.
 *
 * Author: Timo Schulz, Nikos Mavrogiannopoulos
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

#include <config.h>

#ifdef ENABLE_OPENPGP

#ifndef GNUTLS_OPENPGP_LOCAL_H
#define GNUTLS_OPENPGP_LOCAL_H

#include <auth/cert.h>
#include <opencdk.h>
#include <gnutls/abstract.h>

/* OpenCDK compatible */
typedef enum {
	KEY_ATTR_NONE = 0,
	KEY_ATTR_SHORT_KEYID = 3,
	KEY_ATTR_KEYID = 4,
	KEY_ATTR_FPR = 5
} key_attr_t;

int gnutls_openpgp_count_key_names(const gnutls_datum_t * cert);

int gnutls_openpgp_get_key(gnutls_datum_t * key,
			   gnutls_openpgp_keyring_t keyring,
			   key_attr_t by, uint8_t * pattern);

/* internal */
int
_gnutls_openpgp_privkey_cpy(gnutls_openpgp_privkey_t dest,
			    gnutls_openpgp_privkey_t src);

int
_gnutls_openpgp_request_key(gnutls_session_t,
			    gnutls_datum_t * ret,
			    const gnutls_certificate_credentials_t cred,
			    uint8_t * key_fpr, int key_fpr_size);

int
_gnutls_openpgp_verify_key(const gnutls_certificate_credentials_t cred,
			   gnutls_x509_subject_alt_name_t type,
			   const char *hostname,
			   const gnutls_datum_t * cert_list,
			   int cert_list_length,
			   unsigned int verify_flags,
			   unsigned int *status);
int _gnutls_openpgp_fingerprint(const gnutls_datum_t * cert,
				unsigned char *fpr, size_t * fprlen);
time_t _gnutls_openpgp_get_raw_key_creation_time(const gnutls_datum_t *
						 cert);
time_t _gnutls_openpgp_get_raw_key_expiration_time(const gnutls_datum_t *
						   cert);

int
_gnutls_openpgp_privkey_sign_hash(gnutls_openpgp_privkey_t key,
				  const gnutls_datum_t * hash,
				  gnutls_datum_t * signature);


int
_gnutls_openpgp_privkey_decrypt_data(gnutls_openpgp_privkey_t key,
				     unsigned int flags,
				     const gnutls_datum_t * ciphertext,
				     gnutls_datum_t * plaintext);

#endif				/*GNUTLS_OPENPGP_LOCAL_H */

#endif				/*ENABLE_OPENPGP */
