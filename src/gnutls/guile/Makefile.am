#  GnuTLS --- Guile bindings for GnuTLS.
#  Copyright (C) 2007-2012, 2016 Free Software Foundation, Inc.
#
#  GnuTLS is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Lesser General Public
#  License as published by the Free Software Foundation; either
#  version 2.1 of the License, or (at your option) any later version.
#
#  GnuTLS is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  Lesser General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public
#  License along with GnuTLS; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

# First of all, built the DSO.  We cannot compile the Scheme code until this
# is done.
SUBDIRS = src


EXTRA_DIST = .dir-locals.el

guilesitesubdir = $(guilesitedir)/gnutls

nodist_guilesite_DATA = modules/gnutls.scm
dist_guilesitesub_DATA = modules/gnutls/extra.scm

documentation_modules =				\
  modules/system/documentation/README		\
  modules/system/documentation/c-snarf.scm	\
  modules/system/documentation/output.scm

helper_modules =				\
  modules/gnutls/build/enums.scm		\
  modules/gnutls/build/smobs.scm		\
  modules/gnutls/build/utils.scm		\
  modules/gnutls/build/tests.scm

EXTRA_DIST += modules/gnutls.in $(helper_modules) $(documentation_modules)

CLEANFILES = modules/gnutls.scm

.in.scm:
	$(AM_V_GEN)$(MKDIR_P) "`dirname "$@"`" ; cat "$^" |		\
	  $(SED) -e's|[@]guileextensiondir[@]|$(guileextensiondir)|g'	\
	  > "$@.tmp"
	$(AM_V_at)mv "$@.tmp" "$@"


#
# Scheme code compilation.
#

if HAVE_GUILD

guilesiteccachesubdir = $(guilesiteccachedir)/gnutls
nodist_guilesiteccache_DATA = modules/gnutls.go
nodist_guilesiteccachesub_DATA = modules/gnutls/extra.go

GOBJECTS =					\
  $(nodist_guilesiteccache_DATA)		\
  $(nodist_guilesiteccachesub_DATA)

CLEANFILES += $(GOBJECTS)

AM_V_GUILEC = $(AM_V_GUILEC_$(V))
AM_V_GUILEC_ = $(AM_V_GUILEC_$(AM_DEFAULT_VERBOSITY))
AM_V_GUILEC_0 = @echo "  GUILEC  " $@;

# Make sure 'gnutls.scm' is built first.
# Unset 'GUILE_LOAD_COMPILED_PATH' so we can be sure that any .go file that we
# load comes from the build directory.
# XXX: Use the C locale for when Guile lacks
# <https://git.sv.gnu.org/cgit/guile.git/commit/?h=stable-2.0&id=e2c6bf3866d1186c60bacfbd4fe5037087ee5e3f>.
%.go: %.scm modules/gnutls.scm
	$(AM_V_GUILEC)$(MKDIR_P) "`dirname "$@"`" ;			\
	$(AM_V_P) && out=1 || out=- ;					\
	unset GUILE_LOAD_COMPILED_PATH ; LC_ALL=C			\
	GNUTLS_GUILE_EXTENSION_DIR="$(abs_top_builddir)/guile/src"	\
	$(GUILD) compile --target="$(host)"				\
	  -L "$(top_builddir)/guile/modules"				\
	  -L "$(top_srcdir)/guile/modules"				\
	  -Wformat -Wunbound-variable -Warity-mismatch			\
	  -o "$@" "$<" >&$$out

SUFFIXES = .go

endif HAVE_GUILD


#
# Tests.
#

TESTS =						\
  tests/anonymous-auth.scm			\
  tests/session-record-port.scm			\
  tests/pkcs-import-export.scm			\
  tests/errors.scm				\
  tests/x509-certificates.scm			\
  tests/x509-auth.scm				\
  tests/priorities.scm

if ENABLE_SRP
TESTS +=					\
  tests/srp-base64.scm
endif

TESTS_ENVIRONMENT =				\
  GUILE_AUTO_COMPILE=0				\
  GUILE_WARN_DEPRECATED=detailed

LOG_COMPILER = $(top_builddir)/guile/pre-inst-guile -L $(srcdir)/tests


EXTRA_DIST +=					\
  $(TESTS)					\
  tests/rsa-parameters.pem			\
  tests/dh-parameters.pem			\
  tests/x509-certificate.pem			\
  tests/x509-key.pem
