
AM_CFLAGS = -I$(top_srcdir)/build/ -I$(top_srcdir)/libtwolame/ $(SNDFILE_CFLAGS) $(WARNING_CFLAGS)

bin_PROGRAMS = @TWOLAME_BIN@
EXTRA_PROGRAMS = twolame

twolame_SOURCES = frontend.c frontend.h
twolame_LDADD = $(top_builddir)/libtwolame/libtwolame.la $(SNDFILE_LIBS)
