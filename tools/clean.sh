#!/bin/bash

rm -rf ../prebuilt
rm -rf ../.tmp
rm -f ../build.log

rm -rf ../android/build
rm -rf ../android/app/build
rm -rf ../android/obj
rm -rf ../android/libs
rm -rf ../android/test-app/build

rm -rf ../ios/test-app/pods-with-tooltips/Pods
rm -rf ../ios/test-app/manual-frameworks/Pods
rm -rf ../ios/test-app/manual-frameworks/*.framework
rm -rf ../ios/test-app/universal/Pods
rm -rf ../ios/test-app/universal/*.framework
rm -rf ../ios/test-app/universal/mobile-ffmpeg-universal

rm -rf ../tvos/test-app/pods/Pods
rm -rf ../tvos/test-app/pods/*.framework
rm -rf ../tvos/test-app/manual-frameworks/Pods
rm -rf ../tvos/test-app/manual-frameworks/*.framework
rm -rf ../tvos/test-app/universal/Pods
rm -rf ../tvos/test-app/universal/*.framework
rm -rf ../tvos/test-app/universal/mobile-ffmpeg-universal

rm -rf ../src/libvidstab
rm -rf ../src/x264
rm -rf ../src/x265
rm -rf ../src/xvidcore
rm -rf ../src/rubberband
