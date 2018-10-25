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
 *  projection_reg.c
 *
 *    Tests projection stats for rows and columns.
 *    Just for interest, a number of different tests are done.
 */

#include "allheaders.h"

void TestProjection(L_REGPARAMS *rp, PIX *pix);

int main(int    argc,
         char **argv)
{
PIX          *pixs, *pixg1, *pixg2;
L_REGPARAMS  *rp;

    if (regTestSetup(argc, argv, &rp))
        return 1;

        /* Use for input two different images */
    pixs = pixRead("projectionstats.jpg");
    pixg1 = pixConvertTo8(pixs, 0);
    pixDestroy(&pixs);
    pixs = pixRead("feyn.tif");
    pixg2 = pixScaleToGray4(pixs);
    pixDestroy(&pixs);

    TestProjection(rp, pixg1);
    TestProjection(rp, pixg2);
    pixDestroy(&pixg1);
    pixDestroy(&pixg2);
    return regTestCleanup(rp);
}


/*
 *  Test both vertical and horizontal projections on this image.
 *  We rotate the image by 90 degrees for the horizontal projection,
 *  so that the two results should be identical.
 */
void
TestProjection(L_REGPARAMS  *rp,
               PIX          *pixs)
{
l_int32  outline;
NUMA    *na1, *na2, *na3, *na4, *na5, *na6;
NUMA    *na7, *na8, *na9, *na10, *na11, *na12;
PIX     *pixd, *pixt;
PIXA    *pixa;

    outline = 2;
    pixColumnStats(pixs, NULL, &na1, &na3, &na5, &na7, &na9, &na11);
    pixd = pixRotateOrth(pixs, 1);
    pixRowStats(pixd, NULL, &na2, &na4, &na6, &na8, &na10, &na12);

        /* The png plot files are written to "/tmp/lept/regout/proj.0.png", etc.
         * These temp files are overwritten each time this
         * function is called. */
    gplotSimple1(na1, GPLOT_PNG, "/tmp/lept/regout/proj.0", "Mean value");
    gplotSimple1(na2, GPLOT_PNG, "/tmp/lept/regout/proj.1", "Mean value");
    gplotSimple1(na3, GPLOT_PNG, "/tmp/lept/regout/proj.2", "Median value");
    gplotSimple1(na4, GPLOT_PNG, "/tmp/lept/regout/proj.3", "Median value");
    gplotSimple1(na5, GPLOT_PNG, "/tmp/lept/regout/proj.4", "Mode value");
    gplotSimple1(na6, GPLOT_PNG, "/tmp/lept/regout/proj.5", "Mode value");
    gplotSimple1(na7, GPLOT_PNG, "/tmp/lept/regout/proj.6", "Mode count");
    gplotSimple1(na8, GPLOT_PNG, "/tmp/lept/regout/proj.7", "Mode count");
    gplotSimple1(na9, GPLOT_PNG, "/tmp/lept/regout/proj.8", "Variance");
    gplotSimple1(na10, GPLOT_PNG, "/tmp/lept/regout/proj.9", "Variance");
    gplotSimple1(na11, GPLOT_PNG, "/tmp/lept/regout/proj.10",
                 "Square Root Variance");
    gplotSimple1(na12, GPLOT_PNG, "/tmp/lept/regout/proj.11",
                 "Square Root Variance");

        /* Each of the 12 plot files is read into a pix and then:
         *    (1) saved into a pixa for display
         *    (2) saved as a golden file (generate stage) or compared
         *        to the existing golden file (testing stage)    */
    pixa = pixaCreate(13);
    pixSaveTiledOutline(pixs, pixa, 1.0, 1, 30, outline, 32);
    pixt = pixRead("/tmp/lept/regout/proj.0.png");
    regTestWritePixAndCheck(rp, pixt, IFF_PNG);   /* 0 */
    pixSaveTiledOutline(pixt, pixa, 1.0, 1, 30, outline, 32);
    pixDestroy(&pixt);
    pixt = pixRead("/tmp/lept/regout/proj.1.png");
    regTestWritePixAndCheck(rp, pixt, IFF_PNG);  /* 1 */
    pixSaveTiledOutline(pixt, pixa, 1.0, 0, 30, outline, 32);
    pixDestroy(&pixt);
    pixt = pixRead("/tmp/lept/regout/proj.2.png");
    regTestWritePixAndCheck(rp, pixt, IFF_PNG);  /* 2 */
    pixSaveTiledOutline(pixt, pixa, 1.0, 1, 30, outline, 32);
    pixDestroy(&pixt);
    pixt = pixRead("/tmp/lept/regout/proj.3.png");
    regTestWritePixAndCheck(rp, pixt, IFF_PNG);  /* 3 */
    pixSaveTiledOutline(pixt, pixa, 1.0, 0, 30, outline, 32);
    pixDestroy(&pixt);
    pixt = pixRead("/tmp/lept/regout/proj.4.png");
    regTestWritePixAndCheck(rp, pixt, IFF_PNG);  /* 4 */
    pixSaveTiledOutline(pixt, pixa, 1.0, 1, 30, outline, 32);
    pixDestroy(&pixt);
    pixt = pixRead("/tmp/lept/regout/proj.5.png");
    regTestWritePixAndCheck(rp, pixt, IFF_PNG);  /* 5 */
    pixSaveTiledOutline(pixt, pixa, 1.0, 0, 30, outline, 32);
    pixDestroy(&pixt);
    pixt = pixRead("/tmp/lept/regout/proj.6.png");
    regTestWritePixAndCheck(rp, pixt, IFF_PNG);  /* 6 */
    pixSaveTiledOutline(pixt, pixa, 1.0, 1, 30, outline, 32);
    pixDestroy(&pixt);
    pixt = pixRead("/tmp/lept/regout/proj.7.png");
    regTestWritePixAndCheck(rp, pixt, IFF_PNG);  /* 7 */
    pixSaveTiledOutline(pixt, pixa, 1.0, 0, 30, outline, 32);
    pixDestroy(&pixt);
    pixt = pixRead("/tmp/lept/regout/proj.8.png");
    regTestWritePixAndCheck(rp, pixt, IFF_PNG);  /* 8 */
    pixSaveTiledOutline(pixt, pixa, 1.0, 1, 30, outline, 32);
    pixDestroy(&pixt);
    pixt = pixRead("/tmp/lept/regout/proj.9.png");
    regTestWritePixAndCheck(rp, pixt, IFF_PNG);  /* 9 */
    pixSaveTiledOutline(pixt, pixa, 1.0, 0, 30, outline, 32);
    pixDestroy(&pixt);
    pixt = pixRead("/tmp/lept/regout/proj.10.png");
    regTestWritePixAndCheck(rp, pixt, IFF_PNG);  /* 10 */
    pixSaveTiledOutline(pixt, pixa, 1.0, 1, 30, outline, 32);
    pixDestroy(&pixt);
    pixt = pixRead("/tmp/lept/regout/proj.11.png");
    regTestWritePixAndCheck(rp, pixt, IFF_PNG);  /* 11 */
    pixSaveTiledOutline(pixt, pixa, 1.0, 0, 30, outline, 32);
    pixDestroy(&pixt);

        /* The pixa is composited into a pix and 'goldened'/tested */
    pixt = pixaDisplay(pixa, 0, 0);
    regTestWritePixAndCheck(rp, pixt, IFF_PNG);  /* 12 */
    pixDisplayWithTitle(pixt, 100, 0, NULL, rp->display);
    pixDestroy(&pixt);
    pixaDestroy(&pixa);

        /* The 12 plot files are tested in pairs for identity */
    regTestCompareFiles(rp, 0, 1);
    regTestCompareFiles(rp, 2, 3);
    regTestCompareFiles(rp, 4, 5);
    regTestCompareFiles(rp, 6, 7);
    regTestCompareFiles(rp, 8, 9);
    regTestCompareFiles(rp, 10, 11);

    pixDestroy(&pixd);
    numaDestroy(&na1);
    numaDestroy(&na2);
    numaDestroy(&na3);
    numaDestroy(&na4);
    numaDestroy(&na5);
    numaDestroy(&na6);
    numaDestroy(&na7);
    numaDestroy(&na8);
    numaDestroy(&na9);
    numaDestroy(&na10);
    numaDestroy(&na11);
    numaDestroy(&na12);
    return;
}
