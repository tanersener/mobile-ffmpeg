/*
 * Copyright (C) 2008-2012 Free Software Foundation, Inc.
 *
 * Author: Nikos Mavrogiannopoulos
 *
 * This file is part of GnuTLS.
 *
 * GnuTLS is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GnuTLS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GnuTLS.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#if !defined(_WIN32)
#include <sys/wait.h>
#endif

#include "utils.h"
#include <gnutls/gnutls.h>
#include <gnutls/crypto.h>

#if defined(_WIN32)
void doit(void)
{
	exit(77);
}
#else

#include "../lib/atfork.h"
#include "../lib/atfork.c"

void doit(void)
{
	pid_t pid;
	int status;
	unsigned forkid;

	_gnutls_register_fork_handler();

	forkid = _gnutls_get_forkid();
	if (_gnutls_detect_fork(forkid) != 0) {
		fail("Detected fork on parent!\n");
		exit(1);
	}

	pid = fork();
	if (pid == -1) {
		fail("error in fork\n");
		exit(1);
	}

	if (pid == 0) {
		pid = fork();
		if (pid == -1) {
			fail("error in fork\n");
			exit(1);
		}

		if (pid == 0) {
			if (_gnutls_detect_fork(forkid) == 0) {
				fail("child: didn't detect fork on grandchild!\n");
				exit(1);
			}
			exit(0);
		}

		if (waitpid(pid, &status, 0) < 0) {
			fail("error in waitpid\n");
			exit(2);
		}

		if (!WIFEXITED(status) || (WEXITSTATUS(status) != 0)) {
			fail("Didn't detect fork on grandchild\n");
			exit(2);
		}

		if (_gnutls_detect_fork(forkid) == 0) {
			fail("child: didn't detect fork on child!\n");
			exit(1);
		}

		exit(0);
	}

	if (waitpid(pid, &status, 0) < 0) {
		fail("error in waitpid\n");
		exit(1);
	}

	if (!WIFEXITED(status) || (WEXITSTATUS(status) != 0)) {
		fail("Didn't detect fork on child\n");
		exit(1);
	}

	if (_gnutls_detect_fork(forkid) != 0) {
		fail("Detected fork on parent after fork!\n");
		exit(1);
	}

	success("all tests ok\n");
	return;
}

#endif
