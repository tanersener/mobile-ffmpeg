#!/bin/bash

android_get_target_machine() {
    case ${ARCH} in
        arm)
            echo "armv7-a"
        ;;
        arm64)
            echo "aarch64"
        ;;
        x86)
            echo "i686"
        ;;
        x86_64)
            echo "x86_64"
        ;;
    esac
}

if [[ -z $1 ]]; then
    echo "usage: $0 <mobile ffmpeg base directory>"
    exit 1
fi

if [[ -z ${ANDROID_NDK_ROOT} ]]; then
    echo "ANDROID_NDK_ROOT not defined"
    exit 1
fi

if [[ -z ${ARCH} ]]; then
    echo "ARCH not defined"
    exit 1
fi

if [[ -z ${API} ]]; then
    echo "API not defined"
    exit 1
fi

HOST_PKG_CONFIG_PATH=`type pkg-config 2>/dev/null | sed 's/.*is //g'`
if [ -z ${HOST_PKG_CONFIG_PATH} ]; then
    echo "pkg-config not found"
    exit 1
fi

# ENABLE COMMON FUNCTIONS
. $1/build/common.sh

# PREPARING PATHS
android_prepare_toolchain_paths

# PREPARING FLAGS
TARGET_HOST=$(android_get_target_host)
TARGET_MACHINE=$(android_get_target_machine)
COMMON_CFLAGS=$(android_get_cflags "ffmpeg")
COMMON_LDFLAGS=$(android_get_ldflags "ffmpeg")
export PKG_CONFIG_PATH="${INSTALL_PKG_CONFIG_DIR}"

export CFLAGS="${COMMON_CFLAGS} \
$(${HOST_PKG_CONFIG_PATH} --cflags gmp) \
$(${HOST_PKG_CONFIG_PATH} --cflags gnutls) \
$(${HOST_PKG_CONFIG_PATH} --cflags libiconv) \
$(${HOST_PKG_CONFIG_PATH} --cflags fontconfig) \
$(${HOST_PKG_CONFIG_PATH} --cflags freetype2) \
$(${HOST_PKG_CONFIG_PATH} --cflags fribidi) \
$(${HOST_PKG_CONFIG_PATH} --cflags libass) \
$(${HOST_PKG_CONFIG_PATH} --cflags libmp3lame) \
$(${HOST_PKG_CONFIG_PATH} --cflags libpng) \
$(${HOST_PKG_CONFIG_PATH} --cflags libwebp) \
$(${HOST_PKG_CONFIG_PATH} --cflags libxml-2.0) \
$(${HOST_PKG_CONFIG_PATH} --cflags opencore-amrnb) \
$(${HOST_PKG_CONFIG_PATH} --cflags opencore-amrwb) \
$(${HOST_PKG_CONFIG_PATH} --cflags shine) \
$(${HOST_PKG_CONFIG_PATH} --cflags soxr) \
$(${HOST_PKG_CONFIG_PATH} --cflags speex) \
$(${HOST_PKG_CONFIG_PATH} --cflags theora) \
$(${HOST_PKG_CONFIG_PATH} --cflags uuid) \
$(${HOST_PKG_CONFIG_PATH} --cflags vorbis) \
$(${HOST_PKG_CONFIG_PATH} --cflags vpx) \
$(${HOST_PKG_CONFIG_PATH} --cflags wavpack) \
$(${HOST_PKG_CONFIG_PATH} --cflags zlib) \
";

export CXXFLAGS=$(android_get_cxxflags "ffmpeg")

export LDFLAGS="${COMMON_LDFLAGS} \
$(${HOST_PKG_CONFIG_PATH} --libs --static gmp) \
$(${HOST_PKG_CONFIG_PATH} --libs --static gnutls) \
$(${HOST_PKG_CONFIG_PATH} --libs --static libiconv) \
$(${HOST_PKG_CONFIG_PATH} --libs --static fontconfig) \
$(${HOST_PKG_CONFIG_PATH} --libs --static freetype2) \
$(${HOST_PKG_CONFIG_PATH} --libs --static fribidi) \
$(${HOST_PKG_CONFIG_PATH} --libs --static libass) \
$(${HOST_PKG_CONFIG_PATH} --libs --static libmp3lame) \
$(${HOST_PKG_CONFIG_PATH} --libs --static libpng) \
$(${HOST_PKG_CONFIG_PATH} --libs --static libwebp) \
$(${HOST_PKG_CONFIG_PATH} --libs --static libxml-2.0) \
$(${HOST_PKG_CONFIG_PATH} --libs --static opencore-amrnb) \
$(${HOST_PKG_CONFIG_PATH} --libs --static opencore-amrwb) \
$(${HOST_PKG_CONFIG_PATH} --libs --static shine) \
$(${HOST_PKG_CONFIG_PATH} --libs --static soxr) \
$(${HOST_PKG_CONFIG_PATH} --libs --static speex) \
$(${HOST_PKG_CONFIG_PATH} --libs --static theora) \
$(${HOST_PKG_CONFIG_PATH} --libs --static uuid) \
$(${HOST_PKG_CONFIG_PATH} --libs --static vorbis) \
$(${HOST_PKG_CONFIG_PATH} --libs vpx) \
$(${HOST_PKG_CONFIG_PATH} --libs --static wavpack) \
$(${HOST_PKG_CONFIG_PATH} --libs --static zlib) \
-L${ANDROID_NDK_ROOT}/platforms/android-${API}/arch-${ARCH}/usr/lib -landroid"

cd $1/src/ffmpeg || exit 1

make clean

./configure \
    --cross-prefix=${TARGET_HOST}- \
    --sysroot=${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-${ARCH}/sysroot \
    --prefix=${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/ffmpeg \
    --pkgconfigdir="${INSTALL_PKG_CONFIG_DIR}" \
    --pkg-config="${HOST_PKG_CONFIG_PATH}" \
    --extra-cflags="${CFLAGS}" \
    --extra-cxxflags="${CXXFLAGS}" \
    --extra-ldflags="${LDFLAGS}" \
    --enable-version3 \
    --arch=${ARCH} \
    --cpu=${TARGET_MACHINE} \
    --target-os=android \
	--enable-cross-compile \
    --enable-pic \
    --enable-asm \
    --enable-jni \
    --enable-avresample \
    --disable-xmm-clobber-test \
    --disable-debug \
    --disable-neon-clobber-test \
    --disable-ffmpeg \
    --disable-ffplay \
    --disable-ffprobe \
    --disable-ffserver \
    --disable-doc \
    --disable-htmlpages \
    --disable-manpages \
    --disable-podpages \
    --disable-txtpages \
    --enable-inline-asm \
	--enable-neon \
	--enable-thumb \
	--enable-optimizations \
	--enable-small  \
    --enable-static \
    --disable-shared \
    --enable-gmp \
    --enable-iconv \
    --enable-libass \
    --enable-libfontconfig \
    --enable-libfreetype \
    --enable-libfribidi \
    --enable-libmp3lame \
    --enable-libopencore-amrnb \
    --enable-libopencore-amrwb \
    --enable-libshine \
    --enable-libspeex \
    --enable-libtheora \
    --enable-libvorbis \
    --enable-libvpx \
    --enable-libwavpack \
    --enable-libwebp \
    --enable-libxml2 \
    --enable-mediacodec \
    --enable-zlib || exit 1

make -j$(nproc) || exit 1

make install || exit 1
