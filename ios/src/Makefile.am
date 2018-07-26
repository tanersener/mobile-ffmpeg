lib_LTLIBRARIES = libmobileffmpeg.la

libmobileffmpeg_la_LIBADD = @FFMPEG_LIBS@

libmobileffmpeg_la_SOURCES = \
    ArchDetect.m \
    cmdutils.c \
    exception.m \
    ffmpeg.c \
    ffmpeg_filter.c \
    ffmpeg_hw.c \
    ffmpeg_opt.c \
    Log.m \
    MobileFFmpeg.m

include_HEADERS = \
    ArchDetect.h \
    ffmpeg.h \
    MobileFFmpeg.h \
	cmdutils.h \
	exception.h \
	LogDelegate.h \
	Log.h

libmobileffmpeg_la_CFLAGS = $(CFLAGS)
libmobileffmpeg_la_OBJCFLAGS = $(CFLAGS)
libmobileffmpeg_la_CXXFLAGS = $(CXXFLAGS)
libmobileffmpeg_la_LDFLAGS = $(LDFLAGS)
