/*
 *      time status related function source file
 *
 *      Copyright (c) 1999 Mark Taylor
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

/* $Id: timestatus.c,v 1.61 2013/03/20 20:38:43 robert Exp $ */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif


#if 1
# define SPEED_CHAR     "x" /* character x */
# define SPEED_MULT     1.
#else
# define SPEED_CHAR     "%%"
# define SPEED_MULT     100.
#endif

#include <assert.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

#include "lame.h"
#include "main.h"
#include "lametime.h"
#include "timestatus.h"
#include "brhist.h"
#include "console.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

typedef struct time_status_struct {
    double  last_time;       /* result of last call to clock */
    double  elapsed_time;    /* total time */
    double  estimated_time;  /* estimated total duration time [s] */
    double  speed_index;     /* speed relative to realtime coding [100%] */
} timestatus_t;

static struct EncoderProgress {
    timestatus_t real_time;
    timestatus_t proc_time;
    double  last_time;
    int     last_frame_num;
    int     time_status_init;
} global_encoder_progress;


/*
 *  Calculates from the input (see below) the following values:
 *    - total estimated time
 *    - a speed index
 */

static void
ts_calc_times(timestatus_t * const tstime, /* tstime->elapsed_time: elapsed time */
              const int sample_freq, /* sample frequency [Hz/kHz]  */
              const int frameNum, /* Number of the current Frame */
              const int totalframes, /* total umber of Frames */
              const int framesize)
{                       /* Size of a frame [bps/kbps] */
    assert(sample_freq >= 8000 && sample_freq <= 48000);

    if (frameNum > 0 && tstime->elapsed_time > 0) {
        tstime->estimated_time = tstime->elapsed_time * totalframes / frameNum;
        tstime->speed_index = framesize * frameNum / (sample_freq * tstime->elapsed_time);
    }
    else {
        tstime->estimated_time = 0.;
        tstime->speed_index = 0.;
    }
}

/* Decomposes a given number of seconds into a easy to read hh:mm:ss format
 * padded with an additional character
 */

static void
ts_time_decompose(const double x, const char padded_char)
{
    const unsigned long time_in_sec = (unsigned long)x;
    const unsigned long hour = time_in_sec / 3600;
    const unsigned int min = time_in_sec / 60 % 60;
    const unsigned int sec = time_in_sec % 60;

    if (hour == 0)
        console_printf("   %2u:%02u%c", min, sec, padded_char);
    else if (hour < 100)
        console_printf("%2lu:%02u:%02u%c", hour, min, sec, padded_char);
    else
        console_printf("%6lu h%c", hour, padded_char);
}

static void
timestatus(const lame_global_flags * const gfp)
{
    timestatus_t* real_time = &global_encoder_progress.real_time;
    timestatus_t* proc_time = &global_encoder_progress.proc_time;
    int     percent;
    double  tmx, delta;
    int samp_rate     = lame_get_out_samplerate(gfp)
      , frameNum      = lame_get_frameNum(gfp)
      , totalframes   = lame_get_totalframes(gfp)
      , framesize     = lame_get_framesize(gfp)
      ;

    if (totalframes < frameNum) {
        totalframes = frameNum;
    }
    if (global_encoder_progress.time_status_init == 0) {
        real_time->last_time = GetRealTime();
        proc_time->last_time = GetCPUTime();
        real_time->elapsed_time = 0;
        proc_time->elapsed_time = 0;
    }

    /* we need rollover protection for GetCPUTime, and maybe GetRealTime(): */
    tmx = GetRealTime();
    delta = tmx - real_time->last_time;
    if (delta < 0)
        delta = 0;      /* ignore, clock has rolled over */
    real_time->elapsed_time += delta;
    real_time->last_time = tmx;


    tmx = GetCPUTime();
    delta = tmx - proc_time->last_time;
    if (delta < 0)
        delta = 0;      /* ignore, clock has rolled over */
    proc_time->elapsed_time += delta;
    proc_time->last_time = tmx;

    if (global_encoder_progress.time_status_init == 0) {
        console_printf("\r"
                       "    Frame          |  CPU time/estim | REAL time/estim | play/CPU |    ETA \n"
                       "     0/       ( 0%%)|    0:00/     :  |    0:00/     :  |         "
                       SPEED_CHAR "|     :  \r"
                       /* , Console_IO.str_clreoln, Console_IO.str_clreoln */ );
        global_encoder_progress.time_status_init = 1;
        return;
    }

    ts_calc_times(real_time, samp_rate, frameNum, totalframes, framesize);
    ts_calc_times(proc_time, samp_rate, frameNum, totalframes, framesize);

    if (frameNum < totalframes) {
        percent = (int) (100. * frameNum / totalframes + 0.5);
    }
    else {
        percent = 100;
    }

    console_printf("\r%6i/%-6i", frameNum, totalframes);
    console_printf(percent < 100 ? " (%2d%%)|" : "(%3.3d%%)|", percent);
    ts_time_decompose(proc_time->elapsed_time, '/');
    ts_time_decompose(proc_time->estimated_time, '|');
    ts_time_decompose(real_time->elapsed_time, '/');
    ts_time_decompose(real_time->estimated_time, '|');
    console_printf(proc_time->speed_index <= 1. ?
                   "%9.4f" SPEED_CHAR "|" : "%#9.5g" SPEED_CHAR "|",
                   SPEED_MULT * proc_time->speed_index);
    ts_time_decompose((real_time->estimated_time - real_time->elapsed_time), ' ');
}

static void
timestatus_finish(void)
{
    console_printf("\n");
}


static void
brhist_init_package(lame_global_flags const* gf)
{
    if (global_ui_config.brhist) {
        if (brhist_init(gf, lame_get_VBR_min_bitrate_kbps(gf), lame_get_VBR_max_bitrate_kbps(gf))) {
            /* fail to initialize */
            global_ui_config.brhist = 0;
        }
    }
    else {
        brhist_init(gf, 128, 128); /* Dirty hack */
    }
}


void
encoder_progress_begin( lame_global_flags const* gf
                      , char              const* inPath
                      , char              const* outPath
                      )
{
    brhist_init_package(gf);
    global_encoder_progress.time_status_init = 0;
    global_encoder_progress.last_time = 0;
    global_encoder_progress.last_frame_num = 0;
    if (global_ui_config.silent < 9) {
        char* i_file = 0;
        char* o_file = 0;
#if defined( _WIN32 ) && !defined(__MINGW32__)
        inPath = i_file = utf8ToConsole8Bit(inPath);
        outPath = o_file = utf8ToConsole8Bit(outPath);
#endif
        lame_print_config(gf); /* print useful information about options being used */

        console_printf("Encoding %s%s to %s\n",
                       strcmp(inPath, "-") ? inPath : "<stdin>",
                       strlen(inPath) + strlen(outPath) < 66 ? "" : "\n     ",
                       strcmp(outPath, "-") ? outPath : "<stdout>");

        free(i_file);
        free(o_file);

        console_printf("Encoding as %g kHz ", 1.e-3 * lame_get_out_samplerate(gf));

        {
            static const char *mode_names[2][4] = {
                {"stereo", "j-stereo", "dual-ch", "single-ch"},
                {"stereo", "force-ms", "dual-ch", "single-ch"}
            };
            switch (lame_get_VBR(gf)) {
            case vbr_rh:
                console_printf("%s MPEG-%u%s Layer III VBR(q=%g) qval=%i\n",
                               mode_names[lame_get_force_ms(gf)][lame_get_mode(gf)],
                               2 - lame_get_version(gf),
                               lame_get_out_samplerate(gf) < 16000 ? ".5" : "",
                               lame_get_VBR_quality(gf),
                               lame_get_quality(gf));
                break;
            case vbr_mt:
            case vbr_mtrh:
                console_printf("%s MPEG-%u%s Layer III VBR(q=%g)\n",
                               mode_names[lame_get_force_ms(gf)][lame_get_mode(gf)],
                               2 - lame_get_version(gf),
                               lame_get_out_samplerate(gf) < 16000 ? ".5" : "",
                               lame_get_VBR_quality(gf));
                break;
            case vbr_abr:
                console_printf("%s MPEG-%u%s Layer III (%gx) average %d kbps qval=%i\n",
                               mode_names[lame_get_force_ms(gf)][lame_get_mode(gf)],
                               2 - lame_get_version(gf),
                               lame_get_out_samplerate(gf) < 16000 ? ".5" : "",
                               0.1 * (int) (10. * lame_get_compression_ratio(gf) + 0.5),
                               lame_get_VBR_mean_bitrate_kbps(gf),
                               lame_get_quality(gf));
                break;
            default:
                console_printf("%s MPEG-%u%s Layer III (%gx) %3d kbps qval=%i\n",
                               mode_names[lame_get_force_ms(gf)][lame_get_mode(gf)],
                               2 - lame_get_version(gf),
                               lame_get_out_samplerate(gf) < 16000 ? ".5" : "",
                               0.1 * (int) (10. * lame_get_compression_ratio(gf) + 0.5),
                               lame_get_brate(gf),
                               lame_get_quality(gf));
                break;
            }
        }

        if (global_ui_config.silent <= -10) {
            lame_print_internals(gf);
        }
    }
}

void
encoder_progress( lame_global_flags const* gf )
{
    if (global_ui_config.silent <= 0) {
        int const frames = lame_get_frameNum(gf);
        int const frames_diff = frames - global_encoder_progress.last_frame_num;
        if (global_ui_config.update_interval <= 0) {     /*  most likely --disptime x not used */
            if (frames_diff < 100 && frames_diff != 0) {  /*  true, most of the time */
                return;
            }
            global_encoder_progress.last_frame_num = (frames/100)*100;
        }
        else {
            if (frames != 0 && frames != 9) {
                double const act = GetRealTime();
                double const dif = act - global_encoder_progress.last_time;
                if (dif >= 0 && dif < global_ui_config.update_interval) {
                    return;
                }
            }
            global_encoder_progress.last_time = GetRealTime(); /* from now! disp_time seconds */
        }
        if (global_ui_config.brhist) {
            brhist_jump_back();
        }
        timestatus(gf);
        if (global_ui_config.brhist) {
            brhist_disp(gf);
        }
        console_flush();
    }
}

void
encoder_progress_end( lame_global_flags const* gf )
{
    if (global_ui_config.silent <= 0) {
        if (global_ui_config.brhist) {
            brhist_jump_back();
        }
        timestatus(gf);
        if (global_ui_config.brhist) {
            brhist_disp(gf);
        }
        timestatus_finish();
    }
}


/* these functions are used in get_audio.c */
static struct DecoderProgress {
    int     last_mode_ext;
    int     frames_total;
    int     frame_ctr;
    int     framesize;
    unsigned long samples;
} global_decoder_progress;

static
unsigned long calcEndPadding(unsigned long samples, int pcm_samples_per_frame)
{
    unsigned long end_padding;
    samples += 576;
    end_padding = pcm_samples_per_frame - (samples % pcm_samples_per_frame);
    if (end_padding < 576)
        end_padding += pcm_samples_per_frame;
    return end_padding;
}

static
unsigned long calcNumBlocks(unsigned long samples, int pcm_samples_per_frame)
{
    unsigned long end_padding;
    samples += 576;
    end_padding = pcm_samples_per_frame - (samples % pcm_samples_per_frame);
    if (end_padding < 576)
        end_padding += pcm_samples_per_frame;
    return (samples + end_padding) / pcm_samples_per_frame;
}

DecoderProgress
decoder_progress_init(unsigned long n, int framesize)
{
    DecoderProgress dp = &global_decoder_progress;
    dp->last_mode_ext =0;
    dp->frames_total = 0;
    dp->frame_ctr = 0;
    dp->framesize = framesize;
    dp->samples = 0;
    if (n != (0ul-1ul)) {
        if (framesize == 576 || framesize == 1152) {
            dp->frames_total = calcNumBlocks(n, framesize);
            dp->samples = 576 + calcEndPadding(n, framesize);
        }
        else if (framesize > 0) {
            dp->frames_total = n / framesize;
        }
        else {
            dp->frames_total = n;
        }
    }
    return dp;
}

static void
addSamples(DecoderProgress dp, int iread)
{
    dp->samples += iread;
    dp->frame_ctr += dp->samples / dp->framesize;
    dp->samples %= dp->framesize;
    if (dp->frames_total < dp->frame_ctr) {
        dp->frames_total = dp->frame_ctr;
    }
}

void
decoder_progress(DecoderProgress dp, const mp3data_struct * mp3data, int iread)
{
    addSamples(dp, iread);

    console_printf("\rFrame#%6i/%-6i %3i kbps",
                   dp->frame_ctr, dp->frames_total, mp3data->bitrate);

    /* Programmed with a single frame hold delay */
    /* Attention: static data */

    /* MP2 Playback is still buggy. */
    /* "'00' subbands 4-31 in intensity_stereo, bound==4" */
    /* is this really intensity_stereo or is it MS stereo? */

    if (mp3data->mode == JOINT_STEREO) {
        int     curr = mp3data->mode_ext;
        int     last = dp->last_mode_ext;
        console_printf("  %s  %c",
                       curr & 2 ? last & 2 ? " MS " : "LMSR" : last & 2 ? "LMSR" : "L  R",
                       curr & 1 ? last & 1 ? 'I' : 'i' : last & 1 ? 'i' : ' ');
        dp->last_mode_ext = curr;
    }
    else {
        console_printf("         ");
        dp->last_mode_ext = 0;
    }
/*    console_printf ("%s", Console_IO.str_clreoln ); */
    console_printf("        \b\b\b\b\b\b\b\b");
    console_flush();
}

void
decoder_progress_finish(DecoderProgress dp)
{
    (void) dp;
    console_printf("\n");
}
