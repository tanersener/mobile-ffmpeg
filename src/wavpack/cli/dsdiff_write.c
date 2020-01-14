////////////////////////////////////////////////////////////////////////////
//                           **** WAVPACK ****                            //
//                  Hybrid Lossless Wavefile Compressor                   //
//                Copyright (c) 1998 - 2019 David Bryant.                 //
//                          All Rights Reserved.                          //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

// dsdiff_write.c

// This module is a helper to the WavPack command-line programs to support DFF files.

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "wavpack.h"
#include "utils.h"

extern int debug_logging_mode;

#pragma pack(push,2)

typedef struct {
    char ckID [4];
    int64_t ckDataSize;
} DFFChunkHeader;

typedef struct {
    char ckID [4];
    int64_t ckDataSize;
    char formType [4];
} DFFFileHeader;

typedef struct {
    char ckID [4];
    int64_t ckDataSize;
    uint32_t version;
} DFFVersionChunk;

typedef struct {
    char ckID [4];
    int64_t ckDataSize;
    uint32_t sampleRate;
} DFFSampleRateChunk;

typedef struct {
    char ckID [4];
    int64_t ckDataSize;
    uint16_t numChannels;
} DFFChannelsHeader;

typedef struct {
    char ckID [4];
    int64_t ckDataSize;
    char compressionType [4];
} DFFCompressionHeader;

#pragma pack(pop)

#define DFFChunkHeaderFormat "4D"
#define DFFFileHeaderFormat "4D4"
#define DFFVersionChunkFormat "4DL"
#define DFFSampleRateChunkFormat "4DL"
#define DFFChannelsHeaderFormat "4DS"
#define DFFCompressionHeaderFormat "4D4"

int WriteDsdiffHeader (FILE *outfile, WavpackContext *wpc, int64_t total_samples, int qmode)
{
    uint32_t chan_mask = WavpackGetChannelMask (wpc);
    int num_channels = WavpackGetNumChannels (wpc);
    DFFFileHeader file_header, prop_header;
    DFFChunkHeader data_header;
    DFFVersionChunk ver_chunk;
    DFFSampleRateChunk fs_chunk;
    DFFChannelsHeader chan_header;
    DFFCompressionHeader cmpr_header;
    char *cmpr_name = "\016not compressed", *chan_ids;
    int64_t file_size, prop_chunk_size, data_size;
    int cmpr_name_size, chan_ids_size;
    uint32_t bcount;

    if (debug_logging_mode)
        error_line ("WriteDsdiffHeader (), total samples = %lld, qmode = 0x%02x\n",
            (long long) total_samples, qmode);

    cmpr_name_size = (strlen (cmpr_name) + 1) & ~1;
    chan_ids_size = num_channels * 4;
    chan_ids = malloc (chan_ids_size);

    if (chan_ids) {
        uint32_t scan_mask = 0x1;
        char *cptr = chan_ids;
        int ci, uci = 0;

        for (ci = 0; ci < num_channels; ++ci) {
            while (scan_mask && !(scan_mask & chan_mask))
                scan_mask <<= 1;

            if (scan_mask & 0x1)
                memcpy (cptr, num_channels <= 2 ? "SLFT" : "MLFT", 4);
            else if (scan_mask & 0x2)
                memcpy (cptr, num_channels <= 2 ? "SRGT" : "MRGT", 4);
            else if (scan_mask & 0x4)
                memcpy (cptr, "C   ", 4);
            else if (scan_mask & 0x8)
                memcpy (cptr, "LFE ", 4);
            else if (scan_mask & 0x10)
                memcpy (cptr, "LS  ", 4);
            else if (scan_mask & 0x20)
                memcpy (cptr, "RS  ", 4);
            else {
                cptr [0] = 'C';
                cptr [1] = (uci / 100) + '0';
                cptr [2] = ((uci % 100) / 10) + '0';
                cptr [3] = (uci % 10) + '0';
                uci++;
            }

            scan_mask <<= 1;
            cptr += 4;
        }
    }
    else {
        error_line ("can't allocate memory!");
        return FALSE;
    }

    data_size = total_samples * num_channels;
    prop_chunk_size = sizeof (prop_header) + sizeof (fs_chunk) + sizeof (chan_header) + chan_ids_size + sizeof (cmpr_header) + cmpr_name_size;
    file_size = sizeof (file_header) + sizeof (ver_chunk) + prop_chunk_size + sizeof (data_header) + ((data_size + 1) & ~(int64_t)1);

    memcpy (file_header.ckID, "FRM8", 4);
    file_header.ckDataSize = file_size - 12;
    memcpy (file_header.formType, "DSD ", 4);

    memcpy (prop_header.ckID, "PROP", 4);
    prop_header.ckDataSize = prop_chunk_size - 12;
    memcpy (prop_header.formType, "SND ", 4);

    memcpy (ver_chunk.ckID, "FVER", 4);
    ver_chunk.ckDataSize = sizeof (ver_chunk) - 12;
    ver_chunk.version = 0x01050000;

    memcpy (fs_chunk.ckID, "FS  ", 4);
    fs_chunk.ckDataSize = sizeof (fs_chunk) - 12;
    fs_chunk.sampleRate = WavpackGetSampleRate (wpc) * 8;

    memcpy (chan_header.ckID, "CHNL", 4);
    chan_header.ckDataSize = sizeof (chan_header) + chan_ids_size - 12;
    chan_header.numChannels = num_channels;

    memcpy (cmpr_header.ckID, "CMPR", 4);
    cmpr_header.ckDataSize = sizeof (cmpr_header) + cmpr_name_size - 12;
    memcpy (cmpr_header.compressionType, "DSD ", 4);

    memcpy (data_header.ckID, "DSD ", 4);
    data_header.ckDataSize = data_size;

    WavpackNativeToBigEndian (&file_header, DFFFileHeaderFormat);
    WavpackNativeToBigEndian (&ver_chunk, DFFVersionChunkFormat);
    WavpackNativeToBigEndian (&prop_header, DFFFileHeaderFormat);
    WavpackNativeToBigEndian (&fs_chunk, DFFSampleRateChunkFormat);
    WavpackNativeToBigEndian (&chan_header, DFFChannelsHeaderFormat);
    WavpackNativeToBigEndian (&cmpr_header, DFFCompressionHeaderFormat);
    WavpackNativeToBigEndian (&data_header, DFFChunkHeaderFormat);

    if (!DoWriteFile (outfile, &file_header, sizeof (file_header), &bcount) || bcount != sizeof (file_header) ||
        !DoWriteFile (outfile, &ver_chunk, sizeof (ver_chunk), &bcount) || bcount != sizeof (ver_chunk) ||
        !DoWriteFile (outfile, &prop_header, sizeof (prop_header), &bcount) || bcount != sizeof (prop_header) ||
        !DoWriteFile (outfile, &fs_chunk, sizeof (fs_chunk), &bcount) || bcount != sizeof (fs_chunk) ||
        !DoWriteFile (outfile, &chan_header, sizeof (chan_header), &bcount) || bcount != sizeof (chan_header) ||
        !DoWriteFile (outfile, chan_ids, chan_ids_size, &bcount) || bcount != chan_ids_size ||
        !DoWriteFile (outfile, &cmpr_header, sizeof (cmpr_header), &bcount) || bcount != sizeof (cmpr_header) ||
        !DoWriteFile (outfile, cmpr_name, cmpr_name_size, &bcount) || bcount != cmpr_name_size ||
        !DoWriteFile (outfile, &data_header, sizeof (data_header), &bcount) || bcount != sizeof (data_header)) {
            error_line ("can't write .DSF data, disk probably full!");
            free (chan_ids);
            return FALSE;
    }

    free (chan_ids);
    return TRUE;
}

