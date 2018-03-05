/*
 * Copyright (C) 2001-2012 Free Software Foundation, Inc.
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

#include "gnutls_int.h"
#include "errors.h"
#include <datum.h>
#include <auth/srp_passwd.h>

#ifdef ENABLE_SRP

/* this is a modified base64 for srp !!!
 * It seems that everybody makes their own base64 conversion.
 */
static const uint8_t b64table[] =
    "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz./";

static const uint8_t asciitable[128] = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0x3e, 0x3f,
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
	0x06, 0x07, 0x08, 0x09, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0x0a,
	0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
	0x11, 0x12, 0x13, 0x14, 0x15, 0x16,
	0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c,
	0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22,
	0x23, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0x24, 0x25, 0x26, 0x27, 0x28,
	0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e,
	0x2f, 0x30, 0x31, 0x32, 0x33, 0x34,
	0x35, 0x36, 0x37, 0x38, 0x39, 0x3a,
	0x3b, 0x3c, 0x3d, 0xff, 0xff, 0xff,
	0xff, 0xff
};

inline static int encode(uint8_t * result, const uint8_t * rdata, unsigned left)
{

	int data_len;
	int c, ret = 4;
	uint8_t data[3];

	if (left > 3)
		data_len = 3;
	else
		data_len = left;

	data[0] = data[1] = data[2] = 0;
	memcpy(data, rdata, data_len);

	switch (data_len) {
	case 3:
		result[0] = b64table[((data[0] & 0xfc) >> 2)];
		result[1] =
		    b64table[(((((data[0] & 0x03) & 0xff) << 4) & 0xff) |
			      ((data[1] & 0xf0) >> 4))];
		result[2] =
		    b64table[((((data[1] & 0x0f) << 2) & 0xff) |
			      ((data[2] & 0xc0) >> 6))];
		result[3] = b64table[(data[2] & 0x3f) & 0xff];
		break;
	case 2:
		if ((c = ((data[0] & 0xf0) >> 4)) != 0) {
			result[0] = b64table[c];
			result[1] =
			    b64table[((((data[0] & 0x0f) << 2) & 0xff) |
				      ((data[1] & 0xc0) >> 6))];
			result[2] = b64table[(data[1] & 0x3f) & 0xff];
			result[3] = '\0';
			ret -= 1;
		} else {
			if ((c =
			     ((data[0] & 0x0f) << 2) | ((data[1] & 0xc0) >>
							6)) != 0) {
				result[0] = b64table[c];
				result[1] = b64table[data[1] & 0x3f];
				result[2] = '\0';
				result[3] = '\0';
				ret -= 2;
			} else {
				result[0] = b64table[data[0] & 0x3f];
				result[1] = '\0';
				result[2] = '\0';
				result[3] = '\0';
				ret -= 3;
			}
		}
		break;
	case 1:
		if ((c = ((data[0] & 0xc0) >> 6)) != 0) {
			result[0] = b64table[c];
			result[1] = b64table[(data[0] & 0x3f) & 0xff];
			result[2] = '\0';
			result[3] = '\0';
			ret -= 2;
		} else {
			result[0] = b64table[(data[0] & 0x3f) & 0xff];
			result[1] = '\0';
			result[2] = '\0';
			result[3] = '\0';
			ret -= 3;
		}
		break;
	default:
		return GNUTLS_E_BASE64_ENCODING_ERROR;
	}

	return ret;

}

/* encodes data and puts the result into result (locally allocated)
 * The result_size is the return value
 */
static int
_gnutls_sbase64_encode(uint8_t * data, size_t data_size, char **result)
{
	unsigned i, j;
	int ret, tmp;
	uint8_t tmpres[4];
	unsigned mod = data_size % 3;

	ret = mod;
	if (ret != 0)
		ret = 4;
	else
		ret = 0;

	ret += (data_size * 4) / 3;

	(*result) = gnutls_calloc(1, ret + 1);
	if ((*result) == NULL)
		return GNUTLS_E_MEMORY_ERROR;

	i = j = 0;
/* encode the bytes that are not a multiple of 3 
 */
	if (mod > 0) {
		tmp = encode(tmpres, &data[0], mod);
		if (tmp < 0) {
			gnutls_free((*result));
			return tmp;
		}

		memcpy(&(*result)[0], tmpres, tmp);
		i = mod;
		j = tmp;

	}
/* encode the rest
 */
	for (; i < data_size; i += 3, j += 4) {
		tmp = encode(tmpres, &data[i], data_size - i);
		if (tmp < 0) {
			gnutls_free((*result));
			return tmp;
		}
		memcpy(&(*result)[j], tmpres, tmp);
	}

	return strlen(*result);
}


/* data must be 4 bytes
 * result should be 3 bytes
 */
#define TOASCII(c) (c < 127 ? asciitable[c] : 0xff)
inline static int decode(uint8_t * result, const uint8_t * data)
{
	uint8_t a1, a2;
	int ret = 3;

	memset(result, 0, 3);

	a1 = TOASCII(data[3]);
	a2 = TOASCII(data[2]);
	if (a1 != 0xff)
		result[2] = a1 & 0xff;
	else
		return GNUTLS_E_BASE64_DECODING_ERROR;
	if (a2 != 0xff)
		result[2] |= ((a2 & 0x03) << 6) & 0xff;

	a1 = a2;
	a2 = TOASCII(data[1]);
	if (a1 != 0xff)
		result[1] = ((a1 & 0x3c) >> 2);
	if (a2 != 0xff)
		result[1] |= ((a2 & 0x0f) << 4);
	else if (a1 == 0xff || result[1] == 0)
		ret--;

	a1 = a2;
	a2 = TOASCII(data[0]);
	if (a1 != 0xff)
		result[0] = (((a1 & 0x30) >> 4) & 0xff);
	if (a2 != 0xff)
		result[0] |= ((a2 << 2) & 0xff);
	else if (a1 == 0xff || result[0] == 0)
		ret--;

	return ret;
}

/* decodes data and puts the result into result (locally allocated)
 * The result_size is the return value.
 * That function does not ignore newlines tabs etc. You should remove them
 * before calling it.
 */
int
_gnutls_sbase64_decode(char *data, size_t idata_size, uint8_t ** result)
{
	unsigned i, j;
	int ret, left;
	int data_size, tmp;
	uint8_t datrev[4];
	uint8_t tmpres[3];

	data_size = (idata_size / 4) * 4;
	left = idata_size % 4;

	ret = (data_size / 4) * 3;

	if (left > 0)
		ret += 3;

	(*result) = gnutls_malloc(ret + 1);
	if ((*result) == NULL)
		return GNUTLS_E_MEMORY_ERROR;

	/* the first "block" is treated with special care */
	tmp = 0;
	if (left > 0) {
		memset(datrev, 0, 4);
		memcpy(&datrev[4 - left], data, left);

		tmp = decode(tmpres, datrev);
		if (tmp < 0) {
			gnutls_free((*result));
			*result = NULL;
			return tmp;
		}

		memcpy(*result, &tmpres[3 - tmp], tmp);
		if (tmp < 3)
			ret -= (3 - tmp);
	}

	/* rest data */
	for (i = left, j = tmp; i < idata_size; i += 4) {
		tmp = decode(tmpres, (uint8_t *) & data[i]);
		if (tmp < 0) {
			gnutls_free((*result));
			*result = NULL;
			return tmp;
		}
		memcpy(&(*result)[j], tmpres, tmp);
		if (tmp < 3)
			ret -= (3 - tmp);
		j += 3;
	}

	return ret;
}

/**
 * gnutls_srp_base64_encode:
 * @data: contain the raw data
 * @result: the place where base64 data will be copied
 * @result_size: holds the size of the result
 *
 * This function will convert the given data to printable data, using
 * the base64 encoding, as used in the libsrp.  This is the encoding
 * used in SRP password files.  If the provided buffer is not long
 * enough GNUTLS_E_SHORT_MEMORY_BUFFER is returned.
 *
 * Warning!  This base64 encoding is not the "standard" encoding, so
 * do not use it for non-SRP purposes.
 *
 * Returns: %GNUTLS_E_SHORT_MEMORY_BUFFER if the buffer given is not
 * long enough, or 0 on success.
 **/
int
gnutls_srp_base64_encode(const gnutls_datum_t * data, char *result,
			 size_t * result_size)
{
	char *res;
	int size;

	size = _gnutls_sbase64_encode(data->data, data->size, &res);
	if (size < 0)
		return size;

	if (result == NULL || *result_size < (size_t) size) {
		gnutls_free(res);
		*result_size = size;
		return GNUTLS_E_SHORT_MEMORY_BUFFER;
	} else {
		memcpy(result, res, size);
		gnutls_free(res);
		*result_size = size;
	}

	return 0;
}

/**
 * gnutls_srp_base64_encode2:
 * @data: contains the raw data
 * @result: will hold the newly allocated encoded data
 *
 * This function will convert the given data to printable data, using
 * the base64 encoding.  This is the encoding used in SRP password
 * files.  This function will allocate the required memory to hold
 * the encoded data.
 *
 * You should use gnutls_free() to free the returned data.
 *
 * Warning!  This base64 encoding is not the "standard" encoding, so
 * do not use it for non-SRP purposes.
 *
 * Returns: 0 on success, or an error code.
 **/
int
gnutls_srp_base64_encode2(const gnutls_datum_t * data,
			       gnutls_datum_t * result)
{
	char *res;
	int size;

	size = _gnutls_sbase64_encode(data->data, data->size, &res);
	if (size < 0)
		return size;

	if (result == NULL) {
		gnutls_free(res);
		return GNUTLS_E_INVALID_REQUEST;
	} else {
		result->data = (uint8_t *) res;
		result->size = size;
	}

	return 0;
}

/**
 * gnutls_srp_base64_decode:
 * @b64_data: contain the encoded data
 * @result: the place where decoded data will be copied
 * @result_size: holds the size of the result
 *
 * This function will decode the given encoded data, using the base64
 * encoding found in libsrp.
 *
 * Note that @b64_data should be null terminated.
 *
 * Warning!  This base64 encoding is not the "standard" encoding, so
 * do not use it for non-SRP purposes.
 *
 * Returns: %GNUTLS_E_SHORT_MEMORY_BUFFER if the buffer given is not
 * long enough, or 0 on success.
 **/
int
gnutls_srp_base64_decode(const gnutls_datum_t * b64_data, char *result,
			 size_t * result_size)
{
	uint8_t *res;
	int size;

	size =
	    _gnutls_sbase64_decode((char *) b64_data->data, b64_data->size,
				   &res);
	if (size < 0)
		return size;

	if (result == NULL || *result_size < (size_t) size) {
		gnutls_free(res);
		*result_size = size;
		return GNUTLS_E_SHORT_MEMORY_BUFFER;
	} else {
		memcpy(result, res, size);
		gnutls_free(res);
		*result_size = size;
	}

	return 0;
}

/**
 * gnutls_srp_base64_decode2:
 * @b64_data: contains the encoded data
 * @result: the place where decoded data lie
 *
 * This function will decode the given encoded data. The decoded data
 * will be allocated, and stored into result.  It will decode using
 * the base64 algorithm as used in libsrp.
 *
 * You should use gnutls_free() to free the returned data.
 *
 * Warning!  This base64 encoding is not the "standard" encoding, so
 * do not use it for non-SRP purposes.
 *
 * Returns: 0 on success, or an error code.
 **/
int
gnutls_srp_base64_decode2(const gnutls_datum_t * b64_data,
			       gnutls_datum_t * result)
{
	uint8_t *ret;
	int size;

	size =
	    _gnutls_sbase64_decode((char *) b64_data->data, b64_data->size,
				   &ret);
	if (size < 0)
		return size;

	if (result == NULL) {
		gnutls_free(ret);
		return GNUTLS_E_INVALID_REQUEST;
	} else {
		result->data = ret;
		result->size = size;
	}

	return 0;
}

#endif				/* ENABLE_SRP */
