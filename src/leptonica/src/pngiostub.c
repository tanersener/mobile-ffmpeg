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
 * \file pngiostub.c
 * <pre>
 *
 *     Stubs for pngio.c functions
 * </pre>
 */

#ifdef HAVE_CONFIG_H
#include "config_auto.h"
#endif  /* HAVE_CONFIG_H */

#include "allheaders.h"

/* --------------------------------------------*/
#if  !HAVE_LIBPNG   /* defined in environ.h */
/* --------------------------------------------*/

PIX * pixReadStreamPng(FILE *fp)
{
    return (PIX * )ERROR_PTR("function not present", "pixReadStreamPng", NULL);
}

/* ----------------------------------------------------------------------*/

l_int32 readHeaderPng(const char *filename, l_int32 *pwidth, l_int32 *pheight,
                      l_int32 *pbps, l_int32 *pspp, l_int32 *piscmap)
{
    return ERROR_INT("function not present", "readHeaderPng", 1);
}

/* ----------------------------------------------------------------------*/

l_int32 freadHeaderPng(FILE *fp, l_int32 *pwidth, l_int32 *pheight,
                       l_int32 *pbps, l_int32 *pspp, l_int32 *piscmap)
{
    return ERROR_INT("function not present", "freadHeaderPng", 1);
}

/* ----------------------------------------------------------------------*/

l_int32 readHeaderMemPng(const l_uint8 *data, size_t size, l_int32 *pwidth,
                         l_int32 *pheight, l_int32 *pbps, l_int32 *pspp,
                         l_int32 *piscmap)
{
    return ERROR_INT("function not present", "readHeaderMemPng", 1);
}

/* ----------------------------------------------------------------------*/

l_int32 fgetPngResolution(FILE *fp, l_int32 *pxres, l_int32 *pyres)
{
    return ERROR_INT("function not present", "fgetPngResolution", 1);
}

/* ----------------------------------------------------------------------*/

l_int32 isPngInterlaced(const char *filename, l_int32 *pinterlaced)
{
    return ERROR_INT("function not present", "isPngInterlaced", 1);
}

/* ----------------------------------------------------------------------*/

l_int32 fgetPngColormapInfo(FILE *fp, PIXCMAP **pcmap, l_int32 *ptransparency)
{
    return ERROR_INT("function not present", "fgetPngColormapInfo", 1);
}

/* ----------------------------------------------------------------------*/

l_int32 pixWritePng(const char *filename, PIX *pix, l_float32 gamma)
{
    return ERROR_INT("function not present", "pixWritePng", 1);
}

/* ----------------------------------------------------------------------*/

l_int32 pixWriteStreamPng(FILE *fp, PIX *pix, l_float32 gamma)
{
    return ERROR_INT("function not present", "pixWriteStreamPng", 1);
}

/* ----------------------------------------------------------------------*/

l_int32 pixSetZlibCompression(PIX *pix, l_int32 compval)

{
    return ERROR_INT("function not present", "pixSetZlibCompression", 1);
}

/* ----------------------------------------------------------------------*/

void l_pngSetReadStrip16To8(l_int32 flag)
{
    L_ERROR("function not present\n", "l_pngSetReadStrip16To8");
    return;
}

/* ----------------------------------------------------------------------*/

PIX * pixReadMemPng(const l_uint8 *filedata, size_t filesize)
{
    return (PIX * )ERROR_PTR("function not present", "pixReadMemPng", NULL);
}

/* ----------------------------------------------------------------------*/

l_int32 pixWriteMemPng(l_uint8 **pfiledata, size_t *pfilesize, PIX *pix,
                       l_float32 gamma)
{
    return ERROR_INT("function not present", "pixWriteMemPng", 1);
}

/* --------------------------------------------*/
#endif  /* !HAVE_LIBPNG */
/* --------------------------------------------*/
