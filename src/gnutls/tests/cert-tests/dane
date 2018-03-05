#!/bin/sh

# Copyright (C) 2012 Free Software Foundation, Inc.
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

set -e

srcdir="${srcdir:-.}"
DANETOOL="${DANETOOL:-../../src/danetool${EXEEXT}}"
DIFF="${DIFF:-diff}"

test -e "${DANETOOL}" || exit 77

"${DANETOOL}" --tlsa-rr --load-certificate "${srcdir}/data/cert-ecc256.pem" --host www.example.com --outfile tmp-dane.rr 2>/dev/null

${DIFF} "${srcdir}/data/dane-test.rr" tmp-dane.rr
rc=$?

rm -f tmp-dane.rr

# We're done.
if test "${rc}" != "0"; then
	exit ${rc}
fi

exit 0
