#!/bin/sh

# Copyright (C) 2016 Red Hat, Inc.
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

#set -e

# This checks whether OIDs > 2^32 are correctly decoded.

srcdir="${srcdir:-.}"
CERTTOOL="${CERTTOOL:-../../src/certtool${EXEEXT}}"
DIFF="${DIFF:-diff -b -B}"
OUTFILE="long-oids.$$.pem.tmp"
TMPFILE1="long-oids1.$$.pem.tmp"

if ! test -x "${CERTTOOL}"; then
	exit 77
fi

if ! test -z "${VALGRIND}"; then
	VALGRIND="${LIBTOOL:-libtool} --mode=execute ${VALGRIND} --error-exitcode=3"
fi

${VALGRIND} "${CERTTOOL}" -i --infile "${srcdir}/data/long-oids.pem"|grep -v "Not After:"|grep -v ^warning >$OUTFILE
rc=$?

if test "${rc}" != 0;then
	echo "Could not read cert with long OIDs"
	exit 1
fi

cat "${srcdir}/data/long-oids.pem" |grep -v "Not After:"|grep -v ^warning >${TMPFILE1}
$DIFF ${TMPFILE1} ${OUTFILE}
if test $? != 0;then
	echo "Error in parsing cert with long OIDs"
	exit 1
fi

rm -f "$OUTFILE" "${TMPFILE1}" "${TMPFILE2}"

exit 0
