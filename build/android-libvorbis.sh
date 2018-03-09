#!/bin/bash

create_libvorbis_package_config() {
    local LIBVORBIS_VERSION="$1"

    cat > "${INSTALL_PKG_CONFIG_DIR}/vorbis.pc" << EOF
prefix=${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH//-/_}/libvorbis
exec_prefix=\${prefix}
libdir=\${prefix}/lib
includedir=\${prefix}/include

Name: vorbis
Description: vorbis is the primary Ogg Vorbis library
Version: ${LIBVORBIS_VERSION}

Requires: ogg
Libs: -L\${libdir} -lvorbis -lm
Cflags: -I\${includedir}
EOF

cat > "${INSTALL_PKG_CONFIG_DIR}/vorbisenc.pc" << EOF
prefix=${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH//-/_}/libvorbis
exec_prefix=\${prefix}
libdir=\${prefix}/lib
includedir=\${prefix}/include

Name: vorbisenc
Description: vorbisenc is a library that provides a convenient API for setting up an encoding environment using libvorbis
Version: ${LIBVORBIS_VERSION}

Requires: vorbis
Conflicts:
Libs: -L\${libdir} -lvorbisenc
Cflags: -I\${includedir}
EOF

cat > "${INSTALL_PKG_CONFIG_DIR}/vorbisfile.pc" << EOF
prefix=${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH//-/_}/libvorbis
exec_prefix=\${prefix}
libdir=\${prefix}/lib
includedir=\${prefix}/include

Name: vorbisfile
Description: vorbisfile is a library that provides a convenient high-level API for decoding and basic manipulation of all Vorbis I audio streams
Version: ${LIBVORBIS_VERSION}

Requires: vorbis
Conflicts:
Libs: -L\${libdir} -lvorbisfile
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
export CFLAGS=$(android_get_cflags "libvorbis")
export CXXFLAGS=$(android_get_cxxflags "libvorbis")
export LDFLAGS=$(android_get_ldflags "libvorbis")

cd $1/src/libvorbis || exit 1

make distclean

./configure \
    --prefix=${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH//-/_}/libvorbis \
    --with-pic \
    --with-sysroot=${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-${ARCH//-/_}/sysroot \
    --with-ogg-includes=${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH//-/_}/libogg/include \
    --with-ogg-libraries=${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH//-/_}/libogg/lib \
    --enable-static \
    --disable-shared \
    --disable-fast-install \
    --disable-docs \
    --disable-examples \
    --disable-oggtest \
    --host=${TARGET_HOST} || exit 1

make -j$(nproc) || exit 1

# CREATE PACKAGE CONFIG MANUALLY
create_libvorbis_package_config "1.3.5"

make install || exit 1
