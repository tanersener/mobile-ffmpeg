#!/bin/sh

# Copyright (C) 2004-2006, 2010, 2012 Free Software Foundation, Inc.
# Copyright (C) 2016 Red Hat, Inc.
#
# Author: Nikos Mavrogiannopoulos
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
TMPFILE=key-invalid.$$.tmp

if ! test -x "${CERTTOOL}"; then
	exit 77
fi

if ! test -z "${VALGRIND}"; then
	VALGRIND="${LIBTOOL:-libtool} --mode=execute ${VALGRIND}"
fi

ret=0
for p8 in ${srcdir}/data/key-invalid*.der;do
	set -- ${p8}
	file="$1"
	${VALGRIND} "${CERTTOOL}" --inder --key-info \
		--infile "${file}"
	rc=$?
	if test ${rc} != 1; then
		echo "FATAL ${p8} - errno ${rc}"
		ret=1
	else
		echo "OK ${p8} - errno ${rc}"
	fi
done

rm -f $TMPFILE

echo "DONE (rc $ret)"
exit $ret
