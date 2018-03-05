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

#include "gnutls_int.h"
#include "errors.h"
#include <auth/cert.h>
#include <x509/common.h>
#include <x509.h>
#include "x509/x509_int.h"
#ifdef ENABLE_OPENPGP
#include "openpgp/openpgp.h"
#endif

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
 * @pcert: The pcert structure
 * @crt: The certificates to be imported
 * @ncrt: The number of certificates
 * @flags: zero or %GNUTLS_X509_CRT_LIST_SORT
 *
 * This convenience function will import the given certificate to a
 * #gnutls_pcert_st structure. The structure must be deinitialized
 * afterwards using gnutls_pcert_deinit();
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
int gnutls_pcert_import_x509_list(gnutls_pcert_st * pcert,
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
		ret = gnutls_pcert_import_x509(&pcert[i], s[i], 0);
		if (ret < 0) {
			current = i;
			goto cleanup;
		}
	}

	return 0;

 cleanup:
	for (i=0;i<current;i++) {
		gnutls_pcert_deinit(&pcert[i]);
	}
	return ret;

}

/**
 * gnutls_pcert_list_import_x509_raw:
 * @pcerts: The structures to store the parsed certificate. Must not be initialized.
 * @pcert_max: Initially must hold the maximum number of certs. It will be updated with the number of certs available.
 * @data: The certificates.
 * @format: One of DER or PEM.
 * @flags: must be (0) or an OR'd sequence of gnutls_certificate_import_flags.
 *
 * This function will convert the given PEM encoded certificate list
 * to the native gnutls_x509_crt_t format. The output will be stored
 * in @certs.  They will be automatically initialized.
 *
 * If the Certificate is PEM encoded it should have a header of "X509
 * CERTIFICATE", or "CERTIFICATE".
 *
 * Returns: the number of certificates read or a negative error value.
 *
 * Since: 3.0
 **/
int
gnutls_pcert_list_import_x509_raw(gnutls_pcert_st * pcerts,
				  unsigned int *pcert_max,
				  const gnutls_datum_t * data,
				  gnutls_x509_crt_fmt_t format,
				  unsigned int flags)
{
	int ret;
	unsigned int i = 0, j;
	gnutls_x509_crt_t *crt;

	crt = gnutls_malloc((*pcert_max) * sizeof(gnutls_x509_crt_t));

	if (crt == NULL)
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

	ret =
	    gnutls_x509_crt_list_import(crt, pcert_max, data, format,
					flags);
	if (ret < 0) {
		ret = gnutls_assert_val(ret);
		goto cleanup_crt;
	}

	for (i = 0; i < *pcert_max; i++) {
		ret = gnutls_pcert_import_x509(&pcerts[i], crt[i], flags);
		if (ret < 0) {
			ret = gnutls_assert_val(ret);
			goto cleanup_pcert;
		}
	}

	ret = 0;
	goto cleanup;

 cleanup_pcert:
	for (j = 0; j < i; j++)
		gnutls_pcert_deinit(&pcerts[j]);

 cleanup:
	for (i = 0; i < *pcert_max; i++)
		gnutls_x509_crt_deinit(crt[i]);
 
 cleanup_crt:
	gnutls_free(crt);
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

#ifdef ENABLE_OPENPGP

/**
 * gnutls_pcert_import_openpgp:
 * @pcert: The pcert structure
 * @crt: The raw certificate to be imported
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
int gnutls_pcert_import_openpgp(gnutls_pcert_st * pcert,
				gnutls_openpgp_crt_t crt,
				unsigned int flags)
{
	int ret;
	size_t sz;

	memset(pcert, 0, sizeof(*pcert));

	pcert->type = GNUTLS_CRT_OPENPGP;
	pcert->cert.data = NULL;

	sz = 0;
	ret =
	    gnutls_openpgp_crt_export(crt, GNUTLS_OPENPGP_FMT_RAW, NULL,
				      &sz);
	if (ret < 0 && ret != GNUTLS_E_SHORT_MEMORY_BUFFER) {
		ret = gnutls_assert_val(ret);
		goto cleanup;
	}

	pcert->cert.data = gnutls_malloc(sz);
	if (pcert->cert.data == NULL) {
		ret = gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);
		goto cleanup;
	}

	ret =
	    gnutls_openpgp_crt_export(crt, GNUTLS_OPENPGP_FMT_RAW,
				      pcert->cert.data, &sz);
	if (ret < 0) {
		ret = gnutls_assert_val(ret);
		goto cleanup;
	}
	pcert->cert.size = sz;

	ret = gnutls_pubkey_init(&pcert->pubkey);
	if (ret < 0) {
		ret = gnutls_assert_val(ret);
		goto cleanup;
	}

	ret = gnutls_pubkey_import_openpgp(pcert->pubkey, crt, 0);
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
 * gnutls_pcert_import_openpgp_raw:
 * @pcert: The pcert structure
 * @cert: The raw certificate to be imported
 * @format: The format of the certificate
 * @keyid: The key ID to use (NULL for the master key)
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
int gnutls_pcert_import_openpgp_raw(gnutls_pcert_st * pcert,
				    const gnutls_datum_t * cert,
				    gnutls_openpgp_crt_fmt_t format,
				    gnutls_openpgp_keyid_t keyid,
				    unsigned int flags)
{
	int ret;
	gnutls_openpgp_crt_t crt;

	memset(pcert, 0, sizeof(*pcert));

	pcert->cert.data = NULL;

	ret = gnutls_openpgp_crt_init(&crt);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = gnutls_openpgp_crt_import(crt, cert, format);
	if (ret < 0) {
		ret = gnutls_assert_val(ret);
		goto cleanup;
	}

	ret = gnutls_openpgp_crt_set_preferred_key_id(crt, keyid);
	if (ret < 0) {
		ret = gnutls_assert_val(ret);
		goto cleanup;
	}

	ret = gnutls_pcert_import_openpgp(pcert, crt, flags);
	if (ret < 0) {
		ret = gnutls_assert_val(ret);
		goto cleanup;
	}
	ret = 0;

      cleanup:
	gnutls_openpgp_crt_deinit(crt);

	return ret;
}

#endif

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

#ifdef ENABLE_OPENPGP

/**
 * gnutls_pcert_export_x509:
 * @pcert: The pcert structure.
 * @crt: An initialized #gnutls_openpgp_crt_t.
 *
 * Converts the given #gnutls_pcert_t type into a #gnutls_openpgp_crt_t.
 * This function only works if the type of @pcert is %GNUTLS_CRT_OPENPGP.
 * When successful, the value written to @crt must be freed with
 * gnutls_openpgp_crt_deinit() when no longer needed.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 * negative error value.
 *
 * Since: 3.4.0
 */
int gnutls_pcert_export_openpgp(gnutls_pcert_st * pcert,
				gnutls_openpgp_crt_t * crt)
{
	int ret;

	if (pcert->type != GNUTLS_CRT_OPENPGP) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	ret = gnutls_openpgp_crt_init(crt);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = gnutls_openpgp_crt_import(*crt, &pcert->cert, GNUTLS_OPENPGP_FMT_RAW);
	if (ret < 0) {
		gnutls_openpgp_crt_deinit(*crt);
		*crt = NULL;

		return gnutls_assert_val(ret);
	}

	return 0;
}

#endif

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
						    &info->
						    raw_certificate_list
						    [0],
						    GNUTLS_X509_FMT_DER,
						    GNUTLS_PCERT_NO_CERT);
#ifdef ENABLE_OPENPGP
	case GNUTLS_CRT_OPENPGP:
		return gnutls_pcert_import_openpgp_raw(pcert,
						       &info->
						       raw_certificate_list
						       [0],
						       GNUTLS_OPENPGP_FMT_RAW,
						       info->subkey_id,
						       GNUTLS_PCERT_NO_CERT);
#endif
	default:
		gnutls_assert();
		return GNUTLS_E_INTERNAL_ERROR;
	}
}
