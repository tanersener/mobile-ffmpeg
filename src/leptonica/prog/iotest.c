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
 * iotest.c
 *
 *   Tests several I/O operations, including the special operations
 *   for handling 16 bpp png input, zlib compression quality in png,
 *   chroma sampling options in jpeg, and read/write of alpha with png.
 *
 *   This does not testt multipage/custom tiff and PostScript, which
 *   are separately tested in mtifftest and psiotest, respectively.
 */

#include "string.h"
#ifndef  _WIN32
#include <unistd.h>
#else
#include <windows.h>   /* for Sleep() */
#endif  /* _WIN32 */
#include "allheaders.h"

LEPT_DLL extern const char *ImageFileFormatExtensions[];

int main(int    argc,
         char **argv)
{
char        *text;
l_int32      w, h, d, level, wpl, format, xres, yres;
l_int32      bps, spp, res, iscmap;
size_t       size;
FILE        *fp;
PIX         *pixs, *pixg, *pix1, *pix2, *pix3, *pix4;
PIXA        *pixa;
PIXCMAP     *cmap;
static char  mainName[] = "iotest";

    if (argc != 1)
        return ERROR_INT(" Syntax: iotest", mainName, 1);

    setLeptDebugOK(1);
    lept_mkdir("lept/io");

        /* Test 16 to 8 stripping */
    pixs = pixRead("test16.tif");
    pixWrite("/tmp/lept/io/test16.png", pixs, IFF_PNG);
    pix1 = pixRead("/tmp/lept/io/test16.png");
    if ((d = pixGetDepth(pix1)) != 8)
        fprintf(stderr, "Error: d = %d; should be 8", d);
    pixDestroy(&pix1);
    l_pngSetReadStrip16To8(0);
    pix1 = pixRead("/tmp/lept/io/test16.png");
    if ((d = pixGetDepth(pix1)) != 16)
        fprintf(stderr, "Error: d = %d; should be 16", d);
    pixDestroy(&pix1);
    pixDestroy(&pixs);

        /* Test zlib compression in png */
    pixs = pixRead("feyn.tif");
    for (level = 0; level < 10; level++) {
        pixSetZlibCompression(pixs, level);
        pixWrite("/tmp/lept/io/zlibtest.png", pixs, IFF_PNG);
        size = nbytesInFile("/tmp/lept/io/zlibtest.png");
        fprintf(stderr, "zlib level = %d, file size = %ld\n",
                level, (unsigned long)size);
    }
    pixDestroy(&pixs);

        /* Test chroma sampling options in jpeg */
    pixs = pixRead("marge.jpg");
    pixWrite("/tmp/lept/io/chromatest.jpg", pixs, IFF_JFIF_JPEG);
    size = nbytesInFile("/tmp/lept/io/chromatest.jpg");
    fprintf(stderr, "chroma default: file size = %ld\n", (unsigned long)size);
    pixSetChromaSampling(pixs, 0);
    pixWrite("/tmp/lept/io/chromatest.jpg", pixs, IFF_JFIF_JPEG);
    size = nbytesInFile("/tmp/lept/io/chromatest.jpg");
    fprintf(stderr, "no ch. sampling: file size = %ld\n", (unsigned long)size);
    pixSetChromaSampling(pixs, 1);
    pixWrite("/tmp/lept/io/chromatest.jpg", pixs, IFF_JFIF_JPEG);
    size = nbytesInFile("/tmp/lept/io/chromatest.jpg");
    fprintf(stderr, "chroma sampling: file size = %ld\n", (unsigned long)size);
    pixDestroy(&pixs);

        /* Test read/write of alpha with png */
    pixs = pixRead("books_logo.png");
    pixDisplay(pixs, 0, 100);
    pixg = pixGetRGBComponent(pixs, L_ALPHA_CHANNEL);
    pixDisplay(pixg, 300, 100);
    pixDestroy(&pixg);
    pix1 = pixAlphaBlendUniform(pixs, 0xffffff00);  /* render rgb over white */
    pixWrite("/tmp/lept/io/logo1.png", pix1, IFF_PNG);
    pixDisplay(pix1, 0, 250);
    pix2 = pixSetAlphaOverWhite(pix1);  /* regenerate alpha from white */
    pixDisplay(pix2, 0, 400);
    pixWrite("/tmp/lept/io/logo2.png", pix2, IFF_PNG);
    pixg = pixGetRGBComponent(pix2, L_ALPHA_CHANNEL);
    pixDisplay(pixg, 300, 400);
    pixDestroy(&pixg);
    pix3 = pixRead("/tmp/lept/io/logo2.png");
    pix4 = pixAlphaBlendUniform(pix3, 0x00ffff00);  /* render rgb over cyan */
    pixWrite("/tmp/lept/io/logo3.png", pix3, IFF_PNG);
    pixDisplay(pix3, 0, 550);
    pixDestroy(&pix1);
    pixDestroy(&pix2);
    pixDestroy(&pix3);
    pixDestroy(&pix4);
    pixDestroy(&pixs);

        /* A little fun with rgb colormaps */
    pixs = pixRead("weasel4.11c.png");
    pixa = pixaCreate(6);
    pixaAddPix(pixa, pixs, L_CLONE);
    pixGetDimensions(pixs, &w, &h, &d);
    wpl = pixGetWpl(pixs);
    fprintf(stderr, "w = %d, h = %d, d = %d, wpl = %d\n", w, h, d, wpl);
    xres = pixGetXRes(pixs);
    yres = pixGetXRes(pixs);
    if (xres != 0 && yres != 0)
        fprintf(stderr, "xres = %d, yres = %d\n", xres, yres);
    cmap = pixGetColormap(pixs);
        /* Write and read back the colormap */
    pixcmapWriteStream(stderr, pixGetColormap(pixs));
    fp = lept_fopen("/tmp/lept/io/cmap1", "wb");
    pixcmapWriteStream(fp, pixGetColormap(pixs));
    lept_fclose(fp);
    fp = lept_fopen("/tmp/lept/io/cmap1", "rb");
    cmap = pixcmapReadStream(fp);
    lept_fclose(fp);
    fp = lept_fopen("/tmp/lept/io/cmap2", "wb");
    pixcmapWriteStream(fp, cmap);
    lept_fclose(fp);
    pixcmapDestroy(&cmap);
        /* Remove and regenerate colormap */
    pix1 = pixRemoveColormap(pixs, REMOVE_CMAP_BASED_ON_SRC);
    pixaAddPix(pixa, pix1, L_CLONE);
    pixWrite("/tmp/lept/io/weaselrgb.png", pix1, IFF_PNG);
    pix2 = pixConvertRGBToColormap(pix1, 1);
    pixaAddPix(pixa, pix2, L_CLONE);
    pixWrite("/tmp/lept/io/weaselcmap.png", pix2, IFF_PNG);
    pixDestroy(&pixs);
    pixDestroy(&pix1);
    pixDestroy(&pix2);
       /* Remove and regnerate gray colormap */
    pixs = pixRead("weasel4.5g.png");
    pixaAddPix(pixa, pixs, L_CLONE);
    pix1 = pixRemoveColormap(pixs, REMOVE_CMAP_BASED_ON_SRC);
    pixaAddPix(pixa, pix1, L_CLONE);
    pixWrite("/tmp/lept/io/weaselgray.png", pix1, IFF_PNG);
    pix2 = pixConvertGrayToColormap(pix1);
    pixaAddPix(pixa, pix2, L_CLONE);
    pixWrite("/tmp/lept/io/weaselcmap2.png", pix2, IFF_PNG);
    pixDestroy(&pix1);
    pixDestroy(&pix2);
    pix3 = pixaDisplayTiled(pixa, 400, 0, 20);
    pixDisplay(pix3, 0, 750);
    pixDestroy(&pix3);
    pixaDestroy(&pixa);

        /* Other fields in the pix */
    format = pixGetInputFormat(pixs);
    if (format != UNDEF)
        fprintf(stderr, "Input format extension: %s\n",
                ImageFileFormatExtensions[format]);
    pixSetText(pixs, "reconstituted 4-bit weasel");
    text = pixGetText(pixs);
    if (text && strlen(text) != 0)
        fprintf(stderr, "Text: %s\n", text);
    pixDestroy(&pixs);

#ifndef  _WIN32
    sleep(1);
#else
    Sleep(1000);
#endif  /* _WIN32 */

        /* Some tiff compression and headers */
    readHeaderTiff("feyn-fract.tif", 0, &w, &h, &bps, &spp,
                   &res, &iscmap, &format);
    fprintf(stderr, "w = %d, h = %d, bps = %d, spp = %d, res = %d, cmap = %d\n",
            w, h, bps, spp, res, iscmap);
    fprintf(stderr, "Input format extension: %s\n",
            ImageFileFormatExtensions[format]);
    pixs = pixRead("feyn-fract.tif");
    pixWrite("/tmp/lept/io/fract1.tif", pixs, IFF_TIFF);
    size = nbytesInFile("/tmp/lept/io/fract1.tif");
    fprintf(stderr, "uncompressed: %ld\n", (unsigned long)size);
    pixWrite("/tmp/lept/io/fract2.tif", pixs, IFF_TIFF_PACKBITS);
    size = nbytesInFile("/tmp/lept/io/fract2.tif");
    fprintf(stderr, "packbits: %ld\n", (unsigned long)size);
    pixWrite("/tmp/lept/io/fract3.tif", pixs, IFF_TIFF_RLE);
    size = nbytesInFile("/tmp/lept/io/fract3.tif");
    fprintf(stderr, "rle: %ld\n", (unsigned long)size);
    pixWrite("/tmp/lept/io/fract4.tif", pixs, IFF_TIFF_G3);
    size = nbytesInFile("/tmp/lept/io/fract4.tif");
    fprintf(stderr, "g3: %ld\n", (unsigned long)size);
    pixWrite("/tmp/lept/io/fract5.tif", pixs, IFF_TIFF_G4);
    size = nbytesInFile("/tmp/lept/io/fract5.tif");
    fprintf(stderr, "g4: %ld\n", (unsigned long)size);
    pixWrite("/tmp/lept/io/fract6.tif", pixs, IFF_TIFF_LZW);
    size = nbytesInFile("/tmp/lept/io/fract6.tif");
    fprintf(stderr, "lzw: %ld\n", (unsigned long)size);
    pixWrite("/tmp/lept/io/fract7.tif", pixs, IFF_TIFF_ZIP);
    size = nbytesInFile("/tmp/lept/io/fract7.tif");
    fprintf(stderr, "zip: %ld\n", (unsigned long)size);
    pixDestroy(&pixs);

    return 0;
}
