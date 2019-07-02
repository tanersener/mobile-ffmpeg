#!/bin/sh

# Copyright (C) 2004-2006, 2008, 2010, 2012 Free Software Foundation,
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

srcdir="${srcdir:-.}"
CERTTOOL="${CERTTOOL:-../../src/certtool${EXEEXT}}"

if ! test -x "${CERTTOOL}"; then
	exit 77
fi

if test "${GNUTLS_FORCE_FIPS_MODE}" = 1;then
	echo "Cannot run in FIPS140-2 mode"
	exit 77
fi

if ! test -z "${VALGRIND}"; then
#	VALGRIND=$(echo ${VALGRIND}|cut -d ' ' -f 1)
	VALGRIND="${LIBTOOL:-libtool} --mode=execute ${VALGRIND} --error-exitcode=6"
fi

. "${srcdir}/../scripts/common.sh"

TMPFILE="pkcs12-corner.$$.tmp"

# Cases from oss-fuzz

cpassword='1234'
for p12 in "mem-leak.p12";do
	set -- ${p12}
	file="$1"
	${VALGRIND} "${CERTTOOL}" --p12-info --inder --password "${cpassword}" \
		--infile "${srcdir}/data/${file}" >${TMPFILE} 2>&1
	rc=$?
	if test ${rc} != 0 && test ${rc} != 1; then
		cat ${TMPFILE}
		echo "PKCS12 FATAL ${file}"
		exit 1
	fi
done

# Check corner cases in PKCS#12 decoding. Typically the structures tested fail
# in parsing, but we check against crashes, etc. These test cases were taken
# from Hubert Kario's corpus at: https://github.com/redhat-qe-security/keyfile-corpus

cpassword='Red Hat Enterprise Linux 7.4'
for p12 in "key-corpus-rc2-1.p12" "key-corpus-rc2-2.p12" "key-corpus-rc2-3.p12";do 
	set -- ${p12}
	file="$1"
	${VALGRIND} "${CERTTOOL}" --p12-info --inder --password "${cpassword}" \
		--infile "${srcdir}/data/${file}" >${TMPFILE} 2>&1
	rc=$?
	if test ${rc} != 0 && test ${rc} != 1; then
		cat ${TMPFILE}
		echo "PKCS12 FATAL ${file}"
		exit 1
	fi
done

for p12 in "key-corpus-rc2-1.p12";do
	set -- ${p12}
	file="$1"
	"${CERTTOOL}" --p12-info --inder --password "${cpassword}" \
		--infile "${srcdir}/data/${file}" | tr -d '\r' >${TMPFILE} 2>/dev/null
	rc=$?
	if test ${rc} != 0 && test ${rc} != 1; then
		cat ${TMPFILE}
		echo "Error in output from ${file}"
		exit 1
	fi

	check_if_equal ${TMPFILE} "${srcdir}/data/${file}.out"
	rc=$?
	if test ${rc} != 0;then
		echo "Output differs in ${file}.out ${TMPFILE}"
		exit 1
	fi
done

rm -f ${TMPFILE}

exit 0
