#!/bin/sh

# Copyright (C) 2015 Red Hat, Inc.
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

# This checks whether invalid UTF8 strings trigger valgrind warnings.

srcdir="${srcdir:-.}"
CERTTOOL="${CERTTOOL:-../../src/certtool${EXEEXT}}"
DIFF="${DIFF:-diff -b -B}"
outfile="out.$$.pem"

if ! test -x "${CERTTOOL}"; then
	exit 77
fi

if ! test -z "${VALGRIND}"; then
	VALGRIND="${LIBTOOL:-libtool} --mode=execute ${VALGRIND} --error-exitcode=3"
fi

${VALGRIND} "${CERTTOOL}" -i --infile "${srcdir}/data/very-long-dn.pem" >$outfile
rc=$?

if test "${rc}" = 3;then
	echo "Invalid memory access with cert and long CN"
	exit 1
fi

if test "${rc}" != 0;then
	echo "Could not read cert long CN"
	exit 1
fi

$DIFF $outfile "${srcdir}/data/very-long-dn.pem"
if test $? != 0;then
	echo "Error in parsing cert with long CN"
	exit 1
fi

rm -f "$outfile"

exit 0
