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
 *  pixmem_reg.c
 *
 *  Tests the low-level pix data accessors, and functions that
 *  call them.
 */

#include "allheaders.h"

void Compare(PIX *pix1, PIX *pix2, l_int32 *perror);

int main(int    argc,
         char **argv)
{
l_int32    error;
l_uint32  *data;
PIX       *pix1, *pix2, *pix3, *pix1c, *pix2c, *pix1t, *pix2t, *pixd;
PIXA      *pixa;

    setLeptDebugOK(1);
    error = 0;
    pixa = pixaCreate(0);

        /* Copy with internal resizing: onto a cmapped image */
    pix1 = pixRead("weasel4.16c.png");
    pix2 = pixRead("feyn-fract.tif");
    pix3 = pixRead("lucasta.150.jpg");
    fprintf(stderr, "before copy 2 --> 3\n");
    pixCopy(pix3, pix2);
    Compare(pix2, pix3, &error);
    pixSaveTiled(pix3, pixa, 0.25, 1, 30, 32);
    fprintf(stderr, "before copy 3 --> 1\n");
    pixCopy(pix1, pix3);
    Compare(pix2, pix1, &error);
    pixSaveTiled(pix1, pixa, 0.25, 0, 30, 32);
    pixDestroy(&pix1);
    pixDestroy(&pix2);
    pixDestroy(&pix3);

        /* Copy with internal resizing: from a cmapped image */
    pix1 = pixRead("weasel4.16c.png");
    pix2 = pixRead("feyn-fract.tif");
    pix3 = pixRead("lucasta.150.jpg");
    fprintf(stderr, "before copy 1 --> 2\n");
    pixCopy(pix2, pix1);
    Compare(pix2, pix1, &error);
    pixSaveTiled(pix2, pixa, 1.0, 1, 30, 32);
    fprintf(stderr, "before copy 2 --> 3\n");
    pixCopy(pix3, pix2);
    Compare(pix3, pix2, &error);
    pixSaveTiled(pix3, pixa, 1.0, 0, 30, 32);
    pixDestroy(&pix1);
    pixDestroy(&pix2);
    pixDestroy(&pix3);

        /* Transfer of data pixs --> pixd, when pixs is not cloned.
         * pixs is destroyed.  */
    pix1 = pixRead("weasel4.16c.png");
    pix2 = pixRead("feyn-fract.tif");
    pix3 = pixRead("lucasta.150.jpg");
    pix1c = pixCopy(NULL, pix1);
    fprintf(stderr, "before transfer 1 --> 2\n");
    pixTransferAllData(pix2, &pix1, 0, 0);
    Compare(pix2, pix1c, &error);
    pixSaveTiled(pix2, pixa, 1.0, 1, 30, 32);
    fprintf(stderr, "before transfer 2 --> 3\n");
    pixTransferAllData(pix3, &pix2, 0, 0);
    Compare(pix3, pix1c, &error);
    pixSaveTiled(pix3, pixa, 1.0, 0, 30, 32);
    pixDestroy(&pix1c);
    pixDestroy(&pix3);

        /* Another transfer of data pixs --> pixd, when pixs is not cloned.
         * pixs is destroyed. */
    pix1 = pixRead("weasel4.16c.png");
    pix2 = pixRead("feyn-fract.tif");
    pix3 = pixRead("lucasta.150.jpg");
    pix1c = pixCopy(NULL, pix1);
    pix2c = pixCopy(NULL, pix2);
    fprintf(stderr, "before copy transfer 1 --> 2\n");
    pixTransferAllData(pix2, &pix1c, 0, 0);
    Compare(pix2, pix1, &error);
    pixSaveTiled(pix2, pixa, 1.0, 0, 30, 32);
    fprintf(stderr, "before copy transfer 2 --> 3\n");
    pixTransferAllData(pix3, &pix2, 0, 0);
    Compare(pix3, pix1, &error);
    pixSaveTiled(pix3, pixa, 1.0, 0, 30, 32);
    pixDestroy(&pix1);
    pixDestroy(&pix2c);
    pixDestroy(&pix3);

        /* Transfer of data pixs --> pixd, when pixs is cloned.
         * pixs has its refcount reduced by 1. */
    pix1 = pixRead("weasel4.16c.png");
    pix2 = pixRead("feyn-fract.tif");
    pix3 = pixRead("lucasta.150.jpg");
    pix1c = pixClone(pix1);
    pix2c = pixClone(pix2);
    fprintf(stderr, "before clone transfer 1 --> 2\n");
    pixTransferAllData(pix2, &pix1c, 0, 0);
    Compare(pix2, pix1, &error);
    pixSaveTiled(pix2, pixa, 1.0, 0, 30, 32);
    fprintf(stderr, "before clone transfer 2 --> 3\n");
    pixTransferAllData(pix3, &pix2c, 0, 0);
    Compare(pix3, pix1, &error);
    pixSaveTiled(pix3, pixa, 1.0, 0, 30, 32);
    pixDestroy(&pix1);
    pixDestroy(&pix2);
    pixDestroy(&pix3);

        /* Extraction of data when pixs is not cloned, putting
         * the data into a new template of pixs. */
    pix2 = pixRead("feyn-fract.tif");
    fprintf(stderr, "no clone: before extraction and reinsertion of 2\n");
    pix2c = pixCopy(NULL, pix2);  /* for later reference */
    data = pixExtractData(pix2);
    pix2t = pixCreateTemplateNoInit(pix2);
    pixFreeData(pix2t);
    pixSetData(pix2t, data);
    Compare(pix2c, pix2t, &error);
    pixSaveTiled(pix2t, pixa, 0.25, 1, 30, 32);
    pixDestroy(&pix2);
    pixDestroy(&pix2c);
    pixDestroy(&pix2t);

        /* Extraction of data when pixs is cloned, putting
         * a copy of the data into a new template of pixs. */
    pix1 = pixRead("weasel4.16c.png");
    fprintf(stderr, "clone: before extraction and reinsertion of 1\n");
    pix1c = pixClone(pix1);  /* bump refcount of pix1 to 2 */
    data = pixExtractData(pix1);  /* should make a copy of data */
    pix1t = pixCreateTemplateNoInit(pix1);
    pixFreeData(pix1t);
    pixSetData(pix1t, data);
    Compare(pix1c, pix1t, &error);
    pixSaveTiled(pix1t, pixa, 1.0, 0, 30, 32);
    pixDestroy(&pix1);
    pixDestroy(&pix1c);
    pixDestroy(&pix1t);

    pixd = pixaDisplay(pixa, 0, 0);
    pixDisplay(pixd, 100, 100);
    pixWrite("/tmp/junkpixmem.png", pixd, IFF_PNG);
    pixaDestroy(&pixa);
    pixDestroy(&pixd);

    if (error)
        fprintf(stderr, "Fail: an error occurred\n");
    else
        fprintf(stderr, "Success: no errors\n");
    return 0;
}


void Compare(PIX      *pix1,
             PIX      *pix2,
             l_int32  *perror)
{
l_int32  same;

    if (!pix1 || !pix2) {
        fprintf(stderr, "pix not defined\n");
        *perror = 1;
        return;
    }
    pixEqual(pix1, pix2, &same);
    if (same)
        fprintf(stderr, "OK\n");
    else {
        fprintf(stderr, "Fail: not equal\n");
        *perror = 1;
    }
    return;
}
