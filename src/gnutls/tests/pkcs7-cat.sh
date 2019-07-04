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

srcdir="${srcdir:-.}"
CERTTOOL="${CERTTOOL:-../src/certtool${EXEEXT}}"
DIFF="${DIFF:-diff -b -B}"
if ! test -z "${VALGRIND}"; then
	VALGRIND="${LIBTOOL:-libtool} --mode=execute ${VALGRIND} --error-exitcode=15"
fi
OUTFILE=out-pkcs7.$$.tmp

if ! test -x "${CERTTOOL}"; then
	exit 77
fi

. ${srcdir}/scripts/common.sh

check_for_datefudge

#try verification
datefudge -s "2010-10-10" \
${VALGRIND} "${CERTTOOL}" --verify-allow-broken --inder --p7-verify --infile "${srcdir}/data/test1.cat" --load-certificate "${srcdir}/data/pkcs7-cat-ca.pem"
rc=$?

if test "${rc}" = "0"; then
	echo "PKCS7 verification succeeded with invalid date"
	exit 1
fi

datefudge -s "2016-10-10" \
${VALGRIND} "${CERTTOOL}" --verify-allow-broken --inder --p7-verify --infile "${srcdir}/data/test1.cat" --load-certificate "${srcdir}/data/pkcs7-cat-ca.pem"
rc=$?

if test "${rc}" != "0"; then
	echo "PKCS7 verification failed"
	exit ${rc}
fi

# try parsing
for FILE in test1.cat test2.cat; do
${VALGRIND} "${CERTTOOL}" --inder --p7-info --infile "${srcdir}/data/${FILE}" > "${OUTFILE}"
rc=$?

# We're done.
if test "${rc}" != "0"; then
	echo "${FILE}: PKCS7 decoding failed"
	exit ${rc}
fi

${DIFF} "${OUTFILE}" "${srcdir}/data/${FILE}.out" #>/dev/null
if test "$?" != "0"; then
	echo "${FILE}: PKCS7 decoding didn't produce the correct file"
	exit 1
fi
done

rm -f "${OUTFILE}"
#test output files

for FILE in test1.cat test2.cat; do
${VALGRIND} "${CERTTOOL}" --inder --p7-info --p7-show-data --infile "${srcdir}/data/${FILE}" --outfile "${OUTFILE}"
rc=$?

# We're done.
if test "${rc}" != "0"; then
	echo "${FILE}: PKCS7 decoding failed"
	exit ${rc}
fi

${DIFF} "${OUTFILE}" "${srcdir}/data/${FILE}.data" >/dev/null
if test "$?" != "0"; then
	echo "${FILE}: PKCS7 decoding data didn't produce the correct file"
	exit 1
fi
done

rm -f "${OUTFILE}"

exit 0
