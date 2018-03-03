#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

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

#if defined(HAVE_NCURSES_TERMCAP_H)
# include <ncurses/termcap.h>
#elif defined(HAVE_TERMCAP_H)
# include <termcap.h>
#elif defined(HAVE_TERMCAP)
# include <curses.h>
# if !defined(__bsdi__)
#  include <term.h>
# endif
#endif

#include <stdio.h>
#include <stdarg.h>
#include "console.h"
#include "main.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

#define CLASS_ID           0x434F4E53
#define REPORT_BUFF_SIZE   1024

#if defined(_WIN32)  &&  !defined(__CYGWIN__)
# include <windows.h>
#endif



static int
my_console_printing(FILE * fp, const char *format, va_list ap)
{
    if (fp != NULL)
        return vfprintf(fp, format, ap);
    return 0;
}

static int
my_error_printing(FILE * fp, const char *format, va_list ap)
{
    if (fp != NULL)
        return vfprintf(fp, format, ap);
    return 0;
}

static int
my_report_printing(FILE * fp, const char *format, va_list ap)
{
    if (fp != NULL)
        return vfprintf(fp, format, ap);
    return 0;
}


/*
 * Taken from Termcap_Manual.html:
 *
 * With the Unix version of termcap, you must allocate space for the description yourself and pass
 * the address of the space as the argument buffer. There is no way you can tell how much space is
 * needed, so the convention is to allocate a buffer 2048 characters long and assume that is
 * enough.  (Formerly the convention was to allocate 1024 characters and assume that was enough.
 * But one day, for one kind of terminal, that was not enough.)
 */

#ifdef HAVE_TERMCAP

static void
get_termcap_string(char const* id, char* dest, size_t n)
{
    char    tc[16];
    char   *tp = tc;
    tp[0] = '\0';
    tp = tgetstr(id, &tp);
    if (tp != NULL && dest != NULL && n > 0) {
        strncpy(dest, tp, n);
        dest[n-1] = '\0';
    }
}

static void 
get_termcap_number(char const* id, int* dest, int low, int high)
{
    int const val = tgetnum(id);
    if (low <= val && val <= high) {
        *dest = val;
    }
}

static void
apply_termcap_settings(Console_IO_t * const mfp)
{
    /* try to catch additional information about special console sequences */
    char const* term_name = getenv("TERM");
    if (NULL != term_name) {
        char    term_buff[4096];
        int const ret = tgetent(term_buff, term_name);
        if (1 == ret) {
            get_termcap_number("co", &mfp->disp_width, 40, 512);
            get_termcap_number("li", &mfp->disp_height, 16, 256);
            get_termcap_string("up", mfp->str_up, sizeof(mfp->str_up));
            get_termcap_string("md", mfp->str_emph, sizeof(mfp->str_emph));
            get_termcap_string("me", mfp->str_norm, sizeof(mfp->str_norm));
            get_termcap_string("ce", mfp->str_clreoln, sizeof(mfp->str_clreoln));
        }
    }
}
#endif /* TERMCAP_AVAILABLE */

static int
init_console(Console_IO_t * const mfp)
{
    /* setup basics of brhist I/O channels */
    mfp->disp_width = 80;
    mfp->disp_height = 25;
    mfp->Console_fp = stderr;
    mfp->Error_fp = stderr;
    mfp->Report_fp = NULL;

    /*mfp -> Console_buff = calloc ( 1, REPORT_BUFF_SIZE ); */
    setvbuf(mfp->Console_fp, mfp->Console_buff, _IOFBF, sizeof(mfp->Console_buff));
/*  setvbuf ( mfp -> Error_fp  , NULL                   , _IONBF, 0                                ); */

#if defined(_WIN32)  &&  !defined(__CYGWIN__)
    mfp->Console_Handle = GetStdHandle(STD_ERROR_HANDLE);
#endif

    strcpy(mfp->str_up, "\033[A");

#ifdef HAVE_TERMCAP
    apply_termcap_settings(mfp);
#endif /* TERMCAP_AVAILABLE */

    mfp->ClassID = CLASS_ID;

#if defined(_WIN32)  &&  !defined(__CYGWIN__)
    mfp->Console_file_type = GetFileType(Console_IO.Console_Handle);
#else
    mfp->Console_file_type = 0;
#endif
    return 0;
}

static void
deinit_console(Console_IO_t * const mfp)
{
    if (mfp->Report_fp != NULL) {
        fclose(mfp->Report_fp);
        mfp->Report_fp = NULL;
    }
    fflush(mfp->Console_fp);
    setvbuf(mfp->Console_fp, NULL, _IONBF, (size_t) 0);

    memset(mfp->Console_buff, 0x55, REPORT_BUFF_SIZE);
}


/*  LAME console
 */
Console_IO_t Console_IO;

int
frontend_open_console(void)
{
    return init_console(&Console_IO);
}

void
frontend_close_console(void)
{
    deinit_console(&Console_IO);
}

void
frontend_debugf(const char *format, va_list ap)
{
    (void) my_report_printing(Console_IO.Report_fp, format, ap);
}

void
frontend_msgf(const char *format, va_list ap)
{
    (void) my_console_printing(Console_IO.Console_fp, format, ap);
}

void
frontend_errorf(const char *format, va_list ap)
{
    (void) my_error_printing(Console_IO.Error_fp, format, ap);
}

void
frontend_print_null(const char *format, va_list ap)
{
    (void) format;
    (void) ap;
}

int
console_printf(const char *format, ...)
{
    va_list args;
    int     ret;

    va_start(args, format);
    ret = my_console_printing(Console_IO.Console_fp, format, args);
    va_end(args);

    return ret;
}

int
error_printf(const char *format, ...)
{
    va_list args;
    int     ret;

    va_start(args, format);
    ret = my_console_printing(Console_IO.Error_fp, format, args);
    va_end(args);

    return ret;
}

int
report_printf(const char *format, ...)
{
    va_list args;
    int     ret;

    va_start(args, format);
    ret = my_console_printing(Console_IO.Report_fp, format, args);
    va_end(args);

    return ret;
}

void
console_flush()
{
    fflush(Console_IO.Console_fp);
}

void
error_flush()
{
    fflush(Console_IO.Error_fp);
}

void
report_flush()
{
    fflush(Console_IO.Report_fp);
}

void
console_up(int n_lines)
{
#if defined(_WIN32)  &&  !defined(__CYGWIN__)
    if (Console_IO.Console_file_type != FILE_TYPE_PIPE) {
        COORD   Pos;
        CONSOLE_SCREEN_BUFFER_INFO CSBI;

        console_flush();
        GetConsoleScreenBufferInfo(Console_IO.Console_Handle, &CSBI);
        Pos.Y = (SHORT)(CSBI.dwCursorPosition.Y - n_lines);
        Pos.X = 0;
        SetConsoleCursorPosition(Console_IO.Console_Handle, Pos);
    }
#else
    while (n_lines-- > 0)
        fputs(Console_IO.str_up, Console_IO.Console_fp);
    console_flush();
#endif
}


void
set_debug_file(const char *fn)
{
    if (Console_IO.Report_fp == NULL) {
        Console_IO.Report_fp = lame_fopen(fn, "a");
        if (Console_IO.Report_fp != NULL) {
            error_printf("writing debug info into: %s\n", fn);
        }
        else {
            error_printf("Error: can't open for debug info: %s\n", fn);
        }
    }
}

/* end of console.c */
