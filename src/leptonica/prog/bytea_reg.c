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

/*
 * bytea_reg.c
 *
 *  This tests the byte array utility.
 */

#include <string.h>
#include "allheaders.h"

int main(int    argc,
         char **argv)
{
char         *str;
l_uint8      *data1, *data2;
l_int32       i, n;
size_t        size1, size2, slice, total, start;
FILE         *fp;
L_DNA        *da;
SARRAY       *sa;
L_BYTEA      *lba1, *lba2, *lba3, *lba4;
L_REGPARAMS  *rp;

    if (regTestSetup(argc, argv, &rp))
        return 1;

    lept_mkdir("lept/bytea");

        /* Test basic init and join */
    lba1 = l_byteaInitFromFile("feyn.tif");
    lba2 = l_byteaInitFromFile("test24.jpg");
    lba3 = l_byteaCopy(lba2, L_COPY);
    size1 = l_byteaGetSize(lba1);
    size2 = l_byteaGetSize(lba2);
    l_byteaJoin(lba1, &lba3);  /* destroys lba3 */
    l_byteaWrite("/tmp/lept/bytea/lba2.bya", lba2, 0, 0);
    regTestCheckFile(rp, "/tmp/lept/bytea/lba2.bya");  /* 0 */

        /* Test split, using init from memory */
    lba3 = l_byteaInitFromMem(lba1->data, size1);
    lba4 = l_byteaInitFromMem(lba1->data + size1, size2);
    regTestCompareStrings(rp, lba1->data, size1, lba3->data, lba3->size);
    regTestCompareStrings(rp, lba2->data, size2, lba4->data, lba4->size);
                              /* 1, 2 */
    l_byteaDestroy(&lba4);

        /* Test split using the split function */
    l_byteaSplit(lba1, size1, &lba4);   /* zeroes lba1 beyond size1 */
    regTestCompareStrings(rp, lba2->data, size2, lba4->data, lba4->size);
                              /* 3 */
    l_byteaDestroy(&lba1);
    l_byteaDestroy(&lba2);
    l_byteaDestroy(&lba3);
    l_byteaDestroy(&lba4);

        /* Test appending with strings */
    data1 = l_binaryRead("kernel_reg.c", &size1);
    lba1 = l_byteaInitFromMem(data1, size1);
    sa = sarrayCreateLinesFromString((char *)data1, 1);
    lba2 = l_byteaCreate(0);
    n = sarrayGetCount(sa);
    for (i = 0; i < n; i++) {
        str = sarrayGetString(sa, i, L_NOCOPY);
        l_byteaAppendString(lba2, str);
#ifdef _WIN32
        l_byteaAppendString(lba2, "\r\n");
#else
        l_byteaAppendString(lba2, "\n");
#endif
    }
    data2 = l_byteaGetData(lba1, &size2);
    regTestCompareStrings(rp, lba1->data, lba1->size, lba2->data, lba2->size);
                              /* 4 */
    lept_free(data1);
    sarrayDestroy(&sa);
    l_byteaDestroy(&lba1);
    l_byteaDestroy(&lba2);


        /* Test appending with binary data */
    slice = 1000;
    total = nbytesInFile("breviar.38.150.jpg");
    lba1 = l_byteaCreate(100);
    n = 2 + total / slice;  /* using 1 is correct; using 2 gives two errors */
    fprintf(stderr, "******************************************************\n");
    fprintf(stderr, "* Testing error checking: ignore two reported errors *\n");
    for (i = 0, start = 0; i < n; i++, start += slice) {
         data2 = l_binaryReadSelect("breviar.38.150.jpg", start, slice, &size2);
         l_byteaAppendData(lba1, data2, size2);
         lept_free(data2);
    }
    fprintf(stderr, "******************************************************\n");
    data1 = l_byteaGetData(lba1, &size1);
    data2 = l_binaryRead("breviar.38.150.jpg", &size2);
    regTestCompareStrings(rp, data1, size1, data2, size2);  /* 5 */
    l_byteaDestroy(&lba1);
    lept_free(data2);

        /* Test search */
    convertToPdf("test24.jpg", L_JPEG_ENCODE, 0, "/tmp/lept/bytea/test24.pdf",
                 0, 0, 100, NULL, NULL, 0);
    lba1 = l_byteaInitFromFile("/tmp/lept/bytea/test24.pdf");
    l_byteaFindEachSequence(lba1, (l_uint8 *)" 0 obj\n", 7, &da);
    n = l_dnaGetCount(da);
    regTestCompareValues(rp, 6, n, 0.0);  /* 6 */
    l_byteaDestroy(&lba1);
    l_dnaDestroy(&da);

        /* Test write to file */
    lba1 = l_byteaInitFromFile("feyn.tif");
    size1 = l_byteaGetSize(lba1);
    fp = lept_fopen("/tmp/lept/bytea/feyn.dat", "wb");
    for (start = 0; start < size1; start += 1000) {
         l_byteaWriteStream(fp, lba1, start, 1000);
    }
    lept_fclose(fp);
    lba2 = l_byteaInitFromFile("/tmp/lept/bytea/feyn.dat");
    regTestCompareStrings(rp, lba1->data, size1, lba2->data,
                          lba2->size);  /* 7 */
    l_byteaDestroy(&lba1);
    l_byteaDestroy(&lba2);

    return regTestCleanup(rp);
}

