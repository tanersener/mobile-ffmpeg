#!/bin/bash

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

if [[ -z ${BASEDIR} ]]; then
    echo "BASEDIR not defined"
    exit 1
fi

# ENABLE COMMON FUNCTIONS
. ${BASEDIR}/build/android-common.sh

# PREPARING PATHS & DEFINING ${INSTALL_PKG_CONFIG_DIR}
set_toolchain_clang_paths

# PREPARING FLAGS
TARGET_HOST=$(get_target_host)
export CFLAGS="$(get_cflags "libvpx") -I${ANDROID_NDK_ROOT}/sources/android/cpufeatures"
export CXXFLAGS=$(get_cxxflags "libvpx")
export LDFLAGS="$(get_ldflags "libvpx") -L${ANDROID_NDK_ROOT}/sources/android/cpufeatures -lcpufeatures"
export PKG_CONFIG_PATH="${INSTALL_PKG_CONFIG_DIR}"

TARGET_CPU=""
DISABLE_NEON_FLAG=""
case ${ARCH} in
    arm-v7a)
        TARGET_CPU="armv7"
        DISABLE_NEON_FLAG="--disable-neon"
    ;;
    arm-v7a-neon)
        TARGET_CPU="armv7"
        # NEON IS ENABLED BY --enable-runtime-cpu-detect
    ;;
    arm64-v8a)
        TARGET_CPU="arm64"
    ;;
    *)
        TARGET_CPU="$(get_target_build ${ARCH})"
    ;;
esac

cd ${BASEDIR}/src/libvpx || exit 1

# build cpu-features
build_cpufeatures

make distclean 2>/dev/null 1>/dev/null

./configure \
    --prefix=${ANDROID_NDK_ROOT}/prebuilt/android-$(get_target_build ${ARCH})/libvpx \
    --target="${TARGET_CPU}-android-gcc" \
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
    ${DISABLE_NEON_FLAG} \
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

make -j$(get_cpu_count) || exit 1

# MANUALLY COPY PKG-CONFIG FILES
cp ./*.pc ${INSTALL_PKG_CONFIG_DIR}

make install || exit 1
