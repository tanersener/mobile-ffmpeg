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
 * \file psio1stub.c
 * <pre>
 *
 *     Stubs for psio1.c functions
 * </pre>
 */

#include "allheaders.h"

/* --------------------------------------------*/
#if  !USE_PSIO   /* defined in environ.h */
/* --------------------------------------------*/

l_int32 convertFilesToPS(const char *dirin, const char *substr,
                         l_int32 res, const char *fileout)
{
    return ERROR_INT("function not present", "convertFilesToPS", 1);
}

/* ----------------------------------------------------------------------*/

l_int32 sarrayConvertFilesToPS(SARRAY *sa, l_int32 res, const char *fileout)
{
    return ERROR_INT("function not present", "sarrayConvertFilesToPS", 1);
}

/* ----------------------------------------------------------------------*/

l_int32 convertFilesFittedToPS(const char *dirin, const char *substr,
                               l_float32 xpts, l_float32 ypts,
                               const char *fileout)
{
    return ERROR_INT("function not present", "convertFilesFittedToPS", 1);
}

/* ----------------------------------------------------------------------*/

l_int32 sarrayConvertFilesFittedToPS(SARRAY *sa, l_float32 xpts,
                                     l_float32 ypts, const char *fileout)
{
    return ERROR_INT("function not present", "sarrayConvertFilesFittedToPS", 1);
}

/* ----------------------------------------------------------------------*/

l_int32 writeImageCompressedToPSFile(const char *filein, const char *fileout,
                                     l_int32 res, l_int32 *pfirstfile,
                                     l_int32 *pindex)
{
    return ERROR_INT("function not present", "writeImageCompressedToPSFile", 1);
}

/* ----------------------------------------------------------------------*/

l_int32 convertSegmentedPagesToPS(const char *pagedir, const char *pagestr,
                                  l_int32 page_numpre, const char *maskdir,
                                  const char *maskstr, l_int32 mask_numpre,
                                  l_int32 numpost, l_int32 maxnum,
                                  l_float32 textscale, l_float32 imagescale,
                                  l_int32 threshold, const char *fileout)
{
    return ERROR_INT("function not present", "convertSegmentedPagesToPS", 1);
}

/* ----------------------------------------------------------------------*/

l_int32 pixWriteSegmentedPageToPS(PIX *pixs, PIX *pixm, l_float32 textscale,
                                  l_float32 imagescale, l_int32 threshold,
                                  l_int32 pageno, const char *fileout)
{
    return ERROR_INT("function not present", "pixWriteSegmentedPagesToPS", 1);
}

/* ----------------------------------------------------------------------*/

l_int32 pixWriteMixedToPS(PIX *pixb, PIX *pixc, l_float32 scale,
                          l_int32 pageno, const char *fileout)
{
    return ERROR_INT("function not present", "pixWriteMixedToPS", 1);
}

/* ----------------------------------------------------------------------*/

l_int32 convertToPSEmbed(const char *filein, const char *fileout, l_int32 level)
{
    return ERROR_INT("function not present", "convertToPSEmbed", 1);
}

/* ----------------------------------------------------------------------*/

l_int32 pixaWriteCompressedToPS(PIXA *pixa, const char *fileout,
                                l_int32 res, l_int32 level)
{
    return ERROR_INT("function not present", "pixaWriteCompressedtoPS", 1);
}

/* --------------------------------------------*/
#endif  /* !USE_PSIO */
/* --------------------------------------------*/
