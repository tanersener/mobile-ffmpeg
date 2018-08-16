EXTRA_DIST = fribidi.def

lib_LTLIBRARIES = libfribidi.la

AM_CPPFLAGS =

libfribidi_la_LDFLAGS = -no-undefined -version-info $(LT_VERSION_INFO)
libfribidi_la_LIBADD =
libfribidi_la_DEPENDENCIES =

if OS_WIN32
libfribidi_la_LDFLAGS += -export-symbols $(srcdir)/fribidi.def
else
libfribidi_la_LDFLAGS += -export-symbols-regex "^fribidi_.*"
endif # OS_WIN32

include Headers.mk
pkginclude_HEADERS = $(libfribidi_la_headers)
nodist_pkginclude_HEADERS = fribidi-config.h

libfribidi_la_SOURCES =	\
		arabic-misc.tab.i \
		arabic-shaping.tab.i \
		bidi-type.tab.i \
		bidi-types.h \
		common.h \
		debug.h \
		fribidi.c \
		fribidi-arabic.c \
		fribidi-bidi.c \
		fribidi-bidi-types.c \
		fribidi-char-sets.c \
		fribidi-char-sets-cap-rtl.c \
		fribidi-char-sets-cp1255.c \
		fribidi-char-sets-cp1256.c \
		fribidi-char-sets-iso8859-6.c \
		fribidi-char-sets-iso8859-8.c \
		fribidi-char-sets-utf8.c \
		fribidi-char-sets-cap-rtl.h \
		fribidi-char-sets-cp1255.h \
		fribidi-char-sets-cp1256.h \
		fribidi-char-sets-iso8859-6.h \
		fribidi-char-sets-iso8859-8.h \
		fribidi-char-sets-utf8.h \
		fribidi-deprecated.c \
		fribidi-joining.c \
		fribidi-joining-types.c \
		fribidi-mirroring.c \
		fribidi-brackets.c \
		fribidi-run.c \
		fribidi-shape.c \
		joining-type.tab.i \
		joining-types.h \
		mirroring.tab.i \
		brackets.tab.i \
		brackets-type.tab.i \
		run.h

GENERATEDSOURCES = \
		fribidi-unicode-version.h \
		arabic-shaping.tab.i \
		bidi-type.tab.i \
		joining-type.tab.i \
		mirroring.tab.i \
		brackets.tab.i \
		brackets-type.tab.i 

BUILT_SOURCES = \
		$(GENERATEDSOURCES) \
		fribidi-config.h

$(GENERATEDSOURCES):
	@(cd $(top_builddir)/gen.tab && \
	$(MAKE) $(AM_MAKEFLAGS) $@) && \
	(test -f $@ || mv $(top_builddir)/gen.tab/$@ .)

# re-generate all built sources:
gen:
	@(cd $(top_builddir)/gen.tab && \
	$(MAKE) $(AM_MAKEFLAGS) $@) && \
	$(RM) $(BUILT_SOURCES)
	$(MAKE) $(AM_MAKEFLAGS) $(BUILT_SOURCES)

MAINTAINERCLEANFILES = $(BUILT_SOURCES)

$(top_builddir)/config.h: $(top_srcdir)/config.h.in
	cd $(top_builddir) && $(MAKE) $(AM_MAKEFLAGS) config.h

.PHONY: gen

-include $(top_srcdir)/git.mk
