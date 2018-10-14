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
 * boxa1_reg.c
 *
 *    This carries out various operations on boxa, including
 *    region comparison, transforms and display.
 */

#include "allheaders.h"

static PIX *DisplayBoxa(BOXA  *boxa);

int main(int    argc,
         char **argv)
{
l_uint8     *data1, *data2;
l_int32      i, same, w, h, width, success, nba;
size_t       size1, size2;
l_float32    diffarea, diffxor, scalefact;
BOX         *box;
BOXA        *boxa1, *boxa2, *boxa3;
BOXAA       *baa1, *baa2, *baa3;
PIX         *pix1, *pix2, *pix3, *pixdb;
PIXA        *pixa1, *pixa2;
L_REGPARAMS  *rp;

    if (regTestSetup(argc, argv, &rp))
        return 1;

    lept_mkdir("lept/boxa");

        /* Make a boxa and display its contents */
    boxa1 = boxaCreate(6);
    box = boxCreate(60, 60, 40, 20);
    boxaAddBox(boxa1, box, L_INSERT);
    box = boxCreate(120, 50, 20, 50);
    boxaAddBox(boxa1, box, L_INSERT);
    box = boxCreate(50, 140, 46, 60);
    boxaAddBox(boxa1, box, L_INSERT);
    box = boxCreate(166, 130, 64, 28);
    boxaAddBox(boxa1, box, L_INSERT);
    box = boxCreate(64, 224, 44, 34);
    boxaAddBox(boxa1, box, L_INSERT);
    box = boxCreate(117, 206, 26, 74);
    boxaAddBox(boxa1, box, L_INSERT);
    pix1 = DisplayBoxa(boxa1);
    regTestWritePixAndCheck(rp, pix1, IFF_PNG);  /* 0 */
    pixDisplayWithTitle(pix1, 0, 0, NULL, rp->display);
    pixDestroy(&pix1);

    boxaCompareRegions(boxa1, boxa1, 100, &same, &diffarea, &diffxor, NULL);
    regTestCompareValues(rp, 1, same, 0.0);  /* 1 */
    regTestCompareValues(rp, 0.0, diffarea, 0.0);  /* 2 */
    regTestCompareValues(rp, 0.0, diffxor, 0.0);  /* 3 */

    boxa2 = boxaTransform(boxa1, -13, -13, 1.0, 1.0);
    boxaCompareRegions(boxa1, boxa2, 10, &same, &diffarea, &diffxor, NULL);
    regTestCompareValues(rp, 1, same, 0.0);  /* 4 */
    regTestCompareValues(rp, 0.0, diffarea, 0.0);  /* 5 */
    regTestCompareValues(rp, 0.0, diffxor, 0.0);  /* 6 */
    boxaDestroy(&boxa2);

    boxa2 = boxaReconcileEvenOddHeight(boxa1, L_ADJUST_TOP_AND_BOT, 6,
                                       L_ADJUST_CHOOSE_MIN, 1.0, 0);
    pix1 = DisplayBoxa(boxa2);
    regTestWritePixAndCheck(rp, pix1, IFF_PNG);  /* 7 */
    pixDisplayWithTitle(pix1, 200, 0, NULL, rp->display);
    pixDestroy(&pix1);

    boxaCompareRegions(boxa1, boxa2, 10, &same, &diffarea, &diffxor, &pixdb);
    regTestCompareValues(rp, 1, same, 0.0);  /* 8 */
    regTestCompareValues(rp, 0.053, diffarea, 0.002);  /* 9 */
    regTestCompareValues(rp, 0.240, diffxor, 0.002);  /* 10 */
    regTestWritePixAndCheck(rp, pixdb, IFF_PNG);  /* 11 */
    pixDisplayWithTitle(pixdb, 400, 0, NULL, rp->display);
    pixDestroy(&pixdb);
    boxaDestroy(&boxa1);
    boxaDestroy(&boxa2);

        /* Input is a fairly clean boxa */
    boxa1 = boxaRead("boxa1.ba");
    boxa2 = boxaReconcileEvenOddHeight(boxa1, L_ADJUST_TOP, 80,
                                       L_ADJUST_CHOOSE_MIN, 1.05, 1);
    width = 100;
    boxaGetExtent(boxa2, &w, &h, NULL);
    scalefact = (l_float32)width / (l_float32)w;
    boxa3 = boxaTransform(boxa2, 0, 0, scalefact, scalefact);
    pix1 = boxaDisplayTiled(boxa3, NULL, 1500, 2, 1.0, 0, 3, 2);
    regTestWritePixAndCheck(rp, pix1, IFF_PNG);  /* 12 */
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
    pix1 = boxaDisplayTiled(boxa3, NULL, 1500, 2, 1.0, 0, 3, 2);
    regTestWritePixAndCheck(rp, pix1, IFF_PNG);  /* 13 */
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
    regTestWritePixAndCheck(rp, pix1, IFF_PNG);  /* 14 */
    regTestWritePixAndCheck(rp, pix2, IFF_PNG);  /* 15 */
    regTestWritePixAndCheck(rp, pix3, IFF_PNG);  /* 16 */
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
    pix1 = boxaDisplayTiled(boxa3, NULL, 1500, 2, 1.0, 0, 3, 2);
    regTestWritePixAndCheck(rp, pix1, IFF_PNG);  /* 17 */
    pixDisplayWithTitle(pix1, 1000, 0, NULL, rp->display);
    pixDestroy(&pix1);
    boxaDestroy(&boxa1);
    boxaDestroy(&boxa2);
    boxaDestroy(&boxa3);

        /* Test serialized boxa I/O to and from memory */
    data1 = l_binaryRead("boxa2.ba", &size1);
    boxa1 = boxaReadMem(data1, size1);
    boxaWriteMem(&data2, &size2, boxa1);
    boxa2 = boxaReadMem(data2, size2);
    boxaWrite("/tmp/lept/boxa/boxa1.ba", boxa1);
    boxaWrite("/tmp/lept/boxa/boxa2.ba", boxa2);
    filesAreIdentical("/tmp/lept/boxa/boxa1.ba", "/tmp/lept/boxa/boxa2.ba",
                      &same);
    regTestCompareValues(rp, 1, same, 0.0);  /* 18 */
    boxaDestroy(&boxa1);
    boxaDestroy(&boxa2);
    lept_free(data1);
    lept_free(data2);

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
    regTestCompareValues(rp, 1, success, 0.0);  /* 19 */
    pixa2 = pixaDisplayBoxaa(pixa1, baa2, L_DRAW_RGB, 2);
    pix1 = pixaDisplayTiledInRows(pixa2, 32, 1400, 1.0, 0, 10, 0);
    regTestWritePixAndCheck(rp, pix1, IFF_PNG);  /* 20 */
    pixDisplayWithTitle(pix1, 0, 600, NULL, rp->display);
    fprintf(stderr, "Writing to: /tmp/lept/boxa/show.pdf\n");
    l_pdfSetDateAndVersion(FALSE);
    pixaConvertToPdf(pixa2, 75, 1.0, 0, 0, NULL, "/tmp/lept/boxa/show.pdf");
    regTestCheckFile(rp, "/tmp/lept/boxa/show.pdf");  /* 21 */
    pixDestroy(&pix1);
    pixaDestroy(&pixa1);
    pixaDestroy(&pixa2);
    boxaaDestroy(&baa1);
    boxaaDestroy(&baa2);
    boxaaDestroy(&baa3);

    return regTestCleanup(rp);
}


static PIX *
DisplayBoxa(BOXA  *boxa)
{
l_int32  w, h;
BOX     *box;
PIX     *pix1, *pix2, *pix3;
PIXA    *pixa;

    pixa = pixaCreate(2);
    boxaGetExtent(boxa, &w, &h, &box);
    pix1 = pixCreate(w, h, 1);
    pixMaskBoxa(pix1, pix1, boxa, L_SET_PIXELS);
    pixaAddPix(pixa, pix1, L_INSERT);
    pix2 = pixCreate(w, h, 32);
    pixSetAll(pix2);
    pixRenderBoxaArb(pix2, boxa, 2, 0, 255, 0);
    pixRenderBoxArb(pix2, box, 3, 255, 0, 0);
    pixaAddPix(pixa, pix2, L_INSERT);
    pix3 = pixaDisplayTiledInRows(pixa, 32, 1000, 1.0, 0, 30, 2);
    boxDestroy(&box);
    pixaDestroy(&pixa);
    return pix3;
}

