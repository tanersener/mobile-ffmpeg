AM_CFLAGS = -Wall

noinst_PROGRAMS = test
test_SOURCES = test.c
test_CPPFLAGS = -I$(top_srcdir)/libass
test_LDADD = $(top_builddir)/libass/.libs/libass.a
test_LDFLAGS = $(AM_LDFLAGS) $(LIBPNG_LIBS) -static
