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

TARGET_HOST=$(android_get_target_host)
TARGET_MACHINE=$(android_get_target_machine)
COMMON_CFLAGS=$(android_get_cflags "ffmpeg")
CXXFLAGS=$(android_get_cxxflags "ffmpeg")
COMMON_LDFLAGS=$(android_get_ldflags "ffmpeg")

CFLAGS="${COMMON_CFLAGS} \
-I${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/openssl/include \
-I${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/libiconv/include \
-I${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/lame/include \
-I${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/libxml2/include \
-I${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/libtheora/include \
$(pkg-config --cflags ${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/wavpack/lib/pkgconfig/wavpack.pc) \
$(pkg-config --cflags ${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/libogg/lib/pkgconfig/ogg.pc) \
$(pkg-config --cflags ${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/libvpx/lib/pkgconfig/vpx.pc) \
$(pkg-config --cflags ${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/opencore-amr/lib/pkgconfig/opencore-amrnb.pc) \
$(pkg-config --cflags ${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/opencore-amr/lib/pkgconfig/opencore-amrwb.pc)"
LDFLAGS="${COMMON_LDLAGS} \
-L${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/openssl/lib \
-L${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/libiconv/lib \
-L${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/lame/lib \
-L${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/libxml2/lib \
-L${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/libtheora/lib \
-L${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/libvorbis/lib \
$(pkg-config --libs ${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/wavpack/lib/pkgconfig/wavpack.pc) \
$(pkg-config --libs ${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/libogg/lib/pkgconfig/ogg.pc) \
$(pkg-config --libs ${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/libvpx/lib/pkgconfig/vpx.pc) \
$(pkg-config --libs ${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/opencore-amr/lib/pkgconfig/opencore-amrnb.pc) \
$(pkg-config --libs ${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/opencore-amr/lib/pkgconfig/opencore-amrwb.pc) \
-L${ANDROID_NDK_ROOT}/platforms/android-${API}/arch-${ARCH}/usr/lib -lm -landroid -lvorbisenc -lvorbisfile"

export PKG_CONFIG_PATH="/tmp/config"
export PKG_CONFIG_LIBDIR="/tmp/config"

cd $1/src/ffmpeg || exit 1

make clean

CFLAGS=${CFLAGS} \
CXXFLAGS=${CXXFLAGS} \
LDFLAGS=${LDFLAGS} \
./configure \
    --cross-prefix=${TARGET_HOST}- \
    --sysroot=${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-${ARCH}/sysroot \
    --prefix=${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/ffmpeg \
    --pkg-config="${HOST_PKG_CONFIG_PATH}" \
    --extra-cflags="${CFLAGS}" \
    --extra-cxxflags="${CXXFLAGS}" \
    --extra-ldflags="${LDFLAGS}" \
    --extra-libs="-liconv -lxml2" \
    --enable-version3 \
    --arch=${ARCH} \
    --cpu=${TARGET_MACHINE} \
    --target-os=android \
	--enable-cross-compile \
    --enable-pic \
    --enable-asm \
    --enable-jni \
    -- enable-avresample \
    --disable-xmm-clobber-test \
    --disable-debug \
    --disable-neon-clobber-test \
    --disable-ffprobe \
    --disable-ffplay \
    --disable-ffserver \
    --disable-doc \
    --disable-htmlpages \
    --disable-manpages \
    --disable-podpages \
    --disable-txtpages \
    --enable-network \
    --enable-inline-asm \
	--enable-neon \
	--enable-thumb \
	--enable-optimizations \
	--enable-small  \
    --enable-static \
    --disable-shared \
    --enable-iconv \
    --enable-libvorbis \
    --enable-libmp3lame \
    --enable-libopencore-amrnb \
    --enable-libopencore-amrwb \
    --enable-libvpx \
    --enable-libwavpack \
    --enable-libtheora \
    --enable-mediacodec \
    --enable-openssl \
    --enable-zlib || exit 1

CFLAGS=${CFLAGS} \
CXXFLAGS=${CXXFLAGS} \
LDFLAGS=${LDFLAGS} \
#make -j$(nproc) || exit 1

CFLAGS=${CFLAGS} \
CXXFLAGS=${CXXFLAGS} \
LDFLAGS=${LDFLAGS} \
#make install || exit 1
