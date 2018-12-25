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
 * \file flipdetect.c
 * <pre>
 *
 *      High-level interface for detection and correction
 *          l_int32      pixOrientCorrect()
 *
 *      Page orientation detection (pure rotation by 90 degree increments):
 *          l_int32      pixOrientDetect()
 *          l_int32      makeOrientDecision()
 *          l_int32      pixUpDownDetect()
 *          l_int32      pixUpDownDetectGeneral()
 *          l_int32      pixOrientDetectDwa()
 *          l_int32      pixUpDownDetectDwa()
 *          l_int32      pixUpDownDetectGeneralDwa()
 *
 *      Page mirror detection (flip 180 degrees about line in plane of image):
 *          l_int32      pixMirrorDetect()
 *          l_int32      pixMirrorDetectDwa()
 *
 *      Static debug helper
 *          void         pixDebugFlipDetect()
 *
 *  ===================================================================
 *
 *  Page transformation detection:
 *
 *  Once a page is deskewed, there are 8 possible states that it
 *  can be in, shown symbolically below.  Suppose state 0 is correct.
 *
 *      0: correct     1          2          3
 *      +------+   +------+   +------+   +------+
 *      | **** |   | *    |   | **** |   |    * |
 *      | *    |   | *    |   |    * |   |    * |
 *      | *    |   | **** |   |    * |   | **** |
 *      +------+   +------+   +------+   +------+
 *
 *         4          5          6          7
 *      +-----+    +-----+    +-----+    +-----+
 *      | *** |    |   * |    | *** |    | *   |
 *      |   * |    |   * |    | *   |    | *   |
 *      |   * |    |   * |    | *   |    | *   |
 *      |   * |    | *** |    | *   |    | *** |
 *      +-----+    +-----+    +-----+    +-----+
 *
 *  Each of the other seven can be derived from state 0 by applying some
 *  combination of a 90 degree clockwise rotation, a flip about
 *  a horizontal line, and a flip about a vertical line,
 *  all abbreviated as:
 *      R = Rotation (about a line perpendicular to the image)
 *      H = Horizontal flip (about a vertical line in the plane of the image)
 *      V = Vertical flip (about a horizontal line in the plane of the image)
 *
 *  We get these transformations:
 *      RHV
 *      000  -> 0
 *      001  -> 1
 *      010  -> 2
 *      011  -> 3
 *      100  -> 4
 *      101  -> 5
 *      110  -> 6
 *      111  -> 7
 *
 *  Note that in four of these, the sum of H and V is 1 (odd).
 *  For these four, we have a change in parity (handedness) of
 *  the image, and the transformation cannot be performed by
 *  rotation about a vertical line out of the page.   Under
 *  rotation R, the set of 8 transformations decomposes into
 *  two subgroups linking {0, 3, 4, 7} and {1, 2, 5, 6} independently.
 *
 *  pixOrientDetect*() tests for a pure rotation (0, 90, 180, 270 degrees).
 *  It doesn't change parity.
 *
 *  pixMirrorDetect*() tests for a horizontal flip about the vertical axis.
 *  It changes parity.
 *
 *  The landscape/portrait rotation can be detected in two ways:
 *
 *    (1) Compute the deskew confidence for an image segment,
 *        both as is and rotated 90 degrees  (see skew.c).
 *
 *    (2) Compute the ascender/descender signal for the image,
 *        both as is and rotated 90 degrees  (implemented here).
 *
 *  The ascender/descender signal is useful for determining text
 *  orientation in Roman alphabets because the incidence of letters
 *  with straight-line ascenders (b, d, h, k, l, 't') outnumber
 *  those with descenders ('g', p, q).  The letters 't' and 'g'
 *  will respond variably to the filter, depending on the type face.
 *
 *  What about the mirror image situations?  These aren't common
 *  unless you're dealing with film, for example.
 *  But you can reliably test if the image has undergone a
 *  parity-changing flip once about some axis in the plane
 *  of the image, using pixMirrorDetect*().  This works ostensibly by
 *  counting the number of characters with ascenders that
 *  stick out to the left and right of the ascender.  Characters
 *  that are not mirror flipped are more likely to extend to the
 *  right (b, h, k) than to the left (d).  Of course, that is for
 *  text that is rightside-up.  So before you apply the mirror
 *  test, it is necessary to insure that the text has the ascenders
 *  going up, and not down or to the left or right.  But here's
 *  what *really* happens.  It turns out that the pre-filtering before
 *  the hit-miss transform (HMT) is crucial, and surprisingly, when
 *  the pre-filtering is chosen to generate a large signal, the majority
 *  of the signal comes from open regions of common lower-case
 *  letters such as 'e', 'c' and 'f'.
 *
 *  All operations are given in two implementations whose results are
 *  identical: rasterop morphology and dwa morphology.  The dwa
 *  implementations are between 2x and 3x faster.
 *
 *  The set of operations you actually use depends on your prior knowledge:
 *
 *  (1) If the page is known to be either rightside-up or upside-down, use
 *      either pixOrientDetect*() with pleftconf = NULL, or
 *      pixUpDownDetect*().   [The '*' refers to either the rasterop
 *      or dwa versions.]
 *
 *  (2) If any of the four orientations are possible, use pixOrientDetect*().
 *
 *  (3) If the text is horizontal and rightside-up, the only remaining
 *      degree of freedom is a left-right mirror flip: use
 *      pixMirrorDetect*().
 *
 *  (4) If you have a relatively large amount of numbers on the page,
 *      us the slower pixUpDownDetectGeneral().
 *
 *  We summarize the full orientation and mirror flip detection process:
 *
 *  (1) First determine which of the four 90 degree rotations
 *      causes the text to be rightside-up.  This can be done
 *      with either skew confidence or the pixOrientDetect*()
 *      signals.  For the latter, see the table for pixOrientDetect().
 *
 *  (2) Then, with ascenders pointing up, apply pixMirrorDetect*().
 *      In the normal situation the confidence confidence will be
 *      large and positive.  However, if mirror flipped, the
 *      confidence will be large and negative.
 *
 *  A high-level interface, pixOrientCorrect() combines the detection
 *  of the orientation with the rotation decision and the rotation itself.
 * </pre>
 */

#include <math.h>
#include "allheaders.h"

    /* Sels for pixOrientDetect() and pixMirrorDetect() */
static const char *textsel1 = "x  oo "
                              "x oOo "
                              "x  o  "
                              "x     "
                              "xxxxxx";

static const char *textsel2 = " oo  x"
                              " oOo x"
                              "  o  x"
                              "     x"
                              "xxxxxx";

static const char *textsel3 = "xxxxxx"
                              "x     "
                              "x  o  "
                              "x oOo "
                              "x  oo ";

static const char *textsel4 = "xxxxxx"
                              "     x"
                              "  o  x"
                              " oOo x"
                              " oo  x";

    /* Parameters for determining orientation */
static const l_int32  DEFAULT_MIN_UP_DOWN_COUNT = 70;
static const l_float32  DEFAULT_MIN_UP_DOWN_CONF = 8.0;
static const l_float32  DEFAULT_MIN_UP_DOWN_RATIO = 2.5;

    /* Parameters for determining mirror flip */
static const l_int32  DEFAULT_MIN_MIRROR_FLIP_COUNT = 100;
static const l_float32  DEFAULT_MIN_MIRROR_FLIP_CONF = 5.0;

    /* Static debug function */
static void pixDebugFlipDetect(const char *filename, PIX *pixs,
                               PIX *pixhm, l_int32 enable);


/*----------------------------------------------------------------*
 *        High-level interface for detection and correction       *
 *----------------------------------------------------------------*/
/*!
 * \brief   pixOrientCorrect()
 *
 * \param[in]    pixs 1 bpp, deskewed, English text, 150 - 300 ppi
 * \param[in]    minupconf minimum value for which a decision can be made
 * \param[in]    minratio minimum conf ratio required for a decision
 * \param[out]   pupconf [optional] ; use NULL to skip
 * \param[out]   pleftconf [optional] ; use NULL to skip
 * \param[out]   protation [optional] ; use NULL to skip
 * \param[in]    debug 1 for debug output; 0 otherwise
 * \return  pixd  may be rotated by 90, 180 or 270; null on error
 *
 * <pre>
 * Notes:
 *      (1) Simple top-level function to detect if Roman text is in
 *          reading orientation, and to rotate the image accordingly if not.
 *      (2) Returns a copy if no rotation is needed.
 *      (3) See notes for pixOrientDetect() and pixOrientDecision().
 *          Use 0.0 for default values for %minupconf and %minratio
 *      (4) Optional output of intermediate confidence results and
 *          the rotation performed on pixs.
 * </pre>
 */
PIX *
pixOrientCorrect(PIX        *pixs,
                 l_float32   minupconf,
                 l_float32   minratio,
                 l_float32  *pupconf,
                 l_float32  *pleftconf,
                 l_int32    *protation,
                 l_int32     debug)
{
l_int32    orient;
l_float32  upconf, leftconf;
PIX       *pix1;

    PROCNAME("pixOrientCorrect");

    if (!pixs || pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs undefined or not 1 bpp", procName, NULL);

        /* Get confidences for orientation */
    pixUpDownDetectDwa(pixs, &upconf, 0, debug);
    pix1 = pixRotate90(pixs, 1);
    pixUpDownDetectDwa(pix1, &leftconf, 0, debug);
    pixDestroy(&pix1);
    if (pupconf) *pupconf = upconf;
    if (pleftconf) *pleftconf = leftconf;

        /* Decide what to do */
    makeOrientDecision(upconf,leftconf, minupconf, minratio, &orient, debug);

        /* Do it */
    switch (orient)
    {
    case L_TEXT_ORIENT_UNKNOWN:
        L_INFO("text orientation not determined; no rotation\n", procName);
        if (protation) *protation = 0;
        return pixCopy(NULL, pixs);
        break;
    case L_TEXT_ORIENT_UP:
        L_INFO("text is oriented up; no rotation\n", procName);
        if (protation) *protation = 0;
        return pixCopy(NULL, pixs);
        break;
    case L_TEXT_ORIENT_LEFT:
        L_INFO("landscape; text oriented left; 90 cw rotation\n", procName);
        if (protation) *protation = 90;
        return pixRotateOrth(pixs, 1);
        break;
    case L_TEXT_ORIENT_DOWN:
        L_INFO("text oriented down; 180 cw rotation\n", procName);
        if (protation) *protation = 180;
        return pixRotateOrth(pixs, 2);
        break;
    case L_TEXT_ORIENT_RIGHT:
        L_INFO("landscape; text oriented right; 270 cw rotation\n", procName);
        if (protation) *protation = 270;
        return pixRotateOrth(pixs, 3);
        break;
    default:
        L_ERROR("invalid orient flag!\n", procName);
        return pixCopy(NULL, pixs);
    }
}


/*----------------------------------------------------------------*
 *         Orientation detection (four 90 degree angles)          *
 *                      Rasterop implementation                   *
 *----------------------------------------------------------------*/
/*!
 * \brief   pixOrientDetect()
 *
 * \param[in]    pixs 1 bpp, deskewed, English text, 150 - 300 ppi
 * \param[out]   pupconf [optional] ; may be NULL
 * \param[out]   pleftconf [optional] ; may be NULL
 * \param[in]    mincount min number of up + down; use 0 for default
 * \param[in]    debug 1 for debug output; 0 otherwise
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) See "Measuring document image skew and orientation"
 *          Dan S. Bloomberg, Gary E. Kopec and Lakshmi Dasari
 *          IS&T/SPIE EI'95, Conference 2422: Document Recognition II
 *          pp 302-316, Feb 6-7, 1995, San Jose, CA
 *      (2) upconf is the normalized difference between up ascenders
 *          and down ascenders.  The image is analyzed without rotation
 *          for being rightside-up or upside-down.  Set &upconf to null
 *          to skip this operation.
 *      (3) leftconf is the normalized difference between up ascenders
 *          and down ascenders in the image after it has been
 *          rotated 90 degrees clockwise.  With that rotation, ascenders
 *          projecting to the left in the source image will project up
 *          in the rotated image.  We compute this by rotating 90 degrees
 *          clockwise and testing for up and down ascenders.  Set
 *          &leftconf to null to skip this operation.
 *      (4) Note that upconf and leftconf are not linear measures of
 *          confidence, e.g., in a range between 0 and 100.  They
 *          measure how far you are out on the tail of a (presumably)
 *          normal distribution.  For example, a confidence of 10 means
 *          that it is nearly certain that the difference did not
 *          happen at random.  However, these values must be interpreted
 *          cautiously, taking into consideration the estimated prior
 *          for a particular orientation or mirror flip.   The up-down
 *          signal is very strong if applied to text with ascenders
 *          up and down, and relatively weak for text at 90 degrees,
 *          but even at 90 degrees, the difference can look significant.
 *          For example, suppose the ascenders are oriented horizontally,
 *          but the test is done vertically.  Then upconf can
 *          be < -MIN_CONF_FOR_UP_DOWN, suggesting the text may be
 *          upside-down.  However, if instead the test were done
 *          horizontally, leftconf will be very much larger
 *          (in absolute value), giving the correct orientation.
 *      (5) If you compute both upconf and leftconf, and there is
 *          sufficient signal, the following table determines the
 *          cw angle necessary to rotate pixs so that the text is
 *          rightside-up:
 *             0 deg :           upconf >> 1,    abs(upconf) >> abs(leftconf)
 *             90 deg :          leftconf >> 1,  abs(leftconf) >> abs(upconf)
 *             180 deg :         upconf << -1,   abs(upconf) >> abs(leftconf)
 *             270 deg :         leftconf << -1, abs(leftconf) >> abs(upconf)
 *      (6) One should probably not interpret the direction unless
 *          there are a sufficient number of counts for both orientations,
 *          in which case neither upconf nor leftconf will be 0.0.
 *      (7) Uses rasterop implementation of HMT.
 * </pre>
 */
l_ok
pixOrientDetect(PIX        *pixs,
                l_float32  *pupconf,
                l_float32  *pleftconf,
                l_int32     mincount,
                l_int32     debug)
{
PIX  *pix1;

    PROCNAME("pixOrientDetect");

    if (!pixs || pixGetDepth(pixs) != 1)
        return ERROR_INT("pixs not defined or not 1 bpp", procName, 1);
    if (!pupconf && !pleftconf)
        return ERROR_INT("nothing to do", procName, 1);
    if (mincount == 0)
        mincount = DEFAULT_MIN_UP_DOWN_COUNT;

    if (pupconf)
        pixUpDownDetect(pixs, pupconf, mincount, debug);
    if (pleftconf) {
        pix1 = pixRotate90(pixs, 1);
        pixUpDownDetect(pix1, pleftconf, mincount, debug);
        pixDestroy(&pix1);
    }

    return 0;
}


/*!
 * \brief   makeOrientDecision()
 *
 * \param[in]    upconf nonzero
 * \param[in]    leftconf nonzero
 * \param[in]    minupconf minimum value for which a decision can be made
 * \param[in]    minratio minimum conf ratio required for a decision
 * \param[out]   porient text orientation enum {0,1,2,3,4}
 * \param[in]    debug 1 for debug output; 0 otherwise
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) This can be run after pixOrientDetect()
 *      (2) Both upconf and leftconf must be nonzero; otherwise the
 *          orientation cannot be determined.
 *      (3) The abs values of the input confidences are compared to
 *          minupconf.
 *      (4) The abs value of the largest of (upconf/leftconf) and
 *          (leftconf/upconf) is compared with minratio.
 *      (5) Input 0.0 for the default values for minupconf and minratio.
 *      (6) The return value of orient is interpreted thus:
 *            L_TEXT_ORIENT_UNKNOWN:  not enough evidence to determine
 *            L_TEXT_ORIENT_UP:       text rightside-up
 *            L_TEXT_ORIENT_LEFT:     landscape, text up facing left
 *            L_TEXT_ORIENT_DOWN:     text upside-down
 *            L_TEXT_ORIENT_RIGHT:    landscape, text up facing right
 * </pre>
 */
l_ok
makeOrientDecision(l_float32  upconf,
                   l_float32  leftconf,
                   l_float32  minupconf,
                   l_float32  minratio,
                   l_int32   *porient,
                   l_int32    debug)
{
l_float32  absupconf, absleftconf;

    PROCNAME("makeOrientDecision");

    if (!porient)
        return ERROR_INT("&orient not defined", procName, 1);
    *porient = L_TEXT_ORIENT_UNKNOWN;  /* default: no decision */
    if (upconf == 0.0 || leftconf == 0.0) {
        L_INFO("not enough confidence to get orientation\n", procName);
        return 0;
    }

    if (minupconf == 0.0)
        minupconf = DEFAULT_MIN_UP_DOWN_CONF;
    if (minratio == 0.0)
        minratio = DEFAULT_MIN_UP_DOWN_RATIO;
    absupconf = L_ABS(upconf);
    absleftconf = L_ABS(leftconf);

        /* Here are the four possible orientation decisions, based
         * on satisfaction of two threshold constraints. */
    if (upconf > minupconf && absupconf > minratio * absleftconf)
        *porient = L_TEXT_ORIENT_UP;
    else if (leftconf > minupconf && absleftconf > minratio * absupconf)
        *porient = L_TEXT_ORIENT_LEFT;
    else if (upconf < -minupconf && absupconf > minratio * absleftconf)
        *porient = L_TEXT_ORIENT_DOWN;
    else if (leftconf < -minupconf && absleftconf > minratio * absupconf)
        *porient = L_TEXT_ORIENT_RIGHT;

    if (debug) {
        fprintf(stderr, "upconf = %7.3f, leftconf = %7.3f\n", upconf, leftconf);
        if (*porient == L_TEXT_ORIENT_UNKNOWN)
            fprintf(stderr, "Confidence is low; no determination is made\n");
        else if (*porient == L_TEXT_ORIENT_UP)
            fprintf(stderr, "Text is rightside-up\n");
        else if (*porient == L_TEXT_ORIENT_LEFT)
            fprintf(stderr, "Text is rotated 90 deg ccw\n");
        else if (*porient == L_TEXT_ORIENT_DOWN)
            fprintf(stderr, "Text is upside-down\n");
        else   /* *porient == L_TEXT_ORIENT_RIGHT */
            fprintf(stderr, "Text is rotated 90 deg cw\n");
    }

    return 0;
}


/*!
 * \brief   pixUpDownDetect()
 *
 * \param[in]    pixs 1 bpp, deskewed, English text, 150 - 300 ppi
 * \param[out]   pconf confidence that text is rightside-up
 * \param[in]    mincount min number of up + down; use 0 for default
 * \param[in]    debug 1 for debug output; 0 otherwise
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) Special (typical, slightly faster) case, where the pixels
 *          identified through the HMT (hit-miss transform) are not
 *          clipped by a truncated word mask pixm.  See pixOrientDetect()
 *          and pixUpDownDetectGeneral() for details.
 *      (2) The returned confidence is the normalized difference
 *          between the number of detected up and down ascenders,
 *          assuming that the text is either rightside-up or upside-down
 *          and not rotated at a 90 degree angle.
 * </pre>
 */
l_ok
pixUpDownDetect(PIX        *pixs,
                l_float32  *pconf,
                l_int32     mincount,
                l_int32     debug)
{
    return pixUpDownDetectGeneral(pixs, pconf, mincount, 0, debug);
}


/*!
 * \brief   pixUpDownDetectGeneral()
 *
 * \param[in]    pixs 1 bpp, deskewed, English text, 150 - 300 ppi
 * \param[out]   pconf confidence that text is rightside-up
 * \param[in]    mincount min number of up + down; use 0 for default
 * \param[in]    npixels number of pixels removed from each side of word box
 * \param[in]    debug 1 for debug output; 0 otherwise
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) See pixOrientDetect() for other details.
 *      (2) %conf is the normalized difference between the number of
 *          detected up and down ascenders, assuming that the text
 *          is either rightside-up or upside-down and not rotated
 *          at a 90 degree angle.
 *      (3) The typical mode of operation is %npixels == 0.
 *          If %npixels > 0, this removes HMT matches at the
 *          beginning and ending of "words."  This is useful for
 *          pages that may have mostly digits, because if npixels == 0,
 *          leading "1" and "3" digits can register as having
 *          ascenders or descenders, and "7" digits can match descenders.
 *          Consequently, a page image of only digits may register
 *          as being upside-down.
 *      (4) We want to count the number of instances found using the HMT.
 *          An expensive way to do this would be to count the
 *          number of connected components.  A cheap way is to do a rank
 *          reduction cascade that reduces each component to a single
 *          pixel, and results (after two or three 2x reductions)
 *          in one pixel for each of the original components.
 *          After the reduction, you have a much smaller pix over
 *          which to count pixels.  We do only 2 reductions, because
 *          this function is designed to work for input pix between
 *          150 and 300 ppi, and an 8x reduction on a 150 ppi image
 *          is going too far -- components will get merged.
 * </pre>
 */
l_ok
pixUpDownDetectGeneral(PIX        *pixs,
                       l_float32  *pconf,
                       l_int32     mincount,
                       l_int32     npixels,
                       l_int32     debug)
{
l_int32    countup, countdown, nmax;
l_float32  nup, ndown;
PIX       *pix0, *pix1, *pix2, *pix3, *pixm;
SEL       *sel1, *sel2, *sel3, *sel4;

    PROCNAME("pixUpDownDetectGeneral");

    if (!pconf)
        return ERROR_INT("&conf not defined", procName, 1);
    *pconf = 0.0;
    if (!pixs || pixGetDepth(pixs) != 1)
        return ERROR_INT("pixs not defined or not 1 bpp", procName, 1);
    if (mincount == 0)
        mincount = DEFAULT_MIN_UP_DOWN_COUNT;
    if (npixels < 0)
        npixels = 0;

    if (debug) {
        lept_mkdir("lept/orient");
    }

    sel1 = selCreateFromString(textsel1, 5, 6, NULL);
    sel2 = selCreateFromString(textsel2, 5, 6, NULL);
    sel3 = selCreateFromString(textsel3, 5, 6, NULL);
    sel4 = selCreateFromString(textsel4, 5, 6, NULL);

        /* One of many reasonable pre-filtering sequences: (1, 8) and (30, 1).
         * This closes holes in x-height characters and joins them at
         * the x-height.  There is more noise in the descender detection
         * from this, but it works fairly well. */
    pix0 = pixMorphCompSequence(pixs, "c1.8 + c30.1", 0);

        /* Optionally, make a mask of the word bounding boxes, shortening
         * each of them by a fixed amount at each end. */
    pixm = NULL;
    if (npixels > 0) {
        l_int32  i, nbox, x, y, w, h;
        BOX   *box;
        BOXA  *boxa;
        pix1 = pixMorphSequence(pix0, "o10.1", 0);
        boxa = pixConnComp(pix1, NULL, 8);
        pixm = pixCreateTemplate(pix1);
        pixDestroy(&pix1);
        nbox = boxaGetCount(boxa);
        for (i = 0; i < nbox; i++) {
            box = boxaGetBox(boxa, i, L_CLONE);
            boxGetGeometry(box, &x, &y, &w, &h);
            if (w > 2 * npixels)
                pixRasterop(pixm, x + npixels, y - 6, w - 2 * npixels, h + 13,
                            PIX_SET, NULL, 0, 0);
            boxDestroy(&box);
        }
        boxaDestroy(&boxa);
    }

        /* Find the ascenders and optionally filter with pixm.
         * For an explanation of the procedure used for counting the result
         * of the HMT, see comments at the beginning of this function. */
    pix1 = pixHMT(NULL, pix0, sel1);
    pix2 = pixHMT(NULL, pix0, sel2);
    pixOr(pix1, pix1, pix2);
    if (pixm)
        pixAnd(pix1, pix1, pixm);
    pix3 = pixReduceRankBinaryCascade(pix1, 1, 1, 0, 0);
    pixCountPixels(pix3, &countup, NULL);
    pixDebugFlipDetect("/tmp/lept/orient/up.png", pixs, pix1, debug);
    pixDestroy(&pix1);
    pixDestroy(&pix2);
    pixDestroy(&pix3);

        /* Find the ascenders and optionally filter with pixm. */
    pix1 = pixHMT(NULL, pix0, sel3);
    pix2 = pixHMT(NULL, pix0, sel4);
    pixOr(pix1, pix1, pix2);
    if (pixm)
        pixAnd(pix1, pix1, pixm);
    pix3 = pixReduceRankBinaryCascade(pix1, 1, 1, 0, 0);
    pixCountPixels(pix3, &countdown, NULL);
    pixDebugFlipDetect("/tmp/lept/orient/down.png", pixs, pix1, debug);
    pixDestroy(&pix1);
    pixDestroy(&pix2);
    pixDestroy(&pix3);

        /* Evaluate statistically, generating a confidence that is
         * related to the probability with a gaussian distribution. */
    nup = (l_float32)(countup);
    ndown = (l_float32)(countdown);
    nmax = L_MAX(countup, countdown);
    if (nmax > mincount)
        *pconf = 2. * ((nup - ndown) / sqrt(nup + ndown));

    if (debug) {
        if (pixm) pixWriteDebug("/tmp/lept/orient/pixm1.png", pixm, IFF_PNG);
        fprintf(stderr, "nup = %7.3f, ndown = %7.3f, conf = %7.3f\n",
                nup, ndown, *pconf);
        if (*pconf > DEFAULT_MIN_UP_DOWN_CONF)
            fprintf(stderr, "Text is rightside-up\n");
        if (*pconf < -DEFAULT_MIN_UP_DOWN_CONF)
            fprintf(stderr, "Text is upside-down\n");
    }

    pixDestroy(&pix0);
    pixDestroy(&pixm);
    selDestroy(&sel1);
    selDestroy(&sel2);
    selDestroy(&sel3);
    selDestroy(&sel4);
    return 0;
}


/*----------------------------------------------------------------*
 *         Orientation detection (four 90 degree angles)          *
 *                         DWA implementation                     *
 *----------------------------------------------------------------*/
/*!
 * \brief   pixOrientDetectDwa()
 *
 * \param[in]    pixs 1 bpp, deskewed, English text
 * \param[out]   pupconf [optional] ; may be NULL
 * \param[out]   pleftconf [optional] ; may be NULL
 * \param[in]    mincount min number of up + down; use 0 for default
 * \param[in]    debug 1 for debug output; 0 otherwise
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) Same interface as for pixOrientDetect().  See notes
 *          there for usage.
 *      (2) Uses auto-gen'd code for the Sels defined at the
 *          top of this file, with some renaming of functions.
 *          The auto-gen'd code is in fliphmtgen.c, and can
 *          be generated by a simple executable; see prog/flipselgen.c.
 *      (3) This runs about 2.5 times faster than the pixOrientDetect().
 * </pre>
 */
l_ok
pixOrientDetectDwa(PIX        *pixs,
                   l_float32  *pupconf,
                   l_float32  *pleftconf,
                   l_int32     mincount,
                   l_int32     debug)
{
PIX  *pix1;

    PROCNAME("pixOrientDetectDwa");

    if (!pixs || pixGetDepth(pixs) != 1)
        return ERROR_INT("pixs not defined or not 1 bpp", procName, 1);
    if (!pupconf && !pleftconf)
        return ERROR_INT("nothing to do", procName, 1);
    if (mincount == 0)
        mincount = DEFAULT_MIN_UP_DOWN_COUNT;

    if (pupconf)
        pixUpDownDetectDwa(pixs, pupconf, mincount, debug);
    if (pleftconf) {
        pix1 = pixRotate90(pixs, 1);
        pixUpDownDetectDwa(pix1, pleftconf, mincount, debug);
        pixDestroy(&pix1);
    }

    return 0;
}


/*!
 * \brief   pixUpDownDetectDwa()
 *
 * \param[in]    pixs 1 bpp, deskewed, English text, 150 - 300 ppi
 * \param[out]   pconf confidence that text is rightside-up
 * \param[in]    mincount min number of up + down; use 0 for default
 * \param[in]    debug 1 for debug output; 0 otherwise
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) Faster (DWA) version of pixUpDownDetect().
 *      (2) This is a special case (but typical and slightly faster) of
 *          pixUpDownDetectGeneralDwa(), where the pixels identified
 *          through the HMT (hit-miss transform) are not clipped by
 *          a truncated word mask pixm.  See pixUpDownDetectGeneral()
 *          for usage and other details.
 *      (3) The returned confidence is the normalized difference
 *          between the number of detected up and down ascenders,
 *          assuming that the text is either rightside-up or upside-down
 *          and not rotated at a 90 degree angle.
 * </pre>
 */
l_ok
pixUpDownDetectDwa(PIX       *pixs,
                  l_float32  *pconf,
                  l_int32     mincount,
                  l_int32     debug)
{
    return pixUpDownDetectGeneralDwa(pixs, pconf, mincount, 0, debug);
}


/*!
 * \brief   pixUpDownDetectGeneralDwa()
 *
 * \param[in]    pixs 1 bpp, deskewed, English text
 * \param[out]   pconf confidence that text is rightside-up
 * \param[in]    mincount min number of up + down; use 0 for default
 * \param[in]    npixels number of pixels removed from each side of word box
 * \param[in]    debug 1 for debug output; 0 otherwise
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) See the notes in pixUpDownDetectGeneral() for usage.
 * </pre>
 */
l_ok
pixUpDownDetectGeneralDwa(PIX        *pixs,
                          l_float32  *pconf,
                          l_int32     mincount,
                          l_int32     npixels,
                          l_int32     debug)
{
char       flipsel1[] = "flipsel1";
char       flipsel2[] = "flipsel2";
char       flipsel3[] = "flipsel3";
char       flipsel4[] = "flipsel4";
l_int32    countup, countdown, nmax;
l_float32  nup, ndown;
PIX       *pixt, *pix0, *pix1, *pix2, *pix3, *pixm;

    PROCNAME("pixUpDownDetectGeneralDwa");

    if (!pconf)
        return ERROR_INT("&conf not defined", procName, 1);
    *pconf = 0.0;
    if (!pixs || pixGetDepth(pixs) != 1)
        return ERROR_INT("pixs not defined or not 1 bpp", procName, 1);
    if (mincount == 0)
        mincount = DEFAULT_MIN_UP_DOWN_COUNT;
    if (npixels < 0)
        npixels = 0;

        /* One of many reasonable pre-filtering sequences: (1, 8) and (30, 1).
         * This closes holes in x-height characters and joins them at
         * the x-height.  There is more noise in the descender detection
         * from this, but it works fairly well. */
    pixt = pixMorphSequenceDwa(pixs, "c1.8 + c30.1", 0);

        /* Be sure to add the border before the flip DWA operations! */
    pix0 = pixAddBorderGeneral(pixt, ADDED_BORDER, ADDED_BORDER,
                                ADDED_BORDER, ADDED_BORDER, 0);
    pixDestroy(&pixt);

        /* Optionally, make a mask of the word bounding boxes, shortening
         * each of them by a fixed amount at each end. */
    pixm = NULL;
    if (npixels > 0) {
        l_int32  i, nbox, x, y, w, h;
        BOX   *box;
        BOXA  *boxa;
        pix1 = pixMorphSequenceDwa(pix0, "o10.1", 0);
        boxa = pixConnComp(pix1, NULL, 8);
        pixm = pixCreateTemplate(pix1);
        pixDestroy(&pix1);
        nbox = boxaGetCount(boxa);
        for (i = 0; i < nbox; i++) {
            box = boxaGetBox(boxa, i, L_CLONE);
            boxGetGeometry(box, &x, &y, &w, &h);
            if (w > 2 * npixels)
                pixRasterop(pixm, x + npixels, y - 6, w - 2 * npixels, h + 13,
                            PIX_SET, NULL, 0, 0);
            boxDestroy(&box);
        }
        boxaDestroy(&boxa);
    }

        /* Find the ascenders and optionally filter with pixm.
         * For an explanation of the procedure used for counting the result
         * of the HMT, see comments in pixUpDownDetectGeneral().  */
    pix1 = pixFlipFHMTGen(NULL, pix0, flipsel1);
    pix2 = pixFlipFHMTGen(NULL, pix0, flipsel2);
    pixOr(pix1, pix1, pix2);
    if (pixm)
        pixAnd(pix1, pix1, pixm);
    pix3 = pixReduceRankBinaryCascade(pix1, 1, 1, 0, 0);
    pixCountPixels(pix3, &countup, NULL);
    pixDestroy(&pix1);
    pixDestroy(&pix2);
    pixDestroy(&pix3);

        /* Find the ascenders and optionally filter with pixm. */
    pix1 = pixFlipFHMTGen(NULL, pix0, flipsel3);
    pix2 = pixFlipFHMTGen(NULL, pix0, flipsel4);
    pixOr(pix1, pix1, pix2);
    if (pixm)
        pixAnd(pix1, pix1, pixm);
    pix3 = pixReduceRankBinaryCascade(pix1, 1, 1, 0, 0);
    pixCountPixels(pix3, &countdown, NULL);
    pixDestroy(&pix1);
    pixDestroy(&pix2);
    pixDestroy(&pix3);

        /* Evaluate statistically, generating a confidence that is
         * related to the probability with a gaussian distribution. */
    nup = (l_float32)(countup);
    ndown = (l_float32)(countdown);
    nmax = L_MAX(countup, countdown);
    if (nmax > mincount)
        *pconf = 2. * ((nup - ndown) / sqrt(nup + ndown));

    if (debug) {
        if (pixm) {
            lept_mkdir("lept/orient");
            pixWriteDebug("/tmp/lept/orient/pixm2.png", pixm, IFF_PNG);
        }
        fprintf(stderr, "nup = %7.3f, ndown = %7.3f, conf = %7.3f\n",
                nup, ndown, *pconf);
        if (*pconf > DEFAULT_MIN_UP_DOWN_CONF)
            fprintf(stderr, "Text is rightside-up\n");
        if (*pconf < -DEFAULT_MIN_UP_DOWN_CONF)
            fprintf(stderr, "Text is upside-down\n");
    }

    pixDestroy(&pix0);
    pixDestroy(&pixm);
    return 0;
}



/*----------------------------------------------------------------*
 *                     Left-right mirror detection                *
 *                       Rasterop implementation                  *
 *----------------------------------------------------------------*/
/*!
 * \brief   pixMirrorDetect()
 *
 * \param[in]    pixs 1 bpp, deskewed, English text
 * \param[out]   pconf confidence that text is not LR mirror reversed
 * \param[in]    mincount min number of left + right; use 0 for default
 * \param[in]    debug 1 for debug output; 0 otherwise
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) For this test, it is necessary that the text is horizontally
 *          oriented, with ascenders going up.
 *      (2) conf is the normalized difference between the number of
 *          right and left facing characters with ascenders.
 *          Left-facing are {d}; right-facing are {b, h, k}.
 *          At least that was the expectation.  In practice, we can
 *          really just say that it is the normalized difference in
 *          hits using two specific hit-miss filters, textsel1 and textsel2,
 *          after the image has been suitably pre-filtered so that
 *          these filters are effective.  See (4) for what's really happening.
 *      (3) A large positive conf value indicates normal text, whereas
 *          a large negative conf value means the page is mirror reversed.
 *      (4) The implementation is a bit tricky.  The general idea is
 *          to fill the x-height part of characters, but not the space
 *          between them, before doing the HMT.  This is done by
 *          finding pixels added using two different operations -- a
 *          horizontal close and a vertical dilation -- and adding
 *          the intersection of these sets to the original.  It turns
 *          out that the original intuition about the signal was largely
 *          in error: much of the signal for right-facing characters
 *          comes from the lower part of common x-height characters, like
 *          the e and c, that remain open after these operations.
 *          So it's important that the operations to close the x-height
 *          parts of the characters are purposely weakened sufficiently
 *          to allow these characters to remain open.  The wonders
 *          of morphology!
 * </pre>
 */
l_ok
pixMirrorDetect(PIX        *pixs,
                l_float32  *pconf,
                l_int32     mincount,
                l_int32     debug)
{
l_int32    count1, count2, nmax;
l_float32  nleft, nright;
PIX       *pix0, *pix1, *pix2, *pix3;
SEL       *sel1, *sel2;

    PROCNAME("pixMirrorDetect");

    if (!pconf)
        return ERROR_INT("&conf not defined", procName, 1);
    *pconf = 0.0;
    if (!pixs || pixGetDepth(pixs) != 1)
        return ERROR_INT("pixs not defined or not 1 bpp", procName, 1);
    if (mincount == 0)
        mincount = DEFAULT_MIN_MIRROR_FLIP_COUNT;

    if (debug) {
        lept_mkdir("lept/orient");
    }

    sel1 = selCreateFromString(textsel1, 5, 6, NULL);
    sel2 = selCreateFromString(textsel2, 5, 6, NULL);

        /* Fill x-height characters but not space between them, sort of. */
    pix3 = pixMorphCompSequence(pixs, "d1.30", 0);
    pixXor(pix3, pix3, pixs);
    pix0 = pixMorphCompSequence(pixs, "c15.1", 0);
    pixXor(pix0, pix0, pixs);
    pixAnd(pix0, pix0, pix3);
    pixOr(pix0, pix0, pixs);
    pixDestroy(&pix3);

        /* Filter the right-facing characters. */
    pix1 = pixHMT(NULL, pix0, sel1);
    pix3 = pixReduceRankBinaryCascade(pix1, 1, 1, 0, 0);
    pixCountPixels(pix3, &count1, NULL);
    pixDebugFlipDetect("/tmp/lept/orient/right.png", pixs, pix1, debug);
    pixDestroy(&pix1);
    pixDestroy(&pix3);

        /* Filter the left-facing characters. */
    pix2 = pixHMT(NULL, pix0, sel2);
    pix3 = pixReduceRankBinaryCascade(pix2, 1, 1, 0, 0);
    pixCountPixels(pix3, &count2, NULL);
    pixDebugFlipDetect("/tmp/lept/orient/left.png", pixs, pix2, debug);
    pixDestroy(&pix2);
    pixDestroy(&pix3);

    nright = (l_float32)count1;
    nleft = (l_float32)count2;
    nmax = L_MAX(count1, count2);
    pixDestroy(&pix0);
    selDestroy(&sel1);
    selDestroy(&sel2);

    if (nmax > mincount)
        *pconf = 2. * ((nright - nleft) / sqrt(nright + nleft));

    if (debug) {
        fprintf(stderr, "nright = %f, nleft = %f\n", nright, nleft);
        if (*pconf > DEFAULT_MIN_MIRROR_FLIP_CONF)
            fprintf(stderr, "Text is not mirror reversed\n");
        if (*pconf < -DEFAULT_MIN_MIRROR_FLIP_CONF)
            fprintf(stderr, "Text is mirror reversed\n");
    }

    return 0;
}


/*----------------------------------------------------------------*
 *                     Left-right mirror detection                *
 *                          DWA implementation                    *
 *----------------------------------------------------------------*/
/*!
 * \brief   pixMirrorDetectDwa()
 *
 * \param[in]    pixs 1 bpp, deskewed, English text
 * \param[out]   pconf confidence that text is not LR mirror reversed
 * \param[in]    mincount min number of left + right; use 0 for default
 * \param[in]    debug 1 for debug output; 0 otherwise
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) We assume the text is horizontally oriented, with
 *          ascenders going up.
 *      (2) See notes in pixMirrorDetect().
 * </pre>
 */
l_ok
pixMirrorDetectDwa(PIX        *pixs,
                   l_float32  *pconf,
                   l_int32     mincount,
                   l_int32     debug)
{
char       flipsel1[] = "flipsel1";
char       flipsel2[] = "flipsel2";
l_int32    count1, count2, nmax;
l_float32  nleft, nright;
PIX       *pix0, *pix1, *pix2, *pix3;

    PROCNAME("pixMirrorDetectDwa");

    if (!pconf)
        return ERROR_INT("&conf not defined", procName, 1);
    *pconf = 0.0;
    if (!pixs || pixGetDepth(pixs) != 1)
        return ERROR_INT("pixs not defined or not 1 bpp", procName, 1);
    if (mincount == 0)
        mincount = DEFAULT_MIN_MIRROR_FLIP_COUNT;

        /* Fill x-height characters but not space between them, sort of. */
    pix3 = pixMorphSequenceDwa(pixs, "d1.30", 0);
    pixXor(pix3, pix3, pixs);
    pix0 = pixMorphSequenceDwa(pixs, "c15.1", 0);
    pixXor(pix0, pix0, pixs);
    pixAnd(pix0, pix0, pix3);
    pixOr(pix3, pix0, pixs);
    pixDestroy(&pix0);
    pix0 = pixAddBorderGeneral(pix3, ADDED_BORDER, ADDED_BORDER,
                                ADDED_BORDER, ADDED_BORDER, 0);
    pixDestroy(&pix3);

        /* Filter the right-facing characters. */
    pix1 = pixFlipFHMTGen(NULL, pix0, flipsel1);
    pix3 = pixReduceRankBinaryCascade(pix1, 1, 1, 0, 0);
    pixCountPixels(pix3, &count1, NULL);
    pixDestroy(&pix1);
    pixDestroy(&pix3);

        /* Filter the left-facing characters. */
    pix2 = pixFlipFHMTGen(NULL, pix0, flipsel2);
    pix3 = pixReduceRankBinaryCascade(pix2, 1, 1, 0, 0);
    pixCountPixels(pix3, &count2, NULL);
    pixDestroy(&pix2);
    pixDestroy(&pix3);

    pixDestroy(&pix0);
    nright = (l_float32)count1;
    nleft = (l_float32)count2;
    nmax = L_MAX(count1, count2);

    if (nmax > mincount)
        *pconf = 2. * ((nright - nleft) / sqrt(nright + nleft));

    if (debug) {
        fprintf(stderr, "nright = %f, nleft = %f\n", nright, nleft);
        if (*pconf > DEFAULT_MIN_MIRROR_FLIP_CONF)
            fprintf(stderr, "Text is not mirror reversed\n");
        if (*pconf < -DEFAULT_MIN_MIRROR_FLIP_CONF)
            fprintf(stderr, "Text is mirror reversed\n");
    }

    return 0;
}


/*----------------------------------------------------------------*
 *                        Static debug helper                     *
 *----------------------------------------------------------------*/
/*
 *  pixDebugFlipDetect()
 *
 *      Input:  filename (for output debug file)
 *              pixs (input to pix*Detect)
 *              pixhm (hit-miss result from ascenders or descenders)
 *              enable (1 to enable this function; 0 to disable)
 *      Return: void
 */
static void
pixDebugFlipDetect(const char *filename,
                   PIX        *pixs,
                   PIX        *pixhm,
                   l_int32     enable)
{
PIX  *pixt, *pixthm;

   if (!enable) return;

        /* Display with red dot at counted locations */
    pixt = pixConvert1To4Cmap(pixs);
    pixthm = pixMorphSequence(pixhm, "d5.5", 0);
    pixSetMaskedCmap(pixt, pixthm, 0, 0, 255, 0, 0);

    pixWriteDebug(filename, pixt, IFF_PNG);
    pixDestroy(&pixthm);
    pixDestroy(&pixt);
    return;
}
