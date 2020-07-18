////////////////////////////////////////////////////////////////////////////
//                           **** WAVPACK ****                            //
//                  Hybrid Lossless Wavefile Compressor                   //
//                Copyright (c) 1998 - 2017 David Bryant                  //
//                          All Rights Reserved.                          //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

// import_id3.c

// This module provides limited support for importing existing ID3 tags
// (from DSF files, for example) into WavPack files

#include <sys/stat.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "wavpack.h"

static struct {
    char *id3_item, *ape_item;
} text_tag_table [] = {
    { "TALB", "Album" },
    { "TPE1", "Artist" },
    { "TPE2", "AlbumArtist" },
    { "TPE3", "Conductor" },
    { "TIT1", "Grouping" },
    { "TIT2", "Title" },
    { "TIT3", "Subtitle" },
    { "TSST", "DiscSubtitle" },
    { "TSOA", "AlbumSort" },
    { "TSOT", "TitleSort" },
    { "TSO2", "AlbumArtistSort" },
    { "TSOP", "ArtistSort" },
    { "TPOS", "Disc" },
    { "TRCK", "Track" },
    { "TCON", "Genre" },
    { "TYER", "Year" },
    { "TCOM", "Composer" },
    { "TPUB", "Publisher" },
    { "TCMP", "Compilation" },
    { "TENC", "EncodedBy" },
    { "TEXT", "Lyricist" },
    { "TCOP", "Copyright" },
    { "TLAN", "Language" },
    { "TSRC", "ISRC" },
    { "TMED", "Media" },
    { "TMOO", "Mood" },
    { "TBPM", "BPM" }
};

#define NUM_TEXT_TAG_ITEMS (sizeof (text_tag_table) / sizeof (text_tag_table [0]))

static int WideCharToUTF8 (const uint16_t *Wide, unsigned char *pUTF8, int len);
static void Latin1ToUTF8 (void *string, int len);

// Import specified ID3v2.3 tag. The WavPack context accepts the tag items, and can be
// NULL for doing a dry-run through the tag. If errors occur then a description will be
// written to "error" (which must be 80 characters) and -1 will be returned. If no
// errors occur then the number of tag items successfully written will be returned, or
// zero in the case of no applicable tags. An optional integer pointer can be provided
// to accept the total number of bytes consumed by the tag (name and value).

static int ImportID3v2_syncsafe (WavpackContext *wpc, unsigned char *tag_data, int tag_size, char *error, int32_t *bytes_used, int syncsafe)
{
    int tag_size_from_header, items_imported = 0, done_cover = 0;
    unsigned char id3_header [10];

    if (bytes_used)
        *bytes_used = 0;

    if (tag_size < sizeof (id3_header)) {
        strcpy (error, "can't read tag header");
        return -1;
    }

    memcpy (id3_header, tag_data, sizeof (id3_header));
    tag_size -= sizeof (id3_header);
    tag_data += sizeof (id3_header);

    if (strncmp ((char *) id3_header, "ID3", 3)) {
        strcpy (error, "no ID3v2 tag found");
        return -1;
    }

    if (id3_header [3] != 3 || id3_header [4] == 0xFF || (id3_header [5] & 0x1F)) {
        strcpy (error, "not valid ID3v2.3");
        return -1;
    }

    if (id3_header [5] & 0x80) {
        strcpy (error, "unsynchonization detected");
        return -1;
    }

    if (id3_header [5] & 0x40) {
        strcpy (error, "extended header detected");
        return -1;
    }

    if (id3_header [5] & 0x20) {
        strcpy (error, "experimental indicator detected");
        return -1;
    }

    if ((id3_header [6] | id3_header [7] | id3_header [8] | id3_header [9]) & 0x80) {
        strcpy (error, "not valid ID3v2.3 (bad size)");
        return -1;
    }

    tag_size_from_header = id3_header [9] + (id3_header [8] << 7) + (id3_header [7] << 14) + (id3_header [6] << 21);

    if (tag_size_from_header > tag_size) {
        strcpy (error, "tag is truncated");
        return -1;
    }

    while (1) {
        unsigned char frame_header [10], *frame_body;
        int frame_size, i;

        if (tag_size < sizeof (frame_header))
            break;

        memcpy (frame_header, tag_data, sizeof (frame_header));
        tag_size -= sizeof (frame_header);
        tag_data += sizeof (frame_header);

        if (!frame_header [0] && !frame_header [1] && !frame_header [2] && !frame_header [3])
            break;

        for (i = 0; i < 4; ++i)
            if (frame_header [i] < '0' ||
                (frame_header [i] > '9' && frame_header [i] < 'A') ||
                frame_header [i] > 'Z') {
                    strcpy (error, "bad frame identity");
                    return -1;
            }

        if (frame_header [9]) {
            strcpy (error, "unknown frame_header flag set");
            return -1;
        }

        if (syncsafe)
            frame_size = frame_header [7] + (frame_header [6] << 7) + (frame_header [5] << 14) + (frame_header [4] << 21);
        else
            frame_size = frame_header [7] + (frame_header [6] << 8) + (frame_header [5] << 16) + (frame_header [4] << 24);

        if (!frame_size) {
            strcpy (error, "empty frame not allowed");
            return -1;
        }

        if (frame_size > tag_size) {
            strcpy (error, "can't read frame body");
            return -1;
        }

        frame_body = malloc (frame_size + 4);

        memcpy (frame_body, tag_data, frame_size);
        tag_size -= frame_size;
        tag_data += frame_size;

        if (frame_header [0] == 'T') {
            int txxx_mode = !strncmp ((char *) frame_header, "TXXX", 4), si = 0;
            unsigned char *utf8_strings [2];

            if (frame_body [0] == 0) {
                unsigned char *fp = frame_body + 1, *fe = frame_body + frame_size;

                while (si < 2 && fp < fe && *fp) {
                    utf8_strings [si] = malloc (frame_size * 3);

                    for (i = 0; fp < fe; ++i)
                        if (!(utf8_strings [si] [i] = *fp++))
                            break;

                    if (fp == fe)
                        utf8_strings [si] [i] = 0;

                    Latin1ToUTF8 (utf8_strings [si++], frame_size * 3);
                }
            }
            else if (frame_body [0] == 1) {
                unsigned char *fp = frame_body + 1, *fe = frame_body + frame_size;
                uint16_t *wide_string = malloc (frame_size);

                while (si < 2 && fp <= fe - 4 && fp [0] == 0xFF && fp [1] == 0xFE && (fp [2] | fp [3])) {
                    utf8_strings [si] = malloc (frame_size * 2);
                    fp += 2;

                    for (i = 0; fp <= fe - 2; ++i, fp += 2)
                        if (!(wide_string [i] = fp [0] | (fp [1] << 8))) {
                            fp += 2;
                            break;
                        }

                    wide_string [i] = 0;
                    WideCharToUTF8 (wide_string, utf8_strings [si++], frame_size * 2);
                }

                free (wide_string);
            }
            else {
                strcpy (error, "unknown character encoding");
                return -1;
            }

            // if we got a text string (or a TXXX and two text strings) store them here

            if (si) {
                if (txxx_mode && si == 2) {
                    unsigned char *cptr = utf8_strings [0];

                    // if all single-byte UTF8, format TXXX description to match case of regular APEv2 descriptions (e.g., Performer)

                    while (*cptr)
                        if (*cptr & 0x80)
                            break;
                        else
                            cptr++;

                    if (!*cptr && isupper (*utf8_strings [0])) {
                        cptr = utf8_strings [0];

                        while (*++cptr)
                            if (isupper (*cptr))
                                *cptr = tolower (*cptr);
                    }

                    if (wpc && !WavpackAppendTagItem (wpc, (char *) utf8_strings [0], (char *) utf8_strings [1], (int) strlen ((char *) utf8_strings [1]))) {
                        strcpy (error, WavpackGetErrorMessage (wpc));
                        return -1;
                    }

                    items_imported++;
                    if (bytes_used) *bytes_used += (int) (strlen ((char *) utf8_strings [0]) + strlen ((char *) utf8_strings [1]) + 1);
                }
                else if (!txxx_mode && si == 1)    // if not TXXX, look up item in the table to find APEv2 item name
                    for (i = 0; i < NUM_TEXT_TAG_ITEMS; ++i)
                        if (!strncmp ((char *) frame_header, text_tag_table [i].id3_item, 4)) {
                            if (wpc && !WavpackAppendTagItem (wpc, text_tag_table [i].ape_item, (char *) utf8_strings [0], (int) strlen ((char *) utf8_strings [0]))) {
                                strcpy (error, WavpackGetErrorMessage (wpc));
                                return -1;
                            }

                            items_imported++;
                            if (bytes_used) *bytes_used += (int) (strlen ((char *) utf8_strings [0]) + strlen (text_tag_table [i].ape_item) + 1);
                        }

                do
                    free (utf8_strings [--si]);
                while (si);
            }
        }
        else if (!strncmp ((char *) frame_header, "APIC", 4)) {
            if (frame_body [0] == 0) {
                char *mime_type, *extension, *item = NULL;
                unsigned char *frame_ptr = frame_body + 1;
                int frame_bytes = frame_size - 1;
                unsigned char picture_type;

                mime_type = (char *) frame_ptr;

                while (frame_bytes-- && *frame_ptr++);

                if (frame_bytes < 0) {
                    strcpy (error, "unterminated picture mime type");
                    return -1;
                }

                if (frame_bytes == 0) {
                    strcpy (error, "no picture type");
                    return -1;
                }

                picture_type = *frame_ptr++;
                frame_bytes--;

                while (frame_bytes-- && *frame_ptr++);

                if (frame_bytes < 0) {
                    strcpy (error, "unterminated picture description");
                    return -1;
                }

                if (frame_bytes < 2) {
                    strcpy (error, "no picture data");
                    return -1;
                }

                if (strstr (mime_type, "jpeg") || strstr (mime_type, "JPEG"))
                    extension = ".jpg";
                else if (strstr (mime_type, "png") || strstr (mime_type, "PNG"))
                    extension = ".png";
                else if (frame_ptr [0] == 0xFF && frame_ptr [1] == 0xD8)
                    extension = ".jpg";
                else if (frame_ptr [0] == 0x89 && frame_ptr [1] == 0x50)
                    extension = ".png";
                else
                    extension = "";

                if (picture_type == 3) {
                    item = "Cover Art (Front)";
                    done_cover = 1;
                }
                else if (picture_type == 4)
                    item = "Cover Art (Back)";
                else if (picture_type != 1 && picture_type != 2 && !done_cover) {
                    item = "Cover Art (Front)";
                    done_cover = 1;
                }

                if (item) {
                    int binary_tag_size = (int) strlen (item) + (int) strlen (extension) + 1 + frame_bytes;
                    char *binary_tag_image = malloc (binary_tag_size);

                    strcpy (binary_tag_image, item);
                    strcat (binary_tag_image, extension);
                    memcpy (binary_tag_image + binary_tag_size - frame_bytes, frame_ptr, frame_bytes);

                    if (wpc && !WavpackAppendBinaryTagItem (wpc, item, binary_tag_image, binary_tag_size)) {
                        strcpy (error, WavpackGetErrorMessage (wpc));
                        return -1;
                    }

                    items_imported++;
                    if (bytes_used) *bytes_used += (int) strlen (item) + 1 + binary_tag_size;
                    free (binary_tag_image);
                }
            }
            else {
                strcpy (error, "unhandled APIC character encoding");
                return -1;
            }
        }

        free (frame_body);
    }

    return items_imported;
}

int ImportID3v2 (WavpackContext *wpc, unsigned char *tag_data, int tag_size, char *error, int32_t *bytes_used)
{
    int res, res_ss;

    if (bytes_used)
        *bytes_used = 0;

    // look for the ID3 tag in case it's not first thing in the wrapper (like in WAV or DSDIFF files)

    if (tag_size >= 10) {
        unsigned char *cp = tag_data, *ce = cp + tag_size;

        while (cp < ce - 10)
            if (cp [0] == 'I' && cp [1] == 'D' && cp [2] == '3' && cp [3] == 3) {
                tag_size = (int)(ce - cp);
                tag_data = cp;
                break;
            }
            else
                cp++;

        if (cp == ce - 10)      // no tag found is NOT an error
            return 0;
    }

    res = ImportID3v2_syncsafe (NULL, tag_data, tag_size, error, bytes_used, 0);

    if (res > 0)
        return wpc ? ImportID3v2_syncsafe (wpc, tag_data, tag_size, error, bytes_used, 0) : res;

    res_ss = ImportID3v2_syncsafe (NULL, tag_data, tag_size, error, bytes_used, 1);

    if (res_ss > 0)
        return wpc ? ImportID3v2_syncsafe (wpc, tag_data, tag_size, error, bytes_used, 1) : res_ss;

    return res;
}

// Convert the Unicode wide-format string into a UTF-8 string using no more
// than the specified buffer length. The wide-format string must be NULL
// terminated and the resulting string will be NULL terminated. The actual
// number of characters converted (not counting terminator) is returned, which
// may be less than the number of characters in the wide string if the buffer
// length is exceeded.

static int WideCharToUTF8 (const uint16_t *Wide, unsigned char *pUTF8, int len)
{
    const uint16_t *pWide = Wide;
    int outndx = 0;

    while (*pWide) {
        if (*pWide < 0x80 && outndx + 1 < len)
            pUTF8 [outndx++] = (unsigned char) *pWide++;
        else if (*pWide < 0x800 && outndx + 2 < len) {
            pUTF8 [outndx++] = (unsigned char) (0xc0 | ((*pWide >> 6) & 0x1f));
            pUTF8 [outndx++] = (unsigned char) (0x80 | (*pWide++ & 0x3f));
        }
        else if (outndx + 3 < len) {
            pUTF8 [outndx++] = (unsigned char) (0xe0 | ((*pWide >> 12) & 0xf));
            pUTF8 [outndx++] = (unsigned char) (0x80 | ((*pWide >> 6) & 0x3f));
            pUTF8 [outndx++] = (unsigned char) (0x80 | (*pWide++ & 0x3f));
        }
        else
            break;
    }

    pUTF8 [outndx] = 0;
    return (int)(pWide - Wide);
}

// Convert a Latin1 string into its Unicode UTF-8 format equivalent. The
// conversion is done in-place so the maximum length of the string buffer must
// be specified because the string may become longer or shorter. If the
// resulting string will not fit in the specified buffer size then it is
// truncated.

#ifdef _WIN32

#include <windows.h>

static void Latin1ToUTF8 (void *string, int len)
{
    int max_chars = (int) strlen (string);
    uint16_t *temp = (uint16_t *) malloc ((max_chars + 1) * sizeof (uint16_t));

    MultiByteToWideChar (28591, 0, string, -1, temp, max_chars + 1);
    WideCharToUTF8 (temp, (unsigned char *) string, len);
    free (temp);
}

#else

#include <iconv.h>

static void Latin1ToUTF8 (void *string, int len)
{
    char *temp = malloc (len);
    char *outp = temp;
    char *inp = string;
    size_t insize = 0;
    size_t outsize = len - 1;
    int err = 0;
    iconv_t converter;

    memset(temp, 0, len);

    insize = strlen (string);
    converter = iconv_open ("UTF-8", "ISO-8859-1");

    if (converter != (iconv_t) -1) {
        err = iconv (converter, &inp, &insize, &outp, &outsize);
        iconv_close (converter);
    }
    else
        err = -1;

    if (err == -1) {
        free(temp);
        return;
    }

    memmove (string, temp, len);
    free (temp);
}

#endif

