/*
 * Copyright (C) 2001-2012 Free Software Foundation, Inc.
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

/* The certificate authentication functions which are needed in the handshake,
 * and are common to RSA and DHE key exchange, are in this file.
 */

#include "gnutls_int.h"
#include "auth.h"
#include "errors.h"
#include <auth/cert.h>
#include "dh.h"
#include "num.h"
#include "libtasn1.h"
#include "datum.h"
#include "ext/signature.h"
#include <pk.h>
#include <algorithms.h>
#include <global.h>
#include <record.h>
#include <tls-sig.h>
#include <state.h>
#include <pk.h>
#include <x509.h>
#include <x509/verify-high.h>
#include <gnutls/abstract.h>
#include "abstract_int.h"
#include "debug.h"

static void
selected_certs_set(gnutls_session_t session,
		   gnutls_pcert_st * certs, int ncerts,
		   gnutls_ocsp_data_st *ocsp, unsigned nocsp,
		   gnutls_privkey_t key, int need_free,
		   gnutls_status_request_ocsp_func ocsp_func,
		   void *ocsp_func_ptr);

#define MAX_CLIENT_SIGN_ALGOS 3
#define CERTTYPE_SIZE (MAX_CLIENT_SIGN_ALGOS+1)
typedef enum CertificateSigType { RSA_SIGN = 1, DSA_SIGN = 2, ECDSA_SIGN = 64
} CertificateSigType;

/* Moves data from an internal certificate struct (gnutls_pcert_st) to
 * another internal certificate struct (cert_auth_info_t), and deinitializes
 * the former.
 */
int _gnutls_pcert_to_auth_info(cert_auth_info_t info, gnutls_pcert_st * certs, size_t ncerts)
{
	size_t i, j;

	if (info->raw_certificate_list != NULL) {
		for (j = 0; j < info->ncerts; j++)
			_gnutls_free_datum(&info->raw_certificate_list[j]);
		gnutls_free(info->raw_certificate_list);
	}

	if (ncerts == 0) {
		info->raw_certificate_list = NULL;
		info->ncerts = 0;
		return 0;
	}

	info->raw_certificate_list =
	    gnutls_calloc(ncerts, sizeof(gnutls_datum_t));
	if (info->raw_certificate_list == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	info->cert_type = certs[0].type;
	info->ncerts = ncerts;

	for (i = 0; i < ncerts; i++) {
		info->raw_certificate_list[i].data = certs[i].cert.data;
		info->raw_certificate_list[i].size = certs[i].cert.size;
		certs[i].cert.data = NULL;
		gnutls_pcert_deinit(&certs[i]);
	}
	gnutls_free(certs);

	return 0;
}

/* returns 0 if the algo_to-check exists in the pk_algos list,
 * -1 otherwise.
 */
inline static int
check_pk_algo_in_list(const gnutls_pk_algorithm_t *
		      pk_algos, int pk_algos_length,
		      gnutls_pk_algorithm_t algo_to_check)
{
	int i;
	for (i = 0; i < pk_algos_length; i++) {
		if (algo_to_check == pk_algos[i]) {
			return 0;
		}
	}
	return -1;
}

/* Returns the issuer's Distinguished name in odn, of the certificate
 * specified in cert.
 */
static int cert_get_issuer_dn(gnutls_pcert_st * cert, gnutls_datum_t * odn)
{
	ASN1_TYPE dn;
	int len, result;
	int start, end;

	if ((result = asn1_create_element
	     (_gnutls_get_pkix(), "PKIX1.Certificate", &dn)) != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	result = asn1_der_decoding(&dn, cert->cert.data, cert->cert.size, NULL);
	if (result != ASN1_SUCCESS) {
		/* couldn't decode DER */
		gnutls_assert();
		asn1_delete_structure(&dn);
		return _gnutls_asn2err(result);
	}

	result =
	    asn1_der_decoding_startEnd(dn, cert->cert.data,
				       cert->cert.size,
				       "tbsCertificate.issuer", &start, &end);

	if (result != ASN1_SUCCESS) {
		/* couldn't decode DER */
		gnutls_assert();
		asn1_delete_structure(&dn);
		return _gnutls_asn2err(result);
	}
	asn1_delete_structure(&dn);

	len = end - start + 1;

	odn->size = len;
	odn->data = &cert->cert.data[start];

	return 0;
}

/* Locates the most appropriate x509 certificate using the
 * given DN. If indx == -1 then no certificate was found.
 *
 * That is to guess which certificate to use, based on the
 * CAs and sign algorithms supported by the peer server.
 */
static int
find_x509_client_cert(gnutls_session_t session,
	       const gnutls_certificate_credentials_t cred,
	       const uint8_t * _data, size_t _data_size,
	       const gnutls_pk_algorithm_t * pk_algos,
	       int pk_algos_length, int *indx)
{
	unsigned size;
	gnutls_datum_t odn = { NULL, 0 }, asked_dn;
	const uint8_t *data = _data;
	ssize_t data_size = _data_size;
	unsigned i, j;
	int result, cert_pk;
	unsigned key_usage;

	*indx = -1;

	/* If peer doesn't send any issuers and we have a single certificate
	 * then send that one.
	 */
	if (cred->ncerts == 1 &&
	    (data_size == 0
	     || (session->internals.flags & GNUTLS_FORCE_CLIENT_CERT))) {
		if (cred->certs[0].cert_list[0].type == GNUTLS_CRT_X509) {

			key_usage = get_key_usage(session, cred->certs[0].cert_list[0].pubkey);

			/* For client certificates we require signatures */
			result = _gnutls_check_key_usage_for_sig(session, key_usage, 1);
			if (result < 0) {
				_gnutls_debug_log("Client certificate is not suitable for signing\n");
				return gnutls_assert_val(result);
			}

			*indx = 0;
			return 0;
		}
	}

	do {
		DECR_LENGTH_RET(data_size, 2, 0);
		size = _gnutls_read_uint16(data);
		DECR_LENGTH_RET(data_size, size, 0);
		data += 2;

		asked_dn.data = (void*)data;
		asked_dn.size = size;
		_gnutls_dn_log("Peer requested CA", &asked_dn);

		for (i = 0; i < cred->ncerts; i++) {
			for (j = 0; j < cred->certs[i].cert_list_length; j++) {
				if ((result =
				     cert_get_issuer_dn(&cred->certs
							[i].cert_list
							[j], &odn)) < 0) {
					gnutls_assert();
					return result;
				}

				if (odn.size == 0 || odn.size != asked_dn.size)
					continue;

				key_usage = get_key_usage(session, cred->certs[i].cert_list[0].pubkey);

				/* For client certificates we require signatures */
				if (_gnutls_check_key_usage_for_sig(session, key_usage, 1) < 0) {
					_gnutls_debug_log("Client certificate is not suitable for signing\n");
					continue;
				}

				/* If the DN matches and
				 * the *_SIGN algorithm matches
				 * the cert is our cert!
				 */
				cert_pk =
				    gnutls_pubkey_get_pk_algorithm(cred->certs
								   [i].cert_list
								   [0].pubkey,
								   NULL);

				if ((memcmp(odn.data, asked_dn.data, asked_dn.size) == 0) &&
				    (check_pk_algo_in_list
				     (pk_algos, pk_algos_length,
				      cert_pk) == 0)) {
					*indx = i;
					break;
				}
			}
			if (*indx != -1)
				break;
		}

		if (*indx != -1)
			break;

		/* move to next record */
		data += size;
	}
	while (1);

	return 0;

}


/* Locates the first raw public-key.
 * Currently it only makes sense to associate one raw pubkey per session.
 * Associating more raw pubkeys with a session has no use because we
 * don't know how to select the correct one.
 */
static int
find_rawpk_client_cert(gnutls_session_t session,
			const gnutls_certificate_credentials_t cred,
			const gnutls_pk_algorithm_t* pk_algos,
			int pk_algos_length, int* indx)
{
	unsigned i;
	int ret;
	gnutls_pk_algorithm_t pk;

	*indx = -1;

	for (i = 0; i < cred->ncerts; i++) {
		/* We know that our list length will be 1, therefore we can
		 * ignore the rest.
		 */
		if (cred->certs[i].cert_list_length == 1 && cred->certs[i].cert_list[0].type == GNUTLS_CRT_RAWPK) {
			pk = gnutls_pubkey_get_pk_algorithm(cred->certs[i].cert_list[0].pubkey, NULL);

			/* For client certificates we require signatures */
			ret = _gnutls_check_key_usage_for_sig(session, get_key_usage(session, cred->certs[i].cert_list[0].pubkey), 1);
			if (ret < 0) {
				/* we return an error instead of skipping so that the user is notified about
				 * the key incompatibility */
				_gnutls_debug_log("Client certificate is not suitable for signing\n");
				return gnutls_assert_val(ret);
			}

			/* Check whether the public-key algorithm of our credential is in
			 * the list with supported public-key algorithms and whether the
			 * cert type matches. */
			if ((check_pk_algo_in_list(pk_algos, pk_algos_length, pk) == 0)) {
				// We found a compatible credential
				*indx = i;
				break;
			}
		}
	}

	return 0;
}


/* Returns the number of issuers in the server's
 * certificate request packet.
 */
static int
get_issuers_num(gnutls_session_t session, const uint8_t * data, ssize_t data_size)
{
	int issuers_dn_len = 0;
	unsigned size;

	/* Count the number of the given issuers;
	 * This is used to allocate the issuers_dn without
	 * using realloc().
	 */

	if (data_size == 0 || data == NULL)
		return 0;

	while (data_size > 0) {
		/* This works like DECR_LEN()
		 */
		DECR_LENGTH_RET(data_size, 2, GNUTLS_E_UNEXPECTED_PACKET_LENGTH);
		size = _gnutls_read_uint16(data);

		DECR_LENGTH_RET(data_size, size, GNUTLS_E_UNEXPECTED_PACKET_LENGTH);

		data += 2;

		if (size > 0) {
			issuers_dn_len++;
			data += size;
		}
	}

	return issuers_dn_len;
}

/* Returns the issuers in the server's certificate request
 * packet.
 */
static int
get_issuers(gnutls_session_t session,
	    gnutls_datum_t * issuers_dn, int issuers_len,
	    const uint8_t * data, size_t data_size)
{
	int i;
	unsigned size;

	if (get_certificate_type(session, GNUTLS_CTYPE_CLIENT) != GNUTLS_CRT_X509)
		return 0;

	/* put the requested DNs to req_dn, only in case
	 * of X509 certificates.
	 */
	if (issuers_len > 0) {

		for (i = 0; i < issuers_len; i++) {
			/* The checks here for the buffer boundaries
			 * are not needed since the buffer has been
			 * parsed above.
			 */
			data_size -= 2;

			size = _gnutls_read_uint16(data);

			data += 2;

			issuers_dn[i].data = (void*)data;
			issuers_dn[i].size = size;

			_gnutls_dn_log("Peer requested CA", &issuers_dn[i]);

			data += size;
		}
	}

	return 0;
}

/* Calls the client or server certificate get callback.
 */
static int
call_get_cert_callback(gnutls_session_t session,
		       const gnutls_datum_t * issuers_dn,
		       int issuers_dn_length,
		       gnutls_pk_algorithm_t * pk_algos, int pk_algos_length)
{
	gnutls_privkey_t local_key = NULL;
	int ret = GNUTLS_E_INTERNAL_ERROR;
	gnutls_certificate_type_t type;
	gnutls_certificate_credentials_t cred;
	gnutls_pcert_st *pcert = NULL;
	gnutls_ocsp_data_st *ocsp = NULL;
	unsigned int ocsp_length = 0;
	unsigned int pcert_length = 0;

	cred = (gnutls_certificate_credentials_t)
	    _gnutls_get_cred(session, GNUTLS_CRD_CERTIFICATE);
	if (cred == NULL) {
		gnutls_assert();
		return GNUTLS_E_INSUFFICIENT_CREDENTIALS;
	}

	/* Correctly set the certificate type for ourselves */
	type = get_certificate_type(session, GNUTLS_CTYPE_OURS);

	/* Check whether a callback is set and call it */
	if (cred->get_cert_callback3) {
		struct gnutls_cert_retr_st info;
		unsigned int flags = 0;

		memset(&info, 0, sizeof(info));
		info.req_ca_rdn = issuers_dn;
		info.nreqs = issuers_dn_length;
		info.pk_algos = pk_algos;
		info.pk_algos_length = pk_algos_length;
		info.cred = cred;

		/* we avoid all allocations and transformations */
		ret =
		    cred->get_cert_callback3(session, &info,
					     &pcert, &pcert_length,
					     &ocsp, &ocsp_length,
					     &local_key, &flags);
		if (ret < 0)
			return gnutls_assert_val(GNUTLS_E_USER_ERROR);

		if (pcert_length > 0 && type != pcert[0].type)
			return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

		if (pcert_length == 0) {
			pcert = NULL;
			local_key = NULL;
		}

		selected_certs_set(session, pcert, pcert_length,
				   ocsp, ocsp_length,
				   local_key, (flags&GNUTLS_CERT_RETR_DEINIT_ALL)?1:0,
				   cred->glob_ocsp_func, cred->glob_ocsp_func_ptr);

		return 0;
	} else {
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);
	}
}

/* Finds the appropriate certificate depending on the cA Distinguished name
 * advertized by the server. If none matches then returns 0 and -1 as index.
 * In case of an error a negative error code, is returned.
 *
 * 20020128: added ability to select a certificate depending on the SIGN
 * algorithm (only in automatic mode).
 */
int
_gnutls_select_client_cert(gnutls_session_t session,
			   const uint8_t * _data, size_t _data_size,
			   gnutls_pk_algorithm_t * pk_algos, int pk_algos_length)
{
	int result;
	int indx = -1;
	gnutls_certificate_credentials_t cred;
	const uint8_t *data = _data;
	ssize_t data_size = _data_size;
	int issuers_dn_length;
	gnutls_datum_t *issuers_dn = NULL;
	gnutls_certificate_type_t cert_type;

	cred = (gnutls_certificate_credentials_t)
	    _gnutls_get_cred(session, GNUTLS_CRD_CERTIFICATE);
	if (cred == NULL) {
		gnutls_assert();
		return GNUTLS_E_INSUFFICIENT_CREDENTIALS;
	}

	cert_type = get_certificate_type(session, GNUTLS_CTYPE_CLIENT);

	if (cred->get_cert_callback3 != NULL) {

		/* use a callback to get certificate
		 */
		if (cert_type == GNUTLS_CRT_X509) {
			issuers_dn_length =
			    get_issuers_num(session, data, data_size);
			if (issuers_dn_length < 0) {
				gnutls_assert();
				return issuers_dn_length;
			}

			if (issuers_dn_length > 0) {
				issuers_dn =
				    gnutls_malloc(sizeof(gnutls_datum_t) *
						  issuers_dn_length);
				if (issuers_dn == NULL) {
					gnutls_assert();
					return GNUTLS_E_MEMORY_ERROR;
				}

				result =
				    get_issuers(session, issuers_dn,
						issuers_dn_length, data,
						data_size);
				if (result < 0) {
					gnutls_assert();
					goto cleanup;
				}
			}
		} else {
			issuers_dn_length = 0;
		}

		result =
		    call_get_cert_callback(session, issuers_dn,
					   issuers_dn_length, pk_algos,
					   pk_algos_length);
		goto cleanup;

	} else {
		/* If we have no callbacks, try to guess.
		 */
		switch (cert_type) {
			case GNUTLS_CRT_X509:
				result = find_x509_client_cert(session, cred, _data,
										_data_size, pk_algos,
										pk_algos_length, &indx);
				break;
			case GNUTLS_CRT_RAWPK:
				result = find_rawpk_client_cert(session, cred,
							pk_algos, pk_algos_length, &indx);
				break;
			default:
				result = GNUTLS_E_UNSUPPORTED_CERTIFICATE_TYPE;
				break;
		}

		if (result < 0) {
			return gnutls_assert_val(result);
		}

		if (indx >= 0) {
			selected_certs_set(session,
					   &cred->certs[indx].
					   cert_list[0],
					   cred->certs[indx].
					   cert_list_length,
					   cred->certs[indx].ocsp_data,
					   cred->certs[indx].ocsp_data_length,
					   cred->certs[indx].pkey, 0,
					   NULL, NULL);
		} else {
			selected_certs_set(session, NULL, 0, NULL, 0,
					   NULL, 0, NULL, NULL);
		}

		result = 0;
	}

 cleanup:
	gnutls_free(issuers_dn);
	return result;

}

/* Generate certificate message
 */
static int gen_x509_crt(gnutls_session_t session, gnutls_buffer_st * data)
{
	int ret, i;
	gnutls_pcert_st *apr_cert_list;
	gnutls_privkey_t apr_pkey;
	int apr_cert_list_length;
	unsigned init_pos = data->length;

	/* find the appropriate certificate
	 */
	if ((ret =
	     _gnutls_get_selected_cert(session, &apr_cert_list,
				       &apr_cert_list_length, &apr_pkey)) < 0) {
		gnutls_assert();
		return ret;
	}

	ret = 3;
	for (i = 0; i < apr_cert_list_length; i++) {
		ret += apr_cert_list[i].cert.size + 3;
		/* hold size
		 * for uint24 */
	}

	/* if no certificates were found then send:
	 * 0B 00 00 03 00 00 00    // Certificate with no certs
	 * instead of:
	 * 0B 00 00 00	  // empty certificate handshake
	 *
	 * ( the above is the whole handshake message, not
	 * the one produced here )
	 */

	ret = _gnutls_buffer_append_prefix(data, 24, ret - 3);
	if (ret < 0)
		return gnutls_assert_val(ret);

	for (i = 0; i < apr_cert_list_length; i++) {
		ret =
		    _gnutls_buffer_append_data_prefix(data, 24,
						      apr_cert_list[i].
						      cert.data,
						      apr_cert_list[i].
						      cert.size);
		if (ret < 0)
			return gnutls_assert_val(ret);
	}

	return data->length - init_pos;
}


/* Generates a Raw Public Key certificate message that holds only the
 * SubjectPublicKeyInfo part of a regular certificate message.
 *
 * Returns the number of bytes sent or a negative error code.
 */
int
_gnutls_gen_rawpk_crt(gnutls_session_t session, gnutls_buffer_st* data)
{
	int ret;
	gnutls_pcert_st *apr_cert_list;
	gnutls_privkey_t apr_pkey;
	int apr_cert_list_length;

	if((ret = _gnutls_get_selected_cert(session, &apr_cert_list,
				       &apr_cert_list_length, &apr_pkey)) < 0)	{
			return gnutls_assert_val(ret);
	}

	/* Since we are transmitting a raw public key with no additional
	 * certificate credentials attached to it, it doesn't make sense to
	 * have more than one certificate set (i.e. to have a certificate chain).
	 */
	assert(apr_cert_list_length <= 1);

	/* Write our certificate containing only the SubjectPublicKeyInfo to
	 * the output buffer. We always have exactly one certificate that
	 * contains our raw public key. Our message looks like:
	 * <length++certificate> where
	 * length = 3 bytes (or 24 bits) and
	 * certificate = length bytes.
	 */
	if (apr_cert_list_length == 0) {
		ret = _gnutls_buffer_append_prefix(data, 24, 0);
	} else {
		ret = _gnutls_buffer_append_data_prefix(data, 24,
							apr_cert_list[0].cert.data,
							apr_cert_list[0].cert.size);
	}


	if (ret < 0) return gnutls_assert_val(ret);

	return data->length;
}


int
_gnutls_gen_cert_client_crt(gnutls_session_t session, gnutls_buffer_st * data)
{
	gnutls_certificate_type_t cert_type;

	// Retrieve the (negotiated) certificate type for the client
	cert_type = get_certificate_type(session, GNUTLS_CTYPE_CLIENT);

	switch (cert_type) {
		case GNUTLS_CRT_X509:
			return gen_x509_crt(session, data);
		case GNUTLS_CRT_RAWPK:
			return _gnutls_gen_rawpk_crt(session, data);
		default:
			return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);
	}
}

int
_gnutls_gen_cert_server_crt(gnutls_session_t session, gnutls_buffer_st * data)
{
	gnutls_certificate_type_t cert_type;

	// Retrieve the (negotiated) certificate type for the server
	cert_type = get_certificate_type(session, GNUTLS_CTYPE_SERVER);

	switch (cert_type) {
		case GNUTLS_CRT_X509:
			return gen_x509_crt(session, data);
		case GNUTLS_CRT_RAWPK:
			return _gnutls_gen_rawpk_crt(session, data);
		default:
			return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);
	}
}

static
int check_pk_compat(gnutls_session_t session, gnutls_pubkey_t pubkey)
{
	unsigned cert_pk;
	unsigned kx;

	if (session->security_parameters.entity != GNUTLS_CLIENT)
		return 0;

	cert_pk = gnutls_pubkey_get_pk_algorithm(pubkey, NULL);
	if (cert_pk == GNUTLS_PK_UNKNOWN) {
		gnutls_assert();
		return GNUTLS_E_CERTIFICATE_ERROR;
	}

	kx = session->security_parameters.cs->kx_algorithm;

	if (_gnutls_map_kx_get_cred(kx, 1) == GNUTLS_CRD_CERTIFICATE &&
	    !_gnutls_kx_supports_pk(kx, cert_pk)) {
		gnutls_assert();
		return GNUTLS_E_CERTIFICATE_ERROR;
	}

	return 0;
}

/* Process server certificate
 */
#define CLEAR_CERTS for(x=0;x<peer_certificate_list_size;x++) gnutls_pcert_deinit(&peer_certificate_list[x])
static int
_gnutls_proc_x509_crt(gnutls_session_t session,
			     uint8_t * data, size_t data_size)
{
	int size, len, ret;
	uint8_t *p = data;
	cert_auth_info_t info;
	gnutls_certificate_credentials_t cred;
	ssize_t dsize = data_size;
	int i;
	gnutls_pcert_st *peer_certificate_list;
	size_t peer_certificate_list_size = 0, j, x;
	gnutls_datum_t tmp;

	cred = (gnutls_certificate_credentials_t)
	    _gnutls_get_cred(session, GNUTLS_CRD_CERTIFICATE);
	if (cred == NULL) {
		gnutls_assert();
		return GNUTLS_E_INSUFFICIENT_CREDENTIALS;
	}

	if ((ret =
	     _gnutls_auth_info_init(session, GNUTLS_CRD_CERTIFICATE,
				   sizeof(cert_auth_info_st), 1)) < 0) {
		gnutls_assert();
		return ret;
	}

	info = _gnutls_get_auth_info(session, GNUTLS_CRD_CERTIFICATE);

	if (data == NULL || data_size == 0) {
		gnutls_assert();
		/* no certificate was sent */
		return GNUTLS_E_NO_CERTIFICATE_FOUND;
	}

	DECR_LEN(dsize, 3);
	size = _gnutls_read_uint24(p);
	p += 3;

	/* ensure no discrepancy in data */
	if (size != dsize)
		return gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET_LENGTH);

	/* some implementations send 0B 00 00 06 00 00 03 00 00 00
	 * instead of just 0B 00 00 03 00 00 00 as an empty certificate message.
	 */
	if (size == 0 || (size == 3 && memcmp(p, "\x00\x00\x00", 3) == 0)) {
		gnutls_assert();
		/* no certificate was sent */
		return GNUTLS_E_NO_CERTIFICATE_FOUND;
	}

	i = dsize;
	while (i > 0) {
		DECR_LEN(dsize, 3);
		len = _gnutls_read_uint24(p);
		p += 3;
		DECR_LEN(dsize, len);
		peer_certificate_list_size++;
		p += len;
		i -= len + 3;
	}

	if (dsize != 0)
		return gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET_LENGTH);

	if (peer_certificate_list_size == 0) {
		gnutls_assert();
		return GNUTLS_E_NO_CERTIFICATE_FOUND;
	}

	/* Ok we now allocate the memory to hold the
	 * certificate list
	 */

	peer_certificate_list =
	    gnutls_calloc(1,
			  sizeof(gnutls_pcert_st) *
			  (peer_certificate_list_size));
	if (peer_certificate_list == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	p = data + 3;

	/* Now we start parsing the list (again).
	 * We don't use DECR_LEN since the list has
	 * been parsed before.
	 */

	for (j = 0; j < peer_certificate_list_size; j++) {
		len = _gnutls_read_uint24(p);
		p += 3;

		tmp.size = len;
		tmp.data = p;

		ret =
		    gnutls_pcert_import_x509_raw(&peer_certificate_list
						 [j], &tmp,
						 GNUTLS_X509_FMT_DER, 0);
		if (ret < 0) {
			gnutls_assert();
			peer_certificate_list_size = j;
			ret = GNUTLS_E_CERTIFICATE_ERROR;
			goto cleanup;
		}

		p += len;
	}

	ret = check_pk_compat(session, peer_certificate_list[0].pubkey);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret =
	     _gnutls_pcert_to_auth_info(info,
					peer_certificate_list,
					peer_certificate_list_size);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	return 0;

 cleanup:
	CLEAR_CERTS;
	gnutls_free(peer_certificate_list);
	return ret;

}


int _gnutls_proc_rawpk_crt(gnutls_session_t session,
				uint8_t * data, size_t data_size)
{
	int cert_size, ret;
	cert_auth_info_t info;
	gnutls_pcert_st* peer_certificate;
	gnutls_datum_t tmp_cert;

	uint8_t *p = data;
	ssize_t dsize = data_size;

	/* We assume data != null and data_size > 0 because
	 * the caller checks this for us. */

	/* Read the length of our certificate. We always have exactly
	 * one certificate that contains our raw public key. Our message
	 * looks like:
	 * <length++certificate> where
	 * length = 3 bytes and
	 * certificate = length bytes.
	 */
	DECR_LEN(dsize, 3);
	cert_size = _gnutls_read_uint24(p);
	p += 3;

	/* Ensure no discrepancy in data */
	if (cert_size != dsize)
		return gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET_LENGTH);


	if (cert_size == 0)	{
		// No certificate was sent. This is not OK.
		return gnutls_assert_val(GNUTLS_E_NO_CERTIFICATE_FOUND);
	}

	DECR_LEN_FINAL(dsize, cert_size);

	/* We are now going to read our certificate and store it into
	 * the authentication info structure.
	 */
	tmp_cert.size = cert_size;
	tmp_cert.data = p;

	peer_certificate = gnutls_calloc(1, sizeof(*peer_certificate));
	if (peer_certificate == NULL) {
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);
	}

	// Import our raw certificate holding only a raw public key into this pcert
	ret = gnutls_pcert_import_rawpk_raw(peer_certificate, &tmp_cert, GNUTLS_X509_FMT_DER, 0, 0);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	// Check whether the PK algo is compatible with the negotiated KX
	ret = check_pk_compat(session, peer_certificate->pubkey);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = _gnutls_auth_info_init(session, GNUTLS_CRD_CERTIFICATE,
															sizeof(cert_auth_info_st), 1);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	info = _gnutls_get_auth_info(session, GNUTLS_CRD_CERTIFICATE);

	/* Copy our imported certificate into the auth info structure
	 * and free our temporary cert storage peer_certificate.
	 */
	ret = _gnutls_pcert_to_auth_info(info, peer_certificate, 1);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	return GNUTLS_E_SUCCESS;

cleanup:
	if (peer_certificate != NULL) {
		gnutls_pcert_deinit(peer_certificate);
		gnutls_free(peer_certificate);
	}

	return ret;
}


int _gnutls_proc_crt(gnutls_session_t session, uint8_t * data, size_t data_size)
{
	gnutls_certificate_credentials_t cred;
	gnutls_certificate_type_t cert_type;

	cred =
	    (gnutls_certificate_credentials_t) _gnutls_get_cred(session,
								GNUTLS_CRD_CERTIFICATE);
	if (cred == NULL) {
		gnutls_assert();
		return GNUTLS_E_INSUFFICIENT_CREDENTIALS;
	}

	/* Determine what certificate type we need to process.
	 * We need to process the certificate of the peer. */
	cert_type = get_certificate_type(session, GNUTLS_CTYPE_PEERS);

	switch (cert_type) {
		case GNUTLS_CRT_X509:
			return _gnutls_proc_x509_crt(session, data, data_size);
		case GNUTLS_CRT_RAWPK:
			return _gnutls_proc_rawpk_crt(session, data, data_size);
		default:
			return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);
	}
}

/* Checks if we support the given signature algorithm
 * (RSA or DSA). Returns the corresponding gnutls_pk_algorithm_t
 * if true;
 */
inline static int _gnutls_check_supported_sign_algo(CertificateSigType algo)
{
	switch (algo) {
	case RSA_SIGN:
		return GNUTLS_PK_RSA;
	case DSA_SIGN:
		return GNUTLS_PK_DSA;
	case ECDSA_SIGN:
		return GNUTLS_PK_EC;
	}

	return -1;
}

int
_gnutls_proc_cert_cert_req(gnutls_session_t session, uint8_t * data,
			   size_t data_size)
{
	int size, ret;
	uint8_t *p;
	gnutls_certificate_credentials_t cred;
	ssize_t dsize;
	int i;
	gnutls_pk_algorithm_t pk_algos[MAX_CLIENT_SIGN_ALGOS];
	int pk_algos_length;
	const version_entry_st *ver = get_version(session);

	if (unlikely(ver == NULL))
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

	cred = (gnutls_certificate_credentials_t)
	    _gnutls_get_cred(session, GNUTLS_CRD_CERTIFICATE);
	if (cred == NULL) {
		gnutls_assert();
		return GNUTLS_E_INSUFFICIENT_CREDENTIALS;
	}

	if ((ret =
	     _gnutls_auth_info_init(session, GNUTLS_CRD_CERTIFICATE,
				   sizeof(cert_auth_info_st), 0)) < 0) {
		gnutls_assert();
		return ret;
	}

	p = data;
	dsize = data_size;

	DECR_LEN(dsize, 1);
	size = p[0];
	p++;
	/* check if the sign algorithm is supported.
	 */
	pk_algos_length = 0;
	for (i = 0; i < size; i++, p++) {
		DECR_LEN(dsize, 1);
		if ((ret = _gnutls_check_supported_sign_algo(*p)) > 0) {
			if (pk_algos_length < MAX_CLIENT_SIGN_ALGOS) {
				pk_algos[pk_algos_length++] = ret;
			}
		}
	}

	if (pk_algos_length == 0) {
		gnutls_assert();
		return GNUTLS_E_UNKNOWN_PK_ALGORITHM;
	}

	if (_gnutls_version_has_selectable_sighash(ver)) {
		/* read supported hashes */
		int hash_num;
		DECR_LEN(dsize, 2);
		hash_num = _gnutls_read_uint16(p);
		p += 2;
		DECR_LEN(dsize, hash_num);

		ret = _gnutls_sign_algorithm_parse_data(session, p, hash_num);
		if (ret < 0) {
			gnutls_assert();
			return ret;
		}

		p += hash_num;
	}

	/* read the certificate authorities */
	DECR_LEN(dsize, 2);
	size = _gnutls_read_uint16(p);
	p += 2;

	DECR_LEN_FINAL(dsize, size);

	/* We should reply with a certificate message,
	 * even if we have no certificate to send.
	 */
	session->internals.hsk_flags |= HSK_CRT_ASKED;

	/* now we ask the user to tell which one
	 * he wants to use.
	 */
	if ((ret =
	     _gnutls_select_client_cert(session, p, size, pk_algos,
					pk_algos_length)) < 0) {
		gnutls_assert();
		return ret;
	}

	return 0;
}

int
_gnutls_gen_cert_client_crt_vrfy(gnutls_session_t session,
				 gnutls_buffer_st * data)
{
	int ret;
	gnutls_pcert_st *apr_cert_list;
	gnutls_privkey_t apr_pkey;
	int apr_cert_list_length;
	gnutls_datum_t signature = { NULL, 0 };
	gnutls_sign_algorithm_t sign_algo;
	const version_entry_st *ver = get_version(session);
	unsigned init_pos = data->length;

	if (unlikely(ver == NULL))
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

	/* find the appropriate certificate */
	if ((ret =
	     _gnutls_get_selected_cert(session, &apr_cert_list,
				       &apr_cert_list_length, &apr_pkey)) < 0) {
		gnutls_assert();
		return ret;
	}

	if (apr_cert_list_length > 0) {
		if ((ret =
		     _gnutls_handshake_sign_crt_vrfy(session,
						     &apr_cert_list[0],
						     apr_pkey,
						     &signature)) < 0) {
			gnutls_assert();
			return ret;
		}
		sign_algo = ret;
	} else {
		return 0;
	}

	if (_gnutls_version_has_selectable_sighash(ver)) {
		const sign_algorithm_st *aid;
		uint8_t p[2];
		/* error checking is not needed here since we have used those algorithms */
		aid = _gnutls_sign_to_tls_aid(sign_algo);
		if (aid == NULL)
			return gnutls_assert_val(GNUTLS_E_UNKNOWN_ALGORITHM);

		p[0] = aid->id[0];
		p[1] = aid->id[1];
		ret = _gnutls_buffer_append_data(data, p, 2);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}
	}

	ret =
	    _gnutls_buffer_append_data_prefix(data, 16, signature.data,
					      signature.size);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = data->length - init_pos;

 cleanup:
	_gnutls_free_datum(&signature);
	return ret;
}

int
_gnutls_proc_cert_client_crt_vrfy(gnutls_session_t session,
				  uint8_t * data, size_t data_size)
{
	int size, ret;
	ssize_t dsize = data_size;
	uint8_t *pdata = data;
	gnutls_datum_t sig;
	cert_auth_info_t info = _gnutls_get_auth_info(session, GNUTLS_CRD_CERTIFICATE);
	gnutls_pcert_st peer_cert;
	gnutls_sign_algorithm_t sign_algo = GNUTLS_SIGN_UNKNOWN;
	const version_entry_st *ver = get_version(session);
	gnutls_certificate_credentials_t cred;
	unsigned vflags;

	if (unlikely(info == NULL || info->ncerts == 0 || ver == NULL)) {
		gnutls_assert();
		/* we need this in order to get peer's certificate */
		return GNUTLS_E_INTERNAL_ERROR;
	}

	cred = (gnutls_certificate_credentials_t)
	    _gnutls_get_cred(session, GNUTLS_CRD_CERTIFICATE);
	if (cred == NULL) {
		gnutls_assert();
		return GNUTLS_E_INSUFFICIENT_CREDENTIALS;
	}

	vflags = cred->verify_flags | session->internals.additional_verify_flags;

	if (_gnutls_version_has_selectable_sighash(ver)) {
		DECR_LEN(dsize, 2);

		sign_algo = _gnutls_tls_aid_to_sign(pdata[0], pdata[1], ver);
		if (sign_algo == GNUTLS_SIGN_UNKNOWN) {
			gnutls_assert();
			return GNUTLS_E_UNSUPPORTED_SIGNATURE_ALGORITHM;
		}
		pdata += 2;
	}

	ret = _gnutls_session_sign_algo_enabled(session, sign_algo);
	if (ret < 0)
		return gnutls_assert_val(GNUTLS_E_UNSUPPORTED_SIGNATURE_ALGORITHM);

	DECR_LEN(dsize, 2);
	size = _gnutls_read_uint16(pdata);
	pdata += 2;

	DECR_LEN_FINAL(dsize, size);

	sig.data = pdata;
	sig.size = size;

	ret = _gnutls_get_auth_info_pcert(&peer_cert,
					  session->security_parameters.
					  client_ctype, info);

	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	if ((ret =
	     _gnutls_handshake_verify_crt_vrfy(session, vflags, &peer_cert, &sig,
					       sign_algo)) < 0) {
		gnutls_assert();
		gnutls_pcert_deinit(&peer_cert);
		return ret;
	}
	gnutls_pcert_deinit(&peer_cert);

	return 0;
}

int
_gnutls_gen_cert_server_cert_req(gnutls_session_t session,
				 gnutls_buffer_st * data)
{
	gnutls_certificate_credentials_t cred;
	int ret;
	uint8_t tmp_data[CERTTYPE_SIZE];
	const version_entry_st *ver = get_version(session);
	unsigned init_pos = data->length;

	if (unlikely(ver == NULL))
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

	/* Now we need to generate the RDN sequence. This is
	 * already in the CERTIFICATE_CRED structure, to improve
	 * performance.
	 */

	cred = (gnutls_certificate_credentials_t)
	    _gnutls_get_cred(session, GNUTLS_CRD_CERTIFICATE);
	if (cred == NULL) {
		gnutls_assert();
		return GNUTLS_E_INSUFFICIENT_CREDENTIALS;
	}

	tmp_data[0] = CERTTYPE_SIZE - 1;
	tmp_data[1] = RSA_SIGN;
	tmp_data[2] = DSA_SIGN;
	tmp_data[3] = ECDSA_SIGN;	/* only these for now */

	ret = _gnutls_buffer_append_data(data, tmp_data, CERTTYPE_SIZE);
	if (ret < 0)
		return gnutls_assert_val(ret);

	if (_gnutls_version_has_selectable_sighash(ver)) {
		ret =
		    _gnutls_sign_algorithm_write_params(session, data);
		if (ret < 0) {
			gnutls_assert();
			return ret;
		}
	}

	if (session->security_parameters.client_ctype == GNUTLS_CRT_X509 &&
	    session->internals.ignore_rdn_sequence == 0) {

		ret =
		    _gnutls_buffer_append_data_prefix(data, 16,
						      cred->
						      tlist->x509_rdn_sequence.
						      data,
						      cred->
						      tlist->x509_rdn_sequence.
						      size);
		if (ret < 0)
			return gnutls_assert_val(ret);
	} else {
		ret = _gnutls_buffer_append_prefix(data, 16, 0);
		if (ret < 0)
			return gnutls_assert_val(ret);
	}

	return data->length - init_pos;
}

/* This function will return the appropriate certificate to use.
 * Fills in the apr_cert_list, apr_cert_list_length and apr_pkey.
 * The return value is a negative error code on error.
 *
 * It is normal to return 0 with no certificates in client side.
 *
 */
int
_gnutls_get_selected_cert(gnutls_session_t session,
			  gnutls_pcert_st ** apr_cert_list,
			  int *apr_cert_list_length,
			  gnutls_privkey_t * apr_pkey)
{
	if (session->security_parameters.entity == GNUTLS_SERVER) {

		*apr_cert_list = session->internals.selected_cert_list;
		*apr_pkey = session->internals.selected_key;
		*apr_cert_list_length =
		    session->internals.selected_cert_list_length;

		if (*apr_cert_list_length == 0 || *apr_cert_list == NULL) {
			gnutls_assert();
			return GNUTLS_E_INSUFFICIENT_CREDENTIALS;
		}

	} else {		/* CLIENT SIDE */
		/* _gnutls_select_client_cert() must have been called before.
		 */
		*apr_cert_list = session->internals.selected_cert_list;
		*apr_cert_list_length =
		    session->internals.selected_cert_list_length;
		*apr_pkey = session->internals.selected_key;

	}

	return 0;
}


void _gnutls_selected_certs_deinit(gnutls_session_t session)
{
	if (session->internals.selected_need_free != 0) {
		int i;

		for (i = 0;
		     i < session->internals.selected_cert_list_length; i++) {
			gnutls_pcert_deinit(&session->internals.
					    selected_cert_list[i]);
		}
		gnutls_free(session->internals.selected_cert_list);

		for (i = 0;
		     i < session->internals.selected_ocsp_length; i++) {
			_gnutls_free_datum(&session->internals.
					   selected_ocsp[i].response);
		}
		gnutls_free(session->internals.selected_ocsp);

		gnutls_privkey_deinit(session->internals.selected_key);
	}
	session->internals.selected_ocsp_func = NULL;

	session->internals.selected_cert_list = NULL;
	session->internals.selected_cert_list_length = 0;

	session->internals.selected_key = NULL;

	return;
}

static void
selected_certs_set(gnutls_session_t session,
		   gnutls_pcert_st * certs, int ncerts,
		   gnutls_ocsp_data_st *ocsp, unsigned nocsp,
		   gnutls_privkey_t key, int need_free,
		   gnutls_status_request_ocsp_func ocsp_func,
		   void *ocsp_func_ptr)
{
	_gnutls_selected_certs_deinit(session);

	session->internals.selected_cert_list = certs;
	session->internals.selected_cert_list_length = ncerts;

	session->internals.selected_ocsp = ocsp;
	session->internals.selected_ocsp_length = nocsp;

	session->internals.selected_key = key;
	session->internals.selected_need_free = need_free;

	session->internals.selected_ocsp_func = ocsp_func;
	session->internals.selected_ocsp_func_ptr = ocsp_func_ptr;
}

static void get_server_name(gnutls_session_t session, uint8_t * name,
			    size_t max_name_size)
{
	int ret, i;
	size_t max_name;
	unsigned int type;

	ret = 0;
	for (i = 0; !(ret < 0); i++) {
		max_name = max_name_size;
		ret =
		    gnutls_server_name_get(session, name, &max_name, &type, i);
		if (ret >= 0 && type == GNUTLS_NAME_DNS)
			return;
	}

	name[0] = 0;

	return;
}

/* Checks the compatibility of the pubkey in the certificate with the
 * ciphersuite and selects a signature algorithm (if required by the
 * ciphersuite and TLS version) appropriate for the certificate. If none
 * can be selected returns an error.
 *
 * IMPORTANT
 * Currently this function is only called from _gnutls_select_server_cert,
 * i.e. it is only called at the server. We therefore retrieve the
 * negotiated server certificate type within this function.
 * If, in the future, this routine is called at the client then we
 * need to adapt the implementation accordingly.
 */
static
int cert_select_sign_algorithm(gnutls_session_t session,
			       gnutls_pcert_st * cert,
			       gnutls_privkey_t pkey,
			       const gnutls_cipher_suite_entry_st *cs)
{
	gnutls_pubkey_t pubkey = cert->pubkey;
	gnutls_certificate_type_t cert_type = cert->type;
	unsigned pk = pubkey->params.algo;
	unsigned key_usage;
	gnutls_sign_algorithm_t algo;
	const version_entry_st *ver = get_version(session);
	gnutls_certificate_type_t ctype;

	assert(IS_SERVER(session));

	/* Retrieve the server certificate type */
	ctype = get_certificate_type(session, GNUTLS_CTYPE_SERVER);

	if (ctype != cert_type) {
		return gnutls_assert_val(GNUTLS_E_INSUFFICIENT_CREDENTIALS);
	}

	key_usage = get_key_usage(session, pubkey);

	/* In TLS1.3 we support only signatures; ensure the selected key supports them */
	if (ver->tls13_sem && _gnutls_check_key_usage_for_sig(session, key_usage, 1) < 0)
		return gnutls_assert_val(GNUTLS_E_INSUFFICIENT_CREDENTIALS);

	if (!ver->tls13_sem && !_gnutls_kx_supports_pk_usage(cs->kx_algorithm, pk, key_usage)) {
		return gnutls_assert_val(GNUTLS_E_INSUFFICIENT_CREDENTIALS);
	}

	if (!ver->tls13_sem && _gnutls_kx_encipher_type(cs->kx_algorithm) != CIPHER_SIGN)
		return 0;

	if (!_gnutls_version_has_selectable_sighash(ver)) {
		/* For SSL3.0 and TLS1.0 we lie as we cannot express md5-sha1 as
		 * signature algorithm. */
		algo = gnutls_pk_to_sign(cert->pubkey->params.algo, GNUTLS_DIG_SHA1);
		gnutls_sign_algorithm_set_server(session, algo);
		return 0;
	}

	algo = _gnutls_session_get_sign_algo(session, cert, pkey, 0);
	if (algo == GNUTLS_SIGN_UNKNOWN)
		return gnutls_assert_val(GNUTLS_E_INCOMPATIBLE_SIG_WITH_KEY);

	gnutls_sign_algorithm_set_server(session, algo);
	_gnutls_handshake_log("Selected signature algorithm: %s\n", gnutls_sign_algorithm_get_name(algo));

	return 0;
}

/* finds the most appropriate certificate in the cert list.
 * The 'appropriate' is defined by the user.
 *
 * requested_algo holds the parameters required by the peer (RSA, DSA
 * or -1 for any).
 *
 * Returns 0 on success and a negative error code on error. The
 * selected certificate will be in session->internals.selected_*.
 *
 */
int
_gnutls_select_server_cert(gnutls_session_t session, const gnutls_cipher_suite_entry_st *cs)
{
	unsigned i, j;
	int idx, ret;
	gnutls_certificate_credentials_t cred;
	char server_name[MAX_CN];

	cred = (gnutls_certificate_credentials_t)
	    _gnutls_get_cred(session, GNUTLS_CRD_CERTIFICATE);
	if (cred == NULL) {
		gnutls_assert(); /* we don't need to select a cert */
		return 0;
	}

	/* When a callback is set, we call it once to get the
	 * certificate and then check its compatibility with
	 * the ciphersuites.
	 */
	if (cred->get_cert_callback3) {
		if (session->internals.selected_cert_list_length == 0) {
			ret = call_get_cert_callback(session, NULL, 0, NULL, 0);
			if (ret < 0)
				return gnutls_assert_val(ret);

			if (session->internals.selected_cert_list_length == 0)
				return gnutls_assert_val(GNUTLS_E_INSUFFICIENT_CREDENTIALS);

			_gnutls_debug_log("Selected (%s) cert\n",
					  gnutls_pk_get_name(session->internals.selected_cert_list[0].pubkey->params.algo));
		}

		ret = cert_select_sign_algorithm(session,
						 &session->internals.selected_cert_list[0],
						 session->internals.selected_key,
						 cs);
		if (ret < 0)
			return gnutls_assert_val(ret);

		return 0;
	}

	/* Otherwise... we check the compatibility of the ciphersuite
	 * with all the certificates available. */

	get_server_name(session, (unsigned char *)server_name,
			sizeof(server_name));

	_gnutls_handshake_log ("HSK[%p]: Requested server name: '%s'\n",
				     session, server_name);
	idx = -1;		/* default is use no certificate */

	/* find certificates that match the requested server_name
	 */

	if (server_name[0] != 0) {
		for (j = 0; j < cred->ncerts; j++) {
			i = cred->sorted_cert_idx[j];

			if (cred->certs[i].names != NULL
			    && _gnutls_str_array_match(cred->certs[i].names,
						       server_name) != 0) {
				/* if requested algorithms are also compatible select it */

				ret = cert_select_sign_algorithm(session,
								 &cred->certs[i].cert_list[0],
								 cred->certs[i].pkey,
								 cs);
				if (ret >= 0) {
					idx = i;
					_gnutls_debug_log("Selected (%s) cert based on ciphersuite %x.%x: %s\n",
						  gnutls_pk_get_name(cred->certs[i].cert_list[0].pubkey->params.algo),
						  (unsigned)cs->id[0],
						  (unsigned)cs->id[1],
						  cs->name);
					/* found */
					goto finished;
				}
			}
		}
	}

	/* no name match */
	for (j = 0; j < cred->ncerts; j++) {
		i = cred->sorted_cert_idx[j];

		_gnutls_handshake_log
		    ("HSK[%p]: checking compat of %s with certificate[%d] (%s/%s)\n",
		     session, cs->name, i,
		     gnutls_pk_get_name(cred->certs[i].cert_list[0].pubkey->
					params.algo),
		     gnutls_certificate_type_get_name(cred->certs[i].
						      cert_list[0].type));

		ret = cert_select_sign_algorithm(session,
						 &cred->certs[i].cert_list[0],
						 cred->certs[i].pkey,
						 cs);
		if (ret >= 0) {
			idx = i;
			_gnutls_debug_log("Selected (%s) cert based on ciphersuite %x.%x: %s\n",
					  gnutls_pk_get_name(cred->certs[i].cert_list[0].pubkey->params.algo),
					  (unsigned)cs->id[0],
					  (unsigned)cs->id[1],
					  cs->name);
			/* found */
			goto finished;
		}
	}

	/* store the certificate pointer for future use, in the handshake.
	 * (This will allow not calling this callback again.)
	 */
 finished:
	if (idx >= 0) {
		gnutls_status_request_ocsp_func ocsp_func = NULL;
		void *ocsp_ptr = NULL;
		gnutls_ocsp_data_st *ocsp = NULL;
		unsigned nocsp = 0;

		if (cred->certs[idx].ocsp_data_length > 0) {
			ocsp = &cred->certs[idx].ocsp_data[0];
			nocsp = cred->certs[idx].ocsp_data_length;
		} else if (cred->glob_ocsp_func != NULL) {
			ocsp_func = cred->glob_ocsp_func;
			ocsp_ptr = cred->glob_ocsp_func_ptr;
		} else if (cred->certs[idx].ocsp_func != NULL) {
			ocsp_func = cred->certs[idx].ocsp_func;
			ocsp_ptr = cred->certs[idx].ocsp_func_ptr;
		}

		selected_certs_set(session,
				   &cred->certs[idx].cert_list[0],
				   cred->certs[idx].cert_list_length,
				   ocsp, nocsp,
				   cred->certs[idx].pkey, 0,
				   ocsp_func,
				   ocsp_ptr);
	} else {
		/* Certificate does not support REQUESTED_ALGO.  */
		return gnutls_assert_val(GNUTLS_E_INSUFFICIENT_CREDENTIALS);
	}

	return 0;
}

int _gnutls_gen_dhe_signature(gnutls_session_t session,
			      gnutls_buffer_st * data, uint8_t * plain,
			      unsigned plain_size)
{
	gnutls_pcert_st *apr_cert_list;
	gnutls_privkey_t apr_pkey;
	int apr_cert_list_length;
	gnutls_datum_t signature = { NULL, 0 }, ddata;
	gnutls_sign_algorithm_t sign_algo;
	const version_entry_st *ver = get_version(session);
	int ret;

	if (unlikely(ver == NULL))
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

	ddata.data = plain;
	ddata.size = plain_size;

	/* find the appropriate certificate */
	if ((ret =
	     _gnutls_get_selected_cert(session, &apr_cert_list,
				       &apr_cert_list_length, &apr_pkey)) < 0) {
		gnutls_assert();
		return ret;
	}

	if (apr_cert_list_length > 0) {
		if ((ret =
		     _gnutls_handshake_sign_data(session,
						 &apr_cert_list[0],
						 apr_pkey, &ddata,
						 &signature, &sign_algo)) < 0) {
			gnutls_assert();
			goto cleanup;
		}
	} else {
		gnutls_assert();
		ret = 0;	/* ANON-DH, do not put a signature - ILLEGAL! */
		goto cleanup;
	}

	if (_gnutls_version_has_selectable_sighash(ver)) {
		const sign_algorithm_st *aid;
		uint8_t p[2];

		if (sign_algo == GNUTLS_SIGN_UNKNOWN) {
			ret = GNUTLS_E_UNKNOWN_ALGORITHM;
			goto cleanup;
		}

		aid = _gnutls_sign_to_tls_aid(sign_algo);
		if (aid == NULL) {
			gnutls_assert();
			ret = GNUTLS_E_UNKNOWN_ALGORITHM;
			goto cleanup;
		}

		p[0] = aid->id[0];
		p[1] = aid->id[1];

		ret = _gnutls_buffer_append_data(data, p, 2);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}
	}

	ret =
	    _gnutls_buffer_append_data_prefix(data, 16, signature.data,
					      signature.size);
	if (ret < 0) {
		gnutls_assert();
	}

	ret = 0;

 cleanup:
	_gnutls_free_datum(&signature);
	return ret;
}

int
_gnutls_proc_dhe_signature(gnutls_session_t session, uint8_t * data,
			   size_t _data_size, gnutls_datum_t * vparams)
{
	int sigsize;
	gnutls_datum_t signature;
	int ret;
	cert_auth_info_t info = _gnutls_get_auth_info(session, GNUTLS_CRD_CERTIFICATE);
	ssize_t data_size = _data_size;
	gnutls_pcert_st peer_cert;
	gnutls_sign_algorithm_t sign_algo = GNUTLS_SIGN_UNKNOWN;
	const version_entry_st *ver = get_version(session);
	gnutls_certificate_credentials_t cred;
	unsigned vflags;
	gnutls_certificate_type_t cert_type;

	if (unlikely(info == NULL || info->ncerts == 0 || ver == NULL)) {
		gnutls_assert();
		/* we need this in order to get peer's certificate */
		return GNUTLS_E_INTERNAL_ERROR;
	}

	cred = (gnutls_certificate_credentials_t)
	    _gnutls_get_cred(session, GNUTLS_CRD_CERTIFICATE);
	if (cred == NULL) {
		gnutls_assert();
		return GNUTLS_E_INSUFFICIENT_CREDENTIALS;
	}

	vflags = cred->verify_flags | session->internals.additional_verify_flags;

	/* VERIFY SIGNATURE */
	if (_gnutls_version_has_selectable_sighash(ver)) {
		uint8_t id[2];

		DECR_LEN(data_size, 1);
		id[0] = *data++;
		DECR_LEN(data_size, 1);
		id[1] = *data++;

		sign_algo = _gnutls_tls_aid_to_sign(id[0], id[1], ver);
		if (sign_algo == GNUTLS_SIGN_UNKNOWN) {
			_gnutls_debug_log("unknown signature %d.%d\n",
					  (int)id[0], (int)id[1]);
			gnutls_assert();
			return GNUTLS_E_UNSUPPORTED_SIGNATURE_ALGORITHM;
		}
	}
	DECR_LEN(data_size, 2);
	sigsize = _gnutls_read_uint16(data);
	data += 2;

	DECR_LEN_FINAL(data_size, sigsize);
	signature.data = data;
	signature.size = sigsize;

	// Retrieve the negotiated certificate type
	cert_type = get_certificate_type(session, GNUTLS_CTYPE_SERVER);

	if ((ret =
	     _gnutls_get_auth_info_pcert(&peer_cert, cert_type, info)) < 0) {
		gnutls_assert();
		return ret;
	}

	ret =
	    _gnutls_handshake_verify_data(session, vflags, &peer_cert, vparams,
					  &signature, sign_algo);

	gnutls_pcert_deinit(&peer_cert);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	return 0;
}
