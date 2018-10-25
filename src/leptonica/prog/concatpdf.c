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
 * concatpdf.c
 *
 *  N.B. This works on Unix.
 *       It relies on the following resources:
 *          * acroread
 *          * ghostscript
 *       Adobe is no longer making acroread binaries for linux.
 *
 *    Program to concatenate a set of pdf files into a single one.
 *    This works well when the input pdf files are not scanned, but
 *    instead are generated orthographically.
 *
 *    Syntax: concatpdf dir [pattern]
 *        where pattern is an optional string to be matched
 *
 *    The output goes to:  /tmp/lept/image/output.pdf
 *
 *    This works by converting to PostScript (without annotations),
 *    then rasterizing the images, and finally generating a pdf from
 *    the set of images.  A good reference to command-line usage of
 *    acroread is:
 *        http://www.physics.ohio-state.edu/~wilkins/html/acroread.html
 *
 *    The steps are as follows:
 *
 *    (1) Use acroread to generate ps files without annotations, which
 *        can cause difficulties in later stages.  The ps files are
 *        made in /tmp/lept/ps/.
 *    (2) Use ps2pdf-gray from Ghostscript to rasterize the images.
 *        The images are written to /tmp/lept/image/
 *    (3) Use convertFilesToPdf to generate a pdf file,
 *        /tmp/lept/image/output.pdf, from the images.
 */

#include <sys/stat.h>
#include "allheaders.h"

static const l_int32  RESOLUTION = 300;

l_int32 main(int    argc,
             char **argv)
{
char         buf[256], rootname[256];
char        *dir, *pattern, *psdir, *imagedir;
char        *fname, *tail, *filename;
l_int32      i, n, ret;
SARRAY      *sa, *saps;
static char  mainName[] = "concatpdf";

    if (argc != 2 && argc != 3)
        return ERROR_INT("Syntax: concatpdf dir [pattern]", mainName, 1);
    dir = argv[1];
    pattern = (argc == 3) ? argv[2] : NULL;
    setLeptDebugOK(1);

        /* Get the names of the pdf files */
    sa = getSortedPathnamesInDirectory(dir, pattern, 0, 0);
    sarrayWriteStream(stderr, sa);
    n = sarrayGetCount(sa);

#if 1
        /* Convert to ps */
    psdir = genPathname("/tmp/lept/ps", NULL);
    lept_rmdir("lept/ps");
    lept_mkdir("lept/ps");
    saps = sarrayCreate(n);
    for (i = 0; i < n; i++) {
        fname = sarrayGetString(sa, i, L_NOCOPY);
        splitPathAtDirectory(fname, NULL, &tail);
        splitPathAtExtension(tail, &filename, NULL);
        snprintf(buf, sizeof(buf), "acroread -toPostScript -annotsOff %s %s",
                 fname, psdir);
        fprintf(stderr, "%s\n", buf);
        ret = system(buf);  /* acroread -toPostScript -annotsOff */
        snprintf(buf, sizeof(buf), "%s/%s.ps", psdir, filename);
        sarrayAddString(saps, buf, L_COPY);
        lept_free(tail);
        lept_free(filename);
    }
    sarrayDestroy(&sa);
#endif

#if 1
        /* Rasterize */
    imagedir = genPathname("/tmp/lept/image", NULL);
    lept_rmdir("lept/image");
    lept_mkdir("lept/image");
    sarrayWriteStream(stderr, saps);
    n = sarrayGetCount(saps);
    for (i = 0; i < n; i++) {
        fname = sarrayGetString(saps, i, L_NOCOPY);
        snprintf(rootname, sizeof(rootname), "%s/r%d", imagedir, i);
        snprintf(buf, sizeof(buf), "ps2png-gray %s %s", fname, rootname);
        fprintf(stderr, "%s\n", buf);
        ret = system(buf);  /* ps2png-gray */
    }

        /* Generate the pdf */
    convertFilesToPdf(imagedir, "png", RESOLUTION, 1.0, L_FLATE_ENCODE,
                      0, "", "/tmp/lept/image/output.pdf");
    lept_free(imagedir);
#endif

    return 0;
}

