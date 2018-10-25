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
 * graymorph2_reg.c
 *
 *   Compares graymorph results with special (3x1, 1x3, 3x3) cases
 *   against the general case.  Require exact equality.
 */

#include "allheaders.h"

int main(int    argc,
         char **argv)
{
PIX          *pixs, *pix1, *pix2, *pixd;
PIXA         *pixa;
L_REGPARAMS  *rp;

    if (regTestSetup(argc, argv, &rp))
        return 1;

    pixs = pixRead("test8.jpg");

        /* Dilation */
    pixa = pixaCreate(0);
    pixSaveTiled(pixs, pixa, 1.0, 1, 20, 8);
    pix1 = pixDilateGray3(pixs, 3, 1);
    pixSaveTiled(pix1, pixa, 1.0, 1, 20, 8);
    pix2 = pixDilateGray(pixs, 3, 1);
    pixSaveTiled(pix2, pixa, 1.0, 0, 20, 8);
    regTestComparePix(rp, pix1, pix2);  /* 0 */
    pixDestroy(&pix1);
    pixDestroy(&pix2);

    pix1 = pixDilateGray3(pixs, 1, 3);
    pixSaveTiled(pix1, pixa, 1.0, 1, 20, 8);
    pix2 = pixDilateGray(pixs, 1, 3);
    pixSaveTiled(pix2, pixa, 1.0, 0, 20, 8);
    regTestComparePix(rp, pix1, pix2);  /* 1 */
    pixDestroy(&pix1);
    pixDestroy(&pix2);

    pix1 = pixDilateGray3(pixs, 3, 3);
    pixSaveTiled(pix1, pixa, 1.0, 1, 20, 8);
    pix2 = pixDilateGray(pixs, 3, 3);
    pixSaveTiled(pix2, pixa, 1.0, 0, 20, 8);
    regTestComparePix(rp, pix1, pix2);  /* 2 */
    pixDestroy(&pix1);
    pixDestroy(&pix2);

    pixd = pixaDisplay(pixa, 0, 0);
    pixDisplayWithTitle(pixd, 0, 100, "Dilation", rp->display);
    pixDestroy(&pixd);
    pixaDestroy(&pixa);

        /* Erosion */
    pixa = pixaCreate(0);
    pixSaveTiled(pixs, pixa, 1.0, 1, 20, 8);
    pix1 = pixErodeGray3(pixs, 3, 1);
    pixSaveTiled(pix1, pixa, 1.0, 1, 20, 8);
    pix2 = pixErodeGray(pixs, 3, 1);
    pixSaveTiled(pix2, pixa, 1.0, 0, 20, 8);
    regTestComparePix(rp, pix1, pix2);  /* 3 */
    pixDestroy(&pix1);
    pixDestroy(&pix2);

    pix1 = pixErodeGray3(pixs, 1, 3);
    pixSaveTiled(pix1, pixa, 1.0, 1, 20, 8);
    pix2 = pixErodeGray(pixs, 1, 3);
    pixSaveTiled(pix2, pixa, 1.0, 0, 20, 8);
    regTestComparePix(rp, pix1, pix2);  /* 4 */
    pixDestroy(&pix1);
    pixDestroy(&pix2);

    pix1 = pixErodeGray3(pixs, 3, 3);
    pixSaveTiled(pix1, pixa, 1.0, 1, 20, 8);
    pix2 = pixErodeGray(pixs, 3, 3);
    pixSaveTiled(pix2, pixa, 1.0, 0, 20, 8);
    regTestComparePix(rp, pix1, pix2);  /* 5 */
    pixDestroy(&pix1);
    pixDestroy(&pix2);

    pixd = pixaDisplay(pixa, 0, 0);
    pixDisplayWithTitle(pixd, 250, 100, "Erosion", rp->display);
    pixDestroy(&pixd);
    pixaDestroy(&pixa);

        /* Opening */
    pixa = pixaCreate(0);
    pixSaveTiled(pixs, pixa, 1.0, 1, 20, 8);
    pix1 = pixOpenGray3(pixs, 3, 1);
    pixSaveTiled(pix1, pixa, 1.0, 1, 20, 8);
    pix2 = pixOpenGray(pixs, 3, 1);
    pixSaveTiled(pix2, pixa, 1.0, 0, 20, 8);
    regTestComparePix(rp, pix1, pix2);  /* 6 */
    pixDestroy(&pix1);
    pixDestroy(&pix2);

    pix1 = pixOpenGray3(pixs, 1, 3);
    pixSaveTiled(pix1, pixa, 1.0, 1, 20, 8);
    pix2 = pixOpenGray(pixs, 1, 3);
    pixSaveTiled(pix2, pixa, 1.0, 0, 20, 8);
    regTestComparePix(rp, pix1, pix2);  /* 7 */
    pixDestroy(&pix1);
    pixDestroy(&pix2);

    pix1 = pixOpenGray3(pixs, 3, 3);
    pixSaveTiled(pix1, pixa, 1.0, 1, 20, 8);
    pix2 = pixOpenGray(pixs, 3, 3);
    pixSaveTiled(pix2, pixa, 1.0, 0, 20, 8);
    regTestComparePix(rp, pix1, pix2);  /* 8 */
    pixDestroy(&pix1);
    pixDestroy(&pix2);

    pixd = pixaDisplay(pixa, 0, 0);
    pixDisplayWithTitle(pixd, 500, 100, "Opening", rp->display);
    pixDestroy(&pixd);
    pixaDestroy(&pixa);

        /* Closing */
    pixa = pixaCreate(0);
    pixSaveTiled(pixs, pixa, 1.0, 1, 20, 8);
    pix1 = pixCloseGray3(pixs, 3, 1);
    pixSaveTiled(pix1, pixa, 1.0, 1, 20, 8);
    pix2 = pixCloseGray(pixs, 3, 1);
    pixSaveTiled(pix2, pixa, 1.0, 0, 20, 8);
    regTestComparePix(rp, pix1, pix2);  /* 9 */
    pixDestroy(&pix1);
    pixDestroy(&pix2);

    pix1 = pixCloseGray3(pixs, 1, 3);
    pixSaveTiled(pix1, pixa, 1.0, 1, 20, 8);
    pix2 = pixCloseGray(pixs, 1, 3);
    pixSaveTiled(pix2, pixa, 1.0, 0, 20, 8);
    regTestComparePix(rp, pix1, pix2);  /* 10 */
    pixDestroy(&pix1);
    pixDestroy(&pix2);

    pix1 = pixCloseGray3(pixs, 3, 3);
    pixSaveTiled(pix1, pixa, 1.0, 1, 20, 8);
    pix2 = pixCloseGray(pixs, 3, 3);
    pixSaveTiled(pix2, pixa, 1.0, 0, 20, 8);
    regTestComparePix(rp, pix1, pix2);  /* 11 */
    pixDestroy(&pix1);
    pixDestroy(&pix2);

    pixd = pixaDisplay(pixa, 0, 0);
    pixDisplayWithTitle(pixd, 750, 100, "Closing", rp->display);
    pixDestroy(&pixd);
    pixaDestroy(&pixa);

    pixDestroy(&pixs);
    return regTestCleanup(rp);
}
