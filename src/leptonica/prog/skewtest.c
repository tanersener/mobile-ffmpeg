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
 * skewtest.c
 *
 *     Tests various skew finding methods, optionally deskewing
 *     the input (binary) image.  The best version does a linear
 *     sweep followed by a binary (angle-splitting) search.
 *     The basic method is to find the vertical shear angle such
 *     that the differential variance of ON pixels between each
 *     line and it's neighbor, when summed over all lines, is
 *     maximized.
 */

#include "allheaders.h"

    /* deskew */
#define   DESKEW_REDUCTION      2      /* 1, 2 or 4 */

    /* sweep only */
#define   SWEEP_RANGE           10.    /* degrees */
#define   SWEEP_DELTA           0.2    /* degrees */
#define   SWEEP_REDUCTION       2      /* 1, 2, 4 or 8 */

    /* sweep and search */
#define   SWEEP_RANGE2          10.    /* degrees */
#define   SWEEP_DELTA2          1.     /* degrees */
#define   SWEEP_REDUCTION2      2      /* 1, 2, 4 or 8 */
#define   SEARCH_REDUCTION      2      /* 1, 2, 4 or 8 */
#define   SEARCH_MIN_DELTA      0.01   /* degrees */


int main(int    argc,
         char **argv)
{
char        *filein, *fileout;
l_int32      ret;
l_float32    deg2rad;
l_float32    angle, conf, score;
PIX         *pix, *pixs, *pixd;
static char  mainName[] = "skewtest";

    if (argc != 3)
        return ERROR_INT(" Syntax:  skewtest filein fileout", mainName, 1);
    filein = argv[1];
    fileout = argv[2];

    setLeptDebugOK(1);
    pixd = NULL;
    deg2rad = 3.1415926535 / 180.;

    if ((pixs = pixRead(filein)) == NULL)
        return ERROR_INT("pixs not made", mainName, 1);

        /* Find the skew angle various ways */
    pix = pixConvertTo1(pixs, 130);
    pixWrite("/tmp/binarized.tif", pix, IFF_TIFF_G4);
    pixFindSkew(pix, &angle, &conf);
    fprintf(stderr, "pixFindSkew():\n"
                    "  conf = %5.3f, angle = %7.3f degrees\n", conf, angle);

    pixFindSkewSweepAndSearchScorePivot(pix, &angle, &conf, &score,
                                        SWEEP_REDUCTION2, SEARCH_REDUCTION,
                                        0.0, SWEEP_RANGE2, SWEEP_DELTA2,
                                        SEARCH_MIN_DELTA,
                                        L_SHEAR_ABOUT_CORNER);
    fprintf(stderr, "pixFind...Pivot(about corner):\n"
                    "  conf = %5.3f, angle = %7.3f degrees, score = %f\n",
            conf, angle, score);

    pixFindSkewSweepAndSearchScorePivot(pix, &angle, &conf, &score,
                                        SWEEP_REDUCTION2, SEARCH_REDUCTION,
                                        0.0, SWEEP_RANGE2, SWEEP_DELTA2,
                                        SEARCH_MIN_DELTA,
                                        L_SHEAR_ABOUT_CENTER);
    fprintf(stderr, "pixFind...Pivot(about center):\n"
                    "  conf = %5.3f, angle = %7.3f degrees, score = %f\n",
            conf, angle, score);

        /* Use top-level */
    pixd = pixDeskew(pixs, 0);
    pixWriteImpliedFormat(fileout, pixd, 0, 0);


#if 0
        /* Do it piecemeal; fails if outside the range */
    if (pixGetDepth(pixs) == 1) {
        pixd = pixDeskew(pix, DESKEW_REDUCTION);
        pixWrite(fileout, pixd, IFF_PNG);
    }
    else {
        ret = pixFindSkewSweepAndSearch(pix, &angle, &conf, SWEEP_REDUCTION2,
                                        SEARCH_REDUCTION, SWEEP_RANGE2,
                                        SWEEP_DELTA2, SEARCH_MIN_DELTA);
        if (ret)
            L_WARNING("skew angle not valid\n", mainName);
        else {
            fprintf(stderr, "conf = %5.3f, angle = %7.3f degrees\n",
                    conf, angle);
            if (conf > 2.5)
                pixd = pixRotate(pixs, angle * deg2rad, L_ROTATE_AREA_MAP,
                                 L_BRING_IN_WHITE, 0, 0);
            else
                pixd = pixClone(pixs);
            pixWrite(fileout, pixd, IFF_PNG);
            pixDestroy(&pixd);
        }
    }
#endif

    pixDestroy(&pixs);
    pixDestroy(&pix);
    pixDestroy(&pixd);
    return 0;
}

