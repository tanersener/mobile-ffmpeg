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
TMPFILE=pkcs8-invalid.$$.tmp

if ! test -x "${CERTTOOL}"; then
	exit 77
fi

if ! test -z "${VALGRIND}"; then
	VALGRIND="${LIBTOOL:-libtool} --mode=execute ${VALGRIND}"
fi

ret=0
for p8 in "pkcs8-invalid1.der 1234" "pkcs8-invalid2.der 1234" "pkcs8-invalid3.der 1234" "pkcs8-invalid4.der 1234" \
	"pkcs8-invalid5.der 1234" "pkcs8-invalid6.der 1234" "pkcs8-invalid7.der 1234" "pkcs8-invalid8.der password" \
	"pkcs8-invalid9.der password" "pkcs8-invalid10.der password";do
	set -- ${p8}
	file="$1"
	passwd="$2"
	${VALGRIND} "${CERTTOOL}" --inder --key-info --pkcs8 --password "${passwd}" \
		--infile "${srcdir}/data/${file}"
	rc=$?
	if test ${rc} != 1; then
		echo "PKCS8 FATAL ${p8} - errno ${rc}"
		exit 1
	else
		echo "PKCS8 OK ${p8} - errno ${rc}"
	fi
done

rm -f $TMPFILE

echo "PKCS8 DONE (rc $ret)"
exit $ret
