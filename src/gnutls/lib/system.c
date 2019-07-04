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
#include <system.h>
#include "gnutls_int.h"
#include "errors.h"

#include <sys/socket.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#ifdef _WIN32
# include <windows.h>
# include <wincrypt.h>
# if defined(NEED_CERT_ENUM_CRLS)
CertEnumCRLsInStoreFunc pCertEnumCRLsInStore;
static HMODULE Crypt32_dll;
# endif
#endif

/* System specific function wrappers for certificate stores.
 */
gnutls_time_func gnutls_time;
gnutls_gettime_func gnutls_gettime;

/* emulate gnulib's gettime using gettimeofday to avoid linking to
 * librt */
static void _gnutls_gettime(struct timespec *t)
{
#if defined(HAVE_CLOCK_GETTIME) && defined(CLOCK_REALTIME)
	clock_gettime(CLOCK_REALTIME, t);
#else
	struct timeval tv;
	gettimeofday(&tv, NULL);
	t->tv_sec = tv.tv_sec;
	t->tv_nsec = tv.tv_usec * 1000;
#endif
}

void _gnutls_global_set_gettime_function(gnutls_gettime_func gettime_func)
{
	gnutls_gettime = gettime_func;
}

int gnutls_system_global_init(void)
{
#if defined(_WIN32) && defined(NEED_CERT_ENUM_CRLS)
	/* used in system/certs.c */
	HMODULE crypto;
	crypto = LoadLibrary(TEXT("Crypt32.dll"));

	if (crypto == NULL)
		return GNUTLS_E_CRYPTO_INIT_FAILED;

	pCertEnumCRLsInStore =
	    (CertEnumCRLsInStoreFunc) GetProcAddress(crypto,
						      "CertEnumCRLsInStore");
	if (pCertEnumCRLsInStore == NULL) {
		FreeLibrary(crypto);
		return GNUTLS_E_CRYPTO_INIT_FAILED;
	}

	Crypt32_dll = crypto;
#endif
	gnutls_time = time;
	gnutls_gettime = _gnutls_gettime;
	return 0;
}

void gnutls_system_global_deinit(void)
{
#if defined(_WIN32) && defined(NEED_CERT_ENUM_CRLS)
	FreeLibrary(Crypt32_dll);
#endif
	gnutls_time = time;
	gnutls_gettime = _gnutls_gettime;
}


