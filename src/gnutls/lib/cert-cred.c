/*
 * Copyright (C) 2001-2016 Free Software Foundation, Inc.
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
#include "x509/common.h"
#include "dh.h"
#include "cert-cred.h"


/*
 * Adds a public/private key pair to a certificate credential
 */
int
_gnutls_certificate_credential_append_keypair(gnutls_certificate_credentials_t res,
				       gnutls_privkey_t key,
				       gnutls_str_array_t names,
				       gnutls_pcert_st * crt,
				       int nr)
{
	res->sorted_cert_idx = gnutls_realloc_fast(res->sorted_cert_idx,
						(1 + res->ncerts) *
						sizeof(unsigned int));
	if (res->sorted_cert_idx == NULL)
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

	res->certs = gnutls_realloc_fast(res->certs,
					 (1 + res->ncerts) *
					 sizeof(certs_st));
	if (res->certs == NULL)
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

	memset(&res->certs[res->ncerts], 0, sizeof(res->certs[0]));

	res->certs[res->ncerts].cert_list = crt;
	res->certs[res->ncerts].cert_list_length = nr;
	res->certs[res->ncerts].names = names;
	res->certs[res->ncerts].pkey = key;

	if (_gnutls13_sign_get_compatible_with_privkey(key))
		res->tls13_ok = 1;

	/* move RSA-PSS certificates before any RSA key.
	 * Note that we cannot assume that any previous pointers
	 * to sorted list are ok, due to the realloc in res->certs. */
	if (crt->pubkey->params.algo == GNUTLS_PK_RSA_PSS) {
		unsigned i,ridx;
		unsigned tmp;

		for (i=0;i<res->ncerts;i++) {
			ridx = res->sorted_cert_idx[i];

			if (res->certs[ridx].cert_list->pubkey->params.algo == GNUTLS_PK_RSA) {
				tmp = ridx;
				res->sorted_cert_idx[i] = res->ncerts;
				res->sorted_cert_idx[res->ncerts] = tmp;
				goto finish;
			}
		}
	}

	/* otherwise append it normally on the end */
	res->sorted_cert_idx[res->ncerts] = res->ncerts;

 finish:
	return 0;

}


/**
 * gnutls_certificate_set_key:
 * @res: is a #gnutls_certificate_credentials_t type.
 * @names: is an array of DNS names belonging to the public-key (NULL if none)
 * @names_size: holds the size of the names list
 * @pcert_list: contains a certificate list (chain) or raw public-key
 * @pcert_list_size: holds the size of the certificate list
 * @key: is a #gnutls_privkey_t key corresponding to the first public-key in pcert_list
 *
 * This function sets a public/private key pair in the
 * gnutls_certificate_credentials_t type. The given public key may be encapsulated
 * in a certificate or can be given as a raw key. This function may be
 * called more than once, in case multiple key pairs exist for
 * the server. For clients that want to send more than their own end-
 * entity certificate (e.g., also an intermediate CA cert), the full
 * certificate chain must be provided in @pcert_list.
 *
 * Note that the @key will become part of the credentials structure and must
 * not be deallocated. It will be automatically deallocated when the @res structure
 * is deinitialized.
 *
 * If this function fails, the @res structure is at an undefined state and it must
 * not be reused to load other keys or certificates.
 *
 * Note that, this function by default returns zero on success and a negative value on error.
 * Since 3.5.6, when the flag %GNUTLS_CERTIFICATE_API_V2 is set using gnutls_certificate_set_flags()
 * it returns an index (greater or equal to zero). That index can be used for other functions to refer to the added key-pair.
 *
 * Since GnuTLS 3.6.6 this function also handles raw public keys.
 *
 * Returns: On success this functions returns zero, and otherwise a negative value on error (see above for modifying that behavior).
 *
 * Since: 3.0
 **/
int
gnutls_certificate_set_key(gnutls_certificate_credentials_t res,
			   const char **names,
			   int names_size,
			   gnutls_pcert_st * pcert_list,
			   int pcert_list_size,
			   gnutls_privkey_t key)
{
	int ret, i;
	gnutls_str_array_t str_names;
	gnutls_pcert_st *new_pcert_list;

	/* Sanity checks */
	// Check for a valid credential struct
	if (res == NULL) {
		return gnutls_assert_val(GNUTLS_E_ILLEGAL_PARAMETER);
	}

	// A complete key pair must be given
	if (pcert_list == NULL || key == NULL) {
		return gnutls_assert_val(GNUTLS_E_INSUFFICIENT_CREDENTIALS);
	}

	/* Process the names, if any */
	_gnutls_str_array_init(&str_names);

	if (names != NULL && names_size > 0) {
		for (i = 0; i < names_size; i++) {
			ret =
			    _gnutls_str_array_append_idna(&str_names, names[i],
						     strlen(names[i]));
			if (ret < 0) {
				ret = gnutls_assert_val(ret);
				goto cleanup;
			}
		}
	} else if (names == NULL && pcert_list[0].type == GNUTLS_CRT_X509) {
		gnutls_x509_crt_t crt;

		ret = gnutls_x509_crt_init(&crt);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		ret = gnutls_x509_crt_import(crt, &pcert_list[0].cert, GNUTLS_X509_FMT_DER);
		if (ret < 0) {
			gnutls_assert();
			gnutls_x509_crt_deinit(crt);
			goto cleanup;
		}

		ret = _gnutls_get_x509_name(crt, &str_names);
		gnutls_x509_crt_deinit(crt);

		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}
	}

	if (res->pin.cb)
		gnutls_privkey_set_pin_function(key, res->pin.cb,
						res->pin.data);

	new_pcert_list = gnutls_malloc(sizeof(gnutls_pcert_st) * pcert_list_size);
	if (new_pcert_list == NULL) {
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);
	}
	memcpy(new_pcert_list, pcert_list, sizeof(gnutls_pcert_st) * pcert_list_size);

	ret =
	    _gnutls_certificate_credential_append_keypair(res, key, str_names,
						   new_pcert_list,
						   pcert_list_size);
	if (ret < 0) {
		gnutls_assert();
		gnutls_free(new_pcert_list);
		goto cleanup;
	}

	res->ncerts++;

	/* Unlike gnutls_certificate_set_x509_key, we deinitialize everything
	 * local after a failure. That is because the caller is responsible for
	 * freeing these values after a failure, and if we keep references we
	 * lead to double freeing */
	if ((ret = _gnutls_check_key_cert_match(res)) < 0) {
		gnutls_assert();
		gnutls_free(new_pcert_list);
		res->ncerts--;
		goto cleanup;
	}

	CRED_RET_SUCCESS(res);

      cleanup:
	_gnutls_str_array_clear(&str_names);
	return ret;
}

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

		for (j = 0; j < sc->certs[i].ocsp_data_length; j++) {
			gnutls_free(sc->certs[i].ocsp_data[j].response.data);
		}
		_gnutls_str_array_clear(&sc->certs[i].names);
		gnutls_privkey_deinit(sc->certs[i].pkey);
	}

	gnutls_free(sc->certs);
	gnutls_free(sc->sorted_cert_idx);

	sc->ncerts = 0;
}

/**
 * gnutls_certificate_free_cas:
 * @sc: is a #gnutls_certificate_credentials_t type.
 *
 * This function was operational on very early versions of gnutls.
 * Due to internal refactorings and the fact that this was hardly ever
 * used, it is currently a no-op.
 *
 **/
void gnutls_certificate_free_cas(gnutls_certificate_credentials_t sc)
{
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
	// Check for valid pointer and otherwise do nothing
	if (sc == NULL)
		return;

	gnutls_x509_trust_list_deinit(sc->tlist, 1);
	gnutls_certificate_free_keys(sc);
	memset(sc->pin_tmp, 0, sizeof(sc->pin_tmp));

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

/**
 * gnutls_certificate_server_set_request:
 * @session: is a #gnutls_session_t type.
 * @req: is one of GNUTLS_CERT_REQUEST, GNUTLS_CERT_REQUIRE, GNUTLS_CERT_IGNORE
 *
 * This function specifies if we (in case of a server) are going to
 * send a certificate request message to the client. If @req is
 * GNUTLS_CERT_REQUIRE then the server will return the %GNUTLS_E_NO_CERTIFICATE_FOUND
 * error if the peer does not provide a certificate. If you do not call this
 * function then the client will not be asked to send a certificate. Invoking
 * the function with @req GNUTLS_CERT_IGNORE has the same effect.
 **/
void
gnutls_certificate_server_set_request(gnutls_session_t session,
				      gnutls_certificate_request_t req)
{
	session->internals.send_cert_req = req;
}

static int call_legacy_cert_cb1(gnutls_session_t session,
				const struct gnutls_cert_retr_st *info,
				gnutls_pcert_st **certs,
				unsigned int *pcert_length,
				gnutls_ocsp_data_st **ocsp,
				unsigned int *ocsp_length,
				gnutls_privkey_t *privkey,
				unsigned int *flags)
{
	gnutls_retr2_st st2;
	gnutls_pcert_st *local_certs = NULL;
	gnutls_privkey_t local_key = NULL;
	unsigned i;
	int ret;

	*ocsp_length = 0;

	memset(&st2, 0, sizeof(st2));

	ret = info->cred->legacy_cert_cb1(session, info->req_ca_rdn, info->nreqs,
					  info->pk_algos, info->pk_algos_length,
				          &st2);
	if (ret < 0)
		return gnutls_assert_val(ret);

	if (st2.ncerts == 0) {
		*pcert_length = 0;
		*ocsp_length = 0;
		*privkey = NULL;
		return 0;
	}

	if (st2.cert_type != GNUTLS_CRT_X509) {
		gnutls_assert();
		ret = GNUTLS_E_INVALID_REQUEST;
		goto cleanup;
	}

	local_certs =
	    alloc_and_load_x509_certs(st2.cert.x509, st2.ncerts);
	if (local_certs == NULL) {
		gnutls_assert();
		ret = GNUTLS_E_MEMORY_ERROR;
		goto cleanup;
	}

	switch (st2.key_type) {
#ifdef ENABLE_PKCS11
	case GNUTLS_PRIVKEY_PKCS11:
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
		break;
#endif
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

	*privkey = local_key;
	*certs = local_certs;
	*pcert_length = st2.ncerts;

	/* flag the caller to deinitialize our values */
	*flags |= GNUTLS_CERT_RETR_DEINIT_ALL;

	ret = 0;

 cleanup:

	if (st2.cert_type == GNUTLS_CRT_X509) {
		if (st2.deinit_all) {
			for (i = 0; i < st2.ncerts; i++) {
				gnutls_x509_crt_deinit(st2.cert.x509[i]);
			}
			gnutls_free(st2.cert.x509);
		}
	}

	return ret;

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
	cred->legacy_cert_cb1 = func;
	if (!func)
		cred->get_cert_callback3 = NULL;
	else
		cred->get_cert_callback3 = call_legacy_cert_cb1;
}

static int call_legacy_cert_cb2(gnutls_session_t session,
				const struct gnutls_cert_retr_st *info,
				gnutls_pcert_st **certs,
				unsigned int *pcert_length,
				gnutls_ocsp_data_st **ocsp,
				unsigned int *ocsp_length,
				gnutls_privkey_t *privkey,
				unsigned int *flags)
{
	int ret;
	*ocsp_length = 0;
	/* flags will be assumed to be zero */

	ret = info->cred->legacy_cert_cb2(session, info->req_ca_rdn, info->nreqs,
					  info->pk_algos, info->pk_algos_length,
					  certs, pcert_length, privkey);
	if (ret < 0) {
		gnutls_assert();
	}
	return ret;
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
	cred->legacy_cert_cb2 = func;
	if (!func)
		cred->get_cert_callback3 = NULL;
	else
		cred->get_cert_callback3 = call_legacy_cert_cb2;
}

/**
 * gnutls_certificate_set_retrieve_function3:
 * @cred: is a #gnutls_certificate_credentials_t type.
 * @func: is the callback function
 *
 * This function sets a callback to be called in order to retrieve the
 * certificate and OCSP responses to be used in the handshake. @func will
 * be called only if the peer requests a certificate either during handshake
 * or during post-handshake authentication.
 *
 * The callback's function prototype is defined in `abstract.h':
 * int (*callback)(gnutls_session_t, const struct gnutls_cert_retr_st *info,
 * gnutls_pcert_st **certs, unsigned int *pcert_length,
 * gnutls_datum_t **ocsp, unsigned int *ocsp_length,
 * gnutls_privkey_t * pkey, unsigned int *flags);
 *
 * The info field of the callback contains:
 * @req_ca_dn which is a list with the CA names that the server considers trusted.
 * This is a hint and typically the client should send a certificate that is signed
 * by one of these CAs. These names, when available, are DER encoded. To get a more
 * meaningful value use the function gnutls_x509_rdn_get().
 * @pk_algos contains a list with server's acceptable public key algorithms.
 * The certificate returned should support the server's given algorithms.
 *
 * The callback should fill-in the following values.
 *
 * @pcert should contain an allocated list of certificates and public keys.
 * @pcert_length is the size of the previous list.
 * @ocsp should contain an allocated list of OCSP responses.
 * @ocsp_length is the size of the previous list.
 * @pkey is the private key.
 *
 * If flags in the callback are set to %GNUTLS_CERT_RETR_DEINIT_ALL then
 * all provided values must be allocated using gnutls_malloc(), and will
 * be released by gnutls; otherwise they will not be touched by gnutls.
 *
 * The callback function should set the certificate and OCSP response
 * list to be sent, and return 0 on success. If no certificates are available,
 * the @pcert_length and @ocsp_length should be set to zero. The return
 * value (-1) indicates error and the handshake will be terminated. If both
 * certificates are set in the credentials and a callback is available, the
 * callback takes predence.
 *
 * Since: 3.6.3
 **/
void gnutls_certificate_set_retrieve_function3
    (gnutls_certificate_credentials_t cred,
     gnutls_certificate_retrieve_function3 *func)
{
	cred->get_cert_callback3 = func;
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

#define TEST_TEXT "test text"
/* returns error if the certificate has different algorithm than
 * the given key parameters.
 */
int _gnutls_check_key_cert_match(gnutls_certificate_credentials_t res)
{
	gnutls_datum_t test = {(void*)TEST_TEXT, sizeof(TEST_TEXT)-1};
	gnutls_datum_t sig = {NULL, 0};
	gnutls_digest_algorithm_t dig;
	int pk, pk2, ret;
	unsigned sign_algo;

	if (res->flags & GNUTLS_CERTIFICATE_SKIP_KEY_CERT_MATCH)
		return 0;

	pk =
	    gnutls_pubkey_get_pk_algorithm(res->certs[res->ncerts - 1].
					   cert_list[0].pubkey, NULL);
	pk2 =
	    gnutls_privkey_get_pk_algorithm(res->certs[res->ncerts - 1].pkey,
					    NULL);

	if (GNUTLS_PK_IS_RSA(pk) && GNUTLS_PK_IS_RSA(pk2)) {
		if (pk2 == GNUTLS_PK_RSA_PSS && pk == GNUTLS_PK_RSA) {
			_gnutls_debug_log("you cannot mix an RSA-PSS key with an RSA certificate\n");
			return GNUTLS_E_CERTIFICATE_KEY_MISMATCH;
		}

		if (pk2 == GNUTLS_PK_RSA_PSS || pk == GNUTLS_PK_RSA_PSS)
			pk = GNUTLS_PK_RSA_PSS;
	} else if (pk2 != pk) {
		gnutls_assert();
		_gnutls_debug_log("key is %s, certificate is %s\n", gnutls_pk_get_name(pk2),
			gnutls_pk_get_name(pk));
		return GNUTLS_E_CERTIFICATE_KEY_MISMATCH;
	}

	if (pk == GNUTLS_PK_GOST_01)
		dig = GNUTLS_DIG_GOSTR_94;
	else if (pk == GNUTLS_PK_GOST_12_256)
		dig = GNUTLS_DIG_STREEBOG_256;
	else if (pk == GNUTLS_PK_GOST_12_512)
		dig = GNUTLS_DIG_STREEBOG_512;
	else
		dig = GNUTLS_DIG_SHA256;

	sign_algo = gnutls_pk_to_sign(pk, dig);

	/* now check if keys really match. We use the sign/verify approach
	 * because we cannot always obtain the parameters from the abstract
	 * keys (e.g. PKCS #11). */
	ret = gnutls_privkey_sign_data2(res->certs[res->ncerts - 1].pkey,
		sign_algo, 0, &test, &sig);
	if (ret < 0) {
		/* for some reason we couldn't sign that. That shouldn't have
		 * happened, but since it did, report the issue and do not
		 * try the key matching test */
		_gnutls_debug_log("%s: failed signing\n", __func__);
		goto finish;
	}

	ret = gnutls_pubkey_verify_data2(res->certs[res->ncerts - 1].cert_list[0].pubkey,
					 sign_algo,
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

	if (status & GNUTLS_CERT_UNKNOWN_CRIT_EXTENSIONS)
		_gnutls_buffer_append_str(&str,
					  _
					  ("The certificate contains an unknown critical extension. "));

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
 * Deprecated: This function is unnecessary and discouraged on GnuTLS 3.6.0
 * or later. Since 3.6.0, DH parameters are negotiated
 * following RFC7919.
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
	res->dh_sec_param = gnutls_pk_bits_to_sec_param(GNUTLS_PK_DH, _gnutls_mpi_get_nbits(dh_params->params[0]));
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
 * Deprecated: This function is unnecessary and discouraged on GnuTLS 3.6.0
 * or later. Since 3.6.0, DH parameters are negotiated
 * following RFC7919.
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
	res->dh_sec_param = sec_param;

	return 0;
}

#endif				/* DH */
