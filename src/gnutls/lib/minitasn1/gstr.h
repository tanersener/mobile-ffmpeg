/*
 * Copyright (C) 2002-2014 Free Software Foundation, Inc.
 *
 * This file is part of LIBTASN1.
 *
 * The LIBTASN1 library is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA
 */

unsigned int _asn1_str_cpy (char *dest, size_t dest_tot_size,
			    const char *src);
void _asn1_str_cat (char *dest, size_t dest_tot_size, const char *src);

#define Estrcpy(x,y) _asn1_str_cpy(x,ASN1_MAX_ERROR_DESCRIPTION_SIZE,y)
#define Estrcat(x,y) _asn1_str_cat(x,ASN1_MAX_ERROR_DESCRIPTION_SIZE,y)

inline static
void safe_memset(void *data, int c, size_t size)
{
	volatile unsigned volatile_zero = 0;
	volatile char *vdata = (volatile char*)data;

	/* This is based on a nice trick for safe memset,
	 * sent by David Jacobson in the openssl-dev mailing list.
	 */

	if (size > 0) do {
		memset(data, c, size);
	} while(vdata[volatile_zero] != c);
}
