#!/bin/bash

# ARCH INDEXES
ARCH_ARMV7=0
ARCH_ARMV7S=1
ARCH_ARM64=2
ARCH_ARM64E=3
ARCH_I386=4
ARCH_X86_64=5

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
LIBRARY_X264=18
LIBRARY_XVIDCORE=19
LIBRARY_X265=20
LIBRARY_LIBVIDSTAB=21
LIBRARY_LIBILBC=22
LIBRARY_OPUS=23
LIBRARY_SNAPPY=24
LIBRARY_SOXR=25
LIBRARY_LIBAOM=26
LIBRARY_CHROMAPRINT=27
LIBRARY_TWOLAME=28
LIBRARY_SDL=29
LIBRARY_TESSERACT=30
LIBRARY_GIFLIB=31
LIBRARY_JPEG=32
LIBRARY_LIBOGG=33
LIBRARY_LIBPNG=34
LIBRARY_LIBUUID=35
LIBRARY_NETTLE=36
LIBRARY_TIFF=37
LIBRARY_EXPAT=38
LIBRARY_SNDFILE=39
LIBRARY_LEPTONICA=40
LIBRARY_ZLIB=41
LIBRARY_AUDIOTOOLBOX=42
LIBRARY_COREIMAGE=43
LIBRARY_BZIP2=44
LIBRARY_VIDEOTOOLBOX=45
LIBRARY_AVFOUNDATION=46

# ENABLE ARCH
ENABLED_ARCHITECTURES=(1 1 1 1 1 1)

# ENABLE LIBRARIES
ENABLED_LIBRARIES=(0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)

export BASEDIR=$(pwd)

export MOBILE_FFMPEG_TMPDIR="${BASEDIR}/.tmp"

# //@TEST THIS AND INCREASE IF NECESSARY
export IOS_MIN_VERSION=9.3

get_mobile_ffmpeg_version() {
    local MOBILE_FFMPEG_VERSION=$(grep 'MOBILE_FFMPEG_VERSION' ${BASEDIR}/ios/src/MobileFFmpeg.m | grep -Eo '\".*\"' | sed -e 's/\"//g')

    echo ${MOBILE_FFMPEG_VERSION}
}

display_help() {
    COMMAND=`echo $0 | sed -e 's/\.\///g'`

    if [[ -z ${MOBILE_FFMPEG_LTS_BUILD} ]]; then
        echo -e "\n'"$COMMAND"' builds FFmpeg and MobileFFmpeg for IOS platform. By default six architectures (armv7, armv7s, arm64, arm64e, i386 and x86_64) are built without any external libraries enabled. Options can be used to disable architectures and/or enable external libraries. Please note that GPL libraries (external libraries with GPL license) need --enable-gpl flag to be set explicitly. When compilation ends a universal fat binary and an IOS framework is created with enabled architectures inside.\n"
    else
        echo -e "\n'"$COMMAND"' builds FFmpeg and MobileFFmpeg for IOS platform. By default five architectures (armv7, armv7s, arm64, i386 and x86_64) are built without any external libraries enabled. Options can be used to disable architectures and/or enable external libraries. Please note that GPL libraries (external libraries with GPL license) need --enable-gpl flag to be set explicitly. When compilation ends a universal fat binary and an IOS framework is created with enabled architectures inside.\n"
    fi

    echo -e "Usage: ./"$COMMAND" [OPTION]...\n"

    echo -e "Specify environment variables as VARIABLE=VALUE to override default build options.\n"

    echo -e "Options:"

    echo -e "  -h, --help\t\t\tdisplay this help and exit"
    echo -e "  -V, --version\t\t\tdisplay version information and exit"
    echo -e "  -d, --debug\t\t\tbuild with debug information"
    echo -e "  -s, --speed\t\t\toptimize for speed instead of size"
    echo -e "  -l, --lts\t\t\tbuild lts packages to support sdk 9.3+ devices, does not include arm64e architecture"
    echo -e "  -f, --force\t\t\tignore warnings\n"

    echo -e "Licensing options:"

    echo -e "  --enable-gpl\t\t\tallow use of GPL libraries, resulting libs will be licensed under GPLv3.0 [no]\n"

    echo -e "Platforms:"

    echo -e "  --disable-armv7\t\tdo not build armv7 platform [yes]"
    echo -e "  --disable-armv7s\t\tdo not build armv7s platform [yes]"
    echo -e "  --disable-arm64\t\tdo not build arm64 platform [yes]"
    if [[ -z ${MOBILE_FFMPEG_LTS_BUILD} ]]; then
        echo -e "  --disable-arm64e\t\tdo not build arm64e platform [yes]"
    fi
    echo -e "  --disable-i386\t\tdo not build i386 platform [yes]"
    echo -e "  --disable-x86-64\t\tdo not build x86-64 platform [yes]\n"

    echo -e "Libraries:"

    echo -e "  --full\t\t\tenables all non-GPL external libraries"
    echo -e "  --enable-ios-audiotoolbox\tbuild with built-in Apple AudioToolbox support[no]"
    echo -e "  --enable-ios-avfoundation\tbuild with built-in Apple AVFoundation support[no]"
    echo -e "  --enable-ios-coreimage\tbuild with built-in Apple CoreImage support[no]"
    echo -e "  --enable-ios-bzip2\t\tbuild with built-in bzip2 support[no]"
    echo -e "  --enable-ios-videotoolbox\tbuild with built-in Apple VideoToolbox support[no]"
    echo -e "  --enable-ios-zlib\t\tbuild with built-in zlib [no]"
    echo -e "  --enable-chromaprint\t\tbuild with chromaprint [no]"
    echo -e "  --enable-fontconfig\t\tbuild with fontconfig [no]"
    echo -e "  --enable-freetype\t\tbuild with freetype [no]"
    echo -e "  --enable-fribidi\t\tbuild with fribidi [no]"
    echo -e "  --enable-gmp\t\t\tbuild with gmp [no]"
    echo -e "  --enable-gnutls\t\tbuild with gnutls [no]"
    echo -e "  --enable-kvazaar\t\tbuild with kvazaar [no]"
    echo -e "  --enable-lame\t\t\tbuild with lame [no]"
    echo -e "  --enable-libaom\t\tbuild with libaom [no]"
    echo -e "  --enable-libass\t\tbuild with libass [no]"
    echo -e "  --enable-libiconv\t\tbuild with libiconv [no]"
    echo -e "  --enable-libilbc\t\tbuild with libilbc [no]"
    echo -e "  --enable-libtheora\t\tbuild with libtheora [no]"
    echo -e "  --enable-libvorbis\t\tbuild with libvorbis [no]"
    echo -e "  --enable-libvpx\t\tbuild with libvpx [no]"
    echo -e "  --enable-libwebp\t\tbuild with libwebp [no]"
    echo -e "  --enable-libxml2\t\tbuild with libxml2 [no]"
    echo -e "  --enable-opencore-amr\t\tbuild with opencore-amr [no]"
    echo -e "  --enable-opus\t\t\tbuild with opus [no]"
    echo -e "  --enable-sdl\t\t\tbuild with sdl [no]"
    echo -e "  --enable-shine\t\tbuild with shine [no]"
    echo -e "  --enable-snappy\t\tbuild with snappy [no]"
    echo -e "  --enable-soxr\t\t\tbuild with soxr [no]"
    echo -e "  --enable-speex\t\tbuild with speex [no]"
    echo -e "  --enable-tesseract\t\tbuild with tesseract [no]"
    echo -e "  --enable-twolame\t\tbuild with twolame [no]"
    echo -e "  --enable-wavpack\t\tbuild with wavpack [no]\n"

    echo -e "GPL libraries:"

    echo -e "  --enable-libvidstab\t\tbuild with libvidstab [no]"
    echo -e "  --enable-x264\t\t\tbuild with x264 [no]"
    echo -e "  --enable-x265\t\t\tbuild with x265 [no]"
    echo -e "  --enable-xvidcore\t\tbuild with xvidcore [no]\n"

    echo -e "Advanced options:"

    echo -e "  --reconf-LIBRARY\t\trun autoreconf before building LIBRARY [no]"
    echo -e "  --rebuild-LIBRARY\t\tbuild LIBRARY even it is detected as already built [no]\n"
}

display_version() {
    COMMAND=`echo $0 | sed -e 's/\.\///g'`

    echo -e "\
$COMMAND v$(get_mobile_ffmpeg_version)\n\
Copyright (c) 2018 Taner Sener\n\
License LGPLv3.0: GNU LGPL version 3 or later\n\
<https://www.gnu.org/licenses/lgpl-3.0.en.html>\n\
This is free software: you can redistribute it and/or modify it under the terms of the \
GNU Lesser General Public License as published by the Free Software Foundation, \
either version 3 of the License, or (at your option) any later version."
}

skip_library() {
    SKIP_VARIABLE=$(echo "SKIP_$1" | sed "s/\-/\_/g")

    export ${SKIP_VARIABLE}=1
}

enable_debug() {
    export MOBILE_FFMPEG_DEBUG="-DDEBUG -g"

    BUILD_TYPE_ID+="debug "
}

optimize_for_speed() {
    export MOBILE_FFMPEG_OPTIMIZED_FOR_SPEED="1"
}

enable_lts_build() {
    export MOBILE_FFMPEG_LTS_BUILD="1"

    # XCODE 7.3 HAS SDK 9.3
    export IOS_MIN_VERSION=9.3

    disable_arch "arm64e"
}

reconf_library() {
    RECONF_VARIABLE=$(echo "RECONF_$1" | sed "s/\-/\_/g")

    export ${RECONF_VARIABLE}=1
}

rebuild_library() {
    REBUILD_VARIABLE=$(echo "REBUILD_$1" | sed "s/\-/\_/g")

    export ${REBUILD_VARIABLE}=1
}

enable_library() {
    set_library $1 1
}

set_library() {
    case $1 in
        ios-zlib)
            ENABLED_LIBRARIES[LIBRARY_ZLIB]=$2
        ;;
        ios-audiotoolbox)
            ENABLED_LIBRARIES[LIBRARY_AUDIOTOOLBOX]=$2
        ;;
        ios-avfoundation)
            ENABLED_LIBRARIES[LIBRARY_AVFOUNDATION]=$2
        ;;
        ios-coreimage)
            ENABLED_LIBRARIES[LIBRARY_COREIMAGE]=$2
        ;;
        ios-bzip2)
            ENABLED_LIBRARIES[LIBRARY_BZIP2]=$2
        ;;
        ios-videotoolbox)
            ENABLED_LIBRARIES[LIBRARY_VIDEOTOOLBOX]=$2
        ;;
        chromaprint)
            ENABLED_LIBRARIES[LIBRARY_CHROMAPRINT]=$2
        ;;
        fontconfig)
            ENABLED_LIBRARIES[LIBRARY_FONTCONFIG]=$2
            ENABLED_LIBRARIES[LIBRARY_LIBUUID]=$2
            ENABLED_LIBRARIES[LIBRARY_EXPAT]=$2
            ENABLED_LIBRARIES[LIBRARY_LIBICONV]=$2
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
            ENABLED_LIBRARIES[LIBRARY_ZLIB]=$2
            set_library "nettle" $2
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
        libaom)
            ENABLED_LIBRARIES[LIBRARY_LIBAOM]=$2
        ;;
        libass)
            ENABLED_LIBRARIES[LIBRARY_LIBASS]=$2
            ENABLED_LIBRARIES[LIBRARY_LIBUUID]=$2
            ENABLED_LIBRARIES[LIBRARY_EXPAT]=$2
            set_library "freetype" $2
            set_library "fribidi" $2
            set_library "fontconfig" $2
            set_library "libiconv" $2
        ;;
        libiconv)
            ENABLED_LIBRARIES[LIBRARY_LIBICONV]=$2
        ;;
        libilbc)
            ENABLED_LIBRARIES[LIBRARY_LIBILBC]=$2
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
        libvidstab)
            ENABLED_LIBRARIES[LIBRARY_LIBVIDSTAB]=$2
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
            set_library "tiff" $2
            set_library "libpng" $2
        ;;
        libxml2)
            ENABLED_LIBRARIES[LIBRARY_LIBXML2]=$2
            set_library "libiconv" $2
        ;;
        opencore-amr)
            ENABLED_LIBRARIES[LIBRARY_OPENCOREAMR]=$2
        ;;
        opus)
            ENABLED_LIBRARIES[LIBRARY_OPUS]=$2
        ;;
        sdl)
            ENABLED_LIBRARIES[LIBRARY_SDL]=$2
        ;;
        shine)
            ENABLED_LIBRARIES[LIBRARY_SHINE]=$2
        ;;
        snappy)
            ENABLED_LIBRARIES[LIBRARY_SNAPPY]=$2
            ENABLED_LIBRARIES[LIBRARY_ZLIB]=$2
        ;;
        soxr)
            ENABLED_LIBRARIES[LIBRARY_SOXR]=$2
        ;;
        speex)
            ENABLED_LIBRARIES[LIBRARY_SPEEX]=$2
        ;;
        tesseract)
            ENABLED_LIBRARIES[LIBRARY_TESSERACT]=$2
            ENABLED_LIBRARIES[LIBRARY_LEPTONICA]=$2
            ENABLED_LIBRARIES[LIBRARY_LIBWEBP]=$2
            ENABLED_LIBRARIES[LIBRARY_GIFLIB]=$2
            ENABLED_LIBRARIES[LIBRARY_JPEG]=$2
            ENABLED_LIBRARIES[LIBRARY_ZLIB]=$2
            set_library "tiff" $2
            set_library "libpng" $2
        ;;
        twolame)
            ENABLED_LIBRARIES[LIBRARY_TWOLAME]=$2
            ENABLED_LIBRARIES[LIBRARY_SNDFILE]=$2
        ;;
        wavpack)
            ENABLED_LIBRARIES[LIBRARY_WAVPACK]=$2
        ;;
        x264)
            ENABLED_LIBRARIES[LIBRARY_X264]=$2
        ;;
        x265)
            ENABLED_LIBRARIES[LIBRARY_X265]=$2
        ;;
        xvidcore)
            ENABLED_LIBRARIES[LIBRARY_XVIDCORE]=$2
        ;;
        expat | giflib | jpeg | leptonica | libogg | libpng | libsndfile | libuuid)
            # THESE LIBRARIES ARE NOT ENABLED DIRECTLY
        ;;
        nettle)
            ENABLED_LIBRARIES[LIBRARY_NETTLE]=$2
            set_library "gmp" $2
        ;;
        tiff)
            ENABLED_LIBRARIES[LIBRARY_TIFF]=$2
            ENABLED_LIBRARIES[LIBRARY_JPEG]=$2
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
        arm64e)
            ENABLED_ARCHITECTURES[ARCH_ARM64E]=$2
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
    for print_arch in {0..5}
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
    for library in {41..46}
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
    for library in {0..30}
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
	<key>CFBundleSignature</key>
	<string>????</string>
	<key>MinimumOSVersion</key>
	<string>${IOS_MIN_VERSION}</string>
	<key>CFBundleSupportedPlatforms</key>
	<array>
		<string>iPhoneOS</string>
	</array>
	<key>NSPrincipalClass</key>
	<string></string>
</dict>
</plist>
EOF
}

create_static_fat_library() {
    LIPO_COMMAND="${LIPO} -create"

    for TARGET_ARCH in "${TARGET_ARCH_LIST[@]}"
    do
        LIPO_COMMAND+=" $(find ${BASEDIR}/prebuilt/ios-${TARGET_ARCH}-ios-darwin -name $1)"
    done

    LIPO_COMMAND+=" -output ${MOBILE_FFMPEG_UNIVERSAL}/lib/$1"

    RC=$(${LIPO_COMMAND} 1>>${BASEDIR}/build.log 2>&1)

    echo ${RC}
}

# ENABLE COMMON FUNCTIONS
. ${BASEDIR}/build/ios-common.sh

GPL_ENABLED="no"
DISPLAY_HELP=""
BUILD_LTS=""
BUILD_TYPE_ID=""
BUILD_FORCE=""

while [ ! $# -eq 0 ]
do
    case $1 in
	    -h | --help)
            DISPLAY_HELP="1"
	    ;;
	    -V | --version)
            display_version
            exit 0
	    ;;
        --skip-*)
            SKIP_LIBRARY=`echo $1 | sed -e 's/^--[A-Za-z]*-//g'`

            skip_library ${SKIP_LIBRARY}
	    ;;
        -d | --debug)
            enable_debug
	    ;;
	    -s | --speed)
	        optimize_for_speed
	    ;;
	    -l | --lts)
	        BUILD_LTS="1"
	    ;;
	    -f | --force)
	        BUILD_FORCE="1"
	    ;;
        --reconf-*)
            CONF_LIBRARY=`echo $1 | sed -e 's/^--[A-Za-z]*-//g'`

            reconf_library ${CONF_LIBRARY}
	    ;;
        --rebuild-*)
            BUILD_LIBRARY=`echo $1 | sed -e 's/^--[A-Za-z]*-//g'`

            rebuild_library ${BUILD_LIBRARY}
	    ;;
	    --full)
            for library in {0..46}
            do
                if [[ $library -ne 18 ]] && [[ $library -ne 19 ]] && [[ $library -ne 20 ]] && [[ $library -ne 21 ]]; then
                    enable_library $(get_library_name $library)
                fi
            done
	    ;;
        --enable-gpl)
            GPL_ENABLED="yes"
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

# DETECT BUILD TYPE
if [[ ! -z ${BUILD_LTS} ]]; then
    enable_lts_build
    BUILD_TYPE_ID+="LTS "
fi

if [[ ! -z ${DISPLAY_HELP} ]]; then
    display_help
    exit 0
fi

# SELECT XCODE VERSION USED FOR BUILDING
XCODE_FOR_MOBILE_FFMPEG=~/.xcode.for.mobile.ffmpeg.sh
if [[ -f ${XCODE_FOR_MOBILE_FFMPEG} ]]; then
    source ${XCODE_FOR_MOBILE_FFMPEG} 1>>${BASEDIR}/build.log 2>&1
fi
DETECTED_IOS_SDK_VERSION="$(xcrun --sdk iphoneos --show-sdk-version)"

echo -e "INFO: Using SDK ${DETECTED_IOS_SDK_VERSION} by Xcode provided at $(xcode-select -p)\n" 1>>${BASEDIR}/build.log 2>&1
if [[ ! -z ${MOBILE_FFMPEG_LTS_BUILD} ]] && [[ "${DETECTED_IOS_SDK_VERSION}" != "${IOS_MIN_VERSION}" ]]; then
    echo -e "\n(*) LTS packages should be built using SDK ${IOS_MIN_VERSION} but current configuration uses SDK ${DETECTED_IOS_SDK_VERSION}\n"

    if [[ -z ${BUILD_FORCE} ]]; then
        exit 1
    fi
fi

echo -e "Building mobile-ffmpeg ${BUILD_TYPE_ID}static library for IOS\n"

echo -e -n "INFO: Building mobile-ffmpeg ${BUILD_TYPE_ID}for IOS: " 1>>${BASEDIR}/build.log 2>&1
echo -e `date` 1>>${BASEDIR}/build.log 2>&1

print_enabled_architectures
print_enabled_libraries

# CHECKING GPL LIBRARIES
for gpl_library in {18..21}
do
    if [[ ${ENABLED_LIBRARIES[$gpl_library]} -eq 1 ]]; then
        library_name=$(get_library_name ${gpl_library})

        if  [ ${GPL_ENABLED} != "yes" ]; then
            echo -e "\n(*) Invalid configuration detected. GPL library ${library_name} enabled without --enable-gpl flag.\n"
            echo -e "\n(*) Invalid configuration detected. GPL library ${library_name} enabled without --enable-gpl flag.\n" 1>>${BASEDIR}/build.log 2>&1
            exit 1
        else
            DOWNLOAD_RESULT=$(download_gpl_library_source ${library_name})
            if [[ ${DOWNLOAD_RESULT} -ne 0 ]]; then
                echo -e "\n(*) Failed to download GPL library ${library_name} source. Please check build.log file for details. If the problem persists refer to offline building instructions.\n"
                echo -e "\n(*) Failed to download GPL library ${library_name} source.\n" 1>>${BASEDIR}/build.log 2>&1
                exit 1
            fi
        fi
    fi
done

# CHECKING IF XCODE IS INSTALLED
if ! [ -x "$(command -v xcrun)" ]; then
    echo -e "\n(*) xcrun command not found. Please check your Xcode installation.\n"
    exit 1
fi

TARGET_ARCH_LIST=()

for run_arch in {0..5}
do
    if [[ ${ENABLED_ARCHITECTURES[$run_arch]} -eq 1 ]]; then
        export ARCH=$(get_arch_name $run_arch)
        export TARGET_SDK=$(get_target_sdk)
        export SDK_PATH=$(get_sdk_path)
        export LIPO="$(xcrun --sdk $(get_sdk_name) -f lipo)"

        . ${BASEDIR}/build/main-ios.sh "${ENABLED_LIBRARIES[@]}"
        case ${ARCH} in
            x86-64)
                TARGET_ARCH="x86_64"
            ;;
            *)
                TARGET_ARCH="${ARCH}"
            ;;
        esac
        TARGET_ARCH_LIST+=(${TARGET_ARCH})

        # CLEAR FLAGS
        for library in {1..47}
        do
            library_name=$(get_library_name $((library - 1)))
            unset $(echo "OK_${library_name}" | sed "s/\-/\_/g")
            unset $(echo "DEPENDENCY_REBUILT_${library_name}" | sed "s/\-/\_/g")
        done
    fi
done

FFMPEG_LIBS="libavcodec libavdevice libavfilter libavformat libavutil libswresample libswscale"

BUILD_LIBRARY_EXTENSION="a";

if [[ ! -z ${TARGET_ARCH_LIST} ]]; then

    echo -e -n "\n\nCreating fat-binary under prebuilt/ios-universal: "

    MOBILE_FFMPEG_UNIVERSAL=${BASEDIR}/prebuilt/ios-universal/mobile-ffmpeg-universal

    # BUILDING UNIVERSAL LIBRARIES
    rm -rf ${BASEDIR}/prebuilt/ios-universal 1>>${BASEDIR}/build.log 2>&1

    mkdir -p ${MOBILE_FFMPEG_UNIVERSAL}/include 1>>${BASEDIR}/build.log 2>&1
    mkdir -p ${MOBILE_FFMPEG_UNIVERSAL}/lib 1>>${BASEDIR}/build.log 2>&1

    # BUILDING FFMPEG FAT BINARY
    for FFMPEG_LIB in ${FFMPEG_LIBS}
    do
        LIPO_COMMAND="${LIPO} -create"

        for TARGET_ARCH in "${TARGET_ARCH_LIST[@]}"
        do
            LIPO_COMMAND+=" ${BASEDIR}/prebuilt/ios-${TARGET_ARCH}-ios-darwin/ffmpeg/lib/${FFMPEG_LIB}.${BUILD_LIBRARY_EXTENSION}"
        done

        LIPO_COMMAND+=" -output ${MOBILE_FFMPEG_UNIVERSAL}/lib/${FFMPEG_LIB}.${BUILD_LIBRARY_EXTENSION}"

        ${LIPO_COMMAND} 1>>${BASEDIR}/build.log 2>&1

        if [ $? -ne 0 ]; then
            echo -e "failed\n"
            exit 1
        fi

        echo -e "Created fat-binary ${FFMPEG_LIB} successfully.\n" 1>>${BASEDIR}/build.log 2>&1
    done

    # BUILDING MOBILE FFMPEG FAT BINARY
    LIPO_COMMAND="${LIPO} -create"

    for TARGET_ARCH in "${TARGET_ARCH_LIST[@]}"
    do
        LIPO_COMMAND+=" ${BASEDIR}/prebuilt/ios-${TARGET_ARCH}-ios-darwin/mobile-ffmpeg/lib/libmobileffmpeg.${BUILD_LIBRARY_EXTENSION}"
    done

    LIPO_COMMAND+=" -output ${MOBILE_FFMPEG_UNIVERSAL}/lib/libmobileffmpeg.${BUILD_LIBRARY_EXTENSION}"

    ${LIPO_COMMAND} 1>>${BASEDIR}/build.log 2>&1

    if [ $? -ne 0 ]; then
        echo -e "failed\n"
        exit 1
    fi

    for library in {0..40}
    do
        if [[ ${ENABLED_LIBRARIES[$library]} -eq 1 ]]; then

            library_name=$(get_library_name $((library)))

            echo -e "Creating fat library for ${library_name}\n" 1>>${BASEDIR}/build.log 2>&1

            if [[ ${LIBRARY_LIBICONV} == $library ]]; then

                LIBRARY_CREATED=$(create_static_fat_library "libiconv.a")
                if [[ ${LIBRARY_CREATED} -ne 0 ]]; then
                    echo -e "failed\n"
                    exit 1
                fi

                LIBRARY_CREATED=$(create_static_fat_library "libcharset.a")
                if [[ ${LIBRARY_CREATED} -ne 0 ]]; then
                    echo -e "failed\n"
                    exit 1
                fi

            elif [[ ${LIBRARY_LIBTHEORA} == $library ]]; then

                LIBRARY_CREATED=$(create_static_fat_library "libtheora.a")
                if [[ ${LIBRARY_CREATED} -ne 0 ]]; then
                    echo -e "failed\n"
                    exit 1
                fi

                LIBRARY_CREATED=$(create_static_fat_library "libtheoraenc.a")
                if [[ ${LIBRARY_CREATED} -ne 0 ]]; then
                    echo -e "failed\n"
                    exit 1
                fi

                LIBRARY_CREATED=$(create_static_fat_library "libtheoradec.a")
                if [[ ${LIBRARY_CREATED} -ne 0 ]]; then
                    echo -e "failed\n"
                    exit 1
                fi

            elif [[ ${LIBRARY_LIBVORBIS} == $library ]]; then

                LIBRARY_CREATED=$(create_static_fat_library "libvorbisfile.a")
                if [[ ${LIBRARY_CREATED} -ne 0 ]]; then
                    echo -e "failed\n"
                    exit 1
                fi

                LIBRARY_CREATED=$(create_static_fat_library "libvorbisenc.a")
                if [[ ${LIBRARY_CREATED} -ne 0 ]]; then
                    echo -e "failed\n"
                    exit 1
                fi

                LIBRARY_CREATED=$(create_static_fat_library "libvorbis.a")
                if [[ ${LIBRARY_CREATED} -ne 0 ]]; then
                    echo -e "failed\n"
                    exit 1
                fi

            elif [[ ${LIBRARY_LIBWEBP} == $library ]]; then

                LIBRARY_CREATED=$(create_static_fat_library "libwebpdecoder.a")
                if [[ ${LIBRARY_CREATED} -ne 0 ]]; then
                    echo -e "failed\n"
                    exit 1
                fi

                LIBRARY_CREATED=$(create_static_fat_library "libwebpdemux.a")
                if [[ ${LIBRARY_CREATED} -ne 0 ]]; then
                    echo -e "failed\n"
                    exit 1
                fi

                LIBRARY_CREATED=$(create_static_fat_library "libwebp.a")
                if [[ ${LIBRARY_CREATED} -ne 0 ]]; then
                    echo -e "failed\n"
                    exit 1
                fi

            elif [[ ${LIBRARY_OPENCOREAMR} == $library ]]; then

                LIBRARY_CREATED=$(create_static_fat_library "libopencore-amrwb.a")
                if [[ ${LIBRARY_CREATED} -ne 0 ]]; then
                    echo -e "failed\n"
                    exit 1
                fi

                LIBRARY_CREATED=$(create_static_fat_library "libopencore-amrnb.a")
                if [[ ${LIBRARY_CREATED} -ne 0 ]]; then
                    echo -e "failed\n"
                    exit 1
                fi

            elif [[ ${LIBRARY_NETTLE} == $library ]]; then

                LIBRARY_CREATED=$(create_static_fat_library "libnettle.a")
                if [[ ${LIBRARY_CREATED} -ne 0 ]]; then
                    echo -e "failed\n"
                    exit 1
                fi

                LIBRARY_CREATED=$(create_static_fat_library "libhogweed.a")
                if [[ ${LIBRARY_CREATED} -ne 0 ]]; then
                    echo -e "failed\n"
                    exit 1
                fi

            else
                static_archive_name=$(get_static_archive_name $((library)))
                LIBRARY_CREATED=$(create_static_fat_library $static_archive_name)
                if [[ ${LIBRARY_CREATED} -ne 0 ]]; then
                    echo -e "failed\n"
                    exit 1
                fi
            fi

        fi
    done

    # COPYING HEADERS
    cp -r ${BASEDIR}/prebuilt/ios-${TARGET_ARCH_LIST[0]}-ios-darwin/ffmpeg/include/* ${MOBILE_FFMPEG_UNIVERSAL}/include 1>>${BASEDIR}/build.log 2>&1
    cp -r ${BASEDIR}/prebuilt/ios-${TARGET_ARCH_LIST[0]}-ios-darwin/mobile-ffmpeg/include/* ${MOBILE_FFMPEG_UNIVERSAL}/include 1>>${BASEDIR}/build.log 2>&1

    # COPYING THE LICENSE
    if  [ ${GPL_ENABLED} == "yes" ]; then
        cp ${BASEDIR}/LICENSE.GPLv3 ${MOBILE_FFMPEG_UNIVERSAL}/LICENSE 1>>${BASEDIR}/build.log 2>&1
    else
        cp ${BASEDIR}/LICENSE.LGPLv3 ${MOBILE_FFMPEG_UNIVERSAL}/LICENSE 1>>${BASEDIR}/build.log 2>&1
    fi

    echo -e "Created fat-binary mobileffmpeg successfully.\n" 1>>${BASEDIR}/build.log 2>&1

    echo -e "ok\n"

    # BUILDING MOBILE FFMPEG FRAMEWORK
    echo -e -n "\nCreating mobileffmpeg.framework under prebuilt/ios-framework: "

    MOBILE_FFMPEG_VERSION=$(get_mobile_ffmpeg_version)

    FRAMEWORK_PATH=${BASEDIR}/prebuilt/ios-framework/mobileffmpeg.framework

    rm -rf ${FRAMEWORK_PATH} 1>>${BASEDIR}/build.log 2>&1
    mkdir -p ${FRAMEWORK_PATH}/Headers 1>>${BASEDIR}/build.log 2>&1

    cp -r ${MOBILE_FFMPEG_UNIVERSAL}/include/* ${FRAMEWORK_PATH}/Headers 1>>${BASEDIR}/build.log 2>&1
    cp ${MOBILE_FFMPEG_UNIVERSAL}/include/config.h ${FRAMEWORK_PATH}/Headers 1>>${BASEDIR}/build.log 2>&1
    cp ${MOBILE_FFMPEG_UNIVERSAL}/lib/libmobileffmpeg.${BUILD_LIBRARY_EXTENSION} ${FRAMEWORK_PATH}/mobileffmpeg 1>>${BASEDIR}/build.log 2>&1

    # COPYING THE LICENSE
    if  [ ${GPL_ENABLED} == "yes" ]; then

        # GPLv3.0
        cp ${BASEDIR}/LICENSE.GPLv3 ${FRAMEWORK_PATH}/LICENSE 1>>${BASEDIR}/build.log 2>&1
    else

        # LGPLv3.0
        cp ${BASEDIR}/LICENSE.LGPLv3 ${FRAMEWORK_PATH}/LICENSE 1>>${BASEDIR}/build.log 2>&1
    fi

    build_info_plist "${FRAMEWORK_PATH}/Info.plist" "mobileffmpeg" "com.arthenica.mobileffmpeg.MobileFFmpeg" "${MOBILE_FFMPEG_VERSION}" "${MOBILE_FFMPEG_VERSION}"

    # BUILDING SUB FRAMEWORKS

    for FFMPEG_LIB in ${FFMPEG_LIBS}
    do
        FFMPEG_LIB_UPPERCASE=$(echo ${FFMPEG_LIB} | tr '[a-z]' '[A-Z]')

        FFMPEG_LIB_MAJOR=$(grep "#define ${FFMPEG_LIB_UPPERCASE}_VERSION_MAJOR" ${MOBILE_FFMPEG_UNIVERSAL}/include/${FFMPEG_LIB}/version.h | sed -e "s/#define ${FFMPEG_LIB_UPPERCASE}_VERSION_MAJOR//g;s/\ //g")
        FFMPEG_LIB_MINOR=$(grep "#define ${FFMPEG_LIB_UPPERCASE}_VERSION_MINOR" ${MOBILE_FFMPEG_UNIVERSAL}/include/${FFMPEG_LIB}/version.h | sed -e "s/#define ${FFMPEG_LIB_UPPERCASE}_VERSION_MINOR//g;s/\ //g")
        FFMPEG_LIB_MICRO=$(grep "#define ${FFMPEG_LIB_UPPERCASE}_VERSION_MICRO" ${MOBILE_FFMPEG_UNIVERSAL}/include/${FFMPEG_LIB}/version.h | sed "s/#define ${FFMPEG_LIB_UPPERCASE}_VERSION_MICRO//g;s/\ //g")

        FFMPEG_LIB_VERSION="${FFMPEG_LIB_MAJOR}.${FFMPEG_LIB_MINOR}.${FFMPEG_LIB_MICRO}"

        FFMPEG_LIB_FRAMEWORK_PATH=${BASEDIR}/prebuilt/ios-framework/${FFMPEG_LIB}.framework

        rm -rf ${FFMPEG_LIB_FRAMEWORK_PATH} 1>>${BASEDIR}/build.log 2>&1
        mkdir -p ${FFMPEG_LIB_FRAMEWORK_PATH}/Headers 1>>${BASEDIR}/build.log 2>&1

        cp -r ${MOBILE_FFMPEG_UNIVERSAL}/include/${FFMPEG_LIB}/* ${FFMPEG_LIB_FRAMEWORK_PATH}/Headers 1>>${BASEDIR}/build.log 2>&1
        cp ${MOBILE_FFMPEG_UNIVERSAL}/lib/${FFMPEG_LIB}.${BUILD_LIBRARY_EXTENSION} ${FFMPEG_LIB_FRAMEWORK_PATH}/${FFMPEG_LIB} 1>>${BASEDIR}/build.log 2>&1

        # COPYING THE LICENSE
        if  [ ${GPL_ENABLED} == "yes" ]; then

            # GPLv3.0
            cp ${BASEDIR}/LICENSE.GPLv3 ${FFMPEG_LIB_FRAMEWORK_PATH}/LICENSE 1>>${BASEDIR}/build.log 2>&1
        else

            # LGPLv3.0
            cp ${BASEDIR}/LICENSE.LGPLv3 ${FFMPEG_LIB_FRAMEWORK_PATH}/LICENSE 1>>${BASEDIR}/build.log 2>&1
        fi

        build_info_plist "${FFMPEG_LIB_FRAMEWORK_PATH}/Info.plist" "${FFMPEG_LIB}" "com.arthenica.mobileffmpeg.FFmpeg${FFMPEG_LIB}" "${FFMPEG_LIB_VERSION}" "${FFMPEG_LIB_VERSION}"

    done

    echo -e "Created mobile-ffmpeg.framework successfully.\n" 1>>${BASEDIR}/build.log 2>&1

    echo -e "ok\n"
fi
