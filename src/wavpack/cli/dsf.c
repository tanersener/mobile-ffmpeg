////////////////////////////////////////////////////////////////////////////
//                           **** WAVPACK ****                            //
//                  Hybrid Lossless Wavefile Compressor                   //
//                Copyright (c) 1998 - 2019 David Bryant.                 //
//                          All Rights Reserved.                          //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

// dsf.c

// This module is a helper to the WavPack command-line programs to support DSF files.

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "wavpack.h"
#include "utils.h"

#define WAVPACK_NO_ERROR    0
#define WAVPACK_SOFT_ERROR  1
#define WAVPACK_HARD_ERROR  2

extern int debug_logging_mode;

#pragma pack(push,4)

typedef struct {
    char ckID [4];
    int64_t ckSize;
} DSFChunkHeader;

typedef struct {
    char ckID [4];
    int64_t ckSize;
    int64_t fileSize;
    int64_t metaOffset;
} DSFFileChunk;

typedef struct {
    char ckID [4];
    int64_t ckSize;
    uint32_t formatVersion, formatID;
    uint32_t chanType, numChannels, sampleRate, bitsPerSample;
    int64_t sampleCount;
    uint32_t blockSize, reserved;
} DSFFormatChunk;

#pragma pack(pop)

#define DSFChunkHeaderFormat "4D"
#define DSFFileChunkFormat "4DDD"
#define DSFFormatChunkFormat "4DLLLLLLDL4"

#define DSF_BLOCKSIZE 4096

static const uint16_t channel_masks [] = { 0x04, 0x03, 0x07, 0x33, 0x0f, 0x37, 0x3f };
#define NUM_CHAN_TYPES (sizeof (channel_masks) / sizeof (channel_masks [0]))

int ParseDsfHeaderConfig (FILE *infile, char *infilename, char *fourcc, WavpackContext *wpc, WavpackConfig *config)
{
    int64_t infilesize, total_samples, total_blocks, leftover_samples;
    DSFFileChunk file_chunk;
    DSFFormatChunk format_chunk;
    DSFChunkHeader chunk_header;

    uint32_t bcount;

    infilesize = DoGetFileSize (infile);
    memcpy (&file_chunk, fourcc, 4);

    if ((!DoReadFile (infile, ((char *) &file_chunk) + 4, sizeof (DSFFileChunk) - 4, &bcount) ||
        bcount != sizeof (DSFFileChunk) - 4)) {
            error_line ("%s is not a valid .DSF file!", infilename);
            return WAVPACK_SOFT_ERROR;
    }
    else if (!(config->qmode & QMODE_NO_STORE_WRAPPER) &&
        !WavpackAddWrapper (wpc, &file_chunk, sizeof (DSFFileChunk))) {
            error_line ("%s", WavpackGetErrorMessage (wpc));
            return WAVPACK_SOFT_ERROR;
    }

#if 1   // this might be a little too picky...
    WavpackLittleEndianToNative (&file_chunk, DSFFileChunkFormat);

    if (debug_logging_mode)
        error_line ("file header lengths = %lld, %lld, %lld", file_chunk.ckSize, file_chunk.fileSize, file_chunk.metaOffset);

    if (infilesize && !(config->qmode & QMODE_IGNORE_LENGTH) &&
        file_chunk.fileSize && file_chunk.fileSize + 1 && file_chunk.fileSize != infilesize) {
            error_line ("%s is not a valid .DSF file (by total size)!", infilename);
            return WAVPACK_SOFT_ERROR;
    }
#endif

    if (config->channel_mask || (config->qmode & QMODE_CHANS_UNASSIGNED)) {
        error_line ("this DSF file already has channel order information!");
        return WAVPACK_SOFT_ERROR;
    }

    if (!DoReadFile (infile, ((char *) &format_chunk), sizeof (DSFFormatChunk), &bcount) ||
        bcount != sizeof (DSFFormatChunk) || strncmp (format_chunk.ckID, "fmt ", 4)) {
            error_line ("%s is not a valid .DSF file!", infilename);
            return WAVPACK_SOFT_ERROR;
    }
    else if (!(config->qmode & QMODE_NO_STORE_WRAPPER) &&
        !WavpackAddWrapper (wpc, &format_chunk, sizeof (DSFFormatChunk))) {
            error_line ("%s", WavpackGetErrorMessage (wpc));
            return WAVPACK_SOFT_ERROR;
    }

    WavpackLittleEndianToNative (&format_chunk, DSFFormatChunkFormat);

    if (format_chunk.ckSize != sizeof (DSFFormatChunk) || format_chunk.formatVersion != 1 ||
        format_chunk.formatID != 0 || format_chunk.blockSize != DSF_BLOCKSIZE || format_chunk.reserved ||
        (format_chunk.bitsPerSample != 1 && format_chunk.bitsPerSample != 8) ||
        format_chunk.numChannels < 1 || format_chunk.numChannels > 6 ||
        format_chunk.chanType < 1 || format_chunk.chanType > NUM_CHAN_TYPES) {
            error_line ("%s is not a valid .DSF file!", infilename);
            return WAVPACK_SOFT_ERROR;
    }

    if (debug_logging_mode) {
        error_line ("sampling rate = %d Hz", format_chunk.sampleRate);
        error_line ("channel type = %d, channel count = %d", format_chunk.chanType, format_chunk.numChannels);
        error_line ("block size = %d, bits per sample = %d", format_chunk.blockSize, format_chunk.bitsPerSample);
        error_line ("sample count = %lld", format_chunk.sampleCount);
    }

    if (!DoReadFile (infile, ((char *) &chunk_header), sizeof (DSFChunkHeader), &bcount) ||
        bcount != sizeof (DSFChunkHeader) || strncmp (chunk_header.ckID, "data", 4)) {
            error_line ("%s is not a valid .DSF file!", infilename);
            return WAVPACK_SOFT_ERROR;
    }
    else if (!(config->qmode & QMODE_NO_STORE_WRAPPER) &&
        !WavpackAddWrapper (wpc, &chunk_header, sizeof (DSFChunkHeader))) {
            error_line ("%s", WavpackGetErrorMessage (wpc));
            return WAVPACK_SOFT_ERROR;
    }

    WavpackLittleEndianToNative (&chunk_header, DSFChunkHeaderFormat);

    total_samples = format_chunk.sampleCount;
    total_blocks = total_samples / (format_chunk.blockSize * 8);
    leftover_samples = total_samples - (total_blocks * format_chunk.blockSize * 8);

    if (leftover_samples)
        total_blocks++;

    if (debug_logging_mode) {
        error_line ("leftover samples = %lld, leftover bits = %d", leftover_samples, (int)(leftover_samples % 8));
        error_line ("data chunk size (specified) = %lld", chunk_header.ckSize - 12);
        error_line ("data chunk size (calculated) = %lld", total_blocks * DSF_BLOCKSIZE * format_chunk.numChannels);
    }

    if (total_samples & 0x7)
        error_line ("warning: DSF file has partial-byte leftover samples!");

    if (format_chunk.sampleRate & 0x7)
        error_line ("warning: DSF file has non-integer bytes/second!");

    config->bits_per_sample = 8;
    config->bytes_per_sample = 1;
    config->num_channels = format_chunk.numChannels;
    config->channel_mask = channel_masks [format_chunk.chanType - 1];
    config->sample_rate = format_chunk.sampleRate / 8;

    if (format_chunk.bitsPerSample == 1)
        config->qmode |= QMODE_DSD_LSB_FIRST | QMODE_DSD_IN_BLOCKS;
    else
        config->qmode |= QMODE_DSD_MSB_FIRST | QMODE_DSD_IN_BLOCKS;

    if (!WavpackSetConfiguration64 (wpc, config, (total_samples + 7) / 8, NULL)) {
        error_line ("%s: %s", infilename, WavpackGetErrorMessage (wpc));
        return WAVPACK_SOFT_ERROR;
    }

    return WAVPACK_NO_ERROR;
}
