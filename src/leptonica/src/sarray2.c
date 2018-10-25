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
 * \file  sarray2.c
 * <pre>
 *
 *      Sort
 *          SARRAY     *sarraySort()
 *          SARRAY     *sarraySortByIndex()
 *          l_int32     stringCompareLexical()
 *
 *      Set operations using aset (rbtree)
 *          SARRAY     *sarrayUnionByAset()
 *          SARRAY     *sarrayRemoveDupsByAset()
 *          SARRAY     *sarrayIntersectionByAset()
 *          L_ASET     *l_asetCreateFromSarray()
 *
 *      Set operations using hashing (dnahash)
 *          l_int32     sarrayRemoveDupsByHash()
 *          SARRAY     *sarrayIntersectionByHash()
 *          l_int32     sarrayFindStringByHash()
 *          L_DNAHASH  *l_dnaHashCreateFromSarray()
 *
 *      Miscellaneous operations
 *          SARRAY     *sarrayGenerateIntegers()
 *          l_int32     sarrayLookupCSKV()
 *
 *
 * We have two implementations of set operations on an array of strings:
 *
 *   (1) Using an underlying tree (rbtree)
 *       This uses a good 64 bit hashing function for the key,
 *       that is not expected to have hash collisions (and we do
 *       not test for them).  The tree is built up of the hash
 *       values, and if the hash is found in the tree, it is
 *       assumed that the string has already been found.
 *
 *   (2) Using an underlying hashing of the keys (dnahash)
 *       This uses a fast 64 bit hashing function for the key,
 *       which is then hashed into a bucket (a dna in a dnaHash).
 *       Because hash collisions can occur, the index into the
 *       sarray for the string that gave rise to that key is stored,
 *       and the dna (bucket) is traversed, using the stored indices
 *       to determine if that string had already been seen.
 *
 * </pre>
 */

#include <string.h>
#include "allheaders.h"

/*----------------------------------------------------------------------*
 *                                   Sort                               *
 *----------------------------------------------------------------------*/
/*!
 * \brief   sarraySort()
 *
 * \param[in]    saout output sarray; can be NULL or equal to sain
 * \param[in]    sain input sarray
 * \param[in]    sortorder L_SORT_INCREASING or L_SORT_DECREASING
 * \return  saout output sarray, sorted by ascii value, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) Set saout = sain for in-place; otherwise, set naout = NULL.
 *      (2) Shell sort, modified from K&R, 2nd edition, p.62.
 *          Slow but simple O(n logn) sort.
 * </pre>
 */
SARRAY *
sarraySort(SARRAY  *saout,
           SARRAY  *sain,
           l_int32  sortorder)
{
char   **array;
char    *tmp;
l_int32  n, i, j, gap;

    PROCNAME("sarraySort");

    if (!sain)
        return (SARRAY *)ERROR_PTR("sain not defined", procName, NULL);

        /* Make saout if necessary; otherwise do in-place */
    if (!saout)
        saout = sarrayCopy(sain);
    else if (sain != saout)
        return (SARRAY *)ERROR_PTR("invalid: not in-place", procName, NULL);
    array = saout->array;  /* operate directly on the array */
    n = sarrayGetCount(saout);

        /* Shell sort */
    for (gap = n/2; gap > 0; gap = gap / 2) {
        for (i = gap; i < n; i++) {
            for (j = i - gap; j >= 0; j -= gap) {
                if ((sortorder == L_SORT_INCREASING &&
                     stringCompareLexical(array[j], array[j + gap])) ||
                    (sortorder == L_SORT_DECREASING &&
                     stringCompareLexical(array[j + gap], array[j])))
                {
                    tmp = array[j];
                    array[j] = array[j + gap];
                    array[j + gap] = tmp;
                }
            }
        }
    }

    return saout;
}


/*!
 * \brief   sarraySortByIndex()
 *
 * \param[in]    sain
 * \param[in]    naindex na that maps from the new sarray to the input sarray
 * \return  saout sorted, or NULL on error
 */
SARRAY *
sarraySortByIndex(SARRAY  *sain,
                  NUMA    *naindex)
{
char    *str;
l_int32  i, n, index;
SARRAY  *saout;

    PROCNAME("sarraySortByIndex");

    if (!sain)
        return (SARRAY *)ERROR_PTR("sain not defined", procName, NULL);
    if (!naindex)
        return (SARRAY *)ERROR_PTR("naindex not defined", procName, NULL);

    n = sarrayGetCount(sain);
    saout = sarrayCreate(n);
    for (i = 0; i < n; i++) {
        numaGetIValue(naindex, i, &index);
        str = sarrayGetString(sain, index, L_COPY);
        sarrayAddString(saout, str, L_INSERT);
    }

    return saout;
}


/*!
 * \brief   stringCompareLexical()
 *
 * \param[in]    str1
 * \param[in]    str2
 * \return  1 if str1 > str2 lexically; 0 otherwise
 *
 * <pre>
 * Notes:
 *      (1) If the lexical values are identical, return a 0, to
 *          indicate that no swapping is required to sort the strings.
 * </pre>
 */
l_int32
stringCompareLexical(const char *str1,
                     const char *str2)
{
l_int32  i, len1, len2, len;

    PROCNAME("sarrayCompareLexical");

    if (!str1)
        return ERROR_INT("str1 not defined", procName, 1);
    if (!str2)
        return ERROR_INT("str2 not defined", procName, 1);

    len1 = strlen(str1);
    len2 = strlen(str2);
    len = L_MIN(len1, len2);

    for (i = 0; i < len; i++) {
        if (str1[i] == str2[i])
            continue;
        if (str1[i] > str2[i])
            return 1;
        else
            return 0;
    }

    if (len1 > len2)
        return 1;
    else
        return 0;
}


/*----------------------------------------------------------------------*
 *                   Set operations using aset (rbtree)                 *
 *----------------------------------------------------------------------*/
/*!
 * \brief   sarrayUnionByAset()
 *
 * \param[in]    sa1, sa2
 * \return  sad with the union of the string set, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) Duplicates are removed from the concatenation of the two arrays.
 *      (2) The key for each string is a 64-bit hash.
 *      (2) Algorithm: Concatenate the two sarrays.  Then build a set,
 *          using hashed strings as keys.  As the set is built, first do
 *          a find; if not found, add the key to the set and add the string
 *          to the output sarray.  This is O(nlogn).
 * </pre>
 */
SARRAY *
sarrayUnionByAset(SARRAY  *sa1,
                  SARRAY  *sa2)
{
SARRAY  *sa3, *sad;

    PROCNAME("sarrayUnionByAset");

    if (!sa1)
        return (SARRAY *)ERROR_PTR("sa1 not defined", procName, NULL);
    if (!sa2)
        return (SARRAY *)ERROR_PTR("sa2 not defined", procName, NULL);

        /* Join */
    sa3 = sarrayCopy(sa1);
    sarrayJoin(sa3, sa2);

        /* Eliminate duplicates */
    sad = sarrayRemoveDupsByAset(sa3);
    sarrayDestroy(&sa3);
    return sad;
}


/*!
 * \brief   sarrayRemoveDupsByAset()
 *
 * \param[in]    sas
 * \return  sad with duplicates removed, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) This is O(nlogn), considerably slower than
 *          sarrayRemoveDupsByHash() for large string arrays.
 *      (2) The key for each string is a 64-bit hash.
 *      (3) Build a set, using hashed strings as keys.  As the set is
 *          built, first do a find; if not found, add the key to the
 *          set and add the string to the output sarray.
 * </pre>
 */
SARRAY *
sarrayRemoveDupsByAset(SARRAY  *sas)
{
char     *str;
l_int32   i, n;
l_uint64  hash;
L_ASET   *set;
RB_TYPE   key;
SARRAY   *sad;

    PROCNAME("sarrayRemoveDupsByAset");

    if (!sas)
        return (SARRAY *)ERROR_PTR("sas not defined", procName, NULL);

    set = l_asetCreate(L_UINT_TYPE);
    sad = sarrayCreate(0);
    n = sarrayGetCount(sas);
    for (i = 0; i < n; i++) {
        str = sarrayGetString(sas, i, L_NOCOPY);
        l_hashStringToUint64(str, &hash);
        key.utype = hash;
        if (!l_asetFind(set, key)) {
            sarrayAddString(sad, str, L_COPY);
            l_asetInsert(set, key);
        }
    }

    l_asetDestroy(&set);
    return sad;
}


/*!
 * \brief   sarrayIntersectionByAset()
 *
 * \param[in]    sa1, sa2
 * \return  sad with the intersection of the string set, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) Algorithm: put the larger sarray into a set, using the string
 *          hashes as the key values.  Then run through the smaller sarray,
 *          building an output sarray and a second set from the strings
 *          in the larger array: if a string is in the first set but
 *          not in the second, add the string to the output sarray and hash
 *          it into the second set.  The second set is required to make
 *          sure only one instance of each string is put into the output sarray.
 *          This is O(mlogn), {m,n} = sizes of {smaller,larger} input arrays.
 * </pre>
 */
SARRAY *
sarrayIntersectionByAset(SARRAY  *sa1,
                         SARRAY  *sa2)
{
char     *str;
l_int32   n1, n2, i, n;
l_uint64  hash;
L_ASET   *set1, *set2;
RB_TYPE   key;
SARRAY   *sa_small, *sa_big, *sad;

    PROCNAME("sarrayIntersectionByAset");

    if (!sa1)
        return (SARRAY *)ERROR_PTR("sa1 not defined", procName, NULL);
    if (!sa2)
        return (SARRAY *)ERROR_PTR("sa2 not defined", procName, NULL);

        /* Put the elements of the biggest array into a set */
    n1 = sarrayGetCount(sa1);
    n2 = sarrayGetCount(sa2);
    sa_small = (n1 < n2) ? sa1 : sa2;   /* do not destroy sa_small */
    sa_big = (n1 < n2) ? sa2 : sa1;   /* do not destroy sa_big */
    set1 = l_asetCreateFromSarray(sa_big);

        /* Build up the intersection of strings */
    sad = sarrayCreate(0);
    n = sarrayGetCount(sa_small);
    set2 = l_asetCreate(L_UINT_TYPE);
    for (i = 0; i < n; i++) {
        str = sarrayGetString(sa_small, i, L_NOCOPY);
        l_hashStringToUint64(str, &hash);
        key.utype = hash;
        if (l_asetFind(set1, key) && !l_asetFind(set2, key)) {
            sarrayAddString(sad, str, L_COPY);
            l_asetInsert(set2, key);
        }
    }

    l_asetDestroy(&set1);
    l_asetDestroy(&set2);
    return sad;
}


/*!
 * \brief   l_asetCreateFromSarray()
 *
 * \param[in]    sa
 * \return  set using a string hash into a uint64 as the key
 */
L_ASET *
l_asetCreateFromSarray(SARRAY  *sa)
{
char     *str;
l_int32   i, n;
l_uint64  hash;
L_ASET   *set;
RB_TYPE   key;

    PROCNAME("l_asetCreateFromSarray");

    if (!sa)
        return (L_ASET *)ERROR_PTR("sa not defined", procName, NULL);

    set = l_asetCreate(L_UINT_TYPE);
    n = sarrayGetCount(sa);
    for (i = 0; i < n; i++) {
        str = sarrayGetString(sa, i, L_NOCOPY);
        l_hashStringToUint64(str, &hash);
        key.utype = hash;
        l_asetInsert(set, key);
    }

    return set;
}


/*----------------------------------------------------------------------*
 *               Set operations using hashing (dnahash)                 *
 *----------------------------------------------------------------------*/
/*!
 * \brief   sarrayRemoveDupsByHash()
 *
 * \param[in]    sas
 * \param[out]   psad unique set of strings; duplicates removed
 * \param[out]   pdahash [optional] dnahash used for lookup
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) Generates a sarray with unique values.
 *      (2) The dnahash is built up with sad to assure uniqueness.
 *          It can be used to find if a string is in the set:
 *              sarrayFindValByHash(sad, dahash, str, &index)
 *      (3) The hash of the string location is simple and fast.  It scales
 *          up with the number of buckets to insure a fairly random
 *          bucket selection input strings.
 *      (4) This is faster than sarrayRemoveDupsByAset(), because the
 *          bucket lookup is O(n), although there is a double-loop
 *          lookup within the dna in each bucket.
 * </pre>
 */
l_int32
sarrayRemoveDupsByHash(SARRAY      *sas,
                       SARRAY     **psad,
                       L_DNAHASH  **pdahash)
{
char       *str;
l_int32     i, n, index, items;
l_uint32    nsize;
l_uint64    key;
SARRAY     *sad;
L_DNAHASH  *dahash;

    PROCNAME("sarrayRemoveDupsByHash");

    if (pdahash) *pdahash = NULL;
    if (!psad)
        return ERROR_INT("&sad not defined", procName, 1);
    *psad = NULL;
    if (!sas)
        return ERROR_INT("sas not defined", procName, 1);

    n = sarrayGetCount(sas);
    findNextLargerPrime(n / 20, &nsize);  /* buckets in hash table */
    dahash = l_dnaHashCreate(nsize, 8);
    sad = sarrayCreate(n);
    *psad = sad;
    for (i = 0, items = 0; i < n; i++) {
        str = sarrayGetString(sas, i, L_NOCOPY);
        sarrayFindStringByHash(sad, dahash, str, &index);
        if (index < 0) {  /* not found */
            l_hashStringToUint64(str, &key);
            l_dnaHashAdd(dahash, key, (l_float64)items);
            sarrayAddString(sad, str, L_COPY);
            items++;
        }
    }

    if (pdahash)
        *pdahash = dahash;
    else
        l_dnaHashDestroy(&dahash);
    return 0;
}


/*!
 * \brief   sarrayIntersectionByHash()
 *
 * \param[in]    sa1, sa2
 * \return  sad intersection of the strings, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) This is faster than sarrayIntersectionByAset(), because the
 *          bucket lookup is O(n).
 * </pre>
 */
SARRAY *
sarrayIntersectionByHash(SARRAY  *sa1,
                         SARRAY  *sa2)
{
char       *str;
l_int32     n1, n2, nsmall, i, index1, index2;
l_uint32    nsize2;
l_uint64    key;
L_DNAHASH  *dahash1, *dahash2;
SARRAY     *sa_small, *sa_big, *sad;

    PROCNAME("sarrayIntersectionByHash");

    if (!sa1)
        return (SARRAY *)ERROR_PTR("sa1 not defined", procName, NULL);
    if (!sa2)
        return (SARRAY *)ERROR_PTR("sa2 not defined", procName, NULL);

        /* Put the elements of the biggest sarray into a dnahash */
    n1 = sarrayGetCount(sa1);
    n2 = sarrayGetCount(sa2);
    sa_small = (n1 < n2) ? sa1 : sa2;   /* do not destroy sa_small */
    sa_big = (n1 < n2) ? sa2 : sa1;   /* do not destroy sa_big */
    dahash1 = l_dnaHashCreateFromSarray(sa_big);

        /* Build up the intersection of strings.  Add to %sad
         * if the string is in sa_big (using dahash1) but hasn't
         * yet been seen in the traversal of sa_small (using dahash2). */
    sad = sarrayCreate(0);
    nsmall = sarrayGetCount(sa_small);
    findNextLargerPrime(nsmall / 20, &nsize2);  /* buckets in hash table */
    dahash2 = l_dnaHashCreate(nsize2, 0);
    for (i = 0; i < nsmall; i++) {
        str = sarrayGetString(sa_small, i, L_NOCOPY);
        sarrayFindStringByHash(sa_big, dahash1, str, &index1);
        if (index1 >= 0) {
            sarrayFindStringByHash(sa_small, dahash2, str, &index2);
            if (index2 == -1) {
                sarrayAddString(sad, str, L_COPY);
                l_hashStringToUint64(str, &key);
                l_dnaHashAdd(dahash2, key, (l_float64)i);
            }
        }
    }

    l_dnaHashDestroy(&dahash1);
    l_dnaHashDestroy(&dahash2);
    return sad;
}


/*!
 * \brief   sarrayFindStringByHash()
 *
 * \param[in]    sa
 * \param[in]    dahash built from sa
 * \param[in]    str  arbitrary string
 * \param[out]   pindex index into %sa if %str is in %sa;
 *              -1 otherwise
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) Fast lookup in dnaHash associated with a sarray, to see if a
 *          random string %str is already stored in the hash table.
 *      (2) We use a strong hash function to minimize the chance that
 *          two different strings hash to the same key value.
 *      (3) We select the number of buckets to be about 5% of the size
 *          of the input sarray, so that when fully populated, each
 *          bucket (dna) will have about 20 entries, each being an index
 *          into sa.  In lookup, after hashing to the key, and then
 *          again to the bucket, we traverse the bucket (dna), using the
 *          index into sa to check if %str has been found before.
 * </pre>
 */
l_int32
sarrayFindStringByHash(SARRAY      *sa,
                       L_DNAHASH   *dahash,
                       const char  *str,
                       l_int32     *pindex)
{
char     *stri;
l_int32   i, nvals, index;
l_uint64  key;
L_DNA    *da;

    PROCNAME("sarrayFindStringByHash");

    if (!pindex)
        return ERROR_INT("&index not defined", procName, 1);
    *pindex = -1;
    if (!sa)
        return ERROR_INT("sa not defined", procName, 1);
    if (!dahash)
        return ERROR_INT("dahash not defined", procName, 1);

    l_hashStringToUint64(str, &key);
    da = l_dnaHashGetDna(dahash, key, L_NOCOPY);
    if (!da) return 0;

        /* Run through the da, looking for this string */
    nvals = l_dnaGetCount(da);
    for (i = 0; i < nvals; i++) {
        l_dnaGetIValue(da, i, &index);
        stri = sarrayGetString(sa, index, L_NOCOPY);
        if (!strcmp(str, stri)) {  /* duplicate */
            *pindex = index;
            return 0;
        }
    }

    return 0;
}


/*!
 * \brief   l_dnaHashCreateFromSarray()
 *
 * \param[in]    sa
 * \return  dahash, or NULL on error
 */
L_DNAHASH *
l_dnaHashCreateFromSarray(SARRAY  *sa)
{
char       *str;
l_int32     i, n;
l_uint32    nsize;
l_uint64    key;
L_DNAHASH  *dahash;

        /* Build up dnaHash of indices, hashed by a 64-bit key that
         * should randomize the lower bits used in bucket selection.
         * Having about 20 pts in each bucket is roughly optimal. */
    n = sarrayGetCount(sa);
    findNextLargerPrime(n / 20, &nsize);  /* buckets in hash table */
/*    fprintf(stderr, "Prime used: %d\n", nsize); */

        /* Add each string, using the hash as key and the index into %sa
         * as the value.  Storing the index enables operations that check
         * for duplicates.  */
    dahash = l_dnaHashCreate(nsize, 8);
    for (i = 0; i < n; i++) {
        str = sarrayGetString(sa, i, L_NOCOPY);
        l_hashStringToUint64(str, &key);
        l_dnaHashAdd(dahash, key, (l_float64)i);
    }

    return dahash;
}


/*----------------------------------------------------------------------*
 *                      Miscellaneous operations                        *
 *----------------------------------------------------------------------*/
/*!
 * \brief   sarrayGenerateIntegers()
 *
 * \param[in]   n
 * \return  sa  (of printed numbers, 1 - n, or NULL on error
 */
SARRAY *
sarrayGenerateIntegers(l_int32  n)
{
char     buf[32];
l_int32  i;
SARRAY  *sa;

    PROCNAME("sarrayGenerateIntegers");

    if ((sa = sarrayCreate(n)) == NULL)
        return (SARRAY *)ERROR_PTR("sa not made", procName, NULL);
    for (i = 0; i < n; i++) {
        snprintf(buf, sizeof(buf), "%d", i);
        sarrayAddString(sa, buf, L_COPY);
    }
    return sa;
}


/*!
 * \brief   sarrayLookupCSKV()
 *
 * \param[in]    sa (of strings, each being a comma-separated pair of strings,
 *                  the first being a key and the second a value)
 * \param[in]    keystring (an input string to match with each key in %sa
 * \param[out]   pvalstring (the returned value string corresponding to the
 *                           input key string, if found; otherwise NULL)
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) The input %sa can have other strings that are not in
 *          comma-separated key-value format.  These will be ignored.
 *      (2) This returns a copy of the first value string in %sa whose
 *          key string matches the input %keystring.
 *      (3) White space is not ignored; all white space before the ','
 *          is used for the keystring in matching.  This allows the
 *          key and val strings to have white space (e.g., multiple words).
 * </pre>
 */
l_int32
sarrayLookupCSKV(SARRAY      *sa,
                 const char  *keystring,
                 char       **pvalstring)
{
char    *key, *val, *str;
l_int32  i, n;
SARRAY  *sa1;

    PROCNAME("sarrayLookupCSKV");

    if (!pvalstring)
        return ERROR_INT("&valstring not defined", procName, 1);
    *pvalstring = NULL;
    if (!sa)
        return ERROR_INT("sa not defined", procName, 1);
    if (!keystring)
        return ERROR_INT("keystring not defined", procName, 1);

    n = sarrayGetCount(sa);
    for (i = 0; i < n; i++) {
        str = sarrayGetString(sa, i, L_NOCOPY);
        sa1 = sarrayCreate(2);
        sarraySplitString(sa1, str, ",");
        if (sarrayGetCount(sa1) != 2) {
            sarrayDestroy(&sa1);
            continue;
        }
        key = sarrayGetString(sa1, 0, L_NOCOPY);
        val = sarrayGetString(sa1, 1, L_NOCOPY);
        if (!strcmp(key, keystring)) {
            *pvalstring = stringNew(val);
            sarrayDestroy(&sa1);
            return 0;
        }
        sarrayDestroy(&sa1);
    }

    return 0;
}
