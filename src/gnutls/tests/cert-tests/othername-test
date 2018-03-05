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
OUTFILE=tmp-othername.pem

if ! test -x "${CERTTOOL}"; then
	exit 77
fi

export TZ="UTC"

. ${srcdir}/../scripts/common.sh

check_for_datefudge

# Note that in rare cases this test may fail because the
# time set using datefudge could have changed since the generation
# (if example the system was busy)

datefudge -s "2007-04-22" \
	"${CERTTOOL}" --generate-self-signed \
		--load-privkey "${srcdir}/data/template-test.key" \
		--template "${srcdir}/templates/template-othername.tmpl" \
		--outfile ${OUTFILE} 2>/dev/null

${DIFF} "${srcdir}/data/template-othername.pem" ${OUTFILE} >/dev/null 2>&1
rc=$?

# We're done.
if test "${rc}" != "0"; then
	echo "Test 1 (othername) failed"
	exit ${rc}
fi

datefudge -s "2007-04-22" \
	"${CERTTOOL}" --generate-self-signed \
		--load-privkey "${srcdir}/data/template-test.key" \
		--template "${srcdir}/templates/template-othername-xmpp.tmpl" \
		--outfile ${OUTFILE} 2>/dev/null

${DIFF} "${srcdir}/data/template-othername-xmpp.pem" ${OUTFILE} >/dev/null 2>&1
rc=$?

# We're done.
if test "${rc}" != "0"; then
	echo "Test 1 (xmpp) failed"
	exit ${rc}
fi



rm -f ${OUTFILE}

exit 0
