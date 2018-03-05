/*
 * Copyright (C) 2013 Nikos Mavrogiannopoulos
 *
 * This file is part of GnuTLS test suite.
 *
 * ocserv is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * ocserv is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <stdio.h>
#include "utils.h"

#ifdef HAVE_LIBSECCOMP

#include <seccomp.h>
#include <errno.h>
#include <string.h>

int disable_system_calls(void)
{
	int ret;
	scmp_filter_ctx ctx;

	/*ctx = seccomp_init(SCMP_ACT_ERRNO(EPERM));*/
	ctx = seccomp_init(SCMP_ACT_TRAP);
	if (ctx == NULL) {
		fprintf(stderr, "could not initialize seccomp");
		return -1;
	}

#define ADD_SYSCALL(name, ...) \
	ret = seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(name), __VA_ARGS__); \
	/* libseccomp returns EDOM for pseudo-syscalls due to a bug */ \
	if (ret < 0 && ret != -EDOM) { \
		fprintf(stderr, "could not add " #name " to seccomp filter: %s", strerror(-ret)); \
		ret = -1; \
		goto fail; \
	}

	ADD_SYSCALL(nanosleep, 0);
	ADD_SYSCALL(time, 0);
	ADD_SYSCALL(getpid, 0);
	ADD_SYSCALL(gettimeofday, 0);
#if defined(HAVE_CLOCK_GETTIME)
	ADD_SYSCALL(clock_gettime, 0);
#endif

	ADD_SYSCALL(getrusage, 0);

	/* recv/send for the default pull/push functions. It is unknown
	 * which syscall is used by libc and varies from system to system
	 * so we enable all */
	ADD_SYSCALL(recvmsg, 0);
	ADD_SYSCALL(sendmsg, 0);
	ADD_SYSCALL(send, 0);
	ADD_SYSCALL(recv, 0);
	ADD_SYSCALL(sendto, 0);
	ADD_SYSCALL(recvfrom, 0);

	/* to read from /dev/urandom */
	ADD_SYSCALL(read, 0);
	ADD_SYSCALL(getrandom, 0);

	/* we use it in select */
	ADD_SYSCALL(sigprocmask, 0);
	ADD_SYSCALL(rt_sigprocmask, 0);

	/* used in to detect reading timeouts */
	ADD_SYSCALL(poll, 0);

	/* for memory allocation */
	ADD_SYSCALL(brk, 0);

	/* the following are for generic operations, not specific to
	 * gnutls. */
	ADD_SYSCALL(close, 0);
	ADD_SYSCALL(exit, 0);
	ADD_SYSCALL(exit_group, 0);

	/* allow returning from signal handlers */
	ADD_SYSCALL(sigreturn, 0);
	ADD_SYSCALL(rt_sigreturn, 0);

	ret = seccomp_load(ctx);
	if (ret < 0) {
		fprintf(stderr, "could not load seccomp filter");
		ret = -1;
		goto fail;
	}
	
	ret = 0;

fail:
	seccomp_release(ctx);
	return ret;
}
#else
int disable_system_calls(void)
{
	return 0;
}
#endif
