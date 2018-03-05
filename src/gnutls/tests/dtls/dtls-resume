#!/bin/sh

# Copyright (C) 2016 Red Hat, Inc.
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

if test "${WINDIR}" != ""; then
	exit 77
fi

./dtls-stress -resume -sfinished 012 -cfinished 01 -d
./dtls-stress -resume -sfinished 210 -cfinished 01 -d
./dtls-stress -resume -sfinished 120 -cfinished 01 -d
./dtls-stress -resume -sfinished 210 -cfinished 10 -d
./dtls-stress -resume -sfinished 120 -cfinished 10 -d

./dtls-stress -resume -sfinished 012 -cfinished 01 -d SHello
./dtls-stress -resume -sfinished 012 -cfinished 01 -d CChangeCipherSpec
./dtls-stress -resume -sfinished 012 -cfinished 01 -d SChangeCipherSpec
./dtls-stress -resume -sfinished 012 -cfinished 01 -d SHello SChangeCipherSpec
./dtls-stress -resume -sfinished 012 -cfinished 01 -d CChangeCipherSpec SChangeCipherSpec CFinished
./dtls-stress -resume -sfinished 012 -cfinished 01 -d CChangeCipherSpec SChangeCipherSpec SFinished
./dtls-stress -resume -sfinished 012 -cfinished 01 -d CFinished SFinished
./dtls-stress -resume -sfinished 012 -cfinished 01 -d CFinished SFinished SChangeCipherSpec
./dtls-stress -resume -sfinished 012 -cfinished 01 -d CFinished SFinished CChangeCipherSpec

exit 0
