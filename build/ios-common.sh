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
        18) echo "giflib" ;;
        19) echo "jpeg" ;;
        20) echo "libogg" ;;
        21) echo "libpng" ;;
        22) echo "libuuid" ;;
        23) echo "nettle" ;;
        24) echo "tiff" ;;
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
        armv7 | armv7s | i386)
            echo "${ARCH}"
        ;;
        arm64)
            echo "aarch64"
        ;;
        x86-64)
            echo "x86_64"
        ;;
    esac
}

get_target_sdk() {
    echo "-target $(get_target_arch)-apple-ios${IOS_MIN_VERSION}"
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
    case ${ARCH} in
        armv7 | armv7s | arm64)
            echo "-miphoneos-version-min=${IOS_MIN_VERSION}"
        ;;
        i386 | x86-64)
            echo "-mios-simulator-version-min=${IOS_MIN_VERSION}"
        ;;
    esac
}

get_common_cflags() {
    echo "-fstrict-aliasing -DIOS -isysroot ${SDK_PATH}"
}

get_arch_specific_cflags() {
    case ${ARCH} in
        armv7)
            echo "-arch=armv7 -cpu=cortex-a8 -mfpu=neon -mfloat-abi=softfp"
        ;;
        armv7s)
            echo "-arch=armv7s -cpu=generic -mfpu=neon -mfloat-abi=softfp"
        ;;
        arm64)
            echo "-arch=aarch64 -cpu=generic"
        ;;
        x86)
            echo "-arch=i386 -cpu=generic -mtune=intel -mssse3 -mfpmath=sse -m32"
        ;;
        x86-64)
            echo "-arch=x86-64 -cpu=generic -msse4.2 -mpopcnt -m64 -mtune=intel"
        ;;
    esac
}

get_size_optimization_cflags() {

    ARCH_OPTIMIZATION=""
    case ${ARCH} in
        armv7 | armv7s | arm64)
            if [[ $1 -eq libwebp ]]; then
                ARCH_OPTIMIZATION="-Os"
            else
                ARCH_OPTIMIZATION="-Os -finline-limit=64"
            fi
        ;;
        i386 | x86-64)
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
        libwebp)
            APP_FLAGS=""
        ;;
        ffmpeg | shine)
            APP_FLAGS="-Wno-unused-function"
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
    ARCH_FLAGS=$(get_arch_specific_cflags);
    APP_FLAGS=$(get_app_specific_cflags $1);
    COMMON_FLAGS=$(get_common_cflags);
    OPTIMIZATION_FLAGS=$(get_size_optimization_cflags $1);
    MIN_VERSION_FLAGS=$(get_min_version_cflags);

    echo "${ARCH_FLAGS} ${APP_FLAGS} ${COMMON_FLAGS} ${OPTIMIZATION_FLAGS} ${MIN_VERSION_FLAGS}"
}

get_cxxflags() {
    case $1 in
        gnutls)
            echo "-nostdi -std=c++11 -fno-exceptions -fno-rtti"
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
    echo ""
}

get_size_optimization_ldflags() {
    case ${ARCH} in
        arm64)
            echo "-Wl,--gc-sections"
        ;;
        *)
            echo "-Wl,--gc-sections,--icf=safe"
        ;;
    esac
}

get_arch_specific_ldflags() {
    case ${ARCH} in
        armv7)
            echo "-march=armv7 -cpu=cortex-a8 -mfpu=neon -mfloat-abi=softfp -Wl,--fix-cortex-a8"
        ;;
        armv7s)
            echo "-arch=armv7s -cpu=generic -mfpu=neon -mfloat-abi=softfp -Wl,--fix-cortex-a8"
        ;;
        arm64)
            echo "-arch=aarch64"
        ;;
        i386)
            echo "-arch=i386"
        ;;
        x86-64)
            echo "-arch=x86-64"
        ;;
    esac
}

get_ldflags() {
    ARCH_FLAGS=$(get_arch_specific_ldflags);
    OPTIMIZATION_FLAGS=$(get_size_optimization_ldflags);
    COMMON_LINKED_LIBS=$(get_common_linked_libraries $1);

    echo "${ARCH_FLAGS} ${OPTIMIZATION_FLAGS} ${COMMON_LINKED_LIBS}"
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
Requires:  freetype2 >= 21.0.15, uuid, libxml-2.0 >= 2.6
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
Requires.private: zlib
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
Requires: nettle, hogweed, zlib
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

set_toolchain_clang_paths() {
    (curl -L https://github.com/libav/gas-preprocessor/raw/master/gas-preprocessor.pl \
			-o /tmp/gas-preprocessor.pl && chmod +x /tmp/gas-preprocessor.pl) || exit 1

    TARGET_HOST=$(get_target_host)
    
    export AR="$(xcrun --sdk $(get_sdk_name) -f ar)"
    export CC="$(xcrun --sdk $(get_sdk_name) -f clang)"
    export CXX="$(xcrun --sdk $(get_sdk_name) -f clang++)"
    export AS="/tmp/gas-preprocessor.pl -arch $(get_target_arch) -as-type clang -- ${CC}"
    export LD="$(xcrun --sdk $(get_sdk_name) -f ld)"
    export RANLIB="$(xcrun --sdk $(get_sdk_name) -f ranlib)"
    export STRIP="$(xcrun --sdk $(get_sdk_name) -f strip)"

    export INSTALL_PKG_CONFIG_DIR="${BASEDIR}/prebuilt/ios-$(get_target_host)/pkgconfig"
    export ZLIB_PACKAGE_CONFIG_PATH="${INSTALL_PKG_CONFIG_DIR}/zlib.pc"

    if [ ! -d ${INSTALL_PKG_CONFIG_DIR} ]; then
        mkdir -p ${INSTALL_PKG_CONFIG_DIR}
    fi

    if [ ! -f ${ZLIB_PACKAGE_CONFIG_PATH} ]; then
        create_zlib_package_config
    fi
}
