lib_LTLIBRARIES = libwavpack.la

libwavpack_la_SOURCES = \
	common_utils.c \
	decorr_utils.c \
	entropy_utils.c \
	extra1.c \
	extra2.c \
	open_utils.c \
	open_filename.c \
	open_legacy.c \
	open_raw.c \
	pack.c \
	pack_dns.c \
	pack_floats.c \
	pack_utils.c \
	read_words.c \
	tags.c \
	tag_utils.c \
	unpack.c \
	unpack_floats.c \
	unpack_seek.c \
	unpack_utils.c \
	write_words.c

if ENABLE_LEGACY
libwavpack_la_SOURCES += unpack3.c unpack3_open.c unpack3_seek.c
endif

if ENABLE_DSD
libwavpack_la_SOURCES += pack_dsd.c unpack_dsd.c
endif

if ENABLE_X86ASM
libwavpack_la_SOURCES += pack_x86.S unpack_x86.S
endif

if ENABLE_X64ASM
libwavpack_la_SOURCES += pack_x64.S unpack_x64.S
endif

if ENABLE_ARMASM
libwavpack_la_SOURCES += unpack_armv7.S
endif

noinst_HEADERS = \
	decorr_tables.h \
	unpack3.h \
	wavpack_local.h \
	wavpack_version.h

libwavpack_la_CFLAGS = $(AM_CFLAGS)
libwavpack_la_LIBADD = $(AM_LDADD) $(LIBM)
libwavpack_la_LDFLAGS = -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) -export-symbols-regex '^Wavpack.*$$' -no-undefined

MAINTAINERCLEANFILES = \
	Makefile.in
