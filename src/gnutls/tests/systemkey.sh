#!/bin/sh

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
# You should have received a copy of the GNU General Public License
# along with GnuTLS; if not, write to the Free Software Foundation,
# Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

srcdir="${srcdir:-.}"
SYSTEMKEY="${SYSTEMKEY:-../src/systemkey${EXEEXT}}"
unset RETCODE

. "${srcdir}/scripts/common.sh"

if ! test -x $SYSTEMKEY;then
	exit 77
fi

# Basic check for system key support. This is a superficial
# check ensuring that at least the listing works.

${SYSTEMKEY} --list
rc=$?
if test $rc != 0 && test $rc != 1;then
	echo "There was an issue listing system keys"
	exit 1
fi

exit 0
