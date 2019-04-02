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
 * \file utils1.c
 * <pre>
 *
 *       ------------------------------------------
 *       This file has these utilities:
 *         - error, warning and info messages
 *         - low-level endian conversions
 *         - file corruption operations
 *         - random and prime number operations
 *         - 64-bit hash functions
 *         - leptonica version number accessor
 *         - timing and date operations
 *       ------------------------------------------
 *
 *       Control of error, warning and info messages
 *           l_int32    setMsgSeverity()
 *
 *       Error return functions, invoked by macros
 *           l_int32    returnErrorInt()
 *           l_float32  returnErrorFloat()
 *           void      *returnErrorPtr()
 *
 *       Test files for equivalence
 *           l_int32    filesAreIdentical()
 *
 *       Byte-swapping data conversion
 *           l_uint16   convertOnBigEnd16()
 *           l_uint32   convertOnBigEnd32()
 *           l_uint16   convertOnLittleEnd16()
 *           l_uint32   convertOnLittleEnd32()
 *
 *       File corruption operation
 *           l_int32    fileCorruptByDeletion()
 *           l_int32    fileCorruptByMutation()
 *
 *       Generate random integer in given range
 *           l_int32    genRandomIntegerInRange()
 *
 *       Simple math function
 *           l_int32    lept_roundftoi()
 *
 *       64-bit hash functions
 *           l_int32    l_hashStringToUint64()
 *           l_int32    l_hashPtToUint64()
 *           l_int32    l_hashFloat64ToUint64()
 *
 *       Prime finders
 *           l_int32    findNextLargerPrime()
 *           l_int32    lept_isPrime()
 *
 *       Gray code conversion
 *           l_uint32   convertIntToGrayCode()
 *           l_uint32   convertGrayCodeToInt()
 *
 *       Leptonica version number
 *           char      *getLeptonicaVersion()
 *
 *       Timing
 *           void       startTimer()
 *           l_float32  stopTimer()
 *           L_TIMER    startTimerNested()
 *           l_float32  stopTimerNested()
 *           void       l_getCurrentTime()
 *           L_WALLTIMER  *startWallTimer()
 *           l_float32  stopWallTimer()
 *           void       l_getFormattedDate()
 *
 *  For all issues with cross-platform development, see utils2.c.
 * </pre>
 */

#ifdef HAVE_CONFIG_H
#include "config_auto.h"
#endif  /* HAVE_CONFIG_H */

#ifdef _WIN32
#include <windows.h>
#endif  /* _WIN32 */

#include <time.h>
#include "allheaders.h"
#include <math.h>

    /* Global for controlling message output at runtime */
LEPT_DLL l_int32  LeptMsgSeverity = DEFAULT_SEVERITY;

#define  DEBUG_SEV     0

/*----------------------------------------------------------------------*
 *                Control of error, warning and info messages           *
 *----------------------------------------------------------------------*/
/*!
 * \brief   setMsgSeverity()
 *
 * \param[in]    newsev
 * \return  oldsev
 *
 * <pre>
 * Notes:
 *      (1) setMsgSeverity() allows the user to specify the desired
 *          message severity threshold.  Messages of equal or greater
 *          severity will be output.  The previous message severity is
 *          returned when the new severity is set.
 *      (2) If L_SEVERITY_EXTERNAL is passed, then the severity will be
 *          obtained from the LEPT_MSG_SEVERITY environment variable.
 * </pre>
 */
l_int32
setMsgSeverity(l_int32  newsev)
{
l_int32  oldsev;
char    *envsev;

    PROCNAME("setMsgSeverity");

    oldsev = LeptMsgSeverity;
    if (newsev == L_SEVERITY_EXTERNAL) {
        envsev = getenv("LEPT_MSG_SEVERITY");
        if (envsev) {
            LeptMsgSeverity = atoi(envsev);
#if DEBUG_SEV
            L_INFO("message severity set to external\n", procName);
#endif  /* DEBUG_SEV */
        } else {
#if DEBUG_SEV
            L_WARNING("environment var LEPT_MSG_SEVERITY not defined\n",
                      procName);
#endif  /* DEBUG_SEV */
        }
    } else {
        LeptMsgSeverity = newsev;
#if DEBUG_SEV
        L_INFO("message severity set to %d\n", procName, newsev);
#endif  /* DEBUG_SEV */
    }

    return oldsev;
}


/*----------------------------------------------------------------------*
 *                Error return functions, invoked by macros             *
 *                                                                      *
 *    (1) These error functions print messages to stderr and allow      *
 *        exit from the function that called them.                      *
 *    (2) They must be invoked only by the macros ERROR_INT,            *
 *        ERROR_FLOAT and ERROR_PTR, which are in environ.h             *
 *    (3) The print output can be disabled at compile time, either      *
 *        by using -DNO_CONSOLE_IO or by setting LeptMsgSeverity.       *
 *----------------------------------------------------------------------*/
/*!
 * \brief   returnErrorInt()
 *
 * \param[in]    msg        error message
 * \param[in]    procname
 * \param[in]    ival       return error val
 * \return  ival typically 1 for an error return
 */
l_int32
returnErrorInt(const char  *msg,
               const char  *procname,
               l_int32      ival)
{
    fprintf(stderr, "Error in %s: %s\n", procname, msg);
    return ival;
}


/*!
 * \brief   returnErrorFloat()
 *
 * \param[in]    msg        error message
 * \param[in]    procname
 * \param[in]    fval       return error val
 * \return  fval
 */
l_float32
returnErrorFloat(const char  *msg,
                 const char  *procname,
                 l_float32    fval)
{
    fprintf(stderr, "Error in %s: %s\n", procname, msg);
    return fval;
}


/*!
 * \brief   returnErrorPtr()
 *
 * \param[in]    msg        error message
 * \param[in]    procname
 * \param[in]    pval       return error val
 * \return  pval  typically null for an error return
 */
void *
returnErrorPtr(const char  *msg,
               const char  *procname,
               void        *pval)
{
    fprintf(stderr, "Error in %s: %s\n", procname, msg);
    return pval;
}


/*--------------------------------------------------------------------*
 *                      Test files for equivalence                    *
 *--------------------------------------------------------------------*/
/*!
 * \brief   filesAreIdentical()
 *
 * \param[in]    fname1
 * \param[in]    fname2
 * \param[out]   psame     1 if identical; 0 if different
 * \return  0 if OK, 1 on error
 */
l_ok
filesAreIdentical(const char  *fname1,
                  const char  *fname2,
                  l_int32     *psame)
{
l_int32   i, same;
size_t    nbytes1, nbytes2;
l_uint8  *array1, *array2;

    PROCNAME("filesAreIdentical");

    if (!psame)
        return ERROR_INT("&same not defined", procName, 1);
    *psame = 0;
    if (!fname1 || !fname2)
        return ERROR_INT("both names not defined", procName, 1);

    nbytes1 = nbytesInFile(fname1);
    nbytes2 = nbytesInFile(fname2);
    if (nbytes1 != nbytes2)
        return 0;

    if ((array1 = l_binaryRead(fname1, &nbytes1)) == NULL)
        return ERROR_INT("array1 not read", procName, 1);
    if ((array2 = l_binaryRead(fname2, &nbytes2)) == NULL) {
        LEPT_FREE(array1);
        return ERROR_INT("array2 not read", procName, 1);
    }
    same = 1;
    for (i = 0; i < nbytes1; i++) {
        if (array1[i] != array2[i]) {
            same = 0;
            break;
        }
    }
    LEPT_FREE(array1);
    LEPT_FREE(array2);
    *psame = same;

    return 0;
}


/*--------------------------------------------------------------------------*
 *   16 and 32 bit byte-swapping on big endian and little  endian machines  *
 *                                                                          *
 *   These are typically used for I/O conversions:                          *
 *      (1) endian conversion for data that was read from a file            *
 *      (2) endian conversion on data before it is written to a file        *
 *--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------*
 *                        16-bit byte swapping                        *
 *--------------------------------------------------------------------*/
#ifdef L_BIG_ENDIAN

l_uint16
convertOnBigEnd16(l_uint16  shortin)
{
    return ((shortin << 8) | (shortin >> 8));
}

l_uint16
convertOnLittleEnd16(l_uint16  shortin)
{
    return  shortin;
}

#else     /* L_LITTLE_ENDIAN */

l_uint16
convertOnLittleEnd16(l_uint16  shortin)
{
    return ((shortin << 8) | (shortin >> 8));
}

l_uint16
convertOnBigEnd16(l_uint16  shortin)
{
    return  shortin;
}

#endif  /* L_BIG_ENDIAN */


/*--------------------------------------------------------------------*
 *                        32-bit byte swapping                        *
 *--------------------------------------------------------------------*/
#ifdef L_BIG_ENDIAN

l_uint32
convertOnBigEnd32(l_uint32  wordin)
{
    return ((wordin << 24) | ((wordin << 8) & 0x00ff0000) |
            ((wordin >> 8) & 0x0000ff00) | (wordin >> 24));
}

l_uint32
convertOnLittleEnd32(l_uint32  wordin)
{
    return wordin;
}

#else  /*  L_LITTLE_ENDIAN */

l_uint32
convertOnLittleEnd32(l_uint32  wordin)
{
    return ((wordin << 24) | ((wordin << 8) & 0x00ff0000) |
            ((wordin >> 8) & 0x0000ff00) | (wordin >> 24));
}

l_uint32
convertOnBigEnd32(l_uint32  wordin)
{
    return wordin;
}

#endif  /* L_BIG_ENDIAN */


/*---------------------------------------------------------------------*
 *                       File corruption operations                    *
 *---------------------------------------------------------------------*/
/*!
 * \brief   fileCorruptByDeletion()
 *
 * \param[in]    filein
 * \param[in]    loc       fractional location of start of deletion
 * \param[in]    size      fractional size of deletion
 * \param[in]    fileout   corrupted file
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) %loc and %size are expressed as a fraction of the file size.
 *      (2) This makes a copy of the data in %filein, where bytes in the
 *          specified region have deleted.
 *      (3) If (%loc + %size) >= 1.0, this deletes from the position
 *          represented by %loc to the end of the file.
 *      (4) It is useful for testing robustness of I/O wrappers when the
 *          data is corrupted, by simulating data corruption by deletion.
 * </pre>
 */
l_ok
fileCorruptByDeletion(const char  *filein,
                      l_float32    loc,
                      l_float32    size,
                      const char  *fileout)
{
l_int32   i, locb, sizeb, rembytes;
size_t    inbytes, outbytes;
l_uint8  *datain, *dataout;

    PROCNAME("fileCorruptByDeletion");

    if (!filein || !fileout)
        return ERROR_INT("filein and fileout not both specified", procName, 1);
    if (loc < 0.0 || loc >= 1.0)
        return ERROR_INT("loc must be in [0.0 ... 1.0)", procName, 1);
    if (size <= 0.0)
        return ERROR_INT("size must be > 0.0", procName, 1);
    if (loc + size > 1.0)
        size = 1.0 - loc;

    datain = l_binaryRead(filein, &inbytes);
    locb = (l_int32)(loc * inbytes + 0.5);
    locb = L_MIN(locb, inbytes - 1);
    sizeb = (l_int32)(size * inbytes + 0.5);
    sizeb = L_MAX(1, sizeb);
    sizeb = L_MIN(sizeb, inbytes - locb);  /* >= 1 */
    L_INFO("Removed %d bytes at location %d\n", procName, sizeb, locb);
    rembytes = inbytes - locb - sizeb;  /* >= 0; to be copied, after excision */

    outbytes = inbytes - sizeb;
    dataout = (l_uint8 *)LEPT_CALLOC(outbytes, 1);
    for (i = 0; i < locb; i++)
        dataout[i] = datain[i];
    for (i = 0; i < rembytes; i++)
        dataout[locb + i] = datain[locb + sizeb + i];
    l_binaryWrite(fileout, "w", dataout, outbytes);

    LEPT_FREE(datain);
    LEPT_FREE(dataout);
    return 0;
}


/*!
 * \brief   fileCorruptByMutation()
 *
 * \param[in]    filein
 * \param[in]    loc       fractional location of start of randomization
 * \param[in]    size      fractional size of randomization
 * \param[in]    fileout   corrupted file
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) %loc and %size are expressed as a fraction of the file size.
 *      (2) This makes a copy of the data in %filein, where bytes in the
 *          specified region have been replaced by random data.
 *      (3) If (%loc + %size) >= 1.0, this modifies data from the position
 *          represented by %loc to the end of the file.
 *      (4) It is useful for testing robustness of I/O wrappers when the
 *          data is corrupted, by simulating data corruption.
 * </pre>
 */
l_ok
fileCorruptByMutation(const char  *filein,
                      l_float32    loc,
                      l_float32    size,
                      const char  *fileout)
{
l_int32   i, locb, sizeb;
size_t    bytes;
l_uint8  *data;

    PROCNAME("fileCorruptByMutation");

    if (!filein || !fileout)
        return ERROR_INT("filein and fileout not both specified", procName, 1);
    if (loc < 0.0 || loc >= 1.0)
        return ERROR_INT("loc must be in [0.0 ... 1.0)", procName, 1);
    if (size <= 0.0)
        return ERROR_INT("size must be > 0.0", procName, 1);
    if (loc + size > 1.0)
        size = 1.0 - loc;

    data = l_binaryRead(filein, &bytes);
    locb = (l_int32)(loc * bytes + 0.5);
    locb = L_MIN(locb, bytes - 1);
    sizeb = (l_int32)(size * bytes + 0.5);
    sizeb = L_MAX(1, sizeb);
    sizeb = L_MIN(sizeb, bytes - locb);  /* >= 1 */
    L_INFO("Randomizing %d bytes at location %d\n", procName, sizeb, locb);

        /* Make an array of random bytes and do the substitution */
    for (i = 0; i < sizeb; i++) {
        data[locb + i] =
            (l_uint8)(255.9 * ((l_float64)rand() / (l_float64)RAND_MAX));
    }

    l_binaryWrite(fileout, "w", data, bytes);
    LEPT_FREE(data);
    return 0;
}


/*---------------------------------------------------------------------*
 *                Generate random integer in given range               *
 *---------------------------------------------------------------------*/
/*!
 * \brief   genRandomIntegerInRange()
 *
 * \param[in]    range     size of range; must be >= 2
 * \param[in]    seed      use 0 to skip; otherwise call srand
 * \param[out]   pval      random integer in range {0 ... range-1}
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) For example, to choose a rand integer between 0 and 99,
 *          use %range = 100.
 * </pre>
 */
l_ok
genRandomIntegerInRange(l_int32   range,
                        l_int32   seed,
                        l_int32  *pval)
{
    PROCNAME("genRandomIntegerInRange");

    if (!pval)
        return ERROR_INT("&val not defined", procName, 1);
    *pval = 0;
    if (range < 2)
        return ERROR_INT("range must be >= 2", procName, 1);

    if (seed > 0) srand(seed);
    *pval = (l_int32)((l_float64)range *
                       ((l_float64)rand() / (l_float64)RAND_MAX));
    return 0;
}


/*---------------------------------------------------------------------*
 *                         Simple math function                        *
 *---------------------------------------------------------------------*/
/*!
 * \brief   lept_roundftoi()
 *
 * \param[in]    fval
 * \return  value rounded to int
 *
 * <pre>
 * Notes:
 *      (1) For fval >= 0, fval --> round(fval) == floor(fval + 0.5)
 *          For fval < 0, fval --> -round(-fval))
 *          This is symmetric around 0.
 *          e.g., for fval in (-0.5 ... 0.5), fval --> 0
 * </pre>
 */
l_int32
lept_roundftoi(l_float32  fval)
{
    return (fval >= 0.0) ? (l_int32)(fval + 0.5) : (l_int32)(fval - 0.5);
}


/*---------------------------------------------------------------------*
 *                        64-bit hash functions                        *
 *---------------------------------------------------------------------*/
/*!
 * \brief   l_hashStringToUint64()
 *
 * \param[in]    str
 * \param[out]   phash    hash value
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) The intent of the hash is to avoid collisions by mapping
 *          the string as randomly as possible into 64 bits.
 *      (2) To the extent that the hashes are random, the probability of
 *          a collision can be approximated by the square of the number
 *          of strings divided by 2^64.  For 1 million strings, the
 *          collision probability is about 1 in 16 million.
 *      (3) I expect non-randomness of the distribution to be most evident
 *          for small text strings.  This hash function has been tested
 *          for all 5-character text strings composed of 26 letters,
 *          of which there are 26^5 = 12356630.  There are no hash
 *          collisions for this set.
 * </pre>
 */
l_ok
l_hashStringToUint64(const char  *str,
                     l_uint64    *phash)
{
l_uint64  hash, mulp;

    PROCNAME("l_hashStringToUint64");

    if (phash) *phash = 0;
    if (!str || (str[0] == '\0'))
        return ERROR_INT("str not defined or empty", procName, 1);
    if (!phash)
        return ERROR_INT("&hash not defined", procName, 1);

    mulp = 26544357894361247;  /* prime, about 1/700 of the max uint64 */
    hash = 104395301;
    while (*str) {
        hash += (*str++ * mulp) ^ (hash >> 7);   /* shift [1...23] are ok */
    }
    *phash = hash ^ (hash << 37);
    return 0;
}


/*!
 * \brief   l_hashPtToUint64()
 *
 * \param[in]    x, y
 * \param[out]   phash    hash value
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) This simple hash function has no collisions for
 *          any of 400 million points with x and y up to 20000.
 *      (2) Previously used a much more complicated and slower function:
 *            mulp = 26544357894361;
 *            hash = 104395301;
 *            hash += (x * mulp) ^ (hash >> 5);
 *            hash ^= (hash << 7);
 *            hash += (y * mulp) ^ (hash >> 7);
 *            hash = hash ^ (hash << 11);
 *          Such logical gymnastics to get coverage over the 2^64
 *          values are not required.
 * </pre>
 */
l_ok
l_hashPtToUint64(l_int32    x,
                 l_int32    y,
                 l_uint64  *phash)
{
    PROCNAME("l_hashPtToUint64");

    if (!phash)
        return ERROR_INT("&hash not defined", procName, 1);

    *phash = (l_uint64)(2173249142.3849 * x + 3763193258.6227 * y);
    return 0;
}


/*!
 * \brief   l_hashFloat64ToUint64()
 *
 * \param[in]    nbuckets
 * \param[in]    val
 * \param[out]   phash      hash value
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) Simple, fast hash for using dnaHash with 64-bit data
 *          (e.g., sets and histograms).
 *      (2) The resulting hash is called a "key" in a lookup
 *          operation.  The bucket for %val in a dnaHash is simply
 *          found by taking the mod of the hash with the number of
 *          buckets (which is prime).  What gets stored in the
 *          dna in that bucket could depend on use, but for the most
 *          flexibility, we store an index into the associated dna.
 *          This is all that is required for generating either a hash set
 *          or a histogram (an example of a hash map).
 *      (3) For example, to generate a histogram, the histogram dna,
 *          a histogram of unique values aligned with the histogram dna,
 *          and a dnahash hashmap are built.  See l_dnaMakeHistoByHash().
 * </pre>
 */
l_ok
l_hashFloat64ToUint64(l_int32    nbuckets,
                      l_float64  val,
                      l_uint64  *phash)
{
    PROCNAME("l_hashFloatToUint64");

    if (!phash)
        return ERROR_INT("&hash not defined", procName, 1);
    *phash = (l_uint64)((21.732491 * nbuckets) * val);
    return 0;
}


/*---------------------------------------------------------------------*
 *                           Prime finders                             *
 *---------------------------------------------------------------------*/
/*!
 * \brief   findNextLargerPrime()
 *
 * \param[in]    start
 * \param[out]   pprime    first prime larger than %start
 * \return  0 if OK, 1 on error
 */
l_ok
findNextLargerPrime(l_int32    start,
                    l_uint32  *pprime)
{
l_int32  i, is_prime;

    PROCNAME("findNextLargerPrime");

    if (!pprime)
        return ERROR_INT("&prime not defined", procName, 1);
    *pprime = 0;
    if (start <= 0)
        return ERROR_INT("start must be > 0", procName, 1);

    for (i = start + 1; ; i++) {
        lept_isPrime(i, &is_prime, NULL);
        if (is_prime) {
            *pprime = i;
            return 0;
        }
    }

    return ERROR_INT("prime not found!", procName, 1);
}


/*!
 * \brief   lept_isPrime()
 *
 * \param[in]    n           64-bit unsigned
 * \param[out]   pis_prime   1 if prime, 0 otherwise
 * \param[out]   pfactor     [optional] smallest divisor, or 0 on error
 *                           or if prime
 * \return  0 if OK, 1 on error
 */
l_ok
lept_isPrime(l_uint64   n,
             l_int32   *pis_prime,
             l_uint32  *pfactor)
{
l_uint32  div;
l_uint64  limit, ratio;

    PROCNAME("lept_isPrime");

    if (pis_prime) *pis_prime = 0;
    if (pfactor) *pfactor = 0;
    if (!pis_prime)
        return ERROR_INT("&is_prime not defined", procName, 1);
    if (n <= 0)
        return ERROR_INT("n must be > 0", procName, 1);

    if (n % 2 == 0) {
        if (pfactor) *pfactor = 2;
        return 0;
    }

    limit = (l_uint64)sqrt((l_float64)n);
    for (div = 3; div < limit; div += 2) {
       ratio = n / div;
       if (ratio * div == n) {
           if (pfactor) *pfactor = div;
           return 0;
       }
    }

    *pis_prime = 1;
    return 0;
}


/*---------------------------------------------------------------------*
 *                         Gray code conversion                        *
 *---------------------------------------------------------------------*/
/*!
 * \brief   convertIntToGrayCode()
 *
 * \param[in]  val    integer value
 * \return     corresponding gray code value
 *
 * <pre>
 * Notes:
 *      (1) Gray code values corresponding to integers differ by
 *          only one bit transition between successive integers.
 * </pre>
 */
l_uint32
convertIntToGrayCode(l_uint32 val)
{
    return (val >> 1) ^ val;
}


/*!
 * \brief   convertGrayCodeToInt()
 *
 * \param[in]  val    gray code value
 * \return     corresponding integer value
 */
l_uint32
convertGrayCodeToInt(l_uint32 val)
{
l_uint32  shift;

    for (shift = 1; shift < 32; shift <<= 1)
        val ^= val >> shift;
    return val;
}


/*---------------------------------------------------------------------*
 *                       Leptonica version number                      *
 *---------------------------------------------------------------------*/
/*!
 * \brief   getLeptonicaVersion()
 *
 *      Return: string of version number (e.g., 'leptonica-1.74.2')
 *
 *  Notes:
 *      (1) The caller has responsibility to free the memory.
 */
char *
getLeptonicaVersion()
{
size_t  bufsize = 100;

    char *version = (char *)LEPT_CALLOC(bufsize, sizeof(char));

#ifdef _MSC_VER
  #ifdef _USRDLL
    char dllStr[] = "DLL";
  #else
    char dllStr[] = "LIB";
  #endif
  #ifdef _DEBUG
    char debugStr[] = "Debug";
  #else
    char debugStr[] = "Release";
  #endif
  #ifdef _M_IX86
    char bitStr[] = " x86";
  #elif _M_X64
    char bitStr[] = " x64";
  #else
    char bitStr[] = "";
  #endif
    snprintf(version, bufsize, "leptonica-%d.%d.%d (%s, %s) [MSC v.%d %s %s%s]",
           LIBLEPT_MAJOR_VERSION, LIBLEPT_MINOR_VERSION, LIBLEPT_PATCH_VERSION,
           __DATE__, __TIME__, _MSC_VER, dllStr, debugStr, bitStr);

#else

    snprintf(version, bufsize, "leptonica-%d.%d.%d", LIBLEPT_MAJOR_VERSION,
             LIBLEPT_MINOR_VERSION, LIBLEPT_PATCH_VERSION);

#endif   /* _MSC_VER */
    return version;
}


/*---------------------------------------------------------------------*
 *                           Timing procs                              *
 *---------------------------------------------------------------------*/
#ifndef _WIN32

#include <sys/time.h>
#include <sys/resource.h>

static struct rusage rusage_before;
static struct rusage rusage_after;

/*!
 * \brief   startTimer(), stopTimer()
 *
 *  Notes:
 *      (1) These measure the cpu time elapsed between the two calls:
 *            startTimer();
 *            ....
 *            fprintf(stderr, "Elapsed time = %7.3f sec\n", stopTimer());
 */
void
startTimer(void)
{
    getrusage(RUSAGE_SELF, &rusage_before);
}

l_float32
stopTimer(void)
{
l_int32  tsec, tusec;

    getrusage(RUSAGE_SELF, &rusage_after);

    tsec = rusage_after.ru_utime.tv_sec - rusage_before.ru_utime.tv_sec;
    tusec = rusage_after.ru_utime.tv_usec - rusage_before.ru_utime.tv_usec;
    return (tsec + ((l_float32)tusec) / 1000000.0);
}


/*!
 * \brief   startTimerNested(), stopTimerNested()
 *
 *  Example of usage:
 *
 *      L_TIMER  t1 = startTimerNested();
 *      ....
 *      L_TIMER  t2 = startTimerNested();
 *      ....
 *      fprintf(stderr, "Elapsed time 2 = %7.3f sec\n", stopTimerNested(t2));
 *      ....
 *      fprintf(stderr, "Elapsed time 1 = %7.3f sec\n", stopTimerNested(t1));
 */
L_TIMER
startTimerNested(void)
{
struct rusage  *rusage_start;

    rusage_start = (struct rusage *)LEPT_CALLOC(1, sizeof(struct rusage));
    getrusage(RUSAGE_SELF, rusage_start);
    return rusage_start;
}

l_float32
stopTimerNested(L_TIMER  rusage_start)
{
l_int32        tsec, tusec;
struct rusage  rusage_stop;

    getrusage(RUSAGE_SELF, &rusage_stop);

    tsec = rusage_stop.ru_utime.tv_sec -
           ((struct rusage *)rusage_start)->ru_utime.tv_sec;
    tusec = rusage_stop.ru_utime.tv_usec -
           ((struct rusage *)rusage_start)->ru_utime.tv_usec;
    LEPT_FREE(rusage_start);
    return (tsec + ((l_float32)tusec) / 1000000.0);
}


/*!
 * \brief   l_getCurrentTime()
 *
 * \param[out]   sec     [optional] in seconds since birth of Unix
 * \param[out]   usec    [optional] in microseconds since birth of Unix
 * \return  void
 */
void
l_getCurrentTime(l_int32  *sec,
                 l_int32  *usec)
{
struct timeval tv;

    gettimeofday(&tv, NULL);
    if (sec) *sec = (l_int32)tv.tv_sec;
    if (usec) *usec = (l_int32)tv.tv_usec;
    return;
}


#else   /* _WIN32 : resource.h not implemented under Windows */

    /* Note: if division by 10^7 seems strange, the time is expressed
     * as the number of 100-nanosecond intervals that have elapsed
     * since 12:00 A.M. January 1, 1601.  */

static ULARGE_INTEGER utime_before;
static ULARGE_INTEGER utime_after;

void
startTimer(void)
{
HANDLE    this_process;
FILETIME  start, stop, kernel, user;

    this_process = GetCurrentProcess();

    GetProcessTimes(this_process, &start, &stop, &kernel, &user);

    utime_before.LowPart  = user.dwLowDateTime;
    utime_before.HighPart = user.dwHighDateTime;
}

l_float32
stopTimer(void)
{
HANDLE     this_process;
FILETIME   start, stop, kernel, user;
ULONGLONG  hnsec;  /* in units of hecto-nanosecond (100 ns) intervals */

    this_process = GetCurrentProcess();

    GetProcessTimes(this_process, &start, &stop, &kernel, &user);

    utime_after.LowPart  = user.dwLowDateTime;
    utime_after.HighPart = user.dwHighDateTime;
    hnsec = utime_after.QuadPart - utime_before.QuadPart;
    return (l_float32)(signed)hnsec / 10000000.0;
}

L_TIMER
startTimerNested(void)
{
HANDLE           this_process;
FILETIME         start, stop, kernel, user;
ULARGE_INTEGER  *utime_start;

    this_process = GetCurrentProcess();

    GetProcessTimes (this_process, &start, &stop, &kernel, &user);

    utime_start = (ULARGE_INTEGER *)LEPT_CALLOC(1, sizeof(ULARGE_INTEGER));
    utime_start->LowPart  = user.dwLowDateTime;
    utime_start->HighPart = user.dwHighDateTime;
    return utime_start;
}

l_float32
stopTimerNested(L_TIMER  utime_start)
{
HANDLE          this_process;
FILETIME        start, stop, kernel, user;
ULARGE_INTEGER  utime_stop;
ULONGLONG       hnsec;  /* in units of 100 ns intervals */

    this_process = GetCurrentProcess ();

    GetProcessTimes (this_process, &start, &stop, &kernel, &user);

    utime_stop.LowPart  = user.dwLowDateTime;
    utime_stop.HighPart = user.dwHighDateTime;
    hnsec = utime_stop.QuadPart - ((ULARGE_INTEGER *)utime_start)->QuadPart;
    LEPT_FREE(utime_start);
    return (l_float32)(signed)hnsec / 10000000.0;
}

void
l_getCurrentTime(l_int32  *sec,
                 l_int32  *usec)
{
ULARGE_INTEGER  utime, birthunix;
FILETIME        systemtime;
LONGLONG        birthunixhnsec = 116444736000000000;  /*in units of 100 ns */
LONGLONG        usecs;

    GetSystemTimeAsFileTime(&systemtime);
    utime.LowPart  = systemtime.dwLowDateTime;
    utime.HighPart = systemtime.dwHighDateTime;

    birthunix.LowPart = (DWORD) birthunixhnsec;
    birthunix.HighPart = birthunixhnsec >> 32;

    usecs = (LONGLONG) ((utime.QuadPart - birthunix.QuadPart) / 10);

    if (sec) *sec = (l_int32) (usecs / 1000000);
    if (usec) *usec = (l_int32) (usecs % 1000000);
    return;
}

#endif


/*!
 * \brief   startWallTimer()
 *
 * \return  walltimer-ptr
 *
 * <pre>
 * Notes:
 *      (1) These measure the wall clock time  elapsed between the two calls:
 *            L_WALLTIMER *timer = startWallTimer();
 *            ....
 *            fprintf(stderr, "Elapsed time = %f sec\n", stopWallTimer(&timer);
 *      (2) Note that the timer object is destroyed by stopWallTimer().
 * </pre>
 */
L_WALLTIMER *
startWallTimer(void)
{
L_WALLTIMER  *timer;

    timer = (L_WALLTIMER *)LEPT_CALLOC(1, sizeof(L_WALLTIMER));
    l_getCurrentTime(&timer->start_sec, &timer->start_usec);
    return timer;
}

/*!
 * \brief   stopWallTimer()
 *
 * \param[in,out]  ptimer     walltimer pointer
 * \return  time wall time elapsed in seconds
 */
l_float32
stopWallTimer(L_WALLTIMER  **ptimer)
{
l_int32       tsec, tusec;
L_WALLTIMER  *timer;

    PROCNAME("stopWallTimer");

    if (!ptimer)
        return (l_float32)ERROR_FLOAT("&timer not defined", procName, 0.0);
    timer = *ptimer;
    if (!timer)
        return (l_float32)ERROR_FLOAT("timer not defined", procName, 0.0);

    l_getCurrentTime(&timer->stop_sec, &timer->stop_usec);
    tsec = timer->stop_sec - timer->start_sec;
    tusec = timer->stop_usec - timer->start_usec;
    LEPT_FREE(timer);
    *ptimer = NULL;
    return (tsec + ((l_float32)tusec) / 1000000.0);
}


/*!
 * \brief   l_getFormattedDate()
 *
 * \return  formatted date string, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) This is used in pdf, in the form specified in section 3.8.2 of
 *          http://partners.adobe.com/public/developer/en/pdf/PDFReference.pdf
 *      (2) Contributed by Dave Bryan.  Works on all platforms.
 * </pre>
 */
char *
l_getFormattedDate()
{
char        buf[128] = "", sep = 'Z';
l_int32     gmt_offset, relh, relm;
time_t      ut, lt;
struct tm   Tm;
struct tm  *tptr = &Tm;

    ut = time(NULL);

        /* This generates a second "time_t" value by calling "gmtime" to
           fill in a "tm" structure expressed as UTC and then calling
           "mktime", which expects a "tm" structure expressed as the
           local time.  The result is a value that is offset from the
           value returned by the "time" function by the local UTC offset.
           "tm_isdst" is set to -1 to tell "mktime" to determine for
           itself whether DST is in effect.  This is necessary because
           "gmtime" always sets "tm_isdst" to 0, which would tell
           "mktime" to presume that DST is not in effect. */
#ifdef _WIN32
  #ifdef _MSC_VER
    gmtime_s(tptr, &ut);
  #else  /* mingw */
    tptr = gmtime(&ut);
  #endif
#else
    gmtime_r(&ut, tptr);
#endif
    tptr->tm_isdst = -1;
    lt = mktime(tptr);

        /* Calls "difftime" to obtain the resulting difference in seconds,
         * because "time_t" is an opaque type, per the C standard. */
    gmt_offset = (l_int32) difftime(ut, lt);

    if (gmt_offset > 0)
        sep = '+';
    else if (gmt_offset < 0)
        sep = '-';

    relh = L_ABS(gmt_offset) / 3600;
    relm = (L_ABS(gmt_offset) % 3600) / 60;

    strftime(buf, sizeof(buf), "%Y%m%d%H%M%S", localtime(&ut));
    sprintf(buf + 14, "%c%02d'%02d'", sep, relh, relm);
    return stringNew(buf);
}
