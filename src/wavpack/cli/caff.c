////////////////////////////////////////////////////////////////////////////
//                           **** WAVPACK ****                            //
//                  Hybrid Lossless Wavefile Compressor                   //
//                Copyright (c) 1998 - 2016 David Bryant.                 //
//                          All Rights Reserved.                          //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

// caff.c

// This module is a helper to the WavPack command-line programs to support CAF files.

#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <ctype.h>

#include "wavpack.h"
#include "utils.h"
#include "md5.h"

#ifdef _WIN32
#define strdup(x) _strdup(x)
#endif


#define WAVPACK_NO_ERROR    0
#define WAVPACK_SOFT_ERROR  1
#define WAVPACK_HARD_ERROR  2

extern int debug_logging_mode;

typedef struct
{
    char mFileType [4];
    uint16_t mFileVersion;
    uint16_t mFileFlags;
} CAFFileHeader;

#define CAFFileHeaderFormat "4SS"

#pragma pack(push,4)
typedef struct
{
    char mChunkType [4];
    int64_t mChunkSize;
} CAFChunkHeader;
#pragma pack(pop)

#define CAFChunkHeaderFormat "4D"

typedef struct
{
    double mSampleRate;
    char mFormatID [4];
    uint32_t mFormatFlags;
    uint32_t mBytesPerPacket;
    uint32_t mFramesPerPacket;
    uint32_t mChannelsPerFrame;
    uint32_t mBitsPerChannel;
} CAFAudioFormat;

#define CAFAudioFormatFormat "D4LLLLL"
#define CAF_FORMAT_FLOAT            0x1
#define CAF_FORMAT_LITTLE_ENDIAN    0x2

typedef struct
{
    uint32_t mChannelLayoutTag;
    uint32_t mChannelBitmap;
    uint32_t mNumberChannelDescriptions;
} CAFChannelLayout;

#define CAFChannelLayoutFormat "LLL"

enum {
    kCAFChannelLayoutTag_UseChannelDescriptions = (0<<16) | 0,  // use the array of AudioChannelDescriptions to define the mapping.
    kCAFChannelLayoutTag_UseChannelBitmap = (1<<16) | 0,        // use the bitmap to define the mapping.
};

typedef struct
{
    uint32_t mChannelLabel;
    uint32_t mChannelFlags;
    float mCoordinates [3];
} CAFChannelDescription;

#define CAFChannelDescriptionFormat "LLLLL"

static const char TMH_full [] = { 1,2,3,13,9,10,5,6,12,14,15,16,17,9,4,18,7,8,19,20,21 };
static const char TMH_std [] = { 1,2,3,11,8,9,5,6,10,12,13,14,15,7,4,16 };

static struct {
    uint32_t mChannelLayoutTag;     // Core Audio layout, 100 - 146 in high word, num channels in low word
    uint32_t mChannelBitmap;        // Microsoft standard mask (for those channels that appear)
    const char *mChannelReorder;    // reorder string if layout is NOT in Microsoft standard order
    const char *mChannelIdentities; // identities of any channels NOT in Microsoft standard
} layouts [] = {
    { (100<<16) | 1, 0x004, NULL,       NULL            },  // FC
    { (101<<16) | 2, 0x003, NULL,       NULL            },  // FL, FR
    { (102<<16) | 2, 0x003, NULL,       NULL            },  // FL, FR (headphones)
    { (103<<16) | 2, 0x000, NULL,       "\46\47"        },  // [Lt, Rt] (matrix encoded)
    { (104<<16) | 2, 0x000, NULL,       "\314\315"      },  // [Mid, Side]
    { (105<<16) | 2, 0x000, NULL,       "\316\317"      },  // [X, Y]
    { (106<<16) | 2, 0x003, NULL,       NULL            },  // FL, FR (binaural)
    { (107<<16) | 4, 0x000, NULL,       "\310\311\312\313"  },  // [W, X, Y, Z] (ambisonics)
    { (108<<16) | 4, 0x033, NULL,       NULL            },  // FL, FR, BL, BR (quad)
    { (109<<16) | 5, 0x037, "12453",    NULL            },  // FL, FR, BL, BR, FC (pentagonal)
    { (110<<16) | 6, 0x137, "124536",   NULL            },  // FL, FR, BL, BR, FC, BC (hexagonal)
    { (111<<16) | 8, 0x737, "12453678", NULL            },  // FL, FR, BL, BR, FC, BC, SL, SR (octagonal)
    { (112<<16) | 8, 0x2d033, NULL,     NULL            },  // FL, FR, BL, BR, TFL, TFR, TBL, TBR (cubic)
    { (113<<16) | 3, 0x007, NULL,       NULL            },  // FL, FR, FC
    { (114<<16) | 3, 0x007, "312",      NULL            },  // FC, FL, FR
    { (115<<16) | 4, 0x107, NULL,       NULL            },  // FL, FR, FC, BC
    { (116<<16) | 4, 0x107, "3124",     NULL            },  // FC, FL, FR, BC
    { (117<<16) | 5, 0x037, NULL,       NULL            },  // FL, FR, FC, BL, BR
    { (118<<16) | 5, 0x037, "12453",    NULL            },  // FL, FR, BL, BR, FC
    { (119<<16) | 5, 0x037, "13245",    NULL            },  // FL, FC, FR, BL, BR
    { (120<<16) | 5, 0x037, "31245",    NULL            },  // FC, FL, FR, BL, BR
    { (121<<16) | 6, 0x03f, NULL,       NULL            },  // FL, FR, FC, LFE, BL, BR
    { (122<<16) | 6, 0x03f, "125634",   NULL            },  // FL, FR, BL, BR, FC, LFE
    { (123<<16) | 6, 0x03f, "132564",   NULL            },  // FL, FC, FR, BL, BR, LFE
    { (124<<16) | 6, 0x03f, "312564",   NULL            },  // FC, FL, FR, BL, BR, LFE
    { (125<<16) | 7, 0x13f, NULL,       NULL            },  // FL, FR, FC, LFE, BL, BR, BC
    { (126<<16) | 8, 0x0ff, NULL,       NULL            },  // FL, FR, FC, LFE, BL, BR, FLC, FRC
    { (127<<16) | 8, 0x0ff, "37812564", NULL            },  // FC, FLC, FRC, FL, FR, BL, BR, LFE
    { (128<<16) | 8, 0x03f, NULL,       "\41\42"        },  // FL, FR, FC, LFE, BL, BR, [Rls, Rrs]
    { (129<<16) | 8, 0x0ff, "12563478", NULL            },  // FL, FR, BL, BR, FC, LFE, FLC, FRC
    { (130<<16) | 8, 0x03f, NULL,       "\46\47"        },  // FL, FR, FC, LFE, BL, BR, [Lt, Rt]
    { (131<<16) | 3, 0x103, NULL,       NULL            },  // FL, FR, BC
    { (132<<16) | 4, 0x033, NULL,       NULL            },  // FL, FR, BL, BR
    { (133<<16) | 3, 0x00B, NULL,       NULL            },  // FL, FR, LFE
    { (134<<16) | 4, 0x10B, NULL,       NULL            },  // FL, FR, LFE, BC
    { (135<<16) | 5, 0x03B, NULL,       NULL            },  // FL, FR, LFE, BL, BR
    { (136<<16) | 4, 0x00F, NULL,       NULL            },  // FL, FR, FC, LFE
    { (137<<16) | 5, 0x10f, NULL,       NULL            },  // FL, FR, FC, LFE, BC
    { (138<<16) | 5, 0x03b, "12453",    NULL            },  // FL, FR, BL, BR, LFE
    { (139<<16) | 6, 0x137, "124536",   NULL            },  // FL, FR, BL, BR, FC, BC
    { (140<<16) | 7, 0x037, "1245367",  "\41\42"        },  // FL, FR, BL, BR, FC, [Rls, Rrs]
    { (141<<16) | 6, 0x137, "312456",   NULL            },  // FC, FL, FR, BL, BR, BC
    { (142<<16) | 7, 0x13f, "3125674",  NULL            },  // FC, FL, FR, BL, BR, BC, LFE
    { (143<<16) | 7, 0x037, "3124567",  "\41\42"        },  // FC, FL, FR, BL, BR, [Rls, Rrs]
    { (144<<16) | 8, 0x137, "31245786", "\41\42"        },  // FC, FL, FR, BL, BR, [Rls, Rrs], BC
    { (145<<16) | 16, 0x773f, TMH_std,  "\43\44\54\45"  },  // FL, FR, FC, TFC, SL, SR, BL, BR, TFL, TFR, [Lw, Rw, Csd], BC, LFE, [LFE2]
    { (146<<16) | 21, 0x77ff, TMH_full, "\43\44\54\45"  },  // FL, FR, FC, TFC, SL, SR, BL, BR, TFL, TFR, [Lw, Rw, Csd], BC, LFE, [LFE2],
                                                            //     FLC, FRC, [HI, VI, Haptic]
};

#define NUM_LAYOUTS (sizeof (layouts) / sizeof (layouts [0]))

int ParseCaffHeaderConfig (FILE *infile, char *infilename, char *fourcc, WavpackContext *wpc, WavpackConfig *config)
{
    uint32_t chan_chunk = 0, channel_layout = 0, bcount;
    unsigned char *channel_identities = NULL;
    unsigned char *channel_reorder = NULL;
    int64_t total_samples = 0, infilesize;
    CAFFileHeader caf_file_header;
    CAFChunkHeader caf_chunk_header;
    CAFAudioFormat caf_audio_format;
    int i;

    infilesize = DoGetFileSize (infile);
    memcpy (&caf_file_header, fourcc, 4);

    if ((!DoReadFile (infile, ((char *) &caf_file_header) + 4, sizeof (CAFFileHeader) - 4, &bcount) ||
        bcount != sizeof (CAFFileHeader) - 4)) {
            error_line ("%s is not a valid .CAF file!", infilename);
            return WAVPACK_SOFT_ERROR;
    }
    else if (!(config->qmode & QMODE_NO_STORE_WRAPPER) &&
        !WavpackAddWrapper (wpc, &caf_file_header, sizeof (CAFFileHeader))) {
            error_line ("%s", WavpackGetErrorMessage (wpc));
            return WAVPACK_SOFT_ERROR;
    }

    WavpackBigEndianToNative (&caf_file_header, CAFFileHeaderFormat);

    if (caf_file_header.mFileVersion != 1) {
        error_line ("%s: can't handle version %d .CAF files!", infilename, caf_file_header.mFileVersion);
        return WAVPACK_SOFT_ERROR;
    }

    // loop through all elements of the RIFF wav header
    // (until the data chuck) and copy them to the output file

    while (1) {
        if (!DoReadFile (infile, &caf_chunk_header, sizeof (CAFChunkHeader), &bcount) ||
            bcount != sizeof (CAFChunkHeader)) {
                error_line ("%s is not a valid .CAF file!", infilename);
                return WAVPACK_SOFT_ERROR;
        }
        else if (!(config->qmode & QMODE_NO_STORE_WRAPPER) &&
            !WavpackAddWrapper (wpc, &caf_chunk_header, sizeof (CAFChunkHeader))) {
                error_line ("%s", WavpackGetErrorMessage (wpc));
                return WAVPACK_SOFT_ERROR;
        }

        WavpackBigEndianToNative (&caf_chunk_header, CAFChunkHeaderFormat);

        // if it's the format chunk, we want to get some info out of there and
        // make sure it's a .caf file we can handle

        if (!strncmp (caf_chunk_header.mChunkType, "desc", 4)) {
            int supported = TRUE;

            if (caf_chunk_header.mChunkSize != sizeof (CAFAudioFormat) ||
                !DoReadFile (infile, &caf_audio_format, (uint32_t) caf_chunk_header.mChunkSize, &bcount) ||
                bcount != caf_chunk_header.mChunkSize) {
                    error_line ("%s is not a valid .CAF file!", infilename);
                    return WAVPACK_SOFT_ERROR;
            }
            else if (!(config->qmode & QMODE_NO_STORE_WRAPPER) &&
                !WavpackAddWrapper (wpc, &caf_audio_format, (uint32_t) caf_chunk_header.mChunkSize)) {
                    error_line ("%s", WavpackGetErrorMessage (wpc));
                    return WAVPACK_SOFT_ERROR;
            }

            WavpackBigEndianToNative (&caf_audio_format, CAFAudioFormatFormat);

            if (debug_logging_mode) {
                char formatstr [5];

                memcpy (formatstr, caf_audio_format.mFormatID, 4);
                formatstr [4] = 0;
                error_line ("format = %s, flags = %x, sampling rate = %g",
                    formatstr, caf_audio_format.mFormatFlags, caf_audio_format.mSampleRate);
                error_line ("packet = %d bytes and %d frames",
                    caf_audio_format.mBytesPerPacket, caf_audio_format.mFramesPerPacket);
                error_line ("channels per frame = %d, bits per channel = %d",
                    caf_audio_format.mChannelsPerFrame, caf_audio_format.mBitsPerChannel);
            }

            if (strncmp (caf_audio_format.mFormatID, "lpcm", 4) || (caf_audio_format.mFormatFlags & ~3))
                supported = FALSE;
            else if (caf_audio_format.mSampleRate < 1.0 || caf_audio_format.mSampleRate > 16777215.0 ||
                caf_audio_format.mSampleRate != floor (caf_audio_format.mSampleRate))
                    supported = FALSE;
            else if (!caf_audio_format.mChannelsPerFrame || caf_audio_format.mChannelsPerFrame > 256)
                supported = FALSE;
            else if (caf_audio_format.mBitsPerChannel < 1 || caf_audio_format.mBitsPerChannel > 32 ||
                ((caf_audio_format.mFormatFlags & CAF_FORMAT_FLOAT) && caf_audio_format.mBitsPerChannel != 32))
                    supported = FALSE;
            else if (caf_audio_format.mFramesPerPacket != 1 ||
                caf_audio_format.mBytesPerPacket / caf_audio_format.mChannelsPerFrame < (caf_audio_format.mBitsPerChannel + 7) / 8 ||
                caf_audio_format.mBytesPerPacket / caf_audio_format.mChannelsPerFrame > 4 ||
                caf_audio_format.mBytesPerPacket % caf_audio_format.mChannelsPerFrame)
                    supported = FALSE;

            if (!supported) {
                error_line ("%s is an unsupported .CAF format!", infilename);
                return WAVPACK_SOFT_ERROR;
            }

            config->bytes_per_sample = caf_audio_format.mBytesPerPacket / caf_audio_format.mChannelsPerFrame;
            config->float_norm_exp = (caf_audio_format.mFormatFlags & CAF_FORMAT_FLOAT) ? 127 : 0;
            config->bits_per_sample = caf_audio_format.mBitsPerChannel;
            config->num_channels = caf_audio_format.mChannelsPerFrame;
            config->sample_rate = (int) caf_audio_format.mSampleRate;

            if (!(caf_audio_format.mFormatFlags & CAF_FORMAT_LITTLE_ENDIAN) && config->bytes_per_sample > 1)
                config->qmode |= QMODE_BIG_ENDIAN;

            if (config->bytes_per_sample == 1)
                config->qmode |= QMODE_SIGNED_BYTES;

            if (debug_logging_mode) {
                if (config->float_norm_exp == 127)
                    error_line ("data format: 32-bit %s-endian floating point", (config->qmode & QMODE_BIG_ENDIAN) ? "big" : "little");
                else
                    error_line ("data format: %d-bit %s-endian integers stored in %d byte(s)",
                        config->bits_per_sample, (config->qmode & QMODE_BIG_ENDIAN) ? "big" : "little", config->bytes_per_sample);
            }
        }
        else if (!strncmp (caf_chunk_header.mChunkType, "chan", 4)) {
            CAFChannelLayout *caf_channel_layout = malloc ((size_t) caf_chunk_header.mChunkSize);

            if (caf_chunk_header.mChunkSize < sizeof (CAFChannelLayout) ||
                !DoReadFile (infile, caf_channel_layout, (uint32_t) caf_chunk_header.mChunkSize, &bcount) ||
                bcount != caf_chunk_header.mChunkSize) {
                    error_line ("%s is not a valid .CAF file!", infilename);
                    free (caf_channel_layout);
                    return WAVPACK_SOFT_ERROR;
            }
            else if (!(config->qmode & QMODE_NO_STORE_WRAPPER) &&
                !WavpackAddWrapper (wpc, caf_channel_layout, (uint32_t) caf_chunk_header.mChunkSize)) {
                    error_line ("%s", WavpackGetErrorMessage (wpc));
                    free (caf_channel_layout);
                    return WAVPACK_SOFT_ERROR;
            }

            WavpackBigEndianToNative (caf_channel_layout, CAFChannelLayoutFormat);
            chan_chunk = 1;

            if (config->channel_mask || (config->qmode & QMODE_CHANS_UNASSIGNED)) {
                error_line ("this CAF file already has channel order information!");
                free (caf_channel_layout);
                return WAVPACK_SOFT_ERROR;
            }

            switch (caf_channel_layout->mChannelLayoutTag) {
                case kCAFChannelLayoutTag_UseChannelDescriptions:
                    {
                        CAFChannelDescription *descriptions = (CAFChannelDescription *) (caf_channel_layout + 1);
                        int num_descriptions = caf_channel_layout->mNumberChannelDescriptions;
                        int label, cindex = 0, idents = 0;

                        if (caf_chunk_header.mChunkSize != sizeof (CAFChannelLayout) + sizeof (CAFChannelDescription) * num_descriptions ||
                            num_descriptions != config->num_channels) {
                                error_line ("channel descriptions in 'chan' chunk are the wrong size!");
                                free (caf_channel_layout);
                                return WAVPACK_SOFT_ERROR;
                        }

                        if (num_descriptions >= 256) {
                            error_line ("%d channel descriptions is more than we can handle...ignoring!");
                            break;
                        }

                        // we allocate (and initialize to invalid values) a channel reorder array
                        // (even though we might not end up doing any reordering) and a string for
                        // any non-Microsoft channels we encounter

                        channel_reorder = malloc (num_descriptions);
                        memset (channel_reorder, -1, num_descriptions);
                        channel_identities = malloc (num_descriptions+1);

                        // convert the descriptions array to our native endian so it's easy to access

                        for (i = 0; i < num_descriptions; ++i) {
                            WavpackBigEndianToNative (descriptions + i, CAFChannelDescriptionFormat);

                            if (debug_logging_mode)
                                error_line ("chan %d --> %d", i + 1, descriptions [i].mChannelLabel);
                        }

                        // first, we go though and find any MS channels present, and move those to the beginning

                        for (label = 1; label <= 18; ++label)
                            for (i = 0; i < num_descriptions; ++i)
                                if (descriptions [i].mChannelLabel == label) {
                                    config->channel_mask |= 1 << (label - 1);
                                    channel_reorder [i] = cindex++;
                                    break;
                                }

                        // next, we go though the channels again assigning any we haven't done

                        for (i = 0; i < num_descriptions; ++i)
                            if (channel_reorder [i] == (unsigned char) -1) {
                                uint32_t clabel = descriptions [i].mChannelLabel;

                                if (clabel == 0 || clabel == 0xffffffff || clabel == 100)
                                    channel_identities [idents++] = 0xff;
                                else if ((clabel >= 33 && clabel <= 44) || (clabel >= 200 && clabel <= 207) || (clabel >= 301 && clabel <= 305))
                                    channel_identities [idents++] = clabel >= 301 ? clabel - 80 : clabel;
                                else {
                                    error_line ("warning: unknown channel descriptions label: %d", clabel);
                                    channel_identities [idents++] = 0xff;
                                }

                                channel_reorder [i] = cindex++;
                            }

                        // then, go through the reordering array and see if we really have to reorder

                        for (i = 0; i < num_descriptions; ++i)
                            if (channel_reorder [i] != i)
                                break;

                        if (i == num_descriptions) {
                            free (channel_reorder);                 // no reordering required, so don't
                            channel_reorder = NULL;
                        }
                        else {
                            config->qmode |= QMODE_REORDERED_CHANS; // reordering required, put channel count into layout
                            channel_layout = num_descriptions;
                        }

                        if (!idents) {                              // if no non-MS channels, free the identities string
                            free (channel_identities);
                            channel_identities = NULL;
                        }
                        else
                            channel_identities [idents] = 0;        // otherwise NULL terminate it

                        if (debug_logging_mode) {
                            error_line ("layout_tag = 0x%08x, so generated bitmap of 0x%08x from %d descriptions, %d non-MS",
                                caf_channel_layout->mChannelLayoutTag, config->channel_mask,
                                caf_channel_layout->mNumberChannelDescriptions, idents);

                            // if debugging, display the reordering as a string (but only little ones)

                            if (channel_reorder && num_descriptions <= 8) {
                                char reorder_string [] = "12345678";

                                for (i = 0; i < num_descriptions; ++i)
                                    reorder_string [i] = channel_reorder [i] + '1';

                                reorder_string [i] = 0;
                                error_line ("reordering string = \"%s\"\n", reorder_string);
                            }
                        }
                    }

                    break;

                case kCAFChannelLayoutTag_UseChannelBitmap:
                    config->channel_mask = caf_channel_layout->mChannelBitmap;

                    if (debug_logging_mode)
                        error_line ("layout_tag = 0x%08x, so using supplied bitmap of 0x%08x",
                            caf_channel_layout->mChannelLayoutTag, caf_channel_layout->mChannelBitmap);

                    break;

                default:
                    for (i = 0; i < NUM_LAYOUTS; ++i)
                        if (caf_channel_layout->mChannelLayoutTag == layouts [i].mChannelLayoutTag) {
                            config->channel_mask = layouts [i].mChannelBitmap;
                            channel_layout = layouts [i].mChannelLayoutTag;

                            if (layouts [i].mChannelReorder) {
                                channel_reorder = (unsigned char *) strdup (layouts [i].mChannelReorder);
                                config->qmode |= QMODE_REORDERED_CHANS;
                            }

                            if (layouts [i].mChannelIdentities)
                                channel_identities = (unsigned char *) strdup (layouts [i].mChannelIdentities);

                            if (debug_logging_mode)
                                error_line ("layout_tag 0x%08x found in table, bitmap = 0x%08x, reorder = %s, identities = %s",
                                    channel_layout, config->channel_mask, channel_reorder ? "yes" : "no", channel_identities ? "yes" : "no");

                            break;
                        }

                    if (i == NUM_LAYOUTS && debug_logging_mode)
                        error_line ("layout_tag 0x%08x not found in table...all channels unassigned",
                            caf_channel_layout->mChannelLayoutTag);

                    break;
            }

            free (caf_channel_layout);
        }
        else if (!strncmp (caf_chunk_header.mChunkType, "data", 4)) {     // on the data chunk, get size and exit loop
            uint32_t mEditCount;

            if (!DoReadFile (infile, &mEditCount, sizeof (mEditCount), &bcount) ||
                bcount != sizeof (mEditCount)) {
                    error_line ("%s is not a valid .CAF file!", infilename);
                    return WAVPACK_SOFT_ERROR;
            }
            else if (!(config->qmode & QMODE_NO_STORE_WRAPPER) &&
                !WavpackAddWrapper (wpc, &mEditCount, sizeof (mEditCount))) {
                    error_line ("%s", WavpackGetErrorMessage (wpc));
                    return WAVPACK_SOFT_ERROR;
            }

            if ((config->qmode & QMODE_IGNORE_LENGTH) || caf_chunk_header.mChunkSize == -1) {
                config->qmode |= QMODE_IGNORE_LENGTH;

                if (infilesize && DoGetFilePosition (infile) != -1)
                    total_samples = (infilesize - DoGetFilePosition (infile)) / caf_audio_format.mBytesPerPacket;
                else
                    total_samples = -1;
            }
            else {
                if (infilesize && infilesize - caf_chunk_header.mChunkSize > 16777216) {
                    error_line (".CAF file %s has over 16 MB of extra CAFF data, probably is corrupt!", infilename);
                    return WAVPACK_SOFT_ERROR;
                }

                if ((caf_chunk_header.mChunkSize - 4) % caf_audio_format.mBytesPerPacket) {
                    error_line (".CAF file %s has an invalid data chunk size, probably is corrupt!", infilename);
                    return WAVPACK_SOFT_ERROR;
                }

                total_samples = (caf_chunk_header.mChunkSize - 4) / caf_audio_format.mBytesPerPacket;

                if (!total_samples) {
                    error_line ("this .CAF file has no audio samples, probably is corrupt!");
                    return WAVPACK_SOFT_ERROR;
                }

                if (total_samples > MAX_WAVPACK_SAMPLES) {
                    error_line ("%s has too many samples for WavPack!", infilename);
                    return WAVPACK_SOFT_ERROR;
                }
            }

            break;
        }
        else {          // just copy unknown chunks to output file

            int bytes_to_copy = (uint32_t) caf_chunk_header.mChunkSize;
            char *buff = malloc (bytes_to_copy);

            if (debug_logging_mode)
                error_line ("extra unknown chunk \"%c%c%c%c\" of %d bytes",
                    caf_chunk_header.mChunkType [0], caf_chunk_header.mChunkType [1], caf_chunk_header.mChunkType [2],
                    caf_chunk_header.mChunkType [3], caf_chunk_header.mChunkSize);

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

    if (!chan_chunk && !config->channel_mask && config->num_channels <= 2 && !(config->qmode & QMODE_CHANS_UNASSIGNED))
        config->channel_mask = 0x5 - config->num_channels;

    if (!WavpackSetConfiguration64 (wpc, config, total_samples, channel_identities)) {
        error_line ("%s", WavpackGetErrorMessage (wpc));
        return WAVPACK_SOFT_ERROR;
    }

    if (channel_identities)
        free (channel_identities);

    if (channel_layout || channel_reorder) {
        if (!WavpackSetChannelLayout (wpc, channel_layout, channel_reorder)) {
            error_line ("problem with setting channel layout (should not happen)");
            return WAVPACK_SOFT_ERROR;
        }

        if (channel_reorder)
            free (channel_reorder);
    }

    return WAVPACK_NO_ERROR;
}

int WriteCaffHeader (FILE *outfile, WavpackContext *wpc, int64_t total_samples, int qmode)
{
    CAFChunkHeader caf_desc_chunk_header, caf_chan_chunk_header, caf_data_chunk_header;
    CAFChannelLayout caf_channel_layout;
    CAFAudioFormat caf_audio_format;
    CAFFileHeader caf_file_header;
    uint32_t mEditCount, bcount;

    int num_channels = WavpackGetNumChannels (wpc);
    int32_t channel_mask = WavpackGetChannelMask (wpc);
    int32_t sample_rate = WavpackGetSampleRate (wpc);
    int bytes_per_sample = WavpackGetBytesPerSample (wpc);
    int bits_per_sample = WavpackGetBitsPerSample (wpc);
    int float_norm_exp = WavpackGetFloatNormExp (wpc);
    uint32_t channel_layout_tag = WavpackGetChannelLayout (wpc, NULL);
    unsigned char *channel_identities = malloc (num_channels + 1);
    int num_identified_chans, i;

    if (float_norm_exp && float_norm_exp != 127) {
        error_line ("can't create valid CAFF header for non-normalized floating data!");
        free (channel_identities);
        return FALSE;
    }

    // get the channel identities (including Microsoft) and count up the defined ones

    WavpackGetChannelIdentities (wpc, channel_identities);

    for (num_identified_chans = i = 0; i < num_channels; ++i)
        if (channel_identities [i] != 0xff)
            num_identified_chans++;

    // format and write the CAF File Header

    strncpy (caf_file_header.mFileType, "caff", sizeof (caf_file_header.mFileType));
    caf_file_header.mFileVersion = 1;
    caf_file_header.mFileFlags = 0;
    WavpackNativeToBigEndian (&caf_file_header, CAFFileHeaderFormat);

    if (!DoWriteFile (outfile, &caf_file_header, sizeof (caf_file_header), &bcount) ||
        bcount != sizeof (caf_file_header))
            return FALSE;

    // format and write the Audio Description Chunk

    strncpy (caf_desc_chunk_header.mChunkType, "desc", sizeof (caf_desc_chunk_header.mChunkType));
    caf_desc_chunk_header.mChunkSize = sizeof (caf_audio_format);
    WavpackNativeToBigEndian (&caf_desc_chunk_header, CAFChunkHeaderFormat);

    if (!DoWriteFile (outfile, &caf_desc_chunk_header, sizeof (caf_desc_chunk_header), &bcount) ||
        bcount != sizeof (caf_desc_chunk_header))
            return FALSE;

    caf_audio_format.mSampleRate = (double) sample_rate;
    strncpy (caf_audio_format.mFormatID, "lpcm", sizeof (caf_audio_format.mFormatID));
    caf_audio_format.mFormatFlags = float_norm_exp ? CAF_FORMAT_FLOAT : 0;

    if (!(qmode & QMODE_BIG_ENDIAN))
        caf_audio_format.mFormatFlags |= CAF_FORMAT_LITTLE_ENDIAN;

    caf_audio_format.mBytesPerPacket = bytes_per_sample * num_channels;
    caf_audio_format.mFramesPerPacket = 1;
    caf_audio_format.mChannelsPerFrame = num_channels;
    caf_audio_format.mBitsPerChannel = bits_per_sample;
    WavpackNativeToBigEndian (&caf_audio_format, CAFAudioFormatFormat);

    if (!DoWriteFile (outfile, &caf_audio_format, sizeof (caf_audio_format), &bcount) ||
        bcount != sizeof (caf_audio_format))
            return FALSE;

    // we write the Channel Layout Chunk if any of these are true:
    // 1. a specific CAF layout was specified (100 - 147)
    // 2. there are more than 2 channels and ANY are defined
    // 3. there are 1 or 2 channels and NOT regular mono/stereo

    if (channel_layout_tag || (num_channels > 2 ? num_identified_chans : channel_mask != 5 - num_channels)) {
        int bits = 0, bmask;

        for (bmask = 1; bmask; bmask <<= 1)     // count the set bits in the channel mask
            if (bmask & channel_mask)
                ++bits;

        // we use a layout tag if there is a specific CAF layout (100 - 147) or
        // all the channels are MS defined and in MS order...otherwise we have to
        // write a full channel description array

        if ((channel_layout_tag & 0xff0000) || (bits == num_channels && !(qmode & QMODE_REORDERED_CHANS))) {

            strncpy (caf_chan_chunk_header.mChunkType, "chan", sizeof (caf_chan_chunk_header.mChunkType));
            caf_chan_chunk_header.mChunkSize = sizeof (caf_channel_layout);
            WavpackNativeToBigEndian (&caf_chan_chunk_header, CAFChunkHeaderFormat);

            if (!DoWriteFile (outfile, &caf_chan_chunk_header, sizeof (caf_chan_chunk_header), &bcount) ||
                bcount != sizeof (caf_chan_chunk_header))
                    return FALSE;

            if (channel_layout_tag) {
                if (debug_logging_mode)
                    error_line ("writing \"chan\" chunk with layout tag 0x%08x", channel_layout_tag);

                caf_channel_layout.mChannelLayoutTag = channel_layout_tag;
                caf_channel_layout.mChannelBitmap = 0;
            }
            else {
                if (debug_logging_mode)
                    error_line ("writing \"chan\" chunk with UseChannelBitmap tag, bitmap = 0x%08x", channel_mask);

                caf_channel_layout.mChannelLayoutTag = kCAFChannelLayoutTag_UseChannelBitmap;
                caf_channel_layout.mChannelBitmap = channel_mask;
            }

            caf_channel_layout.mNumberChannelDescriptions = 0;
            WavpackNativeToBigEndian (&caf_channel_layout, CAFChannelLayoutFormat);

            if (!DoWriteFile (outfile, &caf_channel_layout, sizeof (caf_channel_layout), &bcount) ||
                bcount != sizeof (caf_channel_layout))
                    return FALSE;
        }
        else {  // write a channel description array because a single layout or bitmap won't do it...
            CAFChannelDescription caf_channel_description;
            unsigned char *new_channel_order = NULL;
            int i;

            if (debug_logging_mode)
                error_line ("writing \"chan\" chunk with UseChannelDescriptions tag, bitmap = 0x%08x, reordered = %s",
                    channel_mask, (qmode & QMODE_REORDERED_CHANS) ? "yes" : "no");

            if (qmode & QMODE_REORDERED_CHANS) {
                if ((int)(channel_layout_tag & 0xff) <= num_channels) {
                    new_channel_order = malloc (num_channels);

                    for (i = 0; i < num_channels; ++i)
                        new_channel_order [i] = i;

                    WavpackGetChannelLayout (wpc, new_channel_order);
                }
            }

            strncpy (caf_chan_chunk_header.mChunkType, "chan", sizeof (caf_chan_chunk_header.mChunkType));
            caf_chan_chunk_header.mChunkSize = sizeof (caf_channel_layout) + sizeof (caf_channel_description) * num_channels;
            WavpackNativeToBigEndian (&caf_chan_chunk_header, CAFChunkHeaderFormat);

            if (!DoWriteFile (outfile, &caf_chan_chunk_header, sizeof (caf_chan_chunk_header), &bcount) ||
                bcount != sizeof (caf_chan_chunk_header))
                    return FALSE;

            caf_channel_layout.mChannelLayoutTag = kCAFChannelLayoutTag_UseChannelDescriptions;
            caf_channel_layout.mChannelBitmap = 0;
            caf_channel_layout.mNumberChannelDescriptions = num_channels;
            WavpackNativeToBigEndian (&caf_channel_layout, CAFChannelLayoutFormat);

            if (!DoWriteFile (outfile, &caf_channel_layout, sizeof (caf_channel_layout), &bcount) ||
                bcount != sizeof (caf_channel_layout))
                    return FALSE;

            for (i = 0; i < num_channels; ++i) {
                unsigned char chan_id = new_channel_order ? channel_identities [new_channel_order [i]] : channel_identities [i];
                CLEAR (caf_channel_description);

                if ((chan_id >= 1 && chan_id <= 18) || (chan_id >= 33 && chan_id <= 44) || (chan_id >= 200 && chan_id <= 207))
                    caf_channel_description.mChannelLabel = chan_id;
                else if (chan_id >= 221 && chan_id <= 225)
                    caf_channel_description.mChannelLabel = chan_id + 80;

                if (debug_logging_mode)
                    error_line ("chan %d --> %d", i + 1, caf_channel_description.mChannelLabel);

                WavpackNativeToBigEndian (&caf_channel_description, CAFChannelDescriptionFormat);

                if (!DoWriteFile (outfile, &caf_channel_description, sizeof (caf_channel_description), &bcount) ||
                    bcount != sizeof (caf_channel_description))
                        return FALSE;
            }

            if (new_channel_order)
                free (new_channel_order);
        }
    }

    // format and write the Audio Data Chunk

    strncpy (caf_data_chunk_header.mChunkType, "data", sizeof (caf_data_chunk_header.mChunkType));

    if (total_samples == -1)
        caf_data_chunk_header.mChunkSize = -1;
    else
        caf_data_chunk_header.mChunkSize = (total_samples * bytes_per_sample * num_channels) + sizeof (mEditCount);

    WavpackNativeToBigEndian (&caf_data_chunk_header, CAFChunkHeaderFormat);

    if (!DoWriteFile (outfile, &caf_data_chunk_header, sizeof (caf_data_chunk_header), &bcount) ||
        bcount != sizeof (caf_data_chunk_header))
            return FALSE;

    mEditCount = 0;
    WavpackNativeToBigEndian (&mEditCount, "L");

    if (!DoWriteFile (outfile, &mEditCount, sizeof (mEditCount), &bcount) ||
        bcount != sizeof (mEditCount))
            return FALSE;

    free (channel_identities);

    return TRUE;
}
