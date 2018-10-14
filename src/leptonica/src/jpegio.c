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
 * \file jpegio.c
 * <pre>
 *
 *    Read jpeg from file
 *          PIX             *pixReadJpeg()  [special top level]
 *          PIX             *pixReadStreamJpeg()
 *
 *    Read jpeg metadata from file
 *          l_int32          readHeaderJpeg()
 *          l_int32          freadHeaderJpeg()
 *          l_int32          fgetJpegResolution()
 *          l_int32          fgetJpegComment()
 *
 *    Write jpeg to file
 *          l_int32          pixWriteJpeg()  [special top level]
 *          l_int32          pixWriteStreamJpeg()
 *
 *    Read/write to memory
 *          PIX             *pixReadMemJpeg()
 *          l_int32          readHeaderMemJpeg()
 *          l_int32          readResolutionMemJpeg()
 *          l_int32          pixWriteMemJpeg()
 *
 *    Setting special flag for chroma sampling on write
 *          l_int32          pixSetChromaSampling()
 *
 *    Static system helpers
 *          static void      jpeg_error_catch_all_1()
 *          static void      jpeg_error_catch_all_2()
 *          static l_uint8   jpeg_getc()
 *          static l_int32   jpeg_comment_callback()
 *
 *    Documentation: libjpeg.doc can be found, along with all
 *    source code, at ftp://ftp.uu.net/graphics/jpeg
 *    Download and untar the file:  jpegsrc.v6b.tar.gz
 *    A good paper on jpeg can also be found there: wallace.ps.gz
 *
 *    The functions in libjpeg make it very simple to compress
 *    and decompress images.  On input (decompression from file),
 *    3 component color images can be read into either an 8 bpp Pix
 *    with a colormap or a 32 bpp Pix with RGB components.  For output
 *    (compression to file), all color Pix, whether 8 bpp with a
 *    colormap or 32 bpp, are written compressed as a set of three
 *    8 bpp (rgb) images.
 *
 *    Low-level error handling
 *    ------------------------
 *    The default behavior of the jpeg library is to call exit.
 *    This is often undesirable, and the caller should make the
 *    decision when to abort a process.  To prevent the jpeg library
 *    from calling exit(), setjmp() has been inserted into all
 *    readers and writers, and the cinfo struct has been set up so that
 *    the low-level jpeg library will call a special error handler
 *    that doesn't exit, instead of the default function error_exit().
 *
 *    To avoid race conditions and make these functions thread-safe in
 *    the rare situation where calls to two threads are simultaneously
 *    failing on bad jpegs, we insert a local copy of the jmp_buf struct
 *    into the cinfo.client_data field, and use this on longjmp.
 *    For extracting the jpeg comment, we have the added complication
 *    that the client_data field must also return the jpeg comment,
 *    and we use a different error handler.
 *
 *    How to avoid subsampling the chroma channels
 *    --------------------------------------------
 *    When writing, you can avoid subsampling the U,V (chroma)
 *    channels.  This gives higher quality for the color, which is
 *    important for some situations.  The default subsampling is 2x2 on
 *    both channels.  Before writing, call pixSetChromaSampling(pix, 0)
 *    to prevent chroma subsampling.
 *
 *    How to extract just the luminance channel in reading RGB
 *    --------------------------------------------------------
 *    For higher resolution and faster decoding of an RGB image, you
 *    can extract just the 8 bpp luminance channel, using pixReadJpeg(),
 *    where you use L_JPEG_READ_LUMINANCE for the %hint arg.
 *
 *    How to fail to read if the data is corrupted
 *    ---------------------------------------------
 *    By default, if the low-level jpeg library functions do not abort,
 *    a pix will be returned, even if the data is corrupted and warnings
 *    are issued.  In order to be most likely to fail to read when there
 *    is data corruption, use L_JPEG_FAIL_ON_BAD_DATA in the %hint arg.
 *
 *    Compressing to memory and decompressing from memory
 *    ---------------------------------------------------
 *    On systems like windows without fmemopen() and open_memstream(),
 *    we write data to a temp file and read it back for operations
 *    between pix and compressed-data, such as pixReadMemJpeg() and
 *    pixWriteMemJpeg().
 *
 *    Vestigial code: parsing the jpeg file for header metadata
 *    ---------------------------------------------------------
 *    For extracting header metadata, we previously parsed the file, looking
 *    for specific markers.  This is error-prone because of non-standard
 *    jpeg files, and we now use readHeaderJpeg() and readHeaderMemJpeg().
 *    The vestigial code is retained in jpegio_notused.c to help you
 *    understand a bit about how to parse jpeg markers.  It is not compiled
 *    into the library.
 * </pre>
 */

#ifdef HAVE_CONFIG_H
#include "config_auto.h"
#endif  /* HAVE_CONFIG_H */

#include <string.h>
#include "allheaders.h"

/* --------------------------------------------*/
#if  HAVE_LIBJPEG   /* defined in environ.h */
/* --------------------------------------------*/

#include <setjmp.h>

    /* jconfig.h makes the error of setting
     *   #define HAVE_STDLIB_H
     * which conflicts with config_auto.h (where it is set to 1) and results
     * for some gcc compiler versions in a warning.  The conflict is harmless
     * but we suppress it by undefining the variable. */
#undef HAVE_STDLIB_H
#include "jpeglib.h"

static void jpeg_error_catch_all_1(j_common_ptr cinfo);
static void jpeg_error_catch_all_2(j_common_ptr cinfo);
static l_uint8 jpeg_getc(j_decompress_ptr cinfo);

    /* Note: 'boolean' is defined in jmorecfg.h.  We use it explicitly
     * here because for windows where __MINGW32__ is defined,
     * the prototype for jpeg_comment_callback() is given as
     * returning a boolean.  */
static boolean jpeg_comment_callback(j_decompress_ptr cinfo);

    /* This is saved in the client_data field of cinfo, and used both
     * to retrieve the comment from its callback and to handle
     * exceptions with a longjmp. */
struct callback_data {
    jmp_buf   jmpbuf;
    l_uint8  *comment;
};

#ifndef  NO_CONSOLE_IO
#define  DEBUG_INFO      0
#endif  /* ~NO_CONSOLE_IO */


/*---------------------------------------------------------------------*
 *                 Read jpeg from file (special function)              *
 *---------------------------------------------------------------------*/
/*!
 * \brief   pixReadJpeg()
 *
 * \param[in]    filename
 * \param[in]    cmapflag 0 for no colormap in returned pix;
 *                        1 to return an 8 bpp cmapped pix if spp = 3 or 4
 * \param[in]    reduction scaling factor: 1, 2, 4 or 8
 * \param[out]   pnwarn [optional] number of warnings about
 *                       corrupted data
 * \param[in]    hint a bitwise OR of L_JPEG_* values; 0 for default
 * \return  pix, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) This is a special function for reading jpeg files.
 *      (2) Use this if you want the jpeg library to create
 *          an 8 bpp colormapped image.
 *      (3) Images reduced by factors of 2, 4 or 8 can be returned
 *          significantly faster than full resolution images.
 *      (4) If the jpeg data is bad, the jpeg library will continue
 *          silently, or return warnings, or attempt to exit.  Depending
 *          on the severity of the data corruption, there are two possible
 *          outcomes:
 *          (a) a possibly damaged pix can be generated, along with zero
 *              or more warnings, or
 *          (b) the library will attempt to exit (caught by our error
 *              handler) and no pix will be returned.
 *          If a pix is generated with at least one warning of data
 *          corruption, and if L_JPEG_FAIL_ON_BAD_DATA is included in %hint,
 *          no pix will be returned.
 *      (5) The possible hint values are given in the enum in imageio.h:
 *            * L_JPEG_READ_LUMINANCE
 *            * L_JPEG_FAIL_ON_BAD_DATA
 *          Default (0) is to do neither.
 * </pre>
 */
PIX *
pixReadJpeg(const char  *filename,
            l_int32      cmapflag,
            l_int32      reduction,
            l_int32     *pnwarn,
            l_int32      hint)
{
l_int32   ret;
l_uint8  *comment;
FILE     *fp;
PIX      *pix;

    PROCNAME("pixReadJpeg");

    if (pnwarn) *pnwarn = 0;
    if (!filename)
        return (PIX *)ERROR_PTR("filename not defined", procName, NULL);
    if (cmapflag != 0 && cmapflag != 1)
        cmapflag = 0;  /* default */
    if (reduction != 1 && reduction != 2 && reduction != 4 && reduction != 8)
        return (PIX *)ERROR_PTR("reduction not in {1,2,4,8}", procName, NULL);

    if ((fp = fopenReadStream(filename)) == NULL)
        return (PIX *)ERROR_PTR("image file not found", procName, NULL);
    pix = pixReadStreamJpeg(fp, cmapflag, reduction, pnwarn, hint);
    if (pix) {
        ret = fgetJpegComment(fp, &comment);
        if (!ret && comment)
            pixSetText(pix, (char *)comment);
        LEPT_FREE(comment);
    }
    fclose(fp);

    if (!pix)
        return (PIX *)ERROR_PTR("image not returned", procName, NULL);
    return pix;
}


/*!
 * \brief   pixReadStreamJpeg()
 *
 * \param[in]    fp file stream
 * \param[in]    cmapflag 0 for no colormap in returned pix;
 *                        1 to return an 8 bpp cmapped pix if spp = 3 or 4
 * \param[in]    reduction scaling factor: 1, 2, 4 or 8
 * \param[out]   pnwarn [optional] number of warnings
 * \param[in]    hint a bitwise OR of L_JPEG_* values; 0 for default
 * \return  pix, or NULL on error
 *
 *  Usage: see pixReadJpeg
 * <pre>
 * Notes:
 *      (1) The jpeg comment, if it exists, is not stored in the pix.
 * </pre>
 */
PIX *
pixReadStreamJpeg(FILE     *fp,
                  l_int32   cmapflag,
                  l_int32   reduction,
                  l_int32  *pnwarn,
                  l_int32   hint)
{
l_int32                        cyan, yellow, magenta, black, nwarn;
l_int32                        i, j, k, rval, gval, bval;
l_int32                        w, h, wpl, spp, ncolors, cindex, ycck, cmyk;
l_uint32                      *data;
l_uint32                      *line, *ppixel;
JSAMPROW                       rowbuffer;
PIX                           *pix;
PIXCMAP                       *cmap;
struct jpeg_decompress_struct  cinfo;
struct jpeg_error_mgr          jerr;
jmp_buf                        jmpbuf;  /* must be local to the function */

    PROCNAME("pixReadStreamJpeg");

    if (pnwarn) *pnwarn = 0;
    if (!fp)
        return (PIX *)ERROR_PTR("fp not defined", procName, NULL);
    if (cmapflag != 0 && cmapflag != 1)
        cmapflag = 0;  /* default */
    if (reduction != 1 && reduction != 2 && reduction != 4 && reduction != 8)
        return (PIX *)ERROR_PTR("reduction not in {1,2,4,8}", procName, NULL);

    if (BITS_IN_JSAMPLE != 8)  /* set in jmorecfg.h */
        return (PIX *)ERROR_PTR("BITS_IN_JSAMPLE != 8", procName, NULL);

    rewind(fp);
    pix = NULL;
    rowbuffer = NULL;

        /* Modify the jpeg error handling to catch fatal errors  */
    cinfo.err = jpeg_std_error(&jerr);
    jerr.error_exit = jpeg_error_catch_all_1;
    cinfo.client_data = (void *)&jmpbuf;
    if (setjmp(jmpbuf)) {
        pixDestroy(&pix);
        LEPT_FREE(rowbuffer);
        return (PIX *)ERROR_PTR("internal jpeg error", procName, NULL);
    }

        /* Initialize jpeg structs for decompression */
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, fp);
    jpeg_read_header(&cinfo, TRUE);
    cinfo.scale_denom = reduction;
    cinfo.scale_num = 1;
    jpeg_calc_output_dimensions(&cinfo);
    if (hint & L_JPEG_READ_LUMINANCE) {
        cinfo.out_color_space = JCS_GRAYSCALE;
        spp = 1;
        L_INFO("reading luminance channel only\n", procName);
    } else {
        spp = cinfo.out_color_components;
    }

        /* Allocate the image and a row buffer */
    w = cinfo.output_width;
    h = cinfo.output_height;
    ycck = (cinfo.jpeg_color_space == JCS_YCCK && spp == 4 && cmapflag == 0);
    cmyk = (cinfo.jpeg_color_space == JCS_CMYK && spp == 4 && cmapflag == 0);
    if (spp != 1 && spp != 3 && !ycck && !cmyk) {
        return (PIX *)ERROR_PTR("spp must be 1 or 3, or YCCK or CMYK",
                                procName, NULL);
    }
    if ((spp == 3 && cmapflag == 0) || ycck || cmyk) {  /* rgb or 4 bpp color */
        rowbuffer = (JSAMPROW)LEPT_CALLOC(sizeof(JSAMPLE), spp * w);
        pix = pixCreate(w, h, 32);
    } else {  /* 8 bpp gray or colormapped */
        rowbuffer = (JSAMPROW)LEPT_CALLOC(sizeof(JSAMPLE), w);
        pix = pixCreate(w, h, 8);
    }
    pixSetInputFormat(pix, IFF_JFIF_JPEG);
    if (!rowbuffer || !pix) {
        LEPT_FREE(rowbuffer);
        pixDestroy(&pix);
        return (PIX *)ERROR_PTR("rowbuffer or pix not made", procName, NULL);
    }

        /* Initialize decompression.  Set up a colormap for color
         * quantization if requested. */
    if (spp == 1) {  /* Grayscale or colormapped */
        jpeg_start_decompress(&cinfo);
    } else {        /* Color; spp == 3 or YCCK or CMYK */
        if (cmapflag == 0) {   /* 24 bit color in 32 bit pix or YCCK/CMYK */
            cinfo.quantize_colors = FALSE;
            jpeg_start_decompress(&cinfo);
        } else {      /* Color quantize to 8 bits */
            cinfo.quantize_colors = TRUE;
            cinfo.desired_number_of_colors = 256;
            jpeg_start_decompress(&cinfo);

                /* Construct a pix cmap */
            cmap = pixcmapCreate(8);
            ncolors = cinfo.actual_number_of_colors;
            for (cindex = 0; cindex < ncolors; cindex++) {
                rval = cinfo.colormap[0][cindex];
                gval = cinfo.colormap[1][cindex];
                bval = cinfo.colormap[2][cindex];
                pixcmapAddColor(cmap, rval, gval, bval);
            }
            pixSetColormap(pix, cmap);
        }
    }
    wpl  = pixGetWpl(pix);
    data = pixGetData(pix);

        /* Decompress.  Unfortunately, we cannot use the return value
         * from jpeg_read_scanlines() to determine if there was a problem
         * with the data; it always appears to return 1.  We can only
         * tell from the warnings during decoding, such as "premature
         * end of data segment".  The default behavior is to return an
         * image even if there are warnings.  However, by setting the
         * hint to have the same bit flag as L_JPEG_FAIL_ON_BAD_DATA,
         * no image will be returned if there are any warnings. */
    for (i = 0; i < h; i++) {
        if (jpeg_read_scanlines(&cinfo, &rowbuffer, (JDIMENSION)1) == 0) {
            L_ERROR("read error at scanline %d\n", procName, i);
            pixDestroy(&pix);
            jpeg_destroy_decompress(&cinfo);
            LEPT_FREE(rowbuffer);
            return (PIX *)ERROR_PTR("bad data", procName, NULL);
        }

            /* -- 24 bit color -- */
        if ((spp == 3 && cmapflag == 0) || ycck || cmyk) {
            ppixel = data + i * wpl;
            if (spp == 3) {
                for (j = k = 0; j < w; j++) {
                    SET_DATA_BYTE(ppixel, COLOR_RED, rowbuffer[k++]);
                    SET_DATA_BYTE(ppixel, COLOR_GREEN, rowbuffer[k++]);
                    SET_DATA_BYTE(ppixel, COLOR_BLUE, rowbuffer[k++]);
                    ppixel++;
                }
            } else {
                    /* This is a conversion from CMYK -> RGB that ignores
                       color profiles, and is invoked when the image header
                       claims to be in CMYK or YCCK colorspace.  If in YCCK,
                       libjpeg may be doing YCCK -> CMYK under the hood.
                       To understand why the colors need to be inverted on
                       read-in for the Adobe marker, see the "Special
                       color spaces" section of "Using the IJG JPEG
                       Library" by Thomas G. Lane:
                         http://www.jpegcameras.com/libjpeg/libjpeg-3.html#ss3.1
                       The non-Adobe conversion is equivalent to:
                           rval = black - black * cyan / 255
                           ...
                       The Adobe conversion is equivalent to:
                           rval = black - black * (255 - cyan) / 255
                           ...
                       Note that cyan is the complement to red, and we
                       are subtracting the complement color (weighted
                       by black) from black.  For Adobe conversions,
                       where they've already inverted the CMY but not
                       the K, we have to invert again.  The results
                       must be clipped to [0 ... 255]. */
                for (j = k = 0; j < w; j++) {
                    cyan = rowbuffer[k++];
                    magenta = rowbuffer[k++];
                    yellow = rowbuffer[k++];
                    black = rowbuffer[k++];
                    if (cinfo.saw_Adobe_marker) {
                        rval = (black * cyan) / 255;
                        gval = (black * magenta) / 255;
                        bval = (black * yellow) / 255;
                    } else {
                        rval = black * (255 - cyan) / 255;
                        gval = black * (255 - magenta) / 255;
                        bval = black * (255 - yellow) / 255;
                    }
                    rval = L_MIN(L_MAX(rval, 0), 255);
                    gval = L_MIN(L_MAX(gval, 0), 255);
                    bval = L_MIN(L_MAX(bval, 0), 255);
                    composeRGBPixel(rval, gval, bval, ppixel);
                    ppixel++;
                }
            }
        } else {    /* 8 bpp grayscale or colormapped pix */
            line = data + i * wpl;
            for (j = 0; j < w; j++)
                SET_DATA_BYTE(line, j, rowbuffer[j]);
        }
    }

    nwarn = cinfo.err->num_warnings;
    if (pnwarn) *pnwarn = nwarn;

        /* If the pixel density is neither 1 nor 2, it may not be defined.
         * In that case, don't set the resolution.  */
    if (cinfo.density_unit == 1) {  /* pixels per inch */
        pixSetXRes(pix, cinfo.X_density);
        pixSetYRes(pix, cinfo.Y_density);
    } else if (cinfo.density_unit == 2) {  /* pixels per centimeter */
        pixSetXRes(pix, (l_int32)((l_float32)cinfo.X_density * 2.54 + 0.5));
        pixSetYRes(pix, (l_int32)((l_float32)cinfo.Y_density * 2.54 + 0.5));
    }

    if (cinfo.output_components != spp)
        fprintf(stderr, "output spp = %d, spp = %d\n",
                cinfo.output_components, spp);

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    LEPT_FREE(rowbuffer);

    if (nwarn > 0) {
        if (hint & L_JPEG_FAIL_ON_BAD_DATA) {
            L_ERROR("fail with %d warning(s) of bad data\n", procName, nwarn);
            pixDestroy(&pix);
        } else {
            L_WARNING("%d warning(s) of bad data\n", procName, nwarn);
        }
    }

    return pix;
}


/*---------------------------------------------------------------------*
 *                     Read jpeg metadata from file                    *
 *---------------------------------------------------------------------*/
/*!
 * \brief   readHeaderJpeg()
 *
 * \param[in]    filename
 * \param[out]   pw [optional]
 *           [out]   ph ([optional]
 *           [out]   pspp ([optional]  samples/pixel
 * \param[out]   pycck [optional]  1 if ycck color space; 0 otherwise
 * \param[out]   pcmyk [optional]  1 if cmyk color space; 0 otherwise
 * \return  0 if OK, 1 on error
 */
l_int32
readHeaderJpeg(const char  *filename,
               l_int32     *pw,
               l_int32     *ph,
               l_int32     *pspp,
               l_int32     *pycck,
               l_int32     *pcmyk)
{
l_int32  ret;
FILE    *fp;

    PROCNAME("readHeaderJpeg");

    if (pw) *pw = 0;
    if (ph) *ph = 0;
    if (pspp) *pspp = 0;
    if (pycck) *pycck = 0;
    if (pcmyk) *pcmyk = 0;
    if (!filename)
        return ERROR_INT("filename not defined", procName, 1);
    if (!pw && !ph && !pspp && !pycck && !pcmyk)
        return ERROR_INT("no results requested", procName, 1);

    if ((fp = fopenReadStream(filename)) == NULL)
        return ERROR_INT("image file not found", procName, 1);
    ret = freadHeaderJpeg(fp, pw, ph, pspp, pycck, pcmyk);
    fclose(fp);
    return ret;
}


/*!
 * \brief   freadHeaderJpeg()
 *
 * \param[in]    fp file stream
 * \param[out]   pw [optional]
 *           [out]   ph ([optional]
 *           [out]   pspp ([optional]  samples/pixel
 * \param[out]   pycck [optional]  1 if ycck color space; 0 otherwise
 * \param[out]   pcmyk [optional]  1 if cmyk color space; 0 otherwise
 * \return  0 if OK, 1 on error
 */
l_int32
freadHeaderJpeg(FILE     *fp,
                l_int32  *pw,
                l_int32  *ph,
                l_int32  *pspp,
                l_int32  *pycck,
                l_int32  *pcmyk)
{
l_int32                        spp;
struct jpeg_decompress_struct  cinfo;
struct jpeg_error_mgr          jerr;
jmp_buf                        jmpbuf;  /* must be local to the function */

    PROCNAME("freadHeaderJpeg");

    if (pw) *pw = 0;
    if (ph) *ph = 0;
    if (pspp) *pspp = 0;
    if (pycck) *pycck = 0;
    if (pcmyk) *pcmyk = 0;
    if (!fp)
        return ERROR_INT("stream not defined", procName, 1);
    if (!pw && !ph && !pspp && !pycck && !pcmyk)
        return ERROR_INT("no results requested", procName, 1);

    rewind(fp);

        /* Modify the jpeg error handling to catch fatal errors  */
    cinfo.err = jpeg_std_error(&jerr);
    cinfo.client_data = (void *)&jmpbuf;
    jerr.error_exit = jpeg_error_catch_all_1;
    if (setjmp(jmpbuf))
        return ERROR_INT("internal jpeg error", procName, 1);

        /* Initialize the jpeg structs for reading the header */
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, fp);
    jpeg_read_header(&cinfo, TRUE);
    jpeg_calc_output_dimensions(&cinfo);

    spp = cinfo.out_color_components;
    if (pspp) *pspp = spp;
    if (pw) *pw = cinfo.output_width;
    if (ph) *ph = cinfo.output_height;
    if (pycck) *pycck =
        (cinfo.jpeg_color_space == JCS_YCCK && spp == 4);
    if (pcmyk) *pcmyk =
        (cinfo.jpeg_color_space == JCS_CMYK && spp == 4);

    jpeg_destroy_decompress(&cinfo);
    rewind(fp);
    return 0;
}


/*
 *  fgetJpegResolution()
 *
 *      Input:  fp (file stream opened for read)
 *              &xres, &yres (<return> resolution in ppi)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) If neither resolution field is set, this is not an error;
 *          the returned resolution values are 0 (designating 'unknown').
 *      (2) Side-effect: this rewinds the stream.
 */
l_int32
fgetJpegResolution(FILE     *fp,
                   l_int32  *pxres,
                   l_int32  *pyres)
{
struct jpeg_decompress_struct  cinfo;
struct jpeg_error_mgr          jerr;
jmp_buf                        jmpbuf;  /* must be local to the function */

    PROCNAME("fgetJpegResolution");

    if (pxres) *pxres = 0;
    if (pyres) *pyres = 0;
    if (!pxres || !pyres)
        return ERROR_INT("&xres and &yres not both defined", procName, 1);
    if (!fp)
        return ERROR_INT("stream not opened", procName, 1);

    rewind(fp);

        /* Modify the jpeg error handling to catch fatal errors  */
    cinfo.err = jpeg_std_error(&jerr);
    cinfo.client_data = (void *)&jmpbuf;
    jerr.error_exit = jpeg_error_catch_all_1;
    if (setjmp(jmpbuf))
        return ERROR_INT("internal jpeg error", procName, 1);

        /* Initialize the jpeg structs for reading the header */
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, fp);
    jpeg_read_header(&cinfo, TRUE);

        /* It is common for the input resolution to be omitted from the
         * jpeg file.  If density_unit is not 1 or 2, simply return 0. */
    if (cinfo.density_unit == 1) {  /* pixels/inch */
        *pxres = cinfo.X_density;
        *pyres = cinfo.Y_density;
    } else if (cinfo.density_unit == 2) {  /* pixels/cm */
        *pxres = (l_int32)((l_float32)cinfo.X_density * 2.54 + 0.5);
        *pyres = (l_int32)((l_float32)cinfo.Y_density * 2.54 + 0.5);
    }

    jpeg_destroy_decompress(&cinfo);
    rewind(fp);
    return 0;
}


/*
 *  fgetJpegComment()
 *
 *      Input:  fp (file stream opened for read)
 *              &comment (<return> comment)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) Side-effect: this rewinds the stream.
 */
l_int32
fgetJpegComment(FILE      *fp,
                l_uint8  **pcomment)
{
struct jpeg_decompress_struct  cinfo;
struct jpeg_error_mgr          jerr;
struct callback_data           cb_data;  /* contains local jmp_buf */

    PROCNAME("fgetJpegComment");

    if (!pcomment)
        return ERROR_INT("&comment not defined", procName, 1);
    *pcomment = NULL;
    if (!fp)
        return ERROR_INT("stream not opened", procName, 1);

    rewind(fp);

        /* Modify the jpeg error handling to catch fatal errors  */
    cinfo.err = jpeg_std_error(&jerr);
    jerr.error_exit = jpeg_error_catch_all_2;
    cb_data.comment = NULL;
    cinfo.client_data = (void *)&cb_data;
    if (setjmp(cb_data.jmpbuf)) {
        LEPT_FREE(cb_data.comment);
        return ERROR_INT("internal jpeg error", procName, 1);
    }

        /* Initialize the jpeg structs for reading the header */
    jpeg_create_decompress(&cinfo);
    jpeg_set_marker_processor(&cinfo, JPEG_COM, jpeg_comment_callback);
    jpeg_stdio_src(&cinfo, fp);
    jpeg_read_header(&cinfo, TRUE);

        /* Save the result */
    *pcomment = cb_data.comment;
    jpeg_destroy_decompress(&cinfo);
    rewind(fp);
    return 0;
}


/*---------------------------------------------------------------------*
 *                             Writing Jpeg                            *
 *---------------------------------------------------------------------*/
/*!
 * \brief   pixWriteJpeg()
 *
 * \param[in]    filename
 * \param[in]    pix  any depth; cmap is OK
 * \param[in]    quality 1 - 100; 75 is default
 * \param[in]    progressive 0 for baseline sequential; 1 for progressive
 * \return  0 if OK; 1 on error
 */
l_int32
pixWriteJpeg(const char  *filename,
             PIX         *pix,
             l_int32      quality,
             l_int32      progressive)
{
FILE  *fp;

    PROCNAME("pixWriteJpeg");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if (!filename)
        return ERROR_INT("filename not defined", procName, 1);

    if ((fp = fopenWriteStream(filename, "wb+")) == NULL)
        return ERROR_INT("stream not opened", procName, 1);

    if (pixWriteStreamJpeg(fp, pix, quality, progressive)) {
        fclose(fp);
        return ERROR_INT("pix not written to stream", procName, 1);
    }

    fclose(fp);
    return 0;
}


/*!
 * \brief   pixWriteStreamJpeg()
 *
 * \param[in]    fp file stream
 * \param[in]    pixs  any depth; cmap is OK
 * \param[in]    quality  1 - 100; 75 is default value; 0 is also default
 * \param[in]    progressive 0 for baseline sequential; 1 for progressive
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) Progressive encoding gives better compression, at the
 *          expense of slower encoding and decoding.
 *      (2) Standard chroma subsampling is 2x2 on both the U and V
 *          channels.  For highest quality, use no subsampling; this
 *          option is set by pixSetChromaSampling(pix, 0).
 *      (3) The only valid pixel depths in leptonica are 1, 2, 4, 8, 16
 *          and 32 bpp.  However, it is possible, and in some cases desirable,
 *          to write out a jpeg file using an rgb pix that has 24 bpp.
 *          This can be created by appending the raster data for a 24 bpp
 *          image (with proper scanline padding) directly to a 24 bpp
 *          pix that was created without a data array.
 *      (4) There are two compression paths in this function:
 *          * Grayscale image, no colormap: compress as 8 bpp image.
 *          * rgb full color image: copy each line into the color
 *            line buffer, and compress as three 8 bpp images.
 *      (5) Under the covers, the jpeg library transforms rgb to a
 *          luminance-chromaticity triple, each component of which is
 *          also 8 bits, and compresses that.  It uses 2 Huffman tables,
 *          a higher resolution one (with more quantization levels)
 *          for luminosity and a lower resolution one for the chromas.
 * </pre>
 */
l_int32
pixWriteStreamJpeg(FILE    *fp,
                   PIX     *pixs,
                   l_int32  quality,
                   l_int32  progressive)
{
l_int32                      xres, yres;
l_int32                      i, j, k;
l_int32                      w, h, d, wpl, spp, colorflag, rowsamples;
l_uint32                    *ppixel, *line, *data;
JSAMPROW                     rowbuffer;
PIX                         *pix;
struct jpeg_compress_struct  cinfo;
struct jpeg_error_mgr        jerr;
char                        *text;
jmp_buf                      jmpbuf;  /* must be local to the function */

    PROCNAME("pixWriteStreamJpeg");

    if (!fp)
        return ERROR_INT("stream not open", procName, 1);
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (quality <= 0) quality = 75;  /* default */
    if (quality > 100) {
        L_ERROR("invalid jpeg quality; setting to 75\n", procName);
        quality = 75;
    }

        /* If necessary, convert the pix so that it can be jpeg compressed.
         * The colormap is removed based on the source, so if the colormap
         * has only gray colors, the image will be compressed with spp = 1. */
    pixGetDimensions(pixs, &w, &h, &d);
    pix = NULL;
    if (pixGetColormap(pixs) != NULL) {
        L_INFO("removing colormap; may be better to compress losslessly\n",
               procName);
        pix = pixRemoveColormap(pixs, REMOVE_CMAP_BASED_ON_SRC);
    } else if (d >= 8 && d != 16) {  /* normal case; no rewrite */
        pix = pixClone(pixs);
    } else if (d < 8 || d == 16) {
        L_INFO("converting from %d to 8 bpp\n", procName, d);
        pix = pixConvertTo8(pixs, 0);  /* 8 bpp, no cmap */
    } else {
        L_ERROR("unknown pix type with d = %d and no cmap\n", procName, d);
        return 1;
    }
    if (!pix)
        return ERROR_INT("pix not made", procName, 1);
    pixSetPadBits(pix, 0);

    rewind(fp);
    rowbuffer = NULL;

        /* Modify the jpeg error handling to catch fatal errors  */
    cinfo.err = jpeg_std_error(&jerr);
    cinfo.client_data = (void *)&jmpbuf;
    jerr.error_exit = jpeg_error_catch_all_1;
    if (setjmp(jmpbuf)) {
        LEPT_FREE(rowbuffer);
        pixDestroy(&pix);
        return ERROR_INT("internal jpeg error", procName, 1);
    }

        /* Initialize the jpeg structs for compression */
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, fp);
    cinfo.image_width  = w;
    cinfo.image_height = h;

        /* Set the color space and number of components */
    d = pixGetDepth(pix);
    if (d == 8) {
        colorflag = 0;    /* 8 bpp grayscale; no cmap */
        cinfo.input_components = 1;
        cinfo.in_color_space = JCS_GRAYSCALE;
    } else {  /* d == 32 || d == 24 */
        colorflag = 1;    /* rgb */
        cinfo.input_components = 3;
        cinfo.in_color_space = JCS_RGB;
    }

    jpeg_set_defaults(&cinfo);

        /* Setting optimize_coding to TRUE seems to improve compression
         * by approx 2-4 percent, and increases comp time by approx 20%. */
    cinfo.optimize_coding = FALSE;

        /* Set resolution in pixels/in (density_unit: 1 = in, 2 = cm) */
    xres = pixGetXRes(pix);
    yres = pixGetYRes(pix);
    if ((xres != 0) && (yres != 0)) {
        cinfo.density_unit = 1;  /* designates pixels per inch */
        cinfo.X_density = xres;
        cinfo.Y_density = yres;
    }

        /* Set the quality and progressive parameters */
    jpeg_set_quality(&cinfo, quality, TRUE);
    if (progressive)
        jpeg_simple_progression(&cinfo);

        /* Set the chroma subsampling parameters.  This is done in
         * YUV color space.  The Y (intensity) channel is never subsampled.
         * The standard subsampling is 2x2 on both the U and V channels.
         * Notation on this is confusing.  For a nice illustrations, see
         *   http://en.wikipedia.org/wiki/Chroma_subsampling
         * The standard subsampling is written as 4:2:0.
         * We allow high quality where there is no subsampling on the
         * chroma channels: denoted as 4:4:4.  */
    if (pixs->special == L_NO_CHROMA_SAMPLING_JPEG) {
        cinfo.comp_info[0].h_samp_factor = 1;
        cinfo.comp_info[0].v_samp_factor = 1;
        cinfo.comp_info[1].h_samp_factor = 1;
        cinfo.comp_info[1].v_samp_factor = 1;
        cinfo.comp_info[2].h_samp_factor = 1;
        cinfo.comp_info[2].v_samp_factor = 1;
    }

    jpeg_start_compress(&cinfo, TRUE);

        /* Cap the text the length limit, 65533, for JPEG_COM payload.
         * Just to be safe, subtract 100 to cover the Adobe name space.  */
    if ((text = pixGetText(pix)) != NULL) {
        if (strlen(text) > 65433) {
            L_WARNING("text is %lu bytes; clipping to 65433\n",
                   procName, (unsigned long)strlen(text));
            text[65433] = '\0';
        }
        jpeg_write_marker(&cinfo, JPEG_COM, (const JOCTET *)text, strlen(text));
    }

        /* Allocate row buffer */
    spp = cinfo.input_components;
    rowsamples = spp * w;
    if ((rowbuffer = (JSAMPROW)LEPT_CALLOC(sizeof(JSAMPLE), rowsamples))
        == NULL) {
        pixDestroy(&pix);
        return ERROR_INT("calloc fail for rowbuffer", procName, 1);
    }

    data = pixGetData(pix);
    wpl  = pixGetWpl(pix);
    for (i = 0; i < h; i++) {
        line = data + i * wpl;
        if (colorflag == 0) {        /* 8 bpp gray */
            for (j = 0; j < w; j++)
                rowbuffer[j] = GET_DATA_BYTE(line, j);
        } else {  /* colorflag == 1 */
            if (d == 24) {  /* See note 3 above; special case of 24 bpp rgb */
                jpeg_write_scanlines(&cinfo, (JSAMPROW *)&line, 1);
            } else {  /* standard 32 bpp rgb */
                ppixel = line;
                for (j = k = 0; j < w; j++) {
                    rowbuffer[k++] = GET_DATA_BYTE(ppixel, COLOR_RED);
                    rowbuffer[k++] = GET_DATA_BYTE(ppixel, COLOR_GREEN);
                    rowbuffer[k++] = GET_DATA_BYTE(ppixel, COLOR_BLUE);
                    ppixel++;
                }
            }
        }
        if (d != 24)
            jpeg_write_scanlines(&cinfo, &rowbuffer, 1);
    }
    jpeg_finish_compress(&cinfo);

    pixDestroy(&pix);
    LEPT_FREE(rowbuffer);
    jpeg_destroy_compress(&cinfo);
    return 0;
}


/*---------------------------------------------------------------------*
 *                         Read/write to memory                        *
 *---------------------------------------------------------------------*/

/*!
 * \brief   pixReadMemJpeg()
 *
 * \param[in]    data const; jpeg-encoded
 * \param[in]    size of data
 * \param[in]    cmflag colormap flag 0 means return RGB image if color;
 *                      1 means create a colormap and return
 *                      an 8 bpp colormapped image if color
 * \param[in]    reduction scaling factor: 1, 2, 4 or 8
 * \param[out]   pnwarn [optional] number of warnings
 * \param[in]    hint a bitwise OR of L_JPEG_* values; 0 for default
 * \return  pix, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) The %size byte of %data must be a null character.
 *      (2) The only hint flag so far is L_JPEG_READ_LUMINANCE,
 *          given in the enum in imageio.h.
 *      (3) See pixReadJpeg() for usage.
 * </pre>
 */
PIX *
pixReadMemJpeg(const l_uint8  *data,
               size_t          size,
               l_int32         cmflag,
               l_int32         reduction,
               l_int32        *pnwarn,
               l_int32         hint)
{
l_int32   ret;
l_uint8  *comment;
FILE     *fp;
PIX      *pix;

    PROCNAME("pixReadMemJpeg");

    if (pnwarn) *pnwarn = 0;
    if (!data)
        return (PIX *)ERROR_PTR("data not defined", procName, NULL);

    if ((fp = fopenReadFromMemory(data, size)) == NULL)
        return (PIX *)ERROR_PTR("stream not opened", procName, NULL);
    pix = pixReadStreamJpeg(fp, cmflag, reduction, pnwarn, hint);
    if (pix) {
        ret = fgetJpegComment(fp, &comment);
        if (!ret && comment) {
            pixSetText(pix, (char *)comment);
            LEPT_FREE(comment);
        }
    }
    fclose(fp);
    if (!pix) L_ERROR("pix not read\n", procName);
    return pix;
}


/*!
 * \brief   readHeaderMemJpeg()
 *
 * \param[in]    data    const; jpeg-encoded
 * \param[in]    size    of data
 * \param[out]   pw      [optional] width
 * \param[out]   ph      [optional] height
 * \param[out]   pspp    [optional] samples/pixel
 * \param[out]   pycck   [optional] 1 if ycck color space; 0 otherwise
 * \param[out]   pcmyk   [optional] 1 if cmyk color space; 0 otherwise
 * \return  0 if OK, 1 on error
 */
l_int32
readHeaderMemJpeg(const l_uint8  *data,
                  size_t          size,
                  l_int32        *pw,
                  l_int32        *ph,
                  l_int32        *pspp,
                  l_int32        *pycck,
                  l_int32        *pcmyk)
{
l_int32  ret;
FILE    *fp;

    PROCNAME("readHeaderMemJpeg");

    if (pw) *pw = 0;
    if (ph) *ph = 0;
    if (pspp) *pspp = 0;
    if (pycck) *pycck = 0;
    if (pcmyk) *pcmyk = 0;
    if (!data)
        return ERROR_INT("data not defined", procName, 1);
    if (!pw && !ph && !pspp && !pycck && !pcmyk)
        return ERROR_INT("no results requested", procName, 1);

    if ((fp = fopenReadFromMemory(data, size)) == NULL)
        return ERROR_INT("stream not opened", procName, 1);
    ret = freadHeaderJpeg(fp, pw, ph, pspp, pycck, pcmyk);
    fclose(fp);
    return ret;
}


/*!
 * \brief   readResolutionMemJpeg()
 *
 * \param[in]   data    const; jpeg-encoded
 * \param[in]   size    of data
 * \param[out]  pxres   [optional]
 * \param[out]  pyres   [optional]
 * \return  0 if OK, 1 on error
 */
l_int32
readResolutionMemJpeg(const l_uint8  *data,
                      size_t          size,
                      l_int32        *pxres,
                      l_int32        *pyres)
{
l_int32  ret;
FILE    *fp;

    PROCNAME("readResolutionMemJpeg");

    if (pxres) *pxres = 0;
    if (pyres) *pyres = 0;
    if (!data)
        return ERROR_INT("data not defined", procName, 1);
    if (!pxres && !pyres)
        return ERROR_INT("no results requested", procName, 1);

    if ((fp = fopenReadFromMemory(data, size)) == NULL)
        return ERROR_INT("stream not opened", procName, 1);
    ret = fgetJpegResolution(fp, pxres, pyres);
    fclose(fp);
    return ret;
}


/*!
 * \brief   pixWriteMemJpeg()
 *
 * \param[out]   pdata data of jpeg compressed image
 * \param[out]   psize size of returned data
 * \param[in]    pix  any depth; cmap is OK
 * \param[in]    quality  1 - 100; 75 is default value; 0 is also default
 * \param[in]    progressive 0 for baseline sequential; 1 for progressive
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) See pixWriteStreamJpeg() for usage.  This version writes to
 *          memory instead of to a file stream.
 * </pre>
 */
l_int32
pixWriteMemJpeg(l_uint8  **pdata,
                size_t    *psize,
                PIX       *pix,
                l_int32    quality,
                l_int32    progressive)
{
l_int32  ret;
FILE    *fp;

    PROCNAME("pixWriteMemJpeg");

    if (pdata) *pdata = NULL;
    if (psize) *psize = 0;
    if (!pdata)
        return ERROR_INT("&data not defined", procName, 1 );
    if (!psize)
        return ERROR_INT("&size not defined", procName, 1 );
    if (!pix)
        return ERROR_INT("&pix not defined", procName, 1 );

#if HAVE_FMEMOPEN
    if ((fp = open_memstream((char **)pdata, psize)) == NULL)
        return ERROR_INT("stream not opened", procName, 1);
    ret = pixWriteStreamJpeg(fp, pix, quality, progressive);
#else
    L_INFO("work-around: writing to a temp file\n", procName);
  #ifdef _WIN32
    if ((fp = fopenWriteWinTempfile()) == NULL)
        return ERROR_INT("tmpfile stream not opened", procName, 1);
  #else
    if ((fp = tmpfile()) == NULL)
        return ERROR_INT("tmpfile stream not opened", procName, 1);
  #endif  /* _WIN32 */
    ret = pixWriteStreamJpeg(fp, pix, quality, progressive);
    rewind(fp);
    *pdata = l_binaryReadStream(fp, psize);
#endif  /* HAVE_FMEMOPEN */
    fclose(fp);
    return ret;
}


/*---------------------------------------------------------------------*
 *           Setting special flag for chroma sampling on write         *
 *---------------------------------------------------------------------*/
/*!
 * \brief   pixSetChromaSampling()
 *
 * \param[in]    pix
 * \param[in]    sampling 1 for subsampling; 0 for no subsampling
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) The default is for 2x2 chroma subsampling because the files are
 *          considerably smaller and the appearance is typically satisfactory.
 *          To get full resolution output in the chroma channels for
 *          jpeg writing, call this with %sampling == 0.
 * </pre>
 */
l_int32
pixSetChromaSampling(PIX     *pix,
                     l_int32  sampling)
{
    PROCNAME("pixSetChromaSampling");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1 );
    if (sampling)
        pixSetSpecial(pix, 0);  /* default */
    else
        pixSetSpecial(pix, L_NO_CHROMA_SAMPLING_JPEG);
    return 0;
}


/*---------------------------------------------------------------------*
 *                        Static system helpers                        *
 *---------------------------------------------------------------------*/
/*!
 * \brief   jpeg_error_catch_all_1()
 *
 *  Notes:
 *      (1) The default jpeg error_exit() kills the process, but we
 *          never want a call to leptonica to kill a process.  If you
 *          do want this behavior, remove the calls to these error handlers.
 *      (2) This is used where cinfo->client_data holds only jmpbuf.
 */
static void
jpeg_error_catch_all_1(j_common_ptr cinfo)
{
    jmp_buf *pjmpbuf = (jmp_buf *)cinfo->client_data;
    (*cinfo->err->output_message) (cinfo);
    jpeg_destroy(cinfo);
    longjmp(*pjmpbuf, 1);
    return;
}

/*!
 * \brief   jpeg_error_catch_all_2()
 *
 *  Notes:
 *      (1) This is used where cinfo->client_data needs to hold both
 *          the jmpbuf and the jpeg comment data.
 *      (2) On error, the comment data will be freed by the caller.
 */
static void
jpeg_error_catch_all_2(j_common_ptr cinfo)
{
struct callback_data  *pcb_data;

    pcb_data = (struct callback_data *)cinfo->client_data;
    (*cinfo->err->output_message) (cinfo);
    jpeg_destroy(cinfo);
    longjmp(pcb_data->jmpbuf, 1);
    return;
}

/* This function was borrowed from libjpeg */
static l_uint8
jpeg_getc(j_decompress_ptr cinfo)
{
struct jpeg_source_mgr *datasrc;

    datasrc = cinfo->src;
    if (datasrc->bytes_in_buffer == 0) {
        if (! (*datasrc->fill_input_buffer) (cinfo)) {
            return 0;
        }
    }
    datasrc->bytes_in_buffer--;
    return GETJOCTET(*datasrc->next_input_byte++);
}

/*!
 * \brief   jpeg_comment_callback()
 *
 *  Notes:
 *      (1) This is used to read the jpeg comment (JPEG_COM).
 *          See the note above the declaration for why it returns
 *          a "boolean".
 */
static boolean
jpeg_comment_callback(j_decompress_ptr cinfo)
{
l_int32                length, i;
l_uint8               *comment;
struct callback_data  *pcb_data;

        /* Get the size of the comment */
    length = jpeg_getc(cinfo) << 8;
    length += jpeg_getc(cinfo);
    length -= 2;
    if (length <= 0)
        return 1;

        /* Extract the comment from the file */
    if ((comment = (l_uint8 *)LEPT_CALLOC(length + 1, sizeof(l_uint8))) == NULL)
        return 0;
    for (i = 0; i < length; i++)
        comment[i] = jpeg_getc(cinfo);

        /* Save the comment and return */
    pcb_data = (struct callback_data *)cinfo->client_data;
    pcb_data->comment = comment;
    return 1;
}

/* --------------------------------------------*/
#endif  /* HAVE_LIBJPEG */
/* --------------------------------------------*/
