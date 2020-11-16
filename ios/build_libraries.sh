#!/bin/bash

# ARCH INDEXES
export ARCH_ARMV7=0
export ARCH_ARMV7S=1
export ARCH_ARM64=2
export ARCH_ARM64E=3
export ARCH_I386=4
export ARCH_X86_64=5
export ARCH_X86_64_MAC_CATALYST=6
export ARCH_ARM64_MAC_CATALYST=7
export ARCH_X86_64_MACOSX=8
export ARCH_ARM64_MACOSX=9
export ARCH_ARM64_SIMULATOR=10

# LIBRARY INDEXES
LIBRARY_FONTCONFIG=0
LIBRARY_FREETYPE=1
LIBRARY_FRIBIDI=2
LIBRARY_GMP=3
LIBRARY_GNUTLS=4
LIBRARY_LAME=5
LIBRARY_LIBASS=6
LIBRARY_LIBTHEORA=7
LIBRARY_LIBVORBIS=8
LIBRARY_LIBVPX=9
LIBRARY_LIBWEBP=10
LIBRARY_LIBXML2=11
LIBRARY_OPENCOREAMR=12
LIBRARY_SHINE=13
LIBRARY_SPEEX=14
LIBRARY_WAVPACK=15
LIBRARY_KVAZAAR=16
LIBRARY_X264=17
LIBRARY_XVIDCORE=18
LIBRARY_X265=19
LIBRARY_LIBVIDSTAB=20
LIBRARY_RUBBERBAND=21
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
LIBRARY_VO_AMRWBENC=32
LIBRARY_GIFLIB=33
LIBRARY_JPEG=34
LIBRARY_LIBOGG=35
LIBRARY_LIBPNG=36
LIBRARY_NETTLE=37
LIBRARY_TIFF=38
LIBRARY_EXPAT=39
LIBRARY_SNDFILE=40
LIBRARY_LEPTONICA=41
LIBRARY_LIBSAMPLERATE=42
LIBRARY_ZLIB=43
LIBRARY_AUDIOTOOLBOX=44
LIBRARY_BZIP2=45
LIBRARY_VIDEOTOOLBOX=46
LIBRARY_AVFOUNDATION=47
LIBRARY_LIBICONV=48
LIBRARY_LIBUUID=49

# ENABLE ARCH
export ENABLED_ARCHITECTURES=(0 0 0 0 0 0 0 0 0 0 0)

# ENABLE LIBRARIES
ENABLED_LIBRARIES=(0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)


export BASEDIR="$(dirname ${SRCROOT})"

RECONF_LIBRARIES=()
REBUILD_LIBRARIES=()
REDOWNLOAD_LIBRARIES=()

# CHECKING IF XCODE IS INSTALLED
if ! [ -x "$(command -v xcrun)" ]; then
    echo -e "\n(*) xcrun command not found. Please check your Xcode installation.\n"
    exit 1
fi

# USE 12.1 AS DEFAULT IOS_MIN_VERSION
if [[ -z ${IPHONEOS_DEPLOYMENT_TARGET} ]];then
    IPHONEOS_DEPLOYMENT_TARGET=13.0
fi
if [[ -z ${MACOSX_DEPLOYMENT_TARGET} ]];then
    MACOSX_DEPLOYMENT_TARGET=10.15
fi

CPUN=$(sysctl -n hw.ncpu)
export MAKEFLAGS="j ${CPUN} -j ${CPUN}"


get_mobile_ffmpeg_version() {
    local MOBILE_FFMPEG_VERSION=$(grep 'const MOBILE_FFMPEG_VERSION' "${BASEDIR}/ios/src/MobileFFmpeg.m" | grep -Eo '\".*\"' | sed -e 's/\"//g')

    if [[ -z ${MOBILE_FFMPEG_LTS_BUILD} ]]; then
        echo "${MOBILE_FFMPEG_VERSION}"
    else
        echo "${MOBILE_FFMPEG_VERSION}.LTS"
    fi
}

display_help() {
    COMMAND=$(echo "$0" | sed -e 's/\.\///g')

    echo -e "\n$COMMAND builds FFmpeg and MobileFFmpeg for iOS platform. By default seven architectures (armv7, armv7s, arm64, arm64e, i386, x86-64 and x86-64-mac-catalyst) are built without any external libraries enabled. Options can be used to disable architectures and/or enable external libraries. Please note that GPL libraries (external libraries with GPL license) need --enable-gpl flag to be set explicitly. When compilation ends, library bundles are created under the prebuilt folder. By default framework bundles and universal fat binaries are created. If --xcframework option is provided then xcframework bundles are created.\n"

    echo -e "Usage: ./$COMMAND [OPTION]...\n"

    echo -e "Specify environment variables as VARIABLE=VALUE to override default build options.\n"

    echo -e "Options:"

    echo -e "  -h, --help\t\t\tdisplay this help and exit"
    echo -e "  -v, --version\t\t\tdisplay version information and exit"
    echo -e "  -d, --debug\t\t\tbuild with debug information"
    echo -e "  -s, --speed\t\t\toptimize for speed instead of size"
    echo -e "  -l, --lts\t\t\tbuild lts packages to support sdk 9.3+ devices"
    echo -e "  -x, --xcframework\t\tbuild xcframework bundles instead of framework bundles and universal libraries"
    echo -e "  -f, --force\t\t\tignore warnings and build with given options"
    echo -e "  -f, --only-xcframework\t\tonly generate binary framework\n"

    echo -e "Licensing options:"

    echo -e "  --enable-gpl\t\t\tallow use of GPL libraries, created libs will be licensed under GPLv3.0 [no]\n"

    echo -e "Platforms:"

    echo -e "  --enable-armv7\t\tdo not build armv7 platform [yes]"
    echo -e "  --enable-armv7s\t\tdo not build armv7s platform [yes]"
    echo -e "  --enable-arm64\t\tdo not build arm64 platform [yes]"
    echo -e "  --enable-arm64e\t\tdo not build arm64e platform [yes]"
    echo -e "  --enable-i386-simulator\t\tdo not build i386 platform [yes]"
    echo -e "  --enable-x86-64-simulator\t\tdo not build x86-64 platform [yes]"
    echo -e "  --enable-arm64-simulator\tdo not build arm64 ios simulator platform [yes]"
    echo -e "  --enable-x86-64-mac-catalyst\tdo not build x86-64-mac-catalyst platform [yes]"
    echo -e "  --enable-arm64-mac-catalyst\tdo not build arm64-mac-catalyst platform [yes]"
    echo -e "  --enable-x86-64-macos\tdo not build x86-64-macos platform [yes]"
    echo -e "  --enable-arm64-macos\t\tdo not build arm64-macos platform [yes]\n"

    echo -e "Libraries:"

    echo -e "  --full\t\t\tenables all non-GPL external libraries"
    echo -e "  --enable-apple-audiotoolbox\tbuild with built-in Apple AudioToolbox support[no]"
    echo -e "  --enable-apple-avfoundation\tbuild with built-in Apple AVFoundation support[no]"
    echo -e "  --enable-apple-bzip2\t\tbuild with built-in bzip2 support[no]"
    echo -e "  --enable-apple-videotoolbox\tbuild with built-in Apple VideoToolbox support[no]"
    echo -e "  --enable-apple-zlib\t\tbuild with built-in zlib [no]"
    echo -e "  --enable-apple-libiconv\t\tbuild with built-in libiconv [no]"
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
    echo -e "  --enable-vo-amrwbenc\t\tbuild with vo-amrwbenc [no]"
    echo -e "  --enable-wavpack\t\tbuild with wavpack [no]\n"

    echo -e "GPL libraries:"

    echo -e "  --enable-libvidstab\t\tbuild with libvidstab [no]"
    echo -e "  --enable-rubberband\t\tbuild with rubber band [no]"
    echo -e "  --enable-x264\t\t\tbuild with x264 [no]"
    echo -e "  --enable-x265\t\t\tbuild with x265 [no]"
    echo -e "  --enable-xvidcore\t\tbuild with xvidcore [no]\n"

    echo -e "Advanced options:"

    echo -e "  --reconf-LIBRARY\t\trun autoreconf before building LIBRARY [no]"
    echo -e "  --redownload-LIBRARY\t\tdownload LIBRARY even it is detected as already downloaded [no]"
    echo -e "  --rebuild-LIBRARY\t\tbuild LIBRARY even it is detected as already built [no]\n"
}

display_version() {
    local COMMAND=$(echo "$0" | sed -e 's/\.\///g')

    echo -e "\
        $COMMAND v$(get_mobile_ffmpeg_version)\n\
        Copyright (c) 2018-2020 Taner Sener\n\
        License LGPLv3.0: GNU LGPL version 3 or later\n\
        <https://www.gnu.org/licenses/lgpl-3.0.en.html>\n\
        This is free software: you can redistribute it and/or modify it under the terms of the \
        GNU Lesser General Public License as published by the Free Software Foundation, \
        either version 3 of the License, or (at your option) any later version."
    }

skip_library() {
    SKIP_VARIABLE=$(echo "SKIP_$1" | sed "s/\-/\_/g")

    export "${SKIP_VARIABLE}"=1
}

no_output_redirection() {
    export NO_OUTPUT_REDIRECTION=1
}

no_workspace_cleanup_library() {
    NO_WORKSPACE_CLEANUP_VARIABLE=$(echo "NO_WORKSPACE_CLEANUP_$1" | sed "s/\-/\_/g")

    export "${NO_WORKSPACE_CLEANUP_VARIABLE}"=1
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

  # XCODE 7.3 HAS IOS SDK 9.3
  export IOS_MIN_VERSION=9.3
}

reconf_library() {
    local RECONF_VARIABLE=$(echo "RECONF_$1" | sed "s/\-/\_/g")
    local library_supported=0

    for library in {1..43}; do
        library_name=$(get_library_name $((library - 1)))

        if [[ $1 != "ffmpeg" ]] && [[ ${library_name} == "$1" ]]; then
            export "${RECONF_VARIABLE}"=1
            RECONF_LIBRARIES+=("$1")
            library_supported=1
        fi
    done

    if [[ ${library_supported} -eq 0 ]]; then
        echo -e "INFO: --reconf flag detected for library $1 is not supported.\n" 1>>"${BASEDIR}/build.log" 2>&1
    fi
}

rebuild_library() {
    local REBUILD_VARIABLE=$(echo "REBUILD_$1" | sed "s/\-/\_/g")
    local library_supported=0

    for library in {1..43}; do
        library_name=$(get_library_name $((library - 1)))

        if [[ $1 != "ffmpeg" ]] && [[ ${library_name} == "$1" ]]; then
            export "${REBUILD_VARIABLE}"=1
            REBUILD_LIBRARIES+=("$1")
            library_supported=1
        fi
    done

    if [[ ${library_supported} -eq 0 ]]; then
        echo -e "INFO: --rebuild flag detected for library $1 is not supported.\n" 1>>"${BASEDIR}/build.log" 2>&1
    fi
}

redownload_library() {
    local REDOWNLOAD_VARIABLE=$(echo "REDOWNLOAD_$1" | sed "s/\-/\_/g")
    local library_supported=0

    for library in {0..42}; do
        library_name=$(get_library_name ${library})

        if [[ ${library_name} == $1 ]]; then
            export ${REDOWNLOAD_VARIABLE}=1
            REDOWNLOAD_LIBRARIES+=($1)
            library_supported=1
        fi
    done

    if [[ "ffmpeg" == $1 ]]; then
        export ${REDOWNLOAD_VARIABLE}=1
        REDOWNLOAD_LIBRARIES+=($1)
        library_supported=1
    fi

    if [[ ${library_supported} -eq 0 ]]; then
        echo -e "INFO: --redownload flag detected for library $1 is not supported.\n" 1>>${BASEDIR}/build.log 2>&1
    fi
}

enable_library() {
    set_library "$1" 1
}

set_library() {
    case $1 in
        apple-zlib)
            ENABLED_LIBRARIES[LIBRARY_ZLIB]=$2
            ;;
        apple-audiotoolbox)
            ENABLED_LIBRARIES[LIBRARY_AUDIOTOOLBOX]=$2
            ;;
        apple-avfoundation)
            ENABLED_LIBRARIES[LIBRARY_AVFOUNDATION]=$2
            ;;
        apple-bzip2)
            ENABLED_LIBRARIES[LIBRARY_BZIP2]=$2
            ;;
        apple-videotoolbox)
            ENABLED_LIBRARIES[LIBRARY_VIDEOTOOLBOX]=$2
            ;;
        apple-libiconv)
            ENABLED_LIBRARIES[LIBRARY_LIBICONV]=$2
            ;;
        apple-libuuid)
            ENABLED_LIBRARIES[LIBRARY_LIBUUID]=$2
            ;;
        chromaprint)
            ENABLED_LIBRARIES[LIBRARY_CHROMAPRINT]=$2
            ;;
        fontconfig)
            ENABLED_LIBRARIES[LIBRARY_FONTCONFIG]=$2
            ENABLED_LIBRARIES[LIBRARY_LIBUUID]=$2
            ENABLED_LIBRARIES[LIBRARY_EXPAT]=$2
            ENABLED_LIBRARIES[LIBRARY_LIBICONV]=$2
            set_library "freetype" "$2"
            ;;
        freetype)
            ENABLED_LIBRARIES[LIBRARY_FREETYPE]=$2
            ENABLED_LIBRARIES[LIBRARY_ZLIB]=$2
            set_library "libpng" "$2"
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
            set_library "nettle" "$2"
            set_library "gmp" "$2"
            set_library "apple-libiconv" "$2"
            ;;
        kvazaar)
            ENABLED_LIBRARIES[LIBRARY_KVAZAAR]=$2
            ;;
        lame)
            ENABLED_LIBRARIES[LIBRARY_LAME]=$2
            set_library "apple-libiconv" "$2"
            ;;
        libaom)
            ENABLED_LIBRARIES[LIBRARY_LIBAOM]=$2
            ;;
        libass)
            ENABLED_LIBRARIES[LIBRARY_LIBASS]=$2
            ENABLED_LIBRARIES[LIBRARY_LIBUUID]=$2
            ENABLED_LIBRARIES[LIBRARY_EXPAT]=$2
            set_library "freetype" "$2"
            set_library "fribidi" "$2"
            set_library "fontconfig" "$2"
            set_library "apple-libiconv" "$2"
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
            set_library "libvorbis" "$2"
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
            set_library "tiff" "$2"
            set_library "libpng" "$2"
            ;;
        libxml2)
            ENABLED_LIBRARIES[LIBRARY_LIBXML2]=$2
            set_library "apple-libiconv" "$2"
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
        rubberband)
            ENABLED_LIBRARIES[LIBRARY_RUBBERBAND]=$2
            ENABLED_LIBRARIES[LIBRARY_SNDFILE]=$2
            ENABLED_LIBRARIES[LIBRARY_LIBSAMPLERATE]=$2
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
            set_library "tiff" "$2"
            set_library "libpng" "$2"
            ;;
        twolame)
            ENABLED_LIBRARIES[LIBRARY_TWOLAME]=$2
            ENABLED_LIBRARIES[LIBRARY_SNDFILE]=$2
            ;;
        vo-amrwbenc)
            ENABLED_LIBRARIES[LIBRARY_VO_AMRWBENC]=$2
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
        expat | giflib | jpeg | leptonica | libogg | libsamplerate | libsndfile)
            # THESE LIBRARIES ARE NOT ENABLED DIRECTLY
            ;;
        nettle)
            ENABLED_LIBRARIES[LIBRARY_NETTLE]=$2
            set_library "gmp" "$2"
            ;;
        tiff)
            ENABLED_LIBRARIES[LIBRARY_TIFF]=$2
            ENABLED_LIBRARIES[LIBRARY_JPEG]=$2
            ;;
        *)
            enable_arch "$1"
            ;;
    esac
}

enable_arch() {
    set_arch "$1" 1
}

disable_arch() {
    set_arch "$1" 0
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
        i386-simulator)
            ENABLED_ARCHITECTURES[ARCH_I386]=$2
            ;;
        x86-64-simulator)
            ENABLED_ARCHITECTURES[ARCH_X86_64]=$2
            ;;
        x86-64-mac-catalyst)
            ENABLED_ARCHITECTURES[ARCH_X86_64_MAC_CATALYST]=$2
            ;;
        arm64-mac-catalyst)
            ENABLED_ARCHITECTURES[ARCH_ARM64_MAC_CATALYST]=$2
            ;;
        x86-64-macos)
            ENABLED_ARCHITECTURES[ARCH_X86_64_MACOSX]=$2
            ;;
        arm64-macos)
            ENABLED_ARCHITECTURES[ARCH_ARM64_MACOSX]=$2
            ;;
        arm64-simulator)
            ENABLED_ARCHITECTURES[ARCH_ARM64_SIMULATOR]=$2
            ;;
        *)
            print_unknown_platform "$1"
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
    echo -e "Unknown platform or library \"$1\".\nSee $0 --help for available platforms and libraries."
    exit 1
}

print_enabled_architectures() {
    echo -n "Architectures: "

    let enabled=0
    for print_arch in {0..10}; do
        if [[ ${ENABLED_ARCHITECTURES[$print_arch]} -eq 1 ]]; then
            if [[ ${enabled} -ge 1 ]]; then
                echo -n ", "
            fi
            echo -n "$(get_arch_name $print_arch)"
            enabled=$((enabled + 1))
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

    let enabled=0

  # FIRST BUILT-IN LIBRARIES
  for library in {43..49}; do
      if [[ ${ENABLED_LIBRARIES[$library]} -eq 1 ]]; then
          if [[ ${enabled} -ge 1 ]]; then
              echo -n ", "
          fi
          echo -n "$(get_library_name $library)"
          enabled=$((enabled + 1))
      fi
  done

  # THEN EXTERNAL LIBRARIES
  for library in {0..32}; do
      if [[ ${ENABLED_LIBRARIES[$library]} -eq 1 ]]; then
          if [[ ${enabled} -ge 1 ]]; then
              echo -n ", "
          fi
          echo -n "$(get_library_name $library)"
          enabled=$((enabled + 1))
      fi
  done

  if [ ${enabled} -gt 0 ]; then
      echo ""
  else
      echo "none"
  fi
}

print_reconfigure_requested_libraries() {
    local counter=0

    for RECONF_LIBRARY in "${RECONF_LIBRARIES[@]}"; do
        if [[ ${counter} -eq 0 ]]; then
            echo -n "Reconfigure: "
        else
            echo -n ", "
        fi

        echo -n "${RECONF_LIBRARY}"

        counter=$((counter + 1))
    done

    if [[ ${counter} -gt 0 ]]; then
        echo ""
    fi
}

print_rebuild_requested_libraries() {
    local counter=0

    for REBUILD_LIBRARY in "${REBUILD_LIBRARIES[@]}"; do
        if [[ ${counter} -eq 0 ]]; then
            echo -n "Rebuild: "
        else
            echo -n ", "
        fi

        echo -n "${REBUILD_LIBRARY}"

        counter=$((counter + 1))
    done

    if [[ ${counter} -gt 0 ]]; then
        echo ""
    fi
}

print_redownload_requested_libraries() {
    local counter=0

    for REDOWNLOAD_LIBRARY in "${REDOWNLOAD_LIBRARIES[@]}"; do
        if [[ ${counter} -eq 0 ]]; then
            echo -n "Redownload: "
        else
            echo -n ", "
        fi

        echo -n ${REDOWNLOAD_LIBRARY}

        counter=$((${counter} + 1))
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

    cat >"${FILE_PATH}" <<EOF
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

build_modulemap() {
    local FILE_PATH="$1"

    cat >"${FILE_PATH}" <<EOF
framework module mobileffmpeg {

  header "ArchDetect.h"
  header "AtomicLong.h"
  header "ExecuteDelegate.h"
  header "FFmpegExecution.h"
  header "LogDelegate.h"
  header "MediaInformation.h"
  header "MediaInformationParser.h"
  header "MobileFFmpeg.h"
  header "MobileFFmpegConfig.h"
  header "MobileFFprobe.h"
  header "Statistics.h"
  header "StatisticsDelegate.h"
  header "StreamInformation.h"
  header "mobileffmpeg_exception.h"

  export *
}
EOF
}

generate_configh() {
    local CONFIGH_PATH="$1"
    # config.h
    cat >"${CONFIGH_PATH}" <<EOF
/* config.h */
#if defined(__APPLE__) && defined (__x86_64__)
# include "config-x86_64.h"
#endif

#if defined(__APPLE__) && defined (__i386__)
# include "config-i386.h"
#endif

#if defined(__APPLE__) && defined (__arm__) && defined (__ARM_ARCH_7A__)
# include "config-armv7.h"
#endif

#if defined(__APPLE__) && defined (__arm__) && defined (__ARM_ARCH_7S__)
# include "config-armv7s.h"
#endif

#if defined(__APPLE__) && (defined (__arm64__) || defined (__aarch64__))
# include "config-aarch64.h"
#endif
EOF
}


# 1 - library index
# 2 - library name
# 3 - static library name
# 4 - library version
create_external_library_package() {
    if [[ -n ${MOBILE_FFMPEG_XCF_BUILD} ]]; then

    # 1. CREATE INDIVIDUAL FRAMEWORKS
    for TARGET_ARCH in "${TARGET_ARCH_LIST[@]}"; do
        if [[ ${TARGET_ARCH} != "arm64e" ]]; then



            local FRAMEWORK_PATH=${BASEDIR}/prebuilt/ios-xcframework/.tmp/ios-${TARGET_ARCH}/$2.framework
            mkdir -p "${FRAMEWORK_PATH}" 1>>"${BASEDIR}/build.log" 2>&1 || exit 1
            local STATIC_LIBRARY_PATH=$(find ${BASEDIR}/prebuilt/ios-${TARGET_ARCH} -name $3)
            local CAPITAL_CASE_LIBRARY_NAME=$(to_capital_case "$2")

            build_info_plist "${FRAMEWORK_PATH}/Info.plist" "$2" "com.arthenica.mobileffmpeg.${CAPITAL_CASE_LIBRARY_NAME}" "$4" "$4"
            cp "${STATIC_LIBRARY_PATH}" "${FRAMEWORK_PATH}/$2" 1>>"${BASEDIR}/build.log" 2>&1
        fi
    done


    for TARGET_ARCH in "${TARGET_ARCH_LIST[@]}"; do
        if [[ ${TARGET_ARCH} != "arm64e" ]]; then
            local FRAMEWORK_PATH=${BASEDIR}/prebuilt/ios-xcframework/.tmp/ios-${TARGET_ARCH}/$2.framework
        fi
    done

    echo "" 1>>"${BASEDIR}/build.log" 2>&1

    if [[ ${COMMAND_OUTPUT} == *"is empty in library"* ]]; then
        RC=1
    else
        RC=0
    fi

else

    # 1. CREATE FAT LIBRARY
    local FAT_LIBRARY_PATH=${BASEDIR}/prebuilt/ios-universal/$2-universal

    mkdir -p "${FAT_LIBRARY_PATH}/lib" 1>>"${BASEDIR}/build.log" 2>&1 || exit 1

    LIPO_COMMAND="${LIPO} -create"

    for TARGET_ARCH in "${TARGET_ARCH_LIST[@]}"; do
        LIPO_COMMAND+=" $(find ${BASEDIR}/prebuilt/ios-${TARGET_ARCH} -name $3)"
    done

    LIPO_COMMAND+=" -output ${FAT_LIBRARY_PATH}/lib/$3"

    RC=$(${LIPO_COMMAND} 1>>"${BASEDIR}/build.log" 2>&1)

    if [[ ${RC} -eq 0 ]]; then

      # 2. CREATE FRAMEWORK
      RC=$(create_static_framework "$2" "$3" "$4")

      if [[ ${RC} -eq 0 ]]; then

    # 3. COPY LICENSES
    if [[ ${LIBRARY_LIBTHEORA} == "$1" ]]; then
        license_directories=("${BASEDIR}/prebuilt/ios-universal/libtheora-universal" "${BASEDIR}/prebuilt/ios-universal/libtheoraenc-universal" "${BASEDIR}/prebuilt/ios-universal/libtheoradec-universal" "${BASEDIR}/prebuilt/ios-framework/libtheora.framework" "${BASEDIR}/prebuilt/ios-framework/libtheoraenc.framework" "${BASEDIR}/prebuilt/ios-framework/libtheoradec.framework")
    elif [[ ${LIBRARY_LIBVORBIS} == "$1" ]]; then
        license_directories=("${BASEDIR}/prebuilt/ios-universal/libvorbisfile-universal" "${BASEDIR}/prebuilt/ios-universal/libvorbisenc-universal" "${BASEDIR}/prebuilt/ios-universal/libvorbis-universal" "${BASEDIR}/prebuilt/ios-framework/libvorbisfile.framework" "${BASEDIR}/prebuilt/ios-framework/libvorbisenc.framework" "${BASEDIR}/prebuilt/ios-framework/libvorbis.framework")
    elif [[ ${LIBRARY_LIBWEBP} == "$1" ]]; then
        license_directories=("${BASEDIR}/prebuilt/ios-universal/libwebpmux-universal" "${BASEDIR}/prebuilt/ios-universal/libwebpdemux-universal" "${BASEDIR}/prebuilt/ios-universal/libwebp-universal" "${BASEDIR}/prebuilt/ios-framework/libwebpmux.framework" "${BASEDIR}/prebuilt/ios-framework/libwebpdemux.framework" "${BASEDIR}/prebuilt/ios-framework/libwebp.framework")
    elif [[ ${LIBRARY_OPENCOREAMR} == "$1" ]]; then
        license_directories=("${BASEDIR}/prebuilt/ios-universal/libopencore-amrnb-universal" "${BASEDIR}/prebuilt/ios-framework/libopencore-amrnb.framework")
    elif [[ ${LIBRARY_NETTLE} == "$1" ]]; then
        license_directories=("${BASEDIR}/prebuilt/ios-universal/libnettle-universal" "${BASEDIR}/prebuilt/ios-universal/libhogweed-universal" "${BASEDIR}/prebuilt/ios-framework/libnettle.framework" "${BASEDIR}/prebuilt/ios-framework/libhogweed.framework")
    else
        license_directories=("${BASEDIR}/prebuilt/ios-universal/$2-universal" "${BASEDIR}/prebuilt/ios-framework/$2.framework")
    fi

    RC=$(copy_external_library_license "$1" "${license_directories[@]}")
      fi

    fi

    fi

    echo "${RC}"

}

# 1 - library name
# 2 - static library name
# 3 - library version
create_static_framework() {
    local FRAMEWORK_PATH=${BASEDIR}/prebuilt/ios-framework/$1.framework

    mkdir -p "${FRAMEWORK_PATH}" 1>>"${BASEDIR}/build.log" 2>&1 || exit 1

    local CAPITAL_CASE_LIBRARY_NAME=$(to_capital_case "$1")

    build_info_plist "${FRAMEWORK_PATH}/Info.plist" "${FFMPEG_LIB}" "com.arthenica.mobileffmpeg.${CAPITAL_CASE_LIBRARY_NAME}" "$3" "$3"

    cp "${BASEDIR}/prebuilt/ios-universal/$1-universal/lib/$2" "${FRAMEWORK_PATH}/$1" 1>>"${BASEDIR}/build.log" 2>&1

    echo "$?"
}

# 1 - library index
# 2 - output path
copy_external_library_license() {
    output_path_array="$2"
    for output_path in "${output_path_array[@]}"
    do
        $(cp $(get_external_library_license_path "$1") "${output_path}/LICENSE" 1>>"${BASEDIR}/build.log" 2>&1)
        if [ $? -ne 0 ]; then
            echo 1
            return
        fi
    done;
    echo 0
}

# 1 - library index
get_external_library_license_path() {
    case $1 in
        1) echo "${BASEDIR}/src/$(get_library_name "$1")/docs/LICENSE.TXT" ;;
        3) echo "${BASEDIR}/src/$(get_library_name "$1")/COPYING.LESSERv3" ;;
        25) echo "${BASEDIR}/src/$(get_library_name "$1")/COPYING.LGPL" ;;
        27) echo "${BASEDIR}/src/$(get_library_name "$1")/LICENSE.md" ;;
        29) echo "${BASEDIR}/src/$(get_library_name "$1")/COPYING.txt" ;;
        34) echo "${BASEDIR}/src/$(get_library_name "$1")/LICENSE.md " ;;
        37) echo "${BASEDIR}/src/$(get_library_name "$1")/COPYING.LESSERv3" ;;
        38) echo "${BASEDIR}/src/$(get_library_name "$1")/COPYRIGHT" ;;
        41) echo "${BASEDIR}/src/$(get_library_name "$1")/leptonica-license.txt" ;;
        4 | 9 | 12 | 18 | 20 | 26 | 31 | 36) echo "${BASEDIR}/src/$(get_library_name "$1")/LICENSE" ;;
        *) echo "${BASEDIR}/src/$(get_library_name "$1")/COPYING" ;;
    esac
}

get_external_library_version() {
    echo "$(grep Version ${BASEDIR}/prebuilt/${TARGET_ARCH_LIST[0]}/pkgconfig/$1.pc 2>>"${BASEDIR}/build.log" | sed 's/Version://g;s/\ //g')"
}

# ENABLE COMMON FUNCTIONS
. ${BASEDIR}/build/apple-common.sh

echo -e "\nINFO: Build options: $*\n" 1>>"${BASEDIR}/build.log" 2>&1

GPL_ENABLED="no"
DISPLAY_HELP=""
BUILD_TYPE_ID=""
BUILD_LTS=""
BUILD_FULL=""
MOBILE_FFMPEG_XCF_BUILD=""
BUILD_FORCE=""
BUILD_VERSION=$(git describe --tags 2>>"${BASEDIR}/build.log")

CPUN=$(sysctl -n hw.ncpu)
export MAKEFLAGS="-j${CPUN}"

while [ ! $# -eq 0 ]; do
    case $1 in
        -h | --help)
            DISPLAY_HELP="1"
            ;;
        -v | --version)
            display_version
            exit 0
            ;;
        --skip-*)
            SKIP_LIBRARY=$(echo "$1" | sed -e 's/^--[A-Za-z]*-//g')

            skip_library "${SKIP_LIBRARY}"
            ;;
        --no-output-redirection)
            no_output_redirection
            ;;
        --no-workspace-cleanup-*)
            NO_WORKSPACE_CLEANUP_LIBRARY=$(echo $1 | sed -e 's/^--[A-Za-z]*-[A-Za-z]*-[A-Za-z]*-//g')

            no_workspace_cleanup_library "${NO_WORKSPACE_CLEANUP_LIBRARY}"
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
        -x | --xcframework)
            MOBILE_FFMPEG_XCF_BUILD="1"
            ;;
        -f | --force)
            BUILD_FORCE="1"
            ;;
        --only-xcframework)
            ONLY_XCFRAMEWORK="1"
            ;;
        --reconf-*)
            CONF_LIBRARY=$(echo $1 | sed -e 's/^--[A-Za-z]*-//g')

            reconf_library "${CONF_LIBRARY}"
            ;;
        --rebuild-*)
            BUILD_LIBRARY=$(echo $1 | sed -e 's/^--[A-Za-z]*-//g')

            rebuild_library "${BUILD_LIBRARY}"
            ;;
        --redownload-*)
            DOWNLOAD_LIBRARY=$(echo $1 | sed -e 's/^--[A-Za-z]*-//g')

            redownload_library ${DOWNLOAD_LIBRARY}
            ;;
        --full)
            BUILD_FULL="1"
            ;;
        --enable-gpl)
            GPL_ENABLED="yes"
            ;;
        --enable-*)
            ENABLED_LIBRARY=$(echo $1 | sed -e 's/^--[A-Za-z]*-//g')

            enable_library "${ENABLED_LIBRARY}"
            ;;
        --enable-*)
            ENABLE_ARCH=$(echo $1 | sed -e 's/^--[A-Za-z]*-//g')

            enable_arch "${ENABLE_ARCH}"
            ;;
        *)
            print_unknown_option "$1"
            ;;
    esac
    shift
done

if [[ -n ${BUILD_FULL} ]]; then
    for library in {0..49}; do
        if [ ${GPL_ENABLED} == "yes" ]; then
            enable_library $(get_library_name $library)
        else
            if [[ $library -ne 17 ]] && [[ $library -ne 18 ]] && [[ $library -ne 19 ]] && [[ $library -ne 20 ]] && [[ $library -ne 21 ]]; then
                enable_library $(get_library_name $library)
            fi
        fi
    done
fi

# DETECT BUILD TYPE
if [[ -n ${BUILD_LTS} ]]; then
    enable_lts_build
    BUILD_TYPE_ID+="LTS "
fi

if [[ -n ${DISPLAY_HELP} ]]; then
    display_help
    exit 0
fi

if [[ -z ${BUILD_VERSION} ]]; then
    echo -e "\nerror: Can not run git commands in this folder. See build.log.\n"
    exit 1
fi

# SELECT XCODE VERSION USED FOR BUILDING
XCODE_FOR_MOBILE_FFMPEG=~/.xcode.for.mobile.ffmpeg.sh
if [[ -f ${XCODE_FOR_MOBILE_FFMPEG} ]]; then
    source ${XCODE_FOR_MOBILE_FFMPEG} 1>>"${BASEDIR}/build.log" 2>&1
fi
DETECTED_IOS_SDK_VERSION="$(xcrun --sdk iphoneos --show-sdk-version)"

echo -e "INFO: Using SDK ${DETECTED_IOS_SDK_VERSION} by Xcode provided at $(xcode-select -p)\n" 1>>"${BASEDIR}/build.log" 2>&1
if [[ -n ${MOBILE_FFMPEG_LTS_BUILD} ]] && [[ "${DETECTED_IOS_SDK_VERSION}" != "${IOS_MIN_VERSION}" ]]; then
    echo -e "\n(*) LTS packages should be built using SDK ${IOS_MIN_VERSION} but current configuration uses SDK ${DETECTED_IOS_SDK_VERSION}\n"

    if [[ -z ${BUILD_FORCE} ]]; then
        exit 1
    fi
fi

# DISABLE 32-bit architectures on newer IOS versions
if [[ ${DETECTED_IOS_SDK_VERSION} == 11* ]] || [[ ${DETECTED_IOS_SDK_VERSION} == 12* ]] || [[ ${DETECTED_IOS_SDK_VERSION} == 13* ]] || [[ ${DETECTED_IOS_SDK_VERSION} == 14* ]]; then
    if [[ -z ${BUILD_FORCE} ]] && [[ ${ENABLED_ARCHITECTURES[${ARCH_ARMV7}]} -eq 1 ]]; then
        echo -e "INFO: Disabled armv7 architecture which is not supported on SDK ${DETECTED_IOS_SDK_VERSION}\n" 1>>"${BASEDIR}/build.log" 2>&1
        disable_arch "armv7"
    elif [[ -n ${BUILD_FORCE} ]] && [[ ${ENABLED_ARCHITECTURES[${ARCH_ARMV7}]} -eq 1 ]]; then
        export IOS_MIN_VERSION=10.0
    fi
    if [[ -z ${BUILD_FORCE} ]] && [[ ${ENABLED_ARCHITECTURES[${ARCH_ARMV7S}]} -eq 1 ]]; then
        echo -e "INFO: Disabled armv7s architecture which is not supported on SDK ${DETECTED_IOS_SDK_VERSION}\n" 1>>"${BASEDIR}/build.log" 2>&1
        disable_arch "armv7s"
    elif [[ -n ${BUILD_FORCE} ]] && [[ ${ENABLED_ARCHITECTURES[${ARCH_ARMV7S}]} -eq 1 ]]; then
        export IOS_MIN_VERSION=10.0
    fi
    if [[ -z ${BUILD_FORCE} ]] && [[ ${ENABLED_ARCHITECTURES[${ARCH_I386}]} -eq 1 ]]; then
        echo -e "INFO: Disabled i386 architecture which is not supported on SDK ${DETECTED_IOS_SDK_VERSION}\n" 1>>"${BASEDIR}/build.log" 2>&1
        disable_arch "i386"
    elif [[ -n ${BUILD_FORCE} ]] && [[ ${ENABLED_ARCHITECTURES[${ARCH_I386}]} -eq 1 ]]; then
        echo -e "INFO: Disabled i386 architecture which is not supported on SDK ${DETECTED_IOS_SDK_VERSION}\n" 1>>"${BASEDIR}/build.log" 2>&1
        export MACOSX_MIN_VERSION=10.12
    fi

# DISABLE arm64e architecture on older IOS versions
elif [[ ${DETECTED_IOS_SDK_VERSION} != 10* ]]; then
    if [[ -z ${BUILD_FORCE} ]] && [[ ${ENABLED_ARCHITECTURES[${ARCH_ARM64E}]} -eq 1 ]]; then
        echo -e "INFO: Disabled arm64e architecture which is not supported on SDK ${DETECTED_IOS_SDK_VERSION}\n" 1>>"${BASEDIR}/build.log" 2>&1
        disable_arch "arm64e"
    fi
fi

# DISABLE x86-64-mac-catalyst architecture on IOS versions lower than 13
if [[ ${DETECTED_IOS_SDK_VERSION} != 13* && ${DETECTED_IOS_SDK_VERSION} != 14* ]] && [[ -z ${BUILD_FORCE} ]] && [[ ${ENABLED_ARCHITECTURES[${ARCH_X86_64_MAC_CATALYST}]} -eq 1 ]]; then
    echo -e "INFO: Disabled x86-64-mac-catalyst architecture which is not supported on SDK ${DETECTED_IOS_SDK_VERSION}\n" 1>>"${BASEDIR}/build.log" 2>&1
    disable_arch "x86_64-mac-catalyst"
fi

# DISABLE arm64e when arm64 is enabled
if [[ -n ${MOBILE_FFMPEG_XCF_BUILD} ]] && [[ ${ENABLED_ARCHITECTURES[${ARCH_ARM64E}]} -eq 1 ]] && [[ ${ENABLED_ARCHITECTURES[${ARCH_ARM64}]} -eq 1 ]]; then
    echo -e "INFO: Disabled arm64e architecture which can not co-exist with arm64 in an xcframework bundle.\n" 1>>"${BASEDIR}/build.log" 2>&1
    disable_arch "arm64e"
fi

echo -e "\nBuilding mobile-ffmpeg ${BUILD_TYPE_ID}static library for Apple Devices\n"

echo -e -n "INFO: Building mobile-ffmpeg ${BUILD_VERSION} ${BUILD_TYPE_ID}for Apple Devices: " 1>>"${BASEDIR}/build.log" 2>&1
echo -e "$(date)" 1>>"${BASEDIR}/build.log" 2>&1

print_enabled_architectures
print_enabled_libraries
print_reconfigure_requested_libraries
print_rebuild_requested_libraries
print_redownload_requested_libraries

# CHECK GPL LIBRARIES
for gpl_library in {17..21}; do
    if [[ ${ENABLED_LIBRARIES[$gpl_library]} -eq 1 ]]; then
        library_name=$(get_library_name ${gpl_library})

        if [ ${GPL_ENABLED} != "yes" ]; then
            echo -e "\n(*) Invalid configuration detected. GPL library ${library_name} enabled without --enable-gpl flag.\n"
            echo -e "\n(*) Invalid configuration detected. GPL library ${library_name} enabled without --enable-gpl flag.\n" 1>>"${BASEDIR}/build.log" 2>&1
            exit 1
        else
            DOWNLOAD_RESULT=$(download_gpl_library_source "${library_name}")
            if [[ ${DOWNLOAD_RESULT} -ne 0 ]]; then
                echo -e "\n(*) Failed to download GPL library ${library_name} source. Please check build.log file for details. If the problem persists refer to offline building instructions.\n"
                echo -e "\n(*) Failed to download GPL library ${library_name} source.\n" 1>>"${BASEDIR}/build.log" 2>&1
                exit 1
            fi
        fi
    fi
done

TARGET_ARCH_LIST=()

for run_arch in {0..10}; do
    if [[ ${ENABLED_ARCHITECTURES[$run_arch]} -eq 1 ]]; then
        export ARCH=$(get_arch_name $run_arch)
        export TARGET_SDK=$(get_target_host)
        export SDK_PATH=$(get_sdk_path)
        export SDK_NAME=$(get_sdk_name)

        export LIPO="$(xcrun --sdk $(get_sdk_name) -f lipo)"

        . ${BASEDIR}/build/main-apple.sh "${ENABLED_LIBRARIES[@]}"
        TARGET_ARCH_LIST+=("${ARCH}")

    # CLEAR FLAGS
    for library in {1..50}; do
        library_name=$(get_library_name $((library - 1)))
        unset "$(echo "OK_${library_name}" | sed "s/\-/\_/g")"
        unset "$(echo "DEPENDENCY_REBUILT_${library_name}" | sed "s/\-/\_/g")"
    done
    fi
done

FFMPEG_LIBS="libavcodec libavdevice libavfilter libavformat libavutil libswresample libswscale"

BUILD_LIBRARY_EXTENSION="a"

echo "Platform: ${PLATFORM}"

export PLATFORM_NAME="${PLATFORM}"
export PLATFORM="${BASEDIR}/prebuilt/${PLATFORM_NAME}"

rm -rf ${PLATFORM}/{lib,include}
rm -rf ${PLATFORM}/MobileFFmpeg.framework

ARCH_BASE=$(ls -1 ${PLATFORM} | head -n 1)

mkdir -p ${PLATFORM}/{lib,include}

echo "Generating universal libraries..."

for LIB in ${PLATFORM}/${ARCH_BASE}/*/lib/*.a; do
    LIB_NAME=$(basename $LIB)
    LIBS=$(find ${PLATFORM} -not -path "*${ARCH_BASE}*" -and -name "${LIB_NAME}" | tr '\n' ' ')
    if [ -n "${LIBS}" ]; then
        lipo -create ${LIBS} ${LIB} -output ${PLATFORM}/lib/${LIB_NAME} >>"${BASEDIR}/build.log" 2>&1
    else
        cp ${LIB} ${PLATFORM}/lib/ >>"${BASEDIR}/build.log" 2>&1
    fi
done

echo "Archiving external libraries in libffmpeg..."
LIBS="$(ls ${PLATFORM}/lib/* | tr '\n' ' ')"
echo "Libs to archive in libffmpeg: ${LIBS}"
libtool -static ${LIBS} -o ${PLATFORM}/lib/libffmpeg.a >>"${BASEDIR}/build.log" 2>&1
find ${PLATFORM}/lib -not -name "libffmpeg.a" -type f -delete >>"${BASEDIR}/build.log" 2>&1

echo "Copying header files"
for file in ${PLATFORM}/*/*/include/*; do
    cp -rf $file ${PLATFORM}/include/ >>"${BASEDIR}/build.log" 2>&1
done

echo "Generating config.h file..."
generate_configh ${PLATFORM}/include/config.h

echo "Generating Symbolic Links..."

rm ${BASEDIR}/ios/Sources/ffmpeg/lib
rm ${BASEDIR}/ios/Sources/ffmpeg/include

ln -s ${PLATFORM}/include ${BASEDIR}/ios/Sources/ffmpeg/include
ln -s ${PLATFORM}/lib ${BASEDIR}/ios/Sources/ffmpeg/lib

echo "Copying modulemap file..."
cp ${BASEDIR}/ios/Sources/ffmpeg/module.modulemap ${PLATFORM}/include

