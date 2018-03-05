/*
 * Copyright (C) 2010-2012 Free Software Foundation, Inc.
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

#ifndef _ABSTRACT_INT_H
#define _ABSTRACT_INT_H

#include <gnutls/abstract.h>

struct gnutls_privkey_st {
	gnutls_privkey_type_t type;
	gnutls_pk_algorithm_t pk_algorithm;

	union {
		gnutls_x509_privkey_t x509;
#ifdef ENABLE_PKCS11
		gnutls_pkcs11_privkey_t pkcs11;
#endif
#ifdef ENABLE_OPENPGP
		gnutls_openpgp_privkey_t openpgp;
#endif
		struct {
			gnutls_privkey_sign_func sign_func;
			gnutls_privkey_decrypt_func decrypt_func;
			gnutls_privkey_deinit_func deinit_func;
			gnutls_privkey_info_func info_func;
			void *userdata;
		} ext;
	} key;

	unsigned int flags;
	gnutls_sign_algorithm_t preferred_sign_algo;
	struct pin_info_st pin;
};

struct gnutls_pubkey_st {
	gnutls_pk_algorithm_t pk_algorithm;
	unsigned int bits;	/* an indication of the security parameter */

	/* the size of params depends on the public
	 * key algorithm
	 * RSA: [0] is modulus
	 *      [1] is public exponent
	 * DSA: [0] is p
	 *      [1] is q
	 *      [2] is g
	 *      [3] is public key
	 */
	gnutls_pk_params_st params;

#ifdef ENABLE_OPENPGP
	uint8_t openpgp_key_id[GNUTLS_OPENPGP_KEYID_SIZE];
	unsigned int openpgp_key_id_set;

	uint8_t openpgp_key_fpr[GNUTLS_OPENPGP_V4_FINGERPRINT_SIZE];
	unsigned int openpgp_key_fpr_set:1;
#endif

	unsigned int key_usage;	/* bits from GNUTLS_KEY_* */

	struct pin_info_st pin;
};

int _gnutls_privkey_get_public_mpis(gnutls_privkey_t key,
				    gnutls_pk_params_st *);

void _gnutls_privkey_cleanup(gnutls_privkey_t key);

unsigned pubkey_to_bits(gnutls_pk_algorithm_t pk, gnutls_pk_params_st * params);
int _gnutls_pubkey_compatible_with_sig(gnutls_session_t,
				       gnutls_pubkey_t pubkey,
				       const version_entry_st * ver,
				       gnutls_sign_algorithm_t sign);
int
_gnutls_pubkey_get_mpis(gnutls_pubkey_t key, gnutls_pk_params_st * params);

int
pubkey_verify_hashed_data(gnutls_pk_algorithm_t pk,
			  const mac_entry_st * algo,
			  const gnutls_datum_t * hash,
			  const gnutls_datum_t * signature,
			  gnutls_pk_params_st * issuer_params);

int pubkey_verify_data(gnutls_pk_algorithm_t pk,
		       const mac_entry_st * algo,
		       const gnutls_datum_t * data,
		       const gnutls_datum_t * signature,
		       gnutls_pk_params_st * issuer_params);



const mac_entry_st *_gnutls_dsa_q_to_hash(gnutls_pk_algorithm_t algo,
					  const gnutls_pk_params_st *
					  params, unsigned int *hash_len);

int
_gnutls_privkey_get_mpis(gnutls_privkey_t key, gnutls_pk_params_st * params);

gnutls_sign_algorithm_t
_gnutls_privkey_get_preferred_sign_algo(gnutls_privkey_t key);

#endif
