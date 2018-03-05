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

if test "${WINDIR}" != ""; then
	exit 77
fi

./dtls-stress -full -shello 01234 -sfinished 01 -cfinished 01234 CCertificate CKeyExchange CCertificateVerify CChangeCipherSpec CFinished
./dtls-stress -full -r -shello 42130 -sfinished 10 -cfinished 43210 SHello SKeyExchange SHelloDone CKeyExchange CChangeCipherSpec CFinished SChangeCipherSpec SCertificate SFinished

./dtls-stress -shello 021 -sfinished 01 -cfinished 012 SKeyExchange CKeyExchange CFinished
./dtls-stress -shello 012 -sfinished 10 -cfinished 210 SHello SKeyExchange SHelloDone
./dtls-stress -shello 012 -sfinished 01 -cfinished 021 SHello SKeyExchange SHelloDone
./dtls-stress -shello 021 -sfinished 01 -cfinished 201 SHello SHelloDone CChangeCipherSpec SChangeCipherSpec SFinished
./dtls-stress -shello 102 -sfinished 01 -cfinished 120 SHello SHelloDone CKeyExchange CFinished SChangeCipherSpec SFinished
./dtls-stress -shello 210 -sfinished 01 -cfinished 201 CChangeCipherSpec SChangeCipherSpec SFinished
./dtls-stress -shello 021 -sfinished 10 -cfinished 210 SHello SHelloDone SChangeCipherSpec CChangeCipherSpec CFinished
./dtls-stress -shello 210 -sfinished 10 -cfinished 210 SHello SKeyExchange SHelloDone CKeyExchange CChangeCipherSpec CFinished SChangeCipherSpec SFinished

./dtls-stress -full -shello 42130 -sfinished 10 -cfinished 43210 SHello SKeyExchange SHelloDone CKeyExchange CChangeCipherSpec CFinished SChangeCipherSpec SCertificate SFinished
./dtls-stress -full -shello 12430 -sfinished 01 -cfinished 01324 SHello SKeyExchange SHelloDone CKeyExchange CChangeCipherSpec CFinished SCertificate SFinished

exit 0
