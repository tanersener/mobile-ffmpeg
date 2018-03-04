AM_CFLAGS = -Wall

noinst_PROGRAMS = profile
profile_SOURCES = profile.c
profile_CPPFLAGS = -I$(top_srcdir)/libass
profile_LDADD = $(top_builddir)/libass/.libs/libass.a
profile_LDFLAGS = $(AM_LDFLAGS) -static
