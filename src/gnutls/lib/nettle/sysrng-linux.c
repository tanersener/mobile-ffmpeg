/*
 * Copyright (C) 2010-2016 Free Software Foundation, Inc.
 * Copyright (C) 2015-2016 Red Hat, Inc.
 *
 * Author: Nikos Mavrogiannopoulos
 *
 * This file is part of GNUTLS.
 *
 * The GNUTLS library is free software; you can redistribute it and/or
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

/* The Linux style system random generator: That is,
 * getrandom() -> /dev/urandom, where "->" indicates fallback.
 */

#ifndef RND_NO_INCLUDES
# include "gnutls_int.h"
# include "errors.h"
# include <num.h>
# include <errno.h>
# include <rnd-common.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

/* gnulib wants to claim strerror even if it cannot provide it. WTF */
#undef strerror

#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>

static int _gnutls_urandom_fd = -1;
static ino_t _gnutls_urandom_fd_ino = 0;
static dev_t _gnutls_urandom_fd_rdev = 0;

get_entropy_func _rnd_get_system_entropy = NULL;

#if defined(__linux__)
# ifdef HAVE_GETRANDOM
#  include <sys/random.h>
# else
#  include <sys/syscall.h>
#  undef getrandom
#  if defined(SYS_getrandom)
#   define getrandom(dst,s,flags) syscall(SYS_getrandom, (void*)dst, (size_t)s, (unsigned int)flags)
#  else
#   define getrandom(dst,s,flags) -1
#  endif
# endif


static unsigned have_getrandom(void)
{
	char c;
	int ret;
	ret = getrandom(&c, 1, 1/*GRND_NONBLOCK*/);
	if (ret == 1 || (ret == -1 && errno == EAGAIN))
		return 1;
	return 0;
}

/* returns exactly the amount of bytes requested */
static int force_getrandom(void *buf, size_t buflen, unsigned int flags)
{
	int left = buflen;
	int ret;
	uint8_t *p = buf;

	while (left > 0) {
		ret = getrandom(p, left, flags);
		if (ret == -1) {
			if (errno != EINTR)
				return ret;
		}

		if (ret > 0) {
			left -= ret;
			p += ret;
		}
	}

	return buflen;
}

static int _rnd_get_system_entropy_getrandom(void* _rnd, size_t size)
{
	int ret;
	ret = force_getrandom(_rnd, size, 0);
	if (ret == -1) {
		int e = errno;
		gnutls_assert();
		_gnutls_debug_log
			("Failed to use getrandom: %s\n",
					 strerror(e));
		return GNUTLS_E_RANDOM_DEVICE_ERROR;
	}

	return 0;
}
#else /* not linux */
# define have_getrandom() 0
#endif

static int _rnd_get_system_entropy_urandom(void* _rnd, size_t size)
{
	uint8_t* rnd = _rnd;
	uint32_t done;

	for (done = 0; done < size;) {
		int res;
		do {
			res = read(_gnutls_urandom_fd, rnd + done, size - done);
		} while (res < 0 && errno == EINTR);

		if (res <= 0) {
			int e = errno;
			if (res < 0) {
				_gnutls_debug_log
					("Failed to read /dev/urandom: %s\n",
					 strerror(e));
			} else {
				_gnutls_debug_log
					("Failed to read /dev/urandom: end of file\n");
			}

			return GNUTLS_E_RANDOM_DEVICE_ERROR;
		}

		done += res;
	}

	return 0;
}

/* This is called when gnutls_global_init() is called for second time.
 * It must check whether any resources are still available.
 * The particular problem it solves is to verify that the urandom fd is still
 * open (for applications that for some reason closed all fds */
int _rnd_system_entropy_check(void)
{
	int ret;
	struct stat st;

	if (_gnutls_urandom_fd == -1) /* not using urandom */
		return 0;

	ret = fstat(_gnutls_urandom_fd, &st);
	if (ret < 0 || st.st_ino != _gnutls_urandom_fd_ino || st.st_rdev != _gnutls_urandom_fd_rdev) {
		return _rnd_system_entropy_init();
	}
	return 0;
}

int _rnd_system_entropy_init(void)
{
	int old;
	struct stat st;

#if defined(__linux__)
	/* Enable getrandom() usage if available */
	if (have_getrandom()) {
		_rnd_get_system_entropy = _rnd_get_system_entropy_getrandom;
		_gnutls_debug_log("getrandom random generator was detected\n");
		return 0;
	}
#endif

	/* First fallback: /dev/unrandom */
	_gnutls_urandom_fd = open("/dev/urandom", O_RDONLY);
	if (_gnutls_urandom_fd < 0) {
		_gnutls_debug_log("Cannot open urandom!\n");
		return gnutls_assert_val(GNUTLS_E_RANDOM_DEVICE_ERROR);
	}

	old = fcntl(_gnutls_urandom_fd, F_GETFD);
	if (old != -1)
		fcntl(_gnutls_urandom_fd, F_SETFD, old | FD_CLOEXEC);

	if (fstat(_gnutls_urandom_fd, &st) >= 0) {
		_gnutls_urandom_fd_ino = st.st_ino;
		_gnutls_urandom_fd_rdev = st.st_rdev;
	}

	_rnd_get_system_entropy = _rnd_get_system_entropy_urandom;

	return 0;
}

void _rnd_system_entropy_deinit(void)
{
	if (_gnutls_urandom_fd >= 0) {
		close(_gnutls_urandom_fd);
		_gnutls_urandom_fd = -1;
	}
}

