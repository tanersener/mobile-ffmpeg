#!/bin/bash

cd ../prebuilt/tvos-universal || exit 1
find . -name "*.a" -not -path "./mobile-ffmpeg-universal/*" -exec cp {} ./mobile-ffmpeg-universal/lib \; || exit 1
cp -r ffmpeg-universal/include/* mobile-ffmpeg-universal/include || exit 1

# COPY LICENSE FILE OF EACH LIBRARY
LICENSE_FILES=$(find . -name LICENSE | grep -vi ffmpeg)
for LICENSE_FILE in ${LICENSE_FILES[@]}
do
    LIBRARY_NAME=$(echo ${LICENSE_FILE} | sed 's/\.\///g;s/-universal\/LICENSE//g')
    cp ${LICENSE_FILE} mobile-ffmpeg-universal/LICENSE.${LIBRARY_NAME} || exit 1
done
