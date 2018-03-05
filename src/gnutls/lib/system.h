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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 */

#ifndef SYSTEM_H
#define SYSTEM_H

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

extern gnutls_time_func gnutls_time;

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

/* emulate gnulib's gettime using gettimeofday to avoid linking to
 * librt */
inline static void gettime(struct timespec *t)
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

int _gnutls_find_config_path(char *path, size_t max_size);
int _gnutls_ucs2_to_utf8(const void *data, size_t size,
			 gnutls_datum_t * output, unsigned bigendian);
int _gnutls_utf8_to_ucs2(const void *data, size_t size,
			 gnutls_datum_t * output);

int gnutls_system_global_init(void);
void gnutls_system_global_deinit(void);

#ifndef _WIN32
# if defined(HAVE_NETINET_IN_H)
#  include <netinet/in.h>
# endif
# include <arpa/inet.h>
#else
# undef inet_aton
# define inet_aton _gnutls_inet_aton
int inet_aton(const char *cp, struct in_addr *inp);
#endif

#ifndef HAVE_INET_PTON
# undef inet_pton
# define inet_pton _gnutls_inet_pton
int inet_pton(int af, const char *src, void *dst);
#endif

#ifndef HAVE_INET_NTOP
# undef inet_ntop
# define inet_ntop _gnutls_inet_ntop
const char *inet_ntop(int af, const void *src,
		      char *dst, unsigned size);
#endif

#endif				/* SYSTEM_H */
