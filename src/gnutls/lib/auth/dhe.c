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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 */

/* This file contains everything for the Ephemeral Diffie-Hellman
 * (DHE) key exchange.  This is used in the handshake procedure of the
 * certificate authentication.
 */

#include "gnutls_int.h"
#include "auth.h"
#include "errors.h"
#include "dh.h"
#include "num.h"
#include "tls-sig.h"
#include <datum.h>
#include <algorithms.h>
#include <auth/cert.h>
#include <x509.h>
#include <state.h>
#include <auth/dh_common.h>
#include <auth/ecdhe.h>

static int gen_dhe_server_kx(gnutls_session_t, gnutls_buffer_st *);
static int proc_dhe_server_kx(gnutls_session_t, uint8_t *, size_t);
static int proc_dhe_client_kx(gnutls_session_t, uint8_t *, size_t);

#ifdef ENABLE_DHE

const mod_auth_st dhe_rsa_auth_struct = {
	"DHE_RSA",
	_gnutls_gen_cert_server_crt,
	_gnutls_gen_cert_client_crt,
	gen_dhe_server_kx,
	_gnutls_gen_dh_common_client_kx,
	_gnutls_gen_cert_client_crt_vrfy,	/* gen client cert vrfy */
	_gnutls_gen_cert_server_cert_req,	/* server cert request */

	_gnutls_proc_crt,
	_gnutls_proc_crt,
	proc_dhe_server_kx,
	proc_dhe_client_kx,
	_gnutls_proc_cert_client_crt_vrfy,	/* proc client cert vrfy */
	_gnutls_proc_cert_cert_req	/* proc server cert request */
};

const mod_auth_st dhe_dss_auth_struct = {
	"DHE_DSS",
	_gnutls_gen_cert_server_crt,
	_gnutls_gen_cert_client_crt,
	gen_dhe_server_kx,
	_gnutls_gen_dh_common_client_kx,
	_gnutls_gen_cert_client_crt_vrfy,	/* gen client cert vrfy */
	_gnutls_gen_cert_server_cert_req,	/* server cert request */

	_gnutls_proc_crt,
	_gnutls_proc_crt,
	proc_dhe_server_kx,
	proc_dhe_client_kx,
	_gnutls_proc_cert_client_crt_vrfy,	/* proc client cert vrfy */
	_gnutls_proc_cert_cert_req	/* proc server cert request */
};

#endif

static int
gen_dhe_server_kx(gnutls_session_t session, gnutls_buffer_st * data)
{
	bigint_t g, p;
	const bigint_t *mpis;
	int ret = 0;
	gnutls_certificate_credentials_t cred;
	gnutls_dh_params_t dh_params;

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

	dh_params =
	    _gnutls_get_dh_params(cred->dh_params, cred->params_func,
				  session);
	mpis = _gnutls_dh_params_to_mpi(dh_params);
	if (mpis == NULL) {
		gnutls_assert();
		return GNUTLS_E_NO_TEMPORARY_DH_PARAMS;
	}

	p = mpis[0];
	g = mpis[1];

	_gnutls_dh_set_group(session, g, p);

	ret = _gnutls_set_dh_pk_params(session, g, p, dh_params->q_bits);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret =
	    _gnutls_dh_common_print_server_kx(session, data);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	/* Generate the signature. */
	return _gnutls_gen_dhe_signature(session, data, data->data,
					 data->length);
}


static int
proc_dhe_server_kx(gnutls_session_t session, uint8_t * data,
		   size_t _data_size)
{
	gnutls_datum_t vdata;
	int ret;

	ret = _gnutls_proc_dh_common_server_kx(session, data, _data_size);
	if (ret < 0)
		return gnutls_assert_val(ret);

	vdata.data = data;
	vdata.size = ret;

	return _gnutls_proc_dhe_signature(session, data + ret,
					  _data_size - ret, &vdata);
}


static int
proc_dhe_client_kx(gnutls_session_t session, uint8_t * data,
		   size_t _data_size)
{
	gnutls_certificate_credentials_t cred;
	bigint_t p, g;
	const bigint_t *mpis;
	gnutls_dh_params_t dh_params;

	cred = (gnutls_certificate_credentials_t)
	    _gnutls_get_cred(session, GNUTLS_CRD_CERTIFICATE);
	if (cred == NULL) {
		gnutls_assert();
		return GNUTLS_E_INSUFFICIENT_CREDENTIALS;
	}

	dh_params =
	    _gnutls_get_dh_params(cred->dh_params, cred->params_func,
				  session);
	mpis = _gnutls_dh_params_to_mpi(dh_params);
	if (mpis == NULL)
		return gnutls_assert_val(GNUTLS_E_NO_TEMPORARY_DH_PARAMS);

	p = mpis[0];
	g = mpis[1];

	return _gnutls_proc_dh_common_client_kx(session, data, _data_size,
						g, p, NULL);
}
