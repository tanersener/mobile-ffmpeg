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
 * \file psio2stub.c
 * <pre>
 *
 *     Stubs for psio2.c functions
 * </pre>
 */

#include "allheaders.h"

/* --------------------------------------------*/
#if  !USE_PSIO   /* defined in environ.h */
/* --------------------------------------------*/

l_ok pixWritePSEmbed(const char *filein, const char *fileout)
{
    return ERROR_INT("function not present", "pixWritePSEmbed", 1);
}

/* ----------------------------------------------------------------------*/

l_ok pixWriteStreamPS(FILE *fp, PIX *pix, BOX *box, l_int32 res,
                         l_float32 scale)
{
    return ERROR_INT("function not present", "pixWriteStreamPS", 1);
}

/* ----------------------------------------------------------------------*/

char * pixWriteStringPS(PIX *pixs, BOX *box, l_int32 res, l_float32 scale)
{
    return (char *)ERROR_PTR("function not present", "pixWriteStringPS", NULL);
}

/* ----------------------------------------------------------------------*/

char * generateUncompressedPS(char *hexdata, l_int32 w, l_int32 h, l_int32 d,
                              l_int32 psbpl, l_int32 bps, l_float32 xpt,
                              l_float32 ypt, l_float32 wpt, l_float32 hpt,
                              l_int32 boxflag)
{
    return (char *)ERROR_PTR("function not present",
                             "generateUncompressedPS", NULL);
}

/* ----------------------------------------------------------------------*/

void getScaledParametersPS(BOX *box, l_int32 wpix, l_int32 hpix, l_int32 res,
                           l_float32 scale, l_float32 *pxpt, l_float32 *pypt,
                           l_float32 *pwpt, l_float32 *phpt)
{
    L_ERROR("function not present\n", "getScaledParametersPS");
    return;
}

/* ----------------------------------------------------------------------*/

void convertByteToHexAscii(l_uint8 byteval, char *pnib1, char *pnib2)
{
    L_ERROR("function not present\n", "convertByteToHexAscii");
    return;
}

/* ----------------------------------------------------------------------*/

l_ok convertJpegToPSEmbed(const char *filein, const char *fileout)
{
    return ERROR_INT("function not present", "convertJpegToPSEmbed", 1);
}

/* ----------------------------------------------------------------------*/

l_ok convertJpegToPS(const char *filein, const char *fileout,
                     const char *operation, l_int32 x, l_int32 y,
                     l_int32 res, l_float32 scale, l_int32 pageno,
                     l_int32 endpage)
{
    return ERROR_INT("function not present", "convertJpegToPS", 1);
}

/* ----------------------------------------------------------------------*/

l_ok convertJpegToPSString(const char *filein, char **poutstr,
                           l_int32 *pnbytes, l_int32 x, l_int32 y,
                           l_int32 res, l_float32 scale, l_int32 pageno,
                           l_int32 endpage)
{
    return ERROR_INT("function not present", "convertJpegToPSString", 1);
}

/* ----------------------------------------------------------------------*/

char * generateJpegPS(const char *filein, L_COMP_DATA *cid,
                      l_float32 xpt, l_float32 ypt, l_float32 wpt,
                      l_float32 hpt, l_int32 pageno, l_int32 endpage)
{
    return (char *)ERROR_PTR("function not present", "generateJpegPS", NULL);
}

/* ----------------------------------------------------------------------*/

l_ok convertG4ToPSEmbed(const char *filein, const char *fileout)
{
    return ERROR_INT("function not present", "convertG4ToPSEmbed", 1);
}

/* ----------------------------------------------------------------------*/

l_ok convertG4ToPS(const char *filein, const char *fileout,
                   const char *operation, l_int32 x, l_int32 y,
                   l_int32 res, l_float32 scale, l_int32 pageno,
                   l_int32 maskflag, l_int32 endpage)
{
    return ERROR_INT("function not present", "convertG4ToPS", 1);
}

/* ----------------------------------------------------------------------*/

l_ok convertG4ToPSString(const char *filein, char **poutstr,
                         l_int32 *pnbytes, l_int32 x, l_int32 y,
                         l_int32 res, l_float32 scale, l_int32 pageno,
                         l_int32 maskflag, l_int32 endpage)
{
    return ERROR_INT("function not present", "convertG4ToPSString", 1);
}

/* ----------------------------------------------------------------------*/

char * generateG4PS(const char *filein, L_COMP_DATA *cid, l_float32 xpt,
                    l_float32 ypt, l_float32 wpt, l_float32 hpt,
                    l_int32 maskflag, l_int32 pageno, l_int32 endpage)
{
    return (char *)ERROR_PTR("function not present", "generateG4PS", NULL);
}

/* ----------------------------------------------------------------------*/

l_ok convertTiffMultipageToPS(const char *filein, const char *fileout,
                              l_float32 fillfract)
{
    return ERROR_INT("function not present", "convertTiffMultipageToPS", 1);
}

/* ----------------------------------------------------------------------*/

l_ok convertFlateToPSEmbed(const char *filein, const char *fileout)
{
    return ERROR_INT("function not present", "convertFlateToPSEmbed", 1);
}

/* ----------------------------------------------------------------------*/

l_ok convertFlateToPS(const char *filein, const char *fileout,
                      const char *operation, l_int32 x, l_int32 y,
                      l_int32 res, l_float32 scale, l_int32 pageno,
                      l_int32 endpage)
{
    return ERROR_INT("function not present", "convertFlateToPS", 1);
}

/* ----------------------------------------------------------------------*/

l_ok convertFlateToPSString(const char *filein, char **poutstr,
                            l_int32 *pnbytes, l_int32 x, l_int32 y,
                            l_int32 res, l_float32 scale,
                            l_int32 pageno, l_int32 endpage)
{
    return ERROR_INT("function not present", "convertFlateToPSString", 1);
}

/* ----------------------------------------------------------------------*/

char * generateFlatePS(const char *filein, L_COMP_DATA *cid,
                       l_float32 xpt, l_float32 ypt, l_float32 wpt,
                       l_float32 hpt, l_int32 pageno, l_int32 endpage)
{
    return (char *)ERROR_PTR("function not present", "generateFlatePS", NULL);
}

/* ----------------------------------------------------------------------*/

l_ok pixWriteMemPS(l_uint8 **pdata, size_t *psize, PIX *pix, BOX *box,
                   l_int32 res, l_float32 scale)
{
    return ERROR_INT("function not present", "pixWriteMemPS", 1);
}

/* ----------------------------------------------------------------------*/

l_int32 getResLetterPage(l_int32 w, l_int32 h, l_float32 fillfract)
{
    return ERROR_INT("function not present", "getResLetterPage", 1);
}

/* ----------------------------------------------------------------------*/

l_int32 getResA4Page(l_int32 w, l_int32 h, l_float32 fillfract)
{
    return ERROR_INT("function not present", "getResA4Page", 1);
}

/* ----------------------------------------------------------------------*/

void l_psWriteBoundingBox(l_int32 flag)
{
    L_ERROR("function not present\n", "l_psWriteBoundingBox");
    return;
}

/* --------------------------------------------*/
#endif  /* !USE_PSIO */
/* --------------------------------------------*/
