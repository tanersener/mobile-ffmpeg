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
 * \file  numafunc2.c
 * <pre>
 *
 *      --------------------------------------
 *      This file has these Numa utilities:
 *         - morphological operations
 *         - arithmetic transforms
 *         - windowed statistical operations
 *         - histogram extraction
 *         - histogram comparison
 *         - extrema finding
 *         - frequency and crossing analysis
 *      --------------------------------------

 *      Morphological (min/max) operations
 *          NUMA        *numaErode()
 *          NUMA        *numaDilate()
 *          NUMA        *numaOpen()
 *          NUMA        *numaClose()
 *
 *      Other transforms
 *          NUMA        *numaTransform()
 *          l_int32      numaSimpleStats()
 *          l_int32      numaWindowedStats()
 *          NUMA        *numaWindowedMean()
 *          NUMA        *numaWindowedMeanSquare()
 *          l_int32      numaWindowedVariance()
 *          NUMA        *numaWindowedMedian()
 *          NUMA        *numaConvertToInt()
 *
 *      Histogram generation and statistics
 *          NUMA        *numaMakeHistogram()
 *          NUMA        *numaMakeHistogramAuto()
 *          NUMA        *numaMakeHistogramClipped()
 *          NUMA        *numaRebinHistogram()
 *          NUMA        *numaNormalizeHistogram()
 *          l_int32      numaGetStatsUsingHistogram()
 *          l_int32      numaGetHistogramStats()
 *          l_int32      numaGetHistogramStatsOnInterval()
 *          l_int32      numaMakeRankFromHistogram()
 *          l_int32      numaHistogramGetRankFromVal()
 *          l_int32      numaHistogramGetValFromRank()
 *          l_int32      numaDiscretizeRankAndIntensity()
 *          l_int32      numaGetRankBinValues()
 *
 *      Splitting a distribution
 *          l_int32      numaSplitDistribution()
 *
 *      Comparing histograms
 *          l_int32      grayHistogramsToEMD()
 *          l_int32      numaEarthMoverDistance()
 *          l_int32      grayInterHistogramStats()
 *
 *      Extrema finding
 *          NUMA        *numaFindPeaks()
 *          NUMA        *numaFindExtrema()
 *          l_int32     *numaCountReversals()
 *
 *      Threshold crossings and frequency analysis
 *          l_int32      numaSelectCrossingThreshold()
 *          NUMA        *numaCrossingsByThreshold()
 *          NUMA        *numaCrossingsByPeaks()
 *          NUMA        *numaEvalBestHaarParameters()
 *          l_int32      numaEvalHaarSum()
 *
 *      Generating numbers in a range under constraints
 *          NUMA        *genConstrainedNumaInRange()
 *
 *    Things to remember when using the Numa:
 *
 *    (1) The numa is a struct, not an array.  Always use accessors
 *        (see numabasic.c), never the fields directly.
 *
 *    (2) The number array holds l_float32 values.  It can also
 *        be used to store l_int32 values.  See numabasic.c for
 *        details on using the accessors.  Integers larger than
 *        about 10M will lose accuracy due on retrieval due to round-off.
 *        For large integers, use the dna (array of l_float64) instead.
 *
 *    (3) Occasionally, in the comments we denote the i-th element of a
 *        numa by na[i].  This is conceptual only -- the numa is not an array!
 *
 *    Some general comments on histograms:
 *
 *    (1) Histograms are the generic statistical representation of
 *        the data about some attribute.  Typically they're not
 *        normalized -- they simply give the number of occurrences
 *        within each range of values of the attribute.  This range
 *        of values is referred to as a 'bucket'.  For example,
 *        the histogram could specify how many connected components
 *        are found for each value of their width; in that case,
 *        the bucket size is 1.
 *
 *    (2) In leptonica, all buckets have the same size.  Histograms
 *        are therefore specified by a numa of occurrences, along
 *        with two other numbers: the 'value' associated with the
 *        occupants of the first bucket and the size (i.e., 'width')
 *        of each bucket.  These two numbers then allow us to calculate
 *        the value associated with the occupants of each bucket.
 *        These numbers are fields in the numa, initialized to
 *        a startx value of 0.0 and a binsize of 1.0.  Accessors for
 *        these fields are functions numa*Parameters().  All histograms
 *        must have these two numbers properly set.
 * </pre>
 */

#include <math.h>
#include "allheaders.h"

    /* bin sizes in numaMakeHistogram() */
static const l_int32 BinSizeArray[] = {2, 5, 10, 20, 50, 100, 200, 500, 1000,\
                      2000, 5000, 10000, 20000, 50000, 100000, 200000,\
                      500000, 1000000, 2000000, 5000000, 10000000,\
                      200000000, 50000000, 100000000};
static const l_int32 NBinSizes = 24;


#ifndef  NO_CONSOLE_IO
#define  DEBUG_HISTO        0
#define  DEBUG_CROSSINGS    0
#define  DEBUG_FREQUENCY    0
#endif  /* ~NO_CONSOLE_IO */


/*----------------------------------------------------------------------*
 *                     Morphological operations                         *
 *----------------------------------------------------------------------*/
/*!
 * \brief   numaErode()
 *
 * \param[in]    nas
 * \param[in]    size   of sel; greater than 0, odd.  The origin
 *                      is implicitly in the center.
 * \return  nad eroded, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) The structuring element (sel) is linear, all "hits"
 *      (2) If size == 1, this returns a copy
 *      (3) General comment.  The morphological operations are equivalent
 *          to those that would be performed on a 1-dimensional fpix.
 *          However, because we have not implemented morphological
 *          operations on fpix, we do this here.  Because it is only
 *          1 dimensional, there is no reason to use the more
 *          complicated van Herk/Gil-Werman algorithm, and we do it
 *          by brute force.
 * </pre>
 */
NUMA *
numaErode(NUMA    *nas,
          l_int32  size)
{
l_int32     i, j, n, hsize, len;
l_float32   minval;
l_float32  *fa, *fas, *fad;
NUMA       *nad;

    PROCNAME("numaErode");

    if (!nas)
        return (NUMA *)ERROR_PTR("nas not defined", procName, NULL);
    if (size <= 0)
        return (NUMA *)ERROR_PTR("size must be > 0", procName, NULL);
    if ((size & 1) == 0 ) {
        L_WARNING("sel size must be odd; increasing by 1\n", procName);
        size++;
    }

    if (size == 1)
        return numaCopy(nas);

        /* Make a source fa (fas) that has an added (size / 2) boundary
         * on left and right, contains a copy of nas in the interior region
         * (between 'size' and 'size + n', and has large values
         * inserted in the boundary (because it is an erosion). */
    n = numaGetCount(nas);
    hsize = size / 2;
    len = n + 2 * hsize;
    if ((fas = (l_float32 *)LEPT_CALLOC(len, sizeof(l_float32))) == NULL)
        return (NUMA *)ERROR_PTR("fas not made", procName, NULL);
    for (i = 0; i < hsize; i++)
         fas[i] = 1.0e37;
    for (i = hsize + n; i < len; i++)
         fas[i] = 1.0e37;
    fa = numaGetFArray(nas, L_NOCOPY);
    for (i = 0; i < n; i++)
         fas[hsize + i] = fa[i];

    nad = numaMakeConstant(0, n);
    numaCopyParameters(nad, nas);
    fad = numaGetFArray(nad, L_NOCOPY);
    for (i = 0; i < n; i++) {
        minval = 1.0e37;  /* start big */
        for (j = 0; j < size; j++)
            minval = L_MIN(minval, fas[i + j]);
        fad[i] = minval;
    }

    LEPT_FREE(fas);
    return nad;
}


/*!
 * \brief   numaDilate()
 *
 * \param[in]    nas
 * \param[in]    size   of sel; greater than 0, odd.  The origin
 *                      is implicitly in the center.
 * \return  nad dilated, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) The structuring element (sel) is linear, all "hits"
 *      (2) If size == 1, this returns a copy
 * </pre>
 */
NUMA *
numaDilate(NUMA    *nas,
           l_int32  size)
{
l_int32     i, j, n, hsize, len;
l_float32   maxval;
l_float32  *fa, *fas, *fad;
NUMA       *nad;

    PROCNAME("numaDilate");

    if (!nas)
        return (NUMA *)ERROR_PTR("nas not defined", procName, NULL);
    if (size <= 0)
        return (NUMA *)ERROR_PTR("size must be > 0", procName, NULL);
    if ((size & 1) == 0 ) {
        L_WARNING("sel size must be odd; increasing by 1\n", procName);
        size++;
    }

    if (size == 1)
        return numaCopy(nas);

        /* Make a source fa (fas) that has an added (size / 2) boundary
         * on left and right, contains a copy of nas in the interior region
         * (between 'size' and 'size + n', and has small values
         * inserted in the boundary (because it is a dilation). */
    n = numaGetCount(nas);
    hsize = size / 2;
    len = n + 2 * hsize;
    if ((fas = (l_float32 *)LEPT_CALLOC(len, sizeof(l_float32))) == NULL)
        return (NUMA *)ERROR_PTR("fas not made", procName, NULL);
    for (i = 0; i < hsize; i++)
         fas[i] = -1.0e37;
    for (i = hsize + n; i < len; i++)
         fas[i] = -1.0e37;
    fa = numaGetFArray(nas, L_NOCOPY);
    for (i = 0; i < n; i++)
         fas[hsize + i] = fa[i];

    nad = numaMakeConstant(0, n);
    numaCopyParameters(nad, nas);
    fad = numaGetFArray(nad, L_NOCOPY);
    for (i = 0; i < n; i++) {
        maxval = -1.0e37;  /* start small */
        for (j = 0; j < size; j++)
            maxval = L_MAX(maxval, fas[i + j]);
        fad[i] = maxval;
    }

    LEPT_FREE(fas);
    return nad;
}


/*!
 * \brief   numaOpen()
 *
 * \param[in]    nas
 * \param[in]    size   of sel; greater than 0, odd.  The origin
 *                      is implicitly in the center.
 * \return  nad opened, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) The structuring element (sel) is linear, all "hits"
 *      (2) If size == 1, this returns a copy
 * </pre>
 */
NUMA *
numaOpen(NUMA    *nas,
         l_int32  size)
{
NUMA  *nat, *nad;

    PROCNAME("numaOpen");

    if (!nas)
        return (NUMA *)ERROR_PTR("nas not defined", procName, NULL);
    if (size <= 0)
        return (NUMA *)ERROR_PTR("size must be > 0", procName, NULL);
    if ((size & 1) == 0 ) {
        L_WARNING("sel size must be odd; increasing by 1\n", procName);
        size++;
    }

    if (size == 1)
        return numaCopy(nas);

    nat = numaErode(nas, size);
    nad = numaDilate(nat, size);
    numaDestroy(&nat);
    return nad;
}


/*!
 * \brief   numaClose()
 *
 * \param[in]    nas
 * \param[in]    size   of sel; greater than 0, odd.  The origin
 *                      is implicitly in the center.
 * \return  nad  closed, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) The structuring element (sel) is linear, all "hits"
 *      (2) If size == 1, this returns a copy
 *      (3) We add a border before doing this operation, for the same
 *          reason that we add a border to a pix before doing a safe closing.
 *          Without the border, a small component near the border gets
 *          clipped at the border on dilation, and can be entirely removed
 *          by the following erosion, violating the basic extensivity
 *          property of closing.
 * </pre>
 */
NUMA *
numaClose(NUMA    *nas,
          l_int32  size)
{
NUMA  *nab, *nat1, *nat2, *nad;

    PROCNAME("numaClose");

    if (!nas)
        return (NUMA *)ERROR_PTR("nas not defined", procName, NULL);
    if (size <= 0)
        return (NUMA *)ERROR_PTR("size must be > 0", procName, NULL);
    if ((size & 1) == 0 ) {
        L_WARNING("sel size must be odd; increasing by 1\n", procName);
        size++;
    }

    if (size == 1)
        return numaCopy(nas);

    nab = numaAddBorder(nas, size, size, 0);  /* to preserve extensivity */
    nat1 = numaDilate(nab, size);
    nat2 = numaErode(nat1, size);
    nad = numaRemoveBorder(nat2, size, size);
    numaDestroy(&nab);
    numaDestroy(&nat1);
    numaDestroy(&nat2);
    return nad;
}


/*----------------------------------------------------------------------*
 *                            Other transforms                          *
 *----------------------------------------------------------------------*/
/*!
 * \brief   numaTransform()
 *
 * \param[in]    nas
 * \param[in]    shift    add this to each number
 * \param[in]    scale    multiply each number by this
 * \return  nad with all values shifted and scaled, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) Each number is shifted before scaling.
 * </pre>
 */
NUMA *
numaTransform(NUMA      *nas,
              l_float32  shift,
              l_float32  scale)
{
l_int32    i, n;
l_float32  val;
NUMA      *nad;

    PROCNAME("numaTransform");

    if (!nas)
        return (NUMA *)ERROR_PTR("nas not defined", procName, NULL);
    n = numaGetCount(nas);
    if ((nad = numaCreate(n)) == NULL)
        return (NUMA *)ERROR_PTR("nad not made", procName, NULL);
    numaCopyParameters(nad, nas);
    for (i = 0; i < n; i++) {
        numaGetFValue(nas, i, &val);
        val = scale * (val + shift);
        numaAddNumber(nad, val);
    }
    return nad;
}


/*!
 * \brief   numaSimpleStats()
 *
 * \param[in]    na       input numa
 * \param[in]    first    first element to use
 * \param[in]    last     last element to use; -1 to go to the end
 * \param[out]   pmean    [optional] mean value
 * \param[out]   pvar     [optional] variance
 * \param[out]   prvar    [optional] rms deviation from the mean
 * \return  0 if OK, 1 on error
 */
l_ok
numaSimpleStats(NUMA       *na,
                l_int32     first,
                l_int32     last,
                l_float32  *pmean,
                l_float32  *pvar,
                l_float32  *prvar)
{
l_int32    i, n, ni;
l_float32  sum, sumsq, val, mean, var;

    PROCNAME("numaSimpleStats");

    if (pmean) *pmean = 0.0;
    if (pvar) *pvar = 0.0;
    if (prvar) *prvar = 0.0;
    if (!pmean && !pvar && !prvar)
        return ERROR_INT("nothing requested", procName, 1);
    if (!na)
        return ERROR_INT("na not defined", procName, 1);
    if ((n = numaGetCount(na)) == 0)
        return ERROR_INT("na is empty", procName, 1);
    first = L_MAX(0, first);
    if (last < 0) last = n - 1;
    if (first >= n)
        return ERROR_INT("invalid first", procName, 1);
    if (last >= n) {
        L_WARNING("last = %d is beyond max index = %d; adjusting\n",
                  procName, last, n - 1);
        last = n - 1;
    }
    if (first > last)
        return ERROR_INT("first > last\n", procName, 1);
    ni = last - first + 1;
    sum = sumsq = 0.0;
    for (i = first; i <= last; i++) {
        numaGetFValue(na, i, &val);
        sum += val;
        sumsq += val * val;
    }

    mean = sum / ni;
    if (pmean)
        *pmean = mean;
    if (pvar || prvar) {
        var = sumsq / ni - mean * mean;
        if (pvar) *pvar = var;
        if (prvar) *prvar = sqrtf(var);
    }

    return 0;
}


/*!
 * \brief   numaWindowedStats()
 *
 * \param[in]    nas     input numa
 * \param[in]    wc      half width of the window
 * \param[out]   pnam    [optional] mean value in window
 * \param[out]   pnams   [optional] mean square value in window
 * \param[out]   pnav    [optional] variance in window
 * \param[out]   pnarv   [optional] rms deviation from the mean
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) This is a high-level convenience function for calculating
 *          any or all of these derived arrays.
 *      (2) These statistical measures over the values in the
 *          rectangular window are:
 *            ~ average value: [x]  (nam)
 *            ~ average squared value: [x*x] (nams)
 *            ~ variance: [(x - [x])*(x - [x])] = [x*x] - [x]*[x]  (nav)
 *            ~ square-root of variance: (narv)
 *          where the brackets [ .. ] indicate that the average value is
 *          to be taken over the window.
 *      (3) Note that the variance is just the mean square difference from
 *          the mean value; and the square root of the variance is the
 *          root mean square difference from the mean, sometimes also
 *          called the 'standard deviation'.
 *      (4) Internally, use mirrored borders to handle values near the
 *          end of each array.
 * </pre>
 */
l_ok
numaWindowedStats(NUMA    *nas,
                  l_int32  wc,
                  NUMA   **pnam,
                  NUMA   **pnams,
                  NUMA   **pnav,
                  NUMA   **pnarv)
{
NUMA  *nam, *nams;

    PROCNAME("numaWindowedStats");

    if (!nas)
        return ERROR_INT("nas not defined", procName, 1);
    if (2 * wc + 1 > numaGetCount(nas))
        L_WARNING("filter wider than input array!\n", procName);

    if (!pnav && !pnarv) {
        if (pnam) *pnam = numaWindowedMean(nas, wc);
        if (pnams) *pnams = numaWindowedMeanSquare(nas, wc);
        return 0;
    }

    nam = numaWindowedMean(nas, wc);
    nams = numaWindowedMeanSquare(nas, wc);
    numaWindowedVariance(nam, nams, pnav, pnarv);
    if (pnam)
        *pnam = nam;
    else
        numaDestroy(&nam);
    if (pnams)
        *pnams = nams;
    else
        numaDestroy(&nams);
    return 0;
}


/*!
 * \brief   numaWindowedMean()
 *
 * \param[in]    nas
 * \param[in]    wc    half width of the convolution window
 * \return  nad after low-pass filtering, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) This is a convolution.  The window has width = 2 * %wc + 1.
 *      (2) We add a mirrored border of size %wc to each end of the array.
 * </pre>
 */
NUMA *
numaWindowedMean(NUMA    *nas,
                 l_int32  wc)
{
l_int32     i, n, n1, width;
l_float32   sum, norm;
l_float32  *fa1, *fad, *suma;
NUMA       *na1, *nad;

    PROCNAME("numaWindowedMean");

    if (!nas)
        return (NUMA *)ERROR_PTR("nas not defined", procName, NULL);
    n = numaGetCount(nas);
    width = 2 * wc + 1;  /* filter width */
    if (width > n)
        L_WARNING("filter wider than input array!\n", procName);

    na1 = numaAddSpecifiedBorder(nas, wc, wc, L_MIRRORED_BORDER);
    n1 = n + 2 * wc;
    fa1 = numaGetFArray(na1, L_NOCOPY);
    nad = numaMakeConstant(0, n);
    fad = numaGetFArray(nad, L_NOCOPY);

        /* Make sum array; note the indexing */
    if ((suma = (l_float32 *)LEPT_CALLOC(n1 + 1, sizeof(l_float32))) == NULL) {
        numaDestroy(&na1);
        numaDestroy(&nad);
        return (NUMA *)ERROR_PTR("suma not made", procName, NULL);
    }
    sum = 0.0;
    suma[0] = 0.0;
    for (i = 0; i < n1; i++) {
        sum += fa1[i];
        suma[i + 1] = sum;
    }

    norm = 1. / (2 * wc + 1);
    for (i = 0; i < n; i++)
        fad[i] = norm * (suma[width + i] - suma[i]);

    LEPT_FREE(suma);
    numaDestroy(&na1);
    return nad;
}


/*!
 * \brief   numaWindowedMeanSquare()
 *
 * \param[in]    nas
 * \param[in]    wc    half width of the window
 * \return  nad containing windowed mean square values, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) The window has width = 2 * %wc + 1.
 *      (2) We add a mirrored border of size %wc to each end of the array.
 * </pre>
 */
NUMA *
numaWindowedMeanSquare(NUMA    *nas,
                       l_int32  wc)
{
l_int32     i, n, n1, width;
l_float32   sum, norm;
l_float32  *fa1, *fad, *suma;
NUMA       *na1, *nad;

    PROCNAME("numaWindowedMeanSquare");

    if (!nas)
        return (NUMA *)ERROR_PTR("nas not defined", procName, NULL);
    n = numaGetCount(nas);
    width = 2 * wc + 1;  /* filter width */
    if (width > n)
        L_WARNING("filter wider than input array!\n", procName);

    na1 = numaAddSpecifiedBorder(nas, wc, wc, L_MIRRORED_BORDER);
    n1 = n + 2 * wc;
    fa1 = numaGetFArray(na1, L_NOCOPY);
    nad = numaMakeConstant(0, n);
    fad = numaGetFArray(nad, L_NOCOPY);

        /* Make sum array; note the indexing */
    if ((suma = (l_float32 *)LEPT_CALLOC(n1 + 1, sizeof(l_float32))) == NULL) {
        numaDestroy(&na1);
        numaDestroy(&nad);
        return (NUMA *)ERROR_PTR("suma not made", procName, NULL);
    }
    sum = 0.0;
    suma[0] = 0.0;
    for (i = 0; i < n1; i++) {
        sum += fa1[i] * fa1[i];
        suma[i + 1] = sum;
    }

    norm = 1. / (2 * wc + 1);
    for (i = 0; i < n; i++)
        fad[i] = norm * (suma[width + i] - suma[i]);

    LEPT_FREE(suma);
    numaDestroy(&na1);
    return nad;
}


/*!
 * \brief   numaWindowedVariance()
 *
 * \param[in]    nam    windowed mean values
 * \param[in]    nams   windowed mean square values
 * \param[out]   pnav   [optional] numa of variance -- the ms deviation
 *                      from the mean
 * \param[out]   pnarv  [optional] numa of rms deviation from the mean
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) The numas of windowed mean and mean square are precomputed,
 *          using numaWindowedMean() and numaWindowedMeanSquare().
 *      (2) Either or both of the variance and square-root of variance
 *          are returned, where the variance is the average over the
 *          window of the mean square difference of the pixel value
 *          from the mean:
 *                [(x - [x])*(x - [x])] = [x*x] - [x]*[x]
 * </pre>
 */
l_ok
numaWindowedVariance(NUMA   *nam,
                     NUMA   *nams,
                     NUMA  **pnav,
                     NUMA  **pnarv)
{
l_int32     i, nm, nms;
l_float32   var;
l_float32  *fam, *fams, *fav, *farv;
NUMA       *nav, *narv;  /* variance and square root of variance */

    PROCNAME("numaWindowedVariance");

    if (pnav) *pnav = NULL;
    if (pnarv) *pnarv = NULL;
    if (!pnav && !pnarv)
        return ERROR_INT("neither &nav nor &narv are defined", procName, 1);
    if (!nam)
        return ERROR_INT("nam not defined", procName, 1);
    if (!nams)
        return ERROR_INT("nams not defined", procName, 1);
    nm = numaGetCount(nam);
    nms = numaGetCount(nams);
    if (nm != nms)
        return ERROR_INT("sizes of nam and nams differ", procName, 1);

    if (pnav) {
        nav = numaMakeConstant(0, nm);
        *pnav = nav;
        fav = numaGetFArray(nav, L_NOCOPY);
    }
    if (pnarv) {
        narv = numaMakeConstant(0, nm);
        *pnarv = narv;
        farv = numaGetFArray(narv, L_NOCOPY);
    }
    fam = numaGetFArray(nam, L_NOCOPY);
    fams = numaGetFArray(nams, L_NOCOPY);

    for (i = 0; i < nm; i++) {
        var = fams[i] - fam[i] * fam[i];
        if (pnav)
            fav[i] = var;
        if (pnarv)
            farv[i] = sqrtf(var);
    }

    return 0;
}


/*!
 * \brief   numaWindowedMedian()
 *
 * \param[in]    nas
 * \param[in]    halfwin   half width of window over which the median is found
 * \return  nad after windowed median filtering, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) The requested window has width = 2 * %halfwin + 1.
 *      (2) If the input nas has less then 3 elements, return a copy.
 *      (3) If the filter is too small (%halfwin <= 0), return a copy.
 *      (4) If the filter is too large, it is reduced in size.
 *      (5) We add a mirrored border of size %halfwin to each end of
 *          the array to simplify the calculation by avoiding end-effects.
 * </pre>
 */
NUMA *
numaWindowedMedian(NUMA    *nas,
                   l_int32  halfwin)
{
l_int32    i, n;
l_float32  medval;
NUMA      *na1, *na2, *nad;

    PROCNAME("numaWindowedMedian");

    if (!nas)
        return (NUMA *)ERROR_PTR("nas not defined", procName, NULL);
    if ((n = numaGetCount(nas)) < 3)
        return numaCopy(nas);
    if (halfwin <= 0) {
        L_ERROR("filter too small; returning a copy\n", procName);
        return numaCopy(nas);
    }

    if (halfwin > (n - 1) / 2) {
        halfwin = (n - 1) / 2;
        L_INFO("reducing filter to halfwin = %d\n", procName, halfwin);
    }

        /* Add a border to both ends */
    na1 = numaAddSpecifiedBorder(nas, halfwin, halfwin, L_MIRRORED_BORDER);

        /* Get the median value at the center of each window, corresponding
         * to locations in the input nas. */
    nad = numaCreate(n);
    for (i = 0; i < n; i++) {
        na2 = numaClipToInterval(na1, i, i + 2 * halfwin);
        numaGetMedian(na2, &medval);
        numaAddNumber(nad, medval);
        numaDestroy(&na2);
    }

    numaDestroy(&na1);
    return nad;
}


/*!
 * \brief   numaConvertToInt()
 *
 * \param[in]    nas   source numa
 * \return  na with all values rounded to nearest integer, or
 *              NULL on error
 */
NUMA *
numaConvertToInt(NUMA  *nas)
{
l_int32  i, n, ival;
NUMA    *nad;

    PROCNAME("numaConvertToInt");

    if (!nas)
        return (NUMA *)ERROR_PTR("nas not defined", procName, NULL);

    n = numaGetCount(nas);
    if ((nad = numaCreate(n)) == NULL)
        return (NUMA *)ERROR_PTR("nad not made", procName, NULL);
    numaCopyParameters(nad, nas);
    for (i = 0; i < n; i++) {
        numaGetIValue(nas, i, &ival);
        numaAddNumber(nad, ival);
    }
    return nad;
}


/*----------------------------------------------------------------------*
 *                 Histogram generation and statistics                  *
 *----------------------------------------------------------------------*/
/*!
 * \brief   numaMakeHistogram()
 *
 * \param[in]    na
 * \param[in]    maxbins    max number of histogram bins
 * \param[out]   pbinsize   size of histogram bins
 * \param[out]   pbinstart  [optional] start val of minimum bin;
 *                          input NULL to force start at 0
 * \return  na consisiting of histogram of integerized values,
 *              or NULL on error.
 *
 * <pre>
 * Notes:
 *      (1) This simple interface is designed for integer data.
 *          The bins are of integer width and start on integer boundaries,
 *          so the results on float data will not have high precision.
 *      (2) Specify the max number of input bins.   Then %binsize,
 *          the size of bins necessary to accommodate the input data,
 *          is returned.  It is one of the sequence:
 *                {1, 2, 5, 10, 20, 50, ...}.
 *      (3) If &binstart is given, all values are accommodated,
 *          and the min value of the starting bin is returned.
 *          Otherwise, all negative values are discarded and
 *          the histogram bins start at 0.
 * </pre>
 */
NUMA *
numaMakeHistogram(NUMA     *na,
                  l_int32   maxbins,
                  l_int32  *pbinsize,
                  l_int32  *pbinstart)
{
l_int32    i, n, ival, hval;
l_int32    iminval, imaxval, range, binsize, nbins, ibin;
l_float32  val, ratio;
NUMA      *nai, *nahist;

    PROCNAME("numaMakeHistogram");

    if (!na)
        return (NUMA *)ERROR_PTR("na not defined", procName, NULL);
    if (!pbinsize)
        return (NUMA *)ERROR_PTR("&binsize not defined", procName, NULL);

        /* Determine input range */
    numaGetMin(na, &val, NULL);
    iminval = (l_int32)(val + 0.5);
    numaGetMax(na, &val, NULL);
    imaxval = (l_int32)(val + 0.5);
    if (pbinstart == NULL) {  /* clip negative vals; start from 0 */
        iminval = 0;
        if (imaxval < 0)
            return (NUMA *)ERROR_PTR("all values < 0", procName, NULL);
    }

        /* Determine binsize */
    range = imaxval - iminval + 1;
    if (range > maxbins - 1) {
        ratio = (l_float64)range / (l_float64)maxbins;
        binsize = 0;
        for (i = 0; i < NBinSizes; i++) {
            if (ratio < BinSizeArray[i]) {
                binsize = BinSizeArray[i];
                break;
            }
        }
        if (binsize == 0)
            return (NUMA *)ERROR_PTR("numbers too large", procName, NULL);
    } else {
        binsize = 1;
    }
    *pbinsize = binsize;
    nbins = 1 + range / binsize;  /* +1 seems to be sufficient */

        /* Redetermine iminval */
    if (pbinstart && binsize > 1) {
        if (iminval >= 0)
            iminval = binsize * (iminval / binsize);
        else
            iminval = binsize * ((iminval - binsize + 1) / binsize);
    }
    if (pbinstart)
        *pbinstart = iminval;

#if  DEBUG_HISTO
    fprintf(stderr, " imaxval = %d, range = %d, nbins = %d\n",
            imaxval, range, nbins);
#endif  /* DEBUG_HISTO */

        /* Use integerized data for input */
    if ((nai = numaConvertToInt(na)) == NULL)
        return (NUMA *)ERROR_PTR("nai not made", procName, NULL);
    n = numaGetCount(nai);

        /* Make histogram, converting value in input array
         * into a bin number for this histogram array. */
    if ((nahist = numaCreate(nbins)) == NULL) {
        numaDestroy(&nai);
        return (NUMA *)ERROR_PTR("nahist not made", procName, NULL);
    }
    numaSetCount(nahist, nbins);
    numaSetParameters(nahist, iminval, binsize);
    for (i = 0; i < n; i++) {
        numaGetIValue(nai, i, &ival);
        ibin = (ival - iminval) / binsize;
        if (ibin >= 0 && ibin < nbins) {
            numaGetIValue(nahist, ibin, &hval);
            numaSetValue(nahist, ibin, hval + 1.0);
        }
    }

    numaDestroy(&nai);
    return nahist;
}


/*!
 * \brief   numaMakeHistogramAuto()
 *
 * \param[in]    na       numa of floats; these may be integers
 * \param[in]    maxbins  max number of histogram bins; >= 1
 * \return  na consisiting of histogram of quantized float values,
 *              or NULL on error.
 *
 * <pre>
 * Notes:
 *      (1) This simple interface is designed for accurate binning
 *          of both integer and float data.
 *      (2) If the array data is integers, and the range of integers
 *          is smaller than %maxbins, they are binned as they fall,
 *          with binsize = 1.
 *      (3) If the range of data, (maxval - minval), is larger than
 *          %maxbins, or if the data is floats, they are binned into
 *          exactly %maxbins bins.
 *      (4) Unlike numaMakeHistogram(), these bins in general have
 *          non-integer location and width, even for integer data.
 * </pre>
 */
NUMA *
numaMakeHistogramAuto(NUMA    *na,
                      l_int32  maxbins)
{
l_int32    i, n, imin, imax, irange, ibin, ival, allints;
l_float32  minval, maxval, range, binsize, fval;
NUMA      *nah;

    PROCNAME("numaMakeHistogramAuto");

    if (!na)
        return (NUMA *)ERROR_PTR("na not defined", procName, NULL);
    maxbins = L_MAX(1, maxbins);

        /* Determine input range */
    numaGetMin(na, &minval, NULL);
    numaGetMax(na, &maxval, NULL);

        /* Determine if values are all integers */
    n = numaGetCount(na);
    numaHasOnlyIntegers(na, maxbins, &allints);

        /* Do simple integer binning if possible */
    if (allints && (maxval - minval < maxbins)) {
        imin = (l_int32)minval;
        imax = (l_int32)maxval;
        irange = imax - imin + 1;
        nah = numaCreate(irange);
        numaSetCount(nah, irange);  /* init */
        numaSetParameters(nah, minval, 1.0);
        for (i = 0; i < n; i++) {
            numaGetIValue(na, i, &ival);
            ibin = ival - imin;
            numaGetIValue(nah, ibin, &ival);
            numaSetValue(nah, ibin, ival + 1.0);
        }

        return nah;
    }

        /* Do float binning, even if the data is integers. */
    range = maxval - minval;
    binsize = range / (l_float32)maxbins;
    if (range == 0.0) {
        nah = numaCreate(1);
        numaSetParameters(nah, minval, binsize);
        numaAddNumber(nah, n);
        return nah;
    }
    nah = numaCreate(maxbins);
    numaSetCount(nah, maxbins);
    numaSetParameters(nah, minval, binsize);
    for (i = 0; i < n; i++) {
        numaGetFValue(na, i, &fval);
        ibin = (l_int32)((fval - minval) / binsize);
        ibin = L_MIN(ibin, maxbins - 1);  /* "edge" case; stay in bounds */
        numaGetIValue(nah, ibin, &ival);
        numaSetValue(nah, ibin, ival + 1.0);
    }

    return nah;
}


/*!
 * \brief   numaMakeHistogramClipped()
 *
 * \param[in]    na
 * \param[in]    binsize    typically 1.0
 * \param[in]    maxsize    of histogram ordinate
 * \return  na histogram of bins of size %binsize, starting with
 *                  the na[0] (x = 0.0 and going up to a maximum of
 *                  x = %maxsize, by increments of %binsize), or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) This simple function generates a histogram of values
 *          from na, discarding all values < 0.0 or greater than
 *          min(%maxsize, maxval), where maxval is the maximum value in na.
 *          The histogram data is put in bins of size delx = %binsize,
 *          starting at x = 0.0.  We use as many bins as are
 *          needed to hold the data.
 * </pre>
 */
NUMA *
numaMakeHistogramClipped(NUMA      *na,
                         l_float32  binsize,
                         l_float32  maxsize)
{
l_int32    i, n, nbins, ival, ibin;
l_float32  val, maxval;
NUMA      *nad;

    PROCNAME("numaMakeHistogramClipped");

    if (!na)
        return (NUMA *)ERROR_PTR("na not defined", procName, NULL);
    if (binsize <= 0.0)
        return (NUMA *)ERROR_PTR("binsize must be > 0.0", procName, NULL);
    if (binsize > maxsize)
        binsize = maxsize;  /* just one bin */

    numaGetMax(na, &maxval, NULL);
    n = numaGetCount(na);
    maxsize = L_MIN(maxsize, maxval);
    nbins = (l_int32)(maxsize / binsize) + 1;

/*    fprintf(stderr, "maxsize = %7.3f, nbins = %d\n", maxsize, nbins); */

    if ((nad = numaCreate(nbins)) == NULL)
        return (NUMA *)ERROR_PTR("nad not made", procName, NULL);
    numaSetParameters(nad, 0.0, binsize);
    numaSetCount(nad, nbins);  /* interpret zeroes in bins as data */
    for (i = 0; i < n; i++) {
        numaGetFValue(na, i, &val);
        ibin = (l_int32)(val / binsize);
        if (ibin >= 0 && ibin < nbins) {
            numaGetIValue(nad, ibin, &ival);
            numaSetValue(nad, ibin, ival + 1.0);
        }
    }

    return nad;
}


/*!
 * \brief   numaRebinHistogram()
 *
 * \param[in]    nas      input histogram
 * \param[in]    newsize  number of old bins contained in each new bin
 * \return  nad more coarsely re-binned histogram, or NULL on error
 */
NUMA *
numaRebinHistogram(NUMA    *nas,
                   l_int32  newsize)
{
l_int32    i, j, ns, nd, index, count, val;
l_float32  start, oldsize;
NUMA      *nad;

    PROCNAME("numaRebinHistogram");

    if (!nas)
        return (NUMA *)ERROR_PTR("nas not defined", procName, NULL);
    if (newsize <= 1)
        return (NUMA *)ERROR_PTR("newsize must be > 1", procName, NULL);
    if ((ns = numaGetCount(nas)) == 0)
        return (NUMA *)ERROR_PTR("no bins in nas", procName, NULL);

    nd = (ns + newsize - 1) / newsize;
    if ((nad = numaCreate(nd)) == NULL)
        return (NUMA *)ERROR_PTR("nad not made", procName, NULL);
    numaGetParameters(nad, &start, &oldsize);
    numaSetParameters(nad, start, oldsize * newsize);

    for (i = 0; i < nd; i++) {  /* new bins */
        count = 0;
        index = i * newsize;
        for (j = 0; j < newsize; j++) {
            if (index < ns) {
                numaGetIValue(nas, index, &val);
                count += val;
                index++;
            }
        }
        numaAddNumber(nad, count);
    }

    return nad;
}


/*!
 * \brief   numaNormalizeHistogram()
 *
 * \param[in]    nas   input histogram
 * \param[in]    tsum  target sum of all numbers in dest histogram; e.g., use
 *                     %tsum= 1.0 if this represents a probability distribution
 * \return  nad normalized histogram, or NULL on error
 */
NUMA *
numaNormalizeHistogram(NUMA      *nas,
                       l_float32  tsum)
{
l_int32    i, ns;
l_float32  sum, factor, fval;
NUMA      *nad;

    PROCNAME("numaNormalizeHistogram");

    if (!nas)
        return (NUMA *)ERROR_PTR("nas not defined", procName, NULL);
    if (tsum <= 0.0)
        return (NUMA *)ERROR_PTR("tsum must be > 0.0", procName, NULL);
    if ((ns = numaGetCount(nas)) == 0)
        return (NUMA *)ERROR_PTR("no bins in nas", procName, NULL);

    numaGetSum(nas, &sum);
    factor = tsum / sum;

    if ((nad = numaCreate(ns)) == NULL)
        return (NUMA *)ERROR_PTR("nad not made", procName, NULL);
    numaCopyParameters(nad, nas);

    for (i = 0; i < ns; i++) {
        numaGetFValue(nas, i, &fval);
        fval *= factor;
        numaAddNumber(nad, fval);
    }

    return nad;
}


/*!
 * \brief   numaGetStatsUsingHistogram()
 *
 * \param[in]    na        an arbitrary set of numbers; not ordered and not
 *                         a histogram
 * \param[in]    maxbins   the maximum number of bins to be allowed in
 *                         the histogram; use an integer larger than the
 *                         largest number in %na for consecutive integer bins
 * \param[out]   pmin      [optional] min value of set
 * \param[out]   pmax      [optional] max value of set
 * \param[out]   pmean     [optional] mean value of set
 * \param[out]   pvariance [optional] variance
 * \param[out]   pmedian   [optional] median value of set
 * \param[in]    rank      in [0.0 ... 1.0]; median has a rank 0.5;
 *                         ignored if &rval == NULL
 * \param[out]   prval     [optional] value in na corresponding to %rank
 * \param[out]   phisto    [optional] Numa histogram; use NULL to prevent
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) This is a simple interface for gathering statistics
 *          from a numa, where a histogram is used 'under the covers'
 *          to avoid sorting if a rank value is requested.  In that case,
 *          by using a histogram we are trading speed for accuracy, because
 *          the values in %na are quantized to the center of a set of bins.
 *      (2) If the median, other rank value, or histogram are not requested,
 *          the calculation is all performed on the input Numa.
 *      (3) The variance is the average of the square of the
 *          difference from the mean.  The median is the value in na
 *          with rank 0.5.
 *      (4) There are two situations where this gives rank results with
 *          accuracy comparable to computing stastics directly on the input
 *          data, without binning into a histogram:
 *           (a) the data is integers and the range of data is less than
 *               %maxbins, and
 *           (b) the data is floats and the range is small compared to
 *               %maxbins, so that the binsize is much less than 1.
 *      (5) If a histogram is used and the numbers in the Numa extend
 *          over a large range, you can limit the required storage by
 *          specifying the maximum number of bins in the histogram.
 *          Use %maxbins == 0 to force the bin size to be 1.
 *      (6) This optionally returns the median and one arbitrary rank value.
 *          If you need several rank values, return the histogram and use
 *               numaHistogramGetValFromRank(nah, rank, &rval)
 *          multiple times.
 * </pre>
 */
l_ok
numaGetStatsUsingHistogram(NUMA       *na,
                           l_int32     maxbins,
                           l_float32  *pmin,
                           l_float32  *pmax,
                           l_float32  *pmean,
                           l_float32  *pvariance,
                           l_float32  *pmedian,
                           l_float32   rank,
                           l_float32  *prval,
                           NUMA      **phisto)
{
l_int32    i, n;
l_float32  minval, maxval, fval, mean, sum;
NUMA      *nah;

    PROCNAME("numaGetStatsUsingHistogram");

    if (pmin) *pmin = 0.0;
    if (pmax) *pmax = 0.0;
    if (pmean) *pmean = 0.0;
    if (pvariance) *pvariance = 0.0;
    if (pmedian) *pmedian = 0.0;
    if (prval) *prval = 0.0;
    if (phisto) *phisto = NULL;
    if (!na)
        return ERROR_INT("na not defined", procName, 1);
    if ((n = numaGetCount(na)) == 0)
        return ERROR_INT("numa is empty", procName, 1);

    numaGetMin(na, &minval, NULL);
    numaGetMax(na, &maxval, NULL);
    if (pmin) *pmin = minval;
    if (pmax) *pmax = maxval;
    if (pmean || pvariance) {
        sum = 0.0;
        for (i = 0; i < n; i++) {
            numaGetFValue(na, i, &fval);
            sum += fval;
        }
        mean = sum / (l_float32)n;
        if (pmean) *pmean = mean;
    }
    if (pvariance) {
        sum = 0.0;
        for (i = 0; i < n; i++) {
            numaGetFValue(na, i, &fval);
            sum += fval * fval;
        }
        *pvariance = sum / (l_float32)n - mean * mean;
    }

    if (!pmedian && !prval && !phisto)
        return 0;

    nah = numaMakeHistogramAuto(na, maxbins);
    if (pmedian)
        numaHistogramGetValFromRank(nah, 0.5, pmedian);
    if (prval)
        numaHistogramGetValFromRank(nah, rank, prval);
    if (phisto)
        *phisto = nah;
    else
        numaDestroy(&nah);
    return 0;
}


/*!
 * \brief   numaGetHistogramStats()
 *
 * \param[in]    nahisto     histogram: y(x(i)), i = 0 ... nbins - 1
 * \param[in]    startx      x value of first bin: x(0)
 * \param[in]    deltax      x increment between bins; the bin size; x(1) - x(0)
 * \param[out]   pxmean      [optional] mean value of histogram
 * \param[out]   pxmedian    [optional] median value of histogram
 * \param[out]   pxmode      [optional] mode value of histogram:
 *                           xmode = x(imode), where y(xmode) >= y(x(i)) for
 *                           all i != imode
 * \param[out]   pxvariance  [optional] variance of x
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) If the histogram represents the relation y(x), the
 *          computed values that are returned are the x values.
 *          These are NOT the bucket indices i; they are related to the
 *          bucket indices by
 *                x(i) = startx + i * deltax
 * </pre>
 */
l_ok
numaGetHistogramStats(NUMA       *nahisto,
                      l_float32   startx,
                      l_float32   deltax,
                      l_float32  *pxmean,
                      l_float32  *pxmedian,
                      l_float32  *pxmode,
                      l_float32  *pxvariance)
{
    PROCNAME("numaGetHistogramStats");

    if (pxmean) *pxmean = 0.0;
    if (pxmedian) *pxmedian = 0.0;
    if (pxmode) *pxmode = 0.0;
    if (pxvariance) *pxvariance = 0.0;
    if (!nahisto)
        return ERROR_INT("nahisto not defined", procName, 1);

    return numaGetHistogramStatsOnInterval(nahisto, startx, deltax, 0, -1,
                                           pxmean, pxmedian, pxmode,
                                           pxvariance);
}


/*!
 * \brief   numaGetHistogramStatsOnInterval()
 *
 * \param[in]    nahisto    histogram: y(x(i)), i = 0 ... nbins - 1
 * \param[in]    startx     x value of first bin: x(0)
 * \param[in]    deltax     x increment between bins; the bin size; x(1) - x(0)
 * \param[in]    ifirst     first bin to use for collecting stats
 * \param[in]    ilast      last bin for collecting stats; -1 to go to the end
 * \param[out]   pxmean     [optional] mean value of histogram
 * \param[out]   pxmedian   [optional] median value of histogram
 * \param[out]   pxmode     [optional] mode value of histogram:
 *                          xmode = x(imode), where y(xmode) >= y(x(i)) for
 *                          all i != imode
 * \param[out]   pxvariance [optional] variance of x
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) If the histogram represents the relation y(x), the
 *          computed values that are returned are the x values.
 *          These are NOT the bucket indices i; they are related to the
 *          bucket indices by
 *                x(i) = startx + i * deltax
 * </pre>
 */
l_ok
numaGetHistogramStatsOnInterval(NUMA       *nahisto,
                                l_float32   startx,
                                l_float32   deltax,
                                l_int32     ifirst,
                                l_int32     ilast,
                                l_float32  *pxmean,
                                l_float32  *pxmedian,
                                l_float32  *pxmode,
                                l_float32  *pxvariance)
{
l_int32    i, n, imax;
l_float32  sum, sumval, halfsum, moment, var, x, y, ymax;

    PROCNAME("numaGetHistogramStatsOnInterval");

    if (pxmean) *pxmean = 0.0;
    if (pxmedian) *pxmedian = 0.0;
    if (pxmode) *pxmode = 0.0;
    if (pxvariance) *pxvariance = 0.0;
    if (!nahisto)
        return ERROR_INT("nahisto not defined", procName, 1);
    if (!pxmean && !pxmedian && !pxmode && !pxvariance)
        return ERROR_INT("nothing to compute", procName, 1);

    n = numaGetCount(nahisto);
    ifirst = L_MAX(0, ifirst);
    if (ilast < 0) ilast = n - 1;
    if (ifirst >= n)
        return ERROR_INT("invalid ifirst", procName, 1);
    if (ilast >= n) {
        L_WARNING("ilast = %d is beyond max index = %d; adjusting\n",
                  procName, ilast, n - 1);
        ilast = n - 1;
    }
    if (ifirst > ilast)
        return ERROR_INT("ifirst > ilast", procName, 1);
    for (sum = 0.0, moment = 0.0, var = 0.0, i = ifirst; i <= ilast ; i++) {
        x = startx + i * deltax;
        numaGetFValue(nahisto, i, &y);
        sum += y;
        moment += x * y;
        var += x * x * y;
    }
    if (sum == 0.0) {
        L_INFO("sum is 0\n", procName);
        return 0;
    }

    if (pxmean)
        *pxmean = moment / sum;
    if (pxvariance)
        *pxvariance = var / sum - moment * moment / (sum * sum);

    if (pxmedian) {
        halfsum = sum / 2.0;
        for (sumval = 0.0, i = ifirst; i <= ilast; i++) {
            numaGetFValue(nahisto, i, &y);
            sumval += y;
            if (sumval >= halfsum) {
                *pxmedian = startx + i * deltax;
                break;
            }
        }
    }

    if (pxmode) {
        imax = -1;
        ymax = -1.0e10;
        for (i = ifirst; i <= ilast; i++) {
            numaGetFValue(nahisto, i, &y);
            if (y > ymax) {
                ymax = y;
                imax = i;
            }
        }
        *pxmode = startx + imax * deltax;
    }

    return 0;
}


/*!
 * \brief   numaMakeRankFromHistogram()
 *
 * \param[in]    startx   xval corresponding to first element in nay
 * \param[in]    deltax   x increment between array elements in nay
 * \param[in]    nasy     input histogram, assumed equally spaced
 * \param[in]    npts     number of points to evaluate rank function
 * \param[out]   pnax     [optional] array of x values in range
 * \param[out]   pnay     rank array of specified npts
 * \return  0 if OK, 1 on error
 */
l_ok
numaMakeRankFromHistogram(l_float32  startx,
                          l_float32  deltax,
                          NUMA      *nasy,
                          l_int32    npts,
                          NUMA     **pnax,
                          NUMA     **pnay)
{
l_int32    i, n;
l_float32  sum, fval;
NUMA      *nan, *nar;

    PROCNAME("numaMakeRankFromHistogram");

    if (pnax) *pnax = NULL;
    if (!pnay)
        return ERROR_INT("&nay not defined", procName, 1);
    *pnay = NULL;
    if (!nasy)
        return ERROR_INT("nasy not defined", procName, 1);
    if ((n = numaGetCount(nasy)) == 0)
        return ERROR_INT("no bins in nas", procName, 1);

        /* Normalize and generate the rank array corresponding to
         * the binned histogram. */
    nan = numaNormalizeHistogram(nasy, 1.0);
    nar = numaCreate(n + 1);  /* rank numa corresponding to nan */
    sum = 0.0;
    numaAddNumber(nar, sum);  /* first element is 0.0 */
    for (i = 0; i < n; i++) {
        numaGetFValue(nan, i, &fval);
        sum += fval;
        numaAddNumber(nar, sum);
    }

        /* Compute rank array on full range with specified
         * number of points and correspondence to x-values. */
    numaInterpolateEqxInterval(startx, deltax, nar, L_LINEAR_INTERP,
                               startx, startx + n * deltax, npts,
                               pnax, pnay);
    numaDestroy(&nan);
    numaDestroy(&nar);
    return 0;
}


/*!
 * \brief   numaHistogramGetRankFromVal()
 *
 * \param[in]    na     histogram
 * \param[in]    rval   value of input sample for which we want the rank
 * \param[out]   prank  fraction of total samples below rval
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) If we think of the histogram as a function y(x), normalized
 *          to 1, for a given input value of x, this computes the
 *          rank of x, which is the integral of y(x) from the start
 *          value of x to the input value.
 *      (2) This function only makes sense when applied to a Numa that
 *          is a histogram.  The values in the histogram can be ints and
 *          floats, and are computed as floats.  The rank is returned
 *          as a float between 0.0 and 1.0.
 *      (3) The numa parameters startx and binsize are used to
 *          compute x from the Numa index i.
 * </pre>
 */
l_ok
numaHistogramGetRankFromVal(NUMA       *na,
                            l_float32   rval,
                            l_float32  *prank)
{
l_int32    i, ibinval, n;
l_float32  startval, binsize, binval, maxval, fractval, total, sum, val;

    PROCNAME("numaHistogramGetRankFromVal");

    if (!prank)
        return ERROR_INT("prank not defined", procName, 1);
    *prank = 0.0;
    if (!na)
        return ERROR_INT("na not defined", procName, 1);
    numaGetParameters(na, &startval, &binsize);
    n = numaGetCount(na);
    if (rval < startval)
        return 0;
    maxval = startval + n * binsize;
    if (rval > maxval) {
        *prank = 1.0;
        return 0;
    }

    binval = (rval - startval) / binsize;
    ibinval = (l_int32)binval;
    if (ibinval >= n) {
        *prank = 1.0;
        return 0;
    }
    fractval = binval - (l_float32)ibinval;

    sum = 0.0;
    for (i = 0; i < ibinval; i++) {
        numaGetFValue(na, i, &val);
        sum += val;
    }
    numaGetFValue(na, ibinval, &val);
    sum += fractval * val;
    numaGetSum(na, &total);
    *prank = sum / total;

/*    fprintf(stderr, "binval = %7.3f, rank = %7.3f\n", binval, *prank); */

    return 0;
}


/*!
 * \brief   numaHistogramGetValFromRank()
 *
 * \param[in]    na     histogram
 * \param[in]    rank   fraction of total samples
 * \param[out]   prval  approx. to the bin value
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) If we think of the histogram as a function y(x), this returns
 *          the value x such that the integral of y(x) from the start
 *          value to x gives the fraction 'rank' of the integral
 *          of y(x) over all bins.
 *      (2) This function only makes sense when applied to a Numa that
 *          is a histogram.  The values in the histogram can be ints and
 *          floats, and are computed as floats.  The val is returned
 *          as a float, even though the buckets are of integer width.
 *      (3) The numa parameters startx and binsize are used to
 *          compute x from the Numa index i.
 * </pre>
 */
l_ok
numaHistogramGetValFromRank(NUMA       *na,
                            l_float32   rank,
                            l_float32  *prval)
{
l_int32    i, n;
l_float32  startval, binsize, rankcount, total, sum, fract, val;

    PROCNAME("numaHistogramGetValFromRank");

    if (!prval)
        return ERROR_INT("prval not defined", procName, 1);
    *prval = 0.0;
    if (!na)
        return ERROR_INT("na not defined", procName, 1);
    if (rank < 0.0) {
        L_WARNING("rank < 0; setting to 0.0\n", procName);
        rank = 0.0;
    }
    if (rank > 1.0) {
        L_WARNING("rank > 1.0; setting to 1.0\n", procName);
        rank = 1.0;
    }

    n = numaGetCount(na);
    numaGetParameters(na, &startval, &binsize);
    numaGetSum(na, &total);
    rankcount = rank * total;  /* count that corresponds to rank */
    sum = 0.0;
    for (i = 0; i < n; i++) {
        numaGetFValue(na, i, &val);
        if (sum + val >= rankcount)
            break;
        sum += val;
    }
    if (val <= 0.0)  /* can == 0 if rank == 0.0 */
        fract = 0.0;
    else  /* sum + fract * val = rankcount */
        fract = (rankcount - sum) / val;

    /* The use of the fraction of a bin allows a simple calculation
     * for the histogram value at the given rank. */
    *prval = startval + binsize * ((l_float32)i + fract);

/*    fprintf(stderr, "rank = %7.3f, val = %7.3f\n", rank, *prval); */

    return 0;
}


/*!
 * \brief   numaDiscretizeRankAndIntensity()
 *
 * \param[in]    na       normalized histo of probability density vs intensity
 * \param[in]    nbins    number of bins at which the rank is divided
 * \param[out]   pnarbin  [optional] rank bin value vs intensity
 * \param[out]   pnam     [optional] median intensity in a bin vs rank bin
 *                        value, with %nbins of discretized rank values
 * \param[out]   pnar     [optional] rank vs intensity; this is
 *                        a cumulative norm histogram
 * \param[out]   pnabb    [optional] intensity at the right bin boundary
 *                        vs rank bin
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) We are inverting the rank(intensity) function to get
 *          the intensity(rank) function at %nbins equally spaced
 *          values of rank between 0.0 and 1.0.  We save integer values
 *          for the intensity.
 *      (2) We are using the word "intensity" to describe the type of
 *          array values, but any array of non-negative numbers will work.
 *      (3) The output arrays give the following mappings, where the
 *          input is a normalized histogram of array values:
 *             array values     -->  rank bin number  (narbin)
 *             rank bin number  -->  median array value in bin (nam)
 *             array values     -->  cumulative norm = rank  (nar)
 *             rank bin number  -->  array value at right bin edge (nabb)
 * </pre>
 */
l_ok
numaDiscretizeRankAndIntensity(NUMA    *na,
                               l_int32  nbins,
                               NUMA   **pnarbin,
                               NUMA   **pnam,
                               NUMA   **pnar,
                               NUMA   **pnabb)
{
NUMA      *nar;  /* rank value as function of intensity */
NUMA      *nam;  /* median intensity in the rank bins */
NUMA      *nabb;  /* rank bin right boundaries (in intensity) */
NUMA      *narbin;  /* binned rank value as a function of intensity */
l_int32    i, j, npts, start, midfound, mcount, rightedge;
l_float32  sum, midrank, endrank, val;

    PROCNAME("numaDiscretizeRankAndIntensity");

    if (pnarbin) *pnarbin = NULL;
    if (pnam) *pnam = NULL;
    if (pnar) *pnar = NULL;
    if (pnabb) *pnabb = NULL;
    if (!pnarbin && !pnam && !pnar && !pnabb)
        return ERROR_INT("no output requested", procName, 1);
    if (!na)
        return ERROR_INT("na not defined", procName, 1);
    if (nbins < 2)
        return ERROR_INT("nbins must be > 1", procName, 1);

        /* Get cumulative normalized histogram (rank vs intensity value).
         * For a normalized histogram from an 8 bpp grayscale image
         * as input, we have 256 bins and 257 points in the
         * cumulative (rank) histogram. */
    npts = numaGetCount(na);
    if ((nar = numaCreate(npts + 1)) == NULL)
        return ERROR_INT("nar not made", procName, 1);
    sum = 0.0;
    numaAddNumber(nar, sum);  /* left side of first bin */
    for (i = 0; i < npts; i++) {
        numaGetFValue(na, i, &val);
        sum += val;
        numaAddNumber(nar, sum);
    }

    nam = numaCreate(nbins);
    narbin = numaCreate(npts);
    nabb = numaCreate(nbins);
    if (!nam || !narbin || !nabb) {
        numaDestroy(&nar);
        numaDestroy(&nam);
        numaDestroy(&narbin);
        numaDestroy(&nabb);
        return ERROR_INT("numa not made", procName, 1);
    }

        /* We find the intensity value at the right edge of each of
         * the rank bins.  We also find the median intensity in the bin,
         * where approximately half the samples are lower and half are
         * higher.  This can be considered as a simple approximation
         * for the average intensity in the bin. */
    start = 0;  /* index in nar */
    mcount = 0;  /* count of median values in rank bins; not to exceed nbins */
    for (i = 0; i < nbins; i++) {
        midrank = (l_float32)(i + 0.5) / (l_float32)(nbins);
        endrank = (l_float32)(i + 1.0) / (l_float32)(nbins);
        endrank = L_MAX(0.0, L_MIN(endrank - 0.001, 1.0));
        midfound = FALSE;
        for (j = start; j < npts; j++) {  /* scan up for each bin value */
            numaGetFValue(nar, j, &val);
                /* Use (j == npts - 1) tests in case all weight is at top end */
            if ((!midfound && val >= midrank) ||
                (mcount < nbins && j == npts - 1)) {
                midfound = TRUE;
                numaAddNumber(nam, j);
                mcount++;
            }
            if ((val >= endrank) || (j == npts - 1)) {
                numaAddNumber(nabb, j);
                if (val == endrank)
                    start = j;
                else
                    start = j - 1;
                break;
            }
        }
    }
    numaSetValue(nabb, nbins - 1, npts - 1);  /* extend to max */

        /* Error checking: did we get data in all bins? */
    if (mcount != nbins)
        L_WARNING("found data for %d bins; should be %d\n",
                  procName, mcount, nbins);

        /* Generate LUT that maps from intensity to bin number */
    start = 0;
    for (i = 0; i < nbins; i++) {
        numaGetIValue(nabb, i, &rightedge);
        for (j = start; j < npts; j++) {
            if (j <= rightedge)
                numaAddNumber(narbin, i);
            if (j > rightedge) {
                start = j;
                break;
            }
            if (j == npts - 1) {  /* we're done */
                start = j + 1;
                break;
            }
        }
    }

    if (pnarbin)
        *pnarbin = narbin;
    else
        numaDestroy(&narbin);
    if (pnam)
        *pnam = nam;
    else
        numaDestroy(&nam);
    if (pnar)
        *pnar = nar;
    else
        numaDestroy(&nar);
    if (pnabb)
        *pnabb = nabb;
    else
        numaDestroy(&nabb);
    return 0;
}


/*!
 * \brief   numaGetRankBinValues()
 *
 * \param[in]    na       an array of values
 * \param[in]    nbins    number of bins at which the rank is divided
 * \param[out]   pnarbin  [optional] rank bin value vs array value
 * \param[out]   pnam     [optional] median intensity in a bin vs rank bin
 *                        value, with %nbins of discretized rank values
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) Simple interface for getting a binned rank representation
 *          of an input array of values.  This returns two mappings:
 *             array value     -->  rank bin number  (narbin)
 *             rank bin number -->  median array value in each rank bin (nam)
 * </pre>
 */
l_ok
numaGetRankBinValues(NUMA    *na,
                     l_int32  nbins,
                     NUMA   **pnarbin,
                     NUMA   **pnam)
{
NUMA      *nah, *nan;  /* histo and normalized histo */
l_int32    maxbins, discardval;
l_float32  maxval, delx;

    PROCNAME("numaGetRankBinValues");

    if (pnarbin) *pnarbin = NULL;
    if (pnam) *pnam = NULL;
    if (!pnarbin && !pnam)
        return ERROR_INT("no output requested", procName, 1);
    if (!na)
        return ERROR_INT("na not defined", procName, 1);
    if (numaGetCount(na) == 0)
        return ERROR_INT("na is empty", procName, 1);
    if (nbins < 2)
        return ERROR_INT("nbins must be > 1", procName, 1);

        /* Get normalized histogram  */
    numaGetMax(na, &maxval, NULL);
    maxbins = L_MIN(100002, (l_int32)maxval + 2);
    nah = numaMakeHistogram(na, maxbins, &discardval, NULL);
    nan = numaNormalizeHistogram(nah, 1.0);

        /* Warn if there is a scale change.  This shouldn't happen
         * unless the max value is above 100000.  */
    numaGetParameters(nan, NULL, &delx);
    if (delx > 1.0)
        L_WARNING("scale change: delx = %6.2f\n", procName, delx);

        /* Rank bin the results */
    numaDiscretizeRankAndIntensity(nan, nbins, pnarbin, pnam, NULL, NULL);
    numaDestroy(&nah);
    numaDestroy(&nan);
    return 0;
}


/*----------------------------------------------------------------------*
 *                      Splitting a distribution                        *
 *----------------------------------------------------------------------*/
/*!
 * \brief   numaSplitDistribution()
 *
 * \param[in]    na           histogram
 * \param[in]    scorefract   fraction of the max score, used to determine
 *                            range over which the histogram min is searched
 * \param[out]   psplitindex  [optional] index for splitting
 * \param[out]   pave1        [optional] average of lower distribution
 * \param[out]   pave2        [optional] average of upper distribution
 * \param[out]   pnum1        [optional] population of lower distribution
 * \param[out]   pnum2        [optional] population of upper distribution
 * \param[out]   pnascore     [optional] for debugging; otherwise use NULL
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) This function is intended to be used on a distribution of
 *          values that represent two sets, such as a histogram of
 *          pixel values for an image with a fg and bg, and the goal
 *          is to determine the averages of the two sets and the
 *          best splitting point.
 *      (2) The Otsu method finds a split point that divides the distribution
 *          into two parts by maximizing a score function that is the
 *          product of two terms:
 *            (a) the square of the difference of centroids, (ave1 - ave2)^2
 *            (b) fract1 * (1 - fract1)
 *          where fract1 is the fraction in the lower distribution.
 *      (3) This works well for images where the fg and bg are
 *          each relatively homogeneous and well-separated in color.
 *          However, if the actual fg and bg sets are very different
 *          in size, and the bg is highly varied, as can occur in some
 *          scanned document images, this will bias the split point
 *          into the larger "bump" (i.e., toward the point where the
 *          (b) term reaches its maximum of 0.25 at fract1 = 0.5.
 *          To avoid this, we define a range of values near the
 *          maximum of the score function, and choose the value within
 *          this range such that the histogram itself has a minimum value.
 *          The range is determined by scorefract: we include all abscissa
 *          values to the left and right of the value that maximizes the
 *          score, such that the score stays above (1 - scorefract) * maxscore.
 *          The intuition behind this modification is to try to find
 *          a split point that both has a high variance score and is
 *          at or near a minimum in the histogram, so that the histogram
 *          slope is small at the split point.
 *      (4) We normalize the score so that if the two distributions
 *          were of equal size and at opposite ends of the numa, the
 *          score would be 1.0.
 * </pre>
 */
l_ok
numaSplitDistribution(NUMA       *na,
                      l_float32   scorefract,
                      l_int32    *psplitindex,
                      l_float32  *pave1,
                      l_float32  *pave2,
                      l_float32  *pnum1,
                      l_float32  *pnum2,
                      NUMA      **pnascore)
{
l_int32    i, n, bestsplit, minrange, maxrange, maxindex;
l_float32  ave1, ave2, ave1prev, ave2prev;
l_float32  num1, num2, num1prev, num2prev;
l_float32  val, minval, sum, fract1;
l_float32  norm, score, minscore, maxscore;
NUMA      *nascore, *naave1, *naave2, *nanum1, *nanum2;

    PROCNAME("numaSplitDistribution");

    if (psplitindex) *psplitindex = 0;
    if (pave1) *pave1 = 0.0;
    if (pave2) *pave2 = 0.0;
    if (pnum1) *pnum1 = 0.0;
    if (pnum2) *pnum2 = 0.0;
    if (pnascore) *pnascore = NULL;
    if (!na)
        return ERROR_INT("na not defined", procName, 1);

    n = numaGetCount(na);
    if (n <= 1)
        return ERROR_INT("n = 1 in histogram", procName, 1);
    numaGetSum(na, &sum);
    if (sum <= 0.0)
        return ERROR_INT("sum <= 0.0", procName, 1);
    norm = 4.0 / ((l_float32)(n - 1) * (n - 1));
    ave1prev = 0.0;
    numaGetHistogramStats(na, 0.0, 1.0, &ave2prev, NULL, NULL, NULL);
    num1prev = 0.0;
    num2prev = sum;
    maxindex = n / 2;  /* initialize with something */

        /* Split the histogram with [0 ... i] in the lower part
         * and [i+1 ... n-1] in upper part.  First, compute an otsu
         * score for each possible splitting.  */
    if ((nascore = numaCreate(n)) == NULL)
        return ERROR_INT("nascore not made", procName, 1);
    naave1 = (pave1) ? numaCreate(n) : NULL;
    naave2 = (pave2) ? numaCreate(n) : NULL;
    nanum1 = (pnum1) ? numaCreate(n) : NULL;
    nanum2 = (pnum2) ? numaCreate(n) : NULL;
    maxscore = 0.0;
    for (i = 0; i < n; i++) {
        numaGetFValue(na, i, &val);
        num1 = num1prev + val;
        if (num1 == 0)
            ave1 = ave1prev;
        else
            ave1 = (num1prev * ave1prev + i * val) / num1;
        num2 = num2prev - val;
        if (num2 == 0)
            ave2 = ave2prev;
        else
            ave2 = (num2prev * ave2prev - i * val) / num2;
        fract1 = num1 / sum;
        score = norm * (fract1 * (1 - fract1)) * (ave2 - ave1) * (ave2 - ave1);
        numaAddNumber(nascore, score);
        if (pave1) numaAddNumber(naave1, ave1);
        if (pave2) numaAddNumber(naave2, ave2);
        if (pnum1) numaAddNumber(nanum1, num1);
        if (pnum2) numaAddNumber(nanum2, num2);
        if (score > maxscore) {
            maxscore = score;
            maxindex = i;
        }
        num1prev = num1;
        num2prev = num2;
        ave1prev = ave1;
        ave2prev = ave2;
    }

        /* Next, for all contiguous scores within a specified fraction
         * of the max, choose the split point as the value with the
         * minimum in the histogram. */
    minscore = (1. - scorefract) * maxscore;
    for (i = maxindex - 1; i >= 0; i--) {
        numaGetFValue(nascore, i, &val);
        if (val < minscore)
            break;
    }
    minrange = i + 1;
    for (i = maxindex + 1; i < n; i++) {
        numaGetFValue(nascore, i, &val);
        if (val < minscore)
            break;
    }
    maxrange = i - 1;
    numaGetFValue(na, minrange, &minval);
    bestsplit = minrange;
    for (i = minrange + 1; i <= maxrange; i++) {
        numaGetFValue(na, i, &val);
        if (val < minval) {
            minval = val;
            bestsplit = i;
        }
    }

        /* Add one to the bestsplit value to get the threshold value,
         * because when we take a threshold, as in pixThresholdToBinary(),
         * we always choose the set with values below the threshold. */
    bestsplit = L_MIN(255, bestsplit + 1);

    if (psplitindex) *psplitindex = bestsplit;
    if (pave1) numaGetFValue(naave1, bestsplit, pave1);
    if (pave2) numaGetFValue(naave2, bestsplit, pave2);
    if (pnum1) numaGetFValue(nanum1, bestsplit, pnum1);
    if (pnum2) numaGetFValue(nanum2, bestsplit, pnum2);

    if (pnascore) {  /* debug mode */
        fprintf(stderr, "minrange = %d, maxrange = %d\n", minrange, maxrange);
        fprintf(stderr, "minval = %10.0f\n", minval);
        gplotSimple1(nascore, GPLOT_PNG, "/tmp/lept/nascore",
                     "Score for split distribution");
        *pnascore = nascore;
    } else {
        numaDestroy(&nascore);
    }

    if (pave1) numaDestroy(&naave1);
    if (pave2) numaDestroy(&naave2);
    if (pnum1) numaDestroy(&nanum1);
    if (pnum2) numaDestroy(&nanum2);
    return 0;
}


/*----------------------------------------------------------------------*
 *                         Comparing histograms                         *
 *----------------------------------------------------------------------*/
/*!
 * \brief   grayHistogramsToEMD()
 *
 * \param[in]    naa1, naa2    two numaa, each with one or more 256-element
 *                             histograms
 * \param[out]   pnad          nad of EM distances for each histogram
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *     (1) The two numaas must be the same size and have corresponding
 *         256-element histograms.  Pairs do not need to be normalized
 *         to the same sum.
 *     (2) This is typically used on two sets of histograms from
 *         corresponding tiles of two images.  The similarity of two
 *         images can be found with the scoring function used in
 *         pixCompareGrayByHisto():
 *             score S = 1.0 - k * D, where
 *                 k is a constant, say in the range 5-10
 *                 D = EMD
 *             for each tile; for multiple tiles, take the Min(S) over
 *             the set of tiles to be the final score.
 * </pre>
 */
l_ok
grayHistogramsToEMD(NUMAA  *naa1,
                    NUMAA  *naa2,
                    NUMA  **pnad)
{
l_int32     i, n, nt;
l_float32   dist;
NUMA       *na1, *na2, *nad;

    PROCNAME("grayHistogramsToEMD");

    if (!pnad)
        return ERROR_INT("&nad not defined", procName, 1);
    *pnad = NULL;
    if (!naa1 || !naa2)
        return ERROR_INT("na1 and na2 not both defined", procName, 1);
    n = numaaGetCount(naa1);
    if (n != numaaGetCount(naa2))
        return ERROR_INT("naa1 and naa2 numa counts differ", procName, 1);
    nt = numaaGetNumberCount(naa1);
    if (nt != numaaGetNumberCount(naa2))
        return ERROR_INT("naa1 and naa2 number counts differ", procName, 1);
    if (256 * n != nt)  /* good enough check */
        return ERROR_INT("na sizes must be 256", procName, 1);

    nad = numaCreate(n);
    *pnad = nad;
    for (i = 0; i < n; i++) {
        na1 = numaaGetNuma(naa1, i, L_CLONE);
        na2 = numaaGetNuma(naa2, i, L_CLONE);
        numaEarthMoverDistance(na1, na2, &dist);
        numaAddNumber(nad, dist / 255.);  /* normalize to [0.0 - 1.0] */
        numaDestroy(&na1);
        numaDestroy(&na2);
    }
    return 0;
}


/*!
 * \brief   numaEarthMoverDistance()
 *
 * \param[in]    na1, na2    two numas of the same size, typically histograms
 * \param[out]   pdist       earthmover distance
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *     (1) The two numas must have the same size.  They do not need to be
 *         normalized to the same sum before applying the function.
 *     (2) For a 1D discrete function, the implementation of the EMD
 *         is trivial.  Just keep filling or emptying buckets in one numa
 *         to match the amount in the other, moving sequentially along
 *         both arrays.
 *     (3) We divide the sum of the absolute value of everything moved
 *         (by 1 unit at a time) by the sum of the numa (amount of "earth")
 *         to get the average distance that the "earth" was moved.
 *         This is the value returned here.
 *     (4) The caller can do a further normalization, by the number of
 *         buckets (minus 1), to get the EM distance as a fraction of
 *         the maximum possible distance, which is n-1.  This fraction
 *         is 1.0 for the situation where all the 'earth' in the first
 *         array is at one end, and all in the second array is at the
 *         other end.
 * </pre>
 */
l_ok
numaEarthMoverDistance(NUMA       *na1,
                       NUMA       *na2,
                       l_float32  *pdist)
{
l_int32     n, norm, i;
l_float32   sum1, sum2, diff, total;
l_float32  *array1, *array3;
NUMA       *na3;

    PROCNAME("numaEarthMoverDistance");

    if (!pdist)
        return ERROR_INT("&dist not defined", procName, 1);
    *pdist = 0.0;
    if (!na1 || !na2)
        return ERROR_INT("na1 and na2 not both defined", procName, 1);
    n = numaGetCount(na1);
    if (n != numaGetCount(na2))
        return ERROR_INT("na1 and na2 have different size", procName, 1);

        /* Generate na3; normalize to na1 if necessary */
    numaGetSum(na1, &sum1);
    numaGetSum(na2, &sum2);
    norm = (L_ABS(sum1 - sum2) < 0.00001 * L_ABS(sum1)) ? 1 : 0;
    if (!norm)
        na3 = numaTransform(na2, 0, sum1 / sum2);
    else
        na3 = numaCopy(na2);
    array1 = numaGetFArray(na1, L_NOCOPY);
    array3 = numaGetFArray(na3, L_NOCOPY);

        /* Move earth in n3 from array elements, to match n1 */
    total = 0;
    for (i = 1; i < n; i++) {
        diff = array1[i - 1] - array3[i - 1];
        array3[i] -= diff;
        total += L_ABS(diff);
    }
    *pdist = total / sum1;

    numaDestroy(&na3);
    return 0;
}


/*!
 * \brief   grayInterHistogramStats()
 *
 * \param[in]    naa      numaa with two or more 256-element histograms
 * \param[in]    wc       half-width of the smoothing window
 * \param[out]   pnam     [optional] mean values
 * \param[out]   pnams    [optional] mean square values
 * \param[out]   pnav     [optional] variances
 * \param[out]   pnarv    [optional] rms deviations from the mean
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *     (1) The %naa has two or more 256-element numa histograms, which
 *         are to be compared value-wise at each of the 256 gray levels.
 *         The result are stats (mean, mean square, variance, root variance)
 *         aggregated across the set of histograms, and each is output
 *         as a 256 entry numa.  Think of these histograms as a matrix,
 *         where each histogram is one row of the array.  The stats are
 *         then aggregated column-wise, between the histograms.
 *     (2) These stats are:
 *            ~ average value: <v>  (nam)
 *            ~ average squared value: <v*v> (nams)
 *            ~ variance: <(v - <v>)*(v - <v>)> = <v*v> - <v>*<v>  (nav)
 *            ~ square-root of variance: (narv)
 *         where the brackets < .. > indicate that the average value is
 *         to be taken over each column of the array.
 *     (3) The input histograms are optionally smoothed before these
 *         statistical operations.
 *     (4) The input histograms are normalized to a sum of 10000.  By
 *         doing this, the resulting numbers are independent of the
 *         number of samples used in building the individual histograms.
 *     (5) A typical application is on a set of histograms from tiles
 *         of an image, to distinguish between text/tables and photo
 *         regions.  If the tiles are much larger than the text line
 *         spacing, text/table regions typically have smaller variance
 *         across tiles than photo regions.  For this application, it
 *         may be useful to ignore values near white, which are large for
 *         text and would magnify the variance due to variations in
 *         illumination.  However, because the variance of a drawing or
 *         a light photo can be similar to that of grayscale text, this
 *         function is only a discriminator between darker photos/drawings
 *         and light photos/text/line-graphics.
 * </pre>
 */
l_ok
grayInterHistogramStats(NUMAA   *naa,
                        l_int32  wc,
                        NUMA   **pnam,
                        NUMA   **pnams,
                        NUMA   **pnav,
                        NUMA   **pnarv)
{
l_int32      i, j, n, nn;
l_float32  **arrays;
l_float32    mean, var, rvar;
NUMA        *na1, *na2, *na3, *na4;

    PROCNAME("grayInterHistogramStats");

    if (pnam) *pnam = NULL;
    if (pnams) *pnams = NULL;
    if (pnav) *pnav = NULL;
    if (pnarv) *pnarv = NULL;
    if (!pnam && !pnams && !pnav && !pnarv)
        return ERROR_INT("nothing requested", procName, 1);
    if (!naa)
        return ERROR_INT("naa not defined", procName, 1);
    n = numaaGetCount(naa);
    for (i = 0; i < n; i++) {
        nn = numaaGetNumaCount(naa, i);
        if (nn != 256) {
            L_ERROR("%d numbers in numa[%d]\n", procName, nn, i);
            return 1;
        }
    }

    if (pnam) *pnam = numaCreate(256);
    if (pnams) *pnams = numaCreate(256);
    if (pnav) *pnav = numaCreate(256);
    if (pnarv) *pnarv = numaCreate(256);

        /* First, use mean smoothing, normalize each histogram,
         * and save all results in a 2D matrix. */
    arrays = (l_float32 **)LEPT_CALLOC(n, sizeof(l_float32 *));
    for (i = 0; i < n; i++) {
        na1 = numaaGetNuma(naa, i, L_CLONE);
        na2 = numaWindowedMean(na1, wc);
        na3 = numaNormalizeHistogram(na2, 10000.);
        arrays[i] = numaGetFArray(na3, L_COPY);
        numaDestroy(&na1);
        numaDestroy(&na2);
        numaDestroy(&na3);
    }

        /* Get stats between histograms */
    for (j = 0; j < 256; j++) {
        na4 = numaCreate(n);
        for (i = 0; i < n; i++) {
            numaAddNumber(na4, arrays[i][j]);
        }
        numaSimpleStats(na4, 0, -1, &mean, &var, &rvar);
        if (pnam) numaAddNumber(*pnam, mean);
        if (pnams) numaAddNumber(*pnams, mean * mean);
        if (pnav) numaAddNumber(*pnav, var);
        if (pnarv) numaAddNumber(*pnarv, rvar);
        numaDestroy(&na4);
    }

    for (i = 0; i < n; i++)
        LEPT_FREE(arrays[i]);
    LEPT_FREE(arrays);
    return 0;
}


/*----------------------------------------------------------------------*
 *                             Extrema finding                          *
 *----------------------------------------------------------------------*/
/*!
 * \brief   numaFindPeaks()
 *
 * \param[in]    nas      source numa
 * \param[in]    nmax     max number of peaks to be found
 * \param[in]    fract1   min fraction of peak value
 * \param[in]    fract2   min slope
 * \return  peak na, or NULL on error.
 *
 * <pre>
 * Notes:
 *     (1) The returned na consists of sets of four numbers representing
 *         the peak, in the following order:
 *            left edge; peak center; right edge; normalized peak area
 * </pre>
 */
NUMA *
numaFindPeaks(NUMA      *nas,
              l_int32    nmax,
              l_float32  fract1,
              l_float32  fract2)
{
l_int32    i, k, n, maxloc, lloc, rloc;
l_float32  fmaxval, sum, total, newtotal, val, lastval;
l_float32  peakfract;
NUMA      *na, *napeak;

    PROCNAME("numaFindPeaks");

    if (!nas)
        return (NUMA *)ERROR_PTR("nas not defined", procName, NULL);
    n = numaGetCount(nas);
    numaGetSum(nas, &total);

        /* We munge this copy */
    if ((na = numaCopy(nas)) == NULL)
        return (NUMA *)ERROR_PTR("na not made", procName, NULL);
    if ((napeak = numaCreate(4 * nmax)) == NULL) {
        numaDestroy(&na);
        return (NUMA *)ERROR_PTR("napeak not made", procName, NULL);
    }

    for (k = 0; k < nmax; k++) {
        numaGetSum(na, &newtotal);
        if (newtotal == 0.0)   /* sanity check */
            break;
        numaGetMax(na, &fmaxval, &maxloc);
        sum = fmaxval;
        lastval = fmaxval;
        lloc = 0;
        for (i = maxloc - 1; i >= 0; --i) {
            numaGetFValue(na, i, &val);
            if (val == 0.0) {
                lloc = i + 1;
                break;
            }
            if (val > fract1 * fmaxval) {
                sum += val;
                lastval = val;
                continue;
            }
            if (lastval - val > fract2 * lastval) {
                sum += val;
                lastval = val;
                continue;
            }
            lloc = i;
            break;
        }
        lastval = fmaxval;
        rloc = n - 1;
        for (i = maxloc + 1; i < n; ++i) {
            numaGetFValue(na, i, &val);
            if (val == 0.0) {
                rloc = i - 1;
                break;
            }
            if (val > fract1 * fmaxval) {
                sum += val;
                lastval = val;
                continue;
            }
            if (lastval - val > fract2 * lastval) {
                sum += val;
                lastval = val;
                continue;
            }
            rloc = i;
            break;
        }
        peakfract = sum / total;
        numaAddNumber(napeak, lloc);
        numaAddNumber(napeak, maxloc);
        numaAddNumber(napeak, rloc);
        numaAddNumber(napeak, peakfract);

        for (i = lloc; i <= rloc; i++)
            numaSetValue(na, i, 0.0);
    }

    numaDestroy(&na);
    return napeak;
}


/*!
 * \brief   numaFindExtrema()
 *
 * \param[in]    nas     input values
 * \param[in]    delta   relative amount to resolve peaks and valleys
 * \param[out]   pnav    [optional] values of extrema
 * \return  nad (locations of extrema, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) This returns a sequence of extrema (peaks and valleys).
 *      (2) The algorithm is analogous to that for determining
 *          mountain peaks.  Suppose we have a local peak, with
 *          bumps on the side.  Under what conditions can we consider
 *          those 'bumps' to be actual peaks?  The answer: if the
 *          bump is separated from the peak by a saddle that is at
 *          least 500 feet below the bump.
 *      (3) Operationally, suppose we are looking for a peak.
 *          We are keeping the largest value we've seen since the
 *          last valley, and are looking for a value that is delta
 *          BELOW our current peak.  When we find such a value,
 *          we label the peak, use the current value to label the
 *          valley, and then do the same operation in reverse (looking
 *          for a valley).
 * </pre>
 */
NUMA *
numaFindExtrema(NUMA      *nas,
                l_float32  delta,
                NUMA     **pnav)
{
l_int32    i, n, found, loc, direction;
l_float32  startval, val, maxval, minval;
NUMA      *nav, *nad;

    PROCNAME("numaFindExtrema");

    if (pnav) *pnav = NULL;
    if (!nas)
        return (NUMA *)ERROR_PTR("nas not defined", procName, NULL);
    if (delta < 0.0)
        return (NUMA *)ERROR_PTR("delta < 0", procName, NULL);

    n = numaGetCount(nas);
    nad = numaCreate(0);
    nav =  NULL;
    if (pnav) {
        nav = numaCreate(0);
        *pnav = nav;
    }

        /* We don't know if we'll find a peak or valley first,
         * but use the first element of nas as the reference point.
         * Break when we deviate by 'delta' from the first point. */
    numaGetFValue(nas, 0, &startval);
    found = FALSE;
    for (i = 1; i < n; i++) {
        numaGetFValue(nas, i, &val);
        if (L_ABS(val - startval) >= delta) {
            found = TRUE;
            break;
        }
    }

    if (!found)
        return nad;  /* it's empty */

        /* Are we looking for a peak or a valley? */
    if (val > startval) {  /* peak */
        direction = 1;
        maxval = val;
    } else {
        direction = -1;
        minval = val;
    }
    loc = i;

        /* Sweep through the rest of the array, recording alternating
         * peak/valley extrema. */
    for (i = i + 1; i < n; i++) {
        numaGetFValue(nas, i, &val);
        if (direction == 1 && val > maxval ) {  /* new local max */
            maxval = val;
            loc = i;
        } else if (direction == -1 && val < minval ) {  /* new local min */
            minval = val;
            loc = i;
        } else if (direction == 1 && (maxval - val >= delta)) {
            numaAddNumber(nad, loc);  /* save the current max location */
            if (nav) numaAddNumber(nav, maxval);
            direction = -1;  /* reverse: start looking for a min */
            minval = val;
            loc = i;  /* current min location */
        } else if (direction == -1 && (val - minval >= delta)) {
            numaAddNumber(nad, loc);  /* save the current min location */
            if (nav) numaAddNumber(nav, minval);
            direction = 1;  /* reverse: start looking for a max */
            maxval = val;
            loc = i;  /* current max location */
        }
    }

        /* Save the final extremum */
/*    numaAddNumber(nad, loc); */
    return nad;
}


/*!
 * \brief   numaCountReversals()
 *
 * \param[in]    nas          input values
 * \param[in]    minreversal  relative amount to resolve peaks and valleys
 * \param[out]   pnr          [optional] number of reversals
 * \param[out]   prd          [optional] reversal density: reversals/length
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) The input numa is can be generated from pixExtractAlongLine().
 *          If so, the x parameters can be used to find the reversal
 *          frequency along a line.
 *      (2) If the input numa was generated from a 1 bpp pix, the
 *          values will be 0 and 1.  Use %minreversal == 1 to get
 *          the number of pixel flips.  If the only values are 0 and 1,
 *          but %minreversal > 1, set the reversal count to 0 and
 *          issue a warning.
 * </pre>
 */
l_ok
numaCountReversals(NUMA       *nas,
                   l_float32   minreversal,
                   l_int32    *pnr,
                   l_float32  *prd)
{
l_int32    i, n, nr, ival, binvals;
l_int32   *ia;
l_float32  fval, delx, len;
NUMA      *nat;

    PROCNAME("numaCountReversals");

    if (pnr) *pnr = 0;
    if (prd) *prd = 0.0;
    if (!pnr && !prd)
        return ERROR_INT("neither &nr nor &rd are defined", procName, 1);
    if (!nas)
        return ERROR_INT("nas not defined", procName, 1);
    if ((n = numaGetCount(nas)) == 0) {
        L_INFO("nas is empty\n", procName);
        return 0;
    }
    if (minreversal < 0.0)
        return ERROR_INT("minreversal < 0", procName, 1);

        /* Decide if the only values are 0 and 1 */
    binvals = TRUE;
    for (i = 0; i < n; i++) {
        numaGetFValue(nas, i, &fval);
        if (fval != 0.0 && fval != 1.0) {
            binvals = FALSE;
            break;
        }
    }

    nr = 0;
    if (binvals) {
        if (minreversal > 1.0) {
            L_WARNING("binary values but minreversal > 1\n", procName);
        } else {
            ia = numaGetIArray(nas);
            ival = ia[0];
            for (i = 1; i < n; i++) {
                if (ia[i] != ival) {
                    nr++;
                    ival = ia[i];
                }
            }
            LEPT_FREE(ia);
        }
    } else {
        nat = numaFindExtrema(nas, minreversal, NULL);
        nr = numaGetCount(nat);
        numaDestroy(&nat);
    }
    if (pnr) *pnr = nr;
    if (prd) {
        numaGetParameters(nas, NULL, &delx);
        len = delx * n;
        *prd = (l_float32)nr / len;
    }

    return 0;
}


/*----------------------------------------------------------------------*
 *                Threshold crossings and frequency analysis            *
 *----------------------------------------------------------------------*/
/*!
 * \brief   numaSelectCrossingThreshold()
 *
 * \param[in]    nax          [optional] numa of abscissa values; can be NULL
 * \param[in]    nay          signal
 * \param[in]    estthresh    estimated pixel threshold for crossing:
 *                            e.g., for images, white <--> black; typ. ~120
 * \param[out]   pbestthresh  robust estimate of threshold to use
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *     (1) When a valid threshold is used, the number of crossings is
 *         a maximum, because none are missed.  If no threshold intersects
 *         all the crossings, the crossings must be determined with
 *         numaCrossingsByPeaks().
 *     (2) %estthresh is an input estimate of the threshold that should
 *         be used.  We compute the crossings with 41 thresholds
 *         (20 below and 20 above).  There is a range in which the
 *         number of crossings is a maximum.  Return a threshold
 *         in the center of this stable plateau of crossings.
 *         This can then be used with numaCrossingsByThreshold()
 *         to get a good estimate of crossing locations.
 * </pre>
 */
l_ok
numaSelectCrossingThreshold(NUMA       *nax,
                            NUMA       *nay,
                            l_float32   estthresh,
                            l_float32  *pbestthresh)
{
l_int32    i, inrun, istart, iend, maxstart, maxend, runlen, maxrunlen;
l_int32    val, maxval, nmax, count;
l_float32  thresh, fmaxval, fmodeval;
NUMA      *nat, *nac;

    PROCNAME("numaSelectCrossingThreshold");

    if (!pbestthresh)
        return ERROR_INT("&bestthresh not defined", procName, 1);
    *pbestthresh = 0.0;
    if (!nay)
        return ERROR_INT("nay not defined", procName, 1);

        /* Compute the number of crossings for different thresholds */
    nat = numaCreate(41);
    for (i = 0; i < 41; i++) {
        thresh = estthresh - 80.0 + 4.0 * i;
        nac = numaCrossingsByThreshold(nax, nay, thresh);
        numaAddNumber(nat, numaGetCount(nac));
        numaDestroy(&nac);
    }

        /* Find the center of the plateau of max crossings, which
         * extends from thresh[istart] to thresh[iend]. */
    numaGetMax(nat, &fmaxval, NULL);
    maxval = (l_int32)fmaxval;
    nmax = 0;
    for (i = 0; i < 41; i++) {
        numaGetIValue(nat, i, &val);
        if (val == maxval)
            nmax++;
    }
    if (nmax < 3) {  /* likely accidental max; try the mode */
        numaGetMode(nat, &fmodeval, &count);
        if (count > nmax && fmodeval > 0.5 * fmaxval)
            maxval = (l_int32)fmodeval;  /* use the mode */
    }

    inrun = FALSE;
    iend = 40;
    maxrunlen = 0, maxstart = 0, maxend = 0;
    for (i = 0; i < 41; i++) {
        numaGetIValue(nat, i, &val);
        if (val == maxval) {
            if (!inrun) {
                istart = i;
                inrun = TRUE;
            }
            continue;
        }
        if (inrun && (val != maxval)) {
            iend = i - 1;
            runlen = iend - istart + 1;
            inrun = FALSE;
            if (runlen > maxrunlen) {
                maxstart = istart;
                maxend = iend;
                maxrunlen = runlen;
            }
        }
    }
    if (inrun) {
        runlen = i - istart;
        if (runlen > maxrunlen) {
            maxstart = istart;
            maxend = i - 1;
            maxrunlen = runlen;
        }
    }

    *pbestthresh = estthresh - 80.0 + 2.0 * (l_float32)(maxstart + maxend);

#if  DEBUG_CROSSINGS
    fprintf(stderr, "\nCrossings attain a maximum at %d thresholds, between:\n"
                    "  thresh[%d] = %5.1f and thresh[%d] = %5.1f\n",
                    nmax, maxstart, estthresh - 80.0 + 4.0 * maxstart,
                    maxend, estthresh - 80.0 + 4.0 * maxend);
    fprintf(stderr, "The best choice: %5.1f\n", *pbestthresh);
    fprintf(stderr, "Number of crossings at the 41 thresholds:");
    numaWriteStream(stderr, nat);
#endif  /* DEBUG_CROSSINGS */

    numaDestroy(&nat);
    return 0;
}


/*!
 * \brief   numaCrossingsByThreshold()
 *
 * \param[in]    nax     [optional] numa of abscissa values; can be NULL
 * \param[in]    nay     numa of ordinate values, corresponding to nax
 * \param[in]    thresh  threshold value for nay
 * \return  nad abscissa pts at threshold, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) If nax == NULL, we use startx and delx from nay to compute
 *          the crossing values in nad.
 * </pre>
 */
NUMA *
numaCrossingsByThreshold(NUMA      *nax,
                         NUMA      *nay,
                         l_float32  thresh)
{
l_int32    i, n;
l_float32  startx, delx;
l_float32  xval1, xval2, yval1, yval2, delta1, delta2, crossval, fract;
NUMA      *nad;

    PROCNAME("numaCrossingsByThreshold");

    if (!nay)
        return (NUMA *)ERROR_PTR("nay not defined", procName, NULL);
    n = numaGetCount(nay);

    if (nax && (numaGetCount(nax) != n))
        return (NUMA *)ERROR_PTR("nax and nay sizes differ", procName, NULL);

    nad = numaCreate(0);
    numaGetFValue(nay, 0, &yval1);
    numaGetParameters(nay, &startx, &delx);
    if (nax)
        numaGetFValue(nax, 0, &xval1);
    else
        xval1 = startx;
    for (i = 1; i < n; i++) {
        numaGetFValue(nay, i, &yval2);
        if (nax)
            numaGetFValue(nax, i, &xval2);
        else
            xval2 = startx + i * delx;
        delta1 = yval1 - thresh;
        delta2 = yval2 - thresh;
        if (delta1 == 0.0) {
            numaAddNumber(nad, xval1);
        } else if (delta2 == 0.0) {
            numaAddNumber(nad, xval2);
        } else if (delta1 * delta2 < 0.0) {  /* crossing */
            fract = L_ABS(delta1) / L_ABS(yval1 - yval2);
            crossval = xval1 + fract * (xval2 - xval1);
            numaAddNumber(nad, crossval);
        }
        xval1 = xval2;
        yval1 = yval2;
    }

    return nad;
}


/*!
 * \brief   numaCrossingsByPeaks()
 *
 * \param[in]    nax     [optional] numa of abscissa values
 * \param[in]    nay     numa of ordinate values, corresponding to nax
 * \param[in]    delta   parameter used to identify when a new peak can be found
 * \return  nad abscissa pts at threshold, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) If nax == NULL, we use startx and delx from nay to compute
 *          the crossing values in nad.
 * </pre>
 */
NUMA *
numaCrossingsByPeaks(NUMA      *nax,
                     NUMA      *nay,
                     l_float32  delta)
{
l_int32    i, j, n, np, previndex, curindex;
l_float32  startx, delx;
l_float32  xval1, xval2, yval1, yval2, delta1, delta2;
l_float32  prevval, curval, thresh, crossval, fract;
NUMA      *nap, *nad;

    PROCNAME("numaCrossingsByPeaks");

    if (!nay)
        return (NUMA *)ERROR_PTR("nay not defined", procName, NULL);

    n = numaGetCount(nay);
    if (nax && (numaGetCount(nax) != n))
        return (NUMA *)ERROR_PTR("nax and nay sizes differ", procName, NULL);

        /* Find the extrema.  Also add last point in nay to get
         * the last transition (from the last peak to the end).
         * The number of crossings is 1 more than the number of extrema. */
    nap = numaFindExtrema(nay, delta, NULL);
    numaAddNumber(nap, n - 1);
    np = numaGetCount(nap);
    L_INFO("Number of crossings: %d\n", procName, np);

        /* Do all computation in index units of nax or the delx of nay */
    nad = numaCreate(np);  /* output crossing locations, in nax units */
    previndex = 0;  /* prime the search with 1st point */
    numaGetFValue(nay, 0, &prevval);  /* prime the search with 1st point */
    numaGetParameters(nay, &startx, &delx);
    for (i = 0; i < np; i++) {
        numaGetIValue(nap, i, &curindex);
        numaGetFValue(nay, curindex, &curval);
        thresh = (prevval + curval) / 2.0;
        if (nax)
            numaGetFValue(nax, previndex, &xval1);
        else
            xval1 = startx + previndex * delx;
        numaGetFValue(nay, previndex, &yval1);
        for (j = previndex + 1; j <= curindex; j++) {
            if (nax)
                numaGetFValue(nax, j, &xval2);
            else
                xval2 = startx + j * delx;
            numaGetFValue(nay, j, &yval2);
            delta1 = yval1 - thresh;
            delta2 = yval2 - thresh;
            if (delta1 == 0.0) {
                numaAddNumber(nad, xval1);
                break;
            } else if (delta2 == 0.0) {
                numaAddNumber(nad, xval2);
                break;
            } else if (delta1 * delta2 < 0.0) {  /* crossing */
                fract = L_ABS(delta1) / L_ABS(yval1 - yval2);
                crossval = xval1 + fract * (xval2 - xval1);
                numaAddNumber(nad, crossval);
                break;
            }
            xval1 = xval2;
            yval1 = yval2;
        }
        previndex = curindex;
        prevval = curval;
    }

    numaDestroy(&nap);
    return nad;
}


/*!
 * \brief   numaEvalBestHaarParameters()
 *
 * \param[in]    nas         numa of non-negative signal values
 * \param[in]    relweight   relative weight of (-1 comb) / (+1 comb)
 *                           contributions to the 'convolution'.  In effect,
 *                           the convolution kernel is a comb consisting of
 *                           alternating +1 and -weight.
 * \param[in]    nwidth      number of widths to consider
 * \param[in]    nshift      number of shifts to consider for each width
 * \param[in]    minwidth    smallest width to consider
 * \param[in]    maxwidth    largest width to consider
 * \param[out]   pbestwidth  width giving largest score
 * \param[out]   pbestshift  shift giving largest score
 * \param[out]   pbestscore  [optional] convolution with "Haar"-like comb
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) This does a linear sweep of widths, evaluating at %nshift
 *          shifts for each width, computing the score from a convolution
 *          with a long comb, and finding the (width, shift) pair that
 *          gives the maximum score.  The best width is the "half-wavelength"
 *          of the signal.
 *      (2) The convolving function is a comb of alternating values
 *          +1 and -1 * relweight, separated by the width and phased by
 *          the shift.  This is similar to a Haar transform, except
 *          there the convolution is performed with a square wave.
 *      (3) The function is useful for finding the line spacing
 *          and strength of line signal from pixel sum projections.
 *      (4) The score is normalized to the size of nas divided by
 *          the number of half-widths.  For image applications, the input is
 *          typically an array of pixel projections, so one should
 *          normalize by dividing the score by the image width in the
 *          pixel projection direction.
 * </pre>
 */
l_ok
numaEvalBestHaarParameters(NUMA       *nas,
                           l_float32   relweight,
                           l_int32     nwidth,
                           l_int32     nshift,
                           l_float32   minwidth,
                           l_float32   maxwidth,
                           l_float32  *pbestwidth,
                           l_float32  *pbestshift,
                           l_float32  *pbestscore)
{
l_int32    i, j;
l_float32  delwidth, delshift, width, shift, score;
l_float32  bestwidth, bestshift, bestscore;

    PROCNAME("numaEvalBestHaarParameters");

    if (pbestscore) *pbestscore = 0.0;
    if (pbestwidth) *pbestwidth = 0.0;
    if (pbestshift) *pbestshift = 0.0;
    if (!pbestwidth || !pbestshift)
        return ERROR_INT("&bestwidth and &bestshift not defined", procName, 1);
    if (!nas)
        return ERROR_INT("nas not defined", procName, 1);

    bestscore = bestwidth = bestshift = 0.0;
    delwidth = (maxwidth - minwidth) / (nwidth - 1.0);
    for (i = 0; i < nwidth; i++) {
        width = minwidth + delwidth * i;
        delshift = width / (l_float32)(nshift);
        for (j = 0; j < nshift; j++) {
            shift = j * delshift;
            numaEvalHaarSum(nas, width, shift, relweight, &score);
            if (score > bestscore) {
                bestscore = score;
                bestwidth = width;
                bestshift = shift;
#if  DEBUG_FREQUENCY
                fprintf(stderr, "width = %7.3f, shift = %7.3f, score = %7.3f\n",
                        width, shift, score);
#endif  /* DEBUG_FREQUENCY */
            }
        }
    }

    *pbestwidth = bestwidth;
    *pbestshift = bestshift;
    if (pbestscore)
        *pbestscore = bestscore;
    return 0;
}


/*!
 * \brief   numaEvalHaarSum()
 *
 * \param[in]    nas         numa of non-negative signal values
 * \param[in]    width       distance between +1 and -1 in convolution comb
 * \param[in]    shift       phase of the comb: location of first +1
 * \param[in]    relweight   relative weight of (-1 comb) / (+1 comb)
 *                           contributions to the 'convolution'.  In effect,
 *                           the convolution kernel is a comb consisting of
 *                           alternating +1 and -weight.
 * \param[out]   pscore      convolution with "Haar"-like comb
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) This does a convolution with a comb of alternating values
 *          +1 and -relweight, separated by the width and phased by the shift.
 *          This is similar to a Haar transform, except that for Haar,
 *            (1) the convolution kernel is symmetric about 0, so the
 *                relweight is 1.0, and
 *            (2) the convolution is performed with a square wave.
 *      (2) The score is normalized to the size of nas divided by
 *          twice the "width".  For image applications, the input is
 *          typically an array of pixel projections, so one should
 *          normalize by dividing the score by the image width in the
 *          pixel projection direction.
 *      (3) To get a Haar-like result, use relweight = 1.0.  For detecting
 *          signals where you expect every other sample to be close to
 *          zero, as with barcodes or filtered text lines, you can
 *          use relweight > 1.0.
 * </pre>
 */
l_ok
numaEvalHaarSum(NUMA       *nas,
                l_float32   width,
                l_float32   shift,
                l_float32   relweight,
                l_float32  *pscore)
{
l_int32    i, n, nsamp, index;
l_float32  score, weight, val;

    PROCNAME("numaEvalHaarSum");

    if (!pscore)
        return ERROR_INT("&score not defined", procName, 1);
    *pscore = 0.0;
    if (!nas)
        return ERROR_INT("nas not defined", procName, 1);
    if ((n = numaGetCount(nas)) < 2 * width)
        return ERROR_INT("nas size too small", procName, 1);

    score = 0.0;
    nsamp = (l_int32)((n - shift) / width);
    for (i = 0; i < nsamp; i++) {
        index = (l_int32)(shift + i * width);
        weight = (i % 2) ? 1.0 : -1.0 * relweight;
        numaGetFValue(nas, index, &val);
        score += weight * val;
    }

    *pscore = 2.0 * width * score / (l_float32)n;
    return 0;
}


/*----------------------------------------------------------------------*
 *            Generating numbers in a range under constraints           *
 *----------------------------------------------------------------------*/
/*!
 * \brief   genConstrainedNumaInRange()
 *
 * \param[in]    first     first number to choose; >= 0
 * \param[in]    last      biggest possible number to reach; >= first
 * \param[in]    nmax      maximum number of numbers to select; > 0
 * \param[in]    use_pairs 1 = select pairs of adjacent numbers;
 *                         0 = select individual numbers
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *     (1) Selection is made uniformly in the range.  This can be used
 *         to select pages distributed as uniformly as possible
 *         through a book, where you are constrained to:
 *          ~ choose between [first, ... biggest],
 *          ~ choose no more than nmax numbers, and
 *         and you have the option of requiring pairs of adjacent numbers.
 * </pre>
 */
NUMA *
genConstrainedNumaInRange(l_int32  first,
                          l_int32  last,
                          l_int32  nmax,
                          l_int32  use_pairs)
{
l_int32    i, nsets, val;
l_float32  delta;
NUMA      *na;

    PROCNAME("genConstrainedNumaInRange");

    first = L_MAX(0, first);
    if (last < first)
        return (NUMA *)ERROR_PTR("last < first!", procName, NULL);
    if (nmax < 1)
        return (NUMA *)ERROR_PTR("nmax < 1!", procName, NULL);

    nsets = L_MIN(nmax, last - first + 1);
    if (use_pairs == 1)
        nsets = nsets / 2;
    if (nsets == 0)
        return (NUMA *)ERROR_PTR("nsets == 0", procName, NULL);

        /* Select delta so that selection covers the full range if possible */
    if (nsets == 1) {
        delta = 0.0;
    } else {
        if (use_pairs == 0)
            delta = (l_float32)(last - first) / (nsets - 1);
        else
            delta = (l_float32)(last - first - 1) / (nsets - 1);
    }

    na = numaCreate(nsets);
    for (i = 0; i < nsets; i++) {
        val = (l_int32)(first + i * delta + 0.5);
        numaAddNumber(na, val);
        if (use_pairs == 1)
            numaAddNumber(na, val + 1);
    }

    return na;
}
