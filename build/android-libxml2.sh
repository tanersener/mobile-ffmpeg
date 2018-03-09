#!/bin/bash

create_libxml2_package_config() {
    local LIBXML2_VERSION="$1"

    cat > "${INSTALL_PKG_CONFIG_DIR}/libxml-2.0.pc" << EOF
prefix=${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH//-/_}/libxml2
exec_prefix=\${prefix}
libdir=\${exec_prefix}/lib
includedir=\${prefix}/include
modules=1

Name: libXML
Version: ${LIBXML2_VERSION}
Description: libXML library version2.
Requires: libiconv
Libs: -L\${libdir} -lxml2
Libs.private:   -lz -lm
Cflags: -I\${includedir} -I\${includedir}/libxml2
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

# PREPARING PATHS & DEFINING ${INSTALL_PKG_CONFIG_DIR}
android_prepare_toolchain_paths

# PREPARING FLAGS
TARGET_HOST=$(android_get_target_host)
export CFLAGS=$(android_get_cflags "libxml2")
export CXXFLAGS=$(android_get_cxxflags "libxml2")
export LDFLAGS=$(android_get_ldflags "libxml2")

cd $1/src/libxml2 || exit 1

make distclean

# NOTE THAT PYTHON IS DISABLED DUE TO THE FOLLOWING ERROR
#
# .../android-sdk/ndk-bundle/toolchains/mobile-ffmpeg-arm/include/python2.7/pyport.h:1029:2: error: #error "LONG_BIT definition appears wrong for platform (bad gcc/glibc config?)."
# #error "LONG_BIT definition appears wrong for platform (bad gcc/glibc config?)."
#

./configure \
    --prefix=${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH//-/_}/libxml2 \
    --with-pic \
    --with-sysroot=${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-${ARCH//-/_}/sysroot \
    --with-zlib \
    --with-iconv=${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH//-/_}/libiconv \
    --with-sax1 \
    --without-python \
    --without-debug \
    --enable-static \
    --disable-shared \
    --disable-fast-install \
    --host=${TARGET_HOST} || exit 1

make -j$(nproc) || exit 1

# CREATE PACKAGE CONFIG MANUALLY
create_libxml2_package_config "2.9.7"

make install || exit 1
