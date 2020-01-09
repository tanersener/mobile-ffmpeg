////////////////////////////////////////////////////////////////////////////
//                           **** WAVPACK ****                            //
//                  Hybrid Lossless Wavefile Compressor                   //
//                Copyright (c) 1998 - 2019 David Bryant.                 //
//                          All Rights Reserved.                          //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

// dsf_write.c

// This module is a helper to the WavPack command-line programs to support DSF files.

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "wavpack.h"
#include "utils.h"

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

int WriteDsfHeader (FILE *outfile, WavpackContext *wpc, int64_t total_samples, int qmode)
{
    uint32_t chan_mask = WavpackGetChannelMask (wpc), chan_type = 0;
    int num_channels = WavpackGetNumChannels (wpc);
    int64_t file_size, total_blocks, data_size;
    DSFFileChunk file_chunk;
    DSFFormatChunk format_chunk;
    DSFChunkHeader chunk_header;
    uint32_t bcount;
    int i;

    if (debug_logging_mode)
        error_line ("WriteDsfHeader (), total samples = %lld, qmode = 0x%02x\n",
            (long long) total_samples, qmode);

    for (i = 0; i < NUM_CHAN_TYPES; ++i)
        if (chan_mask == channel_masks [i])
            chan_type = i + 1;

    // not sure exactly what to do here, but guess (and not just fail)

    if (!chan_type) {
        if (num_channels > 6)
            chan_type = 7;
        else if (num_channels > 4)
            chan_type = num_channels + 1;
        else
            chan_type = num_channels;
    }

    total_blocks = (total_samples + DSF_BLOCKSIZE - 1) / DSF_BLOCKSIZE;
    data_size = total_blocks * DSF_BLOCKSIZE * num_channels;
    file_size = data_size + sizeof (file_chunk) + sizeof (format_chunk) + sizeof (chunk_header);

    memcpy (file_chunk.ckID, "DSD ", 4);
    file_chunk.ckSize = sizeof (file_chunk);
    file_chunk.fileSize = file_size;
    file_chunk.metaOffset = 0;

    memcpy (format_chunk.ckID, "fmt ", 4);
    format_chunk.ckSize = sizeof (format_chunk);
    format_chunk.formatVersion = 1;
    format_chunk.formatID = 0;
    format_chunk.chanType = chan_type;
    format_chunk.numChannels = num_channels;
    format_chunk.sampleRate = WavpackGetSampleRate (wpc) * 8;
    format_chunk.bitsPerSample = (qmode & QMODE_DSD_LSB_FIRST) ? 1 : 8;
    format_chunk.sampleCount = total_samples * 8;
    format_chunk.blockSize = DSF_BLOCKSIZE;
    format_chunk.reserved = 0;

    memcpy (chunk_header.ckID, "data", 4);
    chunk_header.ckSize = data_size + 12;

    // write the 3 chunks up to just before the data starts

    WavpackNativeToLittleEndian (&file_chunk, DSFFileChunkFormat);
    WavpackNativeToLittleEndian (&format_chunk, DSFFormatChunkFormat);
    WavpackNativeToLittleEndian (&chunk_header, DSFChunkHeaderFormat);

    if (!DoWriteFile (outfile, &file_chunk, sizeof (file_chunk), &bcount) || bcount != sizeof (file_chunk) ||
        !DoWriteFile (outfile, &format_chunk, sizeof (format_chunk), &bcount) || bcount != sizeof (format_chunk) ||
        !DoWriteFile (outfile, &chunk_header, sizeof (chunk_header), &bcount) || bcount != sizeof (chunk_header)) {
            error_line ("can't write .DSF data, disk probably full!");
            return FALSE;
    }

    return TRUE;
}
