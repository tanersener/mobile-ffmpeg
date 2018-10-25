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
 * fhmtauto_reg.c
 *
 *    Basic regression test for hit-miss transform: rasterops & dwa.
 *
 *    Tests hmt from a set of hmt structuring elements
 *    by comparing the full image rasterop results with the
 *    automatically generated dwa results.
 *
 *    Results must be identical for all operations.
 */

#include "allheaders.h"

int main(int    argc,
         char **argv)
{
l_int32      i, nsels, same1, same2, ok;
char        *filein, *selname;
PIX         *pixs, *pixref, *pixt1, *pixt2, *pixt3, *pixt4;
SEL         *sel;
SELA        *sela;
static char  mainName[] = "fhmtauto_reg";

    if (argc != 2)
        return ERROR_INT(" Syntax:  fhmtauto_reg filein", mainName, 1);
    filein = argv[1];
    setLeptDebugOK(1);

    if ((pixs = pixRead(filein)) == NULL)
        return ERROR_INT("pixs not made", mainName, 1);

    sela = selaAddHitMiss(NULL);
    nsels = selaGetCount(sela);

    ok = TRUE;
    for (i = 0; i < nsels; i++)
    {
        sel = selaGetSel(sela, i);
        selname = selGetName(sel);

        pixref = pixHMT(NULL, pixs, sel);

        pixt1 = pixAddBorder(pixs, 32, 0);
        pixt2 = pixFHMTGen_1(NULL, pixt1, selname);
        pixt3 = pixRemoveBorder(pixt2, 32);

        pixt4 = pixHMTDwa_1(NULL, pixs, selname);

        pixEqual(pixref, pixt3, &same1);
        pixEqual(pixref, pixt4, &same2);
        if (same1 && same2)
            fprintf(stderr, "hmt are identical for sel %d (%s)\n", i, selname);
        else {
            fprintf(stderr, "hmt differ for sel %d (%s)\n", i, selname);
            ok = FALSE;
        }

        pixDestroy(&pixref);
        pixDestroy(&pixt1);
        pixDestroy(&pixt2);
        pixDestroy(&pixt3);
        pixDestroy(&pixt4);
    }

    if (ok)
        fprintf(stderr, "\n ********  All hmt are correct *******\n");
    else
        fprintf(stderr, "\n ********  ERROR in at least one hmt *******\n");

    pixDestroy(&pixs);
    selaDestroy(&sela);
    return 0;
}

