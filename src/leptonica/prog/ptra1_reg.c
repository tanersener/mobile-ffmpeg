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
 * ptra1_reg.c
 *
 *    Testing basic ptra operations
 */

#include "allheaders.h"

static void MakePtrasFromPixa(PIXA *pixa, L_PTRA **ppapix, L_PTRA **ppabox,
                              l_int32 copyflag);
static PIXA *ReconstructPixa(L_PTRA *papix, L_PTRA *pabox, l_int32 choose);
static PIXA *ReconstructPixa1(L_PTRA *papix, L_PTRA *pabox);
static PIXA *ReconstructPixa2(L_PTRA *papix, L_PTRA *pabox);
static void DisplayResult(PIXA *pixac, PIXA **ppixa, l_int32 w, l_int32 h,
                          l_int32 newline);

#define  CHOOSE_RECON    2    /* 1 or 2 */


int main(int    argc,
         char **argv)
{
l_int32      i, n, w, h, nactual, imax;
BOX         *box;
BOXA        *boxa;
PIX         *pixs, *pixd, *pix;
PIXA        *pixas, *pixat, *pixac;
L_PTRA      *papix, *pabox, *papix2, *pabox2;
static char  mainName[] = "ptra1_reg";

    if (argc != 1)
        return ERROR_INT(" Syntax: ptra1_reg", mainName, 1);

    setLeptDebugOK(1);
    pixac = pixaCreate(0);

    if ((pixs = pixRead("lucasta.1.300.tif")) == NULL)
        return ERROR_INT("pixs not made", mainName, 1);
    pixGetDimensions(pixs, &w, &h, NULL);
    boxa = pixConnComp(pixs, &pixas, 8);
    pixDestroy(&pixs);
    boxaDestroy(&boxa);
    n = pixaGetCount(pixas);

        /* Fill ptras with clones and reconstruct */
    fprintf(stderr, "Fill with clones and reconstruct\n");
    MakePtrasFromPixa(pixas, &papix, &pabox, L_CLONE);
    pixat = ReconstructPixa(papix, pabox, CHOOSE_RECON);
    ptraDestroy(&papix, 0, 1);
    ptraDestroy(&pabox, 0, 1);
    DisplayResult(pixac, &pixat, w, h, 1);

        /* Remove every other one for the first half;
         * with compaction at each removal */
    fprintf(stderr, "Remove every other in 1st half, with compaction\n");
    MakePtrasFromPixa(pixas, &papix, &pabox, L_COPY);
    for (i = 0; i < n / 2; i++) {
        if (i % 2 == 0) {
            pix = (PIX *)ptraRemove(papix, i, L_COMPACTION);
            box = (BOX *)ptraRemove(pabox, i, L_COMPACTION);
            pixDestroy(&pix);
            boxDestroy(&box);
        }
    }
    pixat = ReconstructPixa(papix, pabox, CHOOSE_RECON);
    ptraDestroy(&papix, 0, 1);
    ptraDestroy(&pabox, 0, 1);
    DisplayResult(pixac, &pixat, w, h, 0);

        /* Remove every other one for the entire set,
         * but without compaction at each removal */
    fprintf(stderr,
            "Remove every other in 1st half, without & then with compaction\n");
    MakePtrasFromPixa(pixas, &papix, &pabox, L_COPY);
    for (i = 0; i < n; i++) {
        if (i % 2 == 0) {
            pix = (PIX *)ptraRemove(papix, i, L_NO_COMPACTION);
            box = (BOX *)ptraRemove(pabox, i, L_NO_COMPACTION);
            pixDestroy(&pix);
            boxDestroy(&box);
        }
    }
    ptraCompactArray(papix);  /* now do the compaction */
    ptraCompactArray(pabox);
    pixat = ReconstructPixa(papix, pabox, CHOOSE_RECON);
    ptraDestroy(&papix, 0, 1);
    ptraDestroy(&pabox, 0, 1);
    DisplayResult(pixac, &pixat, w, h, 0);

        /* Fill ptras using insert at head, and reconstruct */
    fprintf(stderr, "Insert at head and reconstruct\n");
    papix = ptraCreate(n);
    pabox = ptraCreate(n);
    for (i = 0; i < n; i++) {
        pix = pixaGetPix(pixas, i, L_CLONE);
        box = pixaGetBox(pixas, i, L_CLONE);
        ptraInsert(papix, 0, pix, L_MIN_DOWNSHIFT);
        ptraInsert(pabox, 0, box, L_FULL_DOWNSHIFT);
    }
    pixat = ReconstructPixa(papix, pabox, CHOOSE_RECON);
    ptraDestroy(&papix, 0, 1);
    ptraDestroy(&pabox, 0, 1);
    DisplayResult(pixac, &pixat, w, h, 1);

        /* Reverse the arrays by swapping */
    fprintf(stderr, "Reverse by swapping\n");
    MakePtrasFromPixa(pixas, &papix, &pabox, L_CLONE);
    for (i = 0; i < n / 2; i++) {
        ptraSwap(papix, i, n - i - 1);
        ptraSwap(pabox, i, n - i - 1);
    }
    ptraCompactArray(papix);  /* already compact; shouldn't do anything */
    ptraCompactArray(pabox);
    pixat = ReconstructPixa(papix, pabox, CHOOSE_RECON);
    ptraDestroy(&papix, 0, 1);
    ptraDestroy(&pabox, 0, 1);
    DisplayResult(pixac, &pixat, w, h, 0);

        /* Remove at the top of the array and push the hole to the end
         * by neighbor swapping (!).  This is O(n^2), so it's not a
         * recommended way to copy a ptra. [joke]  */
    fprintf(stderr,
            "Remove at top, pushing hole to end by swapping -- O(n^2)\n");
    MakePtrasFromPixa(pixas, &papix, &pabox, L_CLONE);
    papix2 = ptraCreate(0);
    pabox2 = ptraCreate(0);
    while (1) {
        ptraGetActualCount(papix, &nactual);
        if (nactual == 0) break;
        ptraGetMaxIndex(papix, &imax);
        pix = (PIX *)ptraRemove(papix, 0, L_NO_COMPACTION);
        box = (BOX *)ptraRemove(pabox, 0, L_NO_COMPACTION);
        ptraAdd(papix2, pix);
        ptraAdd(pabox2, box);
        for (i = 1; i <= imax; i++) {
           ptraSwap(papix, i - 1, i);
           ptraSwap(pabox, i - 1, i);
        }
    }
    ptraCompactArray(papix);  /* should be empty */
    ptraCompactArray(pabox);  /* ditto */
    pixat = ReconstructPixa(papix, pabox, CHOOSE_RECON);
    ptraDestroy(&papix, 0, 1);
    ptraDestroy(&pabox, 0, 1);
    DisplayResult(pixac, &pixat, w, h, 1);  /* nothing there */
    pixat = ReconstructPixa(papix2, pabox2, CHOOSE_RECON);
    ptraDestroy(&papix2, 0, 1);
    ptraDestroy(&pabox2, 0, 1);
    DisplayResult(pixac, &pixat, w, h, 0);

        /* Remove and insert one position above, allowing minimum downshift.
         * If you specify L_AUTO_DOWNSHIFT, because there is only 1 hole,
         * it will do a full downshift at each insert.  This is a
         * situation where the heuristic (expected number of holes)
         * fails to do the optimal thing. */
    fprintf(stderr, "Remove and insert one position above (min downshift)\n");
    MakePtrasFromPixa(pixas, &papix, &pabox, L_CLONE);
    for (i = 1; i < n; i++) {
        pix = (PIX *)ptraRemove(papix, i, L_NO_COMPACTION);
        box = (BOX *)ptraRemove(pabox, i, L_NO_COMPACTION);
        ptraInsert(papix, i - 1, pix, L_MIN_DOWNSHIFT);
        ptraInsert(pabox, i - 1, box, L_MIN_DOWNSHIFT);
    }
    pixat = ReconstructPixa(papix, pabox, CHOOSE_RECON);
    ptraDestroy(&papix, 0, 1);
    ptraDestroy(&pabox, 0, 1);
    DisplayResult(pixac, &pixat, w, h, 1);

        /* Remove and insert one position above, but this time
         * forcing a full downshift at each step.  */
    fprintf(stderr, "Remove and insert one position above (full downshift)\n");
    MakePtrasFromPixa(pixas, &papix, &pabox, L_CLONE);
    for (i = 1; i < n; i++) {
        pix = (PIX *)ptraRemove(papix, i, L_NO_COMPACTION);
        box = (BOX *)ptraRemove(pabox, i, L_NO_COMPACTION);
        ptraInsert(papix, i - 1, pix, L_AUTO_DOWNSHIFT);
        ptraInsert(pabox, i - 1, box, L_AUTO_DOWNSHIFT);
    }
/*    ptraCompactArray(papix);
    ptraCompactArray(pabox); */
    pixat = ReconstructPixa(papix, pabox, CHOOSE_RECON);
    ptraDestroy(&papix, 0, 1);
    ptraDestroy(&pabox, 0, 1);
    DisplayResult(pixac, &pixat, w, h, 0);

    pixd = pixaDisplay(pixac, 0, 0);
    pixDisplay(pixd, 100, 100);
    pixWrite("/tmp/junkptra1.png", pixd, IFF_PNG);
    pixDestroy(&pixd);
    pixaDestroy(&pixac);
    pixaDestroy(&pixas);
    return 0;
}


static void
MakePtrasFromPixa(PIXA     *pixa,
                  L_PTRA  **ppapix,
                  L_PTRA  **ppabox,
                  l_int32   copyflag)
{
l_int32  i, n;
BOX     *box;
PIX     *pix;
L_PTRA  *papix, *pabox;

    n = pixaGetCount(pixa);
    papix = ptraCreate(n);
    pabox = ptraCreate(n);
    for (i = 0; i < n; i++) {
        pix = pixaGetPix(pixa, i, copyflag);
        box = pixaGetBox(pixa, i, copyflag);
        ptraAdd(papix, pix);
        ptraAdd(pabox, box);
    }

    *ppapix = papix;
    *ppabox = pabox;
    return;
}


static PIXA *
ReconstructPixa(L_PTRA  *papix,
                L_PTRA  *pabox,
                l_int32  choose)
{
PIXA  *pixa;

    if (choose == 1)
        pixa = ReconstructPixa1(papix, pabox);
    else
        pixa = ReconstructPixa2(papix, pabox);
    return pixa;
}


    /* Reconstruction without compaction */
static PIXA *
ReconstructPixa1(L_PTRA  *papix,
                 L_PTRA  *pabox)
{
l_int32  i, imax, nactual;
BOX     *box;
PIX     *pix;
PIXA    *pixat;

    ptraGetMaxIndex(papix, &imax);
    ptraGetActualCount(papix, &nactual);
    fprintf(stderr, "Before removal:  imax = %4d, actual = %4d\n",
            imax, nactual);

    pixat = pixaCreate(imax + 1);
    for (i = 0; i <= imax; i++) {
        pix = (PIX *)ptraRemove(papix, i, L_NO_COMPACTION);
        box = (BOX *)ptraRemove(pabox, i, L_NO_COMPACTION);
        if (pix) pixaAddPix(pixat, pix, L_INSERT);
        if (box) pixaAddBox(pixat, box, L_INSERT);
    }

    ptraGetMaxIndex(papix, &imax);
    ptraGetActualCount(papix, &nactual);
    fprintf(stderr, "After removal:   imax = %4d, actual = %4d\n\n",
            imax, nactual);

    return pixat;
}


    /* Reconstruction with compaction */
static PIXA *
ReconstructPixa2(L_PTRA  *papix,
                 L_PTRA  *pabox)
{
l_int32  i, imax, nactual;
BOX     *box;
PIX     *pix;
PIXA    *pixat;

    ptraGetMaxIndex(papix, &imax);
    ptraGetActualCount(papix, &nactual);
    fprintf(stderr, "Before removal:    imax = %4d, actual = %4d\n",
            imax, nactual);

        /* Remove half */
    pixat = pixaCreate(imax + 1);
    for (i = 0; i <= imax; i++) {
        if (i % 2 == 0) {
            pix = (PIX *)ptraRemove(papix, i, L_NO_COMPACTION);
            box = (BOX *)ptraRemove(pabox, i, L_NO_COMPACTION);
            if (pix) pixaAddPix(pixat, pix, L_INSERT);
            if (box) pixaAddBox(pixat, box, L_INSERT);
        }
    }

        /* Compact */
    ptraGetMaxIndex(papix, &imax);
    ptraGetActualCount(papix, &nactual);
    fprintf(stderr, "Before compaction: imax = %4d, actual = %4d\n",
            imax, nactual);
    ptraCompactArray(papix);
    ptraCompactArray(pabox);
    ptraGetMaxIndex(papix, &imax);
    ptraGetActualCount(papix, &nactual);
    fprintf(stderr, "After compaction:  imax = %4d, actual = %4d\n",
            imax, nactual);

        /* Remove the rest (and test compaction with removal) */
    while (1) {
        ptraGetActualCount(papix, &nactual);
        if (nactual == 0) break;

        pix = (PIX *)ptraRemove(papix, 0, L_COMPACTION);
        box = (BOX *)ptraRemove(pabox, 0, L_COMPACTION);
        pixaAddPix(pixat, pix, L_INSERT);
        pixaAddBox(pixat, box, L_INSERT);
    }

    ptraGetMaxIndex(papix, &imax);
    ptraGetActualCount(papix, &nactual);
    fprintf(stderr, "After removal:     imax = %4d, actual = %4d\n\n",
            imax, nactual);

    return pixat;
}


static void
DisplayResult(PIXA   *pixac,
              PIXA  **ppixa,
              l_int32  w,
              l_int32  h,
              l_int32  newline)
{
PIX   *pixd;

    pixd = pixaDisplay(*ppixa, w, h);
    pixSaveTiled(pixd, pixac, 1, newline, 30, 8);
    pixDestroy(&pixd);
    pixaDestroy(ppixa);
    return;
}
