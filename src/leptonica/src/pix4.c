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

/*!
 * \file pix4.c
 * <pre>
 *
 *    This file has these operations:
 *
 *      (1) Pixel histograms
 *      (2) Pixel row/column statistics
 *      (3) Foreground/background estimation
 *
 *    Pixel histogram, rank val, averaging and min/max
 *           NUMA       *pixGetGrayHistogram()
 *           NUMA       *pixGetGrayHistogramMasked()
 *           NUMA       *pixGetGrayHistogramInRect()
 *           NUMAA      *pixGetGrayHistogramTiled()
 *           l_int32     pixGetColorHistogram()
 *           l_int32     pixGetColorHistogramMasked()
 *           NUMA       *pixGetCmapHistogram()
 *           NUMA       *pixGetCmapHistogramMasked()
 *           NUMA       *pixGetCmapHistogramInRect()
 *           l_int32     pixCountRGBColors()
 *           L_AMAP     *pixGetColorAmapHistogram()
 *           l_int32     amapGetCountForColor()
 *           l_int32     pixGetRankValue()
 *           l_int32     pixGetRankValueMaskedRGB()
 *           l_int32     pixGetRankValueMasked()
 *           l_int32     pixGetPixelAverage()
 *           l_int32     pixGetPixelStats()
 *           l_int32     pixGetAverageMaskedRGB()
 *           l_int32     pixGetAverageMasked()
 *           l_int32     pixGetAverageTiledRGB()
 *           PIX        *pixGetAverageTiled()
 *           NUMA       *pixRowStats()
 *           NUMA       *pixColumnStats()
 *           l_int32     pixGetRangeValues()
 *           l_int32     pixGetExtremeValue()
 *           l_int32     pixGetMaxValueInRect()
 *           l_int32     pixGetBinnedComponentRange()
 *           l_int32     pixGetRankColorArray()
 *           l_int32     pixGetBinnedColor()
 *           PIX        *pixDisplayColorArray()
 *           PIX        *pixRankBinByStrip()
 *
 *    Pixelwise aligned statistics
 *           PIX        *pixaGetAlignedStats()
 *           l_int32     pixaExtractColumnFromEachPix()
 *           l_int32     pixGetRowStats()
 *           l_int32     pixGetColumnStats()
 *           l_int32     pixSetPixelColumn()
 *
 *    Foreground/background estimation
 *           l_int32     pixThresholdForFgBg()
 *           l_int32     pixSplitDistributionFgBg()
 * </pre>
 */

#include <string.h>
#include <math.h>
#include "allheaders.h"


/*------------------------------------------------------------------*
 *                  Pixel histogram and averaging                   *
 *------------------------------------------------------------------*/
/*!
 * \brief   pixGetGrayHistogram()
 *
 * \param[in]   pixs     1, 2, 4, 8, 16 bpp; can be colormapped
 * \param[in]   factor   subsampling factor; integer >= 1
 * \return  na histogram, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) If pixs has a colormap, it is converted to 8 bpp gray.
 *          If you want a histogram of the colormap indices, use
 *          pixGetCmapHistogram().
 *      (2) If pixs does not have a colormap, the output histogram is
 *          of size 2^d, where d is the depth of pixs.
 *      (3) Set the subsampling factor > 1 to reduce the amount of computation.
 * </pre>
 */
NUMA *
pixGetGrayHistogram(PIX     *pixs,
                    l_int32  factor)
{
l_int32     i, j, w, h, d, wpl, val, size, count;
l_uint32   *data, *line;
l_float32  *array;
NUMA       *na;
PIX        *pixg;

    PROCNAME("pixGetGrayHistogram");

    if (!pixs)
        return (NUMA *)ERROR_PTR("pixs not defined", procName, NULL);
    d = pixGetDepth(pixs);
    if (d > 16)
        return (NUMA *)ERROR_PTR("depth not in {1,2,4,8,16}", procName, NULL);
    if (factor < 1)
        return (NUMA *)ERROR_PTR("sampling must be >= 1", procName, NULL);

    if (pixGetColormap(pixs))
        pixg = pixRemoveColormap(pixs, REMOVE_CMAP_TO_GRAYSCALE);
    else
        pixg = pixClone(pixs);

    pixGetDimensions(pixg, &w, &h, &d);
    size = 1 << d;
    if ((na = numaCreate(size)) == NULL) {
        pixDestroy(&pixg);
        return (NUMA *)ERROR_PTR("na not made", procName, NULL);
    }
    numaSetCount(na, size);  /* all initialized to 0.0 */
    array = numaGetFArray(na, L_NOCOPY);

    if (d == 1) {  /* special case */
        pixCountPixels(pixg, &count, NULL);
        array[0] = w * h - count;
        array[1] = count;
        pixDestroy(&pixg);
        return na;
    }

    wpl = pixGetWpl(pixg);
    data = pixGetData(pixg);
    for (i = 0; i < h; i += factor) {
        line = data + i * wpl;
        if (d == 2) {
            for (j = 0; j < w; j += factor) {
                val = GET_DATA_DIBIT(line, j);
                array[val] += 1.0;
            }
        } else if (d == 4) {
            for (j = 0; j < w; j += factor) {
                val = GET_DATA_QBIT(line, j);
                array[val] += 1.0;
            }
        } else if (d == 8) {
            for (j = 0; j < w; j += factor) {
                val = GET_DATA_BYTE(line, j);
                array[val] += 1.0;
            }
        } else {  /* d == 16 */
            for (j = 0; j < w; j += factor) {
                val = GET_DATA_TWO_BYTES(line, j);
                array[val] += 1.0;
            }
        }
    }

    pixDestroy(&pixg);
    return na;
}


/*!
 * \brief   pixGetGrayHistogramMasked()
 *
 * \param[in]   pixs     8 bpp, or colormapped
 * \param[in]   pixm     [optional] 1 bpp mask over which histogram is
 *                       to be computed; use all pixels if null
 * \param[in]   x, y     UL corner of pixm relative to the UL corner of pixs;
 *                       can be < 0; these values are ignored if pixm is null
 * \param[in]   factor   subsampling factor; integer >= 1
 * \return  na histogram, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) If pixs is cmapped, it is converted to 8 bpp gray.
 *          If you want a histogram of the colormap indices, use
 *          pixGetCmapHistogramMasked().
 *      (2) This always returns a 256-value histogram of pixel values.
 *      (3) Set the subsampling factor > 1 to reduce the amount of computation.
 *      (4) Clipping of pixm (if it exists) to pixs is done in the inner loop.
 *      (5) Input x,y are ignored unless pixm exists.
 * </pre>
 */
NUMA *
pixGetGrayHistogramMasked(PIX        *pixs,
                          PIX        *pixm,
                          l_int32     x,
                          l_int32     y,
                          l_int32     factor)
{
l_int32     i, j, w, h, wm, hm, dm, wplg, wplm, val;
l_uint32   *datag, *datam, *lineg, *linem;
l_float32  *array;
NUMA       *na;
PIX        *pixg;

    PROCNAME("pixGetGrayHistogramMasked");

    if (!pixm)
        return pixGetGrayHistogram(pixs, factor);
    if (!pixs)
        return (NUMA *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 8 && !pixGetColormap(pixs))
        return (NUMA *)ERROR_PTR("pixs neither 8 bpp nor colormapped",
                                 procName, NULL);
    pixGetDimensions(pixm, &wm, &hm, &dm);
    if (dm != 1)
        return (NUMA *)ERROR_PTR("pixm not 1 bpp", procName, NULL);
    if (factor < 1)
        return (NUMA *)ERROR_PTR("sampling must be >= 1", procName, NULL);

    if ((na = numaCreate(256)) == NULL)
        return (NUMA *)ERROR_PTR("na not made", procName, NULL);
    numaSetCount(na, 256);  /* all initialized to 0.0 */
    array = numaGetFArray(na, L_NOCOPY);

    if (pixGetColormap(pixs))
        pixg = pixRemoveColormap(pixs, REMOVE_CMAP_TO_GRAYSCALE);
    else
        pixg = pixClone(pixs);
    pixGetDimensions(pixg, &w, &h, NULL);
    datag = pixGetData(pixg);
    wplg = pixGetWpl(pixg);
    datam = pixGetData(pixm);
    wplm = pixGetWpl(pixm);

        /* Generate the histogram */
    for (i = 0; i < hm; i += factor) {
        if (y + i < 0 || y + i >= h) continue;
        lineg = datag + (y + i) * wplg;
        linem = datam + i * wplm;
        for (j = 0; j < wm; j += factor) {
            if (x + j < 0 || x + j >= w) continue;
            if (GET_DATA_BIT(linem, j)) {
                val = GET_DATA_BYTE(lineg, x + j);
                array[val] += 1.0;
            }
        }
    }

    pixDestroy(&pixg);
    return na;
}


/*!
 * \brief   pixGetGrayHistogramInRect()
 *
 * \param[in]   pixs    8 bpp, or colormapped
 * \param[in]   box     [optional] over which histogram is to be computed;
 *                      use full image if NULL
 * \param[in]   factor  subsampling factor; integer >= 1
 * \return  na histogram, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) If pixs is cmapped, it is converted to 8 bpp gray.
 *          If you want a histogram of the colormap indices, use
 *          pixGetCmapHistogramInRect().
 *      (2) This always returns a 256-value histogram of pixel values.
 *      (3) Set the subsampling %factor > 1 to reduce the amount of computation.
 * </pre>
 */
NUMA *
pixGetGrayHistogramInRect(PIX     *pixs,
                          BOX     *box,
                          l_int32  factor)
{
l_int32     i, j, bx, by, bw, bh, w, h, wplg, val;
l_uint32   *datag, *lineg;
l_float32  *array;
NUMA       *na;
PIX        *pixg;

    PROCNAME("pixGetGrayHistogramInRect");

    if (!box)
        return pixGetGrayHistogram(pixs, factor);
    if (!pixs)
        return (NUMA *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 8 && !pixGetColormap(pixs))
        return (NUMA *)ERROR_PTR("pixs neither 8 bpp nor colormapped",
                                 procName, NULL);
    if (factor < 1)
        return (NUMA *)ERROR_PTR("sampling must be >= 1", procName, NULL);

    if ((na = numaCreate(256)) == NULL)
        return (NUMA *)ERROR_PTR("na not made", procName, NULL);
    numaSetCount(na, 256);  /* all initialized to 0.0 */
    array = numaGetFArray(na, L_NOCOPY);

    if (pixGetColormap(pixs))
        pixg = pixRemoveColormap(pixs, REMOVE_CMAP_TO_GRAYSCALE);
    else
        pixg = pixClone(pixs);
    pixGetDimensions(pixg, &w, &h, NULL);
    datag = pixGetData(pixg);
    wplg = pixGetWpl(pixg);
    boxGetGeometry(box, &bx, &by, &bw, &bh);

        /* Generate the histogram */
    for (i = 0; i < bh; i += factor) {
        if (by + i < 0 || by + i >= h) continue;
        lineg = datag + (by + i) * wplg;
        for (j = 0; j < bw; j += factor) {
            if (bx + j < 0 || bx + j >= w) continue;
            val = GET_DATA_BYTE(lineg, bx + j);
            array[val] += 1.0;
        }
    }

    pixDestroy(&pixg);
    return na;
}


/*!
 * \brief   pixGetGrayHistogramTiled()
 *
 * \param[in]   pixs     any depth, colormap OK
 * \param[in]   factor   subsampling factor; integer >= 1
 * \param[in]   nx, ny   tiling; >= 1; typically small
 * \return  naa set of histograms, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) If pixs is cmapped, it is converted to 8 bpp gray.
 *      (2) This returns a set of 256-value histograms of pixel values.
 *      (3) Set the subsampling factor > 1 to reduce the amount of computation.
 * </pre>
 */
NUMAA *
pixGetGrayHistogramTiled(PIX     *pixs,
                         l_int32  factor,
                         l_int32  nx,
                         l_int32  ny)
{
l_int32  i, n;
NUMA    *na;
NUMAA   *naa;
PIX     *pix1, *pix2;
PIXA    *pixa;

    PROCNAME("pixGetGrayHistogramTiled");

    if (!pixs)
        return (NUMAA *)ERROR_PTR("pixs not defined", procName, NULL);
    if (factor < 1)
        return (NUMAA *)ERROR_PTR("sampling must be >= 1", procName, NULL);
    if (nx < 1 || ny < 1)
        return (NUMAA *)ERROR_PTR("nx and ny must both be > 0", procName, NULL);

    n = nx * ny;
    if ((naa = numaaCreate(n)) == NULL)
        return (NUMAA *)ERROR_PTR("naa not made", procName, NULL);

    pix1 = pixConvertTo8(pixs, FALSE);
    pixa = pixaSplitPix(pix1, nx, ny, 0, 0);
    for (i = 0; i < n; i++) {
        pix2 = pixaGetPix(pixa, i, L_CLONE);
        na = pixGetGrayHistogram(pix2, factor);
        numaaAddNuma(naa, na, L_INSERT);
        pixDestroy(&pix2);
    }

    pixDestroy(&pix1);
    pixaDestroy(&pixa);
    return naa;
}


/*!
 * \brief   pixGetColorHistogram()
 *
 * \param[in]    pixs     rgb or colormapped
 * \param[in]    factor   subsampling factor; integer >= 1
 * \param[out]   pnar     red histogram
 * \param[out]   pnag     green histogram
 * \param[out]   pnab     blue histogram
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) This generates a set of three 256 entry histograms,
 *          one for each color component (r,g,b).
 *      (2) Set the subsampling %factor > 1 to reduce the amount of computation.
 * </pre>
 */
l_ok
pixGetColorHistogram(PIX     *pixs,
                     l_int32  factor,
                     NUMA   **pnar,
                     NUMA   **pnag,
                     NUMA   **pnab)
{
l_int32     i, j, w, h, d, wpl, index, rval, gval, bval;
l_uint32   *data, *line;
l_float32  *rarray, *garray, *barray;
NUMA       *nar, *nag, *nab;
PIXCMAP    *cmap;

    PROCNAME("pixGetColorHistogram");

    if (pnar) *pnar = NULL;
    if (pnag) *pnag = NULL;
    if (pnab) *pnab = NULL;
    if (!pnar || !pnag || !pnab)
        return ERROR_INT("&nar, &nag, &nab not all defined", procName, 1);
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    pixGetDimensions(pixs, &w, &h, &d);
    cmap = pixGetColormap(pixs);
    if (cmap && (d != 2 && d != 4 && d != 8))
        return ERROR_INT("colormap and not 2, 4, or 8 bpp", procName, 1);
    if (!cmap && d != 32)
        return ERROR_INT("no colormap and not rgb", procName, 1);
    if (factor < 1)
        return ERROR_INT("sampling factor must be >= 1", procName, 1);

        /* Set up the histogram arrays */
    nar = numaCreate(256);
    nag = numaCreate(256);
    nab = numaCreate(256);
    numaSetCount(nar, 256);
    numaSetCount(nag, 256);
    numaSetCount(nab, 256);
    rarray = numaGetFArray(nar, L_NOCOPY);
    garray = numaGetFArray(nag, L_NOCOPY);
    barray = numaGetFArray(nab, L_NOCOPY);
    *pnar = nar;
    *pnag = nag;
    *pnab = nab;

        /* Generate the color histograms */
    data = pixGetData(pixs);
    wpl = pixGetWpl(pixs);
    if (cmap) {
        for (i = 0; i < h; i += factor) {
            line = data + i * wpl;
            for (j = 0; j < w; j += factor) {
                if (d == 8)
                    index = GET_DATA_BYTE(line, j);
                else if (d == 4)
                    index = GET_DATA_QBIT(line, j);
                else   /* 2 bpp */
                    index = GET_DATA_DIBIT(line, j);
                pixcmapGetColor(cmap, index, &rval, &gval, &bval);
                rarray[rval] += 1.0;
                garray[gval] += 1.0;
                barray[bval] += 1.0;
            }
        }
    } else {  /* 32 bpp rgb */
        for (i = 0; i < h; i += factor) {
            line = data + i * wpl;
            for (j = 0; j < w; j += factor) {
                extractRGBValues(line[j], &rval, &gval, &bval);
                rarray[rval] += 1.0;
                garray[gval] += 1.0;
                barray[bval] += 1.0;
            }
        }
    }

    return 0;
}


/*!
 * \brief   pixGetColorHistogramMasked()
 *
 * \param[in]    pixs     32 bpp rgb, or colormapped
 * \param[in]    pixm     [optional] 1 bpp mask over which histogram is
 *                        to be computed; use all pixels if null
 * \param[in]    x, y     UL corner of pixm relative to the UL corner of pixs;
 *                        can be < 0; these values are ignored if pixm is null
 * \param[in]    factor   subsampling factor; integer >= 1
 * \param[out]   pnar     red histogram
 * \param[out]   pnag     green histogram
 * \param[out]   pnab     blue histogram
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) This generates a set of three 256 entry histograms,
 *      (2) Set the subsampling %factor > 1 to reduce the amount of computation.
 *      (3) Clipping of pixm (if it exists) to pixs is done in the inner loop.
 *      (4) Input x,y are ignored unless pixm exists.
 * </pre>
 */
l_ok
pixGetColorHistogramMasked(PIX        *pixs,
                           PIX        *pixm,
                           l_int32     x,
                           l_int32     y,
                           l_int32     factor,
                           NUMA      **pnar,
                           NUMA      **pnag,
                           NUMA      **pnab)
{
l_int32     i, j, w, h, d, wm, hm, dm, wpls, wplm, index, rval, gval, bval;
l_uint32   *datas, *datam, *lines, *linem;
l_float32  *rarray, *garray, *barray;
NUMA       *nar, *nag, *nab;
PIXCMAP    *cmap;

    PROCNAME("pixGetColorHistogramMasked");

    if (!pixm)
        return pixGetColorHistogram(pixs, factor, pnar, pnag, pnab);

    if (pnar) *pnar = NULL;
    if (pnag) *pnag = NULL;
    if (pnab) *pnab = NULL;
    if (!pnar || !pnag || !pnab)
        return ERROR_INT("&nar, &nag, &nab not all defined", procName, 1);
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    pixGetDimensions(pixs, &w, &h, &d);
    cmap = pixGetColormap(pixs);
    if (cmap && (d != 2 && d != 4 && d != 8))
        return ERROR_INT("colormap and not 2, 4, or 8 bpp", procName, 1);
    if (!cmap && d != 32)
        return ERROR_INT("no colormap and not rgb", procName, 1);
    pixGetDimensions(pixm, &wm, &hm, &dm);
    if (dm != 1)
        return ERROR_INT("pixm not 1 bpp", procName, 1);
    if (factor < 1)
        return ERROR_INT("sampling factor must be >= 1", procName, 1);

        /* Set up the histogram arrays */
    nar = numaCreate(256);
    nag = numaCreate(256);
    nab = numaCreate(256);
    numaSetCount(nar, 256);
    numaSetCount(nag, 256);
    numaSetCount(nab, 256);
    rarray = numaGetFArray(nar, L_NOCOPY);
    garray = numaGetFArray(nag, L_NOCOPY);
    barray = numaGetFArray(nab, L_NOCOPY);
    *pnar = nar;
    *pnag = nag;
    *pnab = nab;

        /* Generate the color histograms */
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    datam = pixGetData(pixm);
    wplm = pixGetWpl(pixm);
    if (cmap) {
        for (i = 0; i < hm; i += factor) {
            if (y + i < 0 || y + i >= h) continue;
            lines = datas + (y + i) * wpls;
            linem = datam + i * wplm;
            for (j = 0; j < wm; j += factor) {
                if (x + j < 0 || x + j >= w) continue;
                if (GET_DATA_BIT(linem, j)) {
                    if (d == 8)
                        index = GET_DATA_BYTE(lines, x + j);
                    else if (d == 4)
                        index = GET_DATA_QBIT(lines, x + j);
                    else   /* 2 bpp */
                        index = GET_DATA_DIBIT(lines, x + j);
                    pixcmapGetColor(cmap, index, &rval, &gval, &bval);
                    rarray[rval] += 1.0;
                    garray[gval] += 1.0;
                    barray[bval] += 1.0;
                }
            }
        }
    } else {  /* 32 bpp rgb */
        for (i = 0; i < hm; i += factor) {
            if (y + i < 0 || y + i >= h) continue;
            lines = datas + (y + i) * wpls;
            linem = datam + i * wplm;
            for (j = 0; j < wm; j += factor) {
                if (x + j < 0 || x + j >= w) continue;
                if (GET_DATA_BIT(linem, j)) {
                    extractRGBValues(lines[x + j], &rval, &gval, &bval);
                    rarray[rval] += 1.0;
                    garray[gval] += 1.0;
                    barray[bval] += 1.0;
                }
            }
        }
    }

    return 0;
}


/*!
 * \brief   pixGetCmapHistogram()
 *
 * \param[in]   pixs    colormapped: d = 2, 4 or 8
 * \param[in]   factor  subsampling factor; integer >= 1
 * \return  na histogram of cmap indices, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) This generates a histogram of colormap pixel indices,
 *          and is of size 2^d.
 *      (2) Set the subsampling %factor > 1 to reduce the amount of computation.
 * </pre>
 */
NUMA *
pixGetCmapHistogram(PIX     *pixs,
                    l_int32  factor)
{
l_int32     i, j, w, h, d, wpl, val, size;
l_uint32   *data, *line;
l_float32  *array;
NUMA       *na;

    PROCNAME("pixGetCmapHistogram");

    if (!pixs)
        return (NUMA *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetColormap(pixs) == NULL)
        return (NUMA *)ERROR_PTR("pixs not cmapped", procName, NULL);
    if (factor < 1)
        return (NUMA *)ERROR_PTR("sampling must be >= 1", procName, NULL);
    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 2 && d != 4 && d != 8)
        return (NUMA *)ERROR_PTR("d not 2, 4 or 8", procName, NULL);

    size = 1 << d;
    if ((na = numaCreate(size)) == NULL)
        return (NUMA *)ERROR_PTR("na not made", procName, NULL);
    numaSetCount(na, size);  /* all initialized to 0.0 */
    array = numaGetFArray(na, L_NOCOPY);

    wpl = pixGetWpl(pixs);
    data = pixGetData(pixs);
    for (i = 0; i < h; i += factor) {
        line = data + i * wpl;
        for (j = 0; j < w; j += factor) {
            if (d == 8)
                val = GET_DATA_BYTE(line, j);
            else if (d == 4)
                val = GET_DATA_QBIT(line, j);
            else  /* d == 2 */
                val = GET_DATA_DIBIT(line, j);
            array[val] += 1.0;
        }
    }

    return na;
}


/*!
 * \brief   pixGetCmapHistogramMasked()
 *
 * \param[in]   pixs     colormapped: d = 2, 4 or 8
 * \param[in]   pixm     [optional] 1 bpp mask over which histogram is
 *                       to be computed; use all pixels if null
 * \param[in]   x, y     UL corner of pixm relative to the UL corner of pixs;
 *                       can be < 0; these values are ignored if pixm is null
 * \param[in]   factor   subsampling factor; integer >= 1
 * \return  na histogram, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) This generates a histogram of colormap pixel indices,
 *          and is of size 2^d.
 *      (2) Set the subsampling %factor > 1 to reduce the amount of computation.
 *      (3) Clipping of pixm to pixs is done in the inner loop.
 * </pre>
 */
NUMA *
pixGetCmapHistogramMasked(PIX     *pixs,
                          PIX     *pixm,
                          l_int32  x,
                          l_int32  y,
                          l_int32  factor)
{
l_int32     i, j, w, h, d, wm, hm, dm, wpls, wplm, val, size;
l_uint32   *datas, *datam, *lines, *linem;
l_float32  *array;
NUMA       *na;

    PROCNAME("pixGetCmapHistogramMasked");

    if (!pixm)
        return pixGetCmapHistogram(pixs, factor);

    if (!pixs)
        return (NUMA *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetColormap(pixs) == NULL)
        return (NUMA *)ERROR_PTR("pixs not cmapped", procName, NULL);
    pixGetDimensions(pixm, &wm, &hm, &dm);
    if (dm != 1)
        return (NUMA *)ERROR_PTR("pixm not 1 bpp", procName, NULL);
    if (factor < 1)
        return (NUMA *)ERROR_PTR("sampling must be >= 1", procName, NULL);
    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 2 && d != 4 && d != 8)
        return (NUMA *)ERROR_PTR("d not 2, 4 or 8", procName, NULL);

    size = 1 << d;
    if ((na = numaCreate(size)) == NULL)
        return (NUMA *)ERROR_PTR("na not made", procName, NULL);
    numaSetCount(na, size);  /* all initialized to 0.0 */
    array = numaGetFArray(na, L_NOCOPY);

    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    datam = pixGetData(pixm);
    wplm = pixGetWpl(pixm);

    for (i = 0; i < hm; i += factor) {
        if (y + i < 0 || y + i >= h) continue;
        lines = datas + (y + i) * wpls;
        linem = datam + i * wplm;
        for (j = 0; j < wm; j += factor) {
            if (x + j < 0 || x + j >= w) continue;
            if (GET_DATA_BIT(linem, j)) {
                if (d == 8)
                    val = GET_DATA_BYTE(lines, x + j);
                else if (d == 4)
                    val = GET_DATA_QBIT(lines, x + j);
                else  /* d == 2 */
                    val = GET_DATA_DIBIT(lines, x + j);
                array[val] += 1.0;
            }
        }
    }

    return na;
}


/*!
 * \brief   pixGetCmapHistogramInRect()
 *
 * \param[in]   pixs     colormapped: d = 2, 4 or 8
 * \param[in]   box      [optional] over which histogram is to be computed;
 *                       use full image if NULL
 * \param[in]   factor   subsampling factor; integer >= 1
 * \return  na histogram, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) This generates a histogram of colormap pixel indices,
 *          and is of size 2^d.
 *      (2) Set the subsampling %factor > 1 to reduce the amount of computation.
 *      (3) Clipping to the box is done in the inner loop.
 * </pre>
 */
NUMA *
pixGetCmapHistogramInRect(PIX     *pixs,
                          BOX     *box,
                          l_int32  factor)
{
l_int32     i, j, bx, by, bw, bh, w, h, d, wpls, val, size;
l_uint32   *datas, *lines;
l_float32  *array;
NUMA       *na;

    PROCNAME("pixGetCmapHistogramInRect");

    if (!box)
        return pixGetCmapHistogram(pixs, factor);
    if (!pixs)
        return (NUMA *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetColormap(pixs) == NULL)
        return (NUMA *)ERROR_PTR("pixs not cmapped", procName, NULL);
    if (factor < 1)
        return (NUMA *)ERROR_PTR("sampling must be >= 1", procName, NULL);
    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 2 && d != 4 && d != 8)
        return (NUMA *)ERROR_PTR("d not 2, 4 or 8", procName, NULL);

    size = 1 << d;
    if ((na = numaCreate(size)) == NULL)
        return (NUMA *)ERROR_PTR("na not made", procName, NULL);
    numaSetCount(na, size);  /* all initialized to 0.0 */
    array = numaGetFArray(na, L_NOCOPY);

    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    boxGetGeometry(box, &bx, &by, &bw, &bh);

    for (i = 0; i < bh; i += factor) {
        if (by + i < 0 || by + i >= h) continue;
        lines = datas + (by + i) * wpls;
        for (j = 0; j < bw; j += factor) {
            if (bx + j < 0 || bx + j >= w) continue;
            if (d == 8)
                val = GET_DATA_BYTE(lines, bx + j);
            else if (d == 4)
                val = GET_DATA_QBIT(lines, bx + j);
            else  /* d == 2 */
                val = GET_DATA_DIBIT(lines, bx + j);
            array[val] += 1.0;
        }
    }

    return na;
}


/*!
 * \brief   pixCountRGBColors()
 *
 * \param[in]    pixs    rgb or rgba
 * \return  ncolors, or -1 on error
 */
l_int32
pixCountRGBColors(PIX  *pixs)
{
l_int32  ncolors;
L_AMAP  *amap;

    PROCNAME("pixCountRGBColors");

    if (!pixs || pixGetDepth(pixs) != 32)
        return ERROR_INT("pixs not defined or not 32 bpp", procName, -1);
    amap = pixGetColorAmapHistogram(pixs, 1);
    ncolors = l_amapSize(amap);
    l_amapDestroy(&amap);
    return ncolors;
}


/*!
 * \brief   pixGetColorAmapHistogram()
 *
 * \param[in]   pixs    rgb or rgba
 * \param[in]   factor  subsampling factor; integer >= 1
 * \return  amap, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) This generates an ordered map from pixel value to histogram count.
 *      (2) Use amapGetCountForColor() to use the map to look up a count.
 * </pre>
 */
L_AMAP  *
pixGetColorAmapHistogram(PIX     *pixs,
                         l_int32  factor)
{
l_int32    i, j, w, h, wpl;
l_uint32  *data, *line;
L_AMAP    *amap;
RB_TYPE    key, value;
RB_TYPE   *pval;

    PROCNAME("pixGetColorAmapHistogram");

    if (!pixs)
        return (L_AMAP *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 32)
        return (L_AMAP *)ERROR_PTR("pixs not 32 bpp", procName, NULL);
    pixGetDimensions(pixs, &w, &h, NULL);
    data = pixGetData(pixs);
    wpl = pixGetWpl(pixs);
    amap = l_amapCreate(L_UINT_TYPE);
    for (i = 0; i < h; i += factor) {
        line = data + i * wpl;
        for (j = 0; j < w; j += factor) {
            key.utype = line[j];
            pval = l_amapFind(amap, key);
            if (!pval)
                value.itype = 1;
            else
                value.itype = 1 + pval->itype;
            l_amapInsert(amap, key, value);
        }
    }

    return amap;
}


/*!
 * \brief   amapGetCountForColor()
 *
 * \param[in]   amap   map from pixel value to count
 * \param[in]   val    rgb or rgba pixel value
 * \return  count, or -1 on error
 *
 * <pre>
 * Notes:
 *      (1) The ordered map is made by pixGetColorAmapHistogram().
 * </pre>
 */
l_int32
amapGetCountForColor(L_AMAP   *amap,
                     l_uint32  val)
{
RB_TYPE   key;
RB_TYPE  *pval;

    PROCNAME("amapGetCountForColor");

    if (!amap)
        return ERROR_INT("amap not defined", procName, -1);

    key.utype = val;
    pval = l_amapFind(amap, key);
    return (pval) ? pval->itype : 0;
}


/*!
 * \brief   pixGetRankValue()
 *
 * \param[in]    pixs     8 bpp, 32 bpp or colormapped
 * \param[in]    factor   subsampling factor; integer >= 1
 * \param[in]    rank     between 0.0 and 1.0; 1.0 is brightest, 0.0 is darkest
 * \param[out]   pvalue   pixel value corresponding to input rank
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) Simple function to get rank values of an image.
 *          For a color image, the median value (rank = 0.5) can be
 *          used to linearly remap the colors based on the median
 *          of a target image, using pixLinearMapToTargetColor().
 * </pre>
 */
l_ok
pixGetRankValue(PIX       *pixs,
                l_int32    factor,
                l_float32  rank,
                l_uint32  *pvalue)
{
l_int32    d;
l_float32  val, rval, gval, bval;
PIX       *pixt;
PIXCMAP   *cmap;

    PROCNAME("pixGetRankValue");

    if (!pvalue)
        return ERROR_INT("&value not defined", procName, 1);
    *pvalue = 0;
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    d = pixGetDepth(pixs);
    cmap = pixGetColormap(pixs);
    if (d != 8 && d != 32 && !cmap)
        return ERROR_INT("pixs not 8 or 32 bpp, or cmapped", procName, 1);
    if (cmap)
        pixt = pixRemoveColormap(pixs, REMOVE_CMAP_BASED_ON_SRC);
    else
        pixt = pixClone(pixs);
    d = pixGetDepth(pixt);

    if (d == 8) {
        pixGetRankValueMasked(pixt, NULL, 0, 0, factor, rank, &val, NULL);
        *pvalue = lept_roundftoi(val);
    } else {
        pixGetRankValueMaskedRGB(pixt, NULL, 0, 0, factor, rank,
                                 &rval, &gval, &bval);
        composeRGBPixel(lept_roundftoi(rval), lept_roundftoi(gval),
                        lept_roundftoi(bval), pvalue);
    }

    pixDestroy(&pixt);
    return 0;
}


/*!
 * \brief   pixGetRankValueMaskedRGB()
 *
 * \param[in]    pixs     32 bpp
 * \param[in]    pixm     [optional] 1 bpp mask over which rank val is to be taken;
 *                        use all pixels if null
 * \param[in]    x, y     UL corner of pixm relative to the UL corner of pixs;
 *                        can be < 0; these values are ignored if pixm is null
 * \param[in]    factor   subsampling factor; integer >= 1
 * \param[in]    rank     between 0.0 and 1.0; 1.0 is brightest, 0.0 is darkest
 * \param[out]   prval    [optional] red component val for input rank
 * \param[out]   pgval    [optional] green component val for input rank
 * \param[out]   pbval    [optional] blue component val for input rank
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) Computes the rank component values of pixels in pixs that
 *          are under the fg of the optional mask.  If the mask is null, it
 *          computes the average of the pixels in pixs.
 *      (2) Set the subsampling %factor > 1 to reduce the amount of
 *          computation.
 *      (4) Input x,y are ignored unless pixm exists.
 *      (5) The rank must be in [0.0 ... 1.0], where the brightest pixel
 *          has rank 1.0.  For the median pixel value, use 0.5.
 * </pre>
 */
l_ok
pixGetRankValueMaskedRGB(PIX        *pixs,
                         PIX        *pixm,
                         l_int32     x,
                         l_int32     y,
                         l_int32     factor,
                         l_float32   rank,
                         l_float32  *prval,
                         l_float32  *pgval,
                         l_float32  *pbval)
{
l_float32  scale;
PIX       *pixmt, *pixt;

    PROCNAME("pixGetRankValueMaskedRGB");

    if (prval) *prval = 0.0;
    if (pgval) *pgval = 0.0;
    if (pbval) *pbval = 0.0;
    if (!prval && !pgval && !pbval)
        return ERROR_INT("no results requested", procName, 1);
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (pixGetDepth(pixs) != 32)
        return ERROR_INT("pixs not 32 bpp", procName, 1);
    if (pixm && pixGetDepth(pixm) != 1)
        return ERROR_INT("pixm not 1 bpp", procName, 1);
    if (factor < 1)
        return ERROR_INT("sampling factor must be >= 1", procName, 1);
    if (rank < 0.0 || rank > 1.0)
        return ERROR_INT("rank not in [0.0 ... 1.0]", procName, 1);

    pixmt = NULL;
    if (pixm) {
        scale = 1.0 / (l_float32)factor;
        pixmt = pixScale(pixm, scale, scale);
    }
    if (prval) {
        pixt = pixScaleRGBToGrayFast(pixs, factor, COLOR_RED);
        pixGetRankValueMasked(pixt, pixmt, x / factor, y / factor,
                              factor, rank, prval, NULL);
        pixDestroy(&pixt);
    }
    if (pgval) {
        pixt = pixScaleRGBToGrayFast(pixs, factor, COLOR_GREEN);
        pixGetRankValueMasked(pixt, pixmt, x / factor, y / factor,
                              factor, rank, pgval, NULL);
        pixDestroy(&pixt);
    }
    if (pbval) {
        pixt = pixScaleRGBToGrayFast(pixs, factor, COLOR_BLUE);
        pixGetRankValueMasked(pixt, pixmt, x / factor, y / factor,
                              factor, rank, pbval, NULL);
        pixDestroy(&pixt);
    }
    pixDestroy(&pixmt);
    return 0;
}


/*!
 * \brief   pixGetRankValueMasked()
 *
 * \param[in]    pixs     8 bpp, or colormapped
 * \param[in]    pixm     [optional] 1 bpp mask, over which the rank val
 *                        is to be taken; use all pixels if null
 * \param[in]    x, y     UL corner of pixm relative to the UL corner of pixs;
 *                        can be < 0; these values are ignored if pixm is null
 * \param[in]    factor   subsampling factor; integer >= 1
 * \param[in]    rank     between 0.0 and 1.0; 1.0 is brightest, 0.0 is darkest
 * \param[out]   pval     pixel value corresponding to input rank
 * \param[out]   pna     [optional] of histogram
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) Computes the rank value of pixels in pixs that are under
 *          the fg of the optional mask.  If the mask is null, it
 *          computes the average of the pixels in pixs.
 *      (2) Set the subsampling %factor > 1 to reduce the amount of
 *          computation.
 *      (3) Clipping of pixm (if it exists) to pixs is done in the inner loop.
 *      (4) Input x,y are ignored unless pixm exists.
 *      (5) The rank must be in [0.0 ... 1.0], where the brightest pixel
 *          has rank 1.0.  For the median pixel value, use 0.5.
 *      (6) The histogram can optionally be returned, so that other rank
 *          values can be extracted without recomputing the histogram.
 *          In that case, just use
 *              numaHistogramGetValFromRank(na, rank, &val);
 *          on the returned Numa for additional rank values.
 * </pre>
 */
l_ok
pixGetRankValueMasked(PIX        *pixs,
                      PIX        *pixm,
                      l_int32     x,
                      l_int32     y,
                      l_int32     factor,
                      l_float32   rank,
                      l_float32  *pval,
                      NUMA      **pna)
{
NUMA  *na;

    PROCNAME("pixGetRankValueMasked");

    if (pna) *pna = NULL;
    if (!pval)
        return ERROR_INT("&val not defined", procName, 1);
    *pval = 0.0;
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (pixGetDepth(pixs) != 8 && !pixGetColormap(pixs))
        return ERROR_INT("pixs neither 8 bpp nor colormapped", procName, 1);
    if (pixm && pixGetDepth(pixm) != 1)
        return ERROR_INT("pixm not 1 bpp", procName, 1);
    if (factor < 1)
        return ERROR_INT("sampling factor must be >= 1", procName, 1);
    if (rank < 0.0 || rank > 1.0)
        return ERROR_INT("rank not in [0.0 ... 1.0]", procName, 1);

    if ((na = pixGetGrayHistogramMasked(pixs, pixm, x, y, factor)) == NULL)
        return ERROR_INT("na not made", procName, 1);
    numaHistogramGetValFromRank(na, rank, pval);
    if (pna)
        *pna = na;
    else
        numaDestroy(&na);

    return 0;
}


/*!
 * \brief   pixGetPixelAverage()
 *
 * \param[in]    pixs     8 or 32 bpp, or colormapped
 * \param[in]    pixm     [optional] 1 bpp mask over which average is
 *                        to be taken; use all pixels if null
 * \param[in]    x, y     UL corner of pixm relative to the UL corner of pixs;
 *                        can be < 0
 * \param[in]    factor   subsampling factor; >= 1
 * \param[out]   pval     average pixel value
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) For rgb pix, this is a more direct computation of the
 *          average value of the pixels in %pixs that are under the
 *          mask %pixm. It is faster than pixGetPixelStats(), which
 *          calls pixGetAverageMaskedRGB() and has the overhead of
 *          generating a temporary pix of each of the three components;
 *          this can take most of the time if %factor > 1.
 *      (2) If %pixm is null, this gives the average value of all
 *          pixels in %pixs.  The returned value is an integer.
 *      (3) For color %pixs, the returned pixel value is in the standard
 *          uint32 RGBA packing.
 *      (4) Clipping of pixm (if it exists) to pixs is done in the inner loop.
 *      (5) Input x,y are ignored if %pixm does not exist.
 * </pre>
 */
l_ok
pixGetPixelAverage(PIX       *pixs,
                   PIX       *pixm,
                   l_int32    x,
                   l_int32    y,
                   l_int32    factor,
                   l_uint32  *pval)
{
l_int32    i, j, w, h, d, wm, hm, wpl1, wplm, val, rval, gval, bval, count;
l_uint32  *data1, *datam, *line1, *linem;
l_float64  sum, rsum, gsum, bsum;
PIX       *pix1;

    PROCNAME("pixGetPixelAverage");

    if (!pval)
        return ERROR_INT("&val not defined", procName, 1);
    *pval = 0;
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    d = pixGetDepth(pixs);
    if (d != 32 && !pixGetColormap(pixs))
        return ERROR_INT("pixs not rgb or colormapped", procName, 1);
    if (pixm && pixGetDepth(pixm) != 1)
        return ERROR_INT("pixm not 1 bpp", procName, 1);
    if (factor < 1)
        return ERROR_INT("sampling factor must be >= 1", procName, 1);

    if (pixGetColormap(pixs))
        pix1 = pixRemoveColormap(pixs, REMOVE_CMAP_BASED_ON_SRC);
    else
        pix1 = pixClone(pixs);
    pixGetDimensions(pix1, &w, &h, &d);
    if (d == 1) {
        pixDestroy(&pix1);
        return ERROR_INT("pix1 is just 1 bpp", procName, 1);
    }
    data1 = pixGetData(pix1);
    wpl1 = pixGetWpl(pix1);

    sum = rsum = gsum = bsum = 0.0;
    count = 0;
    if (!pixm) {
        for (i = 0; i < h; i += factor) {
            line1 = data1 + i * wpl1;
            for (j = 0; j < w; j += factor) {
                if (d == 8) {
                    val = GET_DATA_BYTE(line1, j);
                    sum += val;
                } else {  /* rgb */
                    extractRGBValues(*(line1 + j), &rval, &gval, &bval);
                    rsum += rval;
                    gsum += gval;
                    bsum += bval;
                }
                count++;
            }
        }
    } else {  /* masked */
        pixGetDimensions(pixm, &wm, &hm, NULL);
        datam = pixGetData(pixm);
        wplm = pixGetWpl(pixm);
        for (i = 0; i < hm; i += factor) {
            if (y + i < 0 || y + i >= h) continue;
            line1 = data1 + (y + i) * wpl1;
            linem = datam + i * wplm;
            for (j = 0; j < wm; j += factor) {
                if (x + j < 0 || x + j >= w) continue;
                if (GET_DATA_BIT(linem, j)) {
                    if (d == 8) {
                        val = GET_DATA_BYTE(line1, x + j);
                        sum += val;
                    } else {  /* rgb */
                        extractRGBValues(*(line1 + x + j), &rval, &gval, &bval);
                        rsum += rval;
                        gsum += gval;
                        bsum += bval;
                    }
                    count++;
                }
            }
        }
    }

    pixDestroy(&pix1);
    if (count == 0)
        return ERROR_INT("no pixels sampled", procName, 1);
    if (d == 8) {
        *pval = (l_uint32)((l_float64)sum / (l_float64)count);
    } else {  /* d == 32 */
        rval = (l_uint32)((l_float64)rsum / (l_float64)count);
        gval = (l_uint32)((l_float64)gsum / (l_float64)count);
        bval = (l_uint32)((l_float64)bsum / (l_float64)count);
        composeRGBPixel(rval, gval, bval, pval);
    }

    return 0;
}


/*!
 * \brief   pixGetPixelStats()
 *
 * \param[in]    pixs     8 bpp, 32 bpp or colormapped
 * \param[in]    factor   subsampling factor; integer >= 1
 * \param[in]    type     L_MEAN_ABSVAL, L_ROOT_MEAN_SQUARE,
 *                        L_STANDARD_DEVIATION, L_VARIANCE
 * \param[out]   pvalue   pixel value corresponding to input type
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) Simple function to get one of four statistical values of an image.
 *      (2) It does not take a mask: it uses the entire image.
 *      (3) To get the average pixel value of an RGB image, suggest using
 *          pixGetPixelAverage(), which is considerably faster.
 * </pre>
 */
l_ok
pixGetPixelStats(PIX       *pixs,
                 l_int32    factor,
                 l_int32    type,
                 l_uint32  *pvalue)
{
l_int32    d;
l_float32  val, rval, gval, bval;
PIX       *pixt;
PIXCMAP   *cmap;

    PROCNAME("pixGetPixelStats");

    if (!pvalue)
        return ERROR_INT("&value not defined", procName, 1);
    *pvalue = 0;
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    d = pixGetDepth(pixs);
    cmap = pixGetColormap(pixs);
    if (d != 8 && d != 32 && !cmap)
        return ERROR_INT("pixs not 8 or 32 bpp, or cmapped", procName, 1);
    if (cmap)
        pixt = pixRemoveColormap(pixs, REMOVE_CMAP_BASED_ON_SRC);
    else
        pixt = pixClone(pixs);
    d = pixGetDepth(pixt);

    if (d == 8) {
        pixGetAverageMasked(pixt, NULL, 0, 0, factor, type, &val);
        *pvalue = lept_roundftoi(val);
    } else {
        pixGetAverageMaskedRGB(pixt, NULL, 0, 0, factor, type,
                               &rval, &gval, &bval);
        composeRGBPixel(lept_roundftoi(rval), lept_roundftoi(gval),
                        lept_roundftoi(bval), pvalue);
    }

    pixDestroy(&pixt);
    return 0;
}


/*!
 * \brief   pixGetAverageMaskedRGB()
 *
 * \param[in]    pixs     32 bpp, or colormapped
 * \param[in]    pixm     [optional] 1 bpp mask over which average is
 *                        to be taken; use all pixels if null
 * \param[in]    x, y     UL corner of pixm relative to the UL corner of pixs;
 *                        can be < 0
 * \param[in]    factor   subsampling factor; >= 1
 * \param[in]    type     L_MEAN_ABSVAL, L_ROOT_MEAN_SQUARE,
 *                        L_STANDARD_DEVIATION, L_VARIANCE
 * \param[out]   prval    [optional] measured red value of given 'type'
 * \param[out]   pgval    [optional] measured green value of given 'type'
 * \param[out]   pbval    [optional] measured blue value of given 'type'
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) For usage, see pixGetAverageMasked().
 *      (2) If there is a colormap, it is removed before the 8 bpp
 *          component images are extracted.
 *      (3) A better name for this would be: pixGetPixelStatsRGB()
 * </pre>
 */
l_ok
pixGetAverageMaskedRGB(PIX        *pixs,
                       PIX        *pixm,
                       l_int32     x,
                       l_int32     y,
                       l_int32     factor,
                       l_int32     type,
                       l_float32  *prval,
                       l_float32  *pgval,
                       l_float32  *pbval)
{
PIX      *pixt;
PIXCMAP  *cmap;

    PROCNAME("pixGetAverageMaskedRGB");

    if (prval) *prval = 0.0;
    if (pgval) *pgval = 0.0;
    if (pbval) *pbval = 0.0;
    if (!prval && !pgval && !pbval)
        return ERROR_INT("no values requested", procName, 1);
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    cmap = pixGetColormap(pixs);
    if (pixGetDepth(pixs) != 32 && !cmap)
        return ERROR_INT("pixs neither 32 bpp nor colormapped", procName, 1);
    if (pixm && pixGetDepth(pixm) != 1)
        return ERROR_INT("pixm not 1 bpp", procName, 1);
    if (factor < 1)
        return ERROR_INT("sampling factor must be >= 1", procName, 1);
    if (type != L_MEAN_ABSVAL && type != L_ROOT_MEAN_SQUARE &&
        type != L_STANDARD_DEVIATION && type != L_VARIANCE)
        return ERROR_INT("invalid measure type", procName, 1);

    if (prval) {
        if (cmap)
            pixt = pixGetRGBComponentCmap(pixs, COLOR_RED);
        else
            pixt = pixGetRGBComponent(pixs, COLOR_RED);
        pixGetAverageMasked(pixt, pixm, x, y, factor, type, prval);
        pixDestroy(&pixt);
    }
    if (pgval) {
        if (cmap)
            pixt = pixGetRGBComponentCmap(pixs, COLOR_GREEN);
        else
            pixt = pixGetRGBComponent(pixs, COLOR_GREEN);
        pixGetAverageMasked(pixt, pixm, x, y, factor, type, pgval);
        pixDestroy(&pixt);
    }
    if (pbval) {
        if (cmap)
            pixt = pixGetRGBComponentCmap(pixs, COLOR_BLUE);
        else
            pixt = pixGetRGBComponent(pixs, COLOR_BLUE);
        pixGetAverageMasked(pixt, pixm, x, y, factor, type, pbval);
        pixDestroy(&pixt);
    }

    return 0;
}


/*!
 * \brief   pixGetAverageMasked()
 *
 * \param[in]   pixs     8 or 16 bpp, or colormapped
 * \param[in]   pixm     [optional] 1 bpp mask over which average is
 *                       to be taken; use all pixels if null
 * \param[in]   x, y     UL corner of pixm relative to the UL corner of pixs;
 *                       can be < 0
 * \param[in]   factor   subsampling factor; >= 1
 * \param[in]   type     L_MEAN_ABSVAL, L_ROOT_MEAN_SQUARE,
 *                       L_STANDARD_DEVIATION, L_VARIANCE
 * \param[out]  pval     measured value of given 'type'
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) Use L_MEAN_ABSVAL to get the average value of pixels in pixs
 *          that are under the fg of the optional mask.  If the mask
 *          is null, it finds the average of the pixels in pixs.
 *      (2) Likewise, use L_ROOT_MEAN_SQUARE to get the rms value of
 *          pixels in pixs, either masked or not; L_STANDARD_DEVIATION
 *          to get the standard deviation from the mean of the pixels;
 *          L_VARIANCE to get the average squared difference from the
 *          expected value.  The variance is the square of the stdev.
 *          For the standard deviation, we use
 *              sqrt([([x] - x)]^2) = sqrt([x^2] - [x]^2)
 *      (3) Set the subsampling %factor > 1 to reduce the amount of
 *          computation.
 *      (4) Clipping of pixm (if it exists) to pixs is done in the inner loop.
 *      (5) Input x,y are ignored unless pixm exists.
 *      (6) A better name for this would be: pixGetPixelStatsGray()
 * </pre>
 */
l_ok
pixGetAverageMasked(PIX        *pixs,
                    PIX        *pixm,
                    l_int32     x,
                    l_int32     y,
                    l_int32     factor,
                    l_int32     type,
                    l_float32  *pval)
{
l_int32    i, j, w, h, d, wm, hm, wplg, wplm, val, count;
l_uint32  *datag, *datam, *lineg, *linem;
l_float64  sumave, summs, ave, meansq, var;
PIX       *pixg;

    PROCNAME("pixGetAverageMasked");

    if (!pval)
        return ERROR_INT("&val not defined", procName, 1);
    *pval = 0.0;
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    d = pixGetDepth(pixs);
    if (d != 8 && d != 16 && !pixGetColormap(pixs))
        return ERROR_INT("pixs not 8 or 16 bpp or colormapped", procName, 1);
    if (pixm && pixGetDepth(pixm) != 1)
        return ERROR_INT("pixm not 1 bpp", procName, 1);
    if (factor < 1)
        return ERROR_INT("sampling factor must be >= 1", procName, 1);
    if (type != L_MEAN_ABSVAL && type != L_ROOT_MEAN_SQUARE &&
        type != L_STANDARD_DEVIATION && type != L_VARIANCE)
        return ERROR_INT("invalid measure type", procName, 1);

    if (pixGetColormap(pixs))
        pixg = pixRemoveColormap(pixs, REMOVE_CMAP_TO_GRAYSCALE);
    else
        pixg = pixClone(pixs);
    pixGetDimensions(pixg, &w, &h, &d);
    datag = pixGetData(pixg);
    wplg = pixGetWpl(pixg);

    sumave = summs = 0.0;
    count = 0;
    if (!pixm) {
        for (i = 0; i < h; i += factor) {
            lineg = datag + i * wplg;
            for (j = 0; j < w; j += factor) {
                if (d == 8)
                    val = GET_DATA_BYTE(lineg, j);
                else  /* d == 16 */
                    val = GET_DATA_TWO_BYTES(lineg, j);
                if (type != L_ROOT_MEAN_SQUARE)
                    sumave += val;
                if (type != L_MEAN_ABSVAL)
                    summs += (l_float64)(val) * val;
                count++;
            }
        }
    } else {
        pixGetDimensions(pixm, &wm, &hm, NULL);
        datam = pixGetData(pixm);
        wplm = pixGetWpl(pixm);
        for (i = 0; i < hm; i += factor) {
            if (y + i < 0 || y + i >= h) continue;
            lineg = datag + (y + i) * wplg;
            linem = datam + i * wplm;
            for (j = 0; j < wm; j += factor) {
                if (x + j < 0 || x + j >= w) continue;
                if (GET_DATA_BIT(linem, j)) {
                    if (d == 8)
                        val = GET_DATA_BYTE(lineg, x + j);
                    else  /* d == 16 */
                        val = GET_DATA_TWO_BYTES(lineg, x + j);
                    if (type != L_ROOT_MEAN_SQUARE)
                        sumave += val;
                    if (type != L_MEAN_ABSVAL)
                        summs += (l_float64)(val) * val;
                    count++;
                }
            }
        }
    }

    pixDestroy(&pixg);
    if (count == 0)
        return ERROR_INT("no pixels sampled", procName, 1);
    ave = sumave / (l_float64)count;
    meansq = summs / (l_float64)count;
    var = meansq - ave * ave;
    if (type == L_MEAN_ABSVAL)
        *pval = (l_float32)ave;
    else if (type == L_ROOT_MEAN_SQUARE)
        *pval = (l_float32)sqrt(meansq);
    else if (type == L_STANDARD_DEVIATION)
        *pval = (l_float32)sqrt(var);
    else  /* type == L_VARIANCE */
        *pval = (l_float32)var;

    return 0;
}


/*!
 * \brief   pixGetAverageTiledRGB()
 *
 * \param[in]   pixs     32 bpp, or colormapped
 * \param[in]   sx, sy   tile size; must be at least 2 x 2
 * \param[in]   type     L_MEAN_ABSVAL, L_ROOT_MEAN_SQUARE, L_STANDARD_DEVIATION
 * \param[out]  ppixr    [optional] tiled 'average' of red component
 * \param[out]  ppixg    [optional] tiled 'average' of green component
 * \param[out]  ppixb    [optional] tiled 'average' of blue component
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) For usage, see pixGetAverageTiled().
 *      (2) If there is a colormap, it is removed before the 8 bpp
 *          component images are extracted.
 * </pre>
 */
l_ok
pixGetAverageTiledRGB(PIX     *pixs,
                      l_int32  sx,
                      l_int32  sy,
                      l_int32  type,
                      PIX    **ppixr,
                      PIX    **ppixg,
                      PIX    **ppixb)
{
PIX      *pixt;
PIXCMAP  *cmap;

    PROCNAME("pixGetAverageTiledRGB");

    if (ppixr) *ppixr = NULL;
    if (ppixg) *ppixg = NULL;
    if (ppixb) *ppixb = NULL;
    if (!ppixr && !ppixg && !ppixb)
        return ERROR_INT("no data requested", procName, 1);
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    cmap = pixGetColormap(pixs);
    if (pixGetDepth(pixs) != 32 && !cmap)
        return ERROR_INT("pixs neither 32 bpp nor colormapped", procName, 1);
    if (sx < 2 || sy < 2)
        return ERROR_INT("sx and sy not both > 1", procName, 1);
    if (type != L_MEAN_ABSVAL && type != L_ROOT_MEAN_SQUARE &&
        type != L_STANDARD_DEVIATION)
        return ERROR_INT("invalid measure type", procName, 1);

    if (ppixr) {
        if (cmap)
            pixt = pixGetRGBComponentCmap(pixs, COLOR_RED);
        else
            pixt = pixGetRGBComponent(pixs, COLOR_RED);
        *ppixr = pixGetAverageTiled(pixt, sx, sy, type);
        pixDestroy(&pixt);
    }
    if (ppixg) {
        if (cmap)
            pixt = pixGetRGBComponentCmap(pixs, COLOR_GREEN);
        else
            pixt = pixGetRGBComponent(pixs, COLOR_GREEN);
        *ppixg = pixGetAverageTiled(pixt, sx, sy, type);
        pixDestroy(&pixt);
    }
    if (ppixb) {
        if (cmap)
            pixt = pixGetRGBComponentCmap(pixs, COLOR_BLUE);
        else
            pixt = pixGetRGBComponent(pixs, COLOR_BLUE);
        *ppixb = pixGetAverageTiled(pixt, sx, sy, type);
        pixDestroy(&pixt);
    }

    return 0;
}


/*!
 * \brief   pixGetAverageTiled()
 *
 * \param[in]   pixs    8 bpp, or colormapped
 * \param[in]   sx, sy  tile size; must be at least 2 x 2
 * \param[in]   type    L_MEAN_ABSVAL, L_ROOT_MEAN_SQUARE, L_STANDARD_DEVIATION
 * \return  pixd average values in each tile, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) Only computes for tiles that are entirely contained in pixs.
 *      (2) Use L_MEAN_ABSVAL to get the average abs value within the tile;
 *          L_ROOT_MEAN_SQUARE to get the rms value within each tile;
 *          L_STANDARD_DEVIATION to get the standard dev. from the average
 *          within each tile.
 *      (3) If colormapped, converts to 8 bpp gray.
 * </pre>
 */
PIX *
pixGetAverageTiled(PIX     *pixs,
                   l_int32  sx,
                   l_int32  sy,
                   l_int32  type)
{
l_int32    i, j, k, m, w, h, wd, hd, d, pos, wplt, wpld, valt;
l_uint32  *datat, *datad, *linet, *lined, *startt;
l_float64  sumave, summs, ave, meansq, normfact;
PIX       *pixt, *pixd;

    PROCNAME("pixGetAverageTiled");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 8 && !pixGetColormap(pixs))
        return (PIX *)ERROR_PTR("pixs not 8 bpp or cmapped", procName, NULL);
    if (sx < 2 || sy < 2)
        return (PIX *)ERROR_PTR("sx and sy not both > 1", procName, NULL);
    wd = w / sx;
    hd = h / sy;
    if (wd < 1 || hd < 1)
        return (PIX *)ERROR_PTR("wd or hd == 0", procName, NULL);
    if (type != L_MEAN_ABSVAL && type != L_ROOT_MEAN_SQUARE &&
        type != L_STANDARD_DEVIATION)
        return (PIX *)ERROR_PTR("invalid measure type", procName, NULL);

    pixt = pixRemoveColormap(pixs, REMOVE_CMAP_TO_GRAYSCALE);
    pixd = pixCreate(wd, hd, 8);
    datat = pixGetData(pixt);
    wplt = pixGetWpl(pixt);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);
    normfact = 1. / (l_float64)(sx * sy);
    for (i = 0; i < hd; i++) {
        lined = datad + i * wpld;
        linet = datat + i * sy * wplt;
        for (j = 0; j < wd; j++) {
            if (type == L_MEAN_ABSVAL || type == L_STANDARD_DEVIATION) {
                sumave = 0.0;
                for (k = 0; k < sy; k++) {
                    startt = linet + k * wplt;
                    for (m = 0; m < sx; m++) {
                        pos = j * sx + m;
                        valt = GET_DATA_BYTE(startt, pos);
                        sumave += valt;
                    }
                }
                ave = normfact * sumave;
            }
            if (type == L_ROOT_MEAN_SQUARE || type == L_STANDARD_DEVIATION) {
                summs = 0.0;
                for (k = 0; k < sy; k++) {
                    startt = linet + k * wplt;
                    for (m = 0; m < sx; m++) {
                        pos = j * sx + m;
                        valt = GET_DATA_BYTE(startt, pos);
                        summs += (l_float64)(valt) * valt;
                    }
                }
                meansq = normfact * summs;
            }
            if (type == L_MEAN_ABSVAL)
                valt = (l_int32)(ave + 0.5);
            else if (type == L_ROOT_MEAN_SQUARE)
                valt = (l_int32)(sqrt(meansq) + 0.5);
            else  /* type == L_STANDARD_DEVIATION */
                valt = (l_int32)(sqrt(meansq - ave * ave) + 0.5);
            SET_DATA_BYTE(lined, j, valt);
        }
    }

    pixDestroy(&pixt);
    return pixd;
}


/*!
 * \brief   pixRowStats()
 *
 * \param[in]    pixs          8 bpp; not cmapped
 * \param[in]    box           [optional] clipping box; can be null
 * \param[out]   pnamean       [optional] numa of mean values
 * \param[out]   pnamedian     [optional] numa of median values
 * \param[out]   pnamode       [optional] numa of mode intensity values
 * \param[out]   pnamodecount  [optional] numa of mode counts
 * \param[out]   pnavar        [optional] numa of variance
 * \param[out]   pnarootvar    [optional] numa of square root of variance
 * \return  na numa of requested statistic for each row, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) This computes numas that represent column vectors of statistics,
 *          with each of its values derived from the corresponding row of a Pix.
 *      (2) Use NULL on input to prevent computation of any of the 5 numas.
 *      (3) Other functions that compute pixel row statistics are:
 *             pixCountPixelsByRow()
 *             pixAverageByRow()
 *             pixVarianceByRow()
 *             pixGetRowStats()
 * </pre>
 */
l_int32
pixRowStats(PIX    *pixs,
            BOX    *box,
            NUMA  **pnamean,
            NUMA  **pnamedian,
            NUMA  **pnamode,
            NUMA  **pnamodecount,
            NUMA  **pnavar,
            NUMA  **pnarootvar)
{
l_int32     i, j, k, w, h, val, wpls, sum, sumsq, target, max, modeval;
l_int32     xstart, xend, ystart, yend, bw, bh;
l_int32    *histo;
l_uint32   *lines, *datas;
l_float32   norm;
l_float32  *famean, *fameansq, *favar, *farootvar;
l_float32  *famedian, *famode, *famodecount;

    PROCNAME("pixRowStats");

    if (pnamean) *pnamean = NULL;
    if (pnamedian) *pnamedian = NULL;
    if (pnamode) *pnamode = NULL;
    if (pnamodecount) *pnamodecount = NULL;
    if (pnavar) *pnavar = NULL;
    if (pnarootvar) *pnarootvar = NULL;
    if (!pixs || pixGetDepth(pixs) != 8)
        return ERROR_INT("pixs undefined or not 8 bpp", procName, 1);
    famean = fameansq = favar = farootvar = NULL;
    famedian = famode = famodecount = NULL;

    pixGetDimensions(pixs, &w, &h, NULL);
    if (boxClipToRectangleParams(box, w, h, &xstart, &ystart, &xend, &yend,
                                 &bw, &bh) == 1)
        return ERROR_INT("invalid clipping box", procName, 1);

        /* We need the mean for variance and root variance */
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    if (pnamean || pnavar || pnarootvar) {
        norm = 1. / (l_float32)bw;
        famean = (l_float32 *)LEPT_CALLOC(bh, sizeof(l_float32));
        fameansq = (l_float32 *)LEPT_CALLOC(bh, sizeof(l_float32));
        if (pnavar || pnarootvar) {
            favar = (l_float32 *)LEPT_CALLOC(bh, sizeof(l_float32));
            if (pnarootvar)
                farootvar = (l_float32 *)LEPT_CALLOC(bh, sizeof(l_float32));
        }
        for (i = ystart; i < yend; i++) {
            sum = sumsq = 0;
            lines = datas + i * wpls;
            for (j = xstart; j < xend; j++) {
                val = GET_DATA_BYTE(lines, j);
                sum += val;
                sumsq += val * val;
            }
            famean[i] = norm * sum;
            fameansq[i] = norm * sumsq;
            if (pnavar || pnarootvar) {
                favar[i] = fameansq[i] - famean[i] * famean[i];
                if (pnarootvar)
                    farootvar[i] = sqrtf(favar[i]);
            }
        }
        LEPT_FREE(fameansq);
        if (pnamean)
            *pnamean = numaCreateFromFArray(famean, bh, L_INSERT);
        else
            LEPT_FREE(famean);
        if (pnavar)
            *pnavar = numaCreateFromFArray(favar, bh, L_INSERT);
        else
            LEPT_FREE(favar);
        if (pnarootvar)
            *pnarootvar = numaCreateFromFArray(farootvar, bh, L_INSERT);
    }

        /* We need a histogram to find the median and/or mode values */
    if (pnamedian || pnamode || pnamodecount) {
        histo = (l_int32 *)LEPT_CALLOC(256, sizeof(l_int32));
        if (pnamedian) {
            *pnamedian = numaMakeConstant(0, bh);
            famedian = numaGetFArray(*pnamedian, L_NOCOPY);
        }
        if (pnamode) {
            *pnamode = numaMakeConstant(0, bh);
            famode = numaGetFArray(*pnamode, L_NOCOPY);
        }
        if (pnamodecount) {
            *pnamodecount = numaMakeConstant(0, bh);
            famodecount = numaGetFArray(*pnamodecount, L_NOCOPY);
        }
        for (i = ystart; i < yend; i++) {
            lines = datas + i * wpls;
            memset(histo, 0, 1024);
            for (j = xstart; j < xend; j++) {
                val = GET_DATA_BYTE(lines, j);
                histo[val]++;
            }

            if (pnamedian) {
                sum = 0;
                target = (bw + 1) / 2;
                for (k = 0; k < 256; k++) {
                    sum += histo[k];
                    if (sum >= target) {
                        famedian[i] = k;
                        break;
                    }
                }
            }

            if (pnamode || pnamodecount) {
                max = 0;
                modeval = 0;
                for (k = 0; k < 256; k++) {
                    if (histo[k] > max) {
                        max = histo[k];
                        modeval = k;
                    }
                }
                if (pnamode)
                    famode[i] = modeval;
                if (pnamodecount)
                    famodecount[i] = max;
            }
        }
        LEPT_FREE(histo);
    }

    return 0;
}


/*!
 * \brief   pixColumnStats()
 *
 * \param[in]    pixs          8 bpp; not cmapped
 * \param[in]    box           [optional] clipping box; can be null
 * \param[out]   pnamean       [optional] numa of mean values
 * \param[out]   pnamedian     [optional] numa of median values
 * \param[out]   pnamode       [optional] numa of mode intensity values
 * \param[out]   pnamodecount  [optional] numa of mode counts
 * \param[out]   pnavar        [optional] numa of variance
 * \param[out]   pnarootvar    [optional] numa of square root of variance
 * \return  na numa of requested statistic for each column,
 *                  or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) This computes numas that represent row vectors of statistics,
 *          with each of its values derived from the corresponding col of a Pix.
 *      (2) Use NULL on input to prevent computation of any of the 5 numas.
 *      (3) Other functions that compute pixel column statistics are:
 *             pixCountPixelsByColumn()
 *             pixAverageByColumn()
 *             pixVarianceByColumn()
 *             pixGetColumnStats()
 * </pre>
 */
l_int32
pixColumnStats(PIX    *pixs,
               BOX    *box,
               NUMA  **pnamean,
               NUMA  **pnamedian,
               NUMA  **pnamode,
               NUMA  **pnamodecount,
               NUMA  **pnavar,
               NUMA  **pnarootvar)
{
l_int32     i, j, k, w, h, val, wpls, sum, sumsq, target, max, modeval;
l_int32     xstart, xend, ystart, yend, bw, bh;
l_int32    *histo;
l_uint32   *lines, *datas;
l_float32   norm;
l_float32  *famean, *fameansq, *favar, *farootvar;
l_float32  *famedian, *famode, *famodecount;

    PROCNAME("pixColumnStats");

    if (pnamean) *pnamean = NULL;
    if (pnamedian) *pnamedian = NULL;
    if (pnamode) *pnamode = NULL;
    if (pnamodecount) *pnamodecount = NULL;
    if (pnavar) *pnavar = NULL;
    if (pnarootvar) *pnarootvar = NULL;
    if (!pixs || pixGetDepth(pixs) != 8)
        return ERROR_INT("pixs undefined or not 8 bpp", procName, 1);
    famean = fameansq = favar = farootvar = NULL;
    famedian = famode = famodecount = NULL;

    pixGetDimensions(pixs, &w, &h, NULL);
    if (boxClipToRectangleParams(box, w, h, &xstart, &ystart, &xend, &yend,
                                 &bw, &bh) == 1)
        return ERROR_INT("invalid clipping box", procName, 1);

        /* We need the mean for variance and root variance */
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    if (pnamean || pnavar || pnarootvar) {
        norm = 1. / (l_float32)bh;
        famean = (l_float32 *)LEPT_CALLOC(bw, sizeof(l_float32));
        fameansq = (l_float32 *)LEPT_CALLOC(bw, sizeof(l_float32));
        if (pnavar || pnarootvar) {
            favar = (l_float32 *)LEPT_CALLOC(bw, sizeof(l_float32));
            if (pnarootvar)
                farootvar = (l_float32 *)LEPT_CALLOC(bw, sizeof(l_float32));
        }
        for (j = xstart; j < xend; j++) {
            sum = sumsq = 0;
            for (i = ystart, lines = datas; i < yend; lines += wpls, i++) {
                val = GET_DATA_BYTE(lines, j);
                sum += val;
                sumsq += val * val;
            }
            famean[j] = norm * sum;
            fameansq[j] = norm * sumsq;
            if (pnavar || pnarootvar) {
                favar[j] = fameansq[j] - famean[j] * famean[j];
                if (pnarootvar)
                    farootvar[j] = sqrtf(favar[j]);
            }
        }
        LEPT_FREE(fameansq);
        if (pnamean)
            *pnamean = numaCreateFromFArray(famean, bw, L_INSERT);
        else
            LEPT_FREE(famean);
        if (pnavar)
            *pnavar = numaCreateFromFArray(favar, bw, L_INSERT);
        else
            LEPT_FREE(favar);
        if (pnarootvar)
            *pnarootvar = numaCreateFromFArray(farootvar, bw, L_INSERT);
    }

        /* We need a histogram to find the median and/or mode values */
    if (pnamedian || pnamode || pnamodecount) {
        histo = (l_int32 *)LEPT_CALLOC(256, sizeof(l_int32));
        if (pnamedian) {
            *pnamedian = numaMakeConstant(0, bw);
            famedian = numaGetFArray(*pnamedian, L_NOCOPY);
        }
        if (pnamode) {
            *pnamode = numaMakeConstant(0, bw);
            famode = numaGetFArray(*pnamode, L_NOCOPY);
        }
        if (pnamodecount) {
            *pnamodecount = numaMakeConstant(0, bw);
            famodecount = numaGetFArray(*pnamodecount, L_NOCOPY);
        }
        for (j = xstart; j < xend; j++) {
            memset(histo, 0, 1024);
            for (i = ystart, lines = datas; i < yend; lines += wpls, i++) {
                val = GET_DATA_BYTE(lines, j);
                histo[val]++;
            }

            if (pnamedian) {
                sum = 0;
                target = (bh + 1) / 2;
                for (k = 0; k < 256; k++) {
                    sum += histo[k];
                    if (sum >= target) {
                        famedian[j] = k;
                        break;
                    }
                }
            }

            if (pnamode || pnamodecount) {
                max = 0;
                modeval = 0;
                for (k = 0; k < 256; k++) {
                    if (histo[k] > max) {
                        max = histo[k];
                        modeval = k;
                    }
                }
                if (pnamode)
                    famode[j] = modeval;
                if (pnamodecount)
                    famodecount[j] = max;
            }
        }
        LEPT_FREE(histo);
    }

    return 0;
}


/*!
 * \brief   pixGetRangeValues()
 *
 * \param[in]    pixs     8 bpp grayscale, 32 bpp rgb, or colormapped
 * \param[in]    factor   subsampling factor; >= 1; ignored if colormapped
 * \param[in]    color    L_SELECT_RED, L_SELECT_GREEN or L_SELECT_BLUE
 * \param[out]   pminval  [optional] minimum value of component
 * \param[out]   pmaxval  [optional] maximum value of component
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) If pixs is 8 bpp grayscale, the color selection type is ignored.
 * </pre>
 */
l_ok
pixGetRangeValues(PIX      *pixs,
                  l_int32   factor,
                  l_int32   color,
                  l_int32  *pminval,
                  l_int32  *pmaxval)
{
l_int32   d;
PIXCMAP  *cmap;

    PROCNAME("pixGetRangeValues");

    if (pminval) *pminval = 0;
    if (pmaxval) *pmaxval = 0;
    if (!pminval && !pmaxval)
        return ERROR_INT("no result requested", procName, 1);
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);

    cmap = pixGetColormap(pixs);
    if (cmap)
        return pixcmapGetRangeValues(cmap, color, pminval, pmaxval,
                                     NULL, NULL);

    if (factor < 1)
        return ERROR_INT("sampling factor must be >= 1", procName, 1);
    d = pixGetDepth(pixs);
    if (d != 8 && d != 32)
        return ERROR_INT("pixs not 8 or 32 bpp", procName, 1);

    if (d == 8) {
        pixGetExtremeValue(pixs, factor, L_SELECT_MIN,
                           NULL, NULL, NULL, pminval);
        pixGetExtremeValue(pixs, factor, L_SELECT_MAX,
                           NULL, NULL, NULL, pmaxval);
    } else if (color == L_SELECT_RED) {
        pixGetExtremeValue(pixs, factor, L_SELECT_MIN,
                           pminval, NULL, NULL, NULL);
        pixGetExtremeValue(pixs, factor, L_SELECT_MAX,
                           pmaxval, NULL, NULL, NULL);
    } else if (color == L_SELECT_GREEN) {
        pixGetExtremeValue(pixs, factor, L_SELECT_MIN,
                           NULL, pminval, NULL, NULL);
        pixGetExtremeValue(pixs, factor, L_SELECT_MAX,
                           NULL, pmaxval, NULL, NULL);
    } else if (color == L_SELECT_BLUE) {
        pixGetExtremeValue(pixs, factor, L_SELECT_MIN,
                           NULL, NULL, pminval, NULL);
        pixGetExtremeValue(pixs, factor, L_SELECT_MAX,
                           NULL, NULL, pmaxval, NULL);
    } else {
        return ERROR_INT("invalid color", procName, 1);
    }

    return 0;
}


/*!
 * \brief   pixGetExtremeValue()
 *
 * \param[in]    pixs      8 bpp grayscale, 32 bpp rgb, or colormapped
 * \param[in]    factor    subsampling factor; >= 1; ignored if colormapped
 * \param[in]    type      L_SELECT_MIN or L_SELECT_MAX
 * \param[out]   prval     [optional] red component
 * \param[out]   pgval     [optional] green component
 * \param[out]   pbval     [optional] blue component
 * \param[out]   pgrayval  [optional] min or max gray value
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) If pixs is grayscale, the result is returned in &grayval.
 *          Otherwise, if there is a colormap or d == 32,
 *          each requested color component is returned.  At least
 *          one color component (address) must be input.
 * </pre>
 */
l_ok
pixGetExtremeValue(PIX      *pixs,
                   l_int32   factor,
                   l_int32   type,
                   l_int32  *prval,
                   l_int32  *pgval,
                   l_int32  *pbval,
                   l_int32  *pgrayval)
{
l_int32    i, j, w, h, d, wpl;
l_int32    val, extval, rval, gval, bval, extrval, extgval, extbval;
l_uint32   pixel;
l_uint32  *data, *line;
PIXCMAP   *cmap;

    PROCNAME("pixGetExtremeValue");

    if (prval) *prval = -1;
    if (pgval) *pgval = -1;
    if (pbval) *pbval = -1;
    if (pgrayval) *pgrayval = -1;
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (type != L_SELECT_MIN && type != L_SELECT_MAX)
        return ERROR_INT("invalid type", procName, 1);

    cmap = pixGetColormap(pixs);
    if (cmap) {
        if (type == L_SELECT_MIN) {
            if (prval) pixcmapGetRangeValues(cmap, L_SELECT_RED, prval, NULL,
                                             NULL, NULL);
            if (pgval) pixcmapGetRangeValues(cmap, L_SELECT_GREEN, pgval, NULL,
                                             NULL, NULL);
            if (pbval) pixcmapGetRangeValues(cmap, L_SELECT_BLUE, pbval, NULL,
                                             NULL, NULL);
        } else {  /* type == L_SELECT_MAX */
            if (prval) pixcmapGetRangeValues(cmap, L_SELECT_RED, NULL, prval,
                                             NULL, NULL);
            if (pgval) pixcmapGetRangeValues(cmap, L_SELECT_GREEN, NULL, pgval,
                                             NULL, NULL);
            if (pbval) pixcmapGetRangeValues(cmap, L_SELECT_BLUE, NULL, pbval,
                                             NULL, NULL);
        }
        return 0;
    }

    pixGetDimensions(pixs, &w, &h, &d);
    if (factor < 1)
        return ERROR_INT("sampling factor must be >= 1", procName, 1);
    if (d != 8 && d != 32)
        return ERROR_INT("pixs not 8 or 32 bpp", procName, 1);
    if (d == 8 && !pgrayval)
        return ERROR_INT("can't return result in grayval", procName, 1);
    if (d == 32 && !prval && !pgval && !pbval)
        return ERROR_INT("can't return result in r/g/b-val", procName, 1);

    data = pixGetData(pixs);
    wpl = pixGetWpl(pixs);
    if (d == 8) {
        if (type == L_SELECT_MIN)
            extval = 100000;
        else  /* get max */
            extval = -1;

        for (i = 0; i < h; i += factor) {
            line = data + i * wpl;
            for (j = 0; j < w; j += factor) {
                val = GET_DATA_BYTE(line, j);
                if ((type == L_SELECT_MIN && val < extval) ||
                    (type == L_SELECT_MAX && val > extval))
                    extval = val;
            }
        }
        *pgrayval = extval;
        return 0;
    }

        /* 32 bpp rgb */
    if (type == L_SELECT_MIN) {
        extrval = 100000;
        extgval = 100000;
        extbval = 100000;
    } else {
        extrval = -1;
        extgval = -1;
        extbval = -1;
    }
    for (i = 0; i < h; i += factor) {
        line = data + i * wpl;
        for (j = 0; j < w; j += factor) {
            pixel = line[j];
            if (prval) {
                rval = (pixel >> L_RED_SHIFT) & 0xff;
                if ((type == L_SELECT_MIN && rval < extrval) ||
                    (type == L_SELECT_MAX && rval > extrval))
                    extrval = rval;
            }
            if (pgval) {
                gval = (pixel >> L_GREEN_SHIFT) & 0xff;
                if ((type == L_SELECT_MIN && gval < extgval) ||
                    (type == L_SELECT_MAX && gval > extgval))
                    extgval = gval;
            }
            if (pbval) {
                bval = (pixel >> L_BLUE_SHIFT) & 0xff;
                if ((type == L_SELECT_MIN && bval < extbval) ||
                    (type == L_SELECT_MAX && bval > extbval))
                    extbval = bval;
            }
        }
    }
    if (prval) *prval = extrval;
    if (pgval) *pgval = extgval;
    if (pbval) *pbval = extbval;
    return 0;
}


/*!
 * \brief   pixGetMaxValueInRect()
 *
 * \param[in]    pixs     8, 16 or 32 bpp grayscale; no color space components
 * \param[in]    box      [optional] region; set box = NULL to use entire pixs
 * \param[out]   pmaxval  [optional] max value in region
 * \param[out]   pxmax    [optional] x location of max value
 * \param[out]   pymax    [optional] y location of max value
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) This can be used to find the maximum and its location
 *          in a 2-dimensional histogram, where the x and y directions
 *          represent two color components (e.g., saturation and hue).
 *      (2) Note that here a 32 bpp pixs has pixel values that are simply
 *          numbers.  They are not 8 bpp components in a colorspace.
 * </pre>
 */
l_ok
pixGetMaxValueInRect(PIX       *pixs,
                     BOX       *box,
                     l_uint32  *pmaxval,
                     l_int32   *pxmax,
                     l_int32   *pymax)
{
l_int32    i, j, w, h, d, wpl, bw, bh;
l_int32    xstart, ystart, xend, yend, xmax, ymax;
l_uint32   val, maxval;
l_uint32  *data, *line;

    PROCNAME("pixGetMaxValueInRect");

    if (pmaxval) *pmaxval = 0;
    if (pxmax) *pxmax = 0;
    if (pymax) *pymax = 0;
    if (!pmaxval && !pxmax && !pymax)
        return ERROR_INT("no data requested", procName, 1);
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (pixGetColormap(pixs) != NULL)
        return ERROR_INT("pixs has colormap", procName, 1);
    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 8 && d != 16 && d != 32)
        return ERROR_INT("pixs not 8, 16 or 32 bpp", procName, 1);

    xstart = ystart = 0;
    xend = w - 1;
    yend = h - 1;
    if (box) {
        boxGetGeometry(box, &xstart, &ystart, &bw, &bh);
        xend = xstart + bw - 1;
        yend = ystart + bh - 1;
    }

    data = pixGetData(pixs);
    wpl = pixGetWpl(pixs);
    maxval = 0;
    xmax = ymax = 0;
    for (i = ystart; i <= yend; i++) {
        line = data + i * wpl;
        for (j = xstart; j <= xend; j++) {
            if (d == 8)
                val = GET_DATA_BYTE(line, j);
            else if (d == 16)
                val = GET_DATA_TWO_BYTES(line, j);
            else  /* d == 32 */
                val = line[j];
            if (val > maxval) {
                maxval = val;
                xmax = j;
                ymax = i;
            }
        }
    }
    if (maxval == 0) {  /* no counts; pick the center of the rectangle */
        xmax = (xstart + xend) / 2;
        ymax = (ystart + yend) / 2;
    }

    if (pmaxval) *pmaxval = maxval;
    if (pxmax) *pxmax = xmax;
    if (pymax) *pymax = ymax;
    return 0;
}


/*!
 * \brief   pixGetBinnedComponentRange()
 *
 * \param[in]    pixs      32 bpp rgb
 * \param[in]    nbins     number of equal population bins; must be > 1
 * \param[in]    factor    subsampling factor; >= 1
 * \param[in]    color     L_SELECT_RED, L_SELECT_GREEN or L_SELECT_BLUE
 * \param[out]   pminval   [optional] minimum value of component
 * \param[out]   pmaxval   [optional] maximum value of component
 * \param[out]   pcarray   [optional] color array of bins
 * \param[in]    fontsize  [optional] 0 for no debug; for debug, valid set
 *                         is {4,6,8,10,12,14,16,18,20}.
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) This returns the min and max average values of the
 *          selected color component in the set of rank bins,
 *          where the ranking is done using the specified component.
 * </pre>
 */
l_ok
pixGetBinnedComponentRange(PIX        *pixs,
                           l_int32     nbins,
                           l_int32     factor,
                           l_int32     color,
                           l_int32    *pminval,
                           l_int32    *pmaxval,
                           l_uint32  **pcarray,
                           l_int32     fontsize)
{
l_int32    i, minval, maxval, rval, gval, bval;
l_uint32  *carray;
PIX       *pixt;

    PROCNAME("pixGetBinnedComponentRange");

    if (pminval) *pminval = 0;
    if (pmaxval) *pmaxval = 0;
    if (pcarray) *pcarray = NULL;
    if (!pminval && !pmaxval)
        return ERROR_INT("no result requested", procName, 1);
    if (!pixs || pixGetDepth(pixs) != 32)
        return ERROR_INT("pixs not defined or not 32 bpp", procName, 1);
    if (factor < 1)
        return ERROR_INT("sampling factor must be >= 1", procName, 1);
    if (color != L_SELECT_RED && color != L_SELECT_GREEN &&
        color != L_SELECT_BLUE)
        return ERROR_INT("invalid color", procName, 1);
    if (fontsize < 0 || fontsize > 20 || fontsize & 1 || fontsize == 2)
        return ERROR_INT("invalid fontsize", procName, 1);

    pixGetRankColorArray(pixs, nbins, color, factor, &carray, 0, 0);
    if (fontsize > 0) {
        for (i = 0; i < nbins; i++)
            L_INFO("c[%d] = %x\n", procName, i, carray[i]);
        pixt = pixDisplayColorArray(carray, nbins, 200, 5, fontsize);
        pixDisplay(pixt, 100, 100);
        pixDestroy(&pixt);
    }

    extractRGBValues(carray[0], &rval, &gval, &bval);
    minval = rval;
    if (color == L_SELECT_GREEN)
        minval = gval;
    else if (color == L_SELECT_BLUE)
        minval = bval;
    extractRGBValues(carray[nbins - 1], &rval, &gval, &bval);
    maxval = rval;
    if (color == L_SELECT_GREEN)
        maxval = gval;
    else if (color == L_SELECT_BLUE)
        maxval = bval;

    if (pminval) *pminval = minval;
    if (pmaxval) *pmaxval = maxval;
    if (pcarray)
        *pcarray = carray;
    else
        LEPT_FREE(carray);
    return 0;
}


/*!
 * \brief   pixGetRankColorArray()
 *
 * \param[in]    pixs       32 bpp or cmapped
 * \param[in]    nbins      number of equal population bins; must be > 1
 * \param[in]    type       color selection flag
 * \param[in]    factor     subsampling factor; integer >= 1
 * \param[out]   pcarray    array of colors, ranked by intensity
 * \param[in]    debugflag  1 to display color squares and plots of color
 *                          components; 2 to write them as png to file
 * \param[in]    fontsize   [optional] 0 for no debug; for debug, valid set
 *                          is {4,6,8,10,12,14,16,18,20}.  Ignored if
 *                          debugflag == 0.  fontsize == 6 is typical.
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) The color selection flag is one of: L_SELECT_RED, L_SELECT_GREEN,
 *          L_SELECT_BLUE, L_SELECT_MIN, L_SELECT_MAX, L_SELECT_AVERAGE,
 *          L_SELECT_HUE, L_SELECT_SATURATION.
 *      (2) Then it finds the histogram of the selected color type in each
 *          RGB pixel.  For each of the %nbins sets of pixels,
 *          ordered by this color type value, find the average RGB color,
 *          and return this as a "rank color" array.  The output array
 *          has %nbins colors.
 *      (3) Set the subsampling factor > 1 to reduce the amount of
 *          computation.  Typically you want at least 10,000 pixels
 *          for reasonable statistics.
 *      (4) The rank color as a function of rank can then be found from
 *             rankint = (l_int32)(rank * (nbins - 1) + 0.5);
 *             extractRGBValues(array[rankint], &rval, &gval, &bval);
 *          where the rank is in [0.0 ... 1.0].
 *          This function is meant to be simple and approximate.
 *      (5) Compare this with pixGetBinnedColor(), which generates equal
 *          width intensity bins and finds the average color in each bin.
 * </pre>
 */
l_ok
pixGetRankColorArray(PIX        *pixs,
                     l_int32     nbins,
                     l_int32     type,
                     l_int32     factor,
                     l_uint32  **pcarray,
                     l_int32     debugflag,
                     l_int32     fontsize)
{
l_int32    ret;
l_uint32  *array;
NUMA      *na, *nan, *narbin;
PIX       *pixt, *pixc, *pixg, *pixd;
PIXCMAP   *cmap;

    PROCNAME("pixGetRankColorArray");

    if (!pcarray)
        return ERROR_INT("&carray not defined", procName, 1);
    *pcarray = NULL;
    if (factor < 1)
        return ERROR_INT("sampling factor must be >= 1", procName, 1);
    if (nbins < 2)
        return ERROR_INT("nbins must be at least 2", procName, 1);
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    cmap = pixGetColormap(pixs);
    if (pixGetDepth(pixs) != 32 && !cmap)
        return ERROR_INT("pixs neither 32 bpp nor cmapped", procName, 1);
    if (type != L_SELECT_RED && type != L_SELECT_GREEN &&
        type != L_SELECT_BLUE && type != L_SELECT_MIN &&
        type != L_SELECT_MAX && type != L_SELECT_AVERAGE &&
        type != L_SELECT_HUE && type != L_SELECT_SATURATION)
        return ERROR_INT("invalid type", procName, 1);
    if (debugflag > 0) {
        if (fontsize < 0 || fontsize > 20 || fontsize & 1 || fontsize == 2)
            return ERROR_INT("invalid fontsize", procName, 1);
    }

        /* Downscale by factor and remove colormap if it exists */
    pixt = pixScaleByIntSampling(pixs, factor);
    if (cmap)
        pixc = pixRemoveColormap(pixt, REMOVE_CMAP_TO_FULL_COLOR);
    else
        pixc = pixClone(pixt);
    pixDestroy(&pixt);

        /* Get normalized histogram of the selected component */
    if (type == L_SELECT_RED)
        pixg = pixGetRGBComponent(pixc, COLOR_RED);
    else if (type == L_SELECT_GREEN)
        pixg = pixGetRGBComponent(pixc, COLOR_GREEN);
    else if (type == L_SELECT_BLUE)
        pixg = pixGetRGBComponent(pixc, COLOR_BLUE);
    else if (type == L_SELECT_MIN)
        pixg = pixConvertRGBToGrayMinMax(pixc, L_CHOOSE_MIN);
    else if (type == L_SELECT_MAX)
        pixg = pixConvertRGBToGrayMinMax(pixc, L_CHOOSE_MAX);
    else if (type == L_SELECT_AVERAGE)
        pixg = pixConvertRGBToGray(pixc, 0.34, 0.33, 0.33);
    else if (type == L_SELECT_HUE)
        pixg = pixConvertRGBToHue(pixc);
    else  /* L_SELECT_SATURATION */
        pixg = pixConvertRGBToSaturation(pixc);
    if ((na = pixGetGrayHistogram(pixg, 1)) == NULL) {
        pixDestroy(&pixc);
        pixDestroy(&pixg);
        return ERROR_INT("na not made", procName, 1);
    }
    nan = numaNormalizeHistogram(na, 1.0);

        /* Get the following arrays:
         * (1) nar: cumulative normalized histogram (rank vs intensity value).
         *     With 256 intensity values, we have 257 rank values.
         * (2) nai: "average" intensity as function of rank bin, for
         *     %nbins equally spaced in rank between 0.0 and 1.0.
         * (3) narbin: bin number of discretized rank as a function of
         *     intensity.  This is the 'inverse' of nai.
         * (4) nabb: intensity value of the right bin boundary, for each
         *     of the %nbins discretized rank bins. */
    if (!debugflag) {
        numaDiscretizeRankAndIntensity(nan, nbins, &narbin, NULL, NULL, NULL);
    } else {
        NUMA  *nai, *nar, *nabb;
        numaDiscretizeRankAndIntensity(nan, nbins, &narbin, &nai, &nar, &nabb);
        lept_mkdir("lept/regout");
        gplotSimple1(nan, GPLOT_PNG, "/tmp/lept/regout/rtnan",
                     "Normalized Histogram");
        gplotSimple1(nar, GPLOT_PNG, "/tmp/lept/regout/rtnar",
                     "Cumulative Histogram");
        gplotSimple1(nai, GPLOT_PNG, "/tmp/lept/regout/rtnai",
                     "Intensity vs. rank bin");
        gplotSimple1(narbin, GPLOT_PNG, "/tmp/lept/regout/rtnarbin",
                     "LUT: rank bin vs. Intensity");
        gplotSimple1(nabb, GPLOT_PNG, "/tmp/lept/regout/rtnabb",
                     "Intensity of right edge vs. rank bin");
        numaDestroy(&nai);
        numaDestroy(&nar);
        numaDestroy(&nabb);
    }

        /* Get the average color in each bin for pixels whose grayscale
         * values fall in the bin range.  %narbin is the LUT that
         * determines the bin number from the grayscale version of
         * the image.  Because this mapping may not be unique,
         * some bins may not be represented in the LUT. In use, to get fair
         * allocation into all the bins, bin population is monitored
         * as pixels are accumulated, and when bins fill up,
         * pixels are required to overflow into succeeding bins. */
    pixGetBinnedColor(pixc, pixg, 1, nbins, narbin, pcarray, debugflag);
    ret = 0;
    if ((array = *pcarray) == NULL) {
        L_ERROR("color array not returned\n", procName);
        ret = 1;
        debugflag = 0;  /* make sure to skip the following */
    }
    if (debugflag) {
        pixd = pixDisplayColorArray(array, nbins, 200, 5, fontsize);
        if (debugflag == 1)
            pixDisplayWithTitle(pixd, 0, 500, "binned colors", 1);
        else  /* debugflag == 2 */
            pixWriteDebug("/tmp/lept/regout/rankhisto.png", pixd, IFF_PNG);
        pixDestroy(&pixd);
    }

    pixDestroy(&pixc);
    pixDestroy(&pixg);
    numaDestroy(&na);
    numaDestroy(&nan);
    numaDestroy(&narbin);
    return ret;
}


/*!
 * \brief   pixGetBinnedColor()
 *
 * \param[in]    pixs       32 bpp
 * \param[in]    pixg       8 bpp grayscale version of pixs
 * \param[in]    factor     sampling factor along pixel counting direction
 * \param[in]    nbins      number of intensity bins
 * \param[in]    nalut      LUT for mapping from intensity to bin number
 * \param[out]   pcarray    array of average color values in each bin
 * \param[in]    debugflag  1 to display output debug plots of color
 *                          components; 2 to write them as png to file
 * \return  0 if OK; 1 on error
 *
 * <pre>
 * Notes:
 *      (1) This takes a color image, a grayscale (intensity) version,
 *          a LUT from intensity to bin number, and the number of bins.
 *          It computes the average color for pixels whose intensity
 *          is in each bin.  This is returned as an array of l_uint32
 *          colors in our standard RGBA ordering.
 *      (2) This function generates equal width intensity bins and
 *          finds the average color in each bin.  Compare this with
 *          pixGetRankColorArray(), which rank orders the pixels
 *          by the value of the selected component in each pixel,
 *          sets up bins with equal population (not intensity width!),
 *          and gets the average color in each bin.
 * </pre>
 */
l_ok
pixGetBinnedColor(PIX        *pixs,
                  PIX        *pixg,
                  l_int32     factor,
                  l_int32     nbins,
                  NUMA       *nalut,
                  l_uint32  **pcarray,
                  l_int32     debugflag)
{
l_int32     i, j, w, h, wpls, wplg, grayval, bin, rval, gval, bval, success;
l_int32     npts, avepts, maxpts;
l_uint32   *datas, *datag, *lines, *lineg, *carray;
l_float64   norm;
l_float64  *rarray, *garray, *barray, *narray;

    PROCNAME("pixGetBinnedColor");

    if (!pcarray)
        return ERROR_INT("&carray not defined", procName, 1);
    *pcarray = NULL;
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (!pixg)
        return ERROR_INT("pixg not defined", procName, 1);
    if (!nalut)
        return ERROR_INT("nalut not defined", procName, 1);
    if (factor < 1) {
        L_WARNING("sampling factor less than 1; setting to 1\n", procName);
        factor = 1;
    }

        /* Find the color for each rank bin.  Note that we can have
         * multiple bins filled with pixels having the same gray value.
         * Therefore, because in general the mapping from gray value
         * to bin number is not unique, if a bin fills up (actually,
         * we allow it to slightly overfill), we roll the excess
         * over to the next bin, etc. */
    pixGetDimensions(pixs, &w, &h, NULL);
    npts = (w + factor - 1) * (h + factor - 1) / (factor * factor);
    avepts = (npts + nbins - 1) / nbins;  /* average number of pts in a bin */
    maxpts = (l_int32)((1.0 + 0.5 / (l_float32)nbins) * avepts);
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    datag = pixGetData(pixg);
    wplg = pixGetWpl(pixg);
    rarray = (l_float64 *)LEPT_CALLOC(nbins, sizeof(l_float64));
    garray = (l_float64 *)LEPT_CALLOC(nbins, sizeof(l_float64));
    barray = (l_float64 *)LEPT_CALLOC(nbins, sizeof(l_float64));
    narray = (l_float64 *)LEPT_CALLOC(nbins, sizeof(l_float64));
    for (i = 0; i < h; i += factor) {
        lines = datas + i * wpls;
        lineg = datag + i * wplg;
        for (j = 0; j < w; j += factor) {
            grayval = GET_DATA_BYTE(lineg, j);
            numaGetIValue(nalut, grayval, &bin);
            extractRGBValues(lines[j], &rval, &gval, &bval);
            while (narray[bin] >= maxpts && bin < nbins - 1)
                bin++;
            rarray[bin] += rval;
            garray[bin] += gval;
            barray[bin] += bval;
            narray[bin] += 1.0;  /* count samples in each bin */
        }
    }

    for (i = 0; i < nbins; i++) {
        norm = 1. / narray[i];
        rarray[i] *= norm;
        garray[i] *= norm;
        barray[i] *= norm;
/*        fprintf(stderr, "narray[%d] = %f\n", i, narray[i]);  */
    }

    if (debugflag) {
        NUMA *nared, *nagreen, *nablue;
        nared = numaCreate(nbins);
        nagreen = numaCreate(nbins);
        nablue = numaCreate(nbins);
        for (i = 0; i < nbins; i++) {
            numaAddNumber(nared, rarray[i]);
            numaAddNumber(nagreen, garray[i]);
            numaAddNumber(nablue, barray[i]);
        }
        lept_mkdir("lept/regout");
        gplotSimple1(nared, GPLOT_PNG, "/tmp/lept/regout/rtnared",
                     "Average red val vs. rank bin");
        gplotSimple1(nagreen, GPLOT_PNG, "/tmp/lept/regout/rtnagreen",
                     "Average green val vs. rank bin");
        gplotSimple1(nablue, GPLOT_PNG, "/tmp/lept/regout/rtnablue",
                     "Average blue val vs. rank bin");
        numaDestroy(&nared);
        numaDestroy(&nagreen);
        numaDestroy(&nablue);
    }

        /* Save colors for all bins  in a single array */
    success = TRUE;
    if ((carray = (l_uint32 *)LEPT_CALLOC(nbins, sizeof(l_uint32))) == NULL) {
        success = FALSE;
        L_ERROR("carray not made\n", procName);
        goto cleanup_arrays;
    }
    *pcarray = carray;
    for (i = 0; i < nbins; i++) {
        rval = (l_int32)(rarray[i] + 0.5);
        gval = (l_int32)(garray[i] + 0.5);
        bval = (l_int32)(barray[i] + 0.5);
        composeRGBPixel(rval, gval, bval, carray + i);
    }

cleanup_arrays:
    LEPT_FREE(rarray);
    LEPT_FREE(garray);
    LEPT_FREE(barray);
    LEPT_FREE(narray);
    return (success) ? 0 : 1;
}


/*!
 * \brief   pixDisplayColorArray()
 *
 * \param[in]   carray    array of colors: 0xrrggbb00
 * \param[in]   ncolors   size of array
 * \param[in]   side      size of each color square; suggest 200
 * \param[in]   ncols     number of columns in output color matrix
 * \param[in]   fontsize  to label each square with text.  Valid set is
 *                        {4,6,8,10,12,14,16,18,20}.  Use 0 to disable.
 * \return  pixd color array, or NULL on error
 */
PIX *
pixDisplayColorArray(l_uint32  *carray,
                     l_int32    ncolors,
                     l_int32    side,
                     l_int32    ncols,
                     l_int32    fontsize)
{
char     textstr[256];
l_int32  i, rval, gval, bval;
L_BMF   *bmf;
PIX     *pixt, *pixd;
PIXA    *pixa;

    PROCNAME("pixDisplayColorArray");

    if (!carray)
        return (PIX *)ERROR_PTR("carray not defined", procName, NULL);
    if (fontsize < 0 || fontsize > 20 || fontsize & 1 || fontsize == 2)
        return (PIX *)ERROR_PTR("invalid fontsize", procName, NULL);

    bmf = (fontsize == 0) ? NULL : bmfCreate(NULL, fontsize);
    pixa = pixaCreate(ncolors);
    for (i = 0; i < ncolors; i++) {
        pixt = pixCreate(side, side, 32);
        pixSetAllArbitrary(pixt, carray[i]);
        if (bmf) {
            extractRGBValues(carray[i], &rval, &gval, &bval);
            snprintf(textstr, sizeof(textstr),
                     "%d: (%d %d %d)", i, rval, gval, bval);
            pixSaveTiledWithText(pixt, pixa, side, (i % ncols == 0) ? 1 : 0,
                                 20, 2, bmf, textstr, 0xff000000, L_ADD_BELOW);
        } else {
            pixSaveTiled(pixt, pixa, 1.0, (i % ncols == 0) ? 1 : 0, 20, 32);
        }
        pixDestroy(&pixt);
    }
    pixd = pixaDisplay(pixa, 0, 0);

    pixaDestroy(&pixa);
    bmfDestroy(&bmf);
    return pixd;
}


/*!
 * \brief   pixRankBinByStrip()
 *
 * \param[in]   pixs       32 bpp or cmapped
 * \param[in]   direction  L_SCAN_HORIZONTAL or L_SCAN_VERTICAL
 * \param[in]   size       of strips in scan direction
 * \param[in]   nbins      number of equal population bins; must be > 1
 * \param[in]   type       color selection flag
 * \return  pixd result, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) This generates a pix where each column represents a strip of
 *          the input image.  If %direction == L_SCAN_HORIZONTAL, the
 *          input impage is tiled into vertical strips of width %size,
 *          where %size is a compromise between getting better spatial
 *          columnwise resolution (small %size) and getting better
 *          columnwise statistical information (larger %size).  Likewise
 *          with rows of the image if %direction == L_SCAN_VERTICAL.
 *      (2) For L_HORIZONTAL_SCAN, the output pix contains rank binned
 *          median colors in each column that correspond to a vertical
 *          strip of width %size in the input image.
 *      (3) The color selection flag is one of: L_SELECT_RED, L_SELECT_GREEN,
 *          L_SELECT_BLUE, L_SELECT_MIN, L_SELECT_MAX, L_SELECT_AVERAGE.
 *          It determines how the rank ordering is done.
 *      (4) Typical input values might be %size = 5, %nbins = 10.
 * </pre>
 */
PIX *
pixRankBinByStrip(PIX     *pixs,
                  l_int32  direction,
                  l_int32  size,
                  l_int32  nbins,
                  l_int32  type)
{
l_int32    i, j, w, h, nstrips;
l_uint32  *array;
BOXA      *boxa;
PIX       *pix1, *pix2, *pixd;
PIXA      *pixa;
PIXCMAP   *cmap;

    PROCNAME("pixRankBinByStrip");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    cmap = pixGetColormap(pixs);
    if (pixGetDepth(pixs) != 32 && !cmap)
        return (PIX *)ERROR_PTR("pixs neither 32 bpp nor cmapped",
                                procName, NULL);
    if (direction != L_SCAN_HORIZONTAL && direction != L_SCAN_VERTICAL)
        return (PIX *)ERROR_PTR("invalid direction", procName, NULL);
    if (size < 1)
        return (PIX *)ERROR_PTR("size < 1", procName, NULL);
    if (nbins < 2)
        return (PIX *)ERROR_PTR("nbins must be at least 2", procName, NULL);
    if (type != L_SELECT_RED && type != L_SELECT_GREEN &&
        type != L_SELECT_BLUE && type != L_SELECT_MIN &&
        type != L_SELECT_MAX && type != L_SELECT_AVERAGE)
        return (PIX *)ERROR_PTR("invalid type", procName, NULL);

        /* Downscale by factor and remove colormap if it exists */
    if (cmap)
        pix1 = pixRemoveColormap(pixs, REMOVE_CMAP_TO_FULL_COLOR);
    else
        pix1 = pixClone(pixs);
    pixGetDimensions(pixs, &w, &h, NULL);

    pixd = NULL;
    boxa = makeMosaicStrips(w, h, direction, size);
    pixa = pixClipRectangles(pix1, boxa);
    nstrips = pixaGetCount(pixa);
    if (direction == L_SCAN_HORIZONTAL) {
        pixd = pixCreate(nstrips, nbins, 32);
        for (i = 0; i < nstrips; i++) {
            pix2 = pixaGetPix(pixa, i, L_CLONE);
            pixGetRankColorArray(pix2, nbins, type, 1, &array, 0, 0);
            for (j = 0; j < nbins; j++)
                pixSetPixel(pixd, i, j, array[j]);
            LEPT_FREE(array);
            pixDestroy(&pix2);
        }
    } else {  /* L_SCAN_VERTICAL */
        pixd = pixCreate(nbins, nstrips, 32);
        for (i = 0; i < nstrips; i++) {
            pix2 = pixaGetPix(pixa, i, L_CLONE);
            pixGetRankColorArray(pix2, nbins, type, 1, &array, 0, 0);
            for (j = 0; j < nbins; j++)
                pixSetPixel(pixd, j, i, array[j]);
            LEPT_FREE(array);
            pixDestroy(&pix2);
        }
    }
    pixDestroy(&pix1);
    boxaDestroy(&boxa);
    pixaDestroy(&pixa);
    return pixd;
}



/*-------------------------------------------------------------*
 *                 Pixelwise aligned statistics                *
 *-------------------------------------------------------------*/
/*!
 * \brief   pixaGetAlignedStats()
 *
 * \param[in]   pixa    of identically sized, 8 bpp pix; not cmapped
 * \param[in]   type    L_MEAN_ABSVAL, L_MEDIAN_VAL, L_MODE_VAL, L_MODE_COUNT
 * \param[in]   nbins   of histogram for median and mode; ignored for mean
 * \param[in]   thresh  on histogram for mode val; ignored for all other types
 * \return  pix with pixelwise aligned stats, or NULL on error.
 *
 * <pre>
 * Notes:
 *      (1) Each pixel in the returned pix represents an average
 *          (or median, or mode) over the corresponding pixels in each
 *          pix in the pixa.
 *      (2) The %thresh parameter works with L_MODE_VAL only, and
 *          sets a minimum occupancy of the mode bin.
 *          If the occupancy of the mode bin is less than %thresh, the
 *          mode value is returned as 0.  To always return the actual
 *          mode value, set %thresh = 0.  See pixGetRowStats().
 * </pre>
 */
PIX *
pixaGetAlignedStats(PIXA     *pixa,
                    l_int32   type,
                    l_int32   nbins,
                    l_int32   thresh)
{
l_int32     j, n, w, h, d;
l_float32  *colvect;
PIX        *pixt, *pixd;

    PROCNAME("pixaGetAlignedStats");

    if (!pixa)
        return (PIX *)ERROR_PTR("pixa not defined", procName, NULL);
    if (type != L_MEAN_ABSVAL && type != L_MEDIAN_VAL &&
        type != L_MODE_VAL && type != L_MODE_COUNT)
        return (PIX *)ERROR_PTR("invalid type", procName, NULL);
    n = pixaGetCount(pixa);
    if (n == 0)
        return (PIX *)ERROR_PTR("no pix in pixa", procName, NULL);
    pixaGetPixDimensions(pixa, 0, &w, &h, &d);
    if (d != 8)
        return (PIX *)ERROR_PTR("pix not 8 bpp", procName, NULL);

    pixd = pixCreate(w, h, 8);
    pixt = pixCreate(n, h, 8);
    colvect = (l_float32 *)LEPT_CALLOC(h, sizeof(l_float32));
    for (j = 0; j < w; j++) {
        pixaExtractColumnFromEachPix(pixa, j, pixt);
        pixGetRowStats(pixt, type, nbins, thresh, colvect);
        pixSetPixelColumn(pixd, j, colvect);
    }

    LEPT_FREE(colvect);
    pixDestroy(&pixt);
    return pixd;
}


/*!
 * \brief   pixaExtractColumnFromEachPix()
 *
 * \param[in]   pixa   of identically sized, 8 bpp; not cmapped
 * \param[in]   col    column index
 * \param[in]   pixd   pix into which each column is inserted
 * \return  0 if OK, 1 on error
 */
l_ok
pixaExtractColumnFromEachPix(PIXA    *pixa,
                             l_int32  col,
                             PIX     *pixd)
{
l_int32    i, k, n, w, h, ht, val, wplt, wpld;
l_uint32  *datad, *datat;
PIX       *pixt;

    PROCNAME("pixaExtractColumnFromEachPix");

    if (!pixa)
        return ERROR_INT("pixa not defined", procName, 1);
    if (!pixd || pixGetDepth(pixd) != 8)
        return ERROR_INT("pixd not defined or not 8 bpp", procName, 1);
    n = pixaGetCount(pixa);
    pixGetDimensions(pixd, &w, &h, NULL);
    if (n != w)
        return ERROR_INT("pix width != n", procName, 1);
    pixt = pixaGetPix(pixa, 0, L_CLONE);
    wplt = pixGetWpl(pixt);
    pixGetDimensions(pixt, NULL, &ht, NULL);
    pixDestroy(&pixt);
    if (h != ht)
        return ERROR_INT("pixd height != column height", procName, 1);

    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);
    for (k = 0; k < n; k++) {
        pixt = pixaGetPix(pixa, k, L_CLONE);
        datat = pixGetData(pixt);
        for (i = 0; i < h; i++) {
            val = GET_DATA_BYTE(datat, col);
            SET_DATA_BYTE(datad + i * wpld, k, val);
            datat += wplt;
        }
        pixDestroy(&pixt);
    }

    return 0;
}


/*!
 * \brief   pixGetRowStats()
 *
 * \param[in]   pixs     8 bpp; not cmapped
 * \param[in]   type     L_MEAN_ABSVAL, L_MEDIAN_VAL, L_MODE_VAL, L_MODE_COUNT
 * \param[in]   nbins    of histogram for median and mode; ignored for mean
 * \param[in]   thresh   on histogram for mode; ignored for mean and median
 * \param[in]   colvect  vector of results gathered across the rows of pixs
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) This computes a column vector of statistics using each
 *          row of a Pix.  The result is put in %colvect.
 *      (2) The %thresh parameter works with L_MODE_VAL only, and
 *          sets a minimum occupancy of the mode bin.
 *          If the occupancy of the mode bin is less than %thresh, the
 *          mode value is returned as 0.  To always return the actual
 *          mode value, set %thresh = 0.
 *      (3) What is the meaning of this %thresh parameter?
 *          For each row, the total count in the histogram is w, the
 *          image width.  So %thresh, relative to w, gives a measure
 *          of the ratio of the bin width to the width of the distribution.
 *          The larger %thresh, the narrower the distribution must be
 *          for the mode value to be returned (instead of returning 0).
 *      (4) If the Pix consists of a set of corresponding columns,
 *          one for each Pix in a Pixa, the width of the Pix is the
 *          number of Pix in the Pixa and the column vector can
 *          be stored as a column in a Pix of the same size as
 *          each Pix in the Pixa.
 * </pre>
 */
l_ok
pixGetRowStats(PIX        *pixs,
               l_int32     type,
               l_int32     nbins,
               l_int32     thresh,
               l_float32  *colvect)
{
l_int32    i, j, k, w, h, val, wpls, sum, target, max, modeval;
l_int32   *histo, *gray2bin, *bin2gray;
l_uint32  *lines, *datas;

    PROCNAME("pixGetRowStats");

    if (!pixs || pixGetDepth(pixs) != 8)
        return ERROR_INT("pixs not defined or not 8 bpp", procName, 1);
    if (!colvect)
        return ERROR_INT("colvect not defined", procName, 1);
    if (type != L_MEAN_ABSVAL && type != L_MEDIAN_VAL &&
        type != L_MODE_VAL && type != L_MODE_COUNT)
        return ERROR_INT("invalid type", procName, 1);
    if (type != L_MEAN_ABSVAL && (nbins < 1 || nbins > 256))
        return ERROR_INT("invalid nbins", procName, 1);
    pixGetDimensions(pixs, &w, &h, NULL);

    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    if (type == L_MEAN_ABSVAL) {
        for (i = 0; i < h; i++) {
            sum = 0;
            lines = datas + i * wpls;
            for (j = 0; j < w; j++)
                sum += GET_DATA_BYTE(lines, j);
            colvect[i] = (l_float32)sum / (l_float32)w;
        }
        return 0;
    }

        /* We need a histogram; binwidth ~ 256 / nbins */
    histo = (l_int32 *)LEPT_CALLOC(nbins, sizeof(l_int32));
    gray2bin = (l_int32 *)LEPT_CALLOC(256, sizeof(l_int32));
    bin2gray = (l_int32 *)LEPT_CALLOC(nbins, sizeof(l_int32));
    for (i = 0; i < 256; i++)  /* gray value --> histo bin */
        gray2bin[i] = (i * nbins) / 256;
    for (i = 0; i < nbins; i++)  /* histo bin --> gray value */
        bin2gray[i] = (i * 256 + 128) / nbins;

    for (i = 0; i < h; i++) {
        lines = datas + i * wpls;
        for (k = 0; k < nbins; k++)
            histo[k] = 0;
        for (j = 0; j < w; j++) {
            val = GET_DATA_BYTE(lines, j);
            histo[gray2bin[val]]++;
        }

        if (type == L_MEDIAN_VAL) {
            sum = 0;
            target = (w + 1) / 2;
            for (k = 0; k < nbins; k++) {
                sum += histo[k];
                if (sum >= target) {
                    colvect[i] = bin2gray[k];
                    break;
                }
            }
        } else if (type == L_MODE_VAL) {
            max = 0;
            modeval = 0;
            for (k = 0; k < nbins; k++) {
                if (histo[k] > max) {
                    max = histo[k];
                    modeval = k;
                }
            }
            if (max < thresh)
                colvect[i] = 0;
            else
                colvect[i] = bin2gray[modeval];
        } else {  /* type == L_MODE_COUNT */
            max = 0;
            for (k = 0; k < nbins; k++) {
                if (histo[k] > max)
                    max = histo[k];
            }
            colvect[i] = max;
        }
    }

    LEPT_FREE(histo);
    LEPT_FREE(gray2bin);
    LEPT_FREE(bin2gray);
    return 0;
}


/*!
 * \brief   pixGetColumnStats()
 *
 * \param[in]   pixs     8 bpp; not cmapped
 * \param[in]   type     L_MEAN_ABSVAL, L_MEDIAN_VAL, L_MODE_VAL, L_MODE_COUNT
 * \param[in]   nbins    of histogram for median and mode; ignored for mean
 * \param[in]   thresh   on histogram for mode val; ignored for all other types
 * \param[in]   rowvect  vector of results gathered down the columns of pixs
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) This computes a row vector of statistics using each
 *          column of a Pix.  The result is put in %rowvect.
 *      (2) The %thresh parameter works with L_MODE_VAL only, and
 *          sets a minimum occupancy of the mode bin.
 *          If the occupancy of the mode bin is less than %thresh, the
 *          mode value is returned as 0.  To always return the actual
 *          mode value, set %thresh = 0.
 *      (3) What is the meaning of this %thresh parameter?
 *          For each column, the total count in the histogram is h, the
 *          image height.  So %thresh, relative to h, gives a measure
 *          of the ratio of the bin width to the width of the distribution.
 *          The larger %thresh, the narrower the distribution must be
 *          for the mode value to be returned (instead of returning 0).
 * </pre>
 */
l_ok
pixGetColumnStats(PIX        *pixs,
                  l_int32     type,
                  l_int32     nbins,
                  l_int32     thresh,
                  l_float32  *rowvect)
{
l_int32    i, j, k, w, h, val, wpls, sum, target, max, modeval;
l_int32   *histo, *gray2bin, *bin2gray;
l_uint32  *datas;

    PROCNAME("pixGetColumnStats");

    if (!pixs || pixGetDepth(pixs) != 8)
        return ERROR_INT("pixs not defined or not 8 bpp", procName, 1);
    if (!rowvect)
        return ERROR_INT("rowvect not defined", procName, 1);
    if (type != L_MEAN_ABSVAL && type != L_MEDIAN_VAL &&
        type != L_MODE_VAL && type != L_MODE_COUNT)
        return ERROR_INT("invalid type", procName, 1);
    if (type != L_MEAN_ABSVAL && (nbins < 1 || nbins > 256))
        return ERROR_INT("invalid nbins", procName, 1);
    pixGetDimensions(pixs, &w, &h, NULL);

    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    if (type == L_MEAN_ABSVAL) {
        for (j = 0; j < w; j++) {
            sum = 0;
            for (i = 0; i < h; i++)
                sum += GET_DATA_BYTE(datas + i * wpls, j);
            rowvect[j] = (l_float32)sum / (l_float32)h;
        }
        return 0;
    }

        /* We need a histogram; binwidth ~ 256 / nbins */
    histo = (l_int32 *)LEPT_CALLOC(nbins, sizeof(l_int32));
    gray2bin = (l_int32 *)LEPT_CALLOC(256, sizeof(l_int32));
    bin2gray = (l_int32 *)LEPT_CALLOC(nbins, sizeof(l_int32));
    for (i = 0; i < 256; i++)  /* gray value --> histo bin */
        gray2bin[i] = (i * nbins) / 256;
    for (i = 0; i < nbins; i++)  /* histo bin --> gray value */
        bin2gray[i] = (i * 256 + 128) / nbins;

    for (j = 0; j < w; j++) {
        for (i = 0; i < h; i++) {
            val = GET_DATA_BYTE(datas + i * wpls, j);
            histo[gray2bin[val]]++;
        }

        if (type == L_MEDIAN_VAL) {
            sum = 0;
            target = (h + 1) / 2;
            for (k = 0; k < nbins; k++) {
                sum += histo[k];
                if (sum >= target) {
                    rowvect[j] = bin2gray[k];
                    break;
                }
            }
        } else if (type == L_MODE_VAL) {
            max = 0;
            modeval = 0;
            for (k = 0; k < nbins; k++) {
                if (histo[k] > max) {
                    max = histo[k];
                    modeval = k;
                }
            }
            if (max < thresh)
                rowvect[j] = 0;
            else
                rowvect[j] = bin2gray[modeval];
        } else {  /* type == L_MODE_COUNT */
            max = 0;
            for (k = 0; k < nbins; k++) {
                if (histo[k] > max)
                    max = histo[k];
            }
            rowvect[j] = max;
        }
        for (k = 0; k < nbins; k++)
            histo[k] = 0;
    }

    LEPT_FREE(histo);
    LEPT_FREE(gray2bin);
    LEPT_FREE(bin2gray);
    return 0;
}


/*!
 * \brief   pixSetPixelColumn()
 *
 * \param[in]   pix      8 bpp; not cmapped
 * \param[in]   col      column index
 * \param[in]   colvect  vector of floats
 * \return  0 if OK, 1 on error
 */
l_ok
pixSetPixelColumn(PIX        *pix,
                  l_int32     col,
                  l_float32  *colvect)
{
l_int32    i, w, h, wpl;
l_uint32  *data;

    PROCNAME("pixSetCPixelColumn");

    if (!pix || pixGetDepth(pix) != 8)
        return ERROR_INT("pix not defined or not 8 bpp", procName, 1);
    if (!colvect)
        return ERROR_INT("colvect not defined", procName, 1);
    pixGetDimensions(pix, &w, &h, NULL);
    if (col < 0 || col > w)
        return ERROR_INT("invalid col", procName, 1);

    data = pixGetData(pix);
    wpl = pixGetWpl(pix);
    for (i = 0; i < h; i++)
        SET_DATA_BYTE(data + i * wpl, col, (l_int32)colvect[i]);

    return 0;
}


/*-------------------------------------------------------------*
 *              Foreground/background estimation               *
 *-------------------------------------------------------------*/
/*!
 * \brief   pixThresholdForFgBg()
 *
 * \param[in]    pixs    any depth; cmapped ok
 * \param[in]    factor  subsampling factor; integer >= 1
 * \param[in]    thresh  threshold for generating foreground mask
 * \param[out]   pfgval  [optional] average foreground value
 * \param[out]   pbgval  [optional] average background value
 * \return  0 if OK, 1 on error
 */
l_ok
pixThresholdForFgBg(PIX      *pixs,
                    l_int32   factor,
                    l_int32   thresh,
                    l_int32  *pfgval,
                    l_int32  *pbgval)
{
l_float32  fval;
PIX       *pixg, *pixm;

    PROCNAME("pixThresholdForFgBg");

    if (pfgval) *pfgval = 0;
    if (pbgval) *pbgval = 0;
    if (!pfgval && !pbgval)
        return ERROR_INT("no data requested", procName, 1);
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);

        /* Generate a subsampled 8 bpp version and a mask over the fg */
    pixg = pixConvertTo8BySampling(pixs, factor, 0);
    pixm = pixThresholdToBinary(pixg, thresh);

    if (pfgval) {
        pixGetAverageMasked(pixg, pixm, 0, 0, 1, L_MEAN_ABSVAL, &fval);
        *pfgval = (l_int32)(fval + 0.5);
    }

    if (pbgval) {
        pixInvert(pixm, pixm);
        pixGetAverageMasked(pixg, pixm, 0, 0, 1, L_MEAN_ABSVAL, &fval);
        *pbgval = (l_int32)(fval + 0.5);
    }

    pixDestroy(&pixg);
    pixDestroy(&pixm);
    return 0;
}


/*!
 * \brief   pixSplitDistributionFgBg()
 *
 * \param[in]    pixs        any depth; cmapped ok
 * \param[in]    scorefract  fraction of the max score, used to determine
 *                           the range over which the histogram min is searched
 * \param[in]    factor      subsampling factor; integer >= 1
 * \param[out]   pthresh     [optional] best threshold for separating
 * \param[out]   pfgval      [optional] average foreground value
 * \param[out]   pbgval      [optional] average background value
 * \param[out]   ppixdb      [optional] plot of distribution and split point
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) See numaSplitDistribution() for details on the underlying
 *          method of choosing a threshold.
 * </pre>
 */
l_ok
pixSplitDistributionFgBg(PIX       *pixs,
                         l_float32  scorefract,
                         l_int32    factor,
                         l_int32   *pthresh,
                         l_int32   *pfgval,
                         l_int32   *pbgval,
                         PIX      **ppixdb)
{
char       buf[256];
l_int32    thresh;
l_float32  avefg, avebg, maxnum;
GPLOT     *gplot;
NUMA      *na, *nascore, *nax, *nay;
PIX       *pixg;

    PROCNAME("pixSplitDistributionFgBg");

    if (pthresh) *pthresh = 0;
    if (pfgval) *pfgval = 0;
    if (pbgval) *pbgval = 0;
    if (ppixdb) *ppixdb = NULL;
    if (!pthresh && !pfgval && !pbgval)
        return ERROR_INT("no data requested", procName, 1);
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);

        /* Generate a subsampled 8 bpp version */
    pixg = pixConvertTo8BySampling(pixs, factor, 0);

        /* Make the fg/bg estimates */
    na = pixGetGrayHistogram(pixg, 1);
    if (ppixdb) {
        numaSplitDistribution(na, scorefract, &thresh, &avefg, &avebg,
                              NULL, NULL, &nascore);
        numaDestroy(&nascore);
    } else {
        numaSplitDistribution(na, scorefract, &thresh, &avefg, &avebg,
                              NULL, NULL, NULL);
    }

    if (pthresh) *pthresh = thresh;
    if (pfgval) *pfgval = (l_int32)(avefg + 0.5);
    if (pbgval) *pbgval = (l_int32)(avebg + 0.5);

    if (ppixdb) {
        lept_mkdir("lept/redout");
        gplot = gplotCreate("/tmp/lept/redout/histplot", GPLOT_PNG, "Histogram",
                            "Grayscale value", "Number of pixels");
        gplotAddPlot(gplot, NULL, na, GPLOT_LINES, NULL);
        nax = numaMakeConstant(thresh, 2);
        numaGetMax(na, &maxnum, NULL);
        nay = numaMakeConstant(0, 2);
        numaReplaceNumber(nay, 1, (l_int32)(0.5 * maxnum));
        snprintf(buf, sizeof(buf), "score fract = %3.1f", scorefract);
        gplotAddPlot(gplot, nax, nay, GPLOT_LINES, buf);
        gplotMakeOutput(gplot);
        gplotDestroy(&gplot);
        numaDestroy(&nax);
        numaDestroy(&nay);
        *ppixdb = pixRead("/tmp/lept/redout/histplot.png");
    }

    pixDestroy(&pixg);
    numaDestroy(&na);
    return 0;
}
