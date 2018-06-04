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
export CFLAGS=$(get_cflags "libass")
export CXXFLAGS=$(get_cxxflags "libass")
export LDFLAGS=$(get_ldflags "libass")
unset PKG_CONFIG_PATH

cd ${BASEDIR}/src/libass || exit 1

make distclean 2>/dev/null 1>/dev/null

ASM_FLAGS=""
case ${ARCH} in
    x86)
        ASM_FLAGS="	--disable-asm"
    ;;
    *)
        ASM_FLAGS="	--enable-asm"
    ;;
esac

# RECONFIGURING IF REQUESTED
if [[ ${RECONF_libass} -eq 1 ]]; then
    autoreconf --force --install
fi

./configure \
    --prefix=${BASEDIR}/prebuilt/android-$(get_target_build)/libass \
    --with-pic \
    --with-sysroot=${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-${TOOLCHAIN}/sysroot \
    --enable-static \
    --disable-shared \
    --disable-harfbuzz \
    --disable-fast-install \
    --disable-test \
    --disable-profile \
    ${ASM_FLAGS} \
    --host=${TARGET_HOST} || exit 1

make -j$(get_cpu_count) || exit 1

# MANUALLY COPY PKG-CONFIG FILES
cp ./*.pc ${INSTALL_PKG_CONFIG_DIR}

make install || exit 1
