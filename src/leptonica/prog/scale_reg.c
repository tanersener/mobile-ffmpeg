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
 * scale_reg.c
 *
 *      This tests a number of scaling operations, through the pixScale()
 *      interface.
 */

#include "allheaders.h"

static const char *image[10] = {"feyn.tif",         /* 1 bpp */
                                "weasel2.png",      /* 2 bpp; no cmap */
                                "weasel2.4c.png",   /* 2 bpp; cmap */
                                "weasel4.png",      /* 4 bpp; no cmap */
                                "weasel4.16c.png",  /* 4 bpp; cmap */
                                "weasel8.png",      /* 8 bpp; no cmap */
                                "weasel8.240c.png", /* 8 bpp; cmap */
                                "test16.png",       /* 16 bpp rgb */
                                "marge.jpg",        /* 32 bpp rgb */
                                "test24.jpg"};      /* 32 bpp rgb */


static const l_int32    SPACE = 30;
static const l_int32    WIDTH = 300;
static const l_float32  FACTOR[5] = {2.3f, 1.5f, 1.1f, 0.6f, 0.3f};

static void AddScaledImages(PIXA *pixa, const char *fname, l_int32 width);
static void PixSave32(PIXA *pixa, PIX *pixc);
static void PixaSaveDisplay(PIXA *pixa, L_REGPARAMS *rp);


int main(int    argc,
         char **argv)
{
l_int32       i;
PIX          *pixs, *pixc;
PIXA         *pixa;
L_REGPARAMS  *rp;

    if (regTestSetup(argc, argv, &rp))
        return 1;

        /* Test 1 bpp */
    fprintf(stderr, "\n-------------- Testing 1 bpp ----------\n");
    pixa = pixaCreate(0);
    pixs = pixRead(image[0]);
    pixc = pixScale(pixs, 0.32, 0.32);
    regTestWritePixAndCheck(rp, pixc, IFF_PNG);  /* 0 */
    pixSaveTiled(pixc, pixa, 1.0, 1, SPACE, 32);
    pixDestroy(&pixc);

    pixc = pixScaleToGray3(pixs);
    regTestWritePixAndCheck(rp, pixc, IFF_PNG);  /* 1 */
    PixSave32(pixa, pixc);

    pixc = pixScaleToGray4(pixs);
    regTestWritePixAndCheck(rp, pixc, IFF_PNG);  /* 2 */
    pixSaveTiled(pixc, pixa, 1.0, 1, SPACE, 32);
    pixDestroy(&pixc);

    pixc = pixScaleToGray6(pixs);
    regTestWritePixAndCheck(rp, pixc, IFF_PNG);  /* 3 */
    PixSave32(pixa, pixc);

    pixc = pixScaleToGray8(pixs);
    regTestWritePixAndCheck(rp, pixc, IFF_PNG);  /* 4 */
    PixSave32(pixa, pixc);

    pixc = pixScaleToGray16(pixs);
    regTestWritePixAndCheck(rp, pixc, IFF_PNG);  /* 5 */
    PixSave32(pixa, pixc);
    pixDestroy(&pixs);
    PixaSaveDisplay(pixa, rp);  /* 6 */

    for (i = 1; i < 10; i++) {
        pixa = pixaCreate(0);
        AddScaledImages(pixa, image[i], WIDTH);
        PixaSaveDisplay(pixa, rp);  /* 7 - 16 */
    }

        /* Test 2 bpp without colormap */
    fprintf(stderr, "\n-------------- Testing 2 bpp without cmap ----------\n");
    pixa = pixaCreate(0);
    pixs = pixRead(image[1]);
    pixSaveTiled(pixs, pixa, 1.0, 1, SPACE, 32);
    pixc = pixScale(pixs, 2.25, 2.25);
    regTestWritePixAndCheck(rp, pixc, IFF_JFIF_JPEG);  /* 17 */
    PixSave32(pixa, pixc);
    pixc = pixScale(pixs, 0.85, 0.85);
    regTestWritePixAndCheck(rp, pixc, IFF_JFIF_JPEG);  /* 18 */
    PixSave32(pixa, pixc);
    pixc = pixScale(pixs, 0.65, 0.65);
    regTestWritePixAndCheck(rp, pixc, IFF_JFIF_JPEG);  /* 19 */
    PixSave32(pixa, pixc);
    PixaSaveDisplay(pixa, rp);  /* 20 */
    pixDestroy(&pixs);

        /* Test 2 bpp with colormap */
    fprintf(stderr, "\n-------------- Testing 2 bpp with cmap ----------\n");
    pixa = pixaCreate(0);
    pixs = pixRead(image[2]);
    pixSaveTiled(pixs, pixa, 1.0, 1, SPACE, 32);
    pixc = pixScale(pixs, 2.25, 2.25);
    regTestWritePixAndCheck(rp, pixc, IFF_PNG);  /* 21 */
    PixSave32(pixa, pixc);
    pixc = pixScale(pixs, 0.85, 0.85);
    regTestWritePixAndCheck(rp, pixc, IFF_PNG);  /* 22 */
    PixSave32(pixa, pixc);
    pixc = pixScale(pixs, 0.65, 0.65);
    regTestWritePixAndCheck(rp, pixc, IFF_PNG);  /* 23 */
    PixSave32(pixa, pixc);
    PixaSaveDisplay(pixa, rp);  /* 24 */
    pixDestroy(&pixs);

        /* Test 4 bpp without colormap */
    fprintf(stderr, "\n-------------- Testing 4 bpp without cmap ----------\n");
    pixa = pixaCreate(0);
    pixs = pixRead(image[3]);
    pixSaveTiled(pixs, pixa, 1.0, 1, SPACE, 32);
    pixc = pixScale(pixs, 1.72, 1.72);
    regTestWritePixAndCheck(rp, pixc, IFF_PNG);  /* 25 */
    PixSave32(pixa, pixc);
    pixc = pixScale(pixs, 0.85, 0.85);
    regTestWritePixAndCheck(rp, pixc, IFF_PNG);  /* 26 */
    PixSave32(pixa, pixc);
    pixc = pixScale(pixs, 0.65, 0.65);
    regTestWritePixAndCheck(rp, pixc, IFF_PNG);  /* 27 */
    PixSave32(pixa, pixc);
    PixaSaveDisplay(pixa, rp);  /* 28 */
    pixDestroy(&pixs);

        /* Test 4 bpp with colormap */
    fprintf(stderr, "\n-------------- Testing 4 bpp with cmap ----------\n");
    pixa = pixaCreate(0);
    pixs = pixRead(image[4]);
    pixSaveTiled(pixs, pixa, 1.0, 1, SPACE, 32);
    pixc = pixScale(pixs, 1.72, 1.72);
    regTestWritePixAndCheck(rp, pixc, IFF_PNG);  /* 29 */
    PixSave32(pixa, pixc);
    pixc = pixScale(pixs, 0.85, 0.85);
    regTestWritePixAndCheck(rp, pixc, IFF_PNG);  /* 30 */
    PixSave32(pixa, pixc);
    pixc = pixScale(pixs, 0.65, 0.65);
    regTestWritePixAndCheck(rp, pixc, IFF_PNG);  /* 31 */
    PixSave32(pixa, pixc);
    PixaSaveDisplay(pixa, rp);  /* 32 */
    pixDestroy(&pixs);

        /* Test 8 bpp without colormap */
    fprintf(stderr, "\n-------------- Testing 8 bpp without cmap ----------\n");
    pixa = pixaCreate(0);
    pixs = pixRead(image[5]);
    pixSaveTiled(pixs, pixa, 1.0, 1, SPACE, 32);
    pixc = pixScale(pixs, 1.92, 1.92);
    regTestWritePixAndCheck(rp, pixc, IFF_JFIF_JPEG);  /* 33 */
    PixSave32(pixa, pixc);
    pixc = pixScale(pixs, 0.85, 0.85);
    regTestWritePixAndCheck(rp, pixc, IFF_JFIF_JPEG);  /* 34 */
    PixSave32(pixa, pixc);
    pixc = pixScale(pixs, 0.65, 0.65);
    regTestWritePixAndCheck(rp, pixc, IFF_JFIF_JPEG);  /* 35 */
    PixSave32(pixa, pixc);
    pixDestroy(&pixs);
    pixs = pixRead("graytext.png");
    pixc = pixScaleToSize(pixs, 0, 32);  /* uses fast unsharp masking */
    regTestWritePixAndCheck(rp, pixc, IFF_PNG);  /* 36 */
    PixSave32(pixa, pixc);
    PixaSaveDisplay(pixa, rp);  /* 37 */
    pixDestroy(&pixs);

        /* Test 8 bpp with colormap */
    fprintf(stderr, "\n-------------- Testing 8 bpp with cmap ----------\n");
    pixa = pixaCreate(0);
    pixs = pixRead(image[6]);
    pixSaveTiled(pixs, pixa, 1.0, 1, SPACE, 32);
    pixc = pixScale(pixs, 1.92, 1.92);
    regTestWritePixAndCheck(rp, pixc, IFF_JFIF_JPEG);  /* 38 */
    PixSave32(pixa, pixc);
    pixc = pixScale(pixs, 0.85, 0.85);
    regTestWritePixAndCheck(rp, pixc, IFF_JFIF_JPEG);  /* 39 */
    PixSave32(pixa, pixc);
    pixc = pixScale(pixs, 0.65, 0.65);
    regTestWritePixAndCheck(rp, pixc, IFF_JFIF_JPEG);  /* 40 */
    PixSave32(pixa, pixc);
    PixaSaveDisplay(pixa, rp);  /* 41 */
    pixDestroy(&pixs);

        /* Test 16 bpp */
    fprintf(stderr, "\n-------------- Testing 16 bpp ------------\n");
    pixa = pixaCreate(0);
    pixs = pixRead(image[7]);
    pixSaveTiled(pixs, pixa, 1.0, 1, SPACE, 32);
    pixc = pixScale(pixs, 1.92, 1.92);
    regTestWritePixAndCheck(rp, pixc, IFF_JFIF_JPEG);  /* 42 */
    PixSave32(pixa, pixc);
    pixc = pixScale(pixs, 0.85, 0.85);
    regTestWritePixAndCheck(rp, pixc, IFF_JFIF_JPEG);  /* 43 */
    PixSave32(pixa, pixc);
    pixc = pixScale(pixs, 0.65, 0.65);
    regTestWritePixAndCheck(rp, pixc, IFF_JFIF_JPEG);  /* 44 */
    PixSave32(pixa, pixc);
    PixaSaveDisplay(pixa, rp);  /* 45 */
    pixDestroy(&pixs);

        /* Test 32 bpp */
    fprintf(stderr, "\n-------------- Testing 32 bpp ------------\n");
    pixa = pixaCreate(0);
    pixs = pixRead(image[8]);
    pixSaveTiled(pixs, pixa, 1.0, 1, SPACE, 32);
    pixc = pixScale(pixs, 1.42, 1.42);
    regTestWritePixAndCheck(rp, pixc, IFF_JFIF_JPEG);  /* 46 */
    PixSave32(pixa, pixc);
    pixc = pixScale(pixs, 0.85, 0.85);
    regTestWritePixAndCheck(rp, pixc, IFF_JFIF_JPEG);  /* 47 */
    PixSave32(pixa, pixc);
    pixc = pixScale(pixs, 0.65, 0.65);
    regTestWritePixAndCheck(rp, pixc, IFF_JFIF_JPEG);  /* 48 */
    PixSave32(pixa, pixc);
    PixaSaveDisplay(pixa, rp);  /* 49 */
    pixDestroy(&pixs);

    return regTestCleanup(rp);
}

static void
AddScaledImages(PIXA         *pixa,
                const char   *fname,
                l_int32       width)
{
l_int32    i, w;
l_float32  scalefactor;
PIX       *pixs, *pixt1, *pixt2, *pix32;

    pixs = pixRead(fname);
    w = pixGetWidth(pixs);
    for (i = 0; i < 5; i++) {
        scalefactor = (l_float32)width / (FACTOR[i] * (l_float32)w);
        pixt1 = pixScale(pixs, FACTOR[i], FACTOR[i]);
        pixt2 = pixScale(pixt1, scalefactor, scalefactor);
        pix32 = pixConvertTo32(pixt2);
        if (i == 0)
            pixSaveTiled(pix32, pixa, 1.0, 1, SPACE, 32);
        else
            pixSaveTiled(pix32, pixa, 1.0, 0, SPACE, 32);
        pixDestroy(&pixt1);
        pixDestroy(&pixt2);
        pixDestroy(&pix32);
    }
    pixDestroy(&pixs);
    return;
}

static void
PixSave32(PIXA *pixa, PIX *pixc)
{
PIX  *pix32;
    pix32 = pixConvertTo32(pixc);
    pixSaveTiled(pix32, pixa, 1.0, 0, SPACE, 32);
    pixDestroy(&pixc);
    pixDestroy(&pix32);
    return;
}

static void
PixaSaveDisplay(PIXA *pixa, L_REGPARAMS *rp)
{
PIX  *pixd;

    pixd = pixaDisplay(pixa, 0, 0);
    regTestWritePixAndCheck(rp, pixd, IFF_JFIF_JPEG);
    pixDisplayWithTitle(pixd, 100, 100, NULL, rp->display);
    pixDestroy(&pixd);
    pixaDestroy(&pixa);
    return;
}
