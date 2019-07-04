/*
 * Copyright (C) 2010-2016 Free Software Foundation, Inc.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>
 *
 */

#ifndef GNUTLS_LIB_SYSTEM_H
#define GNUTLS_LIB_SYSTEM_H

#include "gnutls_int.h"
#include <time.h>
#include <sys/time.h>

#ifdef _WIN32
# if defined(__MINGW32__) && !defined(__MINGW64__) && __MINGW32_MAJOR_VERSION <= 3 && __MINGW32_MINOR_VERSION <= 20
#  define NEED_CERT_ENUM_CRLS
typedef PCCRL_CONTEXT WINAPI(*CertEnumCRLsInStoreFunc) (HCERTSTORE
							 hCertStore,
							 PCCRL_CONTEXT
							 pPrevCrlContext);
extern CertEnumCRLsInStoreFunc pCertEnumCRLsInStore;
# else
#  define pCertEnumCRLsInStore CertEnumCRLsInStore
# endif
#include <windows.h>		/* for Sleep */
#else
#include <sys/uio.h>		/* for writev */
#endif

#ifdef _POSIX_PATH_MAX
# define GNUTLS_PATH_MAX _POSIX_PATH_MAX
#else
# define GNUTLS_PATH_MAX 256
#endif

int system_errno(gnutls_transport_ptr_t);

#ifdef _WIN32
ssize_t system_write(gnutls_transport_ptr_t ptr, const void *data,
		     size_t data_size);
#else
#define HAVE_WRITEV
ssize_t system_writev(gnutls_transport_ptr_t ptr, const giovec_t * iovec,
		      int iovec_cnt);
ssize_t system_writev_nosignal(gnutls_transport_ptr_t ptr, const giovec_t * iovec,
		      int iovec_cnt);
ssize_t system_writev_tfo(gnutls_session_t ptr, const giovec_t * iovec,
		      int iovec_cnt);
ssize_t system_writev_nosignal_tfo(gnutls_session_t ptr, const giovec_t * iovec,
		      int iovec_cnt);
#endif
ssize_t system_read(gnutls_transport_ptr_t ptr, void *data,
		    size_t data_size);

#if defined(_WIN32)
# define HAVE_WIN32_LOCKS
#elif defined(HAVE_LIBPTHREAD) || defined(HAVE_PTHREAD_MUTEX_LOCK)
# define HAVE_PTHREAD_LOCKS
#else
# define HAVE_NO_LOCKS
#endif

typedef void (*gnutls_gettime_func) (struct timespec *);

extern gnutls_time_func gnutls_time;
extern gnutls_gettime_func gnutls_gettime;

static inline void millisleep(unsigned int ms)
{
#ifdef _WIN32
	Sleep(ms);
#else
	struct timespec ts;

	ts.tv_sec = 0;
	ts.tv_nsec = ms * 1000 * 1000;

	nanosleep(&ts, NULL);
#endif
}

int _gnutls_find_config_path(char *path, size_t max_size);
int _gnutls_ucs2_to_utf8(const void *data, size_t size,
			 gnutls_datum_t * output, unsigned bigendian);
int _gnutls_utf8_to_ucs2(const void *data, size_t size,
			 gnutls_datum_t * output, unsigned be);

void _gnutls_global_set_gettime_function(gnutls_gettime_func gettime_func);

int gnutls_system_global_init(void);
void gnutls_system_global_deinit(void);

#endif /* GNUTLS_LIB_SYSTEM_H */
