/*
 * Copyright (C) 2002-2012 Free Software Foundation, Inc.
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
#include <num.h>
#include "str.h"
#include <stdarg.h>
#include <c-ctype.h>
#include <intprops.h>
#include <nettle/base64.h>
#include "vasprintf.h"
#include "extras/hex.h"

/* These functions are like strcat, strcpy. They only
 * do bound checking (they shouldn't cause buffer overruns),
 * and they always produce null terminated strings.
 *
 * They should be used only with null terminated strings.
 */
void _gnutls_str_cat(char *dest, size_t dest_tot_size, const char *src)
{
	size_t str_size = strlen(src);
	size_t dest_size = strlen(dest);

	if (dest_tot_size - dest_size > str_size) {
		strcat(dest, src);
	} else {
		if (dest_tot_size - dest_size > 0) {
			strncat(dest, src,
				(dest_tot_size - dest_size) - 1);
			dest[dest_tot_size - 1] = 0;
		}
	}
}

void _gnutls_str_cpy(char *dest, size_t dest_tot_size, const char *src)
{
	size_t str_size = strlen(src);

	if (dest_tot_size > str_size) {
		strcpy(dest, src);
	} else {
		if (dest_tot_size > 0) {
			memcpy(dest, src, (dest_tot_size) - 1);
			dest[dest_tot_size - 1] = 0;
		}
	}
}

void
_gnutls_mem_cpy(char *dest, size_t dest_tot_size, const char *src,
		size_t src_size)
{

	if (dest_tot_size >= src_size) {
		memcpy(dest, src, src_size);
	} else {
		if (dest_tot_size > 0) {
			memcpy(dest, src, dest_tot_size);
		}
	}
}

void _gnutls_buffer_init(gnutls_buffer_st * str)
{
	str->data = str->allocd = NULL;
	str->max_length = 0;
	str->length = 0;
}

void _gnutls_buffer_replace_data(gnutls_buffer_st * buf,
				 gnutls_datum_t * data)
{
	gnutls_free(buf->allocd);
	buf->allocd = buf->data = data->data;
	buf->max_length = buf->length = data->size;
}

void _gnutls_buffer_clear(gnutls_buffer_st * str)
{
	if (str == NULL || str->allocd == NULL)
		return;
	gnutls_free(str->allocd);

	str->data = str->allocd = NULL;
	str->max_length = 0;
	str->length = 0;
}

#define MIN_CHUNK 1024

static void align_allocd_with_data(gnutls_buffer_st * dest)
{
	if (dest->length)
		memmove(dest->allocd, dest->data, dest->length);
	dest->data = dest->allocd;
}

/**
 * gnutls_buffer_append_data:
 * @dest: the buffer to append to
 * @data: the data
 * @data_size: the size of @data
 *
 * Appends the provided @data to the destination buffer.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, otherwise a negative error code.
 *
 * Since: 3.4.0
 **/
int
gnutls_buffer_append_data(gnutls_buffer_t dest, const void *data,
			   size_t data_size)
{
	size_t const tot_len = data_size + dest->length;
	size_t const unused = MEMSUB(dest->data, dest->allocd);

	if (data_size == 0)
		return 0;

	if (unlikely(sizeof(size_t) == 4 &&
	    INT_ADD_OVERFLOW (((ssize_t)MAX(data_size, MIN_CHUNK)), ((ssize_t)dest->length)))) {
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);
	}

	if (dest->max_length >= tot_len) {

		if (dest->max_length - unused <= tot_len) {
			align_allocd_with_data(dest);
		}
	} else {
		size_t const new_len =
		    MAX(data_size, MIN_CHUNK) + MAX(dest->max_length,
						    MIN_CHUNK);

		dest->allocd = gnutls_realloc_fast(dest->allocd, new_len);
		if (dest->allocd == NULL) {
			gnutls_assert();
			return GNUTLS_E_MEMORY_ERROR;
		}
		dest->max_length = new_len;
		dest->data = dest->allocd + unused;

		align_allocd_with_data(dest);
	}

	memcpy(&dest->data[dest->length], data, data_size);
	dest->length = tot_len;

	return 0;
}

int _gnutls_buffer_resize(gnutls_buffer_st * dest, size_t new_size)
{
	if (dest->max_length >= new_size) {
		size_t unused = MEMSUB(dest->data, dest->allocd);
		if (dest->max_length - unused <= new_size) {
			align_allocd_with_data(dest);
		}

		return 0;
	} else {
		size_t unused = MEMSUB(dest->data, dest->allocd);
		size_t alloc_len =
		    MAX(new_size, MIN_CHUNK) + MAX(dest->max_length,
						   MIN_CHUNK);

		dest->allocd =
		    gnutls_realloc_fast(dest->allocd, alloc_len);
		if (dest->allocd == NULL) {
			gnutls_assert();
			return GNUTLS_E_MEMORY_ERROR;
		}
		dest->max_length = alloc_len;
		dest->data = dest->allocd + unused;

		align_allocd_with_data(dest);

		return 0;
	}
}

/* Appends the provided string. The null termination byte is appended
 * but not included in length.
 */
int _gnutls_buffer_append_str(gnutls_buffer_st * dest, const char *src)
{
	int ret;
	ret = _gnutls_buffer_append_data(dest, src, strlen(src) + 1);
	if (ret >= 0)
		dest->length--;

	return ret;
}

/* returns data from a string in a constant buffer.
 * The data will NOT be valid if buffer is released or
 * data are appended in the buffer.
 */
void
_gnutls_buffer_pop_datum(gnutls_buffer_st * str, gnutls_datum_t * data,
			 size_t req_size)
{

	if (str->length == 0) {
		data->data = NULL;
		data->size = 0;
		return;
	}

	if (req_size > str->length)
		req_size = str->length;

	data->data = str->data;
	data->size = req_size;

	str->data += req_size;
	str->length -= req_size;

	/* if string becomes empty start from begining */
	if (str->length == 0) {
		str->data = str->allocd;
	}

	return;
}

/* converts the buffer to a datum if possible. After this call 
 * (failed or not) the buffer should be considered deinitialized.
 */
int _gnutls_buffer_to_datum(gnutls_buffer_st * str, gnutls_datum_t * data, unsigned is_str)
{
	int ret;

	if (str->length == 0) {
		data->data = NULL;
		data->size = 0;
		ret = 0;
		goto fail;
	}

	if (is_str) {
		ret = _gnutls_buffer_append_data(str, "\x00", 1);
		if (ret < 0) {
			gnutls_assert();
			goto fail;
		}
	}

	if (str->allocd != str->data) {
		data->data = gnutls_malloc(str->length);
		if (data->data == NULL) {
			gnutls_assert();
			ret = GNUTLS_E_MEMORY_ERROR;
			goto fail;
		}
		memcpy(data->data, str->data, str->length);
		data->size = str->length;
		_gnutls_buffer_clear(str);
	} else {
		data->data = str->data;
		data->size = str->length;
		_gnutls_buffer_init(str);
	}

	if (is_str) {
		data->size--;
	}

	return 0;
 fail:
	_gnutls_buffer_clear(str);
	return ret;
}

/* returns data from a string in a constant buffer.
 */
void
_gnutls_buffer_pop_data(gnutls_buffer_st * str, void *data,
			size_t * req_size)
{
	gnutls_datum_t tdata;

	_gnutls_buffer_pop_datum(str, &tdata, *req_size);
	if (tdata.data == NULL) {
		*req_size = 0;
		return;
	}

	*req_size = tdata.size;
	memcpy(data, tdata.data, tdata.size);

	return;
}

int
_gnutls_buffer_append_printf(gnutls_buffer_st * dest, const char *fmt, ...)
{
	va_list args;
	int len;
	char *str = NULL;

	va_start(args, fmt);
	len = vasprintf(&str, fmt, args);
	va_end(args);

	if (len < 0 || !str)
		return -1;

	len = _gnutls_buffer_append_str(dest, str);

	free(str);

	return len;
}

static int
_gnutls_buffer_insert_data(gnutls_buffer_st * dest, int pos,
			   const void *str, size_t str_size)
{
	size_t orig_length = dest->length;
	int ret;

	ret = _gnutls_buffer_resize(dest, dest->length + str_size);	/* resize to make space */
	if (ret < 0)
		return ret;

	memmove(&dest->data[pos + str_size], &dest->data[pos],
		orig_length - pos);

	memcpy(&dest->data[pos], str, str_size);
	dest->length += str_size;

	return 0;
}

static void
_gnutls_buffer_delete_data(gnutls_buffer_st * dest, int pos,
			   size_t str_size)
{
	memmove(&dest->data[pos], &dest->data[pos + str_size],
		dest->length - pos - str_size);

	dest->length -= str_size;

	return;
}


int
_gnutls_buffer_append_escape(gnutls_buffer_st * dest, const void *data,
			     size_t data_size, const char *invalid_chars)
{
	int rv = -1;
	char t[5];
	unsigned int pos = dest->length;

	rv = _gnutls_buffer_append_data(dest, data, data_size);
	if (rv < 0)
		return gnutls_assert_val(rv);

	while (pos < dest->length) {

		if (dest->data[pos] == '\\'
			|| strchr(invalid_chars, dest->data[pos])
			|| !c_isgraph(dest->data[pos])) {

			snprintf(t, sizeof(t), "%%%.2X",
				 (unsigned int) dest->data[pos]);

			_gnutls_buffer_delete_data(dest, pos, 1);

			if (_gnutls_buffer_insert_data(dest, pos, t, 3) < 0) {
				rv = -1;
				goto cleanup;
			}
			pos += 3;
		} else
			pos++;
	}

	rv = 0;

      cleanup:
	return rv;
}

int _gnutls_buffer_unescape(gnutls_buffer_st * dest)
{
	int rv = -1;
	unsigned int pos = 0;

	while (pos < dest->length) {
		if (dest->data[pos] == '%') {
			char b[3];
			unsigned int u;
			unsigned char x;

			b[0] = dest->data[pos + 1];
			b[1] = dest->data[pos + 2];
			b[2] = 0;

			sscanf(b, "%02x", &u);

			x = u;

			_gnutls_buffer_delete_data(dest, pos, 3);
			_gnutls_buffer_insert_data(dest, pos, &x, 1);
		}
		pos++;
	}

	rv = 0;

	return rv;
}


/* Converts the given string (old) to hex. A buffer must be provided
 * to hold the new hex string. The new string will be null terminated.
 * If the buffer does not have enough space to hold the string, a
 * truncated hex string is returned (always null terminated).
 */
char *_gnutls_bin2hex(const void *_old, size_t oldlen,
		      char *buffer, size_t buffer_size,
		      const char *separator)
{
	unsigned int i, j;
	const uint8_t *old = _old;
	int step = 2;
	const char empty[] = "";

	if (separator != NULL && separator[0] != 0)
		step = 3;
	else
		separator = empty;

	if (buffer_size < 3) {
		gnutls_assert();
		return NULL;
	}

	i = j = 0;
	sprintf(&buffer[j], "%.2x", old[i]);
	j += 2;
	i++;

	for (; i < oldlen && j + step < buffer_size; j += step) {
		sprintf(&buffer[j], "%s%.2x", separator, old[i]);
		i++;
	}
	buffer[j] = '\0';

	return buffer;
}

/**
 * gnutls_hex2bin:
 * @hex_data: string with data in hex format
 * @hex_size: size of hex data
 * @bin_data: output array with binary data
 * @bin_size: when calling should hold maximum size of @bin_data,
 *	    on return will hold actual length of @bin_data.
 *
 * Convert a buffer with hex data to binary data. This function
 * unlike gnutls_hex_decode() can parse hex data with separators
 * between numbers. That is, it ignores any non-hex characters.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, otherwise a negative error code.
 *
 * Since: 2.4.0
 **/
int
gnutls_hex2bin(const char *hex_data,
	       size_t hex_size, void *bin_data, size_t * bin_size)
{
	return _gnutls_hex2bin(hex_data, hex_size, (void *) bin_data,
			       bin_size);
}

int
_gnutls_hex2bin(const char *hex_data, size_t hex_size, uint8_t * bin_data,
		size_t * bin_size)
{
	unsigned int i, j;
	uint8_t hex2_data[3];
	unsigned long val;

	hex2_data[2] = 0;

	for (i = j = 0; i < hex_size;) {
		if (!isxdigit(hex_data[i])) {	/* skip non-hex such as the ':' in 00:FF */
			i++;
			continue;
		}
		if (j >= *bin_size) {
			gnutls_assert();
			return GNUTLS_E_SHORT_MEMORY_BUFFER;
		}

		if (i+1 >= hex_size)
			return gnutls_assert_val(GNUTLS_E_PARSING_ERROR);

		hex2_data[0] = hex_data[i];
		hex2_data[1] = hex_data[i + 1];
		i += 2;

		val = strtoul((char *) hex2_data, NULL, 16);
		if (val == ULONG_MAX) {
			gnutls_assert();
			return GNUTLS_E_PARSING_ERROR;
		}
		bin_data[j] = val;
		j++;
	}
	*bin_size = j;

	return 0;
}

/**
 * gnutls_hex_decode2:
 * @hex_data: contain the encoded data
 * @result: the result in an allocated string
 *
 * This function will decode the given encoded data, using the hex
 * encoding used by PSK password files.
 *
 * Returns: %GNUTLS_E_PARSING_ERROR on invalid hex data, or 0 on success.
 **/
int
gnutls_hex_decode2(const gnutls_datum_t * hex_data, gnutls_datum_t *result)
{
	int ret;
	int size = hex_data_size(hex_data->size);

	result->data = gnutls_malloc(size);
	if (result->data == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	result->size = size;
	ret = hex_decode((char *) hex_data->data, hex_data->size,
			 result->data, result->size);
	if (ret == 0) {
		gnutls_assert();
		gnutls_free(result->data);
		return GNUTLS_E_PARSING_ERROR;
	}

	return 0;
}

/**
 * gnutls_hex_decode:
 * @hex_data: contain the encoded data
 * @result: the place where decoded data will be copied
 * @result_size: holds the size of the result
 *
 * This function will decode the given encoded data, using the hex
 * encoding used by PSK password files.
 *
 * Initially @result_size must hold the maximum size available in
 * @result, and on return it will contain the number of bytes written.
 *
 * Returns: %GNUTLS_E_SHORT_MEMORY_BUFFER if the buffer given is not
 *   long enough, %GNUTLS_E_PARSING_ERROR on invalid hex data, or 0 on success.
 **/
int
gnutls_hex_decode(const gnutls_datum_t * hex_data, void *result,
		  size_t * result_size)
{
	int ret;
	size_t size = hex_data_size(hex_data->size);

	if (*result_size < size) {
		gnutls_assert();
		return GNUTLS_E_SHORT_MEMORY_BUFFER;
	}

	ret = hex_decode((char *) hex_data->data, hex_data->size,
			 result, size);
	if (ret == 0) {
		return gnutls_assert_val(GNUTLS_E_PARSING_ERROR);
	}
	*result_size = size;

	return 0;
}

/**
 * gnutls_hex_encode:
 * @data: contain the raw data
 * @result: the place where hex data will be copied
 * @result_size: holds the size of the result
 *
 * This function will convert the given data to printable data, using
 * the hex encoding, as used in the PSK password files.
 *
 * Note that the size of the result includes the null terminator.
 *
 * Returns: %GNUTLS_E_SHORT_MEMORY_BUFFER if the buffer given is not
 * long enough, or 0 on success.
 **/
int
gnutls_hex_encode(const gnutls_datum_t * data, char *result,
		  size_t * result_size)
{
	int ret;
	size_t size = hex_str_size(data->size);

	if (*result_size < size) {
		gnutls_assert();
		return GNUTLS_E_SHORT_MEMORY_BUFFER;
	}

	ret = hex_encode(data->data, data->size, result, *result_size);
	if (ret == 0) {
		return gnutls_assert_val(GNUTLS_E_PARSING_ERROR);
	}

	*result_size = size;

	return 0;
}

/**
 * gnutls_hex_encode2:
 * @data: contain the raw data
 * @result: the result in an allocated string
 *
 * This function will convert the given data to printable data, using
 * the hex encoding, as used in the PSK password files.
 *
 * Note that the size of the result does NOT include the null terminator.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, otherwise a negative error code.
 **/
int
gnutls_hex_encode2(const gnutls_datum_t * data, gnutls_datum_t *result)
{
	int ret;
	int size = hex_str_size(data->size);

	result->data = gnutls_malloc(size);
	if (result->data == NULL) {
		gnutls_assert();
		return GNUTLS_E_MEMORY_ERROR;
	}

	ret = hex_encode((char*)data->data, data->size, (char*)result->data, size); 
	if (ret == 0) {
		gnutls_free(result->data);
		return gnutls_assert_val(GNUTLS_E_PARSING_ERROR);
	}

	result->size = size-1;

	return 0;
}

static int
hostname_compare_raw(const char *certname,
			 size_t certnamesize, const char *hostname)
{
	if (certnamesize == strlen(hostname) && memcmp(hostname, certname, certnamesize) == 0)
		return 1;
	return 0;
}

static int
hostname_compare_ascii(const char *certname,
			 size_t certnamesize, const char *hostname)
{
	for (;
	     *certname && *hostname
	     && c_toupper(*certname) == c_toupper(*hostname);
	     certname++, hostname++, certnamesize--);

	/* the strings are the same */
	if (certnamesize == 0 && *hostname == '\0')
		return 1;

	return 0;
}

/* compare hostname against certificate, taking account of wildcards
 * return 1 on success or 0 on error
 *
 * note: certnamesize is required as X509 certs can contain embedded NULs in
 * the strings such as CN or subjectAltName.
 *
 * Wildcards are taken into account only if they are the leftmost
 * component, and if the string is ascii only (partial advice from rfc6125)
 *
 */
int
_gnutls_hostname_compare(const char *certname,
			 size_t certnamesize, const char *hostname, unsigned vflags)
{
	char *p;
	unsigned i;

	for (i=0;i<certnamesize;i++) {
		if (c_isprint(certname[i]) == 0)
			return hostname_compare_raw(certname, certnamesize, hostname);
	}

	if (*certname == '*' && !(vflags & GNUTLS_VERIFY_DO_NOT_ALLOW_WILDCARDS)) {
		/* a wildcard certificate */

		/* ensure that we have at least two domain components after
		 * the wildcard. */
		p = strrchr(certname, '.');
		if (p == NULL || strchr(certname, '.') == p || p[1] == 0) {
			return 0;
		}

		certname++;
		certnamesize--;

		while (1) {
			if (hostname_compare_ascii(certname, certnamesize, hostname))
				return 1;

			/* wildcards are only allowed to match a single domain
			   component or component fragment */
			if (*hostname == '\0' || *hostname == '.')
				break;
			hostname++;
		}

		return 0;
	} else {
		return hostname_compare_ascii(certname, certnamesize, hostname);
	}
}

int
_gnutls_buffer_append_prefix(gnutls_buffer_st * buf, int pfx_size,
			     size_t data_size)
{
	uint8_t ss[4];

	if (pfx_size == 32) {
		_gnutls_write_uint32(data_size, ss);
		pfx_size = 4;
	} else if (pfx_size == 24) {
		_gnutls_write_uint24(data_size, ss);
		pfx_size = 3;
	} else if (pfx_size == 16) {
		_gnutls_write_uint16(data_size, ss);
		pfx_size = 2;
	} else if (pfx_size == 8) {
		ss[0] = data_size;
		pfx_size = 1;
	} else
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	return _gnutls_buffer_append_data(buf, ss, pfx_size);
}

/* Reads an uint32 number from the buffer. If check is non zero it will also check whether
 * the number read, is less than the data in the buffer
 */
int
_gnutls_buffer_pop_prefix(gnutls_buffer_st * buf, size_t * data_size,
			  int check)
{
	size_t size;

	if (buf->length < 4) {
		gnutls_assert();
		return GNUTLS_E_PARSING_ERROR;
	}

	size = _gnutls_read_uint32(buf->data);
	if (check && size > buf->length - 4) {
		gnutls_assert();
		return GNUTLS_E_PARSING_ERROR;
	}

	buf->data += 4;
	buf->length -= 4;

	*data_size = size;

	return 0;
}

int
_gnutls_buffer_pop_datum_prefix(gnutls_buffer_st * buf,
				gnutls_datum_t * data)
{
	size_t size;
	int ret;

	ret = _gnutls_buffer_pop_prefix(buf, &size, 1);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	if (size > 0) {
		size_t osize = size;
		_gnutls_buffer_pop_datum(buf, data, size);
		if (osize != data->size) {
			gnutls_assert();
			return GNUTLS_E_PARSING_ERROR;
		}
	} else {
		data->size = 0;
		data->data = NULL;
	}

	return 0;
}

int
_gnutls_buffer_append_data_prefix(gnutls_buffer_st * buf,
				  int pfx_size, const void *data,
				  size_t data_size)
{
	int ret = 0, ret1;

	ret1 = _gnutls_buffer_append_prefix(buf, pfx_size, data_size);
	if (ret1 < 0)
		return gnutls_assert_val(ret1);

	if (data_size > 0) {
		ret = _gnutls_buffer_append_data(buf, data, data_size);

		if (ret < 0)
			return gnutls_assert_val(ret);
	}

	return ret + ret1;
}

int _gnutls_buffer_append_mpi(gnutls_buffer_st * buf, int pfx_size,
			      bigint_t mpi, int lz)
{
	gnutls_datum_t dd;
	int ret;

	if (lz)
		ret = _gnutls_mpi_dprint_lz(mpi, &dd);
	else
		ret = _gnutls_mpi_dprint(mpi, &dd);

	if (ret < 0)
		return gnutls_assert_val(ret);

	ret =
	    _gnutls_buffer_append_data_prefix(buf, pfx_size, dd.data,
					      dd.size);

	_gnutls_free_datum(&dd);

	return ret;
}

int
_gnutls_buffer_pop_data_prefix(gnutls_buffer_st * buf, void *data,
			       size_t * data_size)
{
	size_t size;
	int ret;

	ret = _gnutls_buffer_pop_prefix(buf, &size, 1);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	if (size > 0)
		_gnutls_buffer_pop_data(buf, data, data_size);

	return 0;
}

void
_gnutls_buffer_hexprint(gnutls_buffer_st * str,
			const void *_data, size_t len)
{
	size_t j;
	const unsigned char *data = _data;

	if (len == 0)
		_gnutls_buffer_append_str(str, "00");
	else {
		for (j = 0; j < len; j++)
			_gnutls_buffer_append_printf(str, "%.2x",
						     (unsigned) data[j]);
	}
}

int
_gnutls_buffer_base64print(gnutls_buffer_st * str,
			   const void *_data, size_t len)
{
	const unsigned char *data = _data;
	unsigned b64len = BASE64_ENCODE_RAW_LENGTH(len);
	int ret;

	ret = _gnutls_buffer_resize(str, str->length+b64len+1);
	if (ret < 0) {
		return gnutls_assert_val(ret);
	}

	base64_encode_raw(&str->data[str->length], len, data);
	str->length += b64len;
	str->data[str->length] = 0;

	return 0;
}

void
_gnutls_buffer_hexdump(gnutls_buffer_st * str, const void *_data,
		       size_t len, const char *spc)
{
	size_t j;
	const unsigned char *data = _data;

	if (spc)
		_gnutls_buffer_append_str(str, spc);
	for (j = 0; j < len; j++) {
		if (((j + 1) % 16) == 0) {
			_gnutls_buffer_append_printf(str, "%.2x\n",
						     (unsigned) data[j]);
			if (spc && j != (len - 1))
				_gnutls_buffer_append_str(str, spc);
		} else if (j == (len - 1))
			_gnutls_buffer_append_printf(str, "%.2x",
						     (unsigned) data[j]);
		else
			_gnutls_buffer_append_printf(str, "%.2x:",
						     (unsigned) data[j]);
	}
	if ((j % 16) != 0)
		_gnutls_buffer_append_str(str, "\n");
}

void
_gnutls_buffer_asciiprint(gnutls_buffer_st * str,
			  const char *data, size_t len)
{
	size_t j;

	for (j = 0; j < len; j++)
		if (c_isprint(data[j]))
			_gnutls_buffer_append_printf(str, "%c",
						     (unsigned char)
						     data[j]);
		else
			_gnutls_buffer_append_printf(str, ".");
}
