/* seskey.c - Session key routines
 * Copyright (C) 1998-2012 Free Software Foundation, Inc.
 *
 * Author: Timo Schulz
 *
 * This file is part of OpenCDK.
 *
 * The OpenCDK library is free software; you can redistribute it and/or
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>

#include "opencdk.h"
#include "main.h"
#include "packet.h"
#include <algorithms.h>

/**
 * cdk_s2k_new:
 * @ret_s2k: output for the new S2K object
 * @mode: the S2K mode (simple, salted, iter+salted)
 * @digest_algo: the hash algorithm
 * @salt: random salt
 * 
 * Create a new S2K object with the given parameter.
 * The @salt parameter must be always 8 octets.
 **/
cdk_error_t
cdk_s2k_new(cdk_s2k_t * ret_s2k, int mode, int digest_algo,
	    const byte * salt)
{
	cdk_s2k_t s2k;

	if (!ret_s2k)
		return CDK_Inv_Value;

	if (mode != 0x00 && mode != 0x01 && mode != 0x03)
		return CDK_Inv_Mode;

	if (_gnutls_hash_get_algo_len(mac_to_entry(digest_algo)) <= 0)
		return CDK_Inv_Algo;

	s2k = cdk_calloc(1, sizeof *s2k);
	if (!s2k)
		return CDK_Out_Of_Core;
	s2k->mode = mode;
	s2k->hash_algo = digest_algo;
	if (salt)
		memcpy(s2k->salt, salt, 8);
	*ret_s2k = s2k;
	return 0;
}


/**
 * cdk_s2k_free:
 * @s2k: the S2K object
 * 
 * Release the given S2K object.
 **/
void cdk_s2k_free(cdk_s2k_t s2k)
{
	cdk_free(s2k);
}


/* Make a copy of the source s2k into R_DST. */
cdk_error_t _cdk_s2k_copy(cdk_s2k_t * r_dst, cdk_s2k_t src)
{
	cdk_s2k_t dst;
	cdk_error_t err;

	err = cdk_s2k_new(&dst, src->mode, src->hash_algo, src->salt);
	if (err)
		return err;
	dst->count = src->count;
	*r_dst = dst;

	return 0;
}
