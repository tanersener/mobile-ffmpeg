/*
 * Copyright (C) 2000-2012 Free Software Foundation, Inc.
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

/* This file contains the functions needed for 64 bit integer support in
 * TLS, and functions which ease the access to TLS vectors (data of given size).
 */

#include "gnutls_int.h"
#include <num.h>
#include "errors.h"

/* This function will add one to uint64 x.
 * Returns 0 on success, or -1 if the uint64 max limit
 * has been reached.
 */
int _gnutls_uint64pp(gnutls_uint64 * x)
{
	register int i, y = 0;

	for (i = 7; i >= 0; i--) {
		y = 0;
		if (x->i[i] == 0xff) {
			x->i[i] = 0;
			y = 1;
		} else
			x->i[i]++;

		if (y == 0)
			break;
	}
	if (y != 0)
		return -1;	/* over 64 bits! WOW */

	return 0;
}

/* This function will add one to uint48 x.
 * Returns 0 on success, or -1 if the uint48 max limit
 * has been reached.
 */
int _gnutls_uint48pp(gnutls_uint64 * x)
{
	register int i, y = 0;

	for (i = 7; i >= 3; i--) {
		y = 0;
		if (x->i[i] == 0xff) {
			x->i[i] = 0;
			y = 1;
		} else
			x->i[i]++;

		if (y == 0)
			break;
	}
	if (y != 0)
		return -1;	/* over 48 bits */

	return 0;
}
