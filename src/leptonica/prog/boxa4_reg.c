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
 * boxa4_reg.c
 *
 *    This carries out some smoothing and display operations on boxa.
 */

#include "allheaders.h"

int main(int    argc,
         char **argv)
{
l_int32       i, same, w, h, width, success, nba;
l_float32     scalefact;
BOXA         *boxa1, *boxa2, *boxa3;
BOXAA        *baa1, *baa2, *baa3;
PIX          *pix1, *pix2, *pix3;
PIXA         *pixa1, *pixa2;
L_REGPARAMS  *rp;

    if (regTestSetup(argc, argv, &rp))
        return 1;

    lept_mkdir("lept/boxa");

        /* Input is a fairly clean boxa */
    boxa1 = boxaRead("boxa1.ba");
    boxa2 = boxaReconcileEvenOddHeight(boxa1, L_ADJUST_TOP, 80,
                                       L_ADJUST_CHOOSE_MIN, 1.05, 1);
    width = 100;
    boxaGetExtent(boxa2, &w, &h, NULL);
    scalefact = (l_float32)width / (l_float32)w;
    boxa3 = boxaTransform(boxa2, 0, 0, scalefact, scalefact);
    pix1 = boxaDisplayTiled(boxa3, NULL, 0, -1, 1500, 2, 1.0, 0, 3, 2);
    regTestWritePixAndCheck(rp, pix1, IFF_PNG);  /* 0 */
    pixDisplayWithTitle(pix1, 600, 0, NULL, rp->display);
    pixDestroy(&pix1);
    boxaDestroy(&boxa1);
    boxaDestroy(&boxa2);
    boxaDestroy(&boxa3);

        /* Input is an unsmoothed and noisy boxa */
    boxa1 = boxaRead("boxa2.ba");
    boxa2 = boxaReconcileEvenOddHeight(boxa1, L_ADJUST_TOP, 80,
                                       L_ADJUST_CHOOSE_MIN, 1.05, 1);
    width = 100;
    boxaGetExtent(boxa2, &w, &h, NULL);
    scalefact = (l_float32)width / (l_float32)w;
    boxa3 = boxaTransform(boxa2, 0, 0, scalefact, scalefact);
    pix1 = boxaDisplayTiled(boxa3, NULL, 0, -1, 1500, 2, 1.0, 0, 3, 2);
    regTestWritePixAndCheck(rp, pix1, IFF_PNG);  /* 1 */
    pixDisplayWithTitle(pix1, 800, 0, NULL, rp->display);
    pixDestroy(&pix1);
    boxaDestroy(&boxa1);
    boxaDestroy(&boxa2);
    boxaDestroy(&boxa3);

        /* Input is an unsmoothed and noisy boxa */
    boxa1 = boxaRead("boxa2.ba");
    boxa2 = boxaSmoothSequenceMedian(boxa1, 10, L_SUB_ON_LOC_DIFF, 80, 20, 1);
    boxa3 = boxaSmoothSequenceMedian(boxa1, 10, L_SUB_ON_SIZE_DIFF, 80, 20, 1);
    boxaPlotSides(boxa1, "initial", NULL, NULL, NULL, NULL, &pix1);
    boxaPlotSides(boxa2, "side_smoothing", NULL, NULL, NULL, NULL, &pix2);
    boxaPlotSides(boxa3, "size_smoothing", NULL, NULL, NULL, NULL, &pix3);
    regTestWritePixAndCheck(rp, pix1, IFF_PNG);  /* 2 */
    regTestWritePixAndCheck(rp, pix2, IFF_PNG);  /* 3 */
    regTestWritePixAndCheck(rp, pix3, IFF_PNG);  /* 4 */
    pixDisplayWithTitle(pix1, 1300, 0, NULL, rp->display);
    pixDisplayWithTitle(pix2, 1300, 500, NULL, rp->display);
    pixDisplayWithTitle(pix3, 1300, 1000, NULL, rp->display);
    pixDestroy(&pix1);
    pixDestroy(&pix2);
    pixDestroy(&pix3);
    boxaDestroy(&boxa1);
    boxaDestroy(&boxa2);
    boxaDestroy(&boxa3);

        /* Input is a boxa smoothed with a median window filter */
    boxa1 = boxaRead("boxa3.ba");
    boxa2 = boxaReconcileEvenOddHeight(boxa1, L_ADJUST_TOP, 80,
                                       L_ADJUST_CHOOSE_MIN, 1.05, 1);
    width = 100;
    boxaGetExtent(boxa2, &w, &h, NULL);
    scalefact = (l_float32)width / (l_float32)w;
    boxa3 = boxaTransform(boxa2, 0, 0, scalefact, scalefact);
    pix1 = boxaDisplayTiled(boxa3, NULL, 0, -1, 1500, 2, 1.0, 0, 3, 2);
    regTestWritePixAndCheck(rp, pix1, IFF_PNG);  /* 5 */
    pixDisplayWithTitle(pix1, 1000, 0, NULL, rp->display);
    pixDestroy(&pix1);
    boxaDestroy(&boxa1);
    boxaDestroy(&boxa2);
    boxaDestroy(&boxa3);

        /* ----------- Test pixaDisplayBoxaa() ------------ */
    pixa1 = pixaReadBoth("showboxes.pac");
    baa1 = boxaaRead("showboxes1.baa");
    baa2 = boxaaTranspose(baa1);
    baa3 = boxaaTranspose(baa2);
    nba = boxaaGetCount(baa1);
    success = TRUE;
    for (i = 0; i < nba; i++) {
        boxa1 = boxaaGetBoxa(baa1, i, L_CLONE);
        boxa2 = boxaaGetBoxa(baa3, i, L_CLONE);
        boxaEqual(boxa1, boxa2, 0, NULL, &same);
        boxaDestroy(&boxa1);
        boxaDestroy(&boxa2);
        if (!same) success = FALSE;
    }

        /* Check that the transpose is reversible */
    regTestCompareValues(rp, 1, success, 0.0);  /* 6 */
    pixa2 = pixaDisplayBoxaa(pixa1, baa2, L_DRAW_RGB, 2);
    pix1 = pixaDisplayTiledInRows(pixa2, 32, 1400, 1.0, 0, 10, 0);
    regTestWritePixAndCheck(rp, pix1, IFF_PNG);  /* 7 */
    pixDisplayWithTitle(pix1, 0, 600, NULL, rp->display);
    fprintf(stderr, "Writing to: /tmp/lept/boxa/show.pdf\n");
    l_pdfSetDateAndVersion(FALSE);
    pixaConvertToPdf(pixa2, 75, 1.0, 0, 0, NULL, "/tmp/lept/boxa/show.pdf");
    regTestCheckFile(rp, "/tmp/lept/boxa/show.pdf");  /* 8 */
    pixDestroy(&pix1);
    pixaDestroy(&pixa1);
    pixaDestroy(&pixa2);
    boxaaDestroy(&baa1);
    boxaaDestroy(&baa2);
    boxaaDestroy(&baa3);

    return regTestCleanup(rp);
}
