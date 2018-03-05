/*
 * Copyright (C) 2001-2012 Free Software Foundation, Inc.
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
#include "debug.h"

#ifdef ENABLE_OPENPGP
#include "openpgp/openpgp.h"

static gnutls_privkey_t alloc_and_load_pgp_key(const
					       gnutls_openpgp_privkey_t
					       key, int deinit);
static gnutls_pcert_st *alloc_and_load_pgp_certs(gnutls_openpgp_crt_t cert);

#endif

static gnutls_pcert_st *alloc_and_load_x509_certs(gnutls_x509_crt_t *
						  certs, unsigned);
static gnutls_privkey_t alloc_and_load_x509_key(gnutls_x509_privkey_t key,
						int deinit);

#ifdef ENABLE_PKCS11
static gnutls_privkey_t alloc_and_load_pkcs11_key(gnutls_pkcs11_privkey_t
						  key, int deinit);
#endif

static void
_gnutls_selected_certs_set(gnutls_session_t session,
			   gnutls_pcert_st * certs, int ncerts,
			   gnutls_privkey_t key, int need_free,
			   gnutls_status_request_ocsp_func ocsp_func,
			   void *ocsp_func_ptr);

#define MAX_CLIENT_SIGN_ALGOS 3
#define CERTTYPE_SIZE (MAX_CLIENT_SIGN_ALGOS+1)
typedef enum CertificateSigType { RSA_SIGN = 1, DSA_SIGN = 2, ECDSA_SIGN = 64
} CertificateSigType;

/* Copies data from a internal certificate struct (gnutls_pcert_st) to 
 * exported certificate struct (cert_auth_info_t)
 */
static int copy_certificate_auth_info(cert_auth_info_t info, gnutls_pcert_st * certs, size_t ncerts,	/* openpgp only */
				      void *keyid)
{
	/* Copy peer's information to auth_info_t
	 */
	int ret;
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

	for (i = 0; i < ncerts; i++) {
		if (certs[i].cert.size > 0) {
			ret =
			    _gnutls_set_datum(&info->raw_certificate_list[i],
					      certs[i].cert.data,
					      certs[i].cert.size);
			if (ret < 0) {
				gnutls_assert();
				goto clear;
			}
		}
	}
	info->ncerts = ncerts;
	info->cert_type = certs[0].type;

#ifdef ENABLE_OPENPGP
	if (certs[0].type == GNUTLS_CRT_OPENPGP) {
		if (keyid)
			memcpy(info->subkey_id, keyid,
			       GNUTLS_OPENPGP_KEYID_SIZE);
	}
#endif

	return 0;

 clear:

	for (j = 0; j < i; j++)
		_gnutls_free_datum(&info->raw_certificate_list[j]);

	gnutls_free(info->raw_certificate_list);
	info->raw_certificate_list = NULL;

	return ret;
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
	       uint8_t * _data, size_t _data_size,
	       const gnutls_pk_algorithm_t * pk_algos,
	       int pk_algos_length, int *indx)
{
	unsigned size;
	gnutls_datum_t odn = { NULL, 0 };
	uint8_t *data = _data;
	ssize_t data_size = _data_size;
	unsigned i, j;
	int result, cert_pk;

	*indx = -1;

	/* If peer doesn't send any issuers and we have a single certificate
	 * then send that one.
	 */
	if (cred->ncerts == 1 &&
		(data_size == 0 || (session->internals.flags & GNUTLS_FORCE_CLIENT_CERT))) {
			*indx = 0;
			return 0;
	}

	do {
		DECR_LENGTH_RET(data_size, 2, 0);
		size = _gnutls_read_uint16(data);
		DECR_LENGTH_RET(data_size, size, 0);
		data += 2;

		for (i = 0; i < cred->ncerts; i++) {
			for (j = 0; j < cred->certs[i].cert_list_length; j++) {
				if ((result =
				     cert_get_issuer_dn(&cred->certs
							[i].cert_list
							[j], &odn)) < 0) {
					gnutls_assert();
					return result;
				}

				if (odn.size == 0 || odn.size != size)
					continue;

				/* If the DN matches and
				 * the *_SIGN algorithm matches
				 * the cert is our cert!
				 */
				cert_pk =
				    gnutls_pubkey_get_pk_algorithm(cred->certs
								   [i].cert_list
								   [0].pubkey,
								   NULL);

				if ((memcmp(odn.data, data, size) == 0) &&
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

#ifdef ENABLE_OPENPGP
/* Locates the most appropriate openpgp cert
 */
static int
find_openpgp_cert(const gnutls_certificate_credentials_t cred,
		  gnutls_pk_algorithm_t * pk_algos,
		  int pk_algos_length, int *indx)
{
	unsigned i, j;

	*indx = -1;

	for (i = 0; i < cred->ncerts; i++) {
		for (j = 0; j < cred->certs[i].cert_list_length; j++) {

			/* If the *_SIGN algorithm matches
			 * the cert is our cert!
			 */
			if ((check_pk_algo_in_list
			     (pk_algos, pk_algos_length,
			      gnutls_pubkey_get_pk_algorithm(cred->certs
							     [i].cert_list
							     [0].pubkey,
							     NULL)) == 0)
			    && (cred->certs[i].cert_list[0].type ==
				GNUTLS_CRT_OPENPGP)) {
				*indx = i;
				break;
			}
		}
		if (*indx != -1)
			break;
	}

	return 0;
}
#endif

/* Returns the number of issuers in the server's
 * certificate request packet.
 */
static int
get_issuers_num(gnutls_session_t session, uint8_t * data, ssize_t data_size)
{
	int issuers_dn_len = 0, result;
	unsigned size;

	/* Count the number of the given issuers;
	 * This is used to allocate the issuers_dn without
	 * using realloc().
	 */

	if (data_size == 0 || data == NULL)
		return 0;

	if (data_size > 0)
		do {
			/* This works like DECR_LEN() 
			 */
			result = GNUTLS_E_UNEXPECTED_PACKET_LENGTH;
			DECR_LENGTH_COM(data_size, 2, goto error);
			size = _gnutls_read_uint16(data);

			result = GNUTLS_E_UNEXPECTED_PACKET_LENGTH;
			DECR_LENGTH_COM(data_size, size, goto error);

			data += 2;

			if (size > 0) {
				issuers_dn_len++;
				data += size;
			}

			if (data_size == 0)
				break;

		}
		while (1);

	return issuers_dn_len;

 error:
	return result;
}

/* Returns the issuers in the server's certificate request
 * packet.
 */
static int
get_issuers(gnutls_session_t session,
	    gnutls_datum_t * issuers_dn, int issuers_len,
	    uint8_t * data, size_t data_size)
{
	int i;
	unsigned size;

	if (gnutls_certificate_type_get(session) != GNUTLS_CRT_X509)
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

			issuers_dn[i].data = data;
			issuers_dn[i].size = size;

			data += size;
		}
	}

	return 0;
}

/* Calls the client or server get callback.
 */
static int
call_get_cert_callback(gnutls_session_t session,
		       const gnutls_datum_t * issuers_dn,
		       int issuers_dn_length,
		       gnutls_pk_algorithm_t * pk_algos, int pk_algos_length)
{
	unsigned i;
	gnutls_pcert_st *local_certs = NULL;
	gnutls_privkey_t local_key = NULL;
	int ret = GNUTLS_E_INTERNAL_ERROR;
	gnutls_certificate_type_t type = gnutls_certificate_type_get(session);
	gnutls_certificate_credentials_t cred;
	gnutls_retr2_st st2;
	gnutls_pcert_st *pcert = NULL;
	unsigned int pcert_length = 0;

	cred = (gnutls_certificate_credentials_t)
	    _gnutls_get_cred(session, GNUTLS_CRD_CERTIFICATE);
	if (cred == NULL) {
		gnutls_assert();
		return GNUTLS_E_INSUFFICIENT_CREDENTIALS;
	}

	memset(&st2, 0, sizeof(st2));

	if (cred->get_cert_callback2) {
		/* we avoid all allocations and transformations */
		ret =
		    cred->get_cert_callback2(session, issuers_dn,
					     issuers_dn_length, pk_algos,
					     pk_algos_length, &pcert,
					     &pcert_length, &local_key);
		if (ret < 0)
			return gnutls_assert_val(GNUTLS_E_USER_ERROR);

		if (pcert_length > 0 && type != pcert[0].type)
			return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

		if (pcert_length == 0) {
			pcert = NULL;
			local_key = NULL;
		}

		_gnutls_selected_certs_set(session, pcert, pcert_length,
					   local_key, 0, NULL, NULL);

		return 0;

	} else if (cred->get_cert_callback) {
		ret =
		    cred->get_cert_callback(session, issuers_dn,
					    issuers_dn_length, pk_algos,
					    pk_algos_length, &st2);

	} else {		/* compatibility mode */
		gnutls_assert();
		return GNUTLS_E_INTERNAL_ERROR;
	}

	if (ret < 0) {
		gnutls_assert();
		return GNUTLS_E_USER_ERROR;
	}

	if (st2.ncerts == 0)
		return 0;	/* no certificate was selected */

	if (type != st2.cert_type) {
		gnutls_assert();
		ret = GNUTLS_E_INVALID_REQUEST;
		goto cleanup;
	}

	if (type == GNUTLS_CRT_X509) {
		local_certs =
		    alloc_and_load_x509_certs(st2.cert.x509, st2.ncerts);
	} else {		/* PGP */
		if (st2.ncerts > 1) {
			gnutls_assert();
			ret = GNUTLS_E_INVALID_REQUEST;
			goto cleanup;
		}
#ifdef ENABLE_OPENPGP
		{
			local_certs = alloc_and_load_pgp_certs(st2.cert.pgp);
		}
#else
		ret = GNUTLS_E_UNIMPLEMENTED_FEATURE;
		goto cleanup;
#endif
	}

	if (local_certs == NULL) {
		gnutls_assert();
		ret = GNUTLS_E_MEMORY_ERROR;
		goto cleanup;
	}

	switch (st2.key_type) {
	case GNUTLS_PRIVKEY_OPENPGP:
#ifdef ENABLE_OPENPGP
		if (st2.key.pgp != NULL) {
			local_key =
			    alloc_and_load_pgp_key(st2.key.pgp, st2.deinit_all);
			if (local_key == NULL) {
				gnutls_assert();
				ret = GNUTLS_E_INTERNAL_ERROR;
				goto cleanup;
			}
		}
#endif
		break;
	case GNUTLS_PRIVKEY_PKCS11:
#ifdef ENABLE_PKCS11
		if (st2.key.pkcs11 != NULL) {
			local_key =
			    alloc_and_load_pkcs11_key(st2.key.pkcs11,
						      st2.deinit_all);
			if (local_key == NULL) {
				gnutls_assert();
				ret = GNUTLS_E_INTERNAL_ERROR;
				goto cleanup;
			}
		}
#endif
		break;
	case GNUTLS_PRIVKEY_X509:
		if (st2.key.x509 != NULL) {
			local_key =
			    alloc_and_load_x509_key(st2.key.x509,
						    st2.deinit_all);
			if (local_key == NULL) {
				gnutls_assert();
				ret = GNUTLS_E_INTERNAL_ERROR;
				goto cleanup;
			}
		}
		break;
	default:
		gnutls_assert();
		ret = GNUTLS_E_INVALID_REQUEST;
		goto cleanup;
	}

	_gnutls_selected_certs_set(session, local_certs,
				   st2.ncerts, local_key, 1,
				   NULL, NULL);

	ret = 0;

 cleanup:

	if (st2.cert_type == GNUTLS_CRT_X509) {
		if (st2.deinit_all) {
			for (i = 0; i < st2.ncerts; i++) {
				gnutls_x509_crt_deinit(st2.cert.x509[i]);
			}
			gnutls_free(st2.cert.x509);
		}
	} else {
#ifdef ENABLE_OPENPGP
		if (st2.deinit_all) {
			gnutls_openpgp_crt_deinit(st2.cert.pgp);
		}
#endif
	}

	return ret;
}

/* Finds the appropriate certificate depending on the cA Distinguished name
 * advertized by the server. If none matches then returns 0 and -1 as index.
 * In case of an error a negative error code, is returned.
 *
 * 20020128: added ability to select a certificate depending on the SIGN
 * algorithm (only in automatic mode).
 */
static int
select_client_cert(gnutls_session_t session,
		   uint8_t * _data, size_t _data_size,
		   gnutls_pk_algorithm_t * pk_algos, int pk_algos_length)
{
	int result;
	int indx = -1;
	gnutls_certificate_credentials_t cred;
	uint8_t *data = _data;
	ssize_t data_size = _data_size;
	int issuers_dn_length;
	gnutls_datum_t *issuers_dn = NULL;

	cred = (gnutls_certificate_credentials_t)
	    _gnutls_get_cred(session, GNUTLS_CRD_CERTIFICATE);
	if (cred == NULL) {
		gnutls_assert();
		return GNUTLS_E_INSUFFICIENT_CREDENTIALS;
	}

	if (cred->get_cert_callback != NULL
	    || cred->get_cert_callback2 != NULL) {

		/* use a callback to get certificate 
		 */
		if (session->security_parameters.cert_type != GNUTLS_CRT_X509)
			issuers_dn_length = 0;
		else {
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
		}

		result =
		    call_get_cert_callback(session, issuers_dn,
					   issuers_dn_length, pk_algos,
					   pk_algos_length);
		goto cleanup;

	} else {
		/* If we have no callbacks, try to guess.
		 */
		result = 0;

		if (session->security_parameters.cert_type == GNUTLS_CRT_X509)
			result =
			    find_x509_client_cert(session, cred, _data, _data_size,
					   pk_algos, pk_algos_length, &indx);
#ifdef ENABLE_OPENPGP
		else if (session->security_parameters.cert_type ==
			 GNUTLS_CRT_OPENPGP)
			result =
			    find_openpgp_cert(cred, pk_algos,
					      pk_algos_length, &indx);
#endif

		if (result < 0) {
			gnutls_assert();
			return result;
		}

		if (indx >= 0) {
			_gnutls_selected_certs_set(session,
						   &cred->certs[indx].
						   cert_list[0],
						   cred->certs[indx].
						   cert_list_length,
						   cred->pkey[indx], 0,
						   NULL, NULL);
		} else {
			_gnutls_selected_certs_set(session, NULL, 0, NULL, 0,
						   NULL, NULL);
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

	return data->length;
}

enum PGPKeyDescriptorType
    { PGP_EMPTY_KEY = 1, PGP_KEY_SUBKEY, PGP_KEY_FINGERPRINT_SUBKEY };

#ifdef ENABLE_OPENPGP
static int
gen_openpgp_certificate(gnutls_session_t session, gnutls_buffer_st * data)
{
	int ret;
	gnutls_pcert_st *apr_cert_list;
	gnutls_privkey_t apr_pkey;
	int apr_cert_list_length;
	unsigned int subkey;
	uint8_t type;
	uint8_t fpr[GNUTLS_OPENPGP_V4_FINGERPRINT_SIZE];
	char buf[2 * GNUTLS_OPENPGP_KEYID_SIZE + 1];
	size_t fpr_size;

	/* find the appropriate certificate */
	if ((ret =
	     _gnutls_get_selected_cert(session, &apr_cert_list,
				       &apr_cert_list_length, &apr_pkey)) < 0) {
		gnutls_assert();
		return ret;
	}

	ret = 3 + 1 + 3;

	if (apr_cert_list_length > 0) {
		fpr_size = sizeof(fpr);
		ret =
		    gnutls_pubkey_get_openpgp_key_id(apr_cert_list[0].pubkey, 0,
						     fpr, &fpr_size, &subkey);
		if (ret < 0)
			return gnutls_assert_val(ret);

		ret += 1 + fpr_size;	/* for the keyid */
		_gnutls_handshake_log("Sending PGP key ID %s (%s)\n",
				      _gnutls_bin2hex(fpr,
						      GNUTLS_OPENPGP_KEYID_SIZE,
						      buf, sizeof(buf),
						      NULL),
				      subkey ? "subkey" : "master");

		ret += apr_cert_list[0].cert.size;
	}

	ret = _gnutls_buffer_append_prefix(data, 24, ret - 3);
	if (ret < 0)
		return gnutls_assert_val(ret);

	if (apr_cert_list_length > 0) {
		type = PGP_KEY_SUBKEY;

		ret = _gnutls_buffer_append_data(data, &type, 1);
		if (ret < 0)
			return gnutls_assert_val(ret);

		ret = _gnutls_buffer_append_data_prefix(data, 8, fpr, fpr_size);
		if (ret < 0)
			return gnutls_assert_val(ret);

		ret =
		    _gnutls_buffer_append_data_prefix(data, 24,
						      apr_cert_list[0].
						      cert.data,
						      apr_cert_list[0].
						      cert.size);
		if (ret < 0)
			return gnutls_assert_val(ret);
	} else {		/* empty - no certificate */

		type = PGP_EMPTY_KEY;

		ret = _gnutls_buffer_append_data(data, &type, 1);
		if (ret < 0)
			return gnutls_assert_val(ret);

		ret = _gnutls_buffer_append_prefix(data, 24, 0);
		if (ret < 0)
			return gnutls_assert_val(ret);
	}

	return data->length;
}

static int
gen_openpgp_certificate_fpr(gnutls_session_t session, gnutls_buffer_st * data)
{
	int ret, packet_size;
	uint8_t type, fpr[GNUTLS_OPENPGP_V4_FINGERPRINT_SIZE];
	uint8_t id[GNUTLS_OPENPGP_KEYID_SIZE];
	unsigned int subkey;
	size_t fpr_size, id_size;
	gnutls_pcert_st *apr_cert_list;
	gnutls_privkey_t apr_pkey;
	int apr_cert_list_length;

	/* find the appropriate certificate */
	if ((ret =
	     _gnutls_get_selected_cert(session, &apr_cert_list,
				       &apr_cert_list_length, &apr_pkey)) < 0) {
		gnutls_assert();
		return ret;
	}

	if (apr_cert_list_length <= 0)
		return gen_openpgp_certificate(session, data);

	id_size = sizeof(id);
	ret =
	    gnutls_pubkey_get_openpgp_key_id(apr_cert_list[0].pubkey, 0,
					     id, &id_size, &subkey);
	if (ret < 0)
		return gnutls_assert_val(ret);

	fpr_size = sizeof(fpr);
	ret =
	    gnutls_pubkey_get_openpgp_key_id(apr_cert_list[0].pubkey,
					     GNUTLS_PUBKEY_GET_OPENPGP_FINGERPRINT,
					     fpr, &fpr_size, NULL);
	if (ret < 0)
		return gnutls_assert_val(ret);

	packet_size = 3 + 1;
	packet_size += 1 + fpr_size;	/* for the keyid */

	/* Only v4 fingerprints are sent 
	 */
	packet_size += 20 + 1;

	ret = _gnutls_buffer_append_prefix(data, 24, packet_size - 3);
	if (ret < 0)
		return gnutls_assert_val(ret);

	type = PGP_KEY_FINGERPRINT_SUBKEY;
	ret = _gnutls_buffer_append_data(data, &type, 1);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = _gnutls_buffer_append_data_prefix(data, 8, id, id_size);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = _gnutls_buffer_append_data_prefix(data, 8, fpr, fpr_size);
	if (ret < 0)
		return gnutls_assert_val(ret);

	return data->length;
}
#endif

int
_gnutls_gen_cert_client_crt(gnutls_session_t session, gnutls_buffer_st * data)
{
	switch (session->security_parameters.cert_type) {
#ifdef ENABLE_OPENPGP
	case GNUTLS_CRT_OPENPGP:
		if (_gnutls_openpgp_send_fingerprint(session) == 0)
			return gen_openpgp_certificate(session, data);
		else
			return gen_openpgp_certificate_fpr(session, data);
#endif
	case GNUTLS_CRT_X509:
		return gen_x509_crt(session, data);

	default:
		gnutls_assert();
		return GNUTLS_E_INTERNAL_ERROR;
	}
}

int
_gnutls_gen_cert_server_crt(gnutls_session_t session, gnutls_buffer_st * data)
{
	switch (session->security_parameters.cert_type) {
#ifdef ENABLE_OPENPGP
	case GNUTLS_CRT_OPENPGP:
		return gen_openpgp_certificate(session, data);
#endif
	case GNUTLS_CRT_X509:
		return gen_x509_crt(session, data);
	default:
		gnutls_assert();
		return GNUTLS_E_INTERNAL_ERROR;
	}
}

static
int check_pk_compat(gnutls_session_t session, gnutls_pubkey_t pubkey)
{
	unsigned cert_pk;
	unsigned req_cert_pk;
	unsigned kx;

	if (session->security_parameters.entity != GNUTLS_CLIENT)
		return 0;

	cert_pk = gnutls_pubkey_get_pk_algorithm(pubkey, NULL);
	if (cert_pk == GNUTLS_PK_UNKNOWN) {
		gnutls_assert();
		return GNUTLS_E_CERTIFICATE_ERROR;
	}

	kx = _gnutls_cipher_suite_get_kx_algo(session->
					      security_parameters.cipher_suite);

	req_cert_pk = _gnutls_kx_cert_pk_params(kx);

	if (req_cert_pk == GNUTLS_PK_UNKNOWN)	/* doesn't matter */
		return 0;

	if (req_cert_pk != cert_pk) {
		gnutls_assert();
		return GNUTLS_E_CERTIFICATE_ERROR;
	}

	return 0;
}

/* Process server certificate
 */
#define CLEAR_CERTS for(x=0;x<peer_certificate_list_size;x++) gnutls_pcert_deinit(&peer_certificate_list[x])
static int
_gnutls_proc_x509_server_crt(gnutls_session_t session,
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
	     _gnutls_auth_info_set(session, GNUTLS_CRD_CERTIFICATE,
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

	/* some implementations send 0B 00 00 06 00 00 03 00 00 00
	 * instead of just 0B 00 00 03 00 00 00 as an empty certificate message.
	 */
	if (size == 0 || size == 3) {
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
			goto cleanup;
		}

		p += len;
	}

	ret = check_pk_compat(session, peer_certificate_list[0].pubkey);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	if ((ret =
	     copy_certificate_auth_info(info,
					peer_certificate_list,
					peer_certificate_list_size,
					NULL)) < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = 0;

 cleanup:
	CLEAR_CERTS;
	gnutls_free(peer_certificate_list);
	return ret;

}

#ifdef ENABLE_OPENPGP
static int
_gnutls_proc_openpgp_server_crt(gnutls_session_t session,
				uint8_t * data, size_t data_size)
{
	int size, ret, len;
	uint8_t *p = data;
	cert_auth_info_t info;
	gnutls_certificate_credentials_t cred;
	ssize_t dsize = data_size;
	int key_type;
	gnutls_pcert_st *peer_certificate_list = NULL;
	gnutls_datum_t tmp, akey = { NULL, 0 };
	unsigned int compat = 0;
	uint8_t subkey_id[GNUTLS_OPENPGP_KEYID_SIZE];

	cred = (gnutls_certificate_credentials_t)
	    _gnutls_get_cred(session, GNUTLS_CRD_CERTIFICATE);
	if (cred == NULL) {
		gnutls_assert();
		return GNUTLS_E_INSUFFICIENT_CREDENTIALS;
	}

	if ((ret =
	     _gnutls_auth_info_set(session, GNUTLS_CRD_CERTIFICATE,
				   sizeof(cert_auth_info_st), 1)) < 0) {
		gnutls_assert();
		return ret;
	}

	info = _gnutls_get_auth_info(session, GNUTLS_CRD_CERTIFICATE);

	if (data == NULL || data_size == 0) {
		gnutls_assert();
		return GNUTLS_E_NO_CERTIFICATE_FOUND;
	}

	DECR_LEN(dsize, 3);
	size = _gnutls_read_uint24(p);
	p += 3;

	if (size == 0) {
		gnutls_assert();
		/* no certificate was sent */
		return GNUTLS_E_NO_CERTIFICATE_FOUND;
	}

	/* Read PGPKeyDescriptor */
	DECR_LEN(dsize, 1);
	key_type = *p;
	p++;

	/* Try to read the keyid if present */
	if (key_type == PGP_KEY_FINGERPRINT_SUBKEY
	    || key_type == PGP_KEY_SUBKEY) {
		/* check size */
		if (*p != GNUTLS_OPENPGP_KEYID_SIZE) {
			gnutls_assert();
			return GNUTLS_E_UNSUPPORTED_CERTIFICATE_TYPE;
		}

		DECR_LEN(dsize, 1);
		p++;

		DECR_LEN(dsize, GNUTLS_OPENPGP_KEYID_SIZE);
		memcpy(subkey_id, p, GNUTLS_OPENPGP_KEYID_SIZE);
		p += GNUTLS_OPENPGP_KEYID_SIZE;
	}

	if (key_type == PGP_KEY_FINGERPRINT_SUBKEY) {
		DECR_LEN(dsize, 1);
		len = (uint8_t) * p;
		p++;

		if (len != 20) {
			gnutls_assert();
			return GNUTLS_E_OPENPGP_FINGERPRINT_UNSUPPORTED;
		}

		DECR_LEN(dsize, 20);

		/* request the actual key from our database, or
		 * a key server or anything.
		 */
		if ((ret =
		     _gnutls_openpgp_request_key(session, &akey, cred, p,
						 20)) < 0) {
			gnutls_assert();
			return ret;
		}
		tmp = akey;
	} else if (key_type == PGP_KEY_SUBKEY) {	/* the whole key */

		/* Read the actual certificate */
		DECR_LEN(dsize, 3);
		len = _gnutls_read_uint24(p);
		p += 3;

		if (len == 0) {
			gnutls_assert();
			/* no certificate was sent */
			return
			    gnutls_assert_val
			    (GNUTLS_E_UNEXPECTED_PACKET_LENGTH);
		}

		DECR_LEN(dsize, len);

		tmp.size = len;
		tmp.data = p;

	} else if (key_type == PGP_EMPTY_KEY) {	/* the whole key */

		/* Read the actual certificate */
		DECR_LEN(dsize, 3);
		len = _gnutls_read_uint24(p);
		p += 3;

		if (len == 0)	/* PGP_EMPTY_KEY */
			return GNUTLS_E_NO_CERTIFICATE_FOUND;
		/* Uncomment to remove compatibility with RFC5081.
		   else
		   return gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET_LENGTH); */

		DECR_LEN(dsize, len);

		tmp.size = len;
		tmp.data = p;

		compat = 1;
	} else {
		gnutls_assert();
		return GNUTLS_E_UNSUPPORTED_CERTIFICATE_TYPE;
	}

	if (dsize != 0)
		return gnutls_assert_val(GNUTLS_E_UNEXPECTED_PACKET_LENGTH);

	/* ok we now have the peer's key in tmp datum
	 */
	peer_certificate_list = gnutls_calloc(1, sizeof(gnutls_pcert_st));
	if (peer_certificate_list == NULL) {
		gnutls_assert();
		ret = GNUTLS_E_MEMORY_ERROR;
		goto cleanup;
	}

	ret =
	    gnutls_pcert_import_openpgp_raw(&peer_certificate_list[0],
					    &tmp,
					    GNUTLS_OPENPGP_FMT_RAW,
					    (compat ==
					     0) ? subkey_id : NULL, 0);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	if (compat != 0) {
		size_t t = sizeof(subkey_id);
		gnutls_pubkey_get_openpgp_key_id(peer_certificate_list
						 [0].pubkey, 0, subkey_id, &t,
						 NULL);
	}

	ret = check_pk_compat(session, peer_certificate_list[0].pubkey);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret =
	    copy_certificate_auth_info(info,
				       peer_certificate_list, 1, subkey_id);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = 0;

 cleanup:

	_gnutls_free_datum(&akey);
	if (peer_certificate_list != NULL) {
		gnutls_pcert_deinit(&peer_certificate_list[0]);
		gnutls_free(peer_certificate_list);
	}
	return ret;

}
#endif

int _gnutls_proc_crt(gnutls_session_t session, uint8_t * data, size_t data_size)
{
	int ret;
	gnutls_certificate_credentials_t cred;

	cred =
	    (gnutls_certificate_credentials_t) _gnutls_get_cred(session,
								GNUTLS_CRD_CERTIFICATE);
	if (cred == NULL) {
		gnutls_assert();
		return GNUTLS_E_INSUFFICIENT_CREDENTIALS;
	}

	switch (session->security_parameters.cert_type) {
#ifdef ENABLE_OPENPGP
	case GNUTLS_CRT_OPENPGP:
		ret = _gnutls_proc_openpgp_server_crt(session, data, data_size);
		break;
#endif
	case GNUTLS_CRT_X509:
		ret = _gnutls_proc_x509_server_crt(session, data, data_size);
		break;
	default:
		gnutls_assert();
		return GNUTLS_E_INTERNAL_ERROR;
	}

	return ret;
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
	     _gnutls_auth_info_set(session, GNUTLS_CRD_CERTIFICATE,
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

	if (session->security_parameters.cert_type == GNUTLS_CRT_OPENPGP
	    && size != 0) {
		gnutls_assert();
		return GNUTLS_E_UNEXPECTED_PACKET_LENGTH;
	}

	DECR_LEN_FINAL(dsize, size);

	/* We should reply with a certificate message, 
	 * even if we have no certificate to send.
	 */
	session->internals.crt_requested = 1;

	/* now we ask the user to tell which one
	 * he wants to use.
	 */
	if ((ret =
	     select_client_cert(session, p, size, pk_algos,
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

		p[0] = aid->hash_algorithm;
		p[1] = aid->sign_algorithm;
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

	ret = data->length;

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

	if (unlikely(info == NULL || info->ncerts == 0 || ver == NULL)) {
		gnutls_assert();
		/* we need this in order to get peer's certificate */
		return GNUTLS_E_INTERNAL_ERROR;
	}

	if (_gnutls_version_has_selectable_sighash(ver)) {
		sign_algorithm_st aid;

		DECR_LEN(dsize, 2);
		aid.hash_algorithm = pdata[0];
		aid.sign_algorithm = pdata[1];

		sign_algo = _gnutls_tls_aid_to_sign(&aid);
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
					  cert_type, info);

	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	if ((ret =
	     _gnutls_handshake_verify_crt_vrfy(session, &peer_cert, &sig,
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
		uint8_t p[MAX_SIGN_ALGO_SIZE];

		ret =
		    _gnutls_sign_algorithm_write_params(session, p,
							MAX_SIGN_ALGO_SIZE);
		if (ret < 0) {
			gnutls_assert();
			return ret;
		}

		ret = _gnutls_buffer_append_data(data, p, ret);
		if (ret < 0)
			return gnutls_assert_val(ret);
	}

	if (session->security_parameters.cert_type == GNUTLS_CRT_X509 &&
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

	return data->length;
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

		/* select_client_cert() has been called before.
		 */

		*apr_cert_list = session->internals.selected_cert_list;
		*apr_pkey = session->internals.selected_key;
		*apr_cert_list_length =
		    session->internals.selected_cert_list_length;

		if (*apr_cert_list_length == 0 || *apr_cert_list == NULL) {
			gnutls_assert();
			return GNUTLS_E_INSUFFICIENT_CREDENTIALS;
		}

	} else {		/* CLIENT SIDE 
				 */

		/* we have already decided which certificate
		 * to send.
		 */
		*apr_cert_list = session->internals.selected_cert_list;
		*apr_cert_list_length =
		    session->internals.selected_cert_list_length;
		*apr_pkey = session->internals.selected_key;

	}

	return 0;
}

/* converts the given x509 certificate list to gnutls_pcert_st* and allocates
 * space for them.
 */
static gnutls_pcert_st *alloc_and_load_x509_certs(gnutls_x509_crt_t *
						  certs, unsigned ncerts)
{
	gnutls_pcert_st *local_certs;
	int ret = 0;
	unsigned i, j;

	if (certs == NULL)
		return NULL;

	local_certs = gnutls_malloc(sizeof(gnutls_pcert_st) * ncerts);
	if (local_certs == NULL) {
		gnutls_assert();
		return NULL;
	}

	for (i = 0; i < ncerts; i++) {
		ret = gnutls_pcert_import_x509(&local_certs[i], certs[i], 0);
		if (ret < 0)
			break;
	}

	if (ret < 0) {
		gnutls_assert();
		for (j = 0; j < i; j++) {
			gnutls_pcert_deinit(&local_certs[j]);
		}
		gnutls_free(local_certs);
		return NULL;
	}

	return local_certs;
}

/* converts the given x509 key to gnutls_privkey* and allocates
 * space for it.
 */
static gnutls_privkey_t
alloc_and_load_x509_key(gnutls_x509_privkey_t key, int deinit)
{
	gnutls_privkey_t local_key;
	int ret = 0;

	if (key == NULL)
		return NULL;

	ret = gnutls_privkey_init(&local_key);
	if (ret < 0) {
		gnutls_assert();
		return NULL;
	}

	ret =
	    gnutls_privkey_import_x509(local_key, key,
				       deinit ?
				       GNUTLS_PRIVKEY_IMPORT_AUTO_RELEASE : 0);
	if (ret < 0) {
		gnutls_assert();
		gnutls_privkey_deinit(local_key);
		return NULL;
	}

	return local_key;
}

/* converts the given pgp certificate to gnutls_cert* and allocates
 * space for them.
 */
#ifdef ENABLE_OPENPGP
static gnutls_pcert_st *alloc_and_load_pgp_certs(gnutls_openpgp_crt_t cert)
{
	gnutls_pcert_st *local_certs;
	int ret = 0;

	if (cert == NULL)
		return NULL;

	local_certs = gnutls_malloc(sizeof(gnutls_pcert_st));
	if (local_certs == NULL) {
		gnutls_assert();
		return NULL;
	}

	ret = gnutls_pcert_import_openpgp(local_certs, cert, 0);
	if (ret < 0) {
		gnutls_assert();
		return NULL;
	}

	if (ret < 0) {
		gnutls_assert();
		gnutls_pcert_deinit(local_certs);
		gnutls_free(local_certs);
		return NULL;
	}

	return local_certs;
}

/* converts the given raw key to gnutls_privkey* and allocates
 * space for it.
 */
static gnutls_privkey_t
alloc_and_load_pgp_key(gnutls_openpgp_privkey_t key, int deinit)
{
	gnutls_privkey_t local_key;
	int ret = 0;

	if (key == NULL)
		return NULL;

	ret = gnutls_privkey_init(&local_key);
	if (ret < 0) {
		gnutls_assert();
		return NULL;
	}

	ret =
	    gnutls_privkey_import_openpgp(local_key, key,
					  deinit ?
					  GNUTLS_PRIVKEY_IMPORT_AUTO_RELEASE
					  : 0);
	if (ret < 0) {
		gnutls_assert();
		gnutls_privkey_deinit(local_key);
		return NULL;
	}

	return local_key;
}
#endif

#ifdef ENABLE_PKCS11

/* converts the given raw key to gnutls_privkey* and allocates
 * space for it.
 */
static gnutls_privkey_t
alloc_and_load_pkcs11_key(gnutls_pkcs11_privkey_t key, int deinit)
{
	gnutls_privkey_t local_key;
	int ret = 0;

	if (key == NULL)
		return NULL;

	ret = gnutls_privkey_init(&local_key);
	if (ret < 0) {
		gnutls_assert();
		return NULL;
	}

	ret =
	    gnutls_privkey_import_pkcs11(local_key, key,
					 deinit ?
					 GNUTLS_PRIVKEY_IMPORT_AUTO_RELEASE
					 : 0);
	if (ret < 0) {
		gnutls_assert();
		gnutls_privkey_deinit(local_key);
		return NULL;
	}

	return local_key;
}

#endif

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

		gnutls_privkey_deinit(session->internals.selected_key);
	}
	session->internals.selected_ocsp_func = NULL;

	session->internals.selected_cert_list = NULL;
	session->internals.selected_cert_list_length = 0;

	session->internals.selected_key = NULL;

	return;
}

static void
_gnutls_selected_certs_set(gnutls_session_t session,
			   gnutls_pcert_st * certs, int ncerts,
			   gnutls_privkey_t key, int need_free,
			   gnutls_status_request_ocsp_func ocsp_func,
			   void *ocsp_func_ptr)
{
	_gnutls_selected_certs_deinit(session);

	session->internals.selected_cert_list = certs;
	session->internals.selected_cert_list_length = ncerts;
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
_gnutls_server_select_cert(gnutls_session_t session,
			   gnutls_pk_algorithm_t * pk_algos,
			   size_t pk_algos_size)
{
	unsigned i, j;
	int idx, ret;
	gnutls_certificate_credentials_t cred;
	char server_name[MAX_CN];

	cred = (gnutls_certificate_credentials_t)
	    _gnutls_get_cred(session, GNUTLS_CRD_CERTIFICATE);
	if (cred == NULL) {
		gnutls_assert();
		return GNUTLS_E_INSUFFICIENT_CREDENTIALS;
	}

	/* If the callback which retrieves certificate has been set,
	 * use it and leave.
	 */
	if (cred->get_cert_callback
	    || cred->get_cert_callback2) {
		ret = call_get_cert_callback(session, NULL, 0, NULL, 0);
		if (ret < 0)
			return gnutls_assert_val(ret);
		return ret;
	}

	/* Otherwise... */

	get_server_name(session, (unsigned char *)server_name,
			sizeof(server_name));

	_gnutls_handshake_log ("HSK[%p]: Requested server name: '%s'\n",
				     session, server_name);
	idx = -1;		/* default is use no certificate */

	/* find certificates that match the requested server_name
	 */

	if (server_name[0] != 0) {
		for (i = 0; i < cred->ncerts; i++) {
			if (cred->certs[i].names != NULL
			    && _gnutls_str_array_match(cred->certs[i].names,
						       server_name) != 0) {
				/* if requested algorithms are also compatible select it */
				gnutls_pk_algorithm_t pk =
				    gnutls_pubkey_get_pk_algorithm(cred->certs
								   [i].cert_list
								   [0].pubkey,
								   NULL);

				_gnutls_handshake_log
				    ("HSK[%p]: Requested server name: '%s', ctype: %s (%d)\n",
				     session, server_name,
				     gnutls_certificate_type_get_name
				     (session->security_parameters.cert_type),
				     session->security_parameters.cert_type);

				if (session->security_parameters.cert_type ==
				    cred->certs[i].cert_list[0].type) {
					for (j = 0; j < pk_algos_size; j++)
						if (pk_algos[j] == pk) {
							idx = i;
							goto finished;
						}
				}
			}
		}
	}

	for (j = 0; j < pk_algos_size; j++) {
		_gnutls_handshake_log
		    ("HSK[%p]: Requested PK algorithm: %s (%d) -- ctype: %s (%d)\n",
		     session, gnutls_pk_get_name(pk_algos[j]), pk_algos[j],
		     gnutls_certificate_type_get_name
		     (session->security_parameters.cert_type),
		     session->security_parameters.cert_type);

		for (i = 0; i < cred->ncerts; i++) {
			gnutls_pk_algorithm_t pk =
			    gnutls_pubkey_get_pk_algorithm(cred->certs[i].
							   cert_list[0].pubkey,
							   NULL);
			/* find one compatible certificate
			 */
			_gnutls_handshake_log
			    ("HSK[%p]: certificate[%d] PK algorithm: %s (%d) - ctype: %s (%d)\n",
			     session, i, gnutls_pk_get_name(pk), pk,
			     gnutls_certificate_type_get_name(cred->certs
							      [i].cert_list
							      [0].type),
			     cred->certs[i].cert_list[0].type);

			if (pk_algos[j] == pk) {
				/* if cert type matches
				 */
	  /* *INDENT-OFF* */
	  if (session->security_parameters.cert_type == cred->certs[i].cert_list[0].type)
	    {
	      idx = i;
	      goto finished;
	    }
	  /* *INDENT-ON* */
			}
		}
	}

	/* store the certificate pointer for future use, in the handshake.
	 * (This will allow not calling this callback again.)
	 */
 finished:
	if (idx >= 0) {
		_gnutls_selected_certs_set(session,
					   &cred->certs[idx].cert_list[0],
					   cred->certs[idx].cert_list_length,
					   cred->pkey[idx], 0,
					   cred->certs[idx].ocsp_func,
					   cred->certs[idx].ocsp_func_ptr);
	} else {
		gnutls_assert();
		/* Certificate does not support REQUESTED_ALGO.  */
		return GNUTLS_E_INSUFFICIENT_CREDENTIALS;
	}

	return 0;
}

/* Frees the rsa_info_st structure.
 */
void _gnutls_free_rsa_info(rsa_info_st * rsa)
{
	_gnutls_free_datum(&rsa->modulus);
	_gnutls_free_datum(&rsa->exponent);
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

		p[0] = aid->hash_algorithm;
		p[1] = aid->sign_algorithm;

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

	if (unlikely(info == NULL || info->ncerts == 0 || ver == NULL)) {
		gnutls_assert();
		/* we need this in order to get peer's certificate */
		return GNUTLS_E_INTERNAL_ERROR;
	}

	/* VERIFY SIGNATURE */
	if (_gnutls_version_has_selectable_sighash(ver)) {
		sign_algorithm_st aid;

		DECR_LEN(data_size, 1);
		aid.hash_algorithm = *data++;
		DECR_LEN(data_size, 1);
		aid.sign_algorithm = *data++;
		sign_algo = _gnutls_tls_aid_to_sign(&aid);
		if (sign_algo == GNUTLS_SIGN_UNKNOWN) {
			_gnutls_debug_log("unknown signature %d.%d\n",
					  aid.sign_algorithm,
					  aid.hash_algorithm);
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

	if ((ret =
	     _gnutls_get_auth_info_pcert(&peer_cert,
					 session->security_parameters.cert_type,
					 info)) < 0) {
		gnutls_assert();
		return ret;
	}

	ret =
	    _gnutls_handshake_verify_data(session, &peer_cert, vparams,
					  &signature, sign_algo);

	gnutls_pcert_deinit(&peer_cert);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	return 0;
}
