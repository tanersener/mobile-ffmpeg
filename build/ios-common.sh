#!/bin/bash

get_cpu_count() {
    if [ "$(uname)" == "Darwin" ]; then
        echo $(sysctl -n hw.physicalcpu)
    else
        echo $(nproc)
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
        21) echo "frei0r" ;;
        22) echo "libvidstab" ;;
        23) echo "libilbc" ;;
        24) echo "opus" ;;
        25) echo "snappy" ;;
        26) echo "soxr" ;;
        27) echo "libaom" ;;
        28) echo "chromaprint" ;;
        29) echo "giflib" ;;
        30) echo "jpeg" ;;
        31) echo "libogg" ;;
        32) echo "libpng" ;;
        33) echo "libuuid" ;;
        34) echo "nettle" ;;
        35) echo "tiff" ;;
        36) echo "expat" ;;
        37) echo "ios-zlib" ;;
        38) echo "ios-audiotoolbox" ;;
        39) echo "ios-coreimage" ;;
        40) echo "ios-bzip2" ;;
    esac
}

get_arch_name() {
    case $1 in
        0) echo "armv7" ;;
        1) echo "armv7s" ;;
        2) echo "arm64" ;;
        3) echo "i386" ;;
        4) echo "x86-64" ;;
    esac
}

get_target_host() {
    echo "$(get_target_arch)-apple-darwin"
}

get_target_arch() {
    case ${ARCH} in
        arm64)
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
        armv7 | armv7s | arm64)
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
    local min_version=${IOS_MIN_VERSION}

    if [ "$1" == "gnutls" ]; then
        min_version=${GNUTLS_IOS_MIN_VERSION}
    fi

    case ${ARCH} in
        armv7 | armv7s | arm64)
            echo "-miphoneos-version-min=${min_version}"
        ;;
        i386 | x86-64)
            echo "-mios-simulator-version-min=${min_version}"
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
            echo "-arch armv7 -target $(get_target_host) -march=armv7 -mcpu=cortex-a8 -mfpu=neon -mfloat-abi=softfp"
        ;;
        armv7s)
            echo "-arch armv7s -target $(get_target_host) -march=armv7s -mcpu=generic -mfpu=neon -mfloat-abi=softfp"
        ;;
        arm64)
            echo "-arch arm64 -target $(get_target_host) -march=armv8-a+crc+crypto -mcpu=generic"
        ;;
        i386)
            echo "-arch i386 -target $(get_target_host) -march=i386 -mtune=intel -mssse3 -mfpmath=sse -m32"
        ;;
        x86-64)
            echo "-arch x86_64 -target $(get_target_host) -march=x86-64 -msse4.2 -mpopcnt -m64 -mtune=intel"
        ;;
    esac
}

get_size_optimization_cflags() {

    ARCH_OPTIMIZATION=""
    case ${ARCH} in
        armv7 | armv7s | arm64)
            if [[ $1 -eq libwebp ]]; then
                ARCH_OPTIMIZATION="-Os -Wno-ignored-optimization-argument"
            else
                ARCH_OPTIMIZATION="-Os -finline-limit=64 -Wno-ignored-optimization-argument"
            fi
        ;;
        i386 | x86-64)
            if [[ $1 -eq libvpx ]]; then
                ARCH_OPTIMIZATION="-O2 -Wno-ignored-optimization-argument"
            else
                ARCH_OPTIMIZATION="-O2 -finline-limit=300 -Wno-ignored-optimization-argument"
            fi

        ;;
    esac

    echo "${ARCH_OPTIMIZATION}"
}

get_app_specific_cflags() {

    APP_FLAGS=""
    case $1 in
        libwebp | xvidcore)
            APP_FLAGS="-fno-common -DPIC"
        ;;
        ffmpeg | shine)
            APP_FLAGS="-Wno-unused-function"
        ;;
        soxr | snappy)
            APP_FLAGS="-std=gnu99 -Wno-unused-function -DPIC"
        ;;
        kvazaar)
            APP_FLAGS="-std=gnu99 -Wno-unused-function"
        ;;
        mobile-ffmpeg)
            APP_FLAGS="-std=c99 -Wno-unused-function -Wall -Wno-deprecated-declarations -Wno-pointer-sign -Wno-switch -Wno-unused-result -Wno-unused-variable"
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
    ARCH_FLAGS=$(get_arch_specific_cflags)
    APP_FLAGS=$(get_app_specific_cflags $1)
    COMMON_FLAGS=$(get_common_cflags)
    OPTIMIZATION_FLAGS=$(get_size_optimization_cflags $1)
    MIN_VERSION_FLAGS=$(get_min_version_cflags $1)
    COMMON_INCLUDES=$(get_common_includes)

    echo "${ARCH_FLAGS} ${APP_FLAGS} ${COMMON_FLAGS} ${OPTIMIZATION_FLAGS} ${MIN_VERSION_FLAGS} ${COMMON_INCLUDES}"
}

get_asmflags() {
    ARCH_FLAGS=$(get_arch_specific_cflags)
    APP_FLAGS=$(get_app_specific_cflags $1)
    COMMON_FLAGS=$(get_common_cflags)
    OPTIMIZATION_FLAGS=$(get_size_optimization_cflags $1)
    MIN_VERSION_FLAGS=$(get_min_version_cflags $1)
    COMMON_INCLUDES=$(get_common_includes)

    echo "${ARCH_FLAGS} ${APP_FLAGS} ${COMMON_FLAGS} ${OPTIMIZATION_FLAGS} ${MIN_VERSION_FLAGS} ${COMMON_INCLUDES}"
}

get_cxxflags() {
    local COMMON_CFLAGS="$(get_common_cflags $1) $(get_common_includes $1) $(get_arch_specific_cflags $1) $(get_min_version_cflags $1)"

    case $1 in
        x265)
            echo "-std=c++11 -fno-exceptions -fembed-bitcode ${COMMON_CFLAGS}"
        ;;
        gnutls)
            echo "-std=c++11 -fno-rtti -fembed-bitcode ${COMMON_CFLAGS}"
        ;;
        opencore-amr)
            echo "-fno-rtti -fembed-bitcode ${COMMON_CFLAGS}"
        ;;
        libwebp | xvidcore)
            echo "-std=c++11 -fno-exceptions -fno-rtti -fembed-bitcode -fno-common -DPIC ${COMMON_CFLAGS}"
        ;;
        libaom)
            echo "-std=c++11 -fno-exceptions -fembed-bitcode ${COMMON_CFLAGS}"
        ;;
        *)
            echo "-std=c++11 -fno-exceptions -fno-rtti -fembed-bitcode ${COMMON_CFLAGS}"
        ;;
    esac
}

get_common_linked_libraries() {
    echo "-L${SDK_PATH}/usr/lib -lc++"
}

get_common_ldflags() {
    echo "-isysroot ${SDK_PATH}"
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
        i386)
            echo "-arch i386 -march=i386"
        ;;
        x86-64)
            echo "-arch x86_64 -march=x86-64"
        ;;
    esac
}

get_ldflags() {
    ARCH_FLAGS=$(get_arch_specific_ldflags)
    LINKED_LIBRARIES=$(get_common_linked_libraries)
    COMMON_FLAGS=$(get_common_ldflags)

    echo "${ARCH_FLAGS} ${LINKED_LIBRARIES} ${COMMON_FLAGS}"
}

create_fontconfig_package_config() {
    local FONTCONFIG_VERSION="$1"

    cat > "${INSTALL_PKG_CONFIG_DIR}/fontconfig.pc" << EOF
prefix=${BASEDIR}/prebuilt/ios-$(get_target_host)/fontconfig
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
prefix=${BASEDIR}/prebuilt/ios-$(get_target_host)/freetype
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
prefix=${BASEDIR}/prebuilt/ios-$(get_target_host)/giflib
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
prefix=${BASEDIR}/prebuilt/ios-$(get_target_host)/gmp
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
prefix=${BASEDIR}/prebuilt/ios-$(get_target_host)/gnutls
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
prefix=${BASEDIR}/prebuilt/ios-$(get_target_host)/lame
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
prefix=${BASEDIR}/prebuilt/ios-$(get_target_host)/libiconv
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
prefix=${BASEDIR}/prebuilt/ios-$(get_target_host)/libpng
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
prefix=${BASEDIR}/prebuilt/ios-$(get_target_host)/libvorbis
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
prefix=${BASEDIR}/prebuilt/ios-$(get_target_host)/libvorbis
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
prefix=${BASEDIR}/prebuilt/ios-$(get_target_host)/libvorbis
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
prefix=${BASEDIR}/prebuilt/ios-$(get_target_host)/libwebp
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
prefix=${BASEDIR}/prebuilt/ios-$(get_target_host)/libxml2
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
prefix=${BASEDIR}/prebuilt/ios-$(get_target_host)/snappy
exec_prefix=\${prefix}
libdir=\${prefix}/lib
includedir=\${prefix}/include

Name: snappy
Description: a fast compressor/decompressor
Version: ${SNAPPY_VERSION}

Requires:
Libs: -L\${libdir} -lz -lstdc++
Cflags: -I\${includedir}
EOF
}

create_soxr_package_config() {
    local SOXR_VERSION="$1"

    cat > "${INSTALL_PKG_CONFIG_DIR}/soxr.pc" << EOF
prefix=${BASEDIR}/prebuilt/ios-$(get_target_host)/soxr
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

create_uuid_package_config() {
    local UUID_VERSION="$1"

    cat > "${INSTALL_PKG_CONFIG_DIR}/uuid.pc" << EOF
prefix=${BASEDIR}/prebuilt/ios-$(get_target_host)/libuuid
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
prefix=${BASEDIR}/prebuilt/ios-$(get_target_host)/xvidcore
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

    (curl --fail --location $1 -o ${MOBILE_FFMPEG_TMPDIR}/$2) 1>>${BASEDIR}/build.log 2>>${BASEDIR}/build.log

    local RC=$?

    if [ ${RC} -eq 0 ]; then
        echo -e "\nDEBUG: Downloaded $1 to ${MOBILE_FFMPEG_TMPDIR}/$2\n" >>${BASEDIR}/build.log
    else
        rm -f ${MOBILE_FFMPEG_TMPDIR}/$2 >>${BASEDIR}/build.log

        echo -e -n "\nINFO: Failed to download $1 to ${MOBILE_FFMPEG_TMPDIR}/$2, rc=${RC}. " >>${BASEDIR}/build.log

        if [ "$3" == "exit" ]; then
            echo -e "DEBUG: Build will now exit.\n" >>${BASEDIR}/build.log
            exit 1
        else
            echo -e "DEBUG: Build will continue.\n" >>${BASEDIR}/build.log
        fi
    fi

    echo ${RC}
}

download_gpl_library_source() {
    local GPL_LIB_URL=""
    local GPL_LIB_FILE=""
    local GPL_LIB_ORIG_DIR=""
    local GPL_LIB_DEST_DIR=""

    echo -e "\nDEBUG: Downloading GPL library source: $1\n" >>${BASEDIR}/build.log

    case $1 in
        frei0r)
            GPL_LIB_URL="https://files.dyne.org/frei0r/frei0r-plugins-1.6.1.tar.gz"
            GPL_LIB_FILE="frei0r-plugins-1.6.1.tar.gz"
            GPL_LIB_ORIG_DIR="frei0r-plugins-1.6.1"
            GPL_LIB_DEST_DIR="frei0r"
        ;;
        libvidstab)
            GPL_LIB_URL="https://github.com/georgmartius/vid.stab/archive/v1.1.0.tar.gz"
            GPL_LIB_FILE="v1.1.0.tar.gz"
            GPL_LIB_ORIG_DIR="vid.stab-1.1.0"
            GPL_LIB_DEST_DIR="libvidstab"
        ;;
        x264)
            GPL_LIB_URL="ftp://ftp.videolan.org/pub/videolan/x264/snapshots/x264-snapshot-20180718-2245-stable.tar.bz2"
            GPL_LIB_FILE="x264-snapshot-20180718-2245-stable.tar.bz2"
            GPL_LIB_ORIG_DIR="x264-snapshot-20180718-2245-stable"
            GPL_LIB_DEST_DIR="x264"
        ;;
        x265)
            GPL_LIB_URL="https://download.videolan.org/pub/videolan/x265/x265_2.8.tar.gz"
            GPL_LIB_FILE="x265-2.8.tar.gz"
            GPL_LIB_ORIG_DIR="x265-2.8"
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
        echo -e "INFO: $1 already downloaded. Source folder found at ${GPL_LIB_SOURCE_PATH}\n" >>${BASEDIR}/build.log
        echo 0
        return
    fi

    local GPL_LIB_PACKAGE_PATH="${MOBILE_FFMPEG_TMPDIR}/${GPL_LIB_FILE}"

    echo -e "DEBUG: $1 source not found. Checking if library package ${GPL_LIB_FILE} is downloaded at ${GPL_LIB_PACKAGE_PATH} \n" >>${BASEDIR}/build.log

    if [ ! -f "${GPL_LIB_PACKAGE_PATH}" ]; then
        echo -e "DEBUG: $1 library package not found. Downloading from ${GPL_LIB_URL}\n" >>${BASEDIR}/build.log

        local DOWNLOAD_RC=$(download "${GPL_LIB_URL}" "${GPL_LIB_FILE}")

        if [ ${DOWNLOAD_RC} -ne 0 ]; then
            echo -e "INFO: Downloading GPL library $1 failed. Can not get library package from ${GPL_LIB_URL}\n" >>${BASEDIR}/build.log
            echo ${DOWNLOAD_RC}
            return
        else
            echo -e "DEBUG: $1 library package downloaded\n" >>${BASEDIR}/build.log
        fi
    else
        echo -e "DEBUG: $1 library package already downloaded\n" >>${BASEDIR}/build.log
    fi

    local EXTRACT_COMMAND=""

    if [[ ${GPL_LIB_FILE} == *bz2 ]]; then
        EXTRACT_COMMAND="tar jxf ${GPL_LIB_PACKAGE_PATH} --directory ${MOBILE_FFMPEG_TMPDIR}"
    else
        EXTRACT_COMMAND="tar zxf ${GPL_LIB_PACKAGE_PATH} --directory ${MOBILE_FFMPEG_TMPDIR}"
    fi

    echo -e "DEBUG: Extracting library package ${GPL_LIB_FILE} inside ${MOBILE_FFMPEG_TMPDIR}\n" >>${BASEDIR}/build.log

    ${EXTRACT_COMMAND}

    local EXTRACT_RC=$?

    if [ ${EXTRACT_RC} -ne 0 ]; then
        echo -e "\nINFO: Downloading GPL library $1 failed. Extract for library package ${GPL_LIB_FILE} completed with rc=${EXTRACT_RC}. Deleting failed files.\n" >>${BASEDIR}/build.log
        echo ${EXTRACT_RC}
        rm -f ${GPL_LIB_PACKAGE_PATH} >>${BASEDIR}/build.log
        rm -rf ${MOBILE_FFMPEG_TMPDIR}/${GPL_LIB_ORIG_DIR} >>${BASEDIR}/build.log
        return
    fi

    echo -e "DEBUG: Extract completed. Copying library source to ${GPL_LIB_SOURCE_PATH}\n" >>${BASEDIR}/build.log

    CP_RC=$(cp -r ${MOBILE_FFMPEG_TMPDIR}/${GPL_LIB_ORIG_DIR} ${GPL_LIB_SOURCE_PATH} >>${BASEDIR}/build.log)

    if [[ ${CP_RC} -eq 0 ]]; then
        echo -e "DEBUG: Downloading GPL library source $1 completed successfully\n" >>${BASEDIR}/build.log
        echo ${CP_RC}
    else
        echo -e "INFO: Downloading GPL library $1 failed. Copying library source to ${GPL_LIB_SOURCE_PATH} completed with rc={CP_RC}\n" >>${BASEDIR}/build.log
        rm -rf ${GPL_LIB_SOURCE_PATH} >>${BASEDIR}/build.log
        echo ${CP_RC}
    fi
}

set_toolchain_clang_paths() {
    if [ ! -f "${MOBILE_FFMPEG_TMPDIR}/gas-preprocessor.pl" ]; then
        download "https://github.com/libav/gas-preprocessor/raw/master/gas-preprocessor.pl" "gas-preprocessor.pl" "exit"
        (chmod +x ${MOBILE_FFMPEG_TMPDIR}/gas-preprocessor.pl >>${BASEDIR}/build.log) || exit 1
    fi

    local GAS_PREPROCESSOR="${MOBILE_FFMPEG_TMPDIR}/gas-preprocessor.pl"
    if [ "$1" == "x264" ]; then
        GAS_PREPROCESSOR="${BASEDIR}/src/x264/tools/gas-preprocessor.pl"
    fi

    TARGET_HOST=$(get_target_host)
    
    export AR="$(xcrun --sdk $(get_sdk_name) -f ar)"
    export CC="$(xcrun --sdk $(get_sdk_name) -f clang)"
    export OBJC="$(xcrun --sdk $(get_sdk_name) -f clang)"
    export CXX="$(xcrun --sdk $(get_sdk_name) -f clang++)"

    local ASMFLAGS="$(get_asmflags $1)"
    case ${ARCH} in
        armv7 | armv7s)
            if [ "$1" == "x265" ]; then
                export AS="${GAS_PREPROCESSOR}"
                export AS_ARGUMENTS="-arch arm"
                export ASM_FLAGS="${ASMFLAGS}"
            else
                export AS="${GAS_PREPROCESSOR} -arch arm -- ${CC} ${ASMFLAGS}"
            fi
        ;;
        arm64)
            if [ "$1" == "x265" ]; then
                export AS="${GAS_PREPROCESSOR}"
                export AS_ARGUMENTS="-arch aarch64"
                export ASM_FLAGS="${ASMFLAGS}"
            else
                export AS="${GAS_PREPROCESSOR} -arch aarch64 -- ${CC} ${ASMFLAGS}"
            fi
        ;;
        *)
            export AS="${CC} ${ASMFLAGS}"
        ;;
    esac

    export LD="$(xcrun --sdk $(get_sdk_name) -f ld)"
    export RANLIB="$(xcrun --sdk $(get_sdk_name) -f ranlib)"
    export STRIP="$(xcrun --sdk $(get_sdk_name) -f strip)"

    export INSTALL_PKG_CONFIG_DIR="${BASEDIR}/prebuilt/ios-$(get_target_host)/pkgconfig"
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
}

autoreconf_library() {
    echo -e "\nDEBUG: Running full autoreconf for $1\n" >> ${BASEDIR}/build.log

    # TRY FULL RECONF
    (autoreconf --force --install)

    local EXTRACT_RC=$?
    if [ ${EXTRACT_RC} -eq 0 ]; then
        return
    fi

    echo -e "\nDEBUG: Full autoreconf failed. Running full autoreconf with include for $1\n" >> ${BASEDIR}/build.log

    # TRY FULL RECONF WITH m4
    (autoreconf --force --install -I m4)

    EXTRACT_RC=$?
    if [ ${EXTRACT_RC} -eq 0 ]; then
        return
    fi

    echo -e "\nDEBUG: Full autoreconf with include failed. Running autoreconf without force for $1\n" >> ${BASEDIR}/build.log

    # TRY RECONF WITHOUT FORCE
    (autoreconf --install)

    EXTRACT_RC=$?
    if [ ${EXTRACT_RC} -eq 0 ]; then
        return
    fi

    echo -e "\nDEBUG: Autoreconf without force failed. Running autoreconf without force with include for $1\n" >> ${BASEDIR}/build.log

    # TRY RECONF WITHOUT FORCE WITH m4
    (autoreconf --install -I m4)

    EXTRACT_RC=$?
    if [ ${EXTRACT_RC} -eq 0 ]; then
        return
    fi

    echo -e "\nDEBUG: Autoreconf without force with include failed. Running default autoreconf for $1\n" >> ${BASEDIR}/build.log

    # TRY DEFAULT RECONF
    (autoreconf)

    EXTRACT_RC=$?
    if [ ${EXTRACT_RC} -eq 0 ]; then
        return
    fi

    echo -e "\nDEBUG: Default autoreconf failed. Running default autoreconf with include for $1\n" >> ${BASEDIR}/build.log

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

    echo -e "DEBUG: Checking if ${LIB_NAME} is already built and installed at ${INSTALL_PATH}/${LIB_NAME}\n" >> ${BASEDIR}/build.log

    if [ ! -d ${INSTALL_PATH}/${LIB_NAME} ]; then
        echo -e "DEBUG: ${INSTALL_PATH}/${LIB_NAME} directory not found\n" >> ${BASEDIR}/build.log
        echo 1
        return
    fi

    if [ ! -d ${INSTALL_PATH}/${LIB_NAME}/lib ]; then
        echo -e "DEBUG: ${INSTALL_PATH}/${LIB_NAME}/lib directory not found\n" >> ${BASEDIR}/build.log
        echo 1
        return
    fi

    if [ ! -d ${INSTALL_PATH}/${LIB_NAME}/include ]; then
        echo -e "DEBUG: ${INSTALL_PATH}/${LIB_NAME}/include directory not found\n" >> ${BASEDIR}/build.log
        echo 1
        return
    fi

    local HEADER_COUNT=$(ls -l ${INSTALL_PATH}/${LIB_NAME}/include | wc -l)
    local LIB_COUNT=$(ls -l ${INSTALL_PATH}/${LIB_NAME}/lib | wc -l)

    if [[ ${HEADER_COUNT} -eq 0 ]]; then
        echo -e "DEBUG: No headers found under ${INSTALL_PATH}/${LIB_NAME}/include\n" >> ${BASEDIR}/build.log
        echo 1
        return
    fi

    if [[ ${LIB_COUNT} -eq 0 ]]; then
        echo -e "DEBUG: No libraries found under ${INSTALL_PATH}/${LIB_NAME}/lib\n" >> ${BASEDIR}/build.log
        echo 1
        return
    fi

    echo -e "INFO: ${LIB_NAME} library is already built and installed\n" >> ${BASEDIR}/build.log

    echo 0
}