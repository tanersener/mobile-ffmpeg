#!/bin/sh

# Copyright (C) 2015 Red Hat, Inc.
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
DIFF="${DIFF:-diff}"

if ! test -x "${CERTTOOL}"; then
	exit 77
fi

if ! test -z "${VALGRIND}"; then
	VALGRIND="${LIBTOOL:-libtool} --mode=execute ${VALGRIND}"
fi

TMPFILE=constraints.$$.pem.tmp

. ${srcdir}/../scripts/common.sh

check_for_datefudge

datefudge -s "2016-04-22" \
	${VALGRIND} "${CERTTOOL}" --verify-allow-broken -e --infile "${srcdir}/data/name-constraints-ip.pem"
rc=$?

if test "${rc}" != "0"; then
	echo "name constraints test 1 failed"
	exit 1
fi

${VALGRIND} "${CERTTOOL}" -i --infile "${srcdir}/data/name-constraints-ip2.pem" --outfile "${TMPFILE}"
rc=$?

if test "${rc}" != "0"; then
	echo "name constraints test 2 failed"
	exit 1
fi

${DIFF} -I ^warning "${TMPFILE}" "${srcdir}/data/name-constraints-ip2.pem" >/dev/null 2>&1
rc=$?

if test "${rc}" != "0"; then
	echo "name constraints test 3 failed"
	exit 1
fi

rm -f "${TMPFILE}"

exit 0
