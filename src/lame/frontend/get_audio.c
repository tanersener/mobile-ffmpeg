/*
 *      Get Audio routines source file
 *
 *      Copyright (c) 1999 Albert L Faber
 *                    2008-2017 Robert Hegemann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/* $Id: get_audio.c,v 1.167 2017/09/06 15:07:29 robert Exp $ */


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>

#ifdef HAVE_LIMITS_H
# include <limits.h>
#endif

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

#ifdef HAVE_INTTYPES_H
# include <inttypes.h>
#else
# ifdef HAVE_STDINT_H
#  include <stdint.h>
# endif
#endif

#define         MAX_U_32_NUM            0xFFFFFFFF


#include <math.h>

#if defined(__riscos__)
# include <kernel.h>
# include <sys/swis.h>
#elif defined(_WIN32)
# include <sys/types.h>
# include <sys/stat.h>
#else
# include <sys/stat.h>
#endif

#ifdef __sun__
/* woraround for SunOS 4.x, it has SEEK_* defined here */
#include <unistd.h>
#endif

#include "lame.h"
#include "main.h"
#include "get_audio.h"
#include "lametime.h"
#include "console.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

#ifndef STR
# define __STR(x)  #x
# define STR(x)    __STR(x)
#define __LOC__ __FILE__ "("STR(__LINE__)") : "
#endif


#define FLOAT_TO_UNSIGNED(f) ((unsigned long)(((long)((f) - 2147483648.0)) + 2147483647L + 1))
#define UNSIGNED_TO_FLOAT(u) (((double)((long)((u) - 2147483647L - 1))) + 2147483648.0)

static unsigned int uint32_high_low(unsigned char *bytes)
{
    uint32_t const hh = bytes[0];
    uint32_t const hl = bytes[1];
    uint32_t const lh = bytes[2];
    uint32_t const ll = bytes[3];
    return (hh << 24) | (hl << 16) | (lh << 8) | ll;
}

static double
read_ieee_extended_high_low(FILE * fp)
{
    unsigned char bytes[10];
    memset(bytes, 0, 10);
    fread(bytes, 1, 10, fp);
    {
        int32_t const s = (bytes[0] & 0x80);
        int32_t const e_h = (bytes[0] & 0x7F);
        int32_t const e_l = bytes[1];
        int32_t e = (e_h << 8) | e_l;
        uint32_t const hm = uint32_high_low(bytes + 2);
        uint32_t const lm = uint32_high_low(bytes + 6);
        double  result = 0;
        if (e != 0 || hm != 0 || lm != 0) {
            if (e == 0x7fff) {
                result = HUGE_VAL;
            }
            else {
                double  mantissa_h = UNSIGNED_TO_FLOAT(hm);
                double  mantissa_l = UNSIGNED_TO_FLOAT(lm);
                e -= 0x3fff;
                e -= 31;
                result = ldexp(mantissa_h, e);
                e -= 32;
                result += ldexp(mantissa_l, e);
            }
        }
        return s ? -result : result;
    }
}


static int
read_16_bits_low_high(FILE * fp)
{
    unsigned char bytes[2] = { 0, 0 };
    fread(bytes, 1, 2, fp);
    {
        int32_t const low = bytes[0];
        int32_t const high = (signed char) (bytes[1]);
        return (high << 8) | low;
    }
}


static int
read_32_bits_low_high(FILE * fp)
{
    unsigned char bytes[4] = { 0, 0, 0, 0 };
    fread(bytes, 1, 4, fp);
    {
        int32_t const low = bytes[0];
        int32_t const medl = bytes[1];
        int32_t const medh = bytes[2];
        int32_t const high = (signed char) (bytes[3]);
        return (high << 24) | (medh << 16) | (medl << 8) | low;
    }
}

static int
read_16_bits_high_low(FILE * fp)
{
    unsigned char bytes[2] = { 0, 0 };
    fread(bytes, 1, 2, fp);
    {
        int32_t const low = bytes[1];
        int32_t const high = (signed char) (bytes[0]);
        return (high << 8) | low;
    }
}

static int
read_32_bits_high_low(FILE * fp)
{
    unsigned char bytes[4] = { 0, 0, 0, 0 };
    fread(bytes, 1, 4, fp);
    {
        int32_t const low = bytes[3];
        int32_t const medl = bytes[2];
        int32_t const medh = bytes[1];
        int32_t const high = (signed char) (bytes[0]);
        return (high << 24) | (medh << 16) | (medl << 8) | low;
    }
}

static void
write_16_bits_low_high(FILE * fp, int val)
{
    unsigned char bytes[2];
    bytes[0] = (val & 0xff);
    bytes[1] = ((val >> 8) & 0xff);
    fwrite(bytes, 1, 2, fp);
}

static void
write_32_bits_low_high(FILE * fp, int val)
{
    unsigned char bytes[4];
    bytes[0] = (val & 0xff);
    bytes[1] = ((val >> 8) & 0xff);
    bytes[2] = ((val >> 16) & 0xff);
    bytes[3] = ((val >> 24) & 0xff);
    fwrite(bytes, 1, 4, fp);
}

#ifdef LIBSNDFILE

#include <sndfile.h>


#else

typedef void SNDFILE;

#endif /* ifdef LIBSNDFILE */



typedef struct blockAlign_struct {
    unsigned long offset;
    unsigned long blockSize;
} blockAlign;

typedef struct IFF_AIFF_struct {
    short   numChannels;
    unsigned long numSampleFrames;
    short   sampleSize;
    double  sampleRate;
    unsigned long sampleType;
    blockAlign blkAlgn;
} IFF_AIFF;



struct PcmBuffer {
    void   *ch[2];           /* buffer for each channel */
    int     w;               /* sample width */
    int     n;               /* number samples allocated */
    int     u;               /* number samples used */
    int     skip_start;      /* number samples to ignore at the beginning */
    int     skip_end;        /* number samples to ignore at the end */
};

typedef struct PcmBuffer PcmBuffer;

static void
initPcmBuffer(PcmBuffer * b, int w)
{
    b->ch[0] = 0;
    b->ch[1] = 0;
    b->w = w;
    b->n = 0;
    b->u = 0;
    b->skip_start = 0;
    b->skip_end = 0;
}

static void
freePcmBuffer(PcmBuffer * b)
{
    if (b != 0) {
        free(b->ch[0]);
        free(b->ch[1]);
        b->ch[0] = 0;
        b->ch[1] = 0;
        b->n = 0;
        b->u = 0;
    }
}

static int
addPcmBuffer(PcmBuffer * b, void *a0, void *a1, int read)
{
    int     a_n;

    if (b == 0) {
        return 0;
    }
    if (read < 0) {
        return b->u - b->skip_end;
    }
    if (b->skip_start >= read) {
        b->skip_start -= read;
        return b->u - b->skip_end;
    }
    a_n = read - b->skip_start;

    if (b != 0 && a_n > 0) {
        int const a_skip = b->w * b->skip_start;
        int const a_want = b->w * a_n;
        int const b_used = b->w * b->u;
        int const b_have = b->w * b->n;
        int const b_need = b->w * (b->u + a_n);
        if (b_have < b_need) {
            b->n = b->u + a_n;
            b->ch[0] = realloc(b->ch[0], b_need);
            b->ch[1] = realloc(b->ch[1], b_need);
        }
        b->u += a_n;
        if (b->ch[0] != 0 && a0 != 0) {
            char   *src = a0;
            char   *dst = b->ch[0];
            memcpy(dst + b_used, src + a_skip, a_want);
        }
        if (b->ch[1] != 0 && a1 != 0) {
            char   *src = a1;
            char   *dst = b->ch[1];
            memcpy(dst + b_used, src + a_skip, a_want);
        }
    }
    b->skip_start = 0;
    return b->u - b->skip_end;
}

static int
takePcmBuffer(PcmBuffer * b, void *a0, void *a1, int a_n, int mm)
{
    if (a_n > mm) {
        a_n = mm;
    }
    if (b != 0 && a_n > 0) {
        int const a_take = b->w * a_n;
        if (a0 != 0 && b->ch[0] != 0) {
            memcpy(a0, b->ch[0], a_take);
        }
        if (a1 != 0 && b->ch[1] != 0) {
            memcpy(a1, b->ch[1], a_take);
        }
        b->u -= a_n;
        if (b->u < 0) {
            b->u = 0;
            return a_n;
        }
        if (b->ch[0] != 0) {
            memmove(b->ch[0], (char *) b->ch[0] + a_take, b->w * b->u);
        }
        if (b->ch[1] != 0) {
            memmove(b->ch[1], (char *) b->ch[1] + a_take, b->w * b->u);
        }
    }
    return a_n;
}

/* global data for get_audio.c. */
typedef struct get_audio_global_data_struct {
    int     count_samples_carefully;
    int     pcmbitwidth;
    int     pcmswapbytes;
    int     pcm_is_unsigned_8bit;
    int     pcm_is_ieee_float;
    unsigned int num_samples_read;
    FILE   *music_in;
    SNDFILE *snd_file;
    hip_t   hip;
    PcmBuffer pcm32;
    PcmBuffer pcm16;
    size_t  in_id3v2_size;
    unsigned char* in_id3v2_tag;
} get_audio_global_data;

static get_audio_global_data global;



#ifdef AMIGA_MPEGA
int     lame_decode_initfile(const char *fullname, mp3data_struct * const mp3data);
#else
int     lame_decode_initfile(FILE * fd, mp3data_struct * mp3data, int *enc_delay, int *enc_padding);
#endif

/* read mp3 file until mpglib returns one frame of PCM data */
static int lame_decode_fromfile(FILE * fd, short int pcm_l[], short int pcm_r[],
                                mp3data_struct * mp3data);


static int read_samples_pcm(FILE * musicin, int sample_buffer[2304], int samples_to_read);
static int read_samples_mp3(lame_t gfp, FILE * musicin, short int mpg123pcm[2][1152]);
#ifdef LIBSNDFILE
static SNDFILE *open_snd_file(lame_t gfp, char const *inPath);
#endif
static FILE *open_mpeg_file(lame_t gfp, char const *inPath, int *enc_delay, int *enc_padding);
static FILE *open_wave_file(lame_t gfp, char const *inPath, int *enc_delay, int *enc_padding);
static int close_input_file(FILE * musicin);


static  size_t
min_size_t(size_t a, size_t b)
{
    if (a < b) {
        return a;
    }
    return b;
}

enum ByteOrder machine_byte_order(void);

enum ByteOrder
machine_byte_order(void)
{
    long    one = 1;
    return !(*((char *) (&one))) ? ByteOrderBigEndian : ByteOrderLittleEndian;
}



/* Replacement for forward fseek(,,SEEK_CUR), because fseek() fails on pipes */


static int
fskip(FILE * fp, long offset, int whence)
{
#ifndef PIPE_BUF
    char    buffer[4096];
#else
    char    buffer[PIPE_BUF];
#endif

/* S_ISFIFO macro is defined on newer Linuxes */
#ifndef S_ISFIFO
# ifdef _S_IFIFO
    /* _S_IFIFO is defined on Win32 and Cygwin */
#  define S_ISFIFO(m) (((m)&_S_IFIFO) == _S_IFIFO)
# endif
#endif

#ifdef S_ISFIFO
    /* fseek is known to fail on pipes with several C-Library implementations
       workaround: 1) test for pipe
       2) for pipes, only relatvie seeking is possible
       3)            and only in forward direction!
       else fallback to old code
     */
    {
        int const fd = fileno(fp);
        struct stat file_stat;

        if (fstat(fd, &file_stat) == 0) {
            if (S_ISFIFO(file_stat.st_mode)) {
                if (whence != SEEK_CUR || offset < 0) {
                    return -1;
                }
                while (offset > 0) {
                    size_t const bytes_to_skip = min_size_t(sizeof(buffer), offset);
                    size_t const read = fread(buffer, 1, bytes_to_skip, fp);
                    if (read < 1) {
                        return -1;
                    }
                    assert( read <= LONG_MAX );
                    offset -= (long) read;
                }
                return 0;
            }
        }
    }
#endif
    if (0 == fseek(fp, offset, whence)) {
        return 0;
    }

    if (whence != SEEK_CUR || offset < 0) {
        if (global_ui_config.silent < 10) {
            error_printf
                ("fskip problem: Mostly the return status of functions is not evaluate so it is more secure to polute <stderr>.\n");
        }
        return -1;
    }

    while (offset > 0) {
        size_t const bytes_to_skip = min_size_t(sizeof(buffer), offset);
        size_t const read = fread(buffer, 1, bytes_to_skip, fp);
        if (read < 1) {
            return -1;
        }
        assert( read <= LONG_MAX );
        offset -= (long) read;
    }

    return 0;
}


static  off_t
lame_get_file_size(FILE * fp)
{
    struct stat sb;
    int     fd = fileno(fp);

    if (0 == fstat(fd, &sb))
        return sb.st_size;
    return (off_t) - 1;
}


FILE   *
init_outfile(char const *outPath, int decode)
{
    FILE   *outf;

    /* open the output file */
    if (0 == strcmp(outPath, "-")) {
        outf = stdout;
        lame_set_stream_binary_mode(outf);
    }
    else {
        outf = lame_fopen(outPath, "w+b");
#ifdef __riscos__
        /* Assign correct file type */
        if (outf != NULL) {
            char   *p, *out_path = strdup(outPath);
            for (p = out_path; *p; p++) { /* ugly, ugly to modify a string */
                switch (*p) {
                case '.':
                    *p = '/';
                    break;
                case '/':
                    *p = '.';
                    break;
                }
            }
            SetFiletype(out_path, decode ? 0xFB1 /*WAV*/ : 0x1AD /*AMPEG*/);
            free(out_path);
        }
#else
        (void) decode;
#endif
    }
    return outf;
}


static void
setSkipStartAndEnd(lame_t gfp, int enc_delay, int enc_padding)
{
    int     skip_start = 0, skip_end = 0;

    if (global_decoder.mp3_delay_set)
        skip_start = global_decoder.mp3_delay;

    switch (global_reader.input_format) {
    case sf_mp123:
        break;

    case sf_mp3:
        if (skip_start == 0) {
            if (enc_delay > -1 || enc_padding > -1) {
                if (enc_delay > -1)
                    skip_start = enc_delay + 528 + 1;
                if (enc_padding > -1)
                    skip_end = enc_padding - (528 + 1);
            }
            else
                skip_start = lame_get_encoder_delay(gfp) + 528 + 1;
        }
        else {
            /* user specified a value of skip. just add for decoder */
            skip_start += 528 + 1; /* mp3 decoder has a 528 sample delay, plus user supplied "skip" */
        }
        break;
    case sf_mp2:
        skip_start += 240 + 1;
        break;
    case sf_mp1:
        skip_start += 240 + 1;
        break;
    case sf_raw:
        skip_start += 0; /* other formats have no delay *//* is += 0 not better ??? */
        break;
    case sf_wave:
        skip_start += 0; /* other formats have no delay *//* is += 0 not better ??? */
        break;
    case sf_aiff:
        skip_start += 0; /* other formats have no delay *//* is += 0 not better ??? */
        break;
    default:
        skip_start += 0; /* other formats have no delay *//* is += 0 not better ??? */
        break;
    }
    skip_start = skip_start < 0 ? 0 : skip_start;
    skip_end = skip_end < 0 ? 0 : skip_end;
    global. pcm16.skip_start = global.pcm32.skip_start = skip_start;
    global. pcm16.skip_end = global.pcm32.skip_end = skip_end;
}



int
init_infile(lame_t gfp, char const *inPath)
{
    int     enc_delay = 0, enc_padding = 0;
    /* open the input file */
    global. count_samples_carefully = 0;
    global. num_samples_read = 0;
    global. pcmbitwidth = global_raw_pcm.in_bitwidth;
    global. pcmswapbytes = global_reader.swapbytes;
    global. pcm_is_unsigned_8bit = global_raw_pcm.in_signed == 1 ? 0 : 1;
    global. pcm_is_ieee_float = 0;
    global. hip = 0;
    global. music_in = 0;
    global. snd_file = 0;
    global. in_id3v2_size = 0;
    global. in_id3v2_tag = 0;
    if (is_mpeg_file_format(global_reader.input_format)) {
        global. music_in = open_mpeg_file(gfp, inPath, &enc_delay, &enc_padding);
    }
    else {
#ifdef LIBSNDFILE
        if (strcmp(inPath, "-") != 0) { /* not for stdin */
            global. snd_file = open_snd_file(gfp, inPath);
        }
#endif
        if (global.snd_file == 0) {
            global. music_in = open_wave_file(gfp, inPath, &enc_delay, &enc_padding);
        }
    }
    initPcmBuffer(&global.pcm32, sizeof(int));
    initPcmBuffer(&global.pcm16, sizeof(short));
    setSkipStartAndEnd(gfp, enc_delay, enc_padding);
    {
        unsigned long n = lame_get_num_samples(gfp);
        if (n != MAX_U_32_NUM) {
            unsigned long const discard = global.pcm32.skip_start + global.pcm32.skip_end;
            lame_set_num_samples(gfp, n > discard ? n - discard : 0);
        }
    }
    return (global.snd_file != NULL || global.music_in != NULL) ? 1 : -1;
}

int
samples_to_skip_at_start(void)
{
    return global.pcm32.skip_start;
}

int
samples_to_skip_at_end(void)
{
    return global.pcm32.skip_end;
}

void
close_infile(void)
{
#if defined(HAVE_MPGLIB)
    if (global.hip != 0) {
        hip_decode_exit(global.hip); /* release mp3decoder memory */
        global. hip = 0;
    }
#endif
    close_input_file(global.music_in);
#ifdef LIBSNDFILE
    if (global.snd_file) {
        if (sf_close(global.snd_file) != 0) {
            if (global_ui_config.silent < 10) {
                error_printf("Could not close sound file \n");
            }
        }
        global. snd_file = 0;
    }
#endif
    freePcmBuffer(&global.pcm32);
    freePcmBuffer(&global.pcm16);
    global. music_in = 0;
    free(global.in_id3v2_tag);
    global.in_id3v2_tag = 0;
    global.in_id3v2_size = 0;
}


static int
        get_audio_common(lame_t gfp, int buffer[2][1152], short buffer16[2][1152]);

/************************************************************************
*
* get_audio()
*
* PURPOSE:  reads a frame of audio data from a file to the buffer,
*   aligns the data for future processing, and separates the
*   left and right channels
*
************************************************************************/
int
get_audio(lame_t gfp, int buffer[2][1152])
{
    int     used = 0, read = 0;
    do {
        read = get_audio_common(gfp, buffer, NULL);
        used = addPcmBuffer(&global.pcm32, buffer[0], buffer[1], read);
    } while (used <= 0 && read > 0);
    if (read < 0) {
        return read;
    }
    if (global_reader.swap_channel == 0)
        return takePcmBuffer(&global.pcm32, buffer[0], buffer[1], used, 1152);
    else
        return takePcmBuffer(&global.pcm32, buffer[1], buffer[0], used, 1152);
}

/*
  get_audio16 - behave as the original get_audio function, with a limited
                16 bit per sample output
*/
int
get_audio16(lame_t gfp, short buffer[2][1152])
{
    int     used = 0, read = 0;
    do {
        read = get_audio_common(gfp, NULL, buffer);
        used = addPcmBuffer(&global.pcm16, buffer[0], buffer[1], read);
    } while (used <= 0 && read > 0);
    if (read < 0) {
        return read;
    }
    if (global_reader.swap_channel == 0)
        return takePcmBuffer(&global.pcm16, buffer[0], buffer[1], used, 1152);
    else
        return takePcmBuffer(&global.pcm16, buffer[1], buffer[0], used, 1152);
}

/************************************************************************
  get_audio_common - central functionality of get_audio*
    in: gfp
        buffer    output to the int buffer or 16-bit buffer
   out: buffer    int output    (if buffer != NULL)
        buffer16  16-bit output (if buffer == NULL) 
returns: samples read
note: either buffer or buffer16 must be allocated upon call
*/
static int
get_audio_common(lame_t gfp, int buffer[2][1152], short buffer16[2][1152])
{
    const int num_channels = lame_get_num_channels(gfp);
    const int framesize = lame_get_framesize(gfp);
    int     insamp[2 * 1152];
    short   buf_tmp16[2][1152];
    int     samples_read;
    int     samples_to_read;
    unsigned int remaining;
    int     i;
    int    *p;

    /* sanity checks, that's what we expect to be true */
    if ((num_channels < 1 || 2 < num_channels)
      ||(framesize < 1 || 1152 < framesize)) {
        if (global_ui_config.silent < 10) {
            error_printf("Error: internal problem!\n");
        }
        return -1;
    }

    /* 
     * NOTE: LAME can now handle arbritray size input data packets,
     * so there is no reason to read the input data in chuncks of
     * size "framesize".  EXCEPT:  the LAME graphical frame analyzer 
     * will get out of sync if we read more than framesize worth of data.
     */

    samples_to_read = framesize;

    /* if this flag has been set, then we are carefull to read
     * exactly num_samples and no more.  This is useful for .wav and .aiff
     * files which have id3 or other tags at the end.  Note that if you
     * are using LIBSNDFILE, this is not necessary 
     */
    if (global.count_samples_carefully) {
        unsigned int tmp_num_samples;
        /* get num_samples */
        if (is_mpeg_file_format(global_reader.input_format)) {
            tmp_num_samples = global_decoder.mp3input_data.nsamp;
        }
        else {
            tmp_num_samples = lame_get_num_samples(gfp);
        }
        if (global.num_samples_read < tmp_num_samples) {
            remaining = tmp_num_samples - global.num_samples_read;
        }
        else {
            remaining = 0;
        }
        if (remaining < (unsigned int) framesize && 0 != tmp_num_samples)
            /* in case the input is a FIFO (at least it's reproducible with
               a FIFO) tmp_num_samples may be 0 and therefore remaining
               would be 0, but we need to read some samples, so don't
               change samples_to_read to the wrong value in this case */
            samples_to_read = remaining;
    }

    if (is_mpeg_file_format(global_reader.input_format)) {
        if (buffer != NULL)
            samples_read = read_samples_mp3(gfp, global.music_in, buf_tmp16);
        else
            samples_read = read_samples_mp3(gfp, global.music_in, buffer16);
        if (samples_read < 0) {
            return samples_read;
        }
    }
    else {
        if (global.snd_file) {
#ifdef LIBSNDFILE
            samples_read = sf_read_int(global.snd_file, insamp, num_channels * samples_to_read);
#else
            samples_read = 0;
#endif
        }
        else {
            samples_read =
                read_samples_pcm(global.music_in, insamp, num_channels * samples_to_read);
        }
        if (samples_read < 0) {
            return samples_read;
        }
        p = insamp + samples_read;
        samples_read /= num_channels;
        if (buffer != NULL) { /* output to int buffer */
            if (num_channels == 2) {
                for (i = samples_read; --i >= 0;) {
                    buffer[1][i] = *--p;
                    buffer[0][i] = *--p;
                }
            }
            else if (num_channels == 1) {
                memset(buffer[1], 0, samples_read * sizeof(int));
                for (i = samples_read; --i >= 0;) {
                    buffer[0][i] = *--p;
                }
            }
            else
                assert(0);
        }
        else {          /* convert from int; output to 16-bit buffer */
            if (num_channels == 2) {
                for (i = samples_read; --i >= 0;) {
                    buffer16[1][i] = *--p >> (8 * sizeof(int) - 16);
                    buffer16[0][i] = *--p >> (8 * sizeof(int) - 16);
                }
            }
            else if (num_channels == 1) {
                memset(buffer16[1], 0, samples_read * sizeof(short));
                for (i = samples_read; --i >= 0;) {
                    buffer16[0][i] = *--p >> (8 * sizeof(int) - 16);
                }
            }
            else
                assert(0);
        }
    }

    /* LAME mp3 output 16bit -  convert to int, if necessary */
    if (is_mpeg_file_format(global_reader.input_format)) {
        if (buffer != NULL) {
            for (i = samples_read; --i >= 0;)
                buffer[0][i] = buf_tmp16[0][i] << (8 * sizeof(int) - 16);
            if (num_channels == 2) {
                for (i = samples_read; --i >= 0;)
                    buffer[1][i] = buf_tmp16[1][i] << (8 * sizeof(int) - 16);
            }
            else if (num_channels == 1) {
                memset(buffer[1], 0, samples_read * sizeof(int));
            }
            else
                assert(0);
        }
    }


    /* if ... then it is considered infinitely long.
       Don't count the samples */
    if (global.count_samples_carefully)
        global. num_samples_read += samples_read;

    return samples_read;
}



static int
read_samples_mp3(lame_t gfp, FILE * musicin, short int mpg123pcm[2][1152])
{
    int     out;
#if defined(AMIGA_MPEGA)  ||  defined(HAVE_MPGLIB)
    int     samplerate;
    static const char type_name[] = "MP3 file";

    out = lame_decode_fromfile(musicin, mpg123pcm[0], mpg123pcm[1], &global_decoder.mp3input_data);
    /*
     * out < 0:  error, probably EOF
     * out = 0:  not possible with lame_decode_fromfile() ???
     * out > 0:  number of output samples
     */
    if (out < 0) {
        memset(mpg123pcm, 0, sizeof(**mpg123pcm) * 2 * 1152);
        return 0;
    }

    if (lame_get_num_channels(gfp) != global_decoder.mp3input_data.stereo) {
        if (global_ui_config.silent < 10) {
            error_printf("Error: number of channels has changed in %s - not supported\n",
                         type_name);
        }
        out = -1;
    }
    samplerate = global_reader.input_samplerate;
    if (samplerate == 0) {
        samplerate = global_decoder.mp3input_data.samplerate;
    }
    if (lame_get_in_samplerate(gfp) != samplerate) {
        if (global_ui_config.silent < 10) {
            error_printf("Error: sample frequency has changed in %s - not supported\n", type_name);
        }
        out = -1;
    }
#else
    out = -1;
#endif
    return out;
}

static
int set_input_num_channels(lame_t gfp, int num_channels)
{
    if (gfp) {
        if (-1 == lame_set_num_channels(gfp, num_channels)) {
            if (global_ui_config.silent < 10) {
                error_printf("Unsupported number of channels: %d\n", num_channels);
            }
            return 0;
        }
    }
    return 1;
}

static
int set_input_samplerate(lame_t gfp, int input_samplerate)
{
    if (gfp) {
        int sr = global_reader.input_samplerate;
        if (sr == 0) sr = input_samplerate;
        if (-1 == lame_set_in_samplerate(gfp, sr)) {
            if (global_ui_config.silent < 10) {
                error_printf("Unsupported sample rate: %d\n", sr);
            }
            return 0;
        }
    }
    return 1;
}

int
WriteWaveHeader(FILE * const fp, int pcmbytes, int freq, int channels, int bits)
{
    int     bytes = (bits + 7) / 8;

    /* quick and dirty, but documented */
    fwrite("RIFF", 1, 4, fp); /* label */
    write_32_bits_low_high(fp, pcmbytes + 44 - 8); /* length in bytes without header */
    fwrite("WAVEfmt ", 2, 4, fp); /* 2 labels */
    write_32_bits_low_high(fp, 2 + 2 + 4 + 4 + 2 + 2); /* length of PCM format declaration area */
    write_16_bits_low_high(fp, 1); /* is PCM? */
    write_16_bits_low_high(fp, channels); /* number of channels */
    write_32_bits_low_high(fp, freq); /* sample frequency in [Hz] */
    write_32_bits_low_high(fp, freq * channels * bytes); /* bytes per second */
    write_16_bits_low_high(fp, channels * bytes); /* bytes per sample time */
    write_16_bits_low_high(fp, bits); /* bits per sample */
    fwrite("data", 1, 4, fp); /* label */
    write_32_bits_low_high(fp, pcmbytes); /* length in bytes of raw PCM data */

    return ferror(fp) ? -1 : 0;
}




#if defined(LIBSNDFILE)

extern SNDFILE *sf_wchar_open(wchar_t const *wpath, int mode, SF_INFO * sfinfo);

static SNDFILE *
open_snd_file(lame_t gfp, char const *inPath)
{
    char const *lpszFileName = inPath;
    SNDFILE *gs_pSndFileIn = NULL;
    SF_INFO gs_wfInfo;

    {
#if defined( _WIN32 ) && !defined(__MINGW32__)
        wchar_t *file_name = utf8ToUnicode(lpszFileName);
#endif
        /* Try to open the sound file */
        memset(&gs_wfInfo, 0, sizeof(gs_wfInfo));
#if defined( _WIN32 ) && !defined(__MINGW32__)
        gs_pSndFileIn = sf_wchar_open(file_name, SFM_READ, &gs_wfInfo);
#else
        gs_pSndFileIn = sf_open(lpszFileName, SFM_READ, &gs_wfInfo);
#endif

        if (gs_pSndFileIn == NULL) {
            if (global_raw_pcm.in_signed == 0 && global_raw_pcm.in_bitwidth != 8) {
                error_printf("Unsigned input only supported with bitwidth 8\n");
#if defined( _WIN32 ) && !defined(__MINGW32__)
                free(file_name);
#endif
                return 0;
            }
            /* set some defaults incase input is raw PCM */
            gs_wfInfo.seekable = (global_reader.input_format != sf_raw); /* if user specified -r, set to not seekable */
            gs_wfInfo.samplerate = lame_get_in_samplerate(gfp);
            gs_wfInfo.channels = lame_get_num_channels(gfp);
            gs_wfInfo.format = SF_FORMAT_RAW;
            if ((global_raw_pcm.in_endian == ByteOrderLittleEndian) ^ (global_reader.swapbytes !=
                                                                       0)) {
                gs_wfInfo.format |= SF_ENDIAN_LITTLE;
            }
            else {
                gs_wfInfo.format |= SF_ENDIAN_BIG;
            }
            switch (global_raw_pcm.in_bitwidth) {
            case 8:
                gs_wfInfo.format |=
                    global_raw_pcm.in_signed == 0 ? SF_FORMAT_PCM_U8 : SF_FORMAT_PCM_S8;
                break;
            case 16:
                gs_wfInfo.format |= SF_FORMAT_PCM_16;
                break;
            case 24:
                gs_wfInfo.format |= SF_FORMAT_PCM_24;
                break;
            case 32:
                gs_wfInfo.format |= SF_FORMAT_PCM_32;
                break;
            default:
                break;
            }
#if defined( _WIN32 ) && !defined(__MINGW32__)
            gs_pSndFileIn = sf_wchar_open(file_name, SFM_READ, &gs_wfInfo);
#else
            gs_pSndFileIn = sf_open(lpszFileName, SFM_READ, &gs_wfInfo);
#endif
        }
#if defined( _WIN32 ) && !defined(__MINGW32__)
        free(file_name);
#endif

        /* Check result */
        if (gs_pSndFileIn == NULL) {
            sf_perror(gs_pSndFileIn);
            if (global_ui_config.silent < 10) {
                error_printf("Could not open sound file \"%s\".\n", lpszFileName);
            }
            return 0;
        }
        sf_command(gs_pSndFileIn, SFC_SET_SCALE_FLOAT_INT_READ, NULL, SF_TRUE);

        if ((gs_wfInfo.format & SF_FORMAT_RAW) == SF_FORMAT_RAW) {
            global_reader.input_format = sf_raw;
        }

#ifdef _DEBUG_SND_FILE
        printf("\n\nSF_INFO structure\n");
        printf("samplerate        :%d\n", gs_wfInfo.samplerate);
        printf("samples           :%d\n", gs_wfInfo.frames);
        printf("channels          :%d\n", gs_wfInfo.channels);
        printf("format            :");

        /* new formats from sbellon@sbellon.de  1/2000 */

        switch (gs_wfInfo.format & SF_FORMAT_TYPEMASK) {
        case SF_FORMAT_WAV:
            printf("Microsoft WAV format (big endian). ");
            break;
        case SF_FORMAT_AIFF:
            printf("Apple/SGI AIFF format (little endian). ");
            break;
        case SF_FORMAT_AU:
            printf("Sun/NeXT AU format (big endian). ");
            break;
            /*
               case SF_FORMAT_AULE:
               DEBUGF("DEC AU format (little endian). ");
               break;
             */
        case SF_FORMAT_RAW:
            printf("RAW PCM data. ");
            break;
        case SF_FORMAT_PAF:
            printf("Ensoniq PARIS file format. ");
            break;
        case SF_FORMAT_SVX:
            printf("Amiga IFF / SVX8 / SV16 format. ");
            break;
        case SF_FORMAT_NIST:
            printf("Sphere NIST format. ");
            break;
        default:
            assert(0);
            break;
        }

        switch (gs_wfInfo.format & SF_FORMAT_SUBMASK) {
            /*
               case SF_FORMAT_PCM:
               DEBUGF("PCM data in 8, 16, 24 or 32 bits.");
               break;
             */
        case SF_FORMAT_FLOAT:
            printf("32 bit Intel x86 floats.");
            break;
        case SF_FORMAT_ULAW:
            printf("U-Law encoded.");
            break;
        case SF_FORMAT_ALAW:
            printf("A-Law encoded.");
            break;
        case SF_FORMAT_IMA_ADPCM:
            printf("IMA ADPCM.");
            break;
        case SF_FORMAT_MS_ADPCM:
            printf("Microsoft ADPCM.");
            break;
            /*
               case SF_FORMAT_PCM_BE:
               DEBUGF("Big endian PCM data.");
               break;
               case SF_FORMAT_PCM_LE:
               DEBUGF("Little endian PCM data.");
               break;
             */
        case SF_FORMAT_PCM_S8:
            printf("Signed 8 bit PCM.");
            break;
        case SF_FORMAT_PCM_U8:
            printf("Unsigned 8 bit PCM.");
            break;
        case SF_FORMAT_PCM_16:
            printf("Signed 16 bit PCM.");
            break;
        case SF_FORMAT_PCM_24:
            printf("Signed 24 bit PCM.");
            break;
        case SF_FORMAT_PCM_32:
            printf("Signed 32 bit PCM.");
            break;
            /*
               case SF_FORMAT_SVX_FIB:
               DEBUGF("SVX Fibonacci Delta encoding.");
               break;
               case SF_FORMAT_SVX_EXP:
               DEBUGF("SVX Exponential Delta encoding.");
               break;
             */
        default:
            assert(0);
            break;
        }

        printf("\n");
        printf("sections          :%d\n", gs_wfInfo.sections);
        printf("seekable          :%d\n", gs_wfInfo.seekable);
#endif
        /* Check result */
        if (gs_pSndFileIn == NULL) {
            sf_perror(gs_pSndFileIn);
            if (global_ui_config.silent < 10) {
                error_printf("Could not open sound file \"%s\".\n", lpszFileName);
            }
            return 0;
        }


        if(gs_wfInfo.frames >= 0 && gs_wfInfo.frames < (sf_count_t)(unsigned)MAX_U_32_NUM)
            (void) lame_set_num_samples(gfp, gs_wfInfo.frames);
        else
            (void) lame_set_num_samples(gfp, MAX_U_32_NUM);
        if (!set_input_num_channels(gfp, gs_wfInfo.channels)) {
            sf_close(gs_pSndFileIn);
            return 0;
        }
        if (!set_input_samplerate(gfp, gs_wfInfo.samplerate)) {
            sf_close(gs_pSndFileIn);
            return 0;
        }
        global. pcmbitwidth = 32;
    }
#if 0
    if (lame_get_num_samples(gfp) == MAX_U_32_NUM) {
        /* try to figure out num_samples */
        double const flen = lame_get_file_size(lpszFileName);
        if (flen >= 0) {
            /* try file size, assume 2 bytes per sample */
            lame_set_num_samples(gfp, flen / (2 * lame_get_num_channels(gfp)));
        }
    }
#endif
    return gs_pSndFileIn;
}

#endif /* defined(LIBSNDFILE) */



/************************************************************************
unpack_read_samples - read and unpack signed low-to-high byte or unsigned
                      single byte input. (used for read_samples function)
                      Output integers are stored in the native byte order
                      (little or big endian).  -jd
  in: samples_to_read
      bytes_per_sample
      swap_order    - set for high-to-low byte order input stream
 i/o: pcm_in
 out: sample_buffer  (must be allocated up to samples_to_read upon call)
returns: number of samples read
*/
static int
unpack_read_samples(const int samples_to_read, const int bytes_per_sample,
                    const int swap_order, int *sample_buffer, FILE * pcm_in)
{
    int     samples_read;
    int     i;
    int    *op;              /* output pointer */
    unsigned char *ip = (unsigned char *) sample_buffer; /* input pointer */
    const int b = sizeof(int) * 8;

    {
        size_t  samples_read_ = fread(sample_buffer, bytes_per_sample, samples_to_read, pcm_in);
        assert( samples_read_ <= INT_MAX );
        samples_read = (int) samples_read_;
    }
    op = sample_buffer + samples_read;

#define GA_URS_IFLOOP( ga_urs_bps ) \
    if( bytes_per_sample == ga_urs_bps ) \
      for( i = samples_read * bytes_per_sample; (i -= bytes_per_sample) >=0;)

    if (swap_order == 0) {
        GA_URS_IFLOOP(1)
            * --op = ip[i] << (b - 8);
        GA_URS_IFLOOP(2)
            * --op = ip[i] << (b - 16) | ip[i + 1] << (b - 8);
        GA_URS_IFLOOP(3)
            * --op = ip[i] << (b - 24) | ip[i + 1] << (b - 16) | ip[i + 2] << (b - 8);
        GA_URS_IFLOOP(4)
            * --op =
            ip[i] << (b - 32) | ip[i + 1] << (b - 24) | ip[i + 2] << (b - 16) | ip[i + 3] << (b -
                                                                                              8);
    }
    else {
        GA_URS_IFLOOP(1)
            * --op = (ip[i] ^ 0x80) << (b - 8) | 0x7f << (b - 16); /* convert from unsigned */
        GA_URS_IFLOOP(2)
            * --op = ip[i] << (b - 8) | ip[i + 1] << (b - 16);
        GA_URS_IFLOOP(3)
            * --op = ip[i] << (b - 8) | ip[i + 1] << (b - 16) | ip[i + 2] << (b - 24);
        GA_URS_IFLOOP(4)
            * --op =
            ip[i] << (b - 8) | ip[i + 1] << (b - 16) | ip[i + 2] << (b - 24) | ip[i + 3] << (b -
                                                                                             32);
    }
#undef GA_URS_IFLOOP
    if (global.pcm_is_ieee_float) {
        ieee754_float32_t const m_max = INT_MAX;
        ieee754_float32_t const m_min = -(ieee754_float32_t) INT_MIN;
        ieee754_float32_t *x = (ieee754_float32_t *) sample_buffer;
        assert(sizeof(ieee754_float32_t) == sizeof(int));
        for (i = 0; i < samples_to_read; ++i) {
            ieee754_float32_t const u = x[i];
            int     v;
            if (u >= 1) {
                v = INT_MAX;
            }
            else if (u <= -1) {
                v = INT_MIN;
            }
            else if (u >= 0) {
                v = (int) (u * m_max + 0.5f);
            }
            else {
                v = (int) (u * m_min - 0.5f);
            }
            sample_buffer[i] = v;
        }
    }
    return (samples_read);
}



/************************************************************************
*
* read_samples()
*
* PURPOSE:  reads the PCM samples from a file to the buffer
*
*  SEMANTICS:
* Reads #samples_read# number of shorts from #musicin# filepointer
* into #sample_buffer[]#.  Returns the number of samples read.
*
************************************************************************/

static int
read_samples_pcm(FILE * musicin, int sample_buffer[2304], int samples_to_read)
{
    int     samples_read;
    int     bytes_per_sample = global.pcmbitwidth / 8;
    int     swap_byte_order; /* byte order of input stream */

    switch (global.pcmbitwidth) {
    case 32:
    case 24:
    case 16:
        if (global_raw_pcm.in_signed == 0) {
            if (global_ui_config.silent < 10) {
                error_printf("Unsigned input only supported with bitwidth 8\n");
            }
            return -1;
        }
        swap_byte_order = (global_raw_pcm.in_endian != ByteOrderLittleEndian) ? 1 : 0;
        if (global.pcmswapbytes) {
            swap_byte_order = !swap_byte_order;
        }
        break;

    case 8:
        swap_byte_order = global.pcm_is_unsigned_8bit;
        break;

    default:
        if (global_ui_config.silent < 10) {
            error_printf("Only 8, 16, 24 and 32 bit input files supported \n");
        }
        return -1;
    }
    if (samples_to_read < 0 || samples_to_read > 2304) {
        if (global_ui_config.silent < 10) {
            error_printf("Error: unexpected number of samples to read: %d\n", samples_to_read);
        }
        return -1;
    }
    samples_read = unpack_read_samples(samples_to_read, bytes_per_sample, swap_byte_order,
                                       sample_buffer, musicin);
    if (ferror(musicin)) {
        if (global_ui_config.silent < 10) {
            error_printf("Error reading input file\n");
        }
        return -1;
    }

    return samples_read;
}



/* AIFF Definitions */

static int const IFF_ID_FORM = 0x464f524d; /* "FORM" */
static int const IFF_ID_AIFF = 0x41494646; /* "AIFF" */
static int const IFF_ID_AIFC = 0x41494643; /* "AIFC" */
static int const IFF_ID_COMM = 0x434f4d4d; /* "COMM" */
static int const IFF_ID_SSND = 0x53534e44; /* "SSND" */
static int const IFF_ID_MPEG = 0x4d504547; /* "MPEG" */

static int const IFF_ID_NONE = 0x4e4f4e45; /* "NONE" *//* AIFF-C data format */
static int const IFF_ID_2CBE = 0x74776f73; /* "twos" *//* AIFF-C data format */
static int const IFF_ID_2CLE = 0x736f7774; /* "sowt" *//* AIFF-C data format */

static int const WAV_ID_RIFF = 0x52494646; /* "RIFF" */
static int const WAV_ID_WAVE = 0x57415645; /* "WAVE" */
static int const WAV_ID_FMT = 0x666d7420; /* "fmt " */
static int const WAV_ID_DATA = 0x64617461; /* "data" */

#ifndef WAVE_FORMAT_PCM
static short const WAVE_FORMAT_PCM = 0x0001;
#endif
#ifndef WAVE_FORMAT_IEEE_FLOAT
static short const WAVE_FORMAT_IEEE_FLOAT = 0x0003;
#endif
#ifndef WAVE_FORMAT_EXTENSIBLE
static short const WAVE_FORMAT_EXTENSIBLE = 0xFFFE;
#endif


static long
make_even_number_of_bytes_in_length(long x)
{
    if ((x & 0x01) != 0) {
        return x + 1;
    }
    return x;
}


/*****************************************************************************
 *
 *	Read Microsoft Wave headers
 *
 *	By the time we get here the first 32-bits of the file have already been
 *	read, and we're pretty sure that we're looking at a WAV file.
 *
 *****************************************************************************/

static int
parse_wave_header(lame_global_flags * gfp, FILE * sf)
{
    int     format_tag = 0;
    int     channels = 0;
    int     bits_per_sample = 0;
    int     samples_per_sec = 0;


    int     is_wav = 0;
    unsigned long    data_length = 0, subSize = 0;
    int     loop_sanity = 0;

    (void) read_32_bits_high_low(sf); /* file_length */
    if (read_32_bits_high_low(sf) != WAV_ID_WAVE)
        return -1;

    for (loop_sanity = 0; loop_sanity < 20; ++loop_sanity) {
        int     type = read_32_bits_high_low(sf);

        if (type == WAV_ID_FMT) {
            subSize = read_32_bits_low_high(sf);
            subSize = make_even_number_of_bytes_in_length(subSize);
            if (subSize < 16) {
                /*DEBUGF(
                   "'fmt' chunk too short (only %ld bytes)!", subSize);  */
                return -1;
            }

            format_tag = read_16_bits_low_high(sf);
            subSize -= 2;
            channels = read_16_bits_low_high(sf);
            subSize -= 2;
            samples_per_sec = read_32_bits_low_high(sf);
            subSize -= 4;
            (void) read_32_bits_low_high(sf); /* avg_bytes_per_sec */
            subSize -= 4;
            (void) read_16_bits_low_high(sf); /* block_align */
            subSize -= 2;
            bits_per_sample = read_16_bits_low_high(sf);
            subSize -= 2;

            /* WAVE_FORMAT_EXTENSIBLE support */
            if ((subSize > 9) && (format_tag == WAVE_FORMAT_EXTENSIBLE)) {
                read_16_bits_low_high(sf); /* cbSize */
                read_16_bits_low_high(sf); /* ValidBitsPerSample */
                read_32_bits_low_high(sf); /* ChannelMask */
                /* SubType coincident with format_tag for PCM int or float */
                format_tag = read_16_bits_low_high(sf);
                subSize -= 10;
            }

            /* DEBUGF("   skipping %d bytes\n", subSize); */

            if (subSize > 0) {
                if (fskip(sf, (long) subSize, SEEK_CUR) != 0)
                    return -1;
            };

        }
        else if (type == WAV_ID_DATA) {
            subSize = read_32_bits_low_high(sf);
            data_length = subSize;
            is_wav = 1;
            /* We've found the audio data. Read no further! */
            break;

        }
        else {
            subSize = read_32_bits_low_high(sf);
            subSize = make_even_number_of_bytes_in_length(subSize);
            if (fskip(sf, (long) subSize, SEEK_CUR) != 0) {
                return -1;
            }
        }
    }
    if (is_wav) {
        if (format_tag == 0x0050 || format_tag == 0x0055) {
            return sf_mp123;
        }
        if (format_tag != WAVE_FORMAT_PCM && format_tag != WAVE_FORMAT_IEEE_FLOAT) {
            if (global_ui_config.silent < 10) {
                error_printf("Unsupported data format: 0x%04X\n", format_tag);
            }
            return 0;   /* oh no! non-supported format  */
        }


        /* make sure the header is sane */
        if (!set_input_num_channels(gfp, channels))
            return 0;
        if (!set_input_samplerate(gfp, samples_per_sec))
            return 0;
        /* avoid division by zero */
        if (bits_per_sample < 1) {
            if (global_ui_config.silent < 10)
                error_printf("Unsupported bits per sample: %d\n", bits_per_sample);
            return -1;
        }
        global. pcmbitwidth = bits_per_sample;
        global. pcm_is_unsigned_8bit = 1;
        global. pcm_is_ieee_float = (format_tag == WAVE_FORMAT_IEEE_FLOAT ? 1 : 0);
        if (data_length == MAX_U_32_NUM)
            (void) lame_set_num_samples(gfp, MAX_U_32_NUM);
        else
            (void) lame_set_num_samples(gfp, data_length / (channels * ((bits_per_sample + 7) / 8)));
        return 1;
    }
    return -1;
}



/************************************************************************
* aiff_check2
*
* PURPOSE:	Checks AIFF header information to make sure it is valid.
*	        returns 0 on success, 1 on errors
************************************************************************/

static int
aiff_check2(IFF_AIFF * const pcm_aiff_data)
{
    if (pcm_aiff_data->sampleType != (unsigned long) IFF_ID_SSND) {
        if (global_ui_config.silent < 10) {
            error_printf("ERROR: input sound data is not PCM\n");
        }
        return 1;
    }
    switch (pcm_aiff_data->sampleSize) {
    case 32:
    case 24:
    case 16:
    case 8:
        break;
    default:
        if (global_ui_config.silent < 10) {
            error_printf("ERROR: input sound data is not 8, 16, 24 or 32 bits\n");
        }
        return 1;
    }
    if (pcm_aiff_data->numChannels != 1 && pcm_aiff_data->numChannels != 2) {
        if (global_ui_config.silent < 10) {
            error_printf("ERROR: input sound data is not mono or stereo\n");
        }
        return 1;
    }
    if (pcm_aiff_data->blkAlgn.blockSize != 0) {
        if (global_ui_config.silent < 10) {
            error_printf("ERROR: block size of input sound data is not 0 bytes\n");
        }
        return 1;
    }
    /* A bug, since we correctly skip the offset earlier in the code.
       if (pcm_aiff_data->blkAlgn.offset != 0) {
       error_printf("Block offset is not 0 bytes in '%s'\n", file_name);
       return 1;
       } */

    return 0;
}


/*****************************************************************************
 *
 *	Read Audio Interchange File Format (AIFF) headers.
 *
 *	By the time we get here the first 32 bits of the file have already been
 *	read, and we're pretty sure that we're looking at an AIFF file.
 *
 *****************************************************************************/

static int
parse_aiff_header(lame_global_flags * gfp, FILE * sf)
{
    long    chunkSize = 0, subSize = 0, typeID = 0, dataType = IFF_ID_NONE;
    IFF_AIFF aiff_info;
    int     seen_comm_chunk = 0, seen_ssnd_chunk = 0;
    long    pcm_data_pos = -1;

    memset(&aiff_info, 0, sizeof(aiff_info));
    chunkSize = read_32_bits_high_low(sf);

    typeID = read_32_bits_high_low(sf);
    if ((typeID != IFF_ID_AIFF) && (typeID != IFF_ID_AIFC))
        return -1;

    while (chunkSize > 0) {
        long    ckSize;
        int     type = read_32_bits_high_low(sf);
        chunkSize -= 4;

        /* DEBUGF(
           "found chunk type %08x '%4.4s'\n", type, (char*)&type); */

        /* don't use a switch here to make it easier to use 'break' for SSND */
        if (type == IFF_ID_COMM) {
            seen_comm_chunk = seen_ssnd_chunk + 1;
            subSize = read_32_bits_high_low(sf);
            ckSize = make_even_number_of_bytes_in_length(subSize);
            chunkSize -= ckSize;

            aiff_info.numChannels = (short) read_16_bits_high_low(sf);
            ckSize -= 2;
            aiff_info.numSampleFrames = read_32_bits_high_low(sf);
            ckSize -= 4;
            aiff_info.sampleSize = (short) read_16_bits_high_low(sf);
            ckSize -= 2;
            aiff_info.sampleRate = read_ieee_extended_high_low(sf);
            ckSize -= 10;
            if (typeID == IFF_ID_AIFC) {
                dataType = read_32_bits_high_low(sf);
                ckSize -= 4;
            }
            if (fskip(sf, ckSize, SEEK_CUR) != 0)
                return -1;
        }
        else if (type == IFF_ID_SSND) {
            seen_ssnd_chunk = 1;
            subSize = read_32_bits_high_low(sf);
            ckSize = make_even_number_of_bytes_in_length(subSize);
            chunkSize -= ckSize;

            aiff_info.blkAlgn.offset = read_32_bits_high_low(sf);
            ckSize -= 4;
            aiff_info.blkAlgn.blockSize = read_32_bits_high_low(sf);
            ckSize -= 4;

            aiff_info.sampleType = IFF_ID_SSND;

            if (seen_comm_chunk > 0) {
                if (fskip(sf, (long) aiff_info.blkAlgn.offset, SEEK_CUR) != 0)
                    return -1;
                /* We've found the audio data. Read no further! */
                break;
            }
            pcm_data_pos = ftell(sf);
            if (pcm_data_pos >= 0) {
                pcm_data_pos += aiff_info.blkAlgn.offset;
            }
            if (fskip(sf, ckSize, SEEK_CUR) != 0)
                return -1;
        }
        else {
            subSize = read_32_bits_high_low(sf);
            ckSize = make_even_number_of_bytes_in_length(subSize);
            chunkSize -= ckSize;

            if (fskip(sf, ckSize, SEEK_CUR) != 0)
                return -1;
        }
    }
    if (dataType == IFF_ID_2CLE) {
        global. pcmswapbytes = global_reader.swapbytes;
    }
    else if (dataType == IFF_ID_2CBE) {
        global. pcmswapbytes = !global_reader.swapbytes;
    }
    else if (dataType == IFF_ID_NONE) {
        global. pcmswapbytes = !global_reader.swapbytes;
    }
    else {
        return -1;
    }

    /* DEBUGF("Parsed AIFF %d\n", is_aiff); */
    if (seen_comm_chunk && (seen_ssnd_chunk > 0 || aiff_info.numSampleFrames == 0)) {
        /* make sure the header is sane */
        if (0 != aiff_check2(&aiff_info))
            return 0;
        if (!set_input_num_channels(gfp, aiff_info.numChannels))
            return 0;
        if (!set_input_samplerate(gfp, (int) aiff_info.sampleRate))
            return 0;
        (void) lame_set_num_samples(gfp, aiff_info.numSampleFrames);
        global. pcmbitwidth = aiff_info.sampleSize;
        global. pcm_is_unsigned_8bit = 0;
        global. pcm_is_ieee_float = 0; /* FIXME: possible ??? */
        if (pcm_data_pos >= 0) {
            if (fseek(sf, pcm_data_pos, SEEK_SET) != 0) {
                if (global_ui_config.silent < 10) {
                    error_printf("Can't rewind stream to audio data position\n");
                }
                return 0;
            }
        }

        return 1;
    }
    return -1;
}



/************************************************************************
*
* parse_file_header
*
* PURPOSE: Read the header from a bytestream.  Try to determine whether
*		   it's a WAV file or AIFF without rewinding, since rewind
*		   doesn't work on pipes and there's a good chance we're reading
*		   from stdin (otherwise we'd probably be using libsndfile).
*
* When this function returns, the file offset will be positioned at the
* beginning of the sound data.
*
************************************************************************/

static int
parse_file_header(lame_global_flags * gfp, FILE * sf)
{

    int     type = read_32_bits_high_low(sf);
    /*
       DEBUGF(
       "First word of input stream: %08x '%4.4s'\n", type, (char*) &type); 
     */
    global. count_samples_carefully = 0;
    global. pcm_is_unsigned_8bit = global_raw_pcm.in_signed == 1 ? 0 : 1;
    /*global_reader.input_format = sf_raw; commented out, because it is better to fail
       here as to encode some hundreds of input files not supported by LAME
       If you know you have RAW PCM data, use the -r switch
     */

    if (type == WAV_ID_RIFF) {
        /* It's probably a WAV file */
        int const ret = parse_wave_header(gfp, sf);
        if (ret == sf_mp123) {
        	global. count_samples_carefully = 1;
            return sf_mp123;
        }
        if (ret > 0) {
            if (lame_get_num_samples(gfp) == MAX_U_32_NUM || global_reader.ignorewavlength == 1)
            {
                global. count_samples_carefully = 0;
                lame_set_num_samples(gfp, MAX_U_32_NUM);
            }
            else
                global. count_samples_carefully = 1;
            return sf_wave;
        }
        if (ret < 0) {
            if (global_ui_config.silent < 10) {
                error_printf("Warning: corrupt or unsupported WAVE format\n");
            }
        }
    }
    else if (type == IFF_ID_FORM) {
        /* It's probably an AIFF file */
        int const ret = parse_aiff_header(gfp, sf);
        if (ret > 0) {
            global. count_samples_carefully = 1;
            return sf_aiff;
        }
        if (ret < 0) {
            if (global_ui_config.silent < 10) {
                error_printf("Warning: corrupt or unsupported AIFF format\n");
            }
        }
    }
    else {
        if (global_ui_config.silent < 10) {
            error_printf("Warning: unsupported audio format\n");
        }
    }
    return sf_unknown;
}


static int
open_mpeg_file_part2(lame_t gfp, FILE* musicin, char const *inPath, int *enc_delay, int *enc_padding)
{
#ifdef HAVE_MPGLIB
    if (-1 == lame_decode_initfile(musicin, &global_decoder.mp3input_data, enc_delay, enc_padding)) {
        if (global_ui_config.silent < 10) {
            error_printf("Error reading headers in mp3 input file %s.\n", inPath);
        }
        return 0;
    }
#endif
    if (!set_input_num_channels(gfp, global_decoder.mp3input_data.stereo)) {
        return 0;
    }
    if (!set_input_samplerate(gfp, global_decoder.mp3input_data.samplerate)) {
        return 0;
    }
    (void) lame_set_num_samples(gfp, global_decoder.mp3input_data.nsamp);
    return 1;
}


static FILE *
open_wave_file(lame_t gfp, char const *inPath, int *enc_delay, int *enc_padding)
{
    FILE   *musicin;

    /* set the defaults from info incase we cannot determine them from file */
    lame_set_num_samples(gfp, MAX_U_32_NUM);

    if (!strcmp(inPath, "-")) {
        lame_set_stream_binary_mode(musicin = stdin); /* Read from standard input. */
    }
    else {
        if ((musicin = lame_fopen(inPath, "rb")) == NULL) {
            if (global_ui_config.silent < 10) {
                error_printf("Could not find \"%s\".\n", inPath);
            }
            return 0;
        }
    }

    if (global_reader.input_format == sf_ogg) {
        if (global_ui_config.silent < 10) {
            error_printf("sorry, vorbis support in LAME is deprecated.\n");
        }
        close_input_file(musicin);
        return 0;
    }
    else if (global_reader.input_format == sf_raw) {
        /* assume raw PCM */
        if (global_ui_config.silent < 9) {
            console_printf("Assuming raw pcm input file");
            if (global_reader.swapbytes)
                console_printf(" : Forcing byte-swapping\n");
            else
                console_printf("\n");
        }
        global. pcmswapbytes = global_reader.swapbytes;
    }
    else {
        global_reader.input_format = parse_file_header(gfp, musicin);
    }
    if (global_reader.input_format == sf_mp123) {
        if (open_mpeg_file_part2(gfp, musicin, inPath, enc_delay, enc_padding))
            return musicin;
        close_input_file(musicin);
        return 0;
    }
    if (global_reader.input_format == sf_unknown) {
        close_input_file(musicin);
        return 0;
    }

    if (lame_get_num_samples(gfp) == MAX_U_32_NUM && musicin != stdin) {
        int const tmp_num_channels = lame_get_num_channels(gfp);
        double const flen = lame_get_file_size(musicin); /* try to figure out num_samples */
        if (flen >= 0 && tmp_num_channels > 0 ) {
            /* try file size, assume 2 bytes per sample */
            unsigned long fsize = (unsigned long) (flen / (2 * tmp_num_channels));
            (void) lame_set_num_samples(gfp, fsize);
            global. count_samples_carefully = 0;
        }
    }
    return musicin;
}



static FILE *
open_mpeg_file(lame_t gfp, char const *inPath, int *enc_delay, int *enc_padding)
{
    FILE   *musicin;

    /* set the defaults from info incase we cannot determine them from file */
    lame_set_num_samples(gfp, MAX_U_32_NUM);

    if (strcmp(inPath, "-") == 0) {
        musicin = stdin;
        lame_set_stream_binary_mode(musicin); /* Read from standard input. */
    }
    else {
        musicin = lame_fopen(inPath, "rb");
        if (musicin == NULL) {
            if (global_ui_config.silent < 10) {
                error_printf("Could not find \"%s\".\n", inPath);
            }
            return 0;
        }
    }
#ifdef AMIGA_MPEGA
    if (-1 == lame_decode_initfile(inPath, &global_decoder.mp3input_data)) {
        if (global_ui_config.silent < 10) {
            error_printf("Error reading headers in mp3 input file %s.\n", inPath);
        }
        close_input_file(musicin);
        return 0;
    }
#endif
    if ( 0 == open_mpeg_file_part2(gfp, musicin, inPath, enc_delay, enc_padding) ) {
        close_input_file(musicin);
        return 0;
    }
    if (lame_get_num_samples(gfp) == MAX_U_32_NUM && musicin != stdin) {
        double  flen = lame_get_file_size(musicin); /* try to figure out num_samples */
        if (flen >= 0) {
            /* try file size, assume 2 bytes per sample */
            if (global_decoder.mp3input_data.bitrate > 0) {
                double  totalseconds =
                    (flen * 8.0 / (1000.0 * global_decoder.mp3input_data.bitrate));
                unsigned long tmp_num_samples =
                    (unsigned long) (totalseconds * lame_get_in_samplerate(gfp));

                (void) lame_set_num_samples(gfp, tmp_num_samples);
                global_decoder.mp3input_data.nsamp = tmp_num_samples;
                global. count_samples_carefully = 0;
            }
        }
    }
    return musicin;
}


static int
close_input_file(FILE * musicin)
{
    int     ret = 0;

    if (musicin != stdin && musicin != 0) {
        ret = fclose(musicin);
    }
    if (ret != 0) {
        if (global_ui_config.silent < 10) {
            error_printf("Could not close audio input file\n");
        }
    }
    return ret;
}



#if defined(HAVE_MPGLIB)
static int
check_aid(const unsigned char *header)
{
    return 0 == memcmp(header, "AiD\1", 4);
}

/*
 * Please check this and don't kill me if there's a bug
 * This is a (nearly?) complete header analysis for a MPEG-1/2/2.5 Layer I, II or III
 * data stream
 */

static int
is_syncword_mp123(const void *const headerptr)
{
    const unsigned char *const p = headerptr;
    static const char abl2[16] = { 0, 7, 7, 7, 0, 7, 0, 0, 0, 0, 0, 8, 8, 8, 8, 8 };

    if ((p[0] & 0xFF) != 0xFF)
        return 0;       /* first 8 bits must be '1' */
    if ((p[1] & 0xE0) != 0xE0)
        return 0;       /* next 3 bits are also */
    if ((p[1] & 0x18) == 0x08)
        return 0;       /* no MPEG-1, -2 or -2.5 */
    switch (p[1] & 0x06) {
    default:
    case 0x00:         /* illegal Layer */
        return 0;

    case 0x02:         /* Layer3 */
        if (global_reader.input_format != sf_mp3 && global_reader.input_format != sf_mp123) {
            return 0;
        }
        global_reader.input_format = sf_mp3;
        break;

    case 0x04:         /* Layer2 */
        if (global_reader.input_format != sf_mp2 && global_reader.input_format != sf_mp123) {
            return 0;
        }
        global_reader.input_format = sf_mp2;
        break;

    case 0x06:         /* Layer1 */
        if (global_reader.input_format != sf_mp1 && global_reader.input_format != sf_mp123) {
            return 0;
        }
        global_reader.input_format = sf_mp1;
        break;
    }
    if ((p[1] & 0x06) == 0x00)
        return 0;       /* no Layer I, II and III */
    if ((p[2] & 0xF0) == 0xF0)
        return 0;       /* bad bitrate */
    if ((p[2] & 0x0C) == 0x0C)
        return 0;       /* no sample frequency with (32,44.1,48)/(1,2,4)     */
    if ((p[1] & 0x18) == 0x18 && (p[1] & 0x06) == 0x04 && abl2[p[2] >> 4] & (1 << (p[3] >> 6)))
        return 0;
    if ((p[3] & 3) == 2)
        return 0;       /* reserved enphasis mode */
    return 1;
}

static size_t
lenOfId3v2Tag(unsigned char const* buf)
{
    unsigned int b0 = buf[0] & 127;
    unsigned int b1 = buf[1] & 127;
    unsigned int b2 = buf[2] & 127;
    unsigned int b3 = buf[3] & 127;
    return (((((b0 << 7) + b1) << 7) + b2) << 7) + b3;
}

int
lame_decode_initfile(FILE * fd, mp3data_struct * mp3data, int *enc_delay, int *enc_padding)
{
    /*  VBRTAGDATA pTagData; */
    /* int xing_header,len2,num_frames; */
    unsigned char buf[100];
    int     ret;
    size_t  len;
    int     aid_header;
    short int pcm_l[1152], pcm_r[1152];
    int     freeformat = 0;

    memset(mp3data, 0, sizeof(mp3data_struct));
    if (global.hip) {
        hip_decode_exit(global.hip);
    }
    global. hip = hip_decode_init();
    hip_set_msgf(global.hip, global_ui_config.silent < 10 ? &frontend_msgf : 0);
    hip_set_errorf(global.hip, global_ui_config.silent < 10 ? &frontend_errorf : 0);
    hip_set_debugf(global.hip, &frontend_debugf);

    len = 4;
    if (fread(buf, 1, len, fd) != len)
        return -1;      /* failed */
    while (buf[0] == 'I' && buf[1] == 'D' && buf[2] == '3') {
        len = 6;
        if (fread(&buf[4], 1, len, fd) != len)
            return -1;  /* failed */
        len = lenOfId3v2Tag(&buf[6]);
        if (global.in_id3v2_size < 1) {
            global.in_id3v2_size = 10 + len;
            global.in_id3v2_tag = malloc(global.in_id3v2_size);
            if (global.in_id3v2_tag) {
                memcpy(global.in_id3v2_tag, buf, 10);
                if (fread(&global.in_id3v2_tag[10], 1, len, fd) != len)
                    return -1;  /* failed */
                len = 0; /* copied, nothing to skip */
            }
            else {
                global.in_id3v2_size = 0;
            }
        }
        assert( len <= LONG_MAX );
        fskip(fd, (long) len, SEEK_CUR);
        len = 4;
        if (fread(&buf, 1, len, fd) != len)
            return -1;  /* failed */
    }
    aid_header = check_aid(buf);
    if (aid_header) {
        if (fread(&buf, 1, 2, fd) != 2)
            return -1;  /* failed */
        aid_header = (unsigned char) buf[0] + 256 * (unsigned char) buf[1];
        if (global_ui_config.silent < 9) {
            console_printf("Album ID found.  length=%i \n", aid_header);
        }
        /* skip rest of AID, except for 6 bytes we have already read */
        fskip(fd, aid_header - 6, SEEK_CUR);

        /* read 4 more bytes to set up buffer for MP3 header check */
        if (fread(&buf, 1, len, fd) != len)
            return -1;  /* failed */
    }
    len = 4;
    while (!is_syncword_mp123(buf)) {
        unsigned int i;
        for (i = 0; i < len - 1; i++)
            buf[i] = buf[i + 1];
        if (fread(buf + len - 1, 1, 1, fd) != 1)
            return -1;  /* failed */
    }

    if ((buf[2] & 0xf0) == 0) {
        if (global_ui_config.silent < 9) {
            console_printf("Input file is freeformat.\n");
        }
        freeformat = 1;
    }
    /* now parse the current buffer looking for MP3 headers.    */
    /* (as of 11/00: mpglib modified so that for the first frame where  */
    /* headers are parsed, no data will be decoded.   */
    /* However, for freeformat, we need to decode an entire frame, */
    /* so mp3data->bitrate will be 0 until we have decoded the first */
    /* frame.  Cannot decode first frame here because we are not */
    /* yet prepared to handle the output. */
    ret = hip_decode1_headersB(global.hip, buf, len, pcm_l, pcm_r, mp3data, enc_delay, enc_padding);
    if (-1 == ret)
        return -1;

    /* repeat until we decode a valid mp3 header.  */
    while (!mp3data->header_parsed) {
        len = fread(buf, 1, sizeof(buf), fd);
        if (len != sizeof(buf))
            return -1;
        ret =
            hip_decode1_headersB(global.hip, buf, len, pcm_l, pcm_r, mp3data, enc_delay,
                                 enc_padding);
        if (-1 == ret)
            return -1;
    }

    if (mp3data->bitrate == 0 && !freeformat) {
        if (global_ui_config.silent < 10) {
            error_printf("fail to sync...\n");
        }
        return lame_decode_initfile(fd, mp3data, enc_delay, enc_padding);
    }

    if (mp3data->totalframes > 0) {
        /* mpglib found a Xing VBR header and computed nsamp & totalframes */
    }
    else {
        /* set as unknown.  Later, we will take a guess based on file size
         * ant bitrate */
        mp3data->nsamp = MAX_U_32_NUM;
    }


    /*
       report_printf("ret = %i NEED_MORE=%i \n",ret,MP3_NEED_MORE);
       report_printf("stereo = %i \n",mp.fr.stereo);
       report_printf("samp = %i  \n",freqs[mp.fr.sampling_frequency]);
       report_printf("framesize = %i  \n",framesize);
       report_printf("bitrate = %i  \n",mp3data->bitrate);
       report_printf("num frames = %ui  \n",num_frames);
       report_printf("num samp = %ui  \n",mp3data->nsamp);
       report_printf("mode     = %i  \n",mp.fr.mode);
     */

    return 0;
}

/*
For lame_decode_fromfile:  return code
  -1     error
   n     number of samples output.  either 576 or 1152 depending on MP3 file.


For lame_decode1_headers():  return code
  -1     error
   0     ok, but need more data before outputing any samples
   n     number of samples output.  either 576 or 1152 depending on MP3 file.
*/
static int
lame_decode_fromfile(FILE * fd, short pcm_l[], short pcm_r[], mp3data_struct * mp3data)
{
    int     ret = 0;
    size_t  len = 0;
    unsigned char buf[1024];

    /* first see if we still have data buffered in the decoder: */
    ret = hip_decode1_headers(global.hip, buf, len, pcm_l, pcm_r, mp3data);
    if (ret != 0)
        return ret;


    /* read until we get a valid output frame */
    for (;;) {
        len = fread(buf, 1, 1024, fd);
        if (len == 0) {
            /* we are done reading the file, but check for buffered data */
            ret = hip_decode1_headers(global.hip, buf, len, pcm_l, pcm_r, mp3data);
            if (ret <= 0) {
                return -1; /* done with file */
            }
            break;
        }

        ret = hip_decode1_headers(global.hip, buf, len, pcm_l, pcm_r, mp3data);
        if (ret == -1) {
            return -1;
        }
        if (ret > 0)
            break;
    }
    return ret;
}
#endif /* defined(HAVE_MPGLIB) */


int
is_mpeg_file_format(int input_file_format)
{
    switch (input_file_format) {
    case sf_mp1:
        return 1;
    case sf_mp2:
        return 2;
    case sf_mp3:
        return 3;
    case sf_mp123:
        return -1;
    default:
        break;
    }
    return 0;
}


#define LOW__BYTE(x) (x & 0x00ff)
#define HIGH_BYTE(x) ((x >> 8) & 0x00ff)

void
put_audio16(FILE * outf, short Buffer[2][1152], int iread, int nch)
{
    char    data[2 * 1152 * 2];
    int     i, m = 0;

    if (global_decoder.disable_wav_header && global_reader.swapbytes) {
        if (nch == 1) {
            for (i = 0; i < iread; i++) {
                short   x = Buffer[0][i];
                /* write 16 Bits High Low */
                data[m++] = HIGH_BYTE(x);
                data[m++] = LOW__BYTE(x);
            }
        }
        else {
            for (i = 0; i < iread; i++) {
                short   x = Buffer[0][i], y = Buffer[1][i];
                /* write 16 Bits High Low */
                data[m++] = HIGH_BYTE(x);
                data[m++] = LOW__BYTE(x);
                /* write 16 Bits High Low */
                data[m++] = HIGH_BYTE(y);
                data[m++] = LOW__BYTE(y);
            }
        }
    }
    else {
        if (nch == 1) {
            for (i = 0; i < iread; i++) {
                short   x = Buffer[0][i];
                /* write 16 Bits Low High */
                data[m++] = LOW__BYTE(x);
                data[m++] = HIGH_BYTE(x);
            }
        }
        else {
            for (i = 0; i < iread; i++) {
                short   x = Buffer[0][i], y = Buffer[1][i];
                /* write 16 Bits Low High */
                data[m++] = LOW__BYTE(x);
                data[m++] = HIGH_BYTE(x);
                /* write 16 Bits Low High */
                data[m++] = LOW__BYTE(y);
                data[m++] = HIGH_BYTE(y);
            }
        }
    }
    if (m > 0) {
        fwrite(data, 1, m, outf);
    }
    if (global_writer.flush_write == 1) {
        fflush(outf);
    }
}

hip_t
get_hip(void)
{
    return global.hip;
}

size_t
sizeOfOldTag(lame_t gf)
{
    (void) gf;
    return global.in_id3v2_size;
}

unsigned char*
getOldTag(lame_t gf)
{
    (void) gf;
    return global.in_id3v2_tag;
}

/* end of get_audio.c */
