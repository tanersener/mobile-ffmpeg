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

.PHONY: config glimport

INDENT_SOURCES = `find . -name \*.[ch] -o -name gnutls.h.in | grep -v -e ^./build-aux/ -e ^./lib/minitasn1/ -e ^./lib/build-aux/ -e ^./gl/ -e ^./src/libopts/ -e -args.[ch] -e asn1_tab.c -e ^./tests/suite/`

ifeq ($(.DEFAULT_GOAL),abort-due-to-no-makefile)
.DEFAULT_GOAL := bootstrap
endif

local-checks-to-skip = sc_GPL_version sc_bindtextdomain			\
	sc_immutable_NEWS sc_program_name sc_prohibit_atoi_atof		\
	sc_prohibit_always_true_header_tests                            \
	sc_prohibit_empty_lines_at_EOF sc_prohibit_hash_without_use	\
	sc_prohibit_have_config_h sc_prohibit_magic_number_exit		\
	sc_prohibit_strcmp sc_require_config_h				\
	sc_require_config_h_first sc_texinfo_acronym sc_trailing_blank	\
	sc_unmarked_diagnostics sc_useless_cpp_parens			\
	sc_two_space_separator_in_usage

VC_LIST_ALWAYS_EXCLUDE_REGEX = ^maint.mk|gtk-doc.make|m4/pkg|doc/fdl-1.3.texi|src/.*\.bak|src/crywrap/|(devel/perlasm/|lib/accelerated/x86/|build-aux/|gl/|src/libopts/|tests/suite/ecore/|doc/protocol/).*$$
update-copyright-env = UPDATE_COPYRIGHT_USE_INTERVALS=1

# Explicit syntax-check exceptions.
exclude_file_name_regexp--sc_error_message_uppercase = ^doc/examples/ex-cxx.cpp|guile/src/core.c|src/certtool.c|src/ocsptool.c|src/crywrap/crywrap.c|tests/pkcs12_encode.c$$
exclude_file_name_regexp--sc_file_system = ^doc/doxygen/Doxyfile
exclude_file_name_regexp--sc_prohibit_cvs_keyword = ^lib/nettle/.*$$
exclude_file_name_regexp--sc_prohibit_undesirable_word_seq = ^tests/nist-pkits/gnutls-nist-tests.html$$
exclude_file_name_regexp--sc_space_tab = ^doc/.*.(pdf|png)|\.crl|\.pdf|\.zip|tests/nist-pkits/|tests/data/|tests/system-override-curves.sh|devel/|tests/suite/x509paths/.*|fuzz/.*\.repro|fuzz/.*\.in/.*$$
_makefile_at_at_check_exceptions = ' && !/CODE_COVERAGE_RULES/ && !/VERSION/'
exclude_file_name_regexp--sc_m4_quote_check='lib/unistring/m4/absolute-header.m4'
exclude_file_name_regexp--sc_makefile_at_at_check='lib/unistring/Makefile.am'
exclude_file_name_regexp--sc_prohibit_stddef_without_use='u*-normalize.c'
exclude_file_name_regexp--sc_prohibit_strncpy='unistr.in.h'
exclude_file_name_regexp--sc_prohibit_strncpy='lib/inih/ini.c'
gl_public_submodule_commit =

autoreconf:
	./bootstrap

config:
	./configure $(CFGFLAGS)

.submodule.stamp:
	git submodule init
	git submodule update
	touch $@

bootstrap: autoreconf .submodule.stamp

glimport:
	pushd gnulib && git checkout master && git pull && popd
	echo "If everything looks well, commit the gnulib update with:"
	echo "  git commit -m "Update gnulib submodule" gnulib"

# Code Coverage

clang:
	$(MAKE) clean
	scan-build ./configure
	rm -rf scan.tmp
	scan-build -o scan.tmp $(MAKE)

clang-copy:
	rm -fv `find $(htmldir)/clang -type f | grep -v CVS`
	mkdir -p $(htmldir)/clang/
	cp -rv scan.tmp/*/* $(htmldir)/clang/

# Release

# ChangeLog must be PHONY, else it isn't generated on 'make distcheck'
.PHONY: ChangeLog
ChangeLog:
	(cd "$(srcdir)" ; if test -d .git ; then git log --no-merges --no-decorate --pretty  --since="2014 November 07"|grep -v ^'commit' ; else echo "Empty" ; fi) > ChangeLog

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
	$(MAKE) -C doc gnutls.html
	cd doc && cp gnutls.html *.png ../$(htmldir)/manual/
	cd doc && makeinfo --html --split=node -o ../$(htmldir)/manual/html_node/ --css-include=./texinfo.css gnutls.texi
	cd doc && cp *.png ../$(htmldir)/manual/html_node/
	sed 's/\@VERSION\@/$(VERSION)/g' -i $(htmldir)/manual/html_node/*.html $(htmldir)/manual/gnutls.html
	-cd doc && $(MAKE) gnutls.epub && cp gnutls.epub ../$(htmldir)/manual/
	cd doc/latex && $(MAKE) gnutls.pdf && cp gnutls.pdf ../../$(htmldir)/manual/
	$(MAKE) -C doc gnutls-guile.html gnutls-guile.pdf
	cd doc && makeinfo --html --split=node -o ../$(htmldir)/manual/gnutls-guile/ --css-include=./texinfo.css gnutls-guile.texi
	cd doc && cp gnutls-guile.pdf gnutls-guile.html ../$(htmldir)/manual/
	-cp -v doc/reference/html/*.html doc/reference/html/*.png doc/reference/html/*.devhelp* doc/reference/html/*.css $(htmldir)/reference/

ASM_SOURCES_XXX := \
	lib/accelerated/aarch64/XXX/ghash-aarch64.s \
	lib/accelerated/aarch64/XXX/aes-aarch64.s \
	lib/accelerated/aarch64/XXX/sha1-armv8.s \
	lib/accelerated/aarch64/XXX/sha256-armv8.s \
	lib/accelerated/aarch64/XXX/sha512-armv8.s \
	lib/accelerated/x86/XXX/ghash-x86_64.s \
	lib/accelerated/x86/XXX/aesni-x86_64.s \
	lib/accelerated/x86/XXX/aesni-x86.s \
	lib/accelerated/x86/XXX/sha1-ssse3-x86.s \
	lib/accelerated/x86/XXX/sha1-ssse3-x86_64.s \
	lib/accelerated/x86/XXX/sha256-ssse3-x86.s \
	lib/accelerated/x86/XXX/sha256-ssse3-x86_64.s \
	lib/accelerated/x86/XXX/sha512-ssse3-x86.s \
	lib/accelerated/x86/XXX/sha512-ssse3-x86_64.s \
	lib/accelerated/x86/XXX/aesni-gcm-x86_64.s \
	lib/accelerated/x86/XXX/aes-ssse3-x86.s \
	lib/accelerated/x86/XXX/aes-ssse3-x86_64.s

# CRYPTOGAMS' perl-scripts can produce different output if -fPIC
# is passed as option. List the files that seem to need it:
PL_NEEDS_FPIC := aesni-x86.pl aes-ssse3-x86.pl e_padlock-x86.pl \
	ghash-x86.pl sha1-ssse3-x86.pl sha256-ssse3-x86.pl \
	sha512-ssse3-x86.pl

ASM_SOURCES_ELF := $(subst XXX,elf,$(ASM_SOURCES_XXX))
ASM_SOURCES_COFF := $(subst XXX,coff,$(ASM_SOURCES_XXX))
ASM_SOURCES_MACOSX := $(subst XXX,macosx,$(ASM_SOURCES_XXX))

asm-sources: $(ASM_SOURCES_ELF) $(ASM_SOURCES_COFF) $(ASM_SOURCES_MACOSX) lib/accelerated/x86/files.mk

asm-sources-clean:
	rm -f $(ASM_SOURCES_ELF) $(ASM_SOURCES_COFF) $(ASM_SOURCES_MACOSX) lib/accelerated/x86/files.mk

X86_FILES=XXX/aesni-x86.s XXX/sha1-ssse3-x86.s \
	XXX/sha256-ssse3-x86.s XXX/sha512-ssse3-x86.s XXX/aes-ssse3-x86.s

X86_64_FILES=XXX/aesni-x86_64.s XXX/ghash-x86_64.s \
	XXX/sha1-ssse3-x86_64.s XXX/sha512-ssse3-x86_64.s XXX/aes-ssse3-x86_64.s \
	XXX/aesni-gcm-x86_64.s XXX/sha256-ssse3-x86_64.s

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
	CC=gcc perl $< elf \
		$(if $(findstring $(<F),$(PL_NEEDS_FPIC)),-fPIC) \
		$@.tmp
	cat $<.license $@.tmp > $@ && rm -f $@.tmp
	echo "" >> $@
	echo ".section .note.GNU-stack,\"\",%progbits" >> $@
	sed -i 's/OPENSSL_ia32cap_P/_gnutls_x86_cpuid_s/g' $@

lib/accelerated/x86/coff/%-x86.s: devel/perlasm/%-x86.pl .submodule.stamp 
	CC=gcc perl $< coff \
		$(if $(findstring $(<F),$(PL_NEEDS_FPIC)),-fPIC) \
		$@.tmp
	cat $<.license $@.tmp > $@ && rm -f $@.tmp
	echo "" >> $@
	sed -i 's/OPENSSL_ia32cap_P/_gnutls_x86_cpuid_s/g' $@

lib/accelerated/x86/coff/%-x86_64.s: devel/perlasm/%-x86_64.pl .submodule.stamp 
	CC=gcc perl $< mingw64 \
		$(if $(findstring $(<F),$(PL_NEEDS_FPIC)),-fPIC) \
		$@.tmp
	cat $<.license $@.tmp > $@ && rm -f $@.tmp
	echo "" >> $@
	sed -i 's/OPENSSL_ia32cap_P/_gnutls_x86_cpuid_s/g' $@

lib/accelerated/x86/macosx/%.s: devel/perlasm/%.pl .submodule.stamp 
	CC=gcc perl $< macosx \
		$(if $(findstring $(<F),$(PL_NEEDS_FPIC)),-fPIC) \
		$@.tmp
	cat $<.license $@.tmp > $@ && rm -f $@.tmp
	echo "" >> $@
	sed -i 's/OPENSSL_ia32cap_P/_gnutls_x86_cpuid_s/g' $@

lib/accelerated/aarch64/elf/%.s: devel/perlasm/%.pl .submodule.stamp 
	rm -f $@tmp
	CC=aarch64-linux-gnu-gcc perl $< linux64 \
		$(if $(findstring $(<F),$(PL_NEEDS_FPIC)),-fPIC) \
		$@.tmp
	cat $@.tmp | /usr/bin/perl -ne '/^#(line)?\s*[0-9]+/ or print' > $@.tmp.S
	echo "" >> $@.tmp.S
	sed -i 's/OPENSSL_armcap_P/_gnutls_arm_cpuid_s/g' $@.tmp.S
	sed -i 's/arm_arch.h/aarch64-common.h/g' $@.tmp.S
	aarch64-linux-gnu-gcc -D__ARM_MAX_ARCH__=8 -Ilib/accelerated/aarch64 -Wa,--noexecstack -E $@.tmp.S -o $@.tmp.s
	cat $<.license $@.tmp.s > $@
	echo ".section .note.GNU-stack,\"\",%progbits" >> $@
	rm -f $@.tmp.S $@.tmp.s $@.tmp

lib/accelerated/aarch64/macosx/%.s: devel/perlasm/%.pl .submodule.stamp
	rm -f $@tmp
	CC=aarch64-linux-gnu-gcc perl $< ios64 \
		$(if $(findstring $(<F),$(PL_NEEDS_FPIC)),-fPIC) \
		$@.tmp
	cat $@.tmp | /usr/bin/perl -ne '/^#(line)?\s*[0-9]+/ or print' > $@.tmp.S
	echo "" >> $@.tmp.S
	sed -i 's/OPENSSL_armcap_P/_gnutls_arm_cpuid_s/g' $@.tmp.S
	sed -i 's/arm_arch.h/aarch64-common.h/g' $@.tmp.S
	aarch64-linux-gnu-gcc -D__ARM_MAX_ARCH__=8 -Ilib/accelerated/aarch64 -Wa,--noexecstack -E $@.tmp.S -o $@.tmp.s
	cat $<.license $@.tmp.s > $@
	rm -f $@.tmp.S $@.tmp.s $@.tmp

lib/accelerated/aarch64/coff/%.s: devel/perlasm/%.pl .submodule.stamp
	@true
