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
 *   shear1_reg.c
 *
 *    Regression test for shear, both IP and to new pix.
 */

#include "allheaders.h"

#define   BINARY_IMAGE              "test1.png"
#define   TWO_BPP_IMAGE             "weasel2.4c.png"
#define   FOUR_BPP_IMAGE1           "weasel4.11c.png"
#define   FOUR_BPP_IMAGE2           "weasel4.16g.png"
#define   EIGHT_BPP_IMAGE           "test8.jpg"
#define   EIGHT_BPP_CMAP_IMAGE1     "dreyfus8.png"
#define   EIGHT_BPP_CMAP_IMAGE2     "test24.jpg"
#define   RGB_IMAGE                 "marge.jpg"

static PIX *shearTest(PIX *pixs, l_float32 scale);

static const l_float32  ANGLE1 = 3.14159265 / 12.;


l_int32 main(int    argc,
             char **argv)
{
l_int32       index;
PIX          *pixs, *pixc, *pixd;
PIXCMAP      *cmap;
L_REGPARAMS  *rp;

    if (regTestSetup(argc, argv, &rp))
        return 1;

    fprintf(stderr, "Test binary image:\n");
    pixs = pixRead(BINARY_IMAGE);
    pixd = shearTest(pixs, 1.0);
    regTestWritePixAndCheck(rp, pixd, IFF_PNG);  /* 0 */
    pixDisplayWithTitle(pixd, 100, 100, NULL, rp->display);
    pixDestroy(&pixs);
    pixDestroy(&pixd);

        /* We change the black to dark red so that we can see
         * that the IP shear does brings in that color.  It
         * can't bring in black because the cmap is filled. */
    fprintf(stderr, "Test 2 bpp cmapped image with filled cmap:\n");
    pixs = pixRead(TWO_BPP_IMAGE);
    cmap = pixGetColormap(pixs);
    pixcmapGetIndex(cmap, 40, 44, 40, &index);
    pixcmapResetColor(cmap, index, 100, 0, 0);
    pixd = shearTest(pixs, 1.0);
    regTestWritePixAndCheck(rp, pixd, IFF_PNG);  /* 1 */
    pixDisplayWithTitle(pixd, 100, 100, NULL, rp->display);
    pixDestroy(&pixs);
    pixDestroy(&pixd);

    fprintf(stderr, "Test 4 bpp cmapped image with unfilled cmap:\n");
    pixs = pixRead(FOUR_BPP_IMAGE1);
    pixd = shearTest(pixs, 1.0);
    regTestWritePixAndCheck(rp, pixd, IFF_PNG);  /* 2 */
    pixDisplayWithTitle(pixd, 100, 100, NULL, rp->display);
    pixDestroy(&pixs);
    pixDestroy(&pixd);

    fprintf(stderr, "Test 4 bpp cmapped image with filled cmap:\n");
    pixs = pixRead(FOUR_BPP_IMAGE2);
    pixd = shearTest(pixs, 1.0);
    regTestWritePixAndCheck(rp, pixd, IFF_PNG);  /* 3 */
    pixDisplayWithTitle(pixd, 100, 100, NULL, rp->display);
    pixDestroy(&pixs);
    pixDestroy(&pixd);

    fprintf(stderr, "Test 8 bpp grayscale image:\n");
    pixs = pixRead(EIGHT_BPP_IMAGE);
    pixd = shearTest(pixs, 0.5);
    regTestWritePixAndCheck(rp, pixd, IFF_JFIF_JPEG);  /* 4 */
    pixDisplayWithTitle(pixd, 100, 100, NULL, rp->display);
    pixDestroy(&pixs);
    pixDestroy(&pixd);

    fprintf(stderr, "Test 8 bpp grayscale cmap image:\n");
    pixs = pixRead(EIGHT_BPP_CMAP_IMAGE1);
    pixd = shearTest(pixs, 1.0);
    regTestWritePixAndCheck(rp, pixd, IFF_PNG);  /* 5 */
    pixDisplayWithTitle(pixd, 100, 100, NULL, rp->display);
    pixDestroy(&pixs);
    pixDestroy(&pixd);

    fprintf(stderr, "Test 8 bpp color cmap image:\n");
    pixs = pixRead(EIGHT_BPP_CMAP_IMAGE2);
    pixd = pixOctreeColorQuant(pixs, 200, 0);
    pixc = shearTest(pixd, 1.0 / 3.0);
    regTestWritePixAndCheck(rp, pixc, IFF_JFIF_JPEG);  /* 6 */
    pixDisplayWithTitle(pixc, 100, 100, NULL, rp->display);
    pixDestroy(&pixs);
    pixDestroy(&pixd);
    pixDestroy(&pixc);

    fprintf(stderr, "Test rgb image:\n");
    pixs = pixRead(RGB_IMAGE);
    pixd = shearTest(pixs, 0.5);
    regTestWritePixAndCheck(rp, pixd, IFF_JFIF_JPEG);  /* 7 */
    pixDisplayWithTitle(pixd, 100, 100, NULL, rp->display);
    pixDestroy(&pixs);
    pixDestroy(&pixd);

    return regTestCleanup(rp);
}


static PIX *
shearTest(PIX       *pixs,
          l_float32  scale)
{
l_int32  w, h, d;
PIX     *pixt1, *pixt2, *pixd;
PIXA    *pixa;

    pixa = pixaCreate(0);
    pixGetDimensions(pixs, &w, &h, &d);

    pixt1 = pixHShear(NULL, pixs, 0, ANGLE1, L_BRING_IN_WHITE);
    pixSaveTiled(pixt1, pixa, scale, 1, 20, 32);
    pixt2 = pixHShear(NULL, pixs, h / 2, ANGLE1, L_BRING_IN_WHITE);
    pixSaveTiled(pixt2, pixa, scale, 0, 20, 0);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixt1 = pixHShear(NULL, pixs, 0, ANGLE1, L_BRING_IN_BLACK);
    pixSaveTiled(pixt1, pixa, scale, 0, 20, 0);
    pixt2 = pixHShear(NULL, pixs, h / 2, ANGLE1, L_BRING_IN_BLACK);
    pixSaveTiled(pixt2, pixa, scale, 0, 20, 0);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);

    if (!pixGetColormap(pixs)) {
        pixt1 = pixCopy(NULL, pixs);
        pixHShearIP(pixt1, 0, ANGLE1, L_BRING_IN_WHITE);
        pixSaveTiled(pixt1, pixa, scale, 1, 20, 0);
        pixt2 = pixCopy(NULL, pixs);
        pixHShearIP(pixt2, h / 2, ANGLE1, L_BRING_IN_WHITE);
        pixSaveTiled(pixt2, pixa, scale, 0, 20, 0);
        pixDestroy(&pixt1);
        pixDestroy(&pixt2);
        pixt1 = pixCopy(NULL, pixs);
        pixHShearIP(pixt1, 0, ANGLE1, L_BRING_IN_BLACK);
        pixSaveTiled(pixt1, pixa, scale, 0, 20, 0);
        pixt2 = pixCopy(NULL, pixs);
        pixHShearIP(pixt2, h / 2, ANGLE1, L_BRING_IN_BLACK);
        pixSaveTiled(pixt2, pixa, scale, 0, 20, 32);
        pixDestroy(&pixt1);
        pixDestroy(&pixt2);
    }

    if (d == 8 || d == 32 || pixGetColormap(pixs)) {
        pixt1 = pixHShearLI(pixs, 0, ANGLE1, L_BRING_IN_WHITE);
        pixSaveTiled(pixt1, pixa, scale, 1, 20, 0);
        pixt2 = pixHShearLI(pixs, w / 2, ANGLE1, L_BRING_IN_WHITE);
        pixSaveTiled(pixt2, pixa, scale, 0, 20, 0);
        pixDestroy(&pixt1);
        pixDestroy(&pixt2);
        pixt1 = pixHShearLI(pixs, 0, ANGLE1, L_BRING_IN_BLACK);
        pixSaveTiled(pixt1, pixa, scale, 0, 20, 0);
        pixt2 = pixHShearLI(pixs, w / 2, ANGLE1, L_BRING_IN_BLACK);
        pixSaveTiled(pixt2, pixa, scale, 0, 20, 0);
        pixDestroy(&pixt1);
        pixDestroy(&pixt2);
    }

    pixt1 = pixVShear(NULL, pixs, 0, ANGLE1, L_BRING_IN_WHITE);
    pixSaveTiled(pixt1, pixa, scale, 1, 20, 0);
    pixt2 = pixVShear(NULL, pixs, w / 2, ANGLE1, L_BRING_IN_WHITE);
    pixSaveTiled(pixt2, pixa, scale, 0, 20, 0);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixt1 = pixVShear(NULL, pixs, 0, ANGLE1, L_BRING_IN_BLACK);
    pixSaveTiled(pixt1, pixa, scale, 0, 20, 0);
    pixt2 = pixVShear(NULL, pixs, w / 2, ANGLE1, L_BRING_IN_BLACK);
    pixSaveTiled(pixt2, pixa, scale, 0, 20, 0);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);

    if (!pixGetColormap(pixs)) {
        pixt1 = pixCopy(NULL, pixs);
        pixVShearIP(pixt1, 0, ANGLE1, L_BRING_IN_WHITE);
        pixSaveTiled(pixt1, pixa, scale, 1, 20, 0);
        pixt2 = pixCopy(NULL, pixs);
        pixVShearIP(pixt2, w / 2, ANGLE1, L_BRING_IN_WHITE);
        pixSaveTiled(pixt2, pixa, scale, 0, 20, 0);
        pixDestroy(&pixt1);
        pixDestroy(&pixt2);
        pixt1 = pixCopy(NULL, pixs);
        pixVShearIP(pixt1, 0, ANGLE1, L_BRING_IN_BLACK);
        pixSaveTiled(pixt1, pixa, scale, 0, 20, 0);
        pixt2 = pixCopy(NULL, pixs);
        pixVShearIP(pixt2, w / 2, ANGLE1, L_BRING_IN_BLACK);
        pixSaveTiled(pixt2, pixa, scale, 0, 20, 32);
        pixDestroy(&pixt1);
        pixDestroy(&pixt2);
    }

    if (d == 8 || d == 32 || pixGetColormap(pixs)) {
        pixt1 = pixVShearLI(pixs, 0, ANGLE1, L_BRING_IN_WHITE);
        pixSaveTiled(pixt1, pixa, scale, 1, 20, 0);
        pixt2 = pixVShearLI(pixs, w / 2, ANGLE1, L_BRING_IN_WHITE);
        pixSaveTiled(pixt2, pixa, scale, 0, 20, 0);
        pixDestroy(&pixt1);
        pixDestroy(&pixt2);
        pixt1 = pixVShearLI(pixs, 0, ANGLE1, L_BRING_IN_BLACK);
        pixSaveTiled(pixt1, pixa, scale, 0, 20, 0);
        pixt2 = pixVShearLI(pixs, w / 2, ANGLE1, L_BRING_IN_BLACK);
        pixSaveTiled(pixt2, pixa, scale, 0, 20, 0);
        pixDestroy(&pixt1);
        pixDestroy(&pixt2);
    }

    pixd = pixaDisplay(pixa, 0, 0);
    pixaDestroy(&pixa);
    return pixd;
}
