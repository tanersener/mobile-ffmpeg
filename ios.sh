#!/bin/bash

# ARCH INDEXES
ARCH_ARMV7=0
ARCH_ARMV7S=1
ARCH_ARM64=2
ARCH_I386=3
ARCH_X86_64=4

# LIBRARY INDEXES
LIBRARY_FONTCONFIG=0
LIBRARY_FREETYPE=1
LIBRARY_FRIBIDI=2
LIBRARY_GMP=3
LIBRARY_GNUTLS=4
LIBRARY_LAME=5
LIBRARY_LIBASS=6
LIBRARY_LIBICONV=7
LIBRARY_LIBTHEORA=8
LIBRARY_LIBVORBIS=9
LIBRARY_LIBVPX=10
LIBRARY_LIBWEBP=11
LIBRARY_LIBXML2=12
LIBRARY_OPENCOREAMR=13
LIBRARY_SHINE=14
LIBRARY_SPEEX=15
LIBRARY_WAVPACK=16
LIBRARY_KVAZAAR=17
LIBRARY_GIFLIB=18
LIBRARY_JPEG=19
LIBRARY_LIBOGG=20
LIBRARY_LIBPNG=21
LIBRARY_LIBUUID=22
LIBRARY_NETTLE=23
LIBRARY_TIFF=24
LIBRARY_ZLIB=25

# ENABLE ARCH
ENABLED_ARCHITECTURES=(1 1 1 1 1)

# ENABLE LIBRARIES
ENABLED_LIBRARIES=(0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)

export BASEDIR=$(pwd)

# MIN VERSION IOS7
export IOS_MIN_VERSION=7.0

get_mobile_ffmpeg_version() {
    local MOBILE_FFMPEG_VERSION=$(grep '#define MOBILE_FFMPEG_VERSION' ${BASEDIR}/ios/src/mobileffmpeg.h | grep -Eo '\".*\"' | sed -e 's/\"//g')

    echo ${MOBILE_FFMPEG_VERSION}
}

display_help() {
    COMMAND=`echo $0 | sed -e 's/\.\///g'`

    echo -e "\n'"$COMMAND"' builds FFmpeg for IOS platform. By default five architectures (armv7, armv7s, arm64, i386 and x86_64) are built without any external libraries enabled. Options can be used to disable architectures and/or enable external libraries.\n"

    echo -e "Usage: ./"$COMMAND" [OPTION]...\n"

    echo -e "Specify environment variables as VARIABLE=VALUE to override default build options.\n"

    echo -e "Options:"

    echo -e "  -h, --help\t\t\tdisplay this help and exit"
    echo -e "  -V, --version\t\t\tdisplay version information and exit\n"

    echo -e "Platforms:"

    echo -e "  --disable-armv7\t\tdo not build armv7 platform"
    echo -e "  --disable-armv7s\t\tdo not build armv7s platform"
    echo -e "  --disable-arm64\t\tdo not build arm64 platform"
    echo -e "  --disable-i386\t\tdo not build i386 platform"
    echo -e "  --disable-x86-64\t\tdo not build x86-64 platform\n"

    echo -e "Libraries:"

    echo -e "  --full\t\t\tenables all external libraries"
    echo -e "  --enable-ios-zlib\t\tbuild with built-in zlib"
    echo -e "  --enable-fontconfig\t\tbuild with fontconfig"
    echo -e "  --enable-freetype\t\tbuild with freetype"
    echo -e "  --enable-fribidi\t\tbuild with fribidi"
    echo -e "  --enable-gnutls\t\tbuild with gnutls"
    echo -e "  --enable-gmp\t\t\tbuild with gmp"
    echo -e "  --enable-kvazaar\t\tbuild with kvazaar"
    echo -e "  --enable-lame\t\t\tbuild with lame"
    echo -e "  --enable-libass\t\tbuild with libass"
    echo -e "  --enable-libiconv\t\tbuild with libiconv"
    echo -e "  --enable-libtheora\t\tbuild with libtheora"
    echo -e "  --enable-libvorbis\t\tbuild with libvorbis"
    echo -e "  --enable-libvpx\t\tbuild with libvpx"
    echo -e "  --enable-libwebp\t\tbuild with libwebp"
    echo -e "  --enable-libxml2\t\tbuild with libxml2"
    echo -e "  --enable-opencore-amr\t\tbuild with opencore-amr"
    echo -e "  --enable-shine\t\tbuild with shine"
    echo -e "  --enable-speex\t\tbuild with speex"
    echo -e "  --enable-wavpack\t\tbuild with wavpack"
    echo -e "  --reconf-LIBRARY\t\trun autoreconf before building LIBRARY\n"
}

display_version() {
    COMMAND=`echo $0 | sed -e 's/\.\///g'`

    echo -e "\
$COMMAND $(get_mobile_ffmpeg_version)\n\
Copyright (c) 2018 Taner Sener\n\
License LGPLv3.0: GNU LGPL version 3 or later\n\
<https://www.gnu.org/licenses/lgpl-3.0.en.html>\n\
This is free software: you can redistribute it and/or modify it under the terms of the \
GNU Lesser General Public License as published by the Free Software Foundation, \
either version 3 of the License, or (at your option) any later version."
}

reconf_library() {
    RECONF_VARIABLE=$(echo "RECONF_$1" | sed "s/\-/\_/g")

    export ${RECONF_VARIABLE}=1
}

enable_library() {
    set_library $1 1
}

set_library() {
    case $1 in
        ios-zlib)
            ENABLED_LIBRARIES[LIBRARY_ZLIB]=$2
        ;;
        fontconfig)
            ENABLED_LIBRARIES[LIBRARY_FONTCONFIG]=$2
            ENABLED_LIBRARIES[LIBRARY_LIBUUID]=$2
            set_library "libxml2" $2
            set_library "freetype" $2
        ;;
        freetype)
            ENABLED_LIBRARIES[LIBRARY_FREETYPE]=$2
            ENABLED_LIBRARIES[LIBRARY_ZLIB]=$2
            set_library "libpng" $2
        ;;
        fribidi)
            ENABLED_LIBRARIES[LIBRARY_FRIBIDI]=$2
        ;;
        gmp)
            ENABLED_LIBRARIES[LIBRARY_GMP]=$2
        ;;
        gnutls)
            ENABLED_LIBRARIES[LIBRARY_GNUTLS]=$2
            ENABLED_LIBRARIES[LIBRARY_NETTLE]=$2
            ENABLED_LIBRARIES[LIBRARY_ZLIB]=$2
            set_library "gmp" $2
            set_library "libiconv" $2
        ;;
        kvazaar)
            ENABLED_LIBRARIES[LIBRARY_KVAZAAR]=$2
        ;;
        lame)
            ENABLED_LIBRARIES[LIBRARY_LAME]=$2
            set_library "libiconv" $2
        ;;
        libass)
            ENABLED_LIBRARIES[LIBRARY_LIBASS]=$2
            set_library "freetype" $2
            set_library "fribidi" $2
            set_library "fontconfig" $2
            set_library "libiconv" $2
            ENABLED_LIBRARIES[LIBRARY_LIBUUID]=$2
            set_library "libxml2" $2
        ;;
        libiconv)
            ENABLED_LIBRARIES[LIBRARY_LIBICONV]=$2
        ;;
        libpng)
            ENABLED_LIBRARIES[LIBRARY_LIBPNG]=$2
            ENABLED_LIBRARIES[LIBRARY_ZLIB]=$2
        ;;
        libtheora)
            ENABLED_LIBRARIES[LIBRARY_LIBTHEORA]=$2
            ENABLED_LIBRARIES[LIBRARY_LIBOGG]=$2
            set_library "libvorbis" $2
        ;;
        libvorbis)
            ENABLED_LIBRARIES[LIBRARY_LIBVORBIS]=$2
            ENABLED_LIBRARIES[LIBRARY_LIBOGG]=$2
        ;;
        libvpx)
            ENABLED_LIBRARIES[LIBRARY_LIBVPX]=$2
        ;;
        libwebp)
            ENABLED_LIBRARIES[LIBRARY_LIBWEBP]=$2
            ENABLED_LIBRARIES[LIBRARY_GIFLIB]=$2
            ENABLED_LIBRARIES[LIBRARY_JPEG]=$2
            ENABLED_LIBRARIES[LIBRARY_TIFF]=$2
            set_library "libpng" $2
        ;;
        libxml2)
            ENABLED_LIBRARIES[LIBRARY_LIBXML2]=$2
            set_library "libiconv" $2
        ;;
        opencore-amr)
            ENABLED_LIBRARIES[LIBRARY_OPENCOREAMR]=$2
        ;;
        shine)
            ENABLED_LIBRARIES[LIBRARY_SHINE]=$2
        ;;
        speex)
            ENABLED_LIBRARIES[LIBRARY_SPEEX]=$2
        ;;
        wavpack)
            ENABLED_LIBRARIES[LIBRARY_WAVPACK]=$2
        ;;
        giflib | jpeg | libogg | libpng | libuuid | nettle | tiff)
        ;;
        *)
            print_unknown_library $1
        ;;
    esac
}

disable_arch() {
    set_arch $1 0
}

set_arch() {
    case $1 in
        armv7)
            ENABLED_ARCHITECTURES[ARCH_ARMV7]=$2
        ;;
        armv7s)
            ENABLED_ARCHITECTURES[ARCH_ARMV7S]=$2
        ;;
        arm64)
            ENABLED_ARCHITECTURES[ARCH_ARM64]=$2
        ;;
        i386)
            ENABLED_ARCHITECTURES[ARCH_I386]=$2
        ;;
        x86-64)
            ENABLED_ARCHITECTURES[ARCH_X86_64]=$2
        ;;
        *)
            print_unknown_platform $1
        ;;
    esac
}

print_unknown_option() {
    echo -e "Unknown option \"$1\".\nSee $0 --help for available options."
    exit 1
}

print_unknown_library() {
    echo -e "Unknown library \"$1\".\nSee $0 --help for available libraries."
    exit 1
}

print_unknown_platform() {
    echo -e "Unknown platform \"$1\".\nSee $0 --help for available platforms."
    exit 1
}

print_enabled_architectures() {
    echo -n "Architectures: "

    let enabled=0;
    for print_arch in {0..4}
    do
        if [[ ${ENABLED_ARCHITECTURES[$print_arch]} -eq 1 ]]; then
            if [[ ${enabled} -ge 1 ]]; then
                echo -n ", "
            fi
            echo -n $(get_arch_name $print_arch)
            enabled=$((${enabled} + 1));
        fi
    done

    if [ ${enabled} -gt 0 ]; then
        echo ""
    else
        echo "none"
    fi
}

print_enabled_libraries() {
    echo -n "Libraries: "

    let enabled=0;

    # FIRST BUILT-IN LIBRARIES
    for library in {25..26}
    do
        if [[ ${ENABLED_LIBRARIES[$library]} -eq 1 ]]; then
            if [[ ${enabled} -ge 1 ]]; then
                echo -n ", "
            fi
            echo -n $(get_library_name $library)
            enabled=$((${enabled} + 1));
        fi
    done

    # THEN EXTERNAL LIBRARIES
    for library in {0..17}
    do
        if [[ ${ENABLED_LIBRARIES[$library]} -eq 1 ]]; then
            if [[ ${enabled} -ge 1 ]]; then
                echo -n ", "
            fi
            echo -n $(get_library_name $library)
            enabled=$((${enabled} + 1));
        fi
    done

    if [ ${enabled} -gt 0 ]; then
        echo ""
    else
        echo "none"
    fi
}

build_info_plist() {
    local FILE_PATH="$1"
    local FRAMEWORK_NAME="$2"
    local FRAMEWORK_ID="$3"
    local FRAMEWORK_SHORT_VERSION="$4"
    local FRAMEWORK_VERSION="$5"

    cat > ${FILE_PATH} <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>CFBundleDevelopmentRegion</key>
	<string>en</string>
	<key>CFBundleExecutable</key>
	<string>${FRAMEWORK_NAME}</string>
	<key>CFBundleIdentifier</key>
	<string>${FRAMEWORK_ID}</string>
	<key>CFBundleInfoDictionaryVersion</key>
	<string>6.0</string>
	<key>CFBundleName</key>
	<string>${FRAMEWORK_NAME}</string>
	<key>CFBundlePackageType</key>
	<string>FMWK</string>
	<key>CFBundleShortVersionString</key>
	<string>${FRAMEWORK_SHORT_VERSION}</string>
	<key>CFBundleVersion</key>
	<string>${FRAMEWORK_VERSION}</string>
	<key>NSPrincipalClass</key>
	<string></string>
</dict>
</plist>
EOF
}

# ENABLE COMMON FUNCTIONS
. ${BASEDIR}/build/ios-common.sh

while [ ! $# -eq 0 ]
do

    case $1 in
	    -h | --help)
            display_help
            exit 0
	    ;;
	    -V | --version)
            display_version
            exit 0
	    ;;
        --reconf-*)
            CONF_LIBRARY=`echo $1 | sed -e 's/^--[A-Za-z]*-//g'`

            reconf_library ${CONF_LIBRARY}
	    ;;
	    --full)
            for library in {0..25}
            do
                enable_library $(get_library_name $library)
            done
	    ;;
        --enable-*)
            ENABLED_LIBRARY=`echo $1 | sed -e 's/^--[A-Za-z]*-//g'`

            enable_library ${ENABLED_LIBRARY}
	    ;;
        --disable-*)
            DISABLED_ARCH=`echo $1 | sed -e 's/^--[A-Za-z]*-//g'`

            disable_arch ${DISABLED_ARCH}
	    ;;
	    *)
	        print_unknown_option $1
	    ;;
    esac
    shift
done;

echo -e "Building mobile-ffmpeg for IOS\n"

print_enabled_architectures
print_enabled_libraries

TARGET_ARCH_LIST=()

for run_arch in {0..4}
do
    if [[ ${ENABLED_ARCHITECTURES[$run_arch]} -eq 1 ]]; then
        export ARCH=$(get_arch_name $run_arch)
        export TARGET_SDK=$(get_target_sdk)
        export SDK_PATH=$(get_sdk_path)

        . ${BASEDIR}/build/main-ios.sh "${ENABLED_LIBRARIES[@]}"

        TARGET_ARCH=$(get_target_arch)
        TARGET_ARCH_LIST+=(${TARGET_ARCH})

        # CLEAR FLAGS
        for library in {1..26}
        do
            library_name=$(get_library_name $((library - 1)))
            unset $(echo "OK_${library_name}" | sed "s/\-/\_/g")
        done
    fi
done

FFMPEG_LIBS="libavcodec libavdevice libavfilter libavformat libavutil libswresample libswscale"

if [[ ! -z ${TARGET_ARCH_LIST} ]]; then

    echo -e -n "\n\nCreating fat-binary under prebuilt/ios-universal: "

    FFMPEG_UNIVERSAL=${BASEDIR}/prebuilt/ios-universal/ffmpeg-universal
    MOBILE_FFMPEG_UNIVERSAL=${BASEDIR}/prebuilt/ios-universal/mobile-ffmpeg-universal

    # BUILDING UNIVERSAL LIBRARIES
    rm -rf ${BASEDIR}/prebuilt/ios-universal

    mkdir -p ${FFMPEG_UNIVERSAL}/include
    mkdir -p ${FFMPEG_UNIVERSAL}/lib
    mkdir -p ${MOBILE_FFMPEG_UNIVERSAL}/include
    mkdir -p ${MOBILE_FFMPEG_UNIVERSAL}/lib

    # BUILDING FFMPEG FAT BINARY
    for FFMPEG_LIB in ${FFMPEG_LIBS}
    do
        LIPO_COMMAND="lipo -create"

        for TARGET_ARCH in "${TARGET_ARCH_LIST[@]}"
        do
            LIPO_COMMAND+=" ${BASEDIR}/prebuilt/ios-${TARGET_ARCH}-apple-darwin/ffmpeg/lib/${FFMPEG_LIB}.dylib"
        done

        LIPO_COMMAND+=" -output ${FFMPEG_UNIVERSAL}/lib/${FFMPEG_LIB}.dylib"

        ${LIPO_COMMAND} >> ${BASEDIR}/build.log || exit 1

        echo -e "Created fat-binary ${FFMPEG_LIB} successfully.\n" >> ${BASEDIR}/build.log
    done

    # BUILDING MOBILE FFMPEG FAT BINARY
    LIPO_COMMAND="lipo -create"

    for TARGET_ARCH in "${TARGET_ARCH_LIST[@]}"
    do
        LIPO_COMMAND+=" ${BASEDIR}/prebuilt/ios-${TARGET_ARCH}-apple-darwin/mobile-ffmpeg/lib/libmobileffmpeg.dylib"
    done

    LIPO_COMMAND+=" -output ${MOBILE_FFMPEG_UNIVERSAL}/lib/libmobileffmpeg.dylib"

    ${LIPO_COMMAND} >> ${BASEDIR}/build.log || exit 1

    # COPYING HEADERS
    cp -r ${BASEDIR}/prebuilt/ios-${TARGET_ARCH_LIST[0]}-apple-darwin/ffmpeg/include/* ${FFMPEG_UNIVERSAL}/include
    cp -r ${BASEDIR}/prebuilt/ios-${TARGET_ARCH_LIST[0]}-apple-darwin/mobile-ffmpeg/include/* ${MOBILE_FFMPEG_UNIVERSAL}/include

    echo -e "Created fat-binary mobileffmpeg successfully.\n" >> ${BASEDIR}/build.log

    echo -e "ok\n"

    # BUILDING MOBILE FFMPEG FRAMEWORK
    echo -e -n "\nCreating mobileffmpeg.framework under prebuilt/ios-framework: "

    MOBILE_FFMPEG_VERSION=$(grep '#define MOBILE_FFMPEG_VERSION' ${MOBILE_FFMPEG_UNIVERSAL}/include/mobileffmpeg.h | grep -Eo '\".*\"' | sed -e 's/\"//g')

    FRAMEWORK_PATH=${BASEDIR}/prebuilt/ios-framework/mobileffmpeg.framework

    rm -rf ${FRAMEWORK_PATH}
    mkdir -p ${FRAMEWORK_PATH}/Headers

    cp -r ${MOBILE_FFMPEG_UNIVERSAL}/include/* ${FRAMEWORK_PATH}/Headers
    cp ${FFMPEG_UNIVERSAL}/include/config.h ${FRAMEWORK_PATH}/Headers
    cp ${MOBILE_FFMPEG_UNIVERSAL}/lib/libmobileffmpeg.dylib ${FRAMEWORK_PATH}/mobileffmpeg
    cp ${BASEDIR}/LICENSE ${FRAMEWORK_PATH}

    build_info_plist "${FRAMEWORK_PATH}/Info.plist" "mobileffmpeg" "com.arthenica.mobileffmpeg.MobileFFmpeg" "${MOBILE_FFMPEG_VERSION}" "${MOBILE_FFMPEG_VERSION}"

    install_name_tool -id @rpath/mobileffmpeg.framework/mobileffmpeg ${FRAMEWORK_PATH}/mobileffmpeg

    # BUILDING SUB FRAMEWORKS

    for FFMPEG_LIB in ${FFMPEG_LIBS}
    do
        FFMPEG_LIB_UPPERCASE=$(echo ${FFMPEG_LIB} | tr '[a-z]' '[A-Z]')

        FFMPEG_LIB_MAJOR=$(grep "#define ${FFMPEG_LIB_UPPERCASE}_VERSION_MAJOR" ${FFMPEG_UNIVERSAL}/include/${FFMPEG_LIB}/version.h | sed -e "s/#define ${FFMPEG_LIB_UPPERCASE}_VERSION_MAJOR//g;s/\ //g")
        FFMPEG_LIB_MINOR=$(grep "#define ${FFMPEG_LIB_UPPERCASE}_VERSION_MINOR" ${FFMPEG_UNIVERSAL}/include/${FFMPEG_LIB}/version.h | sed -e "s/#define ${FFMPEG_LIB_UPPERCASE}_VERSION_MINOR//g;s/\ //g")
        FFMPEG_LIB_MICRO=$(grep "#define ${FFMPEG_LIB_UPPERCASE}_VERSION_MICRO" ${FFMPEG_UNIVERSAL}/include/${FFMPEG_LIB}/version.h | sed "s/#define ${FFMPEG_LIB_UPPERCASE}_VERSION_MICRO//g;s/\ //g")

        FFMPEG_LIB_VERSION="${FFMPEG_LIB_MAJOR}.${FFMPEG_LIB_MINOR}.${FFMPEG_LIB_MICRO}"

        FFMPEG_LIB_FRAMEWORK_PATH=${BASEDIR}/prebuilt/ios-framework/${FFMPEG_LIB}.framework

        rm -rf ${FFMPEG_LIB_FRAMEWORK_PATH}
        mkdir -p ${FFMPEG_LIB_FRAMEWORK_PATH}/Headers

        cp -r ${FFMPEG_UNIVERSAL}/include/${FFMPEG_LIB}/* ${FFMPEG_LIB_FRAMEWORK_PATH}/Headers
        cp ${FFMPEG_UNIVERSAL}/lib/${FFMPEG_LIB}.dylib ${FFMPEG_LIB_FRAMEWORK_PATH}/${FFMPEG_LIB}
        cp ${BASEDIR}/LICENSE ${FFMPEG_LIB_FRAMEWORK_PATH}

        build_info_plist "${FFMPEG_LIB_FRAMEWORK_PATH}/Info.plist" "${FFMPEG_LIB}" "com.arthenica.mobileffmpeg.FFmpeg${FFMPEG_LIB}" "${FFMPEG_LIB_VERSION}" "${FFMPEG_LIB_VERSION}"

        install_name_tool -id @rpath/${FFMPEG_LIB}.framework/${FFMPEG_LIB} ${FFMPEG_LIB_FRAMEWORK_PATH}/${FFMPEG_LIB}
    done

    # FIXING PATHS
    ALL_LIBS=("${FFMPEG_LIBS[@]}")
    ALL_LIBS+=" mobileffmpeg"

    for ONE_LIB in ${ALL_LIBS}
    do
        for TARGET_ARCH in "${TARGET_ARCH_LIST[@]}"
        do
            install_name_tool -change ${BASEDIR}/prebuilt/ios-${TARGET_ARCH}-apple-darwin/mobile-ffmpeg/lib/libmobileffmpeg.0.dylib @rpath/mobileffmpeg.framework/mobileffmpeg ${BASEDIR}/prebuilt/ios-framework/${ONE_LIB}.framework/${ONE_LIB}

            install_name_tool -change ${BASEDIR}/prebuilt/ios-${TARGET_ARCH}-apple-darwin/ffmpeg/lib/libavcodec.57.dylib @rpath/libavcodec.framework/libavcodec ${BASEDIR}/prebuilt/ios-framework/${ONE_LIB}.framework/${ONE_LIB}
            install_name_tool -change ${BASEDIR}/prebuilt/ios-${TARGET_ARCH}-apple-darwin/ffmpeg/lib/libavdevice.57.dylib @rpath/libavdevice.framework/libavdevice ${BASEDIR}/prebuilt/ios-framework/${ONE_LIB}.framework/${ONE_LIB}
            install_name_tool -change ${BASEDIR}/prebuilt/ios-${TARGET_ARCH}-apple-darwin/ffmpeg/lib/libavfilter.6.dylib @rpath/libavfilter.framework/libavfilter ${BASEDIR}/prebuilt/ios-framework/${ONE_LIB}.framework/${ONE_LIB}
            install_name_tool -change ${BASEDIR}/prebuilt/ios-${TARGET_ARCH}-apple-darwin/ffmpeg/lib/libavformat.57.dylib @rpath/libavformat.framework/libavformat ${BASEDIR}/prebuilt/ios-framework/${ONE_LIB}.framework/${ONE_LIB}
            install_name_tool -change ${BASEDIR}/prebuilt/ios-${TARGET_ARCH}-apple-darwin/ffmpeg/lib/libswresample.2.dylib @rpath/libswresample.framework/libswresample ${BASEDIR}/prebuilt/ios-framework/${ONE_LIB}.framework/${ONE_LIB}
            install_name_tool -change ${BASEDIR}/prebuilt/ios-${TARGET_ARCH}-apple-darwin/ffmpeg/lib/libswscale.4.dylib @rpath/libswscale.framework/libswscale ${BASEDIR}/prebuilt/ios-framework/${ONE_LIB}.framework/${ONE_LIB}
            install_name_tool -change ${BASEDIR}/prebuilt/ios-${TARGET_ARCH}-apple-darwin/ffmpeg/lib/libavutil.55.dylib @rpath/libavutil.framework/libavutil ${BASEDIR}/prebuilt/ios-framework/${ONE_LIB}.framework/${ONE_LIB}
        done
    done

    echo -e "Created mobile-ffmpeg.framework successfully.\n" >> ${BASEDIR}/build.log

    echo -e "ok\n"
fi