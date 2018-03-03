/*
 *      Command line frontend program
 *
 *      Copyright (c) 1999 Mark Taylor
 *                    2000 Takehiro TOMINAGA
 *                    2010-2012 Robert Hegemann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/* $Id: main.c,v 1.131 2017/08/12 18:56:15 robert Exp $ */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <stdio.h>

#ifdef STDC_HEADERS
# include <stdlib.h>
# include <string.h>
#else
# ifndef HAVE_STRCHR
#  define strchr index
#  define strrchr rindex
# endif
char   *strchr(), *strrchr();
# ifndef HAVE_MEMCPY
#  define memcpy(d, s, n) bcopy ((s), (d), (n))
#  define memmove(d, s, n) bcopy ((s), (d), (n))
# endif
#endif

#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif

#ifdef __sun__
/* woraround for SunOS 4.x, it has SEEK_* defined here */
#include <unistd.h>
#endif

#ifdef __OS2__
#include <os2.h>
#define PRTYC_IDLE 1
#define PRTYC_REGULAR 2
#define PRTYD_MINIMUM -31
#define PRTYD_MAXIMUM 31
#endif

#if defined(_WIN32)
# include <windows.h>
#endif


/*
 main.c is example code for how to use libmp3lame.a.  To use this library,
 you only need the library and lame.h.  All other .h files are private
 to the library.
*/
#include "lame.h"

#include "console.h"
#include "main.h"

/* PLL 14/04/2000 */
#if macintosh
#include <console.h>
#endif

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif


static int c_main(int argc, char *argv[]);
extern int lame_main(lame_t gf, int argc, char *argv[]);


/************************************************************************
*
* main
*
* PURPOSE:  MPEG-1,2 Layer III encoder with GPSYCHO
* psychoacoustic model.
*
************************************************************************/


#if defined( _WIN32 ) && !defined(__MINGW32__)
static void
set_process_affinity()
{
#if 0
    /* rh 061207
       the following fix seems to be a workaround for a problem in the
       parent process calling LAME. It would be better to fix the broken
       application => code disabled.
     */
#if defined(_WIN32)
    /* set affinity back to all CPUs.  Fix for EAC/lame on SMP systems from
       "Todd Richmond" <todd.richmond@openwave.com> */
    typedef BOOL(WINAPI * SPAMFunc) (HANDLE, DWORD_PTR);
    SPAMFunc func;
    SYSTEM_INFO si;

    if ((func = (SPAMFunc) GetProcAddress(GetModuleHandleW(L"KERNEL32.DLL"),
                                          "SetProcessAffinityMask")) != NULL) {
        GetSystemInfo(&si);
        func(GetCurrentProcess(), si.dwActiveProcessorMask);
    }
#endif
#endif
}
#endif

#if defined(WIN32)

/**
 *  Long Filename support for the WIN32 platform
 *
 */

void
dosToLongFileName(char *fn)
{
    const size_t MSIZE = PATH_MAX + 1 - 4; /*  we wanna add ".mp3" later */
    WIN32_FIND_DATAA lpFindFileData;
    HANDLE  h = FindFirstFileA(fn, &lpFindFileData);
    if (h != INVALID_HANDLE_VALUE) {
        size_t  a;
        char   *q, *p;
        FindClose(h);
        for (a = 0; a < MSIZE; a++) {
            if ('\0' == lpFindFileData.cFileName[a])
                break;
        }
        if (a >= MSIZE || a == 0)
            return;
        q = strrchr(fn, '\\');
        p = strrchr(fn, '/');
        if (p - q > 0)
            q = p;
        if (q == NULL)
            q = strrchr(fn, ':');
        if (q == NULL)
            strncpy(fn, lpFindFileData.cFileName, a);
        else {
            a += q - fn + 1;
            if (a >= MSIZE)
                return;
            strncpy(++q, lpFindFileData.cFileName, MSIZE - a);
        }
    }
}

BOOL
SetPriorityClassMacro(DWORD p)
{
    HANDLE  op = GetCurrentProcess();
    return SetPriorityClass(op, p);
}

void
setProcessPriority(int Priority)
{
    switch (Priority) {
    case 0:
    case 1:
        SetPriorityClassMacro(IDLE_PRIORITY_CLASS);
        console_printf("==> Priority set to Low.\n");
        break;
    default:
    case 2:
        SetPriorityClassMacro(NORMAL_PRIORITY_CLASS);
        console_printf("==> Priority set to Normal.\n");
        break;
    case 3:
    case 4:
        SetPriorityClassMacro(HIGH_PRIORITY_CLASS);
        console_printf("==> Priority set to High.\n");
        break;
    }
}
#endif


#if defined(__OS2__)
/* OS/2 priority functions */
static void
setProcessPriority(int Priority)
{
    int     rc;

    switch (Priority) {

    case 0:
        rc = DosSetPriority(0, /* Scope: only one process */
                            PRTYC_IDLE, /* select priority class (idle, regular, etc) */
                            0, /* set delta */
                            0); /* Assume current process */
        console_printf("==> Priority set to 0 (Low priority).\n");
        break;

    case 1:
        rc = DosSetPriority(0, /* Scope: only one process */
                            PRTYC_IDLE, /* select priority class (idle, regular, etc) */
                            PRTYD_MAXIMUM, /* set delta */
                            0); /* Assume current process */
        console_printf("==> Priority set to 1 (Medium priority).\n");
        break;

    case 2:
        rc = DosSetPriority(0, /* Scope: only one process */
                            PRTYC_REGULAR, /* select priority class (idle, regular, etc) */
                            PRTYD_MINIMUM, /* set delta */
                            0); /* Assume current process */
        console_printf("==> Priority set to 2 (Regular priority).\n");
        break;

    case 3:
        rc = DosSetPriority(0, /* Scope: only one process */
                            PRTYC_REGULAR, /* select priority class (idle, regular, etc) */
                            0, /* set delta */
                            0); /* Assume current process */
        console_printf("==> Priority set to 3 (High priority).\n");
        break;

    case 4:
        rc = DosSetPriority(0, /* Scope: only one process */
                            PRTYC_REGULAR, /* select priority class (idle, regular, etc) */
                            PRTYD_MAXIMUM, /* set delta */
                            0); /* Assume current process */
        console_printf("==> Priority set to 4 (Maximum priority). I hope you enjoy it :)\n");
        break;

    default:
        console_printf("==> Invalid priority specified! Assuming idle priority.\n");
    }
}
#endif


/***********************************************************************
*
*  Message Output
*
***********************************************************************/


#if defined( _WIN32 ) && !defined(__MINGW32__)
/* Idea for unicode support in LAME, work in progress
 * - map UTF-16 to UTF-8
 * - advantage, the rest can be kept unchanged (mostly)
 * - make sure, fprintf on console is in correct code page
 *   + normal text in source code is in ASCII anyway
 *   + ID3 tags and filenames coming from command line need attention
 * - call wfopen with UTF-16 names where needed
 *
 * why not wchar_t all the way?
 * well, that seems to be a big mess and not portable at all
 */
#ifndef NDEBUG
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>  
#include <crtdbg.h>
#endif
#include <wchar.h>
#include <mbstring.h>

static wchar_t *mbsToUnicode(const char *mbstr, int code_page)
{
  int n = MultiByteToWideChar(code_page, 0, mbstr, -1, NULL, 0);
  wchar_t* wstr = malloc( n*sizeof(wstr[0]) );
  if ( wstr !=0 ) {
    n = MultiByteToWideChar(code_page, 0, mbstr, -1, wstr, n);
    if ( n==0 ) {
      free( wstr );
      wstr = 0;
    }
  }
  return wstr;
}

static char *unicodeToMbs(const wchar_t *wstr, int code_page)
{
  int n = 1+WideCharToMultiByte(code_page, 0, wstr, -1, 0, 0, 0, 0);
  char* mbstr = malloc( n*sizeof(mbstr[0]) );
  if ( mbstr !=0 ) {
    n = WideCharToMultiByte(code_page, 0, wstr, -1, mbstr, n, 0, 0);
    if( n == 0 ){
      free( mbstr );
      mbstr = 0;
    }
  }
  return mbstr;
}

char* mbsToMbs(const char* str, int cp_from, int cp_to)
{
  wchar_t* wstr = mbsToUnicode(str, cp_from);
  if ( wstr != 0 ) {
    char* local8bit = unicodeToMbs(wstr, cp_to);
    free( wstr );
    return local8bit;
  }
  return 0;
}

enum { cp_utf8, cp_console, cp_actual };

wchar_t *utf8ToUnicode(const char *mbstr)
{
  return mbsToUnicode(mbstr, CP_UTF8);
}

char *unicodeToUtf8(const wchar_t *wstr)
{
  return unicodeToMbs(wstr, CP_UTF8);
}

char* utf8ToLocal8Bit(const char* str)
{
  return mbsToMbs(str, CP_UTF8, CP_ACP);
}

char* utf8ToConsole8Bit(const char* str)
{
  return mbsToMbs(str, CP_UTF8, GetConsoleOutputCP());
}

char* local8BitToUtf8(const char* str)
{
  return mbsToMbs(str, CP_ACP, CP_UTF8);
}

char* console8BitToUtf8(const char* str)
{
  return mbsToMbs(str, GetConsoleOutputCP(), CP_UTF8);
}
 
char* utf8ToLatin1(char const* str)
{
  return mbsToMbs(str, CP_UTF8, 28591); /* Latin-1 is code page 28591 */
}

unsigned short* utf8ToUtf16(char const* mbstr) /* additional Byte-Order-Marker */
{
  int n = MultiByteToWideChar(CP_UTF8, 0, mbstr, -1, NULL, 0);
  wchar_t* wstr = malloc( (n+1)*sizeof(wstr[0]) );
  if ( wstr !=0 ) {
    wstr[0] = 0xfeff; /* BOM */
    n = MultiByteToWideChar(CP_UTF8, 0, mbstr, -1, wstr+1, n);
    if ( n==0 ) {
      free( wstr );
      wstr = 0;
    }
  }
  return wstr;
}

static
void setDebugMode()
{
#ifndef NDEBUG
    if ( IsDebuggerPresent() ) {
        // Get current flag  
        int tmpFlag = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG );
        //tmpFlag |= _CRTDBG_DELAY_FREE_MEM_DF;  
        tmpFlag |= _CRTDBG_ALLOC_MEM_DF|_CRTDBG_LEAK_CHECK_DF;
        // Set flag to the new value.  
        _CrtSetDbgFlag( tmpFlag );
        _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
        _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
    }
#endif
}

int wmain(int argc, wchar_t* argv[])
{
  char **utf8_argv;
  int i, ret;
  setDebugMode();
  utf8_argv = calloc(argc, sizeof(char*));
  for (i = 0; i < argc; ++i) {
    utf8_argv[i] = unicodeToUtf8(argv[i]);
  }
  ret = c_main(argc, utf8_argv);
  for (i = 0; i < argc; ++i) {
    free( utf8_argv[i] );
  }
  free( utf8_argv );
  return ret;
}

FILE* lame_fopen(char const* file, char const* mode)
{
    FILE* fh = 0;
    wchar_t* wfile = utf8ToUnicode(file);
    wchar_t* wmode = utf8ToUnicode(mode);
    if (wfile != 0 && wmode != 0) {
        fh = _wfopen(wfile, wmode);
    }
    else {
        fh = fopen(file, mode);
    }
    free(wfile);
    free(wmode);
    return fh;
}

char* lame_getenv(char const* var)
{
    char* str = 0;
    wchar_t* wvar = utf8ToUnicode(var);
    if (wvar != 0) {
        wchar_t* wstr = _wgetenv(wvar);
        if (wstr != 0) {
            str = unicodeToUtf8(wstr);
        }
    }
    free(wvar);
    return str;
}

#else

FILE* lame_fopen(char const* file, char const* mode)
{
    return fopen(file, mode);
}

char* lame_getenv(char const* var)
{
    char* str = getenv(var);
    if (str) {
        return strdup(str);
    }
    return 0;
}

int main(int argc, char *argv[])
{
    return c_main(argc, argv);
}

#endif




static int
c_main(int argc, char *argv[])
{
    lame_t  gf;
    int     ret;

#if macintosh
    argc = ccommand(&argv);
#endif
#ifdef __EMX__
    /* This gives wildcard expansion on Non-POSIX shells with OS/2 */
    _wildcard(&argc, &argv);
#endif
#if defined( _WIN32 ) && !defined(__MINGW32__)
    set_process_affinity();
#endif

    frontend_open_console();    
    gf = lame_init(); /* initialize libmp3lame */
    if (NULL == gf) {
        error_printf("fatal error during initialization\n");
        ret = 1;
    }
    else {
        ret = lame_main(gf, argc, argv);
        lame_close(gf);
    }
    frontend_close_console();
    return ret;
}
