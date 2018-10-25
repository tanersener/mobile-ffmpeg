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
 * \file  pixafunc2.c
 * <pre>
 *
 *      Pixa display (render into a pix)
 *           PIX      *pixaDisplay()
 *           PIX      *pixaDisplayOnColor()
 *           PIX      *pixaDisplayRandomCmap()
 *           PIX      *pixaDisplayLinearly()
 *           PIX      *pixaDisplayOnLattice()
 *           PIX      *pixaDisplayUnsplit()
 *           PIX      *pixaDisplayTiled()
 *           PIX      *pixaDisplayTiledInRows()
 *           PIX      *pixaDisplayTiledInColumns()
 *           PIX      *pixaDisplayTiledAndScaled()
 *           PIX      *pixaDisplayTiledWithText()
 *           PIX      *pixaDisplayTiledByIndex()
 *
 *      Pixaa display (render into a pix)
 *           PIX      *pixaaDisplay()
 *           PIX      *pixaaDisplayByPixa()
 *           PIXA     *pixaaDisplayTiledAndScaled()
 *
 *      Conversion of all pix to specified type (e.g., depth)
 *           PIXA     *pixaConvertTo1()
 *           PIXA     *pixaConvertTo8()
 *           PIXA     *pixaConvertTo8Colormap()
 *           PIXA     *pixaConvertTo32()
 *
 *      Pixa constrained selection and pdf generation
 *           PIXA     *pixaConstrainedSelect()
 *           l_int32   pixaSelectToPdf()
 *
 *      Pixa display into multiple tiles
 *           PIXA     *pixaDisplayMultiTiled()
 *
 *      Split pixa into files
 *           l_int32   pixaSplitIntoFiles()
 *
 *      Tile N-Up
 *           l_int32   convertToNUpFiles()
 *           PIXA     *convertToNUpPixa()
 *           PIXA     *pixaConvertToNUpPixa()
 *
 *      Render two pixa side-by-side for comparison                   *
 *           l_int32   pixaCompareInPdf()
 *
 *  We give twelve pixaDisplay*() methods for tiling a pixa in a pix.
 *  Some work for 1 bpp input; others for any input depth.
 *  Some give an output depth that depends on the input depth;
 *  others give a different output depth or allow you to choose it.
 *  Some use a boxes to determine where each pix goes; others tile
 *  onto a regular lattice; others tile onto an irregular lattice;
 *  one uses an associated index array to determine which column
 *  each pix goes into.
 *
 *  Here is a brief description of what the pixa display functions do.
 *
 *    pixaDisplay()
 *        This uses the boxes in the pixa to lay out each pix.  This
 *        can be used to reconstruct a pix that has been broken into
 *        components, if the boxes represents the positions of the
 *        components in the original image.
 *    pixaDisplayOnColor()
 *        pixaDisplay() with choice of background color.
 *    pixaDisplayRandomCmap()
 *        This also uses the boxes to lay out each pix.  However, it creates
 *        a colormapped dest, where each 1 bpp pix is given a randomly
 *        generated color (up to 256 are used).
 *    pixaDisplayLinearly()
 *        This puts each pix, sequentially, in a line, either horizontally
 *        or vertically.
 *    pixaDisplayOnLattice()
 *        This puts each pix, sequentially, onto a regular lattice,
 *        omitting any pix that are too big for the lattice size.
 *        This is useful, for example, to store bitmapped fonts,
 *        where all the characters are stored in a single image.
 *    pixaDisplayUnsplit()
 *        This lays out a mosaic of tiles (the pix in the pixa) that
 *        are all of equal size.  (Don't use this for unequal sized pix!)
 *        For example, it can be used to invert the action of
 *        pixaSplitPix().
 *    pixaDisplayTiled()
 *        Like pixaDisplayOnLattice(), this places each pix on a regular
 *        lattice, but here the lattice size is determined by the
 *        largest component, and no components are omitted.  This is
 *        dangerous if there are thousands of small components and
 *        one or more very large one, because the size of the resulting
 *        pix can be huge!
 *    pixaDisplayTiledInRows()
 *        This puts each pix down in a series of rows, where the upper
 *        edges of each pix in a row are aligned and there is a uniform
 *        spacing between the pix.  The height of each row is determined
 *        by the tallest pix that was put in the row.  This function
 *        is a reasonably efficient way to pack the subimages.
 *        A boxa of the locations of each input pix is stored in the output.
 *    pixaDisplayTiledInColumns()
 *        This puts each pix down in a series of rows, each row having
 *        a specified number of pix.  The upper edges of each pix in a
 *        row are aligned and there is a uniform spacing between the pix.
 *        The height of each row is determined by the tallest pix that
 *        was put in the row.  A boxa of the locations of each input
 *        pix is stored in the output.
 *    pixaDisplayTiledAndScaled()
 *        This scales each pix to a given width and output depth, and then
 *        tiles them in rows with a given number placed in each row.
 *        This is useful for presenting a sequence of images that can be
 *        at different resolutions, but which are derived from the same
 *        initial image.
 *    pixaDisplayTiledWithText()
 *        This is a version of pixaDisplayTiledInRows() that prints, below
 *        each pix, the text in the pix text field.  It renders a pixa
 *        to an image with white background that does not exceed a
 *        given value in width.
 *    pixaDisplayTiledByIndex()
 *        This scales each pix to a given width and output depth,
 *        and then tiles them in columns corresponding to the value
 *        in an associated numa.  All pix with the same index value are
 *        rendered in the same column.  Text in the pix text field are
 *        rendered below the pix.
 * </pre>
 */

#include <string.h>
#include <math.h>   /* for sqrt() */
#include "allheaders.h"


/*---------------------------------------------------------------------*
 *                               Pixa Display                          *
 *---------------------------------------------------------------------*/
/*!
 * \brief   pixaDisplay()
 *
 * \param[in]    pixa
 * \param[in]    w, h    if set to 0, the size is determined from the
 *                       bounding box of the components in pixa
 * \return  pix, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) This uses the boxes to place each pix in the rendered composite.
 *      (2) Set w = h = 0 to use the b.b. of the components to determine
 *          the size of the returned pix.
 *      (3) Uses the first pix in pixa to determine the depth.
 *      (4) The background is written "white".  On 1 bpp, each successive
 *          pix is "painted" (adding foreground), whereas for grayscale
 *          or color each successive pix is blitted with just the src.
 *      (5) If the pixa is empty, returns an empty 1 bpp pix.
 * </pre>
 */
PIX *
pixaDisplay(PIXA    *pixa,
            l_int32  w,
            l_int32  h)
{
l_int32  i, n, d, xb, yb, wb, hb, res;
BOXA    *boxa;
PIX     *pix1, *pixd;

    PROCNAME("pixaDisplay");

    if (!pixa)
        return (PIX *)ERROR_PTR("pixa not defined", procName, NULL);

    n = pixaGetCount(pixa);
    if (n == 0 && w == 0 && h == 0)
        return (PIX *)ERROR_PTR("no components; no size", procName, NULL);
    if (n == 0) {
        L_WARNING("no components; returning empty 1 bpp pix\n", procName);
        return pixCreate(w, h, 1);
    }

        /* If w and h not input, determine the minimum size required
         * to contain the origin and all c.c. */
    if (w == 0 || h == 0) {
        boxa = pixaGetBoxa(pixa, L_CLONE);
        boxaGetExtent(boxa, &w, &h, NULL);
        boxaDestroy(&boxa);
        if (w == 0 || h == 0)
            return (PIX *)ERROR_PTR("no associated boxa", procName, NULL);
    }

        /* Use the first pix in pixa to determine depth and resolution  */
    pix1 = pixaGetPix(pixa, 0, L_CLONE);
    d = pixGetDepth(pix1);
    res = pixGetXRes(pix1);
    pixDestroy(&pix1);

    if ((pixd = pixCreate(w, h, d)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixSetResolution(pixd, res, res);
    if (d > 1)
        pixSetAll(pixd);
    for (i = 0; i < n; i++) {
        if (pixaGetBoxGeometry(pixa, i, &xb, &yb, &wb, &hb)) {
            L_WARNING("no box found!\n", procName);
            continue;
        }
        pix1 = pixaGetPix(pixa, i, L_CLONE);
        if (d == 1)
            pixRasterop(pixd, xb, yb, wb, hb, PIX_PAINT, pix1, 0, 0);
        else
            pixRasterop(pixd, xb, yb, wb, hb, PIX_SRC, pix1, 0, 0);
        pixDestroy(&pix1);
    }

    return pixd;
}


/*!
 * \brief   pixaDisplayOnColor()
 *
 * \param[in]    pixa
 * \param[in]    w, h     if set to 0, the size is determined from the
 *                        bounding box of the components in pixa
 * \param[in]    bgcolor  background color to use
 * \return  pix, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) This uses the boxes to place each pix in the rendered composite.
 *      (2) Set w = h = 0 to use the b.b. of the components to determine
 *          the size of the returned pix.
 *      (3) If any pix in %pixa are colormapped, or if the pix have
 *          different depths, it returns a 32 bpp pix.  Otherwise,
 *          the depth of the returned pixa equals that of the pix in %pixa.
 *      (4) If the pixa is empty, return null.
 * </pre>
 */
PIX *
pixaDisplayOnColor(PIXA     *pixa,
                   l_int32   w,
                   l_int32   h,
                   l_uint32  bgcolor)
{
l_int32  i, n, xb, yb, wb, hb, hascmap, maxdepth, same, res;
BOXA    *boxa;
PIX     *pix1, *pix2, *pixd;
PIXA    *pixat;

    PROCNAME("pixaDisplayOnColor");

    if (!pixa)
        return (PIX *)ERROR_PTR("pixa not defined", procName, NULL);
    if ((n = pixaGetCount(pixa)) == 0)
        return (PIX *)ERROR_PTR("no components", procName, NULL);

        /* If w and h are not input, determine the minimum size
         * required to contain the origin and all c.c. */
    if (w == 0 || h == 0) {
        boxa = pixaGetBoxa(pixa, L_CLONE);
        boxaGetExtent(boxa, &w, &h, NULL);
        boxaDestroy(&boxa);
    }

        /* If any pix have colormaps, or if they have different depths,
         * generate rgb */
    pixaAnyColormaps(pixa, &hascmap);
    pixaGetDepthInfo(pixa, &maxdepth, &same);
    if (hascmap || !same) {
        maxdepth = 32;
        pixat = pixaCreate(n);
        for (i = 0; i < n; i++) {
            pix1 = pixaGetPix(pixa, i, L_CLONE);
            pix2 = pixConvertTo32(pix1);
            pixaAddPix(pixat, pix2, L_INSERT);
            pixDestroy(&pix1);
        }
    } else {
        pixat = pixaCopy(pixa, L_CLONE);
    }

        /* Make the output pix and set the background color */
    if ((pixd = pixCreate(w, h, maxdepth)) == NULL) {
        pixaDestroy(&pixat);
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    }
    if ((maxdepth == 1 && bgcolor > 0) ||
        (maxdepth == 2 && bgcolor >= 0x3) ||
        (maxdepth == 4 && bgcolor >= 0xf) ||
        (maxdepth == 8 && bgcolor >= 0xff) ||
        (maxdepth == 16 && bgcolor >= 0xffff) ||
        (maxdepth == 32 && bgcolor >= 0xffffff00)) {
        pixSetAll(pixd);
    } else if (bgcolor > 0) {
        pixSetAllArbitrary(pixd, bgcolor);
    }

        /* Blit each pix into its place */
    for (i = 0; i < n; i++) {
        if (pixaGetBoxGeometry(pixat, i, &xb, &yb, &wb, &hb)) {
            L_WARNING("no box found!\n", procName);
            continue;
        }
        pix1 = pixaGetPix(pixat, i, L_CLONE);
        if (i == 0) res = pixGetXRes(pix1);
        pixRasterop(pixd, xb, yb, wb, hb, PIX_SRC, pix1, 0, 0);
        pixDestroy(&pix1);
    }
    pixSetResolution(pixd, res, res);

    pixaDestroy(&pixat);
    return pixd;
}


/*!
 * \brief   pixaDisplayRandomCmap()
 *
 * \param[in]    pixa    1 bpp regions, with boxa delineating those regions
 * \param[in]    w, h    if set to 0, the size is determined from the
 *                       bounding box of the components in pixa
 * \return  pix   8 bpp, cmapped, with random colors assigned to each region,
 *                or NULL on error.
 *
 * <pre>
 * Notes:
 *      (1) This uses the boxes to place each pix in the rendered composite.
 *          The fg of each pix in %pixa, such as a single connected
 *          component or a line of text, is given a random color.
 *      (2) By default, the background color is black (cmap index 0).
 *          This can be changed by pixcmapResetColor()
 * </pre>
 */
PIX *
pixaDisplayRandomCmap(PIXA    *pixa,
                      l_int32  w,
                      l_int32  h)
{
l_int32   i, n, same, maxd, index, xb, yb, wb, hb, res;
BOXA     *boxa;
PIX      *pixs, *pix1, *pixd;
PIXCMAP  *cmap;

    PROCNAME("pixaDisplayRandomCmap");

    if (!pixa)
        return (PIX *)ERROR_PTR("pixa not defined", procName, NULL);

    if ((n = pixaGetCount(pixa)) == 0)
        return (PIX *)ERROR_PTR("no components", procName, NULL);
    pixaVerifyDepth(pixa, &same, &maxd);
    if (maxd > 1)
        return (PIX *)ERROR_PTR("not all components are 1 bpp", procName, NULL);

        /* If w and h are not input, determine the minimum size required
         * to contain the origin and all c.c. */
    if (w == 0 || h == 0) {
        boxa = pixaGetBoxa(pixa, L_CLONE);
        boxaGetExtent(boxa, &w, &h, NULL);
        boxaDestroy(&boxa);
    }

        /* Set up an 8 bpp dest pix, with a colormap with 254 random colors */
    if ((pixd = pixCreate(w, h, 8)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    cmap = pixcmapCreateRandom(8, 1, 1);
    pixSetColormap(pixd, cmap);

        /* Color each component and blit it in */
    for (i = 0; i < n; i++) {
        index = 1 + (i % 254);
        pixaGetBoxGeometry(pixa, i, &xb, &yb, &wb, &hb);
        pixs = pixaGetPix(pixa, i, L_CLONE);
        if (i == 0) res = pixGetXRes(pixs);
        pix1 = pixConvert1To8(NULL, pixs, 0, index);
        pixRasterop(pixd, xb, yb, wb, hb, PIX_PAINT, pix1, 0, 0);
        pixDestroy(&pixs);
        pixDestroy(&pix1);
    }

    pixSetResolution(pixd, res, res);
    return pixd;
}


/*!
 * \brief   pixaDisplayLinearly()
 *
 * \param[in]    pixas
 * \param[in]    direction    L_HORIZ or L_VERT
 * \param[in]    scalefactor  applied to every pix; use 1.0 for no scaling
 * \param[in]    background   0 for white, 1 for black; this is the color
 *                            of the spacing between the images
 * \param[in]    spacing      between images, and on outside
 * \param[in]    border       width of black border added to each image;
 *                            use 0 for no border
 * \param[out]   pboxa        [optional] location of images in output pix
 * \return  pix of composite images, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) This puts each pix, sequentially, in a line, either horizontally
 *          or vertically.
 *      (2) If any pix has a colormap, all pix are rendered in rgb.
 *      (3) The boxa gives the location of each image.
 * </pre>
 */
PIX *
pixaDisplayLinearly(PIXA      *pixas,
                    l_int32    direction,
                    l_float32  scalefactor,
                    l_int32    background,  /* not used */
                    l_int32    spacing,
                    l_int32    border,
                    BOXA     **pboxa)
{
l_int32  i, n, x, y, w, h, size, depth, bordval;
BOX     *box;
PIX     *pix1, *pix2, *pix3, *pixd;
PIXA    *pixa1, *pixa2;

    PROCNAME("pixaDisplayLinearly");

    if (pboxa) *pboxa = NULL;
    if (!pixas)
        return (PIX *)ERROR_PTR("pixas not defined", procName, NULL);
    if (direction != L_HORIZ && direction != L_VERT)
        return (PIX *)ERROR_PTR("invalid direction", procName, NULL);

        /* Make sure all pix are at the same depth */
    pixa1 = pixaConvertToSameDepth(pixas);
    pixaGetDepthInfo(pixa1, &depth, NULL);

        /* Scale and add border if requested */
    n = pixaGetCount(pixa1);
    pixa2 = pixaCreate(n);
    bordval = (depth == 1) ? 1 : 0;
    size = (n - 1) * spacing;
    x = y = 0;
    for (i = 0; i < n; i++) {
        if ((pix1 = pixaGetPix(pixa1, i, L_CLONE)) == NULL) {
            L_WARNING("missing pix at index %d\n", procName, i);
            continue;
        }

        if (scalefactor != 1.0)
            pix2 = pixScale(pix1, scalefactor, scalefactor);
        else
            pix2 = pixClone(pix1);
        if (border)
            pix3 = pixAddBorder(pix2, border, bordval);
        else
            pix3 = pixClone(pix2);

        pixGetDimensions(pix3, &w, &h, NULL);
        box = boxCreate(x, y, w, h);
        if (direction == L_HORIZ) {
            size += w;
            x += w + spacing;
        } else {  /* vertical */
            size += h;
            y += h + spacing;
        }
        pixaAddPix(pixa2, pix3, L_INSERT);
        pixaAddBox(pixa2, box, L_INSERT);
        pixDestroy(&pix1);
        pixDestroy(&pix2);
    }
    pixd = pixaDisplay(pixa2, 0, 0);

    if (pboxa)
        *pboxa = pixaGetBoxa(pixa2, L_COPY);
    pixaDestroy(&pixa1);
    pixaDestroy(&pixa2);
    return pixd;
}


/*!
 * \brief   pixaDisplayOnLattice()
 *
 * \param[in]    pixa
 * \param[in]    cellw    lattice cell width
 * \param[in]    cellh    lattice cell height
 * \param[out]   pncols   [optional] number of columns in output lattice
 * \param[out]   pboxa    [optional] location of images in lattice
 * \return  pix of composite images, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) This places each pix on sequentially on a regular lattice
 *          in the rendered composite.  If a pix is too large to fit in the
 *          allocated lattice space, it is not rendered.
 *      (2) If any pix has a colormap, all pix are rendered in rgb.
 *      (3) This is useful when putting bitmaps of components,
 *          such as characters, into a single image.
 *      (4) The boxa gives the location of each image.  The UL corner
 *          of each image is on a lattice cell corner.  Omitted images
 *          (due to size) are assigned an invalid width and height of 0.
 * </pre>
 */
PIX *
pixaDisplayOnLattice(PIXA     *pixa,
                     l_int32   cellw,
                     l_int32   cellh,
                     l_int32  *pncols,
                     BOXA    **pboxa)
{
l_int32  n, nw, nh, w, h, d, wt, ht, res;
l_int32  index, i, j, hascmap;
BOX     *box;
BOXA    *boxa;
PIX     *pix1, *pix2, *pixd;
PIXA    *pixa1;

    PROCNAME("pixaDisplayOnLattice");

    if (pncols) *pncols = 0;
    if (pboxa) *pboxa = NULL;
    if (!pixa)
        return (PIX *)ERROR_PTR("pixa not defined", procName, NULL);

        /* If any pix have colormaps, generate rgb */
    if ((n = pixaGetCount(pixa)) == 0)
        return (PIX *)ERROR_PTR("no components", procName, NULL);
    pix1 = pixaGetPix(pixa, 0, L_CLONE);
    res = pixGetXRes(pix1);
    pixDestroy(&pix1);
    pixaAnyColormaps(pixa, &hascmap);
    if (hascmap) {
        pixa1 = pixaCreate(n);
        for (i = 0; i < n; i++) {
            pix1 = pixaGetPix(pixa, i, L_CLONE);
            pix2 = pixConvertTo32(pix1);
            pixaAddPix(pixa1, pix2, L_INSERT);
            pixDestroy(&pix1);
        }
    } else {
        pixa1 = pixaCopy(pixa, L_CLONE);
    }
    boxa = boxaCreate(n);

        /* Have number of rows and columns approximately equal */
    nw = (l_int32)sqrt((l_float64)n);
    nh = (n + nw - 1) / nw;
    w = cellw * nw;
    h = cellh * nh;

        /* Use the first pix in pixa to determine the output depth.  */
    pixaGetPixDimensions(pixa1, 0, NULL, NULL, &d);
    if ((pixd = pixCreate(w, h, d)) == NULL) {
        pixaDestroy(&pixa1);
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    }
    pixSetBlackOrWhite(pixd, L_SET_WHITE);
    pixSetResolution(pixd, res, res);

        /* Tile the output */
    index = 0;
    for (i = 0; i < nh; i++) {
        for (j = 0; j < nw && index < n; j++, index++) {
            pix1 = pixaGetPix(pixa1, index, L_CLONE);
            pixGetDimensions(pix1, &wt, &ht, NULL);
            if (wt > cellw || ht > cellh) {
                L_INFO("pix(%d) omitted; size %dx%x\n", procName, index,
                       wt, ht);
                box = boxCreate(0, 0, 0, 0);
                boxaAddBox(boxa, box, L_INSERT);
                pixDestroy(&pix1);
                continue;
            }
            pixRasterop(pixd, j * cellw, i * cellh, wt, ht,
                        PIX_SRC, pix1, 0, 0);
            box = boxCreate(j * cellw, i * cellh, wt, ht);
            boxaAddBox(boxa, box, L_INSERT);
            pixDestroy(&pix1);
        }
    }

    if (pncols) *pncols = nw;
    if (pboxa)
        *pboxa = boxa;
    else
        boxaDestroy(&boxa);
    pixaDestroy(&pixa1);
    return pixd;
}


/*!
 * \brief   pixaDisplayUnsplit()
 *
 * \param[in]    pixa
 * \param[in]    nx           number of mosaic cells horizontally
 * \param[in]    ny           number of mosaic cells vertically
 * \param[in]    borderwidth  of added border on all sides
 * \param[in]    bordercolor  in our RGBA format: 0xrrggbbaa
 * \return  pix of tiled images, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) This is a logical inverse of pixaSplitPix().  It
 *          constructs a pix from a mosaic of tiles, all of equal size.
 *      (2) For added generality, a border of arbitrary color can
 *          be added to each of the tiles.
 *      (3) In use, pixa will typically have either been generated
 *          from pixaSplitPix() or will derived from a pixa that
 *          was so generated.
 *      (4) All pix in the pixa must be of equal depth, and, if
 *          colormapped, have the same colormap.
 * </pre>
 */
PIX *
pixaDisplayUnsplit(PIXA     *pixa,
                   l_int32   nx,
                   l_int32   ny,
                   l_int32   borderwidth,
                   l_uint32  bordercolor)
{
l_int32  w, h, d, wt, ht;
l_int32  i, j, k, x, y, n;
PIX     *pix1, *pixd;

    PROCNAME("pixaDisplayUnsplit");

    if (!pixa)
        return (PIX *)ERROR_PTR("pixa not defined", procName, NULL);
    if (nx <= 0 || ny <= 0)
        return (PIX *)ERROR_PTR("nx and ny must be > 0", procName, NULL);
    if ((n = pixaGetCount(pixa)) == 0)
        return (PIX *)ERROR_PTR("no components", procName, NULL);
    if (n != nx * ny)
        return (PIX *)ERROR_PTR("n != nx * ny", procName, NULL);
    borderwidth = L_MAX(0, borderwidth);

    pixaGetPixDimensions(pixa, 0, &wt, &ht, &d);
    w = nx * (wt + 2 * borderwidth);
    h = ny * (ht + 2 * borderwidth);

    if ((pixd = pixCreate(w, h, d)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pix1 = pixaGetPix(pixa, 0, L_CLONE);
    pixCopyColormap(pixd, pix1);
    pixDestroy(&pix1);
    if (borderwidth > 0)
        pixSetAllArbitrary(pixd, bordercolor);

    y = borderwidth;
    for (i = 0, k = 0; i < ny; i++) {
        x = borderwidth;
        for (j = 0; j < nx; j++, k++) {
            pix1 = pixaGetPix(pixa, k, L_CLONE);
            pixRasterop(pixd, x, y, wt, ht, PIX_SRC, pix1, 0, 0);
            pixDestroy(&pix1);
            x += wt + 2 * borderwidth;
        }
        y += ht + 2 * borderwidth;
    }

    return pixd;
}


/*!
 * \brief   pixaDisplayTiled()
 *
 * \param[in]    pixa
 * \param[in]    maxwidth     of output image
 * \param[in]    background   0 for white, 1 for black
 * \param[in]    spacing
 * \return  pix of tiled images, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) This renders a pixa to a single image of width not to
 *          exceed maxwidth, with background color either white or black,
 *          and with each subimage spaced on a regular lattice.
 *      (2) The lattice size is determined from the largest width and height,
 *          separately, of all pix in the pixa.
 *      (3) All pix in the pixa must be of equal depth.
 *      (4) If any pix has a colormap, all pix are rendered in rgb.
 *      (5) Careful: because no components are omitted, this is
 *          dangerous if there are thousands of small components and
 *          one or more very large one, because the size of the
 *          resulting pix can be huge!
 * </pre>
 */
PIX *
pixaDisplayTiled(PIXA    *pixa,
                 l_int32  maxwidth,
                 l_int32  background,
                 l_int32  spacing)
{
l_int32  wmax, hmax, wd, hd, d, hascmap, res, same;
l_int32  i, j, n, ni, ncols, nrows;
l_int32  ystart, xstart, wt, ht;
PIX     *pix1, *pix2, *pixd;
PIXA    *pixa1;

    PROCNAME("pixaDisplayTiled");

    if (!pixa)
        return (PIX *)ERROR_PTR("pixa not defined", procName, NULL);

        /* If any pix have colormaps, generate rgb */
    if ((n = pixaGetCount(pixa)) == 0)
        return (PIX *)ERROR_PTR("no components", procName, NULL);
    pixaAnyColormaps(pixa, &hascmap);
    if (hascmap) {
        pixa1 = pixaCreate(n);
        for (i = 0; i < n; i++) {
            pix1 = pixaGetPix(pixa, i, L_CLONE);
            pix2 = pixConvertTo32(pix1);
            pixaAddPix(pixa1, pix2, L_INSERT);
            pixDestroy(&pix1);
        }
    } else {
        pixa1 = pixaCopy(pixa, L_CLONE);
    }

        /* Find the max dimensions and depth subimages */
    pixaGetDepthInfo(pixa1, &d, &same);
    if (!same) {
        pixaDestroy(&pixa1);
        return (PIX *)ERROR_PTR("depths not equal", procName, NULL);
    }
    pixaSizeRange(pixa1, NULL, NULL, &wmax, &hmax);

        /* Get the number of rows and columns and the output image size */
    spacing = L_MAX(spacing, 0);
    ncols = (l_int32)((l_float32)(maxwidth - spacing) /
                      (l_float32)(wmax + spacing));
    ncols = L_MAX(ncols, 1);
    nrows = (n + ncols - 1) / ncols;
    wd = wmax * ncols + spacing * (ncols + 1);
    hd = hmax * nrows + spacing * (nrows + 1);
    if ((pixd = pixCreate(wd, hd, d)) == NULL) {
        pixaDestroy(&pixa1);
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    }

        /* Reset the background color if necessary */
    if ((background == 1 && d == 1) || (background == 0 && d != 1))
        pixSetAll(pixd);

        /* Blit the images to the dest */
    for (i = 0, ni = 0; i < nrows; i++) {
        ystart = spacing + i * (hmax + spacing);
        for (j = 0; j < ncols && ni < n; j++, ni++) {
            xstart = spacing + j * (wmax + spacing);
            pix1 = pixaGetPix(pixa1, ni, L_CLONE);
            if (ni == 0) res = pixGetXRes(pix1);
            pixGetDimensions(pix1, &wt, &ht, NULL);
            pixRasterop(pixd, xstart, ystart, wt, ht, PIX_SRC, pix1, 0, 0);
            pixDestroy(&pix1);
        }
    }
    pixSetResolution(pixd, res, res);

    pixaDestroy(&pixa1);
    return pixd;
}


/*!
 * \brief   pixaDisplayTiledInRows()
 *
 * \param[in]    pixa
 * \param[in]    outdepth     output depth: 1, 8 or 32 bpp
 * \param[in]    maxwidth     of output image
 * \param[in]    scalefactor  applied to every pix; use 1.0 for no scaling
 * \param[in]    background   0 for white, 1 for black; this is the color
 *                            of the spacing between the images
 * \param[in]    spacing      between images, and on outside
 * \param[in]    border       width of black border added to each image;
 *                            use 0 for no border
 * \return  pixd of tiled images, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) This renders a pixa to a single image of width not to
 *          exceed maxwidth, with background color either white or black,
 *          and with each row tiled such that the top of each pix is
 *          aligned and separated by 'spacing' from the next one.
 *          A black border can be added to each pix.
 *      (2) All pix are converted to outdepth; existing colormaps are removed.
 *      (3) This does a reasonably spacewise-efficient job of laying
 *          out the individual pix images into a tiled composite.
 *      (4) A serialized boxa giving the location in pixd of each input
 *          pix (without added border) is stored in the text string of pixd.
 *          This allows, e.g., regeneration of a pixa from pixd, using
 *          pixaCreateFromBoxa().  If there is no scaling and the depth of
 *          each input pix in the pixa is the same, this tiling operation
 *          can be inverted using the boxa (except for loss of text in
 *          each of the input pix):
 *            pix1 = pixaDisplayTiledInRows(pixa1, 1, 1500, 1.0, 0, 30, 0);
 *            char *boxatxt = pixGetText(pix1);
 *            boxa1 = boxaReadMem((l_uint8 *)boxatxt, strlen(boxatxt));
 *            pixa2 = pixaCreateFromBoxa(pix1, boxa1, NULL);
 * </pre>
 */
PIX *
pixaDisplayTiledInRows(PIXA      *pixa,
                       l_int32    outdepth,
                       l_int32    maxwidth,
                       l_float32  scalefactor,
                       l_int32    background,
                       l_int32    spacing,
                       l_int32    border)
{
l_int32   h;  /* cumulative height over all the rows */
l_int32   w;  /* cumulative height in the current row */
l_int32   bordval, wtry, wt, ht;
l_int32   irow;  /* index of current pix in current row */
l_int32   wmaxrow;  /* width of the largest row */
l_int32   maxh;  /* max height in row */
l_int32   i, j, index, n, x, y, nrows, ninrow, res;
size_t    size;
l_uint8  *data;
BOXA     *boxa;
NUMA     *nainrow;  /* number of pix in the row */
NUMA     *namaxh;  /* height of max pix in the row */
PIX      *pix, *pixn, *pix1, *pixd;
PIXA     *pixan;

    PROCNAME("pixaDisplayTiledInRows");

    if (!pixa)
        return (PIX *)ERROR_PTR("pixa not defined", procName, NULL);
    if (outdepth != 1 && outdepth != 8 && outdepth != 32)
        return (PIX *)ERROR_PTR("outdepth not in {1, 8, 32}", procName, NULL);
    if (border < 0)
        border = 0;
    if (scalefactor <= 0.0) scalefactor = 1.0;

    if ((n = pixaGetCount(pixa)) == 0)
        return (PIX *)ERROR_PTR("no components", procName, NULL);

        /* Normalize depths, scale, remove colormaps; optionally add border */
    pixan = pixaCreate(n);
    bordval = (outdepth == 1) ? 1 : 0;
    for (i = 0; i < n; i++) {
        if ((pix = pixaGetPix(pixa, i, L_CLONE)) == NULL)
            continue;

        if (outdepth == 1)
            pixn = pixConvertTo1(pix, 128);
        else if (outdepth == 8)
            pixn = pixConvertTo8(pix, FALSE);
        else  /* outdepth == 32 */
            pixn = pixConvertTo32(pix);
        pixDestroy(&pix);

        if (scalefactor != 1.0)
            pix1 = pixScale(pixn, scalefactor, scalefactor);
        else
            pix1 = pixClone(pixn);
        if (border)
            pixd = pixAddBorder(pix1, border, bordval);
        else
            pixd = pixClone(pix1);
        pixDestroy(&pixn);
        pixDestroy(&pix1);

        pixaAddPix(pixan, pixd, L_INSERT);
    }
    if (pixaGetCount(pixan) != n) {
        n = pixaGetCount(pixan);
        L_WARNING("only got %d components\n", procName, n);
        if (n == 0) {
            pixaDestroy(&pixan);
            return (PIX *)ERROR_PTR("no components", procName, NULL);
        }
    }

        /* Compute parameters for layout */
    nainrow = numaCreate(0);
    namaxh = numaCreate(0);
    wmaxrow = 0;
    w = h = spacing;
    maxh = 0;  /* max height in row */
    for (i = 0, irow = 0; i < n; i++, irow++) {
        pixaGetPixDimensions(pixan, i, &wt, &ht, NULL);
        wtry = w + wt + spacing;
        if (wtry > maxwidth) {  /* end the current row and start next one */
            numaAddNumber(nainrow, irow);
            numaAddNumber(namaxh, maxh);
            wmaxrow = L_MAX(wmaxrow, w);
            h += maxh + spacing;
            irow = 0;
            w = wt + 2 * spacing;
            maxh = ht;
        } else {
            w = wtry;
            maxh = L_MAX(maxh, ht);
        }
    }

        /* Enter the parameters for the last row */
    numaAddNumber(nainrow, irow);
    numaAddNumber(namaxh, maxh);
    wmaxrow = L_MAX(wmaxrow, w);
    h += maxh + spacing;

    if ((pixd = pixCreate(wmaxrow, h, outdepth)) == NULL) {
        numaDestroy(&nainrow);
        numaDestroy(&namaxh);
        pixaDestroy(&pixan);
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    }

        /* Reset the background color if necessary */
    if ((background == 1 && outdepth == 1) ||
        (background == 0 && outdepth != 1))
        pixSetAll(pixd);

        /* Blit the images to the dest, and save the boxa identifying
         * the image regions that do not include the borders. */
    nrows = numaGetCount(nainrow);
    y = spacing;
    boxa = boxaCreate(n);
    for (i = 0, index = 0; i < nrows; i++) {  /* over rows */
        numaGetIValue(nainrow, i, &ninrow);
        numaGetIValue(namaxh, i, &maxh);
        x = spacing;
        for (j = 0; j < ninrow; j++, index++) {   /* over pix in row */
            pix = pixaGetPix(pixan, index, L_CLONE);
            if (index == 0) {
                res = pixGetXRes(pix);
                pixSetResolution(pixd, res, res);
            }
            pixGetDimensions(pix, &wt, &ht, NULL);
            boxaAddBox(boxa, boxCreate(x + border, y + border,
                wt - 2 * border, ht - 2 *border), L_INSERT);
            pixRasterop(pixd, x, y, wt, ht, PIX_SRC, pix, 0, 0);
            pixDestroy(&pix);
            x += wt + spacing;
        }
        y += maxh + spacing;
    }
    boxaWriteMem(&data, &size, boxa);
    pixSetText(pixd, (char *)data);  /* data is ascii */
    LEPT_FREE(data);
    boxaDestroy(&boxa);

    numaDestroy(&nainrow);
    numaDestroy(&namaxh);
    pixaDestroy(&pixan);
    return pixd;
}


/*!
 * \brief   pixaDisplayTiledInColumns()
 *
 * \param[in]    pixas
 * \param[in]    nx           number of columns in output image
 * \param[in]    scalefactor  applied to every pix; use 1.0 for no scaling
 * \param[in]    spacing      between images, and on outside
 * \param[in]    border       width of black border added to each image;
 *                            use 0 for no border
 * \return  pixd of tiled images, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) This renders a pixa to a single image with &nx columns of
 *          subimages.  The background color is white, and each row
 *          is tiled such that the top of each pix is aligned and
 *          each pix is separated by 'spacing' from the next one.
 *          A black border can be added to each pix.
 *      (2) The output depth is determined by the largest depth
 *          required by the pix in the pixa.  Colormaps are removed.
 *      (3) A serialized boxa giving the location in pixd of each input
 *          pix (without added border) is stored in the text string of pixd.
 *          This allows, e.g., regeneration of a pixa from pixd, using
 *          pixaCreateFromBoxa().  If there is no scaling and the depth of
 *          each input pix in the pixa is the same, this tiling operation
 *          can be inverted using the boxa (except for loss of text in
 *          each of the input pix):
 *            pix1 = pixaDisplayTiledInColumns(pixa1, 3, 1.0, 0, 30, 2);
 *            char *boxatxt = pixGetText(pix1);
 *            boxa1 = boxaReadMem((l_uint8 *)boxatxt, strlen(boxatxt));
 *            pixa2 = pixaCreateFromBoxa(pix1, boxa1, NULL);
 * </pre>
 */
PIX *
pixaDisplayTiledInColumns(PIXA      *pixas,
                          l_int32    nx,
                          l_float32  scalefactor,
                          l_int32    spacing,
                          l_int32    border)
{
l_int32   i, j, index, n, x, y, nrows, wb, hb, w, h, maxd, maxh, bordval, res;
size_t    size;
l_uint8  *data;
BOX      *box;
BOXA     *boxa;
PIX      *pix1, *pix2, *pix3, *pixd;
PIXA     *pixa1, *pixa2;

    PROCNAME("pixaDisplayTiledInColumns");

    if (!pixas)
        return (PIX *)ERROR_PTR("pixas not defined", procName, NULL);
    if (border < 0)
        border = 0;
    if (scalefactor <= 0.0) scalefactor = 1.0;

    if ((n = pixaGetCount(pixas)) == 0)
        return (PIX *)ERROR_PTR("no components", procName, NULL);

        /* Convert to same depth, if necessary */
    pixa1 = pixaConvertToSameDepth(pixas);
    pixaGetDepthInfo(pixa1, &maxd, NULL);

        /* Scale and optionally add border */
    pixa2 = pixaCreate(n);
    bordval = (maxd == 1) ? 1 : 0;
    for (i = 0; i < n; i++) {
        if ((pix1 = pixaGetPix(pixa1, i, L_CLONE)) == NULL)
            continue;
        if (scalefactor != 1.0)
            pix2 = pixScale(pix1, scalefactor, scalefactor);
        else
            pix2 = pixClone(pix1);
        if (border)
            pix3 = pixAddBorder(pix2, border, bordval);
        else
            pix3 = pixClone(pix2);
        if (i == 0) res = pixGetXRes(pix3);
        pixaAddPix(pixa2, pix3, L_INSERT);
        pixDestroy(&pix1);
        pixDestroy(&pix2);
    }
    pixaDestroy(&pixa1);
    if (pixaGetCount(pixa2) != n) {
        n = pixaGetCount(pixa2);
        L_WARNING("only got %d components\n", procName, n);
        if (n == 0) {
            pixaDestroy(&pixa2);
            return (PIX *)ERROR_PTR("no components", procName, NULL);
        }
    }

        /* Compute layout parameters and save as a boxa */
    boxa = boxaCreate(n);
    nrows = (n + nx - 1) / nx;
    y = spacing;
    for (i = 0, index = 0; i < nrows; i++) {
        x = spacing;
        maxh = 0;
        for (j = 0; j < nx && index < n; j++) {
            pixaGetPixDimensions(pixa2, index, &wb, &hb, NULL);
            box = boxCreate(x, y, wb, hb);
            boxaAddBox(boxa, box, L_INSERT);
            maxh = L_MAX(maxh, hb + spacing);
            x += wb + spacing;
            index++;
        }
        y += maxh;
    }
    pixaSetBoxa(pixa2, boxa, L_INSERT);

        /* Render the output pix */
    boxaGetExtent(boxa, &w, &h, NULL);
    pixd = pixaDisplay(pixa2, w + spacing, h + spacing);
    pixSetResolution(pixd, res, res);

        /* Save the boxa in the text field of the output pix */
    boxaWriteMem(&data, &size, boxa);
    pixSetText(pixd, (char *)data);  /* data is ascii */
    LEPT_FREE(data);

    pixaDestroy(&pixa2);
    return pixd;
}


/*!
 * \brief   pixaDisplayTiledAndScaled()
 *
 * \param[in]    pixa
 * \param[in]    outdepth    output depth: 1, 8 or 32 bpp
 * \param[in]    tilewidth   each pix is scaled to this width
 * \param[in]    ncols       number of tiles in each row
 * \param[in]    background  0 for white, 1 for black; this is the color
 *                           of the spacing between the images
 * \param[in]    spacing     between images, and on outside
 * \param[in]    border      width of additional black border on each image;
 *                           use 0 for no border
 * \return  pix of tiled images, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) This can be used to tile a number of renderings of
 *          an image that are at different scales and depths.
 *      (2) Each image, after scaling and optionally adding the
 *          black border, has width 'tilewidth'.  Thus, the border does
 *          not affect the spacing between the image tiles.  The
 *          maximum allowed border width is tilewidth / 5.
 * </pre>
 */
PIX *
pixaDisplayTiledAndScaled(PIXA    *pixa,
                          l_int32  outdepth,
                          l_int32  tilewidth,
                          l_int32  ncols,
                          l_int32  background,
                          l_int32  spacing,
                          l_int32  border)
{
l_int32    x, y, w, h, wd, hd, d, res;
l_int32    i, n, nrows, maxht, ninrow, irow, bordval;
l_int32   *rowht;
l_float32  scalefact;
PIX       *pix, *pixn, *pix1, *pixb, *pixd;
PIXA      *pixan;

    PROCNAME("pixaDisplayTiledAndScaled");

    if (!pixa)
        return (PIX *)ERROR_PTR("pixa not defined", procName, NULL);
    if (outdepth != 1 && outdepth != 8 && outdepth != 32)
        return (PIX *)ERROR_PTR("outdepth not in {1, 8, 32}", procName, NULL);
    if (ncols <= 0)
        return (PIX *)ERROR_PTR("ncols must be > 0", procName, NULL);
    if (border < 0 || border > tilewidth / 5)
        border = 0;

    if ((n = pixaGetCount(pixa)) == 0)
        return (PIX *)ERROR_PTR("no components", procName, NULL);

        /* Normalize scale and depth for each pix; optionally add border */
    pixan = pixaCreate(n);
    bordval = (outdepth == 1) ? 1 : 0;
    for (i = 0; i < n; i++) {
        if ((pix = pixaGetPix(pixa, i, L_CLONE)) == NULL)
            continue;

        pixGetDimensions(pix, &w, &h, &d);
        scalefact = (l_float32)(tilewidth - 2 * border) / (l_float32)w;
        if (d == 1 && outdepth > 1 && scalefact < 1.0)
            pix1 = pixScaleToGray(pix, scalefact);
        else
            pix1 = pixScale(pix, scalefact, scalefact);

        if (outdepth == 1)
            pixn = pixConvertTo1(pix1, 128);
        else if (outdepth == 8)
            pixn = pixConvertTo8(pix1, FALSE);
        else  /* outdepth == 32 */
            pixn = pixConvertTo32(pix1);
        pixDestroy(&pix1);

        if (border)
            pixb = pixAddBorder(pixn, border, bordval);
        else
            pixb = pixClone(pixn);

        pixaAddPix(pixan, pixb, L_INSERT);
        pixDestroy(&pix);
        pixDestroy(&pixn);
    }
    if ((n = pixaGetCount(pixan)) == 0) { /* should not have changed! */
        pixaDestroy(&pixan);
        return (PIX *)ERROR_PTR("no components", procName, NULL);
    }

        /* Determine the size of each row and of pixd */
    wd = tilewidth * ncols + spacing * (ncols + 1);
    nrows = (n + ncols - 1) / ncols;
    if ((rowht = (l_int32 *)LEPT_CALLOC(nrows, sizeof(l_int32))) == NULL) {
        pixaDestroy(&pixan);
        return (PIX *)ERROR_PTR("rowht array not made", procName, NULL);
    }
    maxht = 0;
    ninrow = 0;
    irow = 0;
    for (i = 0; i < n; i++) {
        pix = pixaGetPix(pixan, i, L_CLONE);
        ninrow++;
        pixGetDimensions(pix, &w, &h, NULL);
        maxht = L_MAX(h, maxht);
        if (ninrow == ncols) {
            rowht[irow] = maxht;
            maxht = ninrow = 0;  /* reset */
            irow++;
        }
        pixDestroy(&pix);
    }
    if (ninrow > 0) {   /* last fencepost */
        rowht[irow] = maxht;
        irow++;  /* total number of rows */
    }
    nrows = irow;
    hd = spacing * (nrows + 1);
    for (i = 0; i < nrows; i++)
        hd += rowht[i];

    pixd = pixCreate(wd, hd, outdepth);
    if ((background == 1 && outdepth == 1) ||
        (background == 0 && outdepth != 1))
        pixSetAll(pixd);

        /* Now blit images to pixd */
    x = y = spacing;
    irow = 0;
    for (i = 0; i < n; i++) {
        pix = pixaGetPix(pixan, i, L_CLONE);
        if (i == 0) {
            res = pixGetXRes(pix);
            pixSetResolution(pixd, res, res);
        }
        pixGetDimensions(pix, &w, &h, NULL);
        if (i && ((i % ncols) == 0)) {  /* start new row */
            x = spacing;
            y += spacing + rowht[irow];
            irow++;
        }
        pixRasterop(pixd, x, y, w, h, PIX_SRC, pix, 0, 0);
        x += tilewidth + spacing;
        pixDestroy(&pix);
    }

    pixaDestroy(&pixan);
    LEPT_FREE(rowht);
    return pixd;
}


/*!
 * \brief   pixaDisplayTiledWithText()
 *
 * \param[in]    pixa
 * \param[in]    maxwidth     of output image
 * \param[in]    scalefactor  applied to every pix; use 1.0 for no scaling
 * \param[in]    spacing      between images, and on outside
 * \param[in]    border       width of black border added to each image;
 *                            use 0 for no border
 * \param[in]    fontsize     4, 6, ... 20
 * \param[in]    textcolor    0xrrggbb00
 * \return  pixd of tiled images, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) This is a version of pixaDisplayTiledInRows() that prints, below
 *          each pix, the text in the pix text field.  Up to 127 chars
 *          of text in the pix text field are rendered below each pix.
 *      (2) It renders a pixa to a single image of width not to
 *          exceed %maxwidth, with white background color, with each row
 *          tiled such that the top of each pix is aligned and separated
 *          by %spacing from the next one.
 *      (3) All pix are converted to 32 bpp.
 *      (4) This does a reasonably spacewise-efficient job of laying
 *          out the individual pix images into a tiled composite.
 * </pre>
 */
PIX *
pixaDisplayTiledWithText(PIXA      *pixa,
                         l_int32    maxwidth,
                         l_float32  scalefactor,
                         l_int32    spacing,
                         l_int32    border,
                         l_int32    fontsize,
                         l_uint32   textcolor)
{
char      buf[128];
char     *textstr;
l_int32   i, n, maxw;
L_BMF    *bmf;
PIX      *pix1, *pix2, *pix3, *pix4, *pixd;
PIXA     *pixad;

    PROCNAME("pixaDisplayTiledWithText");

    if (!pixa)
        return (PIX *)ERROR_PTR("pixa not defined", procName, NULL);
    if ((n = pixaGetCount(pixa)) == 0)
        return (PIX *)ERROR_PTR("no components", procName, NULL);
    if (maxwidth <= 0)
        return (PIX *)ERROR_PTR("invalid maxwidth", procName, NULL);
    if (border < 0)
        border = 0;
    if (scalefactor <= 0.0) {
        L_WARNING("invalid scalefactor; setting to 1.0\n", procName);
        scalefactor = 1.0;
    }
    if (fontsize < 4 || fontsize > 20 || (fontsize & 1)) {
        l_int32 fsize = L_MAX(L_MIN(fontsize, 20), 4);
        if (fsize & 1) fsize--;
        L_WARNING("changed fontsize from %d to %d\n", procName,
                  fontsize, fsize);
        fontsize = fsize;
    }

        /* Be sure the width can accommodate a single column of images */
    pixaSizeRange(pixa, NULL, NULL, &maxw, NULL);
    maxwidth = L_MAX(maxwidth, scalefactor * (maxw + 2 * spacing + 2 * border));

    bmf = bmfCreate(NULL, fontsize);
    pixad = pixaCreate(n);
    for (i = 0; i < n; i++) {
        pix1 = pixaGetPix(pixa, i, L_CLONE);
        pix2 = pixConvertTo32(pix1);
        pix3 = pixAddBorderGeneral(pix2, spacing, spacing, spacing,
                                   spacing, 0xffffff00);
        textstr = pixGetText(pix1);
        if (textstr && strlen(textstr) > 0) {
            snprintf(buf, sizeof(buf), "%s", textstr);
            pix4 = pixAddSingleTextblock(pix3, bmf, buf, textcolor,
                                     L_ADD_BELOW, NULL);
        } else {
            pix4 = pixClone(pix3);
        }
        pixaAddPix(pixad, pix4, L_INSERT);
        pixDestroy(&pix1);
        pixDestroy(&pix2);
        pixDestroy(&pix3);
    }
    bmfDestroy(&bmf);

    pixd = pixaDisplayTiledInRows(pixad, 32, maxwidth, scalefactor,
                                  0, 10, border);
    pixaDestroy(&pixad);
    return pixd;
}


/*!
 * \brief   pixaDisplayTiledByIndex()
 *
 * \param[in]    pixa
 * \param[in]    na         numa with indices corresponding to the pix in pixa
 * \param[in]    width      each pix is scaled to this width
 * \param[in]    spacing    between images, and on outside
 * \param[in]    border     width of black border added to each image;
 *                          use 0 for no border
 * \param[in]    fontsize   4, 6, ... 20
 * \param[in]    textcolor  0xrrggbb00
 * \return  pixd of tiled images, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) This renders a pixa to a single image with white
 *          background color, where the pix are placed in columns
 *          given by the index value in the numa.  Each pix
 *          is separated by %spacing from the adjacent ones, and
 *          an optional border is placed around them.
 *      (2) Up to 127 chars of text in the pix text field are rendered
 *          below each pix.  Use newlines in the text field to write
 *          the text in multiple lines that fit within the pix width.
 *      (3) To avoid having empty columns, if there are N different
 *          index values, they should be in [0 ... N-1].
 *      (4) All pix are converted to 32 bpp.
 * </pre>
 */
PIX *
pixaDisplayTiledByIndex(PIXA     *pixa,
                        NUMA     *na,
                        l_int32   width,
                        l_int32   spacing,
                        l_int32   border,
                        l_int32   fontsize,
                        l_uint32  textcolor)
{
char      buf[128];
char     *textstr;
l_int32    i, n, x, y, w, h, yval, index;
l_float32  maxindex;
L_BMF     *bmf;
BOX       *box;
NUMA      *nay;  /* top of the next pix to add in that column */
PIX       *pix1, *pix2, *pix3, *pix4, *pix5, *pixd;
PIXA      *pixad;

    PROCNAME("pixaDisplayTiledByIndex");

    if (!pixa)
        return (PIX *)ERROR_PTR("pixa not defined", procName, NULL);
    if (!na)
        return (PIX *)ERROR_PTR("na not defined", procName, NULL);
    if ((n = pixaGetCount(pixa)) == 0)
        return (PIX *)ERROR_PTR("no pixa components", procName, NULL);
    if (n != numaGetCount(na))
        return (PIX *)ERROR_PTR("pixa and na counts differ", procName, NULL);
    if (width <= 0)
        return (PIX *)ERROR_PTR("invalid width", procName, NULL);
    if (width < 20)
        L_WARNING("very small width: %d\n", procName, width);
    if (border < 0)
        border = 0;
    if (fontsize < 4 || fontsize > 20 || (fontsize & 1)) {
        l_int32 fsize = L_MAX(L_MIN(fontsize, 20), 4);
        if (fsize & 1) fsize--;
        L_WARNING("changed fontsize from %d to %d\n", procName,
                  fontsize, fsize);
        fontsize = fsize;
    }

        /* The pix will be rendered in the order they occupy in pixa. */
    bmf = bmfCreate(NULL, fontsize);
    pixad = pixaCreate(n);
    numaGetMax(na, &maxindex, NULL);
    nay = numaMakeConstant(spacing, lept_roundftoi(maxindex) + 1);
    for (i = 0; i < n; i++) {
        numaGetIValue(na, i, &index);
        numaGetIValue(nay, index, &yval);
        pix1 = pixaGetPix(pixa, i, L_CLONE);
        pix2 = pixConvertTo32(pix1);
        pix3 = pixScaleToSize(pix2, width, 0);
        pix4 = pixAddBorderGeneral(pix3, border, border, border, border, 0);
        textstr = pixGetText(pix1);
        if (textstr && strlen(textstr) > 0) {
            snprintf(buf, sizeof(buf), "%s", textstr);
            pix5 = pixAddTextlines(pix4, bmf, textstr, textcolor, L_ADD_BELOW);
        } else {
            pix5 = pixClone(pix4);
        }
        pixaAddPix(pixad, pix5, L_INSERT);
        x = spacing + border + index * (2 * border + width + spacing);
        y = yval;
        pixGetDimensions(pix5, &w, &h, NULL);
        yval += h + spacing;
        numaSetValue(nay, index, yval);
        box = boxCreate(x, y, w, h);
        pixaAddBox(pixad, box, L_INSERT);
        pixDestroy(&pix1);
        pixDestroy(&pix2);
        pixDestroy(&pix3);
        pixDestroy(&pix4);
    }
    numaDestroy(&nay);
    bmfDestroy(&bmf);

    pixd = pixaDisplay(pixad, 0, 0);
    pixaDestroy(&pixad);
    return pixd;
}



/*---------------------------------------------------------------------*
 *                              Pixaa Display                          *
 *---------------------------------------------------------------------*/
/*!
 * \brief   pixaaDisplay()
 *
 * \param[in]    paa
 * \param[in]    w, h   if set to 0, the size is determined from the
 *                      bounding box of the components in pixa
 * \return  pix, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) Each pix of the paa is displayed at the location given by
 *          its box, translated by the box of the containing pixa
 *          if it exists.
 * </pre>
 */
PIX *
pixaaDisplay(PIXAA   *paa,
             l_int32  w,
             l_int32  h)
{
l_int32  i, j, n, nbox, na, d, wmax, hmax, x, y, xb, yb, wb, hb;
BOXA    *boxa1;  /* top-level boxa */
BOXA    *boxa;
PIX     *pix1, *pixd;
PIXA    *pixa;

    PROCNAME("pixaaDisplay");

    if (!paa)
        return (PIX *)ERROR_PTR("paa not defined", procName, NULL);

    n = pixaaGetCount(paa, NULL);
    if (n == 0)
        return (PIX *)ERROR_PTR("no components", procName, NULL);

        /* If w and h not input, determine the minimum size required
         * to contain the origin and all c.c. */
    boxa1 = pixaaGetBoxa(paa, L_CLONE);
    nbox = boxaGetCount(boxa1);
    if (w == 0 || h == 0) {
        if (nbox == n) {
            boxaGetExtent(boxa1, &w, &h, NULL);
        } else {  /* have to use the lower-level boxa for each pixa */
            wmax = hmax = 0;
            for (i = 0; i < n; i++) {
                pixa = pixaaGetPixa(paa, i, L_CLONE);
                boxa = pixaGetBoxa(pixa, L_CLONE);
                boxaGetExtent(boxa, &w, &h, NULL);
                wmax = L_MAX(wmax, w);
                hmax = L_MAX(hmax, h);
                pixaDestroy(&pixa);
                boxaDestroy(&boxa);
            }
            w = wmax;
            h = hmax;
        }
    }

        /* Get depth from first pix */
    pixa = pixaaGetPixa(paa, 0, L_CLONE);
    pix1 = pixaGetPix(pixa, 0, L_CLONE);
    d = pixGetDepth(pix1);
    pixaDestroy(&pixa);
    pixDestroy(&pix1);

    if ((pixd = pixCreate(w, h, d)) == NULL) {
        boxaDestroy(&boxa1);
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    }

    x = y = 0;
    for (i = 0; i < n; i++) {
        pixa = pixaaGetPixa(paa, i, L_CLONE);
        if (nbox == n)
            boxaGetBoxGeometry(boxa1, i, &x, &y, NULL, NULL);
        na = pixaGetCount(pixa);
        for (j = 0; j < na; j++) {
            pixaGetBoxGeometry(pixa, j, &xb, &yb, &wb, &hb);
            pix1 = pixaGetPix(pixa, j, L_CLONE);
            pixRasterop(pixd, x + xb, y + yb, wb, hb, PIX_PAINT, pix1, 0, 0);
            pixDestroy(&pix1);
        }
        pixaDestroy(&pixa);
    }
    boxaDestroy(&boxa1);

    return pixd;
}


/*!
 * \brief   pixaaDisplayByPixa()
 *
 * \param[in]    paa     with pix that may have different depths
 * \param[in]    xspace  between pix in pixa
 * \param[in]    yspace  between pixa
 * \param[in]    maxw    max width of output pix
 * \return  pixd, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) Displays each pixa on a line (or set of lines),
 *          in order from top to bottom.  Within each pixa,
 *          the pix are displayed in order from left to right.
 *      (2) The sizes and depths of each pix can differ.  The output pix
 *          has a depth equal to the max depth of all the pix.
 *      (3) This ignores the boxa of the paa.
 * </pre>
 */
PIX *
pixaaDisplayByPixa(PIXAA   *paa,
                   l_int32  xspace,
                   l_int32  yspace,
                   l_int32  maxw)
{
l_int32   i, j, npixa, npix, same, use_maxw, x, y, w, h, hindex;
l_int32   maxwidth, maxd, width, lmaxh, lmaxw;
l_int32  *harray;
NUMA     *nah;
PIX      *pix, *pix1, *pixd;
PIXA     *pixa;

    PROCNAME("pixaaDisplayByPixa");

    if (!paa)
        return (PIX *)ERROR_PTR("paa not defined", procName, NULL);

    if ((npixa = pixaaGetCount(paa, NULL)) == 0)
        return (PIX *)ERROR_PTR("no components", procName, NULL);
    pixaaVerifyDepth(paa, &same, &maxd);
    if (!same && maxd < 8)
        return (PIX *)ERROR_PTR("depths differ; max < 8", procName, NULL);

        /* Be sure the widest box fits in the output pix */
    pixaaSizeRange(paa, NULL, NULL, &maxwidth, NULL);
    if (maxwidth > maxw) {
        L_WARNING("maxwidth > maxw; using maxwidth\n", procName);
        maxw = maxwidth;
    }

        /* Get size of output pix.  The width is the minimum of the
         * maxw and the largest pixa line width.  The height is whatever
         * it needs to be to accommodate all pixa. */
    lmaxw = 0;  /* widest line found */
    use_maxw = FALSE;
    nah = numaCreate(0);  /* store height of each line */
    y = yspace;
    for (i = 0; i < npixa; i++) {
        pixa = pixaaGetPixa(paa, i, L_CLONE);
        npix = pixaGetCount(pixa);
        if (npix == 0) {
            pixaDestroy(&pixa);
            continue;
        }
        x = xspace;
        lmaxh = 0;  /* max height found in the line */
        for (j = 0; j < npix; j++) {
            pix = pixaGetPix(pixa, j, L_CLONE);
            pixGetDimensions(pix, &w, &h, NULL);
            if (x + w >= maxw) {  /* start new line */
                x = xspace;
                y += lmaxh + yspace;
                numaAddNumber(nah, lmaxh);
                lmaxh = 0;
                use_maxw = TRUE;
            }
            x += w + xspace;
            lmaxh = L_MAX(h, lmaxh);
            lmaxw = L_MAX(lmaxw, x);
            pixDestroy(&pix);
        }
        y += lmaxh + yspace;
        numaAddNumber(nah, lmaxh);
        pixaDestroy(&pixa);
    }
    width = (use_maxw) ? maxw : lmaxw;

    if ((pixd = pixCreate(width, y, maxd)) == NULL) {
        numaDestroy(&nah);
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    }

        /* Now layout the pix by pixa */
    y = yspace;
    harray = numaGetIArray(nah);
    hindex = 0;
    for (i = 0; i < npixa; i++) {
        x = xspace;
        pixa = pixaaGetPixa(paa, i, L_CLONE);
        npix = pixaGetCount(pixa);
        if (npix == 0) {
            pixaDestroy(&pixa);
            continue;
        }
        for (j = 0; j < npix; j++) {
            pix = pixaGetPix(pixa, j, L_CLONE);
            if (pixGetDepth(pix) != maxd) {
                if (maxd == 8)
                     pix1 = pixConvertTo8(pix, 0);
                else  /* 32 bpp */
                     pix1 = pixConvertTo32(pix);
            } else {
                pix1 = pixClone(pix);
            }
            pixGetDimensions(pix1, &w, &h, NULL);
            if (x + w >= maxw) {  /* start new line */
                x = xspace;
                y += harray[hindex++] + yspace;
            }
            pixRasterop(pixd, x, y, w, h, PIX_PAINT, pix1, 0, 0);
            pixDestroy(&pix);
            pixDestroy(&pix1);
            x += w + xspace;
        }
        y += harray[hindex++] + yspace;
        pixaDestroy(&pixa);
    }
    LEPT_FREE(harray);

    numaDestroy(&nah);
    return pixd;
}


/*!
 * \brief   pixaaDisplayTiledAndScaled()
 *
 * \param[in]    paa
 * \param[in]    outdepth    output depth: 1, 8 or 32 bpp
 * \param[in]    tilewidth   each pix is scaled to this width
 * \param[in]    ncols       number of tiles in each row
 * \param[in]    background  0 for white, 1 for black; this is the color
 *                           of the spacing between the images
 * \param[in]    spacing     between images, and on outside
 * \param[in]    border      width of additional black border on each image;
 *                           use 0 for no border
 * \return  pixa of tiled images, one image for each pixa in
 *                    the paa, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) For each pixa, this generates from all the pix a
 *          tiled/scaled output pix, and puts it in the output pixa.
 *      (2) See comments in pixaDisplayTiledAndScaled().
 * </pre>
 */
PIXA *
pixaaDisplayTiledAndScaled(PIXAA   *paa,
                           l_int32  outdepth,
                           l_int32  tilewidth,
                           l_int32  ncols,
                           l_int32  background,
                           l_int32  spacing,
                           l_int32  border)
{
l_int32  i, n;
PIX     *pix;
PIXA    *pixa, *pixad;

    PROCNAME("pixaaDisplayTiledAndScaled");

    if (!paa)
        return (PIXA *)ERROR_PTR("paa not defined", procName, NULL);
    if (outdepth != 1 && outdepth != 8 && outdepth != 32)
        return (PIXA *)ERROR_PTR("outdepth not in {1, 8, 32}", procName, NULL);
    if (ncols <= 0)
        return (PIXA *)ERROR_PTR("ncols must be > 0", procName, NULL);
    if (border < 0 || border > tilewidth / 5)
        border = 0;

    if ((n = pixaaGetCount(paa, NULL)) == 0)
        return (PIXA *)ERROR_PTR("no components", procName, NULL);

    pixad = pixaCreate(n);
    for (i = 0; i < n; i++) {
        pixa = pixaaGetPixa(paa, i, L_CLONE);
        pix = pixaDisplayTiledAndScaled(pixa, outdepth, tilewidth, ncols,
                                        background, spacing, border);
        pixaAddPix(pixad, pix, L_INSERT);
        pixaDestroy(&pixa);
    }

    return pixad;
}


/*---------------------------------------------------------------------*
 *         Conversion of all pix to specified type (e.g., depth)       *
 *---------------------------------------------------------------------*/
/*!
 * \brief   pixaConvertTo1()
 *
 * \param[in]    pixas
 * \param[in]    thresh    threshold for final binarization from 8 bpp gray
 * \return  pixad, or NULL on error
 */
PIXA *
pixaConvertTo1(PIXA    *pixas,
               l_int32  thresh)
{
l_int32  i, n;
BOXA    *boxa;
PIX     *pix1, *pix2;
PIXA    *pixad;

    PROCNAME("pixaConvertTo1");

    if (!pixas)
        return (PIXA *)ERROR_PTR("pixas not defined", procName, NULL);

    n = pixaGetCount(pixas);
    pixad = pixaCreate(n);
    for (i = 0; i < n; i++) {
        pix1 = pixaGetPix(pixas, i, L_CLONE);
        pix2 = pixConvertTo1(pix1, thresh);
        pixaAddPix(pixad, pix2, L_INSERT);
        pixDestroy(&pix1);
    }

    boxa = pixaGetBoxa(pixas, L_COPY);
    pixaSetBoxa(pixad, boxa, L_INSERT);
    return pixad;
}


/*!
 * \brief   pixaConvertTo8()
 *
 * \param[in]    pixas
 * \param[in]    cmapflag   1 to give pixd a colormap; 0 otherwise
 * \return  pixad each pix is 8 bpp, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) See notes for pixConvertTo8(), applied to each pix in pixas.
 * </pre>
 */
PIXA *
pixaConvertTo8(PIXA    *pixas,
               l_int32  cmapflag)
{
l_int32  i, n;
BOXA    *boxa;
PIX     *pix1, *pix2;
PIXA    *pixad;

    PROCNAME("pixaConvertTo8");

    if (!pixas)
        return (PIXA *)ERROR_PTR("pixas not defined", procName, NULL);

    n = pixaGetCount(pixas);
    pixad = pixaCreate(n);
    for (i = 0; i < n; i++) {
        pix1 = pixaGetPix(pixas, i, L_CLONE);
        pix2 = pixConvertTo8(pix1, cmapflag);
        pixaAddPix(pixad, pix2, L_INSERT);
        pixDestroy(&pix1);
    }

    boxa = pixaGetBoxa(pixas, L_COPY);
    pixaSetBoxa(pixad, boxa, L_INSERT);
    return pixad;
}


/*!
 * \brief   pixaConvertTo8Colormap()
 *
 * \param[in]    pixas
 * \param[in]    dither   1 to dither if necessary; 0 otherwise
 * \return  pixad each pix is 8 bpp, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) See notes for pixConvertTo8Colormap(), applied to each pix in pixas.
 * </pre>
 */
PIXA *
pixaConvertTo8Colormap(PIXA    *pixas,
                       l_int32  dither)
{
l_int32  i, n;
BOXA    *boxa;
PIX     *pix1, *pix2;
PIXA    *pixad;

    PROCNAME("pixaConvertTo8Colormap");

    if (!pixas)
        return (PIXA *)ERROR_PTR("pixas not defined", procName, NULL);

    n = pixaGetCount(pixas);
    pixad = pixaCreate(n);
    for (i = 0; i < n; i++) {
        pix1 = pixaGetPix(pixas, i, L_CLONE);
        pix2 = pixConvertTo8Colormap(pix1, dither);
        pixaAddPix(pixad, pix2, L_INSERT);
        pixDestroy(&pix1);
    }

    boxa = pixaGetBoxa(pixas, L_COPY);
    pixaSetBoxa(pixad, boxa, L_INSERT);
    return pixad;
}


/*!
 * \brief   pixaConvertTo32()
 *
 * \param[in]    pixas
 * \return  pixad 32 bpp rgb, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) See notes for pixConvertTo32(), applied to each pix in pixas.
 *      (2) This can be used to allow 1 bpp pix in a pixa to be displayed
 *          with color.
 * </pre>
 */
PIXA *
pixaConvertTo32(PIXA  *pixas)
{
l_int32  i, n;
BOXA    *boxa;
PIX     *pix1, *pix2;
PIXA    *pixad;

    PROCNAME("pixaConvertTo32");

    if (!pixas)
        return (PIXA *)ERROR_PTR("pixas not defined", procName, NULL);

    n = pixaGetCount(pixas);
    pixad = pixaCreate(n);
    for (i = 0; i < n; i++) {
        pix1 = pixaGetPix(pixas, i, L_CLONE);
        pix2 = pixConvertTo32(pix1);
        pixaAddPix(pixad, pix2, L_INSERT);
        pixDestroy(&pix1);
    }

    boxa = pixaGetBoxa(pixas, L_COPY);
    pixaSetBoxa(pixad, boxa, L_INSERT);
    return pixad;
}


/*---------------------------------------------------------------------*
 *                        Pixa constrained selection                   *
 *---------------------------------------------------------------------*/
/*!
 * \brief   pixaConstrainedSelect()
 *
 * \param[in]    pixas
 * \param[in]    first      first index to choose; >= 0
 * \param[in]    last       biggest possible index to reach;
 *                          use -1 to go to the end; otherwise, last >= first
 * \param[in]    nmax       maximum number of pix to select; > 0
 * \param[in]    use_pairs  1 = select pairs of adjacent pix;
 *                          0 = select individual pix
 * \param[in]    copyflag   L_COPY, L_CLONE
 * \return  pixad if OK, NULL on error
 *
 * <pre>
 * Notes:
 *     (1) See notes in genConstrainedNumaInRange() for how selection
 *         is made.
 *     (2) This returns a selection of the pix in the input pixa.
 *     (3) Use copyflag == L_COPY if you don't want changes in the pix
 *         in the returned pixa to affect those in the input pixa.
 * </pre>
 */
PIXA *
pixaConstrainedSelect(PIXA    *pixas,
                      l_int32  first,
                      l_int32  last,
                      l_int32  nmax,
                      l_int32  use_pairs,
                      l_int32  copyflag)
{
l_int32  i, n, nselect, index;
NUMA    *na;
PIX     *pix1;
PIXA    *pixad;

    PROCNAME("pixaConstrainedSelect");

    if (!pixas)
        return (PIXA *)ERROR_PTR("pixas not defined", procName, NULL);
    n = pixaGetCount(pixas);
    first = L_MAX(0, first);
    last = (last < 0) ? n - 1 : L_MIN(n - 1, last);
    if (last < first)
        return (PIXA *)ERROR_PTR("last < first!", procName, NULL);
    if (nmax < 1)
        return (PIXA *)ERROR_PTR("nmax < 1!", procName, NULL);

    na = genConstrainedNumaInRange(first, last, nmax, use_pairs);
    nselect = numaGetCount(na);
    pixad = pixaCreate(nselect);
    for (i = 0; i < nselect; i++) {
        numaGetIValue(na, i, &index);
        pix1 = pixaGetPix(pixas, index, copyflag);
        pixaAddPix(pixad, pix1, L_INSERT);
    }
    numaDestroy(&na);
    return pixad;
}


/*!
 * \brief   pixaSelectToPdf()
 *
 * \param[in]    pixas
 * \param[in]    first     first index to choose; >= 0
 * \param[in]    last      biggest possible index to reach;
 *                         use -1 to go to the end; otherwise, last >= first
 * \param[in]    res       override the resolution of each input image, in ppi;
 *                         use 0 to respect the resolution embedded in the input
 * \param[in]    scalefactor   scaling factor applied to each image; > 0.0
 * \param[in]    type      encoding type (L_JPEG_ENCODE, L_G4_ENCODE,
 *                         L_FLATE_ENCODE, or 0 for default
 * \param[in]    quality   used for JPEG only; 0 for default (75)
 * \param[in]    color     of numbers added to each image (e.g., 0xff000000)
 * \param[in]    fontsize  to print number below each image.  The valid set
 *                         is {4,6,8,10,12,14,16,18,20}.  Use 0 to disable.
 * \param[in]    fileout   pdf file of all images
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) This writes a pdf of the selected images from %pixas, one to
 *          a page.  They are optionally scaled and annotated with the
 *          index printed to the left of the image.
 *      (2) If the input images are 1 bpp and you want the numbers to be
 *          in color, first promote each pix to 8 bpp with a colormap:
 *                pixa1 = pixaConvertTo8(pixas, 1);
 *          and then call this function with the specified color
 * </pre>
 */
l_int32
pixaSelectToPdf(PIXA        *pixas,
                l_int32      first,
                l_int32      last,
                l_int32      res,
                l_float32    scalefactor,
                l_int32      type,
                l_int32      quality,
                l_uint32     color,
                l_int32      fontsize,
                const char  *fileout)
{
l_int32  n;
L_BMF   *bmf;
NUMA    *na;
PIXA    *pixa1, *pixa2;

    PROCNAME("pixaSelectToPdf");

    if (!pixas)
        return ERROR_INT("pixas not defined", procName, 1);
    if (type < 0 || type > L_FLATE_ENCODE) {
        L_WARNING("invalid compression type; using default\n", procName);
        type = 0;
    }
    if (!fileout)
        return ERROR_INT("fileout not defined", procName, 1);

        /* Select from given range */
    n = pixaGetCount(pixas);
    first = L_MAX(0, first);
    last = (last < 0) ? n - 1 : L_MIN(n - 1, last);
    if (first > last) {
        L_ERROR("first = %d > last = %d\n", procName, first, last);
        return 1;
    }
    pixa1 = pixaSelectRange(pixas, first, last, L_CLONE);

        /* Optionally add index numbers */
    bmf = (fontsize <= 0) ? NULL : bmfCreate(NULL, fontsize);
    if (bmf) {
        na = numaMakeSequence(first, 1.0, last - first + 1);
        pixa2 = pixaAddTextNumber(pixa1, bmf, na, color, L_ADD_LEFT);
        numaDestroy(&na);
    } else {
        pixa2 = pixaCopy(pixa1, L_CLONE);
    }
    pixaDestroy(&pixa1);
    bmfDestroy(&bmf);

    pixaConvertToPdf(pixa2, res, scalefactor, type, quality, NULL, fileout);
    pixaDestroy(&pixa2);
    return 0;
}


/*---------------------------------------------------------------------*
 *                     Pixa display into multiple tiles                *
 *---------------------------------------------------------------------*/
/*!
 * \brief   pixaDisplayMultiTiled()
 *
 * \param[in]    pixas
 * \param[in]    nx, ny       in [1, ... 50], tiling factors in each direction
 * \param[in]    maxw, maxh   max sizes to keep
 * \param[in]    scalefactor  scale each image by this
 * \param[in]    spacing      between images, and on outside
 * \param[in]    border       width of additional black border on each image;
 *                            use 0 for no border
 * \return  pixad if OK, NULL on error
 *
 * <pre>
 * Notes:
 *      (1) Each set of %nx * %ny images is optionally scaled and saved
 *          into a new pix, and then aggregated.
 *      (2) Set %maxw = %maxh = 0 if you want to include all pix from %pixs.
 *      (3) This is useful for generating a pdf from the output pixa, where
 *          each page is a tile of (%nx * %ny) images from the input pixa.
 * </pre>
 */
PIXA *
pixaDisplayMultiTiled(PIXA      *pixas,
                      l_int32    nx,
                      l_int32    ny,
                      l_int32    maxw,
                      l_int32    maxh,
                      l_float32  scalefactor,
                      l_int32    spacing,
                      l_int32    border)
{
l_int32  n, i, j, ntile, nout, index;
PIX     *pix1, *pix2;
PIXA    *pixa1, *pixa2, *pixad;

    PROCNAME("pixaDisplayMultiTiled");

    if (!pixas)
        return (PIXA *)ERROR_PTR("pixas not defined", procName, NULL);
    if (nx < 1 || ny < 1 || nx > 50 || ny > 50)
        return (PIXA *)ERROR_PTR("invalid tiling factor(s)", procName, NULL);
    if ((n = pixaGetCount(pixas)) == 0)
        return (PIXA *)ERROR_PTR("pixas is empty", procName, NULL);

        /* Filter out large ones if requested */
    if (maxw == 0 && maxh == 0) {
        pixa1 = pixaCopy(pixas, L_CLONE);
    } else {
        maxw = (maxw == 0) ? 1000000 : maxw;
        maxh = (maxh == 0) ? 1000000 : maxh;
        pixa1 = pixaSelectBySize(pixas, maxw, maxh, L_SELECT_IF_BOTH,
                                 L_SELECT_IF_LTE, NULL);
        n = pixaGetCount(pixa1);
    }

    ntile = nx * ny;
    nout = L_MAX(1, (n + ntile - 1) / ntile);
    pixad = pixaCreate(nout);
    for (i = 0, index = 0; i < nout; i++) {  /* over tiles */
        pixa2 = pixaCreate(ntile);
        for (j = 0; j < ntile && index < n; j++, index++) {
            pix1 = pixaGetPix(pixa1, index, L_COPY);
            pixaAddPix(pixa2, pix1, L_INSERT);
        }
        pix2 = pixaDisplayTiledInColumns(pixa2, nx, scalefactor, spacing,
                                         border);
        pixaAddPix(pixad, pix2, L_INSERT);
        pixaDestroy(&pixa2);
    }
    pixaDestroy(&pixa1);

    return pixad;
}


/*---------------------------------------------------------------------*
 *                       Split pixa into files                         *
 *---------------------------------------------------------------------*/
/*!
 * \brief   pixaSplitIntoFiles()
 *
 * \param[in]    pixas
 * \param[in]    nsplit       split pixas into this number of pixa; >= 2
 * \param[in]    scale        scalefactor applied to each pix
 * \param[in]    outwidth     the maxwidth parameter of tiled images
 *                            for write_pix
 * \param[in]    write_pixa  1 to write the split pixa as separate files
 * \param[in]    write_pix   1 to write tiled images of the split pixa
 * \param[in]    write_pdf   1 to write pdfs of the split pixa
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) For each requested output, %nsplit files are written into
 *          directory /tmp/lept/split/.
 *      (2) This is useful when a pixa is so large that the images
 *          are not conveniently displayed as a single tiled image at
 *          full resolution.
 * </pre>
 */
l_int32
pixaSplitIntoFiles(PIXA      *pixas,
                   l_int32    nsplit,
                   l_float32  scale,
                   l_int32    outwidth,
                   l_int32    write_pixa,
                   l_int32    write_pix,
                   l_int32    write_pdf)
{
char     buf[64];
l_int32  i, j, index, n, nt;
PIX     *pix1, *pix2;
PIXA    *pixa1;

    PROCNAME("pixaSplitIntoFiles");

    if (!pixas)
        return ERROR_INT("pixas not defined", procName, 1);
    if (nsplit <= 1)
        return ERROR_INT("nsplit must be >= 2", procName, 1);
    if ((nt = pixaGetCount(pixas)) == 0)
        return ERROR_INT("pixas is empty", procName, 1);
    if (!write_pixa && !write_pix && !write_pdf)
        return ERROR_INT("no output is requested", procName, 1);

    lept_mkdir("lept/split");
    n = (nt + nsplit - 1) / nsplit;
    fprintf(stderr, "nt = %d, n = %d, nsplit = %d\n", nt, n, nsplit);
    for (i = 0, index = 0; i < nsplit; i++) {
        pixa1 = pixaCreate(n);
        for (j = 0; j < n && index < nt; j++, index++) {
            pix1 = pixaGetPix(pixas, index, L_CLONE);
            pix2 = pixScale(pix1, scale, scale);
            pixaAddPix(pixa1, pix2, L_INSERT);
            pixDestroy(&pix1);
        }
        if (write_pixa) {
            snprintf(buf, sizeof(buf), "/tmp/lept/split/split%d.pa", i + 1);
            pixaWriteDebug(buf, pixa1);
        }
        if (write_pix) {
            snprintf(buf, sizeof(buf), "/tmp/lept/split/split%d.tif", i + 1);
            pix1 = pixaDisplayTiledInRows(pixa1, 1, outwidth, 1.0, 0, 20, 2);
            pixWriteDebug(buf, pix1, IFF_TIFF_G4);
            pixDestroy(&pix1);
        }
        if (write_pdf) {
            snprintf(buf, sizeof(buf), "/tmp/lept/split/split%d.pdf", i + 1);
            pixaConvertToPdf(pixa1, 0, 1.0, L_G4_ENCODE, 0, buf, buf);
        }
        pixaDestroy(&pixa1);
    }

    return 0;
}


/*---------------------------------------------------------------------*
 *                               Tile N-Up                             *
 *---------------------------------------------------------------------*/
/*!
 * \brief   convertToNUpFiles()
 *
 * \param[in]    dir        full path to directory of images
 * \param[in]    substr     [optional] can be null
 * \param[in]    nx, ny     in [1, ... 50], tiling factors in each direction
 * \param[in]    tw         target width, in pixels; must be >= 20
 * \param[in]    spacing    between images, and on outside
 * \param[in]    border     width of additional black border on each image;
 *                          use 0 for no border
 * \param[in]    fontsize   to print tail of filename with image.  Valid set is
 *                          {4,6,8,10,12,14,16,18,20}.  Use 0 to disable.
 * \param[in]    outdir     subdirectory of /tmp to put N-up tiled images
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) Each set of %nx * %ny images is scaled and tiled into a single
 *          image, that is written out to %outdir.
 *      (2) All images in each %nx * %ny set are scaled to the same
 *          width, %tw.  This is typically used when all images are
 *          roughly the same size.
 *      (3) This is useful for generating a pdf from the set of input
 *          files, where each page is a tile of (%nx * %ny) input images.
 *          Typical values for %nx and %ny are in the range [2 ... 5].
 *      (4) If %fontsize != 0, each image has the tail of its filename
 *          rendered below it.
 * </pre>
 */
l_int32
convertToNUpFiles(const char  *dir,
                  const char  *substr,
                  l_int32      nx,
                  l_int32      ny,
                  l_int32      tw,
                  l_int32      spacing,
                  l_int32      border,
                  l_int32      fontsize,
                  const char  *outdir)
{
l_int32  d, format;
char     rootpath[256];
PIXA    *pixa;

    PROCNAME("convertToNUpFiles");

    if (!dir)
        return ERROR_INT("dir not defined", procName, 1);
    if (nx < 1 || ny < 1 || nx > 50 || ny > 50)
        return ERROR_INT("invalid tiling N-factor", procName, 1);
    if (fontsize < 0 || fontsize > 20 || fontsize & 1 || fontsize == 2)
        return ERROR_INT("invalid fontsize", procName, 1);
    if (!outdir)
        return ERROR_INT("outdir not defined", procName, 1);

    pixa = convertToNUpPixa(dir, substr, nx, ny, tw, spacing, border,
                            fontsize);
    if (!pixa)
        return ERROR_INT("pixa not made", procName, 1);

    lept_rmdir(outdir);
    lept_mkdir(outdir);
    pixaGetRenderingDepth(pixa, &d);
    format = (d == 1) ? IFF_TIFF_G4 : IFF_JFIF_JPEG;
    makeTempDirname(rootpath, 256, outdir);
    modifyTrailingSlash(rootpath, 256, L_ADD_TRAIL_SLASH);
    pixaWriteFiles(rootpath, pixa, format);
    pixaDestroy(&pixa);
    return 0;
}


/*!
 * \brief   convertToNUpPixa()
 *
 * \param[in]    dir       full path to directory of images
 * \param[in]    substr    [optional] can be null
 * \param[in]    nx, ny    in [1, ... 50], tiling factors in each direction
 * \param[in]    tw        target width, in pixels; must be >= 20
 * \param[in]    spacing   between images, and on outside
 * \param[in]    border    width of additional black border on each image;
 *                         use 0 for no border
 * \param[in]    fontsize  to print tail of filename with image.  Valid set is
 *                         {4,6,8,10,12,14,16,18,20}.  Use 0 to disable.
 * \return  pixad, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) See notes for convertToNUpFiles()
 * </pre>
 */
PIXA *
convertToNUpPixa(const char  *dir,
                 const char  *substr,
                 l_int32      nx,
                 l_int32      ny,
                 l_int32      tw,
                 l_int32      spacing,
                 l_int32      border,
                 l_int32      fontsize)
{
l_int32  i, n;
char    *fname, *tail;
PIXA    *pixa1, *pixa2;
SARRAY  *sa1, *sa2;

    PROCNAME("convertToNUpPixa");

    if (!dir)
        return (PIXA *)ERROR_PTR("dir not defined", procName, NULL);
    if (nx < 1 || ny < 1 || nx > 50 || ny > 50)
        return (PIXA *)ERROR_PTR("invalid tiling N-factor", procName, NULL);
    if (tw < 20)
        return (PIXA *)ERROR_PTR("tw must be >= 20", procName, NULL);
    if (fontsize < 0 || fontsize > 20 || fontsize & 1 || fontsize == 2)
        return (PIXA *)ERROR_PTR("invalid fontsize", procName, NULL);

    sa1 = getSortedPathnamesInDirectory(dir, substr, 0, 0);
    pixa1 = pixaReadFilesSA(sa1);
    n = sarrayGetCount(sa1);
    sa2 = sarrayCreate(n);
    for (i = 0; i < n; i++) {
        fname = sarrayGetString(sa1, i, L_NOCOPY);
        splitPathAtDirectory(fname, NULL, &tail);
        sarrayAddString(sa2, tail, L_INSERT);
    }
    sarrayDestroy(&sa1);
    pixa2 = pixaConvertToNUpPixa(pixa1, sa2, nx, ny, tw, spacing,
                                 border, fontsize);
    pixaDestroy(&pixa1);
    sarrayDestroy(&sa2);
    return pixa2;
}


/*!
 * \brief   pixaConvertToNUpPixa()
 *
 * \param[in]    pixas
 * \param[in]    sa        [optional] array of strings associated with each pix
 * \param[in]    nx, ny    in [1, ... 50], tiling factors in each direction
 * \param[in]    tw        target width, in pixels; must be >= 20
 * \param[in]    spacing   between images, and on outside
 * \param[in]    border    width of additional black border on each image;
 *                         use 0 for no border
 * \param[in]    fontsize  to print string with each image.  Valid set is
 *                         {4,6,8,10,12,14,16,18,20}.  Use 0 to disable.
 * \return  pixad, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) This takes an input pixa and an optional array of strings, and
 *          generates a pixa of NUp tiles from the input, labeled with
 *          the strings if they exist and %fontsize != 0.
 *      (2) See notes for convertToNUpFiles()
 * </pre>
 */
PIXA *
pixaConvertToNUpPixa(PIXA    *pixas,
                     SARRAY  *sa,
                     l_int32  nx,
                     l_int32  ny,
                     l_int32  tw,
                     l_int32  spacing,
                     l_int32  border,
                     l_int32  fontsize)
{
l_int32    i, j, k, nt, n2, nout, d;
char      *str;
L_BMF     *bmf;
PIX       *pix1, *pix2, *pix3, *pix4;
PIXA      *pixa1, *pixad;

    PROCNAME("pixaConvertToNUpPixa");

    if (!pixas)
        return (PIXA *)ERROR_PTR("pixas not defined", procName, NULL);
    if (nx < 1 || ny < 1 || nx > 50 || ny > 50)
        return (PIXA *)ERROR_PTR("invalid tiling N-factor", procName, NULL);
    if (tw < 20)
        return (PIXA *)ERROR_PTR("tw must be >= 20", procName, NULL);
    if (fontsize < 0 || fontsize > 20 || fontsize & 1 || fontsize == 2)
        return (PIXA *)ERROR_PTR("invalid fontsize", procName, NULL);

    nt = pixaGetCount(pixas);
    if (sa && (sarrayGetCount(sa) != nt)) {
        L_WARNING("pixa size %d not equal to sarray size %d\n", procName,
                  nt, sarrayGetCount(sa));
    }

    n2 = nx * ny;
    nout = (nt + n2 - 1) / n2;
    pixad = pixaCreate(nout);
    bmf = (fontsize == 0) ? NULL : bmfCreate(NULL, fontsize);
    for (i = 0, j = 0; i < nout; i++) {
        pixa1 = pixaCreate(n2);
        for (k = 0; k < n2 && j < nt; j++, k++) {
            pix1 = pixaGetPix(pixas, j, L_CLONE);
            pix2 = pixScaleToSize(pix1, tw, 0);  /* all images have width tw */
            if (bmf && sa) {
                str = sarrayGetString(sa, j, L_NOCOPY);
                pix3 = pixAddTextlines(pix2, bmf, str, 0xff000000,
                                       L_ADD_BELOW);
            } else {
                pix3 = pixClone(pix2);
            }
            pixaAddPix(pixa1, pix3, L_INSERT);
            pixDestroy(&pix1);
            pixDestroy(&pix2);
        }
        if (pixaGetCount(pixa1) == 0) {  /* probably won't happen */
            pixaDestroy(&pixa1);
            continue;
        }

            /* Add 2 * border to image width to prevent scaling */
        pixaGetRenderingDepth(pixa1, &d);
        pix4 = pixaDisplayTiledAndScaled(pixa1, d, tw + 2 * border, nx, 0,
                                         spacing, border);
        pixaAddPix(pixad, pix4, L_INSERT);
        pixaDestroy(&pixa1);
    }

    bmfDestroy(&bmf);
    return pixad;
}


/*---------------------------------------------------------------------*
 *            Render two pixa side-by-side for comparison              *
 *---------------------------------------------------------------------*/
/*!
 * \brief   pixaCompareInPdf()
 *
 * \param[in]    pixa1
 * \param[in]    pixa2
 * \param[in]    nx, ny     in [1, ... 20], tiling factors in each direction
 * \param[in]    tw         target width, in pixels; must be >= 20
 * \param[in]    spacing    between images, and on outside
 * \param[in]    border     width of additional black border on each image
 *                          and on each pair; use 0 for no border
 * \param[in]    fontsize   to print index of each pair of images.  Valid set
 *                          is {4,6,8,10,12,14,16,18,20}.  Use 0 to disable.
 * \param[in]    fileout    output pdf file
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) This takes two pixa and renders them interleaved, side-by-side
 *          in a pdf.  A warning is issued if the input pixa arrays
 *          have different lengths.
 *      (2) %nx and %ny specify how many side-by-side pairs are displayed
 *          on each pdf page.  For example, if %nx = 1 and %ny = 2, then
 *          two pairs are shown, one above the other, on each page.
 *      (3) The input pix are scaled to a target width of %tw, and
 *          then paired with optional %spacing between and optional
 *          black border of width %border.
 *      (4) After a pixa is generated of these tiled images, it is
 *          written to %fileout as a pdf.
 *      (5) Typical numbers for the input parameters are:
 *            %nx = small integer (1 - 4)
 *            %ny = 2 * %nx
 *            %tw = 200 - 500 pixels
 *            %spacing = 10
 *            %border = 2
 *            %fontsize = 10
 *      (6) If %fontsize != 0, the index of the pix pair in their pixa
 *          is printed out below each pair.
 * </pre>
 */
l_int32
pixaCompareInPdf(PIXA        *pixa1,
                 PIXA        *pixa2,
                 l_int32      nx,
                 l_int32      ny,
                 l_int32      tw,
                 l_int32      spacing,
                 l_int32      border,
                 l_int32      fontsize,
                 const char  *fileout)
{
l_int32  n1, n2, npairs;
PIXA    *pixa3, *pixa4, *pixa5;
SARRAY  *sa;

    PROCNAME("pixaCompareInPdf");

    if (!pixa1 || !pixa2)
        return ERROR_INT("pixa1 and pixa2 not both defined", procName, 1);
    if (nx < 1 || ny < 1 || nx > 20 || ny > 20)
        return ERROR_INT("invalid tiling factors", procName, 1);
    if (tw < 20)
        return ERROR_INT("invalid tw; tw must be >= 20", procName, 1);
    if (fontsize < 0 || fontsize > 20 || fontsize & 1 || fontsize == 2)
        return ERROR_INT("invalid fontsize", procName, 1);
    if (!fileout)
        return ERROR_INT("fileout not defined", procName, 1);
    n1 = pixaGetCount(pixa1);
    n2 = pixaGetCount(pixa2);
    if (n1 == 0 || n2 == 0)
        return ERROR_INT("at least one pixa is empty", procName, 1);
    if (n1 != n2)
        L_WARNING("sizes (%d, %d) differ; using the minimum in interleave\n",
                  procName, n1, n2);

        /* Interleave the input pixa */
    if ((pixa3 = pixaInterleave(pixa1, pixa2, L_CLONE)) == NULL)
        return ERROR_INT("pixa3 not made", procName, 1);

        /* Scale the images if necessary and pair them up side/by/side */
    pixa4 = pixaConvertToNUpPixa(pixa3, NULL, 2, 1, tw, spacing, border, 0);
    pixaDestroy(&pixa3);

        /* Label the pairs and mosaic into pages without further scaling */
    npairs = pixaGetCount(pixa4);
    sa = (fontsize > 0) ? sarrayGenerateIntegers(npairs) : NULL;
    pixa5 = pixaConvertToNUpPixa(pixa4, sa, nx, ny,
                                 2 * tw + 4 * border + spacing,
                                 spacing, border, fontsize);
    pixaDestroy(&pixa4);
    sarrayDestroy(&sa);

        /* Output as pdf without scaling */
    pixaConvertToPdf(pixa5, 0, 1.0, 0, 0, NULL, fileout);
    pixaDestroy(&pixa5);
    return 0;
}


