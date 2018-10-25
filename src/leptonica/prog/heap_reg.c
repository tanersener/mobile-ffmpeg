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
 * heap_reg.c
 *
 *   Tests the heap utility.
 */

#include "allheaders.h"

struct HeapElement {
    l_float32  distance;
    l_int32    x;
    l_int32    y;
};
typedef struct HeapElement  HEAPEL;

static const l_int32  NELEM = 50;


int main(int    argc,
         char **argv)
{
l_int32      i;
l_float32    frand, fval;
HEAPEL      *item;
NUMA        *na;
L_HEAP      *lh;
static char  mainName[] = "heap_reg";

    if (argc != 1)
        return ERROR_INT(" Syntax: heap_reg", mainName, 1);
    setLeptDebugOK(1);

        /* make a numa of random numbers */
    na = numaCreate(5);
    for (i = 0; i < NELEM; i++) {
        frand = (l_float32)rand() / (l_float32)RAND_MAX;
        numaAddNumber(na, frand);
    }

        /* make an array of HEAPELs with the same numbers */
    lh = lheapCreate(5, L_SORT_INCREASING);
    for (i = 0; i < NELEM; i++) {
        numaGetFValue(na, i, &fval);
        item = (HEAPEL *)lept_calloc(1, sizeof(HEAPEL));
        item->distance = fval;
        lheapAdd(lh, item);
    }
    lheapPrint(stderr, lh);

        /* switch the direction and resort into a heap */
    lh->direction = L_SORT_DECREASING;
    lheapSort(lh);
    lheapPrint(stderr, lh);

        /* resort for strict order */
    lheapSortStrictOrder(lh);
    lheapPrint(stderr, lh);

        /* switch the direction again and resort into a heap */
    lh->direction = L_SORT_INCREASING;
    lheapSort(lh);
    lheapPrint(stderr, lh);

        /* remove the elements, one at a time */
    for (i = 0; lheapGetCount(lh) > 0; i++) {
        item = (HEAPEL *)lheapRemove(lh);
        fprintf(stderr, "item %d: %f\n", i, item->distance);
        lept_free(item);
    }

    lheapDestroy(&lh, 1);
    numaDestroy(&na);
    return 0;
}

