#!/bin/sh

# Copyright (C) 2006-2012 Free Software Foundation, Inc.
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
OUTFILE="exts.$$.tmp"

if ! test -x "${CERTTOOL}"; then
	exit 77
fi

export TZ="UTC"

. ${srcdir}/../scripts/common.sh

check_for_datefudge

datefudge -s "2007-04-22" \
	"${CERTTOOL}" --generate-self-signed \
		--load-privkey "${srcdir}/data/template-test.key" \
		--template "${srcdir}/templates/arb-extensions.tmpl" \
		--outfile $OUTFILE #2>/dev/null

${DIFF} "${srcdir}/data/arb-extensions.pem" $OUTFILE #>/dev/null 2>&1
rc=$?

# We're done.
if test "${rc}" != "0"; then
	echo "Test with crt failed"
	exit ${rc}
fi

rm -f "$OUTFILE"

# Test adding critical extensions only
datefudge -s "2007-04-22" \
	"${CERTTOOL}" --generate-self-signed \
		--load-privkey "${srcdir}/data/template-test.key" \
		--template "${srcdir}/templates/crit-extensions.tmpl" \
		--outfile $OUTFILE #2>/dev/null

${DIFF} "${srcdir}/data/crit-extensions.pem" $OUTFILE #>/dev/null 2>&1
rc=$?

# We're done.
if test "${rc}" != "0"; then
	echo "Test with critical only failed"
	exit ${rc}
fi

rm -f "$OUTFILE"

datefudge -s "2007-04-22" \
	"${CERTTOOL}" --generate-request \
		--load-privkey "${srcdir}/data/template-test.key" \
		--template "${srcdir}/templates/arb-extensions.tmpl" \
		2>/dev/null | grep -v "Algorithm Security Level" >$OUTFILE

${DIFF} "${srcdir}/data/arb-extensions.csr" $OUTFILE #>/dev/null 2>&1
rc=$?

# We're done.
if test "${rc}" != "0"; then
	echo "Test with crq failed"
	exit ${rc}
fi

rm -f "$OUTFILE"

exit 0
