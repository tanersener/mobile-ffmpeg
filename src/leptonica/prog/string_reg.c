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
 * string_reg.c
 *
 *    This tests:
 *      * search/replace for strings and arrays
 *      * sarray generation and flattening
 *      * sarray serialization
 */

#include <string.h>
#include "allheaders.h"

char strs[32] = "This is a gooood test!";
char  substr1[2] = "o";
char  substr2[4] = "00";

int main(int    argc,
         char **argv)
{
l_int32       loc, count;
size_t        size1, size2;
char         *str0, *str1, *str2, *str3, *str4, *str5, *str6;
l_uint8      *data1, *data2;
L_DNA        *da;
SARRAY       *sa1, *sa2, *sa3;
L_REGPARAMS  *rp;

    if (regTestSetup(argc, argv, &rp))
        return 1;

    lept_mkdir("lept/string");

        /* Finding all substrings */
    da = stringFindEachSubstr(strs, substr1);
    regTestCompareValues(rp, 4, l_dnaGetCount(da), 0.0);  /* 0 */
    l_dnaDestroy(&da);

        /* Replacing a substring */
    loc = 0;
    str1 = stringReplaceSubstr(strs, "his", "hers", &loc, NULL);
    regTestCompareValues(rp, 5, loc, 0.0);  /* 1 */
    regTestCompareStrings(rp, (l_uint8 *)"Thers is a gooood test!", 23,
                          (l_uint8 *)str1, strlen(str1));   /* 2 */
    lept_free(str1);

        /* Replacing all substrings */
    str1 = stringReplaceEachSubstr(strs, substr1, substr2, &count);
    regTestCompareValues(rp, 4, count, 0.0);  /* 3 */
    regTestCompareStrings(rp, (l_uint8 *)"This is a g00000000d test!", 26,
                          (l_uint8 *)str1, strlen(str1));   /* 4 */
    lept_free(str1);

    str1 = stringReplaceEachSubstr(strs, substr1, "", &count);
    regTestCompareValues(rp, 4, count, 0.0);  /* 5 */
    regTestCompareStrings(rp, (l_uint8 *)"This is a gd test!", 18,
                          (l_uint8 *)str1, strlen(str1));   /* 6 */
    lept_free(str1);

        /* Finding all sequances */
    str1 = (char *)l_binaryRead("kernel_reg.c", &size1);
    da = arrayFindEachSequence((l_uint8 *)str1, size1,
                               (l_uint8 *)"Destroy", 7);
    regTestCompareValues(rp, 55, l_dnaGetCount(da), 0.0);  /* 7 */
    l_dnaDestroy(&da);
    lept_free(str1);

        /* Replacing all sequances */
    str1 = (char *)l_binaryRead("kernel_reg.c", &size1);
    data1 = arrayReplaceEachSequence((l_uint8 *)str1, size1,
                                     (l_uint8 *)"Destroy", 7,
                                     (l_uint8 *)"####", 4, &size2, &count);
    l_binaryWrite("/tmp/lept/string/string1.txt", "w", data1, size2);
    regTestCheckFile(rp, "/tmp/lept/string/string1.txt");  /* 8 */
    regTestCompareValues(rp, 55, count, 0.0);  /* 9 */
    data2 = arrayReplaceEachSequence((l_uint8 *)str1, size1,
                                     (l_uint8 *)"Destroy", 7,
                                     NULL, 0, &size2, &count);
    l_binaryWrite("/tmp/lept/string/string2.txt", "w", data2, size2);
    regTestCheckFile(rp, "/tmp/lept/string/string2.txt");  /* 10 */
    regTestCompareValues(rp, 55, count, 0.0);  /* 11 */
    lept_free(data1);
    lept_free(data2);
    lept_free(str1);

        /* Generating sarray from strings, and v.v  */
    str0 = (char *)l_binaryRead("kernel_reg.c", &size1);
    str0[2500] = '\0';
    sa1 = sarrayCreateWordsFromString(str0 + 2000);
    sa2 = sarrayCreateLinesFromString(str0 + 2000, 0);
    sa3 = sarrayCreateLinesFromString(str0 + 2000, 1);
    str1 = sarrayToString(sa1, 0);
    str2 = sarrayToString(sa1, 1);
    str3 = sarrayToString(sa2, 0);
    str4 = sarrayToString(sa2, 1);
    str5 = sarrayToString(sa3, 0);
    str6 = sarrayToString(sa3, 1);
    l_binaryWrite("/tmp/lept/string/test1.txt", "w", str1, strlen(str1));
    l_binaryWrite("/tmp/lept/string/test2.txt", "w", str2, strlen(str2));
    l_binaryWrite("/tmp/lept/string/test3.txt", "w", str3, strlen(str3));
    l_binaryWrite("/tmp/lept/string/test4.txt", "w", str4, strlen(str4));
    l_binaryWrite("/tmp/lept/string/test5.txt", "w", str5, strlen(str5));
    l_binaryWrite("/tmp/lept/string/test6.txt", "w", str6, strlen(str6));
    regTestCheckFile(rp, "/tmp/lept/string/test1.txt");  /* 12 */
    regTestCheckFile(rp, "/tmp/lept/string/test2.txt");  /* 13 */
    regTestCheckFile(rp, "/tmp/lept/string/test3.txt");  /* 14 */
    regTestCheckFile(rp, "/tmp/lept/string/test4.txt");  /* 15 */
    regTestCheckFile(rp, "/tmp/lept/string/test5.txt");  /* 16 */
    regTestCheckFile(rp, "/tmp/lept/string/test6.txt");  /* 17 */
    regTestCompareFiles(rp, 14, 16);  /* 18 */
    lept_free(str0);
    lept_free(str1);
    lept_free(str2);
    lept_free(str3);
    lept_free(str4);
    lept_free(str5);
    lept_free(str6);
    sarrayDestroy(&sa1);
    sarrayDestroy(&sa2);
    sarrayDestroy(&sa3);

        /* Test sarray serialization */
    str1 = (char *)l_binaryRead("kernel_reg.c", &size1);
    sa1 = sarrayCreateLinesFromString(str1, 0);
    sarrayWrite("/tmp/lept/string/test7.txt", sa1);
    sa2 = sarrayRead("/tmp/lept/string/test7.txt");
    sarrayWrite("/tmp/lept/string/test8.txt", sa2);
    regTestCheckFile(rp, "/tmp/lept/string/test7.txt");  /* 19 */
    regTestCheckFile(rp, "/tmp/lept/string/test8.txt");  /* 20 */
    regTestCompareFiles(rp, 19, 20);  /* 21 */
    lept_free(str1);
    sarrayDestroy(&sa1);
    sarrayDestroy(&sa2);

    return regTestCleanup(rp);
}
