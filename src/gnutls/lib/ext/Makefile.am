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
# along with this program.  If not, see <https://www.gnu.org/licenses/>

include $(top_srcdir)/lib/common.mk

AM_CPPFLAGS = \
	-I$(srcdir)/../../gl		\
	-I$(builddir)/../../gl		\
	-I$(srcdir)/../includes		\
	-I$(builddir)/../includes	\
	-I$(builddir)/../../gl          \
	-I$(srcdir)/..

if ENABLE_MINITASN1
AM_CPPFLAGS += -I$(srcdir)/../minitasn1
endif

noinst_LTLIBRARIES = libgnutls_ext.la

libgnutls_ext_la_SOURCES = max_record.c \
	server_name.c signature.c safe_renegotiation.c \
	max_record.h server_name.h srp.h \
	session_ticket.h signature.h safe_renegotiation.h \
	session_ticket.c srp.c heartbeat.c heartbeat.h \
	status_request.h status_request.c dumbfw.c dumbfw.h \
	ext_master_secret.c ext_master_secret.h etm.h etm.c \
	supported_versions.c supported_versions.h \
	post_handshake.c post_handshake.h key_share.c key_share.h \
	cookie.c cookie.h \
	psk_ke_modes.c psk_ke_modes.h pre_shared_key.c pre_shared_key.h \
	supported_groups.c supported_groups.h \
	ec_point_formats.c ec_point_formats.h \
	early_data.c early_data.h \
	record_size_limit.c record_size_limit.h \
	client_cert_type.c client_cert_type.h \
	server_cert_type.c server_cert_type.h \
	cert_types.h

if ENABLE_ALPN
libgnutls_ext_la_SOURCES += alpn.c alpn.h
endif

if ENABLE_DTLS_SRTP
libgnutls_ext_la_SOURCES += srtp.c srtp.h
endif
