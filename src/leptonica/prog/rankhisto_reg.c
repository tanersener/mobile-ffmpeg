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
 * rankhisto_reg.c
 *
 *   Tests grayscale rank functions:
 *      (1) pixGetRankColorArray()
 *      (2) numaDiscretizeRankAndIntensity()
 */

#include <math.h>
#include "allheaders.h"

static PIXA *PixSavePlots1(void);
static PIXA *PixSavePlots2(void);

int main(int    argc,
         char **argv)
{
char          fname[256];
l_int32       i, w, h, nbins, factor;
l_int32       spike;
l_uint32     *array, *marray;
NUMA         *na, *nan, *nai, *narbin;
PIX          *pixs, *pixt, *pixd;
PIXA         *pixa;
L_REGPARAMS  *rp;

    if (regTestSetup(argc, argv, &rp))
        return 1;

        /* Find the rank bin colors */
    pixs = pixRead("map1.jpg");
    pixGetDimensions(pixs, &w, &h, NULL);
    factor = L_MAX(1, (l_int32)sqrt((l_float64)(w * h / 20000.0)));
    nbins = 10;
    pixGetRankColorArray(pixs, nbins, L_SELECT_MIN, factor, &array, 2, 6);
    if (!array)
        return ERROR_INT("\n\n\nFAILURE!\n\n\n", rp->testname, 1);
    for (i = 0; i < nbins; i++)
        fprintf(stderr, "%d: %x\n", i, array[i]);
    pixd = pixDisplayColorArray(array, nbins, 200, 5, 6);
    pixWrite("/tmp/lept/regout/rankhisto.0.png", pixd, IFF_PNG);
    regTestCheckFile(rp, "/tmp/lept/regout/rankhisto.0.png");  /* 0 */
    pixDisplayWithTitle(pixd, 100, 100, NULL, rp->display);
    pixDestroy(&pixd);

        /* Modify the rank bin colors by mapping them such
         * that the lightest color is mapped to white */
    marray = (l_uint32 *)lept_calloc(nbins, sizeof(l_uint32));
    for (i = 0; i < nbins; i++)
        pixelLinearMapToTargetColor(array[i], array[nbins - 1],
                                    0xffffff00, &marray[i]);
    pixd = pixDisplayColorArray(marray, nbins, 200, 5, 6);
    pixWrite("/tmp/lept/regout/rankhisto.1.png", pixd, IFF_PNG);
    regTestCheckFile(rp, "/tmp/lept/regout/rankhisto.1.png");  /* 1 */
    pixDisplayWithTitle(pixd, 100, 600, NULL, rp->display);
    pixDestroy(&pixd);
    lept_free(marray);

        /* Save the histogram plots */
    pixa = PixSavePlots1();
    pixd = pixaDisplay(pixa, 0, 0);
    pixWrite("/tmp/lept/regout/rankhisto.2.png", pixd, IFF_PNG);
    regTestCheckFile(rp, "/tmp/lept/regout/rankhisto.2.png");  /* 2 */
    pixDisplayWithTitle(pixd, 100, 600, NULL, rp->display);
    pixaDestroy(&pixa);
    pixDestroy(&pixd);

        /* Map to the lightest bin; then do TRC adjustment */
    pixt = pixLinearMapToTargetColor(NULL, pixs, array[nbins - 1], 0xffffff00);
    pixd = pixGammaTRC(NULL, pixt, 1.0, 0, 240);
    pixWrite("/tmp/lept/regout/rankhisto.3.png", pixd, IFF_PNG);
    regTestCheckFile(rp, "/tmp/lept/regout/rankhisto.3.png");  /* 3 */
    pixDisplayWithTitle(pixd, 600, 100, NULL, rp->display);
    pixDestroy(&pixt);
    pixDestroy(&pixd);

        /* Now test the edge cases for the histogram and rank LUT,
         * where all the histo data is piled up at one place.
         * We only require that the result be sensible. */
    for (i = 0; i < 3; i++) {
        if (i == 0)
            spike = 0;
        else if (i == 1)
            spike = 50;
        else
            spike = 99;
        na = numaMakeConstant(0, 100);
        numaReplaceNumber(na, spike, 200.0);
        nan = numaNormalizeHistogram(na, 1.0);
        numaDiscretizeRankAndIntensity(nan, 10, &narbin, &nai, NULL, NULL);
        snprintf(fname, sizeof(fname), "/tmp/lept/regout/rtnan%d", i + 1);
        gplotSimple1(nan, GPLOT_PNG, fname, "Normalized Histogram");
        snprintf(fname, sizeof(fname), "/tmp/lept/regout/rtnai%d", i + 1);
        gplotSimple1(nai, GPLOT_PNG, fname, "Intensity vs. rank bin");
        snprintf(fname, sizeof(fname), "/tmp/lept/regout/rtnarbin%d", i + 1);
        gplotSimple1(narbin, GPLOT_PNG, fname, "LUT: rank bin vs. Intensity");
        numaDestroy(&na);
        numaDestroy(&nan);
        numaDestroy(&narbin);
        numaDestroy(&nai);
    }

    pixa = PixSavePlots2();
    pixd = pixaDisplay(pixa, 0, 0);
    pixWrite("/tmp/lept/regout/rankhisto.4.png", pixd, IFF_PNG);
    regTestCheckFile(rp, "/tmp/lept/regout/rankhisto.4.png");  /* 4 */
    pixDisplayWithTitle(pixd, 500, 600, NULL, rp->display);
    pixaDestroy(&pixa);
    pixDestroy(&pixd);

    pixDestroy(&pixs);
    lept_free(array);
    return regTestCleanup(rp);
}


static PIXA *
PixSavePlots1(void)
{
PIX    *pixt;
PIXA   *pixa;

    pixa = pixaCreate(8);
    pixt = pixRead("/tmp/lept/regout/rtnan.png");
    pixSaveTiled(pixt, pixa, 1.0, 1, 20, 8);
    pixDestroy(&pixt);
    pixt = pixRead("/tmp/lept/regout/rtnar.png");
    pixSaveTiled(pixt, pixa, 1.0, 0, 20, 8);
    pixDestroy(&pixt);
    pixt = pixRead("/tmp/lept/regout/rtnai.png");
    pixSaveTiled(pixt, pixa, 1.0, 0, 20, 8);
    pixDestroy(&pixt);
    pixt = pixRead("/tmp/lept/regout/rtnarbin.png");
    pixSaveTiled(pixt, pixa, 1.0, 1, 20, 8);
    pixDestroy(&pixt);
    pixt = pixRead("/tmp/lept/regout/rtnabb.png");
    pixSaveTiled(pixt, pixa, 1.0, 0, 20, 8);
    pixDestroy(&pixt);
    pixt = pixRead("/tmp/lept/regout/rtnared.png");
    pixSaveTiled(pixt, pixa, 1.0, 1, 20, 8);
    pixDestroy(&pixt);
    pixt = pixRead("/tmp/lept/regout/rtnagreen.png");
    pixSaveTiled(pixt, pixa, 1.0, 0, 20, 8);
    pixDestroy(&pixt);
    pixt = pixRead("/tmp/lept/regout/rtnablue.png");
    pixSaveTiled(pixt, pixa, 1.0, 0, 20, 8);
    pixDestroy(&pixt);
    return pixa;
}

static PIXA *
PixSavePlots2(void)
{
PIX    *pixt;
PIXA   *pixa;

    pixa = pixaCreate(9);
    pixt = pixRead("/tmp/lept/regout/rtnan1.png");
    pixSaveTiled(pixt, pixa, 1.0, 1, 20, 8);
    pixDestroy(&pixt);
    pixt = pixRead("/tmp/lept/regout/rtnai1.png");
    pixSaveTiled(pixt, pixa, 1.0, 0, 20, 8);
    pixDestroy(&pixt);
    pixt = pixRead("/tmp/lept/regout/rtnarbin1.png");
    pixSaveTiled(pixt, pixa, 1.0, 0, 20, 8);
    pixDestroy(&pixt);
    pixt = pixRead("/tmp/lept/regout/rtnan2.png");
    pixSaveTiled(pixt, pixa, 1.0, 1, 20, 8);
    pixDestroy(&pixt);
    pixt = pixRead("/tmp/lept/regout/rtnai2.png");
    pixSaveTiled(pixt, pixa, 1.0, 0, 20, 8);
    pixDestroy(&pixt);
    pixt = pixRead("/tmp/lept/regout/rtnarbin2.png");
    pixSaveTiled(pixt, pixa, 1.0, 0, 20, 8);
    pixDestroy(&pixt);
    pixt = pixRead("/tmp/lept/regout/rtnan3.png");
    pixSaveTiled(pixt, pixa, 1.0, 1, 20, 8);
    pixDestroy(&pixt);
    pixt = pixRead("/tmp/lept/regout/rtnai3.png");
    pixSaveTiled(pixt, pixa, 1.0, 0, 20, 8);
    pixDestroy(&pixt);
    pixt = pixRead("/tmp/lept/regout/rtnarbin3.png");
    pixSaveTiled(pixt, pixa, 1.0, 0, 20, 8);
    pixDestroy(&pixt);
    return pixa;
}
