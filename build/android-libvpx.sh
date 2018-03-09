#!/bin/bash

if [[ -z $1 ]]; then
    echo "usage: $0 <mobile ffmpeg base directory>"
    exit 1
fi

if [[ -z ${ANDROID_NDK_ROOT} ]]; then
    echo "ANDROID_NDK_ROOT not defined"
    exit 1
fi

if [[ -z ${ARCH//-/_} ]]; then
    echo "ARCH not defined"
    exit 1
fi

if [[ -z ${API} ]]; then
    echo "API not defined"
    exit 1
fi

# ENABLE COMMON FUNCTIONS
. $1/build/common.sh

# PREPARING PATHS
android_prepare_toolchain_paths

# PREPARING FLAGS
TARGET_HOST=$(android_get_target_host)
export CFLAGS=$(android_get_cflags "libvpx")
export CXXFLAGS=$(android_get_cxxflags "libvpx")
export LDFLAGS=$(android_get_ldflags "libvpx")

SUPPORTED_ARCH=""
case ${ARCH} in
    arm)
        SUPPORTED_ARCH="armv7"
    ;;
    *)
        SUPPORTED_ARCH="${ARCH//-/_}"
    ;;
esac

cd $1/src/libvpx || exit 1

make distclean

./configure \
    --prefix=${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH//-/_}/libvpx \
    --target="${SUPPORTED_ARCH}-android-gcc" \
    --extra-cflags="${CFLAGS}" \
    --extra-cxxflags="${CXXFLAGS}" \
    --as=yasm \
    --log=yes \
    --enable-libs \
    --enable-install-libs \
    --enable-pic \
    --enable-optimizations \
    --enable-better-hw-compatibility \
    --enable-runtime-cpu-detect \
    --enable-vp8 \
    --enable-vp9 \
    --enable-multithread \
    --enable-spatial-resampling \
    --enable-runtime-cpu-detect \
    --enable-small \
    --enable-static \
    --disable-shared \
    --disable-debug \
    --disable-gprof \
    --disable-gcov \
    --disable-ccache \
    --disable-install-bins \
    --disable-install-srcs \
    --disable-install-docs \
    --disable-docs \
    --disable-tools \
    --disable-examples \
    --disable-unit-tests \
    --disable-decode-perf-tests \
    --disable-encode-perf-tests \
    --disable-codec-srcs \
    --disable-debug-libs \
    --disable-internal-stats || exit 1

make -j$(nproc) || exit 1

# MANUALLY COPY PKG-CONFIG FILES
cp ./*.pc ${INSTALL_PKG_CONFIG_DIR}

make install || exit 1
