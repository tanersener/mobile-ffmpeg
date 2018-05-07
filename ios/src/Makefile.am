lib_LTLIBRARIES = libmobileffmpeg.la

libmobileffmpeg_la_LIBADD = @FFMPEG_LIBS@

libmobileffmpeg_la_SOURCES = \
    archdetect.m \
    cmdutils.c \
    exception.c \
    ffmpeg.c \
    ffmpeg_filter.c \
    ffmpeg_hw.c \
    ffmpeg_opt.c \
    log_thread.c \
    log.m \
    mobileffmpeg.c

include_HEADERS = \
    archdetect.h \
    ffmpeg.h \
    mobileffmpeg.h \
	cmdutils.h \
	exception.h \
	log.h

libmobileffmpeg_la_CFLAGS = $(CFLAGS)
libmobileffmpeg_la_OBJCFLAGS = $(CFLAGS)
libmobileffmpeg_la_CXXFLAGS = $(CXXFLAGS)
libmobileffmpeg_la_LDFLAGS = $(LDFLAGS)
