bin_PROGRAMS = wavpack wvunpack wvgain wvtag

wavpack_SOURCES = wavpack.c riff.c wave64.c caff.c dsdiff.c dsf.c utils.c md5.c import_id3.c
if WINDOWS_HOST
wavpack_SOURCES += win32_unicode_support.c
endif
wavpack_CFLAGS = $(AM_CFLAGS) $(ICONV_CFLAGS) -I$(top_srcdir)/include
if ENABLE_RPATH
wavpack_LDFLAGS = -rpath $(libdir)
endif
wavpack_LDADD = $(AM_LDADD) $(top_builddir)/src/libwavpack.la $(LIBM) $(ICONV_LIBS)

wvunpack_SOURCES = wvunpack.c riff_write.c wave64_write.c caff_write.c dsdiff_write.c dsf_write.c utils.c md5.c
if WINDOWS_HOST
wvunpack_SOURCES += win32_unicode_support.c
endif
wvunpack_CFLAGS = $(AM_CFLAGS) $(ICONV_CFLAGS) -I$(top_srcdir)/include
if ENABLE_RPATH
wvunpack_LDFLAGS = -rpath $(libdir)
endif
wvunpack_LDADD = $(AM_LDADD) $(top_builddir)/src/libwavpack.la $(LIBM) $(ICONV_LIBS)

wvgain_SOURCES = wvgain.c utils.c
if WINDOWS_HOST
wvgain_SOURCES += win32_unicode_support.c
endif
wvgain_CFLAGS = $(AM_CFLAGS) -I$(top_srcdir)/include
if ENABLE_RPATH
wvgain_LDFLAGS = -rpath $(libdir)
endif
wvgain_LDADD = $(AM_LDADD) $(top_builddir)/src/libwavpack.la $(LIBM)

wvtag_SOURCES = wvtag.c utils.c import_id3.c
if WINDOWS_HOST
wvtag_SOURCES += win32_unicode_support.c
endif
wvtag_CFLAGS = $(AM_CFLAGS) -I$(top_srcdir)/include
if ENABLE_RPATH
wvtag_LDFLAGS = -rpath $(libdir)
endif
wvtag_LDADD = $(AM_LDADD) $(top_builddir)/src/libwavpack.la $(LIBM) $(ICONV_LIBS)

if ENABLE_TESTS
bin_PROGRAMS += wvtest
wvtest_SOURCES = wvtest.c md5.c
wvtest_CFLAGS = $(AM_CFLAGS) -I$(top_srcdir)/include
if ENABLE_RPATH
wvtest_LDFLAGS = -rpath $(libdir)
endif
wvtest_LDADD = $(AM_LDADD) $(top_builddir)/src/libwavpack.la $(LIBM) -lpthread
endif

noinst_HEADERS = \
	win32_unicode_support.h \
	utils.h \
	md5.h

MAINTAINERCLEANFILES = \
	Makefile.in
