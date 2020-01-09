////////////////////////////////////////////////////////////////////////////
//                           **** WAVPACK ****                            //
//                  Hybrid Lossless Wavefile Compressor                   //
//                Copyright (c) 1998 - 2019 David Bryant.                 //
//                          All Rights Reserved.                          //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

// riff.c

// This module is a helper to the WavPack command-line programs to support WAV files
// (both MS standard and rf64 varients).

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "wavpack.h"
#include "utils.h"

#pragma pack(push,4)

typedef struct {
    char ckID [4];
    uint64_t chunkSize64;
} CS64Chunk;

typedef struct {
    uint64_t riffSize64, dataSize64, sampleCount64;
    uint32_t tableLength;
} DS64Chunk;

typedef struct {
    char ckID [4];
    uint32_t ckSize;
    char junk [28];
} JunkChunk;

#pragma pack(pop)

#define CS64ChunkFormat "4D"
#define DS64ChunkFormat "DDDL"

#define WAVPACK_NO_ERROR    0
#define WAVPACK_SOFT_ERROR  1
#define WAVPACK_HARD_ERROR  2

extern int debug_logging_mode;

int ParseRiffHeaderConfig (FILE *infile, char *infilename, char *fourcc, WavpackContext *wpc, WavpackConfig *config)
{
    int is_rf64 = !strncmp (fourcc, "RF64", 4), got_ds64 = 0, format_chunk = 0;
    int64_t total_samples = 0, infilesize;
    RiffChunkHeader riff_chunk_header;
    ChunkHeader chunk_header;
    WaveHeader WaveHeader;
    DS64Chunk ds64_chunk;
    uint32_t bcount;

    CLEAR (WaveHeader);
    CLEAR (ds64_chunk);
    infilesize = DoGetFileSize (infile);

    if (!is_rf64 && infilesize >= 4294967296LL && !(config->qmode & QMODE_IGNORE_LENGTH)) {
        error_line ("can't handle .WAV files larger than 4 GB (non-standard)!");
        return WAVPACK_SOFT_ERROR;
    }

    memcpy (&riff_chunk_header, fourcc, 4);

    if ((!DoReadFile (infile, ((char *) &riff_chunk_header) + 4, sizeof (RiffChunkHeader) - 4, &bcount) ||
        bcount != sizeof (RiffChunkHeader) - 4 || strncmp (riff_chunk_header.formType, "WAVE", 4))) {
            error_line ("%s is not a valid .WAV file!", infilename);
            return WAVPACK_SOFT_ERROR;
    }
    else if (!(config->qmode & QMODE_NO_STORE_WRAPPER) &&
        !WavpackAddWrapper (wpc, &riff_chunk_header, sizeof (RiffChunkHeader))) {
            error_line ("%s", WavpackGetErrorMessage (wpc));
            return WAVPACK_SOFT_ERROR;
    }

    // loop through all elements of the RIFF wav header
    // (until the data chuck) and copy them to the output file

    while (1) {
        if (!DoReadFile (infile, &chunk_header, sizeof (ChunkHeader), &bcount) ||
            bcount != sizeof (ChunkHeader)) {
                error_line ("%s is not a valid .WAV file!", infilename);
                return WAVPACK_SOFT_ERROR;
        }
        else if (!(config->qmode & QMODE_NO_STORE_WRAPPER) &&
            !WavpackAddWrapper (wpc, &chunk_header, sizeof (ChunkHeader))) {
                error_line ("%s", WavpackGetErrorMessage (wpc));
                return WAVPACK_SOFT_ERROR;
        }

        WavpackLittleEndianToNative (&chunk_header, ChunkHeaderFormat);

        if (!strncmp (chunk_header.ckID, "ds64", 4)) {
            if (chunk_header.ckSize < sizeof (DS64Chunk) ||
                !DoReadFile (infile, &ds64_chunk, sizeof (DS64Chunk), &bcount) ||
                bcount != sizeof (DS64Chunk)) {
                    error_line ("%s is not a valid .WAV file!", infilename);
                    return WAVPACK_SOFT_ERROR;
            }
            else if (!(config->qmode & QMODE_NO_STORE_WRAPPER) &&
                !WavpackAddWrapper (wpc, &ds64_chunk, sizeof (DS64Chunk))) {
                    error_line ("%s", WavpackGetErrorMessage (wpc));
                    return WAVPACK_SOFT_ERROR;
            }

            got_ds64 = 1;
            WavpackLittleEndianToNative (&ds64_chunk, DS64ChunkFormat);

            if (debug_logging_mode)
                error_line ("DS64: riffSize = %lld, dataSize = %lld, sampleCount = %lld, table_length = %d",
                    (long long) ds64_chunk.riffSize64, (long long) ds64_chunk.dataSize64,
                    (long long) ds64_chunk.sampleCount64, ds64_chunk.tableLength);

            if (ds64_chunk.tableLength * sizeof (CS64Chunk) != chunk_header.ckSize - sizeof (DS64Chunk)) {
                error_line ("%s is not a valid .WAV file!", infilename);
                return WAVPACK_SOFT_ERROR;
            }

            while (ds64_chunk.tableLength--) {
                CS64Chunk cs64_chunk;
                if (!DoReadFile (infile, &cs64_chunk, sizeof (CS64Chunk), &bcount) ||
                    bcount != sizeof (CS64Chunk) ||
                    (!(config->qmode & QMODE_NO_STORE_WRAPPER) &&
                    !WavpackAddWrapper (wpc, &cs64_chunk, sizeof (CS64Chunk)))) {
                        error_line ("%s", WavpackGetErrorMessage (wpc));
                        return WAVPACK_SOFT_ERROR;
                }
            }
        }
        else if (!strncmp (chunk_header.ckID, "fmt ", 4)) {     // if it's the format chunk, we want to get some info out of there and
            int supported = TRUE, format;                        // make sure it's a .wav file we can handle

            if (format_chunk++) {
                error_line ("%s is not a valid .WAV file!", infilename);
                return WAVPACK_SOFT_ERROR;
            }

            if (chunk_header.ckSize < 16 || chunk_header.ckSize > sizeof (WaveHeader) ||
                !DoReadFile (infile, &WaveHeader, chunk_header.ckSize, &bcount) ||
                bcount != chunk_header.ckSize) {
                    error_line ("%s is not a valid .WAV file!", infilename);
                    return WAVPACK_SOFT_ERROR;
            }
            else if (!(config->qmode & QMODE_NO_STORE_WRAPPER) &&
                !WavpackAddWrapper (wpc, &WaveHeader, chunk_header.ckSize)) {
                    error_line ("%s", WavpackGetErrorMessage (wpc));
                    return WAVPACK_SOFT_ERROR;
            }

            WavpackLittleEndianToNative (&WaveHeader, WaveHeaderFormat);

            if (debug_logging_mode) {
                error_line ("format tag size = %d", chunk_header.ckSize);
                error_line ("FormatTag = %x, NumChannels = %d, BitsPerSample = %d",
                    WaveHeader.FormatTag, WaveHeader.NumChannels, WaveHeader.BitsPerSample);
                error_line ("BlockAlign = %d, SampleRate = %d, BytesPerSecond = %d",
                    WaveHeader.BlockAlign, WaveHeader.SampleRate, WaveHeader.BytesPerSecond);

                if (chunk_header.ckSize > 16)
                    error_line ("cbSize = %d, ValidBitsPerSample = %d", WaveHeader.cbSize,
                        WaveHeader.ValidBitsPerSample);

                if (chunk_header.ckSize > 20)
                    error_line ("ChannelMask = %x, SubFormat = %d",
                        WaveHeader.ChannelMask, WaveHeader.SubFormat);
            }

            if (chunk_header.ckSize > 16 && WaveHeader.cbSize == 2)
                config->qmode |= QMODE_ADOBE_MODE;

            format = (WaveHeader.FormatTag == 0xfffe && chunk_header.ckSize == 40) ?
                WaveHeader.SubFormat : WaveHeader.FormatTag;

            config->bits_per_sample = (chunk_header.ckSize == 40 && WaveHeader.ValidBitsPerSample) ?
                WaveHeader.ValidBitsPerSample : WaveHeader.BitsPerSample;

            if (format != 1 && format != 3)
                supported = FALSE;

            if (format == 3 && config->bits_per_sample != 32)
                supported = FALSE;

            if (!WaveHeader.NumChannels || WaveHeader.NumChannels > 256 ||
                WaveHeader.BlockAlign / WaveHeader.NumChannels < (config->bits_per_sample + 7) / 8 ||
                WaveHeader.BlockAlign / WaveHeader.NumChannels > 4 ||
                WaveHeader.BlockAlign % WaveHeader.NumChannels)
                    supported = FALSE;

            if (config->bits_per_sample < 1 || config->bits_per_sample > 32)
                supported = FALSE;

            if (!supported) {
                error_line ("%s is an unsupported .WAV format!", infilename);
                return WAVPACK_SOFT_ERROR;
            }

            if (chunk_header.ckSize < 40) {
                if (!config->channel_mask && !(config->qmode & QMODE_CHANS_UNASSIGNED)) {
                    if (WaveHeader.NumChannels <= 2)
                        config->channel_mask = 0x5 - WaveHeader.NumChannels;
                    else if (WaveHeader.NumChannels <= 18)
                        config->channel_mask = (1 << WaveHeader.NumChannels) - 1;
                    else
                        config->channel_mask = 0x3ffff;
                }
            }
            else if (WaveHeader.ChannelMask && (config->channel_mask || (config->qmode & QMODE_CHANS_UNASSIGNED))) {
                error_line ("this WAV file already has channel order information!");
                return WAVPACK_SOFT_ERROR;
            }
            else if (WaveHeader.ChannelMask)
                config->channel_mask = WaveHeader.ChannelMask;

            if (format == 3)
                config->float_norm_exp = 127;
            else if ((config->qmode & QMODE_ADOBE_MODE) &&
                WaveHeader.BlockAlign / WaveHeader.NumChannels == 4) {
                    if (WaveHeader.BitsPerSample == 24)
                        config->float_norm_exp = 127 + 23;
                    else if (WaveHeader.BitsPerSample == 32)
                        config->float_norm_exp = 127 + 15;
            }

            if (debug_logging_mode) {
                if (config->float_norm_exp == 127)
                    error_line ("data format: normalized 32-bit floating point");
                else if (config->float_norm_exp)
                    error_line ("data format: 32-bit floating point (Audition %d:%d float type 1)",
                        config->float_norm_exp - 126, 150 - config->float_norm_exp);
                else
                    error_line ("data format: %d-bit integers stored in %d byte(s)",
                        config->bits_per_sample, WaveHeader.BlockAlign / WaveHeader.NumChannels);
            }
        }
        else if (!strncmp (chunk_header.ckID, "data", 4)) {             // on the data chunk, get size and exit loop

            int64_t data_chunk_size = (got_ds64 && chunk_header.ckSize == (uint32_t) -1) ?
                ds64_chunk.dataSize64 : chunk_header.ckSize;


            if (!WaveHeader.NumChannels || (is_rf64 && !got_ds64)) {   // make sure we saw "fmt" and "ds64" chunks (if required)
                error_line ("%s is not a valid .WAV file!", infilename);
                return WAVPACK_SOFT_ERROR;
            }

            if (infilesize && !(config->qmode & QMODE_IGNORE_LENGTH) && infilesize - data_chunk_size > 16777216) {
                error_line ("this .WAV file has over 16 MB of extra RIFF data, probably is corrupt!");
                return WAVPACK_SOFT_ERROR;
            }

            if (config->qmode & QMODE_IGNORE_LENGTH) {
                if (infilesize && DoGetFilePosition (infile) != -1) {
                    total_samples = (infilesize - DoGetFilePosition (infile)) / WaveHeader.BlockAlign;

                    if ((infilesize - DoGetFilePosition (infile)) % WaveHeader.BlockAlign)
                        error_line ("warning: audio length does not divide evenly, %d bytes will be discarded!",
                            (int)((infilesize - DoGetFilePosition (infile)) % WaveHeader.BlockAlign));
                }
                else
                    total_samples = -1;
            }
            else {
                total_samples = data_chunk_size / WaveHeader.BlockAlign;

                if (got_ds64 && total_samples != ds64_chunk.sampleCount64) {
                    error_line ("%s is not a valid .WAV file!", infilename);
                    return WAVPACK_SOFT_ERROR;
                }

                if (!total_samples) {
                    error_line ("this .WAV file has no audio samples, probably is corrupt!");
                    return WAVPACK_SOFT_ERROR;
                }

                if (total_samples > MAX_WAVPACK_SAMPLES) {
                    error_line ("%s has too many samples for WavPack!", infilename);
                    return WAVPACK_SOFT_ERROR;
                }
            }

            config->bytes_per_sample = WaveHeader.BlockAlign / WaveHeader.NumChannels;
            config->num_channels = WaveHeader.NumChannels;
            config->sample_rate = WaveHeader.SampleRate;
            break;
        }
        else {          // just copy unknown chunks to output file

            int bytes_to_copy = (chunk_header.ckSize + 1) & ~1L;
            char *buff;

            if (bytes_to_copy < 0 || bytes_to_copy > 4194304) {
                error_line ("%s is not a valid .WAV file!", infilename);
                return WAVPACK_SOFT_ERROR;
            }

            buff = malloc (bytes_to_copy);

            if (debug_logging_mode)
                error_line ("extra unknown chunk \"%c%c%c%c\" of %d bytes",
                    chunk_header.ckID [0], chunk_header.ckID [1], chunk_header.ckID [2],
                    chunk_header.ckID [3], chunk_header.ckSize);

            if (!DoReadFile (infile, buff, bytes_to_copy, &bcount) ||
                bcount != bytes_to_copy ||
                (!(config->qmode & QMODE_NO_STORE_WRAPPER) &&
                !WavpackAddWrapper (wpc, buff, bytes_to_copy))) {
                    error_line ("%s", WavpackGetErrorMessage (wpc));
                    free (buff);
                    return WAVPACK_SOFT_ERROR;
            }

            free (buff);
        }
    }

    if (!WavpackSetConfiguration64 (wpc, config, total_samples, NULL)) {
        error_line ("%s: %s", infilename, WavpackGetErrorMessage (wpc));
        return WAVPACK_SOFT_ERROR;
    }

    return WAVPACK_NO_ERROR;
}
