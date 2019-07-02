#!/bin/sh

# Copyright (C) 2004-2006, 2008-2010, 2012 Free Software Foundation,
# Inc.
#
# Author: Simon Josefsson
#
# This file is part of GnuTLS.
#
# GnuTLS is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; either version 3 of the License, or (at
# your option) any later version.
#
# GnuTLS is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GnuTLS; if not, write to the Free Software Foundation,
# Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

set -e

srcdir="${srcdir:-.}"
CERTTOOL="${CERTTOOL:-../../src/certtool${EXEEXT}}"

if ! test -x "${CERTTOOL}"; then
	exit 77
fi

export TZ="UTC"

. ${srcdir}/../scripts/common.sh

check_for_datefudge

TMPFILE1=pkcs1-pad.$$.tmp
TMPFILE2=pkcs1-pad-2.$$.tmp

# Test 1, PKCS#1 pad digestAlgorithm.parameters

EXPECT1=2002

datefudge "2006-09-23" "${CERTTOOL}" --verify-allow-broken --verify-chain --infile "${srcdir}/data/pkcs1-pad-ok.pem" | tee $TMPFILE1 >/dev/null 2>&1
datefudge "2006-09-23" "${CERTTOOL}" --verify-allow-broken --verify-chain --infile "${srcdir}/data/pkcs1-pad-broken.pem" | tee $TMPFILE2 >/dev/null 2>&1

out1oks=`grep 'Verified.' $TMPFILE1 | wc -l | tr -d " "`
out2oks=`grep 'Verified.' $TMPFILE2 | wc -l | tr -d " "`
out1fails=`grep 'Not verified.' $TMPFILE1 | wc -l | tr -d " "`
out2fails=`grep 'Not verified.' $TMPFILE2 | wc -l | tr -d " "`

if test "${out1oks}${out2oks}${out1fails}${out2fails}" != "${EXPECT1}"; then
	echo "$TMPFILE1 oks ${out1oks} fails ${out1fails} $TMPFILE2 oks ${out2oks} fails ${out2fails}"
	echo "expected ${EXPECT1}"
	echo "PKCS1-PAD1 FAIL"
	exit 1
fi

rm -f $TMPFILE1 $TMPFILE2

echo "PKCS1-PAD1 OK"

# Test 2, Bleichenbacher's Crypto 06 rump session

EXPECT2=2002

datefudge "2006-09-23" "${CERTTOOL}" --verify-chain --infile "${srcdir}/data/pkcs1-pad-ok2.pem" | tee $TMPFILE1 >/dev/null 2>&1
datefudge "2006-09-23" "${CERTTOOL}" --verify-chain --infile "${srcdir}/data/pkcs1-pad-broken2.pem" | tee $TMPFILE2 >/dev/null 2>&1

out1oks=`grep 'Verified.' $TMPFILE1 | wc -l | tr -d " "`
out2oks=`grep 'Verified.' $TMPFILE2 | wc -l | tr -d " "`
out1fails=`grep 'Not verified.' $TMPFILE1 | wc -l | tr -d " "`
out2fails=`grep 'Not verified.' $TMPFILE2 | wc -l | tr -d " "`

if test "${out1oks}${out2oks}${out1fails}${out2fails}" != "${EXPECT2}"; then
	echo "$TMPFILE1 oks ${out1oks} fails ${out1fails} $TMPFILE2 oks ${out2oks} fails ${out2fails}"
	echo "expected ${EXPECT2}"
	echo "PKCS1-PAD2 FAIL"
	exit 1
fi

rm -f $TMPFILE1 $TMPFILE2

echo "PKCS1-PAD2 OK"

# Test 3, forged Starfield certificate,
# by Andrei Pyshkin, Erik Tews and Ralf-Philipp Weinmann.


datefudge "2006-09-23" "${CERTTOOL}" --verify-chain --infile "${srcdir}/data/pkcs1-pad-broken3.pem" | tee $TMPFILE1 >/dev/null 2>&1

out1oks=`grep 'Verified.' $TMPFILE1 | wc -l | tr -d " "`
out1fails=`grep 'Not verified.' $TMPFILE1 | wc -l | tr -d " "`

if test ${out1fails} -lt 2 || test ${out1oks} != 0;then
	echo "$TMPFILE1 oks ${out1oks} fails ${out1fails}"
	echo "expected ${EXPECT3}"
	echo "PKCS1-PAD3 FAIL"
	exit 1
fi

rm -f $TMPFILE1

echo "PKCS1-PAD3 OK"

# We're done.
exit 0
