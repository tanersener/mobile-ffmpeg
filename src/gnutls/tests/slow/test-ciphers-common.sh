# Copyright (C) 2014 Red Hat, Inc.
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

# All optimizations disabled
GNUTLS_CPUID_OVERRIDE=0x1 ${PROG}
ret=$?
if test $ret != 0; then
	echo "included cipher tests failed"
	exit $ret
fi

exit_if_non_x86

GNUTLS_CPUID_OVERRIDE=0x2 ${PROG}
ret=$?
if test $ret != 0; then
	echo "AESNI cipher tests failed"
	exit $ret
fi

GNUTLS_CPUID_OVERRIDE=0x4 ${PROG}
ret=$?
if test $ret != 0; then
	echo "SSSE3 cipher tests failed"
	exit $ret
fi

#AESNI+PCLMUL
GNUTLS_CPUID_OVERRIDE=0xA ${PROG}
ret=$?
if test $ret != 0; then
	echo "PCLMUL cipher tests failed"
	exit $ret
fi

#AESNI+PCLMUL+AVX
GNUTLS_CPUID_OVERRIDE=0x1A ${PROG}
ret=$?
if test $ret != 0; then
	echo "PCLMUL-AVX cipher tests failed"
	exit $ret
fi

#SHANI
$(which lscpu)|grep Flags|grep sha_ni >/dev/null
if test $? = 0;then
	GNUTLS_CPUID_OVERRIDE=0x20 ${PROG}
	ret=$?
	if test $ret != 0; then
		echo "SHANI cipher tests failed"
		exit $ret
	fi
fi

GNUTLS_CPUID_OVERRIDE=0x100000 ${PROG}
ret=$?
if test $ret != 0; then
	echo "padlock cipher tests failed"
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
