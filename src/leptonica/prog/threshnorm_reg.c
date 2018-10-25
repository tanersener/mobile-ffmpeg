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
 * threshnorm__reg.c
 *
 *      Regression test for adaptive threshold normalization.
 */

#include <string.h>
#include "allheaders.h"

static void AddTestSet(PIXA *pixa, PIX *pixs,
                       l_int32 filtertype, l_int32 edgethresh,
                       l_int32 smoothx, l_int32 smoothy,
                       l_float32 gamma, l_int32 minval,
                       l_int32 maxval, l_int32 targetthresh);

int main(int    argc,
         char **argv)
{
PIX          *pixs, *pixd;
PIXA         *pixa;
L_REGPARAMS  *rp;

    if (regTestSetup(argc, argv, &rp))
        return 1;

    pixs = pixRead("stampede2.jpg");
    pixa = pixaCreate(0);
    pixSaveTiled(pixs, pixa, 1.0, 1, 20, 8);

    AddTestSet(pixa, pixs, L_SOBEL_EDGE, 18, 40, 40, 0.7, -25, 280, 128);
    AddTestSet(pixa, pixs, L_TWO_SIDED_EDGE, 18, 40, 40, 0.7, -25, 280, 128);
    AddTestSet(pixa, pixs, L_SOBEL_EDGE, 10, 40, 40, 0.7, -15, 305, 128);
    AddTestSet(pixa, pixs, L_TWO_SIDED_EDGE, 10, 40, 40, 0.7, -15, 305, 128);
    AddTestSet(pixa, pixs, L_SOBEL_EDGE, 15, 40, 40, 0.6, -45, 285, 158);
    AddTestSet(pixa, pixs, L_TWO_SIDED_EDGE, 15, 40, 40, 0.6, -45, 285, 158);

    pixDestroy(&pixs);
    pixd = pixaDisplay(pixa, 0, 0);
    regTestWritePixAndCheck(rp, pixd, IFF_JFIF_JPEG);  /* 0 */
    pixDisplayWithTitle(pixd, 100, 100, NULL, rp->display);

    pixDestroy(&pixd);
    pixaDestroy(&pixa);
    return regTestCleanup(rp);
}


void
AddTestSet(PIXA      *pixa,
           PIX       *pixs,
           l_int32    filtertype,
           l_int32    edgethresh,
           l_int32    smoothx,
           l_int32    smoothy,
           l_float32  gamma,
           l_int32    minval,
           l_int32    maxval,
           l_int32    targetthresh)
{
PIX  *pixb, *pixd, *pixth;

    pixThresholdSpreadNorm(pixs, filtertype, edgethresh,
                           smoothx, smoothy, gamma, minval,
                           maxval, targetthresh, &pixth, NULL, &pixd);
    pixSaveTiled(pixth, pixa, 1.0, 1, 20, 0);
    pixSaveTiled(pixd, pixa, 1.0, 0, 20, 0);
    pixb = pixThresholdToBinary(pixd, targetthresh - 20);
    pixSaveTiled(pixb, pixa, 1.0, 0, 20, 0);
    pixDestroy(&pixb);
    pixb = pixThresholdToBinary(pixd, targetthresh);
    pixSaveTiled(pixb, pixa, 1.0, 0, 20, 0);
    pixDestroy(&pixb);
    pixb = pixThresholdToBinary(pixd, targetthresh + 20);
    pixSaveTiled(pixb, pixa, 1.0, 0, 20, 0);
    pixDestroy(&pixb);
    pixb = pixThresholdToBinary(pixd, targetthresh + 40);
    pixSaveTiled(pixb, pixa, 1.0, 0, 20, 0);
    pixDestroy(&pixb);
    pixDestroy(&pixth);
    pixDestroy(&pixd);
    return;
}
