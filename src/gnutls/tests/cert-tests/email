#!/bin/sh

# Copyright (C) 2006-2008, 2010, 2012 Free Software Foundation, Inc.
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
DIFF=$"{DIFF:-diff}"

if ! test -x "${CERTTOOL}"; then
	exit 77
fi

if ! test -z "${VALGRIND}"; then
	VALGRIND="${LIBTOOL:-libtool} --mode=execute ${VALGRIND}"
fi

${VALGRIND} "${CERTTOOL}" -e --infile "${srcdir}/email-certs/chain.exclude.test.example.com" --verify-email test@example.com
rc=$?

if test "${rc}" != "1"; then
	echo "email test 1 failed"
	exit 1
fi

${VALGRIND} "${CERTTOOL}" -e --infile "${srcdir}/email-certs/chain.exclude.test.example.com" --verify-email invalid@example.com
rc=$?

if test "${rc}" != "1"; then
	echo "email test 2 failed"
	exit 1
fi

${VALGRIND} "${CERTTOOL}" -e --infile "${srcdir}/email-certs/chain.test.example.com" --verify-email test@example.com
rc=$?

if test "${rc}" != "0"; then
	echo "email test 3 failed"
	exit 1
fi

${VALGRIND} "${CERTTOOL}" -e --infile "${srcdir}/email-certs/chain.test.example.com" --verify-email invalid@example.com
rc=$?

if test "${rc}" != "1"; then
	echo "email test 4 failed"
	exit 1
fi

${VALGRIND} "${CERTTOOL}" -e --infile "${srcdir}/email-certs/chain.invalid.example.com" --verify-email invalid@example.com
rc=$?

if test "${rc}" != "1"; then
	echo "email test 5 failed"
	exit 1
fi

${VALGRIND} "${CERTTOOL}" -e --infile "${srcdir}/email-certs/chain.invalid.example.com" --verify-email test@cola.com
rc=$?

if test "${rc}" != "1"; then
	echo "email test 6 failed"
	exit 1
fi

${VALGRIND} "${CERTTOOL}" -e --infile "${srcdir}/email-certs/chain.test.example.com-2" --verify-email test@example.com
rc=$?

if test "${rc}" != "0"; then
	echo "email test 7 failed"
	exit 1
fi

${VALGRIND} "${CERTTOOL}" -e --infile "${srcdir}/email-certs/chain.test.example.com-2" --verify-email invalid@example.com
rc=$?

if test "${rc}" != "1"; then
	echo "email test 8 failed"
	exit 1
fi


exit 0
