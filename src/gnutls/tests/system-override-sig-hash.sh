#!/bin/sh

# Copyright (C) 2019 Nikos Mavrogiannopoulos
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
TMPFILE=c.$$.tmp
export GNUTLS_SYSTEM_PRIORITY_FAIL_ON_INVALID=1

cat <<_EOF_ > ${TMPFILE}
[overrides]

insecure-hash = sha256
insecure-hash = sha512
_EOF_

export GNUTLS_SYSTEM_PRIORITY_FILE="${TMPFILE}"

${builddir}/system-override-hash

cat <<_EOF_ > ${TMPFILE}
[overrides]

insecure-sig-for-cert = rsa-sha256
insecure-sig = rsa-sha512
insecure-sig = rsa-sha1
_EOF_

export GNUTLS_SYSTEM_PRIORITY_FILE="${TMPFILE}"

${builddir}/system-override-sig
if test $? != 0;then
	echo "Could not parse config file"
	exit 1
fi

exit 0
