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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>
 *
 */

/* This file contains common stuff in Ephemeral Diffie-Hellman (DHE)
 * and Anonymous DH key exchange(DHA). These are used in the handshake
 * procedure of the certificate and anonymous authentication.
 */

#include "gnutls_int.h"
#include "auth.h"
#include "errors.h"
#include "dh.h"
#include "num.h"
#include "tls-sig.h"
#include <datum.h>
#include <x509.h>
#include <state.h>
#include <pk.h>
#include <auth/dh_common.h>
#include <algorithms.h>
#include <auth/psk.h>

#if defined(ENABLE_DHE) || defined(ENABLE_ANON)

/* Frees the dh_info_st structure.
 */
void _gnutls_free_dh_info(dh_info_st * dh)
{
	dh->secret_bits = 0;
	_gnutls_free_datum(&dh->prime);
	_gnutls_free_datum(&dh->generator);
	_gnutls_free_datum(&dh->public_key);
}

int
_gnutls_proc_dh_common_client_kx(gnutls_session_t session,
				 uint8_t * data, size_t _data_size,
				 gnutls_datum_t * psk_key)
{
	uint16_t n_Y;
	size_t _n_Y;
	int ret;
	ssize_t data_size = _data_size;
	gnutls_datum_t tmp_dh_key = {NULL, 0};
	gnutls_pk_params_st peer_pub;
	
	gnutls_pk_params_init(&peer_pub);

	DECR_LEN(data_size, 2);
	n_Y = _gnutls_read_uint16(&data[0]);
	_n_Y = n_Y;

	DECR_LEN(data_size, n_Y);

	if (data_size != 0)
		return gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET_LENGTH);

	if (_gnutls_mpi_init_scan_nz(&session->key.proto.tls12.dh.client_Y, &data[2], _n_Y)) {
		gnutls_assert();
		return GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER; /* most likely zero or illegal size */
	}

	_gnutls_dh_set_peer_public(session, session->key.proto.tls12.dh.client_Y);

	peer_pub.params[DH_Y] = session->key.proto.tls12.dh.client_Y;

	/* calculate the key after calculating the message */
	ret = _gnutls_pk_derive(GNUTLS_PK_DH, &tmp_dh_key, &session->key.proto.tls12.dh.params, &peer_pub);
	if (ret < 0) {
		gnutls_assert();
		goto error;
	}

	if (psk_key == NULL) {
		session->key.key.data = tmp_dh_key.data;
		session->key.key.size = tmp_dh_key.size;
	} else {		/* In DHE_PSK the key is set differently */
		ret =
		    _gnutls_set_psk_session_key(session, psk_key,
						&tmp_dh_key);
		_gnutls_free_temp_key_datum(&tmp_dh_key);
	}
	
	if (ret < 0) {
		gnutls_assert();
		goto error;
	}

	ret = 0;
 error:
	_gnutls_mpi_release(&session->key.proto.tls12.dh.client_Y);
	gnutls_pk_params_clear(&session->key.proto.tls12.dh.params);

	return ret;
}

int _gnutls_gen_dh_common_client_kx(gnutls_session_t session,
				    gnutls_buffer_st * data)
{
	return _gnutls_gen_dh_common_client_kx_int(session, data, NULL);
}

int
_gnutls_gen_dh_common_client_kx_int(gnutls_session_t session,
				    gnutls_buffer_st * data,
				    gnutls_datum_t * pskkey)
{
	int ret;
	gnutls_pk_params_st peer_pub;
	gnutls_datum_t tmp_dh_key = {NULL, 0};
	unsigned init_pos = data->length;
	
	gnutls_pk_params_init(&peer_pub);

	ret =
	    _gnutls_pk_generate_keys(GNUTLS_PK_DH, 0,
				     &session->key.proto.tls12.dh.params, 1);
	if (ret < 0)
		return gnutls_assert_val(ret);

	_gnutls_dh_set_secret_bits(session, _gnutls_mpi_get_nbits(session->key.proto.tls12.dh.params.params[DH_X]));

	ret = _gnutls_buffer_append_mpi(data, 16, session->key.proto.tls12.dh.params.params[DH_Y], 0);
	if (ret < 0) {
		gnutls_assert();
		goto error;
	}
	
	peer_pub.params[DH_Y] = session->key.proto.tls12.dh.client_Y;

	/* calculate the key after calculating the message */
	ret = _gnutls_pk_derive(GNUTLS_PK_DH, &tmp_dh_key, &session->key.proto.tls12.dh.params, &peer_pub);
	if (ret < 0) {
		gnutls_assert();
		goto error;
	}

	if (session->security_parameters.cs->kx_algorithm != GNUTLS_KX_DHE_PSK) {
		session->key.key.data = tmp_dh_key.data;
		session->key.key.size = tmp_dh_key.size;
	} else {		/* In DHE_PSK the key is set differently */
		ret =
		    _gnutls_set_psk_session_key(session, pskkey,
						&tmp_dh_key);
		_gnutls_free_temp_key_datum(&tmp_dh_key);
	}

	if (ret < 0) {
		gnutls_assert();
		goto error;
	}

	ret = data->length - init_pos;

 error:
	gnutls_pk_params_clear(&session->key.proto.tls12.dh.params);
	return ret;
}

/* Returns the bytes parsed */
int
_gnutls_proc_dh_common_server_kx(gnutls_session_t session,
				 uint8_t * data, size_t _data_size)
{
	uint16_t n_Y, n_g, n_p;
	size_t _n_Y, _n_g, _n_p, _n_q;
	uint8_t *data_p;
	uint8_t *data_g;
	uint8_t *data_Y;
	uint8_t *data_q = NULL;
	int i, bits, ret, p_bits;
	unsigned j;
	ssize_t data_size = _data_size;

	/* just in case we are resuming a session */
	gnutls_pk_params_release(&session->key.proto.tls12.dh.params);

	gnutls_pk_params_init(&session->key.proto.tls12.dh.params);

	i = 0;

	DECR_LEN(data_size, 2);
	n_p = _gnutls_read_uint16(&data[i]);
	i += 2;

	DECR_LEN(data_size, n_p);
	data_p = &data[i];
	i += n_p;

	DECR_LEN(data_size, 2);
	n_g = _gnutls_read_uint16(&data[i]);
	i += 2;

	DECR_LEN(data_size, n_g);
	data_g = &data[i];
	i += n_g;

	DECR_LEN(data_size, 2);
	n_Y = _gnutls_read_uint16(&data[i]);
	i += 2;

	DECR_LEN(data_size, n_Y);
	data_Y = &data[i];

	_n_Y = n_Y;
	_n_g = n_g;
	_n_p = n_p;

	if (_gnutls_mpi_init_scan_nz(&session->key.proto.tls12.dh.client_Y, data_Y, _n_Y) != 0) {
		gnutls_assert();
		return GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER;
	}

	/* if we are doing RFC7919 */
	if (session->internals.priorities->groups.have_ffdhe != 0) {
		/* verify whether the received parameters match the advertised, otherwise
		 * log that. */
		for (j=0;j<session->internals.priorities->groups.size;j++) {
			if (session->internals.priorities->groups.entry[j]->generator &&
			    session->internals.priorities->groups.entry[j]->generator->size == n_g &&
			    session->internals.priorities->groups.entry[j]->prime->size == n_p &&
			    memcmp(session->internals.priorities->groups.entry[j]->generator->data,
				   data_g, n_g) == 0 &&
			    memcmp(session->internals.priorities->groups.entry[j]->prime->data,
				   data_p, n_p) == 0) {

				session->internals.hsk_flags |= HSK_USED_FFDHE;
				_gnutls_session_group_set(session, session->internals.priorities->groups.entry[j]);
				session->key.proto.tls12.dh.params.qbits = *session->internals.priorities->groups.entry[j]->q_bits;
				data_q = session->internals.priorities->groups.entry[j]->q->data;
				_n_q = session->internals.priorities->groups.entry[j]->q->size;
				break;
			}
		}

		if (!(session->internals.hsk_flags & HSK_USED_FFDHE)) {
			_gnutls_audit_log(session, "FFDHE groups advertised, but server didn't support it; falling back to server's choice\n");
		}
	}

	if (_gnutls_mpi_init_scan_nz(&session->key.proto.tls12.dh.params.params[DH_G], data_g, _n_g) != 0) {
		gnutls_assert();
		return GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER;
	}

	if (_gnutls_mpi_init_scan_nz(&session->key.proto.tls12.dh.params.params[DH_P], data_p, _n_p) != 0) {
		gnutls_assert();
		/* we release now because session->key.proto.tls12.dh.params.params_nr is not yet set */
		_gnutls_mpi_release(&session->key.proto.tls12.dh.params.params[DH_G]);
		return GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER;
	}
	if (data_q && _gnutls_mpi_init_scan_nz(
			    &session->key.proto.tls12.dh.params.params[DH_Q],
			    data_q, _n_q) != 0) {
		/* we release now because params_nr is not yet set */
		_gnutls_mpi_release(
			&session->key.proto.tls12.dh.params.params[DH_P]);
		_gnutls_mpi_release(
			&session->key.proto.tls12.dh.params.params[DH_G]);
		return GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER;
	}

	/* include, possibly empty, q */
	session->key.proto.tls12.dh.params.params_nr = 3;
	session->key.proto.tls12.dh.params.algo = GNUTLS_PK_DH;

	if (!(session->internals.hsk_flags & HSK_USED_FFDHE)) {
		bits = _gnutls_dh_get_min_prime_bits(session);
		if (bits < 0) {
			gnutls_assert();
			return bits;
		}

		p_bits = _gnutls_mpi_get_nbits(session->key.proto.tls12.dh.params.params[DH_P]);
		if (p_bits < bits) {
			/* the prime used by the peer is not acceptable
			 */
			gnutls_assert();
			_gnutls_debug_log
			    ("Received a prime of %u bits, limit is %u\n",
			     (unsigned) _gnutls_mpi_get_nbits(session->key.proto.tls12.dh.params.params[DH_P]),
			     (unsigned) bits);
			return GNUTLS_E_DH_PRIME_UNACCEPTABLE;
		}

		if (p_bits >= DEFAULT_MAX_VERIFY_BITS) {
			gnutls_assert();
			_gnutls_debug_log
			    ("Received a prime of %u bits, limit is %u\n",
			     (unsigned) p_bits,
			     (unsigned) DEFAULT_MAX_VERIFY_BITS);
			return GNUTLS_E_DH_PRIME_UNACCEPTABLE;
		}
	}

	_gnutls_dh_save_group(session, session->key.proto.tls12.dh.params.params[DH_G],
			     session->key.proto.tls12.dh.params.params[DH_P]);
	_gnutls_dh_set_peer_public(session, session->key.proto.tls12.dh.client_Y);

	ret = n_Y + n_p + n_g + 6;

	return ret;
}

int
_gnutls_dh_common_print_server_kx(gnutls_session_t session,
				  gnutls_buffer_st * data)
{
	int ret;
	unsigned q_bits = session->key.proto.tls12.dh.params.qbits;
	unsigned init_pos = data->length;

	if (q_bits < 192 && q_bits != 0) {
		gnutls_assert();
		_gnutls_debug_log("too small q_bits value for DH: %u\n", q_bits);
		q_bits = 0; /* auto-detect */
	}

	/* Y=g^x mod p */
	ret =
	    _gnutls_pk_generate_keys(GNUTLS_PK_DH, q_bits,
				     &session->key.proto.tls12.dh.params, 1);
	if (ret < 0)
		return gnutls_assert_val(ret);

	_gnutls_dh_set_secret_bits(session, _gnutls_mpi_get_nbits(session->key.proto.tls12.dh.params.params[DH_X]));

	ret = _gnutls_buffer_append_mpi(data, 16, session->key.proto.tls12.dh.params.params[DH_P], 0);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = _gnutls_buffer_append_mpi(data, 16, session->key.proto.tls12.dh.params.params[DH_G], 0);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = _gnutls_buffer_append_mpi(data, 16, session->key.proto.tls12.dh.params.params[DH_Y], 0);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = data->length - init_pos;

cleanup:
	return ret;
}

#endif
