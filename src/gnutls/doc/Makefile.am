## Process this file with automake to produce Makefile.in
# Copyright (C) 2000-2012 Free Software Foundation, Inc.
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

EXTRA_DIST = TODO certtool.cfg gnutls.pdf gnutls.html	\
	doxygen/Doxyfile.in doxygen/Doxyfile.orig texinfo.css \
	gnutls-guile.pdf gnutls-guile.html stamp_enums stamp_functions \
	doc.mk COPYING COPYING.LESSER

IMAGES = \
	gnutls-handshake-state.png	\
	gnutls-layers.png gnutls-modauth.png \
	gnutls-client-server-use-case.png			\
	gnutls-handshake-sequence.png gnutls-internals.png	\
	gnutls-logo.png gnutls-x509.png	\
	pkcs11-vision.png

SUBDIRS = examples cyclo scripts credentials latex
if ENABLE_GTK_DOC
SUBDIRS += reference
endif

-include $(top_srcdir)/doc/doc.mk

invoke-gnutls-cli.texi: $(top_srcdir)/src/cli-args.def
	PATH="$(top_builddir)/src/$(PATH_SEPARATOR)$${PATH}$(PATH_SEPARATOR)" $(AUTOGEN) -L$(top_srcdir)/src -Tagtexi-cmd.tpl $<; \
	if [ ! -e $@ ]; then \
		cp $(srcdir)/$@ .; \
	fi; \
	$(srcdir)/scripts/cleanup-autogen.pl < $@ > $@.tmp && \
	mv -f $@.tmp $@

invoke-gnutls-cli-debug.texi: $(top_srcdir)/src/cli-debug-args.def invoke-gnutls-cli.texi
	PATH="$(top_builddir)/src/$(PATH_SEPARATOR)$${PATH}$(PATH_SEPARATOR)" $(AUTOGEN) -Tagtexi-cmd.tpl $<; \
	if [ ! -e $@ ]; then \
		cp $(srcdir)/$@ .; \
	fi; \
	$(srcdir)/scripts/cleanup-autogen.pl < $@ > $@.tmp && \
	mv -f $@.tmp $@

invoke-gnutls-serv.texi: $(top_srcdir)/src/serv-args.def invoke-gnutls-cli-debug.texi
	PATH="$(top_builddir)/src/$(PATH_SEPARATOR)$${PATH}$(PATH_SEPARATOR)" $(AUTOGEN) -Tagtexi-cmd.tpl $<; \
	if [ ! -e $@ ]; then \
		cp $(srcdir)/$@ .; \
	fi; \
	$(srcdir)/scripts/cleanup-autogen.pl < $@ > $@.tmp && \
	mv -f $@.tmp $@

invoke-certtool.texi: $(top_srcdir)/src/certtool-args.def invoke-gnutls-serv.texi
	PATH="$(top_builddir)/src/$(PATH_SEPARATOR)$${PATH}$(PATH_SEPARATOR)" $(AUTOGEN) -Tagtexi-cmd.tpl $<; \
	if [ ! -e $@ ]; then \
		cp $(srcdir)/$@ .; \
	fi; \
	$(srcdir)/scripts/cleanup-autogen.pl < $@ > $@.tmp && \
	rm -f $@ && \
	$(SED) -e 's/@subheading/@subsubheading/g' \
	    -e 's/@section/@subsection/g' $@.tmp > $@ && \
	rm -f $@.tmp

invoke-ocsptool.texi: $(top_srcdir)/src/ocsptool-args.def invoke-certtool.texi
	PATH="$(top_builddir)/src/$(PATH_SEPARATOR)$${PATH}$(PATH_SEPARATOR)" $(AUTOGEN) -Tagtexi-cmd.tpl $<; \
	if [ ! -e $@ ]; then \
		cp $(srcdir)/$@ .; \
	fi; \
	$(srcdir)/scripts/cleanup-autogen.pl < $@ > $@.tmp && \
	rm -f $@ && \
	$(SED) -e 's/@subheading/@subsubheading/g' \
	    -e 's/@section/@subsection/g' $@.tmp > $@ && \
	rm -f $@.tmp

invoke-danetool.texi: $(top_srcdir)/src/danetool-args.def invoke-ocsptool.texi
	PATH="$(top_builddir)/src/$(PATH_SEPARATOR)$${PATH}$(PATH_SEPARATOR)" $(AUTOGEN) -Tagtexi-cmd.tpl $<; \
	if [ ! -e $@ ]; then \
		cp $(srcdir)/$@ .; \
	fi; \
	$(srcdir)/scripts/cleanup-autogen.pl < $@ > $@.tmp && \
	rm -f $@ && \
	$(SED) -e 's/@subheading/@subsubheading/g' \
	    -e 's/@section/@subsection/g' $@.tmp > $@ && \
	rm -f $@.tmp

invoke-srptool.texi: $(top_srcdir)/src/srptool-args.def invoke-danetool.texi
	PATH="$(top_builddir)/src/$(PATH_SEPARATOR)$${PATH}$(PATH_SEPARATOR)" $(AUTOGEN) -Tagtexi-cmd.tpl $<; \
	if [ ! -e $@ ]; then \
		cp $(srcdir)/$@ .; \
	fi; \
	$(srcdir)/scripts/cleanup-autogen.pl < $@ > $@.tmp && \
	rm -f $@ && \
	$(SED) -e 's/@subheading/@subsubheading/g' \
	    -e 's/@section/@subsubsection/g' $@.tmp > $@ && \
	rm -f $@.tmp

invoke-psktool.texi: $(top_srcdir)/src/psktool-args.def invoke-srptool.texi
	PATH="$(top_builddir)/src/$(PATH_SEPARATOR)$${PATH}$(PATH_SEPARATOR)" $(AUTOGEN) -Tagtexi-cmd.tpl $<; \
	if [ ! -e $@ ]; then \
		cp $(srcdir)/$@ .; \
	fi; \
	$(srcdir)/scripts/cleanup-autogen.pl < $@ > $@.tmp && \
	rm -f $@ && \
	$(SED) -e 's/@subheading/@subsubheading/g' \
	    -e 's/@section/@subsubsection/g' $@.tmp > $@ && \
	rm -f $@.tmp

invoke-p11tool.texi: $(top_srcdir)/src/p11tool-args.def invoke-psktool.texi
	PATH="$(top_builddir)/src/$(PATH_SEPARATOR)$${PATH}$(PATH_SEPARATOR)" $(AUTOGEN) -Tagtexi-cmd.tpl $<; \
	if [ ! -e $@ ]; then \
		cp $(srcdir)/$@ .; \
	fi; \
	$(srcdir)/scripts/cleanup-autogen.pl < $@ > $@.tmp && \
	rm -f $@ && \
	$(SED) \
	    -e 's/@subsection/@subsubheading/g' \
	    -e 's/@section/@subsection/g' $@.tmp > $@ && \
	rm -f $@.tmp

invoke-tpmtool.texi: $(top_srcdir)/src/tpmtool-args.def invoke-p11tool.texi
	PATH="$(top_builddir)/src/$(PATH_SEPARATOR)$${PATH}$(PATH_SEPARATOR)" $(AUTOGEN) -Tagtexi-cmd.tpl $<; \
	if [ ! -e $@ ]; then \
		cp $(srcdir)/$@ .; \
	fi; \
	$(srcdir)/scripts/cleanup-autogen.pl < $@ > $@.tmp && \
	rm -f $@ && \
	$(SED) \
	    -e 's/@subsection/@subsubheading/g' \
	    -e 's/@section/@subsection/g' $@.tmp > $@ && \
	rm -f $@.tmp


info_TEXINFOS = gnutls.texi gnutls-guile.texi
gnutls_TEXINFOS = gnutls.texi fdl-1.3.texi				\
	cha-bib.texi cha-cert-auth.texi	cha-cert-auth2.texi		\
	cha-ciphersuites.texi cha-copying.texi cha-functions.texi	\
	cha-gtls-app.texi cha-internals.texi cha-intro-tls.texi		\
	cha-library.texi cha-preface.texi cha-programs.texi		\
	sec-tls-app.texi cha-errors.texi cha-support.texi		\
	cha-shared-key.texi cha-gtls-examples.texi cha-upgrade.texi	\
	cha-tokens.texi cha-crypto.texi cha-auth.texi

AUTOGENED_DOC = invoke-gnutls-cli.texi invoke-gnutls-cli-debug.texi \
	invoke-gnutls-serv.texi invoke-certtool.texi invoke-srptool.texi \
	invoke-ocsptool.texi invoke-psktool.texi invoke-p11tool.texi \
	invoke-tpmtool.texi invoke-danetool.texi

gnutls_TEXINFOS += stamp_functions

# Examples.
gnutls_TEXINFOS += examples/ex-client-anon.c				\
	examples/ex-session-info.c examples/ex-verify.c			\
	examples/ex-cert-select.c examples/ex-client-resume.c		\
	examples/ex-client-srp.c examples/ex-client-x509.c		\
	examples/ex-serv-x509.c examples/ex-serv-anon.c			\
	examples/ex-serv-srp.c			\
	examples/ex-alert.c examples/ex-x509-info.c examples/ex-crq.c	\
	examples/ex-pkcs12.c examples/ex-client-dtls.c

# Images.  Make sure there are eps + png + pdf of each, plus the source dia.
gnutls_TEXINFOS += gnutls-internals.dia gnutls-internals.eps		\
                   gnutls-internals.png
gnutls_TEXINFOS += gnutls-layers.dia gnutls-layers.eps			\
                   gnutls-layers.png
gnutls_TEXINFOS += gnutls-crypto-layers.dia gnutls-crypto-layers.eps	\
                   gnutls-crypto-layers.png
gnutls_TEXINFOS += gnutls-x509.dia gnutls-x509.eps gnutls-x509.png
gnutls_TEXINFOS += gnutls-logo.eps gnutls-logo.png
gnutls_TEXINFOS += pkcs11-vision.dia pkcs11-vision.eps pkcs11-vision.png

# Images.  Make sure there are eps + png + pdf of each, plus the source dia.
gnutls_TEXINFOS += gnutls-client-server-use-case.dia	\
	gnutls-client-server-use-case.eps		\
	gnutls-client-server-use-case.png
gnutls_TEXINFOS += gnutls-handshake-sequence.dia			\
	gnutls-handshake-sequence.eps gnutls-handshake-sequence.png
gnutls_TEXINFOS += gnutls-handshake-state.dia			\
	gnutls-handshake-state.eps gnutls-handshake-state.png
gnutls_TEXINFOS += gnutls-modauth.dia gnutls-modauth.eps	\
	gnutls-modauth.png

infoimagesdir = $(infodir)
infoimages_DATA = $(IMAGES)
html_DATA = $(IMAGES)

AM_MAKEINFOFLAGS = -I $(top_srcdir)/doc
TEXI2DVI = texi2dvi $(AM_MAKEINFOFLAGS)
AM_MAKEINFOHTMLFLAGS = $(AM_MAKEINFOFLAGS) \
	--no-split --css-include=$(srcdir)/texinfo.css

MAINTAINERCLEANFILES =

# Generated texinfos.

API_FILES=gnutls-api.texi socket-api.texi x509-api.texi pgp-api.texi pkcs12-api.texi \
	tpm-api.texi pkcs11-api.texi abstract-api.texi compat-api.texi \
	dtls-api.texi crypto-api.texi ocsp-api.texi tpm-api.texi dane-api.texi \
	pkcs7-api.texi

clean-local:
	-rm -f stamp_enums stamp_functions
	-rm -rf functions/ enums/
	-rm -f $(API_FILES)

gnutls_TEXINFOS += $(API_FILES)

MAINTAINERCLEANFILES += $(API_FILES)

gnutls-api.texi: $(top_srcdir)/lib/includes/gnutls/gnutls.h.in
	echo "" > $@-tmp
	for i in `$(top_srcdir)/doc/scripts/getfuncs.pl < $< |sort|uniq`; do \
		echo $(ECHO_N) "Creating documentation for $$i... " && \
		$(srcdir)/scripts/gdoc -texinfo \
			-function $$i \
			$(C_SOURCE_FILES) >> $@-tmp 2>/dev/null && \
		echo "ok"; \
	done
	mv -f $@-tmp $@

pgp-api.texi: $(top_srcdir)/lib/includes/gnutls/openpgp.h
	echo "" > $@-tmp
	for i in `$(top_srcdir)/doc/scripts/getfuncs.pl < $< |sort|uniq`; do \
		echo $(ECHO_N) "Creating documentation for $$i... " && \
		$(srcdir)/scripts/gdoc -texinfo \
			-function $$i \
			$(C_SOURCE_FILES) >> $@-tmp 2>/dev/null && \
		echo "ok"; \
	done
	mv -f $@-tmp $@

socket-api.texi: $(top_srcdir)/lib/includes/gnutls/socket.h
	echo "" > $@-tmp
	for i in `$(top_srcdir)/doc/scripts/getfuncs.pl < $< |sort|uniq`; do \
		echo $(ECHO_N) "Creating documentation for $$i... " && \
		$(srcdir)/scripts/gdoc -texinfo \
			-function $$i \
			$(C_SOURCE_FILES) >> $@-tmp 2>/dev/null && \
		echo "ok"; \
	done
	mv -f $@-tmp $@

dane-api.texi: $(top_srcdir)/libdane/includes/gnutls/dane.h
	echo "" > $@-tmp
	for i in `$(top_srcdir)/doc/scripts/getfuncs.pl < $< |sort|uniq`; do \
		echo $(ECHO_N) "Creating documentation for $$i... " && \
		$(srcdir)/scripts/gdoc -texinfo \
			-function $$i \
			$(C_SOURCE_FILES) >> $@-tmp 2>/dev/null && \
		echo "ok"; \
	done
	mv -f $@-tmp $@

x509-api.texi: $(top_srcdir)/lib/includes/gnutls/x509.h $(top_srcdir)/lib/includes/gnutls/x509-ext.h
	echo "" > $@-tmp
	cat $^ > $@-tmp2
	for i in `$(top_srcdir)/doc/scripts/getfuncs.pl < $@-tmp2 |sort |uniq`; do \
		echo $(ECHO_N) "Creating documentation for $$i... " && \
		$(srcdir)/scripts/gdoc -texinfo \
			-function $$i \
			$(C_SOURCE_FILES) >> $@-tmp 2>/dev/null && \
		echo "ok"; \
	done
	rm -f $@-tmp2
	mv -f $@-tmp $@

pkcs12-api.texi: $(top_srcdir)/lib/includes/gnutls/pkcs12.h
	echo "" > $@-tmp
	for i in `$(top_srcdir)/doc/scripts/getfuncs.pl < $< |sort|uniq`; do \
		echo $(ECHO_N) "Creating documentation for $$i... " && \
		$(srcdir)/scripts/gdoc -texinfo \
			-function $$i \
			$(C_X509_SOURCE_FILES) >> $@-tmp 2>/dev/null && \
		echo "ok"; \
	done
	mv -f $@-tmp $@

pkcs11-api.texi: $(top_srcdir)/lib/includes/gnutls/pkcs11.h
	echo "" > $@-tmp
	for i in `$(top_srcdir)/doc/scripts/getfuncs.pl < $< |sort|uniq`; do \
		echo $(ECHO_N) "Creating documentation for $$i... " && \
		$(srcdir)/scripts/gdoc -texinfo \
			-function $$i \
			$(C_SOURCE_FILES) >> $@-tmp 2>/dev/null && \
		echo "ok"; \
	done
	mv -f $@-tmp $@

pkcs7-api.texi: $(top_srcdir)/lib/includes/gnutls/pkcs7.h
	echo "" > $@-tmp
	for i in `$(top_srcdir)/doc/scripts/getfuncs.pl < $< |sort|uniq`; do \
		echo $(ECHO_N) "Creating documentation for $$i... " && \
		$(srcdir)/scripts/gdoc -texinfo \
			-function $$i \
			$(C_X509_SOURCE_FILES) >> $@-tmp 2>/dev/null && \
		echo "ok"; \
	done
	mv -f $@-tmp $@

tpm-api.texi: $(top_srcdir)/lib/includes/gnutls/tpm.h
	echo "" > $@-tmp
	for i in `$(top_srcdir)/doc/scripts/getfuncs.pl < $< |sort|uniq`; do \
		echo $(ECHO_N) "Creating documentation for $$i... " && \
		$(srcdir)/scripts/gdoc -texinfo \
			-function $$i \
			$(C_SOURCE_FILES) >> $@-tmp 2>/dev/null && \
		echo "ok"; \
	done
	mv -f $@-tmp $@

abstract-api.texi: $(top_srcdir)/lib/includes/gnutls/abstract.h $(top_srcdir)/lib/includes/gnutls/urls.h $(top_srcdir)/lib/includes/gnutls/system-keys.h
	echo "" > $@-tmp
	cat $^ >$@-headers-tmp
	for i in `$(top_srcdir)/doc/scripts/getfuncs.pl < $@-headers-tmp |sort|uniq`; do \
		echo $(ECHO_N) "Creating documentation for $$i... " && \
		$(srcdir)/scripts/gdoc -texinfo \
			-function $$i \
			$(C_SOURCE_FILES) >> $@-tmp 2>/dev/null && \
		echo "ok"; \
	done
	rm -f $@-headers-tmp
	mv -f $@-tmp $@

compat-api.texi: $(top_srcdir)/lib/includes/gnutls/compat.h
	echo "" > $@-tmp
	for i in `$(top_srcdir)/doc/scripts/getfuncs.pl < $< |sort|uniq`; do \
		echo $(ECHO_N) "Creating documentation for $$i... " && \
		$(srcdir)/scripts/gdoc -texinfo \
			-function $$i \
			$(C_SOURCE_FILES) >> $@-tmp 2>/dev/null && \
		echo "ok"; \
	done
	mv -f $@-tmp $@

dtls-api.texi: $(top_srcdir)/lib/includes/gnutls/dtls.h
	echo "" > $@-tmp
	for i in `$(top_srcdir)/doc/scripts/getfuncs.pl < $< |sort|uniq`; do \
		echo $(ECHO_N) "Creating documentation for $$i... " && \
		$(srcdir)/scripts/gdoc -texinfo \
			-function $$i \
			$(C_SOURCE_FILES) >> $@-tmp 2>/dev/null && \
		echo "ok"; \
	done
	mv -f $@-tmp $@

crypto-api.texi: $(top_srcdir)/lib/includes/gnutls/crypto.h
	echo "" > $@-tmp
	for i in `$(top_srcdir)/doc/scripts/getfuncs.pl < $< |sort|uniq`; do \
		echo $(ECHO_N) "Creating documentation for $$i... " && \
		$(srcdir)/scripts/gdoc -texinfo \
			-function $$i \
			$(C_SOURCE_FILES) >> $@-tmp 2>/dev/null && \
		echo "ok"; \
	done
	mv -f $@-tmp $@

ocsp-api.texi: $(top_srcdir)/lib/includes/gnutls/ocsp.h
	echo "" > $@-tmp
	for i in `$(top_srcdir)/doc/scripts/getfuncs.pl < $< |sort|uniq`; do \
		echo $(ECHO_N) "Creating documentation for $$i... " && \
		$(srcdir)/scripts/gdoc -texinfo \
			-function $$i \
			$(C_X509_SOURCE_FILES) >> $@-tmp 2>/dev/null && \
		echo "ok"; \
	done
	mv -f $@-tmp $@

# Generated texinfos.
# for some reason it does not work when cross compiling
if !WINDOWS
gnutls_TEXINFOS += error_codes.texi algorithms.texi alerts.texi enums.texi
endif

DISTCLEANFILES = error_codes.texi algorithms.texi alerts.texi enums.texi

AM_CPPFLAGS = \
	-I$(top_builddir)/lib/includes -I$(top_srcdir)/lib/includes \
	-I$(top_builddir)/

EXTRA_PROGRAMS = errcodes printlist alert-printlist

errcodes_SOURCES = errcodes.c common.c common.h
errcodes_LDADD = ../lib/libgnutls.la ../gl/libgnu.la

printlist_SOURCES = printlist.c common.c common.h
printlist_LDADD = ../lib/libgnutls.la ../gl/libgnu.la

alert_printlist_SOURCES = alert-printlist.c common.c common.h
alert_printlist_LDADD = ../lib/libgnutls.la ../gl/libgnu.la

error_codes.texi: $(top_srcdir)/lib/errors.c $(srcdir)/errcodes.c
	$(MAKE) $(builddir)/errcodes
	$(builddir)/errcodes > $@-tmp
	mv -f $@-tmp $@

algorithms.texi: $(top_srcdir)/lib/algorithms/ciphers.c $(srcdir)/printlist.c
	$(MAKE) $(builddir)/printlist
	$(builddir)/printlist > $@-tmp
	mv -f $@-tmp $@

alerts.texi: $(top_srcdir)/lib/alert.c $(srcdir)/alert-printlist.c
	$(MAKE) $(builddir)/alert-printlist
	$(builddir)/alert-printlist > $@-tmp
	mv -f $@-tmp $@

enums.texi: $(HEADER_FILES)
	echo "" > $@-tmp
	for i in $^; do \
		echo $(ECHO_N) "Creating documentation for $$i... " && \
		$(srcdir)/scripts/gdoc -texinfo $$i >> $@-tmp && \
		echo "ok"; \
	done
	mv -f $@-tmp $@

gnutls_TEXINFOS += $(ENUMS) $(FUNCS) $(AUTOGENED_DOC)
DISTCLEANFILES += $(ENUMS) stamp_enums stamp_functions


stamp_functions: $(API_FILES)
	-mkdir functions
	-rm -f functions/*.short
	for i in $^; do \
		$(srcdir)/scripts/split-texi.pl functions < $$i; \
	done
	$(SED) -i 's/\@anchor{.*//g' functions/*
	$(SED) -i 's/\@subheading.*//g' functions/*
	cd functions && for i in *;do grep ^"@deftypefun" $$i | $(SED) 's/@deftypefun/@item/g;s/ {/ @var{/;s/ {/ @ref{/' > ../functions/$$i.short;done
	echo $@ > $@

stamp_enums: enums.texi
	-mkdir enums
	$(srcdir)/scripts/split-texi.pl enums enum < $<
	echo $@ > $@

$(ENUMS): stamp_enums

$(FUNCS): stamp_functions

compare-exported:
	rm -f tmp-exp-$@ tmp-head-$@
	for i in ../libdane/includes/gnutls/*.h ../lib/includes/gnutls/*.h;do perl scripts/getfuncs.pl <$$i >>tmp-head-$@;done
	sort -u tmp-head-$@ > tmp2-head-$@
	mv tmp2-head-$@ tmp-head-$@
	scripts/getfuncs-map.pl <../lib/libgnutls.map >tmp-exp-$@
	scripts/getfuncs-map.pl <../libdane/libdane.map >>tmp-exp-$@
	sort -u tmp-exp-$@ > tmp2-exp-$@
	mv tmp2-exp-$@ tmp-exp-$@
	@echo "******************************************************************************"
	@echo "If the following step fails there is a symbol in headers that is not exported or vice-versa"
	@echo "******************************************************************************"
	diff -u tmp-exp-$@ tmp-head-$@
	rm -f tmp-exp-$@ tmp-head-$@

compare-makefile: enums.texi
	@echo "******************************************************************************"
	@echo "If the following step fails use 'make files-update'"
	@echo "******************************************************************************"
	ENUMS=`grep '^@c ' $< | $(SED) 's/@c //g' | sort -d`; \
	STR=""; \
	for i in $$ENUMS; do \
		STR="$$STR\nENUMS += enums/$$i"; \
	done; \
	grep -v -e '^ENUMS += ' $(srcdir)/Makefile.am | \
		perl -p -e "s,^ENUMS =,ENUMS =$$STR," > tmp-$@; \
	diff -u $(srcdir)/Makefile.am tmp-$@ >/dev/null
	rm -f tmp-$@
	FUNCS=`cat $(HEADER_FILES) | $(top_srcdir)/doc/scripts/getfuncs.pl|sort -d|uniq`; \
	MANS=""; \
	for i in $$FUNCS; do \
		MANS="$$MANS\nFUNCS += functions/$$i\nFUNCS += functions/$$i.short"; \
	done; \
	grep -v -e '^FUNCS += ' $(srcdir)/Makefile.am > tmp-$@; \
	echo "\"s,^FUNCS =,FUNCS =$$MANS,\" -i tmp-$@"|xargs $(SED)
	@echo "******************************************************************************"
	@echo "If the following step fails use 'make files-update'"
	@echo "******************************************************************************"
	diff -u $(srcdir)/Makefile.am tmp-$@ >/dev/null
	rm -f tmp-$@

.PHONY: compare-makefile compare-exported

# Guile texinfos.

guile_texi = core.c.texi
BUILT_SOURCES        = $(guile_texi)
MAINTAINERCLEANFILES += $(guile_texi)
EXTRA_DIST           += $(guile_texi) extract-guile-c-doc.scm
guile_TEXINFOS       = gnutls-guile.texi $(guile_texi)

if HAVE_GUILE

GUILE_FOR_BUILD =				\
  GUILE_AUTO_COMPILE=0				\
  $(GUILE) -L $(top_srcdir)/guile/modules

SNARF_CPPFLAGS = -I$(top_srcdir) -I$(top_builddir)			\
	 -I$(top_srcdir)/lib/includes -I$(top_builddir)/lib/includes	\
	 -I$(top_srcdir)/extra/includes				\
         -I$(top_srcdir)/guile/src -I$(top_builddir)/guile/src		\
	 $(GUILE_CFLAGS)

core.c.texi: $(top_srcdir)/guile/src/core.c
	$(MAKE) -C ../guile/src built-sources &&			\
	$(GUILE_FOR_BUILD) -l "$(srcdir)/extract-guile-c-doc.scm"	\
	   -e '(apply main (cdr (command-line)))'			\
	   -- "$^" "$(CPP)" "$(SNARF_CPPFLAGS) $(CPPFLAGS)"		\
	   > "$@"

else !HAVE_GUILE

core.c.texi:
	echo "(Guile not available, documentation not generated.)" > $@

endif !HAVE_GUILE

gnutls.xml: epub.texi
	makeinfo --docbook $<
	$(SED) -i 's/\&\#8226;//g' $@

gnutls.epub: gnutls.xml
	dbtoepub $<
	-epub-fix --delete-unmanifested gnutls.epub

ENUMS =
ENUMS += enums/dane_cert_type_t
ENUMS += enums/dane_cert_usage_t
ENUMS += enums/dane_match_type_t
ENUMS += enums/dane_query_status_t
ENUMS += enums/dane_state_flags_t
ENUMS += enums/dane_verify_flags_t
ENUMS += enums/dane_verify_status_t
ENUMS += enums/gnutls_alert_description_t
ENUMS += enums/gnutls_alert_level_t
ENUMS += enums/gnutls_alpn_flags_t
ENUMS += enums/gnutls_certificate_flags
ENUMS += enums/gnutls_certificate_import_flags
ENUMS += enums/gnutls_certificate_print_formats_t
ENUMS += enums/gnutls_certificate_request_t
ENUMS += enums/gnutls_certificate_status_t
ENUMS += enums/gnutls_certificate_type_t
ENUMS += enums/gnutls_certificate_verification_profiles_t
ENUMS += enums/gnutls_certificate_verify_flags
ENUMS += enums/gnutls_channel_binding_t
ENUMS += enums/gnutls_cipher_algorithm_t
ENUMS += enums/gnutls_close_request_t
ENUMS += enums/gnutls_compression_method_t
ENUMS += enums/gnutls_credentials_type_t
ENUMS += enums/gnutls_digest_algorithm_t
ENUMS += enums/gnutls_ecc_curve_t
ENUMS += enums/gnutls_ext_flags_t
ENUMS += enums/gnutls_ext_parse_type_t
ENUMS += enums/gnutls_handshake_description_t
ENUMS += enums/gnutls_init_flags_t
ENUMS += enums/gnutls_keygen_types_t
ENUMS += enums/gnutls_keyid_flags_t
ENUMS += enums/gnutls_kx_algorithm_t
ENUMS += enums/gnutls_mac_algorithm_t
ENUMS += enums/gnutls_ocsp_cert_status_t
ENUMS += enums/gnutls_ocsp_print_formats_t
ENUMS += enums/gnutls_ocsp_resp_status_t
ENUMS += enums/gnutls_ocsp_verify_reason_t
ENUMS += enums/gnutls_openpgp_crt_status_t
ENUMS += enums/gnutls_params_type_t
ENUMS += enums/gnutls_pin_flag_t
ENUMS += enums/gnutls_pk_algorithm_t
ENUMS += enums/gnutls_pkcs11_obj_flags
ENUMS += enums/gnutls_pkcs11_obj_info_t
ENUMS += enums/gnutls_pkcs11_obj_type_t
ENUMS += enums/gnutls_pkcs11_token_info_t
ENUMS += enums/gnutls_pkcs11_url_type_t
ENUMS += enums/gnutls_pkcs12_bag_type_t
ENUMS += enums/gnutls_pkcs7_sign_flags
ENUMS += enums/gnutls_pkcs_encrypt_flags_t
ENUMS += enums/gnutls_privkey_flags_t
ENUMS += enums/gnutls_privkey_type_t
ENUMS += enums/gnutls_protocol_t
ENUMS += enums/gnutls_psk_key_flags
ENUMS += enums/gnutls_pubkey_flags_t
ENUMS += enums/gnutls_rnd_level_t
ENUMS += enums/gnutls_sec_param_t
ENUMS += enums/gnutls_server_name_type_t
ENUMS += enums/gnutls_session_flags_t
ENUMS += enums/gnutls_sign_algorithm_t
ENUMS += enums/gnutls_srtp_profile_t
ENUMS += enums/gnutls_supplemental_data_format_type_t
ENUMS += enums/gnutls_tpmkey_fmt_t
ENUMS += enums/gnutls_vdata_types_t
ENUMS += enums/gnutls_x509_crl_reason_t
ENUMS += enums/gnutls_x509_crt_fmt_t
ENUMS += enums/gnutls_x509_subject_alt_name_t

FUNCS =
FUNCS += functions/dane_cert_type_name
FUNCS += functions/dane_cert_type_name.short
FUNCS += functions/dane_cert_usage_name
FUNCS += functions/dane_cert_usage_name.short
FUNCS += functions/dane_match_type_name
FUNCS += functions/dane_match_type_name.short
FUNCS += functions/dane_query_data
FUNCS += functions/dane_query_data.short
FUNCS += functions/dane_query_deinit
FUNCS += functions/dane_query_deinit.short
FUNCS += functions/dane_query_entries
FUNCS += functions/dane_query_entries.short
FUNCS += functions/dane_query_status
FUNCS += functions/dane_query_status.short
FUNCS += functions/dane_query_tlsa
FUNCS += functions/dane_query_tlsa.short
FUNCS += functions/dane_query_to_raw_tlsa
FUNCS += functions/dane_query_to_raw_tlsa.short
FUNCS += functions/dane_raw_tlsa
FUNCS += functions/dane_raw_tlsa.short
FUNCS += functions/dane_state_deinit
FUNCS += functions/dane_state_deinit.short
FUNCS += functions/dane_state_init
FUNCS += functions/dane_state_init.short
FUNCS += functions/dane_state_set_dlv_file
FUNCS += functions/dane_state_set_dlv_file.short
FUNCS += functions/dane_strerror
FUNCS += functions/dane_strerror.short
FUNCS += functions/dane_verification_status_print
FUNCS += functions/dane_verification_status_print.short
FUNCS += functions/dane_verify_crt
FUNCS += functions/dane_verify_crt.short
FUNCS += functions/dane_verify_crt_raw
FUNCS += functions/dane_verify_crt_raw.short
FUNCS += functions/dane_verify_session_crt
FUNCS += functions/dane_verify_session_crt.short
FUNCS += functions/gnutls_aead_cipher_decrypt
FUNCS += functions/gnutls_aead_cipher_decrypt.short
FUNCS += functions/gnutls_aead_cipher_deinit
FUNCS += functions/gnutls_aead_cipher_deinit.short
FUNCS += functions/gnutls_aead_cipher_encrypt
FUNCS += functions/gnutls_aead_cipher_encrypt.short
FUNCS += functions/gnutls_aead_cipher_init
FUNCS += functions/gnutls_aead_cipher_init.short
FUNCS += functions/gnutls_alert_get
FUNCS += functions/gnutls_alert_get.short
FUNCS += functions/gnutls_alert_get_name
FUNCS += functions/gnutls_alert_get_name.short
FUNCS += functions/gnutls_alert_get_strname
FUNCS += functions/gnutls_alert_get_strname.short
FUNCS += functions/gnutls_alert_send
FUNCS += functions/gnutls_alert_send.short
FUNCS += functions/gnutls_alert_send_appropriate
FUNCS += functions/gnutls_alert_send_appropriate.short
FUNCS += functions/gnutls_alpn_get_selected_protocol
FUNCS += functions/gnutls_alpn_get_selected_protocol.short
FUNCS += functions/gnutls_alpn_set_protocols
FUNCS += functions/gnutls_alpn_set_protocols.short
FUNCS += functions/gnutls_anon_allocate_client_credentials
FUNCS += functions/gnutls_anon_allocate_client_credentials.short
FUNCS += functions/gnutls_anon_allocate_server_credentials
FUNCS += functions/gnutls_anon_allocate_server_credentials.short
FUNCS += functions/gnutls_anon_free_client_credentials
FUNCS += functions/gnutls_anon_free_client_credentials.short
FUNCS += functions/gnutls_anon_free_server_credentials
FUNCS += functions/gnutls_anon_free_server_credentials.short
FUNCS += functions/gnutls_anon_set_params_function
FUNCS += functions/gnutls_anon_set_params_function.short
FUNCS += functions/gnutls_anon_set_server_dh_params
FUNCS += functions/gnutls_anon_set_server_dh_params.short
FUNCS += functions/gnutls_anon_set_server_known_dh_params
FUNCS += functions/gnutls_anon_set_server_known_dh_params.short
FUNCS += functions/gnutls_anon_set_server_params_function
FUNCS += functions/gnutls_anon_set_server_params_function.short
FUNCS += functions/gnutls_auth_client_get_type
FUNCS += functions/gnutls_auth_client_get_type.short
FUNCS += functions/gnutls_auth_get_type
FUNCS += functions/gnutls_auth_get_type.short
FUNCS += functions/gnutls_auth_server_get_type
FUNCS += functions/gnutls_auth_server_get_type.short
FUNCS += functions/gnutls_buffer_append_data
FUNCS += functions/gnutls_buffer_append_data.short
FUNCS += functions/gnutls_bye
FUNCS += functions/gnutls_bye.short
FUNCS += functions/gnutls_certificate_activation_time_peers
FUNCS += functions/gnutls_certificate_activation_time_peers.short
FUNCS += functions/gnutls_certificate_allocate_credentials
FUNCS += functions/gnutls_certificate_allocate_credentials.short
FUNCS += functions/gnutls_certificate_client_get_request_status
FUNCS += functions/gnutls_certificate_client_get_request_status.short
FUNCS += functions/gnutls_certificate_expiration_time_peers
FUNCS += functions/gnutls_certificate_expiration_time_peers.short
FUNCS += functions/gnutls_certificate_free_ca_names
FUNCS += functions/gnutls_certificate_free_ca_names.short
FUNCS += functions/gnutls_certificate_free_cas
FUNCS += functions/gnutls_certificate_free_cas.short
FUNCS += functions/gnutls_certificate_free_credentials
FUNCS += functions/gnutls_certificate_free_credentials.short
FUNCS += functions/gnutls_certificate_free_crls
FUNCS += functions/gnutls_certificate_free_crls.short
FUNCS += functions/gnutls_certificate_free_keys
FUNCS += functions/gnutls_certificate_free_keys.short
FUNCS += functions/gnutls_certificate_get_crt_raw
FUNCS += functions/gnutls_certificate_get_crt_raw.short
FUNCS += functions/gnutls_certificate_get_issuer
FUNCS += functions/gnutls_certificate_get_issuer.short
FUNCS += functions/gnutls_certificate_get_ours
FUNCS += functions/gnutls_certificate_get_ours.short
FUNCS += functions/gnutls_certificate_get_peers
FUNCS += functions/gnutls_certificate_get_peers.short
FUNCS += functions/gnutls_certificate_get_peers_subkey_id
FUNCS += functions/gnutls_certificate_get_peers_subkey_id.short
FUNCS += functions/gnutls_certificate_get_trust_list
FUNCS += functions/gnutls_certificate_get_trust_list.short
FUNCS += functions/gnutls_certificate_get_verify_flags
FUNCS += functions/gnutls_certificate_get_verify_flags.short
FUNCS += functions/gnutls_certificate_get_x509_crt
FUNCS += functions/gnutls_certificate_get_x509_crt.short
FUNCS += functions/gnutls_certificate_get_x509_key
FUNCS += functions/gnutls_certificate_get_x509_key.short
FUNCS += functions/gnutls_certificate_send_x509_rdn_sequence
FUNCS += functions/gnutls_certificate_send_x509_rdn_sequence.short
FUNCS += functions/gnutls_certificate_server_set_request
FUNCS += functions/gnutls_certificate_server_set_request.short
FUNCS += functions/gnutls_certificate_set_dh_params
FUNCS += functions/gnutls_certificate_set_dh_params.short
FUNCS += functions/gnutls_certificate_set_flags
FUNCS += functions/gnutls_certificate_set_flags.short
FUNCS += functions/gnutls_certificate_set_key
FUNCS += functions/gnutls_certificate_set_key.short
FUNCS += functions/gnutls_certificate_set_known_dh_params
FUNCS += functions/gnutls_certificate_set_known_dh_params.short
FUNCS += functions/gnutls_certificate_set_ocsp_status_request_file
FUNCS += functions/gnutls_certificate_set_ocsp_status_request_file.short
FUNCS += functions/gnutls_certificate_set_ocsp_status_request_function
FUNCS += functions/gnutls_certificate_set_ocsp_status_request_function.short
FUNCS += functions/gnutls_certificate_set_ocsp_status_request_function2
FUNCS += functions/gnutls_certificate_set_ocsp_status_request_function2.short
FUNCS += functions/gnutls_certificate_set_params_function
FUNCS += functions/gnutls_certificate_set_params_function.short
FUNCS += functions/gnutls_certificate_set_pin_function
FUNCS += functions/gnutls_certificate_set_pin_function.short
FUNCS += functions/gnutls_certificate_set_retrieve_function
FUNCS += functions/gnutls_certificate_set_retrieve_function.short
FUNCS += functions/gnutls_certificate_set_retrieve_function2
FUNCS += functions/gnutls_certificate_set_retrieve_function2.short
FUNCS += functions/gnutls_certificate_set_trust_list
FUNCS += functions/gnutls_certificate_set_trust_list.short
FUNCS += functions/gnutls_certificate_set_verify_flags
FUNCS += functions/gnutls_certificate_set_verify_flags.short
FUNCS += functions/gnutls_certificate_set_verify_function
FUNCS += functions/gnutls_certificate_set_verify_function.short
FUNCS += functions/gnutls_certificate_set_verify_limits
FUNCS += functions/gnutls_certificate_set_verify_limits.short
FUNCS += functions/gnutls_certificate_set_x509_crl
FUNCS += functions/gnutls_certificate_set_x509_crl.short
FUNCS += functions/gnutls_certificate_set_x509_crl_file
FUNCS += functions/gnutls_certificate_set_x509_crl_file.short
FUNCS += functions/gnutls_certificate_set_x509_crl_mem
FUNCS += functions/gnutls_certificate_set_x509_crl_mem.short
FUNCS += functions/gnutls_certificate_set_x509_key
FUNCS += functions/gnutls_certificate_set_x509_key.short
FUNCS += functions/gnutls_certificate_set_x509_key_file
FUNCS += functions/gnutls_certificate_set_x509_key_file.short
FUNCS += functions/gnutls_certificate_set_x509_key_file2
FUNCS += functions/gnutls_certificate_set_x509_key_file2.short
FUNCS += functions/gnutls_certificate_set_x509_key_mem
FUNCS += functions/gnutls_certificate_set_x509_key_mem.short
FUNCS += functions/gnutls_certificate_set_x509_key_mem2
FUNCS += functions/gnutls_certificate_set_x509_key_mem2.short
FUNCS += functions/gnutls_certificate_set_x509_simple_pkcs12_file
FUNCS += functions/gnutls_certificate_set_x509_simple_pkcs12_file.short
FUNCS += functions/gnutls_certificate_set_x509_simple_pkcs12_mem
FUNCS += functions/gnutls_certificate_set_x509_simple_pkcs12_mem.short
FUNCS += functions/gnutls_certificate_set_x509_system_trust
FUNCS += functions/gnutls_certificate_set_x509_system_trust.short
FUNCS += functions/gnutls_certificate_set_x509_trust
FUNCS += functions/gnutls_certificate_set_x509_trust.short
FUNCS += functions/gnutls_certificate_set_x509_trust_dir
FUNCS += functions/gnutls_certificate_set_x509_trust_dir.short
FUNCS += functions/gnutls_certificate_set_x509_trust_file
FUNCS += functions/gnutls_certificate_set_x509_trust_file.short
FUNCS += functions/gnutls_certificate_set_x509_trust_mem
FUNCS += functions/gnutls_certificate_set_x509_trust_mem.short
FUNCS += functions/gnutls_certificate_type_get
FUNCS += functions/gnutls_certificate_type_get.short
FUNCS += functions/gnutls_certificate_type_get_id
FUNCS += functions/gnutls_certificate_type_get_id.short
FUNCS += functions/gnutls_certificate_type_get_name
FUNCS += functions/gnutls_certificate_type_get_name.short
FUNCS += functions/gnutls_certificate_type_list
FUNCS += functions/gnutls_certificate_type_list.short
FUNCS += functions/gnutls_certificate_verification_status_print
FUNCS += functions/gnutls_certificate_verification_status_print.short
FUNCS += functions/gnutls_certificate_verify_peers
FUNCS += functions/gnutls_certificate_verify_peers.short
FUNCS += functions/gnutls_certificate_verify_peers2
FUNCS += functions/gnutls_certificate_verify_peers2.short
FUNCS += functions/gnutls_certificate_verify_peers3
FUNCS += functions/gnutls_certificate_verify_peers3.short
FUNCS += functions/gnutls_check_version
FUNCS += functions/gnutls_check_version.short
FUNCS += functions/gnutls_cipher_add_auth
FUNCS += functions/gnutls_cipher_add_auth.short
FUNCS += functions/gnutls_cipher_decrypt
FUNCS += functions/gnutls_cipher_decrypt.short
FUNCS += functions/gnutls_cipher_decrypt2
FUNCS += functions/gnutls_cipher_decrypt2.short
FUNCS += functions/gnutls_cipher_deinit
FUNCS += functions/gnutls_cipher_deinit.short
FUNCS += functions/gnutls_cipher_encrypt
FUNCS += functions/gnutls_cipher_encrypt.short
FUNCS += functions/gnutls_cipher_encrypt2
FUNCS += functions/gnutls_cipher_encrypt2.short
FUNCS += functions/gnutls_cipher_get
FUNCS += functions/gnutls_cipher_get.short
FUNCS += functions/gnutls_cipher_get_block_size
FUNCS += functions/gnutls_cipher_get_block_size.short
FUNCS += functions/gnutls_cipher_get_id
FUNCS += functions/gnutls_cipher_get_id.short
FUNCS += functions/gnutls_cipher_get_iv_size
FUNCS += functions/gnutls_cipher_get_iv_size.short
FUNCS += functions/gnutls_cipher_get_key_size
FUNCS += functions/gnutls_cipher_get_key_size.short
FUNCS += functions/gnutls_cipher_get_name
FUNCS += functions/gnutls_cipher_get_name.short
FUNCS += functions/gnutls_cipher_get_tag_size
FUNCS += functions/gnutls_cipher_get_tag_size.short
FUNCS += functions/gnutls_cipher_init
FUNCS += functions/gnutls_cipher_init.short
FUNCS += functions/gnutls_cipher_list
FUNCS += functions/gnutls_cipher_list.short
FUNCS += functions/gnutls_cipher_set_iv
FUNCS += functions/gnutls_cipher_set_iv.short
FUNCS += functions/gnutls_cipher_suite_get_name
FUNCS += functions/gnutls_cipher_suite_get_name.short
FUNCS += functions/gnutls_cipher_suite_info
FUNCS += functions/gnutls_cipher_suite_info.short
FUNCS += functions/gnutls_cipher_tag
FUNCS += functions/gnutls_cipher_tag.short
FUNCS += functions/gnutls_compression_get
FUNCS += functions/gnutls_compression_get.short
FUNCS += functions/gnutls_compression_get_id
FUNCS += functions/gnutls_compression_get_id.short
FUNCS += functions/gnutls_compression_get_name
FUNCS += functions/gnutls_compression_get_name.short
FUNCS += functions/gnutls_compression_list
FUNCS += functions/gnutls_compression_list.short
FUNCS += functions/gnutls_credentials_clear
FUNCS += functions/gnutls_credentials_clear.short
FUNCS += functions/gnutls_credentials_get
FUNCS += functions/gnutls_credentials_get.short
FUNCS += functions/gnutls_credentials_set
FUNCS += functions/gnutls_credentials_set.short
FUNCS += functions/gnutls_crypto_register_aead_cipher
FUNCS += functions/gnutls_crypto_register_aead_cipher.short
FUNCS += functions/gnutls_crypto_register_cipher
FUNCS += functions/gnutls_crypto_register_cipher.short
FUNCS += functions/gnutls_crypto_register_digest
FUNCS += functions/gnutls_crypto_register_digest.short
FUNCS += functions/gnutls_crypto_register_mac
FUNCS += functions/gnutls_crypto_register_mac.short
FUNCS += functions/gnutls_db_check_entry
FUNCS += functions/gnutls_db_check_entry.short
FUNCS += functions/gnutls_db_check_entry_time
FUNCS += functions/gnutls_db_check_entry_time.short
FUNCS += functions/gnutls_db_get_default_cache_expiration
FUNCS += functions/gnutls_db_get_default_cache_expiration.short
FUNCS += functions/gnutls_db_get_ptr
FUNCS += functions/gnutls_db_get_ptr.short
FUNCS += functions/gnutls_db_remove_session
FUNCS += functions/gnutls_db_remove_session.short
FUNCS += functions/gnutls_db_set_cache_expiration
FUNCS += functions/gnutls_db_set_cache_expiration.short
FUNCS += functions/gnutls_db_set_ptr
FUNCS += functions/gnutls_db_set_ptr.short
FUNCS += functions/gnutls_db_set_remove_function
FUNCS += functions/gnutls_db_set_remove_function.short
FUNCS += functions/gnutls_db_set_retrieve_function
FUNCS += functions/gnutls_db_set_retrieve_function.short
FUNCS += functions/gnutls_db_set_store_function
FUNCS += functions/gnutls_db_set_store_function.short
FUNCS += functions/gnutls_decode_ber_digest_info
FUNCS += functions/gnutls_decode_ber_digest_info.short
FUNCS += functions/gnutls_deinit
FUNCS += functions/gnutls_deinit.short
FUNCS += functions/gnutls_dh_get_group
FUNCS += functions/gnutls_dh_get_group.short
FUNCS += functions/gnutls_dh_get_peers_public_bits
FUNCS += functions/gnutls_dh_get_peers_public_bits.short
FUNCS += functions/gnutls_dh_get_prime_bits
FUNCS += functions/gnutls_dh_get_prime_bits.short
FUNCS += functions/gnutls_dh_get_pubkey
FUNCS += functions/gnutls_dh_get_pubkey.short
FUNCS += functions/gnutls_dh_get_secret_bits
FUNCS += functions/gnutls_dh_get_secret_bits.short
FUNCS += functions/gnutls_dh_params_cpy
FUNCS += functions/gnutls_dh_params_cpy.short
FUNCS += functions/gnutls_dh_params_deinit
FUNCS += functions/gnutls_dh_params_deinit.short
FUNCS += functions/gnutls_dh_params_export2_pkcs3
FUNCS += functions/gnutls_dh_params_export2_pkcs3.short
FUNCS += functions/gnutls_dh_params_export_pkcs3
FUNCS += functions/gnutls_dh_params_export_pkcs3.short
FUNCS += functions/gnutls_dh_params_export_raw
FUNCS += functions/gnutls_dh_params_export_raw.short
FUNCS += functions/gnutls_dh_params_generate2
FUNCS += functions/gnutls_dh_params_generate2.short
FUNCS += functions/gnutls_dh_params_import_dsa
FUNCS += functions/gnutls_dh_params_import_dsa.short
FUNCS += functions/gnutls_dh_params_import_pkcs3
FUNCS += functions/gnutls_dh_params_import_pkcs3.short
FUNCS += functions/gnutls_dh_params_import_raw
FUNCS += functions/gnutls_dh_params_import_raw.short
FUNCS += functions/gnutls_dh_params_import_raw2
FUNCS += functions/gnutls_dh_params_import_raw2.short
FUNCS += functions/gnutls_dh_params_init
FUNCS += functions/gnutls_dh_params_init.short
FUNCS += functions/gnutls_dh_set_prime_bits
FUNCS += functions/gnutls_dh_set_prime_bits.short
FUNCS += functions/gnutls_digest_get_id
FUNCS += functions/gnutls_digest_get_id.short
FUNCS += functions/gnutls_digest_get_name
FUNCS += functions/gnutls_digest_get_name.short
FUNCS += functions/gnutls_digest_get_oid
FUNCS += functions/gnutls_digest_get_oid.short
FUNCS += functions/gnutls_digest_list
FUNCS += functions/gnutls_digest_list.short
FUNCS += functions/gnutls_dtls_cookie_send
FUNCS += functions/gnutls_dtls_cookie_send.short
FUNCS += functions/gnutls_dtls_cookie_verify
FUNCS += functions/gnutls_dtls_cookie_verify.short
FUNCS += functions/gnutls_dtls_get_data_mtu
FUNCS += functions/gnutls_dtls_get_data_mtu.short
FUNCS += functions/gnutls_dtls_get_mtu
FUNCS += functions/gnutls_dtls_get_mtu.short
FUNCS += functions/gnutls_dtls_get_timeout
FUNCS += functions/gnutls_dtls_get_timeout.short
FUNCS += functions/gnutls_dtls_prestate_set
FUNCS += functions/gnutls_dtls_prestate_set.short
FUNCS += functions/gnutls_dtls_set_data_mtu
FUNCS += functions/gnutls_dtls_set_data_mtu.short
FUNCS += functions/gnutls_dtls_set_mtu
FUNCS += functions/gnutls_dtls_set_mtu.short
FUNCS += functions/gnutls_dtls_set_timeouts
FUNCS += functions/gnutls_dtls_set_timeouts.short
FUNCS += functions/gnutls_ecc_curve_get
FUNCS += functions/gnutls_ecc_curve_get.short
FUNCS += functions/gnutls_ecc_curve_get_id
FUNCS += functions/gnutls_ecc_curve_get_id.short
FUNCS += functions/gnutls_ecc_curve_get_name
FUNCS += functions/gnutls_ecc_curve_get_name.short
FUNCS += functions/gnutls_ecc_curve_get_oid
FUNCS += functions/gnutls_ecc_curve_get_oid.short
FUNCS += functions/gnutls_ecc_curve_get_pk
FUNCS += functions/gnutls_ecc_curve_get_pk.short
FUNCS += functions/gnutls_ecc_curve_get_size
FUNCS += functions/gnutls_ecc_curve_get_size.short
FUNCS += functions/gnutls_ecc_curve_list
FUNCS += functions/gnutls_ecc_curve_list.short
FUNCS += functions/gnutls_encode_ber_digest_info
FUNCS += functions/gnutls_encode_ber_digest_info.short
FUNCS += functions/gnutls_error_is_fatal
FUNCS += functions/gnutls_error_is_fatal.short
FUNCS += functions/gnutls_error_to_alert
FUNCS += functions/gnutls_error_to_alert.short
FUNCS += functions/gnutls_est_record_overhead_size
FUNCS += functions/gnutls_est_record_overhead_size.short
FUNCS += functions/gnutls_ext_get_data
FUNCS += functions/gnutls_ext_get_data.short
FUNCS += functions/gnutls_ext_get_name
FUNCS += functions/gnutls_ext_get_name.short
FUNCS += functions/gnutls_ext_register
FUNCS += functions/gnutls_ext_register.short
FUNCS += functions/gnutls_ext_set_data
FUNCS += functions/gnutls_ext_set_data.short
FUNCS += functions/gnutls_fingerprint
FUNCS += functions/gnutls_fingerprint.short
FUNCS += functions/gnutls_fips140_mode_enabled
FUNCS += functions/gnutls_fips140_mode_enabled.short
FUNCS += functions/gnutls_global_deinit
FUNCS += functions/gnutls_global_deinit.short
FUNCS += functions/gnutls_global_init
FUNCS += functions/gnutls_global_init.short
FUNCS += functions/gnutls_global_set_audit_log_function
FUNCS += functions/gnutls_global_set_audit_log_function.short
FUNCS += functions/gnutls_global_set_log_function
FUNCS += functions/gnutls_global_set_log_function.short
FUNCS += functions/gnutls_global_set_log_level
FUNCS += functions/gnutls_global_set_log_level.short
FUNCS += functions/gnutls_global_set_mem_functions
FUNCS += functions/gnutls_global_set_mem_functions.short
FUNCS += functions/gnutls_global_set_mutex
FUNCS += functions/gnutls_global_set_mutex.short
FUNCS += functions/gnutls_global_set_time_function
FUNCS += functions/gnutls_global_set_time_function.short
FUNCS += functions/gnutls_handshake
FUNCS += functions/gnutls_handshake.short
FUNCS += functions/gnutls_handshake_description_get_name
FUNCS += functions/gnutls_handshake_description_get_name.short
FUNCS += functions/gnutls_handshake_get_last_in
FUNCS += functions/gnutls_handshake_get_last_in.short
FUNCS += functions/gnutls_handshake_get_last_out
FUNCS += functions/gnutls_handshake_get_last_out.short
FUNCS += functions/gnutls_handshake_set_hook_function
FUNCS += functions/gnutls_handshake_set_hook_function.short
FUNCS += functions/gnutls_handshake_set_max_packet_length
FUNCS += functions/gnutls_handshake_set_max_packet_length.short
FUNCS += functions/gnutls_handshake_set_post_client_hello_function
FUNCS += functions/gnutls_handshake_set_post_client_hello_function.short
FUNCS += functions/gnutls_handshake_set_private_extensions
FUNCS += functions/gnutls_handshake_set_private_extensions.short
FUNCS += functions/gnutls_handshake_set_random
FUNCS += functions/gnutls_handshake_set_random.short
FUNCS += functions/gnutls_handshake_set_timeout
FUNCS += functions/gnutls_handshake_set_timeout.short
FUNCS += functions/gnutls_hash
FUNCS += functions/gnutls_hash.short
FUNCS += functions/gnutls_hash_deinit
FUNCS += functions/gnutls_hash_deinit.short
FUNCS += functions/gnutls_hash_fast
FUNCS += functions/gnutls_hash_fast.short
FUNCS += functions/gnutls_hash_get_len
FUNCS += functions/gnutls_hash_get_len.short
FUNCS += functions/gnutls_hash_init
FUNCS += functions/gnutls_hash_init.short
FUNCS += functions/gnutls_hash_output
FUNCS += functions/gnutls_hash_output.short
FUNCS += functions/gnutls_heartbeat_allowed
FUNCS += functions/gnutls_heartbeat_allowed.short
FUNCS += functions/gnutls_heartbeat_enable
FUNCS += functions/gnutls_heartbeat_enable.short
FUNCS += functions/gnutls_heartbeat_get_timeout
FUNCS += functions/gnutls_heartbeat_get_timeout.short
FUNCS += functions/gnutls_heartbeat_ping
FUNCS += functions/gnutls_heartbeat_ping.short
FUNCS += functions/gnutls_heartbeat_pong
FUNCS += functions/gnutls_heartbeat_pong.short
FUNCS += functions/gnutls_heartbeat_set_timeouts
FUNCS += functions/gnutls_heartbeat_set_timeouts.short
FUNCS += functions/gnutls_hex2bin
FUNCS += functions/gnutls_hex2bin.short
FUNCS += functions/gnutls_hex_decode
FUNCS += functions/gnutls_hex_decode.short
FUNCS += functions/gnutls_hex_decode2
FUNCS += functions/gnutls_hex_decode2.short
FUNCS += functions/gnutls_hex_encode
FUNCS += functions/gnutls_hex_encode.short
FUNCS += functions/gnutls_hex_encode2
FUNCS += functions/gnutls_hex_encode2.short
FUNCS += functions/gnutls_hmac
FUNCS += functions/gnutls_hmac.short
FUNCS += functions/gnutls_hmac_deinit
FUNCS += functions/gnutls_hmac_deinit.short
FUNCS += functions/gnutls_hmac_fast
FUNCS += functions/gnutls_hmac_fast.short
FUNCS += functions/gnutls_hmac_get_len
FUNCS += functions/gnutls_hmac_get_len.short
FUNCS += functions/gnutls_hmac_init
FUNCS += functions/gnutls_hmac_init.short
FUNCS += functions/gnutls_hmac_output
FUNCS += functions/gnutls_hmac_output.short
FUNCS += functions/gnutls_hmac_set_nonce
FUNCS += functions/gnutls_hmac_set_nonce.short
FUNCS += functions/gnutls_idna_map
FUNCS += functions/gnutls_idna_map.short
FUNCS += functions/gnutls_idna_reverse_map
FUNCS += functions/gnutls_idna_reverse_map.short
FUNCS += functions/gnutls_init
FUNCS += functions/gnutls_init.short
FUNCS += functions/gnutls_key_generate
FUNCS += functions/gnutls_key_generate.short
FUNCS += functions/gnutls_kx_get
FUNCS += functions/gnutls_kx_get.short
FUNCS += functions/gnutls_kx_get_id
FUNCS += functions/gnutls_kx_get_id.short
FUNCS += functions/gnutls_kx_get_name
FUNCS += functions/gnutls_kx_get_name.short
FUNCS += functions/gnutls_kx_list
FUNCS += functions/gnutls_kx_list.short
FUNCS += functions/gnutls_load_file
FUNCS += functions/gnutls_load_file.short
FUNCS += functions/gnutls_mac_get
FUNCS += functions/gnutls_mac_get.short
FUNCS += functions/gnutls_mac_get_id
FUNCS += functions/gnutls_mac_get_id.short
FUNCS += functions/gnutls_mac_get_key_size
FUNCS += functions/gnutls_mac_get_key_size.short
FUNCS += functions/gnutls_mac_get_name
FUNCS += functions/gnutls_mac_get_name.short
FUNCS += functions/gnutls_mac_get_nonce_size
FUNCS += functions/gnutls_mac_get_nonce_size.short
FUNCS += functions/gnutls_mac_list
FUNCS += functions/gnutls_mac_list.short
FUNCS += functions/gnutls_memcmp
FUNCS += functions/gnutls_memcmp.short
FUNCS += functions/gnutls_memset
FUNCS += functions/gnutls_memset.short
FUNCS += functions/gnutls_ocsp_req_add_cert
FUNCS += functions/gnutls_ocsp_req_add_cert.short
FUNCS += functions/gnutls_ocsp_req_add_cert_id
FUNCS += functions/gnutls_ocsp_req_add_cert_id.short
FUNCS += functions/gnutls_ocsp_req_deinit
FUNCS += functions/gnutls_ocsp_req_deinit.short
FUNCS += functions/gnutls_ocsp_req_export
FUNCS += functions/gnutls_ocsp_req_export.short
FUNCS += functions/gnutls_ocsp_req_get_cert_id
FUNCS += functions/gnutls_ocsp_req_get_cert_id.short
FUNCS += functions/gnutls_ocsp_req_get_extension
FUNCS += functions/gnutls_ocsp_req_get_extension.short
FUNCS += functions/gnutls_ocsp_req_get_nonce
FUNCS += functions/gnutls_ocsp_req_get_nonce.short
FUNCS += functions/gnutls_ocsp_req_get_version
FUNCS += functions/gnutls_ocsp_req_get_version.short
FUNCS += functions/gnutls_ocsp_req_import
FUNCS += functions/gnutls_ocsp_req_import.short
FUNCS += functions/gnutls_ocsp_req_init
FUNCS += functions/gnutls_ocsp_req_init.short
FUNCS += functions/gnutls_ocsp_req_print
FUNCS += functions/gnutls_ocsp_req_print.short
FUNCS += functions/gnutls_ocsp_req_randomize_nonce
FUNCS += functions/gnutls_ocsp_req_randomize_nonce.short
FUNCS += functions/gnutls_ocsp_req_set_extension
FUNCS += functions/gnutls_ocsp_req_set_extension.short
FUNCS += functions/gnutls_ocsp_req_set_nonce
FUNCS += functions/gnutls_ocsp_req_set_nonce.short
FUNCS += functions/gnutls_ocsp_resp_check_crt
FUNCS += functions/gnutls_ocsp_resp_check_crt.short
FUNCS += functions/gnutls_ocsp_resp_deinit
FUNCS += functions/gnutls_ocsp_resp_deinit.short
FUNCS += functions/gnutls_ocsp_resp_export
FUNCS += functions/gnutls_ocsp_resp_export.short
FUNCS += functions/gnutls_ocsp_resp_get_certs
FUNCS += functions/gnutls_ocsp_resp_get_certs.short
FUNCS += functions/gnutls_ocsp_resp_get_extension
FUNCS += functions/gnutls_ocsp_resp_get_extension.short
FUNCS += functions/gnutls_ocsp_resp_get_nonce
FUNCS += functions/gnutls_ocsp_resp_get_nonce.short
FUNCS += functions/gnutls_ocsp_resp_get_produced
FUNCS += functions/gnutls_ocsp_resp_get_produced.short
FUNCS += functions/gnutls_ocsp_resp_get_responder
FUNCS += functions/gnutls_ocsp_resp_get_responder.short
FUNCS += functions/gnutls_ocsp_resp_get_responder2
FUNCS += functions/gnutls_ocsp_resp_get_responder2.short
FUNCS += functions/gnutls_ocsp_resp_get_responder_raw_id
FUNCS += functions/gnutls_ocsp_resp_get_responder_raw_id.short
FUNCS += functions/gnutls_ocsp_resp_get_response
FUNCS += functions/gnutls_ocsp_resp_get_response.short
FUNCS += functions/gnutls_ocsp_resp_get_signature
FUNCS += functions/gnutls_ocsp_resp_get_signature.short
FUNCS += functions/gnutls_ocsp_resp_get_signature_algorithm
FUNCS += functions/gnutls_ocsp_resp_get_signature_algorithm.short
FUNCS += functions/gnutls_ocsp_resp_get_single
FUNCS += functions/gnutls_ocsp_resp_get_single.short
FUNCS += functions/gnutls_ocsp_resp_get_status
FUNCS += functions/gnutls_ocsp_resp_get_status.short
FUNCS += functions/gnutls_ocsp_resp_get_version
FUNCS += functions/gnutls_ocsp_resp_get_version.short
FUNCS += functions/gnutls_ocsp_resp_import
FUNCS += functions/gnutls_ocsp_resp_import.short
FUNCS += functions/gnutls_ocsp_resp_init
FUNCS += functions/gnutls_ocsp_resp_init.short
FUNCS += functions/gnutls_ocsp_resp_print
FUNCS += functions/gnutls_ocsp_resp_print.short
FUNCS += functions/gnutls_ocsp_resp_verify
FUNCS += functions/gnutls_ocsp_resp_verify.short
FUNCS += functions/gnutls_ocsp_resp_verify_direct
FUNCS += functions/gnutls_ocsp_resp_verify_direct.short
FUNCS += functions/gnutls_ocsp_status_request_enable_client
FUNCS += functions/gnutls_ocsp_status_request_enable_client.short
FUNCS += functions/gnutls_ocsp_status_request_get
FUNCS += functions/gnutls_ocsp_status_request_get.short
FUNCS += functions/gnutls_ocsp_status_request_is_checked
FUNCS += functions/gnutls_ocsp_status_request_is_checked.short
FUNCS += functions/gnutls_oid_to_digest
FUNCS += functions/gnutls_oid_to_digest.short
FUNCS += functions/gnutls_oid_to_ecc_curve
FUNCS += functions/gnutls_oid_to_ecc_curve.short
FUNCS += functions/gnutls_oid_to_mac
FUNCS += functions/gnutls_oid_to_mac.short
FUNCS += functions/gnutls_oid_to_pk
FUNCS += functions/gnutls_oid_to_pk.short
FUNCS += functions/gnutls_oid_to_sign
FUNCS += functions/gnutls_oid_to_sign.short
FUNCS += functions/gnutls_openpgp_privkey_sign_hash
FUNCS += functions/gnutls_openpgp_privkey_sign_hash.short
FUNCS += functions/gnutls_openpgp_send_cert
FUNCS += functions/gnutls_openpgp_send_cert.short
FUNCS += functions/gnutls_packet_deinit
FUNCS += functions/gnutls_packet_deinit.short
FUNCS += functions/gnutls_packet_get
FUNCS += functions/gnutls_packet_get.short
FUNCS += functions/gnutls_pcert_deinit
FUNCS += functions/gnutls_pcert_deinit.short
FUNCS += functions/gnutls_pcert_export_openpgp
FUNCS += functions/gnutls_pcert_export_openpgp.short
FUNCS += functions/gnutls_pcert_export_x509
FUNCS += functions/gnutls_pcert_export_x509.short
FUNCS += functions/gnutls_pcert_import_openpgp
FUNCS += functions/gnutls_pcert_import_openpgp.short
FUNCS += functions/gnutls_pcert_import_openpgp_raw
FUNCS += functions/gnutls_pcert_import_openpgp_raw.short
FUNCS += functions/gnutls_pcert_import_x509
FUNCS += functions/gnutls_pcert_import_x509.short
FUNCS += functions/gnutls_pcert_import_x509_list
FUNCS += functions/gnutls_pcert_import_x509_list.short
FUNCS += functions/gnutls_pcert_import_x509_raw
FUNCS += functions/gnutls_pcert_import_x509_raw.short
FUNCS += functions/gnutls_pcert_list_import_x509_raw
FUNCS += functions/gnutls_pcert_list_import_x509_raw.short
FUNCS += functions/gnutls_pem_base64_decode
FUNCS += functions/gnutls_pem_base64_decode.short
FUNCS += functions/gnutls_pem_base64_decode2
FUNCS += functions/gnutls_pem_base64_decode2.short
FUNCS += functions/gnutls_pem_base64_encode
FUNCS += functions/gnutls_pem_base64_encode.short
FUNCS += functions/gnutls_pem_base64_encode2
FUNCS += functions/gnutls_pem_base64_encode2.short
FUNCS += functions/gnutls_perror
FUNCS += functions/gnutls_perror.short
FUNCS += functions/gnutls_pk_algorithm_get_name
FUNCS += functions/gnutls_pk_algorithm_get_name.short
FUNCS += functions/gnutls_pk_bits_to_sec_param
FUNCS += functions/gnutls_pk_bits_to_sec_param.short
FUNCS += functions/gnutls_pkcs11_add_provider
FUNCS += functions/gnutls_pkcs11_add_provider.short
FUNCS += functions/gnutls_pkcs11_copy_attached_extension
FUNCS += functions/gnutls_pkcs11_copy_attached_extension.short
FUNCS += functions/gnutls_pkcs11_copy_pubkey
FUNCS += functions/gnutls_pkcs11_copy_pubkey.short
FUNCS += functions/gnutls_pkcs11_copy_secret_key
FUNCS += functions/gnutls_pkcs11_copy_secret_key.short
FUNCS += functions/gnutls_pkcs11_copy_x509_crt
FUNCS += functions/gnutls_pkcs11_copy_x509_crt.short
FUNCS += functions/gnutls_pkcs11_copy_x509_crt2
FUNCS += functions/gnutls_pkcs11_copy_x509_crt2.short
FUNCS += functions/gnutls_pkcs11_copy_x509_privkey
FUNCS += functions/gnutls_pkcs11_copy_x509_privkey.short
FUNCS += functions/gnutls_pkcs11_copy_x509_privkey2
FUNCS += functions/gnutls_pkcs11_copy_x509_privkey2.short
FUNCS += functions/gnutls_pkcs11_crt_is_known
FUNCS += functions/gnutls_pkcs11_crt_is_known.short
FUNCS += functions/gnutls_pkcs11_deinit
FUNCS += functions/gnutls_pkcs11_deinit.short
FUNCS += functions/gnutls_pkcs11_delete_url
FUNCS += functions/gnutls_pkcs11_delete_url.short
FUNCS += functions/gnutls_pkcs11_get_pin_function
FUNCS += functions/gnutls_pkcs11_get_pin_function.short
FUNCS += functions/gnutls_pkcs11_get_raw_issuer
FUNCS += functions/gnutls_pkcs11_get_raw_issuer.short
FUNCS += functions/gnutls_pkcs11_get_raw_issuer_by_dn
FUNCS += functions/gnutls_pkcs11_get_raw_issuer_by_dn.short
FUNCS += functions/gnutls_pkcs11_get_raw_issuer_by_subject_key_id
FUNCS += functions/gnutls_pkcs11_get_raw_issuer_by_subject_key_id.short
FUNCS += functions/gnutls_pkcs11_init
FUNCS += functions/gnutls_pkcs11_init.short
FUNCS += functions/gnutls_pkcs11_obj_deinit
FUNCS += functions/gnutls_pkcs11_obj_deinit.short
FUNCS += functions/gnutls_pkcs11_obj_export
FUNCS += functions/gnutls_pkcs11_obj_export.short
FUNCS += functions/gnutls_pkcs11_obj_export2
FUNCS += functions/gnutls_pkcs11_obj_export2.short
FUNCS += functions/gnutls_pkcs11_obj_export3
FUNCS += functions/gnutls_pkcs11_obj_export3.short
FUNCS += functions/gnutls_pkcs11_obj_export_url
FUNCS += functions/gnutls_pkcs11_obj_export_url.short
FUNCS += functions/gnutls_pkcs11_obj_flags_get_str
FUNCS += functions/gnutls_pkcs11_obj_flags_get_str.short
FUNCS += functions/gnutls_pkcs11_obj_get_exts
FUNCS += functions/gnutls_pkcs11_obj_get_exts.short
FUNCS += functions/gnutls_pkcs11_obj_get_flags
FUNCS += functions/gnutls_pkcs11_obj_get_flags.short
FUNCS += functions/gnutls_pkcs11_obj_get_info
FUNCS += functions/gnutls_pkcs11_obj_get_info.short
FUNCS += functions/gnutls_pkcs11_obj_get_type
FUNCS += functions/gnutls_pkcs11_obj_get_type.short
FUNCS += functions/gnutls_pkcs11_obj_import_url
FUNCS += functions/gnutls_pkcs11_obj_import_url.short
FUNCS += functions/gnutls_pkcs11_obj_init
FUNCS += functions/gnutls_pkcs11_obj_init.short
FUNCS += functions/gnutls_pkcs11_obj_list_import_url3
FUNCS += functions/gnutls_pkcs11_obj_list_import_url3.short
FUNCS += functions/gnutls_pkcs11_obj_list_import_url4
FUNCS += functions/gnutls_pkcs11_obj_list_import_url4.short
FUNCS += functions/gnutls_pkcs11_obj_set_info
FUNCS += functions/gnutls_pkcs11_obj_set_info.short
FUNCS += functions/gnutls_pkcs11_obj_set_pin_function
FUNCS += functions/gnutls_pkcs11_obj_set_pin_function.short
FUNCS += functions/gnutls_pkcs11_privkey_cpy
FUNCS += functions/gnutls_pkcs11_privkey_cpy.short
FUNCS += functions/gnutls_pkcs11_privkey_deinit
FUNCS += functions/gnutls_pkcs11_privkey_deinit.short
FUNCS += functions/gnutls_pkcs11_privkey_export_pubkey
FUNCS += functions/gnutls_pkcs11_privkey_export_pubkey.short
FUNCS += functions/gnutls_pkcs11_privkey_export_url
FUNCS += functions/gnutls_pkcs11_privkey_export_url.short
FUNCS += functions/gnutls_pkcs11_privkey_generate
FUNCS += functions/gnutls_pkcs11_privkey_generate.short
FUNCS += functions/gnutls_pkcs11_privkey_generate2
FUNCS += functions/gnutls_pkcs11_privkey_generate2.short
FUNCS += functions/gnutls_pkcs11_privkey_generate3
FUNCS += functions/gnutls_pkcs11_privkey_generate3.short
FUNCS += functions/gnutls_pkcs11_privkey_get_info
FUNCS += functions/gnutls_pkcs11_privkey_get_info.short
FUNCS += functions/gnutls_pkcs11_privkey_get_pk_algorithm
FUNCS += functions/gnutls_pkcs11_privkey_get_pk_algorithm.short
FUNCS += functions/gnutls_pkcs11_privkey_import_url
FUNCS += functions/gnutls_pkcs11_privkey_import_url.short
FUNCS += functions/gnutls_pkcs11_privkey_init
FUNCS += functions/gnutls_pkcs11_privkey_init.short
FUNCS += functions/gnutls_pkcs11_privkey_set_pin_function
FUNCS += functions/gnutls_pkcs11_privkey_set_pin_function.short
FUNCS += functions/gnutls_pkcs11_privkey_status
FUNCS += functions/gnutls_pkcs11_privkey_status.short
FUNCS += functions/gnutls_pkcs11_reinit
FUNCS += functions/gnutls_pkcs11_reinit.short
FUNCS += functions/gnutls_pkcs11_set_pin_function
FUNCS += functions/gnutls_pkcs11_set_pin_function.short
FUNCS += functions/gnutls_pkcs11_set_token_function
FUNCS += functions/gnutls_pkcs11_set_token_function.short
FUNCS += functions/gnutls_pkcs11_token_get_flags
FUNCS += functions/gnutls_pkcs11_token_get_flags.short
FUNCS += functions/gnutls_pkcs11_token_get_info
FUNCS += functions/gnutls_pkcs11_token_get_info.short
FUNCS += functions/gnutls_pkcs11_token_get_mechanism
FUNCS += functions/gnutls_pkcs11_token_get_mechanism.short
FUNCS += functions/gnutls_pkcs11_token_get_random
FUNCS += functions/gnutls_pkcs11_token_get_random.short
FUNCS += functions/gnutls_pkcs11_token_get_url
FUNCS += functions/gnutls_pkcs11_token_get_url.short
FUNCS += functions/gnutls_pkcs11_token_init
FUNCS += functions/gnutls_pkcs11_token_init.short
FUNCS += functions/gnutls_pkcs11_token_set_pin
FUNCS += functions/gnutls_pkcs11_token_set_pin.short
FUNCS += functions/gnutls_pkcs11_type_get_name
FUNCS += functions/gnutls_pkcs11_type_get_name.short
FUNCS += functions/gnutls_pkcs12_bag_decrypt
FUNCS += functions/gnutls_pkcs12_bag_decrypt.short
FUNCS += functions/gnutls_pkcs12_bag_deinit
FUNCS += functions/gnutls_pkcs12_bag_deinit.short
FUNCS += functions/gnutls_pkcs12_bag_enc_info
FUNCS += functions/gnutls_pkcs12_bag_enc_info.short
FUNCS += functions/gnutls_pkcs12_bag_encrypt
FUNCS += functions/gnutls_pkcs12_bag_encrypt.short
FUNCS += functions/gnutls_pkcs12_bag_get_count
FUNCS += functions/gnutls_pkcs12_bag_get_count.short
FUNCS += functions/gnutls_pkcs12_bag_get_data
FUNCS += functions/gnutls_pkcs12_bag_get_data.short
FUNCS += functions/gnutls_pkcs12_bag_get_friendly_name
FUNCS += functions/gnutls_pkcs12_bag_get_friendly_name.short
FUNCS += functions/gnutls_pkcs12_bag_get_key_id
FUNCS += functions/gnutls_pkcs12_bag_get_key_id.short
FUNCS += functions/gnutls_pkcs12_bag_get_type
FUNCS += functions/gnutls_pkcs12_bag_get_type.short
FUNCS += functions/gnutls_pkcs12_bag_init
FUNCS += functions/gnutls_pkcs12_bag_init.short
FUNCS += functions/gnutls_pkcs12_bag_set_crl
FUNCS += functions/gnutls_pkcs12_bag_set_crl.short
FUNCS += functions/gnutls_pkcs12_bag_set_crt
FUNCS += functions/gnutls_pkcs12_bag_set_crt.short
FUNCS += functions/gnutls_pkcs12_bag_set_data
FUNCS += functions/gnutls_pkcs12_bag_set_data.short
FUNCS += functions/gnutls_pkcs12_bag_set_friendly_name
FUNCS += functions/gnutls_pkcs12_bag_set_friendly_name.short
FUNCS += functions/gnutls_pkcs12_bag_set_key_id
FUNCS += functions/gnutls_pkcs12_bag_set_key_id.short
FUNCS += functions/gnutls_pkcs12_bag_set_privkey
FUNCS += functions/gnutls_pkcs12_bag_set_privkey.short
FUNCS += functions/gnutls_pkcs12_deinit
FUNCS += functions/gnutls_pkcs12_deinit.short
FUNCS += functions/gnutls_pkcs12_export
FUNCS += functions/gnutls_pkcs12_export.short
FUNCS += functions/gnutls_pkcs12_export2
FUNCS += functions/gnutls_pkcs12_export2.short
FUNCS += functions/gnutls_pkcs12_generate_mac
FUNCS += functions/gnutls_pkcs12_generate_mac.short
FUNCS += functions/gnutls_pkcs12_generate_mac2
FUNCS += functions/gnutls_pkcs12_generate_mac2.short
FUNCS += functions/gnutls_pkcs12_get_bag
FUNCS += functions/gnutls_pkcs12_get_bag.short
FUNCS += functions/gnutls_pkcs12_import
FUNCS += functions/gnutls_pkcs12_import.short
FUNCS += functions/gnutls_pkcs12_init
FUNCS += functions/gnutls_pkcs12_init.short
FUNCS += functions/gnutls_pkcs12_mac_info
FUNCS += functions/gnutls_pkcs12_mac_info.short
FUNCS += functions/gnutls_pkcs12_set_bag
FUNCS += functions/gnutls_pkcs12_set_bag.short
FUNCS += functions/gnutls_pkcs12_simple_parse
FUNCS += functions/gnutls_pkcs12_simple_parse.short
FUNCS += functions/gnutls_pkcs12_verify_mac
FUNCS += functions/gnutls_pkcs12_verify_mac.short
FUNCS += functions/gnutls_pkcs7_add_attr
FUNCS += functions/gnutls_pkcs7_add_attr.short
FUNCS += functions/gnutls_pkcs7_attrs_deinit
FUNCS += functions/gnutls_pkcs7_attrs_deinit.short
FUNCS += functions/gnutls_pkcs7_deinit
FUNCS += functions/gnutls_pkcs7_deinit.short
FUNCS += functions/gnutls_pkcs7_delete_crl
FUNCS += functions/gnutls_pkcs7_delete_crl.short
FUNCS += functions/gnutls_pkcs7_delete_crt
FUNCS += functions/gnutls_pkcs7_delete_crt.short
FUNCS += functions/gnutls_pkcs7_export
FUNCS += functions/gnutls_pkcs7_export.short
FUNCS += functions/gnutls_pkcs7_export2
FUNCS += functions/gnutls_pkcs7_export2.short
FUNCS += functions/gnutls_pkcs7_get_attr
FUNCS += functions/gnutls_pkcs7_get_attr.short
FUNCS += functions/gnutls_pkcs7_get_crl_count
FUNCS += functions/gnutls_pkcs7_get_crl_count.short
FUNCS += functions/gnutls_pkcs7_get_crl_raw
FUNCS += functions/gnutls_pkcs7_get_crl_raw.short
FUNCS += functions/gnutls_pkcs7_get_crl_raw2
FUNCS += functions/gnutls_pkcs7_get_crl_raw2.short
FUNCS += functions/gnutls_pkcs7_get_crt_count
FUNCS += functions/gnutls_pkcs7_get_crt_count.short
FUNCS += functions/gnutls_pkcs7_get_crt_raw
FUNCS += functions/gnutls_pkcs7_get_crt_raw.short
FUNCS += functions/gnutls_pkcs7_get_crt_raw2
FUNCS += functions/gnutls_pkcs7_get_crt_raw2.short
FUNCS += functions/gnutls_pkcs7_get_embedded_data
FUNCS += functions/gnutls_pkcs7_get_embedded_data.short
FUNCS += functions/gnutls_pkcs7_get_embedded_data_oid
FUNCS += functions/gnutls_pkcs7_get_embedded_data_oid.short
FUNCS += functions/gnutls_pkcs7_get_signature_count
FUNCS += functions/gnutls_pkcs7_get_signature_count.short
FUNCS += functions/gnutls_pkcs7_get_signature_info
FUNCS += functions/gnutls_pkcs7_get_signature_info.short
FUNCS += functions/gnutls_pkcs7_import
FUNCS += functions/gnutls_pkcs7_import.short
FUNCS += functions/gnutls_pkcs7_init
FUNCS += functions/gnutls_pkcs7_init.short
FUNCS += functions/gnutls_pkcs7_print
FUNCS += functions/gnutls_pkcs7_print.short
FUNCS += functions/gnutls_pkcs7_set_crl
FUNCS += functions/gnutls_pkcs7_set_crl.short
FUNCS += functions/gnutls_pkcs7_set_crl_raw
FUNCS += functions/gnutls_pkcs7_set_crl_raw.short
FUNCS += functions/gnutls_pkcs7_set_crt
FUNCS += functions/gnutls_pkcs7_set_crt.short
FUNCS += functions/gnutls_pkcs7_set_crt_raw
FUNCS += functions/gnutls_pkcs7_set_crt_raw.short
FUNCS += functions/gnutls_pkcs7_sign
FUNCS += functions/gnutls_pkcs7_sign.short
FUNCS += functions/gnutls_pkcs7_signature_info_deinit
FUNCS += functions/gnutls_pkcs7_signature_info_deinit.short
FUNCS += functions/gnutls_pkcs7_verify
FUNCS += functions/gnutls_pkcs7_verify.short
FUNCS += functions/gnutls_pkcs7_verify_direct
FUNCS += functions/gnutls_pkcs7_verify_direct.short
FUNCS += functions/gnutls_pkcs8_info
FUNCS += functions/gnutls_pkcs8_info.short
FUNCS += functions/gnutls_pkcs_schema_get_name
FUNCS += functions/gnutls_pkcs_schema_get_name.short
FUNCS += functions/gnutls_pkcs_schema_get_oid
FUNCS += functions/gnutls_pkcs_schema_get_oid.short
FUNCS += functions/gnutls_pk_get_id
FUNCS += functions/gnutls_pk_get_id.short
FUNCS += functions/gnutls_pk_get_name
FUNCS += functions/gnutls_pk_get_name.short
FUNCS += functions/gnutls_pk_get_oid
FUNCS += functions/gnutls_pk_get_oid.short
FUNCS += functions/gnutls_pk_list
FUNCS += functions/gnutls_pk_list.short
FUNCS += functions/gnutls_pk_to_sign
FUNCS += functions/gnutls_pk_to_sign.short
FUNCS += functions/gnutls_prf
FUNCS += functions/gnutls_prf.short
FUNCS += functions/gnutls_prf_raw
FUNCS += functions/gnutls_prf_raw.short
FUNCS += functions/gnutls_prf_rfc5705
FUNCS += functions/gnutls_prf_rfc5705.short
FUNCS += functions/gnutls_priority_certificate_type_list
FUNCS += functions/gnutls_priority_certificate_type_list.short
FUNCS += functions/gnutls_priority_cipher_list
FUNCS += functions/gnutls_priority_cipher_list.short
FUNCS += functions/gnutls_priority_compression_list
FUNCS += functions/gnutls_priority_compression_list.short
FUNCS += functions/gnutls_priority_deinit
FUNCS += functions/gnutls_priority_deinit.short
FUNCS += functions/gnutls_priority_ecc_curve_list
FUNCS += functions/gnutls_priority_ecc_curve_list.short
FUNCS += functions/gnutls_priority_get_cipher_suite_index
FUNCS += functions/gnutls_priority_get_cipher_suite_index.short
FUNCS += functions/gnutls_priority_init
FUNCS += functions/gnutls_priority_init.short
FUNCS += functions/gnutls_priority_kx_list
FUNCS += functions/gnutls_priority_kx_list.short
FUNCS += functions/gnutls_priority_mac_list
FUNCS += functions/gnutls_priority_mac_list.short
FUNCS += functions/gnutls_priority_protocol_list
FUNCS += functions/gnutls_priority_protocol_list.short
FUNCS += functions/gnutls_priority_set
FUNCS += functions/gnutls_priority_set.short
FUNCS += functions/gnutls_priority_set_direct
FUNCS += functions/gnutls_priority_set_direct.short
FUNCS += functions/gnutls_priority_sign_list
FUNCS += functions/gnutls_priority_sign_list.short
FUNCS += functions/gnutls_priority_string_list
FUNCS += functions/gnutls_priority_string_list.short
FUNCS += functions/gnutls_privkey_decrypt_data
FUNCS += functions/gnutls_privkey_decrypt_data.short
FUNCS += functions/gnutls_privkey_deinit
FUNCS += functions/gnutls_privkey_deinit.short
FUNCS += functions/gnutls_privkey_export_dsa_raw
FUNCS += functions/gnutls_privkey_export_dsa_raw.short
FUNCS += functions/gnutls_privkey_export_ecc_raw
FUNCS += functions/gnutls_privkey_export_ecc_raw.short
FUNCS += functions/gnutls_privkey_export_openpgp
FUNCS += functions/gnutls_privkey_export_openpgp.short
FUNCS += functions/gnutls_privkey_export_pkcs11
FUNCS += functions/gnutls_privkey_export_pkcs11.short
FUNCS += functions/gnutls_privkey_export_rsa_raw
FUNCS += functions/gnutls_privkey_export_rsa_raw.short
FUNCS += functions/gnutls_privkey_export_x509
FUNCS += functions/gnutls_privkey_export_x509.short
FUNCS += functions/gnutls_privkey_generate
FUNCS += functions/gnutls_privkey_generate.short
FUNCS += functions/gnutls_privkey_generate2
FUNCS += functions/gnutls_privkey_generate2.short
FUNCS += functions/gnutls_privkey_get_pk_algorithm
FUNCS += functions/gnutls_privkey_get_pk_algorithm.short
FUNCS += functions/gnutls_privkey_get_seed
FUNCS += functions/gnutls_privkey_get_seed.short
FUNCS += functions/gnutls_privkey_get_type
FUNCS += functions/gnutls_privkey_get_type.short
FUNCS += functions/gnutls_privkey_import_dsa_raw
FUNCS += functions/gnutls_privkey_import_dsa_raw.short
FUNCS += functions/gnutls_privkey_import_ecc_raw
FUNCS += functions/gnutls_privkey_import_ecc_raw.short
FUNCS += functions/gnutls_privkey_import_ext
FUNCS += functions/gnutls_privkey_import_ext.short
FUNCS += functions/gnutls_privkey_import_ext2
FUNCS += functions/gnutls_privkey_import_ext2.short
FUNCS += functions/gnutls_privkey_import_ext3
FUNCS += functions/gnutls_privkey_import_ext3.short
FUNCS += functions/gnutls_privkey_import_openpgp
FUNCS += functions/gnutls_privkey_import_openpgp.short
FUNCS += functions/gnutls_privkey_import_openpgp_raw
FUNCS += functions/gnutls_privkey_import_openpgp_raw.short
FUNCS += functions/gnutls_privkey_import_pkcs11
FUNCS += functions/gnutls_privkey_import_pkcs11.short
FUNCS += functions/gnutls_privkey_import_pkcs11_url
FUNCS += functions/gnutls_privkey_import_pkcs11_url.short
FUNCS += functions/gnutls_privkey_import_rsa_raw
FUNCS += functions/gnutls_privkey_import_rsa_raw.short
FUNCS += functions/gnutls_privkey_import_tpm_raw
FUNCS += functions/gnutls_privkey_import_tpm_raw.short
FUNCS += functions/gnutls_privkey_import_tpm_url
FUNCS += functions/gnutls_privkey_import_tpm_url.short
FUNCS += functions/gnutls_privkey_import_url
FUNCS += functions/gnutls_privkey_import_url.short
FUNCS += functions/gnutls_privkey_import_x509
FUNCS += functions/gnutls_privkey_import_x509.short
FUNCS += functions/gnutls_privkey_import_x509_raw
FUNCS += functions/gnutls_privkey_import_x509_raw.short
FUNCS += functions/gnutls_privkey_init
FUNCS += functions/gnutls_privkey_init.short
FUNCS += functions/gnutls_privkey_set_flags
FUNCS += functions/gnutls_privkey_set_flags.short
FUNCS += functions/gnutls_privkey_set_pin_function
FUNCS += functions/gnutls_privkey_set_pin_function.short
FUNCS += functions/gnutls_privkey_sign_data
FUNCS += functions/gnutls_privkey_sign_data.short
FUNCS += functions/gnutls_privkey_sign_hash
FUNCS += functions/gnutls_privkey_sign_hash.short
FUNCS += functions/gnutls_privkey_status
FUNCS += functions/gnutls_privkey_status.short
FUNCS += functions/gnutls_privkey_verify_params
FUNCS += functions/gnutls_privkey_verify_params.short
FUNCS += functions/gnutls_privkey_verify_seed
FUNCS += functions/gnutls_privkey_verify_seed.short
FUNCS += functions/gnutls_protocol_get_id
FUNCS += functions/gnutls_protocol_get_id.short
FUNCS += functions/gnutls_protocol_get_name
FUNCS += functions/gnutls_protocol_get_name.short
FUNCS += functions/gnutls_protocol_get_version
FUNCS += functions/gnutls_protocol_get_version.short
FUNCS += functions/gnutls_protocol_list
FUNCS += functions/gnutls_protocol_list.short
FUNCS += functions/gnutls_psk_allocate_client_credentials
FUNCS += functions/gnutls_psk_allocate_client_credentials.short
FUNCS += functions/gnutls_psk_allocate_server_credentials
FUNCS += functions/gnutls_psk_allocate_server_credentials.short
FUNCS += functions/gnutls_psk_client_get_hint
FUNCS += functions/gnutls_psk_client_get_hint.short
FUNCS += functions/gnutls_psk_free_client_credentials
FUNCS += functions/gnutls_psk_free_client_credentials.short
FUNCS += functions/gnutls_psk_free_server_credentials
FUNCS += functions/gnutls_psk_free_server_credentials.short
FUNCS += functions/gnutls_psk_server_get_username
FUNCS += functions/gnutls_psk_server_get_username.short
FUNCS += functions/gnutls_psk_set_client_credentials
FUNCS += functions/gnutls_psk_set_client_credentials.short
FUNCS += functions/gnutls_psk_set_client_credentials_function
FUNCS += functions/gnutls_psk_set_client_credentials_function.short
FUNCS += functions/gnutls_psk_set_params_function
FUNCS += functions/gnutls_psk_set_params_function.short
FUNCS += functions/gnutls_psk_set_server_credentials_file
FUNCS += functions/gnutls_psk_set_server_credentials_file.short
FUNCS += functions/gnutls_psk_set_server_credentials_function
FUNCS += functions/gnutls_psk_set_server_credentials_function.short
FUNCS += functions/gnutls_psk_set_server_credentials_hint
FUNCS += functions/gnutls_psk_set_server_credentials_hint.short
FUNCS += functions/gnutls_psk_set_server_dh_params
FUNCS += functions/gnutls_psk_set_server_dh_params.short
FUNCS += functions/gnutls_psk_set_server_known_dh_params
FUNCS += functions/gnutls_psk_set_server_known_dh_params.short
FUNCS += functions/gnutls_psk_set_server_params_function
FUNCS += functions/gnutls_psk_set_server_params_function.short
FUNCS += functions/gnutls_pubkey_deinit
FUNCS += functions/gnutls_pubkey_deinit.short
FUNCS += functions/gnutls_pubkey_encrypt_data
FUNCS += functions/gnutls_pubkey_encrypt_data.short
FUNCS += functions/gnutls_pubkey_export
FUNCS += functions/gnutls_pubkey_export.short
FUNCS += functions/gnutls_pubkey_export2
FUNCS += functions/gnutls_pubkey_export2.short
FUNCS += functions/gnutls_pubkey_export_dsa_raw
FUNCS += functions/gnutls_pubkey_export_dsa_raw.short
FUNCS += functions/gnutls_pubkey_export_ecc_raw
FUNCS += functions/gnutls_pubkey_export_ecc_raw.short
FUNCS += functions/gnutls_pubkey_export_ecc_x962
FUNCS += functions/gnutls_pubkey_export_ecc_x962.short
FUNCS += functions/gnutls_pubkey_export_rsa_raw
FUNCS += functions/gnutls_pubkey_export_rsa_raw.short
FUNCS += functions/gnutls_pubkey_get_key_id
FUNCS += functions/gnutls_pubkey_get_key_id.short
FUNCS += functions/gnutls_pubkey_get_key_usage
FUNCS += functions/gnutls_pubkey_get_key_usage.short
FUNCS += functions/gnutls_pubkey_get_openpgp_key_id
FUNCS += functions/gnutls_pubkey_get_openpgp_key_id.short
FUNCS += functions/gnutls_pubkey_get_pk_algorithm
FUNCS += functions/gnutls_pubkey_get_pk_algorithm.short
FUNCS += functions/gnutls_pubkey_get_preferred_hash_algorithm
FUNCS += functions/gnutls_pubkey_get_preferred_hash_algorithm.short
FUNCS += functions/gnutls_pubkey_import
FUNCS += functions/gnutls_pubkey_import.short
FUNCS += functions/gnutls_pubkey_import_dsa_raw
FUNCS += functions/gnutls_pubkey_import_dsa_raw.short
FUNCS += functions/gnutls_pubkey_import_ecc_raw
FUNCS += functions/gnutls_pubkey_import_ecc_raw.short
FUNCS += functions/gnutls_pubkey_import_ecc_x962
FUNCS += functions/gnutls_pubkey_import_ecc_x962.short
FUNCS += functions/gnutls_pubkey_import_openpgp
FUNCS += functions/gnutls_pubkey_import_openpgp.short
FUNCS += functions/gnutls_pubkey_import_openpgp_raw
FUNCS += functions/gnutls_pubkey_import_openpgp_raw.short
FUNCS += functions/gnutls_pubkey_import_pkcs11
FUNCS += functions/gnutls_pubkey_import_pkcs11.short
FUNCS += functions/gnutls_pubkey_import_privkey
FUNCS += functions/gnutls_pubkey_import_privkey.short
FUNCS += functions/gnutls_pubkey_import_rsa_raw
FUNCS += functions/gnutls_pubkey_import_rsa_raw.short
FUNCS += functions/gnutls_pubkey_import_tpm_raw
FUNCS += functions/gnutls_pubkey_import_tpm_raw.short
FUNCS += functions/gnutls_pubkey_import_tpm_url
FUNCS += functions/gnutls_pubkey_import_tpm_url.short
FUNCS += functions/gnutls_pubkey_import_url
FUNCS += functions/gnutls_pubkey_import_url.short
FUNCS += functions/gnutls_pubkey_import_x509
FUNCS += functions/gnutls_pubkey_import_x509.short
FUNCS += functions/gnutls_pubkey_import_x509_crq
FUNCS += functions/gnutls_pubkey_import_x509_crq.short
FUNCS += functions/gnutls_pubkey_import_x509_raw
FUNCS += functions/gnutls_pubkey_import_x509_raw.short
FUNCS += functions/gnutls_pubkey_init
FUNCS += functions/gnutls_pubkey_init.short
FUNCS += functions/gnutls_pubkey_print
FUNCS += functions/gnutls_pubkey_print.short
FUNCS += functions/gnutls_pubkey_set_key_usage
FUNCS += functions/gnutls_pubkey_set_key_usage.short
FUNCS += functions/gnutls_pubkey_set_pin_function
FUNCS += functions/gnutls_pubkey_set_pin_function.short
FUNCS += functions/gnutls_pubkey_verify_data2
FUNCS += functions/gnutls_pubkey_verify_data2.short
FUNCS += functions/gnutls_pubkey_verify_hash2
FUNCS += functions/gnutls_pubkey_verify_hash2.short
FUNCS += functions/gnutls_pubkey_verify_params
FUNCS += functions/gnutls_pubkey_verify_params.short
FUNCS += functions/gnutls_random_art
FUNCS += functions/gnutls_random_art.short
FUNCS += functions/gnutls_range_split
FUNCS += functions/gnutls_range_split.short
FUNCS += functions/gnutls_record_can_use_length_hiding
FUNCS += functions/gnutls_record_can_use_length_hiding.short
FUNCS += functions/gnutls_record_check_corked
FUNCS += functions/gnutls_record_check_corked.short
FUNCS += functions/gnutls_record_check_pending
FUNCS += functions/gnutls_record_check_pending.short
FUNCS += functions/gnutls_record_cork
FUNCS += functions/gnutls_record_cork.short
FUNCS += functions/gnutls_record_disable_padding
FUNCS += functions/gnutls_record_disable_padding.short
FUNCS += functions/gnutls_record_discard_queued
FUNCS += functions/gnutls_record_discard_queued.short
FUNCS += functions/gnutls_record_get_direction
FUNCS += functions/gnutls_record_get_direction.short
FUNCS += functions/gnutls_record_get_discarded
FUNCS += functions/gnutls_record_get_discarded.short
FUNCS += functions/gnutls_record_get_max_size
FUNCS += functions/gnutls_record_get_max_size.short
FUNCS += functions/gnutls_record_get_state
FUNCS += functions/gnutls_record_get_state.short
FUNCS += functions/gnutls_record_overhead_size
FUNCS += functions/gnutls_record_overhead_size.short
FUNCS += functions/gnutls_record_recv
FUNCS += functions/gnutls_record_recv.short
FUNCS += functions/gnutls_record_recv_packet
FUNCS += functions/gnutls_record_recv_packet.short
FUNCS += functions/gnutls_record_recv_seq
FUNCS += functions/gnutls_record_recv_seq.short
FUNCS += functions/gnutls_record_send
FUNCS += functions/gnutls_record_send.short
FUNCS += functions/gnutls_record_send_range
FUNCS += functions/gnutls_record_send_range.short
FUNCS += functions/gnutls_record_set_max_size
FUNCS += functions/gnutls_record_set_max_size.short
FUNCS += functions/gnutls_record_set_state
FUNCS += functions/gnutls_record_set_state.short
FUNCS += functions/gnutls_record_set_timeout
FUNCS += functions/gnutls_record_set_timeout.short
FUNCS += functions/gnutls_record_uncork
FUNCS += functions/gnutls_record_uncork.short
FUNCS += functions/gnutls_register_custom_url
FUNCS += functions/gnutls_register_custom_url.short
FUNCS += functions/gnutls_rehandshake
FUNCS += functions/gnutls_rehandshake.short
FUNCS += functions/gnutls_rnd
FUNCS += functions/gnutls_rnd.short
FUNCS += functions/gnutls_rnd_refresh
FUNCS += functions/gnutls_rnd_refresh.short
FUNCS += functions/gnutls_safe_renegotiation_status
FUNCS += functions/gnutls_safe_renegotiation_status.short
FUNCS += functions/gnutls_sec_param_get_name
FUNCS += functions/gnutls_sec_param_get_name.short
FUNCS += functions/gnutls_sec_param_to_pk_bits
FUNCS += functions/gnutls_sec_param_to_pk_bits.short
FUNCS += functions/gnutls_sec_param_to_symmetric_bits
FUNCS += functions/gnutls_sec_param_to_symmetric_bits.short
FUNCS += functions/gnutls_server_name_get
FUNCS += functions/gnutls_server_name_get.short
FUNCS += functions/gnutls_server_name_set
FUNCS += functions/gnutls_server_name_set.short
FUNCS += functions/gnutls_session_channel_binding
FUNCS += functions/gnutls_session_channel_binding.short
FUNCS += functions/gnutls_session_enable_compatibility_mode
FUNCS += functions/gnutls_session_enable_compatibility_mode.short
FUNCS += functions/gnutls_session_etm_status
FUNCS += functions/gnutls_session_etm_status.short
FUNCS += functions/gnutls_session_ext_master_secret_status
FUNCS += functions/gnutls_session_ext_master_secret_status.short
FUNCS += functions/gnutls_session_ext_register
FUNCS += functions/gnutls_session_ext_register.short
FUNCS += functions/gnutls_session_force_valid
FUNCS += functions/gnutls_session_force_valid.short
FUNCS += functions/gnutls_session_get_data
FUNCS += functions/gnutls_session_get_data.short
FUNCS += functions/gnutls_session_get_data2
FUNCS += functions/gnutls_session_get_data2.short
FUNCS += functions/gnutls_session_get_desc
FUNCS += functions/gnutls_session_get_desc.short
FUNCS += functions/gnutls_session_get_flags
FUNCS += functions/gnutls_session_get_flags.short
FUNCS += functions/gnutls_session_get_id
FUNCS += functions/gnutls_session_get_id.short
FUNCS += functions/gnutls_session_get_id2
FUNCS += functions/gnutls_session_get_id2.short
FUNCS += functions/gnutls_session_get_master_secret
FUNCS += functions/gnutls_session_get_master_secret.short
FUNCS += functions/gnutls_session_get_ptr
FUNCS += functions/gnutls_session_get_ptr.short
FUNCS += functions/gnutls_session_get_random
FUNCS += functions/gnutls_session_get_random.short
FUNCS += functions/gnutls_session_get_verify_cert_status
FUNCS += functions/gnutls_session_get_verify_cert_status.short
FUNCS += functions/gnutls_session_is_resumed
FUNCS += functions/gnutls_session_is_resumed.short
FUNCS += functions/gnutls_session_resumption_requested
FUNCS += functions/gnutls_session_resumption_requested.short
FUNCS += functions/gnutls_session_set_data
FUNCS += functions/gnutls_session_set_data.short
FUNCS += functions/gnutls_session_set_id
FUNCS += functions/gnutls_session_set_id.short
FUNCS += functions/gnutls_session_set_premaster
FUNCS += functions/gnutls_session_set_premaster.short
FUNCS += functions/gnutls_session_set_ptr
FUNCS += functions/gnutls_session_set_ptr.short
FUNCS += functions/gnutls_session_set_verify_cert
FUNCS += functions/gnutls_session_set_verify_cert.short
FUNCS += functions/gnutls_session_set_verify_cert2
FUNCS += functions/gnutls_session_set_verify_cert2.short
FUNCS += functions/gnutls_session_set_verify_function
FUNCS += functions/gnutls_session_set_verify_function.short
FUNCS += functions/gnutls_session_supplemental_register
FUNCS += functions/gnutls_session_supplemental_register.short
FUNCS += functions/gnutls_session_ticket_enable_client
FUNCS += functions/gnutls_session_ticket_enable_client.short
FUNCS += functions/gnutls_session_ticket_enable_server
FUNCS += functions/gnutls_session_ticket_enable_server.short
FUNCS += functions/gnutls_session_ticket_key_generate
FUNCS += functions/gnutls_session_ticket_key_generate.short
FUNCS += functions/gnutls_set_default_priority
FUNCS += functions/gnutls_set_default_priority.short
FUNCS += functions/gnutls_sign_algorithm_get
FUNCS += functions/gnutls_sign_algorithm_get.short
FUNCS += functions/gnutls_sign_algorithm_get_client
FUNCS += functions/gnutls_sign_algorithm_get_client.short
FUNCS += functions/gnutls_sign_algorithm_get_requested
FUNCS += functions/gnutls_sign_algorithm_get_requested.short
FUNCS += functions/gnutls_sign_get_hash_algorithm
FUNCS += functions/gnutls_sign_get_hash_algorithm.short
FUNCS += functions/gnutls_sign_get_id
FUNCS += functions/gnutls_sign_get_id.short
FUNCS += functions/gnutls_sign_get_name
FUNCS += functions/gnutls_sign_get_name.short
FUNCS += functions/gnutls_sign_get_oid
FUNCS += functions/gnutls_sign_get_oid.short
FUNCS += functions/gnutls_sign_get_pk_algorithm
FUNCS += functions/gnutls_sign_get_pk_algorithm.short
FUNCS += functions/gnutls_sign_is_secure
FUNCS += functions/gnutls_sign_is_secure.short
FUNCS += functions/gnutls_sign_list
FUNCS += functions/gnutls_sign_list.short
FUNCS += functions/gnutls_srp_allocate_client_credentials
FUNCS += functions/gnutls_srp_allocate_client_credentials.short
FUNCS += functions/gnutls_srp_allocate_server_credentials
FUNCS += functions/gnutls_srp_allocate_server_credentials.short
FUNCS += functions/gnutls_srp_base64_decode
FUNCS += functions/gnutls_srp_base64_decode.short
FUNCS += functions/gnutls_srp_base64_decode2
FUNCS += functions/gnutls_srp_base64_decode2.short
FUNCS += functions/gnutls_srp_base64_encode
FUNCS += functions/gnutls_srp_base64_encode.short
FUNCS += functions/gnutls_srp_base64_encode2
FUNCS += functions/gnutls_srp_base64_encode2.short
FUNCS += functions/gnutls_srp_free_client_credentials
FUNCS += functions/gnutls_srp_free_client_credentials.short
FUNCS += functions/gnutls_srp_free_server_credentials
FUNCS += functions/gnutls_srp_free_server_credentials.short
FUNCS += functions/gnutls_srp_server_get_username
FUNCS += functions/gnutls_srp_server_get_username.short
FUNCS += functions/gnutls_srp_set_client_credentials
FUNCS += functions/gnutls_srp_set_client_credentials.short
FUNCS += functions/gnutls_srp_set_client_credentials_function
FUNCS += functions/gnutls_srp_set_client_credentials_function.short
FUNCS += functions/gnutls_srp_set_prime_bits
FUNCS += functions/gnutls_srp_set_prime_bits.short
FUNCS += functions/gnutls_srp_set_server_credentials_file
FUNCS += functions/gnutls_srp_set_server_credentials_file.short
FUNCS += functions/gnutls_srp_set_server_credentials_function
FUNCS += functions/gnutls_srp_set_server_credentials_function.short
FUNCS += functions/gnutls_srp_set_server_fake_salt_seed
FUNCS += functions/gnutls_srp_set_server_fake_salt_seed.short
FUNCS += functions/gnutls_srp_verifier
FUNCS += functions/gnutls_srp_verifier.short
FUNCS += functions/gnutls_srtp_get_keys
FUNCS += functions/gnutls_srtp_get_keys.short
FUNCS += functions/gnutls_srtp_get_mki
FUNCS += functions/gnutls_srtp_get_mki.short
FUNCS += functions/gnutls_srtp_get_profile_id
FUNCS += functions/gnutls_srtp_get_profile_id.short
FUNCS += functions/gnutls_srtp_get_profile_name
FUNCS += functions/gnutls_srtp_get_profile_name.short
FUNCS += functions/gnutls_srtp_get_selected_profile
FUNCS += functions/gnutls_srtp_get_selected_profile.short
FUNCS += functions/gnutls_srtp_set_mki
FUNCS += functions/gnutls_srtp_set_mki.short
FUNCS += functions/gnutls_srtp_set_profile
FUNCS += functions/gnutls_srtp_set_profile.short
FUNCS += functions/gnutls_srtp_set_profile_direct
FUNCS += functions/gnutls_srtp_set_profile_direct.short
FUNCS += functions/gnutls_store_commitment
FUNCS += functions/gnutls_store_commitment.short
FUNCS += functions/gnutls_store_pubkey
FUNCS += functions/gnutls_store_pubkey.short
FUNCS += functions/gnutls_strerror
FUNCS += functions/gnutls_strerror.short
FUNCS += functions/gnutls_strerror_name
FUNCS += functions/gnutls_strerror_name.short
FUNCS += functions/gnutls_subject_alt_names_deinit
FUNCS += functions/gnutls_subject_alt_names_deinit.short
FUNCS += functions/gnutls_subject_alt_names_get
FUNCS += functions/gnutls_subject_alt_names_get.short
FUNCS += functions/gnutls_subject_alt_names_init
FUNCS += functions/gnutls_subject_alt_names_init.short
FUNCS += functions/gnutls_subject_alt_names_set
FUNCS += functions/gnutls_subject_alt_names_set.short
FUNCS += functions/gnutls_supplemental_get_name
FUNCS += functions/gnutls_supplemental_get_name.short
FUNCS += functions/gnutls_supplemental_recv
FUNCS += functions/gnutls_supplemental_recv.short
FUNCS += functions/gnutls_supplemental_register
FUNCS += functions/gnutls_supplemental_register.short
FUNCS += functions/gnutls_supplemental_send
FUNCS += functions/gnutls_supplemental_send.short
FUNCS += functions/gnutls_system_key_add_x509
FUNCS += functions/gnutls_system_key_add_x509.short
FUNCS += functions/gnutls_system_key_delete
FUNCS += functions/gnutls_system_key_delete.short
FUNCS += functions/gnutls_system_key_iter_deinit
FUNCS += functions/gnutls_system_key_iter_deinit.short
FUNCS += functions/gnutls_system_key_iter_get_info
FUNCS += functions/gnutls_system_key_iter_get_info.short
FUNCS += functions/gnutls_system_recv_timeout
FUNCS += functions/gnutls_system_recv_timeout.short
FUNCS += functions/gnutls_tdb_deinit
FUNCS += functions/gnutls_tdb_deinit.short
FUNCS += functions/gnutls_tdb_init
FUNCS += functions/gnutls_tdb_init.short
FUNCS += functions/gnutls_tdb_set_store_commitment_func
FUNCS += functions/gnutls_tdb_set_store_commitment_func.short
FUNCS += functions/gnutls_tdb_set_store_func
FUNCS += functions/gnutls_tdb_set_store_func.short
FUNCS += functions/gnutls_tdb_set_verify_func
FUNCS += functions/gnutls_tdb_set_verify_func.short
FUNCS += functions/gnutls_tpm_get_registered
FUNCS += functions/gnutls_tpm_get_registered.short
FUNCS += functions/gnutls_tpm_key_list_deinit
FUNCS += functions/gnutls_tpm_key_list_deinit.short
FUNCS += functions/gnutls_tpm_key_list_get_url
FUNCS += functions/gnutls_tpm_key_list_get_url.short
FUNCS += functions/gnutls_tpm_privkey_delete
FUNCS += functions/gnutls_tpm_privkey_delete.short
FUNCS += functions/gnutls_tpm_privkey_generate
FUNCS += functions/gnutls_tpm_privkey_generate.short
FUNCS += functions/gnutls_transport_get_int
FUNCS += functions/gnutls_transport_get_int.short
FUNCS += functions/gnutls_transport_get_int2
FUNCS += functions/gnutls_transport_get_int2.short
FUNCS += functions/gnutls_transport_get_ptr
FUNCS += functions/gnutls_transport_get_ptr.short
FUNCS += functions/gnutls_transport_get_ptr2
FUNCS += functions/gnutls_transport_get_ptr2.short
FUNCS += functions/gnutls_transport_set_errno
FUNCS += functions/gnutls_transport_set_errno.short
FUNCS += functions/gnutls_transport_set_errno_function
FUNCS += functions/gnutls_transport_set_errno_function.short
FUNCS += functions/gnutls_transport_set_fastopen
FUNCS += functions/gnutls_transport_set_fastopen.short
FUNCS += functions/gnutls_transport_set_int
FUNCS += functions/gnutls_transport_set_int.short
FUNCS += functions/gnutls_transport_set_int2
FUNCS += functions/gnutls_transport_set_int2.short
FUNCS += functions/gnutls_transport_set_ptr
FUNCS += functions/gnutls_transport_set_ptr.short
FUNCS += functions/gnutls_transport_set_ptr2
FUNCS += functions/gnutls_transport_set_ptr2.short
FUNCS += functions/gnutls_transport_set_pull_function
FUNCS += functions/gnutls_transport_set_pull_function.short
FUNCS += functions/gnutls_transport_set_pull_timeout_function
FUNCS += functions/gnutls_transport_set_pull_timeout_function.short
FUNCS += functions/gnutls_transport_set_push_function
FUNCS += functions/gnutls_transport_set_push_function.short
FUNCS += functions/gnutls_transport_set_vec_push_function
FUNCS += functions/gnutls_transport_set_vec_push_function.short
FUNCS += functions/gnutls_url_is_supported
FUNCS += functions/gnutls_url_is_supported.short
FUNCS += functions/gnutls_utf8_password_normalize
FUNCS += functions/gnutls_utf8_password_normalize.short
FUNCS += functions/gnutls_verify_stored_pubkey
FUNCS += functions/gnutls_verify_stored_pubkey.short
FUNCS += functions/gnutls_x509_aia_deinit
FUNCS += functions/gnutls_x509_aia_deinit.short
FUNCS += functions/gnutls_x509_aia_get
FUNCS += functions/gnutls_x509_aia_get.short
FUNCS += functions/gnutls_x509_aia_init
FUNCS += functions/gnutls_x509_aia_init.short
FUNCS += functions/gnutls_x509_aia_set
FUNCS += functions/gnutls_x509_aia_set.short
FUNCS += functions/gnutls_x509_aki_deinit
FUNCS += functions/gnutls_x509_aki_deinit.short
FUNCS += functions/gnutls_x509_aki_get_cert_issuer
FUNCS += functions/gnutls_x509_aki_get_cert_issuer.short
FUNCS += functions/gnutls_x509_aki_get_id
FUNCS += functions/gnutls_x509_aki_get_id.short
FUNCS += functions/gnutls_x509_aki_init
FUNCS += functions/gnutls_x509_aki_init.short
FUNCS += functions/gnutls_x509_aki_set_cert_issuer
FUNCS += functions/gnutls_x509_aki_set_cert_issuer.short
FUNCS += functions/gnutls_x509_aki_set_id
FUNCS += functions/gnutls_x509_aki_set_id.short
FUNCS += functions/gnutls_x509_cidr_to_rfc5280
FUNCS += functions/gnutls_x509_cidr_to_rfc5280.short
FUNCS += functions/gnutls_x509_crl_check_issuer
FUNCS += functions/gnutls_x509_crl_check_issuer.short
FUNCS += functions/gnutls_x509_crl_deinit
FUNCS += functions/gnutls_x509_crl_deinit.short
FUNCS += functions/gnutls_x509_crl_dist_points_deinit
FUNCS += functions/gnutls_x509_crl_dist_points_deinit.short
FUNCS += functions/gnutls_x509_crl_dist_points_get
FUNCS += functions/gnutls_x509_crl_dist_points_get.short
FUNCS += functions/gnutls_x509_crl_dist_points_init
FUNCS += functions/gnutls_x509_crl_dist_points_init.short
FUNCS += functions/gnutls_x509_crl_dist_points_set
FUNCS += functions/gnutls_x509_crl_dist_points_set.short
FUNCS += functions/gnutls_x509_crl_export
FUNCS += functions/gnutls_x509_crl_export.short
FUNCS += functions/gnutls_x509_crl_export2
FUNCS += functions/gnutls_x509_crl_export2.short
FUNCS += functions/gnutls_x509_crl_get_authority_key_gn_serial
FUNCS += functions/gnutls_x509_crl_get_authority_key_gn_serial.short
FUNCS += functions/gnutls_x509_crl_get_authority_key_id
FUNCS += functions/gnutls_x509_crl_get_authority_key_id.short
FUNCS += functions/gnutls_x509_crl_get_crt_count
FUNCS += functions/gnutls_x509_crl_get_crt_count.short
FUNCS += functions/gnutls_x509_crl_get_crt_serial
FUNCS += functions/gnutls_x509_crl_get_crt_serial.short
FUNCS += functions/gnutls_x509_crl_get_dn_oid
FUNCS += functions/gnutls_x509_crl_get_dn_oid.short
FUNCS += functions/gnutls_x509_crl_get_extension_data
FUNCS += functions/gnutls_x509_crl_get_extension_data.short
FUNCS += functions/gnutls_x509_crl_get_extension_data2
FUNCS += functions/gnutls_x509_crl_get_extension_data2.short
FUNCS += functions/gnutls_x509_crl_get_extension_info
FUNCS += functions/gnutls_x509_crl_get_extension_info.short
FUNCS += functions/gnutls_x509_crl_get_extension_oid
FUNCS += functions/gnutls_x509_crl_get_extension_oid.short
FUNCS += functions/gnutls_x509_crl_get_issuer_dn
FUNCS += functions/gnutls_x509_crl_get_issuer_dn.short
FUNCS += functions/gnutls_x509_crl_get_issuer_dn2
FUNCS += functions/gnutls_x509_crl_get_issuer_dn2.short
FUNCS += functions/gnutls_x509_crl_get_issuer_dn3
FUNCS += functions/gnutls_x509_crl_get_issuer_dn3.short
FUNCS += functions/gnutls_x509_crl_get_issuer_dn_by_oid
FUNCS += functions/gnutls_x509_crl_get_issuer_dn_by_oid.short
FUNCS += functions/gnutls_x509_crl_get_next_update
FUNCS += functions/gnutls_x509_crl_get_next_update.short
FUNCS += functions/gnutls_x509_crl_get_number
FUNCS += functions/gnutls_x509_crl_get_number.short
FUNCS += functions/gnutls_x509_crl_get_raw_issuer_dn
FUNCS += functions/gnutls_x509_crl_get_raw_issuer_dn.short
FUNCS += functions/gnutls_x509_crl_get_signature
FUNCS += functions/gnutls_x509_crl_get_signature.short
FUNCS += functions/gnutls_x509_crl_get_signature_algorithm
FUNCS += functions/gnutls_x509_crl_get_signature_algorithm.short
FUNCS += functions/gnutls_x509_crl_get_signature_oid
FUNCS += functions/gnutls_x509_crl_get_signature_oid.short
FUNCS += functions/gnutls_x509_crl_get_this_update
FUNCS += functions/gnutls_x509_crl_get_this_update.short
FUNCS += functions/gnutls_x509_crl_get_version
FUNCS += functions/gnutls_x509_crl_get_version.short
FUNCS += functions/gnutls_x509_crl_import
FUNCS += functions/gnutls_x509_crl_import.short
FUNCS += functions/gnutls_x509_crl_init
FUNCS += functions/gnutls_x509_crl_init.short
FUNCS += functions/gnutls_x509_crl_iter_crt_serial
FUNCS += functions/gnutls_x509_crl_iter_crt_serial.short
FUNCS += functions/gnutls_x509_crl_iter_deinit
FUNCS += functions/gnutls_x509_crl_iter_deinit.short
FUNCS += functions/gnutls_x509_crl_list_import
FUNCS += functions/gnutls_x509_crl_list_import.short
FUNCS += functions/gnutls_x509_crl_list_import2
FUNCS += functions/gnutls_x509_crl_list_import2.short
FUNCS += functions/gnutls_x509_crl_print
FUNCS += functions/gnutls_x509_crl_print.short
FUNCS += functions/gnutls_x509_crl_privkey_sign
FUNCS += functions/gnutls_x509_crl_privkey_sign.short
FUNCS += functions/gnutls_x509_crl_set_authority_key_id
FUNCS += functions/gnutls_x509_crl_set_authority_key_id.short
FUNCS += functions/gnutls_x509_crl_set_crt
FUNCS += functions/gnutls_x509_crl_set_crt.short
FUNCS += functions/gnutls_x509_crl_set_crt_serial
FUNCS += functions/gnutls_x509_crl_set_crt_serial.short
FUNCS += functions/gnutls_x509_crl_set_next_update
FUNCS += functions/gnutls_x509_crl_set_next_update.short
FUNCS += functions/gnutls_x509_crl_set_number
FUNCS += functions/gnutls_x509_crl_set_number.short
FUNCS += functions/gnutls_x509_crl_set_this_update
FUNCS += functions/gnutls_x509_crl_set_this_update.short
FUNCS += functions/gnutls_x509_crl_set_version
FUNCS += functions/gnutls_x509_crl_set_version.short
FUNCS += functions/gnutls_x509_crl_sign
FUNCS += functions/gnutls_x509_crl_sign.short
FUNCS += functions/gnutls_x509_crl_sign2
FUNCS += functions/gnutls_x509_crl_sign2.short
FUNCS += functions/gnutls_x509_crl_verify
FUNCS += functions/gnutls_x509_crl_verify.short
FUNCS += functions/gnutls_x509_crq_deinit
FUNCS += functions/gnutls_x509_crq_deinit.short
FUNCS += functions/gnutls_x509_crq_export
FUNCS += functions/gnutls_x509_crq_export.short
FUNCS += functions/gnutls_x509_crq_export2
FUNCS += functions/gnutls_x509_crq_export2.short
FUNCS += functions/gnutls_x509_crq_get_attribute_by_oid
FUNCS += functions/gnutls_x509_crq_get_attribute_by_oid.short
FUNCS += functions/gnutls_x509_crq_get_attribute_data
FUNCS += functions/gnutls_x509_crq_get_attribute_data.short
FUNCS += functions/gnutls_x509_crq_get_attribute_info
FUNCS += functions/gnutls_x509_crq_get_attribute_info.short
FUNCS += functions/gnutls_x509_crq_get_basic_constraints
FUNCS += functions/gnutls_x509_crq_get_basic_constraints.short
FUNCS += functions/gnutls_x509_crq_get_challenge_password
FUNCS += functions/gnutls_x509_crq_get_challenge_password.short
FUNCS += functions/gnutls_x509_crq_get_dn
FUNCS += functions/gnutls_x509_crq_get_dn.short
FUNCS += functions/gnutls_x509_crq_get_dn2
FUNCS += functions/gnutls_x509_crq_get_dn2.short
FUNCS += functions/gnutls_x509_crq_get_dn3
FUNCS += functions/gnutls_x509_crq_get_dn3.short
FUNCS += functions/gnutls_x509_crq_get_dn_by_oid
FUNCS += functions/gnutls_x509_crq_get_dn_by_oid.short
FUNCS += functions/gnutls_x509_crq_get_dn_oid
FUNCS += functions/gnutls_x509_crq_get_dn_oid.short
FUNCS += functions/gnutls_x509_crq_get_extension_by_oid
FUNCS += functions/gnutls_x509_crq_get_extension_by_oid.short
FUNCS += functions/gnutls_x509_crq_get_extension_by_oid2
FUNCS += functions/gnutls_x509_crq_get_extension_by_oid2.short
FUNCS += functions/gnutls_x509_crq_get_extension_data
FUNCS += functions/gnutls_x509_crq_get_extension_data.short
FUNCS += functions/gnutls_x509_crq_get_extension_data2
FUNCS += functions/gnutls_x509_crq_get_extension_data2.short
FUNCS += functions/gnutls_x509_crq_get_extension_info
FUNCS += functions/gnutls_x509_crq_get_extension_info.short
FUNCS += functions/gnutls_x509_crq_get_key_id
FUNCS += functions/gnutls_x509_crq_get_key_id.short
FUNCS += functions/gnutls_x509_crq_get_key_purpose_oid
FUNCS += functions/gnutls_x509_crq_get_key_purpose_oid.short
FUNCS += functions/gnutls_x509_crq_get_key_rsa_raw
FUNCS += functions/gnutls_x509_crq_get_key_rsa_raw.short
FUNCS += functions/gnutls_x509_crq_get_key_usage
FUNCS += functions/gnutls_x509_crq_get_key_usage.short
FUNCS += functions/gnutls_x509_crq_get_pk_algorithm
FUNCS += functions/gnutls_x509_crq_get_pk_algorithm.short
FUNCS += functions/gnutls_x509_crq_get_pk_oid
FUNCS += functions/gnutls_x509_crq_get_pk_oid.short
FUNCS += functions/gnutls_x509_crq_get_private_key_usage_period
FUNCS += functions/gnutls_x509_crq_get_private_key_usage_period.short
FUNCS += functions/gnutls_x509_crq_get_signature_algorithm
FUNCS += functions/gnutls_x509_crq_get_signature_algorithm.short
FUNCS += functions/gnutls_x509_crq_get_signature_oid
FUNCS += functions/gnutls_x509_crq_get_signature_oid.short
FUNCS += functions/gnutls_x509_crq_get_subject_alt_name
FUNCS += functions/gnutls_x509_crq_get_subject_alt_name.short
FUNCS += functions/gnutls_x509_crq_get_subject_alt_othername_oid
FUNCS += functions/gnutls_x509_crq_get_subject_alt_othername_oid.short
FUNCS += functions/gnutls_x509_crq_get_tlsfeatures
FUNCS += functions/gnutls_x509_crq_get_tlsfeatures.short
FUNCS += functions/gnutls_x509_crq_get_version
FUNCS += functions/gnutls_x509_crq_get_version.short
FUNCS += functions/gnutls_x509_crq_import
FUNCS += functions/gnutls_x509_crq_import.short
FUNCS += functions/gnutls_x509_crq_init
FUNCS += functions/gnutls_x509_crq_init.short
FUNCS += functions/gnutls_x509_crq_print
FUNCS += functions/gnutls_x509_crq_print.short
FUNCS += functions/gnutls_x509_crq_privkey_sign
FUNCS += functions/gnutls_x509_crq_privkey_sign.short
FUNCS += functions/gnutls_x509_crq_set_attribute_by_oid
FUNCS += functions/gnutls_x509_crq_set_attribute_by_oid.short
FUNCS += functions/gnutls_x509_crq_set_basic_constraints
FUNCS += functions/gnutls_x509_crq_set_basic_constraints.short
FUNCS += functions/gnutls_x509_crq_set_challenge_password
FUNCS += functions/gnutls_x509_crq_set_challenge_password.short
FUNCS += functions/gnutls_x509_crq_set_dn
FUNCS += functions/gnutls_x509_crq_set_dn.short
FUNCS += functions/gnutls_x509_crq_set_dn_by_oid
FUNCS += functions/gnutls_x509_crq_set_dn_by_oid.short
FUNCS += functions/gnutls_x509_crq_set_extension_by_oid
FUNCS += functions/gnutls_x509_crq_set_extension_by_oid.short
FUNCS += functions/gnutls_x509_crq_set_key
FUNCS += functions/gnutls_x509_crq_set_key.short
FUNCS += functions/gnutls_x509_crq_set_key_purpose_oid
FUNCS += functions/gnutls_x509_crq_set_key_purpose_oid.short
FUNCS += functions/gnutls_x509_crq_set_key_rsa_raw
FUNCS += functions/gnutls_x509_crq_set_key_rsa_raw.short
FUNCS += functions/gnutls_x509_crq_set_key_usage
FUNCS += functions/gnutls_x509_crq_set_key_usage.short
FUNCS += functions/gnutls_x509_crq_set_private_key_usage_period
FUNCS += functions/gnutls_x509_crq_set_private_key_usage_period.short
FUNCS += functions/gnutls_x509_crq_set_pubkey
FUNCS += functions/gnutls_x509_crq_set_pubkey.short
FUNCS += functions/gnutls_x509_crq_set_subject_alt_name
FUNCS += functions/gnutls_x509_crq_set_subject_alt_name.short
FUNCS += functions/gnutls_x509_crq_set_subject_alt_othername
FUNCS += functions/gnutls_x509_crq_set_subject_alt_othername.short
FUNCS += functions/gnutls_x509_crq_set_tlsfeatures
FUNCS += functions/gnutls_x509_crq_set_tlsfeatures.short
FUNCS += functions/gnutls_x509_crq_set_version
FUNCS += functions/gnutls_x509_crq_set_version.short
FUNCS += functions/gnutls_x509_crq_sign
FUNCS += functions/gnutls_x509_crq_sign.short
FUNCS += functions/gnutls_x509_crq_sign2
FUNCS += functions/gnutls_x509_crq_sign2.short
FUNCS += functions/gnutls_x509_crq_verify
FUNCS += functions/gnutls_x509_crq_verify.short
FUNCS += functions/gnutls_x509_crt_check_email
FUNCS += functions/gnutls_x509_crt_check_email.short
FUNCS += functions/gnutls_x509_crt_check_hostname
FUNCS += functions/gnutls_x509_crt_check_hostname.short
FUNCS += functions/gnutls_x509_crt_check_hostname2
FUNCS += functions/gnutls_x509_crt_check_hostname2.short
FUNCS += functions/gnutls_x509_crt_check_issuer
FUNCS += functions/gnutls_x509_crt_check_issuer.short
FUNCS += functions/gnutls_x509_crt_check_key_purpose
FUNCS += functions/gnutls_x509_crt_check_key_purpose.short
FUNCS += functions/gnutls_x509_crt_check_revocation
FUNCS += functions/gnutls_x509_crt_check_revocation.short
FUNCS += functions/gnutls_x509_crt_cpy_crl_dist_points
FUNCS += functions/gnutls_x509_crt_cpy_crl_dist_points.short
FUNCS += functions/gnutls_x509_crt_deinit
FUNCS += functions/gnutls_x509_crt_deinit.short
FUNCS += functions/gnutls_x509_crt_equals
FUNCS += functions/gnutls_x509_crt_equals.short
FUNCS += functions/gnutls_x509_crt_equals2
FUNCS += functions/gnutls_x509_crt_equals2.short
FUNCS += functions/gnutls_x509_crt_export
FUNCS += functions/gnutls_x509_crt_export.short
FUNCS += functions/gnutls_x509_crt_export2
FUNCS += functions/gnutls_x509_crt_export2.short
FUNCS += functions/gnutls_x509_crt_get_activation_time
FUNCS += functions/gnutls_x509_crt_get_activation_time.short
FUNCS += functions/gnutls_x509_crt_get_authority_info_access
FUNCS += functions/gnutls_x509_crt_get_authority_info_access.short
FUNCS += functions/gnutls_x509_crt_get_authority_key_gn_serial
FUNCS += functions/gnutls_x509_crt_get_authority_key_gn_serial.short
FUNCS += functions/gnutls_x509_crt_get_authority_key_id
FUNCS += functions/gnutls_x509_crt_get_authority_key_id.short
FUNCS += functions/gnutls_x509_crt_get_basic_constraints
FUNCS += functions/gnutls_x509_crt_get_basic_constraints.short
FUNCS += functions/gnutls_x509_crt_get_ca_status
FUNCS += functions/gnutls_x509_crt_get_ca_status.short
FUNCS += functions/gnutls_x509_crt_get_crl_dist_points
FUNCS += functions/gnutls_x509_crt_get_crl_dist_points.short
FUNCS += functions/gnutls_x509_crt_get_dn
FUNCS += functions/gnutls_x509_crt_get_dn.short
FUNCS += functions/gnutls_x509_crt_get_dn2
FUNCS += functions/gnutls_x509_crt_get_dn2.short
FUNCS += functions/gnutls_x509_crt_get_dn3
FUNCS += functions/gnutls_x509_crt_get_dn3.short
FUNCS += functions/gnutls_x509_crt_get_dn_by_oid
FUNCS += functions/gnutls_x509_crt_get_dn_by_oid.short
FUNCS += functions/gnutls_x509_crt_get_dn_oid
FUNCS += functions/gnutls_x509_crt_get_dn_oid.short
FUNCS += functions/gnutls_x509_crt_get_expiration_time
FUNCS += functions/gnutls_x509_crt_get_expiration_time.short
FUNCS += functions/gnutls_x509_crt_get_extension_by_oid
FUNCS += functions/gnutls_x509_crt_get_extension_by_oid.short
FUNCS += functions/gnutls_x509_crt_get_extension_by_oid2
FUNCS += functions/gnutls_x509_crt_get_extension_by_oid2.short
FUNCS += functions/gnutls_x509_crt_get_extension_data
FUNCS += functions/gnutls_x509_crt_get_extension_data.short
FUNCS += functions/gnutls_x509_crt_get_extension_data2
FUNCS += functions/gnutls_x509_crt_get_extension_data2.short
FUNCS += functions/gnutls_x509_crt_get_extension_info
FUNCS += functions/gnutls_x509_crt_get_extension_info.short
FUNCS += functions/gnutls_x509_crt_get_extension_oid
FUNCS += functions/gnutls_x509_crt_get_extension_oid.short
FUNCS += functions/gnutls_x509_crt_get_fingerprint
FUNCS += functions/gnutls_x509_crt_get_fingerprint.short
FUNCS += functions/gnutls_x509_crt_get_issuer
FUNCS += functions/gnutls_x509_crt_get_issuer.short
FUNCS += functions/gnutls_x509_crt_get_issuer_alt_name
FUNCS += functions/gnutls_x509_crt_get_issuer_alt_name.short
FUNCS += functions/gnutls_x509_crt_get_issuer_alt_name2
FUNCS += functions/gnutls_x509_crt_get_issuer_alt_name2.short
FUNCS += functions/gnutls_x509_crt_get_issuer_alt_othername_oid
FUNCS += functions/gnutls_x509_crt_get_issuer_alt_othername_oid.short
FUNCS += functions/gnutls_x509_crt_get_issuer_dn
FUNCS += functions/gnutls_x509_crt_get_issuer_dn.short
FUNCS += functions/gnutls_x509_crt_get_issuer_dn2
FUNCS += functions/gnutls_x509_crt_get_issuer_dn2.short
FUNCS += functions/gnutls_x509_crt_get_issuer_dn3
FUNCS += functions/gnutls_x509_crt_get_issuer_dn3.short
FUNCS += functions/gnutls_x509_crt_get_issuer_dn_by_oid
FUNCS += functions/gnutls_x509_crt_get_issuer_dn_by_oid.short
FUNCS += functions/gnutls_x509_crt_get_issuer_dn_oid
FUNCS += functions/gnutls_x509_crt_get_issuer_dn_oid.short
FUNCS += functions/gnutls_x509_crt_get_issuer_unique_id
FUNCS += functions/gnutls_x509_crt_get_issuer_unique_id.short
FUNCS += functions/gnutls_x509_crt_get_key_id
FUNCS += functions/gnutls_x509_crt_get_key_id.short
FUNCS += functions/gnutls_x509_crt_get_key_purpose_oid
FUNCS += functions/gnutls_x509_crt_get_key_purpose_oid.short
FUNCS += functions/gnutls_x509_crt_get_key_usage
FUNCS += functions/gnutls_x509_crt_get_key_usage.short
FUNCS += functions/gnutls_x509_crt_get_name_constraints
FUNCS += functions/gnutls_x509_crt_get_name_constraints.short
FUNCS += functions/gnutls_x509_crt_get_pk_algorithm
FUNCS += functions/gnutls_x509_crt_get_pk_algorithm.short
FUNCS += functions/gnutls_x509_crt_get_pk_dsa_raw
FUNCS += functions/gnutls_x509_crt_get_pk_dsa_raw.short
FUNCS += functions/gnutls_x509_crt_get_pk_ecc_raw
FUNCS += functions/gnutls_x509_crt_get_pk_ecc_raw.short
FUNCS += functions/gnutls_x509_crt_get_pk_oid
FUNCS += functions/gnutls_x509_crt_get_pk_oid.short
FUNCS += functions/gnutls_x509_crt_get_pk_rsa_raw
FUNCS += functions/gnutls_x509_crt_get_pk_rsa_raw.short
FUNCS += functions/gnutls_x509_crt_get_policy
FUNCS += functions/gnutls_x509_crt_get_policy.short
FUNCS += functions/gnutls_x509_crt_get_preferred_hash_algorithm
FUNCS += functions/gnutls_x509_crt_get_preferred_hash_algorithm.short
FUNCS += functions/gnutls_x509_crt_get_private_key_usage_period
FUNCS += functions/gnutls_x509_crt_get_private_key_usage_period.short
FUNCS += functions/gnutls_x509_crt_get_proxy
FUNCS += functions/gnutls_x509_crt_get_proxy.short
FUNCS += functions/gnutls_x509_crt_get_raw_dn
FUNCS += functions/gnutls_x509_crt_get_raw_dn.short
FUNCS += functions/gnutls_x509_crt_get_raw_issuer_dn
FUNCS += functions/gnutls_x509_crt_get_raw_issuer_dn.short
FUNCS += functions/gnutls_x509_crt_get_serial
FUNCS += functions/gnutls_x509_crt_get_serial.short
FUNCS += functions/gnutls_x509_crt_get_signature
FUNCS += functions/gnutls_x509_crt_get_signature.short
FUNCS += functions/gnutls_x509_crt_get_signature_algorithm
FUNCS += functions/gnutls_x509_crt_get_signature_algorithm.short
FUNCS += functions/gnutls_x509_crt_get_signature_oid
FUNCS += functions/gnutls_x509_crt_get_signature_oid.short
FUNCS += functions/gnutls_x509_crt_get_subject
FUNCS += functions/gnutls_x509_crt_get_subject.short
FUNCS += functions/gnutls_x509_crt_get_subject_alt_name
FUNCS += functions/gnutls_x509_crt_get_subject_alt_name.short
FUNCS += functions/gnutls_x509_crt_get_subject_alt_name2
FUNCS += functions/gnutls_x509_crt_get_subject_alt_name2.short
FUNCS += functions/gnutls_x509_crt_get_subject_alt_othername_oid
FUNCS += functions/gnutls_x509_crt_get_subject_alt_othername_oid.short
FUNCS += functions/gnutls_x509_crt_get_subject_key_id
FUNCS += functions/gnutls_x509_crt_get_subject_key_id.short
FUNCS += functions/gnutls_x509_crt_get_subject_unique_id
FUNCS += functions/gnutls_x509_crt_get_subject_unique_id.short
FUNCS += functions/gnutls_x509_crt_get_tlsfeatures
FUNCS += functions/gnutls_x509_crt_get_tlsfeatures.short
FUNCS += functions/gnutls_x509_crt_get_version
FUNCS += functions/gnutls_x509_crt_get_version.short
FUNCS += functions/gnutls_x509_crt_import
FUNCS += functions/gnutls_x509_crt_import.short
FUNCS += functions/gnutls_x509_crt_import_pkcs11
FUNCS += functions/gnutls_x509_crt_import_pkcs11.short
FUNCS += functions/gnutls_x509_crt_import_url
FUNCS += functions/gnutls_x509_crt_import_url.short
FUNCS += functions/gnutls_x509_crt_init
FUNCS += functions/gnutls_x509_crt_init.short
FUNCS += functions/gnutls_x509_crt_list_import
FUNCS += functions/gnutls_x509_crt_list_import.short
FUNCS += functions/gnutls_x509_crt_list_import2
FUNCS += functions/gnutls_x509_crt_list_import2.short
FUNCS += functions/gnutls_x509_crt_list_import_pkcs11
FUNCS += functions/gnutls_x509_crt_list_import_pkcs11.short
FUNCS += functions/gnutls_x509_crt_list_verify
FUNCS += functions/gnutls_x509_crt_list_verify.short
FUNCS += functions/gnutls_x509_crt_print
FUNCS += functions/gnutls_x509_crt_print.short
FUNCS += functions/gnutls_x509_crt_privkey_sign
FUNCS += functions/gnutls_x509_crt_privkey_sign.short
FUNCS += functions/gnutls_x509_crt_set_activation_time
FUNCS += functions/gnutls_x509_crt_set_activation_time.short
FUNCS += functions/gnutls_x509_crt_set_authority_info_access
FUNCS += functions/gnutls_x509_crt_set_authority_info_access.short
FUNCS += functions/gnutls_x509_crt_set_authority_key_id
FUNCS += functions/gnutls_x509_crt_set_authority_key_id.short
FUNCS += functions/gnutls_x509_crt_set_basic_constraints
FUNCS += functions/gnutls_x509_crt_set_basic_constraints.short
FUNCS += functions/gnutls_x509_crt_set_ca_status
FUNCS += functions/gnutls_x509_crt_set_ca_status.short
FUNCS += functions/gnutls_x509_crt_set_crl_dist_points
FUNCS += functions/gnutls_x509_crt_set_crl_dist_points.short
FUNCS += functions/gnutls_x509_crt_set_crl_dist_points2
FUNCS += functions/gnutls_x509_crt_set_crl_dist_points2.short
FUNCS += functions/gnutls_x509_crt_set_crq
FUNCS += functions/gnutls_x509_crt_set_crq.short
FUNCS += functions/gnutls_x509_crt_set_crq_extension_by_oid
FUNCS += functions/gnutls_x509_crt_set_crq_extension_by_oid.short
FUNCS += functions/gnutls_x509_crt_set_crq_extensions
FUNCS += functions/gnutls_x509_crt_set_crq_extensions.short
FUNCS += functions/gnutls_x509_crt_set_dn
FUNCS += functions/gnutls_x509_crt_set_dn.short
FUNCS += functions/gnutls_x509_crt_set_dn_by_oid
FUNCS += functions/gnutls_x509_crt_set_dn_by_oid.short
FUNCS += functions/gnutls_x509_crt_set_expiration_time
FUNCS += functions/gnutls_x509_crt_set_expiration_time.short
FUNCS += functions/gnutls_x509_crt_set_extension_by_oid
FUNCS += functions/gnutls_x509_crt_set_extension_by_oid.short
FUNCS += functions/gnutls_x509_crt_set_issuer_alt_name
FUNCS += functions/gnutls_x509_crt_set_issuer_alt_name.short
FUNCS += functions/gnutls_x509_crt_set_issuer_alt_othername
FUNCS += functions/gnutls_x509_crt_set_issuer_alt_othername.short
FUNCS += functions/gnutls_x509_crt_set_issuer_dn
FUNCS += functions/gnutls_x509_crt_set_issuer_dn.short
FUNCS += functions/gnutls_x509_crt_set_issuer_dn_by_oid
FUNCS += functions/gnutls_x509_crt_set_issuer_dn_by_oid.short
FUNCS += functions/gnutls_x509_crt_set_issuer_unique_id
FUNCS += functions/gnutls_x509_crt_set_issuer_unique_id.short
FUNCS += functions/gnutls_x509_crt_set_key
FUNCS += functions/gnutls_x509_crt_set_key.short
FUNCS += functions/gnutls_x509_crt_set_key_purpose_oid
FUNCS += functions/gnutls_x509_crt_set_key_purpose_oid.short
FUNCS += functions/gnutls_x509_crt_set_key_usage
FUNCS += functions/gnutls_x509_crt_set_key_usage.short
FUNCS += functions/gnutls_x509_crt_set_name_constraints
FUNCS += functions/gnutls_x509_crt_set_name_constraints.short
FUNCS += functions/gnutls_x509_crt_set_pin_function
FUNCS += functions/gnutls_x509_crt_set_pin_function.short
FUNCS += functions/gnutls_x509_crt_set_policy
FUNCS += functions/gnutls_x509_crt_set_policy.short
FUNCS += functions/gnutls_x509_crt_set_private_key_usage_period
FUNCS += functions/gnutls_x509_crt_set_private_key_usage_period.short
FUNCS += functions/gnutls_x509_crt_set_proxy
FUNCS += functions/gnutls_x509_crt_set_proxy.short
FUNCS += functions/gnutls_x509_crt_set_proxy_dn
FUNCS += functions/gnutls_x509_crt_set_proxy_dn.short
FUNCS += functions/gnutls_x509_crt_set_pubkey
FUNCS += functions/gnutls_x509_crt_set_pubkey.short
FUNCS += functions/gnutls_x509_crt_set_serial
FUNCS += functions/gnutls_x509_crt_set_serial.short
FUNCS += functions/gnutls_x509_crt_set_subject_alternative_name
FUNCS += functions/gnutls_x509_crt_set_subject_alternative_name.short
FUNCS += functions/gnutls_x509_crt_set_subject_alt_name
FUNCS += functions/gnutls_x509_crt_set_subject_alt_name.short
FUNCS += functions/gnutls_x509_crt_set_subject_alt_othername
FUNCS += functions/gnutls_x509_crt_set_subject_alt_othername.short
FUNCS += functions/gnutls_x509_crt_set_subject_key_id
FUNCS += functions/gnutls_x509_crt_set_subject_key_id.short
FUNCS += functions/gnutls_x509_crt_set_subject_unique_id
FUNCS += functions/gnutls_x509_crt_set_subject_unique_id.short
FUNCS += functions/gnutls_x509_crt_set_tlsfeatures
FUNCS += functions/gnutls_x509_crt_set_tlsfeatures.short
FUNCS += functions/gnutls_x509_crt_set_version
FUNCS += functions/gnutls_x509_crt_set_version.short
FUNCS += functions/gnutls_x509_crt_sign
FUNCS += functions/gnutls_x509_crt_sign.short
FUNCS += functions/gnutls_x509_crt_sign2
FUNCS += functions/gnutls_x509_crt_sign2.short
FUNCS += functions/gnutls_x509_crt_verify
FUNCS += functions/gnutls_x509_crt_verify.short
FUNCS += functions/gnutls_x509_crt_verify_data2
FUNCS += functions/gnutls_x509_crt_verify_data2.short
FUNCS += functions/gnutls_x509_dn_deinit
FUNCS += functions/gnutls_x509_dn_deinit.short
FUNCS += functions/gnutls_x509_dn_export
FUNCS += functions/gnutls_x509_dn_export.short
FUNCS += functions/gnutls_x509_dn_export2
FUNCS += functions/gnutls_x509_dn_export2.short
FUNCS += functions/gnutls_x509_dn_get_rdn_ava
FUNCS += functions/gnutls_x509_dn_get_rdn_ava.short
FUNCS += functions/gnutls_x509_dn_get_str
FUNCS += functions/gnutls_x509_dn_get_str.short
FUNCS += functions/gnutls_x509_dn_get_str2
FUNCS += functions/gnutls_x509_dn_get_str2.short
FUNCS += functions/gnutls_x509_dn_import
FUNCS += functions/gnutls_x509_dn_import.short
FUNCS += functions/gnutls_x509_dn_init
FUNCS += functions/gnutls_x509_dn_init.short
FUNCS += functions/gnutls_x509_dn_oid_known
FUNCS += functions/gnutls_x509_dn_oid_known.short
FUNCS += functions/gnutls_x509_dn_oid_name
FUNCS += functions/gnutls_x509_dn_oid_name.short
FUNCS += functions/gnutls_x509_dn_set_str
FUNCS += functions/gnutls_x509_dn_set_str.short
FUNCS += functions/gnutls_x509_ext_deinit
FUNCS += functions/gnutls_x509_ext_deinit.short
FUNCS += functions/gnutls_x509_ext_export_aia
FUNCS += functions/gnutls_x509_ext_export_aia.short
FUNCS += functions/gnutls_x509_ext_export_authority_key_id
FUNCS += functions/gnutls_x509_ext_export_authority_key_id.short
FUNCS += functions/gnutls_x509_ext_export_basic_constraints
FUNCS += functions/gnutls_x509_ext_export_basic_constraints.short
FUNCS += functions/gnutls_x509_ext_export_crl_dist_points
FUNCS += functions/gnutls_x509_ext_export_crl_dist_points.short
FUNCS += functions/gnutls_x509_ext_export_key_purposes
FUNCS += functions/gnutls_x509_ext_export_key_purposes.short
FUNCS += functions/gnutls_x509_ext_export_key_usage
FUNCS += functions/gnutls_x509_ext_export_key_usage.short
FUNCS += functions/gnutls_x509_ext_export_name_constraints
FUNCS += functions/gnutls_x509_ext_export_name_constraints.short
FUNCS += functions/gnutls_x509_ext_export_policies
FUNCS += functions/gnutls_x509_ext_export_policies.short
FUNCS += functions/gnutls_x509_ext_export_private_key_usage_period
FUNCS += functions/gnutls_x509_ext_export_private_key_usage_period.short
FUNCS += functions/gnutls_x509_ext_export_proxy
FUNCS += functions/gnutls_x509_ext_export_proxy.short
FUNCS += functions/gnutls_x509_ext_export_subject_alt_names
FUNCS += functions/gnutls_x509_ext_export_subject_alt_names.short
FUNCS += functions/gnutls_x509_ext_export_subject_key_id
FUNCS += functions/gnutls_x509_ext_export_subject_key_id.short
FUNCS += functions/gnutls_x509_ext_export_tlsfeatures
FUNCS += functions/gnutls_x509_ext_export_tlsfeatures.short
FUNCS += functions/gnutls_x509_ext_import_aia
FUNCS += functions/gnutls_x509_ext_import_aia.short
FUNCS += functions/gnutls_x509_ext_import_authority_key_id
FUNCS += functions/gnutls_x509_ext_import_authority_key_id.short
FUNCS += functions/gnutls_x509_ext_import_basic_constraints
FUNCS += functions/gnutls_x509_ext_import_basic_constraints.short
FUNCS += functions/gnutls_x509_ext_import_crl_dist_points
FUNCS += functions/gnutls_x509_ext_import_crl_dist_points.short
FUNCS += functions/gnutls_x509_ext_import_key_purposes
FUNCS += functions/gnutls_x509_ext_import_key_purposes.short
FUNCS += functions/gnutls_x509_ext_import_key_usage
FUNCS += functions/gnutls_x509_ext_import_key_usage.short
FUNCS += functions/gnutls_x509_ext_import_name_constraints
FUNCS += functions/gnutls_x509_ext_import_name_constraints.short
FUNCS += functions/gnutls_x509_ext_import_policies
FUNCS += functions/gnutls_x509_ext_import_policies.short
FUNCS += functions/gnutls_x509_ext_import_private_key_usage_period
FUNCS += functions/gnutls_x509_ext_import_private_key_usage_period.short
FUNCS += functions/gnutls_x509_ext_import_proxy
FUNCS += functions/gnutls_x509_ext_import_proxy.short
FUNCS += functions/gnutls_x509_ext_import_subject_alt_names
FUNCS += functions/gnutls_x509_ext_import_subject_alt_names.short
FUNCS += functions/gnutls_x509_ext_import_subject_key_id
FUNCS += functions/gnutls_x509_ext_import_subject_key_id.short
FUNCS += functions/gnutls_x509_ext_import_tlsfeatures
FUNCS += functions/gnutls_x509_ext_import_tlsfeatures.short
FUNCS += functions/gnutls_x509_ext_print
FUNCS += functions/gnutls_x509_ext_print.short
FUNCS += functions/gnutls_x509_key_purpose_deinit
FUNCS += functions/gnutls_x509_key_purpose_deinit.short
FUNCS += functions/gnutls_x509_key_purpose_get
FUNCS += functions/gnutls_x509_key_purpose_get.short
FUNCS += functions/gnutls_x509_key_purpose_init
FUNCS += functions/gnutls_x509_key_purpose_init.short
FUNCS += functions/gnutls_x509_key_purpose_set
FUNCS += functions/gnutls_x509_key_purpose_set.short
FUNCS += functions/gnutls_x509_name_constraints_add_excluded
FUNCS += functions/gnutls_x509_name_constraints_add_excluded.short
FUNCS += functions/gnutls_x509_name_constraints_add_permitted
FUNCS += functions/gnutls_x509_name_constraints_add_permitted.short
FUNCS += functions/gnutls_x509_name_constraints_check
FUNCS += functions/gnutls_x509_name_constraints_check.short
FUNCS += functions/gnutls_x509_name_constraints_check_crt
FUNCS += functions/gnutls_x509_name_constraints_check_crt.short
FUNCS += functions/gnutls_x509_name_constraints_deinit
FUNCS += functions/gnutls_x509_name_constraints_deinit.short
FUNCS += functions/gnutls_x509_name_constraints_get_excluded
FUNCS += functions/gnutls_x509_name_constraints_get_excluded.short
FUNCS += functions/gnutls_x509_name_constraints_get_permitted
FUNCS += functions/gnutls_x509_name_constraints_get_permitted.short
FUNCS += functions/gnutls_x509_name_constraints_init
FUNCS += functions/gnutls_x509_name_constraints_init.short
FUNCS += functions/gnutls_x509_othername_to_virtual
FUNCS += functions/gnutls_x509_othername_to_virtual.short
FUNCS += functions/gnutls_x509_policies_deinit
FUNCS += functions/gnutls_x509_policies_deinit.short
FUNCS += functions/gnutls_x509_policies_get
FUNCS += functions/gnutls_x509_policies_get.short
FUNCS += functions/gnutls_x509_policies_init
FUNCS += functions/gnutls_x509_policies_init.short
FUNCS += functions/gnutls_x509_policies_set
FUNCS += functions/gnutls_x509_policies_set.short
FUNCS += functions/gnutls_x509_policy_release
FUNCS += functions/gnutls_x509_policy_release.short
FUNCS += functions/gnutls_x509_privkey_cpy
FUNCS += functions/gnutls_x509_privkey_cpy.short
FUNCS += functions/gnutls_x509_privkey_deinit
FUNCS += functions/gnutls_x509_privkey_deinit.short
FUNCS += functions/gnutls_x509_privkey_export
FUNCS += functions/gnutls_x509_privkey_export.short
FUNCS += functions/gnutls_x509_privkey_export2
FUNCS += functions/gnutls_x509_privkey_export2.short
FUNCS += functions/gnutls_x509_privkey_export2_pkcs8
FUNCS += functions/gnutls_x509_privkey_export2_pkcs8.short
FUNCS += functions/gnutls_x509_privkey_export_dsa_raw
FUNCS += functions/gnutls_x509_privkey_export_dsa_raw.short
FUNCS += functions/gnutls_x509_privkey_export_ecc_raw
FUNCS += functions/gnutls_x509_privkey_export_ecc_raw.short
FUNCS += functions/gnutls_x509_privkey_export_pkcs8
FUNCS += functions/gnutls_x509_privkey_export_pkcs8.short
FUNCS += functions/gnutls_x509_privkey_export_rsa_raw
FUNCS += functions/gnutls_x509_privkey_export_rsa_raw.short
FUNCS += functions/gnutls_x509_privkey_export_rsa_raw2
FUNCS += functions/gnutls_x509_privkey_export_rsa_raw2.short
FUNCS += functions/gnutls_x509_privkey_fix
FUNCS += functions/gnutls_x509_privkey_fix.short
FUNCS += functions/gnutls_x509_privkey_generate
FUNCS += functions/gnutls_x509_privkey_generate.short
FUNCS += functions/gnutls_x509_privkey_generate2
FUNCS += functions/gnutls_x509_privkey_generate2.short
FUNCS += functions/gnutls_x509_privkey_get_key_id
FUNCS += functions/gnutls_x509_privkey_get_key_id.short
FUNCS += functions/gnutls_x509_privkey_get_pk_algorithm
FUNCS += functions/gnutls_x509_privkey_get_pk_algorithm.short
FUNCS += functions/gnutls_x509_privkey_get_pk_algorithm2
FUNCS += functions/gnutls_x509_privkey_get_pk_algorithm2.short
FUNCS += functions/gnutls_x509_privkey_get_seed
FUNCS += functions/gnutls_x509_privkey_get_seed.short
FUNCS += functions/gnutls_x509_privkey_import
FUNCS += functions/gnutls_x509_privkey_import.short
FUNCS += functions/gnutls_x509_privkey_import2
FUNCS += functions/gnutls_x509_privkey_import2.short
FUNCS += functions/gnutls_x509_privkey_import_dsa_raw
FUNCS += functions/gnutls_x509_privkey_import_dsa_raw.short
FUNCS += functions/gnutls_x509_privkey_import_ecc_raw
FUNCS += functions/gnutls_x509_privkey_import_ecc_raw.short
FUNCS += functions/gnutls_x509_privkey_import_openssl
FUNCS += functions/gnutls_x509_privkey_import_openssl.short
FUNCS += functions/gnutls_x509_privkey_import_pkcs8
FUNCS += functions/gnutls_x509_privkey_import_pkcs8.short
FUNCS += functions/gnutls_x509_privkey_import_rsa_raw
FUNCS += functions/gnutls_x509_privkey_import_rsa_raw.short
FUNCS += functions/gnutls_x509_privkey_import_rsa_raw2
FUNCS += functions/gnutls_x509_privkey_import_rsa_raw2.short
FUNCS += functions/gnutls_x509_privkey_init
FUNCS += functions/gnutls_x509_privkey_init.short
FUNCS += functions/gnutls_x509_privkey_sec_param
FUNCS += functions/gnutls_x509_privkey_sec_param.short
FUNCS += functions/gnutls_x509_privkey_set_flags
FUNCS += functions/gnutls_x509_privkey_set_flags.short
FUNCS += functions/gnutls_x509_privkey_set_pin_function
FUNCS += functions/gnutls_x509_privkey_set_pin_function.short
FUNCS += functions/gnutls_x509_privkey_sign_data
FUNCS += functions/gnutls_x509_privkey_sign_data.short
FUNCS += functions/gnutls_x509_privkey_sign_hash
FUNCS += functions/gnutls_x509_privkey_sign_hash.short
FUNCS += functions/gnutls_x509_privkey_verify_params
FUNCS += functions/gnutls_x509_privkey_verify_params.short
FUNCS += functions/gnutls_x509_privkey_verify_seed
FUNCS += functions/gnutls_x509_privkey_verify_seed.short
FUNCS += functions/gnutls_x509_rdn_get
FUNCS += functions/gnutls_x509_rdn_get.short
FUNCS += functions/gnutls_x509_rdn_get2
FUNCS += functions/gnutls_x509_rdn_get2.short
FUNCS += functions/gnutls_x509_rdn_get_by_oid
FUNCS += functions/gnutls_x509_rdn_get_by_oid.short
FUNCS += functions/gnutls_x509_rdn_get_oid
FUNCS += functions/gnutls_x509_rdn_get_oid.short
FUNCS += functions/gnutls_x509_tlsfeatures_add
FUNCS += functions/gnutls_x509_tlsfeatures_add.short
FUNCS += functions/gnutls_x509_tlsfeatures_check_crt
FUNCS += functions/gnutls_x509_tlsfeatures_check_crt.short
FUNCS += functions/gnutls_x509_tlsfeatures_deinit
FUNCS += functions/gnutls_x509_tlsfeatures_deinit.short
FUNCS += functions/gnutls_x509_tlsfeatures_get
FUNCS += functions/gnutls_x509_tlsfeatures_get.short
FUNCS += functions/gnutls_x509_tlsfeatures_init
FUNCS += functions/gnutls_x509_tlsfeatures_init.short
FUNCS += functions/gnutls_x509_trust_list_add_cas
FUNCS += functions/gnutls_x509_trust_list_add_cas.short
FUNCS += functions/gnutls_x509_trust_list_add_crls
FUNCS += functions/gnutls_x509_trust_list_add_crls.short
FUNCS += functions/gnutls_x509_trust_list_add_named_crt
FUNCS += functions/gnutls_x509_trust_list_add_named_crt.short
FUNCS += functions/gnutls_x509_trust_list_add_system_trust
FUNCS += functions/gnutls_x509_trust_list_add_system_trust.short
FUNCS += functions/gnutls_x509_trust_list_add_trust_dir
FUNCS += functions/gnutls_x509_trust_list_add_trust_dir.short
FUNCS += functions/gnutls_x509_trust_list_add_trust_file
FUNCS += functions/gnutls_x509_trust_list_add_trust_file.short
FUNCS += functions/gnutls_x509_trust_list_add_trust_mem
FUNCS += functions/gnutls_x509_trust_list_add_trust_mem.short
FUNCS += functions/gnutls_x509_trust_list_deinit
FUNCS += functions/gnutls_x509_trust_list_deinit.short
FUNCS += functions/gnutls_x509_trust_list_get_issuer
FUNCS += functions/gnutls_x509_trust_list_get_issuer.short
FUNCS += functions/gnutls_x509_trust_list_get_issuer_by_dn
FUNCS += functions/gnutls_x509_trust_list_get_issuer_by_dn.short
FUNCS += functions/gnutls_x509_trust_list_get_issuer_by_subject_key_id
FUNCS += functions/gnutls_x509_trust_list_get_issuer_by_subject_key_id.short
FUNCS += functions/gnutls_x509_trust_list_init
FUNCS += functions/gnutls_x509_trust_list_init.short
FUNCS += functions/gnutls_x509_trust_list_iter_deinit
FUNCS += functions/gnutls_x509_trust_list_iter_deinit.short
FUNCS += functions/gnutls_x509_trust_list_iter_get_ca
FUNCS += functions/gnutls_x509_trust_list_iter_get_ca.short
FUNCS += functions/gnutls_x509_trust_list_remove_cas
FUNCS += functions/gnutls_x509_trust_list_remove_cas.short
FUNCS += functions/gnutls_x509_trust_list_remove_trust_file
FUNCS += functions/gnutls_x509_trust_list_remove_trust_file.short
FUNCS += functions/gnutls_x509_trust_list_remove_trust_mem
FUNCS += functions/gnutls_x509_trust_list_remove_trust_mem.short
FUNCS += functions/gnutls_x509_trust_list_verify_crt
FUNCS += functions/gnutls_x509_trust_list_verify_crt.short
FUNCS += functions/gnutls_x509_trust_list_verify_crt2
FUNCS += functions/gnutls_x509_trust_list_verify_crt2.short
FUNCS += functions/gnutls_x509_trust_list_verify_named_crt
FUNCS += functions/gnutls_x509_trust_list_verify_named_crt.short
