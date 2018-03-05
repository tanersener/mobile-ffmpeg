## Process this file with automake to produce Makefile.in
# Copyright (C) 2002-2012 Free Software Foundation, Inc.
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
# along with this program.  If not, see <http://www.gnu.org/licenses/>

include $(top_srcdir)/lib/common.mk

AM_CPPFLAGS = \
	-I$(srcdir)/../../gl		\
	-I$(builddir)/../../gl		\
	-I$(srcdir)/../includes		\
	-I$(builddir)/../includes	\
	-I$(builddir)/../../gl          \
	-I$(srcdir)/..			\
	-I$(srcdir)/../opencdk

if ENABLE_MINITASN1
AM_CPPFLAGS += -I$(srcdir)/../minitasn1
endif

noinst_LTLIBRARIES = libgnutls_ext.la

libgnutls_ext_la_SOURCES = max_record.c cert_type.c \
	server_name.c signature.c safe_renegotiation.c \
	max_record.h cert_type.h server_name.h srp.h \
	session_ticket.h signature.h safe_renegotiation.h \
	session_ticket.c srp.c ecc.c ecc.h heartbeat.c heartbeat.h \
	status_request.h status_request.c dumbfw.c dumbfw.h \
	ext_master_secret.c ext_master_secret.h etm.h etm.c

if ENABLE_ALPN
libgnutls_ext_la_SOURCES += alpn.c alpn.h
endif

if ENABLE_DTLS_SRTP
libgnutls_ext_la_SOURCES += srtp.c srtp.h
endif
