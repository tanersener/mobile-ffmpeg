#!/bin/sh

# Copyright (C) 2009-2012 Free Software Foundation, Inc.
#
# Author: Simon Josefsson
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

CERTTOOL="${CERTTOOL:-../src/certtool${EXEEXT}}"

if ! test -x "${CERTTOOL}"; then
	exit 77
fi

if ! test -z "${VALGRIND}"; then
	VALGRIND="${LIBTOOL:-libtool} --mode=execute ${VALGRIND} --error-exitcode=1"
fi

if cat<<EOF \
	| ${VALGRIND} "${CERTTOOL}" --certificate-info \
	| grep 'Issuer: OU=Plus \\+ Comma \\,,O=RFC 2253 escape test' > /dev/null
-----BEGIN CERTIFICATE-----
MIICETCCAXygAwIBAgIESnlIMTALBgkqhkiG9w0BAQUwODEdMBsGA1UEChMUUkZD
IDIyNTMgZXNjYXBlIHRlc3QxFzAVBgNVBAsTDlBsdXMgKyBDb21tYSAsMB4XDTA5
MDgwNTA4NTIwMVoXDTM2MTIyMTA4NTIwNFowODEdMBsGA1UEChMUUkZDIDIyNTMg
ZXNjYXBlIHRlc3QxFzAVBgNVBAsTDlBsdXMgKyBDb21tYSAsMIGcMAsGCSqGSIb3
DQEBAQOBjAAwgYgCgYC7ZkP18sXXtozMxd/1iDuxyUtqDqGtIFBACIChT1yj0Phs
z+Y89+wEdhMXi2SJIlvA3VN8O+18BLuAuSi+jpvGjqClEsv1Vx6i57u3M0mf47tK
rmpNaP/JEeIyjc49gAuNde/YAIGPKAQDoCKNYQQH+rY3fSEHSdIJYWmYkKNYqQID
AQABoy8wLTAMBgNVHRMBAf8EAjAAMB0GA1UdDgQWBBRMuQqb+h00437ey9IHFf6h
2stokTALBgkqhkiG9w0BAQUDgYEAmvr55otCWJx8ReDt5jFKd8aDk3jm6RSogV/P
+fBYR69w25NxgSWVsQeoSi2Jklpqa20koynCya087TM8ODl3lO0XbmG1YGksnM6R
RMCUzqiqC2be1s2N+Bml4cIWTHzPZfnF/qXfbbkouepfbdscprXu07Z317kdAG8+
iptEYYo=
-----END CERTIFICATE-----
EOF
then
	:
else
	echo "RFC 2253 escaping not working?"
	exit 1
fi

exit 0
