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
 * numa_reg2.c
 *
 *   Tests:
 *     * numa windowed stats
 *     * numa extraction from pix on a line
 *     * pixel averages and variances
 */

#include <math.h>
#ifndef  _WIN32
#include <unistd.h>
#else
#include <windows.h>   /* for Sleep() */
#endif  /* _WIN32 */
#include "allheaders.h"

#define   DO_ALL     0


int main(int    argc,
         char **argv)
{
l_int32      i, j;
l_int32      w, h, bw, bh, wpls, rval, gval, bval, same;
l_uint32     pixel;
l_uint32    *lines, *datas;
l_float32    sum1, sum2, ave1, ave2, ave3, ave4, diff1, diff2;
l_float32    var1, var2, var3;
BOX         *box1, *box2;
NUMA        *na, *na1, *na2, *na3, *na4;
PIX         *pix, *pixs, *pix1, *pix2, *pix3, *pix4, *pix5, *pixg, *pixd;
PIXA        *pixa;
L_REGPARAMS  *rp;

    if (regTestSetup(argc, argv, &rp))
        return 1;

    lept_mkdir("lept/numa2");

    /* -------------------------------------------------------------------*
     *                         Numa-windowed stats                        *
     * -------------------------------------------------------------------*/
    na = numaRead("lyra.5.na");
    numaWindowedStats(na, 5, &na1, &na2, &na3, &na4);
    gplotSimple1(na, GPLOT_PNG, "/tmp/lept/numa2/lyra1", "Original");
    gplotSimple1(na1, GPLOT_PNG, "/tmp/lept/numa2/lyra2", "Mean");
    gplotSimple1(na2, GPLOT_PNG, "/tmp/lept/numa2/lyra3", "Mean Square");
    gplotSimple1(na3, GPLOT_PNG, "/tmp/lept/numa2/lyra4", "Variance");
    gplotSimple1(na4, GPLOT_PNG, "/tmp/lept/numa2/lyra5", "RMS Difference");
    pix1 = pixRead("/tmp/lept/numa2/lyra1.png");
    pix2 = pixRead("/tmp/lept/numa2/lyra2.png");
    pix3 = pixRead("/tmp/lept/numa2/lyra3.png");
    pix4 = pixRead("/tmp/lept/numa2/lyra4.png");
    pix5 = pixRead("/tmp/lept/numa2/lyra5.png");
    regTestWritePixAndCheck(rp, pix1, IFF_PNG);  /* 0 */
    regTestWritePixAndCheck(rp, pix2, IFF_PNG);  /* 1 */
    regTestWritePixAndCheck(rp, pix3, IFF_PNG);  /* 2 */
    regTestWritePixAndCheck(rp, pix4, IFF_PNG);  /* 3 */
    regTestWritePixAndCheck(rp, pix5, IFF_PNG);  /* 4 */
    pixa = pixaCreate(5);
    pixaAddPix(pixa, pix1, L_INSERT);
    pixaAddPix(pixa, pix2, L_INSERT);
    pixaAddPix(pixa, pix3, L_INSERT);
    pixaAddPix(pixa, pix4, L_INSERT);
    pixaAddPix(pixa, pix5, L_INSERT);
    if (rp->display) {
        pixd = pixaDisplayTiledInRows(pixa, 32, 1500, 1.0, 0, 20, 2);
        pixDisplayWithTitle(pixd, 0, 0, NULL, 1);
        pixDestroy(&pixd);
    }
    pixaDestroy(&pixa);
    numaDestroy(&na);
    numaDestroy(&na1);
    numaDestroy(&na2);
    numaDestroy(&na3);
    numaDestroy(&na4);

    /* -------------------------------------------------------------------*
     *                        Extraction on a line                        *
     * -------------------------------------------------------------------*/
        /* First, make a pretty image */
    w = h = 200;
    pixs = pixCreate(w, h, 32);
    wpls = pixGetWpl(pixs);
    datas = pixGetData(pixs);
    for (i = 0; i < 200; i++) {
        lines = datas + i * wpls;
        for (j = 0; j < 200; j++) {
            rval = (l_int32)((255. * j) / w + (255. * i) / h);
            gval = (l_int32)((255. * 2 * j) / w + (255. * 2 * i) / h) % 255;
            bval = (l_int32)((255. * 4 * j) / w + (255. * 4 * i) / h) % 255;
            composeRGBPixel(rval, gval, bval, &pixel);
            lines[j] = pixel;
        }
    }
    pixg = pixConvertTo8(pixs, 0);  /* and a grayscale version */
    regTestWritePixAndCheck(rp, pixg, IFF_PNG);  /* 5 */
    pixDisplayWithTitle(pixg, 0, 300, NULL, rp->display);

    na1 = pixExtractOnLine(pixg, 20, 20, 180, 20, 1);
    na2 = pixExtractOnLine(pixg, 40, 30, 40, 170, 1);
    na3 = pixExtractOnLine(pixg, 20, 170, 180, 30, 1);
    na4 = pixExtractOnLine(pixg, 20, 190, 180, 10, 1);
    gplotSimple1(na1, GPLOT_PNG, "/tmp/lept/numa2/ext1", "Horizontal");
    gplotSimple1(na2, GPLOT_PNG, "/tmp/lept/numa2/ext2", "Vertical");
    gplotSimple1(na3, GPLOT_PNG, "/tmp/lept/numa2/ext3",
                "Slightly more horizontal than vertical");
    gplotSimple1(na4, GPLOT_PNG, "/tmp/lept/numa2/ext4",
                "Slightly more vertical than horizontal");
    pix1 = pixRead("/tmp/lept/numa2/ext1.png");
    pix2 = pixRead("/tmp/lept/numa2/ext2.png");
    pix3 = pixRead("/tmp/lept/numa2/ext3.png");
    pix4 = pixRead("/tmp/lept/numa2/ext4.png");
    regTestWritePixAndCheck(rp, pix1, IFF_PNG);  /* 6 */
    regTestWritePixAndCheck(rp, pix2, IFF_PNG);  /* 7 */
    regTestWritePixAndCheck(rp, pix3, IFF_PNG);  /* 8 */
    regTestWritePixAndCheck(rp, pix4, IFF_PNG);  /* 9 */
    pixa = pixaCreate(4);
    pixaAddPix(pixa, pix1, L_INSERT);
    pixaAddPix(pixa, pix2, L_INSERT);
    pixaAddPix(pixa, pix3, L_INSERT);
    pixaAddPix(pixa, pix4, L_INSERT);
    if (rp->display) {
        pixd = pixaDisplayTiledInRows(pixa, 32, 1500, 1.0, 0, 20, 2);
        pixDisplayWithTitle(pixd, 300, 0, NULL, 1);
        pixDestroy(&pixd);
    }
    pixaDestroy(&pixa);
    pixDestroy(&pixg);
    pixDestroy(&pixs);
    numaDestroy(&na1);
    numaDestroy(&na2);
    numaDestroy(&na3);
    numaDestroy(&na4);

    /* -------------------------------------------------------------------*
     *                     Row and column pixel sums                      *
     * -------------------------------------------------------------------*/
        /* Sum by columns in two halves (left and right) */
    pixs = pixRead("test8.jpg");
    pixGetDimensions(pixs, &w, &h, NULL);
    box1 = boxCreate(0, 0, w / 2, h);
    box2 = boxCreate(w / 2, 0, w - 2 / 2, h);
    na1 = pixAverageByColumn(pixs, box1, L_BLACK_IS_MAX);
    na2 = pixAverageByColumn(pixs, box2, L_BLACK_IS_MAX);
    numaJoin(na1, na2, 0, -1);
    na3 = pixAverageByColumn(pixs, NULL, L_BLACK_IS_MAX);
    numaSimilar(na1, na3, 0.0, &same);  /* for columns */
    regTestCompareValues(rp, 1, same, 0);  /* 10 */
    pix1 = pixConvertTo32(pixs);
    pixRenderPlotFromNumaGen(&pix1, na3, L_HORIZONTAL_LINE, 3, h / 2, 80, 1,
                             0xff000000);
    pixRenderPlotFromNuma(&pix1, na3, L_PLOT_AT_BOT, 3, 80, 0xff000000);
    boxDestroy(&box1);
    boxDestroy(&box2);
    numaDestroy(&na1);
    numaDestroy(&na2);
    numaDestroy(&na3);

        /* Sum by rows in two halves (top and bottom) */
    box1 = boxCreate(0, 0, w, h / 2);
    box2 = boxCreate(0, h / 2, w, h - h / 2);
    na1 = pixAverageByRow(pixs, box1, L_WHITE_IS_MAX);
    na2 = pixAverageByRow(pixs, box2, L_WHITE_IS_MAX);
    numaJoin(na1, na2, 0, -1);
    na3 = pixAverageByRow(pixs, NULL, L_WHITE_IS_MAX);
    numaSimilar(na1, na3, 0.0, &same);  /* for rows */
    regTestCompareValues(rp, 1, same, 0);  /* 11 */
    pixRenderPlotFromNumaGen(&pix1, na3, L_VERTICAL_LINE, 3, w / 2, 80, 1,
                             0x00ff0000);
    pixRenderPlotFromNuma(&pix1, na3, L_PLOT_AT_RIGHT, 3, 80, 0x00ff0000);
    regTestWritePixAndCheck(rp, pix1, IFF_PNG);  /* 12 */
    pixDisplayWithTitle(pix1, 0, 600, NULL, rp->display);
    pixDestroy(&pix1);
    boxDestroy(&box1);
    boxDestroy(&box2);
    numaDestroy(&na1);
    numaDestroy(&na2);
    numaDestroy(&na3);

        /* Average left by rows; right by columns; compare totals */
    box1 = boxCreate(0, 0, w / 2, h);
    box2 = boxCreate(w / 2, 0, w - 2 / 2, h);
    na1 = pixAverageByRow(pixs, box1, L_WHITE_IS_MAX);
    na2 = pixAverageByColumn(pixs, box2, L_WHITE_IS_MAX);
    numaGetSum(na1, &sum1);  /* sum of averages of left box */
    numaGetSum(na2, &sum2);  /* sum of averages of right box */
    ave1 = sum1 / h;
    ave2 = 2.0 * sum2 / w;
    ave3 = 0.5 * (ave1 + ave2);  /* average over both halves */
    regTestCompareValues(rp, 189.59, ave1, 0.01);  /* 13 */
    regTestCompareValues(rp, 207.89, ave2, 0.01);  /* 14 */

    if (rp->display) {
        fprintf(stderr, "ave1 = %8.4f\n", sum1 / h);
        fprintf(stderr, "ave2 = %8.4f\n", 2.0 * sum2 / w);
    }
    pixAverageInRect(pixs, NULL, &ave4);  /* entire image */
    diff1 = ave4 - ave3;
    diff2 = w * h * ave4 - (0.5 * w * sum1 + h * sum2);
    regTestCompareValues(rp, 0.0, diff1, 0.001);  /* 15 */
    regTestCompareValues(rp, 10.0, diff2, 10.0);  /* 16 */

        /* Variance left and right halves.  Variance doesn't average
         * in a simple way, unlike pixel sums. */
    pixVarianceInRect(pixs, box1, &var1);  /* entire image */
    pixVarianceInRect(pixs, box2, &var2);  /* entire image */
    pixVarianceInRect(pixs, NULL, &var3);  /* entire image */
    regTestCompareValues(rp, 82.06, 0.5 * (var1 + var2), 0.01);  /* 17 */
    regTestCompareValues(rp, 82.66, var3, 0.01);  /* 18 */
    boxDestroy(&box1);
    boxDestroy(&box2);
    numaDestroy(&na1);
    numaDestroy(&na2);

    /* -------------------------------------------------------------------*
     *                     Row and column variances                       *
     * -------------------------------------------------------------------*/
        /* Display variance by rows and columns */
    box1 = boxCreate(415, 0, 130, 425);
    boxGetGeometry(box1, NULL, NULL, &bw, &bh);
    na1 = pixVarianceByRow(pixs, box1);
    na2 = pixVarianceByColumn(pixs, box1);
    pix1 = pixConvertTo32(pixs);
    pix2 = pixCopy(NULL, pix1);
    pixRenderPlotFromNumaGen(&pix1, na1, L_VERTICAL_LINE, 3, 415, 100, 1,
                             0xff000000);
    pixRenderPlotFromNumaGen(&pix1, na2, L_HORIZONTAL_LINE, 3, bh / 2, 100, 1,
                          0x00ff0000);
    pixRenderPlotFromNuma(&pix2, na1, L_PLOT_AT_LEFT, 3, 60, 0x00ff0000);
    pixRenderPlotFromNuma(&pix2, na1, L_PLOT_AT_MID_VERT, 3, 60, 0x0000ff00);
    pixRenderPlotFromNuma(&pix2, na1, L_PLOT_AT_RIGHT, 3, 60, 0xff000000);
    pixRenderPlotFromNuma(&pix2, na2, L_PLOT_AT_TOP, 3, 60, 0x0000ff00);
    pixRenderPlotFromNuma(&pix2, na2, L_PLOT_AT_MID_HORIZ, 3, 60, 0xff000000);
    pixRenderPlotFromNuma(&pix2, na2, L_PLOT_AT_BOT, 3, 60, 0x00ff0000);
    regTestWritePixAndCheck(rp, pix1, IFF_PNG);  /* 19 */
    regTestWritePixAndCheck(rp, pix2, IFF_PNG);  /* 20 */
    pixa = pixaCreate(2);
    pixaAddPix(pixa, pix1, L_INSERT);
    pixaAddPix(pixa, pix2, L_INSERT);
    if (rp->display) {
        pixd = pixaDisplayTiledInRows(pixa, 32, 1500, 1.0, 0, 20, 2);
        pixDisplayWithTitle(pixd, 400, 600, NULL, 1);
        pixDestroy(&pixd);
    }
    pixaDestroy(&pixa);
    boxDestroy(&box1);
    numaDestroy(&na1);
    numaDestroy(&na2);
    pixDestroy(&pixs);

        /* Again on a different image */
    pix1 = pixRead("boxedpage.jpg");
    pix2 = pixConvertTo8(pix1, 0);
    pixGetDimensions(pix2, &w, &h, NULL);
    na1 = pixVarianceByRow(pix2, NULL);
    pix3 = pixConvertTo32(pix1);
    pixRenderPlotFromNumaGen(&pix3, na1, L_VERTICAL_LINE, 3, 0, 70, 1,
                             0xff000000);
    na2 = pixVarianceByColumn(pix2, NULL);
    pixRenderPlotFromNumaGen(&pix3, na2, L_HORIZONTAL_LINE, 3, bh - 1, 70, 1,
                             0x00ff0000);
    regTestWritePixAndCheck(rp, pix3, IFF_PNG);  /* 21 */
    numaDestroy(&na1);
    numaDestroy(&na2);

        /* Again, with an erosion */
    pix4 = pixErodeGray(pix2, 3, 21);
    na1 = pixVarianceByRow(pix4, NULL);
    pix5 = pixConvertTo32(pix1);
    pixRenderPlotFromNumaGen(&pix5, na1, L_VERTICAL_LINE, 3, 30, 70, 1,
                             0xff000000);
    na2 = pixVarianceByColumn(pix4, NULL);
    pixRenderPlotFromNumaGen(&pix5, na2, L_HORIZONTAL_LINE, 3, bh - 1, 70, 1,
                             0x00ff0000);
    regTestWritePixAndCheck(rp, pix5, IFF_PNG);  /* 22 */
    pixa = pixaCreate(2);
    pixaAddPix(pixa, pix3, L_INSERT);
    pixaAddPix(pixa, pix5, L_INSERT);
    if (rp->display) {
        pixd = pixaDisplayTiledInRows(pixa, 32, 1500, 1.0, 0, 20, 2);
        pixDisplayWithTitle(pixd, 800, 600, NULL, 1);
        pixDestroy(&pixd);
    }
    pixaDestroy(&pixa);
    pixDestroy(&pix1);
    pixDestroy(&pix2);
    pixDestroy(&pix4);
    numaDestroy(&na1);
    numaDestroy(&na2);

    /* -------------------------------------------------------------------*
     *                    Windowed variance along a line                  *
     * -------------------------------------------------------------------*/
    pix1 = pixRead("boxedpage.jpg");
    pix2 = pixConvertTo8(pix1, 0);
    pixGetDimensions(pix2, &w, &h, NULL);
    pix3 = pixCopy(NULL, pix1);

        /* Plot along horizontal line */
    pixWindowedVarianceOnLine(pix2, L_HORIZONTAL_LINE, h / 2 - 30, 0,
                              w, 5, &na1);
    pixRenderPlotFromNumaGen(&pix1, na1, L_HORIZONTAL_LINE, 3, h / 2 - 30,
                             80, 1, 0xff000000);
    pixRenderPlotFromNuma(&pix3, na1, L_PLOT_AT_TOP, 3, 60, 0x00ff0000);
    pixRenderPlotFromNuma(&pix3, na1, L_PLOT_AT_BOT, 3, 60, 0x0000ff00);

        /* Plot along vertical line */
    pixWindowedVarianceOnLine(pix2, L_VERTICAL_LINE, 0.78 * w, 0,
                              h, 5, &na2);
    pixRenderPlotFromNumaGen(&pix1, na2, L_VERTICAL_LINE, 3, 0.78 * w, 60,
                             1, 0x00ff0000);
    pixRenderPlotFromNuma(&pix3, na2, L_PLOT_AT_LEFT, 3, 60, 0xff000000);
    pixRenderPlotFromNuma(&pix3, na2, L_PLOT_AT_RIGHT, 3, 60, 0x00ff0000);
    regTestWritePixAndCheck(rp, pix1, IFF_PNG);  /* 23 */
    regTestWritePixAndCheck(rp, pix3, IFF_PNG);  /* 24 */
    pixa = pixaCreate(2);
    pixaAddPix(pixa, pix1, L_INSERT);
    pixaAddPix(pixa, pix3, L_INSERT);
    if (rp->display) {
        pixd = pixaDisplayTiledInRows(pixa, 32, 1500, 1.0, 0, 20, 2);
        pixDisplayWithTitle(pixd, 1200, 600, NULL, 1);
        pixDestroy(&pixd);
    }
    pixaDestroy(&pixa);
    pixDestroy(&pix2);
    numaDestroy(&na1);
    numaDestroy(&na2);

    return regTestCleanup(rp);;
}
