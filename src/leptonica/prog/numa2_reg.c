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

#define   DO_ALL     1


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
static char  mainName[] = "numa2_reg";

    if (argc != 1)
        return ERROR_INT(" Syntax:  numa2_reg", mainName, 1);

    setLeptDebugOK(1);
    lept_mkdir("lept/numa2");

    /* -------------------------------------------------------------------*
     *                         Numa-windowed stats                        *
     * -------------------------------------------------------------------*/
#if  DO_ALL
    na = numaRead("lyra.5.na");
    numaWindowedStats(na, 5, &na1, &na2, &na3, &na4);
    gplotSimple1(na, GPLOT_PNG, "/tmp/lept/numa2/lyra6", "Original");
    gplotSimple1(na1, GPLOT_PNG, "/tmp/lept/numa2/lyra7", "Mean");
    gplotSimple1(na2, GPLOT_PNG, "/tmp/lept/numa2/lyra8", "Mean Square");
    gplotSimple1(na3, GPLOT_PNG, "/tmp/lept/numa2/lyra9", "Variance");
    gplotSimple1(na4, GPLOT_PNG, "/tmp/lept/numa2/lyra10", "RMS Difference");
    pixa = pixaCreate(5);
    pix1 = pixRead("/tmp/lept/numa2/lyra6.png");
    pix2 = pixRead("/tmp/lept/numa2/lyra7.png");
    pix3 = pixRead("/tmp/lept/numa2/lyra8.png");
    pix4 = pixRead("/tmp/lept/numa2/lyra9.png");
    pix5 = pixRead("/tmp/lept/numa2/lyra10.png");
    pixaAddPix(pixa, pix1, L_INSERT);
    pixaAddPix(pixa, pix2, L_INSERT);
    pixaAddPix(pixa, pix3, L_INSERT);
    pixaAddPix(pixa, pix4, L_INSERT);
    pixaAddPix(pixa, pix5, L_INSERT);
    pixd = pixaDisplayTiledInRows(pixa, 32, 1500, 1.0, 0, 20, 2);
    pixDisplay(pixd, 100, 0);
    pixWrite("/tmp/lept/numa2/window.png", pixd, IFF_PNG);
    pixDestroy(&pixd);
    pixaDestroy(&pixa);
    numaDestroy(&na);
    numaDestroy(&na1);
    numaDestroy(&na2);
    numaDestroy(&na3);
    numaDestroy(&na4);
#endif

    /* -------------------------------------------------------------------*
     *                        Extraction on a line                        *
     * -------------------------------------------------------------------*/
#if  DO_ALL
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
    pixWrite("/tmp/lept/numa_pixg.png", pixg, IFF_PNG);
    pixDisplay(pixg, 450, 100);

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
    pixa = pixaCreate(4);
    pix1 = pixRead("/tmp/lept/numa2/ext1.png");
    pix2 = pixRead("/tmp/lept/numa2/ext2.png");
    pix3 = pixRead("/tmp/lept/numa2/ext3.png");
    pix4 = pixRead("/tmp/lept/numa2/ext4.png");
    pixaAddPix(pixa, pix1, L_INSERT);
    pixaAddPix(pixa, pix2, L_INSERT);
    pixaAddPix(pixa, pix3, L_INSERT);
    pixaAddPix(pixa, pix4, L_INSERT);
    pixd = pixaDisplayTiledInRows(pixa, 32, 1500, 1.0, 0, 20, 2);
    pixDisplay(pixd, 100, 450);
    pixWrite("/tmp/lept/numa2/extract.png", pixd, IFF_PNG);
    pixDestroy(&pixd);
    pixaDestroy(&pixa);
    pixDestroy(&pixg);
    numaDestroy(&na1);
    numaDestroy(&na2);
    numaDestroy(&na3);
    numaDestroy(&na4);
#endif

    /* -------------------------------------------------------------------*
     *                     Row and column pixel sums                      *
     * -------------------------------------------------------------------*/
#if  DO_ALL
        /* Sum by columns in two halves (left and right) */
    pixs = pixRead("test8.jpg");
    pixGetDimensions(pixs, &w, &h, NULL);
    box1 = boxCreate(0, 0, w / 2, h);
    box2 = boxCreate(w / 2, 0, w - 2 / 2, h);
    na1 = pixAverageByColumn(pixs, box1, L_BLACK_IS_MAX);
    na2 = pixAverageByColumn(pixs, box2, L_BLACK_IS_MAX);
    numaJoin(na1, na2, 0, -1);
    na3 = pixAverageByColumn(pixs, NULL, L_BLACK_IS_MAX);
    numaSimilar(na1, na3, 0.0, &same);
    if (same)
        fprintf(stderr, "Same for columns\n");
    else
        fprintf(stderr, "Error for columns\n");
    pix = pixConvertTo32(pixs);
    pixRenderPlotFromNumaGen(&pix, na3, L_HORIZONTAL_LINE, 3, h / 2, 80, 1,
                             0xff000000);
    pixRenderPlotFromNuma(&pix, na3, L_PLOT_AT_BOT, 3, 80, 0xff000000);
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
    numaSimilar(na1, na3, 0.0, &same);
    if (same)
        fprintf(stderr, "Same for rows\n");
    else
        fprintf(stderr, "Error for rows\n");
    pixRenderPlotFromNumaGen(&pix, na3, L_VERTICAL_LINE, 3, w / 2, 80, 1,
                             0x00ff0000);
    pixRenderPlotFromNuma(&pix, na3, L_PLOT_AT_RIGHT, 3, 80, 0x00ff0000);
    pixDisplay(pix, 500, 200);
    boxDestroy(&box1);
    boxDestroy(&box2);
    numaDestroy(&na1);
    numaDestroy(&na2);
    numaDestroy(&na3);
    pixDestroy(&pix);

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
    fprintf(stderr, "ave1 = %8.4f\n", sum1 / h);
    fprintf(stderr, "ave2 = %8.4f\n", 2.0 * sum2 / w);
    pixAverageInRect(pixs, NULL, &ave4);  /* entire image */
    diff1 = ave4 - ave3;
    diff2 = w * h * ave4 - (0.5 * w * sum1 + h * sum2);
    if (diff1 < 0.001)
        fprintf(stderr, "Average diffs are correct\n");
    else
        fprintf(stderr, "Average diffs are wrong: diff1 = %7.5f\n", diff1);
    if (diff2 < 20)  /* float-to-integer roundoff */
        fprintf(stderr, "Pixel sums are correct\n");
    else
        fprintf(stderr, "Pixel sums are in error: diff = %7.0f\n", diff2);

        /* Variance left and right halves.  Variance doesn't average
         * in a simple way, unlike pixel sums. */
    pixVarianceInRect(pixs, box1, &var1);  /* entire image */
    pixVarianceInRect(pixs, box2, &var2);  /* entire image */
    pixVarianceInRect(pixs, NULL, &var3);  /* entire image */
    fprintf(stderr, "0.5 * (var1 + var2) = %7.3f, var3 = %7.3f\n",
            0.5 * (var1 + var2), var3);
    boxDestroy(&box1);
    boxDestroy(&box2);
    numaDestroy(&na1);
    numaDestroy(&na2);
#endif

    /* -------------------------------------------------------------------*
     *                     Row and column variances                       *
     * -------------------------------------------------------------------*/
#if  DO_ALL

        /* Display variance by rows and columns */
    box1 = boxCreate(415, 0, 130, 425);
    boxGetGeometry(box1, NULL, NULL, &bw, &bh);
    na1 = pixVarianceByRow(pixs, box1);
    na2 = pixVarianceByColumn(pixs, box1);
    pix = pixConvertTo32(pixs);
    pix1 = pixCopy(NULL, pix);
    pixRenderPlotFromNumaGen(&pix, na1, L_VERTICAL_LINE, 3, 415, 100, 1,
                             0xff000000);
    pixRenderPlotFromNumaGen(&pix, na2, L_HORIZONTAL_LINE, 3, bh / 2, 100, 1,
                          0x00ff0000);
    pixRenderPlotFromNuma(&pix1, na1, L_PLOT_AT_LEFT, 3, 60, 0x00ff0000);
    pixRenderPlotFromNuma(&pix1, na1, L_PLOT_AT_MID_VERT, 3, 60, 0x0000ff00);
    pixRenderPlotFromNuma(&pix1, na1, L_PLOT_AT_RIGHT, 3, 60, 0xff000000);
    pixRenderPlotFromNuma(&pix1, na2, L_PLOT_AT_TOP, 3, 60, 0x0000ff00);
    pixRenderPlotFromNuma(&pix1, na2, L_PLOT_AT_MID_HORIZ, 3, 60, 0xff000000);
    pixRenderPlotFromNuma(&pix1, na2, L_PLOT_AT_BOT, 3, 60, 0x00ff0000);
    pixDisplay(pix, 500, 900);
    pixDisplay(pix1, 500, 1000);
    boxDestroy(&box1);
    numaDestroy(&na1);
    numaDestroy(&na2);
    pixDestroy(&pix);
    pixDestroy(&pix1);
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
    pixDisplay(pix3, 1000, 0);
    numaDestroy(&na1);
    numaDestroy(&na2);
    pixDestroy(&pix3);

        /* Again, with an erosion */
    pix3 = pixErodeGray(pix2, 3, 21);
    pixDisplay(pix3, 1400, 0);
    na1 = pixVarianceByRow(pix3, NULL);
    pix4 = pixConvertTo32(pix1);
    pixRenderPlotFromNumaGen(&pix4, na1, L_VERTICAL_LINE, 3, 30, 70, 1,
                             0xff000000);
    na2 = pixVarianceByColumn(pix3, NULL);
    pixRenderPlotFromNumaGen(&pix4, na2, L_HORIZONTAL_LINE, 3, bh - 1, 70, 1,
                             0x00ff0000);
    pixDisplay(pix4, 1000, 550);
    numaDestroy(&na1);
    numaDestroy(&na2);
    pixDestroy(&pix1);
    pixDestroy(&pix2);
    pixDestroy(&pix3);
    pixDestroy(&pix4);
#endif

    /* -------------------------------------------------------------------*
     *                    Windowed variance along a line                  *
     * -------------------------------------------------------------------*/
#if  DO_ALL
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
    pixDisplay(pix1, 1000, 1000);
    pixDisplay(pix3, 1500, 1000);
    pixDestroy(&pix1);
    pixDestroy(&pix2);
    pixDestroy(&pix3);
    numaDestroy(&na1);
    numaDestroy(&na2);
#endif
    return 0;
}
