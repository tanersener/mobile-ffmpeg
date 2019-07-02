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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>
 *
 */

#ifndef GNUTLS_LIB_PK_H
#define GNUTLS_LIB_PK_H

extern int crypto_pk_prio;
extern gnutls_crypto_pk_st _gnutls_pk_ops;

#define _gnutls_pk_encrypt( algo, ciphertext, plaintext, params) _gnutls_pk_ops.encrypt( algo, ciphertext, plaintext, params)
#define _gnutls_pk_decrypt( algo, ciphertext, plaintext, params) _gnutls_pk_ops.decrypt( algo, ciphertext, plaintext, params)
#define _gnutls_pk_decrypt2( algo, ciphertext, plaintext, size, params) _gnutls_pk_ops.decrypt2( algo, ciphertext, plaintext, size, params)
#define _gnutls_pk_sign( algo, sig, data, params, sign_params) _gnutls_pk_ops.sign( algo, sig, data, params, sign_params)
#define _gnutls_pk_verify( algo, data, sig, params, sign_params) _gnutls_pk_ops.verify( algo, data, sig, params, sign_params)
#define _gnutls_pk_verify_priv_params( algo, params) _gnutls_pk_ops.verify_priv_params( algo, params)
#define _gnutls_pk_verify_pub_params( algo, params) _gnutls_pk_ops.verify_pub_params( algo, params)
#define _gnutls_pk_derive( algo, out, pub, priv) _gnutls_pk_ops.derive( algo, out, pub, priv, 0)
#define _gnutls_pk_derive_tls13( algo, out, pub, priv) _gnutls_pk_ops.derive( algo, out, pub, priv, PK_DERIVE_TLS13)
#define _gnutls_pk_generate_keys( algo, bits, params, temporal) _gnutls_pk_ops.generate_keys( algo, bits, params, temporal)
#define _gnutls_pk_generate_params( algo, bits, priv) _gnutls_pk_ops.generate_params( algo, bits, priv)
#define _gnutls_pk_hash_algorithm( pk, sig, params, hash) _gnutls_pk_ops.hash_algorithm(pk, sig, params, hash)
#define _gnutls_pk_curve_exists( curve) _gnutls_pk_ops.curve_exists(curve)

inline static int
_gnutls_pk_fixup(gnutls_pk_algorithm_t algo, gnutls_direction_t direction,
		 gnutls_pk_params_st * params)
{
	if (_gnutls_pk_ops.pk_fixup_private_params)
		return _gnutls_pk_ops.pk_fixup_private_params(algo,
							      direction,
							      params);
	return 0;
}

int _gnutls_pk_params_copy(gnutls_pk_params_st * dst,
			   const gnutls_pk_params_st * src);

/* The internal PK interface */
int
_gnutls_encode_ber_rs(gnutls_datum_t * sig_value, bigint_t r, bigint_t s);
int
_gnutls_encode_ber_rs_raw(gnutls_datum_t * sig_value,
			  const gnutls_datum_t * r,
			  const gnutls_datum_t * s);

int
_gnutls_decode_ber_rs(const gnutls_datum_t * sig_value, bigint_t * r,
		      bigint_t * s);

int
_gnutls_decode_ber_rs_raw(const gnutls_datum_t * sig_value, gnutls_datum_t *r,
			  gnutls_datum_t *s);

int
_gnutls_encode_gost_rs(gnutls_datum_t * sig_value, bigint_t r, bigint_t s,
		       size_t intsize);

int
_gnutls_decode_gost_rs(const gnutls_datum_t * sig_value, bigint_t * r,
		       bigint_t * s);

gnutls_digest_algorithm_t _gnutls_gost_digest(gnutls_pk_algorithm_t pk);
gnutls_pk_algorithm_t _gnutls_digest_gost(gnutls_digest_algorithm_t digest);
gnutls_gost_paramset_t _gnutls_gost_paramset_default(gnutls_pk_algorithm_t pk);

int
encode_ber_digest_info(const mac_entry_st * e,
		       const gnutls_datum_t * digest,
		       gnutls_datum_t * output);

#define decode_ber_digest_info gnutls_decode_ber_digest_info

int
_gnutls_params_get_rsa_raw(const gnutls_pk_params_st* params,
				    gnutls_datum_t * m, gnutls_datum_t * e,
				    gnutls_datum_t * d, gnutls_datum_t * p,
				    gnutls_datum_t * q, gnutls_datum_t * u,
				    gnutls_datum_t * e1,
				    gnutls_datum_t * e2,
				    unsigned int flags);

int
_gnutls_params_get_dsa_raw(const gnutls_pk_params_st* params,
			     gnutls_datum_t * p, gnutls_datum_t * q,
			     gnutls_datum_t * g, gnutls_datum_t * y,
			     gnutls_datum_t * x, unsigned int flags);

int _gnutls_params_get_ecc_raw(const gnutls_pk_params_st* params,
				       gnutls_ecc_curve_t * curve,
				       gnutls_datum_t * x,
				       gnutls_datum_t * y,
				       gnutls_datum_t * k,
				       unsigned int flags);

int _gnutls_params_get_gost_raw(const gnutls_pk_params_st* params,
				       gnutls_ecc_curve_t * curve,
				       gnutls_digest_algorithm_t * digest,
				       gnutls_gost_paramset_t * paramset,
				       gnutls_datum_t * x,
				       gnutls_datum_t * y,
				       gnutls_datum_t * k,
				       unsigned int flags);

int pk_prepare_hash(gnutls_pk_algorithm_t pk, const mac_entry_st * hash,
		    gnutls_datum_t * output);
int pk_hash_data(gnutls_pk_algorithm_t pk, const mac_entry_st * hash,
		 gnutls_pk_params_st * params, const gnutls_datum_t * data,
		 gnutls_datum_t * digest);

int _gnutls_find_rsa_pss_salt_size(unsigned bits, const mac_entry_st *me,
				   unsigned salt_size);

#endif /* GNUTLS_LIB_PK_H */
