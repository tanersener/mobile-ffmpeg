.PHONY : android

EXTRA_DIST = ChangeLog src/lib/*.h src/bin/*.h README* libshine.sym js

AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS  = -I m4

CFLAGS = @CFLAGS@ -funroll-loops -fno-exceptions -Wall -O2 -fsigned-char

lib_LTLIBRARIES      = libshine.la
libshine_la_SOURCES  = src/lib/bitstream.c src/lib/huffman.c \
                       src/lib/l3bitstream.c src/lib/l3loop.c src/lib/l3mdct.c \
                       src/lib/l3subband.c src/lib/layer3.c src/lib/reservoir.c \
                       src/lib/tables.c

libshine_la_LDFLAGS  = -lm -no-undefined -version-info 3:1:0 -export-symbols libshine.sym
libshine_ladir       = ${prefix}/include/shine
libshine_la_HEADERS  = src/lib/layer3.h

bin_PROGRAMS     = shineenc
shineenc_SOURCES = src/bin/main.c src/bin/wave.c
shineenc_LDADD   = libshine.la
shineenc_CFLAGS  = -Isrc/lib

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = shine.pc

android:
	[ -d android/bin ]     || mkdir android/bin
	[ -d android/include ] || mkdir android/include
	[ -d android/lib ]     || mkdir android/lib
	NDK_PROJECT_PATH=$(shell pwd) ndk-build APP_BUILD_SCRIPT=$(shell pwd)/android/Library.mk
	cp libs/armeabi/libshine.so android/lib
	cp src/lib/layer3.h android/include
	NDK_PROJECT_PATH=$(shell pwd) ndk-build APP_BUILD_SCRIPT=$(shell pwd)/android/Binary.mk
	cp libs/armeabi/shineenc android/bin

clean-local:
	-rm -rf obj libs android/lib android/bin android/include
