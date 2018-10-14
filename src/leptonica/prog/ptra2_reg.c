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
 * ptra2_reg.c
 *
 *    Testing:
 *       - basic ptra and ptraa operations
 *       - bin sort using ptra
 *       - boxaEqual() and pixaEqual()
 */

#include "allheaders.h"

void BoxaSortTest(const char *fname, l_int32 index, const char *text);
void PixaSortTest(const char *fname, l_int32 index, const char *text);

int main(int    argc,
         char **argv)
{
    BoxaSortTest("feyn-fract.tif", 1, "Boxa sort test on small image");
    BoxaSortTest("feyn.tif", 2, "Boxa sort test on large image");
    PixaSortTest("feyn-fract.tif", 3, "Pixa sort test on small image");
    PixaSortTest("feyn.tif", 4, "Pixa sort test on large image");
    return 0;
}


void
BoxaSortTest(const char  *fname,
             l_int32      index,
             const char  *text)
{
l_int32   i, n, m, imax, w, h, x, count, same;
BOX      *box;
BOXA     *boxa, *boxa1, *boxa2, *boxa3;
NUMA     *na, *nad1, *nad2, *nad3, *naindex;
PIX      *pixs;
L_PTRA   *pa, *pad, *paindex;
L_PTRAA  *paa;
char     buf[256];

    setLeptDebugOK(1);
    lept_mkdir("lept/ptra");

    fprintf(stderr, "\nTest %d: %s\n", index, text);
    pixs = pixRead(fname);
    boxa = pixConnComp(pixs, NULL, 8);

        /* Sort by x */
    boxa1 = boxaSort(boxa, L_SORT_BY_X, L_SORT_INCREASING, &nad1);
    snprintf(buf, sizeof(buf), "/tmp/lept/prta/boxa1.%d.ba", index);
    boxaWrite(buf, boxa1);
    snprintf(buf, sizeof(buf), "/tmp/lept/prta/nad1.%d.na", index);
    numaWrite(buf, nad1);

    startTimer();
    boxa2 = boxaBinSort(boxa, L_SORT_BY_X, L_SORT_INCREASING, &nad2);
    snprintf(buf, sizeof(buf), "/tmp/lept/prta/boxa2.%d.ba", index);
    boxaWrite(buf, boxa2);
    snprintf(buf, sizeof(buf), "/tmp/lept/prta/nad2.%d.na", index);
    numaWrite(buf, nad2);

    boxaEqual(boxa1, boxa2, 0, &naindex, &same);
    if (same)
        fprintf(stderr, "boxa1 and boxa2 are identical\n");
    else
        fprintf(stderr, "boxa1 and boxa2 are not identical\n");
    numaDestroy(&naindex);
    boxaEqual(boxa1, boxa2, 2, &naindex, &same);
    if (same)
        fprintf(stderr, "boxa1 and boxa2 are same at maxdiff = 2\n");
    else
        fprintf(stderr, "boxa1 and boxa2 differ at maxdiff = 2\n");
    snprintf(buf, sizeof(buf), "/tmp/lept/prta/naindex.%d.na", index);
    numaWrite(buf, naindex);
    numaDestroy(&naindex);
    boxaDestroy(&boxa1);
    numaDestroy(&nad1);
    numaDestroy(&nad2);

        /* Now do this stuff with ptra and ptraa */
        /* First, store the boxes in a ptraa, where each ptra contains
         * the boxes, and store the sort index in a ptra of numa */
    startTimer();
    pixGetDimensions(pixs, &w, &h, NULL);
    paa = ptraaCreate(w);
    paindex = ptraCreate(w);
    n = boxaGetCount(boxa);
    fprintf(stderr, "n = %d\n", n);
    for (i = 0; i < n; i++) {
        box = boxaGetBox(boxa, i, L_CLONE);
        boxGetGeometry(box, &x, NULL, NULL, NULL);
        pa = ptraaGetPtra(paa, x, L_HANDLE_ONLY);
        na = (NUMA *)ptraGetPtrToItem(paindex, x);
        if (!pa) {  /* na also needs to be made */
            pa = ptraCreate(1);
            ptraaInsertPtra(paa, x, pa);
            na = numaCreate(1);
            ptraInsert(paindex, x, na, L_MIN_DOWNSHIFT);
        }
        ptraAdd(pa, box);
        numaAddNumber(na, i);
    }

    ptraGetActualCount(paindex, &count);
    fprintf(stderr, "count = %d\n", count);

        /* Flatten the ptraa to a ptra containing all the boxes
         * in sorted order, and put them in a boxa */
    pad = ptraaFlattenToPtra(paa);
    ptraaDestroy(&paa, FALSE, FALSE);
    ptraGetActualCount(pad, &m);
    if (m != n)
        fprintf(stderr, "n(orig) = %d, m(new) = %d\n", n, m);
    boxa3 = boxaCreate(m);
    for (i = 0; i < m; i++) {
        box = (BOX *)ptraRemove(pad, i, L_NO_COMPACTION);
        boxaAddBox(boxa3, box, L_INSERT);
    }
    ptraDestroy(&pad, FALSE, FALSE);

        /* Extract the data from the ptra of Numa, putting it into
         * a single Numa */
    ptraGetMaxIndex(paindex, &imax);
    nad3 = numaCreate(0);
    fprintf(stderr, "imax = %d\n", imax);
    for (i = 0; i <= imax; i++) {
        na = (NUMA *)ptraRemove(paindex, i, L_NO_COMPACTION);
        numaJoin(nad3, na, 0, -1);
        numaDestroy(&na);
    }

    fprintf(stderr, "Time for sort: %7.3f sec\n", stopTimer());
    snprintf(buf, sizeof(buf), "/tmp/lept/prta/boxa3.%d.ba", index);
    boxaWrite(buf, boxa3);
    snprintf(buf, sizeof(buf), "/tmp/lept/prta/nad3.%d.na", index);
    numaWrite(buf, nad3);

    boxaDestroy(&boxa2);
    boxaDestroy(&boxa3);
    numaDestroy(&nad3);
    ptraDestroy(&paindex, FALSE, FALSE);

    pixDestroy(&pixs);
    boxaDestroy(&boxa);
    return;
}


void
PixaSortTest(const char  *fname,
             l_int32      index,
             const char  *text)
{
l_int32  same;
BOXA    *boxa, *boxa1, *boxa2;
NUMA    *nap1, *nap2, *naindex;
PIX     *pixs;
PIXA    *pixa, *pixa1, *pixa2;
char     buf[256];

    fprintf(stderr, "\nTest %d: %s\n", index, text);
    pixs = pixRead(fname);
    boxa = pixConnComp(pixs, &pixa, 8);

    startTimer();
    pixa1 = pixaSort(pixa, L_SORT_BY_X, L_SORT_INCREASING, &nap1, L_CLONE);
    fprintf(stderr, "Time for pixa sort: %7.3f sec\n", stopTimer());
    boxa1 = pixaGetBoxa(pixa1, L_CLONE);
    snprintf(buf, sizeof(buf), "/tmp/lept/prta/bap1.%d.ba", index);
    boxaWrite(buf, boxa1);
    snprintf(buf, sizeof(buf), "/tmp/lept/prta/nap1.%d.na", index);
    numaWrite(buf, nap1);
    snprintf(buf, sizeof(buf), "/tmp/lept/prta/pixa1.%d.pa", index);
    pixaWrite(buf, pixa1);

    startTimer();
    pixa2 = pixaBinSort(pixa, L_SORT_BY_X, L_SORT_INCREASING, &nap2, L_CLONE);
    fprintf(stderr, "Time for pixa sort: %7.3f sec\n", stopTimer());
    boxa2 = pixaGetBoxa(pixa2, L_CLONE);
    snprintf(buf, sizeof(buf), "/tmp/lept/prta/bap2.%d.ba", index);
    boxaWrite(buf, boxa2);
    snprintf(buf, sizeof(buf), "/tmp/lept/prta/nap2.%d.na", index);
    numaWrite(buf, nap2);
    snprintf(buf, sizeof(buf), "/tmp/lept/prta/pixa2.%d.pa", index);
    pixaWrite(buf, pixa2);

    startTimer();
    boxaEqual(boxa1, boxa2, 0, &naindex, &same);
    fprintf(stderr, "Time for boxaEqual: %7.3f sec\n", stopTimer());
    if (same)
        fprintf(stderr, "boxa1 and boxa2 are identical\n");
    else
        fprintf(stderr, "boxa1 and boxa2 are not identical\n");
    numaDestroy(&naindex);
    boxaEqual(boxa1, boxa2, 3, &naindex, &same);
    if (same)
        fprintf(stderr, "boxa1 and boxa2 are same at maxdiff = 3\n");
    else
        fprintf(stderr, "boxa1 and boxa2 differ at maxdiff = 3\n");
    numaDestroy(&naindex);

    startTimer();
    pixaEqual(pixa1, pixa2, 0, &naindex, &same);
    fprintf(stderr, "Time for pixaEqual: %7.3f sec\n", stopTimer());
    if (same)
        fprintf(stderr, "pixa1 and pixa2 are identical\n");
    else
        fprintf(stderr, "pixa1 and pixa2 are not identical\n");
    numaDestroy(&naindex);
    pixaEqual(pixa1, pixa2, 3, &naindex, &same);
    if (same)
        fprintf(stderr, "pixa1 and pixa2 are same at maxdiff = 3\n");
    else
        fprintf(stderr, "pixa1 and pixa2 differ at maxdiff = 3\n");
    numaDestroy(&naindex);

    boxaDestroy(&boxa);
    boxaDestroy(&boxa1);
    boxaDestroy(&boxa2);
    numaDestroy(&nap1);
    numaDestroy(&nap2);
    pixaDestroy(&pixa);
    pixaDestroy(&pixa1);
    pixaDestroy(&pixa2);
    pixDestroy(&pixs);
    return;
}
