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

#include "config.h"
#include "ass_compat.h"

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <inttypes.h>

#include "ass_library.h"
#include "ass.h"
#include "ass_utils.h"
#include "ass_string.h"

#if (defined(__i386__) || defined(__x86_64__)) && CONFIG_ASM

#include "x86/cpuid.h"

int has_sse2(void)
{
    uint32_t eax = 1, ebx, ecx, edx;
    ass_get_cpuid(&eax, &ebx, &ecx, &edx);
    return (edx >> 26) & 0x1;
}

int has_avx(void)
{
    uint32_t eax = 1, ebx, ecx, edx;
    ass_get_cpuid(&eax, &ebx, &ecx, &edx);
    if(!(ecx & (1 << 27))) // not OSXSAVE
        return 0;
    uint32_t misc = ecx;
    ass_get_xgetbv(0, &eax, &edx);
    if((eax & 0x6) != 0x6)
        return 0;
    eax = 0;
    ass_get_cpuid(&eax, &ebx, &ecx, &edx);
    return (ecx & 0x6) == 0x6 ? (misc >> 28) & 0x1 : 0; // check high bits are relevant, then AVX support
}

int has_avx2(void)
{
    uint32_t eax = 7, ebx, ecx, edx;
    ass_get_cpuid(&eax, &ebx, &ecx, &edx);
    return (ebx >> 5) & has_avx();
}

#endif // ASM

#ifndef HAVE_STRNDUP
char *ass_strndup(const char *s, size_t n)
{
    char *end = memchr(s, 0, n);
    size_t len = end ? end - s : n;
    char *new = len < SIZE_MAX ? malloc(len + 1) : NULL;
    if (new) {
        memcpy(new, s, len);
        new[len] = 0;
    }
    return new;
}
#endif

void *ass_aligned_alloc(size_t alignment, size_t size, bool zero)
{
    assert(!(alignment & (alignment - 1))); // alignment must be power of 2
    if (size >= SIZE_MAX - alignment - sizeof(void *))
        return NULL;
    char *allocation = zero ? calloc(size + sizeof(void *) + alignment - 1, 1)
                            : malloc(size + sizeof(void *) + alignment - 1);
    if (!allocation)
        return NULL;
    char *ptr = allocation + sizeof(void *);
    unsigned int misalign = (uintptr_t)ptr & (alignment - 1);
    if (misalign)
        ptr += alignment - misalign;
    *((void **)ptr - 1) = allocation;
    return ptr;
}

void ass_aligned_free(void *ptr)
{
    if (ptr)
        free(*((void **)ptr - 1));
}

/**
 * This works similar to realloc(ptr, nmemb * size), but checks for overflow.
 *
 * Unlike some implementations of realloc, this never acts as a call to free().
 * If the total size is 0, it is bumped up to 1. This means a NULL return always
 * means allocation failure, and the unportable realloc(0, 0) case is avoided.
 */
void *ass_realloc_array(void *ptr, size_t nmemb, size_t size)
{
    if (nmemb > (SIZE_MAX / size))
        return NULL;
    size *= nmemb;
    if (size < 1)
        size = 1;

    return realloc(ptr, size);
}

/**
 * Like ass_realloc_array(), but:
 * 1. on failure, return the original ptr value, instead of NULL
 * 2. set errno to indicate failure (errno!=0) or success (errno==0)
 */
void *ass_try_realloc_array(void *ptr, size_t nmemb, size_t size)
{
    void *new_ptr = ass_realloc_array(ptr, nmemb, size);
    if (new_ptr) {
        errno = 0;
        return new_ptr;
    } else {
        errno = ENOMEM;
        return ptr;
    }
}

void skip_spaces(char **str)
{
    char *p = *str;
    while ((*p == ' ') || (*p == '\t'))
        ++p;
    *str = p;
}

void rskip_spaces(char **str, char *limit)
{
    char *p = *str;
    while ((p > limit) && ((p[-1] == ' ') || (p[-1] == '\t')))
        --p;
    *str = p;
}

int mystrtoi(char **p, int *res)
{
    char *start = *p;
    double temp_res = ass_strtod(*p, p);
    *res = (int) (temp_res + (temp_res > 0 ? 0.5 : -0.5));
    return *p != start;
}

int mystrtoll(char **p, long long *res)
{
    char *start = *p;
    double temp_res = ass_strtod(*p, p);
    *res = (long long) (temp_res + (temp_res > 0 ? 0.5 : -0.5));
    return *p != start;
}

int mystrtod(char **p, double *res)
{
    char *start = *p;
    *res = ass_strtod(*p, p);
    return *p != start;
}

int mystrtoi32(char **p, int base, int32_t *res)
{
    char *start = *p;
    long long temp_res = strtoll(*p, p, base);
    *res = FFMINMAX(temp_res, INT32_MIN, INT32_MAX);
    return *p != start;
}

static int read_digits(char **str, int base, uint32_t *res)
{
    char *p = *str;
    char *start = p;
    uint32_t val = 0;

    while (1) {
        int digit;
        if (*p >= '0' && *p < FFMIN(base, 10) + '0')
            digit = *p - '0';
        else if (*p >= 'a' && *p < base - 10 + 'a')
            digit = *p - 'a' + 10;
        else if (*p >= 'A' && *p < base - 10 + 'A')
            digit = *p - 'A' + 10;
        else
            break;
        val = val * base + digit;
        ++p;
    }

    *res = val;
    *str = p;
    return p != start;
}

/**
 * \brief Convert a string to an integer reduced modulo 2**32
 * Follows the rules for strtoul but reduces the number modulo 2**32
 * instead of saturating it to 2**32 - 1.
 */
static int mystrtou32_modulo(char **p, int base, uint32_t *res)
{
    // This emulates scanf with %d or %x format as it works on
    // Windows, because that's what is used by VSFilter. In practice,
    // scanf works the same way on other platforms too, but
    // the standard leaves its behavior on overflow undefined.

    // Unlike scanf and like strtoul, produce 0 for invalid inputs.

    char *start = *p;
    int sign = 1;

    skip_spaces(p);

    if (**p == '+')
        ++*p;
    else if (**p == '-')
        sign = -1, ++*p;

    if (base == 16 && !ass_strncasecmp(*p, "0x", 2))
        *p += 2;

    if (read_digits(p, base, res)) {
        *res *= sign;
        return 1;
    } else {
        *p = start;
        return 0;
    }
}

int32_t parse_alpha_tag(char *str)
{
    int32_t alpha = 0;

    while (*str == '&' || *str == 'H')
        ++str;

    mystrtoi32(&str, 16, &alpha);
    return alpha;
}

uint32_t parse_color_tag(char *str)
{
    int32_t color = 0;

    while (*str == '&' || *str == 'H')
        ++str;

    mystrtoi32(&str, 16, &color);
    return ass_bswap32((uint32_t) color);
}

uint32_t parse_color_header(char *str)
{
    uint32_t color = 0;
    int base;

    if (!ass_strncasecmp(str, "&h", 2) || !ass_strncasecmp(str, "0x", 2)) {
        str += 2;
        base = 16;
    } else
        base = 10;

    mystrtou32_modulo(&str, base, &color);
    return ass_bswap32(color);
}

// Return a boolean value for a string
char parse_bool(char *str)
{
    skip_spaces(&str);
    return !ass_strncasecmp(str, "yes", 3) || strtol(str, NULL, 10) > 0;
}

int parse_ycbcr_matrix(char *str)
{
    skip_spaces(&str);
    if (*str == '\0')
        return YCBCR_DEFAULT;

    char *end = str + strlen(str);
    rskip_spaces(&end, str);

    // Trim a local copy of the input that we know is safe to
    // modify. The buffer is larger than any valid string + NUL,
    // so we can simply chop off the rest of the input.
    char buffer[16];
    size_t n = FFMIN(end - str, sizeof buffer - 1);
    memcpy(buffer, str, n);
    buffer[n] = '\0';

    if (!ass_strcasecmp(buffer, "none"))
        return YCBCR_NONE;
    if (!ass_strcasecmp(buffer, "tv.601"))
        return YCBCR_BT601_TV;
    if (!ass_strcasecmp(buffer, "pc.601"))
        return YCBCR_BT601_PC;
    if (!ass_strcasecmp(buffer, "tv.709"))
        return YCBCR_BT709_TV;
    if (!ass_strcasecmp(buffer, "pc.709"))
        return YCBCR_BT709_PC;
    if (!ass_strcasecmp(buffer, "tv.240m"))
        return YCBCR_SMPTE240M_TV;
    if (!ass_strcasecmp(buffer, "pc.240m"))
        return YCBCR_SMPTE240M_PC;
    if (!ass_strcasecmp(buffer, "tv.fcc"))
        return YCBCR_FCC_TV;
    if (!ass_strcasecmp(buffer, "pc.fcc"))
        return YCBCR_FCC_PC;
    return YCBCR_UNKNOWN;
}

/**
 * \brief converts numpad-style align to align.
 */
int numpad2align(int val)
{
    if (val < -INT_MAX)
        // Pick an alignment somewhat arbitrarily. VSFilter handles
        // INT32_MIN as a mix of 1, 2 and 3, so prefer one of those values.
        val = 2;
    else if (val < 0)
        val = -val;

    int res = ((val - 1) % 3) + 1;  // horizontal alignment
    if (val <= 3)
        res |= VALIGN_SUB;
    else if (val <= 6)
        res |= VALIGN_CENTER;
    else
        res |= VALIGN_TOP;
    return res;
}

void ass_msg(ASS_Library *priv, int lvl, const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    priv->msg_callback(lvl, fmt, va, priv->msg_callback_data);
    va_end(va);
}

unsigned ass_utf8_get_char(char **str)
{
    uint8_t *strp = (uint8_t *) * str;
    unsigned c = *strp++;
    unsigned mask = 0x80;
    int len = -1;
    while (c & mask) {
        mask >>= 1;
        len++;
    }
    if (len <= 0 || len > 4)
        goto no_utf8;
    c &= mask - 1;
    while ((*strp & 0xc0) == 0x80) {
        if (len-- <= 0)
            goto no_utf8;
        c = (c << 6) | (*strp++ & 0x3f);
    }
    if (len)
        goto no_utf8;
    *str = (char *) strp;
    return c;

  no_utf8:
    strp = (uint8_t *) * str;
    c = *strp++;
    *str = (char *) strp;
    return c;
}

/**
 * Original version from http://www.cprogramming.com/tutorial/utf8.c
 * \brief Converts a single UTF-32 code point to UTF-8
 * \param dest Buffer to write to. Writes a NULL terminator.
 * \param ch 32-bit character code to convert
 * \return number of bytes written
 * converts a single character and ASSUMES YOU HAVE ENOUGH SPACE
 */
unsigned ass_utf8_put_char(char *dest, uint32_t ch)
{
    char *orig_dest = dest;

    if (ch < 0x80) {
        *dest++ = (char)ch;
    } else if (ch < 0x800) {
        *dest++ = (ch >> 6) | 0xC0;
        *dest++ = (ch & 0x3F) | 0x80;
    } else if (ch < 0x10000) {
        *dest++ = (ch >> 12) | 0xE0;
        *dest++ = ((ch >> 6) & 0x3F) | 0x80;
        *dest++ = (ch & 0x3F) | 0x80;
    } else if (ch < 0x110000) {
        *dest++ = (ch >> 18) | 0xF0;
        *dest++ = ((ch >> 12) & 0x3F) | 0x80;
        *dest++ = ((ch >> 6) & 0x3F) | 0x80;
        *dest++ = (ch & 0x3F) | 0x80;
    }

    *dest = '\0';
    return dest - orig_dest;
}

/**
 * \brief Parse UTF-16 and return the code point of the sequence starting at src.
 * \param src pointer to a pointer to the start of the UTF-16 data
 *            (will be set to the start of the next code point)
 * \return the code point
 */
static uint32_t ass_read_utf16be(uint8_t **src, size_t bytes)
{
    if (bytes < 2)
        goto too_short;

    uint32_t cp = ((*src)[0] << 8) | (*src)[1];
    *src += 2;
    bytes -= 2;

    if (cp >= 0xD800 && cp <= 0xDBFF) {
        if (bytes < 2)
            goto too_short;

        uint32_t cp2 = ((*src)[0] << 8) | (*src)[1];

        if (cp2 < 0xDC00 || cp2 > 0xDFFF)
            return 0xFFFD;

        *src += 2;

        cp = 0x10000 + ((cp - 0xD800) << 10) + (cp2 - 0xDC00);
    }

    if (cp >= 0xDC00 && cp <= 0xDFFF)
        return 0xFFFD;

    return cp;

too_short:
    *src += bytes;
    return 0xFFFD;
}

void ass_utf16be_to_utf8(char *dst, size_t dst_size, uint8_t *src, size_t src_size)
{
    uint8_t *end = src + src_size;

    if (!dst_size)
        return;

    while (src < end) {
        uint32_t cp = ass_read_utf16be(&src, end - src);
        if (dst_size < 5)
            break;
        unsigned s = ass_utf8_put_char(dst, cp);
        dst += s;
        dst_size -= s;
    }

    *dst = '\0';
}

/**
 * \brief find style by name
 * \param track track
 * \param name style name
 * \return index in track->styles
 * Returns 0 if no styles found => expects at least 1 style.
 * Parsing code always adds "Default" style in the beginning.
 */
int lookup_style(ASS_Track *track, char *name)
{
    int i;
    // '*' seem to mean literally nothing;
    // VSFilter removes them as soon as it can
    while (*name == '*')
        ++name;
    // VSFilter then normalizes the case of "Default"
    // (only in contexts where this function is called)
    if (ass_strcasecmp(name, "Default") == 0)
        name = "Default";
    for (i = track->n_styles - 1; i >= 0; --i) {
        if (strcmp(track->styles[i].Name, name) == 0)
            return i;
    }
    i = track->default_style;
    ass_msg(track->library, MSGL_WARN,
            "[%p]: Warning: no style named '%s' found, using '%s'",
            track, name, track->styles[i].Name);
    return i;
}

/**
 * \brief find style by name as in \r
 * \param track track
 * \param name style name
 * \param len style name length
 * \return style in track->styles
 * Returns NULL if no style has the given name.
 */
ASS_Style *lookup_style_strict(ASS_Track *track, char *name, size_t len)
{
    int i;
    for (i = track->n_styles - 1; i >= 0; --i) {
        if (strncmp(track->styles[i].Name, name, len) == 0 &&
            track->styles[i].Name[len] == '\0')
            return track->styles + i;
    }
    ass_msg(track->library, MSGL_WARN,
            "[%p]: Warning: no style named '%.*s' found",
            track, (int) len, name);
    return NULL;
}

