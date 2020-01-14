/*
 * Copyright (C) 2000-2012 Free Software Foundation, Inc.
 * Copyright (C) 2017 Red Hat, Inc.
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

/* Functions that relate to base64 encoding and decoding.
 */

#include "gnutls_int.h"
#include "errors.h"
#include <datum.h>
#include <x509_b64.h>
#include <nettle/base64.h>

#define INCR(what, size, max_len) \
	do { \
	what+=size; \
	if (what > max_len) { \
		gnutls_assert(); \
		gnutls_free( result->data); result->data = NULL; \
		return GNUTLS_E_INTERNAL_ERROR; \
	} \
	} while(0)

/* encodes data and puts the result into result (locally allocated)
 * The result_size (including the null terminator) is the return value.
 */
int
_gnutls_fbase64_encode(const char *msg, const uint8_t * data,
		       size_t data_size, gnutls_datum_t * result)
{
	int tmp;
	unsigned int i;
	uint8_t tmpres[66];
	uint8_t *ptr;
	char top[80];
	char bottom[80];
	size_t size, max, bytes;
	int pos, top_len = 0, bottom_len = 0;
	unsigned raw_encoding = 0;

	if (msg == NULL || msg[0] == 0)
		raw_encoding = 1;

	if (!raw_encoding) {
		if (strlen(msg) > 50) {
			gnutls_assert();
			return GNUTLS_E_BASE64_ENCODING_ERROR;
		}

		_gnutls_str_cpy(top, sizeof(top), "-----BEGIN ");
		_gnutls_str_cat(top, sizeof(top), msg);
		_gnutls_str_cat(top, sizeof(top), "-----\n");

		_gnutls_str_cpy(bottom, sizeof(bottom), "-----END ");
		_gnutls_str_cat(bottom, sizeof(bottom), msg);
		_gnutls_str_cat(bottom, sizeof(bottom), "-----\n");

		top_len = strlen(top);
		bottom_len = strlen(bottom);
	}

	max = B64FSIZE(top_len + bottom_len, data_size);

	result->data = gnutls_malloc(max + 1);
	if (result->data == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	bytes = pos = 0;
	INCR(bytes, top_len, max);
	pos = top_len;

	memcpy(result->data, top, top_len);

	for (i = 0; i < data_size; i += 48) {
		if (data_size - i < 48)
			tmp = data_size - i;
		else
			tmp = 48;

		size = BASE64_ENCODE_RAW_LENGTH(tmp);
		if (sizeof(tmpres) < size)
			return gnutls_assert_val(GNUTLS_E_BASE64_ENCODING_ERROR);

		base64_encode_raw((void*)tmpres, tmp, &data[i]);

		INCR(bytes, size + 1, max);
		ptr = &result->data[pos];

		memcpy(ptr, tmpres, size);
		ptr += size;
		pos += size;
		if (!raw_encoding) {
			*ptr++ = '\n';
			pos++;
		} else {
			bytes--;
		}
	}

	INCR(bytes, bottom_len, max);

	memcpy(&result->data[bytes - bottom_len], bottom, bottom_len);
	result->data[bytes] = 0;
	result->size = bytes;

	return max + 1;
}

/**
 * gnutls_pem_base64_encode:
 * @msg: is a message to be put in the header (may be %NULL)
 * @data: contain the raw data
 * @result: the place where base64 data will be copied
 * @result_size: holds the size of the result
 *
 * This function will convert the given data to printable data, using
 * the base64 encoding. This is the encoding used in PEM messages.
 *
 * The output string will be null terminated, although the output size will
 * not include the terminating null.
 *
 * Returns: On success %GNUTLS_E_SUCCESS (0) is returned,
 *   %GNUTLS_E_SHORT_MEMORY_BUFFER is returned if the buffer given is
 *   not long enough, or 0 on success.
 **/
int
gnutls_pem_base64_encode(const char *msg, const gnutls_datum_t * data,
			 char *result, size_t * result_size)
{
	gnutls_datum_t res;
	int ret;

	ret = _gnutls_fbase64_encode(msg, data->data, data->size, &res);
	if (ret < 0)
		return ret;

	if (result == NULL || *result_size < (unsigned) res.size) {
		gnutls_free(res.data);
		*result_size = res.size + 1;
		return GNUTLS_E_SHORT_MEMORY_BUFFER;
	} else {
		memcpy(result, res.data, res.size);
		gnutls_free(res.data);
		*result_size = res.size;
	}

	return 0;
}

/**
 * gnutls_pem_base64_encode2:
 * @header: is a message to be put in the encoded header (may be %NULL)
 * @data: contains the raw data
 * @result: will hold the newly allocated encoded data
 *
 * This function will convert the given data to printable data, using
 * the base64 encoding.  This is the encoding used in PEM messages.
 * This function will allocate the required memory to hold the encoded
 * data.
 *
 * You should use gnutls_free() to free the returned data.
 *
 * Note, that prior to GnuTLS 3.4.0 this function was available
 * under the name gnutls_pem_base64_encode_alloc(). There is
 * compatibility macro pointing to this function.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise
 *   an error code is returned.
 *
 * Since: 3.4.0
 **/
int
gnutls_pem_base64_encode2(const char *header,
			       const gnutls_datum_t * data,
			       gnutls_datum_t * result)
{
	int ret;

	if (result == NULL)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	ret = _gnutls_fbase64_encode(header, data->data, data->size, result);
	if (ret < 0)
		return gnutls_assert_val(ret);

	return 0;
}

/* copies data to result but removes newlines and <CR>
 * returns the size of the data copied.
 *
 * It will fail with GNUTLS_E_BASE64_DECODING_ERROR if the
 * end-result is the empty string.
 */
inline static int
cpydata(const uint8_t * data, int data_size, gnutls_datum_t * result)
{
	int i, j;

	result->data = gnutls_malloc(data_size + 1);
	if (result->data == NULL)
		return GNUTLS_E_MEMORY_ERROR;

	for (j = i = 0; i < data_size; i++) {
		if (data[i] == '\n' || data[i] == '\r' || data[i] == ' '
		    || data[i] == '\t')
			continue;
		else if (data[i] == '-')
			break;
		result->data[j] = data[i];
		j++;
	}

	result->size = j;
	result->data[j] = 0;

	if (j==0) {
		gnutls_free(result->data);
		return gnutls_assert_val(GNUTLS_E_BASE64_DECODING_ERROR);
	}

	return j;
}

/* decodes data and puts the result into result (locally allocated).
 * Note that encodings of zero-length strings are being rejected
 * with GNUTLS_E_BASE64_DECODING_ERROR.
 *
 * The result_size is the return value.
 */
int
_gnutls_base64_decode(const uint8_t * data, size_t data_size,
		      gnutls_datum_t * result)
{
	int ret;
	size_t size;
	gnutls_datum_t pdata;
	struct base64_decode_ctx ctx;

	if (data_size == 0) {
		result->data = (unsigned char*)gnutls_strdup("");
		if (result->data == NULL)
			return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);
		result->size = 0;
		return 0;
	}

	ret = cpydata(data, data_size, &pdata);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	base64_decode_init(&ctx);

	size = BASE64_DECODE_LENGTH(pdata.size);
	if (size == 0) {
		ret = gnutls_assert_val(GNUTLS_E_BASE64_DECODING_ERROR);
		goto cleanup;
	}

	result->data = gnutls_malloc(size);
	if (result->data == NULL) {
		ret = gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);
		goto cleanup;
	}

	ret = base64_decode_update(&ctx, &size, result->data,
				   pdata.size, (void*)pdata.data);
	if (ret == 0 || size == 0) {
		gnutls_assert();
		ret = GNUTLS_E_BASE64_DECODING_ERROR;
		goto fail;
	}

	ret = base64_decode_final(&ctx);
	if (ret != 1) {
		ret = gnutls_assert_val(GNUTLS_E_BASE64_DECODING_ERROR);
		goto fail;
	}

	result->size = size;

	ret = size;
	goto cleanup;

 fail:
	gnutls_free(result->data);

 cleanup:
	gnutls_free(pdata.data);
	return ret;
}


/* Searches the given string for ONE PEM encoded certificate, and
 * stores it in the result.
 *
 * The result_size (always non-zero) is the return value,
 * or a negative error code.
 */
#define ENDSTR "-----"
int
_gnutls_fbase64_decode(const char *header, const uint8_t * data,
		       size_t data_size, gnutls_datum_t * result)
{
	int ret;
	static const char top[] = "-----BEGIN ";
	static const char bottom[] = "-----END ";
	uint8_t *rdata, *kdata;
	int rdata_size;
	char pem_header[128];

	_gnutls_str_cpy(pem_header, sizeof(pem_header), top);
	if (header != NULL)
		_gnutls_str_cat(pem_header, sizeof(pem_header), header);

	rdata = memmem(data, data_size, pem_header, strlen(pem_header));
	if (rdata == NULL) {
		gnutls_assert();
		_gnutls_hard_log("Could not find '%s'\n", pem_header);
		return GNUTLS_E_BASE64_UNEXPECTED_HEADER_ERROR;
	}

	data_size -= MEMSUB(rdata, data);

	if (data_size < 4 + strlen(bottom)) {
		gnutls_assert();
		return GNUTLS_E_BASE64_DECODING_ERROR;
	}

	kdata =
	    memmem(rdata + 1, data_size - 1, ENDSTR, sizeof(ENDSTR) - 1);
	/* allow CR as well.
	 */
	if (kdata == NULL) {
		gnutls_assert();
		_gnutls_hard_log("Could not find '%s'\n", ENDSTR);
		return GNUTLS_E_BASE64_DECODING_ERROR;
	}
	data_size -= strlen(ENDSTR);
	data_size -= MEMSUB(kdata, rdata);

	rdata = kdata + strlen(ENDSTR);

	/* position is now after the ---BEGIN--- headers */

	kdata = memmem(rdata, data_size, bottom, strlen(bottom));
	if (kdata == NULL) {
		gnutls_assert();
		return GNUTLS_E_BASE64_DECODING_ERROR;
	}

	/* position of kdata is before the ----END--- footer 
	 */
	rdata_size = MEMSUB(kdata, rdata);

	if (rdata_size < 4) {
		gnutls_assert();
		return GNUTLS_E_BASE64_DECODING_ERROR;
	}

	if ((ret = _gnutls_base64_decode(rdata, rdata_size, result)) < 0) {
		gnutls_assert();
		return GNUTLS_E_BASE64_DECODING_ERROR;
	}

	return ret;
}

/**
 * gnutls_pem_base64_decode:
 * @header: A null terminated string with the PEM header (eg. CERTIFICATE)
 * @b64_data: contain the encoded data
 * @result: the place where decoded data will be copied
 * @result_size: holds the size of the result
 *
 * This function will decode the given encoded data.  If the header
 * given is non %NULL this function will search for "-----BEGIN header"
 * and decode only this part.  Otherwise it will decode the first PEM
 * packet found.
 *
 * Returns: On success %GNUTLS_E_SUCCESS (0) is returned,
 *   %GNUTLS_E_SHORT_MEMORY_BUFFER is returned if the buffer given is
 *   not long enough, or 0 on success.
 **/
int
gnutls_pem_base64_decode(const char *header,
			 const gnutls_datum_t * b64_data,
			 unsigned char *result, size_t * result_size)
{
	gnutls_datum_t res;
	int ret;

	ret =
	    _gnutls_fbase64_decode(header, b64_data->data, b64_data->size,
				   &res);
	if (ret < 0)
		return gnutls_assert_val(ret);

	if (result == NULL || *result_size < (unsigned) res.size) {
		gnutls_free(res.data);
		*result_size = res.size;
		return GNUTLS_E_SHORT_MEMORY_BUFFER;
	} else {
		memcpy(result, res.data, res.size);
		gnutls_free(res.data);
		*result_size = res.size;
	}

	return 0;
}

/**
 * gnutls_pem_base64_decode2:
 * @header: The PEM header (eg. CERTIFICATE)
 * @b64_data: contains the encoded data
 * @result: the location of decoded data
 *
 * This function will decode the given encoded data. The decoded data
 * will be allocated, and stored into result.  If the header given is
 * non null this function will search for "-----BEGIN header" and
 * decode only this part. Otherwise it will decode the first PEM
 * packet found.
 *
 * You should use gnutls_free() to free the returned data.
 *
 * Note, that prior to GnuTLS 3.4.0 this function was available
 * under the name gnutls_pem_base64_decode_alloc(). There is
 * compatibility macro pointing to this function.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise
 *   an error code is returned.
 *
 * Since: 3.4.0
 **/
int
gnutls_pem_base64_decode2(const char *header,
			       const gnutls_datum_t * b64_data,
			       gnutls_datum_t * result)
{
	int ret;

	if (result == NULL)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	ret =
	    _gnutls_fbase64_decode(header, b64_data->data, b64_data->size,
				   result);
	if (ret < 0)
		return gnutls_assert_val(ret);

	return 0;
}

/**
 * gnutls_base64_decode2:
 * @base64: contains the encoded data
 * @result: the location of decoded data
 *
 * This function will decode the given base64 encoded data. The decoded data
 * will be allocated, and stored into result.
 *
 * You should use gnutls_free() to free the returned data.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise
 *   an error code is returned.
 *
 * Since: 3.6.0
 **/
int
gnutls_base64_decode2(const gnutls_datum_t *base64,
		      gnutls_datum_t *result)
{
	int ret;

	ret = _gnutls_base64_decode(base64->data, base64->size, result);
	if (ret < 0) {
		return gnutls_assert_val(ret);
	}

	return 0;
}

/**
 * gnutls_base64_encode2:
 * @data: contains the raw data
 * @result: will hold the newly allocated encoded data
 *
 * This function will convert the given data to printable data, using
 * the base64 encoding. This function will allocate the required
 * memory to hold the encoded data.
 *
 * You should use gnutls_free() to free the returned data.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise
 *   an error code is returned.
 *
 * Since: 3.6.0
 **/
int
gnutls_base64_encode2(const gnutls_datum_t *data,
		      gnutls_datum_t *result)
{
	int ret;

	if (result == NULL)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	ret = _gnutls_fbase64_encode(NULL, data->data, data->size, result);
	if (ret < 0)
		return gnutls_assert_val(ret);

	return 0;
}
