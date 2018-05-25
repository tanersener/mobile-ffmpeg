#!/bin/bash

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
if [[ -z ${HOST_PKG_CONFIG_PATH} ]]; then
    echo "pkg-config not found"
    exit 1
fi

# ENABLE COMMON FUNCTIONS
. ${BASEDIR}/build/android-common.sh

# PREPARING PATHS & DEFINING ${INSTALL_PKG_CONFIG_DIR}
set_toolchain_clang_paths

# PREPARING FLAGS
TARGET_HOST=$(get_target_host)
CFLAGS=$(get_cflags "ffmpeg")
CXXFLAGS=$(get_cxxflags "ffmpeg")
LDFLAGS=$(get_ldflags "ffmpeg")
export PKG_CONFIG_PATH="${INSTALL_PKG_CONFIG_DIR}"

TARGET_CPU=""
TARGET_ARCH=""
NEON_FLAG=""
case ${ARCH} in
    arm-v7a)
        TARGET_CPU="armv7-a"
        TARGET_ARCH="armv7-a"
        NEON_FLAG="	--disable-neon --enable-asm"
    ;;
    arm-v7a-neon)
        TARGET_CPU="armv7-a"
        TARGET_ARCH="armv7-a"
        NEON_FLAG="	--enable-neon --enable-asm"
    ;;
    arm64-v8a)
        TARGET_CPU="armv8-a"
        TARGET_ARCH="aarch64"
        NEON_FLAG="	--enable-neon --enable-asm"
    ;;
    x86)
        TARGET_CPU="i686"
        TARGET_ARCH="i686"

        # asm disabled due to this ticker https://trac.ffmpeg.org/ticket/4928
        NEON_FLAG="	--disable-neon --disable-asm"
    ;;
    x86-64)
        TARGET_CPU="x86_64"
        TARGET_ARCH="x86_64"
        NEON_FLAG="	--disable-neon --enable-asm"
    ;;
esac

CONFIGURE_POSTFIX=""

for library in {1..27}
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
            kvazaar)
                CFLAGS+=" $(${HOST_PKG_CONFIG_PATH} --cflags kvazaar)"
                LDFLAGS+=" $(${HOST_PKG_CONFIG_PATH} --libs --static kvazaar)"
                CONFIGURE_POSTFIX+=" --enable-libkvazaar"
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
                LDFLAGS+=" $(${HOST_PKG_CONFIG_PATH} --libs --static cpufeatures)"
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
        if [[ ${library} -eq 26 ]]; then
            CONFIGURE_POSTFIX+=" --disable-zlib"
        fi
    fi
done

LDFLAGS+=" -L${ANDROID_NDK_ROOT}/platforms/android-${API}/arch-${TOOLCHAIN_ARCH}/usr/lib"

cd ${BASEDIR}/src/ffmpeg || exit 1

echo -n -e "\nffmpeg: "

make distclean 2>/dev/null 1>/dev/null

./configure \
    --cross-prefix="${TARGET_HOST}-" \
    --sysroot="${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-${TOOLCHAIN}/sysroot" \
    --prefix="${BASEDIR}/prebuilt/android-$(get_target_build)/ffmpeg" \
    --pkg-config="${HOST_PKG_CONFIG_PATH}" \
    --extra-cflags="${CFLAGS}" \
    --extra-cxxflags="${CXXFLAGS}" \
    --extra-ldflags="${LDFLAGS}" \
    --enable-version3 \
    --arch="${TARGET_ARCH}" \
    --cpu="${TARGET_CPU}" \
    --target-os=android \
    ${NEON_FLAG} \
    --enable-cross-compile \
    --enable-pic \
    --enable-jni \
    --enable-inline-asm \
    --enable-optimizations \
    --enable-small  \
    --enable-swscale \
    --enable-shared \
    --disable-xmm-clobber-test \
    --disable-debug \
    --disable-neon-clobber-test \
    --disable-ffmpeg \
    --disable-ffplay \
    --disable-ffprobe \
    --disable-ffserver \
    --disable-videotoolbox \
    --disable-doc \
    --disable-htmlpages \
    --disable-manpages \
    --disable-podpages \
    --disable-txtpages \
    --disable-static \
    --disable-xlib \
    ${CONFIGURE_POSTFIX} 1>>${BASEDIR}/build.log 2>>${BASEDIR}/build.log

if [ $? -ne 0 ]; then
    echo "failed"
    exit 1
fi

make -j$(get_cpu_count) 1>>${BASEDIR}/build.log 2>>${BASEDIR}/build.log

if [ $? -ne 0 ]; then
    echo "failed"
    exit 1
fi

make install 1>>${BASEDIR}/build.log 2>>${BASEDIR}/build.log

if [ $? -ne 0 ]; then
    echo "failed"
    exit 1
fi

# MANUALLY ADD CONFIG HEADER
cp -f ${BASEDIR}/src/ffmpeg/config.h ${BASEDIR}/prebuilt/android-$(get_target_build)/ffmpeg/include

if [ $? -eq 0 ]; then
    echo "ok"
else
    echo "failed"
    exit 1
fi
