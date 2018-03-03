/*
 *      Command line frontend program
 *
 *      Copyright (c) 1999 Mark Taylor
 *                    2000 Takehiro TOMIANGA
 *                    2010-2011 Robert Hegemann
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

#ifndef MAIN_H_INCLUDED
#define MAIN_H_INCLUDED

#ifdef HAVE_LIMITS_H
# include <limits.h>
#endif

#include "get_audio.h"

#if defined(__cplusplus)
extern "C" {
#endif

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif


/* GLOBAL VARIABLES used by parse.c and main.c.  
   instantiated in parce.c.  ugly, ugly */

typedef struct ReaderConfig
{
    sound_file_format input_format;
    int   swapbytes;                /* force byte swapping   default=0 */
    int   swap_channel;             /* 0: no-op, 1: swaps input channels */
    int   input_samplerate;
    int   ignorewavlength;
} ReaderConfig;

typedef struct WriterConfig
{
    int   flush_write;
} WriterConfig;

typedef struct UiConfig
{
    int   silent;                   /* Verbosity */
    int   brhist;
    int   print_clipping_info;      /* print info whether waveform clips */
    float update_interval;          /* to use Frank's time status display */
} UiConfig;

typedef struct DecoderConfig
{
    int   mp3_delay;                /* to adjust the number of samples truncated during decode */
    int   mp3_delay_set;            /* user specified the value of the mp3 encoder delay to assume for decoding */
    int   disable_wav_header;
    mp3data_struct mp3input_data;
} DecoderConfig;

typedef enum ByteOrder { ByteOrderLittleEndian, ByteOrderBigEndian } ByteOrder;

typedef struct RawPCMConfig
{
    int     in_bitwidth;
    int     in_signed;
    ByteOrder in_endian;
} RawPCMConfig;

extern ReaderConfig global_reader;
extern WriterConfig global_writer;
extern UiConfig global_ui_config;
extern DecoderConfig global_decoder;
extern RawPCMConfig global_raw_pcm;


extern FILE* lame_fopen(char const* file, char const* mode);
extern char* utf8ToConsole8Bit(const char* str);
extern char* utf8ToLocal8Bit(const char* str);
extern unsigned short* utf8ToUtf16(char const* str);
extern char* utf8ToLatin1(char const* str);
#ifdef _WIN32
extern wchar_t* utf8ToUnicode(char const* str);
#endif

extern void dosToLongFileName(char* filename);
extern void setProcessPriority(int priority);

extern int lame_main(lame_t gf, int argc, char** argv);
extern char* lame_getenv(char const* var);

#if defined(__cplusplus)
}
#endif

#endif
