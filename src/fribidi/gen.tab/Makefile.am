EXTRA_PROGRAMS = \
		gen-unicode-version \
		gen-bidi-type-tab \
		gen-joining-type-tab \
		gen-arabic-shaping-tab \
		gen-mirroring-tab \
		gen-brackets-tab \
                gen-brackets-type-tab 

gen_unicode_version_SOURCES = gen-unicode-version.c
gen_arabic_shaping_tab_SOURCES = gen-arabic-shaping-tab.c
gen_bidi_type_tab_SOURCES = gen-bidi-type-tab.c packtab.c packtab.h
gen_joining_type_tab_SOURCES = gen-joining-type-tab.c packtab.c packtab.h
gen_mirroring_tab_SOURCES = gen-mirroring-tab.c packtab.c packtab.h
gen_brackets_tab_SOURCES = gen-brackets-tab.c packtab.c packtab.h
gen_brackets_type_tab_SOURCES = gen-brackets-type-tab.c packtab.c packtab.h
# Set CPPFLAGS explicitly for these, such that they compile packtab.c to private
# packtab.o so they don't race during parallel build from lib.
# https://github.com/fribidi/fribidi/issues/52
gen_bidi_type_tab_CPPFLAGS = $(AM_CPPFLAGS)
gen_joining_type_tab_CPPFLAGS = $(AM_CPPFLAGS)
gen_mirroring_tab_CPPFLAGS = $(AM_CPPFLAGS)
gen_brackets_tab_CPPFLAGS = $(AM_CPPFLAGS)
gen_brackets_type_tab_CPPFLAGS = $(AM_CPPFLAGS)

CFLAGS_FOR_BUILD += -DHAVE_CONFIG_H -I$(top_builddir) -I$(top_builddir)/lib -I$(top_srcdir)/lib
CC = $(CC_FOR_BUILD)
CFLAGS = $(CFLAGS_FOR_BUILD)
LDFLAGS = $(LDFLAGS_FOR_BUILD)

CLEANFILES = $(EXTRA_PROGRAMS)
DISTCLEANFILES =

AM_CPPFLAGS = \
		-I$(top_builddir)/lib \
		-I$(top_srcdir)/lib

UCD_FILES = \
		unidata/ReadMe.txt \
		unidata/UnicodeData.txt \
		unidata/BidiMirroring.txt \
		unidata/BidiBrackets.txt \
		unidata/ArabicShaping.txt

EXTRA_DIST = $(UCD_FILES)

COMPRESSION = 2

# generate bidi-type.tab.i

gen_bidi_type_tab = gen-bidi-type-tab$(EXEEXT)

bidi-type.tab.i: \
		unidata/UnicodeData.txt \
		fribidi-unicode-version.h \
		$(gen_bidi_type_tab)
	$(AM_V_GEN) ./$(gen_bidi_type_tab) $(COMPRESSION) $^ > $@ \
	|| ($(RM) $@ && false)

DISTCLEANFILES += derived_bidi-type.tab.i

# generate joining-type.tab.i

gen_joining_type_tab = gen-joining-type-tab$(EXEEXT)

joining-type.tab.i: \
		unidata/UnicodeData.txt \
		unidata/ArabicShaping.txt \
		fribidi-unicode-version.h \
		$(gen_joining_type_tab)
	$(AM_V_GEN) ./$(gen_joining_type_tab) $(COMPRESSION) $^ > $@ \
	|| ($(RM) $@ && false)

# generate arabic-shaping.tab.i

gen_arabic_shaping_tab = gen-arabic-shaping-tab$(EXEEXT)

arabic-shaping.tab.i: \
		unidata/UnicodeData.txt \
		fribidi-unicode-version.h \
		$(gen_arabic_shaping_tab)
	$(AM_V_GEN) ./$(gen_arabic_shaping_tab) $(COMPRESSION) $^ > $@ \
	|| ($(RM) $@ && false)

# generate mirroring.tab.i

gen_mirroring_tab = gen-mirroring-tab$(EXEEXT)

mirroring.tab.i: \
		unidata/BidiMirroring.txt \
		fribidi-unicode-version.h \
		$(gen_mirroring_tab)
	$(AM_V_GEN) ./$(gen_mirroring_tab) $(COMPRESSION) $^ > $@ \
	|| ($(RM) $@ && false)

# generate brackets.tab.i

gen_brackets_tab = gen-brackets-tab$(EXEEXT)

brackets.tab.i: \
		unidata/BidiBrackets.txt \
		unidata/UnicodeData.txt \
		fribidi-unicode-version.h \
		$(gen_brackets_tab)
	$(AM_V_GEN) ./$(gen_brackets_tab) $(COMPRESSION) $^ > $@ \
	|| ($(RM) $@ && false)

# generate brackets-type.tab.i

gen_brackets_type_tab = gen-brackets-type-tab$(EXEEXT)

brackets-type.tab.i: \
		unidata/BidiBrackets.txt \
		fribidi-unicode-version.h \
		$(gen_brackets_type_tab)
	$(AM_V_GEN) ./$(gen_brackets_type_tab) $(COMPRESSION) $^ > $@ \
	|| ($(RM) $@ && false)

# generate fribidi-unicode-version.h

gen_unicode_version = gen-unicode-version$(EXEEXT)

fribidi-unicode-version.h: \
		unidata/ReadMe.txt \
		unidata/BidiMirroring.txt \
		$(gen_unicode_version)
	$(AM_V_GEN) ./$(gen_unicode_version) $^ > $@ \
	|| ($(RM) $@ && false)

# generate all generators:
gen: $(EXTRA_PROGRAMS)

$(top_builddir)/config.h: $(top_srcdir)/config.h.in
	cd $(top_builddir) && $(MAKE) $(AM_MAKEFLAGS) config.h

$(top_builddir)/lib/fribidi-config.h: $(top_srcdir)/lib/fribidi-config.h.in
	cd $(top_builddir)/lib && $(MAKE) $(AM_MAKEFLAGS) fribidi-config.h

.PHONY: gen

.DELETE_ON_ERROR:

-include $(top_srcdir)/git.mk
