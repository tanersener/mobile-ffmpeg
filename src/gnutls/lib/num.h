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

#ifndef GNUTLS_NUM_H
#define GNUTLS_NUM_H

#include "gnutls_int.h"

#include <minmax.h>
#include <byteswap.h>

int _gnutls_uint64pp(gnutls_uint64 *);
int _gnutls_uint48pp(gnutls_uint64 *);

#define UINT64DATA(x) ((x).i)

inline static uint32_t _gnutls_uint24touint32(uint24 num)
{
	uint32_t ret = 0;

	((uint8_t *) & ret)[1] = num.pint[0];
	((uint8_t *) & ret)[2] = num.pint[1];
	((uint8_t *) & ret)[3] = num.pint[2];
	return ret;
}

inline static uint24 _gnutls_uint32touint24(uint32_t num)
{
	uint24 ret;

	ret.pint[0] = ((uint8_t *) & num)[1];
	ret.pint[1] = ((uint8_t *) & num)[2];
	ret.pint[2] = ((uint8_t *) & num)[3];
	return ret;

}

/* data should be at least 3 bytes */
inline static uint32_t _gnutls_read_uint24(const uint8_t * data)
{
	uint32_t res;
	uint24 num;

	num.pint[0] = data[0];
	num.pint[1] = data[1];
	num.pint[2] = data[2];

	res = _gnutls_uint24touint32(num);
#ifndef WORDS_BIGENDIAN
	res = bswap_32(res);
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
	uint24 tmp;

#ifndef WORDS_BIGENDIAN
	num = bswap_32(num);
#endif
	tmp = _gnutls_uint32touint24(num);

	data[0] = tmp.pint[0];
	data[1] = tmp.pint[1];
	data[2] = tmp.pint[2];
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

inline static uint32_t _gnutls_uint64touint32(const gnutls_uint64 * num)
{
	uint32_t ret;

	memcpy(&ret, &num->i[4], 4);
#ifndef WORDS_BIGENDIAN
	ret = bswap_32(ret);
#endif

	return ret;
}

#endif				/* GNUTLS_NUM_H */
