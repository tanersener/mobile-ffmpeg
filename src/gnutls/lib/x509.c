/*
 * Copyright (C) 2002-2016 Free Software Foundation, Inc.
 * Copyright (C) 2016 Red Hat, Inc.
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

#include "gnutls_int.h"
#include "auth.h"
#include "errors.h"
#include "extensions.h"
#include <auth/cert.h>
#include "dh.h"
#include "num.h"
#include "datum.h"
#include <pk.h>
#include <algorithms.h>
#include <global.h>
#include <record.h>
#include <tls-sig.h>
#include <state.h>
#include <pk.h>
#include "str.h"
#include <debug.h>
#include <x509_b64.h>
#include <x509.h>
#include <gnutls/ocsp.h>
#include "x509/common.h"
#include "x509/x509_int.h"
#include <str_array.h>
#include <gnutls/x509.h>
#include "read-file.h"
#include "system-keys.h"
#include "urls.h"
#ifdef _WIN32
#include <wincrypt.h>
#endif

/*
 * some x509 certificate parsing functions.
 */

#define CRED_RET_SUCCESS(cred) \
	if (cred->flags & GNUTLS_CERTIFICATE_API_V2) { \
		return cred->ncerts-1; \
	} else { \
		return 0; \
	}

/* fifteen days */
#define MAX_OCSP_VALIDITY_SECS (15*60*60*24)
#ifdef ENABLE_OCSP
/* If the certificate is revoked status will be GNUTLS_CERT_REVOKED.
 * 
 * Returns:
 *  Zero on success, a negative error code otherwise.
 */
static int
check_ocsp_response(gnutls_session_t session, gnutls_x509_crt_t cert,
		    gnutls_x509_trust_list_t tl,
		    unsigned verify_flags,
		    gnutls_x509_crt_t *cand_issuers, unsigned cand_issuers_size,
		    gnutls_datum_t * data, unsigned int *ostatus)
{
	gnutls_ocsp_resp_t resp;
	int ret;
	unsigned int status, cert_status;
	time_t rtime, vtime, ntime, now;
	int check_failed = 0;

	now = gnutls_time(0);

	ret = gnutls_ocsp_resp_init(&resp);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = gnutls_ocsp_resp_import(resp, data);
	if (ret < 0) {
		_gnutls_audit_log(session,
				  "There was an error parsing the OCSP response: %s.\n",
				  gnutls_strerror(ret));
		ret = gnutls_assert_val(0);
		check_failed = 1;
		*ostatus |= GNUTLS_CERT_INVALID_OCSP_STATUS;
		goto cleanup;
	}

	ret = gnutls_ocsp_resp_check_crt(resp, 0, cert);
	if (ret < 0) {
		ret = gnutls_assert_val(0);
		_gnutls_audit_log(session,
				  "Got OCSP response with an unrelated certificate.\n");
		check_failed = 1;
		*ostatus |= GNUTLS_CERT_INVALID_OCSP_STATUS;
		goto cleanup;
	}

	/* Attempt to verify against our trusted list */
	ret = gnutls_ocsp_resp_verify(resp, tl, &status, verify_flags);
	if ((ret < 0 || status != 0) && cand_issuers_size > 0) {
		/* Attempt to verify against the certificate list provided by the server */

		ret = gnutls_ocsp_resp_verify_direct(resp, cand_issuers[0], &status, verify_flags);
		/* if verification fails attempt to find whether any of the other
		 * bundled CAs is an issuer of the OCSP response */
		if ((ret < 0 || status != 0) && cand_issuers_size > 1) {
			int ret2;
			unsigned status2, i;

			for (i=1;i<cand_issuers_size;i++) {
				ret2 = gnutls_ocsp_resp_verify_direct(resp, cand_issuers[i], &status2, verify_flags);
				if (ret2 >= 0 && status2 == 0) {
					status = status2;
					ret = ret2;
					break;
				}
			}
		}
	}

	if (ret < 0) {
		ret = gnutls_assert_val(0);
		gnutls_assert();
		check_failed = 1;
		*ostatus |= GNUTLS_CERT_INVALID_OCSP_STATUS;
		goto cleanup;
	}

	/* do not consider revocation data if response was not verified */
	if (status != 0) {
		ret = gnutls_assert_val(0);
		check_failed = 1;
		*ostatus |= GNUTLS_CERT_INVALID_OCSP_STATUS;
		goto cleanup;
	}

	ret = gnutls_ocsp_resp_get_single(resp, 0, NULL, NULL, NULL, NULL,
					  &cert_status, &vtime, &ntime,
					  &rtime, NULL);
	if (ret < 0) {
		_gnutls_audit_log(session,
				  "There was an error parsing the OCSP response: %s.\n",
				  gnutls_strerror(ret));
		ret = gnutls_assert_val(0);
		check_failed = 1;
		*ostatus |= GNUTLS_CERT_INVALID_OCSP_STATUS;
		goto cleanup;
	}

	if (cert_status == GNUTLS_OCSP_CERT_REVOKED) {
		_gnutls_audit_log(session,
				  "The certificate was revoked via OCSP\n");
		check_failed = 1;
		*ostatus |= GNUTLS_CERT_REVOKED;
		ret = gnutls_assert_val(0);
		goto cleanup;
	}

	/* Report but do not fail on the following errors. That is
	 * because including the OCSP response in the handshake shouldn't 
	 * cause more problems that not including it.
	 */
	if (ntime == -1) {
		if (now - vtime > MAX_OCSP_VALIDITY_SECS) {
			_gnutls_audit_log(session,
					  "The OCSP response is old\n");
			check_failed = 1;
			*ostatus |= GNUTLS_CERT_REVOCATION_DATA_SUPERSEDED;
			goto cleanup;
		}
	} else {
		/* there is a newer OCSP answer, don't trust this one */
		if (ntime < now) {
			_gnutls_audit_log(session,
					  "There is a newer OCSP response but was not provided by the server\n");
			check_failed = 1;
			*ostatus |= GNUTLS_CERT_REVOCATION_DATA_SUPERSEDED;
			goto cleanup;
		}
	}

	ret = 0;
      cleanup:
	if (check_failed == 0)
		session->internals.ocsp_check_ok = 1;

	gnutls_ocsp_resp_deinit(resp);

	return ret;
}

#endif

#define CLEAR_CERTS for(x=0;x<peer_certificate_list_size;x++) { \
	if (peer_certificate_list[x]) \
		gnutls_x509_crt_deinit(peer_certificate_list[x]); \
	} \
	gnutls_free( peer_certificate_list)

#ifdef ENABLE_OCSP
static int
_gnutls_ocsp_verify_mandatory_stapling(gnutls_session_t session,
				       gnutls_x509_crt_t cert,
				       unsigned int * ocsp_status)
{
	gnutls_x509_tlsfeatures_t tlsfeatures;
	int i, ret;
	unsigned feature;

	/* RFC 7633: If cert has TLS feature GNUTLS_EXTENSION_STATUS_REQUEST, stapling is mandatory.
	 *
	 * At this point, we know that we did not get the certificate status.
	 *
	 * To proceed, first check whether we have requested the certificate status
	 */
	if (_gnutls_extension_list_check(session, GNUTLS_EXTENSION_STATUS_REQUEST) < 0) {
		return 0;
	}

	ret = gnutls_x509_tlsfeatures_init(&tlsfeatures);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	/* We have requested the status, now check whether the certificate mandates a response */
	if (gnutls_x509_crt_get_tlsfeatures(cert, tlsfeatures, 0, NULL) == 0) {
		for (i = 0;; ++i) {
			ret = gnutls_x509_tlsfeatures_get(tlsfeatures, i, &feature);
			if (ret == GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE) {
				break;
			}

			if (ret < 0) {
				gnutls_assert();
				goto cleanup;
			}
			if (feature == GNUTLS_EXTENSION_STATUS_REQUEST) {
				/* We sent a status request, the certificate mandates a reply, but we did not get any. */
				*ocsp_status |= GNUTLS_CERT_MISSING_OCSP_STATUS;
				break;
			}
		}
	}

	ret = 0;
 cleanup:
	gnutls_x509_tlsfeatures_deinit(tlsfeatures);
	return ret;
}
#endif

/*-
 * _gnutls_x509_cert_verify_peers - return the peer's certificate status
 * @session: is a gnutls session
 *
 * This function will try to verify the peer's certificate and return its status (TRUSTED, REVOKED etc.).
 * The return value (status) should be one of the gnutls_certificate_status_t enumerated elements.
 * However you must also check the peer's name in order to check if the verified certificate belongs to the
 * actual peer. Returns a negative error code in case of an error, or GNUTLS_E_NO_CERTIFICATE_FOUND if no certificate was sent.
 -*/
int
_gnutls_x509_cert_verify_peers(gnutls_session_t session,
			       gnutls_typed_vdata_st * data,
			       unsigned int elements,
			       unsigned int *status)
{
	cert_auth_info_t info;
	gnutls_certificate_credentials_t cred;
	gnutls_x509_crt_t *peer_certificate_list;
	gnutls_datum_t resp;
	int peer_certificate_list_size, i, x, ret;
	gnutls_x509_crt_t *cand_issuers = NULL;
	unsigned cand_issuers_size = 0;
	unsigned int ocsp_status = 0;
	unsigned int verify_flags;

	/* No OCSP check so far */
	session->internals.ocsp_check_ok = 0;

	CHECK_AUTH(GNUTLS_CRD_CERTIFICATE, GNUTLS_E_INVALID_REQUEST);

	info = _gnutls_get_auth_info(session, GNUTLS_CRD_CERTIFICATE);
	if (info == NULL) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	cred = (gnutls_certificate_credentials_t)
	    _gnutls_get_cred(session, GNUTLS_CRD_CERTIFICATE);
	if (cred == NULL) {
		gnutls_assert();
		return GNUTLS_E_INSUFFICIENT_CREDENTIALS;
	}

	if (info->raw_certificate_list == NULL || info->ncerts == 0)
		return GNUTLS_E_NO_CERTIFICATE_FOUND;

	if (info->ncerts > cred->verify_depth && cred->verify_depth > 0) {
		gnutls_assert();
		return GNUTLS_E_CONSTRAINT_ERROR;
	}

	verify_flags =
	    cred->verify_flags | session->internals.additional_verify_flags;
	/* generate a list of gnutls_certs based on the auth info
	 * raw certs.
	 */
	peer_certificate_list_size = info->ncerts;
	peer_certificate_list =
	    gnutls_calloc(peer_certificate_list_size,
			  sizeof(gnutls_x509_crt_t));
	if (peer_certificate_list == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	for (i = 0; i < peer_certificate_list_size; i++) {
		ret = gnutls_x509_crt_init(&peer_certificate_list[i]);
		if (ret < 0) {
			gnutls_assert();
			CLEAR_CERTS;
			return ret;
		}

		ret =
		    gnutls_x509_crt_import(peer_certificate_list[i],
					   &info->raw_certificate_list[i],
					   GNUTLS_X509_FMT_DER);
		if (ret < 0) {
			gnutls_assert();
			CLEAR_CERTS;
			return ret;
		}
	}

	/* Use the OCSP extension if any */
#ifdef ENABLE_OCSP
	if (verify_flags & GNUTLS_VERIFY_DISABLE_CRL_CHECKS)
		goto skip_ocsp;

	ret = gnutls_ocsp_status_request_get(session, &resp);
	if (ret < 0) {
		ret = _gnutls_ocsp_verify_mandatory_stapling(session, peer_certificate_list[0], &ocsp_status);
		if (ret < 0) {
			gnutls_assert();
			CLEAR_CERTS;
			return ret;
		}

		goto skip_ocsp;
	}

	if (peer_certificate_list_size > 1) {
		cand_issuers = &peer_certificate_list[1];
		cand_issuers_size = peer_certificate_list_size-1;
	}

	ret =
		check_ocsp_response(session, peer_certificate_list[0], cred->tlist, 
				verify_flags, cand_issuers,
				cand_issuers_size, &resp, &ocsp_status);

	if (ret < 0) {
		CLEAR_CERTS;
		return gnutls_assert_val(ret);
	}
#endif

      skip_ocsp:
	/* Verify certificate 
	 */
	ret =
	    gnutls_x509_trust_list_verify_crt2(cred->tlist,
					       peer_certificate_list,
					       peer_certificate_list_size,
					       data, elements,
					       verify_flags, status, NULL);

	if (ret < 0) {
		gnutls_assert();
		CLEAR_CERTS;
		return ret;
	}

	CLEAR_CERTS;

	*status |= ocsp_status;

	return 0;
}

static int str_array_append_idna(gnutls_str_array_t * head, const char *name, size_t size)
{
	int ret;
	gnutls_datum_t ahost;

	/* convert the provided hostname to ACE-Labels domain. */
	ret = gnutls_idna_map(name, size, &ahost, 0);
	if (ret < 0) {
		_gnutls_debug_log("unable to convert hostname %s to IDNA format\n", name);
		/* insert the raw name */
		return _gnutls_str_array_append(head, name, size);
	}

	ret = _gnutls_str_array_append(head, (char*)ahost.data, ahost.size);
	gnutls_free(ahost.data);

	return ret;
}

/* Returns the name of the certificate of a null name
 */
static int get_x509_name(gnutls_x509_crt_t crt, gnutls_str_array_t * names)
{
	size_t max_size;
	int i, ret = 0, ret2;
	char name[MAX_CN];
	unsigned have_dns_name = 0;

	for (i = 0; !(ret < 0); i++) {
		max_size = sizeof(name);

		ret =
		    gnutls_x509_crt_get_subject_alt_name(crt, i, name,
							 &max_size, NULL);
		if (ret == GNUTLS_SAN_DNSNAME) {
			have_dns_name = 1;

			ret2 =
			    str_array_append_idna(names, name,
						  max_size);
			if (ret2 < 0) {
				_gnutls_str_array_clear(names);
				return gnutls_assert_val(ret2);
			}
		}
	}

	if (have_dns_name == 0) {
		max_size = sizeof(name);
		ret =
		    gnutls_x509_crt_get_dn_by_oid(crt, OID_X520_COMMON_NAME, 0, 0,
						  name, &max_size);
		if (ret >= 0) {
			ret = str_array_append_idna(names, name, max_size);
			if (ret < 0) {
				_gnutls_str_array_clear(names);
				return gnutls_assert_val(ret);
			}
		}
	}

	return 0;
}

/* Reads a DER encoded certificate list from memory and stores it to a
 * gnutls_cert structure. Returns the number of certificates parsed.
 */
static int
parse_der_cert_mem(gnutls_certificate_credentials_t res,
		   const void *input_cert, int input_cert_size)
{
	gnutls_datum_t tmp;
	gnutls_x509_crt_t crt;
	gnutls_pcert_st *ccert;
	int ret;
	gnutls_str_array_t names;

	_gnutls_str_array_init(&names);

	ccert = gnutls_malloc(sizeof(*ccert));
	if (ccert == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	ret = gnutls_x509_crt_init(&crt);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	tmp.data = (uint8_t *) input_cert;
	tmp.size = input_cert_size;

	ret = gnutls_x509_crt_import(crt, &tmp, GNUTLS_X509_FMT_DER);
	if (ret < 0) {
		gnutls_assert();
		gnutls_x509_crt_deinit(crt);
		goto cleanup;
	}

	ret = get_x509_name(crt, &names);
	if (ret < 0) {
		gnutls_assert();
		gnutls_x509_crt_deinit(crt);
		goto cleanup;
	}

	ret = gnutls_pcert_import_x509(ccert, crt, 0);
	gnutls_x509_crt_deinit(crt);

	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = certificate_credential_append_crt_list(res, names, ccert, 1);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	return ret;

      cleanup:
	_gnutls_str_array_clear(&names);
	gnutls_free(ccert);
	return ret;
}

/* Reads a base64 encoded certificate list from memory and stores it to
 * a gnutls_cert structure. Returns the number of certificate parsed.
 */
static int
parse_pem_cert_mem(gnutls_certificate_credentials_t res,
		   const char *input_cert, int input_cert_size)
{
	int size;
	const char *ptr;
	gnutls_datum_t tmp;
	int ret, count, i;
	unsigned ncerts = 0;
	gnutls_pcert_st *pcerts = NULL;
	gnutls_str_array_t names;
	gnutls_x509_crt_t unsorted[DEFAULT_MAX_VERIFY_DEPTH];

	_gnutls_str_array_init(&names);

	/* move to the certificate
	 */
	ptr = memmem(input_cert, input_cert_size,
		     PEM_CERT_SEP, sizeof(PEM_CERT_SEP) - 1);
	if (ptr == NULL)
		ptr = memmem(input_cert, input_cert_size,
			     PEM_CERT_SEP2, sizeof(PEM_CERT_SEP2) - 1);

	if (ptr == NULL) {
		gnutls_assert();
		return GNUTLS_E_BASE64_DECODING_ERROR;
	}
	size = input_cert_size - (ptr - input_cert);

	count = 0;

	do {
		tmp.data = (void *) ptr;
		tmp.size = size;

		ret = gnutls_x509_crt_init(&unsorted[count]);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		ret = gnutls_x509_crt_import(unsorted[count], &tmp, GNUTLS_X509_FMT_PEM);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}
		count++;

		/* now we move ptr after the pem header 
		 */
		ptr++;
		size--;

		/* find the next certificate (if any)
		 */

		if (size > 0) {
			char *ptr3;

			ptr3 =
			    memmem(ptr, size, PEM_CERT_SEP,
				   sizeof(PEM_CERT_SEP) - 1);
			if (ptr3 == NULL)
				ptr3 = memmem(ptr, size, PEM_CERT_SEP2,
					      sizeof(PEM_CERT_SEP2) - 1);

			ptr = ptr3;
			size = input_cert_size - (ptr - input_cert);
		} else
			ptr = NULL;

	}
	while (ptr != NULL && count < DEFAULT_MAX_VERIFY_DEPTH);

	ret =
	    get_x509_name(unsorted[0], &names);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	pcerts = gnutls_malloc(sizeof(gnutls_pcert_st) * count);
	if (pcerts == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	ncerts = count;
	ret =
	    gnutls_pcert_import_x509_list(pcerts, unsorted, &ncerts, GNUTLS_X509_CRT_LIST_SORT);
	if (ret < 0) {
		gnutls_free(pcerts);
		pcerts = NULL;
		gnutls_assert();
		goto cleanup;
	}

	ret =
	    certificate_credential_append_crt_list(res, names, pcerts,
						   ncerts);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	for (i = 0; i < count; i++)
		gnutls_x509_crt_deinit(unsorted[i]);

	return ncerts;

      cleanup:
	_gnutls_str_array_clear(&names);
	for (i = 0; i < count; i++)
		gnutls_x509_crt_deinit(unsorted[i]);
	if (pcerts) {
		for (i = 0; i < count; i++)
			gnutls_pcert_deinit(&pcerts[i]);
		gnutls_free(pcerts);
	}
	return ret;
}



/* Reads a DER or PEM certificate from memory
 */
static int
read_cert_mem(gnutls_certificate_credentials_t res, const void *cert,
	      int cert_size, gnutls_x509_crt_fmt_t type)
{
	int ret;

	if (type == GNUTLS_X509_FMT_DER)
		ret = parse_der_cert_mem(res, cert, cert_size);
	else
		ret = parse_pem_cert_mem(res, cert, cert_size);

	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	return ret;
}

static int tmp_pin_cb(void *userdata, int attempt, const char *token_url,
		      const char *token_label, unsigned int flags,
		      char *pin, size_t pin_max)
{
	const char *tmp_pin = userdata;

	if (attempt == 0) {
		snprintf(pin, pin_max, "%s", tmp_pin);
		return 0;
	}

	return -1;
}

/* Reads a PEM encoded PKCS-1 RSA/DSA private key from memory.  Type
 * indicates the certificate format.  KEY can be NULL, to indicate
 * that GnuTLS doesn't know the private key.
 */
static int
read_key_mem(gnutls_certificate_credentials_t res,
	     const void *key, int key_size, gnutls_x509_crt_fmt_t type,
	     const char *pass, unsigned int flags)
{
	int ret;
	gnutls_datum_t tmp;
	gnutls_privkey_t privkey;

	if (key) {
		tmp.data = (uint8_t *) key;
		tmp.size = key_size;

		ret = gnutls_privkey_init(&privkey);
		if (ret < 0) {
			gnutls_assert();
			return ret;
		}

		if (res->pin.cb) {
			gnutls_privkey_set_pin_function(privkey,
							res->pin.cb,
							res->pin.data);
		} else if (pass != NULL) {
			snprintf(res->pin_tmp, sizeof(res->pin_tmp), "%s",
				 pass);
			gnutls_privkey_set_pin_function(privkey,
							tmp_pin_cb,
							res->pin_tmp);
		}

		ret =
		    gnutls_privkey_import_x509_raw(privkey, &tmp, type,
						   pass, flags);
		if (ret < 0) {
			gnutls_assert();
			gnutls_privkey_deinit(privkey);
			return ret;
		}

		ret = certificate_credentials_append_pkey(res, privkey);
		if (ret < 0) {
			gnutls_assert();
			gnutls_privkey_deinit(privkey);
			return ret;
		}

	} else {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}


	return 0;
}


/* Reads a private key from a token.
 */
static int
read_key_url(gnutls_certificate_credentials_t res, const char *url)
{
	int ret;
	gnutls_privkey_t pkey = NULL;

	/* allocate space for the pkey list
	 */
	ret = gnutls_privkey_init(&pkey);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	if (res->pin.cb)
		gnutls_privkey_set_pin_function(pkey, res->pin.cb,
						res->pin.data);

	ret = gnutls_privkey_import_url(pkey, url, 0);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = certificate_credentials_append_pkey(res, pkey);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	return 0;

      cleanup:
	if (pkey)
		gnutls_privkey_deinit(pkey);

	return ret;
}


#define MAX_PKCS11_CERT_CHAIN 8
/* Reads a certificate key from a token.
 */
static int
read_cert_url(gnutls_certificate_credentials_t res, const char *url)
{
	int ret;
	gnutls_x509_crt_t crt = NULL;
	gnutls_pcert_st *ccert = NULL;
	gnutls_str_array_t names;
	gnutls_datum_t t = {NULL, 0};
	unsigned i, count = 0;

	_gnutls_str_array_init(&names);

	ccert = gnutls_malloc(sizeof(*ccert)*MAX_PKCS11_CERT_CHAIN);
	if (ccert == NULL) {
		gnutls_assert();
		ret = GNUTLS_E_MEMORY_ERROR;
		goto cleanup;
	}

	ret = gnutls_x509_crt_init(&crt);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	if (res->pin.cb)
		gnutls_x509_crt_set_pin_function(crt, res->pin.cb,
						 res->pin.data);

	ret = gnutls_x509_crt_import_url(crt, url, 0);
	if (ret == GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE)
		ret =
		    gnutls_x509_crt_import_url(crt, url,
					       GNUTLS_PKCS11_OBJ_FLAG_LOGIN);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = get_x509_name(crt, &names);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	/* Try to load the whole certificate chain from the PKCS #11 token */
	for (i=0;i<MAX_PKCS11_CERT_CHAIN;i++) {
		ret = gnutls_x509_crt_check_issuer(crt, crt);
		if (i > 0 && ret != 0) {
			/* self signed */
			break;
		}

		ret = gnutls_pcert_import_x509(&ccert[i], crt, 0);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}
		count++;

		ret = _gnutls_get_raw_issuer(url, crt, &t, 0);
		if (ret < 0)
			break;

		gnutls_x509_crt_deinit(crt);
		crt = NULL;
		ret = gnutls_x509_crt_init(&crt);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}
		
		ret = gnutls_x509_crt_import(crt, &t, GNUTLS_X509_FMT_DER);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}
		gnutls_free(t.data);
		t.data = NULL;
	}

	ret = certificate_credential_append_crt_list(res, names, ccert, count);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	if (crt != NULL)
		gnutls_x509_crt_deinit(crt);

	return 0;
cleanup:
	if (crt != NULL)
		gnutls_x509_crt_deinit(crt);
	gnutls_free(t.data);
	_gnutls_str_array_clear(&names);
	gnutls_free(ccert);
	return ret;
}

/* Reads a certificate file
 */
static int
read_cert_file(gnutls_certificate_credentials_t res,
	       const char *certfile, gnutls_x509_crt_fmt_t type)
{
	int ret;
	size_t size;
	char *data;

	if (gnutls_url_is_supported(certfile)) {
		return read_cert_url(res, certfile);
	}

	data = read_binary_file(certfile, &size);

	if (data == NULL) {
		gnutls_assert();
		return GNUTLS_E_FILE_ERROR;
	}

	ret = read_cert_mem(res, data, size, type);
	free(data);

	return ret;

}



/* Reads PKCS-1 RSA private key file or a DSA file (in the format openssl
 * stores it).
 */
static int
read_key_file(gnutls_certificate_credentials_t res,
	      const char *keyfile, gnutls_x509_crt_fmt_t type,
	      const char *pass, unsigned int flags)
{
	int ret;
	size_t size;
	char *data;

	if (_gnutls_url_is_known(keyfile)) {
		if (gnutls_url_is_supported(keyfile)) {
			/* if no PIN function is specified, and we have a PIN,
			 * specify one */
			if (pass != NULL && res->pin.cb == NULL) {
				snprintf(res->pin_tmp, sizeof(res->pin_tmp), "%s", pass);
				gnutls_certificate_set_pin_function(res, tmp_pin_cb, res->pin_tmp);
			}

			return read_key_url(res, keyfile);
		} else
			return
			    gnutls_assert_val
			    (GNUTLS_E_UNIMPLEMENTED_FEATURE);
	}

	data = read_binary_file(keyfile, &size);

	if (data == NULL) {
		gnutls_assert();
		return GNUTLS_E_FILE_ERROR;
	}

	ret = read_key_mem(res, data, size, type, pass, flags);
	free(data);

	return ret;
}

/**
 * gnutls_certificate_set_x509_key_mem:
 * @res: is a #gnutls_certificate_credentials_t type.
 * @cert: contains a certificate list (path) for the specified private key
 * @key: is the private key, or %NULL
 * @type: is PEM or DER
 *
 * This function sets a certificate/private key pair in the
 * gnutls_certificate_credentials_t type. This function may be called
 * more than once, in case multiple keys/certificates exist for the
 * server.
 *
 * Note that the keyUsage (2.5.29.15) PKIX extension in X.509 certificates
 * is supported. This means that certificates intended for signing cannot
 * be used for ciphersuites that require encryption.
 *
 * If the certificate and the private key are given in PEM encoding
 * then the strings that hold their values must be null terminated.
 *
 * The @key may be %NULL if you are using a sign callback, see
 * gnutls_sign_callback_set().
 *
 * Note that, this function by default returns zero on success and a negative value on error.
 * Since 3.5.6, when the flag %GNUTLS_CERTIFICATE_API_V2 is set using gnutls_certificate_set_flags()
 * it returns an index (greater or equal to zero). That index can be used to other functions to refer to the added key-pair.
 *
 * Returns: On success this functions returns zero, and otherwise a negative value on error (see above for modifying that behavior).
 *
 **/
int
gnutls_certificate_set_x509_key_mem(gnutls_certificate_credentials_t res,
				    const gnutls_datum_t * cert,
				    const gnutls_datum_t * key,
				    gnutls_x509_crt_fmt_t type)
{
	return gnutls_certificate_set_x509_key_mem2(res, cert, key, type,
						    NULL, 0);
}

/**
 * gnutls_certificate_set_x509_key_mem2:
 * @res: is a #gnutls_certificate_credentials_t type.
 * @cert: contains a certificate list (path) for the specified private key
 * @key: is the private key, or %NULL
 * @type: is PEM or DER
 * @pass: is the key's password
 * @flags: an ORed sequence of gnutls_pkcs_encrypt_flags_t
 *
 * This function sets a certificate/private key pair in the
 * gnutls_certificate_credentials_t type. This function may be called
 * more than once, in case multiple keys/certificates exist for the
 * server.
 *
 * Note that the keyUsage (2.5.29.15) PKIX extension in X.509 certificates
 * is supported. This means that certificates intended for signing cannot
 * be used for ciphersuites that require encryption.
 *
 * If the certificate and the private key are given in PEM encoding
 * then the strings that hold their values must be null terminated.
 *
 * The @key may be %NULL if you are using a sign callback, see
 * gnutls_sign_callback_set().
 *
 * Note that, this function by default returns zero on success and a negative value on error.
 * Since 3.5.6, when the flag %GNUTLS_CERTIFICATE_API_V2 is set using gnutls_certificate_set_flags()
 * it returns an index (greater or equal to zero). That index can be used to other functions to refer to the added key-pair.
 *
 * Returns: On success this functions returns zero, and otherwise a negative value on error (see above for modifying that behavior).
 **/
int
gnutls_certificate_set_x509_key_mem2(gnutls_certificate_credentials_t res,
				     const gnutls_datum_t * cert,
				     const gnutls_datum_t * key,
				     gnutls_x509_crt_fmt_t type,
				     const char *pass, unsigned int flags)
{
	int ret;

	/* this should be first
	 */
	if ((ret = read_key_mem(res, key ? key->data : NULL,
				key ? key->size : 0, type, pass,
				flags)) < 0)
		return ret;

	if ((ret = read_cert_mem(res, cert->data, cert->size, type)) < 0) {
		gnutls_privkey_deinit(res->pkey[res->ncerts]);
		return ret;
	}

	res->ncerts++;

	if (key && (ret = _gnutls_check_key_cert_match(res)) < 0) {
		gnutls_assert();
		return ret;
	}

	CRED_RET_SUCCESS(res);
}

int
certificate_credential_append_crt_list(gnutls_certificate_credentials_t
				       res, gnutls_str_array_t names,
				       gnutls_pcert_st * crt, int nr)
{
	res->certs = gnutls_realloc_fast(res->certs,
					 (1 + res->ncerts) *
					 sizeof(certs_st));
	if (res->certs == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	memset(&res->certs[res->ncerts], 0, sizeof(res->certs[0]));

	res->certs[res->ncerts].cert_list = crt;
	res->certs[res->ncerts].cert_list_length = nr;
	res->certs[res->ncerts].names = names;

	return 0;

}

int
certificate_credentials_append_pkey(gnutls_certificate_credentials_t res,
				    gnutls_privkey_t pkey)
{
	res->pkey = gnutls_realloc_fast(res->pkey,
					(1 + res->ncerts) *
					sizeof(gnutls_privkey_t));
	if (res->pkey == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}
	res->pkey[res->ncerts] = pkey;
	return 0;

}

/**
 * gnutls_certificate_set_x509_key:
 * @res: is a #gnutls_certificate_credentials_t type.
 * @cert_list: contains a certificate list (path) for the specified private key
 * @cert_list_size: holds the size of the certificate list
 * @key: is a #gnutls_x509_privkey_t key
 *
 * This function sets a certificate/private key pair in the
 * gnutls_certificate_credentials_t type.  This function may be
 * called more than once, in case multiple keys/certificates exist for
 * the server.  For clients that wants to send more than their own end
 * entity certificate (e.g., also an intermediate CA cert) then put
 * the certificate chain in @cert_list.
 *
 * Note that the certificates and keys provided, can be safely deinitialized
 * after this function is called.
 *
 * If that function fails to load the @res type is at an undefined state, it must
 * not be reused to load other keys or certificates.
 *
 * Note that, this function by default returns zero on success and a negative value on error.
 * Since 3.5.6, when the flag %GNUTLS_CERTIFICATE_API_V2 is set using gnutls_certificate_set_flags()
 * it returns an index (greater or equal to zero). That index can be used to other functions to refer to the added key-pair.
 *
 * Returns: On success this functions returns zero, and otherwise a negative value on error (see above for modifying that behavior).
 *
 * Since: 2.4.0
 **/
int
gnutls_certificate_set_x509_key(gnutls_certificate_credentials_t res,
				gnutls_x509_crt_t * cert_list,
				int cert_list_size,
				gnutls_x509_privkey_t key)
{
	int ret;
	gnutls_privkey_t pkey;
	gnutls_pcert_st *pcerts = NULL;
	gnutls_str_array_t names;

	_gnutls_str_array_init(&names);

	/* this should be first
	 */
	ret = gnutls_privkey_init(&pkey);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	if (res->pin.cb)
		gnutls_privkey_set_pin_function(pkey, res->pin.cb,
						res->pin.data);

	ret =
	    gnutls_privkey_import_x509(pkey, key,
				       GNUTLS_PRIVKEY_IMPORT_COPY);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	ret = certificate_credentials_append_pkey(res, pkey);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	/* load certificates */
	pcerts = gnutls_malloc(sizeof(gnutls_pcert_st) * cert_list_size);
	if (pcerts == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	ret = get_x509_name(cert_list[0], &names);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret =
		gnutls_pcert_import_x509_list(pcerts, cert_list, (unsigned int*)&cert_list_size,
					      GNUTLS_X509_CRT_LIST_SORT);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret =
	    certificate_credential_append_crt_list(res, names, pcerts,
						   cert_list_size);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	res->ncerts++;

	/* after this point we do not deinitialize anything on failure to avoid
	 * double freeing. We intentionally keep everything as the credentials state
	 * is documented to be on undefined state. */
	if ((ret = _gnutls_check_key_cert_match(res)) < 0) {
		gnutls_assert();
		return ret;
	}

	CRED_RET_SUCCESS(res);

      cleanup:
	gnutls_free(pcerts);
	_gnutls_str_array_clear(&names);
	return ret;
}

/**
 * gnutls_certificate_get_x509_key:
 * @res: is a #gnutls_certificate_credentials_t type.
 * @index: The index of the key to obtain.
 * @key: Location to store the key.
 *
 * Obtains a X.509 private key that has been stored in @res with one of
 * gnutls_certificate_set_x509_key(), gnutls_certificate_set_key(),
 * gnutls_certificate_set_x509_key_file(),
 * gnutls_certificate_set_x509_key_file2(),
 * gnutls_certificate_set_x509_key_mem(), or
 * gnutls_certificate_set_x509_key_mem2(). The returned key must be deallocated
 * with gnutls_x509_privkey_deinit() when no longer needed.
 *
 * The @index matches the return value of gnutls_certificate_set_x509_key() and friends
 * functions, when the %GNUTLS_CERTIFICATE_API_V2 flag is set.
 *
 * If there is no key with the given index,
 * %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE is returned. If the key with the
 * given index is not a X.509 key, %GNUTLS_E_INVALID_REQUEST is returned.
 *
 * Returns: %GNUTLS_E_SUCCESS (0) on success, or a negative error code.
 *
 * Since: 3.4.0
 */
int
gnutls_certificate_get_x509_key(gnutls_certificate_credentials_t res,
				unsigned index,
				gnutls_x509_privkey_t *key)
{
	if (index >= res->ncerts) {
		gnutls_assert();
		return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
	}

	return gnutls_privkey_export_x509(res->pkey[index], key);
}

/**
 * gnutls_certificate_get_x509_crt:
 * @res: is a #gnutls_certificate_credentials_t type.
 * @index: The index of the certificate list to obtain.
 * @crt_list: Where to store the certificate list.
 * @crt_list_size: Will hold the number of certificates.
 *
 * Obtains a X.509 certificate list that has been stored in @res with one of
 * gnutls_certificate_set_x509_key(), gnutls_certificate_set_key(),
 * gnutls_certificate_set_x509_key_file(),
 * gnutls_certificate_set_x509_key_file2(),
 * gnutls_certificate_set_x509_key_mem(), or
 * gnutls_certificate_set_x509_key_mem2(). Each certificate in the returned
 * certificate list must be deallocated with gnutls_x509_crt_deinit(), and the
 * list itself must be freed with gnutls_free().
 *
 * The @index matches the return value of gnutls_certificate_set_x509_key() and friends
 * functions, when the %GNUTLS_CERTIFICATE_API_V2 flag is set.
 *
 * If there is no certificate with the given index,
 * %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE is returned. If the certificate
 * with the given index is not a X.509 certificate, %GNUTLS_E_INVALID_REQUEST
 * is returned. The returned certificates must be deinitialized after
 * use, and the @crt_list pointer must be freed using gnutls_free().
 *
 * Returns: %GNUTLS_E_SUCCESS (0) on success, or a negative error code.
 *
 * Since: 3.4.0
 */
int
gnutls_certificate_get_x509_crt(gnutls_certificate_credentials_t res,
				unsigned index,
				gnutls_x509_crt_t **crt_list,
				unsigned *crt_list_size)
{
	int ret;
	unsigned i;

	if (index >= res->ncerts) {
		gnutls_assert();
		return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
	}

	*crt_list_size = res->certs[index].cert_list_length;
	*crt_list = gnutls_malloc(
		res->certs[index].cert_list_length * sizeof (gnutls_x509_crt_t));
	if (*crt_list == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	for (i = 0; i < res->certs[index].cert_list_length; ++i) {
		ret = gnutls_pcert_export_x509(&res->certs[index].cert_list[i], &(*crt_list)[i]);
		if (ret < 0) {
			while (i--)
				gnutls_x509_crt_deinit((*crt_list)[i]);
			gnutls_free(*crt_list);
			*crt_list = NULL;

			return gnutls_assert_val(ret);
		}
	}

	return 0;
}

/**
 * gnutls_certificate_set_key:
 * @res: is a #gnutls_certificate_credentials_t type.
 * @names: is an array of DNS name of the certificate (NULL if none)
 * @names_size: holds the size of the names list
 * @pcert_list: contains a certificate list (path) for the specified private key
 * @pcert_list_size: holds the size of the certificate list
 * @key: is a #gnutls_privkey_t key
 *
 * This function sets a certificate/private key pair in the
 * gnutls_certificate_credentials_t type.  This function may be
 * called more than once, in case multiple keys/certificates exist for
 * the server.  For clients that want to send more than their own end-
 * entity certificate (e.g., also an intermediate CA cert), the full
 * certificate chain must be provided in @pcert_list.
 *
 * Note that the @key and the elements of @pcert_list will become part of the credentials 
 * structure and must not be deallocated. They will be automatically deallocated 
 * when the @res structure is deinitialized.
 *
 * If that function fails to load the @res structure is at an undefined state, it must
 * not be reused to load other keys or certificates.
 *
 * Note that, this function by default returns zero on success and a negative value on error.
 * Since 3.5.6, when the flag %GNUTLS_CERTIFICATE_API_V2 is set using gnutls_certificate_set_flags()
 * it returns an index (greater or equal to zero). That index can be used to other functions to refer to the added key-pair.
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
			   int pcert_list_size, gnutls_privkey_t key)
{
	int ret, i;
	gnutls_str_array_t str_names;
	gnutls_pcert_st *new_pcert_list;

	_gnutls_str_array_init(&str_names);

	if (names != NULL && names_size > 0) {
		for (i = 0; i < names_size; i++) {
			ret =
			    str_array_append_idna(&str_names, names[i],
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

		ret = get_x509_name(crt, &str_names);
		gnutls_x509_crt_deinit(crt);

		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}
	}

	if (res->pin.cb)
		gnutls_privkey_set_pin_function(key, res->pin.cb,
						res->pin.data);

	ret = certificate_credentials_append_pkey(res, key);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	new_pcert_list = gnutls_malloc(sizeof(gnutls_pcert_st) * pcert_list_size);
	if (new_pcert_list == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}
	memcpy(new_pcert_list, pcert_list, sizeof(gnutls_pcert_st) * pcert_list_size);

	ret =
	    certificate_credential_append_crt_list(res, str_names,
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
 * gnutls_certificate_set_trust_list:
 * @res: is a #gnutls_certificate_credentials_t type.
 * @tlist: is a #gnutls_x509_trust_list_t type
 * @flags: must be zero
 *
 * This function sets a trust list in the gnutls_certificate_credentials_t type. 
 *
 * Note that the @tlist will become part of the credentials 
 * structure and must not be deallocated. It will be automatically deallocated 
 * when the @res structure is deinitialized.
 *
 * Returns: %GNUTLS_E_SUCCESS (0) on success, or a negative error code.
 *
 * Since: 3.2.2
 **/
void
gnutls_certificate_set_trust_list(gnutls_certificate_credentials_t res,
				  gnutls_x509_trust_list_t tlist,
				  unsigned flags)
{
	gnutls_x509_trust_list_deinit(res->tlist, 1);

	res->tlist = tlist;
}

/**
 * gnutls_certificate_get_trust_list:
 * @res: is a #gnutls_certificate_credentials_t type.
 * @tlist: Location where to store the trust list.
 *
 * Obtains the list of trusted certificates stored in @res and writes a
 * pointer to it to the location @tlist. The pointer will point to memory
 * internal to @res, and must not be deinitialized. It will be automatically
 * deallocated when the @res structure is deinitialized.
 *
 * Since: 3.4.0
 **/
void
gnutls_certificate_get_trust_list(gnutls_certificate_credentials_t res,
				  gnutls_x509_trust_list_t *tlist)
{
	*tlist = res->tlist;
}

/**
 * gnutls_certificate_set_x509_key_file:
 * @res: is a #gnutls_certificate_credentials_t type.
 * @certfile: is a file that containing the certificate list (path) for
 *   the specified private key, in PKCS7 format, or a list of certificates
 * @keyfile: is a file that contains the private key
 * @type: is PEM or DER
 *
 * This function sets a certificate/private key pair in the
 * gnutls_certificate_credentials_t type.  This function may be
 * called more than once, in case multiple keys/certificates exist for
 * the server.  For clients that need to send more than its own end
 * entity certificate, e.g., also an intermediate CA cert, then the
 * @certfile must contain the ordered certificate chain.
 *
 * Note that the names in the certificate provided will be considered
 * when selecting the appropriate certificate to use (in case of multiple
 * certificate/key pairs).
 *
 * This function can also accept URLs at @keyfile and @certfile. In that case it
 * will use the private key and certificate indicated by the URLs. Note
 * that the supported URLs are the ones indicated by gnutls_url_is_supported().
 *
 * In case the @certfile is provided as a PKCS #11 URL, then the certificate, and its
 * present issuers in the token are imported (i.e., forming the required trust chain).
 *
 * If that function fails to load the @res structure is at an undefined state, it must
 * not be reused to load other keys or certificates.
 *
 * Note that, this function by default returns zero on success and a negative value on error.
 * Since 3.5.6, when the flag %GNUTLS_CERTIFICATE_API_V2 is set using gnutls_certificate_set_flags()
 * it returns an index (greater or equal to zero). That index can be used to other functions to refer to the added key-pair.
 *
 * Returns: On success this functions returns zero, and otherwise a negative value on error (see above for modifying that behavior).
 *
 * Since: 3.1.11
 **/
int
gnutls_certificate_set_x509_key_file(gnutls_certificate_credentials_t res,
				     const char *certfile,
				     const char *keyfile,
				     gnutls_x509_crt_fmt_t type)
{
	return gnutls_certificate_set_x509_key_file2(res, certfile,
						     keyfile, type, NULL,
						     0);
}

/**
 * gnutls_certificate_set_x509_key_file2:
 * @res: is a #gnutls_certificate_credentials_t type.
 * @certfile: is a file that containing the certificate list (path) for
 *   the specified private key, in PKCS7 format, or a list of certificates
 * @keyfile: is a file that contains the private key
 * @type: is PEM or DER
 * @pass: is the password of the key
 * @flags: an ORed sequence of gnutls_pkcs_encrypt_flags_t
 *
 * This function sets a certificate/private key pair in the
 * gnutls_certificate_credentials_t type.  This function may be
 * called more than once, in case multiple keys/certificates exist for
 * the server.  For clients that need to send more than its own end
 * entity certificate, e.g., also an intermediate CA cert, then the
 * @certfile must contain the ordered certificate chain.
 *
 * Note that the names in the certificate provided will be considered
 * when selecting the appropriate certificate to use (in case of multiple
 * certificate/key pairs).
 *
 * This function can also accept URLs at @keyfile and @certfile. In that case it
 * will use the private key and certificate indicated by the URLs. Note
 * that the supported URLs are the ones indicated by gnutls_url_is_supported().
 * Before GnuTLS 3.4.0 when a URL was specified, the @pass part was ignored and a
 * PIN callback had to be registered, this is no longer the case in current releases.
 *
 * In case the @certfile is provided as a PKCS #11 URL, then the certificate, and its
 * present issuers in the token are imported (i.e., forming the required trust chain).
 *
 * If that function fails to load the @res structure is at an undefined state, it must
 * not be reused to load other keys or certificates.
 *
 * Note that, this function by default returns zero on success and a negative value on error.
 * Since 3.5.6, when the flag %GNUTLS_CERTIFICATE_API_V2 is set using gnutls_certificate_set_flags()
 * it returns an index (greater or equal to zero). That index can be used to other functions to refer to the added key-pair.
 *
 * Returns: On success this functions returns zero, and otherwise a negative value on error (see above for modifying that behavior).
 *
 **/
int
gnutls_certificate_set_x509_key_file2(gnutls_certificate_credentials_t res,
				      const char *certfile,
				      const char *keyfile,
				      gnutls_x509_crt_fmt_t type,
				      const char *pass, unsigned int flags)
{
	int ret;

	/* this should be first
	 */
	if ((ret = read_key_file(res, keyfile, type, pass, flags)) < 0)
		return ret;
	

	if ((ret = read_cert_file(res, certfile, type)) < 0) {
		gnutls_privkey_deinit(res->pkey[res->ncerts]);
		return ret;
	}

	res->ncerts++;

	if ((ret = _gnutls_check_key_cert_match(res)) < 0) {
		gnutls_assert();
		return ret;
	}

	CRED_RET_SUCCESS(res);
}

/**
 * gnutls_certificate_set_x509_trust_mem:
 * @res: is a #gnutls_certificate_credentials_t type.
 * @ca: is a list of trusted CAs or a DER certificate
 * @type: is DER or PEM
 *
 * This function adds the trusted CAs in order to verify client or
 * server certificates. In case of a client this is not required to be
 * called if the certificates are not verified using
 * gnutls_certificate_verify_peers2().  This function may be called
 * multiple times.
 *
 * In case of a server the CAs set here will be sent to the client if
 * a certificate request is sent. This can be disabled using
 * gnutls_certificate_send_x509_rdn_sequence().
 *
 * Returns: the number of certificates processed or a negative error code
 * on error.
 **/
int
gnutls_certificate_set_x509_trust_mem(gnutls_certificate_credentials_t res,
				      const gnutls_datum_t * ca,
				      gnutls_x509_crt_fmt_t type)
{
int ret;

	ret = gnutls_x509_trust_list_add_trust_mem(res->tlist, ca, NULL, 
					type, GNUTLS_TL_USE_IN_TLS, 0);
	if (ret == GNUTLS_E_NO_CERTIFICATE_FOUND)
		return 0;

	return ret;
}

/**
 * gnutls_certificate_set_x509_trust:
 * @res: is a #gnutls_certificate_credentials_t type.
 * @ca_list: is a list of trusted CAs
 * @ca_list_size: holds the size of the CA list
 *
 * This function adds the trusted CAs in order to verify client
 * or server certificates. In case of a client this is not required
 * to be called if the certificates are not verified using
 * gnutls_certificate_verify_peers2().
 * This function may be called multiple times.
 *
 * In case of a server the CAs set here will be sent to the client if
 * a certificate request is sent. This can be disabled using
 * gnutls_certificate_send_x509_rdn_sequence().
 *
 * Returns: the number of certificates processed or a negative error code
 * on error.
 *
 * Since: 2.4.0
 **/
int
gnutls_certificate_set_x509_trust(gnutls_certificate_credentials_t res,
				  gnutls_x509_crt_t * ca_list,
				  int ca_list_size)
{
	int ret, i, j;
	gnutls_x509_crt_t new_list[ca_list_size];

	for (i = 0; i < ca_list_size; i++) {
		ret = gnutls_x509_crt_init(&new_list[i]);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		ret = _gnutls_x509_crt_cpy(new_list[i], ca_list[i]);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}
	}

	ret =
	    gnutls_x509_trust_list_add_cas(res->tlist, new_list,
					   ca_list_size, GNUTLS_TL_USE_IN_TLS);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	return ret;

      cleanup:
	for (j = 0; j < i; j++)
		gnutls_x509_crt_deinit(new_list[j]);

	return ret;
}


/**
 * gnutls_certificate_set_x509_trust_file:
 * @cred: is a #gnutls_certificate_credentials_t type.
 * @cafile: is a file containing the list of trusted CAs (DER or PEM list)
 * @type: is PEM or DER
 *
 * This function adds the trusted CAs in order to verify client or
 * server certificates. In case of a client this is not required to
 * be called if the certificates are not verified using
 * gnutls_certificate_verify_peers2().  This function may be called
 * multiple times.
 *
 * In case of a server the names of the CAs set here will be sent to
 * the client if a certificate request is sent. This can be disabled
 * using gnutls_certificate_send_x509_rdn_sequence().
 *
 * This function can also accept URLs. In that case it
 * will import all certificates that are marked as trusted. Note
 * that the supported URLs are the ones indicated by gnutls_url_is_supported().
 *
 * Returns: the number of certificates processed
 **/
int
gnutls_certificate_set_x509_trust_file(gnutls_certificate_credentials_t
				       cred, const char *cafile,
				       gnutls_x509_crt_fmt_t type)
{
int ret;

	ret = gnutls_x509_trust_list_add_trust_file(cred->tlist, cafile, NULL, 
						type, GNUTLS_TL_USE_IN_TLS, 0);
	if (ret == GNUTLS_E_NO_CERTIFICATE_FOUND)
		return 0;

	return ret;
}

/**
 * gnutls_certificate_set_x509_trust_dir:
 * @cred: is a #gnutls_certificate_credentials_t type.
 * @ca_dir: is a directory containing the list of trusted CAs (DER or PEM list)
 * @type: is PEM or DER
 *
 * This function adds the trusted CAs present in the directory in order to 
 * verify client or server certificates. This function is identical
 * to gnutls_certificate_set_x509_trust_file() but loads all certificates
 * in a directory.
 *
 * Returns: the number of certificates processed
 *
 * Since: 3.3.6
 *
 **/
int
gnutls_certificate_set_x509_trust_dir(gnutls_certificate_credentials_t cred,
				      const char *ca_dir,
				      gnutls_x509_crt_fmt_t type)
{
int ret;

	ret = gnutls_x509_trust_list_add_trust_dir(cred->tlist, ca_dir, NULL, 
						type, GNUTLS_TL_USE_IN_TLS, 0);
	if (ret == GNUTLS_E_NO_CERTIFICATE_FOUND)
		return 0;

	return ret;
}

/**
 * gnutls_certificate_set_x509_system_trust:
 * @cred: is a #gnutls_certificate_credentials_t type.
 *
 * This function adds the system's default trusted CAs in order to
 * verify client or server certificates.
 *
 * In the case the system is currently unsupported %GNUTLS_E_UNIMPLEMENTED_FEATURE
 * is returned.
 *
 * Returns: the number of certificates processed or a negative error code
 * on error.
 *
 * Since: 3.0.20
 **/
int
gnutls_certificate_set_x509_system_trust(gnutls_certificate_credentials_t
					 cred)
{
	return gnutls_x509_trust_list_add_system_trust(cred->tlist, 
					GNUTLS_TL_USE_IN_TLS, 0);
}

/**
 * gnutls_certificate_set_x509_crl_mem:
 * @res: is a #gnutls_certificate_credentials_t type.
 * @CRL: is a list of trusted CRLs. They should have been verified before.
 * @type: is DER or PEM
 *
 * This function adds the trusted CRLs in order to verify client or
 * server certificates.  In case of a client this is not required to
 * be called if the certificates are not verified using
 * gnutls_certificate_verify_peers2().  This function may be called
 * multiple times.
 *
 * Returns: number of CRLs processed, or a negative error code on error.
 **/
int
gnutls_certificate_set_x509_crl_mem(gnutls_certificate_credentials_t res,
				    const gnutls_datum_t * CRL,
				    gnutls_x509_crt_fmt_t type)
{
int ret;

	ret = gnutls_x509_trust_list_add_trust_mem(res->tlist, NULL, CRL, 
					type, GNUTLS_TL_USE_IN_TLS, 0);
	if (ret == GNUTLS_E_NO_CERTIFICATE_FOUND)
		return 0;

	return ret;
}

/**
 * gnutls_certificate_set_x509_crl:
 * @res: is a #gnutls_certificate_credentials_t type.
 * @crl_list: is a list of trusted CRLs. They should have been verified before.
 * @crl_list_size: holds the size of the crl_list
 *
 * This function adds the trusted CRLs in order to verify client or
 * server certificates.  In case of a client this is not required to
 * be called if the certificates are not verified using
 * gnutls_certificate_verify_peers2().  This function may be called
 * multiple times.
 *
 * Returns: number of CRLs processed, or a negative error code on error.
 *
 * Since: 2.4.0
 **/
int
gnutls_certificate_set_x509_crl(gnutls_certificate_credentials_t res,
				gnutls_x509_crl_t * crl_list,
				int crl_list_size)
{
	int ret, i, j;
	gnutls_x509_crl_t new_crl[crl_list_size];

	for (i = 0; i < crl_list_size; i++) {
		ret = gnutls_x509_crl_init(&new_crl[i]);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

		ret = _gnutls_x509_crl_cpy(new_crl[i], crl_list[i]);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}
	}

	ret =
	    gnutls_x509_trust_list_add_crls(res->tlist, new_crl,
				    crl_list_size, GNUTLS_TL_USE_IN_TLS, 0);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	return ret;

      cleanup:
	for (j = 0; j < i; j++)
		gnutls_x509_crl_deinit(new_crl[j]);

	return ret;
}

/**
 * gnutls_certificate_set_x509_crl_file:
 * @res: is a #gnutls_certificate_credentials_t type.
 * @crlfile: is a file containing the list of verified CRLs (DER or PEM list)
 * @type: is PEM or DER
 *
 * This function adds the trusted CRLs in order to verify client or server
 * certificates.  In case of a client this is not required
 * to be called if the certificates are not verified using
 * gnutls_certificate_verify_peers2().
 * This function may be called multiple times.
 *
 * Returns: number of CRLs processed or a negative error code on error.
 **/
int
gnutls_certificate_set_x509_crl_file(gnutls_certificate_credentials_t res,
				     const char *crlfile,
				     gnutls_x509_crt_fmt_t type)
{
int ret;

	ret = gnutls_x509_trust_list_add_trust_file(res->tlist, NULL, crlfile, 
						type, GNUTLS_TL_USE_IN_TLS, 0);
	if (ret == GNUTLS_E_NO_CERTIFICATE_FOUND)
		return 0;

	return ret;
}

#include <gnutls/pkcs12.h>


/**
 * gnutls_certificate_set_x509_simple_pkcs12_file:
 * @res: is a #gnutls_certificate_credentials_t type.
 * @pkcs12file: filename of file containing PKCS#12 blob.
 * @type: is PEM or DER of the @pkcs12file.
 * @password: optional password used to decrypt PKCS#12 file, bags and keys.
 *
 * This function sets a certificate/private key pair and/or a CRL in
 * the gnutls_certificate_credentials_t type.  This function may
 * be called more than once (in case multiple keys/certificates exist
 * for the server).
 *
 * PKCS#12 files with a MAC, encrypted bags and PKCS #8
 * private keys are supported. However,
 * only password based security, and the same password for all
 * operations, are supported.
 *
 * PKCS#12 file may contain many keys and/or certificates, and this
 * function will try to auto-detect based on the key ID the certificate
 * and key pair to use. If the PKCS#12 file contain the issuer of
 * the selected certificate, it will be appended to the certificate
 * to form a chain.
 *
 * If more than one private keys are stored in the PKCS#12 file,
 * then only one key will be read (and it is undefined which one).
 *
 * It is believed that the limitations of this function is acceptable
 * for most usage, and that any more flexibility would introduce
 * complexity that would make it harder to use this functionality at
 * all.
 *
 * Note that, this function by default returns zero on success and a negative value on error.
 * Since 3.5.6, when the flag %GNUTLS_CERTIFICATE_API_V2 is set using gnutls_certificate_set_flags()
 * it returns an index (greater or equal to zero). That index can be used to other functions to refer to the added key-pair.
 *
 * Returns: On success this functions returns zero, and otherwise a negative value on error (see above for modifying that behavior).
 *
 **/
int
 gnutls_certificate_set_x509_simple_pkcs12_file
    (gnutls_certificate_credentials_t res, const char *pkcs12file,
     gnutls_x509_crt_fmt_t type, const char *password) {
	gnutls_datum_t p12blob;
	size_t size;
	int ret;

	p12blob.data = (void *) read_binary_file(pkcs12file, &size);
	p12blob.size = (unsigned int) size;
	if (p12blob.data == NULL) {
		gnutls_assert();
		return GNUTLS_E_FILE_ERROR;
	}

	ret =
	    gnutls_certificate_set_x509_simple_pkcs12_mem(res, &p12blob,
							  type, password);
	free(p12blob.data);

	return ret;
}

/**
 * gnutls_certificate_set_x509_simple_pkcs12_mem:
 * @res: is a #gnutls_certificate_credentials_t type.
 * @p12blob: the PKCS#12 blob.
 * @type: is PEM or DER of the @pkcs12file.
 * @password: optional password used to decrypt PKCS#12 file, bags and keys.
 *
 * This function sets a certificate/private key pair and/or a CRL in
 * the gnutls_certificate_credentials_t type.  This function may
 * be called more than once (in case multiple keys/certificates exist
 * for the server).
 *
 * Encrypted PKCS#12 bags and PKCS#8 private keys are supported.  However,
 * only password based security, and the same password for all
 * operations, are supported.
 *
 * PKCS#12 file may contain many keys and/or certificates, and this
 * function will try to auto-detect based on the key ID the certificate
 * and key pair to use. If the PKCS#12 file contain the issuer of
 * the selected certificate, it will be appended to the certificate
 * to form a chain.
 *
 * If more than one private keys are stored in the PKCS#12 file,
 * then only one key will be read (and it is undefined which one).
 *
 * It is believed that the limitations of this function is acceptable
 * for most usage, and that any more flexibility would introduce
 * complexity that would make it harder to use this functionality at
 * all.
 *
 * Note that, this function by default returns zero on success and a negative value on error.
 * Since 3.5.6, when the flag %GNUTLS_CERTIFICATE_API_V2 is set using gnutls_certificate_set_flags()
 * it returns an index (greater or equal to zero). That index can be used to other functions to refer to the added key-pair.
 *
 * Returns: On success this functions returns zero, and otherwise a negative value on error (see above for modifying that behavior).
 *
 * Since: 2.8.0
 **/
int
 gnutls_certificate_set_x509_simple_pkcs12_mem
    (gnutls_certificate_credentials_t res, const gnutls_datum_t * p12blob,
     gnutls_x509_crt_fmt_t type, const char *password) {
	gnutls_pkcs12_t p12;
	gnutls_x509_privkey_t key = NULL;
	gnutls_x509_crt_t *chain = NULL;
	gnutls_x509_crl_t crl = NULL;
	unsigned int chain_size = 0, i;
	int ret, idx;

	ret = gnutls_pkcs12_init(&p12);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	ret = gnutls_pkcs12_import(p12, p12blob, type, 0);
	if (ret < 0) {
		gnutls_assert();
		gnutls_pkcs12_deinit(p12);
		return ret;
	}

	if (password) {
		ret = gnutls_pkcs12_verify_mac(p12, password);
		if (ret < 0) {
			gnutls_assert();
			gnutls_pkcs12_deinit(p12);
			return ret;
		}
	}

	ret =
	    gnutls_pkcs12_simple_parse(p12, password, &key, &chain,
				       &chain_size, NULL, NULL, &crl, 0);
	gnutls_pkcs12_deinit(p12);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	if (key && chain) {
		ret =
		    gnutls_certificate_set_x509_key(res, chain, chain_size,
						    key);
		if (ret < 0) {
			gnutls_assert();
			goto done;
		}

		idx = ret;
	} else {
		gnutls_assert();
		ret = GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
		goto done;
	}

	if (crl) {
		ret = gnutls_certificate_set_x509_crl(res, &crl, 1);
		if (ret < 0) {
			gnutls_assert();
			goto done;
		}
	}

	if (res->flags & GNUTLS_CERTIFICATE_API_V2)
		ret = idx;
	else
		ret = 0;

      done:
	if (chain) {
		for (i = 0; i < chain_size; i++)
			gnutls_x509_crt_deinit(chain[i]);
		gnutls_free(chain);
	}
	if (key)
		gnutls_x509_privkey_deinit(key);
	if (crl)
		gnutls_x509_crl_deinit(crl);

	return ret;
}



/**
 * gnutls_certificate_free_crls:
 * @sc: is a #gnutls_certificate_credentials_t type.
 *
 * This function will delete all the CRLs associated
 * with the given credentials.
 **/
void gnutls_certificate_free_crls(gnutls_certificate_credentials_t sc)
{
	/* do nothing for now */
	return;
}

/**
 * gnutls_certificate_credentials_t:
 * @cred: is a #gnutls_certificate_credentials_t type.
 * @fn: A PIN callback
 * @userdata: Data to be passed in the callback
 *
 * This function will set a callback function to be used when
 * required to access a protected object. This function overrides any other
 * global PIN functions.
 *
 * Note that this function must be called right after initialization
 * to have effect.
 *
 * Since: 3.1.0
 **/
void gnutls_certificate_set_pin_function(gnutls_certificate_credentials_t
					 cred, gnutls_pin_callback_t fn,
					 void *userdata)
{
	cred->pin.cb = fn;
	cred->pin.data = userdata;
}
