////////////////////////////////////////////////////////////////////////////
//                           **** WAVPACK ****                            //
//                  Hybrid Lossless Wavefile Compressor                   //
//                Copyright (c) 1998 - 2019 David Bryant.                 //
//                          All Rights Reserved.                          //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

// wave64_write.c

// This module is a helper to the WavPack command-line programs to support Sony's
// Wave64 WAV file variant. Note that unlike the WAV/RF64 version, this does not
// fall back to conventional WAV in the < 4GB case.

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "wavpack.h"
#include "utils.h"

typedef struct {
    char ckID [16];
    int64_t ckSize;
    char formType [16];
} Wave64FileHeader;

typedef struct {
    char ckID [16];
    int64_t ckSize;
} Wave64ChunkHeader;

#define Wave64ChunkHeaderFormat "88D"

static const unsigned char riff_guid [16] = { 'r','i','f','f', 0x2e,0x91,0xcf,0x11,0xa5,0xd6,0x28,0xdb,0x04,0xc1,0x00,0x00 };
static const unsigned char wave_guid [16] = { 'w','a','v','e', 0xf3,0xac,0xd3,0x11,0x8c,0xd1,0x00,0xc0,0x4f,0x8e,0xdb,0x8a };
static const unsigned char  fmt_guid [16] = { 'f','m','t',' ', 0xf3,0xac,0xd3,0x11,0x8c,0xd1,0x00,0xc0,0x4f,0x8e,0xdb,0x8a };
static const unsigned char data_guid [16] = { 'd','a','t','a', 0xf3,0xac,0xd3,0x11,0x8c,0xd1,0x00,0xc0,0x4f,0x8e,0xdb,0x8a };

extern int debug_logging_mode;

int WriteWave64Header (FILE *outfile, WavpackContext *wpc, int64_t total_samples, int qmode)
{
    Wave64ChunkHeader datahdr, fmthdr;
    Wave64FileHeader filehdr;
    WaveHeader wavhdr;
    uint32_t bcount;

    int64_t total_data_bytes, total_file_bytes;
    int num_channels = WavpackGetNumChannels (wpc);
    int32_t channel_mask = WavpackGetChannelMask (wpc);
    int32_t sample_rate = WavpackGetSampleRate (wpc);
    int bytes_per_sample = WavpackGetBytesPerSample (wpc);
    int bits_per_sample = WavpackGetBitsPerSample (wpc);
    int format = WavpackGetFloatNormExp (wpc) ? 3 : 1;
    int wavhdrsize = 16;

    if (format == 3 && WavpackGetFloatNormExp (wpc) != 127) {
        error_line ("can't create valid Wave64 header for non-normalized floating data!");
        return FALSE;
    }

    if (total_samples == -1)
        total_samples = 0x7ffff000 / (bytes_per_sample * num_channels);

    total_data_bytes = total_samples * bytes_per_sample * num_channels;
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

    total_file_bytes = sizeof (filehdr) + sizeof (fmthdr) + wavhdrsize + sizeof (datahdr) + ((total_data_bytes + 7) & ~(int64_t)7);

    memcpy (filehdr.ckID, riff_guid, sizeof (riff_guid));
    memcpy (filehdr.formType, wave_guid, sizeof (wave_guid));
    filehdr.ckSize = total_file_bytes;

    memcpy (fmthdr.ckID, fmt_guid, sizeof (fmt_guid));
    fmthdr.ckSize = sizeof (fmthdr) + wavhdrsize;

    memcpy (datahdr.ckID, data_guid, sizeof (data_guid));
    datahdr.ckSize = total_data_bytes + sizeof (datahdr);

    // write the RIFF chunks up to just before the data starts

    WavpackNativeToLittleEndian (&filehdr, Wave64ChunkHeaderFormat);
    WavpackNativeToLittleEndian (&fmthdr, Wave64ChunkHeaderFormat);
    WavpackNativeToLittleEndian (&wavhdr, WaveHeaderFormat);
    WavpackNativeToLittleEndian (&datahdr, Wave64ChunkHeaderFormat);

    if (!DoWriteFile (outfile, &filehdr, sizeof (filehdr), &bcount) || bcount != sizeof (filehdr) ||
        !DoWriteFile (outfile, &fmthdr, sizeof (fmthdr), &bcount) || bcount != sizeof (fmthdr) ||
        !DoWriteFile (outfile, &wavhdr, wavhdrsize, &bcount) || bcount != wavhdrsize ||
        !DoWriteFile (outfile, &datahdr, sizeof (datahdr), &bcount) || bcount != sizeof (datahdr)) {
            error_line ("can't write .W64 data, disk probably full!");
            return FALSE;
    }

    return TRUE;
}

