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
 * binarize_set.c
 *
 *   Does 5 different types of binarization for the contest.
 *
 *   Method 1.  Using local background normalization, followed by
 *              a global threshold.
 *   Method 2.  Using local background normalization, followed by
 *              Otsu on the result to get a global threshold that
 *              can be applied to the normalized image.
 *   Method 3.  Using local background normalization with two different
 *              thresholds.  For the part of the image near the text,
 *              a high threshold can be chosen, to render the text
 *              fully in black.  For the rest of the image,
 *              much of which is background, use a threshold based on
 *              the Otsu global value of the original image.
 *   Method 4.  Background normalization followed by Sauvola binarization.
 *   Method 5.  Contrast normalization followed by background normalization
 *              and thresholding.
 *
 *   The first 3 were submitted to a binarization contest associated
 *   with ICDAR in 2009.  The 4th and 5th work better for difficult
 *   images, such as w91frag.jpg.
 */

#include "allheaders.h"

#define  ALL    1


int main(int    argc,
         char **argv)
{
char        *infile;
l_int32      w, d, threshval, ival, newval;
l_uint32     val;
PIX         *pixs, *pixg, *pixg2;
PIX         *pix1, *pix2;
PIXA        *pixa;
static char  mainName[] = "binarize_set";

    if (argc != 2)
        return ERROR_INT(" Syntax: binarize_set infile", mainName, 1);
    infile = argv[1];

    setLeptDebugOK(1);
    lept_mkdir("lept/binar");

    pixa = pixaCreate(5);
    pixs = pixRead(infile);
    pixGetDimensions(pixs, &w, NULL, &d);
    pixSaveTiled(pixs, pixa, 1.0, 1, 50, 32);
    pixDisplay(pixs, 100, 0);

#if ALL
    /* 1. Standard background normalization with a global threshold.  */
    pixg = pixConvertTo8(pixs, 0);
    pix1 = pixBackgroundNorm(pixg, NULL, NULL, 10, 15, 100, 50, 255, 2, 2);
    pix2 = pixThresholdToBinary(pix1, 160);
    pixWrite("/tmp/lept/binar/binar1.png", pix2, IFF_PNG);
    pixDisplay(pix2, 100, 0);
    pixSaveTiled(pix2, pixa, 1.0, 1, 50, 32);
    pixDestroy(&pixg);
    pixDestroy(&pix1);
    pixDestroy(&pix2);
#endif

#if ALL
    /* 2. Background normalization followed by Otsu thresholding.  Otsu
     * binarization attempts to split the image into two roughly equal
     * sets of pixels, and it does a very poor job when there are large
     * amounts of dark background.  By doing a background normalization
     * first (to get the background near 255), we remove this problem.
     * Then we use a modified Otsu to estimate the best global
     * threshold on the normalized image.  */
    pixg = pixConvertTo8(pixs, 0);
    pix1 = pixOtsuThreshOnBackgroundNorm(pixg, NULL, 10, 15, 100,
                                    50, 255, 2, 2, 0.10, &threshval);
    fprintf(stderr, "thresh val = %d\n", threshval);
    pixSaveTiled(pix1, pixa, 1.0, 1, 50, 32);
    pixWrite("/tmp/lept/binar/binar2.png", pix1, IFF_PNG);
    pixDisplay(pix1, 100, 200);
    pixDestroy(&pixg);
    pixDestroy(&pix1);
#endif

#if ALL
    /* 3. Background normalization with Otsu threshold estimation and
     * masking for threshold selection.  */
    pixg = pixConvertTo8(pixs, 0);
    pix1 = pixMaskedThreshOnBackgroundNorm(pixg, NULL, 10, 15, 100,
                                           50, 2, 2, 0.10, &threshval);
    fprintf(stderr, "thresh val = %d\n", threshval);
    pixSaveTiled(pix1, pixa, 1.0, 1, 50, 32);
    pixWrite("/tmp/lept/binar/binar3.png", pix1, IFF_PNG);
    pixDisplay(pix1, 100, 400);
    pixDestroy(&pixg);
    pixDestroy(&pix1);
#endif

#if ALL
    /* 4. Background normalization followed by Sauvola binarization */
    if (d == 32)
        pixg = pixConvertRGBToGray(pixs, 0.2, 0.7, 0.1);
    else
        pixg = pixConvertTo8(pixs, 0);
    pixg2 = pixContrastNorm(NULL, pixg, 20, 20, 130, 2, 2);
    pixSauvolaBinarizeTiled(pixg2, 25, 0.40, 1, 1, NULL, &pix1);
    pixSaveTiled(pix1, pixa, 1.0, 1, 50, 32);
    pixWrite("/tmp/lept/binar/binar4.png", pix1, IFF_PNG);
    pixDisplay(pix1, 100, 600);
    pixDestroy(&pixg);
    pixDestroy(&pixg2);
    pixDestroy(&pix1);
#endif

#if ALL
    /* 5. Contrast normalization followed by background normalization, and
     * thresholding. */
    if (d == 32)
        pixg = pixConvertRGBToGray(pixs, 0.2, 0.7, 0.1);
    else
        pixg = pixConvertTo8(pixs, 0);

    pixOtsuAdaptiveThreshold(pixg, 5000, 5000, 0, 0, 0.1, &pix1, NULL);
    pixGetPixel(pix1, 0, 0, &val);
    ival = (l_int32)val;
    newval = ival + (l_int32)(0.6 * (110 - ival));
    fprintf(stderr, "th1 = %d, th2 = %d\n", ival, newval);
    pixDestroy(&pix1);

    pixContrastNorm(pixg, pixg, 50, 50, 130, 2, 2);
    pixg2 = pixBackgroundNorm(pixg, NULL, NULL, 20, 20, 70, 40, 200, 2, 2);

    ival = L_MIN(ival, 110);
    pix1 = pixThresholdToBinary(pixg2, ival);
    pixSaveTiled(pix1, pixa, 1.0, 1, 50, 32);
    pixWrite("/tmp/lept/binar/binar5.png", pix1, IFF_PNG);
    pixDisplay(pix1, 100, 800);
    pixDestroy(&pixg);
    pixDestroy(&pixg2);
    pixDestroy(&pix1);
#endif

    pix1 = pixaDisplayTiledInRows(pixa, 32, w + 100, 1.0, 0, 30, 2);
    pixWrite("/tmp/lept/binar/binar6.png", pix1, IFF_PNG);
    pixDisplay(pix1, 1000, 0);
    pixDestroy(&pix1);
    pixaDestroy(&pixa);

    pixDestroy(&pixs);
    return 0;
}


