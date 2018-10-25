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
 * flipdetect_reg.c
 *
 *   flipdetect_reg [filein]
 *
 *   - Tests the high-level interface
 *   - Tests 90 degree orientation of text and whether the text is
 *     mirror reversed.
 *   - Compares the rasterop with dwa implementations for speed.
 *   - Shows the typical 'confidence' outputs from functions in flipdetect.c.
 */

#include "allheaders.h"

static void printStarredMessage(const char *msg);

int main(int    argc,
         char **argv)
{
const char  *filein;
l_int32      i, orient, rotation, same;
l_float32    upconf1, upconf2, leftconf1, leftconf2, conf1, conf2;
PIX         *pixs, *pix1, *pix2;
static char  mainName[] = "flipdetect_reg";

    if (argc != 1 && argc != 2)
        return ERROR_INT(" Syntax: flipdetect_reg [filein]", mainName, 1);
    filein = (argc == 1) ? "feyn.tif" : argv[1];
    setLeptDebugOK(1);

    if ((pix1 = pixRead(filein)) == NULL)
        return ERROR_INT("pix1 not made", mainName, 1);
    pixs = pixConvertTo1(pix1, 130);
    pixDestroy(&pix1);

        /* Test high-level interface */
    fprintf(stderr, "\nTest high-level detection/rotation\n");
    pix1 = pixRotateOrth(pixs, 3);
    pix2 = pixOrientCorrect(pix1, 0.0, 0.0, &upconf1, &leftconf1,
                            &rotation, 0);
    fprintf(stderr, "upconf = %7.3f, leftconf = %7.3f, rotation = %d\n",
            upconf1, leftconf1, rotation);
    pixEqual(pixs, pix2, &same);
    if (!same)
        fprintf(stderr, "Error: image not rotated back correctly!\n");
    pixDestroy(&pix1);
    pixDestroy(&pix2);

        /* Compare rasterop and dwa orientation detection */
    fprintf(stderr, "\nTest orientation detection\n");
    startTimer();
    pixOrientDetect(pixs, &upconf1, &leftconf1, 0, 0);
    fprintf(stderr, "Time for rop orient test: %7.3f sec\n", stopTimer());

    startTimer();
    pixOrientDetectDwa(pixs, &upconf2, &leftconf2, 0, 0);
    fprintf(stderr, "Time for dwa orient test: %7.3f sec\n", stopTimer());

    if (upconf1 == upconf2 && leftconf1 == leftconf2) {
        printStarredMessage("Orient results identical");
        fprintf(stderr, "upconf = %7.3f, leftconf = %7.3f\n",
                upconf1, leftconf1);
    }
    else {
        printStarredMessage("Orient results differ");
        fprintf(stderr, "upconf1 = %7.3f, upconf2 = %7.3f\n", upconf1, upconf2);
        fprintf(stderr, "leftconf1 = %7.3f, leftconf2 = %7.3f\n",
                leftconf1, leftconf2);
    }

    makeOrientDecision(upconf1, leftconf1, 0, 0, &orient, 1);
    fprintf(stderr, "Orientation (enum) found: %d\n", orient);

    pix1 = pixCopy(NULL, pixs);
    fprintf(stderr, "\nTest orient detection for 4 orientations\n");
    for (i = 0; i < 4; i++) {
        pixOrientDetectDwa(pix1, &upconf2, &leftconf2, 0, 0);
        makeOrientDecision(upconf2, leftconf2, 0, 0, &orient, 1);
        if (i == 3) break;
        pix2 = pixRotate90(pix1, 1);
        pixDestroy(&pix1);
        pix1 = pix2;
    }
    pixDestroy(&pix1);

        /* Compare rasterop and dwa mirror flip detection */
    fprintf(stderr, "\nTest mirror reverse detection\n");
    startTimer();
    pixMirrorDetect(pixs, &conf1, 0, 1);
    fprintf(stderr, "Time for rop mirror flip test: %7.3f sec\n", stopTimer());

    startTimer();
    pixMirrorDetectDwa(pixs, &conf2, 0, 0);
    fprintf(stderr, "Time for dwa mirror flip test: %7.3f sec\n", stopTimer());

    if (conf1 == conf2) {
        printStarredMessage("Mirror results identical");
        fprintf(stderr, "conf = %7.3f\n", conf1);
    }
    else {
        printStarredMessage("Mirror results differ");
        fprintf(stderr, "conf1 = %7.3f, conf2 = %7.3f\n", conf1, conf2);
    }

        /* Compare safer rasterop and dwa orientation detection */
    fprintf(stderr, "\nTest safer version of up-down tests\n");
    pixUpDownDetectGeneral(pixs, &conf1, 0, 10, 1);
    pixUpDownDetectGeneralDwa(pixs, &conf2, 0, 10, 1);
    if (conf1 == conf2)
        fprintf(stderr, "Confidence results are identical\n");
    else
        fprintf(stderr, "Confidence results differ\n");

    pixDestroy(&pixs);
    return 0;
}


void
printStarredMessage(const char *msg)
{
    fprintf(stderr, "****************************************************\n");
    fprintf(stderr, "***********   %s   ***********\n", msg);
    fprintf(stderr, "****************************************************\n");
    return;
}

