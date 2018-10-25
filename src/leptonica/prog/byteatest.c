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
 * byteatest.c
 *
 */

#include <string.h>
#include "allheaders.h"

int main(int    argc,
         char **argv)
{
char        *str;
l_uint8     *data1, *data2;
l_int32      i, n, same1, same2;
size_t       size1, size2, slice, total, start, end;
FILE        *fp;
L_DNA       *da;
SARRAY      *sa;
L_BYTEA     *lba1, *lba2, *lba3, *lba4, *lba5;
static char  mainName[] = "byteatest";

    if (argc != 1)
        return ERROR_INT("syntax: byteatest", mainName, 1);

    setLeptDebugOK(1);
    lept_mkdir("bytea");

        /* Test basic init, join and split */
    lba1 = l_byteaInitFromFile("feyn.tif");
    lba2 = l_byteaInitFromFile("test24.jpg");
    size1 = l_byteaGetSize(lba1);
    size2 = l_byteaGetSize(lba2);
    l_byteaJoin(lba1, &lba2);
    lba3 = l_byteaInitFromMem(lba1->data, size1);
    lba4 = l_byteaInitFromMem(lba1->data + size1, size2);

        /* Split by hand */
    l_binaryWrite("/tmp/bytea/junk1.dat", "w", lba3->data, lba3->size);
    l_binaryWrite("/tmp/bytea/junk2.dat", "w", lba4->data, lba4->size);
    filesAreIdentical("feyn.tif", "/tmp/bytea/junk1.dat", &same1);
    filesAreIdentical("test24.jpg", "/tmp/bytea/junk2.dat", &same2);
    if (same1 && same2)
        fprintf(stderr, "OK for join file\n");
    else
        fprintf(stderr, "Error: files are different!\n");

        /* Split by function */
    l_byteaSplit(lba1, size1, &lba5);
    l_binaryWrite("/tmp/bytea/junk3.dat", "w", lba1->data, lba1->size);
    l_binaryWrite("/tmp/bytea/junk4.dat", "w", lba5->data, lba5->size);
    filesAreIdentical("feyn.tif", "/tmp/bytea/junk3.dat", &same1);
    filesAreIdentical("test24.jpg", "/tmp/bytea/junk4.dat", &same2);
    if (same1 && same2)
        fprintf(stderr, "OK for split file\n");
    else
        fprintf(stderr, "Error: files are different!\n");
    l_byteaDestroy(&lba1);
    l_byteaDestroy(&lba2);
    l_byteaDestroy(&lba3);
    l_byteaDestroy(&lba4);
    l_byteaDestroy(&lba5);

        /* Test appending with strings */
    data1 = l_binaryRead("kernel_reg.c", &size1);
    sa = sarrayCreateLinesFromString((char *)data1, 1);
    lba1 = l_byteaCreate(0);
    n = sarrayGetCount(sa);
    for (i = 0; i < n; i++) {
        str = sarrayGetString(sa, i, L_NOCOPY);
        l_byteaAppendString(lba1, str);
        l_byteaAppendString(lba1, (char *)"\n");
    }
    data2 = l_byteaGetData(lba1, &size2);
    l_binaryWrite("/tmp/bytea/junk5.dat", "w", data2, size2);
    filesAreIdentical("kernel_reg.c", "/tmp/bytea/junk5.dat", &same1);
    if (same1)
        fprintf(stderr, "OK for appended string data\n");
    else
        fprintf(stderr, "Error: appended string data is different!\n");
    lept_free(data1);
    sarrayDestroy(&sa);
    l_byteaDestroy(&lba1);

        /* Test appending with binary data */
    slice = 1000;
    total = nbytesInFile("breviar.38.150.jpg");
    lba1 = l_byteaCreate(100);
    n = 1 + total / slice;
    fprintf(stderr, "******************************************************\n");
    fprintf(stderr, "* Testing error checking: ignore two reported errors *\n");
    for (i = 0, start = 0; i <= n; i++, start += slice) {
         data1 = l_binaryReadSelect("breviar.38.150.jpg", start, slice, &size1);
         l_byteaAppendData(lba1, data1, size1);
         lept_free(data1);
    }
    fprintf(stderr, "******************************************************\n");
    data2 = l_byteaGetData(lba1, &size2);
    l_binaryWrite("/tmp/bytea/junk6.dat", "w", data2, size2);
    filesAreIdentical("breviar.38.150.jpg", "/tmp/bytea/junk6.dat", &same1);
    if (same1)
        fprintf(stderr, "OK for appended binary data\n");
    else
        fprintf(stderr, "Error: appended binary data is different!\n");
    l_byteaDestroy(&lba1);

        /* Test search */
    convertToPdf("test24.jpg", L_JPEG_ENCODE, 0, "/tmp/bytea/junk7.pdf",
                 0, 0, 100, NULL, NULL, 0);
    lba1 = l_byteaInitFromFile("/tmp/bytea/junk7.pdf");
    l_byteaFindEachSequence(lba1, (l_uint8 *)" 0 obj\n", 7, &da);
    /* l_dnaWriteStream(stderr, da); */
    n = l_dnaGetCount(da);
    if (n == 6)
        fprintf(stderr, "OK for search: found 6 instances\n");
    else
        fprintf(stderr, "Error in search: found %d instances, not 6\n", n);
    l_byteaDestroy(&lba1);
    l_dnaDestroy(&da);

        /* Test write to file */
    lba1 = l_byteaInitFromFile("feyn.tif");
    fp = lept_fopen("/tmp/bytea/junk8.dat", "wb");
    size1 = l_byteaGetSize(lba1);
    for (start = 0; start < size1; start += 1000) {
         end = L_MIN(start + 1000 - 1, size1 - 1);
         l_byteaWriteStream(fp, lba1, start, end);
    }
    lept_fclose(fp);
    filesAreIdentical("feyn.tif", "/tmp/bytea/junk8.dat", &same1);
    if (same1)
        fprintf(stderr, "OK for written binary data\n");
    else
        fprintf(stderr, "Error: written binary data is different!\n");
    l_byteaDestroy(&lba1);

    return 0;
}


