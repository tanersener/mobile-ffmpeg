## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign 1.6 dist-zip dist-bzip2

if THEORA_ENABLE_EXAMPLES
EXAMPLES_DIR = examples
else
EXAMPLES_DIR =
endif

SUBDIRS = lib include doc tests m4 $(EXAMPLES_DIR)


# we include the whole debian/ dir in EXTRA_DIST because there's a problem
# with autotools and HFS+ MacOSX file systems that caused debian/Makefile.am
# to pick up on the lowercase changelog file and add ChangeLog to DIST_COMMON
# because of it, breaking make dist.  This works just as well.
EXTRA_DIST = \
	README CHANGES COPYING LICENSE \
	autogen.sh win32 macosx symbian SConstruct \
	libtheora.spec libtheora.spec.in \
	theora-uninstalled.pc.in

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = theora.pc theoradec.pc theoraenc.pc

# Remove the .svn folders included in the tarball
dist-hook:
	find $(distdir) -type d -name '.svn' | xargs rm -rf

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"
