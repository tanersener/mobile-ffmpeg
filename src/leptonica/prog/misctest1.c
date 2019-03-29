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
 *   misctest1.c
 *        * Combine two grayscale images using a mask
 *        * Combine two binary images using a mask
 *        * Do a restricted seedfill
 *        * Colorize a grayscale image
 *        * Convert color to gray
 *        * Extract text lines
 *        * Plot box side locations and dimension of a boxa
 *        * Extract and display rank sized components
 *        * Extract parts of an image using a boxa
 */

#include "allheaders.h"

#define   SHOW    0

int main(int    argc,
         char **argv)
{
l_int32   w, h, bx, by, bw, bh, i, j;
BOX      *box1, *box2;
BOXA     *boxa1, *boxa2, *boxae, *boxao;
PIX      *pixs, *pix1, *pix2, *pix3, *pixg, *pixb, *pixd, *pixc;
PIX      *pixm, *pixm2, *pixd2, *pixs2;
PIXA     *pixa1, *pixa2;
PIXCMAP  *cmap, *cmapg;

    setLeptDebugOK(1);
    lept_mkdir("lept/misc");
    pixa1 = pixaCreate(0);

        /* Combine two grayscale images using a mask */
    pixd = pixRead("feyn.tif");
    pixs = pixRead("rabi.png");
    pixm = pixRead("pageseg2-seed.png");
    pixd2 = pixScaleToGray2(pixd);
    pixs2 = pixScaleToGray2(pixs);
    pixSaveTiled(pixd2, pixa1, 0.5, 1, 40, 32);
    pixSaveTiled(pixs2, pixa1, 0.5, 0, 40, 0);
    pixSaveTiled(pixm, pixa1, 0.5, 0, 40, 0);
    pixCombineMaskedGeneral(pixd2, pixs2, pixm, 100, 100);
    pixSaveTiled(pixd2, pixa1, 0.5, 1, 40, 0);
    pixDisplayWithTitle(pixd2, 100, 100, NULL, SHOW);
    pixDestroy(&pixd2);
    pixDestroy(&pixs2);

        /* Combine two binary images using a mask */
    pixm2 = pixExpandBinaryReplicate(pixm, 2, 2);
    pix1 = pixCopy(NULL, pixd);
    pixCombineMaskedGeneral(pixd, pixs, pixm2, 200, 200);
    pixSaveTiled(pixd, pixa1, 0.25, 0, 40, 0);
    pixDisplayWithTitle(pixd, 700, 100, NULL, SHOW);
    pixCombineMasked(pix1, pixs, pixm2);
    pixSaveTiled(pix1, pixa1, 0.25, 0, 40, 0);
    pixDestroy(&pixd);
    pixDestroy(&pix1);
    pixDestroy(&pixs);
    pixDestroy(&pixm);
    pixDestroy(&pixm2);

        /* Do a restricted seedfill */
    pixs = pixRead("pageseg2-seed.png");
    pixm = pixRead("pageseg2-mask.png");
    pixd = pixSeedfillBinaryRestricted(NULL, pixs, pixm, 8, 50, 175);
    pixSaveTiled(pixs, pixa1, 0.5, 1, 40, 0);
    pixSaveTiled(pixm, pixa1, 0.5, 0, 40, 0);
    pixSaveTiled(pixd, pixa1, 0.5, 0, 40, 0);
    pixDestroy(&pixs);
    pixDestroy(&pixm);
    pixDestroy(&pixd);

        /* Colorize a grayscale image */
    pixs = pixRead("lucasta.150.jpg");
    pixGetDimensions(pixs, &w, &h, NULL);
    pixb = pixThresholdToBinary(pixs, 128);
    boxa1 = pixConnComp(pixb, &pixa2, 8);
    pixSaveTiled(pixs, pixa1, 1.0, 1, 40, 0);
    cmap = pixcmapGrayToColor(0x6f90c0);
    pixSetColormap(pixs, cmap);
    pixSaveTiled(pixs, pixa1, 1.0, 0, 40, 0);
    pixc = pixaDisplayRandomCmap(pixa2, w, h);
    pixcmapResetColor(pixGetColormap(pixc), 0, 255, 255, 255);
    pixSaveTiled(pixc, pixa1, 1.0, 0, 40, 0);
    pixDestroy(&pixs);
    pixDestroy(&pixb);
    pixDestroy(&pixc);
    boxaDestroy(&boxa1);
    pixaDestroy(&pixa2);

        /* Convert color to gray */
    pixs = pixRead("weasel4.16c.png");
    pixSaveTiled(pixs, pixa1, 1.0, 1, 20, 0);
    pixc = pixConvertTo32(pixs);
    pix1 = pixConvertRGBToGray(pixc, 3., 7., 5.);  /* bad weights */
    pixSaveTiled(pix1, pixa1, 1.0, 0, 20, 0);
    pix2 = pixConvertRGBToGrayFast(pixc);
    pixSaveTiled(pix2, pixa1, 1.0, 0, 20, 0);
    pixg = pixCopy(NULL, pixs);
    cmap = pixGetColormap(pixs);
    cmapg = pixcmapColorToGray(cmap, 4., 6., 3.);
    pixSetColormap(pixg, cmapg);
    pixSaveTiled(pixg, pixa1, 1.0, 0, 20, 0);
    pixDestroy(&pixs);
    pixDestroy(&pixc);
    pixDestroy(&pix1);
    pixDestroy(&pix2);
    pixDestroy(&pixg);

    pixd = pixaDisplay(pixa1, 0, 0);
    pixDisplayWithTitle(pixd, 100, 100, NULL, 1);
    pixWrite("/tmp/misc1.png", pixd, IFF_PNG);
    pixDestroy(&pixd);
    pixaDestroy(&pixa1);

        /* Extract text lines */
    pix1 = pixRead("feyn.tif");
    pixa1 = pixExtractTextlines(pix1, 150, 150, 0, 0, 5, 5, NULL);
    boxa1 = pixaGetBoxa(pixa1, L_CLONE);
    boxaWrite("/tmp/lept/misc/lines1.ba", boxa1);
    pix2 = pixaDisplayRandomCmap(pixa1, 0, 0);
    pixcmapResetColor(pixGetColormap(pix2), 0, 255, 255, 255);
    pixDisplay(pix2, 400, 0);
    pixWrite("/tmp/lept/misc/lines1.png", pix2, IFF_PNG);
    boxaDestroy(&boxa1);
    pixDestroy(&pix1);
    pixDestroy(&pix2);
    pixaDestroy(&pixa1);

    pix1 = pixRead("arabic.png");
    pixa1 = pixExtractTextlines(pix1, 150, 150, 0, 0, 5, 5, NULL);
    pix2 = pixaDisplayRandomCmap(pixa1, 0, 0);
    pixcmapResetColor(pixGetColormap(pix2), 0, 255, 255, 255);
    pixDisplay(pix2, 400, 400);
    pixWrite("/tmp/lept/misc/lines2.png", pix2, IFF_PNG);
    pixDestroy(&pix1);
    pixDestroy(&pix2);
    pixaDestroy(&pixa1);

    pix1 = pixRead("arabic2.png");
    pixa1 = pixExtractTextlines(pix1, 150, 150, 0, 0, 5, 5, NULL);
    pix2 = pixaDisplayRandomCmap(pixa1, 0, 0);
    pixcmapResetColor(pixGetColormap(pix2), 0, 255, 255, 255);
    pixDisplay(pix2, 400, 800);
    pixWrite("/tmp/lept/misc/lines3.png", pix2, IFF_PNG);
    pixDestroy(&pix1);
    pixDestroy(&pix2);
    pixaDestroy(&pixa1);

        /* Plot box side locations and dimensions of a boxa */
    pixa1 = pixaCreate(0);
    boxa1 = boxaRead("boxa2.ba");
    boxaSplitEvenOdd(boxa1, 0, &boxae, &boxao);
    boxaPlotSides(boxae, "1-sides-even", NULL, NULL, NULL, NULL, &pix1);
    pixaAddPix(pixa1, pix1, L_INSERT);
    boxaPlotSides(boxao, "1-sides-odd", NULL, NULL, NULL, NULL, &pix1);
    pixaAddPix(pixa1, pix1, L_INSERT);
    boxaPlotSizes(boxae, "1-sizes-even", NULL, NULL, &pix1);
    pixaAddPix(pixa1, pix1, L_INSERT);
    boxaPlotSizes(boxao, "1-sizes-odd", NULL, NULL, &pix1);
    pixaAddPix(pixa1, pix1, L_INSERT);
    boxaDestroy(&boxae);
    boxaDestroy(&boxao);
    boxaDestroy(&boxa1);
    boxa1 = boxaRead("boxa3.ba");
    boxaSplitEvenOdd(boxa1, 0, &boxae, &boxao);
    boxaPlotSides(boxae, "2-sides-even", NULL, NULL, NULL, NULL, &pix1);
    pixaAddPix(pixa1, pix1, L_INSERT);
    boxaPlotSides(boxao, "2-sides-odd", NULL, NULL, NULL, NULL, &pix1);
    pixaAddPix(pixa1, pix1, L_INSERT);
    boxaPlotSizes(boxae, "2-sizes-even", NULL, NULL, &pix1);
    pixaAddPix(pixa1, pix1, L_INSERT);
    boxaPlotSizes(boxao, "2-sizes-odd", NULL, NULL, &pix1);
    pixaAddPix(pixa1, pix1, L_INSERT);
    boxaDestroy(&boxae);
    boxaDestroy(&boxao);
    boxaDestroy(&boxa1);
    pix1 = pixaDisplayTiledInRows(pixa1, 32, 1500, 1.0, 0, 30, 2);
    pixWrite("/tmp/lept/misc/boxaplots.png", pix1, IFF_PNG);
    pixDisplay(pix1, 800, 0);
    pixDestroy(&pix1);
    pixaDestroy(&pixa1);

        /* Extract and display rank sized components */
    pixs = pixRead("rabi-tiny.png");
    pixa1 = pixaCreate(0);
    for (i = 1; i <= 5; i++) {
        pixaAddPix(pixa1, pixs, L_COPY);
        pixGetDimensions(pixs, &w, &h, NULL);
        pixd = pixCreate(w, h, 32);
        pixSetAll(pixd);
        for (j = 0; j < 6; j++) {
            pix1 = pixSelectComponentBySize(pixs, j, i, 8, &box1);
            pix2 = pixConvertTo32(pix1);
            boxGetGeometry(box1, &bx, &by, &bw, &bh);
            pixRasterop(pixd, bx, by, bw, bh, PIX_SRC, pix2, 0, 0);
            box2 = boxAdjustSides(NULL, box1, -2, 2, -2, 2);
            pixRenderBoxArb(pixd, box2, 2, 255, 0, 0);
            pixaAddPix(pixa1, pixd, L_COPY);
            pixDestroy(&pix1);
            pixDestroy(&pix2);
            boxDestroy(&box1);
            boxDestroy(&box2);
        }
        pixDestroy(&pixd);
    }
    pix3 = pixaDisplayTiledAndScaled(pixa1, 32, 300, 7, 0, 30, 2);
    pixWrite("/tmp/lept/misc/comps.png", pix3, IFF_PNG);
    pixDisplay(pix3, 600, 300);
    pixaDestroy(&pixa1);
    pixDestroy(&pixs);
    pixDestroy(&pix3);

        /* Extract parts of an image using a boxa */
    pix1 = pixRead("feyn-fract.tif");
    boxa1 = pixConnCompBB(pix1, 4);
    boxa2 = boxaSelectBySize(boxa1, 0, 28, L_SELECT_HEIGHT, L_SELECT_IF_GT, NULL),
    pix2 = pixCopyWithBoxa(pix1, boxa2, L_SET_WHITE);
    pixWrite("/tmp/lept/misc/tallcomps.png", pix2, IFF_PNG);
    pixDisplay(pix2, 600, 600);
    pixDestroy(&pix1);
    pixDestroy(&pix2);
    boxaDestroy(&boxa1);
    boxaDestroy(&boxa2);

    return 0;
}


