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
 *  colorcontent_reg.c
 *
 *   This tests various color content functions, including a simple
 *   color quantization method.
 */

#include "string.h"
#include "allheaders.h"

l_int32 main(int    argc,
             char **argv)
{
l_uint32     *colors;
l_int32       ncolors;
l_float32     fcolor;
PIX          *pix1, *pix2, *pix3;
PIXA         *pixadb;
L_REGPARAMS  *rp;

    if (regTestSetup(argc, argv, &rp))
        return 1;

        /* Find the most populated colors */
    pix1 = pixRead("fish24.jpg");
    pixGetMostPopulatedColors(pix1, 2, 3, 10, &colors, NULL);
    pix2 = pixDisplayColorArray(colors, 10, 190, 5, 6);
    pixDisplayWithTitle(pix2, 0, 0, NULL, rp->display);
    regTestWritePixAndCheck(rp, pix2, IFF_PNG);  /* 0 */
    lept_free(colors);
    pixDestroy(&pix2);

        /* Do a simple color quantization with sigbits = 2 */
    pix2 = pixSimpleColorQuantize(pix1, 2, 3, 10);
    pixDisplayWithTitle(pix2, 0, 400, NULL, rp->display);
    regTestWritePixAndCheck(rp, pix2, IFF_PNG);  /* 1 */
    pix3 = pixRemoveColormap(pix2, REMOVE_CMAP_TO_FULL_COLOR);
    regTestComparePix(rp, pix2, pix3);  /* 2 */
    pixNumColors(pix3, 1, &ncolors);
    regTestCompareValues(rp, ncolors, 10, 0.0);  /* 3 */
    pixDestroy(&pix1);
    pixDestroy(&pix2);
    pixDestroy(&pix3);

        /* Do a simple color quantization with sigbits = 3 */
    pix1 = pixRead("wyom.jpg");
    pixNumColors(pix1, 1, &ncolors);  /* >255, so should give 0 */
    regTestCompareValues(rp, ncolors, 0, 0.0);  /* 4 */
    pix2 = pixSimpleColorQuantize(pix1, 3, 3, 20);
    pixDisplayWithTitle(pix2, 1000, 0, NULL, rp->display);
    regTestWritePixAndCheck(rp, pix2, IFF_PNG);  /* 5 */
    ncolors = pixcmapGetCount(pixGetColormap(pix2));
    regTestCompareValues(rp, ncolors, 20, 0.0);  /* 6 */
    pixDestroy(&pix1);
    pixDestroy(&pix2);

        /* Find the number of perceptually significant gray intensities */
    pix1 = pixRead("marge.jpg");
    pix2 = pixConvertTo8(pix1, 0);
    pixNumSignificantGrayColors(pix2, 20, 236, 0.0001, 1, &ncolors);
    regTestCompareValues(rp, ncolors, 219, 0.0);  /* 7 */
    pixDestroy(&pix1);
    pixDestroy(&pix2);

        /* Find background color in image with light color regions */
    pix1 = pixRead("map.057.jpg");
    pixadb = pixaCreate(0);
    pixFindColorRegions(pix1, NULL, 4, 200, 70, 10, 90, 0.05,
                          &fcolor, &pix2, NULL, pixadb);
    regTestWritePixAndCheck(rp, pix2, IFF_PNG);  /* 8 */
    pix3 = pixaDisplayTiledInColumns(pixadb, 5, 0.3, 20, 2);
    regTestWritePixAndCheck(rp, pix3, IFF_PNG);  /* 9 */
    pixDisplayWithTitle(pix3, 1000, 500, NULL, rp->display);
    pixDestroy(&pix1);
    pixDestroy(&pix2);
    pixDestroy(&pix3);
    pixaDestroy(&pixadb);

    return regTestCleanup(rp);
}
