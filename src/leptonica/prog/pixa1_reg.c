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
 * pixa1_reg.c
 *
 *    Tests removal of connected components.
 */

#include "allheaders.h"

static const l_int32 CONNECTIVITY = 8;

int main(int    argc,
         char **argv)
{
l_int32      size, i, n, n0;
BOXA        *boxa;
GPLOT       *gplot;
NUMA        *nax, *nay1, *nay2;
PIX         *pixs, *pix1, *pix2, *pixd;
PIXA        *pixa;
static char  mainName[] = "pixa1_reg";

    if (argc != 1)
        return ERROR_INT(" Syntax:  pixa1_reg", mainName, 1);

    if ((pixs = pixRead("feyn.tif")) == NULL)
        return ERROR_INT("pixs not made", mainName, 1);

    setLeptDebugOK(1);
    lept_mkdir("lept/pixa");

    /* ----------------  Remove small components --------------- */
    boxa = pixConnComp(pixs, NULL, 8);
    n0 = boxaGetCount(boxa);
    nax = numaMakeSequence(0, 2, 51);
    nay1 = numaCreate(51);
    nay2 = numaCreate(51);
    boxaDestroy(&boxa);

    fprintf(stderr, "\n Select Large if Both\n");
    fprintf(stderr, "Iter 0: n = %d\n", n0);
    numaAddNumber(nay1, n0);
    for (i = 1; i <= 50; i++) {
        size = 2 * i;
        pixd = pixSelectBySize(pixs, size, size, CONNECTIVITY,
                               L_SELECT_IF_BOTH, L_SELECT_IF_GTE, NULL);
        boxa = pixConnComp(pixd, NULL, 8);
        n = boxaGetCount(boxa);
        numaAddNumber(nay1, n);
        fprintf(stderr, "Iter %d: n = %d\n", i, n);
        boxaDestroy(&boxa);
        pixDestroy(&pixd);
    }

    fprintf(stderr, "\n Select Large if Either\n");
    fprintf(stderr, "Iter 0: n = %d\n", n0);
    numaAddNumber(nay2, n0);
    for (i = 1; i <= 50; i++) {
        size = 2 * i;
        pixd = pixSelectBySize(pixs, size, size, CONNECTIVITY,
                               L_SELECT_IF_EITHER, L_SELECT_IF_GTE, NULL);
        boxa = pixConnComp(pixd, NULL, 8);
        n = boxaGetCount(boxa);
        numaAddNumber(nay2, n);
        fprintf(stderr, "Iter %d: n = %d\n", i, n);
        boxaDestroy(&boxa);
        pixDestroy(&pixd);
    }

    gplot = gplotCreate("/tmp/lept/pixa/root1", GPLOT_PNG,
                        "Select large: number of cc vs size removed",
                        "min size", "number of c.c.");
    gplotAddPlot(gplot, nax, nay1, GPLOT_LINES, "select if both");
    gplotAddPlot(gplot, nax, nay2, GPLOT_LINES, "select if either");
    gplotMakeOutput(gplot);
    gplotDestroy(&gplot);

    /* ----------------  Remove large components --------------- */
    numaEmpty(nay1);
    numaEmpty(nay2);

    fprintf(stderr, "\n Select Small if Both\n");
    fprintf(stderr, "Iter 0: n = %d\n", 0);
    numaAddNumber(nay1, 0);
    for (i = 1; i <= 50; i++) {
        size = 2 * i;
        pixd = pixSelectBySize(pixs, size, size, CONNECTIVITY,
                               L_SELECT_IF_BOTH, L_SELECT_IF_LTE, NULL);
        boxa = pixConnComp(pixd, NULL, 8);
        n = boxaGetCount(boxa);
        numaAddNumber(nay1, n);
        fprintf(stderr, "Iter %d: n = %d\n", i, n);
        boxaDestroy(&boxa);
        pixDestroy(&pixd);
    }

    fprintf(stderr, "\n Select Small if Either\n");
    fprintf(stderr, "Iter 0: n = %d\n", 0);
    numaAddNumber(nay2, 0);
    for (i = 1; i <= 50; i++) {
        size = 2 * i;
        pixd = pixSelectBySize(pixs, size, size, CONNECTIVITY,
                               L_SELECT_IF_EITHER, L_SELECT_IF_LTE, NULL);
        boxa = pixConnComp(pixd, NULL, 8);
        n = boxaGetCount(boxa);
        numaAddNumber(nay2, n);
        fprintf(stderr, "Iter %d: n = %d\n", i, n);
        boxaDestroy(&boxa);
        pixDestroy(&pixd);
    }

    gplot = gplotCreate("/tmp/lept/pixa/root2", GPLOT_PNG,
                        "Remove large: number of cc vs size removed",
                        "min size", "number of c.c.");
    gplotAddPlot(gplot, nax, nay1, GPLOT_LINES, "select if both");
    gplotAddPlot(gplot, nax, nay2, GPLOT_LINES, "select if either");
    gplotMakeOutput(gplot);
    gplotDestroy(&gplot);

    pixa = pixaCreate(2);
    pix1 = pixRead("/tmp/lept/pixa/root1.png");
    pix2 = pixRead("/tmp/lept/pixa/root2.png");
    pixaAddPix(pixa, pix1, L_INSERT);
    pixaAddPix(pixa, pix2, L_INSERT);
    pixd = pixaDisplayTiledInRows(pixa, 32, 1500, 1.0, 0, 20, 2);
    pixDisplay(pixd, 100, 0);
    pixWrite("/tmp/lept/pixa/root.png", pixd, IFF_PNG);
    pixDestroy(&pixd);
    pixaDestroy(&pixa);

    numaDestroy(&nax);
    numaDestroy(&nay1);
    numaDestroy(&nay2);
    pixDestroy(&pixs);
    return 0;
}
