#!/usr/bin/make -f

# Build libsndfile as a dynamic/shared library, but statically link to
# libFLAC, libogg and libvorbis

ogg_version = libogg-1.3.2
ogg_md5sum = 5c3a34309d8b98640827e5d0991a4015

vorbis_version = libvorbis-1.3.5
vorbis_md5sum = 28cb28097c07a735d6af56e598e1c90f

flac_version = flac-1.3.2
flac_md5sum = 454f1bfa3f93cc708098d7890d0499bd

#-------------------------------------------------------------------------------
# Code follows.

ogg_tarball = $(ogg_version).tar.xz
vorbis_tarball = $(vorbis_version).tar.xz
flac_tarball = $(flac_version).tar.xz

download_url = http://downloads.xiph.org/releases/
tarball_dir = Build/Tarballs
stamp_dir = Build/Stamp

build_dir = $(shell pwd)/Build
config_options = --prefix=$(build_dir) --disable-shared

pwd = $(shell pwd)

help :
	@echo
	@echo "This script will build libsndfile as a dynamic/shared library but statically linked"
	@echo "to libFLAC, libogg and libvorbis. It should work on Linux and Mac OS X. It might"
	@echo "work on Windows with a correctly set up MinGW."
	@echo
	@echo "It requires all the normal build tools require to build libsndfile plus wget."
	@echo

config : Build/Stamp/configure

build : Build/Stamp/build

clean :
	rm -rf Build/flac-* Build/libogg-* Build/libvorbis-*
	rm -rf Build/bin Build/include Build/lib Build/share
	rm -f Build/Stamp/install Build/Stamp/extract Build/Stamp/md5sum

Build/Stamp/init :
	mkdir -p $(stamp_dir) $(tarball_dir)
	touch $@

Build/Tarballs/$(flac_tarball) : Build/Stamp/init
	(cd $(tarball_dir) && wget $(download_url)flac/$(flac_tarball) -O $(flac_tarball))
	touch $@

Build/Tarballs/$(ogg_tarball) : Build/Stamp/init
	(cd $(tarball_dir) && wget $(download_url)ogg/$(ogg_tarball) -O $(ogg_tarball))
	touch $@

Build/Tarballs/$(vorbis_tarball) : Build/Stamp/init
	(cd $(tarball_dir) && wget $(download_url)vorbis/$(vorbis_tarball) -O $(vorbis_tarball))
	touch $@

Build/Stamp/tarballs : Build/Tarballs/$(flac_tarball) Build/Tarballs/$(ogg_tarball) Build/Tarballs/$(vorbis_tarball)
	touch $@

Build/Stamp/md5sum : Build/Stamp/tarballs
	test `md5sum $(tarball_dir)/$(ogg_tarball) | sed "s/ .*//"` = $(ogg_md5sum)
	test `md5sum $(tarball_dir)/$(vorbis_tarball) | sed "s/ .*//"` = $(vorbis_md5sum)
	test `md5sum $(tarball_dir)/$(flac_tarball) | sed "s/ .*//"` = $(flac_md5sum)
	touch $@

Build/Stamp/extract : Build/Stamp/md5sum
	(cd Build && tar xf Tarballs/$(ogg_tarball))
	(cd Build && tar xf Tarballs/$(flac_tarball))
	(cd Build && tar xf Tarballs/$(vorbis_tarball))
	touch $@

Build/Stamp/install-libs : Build/Stamp/extract
	(cd Build/$(ogg_version) && CFLAGS=-fPIC ./configure $(config_options) && make all install)
	(cd Build/$(vorbis_version) && CFLAGS=-fPIC ./configure $(config_options) && make all install)
	(cd Build/$(flac_version) && CFLAGS=-fPIC ./configure $(config_options) && make all install)
	touch $@

configure : configure.ac
	./autogen.sh

Build/Stamp/configure : Build/Stamp/install-libs configure
	PKG_CONFIG_LIBDIR=Build/lib/pkgconfig ./configure
	sed -i 's#^EXTERNAL_XIPH_CFLAGS.*#EXTERNAL_XIPH_CFLAGS = -I$(pwd)/Build/include#' src/Makefile
	sed -i 's#^EXTERNAL_XIPH_LIBS.*#EXTERNAL_XIPH_LIBS = -static $(pwd)/Build/lib/libFLAC.la $(pwd)/Build/lib/libogg.la $(pwd)/Build/lib/libvorbis.la $(pwd)/Build/lib/libvorbisenc.la -dynamic #' src/Makefile
	make clean
	touch $@

Build/Stamp/build : Build/Stamp/configure
	make all check
	touch $@

