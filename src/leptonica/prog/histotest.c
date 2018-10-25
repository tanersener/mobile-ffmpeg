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
 * histotest.c
 *
 *    Makes histograms of grayscale and color pixels
 *    from a pix.  For RGB color, this uses
 *    rgb --> octcube indexing.
 *
 *       histotest filein sigbits
 *
 *    where the number of octcubes is 8^(sigbits)
 *
 *    For gray, sigbits is ignored.
 */

#include "allheaders.h"

int main(int    argc,
         char **argv)
{
char        *filein;
l_int32      d, sigbits;
GPLOT       *gplot;
NUMA        *na;
PIX         *pixs;
static char  mainName[] = "histotest";

    if (argc != 3)
        return ERROR_INT(" Syntax:  histotest filein sigbits", mainName, 1);
    filein = argv[1];
    sigbits = atoi(argv[2]);

    setLeptDebugOK(1);
    lept_mkdir("lept/histo");

    if ((pixs = pixRead(filein)) == NULL)
        return ERROR_INT("pixs not made", mainName, 1);
    d = pixGetDepth(pixs);
    if (d != 8 && d != 32)
        return ERROR_INT("depth not 8 or 32 bpp", mainName, 1);

    if (d == 32) {
        startTimer();
        if ((na = pixOctcubeHistogram(pixs, sigbits, NULL)) == NULL)
            return ERROR_INT("na not made", mainName, 1);
        fprintf(stderr, "histo time = %7.3f sec\n", stopTimer());
        gplot = gplotCreate("/tmp/lept/histo/color", GPLOT_PNG,
                            "color histogram with octcube indexing",
                            "octcube index", "number of pixels in cube");
        gplotAddPlot(gplot, NULL, na, GPLOT_LINES, "input pix");
        gplotMakeOutput(gplot);
        gplotDestroy(&gplot);
        l_fileDisplay("/tmp/lept/histo/color.png", 100, 100, 1.0);
    }
    else {
        if ((na = pixGetGrayHistogram(pixs, 1)) == NULL)
            return ERROR_INT("na not made", mainName, 1);
        numaWrite("/tmp/junkna", na);
        gplot = gplotCreate("/tmp/lept/histo/gray", GPLOT_PNG,
                            "grayscale histogram", "gray value",
                            "number of pixels");
        gplotSetScaling(gplot, GPLOT_LOG_SCALE_Y);
        gplotAddPlot(gplot, NULL, na, GPLOT_LINES, "input pix");
        gplotMakeOutput(gplot);
        gplotDestroy(&gplot);
        l_fileDisplay("/tmp/lept/histo/gray.png", 100, 100, 1.0);
    }

    pixDestroy(&pixs);
    numaDestroy(&na);
    return 0;
}

