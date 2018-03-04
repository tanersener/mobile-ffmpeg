#!/bin/bash

${ANDROID_NDK_ROOT}/build/tools/make_standalone_toolchain.py --arch ${ARCH} --api ${API} --stl libc++ --install-dir "${ANDROID_NDK_ROOT}/toolchains/mobile-ffmpeg-"${ARCH}
