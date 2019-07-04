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

srcdir="${srcdir:-.}"
CERTTOOL="${CERTTOOL:-../../src/certtool${EXEEXT}}"
DIFF="${DIFF:-diff}"
TMPFILE=tmp-$$.privkey.tmp

if ! test -x "${CERTTOOL}"; then
	exit 77
fi

if ! test -z "${VALGRIND}"; then
	VALGRIND="${LIBTOOL:-libtool} --mode=execute ${VALGRIND}"
fi

for i in privkey1.pem privkey2.pem privkey3.pem;do
#check whether "funny" spaces can be interpreted
${VALGRIND} "${CERTTOOL}" -k --infile "${srcdir}/data/${i}"
rc=$?

if test "${rc}" != "0";then
	echo "Error importing private key ${i}"
	exit 1
fi
done

${VALGRIND} "${CERTTOOL}" -k --infile "${srcdir}/data/privkey1.pem" --no-text --outfile ${TMPFILE}
rc=$?

if test "${rc}" != "0"; then
	echo "--no-text privkey info failed 1"
	exit ${rc}
fi

if grep -v '^-----BEGIN [A-Z0-9 ]\+-----$' ${TMPFILE} | grep -v '^[A-Za-z0-9/+=]\+$' | grep -v '^-----END [A-Z0-9 ]\+-----$' ; then
	echo "--no-text privkey info failed 2"
	exit 1
fi

rm -f ${TMPFILE}

exit 0
