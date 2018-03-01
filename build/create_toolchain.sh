#!/bin/bash

$ANDROID_NDK/build/tools/make_standalone_toolchain.py --arch $ARCH --api $API --stl libc++ --install-dir "$ANDROID_NDK/toolchains/mobile-ffmpeg-"$ARCH
