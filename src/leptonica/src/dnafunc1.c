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
 * \file  dnafunc1.c
 * <pre>
 *
 *      Rearrangements
 *          l_int32     *l_dnaJoin()
 *          l_int32     *l_dnaaFlattenToDna()
 *
 *      Conversion between numa and dna
 *          NUMA        *l_dnaConvertToNuma()
 *          L_DNA       *numaConvertToDna()
 *
 *      Set operations using aset (rbtree)
 *          L_DNA       *l_dnaUnionByAset()
 *          L_DNA       *l_dnaRemoveDupsByAset()
 *          L_DNA       *l_dnaIntersectionByAset()
 *          L_ASET      *l_asetCreateFromDna()
 *
 *      Miscellaneous operations
 *          L_DNA       *l_dnaDiffAdjValues()
 *
 *
 * This file contains an implemention on sets of doubles (or integers)
 * that uses an underlying tree (rbtree).  The keys stored in the tree
 * are simply the double array values in the dna.  Use of a DnaHash
 * is typically more efficient, with O(1) in lookup and insertion.
 *
 * </pre>
 */

#include "allheaders.h"

/*----------------------------------------------------------------------*
 *                            Rearrangements                            *
 *----------------------------------------------------------------------*/
/*!
 * \brief   l_dnaJoin()
 *
 * \param[in]    dad  dest dna; add to this one
 * \param[in]    das  [optional] source dna; add from this one
 * \param[in]    istart  starting index in das
 * \param[in]    iend  ending index in das; use -1 to cat all
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) istart < 0 is taken to mean 'read from the start' (istart = 0)
 *      (2) iend < 0 means 'read to the end'
 *      (3) if das == NULL, this is a no-op
 * </pre>
 */
l_int32
l_dnaJoin(L_DNA   *dad,
          L_DNA   *das,
          l_int32  istart,
          l_int32  iend)
{
l_int32    n, i;
l_float64  val;

    PROCNAME("l_dnaJoin");

    if (!dad)
        return ERROR_INT("dad not defined", procName, 1);
    if (!das)
        return 0;

    if (istart < 0)
        istart = 0;
    n = l_dnaGetCount(das);
    if (iend < 0 || iend >= n)
        iend = n - 1;
    if (istart > iend)
        return ERROR_INT("istart > iend; nothing to add", procName, 1);

    for (i = istart; i <= iend; i++) {
        l_dnaGetDValue(das, i, &val);
        l_dnaAddNumber(dad, val);
    }

    return 0;
}


/*!
 * \brief   l_dnaaFlattenToDna()
 *
 * \param[in]    daa
 * \return  dad, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) This 'flattens' the dnaa to a dna, by joining successively
 *          each dna in the dnaa.
 *      (2) It leaves the input dnaa unchanged.
 * </pre>
 */
L_DNA *
l_dnaaFlattenToDna(L_DNAA  *daa)
{
l_int32  i, nalloc;
L_DNA   *da, *dad;
L_DNA  **array;

    PROCNAME("l_dnaaFlattenToDna");

    if (!daa)
        return (L_DNA *)ERROR_PTR("daa not defined", procName, NULL);

    nalloc = daa->nalloc;
    array = daa->dna;
    dad = l_dnaCreate(0);
    for (i = 0; i < nalloc; i++) {
        da = array[i];
        if (!da) continue;
        l_dnaJoin(dad, da, 0, -1);
    }

    return dad;
}


/*----------------------------------------------------------------------*
 *                   Conversion between numa and dna                    *
 *----------------------------------------------------------------------*/
/*!
 * \brief   l_dnaConvertToNuma()
 *
 * \param[in]    da
 * \return  na, or NULL on error
 */
NUMA *
l_dnaConvertToNuma(L_DNA  *da)
{
l_int32    i, n;
l_float64  val;
NUMA      *na;

    PROCNAME("l_dnaConvertToNuma");

    if (!da)
        return (NUMA *)ERROR_PTR("da not defined", procName, NULL);

    n = l_dnaGetCount(da);
    na = numaCreate(n);
    for (i = 0; i < n; i++) {
        l_dnaGetDValue(da, i, &val);
        numaAddNumber(na, val);
    }
    return na;
}


/*!
 * \brief   numaConvertToDna
 *
 * \param[in]    na
 * \return  da, or NULL on error
 */
L_DNA *
numaConvertToDna(NUMA  *na)
{
l_int32    i, n;
l_float32  val;
L_DNA     *da;

    PROCNAME("numaConvertToDna");

    if (!na)
        return (L_DNA *)ERROR_PTR("na not defined", procName, NULL);

    n = numaGetCount(na);
    da = l_dnaCreate(n);
    for (i = 0; i < n; i++) {
        numaGetFValue(na, i, &val);
        l_dnaAddNumber(da, val);
    }
    return da;
}


/*----------------------------------------------------------------------*
 *                   Set operations using aset (rbtree)                 *
 *----------------------------------------------------------------------*/
/*!
 * \brief   l_dnaUnionByAset()
 *
 * \param[in]    da1, da2
 * \return  dad with the union of the set of numbers, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) See sarrayUnionByAset() for the approach.
 *      (2) Here, the key in building the sorted tree is the number itself.
 *      (3) Operations using an underlying tree are O(nlogn), which is
 *          typically less efficient than hashing, which is O(n).
 * </pre>
 */
L_DNA *
l_dnaUnionByAset(L_DNA  *da1,
                 L_DNA  *da2)
{
L_DNA  *da3, *dad;

    PROCNAME("l_dnaUnionByAset");

    if (!da1)
        return (L_DNA *)ERROR_PTR("da1 not defined", procName, NULL);
    if (!da2)
        return (L_DNA *)ERROR_PTR("da2 not defined", procName, NULL);

        /* Join */
    da3 = l_dnaCopy(da1);
    l_dnaJoin(da3, da2, 0, -1);

        /* Eliminate duplicates */
    dad = l_dnaRemoveDupsByAset(da3);
    l_dnaDestroy(&da3);
    return dad;
}


/*!
 * \brief   l_dnaRemoveDupsByAset()
 *
 * \param[in]    das
 * \return  dad with duplicates removed, or NULL on error
 */
L_DNA *
l_dnaRemoveDupsByAset(L_DNA  *das)
{
l_int32    i, n;
l_float64  val;
L_DNA     *dad;
L_ASET    *set;
RB_TYPE    key;

    PROCNAME("l_dnaRemoveDupsByAset");

    if (!das)
        return (L_DNA *)ERROR_PTR("das not defined", procName, NULL);

    set = l_asetCreate(L_FLOAT_TYPE);
    dad = l_dnaCreate(0);
    n = l_dnaGetCount(das);
    for (i = 0; i < n; i++) {
        l_dnaGetDValue(das, i, &val);
        key.ftype = val;
        if (!l_asetFind(set, key)) {
            l_dnaAddNumber(dad, val);
            l_asetInsert(set, key);
        }
    }

    l_asetDestroy(&set);
    return dad;
}


/*!
 * \brief   l_dnaIntersectionByAset()
 *
 * \param[in]    da1, da2
 * \return  dad with the intersection of the two arrays, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) See sarrayIntersection() for the approach.
 *      (2) Here, the key in building the sorted tree is the number itself.
 *      (3) Operations using an underlying tree are O(nlogn), which is
 *          typically less efficient than hashing, which is O(n).
 * </pre>
 */
L_DNA *
l_dnaIntersectionByAset(L_DNA  *da1,
                        L_DNA  *da2)
{
l_int32    n1, n2, i, n;
l_float64  val;
L_ASET    *set1, *set2;
RB_TYPE    key;
L_DNA     *da_small, *da_big, *dad;

    PROCNAME("l_dnaIntersectionByAset");

    if (!da1)
        return (L_DNA *)ERROR_PTR("da1 not defined", procName, NULL);
    if (!da2)
        return (L_DNA *)ERROR_PTR("da2 not defined", procName, NULL);

        /* Put the elements of the largest array into a set */
    n1 = l_dnaGetCount(da1);
    n2 = l_dnaGetCount(da2);
    da_small = (n1 < n2) ? da1 : da2;   /* do not destroy da_small */
    da_big = (n1 < n2) ? da2 : da1;   /* do not destroy da_big */
    set1 = l_asetCreateFromDna(da_big);

        /* Build up the intersection of floats */
    dad = l_dnaCreate(0);
    n = l_dnaGetCount(da_small);
    set2 = l_asetCreate(L_FLOAT_TYPE);
    for (i = 0; i < n; i++) {
        l_dnaGetDValue(da_small, i, &val);
        key.ftype = val;
        if (l_asetFind(set1, key) && !l_asetFind(set2, key)) {
            l_dnaAddNumber(dad, val);
            l_asetInsert(set2, key);
        }
    }

    l_asetDestroy(&set1);
    l_asetDestroy(&set2);
    return dad;
}


/*!
 * \brief   l_asetCreateFromDna()
 *
 * \param[in]    da source dna
 * \return  set using the doubles in %da as keys
 */
L_ASET *
l_asetCreateFromDna(L_DNA  *da)
{
l_int32    i, n;
l_float64  val;
L_ASET    *set;
RB_TYPE    key;

    PROCNAME("l_asetCreateFromDna");

    if (!da)
        return (L_ASET *)ERROR_PTR("da not defined", procName, NULL);

    set = l_asetCreate(L_FLOAT_TYPE);
    n = l_dnaGetCount(da);
    for (i = 0; i < n; i++) {
        l_dnaGetDValue(da, i, &val);
        key.ftype = val;
        l_asetInsert(set, key);
    }

    return set;
}


/*----------------------------------------------------------------------*
 *                       Miscellaneous operations                       *
 *----------------------------------------------------------------------*/
/*!
 * \brief   l_dnaDiffAdjValues()
 *
 * \param[in]    das input l_dna
 * \return  dad of difference values val[i+1] - val[i],
 *                   or NULL on error
 */
L_DNA *
l_dnaDiffAdjValues(L_DNA  *das)
{
l_int32  i, n, prev, cur;
L_DNA   *dad;

    PROCNAME("l_dnaDiffAdjValues");

    if (!das)
        return (L_DNA *)ERROR_PTR("das not defined", procName, NULL);
    n = l_dnaGetCount(das);
    dad = l_dnaCreate(n - 1);
    prev = 0;
    for (i = 1; i < n; i++) {
        l_dnaGetIValue(das, i, &cur);
        l_dnaAddNumber(dad, cur - prev);
        prev = cur;
    }
    return dad;
}

