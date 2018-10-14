/*====================================================================*
 -  Copyright (C) 2001 Leptonica.  All rights reserved.
 -
 -  Redistribution and use in source and binary forms, with or without
 -  modification, are permitted provided that the following conditions
 -  are met:
 -  1. Redistributions of source code must retain the above copyright
 -     notice, this list of conditions and the following disclaimer.
 -  2. Redistributions in binary form must reproduce the above
 -     copyright notice, this list of conditions and the following
 -     disclaimer in the documentation and/or other materials
 -     provided with the distribution.
 -
 -  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 -  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 -  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 -  A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ANY
 -  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 -  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 -  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 -  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 -  OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 -  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 -  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *====================================================================*/

/*
 * corrupttest.c
 *
 *     Excises or permutes a given fraction of bytes, starting from
 *     a specified location.  The parameters @loc and @size are fractions
 *     of the entire file (between 0.0 and 1.0).
 *
 *     Syntax:  corrupttest <file> [loc size]
 *
 *     Use: "fuzz testing" jpeg and png reading, under corruption by
 *     random byte permutation or by deletion of part of the compressed file.
 *
 *     For example,
 *          corrupttest rabi.png 0.0001 0.0001
 *     which tests read functions on rabi.png after 23 bytes (0.01%)
 *     starting at byte 23 have been randomly permuted, emits the following:
 *      > Info in fileCorruptByMutation: Randomizing 23 bytes at location 23
 *      > libpng error: IHDR: CRC error
 *      > Error in pixReadMemPng: internal png error
 *      > Error in pixReadStream: png: no pix returned
 *      > Error in pixRead: pix not read
 *      > libpng error: IHDR: CRC error
 *      > Error in fgetPngResolution: internal png error
 */

#include "string.h"
#include "allheaders.h"

    /* ------------------------------------------------------------------ */
    /*  Set this to TRUE for deletion; FALSE for random byte permutation  */
    /* ------------------------------------------------------------------ */
// static const l_int32  Deletion = TRUE;
static const l_int32  Deletion = FALSE;

int main(int     argc,
         char  **argv)
{
size_t       filesize;
l_float32    loc, size;
l_int32      i, j, w, xres, yres, format, ret, nwarn, hint;
l_uint8     *comment, *filedata;
char        *filein;
FILE        *fp;
PIX         *pix;
static char  mainName[] = "corrupttest";

    if (argc != 2 && argc != 4)
        return ERROR_INT("syntax: corrupttest filein [loc size]", mainName, 1);
    filein = argv[1];
    findFileFormat(filein, &format);

    setLeptDebugOK(1);
    lept_mkdir("lept/corrupt");

    hint = 0;
    if (argc == 4) {  /* Single test */
        loc = atof(argv[2]);
        size = atof(argv[3]);
        if (Deletion == TRUE) {
            fileCorruptByDeletion(filein, loc, size,
                                  "/tmp/lept/corrupt/junkout");
        } else {  /* mutation */
            fileCorruptByMutation(filein, loc, size,
                                  "/tmp/lept/corrupt/junkout");
        }
        fp = fopenReadStream("/tmp/lept/corrupt/junkout");
        if (format == IFF_JFIF_JPEG) {
            if ((pix = pixReadJpeg("/tmp/lept/corrupt/junkout", 0, 1,
                                   &nwarn, hint)) != NULL) {
                pixDisplay(pix, 100, 100);
                pixDestroy(&pix);
            }
            freadHeaderJpeg(fp, &w, NULL, NULL, NULL, NULL);
            fgetJpegResolution(fp, &xres, &yres);
            ret = fgetJpegComment(fp, &comment);
            if (!ret && comment) {
                fprintf(stderr, "comment: %s\n", comment);
                lept_free(comment);
            }
        } else if (format == IFF_PNG) {
            if ((pix = pixRead("/tmp/lept/corrupt/junkout")) != NULL) {
                pixDisplay(pix, 100, 100);
                pixDestroy(&pix);
            }
            freadHeaderPng(fp, &w, NULL, NULL, NULL, NULL);
            fgetPngResolution(fp, &xres, &yres);
        } else if (format == IFF_PNM) {
            freadHeaderPnm(fp, &w, NULL, NULL, NULL, NULL, NULL);
        }
        fclose(fp);
        return 0;
    }

        /* Multiple test (argc == 2) */
    for (i = 0; i < 50; i++) {
        loc = 0.02 * i;
        for (j = 1; j < 11; j++) {
            size = 0.001 * j;

                /* Write corrupt file */
            if (Deletion == TRUE) {
                fileCorruptByDeletion(filein, loc, size,
                                      "/tmp/lept/corrupt/junkout");
            } else {
                fileCorruptByMutation(filein, loc, size,
                                      "/tmp/lept/corrupt/junkout");
            }

                /* Attempt to read the file */
            pix = NULL;
            if (format == IFF_JFIF_JPEG) {
                /* The pix is usually returned as long as the header
                 * information is not damaged.
                 * We expect nwarn > 0 (typically 1) for nearly every
                 * corrupted image.  In the situation where only a few
                 * bytes are removed, a corrupted image will occasionally
                 * have nwarn == 0 even though it's visually defective.  */
                pix = pixReadJpeg("/tmp/lept/corrupt/junkout", 0, 1, &nwarn, 0);
                if (pix && nwarn != 1 && Deletion == TRUE) {
                    fprintf(stderr, "nwarn[%d,%d] = %d\n", j, i, nwarn);
                    pixDisplay(pix, 20 * i, 50 * j);  /* show the outliers */
                }
            } else if (format == IFF_PNG)  {
                pix = pixRead("/tmp/lept/corrupt/junkout");
                if (pix) {
                    fprintf(stderr, "pix[%d,%d] is read\n", j, i);
                    pixDisplay(pix, 20 * i, 50 * j);  /* show the outliers */
                }
                pixDestroy(&pix);
                filedata = l_binaryRead("/tmp/lept/corrupt/junkout", &filesize);
                pix = pixReadMemPng(filedata, filesize);
                lept_free(filedata);
            }

                /* Effect of 1% byte mangling from interior of data stream */
            if (pix && j == 10 && i == 10 && Deletion == FALSE)
                pixDisplay(pix, 0, 0);
            pixDestroy(&pix);

                /* Attempt to read the header and the resolution */
            fp = fopenReadStream("/tmp/lept/corrupt/junkout");
            if (format == IFF_JFIF_JPEG) {
                freadHeaderJpeg(fp, &w, NULL, NULL, NULL, NULL);
                fgetJpegResolution(fp, &xres, &yres);
                fprintf(stderr, "w = %d, xres = %d, yres = %d\n",
                        w, xres, yres);
            } else if (format == IFF_PNG)  {
                freadHeaderPng(fp, &w, NULL, NULL, NULL, NULL);
                fgetPngResolution(fp, &xres, &yres);
                fprintf(stderr, "w = %d, xres = %d, yres = %d\n",
                        w, xres, yres);
            }
            fclose(fp);
        }
    }
    return 0;
}

