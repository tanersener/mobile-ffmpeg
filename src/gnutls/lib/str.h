/*
 * Copyright (C) 2000-2012 Free Software Foundation, Inc.
 * Copyright (C) 2016-2017 Red Hat, Inc.
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

#ifndef GNUTLS_LIB_STR_H
#define GNUTLS_LIB_STR_H

#include <config.h>
#include "gnutls_int.h"
#include "errors.h"
#include <datum.h>
#include <c-ctype.h>
#include "errors.h"

#ifdef HAVE_DCGETTEXT
# include "gettext.h"
# define _(String) dgettext (PACKAGE, String)
# define N_(String) gettext_noop (String)
#else
# define _(String) String
# define N_(String) String
#endif

int gnutls_utf8_password_normalize(const uint8_t *password, unsigned password_len,
				   gnutls_datum_t *out, unsigned flags);

#define _gnutls_utf8_password_normalize(p, plen, out, ignore_errs) \
	gnutls_utf8_password_normalize((unsigned char*)p, plen, out, \
		ignore_errs?(GNUTLS_UTF8_IGNORE_ERRS):0)

int _gnutls_idna_email_map(const char *input, unsigned ilen, gnutls_datum_t *output);
int _gnutls_idna_email_reverse_map(const char *input, unsigned ilen, gnutls_datum_t *output);

inline static unsigned _gnutls_str_is_print(const char *str, unsigned size)
{
	unsigned i;
	for (i=0;i<size;i++) {
		if (!c_isprint(str[i]))
			return 0;
	}
	return 1;
}

inline static unsigned _gnutls_dnsname_is_valid(const char *str, unsigned size)
{
	unsigned i;
	for (i=0;i<size;i++) {
		if (!(c_isalnum(str[i]) || str[i] == '-' || str[i] == '.'))
			return 0;
	}
	return 1;
}

inline static bool _gnutls_has_embedded_null(const char *str, unsigned size)
{
	if (strlen(str) != size)
		return true;
	return false;
}

void _gnutls_str_cpy(char *dest, size_t dest_tot_size, const char *src);
void _gnutls_str_cat(char *dest, size_t dest_tot_size, const char *src);

typedef struct gnutls_buffer_st {
	uint8_t *allocd;	/* pointer to allocated data */
	uint8_t *data;		/* API: pointer to data to copy from */
	size_t max_length;
	size_t length;		/* API: current length */
} gnutls_buffer_st;

/* Initialize a buffer */
void _gnutls_buffer_init(gnutls_buffer_st *);

/* Free the data in a buffer */
void _gnutls_buffer_clear(gnutls_buffer_st *);

/* Set the buffer data to be of zero length */
inline static void _gnutls_buffer_reset(gnutls_buffer_st * buf)
{
	buf->data = buf->allocd;
	buf->length = 0;
}

int _gnutls_buffer_resize(gnutls_buffer_st *, size_t new_size);

int _gnutls_buffer_append_str(gnutls_buffer_st *, const char *str);

#define _gnutls_buffer_append_data gnutls_buffer_append_data

#include <num.h>

int _gnutls_buffer_append_prefix(gnutls_buffer_st * buf, int pfx_size,
				 size_t data_size);

int _gnutls_buffer_append_mpi(gnutls_buffer_st * buf, int pfx_size,
			      bigint_t, int lz);

int _gnutls_buffer_append_fixed_mpi(gnutls_buffer_st * buf,
				    bigint_t mpi, unsigned size);

int _gnutls_buffer_append_data_prefix(gnutls_buffer_st * buf, int pfx_size,
				      const void *data, size_t data_size);
int _gnutls_buffer_pop_data(gnutls_buffer_st *, void *, size_t size);
void _gnutls_buffer_pop_datum(gnutls_buffer_st *, gnutls_datum_t *,
			      size_t max_size);

int _gnutls_buffer_pop_prefix8(gnutls_buffer_st *, uint8_t *, int check);

/* 32-bit prefix */
int _gnutls_buffer_pop_prefix32(gnutls_buffer_st * buf, size_t * data_size,
			        int check);

int _gnutls_buffer_pop_prefix24(gnutls_buffer_st * buf, size_t * data_size,
			        int check);

/* 32-bit prefix */
int _gnutls_buffer_pop_datum_prefix32(gnutls_buffer_st * buf,
				      gnutls_datum_t * data);

/* 16-bit prefix */
int _gnutls_buffer_pop_datum_prefix16(gnutls_buffer_st * buf,
				      gnutls_datum_t * data);

/* 8-bit prefix */
int _gnutls_buffer_pop_datum_prefix8(gnutls_buffer_st * buf,
				     gnutls_datum_t * data);

int _gnutls_buffer_to_datum(gnutls_buffer_st * str, gnutls_datum_t * data, unsigned is_str);

inline static
void _gnutls_ro_buffer_from_datum(gnutls_buffer_st * str, gnutls_datum_t * data)
{
	_gnutls_buffer_init(str);
	str->length = data->size;
	str->max_length = data->size;
	str->data = data->data;
}

int
_gnutls_buffer_append_escape(gnutls_buffer_st * dest, const void *data,
			     size_t data_size, const char *invalid_chars);
int _gnutls_buffer_unescape(gnutls_buffer_st * dest);

#ifndef __attribute__
/* This feature is available in gcc versions 2.5 and later.  */
#if __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 5)
#define __attribute__(Spec)	/* empty */
#endif
#endif

int _gnutls_buffer_append_printf(gnutls_buffer_st * dest, const char *fmt,
				 ...)
    __attribute__ ((format(printf, 2, 3)));

void _gnutls_buffer_hexprint(gnutls_buffer_st * str,
			     const void *data, size_t len);
int _gnutls_buffer_base64print(gnutls_buffer_st * str,
			        const void *data, size_t len);
void _gnutls_buffer_hexdump(gnutls_buffer_st * str, const void *data,
			    size_t len, const char *spc);
void _gnutls_buffer_asciiprint(gnutls_buffer_st * str,
			       const char *data, size_t len);

char *_gnutls_bin2hex(const void *old, size_t oldlen, char *buffer,
		      size_t buffer_size, const char *separator);
int _gnutls_hex2bin(const char *hex_data, size_t hex_size,
		    uint8_t * bin_data, size_t * bin_size);

int _gnutls_hostname_compare(const char *certname, size_t certnamesize,
			     const char *hostname, unsigned vflags);

#define MAX_CN 256
#define MAX_DN 1024

#define BUFFER_APPEND(b, x, s) { \
	ret = _gnutls_buffer_append_data(b, x, s); \
	if (ret < 0) { \
	    gnutls_assert(); \
	    return ret; \
	} \
    }

/* append data prefixed with 4-bytes length field*/
#define BUFFER_APPEND_PFX4(b, x, s) { \
	ret = _gnutls_buffer_append_data_prefix(b, 32, x, s); \
	if (ret < 0) { \
	    gnutls_assert(); \
	    return ret; \
	} \
    }

#define BUFFER_APPEND_PFX3(b, x, s) { \
	ret = _gnutls_buffer_append_data_prefix(b, 24, x, s); \
	if (ret < 0) { \
	    gnutls_assert(); \
	    return ret; \
	} \
    }

#define BUFFER_APPEND_PFX2(b, x, s) { \
	ret = _gnutls_buffer_append_data_prefix(b, 16, x, s); \
	if (ret < 0) { \
	    gnutls_assert(); \
	    return ret; \
	} \
    }

#define BUFFER_APPEND_PFX1(b, x, s) { \
	ret = _gnutls_buffer_append_data_prefix(b, 8, x, s); \
	if (ret < 0) { \
	    gnutls_assert(); \
	    return ret; \
	} \
    }

#define BUFFER_APPEND_NUM(b, s) { \
	ret = _gnutls_buffer_append_prefix(b, 32, s); \
	if (ret < 0) { \
	    gnutls_assert(); \
	    return ret; \
	} \
    }

#define BUFFER_APPEND_TS(b, s) { \
	ret = _gnutls_buffer_append_prefix(b, 32, (uint64_t) s.tv_sec >> 32); \
	if (ret < 0) { \
	    gnutls_assert(); \
	    return ret; \
	} \
	ret = _gnutls_buffer_append_prefix(b, 32, s.tv_sec & 0xFFFFFFFF); \
	if (ret < 0) { \
	    gnutls_assert(); \
	    return ret; \
	} \
	ret = _gnutls_buffer_append_prefix(b, 32, s.tv_nsec); \
	if (ret < 0) { \
	    gnutls_assert(); \
	    return ret; \
	} \
    }

#define BUFFER_POP(b, x, s) { \
	ret = _gnutls_buffer_pop_data(b, x, s); \
	if (ret < 0) { \
	    ret = GNUTLS_E_PARSING_ERROR; \
	    gnutls_assert(); \
	    goto error; \
	} \
    }

#define BUFFER_POP_DATUM(b, o) { \
	gnutls_datum_t d; \
	ret = _gnutls_buffer_pop_datum_prefix32(b, &d); \
	if (ret >= 0) \
	    ret = _gnutls_set_datum (o, d.data, d.size); \
	if (ret < 0) { \
	    gnutls_assert(); \
	    goto error; \
	} \
    }

#define BUFFER_POP_NUM(b, o) { \
	size_t s; \
	ret = _gnutls_buffer_pop_prefix32(b, &s, 0); \
	if (ret < 0) { \
	    gnutls_assert(); \
	    goto error; \
	} \
	o = s; \
    }

#define BUFFER_POP_CAST_NUM(b, o) { \
	size_t s; \
	ret = _gnutls_buffer_pop_prefix32(b, &s, 0); \
	if (ret < 0) { \
	    gnutls_assert(); \
	    goto error; \
	} \
	o = (void *) (intptr_t)(s); \
    }

#define BUFFER_POP_TS(b, o) { \
	size_t s; \
	uint64_t v; \
	ret = _gnutls_buffer_pop_prefix32(b, &s, 0); \
	if (ret < 0) { \
	    gnutls_assert(); \
	    goto error; \
	} \
	v = s; \
	ret = _gnutls_buffer_pop_prefix32(b, &s, 0); \
	if (ret < 0) { \
	    gnutls_assert(); \
	    goto error; \
	} \
	v = (v << 32) | s; \
	ret = _gnutls_buffer_pop_prefix32(b, &s, 0); \
	if (ret < 0) { \
	    gnutls_assert(); \
	    goto error; \
	} \
	o.tv_sec = v; \
	o.tv_nsec = s; \
    }

#endif /* GNUTLS_LIB_STR_H */
