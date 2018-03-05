#!/bin/sh

# Copyright (C) 2016 Nikos Mavrogiannopoulos
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

PROG=./hash-large${EXEEXT}
unset RETCODE
if ! test -z "${VALGRIND}"; then
	VALGRIND="${LIBTOOL:-libtool} --mode=execute ${VALGRIND}"
fi

srcdir="${srcdir:-.}"
. "${srcdir}/../scripts/common.sh"

${PROG}
ret=$?
if test $ret != 0; then
	echo "default cipher tests failed"
	exit $ret
fi

GNUTLS_CPUID_OVERRIDE=0x1 ${PROG}
ret=$?
if test $ret != 0; then
	echo "included cipher tests failed"
	exit $ret
fi

exit_if_non_x86

GNUTLS_CPUID_OVERRIDE=0x4 ${PROG}
ret=$?
if test $ret != 0; then
	echo "SSSE3 cipher tests failed"
	exit $ret
fi

GNUTLS_CPUID_OVERRIDE=0x200000 ${PROG}
ret=$?
if test $ret != 0; then
	echo "padlock PHE cipher tests failed"
	exit $ret
fi

GNUTLS_CPUID_OVERRIDE=0x400000 ${PROG}
ret=$?
if test $ret != 0; then
	echo "padlock PHE SHA512 cipher tests failed"
	exit $ret
fi

exit 0
