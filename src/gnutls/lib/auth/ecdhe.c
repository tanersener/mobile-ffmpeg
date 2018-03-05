/*
 * Copyright (C) 2011-2012 Free Software Foundation, Inc.
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

/* This file contains common stuff in Ephemeral Diffie-Hellman (DHE)
 * and Anonymous DH key exchange(DHA). These are used in the handshake
 * procedure of the certificate and anoymous authentication.
 */

#include "gnutls_int.h"
#include "auth.h"
#include "errors.h"
#include "dh.h"
#include "num.h"
#include "tls-sig.h"
#include <state.h>
#include <datum.h>
#include <x509.h>
#include <auth/ecdhe.h>
#include <ecc.h>
#include <ext/ecc.h>
#include <algorithms.h>
#include <auth/psk.h>
#include <auth/cert.h>
#include <pk.h>

static int gen_ecdhe_server_kx(gnutls_session_t, gnutls_buffer_st *);
static int
proc_ecdhe_server_kx(gnutls_session_t session,
		     uint8_t * data, size_t _data_size);
static int
proc_ecdhe_client_kx(gnutls_session_t session,
		     uint8_t * data, size_t _data_size);

#if defined(ENABLE_ECDHE)
const mod_auth_st ecdhe_ecdsa_auth_struct = {
	"ECDHE_ECDSA",
	_gnutls_gen_cert_server_crt,
	_gnutls_gen_cert_client_crt,
	gen_ecdhe_server_kx,
	_gnutls_gen_ecdh_common_client_kx,	/* This is the only difference */
	_gnutls_gen_cert_client_crt_vrfy,
	_gnutls_gen_cert_server_cert_req,

	_gnutls_proc_crt,
	_gnutls_proc_crt,
	proc_ecdhe_server_kx,
	proc_ecdhe_client_kx,
	_gnutls_proc_cert_client_crt_vrfy,
	_gnutls_proc_cert_cert_req
};

const mod_auth_st ecdhe_rsa_auth_struct = {
	"ECDHE_RSA",
	_gnutls_gen_cert_server_crt,
	_gnutls_gen_cert_client_crt,
	gen_ecdhe_server_kx,
	_gnutls_gen_ecdh_common_client_kx,	/* This is the only difference */
	_gnutls_gen_cert_client_crt_vrfy,
	_gnutls_gen_cert_server_cert_req,

	_gnutls_proc_crt,
	_gnutls_proc_crt,
	proc_ecdhe_server_kx,
	proc_ecdhe_client_kx,
	_gnutls_proc_cert_client_crt_vrfy,
	_gnutls_proc_cert_cert_req
};

static int calc_ecdh_key(gnutls_session_t session,
			 gnutls_datum_t * psk_key,
			 const gnutls_ecc_curve_entry_st *ecurve)
{
	gnutls_pk_params_st pub;
	int ret;
	gnutls_datum_t tmp_dh_key;

	gnutls_pk_params_init(&pub);
	pub.params[ECC_X] = session->key.ecdh_x;
	pub.params[ECC_Y] = session->key.ecdh_y;
	pub.raw_pub.data = session->key.ecdhx.data;
	pub.raw_pub.size = session->key.ecdhx.size;
	pub.flags = ecurve->id;

	ret =
	    _gnutls_pk_derive(ecurve->pk, &tmp_dh_key,
			      &session->key.ecdh_params, &pub);
	if (ret < 0) {
		ret = gnutls_assert_val(ret);
		goto cleanup;
	}

	if (psk_key == NULL) {
		memcpy(&session->key.key, &tmp_dh_key, sizeof(gnutls_datum_t));
		tmp_dh_key.data = NULL; /* no longer needed */
	} else {
		ret =
		    _gnutls_set_psk_session_key(session, psk_key,
						&tmp_dh_key);
		_gnutls_free_temp_key_datum(&tmp_dh_key);

		if (ret < 0) {
			ret = gnutls_assert_val(ret);
			goto cleanup;
		}
	}

	ret = 0;

      cleanup:
	/* no longer needed */
	_gnutls_mpi_release(&session->key.ecdh_x);
	_gnutls_mpi_release(&session->key.ecdh_y);
	_gnutls_free_datum(&session->key.ecdhx);
	gnutls_pk_params_release(&session->key.ecdh_params);
	return ret;
}

int _gnutls_proc_ecdh_common_client_kx(gnutls_session_t session,
				       uint8_t * data, size_t _data_size,
				       gnutls_ecc_curve_t curve,
				       gnutls_datum_t * psk_key)
{
	ssize_t data_size = _data_size;
	int ret, i = 0;
	int point_size;
	const gnutls_ecc_curve_entry_st *ecurve = _gnutls_ecc_curve_get_params(curve);

	if (curve == GNUTLS_ECC_CURVE_INVALID || ecurve == NULL)
		return gnutls_assert_val(GNUTLS_E_ECC_NO_SUPPORTED_CURVES);

	DECR_LEN(data_size, 1);
	point_size = data[i];
	i += 1;

	if (point_size == 0) {
		ret = gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET_LENGTH);
		goto cleanup;
	}

	DECR_LEN(data_size, point_size);

	if (ecurve->pk == GNUTLS_PK_EC) {
		ret =
		    _gnutls_ecc_ansi_x963_import(&data[i], point_size,
					 &session->key.ecdh_x,
					 &session->key.ecdh_y);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}
	} else if (ecurve->pk == GNUTLS_PK_ECDHX) {
		if (ecurve->size != point_size)
			return gnutls_assert_val(GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER);

		ret = _gnutls_set_datum(&session->key.ecdhx,
					&data[i], point_size);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		/* RFC7748 requires to mask the MSB in the final byte */
		if (ecurve->id == GNUTLS_ECC_CURVE_X25519) {
			session->key.ecdhx.data[point_size-1] &= 0x7f;
		}
	} else {
		return gnutls_assert_val(GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER);
	}

	if (data_size != 0)
		return gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET_LENGTH);

	/* generate pre-shared key */
	ret = calc_ecdh_key(session, psk_key, ecurve);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

 cleanup:
	gnutls_pk_params_clear(&session->key.ecdh_params);
	return ret;
}

static int
proc_ecdhe_client_kx(gnutls_session_t session,
		     uint8_t * data, size_t _data_size)
{
	gnutls_certificate_credentials_t cred;

	cred = (gnutls_certificate_credentials_t)
	    _gnutls_get_cred(session, GNUTLS_CRD_CERTIFICATE);
	if (cred == NULL) {
		gnutls_assert();
		return GNUTLS_E_INSUFFICIENT_CREDENTIALS;
	}

	return _gnutls_proc_ecdh_common_client_kx(session, data,
						  _data_size,
						  _gnutls_session_ecc_curve_get
						  (session), NULL);
}

int
_gnutls_gen_ecdh_common_client_kx(gnutls_session_t session,
				  gnutls_buffer_st * data)
{
	return _gnutls_gen_ecdh_common_client_kx_int(session, data, NULL);
}

int
_gnutls_gen_ecdh_common_client_kx_int(gnutls_session_t session,
				      gnutls_buffer_st * data,
				      gnutls_datum_t * psk_key)
{
	int ret;
	gnutls_datum_t out;
	int curve = _gnutls_session_ecc_curve_get(session);
	const gnutls_ecc_curve_entry_st *ecurve = _gnutls_ecc_curve_get_params(curve);
	int pk;

	if (ecurve == NULL)
		return gnutls_assert_val(GNUTLS_E_ECC_NO_SUPPORTED_CURVES);

	pk = ecurve->pk;

	/* generate temporal key */
	ret =
	    _gnutls_pk_generate_keys(pk, curve,
				     &session->key.ecdh_params);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER;
	if (pk == GNUTLS_PK_EC) {
		ret =
		    _gnutls_ecc_ansi_x963_export(curve,
						 session->key.ecdh_params.
						 params[ECC_X] /* x */ ,
						 session->key.ecdh_params.
						 params[ECC_Y] /* y */ , &out);

		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		ret =
		    _gnutls_buffer_append_data_prefix(data, 8, out.data, out.size);

		_gnutls_free_datum(&out);

		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}
	} else if (pk == GNUTLS_PK_ECDHX) {
		ret =
		    _gnutls_buffer_append_data_prefix(data, 8,
					session->key.ecdh_params.raw_pub.data,
					session->key.ecdh_params.raw_pub.size);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}
	}

	/* generate pre-shared key */
	ret = calc_ecdh_key(session, psk_key, ecurve);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = data->length;
 cleanup:
	gnutls_pk_params_clear(&session->key.ecdh_params);
	return ret;
}

static int
proc_ecdhe_server_kx(gnutls_session_t session,
		     uint8_t * data, size_t _data_size)
{
	int ret;
	gnutls_datum_t vparams;

	ret =
	    _gnutls_proc_ecdh_common_server_kx(session, data, _data_size);
	if (ret < 0)
		return gnutls_assert_val(ret);

	vparams.data = data;
	vparams.size = ret;

	return _gnutls_proc_dhe_signature(session, data + ret,
					  _data_size - ret, &vparams);
}

int
_gnutls_proc_ecdh_common_server_kx(gnutls_session_t session,
				   uint8_t * data, size_t _data_size)
{
	int i, ret, point_size;
	gnutls_ecc_curve_t curve;
	ssize_t data_size = _data_size;
	const gnutls_ecc_curve_entry_st *ecurve;

	/* just in case we are resuming a session */
	gnutls_pk_params_release(&session->key.ecdh_params);

	gnutls_pk_params_init(&session->key.ecdh_params);

	i = 0;
	DECR_LEN(data_size, 1);
	if (data[i++] != 3)
		return gnutls_assert_val(GNUTLS_E_ECC_NO_SUPPORTED_CURVES);

	DECR_LEN(data_size, 2);
	curve = _gnutls_tls_id_to_ecc_curve(_gnutls_read_uint16(&data[i]));

	if (curve == GNUTLS_ECC_CURVE_INVALID) {
		_gnutls_debug_log("received curve %u.%u\n", (unsigned)data[i], (unsigned)data[i+1]);
	} else {
		_gnutls_debug_log("received curve %s\n", gnutls_ecc_curve_get_name(curve));
	}

	i += 2;

	ret = _gnutls_session_supports_ecc_curve(session, curve);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ecurve = _gnutls_ecc_curve_get_params(curve);
	if (ecurve == NULL) {
		gnutls_assert();
		return gnutls_assert_val(GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER);
	}

	_gnutls_session_ecc_curve_set(session, curve);

	DECR_LEN(data_size, 1);
	point_size = data[i];
	i++;

	DECR_LEN(data_size, point_size);

	if (ecurve->pk == GNUTLS_PK_EC) {
		ret =
		    _gnutls_ecc_ansi_x963_import(&data[i], point_size,
						 &session->key.ecdh_x,
						 &session->key.ecdh_y);
		if (ret < 0)
			return gnutls_assert_val(ret);

	} else if (ecurve->pk == GNUTLS_PK_ECDHX) {
		if (ecurve->size != point_size)
			return gnutls_assert_val(GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER);

		ret = _gnutls_set_datum(&session->key.ecdhx,
					&data[i], point_size);
		if (ret < 0)
			return gnutls_assert_val(ret);

		/* RFC7748 requires to mask the MSB in the final byte */
		if (ecurve->id == GNUTLS_ECC_CURVE_X25519) {
			session->key.ecdhx.data[point_size-1] &= 0x7f;
		}
	} else {
		return gnutls_assert_val(GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER);
	}

	i += point_size;

	return i;
}

/* If the psk flag is set, then an empty psk_identity_hint will
 * be inserted */
int _gnutls_ecdh_common_print_server_kx(gnutls_session_t session,
					gnutls_buffer_st * data,
					gnutls_ecc_curve_t curve)
{
	uint8_t p;
	int ret, pk;
	gnutls_datum_t out;

	if (curve == GNUTLS_ECC_CURVE_INVALID)
		return gnutls_assert_val(GNUTLS_E_ECC_NO_SUPPORTED_CURVES);

	/* just in case we are resuming a session */
	gnutls_pk_params_release(&session->key.ecdh_params);

	gnutls_pk_params_init(&session->key.ecdh_params);

	/* curve type */
	p = 3;

	ret = _gnutls_buffer_append_data(data, &p, 1);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret =
	    _gnutls_buffer_append_prefix(data, 16,
					 _gnutls_ecc_curve_get_tls_id
					 (curve));
	if (ret < 0)
		return gnutls_assert_val(ret);

	pk = gnutls_ecc_curve_get_pk(curve);

	/* generate temporal key */
	ret =
	    _gnutls_pk_generate_keys(pk, curve,
				     &session->key.ecdh_params);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER;
	if (pk == GNUTLS_PK_EC) {
		ret =
		    _gnutls_ecc_ansi_x963_export(curve,
						 session->key.ecdh_params.
						 params[ECC_X] /* x */ ,
						 session->key.ecdh_params.
						 params[ECC_Y] /* y */ , &out);
		if (ret < 0)
			return gnutls_assert_val(ret);

		ret =
		    _gnutls_buffer_append_data_prefix(data, 8, out.data, out.size);

		_gnutls_free_datum(&out);

		if (ret < 0)
			return gnutls_assert_val(ret);

	} else if (pk == GNUTLS_PK_ECDHX) {
		ret =
			_gnutls_buffer_append_data_prefix(data, 8,
					session->key.ecdh_params.raw_pub.data,
					session->key.ecdh_params.raw_pub.size);
		if (ret < 0)
			return gnutls_assert_val(ret);
	}

	return data->length;
}

static int
gen_ecdhe_server_kx(gnutls_session_t session, gnutls_buffer_st * data)
{
	int ret = 0;
	gnutls_certificate_credentials_t cred;

	cred = (gnutls_certificate_credentials_t)
	    _gnutls_get_cred(session, GNUTLS_CRD_CERTIFICATE);
	if (cred == NULL) {
		gnutls_assert();
		return GNUTLS_E_INSUFFICIENT_CREDENTIALS;
	}

	if ((ret = _gnutls_auth_info_set(session, GNUTLS_CRD_CERTIFICATE,
					 sizeof(cert_auth_info_st),
					 1)) < 0) {
		gnutls_assert();
		return ret;
	}

	ret =
	    _gnutls_ecdh_common_print_server_kx(session, data,
						_gnutls_session_ecc_curve_get
						(session));
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	/* Generate the signature. */
	return _gnutls_gen_dhe_signature(session, data, data->data,
					 data->length);
}

#endif
