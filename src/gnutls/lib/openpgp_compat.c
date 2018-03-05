/*
 * Copyright (C) 2017 Nikos Mavrogiannopoulos
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

/* This file contains the definitions of OpenPGP stub functions
 * for ABI compatibility.
 */

#include "gnutls_int.h"
#include <gnutls/openpgp.h>
#include <gnutls/abstract.h>

#ifndef ENABLE_OPENPGP

int gnutls_openpgp_crt_init(gnutls_openpgp_crt_t * key)
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}

void gnutls_openpgp_crt_deinit(gnutls_openpgp_crt_t key)
{
	return;
}

int gnutls_openpgp_crt_import(gnutls_openpgp_crt_t key,
			      const gnutls_datum_t * data,
			      gnutls_openpgp_crt_fmt_t format)
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}

int gnutls_openpgp_crt_export(gnutls_openpgp_crt_t key,
			      gnutls_openpgp_crt_fmt_t format,
			      void *output_data,
			      size_t * output_data_size) 
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}

int gnutls_openpgp_crt_export2(gnutls_openpgp_crt_t key,
			       gnutls_openpgp_crt_fmt_t format,
			       gnutls_datum_t * out) 
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}


int gnutls_openpgp_crt_print(gnutls_openpgp_crt_t cert,
			     gnutls_certificate_print_formats_t
			     format, gnutls_datum_t * out) 
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}


int gnutls_openpgp_crt_get_key_usage(gnutls_openpgp_crt_t key,
				     unsigned int *key_usage) 
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}

int gnutls_openpgp_crt_get_fingerprint(gnutls_openpgp_crt_t key,
				       void *fpr, size_t * fprlen) 
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}

int gnutls_openpgp_crt_get_subkey_fingerprint(gnutls_openpgp_crt_t
					      key,
					      unsigned int idx,
					      void *fpr, size_t * fprlen) 
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}


int gnutls_openpgp_crt_get_name(gnutls_openpgp_crt_t key,
				int idx, char *buf, size_t * sizeof_buf) 
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}


gnutls_pk_algorithm_t
gnutls_openpgp_crt_get_pk_algorithm(gnutls_openpgp_crt_t key,
				    unsigned int *bits) 
{
	return GNUTLS_PK_UNKNOWN;
}


int gnutls_openpgp_crt_get_version(gnutls_openpgp_crt_t key) 
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}


time_t gnutls_openpgp_crt_get_creation_time(gnutls_openpgp_crt_t key) 
{
	return (time_t)-1;
}

time_t gnutls_openpgp_crt_get_expiration_time(gnutls_openpgp_crt_t key) 
{
	return (time_t)-1;
}


int gnutls_openpgp_crt_get_key_id(gnutls_openpgp_crt_t key,
				  gnutls_openpgp_keyid_t keyid) 
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}


int gnutls_openpgp_crt_check_hostname(gnutls_openpgp_crt_t key,
				      const char *hostname) 
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}

int gnutls_openpgp_crt_check_hostname2(gnutls_openpgp_crt_t key,
				      const char *hostname, unsigned int flags) 
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}

int
gnutls_openpgp_crt_check_email(gnutls_openpgp_crt_t key, const char *email, unsigned flags) 
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}


int gnutls_openpgp_crt_get_revoked_status(gnutls_openpgp_crt_t key) 
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}


int gnutls_openpgp_crt_get_subkey_count(gnutls_openpgp_crt_t key) 
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}

int gnutls_openpgp_crt_get_subkey_idx(gnutls_openpgp_crt_t key,
				      const gnutls_openpgp_keyid_t keyid) 
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}

int gnutls_openpgp_crt_get_subkey_revoked_status
    (gnutls_openpgp_crt_t key, unsigned int idx) 
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}

gnutls_pk_algorithm_t
gnutls_openpgp_crt_get_subkey_pk_algorithm(gnutls_openpgp_crt_t
					   key,
					   unsigned int idx,
					   unsigned int *bits) 
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}

time_t
    gnutls_openpgp_crt_get_subkey_creation_time
    (gnutls_openpgp_crt_t key, unsigned int idx) 
{
	return (time_t)-1;
}

time_t
    gnutls_openpgp_crt_get_subkey_expiration_time
    (gnutls_openpgp_crt_t key, unsigned int idx) 
{
	return (time_t)-1;
}

int gnutls_openpgp_crt_get_subkey_id(gnutls_openpgp_crt_t key,
				     unsigned int idx,
				     gnutls_openpgp_keyid_t keyid) 
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}

int gnutls_openpgp_crt_get_subkey_usage(gnutls_openpgp_crt_t key,
					unsigned int idx,
					unsigned int *key_usage) 
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}


int gnutls_openpgp_crt_get_subkey_pk_dsa_raw(gnutls_openpgp_crt_t
					     crt, unsigned int idx,
					     gnutls_datum_t * p,
					     gnutls_datum_t * q,
					     gnutls_datum_t * g,
					     gnutls_datum_t * y) 
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}

int gnutls_openpgp_crt_get_subkey_pk_rsa_raw(gnutls_openpgp_crt_t
					     crt, unsigned int idx,
					     gnutls_datum_t * m,
					     gnutls_datum_t * e) 
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}

int gnutls_openpgp_crt_get_pk_dsa_raw(gnutls_openpgp_crt_t crt,
				      gnutls_datum_t * p,
				      gnutls_datum_t * q,
				      gnutls_datum_t * g,
				      gnutls_datum_t * y) 
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}

int gnutls_openpgp_crt_get_pk_rsa_raw(gnutls_openpgp_crt_t crt,
				      gnutls_datum_t * m,
				      gnutls_datum_t * e) 
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}


int gnutls_openpgp_crt_get_preferred_key_id(gnutls_openpgp_crt_t
					    key,
					    gnutls_openpgp_keyid_t keyid) 
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}

int
gnutls_openpgp_crt_set_preferred_key_id(gnutls_openpgp_crt_t key,
					const
					gnutls_openpgp_keyid_t keyid) 
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}


/* privkey stuff.
 */
int gnutls_openpgp_privkey_init(gnutls_openpgp_privkey_t * key) 
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}

void gnutls_openpgp_privkey_deinit(gnutls_openpgp_privkey_t key)
{
	return;
}

gnutls_pk_algorithm_t
    gnutls_openpgp_privkey_get_pk_algorithm
    (gnutls_openpgp_privkey_t key, unsigned int *bits)
{
	return GNUTLS_PK_UNKNOWN;
}

gnutls_sec_param_t
gnutls_openpgp_privkey_sec_param(gnutls_openpgp_privkey_t key) 
{
	return 0;
}

int gnutls_openpgp_privkey_import(gnutls_openpgp_privkey_t key,
				  const gnutls_datum_t * data,
				  gnutls_openpgp_crt_fmt_t format,
				  const char *password,
				  unsigned int flags) 
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}


int gnutls_openpgp_privkey_get_fingerprint(gnutls_openpgp_privkey_t
					   key, void *fpr,
					   size_t * fprlen) 
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}

int gnutls_openpgp_privkey_get_subkey_fingerprint
    (gnutls_openpgp_privkey_t key, unsigned int idx, void *fpr,
     size_t * fprlen) 
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}

int gnutls_openpgp_privkey_get_key_id(gnutls_openpgp_privkey_t key,
				      gnutls_openpgp_keyid_t keyid) 
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}

int gnutls_openpgp_privkey_get_subkey_count(gnutls_openpgp_privkey_t key) 
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}

int gnutls_openpgp_privkey_get_subkey_idx(gnutls_openpgp_privkey_t
					  key,
					  const
					  gnutls_openpgp_keyid_t keyid) 
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}


int gnutls_openpgp_privkey_get_subkey_revoked_status
    (gnutls_openpgp_privkey_t key, unsigned int idx) 
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}


int gnutls_openpgp_privkey_get_revoked_status
    (gnutls_openpgp_privkey_t key) 
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}


gnutls_pk_algorithm_t
    gnutls_openpgp_privkey_get_subkey_pk_algorithm
    (gnutls_openpgp_privkey_t key, unsigned int idx, unsigned int *bits) 
{
	return GNUTLS_PK_UNKNOWN;
}


time_t
    gnutls_openpgp_privkey_get_subkey_expiration_time
    (gnutls_openpgp_privkey_t key, unsigned int idx)
{
	return (time_t)-1;
}

int gnutls_openpgp_privkey_get_subkey_id(gnutls_openpgp_privkey_t
					 key, unsigned int idx,
					 gnutls_openpgp_keyid_t keyid) 
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}


time_t
    gnutls_openpgp_privkey_get_subkey_creation_time
    (gnutls_openpgp_privkey_t key, unsigned int idx) 
{
	return (time_t)-1;
}


int gnutls_openpgp_privkey_export_subkey_dsa_raw
    (gnutls_openpgp_privkey_t pkey, unsigned int idx,
     gnutls_datum_t * p, gnutls_datum_t * q, gnutls_datum_t * g,
     gnutls_datum_t * y, gnutls_datum_t * x) 
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}

int gnutls_openpgp_privkey_export_subkey_rsa_raw
    (gnutls_openpgp_privkey_t pkey, unsigned int idx,
     gnutls_datum_t * m, gnutls_datum_t * e, gnutls_datum_t * d,
     gnutls_datum_t * p, gnutls_datum_t * q, gnutls_datum_t * u) 
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}


int gnutls_openpgp_privkey_export_dsa_raw(gnutls_openpgp_privkey_t
					  pkey, gnutls_datum_t * p,
					  gnutls_datum_t * q,
					  gnutls_datum_t * g,
					  gnutls_datum_t * y,
					  gnutls_datum_t * x) 
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}

int gnutls_openpgp_privkey_export_rsa_raw(gnutls_openpgp_privkey_t
					  pkey, gnutls_datum_t * m,
					  gnutls_datum_t * e,
					  gnutls_datum_t * d,
					  gnutls_datum_t * p,
					  gnutls_datum_t * q,
					  gnutls_datum_t * u) 
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}


int gnutls_openpgp_privkey_export(gnutls_openpgp_privkey_t key,
				  gnutls_openpgp_crt_fmt_t format,
				  const char *password,
				  unsigned int flags,
				  void *output_data,
				  size_t * output_data_size) 
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}

int gnutls_openpgp_privkey_export2(gnutls_openpgp_privkey_t key,
				   gnutls_openpgp_crt_fmt_t format,
				   const char *password,
				   unsigned int flags,
				   gnutls_datum_t * out) 
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}


int gnutls_openpgp_privkey_set_preferred_key_id
    (gnutls_openpgp_privkey_t key, const gnutls_openpgp_keyid_t keyid) 
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}

int gnutls_openpgp_privkey_get_preferred_key_id
    (gnutls_openpgp_privkey_t key, gnutls_openpgp_keyid_t keyid) 
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}


int gnutls_openpgp_crt_get_auth_subkey(gnutls_openpgp_crt_t crt,
				       gnutls_openpgp_keyid_t
				       keyid, unsigned int flag) 
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}


/* Keyring stuff.
 */

int gnutls_openpgp_keyring_init(gnutls_openpgp_keyring_t * keyring) 
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}

void gnutls_openpgp_keyring_deinit(gnutls_openpgp_keyring_t keyring)
{
	return;
}

int gnutls_openpgp_keyring_import(gnutls_openpgp_keyring_t keyring,
				  const gnutls_datum_t * data,
				  gnutls_openpgp_crt_fmt_t format) 
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}


int gnutls_openpgp_keyring_check_id(gnutls_openpgp_keyring_t ring,
				    const gnutls_openpgp_keyid_t
				    keyid, unsigned int flags) 
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}



int gnutls_openpgp_crt_verify_ring(gnutls_openpgp_crt_t key,
				   gnutls_openpgp_keyring_t
				   keyring, unsigned int flags,
				   unsigned int *verify
				   /* the output of the verification */
    ) 
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}


int gnutls_openpgp_crt_verify_self(gnutls_openpgp_crt_t key,
				   unsigned int flags,
				   unsigned int *verify) 
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}


int gnutls_openpgp_keyring_get_crt(gnutls_openpgp_keyring_t ring,
				   unsigned int idx,
				   gnutls_openpgp_crt_t * cert) 
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}


int gnutls_openpgp_keyring_get_crt_count(gnutls_openpgp_keyring_t ring) 
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}




void
gnutls_openpgp_set_recv_key_function(gnutls_session_t session,
				     gnutls_openpgp_recv_key_func func)
{
	return;
}

int gnutls_certificate_set_openpgp_key
    (gnutls_certificate_credentials_t res,
     gnutls_openpgp_crt_t crt, gnutls_openpgp_privkey_t pkey) 
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}


int
gnutls_certificate_get_openpgp_key(gnutls_certificate_credentials_t res,
                                   unsigned index,
                                   gnutls_openpgp_privkey_t *key) 
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}

int
gnutls_certificate_get_openpgp_crt(gnutls_certificate_credentials_t res,
                                   unsigned index,
                                   gnutls_openpgp_crt_t **crt_list,
                                   unsigned *crt_list_size) 
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}


int
 gnutls_certificate_set_openpgp_key_file
    (gnutls_certificate_credentials_t res, const char *certfile,
     const char *keyfile, gnutls_openpgp_crt_fmt_t format) 
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}

int gnutls_certificate_set_openpgp_key_mem
    (gnutls_certificate_credentials_t res,
     const gnutls_datum_t * cert, const gnutls_datum_t * key,
     gnutls_openpgp_crt_fmt_t format) 
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}


int
 gnutls_certificate_set_openpgp_key_file2
    (gnutls_certificate_credentials_t res, const char *certfile,
     const char *keyfile, const char *subkey_id,
     gnutls_openpgp_crt_fmt_t format) 
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}

int
 gnutls_certificate_set_openpgp_key_mem2
    (gnutls_certificate_credentials_t res,
     const gnutls_datum_t * cert, const gnutls_datum_t * key,
     const char *subkey_id, gnutls_openpgp_crt_fmt_t format) 
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}


int gnutls_certificate_set_openpgp_keyring_mem
    (gnutls_certificate_credentials_t c, const unsigned char *data,
     size_t dlen, gnutls_openpgp_crt_fmt_t format) 
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}


int gnutls_certificate_set_openpgp_keyring_file
    (gnutls_certificate_credentials_t c, const char *file,
     gnutls_openpgp_crt_fmt_t format) 
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}

int gnutls_pubkey_import_openpgp(gnutls_pubkey_t key,
				 gnutls_openpgp_crt_t crt,
				 unsigned int flags)
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}

int gnutls_pubkey_import_openpgp_raw(gnutls_pubkey_t pkey,
				     const gnutls_datum_t * data,
				     gnutls_openpgp_crt_fmt_t
				     format,
				     const gnutls_openpgp_keyid_t
				     keyid, unsigned int flags)
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}

int
gnutls_pubkey_get_openpgp_key_id(gnutls_pubkey_t key,
				 unsigned int flags,
				 unsigned char *output_data,
				 size_t * output_data_size,
				 unsigned int *subkey)
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}

int gnutls_privkey_import_openpgp(gnutls_privkey_t pkey,
				  gnutls_openpgp_privkey_t key,
				  unsigned int flags)
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}

int gnutls_privkey_export_openpgp(gnutls_privkey_t pkey,
                                  gnutls_openpgp_privkey_t * key)
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}

int gnutls_privkey_import_openpgp_raw(gnutls_privkey_t pkey,
				      const gnutls_datum_t * data,
				      gnutls_openpgp_crt_fmt_t
				      format,
				      const gnutls_openpgp_keyid_t
				      keyid, const char *password)
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}

int gnutls_pcert_import_openpgp_raw(gnutls_pcert_st * pcert,
				    const gnutls_datum_t * cert,
				    gnutls_openpgp_crt_fmt_t
				    format,
				    gnutls_openpgp_keyid_t keyid,
				    unsigned int flags)
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}

int gnutls_pcert_import_openpgp(gnutls_pcert_st * pcert,
				gnutls_openpgp_crt_t crt,
				unsigned int flags)
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}

int gnutls_pcert_export_openpgp(gnutls_pcert_st * pcert,
                                gnutls_openpgp_crt_t * crt)
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}

void
gnutls_openpgp_send_cert(gnutls_session_t session,
			 gnutls_openpgp_crt_status_t status)
{
	return;
}

int gnutls_certificate_get_peers_subkey_id(gnutls_session_t session,
					   gnutls_datum_t * id)
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}

int
gnutls_openpgp_privkey_sign_hash(gnutls_openpgp_privkey_t key,
				 const gnutls_datum_t * hash,
				 gnutls_datum_t * signature)
{
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
}

#endif
