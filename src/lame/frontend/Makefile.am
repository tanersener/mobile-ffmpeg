## $Id: Makefile.am,v 1.35 2011/10/16 13:09:01 robert Exp $

include $(top_srcdir)/Makefile.am.global

bin_PROGRAMS = @WITH_FRONTEND@ @WITH_MP3RTP@ @WITH_MP3X@
EXTRA_PROGRAMS = lame$(EXEEXT) mp3rtp$(EXEEXT) mp3x$(EXEEXT)

EXTRA_DIST = \
	amiga_mpega.c

DEFS = @DEFS@ @CONFIG_DEFS@

common_sources = \
	main.c \
	brhist.c \
	console.c \
	get_audio.c \
	lametime.c \
	parse.c \
	timestatus.c

noinst_HEADERS = \
	brhist.h \
	console.h \
	get_audio.h \
	gtkanal.h \
	gpkplotting.h \
	lametime.h \
	main.h \
	parse.h \
	rtp.h \
	timestatus.h

lame_SOURCES = lame_main.c $(common_sources)
mp3rtp_SOURCES = mp3rtp.c rtp.c $(common_sources)
mp3x_SOURCES = mp3x.c gtkanal.c gpkplotting.c $(common_sources)

CFLAGS = @CFLAGS@ @GTK_CFLAGS@ @FRONTEND_CFLAGS@ @SNDFILE_CFLAGS@
LDFLAGS = @LDFLAGS@ @FRONTEND_LDFLAGS@ @SNDFILE_LIBS@

INCLUDES = -I$(top_srcdir)/libmp3lame -I$(top_srcdir)/include -I$(top_builddir)

LDADD = @LDADD@ \
	$(top_builddir)/libmp3lame/libmp3lame.la \
	@FRONTEND_LDADD@

mp3x_LDADD = $(LDADD) @GTK_LIBS@

CLEANFILES = lclint.txt $(EXTRA_PROGRAMS)

LCLINTFLAGS= \
	+posixlib \
	+showsummary \
	+showalluses \
	+whichlib \
	+forcehints \
	-fixedformalarray \
	+matchanyintegral \
	-Dlint

lclint.txt: ${lame_SOURCES} ${noinst_HEADERS}
	@lclint ${LCLINTFLAGS} ${INCLUDES} ${DEFS} ${lame_SOURCES} 2>&1 >lclint.txt || true

lclint: lclint.txt
	more lclint.txt
