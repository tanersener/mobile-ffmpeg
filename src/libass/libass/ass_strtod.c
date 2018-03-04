/*
 * Copyright (c) 1988-1993 The Regents of the University of California.
 * Copyright (c) 1994 Sun Microsystems, Inc.
 * Copyright (c) 2016 Oleg Oshmyan <chortos@inbox.lv>
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 */

#include "config.h"
#include "ass_compat.h"

#include <stdlib.h>
#include <float.h>
#include <errno.h>
#include "ass_string.h"

static
const size_t maxExponent = 511; /* Largest possible base 10 exponent.  Any
                                 * exponent larger than this will already
                                 * produce underflow or overflow, so there's
                                 * no need to worry about additional digits.
                                 */

static
const double powersOf10[] = {   /* Table giving binary powers of 10.  Entry */
    10.,                        /* is 10^2^i.  Used to convert decimal */
    100.,                       /* exponents into floating-point numbers. */
    1.0e4,
    1.0e8,
    1.0e16,
    1.0e32,
    1.0e64,
    1.0e128,
    1.0e256
};

static
const double negPowOf10[] = {   /* Table giving negative binary powers */
    0.1,                        /* of 10.  Entry is 10^-2^i. */
    0.01,                       /* Used to convert decimal exponents */
    1.0e-4,                     /* into floating-point numbers. */
    1.0e-8,
    1.0e-16,
    1.0e-32,
    1.0e-64,
    1.0e-128,
    1.0e-256
};

/*
 *----------------------------------------------------------------------
 *
 * strtod --
 *
 * This procedure converts a floating-point number from an ASCII
 * decimal representation to internal double-precision format.
 *
 * Results:
 * The return value is the double-precision floating-point
 * representation of the characters in string.  If endPtr isn't
 * NULL, then *endPtr is filled in with the address of the
 * next character after the last one that was part of the
 * floating-point number.
 *
 * Side effects:
 * None.
 *
 *----------------------------------------------------------------------
 */

double
ass_strtod(
    const char *string,     /* A decimal ASCII floating-point number,
                             * optionally preceded by white space.
                             * Must have form "-I.FE-X", where I is the
                             * integer part of the mantissa, F is the
                             * fractional part of the mantissa, and X
                             * is the exponent.  Either of the signs
                             * may be "+", "-", or omitted.  Either I
                             * or F may be omitted, or both.  The decimal
                             * point isn't necessary unless F is present.
                             * The "E" may actually be an "e".  E and X
                             * may both be omitted (but not just one).
                             */
    char **endPtr           /* If non-NULL, store terminating character's
                             * address here. */
    )
{
    int sign, fracExpSign, expSign;
    double fraction, dblExp;
    const double *d;
    register const char *p;
    register int c;
    size_t exp = 0;         /* Exponent read from "EX" field. */
    size_t fracExp;         /* Exponent that derives from the fractional
                             * part.  Under normal circumstatnces, it is
                             * the negative of the number of digits in F.
                             * However, if I is very long, the last digits
                             * of I get dropped (otherwise a long I with a
                             * large negative exponent could cause an
                             * unnecessary overflow on I alone).  In this
                             * case, fracExp is incremented one for each
                             * dropped digit. */
    size_t mantSize;    /* Number of digits in mantissa. */
    size_t decPt;       /* Number of mantissa digits BEFORE decimal
                         * point. */
    size_t leadZeros;   /* Number of leading zeros in mantissa. */
    const char *pExp;       /* Temporarily holds location of exponent
                             * in string. */

    /*
     * Strip off leading blanks and check for a sign.
     */

    p = string;
    while (ass_isspace(*p)) {
        p += 1;
    }
    if (*p == '-') {
        sign = 1;
        p += 1;
    } else {
        if (*p == '+') {
            p += 1;
        }
        sign = 0;
    }

    /*
     * Count the number of digits in the mantissa (including the decimal
     * point), and also locate the decimal point.
     */

    decPt = -1;
    leadZeros = -1;
    for (mantSize = 0; ; mantSize += 1)
    {
        c = *p;
        if (!ass_isdigit(c)) {
            if ((c != '.') || (decPt != (size_t) -1)) {
                break;
            }
            decPt = mantSize;
        } else if ((c != '0') && (leadZeros == (size_t) -1)) {
            leadZeros = mantSize;
        }
        p += 1;
    }

    /*
     * Now suck up the digits in the mantissa.  Use two integers to
     * collect 9 digits each (this is faster than using floating-point).
     * If the mantissa has more than 18 digits, ignore the extras, since
     * they can't affect the value anyway.
     */

    if (leadZeros == (size_t) -1) {
        leadZeros = mantSize;
    }
    pExp  = p;
    p -= mantSize - leadZeros;
    if (decPt == (size_t) -1) {
        decPt = mantSize;
    } else {
        mantSize -= 1;      /* One of the digits was the point. */
        if (decPt < leadZeros) {
            leadZeros -= 1;
        }
    }
    if (mantSize - leadZeros > 18) {
        mantSize = leadZeros + 18;
    }
    if (decPt < mantSize) {
        fracExpSign = 1;
        fracExp = mantSize - decPt;
    } else {
        fracExpSign = 0;
        fracExp = decPt - mantSize;
    }
    if (mantSize == 0) {
        fraction = 0.0;
        p = string;
        goto done;
    } else {
        int frac1, frac2, m;
        mantSize -= leadZeros;
        m = mantSize;
        frac1 = 0;
        for ( ; m > 9; m -= 1)
        {
            c = *p;
            p += 1;
            if (c == '.') {
                c = *p;
                p += 1;
            }
            frac1 = 10*frac1 + (c - '0');
        }
        frac2 = 0;
        for (; m > 0; m -= 1)
        {
            c = *p;
            p += 1;
            if (c == '.') {
                c = *p;
                p += 1;
            }
            frac2 = 10*frac2 + (c - '0');
        }
        fraction = (1.0e9 * frac1) + frac2;
    }

    /*
     * Skim off the exponent.
     */

    p = pExp;
    if ((*p == 'E') || (*p == 'e')) {
        size_t expLimit;    /* If exp > expLimit, appending another digit
                             * to exp is guaranteed to make it too large.
                             * If exp == expLimit, this may depend on
                             * the exact digit, but in any case exp with
                             * the digit appended and fracExp added will
                             * still fit in size_t, even if it does
                             * exceed maxExponent. */
        int expWraparound = 0;
        p += 1;
        if (*p == '-') {
            expSign = 1;
            p += 1;
        } else {
            if (*p == '+') {
                p += 1;
            }
            expSign = 0;
        }
        if (expSign == fracExpSign) {
            if (maxExponent < fracExp) {
                expLimit = 0;
            } else {
                expLimit = (maxExponent - fracExp) / 10;
            }
        } else {
            expLimit = fracExp / 10 + (fracExp % 10 + maxExponent) / 10;
        }
        while (ass_isdigit(*p)) {
            if ((exp > expLimit) || expWraparound) {
                do {
                    p += 1;
                } while (ass_isdigit(*p));
                goto expOverflow;
            } else if (exp > ((size_t) -1 - (*p - '0')) / 10) {
                expWraparound = 1;
            }
            exp = exp * 10 + (*p - '0');
            p += 1;
        }
        if (expSign == fracExpSign) {
            exp = fracExp + exp;
        } else if ((fracExp <= exp) || expWraparound) {
            exp = exp - fracExp;
        } else {
            exp = fracExp - exp;
            expSign = fracExpSign;
        }
    } else {
        exp = fracExp;
        expSign = fracExpSign;
    }

    /*
     * Generate a floating-point number that represents the exponent.
     * Do this by processing the exponent one bit at a time to combine
     * many powers of 2 of 10. Then combine the exponent with the
     * fraction.
     */

    if (exp > maxExponent) {
expOverflow:
        exp = maxExponent;
        if (fraction != 0.0) {
            errno = ERANGE;
        }
    }
    /* Prefer positive powers of 10 for increased precision, especially
     * for small powers that are represented exactly in floating-point. */
    if ((exp <= DBL_MAX_10_EXP) || !expSign) {
        d = powersOf10;
    } else {
        /* The floating-point format supports more negative exponents
         * than positive, or perhaps the result is a subnormal number. */
        if (exp > -DBL_MIN_10_EXP) {
            /* The result might be a valid subnormal number, but the
             * exponent underflows.  Tweak fraction so that it is below
             * 1.0 first, so that if the exponent still underflows after
             * that, the result is sure to underflow as well. */
            exp -= mantSize;
            dblExp = 1.0;
            for (d = powersOf10; mantSize != 0; mantSize >>= 1, d += 1) {
                if (mantSize & 01) {
                    dblExp *= *d;
                }
            }
            fraction /= dblExp;
        }
        d = negPowOf10;
        expSign = 0;
    }
    dblExp = 1.0;
    for (; exp != 0; exp >>= 1, d += 1) {
        if (exp & 01) {
            dblExp *= *d;
        }
    }
    if (expSign) {
        fraction /= dblExp;
    } else {
        fraction *= dblExp;
    }

done:
    if (endPtr != NULL) {
        *endPtr = (char *) p;
    }

    if (sign) {
        return -fraction;
    }
    return fraction;
}
