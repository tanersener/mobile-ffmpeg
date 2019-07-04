/*
 * Copyright (C) 2012-2014 Free Software Foundation, Inc.
 * Copyright (C) 2014 Nikos Mavrogiannopoulos
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
#include <libtasn1.h>
#include <global.h>
#include <num.h>
#include <tls-sig.h>
#include <str.h>
#include <c-strcase.h>
#include <datum.h>
#include "x509_int.h"
#include <common.h>
#include "verify-high.h"
#include "read-file.h"
#include <pkcs11_int.h>
#include "urls.h"

#include <dirent.h>

#if !defined(_DIRENT_HAVE_D_TYPE) && !defined(__native_client__)
# ifdef DT_UNKNOWN
#  define _DIRENT_HAVE_D_TYPE
# endif
#endif

#ifdef _WIN32
# include <tchar.h>
#endif

/* Convenience functions for verify-high functionality 
 */

/**
 * gnutls_x509_trust_list_add_trust_mem:
 * @list: The list
 * @cas: A buffer containing a list of CAs (optional)
 * @crls: A buffer containing a list of CRLs (optional)
 * @type: The format of the certificates
 * @tl_flags: flags from %gnutls_trust_list_flags_t
 * @tl_vflags: gnutls_certificate_verify_flags if flags specifies GNUTLS_TL_VERIFY_CRL
 *
 * This function will add the given certificate authorities
 * to the trusted list. 
 *
 * If this function is used gnutls_x509_trust_list_deinit() must be called
 * with parameter @all being 1.
 *
 * Returns: The number of added elements is returned.
 *
 * Since: 3.1
 **/
int
gnutls_x509_trust_list_add_trust_mem(gnutls_x509_trust_list_t list,
				     const gnutls_datum_t * cas,
				     const gnutls_datum_t * crls,
				     gnutls_x509_crt_fmt_t type,
				     unsigned int tl_flags,
				     unsigned int tl_vflags)
{
	int ret;
	gnutls_x509_crt_t *x509_ca_list = NULL;
	gnutls_x509_crl_t *x509_crl_list = NULL;
	unsigned int x509_ncas, x509_ncrls;
	unsigned int r = 0;

	/* When adding CAs or CRLs, we use the GNUTLS_TL_NO_DUPLICATES flag to ensure
	 * that unaccounted certificates/CRLs are deinitialized. */

	if (cas != NULL && cas->data != NULL) {
		ret =
		    gnutls_x509_crt_list_import2(&x509_ca_list, &x509_ncas,
						 cas, type, 0);
		if (ret < 0)
			return gnutls_assert_val(ret);

		ret =
		    gnutls_x509_trust_list_add_cas(list, x509_ca_list,
						   x509_ncas, tl_flags|GNUTLS_TL_NO_DUPLICATES);
		gnutls_free(x509_ca_list);

		if (ret < 0)
			return gnutls_assert_val(ret);
		else
			r += ret;
	}

	if (crls != NULL && crls->data != NULL) {
		ret =
		    gnutls_x509_crl_list_import2(&x509_crl_list,
						 &x509_ncrls, crls, type,
						 0);
		if (ret < 0)
			return gnutls_assert_val(ret);

		ret =
		    gnutls_x509_trust_list_add_crls(list, x509_crl_list,
						    x509_ncrls, tl_flags|GNUTLS_TL_NO_DUPLICATES,
						    tl_vflags);
		gnutls_free(x509_crl_list);

		if (ret < 0)
			return gnutls_assert_val(ret);
		else
			r += ret;
	}

	return r;
}

/**
 * gnutls_x509_trust_list_remove_trust_mem:
 * @list: The list
 * @cas: A buffer containing a list of CAs (optional)
 * @type: The format of the certificates
 *
 * This function will remove the provided certificate authorities
 * from the trusted list, and add them into a black list when needed. 
 *
 * See also gnutls_x509_trust_list_remove_cas().
 *
 * Returns: The number of removed elements is returned.
 *
 * Since: 3.1.10
 **/
int
gnutls_x509_trust_list_remove_trust_mem(gnutls_x509_trust_list_t list,
					const gnutls_datum_t * cas,
					gnutls_x509_crt_fmt_t type)
{
	int ret;
	gnutls_x509_crt_t *x509_ca_list = NULL;
	unsigned int x509_ncas;
	unsigned int r = 0, i;

	if (cas != NULL && cas->data != NULL) {
		ret =
		    gnutls_x509_crt_list_import2(&x509_ca_list, &x509_ncas,
						 cas, type, 0);
		if (ret < 0)
			return gnutls_assert_val(ret);

		ret =
		    gnutls_x509_trust_list_remove_cas(list, x509_ca_list,
						      x509_ncas);

		for (i = 0; i < x509_ncas; i++)
			gnutls_x509_crt_deinit(x509_ca_list[i]);
		gnutls_free(x509_ca_list);

		if (ret < 0)
			return gnutls_assert_val(ret);
		else
			r += ret;
	}

	return r;
}

#ifdef ENABLE_PKCS11
static
int remove_pkcs11_url(gnutls_x509_trust_list_t list, const char *ca_file)
{
	if (strcmp(ca_file, list->pkcs11_token) == 0) {
		gnutls_free(list->pkcs11_token);
	}
	return 0;
}

/* This function does add a PKCS #11 object URL into trust list. The
 * CA certificates are imported directly, rather than using it as a
 * trusted PKCS#11 token.
 */
static
int add_trust_list_pkcs11_object_url(gnutls_x509_trust_list_t list, const char *url, unsigned flags)
{
	gnutls_x509_crt_t *xcrt_list = NULL;
	gnutls_pkcs11_obj_t *pcrt_list = NULL;
	unsigned int pcrt_list_size = 0, i;
	int ret;

	/* here we don't use the flag GNUTLS_PKCS11_OBJ_FLAG_PRESENT_IN_TRUSTED_MODULE,
	 * as we want to explicitly load from any module available in the system.
	 */
	ret =
	    gnutls_pkcs11_obj_list_import_url2(&pcrt_list, &pcrt_list_size,
					       url,
					       GNUTLS_PKCS11_OBJ_FLAG_CRT|GNUTLS_PKCS11_OBJ_FLAG_MARK_TRUSTED,
					       0);
	if (ret < 0)
		return gnutls_assert_val(ret);

	if (pcrt_list_size == 0) {
		ret = 0;
		goto cleanup;
	}

	xcrt_list = gnutls_malloc(sizeof(gnutls_x509_crt_t) * pcrt_list_size);
	if (xcrt_list == NULL) {
		ret = GNUTLS_E_MEMORY_ERROR;
		goto cleanup;
	}

	ret =
	    gnutls_x509_crt_list_import_pkcs11(xcrt_list, pcrt_list_size,
					       pcrt_list, 0);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret =
	    gnutls_x509_trust_list_add_cas(list, xcrt_list, pcrt_list_size,
					   flags);

 cleanup:
	for (i = 0; i < pcrt_list_size; i++)
		gnutls_pkcs11_obj_deinit(pcrt_list[i]);
	gnutls_free(pcrt_list);
	gnutls_free(xcrt_list);

	return ret;
}

static
int remove_pkcs11_object_url(gnutls_x509_trust_list_t list, const char *url)
{
	gnutls_x509_crt_t *xcrt_list = NULL;
	gnutls_pkcs11_obj_t *pcrt_list = NULL;
	unsigned int pcrt_list_size = 0, i;
	int ret;

	ret =
	    gnutls_pkcs11_obj_list_import_url2(&pcrt_list, &pcrt_list_size,
					       url,
					       GNUTLS_PKCS11_OBJ_FLAG_CRT|GNUTLS_PKCS11_OBJ_FLAG_MARK_TRUSTED,
					       0);
	if (ret < 0)
		return gnutls_assert_val(ret);

	if (pcrt_list_size == 0) {
		ret = 0;
		goto cleanup;
	}

	xcrt_list = gnutls_malloc(sizeof(gnutls_x509_crt_t) * pcrt_list_size);
	if (xcrt_list == NULL) {
		ret = GNUTLS_E_MEMORY_ERROR;
		goto cleanup;
	}

	ret =
	    gnutls_x509_crt_list_import_pkcs11(xcrt_list, pcrt_list_size,
					       pcrt_list, 0);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret =
	    gnutls_x509_trust_list_remove_cas(list, xcrt_list, pcrt_list_size);

 cleanup:
	for (i = 0; i < pcrt_list_size; i++) {
		gnutls_pkcs11_obj_deinit(pcrt_list[i]);
		if (xcrt_list)
			gnutls_x509_crt_deinit(xcrt_list[i]);
	}
	gnutls_free(pcrt_list);
	gnutls_free(xcrt_list);

	return ret;
}
#endif


/**
 * gnutls_x509_trust_list_add_trust_file:
 * @list: The list
 * @ca_file: A file containing a list of CAs (optional)
 * @crl_file: A file containing a list of CRLs (optional)
 * @type: The format of the certificates
 * @tl_flags: flags from %gnutls_trust_list_flags_t
 * @tl_vflags: gnutls_certificate_verify_flags if flags specifies GNUTLS_TL_VERIFY_CRL
 *
 * This function will add the given certificate authorities
 * to the trusted list. PKCS #11 URLs are also accepted, instead
 * of files, by this function. A PKCS #11 URL implies a trust
 * database (a specially marked module in p11-kit); the URL "pkcs11:"
 * implies all trust databases in the system. Only a single URL specifying
 * trust databases can be set; they cannot be stacked with multiple calls.
 *
 * Returns: The number of added elements is returned.
 *
 * Since: 3.1
 **/
int
gnutls_x509_trust_list_add_trust_file(gnutls_x509_trust_list_t list,
				      const char *ca_file,
				      const char *crl_file,
				      gnutls_x509_crt_fmt_t type,
				      unsigned int tl_flags,
				      unsigned int tl_vflags)
{
	gnutls_datum_t cas = { NULL, 0 };
	gnutls_datum_t crls = { NULL, 0 };
	size_t size;
	int ret;

	if (ca_file != NULL) {
#ifdef ENABLE_PKCS11
		if (c_strncasecmp(ca_file, PKCS11_URL, PKCS11_URL_SIZE) == 0) {
			unsigned pcrt_list_size = 0;

			/* in case of a token URL import it as a PKCS #11 token,
			 * otherwise import the individual certificates.
			 */
			if (is_pkcs11_url_object(ca_file) != 0) {
				return add_trust_list_pkcs11_object_url(list, ca_file, tl_flags);
			} else { /* trusted token */
				if (list->pkcs11_token != NULL)
					return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);
				list->pkcs11_token = gnutls_strdup(ca_file);

				/* enumerate the certificates */
				ret = gnutls_pkcs11_obj_list_import_url(NULL, &pcrt_list_size,
					ca_file, 
					(GNUTLS_PKCS11_OBJ_FLAG_PRESENT_IN_TRUSTED_MODULE|GNUTLS_PKCS11_OBJ_FLAG_CRT|GNUTLS_PKCS11_OBJ_FLAG_MARK_CA|GNUTLS_PKCS11_OBJ_FLAG_MARK_TRUSTED),
					0);
				if (ret < 0 && ret != GNUTLS_E_SHORT_MEMORY_BUFFER)
					return gnutls_assert_val(ret);

				return pcrt_list_size;
			}
		} else
#endif
		{
			cas.data = (void *) read_binary_file(ca_file, &size);
			if (cas.data == NULL) {
				gnutls_assert();
				return GNUTLS_E_FILE_ERROR;
			}
			cas.size = size;
		}
	}

	if (crl_file) {
		crls.data = (void *) read_binary_file(crl_file, &size);
		if (crls.data == NULL) {
			gnutls_assert();
			return GNUTLS_E_FILE_ERROR;
		}
		crls.size = size;
	}

	ret =
	    gnutls_x509_trust_list_add_trust_mem(list, &cas, &crls, type,
						 tl_flags, tl_vflags);
	free(crls.data);
	free(cas.data);

	return ret;
}

static
int load_dir_certs(const char *dirname,
			  gnutls_x509_trust_list_t list,
			  unsigned int tl_flags, unsigned int tl_vflags,
			  unsigned type, unsigned crl)
{
	int ret;
	int r = 0;
	char path[GNUTLS_PATH_MAX];

#if !defined(_WIN32) || !defined(_UNICODE)
	DIR *dirp;
	struct dirent *d;

	dirp = opendir(dirname);
	if (dirp != NULL) {
		while ((d = readdir(dirp)) != NULL) {
#ifdef _DIRENT_HAVE_D_TYPE
				if (d->d_type == DT_REG || d->d_type == DT_LNK || d->d_type == DT_UNKNOWN)
#endif
			{
				snprintf(path, sizeof(path), "%s/%s",
					 dirname, d->d_name);

				if (crl != 0) {
					ret =
					    gnutls_x509_trust_list_add_trust_file
					    (list, NULL, path, type, tl_flags,
					     tl_vflags);
				} else {
					ret =
					    gnutls_x509_trust_list_add_trust_file
					    (list, path, NULL, type, tl_flags,
					     tl_vflags);
				}
				if (ret >= 0)
					r += ret;
			}
		}
		closedir(dirp);
	}
#else /* _WIN32 */

	_TDIR *dirp;
	struct _tdirent *d;
	gnutls_datum_t utf16 = {NULL, 0};

#ifdef WORDS_BIGENDIAN
	r = _gnutls_utf8_to_ucs2(dirname, strlen(dirname), &utf16, 1);
#else
	r = _gnutls_utf8_to_ucs2(dirname, strlen(dirname), &utf16, 0);
#endif
	if (r < 0)
		return gnutls_assert_val(r);
	dirp = _topendir((_TCHAR*)utf16.data);
	gnutls_free(utf16.data);
	if (dirp != NULL) {
		while ((d = _treaddir(dirp)) != NULL) {
#ifdef _DIRENT_HAVE_D_TYPE
			if (d->d_type == DT_REG || d->d_type == DT_LNK || d->d_type == DT_UNKNOWN)
#endif
			{
				snprintf(path, sizeof(path), "%s/%ls",
					dirname, d->d_name);

				if (crl != 0) {
					ret =
					    gnutls_x509_trust_list_add_trust_file
					    (list, NULL, path, type, tl_flags,
					     tl_vflags);
				} else {
					ret =
					    gnutls_x509_trust_list_add_trust_file
					    (list, path, NULL, type, tl_flags,
					     tl_vflags);
				}
				if (ret >= 0)
					r += ret;
			}
		}
		_tclosedir(dirp);
	}
#endif /* _WIN32 */
	return r;
}

/**
 * gnutls_x509_trust_list_add_trust_dir:
 * @list: The list
 * @ca_dir: A directory containing the CAs (optional)
 * @crl_dir: A directory containing a list of CRLs (optional)
 * @type: The format of the certificates
 * @tl_flags: flags from %gnutls_trust_list_flags_t
 * @tl_vflags: gnutls_certificate_verify_flags if flags specifies GNUTLS_TL_VERIFY_CRL
 *
 * This function will add the given certificate authorities
 * to the trusted list. Only directories are accepted by
 * this function.
 *
 * Returns: The number of added elements is returned.
 *
 * Since: 3.3.6
 **/
int
gnutls_x509_trust_list_add_trust_dir(gnutls_x509_trust_list_t list,
				      const char *ca_dir,
				      const char *crl_dir,
				      gnutls_x509_crt_fmt_t type,
				      unsigned int tl_flags,
				      unsigned int tl_vflags)
{
	int ret = 0;

	if (ca_dir != NULL) {
		int r = 0;
		r = load_dir_certs(ca_dir, list, tl_flags, tl_vflags, type, 0);

		if (r >= 0)
			ret += r;
	}

	if (crl_dir) {
		int r = 0;
		r = load_dir_certs(crl_dir, list, tl_flags, tl_vflags, type, 1);

		if (r >= 0)
			ret += r;
	}

	return ret;
}

/**
 * gnutls_x509_trust_list_remove_trust_file:
 * @list: The list
 * @ca_file: A file containing a list of CAs
 * @type: The format of the certificates
 *
 * This function will remove the given certificate authorities
 * from the trusted list, and add them into a black list when needed. 
 * PKCS 11 URLs are also accepted, instead
 * of files, by this function.
 *
 * See also gnutls_x509_trust_list_remove_cas().
 *
 * Returns: The number of added elements is returned.
 *
 * Since: 3.1.10
 **/
int
gnutls_x509_trust_list_remove_trust_file(gnutls_x509_trust_list_t list,
					 const char *ca_file,
					 gnutls_x509_crt_fmt_t type)
{
	gnutls_datum_t cas = { NULL, 0 };
	size_t size;
	int ret;

#ifdef ENABLE_PKCS11
	if (c_strncasecmp(ca_file, PKCS11_URL, PKCS11_URL_SIZE) == 0) {
		if (is_pkcs11_url_object(ca_file) != 0) {
			return remove_pkcs11_object_url(list, ca_file);
		} else { /* token */
			return remove_pkcs11_url(list, ca_file);
		}
	} else
#endif
	{
		cas.data = (void *) read_binary_file(ca_file, &size);
		if (cas.data == NULL) {
			gnutls_assert();
			return GNUTLS_E_FILE_ERROR;
		}
		cas.size = size;
	}

	ret = gnutls_x509_trust_list_remove_trust_mem(list, &cas, type);
	free(cas.data);

	return ret;
}
