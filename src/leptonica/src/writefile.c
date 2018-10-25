/*====================================================================*
 -  Copyright (C) 2001-2016 Leptonica.  All rights reserved.
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
 * writefile.c
 *
 *     Set jpeg quality for pixWrite() and pixWriteMem()
 *        l_int32     l_jpegSetQuality()
 *
 *     Set global variable LeptDebugOK for writing to named temp files
 *        l_int32     setLeptDebugOK()
 *
 *     High-level procedures for writing images to file:
 *        l_int32     pixaWriteFiles()
 *        l_int32     pixWriteDebug()
 *        l_int32     pixWrite()
 *        l_int32     pixWriteAutoFormat()
 *        l_int32     pixWriteStream()
 *        l_int32     pixWriteImpliedFormat()
 *
 *     Selection of output format if default is requested
 *        l_int32     pixChooseOutputFormat()
 *        l_int32     getImpliedFileFormat()
 *        l_int32     pixGetAutoFormat()
 *        const char *getFormatExtension()
 *
 *     Write to memory
 *        l_int32     pixWriteMem()
 *
 *     Image display for debugging
 *        l_int32     l_fileDisplay()
 *        l_int32     pixDisplay()
 *        l_int32     pixDisplayWithTitle()
 *        l_int32     pixSaveTiled()
 *        l_int32     pixSaveTiledOutline()
 *        l_int32     pixSaveTiledWithText()
 *        void        l_chooseDisplayProg()
 *
 *     Deprecated pix output for debugging (still used in tesseract 3.05)
 *        l_int32     pixDisplayWrite()
 *
 *  Supported file formats:
 *  (1) Writing is supported without any external libraries:
 *          bmp
 *          pnm   (including pbm, pgm, etc)
 *          spix  (raw serialized)
 *  (2) Writing is supported with installation of external libraries:
 *          png
 *          jpg   (standard jfif version)
 *          tiff  (including most varieties of compression)
 *          gif
 *          webp
 *          jp2 (jpeg2000)
 *  (3) Writing is supported through special interfaces:
 *          ps (PostScript, in psio1.c, psio2.c):
 *              level 1 (uncompressed)
 *              level 2 (g4 and dct encoding: requires tiff, jpg)
 *              level 3 (g4, dct and flate encoding: requires tiff, jpg, zlib)
 *          pdf (PDF, in pdfio.c):
 *              level 1 (g4 and dct encoding: requires tiff, jpg)
 *              level 2 (g4, dct and flate encoding: requires tiff, jpg, zlib)
 */

#include <string.h>
#include "allheaders.h"

    /* Display program (xv, xli, xzgv, open) to be invoked by pixDisplay()  */
#ifdef _WIN32
static l_int32  var_DISPLAY_PROG = L_DISPLAY_WITH_IV;  /* default */
#elif  defined(__APPLE__)
static l_int32  var_DISPLAY_PROG = L_DISPLAY_WITH_OPEN;  /* default */
#else
static l_int32  var_DISPLAY_PROG = L_DISPLAY_WITH_XZGV;  /* default */
#endif  /* _WIN32 */

static const l_int32  L_BUFSIZE = 512;
static const l_int32  MAX_DISPLAY_WIDTH = 1000;
static const l_int32  MAX_DISPLAY_HEIGHT = 800;
static const l_int32  MAX_SIZE_FOR_PNG = 200;

    /* PostScript output for printing */
static const l_float32  DEFAULT_SCALING = 1.0;

    /* Global array of image file format extension names.                */
    /* This is in 1-1 corrspondence with format enum in imageio.h.       */
    /* The empty string at the end represents the serialized format,     */
    /* which has no recognizable extension name, but the array must      */
    /* be padded to agree with the format enum.                          */
    /* (Note on 'const': The size of the array can't be defined 'const'  */
    /* because that makes it static.  The 'const' in the definition of   */
    /* the array refers to the strings in the array; the ptr to the      */
    /* array is not const and can be used 'extern' in other files.)      */
LEPT_DLL l_int32  NumImageFileFormatExtensions = 19;  /* array size */
LEPT_DLL const char *ImageFileFormatExtensions[] =
         {"unknown",
          "bmp",
          "jpg",
          "png",
          "tif",
          "tif",
          "tif",
          "tif",
          "tif",
          "tif",
          "tif",
          "pnm",
          "ps",
          "gif",
          "jp2",
          "webp",
          "pdf",
          "default",
          ""};

    /* Local map of image file name extension to output format */
struct ExtensionMap
{
    char     extension[8];
    l_int32  format;
};
static const struct ExtensionMap extension_map[] =
                            { { ".bmp",  IFF_BMP       },
                              { ".jpg",  IFF_JFIF_JPEG },
                              { ".jpeg", IFF_JFIF_JPEG },
                              { ".png",  IFF_PNG       },
                              { ".tif",  IFF_TIFF      },
                              { ".tiff", IFF_TIFF      },
                              { ".pnm",  IFF_PNM       },
                              { ".gif",  IFF_GIF       },
                              { ".jp2",  IFF_JP2       },
                              { ".ps",   IFF_PS        },
                              { ".pdf",  IFF_LPDF      },
                              { ".webp", IFF_WEBP      } };


/*---------------------------------------------------------------------*
 *           Set jpeg quality for pixWrite() and pixWriteMem()         *
 *---------------------------------------------------------------------*/
    /* Parameter that controls jpeg quality for high-level calls. */
static l_int32  var_JPEG_QUALITY = 75;   /* default */

/*!
 * \brief   l_jpegSetQuality()
 *
 * \param[in]    new_quality    1 - 100; 75 is default; 0 defaults to 75
 * \return       prev           previous quality
 *
 * <pre>
 * Notes:
 *      (1) This variable is used in pixWriteStream() and pixWriteMem(),
 *          to control the jpeg quality.  The default is 75.
 *      (2) It returns the previous quality, so for example:
 *           l_int32  prev = l_jpegSetQuality(85);  //sets to 85
 *           pixWriteStream(...);
 *           l_jpegSetQuality(prev);   // resets to previous value
 *      (3) On error, logs a message and does not change the variable.
 */
l_int32
l_jpegSetQuality(l_int32  new_quality)
{
l_int32  prevq, newq;

    PROCNAME("l_jpeqSetQuality");

    prevq = var_JPEG_QUALITY;
    newq = (new_quality == 0) ? 75 : new_quality;
    if (newq < 1 || newq > 100)
        L_ERROR("invalid jpeg quality; unchanged\n", procName);
    else
        var_JPEG_QUALITY = newq;
    return prevq;
}


/*----------------------------------------------------------------------*
 *    Set global variable LeptDebugOK for writing to named temp files   *
 *----------------------------------------------------------------------*/
l_int32 LeptDebugOK = 0;  /* default value */
/*!
 * \brief   setLeptDebugOK()
 *
 * \param[in]    allow     TRUE (1) or FALSE (0)
 * \return       void
 *
 * <pre>
 * Notes:
 *      (1) This sets or clears the global variable LeptDebugOK, to
 *          control writing files in a temp directory with names that
 *          are compiled in.
 *      (2) The default in the library distribution is 0.  Call with
 *          %allow = 1 for development and debugging.
 */
void
setLeptDebugOK(l_int32  allow)
{
    if (allow != 0) allow = 1;
    LeptDebugOK = allow;
}


/*---------------------------------------------------------------------*
 *           Top-level procedures for writing images to file           *
 *---------------------------------------------------------------------*/
/*!
 * \brief   pixaWriteFiles()
 *
 * \param[in]    rootname
 * \param[in]    pixa
 * \param[in]    format  defined in imageio.h; see notes for default
 * \return  0 if OK; 1 on error
 *
 * <pre>
 * Notes:
 *      (1) Use %format = IFF_DEFAULT to decide the output format
 *          individually for each pix.
 * </pre>
 */
l_int32
pixaWriteFiles(const char  *rootname,
               PIXA        *pixa,
               l_int32      format)
{
char     bigbuf[L_BUFSIZE];
l_int32  i, n, pixformat;
PIX     *pix;

    PROCNAME("pixaWriteFiles");

    if (!rootname)
        return ERROR_INT("rootname not defined", procName, 1);
    if (!pixa)
        return ERROR_INT("pixa not defined", procName, 1);
    if (format < 0 || format == IFF_UNKNOWN ||
        format >= NumImageFileFormatExtensions)
        return ERROR_INT("invalid format", procName, 1);

    n = pixaGetCount(pixa);
    for (i = 0; i < n; i++) {
        pix = pixaGetPix(pixa, i, L_CLONE);
        if (format == IFF_DEFAULT)
            pixformat = pixChooseOutputFormat(pix);
        else
            pixformat = format;
        snprintf(bigbuf, L_BUFSIZE, "%s%03d.%s", rootname, i,
                 ImageFileFormatExtensions[pixformat]);
        pixWrite(bigbuf, pix, pixformat);
        pixDestroy(&pix);
    }

    return 0;
}


/*!
 * \brief   pixWriteDebug()
 *
 * \param[in]    fname
 * \param[in]    pix
 * \param[in]    format  defined in imageio.h
 * \return  0 if OK; 1 on error
 *
 * <pre>
 * Notes:
 *      (1) Debug version, intended for use in the library when writing
 *          to files in a temp directory with names that are compiled in.
 *          This is used instead of pixWrite() for all such library calls.
 *      (2) The global variable LeptDebugOK defaults to 0, and can be set
 *          or cleared by the function setLeptDebugOK().
 * </pre>
 */
l_int32
pixWriteDebug(const char  *fname,
              PIX         *pix,
              l_int32      format)
{
    PROCNAME("pixWriteDebug");

    if (LeptDebugOK) {
        return pixWrite(fname, pix, format);
    } else {
        L_INFO("write to named temp file %s is disabled\n", procName, fname);
        return 0;
    }
}


/*!
 * \brief   pixWrite()
 *
 * \param[in]    fname
 * \param[in]    pix
 * \param[in]    format  defined in imageio.h
 * \return  0 if OK; 1 on error
 *
 * <pre>
 * Notes:
 *      (1) Open for write using binary mode (with the "b" flag)
 *          to avoid having Windows automatically translate the NL
 *          into CRLF, which corrupts image files.  On non-windows
 *          systems this flag should be ignored, per ISO C90.
 *          Thanks to Dave Bryan for pointing this out.
 *      (2) If the default image format IFF_DEFAULT is requested:
 *          use the input format if known; otherwise, use a lossless format.
 *      (3) The default jpeg quality is 75.  For some other value,
 *          Use l_jpegSetQuality().
 * </pre>
 */
l_int32
pixWrite(const char  *fname,
         PIX         *pix,
         l_int32      format)
{
l_int32  ret;
FILE    *fp;

    PROCNAME("pixWrite");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if (!fname)
        return ERROR_INT("fname not defined", procName, 1);

    if ((fp = fopenWriteStream(fname, "wb+")) == NULL)
        return ERROR_INT("stream not opened", procName, 1);

    ret = pixWriteStream(fp, pix, format);
    fclose(fp);
    if (ret)
        return ERROR_INT("pix not written to stream", procName, 1);
    return 0;
}


/*!
 * \brief   pixWriteAutoFormat()
 *
 * \param[in]    filename
 * \param[in]    pix
 * \return  0 if OK; 1 on error
 */
l_int32
pixWriteAutoFormat(const char  *filename,
                   PIX         *pix)
{
l_int32  format;

    PROCNAME("pixWriteAutoFormat");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if (!filename)
        return ERROR_INT("filename not defined", procName, 1);

    if (pixGetAutoFormat(pix, &format))
        return ERROR_INT("auto format not returned", procName, 1);
    return pixWrite(filename, pix, format);
}


/*!
 * \brief   pixWriteStream()
 *
 * \param[in]    fp file stream
 * \param[in]    pix
 * \param[in]    format
 * \return  0 if OK; 1 on error.
 */
l_int32
pixWriteStream(FILE    *fp,
               PIX     *pix,
               l_int32  format)
{
    PROCNAME("pixWriteStream");

    if (!fp)
        return ERROR_INT("stream not defined", procName, 1);
    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);

    if (format == IFF_DEFAULT)
        format = pixChooseOutputFormat(pix);

    switch(format)
    {
    case IFF_BMP:
        pixWriteStreamBmp(fp, pix);
        break;

    case IFF_JFIF_JPEG:   /* default quality; baseline sequential */
        return pixWriteStreamJpeg(fp, pix, var_JPEG_QUALITY, 0);
        break;

    case IFF_PNG:   /* no gamma value stored */
        return pixWriteStreamPng(fp, pix, 0.0);
        break;

    case IFF_TIFF:           /* uncompressed */
    case IFF_TIFF_PACKBITS:  /* compressed, binary only */
    case IFF_TIFF_RLE:       /* compressed, binary only */
    case IFF_TIFF_G3:        /* compressed, binary only */
    case IFF_TIFF_G4:        /* compressed, binary only */
    case IFF_TIFF_LZW:       /* compressed, all depths */
    case IFF_TIFF_ZIP:       /* compressed, all depths */
        return pixWriteStreamTiff(fp, pix, format);
        break;

    case IFF_PNM:
        return pixWriteStreamPnm(fp, pix);
        break;

    case IFF_PS:
        return pixWriteStreamPS(fp, pix, NULL, 0, DEFAULT_SCALING);
        break;

    case IFF_GIF:
        return pixWriteStreamGif(fp, pix);
        break;

    case IFF_JP2:
        return pixWriteStreamJp2k(fp, pix, 34, 4, 0, 0);
        break;

    case IFF_WEBP:
        return pixWriteStreamWebP(fp, pix, 80, 0);
        break;

    case IFF_LPDF:
        return pixWriteStreamPdf(fp, pix, 0, NULL);
        break;

    case IFF_SPIX:
        return pixWriteStreamSpix(fp, pix);
        break;

    default:
        return ERROR_INT("unknown format", procName, 1);
        break;
    }

    return 0;
}


/*!
 * \brief   pixWriteImpliedFormat()
 *
 * \param[in]    filename
 * \param[in]    pix
 * \param[in]    quality iff JPEG; 1 - 100, 0 for default
 * \param[in]    progressive iff JPEG; 0 for baseline seq., 1 for progressive
 * \return  0 if OK; 1 on error
 *
 * <pre>
 * Notes:
 *      (1) This determines the output format from the filename extension.
 *      (2) The last two args are ignored except for requests for jpeg files.
 *      (3) The jpeg default quality is 75.
 * </pre>
 */
l_int32
pixWriteImpliedFormat(const char  *filename,
                      PIX         *pix,
                      l_int32      quality,
                      l_int32      progressive)
{
l_int32  format;

    PROCNAME("pixWriteImpliedFormat");

    if (!filename)
        return ERROR_INT("filename not defined", procName, 1);
    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);

        /* Determine output format */
    format = getImpliedFileFormat(filename);
    if (format == IFF_UNKNOWN) {
        format = IFF_PNG;
    } else if (format == IFF_TIFF) {
        if (pixGetDepth(pix) == 1)
            format = IFF_TIFF_G4;
        else
#ifdef _WIN32
            format = IFF_TIFF_LZW;  /* poor compression */
#else
            format = IFF_TIFF_ZIP;  /* native windows tools can't handle this */
#endif  /* _WIN32 */
    }

    if (format == IFF_JFIF_JPEG) {
        quality = L_MIN(quality, 100);
        quality = L_MAX(quality, 0);
        if (progressive != 0 && progressive != 1) {
            progressive = 0;
            L_WARNING("invalid progressive; setting to baseline\n", procName);
        }
        if (quality == 0)
            quality = 75;
        pixWriteJpeg (filename, pix, quality, progressive);
    } else {
        pixWrite(filename, pix, format);
    }

    return 0;
}


/*---------------------------------------------------------------------*
 *          Selection of output format if default is requested         *
 *---------------------------------------------------------------------*/
/*!
 * \brief   pixChooseOutputFormat()
 *
 * \param[in]    pix
 * \return  output format, or 0 on error
 *
 * <pre>
 * Notes:
 *      (1) This should only be called if the requested format is IFF_DEFAULT.
 *      (2) If the pix wasn't read from a file, its input format value
 *          will be IFF_UNKNOWN, and in that case it is written out
 *          in a compressed but lossless format.
 * </pre>
 */
l_int32
pixChooseOutputFormat(PIX  *pix)
{
l_int32  d, format;

    PROCNAME("pixChooseOutputFormat");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 0);

    d = pixGetDepth(pix);
    format = pixGetInputFormat(pix);
    if (format == IFF_UNKNOWN) {  /* output lossless */
        if (d == 1)
            format = IFF_TIFF_G4;
        else
            format = IFF_PNG;
    }

    return format;
}


/*!
 * \brief   getImpliedFileFormat()
 *
 * \param[in]    filename
 * \return  output format, or IFF_UNKNOWN on error or invalid extension.
 *
 * <pre>
 * Notes:
 *      (1) This determines the output file format from the extension
 *          of the input filename.
 * </pre>
 */
l_int32
getImpliedFileFormat(const char  *filename)
{
char    *extension;
int      i, numext;
l_int32  format = IFF_UNKNOWN;

    if (splitPathAtExtension (filename, NULL, &extension))
        return IFF_UNKNOWN;

    numext = sizeof(extension_map) / sizeof(extension_map[0]);
    for (i = 0; i < numext; i++) {
        if (!strcmp(extension, extension_map[i].extension)) {
            format = extension_map[i].format;
            break;
        }
    }

    LEPT_FREE(extension);
    return format;
}


/*!
 * \brief   pixGetAutoFormat()
 *
 * \param[in]    pix
 * \param[in]    &format
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) The output formats are restricted to tiff, jpeg and png
 *          because these are the most commonly used image formats and
 *          the ones that are typically installed with leptonica.
 *      (2) This decides what compression to use based on the pix.
 *          It chooses tiff-g4 if 1 bpp without a colormap, jpeg with
 *          quality 75 if grayscale, rgb or rgba (where it loses
 *          the alpha layer), and lossless png for all other situations.
 * </pre>
 */
l_int32
pixGetAutoFormat(PIX      *pix,
                 l_int32  *pformat)
{
l_int32   d;
PIXCMAP  *cmap;

    PROCNAME("pixGetAutoFormat");

    if (!pformat)
        return ERROR_INT("&format not defined", procName, 0);
    *pformat = IFF_UNKNOWN;
    if (!pix)
        return ERROR_INT("pix not defined", procName, 0);

    d = pixGetDepth(pix);
    cmap = pixGetColormap(pix);
    if (d == 1 && !cmap) {
        *pformat = IFF_TIFF_G4;
    } else if ((d == 8 && !cmap) || d == 24 || d == 32) {
        *pformat = IFF_JFIF_JPEG;
    } else {
        *pformat = IFF_PNG;
    }

    return 0;
}


/*!
 * \brief   getFormatExtension()
 *
 * \param[in]    format integer
 * \return  extension string, or NULL if format is out of range
 *
 * <pre>
 * Notes:
 *      (1) This string is NOT owned by the caller; it is just a pointer
 *          to a global string.  Do not free it.
 * </pre>
 */
const char *
getFormatExtension(l_int32  format)
{
    PROCNAME("getFormatExtension");

    if (format < 0 || format >= NumImageFileFormatExtensions)
        return (const char *)ERROR_PTR("invalid format", procName, NULL);

    return ImageFileFormatExtensions[format];
}


/*---------------------------------------------------------------------*
 *                            Write to memory                          *
 *---------------------------------------------------------------------*/
/*!
 * \brief   pixWriteMem()
 *
 * \param[out]   pdata data of tiff compressed image
 * \param[out]   psize size of returned data
 * \param[in]    pix
 * \param[in]    format  defined in imageio.h
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) On windows, this will only write tiff and PostScript to memory.
 *          For other formats, it requires open_memstream(3).
 *      (2) PostScript output is uncompressed, in hex ascii.
 *          Most printers support level 2 compression (tiff_g4 for 1 bpp,
 *          jpeg for 8 and 32 bpp).
 *      (3) The default jpeg quality is 75.  For some other value,
 *          Use l_jpegSetQuality().
 * </pre>
 */
l_int32
pixWriteMem(l_uint8  **pdata,
            size_t    *psize,
            PIX       *pix,
            l_int32    format)
{
l_int32  ret;

    PROCNAME("pixWriteMem");

    if (!pdata)
        return ERROR_INT("&data not defined", procName, 1 );
    if (!psize)
        return ERROR_INT("&size not defined", procName, 1 );
    if (!pix)
        return ERROR_INT("&pix not defined", procName, 1 );

    if (format == IFF_DEFAULT)
        format = pixChooseOutputFormat(pix);

    switch(format)
    {
    case IFF_BMP:
        ret = pixWriteMemBmp(pdata, psize, pix);
        break;

    case IFF_JFIF_JPEG:   /* default quality; baseline sequential */
        ret = pixWriteMemJpeg(pdata, psize, pix, var_JPEG_QUALITY, 0);
        break;

    case IFF_PNG:   /* no gamma value stored */
        ret = pixWriteMemPng(pdata, psize, pix, 0.0);
        break;

    case IFF_TIFF:           /* uncompressed */
    case IFF_TIFF_PACKBITS:  /* compressed, binary only */
    case IFF_TIFF_RLE:       /* compressed, binary only */
    case IFF_TIFF_G3:        /* compressed, binary only */
    case IFF_TIFF_G4:        /* compressed, binary only */
    case IFF_TIFF_LZW:       /* compressed, all depths */
    case IFF_TIFF_ZIP:       /* compressed, all depths */
        ret = pixWriteMemTiff(pdata, psize, pix, format);
        break;

    case IFF_PNM:
        ret = pixWriteMemPnm(pdata, psize, pix);
        break;

    case IFF_PS:
        ret = pixWriteMemPS(pdata, psize, pix, NULL, 0, DEFAULT_SCALING);
        break;

    case IFF_GIF:
        ret = pixWriteMemGif(pdata, psize, pix);
        break;

    case IFF_JP2:
        ret = pixWriteMemJp2k(pdata, psize, pix, 34, 0, 0, 0);
        break;

    case IFF_WEBP:
        ret = pixWriteMemWebP(pdata, psize, pix, 80, 0);
        break;

    case IFF_LPDF:
        ret = pixWriteMemPdf(pdata, psize, pix, 0, NULL);
        break;

    case IFF_SPIX:
        ret = pixWriteMemSpix(pdata, psize, pix);
        break;

    default:
        return ERROR_INT("unknown format", procName, 1);
        break;
    }

    return ret;
}


/*---------------------------------------------------------------------*
 *                      Image display for debugging                    *
 *---------------------------------------------------------------------*/
/*!
 * \brief   l_fileDisplay()
 *
 * \param[in]    fname
 * \param[in]    x, y  location of display frame on the screen
 * \param[in]    scale  scale factor (use 0 to skip display)
 * \return  0 if OK; 1 on error
 *
 * <pre>
 * Notes:
 *      (1) This is a convenient wrapper for displaying image files.
 *      (2) Set %scale = 0 to disable display.
 *      (3) This downscales 1 bpp to gray.
 * </pre>
 */
l_int32
l_fileDisplay(const char  *fname,
              l_int32      x,
              l_int32      y,
              l_float32    scale)
{
PIX  *pixs, *pixd;

    PROCNAME("l_fileDisplay");

    if (scale == 0.0)
        return 0;

    if (scale < 0.0)
        return ERROR_INT("invalid scale factor", procName, 1);
    if ((pixs = pixRead(fname)) == NULL)
        return ERROR_INT("pixs not read", procName, 1);

    if (scale == 1.0) {
        pixd = pixClone(pixs);
    } else {
        if (scale < 1.0 && pixGetDepth(pixs) == 1)
            pixd = pixScaleToGray(pixs, scale);
        else
            pixd = pixScale(pixs, scale, scale);
    }
    pixDisplay(pixd, x, y);
    pixDestroy(&pixs);
    pixDestroy(&pixd);
    return 0;
}


/*!
 * \brief   pixDisplay()
 *
 * \param[in]    pix 1, 2, 4, 8, 16, 32 bpp
 * \param[in]    x, y  location of display frame on the screen
 * \return  0 if OK; 1 on error
 *
 * <pre>
 * Notes:
 *      (1) This is debugging code that displays an image on the screen.
 *          It uses a static internal variable to number the output files
 *          written by a single process.  Behavior with a shared library
 *          may be unpredictable.
 *      (2) It uses these programs to display the image:
 *             On Unix: xzgv, xli or xv
 *             On Windows: i_view
 *          The display program must be on your $PATH variable.  It is
 *          chosen by setting the global var_DISPLAY_PROG, using
 *          l_chooseDisplayProg().  Default on Unix is xzgv.
 *      (3) Images with dimensions larger than MAX_DISPLAY_WIDTH or
 *          MAX_DISPLAY_HEIGHT are downscaled to fit those constraints.
 *          This is particularly important for displaying 1 bpp images
 *          with xv, because xv automatically downscales large images
 *          by subsampling, which looks poor.  For 1 bpp, we use
 *          scale-to-gray to get decent-looking anti-aliased images.
 *          In all cases, we write a temporary file to /tmp/lept/disp,
 *          that is read by the display program.
 *      (4) The temporary file is written as png if, after initial
 *          processing for special cases, any of these obtain:
 *            * pix dimensions are smaller than some thresholds
 *            * pix depth is less than 8 bpp
 *            * pix is colormapped
 *      (5) For spp == 4, we call pixDisplayLayersRGBA() to show 3
 *          versions of the image: the image with a fully opaque
 *          alpha, the alpha, and the image as it would appear with
 *          a white background.
 * </pre>
 */
l_int32
pixDisplay(PIX     *pixs,
           l_int32  x,
           l_int32  y)
{
    return pixDisplayWithTitle(pixs, x, y, NULL, 1);
}


/*!
 * \brief   pixDisplayWithTitle()
 *
 * \param[in]    pix 1, 2, 4, 8, 16, 32 bpp
 * \param[in]    x, y  location of display frame
 * \param[in]    title [optional] on frame; can be NULL;
 * \param[in]    dispflag 1 to write, else disabled
 * \return  0 if OK; 1 on error
 *
 * <pre>
 * Notes:
 *      (1) See notes for pixDisplay().
 *      (2) This displays the image if dispflag == 1; otherwise it punts.
 * </pre>
 */
l_int32
pixDisplayWithTitle(PIX         *pixs,
                    l_int32      x,
                    l_int32      y,
                    const char  *title,
                    l_int32      dispflag)
{
char           *tempname;
char            buffer[L_BUFSIZE];
static l_int32  index = 0;  /* caution: not .so or thread safe */
l_int32         w, h, d, spp, maxheight, opaque, threeviews, ignore;
l_float32       ratw, rath, ratmin;
PIX            *pix0, *pix1, *pix2;
PIXCMAP        *cmap;
#ifndef _WIN32
l_int32         wt, ht;
#else
char           *pathname;
char            fullpath[_MAX_PATH];
#endif  /* _WIN32 */

    PROCNAME("pixDisplayWithTitle");

    if (!LeptDebugOK) {
        L_INFO("displaying files is disabled\n", procName);
        return 0;
    }

    if (dispflag != 1) return 0;
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (var_DISPLAY_PROG != L_DISPLAY_WITH_XZGV &&
        var_DISPLAY_PROG != L_DISPLAY_WITH_XLI &&
        var_DISPLAY_PROG != L_DISPLAY_WITH_XV &&
        var_DISPLAY_PROG != L_DISPLAY_WITH_IV &&
        var_DISPLAY_PROG != L_DISPLAY_WITH_OPEN) {
        return ERROR_INT("no program chosen for display", procName, 1);
    }

        /* Display with three views if either spp = 4 or if colormapped
         * and the alpha component is not fully opaque */
    opaque = TRUE;
    if ((cmap = pixGetColormap(pixs)) != NULL)
        pixcmapIsOpaque(cmap, &opaque);
    spp = pixGetSpp(pixs);
    threeviews = (spp == 4 || !opaque) ? TRUE : FALSE;

        /* If colormapped and not opaque, remove the colormap to RGBA */
    if (!opaque)
        pix0 = pixRemoveColormap(pixs, REMOVE_CMAP_WITH_ALPHA);
    else
        pix0 = pixClone(pixs);

        /* Scale if necessary; this will also remove a colormap */
    pixGetDimensions(pix0, &w, &h, &d);
    maxheight = (threeviews) ? MAX_DISPLAY_HEIGHT / 3 : MAX_DISPLAY_HEIGHT;
    if (w <= MAX_DISPLAY_WIDTH && h <= maxheight) {
        if (d == 16)  /* take MSB */
            pix1 = pixConvert16To8(pix0, 1);
        else
            pix1 = pixClone(pix0);
    } else {
        ratw = (l_float32)MAX_DISPLAY_WIDTH / (l_float32)w;
        rath = (l_float32)maxheight / (l_float32)h;
        ratmin = L_MIN(ratw, rath);
        if (ratmin < 0.125 && d == 1)
            pix1 = pixScaleToGray8(pix0);
        else if (ratmin < 0.25 && d == 1)
            pix1 = pixScaleToGray4(pix0);
        else if (ratmin < 0.33 && d == 1)
            pix1 = pixScaleToGray3(pix0);
        else if (ratmin < 0.5 && d == 1)
            pix1 = pixScaleToGray2(pix0);
        else
            pix1 = pixScale(pix0, ratmin, ratmin);
    }
    pixDestroy(&pix0);
    if (!pix1)
        return ERROR_INT("pix1 not made", procName, 1);

        /* Generate the three views if required */
    if (threeviews)
        pix2 = pixDisplayLayersRGBA(pix1, 0xffffff00, 0);
    else
        pix2 = pixClone(pix1);

    if (index == 0) {  /* erase any existing images */
        lept_rmdir("lept/disp");
        lept_mkdir("lept/disp");
    }

    index++;
    if (pixGetDepth(pix2) < 8 || pixGetColormap(pix2) ||
        (w < MAX_SIZE_FOR_PNG && h < MAX_SIZE_FOR_PNG)) {
        snprintf(buffer, L_BUFSIZE, "/tmp/lept/disp/write.%03d.png", index);
        pixWrite(buffer, pix2, IFF_PNG);
    } else {
        snprintf(buffer, L_BUFSIZE, "/tmp/lept/disp/write.%03d.jpg", index);
        pixWrite(buffer, pix2, IFF_JFIF_JPEG);
    }
    tempname = genPathname(buffer, NULL);

#ifndef _WIN32

        /* Unix */
    if (var_DISPLAY_PROG == L_DISPLAY_WITH_XZGV) {
            /* no way to display title */
        pixGetDimensions(pix2, &wt, &ht, NULL);
        snprintf(buffer, L_BUFSIZE,
                 "xzgv --geometry %dx%d+%d+%d %s &", wt + 10, ht + 10,
                 x, y, tempname);
    } else if (var_DISPLAY_PROG == L_DISPLAY_WITH_XLI) {
        if (title) {
            snprintf(buffer, L_BUFSIZE,
               "xli -dispgamma 1.0 -quiet -geometry +%d+%d -title \"%s\" %s &",
               x, y, title, tempname);
        } else {
            snprintf(buffer, L_BUFSIZE,
               "xli -dispgamma 1.0 -quiet -geometry +%d+%d %s &",
               x, y, tempname);
        }
    } else if (var_DISPLAY_PROG == L_DISPLAY_WITH_XV) {
        if (title) {
            snprintf(buffer, L_BUFSIZE,
                     "xv -quit -geometry +%d+%d -name \"%s\" %s &",
                     x, y, title, tempname);
        } else {
            snprintf(buffer, L_BUFSIZE,
                     "xv -quit -geometry +%d+%d %s &", x, y, tempname);
        }
    } else if (var_DISPLAY_PROG == L_DISPLAY_WITH_OPEN) {
        snprintf(buffer, L_BUFSIZE, "open %s &", tempname);
    }
#ifndef OS_IOS /* iOS 11 does not support system() */
    ignore = system(buffer);
#endif /* !OS_IOS */

#else  /* _WIN32 */

        /* Windows: L_DISPLAY_WITH_IV */
    pathname = genPathname(tempname, NULL);
    _fullpath(fullpath, pathname, sizeof(fullpath));
    if (title) {
        snprintf(buffer, L_BUFSIZE,
                 "i_view32.exe \"%s\" /pos=(%d,%d) /title=\"%s\"",
                 fullpath, x, y, title);
    } else {
        snprintf(buffer, L_BUFSIZE, "i_view32.exe \"%s\" /pos=(%d,%d)",
                 fullpath, x, y);
    }
    ignore = system(buffer);
    LEPT_FREE(pathname);

#endif  /* _WIN32 */

    pixDestroy(&pix1);
    pixDestroy(&pix2);
    LEPT_FREE(tempname);
    return 0;
}


/*!
 * \brief   pixSaveTiled()
 *
 * \param[in]    pixs 1, 2, 4, 8, 32 bpp
 * \param[in]    pixa the pix are accumulated here
 * \param[in]    scalefactor 0.0 to disable; otherwise this is a scale factor
 * \param[in]    newrow 0 if placed on the same row as previous; 1 otherwise
 * \param[in]    space horizontal and vertical spacing, in pixels
 * \param[in]    dp depth of pixa; 8 or 32 bpp; only used on first call
 * \return  0 if OK, 1 on error.
 */
l_int32
pixSaveTiled(PIX       *pixs,
             PIXA      *pixa,
             l_float32  scalefactor,
             l_int32    newrow,
             l_int32    space,
             l_int32    dp)
{
        /* Save without an outline */
    return pixSaveTiledOutline(pixs, pixa, scalefactor, newrow, space, 0, dp);
}


/*!
 * \brief   pixSaveTiledOutline()
 *
 * \param[in]    pixs 1, 2, 4, 8, 32 bpp
 * \param[in]    pixa the pix are accumulated here
 * \param[in]    scalefactor 0.0 to disable; otherwise this is a scale factor
 * \param[in]    newrow 0 if placed on the same row as previous; 1 otherwise
 * \param[in]    space horizontal and vertical spacing, in pixels
 * \param[in]    linewidth width of added outline for image; 0 for no outline
 * \param[in]    dp depth of pixa; 8 or 32 bpp; only used on first call
 * \return  0 if OK, 1 on error.
 *
 * <pre>
 * Notes:
 *      (1) Before calling this function for the first time, use
 *          pixaCreate() to make the %pixa that will accumulate the pix.
 *          This is passed in each time pixSaveTiled() is called.
 *      (2) %scalefactor scales the input image.  After scaling and
 *          possible depth conversion, the image is saved in the input
 *          pixa, along with a box that specifies the location to
 *          place it when tiled later.  Disable saving the pix by
 *          setting %scalefactor == 0.0.
 *      (3) %newrow and %space specify the location of the new pix
 *          with respect to the last one(s) that were entered.
 *      (4) %dp specifies the depth at which all pix are saved.  It can
 *          be only 8 or 32 bpp.  Any colormap is removed.  This is only
 *          used at the first invocation.
 *      (5) This function uses two variables from call to call.
 *          If they were static, the function would not be .so or thread
 *          safe, and furthermore, there would be interference with two or
 *          more pixa accumulating images at a time.  Consequently,
 *          we use the first pix in the pixa to store and obtain both
 *          the depth and the current position of the bottom (one pixel
 *          below the lowest image raster line when laid out using
 *          the boxa).  The bottom variable is stored in the input format
 *          field, which is the only field available for storing an int.
 * </pre>
 */
l_int32
pixSaveTiledOutline(PIX       *pixs,
                    PIXA      *pixa,
                    l_float32  scalefactor,
                    l_int32    newrow,
                    l_int32    space,
                    l_int32    linewidth,
                    l_int32    dp)
{
l_int32  n, top, left, bx, by, bw, w, h, depth, bottom;
BOX     *box;
PIX     *pix1, *pix2, *pix3, *pix4;

    PROCNAME("pixSaveTiledOutline");

    if (scalefactor == 0.0) return 0;

    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (!pixa)
        return ERROR_INT("pixa not defined", procName, 1);

    n = pixaGetCount(pixa);
    if (n == 0) {
        bottom = 0;
        if (dp != 8 && dp != 32) {
            L_WARNING("dp not 8 or 32 bpp; using 32\n", procName);
            depth = 32;
        } else {
            depth = dp;
        }
    } else {  /* extract the depth and bottom params from the first pix */
        pix1 = pixaGetPix(pixa, 0, L_CLONE);
        depth = pixGetDepth(pix1);
        bottom = pixGetInputFormat(pix1);  /* not typical usage! */
        pixDestroy(&pix1);
    }

        /* Remove colormap if it exists; otherwise a copy.  This
         * guarantees that pix4 is not a clone of pixs. */
    pix1 = pixRemoveColormapGeneral(pixs, REMOVE_CMAP_BASED_ON_SRC, L_COPY);

        /* Scale and convert to output depth */
    if (scalefactor == 1.0) {
        pix2 = pixClone(pix1);
    } else if (scalefactor > 1.0) {
        pix2 = pixScale(pix1, scalefactor, scalefactor);
    } else {  /* scalefactor < 1.0) */
        if (pixGetDepth(pix1) == 1)
            pix2 = pixScaleToGray(pix1, scalefactor);
        else
            pix2 = pixScale(pix1, scalefactor, scalefactor);
    }
    pixDestroy(&pix1);
    if (depth == 8)
        pix3 = pixConvertTo8(pix2, 0);
    else
        pix3 = pixConvertTo32(pix2);
    pixDestroy(&pix2);

        /* Add black outline */
    if (linewidth > 0)
        pix4 = pixAddBorder(pix3, linewidth, 0);
    else
        pix4 = pixClone(pix3);
    pixDestroy(&pix3);

        /* Find position of current pix (UL corner plus size) */
    if (n == 0) {
        top = 0;
        left = 0;
    } else if (newrow == 1) {
        top = bottom + space;
        left = 0;
    } else {  /* n > 0 */
        pixaGetBoxGeometry(pixa, n - 1, &bx, &by, &bw, NULL);
        top = by;
        left = bx + bw + space;
    }

    pixGetDimensions(pix4, &w, &h, NULL);
    bottom = L_MAX(bottom, top + h);
    box = boxCreate(left, top, w, h);
    pixaAddPix(pixa, pix4, L_INSERT);
    pixaAddBox(pixa, box, L_INSERT);

        /* Save the new bottom value */
    pix1 = pixaGetPix(pixa, 0, L_CLONE);
    pixSetInputFormat(pix1, bottom);  /* not typical usage! */
    pixDestroy(&pix1);
    return 0;
}


/*!
 * \brief   pixSaveTiledWithText()
 *
 * \param[in]    pixs 1, 2, 4, 8, 32 bpp
 * \param[in]    pixa the pix are accumulated here; as 32 bpp
 * \param[in]    outwidth in pixels; use 0 to disable entirely
 * \param[in]    newrow 1 to start a new row; 0 to go on same row as previous
 * \param[in]    space horizontal and vertical spacing, in pixels
 * \param[in]    linewidth width of added outline for image; 0 for no outline
 * \param[in]    bmf [optional] font struct
 * \param[in]    textstr [optional] text string to be added
 * \param[in]    val color to set the text
 * \param[in]    location L_ADD_ABOVE, L_ADD_AT_TOP, L_ADD_AT_BOT, L_ADD_BELOW
 * \return  0 if OK, 1 on error.
 *
 * <pre>
 * Notes:
 *      (1) Before calling this function for the first time, use
 *          pixaCreate() to make the %pixa that will accumulate the pix.
 *          This is passed in each time pixSaveTiled() is called.
 *      (2) %outwidth is the scaled width.  After scaling, the image is
 *          saved in the input pixa, along with a box that specifies
 *          the location to place it when tiled later.  Disable saving
 *          the pix by setting %outwidth == 0.
 *      (3) %newrow and %space specify the location of the new pix
 *          with respect to the last one(s) that were entered.
 *      (4) All pix are saved as 32 bpp RGB.
 *      (5) If both %bmf and %textstr are defined, this generates a pix
 *          with the additional text; otherwise, no text is written.
 *      (6) The text is written before scaling, so it is properly
 *          antialiased in the scaled pix.  However, if the pix on
 *          different calls have different widths, the size of the
 *          text will vary.
 *      (7) See pixSaveTiledOutline() for other implementation details.
 * </pre>
 */
l_int32
pixSaveTiledWithText(PIX         *pixs,
                     PIXA        *pixa,
                     l_int32      outwidth,
                     l_int32      newrow,
                     l_int32      space,
                     l_int32      linewidth,
                     L_BMF       *bmf,
                     const char  *textstr,
                     l_uint32     val,
                     l_int32      location)
{
PIX  *pix1, *pix2, *pix3, *pix4;

    PROCNAME("pixSaveTiledWithText");

    if (outwidth == 0) return 0;

    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (!pixa)
        return ERROR_INT("pixa not defined", procName, 1);

    pix1 = pixConvertTo32(pixs);
    if (linewidth > 0)
        pix2 = pixAddBorder(pix1, linewidth, 0);
    else
        pix2 = pixClone(pix1);
    if (bmf && textstr)
        pix3 = pixAddSingleTextblock(pix2, bmf, textstr, val, location, NULL);
    else
        pix3 = pixClone(pix2);
    pix4 = pixScaleToSize(pix3, outwidth, 0);
    pixSaveTiled(pix4, pixa, 1.0, newrow, space, 32);
    pixDestroy(&pix1);
    pixDestroy(&pix2);
    pixDestroy(&pix3);
    pixDestroy(&pix4);
    return 0;
}


void
l_chooseDisplayProg(l_int32  selection)
{
    if (selection == L_DISPLAY_WITH_XLI ||
        selection == L_DISPLAY_WITH_XZGV ||
        selection == L_DISPLAY_WITH_XV ||
        selection == L_DISPLAY_WITH_IV ||
        selection == L_DISPLAY_WITH_OPEN) {
        var_DISPLAY_PROG = selection;
    } else {
        L_ERROR("invalid display program\n", "l_chooseDisplayProg");
    }
    return;
}


/*---------------------------------------------------------------------*
 *                Deprecated pix output for debugging                  *
 *---------------------------------------------------------------------*/
/*!
 * \brief   pixDisplayWrite()
 *
 * \param[in]    pix 1, 2, 4, 8, 16, 32 bpp
 * \param[in]    reduction -1 to reset/erase; 0 to disable;
 *                         otherwise this is a reduction factor
 * \return  0 if OK; 1 on error
 *
 * <pre>
 * Notes:
 *      (0) Deprecated.
 *      (1) This is a simple interface for writing a set of files.
 *      (2) This uses jpeg output for pix that are 32 bpp or 8 bpp
 *          without a colormap; otherwise, it uses png.
 *      (3) To erase any previously written files in the output directory:
 *             pixDisplayWrite(NULL, -1);
 *      (4) If reduction > 1 and depth == 1, this does a scale-to-gray
 *          reduction.
 *      (5) This function uses a static internal variable to number
 *          output files written by a single process.  Behavior
 *          with a shared library may be unpredictable.
 *      (6) For 16 bpp, this displays the full dynamic range with log scale.
 *          Alternative image transforms to generate 8 bpp pix are:
 *             pix8 = pixMaxDynamicRange(pixt, L_LINEAR_SCALE);
 *             pix8 = pixConvert16To8(pixt, 0);  // low order byte
 *             pix8 = pixConvert16To8(pixt, 1);  // high order byte
 * </pre>
 */
l_int32
pixDisplayWrite(PIX     *pixs,
                l_int32  reduction)
{
char            buf[L_BUFSIZE];
char           *fname;
l_float32       scale;
PIX            *pix1, *pix2;
static l_int32  index = 0;  /* caution: not .so or thread safe */

    PROCNAME("pixDisplayWrite");

    if (reduction == 0) return 0;
    if (reduction < 0) {  /* initialize */
        lept_rmdir("lept/display");
        index = 0;
        return 0;
    }
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (index == 0)
        lept_mkdir("lept/display");
    index++;

    if (reduction == 1) {
        pix1 = pixClone(pixs);
    } else {
        scale = 1. / (l_float32)reduction;
        if (pixGetDepth(pixs) == 1)
            pix1 = pixScaleToGray(pixs, scale);
        else
            pix1 = pixScale(pixs, scale, scale);
    }

    if (pixGetDepth(pix1) == 16) {
        pix2 = pixMaxDynamicRange(pix1, L_LOG_SCALE);
        snprintf(buf, L_BUFSIZE, "file.%03d.png", index);
        fname = pathJoin("/tmp/lept/display", buf);
        pixWrite(fname, pix2, IFF_PNG);
        pixDestroy(&pix2);
    } else if (pixGetDepth(pix1) < 8 || pixGetColormap(pix1)) {
        snprintf(buf, L_BUFSIZE, "file.%03d.png", index);
        fname = pathJoin("/tmp/lept/display", buf);
        pixWrite(fname, pix1, IFF_PNG);
    } else {
        snprintf(buf, L_BUFSIZE, "file.%03d.jpg", index);
        fname = pathJoin("/tmp/lept/display", buf);
        pixWrite(fname, pix1, IFF_JFIF_JPEG);
    }
    LEPT_FREE(fname);
    pixDestroy(&pix1);
    return 0;
}
