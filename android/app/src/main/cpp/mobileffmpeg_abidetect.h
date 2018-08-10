/*
 * Copyright (c) 2018 Taner Sener
 *
 * This file is part of MobileFFmpeg.
 *
 * MobileFFmpeg is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MobileFFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with MobileFFmpeg.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MOBILE_FFMPEG_ABIDETECT_H
#define MOBILE_FFMPEG_ABIDETECT_H

#include <jni.h>
#include "mobileffmpeg.h"

/** Represents armeabi-v7a ABI with NEON support. */
#define ABI_ARMV7A_NEON "armeabi-v7a-neon"

/** Represents armeabi-v7a ABI. */
#define ABI_ARMV7A "armeabi-v7a"

/** Represents armeabi ABI. */
#define ABI_ARM "armeabi"

/** Represents x86 ABI. */
#define ABI_X86 "x86"

/** Represents x86_64 ABI. */
#define ABI_X86_64 "x86_64"

/** Represents arm64-v8a ABI. */
#define ABI_ARM64_V8A "arm64-v8a"

/** Represents not supported ABIs. */
#define ABI_UNKNOWN "unknown"

/*
 * Class:     com_arthenica_mobileffmpeg_AbiDetect
 * Method:    getAbi
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_arthenica_mobileffmpeg_AbiDetect_getAbi(JNIEnv *, jclass);

#endif /* MOBILE_FFMPEG_ABIDETECT_H */
