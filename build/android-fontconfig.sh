#!/bin/bash

create_fontconfig_package_config() {
    local FONTCONFIG_VERSION="$1"

    cat > "${INSTALL_PKG_CONFIG_DIR}/fontconfig.pc" << EOF
prefix=${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/fontconfig
exec_prefix=\${prefix}
libdir=\${exec_prefix}/lib
includedir=\${prefix}/include
sysconfdir=\${prefix}/etc
localstatedir=\${prefix}/var
PACKAGE=fontconfig
confdir=\${sysconfdir}/fonts
cachedir=\${localstatedir}/cache/\${PACKAGE}

Name: Fontconfig
Description: Font configuration and customization library
Version: ${FONTCONFIG_VERSION}
Requires:  freetype2 >= 21.0.15, uuid, libxml-2.0 >= 2.6
Requires.private:
Libs: -L\${libdir} -lfontconfig
Libs.private:
Cflags: -I\${includedir}
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
export CFLAGS=$(android_get_cflags "fontconfig")
export CXXFLAGS=$(android_get_cxxflags "fontconfig")
export LDFLAGS=$(android_get_ldflags "fontconfig")
export PKG_CONFIG_PATH=${INSTALL_PKG_CONFIG_DIR}

cd $1/src/fontconfig || exit 1

make clean

./configure \
    --prefix=${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/fontconfig \
    --with-pic \
    --with-libiconv \
    --enable-static \
    --disable-shared \
    --disable-fast-install \
    --disable-rpath \
    --enable-iconv \
    --enable-libxml2 \
    --disable-docs \
    --host=${TARGET_HOST} || exit 1

make -j$(nproc) || exit 1

# CREATE PACKAGE CONFIG MANUALLY
create_fontconfig_package_config "2.12.93"

make install || exit 1
