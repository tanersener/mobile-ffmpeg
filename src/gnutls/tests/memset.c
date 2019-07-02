/*
 * Copyright (C) 2017 Red Hat, Inc.
 *
 * Author: Nikos Mavrogiannopoulos
 *
 * This file is part of GnuTLS.
 *
 * GnuTLS is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * GnuTLS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <stdint.h>
#include <gnutls/gnutls.h>

#include "utils.h"

#define BUF_SIZE 128

void func1(void);
void func2(void);

static unsigned char *ptr;

/* Checks whether gnutls_memset() call is being optimized
 * out.
 */

void func1(void)
{
	unsigned char buf[BUF_SIZE];
	ptr = buf;

	gnutls_memset(buf, CHAR, sizeof(buf));
}

void func2(void)
{
	if (ptr[0] != CHAR || ptr[2] != CHAR || ptr[16] != CHAR)
		fail("previous memset failed!\n");
}

void doit(void)
{
#if defined(__has_feature)
#  if __has_feature(address_sanitizer)
	exit(77);
#  endif
#endif

#if __SANITIZE_ADDRESS__ == 1
	exit(77);
#endif

	func1();
	func2();
	success("memset test succeeded\n");
}
