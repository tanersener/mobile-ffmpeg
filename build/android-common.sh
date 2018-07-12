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
        20) echo "libilbc" ;;
        21) echo "opus" ;;
        22) echo "snappy" ;;
        23) echo "soxr" ;;
        24) echo "libaom" ;;
        25) echo "giflib" ;;
        26) echo "jpeg" ;;
        27) echo "libogg" ;;
        28) echo "libpng" ;;
        29) echo "libuuid" ;;
        30) echo "nettle" ;;
        31) echo "tiff" ;;
        32) echo "expat" ;;
        33) echo "android-zlib" ;;
        34) echo "android-media-codec" ;;
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

get_toolchain() {
    case ${ARCH} in
        arm-v7a | arm-v7a-neon)
            echo "arm"
        ;;
        arm64-v8a)
            echo "aarch64"
        ;;
        x86)
            echo "i686"
        ;;
        x86-64)
            echo "x86_64"
        ;;
    esac
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
            echo "arm/neon"
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
    echo "-I${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-${TOOLCHAIN}/sysroot/usr/include -I${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-${TOOLCHAIN}/sysroot/usr/local/include"
}

get_common_cflags() {
    echo "-fstrict-aliasing -fPIC -DANDROID -D__ANDROID_API__=${API}"
}

get_arch_specific_cflags() {
    case ${ARCH} in
        arm-v7a)
            echo "-march=armv7-a -mfpu=vfpv3-d16 -mfloat-abi=softfp"
        ;;
        arm-v7a-neon)
            echo "-march=armv7-a -mfpu=neon -mfloat-abi=softfp"
        ;;
        arm64-v8a)
            echo "-march=armv8-a"
        ;;
        x86)
            echo "-march=i686 -mtune=intel -mssse3 -mfpmath=sse -m32"
        ;;
        x86-64)
            echo "-march=x86-64 -msse4.2 -mpopcnt -m64 -mtune=intel"
        ;;
    esac
}

get_size_optimization_cflags() {

    ARCH_OPTIMIZATION=""
    case ${ARCH} in
        arm-v7a | arm-v7a-neon | arm64-v8a)
            if [[ $1 -eq libwebp ]]; then
                ARCH_OPTIMIZATION="-Os"
            else
                ARCH_OPTIMIZATION="-Os -finline-limit=64"
            fi
        ;;
        x86 | x86-64)
            if [[ $1 -eq libvpx ]]; then
                ARCH_OPTIMIZATION="-O2"
            else
                ARCH_OPTIMIZATION="-O2 -finline-limit=300"
            fi

        ;;
    esac

    LIB_OPTIMIZATION=""

    echo "${ARCH_OPTIMIZATION} ${LIB_OPTIMIZATION}"
}

get_app_specific_cflags() {

    APP_FLAGS=""
    case $1 in
        xvidcore)
            APP_FLAGS=""
        ;;
        ffmpeg | shine)
            APP_FLAGS="-Wno-unused-function"
        ;;
        soxr | snappy | libwebp)
            APP_FLAGS="-std=gnu99 -Wno-unused-function -DPIC"
        ;;
        kvazaar)
            APP_FLAGS="-std=gnu99 -Wno-unused-function"
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
    COMMON_INCLUDES=$(get_common_includes)

    echo "${ARCH_FLAGS} ${APP_FLAGS} ${COMMON_FLAGS} ${OPTIMIZATION_FLAGS} ${COMMON_INCLUDES}"
}

get_cxxflags() {
    case $1 in
        gnutls)
            echo "-std=c++11 -fno-rtti"
        ;;
        opencore-amr)
            echo ""
        ;;
        *)
            echo "-std=c++11 -fno-exceptions -fno-rtti"
        ;;
    esac
}

get_common_linked_libraries() {
    case $1 in
        ffmpeg)
            echo "-lc -lm -ldl -llog -L${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-${TOOLCHAIN}/sysroot/usr/lib -L${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-${TOOLCHAIN}/lib -L${ANDROID_NDK_ROOT}/sources/cxx-stl/llvm-libc++/libs/${ANDROID_ARCH}"
        ;;
        libvpx)
            echo "-lc -lm -L${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-${TOOLCHAIN}/sysroot/usr/lib -L${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-${TOOLCHAIN}/lib -L${ANDROID_NDK_ROOT}/sources/cxx-stl/llvm-libc++/libs/${ANDROID_ARCH}"
        ;;
        *)
            echo "-lc -lm -ldl -llog -L${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-${TOOLCHAIN}/sysroot/usr/lib -L${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-${TOOLCHAIN}/lib -L${ANDROID_NDK_ROOT}/sources/cxx-stl/llvm-libc++/libs/${ANDROID_ARCH}"
        ;;
    esac
}

get_size_optimization_ldflags() {
    case ${ARCH} in
        arm64-v8a)
            echo "-Wl,--gc-sections"
        ;;
        *)
            echo "-Wl,--gc-sections,--icf=safe"
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
    ARCH_FLAGS=$(get_arch_specific_ldflags)
    OPTIMIZATION_FLAGS=$(get_size_optimization_ldflags)
    COMMON_LINKED_LIBS=$(get_common_linked_libraries $1)

    echo "${ARCH_FLAGS} ${OPTIMIZATION_FLAGS} ${COMMON_LINKED_LIBS}"
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
Description: AV1 codec library v0.1.0.
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
Libs: -L\${libdir} -lz -lstdc++
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
    ZLIB_VERSION=$(grep '#define ZLIB_VERSION' ${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-${TOOLCHAIN}/sysroot/usr/include/zlib.h | grep -Eo '\".*\"' | sed -e 's/\"//g')

    cat > "${INSTALL_PKG_CONFIG_DIR}/zlib.pc" << EOF
prefix=${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-${TOOLCHAIN}/sysroot/usr
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
        x264)
            GPL_LIB_URL="ftp://ftp.videolan.org/pub/videolan/x264/snapshots/x264-snapshot-20180627-2245-stable.tar.bz2"
            GPL_LIB_FILE="x264-snapshot-20180627-2245-stable.tar.bz2"
            GPL_LIB_ORIG_DIR="x264-snapshot-20180627-2245-stable"
            GPL_LIB_DEST_DIR="x264"
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
    export PATH=$PATH:${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-${TOOLCHAIN}/bin

    TARGET_HOST=$(get_target_host)
    
    export AR=${TARGET_HOST}-ar
    export CC=${TARGET_HOST}-clang
    export CXX=${TARGET_HOST}-clang++

    if [ "$1" == "x264" ]; then
        export AS=${CC}
    else
        export AS=${TARGET_HOST}-as
    fi

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
}

set_toolchain_gcc_paths() {
    export PATH=$PATH:${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-${TOOLCHAIN}/bin

    TARGET_HOST=$(get_target_host)

    export AR=${TARGET_HOST}-ar
    export CC=${TARGET_HOST}-gcc
    export CXX=${TARGET_HOST}-gcc++

    if [ "$1" == "x264" ]; then
        export AS=${CC}
    else
        export AS=${TARGET_HOST}-as
    fi

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
}

create_toolchain() {
    local TOOLCHAIN_DIR="${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-"${TOOLCHAIN}

    if [ ! -d ${TOOLCHAIN_DIR} ]; then
        ${ANDROID_NDK_ROOT}/build/tools/make_standalone_toolchain.py --arch ${TOOLCHAIN_ARCH} --api ${API} --stl libc++ --install-dir ${TOOLCHAIN_DIR} || exit 1
    fi
}

build_cpufeatures() {

    # CLEAN OLD BUILD
    rm -f ${ANDROID_NDK_ROOT}/sources/android/cpufeatures/cpu-features.o >> ${BASEDIR}/build.log
    rm -f ${ANDROID_NDK_ROOT}/sources/android/cpufeatures/libcpufeatures.a >> ${BASEDIR}/build.log
    rm -f ${ANDROID_NDK_ROOT}/sources/android/cpufeatures/libcpufeatures.so >> ${BASEDIR}/build.log

    set_toolchain_clang_paths "cpu-features"

    echo -e "\nINFO: Building cpu-features for for ${ARCH}\n" >> ${BASEDIR}/build.log

    # THEN BUILD FOR THIS ABI
    ${TARGET_HOST}-clang -c ${ANDROID_NDK_ROOT}/sources/android/cpufeatures/cpu-features.c -o ${ANDROID_NDK_ROOT}/sources/android/cpufeatures/cpu-features.o >> ${BASEDIR}/build.log
    ${TARGET_HOST}-ar rcs ${ANDROID_NDK_ROOT}/sources/android/cpufeatures/libcpufeatures.a ${ANDROID_NDK_ROOT}/sources/android/cpufeatures/cpu-features.o >> ${BASEDIR}/build.log
    ${TARGET_HOST}-clang -shared ${ANDROID_NDK_ROOT}/sources/android/cpufeatures/cpu-features.o -o ${ANDROID_NDK_ROOT}/sources/android/cpufeatures/libcpufeatures.so >> ${BASEDIR}/build.log

    create_cpufeatures_package_config
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