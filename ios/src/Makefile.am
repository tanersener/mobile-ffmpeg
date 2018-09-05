lib_LTLIBRARIES = libmobileffmpeg.la

libmobileffmpeg_la_LIBADD = @FFMPEG_LIBS@

libmobileffmpeg_la_SOURCES = \
    ArchDetect.m \
    fftools_cmdutils.c \
    fftools_ffmpeg.c \
    fftools_ffmpeg_filter.c \
    fftools_ffmpeg_hw.c \
    fftools_ffmpeg_opt.c \
    MobileFFmpeg.m \
    MobileFFmpegConfig.m \
    mobileffmpeg_exception.m \
    Statistics.m

include_HEADERS = \
    ArchDetect.h \
	fftools_cmdutils.h \
    fftools_ffmpeg.h \
	LogDelegate.h \
    MobileFFmpeg.h \
    MobileFFmpegConfig.h \
	mobileffmpeg_exception.h \
	Statistics.h \
	StatisticsDelegate.h

libmobileffmpeg_la_CFLAGS = $(CFLAGS)
libmobileffmpeg_la_OBJCFLAGS = $(CFLAGS)
libmobileffmpeg_la_CXXFLAGS = $(CXXFLAGS)
libmobileffmpeg_la_LDFLAGS = $(LDFLAGS)
