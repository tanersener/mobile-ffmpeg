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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>
 *
 */

#include "gnutls_int.h"
#include "errors.h"
#include <auth/cert.h>
#include <x509/common.h>
#include <x509.h>
#include "x509/x509_int.h"
#include <gnutls/x509.h>
#include "x509_b64.h"

/**
 * gnutls_pcert_import_x509:
 * @pcert: The pcert structure
 * @crt: The certificate to be imported
 * @flags: zero for now
 *
 * This convenience function will import the given certificate to a
 * #gnutls_pcert_st structure. The structure must be deinitialized
 * afterwards using gnutls_pcert_deinit();
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.0
 **/
int gnutls_pcert_import_x509(gnutls_pcert_st * pcert,
			     gnutls_x509_crt_t crt, unsigned int flags)
{
	int ret;

	memset(pcert, 0, sizeof(*pcert));

	pcert->type = GNUTLS_CRT_X509;
	pcert->cert.data = NULL;

	ret =
	    gnutls_x509_crt_export2(crt, GNUTLS_X509_FMT_DER,
				    &pcert->cert);
	if (ret < 0) {
		ret = gnutls_assert_val(ret);
		goto cleanup;
	}

	ret = gnutls_pubkey_init(&pcert->pubkey);
	if (ret < 0) {
		ret = gnutls_assert_val(ret);
		goto cleanup;
	}

	ret = gnutls_pubkey_import_x509(pcert->pubkey, crt, 0);
	if (ret < 0) {
		gnutls_pubkey_deinit(pcert->pubkey);
		pcert->pubkey = NULL;
		ret = gnutls_assert_val(ret);
		goto cleanup;
	}

	return 0;

      cleanup:
	_gnutls_free_datum(&pcert->cert);

	return ret;
}

/**
 * gnutls_pcert_import_x509_list:
 * @pcert_list: The structures to store the certificates; must not contain initialized #gnutls_pcert_st structures.
 * @crt: The certificates to be imported
 * @ncrt: The number of certificates in @crt; will be updated if necessary
 * @flags: zero or %GNUTLS_X509_CRT_LIST_SORT
 *
 * This convenience function will import the given certificates to an
 * already allocated set of #gnutls_pcert_st structures. The structures must
 * be deinitialized afterwards using gnutls_pcert_deinit(). @pcert_list
 * should contain space for at least @ncrt elements.
 *
 * In the case %GNUTLS_X509_CRT_LIST_SORT is specified and that
 * function cannot sort the list, %GNUTLS_E_CERTIFICATE_LIST_UNSORTED
 * will be returned. Currently sorting can fail if the list size
 * exceeds an internal constraint (16).
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.4.0
 **/
int gnutls_pcert_import_x509_list(gnutls_pcert_st * pcert_list,
				  gnutls_x509_crt_t *crt, unsigned *ncrt,
				  unsigned int flags)
{
	int ret;
	unsigned i;
	unsigned current = 0;
	gnutls_x509_crt_t sorted[DEFAULT_MAX_VERIFY_DEPTH];
	gnutls_x509_crt_t *s;

	s = crt;

	if (flags & GNUTLS_X509_CRT_LIST_SORT && *ncrt > 1) {
		if (*ncrt > DEFAULT_MAX_VERIFY_DEPTH) {
			ret = _gnutls_check_if_sorted(crt, *ncrt);
			if (ret < 0) {
				gnutls_assert();
				return GNUTLS_E_CERTIFICATE_LIST_UNSORTED;
			}
		} else {
			s = _gnutls_sort_clist(sorted, crt, ncrt, NULL);
			if (s == crt) {
				gnutls_assert();
				return GNUTLS_E_UNIMPLEMENTED_FEATURE;
			}
		}
	}

	for (i=0;i<*ncrt;i++) {
		ret = gnutls_pcert_import_x509(&pcert_list[i], s[i], 0);
		if (ret < 0) {
			current = i;
			goto cleanup;
		}
	}

	return 0;

 cleanup:
	for (i=0;i<current;i++) {
		gnutls_pcert_deinit(&pcert_list[i]);
	}
	return ret;

}

/**
 * gnutls_pcert_list_import_x509_raw:
 * @pcert_list: The structures to store the certificates; must not contain initialized #gnutls_pcert_st structures.
 * @pcert_list_size: Initially must hold the maximum number of certs. It will be updated with the number of certs available.
 * @data: The certificates.
 * @format: One of DER or PEM.
 * @flags: must be (0) or an OR'd sequence of gnutls_certificate_import_flags.
 *
 * This function will import the provided DER or PEM encoded certificates to an
 * already allocated set of #gnutls_pcert_st structures. The structures must
 * be deinitialized afterwards using gnutls_pcert_deinit(). @pcert_list
 * should contain space for at least @pcert_list_size elements.
 *
 * If the Certificate is PEM encoded it should have a header of "X509
 * CERTIFICATE", or "CERTIFICATE".
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value; if the @pcert list doesn't have enough space
 *   %GNUTLS_E_SHORT_MEMORY_BUFFER will be returned.
 *
 * Since: 3.0
 **/
int
gnutls_pcert_list_import_x509_raw(gnutls_pcert_st *pcert_list,
				  unsigned int *pcert_list_size,
				  const gnutls_datum_t *data,
				  gnutls_x509_crt_fmt_t format,
				  unsigned int flags)
{
	int ret;
	unsigned int i = 0, j;
	gnutls_x509_crt_t *crt;

	crt = gnutls_malloc((*pcert_list_size) * sizeof(gnutls_x509_crt_t));

	if (crt == NULL)
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

	ret =
	    gnutls_x509_crt_list_import(crt, pcert_list_size, data, format,
					flags);
	if (ret < 0) {
		ret = gnutls_assert_val(ret);
		goto cleanup_crt;
	}

	for (i = 0; i < *pcert_list_size; i++) {
		ret = gnutls_pcert_import_x509(&pcert_list[i], crt[i], flags);
		if (ret < 0) {
			ret = gnutls_assert_val(ret);
			goto cleanup_pcert;
		}
	}

	ret = 0;
	goto cleanup;

 cleanup_pcert:
	for (j = 0; j < i; j++)
		gnutls_pcert_deinit(&pcert_list[j]);

 cleanup:
	for (i = 0; i < *pcert_list_size; i++)
		gnutls_x509_crt_deinit(crt[i]);

 cleanup_crt:
	gnutls_free(crt);
	return ret;

}

/**
 * gnutls_pcert_list_import_x509_url:
 * @pcert_list: The structures to store the certificates; must not contain initialized #gnutls_pcert_st structures.
 * @pcert_list_size: Initially must hold the maximum number of certs. It will be updated with the number of certs available.
 * @file: A file or supported URI with the certificates to load
 * @format: %GNUTLS_X509_FMT_DER or %GNUTLS_X509_FMT_PEM if a file is given
 * @pin_fn: a PIN callback if not globally set
 * @pin_fn_userdata: parameter for the PIN callback
 * @flags: zero or flags from %gnutls_certificate_import_flags
 *
 * This convenience function will import a certificate chain from the given
 * file or supported URI to #gnutls_pcert_st structures. The structures
 * must be deinitialized afterwards using gnutls_pcert_deinit().
 *
 * This function will always return a sorted certificate chain.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value; if the @pcert list doesn't have enough space
 *   %GNUTLS_E_SHORT_MEMORY_BUFFER will be returned.
 *
 * Since: 3.6.3
 **/
int gnutls_pcert_list_import_x509_file(gnutls_pcert_st *pcert_list,
				       unsigned *pcert_list_size,
				       const char *file,
				       gnutls_x509_crt_fmt_t format,
				       gnutls_pin_callback_t pin_fn,
				       void *pin_fn_userdata,
				       unsigned int flags)
{
	int ret, ret2;
	unsigned i;
	gnutls_x509_crt_t *crts = NULL;
	unsigned crts_size = 0;
	gnutls_datum_t data = {NULL, 0};

	if (gnutls_url_is_supported(file) != 0) {
		ret = gnutls_x509_crt_list_import_url(&crts, &crts_size, file, pin_fn, pin_fn_userdata, 0);
		if (ret < 0) {
			ret2 = gnutls_x509_crt_list_import_url(&crts, &crts_size, file, pin_fn, pin_fn_userdata, GNUTLS_PKCS11_OBJ_FLAG_LOGIN);
			if (ret2 >= 0) ret = ret2;
		}

		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}

	} else { /* file */
		ret = gnutls_load_file(file, &data);
		if (ret < 0)
			return gnutls_assert_val(ret);

		ret = gnutls_x509_crt_list_import2(&crts, &crts_size, &data, format, flags|GNUTLS_X509_CRT_LIST_SORT);
		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}
	}

	if (crts_size > *pcert_list_size) {
		gnutls_assert();
		ret = GNUTLS_E_SHORT_MEMORY_BUFFER;
		goto cleanup;
	}

	ret = gnutls_pcert_import_x509_list(pcert_list, crts, &crts_size, flags);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}
	*pcert_list_size = crts_size;

	ret = 0;
cleanup:
	for (i=0;i<crts_size;i++)
		gnutls_x509_crt_deinit(crts[i]);
	gnutls_free(crts);
	gnutls_free(data.data);
	return ret;
}


/**
 * gnutls_pcert_import_x509_raw:
 * @pcert: The pcert structure
 * @cert: The raw certificate to be imported
 * @format: The format of the certificate
 * @flags: zero for now
 *
 * This convenience function will import the given certificate to a
 * #gnutls_pcert_st structure. The structure must be deinitialized
 * afterwards using gnutls_pcert_deinit();
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.0
 **/
int gnutls_pcert_import_x509_raw(gnutls_pcert_st * pcert,
				 const gnutls_datum_t * cert,
				 gnutls_x509_crt_fmt_t format,
				 unsigned int flags)
{
	int ret;
	gnutls_x509_crt_t crt;

	memset(pcert, 0, sizeof(*pcert));

	ret = gnutls_x509_crt_init(&crt);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = gnutls_x509_crt_import(crt, cert, format);
	if (ret < 0) {
		ret = gnutls_assert_val(ret);
		goto cleanup;
	}

	ret = gnutls_pcert_import_x509(pcert, crt, flags);
	if (ret < 0) {
		ret = gnutls_assert_val(ret);
		goto cleanup;
	}

	ret = 0;

      cleanup:
	gnutls_x509_crt_deinit(crt);

	return ret;
}

/**
 * gnutls_pcert_import_rawpk:
 * @pcert: The pcert structure to import the data into.
 * @pubkey: The raw public-key in #gnutls_pubkey_t format to be imported
 * @flags: zero for now
 *
 * This convenience function will import (i.e. convert) the given raw
 * public key @pubkey into a #gnutls_pcert_st structure. The structure
 * must be deinitialized afterwards using gnutls_pcert_deinit(). The
 * given @pubkey must not be deinitialized because it will be associated
 * with the given @pcert structure and will be deinitialized with it.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.6.6
 **/
int gnutls_pcert_import_rawpk(gnutls_pcert_st* pcert,
			     gnutls_pubkey_t pubkey, unsigned int flags)
{
	int ret;

	if (pubkey == NULL) {
		return gnutls_assert_val(GNUTLS_E_INSUFFICIENT_CREDENTIALS);
	}

	memset(pcert, 0, sizeof(*pcert));

	/* A pcert struct holds a raw copy of the certificate data.
	 * Therefore we convert our gnutls_pubkey_t to its raw DER
	 * representation and copy it into our pcert. It is this raw data
	 * that will be transferred to the peer via a Certificate msg.
	 * According to the spec (RFC7250) a DER representation must be used.
	 */
	ret = gnutls_pubkey_export2(pubkey, GNUTLS_X509_FMT_DER, &pcert->cert);
	if (ret < 0) {
		return gnutls_assert_val(ret);
	}

	pcert->pubkey = pubkey;

	pcert->type = GNUTLS_CRT_RAWPK;

	return GNUTLS_E_SUCCESS;
}

/**
 * gnutls_pcert_import_rawpk_raw:
 * @pcert: The pcert structure to import the data into.
 * @rawpubkey: The raw public-key in #gnutls_datum_t format to be imported.
 * @format: The format of the raw public-key. DER or PEM.
 * @key_usage: An ORed sequence of %GNUTLS_KEY_* flags.
 * @flags: zero for now
 *
 * This convenience function will import (i.e. convert) the given raw
 * public key @rawpubkey into a #gnutls_pcert_st structure. The structure
 * must be deinitialized afterwards using gnutls_pcert_deinit().
 * Note that the caller is responsible for freeing @rawpubkey. All necessary
 * values will be copied into @pcert.
 *
 * Key usage (as defined by X.509 extension (2.5.29.15)) can be explicitly
 * set because there is no certificate structure around the key to define
 * this value. See for more info gnutls_x509_crt_get_key_usage().
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 *
 * Since: 3.6.6
 **/
int gnutls_pcert_import_rawpk_raw(gnutls_pcert_st* pcert,
				    const gnutls_datum_t* rawpubkey,
				    gnutls_x509_crt_fmt_t format,
				    unsigned int key_usage, unsigned int flags)
{
	int ret;

	if (rawpubkey == NULL) {
		return gnutls_assert_val(GNUTLS_E_INSUFFICIENT_CREDENTIALS);
	}

	memset(pcert, 0, sizeof(*pcert));

	ret = gnutls_pubkey_init(&pcert->pubkey);
	if (ret < 0) {
		return gnutls_assert_val(ret);
	}

	// Convert our raw public-key to a gnutls_pubkey_t structure
	ret = gnutls_pubkey_import(pcert->pubkey, rawpubkey, format);
	if (ret < 0) {
		return gnutls_assert_val(ret);
	}

	pcert->pubkey->key_usage = key_usage;

	/* A pcert struct holds a raw copy of the certificate data.
	 * It is this raw data that will be transferred to the peer via a
	 * Certificate message. According to the spec (RFC7250) a DER
	 * representation must be used. Therefore we check the format and
	 * convert if necessary.
	 */
	if (format == GNUTLS_X509_FMT_PEM) {
		ret = _gnutls_fbase64_decode(PEM_PK,
					rawpubkey->data, rawpubkey->size,
					&pcert->cert);

		if (ret < 0) {
			gnutls_pubkey_deinit(pcert->pubkey);

			return gnutls_assert_val(ret);
		}
	} else {
		// Directly copy the raw DER data to our pcert
		ret = _gnutls_set_datum(&pcert->cert, rawpubkey->data, rawpubkey->size);

		if (ret < 0) {
			gnutls_pubkey_deinit(pcert->pubkey);

			return gnutls_assert_val(ret);
		}
	}

	pcert->type = GNUTLS_CRT_RAWPK;

	return GNUTLS_E_SUCCESS;
}

/**
 * gnutls_pcert_export_x509:
 * @pcert: The pcert structure.
 * @crt: An initialized #gnutls_x509_crt_t.
 *
 * Converts the given #gnutls_pcert_t type into a #gnutls_x509_crt_t.
 * This function only works if the type of @pcert is %GNUTLS_CRT_X509.
 * When successful, the value written to @crt must be freed with
 * gnutls_x509_crt_deinit() when no longer needed.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 * negative error value.
 *
 * Since: 3.4.0
 */
int gnutls_pcert_export_x509(gnutls_pcert_st * pcert,
			     gnutls_x509_crt_t * crt)
{
	int ret;

	if (pcert->type != GNUTLS_CRT_X509) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	ret = gnutls_x509_crt_init(crt);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = gnutls_x509_crt_import(*crt, &pcert->cert, GNUTLS_X509_FMT_DER);
	if (ret < 0) {
		gnutls_x509_crt_deinit(*crt);
		*crt = NULL;

		return gnutls_assert_val(ret);
	}

	return 0;
}

/**
 * gnutls_pcert_deinit:
 * @pcert: The structure to be deinitialized
 *
 * This function will deinitialize a pcert structure.
 *
 * Since: 3.0
 **/
void gnutls_pcert_deinit(gnutls_pcert_st * pcert)
{
	if (pcert->pubkey)
		gnutls_pubkey_deinit(pcert->pubkey);
	pcert->pubkey = NULL;
	_gnutls_free_datum(&pcert->cert);
}

/* Converts the first certificate for the cert_auth_info structure
 * to a pcert.
 */
int
_gnutls_get_auth_info_pcert(gnutls_pcert_st * pcert,
			    gnutls_certificate_type_t type,
			    cert_auth_info_t info)
{
	switch (type) {
		case GNUTLS_CRT_X509:
			return gnutls_pcert_import_x509_raw(pcert,
							&info->raw_certificate_list[0],
							GNUTLS_X509_FMT_DER,
							0);
		case GNUTLS_CRT_RAWPK:
			return gnutls_pcert_import_rawpk_raw(pcert,
							&info->raw_certificate_list[0],
							GNUTLS_X509_FMT_DER,
							0, 0);
		default:
			return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);
	}
}
