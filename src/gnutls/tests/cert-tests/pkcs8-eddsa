#!/bin/sh

# Copyright (C) 2004-2006, 2010, 2012 Free Software Foundation, Inc.
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

srcdir="${srcdir:-.}"
CERTTOOL="${CERTTOOL:-../../src/certtool${EXEEXT}}"
DIFF="${DIFF:-diff -b -B}"
TMPFILE=pkcs8-eddsa.$$.tmp

if ! test -x "${CERTTOOL}"; then
	exit 77
fi

if test "${GNUTLS_FORCE_FIPS_MODE}" = 1;then
	echo "Cannot run in FIPS140-2 mode"
	exit 77
fi

if ! test -z "${VALGRIND}"; then
	VALGRIND="${LIBTOOL:-libtool} --mode=execute ${VALGRIND}"
fi

for p8 in "pkcs8-eddsa.pem"; do
	set -- ${p8}
	file="$1"
	${VALGRIND} "${CERTTOOL}" --key-info --pkcs8 --password "" \
		--infile "${srcdir}/data/${file}" --outfile $TMPFILE
	rc=$?
	if test ${rc} != 0; then
		echo "PKCS8 FATAL ${p8}"
		exit 1
	fi

	echo ""
	${DIFF} -u "${srcdir}/data/${p8}.txt" $TMPFILE
	rc=$?
	if test ${rc} != 0; then
		cat $TMPFILE
		echo "PKCS8 FATAL TXT ${p8}"
		exit 1
	fi
done
rm -f $TMPFILE

echo "PKCS8 DONE"
exit 0
