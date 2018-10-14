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
 * printimage.c
 *
 *   This prints an image.  It rotates and isotropically scales the image,
 *   as necessary, to get a maximum filling when printing onto an
 *   8.5 x 11 inch page.
 *
 *        Syntax:  printimage <filein> <printer> [other lpr args]
 *
 *   The most simple input would be something like this:
 *        printimage myfile.jpg myprinter
 *
 *   You can add lpr flags after this.
 *   To print more than one copy, add:
 *       -#N        (prints N copies)
 *   To print in color, add a printer-dependent flag; e.g.,
 *       -o ColorModel=Color
 *       -o ColorModel=CMYK
 *
 *   For example, to make 3 color copies, you might use:
 *       printimage myfile.jpg myprinter -#3 -o ColorModel=Color
 *
 *   The intermediate PostScript file generated is level 1 (uncompressed).
 *   This can be large, but it will work on all PostScript printers.
 *
 *   ***************************************************************
 *   N.B.  This requires lpr, which is invoked via 'system'.  It could
 *         pose a security vulnerability if used as a service in a
 *         production environment.  Consequently, this program should
 *         only be used for debug and testing.
 *   ***************************************************************
 */

#include "allheaders.h"

static const l_float32  FILL_FACTOR = 0.95;   /* fill factor on 8.5 x 11 page */


int main(int    argc,
         char **argv)
{
char        *filein, *printer, *extra, *fname;
char         buffer[512];
l_int32      i, w, h, ignore;
l_float32    scale;
FILE        *fp;
PIX         *pixs, *pix1;
SARRAY      *sa;
static char  mainName[] = "printimage";

    if (argc < 3)
        return ERROR_INT(
            " Syntax:  printimage <filein> <printer> [other lpr args]",
            mainName, 1);
    filein = argv[1];
    printer = argv[2];

    fprintf(stderr,
         "\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n"
         "   Warning: this program should only be used for testing,\n"
         "     and not in a production environment, because of a\n"
         "      potential vulnerability with the 'system' call.\n"
         "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n");

    setLeptDebugOK(1);
    lept_rm(NULL, "print_image.ps");

    if ((pixs = pixRead(filein)) == NULL)
        return ERROR_INT("pixs not made", mainName, 1);

    pixGetDimensions(pixs, &w, &h, NULL);
    if (w > h) {
        pix1 = pixRotate90(pixs, 1);
        pixGetDimensions(pix1, &w, &h, NULL);
    }
    else {
        pix1 = pixClone(pixs);
    }
    scale = L_MIN(FILL_FACTOR * 2550 / w, FILL_FACTOR * 3300 / h);
    fname = genPathname("/tmp", "print_image.ps");
    fp = lept_fopen(fname, "wb+");
    pixWriteStreamPS(fp, pix1, NULL, 300, scale);
    lept_fclose(fp);

        /* Print it out */
    extra = NULL;
    if (argc > 3) {  /* concatenate the extra args */
        sa = sarrayCreate(0);
        for (i = 3; i < argc; i++)
            sarrayAddString(sa, argv[i], L_COPY);
        extra = sarrayToString(sa, 2);
        sarrayDestroy(&sa);
    }
    if (!extra) {
        snprintf(buffer, sizeof(buffer), "lpr %s -P%s &", fname, printer);
        ignore = system(buffer);
    } else {
        snprintf(buffer, sizeof(buffer), "lpr %s -P%s %s &",
                 fname, printer, extra);
        ignore = system(buffer);
    }

    lept_free(fname);
    lept_free(extra);
    pixDestroy(&pixs);
    pixDestroy(&pix1);
    return 0;
}

