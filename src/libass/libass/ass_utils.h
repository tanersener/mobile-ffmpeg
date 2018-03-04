/*
 * Copyright (C) 2006 Evgeniy Stepanov <eugeni.stepanov@gmail.com>
 *
 * This file is part of libass.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef LIBASS_UTILS_H
#define LIBASS_UTILS_H

#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <math.h>

#include "ass.h"

#ifndef SIZE_MAX
#define SIZE_MAX ((size_t)-1)
#endif

#define MSGL_FATAL 0
#define MSGL_ERR 1
#define MSGL_WARN 2
#define MSGL_INFO 4
#define MSGL_V 6
#define MSGL_DBG2 7

#define FFMAX(a,b) ((a) > (b) ? (a) : (b))
#define FFMIN(a,b) ((a) > (b) ? (b) : (a))
#define FFMINMAX(c,a,b) FFMIN(FFMAX(c, a), b)

#if (defined(__i386__) || defined(__x86_64__)) && CONFIG_ASM
int has_sse2(void);
int has_avx(void);
int has_avx2(void);
#endif

#ifndef HAVE_STRNDUP
char *ass_strndup(const char *s, size_t n);
#define strndup ass_strndup
#endif

void *ass_aligned_alloc(size_t alignment, size_t size, bool zero);
void ass_aligned_free(void *ptr);

void *ass_realloc_array(void *ptr, size_t nmemb, size_t size);
void *ass_try_realloc_array(void *ptr, size_t nmemb, size_t size);

/**
 * Reallocate the array in ptr to at least count elements. For example, if
 * you do "int *ptr = NULL; ASS_REALLOC_ARRAY(ptr, 5)", you can access ptr[0]
 * through ptr[4] (inclusive).
 *
 * If memory allocation fails, ptr is left unchanged, and the macro returns 0:
 * "if (!ASS_REALLOC_ARRAY(ptr, 5)) goto error;"
 *
 * A count of 0 does not free the array (see ass_realloc_array for remarks).
 */
#define ASS_REALLOC_ARRAY(ptr, count) \
    (errno = 0, (ptr) = ass_try_realloc_array(ptr, count, sizeof(*ptr)), !errno)

void skip_spaces(char **str);
void rskip_spaces(char **str, char *limit);
int mystrtoi(char **p, int *res);
int mystrtoll(char **p, long long *res);
int mystrtod(char **p, double *res);
int mystrtoi32(char **p, int base, int32_t *res);
int32_t parse_alpha_tag(char *str);
uint32_t parse_color_tag(char *str);
uint32_t parse_color_header(char *str);
char parse_bool(char *str);
int parse_ycbcr_matrix(char *str);
int numpad2align(int val);
unsigned ass_utf8_get_char(char **str);
unsigned ass_utf8_put_char(char *dest, uint32_t ch);
void ass_utf16be_to_utf8(char *dst, size_t dst_size, uint8_t *src, size_t src_size);
void ass_msg(ASS_Library *priv, int lvl, const char *fmt, ...);
int lookup_style(ASS_Track *track, char *name);
ASS_Style *lookup_style_strict(ASS_Track *track, char *name, size_t len);

/* defined in ass_strtod.c */
double ass_strtod(const char *string, char **endPtr);

static inline size_t ass_align(size_t alignment, size_t s)
{
    if (s > SIZE_MAX - (alignment - 1))
        return s;
    return (s + (alignment - 1)) & ~(alignment - 1);
}

static inline uint32_t ass_bswap32(uint32_t x)
{
#ifdef _MSC_VER
    return _byteswap_ulong(x);
#else
    return (x & 0x000000FF) << 24 | (x & 0x0000FF00) <<  8 |
           (x & 0x00FF0000) >>  8 | (x & 0xFF000000) >> 24;
#endif
}

static inline int d6_to_int(int x)
{
    return (x + 32) >> 6;
}
static inline int d16_to_int(int x)
{
    return (x + 32768) >> 16;
}
static inline int int_to_d6(int x)
{
    return x * (1 << 6);
}
static inline int int_to_d16(int x)
{
    return x * (1 << 16);
}
static inline int d16_to_d6(int x)
{
    return (x + 512) >> 10;
}
static inline int d6_to_d16(int x)
{
    return x * (1 << 10);
}
static inline double d6_to_double(int x)
{
    return x / 64.;
}
static inline int double_to_d6(double x)
{
    return (int) (x * 64);
}
static inline double d16_to_double(int x)
{
    return ((double) x) / 0x10000;
}
static inline int double_to_d16(double x)
{
    return (int) (x * 0x10000);
}
static inline double d22_to_double(int x)
{
    return ((double) x) / 0x400000;
}
static inline int double_to_d22(double x)
{
    return (int) (x * 0x400000);
}

// Calculate cache key for a rotational angle in radians
static inline int rot_key(double a)
{
    return double_to_d22(remainder(a, 2 * M_PI));
}

#define FNV1_32A_INIT 0x811c9dc5U
#define FNV1_32A_PRIME 16777619U

static inline unsigned fnv_32a_buf(void *buf, size_t len, unsigned hval)
{
    unsigned char *bp = (unsigned char*)buf;
    size_t n = (len + 3) / 4;

    switch (len % 4) {
    case 0: do { hval ^= (unsigned) *bp++; hval *= FNV1_32A_PRIME;
    case 3:      hval ^= (unsigned) *bp++; hval *= FNV1_32A_PRIME;
    case 2:      hval ^= (unsigned) *bp++; hval *= FNV1_32A_PRIME;
    case 1:      hval ^= (unsigned) *bp++; hval *= FNV1_32A_PRIME;
               } while (--n > 0);
    }

    return hval;
}
static inline unsigned fnv_32a_str(char *str, unsigned hval)
{
    unsigned char *s = (unsigned char *) str;
    while (*s) {
        hval ^= (unsigned) *s++;
        hval *= FNV1_32A_PRIME;
    }
    return hval;
}

#endif                          /* LIBASS_UTILS_H */
