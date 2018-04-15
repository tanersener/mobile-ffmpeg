#!/bin/bash

if [[ -z ${ARCH} ]]; then
    echo "ARCH not defined"
    exit 1
fi

if [[ -z ${IOS_MIN_VERSION} ]]; then
    echo "IOS_MIN_VERSION not defined"
    exit 1
fi

if [[ -z ${TARGET_SDK} ]]; then
    echo "TARGET_SDK not defined"
    exit 1
fi

if [[ -z ${SDK_PATH} ]]; then
    echo "SDK_PATH not defined"
    exit 1
fi

if [[ -z ${BASEDIR} ]]; then
    echo "BASEDIR not defined"
    exit 1
fi

# ENABLE COMMON FUNCTIONS
. ${BASEDIR}/build/ios-common.sh

# PREPARING PATHS & DEFINING ${INSTALL_PKG_CONFIG_DIR}
set_toolchain_clang_paths

# PREPARING FLAGS
export CFLAGS=$(get_cflags "libvpx")
export CXXFLAGS=$(get_cxxflags "libvpx")
export LDFLAGS=$(get_ldflags "libvpx")
export PKG_CONFIG_PATH="${INSTALL_PKG_CONFIG_DIR}"

TARGET_ARCH=""
NEON=""
case ${ARCH} in
    armv7 | armv7s)
        NEON="--enable-neon --enable-neon-asm"
        TARGET_ARCH="$(get_target_arch)"
    ;;
    arm64)
        NEON="--enable-neon"
        TARGET_ARCH="arm64"
    ;;
    *)
        NEON="--disable-neon --disable-neon-asm"
        TARGET_ARCH="$(get_target_arch)"
    ;;
esac

cd ${BASEDIR}/src/libvpx || exit 1

make distclean 2>/dev/null 1>/dev/null

./configure \
    --prefix=${BASEDIR}/prebuilt/ios-$(get_target_host)/libvpx \
    --target="${TARGET_ARCH}-darwin-gcc" \
    --extra-cflags="${CFLAGS}" \
    --extra-cxxflags="${CXXFLAGS}" \
    --as=yasm \
    --log=yes \
    --enable-libs \
    --enable-install-libs \
    --enable-pic \
    --enable-optimizations \
    --enable-better-hw-compatibility \
    --disable-runtime-cpu-detect \
    ${NEON} \
    --enable-vp8 \
    --enable-vp9 \
    --enable-multithread \
    --enable-spatial-resampling \
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
