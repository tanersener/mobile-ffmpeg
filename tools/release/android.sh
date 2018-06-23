#!/bin/bash
# 
# Creates a new android release from current branch
#

if [ $# -ne 2 ];
then
    echo "Usage: release.sh <version code> <version name>"
    exit 1
fi

export BASEDIR=$(pwd)

# MIN RELEASE
cd ${BASEDIR}/../.. || exit 1
./android.sh || exit 1
cd ${BASEDIR}/../../android/app || exit 1
gradle -b ${BASEDIR}/../../android/app/release.template.gradle -PreleaseVersionCode=$1 -PreleaseVersionName=$2 -PreleaseProject=mobile-ffmpeg-min -PreleaseProjectDescription='Includes FFmpeg v3.4.2.' clean install bintrayUpload || exit 1

# MIN-GPL RELEASE
cd ${BASEDIR}/../.. || exit 1
./android.sh --enable-gpl --enable-x264 --enable-xvidcore || exit 1
cd ${BASEDIR}/../../android/app || exit 1
gradle -b ${BASEDIR}/../../android/app/release.template.gradle -PreleaseVersionCode=$1 -PreleaseVersionName=$2 -PreleaseProject=mobile-ffmpeg-min-gpl -PreleaseProjectDescription='Includes FFmpeg v3.4.2 with x264 v20180606-2245-stable and xvidcore v1.3.5 libraries enabled.' -PreleaseGPL=1 clean install bintrayUpload || exit 1

# HTTPS RELEASE
cd ${BASEDIR}/../.. || exit 1
./android.sh --enable-gnutls || exit 1
cd ${BASEDIR}/../../android/app || exit 1
gradle -b ${BASEDIR}/../../android/app/release.template.gradle -PreleaseVersionCode=$1 -PreleaseVersionName=$2 -PreleaseProject=mobile-ffmpeg-https -PreleaseProjectDescription='Includes FFmpeg v3.4.2 with gnutls v3.5.18 library enabled.' clean install bintrayUpload || exit 1

# HTTPS-GPL RELEASE
cd ${BASEDIR}/../.. || exit 1
./android.sh --enable-gnutls --enable-gpl --enable-x264 --enable-xvidcore || exit 1
cd ${BASEDIR}/../../android/app || exit 1
gradle -b ${BASEDIR}/../../android/app/release.template.gradle -PreleaseVersionCode=$1 -PreleaseVersionName=$2 -PreleaseProject=mobile-ffmpeg-https-gpl -PreleaseProjectDescription='Includes FFmpeg v3.4.2 with gnutls v3.5.18, x264 v20180606-2245-stable and xvidcore v1.3.5 libraries enabled.' -PreleaseGPL=1 clean install bintrayUpload || exit 1

# FULL RELEASE
cd ${BASEDIR}/../.. || exit 1
./android.sh --full || exit 1
cd ${BASEDIR}/../../android/app || exit 1
gradle -b ${BASEDIR}/../../android/app/release.template.gradle -PreleaseVersionCode=$1 -PreleaseVersionName=$2 -PreleaseProject=mobile-ffmpeg-full -PreleaseProjectDescription='Includes FFmpeg v3.4.2 with fontconfig v2.12.93, freetype v2.9, fribidi v1.0.1, gmp v6.1.2, gnutls v3.5.18, kvazaar v1.2.0, libiconv v1.15, lame v3.100, libass v0.14.0, libilbc v2.0.2, libtheora v1.1.1, libvorbis v1.3.5, libvpx v1.7.0, libwebp v0.6.1, libxml2 v2.9.7, opencore-amr v0.1.5, opus v1.2.1, shine v3.1.1, snappy v1.1.7, speex v1.2.0 and wavpack v5.1.0 libraries enabled.' clean install bintrayUpload || exit 1

# FULL-GPL RELEASE
cd ${BASEDIR}/../.. || exit 1
./android.sh --full --enable-gpl --enable-x264 --enable-xvidcore || exit 1
cd ${BASEDIR}/../../android/app || exit 1
gradle -b ${BASEDIR}/../../android/app/release.template.gradle -PreleaseVersionCode=$1 -PreleaseVersionName=$2 -PreleaseProject=mobile-ffmpeg-full-gpl -PreleaseProjectDescription='Includes FFmpeg v3.4.2 with fontconfig v2.12.93, freetype v2.9, fribidi v1.0.1, gmp v6.1.2, gnutls v3.5.18, kvazaar v1.2.0, libiconv v1.15, lame v3.100, libass v0.14.0, libilbc v2.0.2, libtheora v1.1.1, libvorbis v1.3.5, libvpx v1.7.0, libwebp v0.6.1, libxml2 v2.9.7, opencore-amr v0.1.5, opus v1.2.1, shine v3.1.1, snappy v1.1.7, speex v1.2.0, wavpack v5.1.0, x264 v20180606-2245-stable and xvidcore v1.3.5 libraries enabled.' -PreleaseGPL=1 clean install bintrayUpload || exit 1
