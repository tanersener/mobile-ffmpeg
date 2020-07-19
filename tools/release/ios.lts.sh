#!/bin/bash
# 
# Creates a new ios lts release from current branch
#

export BASEDIR=$(pwd)
export SOURCE_PACKAGE="${BASEDIR}/../../prebuilt/ios-framework"
export COCOA_PACKAGE="${BASEDIR}/../../prebuilt/ios-cocoa"
export UNIVERSAL_PACKAGE="${BASEDIR}/../../prebuilt/ios-universal"
export ALL_UNIVERSAL_PACKAGES="${BASEDIR}/../../prebuilt/ios-all-universal"
export CUSTOM_OPTIONS="--disable-armv7s --lts --enable-ios-zlib --enable-ios-bzip2 --enable-ios-audiotoolbox --enable-ios-videotoolbox"
export GPL_PACKAGES="--enable-gpl --enable-libvidstab --enable-x264 --enable-x265 --enable-xvidcore"
export FULL_PACKAGES="--enable-fontconfig --enable-freetype --enable-fribidi --enable-gmp --enable-gnutls --enable-kvazaar --enable-lame --enable-libaom --enable-libass --enable-libilbc --enable-libtheora --enable-libvorbis --enable-libvpx --enable-libwebp --enable-libxml2 --enable-opencore-amr --enable-opus --enable-shine --enable-snappy --enable-soxr --enable-speex --enable-twolame --enable-wavpack"

create_package() {
    local PACKAGE_NAME="mobile-ffmpeg-$1"
    local PACKAGE_VERSION="$2"

    local CURRENT_PACKAGE="${COCOA_PACKAGE}/${PACKAGE_NAME}"
    rm -rf ${CURRENT_PACKAGE}
    mkdir -p ${CURRENT_PACKAGE} || exit 1

    cp -r ${SOURCE_PACKAGE}/* ${CURRENT_PACKAGE} || exit 1
    cd ${CURRENT_PACKAGE} || exit 1
    zip -r "../mobile-ffmpeg-$1-$2-ios-framework.zip" * || exit 1

    # COPY PODSPEC AS THE LAST ITEM
    cp ${BASEDIR}/ios/${PACKAGE_NAME}.podspec ${CURRENT_PACKAGE} || exit 1
    sed -i '' "s/VERSION/${PACKAGE_VERSION}/g" ${CURRENT_PACKAGE}/${PACKAGE_NAME}.podspec || exit 1

    mkdir -p ${ALL_UNIVERSAL_PACKAGES} || exit 1
    local CURRENT_UNIVERSAL_PACKAGE="${ALL_UNIVERSAL_PACKAGES}/${PACKAGE_NAME}-universal"
    rm -rf ${CURRENT_UNIVERSAL_PACKAGE}
    mkdir -p ${CURRENT_UNIVERSAL_PACKAGE}/include || exit 1
    mkdir -p ${CURRENT_UNIVERSAL_PACKAGE}/lib || exit 1

    cd ${UNIVERSAL_PACKAGE} || exit 1
    find . -name "*.a" -exec cp {} ${CURRENT_UNIVERSAL_PACKAGE}/lib \;  || exit 1

    # COPY LICENSE FILE OF EACH LIBRARY
    LICENSE_FILES=$(find . -name LICENSE | grep -vi ffmpeg)

    for LICENSE_FILE in ${LICENSE_FILES[@]}
    do
        LIBRARY_NAME=$(echo ${LICENSE_FILE} | sed 's/\.\///g;s/-universal\/LICENSE//g')
        cp ${LICENSE_FILE} ${CURRENT_UNIVERSAL_PACKAGE}/LICENSE.${LIBRARY_NAME} || exit 1
    done

    cp -r ${UNIVERSAL_PACKAGE}/mobile-ffmpeg-universal/include/* ${CURRENT_UNIVERSAL_PACKAGE}/include || exit 1
    cp -r ${UNIVERSAL_PACKAGE}/ffmpeg-universal/include/* ${CURRENT_UNIVERSAL_PACKAGE}/include || exit 1
    cp ${UNIVERSAL_PACKAGE}/ffmpeg-universal/LICENSE ${CURRENT_UNIVERSAL_PACKAGE}/LICENSE || exit 1

    cd ${ALL_UNIVERSAL_PACKAGES} || exit 1
    zip -r "${PACKAGE_NAME}-${PACKAGE_VERSION}-ios-static-universal.zip" ${PACKAGE_NAME}-universal || exit 1
}

if [[ $# -ne 1 ]];
then
    echo "Usage: ios.sh <version name>"
    exit 1
fi

# VALIDATE VERSIONS
MOBILE_FFMPEG_VERSION=$(grep 'MOBILE_FFMPEG_VERSION ' ${BASEDIR}/../../ios/src/MobileFFmpeg.m | grep -Eo '\".*\"' | sed -e 's/\"//g')
if [[ "${MOBILE_FFMPEG_VERSION}" != "$1" ]]; then
    echo "Error: version mismatch. v$1 requested but v${MOBILE_FFMPEG_VERSION} found. Please perform the following updates and try again."
    echo "1. Update docs"
    echo "2. Update android/app/build.gradle file versions"
    echo "3. Update tools/release scripts' descriptions"
    echo "4. Update podspec links"
    echo "5. Update mobileffmpeg.h versions for both android and ios"
    echo "6. Update versions in Doxyfile"
    exit 1
fi

# CREATE COCOA DIRECTORY
rm -rf ${COCOA_PACKAGE}
mkdir -p ${COCOA_PACKAGE} || exit 1

rm -rf ${ALL_UNIVERSAL_PACKAGES}
mkdir -p ${ALL_UNIVERSAL_PACKAGES} || exit 1

# MIN RELEASE
cd ${BASEDIR}/../.. || exit 1
./ios.sh ${CUSTOM_OPTIONS} || exit 1
create_package "min" "$1.LTS" || exit 1

# MIN-GPL RELEASE
cd ${BASEDIR}/../.. || exit 1
./ios.sh ${CUSTOM_OPTIONS} ${GPL_PACKAGES} || exit 1
create_package "min-gpl" "$1.LTS" || exit 1

# HTTPS RELEASE
cd ${BASEDIR}/../.. || exit 1
./ios.sh ${CUSTOM_OPTIONS} --enable-gnutls --enable-gmp || exit 1
create_package "https" "$1.LTS" || exit 1

# HTTPS-GPL RELEASE
cd ${BASEDIR}/../.. || exit 1
./ios.sh ${CUSTOM_OPTIONS} --enable-gnutls --enable-gmp ${GPL_PACKAGES} || exit 1
create_package "https-gpl" "$1.LTS" || exit 1

# AUDIO RELEASE
cd ${BASEDIR}/../.. || exit 1
./ios.sh ${CUSTOM_OPTIONS} --enable-lame --enable-libilbc --enable-libvorbis --enable-opencore-amr --enable-opus --enable-shine --enable-soxr --enable-speex --enable-twolame --enable-wavpack || exit 1
create_package "audio" "$1.LTS" || exit 1

# VIDEO RELEASE
cd ${BASEDIR}/../.. || exit 1
./ios.sh ${CUSTOM_OPTIONS} --enable-fontconfig --enable-freetype --enable-fribidi --enable-kvazaar --enable-libaom --enable-libass --enable-libtheora --enable-libvpx --enable-snappy --enable-libwebp || exit 1
create_package "video" "$1.LTS" || exit 1

# FULL RELEASE
cd ${BASEDIR}/../.. || exit 1
./ios.sh ${CUSTOM_OPTIONS} ${FULL_PACKAGES} || exit 1
create_package "full" "$1.LTS" || exit 1

# FULL-GPL RELEASE
cd ${BASEDIR}/../.. || exit 1
./ios.sh ${CUSTOM_OPTIONS} ${FULL_PACKAGES} ${GPL_PACKAGES} || exit 1
create_package "full-gpl" "$1.LTS" || exit 1