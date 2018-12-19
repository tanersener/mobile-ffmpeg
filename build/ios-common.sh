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
        31) echo "giflib" ;;
        32) echo "jpeg" ;;
        33) echo "libogg" ;;
        34) echo "libpng" ;;
        35) echo "libuuid" ;;
        36) echo "nettle" ;;
        37) echo "tiff" ;;
        38) echo "expat" ;;
        39) echo "libsndfile" ;;
        40) echo "leptonica" ;;
        41) echo "ios-zlib" ;;
        42) echo "ios-audiotoolbox" ;;
        43) echo "ios-coreimage" ;;
        44) echo "ios-bzip2" ;;
        45) echo "ios-videotoolbox" ;;
        46) echo "ios-avfoundation" ;;
    esac
}

get_static_archive_name() {
    case $1 in
        5) echo "libmp3lame.a" ;;
        6) echo "libass.a" ;;
        10) echo "libvpx.a" ;;
        12) echo "libxml2.a" ;;
        21) echo "libvidstab.a" ;;
        22) echo "libilbc.a" ;;
        26) echo "libaom.a" ;;
        28) echo "libtwolame.a" ;;
        29) echo "libSDL2.a" ;;
        30) echo "libtesseract.a" ;;
        31) echo "libgif.a" ;;
        33) echo "libogg.a" ;;
        34) echo "libpng.a" ;;
        35) echo "libuuid.a" ;;
        39) echo "libsndfile.a" ;;
        40) echo "liblept.a" ;;
        *) echo lib$(get_library_name $1).a
    esac
}

get_arch_name() {
    case $1 in
        0) echo "armv7" ;;
        1) echo "armv7s" ;;
        2) echo "arm64" ;;
        3) echo "arm64e" ;;
        4) echo "i386" ;;
        5) echo "x86-64" ;;
    esac
}

get_target_host() {
    echo "$(get_target_arch)-ios-darwin"
}

get_target_build_directory() {
    case ${ARCH} in
        x86-64)
            echo "x86_64-ios-darwin"
        ;;
        *)
            echo "${ARCH}-ios-darwin"
        ;;
    esac
}

get_target_arch() {
    case ${ARCH} in
        arm64 | arm64e)
            echo "aarch64"
        ;;
        x86-64)
            echo "x86_64"
        ;;
        *)
            echo "${ARCH}"
        ;;
    esac
}

get_target_sdk() {
    echo "$(get_target_arch)-apple-ios${IOS_MIN_VERSION}"
}

get_sdk_name() {
    case ${ARCH} in
        armv7 | armv7s | arm64 | arm64e)
            echo "iphoneos"
        ;;
        i386 | x86-64)
            echo "iphonesimulator"
        ;;
    esac
}

get_sdk_path() {
    echo "$(xcrun --sdk $(get_sdk_name) --show-sdk-path)"
}

get_min_version_cflags() {
    case ${ARCH} in
        armv7 | armv7s | arm64 | arm64e)
            echo "-miphoneos-version-min=${IOS_MIN_VERSION}"
        ;;
        i386 | x86-64)
            echo "-mios-simulator-version-min=${IOS_MIN_VERSION}"
        ;;
    esac
}

get_common_includes() {
    echo "-I${SDK_PATH}/usr/include"
}

get_common_cflags() {
    case ${ARCH} in
        i386 | x86-64)
            echo "-fstrict-aliasing -DIOS -isysroot ${SDK_PATH}"
        ;;
        *)
            echo "-fstrict-aliasing -fembed-bitcode -DIOS -isysroot ${SDK_PATH}"
        ;;
    esac
}

get_arch_specific_cflags() {
    case ${ARCH} in
        armv7)
            echo "-arch armv7 -target $(get_target_host) -march=armv7 -mcpu=cortex-a8 -mfpu=neon -mfloat-abi=softfp -DMOBILE_FFMPEG_ARMV7"
        ;;
        armv7s)
            echo "-arch armv7s -target $(get_target_host) -march=armv7s -mcpu=generic -mfpu=neon -mfloat-abi=softfp -DMOBILE_FFMPEG_ARMV7S"
        ;;
        arm64)
            echo "-arch arm64 -target $(get_target_host) -march=armv8-a+crc+crypto -mcpu=generic -DMOBILE_FFMPEG_ARM64"
        ;;
        arm64e)
            echo "-arch arm64e -target $(get_target_host) -march=armv8.3-a+dotprod -mcpu=generic -DMOBILE_FFMPEG_ARM64E"
        ;;
        i386)
            echo "-arch i386 -target $(get_target_host) -march=i386 -mtune=intel -mssse3 -mfpmath=sse -m32 -DMOBILE_FFMPEG_I386"
        ;;
        x86-64)
            echo "-arch x86_64 -target $(get_target_host) -march=x86-64 -msse4.2 -mpopcnt -m64 -mtune=intel -DMOBILE_FFMPEG_X86_64"
        ;;
    esac
}

get_size_optimization_cflags() {

    local ARCH_OPTIMIZATION=""
    case ${ARCH} in
        armv7 | armv7s | arm64 | arm64e)
            case $1 in
                x264)
                    ARCH_OPTIMIZATION="-Oz -Wno-ignored-optimization-argument"
                ;;
                x265 | ffmpeg | mobile-ffmpeg)
                    ARCH_OPTIMIZATION="-flto=thin -Oz -Wno-ignored-optimization-argument"
                ;;
                *)
                    ARCH_OPTIMIZATION="-flto -Oz -Wno-ignored-optimization-argument"
                ;;
            esac
        ;;
        i386 | x86-64)
            case $1 in
                x264 | ffmpeg)
                    ARCH_OPTIMIZATION="-O2 -Wno-ignored-optimization-argument"
                ;;
                x265)
                    ARCH_OPTIMIZATION="-flto=thin -O2 -Wno-ignored-optimization-argument"
                ;;
                *)
                    ARCH_OPTIMIZATION="-flto -O2 -Wno-ignored-optimization-argument"
                ;;
            esac
        ;;
    esac

    echo "${ARCH_OPTIMIZATION}"
}

get_size_optimization_asm_cflags() {

    local ARCH_OPTIMIZATION=""
    case $1 in
        jpeg | ffmpeg)
            case ${ARCH} in
                armv7 | armv7s | arm64 | arm64e)
                    ARCH_OPTIMIZATION="-Oz"
                ;;
                i386 | x86-64)
                    ARCH_OPTIMIZATION="-O2"
                ;;
            esac
        ;;
        *)
            ARCH_OPTIMIZATION=$(get_size_optimization_cflags $1)
        ;;
    esac

    echo "${ARCH_OPTIMIZATION}"
}

get_app_specific_cflags() {

    local APP_FLAGS=""
    case $1 in
        fontconfig)
            case ${ARCH} in
                armv7 | armv7s | arm64 | arm64e)
                    APP_FLAGS="-std=c99 -Wno-unused-function -D__IPHONE_OS_MIN_REQUIRED -D__IPHONE_VERSION_MIN_REQUIRED=30000"
                ;;
                *)
                    APP_FLAGS="-std=c99 -Wno-unused-function"
                ;;
            esac
        ;;
        ffmpeg)
            APP_FLAGS="-Wno-unused-function -DPIC"
        ;;
        kvazaar)
            APP_FLAGS="-std=gnu99 -Wno-unused-function"
        ;;
        leptonica)
            APP_FLAGS="-std=c99 -Wno-unused-function -DOS_IOS"
        ;;
        libwebp | xvidcore)
            APP_FLAGS="-fno-common -DPIC"
        ;;
        mobile-ffmpeg)
            APP_FLAGS="-std=c99 -Wno-unused-function -Wall -Wno-deprecated-declarations -Wno-pointer-sign -Wno-switch -Wno-unused-result -Wno-unused-variable -DPIC -fobjc-arc"
        ;;
        sdl2)
            APP_FLAGS="-DPIC -Wno-unused-function -D__IPHONEOS__"
        ;;
        shine)
            APP_FLAGS="-Wno-unused-function"
        ;;
        soxr | snappy)
            APP_FLAGS="-std=gnu99 -Wno-unused-function -DPIC"
        ;;
        x265)
            APP_FLAGS="-Wno-unused-function"
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
    local MIN_VERSION_FLAGS=$(get_min_version_cflags $1)
    local COMMON_INCLUDES=$(get_common_includes)

    echo "${ARCH_FLAGS} ${APP_FLAGS} ${COMMON_FLAGS} ${OPTIMIZATION_FLAGS} ${MIN_VERSION_FLAGS} ${COMMON_INCLUDES}"
}

get_asmflags() {
    local ARCH_FLAGS=$(get_arch_specific_cflags)
    local APP_FLAGS=$(get_app_specific_cflags $1)
    local COMMON_FLAGS=$(get_common_cflags)
    if [[ -z ${MOBILE_FFMPEG_DEBUG} ]]; then
        local OPTIMIZATION_FLAGS=$(get_size_optimization_asm_cflags $1)
    else
        local OPTIMIZATION_FLAGS="${MOBILE_FFMPEG_DEBUG}"
    fi
    local MIN_VERSION_FLAGS=$(get_min_version_cflags $1)
    local COMMON_INCLUDES=$(get_common_includes)

    echo "${ARCH_FLAGS} ${APP_FLAGS} ${COMMON_FLAGS} ${OPTIMIZATION_FLAGS} ${MIN_VERSION_FLAGS} ${COMMON_INCLUDES}"
}

get_cxxflags() {
    local COMMON_CFLAGS="$(get_common_cflags $1) $(get_common_includes $1) $(get_arch_specific_cflags) $(get_min_version_cflags $1)"
    if [[ -z ${MOBILE_FFMPEG_DEBUG} ]]; then
        local OPTIMIZATION_FLAGS="-Oz"
    else
        local OPTIMIZATION_FLAGS="${MOBILE_FFMPEG_DEBUG}"
    fi

    local BITCODE_FLAGS=""
    case ${ARCH} in
        armv7 | armv7s | arm64 | arm64e)
            local BITCODE_FLAGS="-fembed-bitcode"
        ;;
    esac

    case $1 in
        x265)
            echo "-std=c++11 -fno-exceptions ${BITCODE_FLAGS} ${COMMON_CFLAGS} ${OPTIMIZATION_FLAGS}"
        ;;
        gnutls)
            echo "-std=c++11 -fno-rtti ${BITCODE_FLAGS} ${COMMON_CFLAGS} ${OPTIMIZATION_FLAGS}"
        ;;
        opencore-amr)
            echo "-fno-rtti ${BITCODE_FLAGS} ${COMMON_CFLAGS} ${OPTIMIZATION_FLAGS}"
        ;;
        libwebp | xvidcore)
            echo "-std=c++11 -fno-exceptions -fno-rtti ${BITCODE_FLAGS} -fno-common -DPIC ${COMMON_CFLAGS} ${OPTIMIZATION_FLAGS}"
        ;;
        libaom)
            echo "-std=c++11 -fno-exceptions ${BITCODE_FLAGS} ${COMMON_CFLAGS} ${OPTIMIZATION_FLAGS}"
        ;;
        *)
            echo "-std=c++11 -fno-exceptions -fno-rtti ${BITCODE_FLAGS} ${COMMON_CFLAGS} ${OPTIMIZATION_FLAGS}"
        ;;
    esac
}

get_common_linked_libraries() {
    echo "-L${SDK_PATH}/usr/lib -lc++"
}

get_common_ldflags() {
    echo "-isysroot ${SDK_PATH}"
}

get_size_optimization_ldflags() {
    case ${ARCH} in
        armv7 | armv7s | arm64 | arm64e)
            case $1 in
                ffmpeg | mobile-ffmpeg)
                    echo "-flto=thin -Oz"
                ;;
                *)
                    echo "-flto -Oz"
                ;;
            esac
        ;;
        *)
            case $1 in
                ffmpeg)
                    echo "-O2"
                ;;
                *)
                    echo "-flto -O2"
                ;;
            esac
        ;;
    esac
}

get_arch_specific_ldflags() {
    case ${ARCH} in
        armv7)
            echo "-arch armv7 -march=armv7 -mfpu=neon -mfloat-abi=softfp -fembed-bitcode"
        ;;
        armv7s)
            echo "-arch armv7s -march=armv7s -mfpu=neon -mfloat-abi=softfp -fembed-bitcode"
        ;;
        arm64)
            echo "-arch arm64 -march=armv8-a+crc+crypto -fembed-bitcode"
        ;;
        arm64e)
            echo "-arch arm64e -march=armv8.3-a+dotprod -fembed-bitcode"
        ;;
        i386)
            echo "-arch i386 -march=i386"
        ;;
        x86-64)
            echo "-arch x86_64 -march=x86-64"
        ;;
    esac
}

get_ldflags() {
    local ARCH_FLAGS=$(get_arch_specific_ldflags)
    local LINKED_LIBRARIES=$(get_common_linked_libraries)
    if [[ -z ${MOBILE_FFMPEG_DEBUG} ]]; then
        local OPTIMIZATION_FLAGS="$(get_size_optimization_ldflags $1)"
    else
        local OPTIMIZATION_FLAGS="${MOBILE_FFMPEG_DEBUG}"
    fi
    local COMMON_FLAGS=$(get_common_ldflags)

    case $1 in
        mobile-ffmpeg)
            case ${ARCH} in
                armv7 | armv7s | arm64 | arm64e)
                    echo "${ARCH_FLAGS} ${LINKED_LIBRARIES} ${COMMON_FLAGS} -fembed-bitcode -Wc,-fembed-bitcode ${OPTIMIZATION_FLAGS}"
                ;;
                *)
                    echo "${ARCH_FLAGS} ${LINKED_LIBRARIES} ${COMMON_FLAGS} ${OPTIMIZATION_FLAGS}"
                ;;
            esac
        ;;
        *)
            echo "${ARCH_FLAGS} ${LINKED_LIBRARIES} ${COMMON_FLAGS} ${OPTIMIZATION_FLAGS}"
        ;;
    esac
}

create_fontconfig_package_config() {
    local FONTCONFIG_VERSION="$1"

    cat > "${INSTALL_PKG_CONFIG_DIR}/fontconfig.pc" << EOF
prefix=${BASEDIR}/prebuilt/ios-$(get_target_build_directory)/fontconfig
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
prefix=${BASEDIR}/prebuilt/ios-$(get_target_build_directory)/freetype
exec_prefix=\${prefix}
libdir=\${exec_prefix}/lib
includedir=\${prefix}/include

Name: FreeType 2
URL: https://freetype.org
Description: A free, high-quality, and portable font engine.
Version: ${FREETYPE_VERSION}
Requires: libpng
Requires.private:
Libs: -L\${libdir} -lfreetype
Libs.private:
Cflags: -I\${includedir}/freetype2
EOF
}

create_giflib_package_config() {
    local GIFLIB_VERSION="$1"

    cat > "${INSTALL_PKG_CONFIG_DIR}/giflib.pc" << EOF
prefix=${BASEDIR}/prebuilt/ios-$(get_target_build_directory)/giflib
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
prefix=${BASEDIR}/prebuilt/ios-$(get_target_build_directory)/gmp
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
prefix=${BASEDIR}/prebuilt/ios-$(get_target_build_directory)/gnutls
exec_prefix=\${prefix}
libdir=\${exec_prefix}/lib
includedir=\${prefix}/include

Name: gnutls
Description: GNU TLS Implementation

Version: ${GNUTLS_VERSION}
Requires: nettle, hogweed
Cflags: -I\${includedir}
Libs: -L\${libdir} -lgnutls
Libs.private: -lgmp
EOF
}

create_libmp3lame_package_config() {
    local LAME_VERSION="$1"

    cat > "${INSTALL_PKG_CONFIG_DIR}/libmp3lame.pc" << EOF
prefix=${BASEDIR}/prebuilt/ios-$(get_target_build_directory)/lame
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

create_libiconv_package_config() {
    local LIB_ICONV_VERSION="$1"

    cat > "${INSTALL_PKG_CONFIG_DIR}/libiconv.pc" << EOF
prefix=${BASEDIR}/prebuilt/ios-$(get_target_build_directory)/libiconv
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

create_libpng_package_config() {
    local LIBPNG_VERSION="$1"

    cat > "${INSTALL_PKG_CONFIG_DIR}/libpng.pc" << EOF
prefix=${BASEDIR}/prebuilt/ios-$(get_target_build_directory)/libpng
exec_prefix=\${prefix}
libdir=\${exec_prefix}/lib
includedir=\${prefix}/include

Name: libpng
Description: Loads and saves PNG files
Version: ${LIBPNG_VERSION}
Requires:
Cflags: -I\${includedir}
Libs: -L\${libdir} -lpng
EOF
}

create_libvorbis_package_config() {
    local LIBVORBIS_VERSION="$1"

    cat > "${INSTALL_PKG_CONFIG_DIR}/vorbis.pc" << EOF
prefix=${BASEDIR}/prebuilt/ios-$(get_target_build_directory)/libvorbis
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
prefix=${BASEDIR}/prebuilt/ios-$(get_target_build_directory)/libvorbis
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
prefix=${BASEDIR}/prebuilt/ios-$(get_target_build_directory)/libvorbis
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
prefix=${BASEDIR}/prebuilt/ios-$(get_target_build_directory)/libwebp
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
prefix=${BASEDIR}/prebuilt/ios-$(get_target_build_directory)/libxml2
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
prefix=${BASEDIR}/prebuilt/ios-$(get_target_build_directory)/snappy
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
prefix=${BASEDIR}/prebuilt/ios-$(get_target_build_directory)/soxr
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
prefix=${BASEDIR}/prebuilt/ios-$(get_target_build_directory)/tesseract
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

Requires: lept
Libs: -L\${libdir} -ltesseract
Cflags: -I\${includedir}
EOF
}

create_uuid_package_config() {
    local UUID_VERSION="$1"

    cat > "${INSTALL_PKG_CONFIG_DIR}/uuid.pc" << EOF
prefix=${BASEDIR}/prebuilt/ios-$(get_target_build_directory)/libuuid
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

create_xvidcore_package_config() {
    local XVIDCORE_VERSION="$1"

    cat > "${INSTALL_PKG_CONFIG_DIR}/xvidcore.pc" << EOF
prefix=${BASEDIR}/prebuilt/ios-$(get_target_build_directory)/xvidcore
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
    ZLIB_VERSION=$(grep '#define ZLIB_VERSION' ${SDK_PATH}/usr/include/zlib.h | grep -Eo '\".*\"' | sed -e 's/\"//g')

    cat > "${INSTALL_PKG_CONFIG_DIR}/zlib.pc" << EOF
prefix=${SDK_PATH}/usr
exec_prefix=\${prefix}
libdir=\${exec_prefix}/lib
includedir=\${prefix}/include

Name: zlib
Description: zlib compression library
Version: ${ZLIB_VERSION}

Requires:
Libs: -L\${libdir} -lz
Cflags: -I\${includedir}
EOF
}

create_bzip2_package_config() {
    BZIP2_VERSION=$(grep -Eo 'version.*of' ${SDK_PATH}/usr/include/bzlib.h | sed -e 's/of//;s/version//g;s/\ //g')

    cat > "${INSTALL_PKG_CONFIG_DIR}/bzip2.pc" << EOF
prefix=${SDK_PATH}/usr
exec_prefix=\${prefix}
libdir=\${exec_prefix}/lib
includedir=\${prefix}/include

Name: bzip2
Description: library for lossless, block-sorting data compression
Version: ${BZIP2_VERSION}

Requires:
Libs: -L\${libdir} -lbz2
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
            GPL_LIB_URL="ftp://ftp.videolan.org/pub/videolan/x264/snapshots/x264-snapshot-20181208-2245-stable.tar.bz2"
            GPL_LIB_FILE="x264-snapshot-20181208-2245-stable.tar.bz2"
            GPL_LIB_ORIG_DIR="x264-snapshot-20181208-2245-stable"
            GPL_LIB_DEST_DIR="x264"
        ;;
        x265)
            GPL_LIB_URL="https://download.videolan.org/pub/videolan/x265/x265_2.9.tar.gz"
            GPL_LIB_FILE="x265-2.9.tar.gz"
            GPL_LIB_ORIG_DIR="x265_2.9"
            GPL_LIB_DEST_DIR="x265"
        ;;
        xvidcore)
            GPL_LIB_URL="https://downloads.xvid.com/downloads/xvidcore-1.3.5.tar.gz"
            GPL_LIB_FILE="xvidcore-1.3.5.tar.gz"
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
    if [ ! -f "${MOBILE_FFMPEG_TMPDIR}/gas-preprocessor.pl" ]; then
        DOWNLOAD_RESULT=$(download "https://github.com/libav/gas-preprocessor/raw/master/gas-preprocessor.pl" "gas-preprocessor.pl" "exit")
        if [[ ${DOWNLOAD_RESULT} -ne 0 ]]; then
            exit 1
        fi
        (chmod +x ${MOBILE_FFMPEG_TMPDIR}/gas-preprocessor.pl 1>>${BASEDIR}/build.log 2>&1) || exit 1

        # patch gas-preprocessor.pl against the following warning
        # Unescaped left brace in regex is deprecated here (and will be fatal in Perl 5.32), passed through in regex; marked by <-- HERE in m/(?:ld|st)\d\s+({ <-- HERE \s*v(\d+)\.(\d[bhsdBHSD])\s*-\s*v(\d+)\.(\d[bhsdBHSD])\s*})/ at /Users/taner/Projects/mobile-ffmpeg/.tmp/gas-preprocessor.pl line 1065.
        sed -i .tmp "s/s\+({/s\+(\\\\{/g;s/s\*})/s\*\\\\})/g" ${MOBILE_FFMPEG_TMPDIR}/gas-preprocessor.pl
    fi

    LOCAL_GAS_PREPROCESSOR="${MOBILE_FFMPEG_TMPDIR}/gas-preprocessor.pl"
    if [ "$1" == "x264" ]; then
        LOCAL_GAS_PREPROCESSOR="${BASEDIR}/src/x264/tools/gas-preprocessor.pl"
    fi

    TARGET_HOST=$(get_target_host)
    
    export AR="$(xcrun --sdk $(get_sdk_name) -f ar)"
    export CC="$(xcrun --sdk $(get_sdk_name) -f clang)"
    export OBJC="$(xcrun --sdk $(get_sdk_name) -f clang)"
    export CXX="$(xcrun --sdk $(get_sdk_name) -f clang++)"

    LOCAL_ASMFLAGS="$(get_asmflags $1)"
    case ${ARCH} in
        armv7 | armv7s)
            if [ "$1" == "x265" ]; then
                export AS="${LOCAL_GAS_PREPROCESSOR}"
                export AS_ARGUMENTS="-arch arm"
                export ASM_FLAGS="${LOCAL_ASMFLAGS}"
            else
                export AS="${LOCAL_GAS_PREPROCESSOR} -arch arm -- ${CC} ${LOCAL_ASMFLAGS}"
            fi
        ;;
        arm64 | arm64e)
            if [ "$1" == "x265" ]; then
                export AS="${LOCAL_GAS_PREPROCESSOR}"
                export AS_ARGUMENTS="-arch aarch64"
                export ASM_FLAGS="${LOCAL_ASMFLAGS}"
            else
                export AS="${LOCAL_GAS_PREPROCESSOR} -arch aarch64 -- ${CC} ${LOCAL_ASMFLAGS}"
            fi
        ;;
        *)
            export AS="${CC} ${LOCAL_ASMFLAGS}"
        ;;
    esac

    export LD="$(xcrun --sdk $(get_sdk_name) -f ld)"
    export RANLIB="$(xcrun --sdk $(get_sdk_name) -f ranlib)"
    export STRIP="$(xcrun --sdk $(get_sdk_name) -f strip)"

    export INSTALL_PKG_CONFIG_DIR="${BASEDIR}/prebuilt/ios-$(get_target_build_directory)/pkgconfig"
    export ZLIB_PACKAGE_CONFIG_PATH="${INSTALL_PKG_CONFIG_DIR}/zlib.pc"
    export BZIP2_PACKAGE_CONFIG_PATH="${INSTALL_PKG_CONFIG_DIR}/bzip2.pc"

    if [ ! -d ${INSTALL_PKG_CONFIG_DIR} ]; then
        mkdir -p ${INSTALL_PKG_CONFIG_DIR}
    fi

    if [ ! -f ${ZLIB_PACKAGE_CONFIG_PATH} ]; then
        create_zlib_package_config
    fi

    if [ ! -f ${BZIP2_PACKAGE_CONFIG_PATH} ]; then
        create_bzip2_package_config
    fi

    prepare_inline_sed
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