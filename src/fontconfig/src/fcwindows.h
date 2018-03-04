/*
 * fontconfig/src/fcwindows.h
 *
 * Copyright © 2013  Google, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the author(s) not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  The authors make no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * THE AUTHOR(S) DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Google Author(s): Behdad Esfahbod
 */

#ifndef _FCWINDOWS_H_
#define _FCWINDOWS_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef _WIN32
   /* Request Windows Vista for building.  This is required to
    * get MemoryBarrier on mingw32... */
#  if defined(_WIN32_WINNT) && _WIN32_WINNT < 0x0600
#    undef _WIN32_WINNT
#  endif
#  ifndef _WIN32_WINNT
#    define _WIN32_WINNT 0x0600
#  endif
#  define WIN32_LEAN_AND_MEAN
#  define WIN32_EXTRA_LEAN
#  define STRICT
#  include <windows.h>
#endif

#endif /* _FCWINDOWS_H_ */
