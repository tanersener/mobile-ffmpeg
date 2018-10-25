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
 * rankbin_reg.c
 *
 *   Tests rank bin functions:
 *      (1) numaDiscretizeRankAndIntensity()
 *      (2) numaGetRankBinValues()
 */

#include "allheaders.h"

int main(int    argc,
         char **argv)
{
l_int32       i, n, w, h;
BOXA         *boxa;
NUMA         *naindex, *naw, *nah, *naw_med, *nah_med;
PIX          *pixs, *pix1;
L_REGPARAMS  *rp;

    if (regTestSetup(argc, argv, &rp))
        return 1;

        /* Generate arrays of word widths and heights */
    pixs = pixRead("feyn.tif");
    pix1 = pixReduceRankBinaryCascade(pixs, 1, 0, 0, 0);
    pixGetWordBoxesInTextlines(pix1, 6, 6, 500, 50, &boxa, &naindex);
    n = boxaGetCount(boxa);
    naw = numaCreate(0);
    nah = numaCreate(0);
    for (i = 0; i < n; i++) {
        boxaGetBoxGeometry(boxa, i, NULL, NULL, &w, &h);
        numaAddNumber(naw, w);
        numaAddNumber(nah, h);
    }
    boxaDestroy(&boxa);
    numaDestroy(&naindex);
    pixDestroy(&pix1);

        /* Make the rank bin arrays of median values, with 10 bins */
    lept_rmfile("/tmp/lept/regout/w_10bin.png");  /* remove existing ones */
    lept_rmfile("/tmp/lept/regout/h_10bin.png");
    lept_rmfile("/tmp/lept/regout/w_30bin.png");
    lept_rmfile("/tmp/lept/regout/h_30bin.png");
    numaGetRankBinValues(naw, 10, NULL, &naw_med);
    numaGetRankBinValues(nah, 10, NULL, &nah_med);
    gplotSimple1(naw_med, GPLOT_PNG, "/tmp/lept/regout/w_10bin",
                 "width vs rank bins (10)");
    gplotSimple1(nah_med, GPLOT_PNG, "/tmp/lept/regout/h_10bin",
                 "height vs rank bins (10)");
    numaDestroy(&naw_med);
    numaDestroy(&nah_med);

        /* Make the rank bin arrays of median values, with 30 bins */
    numaGetRankBinValues(naw, 30, NULL, &naw_med);
    numaGetRankBinValues(nah, 30, NULL, &nah_med);
    gplotSimple1(naw_med, GPLOT_PNG, "/tmp/lept/regout/w_30bin",
                 "width vs rank bins (30)");
    gplotSimple1(nah_med, GPLOT_PNG, "/tmp/lept/regout/h_30bin",
                 "height vs rank bins (30)");
    numaDestroy(&naw_med);
    numaDestroy(&nah_med);

        /* Save as golden files, or check against them */
    regTestCheckFile(rp, "/tmp/lept/regout/w_10bin.png");  /* 0 */
    regTestCheckFile(rp, "/tmp/lept/regout/h_10bin.png");  /* 1 */
    regTestCheckFile(rp, "/tmp/lept/regout/w_30bin.png");  /* 2 */
    regTestCheckFile(rp, "/tmp/lept/regout/h_30bin.png");  /* 3 */

        /* Display results for debugging */
    pix1 = pixRead("/tmp/lept/regout/w_10bin.png");
    pixDisplayWithTitle(pix1, 0, 0, NULL, rp->display);
    pixDestroy(&pix1);
    pix1 = pixRead("/tmp/lept/regout/h_10bin.png");
    pixDisplayWithTitle(pix1, 650, 0, NULL, rp->display);
    pixDestroy(&pix1);
    pix1 = pixRead("/tmp/lept/regout/w_30bin.png");
    pixDisplayWithTitle(pix1, 0, 550, NULL, rp->display);
    pixDestroy(&pix1);
    pix1 = pixRead("/tmp/lept/regout/h_30bin.png");
    pixDisplayWithTitle(pix1, 650, 550, NULL, rp->display);
    pixDestroy(&pix1);

    pixDestroy(&pixs);
    numaDestroy(&naw);
    numaDestroy(&nah);
    return regTestCleanup(rp);
}
