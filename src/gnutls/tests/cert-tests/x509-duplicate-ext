#!/bin/sh

# Copyright (C) 2019 Nikos Mavrogiannopoulos
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
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>

srcdir="${srcdir:-.}"
CERTTOOL="${CERTTOOL:-../../src/certtool${EXEEXT}}"
OUTFILE=out.$$.tmp

if ! test -x "${CERTTOOL}"; then
	exit 77
fi

"${CERTTOOL}" --certificate-info --infile "${srcdir}/data/dup-exts.pem" >${OUTFILE} 2>&1
RET=$?
if test ${RET} =  0; then
	echo "Successfully loaded a certificate with duplicate extensions"
	cat ${OUTFILE}
	exit 1
fi

grep "Duplicate extension in" ${OUTFILE} 2>/dev/null
if test $? !=  0; then
	echo "Could not find the expected error value"
	cat ${OUTFILE}
	exit 1
fi


rm -f ${OUTFILE}

exit 0
