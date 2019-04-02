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
 * \file pdfio2.c
 * <pre>
 *
 *    Lower-level operations for generating pdf.
 *
 *     Intermediate function for single page, multi-image conversion
 *          l_int32              pixConvertToPdfData()
 *
 *     Intermediate function for generating multipage pdf output
 *          l_int32              ptraConcatenatePdfToData()
 *
 *     Convert tiff multipage to pdf file
 *          l_int32              convertTiffMultipageToPdf()
 *
 *     Low-level CID-based operations
 *
 *       Without transcoding
 *          l_int32              l_generateCIDataForPdf()
 *          L_COMP_DATA         *l_generateFlateDataPdf()
 *          L_COMP_DATA         *l_generateJpegData()
 *          L_COMP_DATA         *l_generateJpegDataMem()
 *          static L_COMP_DATA  *l_generateJp2kData()
 *
 *       With transcoding
 *          l_int32              l_generateCIData()
 *          static l_int32       pixGenerateCIData()
 *          L_COMP_DATA         *l_generateFlateData()
 *          static L_COMP_DATA  *pixGenerateFlateData()
 *          static L_COMP_DATA  *pixGenerateJpegData()
 *          static L_COMP_DATA  *pixGenerateJp2kData()
 *          static L_COMP_DATA  *pixGenerateG4Data()
 *          L_COMP_DATA         *l_generateG4Data()
 *
 *       Other
 *          l_int32              cidConvertToPdfData()
 *          void                 l_CIDataDestroy()
 *
 *     Helper functions for generating the output pdf string
 *          static l_int32       l_generatePdf()
 *          static void          generateFixedStringsPdf()
 *          static char         *generateEscapeString()
 *          static void          generateMediaboxPdf()
 *          static l_int32       generatePageStringPdf()
 *          static l_int32       generateContentStringPdf()
 *          static l_int32       generatePreXStringsPdf()
 *          static l_int32       generateColormapStringsPdf()
 *          static void          generateTrailerPdf()
 *          static l_int32       makeTrailerStringPdf()
 *          static l_int32       generateOutputDataPdf()
 *
 *     Helper functions for generating multipage pdf output
 *          static l_int32       parseTrailerPdf()
 *          static char         *generatePagesObjStringPdf()
 *          static L_BYTEA      *substituteObjectNumbers()
 *
 *     Create/destroy/access pdf data
 *          static L_PDF_DATA   *pdfdataCreate()
 *          static void          pdfdataDestroy()
 *          static L_COMP_DATA  *pdfdataGetCid()
 *
 *     Set flags for special modes
 *          void                 l_pdfSetG4ImageMask()
 *          void                 l_pdfSetDateAndVersion()
 * </pre>
 */

#include <string.h>
#include <math.h>
#include "allheaders.h"

/* --------------------------------------------*/
#if  USE_PDFIO   /* defined in environ.h */
 /* --------------------------------------------*/

    /* Typical scan resolution in ppi (pixels/inch) */
static const l_int32  DEFAULT_INPUT_RES = 300;

    /* Static helpers */
static L_COMP_DATA  *l_generateJp2kData(const char *fname);
static L_COMP_DATA  *pixGenerateFlateData(PIX *pixs, l_int32 ascii85flag);
static L_COMP_DATA  *pixGenerateJpegData(PIX *pixs, l_int32 ascii85flag,
                                         l_int32 quality);
static L_COMP_DATA  *pixGenerateJp2kData(PIX *pixs, l_int32 quality);
static L_COMP_DATA  *pixGenerateG4Data(PIX *pixs, l_int32 ascii85flag);

static l_int32       l_generatePdf(l_uint8 **pdata, size_t *pnbytes,
                                   L_PDF_DATA  *lpd);
static void          generateFixedStringsPdf(L_PDF_DATA *lpd);
static char         *generateEscapeString(const char  *str);
static void          generateMediaboxPdf(L_PDF_DATA *lpd);
static l_int32       generatePageStringPdf(L_PDF_DATA *lpd);
static l_int32       generateContentStringPdf(L_PDF_DATA *lpd);
static l_int32       generatePreXStringsPdf(L_PDF_DATA *lpd);
static l_int32       generateColormapStringsPdf(L_PDF_DATA *lpd);
static void          generateTrailerPdf(L_PDF_DATA *lpd);
static char         *makeTrailerStringPdf(L_DNA *daloc);
static l_int32       generateOutputDataPdf(l_uint8 **pdata, size_t *pnbytes,
                                       L_PDF_DATA *lpd);

static l_int32       parseTrailerPdf(L_BYTEA *bas, L_DNA **pda);
static char         *generatePagesObjStringPdf(NUMA *napage);
static L_BYTEA      *substituteObjectNumbers(L_BYTEA *bas, NUMA *na_objs);

static L_PDF_DATA   *pdfdataCreate(const char *title);
static void          pdfdataDestroy(L_PDF_DATA **plpd);
static L_COMP_DATA  *pdfdataGetCid(L_PDF_DATA *lpd, l_int32 index);


/* ---------------- Defaults for rendering options ----------------- */
    /* Output G4 as writing through image mask; this is the default */
static l_int32   var_WRITE_G4_IMAGE_MASK = 1;
    /* Write date/time and lib version into pdf; this is the default */
static l_int32   var_WRITE_DATE_AND_VERSION = 1;

#define L_SMALLBUF   256
#define L_BIGBUF    2048   /* must be able to hold hex colormap */


#ifndef  NO_CONSOLE_IO
#define  DEBUG_MULTIPAGE      0
#endif  /* ~NO_CONSOLE_IO */


/*---------------------------------------------------------------------*
 *       Intermediate function for generating multipage pdf output     *
 *---------------------------------------------------------------------*/
/*!
 * \brief   pixConvertToPdfData()
 *
 * \param[in]      pix       all depths; cmap OK
 * \param[in]      type      L_G4_ENCODE, L_JPEG_ENCODE, L_FLATE_ENCODE,
 *                           L_JP2K_ENCODE
 * \param[in]      quality   for jpeg: 1-100; 0 for default (75)
 *                           for jp2k: 27-45; 0 for default (34)
 * \param[out]     pdata     pdf array
 * \param[out]     pnbytes   number of bytes in pdf array
 * \param[in]      x, y      location of lower-left corner of image, in pixels,
 *                           relative to the PostScript origin (0,0) at
 *                           the lower-left corner of the page)
 * \param[in]      res       override the resolution of the input image, in ppi;
 *                           use 0 to respect resolution embedded in the input
 * \param[in]      title     [optional] pdf title; can be null
 * \param[in,out]  plpd      ptr to lpd; created on the first invocation and
 *                           returned until last image is processed
 * \param[in]      position  in image sequence: L_FIRST_IMAGE, L_NEXT_IMAGE,
 *                           L_LAST_IMAGE
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) If %res == 0 and the input resolution field is 0,
 *          this will use DEFAULT_INPUT_RES.
 *      (2) This only writes %data if it is the last image to be
 *          written on the page.
 *      (3) See comments in convertToPdf().
 * </pre>
 */
l_ok
pixConvertToPdfData(PIX          *pix,
                    l_int32       type,
                    l_int32       quality,
                    l_uint8     **pdata,
                    size_t       *pnbytes,
                    l_int32       x,
                    l_int32       y,
                    l_int32       res,
                    const char   *title,
                    L_PDF_DATA  **plpd,
                    l_int32       position)
{
l_int32       pixres, w, h, ret;
l_float32     xpt, ypt, wpt, hpt;
L_COMP_DATA  *cid = NULL;
L_PDF_DATA   *lpd = NULL;

    PROCNAME("pixConvertToPdfData");

    if (!pdata)
        return ERROR_INT("&data not defined", procName, 1);
    *pdata = NULL;
    if (!pnbytes)
        return ERROR_INT("&nbytes not defined", procName, 1);
    *pnbytes = 0;
    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if (plpd) {  /* part of multi-page invocation */
        if (position == L_FIRST_IMAGE)
            *plpd = NULL;
    }

        /* Generate the compressed image data.  It must NOT
         * be ascii85 encoded. */
    pixGenerateCIData(pix, type, quality, 0, &cid);
    if (!cid)
        return ERROR_INT("cid not made", procName, 1);

        /* Get media box in pts.  Guess the input image resolution
         * based on the input parameter %res, the resolution data in
         * the pix, and the size of the image. */
    pixres = cid->res;
    w = cid->w;
    h = cid->h;
    if (res <= 0.0) {
        if (pixres > 0)
            res = pixres;
        else
            res = DEFAULT_INPUT_RES;
    }
    xpt = x * 72. / res;
    ypt = y * 72. / res;
    wpt = w * 72. / res;
    hpt = h * 72. / res;

        /* Set up lpd */
    if (!plpd) {  /* single image */
        if ((lpd = pdfdataCreate(title)) == NULL)
            return ERROR_INT("lpd not made", procName, 1);
    } else if (position == L_FIRST_IMAGE) {  /* first of multiple images */
        if ((lpd = pdfdataCreate(title)) == NULL)
            return ERROR_INT("lpd not made", procName, 1);
        *plpd = lpd;
    } else {  /* not the first of multiple images */
        lpd = *plpd;
    }

        /* Add the data to the lpd */
    ptraAdd(lpd->cida, cid);
    lpd->n++;
    ptaAddPt(lpd->xy, xpt, ypt);
    ptaAddPt(lpd->wh, wpt, hpt);

        /* If a single image or the last of multiple images,
         * generate the pdf and destroy the lpd */
    if (!plpd || (position == L_LAST_IMAGE)) {
        ret = l_generatePdf(pdata, pnbytes, lpd);
        pdfdataDestroy(&lpd);
        if (plpd) *plpd = NULL;
        if (ret)
            return ERROR_INT("pdf output not made", procName, 1);
    }

    return 0;
}


/*---------------------------------------------------------------------*
 *      Intermediate function for generating multipage pdf output      *
 *---------------------------------------------------------------------*/
/*!
 * \brief   ptraConcatenatePdfToData()
 *
 * \param[in]    pa_data    ptra array of pdf strings, each for a
 *                          single-page pdf file
 * \param[in]    sa         [optional] string array of pathnames for
 *                          input pdf files; can be null
 * \param[out]   pdata      concatenated pdf data in memory
 * \param[out]   pnbytes    number of bytes in pdf data
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) This only works with leptonica-formatted single-page pdf files.
 *          pdf files generated by other programs will have unpredictable
 *          (and usually bad) results.  The requirements for each pdf file:
 *            (a) The Catalog and Info objects are the first two.
 *            (b) Object 3 is Pages
 *            (c) Object 4 is Page
 *            (d) The remaining objects are Contents, XObjects, and ColorSpace
 *      (2) We remove trailers from each page, and append the full trailer
 *          for all pages at the end.
 *      (3) For all but the first file, remove the ID and the first 3
 *          objects (catalog, info, pages), so that each subsequent
 *          file has only objects of these classes:
 *              Page, Contents, XObject, ColorSpace (Indexed RGB).
 *          For those objects, we substitute these refs to objects
 *          in the local file:
 *              Page:  Parent(object 3), Contents, XObject(typically multiple)
 *              XObject:  [ColorSpace if indexed]
 *          The Pages object on the first page (object 3) has a Kids array
 *          of references to all the Page objects, with a Count equal
 *          to the number of pages.  Each Page object refers back to
 *          this parent.
 * </pre>
 */
l_ok
ptraConcatenatePdfToData(L_PTRA    *pa_data,
                         SARRAY    *sa,
                         l_uint8  **pdata,
                         size_t    *pnbytes)
{
char     *fname, *str_pages, *str_trailer;
l_uint8  *pdfdata, *data;
l_int32   i, j, index, nobj, npages;
l_int32  *sizes, *locs;
size_t    size;
L_BYTEA  *bas, *bad, *bat1, *bat2;
L_DNA    *da_locs, *da_sizes, *da_outlocs, *da;
L_DNAA   *daa_locs;  /* object locations on each page */
NUMA     *na_objs, *napage;
NUMAA    *naa_objs;  /* object mapping numbers to new values */

    PROCNAME("ptraConcatenatePdfToData");

    if (!pdata)
        return ERROR_INT("&data not defined", procName, 1);
    *pdata = NULL;
    if (!pnbytes)
        return ERROR_INT("&nbytes not defined", procName, 1);
    *pnbytes = 0;
    if (!pa_data)
        return ERROR_INT("pa_data not defined", procName, 1);

        /* Parse the files and find the object locations.
         * Remove file data that cannot be parsed. */
    ptraGetActualCount(pa_data, &npages);
    daa_locs = l_dnaaCreate(npages);
    for (i = 0; i < npages; i++) {
        bas = (L_BYTEA *)ptraGetPtrToItem(pa_data, i);
        if (parseTrailerPdf(bas, &da_locs) != 0) {
            bas = (L_BYTEA *)ptraRemove(pa_data, i, L_NO_COMPACTION);
            l_byteaDestroy(&bas);
            if (sa) {
                fname = sarrayGetString(sa, i, L_NOCOPY);
                L_ERROR("can't parse file %s; skipping\n", procName, fname);
            } else {
                L_ERROR("can't parse file %d; skipping\n", procName, i);
            }
        } else {
            l_dnaaAddDna(daa_locs, da_locs, L_INSERT);
        }
    }

        /* Recompute npages in case some of the files were not pdf */
    ptraCompactArray(pa_data);
    ptraGetActualCount(pa_data, &npages);
    if (npages == 0) {
        l_dnaaDestroy(&daa_locs);
        return ERROR_INT("no parsable pdf files found", procName, 1);
    }

        /* Find the mapping from initial to final object numbers */
    naa_objs = numaaCreate(npages);  /* stores final object numbers */
    napage = numaCreate(npages);  /* stores "Page" object numbers */
    index = 0;
    for (i = 0; i < npages; i++) {
        da = l_dnaaGetDna(daa_locs, i, L_CLONE);
        nobj = l_dnaGetCount(da);
        if (i == 0) {
            numaAddNumber(napage, 4);  /* object 4 on first page */
            na_objs = numaMakeSequence(0.0, 1.0, nobj - 1);
            index = nobj - 1;
        } else {  /* skip the first 3 objects in each file */
            numaAddNumber(napage, index);  /* Page object is first we add */
            na_objs = numaMakeConstant(0.0, nobj - 1);
            numaReplaceNumber(na_objs, 3, 3);  /* refers to parent of all */
            for (j = 4; j < nobj - 1; j++)
                numaSetValue(na_objs, j, index++);
        }
        numaaAddNuma(naa_objs, na_objs, L_INSERT);
        l_dnaDestroy(&da);
    }

        /* Make the Pages object (#3) */
    str_pages = generatePagesObjStringPdf(napage);

        /* Build the output */
    bad = l_byteaCreate(5000);
    da_outlocs = l_dnaCreate(0);  /* locations of all output objects */
    for (i = 0; i < npages; i++) {
        bas = (L_BYTEA *)ptraGetPtrToItem(pa_data, i);
        pdfdata = l_byteaGetData(bas, &size);
        da_locs = l_dnaaGetDna(daa_locs, i, L_CLONE);  /* locs on this page */
        na_objs = numaaGetNuma(naa_objs, i, L_CLONE);  /* obj # on this page */
        nobj = l_dnaGetCount(da_locs) - 1;
        da_sizes = l_dnaDiffAdjValues(da_locs);  /* object sizes on this page */
        sizes = l_dnaGetIArray(da_sizes);
        locs = l_dnaGetIArray(da_locs);
        if (i == 0) {
            l_byteaAppendData(bad, pdfdata, sizes[0]);
            l_byteaAppendData(bad, pdfdata + locs[1], sizes[1]);
            l_byteaAppendData(bad, pdfdata + locs[2], sizes[2]);
            l_byteaAppendString(bad, str_pages);
            for (j = 0; j < 4; j++)
                l_dnaAddNumber(da_outlocs, locs[j]);
        }
        for (j = 4; j < nobj; j++) {
            l_dnaAddNumber(da_outlocs, l_byteaGetSize(bad));
            bat1 = l_byteaInitFromMem(pdfdata + locs[j], sizes[j]);
            bat2 = substituteObjectNumbers(bat1, na_objs);
            data = l_byteaGetData(bat2, &size);
            l_byteaAppendData(bad, data, size);
            l_byteaDestroy(&bat1);
            l_byteaDestroy(&bat2);
        }
        if (i == npages - 1)  /* last one */
            l_dnaAddNumber(da_outlocs, l_byteaGetSize(bad));
        LEPT_FREE(sizes);
        LEPT_FREE(locs);
        l_dnaDestroy(&da_locs);
        numaDestroy(&na_objs);
        l_dnaDestroy(&da_sizes);
    }

        /* Add the trailer */
    str_trailer = makeTrailerStringPdf(da_outlocs);
    l_byteaAppendString(bad, str_trailer);

        /* Transfer the output data */
    *pdata = l_byteaCopyData(bad, pnbytes);
    l_byteaDestroy(&bad);

#if  DEBUG_MULTIPAGE
    fprintf(stderr, "******** object mapper **********");
    numaaWriteStream(stderr, naa_objs);

    fprintf(stderr, "******** Page object numbers ***********");
    numaWriteStream(stderr, napage);

    fprintf(stderr, "******** Pages object ***********\n");
    fprintf(stderr, "%s\n", str_pages);
#endif  /* DEBUG_MULTIPAGE */

    numaDestroy(&napage);
    numaaDestroy(&naa_objs);
    l_dnaDestroy(&da_outlocs);
    l_dnaaDestroy(&daa_locs);
    LEPT_FREE(str_pages);
    LEPT_FREE(str_trailer);
    return 0;
}


/*---------------------------------------------------------------------*
 *                  Convert tiff multipage to pdf file                 *
 *---------------------------------------------------------------------*/
/*!
 * \brief   convertTiffMultipageToPdf()
 *
 * \param[in]    filein    (tiff)
 * \param[in]    fileout   (pdf)
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) A multipage tiff file can also be converted to PS, using
 *          convertTiffMultipageToPS()
 * </pre>
 */
l_ok
convertTiffMultipageToPdf(const char  *filein,
                          const char  *fileout)
{
l_int32  istiff;
PIXA    *pixa;
FILE    *fp;

    PROCNAME("convertTiffMultipageToPdf");

    if ((fp = fopenReadStream(filein)) == NULL)
        return ERROR_INT("file not found", procName, 1);
    istiff = fileFormatIsTiff(fp);
    fclose(fp);
    if (!istiff)
        return ERROR_INT("file not tiff format", procName, 1);

    pixa = pixaReadMultipageTiff(filein);
    pixaConvertToPdf(pixa, 0, 1.0, 0, 0, "weasel2", fileout);
    pixaDestroy(&pixa);
    return 0;
}


/*---------------------------------------------------------------------*
 *                     Low-level CID-based operations                  *
 *---------------------------------------------------------------------*/
/*!
 * \brief   l_generateCIDataForPdf()
 *
 * \param[in]    fname      [optional] can be null
 * \param[in]    pix        [optional] can be null
 * \param[in]    quality    for jpeg if transcoded: 1-100; 0 for default (75)
 *                          for jp2k if transcoded: 27-45; 0 for default (34)
 * \param[out]   pcid       compressed data
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) You must set either filename or pix.
 *      (2) Given an image file and optionally a pix raster of that data,
 *          this provides a CID that is compatible with PDF, preferably
 *          without transcoding.
 *      (3) The pix is included for efficiency, in case transcoding
 *          is required and the pix is available to the caller.
 *      (4) We don't try to open files named "stdin" or "-" for Tesseract
 *          compatibility reasons. We may remove this restriction
 *          in the future.
 * </pre>
 */
l_ok
l_generateCIDataForPdf(const char    *fname,
                       PIX           *pix,
                       l_int32        quality,
                       L_COMP_DATA  **pcid)
{
l_int32       format, type;
L_COMP_DATA  *cid;
PIX          *pixt;

    PROCNAME("l_generateCIDataForPdf");

    if (!pcid)
        return ERROR_INT("&cid not defined", procName, 1);
    *pcid = cid = NULL;
    if (!fname && !pix)
        return ERROR_INT("neither fname nor pix are defined", procName, 1);

        /* If a compressed file is given that is not 'stdin', see if we
         * can generate the pdf output without transcoding. */
    if (fname && strcmp(fname, "-") != 0 && strcmp(fname, "stdin") != 0) {
        findFileFormat(fname, &format);
        if (format == IFF_UNKNOWN)
            L_WARNING("file %s format is unknown\n", procName, fname);
        if (format == IFF_PS || format == IFF_LPDF) {
            L_ERROR("file %s is unsupported format %d\n",
                  procName, fname, format);
            return 1;
        }
        if (format == IFF_JFIF_JPEG) {
            cid = l_generateJpegData(fname, 0);
        } else if (format == IFF_JP2) {
            cid = l_generateJp2kData(fname);
        } else if (format == IFF_PNG) {
            cid = l_generateFlateDataPdf(fname, pix);
        }

    }

        /* Otherwise, use the pix to generate the pdf output */
    if  (!cid) {
        if (!pix)
            pixt = pixRead(fname);
        else
            pixt = pixClone(pix);
        if (!pixt)
            return ERROR_INT("pixt not made", procName, 1);
        selectDefaultPdfEncoding(pixt, &type);
        pixGenerateCIData(pixt, type, quality, 0, &cid);
        pixDestroy(&pixt);
    }
    if (!cid) {
        L_ERROR("totally kerflummoxed\n", procName);
        return 1;
    }
    *pcid = cid;
    return 0;
}


/*!
 * \brief   l_generateFlateDataPdf()
 *
 * \param[in]    fname     preferably png
 * \param[in]    pixs      [optional] can be null
 * \return  cid containing png data, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) If you hand this a png file, you are going to get
 *          png predictors embedded in the flate data. So it has
 *          come to this. http://xkcd.com/1022/
 *      (2) Exception: if the png is interlaced or if it is RGBA,
 *          it will be transcoded.
 *      (3) If transcoding is required, this will not have to read from
 *          file if you also input a pix.
 * </pre>
 */
L_COMP_DATA *
l_generateFlateDataPdf(const char  *fname,
                       PIX         *pixs)
{
l_uint8      *pngcomp = NULL;  /* entire PNG compressed file */
l_uint8      *datacomp = NULL;  /* gzipped raster data */
l_uint8      *cmapdata = NULL;  /* uncompressed colormap */
char         *cmapdatahex = NULL;  /* hex ascii uncompressed colormap */
l_uint32      i, j, n;
l_int32       format, interlaced;
l_int32       ncolors;  /* in colormap */
l_int32       bps;  /* bits/sample: usually 8 */
l_int32       spp;  /* samples/pixel: 1-grayscale/cmap); 3-rgb; 4-rgba */
l_int32       w, h, cmapflag;
l_int32       xres, yres;
size_t        nbytescomp = 0, nbytespng = 0;
FILE         *fp;
L_COMP_DATA  *cid;
PIX          *pix;
PIXCMAP      *cmap = NULL;

    PROCNAME("l_generateFlateDataPdf");

    if (!fname)
        return (L_COMP_DATA *)ERROR_PTR("fname not defined", procName, NULL);

    findFileFormat(fname, &format);
    spp = 0;  /* init to spp != 4 if not png */
    interlaced = 0;  /* initialize to no interlacing */
    bps = 0;  /* initialize to a nonsense value */
    if (format == IFF_PNG) {
        isPngInterlaced(fname, &interlaced);
        readHeaderPng(fname, NULL, NULL, &bps, &spp, NULL);
    }

        /* PDF is capable of inlining some types of PNG files, but not all
           of them. We need to transcode anything with interlacing, an
           alpha channel, or 1 bpp (which would otherwise be photo-inverted).

           Be careful with spp. Any PNG image file with an alpha
           channel is converted on reading to RGBA (spp == 4). This
           includes the (gray + alpha) format with spp == 2. You
           will get different results if you look at spp via
           readHeaderPng() versus pixGetSpp() */
    if (format != IFF_PNG || interlaced || bps == 1 || spp == 4 || spp == 2) {
        if (!pixs)
            pix = pixRead(fname);
        else
            pix = pixClone(pixs);
        if (!pix)
            return (L_COMP_DATA *)ERROR_PTR("pix not made", procName, NULL);
        cid = pixGenerateFlateData(pix, 0);
        pixDestroy(&pix);
        return cid;
    }

        /* It's png.  Generate the pdf data without transcoding.
         * Implementation by Jeff Breidenbach.
         * First, read the metadata */
    if ((fp = fopenReadStream(fname)) == NULL)
        return (L_COMP_DATA *)ERROR_PTR("stream not opened", procName, NULL);
    freadHeaderPng(fp, &w, &h, &bps, &spp, &cmapflag);
    fgetPngResolution(fp, &xres, &yres);
    fclose(fp);

        /* We get pdf corruption when inlining the data from 16 bpp png. */
    if (bps == 16)
        return l_generateFlateData(fname, 0);

        /* Read the entire png file */
    if ((pngcomp = l_binaryRead(fname, &nbytespng)) == NULL)
        return (L_COMP_DATA *)ERROR_PTR("unable to read file",
                                        procName, NULL);

        /* Extract flate data, copying portions of it to memory, including
         * the predictor information in a byte at the beginning of each
         * raster line.  The flate data makes up the vast majority of
         * the png file, so after extraction we expect datacomp to
         * be nearly full (i.e., nbytescomp will be only slightly less
         * than nbytespng).  Also extract the colormap if present. */
    if ((datacomp = (l_uint8 *)LEPT_CALLOC(1, nbytespng)) == NULL) {
        LEPT_FREE(pngcomp);
        return (L_COMP_DATA *)ERROR_PTR("unable to allocate memory",
                                        procName, NULL);
    }

        /* Parse the png file.  Each chunk consists of:
         *    length: 4 bytes
         *    name:   4 bytes (e.g., "IDAT")
         *    data:   n bytes
         *    CRC:    4 bytes
         * Start at the beginning of the data section of the first chunk,
         * byte 16, because the png file begins with 8 bytes of header,
         * followed by the first 8 bytes of the first chunk
         * (length and name).  On each loop, increment by 12 bytes to
         * skip over the CRC, length and name of the next chunk. */
    for (i = 16; i < nbytespng; i += 12) {  /* do each successive chunk */
            /* Get the chunk length */
        n  = pngcomp[i - 8] << 24;
        n += pngcomp[i - 7] << 16;
        n += pngcomp[i - 6] << 8;
        n += pngcomp[i - 5] << 0;
        if (i + n >= nbytespng) {
            LEPT_FREE(pngcomp);
            LEPT_FREE(datacomp);
            pixcmapDestroy(&cmap);
            L_ERROR("invalid png: i = %d, n = %d, nbytes = %lu\n", procName,
                    i, n, (unsigned long)nbytespng);
            return NULL;
        }

            /* Is it a data chunk? */
        if (memcmp(pngcomp + i - 4, "IDAT", 4) == 0) {
            memcpy(datacomp + nbytescomp, pngcomp + i, n);
            nbytescomp += n;
        }

            /* Is it a palette chunk? */
        if (cmapflag && !cmap &&
            memcmp(pngcomp + i - 4, "PLTE", 4) == 0) {
            if ((n / 3) > (1 << bps)) {
                LEPT_FREE(pngcomp);
                LEPT_FREE(datacomp);
                pixcmapDestroy(&cmap);
                L_ERROR("invalid png: i = %d, n = %d, cmapsize = %d\n",
                        procName, i, n, (1 << bps));
                return NULL;
            }
            cmap = pixcmapCreate(bps);
            for (j = i; j < i + n; j += 3) {
                pixcmapAddColor(cmap, pngcomp[j], pngcomp[j + 1],
                                pngcomp[j + 2]);
            }
        }
        i += n;  /* move to the end of the data chunk */
    }
    LEPT_FREE(pngcomp);

    if (nbytescomp == 0) {
        LEPT_FREE(datacomp);
        pixcmapDestroy(&cmap);
        return (L_COMP_DATA *)ERROR_PTR("invalid PNG file", procName, NULL);
    }

        /* Extract and encode the colormap data as hexascii  */
    ncolors = 0;
    if (cmap) {
        pixcmapSerializeToMemory(cmap, 3, &ncolors, &cmapdata);
        pixcmapDestroy(&cmap);
        if (!cmapdata) {
            LEPT_FREE(datacomp);
            return (L_COMP_DATA *)ERROR_PTR("cmapdata not made",
                                            procName, NULL);
        }
        cmapdatahex = pixcmapConvertToHex(cmapdata, ncolors);
        LEPT_FREE(cmapdata);
    }

        /* Note that this is the only situation where the predictor
         * field of the CID is set to 1.  Adobe's predictor values on
         * p. 76 of pdf_reference_1-7.pdf give 1 for no predictor and
         * 10-14 for inline predictors, the specifics of which are
         * ignored by the pdf interpreter, which just needs to know that
         * the first byte on each compressed scanline is some predictor
         * whose type can be inferred from the byte itself.  */
    cid = (L_COMP_DATA *)LEPT_CALLOC(1, sizeof(L_COMP_DATA));
    cid->datacomp = datacomp;
    cid->type = L_FLATE_ENCODE;
    cid->cmapdatahex = cmapdatahex;
    cid->nbytescomp = nbytescomp;
    cid->ncolors = ncolors;
    cid->predictor = TRUE;
    cid->w = w;
    cid->h = h;
    cid->bps = bps;
    cid->spp = spp;
    cid->res = xres;
    return cid;
}


/*!
 * \brief   l_generateJpegData()
 *
 * \param[in]    fname           of jpeg file
 * \param[in]    ascii85flag     0 for jpeg; 1 for ascii85-encoded jpeg
 * \return  cid containing jpeg data, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) Set ascii85flag:
 *           ~ 0 for binary data (not permitted in PostScript)
 *           ~ 1 for ascii85 (5 for 4) encoded binary data
 *               (not permitted in pdf)
 *      (2) Do not free the data.  l_generateJpegDataMem() will free
 *          the data if it does not use ascii encoding.
 * </pre>
 */
L_COMP_DATA *
l_generateJpegData(const char  *fname,
                   l_int32      ascii85flag)
{
l_uint8      *data = NULL;
size_t        nbytes;

    PROCNAME("l_generateJpegData");

    if (!fname)
        return (L_COMP_DATA *)ERROR_PTR("fname not defined", procName, NULL);

        /* The returned jpeg data in memory is the entire jpeg file,
         * which starts with ffd8 and ends with ffd9 */
    if ((data = l_binaryRead(fname, &nbytes)) == NULL)
        return (L_COMP_DATA *)ERROR_PTR("data not extracted", procName, NULL);

    return l_generateJpegDataMem(data, nbytes, ascii85flag);
}


/*!
 * \brief   l_generateJpegDataMem()
 *
 * \param[in]    data           of jpeg file
 * \param[in]    nbytes         of jpeg file
 * \param[in]    ascii85flag    0 for jpeg; 1 for ascii85-encoded jpeg
 * \return  cid containing jpeg data, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) See l_generateJpegData().
 * </pre>
 */
L_COMP_DATA *
l_generateJpegDataMem(l_uint8  *data,
                      size_t    nbytes,
                      l_int32   ascii85flag)
{
char         *data85 = NULL;  /* ascii85 encoded jpeg compressed file */
l_int32       w, h, xres, yres, bps, spp;
l_int32       nbytes85;
L_COMP_DATA  *cid;

    PROCNAME("l_generateJpegDataMem");

    if (!data)
        return (L_COMP_DATA *)ERROR_PTR("data not defined", procName, NULL);

        /* Read the metadata */
    readHeaderMemJpeg(data, nbytes, &w, &h, &spp, NULL, NULL);
    bps = 8;
    readResolutionMemJpeg(data, nbytes, &xres, &yres);

        /* Optionally, encode the compressed data */
    if (ascii85flag == 1) {
        data85 = encodeAscii85(data, nbytes, &nbytes85);
        LEPT_FREE(data);
        if (!data85)
            return (L_COMP_DATA *)ERROR_PTR("data85 not made", procName, NULL);
        else
            data85[nbytes85 - 1] = '\0';  /* remove the newline */
    }

    cid = (L_COMP_DATA *)LEPT_CALLOC(1, sizeof(L_COMP_DATA));
    if (ascii85flag == 0) {
        cid->datacomp = data;
    } else {  /* ascii85 */
        cid->data85 = data85;
        cid->nbytes85 = nbytes85;
    }
    cid->type = L_JPEG_ENCODE;
    cid->nbytescomp = nbytes;
    cid->w = w;
    cid->h = h;
    cid->bps = bps;
    cid->spp = spp;
    cid->res = xres;
    return cid;
}


/*!
 * \brief   l_generateJp2kData()
 *
 * \param[in]    fname     of jp2k file
 * \return  cid containing jp2k data, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) This is only called after the file is verified to be jp2k.
 * </pre>
 */
static L_COMP_DATA *
l_generateJp2kData(const char  *fname)
{
l_int32       w, h, bps, spp, xres, yres;
size_t        nbytes;
L_COMP_DATA  *cid;
FILE         *fp;

    PROCNAME("l_generateJp2kData");

    if (!fname)
        return (L_COMP_DATA *)ERROR_PTR("fname not defined", procName, NULL);

    if ((cid = (L_COMP_DATA *)LEPT_CALLOC(1, sizeof(L_COMP_DATA))) == NULL)
        return (L_COMP_DATA *)ERROR_PTR("cid not made", procName, NULL);

        /* The returned jp2k data in memory is the entire jp2k file */
    if ((cid->datacomp = l_binaryRead(fname, &nbytes)) == NULL) {
        l_CIDataDestroy(&cid);
        return (L_COMP_DATA *)ERROR_PTR("data not extracted", procName, NULL);
    }

    readHeaderJp2k(fname, &w, &h, &bps, &spp);
    xres = yres = 0;
    if ((fp = fopenReadStream(fname)) != NULL) {
        fgetJp2kResolution(fp, &xres, &yres);
        fclose(fp);
    }
    cid->type = L_JP2K_ENCODE;
    cid->nbytescomp = nbytes;
    cid->w = w;
    cid->h = h;
    cid->bps = bps;
    cid->spp = spp;
    cid->res = xres;
    return cid;
}


/*!
 * \brief   l_generateCIData()
 *
 * \param[in]    fname
 * \param[in]    type       L_G4_ENCODE, L_JPEG_ENCODE, L_FLATE_ENCODE,
 *                          L_JP2K_ENCODE
 * \param[in]    quality    for jpeg if transcoded: 1-100; 0 for default (75)
 *                          for jp2k if transcoded: 27-45; 0 for default (34)
 * \param[in]    ascii85    0 for binary; 1 for ascii85-encoded
 * \param[out]   pcid       compressed data
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) This can be used for both PostScript and pdf.
 *      (1) Set ascii85:
 *           ~ 0 for binary data (not permitted in PostScript)
 *           ~ 1 for ascii85 (5 for 4) encoded binary data
 *      (2) This attempts to compress according to the requested type.
 *          If this can't be done, it falls back to ordinary flate encoding.
 *      (3) This differs from l_generateCIDataPdf(), which determines
 *          the format and attempts to generate the CID without transcoding.
 * </pre>
 */
l_ok
l_generateCIData(const char    *fname,
                 l_int32        type,
                 l_int32        quality,
                 l_int32        ascii85,
                 L_COMP_DATA  **pcid)
{
l_int32       format, d, bps, spp, iscmap;
L_COMP_DATA  *cid;
PIX          *pix;

    PROCNAME("l_generateCIData");

    if (!pcid)
        return ERROR_INT("&cid not defined", procName, 1);
    *pcid = NULL;
    if (!fname)
        return ERROR_INT("fname not defined", procName, 1);
    if (type != L_G4_ENCODE && type != L_JPEG_ENCODE &&
        type != L_FLATE_ENCODE && type != L_JP2K_ENCODE)
        return ERROR_INT("invalid conversion type", procName, 1);
    if (ascii85 != 0 && ascii85 != 1)
        return ERROR_INT("invalid ascii85", procName, 1);

        /* Sanity check on requested encoding */
    pixReadHeader(fname, &format, NULL, NULL, &bps, &spp, &iscmap);
    d = bps * spp;
    if (d == 24) d = 32;
    if (iscmap && type != L_FLATE_ENCODE) {
        L_WARNING("pixs has cmap; using flate encoding\n", procName);
        type = L_FLATE_ENCODE;
    } else if (d < 8 && type == L_JPEG_ENCODE) {
        L_WARNING("pixs has < 8 bpp; using flate encoding\n", procName);
        type = L_FLATE_ENCODE;
    } else if (d < 8 && type == L_JP2K_ENCODE) {
        L_WARNING("pixs has < 8 bpp; using flate encoding\n", procName);
        type = L_FLATE_ENCODE;
    } else if (d > 1 && type == L_G4_ENCODE) {
        L_WARNING("pixs has > 1 bpp; using flate encoding\n", procName);
        type = L_FLATE_ENCODE;
    }

    if (type == L_JPEG_ENCODE) {
        if (format == IFF_JFIF_JPEG) {  /* do not transcode */
            cid = l_generateJpegData(fname, ascii85);
        } else {
            if ((pix = pixRead(fname)) == NULL)
                return ERROR_INT("pix not returned", procName, 1);
            cid = pixGenerateJpegData(pix, ascii85, quality);
            pixDestroy(&pix);
        }
        if (!cid)
            return ERROR_INT("jpeg data not made", procName, 1);
    } else if (type == L_JP2K_ENCODE) {
        if (format == IFF_JP2) {  /* do not transcode */
            cid = l_generateJp2kData(fname);
        } else {
            if ((pix = pixRead(fname)) == NULL)
                return ERROR_INT("pix not returned", procName, 1);
            cid = pixGenerateJp2kData(pix, quality);
            pixDestroy(&pix);
        }
        if (!cid)
            return ERROR_INT("jpeg data not made", procName, 1);
    } else if (type == L_G4_ENCODE) {
        if ((cid = l_generateG4Data(fname, ascii85)) == NULL)
            return ERROR_INT("g4 data not made", procName, 1);
    } else if (type == L_FLATE_ENCODE) {
        if ((cid = l_generateFlateData(fname, ascii85)) == NULL)
            return ERROR_INT("flate data not made", procName, 1);
    } else {
        return ERROR_INT("invalid conversion type", procName, 1);
    }
    *pcid = cid;

    return 0;
}


/*!
 * \brief   pixGenerateCIData()
 *
 * \param[in]    pixs       8 or 32 bpp, no colormap
 * \param[in]    type       L_G4_ENCODE, L_JPEG_ENCODE, L_FLATE_ENCODE or
 *                          L_JP2K_ENCODE
 * \param[in]    quality    for jpeg if transcoded: 1-100; 0 for default (75)
 *                          for jp2k if transcoded: 27-45; 0 for default (34)
 * \param[in]    ascii85    0 for binary; 1 for ascii85-encoded
 * \param[out]   pcid       compressed data
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) Set ascii85:
 *           ~ 0 for binary data (not permitted in PostScript)
 *           ~ 1 for ascii85 (5 for 4) encoded binary data
 * </pre>
 */
l_ok
pixGenerateCIData(PIX           *pixs,
                  l_int32        type,
                  l_int32        quality,
                  l_int32        ascii85,
                  L_COMP_DATA  **pcid)
{
l_int32   d;
PIXCMAP  *cmap;

    PROCNAME("pixGenerateCIData");

    if (!pcid)
        return ERROR_INT("&cid not defined", procName, 1);
    *pcid = NULL;
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (type != L_G4_ENCODE && type != L_JPEG_ENCODE &&
        type != L_FLATE_ENCODE && type != L_JP2K_ENCODE)
        return ERROR_INT("invalid conversion type", procName, 1);
    if (ascii85 != 0 && ascii85 != 1)
        return ERROR_INT("invalid ascii85", procName, 1);

        /* Sanity check on requested encoding */
    d = pixGetDepth(pixs);
    cmap = pixGetColormap(pixs);
    if (cmap && type != L_FLATE_ENCODE) {
        L_WARNING("pixs has cmap; using flate encoding\n", procName);
        type = L_FLATE_ENCODE;
    } else if (d < 8 && (type == L_JPEG_ENCODE || type == L_JP2K_ENCODE)) {
        L_WARNING("pixs has < 8 bpp; using flate encoding\n", procName);
        type = L_FLATE_ENCODE;
    } else if (d > 1 && type == L_G4_ENCODE) {
        L_WARNING("pixs has > 1 bpp; using flate encoding\n", procName);
        type = L_FLATE_ENCODE;
    }

    if (type == L_JPEG_ENCODE) {
        if ((*pcid = pixGenerateJpegData(pixs, ascii85, quality)) == NULL)
            return ERROR_INT("jpeg data not made", procName, 1);
    } else if (type == L_JP2K_ENCODE) {
        if ((*pcid = pixGenerateJp2kData(pixs, quality)) == NULL)
            return ERROR_INT("jp2k data not made", procName, 1);
    } else if (type == L_G4_ENCODE) {
        if ((*pcid = pixGenerateG4Data(pixs, ascii85)) == NULL)
            return ERROR_INT("g4 data not made", procName, 1);
    } else if (type == L_FLATE_ENCODE) {
        if ((*pcid = pixGenerateFlateData(pixs, ascii85)) == NULL)
            return ERROR_INT("flate data not made", procName, 1);
    } else {
        return ERROR_INT("invalid conversion type", procName, 1);
    }

    return 0;
}


/*!
 * \brief   l_generateFlateData()
 *
 * \param[in]    fname
 * \param[in]    ascii85flag    0 for gzipped; 1 for ascii85-encoded gzipped
 * \return  cid flate compressed image data, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) The input image is converted to one of these 4 types:
 *           ~ 1 bpp
 *           ~ 8 bpp, no colormap
 *           ~ 8 bpp, colormap
 *           ~ 32 bpp rgb
 *      (2) Set ascii85flag:
 *           ~ 0 for binary data (not permitted in PostScript)
 *           ~ 1 for ascii85 (5 for 4) encoded binary data
 * </pre>
 */
L_COMP_DATA *
l_generateFlateData(const char  *fname,
                    l_int32      ascii85flag)
{
L_COMP_DATA  *cid;
PIX          *pixs;

    PROCNAME("l_generateFlateData");

    if (!fname)
        return (L_COMP_DATA *)ERROR_PTR("fname not defined", procName, NULL);

    if ((pixs = pixRead(fname)) == NULL)
        return (L_COMP_DATA *)ERROR_PTR("pixs not made", procName, NULL);
    cid = pixGenerateFlateData(pixs, ascii85flag);
    pixDestroy(&pixs);
    return cid;
}


/*!
 * \brief   pixGenerateFlateData()
 *
 * \param[in]    pixs
 * \param[in]    ascii85flag 0    for gzipped; 1 for ascii85-encoded gzipped
 * \return  cid flate compressed image data, or NULL on error
 *
 *      Notes:
 *          1) This should not be called with an RGBA pix (spp == 4; it
 *             will ignore the alpha channel.  Likewise, if called with a
 *             colormapped pix, the alpha component in the colormap will
 *             be ignored as it is for all leptonica operations
 *             on colormapped pix.
 */
static L_COMP_DATA *
pixGenerateFlateData(PIX     *pixs,
                     l_int32  ascii85flag)
{
l_uint8      *data = NULL;  /* uncompressed raster data in required format */
l_uint8      *datacomp = NULL;  /* gzipped raster data */
char         *data85 = NULL;  /* ascii85 encoded gzipped raster data */
l_uint8      *cmapdata = NULL;  /* uncompressed colormap */
char         *cmapdata85 = NULL;  /* ascii85 encoded uncompressed colormap */
char         *cmapdatahex = NULL;  /* hex ascii uncompressed colormap */
l_int32       ncolors;  /* in colormap; not used if cmapdata85 is null */
l_int32       bps;  /* bits/sample: usually 8 */
l_int32       spp;  /* samples/pixel: 1-grayscale/cmap); 3-rgb */
l_int32       w, h, d, cmapflag;
l_int32       ncmapbytes85 = 0;
l_int32       nbytes85 = 0;
size_t        nbytes, nbytescomp;
L_COMP_DATA  *cid;
PIX          *pixt;
PIXCMAP      *cmap;

    PROCNAME("pixGenerateFlateData");

    if (!pixs)
        return (L_COMP_DATA *)ERROR_PTR("pixs not defined", procName, NULL);

        /* Convert the image to one of these 4 types:
         *     1 bpp
         *     8 bpp, no colormap
         *     8 bpp, colormap
         *     32 bpp rgb    */
    pixGetDimensions(pixs, &w, &h, &d);
    cmap = pixGetColormap(pixs);
    cmapflag = (cmap) ? 1 : 0;
    if (d == 2 || d == 4 || d == 16) {
        pixt = pixConvertTo8(pixs, cmapflag);
        cmap = pixGetColormap(pixt);
        d = pixGetDepth(pixt);
    } else {
        pixt = pixClone(pixs);
    }
    spp = (d == 32) ? 3 : 1;  /* ignores alpha */
    bps = (d == 32) ? 8 : d;

        /* Extract and encode the colormap data as both ascii85 and hexascii  */
    ncolors = 0;
    if (cmap) {
        pixcmapSerializeToMemory(cmap, 3, &ncolors, &cmapdata);
        if (!cmapdata) {
            pixDestroy(&pixt);
            return (L_COMP_DATA *)ERROR_PTR("cmapdata not made",
                                            procName, NULL);
        }

        cmapdata85 = encodeAscii85(cmapdata, 3 * ncolors, &ncmapbytes85);
        cmapdatahex = pixcmapConvertToHex(cmapdata, ncolors);
        LEPT_FREE(cmapdata);
    }

        /* Extract and compress the raster data */
    pixGetRasterData(pixt, &data, &nbytes);
    pixDestroy(&pixt);
    datacomp = zlibCompress(data, nbytes, &nbytescomp);
    LEPT_FREE(data);
    if (!datacomp) {
        LEPT_FREE(cmapdata85);
        LEPT_FREE(cmapdatahex);
        return (L_COMP_DATA *)ERROR_PTR("datacomp not made", procName, NULL);
    }

        /* Optionally, encode the compressed data */
    if (ascii85flag == 1) {
        data85 = encodeAscii85(datacomp, nbytescomp, &nbytes85);
        LEPT_FREE(datacomp);
        if (!data85) {
            LEPT_FREE(cmapdata85);
            LEPT_FREE(cmapdatahex);
            return (L_COMP_DATA *)ERROR_PTR("data85 not made", procName, NULL);
        } else {
            data85[nbytes85 - 1] = '\0';  /* remove the newline */
        }
    }

    cid = (L_COMP_DATA *)LEPT_CALLOC(1, sizeof(L_COMP_DATA));
    if (ascii85flag == 0) {
        cid->datacomp = datacomp;
    } else {  /* ascii85 */
        cid->data85 = data85;
        cid->nbytes85 = nbytes85;
    }
    cid->type = L_FLATE_ENCODE;
    cid->cmapdatahex = cmapdatahex;
    cid->cmapdata85 = cmapdata85;
    cid->nbytescomp = nbytescomp;
    cid->ncolors = ncolors;
    cid->w = w;
    cid->h = h;
    cid->bps = bps;
    cid->spp = spp;
    cid->res = pixGetXRes(pixs);
    cid->nbytes = nbytes;  /* only for debugging */
    return cid;
}


/*!
 * \brief   pixGenerateJpegData()
 *
 * \param[in]    pixs           8 or 32 bpp, no colormap
 * \param[in]    ascii85flag    0 for jpeg; 1 for ascii85-encoded jpeg
 * \param[in]    quality        0 for default, which is 75
 * \return  cid jpeg compressed data, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) Set ascii85flag:
 *           ~ 0 for binary data (not permitted in PostScript)
 *           ~ 1 for ascii85 (5 for 4) encoded binary data
 * </pre>
 */
static L_COMP_DATA *
pixGenerateJpegData(PIX     *pixs,
                    l_int32  ascii85flag,
                    l_int32  quality)
{
l_int32       d;
char         *fname;
L_COMP_DATA  *cid;

    PROCNAME("pixGenerateJpegData");

    if (!pixs)
        return (L_COMP_DATA *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetColormap(pixs))
        return (L_COMP_DATA *)ERROR_PTR("pixs has colormap", procName, NULL);
    d = pixGetDepth(pixs);
    if (d != 8 && d != 32)
        return (L_COMP_DATA *)ERROR_PTR("pixs not 8 or 32 bpp", procName, NULL);

        /* Compress to a temp jpeg file */
    fname = l_makeTempFilename();
    pixWriteJpeg(fname, pixs, quality, 0);

        /* Generate the data */
    cid = l_generateJpegData(fname, ascii85flag);
    lept_rmfile(fname);
    LEPT_FREE(fname);
    return cid;
}


/*!
 * \brief   pixGenerateJp2kData()
 *
 * \param[in]    pixs           8 or 32 bpp, no colormap
 * \param[in]    quality        0 for default, which is 34
 * \return  cid jp2k compressed data, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) The quality can be set between 27 (very poor) and 45
 *          (nearly perfect).  Use 0 for default (34). Use 100 for lossless,
 *          but this is very expensive and not recommended.
 * </pre>
 */
static L_COMP_DATA *
pixGenerateJp2kData(PIX     *pixs,
                    l_int32  quality)
{
l_int32       d;
char         *fname;
L_COMP_DATA  *cid;

    PROCNAME("pixGenerateJp2kData");

    if (!pixs)
        return (L_COMP_DATA *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetColormap(pixs))
        return (L_COMP_DATA *)ERROR_PTR("pixs has colormap", procName, NULL);
    d = pixGetDepth(pixs);
    if (d != 8 && d != 32)
        return (L_COMP_DATA *)ERROR_PTR("pixs not 8 or 32 bpp", procName, NULL);

        /* Compress to a temp jp2k file */
    fname = l_makeTempFilename();
    pixWriteJp2k(fname, pixs, quality, 5, 0, 0);

        /* Generate the data */
    cid = l_generateJp2kData(fname);
    if (lept_rmfile(fname) != 0)
        L_ERROR("temp file %s was not deleted\n", procName, fname);
    LEPT_FREE(fname);
    return cid;
}


/*!
 * \brief   pixGenerateG4Data()
 *
 * \param[in]    pixs           1 bpp
 * \param[in]    ascii85flag    0 for gzipped; 1 for ascii85-encoded gzipped
 * \return  cid g4 compressed image data, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) Set ascii85flag:
 *           ~ 0 for binary data (not permitted in PostScript)
 *           ~ 1 for ascii85 (5 for 4) encoded binary data
 * </pre>
 */
static L_COMP_DATA *
pixGenerateG4Data(PIX     *pixs,
                  l_int32  ascii85flag)
{
char         *tname;
L_COMP_DATA  *cid;

    PROCNAME("pixGenerateG4Data");

    if (!pixs)
        return (L_COMP_DATA *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 1)
        return (L_COMP_DATA *)ERROR_PTR("pixs not 1 bpp", procName, NULL);

        /* Compress to a temp tiff g4 file */
    tname = l_makeTempFilename();
    pixWrite(tname, pixs, IFF_TIFF_G4);

    cid = l_generateG4Data(tname, ascii85flag);
    lept_rmfile(tname);
    LEPT_FREE(tname);
    return cid;
}


/*!
 * \brief   l_generateG4Data()
 *
 * \param[in]    fname          of g4 compressed file
 * \param[in]    ascii85flag    0 for g4 compressed; 1 for ascii85-encoded g4
 * \return  cid g4 compressed image data, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) Set ascii85flag:
 *           ~ 0 for binary data (not permitted in PostScript)
 *           ~ 1 for ascii85 (5 for 4) encoded binary data
 *             (not permitted in pdf)
 * </pre>
 */
L_COMP_DATA *
l_generateG4Data(const char  *fname,
                 l_int32      ascii85flag)
{
l_uint8      *datacomp = NULL;  /* g4 compressed raster data */
char         *data85 = NULL;  /* ascii85 encoded g4 compressed data */
l_int32       w, h, xres, yres;
l_int32       minisblack;  /* TRUE or FALSE */
l_int32       nbytes85;
size_t        nbytescomp;
L_COMP_DATA  *cid;
FILE         *fp;

    PROCNAME("l_generateG4Data");

    if (!fname)
        return (L_COMP_DATA *)ERROR_PTR("fname not defined", procName, NULL);

        /* Read the resolution */
    if ((fp = fopenReadStream(fname)) == NULL)
        return (L_COMP_DATA *)ERROR_PTR("stream not opened", procName, NULL);
    getTiffResolution(fp, &xres, &yres);
    fclose(fp);

        /* The returned ccitt g4 data in memory is the block of
         * bytes in the tiff file, starting after 8 bytes and
         * ending before the directory. */
    if (extractG4DataFromFile(fname, &datacomp, &nbytescomp,
                              &w, &h, &minisblack)) {
        return (L_COMP_DATA *)ERROR_PTR("datacomp not extracted",
                                        procName, NULL);
    }

        /* Optionally, encode the compressed data */
    if (ascii85flag == 1) {
        data85 = encodeAscii85(datacomp, nbytescomp, &nbytes85);
        LEPT_FREE(datacomp);
        if (!data85)
            return (L_COMP_DATA *)ERROR_PTR("data85 not made", procName, NULL);
        else
            data85[nbytes85 - 1] = '\0';  /* remove the newline */
    }

    cid = (L_COMP_DATA *)LEPT_CALLOC(1, sizeof(L_COMP_DATA));
    if (ascii85flag == 0) {
        cid->datacomp = datacomp;
    } else {  /* ascii85 */
        cid->data85 = data85;
        cid->nbytes85 = nbytes85;
    }
    cid->type = L_G4_ENCODE;
    cid->nbytescomp = nbytescomp;
    cid->w = w;
    cid->h = h;
    cid->bps = 1;
    cid->spp = 1;
    cid->minisblack = minisblack;
    cid->res = xres;
    return cid;
}


/*!
 * \brief   cidConvertToPdfData()
 *
 * \param[in]    cid       compressed image data
 * \param[in]    title     [optional] pdf title; can be NULL
 * \param[out]   pdata     output pdf data for image
 * \param[out]   pnbytes   size of output pdf data
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) Caller must not destroy the cid.  It is absorbed in the
 *          lpd and destroyed by this function.
 * </pre>
 */
l_ok
cidConvertToPdfData(L_COMP_DATA  *cid,
                    const char   *title,
                    l_uint8     **pdata,
                    size_t       *pnbytes)
{
l_int32      res, ret;
l_float32    wpt, hpt;
L_PDF_DATA  *lpd = NULL;

    PROCNAME("cidConvertToPdfData");

    if (!pdata || !pnbytes)
        return ERROR_INT("&data and &nbytes not both defined", procName, 1);
    *pdata = NULL;
    *pnbytes = 0;
    if (!cid)
        return ERROR_INT("cid not defined", procName, 1);

        /* Get media box parameters, in pts */
    res = cid->res;
    if (res <= 0)
        res = DEFAULT_INPUT_RES;
    wpt = cid->w * 72. / res;
    hpt = cid->h * 72. / res;

        /* Set up the pdf data struct (lpd) */
    if ((lpd = pdfdataCreate(title)) == NULL)
        return ERROR_INT("lpd not made", procName, 1);
    ptraAdd(lpd->cida, cid);
    lpd->n++;
    ptaAddPt(lpd->xy, 0, 0);   /* xpt = ypt = 0 */
    ptaAddPt(lpd->wh, wpt, hpt);

        /* Generate the pdf string and destroy the lpd */
    ret = l_generatePdf(pdata, pnbytes, lpd);
    pdfdataDestroy(&lpd);
    if (ret)
        return ERROR_INT("pdf output not made", procName, 1);
    return 0;
}


/*!
 * \brief   l_CIDataDestroy()
 *
 * \param[in,out]   pcid     will be set to null before returning
 * \return  void
 */
void
l_CIDataDestroy(L_COMP_DATA  **pcid)
{
L_COMP_DATA  *cid;

    PROCNAME("l_CIDataDestroy");

    if (pcid == NULL) {
        L_WARNING("ptr address is null!\n", procName);
        return;
    }
    if ((cid = *pcid) == NULL)
        return;

    if (cid->datacomp) LEPT_FREE(cid->datacomp);
    if (cid->data85) LEPT_FREE(cid->data85);
    if (cid->cmapdata85) LEPT_FREE(cid->cmapdata85);
    if (cid->cmapdatahex) LEPT_FREE(cid->cmapdatahex);
    LEPT_FREE(cid);
    *pcid = NULL;
    return;
}


/*---------------------------------------------------------------------*
 *         Helper functions for generating the output pdf string       *
 *---------------------------------------------------------------------*/
/*!
 * \brief   l_generatePdf()
 *
 * \param[out]   pdata     pdf array
 * \param[out]   pnbytes   number of bytes in pdf array
 * \param[in]    lpd       all the required input image data
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) On error, no data is returned.
 *      (2) The objects are:
 *            1: Catalog
 *            2: Info
 *            3: Pages
 *            4: Page
 *            5: Contents  (rendering command)
 *            6 to 6+n-1: n XObjects
 *            6+n to 6+n+m-1: m colormaps
 * </pre>
 */
static l_int32
l_generatePdf(l_uint8    **pdata,
              size_t      *pnbytes,
              L_PDF_DATA  *lpd)
{
    PROCNAME("l_generatePdf");

    if (!pdata)
        return ERROR_INT("&data not defined", procName, 1);
    *pdata = NULL;
    if (!pnbytes)
        return ERROR_INT("&nbytes not defined", procName, 1);
    *pnbytes = 0;
    if (!lpd)
        return ERROR_INT("lpd not defined", procName, 1);

    generateFixedStringsPdf(lpd);
    generateMediaboxPdf(lpd);
    generatePageStringPdf(lpd);
    generateContentStringPdf(lpd);
    generatePreXStringsPdf(lpd);
    generateColormapStringsPdf(lpd);
    generateTrailerPdf(lpd);
    return generateOutputDataPdf(pdata, pnbytes, lpd);
}


static void
generateFixedStringsPdf(L_PDF_DATA  *lpd)
{
char     buf[L_SMALLBUF];
char    *version, *datestr;
SARRAY  *sa;

    PROCNAME("generateFixedStringsPdf");

        /* Accumulate data for the header and objects 1-3 */
    lpd->id = stringNew("%PDF-1.5\n");
    l_dnaAddNumber(lpd->objsize, strlen(lpd->id));

    lpd->obj1 = stringNew("1 0 obj\n"
                          "<<\n"
                          "/Type /Catalog\n"
                          "/Pages 3 0 R\n"
                          ">>\n"
                          "endobj\n");
    l_dnaAddNumber(lpd->objsize, strlen(lpd->obj1));

    sa = sarrayCreate(0);
    sarrayAddString(sa, "2 0 obj\n"
                        "<<\n", L_COPY);
    if (var_WRITE_DATE_AND_VERSION) {
        datestr = l_getFormattedDate();
        snprintf(buf, sizeof(buf), "/CreationDate (D:%s)\n", datestr);
        sarrayAddString(sa, buf, L_COPY);
        LEPT_FREE(datestr);
        version = getLeptonicaVersion();
        snprintf(buf, sizeof(buf),
                 "/Producer (leptonica: %s)\n", version);
        LEPT_FREE(version);
    } else {
        snprintf(buf, sizeof(buf), "/Producer (leptonica)\n");
    }
    sarrayAddString(sa, buf, L_COPY);
    if (lpd->title) {
        char *hexstr;
        if ((hexstr = generateEscapeString(lpd->title)) != NULL) {
            snprintf(buf, sizeof(buf), "/Title %s\n", hexstr);
            sarrayAddString(sa, buf, L_COPY);
        } else {
            L_ERROR("title string is not ascii\n", procName);
        }
        LEPT_FREE(hexstr);
    }
    sarrayAddString(sa, ">>\n"
                                "endobj\n", L_COPY);
    lpd->obj2 = sarrayToString(sa, 0);
    l_dnaAddNumber(lpd->objsize, strlen(lpd->obj2));
    sarrayDestroy(&sa);

    lpd->obj3 = stringNew("3 0 obj\n"
                          "<<\n"
                          "/Type /Pages\n"
                          "/Kids [ 4 0 R ]\n"
                          "/Count 1\n"
                          ">>\n");
    l_dnaAddNumber(lpd->objsize, strlen(lpd->obj3));

        /* Do the post-datastream string */
    lpd->poststream = stringNew("\n"
                                "endstream\n"
                                "endobj\n");
    return;
}


/*!
 * \brief   generateEscapeString()
 *
 * \param[in]   str      input string
 * \return   hex escape string, or null on error
 *
 * <pre>
 * Notes:
 *      (1) If the input string is not ascii, returns null.
 *      (2) This takes an input ascii string and generates a hex
 *          ascii output string with 4 bytes out for each byte in.
 *          The feff code at the beginning tells the pdf interpreter
 *          that the data is to be interpreted as big-endian, 4 bytes
 *          at a time.  For ascii, the first two bytes are 0 and the
 *          last two bytes are less than 0x80.
 * </pre>
 */
static char  *
generateEscapeString(const char  *str)
{
char     smallbuf[8];
char    *buffer;
l_int32  i, nchar, buflen;

    PROCNAME("generateEscapeString");

    if (!str)
        return (char *)ERROR_PTR("str not defined", procName, NULL);
    nchar = strlen(str);
    for (i = 0; i < nchar; i++) {
        if (str[i] < 0)
            return (char *)ERROR_PTR("str not all ascii", procName, NULL);
    }

    buflen = 4 * nchar + 10;
    buffer = (char *)LEPT_CALLOC(buflen, sizeof(char));
    stringCat(buffer, buflen, "<feff");
    for (i = 0; i < nchar; i++) {
        snprintf(smallbuf, sizeof(smallbuf), "%04x", str[i]);
        stringCat(buffer, buflen, smallbuf);
    }
    stringCat(buffer, buflen, ">");
    return buffer;
}


static void
generateMediaboxPdf(L_PDF_DATA  *lpd)
{
l_int32    i;
l_float32  xpt, ypt, wpt, hpt, maxx, maxy;

        /* First get the full extent of all the images.
         * This is the mediabox, in pts. */
    maxx = maxy = 0;
    for (i = 0; i < lpd->n; i++) {
        ptaGetPt(lpd->xy, i, &xpt, &ypt);
        ptaGetPt(lpd->wh, i, &wpt, &hpt);
        maxx = L_MAX(maxx, xpt + wpt);
        maxy = L_MAX(maxy, ypt + hpt);
    }

    lpd->mediabox = boxCreate(0, 0, (l_int32)(maxx + 0.5),
                              (l_int32)(maxy + 0.5));

        /* ypt is in standard image coordinates: the location of
         * the UL image corner with respect to the UL media box corner.
         * Rewrite each ypt for PostScript coordinates: the location of
         * the LL image corner with respect to the LL media box corner. */
    for (i = 0; i < lpd->n; i++) {
        ptaGetPt(lpd->xy, i, &xpt, &ypt);
        ptaGetPt(lpd->wh, i, &wpt, &hpt);
        ptaSetPt(lpd->xy, i, xpt, maxy - ypt - hpt);
    }

    return;
}


static l_int32
generatePageStringPdf(L_PDF_DATA  *lpd)
{
char    *buf;
char    *xstr;
l_int32  bufsize, i, wpt, hpt;
SARRAY  *sa;

    PROCNAME("generatePageStringPdf");

        /* Allocate 1000 bytes for the boilerplate text, and
         * 50 bytes for each reference to an image in the
         * ProcSet array.  */
    bufsize = 1000 + 50 * lpd->n;
    if ((buf = (char *)LEPT_CALLOC(bufsize, sizeof(char))) == NULL)
        return ERROR_INT("calloc fail for buf", procName, 1);

    boxGetGeometry(lpd->mediabox, NULL, NULL, &wpt, &hpt);
    sa = sarrayCreate(lpd->n);
    for (i = 0; i < lpd->n; i++) {
        snprintf(buf, bufsize, "/Im%d %d 0 R   ", i + 1, 6 + i);
        sarrayAddString(sa, buf, L_COPY);
    }
    xstr = sarrayToString(sa, 0);
    sarrayDestroy(&sa);
    if (!xstr) {
        LEPT_FREE(buf);
        return ERROR_INT("xstr not made", procName, 1);
    }

    snprintf(buf, bufsize, "4 0 obj\n"
                           "<<\n"
                           "/Type /Page\n"
                           "/Parent 3 0 R\n"
                           "/MediaBox [%d %d %d %d]\n"
                           "/Contents 5 0 R\n"
                           "/Resources\n"
                           "<<\n"
                           "/XObject << %s >>\n"
                           "/ProcSet [ /ImageB /ImageI /ImageC ]\n"
                           ">>\n"
                           ">>\n"
                           "endobj\n",
                           0, 0, wpt, hpt, xstr);

    lpd->obj4 = stringNew(buf);
    l_dnaAddNumber(lpd->objsize, strlen(lpd->obj4));
    sarrayDestroy(&sa);
    LEPT_FREE(buf);
    LEPT_FREE(xstr);
    return 0;
}


static l_int32
generateContentStringPdf(L_PDF_DATA  *lpd)
{
char      *buf;
char      *cstr;
l_int32    i, bufsize;
l_float32  xpt, ypt, wpt, hpt;
SARRAY    *sa;

    PROCNAME("generateContentStringPdf");

    bufsize = 1000 + 200 * lpd->n;
    if ((buf = (char *)LEPT_CALLOC(bufsize, sizeof(char))) == NULL)
        return ERROR_INT("calloc fail for buf", procName, 1);

    sa = sarrayCreate(lpd->n);
    for (i = 0; i < lpd->n; i++) {
        ptaGetPt(lpd->xy, i, &xpt, &ypt);
        ptaGetPt(lpd->wh, i, &wpt, &hpt);
        snprintf(buf, bufsize,
                 "q %.4f %.4f %.4f %.4f %.4f %.4f cm /Im%d Do Q\n",
                 wpt, 0.0, 0.0, hpt, xpt, ypt, i + 1);
        sarrayAddString(sa, buf, L_COPY);
    }
    cstr = sarrayToString(sa, 0);
    sarrayDestroy(&sa);
    if (!cstr) {
        LEPT_FREE(buf);
        return ERROR_INT("cstr not made", procName, 1);
    }

    snprintf(buf, bufsize, "5 0 obj\n"
                           "<< /Length %d >>\n"
                           "stream\n"
                           "%s"
                           "endstream\n"
                           "endobj\n",
                           (l_int32)strlen(cstr), cstr);

    lpd->obj5 = stringNew(buf);
    l_dnaAddNumber(lpd->objsize, strlen(lpd->obj5));
    sarrayDestroy(&sa);
    LEPT_FREE(buf);
    LEPT_FREE(cstr);
    return 0;
}


static l_int32
generatePreXStringsPdf(L_PDF_DATA  *lpd)
{
char          buff[256];
char          buf[L_BIGBUF];
char         *cstr, *bstr, *fstr, *pstr, *xstr;
l_int32       i, cmindex;
L_COMP_DATA  *cid;
SARRAY       *sa;

    PROCNAME("generatePreXStringsPdf");

    sa = lpd->saprex;
    cmindex = 6 + lpd->n;  /* starting value */
    for (i = 0; i < lpd->n; i++) {
        pstr = cstr = NULL;
        if ((cid = pdfdataGetCid(lpd, i)) == NULL)
            return ERROR_INT("cid not found", procName, 1);

        if (cid->type == L_G4_ENCODE) {
            if (var_WRITE_G4_IMAGE_MASK) {
                cstr = stringNew("/ImageMask true\n"
                                 "/ColorSpace /DeviceGray");
            } else {
                cstr = stringNew("/ColorSpace /DeviceGray");
            }
            bstr = stringNew("/BitsPerComponent 1\n"
                             "/Interpolate true");
            snprintf(buff, sizeof(buff),
                     "/Filter /CCITTFaxDecode\n"
                     "/DecodeParms\n"
                     "<<\n"
                     "/K -1\n"
                     "/Columns %d\n"
                     ">>", cid->w);
            fstr = stringNew(buff);
        } else if (cid->type == L_JPEG_ENCODE) {
            if (cid->spp == 1)
                cstr = stringNew("/ColorSpace /DeviceGray");
            else if (cid->spp == 3)
                cstr = stringNew("/ColorSpace /DeviceRGB");
            else if (cid->spp == 4)   /* pdf supports cmyk */
                cstr = stringNew("/ColorSpace /DeviceCMYK");
            else
                L_ERROR("in jpeg: spp != 1, 3 or 4\n", procName);
            bstr = stringNew("/BitsPerComponent 8");
            fstr = stringNew("/Filter /DCTDecode");
        } else if (cid->type == L_JP2K_ENCODE) {
            if (cid->spp == 1)
                cstr = stringNew("/ColorSpace /DeviceGray");
            else if (cid->spp == 3)
                cstr = stringNew("/ColorSpace /DeviceRGB");
            else
                L_ERROR("in jp2k: spp != 1 && spp != 3\n", procName);
            bstr = stringNew("/BitsPerComponent 8");
            fstr = stringNew("/Filter /JPXDecode");
        } else {  /* type == L_FLATE_ENCODE */
            if (cid->ncolors > 0) {  /* cmapped */
                snprintf(buff, sizeof(buff), "/ColorSpace %d 0 R", cmindex++);
                cstr = stringNew(buff);
            } else {
                if (cid->spp == 1 && cid->bps == 1)
                    cstr = stringNew("/ColorSpace /DeviceGray\n"
                                     "/Decode [1 0]");
                else if (cid->spp == 1)  /* 8 bpp */
                    cstr = stringNew("/ColorSpace /DeviceGray");
                else if (cid->spp == 3)
                    cstr = stringNew("/ColorSpace /DeviceRGB");
                else
                    L_ERROR("unknown colorspace: spp = %d\n",
                            procName, cid->spp);
            }
            snprintf(buff, sizeof(buff), "/BitsPerComponent %d", cid->bps);
            bstr = stringNew(buff);
            fstr = stringNew("/Filter /FlateDecode");
            if (cid->predictor == TRUE) {
                snprintf(buff, sizeof(buff),
                         "/DecodeParms\n"
                         "<<\n"
                         "  /Columns %d\n"
                         "  /Predictor 14\n"
                         "  /Colors %d\n"
                         "  /BitsPerComponent %d\n"
                         ">>\n", cid->w, cid->spp, cid->bps);
                pstr = stringNew(buff);
            }
        }
        if (!pstr)  /* no decode parameters */
            pstr = stringNew("");

        snprintf(buf, sizeof(buf),
                 "%d 0 obj\n"
                 "<<\n"
                 "/Length %lu\n"
                 "/Subtype /Image\n"
                 "%s\n"  /* colorspace */
                 "/Width %d\n"
                 "/Height %d\n"
                 "%s\n"  /* bits/component */
                 "%s\n"  /* filter */
                 "%s"   /* decode parms; can be empty */
                 ">>\n"
                 "stream\n",
                 6 + i, (unsigned long)cid->nbytescomp, cstr,
                 cid->w, cid->h, bstr, fstr, pstr);
        xstr = stringNew(buf);
        sarrayAddString(sa, xstr, L_INSERT);
        l_dnaAddNumber(lpd->objsize,
                      strlen(xstr) + cid->nbytescomp + strlen(lpd->poststream));
        LEPT_FREE(cstr);
        LEPT_FREE(bstr);
        LEPT_FREE(fstr);
        LEPT_FREE(pstr);
    }

    return 0;
}


static l_int32
generateColormapStringsPdf(L_PDF_DATA  *lpd)
{
char          buf[L_BIGBUF];
char         *cmstr;
l_int32       i, cmindex, ncmap;
L_COMP_DATA  *cid;
SARRAY       *sa;

    PROCNAME("generateColormapStringsPdf");

        /* In our canonical format, we have 5 objects, followed
         * by n XObjects, followed by m colormaps, so the index of
         * the first colormap object is 6 + n. */
    sa = lpd->sacmap;
    cmindex = 6 + lpd->n;  /* starting value */
    ncmap = 0;
    for (i = 0; i < lpd->n; i++) {
        if ((cid = pdfdataGetCid(lpd, i)) == NULL)
            return ERROR_INT("cid not found", procName, 1);
        if (cid->ncolors == 0) continue;

        ncmap++;
        snprintf(buf, sizeof(buf), "%d 0 obj\n"
                                   "[ /Indexed /DeviceRGB\n"
                                   "%d\n"
                                   "%s\n"
                                   "]\n"
                                   "endobj\n",
                                   cmindex, cid->ncolors - 1, cid->cmapdatahex);
        cmindex++;
        cmstr = stringNew(buf);
        l_dnaAddNumber(lpd->objsize, strlen(cmstr));
        sarrayAddString(sa, cmstr, L_INSERT);
    }

    lpd->ncmap = ncmap;
    return 0;
}


static void
generateTrailerPdf(L_PDF_DATA  *lpd)
{
l_int32  i, n, size, linestart;
L_DNA   *daloc, *dasize;

        /* Let nobj be the number of numbered objects.  These numbered
         * objects are indexed by their pdf number in arrays naloc[]
         * and nasize[].  The 0th object is the 9 byte header.  Then
         * the number of objects in nasize, which includes the header,
         * is n = nobj + 1.  The array naloc[] has n + 1 elements,
         * because it includes as the last element the starting
         * location of xref.  The indexing of these objects, their
         * starting locations and sizes are:
         *
         *     Object number         Starting location         Size
         *     -------------         -----------------     --------------
         *          0                   daloc[0] = 0       dasize[0] = 9
         *          1                   daloc[1] = 9       dasize[1] = 49
         *          n                   daloc[n]           dasize[n]
         *          xref                daloc[n+1]
         *
         * We first generate daloc.
         */
    dasize = lpd->objsize;
    daloc = lpd->objloc;
    linestart = 0;
    l_dnaAddNumber(daloc, linestart);  /* header */
    n = l_dnaGetCount(dasize);
    for (i = 0; i < n; i++) {
        l_dnaGetIValue(dasize, i, &size);
        linestart += size;
        l_dnaAddNumber(daloc, linestart);
    }
    l_dnaGetIValue(daloc, n, &lpd->xrefloc);  /* save it */

        /* Now make the actual trailer string */
    lpd->trailer = makeTrailerStringPdf(daloc);
}


static char *
makeTrailerStringPdf(L_DNA  *daloc)
{
char    *outstr;
char     buf[L_BIGBUF];
l_int32  i, n, linestart, xrefloc;
SARRAY  *sa;

    PROCNAME("makeTrailerStringPdf");

    if (!daloc)
        return (char *)ERROR_PTR("daloc not defined", procName, NULL);
    n = l_dnaGetCount(daloc) - 1;  /* numbered objects + 1 (yes, +1) */

    sa = sarrayCreate(0);
    snprintf(buf, sizeof(buf), "xref\n"
                               "0 %d\n"
                               "0000000000 65535 f \n", n);
    sarrayAddString(sa, buf, L_COPY);
    for (i = 1; i < n; i++) {
        l_dnaGetIValue(daloc, i, &linestart);
        snprintf(buf, sizeof(buf), "%010d 00000 n \n", linestart);
        sarrayAddString(sa, buf, L_COPY);
    }

    l_dnaGetIValue(daloc, n, &xrefloc);
    snprintf(buf, sizeof(buf), "trailer\n"
                               "<<\n"
                               "/Size %d\n"
                               "/Root 1 0 R\n"
                               "/Info 2 0 R\n"
                               ">>\n"
                               "startxref\n"
                               "%d\n"
                               "%%%%EOF\n", n, xrefloc);
    sarrayAddString(sa, buf, L_COPY);
    outstr = sarrayToString(sa, 0);
    sarrayDestroy(&sa);
    return outstr;
}


/*!
 * \brief   generateOutputDataPdf()
 *
 * \param[out]   pdata      pdf data array
 * \param[out]   pnbytes    size of pdf data array
 * \param[in]    lpd        input data used to make pdf
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) Only called from l_generatePdf().  On error, no data is returned.
 * </pre>
 */
static l_int32
generateOutputDataPdf(l_uint8    **pdata,
                      size_t      *pnbytes,
                      L_PDF_DATA  *lpd)
{
char         *str;
l_uint8      *data;
l_int32       nimages, i, len;
l_int32      *sizes, *locs;
size_t        nbytes;
L_COMP_DATA  *cid;

    PROCNAME("generateOutputDataPdf");

    if (!pdata)
        return ERROR_INT("&data not defined", procName, 1);
    *pdata = NULL;
    if (!pnbytes)
        return ERROR_INT("&nbytes not defined", procName, 1);
    nbytes = lpd->xrefloc + strlen(lpd->trailer);
    *pnbytes = nbytes;
    if ((data = (l_uint8 *)LEPT_CALLOC(nbytes, sizeof(l_uint8))) == NULL)
        return ERROR_INT("calloc fail for data", procName, 1);
    *pdata = data;

    sizes = l_dnaGetIArray(lpd->objsize);
    locs = l_dnaGetIArray(lpd->objloc);
    memcpy(data, lpd->id, sizes[0]);
    memcpy(data + locs[1], lpd->obj1, sizes[1]);
    memcpy(data + locs[2], lpd->obj2, sizes[2]);
    memcpy(data + locs[3], lpd->obj3, sizes[3]);
    memcpy(data + locs[4], lpd->obj4, sizes[4]);
    memcpy(data + locs[5], lpd->obj5, sizes[5]);

        /* Each image has 3 parts: variable preamble, the compressed
         * data stream, and the fixed poststream. */
    nimages = lpd->n;
    for (i = 0; i < nimages; i++) {
        if ((cid = pdfdataGetCid(lpd, i)) == NULL) {  /* should not happen */
            LEPT_FREE(sizes);
            LEPT_FREE(locs);
            return ERROR_INT("cid not found", procName, 1);
        }
        str = sarrayGetString(lpd->saprex, i, L_NOCOPY);
        len = strlen(str);
        memcpy(data + locs[6 + i], str, len);
        memcpy(data + locs[6 + i] + len,
               cid->datacomp, cid->nbytescomp);
        memcpy(data + locs[6 + i] + len + cid->nbytescomp,
               lpd->poststream, strlen(lpd->poststream));
    }

        /* Each colormap is simply a stored string */
    for (i = 0; i < lpd->ncmap; i++) {
        str = sarrayGetString(lpd->sacmap, i, L_NOCOPY);
        memcpy(data + locs[6 + nimages + i], str, strlen(str));
    }

        /* And finally the trailer */
    memcpy(data + lpd->xrefloc, lpd->trailer, strlen(lpd->trailer));
    LEPT_FREE(sizes);
    LEPT_FREE(locs);
    return 0;
}


/*---------------------------------------------------------------------*
 *          Helper functions for generating multipage pdf output       *
 *---------------------------------------------------------------------*/
/*!
 * \brief   parseTrailerPdf()
 *
 * \param[in]    bas     lba of a pdf file
 * \param[out]   pda     byte locations of the beginning of each object
 * \return  0 if OK, 1 on error
 */
static l_int32
parseTrailerPdf(L_BYTEA  *bas,
                L_DNA   **pda)
{
char     *str;
l_uint8   nl = '\n';
l_uint8  *data;
l_int32   i, j, start, startloc, xrefloc, found, loc, nobj, objno, trailer_ok;
size_t    size;
L_DNA    *da, *daobj, *daxref;
SARRAY   *sa;

    PROCNAME("parseTrailerPdf");

    if (!pda)
        return ERROR_INT("&da not defined", procName, 1);
    *pda = NULL;
    if (!bas)
        return ERROR_INT("bas not defined", procName, 1);
    data = l_byteaGetData(bas, &size);
    if (memcmp(data, "%PDF-1.", 7) != 0)
        return ERROR_INT("PDF header signature not found", procName, 1);

        /* Search for "startxref" starting 50 bytes from the EOF */
    start = 0;
    if (size > 50)
        start = size - 50;
    arrayFindSequence(data + start, size - start,
                      (l_uint8 *)"startxref\n", 10, &loc, &found);
    if (!found)
        return ERROR_INT("startxref not found!", procName, 1);
    if (sscanf((char *)(data + start + loc + 10), "%d\n", &xrefloc) != 1)
        return ERROR_INT("xrefloc not found!", procName, 1);
    if (xrefloc < 0 || xrefloc >= size)
        return ERROR_INT("invalid xrefloc!", procName, 1);
    sa = sarrayCreateLinesFromString((char *)(data + xrefloc), 0);
    str = sarrayGetString(sa, 1, L_NOCOPY);
    if ((sscanf(str, "0 %d", &nobj)) != 1) {
        sarrayDestroy(&sa);
        return ERROR_INT("nobj not found", procName, 1);
    }

        /* Get starting locations.  The numa index is the
         * object number.  loc[0] is the ID; loc[nobj + 1] is xrefloc.  */
    da = l_dnaCreate(nobj + 1);
    *pda = da;
    for (i = 0; i < nobj; i++) {
        str = sarrayGetString(sa, i + 2, L_NOCOPY);
        sscanf(str, "%d", &startloc);
        l_dnaAddNumber(da, startloc);
    }
    l_dnaAddNumber(da, xrefloc);

#if  DEBUG_MULTIPAGE
    fprintf(stderr, "************** Trailer string ************\n");
    fprintf(stderr, "xrefloc = %d", xrefloc);
    sarrayWriteStream(stderr, sa);

    fprintf(stderr, "************** Object locations ************");
    l_dnaWriteStream(stderr, da);
#endif  /* DEBUG_MULTIPAGE */
    sarrayDestroy(&sa);

        /* Verify correct parsing */
    trailer_ok = TRUE;
    for (i = 1; i < nobj; i++) {
        l_dnaGetIValue(da, i, &startloc);
        if ((sscanf((char *)(data + startloc), "%d 0 obj", &objno)) != 1) {
            L_ERROR("bad trailer for object %d\n", procName, i);
            trailer_ok = FALSE;
            break;
        }
    }

        /* If the trailer is broken, reconstruct the correct obj locations */
    if (!trailer_ok) {
        L_INFO("rebuilding pdf trailer\n", procName);
        l_dnaEmpty(da);
        l_dnaAddNumber(da, 0);
        l_byteaFindEachSequence(bas, (l_uint8 *)" 0 obj\n", 7, &daobj);
        nobj = l_dnaGetCount(daobj);
        for (i = 0; i < nobj; i++) {
            l_dnaGetIValue(daobj, i, &loc);
            for (j = loc - 1; j > 0; j--) {
                if (data[j] == nl)
                    break;
            }
            l_dnaAddNumber(da, j + 1);
        }
        l_byteaFindEachSequence(bas, (l_uint8 *)"xref", 4, &daxref);
        l_dnaGetIValue(daxref, 0, &loc);
        l_dnaAddNumber(da, loc);
        l_dnaDestroy(&daobj);
        l_dnaDestroy(&daxref);
    }

    return 0;
}


static char *
generatePagesObjStringPdf(NUMA  *napage)
{
char    *str;
char    *buf;
l_int32  i, n, index, bufsize;
SARRAY  *sa;

    PROCNAME("generatePagesObjStringPdf");

    if (!napage)
        return (char *)ERROR_PTR("napage not defined", procName, NULL);

    n = numaGetCount(napage);
    bufsize = 100 + 16 * n;  /* large enough to hold the output string */
    buf = (char *)LEPT_CALLOC(bufsize, sizeof(char));
    sa = sarrayCreate(n);
    for (i = 0; i < n; i++) {
        numaGetIValue(napage, i, &index);
        snprintf(buf, bufsize, " %d 0 R ", index);
        sarrayAddString(sa, buf, L_COPY);
    }

    str = sarrayToString(sa, 0);
    snprintf(buf, bufsize - 1, "3 0 obj\n"
                               "<<\n"
                               "/Type /Pages\n"
                               "/Kids [%s]\n"
                               "/Count %d\n"
                               ">>\n", str, n);
    sarrayDestroy(&sa);
    LEPT_FREE(str);
    return buf;
}


/*!
 * \brief   substituteObjectNumbers()
 *
 * \param[in]   bas        lba of a pdf object
 * \param[in]   na_objs    object number mapping array
 * \return    bad   lba of rewritten pdf for the object
 *
 * <pre>
 * Notes:
 *      (1) Interpret the first set of bytes as the object number,
 *          map to the new number, and write it out.
 *      (2) Find all occurrences of this 4-byte sequence: " 0 R"
 *      (3) Find the location and value of the integer preceding this,
 *          and map it to the new value.
 *      (4) Rewrite the object with new object numbers.
 * </pre>
 */
static L_BYTEA *
substituteObjectNumbers(L_BYTEA  *bas,
                        NUMA     *na_objs)
{
l_uint8   space = ' ';
l_uint8  *datas;
l_uint8   buf[32];  /* only needs to hold one integer in ascii format */
l_int32   start, nrepl, i, j, objin, objout, found;
l_int32  *objs, *matches;
size_t    size;
L_BYTEA  *bad;
L_DNA    *da_match;

    datas = l_byteaGetData(bas, &size);
    bad = l_byteaCreate(100);
    objs = numaGetIArray(na_objs);  /* object number mapper */

        /* Substitute the object number on the first line */
    sscanf((char *)datas, "%d", &objin);
    objout = objs[objin];
    snprintf((char *)buf, 32, "%d", objout);
    l_byteaAppendString(bad, (char *)buf);

        /* Find the set of matching locations for object references */
    arrayFindSequence(datas, size, &space, 1, &start, &found);
    da_match = arrayFindEachSequence(datas, size, (l_uint8 *)" 0 R", 4);
    if (!da_match) {
        l_byteaAppendData(bad, datas + start, size - start);
        LEPT_FREE(objs);
        return bad;
    }

        /* Substitute all the object reference numbers */
    nrepl = l_dnaGetCount(da_match);
    matches = l_dnaGetIArray(da_match);
    for (i = 0; i < nrepl; i++) {
            /* Find the first space before the object number */
        for (j = matches[i] - 1; j > 0; j--) {
            if (datas[j] == space)
                break;
        }
            /* Copy bytes from 'start' up to the object number */
        l_byteaAppendData(bad, datas + start, j - start + 1);
        sscanf((char *)(datas + j + 1), "%d", &objin);
        objout = objs[objin];
        snprintf((char *)buf, 32, "%d", objout);
        l_byteaAppendString(bad, (char *)buf);
        start = matches[i];
    }
    l_byteaAppendData(bad, datas + start, size - start);

    LEPT_FREE(objs);
    LEPT_FREE(matches);
    l_dnaDestroy(&da_match);
    return bad;
}


/*---------------------------------------------------------------------*
 *                     Create/destroy/access pdf data                  *
 *---------------------------------------------------------------------*/
static L_PDF_DATA *
pdfdataCreate(const char  *title)
{
L_PDF_DATA *lpd;

    lpd = (L_PDF_DATA *)LEPT_CALLOC(1, sizeof(L_PDF_DATA));
    if (title) lpd->title = stringNew(title);
    lpd->cida = ptraCreate(10);
    lpd->xy = ptaCreate(10);
    lpd->wh = ptaCreate(10);
    lpd->saprex = sarrayCreate(10);
    lpd->sacmap = sarrayCreate(10);
    lpd->objsize = l_dnaCreate(20);
    lpd->objloc = l_dnaCreate(20);
    return lpd;
}

static void
pdfdataDestroy(L_PDF_DATA  **plpd)
{
l_int32       i;
L_COMP_DATA  *cid;
L_PDF_DATA   *lpd;

    PROCNAME("pdfdataDestroy");

    if (plpd== NULL) {
        L_WARNING("ptr address is null!\n", procName);
        return;
    }
    if ((lpd = *plpd) == NULL)
        return;

    if (lpd->title) LEPT_FREE(lpd->title);
    for (i = 0; i < lpd->n; i++) {
        cid = (L_COMP_DATA *)ptraRemove(lpd->cida, i, L_NO_COMPACTION);
        l_CIDataDestroy(&cid);
    }

    ptraDestroy(&lpd->cida, 0, 0);
    if (lpd->id) LEPT_FREE(lpd->id);
    if (lpd->obj1) LEPT_FREE(lpd->obj1);
    if (lpd->obj2) LEPT_FREE(lpd->obj2);
    if (lpd->obj3) LEPT_FREE(lpd->obj3);
    if (lpd->obj4) LEPT_FREE(lpd->obj4);
    if (lpd->obj5) LEPT_FREE(lpd->obj5);
    if (lpd->poststream) LEPT_FREE(lpd->poststream);
    if (lpd->trailer) LEPT_FREE(lpd->trailer);
    if (lpd->xy) ptaDestroy(&lpd->xy);
    if (lpd->wh) ptaDestroy(&lpd->wh);
    if (lpd->mediabox) boxDestroy(&lpd->mediabox);
    if (lpd->saprex) sarrayDestroy(&lpd->saprex);
    if (lpd->sacmap) sarrayDestroy(&lpd->sacmap);
    if (lpd->objsize) l_dnaDestroy(&lpd->objsize);
    if (lpd->objloc) l_dnaDestroy(&lpd->objloc);
    LEPT_FREE(lpd);
    *plpd = NULL;
    return;
}


static L_COMP_DATA *
pdfdataGetCid(L_PDF_DATA  *lpd,
              l_int32      index)
{
    PROCNAME("pdfdataGetCid");

    if (!lpd)
        return (L_COMP_DATA *)ERROR_PTR("lpd not defined", procName, NULL);
    if (index < 0 || index >= lpd->n)
        return (L_COMP_DATA *)ERROR_PTR("invalid image index", procName, NULL);

    return (L_COMP_DATA *)ptraGetPtrToItem(lpd->cida, index);
}


/*---------------------------------------------------------------------*
 *                       Set flags for special modes                   *
 *---------------------------------------------------------------------*/
/*!
 * \brief   l_pdfSetG4ImageMask()
 *
 * \param[in]    flag    1 for writing g4 data as fg only through a mask;
 *                       0 for writing fg and bg
 * \return  void
 *
 * <pre>
 * Notes:
 *      (1) The default is for writing only the fg (through the mask).
 *          That way when you write a 1 bpp image, the bg is transparent,
 *          so any previously written image remains visible behind it.
 * </pre>
 */
void
l_pdfSetG4ImageMask(l_int32  flag)
{
    var_WRITE_G4_IMAGE_MASK = flag;
}


/*!
 * \brief   l_pdfSetDateAndVersion()
 *
 * \param[in]    flag    1 for writing date/time and leptonica version;
 *                       0 for omitting this from the metadata
 * \return  void
 *
 * <pre>
 * Notes:
 *      (1) The default is for writing this data.  For regression tests
 *          that compare output against golden files, it is useful to omit.
 * </pre>
 */
void
l_pdfSetDateAndVersion(l_int32  flag)
{
    var_WRITE_DATE_AND_VERSION = flag;
}

/* --------------------------------------------*/
#endif  /* USE_PDFIO */
/* --------------------------------------------*/
