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
 * blend4_reg.c
 *
 *   Regression test for this function:
 *       pixAddAlphaToBlend()
 *
 *   Blending is done using pixBlendWithGrayMask()
 */

#include "allheaders.h"

static const char *blenders[] =
            {"feyn-word.tif", "weasel4.16c.png", "karen8.jpg"};

int main(int    argc,
         char **argv)
{
l_int32       i, w, h;
PIX          *pix0, *pix1, *pix2, *pix3, *pix4, *pix5;
PIXA         *pixa;
L_REGPARAMS  *rp;

    if (regTestSetup(argc, argv, &rp))
        return 1;

    pixa = pixaCreate(0);

        /* Blending on a light image */
    pix1 = pixRead("fish24.jpg");
    pixGetDimensions(pix1, &w, &h, NULL);
    for (i = 0; i < 3; i++) {
        pix2 = pixRead(blenders[i]);
        if (i == 2) {
            pix3 = pixScale(pix2, 0.5, 0.5);
            pixDestroy(&pix2);
            pix2 = pix3;
        }
        pix3 = pixAddAlphaToBlend(pix2, 0.3, 0);
        pix4 = pixMirroredTiling(pix3, w, h);
        pix5 = pixBlendWithGrayMask(pix1, pix4, NULL, 0, 0);
        pixaAddPix(pixa, pix5, L_INSERT);
        regTestWritePixAndCheck(rp, pix5, IFF_JFIF_JPEG);  /* 0 - 2 */
        pixDisplayWithTitle(pix5, 200 * i, 0, NULL, rp->display);
        pixDestroy(&pix2);
        pixDestroy(&pix3);
        pixDestroy(&pix4);
    }
    pixDestroy(&pix1);

        /* Blending on a dark image */
    pix0 = pixRead("karen8.jpg");
    pix1 = pixScale(pix0, 2.0, 2.0);
    pixGetDimensions(pix1, &w, &h, NULL);
    for (i = 0; i < 2; i++) {
        pix2 = pixRead(blenders[i]);
        pix3 = pixAddAlphaToBlend(pix2, 0.3, 1);
        pix4 = pixMirroredTiling(pix3, w, h);
        pix5 = pixBlendWithGrayMask(pix1, pix4, NULL, 0, 0);
        pixaAddPix(pixa, pix5, L_INSERT);
        regTestWritePixAndCheck(rp, pix5, IFF_JFIF_JPEG);  /* 3 - 4 */
        pixDisplayWithTitle(pix5, 600 + 200 * i, 0, NULL, rp->display);
        pixDestroy(&pix2);
        pixDestroy(&pix3);
        pixDestroy(&pix4);
    }

    pixaConvertToPdf(pixa, 100, 1.0, L_JPEG_ENCODE, 0,
                     "Blendings: blend4_reg", "/tmp/lept/regout/blend.pdf");
    L_INFO("Output pdf: /tmp/lept/regout/blend.pdf\n", rp->testname);
    pixDestroy(&pix0);
    pixDestroy(&pix1);
    pixaDestroy(&pixa);

    return regTestCleanup(rp);
}

