/*
 * Copyright (C) 2003-2016 Free Software Foundation, Inc.
 * Copyright (C) 2012-2016 Nikos Mavrogiannopoulos
 * Copyright (C) 2015-2017 Red Hat, Inc.
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

#include "gnutls_int.h"
#include <datum.h>
#include <global.h>
#include "errors.h"
#include <tls-sig.h>
#include <common.h>
#include <x509.h>
#include <x509_b64.h>
#include <x509_int.h>
#include <pk.h>
#include <mpi.h>
#include <ecc.h>
#include <pin.h>

/**
 * gnutls_x509_privkey_init:
 * @key: A pointer to the type to be initialized
 *
 * This function will initialize a private key type.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int gnutls_x509_privkey_init(gnutls_x509_privkey_t * key)
{
	FAIL_IF_LIB_ERROR;

	*key = gnutls_calloc(1, sizeof(gnutls_x509_privkey_int));

	if (*key) {
		(*key)->key = ASN1_TYPE_EMPTY;
		return 0;	/* success */
	}

	return GNUTLS_E_MEMORY_ERROR;
}

void _gnutls_x509_privkey_reinit(gnutls_x509_privkey_t key)
{
	gnutls_pk_params_clear(&key->params);
	gnutls_pk_params_release(&key->params);
	/* avoid re-use of fields which may have had some sensible value */
	memset(&key->params, 0, sizeof(key->params));

	if (key->key)
		asn1_delete_structure2(&key->key, ASN1_DELETE_FLAG_ZEROIZE);
	key->key = ASN1_TYPE_EMPTY;
}

/**
 * gnutls_x509_privkey_deinit:
 * @key: The key to be deinitialized
 *
 * This function will deinitialize a private key structure.
 **/
void gnutls_x509_privkey_deinit(gnutls_x509_privkey_t key)
{
	if (!key)
		return;

	_gnutls_x509_privkey_reinit(key);
	gnutls_free(key);
}

/**
 * gnutls_x509_privkey_cpy:
 * @dst: The destination key, which should be initialized.
 * @src: The source key
 *
 * This function will copy a private key from source to destination
 * key. Destination has to be initialized.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_x509_privkey_cpy(gnutls_x509_privkey_t dst,
			gnutls_x509_privkey_t src)
{
	int ret;

	if (!src || !dst)
		return GNUTLS_E_INVALID_REQUEST;

	ret = _gnutls_pk_params_copy(&dst->params, &src->params);
	if (ret < 0) {
		return gnutls_assert_val(ret);
	}

	ret =
	    _gnutls_asn1_encode_privkey(&dst->key,
					&dst->params);
	if (ret < 0) {
		gnutls_assert();
		gnutls_pk_params_release(&dst->params);
		return ret;
	}

	return 0;
}

/* Converts an RSA PKCS#1 key to
 * an internal structure (gnutls_private_key)
 */
ASN1_TYPE
_gnutls_privkey_decode_pkcs1_rsa_key(const gnutls_datum_t * raw_key,
				     gnutls_x509_privkey_t pkey)
{
	int result;
	ASN1_TYPE pkey_asn;

	gnutls_pk_params_init(&pkey->params);

	if ((result =
	     asn1_create_element(_gnutls_get_gnutls_asn(),
				 "GNUTLS.RSAPrivateKey",
				 &pkey_asn)) != ASN1_SUCCESS) {
		gnutls_assert();
		return NULL;
	}

	result =
	    _asn1_strict_der_decode(&pkey_asn, raw_key->data, raw_key->size,
			      NULL);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		goto error;
	}

	if ((result = _gnutls_x509_read_int(pkey_asn, "modulus",
					    &pkey->params.params[0])) < 0)
	{
		gnutls_assert();
		goto error;
	}
	pkey->params.params_nr++;

	if ((result =
	     _gnutls_x509_read_int(pkey_asn, "publicExponent",
				   &pkey->params.params[1])) < 0) {
		gnutls_assert();
		goto error;
	}
	pkey->params.params_nr++;

	if ((result =
	     _gnutls_x509_read_key_int(pkey_asn, "privateExponent",
				   &pkey->params.params[2])) < 0) {
		gnutls_assert();
		goto error;
	}
	pkey->params.params_nr++;

	if ((result = _gnutls_x509_read_key_int(pkey_asn, "prime1",
					    &pkey->params.params[3])) < 0)
	{
		gnutls_assert();
		goto error;
	}
	pkey->params.params_nr++;

	if ((result = _gnutls_x509_read_key_int(pkey_asn, "prime2",
					    &pkey->params.params[4])) < 0)
	{
		gnutls_assert();
		goto error;
	}
	pkey->params.params_nr++;

	if ((result = _gnutls_x509_read_key_int(pkey_asn, "coefficient",
					    &pkey->params.params[5])) < 0)
	{
		gnutls_assert();
		goto error;
	}
	pkey->params.params_nr++;

	if ((result = _gnutls_x509_read_key_int(pkey_asn, "exponent1",
					    &pkey->params.params[6])) < 0)
	{
		gnutls_assert();
		goto error;
	}
	pkey->params.params_nr++;

	if ((result = _gnutls_x509_read_key_int(pkey_asn, "exponent2",
					    &pkey->params.params[7])) < 0)
	{
		gnutls_assert();
		goto error;
	}
	pkey->params.params_nr++;

	pkey->params.params_nr = RSA_PRIVATE_PARAMS;
	pkey->params.algo = GNUTLS_PK_RSA;

	return pkey_asn;

      error:
	asn1_delete_structure2(&pkey_asn, ASN1_DELETE_FLAG_ZEROIZE);
	gnutls_pk_params_clear(&pkey->params);
	gnutls_pk_params_release(&pkey->params);
	return NULL;
}

/* Converts an ECC key to
 * an internal structure (gnutls_private_key)
 */
int
_gnutls_privkey_decode_ecc_key(ASN1_TYPE* pkey_asn, const gnutls_datum_t * raw_key,
			       gnutls_x509_privkey_t pkey, gnutls_ecc_curve_t curve)
{
	int ret;
	unsigned int version;
	char oid[MAX_OID_SIZE];
	int oid_size;
	gnutls_datum_t out;

	if (curve_is_eddsa(curve)) {
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);
	}

	gnutls_pk_params_init(&pkey->params);

	if ((ret =
	     asn1_create_element(_gnutls_get_gnutls_asn(),
				 "GNUTLS.ECPrivateKey",
				 pkey_asn)) != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(ret);
	}

	ret =
	    _asn1_strict_der_decode(pkey_asn, raw_key->data, raw_key->size,
			      NULL);
	if (ret != ASN1_SUCCESS) {
		gnutls_assert();
		ret = _gnutls_asn2err(ret);
		goto error;
	}

	ret = _gnutls_x509_read_uint(*pkey_asn, "Version", &version);
	if (ret < 0) {
		gnutls_assert();
		goto error;
	}

	if (version != 1) {
		_gnutls_debug_log
		    ("ECC private key version %u is not supported\n",
		     version);
		gnutls_assert();
		ret = GNUTLS_E_ECC_UNSUPPORTED_CURVE;
		goto error;
	}

	/* read the curve */
	if (curve == GNUTLS_ECC_CURVE_INVALID) {
		oid_size = sizeof(oid);
		ret =
		    asn1_read_value(*pkey_asn, "parameters.namedCurve", oid,
			    &oid_size);
		if (ret != ASN1_SUCCESS) {
			gnutls_assert();
			ret = _gnutls_asn2err(ret);
			goto error;
		}

		pkey->params.curve = gnutls_oid_to_ecc_curve(oid);

		if (pkey->params.curve == GNUTLS_ECC_CURVE_INVALID) {
			_gnutls_debug_log("Curve %s is not supported\n", oid);
			gnutls_assert();
			ret = GNUTLS_E_ECC_UNSUPPORTED_CURVE;
			goto error;
		}
	} else {
		pkey->params.curve = curve;
	}


	/* read the public key */
	ret = _gnutls_x509_read_value(*pkey_asn, "publicKey", &out);
	if (ret < 0) {
		gnutls_assert();
		goto error;
	}

	ret =
	    _gnutls_ecc_ansi_x962_import(out.data, out.size,
					 &pkey->params.params[ECC_X],
					 &pkey->params.params[ECC_Y]);

	_gnutls_free_datum(&out);
	if (ret < 0) {
		gnutls_assert();
		goto error;
	}
	pkey->params.params_nr += 2;

	/* read the private key */
	ret =
	    _gnutls_x509_read_key_int(*pkey_asn, "privateKey",
				  &pkey->params.params[ECC_K]);
	if (ret < 0) {
		gnutls_assert();
		goto error;
	}
	pkey->params.params_nr++;
	pkey->params.algo = GNUTLS_PK_EC;

	return 0;

      error:
	asn1_delete_structure2(pkey_asn, ASN1_DELETE_FLAG_ZEROIZE);
	gnutls_pk_params_clear(&pkey->params);
	gnutls_pk_params_release(&pkey->params);
	return ret;

}


static ASN1_TYPE
decode_dsa_key(const gnutls_datum_t * raw_key, gnutls_x509_privkey_t pkey)
{
	int result;
	ASN1_TYPE dsa_asn;
	gnutls_datum_t seed = {NULL,0};
	char oid[MAX_OID_SIZE];
	int oid_size;

	if ((result =
	     asn1_create_element(_gnutls_get_gnutls_asn(),
				 "GNUTLS.DSAPrivateKey",
				 &dsa_asn)) != ASN1_SUCCESS) {
		gnutls_assert();
		return NULL;
	}

	gnutls_pk_params_init(&pkey->params);


	result =
	    _asn1_strict_der_decode(&dsa_asn, raw_key->data, raw_key->size,
			      NULL);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		goto error;
	}

	if ((result =
	     _gnutls_x509_read_int(dsa_asn, "p",
				   &pkey->params.params[0])) < 0) {
		gnutls_assert();
		goto error;
	}
	pkey->params.params_nr++;

	if ((result =
	     _gnutls_x509_read_int(dsa_asn, "q",
				   &pkey->params.params[1])) < 0) {
		gnutls_assert();
		goto error;
	}
	pkey->params.params_nr++;

	if ((result =
	     _gnutls_x509_read_int(dsa_asn, "g",
				   &pkey->params.params[2])) < 0) {
		gnutls_assert();
		goto error;
	}
	pkey->params.params_nr++;

	if ((result =
	     _gnutls_x509_read_int(dsa_asn, "Y",
				   &pkey->params.params[3])) < 0) {
		gnutls_assert();
		goto error;
	}
	pkey->params.params_nr++;

	if ((result = _gnutls_x509_read_key_int(dsa_asn, "priv",
					    &pkey->params.params[4])) < 0)
	{
		gnutls_assert();
		goto error;
	}
	pkey->params.params_nr++;
	pkey->params.algo = GNUTLS_PK_DSA;

	oid_size = sizeof(oid);
	result = asn1_read_value(dsa_asn, "seed.algorithm", oid, &oid_size);
	if (result == ASN1_SUCCESS) {
		pkey->params.palgo = gnutls_oid_to_digest(oid);

		result = _gnutls_x509_read_value(dsa_asn, "seed.seed", &seed);
		if (result == ASN1_SUCCESS) {
			if (seed.size <= sizeof(pkey->params.seed)) {
				memcpy(pkey->params.seed, seed.data, seed.size);
				pkey->params.seed_size = seed.size;
			}
			gnutls_free(seed.data);
		}
	}

	return dsa_asn;

      error:
	asn1_delete_structure2(&dsa_asn, ASN1_DELETE_FLAG_ZEROIZE);
	gnutls_pk_params_clear(&pkey->params);
	gnutls_pk_params_release(&pkey->params);
	return NULL;

}


#define PEM_KEY_DSA "DSA PRIVATE KEY"
#define PEM_KEY_RSA "RSA PRIVATE KEY"
#define PEM_KEY_ECC "EC PRIVATE KEY"
#define PEM_KEY_PKCS8 "PRIVATE KEY"

#define MAX_PEM_HEADER_SIZE 25

#define IF_CHECK_FOR(pemstr, _algo, cptr, bptr, size, key) \
		if (left > sizeof(pemstr) && memcmp(cptr, pemstr, sizeof(pemstr)-1) == 0) { \
			result = _gnutls_fbase64_decode(pemstr, bptr, size, &_data); \
			if (result >= 0) \
				key->params.algo = _algo; \
		}

/**
 * gnutls_x509_privkey_import:
 * @key: The data to store the parsed key
 * @data: The DER or PEM encoded certificate.
 * @format: One of DER or PEM
 *
 * This function will convert the given DER or PEM encoded key to the
 * native #gnutls_x509_privkey_t format. The output will be stored in
 * @key .
 *
 * If the key is PEM encoded it should have a header that contains "PRIVATE
 * KEY". Note that this function falls back to PKCS #8 decoding without
 * password, if the default format fails to import.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_x509_privkey_import(gnutls_x509_privkey_t key,
			   const gnutls_datum_t * data,
			   gnutls_x509_crt_fmt_t format)
{
	int result = 0, need_free = 0;
	gnutls_datum_t _data;

	if (key == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	_data.data = data->data;
	_data.size = data->size;

	key->params.algo = GNUTLS_PK_UNKNOWN;

	/* If the Certificate is in PEM format then decode it
	 */
	if (format == GNUTLS_X509_FMT_PEM) {
		unsigned left;
		char *ptr;
		uint8_t *begin_ptr;

		ptr = memmem(data->data, data->size, "PRIVATE KEY-----", sizeof("PRIVATE KEY-----")-1);

		result = GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;

		if (ptr != NULL) {
			left = data->size - ((ptrdiff_t)ptr - (ptrdiff_t)data->data);

			if (data->size - left > MAX_PEM_HEADER_SIZE) {
				ptr -= MAX_PEM_HEADER_SIZE;
				left += MAX_PEM_HEADER_SIZE;
			} else {
				ptr = (char*)data->data;
				left = data->size;
			}

			ptr = memmem(ptr, left, "-----BEGIN ", sizeof("-----BEGIN ")-1);
			if (ptr != NULL) {
				begin_ptr = (uint8_t*)ptr;
				left = data->size - ((ptrdiff_t)begin_ptr - (ptrdiff_t)data->data);

				ptr += sizeof("-----BEGIN ")-1;

				IF_CHECK_FOR(PEM_KEY_RSA, GNUTLS_PK_RSA, ptr, begin_ptr, left, key)
				else IF_CHECK_FOR(PEM_KEY_ECC, GNUTLS_PK_EC, ptr, begin_ptr, left, key)
				else IF_CHECK_FOR(PEM_KEY_DSA, GNUTLS_PK_DSA, ptr, begin_ptr, left, key)

				if (key->params.algo == GNUTLS_PK_UNKNOWN && left >= sizeof(PEM_KEY_PKCS8)) {
					if (memcmp(ptr, PEM_KEY_PKCS8, sizeof(PEM_KEY_PKCS8)-1) == 0) {
						result =
							_gnutls_fbase64_decode(PEM_KEY_PKCS8,
								begin_ptr, left, &_data);
						if (result >= 0) {
							/* signal for PKCS #8 keys */
							key->params.algo = -1;
						}
					}
				}
			}

		}

		if (result < 0) {
			gnutls_assert();
			return result;
		}

		need_free = 1;
	}

	if (key->expanded) {
		_gnutls_x509_privkey_reinit(key);
	}
	key->expanded = 1;

	if (key->params.algo == (gnutls_pk_algorithm_t)-1) {
		result =
		    gnutls_x509_privkey_import_pkcs8(key, data, format,
						     NULL,
						     GNUTLS_PKCS_PLAIN);
		if (result < 0) {
			gnutls_assert();
			key->key = NULL;
			goto cleanup;
		} else {
			/* some keys under PKCS#8 don't set key->key */
			goto finish;
		}
	} else if (key->params.algo == GNUTLS_PK_RSA) {
		key->key =
		    _gnutls_privkey_decode_pkcs1_rsa_key(&_data, key);
		if (key->key == NULL)
			gnutls_assert();
	} else if (key->params.algo == GNUTLS_PK_DSA) {
		key->key = decode_dsa_key(&_data, key);
		if (key->key == NULL)
			gnutls_assert();
	} else if (key->params.algo == GNUTLS_PK_EC) {
		result = _gnutls_privkey_decode_ecc_key(&key->key, &_data, key, 0);
		if (result < 0) {
			gnutls_assert();
			key->key = NULL;
		}
	} else {
		/* Try decoding each of the keys, and accept the one that
		 * succeeds.
		 */
		key->params.algo = GNUTLS_PK_RSA;
		key->key =
		    _gnutls_privkey_decode_pkcs1_rsa_key(&_data, key);

		if (key->key == NULL) {
			key->params.algo = GNUTLS_PK_DSA;
			key->key = decode_dsa_key(&_data, key);
			if (key->key == NULL) {
				key->params.algo = GNUTLS_PK_EC;
				result =
				    _gnutls_privkey_decode_ecc_key(&key->key, &_data, key, 0);
				if (result < 0) {
					result =
					    gnutls_x509_privkey_import_pkcs8(key, data, format,
									     NULL,
									     GNUTLS_PKCS_PLAIN);
					if (result >= 0) {
						/* there are keys (ed25519) which leave key->key NULL */
						goto finish;
					}

					/* result < 0 */
					gnutls_assert();
					key->key = NULL;

					if (result == GNUTLS_E_PK_INVALID_PRIVKEY)
						goto cleanup;
				}
			}
		}
	}

	if (key->key == NULL) {
		gnutls_assert();
		result = GNUTLS_E_ASN1_DER_ERROR;
		goto cleanup;
	}

 finish:
	result =
	    _gnutls_pk_fixup(key->params.algo, GNUTLS_IMPORT, &key->params);
	if (result < 0) {
		gnutls_assert();
	}

 cleanup:
	if (need_free)
		_gnutls_free_datum(&_data);

	/* The key has now been decoded.
	 */

	return result;
}

static int import_pkcs12_privkey(gnutls_x509_privkey_t key,
				 const gnutls_datum_t * data,
				 gnutls_x509_crt_fmt_t format,
				 const char *password, unsigned int flags)
{
	int ret;
	gnutls_pkcs12_t p12;
	gnutls_x509_privkey_t newkey;

	ret = gnutls_pkcs12_init(&p12);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = gnutls_pkcs12_import(p12, data, format, flags);
	if (ret < 0) {
		gnutls_assert();
		goto fail;
	}

	ret =
	    gnutls_pkcs12_simple_parse(p12, password, &newkey, NULL, NULL,
				       NULL, NULL, NULL, 0);
	if (ret < 0) {
		gnutls_assert();
		goto fail;
	}

	ret = gnutls_x509_privkey_cpy(key, newkey);
	gnutls_x509_privkey_deinit(newkey);
	if (ret < 0) {
		gnutls_assert();
		goto fail;
	}

	ret = 0;
      fail:

	gnutls_pkcs12_deinit(p12);

	return ret;
}

/**
 * gnutls_x509_privkey_import2:
 * @key: The data to store the parsed key
 * @data: The DER or PEM encoded key.
 * @format: One of DER or PEM
 * @password: A password (optional)
 * @flags: an ORed sequence of gnutls_pkcs_encrypt_flags_t
 *
 * This function will import the given DER or PEM encoded key, to 
 * the native #gnutls_x509_privkey_t format, irrespective of the
 * input format. The input format is auto-detected.
 *
 * The supported formats are basic unencrypted key, PKCS8, PKCS12,
 * and the openssl format.
 *
 * If the provided key is encrypted but no password was given, then
 * %GNUTLS_E_DECRYPTION_FAILED is returned. Since GnuTLS 3.4.0 this
 * function will utilize the PIN callbacks if any.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_x509_privkey_import2(gnutls_x509_privkey_t key,
			    const gnutls_datum_t * data,
			    gnutls_x509_crt_fmt_t format,
			    const char *password, unsigned int flags)
{
	int ret = 0;
	int saved_ret = GNUTLS_E_PARSING_ERROR;
	char pin[GNUTLS_PKCS11_MAX_PIN_LEN];
	unsigned head_enc = 1;

	if (format == GNUTLS_X509_FMT_PEM) {
		size_t left;
		char *ptr;

		ptr = memmem(data->data, data->size, "PRIVATE KEY-----", sizeof("PRIVATE KEY-----")-1);

		if (ptr != NULL) {
			left = data->size - ((ptrdiff_t)ptr - (ptrdiff_t)data->data);

			if (data->size - left > 15) {
				ptr -= 15;
				left += 15;
			} else {
				ptr = (char*)data->data;
				left = data->size;
			}

			ptr = memmem(ptr, left, "-----BEGIN ", sizeof("-----BEGIN ")-1);
			if (ptr != NULL) {
				ptr += sizeof("-----BEGIN ")-1;
				left = data->size - ((ptrdiff_t)ptr - (ptrdiff_t)data->data);
			}

			if (ptr != NULL && left > sizeof(PEM_KEY_RSA)) {
				if (memcmp(ptr, PEM_KEY_RSA, sizeof(PEM_KEY_RSA)-1) == 0 ||
				    memcmp(ptr, PEM_KEY_ECC, sizeof(PEM_KEY_ECC)-1) == 0 ||
				    memcmp(ptr, PEM_KEY_DSA, sizeof(PEM_KEY_DSA)-1) == 0) {
					head_enc = 0;
				}
			}
		}
	}

	if (head_enc == 0 || (password == NULL && !(flags & GNUTLS_PKCS_NULL_PASSWORD))) {
		ret = gnutls_x509_privkey_import(key, data, format);
		if (ret >= 0)
			return ret;

		if (ret < 0) {
			gnutls_assert();
			saved_ret = ret;
			/* fall through to PKCS #8 decoding */
		}
	}

	if ((password != NULL || (flags & GNUTLS_PKCS_NULL_PASSWORD))
	    || ret < 0) {

		ret =
		    gnutls_x509_privkey_import_pkcs8(key, data, format,
						     password, flags);

		if (ret == GNUTLS_E_DECRYPTION_FAILED &&
		    password == NULL && (!(flags & GNUTLS_PKCS_PLAIN))) {
		    /* use the callback if any */
			ret = _gnutls_retrieve_pin(&key->pin, "key:", "", 0, pin, sizeof(pin));
			if (ret == 0) {
				password = pin;
			}

			ret =
			    gnutls_x509_privkey_import_pkcs8(key, data, format,
						     password, flags);
		}

		if (saved_ret == GNUTLS_E_PARSING_ERROR)
			saved_ret = ret;

		if (ret < 0) {
			if (ret == GNUTLS_E_DECRYPTION_FAILED)
				goto cleanup;
			ret =
			    import_pkcs12_privkey(key, data, format,
						  password, flags);
			if (ret < 0 && format == GNUTLS_X509_FMT_PEM) {
				if (ret == GNUTLS_E_DECRYPTION_FAILED)
					goto cleanup;

				ret =
				    gnutls_x509_privkey_import_openssl(key,
								       data,
								       password);
				if (ret < 0) {
					gnutls_assert();
					goto cleanup;
				}
			} else {
				gnutls_assert();
				goto cleanup;
			}
		}
	}

	ret = 0;

      cleanup:
	if (ret == GNUTLS_E_PARSING_ERROR)
		ret = saved_ret;

	return ret;
}


/**
 * gnutls_x509_privkey_import_rsa_raw:
 * @key: The data to store the parsed key
 * @m: holds the modulus
 * @e: holds the public exponent
 * @d: holds the private exponent
 * @p: holds the first prime (p)
 * @q: holds the second prime (q)
 * @u: holds the coefficient
 *
 * This function will convert the given RSA raw parameters to the
 * native #gnutls_x509_privkey_t format.  The output will be stored in
 * @key.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_x509_privkey_import_rsa_raw(gnutls_x509_privkey_t key,
				   const gnutls_datum_t * m,
				   const gnutls_datum_t * e,
				   const gnutls_datum_t * d,
				   const gnutls_datum_t * p,
				   const gnutls_datum_t * q,
				   const gnutls_datum_t * u)
{
	return gnutls_x509_privkey_import_rsa_raw2(key, m, e, d, p, q, u,
						   NULL, NULL);
}

/**
 * gnutls_x509_privkey_import_rsa_raw2:
 * @key: The data to store the parsed key
 * @m: holds the modulus
 * @e: holds the public exponent
 * @d: holds the private exponent
 * @p: holds the first prime (p)
 * @q: holds the second prime (q)
 * @u: holds the coefficient (optional)
 * @e1: holds e1 = d mod (p-1) (optional)
 * @e2: holds e2 = d mod (q-1) (optional)
 *
 * This function will convert the given RSA raw parameters to the
 * native #gnutls_x509_privkey_t format.  The output will be stored in
 * @key.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_x509_privkey_import_rsa_raw2(gnutls_x509_privkey_t key,
				    const gnutls_datum_t * m,
				    const gnutls_datum_t * e,
				    const gnutls_datum_t * d,
				    const gnutls_datum_t * p,
				    const gnutls_datum_t * q,
				    const gnutls_datum_t * u,
				    const gnutls_datum_t * e1,
				    const gnutls_datum_t * e2)
{
	int ret;
	size_t siz = 0;

	if (key == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	gnutls_pk_params_init(&key->params);

	siz = m->size;
	if (_gnutls_mpi_init_scan_nz(&key->params.params[0], m->data, siz)) {
		gnutls_assert();
		ret = GNUTLS_E_MPI_SCAN_FAILED;
		goto cleanup;
	}
	key->params.params_nr++;

	siz = e->size;
	if (_gnutls_mpi_init_scan_nz(&key->params.params[1], e->data, siz)) {
		gnutls_assert();
		ret = GNUTLS_E_MPI_SCAN_FAILED;
		goto cleanup;
	}
	key->params.params_nr++;

	siz = d->size;
	if (_gnutls_mpi_init_scan_nz(&key->params.params[2], d->data, siz)) {
		gnutls_assert();
		ret = GNUTLS_E_MPI_SCAN_FAILED;
		goto cleanup;
	}
	key->params.params_nr++;

	siz = p->size;
	if (_gnutls_mpi_init_scan_nz(&key->params.params[3], p->data, siz)) {
		gnutls_assert();
		ret = GNUTLS_E_MPI_SCAN_FAILED;
		goto cleanup;
	}
	key->params.params_nr++;

	siz = q->size;
	if (_gnutls_mpi_init_scan_nz(&key->params.params[4], q->data, siz)) {
		gnutls_assert();
		ret = GNUTLS_E_MPI_SCAN_FAILED;
		goto cleanup;
	}
	key->params.params_nr++;

	if (u) {
		siz = u->size;
		if (_gnutls_mpi_init_scan_nz(&key->params.params[RSA_COEF], u->data, siz)) {
			gnutls_assert();
			ret = GNUTLS_E_MPI_SCAN_FAILED;
			goto cleanup;
		}
		key->params.params_nr++;
	}

	if (e1 && e2) {
		siz = e1->size;
		if (_gnutls_mpi_init_scan_nz
		    (&key->params.params[RSA_E1], e1->data, siz)) {
			gnutls_assert();
			ret = GNUTLS_E_MPI_SCAN_FAILED;
			goto cleanup;
		}
		key->params.params_nr++;

		siz = e2->size;
		if (_gnutls_mpi_init_scan_nz
		    (&key->params.params[RSA_E2], e2->data, siz)) {
			gnutls_assert();
			ret = GNUTLS_E_MPI_SCAN_FAILED;
			goto cleanup;
		}
		key->params.params_nr++;
	}

	key->params.algo = GNUTLS_PK_RSA;

	ret = _gnutls_pk_fixup(GNUTLS_PK_RSA, GNUTLS_IMPORT, &key->params);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	key->params.params_nr = RSA_PRIVATE_PARAMS;
	key->params.algo = GNUTLS_PK_RSA;

	ret =
	    _gnutls_asn1_encode_privkey(&key->key,
					&key->params);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	return 0;

      cleanup:
	gnutls_pk_params_clear(&key->params);
	gnutls_pk_params_release(&key->params);
	return ret;

}

/**
 * gnutls_x509_privkey_import_dsa_raw:
 * @key: The data to store the parsed key
 * @p: holds the p
 * @q: holds the q
 * @g: holds the g
 * @y: holds the y
 * @x: holds the x
 *
 * This function will convert the given DSA raw parameters to the
 * native #gnutls_x509_privkey_t format.  The output will be stored
 * in @key.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_x509_privkey_import_dsa_raw(gnutls_x509_privkey_t key,
				   const gnutls_datum_t * p,
				   const gnutls_datum_t * q,
				   const gnutls_datum_t * g,
				   const gnutls_datum_t * y,
				   const gnutls_datum_t * x)
{
	int ret;
	size_t siz = 0;

	if (key == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	gnutls_pk_params_init(&key->params);

	siz = p->size;
	if (_gnutls_mpi_init_scan_nz(&key->params.params[0], p->data, siz)) {
		gnutls_assert();
		ret = GNUTLS_E_MPI_SCAN_FAILED;
		goto cleanup;
	}

	siz = q->size;
	if (_gnutls_mpi_init_scan_nz(&key->params.params[1], q->data, siz)) {
		gnutls_assert();
		ret = GNUTLS_E_MPI_SCAN_FAILED;
		goto cleanup;
	}

	siz = g->size;
	if (_gnutls_mpi_init_scan_nz(&key->params.params[2], g->data, siz)) {
		gnutls_assert();
		ret = GNUTLS_E_MPI_SCAN_FAILED;
		goto cleanup;
	}

	siz = y->size;
	if (_gnutls_mpi_init_scan_nz(&key->params.params[3], y->data, siz)) {
		gnutls_assert();
		ret = GNUTLS_E_MPI_SCAN_FAILED;
		goto cleanup;
	}

	siz = x->size;
	if (_gnutls_mpi_init_scan_nz(&key->params.params[4], x->data, siz)) {
		gnutls_assert();
		ret = GNUTLS_E_MPI_SCAN_FAILED;
		goto cleanup;
	}

	ret =
	    _gnutls_pk_fixup(GNUTLS_PK_DSA, GNUTLS_IMPORT, &key->params);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	key->params.algo = GNUTLS_PK_DSA;
	key->params.params_nr = DSA_PRIVATE_PARAMS;

	ret =
	    _gnutls_asn1_encode_privkey(&key->key,
					&key->params);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	return 0;

      cleanup:
	gnutls_pk_params_clear(&key->params);
	gnutls_pk_params_release(&key->params);
	return ret;

}

/**
 * gnutls_x509_privkey_import_ecc_raw:
 * @key: The data to store the parsed key
 * @curve: holds the curve
 * @x: holds the x-coordinate
 * @y: holds the y-coordinate
 * @k: holds the k
 *
 * This function will convert the given elliptic curve parameters to the
 * native #gnutls_x509_privkey_t format.  The output will be stored
 * in @key. For EdDSA keys, the @x and @k values must be in the
 * native to curve format.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.0
 **/
int
gnutls_x509_privkey_import_ecc_raw(gnutls_x509_privkey_t key,
				   gnutls_ecc_curve_t curve,
				   const gnutls_datum_t * x,
				   const gnutls_datum_t * y,
				   const gnutls_datum_t * k)
{
	int ret;

	if (key == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	gnutls_pk_params_init(&key->params);

	key->params.curve = curve;

	if (curve_is_eddsa(curve)) {
		unsigned size;
		key->params.algo = GNUTLS_PK_EDDSA_ED25519;

		size = gnutls_ecc_curve_get_size(curve);
		if (x->size != size || k->size != size) {
			ret = gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);
			goto cleanup;
		}

		ret = _gnutls_set_datum(&key->params.raw_pub, x->data, x->size);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		ret = _gnutls_set_datum(&key->params.raw_priv, k->data, k->size);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		return 0;
	}

	if (_gnutls_mpi_init_scan_nz
	    (&key->params.params[ECC_X], x->data, x->size)) {
		gnutls_assert();
		ret = GNUTLS_E_MPI_SCAN_FAILED;
		goto cleanup;
	}
	key->params.params_nr++;

	if (_gnutls_mpi_init_scan_nz
	    (&key->params.params[ECC_Y], y->data, y->size)) {
		gnutls_assert();
		ret = GNUTLS_E_MPI_SCAN_FAILED;
		goto cleanup;
	}
	key->params.params_nr++;

	if (_gnutls_mpi_init_scan_nz
	    (&key->params.params[ECC_K], k->data, k->size)) {
		gnutls_assert();
		ret = GNUTLS_E_MPI_SCAN_FAILED;
		goto cleanup;
	}
	key->params.params_nr++;

	key->params.algo = GNUTLS_PK_EC;

	ret =
	    _gnutls_pk_fixup(GNUTLS_PK_EC, GNUTLS_IMPORT, &key->params);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret =
	    _gnutls_asn1_encode_privkey(&key->key,
					&key->params);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	return 0;

      cleanup:
	gnutls_pk_params_clear(&key->params);
	gnutls_pk_params_release(&key->params);
	return ret;

}

/**
 * gnutls_x509_privkey_import_gost_raw:
 * @key: The data to store the parsed key
 * @curve: holds the curve
 * @digest: will hold the digest
 * @paramset: will hold the GOST parameter set ID
 * @x: holds the x-coordinate
 * @y: holds the y-coordinate
 * @k: holds the k (private key)
 *
 * This function will convert the given GOST private key's parameters to the
 * native #gnutls_x509_privkey_t format.  The output will be stored
 * in @key.  @digest should be one of GNUTLS_DIG_GOSR_94,
 * GNUTLS_DIG_STREEBOG_256 or GNUTLS_DIG_STREEBOG_512.  If @paramset is set to
 * GNUTLS_GOST_PARAMSET_UNKNOWN default one will be selected depending on
 * @digest.
 *
 * Note: parameters should be stored with least significant byte first. On
 * version 3.6.3 big-endian format was used incorrectly.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.6.3
 **/
int
gnutls_x509_privkey_import_gost_raw(gnutls_x509_privkey_t key,
				    gnutls_ecc_curve_t curve,
				    gnutls_digest_algorithm_t digest,
				    gnutls_gost_paramset_t paramset,
				    const gnutls_datum_t * x,
				    const gnutls_datum_t * y,
				    const gnutls_datum_t * k)
{
	int ret;

	if (key == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	key->params.curve = curve;
	key->params.algo = _gnutls_digest_gost(digest);

	if (paramset == GNUTLS_GOST_PARAMSET_UNKNOWN)
		paramset = _gnutls_gost_paramset_default(key->params.algo);

	key->params.gost_params = paramset;

	if (_gnutls_mpi_init_scan_le
	    (&key->params.params[GOST_X], x->data, x->size)) {
		gnutls_assert();
		ret = GNUTLS_E_MPI_SCAN_FAILED;
		goto cleanup;
	}
	key->params.params_nr++;

	if (_gnutls_mpi_init_scan_le
	    (&key->params.params[GOST_Y], y->data, y->size)) {
		gnutls_assert();
		ret = GNUTLS_E_MPI_SCAN_FAILED;
		goto cleanup;
	}
	key->params.params_nr++;

	if (_gnutls_mpi_init_scan_le
	    (&key->params.params[GOST_K], k->data, k->size)) {
		gnutls_assert();
		ret = GNUTLS_E_MPI_SCAN_FAILED;
		goto cleanup;
	}
	key->params.params_nr++;

	ret =
	    _gnutls_pk_fixup(key->params.algo, GNUTLS_IMPORT, &key->params);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	return 0;

      cleanup:
	gnutls_pk_params_clear(&key->params);
	gnutls_pk_params_release(&key->params);
	return ret;

}


/**
 * gnutls_x509_privkey_get_pk_algorithm:
 * @key: should contain a #gnutls_x509_privkey_t type
 *
 * This function will return the public key algorithm of a private
 * key.
 *
 * Returns: a member of the #gnutls_pk_algorithm_t enumeration on
 *   success, or a negative error code on error.
 **/
int gnutls_x509_privkey_get_pk_algorithm(gnutls_x509_privkey_t key)
{
	if (key == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	return key->params.algo;
}

/**
 * gnutls_x509_privkey_get_pk_algorithm2:
 * @key: should contain a #gnutls_x509_privkey_t type
 * @bits: The number of bits in the public key algorithm
 *
 * This function will return the public key algorithm of a private
 * key.
 *
 * Returns: a member of the #gnutls_pk_algorithm_t enumeration on
 *   success, or a negative error code on error.
 **/
int
gnutls_x509_privkey_get_pk_algorithm2(gnutls_x509_privkey_t key,
				      unsigned int *bits)
{
	int ret;

	if (key == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	if (bits) {
		ret = pubkey_to_bits(&key->params);
		if (ret < 0)
			ret = 0;
		*bits = ret;
	}

	return key->params.algo;
}

void
_gnutls_x509_privkey_get_spki_params(gnutls_x509_privkey_t key,
				     gnutls_x509_spki_st *params)
{
	memcpy(params, &key->params.spki, sizeof(gnutls_x509_spki_st));
}

/**
 * gnutls_x509_privkey_get_spki:
 * @key: should contain a #gnutls_x509_privkey_t type
 * @spki: a SubjectPublicKeyInfo structure of type #gnutls_x509_spki_t
 * @flags: must be zero
 *
 * This function will return the public key information of a private
 * key. The provided @spki must be initialized.
 *
 * Returns: Zero on success, or a negative error code on error.
 **/
int
gnutls_x509_privkey_get_spki(gnutls_x509_privkey_t key, gnutls_x509_spki_t spki, unsigned int flags)
{
	if (key == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	if (key->params.spki.pk == GNUTLS_PK_UNKNOWN)
		return gnutls_assert_val(GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE);

	_gnutls_x509_privkey_get_spki_params(key, spki);

	return 0;
}

/**
 * gnutls_x509_privkey_set_spki:
 * @key: should contain a #gnutls_x509_privkey_t type
 * @spki: a SubjectPublicKeyInfo structure of type #gnutls_x509_spki_t
 * @flags: must be zero
 *
 * This function will return the public key information of a private
 * key. The provided @spki must be initialized.
 *
 * Returns: Zero on success, or a negative error code on error.
 **/
int
gnutls_x509_privkey_set_spki(gnutls_x509_privkey_t key, const gnutls_x509_spki_t spki, unsigned int flags)
{
	gnutls_pk_params_st tparams;
	int ret;

	if (key == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	if (!_gnutls_pk_are_compat(key->params.algo, spki->pk))
                return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	memcpy(&tparams, &key->params, sizeof(gnutls_pk_params_st));
	memcpy(&tparams.spki, spki, sizeof (gnutls_x509_spki_st));
	ret = _gnutls_x509_check_pubkey_params(&tparams);
	if (ret < 0)
		return gnutls_assert_val(ret);

	memcpy(&key->params.spki, spki, sizeof (gnutls_x509_spki_st));

	key->params.algo = spki->pk;

	return 0;
}

static const char *set_msg(gnutls_x509_privkey_t key)
{
	if (GNUTLS_PK_IS_RSA(key->params.algo)) {
		return PEM_KEY_RSA;
	} else if (key->params.algo == GNUTLS_PK_DSA) {
		return PEM_KEY_DSA;
	} else if (key->params.algo == GNUTLS_PK_EC)
		return PEM_KEY_ECC;
	else
		return "UNKNOWN";
}

/**
 * gnutls_x509_privkey_export:
 * @key: Holds the key
 * @format: the format of output params. One of PEM or DER.
 * @output_data: will contain a private key PEM or DER encoded
 * @output_data_size: holds the size of output_data (and will be
 *   replaced by the actual size of parameters)
 *
 * This function will export the private key to a PKCS#1 structure for
 * RSA or RSA-PSS keys, and integer sequence for DSA keys. Other keys types
 * will be exported in PKCS#8 form.
 *
 * If the structure is PEM encoded, it will have a header
 * of "BEGIN RSA PRIVATE KEY".
 *
 * It is recommended to use gnutls_x509_privkey_export_pkcs8() instead
 * of this function, when a consistent output format is required.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_x509_privkey_export(gnutls_x509_privkey_t key,
			   gnutls_x509_crt_fmt_t format, void *output_data,
			   size_t * output_data_size)
{
	gnutls_datum_t out;
	int ret;

	ret = gnutls_x509_privkey_export2(key, format, &out);
	if (ret < 0)
		return gnutls_assert_val(ret);

	if (format == GNUTLS_X509_FMT_PEM)
		ret = _gnutls_copy_string(&out, output_data, output_data_size);
	else
		ret = _gnutls_copy_data(&out, output_data, output_data_size);
	gnutls_free(out.data);

	return ret;
}

/**
 * gnutls_x509_privkey_export2:
 * @key: Holds the key
 * @format: the format of output params. One of PEM or DER.
 * @out: will contain a private key PEM or DER encoded
 *
 * This function will export the private key to a PKCS#1 structure for
 * RSA or RSA-PSS keys, and integer sequence for DSA keys. Other keys types
 * will be exported in PKCS#8 form.
 *
 * The output buffer is allocated using gnutls_malloc().
 *
 * It is recommended to use gnutls_x509_privkey_export2_pkcs8() instead
 * of this function, when a consistent output format is required.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since 3.1.3
 **/
int
gnutls_x509_privkey_export2(gnutls_x509_privkey_t key,
			    gnutls_x509_crt_fmt_t format,
			    gnutls_datum_t * out)
{
	const char *msg;
	int ret;

	if (key == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	if (key->key == NULL) { /* can only export in PKCS#8 form */
		return gnutls_x509_privkey_export2_pkcs8(key, format, NULL, 0, out);
	}

	msg = set_msg(key);

	if (key->flags & GNUTLS_PRIVKEY_FLAG_EXPORT_COMPAT) {
		ret = gnutls_x509_privkey_fix(key);
		if (ret < 0)
			return gnutls_assert_val(ret);
	}

	return _gnutls_x509_export_int2(key->key, format, msg, out);
}

/**
 * gnutls_x509_privkey_sec_param:
 * @key: a key
 *
 * This function will return the security parameter appropriate with
 * this private key.
 *
 * Returns: On success, a valid security parameter is returned otherwise
 * %GNUTLS_SEC_PARAM_UNKNOWN is returned.
 *
 * Since: 2.12.0
 **/
gnutls_sec_param_t gnutls_x509_privkey_sec_param(gnutls_x509_privkey_t key)
{
	int bits;

	bits = pubkey_to_bits(&key->params);
	if (bits <= 0)
		return GNUTLS_SEC_PARAM_UNKNOWN;

	return gnutls_pk_bits_to_sec_param(key->params.algo, bits);
}

/**
 * gnutls_x509_privkey_export_ecc_raw:
 * @key: a key
 * @curve: will hold the curve
 * @x: will hold the x-coordinate
 * @y: will hold the y-coordinate
 * @k: will hold the private key
 *
 * This function will export the ECC private key's parameters found
 * in the given structure. The new parameters will be allocated using
 * gnutls_malloc() and will be stored in the appropriate datum.
 *
 * In EdDSA curves the @y parameter will be %NULL and the other parameters
 * will be in the native format for the curve.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.0
 **/
int gnutls_x509_privkey_export_ecc_raw(gnutls_x509_privkey_t key,
				       gnutls_ecc_curve_t *curve,
				       gnutls_datum_t *x,
				       gnutls_datum_t *y,
				       gnutls_datum_t *k)
{
	if (key == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	return _gnutls_params_get_ecc_raw(&key->params, curve, x, y, k, 0);
}

/**
 * gnutls_x509_privkey_export_gost_raw:
 * @key: a key
 * @curve: will hold the curve
 * @digest: will hold the digest
 * @paramset: will hold the GOST parameter set ID
 * @x: will hold the x-coordinate
 * @y: will hold the y-coordinate
 * @k: will hold the private key
 *
 * This function will export the GOST private key's parameters found
 * in the given structure. The new parameters will be allocated using
 * gnutls_malloc() and will be stored in the appropriate datum.
 *
 * Note: parameters will be stored with least significant byte first. On
 * version 3.6.3 this was incorrectly returned in big-endian format.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.6.3
 **/
int gnutls_x509_privkey_export_gost_raw(gnutls_x509_privkey_t key,
					gnutls_ecc_curve_t * curve,
					gnutls_digest_algorithm_t * digest,
					gnutls_gost_paramset_t * paramset,
					gnutls_datum_t * x,
					gnutls_datum_t * y,
					gnutls_datum_t * k)
{
	if (key == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	return _gnutls_params_get_gost_raw(&key->params, curve, digest, paramset,
					   x, y, k, 0);
}

/**
 * gnutls_x509_privkey_export_rsa_raw:
 * @key: a key
 * @m: will hold the modulus
 * @e: will hold the public exponent
 * @d: will hold the private exponent
 * @p: will hold the first prime (p)
 * @q: will hold the second prime (q)
 * @u: will hold the coefficient
 *
 * This function will export the RSA private key's parameters found
 * in the given structure. The new parameters will be allocated using
 * gnutls_malloc() and will be stored in the appropriate datum.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_x509_privkey_export_rsa_raw(gnutls_x509_privkey_t key,
				   gnutls_datum_t * m, gnutls_datum_t * e,
				   gnutls_datum_t * d, gnutls_datum_t * p,
				   gnutls_datum_t * q, gnutls_datum_t * u)
{
	return _gnutls_params_get_rsa_raw(&key->params, m, e, d, p, q, u, NULL, NULL, 0);
}

/**
 * gnutls_x509_privkey_export_rsa_raw2:
 * @key: a key
 * @m: will hold the modulus
 * @e: will hold the public exponent
 * @d: will hold the private exponent
 * @p: will hold the first prime (p)
 * @q: will hold the second prime (q)
 * @u: will hold the coefficient
 * @e1: will hold e1 = d mod (p-1)
 * @e2: will hold e2 = d mod (q-1)
 *
 * This function will export the RSA private key's parameters found
 * in the given structure. The new parameters will be allocated using
 * gnutls_malloc() and will be stored in the appropriate datum.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 2.12.0
 **/
int
gnutls_x509_privkey_export_rsa_raw2(gnutls_x509_privkey_t key,
				    gnutls_datum_t * m, gnutls_datum_t * e,
				    gnutls_datum_t * d, gnutls_datum_t * p,
				    gnutls_datum_t * q, gnutls_datum_t * u,
				    gnutls_datum_t * e1,
				    gnutls_datum_t * e2)
{
	return _gnutls_params_get_rsa_raw(&key->params, m, e, d, p, q, u, e1, e2, 0);
}

/**
 * gnutls_x509_privkey_export_dsa_raw:
 * @key: a key
 * @p: will hold the p
 * @q: will hold the q
 * @g: will hold the g
 * @y: will hold the y
 * @x: will hold the x
 *
 * This function will export the DSA private key's parameters found
 * in the given structure. The new parameters will be allocated using
 * gnutls_malloc() and will be stored in the appropriate datum.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_x509_privkey_export_dsa_raw(gnutls_x509_privkey_t key,
				   gnutls_datum_t * p, gnutls_datum_t * q,
				   gnutls_datum_t * g, gnutls_datum_t * y,
				   gnutls_datum_t * x)
{
	return _gnutls_params_get_dsa_raw(&key->params, p, q, g, y, x, 0);
}

/**
 * gnutls_x509_privkey_generate:
 * @key: an initialized key
 * @algo: is one of the algorithms in #gnutls_pk_algorithm_t.
 * @bits: the size of the parameters to generate
 * @flags: Must be zero or flags from #gnutls_privkey_flags_t.
 *
 * This function will generate a random private key. Note that this
 * function must be called on an initialized private key.
 *
 * The flag %GNUTLS_PRIVKEY_FLAG_PROVABLE
 * instructs the key generation process to use algorithms like Shawe-Taylor
 * (from FIPS PUB186-4) which generate provable parameters out of a seed
 * for RSA and DSA keys. See gnutls_x509_privkey_generate2() for more
 * information.
 *
 * Note that when generating an elliptic curve key, the curve
 * can be substituted in the place of the bits parameter using the
 * GNUTLS_CURVE_TO_BITS() macro. The input to the macro is any curve from
 * %gnutls_ecc_curve_t.
 *
 * For DSA keys, if the subgroup size needs to be specified check
 * the GNUTLS_SUBGROUP_TO_BITS() macro.
 *
 * It is recommended to do not set the number of @bits directly, use gnutls_sec_param_to_pk_bits() instead .
 *
 * See also gnutls_privkey_generate(), gnutls_x509_privkey_generate2().
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_x509_privkey_generate(gnutls_x509_privkey_t key,
			     gnutls_pk_algorithm_t algo, unsigned int bits,
			     unsigned int flags)
{
	return gnutls_x509_privkey_generate2(key, algo, bits, flags, NULL, 0);   
}

/**
 * gnutls_x509_privkey_generate2:
 * @key: a key
 * @algo: is one of the algorithms in #gnutls_pk_algorithm_t.
 * @bits: the size of the modulus
 * @flags: Must be zero or flags from #gnutls_privkey_flags_t.
 * @data: Allow specifying %gnutls_keygen_data_st types such as the seed to be used.
 * @data_size: The number of @data available.
 *
 * This function will generate a random private key. Note that this
 * function must be called on an initialized private key.
 *
 * The flag %GNUTLS_PRIVKEY_FLAG_PROVABLE
 * instructs the key generation process to use algorithms like Shawe-Taylor
 * (from FIPS PUB186-4) which generate provable parameters out of a seed
 * for RSA and DSA keys. On DSA keys the PQG parameters are generated using the
 * seed, while on RSA the two primes. To specify an explicit seed
 * (by default a random seed is used), use the @data with a %GNUTLS_KEYGEN_SEED
 * type.
 *
 * Note that when generating an elliptic curve key, the curve
 * can be substituted in the place of the bits parameter using the
 * GNUTLS_CURVE_TO_BITS() macro.
 *
 * To export the generated keys in memory or in files it is recommended to use the
 * PKCS#8 form as it can handle all key types, and can store additional parameters
 * such as the seed, in case of provable RSA or DSA keys.
 * Generated keys can be exported in memory using gnutls_privkey_export_x509(),
 * and then with gnutls_x509_privkey_export2_pkcs8().
 *
 * If key generation is part of your application, avoid setting the number
 * of bits directly, and instead use gnutls_sec_param_to_pk_bits().
 * That way the generated keys will adapt to the security levels
 * of the underlying GnuTLS library.
 *
 * See also gnutls_privkey_generate2().
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_x509_privkey_generate2(gnutls_x509_privkey_t key,
			      gnutls_pk_algorithm_t algo, unsigned int bits,
			      unsigned int flags, const gnutls_keygen_data_st *data, unsigned data_size)
{
	int ret;
	unsigned i;
	gnutls_x509_spki_t tpki = NULL;

	if (key == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	gnutls_pk_params_init(&key->params);

	for (i=0;i<data_size;i++) {
		if (data[i].type == GNUTLS_KEYGEN_SEED && data[i].size < sizeof(key->params.seed)) {
			key->params.seed_size = data[i].size;
			memcpy(key->params.seed, data[i].data, data[i].size);
		} else if (data[i].type == GNUTLS_KEYGEN_DIGEST) {
			key->params.palgo = data[i].size;
		} else if (data[i].type == GNUTLS_KEYGEN_SPKI) {
			tpki = (void*)data[i].data;
		}
	}

	if (IS_EC(algo)) {
		if (GNUTLS_BITS_ARE_CURVE(bits))
			bits = GNUTLS_BITS_TO_CURVE(bits);
		else
			bits = _gnutls_ecc_bits_to_curve(algo, bits);

		if (gnutls_ecc_curve_get_pk(bits) != algo) {
			_gnutls_debug_log("curve is incompatible with public key algorithm\n");
			return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);
		}
	}

	if (IS_GOSTEC(algo)) {
		int size;

		if (GNUTLS_BITS_ARE_CURVE(bits))
			bits = GNUTLS_BITS_TO_CURVE(bits);
		else
			bits = _gnutls_ecc_bits_to_curve(algo, bits);

		size = gnutls_ecc_curve_get_size(bits);

		if ((algo == GNUTLS_PK_GOST_01 && size != 32) ||
		    (algo == GNUTLS_PK_GOST_12_256 && size != 32) ||
		    (algo == GNUTLS_PK_GOST_12_512 && size != 64)) {
			_gnutls_debug_log("curve is incompatible with public key algorithm\n");
			return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);
		}

		key->params.gost_params = _gnutls_gost_paramset_default(algo);
	}

	if (flags & GNUTLS_PRIVKEY_FLAG_PROVABLE) {
		key->params.pkflags |= GNUTLS_PK_FLAG_PROVABLE;
	}

	key->params.algo = algo;

	ret = _gnutls_pk_generate_params(algo, bits, &key->params);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	if (algo == GNUTLS_PK_RSA_PSS && (flags & GNUTLS_PRIVKEY_FLAG_CA) &&
	    !key->params.spki.pk) {
		const mac_entry_st *me;
		key->params.spki.pk = GNUTLS_PK_RSA_PSS;

		key->params.spki.rsa_pss_dig = _gnutls_pk_bits_to_sha_hash(bits);

		me = hash_to_entry(key->params.spki.rsa_pss_dig);
		if (unlikely(me == NULL)) {
			gnutls_assert();
			ret = GNUTLS_E_INVALID_REQUEST;
			goto cleanup;
		}

		ret = _gnutls_find_rsa_pss_salt_size(bits, me, 0);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		key->params.spki.salt_size = ret;
	}

	ret = _gnutls_pk_generate_keys(algo, bits, &key->params, 0);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = _gnutls_pk_verify_priv_params(algo, &key->params);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	if (tpki) {
		ret = gnutls_x509_privkey_set_spki(key, tpki, 0);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}
	}

	ret = _gnutls_asn1_encode_privkey(&key->key, &key->params);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	return 0;

      cleanup:
	key->params.algo = GNUTLS_PK_UNKNOWN;
	gnutls_pk_params_clear(&key->params);
	gnutls_pk_params_release(&key->params);

	return ret;
}

/**
 * gnutls_x509_privkey_get_seed:
 * @key: should contain a #gnutls_x509_privkey_t type
 * @digest: if non-NULL it will contain the digest algorithm used for key generation (if applicable)
 * @seed: where seed will be copied to
 * @seed_size: originally holds the size of @seed, will be updated with actual size
 *
 * This function will return the seed that was used to generate the
 * given private key. That function will succeed only if the key was generated
 * as a provable key.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.5.0
 **/
int gnutls_x509_privkey_get_seed(gnutls_x509_privkey_t key, gnutls_digest_algorithm_t *digest, void *seed, size_t *seed_size)
{
	if (key->params.seed_size == 0)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	if (seed_size == NULL || seed == NULL) {
		if (key->params.seed_size)
			return gnutls_assert_val(GNUTLS_E_SHORT_MEMORY_BUFFER);
		else
			return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);
	}

	if (*seed_size < key->params.seed_size) {
		*seed_size = key->params.seed_size;
		return gnutls_assert_val(GNUTLS_E_SHORT_MEMORY_BUFFER);
	}

	if (digest)
		*digest = key->params.palgo;

	memcpy(seed, key->params.seed, key->params.seed_size);
	*seed_size = key->params.seed_size; 
	return 0;
}

static
int cmp_rsa_key(gnutls_x509_privkey_t key1, gnutls_x509_privkey_t key2)
{
	gnutls_datum_t m1 = {NULL, 0}, e1 = {NULL, 0}, d1 = {NULL, 0}, p1 = {NULL, 0}, q1 = {NULL, 0};
	gnutls_datum_t m2 = {NULL, 0}, e2 = {NULL, 0}, d2 = {NULL, 0}, p2 = {NULL, 0}, q2 = {NULL, 0};
	int ret;

	ret = gnutls_x509_privkey_export_rsa_raw(key1, &m1, &e1, &d1, &p1, &q1, NULL);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	ret = gnutls_x509_privkey_export_rsa_raw(key2, &m2, &e2, &d2, &p2, &q2, NULL);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	if (m1.size != m2.size || memcmp(m1.data, m2.data, m1.size) != 0) {
		gnutls_assert();
		ret = GNUTLS_E_PRIVKEY_VERIFICATION_ERROR;
		goto cleanup;
	}

	if (d1.size != d2.size || memcmp(d1.data, d2.data, d1.size) != 0) {
		gnutls_assert();
		ret = GNUTLS_E_PRIVKEY_VERIFICATION_ERROR;
		goto cleanup;
	}

	if (e1.size != e2.size || memcmp(e1.data, e2.data, e1.size) != 0) {
		gnutls_assert();
		ret = GNUTLS_E_PRIVKEY_VERIFICATION_ERROR;
		goto cleanup;
	}

	if (p1.size != p2.size || memcmp(p1.data, p2.data, p1.size) != 0) {
		gnutls_assert();
		ret = GNUTLS_E_PRIVKEY_VERIFICATION_ERROR;
		goto cleanup;
	}

	if (q1.size != q2.size || memcmp(q1.data, q2.data, q1.size) != 0) {
		gnutls_assert();
		ret = GNUTLS_E_PRIVKEY_VERIFICATION_ERROR;
		goto cleanup;
	}

	ret = 0;
 cleanup:
	gnutls_free(m1.data);
	gnutls_free(e1.data);
	gnutls_free(d1.data);
	gnutls_free(p1.data);
	gnutls_free(q1.data);
	gnutls_free(m2.data);
	gnutls_free(e2.data);
	gnutls_free(d2.data);
	gnutls_free(p2.data);
	gnutls_free(q2.data);
	return ret;
}

static
int cmp_dsa_key(gnutls_x509_privkey_t key1, gnutls_x509_privkey_t key2)
{
	gnutls_datum_t p1 = {NULL, 0}, q1 = {NULL, 0}, g1 = {NULL, 0};
	gnutls_datum_t p2 = {NULL, 0}, q2 = {NULL, 0}, g2 = {NULL, 0};
	int ret;

	ret = gnutls_x509_privkey_export_dsa_raw(key1, &p1, &q1, &g1, NULL, NULL);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	ret = gnutls_x509_privkey_export_dsa_raw(key2, &p2, &q2, &g2, NULL, NULL);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	if (g1.size != g2.size || memcmp(g1.data, g2.data, g1.size) != 0) {
		gnutls_assert();
		ret = GNUTLS_E_PRIVKEY_VERIFICATION_ERROR;
		goto cleanup;
	}

	if (p1.size != p2.size || memcmp(p1.data, p2.data, p1.size) != 0) {
		gnutls_assert();
		ret = GNUTLS_E_PRIVKEY_VERIFICATION_ERROR;
		goto cleanup;
	}

	if (q1.size != q2.size || memcmp(q1.data, q2.data, q1.size) != 0) {
		gnutls_assert();
		ret = GNUTLS_E_PRIVKEY_VERIFICATION_ERROR;
		goto cleanup;
	}

	ret = 0;
 cleanup:
	gnutls_free(g1.data);
	gnutls_free(p1.data);
	gnutls_free(q1.data);
	gnutls_free(g2.data);
	gnutls_free(p2.data);
	gnutls_free(q2.data);
	return ret;
}

/**
 * gnutls_x509_privkey_verify_seed:
 * @key: should contain a #gnutls_x509_privkey_t type
 * @digest: it contains the digest algorithm used for key generation (if applicable)
 * @seed: the seed of the key to be checked with
 * @seed_size: holds the size of @seed
 *
 * This function will verify that the given private key was generated from
 * the provided seed. If @seed is %NULL then the seed stored in the @key's structure
 * will be used for verification.
 *
 * Returns: In case of a verification failure %GNUTLS_E_PRIVKEY_VERIFICATION_ERROR
 * is returned, and zero or positive code on success.
 *
 * Since: 3.5.0
 **/
int gnutls_x509_privkey_verify_seed(gnutls_x509_privkey_t key, gnutls_digest_algorithm_t digest, const void *seed, size_t seed_size)
{
	int ret;
	gnutls_x509_privkey_t okey;
	unsigned bits;
	gnutls_keygen_data_st data;

	if (key == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	if (key->params.algo != GNUTLS_PK_RSA && key->params.algo != GNUTLS_PK_DSA)
		return gnutls_assert_val(GNUTLS_E_UNIMPLEMENTED_FEATURE);

	ret = gnutls_x509_privkey_get_pk_algorithm2(key, &bits);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = gnutls_x509_privkey_init(&okey);
	if (ret < 0)
		return gnutls_assert_val(ret);

	if (seed == NULL) {
		seed = key->params.seed;
		seed_size = key->params.seed_size;
	}

	if (seed == NULL || seed_size == 0)
		return gnutls_assert_val(GNUTLS_E_PK_NO_VALIDATION_PARAMS);

	data.type = GNUTLS_KEYGEN_SEED;
	data.data = (void*)seed;
	data.size = seed_size;

	ret = gnutls_x509_privkey_generate2(okey, key->params.algo, bits,
					    GNUTLS_PRIVKEY_FLAG_PROVABLE, &data, 1);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	if (key->params.algo == GNUTLS_PK_RSA)
		ret = cmp_rsa_key(key, okey);
	else
		ret = cmp_dsa_key(key, okey);

      cleanup:
	gnutls_x509_privkey_deinit(okey);

	return ret;
}

/**
 * gnutls_x509_privkey_verify_params:
 * @key: a key
 *
 * This function will verify the private key parameters.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int gnutls_x509_privkey_verify_params(gnutls_x509_privkey_t key)
{
	int ret;

	ret = _gnutls_pk_verify_priv_params(key->params.algo, &key->params);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	return 0;
}

/**
 * gnutls_x509_privkey_get_key_id:
 * @key: a key
 * @flags: should be one of the flags from %gnutls_keyid_flags_t
 * @output_data: will contain the key ID
 * @output_data_size: holds the size of output_data (and will be
 *   replaced by the actual size of parameters)
 *
 * This function will return a unique ID that depends on the public key
 * parameters. This ID can be used in checking whether a certificate
 * corresponds to the given key.
 *
 * If the buffer provided is not long enough to hold the output, then
 * *@output_data_size is updated and %GNUTLS_E_SHORT_MEMORY_BUFFER will
 * be returned.  The output will normally be a SHA-1 hash output,
 * which is 20 bytes.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_x509_privkey_get_key_id(gnutls_x509_privkey_t key,
			       unsigned int flags,
			       unsigned char *output_data,
			       size_t * output_data_size)
{
	int ret;

	if (key == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	ret =
	    _gnutls_get_key_id(&key->params,
			       output_data, output_data_size, flags);
	if (ret < 0) {
		gnutls_assert();
	}

	return ret;
}


/**
 * gnutls_x509_privkey_sign_hash:
 * @key: a key
 * @hash: holds the data to be signed
 * @signature: will contain newly allocated signature
 *
 * This function will sign the given hash using the private key. Do not
 * use this function directly unless you know what it is. Typical signing
 * requires the data to be hashed and stored in special formats 
 * (e.g. BER Digest-Info for RSA).
 *
 * This API is provided only for backwards compatibility, and thus
 * restricted to RSA, DSA and ECDSA key types. For other key types please
 * use gnutls_privkey_sign_hash() and gnutls_privkey_sign_data().
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Deprecated in: 2.12.0
 */
int
gnutls_x509_privkey_sign_hash(gnutls_x509_privkey_t key,
			      const gnutls_datum_t * hash,
			      gnutls_datum_t * signature)
{
	int result;

	if (key == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	if (key->params.algo != GNUTLS_PK_RSA && key->params.algo != GNUTLS_PK_ECDSA &&
	    key->params.algo != GNUTLS_PK_DSA) {
		/* too primitive API - use only with legacy types */
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	result =
	    _gnutls_pk_sign(key->params.algo, signature, hash,
			    &key->params, &key->params.spki);

	if (result < 0) {
		gnutls_assert();
		return result;
	}

	return 0;
}

/**
 * gnutls_x509_privkey_sign_data:
 * @key: a key
 * @digest: should be a digest algorithm
 * @flags: should be 0 for now
 * @data: holds the data to be signed
 * @signature: will contain the signature
 * @signature_size: holds the size of signature (and will be replaced
 *   by the new size)
 *
 * This function will sign the given data using a signature algorithm
 * supported by the private key. Signature algorithms are always used
 * together with a hash functions.  Different hash functions may be
 * used for the RSA algorithm, but only SHA-1 for the DSA keys.
 *
 * If the buffer provided is not long enough to hold the output, then
 * *@signature_size is updated and %GNUTLS_E_SHORT_MEMORY_BUFFER will
 * be returned.
 *
 * Use gnutls_x509_crt_get_preferred_hash_algorithm() to determine
 * the hash algorithm.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 */
int
gnutls_x509_privkey_sign_data(gnutls_x509_privkey_t key,
			      gnutls_digest_algorithm_t digest,
			      unsigned int flags,
			      const gnutls_datum_t * data,
			      void *signature, size_t * signature_size)
{
	gnutls_privkey_t privkey;
	gnutls_datum_t sig = {NULL, 0};
	int ret;

	ret = gnutls_privkey_init(&privkey);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = gnutls_privkey_import_x509(privkey, key, 0);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = gnutls_privkey_sign_data(privkey, digest, flags, data, &sig);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	if (*signature_size < sig.size) {
		*signature_size = sig.size;
		ret = GNUTLS_E_SHORT_MEMORY_BUFFER;
		goto cleanup;
	}

	*signature_size = sig.size;
	memcpy(signature, sig.data, sig.size);

cleanup:
	_gnutls_free_datum(&sig);
	gnutls_privkey_deinit(privkey);
	return ret;
}

/**
 * gnutls_x509_privkey_fix:
 * @key: a key
 *
 * This function will recalculate the secondary parameters in a key.
 * In RSA keys, this can be the coefficient and exponent1,2.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int gnutls_x509_privkey_fix(gnutls_x509_privkey_t key)
{
	int ret;

	if (key == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	if (key->key) {
		asn1_delete_structure2(&key->key, ASN1_DELETE_FLAG_ZEROIZE);

		ret =
		    _gnutls_asn1_encode_privkey(&key->key,
						&key->params);
		if (ret < 0) {
			gnutls_assert();
			return ret;
		}
	}

	return 0;
}

/**
 * gnutls_x509_privkey_set_pin_function:
 * @privkey: The certificate structure
 * @fn: the callback
 * @userdata: data associated with the callback
 *
 * This function will set a callback function to be used when
 * it is required to access a protected object. This function overrides 
 * the global function set using gnutls_pkcs11_set_pin_function().
 *
 * Note that this callback is used when decrypting a key.
 *
 * Since: 3.4.0
 *
 **/
void gnutls_x509_privkey_set_pin_function(gnutls_x509_privkey_t privkey,
				      gnutls_pin_callback_t fn,
				      void *userdata)
{
	privkey->pin.cb = fn;
	privkey->pin.data = userdata;
}

/**
 * gnutls_x509_privkey_set_flags:
 * @key: A key of type #gnutls_x509_privkey_t
 * @flags: flags from the %gnutls_privkey_flags
 *
 * This function will set flags for the specified private key, after
 * it is generated. Currently this is useful for the %GNUTLS_PRIVKEY_FLAG_EXPORT_COMPAT
 * to allow exporting a "provable" private key in backwards compatible way.
 *
 * Since: 3.5.0
 *
 **/
void gnutls_x509_privkey_set_flags(gnutls_x509_privkey_t key,
				   unsigned int flags)
{
	key->flags |= flags;
}

