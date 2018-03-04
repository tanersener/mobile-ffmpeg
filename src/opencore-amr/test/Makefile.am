
bin_PROGRAMS = amrwb-dec$(EXEEXT)
noinst_PROGRAMS = linkboth$(EXEEXT)
AM_CFLAGS =

if AMRNB_DECODER
    bin_PROGRAMS += amrnb-dec$(EXEEXT)
else
    AM_CFLAGS += -DDISABLE_AMRNB_DECODER
endif
if AMRNB_ENCODER
    bin_PROGRAMS += amrnb-enc$(EXEEXT)
    noinst_PROGRAMS += amrnb-enc-sine$(EXEEXT)
else
    AM_CFLAGS += -DDISABLE_AMRNB_ENCODER
endif

INCLUDES = -I$(top_srcdir)/amrnb -I$(top_srcdir)/amrwb

amrnb_dec_LDADD = $(top_builddir)/amrnb/libopencore-amrnb.la
amrnb_enc_LDADD = $(top_builddir)/amrnb/libopencore-amrnb.la
amrnb_enc_sine_LDADD = $(top_builddir)/amrnb/libopencore-amrnb.la
amrnb_enc_sine_LDFLAGS = -lm
amrwb_dec_LDADD = $(top_builddir)/amrwb/libopencore-amrwb.la
linkboth_LDFLAGS = -static
linkboth_LDADD = $(top_builddir)/amrnb/libopencore-amrnb.la $(top_builddir)/amrwb/libopencore-amrwb.la

amrnb_dec_SOURCES = amrnb-dec.c wavwriter.c
amrnb_enc_SOURCES = amrnb-enc.c wavreader.c
amrnb_enc_sine_SOURCES = amrnb-enc-sine.c
amrwb_dec_SOURCES = amrwb-dec.c wavwriter.c
linkboth_SOURCES = linkboth.c

noinst_HEADERS = wavwriter.h wavreader.h

