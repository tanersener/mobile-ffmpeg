#!/bin/bash

create_freetype_package_config() {
    local FREETYPE_VERSION="$1"

    cat > "${INSTALL_PKG_CONFIG_DIR}/freetype2.pc" << EOF
prefix=${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/freetype
exec_prefix=\${prefix}
libdir=\${exec_prefix}/lib
includedir=\${prefix}/include

Name: FreeType 2
URL: https://freetype.org
Description: A free, high-quality, and portable font engine.
Version: ${FREETYPE_VERSION}
Requires: libpng
Requires.private: zlib
Libs: -L\${libdir} -lfreetype
Libs.private:
Cflags: -I\${includedir}/freetype2
EOF
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

# ENABLE COMMON FUNCTIONS
. $1/build/common.sh

# PREPARING PATHS & DEFINING ${INSTALL_PKG_CONFIG_DIR}
android_prepare_toolchain_paths

# PREPARING FLAGS
TARGET_HOST=$(android_get_target_host)
export CFLAGS=$(android_get_cflags "freetype")
export CXXFLAGS=$(android_get_cxxflags "freetype")
export LDFLAGS=$(android_get_ldflags "freetype")
export PKG_CONFIG_PATH="${INSTALL_PKG_CONFIG_DIR}"

cd $1/src/freetype || exit 1

make clean

./configure \
    --prefix=${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/freetype \
    --with-pic \
    --with-zlib \
    --with-png \
    --with-sysroot=${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-${ARCH}/sysroot \
    --enable-static \
    --disable-shared \
    --disable-fast-install \
    --disable-mmap \
    --host=${TARGET_HOST} || exit 1

make -j$(nproc) || exit 1

# CREATE PACKAGE CONFIG MANUALLY
create_freetype_package_config "22.0.16"

make install || exit 1
