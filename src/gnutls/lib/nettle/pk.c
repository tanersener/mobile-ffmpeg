/*
 * Copyright (C) 2010-2012 Free Software Foundation, Inc.
 * Copyright (C) 2013-2015 Nikos Mavrogiannopoulos
 *
 * Author: Nikos Mavrogiannopoulos
 *
 * This file is part of GNUTLS.
 *
 * The GNUTLS library is free software; you can redistribute it and/or
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

/* This file contains the functions needed for RSA/DSA public key
 * encryption and signatures.
 */

#include "gnutls_int.h"
#include <mpi.h>
#include <pk.h>
#include "errors.h"
#include <datum.h>
#include <global.h>
#include <tls-sig.h>
#include <num.h>
#include <x509/x509_int.h>
#include <x509/common.h>
#include <random.h>
#include <pk.h>
#include <nettle/dsa.h>
#include <dsa-fips.h>
#include <rsa-fips.h>
#include <nettle/rsa.h>
#include <gnutls/crypto.h>
#include <nettle/bignum.h>
#include <nettle/ecc.h>
#include <nettle/ecdsa.h>
#include <nettle/ecc-curve.h>
#include <nettle/curve25519.h>
#include <nettle/version.h>
#include <gnettle.h>
#include <fips.h>

static inline const struct ecc_curve *get_supported_nist_curve(int curve);

static void rnd_func(void *_ctx, size_t length, uint8_t * data)
{
	if (gnutls_rnd(GNUTLS_RND_RANDOM, data, length) < 0) {
#ifdef ENABLE_FIPS140
		_gnutls_switch_lib_state(LIB_STATE_ERROR);
#else
		abort();
#endif
	}
}

static void
ecc_scalar_zclear (struct ecc_scalar *s)
{
	zeroize_key(s->p, ecc_size(s->ecc)*sizeof(mp_limb_t));
	ecc_scalar_clear(s);
}

static void
ecc_point_zclear (struct ecc_point *p)
{
	zeroize_key(p->p, ecc_size_a(p->ecc)*sizeof(mp_limb_t));
	ecc_point_clear(p);
}
 
static void
_dsa_params_get(const gnutls_pk_params_st * pk_params,
		struct dsa_params *pub)
{
	memcpy(pub->p, pk_params->params[DSA_P], SIZEOF_MPZT);

	if (pk_params->params[DSA_Q])
		memcpy(&pub->q, pk_params->params[DSA_Q], SIZEOF_MPZT);
	memcpy(pub->g, pk_params->params[DSA_G], SIZEOF_MPZT);
}

static void
_rsa_params_to_privkey(const gnutls_pk_params_st * pk_params,
		       struct rsa_private_key *priv)
{
	memcpy(priv->d, pk_params->params[2], SIZEOF_MPZT);
	memcpy(priv->p, pk_params->params[3], SIZEOF_MPZT);
	memcpy(priv->q, pk_params->params[4], SIZEOF_MPZT);
	memcpy(priv->c, pk_params->params[5], SIZEOF_MPZT);
	memcpy(priv->a, pk_params->params[6], SIZEOF_MPZT);
	memcpy(priv->b, pk_params->params[7], SIZEOF_MPZT);
	/* we do not rsa_private_key_prepare() because it involves a multiplication.
	 * we call it once when we import the parameters */
	priv->size =
	    nettle_mpz_sizeinbase_256_u(TOMPZ
					(pk_params->params[RSA_MODULUS]));
}

/* returns a negative value on invalid pubkey */
static int
_rsa_params_to_pubkey(const gnutls_pk_params_st * pk_params,
		      struct rsa_public_key *pub)
{
	memcpy(pub->n, pk_params->params[RSA_MODULUS], SIZEOF_MPZT);
	memcpy(pub->e, pk_params->params[RSA_PUB], SIZEOF_MPZT);
	if (rsa_public_key_prepare(pub) == 0)
		return gnutls_assert_val(GNUTLS_E_PK_INVALID_PUBKEY);

	return 0;
}

static int
_ecc_params_to_privkey(const gnutls_pk_params_st * pk_params,
		       struct ecc_scalar *priv,
		       const struct ecc_curve *curve)
{
	ecc_scalar_init(priv, curve);
	if (ecc_scalar_set(priv, pk_params->params[ECC_K]) == 0) {
		ecc_scalar_clear(priv);
		return gnutls_assert_val(GNUTLS_E_PK_INVALID_PRIVKEY);
	}

	return 0;
}

static int
_ecc_params_to_pubkey(const gnutls_pk_params_st * pk_params,
		      struct ecc_point *pub, const struct ecc_curve *curve)
{
	ecc_point_init(pub, curve);
	if (ecc_point_set
	    (pub, pk_params->params[ECC_X], pk_params->params[ECC_Y]) == 0) {
		ecc_point_clear(pub);
		return gnutls_assert_val(GNUTLS_E_PK_INVALID_PUBKEY);
	}

	return 0;
}

static void
ecc_shared_secret(struct ecc_scalar *private_key,
		  struct ecc_point *public_key, void *out, unsigned size)
{
	struct ecc_point r;
	mpz_t x;

	mpz_init(x);
	ecc_point_init(&r, public_key->ecc);

	ecc_point_mul(&r, private_key, public_key);

	ecc_point_get(&r, x, NULL);
	nettle_mpz_get_str_256(size, out, x);

	mpz_clear(x);
	ecc_point_clear(&r);

	return;
}

#define MAX_DH_BITS DEFAULT_MAX_VERIFY_BITS
/* This is used when we have no idea on the structure
 * of p-1 used by the peer. It is still a conservative
 * choice, but small than what we've been using before.
 */
#define DH_EXPONENT_SIZE(p_size) (2*_gnutls_pk_bits_to_subgroup_bits(p_size))

/* This is used for DH or ECDH key derivation. In DH for example
 * it is given the peers Y and our x, and calculates Y^x
 */
static int _wrap_nettle_pk_derive(gnutls_pk_algorithm_t algo,
				  gnutls_datum_t * out,
				  const gnutls_pk_params_st * priv,
				  const gnutls_pk_params_st * pub)
{
	int ret;

	switch (algo) {
	case GNUTLS_PK_DH: {
		bigint_t f, x, prime;
		bigint_t k = NULL, ff = NULL;
		unsigned int bits;

		f = pub->params[DH_Y];
		x = priv->params[DH_X];
		prime = priv->params[DH_P];

		ret = _gnutls_mpi_init_multi(&k, &ff, NULL);
		if (ret < 0)
			return gnutls_assert_val(ret);

		ret = _gnutls_mpi_add_ui(ff, f, 1);
		if (ret < 0) {
			gnutls_assert();
			goto dh_cleanup;
		}

		/* check if f==0,1, or f >= p-1.
		 * or (ff=f+1) equivalently ff==1,2, ff >= p */
		if ((_gnutls_mpi_cmp_ui(ff, 2) == 0)
		    || (_gnutls_mpi_cmp_ui(ff, 1) == 0)
		    || (_gnutls_mpi_cmp(ff, prime) >= 0)) {
			gnutls_assert();
			ret = GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER;
			goto dh_cleanup;
		}

		/* prevent denial of service */
		bits = _gnutls_mpi_get_nbits(prime);
		if (bits == 0 || bits > MAX_DH_BITS) {
			gnutls_assert();
			ret = GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER;
			goto dh_cleanup;
		}


		ret = _gnutls_mpi_powm(k, f, x, prime);
		if (ret < 0) {
			gnutls_assert();
			goto dh_cleanup;
		}

		ret = _gnutls_mpi_dprint(k, out);
		if (ret < 0) {
			gnutls_assert();
			goto dh_cleanup;
		}

		ret = 0;
dh_cleanup:
		_gnutls_mpi_release(&ff);
		zrelease_temp_mpi_key(&k);
		if (ret < 0)
			goto cleanup;

		break;
	}
	case GNUTLS_PK_EC:
		{
			struct ecc_scalar ecc_priv;
			struct ecc_point ecc_pub;
			const struct ecc_curve *curve;

			out->data = NULL;

			curve = get_supported_nist_curve(priv->flags);
			if (curve == NULL)
				return
				    gnutls_assert_val
				    (GNUTLS_E_ECC_UNSUPPORTED_CURVE);

			ret = _ecc_params_to_pubkey(pub, &ecc_pub, curve);
			if (ret < 0)
				return gnutls_assert_val(ret);

			ret =
			    _ecc_params_to_privkey(priv, &ecc_priv, curve);
			if (ret < 0) {
				ecc_point_clear(&ecc_pub);
				return gnutls_assert_val(ret);
			}

			out->size = gnutls_ecc_curve_get_size(priv->flags);
			/*ecc_size(curve)*sizeof(mp_limb_t); */
			out->data = gnutls_malloc(out->size);
			if (out->data == NULL) {
				ret =
				    gnutls_assert_val
				    (GNUTLS_E_MEMORY_ERROR);
				goto ecc_cleanup;
			}

			ecc_shared_secret(&ecc_priv, &ecc_pub, out->data,
					  out->size);

		      ecc_cleanup:
			ecc_point_clear(&ecc_pub);
			ecc_scalar_zclear(&ecc_priv);
			if (ret < 0)
				goto cleanup;
			break;
		}
	case GNUTLS_PK_ECDHX:
		{
			unsigned size = gnutls_ecc_curve_get_size(priv->flags);

			/* The point is in pub, while the private part (scalar) in priv. */

			if (size == 0 || priv->raw_priv.size != size)
				return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

			out->data = gnutls_malloc(size);
			if (out->data == NULL) {
				ret = gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);
				goto cleanup;
			}

			out->size = size;

			curve25519_mul(out->data, priv->raw_priv.data, pub->raw_pub.data);

			if (_gnutls_mem_is_zero(out->data, out->size)) {
				gnutls_free(out->data);
				out->data = NULL;
				gnutls_assert();
				ret = GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER;
				goto cleanup;
			}
			break;
		}
	default:
		gnutls_assert();
		ret = GNUTLS_E_INTERNAL_ERROR;
		goto cleanup;
	}

	ret = 0;

      cleanup:

	return ret;
}

static int
_wrap_nettle_pk_encrypt(gnutls_pk_algorithm_t algo,
			gnutls_datum_t * ciphertext,
			const gnutls_datum_t * plaintext,
			const gnutls_pk_params_st * pk_params)
{
	int ret;
	mpz_t p;

	mpz_init(p);

	switch (algo) {
	case GNUTLS_PK_RSA:
		{
			struct rsa_public_key pub;

			ret = _rsa_params_to_pubkey(pk_params, &pub);
			if (ret < 0) {
				gnutls_assert();
				goto cleanup;
			}

			ret =
			    rsa_encrypt(&pub, NULL, rnd_func,
					plaintext->size, plaintext->data,
					p);
			if (ret == 0) {
				ret =
				    gnutls_assert_val
				    (GNUTLS_E_ENCRYPTION_FAILED);
				goto cleanup;
			}

			ret =
			    _gnutls_mpi_dprint_size(p, ciphertext,
						    pub.size);
			if (ret < 0) {
				gnutls_assert();
				goto cleanup;
			}

			break;
		}
	default:
		gnutls_assert();
		ret = GNUTLS_E_INTERNAL_ERROR;
		goto cleanup;
	}

	ret = 0;

      cleanup:
	mpz_clear(p);

	FAIL_IF_LIB_ERROR;
	return ret;
}

static int
_wrap_nettle_pk_decrypt(gnutls_pk_algorithm_t algo,
			gnutls_datum_t * plaintext,
			const gnutls_datum_t * ciphertext,
			const gnutls_pk_params_st * pk_params)
{
	int ret;

	plaintext->data = NULL;

	/* make a sexp from pkey */
	switch (algo) {
	case GNUTLS_PK_RSA:
		{
			struct rsa_private_key priv;
			struct rsa_public_key pub;
			size_t length;
			bigint_t c;

			_rsa_params_to_privkey(pk_params, &priv);
			ret = _rsa_params_to_pubkey(pk_params, &pub);
			if (ret < 0)
				return
				    gnutls_assert_val(ret);

			if (ciphertext->size != pub.size)
				return
				    gnutls_assert_val
				    (GNUTLS_E_DECRYPTION_FAILED);

			if (_gnutls_mpi_init_scan_nz
			    (&c, ciphertext->data,
			     ciphertext->size) != 0) {
				ret =
				    gnutls_assert_val
				    (GNUTLS_E_MPI_SCAN_FAILED);
				goto cleanup;
			}

			length = pub.size;
			plaintext->data = gnutls_malloc(length);
			if (plaintext->data == NULL) {
				ret =
				    gnutls_assert_val
				    (GNUTLS_E_MEMORY_ERROR);
				goto cleanup;
			}

			ret =
			    rsa_decrypt_tr(&pub, &priv, NULL, rnd_func,
					   &length, plaintext->data,
					   TOMPZ(c));
			_gnutls_mpi_release(&c);
			plaintext->size = length;

			if (ret == 0) {
				ret =
				    gnutls_assert_val
				    (GNUTLS_E_DECRYPTION_FAILED);
				goto cleanup;
			}

			break;
		}
	default:
		gnutls_assert();
		ret = GNUTLS_E_INTERNAL_ERROR;
		goto cleanup;
	}

	ret = 0;

      cleanup:
	if (ret < 0)
		gnutls_free(plaintext->data);

	FAIL_IF_LIB_ERROR;
	return ret;
}

/* in case of DSA puts into data, r,s
 */
static int
_wrap_nettle_pk_sign(gnutls_pk_algorithm_t algo,
		     gnutls_datum_t * signature,
		     const gnutls_datum_t * vdata,
		     const gnutls_pk_params_st * pk_params)
{
	int ret;
	unsigned int hash_len;
	const mac_entry_st *me;

	switch (algo) {
	case GNUTLS_PK_EC:	/* we do ECDSA */
		{
			struct ecc_scalar priv;
			struct dsa_signature sig;
			int curve_id = pk_params->flags;
			const struct ecc_curve *curve;

			curve = get_supported_nist_curve(curve_id);
			if (curve == NULL)
				return
				    gnutls_assert_val
				    (GNUTLS_E_ECC_UNSUPPORTED_CURVE);

			ret =
			    _ecc_params_to_privkey(pk_params, &priv,
						   curve);
			if (ret < 0)
				return gnutls_assert_val(ret);

			dsa_signature_init(&sig);

			me = _gnutls_dsa_q_to_hash(algo, pk_params,
						   &hash_len);

			if (hash_len > vdata->size) {
				gnutls_assert();
				_gnutls_debug_log
				    ("Security level of algorithm requires hash %s(%d) or better\n",
				     _gnutls_mac_get_name(me), hash_len);
				hash_len = vdata->size;
			}

			ecdsa_sign(&priv, NULL, rnd_func, hash_len,
				   vdata->data, &sig);

			ret =
			    _gnutls_encode_ber_rs(signature, &sig.r,
						  &sig.s);

			dsa_signature_clear(&sig);
			ecc_scalar_zclear(&priv);

			if (ret < 0) {
				gnutls_assert();
				goto cleanup;
			}
			break;
		}
	case GNUTLS_PK_DSA:
		{
			struct dsa_params pub;
			bigint_t priv;
			struct dsa_signature sig;

			memset(&priv, 0, sizeof(priv));
			memset(&pub, 0, sizeof(pub));
			_dsa_params_get(pk_params, &pub);

			priv = pk_params->params[DSA_X];

			dsa_signature_init(&sig);

			me = _gnutls_dsa_q_to_hash(algo, pk_params,
						   &hash_len);

			if (hash_len > vdata->size) {
				gnutls_assert();
				_gnutls_debug_log
				    ("Security level of algorithm requires hash %s(%d) or better (have: %d)\n",
				     _gnutls_mac_get_name(me), hash_len, (int)vdata->size);
				hash_len = vdata->size;
			}

			ret =
			    dsa_sign(&pub, TOMPZ(priv), NULL, rnd_func,
				     hash_len, vdata->data, &sig);
			if (ret == 0) {
				gnutls_assert();
				ret = GNUTLS_E_PK_SIGN_FAILED;
				goto dsa_fail;
			}

			ret =
			    _gnutls_encode_ber_rs(signature, &sig.r,
						  &sig.s);

		      dsa_fail:
			dsa_signature_clear(&sig);

			if (ret < 0) {
				gnutls_assert();
				goto cleanup;
			}
			break;
		}
	case GNUTLS_PK_RSA:
		{
			struct rsa_private_key priv;
			struct rsa_public_key pub;
			mpz_t s;

			_rsa_params_to_privkey(pk_params, &priv);
			ret = _rsa_params_to_pubkey(pk_params, &pub);
			if (ret < 0)
				return
				    gnutls_assert_val(ret);

			mpz_init(s);

			ret =
			    rsa_pkcs1_sign_tr(&pub, &priv, NULL, rnd_func,
					      vdata->size, vdata->data, s);
			if (ret == 0) {
				gnutls_assert();
				ret = GNUTLS_E_PK_SIGN_FAILED;
				goto rsa_fail;
			}

			ret =
			    _gnutls_mpi_dprint_size(s, signature,
						    pub.size);

		      rsa_fail:
			mpz_clear(s);

			if (ret < 0) {
				gnutls_assert();
				goto cleanup;
			}

			break;
		}
	default:
		gnutls_assert();
		ret = GNUTLS_E_INTERNAL_ERROR;
		goto cleanup;
	}

	ret = 0;

      cleanup:

	FAIL_IF_LIB_ERROR;
	return ret;
}

static int
_wrap_nettle_pk_verify(gnutls_pk_algorithm_t algo,
		       const gnutls_datum_t * vdata,
		       const gnutls_datum_t * signature,
		       const gnutls_pk_params_st * pk_params)
{
	int ret;
	unsigned int hash_len;
	bigint_t tmp[2] = { NULL, NULL };

	switch (algo) {
	case GNUTLS_PK_EC:	/* ECDSA */
		{
			struct ecc_point pub;
			struct dsa_signature sig;
			int curve_id = pk_params->flags;
			const struct ecc_curve *curve;

			curve = get_supported_nist_curve(curve_id);
			if (curve == NULL)
				return
				    gnutls_assert_val
				    (GNUTLS_E_ECC_UNSUPPORTED_CURVE);

			ret =
			    _gnutls_decode_ber_rs(signature, &tmp[0],
						  &tmp[1]);
			if (ret < 0)
				return gnutls_assert_val(ret);

			ret =
			    _ecc_params_to_pubkey(pk_params, &pub, curve);
			if (ret < 0) {
				gnutls_assert();
				goto cleanup;
			}

			memcpy(sig.r, tmp[0], SIZEOF_MPZT);
			memcpy(sig.s, tmp[1], SIZEOF_MPZT);

			_gnutls_dsa_q_to_hash(algo, pk_params, &hash_len);

			if (hash_len > vdata->size)
				hash_len = vdata->size;

			ret =
			    ecdsa_verify(&pub, hash_len, vdata->data,
					 &sig);
			if (ret == 0) {
				gnutls_assert();
				ret = GNUTLS_E_PK_SIG_VERIFY_FAILED;
			} else
				ret = 0;

			ecc_point_clear(&pub);
			break;
		}
	case GNUTLS_PK_DSA:
		{
			struct dsa_params pub;
			struct dsa_signature sig;
			bigint_t y;

			ret =
			    _gnutls_decode_ber_rs(signature, &tmp[0],
						  &tmp[1]);
			if (ret < 0) {
				gnutls_assert();
				goto cleanup;
			}
			memset(&pub, 0, sizeof(pub));
			_dsa_params_get(pk_params, &pub);
			y = pk_params->params[DSA_Y];

			memcpy(sig.r, tmp[0], SIZEOF_MPZT);
			memcpy(sig.s, tmp[1], SIZEOF_MPZT);

			_gnutls_dsa_q_to_hash(algo, pk_params, &hash_len);

			if (hash_len > vdata->size)
				hash_len = vdata->size;

			ret =
			    dsa_verify(&pub, TOMPZ(y), hash_len, vdata->data, &sig);
			if (ret == 0) {
				gnutls_assert();
				ret = GNUTLS_E_PK_SIG_VERIFY_FAILED;
			} else
				ret = 0;

			break;
		}
	case GNUTLS_PK_RSA:
		{
			struct rsa_public_key pub;

			ret = _rsa_params_to_pubkey(pk_params, &pub);
			if (ret < 0)
				return
				    gnutls_assert_val(ret);

			if (signature->size != pub.size)
				return
				    gnutls_assert_val
				    (GNUTLS_E_PK_SIG_VERIFY_FAILED);

			ret =
			    _gnutls_mpi_init_scan_nz(&tmp[0], signature->data,
						signature->size);
			if (ret < 0) {
				gnutls_assert();
				goto cleanup;
			}

			ret =
			    rsa_pkcs1_verify(&pub, vdata->size,
					     vdata->data, TOMPZ(tmp[0]));
			if (ret == 0)
				ret =
				    gnutls_assert_val
				    (GNUTLS_E_PK_SIG_VERIFY_FAILED);
			else
				ret = 0;

			break;
		}
	default:
		gnutls_assert();
		ret = GNUTLS_E_INTERNAL_ERROR;
		goto cleanup;
	}

      cleanup:

	_gnutls_mpi_release(&tmp[0]);
	_gnutls_mpi_release(&tmp[1]);
	FAIL_IF_LIB_ERROR;
	return ret;
}

#if !defined(NETTLE_VERSION_MAJOR) || (NETTLE_VERSION_MAJOR < 3 || (NETTLE_VERSION_MAJOR == 3 && NETTLE_VERSION_MINOR < 4))
# ifdef ENABLE_NON_SUITEB_CURVES
#  define nettle_get_secp_192r1() &nettle_secp_192r1
#  define nettle_get_secp_224r1() &nettle_secp_224r1
# endif
# define nettle_get_secp_256r1() &nettle_secp_256r1
# define nettle_get_secp_384r1() &nettle_secp_384r1
# define nettle_get_secp_521r1() &nettle_secp_521r1
#endif

static inline const struct ecc_curve *get_supported_nist_curve(int curve)
{
	switch (curve) {
#ifdef ENABLE_NON_SUITEB_CURVES
	case GNUTLS_ECC_CURVE_SECP192R1:
		return nettle_get_secp_192r1();
	case GNUTLS_ECC_CURVE_SECP224R1:
		return nettle_get_secp_224r1();
#endif
	case GNUTLS_ECC_CURVE_SECP256R1:
		return nettle_get_secp_256r1();
	case GNUTLS_ECC_CURVE_SECP384R1:
		return nettle_get_secp_384r1();
	case GNUTLS_ECC_CURVE_SECP521R1:
		return nettle_get_secp_521r1();
	default:
		return NULL;
	}
}

static int _wrap_nettle_pk_curve_exists(gnutls_ecc_curve_t curve)
{
	switch (curve) {
		case GNUTLS_ECC_CURVE_X25519:
			return 1;
		default:
			return ((get_supported_nist_curve(curve)!=NULL)?1:0);
	}
}

/* Generates algorithm's parameters. That is:
 *  For DSA: p, q, and g are generated.
 *  For RSA: nothing
 *  For ECDSA: just checks the curve is ok
 */
static int
wrap_nettle_pk_generate_params(gnutls_pk_algorithm_t algo,
			       unsigned int level /*bits or curve*/ ,
			       gnutls_pk_params_st * params)
{
	int ret;
	unsigned int i, q_bits;

	params->algo = algo;

	switch (algo) {
	case GNUTLS_PK_DSA:
	case GNUTLS_PK_DH:
		{
			struct dsa_params pub;
			struct dss_params_validation_seeds cert;
			unsigned index;

			dsa_params_init(&pub);

			if (GNUTLS_BITS_HAVE_SUBGROUP(level)) {
				q_bits = GNUTLS_BITS_TO_SUBGROUP(level);
				level = GNUTLS_BITS_TO_GROUP(level);
			} else {
				q_bits = _gnutls_pk_bits_to_subgroup_bits(level);
			}

			if (q_bits == 0)
				return gnutls_assert_val(GNUTLS_E_ILLEGAL_PARAMETER);

			if (_gnutls_fips_mode_enabled() != 0 || params->flags & GNUTLS_PK_FLAG_PROVABLE) {
				if (algo==GNUTLS_PK_DSA)
					index = 1;
				else
					index = 2;

				if (params->palgo != 0 && params->palgo != GNUTLS_DIG_SHA384) {
					ret = GNUTLS_E_INVALID_REQUEST;
					goto dsa_fail;
				}

				params->palgo = GNUTLS_DIG_SHA384;

				if (params->seed_size) {
					ret =
						_dsa_generate_dss_pqg(&pub, &cert,
							index, params->seed_size, params->seed,
							NULL, NULL, level, q_bits);
				} else {
					ret =
						dsa_generate_dss_pqg(&pub, &cert,
							index, NULL, rnd_func,
							NULL, NULL, level, q_bits);
				}
				if (ret != 1) {
					gnutls_assert();
					ret = GNUTLS_E_PK_GENERATION_ERROR;
					goto dsa_fail;
				}

				if (cert.seed_length && cert.seed_length < sizeof(params->seed)) {
					params->seed_size = cert.seed_length;
					memcpy(params->seed, cert.seed, cert.seed_length);
				}

				/* verify the generated parameters */
				ret = dsa_validate_dss_pqg(&pub, &cert, index);
				if (ret != 1) {
					gnutls_assert();
					ret = GNUTLS_E_PK_GENERATION_ERROR;
					goto dsa_fail;
				}
			} else {
				if (q_bits < 160)
					q_bits = 160;

				ret = dsa_generate_params(&pub, NULL, rnd_func,
							  NULL, NULL, level, q_bits);
				if (ret != 1) {
					gnutls_assert();
					ret = GNUTLS_E_PK_GENERATION_ERROR;
					goto dsa_fail;
				}
			}

			params->params_nr = 0;

			ret = _gnutls_mpi_init_multi(&params->params[DSA_P], &params->params[DSA_Q],
					&params->params[DSA_G], NULL);
			if (ret < 0) {
				gnutls_assert();
				goto dsa_fail;
			}
			params->params_nr = 3;

			mpz_set(TOMPZ(params->params[DSA_P]), pub.p);
			mpz_set(TOMPZ(params->params[DSA_Q]), pub.q);
			mpz_set(TOMPZ(params->params[DSA_G]), pub.g);

			ret = 0;

		      dsa_fail:
			dsa_params_clear(&pub);

			if (ret < 0)
				goto fail;

			break;
		}
	case GNUTLS_PK_RSA:
	case GNUTLS_PK_EC:
		ret = 0;
		break;
	default:
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	FAIL_IF_LIB_ERROR;
	return 0;

      fail:

	for (i = 0; i < params->params_nr; i++) {
		_gnutls_mpi_release(&params->params[i]);
	}
	params->params_nr = 0;

	FAIL_IF_LIB_ERROR;
	return ret;
}

#ifdef ENABLE_FIPS140
int _gnutls_dh_generate_key(gnutls_dh_params_t dh_params,
			    gnutls_datum_t *priv_key, gnutls_datum_t *pub_key);

int _gnutls_dh_compute_key(gnutls_dh_params_t dh_params,
			   const gnutls_datum_t *priv_key, const gnutls_datum_t *pub_key,
			   const gnutls_datum_t *peer_key, gnutls_datum_t *Z);

int _gnutls_ecdh_compute_key(gnutls_ecc_curve_t curve,
			   const gnutls_datum_t *x, const gnutls_datum_t *y,
			   const gnutls_datum_t *k,
			   const gnutls_datum_t *peer_x, const gnutls_datum_t *peer_y,
			   gnutls_datum_t *Z);

int _gnutls_ecdh_generate_key(gnutls_ecc_curve_t curve,
			      gnutls_datum_t *x, gnutls_datum_t *y,
			      gnutls_datum_t *k);


int _gnutls_dh_generate_key(gnutls_dh_params_t dh_params,
			    gnutls_datum_t *priv_key, gnutls_datum_t *pub_key)
{
	gnutls_pk_params_st params;
	int ret;

	gnutls_pk_params_init(&params);
	params.params[DH_P] = _gnutls_mpi_copy(dh_params->params[0]);
	params.params[DH_G] = _gnutls_mpi_copy(dh_params->params[1]);

	params.params_nr = 3; /* include empty q */
	params.algo = GNUTLS_PK_DH;

	priv_key->data = NULL;
	pub_key->data = NULL;

	ret = _gnutls_pk_generate_keys(GNUTLS_PK_DH, dh_params->q_bits, &params);
	if (ret < 0) {
		return gnutls_assert_val(ret);
	}

	ret =
	    _gnutls_mpi_dprint_lz(params.params[DH_X], priv_key);
	if (ret < 0) {
		gnutls_assert();
		goto fail;
	}

	ret =
	    _gnutls_mpi_dprint_lz(params.params[DH_Y], pub_key);
	if (ret < 0) {
		gnutls_assert();
		goto fail;
	}

	ret = 0;
	goto cleanup;
 fail:
	gnutls_free(pub_key->data);
	gnutls_free(priv_key->data);
 cleanup:
	gnutls_pk_params_clear(&params);
	return ret;
}

/* Note that the value of Z will have the leading bytes stripped if they are zero -
 * which follows the TLS approach. */
int _gnutls_dh_compute_key(gnutls_dh_params_t dh_params,
			   const gnutls_datum_t *priv_key, const gnutls_datum_t *pub_key,
			   const gnutls_datum_t *peer_key, gnutls_datum_t *Z)
{
	gnutls_pk_params_st pub, priv;
	int ret;

	gnutls_pk_params_init(&pub);
	gnutls_pk_params_init(&priv);
	pub.algo = GNUTLS_PK_DH;

	if (_gnutls_mpi_init_scan_nz
		    (&pub.params[DH_Y], peer_key->data,
		     peer_key->size) != 0) {
		ret =
		    gnutls_assert_val(GNUTLS_E_MPI_SCAN_FAILED);
		goto cleanup;
	}

	priv.params[DH_P] = _gnutls_mpi_copy(dh_params->params[0]);
	priv.params[DH_G] = _gnutls_mpi_copy(dh_params->params[1]);

	if (_gnutls_mpi_init_scan_nz
		    (&priv.params[DH_X], priv_key->data,
		     priv_key->size) != 0) {
		ret =
		    gnutls_assert_val(GNUTLS_E_MPI_SCAN_FAILED);
		goto cleanup;
	}

	priv.params_nr = 3; /* include empty q */
	priv.algo = GNUTLS_PK_DH;

	Z->data = NULL;

	ret = _gnutls_pk_derive(GNUTLS_PK_DH, Z, &priv, &pub);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = 0;
 cleanup:
	gnutls_pk_params_clear(&pub);
	gnutls_pk_params_clear(&priv);
	return ret;
}

int _gnutls_ecdh_generate_key(gnutls_ecc_curve_t curve,
			      gnutls_datum_t *x, gnutls_datum_t *y,
			      gnutls_datum_t *k)
{
	gnutls_pk_params_st params;
	int ret;

	gnutls_pk_params_init(&params);
	params.flags = curve;
	params.algo = GNUTLS_PK_EC;

	x->data = NULL;
	y->data = NULL;
	k->data = NULL;

	ret = _gnutls_pk_generate_keys(GNUTLS_PK_EC, curve, &params);
	if (ret < 0) {
		return gnutls_assert_val(ret);
	}

	ret =
	    _gnutls_mpi_dprint_lz(params.params[ECC_X], x);
	if (ret < 0) {
		gnutls_assert();
		goto fail;
	}

	ret =
	    _gnutls_mpi_dprint_lz(params.params[ECC_Y], y);
	if (ret < 0) {
		gnutls_assert();
		goto fail;
	}

	ret =
	    _gnutls_mpi_dprint_lz(params.params[ECC_K], k);
	if (ret < 0) {
		gnutls_assert();
		goto fail;
	}

	ret = 0;
	goto cleanup;
 fail:
	gnutls_free(y->data);
	gnutls_free(x->data);
	gnutls_free(k->data);
 cleanup:
	gnutls_pk_params_clear(&params);
	return ret;
}

int _gnutls_ecdh_compute_key(gnutls_ecc_curve_t curve,
			   const gnutls_datum_t *x, const gnutls_datum_t *y,
			   const gnutls_datum_t *k,
			   const gnutls_datum_t *peer_x, const gnutls_datum_t *peer_y,
			   gnutls_datum_t *Z)
{
	gnutls_pk_params_st pub, priv;
	int ret;

	gnutls_pk_params_init(&pub);
	gnutls_pk_params_init(&priv);

	pub.algo = GNUTLS_PK_EC;
	pub.flags = curve;

	if (_gnutls_mpi_init_scan_nz
		    (&pub.params[ECC_Y], peer_y->data,
		     peer_y->size) != 0) {
		ret =
		    gnutls_assert_val(GNUTLS_E_MPI_SCAN_FAILED);
		goto cleanup;
	}

	if (_gnutls_mpi_init_scan_nz
		    (&pub.params[ECC_X], peer_x->data,
		     peer_x->size) != 0) {
		ret =
		    gnutls_assert_val(GNUTLS_E_MPI_SCAN_FAILED);
		goto cleanup;
	}

	pub.params_nr = 2;

	if (_gnutls_mpi_init_scan_nz
		    (&priv.params[ECC_Y], y->data,
		     y->size) != 0) {
		ret =
		    gnutls_assert_val(GNUTLS_E_MPI_SCAN_FAILED);
		goto cleanup;
	}

	if (_gnutls_mpi_init_scan_nz
		    (&priv.params[ECC_X], x->data,
		     x->size) != 0) {
		ret =
		    gnutls_assert_val(GNUTLS_E_MPI_SCAN_FAILED);
		goto cleanup;
	}

	if (_gnutls_mpi_init_scan_nz
		    (&priv.params[ECC_K], k->data,
		     k->size) != 0) {
		ret =
		    gnutls_assert_val(GNUTLS_E_MPI_SCAN_FAILED);
		goto cleanup;
	}


	priv.params_nr = 3;
	priv.algo = GNUTLS_PK_EC;
	priv.flags = curve;

	Z->data = NULL;

	ret = _gnutls_pk_derive(GNUTLS_PK_EC, Z, &priv, &pub);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = 0;
 cleanup:
	gnutls_pk_params_clear(&pub);
	gnutls_pk_params_clear(&priv);
	return ret;
}

static int pct_test(gnutls_pk_algorithm_t algo, const gnutls_pk_params_st* params)
{
int ret;
gnutls_datum_t sig = {NULL, 0};
const char const_data[20] = "onetwothreefourfive";
gnutls_datum_t ddata, tmp = {NULL,0};
char* gen_data = NULL;

	if (algo == GNUTLS_PK_DSA || algo == GNUTLS_PK_EC) {
		unsigned hash_len;

		_gnutls_dsa_q_to_hash(algo, params, &hash_len);
		gen_data = gnutls_malloc(hash_len);
		gnutls_rnd(GNUTLS_RND_NONCE, gen_data, hash_len);

		ddata.data = (void*)gen_data;
		ddata.size = hash_len;
	} else {
		ddata.data = (void*)const_data;
		ddata.size = sizeof(const_data);
	}

	switch (algo) {
	case GNUTLS_PK_RSA:
		ret = _gnutls_pk_encrypt(algo, &sig, &ddata, params);
		if (ret < 0) {
			ret = gnutls_assert_val(GNUTLS_E_PK_GENERATION_ERROR);
			goto cleanup;
		}

		if (ddata.size == sig.size && memcmp(ddata.data, sig.data, sig.size) == 0) {
			ret = gnutls_assert_val(GNUTLS_E_PK_GENERATION_ERROR);
			gnutls_assert();
			goto cleanup;
		}

		ret = _gnutls_pk_decrypt(algo, &tmp, &sig, params);
		if (ret < 0) {
			ret = gnutls_assert_val(GNUTLS_E_PK_GENERATION_ERROR);
			gnutls_assert();
			goto cleanup;
		}

		if (tmp.size != ddata.size || memcmp(tmp.data, ddata.data, tmp.size) != 0) {
			ret = gnutls_assert_val(GNUTLS_E_PK_GENERATION_ERROR);
			gnutls_assert();
			goto cleanup;
		}

		free(sig.data);
		sig.data = NULL;

		/* Here we don't know the purpose of the key. Check both
		 * signing and encryption.
		 */
	case GNUTLS_PK_EC: /* we only do keys for ECDSA */
	case GNUTLS_PK_DSA:
		ret = _gnutls_pk_sign(algo, &sig, &ddata, params);
		if (ret < 0) {
			ret = gnutls_assert_val(GNUTLS_E_PK_GENERATION_ERROR);
			goto cleanup;
		}

		ret = _gnutls_pk_verify(algo, &ddata, &sig, params);
		if (ret < 0) {
			ret = gnutls_assert_val(GNUTLS_E_PK_GENERATION_ERROR);
			gnutls_assert();
			goto cleanup;
		}
		break;
	case GNUTLS_PK_DH:
	case GNUTLS_PK_ECDHX:
		ret = 0;
		goto cleanup;
	default:
		ret = gnutls_assert_val(GNUTLS_E_UNKNOWN_PK_ALGORITHM);
		goto cleanup;
	}

	ret = 0;
cleanup:
	if (ret == GNUTLS_E_PK_GENERATION_ERROR) {
		_gnutls_switch_lib_state(LIB_STATE_ERROR);
	}
	gnutls_free(gen_data);
	gnutls_free(sig.data);
	gnutls_free(tmp.data);
	return ret;
}
#endif

/* To generate a DH key either q must be set in the params or
 * level should be set to the number of required bits.
 */
static int
wrap_nettle_pk_generate_keys(gnutls_pk_algorithm_t algo,
			       unsigned int level /*bits or curve */ ,
			       gnutls_pk_params_st * params)
{
	int ret;
	unsigned int i;

	switch (algo) {
	case GNUTLS_PK_DSA:
#ifdef ENABLE_FIPS140
		if (_gnutls_fips_mode_enabled() != 0) {
			struct dsa_params pub;
			mpz_t x, y;

			if (params->params[DSA_Q] == NULL)
				return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

			_dsa_params_get(params, &pub);

			mpz_init(x);
			mpz_init(y);

			ret =
			    dsa_generate_dss_keypair(&pub, y, x,
						 NULL, rnd_func,
						 NULL, NULL);
			if (ret != 1) {
				gnutls_assert();
				ret = GNUTLS_E_PK_GENERATION_ERROR;
				goto dsa_fail;
			}

			ret = _gnutls_mpi_init_multi(&params->params[DSA_Y], &params->params[DSA_X], NULL);
			if (ret < 0) {
				gnutls_assert();
				goto dsa_fail;
			}

			mpz_set(TOMPZ(params->params[DSA_Y]), y);
			mpz_set(TOMPZ(params->params[DSA_X]), x);
			params->params_nr += 2;

		      dsa_fail:
			mpz_clear(x);
			mpz_clear(y);

			if (ret < 0)
				goto fail;

			break;
		}
#endif
	case GNUTLS_PK_DH:
		{
			struct dsa_params pub;
			mpz_t r;
			mpz_t x, y;
			int max_tries;
			unsigned have_q = 0;

			if (algo != params->algo)
				return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

			_dsa_params_get(params, &pub);

			if (params->params[DSA_Q] != NULL)
				have_q = 1;

			/* This check is for the case !ENABLE_FIPS140 */
			if (algo == GNUTLS_PK_DSA && have_q == 0)
				return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

			mpz_init(r);
			mpz_init(x);
			mpz_init(y);

			max_tries = 3;
			do {
				if (have_q) {
					mpz_set(r, pub.q);
					mpz_sub_ui(r, r, 2);
					nettle_mpz_random(x, NULL, rnd_func, r);
					mpz_add_ui(x, x, 1);
				} else {
					unsigned size = mpz_sizeinbase(pub.p, 2);
					if (level == 0)
						level = MIN(size, DH_EXPONENT_SIZE(size));
					nettle_mpz_random_size(x, NULL, rnd_func, level);

					if (level >= size)
						mpz_mod(x, x, pub.p);
				}

				mpz_powm(y, pub.g, x, pub.p);

				max_tries--;
				if (max_tries <= 0) {
					gnutls_assert();
					ret = GNUTLS_E_RANDOM_FAILED;
					goto dh_fail;
				}
			} while(mpz_cmp_ui(y, 1) == 0);

			ret = _gnutls_mpi_init_multi(&params->params[DSA_Y], &params->params[DSA_X], NULL);
			if (ret < 0) {
				gnutls_assert();
				goto dh_fail;
			}

			mpz_set(TOMPZ(params->params[DSA_Y]), y);
			mpz_set(TOMPZ(params->params[DSA_X]), x);
			params->params_nr += 2;

			ret = 0;

		      dh_fail:
			mpz_clear(r);
			mpz_clear(x);
			mpz_clear(y);

			if (ret < 0)
				goto fail;

			break;
		}
	case GNUTLS_PK_RSA:
		{
			struct rsa_public_key pub;
			struct rsa_private_key priv;

			rsa_public_key_init(&pub);
			rsa_private_key_init(&priv);

			mpz_set_ui(pub.e, 65537);

			if ((params->flags & GNUTLS_PK_FLAG_PROVABLE) || _gnutls_fips_mode_enabled() != 0) {
				params->flags |= GNUTLS_PK_FLAG_PROVABLE;
				if (params->palgo != 0 && params->palgo != GNUTLS_DIG_SHA384) {
					ret = GNUTLS_E_INVALID_REQUEST;
					goto rsa_fail;
				}

				params->palgo = GNUTLS_DIG_SHA384;

				if (params->seed_size) {
					ret = _rsa_generate_fips186_4_keypair(&pub, &priv,
						params->seed_size, params->seed,
						NULL, NULL, level);
				} else {
					params->seed_size = sizeof(params->seed);
					ret =
					    rsa_generate_fips186_4_keypair(&pub, &priv, NULL,
							 rnd_func, NULL, NULL,
							 &params->seed_size, params->seed,
							 level);
				}
			} else {
				ret =
				    rsa_generate_keypair(&pub, &priv, NULL,
						 rnd_func, NULL, NULL,
						 level, 0);
			}
			if (ret != 1) {
				gnutls_assert();
				ret = GNUTLS_E_PK_GENERATION_ERROR;
				goto rsa_fail;
			}

			params->params_nr = 0;
			for (i = 0; i < RSA_PRIVATE_PARAMS; i++) {
				ret = _gnutls_mpi_init(&params->params[i]);
				if (ret < 0) {
					gnutls_assert();
					goto rsa_fail;
				}
				params->params_nr++;
			}

			mpz_set(TOMPZ(params->params[0]), pub.n);
			mpz_set(TOMPZ(params->params[1]), pub.e);
			mpz_set(TOMPZ(params->params[2]), priv.d);
			mpz_set(TOMPZ(params->params[3]), priv.p);
			mpz_set(TOMPZ(params->params[4]), priv.q);
			mpz_set(TOMPZ(params->params[5]), priv.c);
			mpz_set(TOMPZ(params->params[6]), priv.a);
			mpz_set(TOMPZ(params->params[7]), priv.b);

			ret = 0;

		      rsa_fail:
			rsa_private_key_clear(&priv);
			rsa_public_key_clear(&pub);

			if (ret < 0)
				goto fail;

			break;
		}
	case GNUTLS_PK_EC:
		if (params->flags & GNUTLS_PK_FLAG_PROVABLE)
			return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

		{
			struct ecc_scalar key;
			struct ecc_point pub;
			const struct ecc_curve *curve;

			curve = get_supported_nist_curve(level);
			if (curve == NULL)
				return
				    gnutls_assert_val
				    (GNUTLS_E_ECC_UNSUPPORTED_CURVE);

			ecc_scalar_init(&key, curve);
			ecc_point_init(&pub, curve);

			ecdsa_generate_keypair(&pub, &key, NULL, rnd_func);

			ret = _gnutls_mpi_init_multi(&params->params[ECC_X], &params->params[ECC_Y],
					&params->params[ECC_K], NULL);
			if (ret < 0) {
				gnutls_assert();
				goto ecc_fail;
			}

			params->flags = level;
			params->params_nr = ECC_PRIVATE_PARAMS;

			ecc_point_get(&pub, TOMPZ(params->params[ECC_X]),
				      TOMPZ(params->params[ECC_Y]));
			ecc_scalar_get(&key, TOMPZ(params->params[ECC_K]));

			ret = 0;

		      ecc_fail:
			ecc_point_clear(&pub);
			ecc_scalar_clear(&key);

			if (ret < 0)
				goto fail;

			break;
		}
	case GNUTLS_PK_ECDHX:
		{
			unsigned size = gnutls_ecc_curve_get_size(level);

			if (size == 0)
				return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

			params->flags = level;

			params->raw_priv.data = gnutls_malloc(size);
			if (params->raw_priv.data == NULL)
				return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

			params->raw_pub.data = gnutls_malloc(size);
			if (params->raw_pub.data == NULL) {
				ret = gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);
				goto fail;
			}

			ret = gnutls_rnd(GNUTLS_RND_RANDOM, params->raw_priv.data, size);
			if (ret < 0) {
				ret = gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);
				goto fail;
			}
			params->raw_pub.size = size;
			params->raw_priv.size = size;

			curve25519_mul_g(params->raw_pub.data, params->raw_priv.data);
			break;
		}
	default:
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

#ifdef ENABLE_FIPS140
	ret = pct_test(algo, params);
	if (ret < 0) {
		gnutls_assert();
		goto fail;
	}
#endif

	FAIL_IF_LIB_ERROR;
	return 0;

      fail:

	for (i = 0; i < params->params_nr; i++) {
		_gnutls_mpi_release(&params->params[i]);
	}
	params->params_nr = 0;
	gnutls_free(params->raw_priv.data);
	gnutls_free(params->raw_pub.data);
	params->raw_priv.data = NULL;
	params->raw_pub.data = NULL;

	FAIL_IF_LIB_ERROR;
	return ret;
}


static int
wrap_nettle_pk_verify_priv_params(gnutls_pk_algorithm_t algo,
			     const gnutls_pk_params_st * params)
{
	int ret;

	switch (algo) {
	case GNUTLS_PK_RSA:
		{
			bigint_t t1 = NULL, t2 = NULL;

			if (params->params_nr != RSA_PRIVATE_PARAMS)
				return
				    gnutls_assert_val
				    (GNUTLS_E_INVALID_REQUEST);

			ret = _gnutls_mpi_init_multi(&t1, &t2, NULL);
			if (ret < 0)
				return
				    gnutls_assert_val(ret);

			_gnutls_mpi_mulm(t1, params->params[RSA_PRIME1],
					 params->params[RSA_PRIME2],
					 params->params[RSA_MODULUS]);
			if (_gnutls_mpi_cmp_ui(t1, 0) != 0) {
				ret =
				    gnutls_assert_val
				    (GNUTLS_E_ILLEGAL_PARAMETER);
				goto rsa_cleanup;
			}

			mpz_invert(TOMPZ(t1),
				   TOMPZ(params->params[RSA_PRIME2]),
				   TOMPZ(params->params[RSA_PRIME1]));
			if (_gnutls_mpi_cmp(t1, params->params[RSA_COEF])
			    != 0) {
				ret =
				    gnutls_assert_val
				    (GNUTLS_E_ILLEGAL_PARAMETER);
				goto rsa_cleanup;
			}

			/* [RSA_PRIME1] = d % p-1, [RSA_PRIME2] = d % q-1 */
			_gnutls_mpi_sub_ui(t1, params->params[RSA_PRIME1],
					   1);
			ret = _gnutls_mpi_modm(t2, params->params[RSA_PRIV], t1);
			if (ret < 0) {
				ret =
				    gnutls_assert_val
				    (GNUTLS_E_MEMORY_ERROR);
				goto rsa_cleanup;
			}

			if (_gnutls_mpi_cmp(params->params[RSA_E1], t2) !=
			    0) {
				ret =
				    gnutls_assert_val
				    (GNUTLS_E_ILLEGAL_PARAMETER);
				goto rsa_cleanup;
			}

			_gnutls_mpi_sub_ui(t1, params->params[RSA_PRIME2],
					   1);

			ret = _gnutls_mpi_modm(t2, params->params[RSA_PRIV], t1);
			if (ret < 0) {
				ret =
				    gnutls_assert_val
				    (GNUTLS_E_MEMORY_ERROR);
				goto rsa_cleanup;
			}

			if (_gnutls_mpi_cmp(params->params[RSA_E2], t2) !=
			    0) {
				ret =
				    gnutls_assert_val
				    (GNUTLS_E_ILLEGAL_PARAMETER);
				goto rsa_cleanup;
			}

			ret = 0;

		      rsa_cleanup:
			zrelease_mpi_key(&t1);
			zrelease_mpi_key(&t2);
		}

		break;
	case GNUTLS_PK_DSA:
		{
			bigint_t t1 = NULL;

			if (params->params_nr != DSA_PRIVATE_PARAMS)
				return
				    gnutls_assert_val
				    (GNUTLS_E_INVALID_REQUEST);

			ret = _gnutls_mpi_init(&t1);
			if (ret < 0)
				return
				    gnutls_assert_val(ret);

			ret = _gnutls_mpi_powm(t1, params->params[DSA_G],
					 params->params[DSA_X],
					 params->params[DSA_P]);
			if (ret < 0) {
				gnutls_assert();
				goto dsa_cleanup;
			}

			if (_gnutls_mpi_cmp(t1, params->params[DSA_Y]) !=
			    0) {
				ret =
				    gnutls_assert_val
				    (GNUTLS_E_ILLEGAL_PARAMETER);
				goto dsa_cleanup;
			}

			ret = 0;

		      dsa_cleanup:
			zrelease_mpi_key(&t1);
		}

		break;
	case GNUTLS_PK_EC:
		{
			struct ecc_point r, pub;
			struct ecc_scalar priv;
			mpz_t x1, y1, x2, y2;
			const struct ecc_curve *curve;

			if (params->params_nr != ECC_PRIVATE_PARAMS)
				return
				    gnutls_assert_val
				    (GNUTLS_E_INVALID_REQUEST);

			curve = get_supported_nist_curve(params->flags);
			if (curve == NULL)
				return
				    gnutls_assert_val
				    (GNUTLS_E_ECC_UNSUPPORTED_CURVE);

			ret = _ecc_params_to_pubkey(params, &pub, curve);
			if (ret < 0)
				return gnutls_assert_val(ret);

			ret = _ecc_params_to_privkey(params, &priv, curve);
			if (ret < 0) {
				ecc_point_clear(&pub);
				return gnutls_assert_val(ret);
			}

			ecc_point_init(&r, curve);
			/* verify that x,y lie on the curve */
			ret =
			    ecc_point_set(&r, TOMPZ(params->params[ECC_X]),
					  TOMPZ(params->params[ECC_Y]));
			if (ret == 0) {
				ret =
				    gnutls_assert_val
				    (GNUTLS_E_ILLEGAL_PARAMETER);
				goto ecc_cleanup;
			}
			ecc_point_clear(&r);

			ecc_point_init(&r, curve);
			ecc_point_mul_g(&r, &priv);

			mpz_init(x1);
			mpz_init(y1);
			ecc_point_get(&r, x1, y1);
			ecc_point_zclear(&r);

			mpz_init(x2);
			mpz_init(y2);
			ecc_point_get(&pub, x2, y2);

			/* verify that k*(Gx,Gy)=(x,y) */
			if (mpz_cmp(x1, x2) != 0 || mpz_cmp(y1, y2) != 0) {
				ret =
				    gnutls_assert_val
				    (GNUTLS_E_ILLEGAL_PARAMETER);
				goto ecc_cleanup;
			}

			ret = 0;

		      ecc_cleanup:
			ecc_scalar_zclear(&priv);
			ecc_point_clear(&pub);

			mpz_clear(x1);
			mpz_clear(y1);
			mpz_clear(x2);
			mpz_clear(y2);
		}
		break;
	default:
		ret = gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);
	}

	return ret;
}

static int
wrap_nettle_pk_verify_pub_params(gnutls_pk_algorithm_t algo,
			     const gnutls_pk_params_st * params)
{
	int ret;

	switch (algo) {
	case GNUTLS_PK_RSA:
	case GNUTLS_PK_DSA:
		return 0;
	case GNUTLS_PK_EC:
		{
			/* just verify that x and y lie on the curve */
			struct ecc_point r, pub;
			const struct ecc_curve *curve;

			if (params->params_nr != ECC_PUBLIC_PARAMS)
				return
				    gnutls_assert_val
				    (GNUTLS_E_INVALID_REQUEST);

			curve = get_supported_nist_curve(params->flags);
			if (curve == NULL)
				return
				    gnutls_assert_val
				    (GNUTLS_E_ECC_UNSUPPORTED_CURVE);

			ret = _ecc_params_to_pubkey(params, &pub, curve);
			if (ret < 0)
				return gnutls_assert_val(ret);

			ecc_point_init(&r, curve);
			/* verify that x,y lie on the curve */
			ret =
			    ecc_point_set(&r, TOMPZ(params->params[ECC_X]),
					  TOMPZ(params->params[ECC_Y]));
			if (ret == 0) {
				ret =
				    gnutls_assert_val
				    (GNUTLS_E_ILLEGAL_PARAMETER);
				goto ecc_cleanup;
			}
			ecc_point_clear(&r);

			ret = 0;

		      ecc_cleanup:
			ecc_point_clear(&pub);
		}
		break;
	default:
		ret = gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);
	}

	return ret;
}

static int calc_rsa_exp(gnutls_pk_params_st * params)
{
	bigint_t tmp;
	int ret;

	if (params->params_nr < RSA_PRIVATE_PARAMS - 2) {
		gnutls_assert();
		return GNUTLS_E_INTERNAL_ERROR;
	}

	params->params[6] = params->params[7] = NULL;

	ret = _gnutls_mpi_init_multi(&tmp, &params->params[6], &params->params[7], NULL);
	if (ret < 0)
		return gnutls_assert_val(ret);

	/* [6] = d % p-1, [7] = d % q-1 */
	_gnutls_mpi_sub_ui(tmp, params->params[3], 1);
	ret =
	    _gnutls_mpi_modm(params->params[6], params->params[2] /*d */ , tmp);
	if (ret < 0)
		goto fail;

	_gnutls_mpi_sub_ui(tmp, params->params[4], 1);
	ret =
	    _gnutls_mpi_modm(params->params[7], params->params[2] /*d */ , tmp);
	if (ret < 0)
		goto fail;

	zrelease_mpi_key(&tmp);

	return 0;

fail:
	zrelease_mpi_key(&tmp);
	zrelease_mpi_key(&params->params[6]);
	zrelease_mpi_key(&params->params[7]);

	return ret;
}


static int
wrap_nettle_pk_fixup(gnutls_pk_algorithm_t algo,
		     gnutls_direction_t direction,
		     gnutls_pk_params_st * params)
{
	int ret;

	if (direction != GNUTLS_IMPORT)
		return 0;

	if (algo == GNUTLS_PK_RSA) {
		struct rsa_private_key priv;

		/* do not trust the generated values. Some old private keys
		 * generated by us have mess on the values. Those were very
		 * old but it seemed some of the shipped example private
		 * keys were as old.
		 */
		if (params->params_nr < RSA_PRIVATE_PARAMS - 3)
			return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

		if (params->params[RSA_COEF] == NULL) {
			ret = _gnutls_mpi_init(&params->params[RSA_COEF]);
			if (ret < 0)
				return gnutls_assert_val(ret);
		}

		if (mpz_cmp_ui(TOMPZ(params->params[RSA_PRIME1]), 0) == 0)
			return gnutls_assert_val(GNUTLS_E_PK_INVALID_PRIVKEY);

		if (mpz_invert(TOMPZ(params->params[RSA_COEF]),
			       TOMPZ(params->params[RSA_PRIME2]),
			       TOMPZ(params->params[RSA_PRIME1])) == 0)
			return gnutls_assert_val(GNUTLS_E_PK_INVALID_PRIVKEY);

		/* calculate exp1 [6] and exp2 [7] */
		zrelease_mpi_key(&params->params[RSA_E1]);
		zrelease_mpi_key(&params->params[RSA_E2]);

		ret = calc_rsa_exp(params);
		if (ret < 0)
			return gnutls_assert_val(ret);

		params->params_nr = RSA_PRIVATE_PARAMS;

		/* perform nettle's internal checks */
		_rsa_params_to_privkey(params, &priv);
		ret = rsa_private_key_prepare(&priv);
		if (ret == 0) {
			return gnutls_assert_val(GNUTLS_E_PK_INVALID_PRIVKEY);
		}
	}

	return 0;
}

int crypto_pk_prio = INT_MAX;

gnutls_crypto_pk_st _gnutls_pk_ops = {
	.encrypt = _wrap_nettle_pk_encrypt,
	.decrypt = _wrap_nettle_pk_decrypt,
	.sign = _wrap_nettle_pk_sign,
	.verify = _wrap_nettle_pk_verify,
	.verify_priv_params = wrap_nettle_pk_verify_priv_params,
	.verify_pub_params = wrap_nettle_pk_verify_pub_params,
	.generate_params = wrap_nettle_pk_generate_params,
	.generate_keys = wrap_nettle_pk_generate_keys,
	.pk_fixup_private_params = wrap_nettle_pk_fixup,
	.derive = _wrap_nettle_pk_derive,
	.curve_exists = _wrap_nettle_pk_curve_exists,
};
