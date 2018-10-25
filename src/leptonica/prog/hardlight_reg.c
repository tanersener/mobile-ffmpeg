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
 * hardlight_reg.c
 *
 */

#include "allheaders.h"

static PIXA *TestHardlight(const char *file1, const char *file2,
                            L_REGPARAMS *rp);

int main(int    argc,
         char **argv)
{
PIX          *pix;
PIXA         *pixa;
L_REGPARAMS  *rp;

    if (regTestSetup(argc, argv, &rp))
        return 1;

    pixa = TestHardlight("hardlight1_1.jpg", "hardlight1_2.jpg", rp);
    pix = pixaDisplay(pixa, 0, 0);
    regTestWritePixAndCheck(rp, pix, IFF_PNG);
    pixDisplayWithTitle(pix, 0, 0, NULL, rp->display);
    pixaDestroy(&pixa);
    pixDestroy(&pix);

    pixa = TestHardlight("hardlight2_1.jpg", "hardlight2_2.jpg", rp);
    pix = pixaDisplay(pixa, 0, 500);
    regTestWritePixAndCheck(rp, pix, IFF_PNG);
    pixDisplayWithTitle(pix, 0, 0, NULL, rp->display);
    pixaDestroy(&pixa);
    pixDestroy(&pix);

    return regTestCleanup(rp);
}

static PIXA *
TestHardlight(const char   *file1,
              const char   *file2,
              L_REGPARAMS  *rp)
{
PIX   *pixs1, *pixs2, *pixt1, *pixt2, *pixd;
PIXA  *pixa;

    PROCNAME("TestHardlight");

        /* Read in images */
    pixs1 = pixRead(file1);
    pixs2 = pixRead(file2);
    if (!pixs1)
        return (PIXA *)ERROR_PTR("pixs1 not read", procName, NULL);
    if (!pixs2)
        return (PIXA *)ERROR_PTR("pixs2 not read", procName, NULL);

    pixa = pixaCreate(0);

        /* ---------- Test not-in-place; no colormaps ----------- */
    pixSaveTiled(pixs1, pixa, 1.0, 1, 20, 32);
    pixSaveTiled(pixs2, pixa, 1.0, 0, 20, 0);
    pixd = pixBlendHardLight(NULL, pixs1, pixs2, 0, 0, 1.0);
    regTestWritePixAndCheck(rp, pixd, IFF_PNG);
    pixSaveTiled(pixd, pixa, 1.0, 1, 20, 0);
    pixDestroy(&pixd);

    pixt2 = pixConvertTo32(pixs2);
    pixd = pixBlendHardLight(NULL, pixs1, pixt2, 0, 0, 1.0);
    regTestWritePixAndCheck(rp, pixd, IFF_PNG);
    pixSaveTiled(pixd, pixa, 1.0, 0, 20, 0);
    pixDestroy(&pixt2);
    pixDestroy(&pixd);

    pixd = pixBlendHardLight(NULL, pixs2, pixs1, 0, 0, 1.0);
    pixSaveTiled(pixd, pixa, 1.0, 0, 20, 0);
    pixDestroy(&pixd);

        /* ---------- Test not-in-place; colormaps ----------- */
    pixt1 = pixMedianCutQuant(pixs1, 0);
    if (pixGetDepth(pixs2) == 8)
        pixt2 = pixConvertGrayToColormap8(pixs2, 8);
    else
/*        pixt2 = pixConvertTo8(pixs2, 1); */
        pixt2 = pixMedianCutQuant(pixs2, 0);
    pixSaveTiled(pixt1, pixa, 1.0, 1, 20, 0);
    pixSaveTiled(pixt2, pixa, 1.0, 0, 20, 0);

    pixd = pixBlendHardLight(NULL, pixt1, pixs2, 0, 0, 1.0);
    regTestWritePixAndCheck(rp, pixd, IFF_PNG);
    pixSaveTiled(pixd, pixa, 1.0, 1, 20, 0);
    pixDestroy(&pixd);

    pixd = pixBlendHardLight(NULL, pixt1, pixt2, 0, 0, 1.0);
    regTestWritePixAndCheck(rp, pixd, IFF_PNG);
    pixSaveTiled(pixd, pixa, 1.0, 0, 20, 0);
    pixDestroy(&pixd);

    pixd = pixBlendHardLight(NULL, pixt2, pixt1, 0, 0, 1.0);
    regTestWritePixAndCheck(rp, pixd, IFF_PNG);
    pixSaveTiled(pixd, pixa, 1.0, 0, 20, 0);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixd);

        /* ---------- Test in-place; no colormaps ----------- */
    pixBlendHardLight(pixs1, pixs1, pixs2, 0, 0, 1.0);
    regTestWritePixAndCheck(rp, pixs1, IFF_PNG);
    pixSaveTiled(pixs1, pixa, 1.0, 1, 20, 0);
    pixDestroy(&pixs1);

    pixs1 = pixRead(file1);
    pixt2 = pixConvertTo32(pixs2);
    pixBlendHardLight(pixs1, pixs1, pixt2, 0, 0, 1.0);
    regTestWritePixAndCheck(rp, pixs1, IFF_PNG);
    pixSaveTiled(pixs1, pixa, 1.0, 0, 20, 0);
    pixDestroy(&pixt2);
    pixDestroy(&pixs1);

    pixs1 = pixRead(file1);
    pixBlendHardLight(pixs2, pixs2, pixs1, 0, 0, 1.0);
    regTestWritePixAndCheck(rp, pixs2, IFF_PNG);
    pixSaveTiled(pixs2, pixa, 1.0, 0, 20, 0);
    pixDestroy(&pixs2);

    pixDestroy(&pixs1);
    pixDestroy(&pixs2);
    return pixa;
}
