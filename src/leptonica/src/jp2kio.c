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
 * \file jp2kio.c
 * <pre>
 *
 *    Read jp2k from file
 *          PIX                *pixReadJp2k()  [special top level]
 *          PIX                *pixReadStreamJp2k()
 *
 *    Write jp2k to file
 *          l_int32             pixWriteJp2k()  [special top level]
 *          l_int32             pixWriteStreamJp2k()
 *          static opj_image_t *pixConvertToOpjImage()
 *
 *    Read/write to memory
 *          PIX                *pixReadMemJp2k()
 *          l_int32             pixWriteMemJp2k()
 *
 *    Static functions from opj 2.0 to retain file stream interface
 *          static opj_stream_t  *opjCreateStream()
 *          [other static helpers]
 *
 *    Based on the OpenJPEG distribution:
 *        http://www.openjpeg.org/
 *    The ISO/IEC reference for jpeg2000 is:
 *        http://www.jpeg.org/public/15444-1annexi.pdf
 *
 *    Compressing to memory and decompressing from memory
 *    ---------------------------------------------------
 *    On systems like windows without fmemopen() and open_memstream(),
 *    we write data to a temp file and read it back for operations
 *    between pix and compressed-data, such as pixReadMemJp2k() and
 *    pixWriteMemJp2k().
 *
 *    Pdf can accept jp2k compressed strings directly
 *    -----------------------------------------------
 *    Transcoding (with the uncompress/compress cycle) is not required
 *    to wrap images that have already been compressed with jp2k in pdf,
 *    because the pdf format for jp2k includes the full string of the
 *    jp2k compressed images.  This is also true for jpeg compressed
 *    strings.
 *
 *    N.B.
 *    * This is based on the most recent openjpeg release: 2.1.
 *    * The openjpeg interface was massively changed from 1.X.  The debian
 *      distribution is way back at 1.3.  We have inquired but are unable
 *      to determine if or when a debian distribution will be built for 2.1.
 *    * For version 2.1, the openjpeg.h file is installed in an
 *      openjpeg-2.1 subdirectory, which is hard to support.
 *    * In openjpeg-2.1, reading is slow compared to jpeg or webp,
 *      and writing is very slow compared to jpeg or webp.  This is expected
 *      to improve significantly in future versions.
 *    * Reading and writing jp2k are supported here for 2.1.
 *      The high-level interface to openjpeg continues to change.
 *      From 2.0 to 2.1, the ability to interface to a C file stream
 *      was removed permanently.  Leptonica supports both file stream
 *      and memory buffer interfaces for every image I/O library, and
 *      it requires the libraries to support at least one of these.
 *      However, openjpeg-2.1 provides neither, so we have brought
 *      several static functions over from openjpeg-2.0 in order to
 *      retain the file stream interface.  See our static function
 *      opjCreateStream().
 *    * Specifying a quality factor for jpeg2000 requires caution.  Unlike
 *      jpeg and webp, which have a sensible scale that goes from 0 (very poor)
 *      to 100 (nearly lossless), kakadu and openjpeg use idiosyncratic and
 *      non-intuitive numbers.  kakadu uses "rate/distortion" numbers in
 *      a narrow range around 50,000; openjpeg (and our write interface)
 *      use SNR.  The visually apparent artifacts introduced by compression
 *      are strongly content-dependent and vary in a highly non-linear
 *      way with SNR.  We take SNR = 34 as default, roughly similar in
 *      quality to jpeg's default standard of 75.  For document images,
 *      SNR = 25 is very poor, whereas SNR = 45 is nearly lossless.  If you
 *      use the latter, you will pay dearly in the size of the compressed file.
 * </pre>
 */

#ifdef HAVE_CONFIG_H
#include "config_auto.h"
#endif  /* HAVE_CONFIG_H */

#include <string.h>
#include "allheaders.h"

/* --------------------------------------------*/
#if  HAVE_LIBJP2K   /* defined in environ.h */
/* --------------------------------------------*/

    /* Leptonica supports versions 2.0 and newer */
#ifdef LIBJP2K_HEADER
#include LIBJP2K_HEADER
#else
#include <openjpeg.h>
#endif

    /* 2.0 didn't define OPJ_VERSION_MINOR. */
#ifndef OPJ_VERSION_MINOR
#define OPJ_VERSION_MINOR 0
#endif

    /* Static generator of opj_stream from file stream.
     * In 2.0.1, this functionality is provided by
     *    opj_stream_create_default_file_stream(),
     * but it was removed in 2.1.0. Because we must have either
     * a file stream or a memory interface to the compressed data,
     * it is necessary to recreate the stream interface here.  */
static opj_stream_t *opjCreateStream(FILE *fp, l_int32 is_read);

    /* Static converter pix --> opj_image.  Used for compressing pix,
     * because the codec works on data stored in their raster format. */
static opj_image_t *pixConvertToOpjImage(PIX *pix);

/*---------------------------------------------------------------------*
 *                        Callback event handlers                      *
 *---------------------------------------------------------------------*/
static void error_callback(const char *msg, void *client_data) {
  (void)client_data;
  fprintf(stdout, "[ERROR] %s", msg);
}

static void warning_callback(const char *msg, void *client_data) {
  (void)client_data;
  fprintf(stdout, "[WARNING] %s", msg);
}

static void info_callback(const char *msg, void *client_data) {
  (void)client_data;
  fprintf(stdout, "[INFO] %s", msg);
}


/*---------------------------------------------------------------------*
 *                 Read jp2k from file (special function)              *
 *---------------------------------------------------------------------*/
/*!
 * \brief   pixReadJp2k()
 *
 * \param[in]    filename
 * \param[in]    reduction scaling factor: 1, 2, 4, 8, 16
 * \param[in]    box  [optional] for extracting a subregion, can be null
 * \param[in]    hint a bitwise OR of L_JP2K_* values; 0 for default
 * \param[in]    debug output callback messages, etc
 * \return  pix 8 or 32 bpp, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) This is a special function for reading jp2k files.
 *          The high-level pixReadStream() uses default values:
 *             %reduction = 1
 *             %box = NULL
 *      (2) This decodes at either full resolution or at a reduction by
 *          a power of 2.  The default value %reduction == 1 gives a full
 *          resolution image.  Use %reduction > 1 to get a reduced image.
 *          The actual values of %reduction that can be used on an image
 *          depend on the number of resolution levels chosen when the
 *          image was compressed.  Typical values might be 1, 2, 4, 8 and 16.
 *          Using a value representing a reduction level that was not
 *          stored when the file was written will fail with the message:
 *          "failed to read the header".
 *      (3) Use %box to decode only a part of the image.  The box is defined
 *          at full resolution.  It is reduced internally by %reduction,
 *          and clipping to the right and bottom of the image is automatic.
 *      (4) We presently only handle images with 8 bits/sample (bps).
 *          If the image has 16 bps, the read will fail.
 *      (5) There are 4 possible values of samples/pixel (spp).
 *          The values in brackets give the pixel values in the Pix:
 *           spp = 1  ==>  grayscale           [8 bpp grayscale]
 *           spp = 2  ==>  grayscale + alpha   [32 bpp rgba]
 *           spp = 3  ==>  rgb                 [32 bpp rgb]
 *           spp = 4  ==>  rgba                [32 bpp rgba]
 *      (6) The %hint parameter is reserved for future use.
 * </pre>
 */
PIX *
pixReadJp2k(const char  *filename,
            l_uint32     reduction,
            BOX         *box,
            l_int32      hint,
            l_int32      debug)
{
FILE     *fp;
PIX      *pix;

    PROCNAME("pixReadJp2k");

    if (!filename)
        return (PIX *)ERROR_PTR("filename not defined", procName, NULL);

    if ((fp = fopenReadStream(filename)) == NULL)
        return (PIX *)ERROR_PTR("image file not found", procName, NULL);
    pix = pixReadStreamJp2k(fp, reduction, box, hint, debug);
    fclose(fp);

    if (!pix)
        return (PIX *)ERROR_PTR("image not returned", procName, NULL);
    return pix;
}


/*!
 * \brief   pixReadStreamJp2k()
 *
 * \param[in]    fp file stream
 * \param[in]    reduction scaling factor: 1, 2, 4, 8
 * \param[in]    box  [optional] for extracting a subregion, can be null
 * \param[in]    hint a bitwise OR of L_JP2K_* values; 0 for default
 * \param[in]    debug output callback messages, etc
 * \return  pix 8 or 32 bpp, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) See pixReadJp2k() for usage.
 * </pre>
 */
PIX *
pixReadStreamJp2k(FILE     *fp,
                  l_uint32  reduction,
                  BOX      *box,
                  l_int32   hint,
                  l_int32   debug)
{
const char        *opjVersion;
l_int32            i, j, index, bx, by, bw, bh, val, rval, gval, bval, aval;
l_int32            w, h, wpl, bps, spp, xres, yres, reduce, prec, colorspace;
l_uint32           pixel;
l_uint32          *data, *line;
opj_dparameters_t  parameters;   /* decompression parameters */
opj_image_t       *image = NULL;
opj_codec_t       *l_codec = NULL;  /* handle to decompressor */
opj_stream_t      *l_stream = NULL;  /* opj stream */
PIX               *pix = NULL;

    PROCNAME("pixReadStreamJp2k");

    if (!fp)
        return (PIX *)ERROR_PTR("fp not defined", procName, NULL);

    opjVersion = opj_version();
    if (opjVersion[0] != '2') {
        L_ERROR("version is %s; must be 2.0 or higher\n", procName, opjVersion);
        return NULL;
    }
    if ((opjVersion[2] - 0x30) != OPJ_VERSION_MINOR) {
        L_ERROR("version %s: differs from minor = %d\n",
                procName, opjVersion, OPJ_VERSION_MINOR);
         return NULL;
     }

        /* Get the resolution and the bits/sample */
    rewind(fp);
    fgetJp2kResolution(fp, &xres, &yres);
    freadHeaderJp2k(fp, NULL, NULL, &bps, NULL);
    rewind(fp);

    if (bps > 8) {
        L_ERROR("found %d bps; can only handle 8 bps\n", procName, bps);
        return NULL;
    }

        /* Set decoding parameters to default values */
    opj_set_default_decoder_parameters(&parameters);

        /* Find and set the reduce parameter, which is log2(reduction).
         * Valid reductions are powers of 2, and are determined when the
         * compressed string is made.  A request for an invalid reduction
         * will cause an error in opj_read_header(), and no image will
         * be returned. */
    for (reduce = 0; (1L << reduce) < reduction; reduce++) { }
    if ((1L << reduce) != reduction) {
        L_ERROR("invalid reduction %d; not power of 2\n", procName, reduction);
        return NULL;
    }
    parameters.cp_reduce = reduce;

        /* Get a decoder handle */
    if ((l_codec = opj_create_decompress(OPJ_CODEC_JP2)) == NULL) {
        L_ERROR("failed to make the codec\n", procName);
        return NULL;
    }

        /* Catch and report events using callbacks */
    if (debug) {
        opj_set_info_handler(l_codec, info_callback, NULL);
        opj_set_warning_handler(l_codec, warning_callback, NULL);
        opj_set_error_handler(l_codec, error_callback, NULL);
    }

        /* Setup the decoding parameters using user parameters */
    if (!opj_setup_decoder(l_codec, &parameters)){
        L_ERROR("failed to set up decoder\n", procName);
        opj_destroy_codec(l_codec);
        return NULL;
    }

        /* Open decompression 'stream'.  In 2.0, we could call this:
         *    opj_stream_create_default_file_stream(fp, 1)
         * but the file stream interface was removed in 2.1. */
    if ((l_stream = opjCreateStream(fp, 1)) == NULL) {
        L_ERROR("failed to open the stream\n", procName);
        opj_destroy_codec(l_codec);
        return NULL;
    }

        /* Read the main header of the codestream and, if necessary,
         * the JP2 boxes */
    if(!opj_read_header(l_stream, l_codec, &image)){
        L_ERROR("failed to read the header\n", procName);
        opj_stream_destroy(l_stream);
        opj_destroy_codec(l_codec);
        opj_image_destroy(image);
        return NULL;
    }

        /* Set up to decode a rectangular region */
    if (box) {
        boxGetGeometry(box, &bx, &by, &bw, &bh);
        if (!opj_set_decode_area(l_codec, image, bx, by,
                                 bx + bw, by + bh)) {
            L_ERROR("failed to set the region for decoding\n", procName);
            opj_stream_destroy(l_stream);
            opj_destroy_codec(l_codec);
            opj_image_destroy(image);
            return NULL;
        }
    }

        /* Get the decoded image */
    if (!(opj_decode(l_codec, l_stream, image) &&
          opj_end_decompress(l_codec, l_stream))) {
        L_ERROR("failed to decode the image\n", procName);
        opj_destroy_codec(l_codec);
        opj_stream_destroy(l_stream);
        opj_image_destroy(image);
        return NULL;
    }

        /* Finished with the byte stream and the codec */
    opj_stream_destroy(l_stream);
    opj_destroy_codec(l_codec);

        /* Get the image parameters */
    spp = image->numcomps;
    w = image->comps[0].w;
    h = image->comps[0].h;
    prec = image->comps[0].prec;
    if (prec != bps)
        L_WARNING("precision %d != bps %d!\n", procName, prec, bps);
    if (debug) {
        L_INFO("w = %d, h = %d, bps = %d, spp = %d\n",
               procName, w, h, bps, spp);
        colorspace = image->color_space;
        if (colorspace == OPJ_CLRSPC_SRGB)
            L_INFO("colorspace is sRGB\n", procName);
        else if (colorspace == OPJ_CLRSPC_GRAY)
            L_INFO("colorspace is grayscale\n", procName);
        else if (colorspace == OPJ_CLRSPC_SYCC)
            L_INFO("colorspace is YUV\n", procName);
    }

        /* Convert the image to a pix */
    if (spp == 1)
        pix = pixCreate(w, h, 8);
    else
        pix = pixCreate(w, h, 32);
    pixSetInputFormat(pix, IFF_JP2);
    pixSetResolution(pix, xres, yres);
    data = pixGetData(pix);
    wpl = pixGetWpl(pix);
    index = 0;
    if (spp == 1) {
        for (i = 0; i < h; i++) {
            line = data + i * wpl;
            for (j = 0; j < w; j++) {
                val = image->comps[0].data[index];
                SET_DATA_BYTE(line, j, val);
                index++;
            }
        }
    } else if (spp == 2) {  /* convert to RGBA */
        for (i = 0; i < h; i++) {
            line = data + i * wpl;
            for (j = 0; j < w; j++) {
                val = image->comps[0].data[index];
                aval = image->comps[1].data[index];
                composeRGBAPixel(val, val, val, aval, &pixel);
                line[j] = pixel;
                index++;
            }
        }
    } else if (spp >= 3) {
        for (i = 0; i < h; i++) {
            line = data + i * wpl;
            for (j = 0; j < w; j++) {
                rval = image->comps[0].data[index];
                gval = image->comps[1].data[index];
                bval = image->comps[2].data[index];
                if (spp == 3) {
                    composeRGBPixel(rval, gval, bval, &pixel);
                } else {  /* spp == 4 */
                    aval = image->comps[3].data[index];
                    composeRGBAPixel(rval, gval, bval, aval, &pixel);
                }
                line[j] = pixel;
                index++;
            }
        }
    }

        /* Free the opj image data structure */
    opj_image_destroy(image);

    return pix;
}


/*---------------------------------------------------------------------*
 *                        Write jp2k to file                           *
 *---------------------------------------------------------------------*/
/*!
 * \brief   pixWriteJp2k()
 *
 * \param[in]    filename
 * \param[in]    pix  any depth, cmap is OK
 * \param[in]    quality SNR > 0; default ~34; 0 for lossless encoding
 * \param[in]    nlevels resolution levels; <= 10; default = 5
 * \param[in]    hint a bitwise OR of L_JP2K_* values; 0 for default
 * \param[in]    debug output callback messages, etc
 * \return  0 if OK; 1 on error
 *
 * <pre>
 * Notes:
 *      (1) The %quality parameter is the SNR.  The useful range is narrow:
 *             SNR < 27  (terrible quality)
 *             SNR = 34  (default; approximately equivalent to jpeg quality 75)
 *             SNR = 40  (very high quality)
 *             SNR = 45  (nearly lossless)
 *          Use 0 for default.
 *      (2) The %nlevels parameter is the number of resolution levels
 *          to be written.  For example, with nlevels == 5, images with
 *          reduction factors of 1, 2, 4, 8 and 16 are encoded, and retrieval
 *          is done at the level requested when reading.  For default,
 *          use either 5 or 0.
 *      (3) The %hint parameter is not yet in use.
 *      (4) For now, we only support 1 "layer" for quality.
 * </pre>
 */
l_int32
pixWriteJp2k(const char  *filename,
             PIX         *pix,
             l_int32      quality,
             l_int32      nlevels,
             l_int32      hint,
             l_int32      debug)
{
FILE  *fp;

    PROCNAME("pixWriteJp2k");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if (!filename)
        return ERROR_INT("filename not defined", procName, 1);

    if ((fp = fopenWriteStream(filename, "wb+")) == NULL)
        return ERROR_INT("stream not opened", procName, 1);

    if (pixWriteStreamJp2k(fp, pix, quality, nlevels, hint, debug)) {
        fclose(fp);
        return ERROR_INT("pix not written to stream", procName, 1);
    }

    fclose(fp);
    return 0;
}


/*!
 * \brief   pixWriteStreamJp2k()
 *
 * \param[in]    fp file stream
 * \param[in]    pix  any depth, cmap is OK
 * \param[in]    quality SNR > 0; default ~34; 0 for lossless encoding
 * \param[in]    nlevels <= 10
 * \param[in]    hint a bitwise OR of L_JP2K_* values; 0 for default
 * \param[in]    debug output callback messages, etc
 * \return  0 if OK, 1 on error
 * <pre>
 * Notes:
 *      (1) See pixWriteJp2k() for usage.
 *      (2) For an encoder with more encoding options, see, e.g.,
 *    https://github.com/OpenJPEG/openjpeg/blob/master/tests/test_tile_encoder.c
 * </pre>
 */
l_int32
pixWriteStreamJp2k(FILE    *fp,
                   PIX     *pix,
                   l_int32  quality,
                   l_int32  nlevels,
                   l_int32  hint,
                   l_int32  debug)
{
l_int32            w, h, d, success, snr;
const char        *opjVersion;
PIX               *pixs;
opj_cparameters_t  parameters;   /* compression parameters */
opj_stream_t      *l_stream = NULL;
opj_codec_t*       l_codec = NULL;;
opj_image_t       *image = NULL;

    PROCNAME("pixWriteStreamJp2k");

    if (!fp)
        return ERROR_INT("stream not open", procName, 1);
    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if (quality < 0)
        return ERROR_INT("quality must be >= 0", procName, 1);
    if (quality > 0 && quality < 27)
        L_WARNING("SNR = %d < 27; very low\n", procName, quality);
    if (quality > 45)
        L_WARNING("SNR = %d > 45; nearly lossless\n", procName, quality);
    snr = (l_float32)quality;

    if (nlevels <= 0) nlevels = 5;  /* default */
    if (nlevels > 10) {
        L_WARNING("nlevels = %d > 10; setting to 10\n", procName, nlevels);
        nlevels = 10;
    }

    opjVersion = opj_version();
    if (opjVersion[0] != '2') {
        L_ERROR("version is %s; must be 2.0 or higher\n", procName, opjVersion);
        return 1;
    }
    if ((opjVersion[2] - 0x30) != OPJ_VERSION_MINOR) {
        L_ERROR("version %s: differs from minor = %d\n",
                procName, opjVersion, OPJ_VERSION_MINOR);
         return 1;
     }

        /* Remove colormap if it exists; result is 8 or 32 bpp */
    pixGetDimensions(pix, &w, &h, &d);
    if (d == 24) {
        pixs = pixConvert24To32(pix);
    } else if (d == 32) {
        pixs = pixClone(pix);
    } else if (pixGetColormap(pix) == NULL) {
        pixs = pixConvertTo8(pix, 0);
    } else {  /* colormap */
        L_INFO("removing colormap; may be better to compress losslessly\n",
               procName);
        pixs = pixRemoveColormap(pix, REMOVE_CMAP_BASED_ON_SRC);
    }

        /* Convert to opj image format. */
    pixSetPadBits(pixs, 0);
    image = pixConvertToOpjImage(pixs);
    pixDestroy(&pixs);

        /* Set encoding parameters to default values.
         * We use one layer with the input SNR. */
    opj_set_default_encoder_parameters(&parameters);
    parameters.cp_fixed_quality = 1;
    parameters.cp_disto_alloc = 0;
    parameters.cp_fixed_alloc =  0;
    parameters.tcp_distoratio[0] = snr;
    parameters.tcp_numlayers = 1;
    parameters.numresolution = nlevels + 1;

        /* Create comment for codestream */
    if (parameters.cp_comment == NULL) {
        const char comment1[] = "Created by Leptonica, version ";
        const char comment2[] = "; using OpenJPEG, version ";
        size_t len1 = strlen(comment1);
        size_t len2 = strlen(comment2);
        char *version1 = getLeptonicaVersion();
        const char *version2 = opj_version();
        len1 += len2 + strlen(version1) + strlen(version2) + 1;
        parameters.cp_comment = (char *)LEPT_MALLOC(len1);
        snprintf(parameters.cp_comment, len1, "%s%s%s%s", comment1, version1,
                 comment2, version2);
        LEPT_FREE(version1);
    }

        /* Get the encoder handle */
    if ((l_codec = opj_create_compress(OPJ_CODEC_JP2)) == NULL) {
        opj_image_destroy(image);
        LEPT_FREE(parameters.cp_comment);
        return ERROR_INT("failed to get the encoder handle\n", procName, 1);
    }

        /* Catch and report events using callbacks */
    if (debug) {
        opj_set_info_handler(l_codec, info_callback, NULL);
        opj_set_warning_handler(l_codec, warning_callback, NULL);
        opj_set_error_handler(l_codec, error_callback, NULL);
    }

        /* Set up the encoder */
    if (!opj_setup_encoder(l_codec, &parameters, image)) {
        opj_destroy_codec(l_codec);
        opj_image_destroy(image);
        LEPT_FREE(parameters.cp_comment);
        return ERROR_INT("failed to set up the encoder\n", procName, 1);
    }

        /* Open a compression stream for writing.  In 2.0 we could use this:
         *     opj_stream_create_default_file_stream(fp, 0)
         * but the file stream interface was removed in 2.1.  */
    rewind(fp);
    if ((l_stream = opjCreateStream(fp, 0)) == NULL) {
        opj_destroy_codec(l_codec);
        opj_image_destroy(image);
        LEPT_FREE(parameters.cp_comment);
        return ERROR_INT("failed to open l_stream\n", procName, 1);
    }

        /* Encode the image */
    if (!opj_start_compress(l_codec, image, l_stream)) {
        opj_stream_destroy(l_stream);
        opj_destroy_codec(l_codec);
        opj_image_destroy(image);
        LEPT_FREE(parameters.cp_comment);
        return ERROR_INT("opj_start_compress failed\n", procName, 1);
    }
    if (!opj_encode(l_codec, l_stream)) {
        opj_stream_destroy(l_stream);
        opj_destroy_codec(l_codec);
        opj_image_destroy(image);
        LEPT_FREE(parameters.cp_comment);
        return ERROR_INT("opj_encode failed\n", procName, 1);
    }
    success = opj_end_compress(l_codec, l_stream);

        /* Clean up */
    opj_stream_destroy(l_stream);
    opj_destroy_codec(l_codec);
    opj_image_destroy(image);
    LEPT_FREE(parameters.cp_comment);
    if (success)
        return 0;
    else
        return ERROR_INT("opj_end_compress failed\n", procName, 1);
}


/*!
 * \brief   pixConvertToOpjImage()
 *
 * \param[in]    pix  8 or 32 bpp
 * \return  opj_image, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) Input pix is 8 bpp grayscale, 32 bpp rgb, or 32 bpp rgba.
 *      (2) Gray + alpha pix are all represented as rgba.
 * </pre>
 */
static opj_image_t *
pixConvertToOpjImage(PIX  *pix)
{
l_int32               i, j, k, w, h, d, spp, wpl;
OPJ_COLOR_SPACE       colorspace;
l_int32              *ir = NULL;
l_int32              *ig = NULL;
l_int32              *ib = NULL;
l_int32              *ia = NULL;
l_uint32             *line, *data;
opj_image_t          *image;
opj_image_cmptparm_t  cmptparm[4];

    PROCNAME("pixConvertToOpjImage");

    if (!pix)
        return (opj_image_t *)ERROR_PTR("pix not defined", procName, NULL);
    pixGetDimensions(pix, &w, &h, &d);
    if (d != 8 && d != 32) {
        L_ERROR("invalid depth: %d\n", procName, d);
        return NULL;
    }

        /* Allocate the opj_image. */
    spp = pixGetSpp(pix);
    memset(&cmptparm[0], 0, 4 * sizeof(opj_image_cmptparm_t));
    for (i = 0; i < spp; i++) {
        cmptparm[i].prec = 8;
        cmptparm[i].bpp = 8;
        cmptparm[i].sgnd = 0;
        cmptparm[i].dx = 1;
        cmptparm[i].dy = 1;
        cmptparm[i].w = w;
        cmptparm[i].h = h;
    }
    colorspace = (spp == 1) ? OPJ_CLRSPC_GRAY : OPJ_CLRSPC_SRGB;
    if ((image = opj_image_create(spp, &cmptparm[0], colorspace)) == NULL)
        return (opj_image_t *)ERROR_PTR("image not made", procName, NULL);
    image->x0 = 0;
    image->y0 = 0;
    image->x1 = w;
    image->y1 = h;

        /* Set the component pointers */
    ir = image->comps[0].data;
    if (spp > 1) {
        ig = image->comps[1].data;
        ib = image->comps[2].data;
    }
    if(spp == 4)
        ia = image->comps[3].data;

        /* Transfer the data from the pix */
    data = pixGetData(pix);
    wpl = pixGetWpl(pix);
    for (i = 0, k = 0; i < h; i++) {
        line = data + i * wpl;
        for (j = 0; j < w; j++, k++) {
            if (spp == 1) {
                ir[k] = GET_DATA_BYTE(line, j);
            } else if (spp > 1) {
                ir[k] = GET_DATA_BYTE(line + j, COLOR_RED);
                ig[k] = GET_DATA_BYTE(line + j, COLOR_GREEN);
                ib[k] = GET_DATA_BYTE(line + j, COLOR_BLUE);
            }
            if (spp == 4)
                ia[k] = GET_DATA_BYTE(line + j, L_ALPHA_CHANNEL);
        }
    }

    return image;
}


/*---------------------------------------------------------------------*
 *                         Read/write to memory                        *
 *---------------------------------------------------------------------*/

/*!
 * \brief   pixReadMemJp2k()
 *
 * \param[in]    data const; jpeg-encoded
 * \param[in]    size of data
 * \param[in]    reduction scaling factor: 1, 2, 4, 8
 * \param[in]    box  [optional] for extracting a subregion, can be null
 * \param[in]    hint a bitwise OR of L_JP2K_* values; 0 for default
 * \param[in]    debug output callback messages, etc
 * \return  pix, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) This crashes when reading through the fmemopen cookie.
 *          Until we can fix this, we use the file-based work-around.
 *          And fixing this may take some time, because the basic
 *          stream interface is no longer supported in openjpeg.
 *      (2) See pixReadJp2k() for usage.
 * </pre>
 */
PIX *
pixReadMemJp2k(const l_uint8  *data,
               size_t          size,
               l_uint32        reduction,
               BOX            *box,
               l_int32         hint,
               l_int32         debug)
{
FILE     *fp;
PIX      *pix;

    PROCNAME("pixReadMemJp2k");

    if (!data)
        return (PIX *)ERROR_PTR("data not defined", procName, NULL);

    if ((fp = fopenReadFromMemory(data, size)) == NULL)
        return (PIX *)ERROR_PTR("stream not opened", procName, NULL);
    pix = pixReadStreamJp2k(fp, reduction, box, hint, debug);
    fclose(fp);
    if (!pix) L_ERROR("pix not read\n", procName);
    return pix;
}


/*!
 * \brief   pixWriteMemJp2k()
 *
 * \param[out]   pdata data of jpeg compressed image
 * \param[out]   psize size of returned data
 * \param[in]    pix 8 or 32 bpp
 * \param[in]    quality SNR > 0; default ~34; 0 for lossless encoding
 * \param[in]    nlevels 0 for default
 * \param[in]    hint a bitwise OR of L_JP2K_* values; 0 for default
 * \param[in]    debug output callback messages, etc
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) See pixWriteJp2k() for usage.  This version writes to
 *          memory instead of to a file stream.
 * </pre>
 */
l_int32
pixWriteMemJp2k(l_uint8  **pdata,
                size_t    *psize,
                PIX       *pix,
                l_int32    quality,
                l_int32    nlevels,
                l_int32    hint,
                l_int32    debug)
{
l_int32  ret;
FILE    *fp;

    PROCNAME("pixWriteMemJp2k");

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
    ret = pixWriteStreamJp2k(fp, pix, quality, nlevels, hint, debug);
#else
    L_INFO("work-around: writing to a temp file\n", procName);
  #ifdef _WIN32
    if ((fp = fopenWriteWinTempfile()) == NULL)
        return ERROR_INT("tmpfile stream not opened", procName, 1);
  #else
    if ((fp = tmpfile()) == NULL)
        return ERROR_INT("tmpfile stream not opened", procName, 1);
  #endif  /* _WIN32 */
    ret = pixWriteStreamJp2k(fp, pix, quality, nlevels, hint, debug);
    rewind(fp);
    *pdata = l_binaryReadStream(fp, psize);
#endif  /* HAVE_FMEMOPEN */
    fclose(fp);
    return ret;
}


/*---------------------------------------------------------------------*
 *    Static functions from opj 2.0 to retain file stream interface    *
 *---------------------------------------------------------------------*/
static l_uint64
opj_get_user_data_length(FILE *fp) {
    OPJ_OFF_T length = 0;
    fseek(fp, 0, SEEK_END);
    length = (OPJ_OFF_T)ftell(fp);
    fseek(fp, 0, SEEK_SET);
    return (l_uint64)length;
}

static OPJ_SIZE_T
opj_read_from_file(void *p_buffer, OPJ_SIZE_T p_nb_bytes, FILE *fp) {
    OPJ_SIZE_T l_nb_read = fread(p_buffer, 1, p_nb_bytes, fp);
    return l_nb_read ? l_nb_read : (OPJ_SIZE_T) - 1;
}

static OPJ_SIZE_T
opj_write_from_file(void *p_buffer, OPJ_SIZE_T p_nb_bytes, FILE *fp)
{
    return fwrite(p_buffer, 1, p_nb_bytes, fp);
}

static OPJ_OFF_T
opj_skip_from_file(OPJ_OFF_T offset, FILE *fp) {
    if (fseek(fp, offset, SEEK_CUR)) {
        return -1;
    }
    return offset;
}

static l_int32
opj_seek_from_file(OPJ_OFF_T offset, FILE *fp) {
    if (fseek(fp, offset, SEEK_SET)) {
        return 0;
    }
    return 1;
}

    /* Static generator of opj_stream from file stream */
static opj_stream_t *
opjCreateStream(FILE    *fp,
                l_int32  is_read_stream)
{
opj_stream_t  *l_stream;

    PROCNAME("opjCreateStream");

    if (!fp)
        return (opj_stream_t *)ERROR_PTR("fp not defined", procName, NULL);

    l_stream = opj_stream_create(OPJ_J2K_STREAM_CHUNK_SIZE, is_read_stream);
    if (!l_stream)
        return (opj_stream_t *)ERROR_PTR("stream not made", procName, NULL);

#if OPJ_VERSION_MINOR == 0
    opj_stream_set_user_data(l_stream, fp);
#else
    opj_stream_set_user_data(l_stream, fp,
                             (opj_stream_free_user_data_fn)NULL);
#endif
    opj_stream_set_user_data_length(l_stream, opj_get_user_data_length(fp));
    opj_stream_set_read_function(l_stream,
                                 (opj_stream_read_fn)opj_read_from_file);
    opj_stream_set_write_function(l_stream,
                                  (opj_stream_write_fn)opj_write_from_file);
    opj_stream_set_skip_function(l_stream,
                                 (opj_stream_skip_fn)opj_skip_from_file);
    opj_stream_set_seek_function(l_stream,
                                 (opj_stream_seek_fn)opj_seek_from_file);

    return l_stream;
}

/* --------------------------------------------*/
#endif  /* HAVE_LIBJP2K */
/* --------------------------------------------*/
