#!/bin/bash

# ARCH INDEXES
ARCH_ARM64=0
ARCH_X86_64=1

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
LIBRARY_OPENH264=31
LIBRARY_GIFLIB=32
LIBRARY_JPEG=33
LIBRARY_LIBOGG=34
LIBRARY_LIBPNG=35
LIBRARY_LIBUUID=36
LIBRARY_NETTLE=37
LIBRARY_TIFF=38
LIBRARY_EXPAT=39
LIBRARY_SNDFILE=40
LIBRARY_LEPTONICA=41
LIBRARY_ZLIB=42
LIBRARY_AUDIOTOOLBOX=43
LIBRARY_COREIMAGE=44
LIBRARY_BZIP2=45
LIBRARY_VIDEOTOOLBOX=46

# ENABLE ARCH
ENABLED_ARCHITECTURES=(1 1)

# ENABLE LIBRARIES
ENABLED_LIBRARIES=(0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)

export BASEDIR=$(pwd)
export MOBILE_FFMPEG_TMPDIR="${BASEDIR}/.tmp"

RECONF_LIBRARIES=()
REBUILD_LIBRARIES=()

# CHECKING IF XCODE IS INSTALLED
if ! [ -x "$(command -v xcrun)" ]; then
    echo -e "\n(*) xcrun command not found. Please check your Xcode installation.\n"
    exit 1
fi

# USE 10.2 AS TVOS_MIN_VERSION
export TVOS_MIN_VERSION=10.2

export APPLE_TVOS_BUILD=1

get_mobile_ffmpeg_version() {
    local MOBILE_FFMPEG_VERSION=$(grep 'const MOBILE_FFMPEG_VERSION' ${BASEDIR}/ios/src/MobileFFmpeg.m | grep -Eo '\".*\"' | sed -e 's/\"//g')

    if [[ -z ${MOBILE_FFMPEG_LTS_BUILD} ]]; then
        echo "${MOBILE_FFMPEG_VERSION}"
    else
        echo "${MOBILE_FFMPEG_VERSION}.LTS"
    fi
}

display_help() {
    COMMAND=`echo $0 | sed -e 's/\.\///g'`

    echo -e "\n'"$COMMAND"' builds FFmpeg and MobileFFmpeg for tvOS platform. By default two architectures (arm64 and x86_64) are built without any external libraries enabled. Options can be used to disable architectures and/or enable external libraries. Please note that GPL libraries (external libraries with GPL license) need --enable-gpl flag to be set explicitly. When compilation ends universal fat binaries and tvOS frameworks are created with enabled architectures inside.\n"

    echo -e "Usage: ./"$COMMAND" [OPTION]...\n"

    echo -e "Specify environment variables as VARIABLE=VALUE to override default build options.\n"

    echo -e "Options:"

    echo -e "  -h, --help\t\t\tdisplay this help and exit"
    echo -e "  -V, --version\t\t\tdisplay version information and exit"
    echo -e "  -d, --debug\t\t\tbuild with debug information"
    echo -e "  -s, --speed\t\t\toptimize for speed instead of size"
    echo -e "  -l, --lts\t\t\tbuild lts packages to support sdk 9.2+ devices"
    echo -e "  -f, --force\t\t\tignore warnings\n"

    echo -e "Licensing options:"

    echo -e "  --enable-gpl\t\t\tallow use of GPL libraries, resulting libs will be licensed under GPLv3.0 [no]\n"

    echo -e "Platforms:"

    echo -e "  --disable-arm64\t\tdo not build arm64 platform [yes]"
    echo -e "  --disable-x86-64\t\tdo not build x86-64 platform [yes]\n"

    echo -e "Libraries:"

    echo -e "  --full\t\t\tenables all non-GPL external libraries"
    echo -e "  --enable-tvos-audiotoolbox\tbuild with built-in Apple AudioToolbox support[no]"
    echo -e "  --enable-tvos-coreimage\tbuild with built-in Apple CoreImage support[no]"
    echo -e "  --enable-tvos-bzip2\t\tbuild with built-in bzip2 support[no]"
    if [[ -z ${MOBILE_FFMPEG_LTS_BUILD} ]]; then
        echo -e "  --enable-tvos-videotoolbox\tbuild with built-in Apple VideoToolbox support[no]"
    fi
    echo -e "  --enable-tvos-zlib\t\tbuild with built-in zlib [no]"
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
    echo -e "  --enable-openh264\t\tbuild with openh264 [no]"
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
Copyright (c) 2019 Taner Sener\n\
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

no_output_redirection() {
    export NO_OUTPUT_REDIRECTION=1
}

no_workspace_cleanup_library() {
    NO_WORKSPACE_CLEANUP_VARIABLE=$(echo "NO_WORKSPACE_CLEANUP_$1" | sed "s/\-/\_/g")

    export ${NO_WORKSPACE_CLEANUP_VARIABLE}=1
}

enable_debug() {
    export MOBILE_FFMPEG_DEBUG="-g"

    BUILD_TYPE_ID+="debug "
}

optimize_for_speed() {
    export MOBILE_FFMPEG_OPTIMIZED_FOR_SPEED="1"
}

enable_lts_build() {
    export MOBILE_FFMPEG_LTS_BUILD="1"

    # XCODE 7.3 HAS TVOS SDK 9.2
    export TVOS_MIN_VERSION=9.2

    # TVOS SDK 9.2 DOES NOT INCLUDE VIDEOTOOLBOX
    ENABLED_LIBRARIES[LIBRARY_VIDEOTOOLBOX]=0
}

reconf_library() {
    local RECONF_VARIABLE=$(echo "RECONF_$1" | sed "s/\-/\_/g")
    local library_supported=0

    for library in {1..42}
    do
        library_name=$(get_library_name $((library - 1)))

        if [[ $1 != "ffmpeg" ]] && [[ ${library_name} == $1 ]]; then
            export ${RECONF_VARIABLE}=1
            RECONF_LIBRARIES+=($1)
            library_supported=1
        fi
    done

    if [[ ${library_supported} -eq 0 ]]; then
        echo -e "INFO: --reconf flag detected for library $1 is not supported.\n" 1>>${BASEDIR}/build.log 2>&1
    fi
}

rebuild_library() {
    local REBUILD_VARIABLE=$(echo "REBUILD_$1" | sed "s/\-/\_/g")
    local library_supported=0

    for library in {1..42}
    do
        library_name=$(get_library_name $((library - 1)))

        if [[ $1 != "ffmpeg" ]] && [[ ${library_name} == $1 ]]; then
            export ${REBUILD_VARIABLE}=1
            REBUILD_LIBRARIES+=($1)
            library_supported=1
        fi
    done

    if [[ ${library_supported} -eq 0 ]]; then
        echo -e "INFO: --rebuild flag detected for library $1 is not supported.\n" 1>>${BASEDIR}/build.log 2>&1
    fi
}

enable_library() {
    set_library $1 1
}

set_library() {
    case $1 in
        tvos-zlib)
            ENABLED_LIBRARIES[LIBRARY_ZLIB]=$2
        ;;
        tvos-audiotoolbox)
            ENABLED_LIBRARIES[LIBRARY_AUDIOTOOLBOX]=$2
        ;;
        tvos-coreimage)
            ENABLED_LIBRARIES[LIBRARY_COREIMAGE]=$2
        ;;
        tvos-bzip2)
            ENABLED_LIBRARIES[LIBRARY_BZIP2]=$2
        ;;
        tvos-videotoolbox)
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
        openh264)
            ENABLED_LIBRARIES[LIBRARY_OPENH264]=$2
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
        arm64)
            ENABLED_ARCHITECTURES[ARCH_ARM64]=$2
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
    for print_arch in {0..1}
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
    for library in {42..46}
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
    for library in {0..31}
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

print_reconfigure_requested_libraries() {
    local counter=0;

    for RECONF_LIBRARY in "${RECONF_LIBRARIES[@]}"
    do
        if [[ ${counter} -eq 0 ]]; then
            echo -n "Reconfigure: "
        else
            echo -n ", "
        fi

        echo -n ${RECONF_LIBRARY}

        counter=$((${counter} + 1));
    done

    if [[ ${counter} -gt 0 ]]; then
        echo ""
    fi
}

print_rebuild_requested_libraries() {
    local counter=0;

    for REBUILD_LIBRARY in "${REBUILD_LIBRARIES[@]}"
    do
        if [[ ${counter} -eq 0 ]]; then
            echo -n "Rebuild: "
        else
            echo -n ", "
        fi

        echo -n ${REBUILD_LIBRARY}

        counter=$((${counter} + 1));
    done

    if [[ ${counter} -gt 0 ]]; then
        echo ""
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
	<string>${TVOS_MIN_VERSION}</string>
	<key>CFBundleSupportedPlatforms</key>
	<array>
		<string>AppleTVOS</string>
	</array>
	<key>NSPrincipalClass</key>
	<string></string>
</dict>
</plist>
EOF
}

create_static_fat_library() {
    local FAT_LIBRARY_PATH=${BASEDIR}/prebuilt/tvos-universal/$2-universal

    mkdir -p ${FAT_LIBRARY_PATH}/lib 1>>${BASEDIR}/build.log 2>&1

    LIPO_COMMAND="${LIPO} -create"

    for TARGET_ARCH in "${TARGET_ARCH_LIST[@]}"
    do
        LIPO_COMMAND+=" $(find ${BASEDIR}/prebuilt/tvos-${TARGET_ARCH}-apple-darwin -name $1)"
    done

    LIPO_COMMAND+=" -output ${FAT_LIBRARY_PATH}/lib/$1"

    RC=$(${LIPO_COMMAND} 1>>${BASEDIR}/build.log 2>&1)

    echo ${RC}
}

# 1 - library name
# 2 - static library name
# 3 - library version
create_static_framework() {
    local FRAMEWORK_PATH=${BASEDIR}/prebuilt/tvos-framework/$1.framework

    mkdir -p ${FRAMEWORK_PATH} 1>>${BASEDIR}/build.log 2>&1

    local CAPITAL_CASE_LIBRARY_NAME=$(to_capital_case $1)

    build_info_plist "${FRAMEWORK_PATH}/Info.plist" "${FFMPEG_LIB}" "com.arthenica.mobileffmpeg.${CAPITAL_CASE_LIBRARY_NAME}" "$3" "$3"

    cp ${BASEDIR}/prebuilt/tvos-universal/$1-universal/lib/$2 ${FRAMEWORK_PATH}/$1 1>>${BASEDIR}/build.log 2>&1

    echo "$?"
}

get_external_library_license_path() {
    case $1 in
        1) echo "${BASEDIR}/src/$(get_library_name $1)/docs/LICENSE.TXT" ;;
        3) echo "${BASEDIR}/src/$(get_library_name $1)/COPYING.LESSERv3" ;;
        7) echo "${BASEDIR}/src/$(get_library_name $1)/COPYING.LIB" ;;
        25) echo "${BASEDIR}/src/$(get_library_name $1)/COPYING.LGPL" ;;
        27) echo "${BASEDIR}/src/$(get_library_name $1)/LICENSE.md" ;;
        29) echo "${BASEDIR}/src/$(get_library_name $1)/COPYING.txt" ;;
        33) echo "${BASEDIR}/src/$(get_library_name $1)/LICENSE.md " ;;
        37) echo "${BASEDIR}/src/$(get_library_name $1)/COPYING.LESSERv3" ;;
        38) echo "${BASEDIR}/src/$(get_library_name $1)/COPYRIGHT" ;;
        41) echo "${BASEDIR}/src/$(get_library_name $1)/leptonica-license.txt" ;;
        4 | 10 | 13 | 19 | 21 | 26 | 31 | 35) echo "${BASEDIR}/src/$(get_library_name $1)/LICENSE" ;;
        *) echo "${BASEDIR}/src/$(get_library_name $1)/COPYING" ;;
    esac
}

get_external_library_version() {
    echo "$(grep Version ${BASEDIR}/prebuilt/tvos-${TARGET_ARCH_LIST[0]}-apple-darwin/pkgconfig/$1.pc 2>>${BASEDIR}/build.log | sed 's/Version://g;s/\ //g')"
}

# ENABLE COMMON FUNCTIONS
. ${BASEDIR}/build/tvos-common.sh

echo -e "\nINFO: Build options: $@\n" 1>>${BASEDIR}/build.log 2>&1

GPL_ENABLED="no"
DISPLAY_HELP=""
BUILD_LTS=""
BUILD_TYPE_ID=""
BUILD_FORCE=""
BUILD_VERSION=$(git describe --tags 2>>${BASEDIR}/build.log)

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
	    --no-output-redirection)
            no_output_redirection
	    ;;
      --no-workspace-cleanup-*)
            NO_WORKSPACE_CLEANUP_LIBRARY=`echo $1 | sed -e 's/^--[A-Za-z]*-[A-Za-z]*-[A-Za-z]*-//g'`

            no_workspace_cleanup_library ${NO_WORKSPACE_CLEANUP_LIBRARY}
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
DETECTED_TVOS_SDK_VERSION="$(xcrun --sdk appletvos --show-sdk-version)"

echo -e "INFO: Using SDK ${DETECTED_TVOS_SDK_VERSION} by Xcode provided at $(xcode-select -p)\n" 1>>${BASEDIR}/build.log 2>&1
if [[ ! -z ${MOBILE_FFMPEG_LTS_BUILD} ]] && [[ "${DETECTED_TVOS_SDK_VERSION}" != "${TVOS_MIN_VERSION}" ]]; then
    echo -e "\n(*) LTS packages should be built using SDK ${TVOS_MIN_VERSION} but current configuration uses SDK ${DETECTED_TVOS_SDK_VERSION}\n"

    if [[ -z ${BUILD_FORCE} ]]; then
        exit 1
    fi
fi

echo -e "Building mobile-ffmpeg ${BUILD_TYPE_ID}static library for tvOS\n"

echo -e -n "INFO: Building mobile-ffmpeg ${BUILD_VERSION} ${BUILD_TYPE_ID}for tvOS: " 1>>${BASEDIR}/build.log 2>&1
echo -e `date` 1>>${BASEDIR}/build.log 2>&1

print_enabled_architectures
print_enabled_libraries
print_reconfigure_requested_libraries
print_rebuild_requested_libraries

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

TARGET_ARCH_LIST=()

for run_arch in {0..1}
do
    if [[ ${ENABLED_ARCHITECTURES[$run_arch]} -eq 1 ]]; then
        export ARCH=$(get_arch_name $run_arch)
        export TARGET_SDK=$(get_target_sdk)
        export SDK_PATH=$(get_sdk_path)
        export SDK_NAME=$(get_sdk_name)
        export LIPO="$(xcrun --sdk $(get_sdk_name) -f lipo)"

        . ${BASEDIR}/build/main-tvos.sh "${ENABLED_LIBRARIES[@]}"
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

    echo -e -n "\n\nCreating frameworks under prebuilt: "

    # BUILDING UNIVERSAL LIBRARIES
    rm -rf ${BASEDIR}/prebuilt/tvos-universal 1>>${BASEDIR}/build.log 2>&1
    mkdir -p ${BASEDIR}/prebuilt/tvos-universal 1>>${BASEDIR}/build.log 2>&1
    rm -rf ${BASEDIR}/prebuilt/tvos-framework 1>>${BASEDIR}/build.log 2>&1
    mkdir -p ${BASEDIR}/prebuilt/tvos-framework 1>>${BASEDIR}/build.log 2>&1

    # 1. EXTERNAL LIBRARIES
    for library in {0..41}
    do
        if [[ ${ENABLED_LIBRARIES[$library]} -eq 1 ]]; then

            library_name=$(get_library_name ${library})
            package_config_file_name=$(get_package_config_file_name ${library})
            library_version=$(get_external_library_version ${package_config_file_name})
            capital_case_library_name=$(to_capital_case ${library_name})
            if [[ -z ${library_version} ]]; then
                echo -e "Failed to detect version for ${library_name} from ${package_config_file_name}.pc\n" 1>>${BASEDIR}/build.log 2>&1
                echo -e "failed\n"
                exit 1
            fi

            echo -e "Creating fat library for ${library_name}\n" 1>>${BASEDIR}/build.log 2>&1

            if [[ ${LIBRARY_LIBICONV} == $library ]]; then

                LIBRARY_CREATED=$(create_static_fat_library "libiconv.a" "libiconv")
                if [[ ${LIBRARY_CREATED} -ne 0 ]]; then
                    echo -e "failed\n"
                    exit 1
                fi

                LIBRARY_CREATED=$(create_static_fat_library "libcharset.a" "libcharset")
                if [[ ${LIBRARY_CREATED} -ne 0 ]]; then
                    echo -e "failed\n"
                    exit 1
                fi

                FRAMEWORK_CREATED=$(create_static_framework "libiconv" "libiconv.a" $library_version)
                if [[ ${FRAMEWORK_CREATED} -ne 0 ]]; then
                    echo -e "failed\n"
                    exit 1
                fi

                FRAMEWORK_CREATED=$(create_static_framework "libcharset" "libcharset.a" $library_version)
                if [[ ${FRAMEWORK_CREATED} -ne 0 ]]; then
                    echo -e "failed\n"
                    exit 1
                fi

                $(cp $(get_external_library_license_path ${library}) ${BASEDIR}/prebuilt/tvos-universal/libiconv-universal/LICENSE 1>>${BASEDIR}/build.log 2>&1)
                if [ $? -ne 0 ]; then
                    echo -e "failed\n"
                    exit 1
                fi

                $(cp $(get_external_library_license_path ${library}) ${BASEDIR}/prebuilt/tvos-universal/libcharset-universal/LICENSE 1>>${BASEDIR}/build.log 2>&1)
                if [ $? -ne 0 ]; then
                    echo -e "failed\n"
                    exit 1
                fi

                $(cp $(get_external_library_license_path ${library}) ${BASEDIR}/prebuilt/tvos-framework/libiconv.framework/LICENSE 1>>${BASEDIR}/build.log 2>&1)
                if [ $? -ne 0 ]; then
                    echo -e "failed\n"
                    exit 1
                fi

                $(cp $(get_external_library_license_path ${library}) ${BASEDIR}/prebuilt/tvos-framework/libcharset.framework/LICENSE 1>>${BASEDIR}/build.log 2>&1)
                if [ $? -ne 0 ]; then
                    echo -e "failed\n"
                    exit 1
                fi

            elif [[ ${LIBRARY_LIBTHEORA} == $library ]]; then

                LIBRARY_CREATED=$(create_static_fat_library "libtheora.a" "libtheora")
                if [[ ${LIBRARY_CREATED} -ne 0 ]]; then
                    echo -e "failed\n"
                    exit 1
                fi

                LIBRARY_CREATED=$(create_static_fat_library "libtheoraenc.a" "libtheoraenc")
                if [[ ${LIBRARY_CREATED} -ne 0 ]]; then
                    echo -e "failed\n"
                    exit 1
                fi

                LIBRARY_CREATED=$(create_static_fat_library "libtheoradec.a" "libtheoradec")
                if [[ ${LIBRARY_CREATED} -ne 0 ]]; then
                    echo -e "failed\n"
                    exit 1
                fi

                FRAMEWORK_CREATED=$(create_static_framework "libtheora" "libtheora.a" $library_version)
                if [[ ${FRAMEWORK_CREATED} -ne 0 ]]; then
                    echo -e "failed\n"
                    exit 1
                fi

                FRAMEWORK_CREATED=$(create_static_framework "libtheoraenc" "libtheoraenc.a" $library_version)
                if [[ ${FRAMEWORK_CREATED} -ne 0 ]]; then
                    echo -e "failed\n"
                    exit 1
                fi

                FRAMEWORK_CREATED=$(create_static_framework "libtheoradec" "libtheoradec.a" $library_version)
                if [[ ${FRAMEWORK_CREATED} -ne 0 ]]; then
                    echo -e "failed\n"
                    exit 1
                fi

                $(cp $(get_external_library_license_path ${library}) ${BASEDIR}/prebuilt/tvos-universal/libtheora-universal/LICENSE 1>>${BASEDIR}/build.log 2>&1)
                if [ $? -ne 0 ]; then
                    echo -e "failed\n"
                    exit 1
                fi

                $(cp $(get_external_library_license_path ${library}) ${BASEDIR}/prebuilt/tvos-universal/libtheoraenc-universal/LICENSE 1>>${BASEDIR}/build.log 2>&1)
                if [ $? -ne 0 ]; then
                    echo -e "failed\n"
                    exit 1
                fi

                $(cp $(get_external_library_license_path ${library}) ${BASEDIR}/prebuilt/tvos-universal/libtheoradec-universal/LICENSE 1>>${BASEDIR}/build.log 2>&1)
                if [ $? -ne 0 ]; then
                    echo -e "failed\n"
                    exit 1
                fi

                $(cp $(get_external_library_license_path ${library}) ${BASEDIR}/prebuilt/tvos-framework/libtheora.framework/LICENSE 1>>${BASEDIR}/build.log 2>&1)
                if [ $? -ne 0 ]; then
                    echo -e "failed\n"
                    exit 1
                fi

                $(cp $(get_external_library_license_path ${library}) ${BASEDIR}/prebuilt/tvos-framework/libtheoraenc.framework/LICENSE 1>>${BASEDIR}/build.log 2>&1)
                if [ $? -ne 0 ]; then
                    echo -e "failed\n"
                    exit 1
                fi

                $(cp $(get_external_library_license_path ${library}) ${BASEDIR}/prebuilt/tvos-framework/libtheoradec.framework/LICENSE 1>>${BASEDIR}/build.log 2>&1)
                if [ $? -ne 0 ]; then
                    echo -e "failed\n"
                    exit 1
                fi

            elif [[ ${LIBRARY_LIBVORBIS} == $library ]]; then

                LIBRARY_CREATED=$(create_static_fat_library "libvorbisfile.a" "libvorbisfile")
                if [[ ${LIBRARY_CREATED} -ne 0 ]]; then
                    echo -e "failed\n"
                    exit 1
                fi

                LIBRARY_CREATED=$(create_static_fat_library "libvorbisenc.a" "libvorbisenc")
                if [[ ${LIBRARY_CREATED} -ne 0 ]]; then
                    echo -e "failed\n"
                    exit 1
                fi

                LIBRARY_CREATED=$(create_static_fat_library "libvorbis.a" "libvorbis")
                if [[ ${LIBRARY_CREATED} -ne 0 ]]; then
                    echo -e "failed\n"
                    exit 1
                fi

                FRAMEWORK_CREATED=$(create_static_framework "libvorbisfile" "libvorbisfile.a" $library_version)
                if [[ ${FRAMEWORK_CREATED} -ne 0 ]]; then
                    echo -e "failed\n"
                    exit 1
                fi

                FRAMEWORK_CREATED=$(create_static_framework "libvorbisenc" "libvorbisenc.a" $library_version)
                if [[ ${FRAMEWORK_CREATED} -ne 0 ]]; then
                    echo -e "failed\n"
                    exit 1
                fi

                FRAMEWORK_CREATED=$(create_static_framework "libvorbis" "libvorbis.a" $library_version)
                if [[ ${FRAMEWORK_CREATED} -ne 0 ]]; then
                    echo -e "failed\n"
                    exit 1
                fi

                $(cp $(get_external_library_license_path ${library}) ${BASEDIR}/prebuilt/tvos-universal/libvorbisfile-universal/LICENSE 1>>${BASEDIR}/build.log 2>&1)
                if [ $? -ne 0 ]; then
                    echo -e "failed\n"
                    exit 1
                fi

                $(cp $(get_external_library_license_path ${library}) ${BASEDIR}/prebuilt/tvos-universal/libvorbisenc-universal/LICENSE 1>>${BASEDIR}/build.log 2>&1)
                if [ $? -ne 0 ]; then
                    echo -e "failed\n"
                    exit 1
                fi

                $(cp $(get_external_library_license_path ${library}) ${BASEDIR}/prebuilt/tvos-universal/libvorbis-universal/LICENSE 1>>${BASEDIR}/build.log 2>&1)
                if [ $? -ne 0 ]; then
                    echo -e "failed\n"
                    exit 1
                fi

                $(cp $(get_external_library_license_path ${library}) ${BASEDIR}/prebuilt/tvos-framework/libvorbisfile.framework/LICENSE 1>>${BASEDIR}/build.log 2>&1)
                if [ $? -ne 0 ]; then
                    echo -e "failed\n"
                    exit 1
                fi

                $(cp $(get_external_library_license_path ${library}) ${BASEDIR}/prebuilt/tvos-framework/libvorbisenc.framework/LICENSE 1>>${BASEDIR}/build.log 2>&1)
                if [ $? -ne 0 ]; then
                    echo -e "failed\n"
                    exit 1
                fi

                $(cp $(get_external_library_license_path ${library}) ${BASEDIR}/prebuilt/tvos-framework/libvorbis.framework/LICENSE 1>>${BASEDIR}/build.log 2>&1)
                if [ $? -ne 0 ]; then
                    echo -e "failed\n"
                    exit 1
                fi

            elif [[ ${LIBRARY_LIBWEBP} == $library ]]; then

                LIBRARY_CREATED=$(create_static_fat_library "libwebpdecoder.a" "libwebpdecoder")
                if [[ ${LIBRARY_CREATED} -ne 0 ]]; then
                    echo -e "failed\n"
                    exit 1
                fi

                LIBRARY_CREATED=$(create_static_fat_library "libwebpdemux.a" "libwebpdemux")
                if [[ ${LIBRARY_CREATED} -ne 0 ]]; then
                    echo -e "failed\n"
                    exit 1
                fi

                LIBRARY_CREATED=$(create_static_fat_library "libwebp.a" "libwebp")
                if [[ ${LIBRARY_CREATED} -ne 0 ]]; then
                    echo -e "failed\n"
                    exit 1
                fi

                FRAMEWORK_CREATED=$(create_static_framework "libwebpdecoder" "libwebpdecoder.a" $library_version)
                if [[ ${FRAMEWORK_CREATED} -ne 0 ]]; then
                    echo -e "failed\n"
                    exit 1
                fi

                FRAMEWORK_CREATED=$(create_static_framework "libwebpdemux" "libwebpdemux.a" $library_version)
                if [[ ${FRAMEWORK_CREATED} -ne 0 ]]; then
                    echo -e "failed\n"
                    exit 1
                fi

                FRAMEWORK_CREATED=$(create_static_framework "libwebp" "libwebp.a" $library_version)
                if [[ ${FRAMEWORK_CREATED} -ne 0 ]]; then
                    echo -e "failed\n"
                    exit 1
                fi

                $(cp $(get_external_library_license_path ${library}) ${BASEDIR}/prebuilt/tvos-universal/libwebpdecoder-universal/LICENSE 1>>${BASEDIR}/build.log 2>&1)
                if [ $? -ne 0 ]; then
                    echo -e "failed\n"
                    exit 1
                fi

                $(cp $(get_external_library_license_path ${library}) ${BASEDIR}/prebuilt/tvos-universal/libwebpdemux-universal/LICENSE 1>>${BASEDIR}/build.log 2>&1)
                if [ $? -ne 0 ]; then
                    echo -e "failed\n"
                    exit 1
                fi

                $(cp $(get_external_library_license_path ${library}) ${BASEDIR}/prebuilt/tvos-universal/libwebp-universal/LICENSE 1>>${BASEDIR}/build.log 2>&1)
                if [ $? -ne 0 ]; then
                    echo -e "failed\n"
                    exit 1
                fi

                $(cp $(get_external_library_license_path ${library}) ${BASEDIR}/prebuilt/tvos-framework/libwebpdecoder.framework/LICENSE 1>>${BASEDIR}/build.log 2>&1)
                if [ $? -ne 0 ]; then
                    echo -e "failed\n"
                    exit 1
                fi

                $(cp $(get_external_library_license_path ${library}) ${BASEDIR}/prebuilt/tvos-framework/libwebpdemux.framework/LICENSE 1>>${BASEDIR}/build.log 2>&1)
                if [ $? -ne 0 ]; then
                    echo -e "failed\n"
                    exit 1
                fi

                $(cp $(get_external_library_license_path ${library}) ${BASEDIR}/prebuilt/tvos-framework/libwebp.framework/LICENSE 1>>${BASEDIR}/build.log 2>&1)
                if [ $? -ne 0 ]; then
                    echo -e "failed\n"
                    exit 1
                fi


            elif [[ ${LIBRARY_OPENCOREAMR} == $library ]]; then

                LIBRARY_CREATED=$(create_static_fat_library "libopencore-amrwb.a" "libopencore-amrwb")
                if [[ ${LIBRARY_CREATED} -ne 0 ]]; then
                    echo -e "failed\n"
                    exit 1
                fi

                LIBRARY_CREATED=$(create_static_fat_library "libopencore-amrnb.a" "libopencore-amrnb")
                if [[ ${LIBRARY_CREATED} -ne 0 ]]; then
                    echo -e "failed\n"
                    exit 1
                fi

                FRAMEWORK_CREATED=$(create_static_framework "libopencore-amrwb" "libopencore-amrwb.a" $library_version)
                if [[ ${FRAMEWORK_CREATED} -ne 0 ]]; then
                    echo -e "failed\n"
                    exit 1
                fi

                FRAMEWORK_CREATED=$(create_static_framework "libopencore-amrnb" "libopencore-amrnb.a" $library_version)
                if [[ ${FRAMEWORK_CREATED} -ne 0 ]]; then
                    echo -e "failed\n"
                    exit 1
                fi

                $(cp $(get_external_library_license_path ${library}) ${BASEDIR}/prebuilt/tvos-universal/libopencore-amrwb-universal/LICENSE 1>>${BASEDIR}/build.log 2>&1)
                if [ $? -ne 0 ]; then
                    echo -e "failed\n"
                    exit 1
                fi

                $(cp $(get_external_library_license_path ${library}) ${BASEDIR}/prebuilt/tvos-universal/libopencore-amrnb-universal/LICENSE 1>>${BASEDIR}/build.log 2>&1)
                if [ $? -ne 0 ]; then
                    echo -e "failed\n"
                    exit 1
                fi

                $(cp $(get_external_library_license_path ${library}) ${BASEDIR}/prebuilt/tvos-framework/libopencore-amrwb.framework/LICENSE 1>>${BASEDIR}/build.log 2>&1)
                if [ $? -ne 0 ]; then
                    echo -e "failed\n"
                    exit 1
                fi

                $(cp $(get_external_library_license_path ${library}) ${BASEDIR}/prebuilt/tvos-framework/libopencore-amrnb.framework/LICENSE 1>>${BASEDIR}/build.log 2>&1)
                if [ $? -ne 0 ]; then
                    echo -e "failed\n"
                    exit 1
                fi

            elif [[ ${LIBRARY_NETTLE} == $library ]]; then

                LIBRARY_CREATED=$(create_static_fat_library "libnettle.a" "libnettle")
                if [[ ${LIBRARY_CREATED} -ne 0 ]]; then
                    echo -e "failed\n"
                    exit 1
                fi

                LIBRARY_CREATED=$(create_static_fat_library "libhogweed.a" "libhogweed")
                if [[ ${LIBRARY_CREATED} -ne 0 ]]; then
                    echo -e "failed\n"
                    exit 1
                fi

                FRAMEWORK_CREATED=$(create_static_framework "libnettle" "libnettle.a" $library_version)
                if [[ ${FRAMEWORK_CREATED} -ne 0 ]]; then
                    echo -e "failed\n"
                    exit 1
                fi

                FRAMEWORK_CREATED=$(create_static_framework "libhogweed" "libhogweed.a" $library_version)
                if [[ ${FRAMEWORK_CREATED} -ne 0 ]]; then
                    echo -e "failed\n"
                    exit 1
                fi

                $(cp $(get_external_library_license_path ${library}) ${BASEDIR}/prebuilt/tvos-universal/libnettle-universal/LICENSE 1>>${BASEDIR}/build.log 2>&1)
                if [ $? -ne 0 ]; then
                    echo -e "failed\n"
                    exit 1
                fi

                $(cp $(get_external_library_license_path ${library}) ${BASEDIR}/prebuilt/tvos-universal/libhogweed-universal/LICENSE 1>>${BASEDIR}/build.log 2>&1)
                if [ $? -ne 0 ]; then
                    echo -e "failed\n"
                    exit 1
                fi

                $(cp $(get_external_library_license_path ${library}) ${BASEDIR}/prebuilt/tvos-framework/libnettle.framework/LICENSE 1>>${BASEDIR}/build.log 2>&1)
                if [ $? -ne 0 ]; then
                    echo -e "failed\n"
                    exit 1
                fi

                $(cp $(get_external_library_license_path ${library}) ${BASEDIR}/prebuilt/tvos-framework/libhogweed.framework/LICENSE 1>>${BASEDIR}/build.log 2>&1)
                if [ $? -ne 0 ]; then
                    echo -e "failed\n"
                    exit 1
                fi

            else
                library_name=$(get_library_name $((library)))
                static_archive_name=$(get_static_archive_name $((library)))
                LIBRARY_CREATED=$(create_static_fat_library $static_archive_name $library_name)
                if [[ ${LIBRARY_CREATED} -ne 0 ]]; then
                    echo -e "failed\n"
                    exit 1
                fi

                FRAMEWORK_CREATED=$(create_static_framework $library_name $static_archive_name $library_version)
                if [[ ${FRAMEWORK_CREATED} -ne 0 ]]; then
                    echo -e "failed\n"
                    exit 1
                fi

                $(cp $(get_external_library_license_path ${library}) ${BASEDIR}/prebuilt/tvos-universal/${library_name}-universal/LICENSE 1>>${BASEDIR}/build.log 2>&1)
                if [ $? -ne 0 ]; then
                    echo -e "failed\n"
                    exit 1
                fi

                $(cp $(get_external_library_license_path ${library}) ${BASEDIR}/prebuilt/tvos-framework/${library_name}.framework/LICENSE 1>>${BASEDIR}/build.log 2>&1)
                if [ $? -ne 0 ]; then
                    echo -e "failed\n"
                    exit 1
                fi

            fi

        fi
    done

    # 2. FFMPEG
    FFMPEG_UNIVERSAL=${BASEDIR}/prebuilt/tvos-universal/ffmpeg-universal
    mkdir -p ${FFMPEG_UNIVERSAL}/include 1>>${BASEDIR}/build.log 2>&1
    mkdir -p ${FFMPEG_UNIVERSAL}/lib 1>>${BASEDIR}/build.log 2>&1

    cp -r ${BASEDIR}/prebuilt/tvos-${TARGET_ARCH_LIST[0]}-apple-darwin/ffmpeg/include/* ${FFMPEG_UNIVERSAL}/include 1>>${BASEDIR}/build.log 2>&1
    cp ${BASEDIR}/prebuilt/tvos-${TARGET_ARCH_LIST[0]}-apple-darwin/ffmpeg/include/config.h ${FFMPEG_UNIVERSAL}/include 1>>${BASEDIR}/build.log 2>&1

    for FFMPEG_LIB in ${FFMPEG_LIBS}
    do
        LIPO_COMMAND="${LIPO} -create"

        for TARGET_ARCH in "${TARGET_ARCH_LIST[@]}"
        do
            LIPO_COMMAND+=" ${BASEDIR}/prebuilt/tvos-${TARGET_ARCH}-apple-darwin/ffmpeg/lib/${FFMPEG_LIB}.${BUILD_LIBRARY_EXTENSION}"
        done

        LIPO_COMMAND+=" -output ${FFMPEG_UNIVERSAL}/lib/${FFMPEG_LIB}.${BUILD_LIBRARY_EXTENSION}"

        ${LIPO_COMMAND} 1>>${BASEDIR}/build.log 2>&1

        if [ $? -ne 0 ]; then
            echo -e "failed\n"
            exit 1
        fi

        FFMPEG_LIB_UPPERCASE=$(echo ${FFMPEG_LIB} | tr '[a-z]' '[A-Z]')
        FFMPEG_LIB_CAPITALCASE=$(to_capital_case ${FFMPEG_LIB})

        FFMPEG_LIB_MAJOR=$(grep "#define ${FFMPEG_LIB_UPPERCASE}_VERSION_MAJOR" ${FFMPEG_UNIVERSAL}/include/${FFMPEG_LIB}/version.h | sed -e "s/#define ${FFMPEG_LIB_UPPERCASE}_VERSION_MAJOR//g;s/\ //g")
        FFMPEG_LIB_MINOR=$(grep "#define ${FFMPEG_LIB_UPPERCASE}_VERSION_MINOR" ${FFMPEG_UNIVERSAL}/include/${FFMPEG_LIB}/version.h | sed -e "s/#define ${FFMPEG_LIB_UPPERCASE}_VERSION_MINOR//g;s/\ //g")
        FFMPEG_LIB_MICRO=$(grep "#define ${FFMPEG_LIB_UPPERCASE}_VERSION_MICRO" ${FFMPEG_UNIVERSAL}/include/${FFMPEG_LIB}/version.h | sed "s/#define ${FFMPEG_LIB_UPPERCASE}_VERSION_MICRO//g;s/\ //g")

        FFMPEG_LIB_VERSION="${FFMPEG_LIB_MAJOR}.${FFMPEG_LIB_MINOR}.${FFMPEG_LIB_MICRO}"

        FFMPEG_LIB_FRAMEWORK_PATH=${BASEDIR}/prebuilt/tvos-framework/${FFMPEG_LIB}.framework

        rm -rf ${FFMPEG_LIB_FRAMEWORK_PATH} 1>>${BASEDIR}/build.log 2>&1
        mkdir -p ${FFMPEG_LIB_FRAMEWORK_PATH}/Headers 1>>${BASEDIR}/build.log 2>&1

        cp -r ${FFMPEG_UNIVERSAL}/include/${FFMPEG_LIB}/* ${FFMPEG_LIB_FRAMEWORK_PATH}/Headers 1>>${BASEDIR}/build.log 2>&1
        cp ${FFMPEG_UNIVERSAL}/lib/${FFMPEG_LIB}.${BUILD_LIBRARY_EXTENSION} ${FFMPEG_LIB_FRAMEWORK_PATH}/${FFMPEG_LIB} 1>>${BASEDIR}/build.log 2>&1

        # COPYING THE LICENSE
        if  [ ${GPL_ENABLED} == "yes" ]; then

            # GPLv3.0
            cp ${BASEDIR}/LICENSE.GPLv3 ${FFMPEG_LIB_FRAMEWORK_PATH}/LICENSE 1>>${BASEDIR}/build.log 2>&1
        else

            # LGPLv3.0
            cp ${BASEDIR}/LICENSE.LGPLv3 ${FFMPEG_LIB_FRAMEWORK_PATH}/LICENSE 1>>${BASEDIR}/build.log 2>&1
        fi

        build_info_plist "${FFMPEG_LIB_FRAMEWORK_PATH}/Info.plist" "${FFMPEG_LIB}" "com.arthenica.mobileffmpeg.${FFMPEG_LIB_CAPITALCASE}" "${FFMPEG_LIB_VERSION}" "${FFMPEG_LIB_VERSION}"

        echo -e "Created ${FFMPEG_LIB} framework successfully.\n" 1>>${BASEDIR}/build.log 2>&1
    done

    # COPYING THE LICENSE
    if  [ ${GPL_ENABLED} == "yes" ]; then
        cp ${BASEDIR}/LICENSE.GPLv3 ${FFMPEG_UNIVERSAL}/LICENSE 1>>${BASEDIR}/build.log 2>&1
    else
        cp ${BASEDIR}/LICENSE.LGPLv3 ${FFMPEG_UNIVERSAL}/LICENSE 1>>${BASEDIR}/build.log 2>&1
    fi

    # 3. MOBILE FFMPEG
    MOBILE_FFMPEG_VERSION=$(get_mobile_ffmpeg_version)
    MOBILE_FFMPEG_UNIVERSAL=${BASEDIR}/prebuilt/tvos-universal/mobile-ffmpeg-universal
    MOBILE_FFMPEG_FRAMEWORK_PATH=${BASEDIR}/prebuilt/tvos-framework/mobileffmpeg.framework
    mkdir -p ${MOBILE_FFMPEG_UNIVERSAL}/include 1>>${BASEDIR}/build.log 2>&1
    mkdir -p ${MOBILE_FFMPEG_UNIVERSAL}/lib 1>>${BASEDIR}/build.log 2>&1
    rm -rf ${MOBILE_FFMPEG_FRAMEWORK_PATH} 1>>${BASEDIR}/build.log 2>&1
    mkdir -p ${MOBILE_FFMPEG_FRAMEWORK_PATH}/Headers 1>>${BASEDIR}/build.log 2>&1

    LIPO_COMMAND="${LIPO} -create"
    for TARGET_ARCH in "${TARGET_ARCH_LIST[@]}"
    do
        LIPO_COMMAND+=" ${BASEDIR}/prebuilt/tvos-${TARGET_ARCH}-apple-darwin/mobile-ffmpeg/lib/libmobileffmpeg.${BUILD_LIBRARY_EXTENSION}"
    done
    LIPO_COMMAND+=" -output ${MOBILE_FFMPEG_UNIVERSAL}/lib/libmobileffmpeg.${BUILD_LIBRARY_EXTENSION}"

    ${LIPO_COMMAND} 1>>${BASEDIR}/build.log 2>&1

    if [ $? -ne 0 ]; then
        echo -e "failed\n"
        exit 1
    fi

    cp -r ${BASEDIR}/prebuilt/tvos-${TARGET_ARCH_LIST[0]}-apple-darwin/mobile-ffmpeg/include/* ${MOBILE_FFMPEG_UNIVERSAL}/include 1>>${BASEDIR}/build.log 2>&1
    cp -r ${MOBILE_FFMPEG_UNIVERSAL}/include/* ${MOBILE_FFMPEG_FRAMEWORK_PATH}/Headers 1>>${BASEDIR}/build.log 2>&1
    cp ${MOBILE_FFMPEG_UNIVERSAL}/lib/libmobileffmpeg.${BUILD_LIBRARY_EXTENSION} ${MOBILE_FFMPEG_FRAMEWORK_PATH}/mobileffmpeg 1>>${BASEDIR}/build.log 2>&1

    # COPYING THE LICENSE
    if  [ ${GPL_ENABLED} == "yes" ]; then
        cp ${BASEDIR}/LICENSE.GPLv3 ${MOBILE_FFMPEG_UNIVERSAL}/LICENSE 1>>${BASEDIR}/build.log 2>&1
        cp ${BASEDIR}/LICENSE.GPLv3 ${MOBILE_FFMPEG_FRAMEWORK_PATH}/LICENSE 1>>${BASEDIR}/build.log 2>&1
    else
        cp ${BASEDIR}/LICENSE.LGPLv3 ${MOBILE_FFMPEG_UNIVERSAL}/LICENSE 1>>${BASEDIR}/build.log 2>&1
        cp ${BASEDIR}/LICENSE.LGPLv3 ${MOBILE_FFMPEG_FRAMEWORK_PATH}/LICENSE 1>>${BASEDIR}/build.log 2>&1
    fi

    build_info_plist "${MOBILE_FFMPEG_FRAMEWORK_PATH}/Info.plist" "mobileffmpeg" "com.arthenica.mobileffmpeg.MobileFFmpeg" "${MOBILE_FFMPEG_VERSION}" "${MOBILE_FFMPEG_VERSION}"

    echo -e "Created mobile-ffmpeg.framework successfully.\n" 1>>${BASEDIR}/build.log 2>&1

    echo -e "ok\n"
fi
