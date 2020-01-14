/*
 * Copyright (C) 2011-2012 Free Software Foundation, Inc.
 * Copyright (C) 2016 Dmitry Eremin-Solenikov
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
 */

/*
 * This is split from main TLS key exchange, because it might be useful in
 * future for S/MIME support. For the definition of the algorithm see RFC 4357,
 * section 5.2.
 */
#include "gnutls_int.h"
#include "vko.h"
#include "pk.h"
#include "common.h"

static int
_gnutls_gost_vko_key(gnutls_pk_params_st *pub,
		     gnutls_pk_params_st *priv,
		     gnutls_datum_t *ukm,
		     gnutls_digest_algorithm_t digalg,
		     gnutls_datum_t *kek)
{
	gnutls_datum_t tmp_vko_key;
	int ret;

	ret = _gnutls_pk_derive_nonce(pub->algo, &tmp_vko_key,
				      priv, pub, ukm);
	if (ret < 0)
		return gnutls_assert_val(ret);

	kek->size = gnutls_hash_get_len(digalg);
	kek->data = gnutls_malloc(kek->size);
	if (kek->data == NULL) {
		gnutls_assert();
		ret = GNUTLS_E_MEMORY_ERROR;
		goto cleanup;
	}

	ret = gnutls_hash_fast(digalg, tmp_vko_key.data, tmp_vko_key.size, kek->data);
	if (ret < 0) {
		gnutls_assert();
		_gnutls_free_datum(kek);
		goto cleanup;
	}

	ret = 0;

cleanup:
	_gnutls_free_temp_key_datum(&tmp_vko_key);

	return ret;
}

static const gnutls_datum_t zero_data = { NULL, 0 };

int
_gnutls_gost_keytrans_encrypt(gnutls_pk_params_st *pub,
			      gnutls_pk_params_st *priv,
			      gnutls_datum_t *cek,
			      gnutls_datum_t *ukm,
			      gnutls_datum_t *out)
{
	int ret;
	gnutls_datum_t kek;
	gnutls_datum_t enc, imit;
	gnutls_digest_algorithm_t digalg;
	ASN1_TYPE kx;

	if (pub->algo == GNUTLS_PK_GOST_01)
		digalg = GNUTLS_DIG_GOSTR_94;
	else
		digalg = GNUTLS_DIG_STREEBOG_256;

	ret = _gnutls_gost_vko_key(pub, priv, ukm, digalg, &kek);
	if (ret < 0) {
		gnutls_assert();

		return ret;
	}

	ret = _gnutls_gost_key_wrap(pub->gost_params, &kek, ukm, cek,
				    &enc, &imit);
	_gnutls_free_key_datum(&kek);
	if (ret < 0) {
		gnutls_assert();

		return ret;
	}

	ret = asn1_create_element(_gnutls_get_gnutls_asn(),
				  "GNUTLS.GostR3410-KeyTransport",
				  &kx);
	if (ret != ASN1_SUCCESS) {
		gnutls_assert();
		ret = _gnutls_asn2err(ret);
		_gnutls_free_datum(&enc);
		_gnutls_free_datum(&imit);

		return ret;
	}

	ret = _gnutls_x509_write_value(kx, "transportParameters.ukm", ukm);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = _gnutls_x509_encode_and_copy_PKI_params(kx,
			"transportParameters.ephemeralPublicKey",
			priv);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	if ((ret = asn1_write_value(kx, "transportParameters.encryptionParamSet",
				    gnutls_gost_paramset_get_oid(pub->gost_params),
				    1)) != ASN1_SUCCESS) {
		gnutls_assert();
		ret = _gnutls_asn2err(ret);
		goto cleanup;
	}

	ret = _gnutls_x509_write_value(kx, "sessionEncryptedKey.encryptedKey", &enc);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = _gnutls_x509_write_value(kx, "sessionEncryptedKey.maskKey", &zero_data);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}
	ret = _gnutls_x509_write_value(kx, "sessionEncryptedKey.macKey", &imit);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = _gnutls_x509_der_encode(kx, "", out, 0);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = 0;

cleanup:
	asn1_delete_structure(&kx);
	_gnutls_free_datum(&enc);
	_gnutls_free_datum(&imit);

	return ret;
}

int
_gnutls_gost_keytrans_decrypt(gnutls_pk_params_st *priv,
			      gnutls_datum_t *cek,
			      gnutls_datum_t *ukm,
			      gnutls_datum_t *out)
{
	int ret;
	ASN1_TYPE kx;
	gnutls_pk_params_st pub;
	gnutls_datum_t kek;
	gnutls_datum_t ukm2, enc, imit;
	char oid[MAX_OID_SIZE];
	int oid_size;
	gnutls_digest_algorithm_t digalg;

	if ((ret = asn1_create_element(_gnutls_get_gnutls_asn(),
				       "GNUTLS.GostR3410-KeyTransport",
				       &kx)) != ASN1_SUCCESS) {
		gnutls_assert();
		ret = _gnutls_asn2err(ret);

		return ret;
	}

	ret = _asn1_strict_der_decode(&kx, cek->data, cek->size, NULL);
	if (ret != ASN1_SUCCESS) {
		gnutls_assert();
		ret = _gnutls_asn2err(ret);
		asn1_delete_structure(&kx);

		return ret;
	}

	ret = _gnutls_get_asn_mpis(kx,
				   "transportParameters.ephemeralPublicKey",
				   &pub);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	if (pub.algo != priv->algo ||
	    pub.gost_params != priv->gost_params ||
	    pub.curve != priv->curve) {
		gnutls_assert();
		ret = GNUTLS_E_ILLEGAL_PARAMETER;
		goto cleanup;
	}

	oid_size = sizeof(oid);
	ret = asn1_read_value(kx, "transportParameters.encryptionParamSet", oid, &oid_size);
	if (ret != ASN1_SUCCESS) {
		gnutls_assert();
		ret = _gnutls_asn2err(ret);
		goto cleanup;
	}

	if (gnutls_oid_to_gost_paramset(oid) != priv->gost_params) {
		gnutls_assert();
		ret = GNUTLS_E_ASN1_DER_ERROR;
		goto cleanup;
	}

	ret = _gnutls_x509_read_value(kx, "transportParameters.ukm", &ukm2);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	/* Kind of strange design. For TLS UKM is calculated as a hash of
	 * client and server random. At the same time UKM is transmitted as a
	 * part of KeyTransport structure. At this point we have to compare
	 * them to check that they are equal. This does not result in an oracle
	 * of any kind as all values are transmitted in cleartext. Returning
	 * that this point won't give any information to the attacker.
	 */
	if (ukm2.size != ukm->size || memcmp(ukm2.data, ukm->data, ukm->size) != 0) {
		gnutls_assert();
		_gnutls_free_datum(&ukm2);
		ret = GNUTLS_E_DECRYPTION_FAILED;
		goto cleanup;
	}
	_gnutls_free_datum(&ukm2);

	ret = _gnutls_x509_read_value(kx, "sessionEncryptedKey.encryptedKey",
				      &enc);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = _gnutls_x509_read_value(kx, "sessionEncryptedKey.macKey",
				      &imit);
	if (ret < 0) {
		gnutls_assert();
		_gnutls_free_datum(&enc);
		goto cleanup;
	}

	if (pub.algo == GNUTLS_PK_GOST_01)
		digalg = GNUTLS_DIG_GOSTR_94;
	else
		digalg = GNUTLS_DIG_STREEBOG_256;

	ret = _gnutls_gost_vko_key(&pub, priv, ukm, digalg, &kek);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup2;
	}

	ret = _gnutls_gost_key_unwrap(pub.gost_params, &kek, ukm,
				      &enc, &imit, out);
	_gnutls_free_key_datum(&kek);

	if (ret < 0) {
		gnutls_assert();
		goto cleanup2;
	}

	ret = 0;

cleanup2:
	_gnutls_free_datum(&imit);
	_gnutls_free_datum(&enc);
cleanup:
	gnutls_pk_params_release(&pub);
	asn1_delete_structure(&kx);

	return ret;
}
