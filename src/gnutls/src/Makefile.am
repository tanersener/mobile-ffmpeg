## Process this file with automake to produce Makefile.in
# Copyright (C) 2000-2012 Free Software Foundation, Inc.
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

# src/gl is being added by the top level makefile
SUBDIRS = 

BUILT_SOURCES = srptool-args.c srptool-args.h \
	psktool-args.c psktool-args.h ocsptool-args.h ocsptool-args.c \
	serv-args.c serv-args.h cli-args.c cli-args.h \
	cli-debug-args.c cli-debug-args.h certtool-args.c certtool-args.h \
	danetool-args.c danetool-args.h p11tool-args.c p11tool-args.h \
	tpmtool-args.c tpmtool-args.h systemkey-args.c systemkey-args.h

EXTRA_DIST = args-std.def

AM_CFLAGS = $(WARN_CFLAGS) $(WERROR_CFLAGS)
AM_CPPFLAGS = \
	-I$(srcdir)/gl				\
	-I$(builddir)/gl			\
	-I$(builddir)/../lib/includes		\
	-I$(srcdir)/../lib/includes		\
	-I$(srcdir)/../libdane/includes		\
	-I$(srcdir)/../extra/includes	\
	$(LIBOPTS_CFLAGS)

if NEED_LIBOPTS
LIBOPTS = libopts/libopts.la
SUBDIRS += libopts
AM_CPPFLAGS += -I$(srcdir)/libopts
else
LIBOPTS = $(LIBOPTS_LDADD)
endif

bin_PROGRAMS = psktool gnutls-cli-debug certtool
if ENABLE_SRP
bin_PROGRAMS += srptool
endif

if ENABLE_OCSP
bin_PROGRAMS += ocsptool gnutls-serv 
if ENABLE_ANON
bin_PROGRAMS += gnutls-cli
endif
endif

if ENABLE_DANE
bin_PROGRAMS += danetool
endif

noinst_PROGRAMS = systemkey

if ENABLE_PKCS11

# TPM makes use of PKCS #11 callbacks
if ENABLE_TROUSERS
bin_PROGRAMS += tpmtool
endif

bin_PROGRAMS += p11tool
endif

noinst_LTLIBRARIES =


if ENABLE_SRP
srptool_SOURCES = srptool.c
srptool_LDADD = ../lib/libgnutls.la libcmd-srp.la $(LIBOPTS) ../gl/libgnu.la 
srptool_LDADD += $(LTLIBINTL) gl/libgnu_gpl.la
noinst_LTLIBRARIES += libcmd-srp.la
libcmd_srp_la_SOURCES = srptool-args.def srptool-args.c srptool-args.h
endif

psktool_SOURCES = psk.c
psktool_LDADD = ../lib/libgnutls.la libcmd-psk.la $(LIBOPTS) ../gl/libgnu.la 
psktool_LDADD += $(LTLIBINTL) gl/libgnu_gpl.la
noinst_LTLIBRARIES += libcmd-psk.la
libcmd_psk_la_SOURCES = psktool-args.def psktool-args.c psktool-args.h


if ENABLE_OCSP

ocsptool_SOURCES = ocsptool.c ocsptool-common.h ocsptool-common.c \
	socket.c common.c common.h socket.h
ocsptool_LDADD = ../lib/libgnutls.la libcmd-ocsp.la $(LIBOPTS) ../gl/libgnu.la 
ocsptool_LDADD += $(LTLIBINTL) $(GETADDRINFO_LIB) gl/libgnu_gpl.la $(LIBIDN_LIBS)
noinst_LTLIBRARIES += libcmd-ocsp.la
libcmd_ocsp_la_SOURCES = ocsptool-args.def ocsptool-args.h ocsptool-args.c

gnutls_serv_SOURCES =		\
  list.h serv.c			\
  udp-serv.c udp-serv.h		\
  common.h common.c		\
  certtool-common.h 
gnutls_serv_LDADD = ../lib/libgnutls.la
gnutls_serv_LDADD += libcmd-serv.la $(LIBOPTS) ../gl/libgnu.la
gnutls_serv_LDADD += $(LTLIBINTL) gl/libgnu_gpl.la
gnutls_serv_LDADD += $(LIBSOCKET) $(GETADDRINFO_LIB) $(LIBIDN_LIBS)
noinst_LTLIBRARIES += libcmd-serv.la
libcmd_serv_la_SOURCES = serv-args.def serv-args.c serv-args.h

if ENABLE_ANON

BENCHMARK_SRCS = benchmark-cipher.c benchmark.c benchmark.h benchmark-tls.c

gnutls_cli_SOURCES = cli.c common.h common.c \
	socket.c socket.h ocsptool-common.c inline_cmds.h \
	$(BENCHMARK_SRCS)
gnutls_cli_LDADD = ../lib/libgnutls.la
if ENABLE_DANE
gnutls_cli_LDADD += ../libdane/libgnutls-dane.la
endif
gnutls_cli_LDADD += libcmd-cli.la $(LIBOPTS) ../gl/libgnu.la $(LTLIBINTL) $(LIBIDN_LIBS)
gnutls_cli_LDADD += $(LIBSOCKET) $(GETADDRINFO_LIB) $(LIB_CLOCK_GETTIME) \
	 $(SERVENT_LIB) gl/libgnu_gpl.la
noinst_LTLIBRARIES += libcmd-cli.la
libcmd_cli_la_SOURCES = cli-args.def cli-args.c cli-args.h
endif
endif

gnutls_cli_debug_SOURCES = cli-debug.c tests.h tests.c \
		socket.c socket.h common.h common.c
gnutls_cli_debug_LDADD = ../lib/libgnutls.la libcmd-cli-debug.la 
gnutls_cli_debug_LDADD += $(LIBOPTS) ../gl/libgnu.la gl/libgnu_gpl.la
gnutls_cli_debug_LDADD += $(LTLIBINTL) $(LIBIDN_LIBS)
gnutls_cli_debug_LDADD += $(LIBSOCKET) $(GETADDRINFO_LIB)
noinst_LTLIBRARIES += libcmd-cli-debug.la
libcmd_cli_debug_la_SOURCES = cli-debug-args.def cli-debug-args.c cli-debug-args.h

#certtool
COMMON_LIBS = $(LIBOPTS) $(LTLIBINTL)
if ENABLE_MINITASN1
COMMON_LIBS += ../lib/minitasn1/libminitasn1.la ../gl/libgnu.la 
AM_CPPFLAGS += -I$(top_srcdir)/lib/minitasn1
else
COMMON_LIBS += $(LIBTASN1_LIBS)
endif


certtool_SOURCES = certtool.c certtool-common.c certtool-extras.c common.c
certtool_LDADD = ../lib/libgnutls.la 
certtool_LDADD += libcmd-certtool.la ../gl/libgnu.la gl/libgnu_gpl.la

noinst_LTLIBRARIES += libcmd-certtool.la
libcmd_certtool_la_SOURCES = certtool-args.c certtool-args.def certtool-args.h	\
	certtool-cfg.h certtool-cfg.c
libcmd_certtool_la_LIBADD = ../lib/libgnutls.la gl/libgnu_gpl.la ../gl/libgnu.la 
libcmd_certtool_la_LIBADD += $(COMMON_LIBS)
libcmd_certtool_la_LIBADD += $(LTLIBREADLINE) gl/libgnu_gpl.la
libcmd_certtool_la_LIBADD += $(INET_PTON_LIB) $(LIB_CLOCK_GETTIME)


danetool_SOURCES = danetool.c certtool-common.c certtool-extras.c common.c socket.c
danetool_LDADD = ../lib/libgnutls.la  $(LIBIDN_LIBS)
danetool_LDADD += libcmd-danetool.la ../gl/libgnu.la gl/libgnu_gpl.la
if ENABLE_DANE
danetool_LDADD += ../libdane/libgnutls-dane.la
endif

noinst_LTLIBRARIES += libcmd-danetool.la
libcmd_danetool_la_SOURCES = danetool-args.c danetool-args.def danetool-args.h \
	certtool-cfg.h certtool-cfg.c
libcmd_danetool_la_LIBADD = ../lib/libgnutls.la gl/libgnu_gpl.la ../gl/libgnu.la 
libcmd_danetool_la_LIBADD += $(COMMON_LIBS)
libcmd_danetool_la_LIBADD += $(LTLIBREADLINE)
libcmd_danetool_la_LIBADD += $(INET_PTON_LIB) $(LIB_CLOCK_GETTIME)

# p11 tool
if ENABLE_PKCS11

p11tool_SOURCES = p11tool-args.def p11tool.c pkcs11.c certtool-common.c \
	certtool-extras.c p11tool.h common.c
p11tool_LDADD = ../lib/libgnutls.la
p11tool_LDADD += libcmd-p11tool.la ../gl/libgnu.la gl/libgnu_gpl.la
p11tool_LDADD += $(COMMON_LIBS)

noinst_LTLIBRARIES += libcmd-p11tool.la
libcmd_p11tool_la_SOURCES = p11tool-args.def p11tool-args.c p11tool-args.h \
	certtool-cfg.h certtool-cfg.c
libcmd_p11tool_la_LIBADD = ../lib/libgnutls.la gl/libgnu_gpl.la ../gl/libgnu.la 
libcmd_p11tool_la_LIBADD += $(LTLIBREADLINE) $(INET_PTON_LIB) $(LIB_CLOCK_GETTIME)

endif # ENABLE_PKCS11

if ENABLE_TROUSERS

tpmtool_SOURCES = tpmtool-args.def tpmtool.c certtool-common.c certtool-extras.c common.c
tpmtool_LDADD = ../lib/libgnutls.la
tpmtool_LDADD += libcmd-tpmtool.la ../gl/libgnu.la gl/libgnu_gpl.la
tpmtool_LDADD += $(COMMON_LIBS)

noinst_LTLIBRARIES += libcmd-tpmtool.la
libcmd_tpmtool_la_SOURCES = tpmtool-args.def tpmtool-args.c tpmtool-args.h \
	certtool-cfg.h certtool-cfg.c
libcmd_tpmtool_la_LIBADD = ../lib/libgnutls.la gl/libgnu_gpl.la ../gl/libgnu.la 
libcmd_tpmtool_la_LIBADD += $(LTLIBREADLINE) $(INET_PTON_LIB) $(LIB_CLOCK_GETTIME)

endif # ENABLE_TROUSERS

systemkey_SOURCES = systemkey.c certtool-common.c common.c
systemkey_LDADD = ../lib/libgnutls.la
systemkey_LDADD += libcmd-systemkey.la ../gl/libgnu.la gl/libgnu_gpl.la
systemkey_LDADD += $(COMMON_LIBS)

noinst_LTLIBRARIES += libcmd-systemkey.la
libcmd_systemkey_la_SOURCES = systemkey-args.def systemkey-args.c systemkey-args.h \
	certtool-cfg.h certtool-cfg.c
libcmd_systemkey_la_LIBADD = ../lib/libgnutls.la gl/libgnu_gpl.la ../gl/libgnu.la 
libcmd_systemkey_la_LIBADD += $(LTLIBREADLINE) $(INET_PTON_LIB) $(LIB_CLOCK_GETTIME)

danetool-args.h: danetool-args.stamp
danetool-args.c: danetool-args.stamp
danetool-args.stamp: $(srcdir)/danetool-args.def $(srcdir)/args-std.def
	-$(AUTOGEN) $<
	touch $@

ocsptool-args.h: ocsptool-args.stamp
ocsptool-args.c: ocsptool-args.stamp
ocsptool-args.stamp: $(srcdir)/ocsptool-args.def $(srcdir)/args-std.def
	-$(AUTOGEN) $<
	touch $@

tpmtool-args.h: tpmtool-args.stamp
tpmtool-args.c: tpmtool-args.stamp
tpmtool-args.stamp: $(srcdir)/tpmtool-args.def $(srcdir)/args-std.def
	-$(AUTOGEN) $<
	touch $@

p11tool-args.h: p11tool-args.stamp
p11tool-args.c: p11tool-args.stamp
p11tool-args.stamp: $(srcdir)/p11tool-args.def $(srcdir)/args-std.def
	-$(AUTOGEN) $<
	touch $@

psktool-args.h: psktool-args.stamp
psktool-args.c: psktool-args.stamp
psktool-args.stamp: $(srcdir)/psktool-args.def $(srcdir)/args-std.def
	-$(AUTOGEN) $<
	touch $@

cli-debug-args.h: cli-debug-args.stamp
cli-debug-args.c: cli-debug-args.stamp
cli-debug-args.stamp: $(srcdir)/cli-debug-args.def $(srcdir)/args-std.def
	-$(AUTOGEN) $<
	touch $@

cli-args.h: cli-args.stamp
cli-args.c: cli-args.stamp
cli-args.stamp: $(srcdir)/cli-args.def $(srcdir)/args-std.def
	-$(AUTOGEN) $<
	touch $@

serv-args.h: serv-args.stamp
serv-args.c: serv-args.stamp
serv-args.stamp: $(srcdir)/serv-args.def $(srcdir)/args-std.def
	-$(AUTOGEN) $<
	touch $@

srptool-args.h: srptool-args.stamp
srptool-args.c: srptool-args.stamp
srptool-args.stamp: $(srcdir)/srptool-args.def $(srcdir)/args-std.def
	-$(AUTOGEN) $<
	touch $@

certtool-args.h: certtool-args.stamp
certtool-args.c: certtool-args.stamp
certtool-args.stamp: $(srcdir)/certtool-args.def $(srcdir)/args-std.def
	-$(AUTOGEN) $<
	touch $@

systemkey-args.h: systemkey-args.stamp
systemkey-args.c: systemkey-args.stamp
systemkey-args.stamp: $(srcdir)/systemkey-args.def $(srcdir)/args-std.def
	-$(AUTOGEN) $<
	touch $@

clean-local:
	rm -f *.stamp
