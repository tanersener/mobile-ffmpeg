/*
 * Copyright (C) 2000-2012 Free Software Foundation, Inc.
 * Copyright (C) 2017 Red Hat, Inc.
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
#include "errors.h"
#include <datum.h>
#include <x509_b64.h>		/* for PKCS3 PEM decoding */
#include <global.h>
#include <dh.h>
#include <pk.h>
#include <x509/common.h>
#include <gnutls/crypto.h>
#include "x509/x509_int.h"
#include <mpi.h>
#include "debug.h"
#include "state.h"

static
int set_dh_pk_params(gnutls_session_t session, bigint_t g, bigint_t p,
		     bigint_t q, unsigned q_bits)
{
	/* just in case we are resuming a session */
	gnutls_pk_params_release(&session->key.proto.tls12.dh.params);

	gnutls_pk_params_init(&session->key.proto.tls12.dh.params);

	session->key.proto.tls12.dh.params.params[DH_G] = _gnutls_mpi_copy(g);
	if (session->key.proto.tls12.dh.params.params[DH_G] == NULL)
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

	session->key.proto.tls12.dh.params.params[DH_P] = _gnutls_mpi_copy(p);
	if (session->key.proto.tls12.dh.params.params[DH_P] == NULL) {
		_gnutls_mpi_release(&session->key.proto.tls12.dh.params.params[DH_G]);
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);
	}

	if (q) {
		session->key.proto.tls12.dh.params.params[DH_Q] = _gnutls_mpi_copy(q);
		if (session->key.proto.tls12.dh.params.params[DH_Q] == NULL) {
			_gnutls_mpi_release(&session->key.proto.tls12.dh.params.params[DH_P]);
			_gnutls_mpi_release(&session->key.proto.tls12.dh.params.params[DH_G]);
			return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);
		}
	}
	/* include, possibly empty, q */
	session->key.proto.tls12.dh.params.params_nr = 3;
	session->key.proto.tls12.dh.params.algo = GNUTLS_PK_DH;
	session->key.proto.tls12.dh.params.qbits = q_bits;

	return 0;
}

/* Use all available information to decide the DH parameters to use,
 * that being the negotiated RFC7919 group, the callback, and the
 * provided parameters structure.
 */
int
_gnutls_figure_dh_params(gnutls_session_t session, gnutls_dh_params_t dh_params,
		      gnutls_params_function * func, gnutls_sec_param_t sec_param)
{
	gnutls_params_st params;
	bigint_t p, g, q = NULL;
	unsigned free_pg = 0;
	int ret;
	unsigned q_bits = 0, i;
	const gnutls_group_entry_st *group;

	group = get_group(session);

	params.deinit = 0;

	/* if we negotiated RFC7919 FFDHE */
	if (group && group->pk == GNUTLS_PK_DH) {
		for (i=0;i<session->internals.priorities->groups.size;i++) {
			if (session->internals.priorities->groups.entry[i] == group) {
				ret = _gnutls_mpi_init_scan_nz(&p,
						session->internals.priorities->groups.entry[i]->prime->data,
						session->internals.priorities->groups.entry[i]->prime->size);
				if (ret < 0)
					return gnutls_assert_val(ret);

				free_pg = 1;

				ret = _gnutls_mpi_init_scan_nz(&g,
						session->internals.priorities->groups.entry[i]->generator->data,
						session->internals.priorities->groups.entry[i]->generator->size);
				if (ret < 0) {
					gnutls_assert();
					goto cleanup;
				}

				ret = _gnutls_mpi_init_scan_nz(&q,
						session->internals.priorities->groups.entry[i]->q->data,
						session->internals.priorities->groups.entry[i]->q->size);
				if (ret < 0) {
					gnutls_assert();
					goto cleanup;
				}

				session->internals.hsk_flags |= HSK_USED_FFDHE;
				q_bits = *session->internals.priorities->groups.entry[i]->q_bits;
				goto finished;
			}
		}

		/* didn't find anything, that shouldn't have occurred
		 * as we received that extension */
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);
	} else if (sec_param) {
		unsigned bits = gnutls_sec_param_to_pk_bits(GNUTLS_PK_DH, sec_param)/8;

		for (i=0;i<session->internals.priorities->groups.size;i++) {
			if (!session->internals.priorities->groups.entry[i]->prime)
				continue;

			if (bits <= session->internals.priorities->groups.entry[i]->prime->size) {
				ret = _gnutls_mpi_init_scan_nz(&p,
						session->internals.priorities->groups.entry[i]->prime->data,
						session->internals.priorities->groups.entry[i]->prime->size);
				if (ret < 0)
					return gnutls_assert_val(ret);

				free_pg = 1;

				ret = _gnutls_mpi_init_scan_nz(&g,
						session->internals.priorities->groups.entry[i]->generator->data,
						session->internals.priorities->groups.entry[i]->generator->size);
				if (ret < 0) {
					gnutls_assert();
					goto cleanup;
				}

				q_bits = *session->internals.priorities->groups.entry[i]->q_bits;
				goto finished;
			}
		}

	}

	if (dh_params) {
		p = dh_params->params[0];
		g = dh_params->params[1];
		q_bits = dh_params->q_bits;
	} else if (func) {
		ret = func(session, GNUTLS_PARAMS_DH, &params);
		if (ret == 0 && params.type == GNUTLS_PARAMS_DH) {
			p = params.params.dh->params[0];
			g = params.params.dh->params[1];
			q_bits = params.params.dh->q_bits;
		} else
			return gnutls_assert_val(GNUTLS_E_NO_TEMPORARY_DH_PARAMS);
	} else
		return gnutls_assert_val(GNUTLS_E_NO_TEMPORARY_DH_PARAMS);

 finished:
	_gnutls_dh_save_group(session, g, p);

	ret = set_dh_pk_params(session, g, p, q, q_bits);
	if (ret < 0) {
		gnutls_assert();
	}

 cleanup:
	if (free_pg) {
		_gnutls_mpi_release(&p);
		_gnutls_mpi_release(&q);
		_gnutls_mpi_release(&g);
	}
	if (params.deinit && params.type == GNUTLS_PARAMS_DH)
		gnutls_dh_params_deinit(params.params.dh);

	return ret;

}

/* returns the prime and the generator of DH params.
 */
const bigint_t *_gnutls_dh_params_to_mpi(gnutls_dh_params_t dh_primes)
{
	if (dh_primes == NULL || dh_primes->params[1] == NULL ||
	    dh_primes->params[0] == NULL) {
		return NULL;
	}

	return dh_primes->params;
}


/**
 * gnutls_dh_params_import_raw:
 * @dh_params: The parameters
 * @prime: holds the new prime
 * @generator: holds the new generator
 *
 * This function will replace the pair of prime and generator for use
 * in the Diffie-Hellman key exchange.  The new parameters should be
 * stored in the appropriate gnutls_datum.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned,
 *   otherwise a negative error code is returned.
 **/
int
gnutls_dh_params_import_raw(gnutls_dh_params_t dh_params,
			    const gnutls_datum_t * prime,
			    const gnutls_datum_t * generator)
{
	return gnutls_dh_params_import_raw2(dh_params, prime, generator, 0);
}

/**
 * gnutls_dh_params_import_dsa:
 * @dh_params: The parameters
 * @key: holds a DSA private key
 *
 * This function will import the prime and generator of the DSA key for use 
 * in the Diffie-Hellman key exchange.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned,
 *   otherwise a negative error code is returned.
 **/
int
gnutls_dh_params_import_dsa(gnutls_dh_params_t dh_params, gnutls_x509_privkey_t key)
{
	gnutls_datum_t p, g, q;
	int ret;

	ret = gnutls_x509_privkey_export_dsa_raw(key, &p, &q, &g, NULL, NULL);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = gnutls_dh_params_import_raw3(dh_params, &p, &q, &g);

	gnutls_free(p.data);
	gnutls_free(g.data);
	gnutls_free(q.data);

	return ret;
}

/**
 * gnutls_dh_params_import_raw2:
 * @dh_params: The parameters
 * @prime: holds the new prime
 * @generator: holds the new generator
 * @key_bits: the private key bits (set to zero when unknown)
 *
 * This function will replace the pair of prime and generator for use
 * in the Diffie-Hellman key exchange.  The new parameters should be
 * stored in the appropriate gnutls_datum.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned,
 *   otherwise a negative error code is returned.
 **/
int
gnutls_dh_params_import_raw2(gnutls_dh_params_t dh_params,
			     const gnutls_datum_t * prime,
			     const gnutls_datum_t * generator,
			     unsigned key_bits)
{
	bigint_t tmp_prime, tmp_g;
	size_t siz;

	siz = prime->size;
	if (_gnutls_mpi_init_scan_nz(&tmp_prime, prime->data, siz)) {
		gnutls_assert();
		return GNUTLS_E_MPI_SCAN_FAILED;
	}

	siz = generator->size;
	if (_gnutls_mpi_init_scan_nz(&tmp_g, generator->data, siz)) {
		_gnutls_mpi_release(&tmp_prime);
		gnutls_assert();
		return GNUTLS_E_MPI_SCAN_FAILED;
	}

	/* store the generated values
	 */
	dh_params->params[0] = tmp_prime;
	dh_params->params[1] = tmp_g;
	dh_params->q_bits = key_bits;

	return 0;
}

/**
 * gnutls_dh_params_import_raw3:
 * @dh_params: The parameters
 * @prime: holds the new prime
 * @q: holds the subgroup if available, otherwise NULL
 * @generator: holds the new generator
 *
 * This function will replace the pair of prime and generator for use
 * in the Diffie-Hellman key exchange.  The new parameters should be
 * stored in the appropriate gnutls_datum.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned,
 *   otherwise a negative error code is returned.
 **/
int
gnutls_dh_params_import_raw3(gnutls_dh_params_t dh_params,
			     const gnutls_datum_t * prime,
			     const gnutls_datum_t * q,
			     const gnutls_datum_t * generator)
{
	bigint_t tmp_p, tmp_g, tmp_q = NULL;

	if (_gnutls_mpi_init_scan_nz(&tmp_p, prime->data, prime->size)) {
		gnutls_assert();
		return GNUTLS_E_MPI_SCAN_FAILED;
	}

	if (_gnutls_mpi_init_scan_nz(&tmp_g, generator->data,
				     generator->size)) {
		_gnutls_mpi_release(&tmp_p);
		gnutls_assert();
		return GNUTLS_E_MPI_SCAN_FAILED;
	}

	if (q) {
		if (_gnutls_mpi_init_scan_nz(&tmp_q, q->data, q->size)) {
			_gnutls_mpi_release(&tmp_p);
			_gnutls_mpi_release(&tmp_g);
			gnutls_assert();
			return GNUTLS_E_MPI_SCAN_FAILED;
		}
	} else if (_gnutls_fips_mode_enabled()) {
		/* Mandatory in FIPS mode */
		gnutls_assert();
		return GNUTLS_E_DH_PRIME_UNACCEPTABLE;
	}

	/* store the generated values
	 */
	dh_params->params[0] = tmp_p;
	dh_params->params[1] = tmp_g;
	dh_params->params[2] = tmp_q;
	if (tmp_q)
		dh_params->q_bits = _gnutls_mpi_get_nbits(tmp_q);

	return 0;
}

/**
 * gnutls_dh_params_init:
 * @dh_params: The parameters
 *
 * This function will initialize the DH parameters type.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned,
 *   otherwise a negative error code is returned.
 **/
int gnutls_dh_params_init(gnutls_dh_params_t * dh_params)
{

	(*dh_params) = gnutls_calloc(1, sizeof(dh_params_st));
	if (*dh_params == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	return 0;

}

/**
 * gnutls_dh_params_deinit:
 * @dh_params: The parameters
 *
 * This function will deinitialize the DH parameters type.
 **/
void gnutls_dh_params_deinit(gnutls_dh_params_t dh_params)
{
	if (dh_params == NULL)
		return;

	_gnutls_mpi_release(&dh_params->params[0]);
	_gnutls_mpi_release(&dh_params->params[1]);
	_gnutls_mpi_release(&dh_params->params[2]);

	gnutls_free(dh_params);

}

/**
 * gnutls_dh_params_cpy:
 * @dst: Is the destination parameters, which should be initialized.
 * @src: Is the source parameters
 *
 * This function will copy the DH parameters structure from source
 * to destination. The destination should be already initialized.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned,
 *   otherwise a negative error code is returned.
 **/
int gnutls_dh_params_cpy(gnutls_dh_params_t dst, gnutls_dh_params_t src)
{
	if (src == NULL)
		return GNUTLS_E_INVALID_REQUEST;

	dst->params[0] = _gnutls_mpi_copy(src->params[0]);
	dst->params[1] = _gnutls_mpi_copy(src->params[1]);
	if (src->params[2])
		dst->params[2] = _gnutls_mpi_copy(src->params[2]);
	dst->q_bits = src->q_bits;

	if (dst->params[0] == NULL || dst->params[1] == NULL)
		return GNUTLS_E_MEMORY_ERROR;

	return 0;
}


/**
 * gnutls_dh_params_generate2:
 * @dparams: The parameters
 * @bits: is the prime's number of bits
 *
 * This function will generate a new pair of prime and generator for use in
 * the Diffie-Hellman key exchange. This may take long time.
 *
 * It is recommended not to set the number of bits directly, but 
 * use gnutls_sec_param_to_pk_bits() instead.

 * Also note that the DH parameters are only useful to servers.
 * Since clients use the parameters sent by the server, it's of
 * no use to call this in client side.
 *
 * The parameters generated are of the DSA form. It also is possible
 * to generate provable parameters (following the Shawe-Taylor
 * algorithm), using gnutls_x509_privkey_generate2() with DSA option
 * and the %GNUTLS_PRIVKEY_FLAG_PROVABLE flag set. These can the
 * be imported with gnutls_dh_params_import_dsa().
 *
 * It is no longer recommended for applications to generate parameters.
 * See the "Parameter generation" section in the manual.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned,
 *   otherwise a negative error code is returned.
 **/
int
gnutls_dh_params_generate2(gnutls_dh_params_t dparams, unsigned int bits)
{
	int ret;
	gnutls_pk_params_st params;

	gnutls_pk_params_init(&params);

	ret = _gnutls_pk_generate_params(GNUTLS_PK_DH, bits, &params);
	if (ret < 0)
		return gnutls_assert_val(ret);

	dparams->params[0] = params.params[DSA_P];
	dparams->params[1] = params.params[DSA_G];
	dparams->q_bits = _gnutls_mpi_get_nbits(params.params[DSA_Q]);

	_gnutls_mpi_release(&params.params[DSA_Q]);

	return 0;
}

/**
 * gnutls_dh_params_import_pkcs3:
 * @params: The parameters
 * @pkcs3_params: should contain a PKCS3 DHParams structure PEM or DER encoded
 * @format: the format of params. PEM or DER.
 *
 * This function will extract the DHParams found in a PKCS3 formatted
 * structure. This is the format generated by "openssl dhparam" tool.
 *
 * If the structure is PEM encoded, it should have a header
 * of "BEGIN DH PARAMETERS".
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned,
 *   otherwise a negative error code is returned.
 **/
int
gnutls_dh_params_import_pkcs3(gnutls_dh_params_t params,
			      const gnutls_datum_t * pkcs3_params,
			      gnutls_x509_crt_fmt_t format)
{
	ASN1_TYPE c2;
	int result, need_free = 0;
	unsigned int q_bits;
	gnutls_datum_t _params;

	if (format == GNUTLS_X509_FMT_PEM) {

		result = _gnutls_fbase64_decode("DH PARAMETERS",
						pkcs3_params->data,
						pkcs3_params->size,
						&_params);

		if (result < 0) {
			gnutls_assert();
			return result;
		}

		need_free = 1;
	} else {
		_params.data = pkcs3_params->data;
		_params.size = pkcs3_params->size;
	}

	if ((result = asn1_create_element
	     (_gnutls_get_gnutls_asn(), "GNUTLS.DHParameter", &c2))
	    != ASN1_SUCCESS) {
		gnutls_assert();
		if (need_free != 0) {
			gnutls_free(_params.data);
			_params.data = NULL;
		}
		return _gnutls_asn2err(result);
	}

	/* PKCS#3 doesn't specify whether DHParameter is encoded as
	 * BER or DER, thus we don't restrict libtasn1 to DER subset */
	result = asn1_der_decoding(&c2, _params.data, _params.size, NULL);

	if (need_free != 0) {
		gnutls_free(_params.data);
		_params.data = NULL;
	}

	if (result != ASN1_SUCCESS) {
		/* couldn't decode DER */

		_gnutls_debug_log("DHParams: Decoding error %d\n", result);
		gnutls_assert();
		asn1_delete_structure(&c2);
		return _gnutls_asn2err(result);
	}

	/* Read q length */
	result = _gnutls_x509_read_uint(c2, "privateValueLength", &q_bits);
	if (result < 0) {
		gnutls_assert();
		params->q_bits = 0;
	} else
		params->q_bits = q_bits;

	/* Read PRIME 
	 */
	result = _gnutls_x509_read_int(c2, "prime", &params->params[0]);
	if (result < 0) {
		asn1_delete_structure(&c2);
		gnutls_assert();
		return result;
	}

	if (_gnutls_mpi_cmp_ui(params->params[0], 0) == 0) {
		asn1_delete_structure(&c2);
		return gnutls_assert_val(GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER);
	}

	/* read the generator
	 */
	result = _gnutls_x509_read_int(c2, "base", &params->params[1]);
	if (result < 0) {
		asn1_delete_structure(&c2);
		_gnutls_mpi_release(&params->params[0]);
		gnutls_assert();
		return result;
	}

	if (_gnutls_mpi_cmp_ui(params->params[1], 0) == 0) {
		asn1_delete_structure(&c2);
		_gnutls_mpi_release(&params->params[0]);
		return gnutls_assert_val(GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER);
	}

	asn1_delete_structure(&c2);

	return 0;
}

/**
 * gnutls_dh_params_export_pkcs3:
 * @params: Holds the DH parameters
 * @format: the format of output params. One of PEM or DER.
 * @params_data: will contain a PKCS3 DHParams structure PEM or DER encoded
 * @params_data_size: holds the size of params_data (and will be replaced by the actual size of parameters)
 *
 * This function will export the given dh parameters to a PKCS3
 * DHParams structure. This is the format generated by "openssl dhparam" tool.
 * If the buffer provided is not long enough to hold the output, then
 * GNUTLS_E_SHORT_MEMORY_BUFFER will be returned.
 *
 * If the structure is PEM encoded, it will have a header
 * of "BEGIN DH PARAMETERS".
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned,
 *   otherwise a negative error code is returned.
 **/
int
gnutls_dh_params_export_pkcs3(gnutls_dh_params_t params,
			      gnutls_x509_crt_fmt_t format,
			      unsigned char *params_data,
			      size_t * params_data_size)
{
	gnutls_datum_t out = {NULL, 0};
	int ret;

	ret = gnutls_dh_params_export2_pkcs3(params, format, &out);
	if (ret < 0)
		return gnutls_assert_val(ret);

	if (*params_data_size < (unsigned) out.size + 1) {
		gnutls_assert();
		gnutls_free(out.data);
		*params_data_size = out.size + 1;
		return GNUTLS_E_SHORT_MEMORY_BUFFER;
	}

	assert(out.data != NULL);
	*params_data_size = out.size;
	if (params_data) {
		memcpy(params_data, out.data, out.size);
		params_data[out.size] = 0;
	}

	gnutls_free(out.data);

	return 0;
}

/**
 * gnutls_dh_params_export2_pkcs3:
 * @params: Holds the DH parameters
 * @format: the format of output params. One of PEM or DER.
 * @out: will contain a PKCS3 DHParams structure PEM or DER encoded
 *
 * This function will export the given dh parameters to a PKCS3
 * DHParams structure. This is the format generated by "openssl dhparam" tool.
 * The data in @out will be allocated using gnutls_malloc().
 *
 * If the structure is PEM encoded, it will have a header
 * of "BEGIN DH PARAMETERS".
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned,
 *   otherwise a negative error code is returned.
 *
 * Since: 3.1.3
 **/
int
gnutls_dh_params_export2_pkcs3(gnutls_dh_params_t params,
			       gnutls_x509_crt_fmt_t format,
			       gnutls_datum_t * out)
{
	ASN1_TYPE c2;
	int result;
	size_t g_size, p_size;
	uint8_t *p_data, *g_data;
	uint8_t *all_data;

	_gnutls_mpi_print_lz(params->params[1], NULL, &g_size);
	_gnutls_mpi_print_lz(params->params[0], NULL, &p_size);

	all_data = gnutls_malloc(g_size + p_size);
	if (all_data == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	p_data = &all_data[0];
	_gnutls_mpi_print_lz(params->params[0], p_data, &p_size);

	g_data = &all_data[p_size];
	_gnutls_mpi_print_lz(params->params[1], g_data, &g_size);


	/* Ok. Now we have the data. Create the asn1 structures
	 */

	if ((result = asn1_create_element
	     (_gnutls_get_gnutls_asn(), "GNUTLS.DHParameter", &c2))
	    != ASN1_SUCCESS) {
		gnutls_assert();
		gnutls_free(all_data);
		return _gnutls_asn2err(result);
	}

	/* Write PRIME 
	 */
	if ((result = asn1_write_value(c2, "prime",
				       p_data, p_size)) != ASN1_SUCCESS) {
		gnutls_assert();
		gnutls_free(all_data);
		asn1_delete_structure(&c2);
		return _gnutls_asn2err(result);
	}

	if (params->q_bits > 0)
		result =
		    _gnutls_x509_write_uint32(c2, "privateValueLength",
					      params->q_bits);
	else
		result =
		    asn1_write_value(c2, "privateValueLength", NULL, 0);

	if (result < 0) {
		gnutls_assert();
		gnutls_free(all_data);
		asn1_delete_structure(&c2);
		return _gnutls_asn2err(result);
	}

	/* Write the GENERATOR
	 */
	if ((result = asn1_write_value(c2, "base",
				       g_data, g_size)) != ASN1_SUCCESS) {
		gnutls_assert();
		gnutls_free(all_data);
		asn1_delete_structure(&c2);
		return _gnutls_asn2err(result);
	}

	gnutls_free(all_data);


	if (format == GNUTLS_X509_FMT_DER) {
		result = _gnutls_x509_der_encode(c2, "", out, 0);

		asn1_delete_structure(&c2);

		if (result < 0)
			return gnutls_assert_val(result);

	} else {		/* PEM */
		gnutls_datum_t t;

		result = _gnutls_x509_der_encode(c2, "", &t, 0);

		asn1_delete_structure(&c2);

		if (result < 0)
			return gnutls_assert_val(result);

		result =
		    _gnutls_fbase64_encode("DH PARAMETERS", t.data, t.size,
					   out);

		gnutls_free(t.data);

		if (result < 0) {
			gnutls_assert();
			return result;
		}
	}

	return 0;
}

/**
 * gnutls_dh_params_export_raw:
 * @params: Holds the DH parameters
 * @prime: will hold the new prime
 * @generator: will hold the new generator
 * @bits: if non null will hold the secret key's number of bits
 *
 * This function will export the pair of prime and generator for use
 * in the Diffie-Hellman key exchange.  The new parameters will be
 * allocated using gnutls_malloc() and will be stored in the
 * appropriate datum.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned,
 *   otherwise a negative error code is returned.
 **/
int
gnutls_dh_params_export_raw(gnutls_dh_params_t params,
			    gnutls_datum_t * prime,
			    gnutls_datum_t * generator, unsigned int *bits)
{
	int ret;

	if (params->params[1] == NULL || params->params[0] == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	ret = _gnutls_mpi_dprint(params->params[1], generator);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	ret = _gnutls_mpi_dprint(params->params[0], prime);
	if (ret < 0) {
		gnutls_assert();
		_gnutls_free_datum(generator);
		return ret;
	}

	if (bits)
		*bits = params->q_bits;

	return 0;

}
