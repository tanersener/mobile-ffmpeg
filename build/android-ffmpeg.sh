#!/bin/bash

android_get_target_machine() {
    case ${ARCH} in
        arm)
            echo "armv7-a"
        ;;
        arm64)
            echo "aarch64"
        ;;
        x86)
            echo "i686"
        ;;
        x86_64)
            echo "x86_64"
        ;;
    esac
}

if [[ -z $1 ]]; then
    echo "usage: $0 <enabled libraries list>"
    exit 1
fi

if [[ -z ${ANDROID_NDK_ROOT} ]]; then
    echo "ANDROID_NDK_ROOT not defined"
    exit 1
fi

if [[ -z ${ARCH} ]]; then
    echo "ARCH not defined"
    exit 1
fi

if [[ -z ${API} ]]; then
    echo "API not defined"
    exit 1
fi

if [[ -z ${BASEDIR} ]]; then
    echo "BASEDIR not defined"
    exit 1
fi

HOST_PKG_CONFIG_PATH=`type pkg-config 2>/dev/null | sed 's/.*is //g'`
if [ -z ${HOST_PKG_CONFIG_PATH} ]; then
    echo "pkg-config not found"
    exit 1
fi

# ENABLE COMMON FUNCTIONS
. ${BASEDIR}/build/common.sh

# PREPARING PATHS
android_prepare_toolchain_paths

# PREPARING FLAGS
TARGET_HOST=$(android_get_target_host)
TARGET_MACHINE=$(android_get_target_machine)
CFLAGS=$(android_get_cflags "ffmpeg")
CXXFLAGS=$(android_get_cxxflags "ffmpeg")
LDFLAGS=$(android_get_ldflags "ffmpeg")
export PKG_CONFIG_PATH="${INSTALL_PKG_CONFIG_DIR}"

CONFIGURE_POSTFIX=""

for library in {1..26}
do
    if [[ ${!library} -eq 1 ]]; then
        ENABLED_LIBRARY=$(get_library_name $((library - 1)))

        echo -e "INFO: Enabling library ${ENABLED_LIBRARY}" >> ${BASEDIR}/build.log

        case $ENABLED_LIBRARY in
            fontconfig)
                CFLAGS+=" $(${HOST_PKG_CONFIG_PATH} --cflags fontconfig)"
                LDFLAGS+=" $(${HOST_PKG_CONFIG_PATH} --libs --static fontconfig)"
                CONFIGURE_POSTFIX+=" --enable-libfontconfig"
            ;;
            freetype)
                CFLAGS+=" $(${HOST_PKG_CONFIG_PATH} --cflags freetype2)"
                LDFLAGS+=" $(${HOST_PKG_CONFIG_PATH} --libs --static freetype2)"
                CONFIGURE_POSTFIX+=" --enable-libfreetype"
            ;;
            fribidi)
                CFLAGS+=" $(${HOST_PKG_CONFIG_PATH} --cflags fribidi)"
                LDFLAGS+=" $(${HOST_PKG_CONFIG_PATH} --libs --static fribidi)"
                CONFIGURE_POSTFIX+=" --enable-libfribidi"
            ;;
            gmp)
                CFLAGS+=" $(${HOST_PKG_CONFIG_PATH} --cflags gmp)"
                LDFLAGS+=" $(${HOST_PKG_CONFIG_PATH} --libs --static gmp)"
                CONFIGURE_POSTFIX+=" --enable-gmp"
            ;;
            gnutls)
                CFLAGS+=" $(${HOST_PKG_CONFIG_PATH} --cflags gnutls)"
                LDFLAGS+=" $(${HOST_PKG_CONFIG_PATH} --libs --static gnutls)"
                CONFIGURE_POSTFIX+=" --enable-gnutls"
            ;;
            lame)
                CFLAGS+=" $(${HOST_PKG_CONFIG_PATH} --cflags libmp3lame)"
                LDFLAGS+=" $(${HOST_PKG_CONFIG_PATH} --libs --static libmp3lame)"
                CONFIGURE_POSTFIX+=" --enable-libmp3lame"
            ;;
            libass)
                CFLAGS+=" $(${HOST_PKG_CONFIG_PATH} --cflags libass)"
                LDFLAGS+=" $(${HOST_PKG_CONFIG_PATH} --libs --static libass)"
                CONFIGURE_POSTFIX+=" --enable-libass"
            ;;
            libiconv)
                CFLAGS+=" $(${HOST_PKG_CONFIG_PATH} --cflags libiconv)"
                LDFLAGS+=" $(${HOST_PKG_CONFIG_PATH} --libs --static libiconv)"
                CONFIGURE_POSTFIX+=" --enable-iconv"
            ;;
            libtheora)
                CFLAGS+=" $(${HOST_PKG_CONFIG_PATH} --cflags theora)"
                LDFLAGS+=" $(${HOST_PKG_CONFIG_PATH} --libs --static theora)"
                CONFIGURE_POSTFIX+=" --enable-libtheora"
            ;;
            libvorbis)
                CFLAGS+=" $(${HOST_PKG_CONFIG_PATH} --cflags vorbis)"
                LDFLAGS+=" $(${HOST_PKG_CONFIG_PATH} --libs --static vorbis)"
                CONFIGURE_POSTFIX+=" --enable-libvorbis"
            ;;
            libvpx)
                CFLAGS+=" $(${HOST_PKG_CONFIG_PATH} --cflags vpx)"
                LDFLAGS+=" $(${HOST_PKG_CONFIG_PATH} --libs vpx)"
                CONFIGURE_POSTFIX+=" --enable-libvpx"
            ;;
            libwebp)
                CFLAGS+=" $(${HOST_PKG_CONFIG_PATH} --cflags libwebp)"
                LDFLAGS+=" $(${HOST_PKG_CONFIG_PATH} --libs --static libwebp)"
                CONFIGURE_POSTFIX+=" --enable-libwebp"
            ;;
            libxml2)
                CFLAGS+=" $(${HOST_PKG_CONFIG_PATH} --cflags libxml-2.0)"
                LDFLAGS+=" $(${HOST_PKG_CONFIG_PATH} --libs --static libxml-2.0)"
                CONFIGURE_POSTFIX+=" --enable-libxml2"
            ;;
            opencore-amr)
                CFLAGS+=" $(${HOST_PKG_CONFIG_PATH} --cflags opencore-amrnb)"
                CFLAGS+=" $(${HOST_PKG_CONFIG_PATH} --cflags opencore-amrwb)"
                LDFLAGS+=" $(${HOST_PKG_CONFIG_PATH} --libs --static opencore-amrnb)"
                LDFLAGS+=" $(${HOST_PKG_CONFIG_PATH} --libs --static opencore-amrwb)"
                CONFIGURE_POSTFIX+=" --enable-libopencore-amrnb"
                CONFIGURE_POSTFIX+=" --enable-libopencore-amrwb"
            ;;
            shine)
                CFLAGS+=" $(${HOST_PKG_CONFIG_PATH} --cflags shine)"
                LDFLAGS+=" $(${HOST_PKG_CONFIG_PATH} --libs --static shine)"
                CONFIGURE_POSTFIX+=" --enable-libshine"
            ;;
            speex)
                CFLAGS+=" $(${HOST_PKG_CONFIG_PATH} --cflags speex)"
                LDFLAGS+=" $(${HOST_PKG_CONFIG_PATH} --libs --static speex)"
                CONFIGURE_POSTFIX+=" --enable-libspeex"
            ;;
            wavpack)
                CFLAGS+=" $(${HOST_PKG_CONFIG_PATH} --cflags wavpack)"
                LDFLAGS+=" $(${HOST_PKG_CONFIG_PATH} --libs --static wavpack)"
                CONFIGURE_POSTFIX+=" --enable-libwavpack"
            ;;
            libogg)
                CFLAGS+=" $(${HOST_PKG_CONFIG_PATH} --cflags ogg)"
                LDFLAGS+=" $(${HOST_PKG_CONFIG_PATH} --libs --static ogg)"
            ;;
            libpng)
                CFLAGS+=" $(${HOST_PKG_CONFIG_PATH} --cflags libpng)"
                LDFLAGS+=" $(${HOST_PKG_CONFIG_PATH} --libs --static libpng)"
            ;;
            libuuid)
                CFLAGS+=" $(${HOST_PKG_CONFIG_PATH} --cflags uuid)"
                LDFLAGS+=" $(${HOST_PKG_CONFIG_PATH} --libs --static uuid)"
            ;;
            nettle)
                CFLAGS+=" $(${HOST_PKG_CONFIG_PATH} --cflags nettle)"
                LDFLAGS+=" $(${HOST_PKG_CONFIG_PATH} --libs --static nettle)"
                CFLAGS+=" $(${HOST_PKG_CONFIG_PATH} --cflags hogweed)"
                LDFLAGS+=" $(${HOST_PKG_CONFIG_PATH} --libs --static hogweed)"
            ;;
            android-zlib)
                CFLAGS+=" $(${HOST_PKG_CONFIG_PATH} --cflags zlib)"
                LDFLAGS+=" $(${HOST_PKG_CONFIG_PATH} --libs --static zlib)"
                CONFIGURE_POSTFIX+=" --enable-zlib"
            ;;
            android-media-codec)
                CONFIGURE_POSTFIX+=" --enable-mediacodec"
            ;;
        esac
    else
        if [[ ${library} -eq 25 ]]; then
            CONFIGURE_POSTFIX+=" --disable-zlib"
        fi
    fi
done

LDFLAGS+=" -L${ANDROID_NDK_ROOT}/platforms/android-${API}/arch-${ARCH}/usr/lib"

cd ${BASEDIR}/src/ffmpeg || exit 1

echo -n -e "\nffmpeg: "

make clean

./configure \
    --cross-prefix="${TARGET_HOST}-" \
    --sysroot="${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-${ARCH}/sysroot" \
    --prefix="${ANDROID_NDK_ROOT}/prebuilt/android-${ARCH}/ffmpeg" \
    --pkg-config="${HOST_PKG_CONFIG_PATH}" \
    --extra-cflags="${CFLAGS}" \
    --extra-cxxflags="${CXXFLAGS}" \
    --extra-ldflags="${LDFLAGS}" \
    --enable-version3 \
    --arch="${ARCH}" \
    --cpu="${TARGET_MACHINE}" \
    --target-os=android \
	--enable-cross-compile \
    --enable-pic \
    --enable-asm \
    --enable-jni \
    --enable-avresample \
    --disable-xmm-clobber-test \
    --disable-debug \
    --disable-neon-clobber-test \
    --enable-ffmpeg \
    --disable-ffplay \
    --disable-ffprobe \
    --disable-ffserver \
    --disable-doc \
    --disable-htmlpages \
    --disable-manpages \
    --disable-podpages \
    --disable-txtpages \
    --enable-inline-asm \
	--enable-neon \
	--enable-thumb \
	--enable-optimizations \
	--enable-small  \
    --enable-static \
    --disable-shared \
    --disable-xlib \
    ${CONFIGURE_POSTFIX} 1>>${BASEDIR}/build.log 2>>${BASEDIR}/build.log

if [ $? -ne 0 ]; then
    echo "failed"
    exit 1
fi

make -j$(nproc) 1>>${BASEDIR}/build.log 2>>${BASEDIR}/build.log

if [ $? -ne 0 ]; then
    echo "failed"
    exit 1
fi

make install 1>>${BASEDIR}/build.log 2>>${BASEDIR}/build.log

if [ $? -eq 0 ]; then
    echo "ok"
else
    echo "failed"
    exit 1
fi