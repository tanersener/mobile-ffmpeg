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
