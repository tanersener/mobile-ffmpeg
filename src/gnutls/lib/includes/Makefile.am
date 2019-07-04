## Process this file with automake to produce Makefile.in
# Copyright (C) 2000-2012 Free Software Foundation, Inc.
#
# Author: Nikos Mavrogiannopoulos
#
# This file is part of GnuTLS.
#
# The GnuTLS is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public License
# as published by the Free Software Foundation; either version 3 of
# the License, or (at your option) any later version.
#
# The GnuTLS is distributed in the hope that it will be
# useful, but WITHOUT ANY WARRANTY; without even the implied warranty
# of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>

nobase_include_HEADERS = gnutls/x509.h gnutls/pkcs12.h gnutls/compat.h	\
	gnutls/openpgp.h gnutls/crypto.h gnutls/pkcs11.h		\
	gnutls/abstract.h gnutls/dtls.h gnutls/ocsp.h gnutls/tpm.h	\
	gnutls/x509-ext.h gnutls/self-test.h gnutls/system-keys.h	\
	gnutls/urls.h gnutls/pkcs7.h gnutls/socket.h

if ENABLE_CXX
nobase_include_HEADERS += gnutls/gnutlsxx.h
endif

nobase_nodist_include_HEADERS = gnutls/gnutls.h
