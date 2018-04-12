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

# ENABLE ARCH
ENABLED_ARCHITECTURES=(1 1 1 1 1)

# ENABLE LIBRARIES
ENABLED_LIBRARIES=(0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)

export BASEDIR=$(pwd)

# MIN VERSION IOS7
export IOS_MIN_VERSION=7.0

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
    echo -e "  --enable-wavpack\t\tbuild with wavpack"s
}

enable_library() {
    set_library $1 1
}

set_library() {
    case $1 in
        fontconfig)
            ENABLED_LIBRARIES[LIBRARY_FONTCONFIG]=$2
            ENABLED_LIBRARIES[LIBRARY_LIBUUID]=$2
            set_library "libxml2" $2
            set_library "freetype" $2
        ;;
        freetype)
            ENABLED_LIBRARIES[LIBRARY_FREETYPE]=$2
            ENABLED_LIBRARIES[LIBRARY_LIBPNG]=$2
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
            ENABLED_LIBRARIES[LIBRARY_LIBPNG]=$2
            ENABLED_LIBRARIES[LIBRARY_TIFF]=$2
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
        if [[ ENABLED_ARCHITECTURES[$print_arch] -eq 1 ]]; then
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
    for library in {0..17}
    do
        if [[ ENABLED_LIBRARIES[$library] -eq 1 ]]; then
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

# ENABLE COMMON FUNCTIONS
. ${BASEDIR}/build/ios-common.sh

while [ ! $# -eq 0 ]
do

    case $1 in
	    --help)
            display_help
            exit 0
	    ;;
	    --full)
            for library in {0..24}
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

for run_arch in {0..4}
do
    if [[ ENABLED_ARCHITECTURES[$run_arch] -eq 1 ]]; then
        export ARCH=$(get_arch_name $run_arch)
        export TARGET_SDK=$(get_target_sdk)
        export SDK_PATH=$(get_sdk_path)

        . ${BASEDIR}/build/main-ios.sh "${ENABLED_LIBRARIES[@]}"

        # CLEAR FLAGS
        for library in {1..25}
        do
            library_name=$(get_library_name $((library - 1)))
            unset $(echo "OK_${library_name}" | sed "s/\-/\_/g")
        done
    fi
done
