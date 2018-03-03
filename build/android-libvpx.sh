#!/bin/bash

if [[ -z $1 ]]; then
    echo "usage: $0 <mobile ffmpeg base directory>"
    exit 1
fi

if [[ -z $ANDROID_NDK ]]; then
    echo "ANDROID_NDK not defined"
    exit 1
fi

if [[ -z $ARCH ]]; then
    echo "ARCH not defined"
    exit 1
fi

if [[ -z $API ]]; then
    echo "API not defined"
    exit 1
fi

# ENABLE COMMON FUNCTIONS
. $1/build/common.sh

# PREPARING PATHS
android_prepare_toolchain_paths

TARGET_HOST=$(android_get_target_host)
CFLAGS=$(android_get_cflags "libvpx")
CXXFLAGS=$(android_get_cxxflags)
LDFLAGS=$(android_get_ldflags "libvpx")

OPTIONAL_CPU_SUPPORT=""
case $ARCH in
    arm)
        SUPPORTED_CPU="armv7"
    ;;
    *)
        SUPPORTED_CPU="${ARCH}"
    ;;
esac

cd $1/src/libvpx || exit 1

make clean

./configure \
    --prefix=$ANDROID_NDK/prebuilt/android-$ARCH/libvpx \
    --target="${SUPPORTED_CPU}-android-gcc" \
    --extra-cflags="${CFLAGS}" \
    --extra-cxxflags="${CXXFLAGS}" \
    --log=yes \
    --enable-optimizations \
    --enable-pic \
    --disable-ccache \
    --enable-thumb \
    --disable-debug \
    --disable-gprof \
    --disable-gcov \
    --enable-libs \
    --enable-install-libs \
    --disable-install-bins \
    --disable-install-srcs \
    --disable-install-docs \
    --disable-docs \
    --disable-tools \
    --disable-examples \
    --disable-unit-tests \
    --disable-decode-perf-tests \
    --disable-encode-perf-tests \
    --sdk-path=${ANDROID_NDK} \
    --disable-codec-srcs \
    --disable-debug-libs \
    --enable-better-hw-compatibility \
    --enable-vp8 \
    --enable-vp9 \
    --disable-internal-stats \
    --enable-multithread \
    --enable-spatial-resampling \
    --enable-runtime-cpu-detect \
    --enable-static \
    --disable-shared \
    --enable-small || exit 1

CFLAGS=${CFLAGS} \
CXXFLAGS=${CXXFLAGS} \
LDFLAGS=${LDFLAGS} \
make -j$(nproc) || exit 1

CFLAGS=${CFLAGS} \
CXXFLAGS=${CXXFLAGS} \
LDFLAGS=${LDFLAGS} \
make install || exit 1
