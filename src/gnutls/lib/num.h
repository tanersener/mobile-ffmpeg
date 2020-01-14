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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>
 *
 */

#ifndef GNUTLS_LIB_NUM_H
#define GNUTLS_LIB_NUM_H

#include "gnutls_int.h"

#include <minmax.h>
#include <byteswap.h>

/* data should be at least 3 bytes */
inline static uint32_t _gnutls_read_uint24(const uint8_t * data)
{
	return (data[0] << 16) | (data[1] << 8) | (data[2]);
}

inline static uint64_t _gnutls_read_uint64(const uint8_t * data)
{
	uint64_t res;

	memcpy(&res, data, sizeof(uint64_t));
#ifndef WORDS_BIGENDIAN
	res = bswap_64(res);
#endif
	return res;
}

inline static void _gnutls_write_uint64(uint64_t num, uint8_t * data)
{
#ifndef WORDS_BIGENDIAN
	num = bswap_64(num);
#endif
	memcpy(data, &num, 8);
}

inline static void _gnutls_write_uint24(uint32_t num, uint8_t * data)
{
	data[0] = num >> 16;
	data[1] = num >> 8;
	data[2] = num;
}

inline static uint32_t _gnutls_read_uint32(const uint8_t * data)
{
	uint32_t res;

	memcpy(&res, data, sizeof(uint32_t));
#ifndef WORDS_BIGENDIAN
	res = bswap_32(res);
#endif
	return res;
}

inline static void _gnutls_write_uint32(uint32_t num, uint8_t * data)
{

#ifndef WORDS_BIGENDIAN
	num = bswap_32(num);
#endif
	memcpy(data, &num, sizeof(uint32_t));
}

inline static uint16_t _gnutls_read_uint16(const uint8_t * data)
{
	uint16_t res;
	memcpy(&res, data, sizeof(uint16_t));
#ifndef WORDS_BIGENDIAN
	res = bswap_16(res);
#endif
	return res;
}

inline static void _gnutls_write_uint16(uint16_t num, uint8_t * data)
{

#ifndef WORDS_BIGENDIAN
	num = bswap_16(num);
#endif
	memcpy(data, &num, sizeof(uint16_t));
}

inline static uint32_t _gnutls_conv_uint32(uint32_t data)
{
#ifndef WORDS_BIGENDIAN
	return bswap_32(data);
#else
	return data;
#endif
}

inline static uint16_t _gnutls_conv_uint16(uint16_t data)
{
#ifndef WORDS_BIGENDIAN
	return bswap_16(data);
#else
	return data;
#endif
}

#endif /* GNUTLS_LIB_NUM_H */
