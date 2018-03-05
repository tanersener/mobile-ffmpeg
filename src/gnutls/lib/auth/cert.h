/*
 * Copyright (C) 2002-2012 Free Software Foundation, Inc.
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

#ifndef AUTH_CERT_H
#define AUTH_CERT_H
#include "auth.h"
#include <auth/dh_common.h>
#include <x509/x509_int.h>
#include <openpgp/openpgp_int.h>
#include <gnutls/abstract.h>
#include <gnutls/compat.h>
#include <str_array.h>

typedef struct {
	gnutls_pcert_st *cert_list;	/* a certificate chain */
	unsigned int cert_list_length;	/* its length */
	gnutls_str_array_t names;	/* the names in the first certificate */

	gnutls_status_request_ocsp_func ocsp_func;
	void *ocsp_func_ptr; /* corresponding OCSP response function + ptr */
	char *ocsp_response_file; /* corresponding OCSP response file */
} certs_st;

/* This structure may be complex, but it's the only way to
 * support a server that has multiple certificates
 */
typedef struct gnutls_certificate_credentials_st {
	gnutls_dh_params_t dh_params;
	unsigned deinit_dh_params; /* if the internal values are set */

	/* this callback is used to retrieve the DH or RSA
	 * parameters.
	 */
	gnutls_params_function *params_func;

	certs_st *certs;
	unsigned ncerts;	/* the number of certs */

	gnutls_privkey_t *pkey;
	/* private keys. It contains ncerts private
	 * keys. pkey[i] corresponds to certificate in
	 * cert_list[i][0].
	 */

#ifdef ENABLE_OPENPGP
	/* OpenPGP specific stuff */
	gnutls_openpgp_keyring_t keyring;
#endif

	/* X509 specific stuff */
	gnutls_x509_trust_list_t tlist;
	unsigned flags; /* gnutls_certificate_flags */
	unsigned int verify_flags;	/* flags to be used at 
					 * certificate verification.
					 */
	unsigned int verify_depth;
	unsigned int verify_bits;

	/* It's a mess here. However we need to keep the old 3 functions
	 * for compatibility */
	gnutls_certificate_retrieve_function *get_cert_callback;	/* deprecated */
	gnutls_certificate_retrieve_function2 *get_cert_callback2;

	gnutls_certificate_verify_function *verify_callback;

	struct pin_info_st pin;
	/* temporarily hold the PIN if set_key_file2() is used with a PIN */
	char pin_tmp[GNUTLS_PKCS11_MAX_PIN_LEN];

	/* OCSP */
	gnutls_status_request_ocsp_func glob_ocsp_func;
	void *glob_ocsp_func_ptr; /* corresponding OCSP response function */
} certificate_credentials_st;

typedef struct rsa_info_st {
	gnutls_datum_t modulus;
	gnutls_datum_t exponent;
} rsa_info_st;

/* This is the information we keep for the peer
 * certificate.
 */
typedef struct cert_auth_info_st {
	/* These (dh/rsa) are just copies from the credentials_t structure.
	 * They must be freed.
	 */
	dh_info_st dh;

	gnutls_datum_t *raw_certificate_list;	/* holds the raw certificate of the
						 * peer.
						 */
	unsigned int ncerts;	/* holds the size of the list above */

	gnutls_certificate_type_t cert_type;
#ifdef ENABLE_OPENPGP
	uint8_t subkey_id[GNUTLS_OPENPGP_KEYID_SIZE];
#endif
} *cert_auth_info_t;

typedef struct cert_auth_info_st cert_auth_info_st;

void _gnutls_free_rsa_info(rsa_info_st * rsa);

/* AUTH X509 functions */
int _gnutls_gen_cert_server_crt(gnutls_session_t, gnutls_buffer_st *);
int _gnutls_gen_cert_client_crt(gnutls_session_t, gnutls_buffer_st *);
int _gnutls_gen_cert_client_crt_vrfy(gnutls_session_t, gnutls_buffer_st *);
int _gnutls_gen_cert_server_cert_req(gnutls_session_t, gnutls_buffer_st *);
int _gnutls_proc_cert_cert_req(gnutls_session_t, uint8_t *, size_t);
int _gnutls_proc_cert_client_crt_vrfy(gnutls_session_t, uint8_t *, size_t);
int _gnutls_proc_crt(gnutls_session_t, uint8_t *, size_t);
int _gnutls_get_selected_cert(gnutls_session_t session,
			      gnutls_pcert_st ** apr_cert_list,
			      int *apr_cert_list_length,
			      gnutls_privkey_t * apr_pkey);

int _gnutls_server_select_cert(struct gnutls_session_int *,
			       gnutls_pk_algorithm_t *, size_t);
void _gnutls_selected_certs_deinit(gnutls_session_t session);

int _gnutls_get_auth_info_pcert(gnutls_pcert_st * gcert,
				gnutls_certificate_type_t type,
				cert_auth_info_t info);

int certificate_credential_append_crt_list(gnutls_certificate_credentials_t
					   res, gnutls_str_array_t names,
					   gnutls_pcert_st * crt, int nr);
int certificate_credentials_append_pkey(gnutls_certificate_credentials_t
					res, gnutls_privkey_t pkey);

int _gnutls_selected_cert_supported_kx(struct gnutls_session_int *session,
				       gnutls_kx_algorithm_t * alg,
				       int *alg_size);

int _gnutls_check_key_cert_match(gnutls_certificate_credentials_t res);

int _gnutls_gen_dhe_signature(gnutls_session_t session,
			      gnutls_buffer_st * data, uint8_t * plain,
			      unsigned plain_size);
int _gnutls_proc_dhe_signature(gnutls_session_t session, uint8_t * data,
			       size_t _data_size,
			       gnutls_datum_t * vparams);

#endif
