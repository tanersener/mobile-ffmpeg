# Copyright (C) 2006-2012 Free Software Foundation, Inc.
#
# Author: Simon Josefsson
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

WFLAGS ?= --enable-gcc-warnings
ADDFLAGS ?=
CFGFLAGS ?= --enable-gtk-doc --enable-gtk-doc-pdf --enable-gtk-doc-html $(ADDFLAGS) $(WFLAGS)
PACKAGE ?= gnutls

.PHONY: config

INDENT_SOURCES = `find . -name \*.[ch] -o -name gnutls.h.in | grep -v -e ^./build-aux/ -e ^./lib/minitasn1/ -e ^./lib/build-aux/ -e ^./gl/ -e ^./src/libopts/ -e -args.[ch] -e asn1_tab.c -e ^./tests/suite/`

ifeq ($(.DEFAULT_GOAL),abort-due-to-no-makefile)
.DEFAULT_GOAL := bootstrap
endif

PODIR := po
PO_DOMAIN := libgnutls

local-checks-to-skip = sc_GPL_version sc_bindtextdomain			\
	sc_immutable_NEWS sc_program_name sc_prohibit_atoi_atof		\
	sc_prohibit_empty_lines_at_EOF sc_prohibit_hash_without_use	\
	sc_prohibit_have_config_h sc_prohibit_magic_number_exit		\
	sc_prohibit_strcmp sc_require_config_h				\
	sc_require_config_h_first sc_texinfo_acronym sc_trailing_blank	\
	sc_unmarked_diagnostics sc_useless_cpp_parens			\
	sc_two_space_separator_in_usage

VC_LIST_ALWAYS_EXCLUDE_REGEX = ^maint.mk|gtk-doc.make|m4/pkg|doc/fdl-1.3.texi|po/.*.po.in|src/crywrap/|(devel/perlasm/|lib/accelerated/x86/|build-aux/|gl/|src/libopts/|tests/suite/ecore/|doc/protocol/).*$$
update-copyright-env = UPDATE_COPYRIGHT_USE_INTERVALS=1

# Explicit syntax-check exceptions.
exclude_file_name_regexp--sc_error_message_period = ^src/crywrap/crywrap.c$$
exclude_file_name_regexp--sc_error_message_uppercase = ^doc/examples/ex-cxx.cpp|guile/src/core.c|src/certtool.c|src/ocsptool.c|src/crywrap/crywrap.c|tests/pkcs12_encode.c$$
exclude_file_name_regexp--sc_file_system = ^doc/doxygen/Doxyfile
exclude_file_name_regexp--sc_prohibit_cvs_keyword = ^lib/nettle/.*$$
exclude_file_name_regexp--sc_prohibit_undesirable_word_seq = ^tests/nist-pkits/gnutls-nist-tests.html$$
exclude_file_name_regexp--sc_space_tab = ^doc/.*.(pdf|png)|\.crl|\.pdf|\.zip|tests/nist-pkits/|tests/suite/x509paths/.*$$
_makefile_at_at_check_exceptions = ' && !/CODE_COVERAGE_RULES/ && !/VERSION/'
exclude_file_name_regexp--sc_m4_quote_check='lib/unistring/m4/absolute-header.m4'
exclude_file_name_regexp--sc_makefile_at_at_check='lib/unistring/Makefile.am'
exclude_file_name_regexp--sc_prohibit_stddef_without_use='u*-normalize.c'
exclude_file_name_regexp--sc_prohibit_strncpy='unistr.in.h'
gl_public_submodule_commit =

autoreconf:
	for f in $(PODIR)/*.po.in; do \
		cp $$f `echo $$f | sed 's/.in//'`; \
	done
	autopoint
	for i in po.m4 nls.m4 gettext.m4 codeset.m4 glibc21.m4 glibc2.m4 iconv.m4 intdiv0.m4 intldir.m4 intl.m4 intlmacosx.m4 intmax.m4 inttypes_h.m4 inttypes-pri.m4 lcmessage.m4 lib-ld.m4 lib-link.m4 lib-prefix.m4 lock.m4 longlong.m4 printf-posix.m4 progtest.m4 size_max.m4 stdint_h.m4 uintmax_t.m4 wchar_t.m4 wint_t.m4 visibility.m4 xsize.m4;do \
		if test -f /usr/share/aclocal/$$i;then \
			rm -f m4/$$i; \
		fi; \
	done
	touch ChangeLog
	test -f ./configure || AUTOPOINT=true autoreconf --install

update-po: refresh-po
	for f in `ls $(PODIR)/*.po | grep -v quot.po`; do \
		cp $$f $$f.in; \
	done
	git add $(PODIR)/*.po.in
	git commit -m "Sync with TP." $(PODIR)/LINGUAS $(PODIR)/*.po.in

config:
	./configure $(CFGFLAGS)

.submodule.stamp:
	git submodule init
	git submodule update
	touch $@

bootstrap: autoreconf .submodule.stamp

UNISTRING_MODULES = "unistr/u8-check unistr/u8-to-u16 unistr/u8-to-u32 unistr/u32-to-u8 \
	unistr/u16-to-u8 uninorm/nfc uninorm/nfkc uninorm/u8-normalize uninorm/u16-normalize \
	uninorm/u32-normalize unictype/category-all unictype/property-not-a-character \
	unictype/property-default-ignorable-code-point unictype/property-join-control"

unistringimport:
	../gnulib/gnulib-tool --without-tests --libtool --macro-prefix=unistring --lgpl=3orGPLv2 --dir=. --local-dir=lib/unistring/override --lib=libunistring --without-tests --source-base=lib/unistring --m4-base=lib/unistring/m4 --doc-base=doc --aux-dir=build-aux --import $(UNISTRING_MODULES)

# The only non-lgpl modules used are: gettime progname timespec. Those
# are not used (and must not be used) in the library)
glimport: unistringimport
	../gnulib/gnulib-tool --dir=. --local-dir=gl/override --lib=libgnu --source-base=gl --m4-base=gl/m4 --doc-base=doc --tests-base=gl/tests --aux-dir=build-aux --lgpl=2 --add-import
	../gnulib/gnulib-tool --dir=. --local-dir=src/gl/override --lib=libgnu_gpl --source-base=src/gl --m4-base=src/gl/m4 --doc-base=doc --tests-base=tests --aux-dir=build-aux --add-import

# Code Coverage

clang:
	make clean
	scan-build ./configure
	rm -rf scan.tmp
	scan-build -o scan.tmp make

clang-copy:
	rm -fv `find $(htmldir)/clang -type f | grep -v CVS`
	mkdir -p $(htmldir)/clang/
	cp -rv scan.tmp/*/* $(htmldir)/clang/

# Release

ChangeLog:
	git log --pretty --numstat --summary --since="2014 November 07" -- | git2cl > ChangeLog
	cat .clcopying >> ChangeLog

tag = $(PACKAGE)_`echo $(VERSION) | sed 's/\./_/g'`

release: syntax-check prepare upload-tarballs

prepare:
	! git tag -l $(tag) | grep $(PACKAGE) > /dev/null
	git tag -u 96865171! -m $(VERSION) $(tag)

upload-tarballs: dist
	gpg --sign --detached $(distdir).tar.xz
	scp $(distdir).tar.xz* trithemius.gnupg.org:/home/ftp/gcrypt/gnutls/v$(MAJOR_VERSION).$(MINOR_VERSION)

web:
	echo generating documentation for $(PACKAGE)
	mkdir -p $(htmldir)/manual
	mkdir -p $(htmldir)/reference
	make -C doc gnutls.html
	cd doc && cp gnutls.html *.png ../$(htmldir)/manual/
	cd doc && makeinfo --html --split=node -o ../$(htmldir)/manual/html_node/ --css-include=./texinfo.css gnutls.texi
	cd doc && cp *.png ../$(htmldir)/manual/html_node/
	sed 's/\@VERSION\@/$(VERSION)/g' -i $(htmldir)/manual/html_node/*.html $(htmldir)/manual/gnutls.html
	-cd doc && make gnutls.epub && cp gnutls.epub ../$(htmldir)/manual/
	cd doc/latex && make gnutls.pdf && cp gnutls.pdf ../../$(htmldir)/manual/
	make -C doc gnutls-guile.html gnutls-guile.pdf
	cd doc && makeinfo --html --split=node -o ../$(htmldir)/manual/gnutls-guile/ --css-include=./texinfo.css gnutls-guile.texi
	cd doc && cp gnutls-guile.pdf gnutls-guile.html ../$(htmldir)/manual/
	-cp -v doc/reference/html/*.html doc/reference/html/*.png doc/reference/html/*.devhelp* doc/reference/html/*.css $(htmldir)/reference/

ASM_SOURCES_XXX := \
	lib/accelerated/aarch64/XXX/ghash-aarch64.s \
	lib/accelerated/aarch64/XXX/aes-aarch64.s \
	lib/accelerated/aarch64/XXX/sha1-armv8.s \
	lib/accelerated/aarch64/XXX/sha256-armv8.s \
	lib/accelerated/aarch64/XXX/sha512-armv8.s \
	lib/accelerated/x86/XXX/cpuid-x86_64.s \
	lib/accelerated/x86/XXX/cpuid-x86.s \
	lib/accelerated/x86/XXX/ghash-x86_64.s \
	lib/accelerated/x86/XXX/aesni-x86_64.s \
	lib/accelerated/x86/XXX/aesni-x86.s \
	lib/accelerated/x86/XXX/sha1-ssse3-x86.s \
	lib/accelerated/x86/XXX/sha1-ssse3-x86_64.s \
	lib/accelerated/x86/XXX/sha256-ssse3-x86.s \
	lib/accelerated/x86/XXX/sha512-ssse3-x86.s \
	lib/accelerated/x86/XXX/sha512-ssse3-x86_64.s \
	lib/accelerated/x86/XXX/aesni-gcm-x86_64.s \
	lib/accelerated/x86/XXX/aes-ssse3-x86.s \
	lib/accelerated/x86/XXX/aes-ssse3-x86_64.s

ASM_SOURCES_ELF := $(subst XXX,elf,$(ASM_SOURCES_XXX))
ASM_SOURCES_COFF := $(subst XXX,coff,$(ASM_SOURCES_XXX))
ASM_SOURCES_MACOSX := $(subst XXX,macosx,$(ASM_SOURCES_XXX))

asm-sources: $(ASM_SOURCES_ELF) $(ASM_SOURCES_COFF) $(ASM_SOURCES_MACOSX) lib/accelerated/x86/files.mk

asm-sources-clean:
	rm -f $(ASM_SOURCES_ELF) $(ASM_SOURCES_COFF) $(ASM_SOURCES_MACOSX) lib/accelerated/x86/files.mk

X86_FILES=XXX/aesni-x86.s XXX/cpuid-x86.s XXX/sha1-ssse3-x86.s \
	XXX/sha256-ssse3-x86.s XXX/sha512-ssse3-x86.s XXX/aes-ssse3-x86.s

X86_64_FILES=XXX/aesni-x86_64.s XXX/cpuid-x86_64.s XXX/ghash-x86_64.s \
	XXX/sha1-ssse3-x86_64.s XXX/sha512-ssse3-x86_64.s XXX/aes-ssse3-x86_64.s \
	XXX/aesni-gcm-x86_64.s

X86_PADLOCK_FILES=XXX/e_padlock-x86.s
X86_64_PADLOCK_FILES=XXX/e_padlock-x86_64.s

X86_FILES_ELF := $(subst XXX,elf,$(X86_FILES))
X86_FILES_COFF := $(subst XXX,coff,$(X86_FILES))
X86_FILES_MACOSX := $(subst XXX,macosx,$(X86_FILES))
X86_64_FILES_ELF := $(subst XXX,elf,$(X86_64_FILES))
X86_64_FILES_COFF := $(subst XXX,coff,$(X86_64_FILES))
X86_64_FILES_MACOSX := $(subst XXX,macosx,$(X86_64_FILES))

X86_PADLOCK_FILES_ELF := $(subst XXX,elf,$(X86_PADLOCK_FILES))
X86_PADLOCK_FILES_COFF := $(subst XXX,coff,$(X86_PADLOCK_FILES))
X86_PADLOCK_FILES_MACOSX := $(subst XXX,macosx,$(X86_PADLOCK_FILES))
X86_64_PADLOCK_FILES_ELF := $(subst XXX,elf,$(X86_64_PADLOCK_FILES))
X86_64_PADLOCK_FILES_COFF := $(subst XXX,coff,$(X86_64_PADLOCK_FILES))
X86_64_PADLOCK_FILES_MACOSX := $(subst XXX,macosx,$(X86_64_PADLOCK_FILES))

lib/accelerated/x86/files.mk: $(ASM_SOURCES_ELF)
	echo X86_FILES_ELF=$(X86_FILES_ELF) > $@.tmp
	echo X86_FILES_COFF=$(X86_FILES_COFF) >> $@.tmp
	echo X86_FILES_MACOSX=$(X86_FILES_MACOSX) >> $@.tmp
	echo X86_64_FILES_ELF=$(X86_64_FILES_ELF) >> $@.tmp
	echo X86_64_FILES_COFF=$(X86_64_FILES_COFF) >> $@.tmp
	echo X86_64_FILES_MACOSX=$(X86_64_FILES_MACOSX) >> $@.tmp
	echo X86_PADLOCK_FILES_ELF=$(X86_PADLOCK_FILES_ELF) >> $@.tmp
	echo X86_PADLOCK_FILES_COFF=$(X86_PADLOCK_FILES_COFF) >> $@.tmp
	echo X86_PADLOCK_FILES_MACOSX=$(X86_PADLOCK_FILES_MACOSX) >> $@.tmp
	echo X86_64_PADLOCK_FILES_ELF=$(X86_64_PADLOCK_FILES_ELF) >> $@.tmp
	echo X86_64_PADLOCK_FILES_COFF=$(X86_64_PADLOCK_FILES_COFF) >> $@.tmp
	echo X86_64_PADLOCK_FILES_MACOSX=$(X86_64_PADLOCK_FILES_MACOSX) >> $@.tmp
	mv $@.tmp $@

# Appro's code
lib/accelerated/x86/elf/%.s: devel/perlasm/%.pl .submodule.stamp 
	cat $<.license > $@
	CC=gcc perl $< elf >> $@
	echo "" >> $@
	echo ".section .note.GNU-stack,\"\",%progbits" >> $@
	sed -i 's/OPENSSL_ia32cap_P/_gnutls_x86_cpuid_s/g' $@

lib/accelerated/x86/coff/%-x86.s: devel/perlasm/%-x86.pl .submodule.stamp 
	cat $<.license > $@
	CC=gcc perl $< coff >> $@
	echo "" >> $@
	sed -i 's/OPENSSL_ia32cap_P/_gnutls_x86_cpuid_s/g' $@

lib/accelerated/x86/coff/%-x86_64.s: devel/perlasm/%-x86_64.pl .submodule.stamp 
	cat $<.license > $@
	CC=gcc perl $< mingw64 >> $@
	echo "" >> $@
	sed -i 's/OPENSSL_ia32cap_P/_gnutls_x86_cpuid_s/g' $@

lib/accelerated/x86/macosx/%.s: devel/perlasm/%.pl .submodule.stamp 
	cat $<.license > $@
	CC=gcc perl $< macosx >> $@
	echo "" >> $@
	sed -i 's/OPENSSL_ia32cap_P/_gnutls_x86_cpuid_s/g' $@

lib/accelerated/aarch64/elf/%.s: devel/perlasm/%.pl .submodule.stamp 
	rm -f $@tmp
	CC=aarch64-linux-gnu-gcc perl $< linux64 $@.tmp
	cat $@.tmp | /usr/bin/perl -ne '/^#(line)?\s*[0-9]+/ or print' > $@.tmp.S
	echo "" >> $@.tmp.S
	sed -i 's/OPENSSL_armcap_P/_gnutls_arm_cpuid_s/g' $@.tmp.S
	sed -i 's/arm_arch.h/aarch64-common.h/g' $@.tmp.S
	aarch64-linux-gnu-gcc -D__ARM_MAX_ARCH__=8 -Ilib/accelerated/aarch64 -Wa,--noexecstack -E $@.tmp.S -o $@.tmp.s
	cat $<.license $@.tmp.s > $@
	echo ".section .note.GNU-stack,\"\",%progbits" >> $@
	rm -f $@.tmp.S $@.tmp.s $@.tmp


