/*
 *    Get Audio routines include file
 *
 *    Copyright (c) 1999 Albert L Faber
 *                  2010 Robert Hegemann
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


#ifndef LAME_GET_AUDIO_H
#define LAME_GET_AUDIO_H
#include "lame.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum sound_file_format_e {
    sf_unknown,
    sf_raw,
    sf_wave,
    sf_aiff,
    sf_mp1,                  /* MPEG Layer 1, aka mpg */
    sf_mp2,                  /* MPEG Layer 2 */
    sf_mp3,                  /* MPEG Layer 3 */
    sf_mp123,                /* MPEG Layer 1,2 or 3; whatever .mp3, .mp2, .mp1 or .mpg contains */
    sf_ogg
} sound_file_format;

int     is_mpeg_file_format( int input_format );

int     init_infile(lame_t gfp, char const * inPath);
int     samples_to_skip_at_start(void);
int     samples_to_skip_at_end(void);
void    close_infile(void);
int     get_audio(lame_t gfp, int buffer[2][1152]);
int     get_audio16(lame_t gfp, short buffer[2][1152]);
int     get_audio_float(lame_t gfp, float buffer[2][1152]);
int     get_audio_double(lame_t gfp, double buffer[2][1152]);
hip_t   get_hip(void);

FILE   *init_outfile(char const *outPath, int decode);
int     WriteWaveHeader(FILE * const fp, int pcmbytes, int freq, int channels, int bits);
void    put_audio16(FILE* outf, short Buffer[2][1152], int iread, int nch);

/*
struct AudioReader;
typedef struct AudioReader* AudioReader;

AudioReader ar_open(lame_t gfp, char const* inPath);
int     ar_samplesToSkipAtStart(AudioReader ar);
int     ar_samplesToSkipAtEnd(AudioReader ar);
void    ar_close(AudioReader ar);
int     ar_readInt(AudioReader ar, lame_t gfp, int buffer[2][1152]);
int     ar_readShort(AudioReader ar, lame_t gfp, short buffer[2][1152]);
int     ar_readFloat(AudioReader ar, lame_t gfp, float buffer[2][1152]);

struct AudioWriter;
typedef struct AudioWriter* AudioWriter;

AudioWriter aw_open(lame_t gfp, char const* outPath, int pcmbystes, int freq, int channels, int bits);
int     aw_writeWaveHeader(AudioWriter aw);
int     aw_write(AudioWriter aw, short buffer[2][1152], int n);
int     aw_write(AudioWriter aw, float buffer[2][1152], int n);

*/

extern size_t sizeOfOldTag(lame_t gf);
extern unsigned char* getOldTag(lame_t gf);

#ifdef _cplusplus
}
#endif

#endif /* ifndef LAME_GET_AUDIO_H */
