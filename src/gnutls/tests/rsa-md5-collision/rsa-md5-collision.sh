#!/bin/sh

# Copyright (C) 2006, 2008, 2010, 2012 Free Software Foundation, Inc.
# Copyright (C) 2016, Red Hat, Inc.
#
# Author: Simon Josefsson, Nikos Mavrogiannopoulos
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

srcdir="${srcdir:-.}"
CERTTOOL="${CERTTOOL:-../src/certtool${EXEEXT}}"
TMPFILE1=rsa-md5.$$.tmp
TMPFILE2=rsa-md5-2.$$.tmp

if ! test -x "${CERTTOOL}"; then
	exit 77
fi

. ${srcdir}/scripts/common.sh
check_for_datefudge

# Disable leak detection
ASAN_OPTIONS="detect_leaks=0"
export ASAN_OPTIONS

datefudge -s "2006-10-1" \
"${CERTTOOL}" --verify-chain --outfile "$TMPFILE1" --infile "${srcdir}/rsa-md5-collision/colliding-chain-md5-1.pem"
if test $? = 0;then
	echo "Verification on chain1 succeeded"
	exit 1
fi

grep 'Not verified.' $TMPFILE1| grep 'insecure algorithm'
if test $? != 0;then
	echo "Output on chain1 doesn't match the expected"
	exit 1
fi


datefudge -s "2006-10-1" \
"${CERTTOOL}" --verify-chain --outfile "$TMPFILE2" --infile "${srcdir}/rsa-md5-collision/colliding-chain-md5-2.pem"
if test $? = 0;then
	echo "Verification on chain2 succeeded"
	exit 1
fi

grep 'Not verified.' $TMPFILE2| grep 'insecure algorithm'
if test $? != 0;then
	echo "Output on chain2 doesn't match the expected"
	exit 1
fi

rm -f $TMPFILE1 $TMPFILE2

# We're done.
exit 0
