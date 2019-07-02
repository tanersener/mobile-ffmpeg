/*
 * Copyright (C) 2010-2016 Free Software Foundation, Inc.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>
 *
 */

#include <config.h>
#include "gnutls_int.h"
#include "errors.h"

#include <sys/socket.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "system.h"

#ifdef _WIN32
# include <windows.h>
# include <wincrypt.h>

#else /* !_WIN32 */

# include <poll.h>

# if defined(HAVE_GETPWUID_R)
#  include <pwd.h>
# endif
#endif

#ifdef __APPLE__
# include <CoreFoundation/CoreFoundation.h>
# include <Security/Security.h>
# include <Availability.h>
#endif

/* System specific function wrappers for certificate stores.
 */

#define CONFIG_PATH ".gnutls"

/* Returns a path to store user-specific configuration
 * data.
 */
int _gnutls_find_config_path(char *path, size_t max_size)
{
	const char *home_dir = secure_getenv("HOME");

	if (home_dir != NULL && home_dir[0] != 0) {
		snprintf(path, max_size, "%s/" CONFIG_PATH, home_dir);
		return 0;
	}

#ifdef _WIN32
	if (home_dir == NULL || home_dir[0] == '\0') {
		const char *home_drive = getenv("HOMEDRIVE");
		const char *home_path = getenv("HOMEPATH");

		if (home_drive != NULL && home_path != NULL) {
			snprintf(path, max_size, "%s%s\\" CONFIG_PATH, home_drive, home_path);
		} else {
			path[0] = 0;
		}
	}
#elif defined(HAVE_GETPWUID_R)
	if (home_dir == NULL || home_dir[0] == '\0') {
		struct passwd *pwd;
		struct passwd _pwd;
		int ret;
		char tmp[512];

		ret = getpwuid_r(getuid(), &_pwd, tmp, sizeof(tmp), &pwd);
		if (ret == 0 && pwd != NULL) {
			snprintf(path, max_size, "%s/" CONFIG_PATH, pwd->pw_dir);
		} else {
			path[0] = 0;
		}
	}
#else
	if (home_dir == NULL || home_dir[0] == '\0') {
			path[0] = 0;
	}
#endif

	return 0;
}

#if defined(DEFAULT_TRUST_STORE_FILE) || (defined(DEFAULT_TRUST_STORE_PKCS11) && defined(ENABLE_PKCS11))
static
int
add_system_trust(gnutls_x509_trust_list_t list,
		 unsigned int tl_flags, unsigned int tl_vflags)
{
	int ret, r = 0;
	const char *crl_file =
#ifdef DEFAULT_CRL_FILE
	    DEFAULT_CRL_FILE;
#else
	    NULL;
#endif

#if defined(ENABLE_PKCS11) && defined(DEFAULT_TRUST_STORE_PKCS11)
	ret =
	    gnutls_x509_trust_list_add_trust_file(list,
						  DEFAULT_TRUST_STORE_PKCS11,
						  crl_file,
						  GNUTLS_X509_FMT_DER,
						  tl_flags, tl_vflags);
	if (ret > 0)
		r += ret;
#endif

#ifdef DEFAULT_TRUST_STORE_FILE
	ret =
	    gnutls_x509_trust_list_add_trust_file(list,
						  DEFAULT_TRUST_STORE_FILE,
						  crl_file,
						  GNUTLS_X509_FMT_PEM,
						  tl_flags, tl_vflags);
	if (ret > 0)
		r += ret;
#endif

#ifdef DEFAULT_BLACKLIST_FILE
	ret = gnutls_x509_trust_list_remove_trust_file(list, DEFAULT_BLACKLIST_FILE, GNUTLS_X509_FMT_PEM);
	if (ret < 0) {
		_gnutls_debug_log("Could not load blacklist file '%s'\n", DEFAULT_BLACKLIST_FILE);
	}
#endif

	return r;
}
#elif defined(_WIN32)
static
int add_system_trust(gnutls_x509_trust_list_t list, unsigned int tl_flags,
		     unsigned int tl_vflags)
{
	unsigned int i;
	int r = 0;

	for (i = 0; i < 2; i++) {
		HCERTSTORE store;
		const CERT_CONTEXT *cert;
		const CRL_CONTEXT *crl;
		gnutls_datum_t data;

		if (i == 0)
			store = CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, 0, CERT_SYSTEM_STORE_CURRENT_USER , L"ROOT");
		else
			store = CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, 0, CERT_SYSTEM_STORE_CURRENT_USER, L"CA");

		if (store == NULL)
			return GNUTLS_E_FILE_ERROR;

		cert = CertEnumCertificatesInStore(store, NULL);
		crl = pCertEnumCRLsInStore(store, NULL);

		while (cert != NULL) {
			if (cert->dwCertEncodingType == X509_ASN_ENCODING) {
				data.data = cert->pbCertEncoded;
				data.size = cert->cbCertEncoded;
				if (gnutls_x509_trust_list_add_trust_mem
				    (list, &data, NULL,
				     GNUTLS_X509_FMT_DER, tl_flags,
				     tl_vflags) > 0)
					r++;
			}
			cert = CertEnumCertificatesInStore(store, cert);
		}

		while (crl != NULL) {
			if (crl->dwCertEncodingType == X509_ASN_ENCODING) {
				data.data = crl->pbCrlEncoded;
				data.size = crl->cbCrlEncoded;
				gnutls_x509_trust_list_add_trust_mem(list,
								     NULL,
								     &data,
								     GNUTLS_X509_FMT_DER,
								     tl_flags,
								     tl_vflags);
			}
			crl = pCertEnumCRLsInStore(store, crl);
		}
		CertCloseStore(store, 0);
	}

#ifdef DEFAULT_BLACKLIST_FILE
	ret = gnutls_x509_trust_list_remove_trust_file(list, DEFAULT_BLACKLIST_FILE, GNUTLS_X509_FMT_PEM);
	if (ret < 0) {
		_gnutls_debug_log("Could not load blacklist file '%s'\n", DEFAULT_BLACKLIST_FILE);
	}
#endif

	return r;
}
#elif defined(ANDROID) || defined(__ANDROID__) || defined(DEFAULT_TRUST_STORE_DIR)

# include <dirent.h>
# include <unistd.h>

# if defined(ANDROID) || defined(__ANDROID__)
#  define DEFAULT_TRUST_STORE_DIR "/system/etc/security/cacerts/"

static int load_revoked_certs(gnutls_x509_trust_list_t list, unsigned type)
{
	DIR *dirp;
	struct dirent *d;
	int ret;
	int r = 0;
	char path[GNUTLS_PATH_MAX];

	dirp = opendir("/data/misc/keychain/cacerts-removed/");
	if (dirp != NULL) {
		do {
			d = readdir(dirp);
			if (d != NULL && d->d_type == DT_REG) {
				snprintf(path, sizeof(path),
					 "/data/misc/keychain/cacerts-removed/%s",
					 d->d_name);

				ret =
				    gnutls_x509_trust_list_remove_trust_file
				    (list, path, type);
				if (ret >= 0)
					r += ret;
			}
		}
		while (d != NULL);
		closedir(dirp);
	}

	return r;
}
# endif


/* This works on android 4.x 
 */
static
int add_system_trust(gnutls_x509_trust_list_t list, unsigned int tl_flags,
		     unsigned int tl_vflags)
{
	int r = 0, ret;

	ret = gnutls_x509_trust_list_add_trust_dir(list, DEFAULT_TRUST_STORE_DIR,
		NULL, GNUTLS_X509_FMT_PEM, tl_flags, tl_vflags);
	if (ret >= 0)
		r += ret;

# if defined(ANDROID) || defined(__ANDROID__)
	ret = load_revoked_certs(list, GNUTLS_X509_FMT_DER);
	if (ret >= 0)
		r -= ret;

	ret = gnutls_x509_trust_list_add_trust_dir(list, "/data/misc/keychain/cacerts-added/",
		NULL, GNUTLS_X509_FMT_DER, tl_flags, tl_vflags);
	if (ret >= 0)
		r += ret;
# endif

	return r;
}
#elif defined(__APPLE__) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
static
int osstatus_error(status)
{
	CFStringRef err_str = SecCopyErrorMessageString(status, NULL);
	_gnutls_debug_log("Error loading system root certificates: %s\n",
			  CFStringGetCStringPtr(err_str, kCFStringEncodingUTF8));
	CFRelease(err_str);
	return GNUTLS_E_FILE_ERROR;
}

static
int add_system_trust(gnutls_x509_trust_list_t list, unsigned int tl_flags,
		     unsigned int tl_vflags)
{
	int r=0;

	SecTrustSettingsDomain domain[] = { kSecTrustSettingsDomainUser,
					    kSecTrustSettingsDomainAdmin,
					    kSecTrustSettingsDomainSystem };
	for (size_t d=0; d<sizeof(domain)/sizeof(*domain); d++) {
		CFArrayRef certs = NULL;
		OSStatus status = SecTrustSettingsCopyCertificates(domain[d],
								   &certs);
		if (status == errSecNoTrustSettings)
			continue;
		if (status != errSecSuccess)
			return osstatus_error(status);

		int cert_count = CFArrayGetCount(certs);
		for (int i=0; i<cert_count; i++) {
			SecCertificateRef cert =
				(void*)CFArrayGetValueAtIndex(certs, i);
			CFDataRef der;
			status = SecItemExport(cert, kSecFormatX509Cert, 0,
					       NULL, &der);
			if (status != errSecSuccess) {
				CFRelease(der);
				CFRelease(certs);
				return osstatus_error(status);
			}

			if (gnutls_x509_trust_list_add_trust_mem(list,
								 &(gnutls_datum_t) {
									.data = (void*)CFDataGetBytePtr(der),
									.size = CFDataGetLength(der),
								 },
								 NULL,
			                                         GNUTLS_X509_FMT_DER,
								 tl_flags,
								 tl_vflags) > 0)
				r++;
			CFRelease(der);
		}
		CFRelease(certs);
	}

#ifdef DEFAULT_BLACKLIST_FILE
	ret = gnutls_x509_trust_list_remove_trust_file(list, DEFAULT_BLACKLIST_FILE, GNUTLS_X509_FMT_PEM);
	if (ret < 0) {
		_gnutls_debug_log("Could not load blacklist file '%s'\n", DEFAULT_BLACKLIST_FILE);
	}
#endif

	return r;
}
#else

#define add_system_trust(x,y,z) GNUTLS_E_UNIMPLEMENTED_FEATURE

#endif

/**
 * gnutls_x509_trust_list_add_system_trust:
 * @list: The structure of the list
 * @tl_flags: GNUTLS_TL_*
 * @tl_vflags: gnutls_certificate_verify_flags if flags specifies GNUTLS_TL_VERIFY_CRL
 *
 * This function adds the system's default trusted certificate
 * authorities to the trusted list. Note that on unsupported systems
 * this function returns %GNUTLS_E_UNIMPLEMENTED_FEATURE.
 *
 * This function implies the flag %GNUTLS_TL_NO_DUPLICATES.
 *
 * Returns: The number of added elements or a negative error code on error.
 *
 * Since: 3.1
 **/
int
gnutls_x509_trust_list_add_system_trust(gnutls_x509_trust_list_t list,
					unsigned int tl_flags,
					unsigned int tl_vflags)
{
	return add_system_trust(list, tl_flags|GNUTLS_TL_NO_DUPLICATES, tl_vflags);
}

