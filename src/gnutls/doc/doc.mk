# Copyright (C) 2012 Free Software Foundation, Inc.
#
# Author: Nikos Mavrogiannopoulos
#
# This file is part of GnuTLS.
#
# This file is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This file is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this file; if not, write to the Free Software Foundation,
# Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

HEADER_FILES = $(top_srcdir)/lib/includes/gnutls/gnutls.h.in \
	$(top_srcdir)/lib/includes/gnutls/x509.h \
	$(top_srcdir)/lib/includes/gnutls/pkcs12.h $(top_srcdir)/lib/includes/gnutls/pkcs11.h \
	$(top_srcdir)/lib/includes/gnutls/abstract.h $(top_srcdir)/lib/includes/gnutls/compat.h \
	$(top_srcdir)/lib/includes/gnutls/dtls.h $(top_srcdir)/lib/includes/gnutls/crypto.h \
	$(top_srcdir)/lib/includes/gnutls/ocsp.h $(top_srcdir)/lib/includes/gnutls/tpm.h \
	$(top_srcdir)/libdane/includes/gnutls/dane.h $(top_srcdir)/lib/includes/gnutls/x509-ext.h \
	$(top_srcdir)/lib/includes/gnutls/urls.h $(top_srcdir)/lib/includes/gnutls/system-keys.h \
	$(top_srcdir)/lib/includes/gnutls/pkcs7.h $(top_srcdir)/lib/includes/gnutls/socket.h

C_SOURCE_FILES = $(top_srcdir)/lib/*/*.c $(top_srcdir)/lib/*.c $(top_srcdir)/libdane/*.c
C_X509_SOURCE_FILES = $(top_srcdir)/lib/x509/*.c $(top_srcdir)/lib/*.c $(top_srcdir)/lib/system/cert.c
