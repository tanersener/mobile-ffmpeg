/*
 *      mp3rtp command line frontend program
 *
 *      initially contributed by Felix von Leitner
 *
 *      Copyright (c) 2000 Mark Taylor
 *                    2010 Robert Hegemann
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

/* $Id: mp3rtp.c,v 1.37 2017/08/24 20:43:10 robert Exp $ */

/* Still under work ..., need a client for test, where can I get one? */

/* An audio player named Zinf (aka freeamp) can play rtp streams */

/* 
 *  experimental translation:
 *
 *  gcc -I..\include -I..\libmp3lame -o mp3rtp mp3rtp.c ../libmp3lame/libmp3lame.a lametime.c get_audio.c ieeefloat.c timestatus.c parse.c rtp.c -lm
 *
 *  wavrec -t 14400 -s 44100 -S /proc/self/fd/1 | ./mp3rtp 10.1.1.42 -V2 -b128 -B256 - my_mp3file.mp3
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef HAVE_STDINT_H
# include <stdint.h>
#endif

#ifdef STDC_HEADERS
# include <stdlib.h>
# include <string.h>
#endif

#include <time.h>

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "lame.h"
#include "main.h"
#include "parse.h"
#include "lametime.h"
#include "timestatus.h"
#include "get_audio.h"
#include "rtp.h"
#include "console.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

/*
 * Encode (via LAME) to mp3 with RTP streaming of the output.
 *
 * Author: Felix von Leitner <leitner@vim.org>
 *
 *   mp3rtp ip[:port[:ttl]] [lame encoding options] infile outfile
 *
 * examples:
 *   arecord -b 16 -s 22050 -w | ./mp3rtp 224.17.23.42:5004:2 -b 56 - /dev/null
 *   arecord -b 16 -s 44100 -w | ./mp3rtp 10.1.1.42 -V2 -b128 -B256 - my_mp3file.mp3
 *
 */


static unsigned int
maxvalue(int Buffer[2][1152])
{
    int     max = 0;
    int     i;

    for (i = 0; i < 1152; i++) {
        if (abs(Buffer[0][i]) > max)
            max = abs(Buffer[0][i]);
        if (abs(Buffer[1][i]) > max)
            max = abs(Buffer[1][i]);
    }
    return max >> 16;
}

static void
levelmessage(unsigned int maxv, int* maxx, int* tmpx)
{
    char    buff[] = "|  .  |  .  |  .  |  .  |  .  |  .  |  .  |  .  |  .  |  .  |  \r";
    int     tmp = *tmpx, max = *maxx;

    buff[tmp] = '+';
    tmp = (maxv * 61 + 16384) / (32767 + 16384 / 61);
    if (tmp > sizeof(buff) - 2)
        tmp = sizeof(buff) - 2;
    if (max < tmp)
        max = tmp;
    buff[max] = 'x';
    buff[tmp] = '#';
    console_printf(buff);
    console_flush();
    *maxx = max;
    *tmpx = tmp;
}


/************************************************************************
*
* main
*
* PURPOSE:  MPEG-1,2 Layer III encoder with GPSYCHO 
* psychoacoustic model.
*
************************************************************************/

int
lame_main(lame_t gf, int argc, char **argv)
{
    unsigned char mp3buffer[LAME_MAXMP3BUFFER];
    char    inPath[PATH_MAX + 1];
    char    outPath[PATH_MAX + 1];
    int     Buffer[2][1152];

    int     maxx = 0, tmpx = 0;
    int     ret;
    int     wavsamples;
    int     mp3bytes;
    FILE   *outf;

    char    ip[16];
    unsigned int port = 5004;
    unsigned int ttl = 2;
    char    dummy;

    if (argc <= 2) {
        console_printf("Encode (via LAME) to mp3 with RTP streaming of the output\n"
                       "\n"
                       "    mp3rtp ip[:port[:ttl]] [lame encoding options] infile outfile\n"
                       "\n"
                       "    examples:\n"
                       "      arecord -b 16 -s 22050 -w | ./mp3rtp 224.17.23.42:5004:2 -b 56 - /dev/null\n"
                       "      arecord -b 16 -s 44100 -w | ./mp3rtp 10.1.1.42 -V2 -b128 -B256 - my_mp3file.mp3\n"
                       "\n");
        return 1;
    }

    switch (sscanf(argv[1], "%11[.0-9]:%u:%u%c", ip, &port, &ttl, &dummy)) {
    case 1:
    case 2:
    case 3:
        break;
    default:
        error_printf("Illegal destination selector '%s', must be ip[:port[:ttl]]\n", argv[1]);
        return -1;
    }
    rtp_initialization();
    if (rtp_socket(ip, port, ttl)) {
        rtp_deinitialization();
        error_printf("fatal error during initialization\n");
        return 1;
    }

    lame_set_errorf(gf, &frontend_errorf);
    lame_set_debugf(gf, &frontend_debugf);
    lame_set_msgf(gf, &frontend_msgf);

    /* Remove the argumets that are rtp related, and then 
     * parse the command line arguments, setting various flags in the
     * struct pointed to by 'gf'.  If you want to parse your own arguments,
     * or call libmp3lame from a program which uses a GUI to set arguments,
     * skip this call and set the values of interest in the gf struct.  
     * (see lame.h for documentation about these parameters)
     */
    {
        int     i;
        int     argc_mod = argc-1; /* leaving out exactly one argument */
        char**  argv_mod = calloc(argc_mod, sizeof(char*));
        argv_mod[0] = argv[0];
        for (i = 2; i < argc; ++i) { /* leaving out argument number 1, parsed above */
            argv_mod[i-1] = argv[i];
        }
        parse_args(gf, argc_mod, argv_mod, inPath, outPath, NULL, NULL);
        free(argv_mod);
    }

    /* open the output file.  Filename parsed into gf.inPath */
    if (0 == strcmp(outPath, "-")) {
        lame_set_stream_binary_mode(outf = stdout);
    }
    else {
        if ((outf = lame_fopen(outPath, "wb+")) == NULL) {
            rtp_deinitialization();
            error_printf("Could not create \"%s\".\n", outPath);
            return 1;
        }
    }


    /* open the wav/aiff/raw pcm or mp3 input file.  This call will
     * open the file with name gf.inFile, try to parse the headers and
     * set gf.samplerate, gf.num_channels, gf.num_samples.
     * if you want to do your own file input, skip this call and set
     * these values yourself.  
     */
    if (init_infile(gf, inPath) < 0) {
        rtp_deinitialization();
        fclose(outf);
        error_printf("Can't init infile '%s'\n", inPath);
        return 1;
    }


    /* Now that all the options are set, lame needs to analyze them and
     * set some more options 
     */
    ret = lame_init_params(gf);
    if (ret < 0) {
        if (ret == -1)
            display_bitrates(stderr);
        rtp_deinitialization();
        fclose(outf);
        close_infile();
        error_printf("fatal error during initialization\n");
        return -1;
    }

    lame_print_config(gf); /* print useful information about options being used */

    if (global_ui_config.update_interval < 0.)
        global_ui_config.update_interval = 2.;

    /* encode until we hit EOF */
    while ((wavsamples = get_audio(gf, Buffer)) > 0) { /* read in 'wavsamples' samples */
        levelmessage(maxvalue(Buffer), &maxx, &tmpx);
        mp3bytes = lame_encode_buffer_int(gf, /* encode the frame */
                                          Buffer[0], Buffer[1], wavsamples,
                                          mp3buffer, sizeof(mp3buffer));
        rtp_output(mp3buffer, mp3bytes); /* write MP3 output to RTP port */
        fwrite(mp3buffer, 1, mp3bytes, outf); /* write the MP3 output to file */
    }

    mp3bytes = lame_encode_flush(gf, /* may return one or more mp3 frame */
                                 mp3buffer, sizeof(mp3buffer));
    rtp_output(mp3buffer, mp3bytes); /* write MP3 output to RTP port */
    fwrite(mp3buffer, 1, mp3bytes, outf); /* write the MP3 output to file */

    lame_mp3_tags_fid(gf, outf); /* add VBR tags to mp3 file */

    rtp_deinitialization();
    fclose(outf);
    close_infile();     /* close the sound input file */
    return 0;
}

/* end of mp3rtp.c */
