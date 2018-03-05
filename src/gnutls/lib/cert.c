/*
 * Copyright (C) 2001-2016 Free Software Foundation, Inc.
 * Copyright (C) 2015-2016 Red Hat, Inc.
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

/* Some of the stuff needed for Certificate authentication is contained
 * in this file.
 */

#include "gnutls_int.h"
#include "errors.h"
#include <auth/cert.h>
#include <datum.h>
#include <mpi.h>
#include <global.h>
#include <algorithms.h>
#include <dh.h>
#include "str.h"
#include <state.h>
#include <auth.h>
#include <x509.h>
#include <str_array.h>
#include <x509/verify-high.h>
#include "x509/x509_int.h"
#ifdef ENABLE_OPENPGP
#include "openpgp/openpgp.h"
#endif
#include "dh.h"

/**
 * gnutls_certificate_free_keys:
 * @sc: is a #gnutls_certificate_credentials_t type.
 *
 * This function will delete all the keys and the certificates associated
 * with the given credentials. This function must not be called when a
 * TLS negotiation that uses the credentials is in progress.
 *
 **/
void gnutls_certificate_free_keys(gnutls_certificate_credentials_t sc)
{
	unsigned i, j;

	for (i = 0; i < sc->ncerts; i++) {
		for (j = 0; j < sc->certs[i].cert_list_length; j++) {
			gnutls_pcert_deinit(&sc->certs[i].cert_list[j]);
		}
		gnutls_free(sc->certs[i].cert_list);
		gnutls_free(sc->certs[i].ocsp_response_file);
		_gnutls_str_array_clear(&sc->certs[i].names);
	}

	gnutls_free(sc->certs);
	sc->certs = NULL;

	for (i = 0; i < sc->ncerts; i++) {
		gnutls_privkey_deinit(sc->pkey[i]);
	}

	gnutls_free(sc->pkey);
	sc->pkey = NULL;

	sc->ncerts = 0;
}

/**
 * gnutls_certificate_free_cas:
 * @sc: is a #gnutls_certificate_credentials_t type.
 *
 * This function will delete all the CAs associated with the given
 * credentials. Servers that do not use
 * gnutls_certificate_verify_peers2() may call this to save some
 * memory.
 **/
void gnutls_certificate_free_cas(gnutls_certificate_credentials_t sc)
{
	/* FIXME: do nothing for now */
	return;
}

/**
 * gnutls_certificate_get_issuer:
 * @sc: is a #gnutls_certificate_credentials_t type.
 * @cert: is the certificate to find issuer for
 * @issuer: Will hold the issuer if any. Should be treated as constant.
 * @flags: Use zero or %GNUTLS_TL_GET_COPY
 *
 * This function will return the issuer of a given certificate.
 * If the flag %GNUTLS_TL_GET_COPY is specified a copy of the issuer
 * will be returned which must be freed using gnutls_x509_crt_deinit().
 * In that case the provided @issuer must not be initialized.
 *
 * As with gnutls_x509_trust_list_get_issuer() this function requires
 * the %GNUTLS_TL_GET_COPY flag in order to operate with PKCS#11 trust
 * lists in a thread-safe way. 
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.0
 **/
int
gnutls_certificate_get_issuer(gnutls_certificate_credentials_t sc,
			      gnutls_x509_crt_t cert,
			      gnutls_x509_crt_t * issuer,
			      unsigned int flags)
{
	return gnutls_x509_trust_list_get_issuer(sc->tlist, cert, issuer,
						 flags);
}

/**
 * gnutls_certificate_get_crt_raw:
 * @sc: is a #gnutls_certificate_credentials_t type.
 * @idx1: the index of the certificate chain if multiple are present
 * @idx2: the index of the certificate in the chain. Zero gives the server's certificate.
 * @cert: Will hold the DER encoded certificate.
 *
 * This function will return the DER encoded certificate of the
 * server or any other certificate on its certificate chain (based on @idx2).
 * The returned data should be treated as constant and only accessible during the lifetime
 * of @sc. The @idx1 matches the value gnutls_certificate_set_x509_key() and friends
 * functions.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value. In case the indexes are out of bounds %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE
 *   is returned.
 *
 * Since: 3.2.5
 **/
int
gnutls_certificate_get_crt_raw(gnutls_certificate_credentials_t sc,
			       unsigned idx1,
			       unsigned idx2, gnutls_datum_t * cert)
{
	if (idx1 >= sc->ncerts)
		return
		    gnutls_assert_val
		    (GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE);

	if (idx2 >= sc->certs[idx1].cert_list_length)
		return
		    gnutls_assert_val
		    (GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE);

	cert->data = sc->certs[idx1].cert_list[idx2].cert.data;
	cert->size = sc->certs[idx1].cert_list[idx2].cert.size;

	return 0;
}

/**
 * gnutls_certificate_free_ca_names:
 * @sc: is a #gnutls_certificate_credentials_t type.
 *
 * This function will delete all the CA name in the given
 * credentials. Clients may call this to save some memory since in
 * client side the CA names are not used. Servers might want to use
 * this function if a large list of trusted CAs is present and
 * sending the names of it would just consume bandwidth without providing 
 * information to client.
 *
 * CA names are used by servers to advertise the CAs they support to
 * clients.
 **/
void gnutls_certificate_free_ca_names(gnutls_certificate_credentials_t sc)
{
	_gnutls_free_datum(&sc->tlist->x509_rdn_sequence);
}


/**
 * gnutls_certificate_free_credentials:
 * @sc: is a #gnutls_certificate_credentials_t type.
 *
 * Free a gnutls_certificate_credentials_t structure.
 *
 * This function does not free any temporary parameters associated
 * with this structure (ie RSA and DH parameters are not freed by this
 * function).
 **/
void
gnutls_certificate_free_credentials(gnutls_certificate_credentials_t sc)
{
	gnutls_x509_trust_list_deinit(sc->tlist, 1);
	gnutls_certificate_free_keys(sc);
	memset(sc->pin_tmp, 0, sizeof(sc->pin_tmp));
#ifdef ENABLE_OPENPGP
	gnutls_openpgp_keyring_deinit(sc->keyring);
#endif
	if (sc->deinit_dh_params) {
		gnutls_dh_params_deinit(sc->dh_params);
	}

	gnutls_free(sc);
}


/**
 * gnutls_certificate_allocate_credentials:
 * @res: is a pointer to a #gnutls_certificate_credentials_t type.
 *
 * Allocate a gnutls_certificate_credentials_t structure.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, or an error code.
 **/
int
gnutls_certificate_allocate_credentials(gnutls_certificate_credentials_t *
					res)
{
	int ret;

	*res = gnutls_calloc(1, sizeof(certificate_credentials_st));

	if (*res == NULL)
		return GNUTLS_E_MEMORY_ERROR;

	ret = gnutls_x509_trust_list_init(&(*res)->tlist, 0);
	if (ret < 0) {
		gnutls_assert();
		gnutls_free(*res);
		return GNUTLS_E_MEMORY_ERROR;
	}
	(*res)->verify_bits = DEFAULT_MAX_VERIFY_BITS;
	(*res)->verify_depth = DEFAULT_MAX_VERIFY_DEPTH;


	return 0;
}

/* Returns 0 if it's ok to use the gnutls_kx_algorithm_t with this 
 * certificate (uses the KeyUsage field). 
 */
static int
check_key_usage(const gnutls_pcert_st * cert,
		gnutls_kx_algorithm_t alg)
{
	unsigned int key_usage = 0;
	int encipher_type;

	if (cert == NULL || alg == GNUTLS_KX_UNKNOWN) {
		gnutls_assert();
		return GNUTLS_E_INTERNAL_ERROR;
	}

	if (_gnutls_map_kx_get_cred(alg, 1) == GNUTLS_CRD_CERTIFICATE ||
	    _gnutls_map_kx_get_cred(alg, 0) == GNUTLS_CRD_CERTIFICATE) {

		gnutls_pubkey_get_key_usage(cert->pubkey, &key_usage);

		encipher_type = _gnutls_kx_encipher_type(alg);

		if (key_usage != 0 && encipher_type != CIPHER_IGN) {
			/* If key_usage has been set in the certificate
			 */

			if (encipher_type == CIPHER_ENCRYPT) {
				/* If the key exchange method requires an encipher
				 * type algorithm, and key's usage does not permit
				 * encipherment, then fail.
				 */
				if (!(key_usage & GNUTLS_KEY_KEY_ENCIPHERMENT)) {
					gnutls_assert();
					return
					    GNUTLS_E_KEY_USAGE_VIOLATION;
				}
			}

			if (encipher_type == CIPHER_SIGN) {
				/* The same as above, but for sign only keys
				 */
				if (!(key_usage & GNUTLS_KEY_DIGITAL_SIGNATURE)) {
					gnutls_assert();
					return
					    GNUTLS_E_KEY_USAGE_VIOLATION;
				}
			}
		}
	}
	return 0;
}


/* returns the KX algorithms that are supported by a
 * certificate. (Eg a certificate with RSA params, supports
 * GNUTLS_KX_RSA algorithm).
 * This function also uses the KeyUsage field of the certificate
 * extensions in order to disable unneeded algorithms.
 */
int
_gnutls_selected_cert_supported_kx(gnutls_session_t session,
				   gnutls_kx_algorithm_t * alg,
				   int *alg_size)
{
	unsigned kx;
	gnutls_pk_algorithm_t pk, cert_pk;
	gnutls_pcert_st *cert;
	int i;

	if (session->internals.selected_cert_list_length == 0) {
		*alg_size = 0;
		return 0;
	}

	cert = &session->internals.selected_cert_list[0];
	cert_pk = gnutls_pubkey_get_pk_algorithm(cert->pubkey, NULL);
	i = 0;

	for (kx = 0; kx < MAX_ALGOS; kx++) {
		pk = _gnutls_map_kx_get_pk(kx);
		if (pk == cert_pk) {
			/* then check key usage */
			if (check_key_usage(cert, kx) == 0 ||
			    unlikely(session->internals.priorities.allow_server_key_usage_violation != 0)) {
				alg[i] = kx;
				i++;

				if (i > *alg_size)
					return
					    gnutls_assert_val
					    (GNUTLS_E_INTERNAL_ERROR);
			}
		}
	}

	if (i == 0) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	*alg_size = i;

	return 0;
}


/**
 * gnutls_certificate_server_set_request:
 * @session: is a #gnutls_session_t type.
 * @req: is one of GNUTLS_CERT_REQUEST, GNUTLS_CERT_REQUIRE
 *
 * This function specifies if we (in case of a server) are going to
 * send a certificate request message to the client. If @req is
 * GNUTLS_CERT_REQUIRE then the server will return an error if the
 * peer does not provide a certificate. If you do not call this
 * function then the client will not be asked to send a certificate.
 **/
void
gnutls_certificate_server_set_request(gnutls_session_t session,
				      gnutls_certificate_request_t req)
{
	session->internals.send_cert_req = req;
}

/**
 * gnutls_certificate_set_retrieve_function:
 * @cred: is a #gnutls_certificate_credentials_t type.
 * @func: is the callback function
 *
 * This function sets a callback to be called in order to retrieve the
 * certificate to be used in the handshake. The callback will take control
 * only if a certificate is requested by the peer. You are advised
 * to use gnutls_certificate_set_retrieve_function2() because it
 * is much more efficient in the processing it requires from gnutls.
 *
 * The callback's function prototype is:
 * int (*callback)(gnutls_session_t, const gnutls_datum_t* req_ca_dn, int nreqs,
 * const gnutls_pk_algorithm_t* pk_algos, int pk_algos_length, gnutls_retr2_st* st);
 *
 * @req_ca_dn is only used in X.509 certificates.
 * Contains a list with the CA names that the server considers trusted.
 * This is a hint and typically the client should send a certificate that is signed
 * by one of these CAs. These names, when available, are DER encoded. To get a more
 * meaningful value use the function gnutls_x509_rdn_get().
 *
 * @pk_algos contains a list with server's acceptable public key algorithms.
 * The certificate returned should support the server's given algorithms.
 *
 * @st should contain the certificates and private keys.
 *
 * If the callback function is provided then gnutls will call it, in the
 * handshake, after the certificate request message has been received.
 *
 * In server side pk_algos and req_ca_dn are NULL.
 *
 * The callback function should set the certificate list to be sent,
 * and return 0 on success. If no certificate was selected then the
 * number of certificates should be set to zero. The value (-1)
 * indicates error and the handshake will be terminated. If both certificates
 * are set in the credentials and a callback is available, the callback
 * takes predence.
 *
 * Since: 3.0
 **/
void gnutls_certificate_set_retrieve_function
    (gnutls_certificate_credentials_t cred,
     gnutls_certificate_retrieve_function * func)
{
	cred->get_cert_callback = func;
}

/**
 * gnutls_certificate_set_retrieve_function2:
 * @cred: is a #gnutls_certificate_credentials_t type.
 * @func: is the callback function
 *
 * This function sets a callback to be called in order to retrieve the
 * certificate to be used in the handshake. The callback will take control
 * only if a certificate is requested by the peer.
 *
 * The callback's function prototype is:
 * int (*callback)(gnutls_session_t, const gnutls_datum_t* req_ca_dn, int nreqs,
 * const gnutls_pk_algorithm_t* pk_algos, int pk_algos_length, gnutls_pcert_st** pcert,
 * unsigned int *pcert_length, gnutls_privkey_t * pkey);
 *
 * @req_ca_dn is only used in X.509 certificates.
 * Contains a list with the CA names that the server considers trusted.
 * This is a hint and typically the client should send a certificate that is signed
 * by one of these CAs. These names, when available, are DER encoded. To get a more
 * meaningful value use the function gnutls_x509_rdn_get().
 *
 * @pk_algos contains a list with server's acceptable public key algorithms.
 * The certificate returned should support the server's given algorithms.
 *
 * @pcert should contain a single certificate and public key or a list of them.
 *
 * @pcert_length is the size of the previous list.
 *
 * @pkey is the private key.
 *
 * If the callback function is provided then gnutls will call it, in the
 * handshake, after the certificate request message has been received.
 * All the provided by the callback values will not be released or
 * modified by gnutls.
 *
 * In server side pk_algos and req_ca_dn are NULL.
 *
 * The callback function should set the certificate list to be sent,
 * and return 0 on success. If no certificate was selected then the
 * number of certificates should be set to zero. The value (-1)
 * indicates error and the handshake will be terminated. If both certificates
 * are set in the credentials and a callback is available, the callback
 * takes predence.
 *
 * Since: 3.0
 **/
void gnutls_certificate_set_retrieve_function2
    (gnutls_certificate_credentials_t cred,
     gnutls_certificate_retrieve_function2 * func) 
{
	cred->get_cert_callback2 = func;
}

/**
 * gnutls_certificate_set_verify_function:
 * @cred: is a #gnutls_certificate_credentials_t type.
 * @func: is the callback function
 *
 * This function sets a callback to be called when peer's certificate
 * has been received in order to verify it on receipt rather than
 * doing after the handshake is completed.
 *
 * The callback's function prototype is:
 * int (*callback)(gnutls_session_t);
 *
 * If the callback function is provided then gnutls will call it, in the
 * handshake, just after the certificate message has been received.
 * To verify or obtain the certificate the gnutls_certificate_verify_peers2(),
 * gnutls_certificate_type_get(), gnutls_certificate_get_peers() functions
 * can be used.
 *
 * The callback function should return 0 for the handshake to continue
 * or non-zero to terminate.
 *
 * Since: 2.10.0
 **/
void
 gnutls_certificate_set_verify_function
    (gnutls_certificate_credentials_t cred,
     gnutls_certificate_verify_function * func)
{
	cred->verify_callback = func;
}

/*-
 * _gnutls_x509_extract_certificate_activation_time - return the peer's certificate activation time
 * @cert: should contain an X.509 DER encoded certificate
 *
 * This function will return the certificate's activation time in UNIX time
 * (ie seconds since 00:00:00 UTC January 1, 1970).
 *
 * Returns a (time_t) -1 in case of an error.
 *
 -*/
static time_t
_gnutls_x509_get_raw_crt_activation_time(const gnutls_datum_t * cert)
{
	gnutls_x509_crt_t xcert;
	time_t result;

	result = gnutls_x509_crt_init(&xcert);
	if (result < 0)
		return (time_t) - 1;

	result = gnutls_x509_crt_import(xcert, cert, GNUTLS_X509_FMT_DER);
	if (result < 0) {
		gnutls_x509_crt_deinit(xcert);
		return (time_t) - 1;
	}

	result = gnutls_x509_crt_get_activation_time(xcert);

	gnutls_x509_crt_deinit(xcert);

	return result;
}

/*-
 * gnutls_x509_extract_certificate_expiration_time:
 * @cert: should contain an X.509 DER encoded certificate
 *
 * This function will return the certificate's expiration time in UNIX
 * time (ie seconds since 00:00:00 UTC January 1, 1970).  Returns a
 *
 * (time_t) -1 in case of an error.
 *
 -*/
static time_t
_gnutls_x509_get_raw_crt_expiration_time(const gnutls_datum_t * cert)
{
	gnutls_x509_crt_t xcert;
	time_t result;

	result = gnutls_x509_crt_init(&xcert);
	if (result < 0)
		return (time_t) - 1;

	result = gnutls_x509_crt_import(xcert, cert, GNUTLS_X509_FMT_DER);
	if (result < 0) {
		gnutls_x509_crt_deinit(xcert);
		return (time_t) - 1;
	}

	result = gnutls_x509_crt_get_expiration_time(xcert);

	gnutls_x509_crt_deinit(xcert);

	return result;
}

#ifdef ENABLE_OPENPGP
/*-
 * _gnutls_openpgp_crt_verify_peers - return the peer's certificate status
 * @session: is a gnutls session
 *
 * This function will try to verify the peer's certificate and return its status (TRUSTED, INVALID etc.).
 * Returns a negative error code in case of an error, or GNUTLS_E_NO_CERTIFICATE_FOUND if no certificate was sent.
 -*/
static int
_gnutls_openpgp_crt_verify_peers(gnutls_session_t session,
				 gnutls_x509_subject_alt_name_t type,
				 const char *hostname,
				 unsigned int *status)
{
	cert_auth_info_t info;
	gnutls_certificate_credentials_t cred;
	int peer_certificate_list_size, ret;
	unsigned int verify_flags;

	CHECK_AUTH(GNUTLS_CRD_CERTIFICATE, GNUTLS_E_INVALID_REQUEST);

	info = _gnutls_get_auth_info(session, GNUTLS_CRD_CERTIFICATE);
	if (info == NULL)
		return GNUTLS_E_INVALID_REQUEST;

	cred = (gnutls_certificate_credentials_t)
	    _gnutls_get_cred(session, GNUTLS_CRD_CERTIFICATE);
	if (cred == NULL) {
		gnutls_assert();
		return GNUTLS_E_INSUFFICIENT_CREDENTIALS;
	}

	if (info->raw_certificate_list == NULL || info->ncerts == 0) {
		gnutls_assert();
		return GNUTLS_E_NO_CERTIFICATE_FOUND;
	}

	verify_flags = cred->verify_flags | session->internals.additional_verify_flags;

	/* generate a list of gnutls_certs based on the auth info
	 * raw certs.
	 */
	peer_certificate_list_size = info->ncerts;

	if (peer_certificate_list_size != 1) {
		gnutls_assert();
		return GNUTLS_E_INTERNAL_ERROR;
	}

	/* Verify certificate 
	 */
	ret =
	    _gnutls_openpgp_verify_key(cred, type, hostname,
				       &info->raw_certificate_list[0],
				       peer_certificate_list_size,
				       verify_flags,
				       status);

	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	return 0;
}
#endif

/**
 * gnutls_certificate_verify_peers2:
 * @session: is a gnutls session
 * @status: is the output of the verification
 *
 * This function will verify the peer's certificate and store
 * the status in the @status variable as a bitwise or'd gnutls_certificate_status_t
 * values or zero if the certificate is trusted. Note that value in @status
 * is set only when the return value of this function is success (i.e, failure 
 * to trust a certificate does not imply a negative return value).
 * The default verification flags used by this function can be overridden
 * using gnutls_certificate_set_verify_flags().
 *
 * This function will take into account the OCSP Certificate Status TLS extension,
 * as well as the following X.509 certificate extensions: Name Constraints,
 * Key Usage, and Basic Constraints (pathlen).
 * 
 * To avoid denial of service attacks some
 * default upper limits regarding the certificate key size and chain
 * size are set. To override them use gnutls_certificate_set_verify_limits().
 *
 * Note that you must also check the peer's name in order to check if
 * the verified certificate belongs to the actual peer, see gnutls_x509_crt_check_hostname(),
 * or use gnutls_certificate_verify_peers3().
 *
 * Returns: %GNUTLS_E_SUCCESS (0) when the validation is performed, or a negative error code otherwise.
 * A sucessful error code means that the @status parameter must be checked to obtain the validation status.
 **/
int
gnutls_certificate_verify_peers2(gnutls_session_t session,
				 unsigned int *status)
{
	return gnutls_certificate_verify_peers(session, NULL, 0, status);
}

/**
 * gnutls_certificate_verify_peers3:
 * @session: is a gnutls session
 * @hostname: is the expected name of the peer; may be %NULL
 * @status: is the output of the verification
 *
 * This function will verify the peer's certificate and store the
 * status in the @status variable as a bitwise or'd gnutls_certificate_status_t
 * values or zero if the certificate is trusted. Note that value in @status
 * is set only when the return value of this function is success (i.e, failure 
 * to trust a certificate does not imply a negative return value).
 * The default verification flags used by this function can be overridden
 * using gnutls_certificate_set_verify_flags(). See the documentation
 * of gnutls_certificate_verify_peers2() for details in the verification process.
 *
 * If the @hostname provided is non-NULL then this function will compare
 * the hostname in the certificate against it. The comparison will follow
 * the RFC6125 recommendations. If names do not match the
 * %GNUTLS_CERT_UNEXPECTED_OWNER status flag will be set.
 *
 * In order to verify the purpose of the end-certificate (by checking the extended
 * key usage), use gnutls_certificate_verify_peers().
 *
 * Returns: %GNUTLS_E_SUCCESS (0) when the validation is performed, or a negative error code otherwise.
 * A sucessful error code means that the @status parameter must be checked to obtain the validation status.
 *
 * Since: 3.1.4
 **/
int
gnutls_certificate_verify_peers3(gnutls_session_t session,
				 const char *hostname,
				 unsigned int *status)
{
gnutls_typed_vdata_st data;

	data.type = GNUTLS_DT_DNS_HOSTNAME;
	data.size = 0;
	data.data = (void*)hostname;

	return gnutls_certificate_verify_peers(session, &data, 1, status);
}

/**
 * gnutls_certificate_verify_peers:
 * @session: is a gnutls session
 * @data: an array of typed data
 * @elements: the number of data elements
 * @status: is the output of the verification
 *
 * This function will verify the peer's certificate and store the
 * status in the @status variable as a bitwise or'd gnutls_certificate_status_t
 * values or zero if the certificate is trusted. Note that value in @status
 * is set only when the return value of this function is success (i.e, failure 
 * to trust a certificate does not imply a negative return value).
 * The default verification flags used by this function can be overridden
 * using gnutls_certificate_set_verify_flags(). See the documentation
 * of gnutls_certificate_verify_peers2() for details in the verification process.
 *
 * The acceptable @data types are %GNUTLS_DT_DNS_HOSTNAME, %GNUTLS_DT_RFC822NAME and %GNUTLS_DT_KEY_PURPOSE_OID.
 * The former two accept as data a null-terminated hostname or email address, and the latter a null-terminated
 * object identifier (e.g., %GNUTLS_KP_TLS_WWW_SERVER).
 *
 * If a DNS hostname is provided then this function will compare
 * the hostname in the certificate against the given. If names do not match the 
 * %GNUTLS_CERT_UNEXPECTED_OWNER status flag will be set.
 * If a key purpose OID is provided and the end-certificate contains the extended key
 * usage PKIX extension, it will be required to be have the provided key purpose 
 * or be marked for any purpose, otherwise verification status will have the
 * %GNUTLS_CERT_SIGNER_CONSTRAINTS_FAILURE flag set.
 *
 * Returns: %GNUTLS_E_SUCCESS (0) when the validation is performed, or a negative error code otherwise.
 * A sucessful error code means that the @status parameter must be checked to obtain the validation status.
 *
 * Since: 3.3.0
 **/
int
gnutls_certificate_verify_peers(gnutls_session_t session,
				gnutls_typed_vdata_st * data,
				unsigned int elements,
				unsigned int *status)
{
	cert_auth_info_t info;
	const char *hostname = NULL;
	unsigned i, type = 0;

	CHECK_AUTH(GNUTLS_CRD_CERTIFICATE, GNUTLS_E_INVALID_REQUEST);

	info = _gnutls_get_auth_info(session, GNUTLS_CRD_CERTIFICATE);
	if (info == NULL) {
		return GNUTLS_E_NO_CERTIFICATE_FOUND;
	}

	if (info->raw_certificate_list == NULL || info->ncerts == 0)
		return GNUTLS_E_NO_CERTIFICATE_FOUND;


	switch (gnutls_certificate_type_get(session)) {
	case GNUTLS_CRT_X509:
		return _gnutls_x509_cert_verify_peers(session, data, elements,
						      status);
#ifdef ENABLE_OPENPGP
	case GNUTLS_CRT_OPENPGP:
		for (i=0;i<elements;i++) {
			if (data[i].type == GNUTLS_DT_DNS_HOSTNAME) {
				hostname = (char*)data[i].data;
				type = GNUTLS_SAN_DNSNAME;
				break;
			} else if (data[i].type == GNUTLS_DT_RFC822NAME) {
				hostname = (char*)data[i].data;
				type = GNUTLS_SAN_RFC822NAME;
				break;
			}
		}
		return _gnutls_openpgp_crt_verify_peers(session,
							type,
							hostname,
							status);
#endif
	default:
		return GNUTLS_E_INVALID_REQUEST;
	}
}

/**
 * gnutls_certificate_expiration_time_peers:
 * @session: is a gnutls session
 *
 * This function will return the peer's certificate expiration time.
 *
 * Returns: (time_t)-1 on error.
 *
 * Deprecated: gnutls_certificate_verify_peers2() now verifies expiration times.
 **/
time_t gnutls_certificate_expiration_time_peers(gnutls_session_t session)
{
	cert_auth_info_t info;

	CHECK_AUTH(GNUTLS_CRD_CERTIFICATE, GNUTLS_E_INVALID_REQUEST);

	info = _gnutls_get_auth_info(session, GNUTLS_CRD_CERTIFICATE);
	if (info == NULL) {
		return (time_t) - 1;
	}

	if (info->raw_certificate_list == NULL || info->ncerts == 0) {
		gnutls_assert();
		return (time_t) - 1;
	}

	switch (gnutls_certificate_type_get(session)) {
	case GNUTLS_CRT_X509:
		return
		    _gnutls_x509_get_raw_crt_expiration_time(&info->
							     raw_certificate_list
							     [0]);
#ifdef ENABLE_OPENPGP
	case GNUTLS_CRT_OPENPGP:
		return
		    _gnutls_openpgp_get_raw_key_expiration_time
		    (&info->raw_certificate_list[0]);
#endif
	default:
		return (time_t) - 1;
	}
}

/**
 * gnutls_certificate_activation_time_peers:
 * @session: is a gnutls session
 *
 * This function will return the peer's certificate activation time.
 * This is the creation time for openpgp keys.
 *
 * Returns: (time_t)-1 on error.
 *
 * Deprecated: gnutls_certificate_verify_peers2() now verifies activation times.
 **/
time_t gnutls_certificate_activation_time_peers(gnutls_session_t session)
{
	cert_auth_info_t info;

	CHECK_AUTH(GNUTLS_CRD_CERTIFICATE, GNUTLS_E_INVALID_REQUEST);

	info = _gnutls_get_auth_info(session, GNUTLS_CRD_CERTIFICATE);
	if (info == NULL) {
		return (time_t) - 1;
	}

	if (info->raw_certificate_list == NULL || info->ncerts == 0) {
		gnutls_assert();
		return (time_t) - 1;
	}

	switch (gnutls_certificate_type_get(session)) {
	case GNUTLS_CRT_X509:
		return
		    _gnutls_x509_get_raw_crt_activation_time(&info->
							     raw_certificate_list
							     [0]);
#ifdef ENABLE_OPENPGP
	case GNUTLS_CRT_OPENPGP:
		return
		    _gnutls_openpgp_get_raw_key_creation_time(&info->
							      raw_certificate_list
							      [0]);
#endif
	default:
		return (time_t) - 1;
	}
}

#define TEST_TEXT "test text"
/* returns error if the certificate has different algorithm than
 * the given key parameters.
 */
int _gnutls_check_key_cert_match(gnutls_certificate_credentials_t res)
{
	gnutls_datum_t test = {(void*)TEST_TEXT, sizeof(TEST_TEXT)-1};
	gnutls_datum_t sig = {NULL, 0};
	int pk, pk2, ret;

	if (res->flags & GNUTLS_CERTIFICATE_SKIP_KEY_CERT_MATCH)
		return 0;

	pk =
	    gnutls_pubkey_get_pk_algorithm(res->certs[res->ncerts - 1].
					   cert_list[0].pubkey, NULL);
	pk2 =
	    gnutls_privkey_get_pk_algorithm(res->pkey[res->ncerts - 1],
					    NULL);

	if (pk2 != pk) {
		gnutls_assert();
		return GNUTLS_E_CERTIFICATE_KEY_MISMATCH;
	}

	/* now check if keys really match. We use the sign/verify approach
	 * because we cannot always obtain the parameters from the abstract
	 * keys (e.g. PKCS #11). */
	ret = gnutls_privkey_sign_data(res->pkey[res->ncerts - 1],
		GNUTLS_DIG_SHA256, 0, &test, &sig);
	if (ret < 0) {
		/* for some reason we couldn't sign that. That shouldn't have
		 * happened, but since it did, report the issue and do not
		 * try the key matching test */
		_gnutls_debug_log("%s: failed signing\n", __func__);
		goto finish;
	}

	ret = gnutls_pubkey_verify_data2(res->certs[res->ncerts - 1].cert_list[0].pubkey,
		gnutls_pk_to_sign(pk, GNUTLS_DIG_SHA256),
		GNUTLS_VERIFY_ALLOW_BROKEN, &test, &sig);

	gnutls_free(sig.data);

	if (ret < 0)
		return gnutls_assert_val(GNUTLS_E_CERTIFICATE_KEY_MISMATCH);

 finish:
	return 0;
}

/**
 * gnutls_certificate_verification_status_print:
 * @status: The status flags to be printed
 * @type: The certificate type
 * @out: Newly allocated datum with (0) terminated string.
 * @flags: should be zero
 *
 * This function will pretty print the status of a verification
 * process -- eg. the one obtained by gnutls_certificate_verify_peers3().
 *
 * The output @out needs to be deallocated using gnutls_free().
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.1.4
 **/
int
gnutls_certificate_verification_status_print(unsigned int status,
					     gnutls_certificate_type_t
					     type, gnutls_datum_t * out,
					     unsigned int flags)
{
	gnutls_buffer_st str;

	_gnutls_buffer_init(&str);

	if (status == 0)
		_gnutls_buffer_append_str(&str,
					  _
					  ("The certificate is trusted. "));
	else
		_gnutls_buffer_append_str(&str,
					  _
					  ("The certificate is NOT trusted. "));

	if (type == GNUTLS_CRT_X509) {
		if (status & GNUTLS_CERT_REVOKED)
			_gnutls_buffer_append_str(&str,
						  _
						  ("The certificate chain is revoked. "));

		if (status & GNUTLS_CERT_MISMATCH)
			_gnutls_buffer_append_str(&str,
						  _
						  ("The certificate doesn't match the local copy (TOFU). "));

		if (status & GNUTLS_CERT_REVOCATION_DATA_SUPERSEDED)
			_gnutls_buffer_append_str(&str,
						  _
						  ("The revocation or OCSP data are old and have been superseded. "));

		if (status & GNUTLS_CERT_REVOCATION_DATA_ISSUED_IN_FUTURE)
			_gnutls_buffer_append_str(&str,
						  _
						  ("The revocation or OCSP data are issued with a future date. "));

		if (status & GNUTLS_CERT_SIGNER_NOT_FOUND)
			_gnutls_buffer_append_str(&str,
						  _
						  ("The certificate issuer is unknown. "));

		if (status & GNUTLS_CERT_SIGNER_NOT_CA)
			_gnutls_buffer_append_str(&str,
						  _
						  ("The certificate issuer is not a CA. "));
	} else if (type == GNUTLS_CRT_OPENPGP) {
		if (status & GNUTLS_CERT_SIGNER_NOT_FOUND)
			_gnutls_buffer_append_str(&str,
						  _
						  ("Could not find a signer of the certificate. "));

		if (status & GNUTLS_CERT_REVOKED)
			_gnutls_buffer_append_str(&str,
						  _
						  ("The certificate is revoked. "));
	}

	if (status & GNUTLS_CERT_INSECURE_ALGORITHM)
		_gnutls_buffer_append_str(&str,
					  _
					  ("The certificate chain uses insecure algorithm. "));

	if (status & GNUTLS_CERT_SIGNER_CONSTRAINTS_FAILURE)
		_gnutls_buffer_append_str(&str,
					  _
					  ("The certificate chain violates the signer's constraints. "));

	if (status & GNUTLS_CERT_PURPOSE_MISMATCH)
		_gnutls_buffer_append_str(&str,
					  _
					  ("The certificate chain does not match the intended purpose. "));

	if (status & GNUTLS_CERT_NOT_ACTIVATED)
		_gnutls_buffer_append_str(&str,
					  _
					  ("The certificate chain uses not yet valid certificate. "));

	if (status & GNUTLS_CERT_EXPIRED)
		_gnutls_buffer_append_str(&str,
					  _
					  ("The certificate chain uses expired certificate. "));

	if (status & GNUTLS_CERT_SIGNATURE_FAILURE)
		_gnutls_buffer_append_str(&str,
					  _
					  ("The signature in the certificate is invalid. "));

	if (status & GNUTLS_CERT_UNEXPECTED_OWNER)
		_gnutls_buffer_append_str(&str,
					  _
					  ("The name in the certificate does not match the expected. "));

	if (status & GNUTLS_CERT_MISSING_OCSP_STATUS)
		_gnutls_buffer_append_str(&str,
					  _
					  ("The certificate requires the server to include an OCSP status in its response, but the OCSP status is missing. "));

	if (status & GNUTLS_CERT_INVALID_OCSP_STATUS)
		_gnutls_buffer_append_str(&str,
					  _
					  ("The received OCSP status response is invalid. "));

	return _gnutls_buffer_to_datum(&str, out, 1);
}

#if defined(ENABLE_DHE) || defined(ENABLE_ANON)
/**
 * gnutls_certificate_set_dh_params:
 * @res: is a gnutls_certificate_credentials_t type
 * @dh_params: the Diffie-Hellman parameters.
 *
 * This function will set the Diffie-Hellman parameters for a
 * certificate server to use. These parameters will be used in
 * Ephemeral Diffie-Hellman cipher suites.  Note that only a pointer
 * to the parameters are stored in the certificate handle, so you
 * must not deallocate the parameters before the certificate is deallocated.
 *
 **/
void
gnutls_certificate_set_dh_params(gnutls_certificate_credentials_t res,
				 gnutls_dh_params_t dh_params)
{
	if (res->deinit_dh_params) {
		res->deinit_dh_params = 0;
		gnutls_dh_params_deinit(res->dh_params);
		res->dh_params = NULL;
	}

	res->dh_params = dh_params;
}


/**
 * gnutls_certificate_set_known_dh_params:
 * @res: is a gnutls_certificate_credentials_t type
 * @sec_param: is an option of the %gnutls_sec_param_t enumeration
 *
 * This function will set the Diffie-Hellman parameters for a
 * certificate server to use. These parameters will be used in
 * Ephemeral Diffie-Hellman cipher suites and will be selected from
 * the FFDHE set of RFC7919 according to the security level provided.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.5.6
 **/
int
gnutls_certificate_set_known_dh_params(gnutls_certificate_credentials_t res,
				       gnutls_sec_param_t sec_param)
{
	int ret;

	if (res->deinit_dh_params) {
		res->deinit_dh_params = 0;
		gnutls_dh_params_deinit(res->dh_params);
		res->dh_params = NULL;
	}

	ret = _gnutls_set_cred_dh_params(&res->dh_params, sec_param);
	if (ret < 0)
		return gnutls_assert_val(ret);

	res->deinit_dh_params = 1;

	return 0;
}

#endif				/* DH */
