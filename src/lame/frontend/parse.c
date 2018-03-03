/*
 *      Command line parsing related functions
 *
 *      Copyright (c) 1999 Mark Taylor
 *                    2000-2017 Robert Hegemann
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

/* $Id: parse.c,v 1.307 2017/09/26 12:25:07 robert Exp $ */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <ctype.h>
#include <math.h>
 
#ifdef STDC_HEADERS
# include <stdio.h>
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


#ifdef HAVE_LIMITS_H
# include <limits.h>
#endif

#include "lame.h"

#include "parse.h"
#include "main.h"
#include "get_audio.h"
#include "version.h"
#include "console.h"

#undef dimension_of
#define dimension_of(array) (sizeof(array)/sizeof(array[0]))

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

                 
#ifdef HAVE_ICONV
#include <iconv.h>
#include <errno.h>
#include <locale.h>
#include <langinfo.h>
#endif

#if defined _ALLOW_INTERNAL_OPTIONS
#define INTERNAL_OPTS 1
#else
#define INTERNAL_OPTS 0
#endif

#if (INTERNAL_OPTS!=0)
#include "set_get.h"
#define DEV_HELP(a) a
#else
#define DEV_HELP(a)
#define lame_set_tune(a,b) (void)0
#define lame_set_short_threshold(a,b,c) (void)0
#define lame_set_maskingadjust(a,b) (void)0
#define lame_set_maskingadjust_short(a,b) (void)0
#define lame_set_ATHcurve(a,b) (void)0
#define lame_set_preset_notune(a,b) (void)0
#define lame_set_substep(a,b) (void)0
#define lame_set_subblock_gain(a,b) (void)0
#define lame_set_sfscale(a,b) (void)0
#endif

static int const lame_alpha_version_enabled = LAME_ALPHA_VERSION;
static int const internal_opts_enabled = INTERNAL_OPTS;

/* GLOBAL VARIABLES.  set by parse_args() */
/* we need to clean this up */

ReaderConfig global_reader = { sf_unknown, 0, 0, 0, 0 };
WriterConfig global_writer = { 0 };

UiConfig global_ui_config = {0,0,0,0};

DecoderConfig global_decoder;

RawPCMConfig global_raw_pcm = 
{ /* in_bitwidth */ 16
, /* in_signed   */ -1
, /* in_endian   */ ByteOrderLittleEndian
};



/* possible text encodings */
typedef enum TextEncoding
{ TENC_RAW     /* bytes will be stored as-is into ID3 tags, which are Latin1 per definition */
, TENC_LATIN1  /* text will be converted from local encoding to Latin1, as ID3 needs it */
, TENC_UTF16   /* text will be converted from local encoding to Unicode, as ID3v2 wants it */
} TextEncoding;

#ifdef HAVE_ICONV
#define ID3TAGS_EXTENDED
/* search for Zero termination in multi-byte strings */
static size_t
strlenMultiByte(char const* str, size_t w)
{    
    size_t n = 0;
    if (str != 0) {
        size_t i, x = 0;
        for (n = 0; ; ++n) {
            x = 0;
            for (i = 0; i < w; ++i) {
                x += *str++ == 0 ? 1 : 0;
            }
            if (x == w) {
                break;
            }
        }
    }
    return n;
}


static size_t
currCharCodeSize(void)
{
    size_t n = 1;
    char dst[32];
    char* src = "A";
    char* cur_code = nl_langinfo(CODESET);
    iconv_t xiconv = iconv_open(cur_code, "ISO_8859-1");
    if (xiconv != (iconv_t)-1) {
        for (n = 0; n < 32; ++n) {
            char* i_ptr = src;
            char* o_ptr = dst;
            size_t srcln = 1;
            size_t avail = n;
            size_t rc = iconv(xiconv, &i_ptr, &srcln, &o_ptr, &avail);
            if (rc != (size_t)-1) {
                break;
            }
        }
        iconv_close(xiconv);
    }
    return n;
}

#if 0
static
char* fromLatin1( char* src )
{
    char* dst = 0;
    if (src != 0) {
        size_t const l = strlen(src);
        size_t const n = l*4;
        dst = calloc(n+4, 4);
        if (dst != 0) {
            char* cur_code = nl_langinfo(CODESET);
            iconv_t xiconv = iconv_open(cur_code, "ISO_8859-1");
            if (xiconv != (iconv_t)-1) {
                char* i_ptr = src;
                char* o_ptr = dst;
                size_t srcln = l;
                size_t avail = n;                
                iconv(xiconv, &i_ptr, &srcln, &o_ptr, &avail);
                iconv_close(xiconv);
            }
        }
    }
    return dst;
}

static
char* fromUtf16( char* src )
{
    char* dst = 0;
    if (src != 0) {
        size_t const l = strlenMultiByte(src, 2);
        size_t const n = l*4;
        dst = calloc(n+4, 4);
        if (dst != 0) {
            char* cur_code = nl_langinfo(CODESET);
            iconv_t xiconv = iconv_open(cur_code, "UTF-16LE");
            if (xiconv != (iconv_t)-1) {
                char* i_ptr = (char*)src;
                char* o_ptr = dst;
                size_t srcln = l*2;
                size_t avail = n;                
                iconv(xiconv, &i_ptr, &srcln, &o_ptr, &avail);
                iconv_close(xiconv);
            }
        }
    }
    return dst;
}
#endif

static
char* toLatin1( char* src )
{
    size_t w = currCharCodeSize();
    char* dst = 0;
    if (src != 0) {
        size_t const l = strlenMultiByte(src, w);
        size_t const n = l*4;
        dst = calloc(n+4, 4);
        if (dst != 0) {
            char* cur_code = nl_langinfo(CODESET);
            iconv_t xiconv = iconv_open("ISO_8859-1//TRANSLIT", cur_code);
            if (xiconv != (iconv_t)-1) {
                char* i_ptr = (char*)src;
                char* o_ptr = dst;
                size_t srcln = l*w;
                size_t avail = n;
                iconv(xiconv, &i_ptr, &srcln, &o_ptr, &avail);
                iconv_close(xiconv);
            }
        }
    }
    return dst;
}


static
char* toUtf16( char* src )
{
    size_t w = currCharCodeSize();
    char* dst = 0;
    if (src != 0) {
        size_t const l = strlenMultiByte(src, w);
        size_t const n = (l+1)*4;
        dst = calloc(n+4, 4);
        if (dst != 0) {
            char* cur_code = nl_langinfo(CODESET);
            iconv_t xiconv = iconv_open("UTF-16LE//TRANSLIT", cur_code);
            dst[0] = 0xff;
            dst[1] = 0xfe;
            if (xiconv != (iconv_t)-1) {
                char* i_ptr = (char*)src;
                char* o_ptr = &dst[2];
                size_t srcln = l*w;
                size_t avail = n;
                iconv(xiconv, &i_ptr, &srcln, &o_ptr, &avail);
                iconv_close(xiconv);
            }
        }
    }
    return dst;
}
#endif

#if defined( _WIN32 ) && !defined(__MINGW32__)
#define ID3TAGS_EXTENDED

char* toLatin1(char const* s)
{
    return utf8ToLatin1(s);
}

unsigned short* toUtf16(char const* s)
{
    return utf8ToUtf16(s);
}
#endif

static int evaluateArgument(char const* token, char const* arg, char* _EndPtr)
{
    if (arg != 0 && arg != _EndPtr)
        return 1;
    error_printf("WARNING: argument missing for '%s'\n", token);
    return 0;
}

static int getDoubleValue(char const* token, char const* arg, double* ptr)
{
    char *_EndPtr=0;
    double d = strtod(arg, &_EndPtr);
    if (ptr != 0) {
        *ptr = d;
    }
    return evaluateArgument(token, arg, _EndPtr);
}

static int getIntValue(char const* token, char const* arg, int* ptr)
{
    char *_EndPtr=0;
    long d = strtol(arg, &_EndPtr, 10);
    if (ptr != 0) {
        *ptr = d;
    }
    return evaluateArgument(token, arg, _EndPtr);
}

#ifdef ID3TAGS_EXTENDED
static int
set_id3v2tag(lame_global_flags* gfp, int type, unsigned short const* str)
{
    switch (type)
    {
        case 'a': return id3tag_set_textinfo_utf16(gfp, "TPE1", str);
        case 't': return id3tag_set_textinfo_utf16(gfp, "TIT2", str);
        case 'l': return id3tag_set_textinfo_utf16(gfp, "TALB", str);
        case 'g': return id3tag_set_textinfo_utf16(gfp, "TCON", str);
        case 'c': return id3tag_set_comment_utf16(gfp, 0, 0, str);
        case 'n': return id3tag_set_textinfo_utf16(gfp, "TRCK", str);
        case 'y': return id3tag_set_textinfo_utf16(gfp, "TYER", str);
        case 'v': return id3tag_set_fieldvalue_utf16(gfp, str);
    }
    return 0;
}
#endif

static int
set_id3tag(lame_global_flags* gfp, int type, char const* str)
{
    switch (type)
    {
        case 'a': return id3tag_set_artist(gfp, str), 0;
        case 't': return id3tag_set_title(gfp, str), 0;
        case 'l': return id3tag_set_album(gfp, str), 0;
        case 'g': return id3tag_set_genre(gfp, str);
        case 'c': return id3tag_set_comment(gfp, str), 0;
        case 'n': return id3tag_set_track(gfp, str);
        case 'y': return id3tag_set_year(gfp, str), 0;
        case 'v': return id3tag_set_fieldvalue(gfp, str);
    }
    return 0;
}

static int
id3_tag(lame_global_flags* gfp, int type, TextEncoding enc, char* str)
{
    void* x = 0;
    int result;
    if (enc == TENC_UTF16 && type != 'v' ) {
        id3_tag(gfp, type, TENC_LATIN1, str); /* for id3v1 */
    }
    switch (enc) 
    {
        default:
#ifdef ID3TAGS_EXTENDED
        case TENC_LATIN1: x = toLatin1(str); break;
        case TENC_UTF16:  x = toUtf16(str);   break;
#else
        case TENC_RAW:    x = strdup(str);   break;
#endif
    }
    switch (enc)
    {
        default:
#ifdef ID3TAGS_EXTENDED
        case TENC_LATIN1: result = set_id3tag(gfp, type, x);   break;
        case TENC_UTF16:  result = set_id3v2tag(gfp, type, x); break;
#else
        case TENC_RAW:    result = set_id3tag(gfp, type, x);   break;
#endif
    }
    free(x);
    return result;
}




/************************************************************************
*
* license
*
* PURPOSE:  Writes version and license to the file specified by fp
*
************************************************************************/

static int
lame_version_print(FILE * const fp)
{
    const char *b = get_lame_os_bitness();
    const char *v = get_lame_version();
    const char *u = get_lame_url();
    const size_t lenb = strlen(b);
    const size_t lenv = strlen(v);
    const size_t lenu = strlen(u);
    const size_t lw = 80;       /* line width of terminal in characters */
    const size_t sw = 16;       /* static width of text */

    if (lw >= lenb + lenv + lenu + sw || lw < lenu + 2)
        /* text fits in 80 chars per line, or line even too small for url */
        if (lenb > 0)
            fprintf(fp, "LAME %s version %s (%s)\n\n", b, v, u);
        else
            fprintf(fp, "LAME version %s (%s)\n\n", v, u);
    else {
        int const n_white_spaces = (int)((lenu+2) > lw ? 0 : lw-2-lenu);
        /* text too long, wrap url into next line, right aligned */
        if (lenb > 0)
            fprintf(fp, "LAME %s version %s\n%*s(%s)\n\n", b, v, n_white_spaces, "", u);
        else
            fprintf(fp, "LAME version %s\n%*s(%s)\n\n", v, n_white_spaces, "", u);
    }
    if (lame_alpha_version_enabled)
        fprintf(fp, "warning: alpha versions should be used for testing only\n\n");


    return 0;
}

static int
print_license(FILE * const fp)
{                       /* print version & license */
    lame_version_print(fp);
    fprintf(fp,
            "Copyright (c) 1999-2011 by The LAME Project\n"
            "Copyright (c) 1999,2000,2001 by Mark Taylor\n"
            "Copyright (c) 1998 by Michael Cheng\n"
            "Copyright (c) 1995,1996,1997 by Michael Hipp: mpglib\n" "\n");
    fprintf(fp,
            "This library is free software; you can redistribute it and/or\n"
            "modify it under the terms of the GNU Library General Public\n"
            "License as published by the Free Software Foundation; either\n"
            "version 2 of the License, or (at your option) any later version.\n"
            "\n");
    fprintf(fp,
            "This library is distributed in the hope that it will be useful,\n"
            "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
            "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU\n"
            "Library General Public License for more details.\n"
            "\n");
    fprintf(fp,
            "You should have received a copy of the GNU Library General Public\n"
            "License along with this program. If not, see\n"
            "<http://www.gnu.org/licenses/>.\n");
    return 0;
}


/************************************************************************
*
* usage
*
* PURPOSE:  Writes command line syntax to the file specified by fp
*
************************************************************************/

int
usage(FILE * const fp, const char *ProgramName)
{                       /* print general syntax */
    lame_version_print(fp);
    fprintf(fp,
            "usage: %s [options] <infile> [outfile]\n"
            "\n"
            "    <infile> and/or <outfile> can be \"-\", which means stdin/stdout.\n"
            "\n"
            "Try:\n"
            "     \"%s --help\"           for general usage information\n"
            " or:\n"
            "     \"%s --preset help\"    for information on suggested predefined settings\n"
            " or:\n"
            "     \"%s --longhelp\"\n"
            "  or \"%s -?\"              for a complete options list\n\n",
            ProgramName, ProgramName, ProgramName, ProgramName, ProgramName);
    return 0;
}


/************************************************************************
*
* usage
*
* PURPOSE:  Writes command line syntax to the file specified by fp
*           but only the most important ones, to fit on a vt100 terminal
*
************************************************************************/

int
short_help(const lame_global_flags * gfp, FILE * const fp, const char *ProgramName)
{                       /* print short syntax help */
    lame_version_print(fp);
    fprintf(fp,
            "usage: %s [options] <infile> [outfile]\n"
            "\n"
            "    <infile> and/or <outfile> can be \"-\", which means stdin/stdout.\n"
            "\n" "RECOMMENDED:\n" "    lame -V2 input.wav output.mp3\n" "\n", ProgramName);
    fprintf(fp,
            "OPTIONS:\n"
            "    -b bitrate      set the bitrate, default 128 kbps\n"
            "    -h              higher quality, but a little slower.\n"
            "    -f              fast mode (lower quality)\n"
            "    -V n            quality setting for VBR.  default n=%i\n"
            "                    0=high quality,bigger files. 9.999=smaller files\n",
            lame_get_VBR_q(gfp));
    fprintf(fp,
            "    --preset type   type must be \"medium\", \"standard\", \"extreme\", \"insane\",\n"
            "                    or a value for an average desired bitrate and depending\n"
            "                    on the value specified, appropriate quality settings will\n"
            "                    be used.\n"
            "                    \"--preset help\" gives more info on these\n" "\n");
    fprintf(fp,
#if defined(WIN32)
            "    --priority type  sets the process priority\n"
            "                     0,1 = Low priority\n"
            "                     2   = normal priority\n"
            "                     3,4 = High priority\n" "\n"
#endif
#if defined(__OS2__)
            "    --priority type  sets the process priority\n"
            "                     0 = Low priority\n"
            "                     1 = Medium priority\n"
            "                     2 = Regular priority\n"
            "                     3 = High priority\n"
            "                     4 = Maximum priority\n" "\n"
#endif
            "    --help id3      ID3 tagging related options\n" "\n"
            DEV_HELP(
            "    --help dev      developer options\n" "\n"
            )
            "    --longhelp      full list of options\n" "\n"
            "    --license       print License information\n\n"
            );

    return 0;
}

/************************************************************************
*
* usage
*
* PURPOSE:  Writes command line syntax to the file specified by fp
*
************************************************************************/

static void
wait_for(FILE * const fp, int lessmode)
{
    if (lessmode) {
        fflush(fp);
        getchar();
    }
    else {
        fprintf(fp, "\n");
    }
    fprintf(fp, "\n");
}

static void
help_id3tag(FILE * const fp)
{
    fprintf(fp,
            "  ID3 tag options:\n"
            "    --tt <title>    audio/song title (max 30 chars for version 1 tag)\n"
            "    --ta <artist>   audio/song artist (max 30 chars for version 1 tag)\n"
            "    --tl <album>    audio/song album (max 30 chars for version 1 tag)\n"
            "    --ty <year>     audio/song year of issue (1 to 9999)\n"
            "    --tc <comment>  user-defined text (max 30 chars for v1 tag, 28 for v1.1)\n");
    fprintf(fp,
            "    --tn <track[/total]>   audio/song track number and (optionally) the total\n"
            "                           number of tracks on the original recording. (track\n"
            "                           and total each 1 to 255. just the track number\n"
            "                           creates v1.1 tag, providing a total forces v2.0).\n");
    fprintf(fp,
            "    --tg <genre>    audio/song genre (name or number in list)\n"
            "    --ti <file>     audio/song albumArt (jpeg/png/gif file, v2.3 tag)\n"
            "    --tv <id=value> user-defined frame specified by id and value (v2.3 tag)\n"
            "                    syntax: --tv \"TXXX=description=content\"\n"
            );
    fprintf(fp,
            "    --add-id3v2     force addition of version 2 tag\n"
            "    --id3v1-only    add only a version 1 tag\n"
            "    --id3v2-only    add only a version 2 tag\n"
#ifdef ID3TAGS_EXTENDED
            "    --id3v2-utf16   add following options in unicode text encoding\n"
            "    --id3v2-latin1  add following options in latin-1 text encoding\n"
#endif
            "    --space-id3v1   pad version 1 tag with spaces instead of nulls\n"
            "    --pad-id3v2     same as '--pad-id3v2-size 128'\n"
            "    --pad-id3v2-size <value> adds version 2 tag, pad with extra <value> bytes\n"
            "    --genre-list    print alphabetically sorted ID3 genre list and exit\n"
            "    --ignore-tag-errors  ignore errors in values passed for tags\n" "\n"
            );
    fprintf(fp,
            "    Note: A version 2 tag will NOT be added unless one of the input fields\n"
            "    won't fit in a version 1 tag (e.g. the title string is longer than 30\n"
            "    characters), or the '--add-id3v2' or '--id3v2-only' options are used,\n"
            "    or output is redirected to stdout.\n"
            );
}

static void
help_developer_switches(FILE * const fp)
{
    if ( !internal_opts_enabled ) {
    fprintf(fp,
            "    Note: Almost all of the following switches aren't available in this build!\n\n"
            );
    }
    fprintf(fp,
            "  ATH related:\n"
            "    --noath         turns ATH down to a flat noise floor\n"
            "    --athshort      ignore GPSYCHO for short blocks, use ATH only\n"
            "    --athonly       ignore GPSYCHO completely, use ATH only\n"
            "    --athtype n     selects between different ATH types [0-4]\n"
            "    --athlower x    lowers ATH by x dB\n"
            );
    fprintf(fp,
            "    --athaa-type n  ATH auto adjust: 0 'no' else 'loudness based'\n"
/** OBSOLETE "    --athaa-loudapprox n   n=1 total energy or n=2 equal loudness curve\n"*/
            "    --athaa-sensitivity x  activation offset in -/+ dB for ATH auto-adjustment\n"
            "\n");
    fprintf(fp,
            "  PSY related:\n"
            "    --short         use short blocks when appropriate\n"
            "    --noshort       do not use short blocks\n"
            "    --allshort      use only short blocks\n"
            );
    fprintf(fp,
            "(1) --temporal-masking x   x=0 disables, x=1 enables temporal masking effect\n"
            "    --nssafejoint   M/S switching criterion\n"
            "    --nsmsfix <arg> M/S switching tuning [effective 0-3.5]\n"
            "(2) --interch x     adjust inter-channel masking ratio\n"
            "    --ns-bass x     adjust masking for sfbs  0 -  6 (long)  0 -  5 (short)\n"
            "    --ns-alto x     adjust masking for sfbs  7 - 13 (long)  6 - 10 (short)\n"
            "    --ns-treble x   adjust masking for sfbs 14 - 21 (long) 11 - 12 (short)\n"
            );
    fprintf(fp,
            "    --ns-sfb21 x    change ns-treble by x dB for sfb21\n"
            "    --shortthreshold x,y  short block switching threshold,\n"
            "                          x for L/R/M channel, y for S channel\n"
            "    -Z [n]          always do calculate short block maskings\n");
    fprintf(fp,
            "  Noise Shaping related:\n"
            "(1) --substep n     use pseudo substep noise shaping method types 0-2\n"
            "(1) -X n[,m]        selects between different noise measurements\n"
            "                    n for long block, m for short. if m is omitted, m = n\n"
            " 1: CBR, ABR and VBR-old encoding modes only\n"
            " 2: ignored\n"
           );
}

int
long_help(const lame_global_flags * gfp, FILE * const fp, const char *ProgramName, int lessmode)
{                       /* print long syntax help */
    lame_version_print(fp);
    fprintf(fp,
            "usage: %s [options] <infile> [outfile]\n"
            "\n"
            "    <infile> and/or <outfile> can be \"-\", which means stdin/stdout.\n"
            "\n" "RECOMMENDED:\n" "    lame -V2 input.wav output.mp3\n" "\n", ProgramName);
    fprintf(fp,
            "OPTIONS:\n"
            "  Input options:\n"
            "    --scale <arg>   scale input (multiply PCM data) by <arg>\n"
            "    --scale-l <arg> scale channel 0 (left) input (multiply PCM data) by <arg>\n"
            "    --scale-r <arg> scale channel 1 (right) input (multiply PCM data) by <arg>\n"
            "    --swap-channel  swap L/R channels\n"
            "    --ignorelength  ignore file length in WAV header\n"
            "    --gain <arg>    apply Gain adjustment in decibels, range -20.0 to +12.0\n"
           );
#if (defined HAVE_MPGLIB || defined AMIGA_MPEGA)
    fprintf(fp,
            "    --mp1input      input file is a MPEG Layer I   file\n"
            "    --mp2input      input file is a MPEG Layer II  file\n"
            "    --mp3input      input file is a MPEG Layer III file\n"
           );
#endif
    fprintf(fp,
            "    --nogap <file1> <file2> <...>\n"
            "                    gapless encoding for a set of contiguous files\n"
            "    --nogapout <dir>\n"
            "                    output dir for gapless encoding (must precede --nogap)\n"
            "    --nogaptags     allow the use of VBR tags in gapless encoding\n"
            "    --out-dir <dir> output dir, must exist\n"
           );
    fprintf(fp,
            "\n"
            "  Input options for RAW PCM:\n"
            "    -r              input is raw pcm\n"
            "    -s sfreq        sampling frequency of input file (kHz) - default 44.1 kHz\n"
            "    --signed        input is signed (default)\n"
            "    --unsigned      input is unsigned\n"
            "    --bitwidth w    input bit width is w (default 16)\n"
            "    -x              force byte-swapping of input\n"
            "    --little-endian input is little-endian (default)\n"
            "    --big-endian    input is big-endian\n"
            "    -a              downmix from stereo to mono file for mono encoding\n"
           );

    wait_for(fp, lessmode);
    fprintf(fp,
            "  Operational options:\n"
            "    -m <mode>       (j)oint, (s)imple, (f)orce, (d)ual-mono, (m)ono (l)eft (r)ight\n"
            "                    default is (j)\n"
            "                    joint  = Uses the best possible of MS and LR stereo\n"
            "                    simple = force LR stereo on all frames\n"
            "                    force  = force MS stereo on all frames.\n"
	   );
    fprintf(fp,
            "    --preset type   type must be \"medium\", \"standard\", \"extreme\", \"insane\",\n"
            "                    or a value for an average desired bitrate and depending\n"
            "                    on the value specified, appropriate quality settings will\n"
            "                    be used.\n"
            "                    \"--preset help\" gives more info on these\n"
            "    --comp  <arg>   choose bitrate to achieve a compression ratio of <arg>\n");
    fprintf(fp, "    --replaygain-fast   compute RG fast but slightly inaccurately (default)\n"
#ifdef DECODE_ON_THE_FLY
            "    --replaygain-accurate   compute RG more accurately and find the peak sample\n"
#endif
            "    --noreplaygain  disable ReplayGain analysis\n"
#ifdef DECODE_ON_THE_FLY
            "    --clipdetect    enable --replaygain-accurate and print a message whether\n"
            "                    clipping occurs and how far the waveform is from full scale\n"
#endif
        );
    fprintf(fp,
            "    --flush         flush output stream as soon as possible\n"
            "    --freeformat    produce a free format bitstream\n"
            "    --decode        input=mp3 file, output=wav\n"
            "    -t              disable writing wav header when using --decode\n");

    wait_for(fp, lessmode);
    fprintf(fp,
            "  Verbosity:\n"
            "    --disptime <arg>print progress report every arg seconds\n"
            "    -S              don't print progress report, VBR histograms\n"
            "    --nohist        disable VBR histogram display\n"
            "    --quiet         don't print anything on screen\n"
            "    --silent        don't print anything on screen, but fatal errors\n"
            "    --brief         print more useful information\n"
            "    --verbose       print a lot of useful information\n" "\n");
    fprintf(fp,
            "  Noise shaping & psycho acoustic algorithms:\n"
            "    -q <arg>        <arg> = 0...9.  Default  -q 3 \n"
            "                    -q 0:  Highest quality, very slow \n"
            "                    -q 9:  Poor quality, but fast \n"
            "    -h              Same as -q 2.   \n"
            "    -f              Same as -q 7.   Fast, ok quality\n");

    wait_for(fp, lessmode);
    fprintf(fp,
            "  CBR (constant bitrate, the default) options:\n"
            "    -b <bitrate>    set the bitrate in kbps, default 128 kbps\n"
            "    --cbr           enforce use of constant bitrate\n"
            "\n"
            "  ABR options:\n"
            "    --abr <bitrate> specify average bitrate desired (instead of quality)\n" "\n");
    fprintf(fp,
            "  VBR options:\n"
            "    -V n            quality setting for VBR.  default n=%i\n"
            "                    0=high quality,bigger files. 9=smaller files\n"
            "    -v              the same as -V 4\n"
            "    --vbr-old       use old variable bitrate (VBR) routine\n"
            "    --vbr-new       use new variable bitrate (VBR) routine (default)\n"
            "    -Y              lets LAME ignore noise in sfb21, like in CBR\n"
			"                    (Default for V3 to V9.999)\n"
            ,
            lame_get_VBR_q(gfp));
    fprintf(fp,
            "    -b <bitrate>    specify minimum allowed bitrate, default  32 kbps\n"
            "    -B <bitrate>    specify maximum allowed bitrate, default 320 kbps\n"
            "    -F              strictly enforce the -b option, for use with players that\n"
            "                    do not support low bitrate mp3\n"
            "    -t              disable writing LAME Tag\n"
            "    -T              enable and force writing LAME Tag\n");

    wait_for(fp, lessmode);
    DEV_HELP(
        help_developer_switches(fp);
        wait_for(fp, lessmode);
            )

    fprintf(fp,
            "  MP3 header/stream options:\n"
            "    -e <emp>        de-emphasis n/5/c  (obsolete)\n"
            "    -c              mark as copyright\n"
            "    -o              mark as non-original\n"
            "    -p              error protection.  adds 16 bit checksum to every frame\n"
            "                    (the checksum is computed correctly)\n"
            "    --nores         disable the bit reservoir\n"
            "    --strictly-enforce-ISO   comply as much as possible to ISO MPEG spec\n");
    fprintf(fp,
            "    --buffer-constraint <constraint> available values for constraint:\n"
            "                                     default, strict, maximum\n"
            "\n"
            );
    fprintf(fp,
            "  Filter options:\n"
            "  --lowpass <freq>        frequency(kHz), lowpass filter cutoff above freq\n"
            "  --lowpass-width <freq>  frequency(kHz) - default 15%% of lowpass freq\n"
            "  --highpass <freq>       frequency(kHz), highpass filter cutoff below freq\n"
            "  --highpass-width <freq> frequency(kHz) - default 15%% of highpass freq\n");
    fprintf(fp,
            "  --resample <sfreq>  sampling frequency of output file(kHz)- default=automatic\n");

    wait_for(fp, lessmode);
    help_id3tag(fp);
    fprintf(fp,
#if defined(WIN32)
            "\n\nMS-Windows-specific options:\n"
            "    --priority <type>  sets the process priority:\n"
            "                         0,1 = Low priority (IDLE_PRIORITY_CLASS)\n"
            "                         2 = normal priority (NORMAL_PRIORITY_CLASS, default)\n"
            "                         3,4 = High priority (HIGH_PRIORITY_CLASS))\n"
            "    Note: Calling '--priority' without a parameter will select priority 0.\n"
#endif
#if defined(__OS2__)
            "\n\nOS/2-specific options:\n"
            "    --priority <type>  sets the process priority:\n"
            "                         0 = Low priority (IDLE, delta = 0)\n"
            "                         1 = Medium priority (IDLE, delta = +31)\n"
            "                         2 = Regular priority (REGULAR, delta = -31)\n"
            "                         3 = High priority (REGULAR, delta = 0)\n"
            "                         4 = Maximum priority (REGULAR, delta = +31)\n"
            "    Note: Calling '--priority' without a parameter will select priority 0.\n"
#endif
            "\nMisc:\n    --license       print License information\n\n"
        );

#if defined(HAVE_NASM)
    wait_for(fp, lessmode);
    fprintf(fp,
            "  Platform specific:\n"
            "    --noasm <instructions> disable assembly optimizations for mmx/3dnow/sse\n");
    wait_for(fp, lessmode);
#endif

    display_bitrates(fp);

    return 0;
}

static void
display_bitrate(FILE * const fp, const char *const version, const int d, const int indx)
{
    int     i;
    int nBitrates = 14;
    if (d == 4)
        nBitrates = 8;


    fprintf(fp,
            "\nMPEG-%-3s layer III sample frequencies (kHz):  %2d  %2d  %g\n"
            "bitrates (kbps):", version, 32 / d, 48 / d, 44.1 / d);
    for (i = 1; i <= nBitrates; i++)
        fprintf(fp, " %2i", lame_get_bitrate(indx, i));
    fprintf(fp, "\n");
}

int
display_bitrates(FILE * const fp)
{
    display_bitrate(fp, "1", 1, 1);
    display_bitrate(fp, "2", 2, 0);
    display_bitrate(fp, "2.5", 4, 0);
    fprintf(fp, "\n");
    fflush(fp);
    return 0;
}


/*  note: for presets it would be better to externalize them in a file.
    suggestion:  lame --preset <file-name> ...
            or:  lame --preset my-setting  ... and my-setting is defined in lame.ini
 */

/*
Note from GB on 08/25/2002:
I am merging --presets and --alt-presets. Old presets are now aliases for
corresponding abr values from old alt-presets. This way we now have a
unified preset system, and I hope than more people will use the new tuned
presets instead of the old unmaintained ones.
*/



/************************************************************************
*
* usage
*
* PURPOSE:  Writes presetting info to #stdout#
*
************************************************************************/


static void
presets_longinfo_dm(FILE * msgfp)
{
    fprintf(msgfp,
            "\n"
            "The --preset switches are aliases over LAME settings.\n"
            "\n" "\n");
    fprintf(msgfp,
            "To activate these presets:\n"
            "\n" "   For VBR modes (generally highest quality):\n" "\n");
    fprintf(msgfp,
            "     --preset medium      This preset should provide near transparency to most\n"
            "                          people on most music.\n"
            "\n"
            "     --preset standard    This preset should generally be transparent to most\n"
            "                          people on most music and is already quite high\n"
            "                          in quality.\n" "\n");
    fprintf(msgfp,
            "     --preset extreme     If you have extremely good hearing and similar\n"
            "                          equipment, this preset will generally provide\n"
            "                          slightly higher quality than the \"standard\" mode.\n" "\n");
    fprintf(msgfp,
            "   For CBR 320kbps (highest quality possible from the --preset switches):\n"
            "\n"
            "     --preset insane      This preset will usually be overkill for most people\n"
            "                          and most situations, but if you must have the\n"
            "                          absolute highest quality with no regard to filesize,\n"
            "                          this is the way to go.\n" "\n");
    fprintf(msgfp,
            "   For ABR modes (high quality per given bitrate but not as high as VBR):\n"
            "\n"
            "     --preset <kbps>      Using this preset will usually give you good quality\n"
            "                          at a specified bitrate. Depending on the bitrate\n"
            "                          entered, this preset will determine the optimal\n"
            "                          settings for that particular situation. For example:\n"
            "                          \"--preset 185\" activates this preset and uses 185\n"
            "                          as an average kbps.\n" "\n");
    fprintf(msgfp,
            "   \"cbr\"  - If you use the ABR mode (read above) with a significant\n"
            "            bitrate such as 80, 96, 112, 128, 160, 192, 224, 256, 320,\n"
            "            you can use the \"cbr\" option to force CBR mode encoding\n"
            "            instead of the standard abr mode. ABR does provide higher\n"
            "            quality but CBR may be useful in situations such as when\n"
            "            streaming an mp3 over the internet may be important.\n" "\n");
    fprintf(msgfp,
            "    For example:\n"
            "\n"
            "    --preset standard <input file> <output file>\n"
            " or --preset cbr 192 <input file> <output file>\n"
            " or --preset 172 <input file> <output file>\n"
            " or --preset extreme <input file> <output file>\n" "\n" "\n");
    fprintf(msgfp,
            "A few aliases are also available for ABR mode:\n"
            "phone => 16kbps/mono        phon+/lw/mw-eu/sw => 24kbps/mono\n"
            "mw-us => 40kbps/mono        voice => 56kbps/mono\n"
            "fm/radio/tape => 112kbps    hifi => 160kbps\n"
            "cd => 192kbps               studio => 256kbps\n");
}


static int
presets_set(lame_t gfp, int fast, int cbr, const char *preset_name, const char *ProgramName)
{
    int     mono = 0;

    if ((strcmp(preset_name, "help") == 0) && (fast < 1)
        && (cbr < 1)) {
        lame_version_print(stdout);
        presets_longinfo_dm(stdout);
        return -1;
    }

    /*aliases for compatibility with old presets */

    if (strcmp(preset_name, "phone") == 0) {
        preset_name = "16";
        mono = 1;
    }
    if ((strcmp(preset_name, "phon+") == 0) ||
        (strcmp(preset_name, "lw") == 0) ||
        (strcmp(preset_name, "mw-eu") == 0) || (strcmp(preset_name, "sw") == 0)) {
        preset_name = "24";
        mono = 1;
    }
    if (strcmp(preset_name, "mw-us") == 0) {
        preset_name = "40";
        mono = 1;
    }
    if (strcmp(preset_name, "voice") == 0) {
        preset_name = "56";
        mono = 1;
    }
    if (strcmp(preset_name, "fm") == 0) {
        preset_name = "112";
    }
    if ((strcmp(preset_name, "radio") == 0) || (strcmp(preset_name, "tape") == 0)) {
        preset_name = "112";
    }
    if (strcmp(preset_name, "hifi") == 0) {
        preset_name = "160";
    }
    if (strcmp(preset_name, "cd") == 0) {
        preset_name = "192";
    }
    if (strcmp(preset_name, "studio") == 0) {
        preset_name = "256";
    }

    if (strcmp(preset_name, "medium") == 0) {
        lame_set_VBR_q(gfp, 4);
        lame_set_VBR(gfp, vbr_default);
        return 0;
    }

    if (strcmp(preset_name, "standard") == 0) {
        lame_set_VBR_q(gfp, 2);
        lame_set_VBR(gfp, vbr_default);
        return 0;
    }

    else if (strcmp(preset_name, "extreme") == 0) {
        lame_set_VBR_q(gfp, 0);
        lame_set_VBR(gfp, vbr_default);
        return 0;
    }

    else if ((strcmp(preset_name, "insane") == 0) && (fast < 1)) {

        lame_set_preset(gfp, INSANE);

        return 0;
    }

    /* Generic ABR Preset */
    if (((atoi(preset_name)) > 0) && (fast < 1)) {
        if ((atoi(preset_name)) >= 8 && (atoi(preset_name)) <= 320) {
            lame_set_preset(gfp, atoi(preset_name));

            if (cbr == 1)
                lame_set_VBR(gfp, vbr_off);

            if (mono == 1) {
                lame_set_mode(gfp, MONO);
            }

            return 0;

        }
        else {
            lame_version_print(Console_IO.Error_fp);
            error_printf("Error: The bitrate specified is out of the valid range for this preset\n"
                         "\n"
                         "When using this mode you must enter a value between \"32\" and \"320\"\n"
                         "\n" "For further information try: \"%s --preset help\"\n", ProgramName);
            return -1;
        }
    }

    lame_version_print(Console_IO.Error_fp);
    error_printf("Error: You did not enter a valid profile and/or options with --preset\n"
                 "\n"
                 "Available profiles are:\n"
                 "\n"
                 "                 medium\n"
                 "                 standard\n"
                 "                 extreme\n"
                 "                 insane\n"
                 "          <cbr> (ABR Mode) - The ABR Mode is implied. To use it,\n"
                 "                             simply specify a bitrate. For example:\n"
                 "                             \"--preset 185\" activates this\n"
                 "                             preset and uses 185 as an average kbps.\n" "\n");
    error_printf("    Some examples:\n"
                 "\n"
                 " or \"%s --preset standard <input file> <output file>\"\n"
                 " or \"%s --preset cbr 192 <input file> <output file>\"\n"
                 " or \"%s --preset 172 <input file> <output file>\"\n"
                 " or \"%s --preset extreme <input file> <output file>\"\n"
                 "\n"
                 "For further information try: \"%s --preset help\"\n", ProgramName, ProgramName,
                 ProgramName, ProgramName, ProgramName);
    return -1;
}

static void
genre_list_handler(int num, const char *name, void *cookie)
{
    (void) cookie;
    console_printf("%3d %s\n", num, name);
}


/************************************************************************
*
* parse_args
*
* PURPOSE:  Sets encoding parameters to the specifications of the
* command line.  Default settings are used for parameters
* not specified in the command line.
*
* If the input file is in WAVE or AIFF format, the sampling frequency is read
* from the AIFF header.
*
* The input and output filenames are read into #inpath# and #outpath#.
*
************************************************************************/

/* would use real "strcasecmp" but it isn't portable */
static int
local_strcasecmp(const char *s1, const char *s2)
{
    unsigned char c1;
    unsigned char c2;

    do {
        c1 = (unsigned char) tolower(*s1);
        c2 = (unsigned char) tolower(*s2);
        if (!c1) {
            break;
        }
        ++s1;
        ++s2;
    } while (c1 == c2);
    return c1 - c2;
}

static int
local_strncasecmp(const char *s1, const char *s2, int n)
{
    unsigned char c1 = 0;
    unsigned char c2 = 0;
    int     cnt = 0;

    do {
        if (cnt == n) {
            break;
        }
        c1 = (unsigned char) tolower(*s1);
        c2 = (unsigned char) tolower(*s2);
        if (!c1) {
            break;
        }
        ++s1;
        ++s2;
        ++cnt;
    } while (c1 == c2);
    return c1 - c2;
}



/* LAME is a simple frontend which just uses the file extension */
/* to determine the file type.  Trying to analyze the file */
/* contents is well beyond the scope of LAME and should not be added. */
static int
filename_to_type(const char *FileName)
{
    size_t  len = strlen(FileName);

    if (len < 4)
        return sf_unknown;

    FileName += len - 4;
    if (0 == local_strcasecmp(FileName, ".mpg"))
        return sf_mp123;
    if (0 == local_strcasecmp(FileName, ".mp1"))
        return sf_mp123;
    if (0 == local_strcasecmp(FileName, ".mp2"))
        return sf_mp123;
    if (0 == local_strcasecmp(FileName, ".mp3"))
        return sf_mp123;
    if (0 == local_strcasecmp(FileName, ".wav"))
        return sf_wave;
    if (0 == local_strcasecmp(FileName, ".aif"))
        return sf_aiff;
    if (0 == local_strcasecmp(FileName, ".raw"))
        return sf_raw;
    if (0 == local_strcasecmp(FileName, ".ogg"))
        return sf_ogg;
    return sf_unknown;
}

static int
resample_rate(double freq)
{
    if (freq >= 1.e3)
        freq *= 1.e-3;

    switch ((int) freq) {
    case 8:
        return 8000;
    case 11:
        return 11025;
    case 12:
        return 12000;
    case 16:
        return 16000;
    case 22:
        return 22050;
    case 24:
        return 24000;
    case 32:
        return 32000;
    case 44:
        return 44100;
    case 48:
        return 48000;
    default:
        error_printf("Illegal resample frequency: %.3f kHz\n", freq);
        return 0;
    }
}

#ifdef _WIN32
#define SLASH '\\'
#define COLON ':'
#elif __OS2__
#define SLASH '\\'
#else
#define SLASH '/'
#endif

static
size_t scanPath(char const* s, char const** a, char const** b)
{
    char const* s1 = s;
    char const* s2 = s;
    if (s != 0) {
        for (; *s; ++s) {
            switch (*s) {
            case SLASH:
#ifdef _WIN32
            case COLON:
#endif
                s2 = s;
                break;
            }
        }
#ifdef _WIN32
        if (*s2 == COLON) {
            ++s2;
        }
#endif
    }
    if (a) {
        *a = s1;
    }
    if (b) {
        *b = s2;
    }
    return s2-s1;
}

static
size_t scanBasename(char const* s, char const** a, char const** b)
{
    char const* s1 = s;
    char const* s2 = s;
    if (s != 0) {
        for (; *s; ++s) {
            switch (*s) {
            case SLASH:
#ifdef _WIN32
            case COLON:
#endif
                s1 = s2 = s;
                break;
            case '.':
                s2 = s;
                break;
            }
        }
        if (s2 == s1) {
            s2 = s;
        }
        if (*s1 == SLASH 
#ifdef _WIN32
          || *s1 == COLON
#endif
           ) {
            ++s1;
        }
    }
    if (a != 0) {
        *a = s1;
    }
    if (b != 0) {
        *b = s2;
    }
    return s2-s1;
}

static 
int isCommonSuffix(char const* s_ext)
{
    char const* suffixes[] = 
    { ".WAV", ".RAW", ".MP1", ".MP2"
    , ".MP3", ".MPG", ".MPA", ".CDA"
    , ".OGG", ".AIF", ".AIFF", ".AU"
    , ".SND", ".FLAC", ".WV", ".OFR"
    , ".TAK", ".MP4", ".M4A", ".PCM"
    , ".W64"
    };
    size_t i;
    for (i = 0; i < dimension_of(suffixes); ++i) {
        if (local_strcasecmp(s_ext, suffixes[i]) == 0) {
            return 1;
        }
    }
    return 0;
}


int generateOutPath(char const* inPath, char const* outDir, char const* s_ext, char* outPath)
{
    size_t const max_path = PATH_MAX;
#if 1
    size_t i = 0;
    int out_dir_used = 0;

    if (outDir != 0 && outDir[0] != 0) {
        out_dir_used = 1;
        while (*outDir) {
            outPath[i++] = *outDir++;
            if (i >= max_path) {
                goto err_generateOutPath;
            }
        }
        if (i > 0 && outPath[i-1] != SLASH) {
            outPath[i++] = SLASH;
            if (i >= max_path) {
                goto err_generateOutPath;
            }
        }
        outPath[i] = 0;
    }
    else {
        char const* pa;
        char const* pb;
        size_t j, n = scanPath(inPath, &pa, &pb);
        if (i+n >= max_path) {
            goto err_generateOutPath;
        }
        for (j = 0; j < n; ++j) {
            outPath[i++] = pa[j];
        }
        if (n > 0) {
            outPath[i++] = SLASH;
            if (i >= max_path) {
                goto err_generateOutPath;
            }
        }
        outPath[i] = 0;
    }
    {
        int replace_suffix = 0;
        char const* na;
        char const* nb;
        size_t j, n = scanBasename(inPath, &na, &nb);
        if (i+n >= max_path) {
            goto err_generateOutPath;
        }
        for (j = 0; j < n; ++j) {
            outPath[i++] = na[j];
        }
        outPath[i] = 0;
        if (isCommonSuffix(nb) == 1) {
            replace_suffix = 1;
            if (out_dir_used == 0) {
                if (local_strcasecmp(nb, s_ext) == 0) {
                    replace_suffix = 0;
                }
            }
        }
        if (replace_suffix == 0) {
            while (*nb) {
                outPath[i++] = *nb++;
                if (i >= max_path) {
                    goto err_generateOutPath;
                }
            }
            outPath[i] = 0;
        }
    }
    if (i+5 >= max_path) {
        goto err_generateOutPath;
    }
    while (*s_ext) {
        outPath[i++] = *s_ext++;
    }
    outPath[i] = 0;
    return 0;
err_generateOutPath:
    error_printf( "error: output file name too long\n" );
    return 1;
#else
    strncpy(outPath, inPath, PATH_MAX + 1 - 4);
    strncat(outPath, s_ext, 4);
    return 0;
#endif
}


static int
set_id3_albumart(lame_t gfp, char const* file_name)
{
    int ret = -1;
    FILE *fpi = 0;
    char *albumart = 0;

    if (file_name == 0) {
        return 0;
    }
    fpi = lame_fopen(file_name, "rb");
    if (!fpi) {
        ret = 1;
    }
    else {
        size_t size;

        fseek(fpi, 0, SEEK_END);
        size = ftell(fpi);
        fseek(fpi, 0, SEEK_SET);
        albumart = (char *)malloc(size);
        if (!albumart) {
            ret = 2;            
        }
        else {
            if (fread(albumart, 1, size, fpi) != size) {
                ret = 3;
            }
            else {
                ret = id3tag_set_albumart(gfp, albumart, size) ? 4 : 0;
            }
            free(albumart);
        }
        fclose(fpi);
    }
    switch (ret) {
    case 1: error_printf("Could not find: '%s'.\n", file_name); break;
    case 2: error_printf("Insufficient memory for reading the albumart.\n"); break;
    case 3: error_printf("Read error: '%s'.\n", file_name); break;
    case 4: error_printf("Unsupported image: '%s'.\nSpecify JPEG/PNG/GIF image\n", file_name); break;
    default: break;
    }
    return ret;
}


enum ID3TAG_MODE 
{ ID3TAG_MODE_DEFAULT
, ID3TAG_MODE_V1_ONLY
, ID3TAG_MODE_V2_ONLY
};

static int dev_only_with_arg(char const* str, char const* token, char const* nextArg, int* argIgnored, int* argUsed)
{
    if (0 != local_strcasecmp(token,str)) return 0;
    *argUsed = 1;
    if (internal_opts_enabled) return 1;
    *argIgnored = 1;
    error_printf("WARNING: ignoring developer-only switch --%s %s\n", token, nextArg);
    return 0;
}

static int dev_only_without_arg(char const* str, char const* token, int* argIgnored)
{
    if (0 != local_strcasecmp(token,str)) return 0;
    if (internal_opts_enabled) return 1;
    *argIgnored = 1;
    error_printf("WARNING: ignoring developer-only switch --%s\n", token);
    return 0;
}

/* Ugly, NOT final version */

#define T_IF(str)          if ( 0 == local_strcasecmp (token,str) ) {
#define T_ELIF(str)        } else if ( 0 == local_strcasecmp (token,str) ) {
#define T_ELIF2(str1,str2) } else if ( 0 == local_strcasecmp (token,str1)  ||  0 == local_strcasecmp (token,str2) ) {
#define T_ELSE             } else {
#define T_END              }

#define T_ELIF_INTERNAL(str) \
                           } else if (dev_only_without_arg(str,token,&argIgnored)) {

#define T_ELIF_INTERNAL_WITH_ARG(str) \
                           } else if (dev_only_with_arg(str,token,nextArg,&argIgnored,&argUsed)) {


static int
parse_args_(lame_global_flags * gfp, int argc, char **argv,
           char *const inPath, char *const outPath, char **nogap_inPath, int *num_nogap)
{
    char    outDir[PATH_MAX+1] = "";
    int     input_file = 0;  /* set to 1 if we parse an input file name  */
    int     i;
    int     autoconvert = 0;
    int     nogap = 0;
    int     nogap_tags = 0;  /* set to 1 to use VBR tags in NOGAP mode */
    const char *ProgramName = argv[0];
    int     count_nogap = 0;
    int     noreplaygain = 0; /* is RG explicitly disabled by the user */
    int     id3tag_mode = ID3TAG_MODE_DEFAULT;
    int     ignore_tag_errors = 0;  /* Ignore errors in values passed for tags */
#ifdef ID3TAGS_EXTENDED
    enum TextEncoding id3_tenc = TENC_UTF16;
#else
    enum TextEncoding id3_tenc = TENC_LATIN1;
#endif

#ifdef HAVE_ICONV
    setlocale(LC_CTYPE, "");
#endif
    inPath[0] = '\0';
    outPath[0] = '\0';
    /* turn on display options. user settings may turn them off below */
    global_ui_config.silent = 0; /* default */
    global_ui_config.brhist = 1;
    global_decoder.mp3_delay = 0;
    global_decoder.mp3_delay_set = 0;
    global_decoder.disable_wav_header = 0;
    global_ui_config.print_clipping_info = 0;
    id3tag_init(gfp);

    /* process args */
    for (i = 0; ++i < argc;) {
        char    c;
        char   *token;
        char   *arg;
        char   *nextArg;
        int     argUsed;
        int     argIgnored=0;

        token = argv[i];
        if (*token++ == '-') {
            argUsed = 0;
            nextArg = i + 1 < argc ? argv[i + 1] : "";

            if (!*token) { /* The user wants to use stdin and/or stdout. */
                input_file = 1;
                if (inPath[0] == '\0')
                    strncpy(inPath, argv[i], PATH_MAX + 1);
                else if (outPath[0] == '\0')
                    strncpy(outPath, argv[i], PATH_MAX + 1);
            }
            if (*token == '-') { /* GNU style */
                double  double_value = 0;
                int     int_value = 0;
                token++;

                T_IF("resample")
                    argUsed = getDoubleValue(token, nextArg, &double_value);
                    if (argUsed) 
                        (void) lame_set_out_samplerate(gfp, resample_rate(double_value));

                T_ELIF("vbr-old")
                    lame_set_VBR(gfp, vbr_rh);

                T_ELIF("vbr-new")
                    lame_set_VBR(gfp, vbr_mt);

                T_ELIF("vbr-mtrh")
                    lame_set_VBR(gfp, vbr_mtrh);

                T_ELIF("cbr")
                    lame_set_VBR(gfp, vbr_off);

                T_ELIF("abr")
                    /* values larger than 8000 are bps (like Fraunhofer), so it's strange to get 320000 bps MP3 when specifying 8000 bps MP3 */
                    argUsed = getIntValue(token, nextArg, &int_value);
                    if (argUsed) {
                        if (int_value >= 8000) {
                            int_value = (int_value + 500) / 1000;
                        }
                        if (int_value > 320) {
                            int_value = 320;
                        }
                        if (int_value < 8) {
                            int_value = 8;
                        }
                        lame_set_VBR(gfp, vbr_abr);
                        lame_set_VBR_mean_bitrate_kbps(gfp, int_value);
                    }

                T_ELIF("r3mix")
                    lame_set_preset(gfp, R3MIX);

                T_ELIF("bitwidth")
                    argUsed = getIntValue(token, nextArg, &int_value);
                    if (argUsed) 
                        global_raw_pcm.in_bitwidth = int_value;
                
                T_ELIF("signed")
                    global_raw_pcm.in_signed = 1;

                T_ELIF("unsigned")
                    global_raw_pcm.in_signed = 0;

                T_ELIF("little-endian")
                    global_raw_pcm.in_endian = ByteOrderLittleEndian;

                T_ELIF("big-endian")
                    global_raw_pcm.in_endian = ByteOrderBigEndian;

                T_ELIF("mp1input")
                    global_reader.input_format = sf_mp1;

                T_ELIF("mp2input")
                    global_reader.input_format = sf_mp2;

                T_ELIF("mp3input")
                    global_reader.input_format = sf_mp3;

                T_ELIF("ogginput")
                    error_printf("sorry, vorbis support in LAME is deprecated.\n");
                return -1;

                T_ELIF("decode")
                    (void) lame_set_decode_only(gfp, 1);

                T_ELIF("flush")
                    global_writer.flush_write = 1;

                T_ELIF("decode-mp3delay")
                    argUsed = getIntValue(token, nextArg, &int_value);
                    if (argUsed) {
                        global_decoder.mp3_delay = int_value;
                        global_decoder.mp3_delay_set = 1;
                    }

                T_ELIF("nores")
                    lame_set_disable_reservoir(gfp, 1);

                T_ELIF("strictly-enforce-ISO")
                    lame_set_strict_ISO(gfp, MDB_STRICT_ISO);

                T_ELIF("buffer-constraint")
                  argUsed = 1;
                if (strcmp(nextArg, "default") == 0)
                  (void) lame_set_strict_ISO(gfp, MDB_DEFAULT);
                else if (strcmp(nextArg, "strict") == 0)
                  (void) lame_set_strict_ISO(gfp, MDB_STRICT_ISO);
                else if (strcmp(nextArg, "maximum") == 0)
                  (void) lame_set_strict_ISO(gfp, MDB_MAXIMUM);
                else {
                    error_printf("unknown buffer constraint '%s'\n", nextArg);
                    return -1;
                }

                T_ELIF("scale")
                    argUsed = getDoubleValue(token, nextArg, &double_value);
                    if (argUsed)
                        (void) lame_set_scale(gfp, (float) double_value);

                T_ELIF("scale-l")
                    argUsed = getDoubleValue(token, nextArg, &double_value);
                    if (argUsed)
                        (void) lame_set_scale_left(gfp, (float) double_value);

                T_ELIF("scale-r")
                    argUsed = getDoubleValue(token, nextArg, &double_value);
                    if (argUsed)
                        (void) lame_set_scale_right(gfp, (float) double_value);

                T_ELIF("gain")
                    argUsed = getDoubleValue(token, nextArg, &double_value);
                    if (argUsed) {
                        double gain = double_value;
                        gain = gain > -20.f ? gain : -20.f;
                        gain = gain < 12.f ? gain : 12.f;
                        gain = pow(10.f, gain*0.05);
                        (void) lame_set_scale(gfp, (float) gain);
                    }

                T_ELIF("noasm")
                    argUsed = 1;
                if (!strcmp(nextArg, "mmx"))
                    (void) lame_set_asm_optimizations(gfp, MMX, 0);
                if (!strcmp(nextArg, "3dnow"))
                    (void) lame_set_asm_optimizations(gfp, AMD_3DNOW, 0);
                if (!strcmp(nextArg, "sse"))
                    (void) lame_set_asm_optimizations(gfp, SSE, 0);

                T_ELIF("freeformat")
                    lame_set_free_format(gfp, 1);

                T_ELIF("replaygain-fast")
                    lame_set_findReplayGain(gfp, 1);

#ifdef DECODE_ON_THE_FLY
                T_ELIF("replaygain-accurate")
                    lame_set_decode_on_the_fly(gfp, 1);
                lame_set_findReplayGain(gfp, 1);
#endif

                T_ELIF("noreplaygain")
                    noreplaygain = 1;
                lame_set_findReplayGain(gfp, 0);


#ifdef DECODE_ON_THE_FLY
                T_ELIF("clipdetect")
                    global_ui_config.print_clipping_info = 1;
                    lame_set_decode_on_the_fly(gfp, 1);
#endif

                T_ELIF("nohist")
                    global_ui_config.brhist = 0;

#if defined(__OS2__) || defined(WIN32)
                T_ELIF("priority")
                    argUsed = getIntValue(token, nextArg, &int_value);
                    if (argUsed)
                        setProcessPriority(int_value);
#endif

                /* options for ID3 tag */
#ifdef ID3TAGS_EXTENDED
                T_ELIF2("id3v2-utf16","id3v2-ucs2") /* id3v2-ucs2 for compatibility only */
                    id3_tenc = TENC_UTF16;
                    id3tag_add_v2(gfp);

                T_ELIF("id3v2-latin1")
                    id3_tenc = TENC_LATIN1;
                    id3tag_add_v2(gfp);
#endif

                T_ELIF("tt")
                    argUsed = 1;
                    id3_tag(gfp, 't', id3_tenc, nextArg);

                T_ELIF("ta")
                    argUsed = 1;
                    id3_tag(gfp, 'a', id3_tenc, nextArg);

                T_ELIF("tl")
                    argUsed = 1;
                    id3_tag(gfp, 'l', id3_tenc, nextArg);

                T_ELIF("ty")
                    argUsed = 1;
                    id3_tag(gfp, 'y', id3_tenc, nextArg);

                T_ELIF("tc")
                    argUsed = 1;
                    id3_tag(gfp, 'c', id3_tenc, nextArg);

                T_ELIF("tn")
                    int ret = id3_tag(gfp, 'n', id3_tenc, nextArg);
                    argUsed = 1;
                    if (ret != 0) {
                        if (0 == ignore_tag_errors) {
                            if (id3tag_mode == ID3TAG_MODE_V1_ONLY) {
                                if (global_ui_config.silent < 9) {
                                    error_printf("The track number has to be between 1 and 255 for ID3v1.\n");
                                }
                                return -1;
                            }
                            else if (id3tag_mode == ID3TAG_MODE_V2_ONLY) {
                                /* track will be stored as-is in ID3v2 case, so no problem here */
                            }
                            else {
                                if (global_ui_config.silent < 9) {
                                    error_printf("The track number has to be between 1 and 255 for ID3v1, ignored for ID3v1.\n");
                                }
                            }
                        }
                    }

                T_ELIF("tg")
                    int ret = 0;
                    argUsed = 1;
                    if (nextArg != 0 && strlen(nextArg) > 0) {
                        ret = id3_tag(gfp, 'g', id3_tenc, nextArg);
                    }
                    if (ret != 0) {
                        if (0 == ignore_tag_errors) {
                            if (ret == -1) {
                                error_printf("Unknown ID3v1 genre number: '%s'.\n", nextArg);
                                return -1;
                            }
                            else if (ret == -2) {
                                if (id3tag_mode == ID3TAG_MODE_V1_ONLY) {
                                    error_printf("Unknown ID3v1 genre: '%s'.\n", nextArg);
                                    return -1;
                                }
                                else if (id3tag_mode == ID3TAG_MODE_V2_ONLY) {
                                    /* genre will be stored as-is in ID3v2 case, so no problem here */
                                }
                                else {
                                    if (global_ui_config.silent < 9) {
                                        error_printf("Unknown ID3v1 genre: '%s'.  Setting ID3v1 genre to 'Other'\n", nextArg);
                                    }
                                }
                            }
                            else {
                                if (global_ui_config.silent < 10)
                                    error_printf("Internal error.\n");
                                return -1;
                            }
                        }
                    }

                T_ELIF("tv")
                    argUsed = 1;
                    if (id3_tag(gfp, 'v', id3_tenc, nextArg)) {
                        if (global_ui_config.silent < 9) {
                            error_printf("Invalid field value: '%s'. Ignored\n", nextArg);
                        }
                    }

                T_ELIF("ti")
                    argUsed = 1;
                    if (set_id3_albumart(gfp, nextArg) != 0) {
                        if (! ignore_tag_errors) {
                            return -1;
                        }
                    }

                T_ELIF("ignore-tag-errors")
                    ignore_tag_errors = 1;

                T_ELIF("add-id3v2")
                    id3tag_add_v2(gfp);

                T_ELIF("id3v1-only")
                    id3tag_v1_only(gfp);
                    id3tag_mode = ID3TAG_MODE_V1_ONLY;

                T_ELIF("id3v2-only")
                    id3tag_v2_only(gfp);
                    id3tag_mode = ID3TAG_MODE_V2_ONLY;

                T_ELIF("space-id3v1")
                    id3tag_space_v1(gfp);

                T_ELIF("pad-id3v2")
                    id3tag_pad_v2(gfp);

                T_ELIF("pad-id3v2-size")
                    argUsed = getIntValue(token, nextArg, &int_value);
                    if (argUsed) {
                        int_value = int_value <= 128000 ? int_value : 128000;
                        int_value = int_value >= 0      ? int_value : 0;
                        id3tag_set_pad(gfp, int_value);
                    }

                T_ELIF("genre-list")
                    id3tag_genre_list(genre_list_handler, NULL);
                    return -2;


                T_ELIF("lowpass")
                    argUsed = getDoubleValue(token, nextArg, &double_value);
                    if (argUsed) {
                        if (double_value < 0) {
                            lame_set_lowpassfreq(gfp, -1);
                        }
                        else {
                            /* useful are 0.001 kHz...50 kHz, 50 Hz...50000 Hz */
                            if (double_value < 0.001 || double_value > 50000.) {
                                error_printf("Must specify lowpass with --lowpass freq, freq >= 0.001 kHz\n");
                                return -1;
                            }
                            lame_set_lowpassfreq(gfp, (int) (double_value * (double_value < 50. ? 1.e3 : 1.e0) + 0.5));
                        }
                    }
                
                T_ELIF("lowpass-width")
                    argUsed = getDoubleValue(token, nextArg, &double_value);
                    if (argUsed) {
                        /* useful are 0.001 kHz...16 kHz, 16 Hz...50000 Hz */
                        if (double_value < 0.001 || double_value > 50000.) {
                            error_printf
                                ("Must specify lowpass width with --lowpass-width freq, freq >= 0.001 kHz\n");
                            return -1;
                        }
                        lame_set_lowpasswidth(gfp, (int) (double_value * (double_value < 16. ? 1.e3 : 1.e0) + 0.5));
                    }

                T_ELIF("highpass")
                    argUsed = getDoubleValue(token, nextArg, &double_value);
                    if (argUsed) {
                        if (double_value < 0.0) {
                            lame_set_highpassfreq(gfp, -1);
                        }
                        else {
                            /* useful are 0.001 kHz...16 kHz, 16 Hz...50000 Hz */
                            if (double_value < 0.001 || double_value > 50000.) {
                                error_printf("Must specify highpass with --highpass freq, freq >= 0.001 kHz\n");
                                return -1;
                            }
                            lame_set_highpassfreq(gfp, (int) (double_value * (double_value < 16. ? 1.e3 : 1.e0) + 0.5));
                        }
                    }
                    
                T_ELIF("highpass-width")
                    argUsed = getDoubleValue(token, nextArg, &double_value);
                    if (argUsed) {
                        /* useful are 0.001 kHz...16 kHz, 16 Hz...50000 Hz */
                        if (double_value < 0.001 || double_value > 50000.) {
                            error_printf
                                ("Must specify highpass width with --highpass-width freq, freq >= 0.001 kHz\n");
                            return -1;
                        }
                        lame_set_highpasswidth(gfp, (int) double_value);
                    }

                T_ELIF("comp")
                    argUsed = getDoubleValue(token, nextArg, &double_value);
                    if (argUsed) {
                        if (double_value < 1.0) {
                            error_printf("Must specify compression ratio >= 1.0\n");
                            return -1;
                        }
                        else {
                            lame_set_compression_ratio(gfp, (float) double_value);
                        }
                    }
    
                /* some more GNU-ish options could be added
                 * brief         => few messages on screen (name, status report)
                 * o/output file => specifies output filename
                 * O             => stdout
                 * i/input file  => specifies input filename
                 * I             => stdin
                 */
                T_ELIF("quiet")
                    global_ui_config.silent = 10; /* on a scale from 1 to 10 be very silent */

                T_ELIF("silent")
                    global_ui_config.silent = 9;

                T_ELIF("brief")
                    global_ui_config.silent = -5; /* print few info on screen */

                T_ELIF("verbose")
                    global_ui_config.silent = -10; /* print a lot on screen */
                
                T_ELIF2("version", "license")
                    print_license(stdout);
                return -2;

                T_ELIF2("help", "usage")
                    if (0 == local_strncasecmp(nextArg, "id3", 3)) {
                        help_id3tag(stdout);
                    }
                    else if (0 == local_strncasecmp(nextArg, "dev", 3)) {
                        help_developer_switches(stdout);
                    }
                    else {
                        short_help(gfp, stdout, ProgramName);
                    }
                return -2;

                T_ELIF("longhelp")
                    long_help(gfp, stdout, ProgramName, 0 /* lessmode=NO */ );
                return -2;

                T_ELIF("?")
#ifdef __unix__
                    FILE   *fp = popen("less -Mqc", "w");
                    long_help(gfp, fp, ProgramName, 0 /* lessmode=NO */ );
                    pclose(fp);
#else
                    long_help(gfp, stdout, ProgramName, 1 /* lessmode=YES */ );
#endif
                return -2;

                T_ELIF2("preset", "alt-preset")
                    argUsed = 1;
                {
                    int     fast = 0, cbr = 0;

                    while ((strcmp(nextArg, "fast") == 0) || (strcmp(nextArg, "cbr") == 0)) {

                        if ((strcmp(nextArg, "fast") == 0) && (fast < 1))
                            fast = 1;
                        if ((strcmp(nextArg, "cbr") == 0) && (cbr < 1))
                            cbr = 1;

                        argUsed++;
                        nextArg = i + argUsed < argc ? argv[i + argUsed] : "";
                    }

                    if (presets_set(gfp, fast, cbr, nextArg, ProgramName) < 0)
                        return -1;
                }

                T_ELIF("disptime")
                    argUsed = getDoubleValue(token, nextArg, &double_value);
                    if (argUsed)
                        global_ui_config.update_interval = (float) double_value;

                T_ELIF("nogaptags")
                    nogap_tags = 1;

                T_ELIF("nogapout")
                    int const arg_n = strnlen(nextArg, PATH_MAX);
                    if (arg_n >= PATH_MAX) {
                        error_printf("%s: %s argument length (%d) exceeds limit (%d)\n", ProgramName, token, arg_n, PATH_MAX);
                        return -1;
                    }
                    strncpy(outPath, nextArg, PATH_MAX);
                    outPath[PATH_MAX] = '\0';
                    argUsed = 1;

                T_ELIF("out-dir")
                    int const arg_n = strnlen(nextArg, PATH_MAX);
                    if (arg_n >= PATH_MAX) {
                        error_printf("%s: %s argument length (%d) exceeds limit (%d)\n", ProgramName, token, arg_n, PATH_MAX);
                        return -1;
                    }
                    strncpy(outDir, nextArg, PATH_MAX);
                    outDir[PATH_MAX] = '\0';
                    argUsed = 1;

                T_ELIF("nogap")
                    nogap = 1;

                T_ELIF("swap-channel")
                    global_reader.swap_channel = 1;

                T_ELIF("ignorelength")
                    global_reader.ignorewavlength = 1;

                T_ELIF ("athaa-sensitivity")
                    argUsed = getDoubleValue(token, nextArg, &double_value);
                    if (argUsed)
                        lame_set_athaa_sensitivity(gfp, (float) double_value);

                /* ---------------- lots of dead switches ---------------- */

                T_ELIF_INTERNAL("noshort")
                    (void) lame_set_no_short_blocks(gfp, 1);

                T_ELIF_INTERNAL("short")
                    (void) lame_set_no_short_blocks(gfp, 0);

                T_ELIF_INTERNAL("allshort")
                    (void) lame_set_force_short_blocks(gfp, 1);

                T_ELIF_INTERNAL("notemp")
                    (void) lame_set_useTemporal(gfp, 0);

                T_ELIF_INTERNAL_WITH_ARG("interch")
                    argUsed = getDoubleValue(token, nextArg, &double_value);
                    if (argUsed)
                        (void) lame_set_interChRatio(gfp, (float) double_value);

                T_ELIF_INTERNAL_WITH_ARG("temporal-masking")
                    argUsed = getIntValue(token, nextArg, &int_value);
                    if (argUsed) 
                        (void) lame_set_useTemporal(gfp, int_value ? 1 : 0);

                T_ELIF_INTERNAL("nspsytune")
                    ;

                T_ELIF_INTERNAL("nssafejoint")
                    lame_set_exp_nspsytune(gfp, lame_get_exp_nspsytune(gfp) | 2);

                T_ELIF_INTERNAL_WITH_ARG("nsmsfix")
                    argUsed = getDoubleValue(token, nextArg, &double_value);
                    if (argUsed)
                        (void) lame_set_msfix(gfp, double_value);

                T_ELIF_INTERNAL_WITH_ARG("ns-bass")
                    argUsed = getDoubleValue(token, nextArg, &double_value);
                    if (argUsed) {
                        int     k = (int) (double_value * 4);
                        if (k < -32)
                            k = -32;
                        if (k > 31)
                            k = 31;
                        if (k < 0)
                            k += 64;
                        lame_set_exp_nspsytune(gfp, lame_get_exp_nspsytune(gfp) | (k << 2));
                    }

                T_ELIF_INTERNAL_WITH_ARG("ns-alto")
                    argUsed = getDoubleValue(token, nextArg, &double_value);
                    if (argUsed) {
                        int     k = (int) (double_value * 4);
                        if (k < -32)
                            k = -32;
                        if (k > 31)
                            k = 31;
                        if (k < 0)
                            k += 64;
                        lame_set_exp_nspsytune(gfp, lame_get_exp_nspsytune(gfp) | (k << 8));
                    }

                T_ELIF_INTERNAL_WITH_ARG("ns-treble")
                    argUsed = getDoubleValue(token, nextArg, &double_value);
                    if (argUsed) {
                        int     k = (int) (double_value * 4);
                        if (k < -32)
                            k = -32;
                        if (k > 31)
                            k = 31;
                        if (k < 0)
                            k += 64;
                        lame_set_exp_nspsytune(gfp, lame_get_exp_nspsytune(gfp) | (k << 14));
                    }

                T_ELIF_INTERNAL_WITH_ARG("ns-sfb21")
                    /*  to be compatible with Naoki's original code,
                     *  ns-sfb21 specifies how to change ns-treble for sfb21 */
                    argUsed = getDoubleValue(token, nextArg, &double_value);
                    if (argUsed) {
                        int     k = (int) (double_value * 4);
                        if (k < -32)
                            k = -32;
                        if (k > 31)
                            k = 31;
                        if (k < 0)
                            k += 64;
                        lame_set_exp_nspsytune(gfp, lame_get_exp_nspsytune(gfp) | (k << 20));
                    }

                T_ELIF_INTERNAL_WITH_ARG("tune") /*without helptext */
                    argUsed = getDoubleValue(token, nextArg, &double_value);
                    if (argUsed)
                        lame_set_tune(gfp, (float) double_value);

                T_ELIF_INTERNAL_WITH_ARG("shortthreshold")
                {
                    float   x, y;
                    int     n = sscanf(nextArg, "%f,%f", &x, &y);
                    if (n == 1) {
                        y = x;
                    }
                    (void) lame_set_short_threshold(gfp, x, y);
                }
                T_ELIF_INTERNAL_WITH_ARG("maskingadjust") /*without helptext */
                    argUsed = getDoubleValue(token, nextArg, &double_value);
                    if (argUsed)
                        (void) lame_set_maskingadjust(gfp, (float) double_value);

                T_ELIF_INTERNAL_WITH_ARG("maskingadjustshort") /*without helptext */
                    argUsed = getDoubleValue(token, nextArg, &double_value);
                    if (argUsed)
                        (void) lame_set_maskingadjust_short(gfp, (float) double_value);

                T_ELIF_INTERNAL_WITH_ARG("athcurve") /*without helptext */
                    argUsed = getDoubleValue(token, nextArg, &double_value);
                    if (argUsed)
                        (void) lame_set_ATHcurve(gfp, (float) double_value);

                T_ELIF_INTERNAL("no-preset-tune") /*without helptext */
                    (void) lame_set_preset_notune(gfp, 0);

                T_ELIF_INTERNAL_WITH_ARG("substep")
                    argUsed = getIntValue(token, nextArg, &int_value);
                    if (argUsed)
                        (void) lame_set_substep(gfp, int_value);

                T_ELIF_INTERNAL_WITH_ARG("sbgain") /*without helptext */
                    argUsed = getIntValue(token, nextArg, &int_value);
                    if (argUsed) 
                        (void) lame_set_subblock_gain(gfp, int_value);

                T_ELIF_INTERNAL("sfscale") /*without helptext */
                    (void) lame_set_sfscale(gfp, 1);

                T_ELIF_INTERNAL("noath")
                    (void) lame_set_noATH(gfp, 1);

                T_ELIF_INTERNAL("athonly")
                    (void) lame_set_ATHonly(gfp, 1);

                T_ELIF_INTERNAL("athshort")
                    (void) lame_set_ATHshort(gfp, 1);

                T_ELIF_INTERNAL_WITH_ARG("athlower")
                    argUsed = getDoubleValue(token, nextArg, &double_value);
                    if (argUsed)
                        (void) lame_set_ATHlower(gfp, (float) double_value);

                T_ELIF_INTERNAL_WITH_ARG("athtype")
                    argUsed = getIntValue(token, nextArg, &int_value);
                    if (argUsed)
                        (void) lame_set_ATHtype(gfp, int_value);

                T_ELIF_INTERNAL_WITH_ARG("athaa-type") /*  switch for developing, no DOCU */
                    /* once was 1:Gaby, 2:Robert, 3:Jon, else:off */
                    argUsed = getIntValue(token, nextArg, &int_value);
                    if (argUsed)
                        (void) lame_set_athaa_type(gfp, int_value); /* now: 0:off else:Jon */

                T_ELIF_INTERNAL_WITH_ARG("debug-file") /* switch for developing, no DOCU */
                    /* file name to print debug info into */
                    set_debug_file(nextArg);

                T_ELSE {
                    if (!argIgnored) {
                        error_printf("%s: unrecognized option --%s\n", ProgramName, token);
                        return -1;
                    }
                    argIgnored = 0;
                }
                T_END   i += argUsed;

            }
            else {
                while ((c = *token++) != '\0') {
                    double double_value = 0;
                    int int_value = 0;
                    arg = *token ? token : nextArg;
                    switch (c) {
                    case 'm':
                        argUsed = 1;

                        switch (*arg) {
                        case 's':
                            (void) lame_set_mode(gfp, STEREO);
                            break;
                        case 'd':
                            (void) lame_set_mode(gfp, DUAL_CHANNEL);
                            break;
                        case 'f':
                            lame_set_force_ms(gfp, 1);
                            (void) lame_set_mode(gfp, JOINT_STEREO);
                            break;
                        case 'j':
                            lame_set_force_ms(gfp, 0);
                            (void) lame_set_mode(gfp, JOINT_STEREO);
                            break;
                        case 'm':
                            (void) lame_set_mode(gfp, MONO);
                            break;
                        case 'l':
                            (void) lame_set_mode(gfp, MONO);
                            (void) lame_set_scale_left(gfp, 2);
                            (void) lame_set_scale_right(gfp, 0);
                            break;
                        case 'r':
                            (void) lame_set_mode(gfp, MONO);
                            (void) lame_set_scale_left(gfp, 0);
                            (void) lame_set_scale_right(gfp, 2);
                            break;
                        case 'a': /* same as 'j' ??? */
                            lame_set_force_ms(gfp, 0);
                            (void) lame_set_mode(gfp, JOINT_STEREO);
                            break;
                        default:
                            error_printf("%s: -m mode must be s/d/f/j/m/l/r not %s\n", ProgramName,
                                         arg);
                            return -1;
                        }
                        break;

                    case 'V':
                        argUsed = getDoubleValue("V", arg, &double_value);
                        if (argUsed) {
                            /* to change VBR default look in lame.h */
                            if (lame_get_VBR(gfp) == vbr_off)
                                lame_set_VBR(gfp, vbr_default);
                            lame_set_VBR_quality(gfp, (float) double_value);
                        }
                        break;
                    case 'v':
                        /* to change VBR default look in lame.h */
                        if (lame_get_VBR(gfp) == vbr_off)
                            lame_set_VBR(gfp, vbr_default);
                        break;

                    case 'q':
                        argUsed = getIntValue("q", arg, &int_value);
                        if (argUsed) 
                            (void) lame_set_quality(gfp, int_value);
                        break;
                    case 'f':
                        (void) lame_set_quality(gfp, 7);
                        break;
                    case 'h':
                        (void) lame_set_quality(gfp, 2);
                        break;

                    case 's':
                        argUsed = getDoubleValue("s", arg, &double_value);
                        if (argUsed) {
                            double_value = (int) (double_value * (double_value <= 192 ? 1.e3 : 1.e0) + 0.5);
                            global_reader.input_samplerate = (int)double_value;
                            (void) lame_set_in_samplerate(gfp, (int)double_value);
                        }
                        break;
                    case 'b':
                        argUsed = getIntValue("b", arg, &int_value);
                        if (argUsed) {
                            lame_set_brate(gfp, int_value);
                            lame_set_VBR_min_bitrate_kbps(gfp, lame_get_brate(gfp));
                        }
                        break;
                    case 'B':
                        argUsed = getIntValue("B", arg, &int_value);
                        if (argUsed) {
                            lame_set_VBR_max_bitrate_kbps(gfp, int_value);
                        }
                        break;
                    case 'F':
                        lame_set_VBR_hard_min(gfp, 1);
                        break;
                    case 't': /* dont write VBR tag */
                        (void) lame_set_bWriteVbrTag(gfp, 0);
                        global_decoder.disable_wav_header = 1;
                        break;
                    case 'T': /* do write VBR tag */
                        (void) lame_set_bWriteVbrTag(gfp, 1);
                        nogap_tags = 1;
                        global_decoder.disable_wav_header = 0;
                        break;
                    case 'r': /* force raw pcm input file */
#if defined(LIBSNDFILE)
                        error_printf
                            ("WARNING: libsndfile may ignore -r and perform fseek's on the input.\n"
                             "Compile without libsndfile if this is a problem.\n");
#endif
                        global_reader.input_format = sf_raw;
                        break;
                    case 'x': /* force byte swapping */
                        global_reader.swapbytes = 1;
                        break;
                    case 'p': /* (jo) error_protection: add crc16 information to stream */
                        lame_set_error_protection(gfp, 1);
                        break;
                    case 'a': /* autoconvert input file from stereo to mono - for mono mp3 encoding */
                        autoconvert = 1;
                        (void) lame_set_mode(gfp, MONO);
                        break;
                    case 'd':   /*(void) lame_set_allow_diff_short( gfp, 1 ); */
                    case 'k':   /*lame_set_lowpassfreq(gfp, -1);
                                  lame_set_highpassfreq(gfp, -1); */
                        error_printf("WARNING: -%c is obsolete.\n", c);
                        break;
                    case 'S':
                        global_ui_config.silent = 5;
                        break;
                    case 'X':
                        /*  experimental switch -X:
                            the differnt types of quant compare are tough
                            to communicate to endusers, so they shouldn't
                            bother to toy around with them
                         */
                        {
                            int     x, y;
                            int     n = sscanf(arg, "%d,%d", &x, &y);
                            if (n == 1) {
                                y = x;
                            }
                            argUsed = 1;
                            if (internal_opts_enabled) {
                                lame_set_quant_comp(gfp, x);
                                lame_set_quant_comp_short(gfp, y);
                            }
                        }
                        break;
                    case 'Y':
                        lame_set_experimentalY(gfp, 1);
                        break;
                    case 'Z':
                        /*  experimental switch -Z:
                         */
                        {
                            int     n = 1;
                            argUsed = sscanf(arg, "%d", &n);
                            /*if (internal_opts_enabled)*/
                            {
                                lame_set_experimentalZ(gfp, n);
                            }
                        }
                        break;
                    case 'e':
                        argUsed = 1;

                        switch (*arg) {
                        case 'n':
                            lame_set_emphasis(gfp, 0);
                            break;
                        case '5':
                            lame_set_emphasis(gfp, 1);
                            break;
                        case 'c':
                            lame_set_emphasis(gfp, 3);
                            break;
                        default:
                            error_printf("%s: -e emp must be n/5/c not %s\n", ProgramName, arg);
                            return -1;
                        }
                        break;
                    case 'c':
                        lame_set_copyright(gfp, 1);
                        break;
                    case 'o':
                        lame_set_original(gfp, 0);
                        break;

                    case '?':
                        long_help(gfp, stdout, ProgramName, 0 /* LESSMODE=NO */ );
                        return -1;

                    default:
                        error_printf("%s: unrecognized option -%c\n", ProgramName, c);
                        return -1;
                    }
                    if (argUsed) {
                        if (arg == token)
                            token = ""; /* no more from token */
                        else
                            ++i; /* skip arg we used */
                        arg = "";
                        argUsed = 0;
                    }
                }
            }
        }
        else {
            if (nogap) {
                if ((num_nogap != NULL) && (count_nogap < *num_nogap)) {
                    strncpy(nogap_inPath[count_nogap++], argv[i], PATH_MAX + 1);
                    input_file = 1;
                }
                else {
                    /* sorry, calling program did not allocate enough space */
                    error_printf
                        ("Error: 'nogap option'.  Calling program does not allow nogap option, or\n"
                         "you have exceeded maximum number of input files for the nogap option\n");
                    *num_nogap = -1;
                    return -1;
                }
            }
            else {
                /* normal options:   inputfile  [outputfile], and
                   either one can be a '-' for stdin/stdout */
                if (inPath[0] == '\0') {
                    strncpy(inPath, argv[i], PATH_MAX + 1);
                    input_file = 1;
                }
                else {
                    if (outPath[0] == '\0')
                        strncpy(outPath, argv[i], PATH_MAX + 1);
                    else {
                        error_printf("%s: excess arg %s\n", ProgramName, argv[i]);
                        return -1;
                    }
                }
            }
        }
    }                   /* loop over args */

    if (!input_file) {
        usage(Console_IO.Console_fp, ProgramName);
        return -1;
    }

    if (lame_get_decode_only(gfp) && count_nogap > 0) {
        error_printf("combination of nogap and decode not supported!\n");
        return -1;
    }

    if (inPath[0] == '-') {
        if (global_ui_config.silent == 0) { /* user didn't overrule default behaviour */
            global_ui_config.silent = 1;
        }
    }
#ifdef WIN32
    else
        dosToLongFileName(inPath);
#endif

    if (outPath[0] == '\0') { /* no explicit output dir or file */
        if (count_nogap > 0) { /* in case of nogap encode */
            strncpy(outPath, outDir, PATH_MAX);
            outPath[PATH_MAX] = '\0'; /* whatever someone set via --out-dir <path> argument */
        }
        else if (inPath[0] == '-') {
            /* if input is stdin, default output is stdout */
            strcpy(outPath, "-");
        }
        else {
            char const* s_ext = lame_get_decode_only(gfp) ? ".wav" : ".mp3";
            if (generateOutPath(inPath, outDir, s_ext, outPath) != 0) {
                return -1;
            }
        }
    }

    /* RG is enabled by default */
    if (!noreplaygain)
        lame_set_findReplayGain(gfp, 1);

    /* disable VBR tags with nogap unless the VBR tags are forced */
    if (nogap && lame_get_bWriteVbrTag(gfp) && nogap_tags == 0) {
        console_printf("Note: Disabling VBR Xing/Info tag since it interferes with --nogap\n");
        lame_set_bWriteVbrTag(gfp, 0);
    }

    /* some file options not allowed with stdout */
    if (outPath[0] == '-') {
        (void) lame_set_bWriteVbrTag(gfp, 0); /* turn off VBR tag */
    }

    /* if user did not explicitly specify input is mp3, check file name */
    if (global_reader.input_format == sf_unknown)
        global_reader.input_format = filename_to_type(inPath);

#if !(defined HAVE_MPGLIB || defined AMIGA_MPEGA)
    if (is_mpeg_file_format(global_reader.input_format)) {
        error_printf("Error: libmp3lame not compiled with mpg123 *decoding* support \n");
        return -1;
    }
#endif

    /* default guess for number of channels */
    if (autoconvert)
        (void) lame_set_num_channels(gfp, 2);
    else if (MONO == lame_get_mode(gfp))
        (void) lame_set_num_channels(gfp, 1);
    else
        (void) lame_set_num_channels(gfp, 2);

    if (lame_get_free_format(gfp)) {
        if (lame_get_brate(gfp) < 8 || lame_get_brate(gfp) > 640) {
            error_printf("For free format, specify a bitrate between 8 and 640 kbps\n");
            error_printf("with the -b <bitrate> option\n");
            return -1;
        }
    }
    if (num_nogap != NULL)
        *num_nogap = count_nogap;
    return 0;
}

static int
string_to_argv(char* str, char** argv, int N)
{
    int     argc = 0;
    if (str == 0) return argc;
    argv[argc++] = "lhama";
    for (;;) {
        int     quoted = 0;
        while (isspace(*str)) { /* skip blanks */
            ++str;
        }
        if (*str == '\"') { /* is quoted argument ? */
            quoted = 1;
            ++str;
        }
        if (*str == '\0') { /* end of string reached */
            break;
        }
        /* found beginning of some argument */
        if (argc < N) {
            argv[argc++] = str;
        }
        /* look out for end of argument, either end of string, blank or quote */
        for(; *str != '\0'; ++str) {
            if (quoted) {
                if (*str == '\"') { /* end of quotation reached */
                    *str++ = '\0';
                    break;
                }
            }
            else {
                if (isspace(*str)) { /* parameter separator reached */
                    *str++ = '\0';
                    break;
                }
            }
        }
    }
    return argc;
}

static int
merge_argv(int argc, char** argv, int str_argc, char** str_argv, int N)
{
    int     i;
    if (argc > 0) {
        str_argv[0] = argv[0];
        if (str_argc < 1) str_argc = 1;
    }
    for (i = 1; i < argc; ++i) {
        int     j = str_argc + i - 1;
        if (j < N) {
            str_argv[j] = argv[i];
        }
    }
    return argc + str_argc - 1;
}

#ifdef DEBUG
static void
dump_argv(int argc, char** argv)
{
    int     i;
    for (i = 0; i < argc; ++i) {
        printf("%d: \"%s\"\n",i,argv[i]);
    }
}
#endif


int parse_args(lame_t gfp, int argc, char **argv, char *const inPath, char *const outPath, char **nogap_inPath, int *num_nogap)
{
    char   *str_argv[512], *str;
    int     str_argc, ret;
    str = lame_getenv("LAMEOPT");
    str_argc = string_to_argv(str, str_argv, dimension_of(str_argv));
    str_argc = merge_argv(argc, argv, str_argc, str_argv, dimension_of(str_argv));
#ifdef DEBUG
    dump_argv(str_argc, str_argv);
#endif
    ret = parse_args_(gfp, str_argc, str_argv, inPath, outPath, nogap_inPath, num_nogap);
    free(str);
    return ret;
}

/* end of parse.c */
