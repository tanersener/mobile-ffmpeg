/*
 * Copyright (C) 2010-2016 Free Software Foundation, Inc.
 * Copyright (C) 2015-2016 Red Hat, Inc.
 *
 * Author: Nia Alarie
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

/*
 * The *BSD sysctl-based system random generator.
 * Used on NetBSD.
 */

#include "gnutls_int.h"
#include "errors.h"
#include <locks.h>
#include <num.h>
#include <errno.h>
#include <rnd-common.h>

#include <sys/sysctl.h>

#define	ARRAY_SIZE(A)	(sizeof(A)/sizeof((A)[0]))

get_entropy_func _rnd_get_system_entropy = NULL;

static int _rnd_get_system_entropy_sysctl(void* _rnd, size_t size)
{
	static int name[] = {CTL_KERN, KERN_ARND};
	size_t count, req;
	unsigned char* p;

	p = _rnd;
	while (size) {
		req = size < 32 ? size : 32;
		count = req;

		if (sysctl(name, ARRAY_SIZE(name), p, &count, NULL, 0) == -1) {
			return GNUTLS_E_RANDOM_DEVICE_ERROR;
		}

		if (count != req) {
			return GNUTLS_E_RANDOM_DEVICE_ERROR; /* Can't happen. */
		}

		p += count;
		size -= count;
	}

	return 0;
}

int _rnd_system_entropy_init(void)
{
	_rnd_get_system_entropy = _rnd_get_system_entropy_sysctl;
	return 0;
}

int _rnd_system_entropy_check(void)
{
	return 0;
}

void _rnd_system_entropy_deinit(void)
{
	return;
}

