#!/bin/bash

get_cpu_count() {
    if [ "$(uname)" == "Darwin" ]; then
        echo $(sysctl -n hw.physicalcpu)
    else
        echo $(nproc)
    fi
}

prepare_inline_sed() {
    if [ "$(uname)" == "Darwin" ]; then
        export SED_INLINE="sed -i .tmp"
    else
        export SED_INLINE="sed -i"
    fi
}

get_library_name() {
    case $1 in
        0) echo "fontconfig" ;;
        1) echo "freetype" ;;
        2) echo "fribidi" ;;
        3) echo "gmp" ;;
        4) echo "gnutls" ;;
        5) echo "lame" ;;
        6) echo "libass" ;;
        7) echo "libiconv" ;;
        8) echo "libtheora" ;;
        9) echo "libvorbis" ;;
        10) echo "libvpx" ;;
        11) echo "libwebp" ;;
        12) echo "libxml2" ;;
        13) echo "opencore-amr" ;;
        14) echo "shine" ;;
        15) echo "speex" ;;
        16) echo "wavpack" ;;
        17) echo "kvazaar" ;;
        18) echo "x264" ;;
        19) echo "xvidcore" ;;
        20) echo "x265" ;;
        21) echo "libvidstab" ;;
        22) echo "libilbc" ;;
        23) echo "opus" ;;
        24) echo "snappy" ;;
        25) echo "soxr" ;;
        26) echo "libaom" ;;
        27) echo "chromaprint" ;;
        28) echo "twolame" ;;
        29) echo "sdl" ;;
        30) echo "tesseract" ;;
        31) echo "openh264" ;;
        32) echo "giflib" ;;
        33) echo "jpeg" ;;
        34) echo "libogg" ;;
        35) echo "libpng" ;;
        36) echo "libuuid" ;;
        37) echo "nettle" ;;
        38) echo "tiff" ;;
        39) echo "expat" ;;
        40) echo "libsndfile" ;;
        41) echo "leptonica" ;;
        42) echo "android-zlib" ;;
        43) echo "android-media-codec" ;;
    esac
}

get_arch_name() {
    case $1 in
        0) echo "arm-v7a" ;;
        1) echo "arm-v7a-neon" ;;
        2) echo "arm64-v8a" ;;
        3) echo "x86" ;;
        4) echo "x86-64" ;;
    esac
}

get_target_host() {
    case ${ARCH} in
        arm-v7a | arm-v7a-neon)
            echo "arm-linux-androideabi"
        ;;
        arm64-v8a)
            echo "aarch64-linux-android"
        ;;
        x86)
            echo "i686-linux-android"
        ;;
        x86-64)
            echo "x86_64-linux-android"
        ;;
    esac
}


get_clang_target_host() {
    case ${ARCH} in
        arm-v7a | arm-v7a-neon)
            echo "armv7a-linux-androideabi${API}"
        ;;
        arm64-v8a)
            echo "aarch64-linux-android${API}"
        ;;
        x86)
            echo "i686-linux-android${API}"
        ;;
        x86-64)
            echo "x86_64-linux-android${API}"
        ;;
    esac
}

get_toolchain() {
    HOST_OS=$(uname -s)
    case ${HOST_OS} in
        Darwin) HOST_OS=darwin;;
        Linux) HOST_OS=linux;;
        FreeBsd) HOST_OS=freebsd;;
        CYGWIN*|*_NT-*) HOST_OS=cygwin;;
    esac

    HOST_ARCH=$(uname -m)
    case ${HOST_ARCH} in
        i?86) HOST_ARCH=x86;;
        x86_64|amd64) HOST_ARCH=x86_64;;
    esac

    echo "${HOST_OS}-${HOST_ARCH}"
}

get_cmake_target_processor() {
    case ${ARCH} in
        arm-v7a | arm-v7a-neon)
            echo "arm"
        ;;
        arm64-v8a)
            echo "aarch64"
        ;;
        x86)
            echo "x86"
        ;;
        x86-64)
            echo "x86_64"
        ;;
    esac
}

get_target_build() {
    case ${ARCH} in
        arm-v7a)
            echo "arm"
        ;;
        arm-v7a-neon)
            if [[ ! -z ${MOBILE_FFMPEG_LTS_BUILD} ]]; then
                echo "arm/neon"
            else
                echo "arm"
            fi
        ;;
        arm64-v8a)
            echo "arm64"
        ;;
        x86)
            echo "x86"
        ;;
        x86-64)
            echo "x86_64"
        ;;
    esac
}

get_toolchain_arch() {
    case ${ARCH} in
        arm-v7a | arm-v7a-neon)
            echo "arm"
        ;;
        arm64-v8a)
            echo "arm64"
        ;;
        x86)
            echo "x86"
        ;;
        x86-64)
            echo "x86_64"
        ;;
    esac
}

get_android_arch() {
    case $1 in
        0 | 1)
            echo "armeabi-v7a"
        ;;
        2)
            echo "arm64-v8a"
        ;;
        3)
            echo "x86"
        ;;
        4)
            echo "x86_64"
        ;;
    esac
}

get_common_includes() {
    echo "-I${ANDROID_NDK_ROOT}/toolchains/llvm/prebuilt/${TOOLCHAIN}/sysroot/usr/include -I${ANDROID_NDK_ROOT}/toolchains/llvm/prebuilt/${TOOLCHAIN}/sysroot/usr/local/include"
}

get_common_cflags() {
    if [[ ! -z ${MOBILE_FFMPEG_LTS_BUILD} ]]; then
        local LTS_BUILD__FLAG="-DMOBILE_FFMPEG_LTS "
    fi

    echo "-fno-integrated-as -fstrict-aliasing -fPIC -DANDROID ${LTS_BUILD__FLAG}-D__ANDROID__ -D__ANDROID_API__=${API}"
}

get_arch_specific_cflags() {
    case ${ARCH} in
        arm-v7a)
            echo "-march=armv7-a -mfpu=vfpv3-d16 -mfloat-abi=softfp -DMOBILE_FFMPEG_ARM_V7A"
        ;;
        arm-v7a-neon)
            echo "-march=armv7-a -mfpu=neon -mfloat-abi=softfp -DMOBILE_FFMPEG_ARM_V7A_NEON"
        ;;
        arm64-v8a)
            echo "-march=armv8-a -DMOBILE_FFMPEG_ARM64_V8A"
        ;;
        x86)
            echo "-march=i686 -mtune=intel -mssse3 -mfpmath=sse -m32 -DMOBILE_FFMPEG_X86"
        ;;
        x86-64)
            echo "-march=x86-64 -msse4.2 -mpopcnt -m64 -mtune=intel -DMOBILE_FFMPEG_X86_64"
        ;;
    esac
}

get_size_optimization_cflags() {
    if [[ -z ${NO_LINK_TIME_OPTIMIZATION} ]]; then
        local LINK_TIME_OPTIMIZATION_FLAGS="-flto"
    else
        local LINK_TIME_OPTIMIZATION_FLAGS=""
    fi

    local ARCH_OPTIMIZATION=""
    case ${ARCH} in
        arm-v7a | arm-v7a-neon)
            case $1 in
                ffmpeg)
                    ARCH_OPTIMIZATION="${LINK_TIME_OPTIMIZATION_FLAGS} -O2 -ffunction-sections -fdata-sections"
                ;;
                *)
                    ARCH_OPTIMIZATION="-O2 -ffunction-sections -fdata-sections"
                ;;
            esac
        ;;
        arm64-v8a)
            case $1 in
                ffmpeg)
                    ARCH_OPTIMIZATION="${LINK_TIME_OPTIMIZATION_FLAGS} -fuse-ld=bfd -O2 -ffunction-sections -fdata-sections"
                ;;
                *)
                    ARCH_OPTIMIZATION="-O2 -ffunction-sections -fdata-sections"
                ;;
            esac
        ;;
        x86 | x86-64)
            case $1 in
                ffmpeg)
                    ARCH_OPTIMIZATION="${LINK_TIME_OPTIMIZATION_FLAGS} -O2 -ffunction-sections -fdata-sections"
                ;;
                *)
                    ARCH_OPTIMIZATION="-O2 -ffunction-sections -fdata-sections"
                ;;
            esac
        ;;
    esac

    local LIB_OPTIMIZATION=""

    echo "${ARCH_OPTIMIZATION} ${LIB_OPTIMIZATION}"
}

get_app_specific_cflags() {

    local APP_FLAGS=""
    case $1 in
        xvidcore)
            APP_FLAGS=""
        ;;
        ffmpeg)
            APP_FLAGS="-Wno-unused-function -DBIONIC_IOCTL_NO_SIGNEDNESS_OVERLOAD"
        ;;
        kvazaar)
            APP_FLAGS="-std=gnu99 -Wno-unused-function"
        ;;
        shine)
            APP_FLAGS="-Wno-unused-function"
        ;;
        sdl)
            APP_FLAGS="-std=c11 -Wno-unused-function"
        ;;
        soxr | snappy | libwebp)
            APP_FLAGS="-std=gnu99 -Wno-unused-function -DPIC"
        ;;
        *)
            APP_FLAGS="-std=c99 -Wno-unused-function"
        ;;
    esac

    echo "${APP_FLAGS}"
}

get_cflags() {
    local ARCH_FLAGS=$(get_arch_specific_cflags)
    local APP_FLAGS=$(get_app_specific_cflags $1)
    local COMMON_FLAGS=$(get_common_cflags)
    if [[ -z ${MOBILE_FFMPEG_DEBUG} ]]; then
        local OPTIMIZATION_FLAGS=$(get_size_optimization_cflags $1)
    else
        local OPTIMIZATION_FLAGS="${MOBILE_FFMPEG_DEBUG}"
    fi
    local COMMON_INCLUDES=$(get_common_includes)

    echo "${ARCH_FLAGS} ${APP_FLAGS} ${COMMON_FLAGS} ${OPTIMIZATION_FLAGS} ${COMMON_INCLUDES}"
}

get_cxxflags() {
    if [[ -z ${NO_LINK_TIME_OPTIMIZATION} ]]; then
        local LINK_TIME_OPTIMIZATION_FLAGS="-flto"
    else
        local LINK_TIME_OPTIMIZATION_FLAGS=""
    fi

    if [[ -z ${MOBILE_FFMPEG_DEBUG} ]]; then
        local OPTIMIZATION_FLAGS="-O2 -ffunction-sections -fdata-sections"
    else
        local OPTIMIZATION_FLAGS="${MOBILE_FFMPEG_DEBUG}"
    fi

    case $1 in
        gnutls)
            echo "-std=c++11 -fno-rtti ${OPTIMIZATION_FLAGS}"
        ;;
        ffmpeg)
            if [[ -z ${MOBILE_FFMPEG_DEBUG} ]]; then
                echo "-std=c++11 -fno-exceptions -fno-rtti ${LINK_TIME_OPTIMIZATION_FLAGS} -O2 -ffunction-sections -fdata-sections"
            else
                echo "-std=c++11 -fno-exceptions -fno-rtti ${MOBILE_FFMPEG_DEBUG}"
            fi
        ;;
        opencore-amr)
            echo "${OPTIMIZATION_FLAGS}"
        ;;
        x265)
            echo "-std=c++11 -fno-exceptions ${OPTIMIZATION_FLAGS}"
        ;;
        *)
            echo "-std=c++11 -fno-exceptions -fno-rtti ${OPTIMIZATION_FLAGS}"
        ;;
    esac
}

get_common_linked_libraries() {
    local COMMON_LIBRARY_PATHS="-L${ANDROID_NDK_ROOT}/toolchains/llvm/prebuilt/${TOOLCHAIN}/${TARGET_HOST}/lib -L${ANDROID_NDK_ROOT}/toolchains/llvm/prebuilt/${TOOLCHAIN}/sysroot/usr/lib/${TARGET_HOST}/${API} -L${ANDROID_NDK_ROOT}/toolchains/llvm/prebuilt/${TOOLCHAIN}/lib"

    case $1 in
        ffmpeg)
            if [[ -z ${MOBILE_FFMPEG_LTS_BUILD} ]]; then
                echo "-lc -lm -ldl -llog -lcamera2ndk -lmediandk ${COMMON_LIBRARY_PATHS}"
            else
                echo "-lc -lm -ldl -llog ${COMMON_LIBRARY_PATHS}"
            fi
        ;;
        libvpx)
            echo "-lc -lm ${COMMON_LIBRARY_PATHS}"
        ;;
        tesseract | x265)
            echo "-lc -lm -ldl -llog -lc++_shared ${COMMON_LIBRARY_PATHS}"
        ;;
        *)
            echo "-lc -lm -ldl -llog ${COMMON_LIBRARY_PATHS}"
        ;;
    esac
}

get_size_optimization_ldflags() {
    if [[ -z ${NO_LINK_TIME_OPTIMIZATION} ]]; then
        local LINK_TIME_OPTIMIZATION_FLAGS="-flto"
    else
        local LINK_TIME_OPTIMIZATION_FLAGS=""
    fi

    case ${ARCH} in
        arm64-v8a)
            case $1 in
                ffmpeg)
                    echo "-Wl,--gc-sections ${LINK_TIME_OPTIMIZATION_FLAGS} -fuse-ld=bfd -O2 -ffunction-sections -fdata-sections -finline-functions"
                ;;
                *)
                    echo "-Wl,--gc-sections -O2 -ffunction-sections -fdata-sections"
                ;;
            esac
        ;;
        *)
            case $1 in
                ffmpeg)
                    echo "-Wl,--gc-sections,--icf=safe ${LINK_TIME_OPTIMIZATION_FLAGS} -O2 -ffunction-sections -fdata-sections -finline-functions"
                ;;
                *)
                    echo "-Wl,--gc-sections,--icf=safe -O2 -ffunction-sections -fdata-sections"
                ;;
            esac
        ;;
    esac
}

get_arch_specific_ldflags() {
    case ${ARCH} in
        arm-v7a)
            echo "-march=armv7-a -mfpu=vfpv3-d16 -mfloat-abi=softfp -Wl,--fix-cortex-a8"
        ;;
        arm-v7a-neon)
            echo "-march=armv7-a -mfpu=neon -mfloat-abi=softfp -Wl,--fix-cortex-a8"
        ;;
        arm64-v8a)
            echo "-march=armv8-a"
        ;;
        x86)
            echo "-march=i686"
        ;;
        x86-64)
            echo "-march=x86-64"
        ;;
    esac
}

get_ldflags() {
    local ARCH_FLAGS=$(get_arch_specific_ldflags)
    if [[ -z ${MOBILE_FFMPEG_DEBUG} ]]; then
        local OPTIMIZATION_FLAGS="$(get_size_optimization_ldflags $1)"
    else
        local OPTIMIZATION_FLAGS="${MOBILE_FFMPEG_DEBUG}"
    fi
    local COMMON_LINKED_LIBS=$(get_common_linked_libraries $1)

    echo "${ARCH_FLAGS} ${OPTIMIZATION_FLAGS} ${COMMON_LINKED_LIBS} -Wl,--hash-style=both -Wl,--exclude-libs,libgcc.a -Wl,--exclude-libs,libunwind.a"
}

create_chromaprint_package_config() {
    local CHROMAPRINT_VERSION="$1"

    cat > "${INSTALL_PKG_CONFIG_DIR}/libchromaprint.pc" << EOF
prefix=${BASEDIR}/prebuilt/android-$(get_target_build)/chromaprint
exec_prefix=\${prefix}
libdir=\${exec_prefix}/lib
includedir=\${prefix}/include

Name: chromaprint
Description: Audio fingerprint library
URL: http://acoustid.org/chromaprint
Version: ${CHROMAPRINT_VERSION}
Libs: -L\${libdir} -lchromaprint
Cflags: -I\${includedir}
EOF
}

create_fontconfig_package_config() {
    local FONTCONFIG_VERSION="$1"

    cat > "${INSTALL_PKG_CONFIG_DIR}/fontconfig.pc" << EOF
prefix=${BASEDIR}/prebuilt/android-$(get_target_build)/fontconfig
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
Requires:  freetype2 >= 21.0.15, uuid, expat >= 2.2.0, libiconv
Requires.private:
Libs: -L\${libdir} -lfontconfig
Libs.private:
Cflags: -I\${includedir}
EOF
}

create_freetype_package_config() {
    local FREETYPE_VERSION="$1"

    cat > "${INSTALL_PKG_CONFIG_DIR}/freetype2.pc" << EOF
prefix=${BASEDIR}/prebuilt/android-$(get_target_build)/freetype
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

create_giflib_package_config() {
    local GIFLIB_VERSION="$1"

    cat > "${INSTALL_PKG_CONFIG_DIR}/giflib.pc" << EOF
prefix=${BASEDIR}/prebuilt/android-$(get_target_build)/giflib
exec_prefix=\${prefix}
libdir=\${prefix}/lib
includedir=\${prefix}/include

Name: giflib
Description: gif library
Version: ${GIFLIB_VERSION}

Requires:
Libs: -L\${libdir} -lgif
Cflags: -I\${includedir}
EOF
}

create_gmp_package_config() {
    local GMP_VERSION="$1"

    cat > "${INSTALL_PKG_CONFIG_DIR}/gmp.pc" << EOF
prefix=${BASEDIR}/prebuilt/android-$(get_target_build)/gmp
exec_prefix=\${prefix}
libdir=\${prefix}/lib
includedir=\${prefix}/include

Name: gmp
Description: gnu mp library
Version: ${GMP_VERSION}

Requires:
Libs: -L\${libdir} -lgmp
Cflags: -I\${includedir}
EOF
}

create_gnutls_package_config() {
    local GNUTLS_VERSION="$1"

    cat > "${INSTALL_PKG_CONFIG_DIR}/gnutls.pc" << EOF
prefix=${BASEDIR}/prebuilt/android-$(get_target_build)/gnutls
exec_prefix=\${prefix}
libdir=\${exec_prefix}/lib
includedir=\${prefix}/include

Name: gnutls
Description: GNU TLS Implementation

Version: ${GNUTLS_VERSION}
Requires: nettle, hogweed, zlib
Cflags: -I\${includedir}
Libs: -L\${libdir} -lgnutls
Libs.private: -lgmp
EOF
}

create_libaom_package_config() {
    local AOM_VERSION="$1"

    cat > "${INSTALL_PKG_CONFIG_DIR}/aom.pc" << EOF
prefix=${BASEDIR}/prebuilt/android-$(get_target_build)/libaom
exec_prefix=\${prefix}
libdir=\${prefix}/lib
includedir=\${prefix}/include

Name: aom
Description: AV1 codec library v${AOM_VERSION}.
Version: ${AOM_VERSION}

Requires:
Libs: -L\${libdir} -laom -lm
Cflags: -I\${includedir}
EOF
}

create_libiconv_package_config() {
    local LIB_ICONV_VERSION="$1"

    cat > "${INSTALL_PKG_CONFIG_DIR}/libiconv.pc" << EOF
prefix=${BASEDIR}/prebuilt/android-$(get_target_build)/libiconv
exec_prefix=\${prefix}
libdir=\${exec_prefix}/lib
includedir=\${prefix}/include

Name: libiconv
Description: Character set conversion library
Version: ${LIB_ICONV_VERSION}

Requires:
Libs: -L\${libdir} -liconv -lcharset
Cflags: -I\${includedir}
EOF
}

create_libmp3lame_package_config() {
    local LAME_VERSION="$1"

    cat > "${INSTALL_PKG_CONFIG_DIR}/libmp3lame.pc" << EOF
prefix=${BASEDIR}/prebuilt/android-$(get_target_build)/lame
exec_prefix=\${prefix}
libdir=\${exec_prefix}/lib
includedir=\${prefix}/include

Name: libmp3lame
Description: lame mp3 encoder library
Version: ${LAME_VERSION}

Requires:
Libs: -L\${libdir} -lmp3lame
Cflags: -I\${includedir}
EOF
}

create_libvorbis_package_config() {
    local LIBVORBIS_VERSION="$1"

    cat > "${INSTALL_PKG_CONFIG_DIR}/vorbis.pc" << EOF
prefix=${BASEDIR}/prebuilt/android-$(get_target_build)/libvorbis
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
prefix=${BASEDIR}/prebuilt/android-$(get_target_build)/libvorbis
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
prefix=${BASEDIR}/prebuilt/android-$(get_target_build)/libvorbis
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

create_libwebp_package_config() {
    local LIB_WEBP_VERSION="$1"

    cat > "${INSTALL_PKG_CONFIG_DIR}/libwebp.pc" << EOF
prefix=${BASEDIR}/prebuilt/android-$(get_target_build)/libwebp
exec_prefix=\${prefix}
libdir=\${prefix}/lib
includedir=\${prefix}/include

Name: libwebp
Description: webp codec library
Version: ${LIB_WEBP_VERSION}

Requires:
Libs: -L\${libdir} -lwebp -lwebpdecoder -lwebpdemux
Cflags: -I\${includedir}
EOF
}

create_libxml2_package_config() {
    local LIBXML2_VERSION="$1"

    cat > "${INSTALL_PKG_CONFIG_DIR}/libxml-2.0.pc" << EOF
prefix=${BASEDIR}/prebuilt/android-$(get_target_build)/libxml2
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

create_snappy_package_config() {
    local SNAPPY_VERSION="$1"

    cat > "${INSTALL_PKG_CONFIG_DIR}/snappy.pc" << EOF
prefix=${BASEDIR}/prebuilt/android-$(get_target_build)/snappy
exec_prefix=\${prefix}
libdir=\${prefix}/lib
includedir=\${prefix}/include

Name: snappy
Description: a fast compressor/decompressor
Version: ${SNAPPY_VERSION}

Requires:
Libs: -L\${libdir} -lz -lc++
Cflags: -I\${includedir}
EOF
}

create_soxr_package_config() {
    local SOXR_VERSION="$1"

    cat > "${INSTALL_PKG_CONFIG_DIR}/soxr.pc" << EOF
prefix=${BASEDIR}/prebuilt/android-$(get_target_build)/soxr
exec_prefix=\${prefix}
libdir=\${prefix}/lib
includedir=\${prefix}/include

Name: soxr
Description: High quality, one-dimensional sample-rate conversion library
Version: ${SOXR_VERSION}

Requires:
Libs: -L\${libdir} -lsoxr
Cflags: -I\${includedir}
EOF
}

create_tesseract_package_config() {
    local TESSERACT_VERSION="$1"

    cat > "${INSTALL_PKG_CONFIG_DIR}/tesseract.pc" << EOF
prefix=${BASEDIR}/prebuilt/android-$(get_target_build)/tesseract
exec_prefix=\${prefix}
bindir=\${exec_prefix}/bin
datarootdir=\${prefix}/share
datadir=\${datarootdir}
libdir=\${exec_prefix}/lib
includedir=\${prefix}/include

Name: tesseract
Description: An OCR Engine that was developed at HP Labs between 1985 and 1995... and now at Google.
URL: https://github.com/tesseract-ocr/tesseract
Version: ${TESSERACT_VERSION}

Requires: lept, libjpeg, libpng, giflib, zlib, libwebp, libtiff-4
Libs: -L\${libdir} -ltesseract -lc++_shared
Cflags: -I\${includedir}
EOF
}

create_uuid_package_config() {
    local UUID_VERSION="$1"

    cat > "${INSTALL_PKG_CONFIG_DIR}/uuid.pc" << EOF
prefix=${BASEDIR}/prebuilt/android-$(get_target_build)/libuuid
exec_prefix=\${prefix}
libdir=\${exec_prefix}/lib
includedir=\${prefix}/include

Name: uuid
Description: Universally unique id library
Version: ${UUID_VERSION}
Requires:
Cflags: -I\${includedir}
Libs: -L\${libdir} -luuid
EOF
}

create_x265_package_config() {
    local X265_VERSION="$1"

    cat > "${INSTALL_PKG_CONFIG_DIR}/x265.pc" << EOF
prefix=${BASEDIR}/prebuilt/android-$(get_target_build)/x265
exec_prefix=\${prefix}
libdir=\${prefix}/lib
includedir=\${prefix}/include

Name: x265
Description: H.265/HEVC video encoder
Version: ${X265_VERSION}

Requires:
Libs: -L\${libdir} -lx265
Libs.private: -lm -lgcc -lgcc -ldl -lgcc -lgcc -ldl -lc++_shared
Cflags: -I\${includedir}
EOF
}

create_xvidcore_package_config() {
    local XVIDCORE_VERSION="$1"

    cat > "${INSTALL_PKG_CONFIG_DIR}/xvidcore.pc" << EOF
prefix=${BASEDIR}/prebuilt/android-$(get_target_build)/xvidcore
exec_prefix=\${prefix}
libdir=\${prefix}/lib
includedir=\${prefix}/include

Name: xvidcore
Description: the main MPEG-4 de-/encoding library
Version: ${XVIDCORE_VERSION}

Requires:
Libs: -L\${libdir}
Cflags: -I\${includedir}
EOF
}

create_zlib_package_config() {
    ZLIB_VERSION=$(grep '#define ZLIB_VERSION' ${ANDROID_NDK_ROOT}/toolchains/llvm/prebuilt/${TOOLCHAIN}/sysroot/usr/include/zlib.h | grep -Eo '\".*\"' | sed -e 's/\"//g')

    cat > "${INSTALL_PKG_CONFIG_DIR}/zlib.pc" << EOF
prefix=${ANDROID_NDK_ROOT}/toolchains/llvm/prebuilt/${TOOLCHAIN}/sysroot/usr
exec_prefix=\${prefix}
libdir=${ANDROID_NDK_ROOT}/platforms/android-${API}/arch-${TOOLCHAIN_ARCH}/usr/lib
includedir=\${prefix}/include

Name: zlib
Description: zlib compression library
Version: ${ZLIB_VERSION}

Requires:
Libs: -L\${libdir} -lz
Cflags: -I\${includedir}
EOF
}

create_cpufeatures_package_config() {
    cat > "${INSTALL_PKG_CONFIG_DIR}/cpufeatures.pc" << EOF
prefix=${ANDROID_NDK_ROOT}/sources/android/cpufeatures
exec_prefix=\${prefix}
libdir=\${exec_prefix}
includedir=\${prefix}

Name: cpufeatures
Description: cpu features Android utility
Version: 1.${API}

Requires:
Libs: -L\${libdir} -lcpufeatures
Cflags: -I\${includedir}
EOF
}

#
# download <url> <local file name> <on error action>
#
download() {
    if [ ! -d "${MOBILE_FFMPEG_TMPDIR}" ]; then
        mkdir -p "${MOBILE_FFMPEG_TMPDIR}"
    fi

    (curl --fail --location $1 -o ${MOBILE_FFMPEG_TMPDIR}/$2 1>>${BASEDIR}/build.log 2>&1)

    local RC=$?

    if [ ${RC} -eq 0 ]; then
        echo -e "\nDEBUG: Downloaded $1 to ${MOBILE_FFMPEG_TMPDIR}/$2\n" 1>>${BASEDIR}/build.log 2>&1
    else
        rm -f ${MOBILE_FFMPEG_TMPDIR}/$2 1>>${BASEDIR}/build.log 2>&1

        echo -e -n "\nINFO: Failed to download $1 to ${MOBILE_FFMPEG_TMPDIR}/$2, rc=${RC}. " 1>>${BASEDIR}/build.log 2>&1

        if [ "$3" == "exit" ]; then
            echo -e "DEBUG: Build will now exit.\n" 1>>${BASEDIR}/build.log 2>&1
            exit 1
        else
            echo -e "DEBUG: Build will continue.\n" 1>>${BASEDIR}/build.log 2>&1
        fi
    fi

    echo ${RC}
}

download_gpl_library_source() {
    local GPL_LIB_URL=""
    local GPL_LIB_FILE=""
    local GPL_LIB_ORIG_DIR=""
    local GPL_LIB_DEST_DIR=""

    echo -e "\nDEBUG: Downloading GPL library source: $1\n" 1>>${BASEDIR}/build.log 2>&1

    case $1 in
        libvidstab)
            GPL_LIB_URL="https://github.com/georgmartius/vid.stab/archive/v1.1.0.tar.gz"
            GPL_LIB_FILE="v1.1.0.tar.gz"
            GPL_LIB_ORIG_DIR="vid.stab-1.1.0"
            GPL_LIB_DEST_DIR="libvidstab"
        ;;
        x264)
            GPL_LIB_URL="https://code.videolan.org/videolan/x264/-/archive/1771b556ee45207f8711744ccbd5d42a3949b14c/x264-1771b556ee45207f8711744ccbd5d42a3949b14c.tar.bz2"
            GPL_LIB_FILE="x264-1771b556ee45207f8711744ccbd5d42a3949b14c.tar.bz2"
            GPL_LIB_ORIG_DIR="x264-1771b556ee45207f8711744ccbd5d42a3949b14c"
            GPL_LIB_DEST_DIR="x264"
        ;;
        x265)
            GPL_LIB_URL="https://bitbucket.org/multicoreware/x265/downloads/x265_3.2.1.tar.gz"
            GPL_LIB_FILE="x265-3.2.1.tar.gz"
            GPL_LIB_ORIG_DIR="x265_3.2.1"
            GPL_LIB_DEST_DIR="x265"
        ;;
        xvidcore)
            GPL_LIB_URL="https://downloads.xvid.com/downloads/xvidcore-1.3.7.tar.gz"
            GPL_LIB_FILE="xvidcore-1.3.7.tar.gz"
            GPL_LIB_ORIG_DIR="xvidcore"
            GPL_LIB_DEST_DIR="xvidcore"
        ;;
    esac

    local GPL_LIB_SOURCE_PATH="${BASEDIR}/src/${GPL_LIB_DEST_DIR}"

    if [ -d "${GPL_LIB_SOURCE_PATH}" ]; then
        echo -e "INFO: $1 already downloaded. Source folder found at ${GPL_LIB_SOURCE_PATH}\n" 1>>${BASEDIR}/build.log 2>&1
        echo 0
        return
    fi

    local GPL_LIB_PACKAGE_PATH="${MOBILE_FFMPEG_TMPDIR}/${GPL_LIB_FILE}"

    echo -e "DEBUG: $1 source not found. Checking if library package ${GPL_LIB_FILE} is downloaded at ${GPL_LIB_PACKAGE_PATH} \n" 1>>${BASEDIR}/build.log 2>&1

    if [ ! -f "${GPL_LIB_PACKAGE_PATH}" ]; then
        echo -e "DEBUG: $1 library package not found. Downloading from ${GPL_LIB_URL}\n" 1>>${BASEDIR}/build.log 2>&1

        local DOWNLOAD_RC=$(download "${GPL_LIB_URL}" "${GPL_LIB_FILE}")

        if [ ${DOWNLOAD_RC} -ne 0 ]; then
            echo -e "INFO: Downloading GPL library $1 failed. Can not get library package from ${GPL_LIB_URL}\n" 1>>${BASEDIR}/build.log 2>&1
            echo ${DOWNLOAD_RC}
            return
        else
            echo -e "DEBUG: $1 library package downloaded\n" 1>>${BASEDIR}/build.log 2>&1
        fi
    else
        echo -e "DEBUG: $1 library package already downloaded\n" 1>>${BASEDIR}/build.log 2>&1
    fi

    local EXTRACT_COMMAND=""

    if [[ ${GPL_LIB_FILE} == *bz2 ]]; then
        EXTRACT_COMMAND="tar jxf ${GPL_LIB_PACKAGE_PATH} --directory ${MOBILE_FFMPEG_TMPDIR}"
    else
        EXTRACT_COMMAND="tar zxf ${GPL_LIB_PACKAGE_PATH} --directory ${MOBILE_FFMPEG_TMPDIR}"
    fi

    echo -e "DEBUG: Extracting library package ${GPL_LIB_FILE} inside ${MOBILE_FFMPEG_TMPDIR}\n" 1>>${BASEDIR}/build.log 2>&1

    ${EXTRACT_COMMAND} 1>>${BASEDIR}/build.log 2>&1

    local EXTRACT_RC=$?

    if [ ${EXTRACT_RC} -ne 0 ]; then
        echo -e "\nINFO: Downloading GPL library $1 failed. Extract for library package ${GPL_LIB_FILE} completed with rc=${EXTRACT_RC}. Deleting failed files.\n" 1>>${BASEDIR}/build.log 2>&1
        rm -f ${GPL_LIB_PACKAGE_PATH} 1>>${BASEDIR}/build.log 2>&1
        rm -rf ${MOBILE_FFMPEG_TMPDIR}/${GPL_LIB_ORIG_DIR} 1>>${BASEDIR}/build.log 2>&1
        echo ${EXTRACT_RC}
        return
    fi

    echo -e "DEBUG: Extract completed. Copying library source to ${GPL_LIB_SOURCE_PATH}\n" 1>>${BASEDIR}/build.log 2>&1

    COPY_COMMAND="cp -r ${MOBILE_FFMPEG_TMPDIR}/${GPL_LIB_ORIG_DIR} ${GPL_LIB_SOURCE_PATH}"

    ${COPY_COMMAND} 1>>${BASEDIR}/build.log 2>&1

    local COPY_RC=$?

    if [ ${COPY_RC} -eq 0 ]; then
        echo -e "DEBUG: Downloading GPL library source $1 completed successfully\n" 1>>${BASEDIR}/build.log 2>&1
    else
        echo -e "\nINFO: Downloading GPL library $1 failed. Copying library source to ${GPL_LIB_SOURCE_PATH} completed with rc=${COPY_RC}\n" 1>>${BASEDIR}/build.log 2>&1
        rm -rf ${GPL_LIB_SOURCE_PATH} 1>>${BASEDIR}/build.log 2>&1
        echo ${COPY_RC}
        return
    fi
}

set_toolchain_clang_paths() {
    export PATH=$PATH:${ANDROID_NDK_ROOT}/toolchains/llvm/prebuilt/${TOOLCHAIN}/bin

    TARGET_HOST=$(get_target_host)
    
    export AR=${TARGET_HOST}-ar
    export CC=$(get_clang_target_host)-clang
    export CXX=$(get_clang_target_host)-clang++

    if [ "$1" == "x264" ]; then
        export AS=${CC}
    else
        export AS=${TARGET_HOST}-as
    fi

    case ${ARCH} in
        arm64-v8a)
            export ac_cv_c_bigendian=no
        ;;
    esac

    export LD=${TARGET_HOST}-ld
    export RANLIB=${TARGET_HOST}-ranlib
    export STRIP=${TARGET_HOST}-strip

    export INSTALL_PKG_CONFIG_DIR="${BASEDIR}/prebuilt/android-$(get_target_build)/pkgconfig"
    export ZLIB_PACKAGE_CONFIG_PATH="${INSTALL_PKG_CONFIG_DIR}/zlib.pc"

    if [ ! -d ${INSTALL_PKG_CONFIG_DIR} ]; then
        mkdir -p ${INSTALL_PKG_CONFIG_DIR}
    fi

    if [ ! -f ${ZLIB_PACKAGE_CONFIG_PATH} ]; then
        create_zlib_package_config
    fi

    prepare_inline_sed
}

build_cpufeatures() {

    # CLEAN OLD BUILD
    rm -f ${ANDROID_NDK_ROOT}/sources/android/cpufeatures/cpu-features.o 1>>${BASEDIR}/build.log 2>&1
    rm -f ${ANDROID_NDK_ROOT}/sources/android/cpufeatures/libcpufeatures.a 1>>${BASEDIR}/build.log 2>&1
    rm -f ${ANDROID_NDK_ROOT}/sources/android/cpufeatures/libcpufeatures.so 1>>${BASEDIR}/build.log 2>&1

    echo -e "\nINFO: Building cpu-features for ${ARCH}\n" 1>>${BASEDIR}/build.log 2>&1

    set_toolchain_clang_paths "cpu-features"

    TARGET_HOST=$(get_target_host)
    export CFLAGS=$(get_cflags "cpu-features")
    export CXXFLAGS=$(get_cxxflags "cpu-features")
    export LDFLAGS=$(get_ldflags "cpu-features")

    # THEN BUILD FOR THIS ABI
    $(get_clang_target_host)-clang -c ${ANDROID_NDK_ROOT}/sources/android/cpufeatures/cpu-features.c -o ${ANDROID_NDK_ROOT}/sources/android/cpufeatures/cpu-features.o 1>>${BASEDIR}/build.log 2>&1
    ${TARGET_HOST}-ar rcs ${ANDROID_NDK_ROOT}/sources/android/cpufeatures/libcpufeatures.a ${ANDROID_NDK_ROOT}/sources/android/cpufeatures/cpu-features.o 1>>${BASEDIR}/build.log 2>&1
    $(get_clang_target_host)-clang -shared ${ANDROID_NDK_ROOT}/sources/android/cpufeatures/cpu-features.o -o ${ANDROID_NDK_ROOT}/sources/android/cpufeatures/libcpufeatures.so 1>>${BASEDIR}/build.log 2>&1

    create_cpufeatures_package_config
}

build_android_lts_support() {

    # CLEAN OLD BUILD
    rm -f ${BASEDIR}/android/app/src/main/cpp/android_lts_support.o 1>>${BASEDIR}/build.log 2>&1
    rm -f ${BASEDIR}/android/app/src/main/cpp/android_lts_support.a 1>>${BASEDIR}/build.log 2>&1

    echo -e "INFO: Building android-lts-support objects for ${ARCH}\n" 1>>${BASEDIR}/build.log 2>&1

    # PREPARING PATHS
    LIB_NAME="android-lts-support"
    set_toolchain_clang_paths ${LIB_NAME}

    # PREPARING FLAGS
    TARGET_HOST=$(get_target_host)
    CFLAGS=$(get_cflags ${LIB_NAME})
    LDFLAGS=$(get_ldflags ${LIB_NAME})

    # THEN BUILD FOR THIS ABI
    $(get_clang_target_host)-clang ${CFLAGS} -Wno-unused-command-line-argument -c ${BASEDIR}/android/app/src/main/cpp/android_lts_support.c -o ${BASEDIR}/android/app/src/main/cpp/android_lts_support.o ${LDFLAGS} 1>>${BASEDIR}/build.log 2>&1
    ${TARGET_HOST}-ar rcs ${BASEDIR}/android/app/src/main/cpp/libandroidltssupport.a ${BASEDIR}/android/app/src/main/cpp/android_lts_support.o 1>>${BASEDIR}/build.log 2>&1
}

autoreconf_library() {
    echo -e "\nDEBUG: Running full autoreconf for $1\n" 1>>${BASEDIR}/build.log 2>&1

    # TRY FULL RECONF
    (autoreconf --force --install)

    local EXTRACT_RC=$?
    if [ ${EXTRACT_RC} -eq 0 ]; then
        return
    fi

    echo -e "\nDEBUG: Full autoreconf failed. Running full autoreconf with include for $1\n" 1>>${BASEDIR}/build.log 2>&1

    # TRY FULL RECONF WITH m4
    (autoreconf --force --install -I m4)

    EXTRACT_RC=$?
    if [ ${EXTRACT_RC} -eq 0 ]; then
        return
    fi

    echo -e "\nDEBUG: Full autoreconf with include failed. Running autoreconf without force for $1\n" 1>>${BASEDIR}/build.log 2>&1

    # TRY RECONF WITHOUT FORCE
    (autoreconf --install)

    EXTRACT_RC=$?
    if [ ${EXTRACT_RC} -eq 0 ]; then
        return
    fi

    echo -e "\nDEBUG: Autoreconf without force failed. Running autoreconf without force with include for $1\n" 1>>${BASEDIR}/build.log 2>&1

    # TRY RECONF WITHOUT FORCE WITH m4
    (autoreconf --install -I m4)

    EXTRACT_RC=$?
    if [ ${EXTRACT_RC} -eq 0 ]; then
        return
    fi

    echo -e "\nDEBUG: Autoreconf without force with include failed. Running default autoreconf for $1\n" 1>>${BASEDIR}/build.log 2>&1

    # TRY DEFAULT RECONF
    (autoreconf)

    EXTRACT_RC=$?
    if [ ${EXTRACT_RC} -eq 0 ]; then
        return
    fi

    echo -e "\nDEBUG: Default autoreconf failed. Running default autoreconf with include for $1\n" 1>>${BASEDIR}/build.log 2>&1

    # TRY DEFAULT RECONF WITH m4
    (autoreconf -I m4)

    EXTRACT_RC=$?
    if [ ${EXTRACT_RC} -eq 0 ]; then
        return
    fi
}

library_is_installed() {
    local INSTALL_PATH=$1
    local LIB_NAME=$2

    echo -e "DEBUG: Checking if ${LIB_NAME} is already built and installed at ${INSTALL_PATH}/${LIB_NAME}\n" 1>>${BASEDIR}/build.log 2>&1

    if [ ! -d ${INSTALL_PATH}/${LIB_NAME} ]; then
        echo -e "DEBUG: ${INSTALL_PATH}/${LIB_NAME} directory not found\n" 1>>${BASEDIR}/build.log 2>&1
        echo 1
        return
    fi

    if [ ! -d ${INSTALL_PATH}/${LIB_NAME}/lib ]; then
        echo -e "DEBUG: ${INSTALL_PATH}/${LIB_NAME}/lib directory not found\n" 1>>${BASEDIR}/build.log 2>&1
        echo 1
        return
    fi

    if [ ! -d ${INSTALL_PATH}/${LIB_NAME}/include ]; then
        echo -e "DEBUG: ${INSTALL_PATH}/${LIB_NAME}/include directory not found\n" 1>>${BASEDIR}/build.log 2>&1
        echo 1
        return
    fi

    local HEADER_COUNT=$(ls -l ${INSTALL_PATH}/${LIB_NAME}/include | wc -l)
    local LIB_COUNT=$(ls -l ${INSTALL_PATH}/${LIB_NAME}/lib | wc -l)

    if [[ ${HEADER_COUNT} -eq 0 ]]; then
        echo -e "DEBUG: No headers found under ${INSTALL_PATH}/${LIB_NAME}/include\n" 1>>${BASEDIR}/build.log 2>&1
        echo 1
        return
    fi

    if [[ ${LIB_COUNT} -eq 0 ]]; then
        echo -e "DEBUG: No libraries found under ${INSTALL_PATH}/${LIB_NAME}/lib\n" 1>>${BASEDIR}/build.log 2>&1
        echo 1
        return
    fi

    echo -e "INFO: ${LIB_NAME} library is already built and installed\n" 1>>${BASEDIR}/build.log 2>&1

    echo 0
}
