////////////////////////////////////////////////////////////////////////////
//                           **** WAVPACK ****                            //
//                  Hybrid Lossless Wavefile Compressor                   //
//                Copyright (c) 1998 - 2019 David Bryant.                 //
//                          All Rights Reserved.                          //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

// riff_write.c

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

extern int debug_logging_mode;

int WriteRiffHeader (FILE *outfile, WavpackContext *wpc, int64_t total_samples, int qmode)
{
    int do_rf64 = 0, write_junk = 1, table_length = 0;
    ChunkHeader ds64hdr, datahdr, fmthdr;
    RiffChunkHeader riffhdr;
    DS64Chunk ds64_chunk;
    CS64Chunk cs64_chunk;
    JunkChunk junkchunk;
    WaveHeader wavhdr;
    uint32_t bcount;

    int64_t total_data_bytes, total_riff_bytes;
    int num_channels = WavpackGetNumChannels (wpc);
    int32_t channel_mask = WavpackGetChannelMask (wpc);
    int32_t sample_rate = WavpackGetSampleRate (wpc);
    int bytes_per_sample = WavpackGetBytesPerSample (wpc);
    int bits_per_sample = WavpackGetBitsPerSample (wpc);
    int format = WavpackGetFloatNormExp (wpc) ? 3 : 1;
    int wavhdrsize = 16;

    if (format == 3 && WavpackGetFloatNormExp (wpc) != 127) {
        error_line ("can't create valid RIFF wav header for non-normalized floating data!");
        return FALSE;
    }

    if (total_samples == -1)
        total_samples = 0x7ffff000 / (bytes_per_sample * num_channels);

    total_data_bytes = total_samples * bytes_per_sample * num_channels;

    if (total_data_bytes > 0xff000000) {
        if (debug_logging_mode)
            error_line ("total_data_bytes = %lld, so rf64", total_data_bytes);
        write_junk = 0;
        do_rf64 = 1;
    }
    else if (debug_logging_mode)
        error_line ("total_data_bytes = %lld, so riff", total_data_bytes);

    CLEAR (wavhdr);

    wavhdr.FormatTag = format;
    wavhdr.NumChannels = num_channels;
    wavhdr.SampleRate = sample_rate;
    wavhdr.BytesPerSecond = sample_rate * num_channels * bytes_per_sample;
    wavhdr.BlockAlign = bytes_per_sample * num_channels;
    wavhdr.BitsPerSample = bits_per_sample;

    if (num_channels > 2 || channel_mask != 0x5 - num_channels) {
        wavhdrsize = sizeof (wavhdr);
        wavhdr.cbSize = 22;
        wavhdr.ValidBitsPerSample = bits_per_sample;
        wavhdr.SubFormat = format;
        wavhdr.ChannelMask = channel_mask;
        wavhdr.FormatTag = 0xfffe;
        wavhdr.BitsPerSample = bytes_per_sample * 8;
        wavhdr.GUID [4] = 0x10;
        wavhdr.GUID [6] = 0x80;
        wavhdr.GUID [9] = 0xaa;
        wavhdr.GUID [11] = 0x38;
        wavhdr.GUID [12] = 0x9b;
        wavhdr.GUID [13] = 0x71;
    }

    memcpy (riffhdr.ckID, do_rf64 ? "RF64" : "RIFF", sizeof (riffhdr.ckID));
    memcpy (riffhdr.formType, "WAVE", sizeof (riffhdr.formType));
    total_riff_bytes = sizeof (riffhdr) + wavhdrsize + sizeof (datahdr) + ((total_data_bytes + 1) & ~(int64_t)1);
    if (do_rf64) total_riff_bytes += sizeof (ds64hdr) + sizeof (ds64_chunk);
    total_riff_bytes += table_length * sizeof (CS64Chunk);
    if (write_junk) total_riff_bytes += sizeof (junkchunk);
    memcpy (fmthdr.ckID, "fmt ", sizeof (fmthdr.ckID));
    memcpy (datahdr.ckID, "data", sizeof (datahdr.ckID));
    fmthdr.ckSize = wavhdrsize;

    if (write_junk) {
        CLEAR (junkchunk);
        memcpy (junkchunk.ckID, "junk", sizeof (junkchunk.ckID));
        junkchunk.ckSize = sizeof (junkchunk) - 8;
        WavpackNativeToLittleEndian (&junkchunk, ChunkHeaderFormat);
    }

    if (do_rf64) {
        memcpy (ds64hdr.ckID, "ds64", sizeof (ds64hdr.ckID));
        ds64hdr.ckSize = sizeof (ds64_chunk) + (table_length * sizeof (CS64Chunk));
        CLEAR (ds64_chunk);
        ds64_chunk.riffSize64 = total_riff_bytes;
        ds64_chunk.dataSize64 = total_data_bytes;
        ds64_chunk.sampleCount64 = total_samples;
        ds64_chunk.tableLength = table_length;
        riffhdr.ckSize = (uint32_t) -1;
        datahdr.ckSize = (uint32_t) -1;
        WavpackNativeToLittleEndian (&ds64hdr, ChunkHeaderFormat);
        WavpackNativeToLittleEndian (&ds64_chunk, DS64ChunkFormat);
    }
    else {
        riffhdr.ckSize = (uint32_t) total_riff_bytes;
        datahdr.ckSize = (uint32_t) total_data_bytes;
    }

    // this "table" is just a dummy placeholder for testing (normally not written)

    if (table_length) {
        memcpy (cs64_chunk.ckID, "dmmy", sizeof (cs64_chunk.ckID));
        cs64_chunk.chunkSize64 = 12345678;
        WavpackNativeToLittleEndian (&cs64_chunk, CS64ChunkFormat);
    }

    // write the RIFF chunks up to just before the data starts

    WavpackNativeToLittleEndian (&riffhdr, ChunkHeaderFormat);
    WavpackNativeToLittleEndian (&fmthdr, ChunkHeaderFormat);
    WavpackNativeToLittleEndian (&wavhdr, WaveHeaderFormat);
    WavpackNativeToLittleEndian (&datahdr, ChunkHeaderFormat);

    if (!DoWriteFile (outfile, &riffhdr, sizeof (riffhdr), &bcount) || bcount != sizeof (riffhdr) ||
        (do_rf64 && (!DoWriteFile (outfile, &ds64hdr, sizeof (ds64hdr), &bcount) || bcount != sizeof (ds64hdr))) ||
        (do_rf64 && (!DoWriteFile (outfile, &ds64_chunk, sizeof (ds64_chunk), &bcount) || bcount != sizeof (ds64_chunk)))) {
            error_line ("can't write .WAV data, disk probably full!");
            return FALSE;
    }

    // again, this is normally not written except for testing

    while (table_length--)
        if (!DoWriteFile (outfile, &cs64_chunk, sizeof (cs64_chunk), &bcount) || bcount != sizeof (cs64_chunk)) {
            error_line ("can't write .WAV data, disk probably full!");
            return FALSE;
        }


    if ((write_junk && (!DoWriteFile (outfile, &junkchunk, sizeof (junkchunk), &bcount) || bcount != sizeof (junkchunk))) ||
        !DoWriteFile (outfile, &fmthdr, sizeof (fmthdr), &bcount) || bcount != sizeof (fmthdr) ||
        !DoWriteFile (outfile, &wavhdr, wavhdrsize, &bcount) || bcount != wavhdrsize ||
        !DoWriteFile (outfile, &datahdr, sizeof (datahdr), &bcount) || bcount != sizeof (datahdr)) {
            error_line ("can't write .WAV data, disk probably full!");
            return FALSE;
    }

    return TRUE;
}

