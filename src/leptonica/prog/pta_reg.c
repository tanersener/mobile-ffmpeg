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
 *  pta_reg.c
 *
 *  This tests several ptaa functions, including:
 *     -  ptaaGetBoundaryPixels()
 *     -  pixRenderRandomCmapPtaa()
 *     -  pixDisplayPtaa()
 *
 *  Also tests these pta functions:
 *     -  pixRenderPtaArb()
 *     -  ptaRotate()
 */

#include "allheaders.h"

static PIX *PtaDisplayRotate(PIX *pixs, l_float32 xc, l_float32 yc);

int main(int    argc,
         char **argv)
{
l_int32       i, nbox, npta, fgcount, bgcount, count;
BOXA         *boxa;
PIX          *pixs, *pixfg, *pixbg, *pixc, *pixb, *pixd;
PIX          *pix1, *pix2, *pix3, *pix4;
PIXA         *pixa;
PTA          *pta;
PTAA         *ptaafg, *ptaabg;
L_REGPARAMS  *rp;

    if (regTestSetup(argc, argv, &rp))
        return 1;

    pixs = pixRead("feyn-fract.tif");
    boxa = pixConnComp(pixs, NULL, 8);
    nbox = boxaGetCount(boxa);
    regTestCompareValues(rp, nbox, 464, 0);  /* 0 */

        /* Get fg and bg boundary pixels */
    pixfg = pixMorphSequence(pixs, "e3.3", 0);
    pixXor(pixfg, pixfg, pixs);
    pixCountPixels(pixfg, &fgcount, NULL);
    regTestCompareValues(rp, fgcount, 58764, 0);  /* 1 */

    pixbg = pixMorphSequence(pixs, "d3.3", 0);
    pixXor(pixbg, pixbg, pixs);
    pixCountPixels(pixbg, &bgcount, NULL);
    regTestCompareValues(rp, bgcount, 60335, 0);  /* 2 */

        /* Get ptaa of fg pixels */
    ptaafg = ptaaGetBoundaryPixels(pixs, L_BOUNDARY_FG, 8, NULL, NULL);
    npta = ptaaGetCount(ptaafg);
    regTestCompareValues(rp, npta, nbox, 0);  /* 3 */
    count = 0;
    for (i = 0; i < npta; i++) {
        pta = ptaaGetPta(ptaafg, i, L_CLONE);
        count += ptaGetCount(pta);
        ptaDestroy(&pta);
    }
    regTestCompareValues(rp, fgcount, count, 0);  /* 4 */

        /* Get ptaa of bg pixels.  Note that the number of bg pts
         * is, in general, larger than the number of bg boundary pixels,
         * because bg boundary pixels are shared by two c.c. that
         * are 1 pixel apart. */
    ptaabg = ptaaGetBoundaryPixels(pixs, L_BOUNDARY_BG, 8, NULL, NULL);
    npta = ptaaGetCount(ptaabg);
    regTestCompareValues(rp, npta, nbox, 0);  /* 5 */
    count = 0;
    for (i = 0; i < npta; i++) {
        pta = ptaaGetPta(ptaabg, i, L_CLONE);
        count += ptaGetCount(pta);
        ptaDestroy(&pta);
    }
    regTestCompareValues(rp, count, 60602, 0);  /* 6 */

        /* Render the fg boundary pixels on top of pixs. */
    pixa = pixaCreate(4);
    pixc = pixRenderRandomCmapPtaa(pixs, ptaafg, 0, 0, 0);
    regTestWritePixAndCheck(rp, pixc, IFF_PNG);  /* 7 */
    pixSaveTiledOutline(pixc, pixa, 1.0, 1, 30, 2, 32);
    pixDestroy(&pixc);

        /* Render the bg boundary pixels on top of pixs. */
    pixc = pixRenderRandomCmapPtaa(pixs, ptaabg, 0, 0, 0);
    regTestWritePixAndCheck(rp, pixc, IFF_PNG);  /* 8 */
    pixSaveTiledOutline(pixc, pixa, 1.0, 0, 30, 2, 32);
    pixDestroy(&pixc);

    pixClearAll(pixs);

        /* Render the fg boundary pixels alone. */
    pixc = pixRenderRandomCmapPtaa(pixs, ptaafg, 0, 0, 0);
    regTestWritePixAndCheck(rp, pixc, IFF_PNG);  /* 9 */
    pixSaveTiledOutline(pixc, pixa, 1.0, 1, 30, 2, 32);

        /* Verify that the fg pixels are the same set as we
         * originally started with. */
    pixb = pixConvertTo1(pixc, 255);
    regTestComparePix(rp, pixb, pixfg);  /* 10 */
    pixDestroy(&pixc);
    pixDestroy(&pixb);

        /* Render the bg boundary pixels alone. */
    pixc = pixRenderRandomCmapPtaa(pixs, ptaabg, 0, 0, 0);
    regTestWritePixAndCheck(rp, pixc, IFF_PNG);  /* 11 */
    pixSaveTiledOutline(pixc, pixa, 1.0, 0, 30, 2, 32);

        /* Verify that the bg pixels are the same set as we
         * originally started with. */
    pixb = pixConvertTo1(pixc, 255);
    regTestComparePix(rp, pixb, pixbg);  /* 12 */
    pixDestroy(&pixc);
    pixDestroy(&pixb);

    pixd = pixaDisplay(pixa, 0, 0);
    pixDisplayWithTitle(pixd, 0, 0, NULL, rp->display);
    ptaaDestroy(&ptaafg);
    ptaaDestroy(&ptaabg);
    pixDestroy(&pixs);
    pixDestroy(&pixfg);
    pixDestroy(&pixbg);
    pixDestroy(&pixd);
    pixaDestroy(&pixa);
    boxaDestroy(&boxa);

        /* Test rotation */
    pix1 = pixRead("feyn-word.tif");
    pix2 = pixAddBorderGeneral(pix1, 200, 200, 200, 200, 0);
    pixa = pixaCreate(0);
    pix3 = PtaDisplayRotate(pix2, 0, 0);
    pixaAddPix(pixa, pix3, L_INSERT);
    pix3 = PtaDisplayRotate(pix2, 500, 100);
    pixaAddPix(pixa, pix3, L_INSERT);
    pix3 = PtaDisplayRotate(pix2, 100, 410);
    pixaAddPix(pixa, pix3, L_INSERT);
    pix3 = PtaDisplayRotate(pix2, 500, 410);
    pixaAddPix(pixa, pix3, L_INSERT);
    pix4 = pixaDisplayTiledInRows(pixa, 32, 1500, 1.0, 0, 30, 2);
    regTestWritePixAndCheck(rp, pix4, IFF_PNG);  /* 13 */
    pixDisplayWithTitle(pix4, 800, 0, NULL, rp->display);
    pixDestroy(&pix1);
    pixDestroy(&pix2);
    pixDestroy(&pix4);
    pixaDestroy(&pixa);

    return regTestCleanup(rp);
}


static PIX *
PtaDisplayRotate(PIX       *pixs,
                 l_float32  xc,
                 l_float32  yc)
{
l_int32  i, w, h;
PIX     *pix1, *pix2;
PTA     *pta1, *pta2, *pta3, *pta4;
PTAA    *ptaa;

        /* Save rotated sets of pixels */
    pta1 = ptaGetPixelsFromPix(pixs, NULL);
    ptaa = ptaaCreate(0);
    for (i = 0; i < 9; i++) {
        pta2 = ptaRotate(pta1, xc, yc, -0.8 + 0.2 * i);
        ptaaAddPta(ptaa, pta2, L_INSERT);
    }
    ptaDestroy(&pta1);

        /* Render them */
    pixGetDimensions(pixs, &w, &h, NULL);
    pix1 = pixCreate(w, h, 32);
    pixSetAll(pix1);
    pta3 = generatePtaFilledCircle(4);
    pta4 = ptaTranslate(pta3, xc, yc);
    pixRenderPtaArb(pix1, pta4, 255, 0, 0);  /* circle at rotation center */
    pix2 = pixDisplayPtaa(pix1, ptaa);  /* rotated sets */

    pixDestroy(&pix1);
    ptaDestroy(&pta3);
    ptaDestroy(&pta4);
    ptaaDestroy(&ptaa);
    return pix2;
}
