#!/bin/sh

# Copyright (C) 2016 Nikos Mavrogiannopoulos
# Copyright (C) 2017 Red Hat, Inc.
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
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>
#

PROG=./hash-large${EXEEXT}
unset RETCODE
if ! test -z "${VALGRIND}"; then
	VALGRIND="${LIBTOOL:-libtool} --mode=execute ${VALGRIND}"
fi

srcdir="${srcdir:-.}"
. "${srcdir}/../scripts/common.sh"

run_test() {
	GNUTLS_CPUID_OVERRIDE=$1 ${PROG}
	ret=$?
	if test $ret != 0; then
		echo "tests failed for flags $1"
		exit $ret
	fi
}

#0x20: SHA_NI
#0x4: SSSE3
#0x1: no optimizations
#"": default optimizations

SSSE3FLAG=""
SHANIFLAG=""
which lscpu >/dev/null 2>&1
if test $? = 0;then
        $(which lscpu)|grep Architecture|grep x86 >/dev/null
        if test $? = 0;then
                SSSE3FLAG="0x4"
        fi

        $(which lscpu)|grep Flags|grep sha_ni >/dev/null
        if test $? = 0;then
                SHANIFLAG="0x20"
        fi
fi

WAITPID=""
for flags in "" "0x1" ${SSSE3FLAG} ${SHANIFLAG};do
	run_test ${flags} &
	WAITPID="${WAITPID} $!"
done

for i in "$WAITPID";do
	wait $i
	ret=$?
	test ${ret} != 0 && exit ${ret}
done

exit_if_non_padlock

#0x200000: Padlock PHE
#0x400000: Padlock PHE SHA512

WAITPID=""
for flags in "0x200000" "0x400000";do
	run_test ${flags} &
	WAITPID="${WAITPID} $!"
done

for i in "$WAITPID";do
	wait $i
	ret=$?
	test ${ret} != 0 && exit ${ret}
done

exit 0
