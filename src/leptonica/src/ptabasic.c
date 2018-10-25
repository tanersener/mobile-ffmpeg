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
 * \file  ptabasic.c
 * <pre>
 *
 *      Pta creation, destruction, copy, clone, empty
 *           PTA            *ptaCreate()
 *           PTA            *ptaCreateFromNuma()
 *           void            ptaDestroy()
 *           PTA            *ptaCopy()
 *           PTA            *ptaCopyRange()
 *           PTA            *ptaClone()
 *           l_int32         ptaEmpty()
 *
 *      Pta array extension
 *           l_int32         ptaAddPt()
 *           static l_int32  ptaExtendArrays()
 *
 *      Pta insertion and removal
 *           l_int32         ptaInsertPt()
 *           l_int32         ptaRemovePt()
 *
 *      Pta accessors
 *           l_int32         ptaGetRefcount()
 *           l_int32         ptaChangeRefcount()
 *           l_int32         ptaGetCount()
 *           l_int32         ptaGetPt()
 *           l_int32         ptaGetIPt()
 *           l_int32         ptaSetPt()
 *           l_int32         ptaGetArrays()
 *
 *      Pta serialized for I/O
 *           PTA            *ptaRead()
 *           PTA            *ptaReadStream()
 *           PTA            *ptaReadMem()
 *           l_int32         ptaWriteDebug()
 *           l_int32         ptaWrite()
 *           l_int32         ptaWriteStream()
 *           l_int32         ptaWriteMem()
 *
 *      Ptaa creation, destruction
 *           PTAA           *ptaaCreate()
 *           void            ptaaDestroy()
 *
 *      Ptaa array extension
 *           l_int32         ptaaAddPta()
 *           static l_int32  ptaaExtendArray()
 *
 *      Ptaa accessors
 *           l_int32         ptaaGetCount()
 *           l_int32         ptaaGetPta()
 *           l_int32         ptaaGetPt()
 *
 *      Ptaa array modifiers
 *           l_int32         ptaaInitFull()
 *           l_int32         ptaaReplacePta()
 *           l_int32         ptaaAddPt()
 *           l_int32         ptaaTruncate()
 *
 *      Ptaa serialized for I/O
 *           PTAA           *ptaaRead()
 *           PTAA           *ptaaReadStream()
 *           PTAA           *ptaaReadMem()
 *           l_int32         ptaaWrite()
 *           l_int32         ptaaWriteStream()
 *           l_int32         ptaaWriteMem()
 * </pre>
 */

#include <string.h>
#include "allheaders.h"

static const l_int32  INITIAL_PTR_ARRAYSIZE = 20;   /* n'import quoi */

    /* Static functions */
static l_int32 ptaExtendArrays(PTA *pta);
static l_int32 ptaaExtendArray(PTAA *ptaa);


/*---------------------------------------------------------------------*
 *                Pta creation, destruction, copy, clone               *
 *---------------------------------------------------------------------*/
/*!
 * \brief   ptaCreate()
 *
 * \param[in]    n  initial array sizes
 * \return  pta, or NULL on error.
 */
PTA *
ptaCreate(l_int32  n)
{
PTA  *pta;

    PROCNAME("ptaCreate");

    if (n <= 0)
        n = INITIAL_PTR_ARRAYSIZE;

    pta = (PTA *)LEPT_CALLOC(1, sizeof(PTA));
    pta->n = 0;
    pta->nalloc = n;
    ptaChangeRefcount(pta, 1);  /* sets to 1 */

    pta->x = (l_float32 *)LEPT_CALLOC(n, sizeof(l_float32));
    pta->y = (l_float32 *)LEPT_CALLOC(n, sizeof(l_float32));
    if (!pta->x || !pta->y) {
        ptaDestroy(&pta);
        return (PTA *)ERROR_PTR("x and y arrays not both made", procName, NULL);
    }

    return pta;
}


/*!
 * \brief   ptaCreateFromNuma()
 *
 * \param[in]    nax [optional] can be null
 * \param[in]    nay
 * \return  pta, or NULL on error.
 */
PTA *
ptaCreateFromNuma(NUMA  *nax,
                  NUMA  *nay)
{
l_int32    i, n;
l_float32  startx, delx, xval, yval;
PTA       *pta;

    PROCNAME("ptaCreateFromNuma");

    if (!nay)
        return (PTA *)ERROR_PTR("nay not defined", procName, NULL);
    n = numaGetCount(nay);
    if (nax && numaGetCount(nax) != n)
        return (PTA *)ERROR_PTR("nax and nay sizes differ", procName, NULL);

    pta = ptaCreate(n);
    numaGetParameters(nay, &startx, &delx);
    for (i = 0; i < n; i++) {
        if (nax)
            numaGetFValue(nax, i, &xval);
        else  /* use implicit x values from nay */
            xval = startx + i * delx;
        numaGetFValue(nay, i, &yval);
        ptaAddPt(pta, xval, yval);
    }

    return pta;
}


/*!
 * \brief   ptaDestroy()
 *
 * \param[in,out]   ppta to be nulled
 * \return  void
 *
 * <pre>
 * Notes:
 *      (1) Decrements the ref count and, if 0, destroys the pta.
 *      (2) Always nulls the input ptr.
 * </pre>
 */
void
ptaDestroy(PTA  **ppta)
{
PTA  *pta;

    PROCNAME("ptaDestroy");

    if (ppta == NULL) {
        L_WARNING("ptr address is NULL!\n", procName);
        return;
    }

    if ((pta = *ppta) == NULL)
        return;

    ptaChangeRefcount(pta, -1);
    if (ptaGetRefcount(pta) <= 0) {
        LEPT_FREE(pta->x);
        LEPT_FREE(pta->y);
        LEPT_FREE(pta);
    }

    *ppta = NULL;
    return;
}


/*!
 * \brief   ptaCopy()
 *
 * \param[in]    pta
 * \return  copy of pta, or NULL on error
 */
PTA *
ptaCopy(PTA  *pta)
{
l_int32    i;
l_float32  x, y;
PTA       *npta;

    PROCNAME("ptaCopy");

    if (!pta)
        return (PTA *)ERROR_PTR("pta not defined", procName, NULL);

    if ((npta = ptaCreate(pta->nalloc)) == NULL)
        return (PTA *)ERROR_PTR("npta not made", procName, NULL);

    for (i = 0; i < pta->n; i++) {
        ptaGetPt(pta, i, &x, &y);
        ptaAddPt(npta, x, y);
    }

    return npta;
}


/*!
 * \brief   ptaCopyRange()
 *
 * \param[in]    ptas
 * \param[in]    istart  starting index in ptas
 * \param[in]    iend  ending index in ptas; use 0 to copy to end
 * \return  0 if OK, 1 on error
 */
PTA *
ptaCopyRange(PTA     *ptas,
             l_int32  istart,
             l_int32  iend)
{
l_int32  n, i, x, y;
PTA     *ptad;

    PROCNAME("ptaCopyRange");

    if (!ptas)
        return (PTA *)ERROR_PTR("ptas not defined", procName, NULL);
    n = ptaGetCount(ptas);
    if (istart < 0)
        istart = 0;
    if (istart >= n)
        return (PTA *)ERROR_PTR("istart out of bounds", procName, NULL);
    if (iend <= 0 || iend >= n)
        iend = n - 1;
    if (istart > iend)
        return (PTA *)ERROR_PTR("istart > iend; no pts", procName, NULL);

    if ((ptad = ptaCreate(iend - istart + 1)) == NULL)
        return (PTA *)ERROR_PTR("ptad not made", procName, NULL);
    for (i = istart; i <= iend; i++) {
        ptaGetIPt(ptas, i, &x, &y);
        ptaAddPt(ptad, x, y);
    }

    return ptad;
}


/*!
 * \brief   ptaClone()
 *
 * \param[in]    pta
 * \return  ptr to same pta, or NULL on error
 */
PTA *
ptaClone(PTA  *pta)
{
    PROCNAME("ptaClone");

    if (!pta)
        return (PTA *)ERROR_PTR("pta not defined", procName, NULL);

    ptaChangeRefcount(pta, 1);
    return pta;
}


/*!
 * \brief   ptaEmpty()
 *
 * \param[in]    pta
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      This only resets the Pta::n field, for reuse
 * </pre>
 */
l_int32
ptaEmpty(PTA  *pta)
{
    PROCNAME("ptaEmpty");

    if (!pta)
        return ERROR_INT("ptad not defined", procName, 1);
    pta->n = 0;
    return 0;
}


/*---------------------------------------------------------------------*
 *                         Pta array extension                         *
 *---------------------------------------------------------------------*/
/*!
 * \brief   ptaAddPt()
 *
 * \param[in]    pta
 * \param[in]    x, y
 * \return  0 if OK, 1 on error
 */
l_int32
ptaAddPt(PTA       *pta,
         l_float32  x,
         l_float32  y)
{
l_int32  n;

    PROCNAME("ptaAddPt");

    if (!pta)
        return ERROR_INT("pta not defined", procName, 1);

    n = pta->n;
    if (n >= pta->nalloc)
        ptaExtendArrays(pta);
    pta->x[n] = x;
    pta->y[n] = y;
    pta->n++;

    return 0;
}


/*!
 * \brief   ptaExtendArrays()
 *
 * \param[in]    pta
 * \return  0 if OK; 1 on error
 */
static l_int32
ptaExtendArrays(PTA  *pta)
{
    PROCNAME("ptaExtendArrays");

    if (!pta)
        return ERROR_INT("pta not defined", procName, 1);

    if ((pta->x = (l_float32 *)reallocNew((void **)&pta->x,
                               sizeof(l_float32) * pta->nalloc,
                               2 * sizeof(l_float32) * pta->nalloc)) == NULL)
        return ERROR_INT("new x array not returned", procName, 1);
    if ((pta->y = (l_float32 *)reallocNew((void **)&pta->y,
                               sizeof(l_float32) * pta->nalloc,
                               2 * sizeof(l_float32) * pta->nalloc)) == NULL)
        return ERROR_INT("new y array not returned", procName, 1);

    pta->nalloc = 2 * pta->nalloc;
    return 0;
}


/*---------------------------------------------------------------------*
 *                     Pta insertion and removal                       *
 *---------------------------------------------------------------------*/
/*!
 * \brief   ptaInsertPt()
 *
 * \param[in]    pta
 * \param[in]    index at which pt is to be inserted
 * \param[in]    x, y point values
 * \return  0 if OK; 1 on error
 */
l_int32
ptaInsertPt(PTA     *pta,
            l_int32  index,
            l_int32  x,
            l_int32  y)
{
l_int32  i, n;

    PROCNAME("ptaInsertPt");

    if (!pta)
        return ERROR_INT("pta not defined", procName, 1);
    n = ptaGetCount(pta);
    if (index < 0 || index > n)
        return ERROR_INT("index not in {0...n}", procName, 1);

    if (n > pta->nalloc)
        ptaExtendArrays(pta);
    pta->n++;
    for (i = n; i > index; i--) {
        pta->x[i] = pta->x[i - 1];
        pta->y[i] = pta->y[i - 1];
    }
    pta->x[index] = x;
    pta->y[index] = y;
    return 0;
}


/*!
 * \brief   ptaRemovePt()
 *
 * \param[in]    pta
 * \param[in]    index of point to be removed
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) This shifts pta[i] --> pta[i - 1] for all i > index.
 *      (2) It should not be used repeatedly on large arrays,
 *          because the function is O(n).
 * </pre>
 */
l_int32
ptaRemovePt(PTA     *pta,
            l_int32  index)
{
l_int32  i, n;

    PROCNAME("ptaRemovePt");

    if (!pta)
        return ERROR_INT("pta not defined", procName, 1);
    n = ptaGetCount(pta);
    if (index < 0 || index >= n)
        return ERROR_INT("index not in {0...n - 1}", procName, 1);

        /* Remove the point */
    for (i = index + 1; i < n; i++) {
        pta->x[i - 1] = pta->x[i];
        pta->y[i - 1] = pta->y[i];
    }
    pta->n--;
    return 0;
}


/*---------------------------------------------------------------------*
 *                           Pta accessors                             *
 *---------------------------------------------------------------------*/
l_int32
ptaGetRefcount(PTA  *pta)
{
    PROCNAME("ptaGetRefcount");

    if (!pta)
        return ERROR_INT("pta not defined", procName, 1);
    return pta->refcount;
}


l_int32
ptaChangeRefcount(PTA     *pta,
                  l_int32  delta)
{
    PROCNAME("ptaChangeRefcount");

    if (!pta)
        return ERROR_INT("pta not defined", procName, 1);
    pta->refcount += delta;
    return 0;
}


/*!
 * \brief   ptaGetCount()
 *
 * \param[in]    pta
 * \return  count, or 0 if no pta
 */
l_int32
ptaGetCount(PTA  *pta)
{
    PROCNAME("ptaGetCount");

    if (!pta)
        return ERROR_INT("pta not defined", procName, 0);

    return pta->n;
}


/*!
 * \brief   ptaGetPt()
 *
 * \param[in]    pta
 * \param[in]    index  into arrays
 * \param[out]   px [optional] float x value
 * \param[out]   py [optional] float y value
 * \return  0 if OK; 1 on error
 */
l_int32
ptaGetPt(PTA        *pta,
         l_int32     index,
         l_float32  *px,
         l_float32  *py)
{
    PROCNAME("ptaGetPt");

    if (px) *px = 0;
    if (py) *py = 0;
    if (!pta)
        return ERROR_INT("pta not defined", procName, 1);
    if (index < 0 || index >= pta->n)
        return ERROR_INT("invalid index", procName, 1);

    if (px) *px = pta->x[index];
    if (py) *py = pta->y[index];
    return 0;
}


/*!
 * \brief   ptaGetIPt()
 *
 * \param[in]    pta
 * \param[in]    index  into arrays
 * \param[out]   px [optional] integer x value
 * \param[out]   py [optional] integer y value
 * \return  0 if OK; 1 on error
 */
l_int32
ptaGetIPt(PTA      *pta,
          l_int32   index,
          l_int32  *px,
          l_int32  *py)
{
    PROCNAME("ptaGetIPt");

    if (px) *px = 0;
    if (py) *py = 0;
    if (!pta)
        return ERROR_INT("pta not defined", procName, 1);
    if (index < 0 || index >= pta->n)
        return ERROR_INT("invalid index", procName, 1);

    if (px) *px = (l_int32)(pta->x[index] + 0.5);
    if (py) *py = (l_int32)(pta->y[index] + 0.5);
    return 0;
}


/*!
 * \brief   ptaSetPt()
 *
 * \param[in]    pta
 * \param[in]    index  into arrays
 * \param[in]    x, y
 * \return  0 if OK; 1 on error
 */
l_int32
ptaSetPt(PTA       *pta,
         l_int32    index,
         l_float32  x,
         l_float32  y)
{
    PROCNAME("ptaSetPt");

    if (!pta)
        return ERROR_INT("pta not defined", procName, 1);
    if (index < 0 || index >= pta->n)
        return ERROR_INT("invalid index", procName, 1);

    pta->x[index] = x;
    pta->y[index] = y;
    return 0;
}


/*!
 * \brief   ptaGetArrays()
 *
 * \param[in]    pta
 * \param[out]   pnax [optional] numa of x array
 * \param[out]   pnay [optional] numa of y array
 * \return  0 if OK; 1 on error or if pta is empty
 *
 * <pre>
 * Notes:
 *      (1) This copies the internal arrays into new Numas.
 * </pre>
 */
l_int32
ptaGetArrays(PTA    *pta,
             NUMA  **pnax,
             NUMA  **pnay)
{
l_int32  i, n;
NUMA    *nax, *nay;

    PROCNAME("ptaGetArrays");

    if (!pnax && !pnay)
        return ERROR_INT("no output requested", procName, 1);
    if (pnax) *pnax = NULL;
    if (pnay) *pnay = NULL;
    if (!pta)
        return ERROR_INT("pta not defined", procName, 1);
    if ((n = ptaGetCount(pta)) == 0)
        return ERROR_INT("pta is empty", procName, 1);

    if (pnax) {
        if ((nax = numaCreate(n)) == NULL)
            return ERROR_INT("nax not made", procName, 1);
        *pnax = nax;
        for (i = 0; i < n; i++)
            nax->array[i] = pta->x[i];
        nax->n = n;
    }
    if (pnay) {
        if ((nay = numaCreate(n)) == NULL)
            return ERROR_INT("nay not made", procName, 1);
        *pnay = nay;
        for (i = 0; i < n; i++)
            nay->array[i] = pta->y[i];
        nay->n = n;
    }
    return 0;
}


/*---------------------------------------------------------------------*
 *                       Pta serialized for I/O                        *
 *---------------------------------------------------------------------*/
/*!
 * \brief   ptaRead()
 *
 * \param[in]    filename
 * \return  pta, or NULL on error
 */
PTA *
ptaRead(const char  *filename)
{
FILE  *fp;
PTA   *pta;

    PROCNAME("ptaRead");

    if (!filename)
        return (PTA *)ERROR_PTR("filename not defined", procName, NULL);

    if ((fp = fopenReadStream(filename)) == NULL)
        return (PTA *)ERROR_PTR("stream not opened", procName, NULL);
    pta = ptaReadStream(fp);
    fclose(fp);
    if (!pta)
        return (PTA *)ERROR_PTR("pta not read", procName, NULL);
    return pta;
}


/*!
 * \brief   ptaReadStream()
 *
 * \param[in]    fp file stream
 * \return  pta, or NULL on error
 */
PTA *
ptaReadStream(FILE  *fp)
{
char       typestr[128];  /* hardcoded below in fscanf */
l_int32    i, n, ix, iy, type, version;
l_float32  x, y;
PTA       *pta;

    PROCNAME("ptaReadStream");

    if (!fp)
        return (PTA *)ERROR_PTR("stream not defined", procName, NULL);

    if (fscanf(fp, "\n Pta Version %d\n", &version) != 1)
        return (PTA *)ERROR_PTR("not a pta file", procName, NULL);
    if (version != PTA_VERSION_NUMBER)
        return (PTA *)ERROR_PTR("invalid pta version", procName, NULL);
    if (fscanf(fp, " Number of pts = %d; format = %127s\n", &n, typestr) != 2)
        return (PTA *)ERROR_PTR("not a pta file", procName, NULL);
    if (!strcmp(typestr, "float"))
        type = 0;
    else  /* typestr is "integer" */
        type = 1;

    if ((pta = ptaCreate(n)) == NULL)
        return (PTA *)ERROR_PTR("pta not made", procName, NULL);
    for (i = 0; i < n; i++) {
        if (type == 0) {  /* data is float */
            if (fscanf(fp, "   (%f, %f)\n", &x, &y) != 2) {
                ptaDestroy(&pta);
                return (PTA *)ERROR_PTR("error reading floats", procName, NULL);
            }
            ptaAddPt(pta, x, y);
        } else {   /* data is integer */
            if (fscanf(fp, "   (%d, %d)\n", &ix, &iy) != 2) {
                ptaDestroy(&pta);
                return (PTA *)ERROR_PTR("error reading ints", procName, NULL);
            }
            ptaAddPt(pta, ix, iy);
        }
    }

    return pta;
}


/*!
 * \brief   ptaReadMem()
 *
 * \param[in]    data  serialization in ascii
 * \param[in]    size  of data in bytes; can use strlen to get it
 * \return  pta, or NULL on error
 */
PTA *
ptaReadMem(const l_uint8  *data,
           size_t          size)
{
FILE  *fp;
PTA   *pta;

    PROCNAME("ptaReadMem");

    if (!data)
        return (PTA *)ERROR_PTR("data not defined", procName, NULL);
    if ((fp = fopenReadFromMemory(data, size)) == NULL)
        return (PTA *)ERROR_PTR("stream not opened", procName, NULL);

    pta = ptaReadStream(fp);
    fclose(fp);
    if (!pta) L_ERROR("pta not read\n", procName);
    return pta;
}


/*!
 * \brief   ptaWriteDebug()
 *
 * \param[in]    filename
 * \param[in]    pta
 * \param[in]    type  0 for float values; 1 for integer values
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) Debug version, intended for use in the library when writing
 *          to files in a temp directory with names that are compiled in.
 *          This is used instead of ptaWrite() for all such library calls.
 *      (2) The global variable LeptDebugOK defaults to 0, and can be set
 *          or cleared by the function setLeptDebugOK().
 * </pre>
 */
l_int32
ptaWriteDebug(const char  *filename,
              PTA         *pta,
              l_int32      type)
{
    PROCNAME("ptaWriteDebug");

    if (LeptDebugOK) {
        return ptaWrite(filename, pta, type);
    } else {
        L_INFO("write to named temp file %s is disabled\n", procName, filename);
        return 0;
    }
}


/*!
 * \brief   ptaWrite()
 *
 * \param[in]    filename
 * \param[in]    pta
 * \param[in]    type  0 for float values; 1 for integer values
 * \return  0 if OK, 1 on error
 */
l_int32
ptaWrite(const char  *filename,
         PTA         *pta,
         l_int32      type)
{
l_int32  ret;
FILE    *fp;

    PROCNAME("ptaWrite");

    if (!filename)
        return ERROR_INT("filename not defined", procName, 1);
    if (!pta)
        return ERROR_INT("pta not defined", procName, 1);

    if ((fp = fopenWriteStream(filename, "w")) == NULL)
        return ERROR_INT("stream not opened", procName, 1);
    ret = ptaWriteStream(fp, pta, type);
    fclose(fp);
    if (ret)
        return ERROR_INT("pta not written to stream", procName, 1);
    return 0;
}


/*!
 * \brief   ptaWriteStream()
 *
 * \param[in]    fp file stream
 * \param[in]    pta
 * \param[in]    type  0 for float values; 1 for integer values
 * \return  0 if OK; 1 on error
 */
l_int32
ptaWriteStream(FILE    *fp,
               PTA     *pta,
               l_int32  type)
{
l_int32    i, n, ix, iy;
l_float32  x, y;

    PROCNAME("ptaWriteStream");

    if (!fp)
        return ERROR_INT("stream not defined", procName, 1);
    if (!pta)
        return ERROR_INT("pta not defined", procName, 1);

    n = ptaGetCount(pta);
    fprintf(fp, "\n Pta Version %d\n", PTA_VERSION_NUMBER);
    if (type == 0)
        fprintf(fp, " Number of pts = %d; format = float\n", n);
    else  /* type == 1 */
        fprintf(fp, " Number of pts = %d; format = integer\n", n);
    for (i = 0; i < n; i++) {
        if (type == 0) {  /* data is float */
            ptaGetPt(pta, i, &x, &y);
            fprintf(fp, "   (%f, %f)\n", x, y);
        } else {   /* data is integer */
            ptaGetIPt(pta, i, &ix, &iy);
            fprintf(fp, "   (%d, %d)\n", ix, iy);
        }
    }

    return 0;
}


/*!
 * \brief   ptaWriteMem()
 *
 * \param[out]   pdata data of serialized pta; ascii
 * \param[out]   psize size of returned data
 * \param[in]    pta
 * \param[in]    type  0 for float values; 1 for integer values
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) Serializes a pta in memory and puts the result in a buffer.
 * </pre>
 */
l_int32
ptaWriteMem(l_uint8  **pdata,
            size_t    *psize,
            PTA       *pta,
            l_int32    type)
{
l_int32  ret;
FILE    *fp;

    PROCNAME("ptaWriteMem");

    if (pdata) *pdata = NULL;
    if (psize) *psize = 0;
    if (!pdata)
        return ERROR_INT("&data not defined", procName, 1);
    if (!psize)
        return ERROR_INT("&size not defined", procName, 1);
    if (!pta)
        return ERROR_INT("pta not defined", procName, 1);

#if HAVE_FMEMOPEN
    if ((fp = open_memstream((char **)pdata, psize)) == NULL)
        return ERROR_INT("stream not opened", procName, 1);
    ret = ptaWriteStream(fp, pta, type);
#else
    L_INFO("work-around: writing to a temp file\n", procName);
  #ifdef _WIN32
    if ((fp = fopenWriteWinTempfile()) == NULL)
        return ERROR_INT("tmpfile stream not opened", procName, 1);
  #else
    if ((fp = tmpfile()) == NULL)
        return ERROR_INT("tmpfile stream not opened", procName, 1);
  #endif  /* _WIN32 */
    ret = ptaWriteStream(fp, pta, type);
    rewind(fp);
    *pdata = l_binaryReadStream(fp, psize);
#endif  /* HAVE_FMEMOPEN */
    fclose(fp);
    return ret;
}


/*---------------------------------------------------------------------*
 *                     PTAA creation, destruction                      *
 *---------------------------------------------------------------------*/
/*!
 * \brief   ptaaCreate()
 *
 * \param[in]    n  initial number of ptrs
 * \return  ptaa, or NULL on error
 */
PTAA *
ptaaCreate(l_int32  n)
{
PTAA  *ptaa;

    PROCNAME("ptaaCreate");

    if (n <= 0)
        n = INITIAL_PTR_ARRAYSIZE;

    if ((ptaa = (PTAA *)LEPT_CALLOC(1, sizeof(PTAA))) == NULL)
        return (PTAA *)ERROR_PTR("ptaa not made", procName, NULL);
    ptaa->n = 0;
    ptaa->nalloc = n;
    if ((ptaa->pta = (PTA **)LEPT_CALLOC(n, sizeof(PTA *))) == NULL) {
        ptaaDestroy(&ptaa);
        return (PTAA *)ERROR_PTR("pta ptrs not made", procName, NULL);
    }
    return ptaa;
}


/*!
 * \brief   ptaaDestroy()
 *
 * \param[in,out]   pptaa to be nulled
 * \return  void
 */
void
ptaaDestroy(PTAA  **pptaa)
{
l_int32  i;
PTAA    *ptaa;

    PROCNAME("ptaaDestroy");

    if (pptaa == NULL) {
        L_WARNING("ptr address is NULL!\n", procName);
        return;
    }

    if ((ptaa = *pptaa) == NULL)
        return;

    for (i = 0; i < ptaa->n; i++)
        ptaDestroy(&ptaa->pta[i]);
    LEPT_FREE(ptaa->pta);

    LEPT_FREE(ptaa);
    *pptaa = NULL;
    return;
}


/*---------------------------------------------------------------------*
 *                          PTAA array extension                       *
 *---------------------------------------------------------------------*/
/*!
 * \brief   ptaaAddPta()
 *
 * \param[in]    ptaa
 * \param[in]    pta  to be added
 * \param[in]    copyflag  L_INSERT, L_COPY, L_CLONE
 * \return  0 if OK, 1 on error
 */
l_int32
ptaaAddPta(PTAA    *ptaa,
           PTA     *pta,
           l_int32  copyflag)
{
l_int32  n;
PTA     *ptac;

    PROCNAME("ptaaAddPta");

    if (!ptaa)
        return ERROR_INT("ptaa not defined", procName, 1);
    if (!pta)
        return ERROR_INT("pta not defined", procName, 1);

    if (copyflag == L_INSERT) {
        ptac = pta;
    } else if (copyflag == L_COPY) {
        if ((ptac = ptaCopy(pta)) == NULL)
            return ERROR_INT("ptac not made", procName, 1);
    } else if (copyflag == L_CLONE) {
        if ((ptac = ptaClone(pta)) == NULL)
            return ERROR_INT("pta clone not made", procName, 1);
    } else {
        return ERROR_INT("invalid copyflag", procName, 1);
    }

    n = ptaaGetCount(ptaa);
    if (n >= ptaa->nalloc)
        ptaaExtendArray(ptaa);
    ptaa->pta[n] = ptac;
    ptaa->n++;

    return 0;
}


/*!
 * \brief   ptaaExtendArray()
 *
 * \param[in]    ptaa
 * \return  0 if OK, 1 on error
 */
static l_int32
ptaaExtendArray(PTAA  *ptaa)
{
    PROCNAME("ptaaExtendArray");

    if (!ptaa)
        return ERROR_INT("ptaa not defined", procName, 1);

    if ((ptaa->pta = (PTA **)reallocNew((void **)&ptaa->pta,
                             sizeof(PTA *) * ptaa->nalloc,
                             2 * sizeof(PTA *) * ptaa->nalloc)) == NULL)
        return ERROR_INT("new ptr array not returned", procName, 1);

    ptaa->nalloc = 2 * ptaa->nalloc;
    return 0;
}


/*---------------------------------------------------------------------*
 *                          Ptaa accessors                             *
 *---------------------------------------------------------------------*/
/*!
 * \brief   ptaaGetCount()
 *
 * \param[in]    ptaa
 * \return  count, or 0 if no ptaa
 */
l_int32
ptaaGetCount(PTAA  *ptaa)
{
    PROCNAME("ptaaGetCount");

    if (!ptaa)
        return ERROR_INT("ptaa not defined", procName, 0);

    return ptaa->n;
}


/*!
 * \brief   ptaaGetPta()
 *
 * \param[in]    ptaa
 * \param[in]    index  to the i-th pta
 * \param[in]    accessflag  L_COPY or L_CLONE
 * \return  pta, or NULL on error
 */
PTA *
ptaaGetPta(PTAA    *ptaa,
           l_int32  index,
           l_int32  accessflag)
{
    PROCNAME("ptaaGetPta");

    if (!ptaa)
        return (PTA *)ERROR_PTR("ptaa not defined", procName, NULL);
    if (index < 0 || index >= ptaa->n)
        return (PTA *)ERROR_PTR("index not valid", procName, NULL);

    if (accessflag == L_COPY)
        return ptaCopy(ptaa->pta[index]);
    else if (accessflag == L_CLONE)
        return ptaClone(ptaa->pta[index]);
    else
        return (PTA *)ERROR_PTR("invalid accessflag", procName, NULL);
}


/*!
 * \brief   ptaaGetPt()
 *
 * \param[in]    ptaa
 * \param[in]    ipta  to the i-th pta
 * \param[in]    jpt index to the j-th pt in the pta
 * \param[out]   px [optional] float x value
 * \param[out]   py [optional] float y value
 * \return  0 if OK; 1 on error
 */
l_int32
ptaaGetPt(PTAA       *ptaa,
           l_int32     ipta,
           l_int32     jpt,
           l_float32  *px,
           l_float32  *py)
{
PTA  *pta;

    PROCNAME("ptaaGetPt");

    if (px) *px = 0;
    if (py) *py = 0;
    if (!ptaa)
        return ERROR_INT("ptaa not defined", procName, 1);
    if (ipta < 0 || ipta >= ptaa->n)
        return ERROR_INT("index ipta not valid", procName, 1);

    pta = ptaaGetPta(ptaa, ipta, L_CLONE);
    if (jpt < 0 || jpt >= pta->n) {
        ptaDestroy(&pta);
        return ERROR_INT("index jpt not valid", procName, 1);
    }

    ptaGetPt(pta, jpt, px, py);
    ptaDestroy(&pta);
    return 0;
}


/*---------------------------------------------------------------------*
 *                        Ptaa array modifiers                         *
 *---------------------------------------------------------------------*/
/*!
 * \brief   ptaaInitFull()
 *
 * \param[in]    ptaa can have non-null ptrs in the ptr array
 * \param[in]    pta to be replicated into the entire ptr array
 * \return  0 if OK; 1 on error
 */
l_int32
ptaaInitFull(PTAA  *ptaa,
             PTA   *pta)
{
l_int32  n, i;
PTA     *ptat;

    PROCNAME("ptaaInitFull");

    if (!ptaa)
        return ERROR_INT("ptaa not defined", procName, 1);
    if (!pta)
        return ERROR_INT("pta not defined", procName, 1);

    n = ptaa->nalloc;
    ptaa->n = n;
    for (i = 0; i < n; i++) {
        ptat = ptaCopy(pta);
        ptaaReplacePta(ptaa, i, ptat);
    }
    return 0;
}


/*!
 * \brief   ptaaReplacePta()
 *
 * \param[in]    ptaa
 * \param[in]    index  to the index-th pta
 * \param[in]    pta insert and replace any existing one
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) Any existing pta is destroyed, and the input one
 *          is inserted in its place.
 *      (2) If the index is invalid, return 1 (error)
 * </pre>
 */
l_int32
ptaaReplacePta(PTAA    *ptaa,
               l_int32  index,
               PTA     *pta)
{
l_int32  n;

    PROCNAME("ptaaReplacePta");

    if (!ptaa)
        return ERROR_INT("ptaa not defined", procName, 1);
    if (!pta)
        return ERROR_INT("pta not defined", procName, 1);
    n = ptaaGetCount(ptaa);
    if (index < 0 || index >= n)
        return ERROR_INT("index not valid", procName, 1);

    ptaDestroy(&ptaa->pta[index]);
    ptaa->pta[index] = pta;
    return 0;
}


/*!
 * \brief   ptaaAddPt()
 *
 * \param[in]    ptaa
 * \param[in]    ipta  to the i-th pta
 * \param[in]    x,y point coordinates
 * \return  0 if OK; 1 on error
 */
l_int32
ptaaAddPt(PTAA      *ptaa,
          l_int32    ipta,
          l_float32  x,
          l_float32  y)
{
PTA  *pta;

    PROCNAME("ptaaAddPt");

    if (!ptaa)
        return ERROR_INT("ptaa not defined", procName, 1);
    if (ipta < 0 || ipta >= ptaa->n)
        return ERROR_INT("index ipta not valid", procName, 1);

    pta = ptaaGetPta(ptaa, ipta, L_CLONE);
    ptaAddPt(pta, x, y);
    ptaDestroy(&pta);
    return 0;
}


/*!
 * \brief   ptaaTruncate()
 *
 * \param[in]    ptaa
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) This identifies the largest index containing a pta that
 *          has any points within it, destroys all pta above that index,
 *          and resets the count.
 * </pre>
 */
l_int32
ptaaTruncate(PTAA  *ptaa)
{
l_int32  i, n, np;
PTA     *pta;

    PROCNAME("ptaaTruncate");

    if (!ptaa)
        return ERROR_INT("ptaa not defined", procName, 1);

    n = ptaaGetCount(ptaa);
    for (i = n - 1; i >= 0; i--) {
        pta = ptaaGetPta(ptaa, i, L_CLONE);
        if (!pta) {
            ptaa->n--;
            continue;
        }
        np = ptaGetCount(pta);
        ptaDestroy(&pta);
        if (np == 0) {
            ptaDestroy(&ptaa->pta[i]);
            ptaa->n--;
        } else {
            break;
        }
    }
    return 0;
}


/*---------------------------------------------------------------------*
 *                       Ptaa serialized for I/O                       *
 *---------------------------------------------------------------------*/
/*!
 * \brief   ptaaRead()
 *
 * \param[in]    filename
 * \return  ptaa, or NULL on error
 */
PTAA *
ptaaRead(const char  *filename)
{
FILE  *fp;
PTAA  *ptaa;

    PROCNAME("ptaaRead");

    if (!filename)
        return (PTAA *)ERROR_PTR("filename not defined", procName, NULL);

    if ((fp = fopenReadStream(filename)) == NULL)
        return (PTAA *)ERROR_PTR("stream not opened", procName, NULL);
    ptaa = ptaaReadStream(fp);
    fclose(fp);
    if (!ptaa)
        return (PTAA *)ERROR_PTR("ptaa not read", procName, NULL);
    return ptaa;
}


/*!
 * \brief   ptaaReadStream()
 *
 * \param[in]    fp file stream
 * \return  ptaa, or NULL on error
 */
PTAA *
ptaaReadStream(FILE  *fp)
{
l_int32  i, n, version;
PTA     *pta;
PTAA    *ptaa;

    PROCNAME("ptaaReadStream");

    if (!fp)
        return (PTAA *)ERROR_PTR("stream not defined", procName, NULL);

    if (fscanf(fp, "\nPtaa Version %d\n", &version) != 1)
        return (PTAA *)ERROR_PTR("not a ptaa file", procName, NULL);
    if (version != PTA_VERSION_NUMBER)
        return (PTAA *)ERROR_PTR("invalid ptaa version", procName, NULL);
    if (fscanf(fp, "Number of Pta = %d\n", &n) != 1)
        return (PTAA *)ERROR_PTR("not a ptaa file", procName, NULL);

    if ((ptaa = ptaaCreate(n)) == NULL)
        return (PTAA *)ERROR_PTR("ptaa not made", procName, NULL);
    for (i = 0; i < n; i++) {
        if ((pta = ptaReadStream(fp)) == NULL) {
            ptaaDestroy(&ptaa);
            return (PTAA *)ERROR_PTR("error reading pta", procName, NULL);
        }
        ptaaAddPta(ptaa, pta, L_INSERT);
    }

    return ptaa;
}


/*!
 * \brief   ptaaReadMem()
 *
 * \param[in]    data  serialization in ascii
 * \param[in]    size  of data in bytes; can use strlen to get it
 * \return  ptaa, or NULL on error
 */
PTAA *
ptaaReadMem(const l_uint8  *data,
            size_t          size)
{
FILE  *fp;
PTAA  *ptaa;

    PROCNAME("ptaaReadMem");

    if (!data)
        return (PTAA *)ERROR_PTR("data not defined", procName, NULL);
    if ((fp = fopenReadFromMemory(data, size)) == NULL)
        return (PTAA *)ERROR_PTR("stream not opened", procName, NULL);

    ptaa = ptaaReadStream(fp);
    fclose(fp);
    if (!ptaa) L_ERROR("ptaa not read\n", procName);
    return ptaa;
}


/*!
 * \brief   ptaaWriteDebug()
 *
 * \param[in]    filename
 * \param[in]    ptaa
 * \param[in]    type  0 for float values; 1 for integer values
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) Debug version, intended for use in the library when writing
 *          to files in a temp directory with names that are compiled in.
 *          This is used instead of ptaaWrite() for all such library calls.
 *      (2) The global variable LeptDebugOK defaults to 0, and can be set
 *          or cleared by the function setLeptDebugOK().
 * </pre>
 */
l_int32
ptaaWriteDebug(const char  *filename,
               PTAA        *ptaa,
               l_int32      type)
{
    PROCNAME("ptaaWriteDebug");

    if (LeptDebugOK) {
        return ptaaWrite(filename, ptaa, type);
    } else {
        L_INFO("write to named temp file %s is disabled\n", procName, filename);
        return 0;
    }
}


/*!
 * \brief   ptaaWrite()
 *
 * \param[in]    filename
 * \param[in]    ptaa
 * \param[in]    type  0 for float values; 1 for integer values
 * \return  0 if OK, 1 on error
 */
l_int32
ptaaWrite(const char  *filename,
          PTAA        *ptaa,
          l_int32      type)
{
l_int32  ret;
FILE    *fp;

    PROCNAME("ptaaWrite");

    if (!filename)
        return ERROR_INT("filename not defined", procName, 1);
    if (!ptaa)
        return ERROR_INT("ptaa not defined", procName, 1);

    if ((fp = fopenWriteStream(filename, "w")) == NULL)
        return ERROR_INT("stream not opened", procName, 1);
    ret = ptaaWriteStream(fp, ptaa, type);
    fclose(fp);
    if (ret)
        return ERROR_INT("ptaa not written to stream", procName, 1);
    return 0;
}


/*!
 * \brief   ptaaWriteStream()
 *
 * \param[in]    fp file stream
 * \param[in]    ptaa
 * \param[in]    type  0 for float values; 1 for integer values
 * \return  0 if OK; 1 on error
 */
l_int32
ptaaWriteStream(FILE    *fp,
                PTAA    *ptaa,
                l_int32  type)
{
l_int32  i, n;
PTA     *pta;

    PROCNAME("ptaaWriteStream");

    if (!fp)
        return ERROR_INT("stream not defined", procName, 1);
    if (!ptaa)
        return ERROR_INT("ptaa not defined", procName, 1);

    n = ptaaGetCount(ptaa);
    fprintf(fp, "\nPtaa Version %d\n", PTA_VERSION_NUMBER);
    fprintf(fp, "Number of Pta = %d\n", n);
    for (i = 0; i < n; i++) {
        pta = ptaaGetPta(ptaa, i, L_CLONE);
        ptaWriteStream(fp, pta, type);
        ptaDestroy(&pta);
    }

    return 0;
}


/*!
 * \brief   ptaaWriteMem()
 *
 * \param[out]   pdata data of serialized ptaa; ascii
 * \param[out]   psize size of returned data
 * \param[in]    ptaa
 * \param[in]    type  0 for float values; 1 for integer values
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) Serializes a ptaa in memory and puts the result in a buffer.
 * </pre>
 */
l_int32
ptaaWriteMem(l_uint8  **pdata,
             size_t    *psize,
             PTAA      *ptaa,
             l_int32    type)
{
l_int32  ret;
FILE    *fp;

    PROCNAME("ptaaWriteMem");

    if (pdata) *pdata = NULL;
    if (psize) *psize = 0;
    if (!pdata)
        return ERROR_INT("&data not defined", procName, 1);
    if (!psize)
        return ERROR_INT("&size not defined", procName, 1);
    if (!ptaa)
        return ERROR_INT("ptaa not defined", procName, 1);

#if HAVE_FMEMOPEN
    if ((fp = open_memstream((char **)pdata, psize)) == NULL)
        return ERROR_INT("stream not opened", procName, 1);
    ret = ptaaWriteStream(fp, ptaa, type);
#else
    L_INFO("work-around: writing to a temp file\n", procName);
  #ifdef _WIN32
    if ((fp = fopenWriteWinTempfile()) == NULL)
        return ERROR_INT("tmpfile stream not opened", procName, 1);
  #else
    if ((fp = tmpfile()) == NULL)
        return ERROR_INT("tmpfile stream not opened", procName, 1);
  #endif  /* _WIN32 */
    ret = ptaaWriteStream(fp, ptaa, type);
    rewind(fp);
    *pdata = l_binaryReadStream(fp, psize);
#endif  /* HAVE_FMEMOPEN */
    fclose(fp);
    return ret;
}

